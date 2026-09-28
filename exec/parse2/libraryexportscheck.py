#!/usr/bin/env python3
"""Library metadata is independently asserted, never inferred from classic tape.
Use private prepared JSON/TBL, C executor, token dumper and reference paths.
"""
import json,os,pathlib,struct,subprocess,sys,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[2]
sys.path[:0]=[str(ROOT/'exec/pp'),str(ROOT/'exec/c'),str(ROOT)]
import sim
from pack import build
SOURCE='''static long hidden(long x){return x;}
long add(int a,unsigned short b){return a+b;}
void done(void){return;}
long *identity(long *p){return p;}
double floating(double d){return d;}
long callback(long (*f)(long),long x){return f(x);}
long many(long a,long b,long c,long d,long e,long f,long g){return g;}
long varied(long x,...){return x;}
int main(void){return 0;}
'''
def run(args,env=None):
    p=subprocess.run(list(map(str,args)),cwd=ROOT,capture_output=True,timeout=20,env=env)
    return p

def good(args,env=None):
    p=run(args,env);assert p.returncode==0,(args,p.returncode,p.stderr);return p.stdout

def decode(out):
    assert out[:9]==b'USLTAPE1\n';tn,mn=struct.unpack_from('<2Q',out,9)
    assert len(out)==25+tn+mn
    tape=out[25:25+tn];meta=out[25+tn:];assert meta[:8]==b'USLSIG1\n'
    count=struct.unpack_from('<Q',meta,8)[0];at=16;records={}
    for _ in range(count):
        n=struct.unpack_from('<Q',meta,at)[0];at+=8;name=meta[at:at+n].decode();at+=n
        linkage,defined,var=meta[at:at+3];at+=3
        np=struct.unpack_from('<Q',meta,at)[0];at+=8
        ret=struct.unpack_from('<6Q',meta,at);at+=48
        stored=struct.unpack_from('<Q',meta,at)[0];at+=8;params=[]
        for _ in range(stored):params.append(struct.unpack_from('<6Q',meta,at));at+=48
        supported=meta[at];at+=1
        assert name and name not in records and linkage in (0,1) and defined==1 and var in (0,1) and supported in (0,1)
        assert stored==min(np,8)
        records[name]=(linkage,var,np,ret,params,supported)
    assert at==len(meta)
    return tape,records

def check(paths):
    js,tbl,core,dumper,ref=map(pathlib.Path,paths)
    delta=json.loads(js.read_text());loaded=sim.load(delta)
    with tempfile.TemporaryDirectory(prefix='unisacc-exports-check-') as name:
        d=pathlib.Path(name);src=d/'source.c';src.write_text(SOURCE)
        env=dict(os.environ,UA_TYPESPELL='1');tokens=good([dumper,'-dump-tokens',src],env);inp=d/'tokens';inp.write_bytes(tokens)
        net=d/'parse.net';good([sys.executable,ROOT/'exec/c/net.py',tbl,net]);good([core,'--check-net',tbl,net])
        manifest=d/'route.tsv';manifest.write_text('library\tparse\ttokens\ttape\tparse.net\n')
        resources=d/'resources';(resources/'library').mkdir(parents=True)
        def execute(flag,content=tokens):
            files=sim.Files();mounts=[]
            if flag is not None:
                files.cache[b'\0library/symbols']=flag;(resources/'library/symbols').write_bytes(flag);mounts=[('00',resources)]
            inp.write_bytes(content);pkg=d/'pkg';pkg.write_bytes(build([manifest],mounts,cache=False))
            verdict,out,_=sim.run(delta,content,'fixture',files,maxsteps=2000000,loaded=loaded)
            p=run([core,'--bundle',pkg,'library',inp])
            if verdict=='accept':assert p.returncode==0 and p.stdout==out,(verdict,p.returncode,p.stderr)
            else:assert p.returncode!=0 and not p.stdout,(verdict,p.returncode,p.stdout)
            return verdict,out
        r,plain=execute(None);assert r=='accept'
        classic=good([ref,src,'-S','-o','-']);assert classic==plain
        r,wrapped=execute(struct.pack('<Q',1));assert r=='accept'
        tape,rec=decode(wrapped);assert tape==plain
        assert set(rec)=={'hidden','add','done','identity','floating','callback','many','varied','main'},rec
        assert rec['hidden'][0]==1 and rec['hidden'][-1]==0
        assert rec['add'][0:3]==(0,0,2) and rec['add'][-1]==1
        assert [(p[3],p[4],p[5]) for p in rec['add'][4]]==[(1,4,0),(1,2,1)]
        assert rec['done'][2]==0 and rec['done'][3][3]==0 and rec['done'][-1]==1
        assert rec['identity'][3][3:5]==(2,8) and rec['identity'][-1]==1
        assert rec['floating'][3][3:5]==(3,8) and rec['floating'][-1]==0
        assert rec['callback'][4][0][3]==4 and rec['callback'][-1]==0,rec['callback']
        assert rec['many'][1:3]==(0,7) and rec['many'][-1]==0
        assert rec['varied'][1]==1 and rec['varied'][-1]==0
        assert rec['main'][3][3:6]==(1,4,0) and rec['main'][-1]==1
        for bad in [b'',b'\1',struct.pack('<Q',0),struct.pack('<Q',2)]:assert execute(bad)[0]!='accept'
        repeated=SOURCE.replace('int main(void)', 'long add(int a,unsigned short b){return a;}\nint main(void)')
        src.write_text(repeated);twice=good([dumper,'-dump-tokens',src],env)
        assert execute(struct.pack('<Q',1),twice)[0]!='accept'
        for damaged in [wrapped[:-1],wrapped+b'X',b'X'+wrapped[1:]]:
            try:decode(damaged)
            except (AssertionError,struct.error,IndexError,UnicodeError):pass
            else:raise AssertionError('bad envelope accepted')
        print('E3 library metadata: C network=sim; default tape=classic; 9 independent signatures; malformed resource/envelope and duplicate definition rejected')
def keep(shard,tbl,core,dumper,ref):
    a,z=map(int,shard.split('/'));assert 1<=a<=z
    names=(ROOT/'exec/parse2/keep-e3.txt').read_text().split();assert names and len(names)==len(set(names))
    selected=names[a-1::z];assert selected
    env=dict(os.environ,UA_TYPESPELL='1')
    with tempfile.TemporaryDirectory(prefix='unisacc-library-default-') as d:
        inp=pathlib.Path(d)/'tokens'
        for f in selected:
            tokens=good([dumper,'-dump-tokens',f],env);inp.write_bytes(tokens)
            actual=good([core,tbl,inp,f]);expected=good([ref,f,'-S','-o','-'])
            assert actual==expected,('LOST',f)
    print('default library-resource-absent keep',shard,len(selected),'equal; full fixed list',len(names))

if __name__=='__main__':
    if len(sys.argv)==7 and sys.argv[1]=='--keep':keep(*sys.argv[2:])
    else:
        if len(sys.argv)!=6:sys.exit('usage: libraryexportscheck.py JSON TBL PRIVATE_CORE PRIVATE_DUMP PRIVATE_REF; or --keep N/M TBL CORE DUMP REF')
        check(sys.argv[1:])
