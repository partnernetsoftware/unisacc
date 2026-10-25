#include <stdio.h>
int main(void)
{
    int a, b, n; long l; unsigned u; char w[16]; char c; double d; float f; short h;
    int r = sscanf("  42 -7 0x1F 077 word", "%d %d %x %o %s", &a, &b, &u, &n, w);
    printf("%d: %d %d %u %d %s\n", r, a, b, u, n, w);
    r = sscanf("12345678901 3.25 -1.5e2 x", "%ld %lf %f %c", &l, &d, &f, &c);
    printf("%d: %ld %.2f %.1f %c\n", r, l, d, (double)f, c);
    r = sscanf("k=17;v=9", "k=%d;v=%d", &a, &b);
    printf("%d: %d %d\n", r, a, b);
    r = sscanf("abc", "%d", &a);
    printf("%d\n", r);
    r = sscanf("", "%d", &a);
    printf("%d\n", r == EOF);
    r = sscanf("99 1234 5", "%*d %2d %hd %n", &a, &h, &n);
    printf("%d: %d %d %d\n", r, a, h, n);
    r = sscanf("0x10 010 10", "%i %i %i", &a, &b, &n);
    printf("%d: %d %d %d\n", r, a, b, n);
    return 0;
}
