/* examples/win/tick.c -- the shortest possible host forward on Windows.
 *
 * Every function here is a kernel32 prototype with no definition and no
 * bundled body, which is what makes it a forward: the driver emits a stub
 * that resolves the name through the host channel (LoadLibraryA /
 * GetProcAddress on kernel32) and calls it with unisacc's Win64 call.
 * Nothing in this file is a bundled libc function, so if this probe runs,
 * the Windows host path works at all.
 *
 * Exercises: 0-argument and 1-argument integer returns, void return,
 * a pointer out-parameter, and two independent calls in one expression.
 *
 * Today this needs the CLASSIC route (out/ua-ref-win.exe or make
 * classic-com). The shipped model product refuses every Win32 prototype on a
 * win target by name -- see examples/win/README.md.
 *
 * Build and run:
 *   out/ua-ref-win.exe -b win/x86_64 -o tick.exe examples/win/tick.c && ./tick.exe
 */
#include <stdio.h>

/* kernel32, resolved at run time. Prototypes only: no definitions. */
unsigned int GetTickCount(void);
unsigned long GetTickCount64(void);
unsigned int GetCurrentProcessId(void);
void Sleep(unsigned int ms);
int QueryPerformanceCounter(long *counter);
int QueryPerformanceFrequency(long *freq);

int main(void)
{
    unsigned int t0;
    unsigned int t1;
    unsigned long q0;
    unsigned long q1;
    long freq;
    int i;

    t0 = GetTickCount();
    q0 = 0;
    q1 = 0;
    /* Both of these are forwarded; the second one has a pointer argument. */
    QueryPerformanceCounter(&q0);
    if (!QueryPerformanceFrequency(&freq)) {
        printf("tick: QueryPerformanceFrequency failed\n");
        return 1;
    }
    for (i = 0; i < 3; i = i + 1) {
        Sleep(10);
    }
    t1 = GetTickCount();
    QueryPerformanceCounter(&q1);

    printf("pid>0 %d\n", GetCurrentProcessId() > 0);
    printf("tick32 wraps %d\n", t1 >= t0);
    printf("tick64>0 %d\n", GetTickCount64() > 0);
    /* 10 ms of sleep must show up in both clocks; q1 - q0 is in counts. */
    printf("tick32 delta %u ms\n", t1 - t0);
    printf("qpc freq>0 %d delta>0 %d\n", freq > 0, q1 > q0);
    return 0;
}
