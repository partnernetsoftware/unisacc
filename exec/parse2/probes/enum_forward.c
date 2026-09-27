/* Forward enums are an intentional compatibility extension. */
#include <stdio.h>
enum E;
typedef enum E Alias;
enum E *early;
enum E identity(enum E x);
int (*parameter)(enum E);
enum E { ZERO, SEVEN = 7 };
enum N;
typedef enum N Neg;
enum N { MINUS = -2 };
struct Bits { Alias positive:3; Neg negative:3; };
enum E identity(enum E x) { return x; }
int use(enum E x) { return x; }
int main(void) {
    Alias x = SEVEN;
    struct Bits bits;
    early = &x;
    parameter = use;
    bits.positive = SEVEN;
    bits.negative = MINUS;
    printf("%d %d %d %d %d\n", x, *early, identity(x), parameter(x), (int)sizeof(*early));
    { enum E; enum E *inside; enum E { INNER = 3 }; enum E y=INNER; inside=&y;
      printf("%d %d %d\n", *inside, *early, (int)sizeof(*inside)); }
    x += 1;
    printf("%d %d %d %d\n", x, bits.positive, bits.negative, (int)sizeof(enum E));
    return 0;
}
