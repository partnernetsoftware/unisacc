/* Complete literal operands retain their array extent under sizeof. */
#include <stdio.h>
int main(void) {
    printf("%lu %lu %lu %lu %lu\n", (unsigned long)sizeof "abc",
           (unsigned long)sizeof("\0cli/defines"),
           (unsigned long)sizeof(("a\0b" "cd")),
           (unsigned long)sizeof(""), (unsigned long)sizeof("abc"[1]));
    return 0;
}
