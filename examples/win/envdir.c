/* examples/win/envdir.c -- paths and environment, the inputs a launcher needs.
 *
 * The ORB's "Run..." dialog, a crash report and a log all need the same four
 * things: the temp directory, the system directory, the current directory and
 * one environment variable. None of them is in the bundled libc on Windows,
 * because there is no Windows host libc here -- every one is a forward.
 *
 * Exercises: 1, 2, 3 and 4 argument forwards, an out-parameter that reports
 * the required length (GetComputerNameA), a write into the environment
 * (SetEnvironmentVariableA), and the difference between a value that fits and
 * a value that is truncated.
 *
 * Build and run:
 *   out/ua-ref-win.exe -b win/x86_64 -o envdir.exe examples/win/envdir.c
 *   ./envdir.exe
 */
#include <stdio.h>

unsigned long GetEnvironmentVariableA(const char *name, char *buf, unsigned long len);
int SetEnvironmentVariableA(const char *name, const char *value);
unsigned long GetCurrentDirectoryA(unsigned long len, char *buf);
unsigned long GetSystemDirectoryA(char *buf, unsigned long len);
unsigned long GetTempPathA(unsigned long len, char *buf);
unsigned long ExpandEnvironmentStringsA(const char *in, char *out, unsigned long len, void *flags);
int GetComputerNameA(char *buf, unsigned long *len);
unsigned long GetLastError(void);

int main(void)
{
    char buf[520];
    char small[8];
    char expanded[600];
    char name[64];
    unsigned long need;
    unsigned long n;

    n = GetEnvironmentVariableA("USERNAME", buf, 260);
    printf("USERNAME (%lu) %s\n", n, buf);

    /* Ask for a buffer that cannot hold the value: the call succeeds, the
       buffer is truncated, and the return value is what it needed. This is
       the idiom every caller has to get right. */
    n = GetEnvironmentVariableA("USERNAME", small, 4);
    printf("USERNAME small (%lu) %s\n", n, small);

    n = GetCurrentDirectoryA(260, buf);
    printf("cwd (%lu) %s\n", n, buf);

    n = GetSystemDirectoryA(buf, 260);
    printf("system (%lu) %s\n", n, buf);

    n = GetTempPathA(260, buf);
    printf("temp (%lu) %s\n", n, buf);

    /* A write, then a read back: the environment is a process-local thing
       and this is the only way to see it change. */
    if (SetEnvironmentVariableA("UNISACC_WIN", "yes")) {
        n = GetEnvironmentVariableA("UNISACC_WIN", buf, 260);
        printf("UNISACC_WIN (%lu) %s\n", n, buf);
    } else {
        printf("UNISACC_WIN set failed %lu\n", GetLastError());
    }

    /* Four arguments, and the last one is a NULL out-parameter. */
    n = ExpandEnvironmentStringsA("%TEMP%\\unisacc-win-probe.txt",
                                  expanded, 300, 0);
    printf("expand (%lu) %s\n", n, expanded);

    /* need is in/out: pass the capacity, get the length back. */
    need = 32;
    if (GetComputerNameA(name, &need)) {
        printf("computer (%lu) %s\n", need, name);
    } else {
        printf("computer failed %lu\n", GetLastError());
    }
    return 0;
}
