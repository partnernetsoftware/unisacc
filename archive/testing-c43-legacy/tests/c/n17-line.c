/* N17a: __LINE__ is the physical line of the token, spliced headers and
   joined continuation lines notwithstanding; inside a macro body it is the
   line of the invocation.  gcc/clang agree on every value below. */
#include <stdio.h>
#define WHERE __LINE__
#define AT(x) ((x) + __LINE__)
#define PAIR(a, b) a, b
int main(void) {
    printf("%d\n", __LINE__);
    printf("%d\n", AT(1000));
    printf("%d %d\n", PAIR(__LINE__,
                           __LINE__));
    int x = 1 + \
        __LINE__;
    printf("%d\n", x);
    printf("%d\n", __LINE__);
    return 0;
}
