/* 0.0.28 F1: constructors run before main in definition order, destructors at exit (cc: main 12, bye 12, exit 3) */
#include <stdio.h>
static int seen;
__attribute__((constructor)) static void boot(void) { seen = seen * 10 + 1; }
static void __attribute__((constructor)) boot2(void) { seen = seen * 10 + 2; }
__attribute__((destructor)) static void bye(void) { printf("bye %d\n", seen); }
int main(void) { printf("main %d\n", seen); return seen == 12 ? 3 : 1; }
