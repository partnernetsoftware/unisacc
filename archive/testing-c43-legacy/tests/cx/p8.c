/* p8.c -- CX2a P8: a .c unit is never scanned; this pragma is ignored as any unknown pragma is */
#pragma unisa use nosuch
#include <stdio.h>
int main(void) { printf("c ok\n"); return 0; }
