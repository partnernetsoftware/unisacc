/* Host-only API contract double; not Windows execution evidence. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct { unsigned char *b;int n; } Buf;
static int mode=0,count=0;
static void die(const char *s) { fprintf(stderr,"%s\n",s);exit(2); }
static long __mmap(long a,long n,long flags,long prot,long fd,long off) {
    count++;
    if(count==1){ if(a || n!=2147467264 || flags!=0x2000 || prot!=1)die("bad reserve contract");return mode==1 ? 0 : 0x10000000000; }
    if(a!=0x10000000000 || n!=32768 || flags!=0x1000 || prot!=4)die("bad commit contract");
    return mode==2 ? 0 : mode==3 ? a+65536 : a;
}
static int __mprotect(long a,long n,long p) { return 0; }
#define _WIN32
#include "memory.c"
int main(int n,char **v) {
 mode=n>1?atoi(v[1]):0;
 MemoryMap x;MemoryImage m={1,1,0,0};
 memory_reserve(&x);
 if(mode==4)m.extent=2147483647;
 memory_commit(&m,&x);
 if(count!=2 || x.base!=(unsigned char *)0x10000000000 || x.size!=32768)return 7;
 puts("reserve and exact-address commit");return 0;
}
