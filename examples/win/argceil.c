/* examples/win/argceil.c -- where the forward'"'"'s argument ceiling actually is.
 *
 * A Windows forward with at most six integer/pointer arguments is emitted as
 * a direct host call; a seventh needs the libffi bridge that only macOS and
 * Linux have. Six is the documented number, but the Microsoft x64 calling
 * convention only has four register slots, so arguments five and six arrive
 * on the stack -- and this probe is what establishes whether the generated
 * call builds that stack frame correctly on Windows.
 *
 * Each call is followed by a flush, so a crash names the exact arity that
 * broke rather than leaving an empty line:
 *
 *   4 args  VirtualProtect   measured working (vmem.c)
 *   5 args  SearchPathA      measured here
 *   6 args  CreateEventExA   measured here
 *   7 args  CreateFileA      refused at compile time (refused/createfile7.c)
 *
 * All four names are plain kernel32, so nothing here depends on which DLL a
 * forward searches -- this is purely about how the call is built.
 *
 * Build and run:
 *   out/ua-ref-win.exe -b win/x86_64 -o argceil.exe examples/win/argceil.c
 *   ./argceil.exe
 */
#include <stdio.h>

void *VirtualAlloc(void *addr, unsigned long size, unsigned long type, unsigned long prot);
int VirtualProtect(void *addr, unsigned long size, unsigned long prot, unsigned long *old);
unsigned long SearchPathA(const char *path, const char *file,
                          const char *ext, char *buffer, unsigned long size);
void *CreateEventExA(void *attrs, const char *name, unsigned long flags,
                     unsigned long types, unsigned long initial,
                     void *extended);
int CloseHandle(void *handle);
unsigned long GetLastError(void);

int main(void)
{
    char found[520];
    unsigned long n;
    void *page;
    unsigned long old;
    void *event;

    page = VirtualAlloc(0, 4096, 4096UL + 8192UL, 4UL);
    printf("0-4 args alloc %d\n", page != 0);
    fflush(stdout);
    printf("4 args protect %d\n", VirtualProtect(page, 4096, 1UL, &old));
    fflush(stdout);

    found[0] = 0;
    n = SearchPathA(0, "notepad.exe", 0, found, 520);
    printf("5 args searchpath %lu found %d\n", n, n > 0);
    fflush(stdout);
    if (n > 0) {
        printf("5 args path %s\n", found);
        fflush(stdout);
    }

    event = CreateEventExA(0, 0, 0, 0, 0, 0);
    printf("6 args event %d\n", event != 0);
    fflush(stdout);
    if (event != 0) {
        printf("6 args close %d\n", CloseHandle(event));
    }
    return 0;
}