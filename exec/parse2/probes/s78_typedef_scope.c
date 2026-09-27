#include <stdio.h>
typedef long W;
enum E { K = 7 };
int inc(int x) { return x + 1; }
int check(void) {
    W a = 9; int r = 0;
    { typedef char W; W b = 3; r = sizeof(b);
      { typedef short W; W c = 2; r = r * 10 + sizeof(c); }
      r = r * 10 + sizeof(b);
    }
    { int W = 5; r += W; }
    { enum E { K = 2, J = K + 3 }; enum E q = J; r += q + K; }
    { typedef int (*F)(int); F fn = inc; r += fn(2); }
    return r + K + sizeof(a);
}
int main(void) { W a = 1; printf("%d %ld %d\n", check(), (long)sizeof(a), K); return 0; }
