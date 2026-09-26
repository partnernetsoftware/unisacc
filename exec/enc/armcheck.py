"""ARM64 delta: reference bytes, both runtimes, and host assembler/execution.
Native check requires macOS arm64; other hosts report it skipped explicitly.
"""
import pathlib
import platform
import subprocess
import sys
import tempfile
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[2]))
from unisa.emit_arm import encode
from tins import parse


def run(cmd):
    return subprocess.run(cmd,capture_output=True,timeout=60)


def main():
    with tempfile.TemporaryDirectory() as tmp:
        d=pathlib.Path(tmp); inp=d/'input'
        lines=['mov x0, x1','mov x30, x29','ret','nop','callr x16']
        vals=[0,1,65535,65536,4294967296,9223372036854775807,9223372036854775808,18446744073709551615,-1,-9223372036854775808]
        lines += ['imm x%d, %d'%(i%31,v) for i,v in enumerate(vals)]
        ops=['add64','sub64','xor64','and64','or64','shl64','shr64','lshr64','mul64','slt64','sle64','eq','ne','ult64','ule64']
        lines += [op+' x%d, x%d, x%d'%regs for op in ops for regs in [(0,1,2),(30,29,28),(0,0,0)]]
        source='\n'.join(lines)+'\n'; inp.write_text(source)
        tp=parse(source); parts=[encode(i,0,{}) for i in tp.code]
        assert parts and all(p is not None for p in parts)
        want=b''.join(parts)
        cmds=[[sys.argv[1],sys.argv[2]], [sys.executable,'exec/pp/sim.py',sys.argv[3]]]
        for cmd in cmds:
            r=run(cmd+[str(inp)]); assert r.returncode==0 and r.stdout==want,(cmd,r.returncode,r.stderr,[(i,a,b) for i,(a,b) in enumerate(zip(r.stdout,want)) if a!=b][:12],len(r.stdout),len(want))
        print('ARM64 reference:',len(lines),'instructions,',len(want),'bytes; both executors equal')
        for bad in ['imm x0, 18446744073709551616','imm x0, -9223372036854775809','imm x0, -','mov x31, x0','mov x0, 1','add64 x0, x1','ret x0','mov x0, x1,','jump L','mov x0, x1 role=a','callr x17','callr x7']:
            inp.write_text(bad+'\n')
            for cmd in cmds:
                r=run(cmd+[str(inp)]); assert r.returncode==1 and not r.stdout and b'not covered' in r.stderr,(bad,r.returncode,r.stderr)
        print('ARM64 declared input boundary: 12 rejects on both executors')
        native_host = sys.platform=='darwin' and platform.machine()=='arm64'
        from armmemcheck import check
        check(sys.argv[1:],native_host)
        from armbranchcheck import check as branchcheck
        branchcheck(sys.argv[1:],native_host)
        from armintcheck import check as intcheck
        intcheck(sys.argv[1:],native_host)
        if not native_host:
            print('ARM64 native assembler/execution: SKIPPED (needs macOS arm64)'); return
        # Separate assembler reference, never calls the Python encoder.
        asm=['.text']; decl=[]; checks=[]
        native=[('add64','add x0,x1,x2'),('sub64','sub x0,x1,x2'),('xor64','eor x0,x1,x2'),('and64','and x0,x1,x2'),('or64','orr x0,x1,x2'),('mul64','mul x0,x1,x2'),('shl64','lsl x0,x1,x2'),('shr64','asr x0,x1,x2'),('lshr64','lsr x0,x1,x2')]
        native += [(op,'cmp x1,x2\ncset x0,'+cond) for op,cond in [('slt64','lt'),('sle64','le'),('eq','eq'),('ne','ne'),('ult64','lo'),('ule64','ls')]]
        for i,(op,ref) in enumerate(native):
            inp.write_text(op+' x0, x1, x2\n');r=run(cmds[0]+[str(inp)]);assert r.returncode==0
            asm+=['.globl _d%d'%i,'_d%d:'%i,'.byte '+','.join(map(str,r.stdout)),'ret','.globl _r%d'%i,'_r%d:'%i,ref,'ret']
            decl+=['extern U d%d(U,U,U),r%d(U,U,U);'%(i,i)]
            checks+=['for(int a=0;a<6;a++)for(int b=0;b<6;b++)if(d%d(0,v[a],v[b])!=r%d(0,v[a],v[b]))return %d;'%(i,i,i+1)]
        for i,v in enumerate(vals):
            inp.write_text('imm x0, %d\n'%v); r=run(cmds[0]+[str(inp)]); assert r.returncode==0
            asm+=['.globl _imm%d'%i,'_imm%d:'%i,'.byte '+','.join(map(str,r.stdout)),'ret']
            decl+=['extern U imm%d(void);'%i]
            checks+=['if(imm%d()!=%dULL)return %d;'%(i,v & ((1<<64)-1),30+i)]
        # Independent worked tape-call sequence: ADR x17,+16, software push,
        # BLR x16; software pop and RET x17. No relocation/layout involved.
        inp.write_text('callr x16\nret\n')
        r=run(cmds[0]+[str(inp)])
        assert r.returncode==0 and r.stdout.hex()=='91000010e72000d1f10000f900023fd6f10040f9e720009120025fd6'
        (d/'p.s').write_text('\n'.join(asm)+'\n')
        (d/'p.c').write_text('typedef unsigned long long U;\n'+'\n'.join(decl)+'\nint main(void){U v[]={0,1,63,64,~0ULL,1ULL<<63};'+''.join(checks)+'return 0;}')
        r=run(['cc','-o',str(d/'p'),str(d/'p.c'),str(d/'p.s')]);assert r.returncode==0,r.stderr
        r=run([str(d/'p')]);assert r.returncode==0,r.returncode
        print('ARM64 native delta bytes vs system assembler: 540 arithmetic/comparison + 10 constant executions equal')

if __name__=='__main__': main()
