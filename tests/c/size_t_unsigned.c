/* size_t is unsigned (C99 7.17): SIZE_MAX compares above every index.
 * <stddef.h> once declared it `long`, so `i < SIZE_MAX` sentinels went
 * signed and seed/gen.c's rule-list merge indexed rules[SIZE_MAX]. */
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
int main(void) {
    size_t a = 0, b = SIZE_MAX, next[3] = {1, SIZE_MAX, SIZE_MAX};
    printf("%d %d\n", a < b, (size_t)-1 > 0);
    while (a != SIZE_MAX || b != SIZE_MAX) {
        size_t r;
        if (a < b) { r = a; a = next[r]; } else { r = b; b = next[r]; }
        printf("%lu\n", (unsigned long)r);
    }
    return 0;
}
