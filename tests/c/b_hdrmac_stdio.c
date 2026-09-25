/* Macros named fmt, buf, n, width, prec, v, f, ap before <stdio.h>. */
#define fmt 11
#define buf 12
#define n 13
#define width 14
#define prec 15
#define v 16
#define f 17
#define ap 18
#include <stdio.h>
int main(void) {
    char t[64];
    int a, b;
    snprintf(t, sizeof t, "%5d|%-4s|%.3f|%x", 42, "ab", 3.14159, 255);
    puts(t);
    sprintf(t, "%ld %c", 123456789L, 'z');
    printf("%s\n", t);
    a = 0; b = 0;
    sscanf("7 9", "%d %d", &a, &b);
    printf("%d %d %d %d %d %d %d %d %d %d\n", a, b, fmt, buf, n, width, prec, v, f, ap);
    fputs("done\n", stdout);
    return 0;
}
