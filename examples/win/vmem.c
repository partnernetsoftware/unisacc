/* examples/win/vmem.c -- memory a shell can hand to a JIT.
 *
 * Allocate at a chosen address, write bytes, change protection, free: all
 * four are kernel32 forwards, and no host libc is involved in this file. The
 * old protection comes back through an out-parameter, which is why a caller
 * must pass a real pointer rather than 0.
 *
 * SYSTEM_INFO is read through a correctly sized structure (48 bytes). See the
 * note in clockfmt.c: the kernel writes the real structure, and a short
 * declaration compiles and then corrupts the frame.
 *
 * Build and run:
 *   out/ua-ref-win.exe -b win/x86_64 -o vmem.exe examples/win/vmem.c
 *   ./vmem.exe
 */
#include <stdio.h>

#define MEM_COMMIT 4096UL
#define MEM_RESERVE 8192UL
#define MEM_RELEASE 32768UL
#define PAGE_READWRITE 4UL
#define PAGE_NOACCESS 1UL

void *VirtualAlloc(void *addr, unsigned long size, unsigned long type, unsigned long prot);
int VirtualProtect(void *addr, unsigned long size, unsigned long prot, unsigned long *old);
int VirtualFree(void *addr, unsigned long size, unsigned long type);
void GetSystemInfo(void *info);
unsigned long GetLastError(void);

/* SYSTEM_INFO as it is laid out on a win64 target, 48 bytes, no padding
   between the members. Two traps live here. There is NO dwOemId on win64 --
   that member belongs to the 32-bit layout, and a struct copied from an x86
   header shifts every field by four bytes. And a Win32 DWORD is 4 bytes while
   unsigned long is 8 on this target, so the DWORDs must be unsigned int and
   only the address-shaped members may be unsigned long. Get either wrong and
   the struct still compiles; the numbers just come from the wrong offsets.
   examples/win/structs.c dumps the kernel's own bytes for the check. */
struct sys_info {
    unsigned short arch;
    unsigned short reserved;
    unsigned int page_size;
    unsigned long min_address;
    unsigned long max_address;
    unsigned long active_mask;
    unsigned int processors;
    unsigned int proc_type;
    unsigned int alloc_granularity;
    unsigned short proc_level;
    unsigned short proc_revision;
};

int main(void)
{
    struct sys_info info;
    unsigned char *page;
    unsigned long old;
    unsigned long i;
    unsigned int sum;

    GetSystemInfo(&info);
    printf("arch %u page %lu procs %lu granularity %lu\n",
           (unsigned int)info.arch, (unsigned long)info.page_size,
           (unsigned long)info.processors, (unsigned long)info.alloc_granularity);

    page = (unsigned char *)VirtualAlloc(0, 4096,
                                         MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (page == 0) {
        printf("alloc failed %lu\n", GetLastError());
        return 1;
    }
    for (i = 0; i < 4096; i = i + 1) {
        page[i] = (unsigned char)(i * 7);
    }
    sum = 0;
    for (i = 0; i < 4096; i = i + 64) {
        sum = sum + (unsigned int)page[i];
    }
    printf("aligned %d sum %u\n",
           ((unsigned long)page % (unsigned long)info.page_size) == 0, sum);

    if (VirtualProtect(page, 4096, PAGE_NOACCESS, &old)) {
        printf("was %lu restored %d\n", old, VirtualProtect(page, 4096, old, &old));
    } else {
        printf("protect failed %lu\n", GetLastError());
    }
    printf("free %d\n", VirtualFree(page, 0, MEM_RELEASE));
    return 0;
}