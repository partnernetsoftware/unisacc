/* examples/win/structs.c -- read the kernel's structures at explicit offsets.
 *
 * There is no Win32 SDK here, so every structure a Windows program needs is
 * declared by hand -- and a hand-declared structure that is wrong still
 * compiles, still runs, and returns plausible numbers from the wrong bytes.
 * This probe is the defence: it asks the kernel for four structures, prints
 * the bytes the kernel actually wrote, and then decodes the fields from
 * explicit offsets. Every other probe in this directory is written against
 * these offsets.
 *
 * The two traps, both of which this file was written after hitting:
 *   - a Win32 DWORD is 4 bytes; unsigned long is 8 on a win64 target;
 *   - SYSTEM_INFO has no dwOemId on win64 -- that member is 32-bit only, and
 *     a struct copied from an x86 header shifts every later field by four.
 *
 * Build and run:
 *   out/ua-ref-win.exe -b win/x86_64 -o structs.exe examples/win/structs.c
 *   ./structs.exe
 */
#include <stdio.h>

#define PROCESSOR_ARCHITECTURE_AMD64 9UL
#define PROCESSOR_ARCHITECTURE_ARM64 12UL

void GetSystemInfo(void *info);
void GetLocalTime(void *st);
unsigned long GetTimeZoneInformation(void *tz);
void GetSystemTime(void *st);

static unsigned int u16at(unsigned char *p, unsigned long off)
{
    return (unsigned int)p[off] + ((unsigned int)p[off + 1] << 8);
}

static unsigned long u32at(unsigned char *p, unsigned long off)
{
    return (unsigned long)p[off]
         + ((unsigned long)p[off + 1] << 8)
         + ((unsigned long)p[off + 2] << 16)
         + ((unsigned long)p[off + 3] << 24);
}

static void hex(unsigned char *p, unsigned long n)
{
    unsigned long i;
    for (i = 0; i < n; i = i + 1) {
        printf("%02lx ", (unsigned long)p[i]);
    }
    printf("\n");
}

int main(void)
{
    unsigned char sysinfo[48];
    unsigned char tz[172];
    unsigned char st[18];
    unsigned long arch;

    GetSystemInfo(sysinfo);
    GetTimeZoneInformation(tz);
    GetLocalTime(st);

    printf("SYSTEM_INFO\n");
    hex(sysinfo, 48);
    arch = u16at(sysinfo, 0);
    printf("arch %lu amd64 %d arm64 %d\n", arch,
           arch == PROCESSOR_ARCHITECTURE_AMD64, arch == PROCESSOR_ARCHITECTURE_ARM64);
    printf("page %lu min %lu max %lu\n",
           u32at(sysinfo, 4), u32at(sysinfo, 8), u32at(sysinfo, 16));
    printf("mask %lu procs %lu granularity %lu\n",
           u32at(sysinfo, 24), u32at(sysinfo, 32), u32at(sysinfo, 40));

    printf("TIME_ZONE_INFORMATION\n");
    printf("bias %ld name head %04lx %04lx\n",
           (long)(int)u32at(tz, 0), u16at(tz, 4), u16at(tz, 6));

    printf("SYSTEMTIME\n");
    hex(st, 18);
    printf("%04u/%02u/%02u %02u:%02u:%02u.%03u wd%u\n",
           u16at(st, 0), u16at(st, 2), u16at(st, 6),
           u16at(st, 8), u16at(st, 10), u16at(st, 12), u16at(st, 14),
           u16at(st, 4));
    return 0;
}