#!/usr/bin/env python3
'''Bounded bridge checks, private models/outputs, no shared compiler rebuild.
UA=/path/to/frozen/ref python3 exec/ffi/bridgecheck.py
Requires macOS arm64 plus Rosetta; absent native targets fail explicitly.
'''
import argparse, hashlib, math, os, platform, signal, subprocess, sys, tempfile, time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(ROOT),str(ROOT/'exec/enc')]
from unisa import image
from unisa.assemble import assemble
from unisa.catalog import REGMAP
from unisa.emit_arm import encode as encode_arm
from unisa.emit_x86 import encode as encode_x86
from unisa.lower import TIns, lower
from unisa.tape import parse as parse_tape
from unisa.__main__ import _oracle
from tins import parse as parse_tins
DEADLINE=0


def require(ok,why):
    if not ok: raise RuntimeError(why)


def call(command,maximum=15,okay=True):
    seconds=min(maximum,math.floor(DEADLINE-time.monotonic())-3)
    require(seconds>=1,'total 53-second bridgecheck budget exhausted')
    argv=[sys.executable,str(ROOT/'tests/bound.py'),str(seconds),*map(str,command)]
    p=subprocess.Popen(argv,cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.PIPE,start_new_session=True)
    try: out,err=p.communicate(timeout=seconds+2)
    except subprocess.TimeoutExpired:
        # Let bound.py clean its child's separate process group first.
        os.killpg(p.pid,signal.SIGTERM)
        try: p.communicate(timeout=2)
        except subprocess.TimeoutExpired:
            os.killpg(p.pid,signal.SIGKILL);p.communicate()
        raise RuntimeError('watchdog did not finish: '+str(command))
    result=subprocess.CompletedProcess(command,p.returncode,out,err)
    if okay: require(result.returncode==0,f'{command}: rc={p.returncode}, stderr={err[-1200:]!r}')
    return result


def model(d,name,generator,flags=()):
    js,tbl,net=[d/(name+s) for s in ('.json','.tbl','.net')]
    call([sys.executable,ROOT/generator,js,*flags])
    call([sys.executable,ROOT/'exec/c/tbl.py',js,tbl])
    call([sys.executable,ROOT/'exec/c/net.py',tbl,net])
    return tbl,net


def reject(runner,models,inp,text,empty=True):
    inp.write_text(text)
    for current in models:
        r=call([runner,current,inp],okay=False)
        require(r.returncode==1 and b'not covered' in r.stderr,f'wrong rejection: {text!r}: {r.returncode}, {r.stderr!r}')
        require(not empty or not r.stdout,'invalid encoder input leaked output')


def encodings(d,runner,arch,models):
    regs=REGMAP[arch]
    lines=['hostcall '+a+', '+b for a in regs for b in regs]
    lines+=['hostaddr '+a+', '+str(i) for a in regs for i in range(4)]
    raw='@target osx/'+arch+'\n@data -\n'+'\n'.join(lines)+'\n'
    inp=d/(arch+'.tins');inp.write_text(raw)
    expected,stats=assemble(parse_tins(raw))
    require(stats['encoded']==96,'fixture stopped covering all bridge operand combinations')
    for current in models: require(call([runner,current,inp]).stdout==expected,arch+' table/net encoding differs')
    bad=['hostcall '+regs[0]+', 0','hostcall '+regs[0],
         'hostaddr '+regs[0]+', -1','hostaddr '+regs[0]+', 4',
         'hostaddr '+regs[0]+', 4294967296',
         'hostcall '+regs[0]+', '+('x16' if arch=='arm64' else 'r11')]
    for text in bad: reject(runner,models,inp,'@target osx/'+arch+'\n'+text+'\n')
    for os_ in ('win',): reject(runner,models,inp,'@target '+os_+'/'+arch+'\nhostcall '+regs[0]+', '+regs[1]+'\n')   # 0.0.21: lnx encodes host calls (dynamic ELF); win is refused until 0.0.22
    print(arch+': 96 bridge encodings identical on table/net; 8 invalid cases rejected',flush=True)


