/* E19: the star before a cast is binary after an operand, including postfix ++. */
#include <stdio.h>
typedef long long T;
struct G { int a, b; };
int main(void) {
    struct G g = {2, 4};
    long long a = 1, b = 2, r = 96, n = 2, x = 3, y = 2;
    long long shift = a << b * (T)(y);
    r /= n * (T)(g).b;
    long long post = x++ * (T)(y);
    printf("%lld %lld %lld %lld\n", shift, r, post, x);
    return 0;
}
