#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
static volatile sig_atomic_t seen = 0;
static void h(int s) { seen = s; }
int main(void)
{
    void (*old)(int);
    old = signal(SIGTERM, h);
    printf("%d\n", old == SIG_DFL);
    raise(SIGTERM);
    /* whether delivery resets the handler to SIG_DFL is implementation-
       defined (C99 7.14.1.1p3): glibc in strict C99 mode resets, BSD libc
       does not, so only the handler's effect is printed */
    printf("%d\n", (int)seen);
    signal(SIGTERM, SIG_IGN);
    raise(SIGTERM);
    printf("still here\n");
    return 0;
}
