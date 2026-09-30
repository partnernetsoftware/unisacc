/* C99 6.10.5/6.10.6/6.10.7: #error in a group that is not taken is skipped; an unknown #pragma is ignored; a null directive (a lone #) has no effect. */
#include <stdio.h>
#if 0
#error this group is skipped
#endif
#pragma unisacc_unknown_pragma 1
#
int main(void){ printf("ok\n"); return 0; }
