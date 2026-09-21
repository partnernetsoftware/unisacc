/* A format string that is not a literal.  The compiler desugars printf
   against a STATIC format [W-9]; this one cannot be desugared at all, so it
   has to become a real variadic call on the printf in <stdio.h>. */
#include <stdio.h>

static const char *pick(int i)
{
    if (i == 0) return "%s=%d\n";
    return "[%s] %d\n";
}

int main(void)
{
    const char *fmt = "%d %s %c %x\n";
    int i;
    printf(fmt, 42, "mid", 'z', 255);
    for (i = 0; i < 2; i = i + 1)
        printf(pick(i), "k", i * 7);
    printf("%s", "literal still goes the fast way\n");
    printf("%5d|%-5d|%05d|\n", 42, 42, 42);
    return 0;
}
