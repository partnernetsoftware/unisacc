#include <stdio.h>
__attribute__((constructor(200))) static void c200(void){printf("c200\n");}
__attribute__((constructor(101))) static void c101(void){printf("c101\n");}
__attribute__((constructor)) static void cdef(void){printf("cdef\n");}
__attribute__((destructor(101))) static void d101(void){printf("d101\n");}
__attribute__((destructor(200))) static void d200(void){printf("d200\n");}
int main(void){printf("main\n");return 0;}
