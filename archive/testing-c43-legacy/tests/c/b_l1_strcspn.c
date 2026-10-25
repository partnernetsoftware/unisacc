#include <stdio.h>
#include <string.h>
int main(void) {
    printf("%ld %ld %ld\n", (long)strcspn("abcX", "X"),
           (long)strcspn("abc", ""), (long)strcspn("", "abc"));
    return 0;
}