def lowering(d,runner,arch,models,oracle):
    raw='_start:\n.hostaddr r4, 0\n.hostcall r4, r5\nimm r2, 8\nadd64 r3, r0, r2\n.hostcall r2, r1\n'
    inp=d/(arch+'.tape');inp.write_text(raw)
    expected=lower(parse_tape(raw),'osx/'+arch,oracle,drive='built')
    for current in models:
        got=parse_tins(call([runner,current,inp]).stdout.decode())
        require([(i.op,i.args,i.meta) for i in got.code]==[(i.op,i.args,i.meta) for i in expected.code],arch+' full lowering differs')
        require(got.labels==expected.labels and got.syms==expected.syms and got.data_len==expected.data_len and got.bss==expected.bss and got.relocs==expected.relocs,arch+' lowering declarations differ')
        require(got.data==expected.data[:len(got.data)] and not any(expected.data[len(got.data):]),arch+' lowering data differs')
    for bad in ('.hostaddr r0, 4','.hostaddr r0, -1','.hostcall r0'): reject(runner,models,inp,bad+'\n',empty=False)
    print(arch+': full table/net lowering identical; fusion barrier and malformed inputs checked',flush=True)


ARM_NATIVE = '.text\n.globl _bridge\n_bridge:\n stp x19,x20,[sp,#-32]!\n str x30,[sp,#16]\n mov x19,x1\n mov x2,#202\n mov x3,#203\n mov x4,#204\n mov x5,#205\n mov x6,#206\n mov x7,sp\n{bridge}\n mov x20,x0\n cmp x1,x19\n b.ne bad\n cmp x2,#202\n b.ne bad\n cmp x3,#203\n b.ne bad\n cmp x4,#204\n b.ne bad\n cmp x5,#205\n b.ne bad\n cmp x6,#206\n b.ne bad\n mov x9,sp\n cmp x7,x9\n b.ne bad\n mov x0,x20\n b done\nbad:\n mov x0,#-1\ndone:\n ldr x30,[sp,#16]\n ldp x19,x20,[sp],#32\n ret\n.globl _foreign\n_foreign:\n mov x9,sp\n tst x9,#15\n b.ne fbad\n mov x9,#10\n madd x0,x1,x9,x0\n mov x9,#100\n madd x0,x2,x9,x0\n mov x9,#1000\n madd x0,x3,x9,x0\n mov x9,#10000\n madd x0,x4,x9,x0\n mov x9,#34464\n movk x9,#1,lsl #16\n madd x0,x5,x9,x0\n mov x1,#255\n mov x2,#255\n mov x3,#255\n mov x4,#255\n mov x5,#255\n mov x6,#255\n mov x7,#255\n mov x9,#255\n mov x10,#255\n mov x16,#255\n mov x17,#255\n ret\nfbad:\n mov x0,#-2\n ret\n'
X86_NATIVE = '.text\n.globl _bridge\n_bridge:\n push %r12\n push %r13\n push %r14\n mov %rdi,%r12\n mov %rsi,%r13\n mov %rsp,%r14\n mov $203,%rdx\n mov $204,%r8\n mov $205,%r10\n mov $206,%r9\n{bridge}\n cmp %r12,%rdi\n jne bad\n cmp %r13,%rsi\n jne bad\n cmp $203,%rdx\n jne bad\n cmp $204,%r8\n jne bad\n cmp $205,%r10\n jne bad\n cmp $206,%r9\n jne bad\n cmp %r14,%rsp\n jne bad\n jmp done\nbad:\n mov $-1,%rax\ndone:\n pop %r14\n pop %r13\n pop %r12\n ret\n.globl _foreign\n_foreign:\n mov %rsp,%rax\n and $15,%rax\n cmp $8,%rax\n jne fbad\n mov %rdi,%rax\n imul $10,%rsi,%r11\n add %r11,%rax\n imul $100,%rdx,%r11\n add %r11,%rax\n imul $1000,%rcx,%r11\n add %r11,%rax\n imul $10000,%r8,%r11\n add %r11,%rax\n imul $100000,%r9,%r11\n add %r11,%rax\n mov $255,%rdi\n mov $255,%rsi\n mov $255,%rdx\n mov $255,%rcx\n mov $255,%r8\n mov $255,%r9\n mov $255,%r10\n mov $255,%r11\n ret\nfbad:\n mov $-2,%rax\n ret\n'


