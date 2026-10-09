#!/usr/bin/env python3
"""ALL_STACK x86 raw-slot bridge check; macOS Rosetta and Win64-ABI adaptation.
Windows adaptation tests calling convention, not PE execution or guest SEH.
Each process is bounded; all inputs and outputs are private snapshots.
"""
import pathlib
import subprocess
import tempfile
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]

def run(args):
    subprocess.run([sys.executable, str(ROOT/'tests/bound.py'), '20', *map(str,args)], check=True)


def main():
    from hostcheck import require_host
    rc=require_host('darwin-x86_64','osx/x86_64')
    if rc: raise SystemExit(rc)
    with tempfile.TemporaryDirectory(prefix='r10stackx86-') as directory:
        p = pathlib.Path(directory)
        sysv = (ROOT/'exec/c/librarycall_x86_64.S').read_text()
        windows = (ROOT/'exec/c/librarycall_windows_x86_64.S').read_text()
        # Same Win64 instructions; Mach-O lacks COFF SEH/definition directives.
        windows = '\n'.join(line for line in windows.splitlines()
                if not line.strip().startswith(('.seh_', '.def ')))
        windows = windows.replace('us_library_bridge_raw', '_us_library_bridge_raw')
        windows = windows.replace('us_library_bridge_stack_raw', '_us_library_bridge_stack_raw')
        counts = [0,1,6,7,9,17,1024]
        guests = ['.text']
        for n in counts:
            guests += [f'.globl _guest{n}', f'_guest{n}:',
                'pushq %r9', 'movq %rsp,%r9', 'subq $32,%rsp',
                'xorq %r11,%r11']
            for k in range(n):
                guests += [f'movq {16+8*k}(%r9),%rdx',
                    f'imulq ${k+1},%rdx', 'addq %rdx,%r11']
            guests += ['movq %r11,%rax']
            # Deliberately clobber all native nonvolatiles available to scripts.
            for reg in ['rbp','rbx','rdi','rsi','r12','r13','r14','r15']:
                guests += [f'movq $123,%{reg}']
            for reg in range(6,16): guests += [f'pxor %xmm{reg},%xmm{reg}']
            guests += ['movq %r9,%rsp', 'popq %r9', 'retq']
        # A SysV harness enters the Win64 bridge with live native GPR/XMM
        # values. The guest above deliberately destroys every one of them.
        probe=['.globl _win_nonvolatile_probe','_win_nonvolatile_probe:']
        saved=['rbp','rbx','r12','r13','r14','r15']
        probe += [f'pushq %{r}' for r in saved]
        probe += ['subq $72,%rsp','movq %rdi,32(%rsp)',
                  'movq %rsi,40(%rsp)','movq %rdx,48(%rsp)','movq %rcx,56(%rsp)']
        for r in saved+['rdi','rsi']: probe += [f'movq $456,%{r}']
        for r in range(6,16): probe += [f'pcmpeqd %xmm{r},%xmm{r}']
        probe += ['movq 32(%rsp),%rcx','movq 40(%rsp),%rdx',
                  'movq 48(%rsp),%r8','movq 56(%rsp),%r9',
                  'callq _us_library_bridge_stack_raw']
        for r in saved+['rdi','rsi']: probe += [f'cmpq $456,%{r}','jne .Lprobe_fail']
        for r in range(6,16): probe += [f'pmovmskb %xmm{r},%eax','cmpl $65535,%eax','jne .Lprobe_fail']
        probe += ['xorq %rax,%rax','jmp .Lprobe_done','.Lprobe_fail:',
                  'movq $1,%rax','.Lprobe_done:','addq $72,%rsp']
        probe += [f'popq %{r}' for r in reversed(saved)]
        probe += ['retq']
        # Keep this helper out of the SysV executable; its bridge call is ms_abi.
        (p/'probe.S').write_text('.text\n'+'\n'.join(probe)+'\n')
        (p/'guest.S').write_text('\n'.join(guests)+'\n')
        declarations='\n'.join(f'extern void guest{n}(void);' for n in counts)
        cases='\n'.join(f'check(guest{n},{n});' for n in counts)
        template='''#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
BRIDGE_DECL
DECLARATIONS
static void check(void (*entry)(void),unsigned n) {
    uint64_t slots[1024], want=0;
    unsigned char stack[32768] __attribute__((aligned(16)));
    for(unsigned i=0;i<1024;i++) slots[i]=UINT64_C(0xfedcba9876000000)+17*i;
    for(unsigned i=0;i<n;i++) want+=slots[i]*(i+1);
    uint64_t got=us_library_bridge_stack_raw(entry,n?slots:0,stack+sizeof stack,n);
    if(got!=want) {fprintf(stderr,"count %u got %llx expected %llx\\n",n,(unsigned long long)got,(unsigned long long)want);exit(1);}
}
int main(void) { CASES puts("stack bridge: 7 counts, raw bits and native clobbers: ok");return 0; }
'''
        for name,asm,attribute in [('sysv',sysv,''),('windows',windows,'__attribute__((ms_abi))')]:
            (p/f'{name}.S').write_text(asm)
            decl=f'extern uint64_t {attribute} us_library_bridge_stack_raw(const void *,const uint64_t *,void *,uint64_t);'
            extra=[]
            if name=='windows':
                decl+='\nextern uint64_t win_nonvolatile_probe(const void *,const uint64_t *,void *,uint64_t);'
                windows_case='''{ uint64_t slots[9]={0}; unsigned char stack[32768] __attribute__((aligned(16)));
                if(win_nonvolatile_probe(guest9,slots,stack+sizeof stack,9)) return 2; }'''
                used_cases=cases+windows_case
                extra=[p/'probe.S']
            else: used_cases=cases
            (p/f'{name}.c').write_text(template.replace('BRIDGE_DECL',decl).replace('DECLARATIONS',declarations).replace('CASES',used_cases))
            run(['cc','-arch','x86_64','-O2','-Wall','-Wextra','-Werror',p/f'{name}.c',p/f'{name}.S',p/'guest.S',*extra,'-o',p/name])
            run(['/usr/bin/arch','-x86_64',p/name])
        # Compile the unmodified genuine Windows object too (SEH metadata included).
        (p/'windows-original.S').write_text((ROOT/'exec/c/librarycall_windows_x86_64.S').read_text())
        run(['clang','-target','x86_64-pc-windows-msvc','-c',p/'windows-original.S','-o',p/'windows.obj'])

if __name__=='__main__': main()
