"""Integer/fixed-immediate/frame differential and native checks."""
import pathlib
import subprocess
import sys
import tempfile
from tins import parse
from unisa.emit_arm import encode


def run(cmd): return subprocess.run(cmd,capture_output=True,timeout=60)


def check(args,native):
    with tempfile.TemporaryDirectory() as tmp:
        d=pathlib.Path(tmp);f=d/'in';cmds=[[args[0],args[1]],[sys.executable,'exec/pp/sim.py',args[2]]]
        lines=[op+' x0, x1, x2' for op in ('.div','.udiv','.mod','.umod')]
        lines += ['sext x0, x1, %d'%w for w in (1,2,4,8)]
        lines += [op+' x0, x1, %d'%v for op in ('addi','subi') for v in (0,1,4095)]
        lines += ['lsli x0, x1, %d'%v for v in (0,1,32,63)]
        frames=[0,1,-1,4095,-4095,4096,-4096,65536,-65536,-9223372036854775808]
        lines += ['.frame %d'%v for v in frames]
        f.write_text('\n'.join(lines)+'\n');tp=parse(f.read_text());parts=[encode(i,0,{}) for i in tp.code];assert all(p is not None for p in parts)
        want=b''.join(parts)
        for cmd in cmds:
            r=run(cmd+[str(f)]);assert r.returncode==0 and r.stdout==want,(r.returncode,r.stderr,r.stdout.hex(),want.hex())
        print('ARM64 integer/frame:',len(lines),'instructions,',len(want),'bytes equal, both executors')
        bad=['.frame 9223372036854775808','.ld x0, x1, 9223372036854775808, 8','sext x0, x1, 3','sext x0, x1, 4294967297','.ld x0, x1, 0, 4294967297',
             'addi x0, x1, -1','subi x0, x1, 4096','lsli x0, x1, 64','.mod x0, x17, x2','.umod x0, x1, x17']
        for source in bad:
            f.write_text(source+'\n')
            for cmd in cmds:
                r=run(cmd+[str(f)]);assert r.returncode==1 and not r.stdout and b'not covered' in r.stderr
        print('ARM64 integer/frame: 10 signed-domain/field-width/scratch rejects, both executors')
        if not native:
            print('ARM64 integer/frame native: SKIPPED');return
        asm=['.text'];decl=[];checks=[];n=0
        def function(source,pre='',post=''):
            nonlocal n
            f.write_text(source+'\n');r=run(cmds[0]+[str(f)]);assert r.returncode==0,r.stderr
            name='p%d'%n;n+=1
            asm.extend(['.globl _'+name,'_'+name+':',pre,'.byte '+','.join(map(str,r.stdout)),post,'ret'])
            decl.append('extern U '+name+'(U,U,U);');return name
        for op in ('.div','.udiv','.mod','.umod'):
            name=function(op+' x0, x1, x2')
            signed=op in ('.div','.mod');exp='(S)v[a]' if signed else 'v[a]';den='(S)v[b]' if signed else 'v[b]'
            operator='%' if 'mod' in op else '/'
            checks.append('for(int a=0;a<5;a++)for(int b=0;b<5;b++){if(v[b]==0 || (v[a]==(1ULL<<63)&&v[b]==~0ULL))continue; if(%s(0,v[a],v[b])!=(U)((%s)%s(%s)))return %d;}'%(name,exp,operator,den,n))
        for wd,typ in ((1,'int8_t'),(2,'int16_t'),(4,'int32_t'),(8,'int64_t')):
            name=function('sext x0, x1, %d'%wd);checks.append('for(int a=0;a<5;a++)if(%s(0,v[a],0)!=(U)(S)(%s)v[a])return %d;'%(name,typ,n))
        for op,vals in (('addi',(0,4095)),('subi',(0,4095)),('lsli',(0,63))):
            for v in vals:
                name=function(op+' x0, x1, %d'%v);operator={'addi':'+','subi':'-','lsli':'<<'}[op]
                checks.append('for(int a=0;a<5;a++)if(%s(0,v[a],0)!=(v[a]%s%d))return %d;'%(name,operator,v,n))
        for v in frames:
            name=function('.frame %d'%v,'mov x7,#65536','mov x0,x7')
            checks.append('if(%s(0,0,0)!=%dULL)return %d;'%(name,(65536-v)&((1<<64)-1),n))
        (d/'p.s').write_text('\n'.join(asm)+'\n')
        (d/'p.c').write_text('#include <stdint.h>\ntypedef uint64_t U;typedef int64_t S;\n'+'\n'.join(decl)+'\nint main(void){U v[]={0,1,7,~0ULL,1ULL<<63};'+''.join(checks)+'return 0;}')
        r=run(['cc','-o',str(d/'p'),str(d/'p.c'),str(d/'p.s')]);assert r.returncode==0,r.stderr
        r=run([str(d/'p')]);assert r.returncode==0,r.returncode
        print('ARM64 integer/frame native:',n,'generated functions match defined C arithmetic/frame results')
