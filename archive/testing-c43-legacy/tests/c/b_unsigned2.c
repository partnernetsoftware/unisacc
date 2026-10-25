/* unsigned semantics: wraparound, comparison, shift, divide, literal typing */
#include <stdio.h>
int main() {
    unsigned char c;
    unsigned short h;
    unsigned int a;
    unsigned long b;
    int s;
    c = 200; h = 60000; a = 4000000000; b = 0; s = 0 - 1;
    b = b - 1;
    printf("%d %d %d\n", (int)c, (int)h, c + h);
    printf("%d %d %d %d\n", a > 2147483647, a / 1000, a % 1000, (int)(a >> 28));
    printf("%d %d %d\n", b > 0, (int)(b >> 60), s >> 1 < 0);
    printf("%d %d\n", (int)(unsigned char)(0 - 1), (int)(unsigned short)(0 - 1));
    /* C99 6.4.4.1: 0xffffffff is unsigned int, so this comparison is true */
    printf("%d %d\n", s == 0xffffffff, (unsigned int)s == 4294967295);
    return 0;
}
