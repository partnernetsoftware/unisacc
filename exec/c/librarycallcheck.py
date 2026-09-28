#!/usr/bin/env python3
"""True unisacc machine bytes through a host/script ABI bridge, private snapshot.
Six integer/pointer parameters only. No raw script address is called natively.
"""
import os,pathlib,signal,subprocess,sys,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT))
from unisa.tape import parse
from unisa.lower import lower
from unisa.__main__ import _oracle
from unisa.assemble import assemble,BACKEND
from unisa.image.macho import HDRS
SOURCE='''long sum6(long a,long b,long c,long d,long e,long f){long v[3];v[0]=a+b;v[1]=c+d;v[2]=e+f;return v[0]+v[1]*3+v[2]*7;}
long recur(long n){if(n==0)return 5;return recur(n-1)+n;}
long leaf(long x){return x*3+1;}
long outer(long x){long v[5];long i;for(i=0;i<5;i++)v[i]=x+i;return leaf(v[4])+recur(4);}
long touch(long *p,long d){*p=*p+d;return *p;}
long *identity(long *p){return p;}
int main(void){long n=1;return sum6(1,2,3,4,5,6)+outer(3)+touch(identity(&n),2);}
'''
def command(args):
    p=subprocess.Popen(list(map(str,args)),cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.PIPE,start_new_session=True)
    try:out,err=p.communicate(timeout=25)
    except subprocess.TimeoutExpired:
        os.killpg(p.pid,signal.SIGKILL);p.communicate();raise
    assert p.returncode==0,(args,p.returncode,err)
    return out

def labels(tp):
    be=BACKEND[tp.arch];sizes=[be.size(i,tp.labels) for i in tp.code];short=set()
    while True:
        offsets=[];end=0
        for size in sizes:offsets.append(end);end+=size
        names={k:offsets[i] if i<len(offsets) else end for k,i in tp.labels.items()}
        fits=[(i,be.short_size(ins)) for i,ins in enumerate(tp.code)
              if i not in short and be.short_size(ins) is not None and
              -128<=names[be.branch_target(ins)]-(offsets[i]+be.short_size(ins))<=127]
        if not fits:return names
        for i,n in fits:short.add(i);sizes[i]=n

