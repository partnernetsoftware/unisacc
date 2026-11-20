#include <stdio.h>
/* musl idiom: weak_alias(__impl, pub) */
int __impl(void){return 42;}
extern __typeof(__impl) pub __attribute__((weak,alias("__impl")));
int main(void){printf("%d\n",pub());return 0;}
