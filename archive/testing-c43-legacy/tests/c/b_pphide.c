/* Hide sets (C99 6.10.3.4): a macro name met again during the rescan of
   its own replacement is not replaced -- it is "painted blue" and stays an
   ordinary identifier for good.  `#define foo foo + 1` once expanded eight
   times over. */
#include <stdio.h>
int foo = 10;
#define foo foo + 1
int x = 2, y = 3;
#define x (4 + y)
#define y (2 * x)
int f(int a) { return a * 1000; }
int g = 3;
#define f(a) a*g
#define g(a) f(a)
#define AA BB
#define BB AA
int AA = 5;
int BB = 6;
int self = 7;
#define self(n) (self + n)
#define str(s) # s
#define xstr(s) str(s)
#define ID(p) p
#define LOOP ID(LOOP)
int LOOP = 9;
#define x2 x2
int x2 = 11;
int main(void) {
    printf("%d %d %d\n", foo, x, y);
    printf("%d %d %d %d\n", f(2)(9), AA, BB, self(3));
    printf("%s | %s | %s\n", xstr(foo), xstr(f(2)(9)), xstr(x));
    printf("%d %d %s\n", LOOP, x2, xstr(LOOP));
    return 0;
}
