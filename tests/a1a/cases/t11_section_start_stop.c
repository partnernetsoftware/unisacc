#include <stdio.h>
/* C-identifier section names get __start_/__stop_ symbols from ld */
__attribute__((section("regs"),used)) static const int r1=10;
__attribute__((section("regs"),used)) static const int r2=20;
extern const int __start_regs[],__stop_regs[];
int main(void){const int*p;int s=0;for(p=__start_regs;p<__stop_regs;p++)s+=*p;printf("%d\n",s);return 0;}
