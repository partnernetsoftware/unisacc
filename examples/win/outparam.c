/* examples/win/outparam.c -- four arguments DO arrive, with out-parameters.
 *
 * The positive half of refused/wideargs.c. A Windows host call has four
 * integer register slots, and this probe fills all of them and reads three
 * out-parameters back: GetDiskFreeSpaceExA is called with a path and three
 * pointers, and the numbers that come back are this machine's disk, not
 * whatever was on the stack. It is the control for the silent-drop
 * measurement, and it is why the reachable half of computer use is reachable
 * at all: window enumeration, hotkeys, the tray, cursor moves and injected
 * keystrokes are all four arguments or fewer.
 *
 * (Measured on this machine: free 29 GB, total 268 GB. The 4-argument slot
 * budget is the whole reason gui/input.c works and gui/window.c does not.)
 *
 * Build and run:
 *   out/ua-ref-win.exe -b win/x86_64 -o outparam.exe examples/win/outparam.c
 *   ./outparam.exe
 */
#include <stdio.h>

#define FILE_NAME "unisacc-win-probe.txt"

int GetDiskFreeSpaceExA(const char *path, unsigned long *free_to_me,
                        unsigned long *total, unsigned long *total_free);
unsigned long GetTempPathA(unsigned long len, char *buf);
unsigned long GetEnvironmentVariableA(const char *name, char *buf, unsigned long len);
int DeleteFileA(const char *path);

int main(void)
{
    char path[520];
    char name[520];
    unsigned long free_to_me;
    unsigned long total;
    unsigned long total_free;
    unsigned long n;

    /* Four arguments, three of them out-parameters, and the values that come
       back are the disk's. */
    free_to_me = 0;
    total = 0;
    total_free = 0;
    if (GetDiskFreeSpaceExA("C:\\", &free_to_me, &total, &total_free) == 0) {
        printf("disk query failed\n");
        return 1;
    }
    printf("free %lu total %lu total_free %lu\n", free_to_me, total, total_free);
    printf("out-parameters arrived %d\n", total > 0 && free_to_me > 0);

    /* Three arguments with a buffer, and a two-argument call into the same
       slot budget, to show the budget is not the only thing that works. */
    n = GetTempPathA(260, path);
    printf("temp (%lu) %s\n", n, path);
    n = GetEnvironmentVariableA("USERNAME", name, 260);
    printf("user (%lu) %s\n", n, name);

    /* And a buffer written by the host, then deleted, so the probe leaves
       nothing behind. */
    n = GetEnvironmentVariableA("TEMP", name, 260);
    if (n > 0 && n < 250) {
        unsigned long i;
        unsigned long len;
        len = 0;
        while (name[len]) {
            len = len + 1;
        }
        for (i = len; i > 0; i = i - 1) {
            if (name[i - 1] == 92) {
                break;
            }
        }
        /* i is the offset just past the last backslash. */
        if (i < 250) {
            unsigned long j;
            j = 0;
            while (FILE_NAME[j]) {
                name[i + j] = FILE_NAME[j];
                j = j + 1;
            }
            name[i + j] = 0;
            printf("probe file %s deleted %d\n", name, DeleteFileA(name));
        }
    }
    return 0;
}
