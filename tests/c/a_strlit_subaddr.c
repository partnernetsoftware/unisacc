#include <stdio.h>

int main(void) {
    const char *p = &"abc"[1];
    printf("%c %c\n", p[0], p[1]);
    return 0;
}
