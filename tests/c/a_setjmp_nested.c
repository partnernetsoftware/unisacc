/* A resumed setjmp must restore the pending expression temporaries. */
#include <setjmp.h>
#include <stdio.h>

static jmp_buf jb;

int main(void) {
    volatile int a = 0, b = 0;
    if ((a = (b = setjmp(jb))) == 0) longjmp(jb, 3);
    printf("%d %d\n", a, b);
    return 0;
}
