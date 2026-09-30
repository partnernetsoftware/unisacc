#include <stdio.h>
#include <string.h>
int main(void) {
    printf("%ld %ld %ld %ld\n", (long)strspn("aaab", "a"),
           (long)strspn("", "abc"), (long)strspn("xy", ""),
           (long)strspn("abc", "abc"));
    return 0;
}
