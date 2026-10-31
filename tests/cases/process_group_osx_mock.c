/* Host simulation of Darwin syscall arguments and result propagation. */
#include <errno.h>
#include <stdio.h>
#ifndef MOCK_NO_APPLE
#define __APPLE__ 1
#endif
#ifdef MOCK_ARM
#undef __x86_64__
#define __aarch64__ 1
#endif
static long mock_result, number, args[5];
static long __syscall6(long n,long a,long b,long c,long d,long e) {
    number=n;args[0]=a;args[1]=b;args[2]=c;args[3]=d;args[4]=e;return mock_result;
}
#include "../../cx/sys/process_group_osx.cx"
int main(void) {
#ifdef MOCK_ARM
    const long want=82;
#else
    const long want=0x2000052L;
#endif
    errno=17;mock_result=0;
    if(setpgid(123,456)!=0||number!=want||args[0]!=123||args[1]!=456||args[2]||args[3]||args[4]||errno!=17)return 1;
    mock_result=-22;if(setpgid(5,6)!=-1||errno!=22||number!=want||args[0]!=5||args[1]!=6)return 1;
    puts("PROCESS_GROUP_OSX_MOCK ok");return 0;
}
