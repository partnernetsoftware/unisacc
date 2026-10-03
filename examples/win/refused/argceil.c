/* examples/win/refused/argceil.c -- the argument ceiling, measured and then
 * pinned.
 *
 * Four arguments is what a Windows host call delivers, and this probe walks
 * the ladder: 0, 1, 4, 5, 6, 7. The first four worked and still work
 * (tick.c, vmem.c, outparam.c). The rest used to be emitted anyway and used
 * to lose the arguments past the fourth in silence -- this file is where that
 * was measured, on this machine, before the fix:
 *
 *   4 args  VirtualProtect     worked
 *   5 args  SearchPathA        returned the right length, wrote nothing
 *   6 args  CreateEventExA     returned a plausible handle, stack slots garbage
 *   7 args  CreateFileA        no stub at all (fwd_emit stops at six)
 *
 * 0.0.23 turns the silent part into a refusal: src/fwdstub.c carries
 * fwd_maxargs, the driver sets it from the -b target, and src/front_parse.c
 * refuses a wider forward BY NAME. So this file no longer compiles, and that
 * is the regression test:
 *
 *   unisacc: error: host function 'SearchPathA' takes 5 arguments, and this
 *   target's host call delivers 4. ...
 *
 * Four-argument calls are not affected; vmem.c and outparam.c are the evidence
 * that the ceiling did not move under the working ones.
 *
 *   out/ua-ref-win.exe -b win/x86_64 -o argceil.exe examples/win/refused/argceil.c
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