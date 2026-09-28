#!/usr/bin/env python3
"""Library metadata is independently asserted, never inferred from classic tape.
Use private prepared JSON/TBL, C executor, token dumper and reference paths.
"""
import json,os,pathlib,struct,subprocess,sys,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[2]
sys.path[:0]=[str(ROOT/'exec/pp'),str(ROOT/'exec/c'),str(ROOT)]
import sim
from pack import build
SOURCE='''struct Pair { double d; int n; };
struct Pair exchange(int a,double b,float c,int *p,struct Pair s,int e,double f,int g,int h){struct Pair r;r.d=b+c+s.d+f;r.n=a+*p+s.n+e+g+h;return r;}
struct Box { struct Pair p; short a[3]; };
struct Box box(struct Box x){return x;}
union Any { int i; double d; };
union Any unionval(union Any x){return x;}
long many17(long a,long b,long c,long d,long e,long f,long g,long h,long i,long j,long k,long l,long m,long n,long o,long p,long q){return q;}
static long hidden(long x){return x;}
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
    tape=out[25:25+tn];meta=out[25+tn:];assert meta[:8]==b'USLSIG2\n'
    at=8
    def word():
        nonlocal at
        v=struct.unpack_from('<Q',meta,at)[0];at+=8;return v
    nodes=0
    def desc(level=0):
        nonlocal at,nodes
        nodes+=1;assert level<32 and nodes<=16384
        fields=tuple(word() for _ in range(7));tag=meta[at];at+=1;n=word();end=at+n
        assert end<=len(meta) and n<=16*1024*1024
        payload=None
        if tag in (1,2):
            count=word();assert count<=64
            payload=[(tuple(word() for _ in range(4)),desc(level+1)) for _ in range(count)]
        elif tag==3:payload=(word(),word(),desc(level+1))
        else:assert tag==0 and n==0
        assert at==end
        return fields+(tag,payload)
    count=word();assert count<=8192;records={}
    for _ in range(count):
        nodes=0;n=word();name=meta[at:at+n].decode();at+=n
        linkage,defined,var,mode=meta[at:at+4];at+=4;np=word();ret=desc()
        stored=word();assert stored==np<=1024
        params=[desc() for _ in range(stored)];supported=meta[at];at+=1
        assert name and name not in records and linkage in (0,1) and defined==1 and var in (0,1) and supported in (0,1)
        assert mode==(1 if var or np>6 else 0)
        records[name]=(linkage,var,np,ret,params,supported,mode)
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
        assert set(rec)=={'hidden','add','done','identity','floating','callback','many','varied','main','exchange','box','unionval','many17'},rec
        assert rec['hidden'][0]==1 and rec['hidden'][5]==0
        ex=rec['exchange'];assert ex[2]==9 and len(ex[4])==9 and ex[5:]==(1,1),ex
        assert [(p[3],p[4]) for p in ex[4]]==[(1,4),(3,8),(3,4),(2,8),(5,16),(1,4),(3,8),(1,4),(1,4)],ex
        assert ex[3][3:8]==(5,16,0,8,1),ex[3]
        members=ex[3][8];assert len(members)==2
        assert [x[0][0] for x in members]==[0,8],members
        assert [(x[1][3],x[1][4],x[1][6]) for x in members]==[(3,8,8),(1,4,4)],members
        assert ex[4][4][3:]==ex[3][3:],ex
        bx=rec['box'];assert bx[5]==1 and bx[3][3:8]==(5,24,0,8,1),bx
        assert bx[3][8][0][1][3:]==ex[3][3:],bx
        ar=bx[3][8][1][1];assert ar[3:8]==(5,6,0,2,3) and ar[8][0:2]==(3,2),ar
        un=rec['unionval'];assert un[5]==0 and un[3][7]==2 and un[3][4:7]==(8,0,8),un
        m17=rec['many17'];assert m17[2]==17 and len(m17[4])==17 and m17[5:]==(1,1),m17
        assert rec['add'][0:3]==(0,0,2) and rec['add'][5]==1
        assert [(p[3],p[4],p[5]) for p in rec['add'][4]]==[(1,4,0),(1,2,1)]
        assert rec['done'][2]==0 and rec['done'][3][3]==0 and rec['done'][5]==1
        assert rec['identity'][3][3:5]==(2,8) and rec['identity'][5]==1
        assert rec['floating'][3][3:5]==(3,8) and rec['floating'][5]==1
        assert rec['callback'][4][0][3]==4 and rec['callback'][5]==0,rec['callback']
        assert rec['many'][1:3]==(0,7) and rec['many'][5]==1
        assert rec['varied'][1]==1 and rec['varied'][5]==0
        assert rec['main'][3][3:6]==(1,4,0) and rec['main'][5]==1
        for bad in [b'',b'\1',struct.pack('<Q',0),struct.pack('<Q',2)]:assert execute(bad)[0]!='accept'
        repeated=SOURCE.replace('int main(void)', 'long add(int a,unsigned short b){return a;}\nint main(void)')
        src.write_text(repeated);twice=good([dumper,'-dump-tokens',src],env)
        assert execute(struct.pack('<Q',1),twice)[0]!='accept'
        for damaged in [wrapped[:-1],wrapped+b'X',b'X'+wrapped[1:]]:
            try:decode(damaged)
            except (AssertionError,struct.error,IndexError,UnicodeError):pass
            else:raise AssertionError('bad envelope accepted')
        print('E3 library metadata: C network=sim; default tape=classic; 13 independent signatures, complete nine-mixed/Pair layout; malformed resource/envelope and duplicate definition rejected')
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