def native_call(d,runner,arch,models):
    args,template=(['x0','x1'],ARM_NATIVE) if arch=='arm64' else (['rdi','rsi'],X86_NATIVE)
    inp=d/(arch+'.native.tins');inp.write_text('@target osx/'+arch+'\nhostcall '+', '.join(args)+'\n')
    code=call([runner,models[1],inp]).stdout
    expected=(encode_arm if arch=='arm64' else encode_x86)(TIns('hostcall',args),0,{})
    require(code==expected,'native bridge must come from the actual network')
    asm,c,exe=d/(arch+'.s'),d/(arch+'.c'),d/(arch+'.native')
    asm.write_text(template.format(bridge='.byte '+','.join(map(str,code))))
    c.write_text('extern long bridge(void*,long*);extern long foreign(long,long,long,long,long,long);\nint main(void){long v[]={1,2,3,4,5,6};return bridge((void*)foreign,v)!=654321;}\n')
    call(['cc','-arch',arch,'-o',exe,c,asm])
    call([exe] if arch=='arm64' else ['/usr/bin/arch','-x86_64',exe])
    print(arch+': actual network bridge calls hostile native ABI callee; args/alignment/live regs/sp preserved',flush=True)


DLOPEN_TAPE=r'''.bss av 48
.str sym "strlen\x00"
.str msg "host-ABI\x00"
_start:
 .lea r5, av
 imm r0, 0
 .st r5, 0, r0, 8
 imm r0, 2
 .st r5, 8, r0, 8
 .hostaddr r4, 0
 .hostcall r4, r5
 .st r5, 0, r0, 8
 .lea r0, sym
 .st r5, 8, r0, 8
 .hostaddr r4, 1
 .hostcall r4, r5
 mov r4, r0
 .lea r0, msg
 .st r5, 0, r0, 8
 .hostcall r4, r5
 .print r0
 imm r0, 0
 .exit r0
'''


def complete(d,ua,oracle):
    tape=d/'dl.tape';tape.write_text(DLOPEN_TAPE)
    for arch in ('arm64','x86_64'):
        exe=d/(arch+'.image');call([ua,tape,'-b','osx/'+arch,'-o',exe])
        tp=lower(parse_tape(DLOPEN_TAPE),'osx/'+arch,oracle,drive='built');text,stats=assemble(tp)
        data=image.relocate(tp,tp.data,stats['data_va']-256)
        expected=image.build(tp,text,data,stats['entry'])
        require(exe.read_bytes()==expected,arch+' complete C/Python image differs')
        exe.chmod(0o755)
        result=call([exe] if arch=='arm64' else ['/usr/bin/arch','-x86_64',exe])
        require(result.stdout==b'8',arch+' actual dyld/strlen output differs')
        print(arch+': C/Python image identical; actual dyld dlopen/dlsym/strlen PASS '+hashlib.sha256(expected).hexdigest()[:12],flush=True)
    c=d/'direct.c';c.write_text('int main(void){long a[6]={0,2,0,0,0,0};long h=__hostcall(__hostaddr0(),a);a[0]=h;a[1]=(long)"strlen";long f=__hostcall(__hostaddr1(),a);a[0]=(long)"host-ABI";return __hostcall(f,a)!=8;}\n')
    call([ua,'-run',c]);print('classic in-memory -run fixed loader slots PASS',flush=True)


def main():
    global DEADLINE
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--ua',default=os.environ.get('UA'),help='existing frozen reference compiler; never rebuilt')
    args=ap.parse_args();require(args.ua,'pass --ua or UA pointing to the frozen reference compiler')
    ua=Path(args.ua).resolve();ua.stat();require(ua.is_file(),'UA must be a file')
    require(sys.platform=='darwin' and platform.machine()=='arm64','both native checks require macOS arm64 plus Rosetta; unavailable targets are not a pass')
    DEADLINE=time.monotonic()+53
    with tempfile.TemporaryDirectory(prefix='unisacc-bridgecheck-') as tmp:
        d=Path(tmp);runner=d/'run';call(['cc','-O2',ROOT/'exec/c/run.c','-o',runner])
        oracle=_oracle('built')
        for arch,tag in [('arm64','arm'),('x86_64','x86')]:
            enc=model(d,'enc-'+tag,'exec/enc/'+('arm.py' if arch=='arm64' else 'gen.py'))
            low=model(d,'lower-'+tag,'exec/lower/gen.py',['--full','--osx']+(['--arm64'] if arch=='arm64' else []))
            encodings(d,runner,arch,enc);lowering(d,runner,arch,low,oracle);native_call(d,runner,arch,enc)
        complete(d,ua,oracle)
    print(f'bridgecheck PASS: both ISA table/net and actual native calls; {time.monotonic()-(DEADLINE-53):.2f}s',flush=True)


if __name__=='__main__':
    try: main()
    except (RuntimeError,OSError,ValueError) as exc:
        print('bridgecheck: '+str(exc),file=sys.stderr);sys.exit(1)
