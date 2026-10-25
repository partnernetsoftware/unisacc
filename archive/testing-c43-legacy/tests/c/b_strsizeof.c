/* String literals retain array extent under sizeof, including embedded NUL;
   value operators still decay them to pointers. */
#include <stdio.h>
int main(void) {
    printf("%lu %lu %lu %lu %lu\n", (unsigned long)sizeof "abc",
           (unsigned long)sizeof("\0cli/defines"),
           (unsigned long)sizeof("a\0b" "cd"),
           (unsigned long)sizeof(""), (unsigned long)sizeof(L"ab"));
    printf("%lu %lu %lu %lu %lu\n", (unsigned long)sizeof(("abc")),
           (unsigned long)sizeof("abc" + 0),
           (unsigned long)sizeof(1 ? "abc" : "z"),
           (unsigned long)sizeof((0, "abc")),
           (unsigned long)sizeof("abc"[1]));
    return 0;
}
