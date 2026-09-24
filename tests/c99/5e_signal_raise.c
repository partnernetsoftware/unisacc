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
    printf("%d %d\n", (int)seen, signal(SIGTERM, SIG_IGN) == SIG_DFL);
    raise(SIGTERM);
    printf("still here\n");
    return 0;
}
