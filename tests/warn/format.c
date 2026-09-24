#include <stdio.h>
int main(void) {
    long l = 5; int i = 3; double d = 1.5; char *s = "s";
    printf("%d %s %ld %f\n", l, i, i, i);
    printf("%d %s %ld %f %c\n", i, s, l, d, 'c');
    printf("%d\n", s);
    return 0;
}