def check(ua,arch):
    with tempfile.TemporaryDirectory(prefix='unisacc-librarycall-') as name:
        d=pathlib.Path(name);source=d/'p.c';source.write_text(SOURCE);image=d/'image'
        target='osx/'+arch
        tape=command(["/bin/sh",ua,'-nostdinc','-b',target,'-O0','-S',source])
        command(["/bin/sh",ua,'-nostdinc','-b',target,'-O0','-o',image,source])
        tp=lower(parse(tape.decode()),target,_oracle('built'),drive='built',prune_input=True)
        ref,stats=assemble(tp);assert stats['encoded']==stats['insns']
        machine=image.read_bytes()[HDRS(arch):HDRS(arch)+len(ref)]
        if machine!=ref:
            pathlib.Path('/tmp/r10-bridge-actual').write_bytes(image.read_bytes())
            pathlib.Path('/tmp/r10-bridge-ref').write_bytes(ref)
            pathlib.Path('/tmp/r10-bridge-tape').write_bytes(tape)
        assert machine==ref,'actual unisacc image differs from independently resolved reference text'
        names=labels(tp);blob=d/'script.bin';blob.write_bytes(machine)
        (d/'blob.S').write_text('.text\n.p2align 4\n.globl _script_blob\n_script_blob:\n.incbin "'+str(blob)+'"\n')
        probe=['.text','.p2align 4','.globl _bridge_clobber','_bridge_clobber:']
        if arch=='arm64':
            probe += ['mov x%d, #49'%r for r in range(19,30)]
            probe += ['fmov d%d, xzr'%r for r in range(8,16)]
            probe += ['mov x0, #42','ldr x17, [x7]','add x7, x7, #8','ret x17']
        else:
            probe += ['movq $49, %'+r for r in ('rbx','rbp','r12','r13','r14','r15')]
            probe += ['movq $42, %rax','retq']
        probe += ['.p2align 4','.globl _bridge_verify','_bridge_verify:']
        if arch=='arm64':
            probe += ['sub sp, sp, #224']
            for i in range(6):probe += ['stp x%d, x%d, [sp, #%d]'%(19+2*i,20+2*i,16*i)]
            for i in range(4):probe += ['stp d%d, d%d, [sp, #%d]'%(8+2*i,9+2*i,96+16*i)]
            probe += ['mov x2, x0','adrp x0, _bridge_clobber@PAGE','add x0, x0, _bridge_clobber@PAGEOFF',
                      'add x1, sp, #160','mov x9, sp','str x9, [sp, #208]']
            for i in range(3):probe += ['stp xzr, xzr, [x1, #%d]'%(16*i)]
            probe += ['mov x%d, #49'%r for r in range(19,30)] + ['mov x9, #49']
            probe += ['fmov d%d, x9'%r for r in range(8,16)]
            probe += ['bl _us_library_bridge_raw','cmp x0, #42','b.ne Lverify_bad','mov x10, sp','ldr x9, [sp, #208]','cmp x9, x10','b.ne Lverify_bad']
            for r in range(19,30):probe += ['cmp x%d, #49'%r,'b.ne Lverify_bad']
            for r in range(8,16):probe += ['fmov x9, d%d'%r,'cmp x9, #49','b.ne Lverify_bad']
            probe += ['mov x0, #0','b Lverify_restore','Lverify_bad:','mov x0, #1','Lverify_restore:']
            for i in range(4):probe += ['ldp d%d, d%d, [sp, #%d]'%(8+2*i,9+2*i,96+16*i)]
            for i in range(6):probe += ['ldp x%d, x%d, [sp, #%d]'%(19+2*i,20+2*i,16*i)]
            probe += ['add sp, sp, #224','ret']
        else:
            regs=('rbp','rbx','r12','r13','r14','r15')
            probe += ['pushq %'+r for r in regs] + ['subq $56, %rsp','movq %rdi, %rdx',
                     'leaq _bridge_clobber(%rip), %rdi','movq %rsp, %rsi','movq %rsp, 48(%rsp)']
            probe += ['movq $0, %d(%%rsp)'%(8*i) for i in range(6)]
            probe += ['movq $49, %'+r for r in regs]
            probe += ['callq _us_library_bridge_raw','cmpq $42, %rax','jne Lverify_bad','cmpq %rsp, 48(%rsp)','jne Lverify_bad']
            for r in regs:probe += ['cmpq $49, %'+r,'jne Lverify_bad']
            probe += ['xorq %rax, %rax','jmp Lverify_restore','Lverify_bad:','movq $1, %rax','Lverify_restore:', 'addq $56, %rsp']
            probe += ['popq %'+r for r in reversed(regs)] + ['retq']
        (d/'probe.S').write_text('\n'.join(probe)+'\n')
        offsets='\n'.join('#define OFF_'+n+' '+str(names[n]) for n in ('sum6','recur','outer','touch','identity'))
        (d/'h.c').write_text('#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n#include "librarycall.h"\nextern unsigned char script_blob[],bridge_clobber[];\nextern uint64_t bridge_verify(void *);\n'+offsets+'''
static uint64_t invoke(unsigned off,uint64_t *a,void *top,unsigned n){
    us_library_signature s={n,US_LIBRARY_INTEGER,{1,1,1,1,1,1},0};uint64_t r;
    if(!us_library_call(script_blob+off,a,top,&s,&r))abort();return r;
}
#if defined(__aarch64__)
#define READSP(v) __asm__ volatile("mov %0, sp":"=r"(v))
#else
#define READSP(v) __asm__ volatile("movq %%rsp, %0":"=r"(v))
#endif
int main(void){
    unsigned char *stack=malloc(65536+16);if(!stack)return 2;
    void *top=(void *)(((uintptr_t)stack+65536)&~(uintptr_t)15);
    uint64_t args[6]={1,2,3,4,5,6};unsigned i;
    for(i=0;i<100;i++){
        uintptr_t before,after;READSP(before);
        if(invoke(OFF_sum6,args,top,6)!=101)return 3;
        args[0]=10;if(invoke(OFF_recur,args,top,1)!=60)return 4;
        args[0]=3;if(invoke(OFF_outer,args,top,1)!=37)return 5;
        long p=7;args[0]=(uintptr_t)&p;args[1]=11;
        if(invoke(OFF_touch,args,top,2)!=18 || p!=18)return 6;
        us_library_signature ps={1,US_LIBRARY_POINTER,{US_LIBRARY_POINTER,1,1,1,1,1},0};uint64_t pointer;
        if(!us_library_call(script_blob+OFF_identity,args,top,&ps,&pointer) || pointer!=(uintptr_t)&p)return 12;
        READSP(after);if(before!=after || (after&15))return 13;
        args[0]=1;args[1]=2;
    }
    if(bridge_verify(top)!=0)return 15;
    us_library_signature cs={0,1,{1,1,1,1,1,1},0};uint64_t clobbered;
    if(!us_library_call(bridge_clobber,args,top,&cs,&clobbered) || clobbered!=42)return 14;
    us_library_signature s={7,1,{1,1,1,1,1,1},0};uint64_t out;
    if(us_library_call(script_blob,args,top,&s,&out))return 7;
    s.argument_count=1;s.argument_kind[0]=3;if(us_library_call(script_blob,args,top,&s,&out))return 8;
    s.argument_kind[0]=1;s.result_kind=3;if(us_library_call(script_blob,args,top,&s,&out))return 9;
    s.result_kind=1;s.variadic=1;if(us_library_call(script_blob,args,top,&s,&out))return 10;
    s.variadic=0;if(us_library_call(script_blob,args,(char*)top-1,&s,&out))return 11;
    free(stack);puts("six args / recursion / local stack / pointer return-mutation / 100 repeat / callee-save+SP / signature guards: ok");return 0;
}
''')
        command(['cc','-arch',arch,'-O2','-Wall','-Wextra','-I',ROOT/'exec/c',d/'h.c',d/'blob.S',d/'probe.S',ROOT/'exec/c'/('librarycall_'+arch+'.S'),'-o',d/'h'])
        out=command([d/'h']);print(arch,out.decode().strip(),'; actual unisacc image bytes = reference')
if __name__=='__main__':
    if len(sys.argv)!=3:sys.exit('usage: librarycallcheck.py PRIVATE_UNISACC ARCH')
    check(pathlib.Path(sys.argv[1]).resolve(),sys.argv[2])
