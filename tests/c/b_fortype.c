/* A for initializer may start with a typedef or an enum tag, not only
   a built-in type keyword. This is used by the generic C executor. */
#include <stdio.h>
#include <stddef.h>
typedef long Index;
enum Step { FIRST = 0, LAST = 3 };
int main(void) {
    long sum = 0;
    for (size_t i = 0; i < 4; i++) sum += i;
    for (Index j = -2; j < 2; j++) sum += j;
    for (enum Step k = FIRST; k < LAST; k++) sum += k;
    printf("%ld\n", sum);
    return sum != 7;
}
