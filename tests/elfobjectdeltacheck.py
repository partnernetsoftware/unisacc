"""Independent struct oracle for the object writer's checked-plan interface."""
import importlib.util
import json
import os
import subprocess
import tempfile
import struct
import sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT))
from exec.pp.sim import run
sys.path.insert(0,str(ROOT/"exec"))
import assemble


def fixture(arch,tape,work,runtime,bad=None):
    spec=importlib.util.spec_from_file_location("eogen",ROOT/"exec/parse/gen.py")
    E=importlib.util.module_from_spec(spec);spec.loader.exec_module(E)
    regions={k:(i+100)<<40 for i,k in enumerate(("text","data","strings","tape","symbols","relocs"))}
    text=b"\xc3\x90\0";data=b"a\0\0";strings=b"\0local\0entry\0missing\0"
    symbols=[(1,0,2,1),(7,18,1,0),(13,16,0,0)]
    rels=[(0,4,6,-4),(1,1,2,0)]
    regs=dict(eo_textlen=len(text),eo_nzend=len(data),eo_datalen=24,eo_nrel=len(rels),eo_nsym=len(symbols),eo_nlocal=1,eo_strsize=len(strings),eo_tapelen=len(tape))
    if bad=="short":regs["eo_datalen"]=2
    if bad=="negative":regs["eo_datalen"]=-1
    if bad=="negative-nzend":regs["eo_nzend"]=-1
    if bad=="symbol":rels[0]=(0,4,7,-4)
    if bad=="negative-symbol":rels[0]=(0,4,-1,-4)
    p=E.P("START")
    for k,v in regs.items():p.a(("LDI",k,v))
    for key,values in {"text":text,"data":data,"strings":strings,"tape":tape,"symbols":[v for row in symbols for v in row],"relocs":[v for row in rels for v in row]}.items():
        for i,v in enumerate(values):p.a(("LDI","i",i),("LDI","v",v),("STX","i",regions[key],"v"))
    assemble.run(ROOT/"exec/enc/elfobject-manifest.tsv",E,E.P,{},dict(regions,arch=arch))
    p.call("EO.write").goto("DONE")
    E.P("DONE").a(("ACCEPT",)).goto("DONE")
    E.g.finish()
    delta={"start":"START","states":{k:[m,{str(kk):v for kk,v in row.items()}] for k,(m,row) in E.g.st.items()},"seqs":E.g.seqs}
    status,out,steps=run(delta,b"","fixture",maxsteps=100000)
    model=work/"model.json";table=work/"model.tbl";net=work/"model.net"
    model.write_text(json.dumps(delta))
    command([sys.executable,str(ROOT/"exec/c/tbl.py"),str(model),str(table)])
    command([sys.executable,str(ROOT/"exec/c/net.py"),str(table),str(net)])
    checked=command([str(runtime),"--check-net",str(table),str(net)])
    assert checked.returncode==0
    for path in (table,net):
        actual=command([str(runtime),str(path),str(work/"empty")],check=False)
        if bad:
            reason="not covered: object relocation symbol" if "symbol" in bad else "not covered: object data extent"
            assert actual.returncode!=0 and reason.encode() in actual.stderr,(path,actual.returncode,actual.stderr)
        else:
            assert actual.returncode==0 and actual.stdout==out,(path,actual.returncode,actual.stderr)
    if bad:
        reason="not covered: object relocation symbol" if "symbol" in bad else "not covered: object data extent"
        assert status=="reject" and out[0]==reason,(status,out)
        return steps
    assert status=="accept",(status,out)
    align=lambda x,n:(x+n-1)&-n
    doff=align(64+len(text),16);roff=align(doff+len(data),8);soff=roff+24*len(rels)
    stoff=soff+24*(4+len(symbols));shsoff=stoff+len(strings)
    shstr=b"\0.text\0.data\0.bss\0.rela.text\0.symtab\0.strtab\0.shstrtab\0"+(b".unisa.tape\0" if tape else b"")
    toff=shsoff+len(shstr);shoff=align(toff+len(tape),8)
    expected=bytearray(struct.pack("<16sHHIQQQIHHHHHH",b"\x7fELF\x02\x01\x01"+bytes(9),1,183 if arch=="arm64" else 62,1,0,0,shoff,0,64,0,0,64,9 if tape else 8,7))
    def pad(n):expected.extend(bytes(n-len(expected)))
    expected.extend(text);pad(doff);expected.extend(data);pad(roff)
    for off,ty,sym,add in rels:expected.extend(struct.pack("<QQq",off,(sym<<32)|ty,add))
    pad(soff)
    for name,info,sec,val in [(0,0,0,0),(0,3,1,0),(0,3,2,0),(0,3,3,0)]+symbols:expected.extend(struct.pack("<IBBHQQ",name,info,0,sec,val,0))
    expected.extend(strings+shstr+tape);pad(shoff)
    rows=[(0,0,0,0,0,0,0,0,0),(1,1,6,64,len(text),0,0,16,0),(7,1,3,doff,len(data),0,0,16,0),(13,8,3,doff+len(data),24-len(data),0,0,16,0),(18,4,64,roff,24*len(rels),5,1,8,24),(29,2,0,soff,24*(4+len(symbols)),6,5,8,24),(37,3,0,stoff,len(strings),0,0,1,0),(45,3,0,shsoff,len(shstr),0,0,1,0)]
    if tape:rows.append((55,1,0,toff,len(tape),0,0,1,0))
    for name,ty,flags,off,size,link,info,al,ent in rows:expected.extend(struct.pack("<IIQQQQIIQQ",name,ty,flags,0,off,size,link,info,al,ent))
    assert out==expected,(arch,len(tape),len(out),len(expected),next((i for i,(a,b) in enumerate(zip(out,expected)) if a!=b),None))
    return steps


def command(argv,check=True):
    result=subprocess.run(argv,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=30)
    if check and result.returncode:
        raise AssertionError((argv,result.returncode,result.stderr.decode(errors="replace")))
    return result


if __name__=="__main__":
    with tempfile.TemporaryDirectory(prefix="elfobject-delta-") as temp:
        work=Path(temp);runtime=work/"run";(work/"empty").write_bytes(b"")
        command([os.environ.get("CC","cc"),"-O2","-std=c99","-w","-o",str(runtime),str(ROOT/"exec/c/run.c")])
        for arch in ("x86_64","arm64"):
            for tape in (b"",b"UNISATAPE1 lnx/x86_64 5\nret\n"):
                fixture(arch,tape,work,runtime)
        for bad in ("short","negative","negative-nzend","symbol","negative-symbol"):
            fixture("x86_64",b"",work,runtime,bad)
    print("elf object delta: sim/C table/C net 4 oracle-equal; 5 malformed plans rejected; 9 all-domain net=table checks")
