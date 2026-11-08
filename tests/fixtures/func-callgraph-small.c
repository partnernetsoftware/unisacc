#include <stdio.h>
#define M(x) do { \
    x; \
} while (0)
typedef struct { int a; } S;
static const int T[] = { 1, 2,
    3 };
/* { 注释里的花括号 */
static int one(void) { return 1; }
static int alpha(int x)
{
    const char *s = "}{";   // }
    char c = '}';
    return x + c + (int)(s != 0);
}
static long beta(long a,
                 long b) {
    if (a) { b++; }
    return alpha((int)b);
}
int main(void) { return (int)beta(1, 2); }
int gamma_f(void) {
    return 0;
}
