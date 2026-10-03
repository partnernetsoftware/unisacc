#!/usr/bin/env python3
"""Optional raw-symbol delta contract: both executors, two ISAs, final addresses.
Run on a private frozen snapshot. No raw code entry is called as a host ABI.
"""
import copy,json,pathlib,signal,struct,subprocess,sys,tempfile,os
ROOT=pathlib.Path(__file__).resolve().parents[2]
sys.path[:0]=[str(ROOT/'exec/pp'),str(ROOT/'exec/c'),str(ROOT/'exec/enc'),str(ROOT)]
import sim
from pack import build
import sys as _ts, pathlib as _tp; _ts.path.insert(0, str(_tp.Path(__file__).resolve().parents[2] / 'tests' / 'enc'))  # tins: TIns text tool lives in tests/enc
from tins import parse
from unisa.assemble import assemble

def command(args):
    p=subprocess.Popen(list(map(str,args)),cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.PIPE,start_new_session=True)
    try: out,err=p.communicate(timeout=25)
    except subprocess.TimeoutExpired:
        os.killpg(p.pid,signal.SIGKILL);p.communicate();raise
    assert p.returncode==0,(args,p.returncode,err)
    return out

def decode(raw):
    assert raw[:8]==b'UNILIB1\n'
    text,extent,stored,entry=struct.unpack_from('<4Q',raw,8)
    at=40+text+stored
    assert raw[at:at+6]==b'SYMS1\n';at+=6
    n=struct.unpack_from('<Q',raw,at)[0];at+=8;items={}
    for _ in range(n):
        kind=raw[at];length,address=struct.unpack_from('<2Q',raw,at+1);at+=17
        name=raw[at:at+length].decode('ascii');at+=length
        assert kind in (0,1) and name and name not in items
        items[name]=(kind,address)
    assert at==len(raw)
    return items,raw[8:40+text+stored]

def check():
    with tempfile.TemporaryDirectory(prefix='unisacc-libsymbols-') as name:
        d=pathlib.Path(name)
        command(['cc','-O2','-I',ROOT/'exec/c',ROOT/'exec/c/run.c','-o',d/'run'])
        for arch,script,reg,expected in [
            ('x86_64','enc','rax',{'_start':0,'__init':2,'main':3,'end':9}),
            ('arm64','enc/arm','x0',{'_start':0,'__init':4,'main':16,'end':32})]:
            js=d/(arch+'.json');tbl=d/(arch+'.tbl');net=d/(arch+'.net')
            command([sys.executable,ROOT/'exec/build/gen.py',script,js,'--elf'])
            command([sys.executable,ROOT/'exec/c/tbl.py',js,tbl])
            command([sys.executable,ROOT/'exec/c/net.py',tbl,net])
            command([d/'run','--check-net',tbl,net])
            delta=json.loads(js.read_text());loaded=sim.load(delta)
            body=f"_start:\njump main reloc={'arm26' if arch=='arm64' else 'rel32'}\n__init:\nret\nmain:\nimm {reg}, 7\nret\nend:\n"
            x=(f'@target lnx/{arch}\n@data '+32*'00'+'\n@argc 256\n@argv 264\n@sym g_counter 272\n'+body).encode()
            vals={'process/argc':1,'process/argv':123,'memory/text':0x10000000,'memory/data':0x20000000}
            resources=d/'resources';resources.mkdir(exist_ok=True)
            for key,value in vals.items():
                f=resources/key;f.parent.mkdir(exist_ok=True,parents=True);f.write_bytes(struct.pack('<Q',value))
            manifest=d/'route.tsv';manifest.write_text(f'library\tencode\ttarget.text\tmemory-v1\t{arch}.net\n')
            inp=d/'input';inp.write_bytes(x)
            def execute(content,flag):
                files=sim.Files()
                for k,v in vals.items():files.cache[b'\0'+k.encode()]=struct.pack('<Q',v)
                f=resources/'library/symbols';f.parent.mkdir(exist_ok=True)
                if flag is None:
                    try:f.unlink()
                    except FileNotFoundError:pass
                else:files.cache[b'\0library/symbols']=flag;f.write_bytes(flag)
                inp.write_bytes(content);pkg=d/'route.pkg';pkg.write_bytes(build([manifest],[('00',resources)] if vals or flag is not None else [],cache=False))
                verdict,out,_=sim.run(delta,content,'fixture',files,maxsteps=1000000,loaded=loaded)
                p=subprocess.run([d/'run','--bundle',pkg,'library',inp],cwd=ROOT,capture_output=True,timeout=25)
                if verdict=='accept':assert p.returncode==0 and p.stdout==out,(arch,p.returncode,p.stderr)
                else:assert p.returncode!=0 and not p.stdout,(arch,verdict,p.returncode,p.stdout)
                return verdict,out,files
            verdict,plain,files=execute(x,None);assert verdict=='accept' and plain[:8]==b'UNIMEM1\n'
            # Direct previous completion edge is an independent default-output control.
            previous=copy.deepcopy(delta)
            for _,row in previous['states'].values():
                for k,(n,q) in list(row.items()):
                    if n=='LIB.finish':row[k]=['RET',q]
            r,old,_=sim.run(previous,x,'fixture',files,maxsteps=1000000)
            assert r=='accept' and old==plain
            verdict,raw,_=execute(x,struct.pack('<Q',1));assert verdict=='accept'
            items,payload=decode(raw);assert payload==plain[8:]
            assert items=={**{k:(0,0x10000000+v) for k,v in expected.items()},
                           'g_counter':(1,0x20000010),'counter':(1,0x20000010)},items
            tp=parse(body,target='lnx/'+arch);ref,stats=assemble(tp)
            assert stats['encoded']==stats['insns'] and ref==plain[40:40+len(ref)]
            for bad in [b'',b'\1',struct.pack('<Q',0),struct.pack('<Q',2),bytes(9)]:
                r,_,_=execute(x,bad);assert r!='accept'
            collision=x.replace(b'g_counter',b'g_main')
            r,_,_=execute(collision,struct.pack('<Q',1));assert r!='accept'
            saved=dict(vals);vals.clear()
            for key in saved:(resources/key).unlink()
            file_input=x.replace(b'@argc 256\n@argv 264\n',b'')
            r,filebytes,_=execute(file_input,None);assert r=='accept' and filebytes[:4]==b'\x7fELF'
            r,_,_=execute(file_input,struct.pack('<Q',1));assert r!='accept'
            vals.update(saved)
            for key,value in vals.items():(resources/key).write_bytes(struct.pack("<Q",value))
            print(arch,': default bytes unchanged; raw map/ref bytes and final addresses; 5 malformed resources, nonmemory request and alias collision rejected; C network = sim')
if __name__=='__main__':check()
