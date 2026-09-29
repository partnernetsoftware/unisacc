/* Host-only API contract double; not Windows execution evidence. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct { unsigned char *b;int n; } Buf;
static int mode=0,count=0;
static void die(const char *s) { fprintf(stderr,"%s\n",s);exit(2); }
/* Doubles for the Win32 calls the host loader makes; no <windows.h> on this host. */
typedef unsigned long DWORD; typedef size_t SIZE_T; typedef void *HANDLE; typedef int BOOL;
#define MEM_RESERVE 0x2000
#define MEM_COMMIT 0x1000
#define PAGE_NOACCESS 1
#define PAGE_READWRITE 4
#define PAGE_EXECUTE_READ 0x20
static void *VirtualAlloc(void *a,SIZE_T n,DWORD flags,DWORD prot) {
    count++;
    if(count==1){ if(a || n!=2147467264 || flags!=MEM_RESERVE || prot!=PAGE_NOACCESS)die("bad reserve contract");return mode==1 ? 0 : (void *)0x10000000000; }
    if(a!=(void *)0x10000000000 || n!=32768 || flags!=MEM_COMMIT || prot!=PAGE_READWRITE)die("bad commit contract");
    return mode==2 ? 0 : mode==3 ? (unsigned char *)a+65536 : a;
}
static BOOL VirtualProtect(void *a,SIZE_T n,DWORD prot,DWORD *old) { (void)a;(void)n;(void)prot;*old=0;return 1; }
static HANDLE GetCurrentProcess(void) { return (HANDLE)1; }
static BOOL FlushInstructionCache(HANDLE h,const void *a,SIZE_T n) { (void)h;(void)a;(void)n;return 1; }
#define UNISA_MEMORY_CONTRACT_DOUBLE
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
