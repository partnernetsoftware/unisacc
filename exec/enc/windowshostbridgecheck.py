#!/usr/bin/env python3
"""Independent Win64 declaration assembly and actual six-GP MS ABI probe.
On macOS x86_64/Rosetta executes MS ABI calls; not Windows SEH qualification.
"""
import pathlib, subprocess, tempfile, platform, sys
ROOT=pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT))
from unisa.hostabi import WIN_X86_BODY,WIN_ARM_BODY,ARM_BODY
ASM = r""".text
.globl bridge
bridge:
 pushq %rbx
 movq %rax,%rbx
 movq %rsp,%rax
 subq $128,%rsp
 andq $-16,%rsp
 movq %rdi,48(%rsp)
 movq %rsi,56(%rsp)
 movq %rdx,64(%rsp)
 movq %rcx,72(%rsp)
 movq %r8,80(%rsp)
 movq %r9,88(%rsp)
 movq %rax,96(%rsp)
 movq 32(%rbx),%rax
 movq %rax,32(%rsp)
 movq 40(%rbx),%rax
 movq %rax,40(%rsp)
 movq 0(%rbx),%rcx
 movq 8(%rbx),%rdx
 movq 16(%rbx),%r8
 movq 24(%rbx),%r9
 callq *%r11
 movq 48(%rsp),%rdi
 movq 56(%rsp),%rsi
 movq 64(%rsp),%rdx
 movq 72(%rsp),%rcx
 movq 80(%rsp),%r8
 movq 88(%rsp),%r9
 movq 96(%rsp),%rsp
 popq %rbx
"""
def command(args):
 p=subprocess.run(list(map(str,args)),capture_output=True,timeout=20)
 assert p.returncode==0,(args,p.returncode,p.stdout,p.stderr)
 return p.stdout

def main():
 llvm=pathlib.Path('/opt/homebrew/opt/llvm/bin')
 clang=llvm/'clang';objcopy=llvm/'llvm-objcopy'
 with tempfile.TemporaryDirectory(prefix='r10-winhost-template-') as name:
  d=pathlib.Path(name);src=d/'bridge.s';src.write_text(ASM)
  command([clang,'-target','x86_64-pc-windows-msvc','-c',src,'-o',d/'bridge.obj'])
  command([objcopy,'--dump-section=.text='+str(d/'bridge.bin'),d/'bridge.obj',d/'bridge-copy.obj'])
  assert (d/'bridge.bin').read_bytes()==WIN_X86_BODY,'declaration differs from independent assembly'
  assert WIN_ARM_BODY==ARM_BODY, 'ARM fixed GP body drift'
  # Every ARM declaration word has a register Rt/Rd in bits0..4; no x18
  # appears there or as memory base Rn. No PC-relative instruction uses x18.
  assert all((w&31)!=18 and ((w>>5)&31)!=18 for w in WIN_ARM_BODY), 'ARM platform register clobber'
  if sys.platform=='darwin':
   # Native C callback consumes RCX/RDX/R8/R9 and stack parameters5/6.
   c=d/'probe.c';c.write_text(r"""#include <stdint.h>
#include <stdio.h>
extern void probe(void*,uint64_t*,uint64_t*);
__attribute__((ms_abi,noinline)) uint64_t host(uint64_t a,uint64_t b,uint64_t c,uint64_t d,uint64_t e,uint64_t f) {
 uint64_t r=a+2*b+3*c+4*d+5*e+6*f;
 __asm__ volatile("mov $999,%%rcx;mov $888,%%rdx;mov $777,%%r8;mov $666,%%r9;mov $555,%%r10;mov $444,%%r11":::"rcx","rdx","r8","r9","r10","r11");return r;
}
int main(void){uint64_t v[8]={0,1,UINT32_MAX,UINT64_C(0x100000000),INT64_MAX,UINT64_C(0x8000000000000000),UINT64_MAX,UINT64_C(0x123456789abcdef0)};for(int i=0;i<64;i++){uint64_t a[6],o[9]={0},want=0;for(int j=0;j<6;j++){a[j]=v[(i+j)%8]+i;want+=(j+1)*a[j];}probe((void*)host,a,o);
 if(o[0]!=want||o[1]!=17||o[2]!=34||o[3]!=51||o[4]!=68||o[5]!=85||o[6]!=102||o[7]!=119||o[8]!=1)return 1;}puts("64 six-GP MS-ABI register/stack calls and restores: ok");return 0;}
""")
   a=d/'probe.s';hexline='.byte '+','.join(str(v) for v in WIN_X86_BODY)
   a.write_text('.text\n.globl _probe\n_probe:\npushq %rbx\npushq %r12\npushq %r13\nmovq %rdx,%r12\nmovq %rsp,%r13\nmovq %rdi,%r11\nmovq %rsi,%rax\n'+''.join('movq $'+str(n)+',%'+reg+'\n' for reg,n in [('rdi',17),('rsi',34),('rdx',51),('rcx',68),('r8',85),('r9',102),('rbx',119)])+hexline+'\n'+''.join('movq %'+reg+','+str(8*i)+'(%r12)\n' for i,reg in enumerate(['rax','rdi','rsi','rdx','rcx','r8','r9','rbx']))+'cmpq %r13,%rsp\nsete %al\nmovzbq %al,%rax\nmovq %rax,64(%r12)\npopq %r13\npopq %r12\npopq %rbx\nretq\n')
   command([clang,'-arch','x86_64','-O2',c,a,'-o',d/'probe'])
   print(command([d/'probe']).decode().strip())
   restore=bytes.fromhex('488b4c2448');assert WIN_X86_BODY.count(restore)==1
   bad=WIN_X86_BODY.replace(restore,b'\x90'*5)
   badline='.byte '+','.join(str(v) for v in bad)
   a.write_text(a.read_text().replace(hexline,badline))
   command([clang,'-arch','x86_64','-O2',c,a,'-o',d/'bad-probe'])
   control=subprocess.run([d/'bad-probe'],capture_output=True,timeout=20)
   assert control.returncode==1, ('missing rcx restore not detected',control.returncode)
   print('missing rcx restore mutant rejected: ok')
  else:print('COFF byte declaration verified; native MS ABI probe not run on this host')
if __name__=='__main__':main()
