"""Memory byte oracle and real execution, including signed narrow loads."""
import pathlib
import subprocess
import sys
import tempfile
from tins import parse
from unisa.emit_arm import encode


def call(cmd):
    return subprocess.run(cmd,capture_output=True,timeout=60)


def check(args, native_host):
    with tempfile.TemporaryDirectory() as tmp:
        d=pathlib.Path(tmp); f=d/'in'
        cmds=[[args[0],args[1]], [sys.executable,'exec/pp/sim.py',args[2]]]
        lines=[]
        for w in (1,2,4,8):
            for off in (0,1,-1,-256,-257,255,256,4095*w,4096*w,32769,-32769,-9223372036854775808):
                lines+=['.ld x0, x1, %d, %d'%(off,w),'.st x1, %d, x2, %d'%(off,w)]
        lines+=['load64 x16, x1, 32768','store64 x1, -257, x2', '.ld x0, x16, 0, 8','.st x1, 0, x16, 8']
        f.write_text('\n'.join(lines)+'\n');tp=parse(f.read_text());parts=[encode(i,0,{}) for i in tp.code]
        assert all(x is not None for x in parts); want=b''.join(parts)
        for cmd in cmds:
            r=call(cmd+[str(f)]);assert r.returncode==0 and r.stdout==want,(r.returncode,r.stderr)
        print('ARM64 memory:',len(lines),'instructions,',len(want),'bytes; both executors equal')
        for text in ['.ld x0, x1, 0, 3','.st x1, 0, x2, 16','.ld x0, x16, 32768, 8','.st x1, -257, x16, 8']:
            f.write_text(text+'\n')
            for cmd in cmds:
                r=call(cmd+[str(f)]);assert r.returncode==1 and not r.stdout and b'not covered' in r.stderr
        print('ARM64 memory: width and fallback scratch rejection passed')
        if not native_host:
            print('ARM64 memory native: SKIPPED (needs macOS arm64)'); return
        asm=['.text']; decl=[]; checks=[]; n=0
        for wd in (1,2,4,8):
            for off in (0,1,-256,-257,255,256,4095*wd,4096*wd):
                for store in (False,True):
                    text=('.st x0, %d, x1, %d' if store else '.ld x0, x0, %d, %d')%(off,wd)
                    f.write_text(text+'\n');r=call(cmds[0]+[str(f)]);assert r.returncode==0
                    name='m%d'%n
                    asm+=['.globl _'+name,'_'+name+':','.byte '+','.join(map(str,r.stdout)),'ret']
                    decl+=['extern U '+name+'(void *,U);']
                    if store:
                        checks+=['memset(a,0,sizeof a);memset(b,0,sizeof b); U v=0xfedcba9876543280ULL;memcpy(b+40000+(%d),&v,%d);%s(a+40000,v);if(memcmp(a,b,sizeof a))return %d;'%(off,wd,name,n+1)]
                    else:
                        typ={1:'int8_t',2:'int16_t',4:'int32_t',8:'int64_t'}[wd]
                        checks+=['memset(a,0x80,sizeof a);%s v;memcpy(&v,a+40000+(%d),%d);if(%s(a+40000,0)!=(U)(int64_t)v)return %d;'%(typ,off,wd,name,n+1)]
                    n+=1
        (d/'p.s').write_text('\n'.join(asm)+'\n')
        (d/'p.c').write_text('#include <stdint.h>\n#include <string.h>\ntypedef uint64_t U;\n'+ '\n'.join(decl)+'\nstatic unsigned char a[100000],b[100000];\nint main(void){'+''.join('{'+c+'}' for c in checks)+'return 0;}')
        r=call(['cc','-o',str(d/'p'),str(d/'p.c'),str(d/'p.s')]);assert r.returncode==0,r.stderr
        r=call([str(d/'p')]);assert r.returncode==0,r.returncode
        print('ARM64 memory native:',n,'load/store cases equal to C memcpy and signed-width values')
