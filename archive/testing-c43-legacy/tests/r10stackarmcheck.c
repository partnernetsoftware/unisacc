/* Mechanical ALL_STACK ABI test. These are synthetic script entries, not
   evidence that a source-language signature is classified correctly. */
#include <stdint.h>
#include <stdio.h>
#include "exec/c/librarycall.h"
#if !defined(__aarch64__) || !defined(__APPLE__)
#error "this normal-return execution probe runs on macOS ARM64"
#endif
extern void r10_stack_sum(void);
extern void r10_stack_zero(void);
__asm__(
".text\n.p2align 2\n_r10_stack_sum:\n"
"sub x7,x7,#8\nstr x6,[x7]\nmov x6,x7\n"
"ldr x9,[x6,#16]\nadd x10,x6,#24\nmov x0,#0\n"
"cbz x9,1f\n0: ldr x11,[x10],#8\nadd x0,x0,x11\n"
"subs x9,x9,#1\nb.ne 0b\n1: mov x19,#99\n"
"fmov d8,x19\nmov x7,x6\nldr x6,[x7]\nadd x7,x7,#8\n"
"ldr x17,[x7]\nadd x7,x7,#8\nbr x17\n"
".p2align 2\n_r10_stack_zero:\nmov x0,#77\n"
"ldr x17,[x7]\nadd x7,x7,#8\nbr x17\n");
int main(void) {
    _Alignas(16) uint64_t stack[4096];
    uint64_t slots[1024], result, expected=0;
    unsigned i;
    if (!us_library_call_stack(r10_stack_zero,0,stack+4096,sizeof stack,0,&result) || result!=77) return 1;
    slots[0]=8;
    for (i=1;i<9;i++) { slots[i]=UINT64_C(0x100000000)+i; expected+=slots[i]; }
    if (!us_library_call_stack(r10_stack_sum,slots,stack+4096,sizeof stack,9,&result) || result!=expected) return 2;
    slots[0]=1023; expected=0;
    for (i=1;i<1024;i++) { slots[i]=UINT64_C(0x8000000000000000)+i; expected+=slots[i]; }
    if (!us_library_call_stack(r10_stack_sum,slots,stack+4096,sizeof stack,1024,&result) || result!=expected) return 3;
    if (us_library_call_stack(r10_stack_sum,slots,stack+4096,sizeof stack,1025,&result)) return 4;
    if (us_library_call_stack(r10_stack_sum,0,stack+4096,sizeof stack,1,&result)) return 5;
    if (us_library_call_stack(r10_stack_sum,slots,stack+4096,8,1,&result)) return 6;
    puts("r10 ALL_STACK ARM64: zero/9/1024 raw-word calls and rejected bounds ok");
    return 0;
}
