/* Adjacent fields share remaining bits (C99 6.7.2.1p10). Six targets are LE.
   Zero the storage before checking bytes; padding is not an output oracle. */
#include <stdio.h>
#include <string.h>
struct Bits { unsigned int a:3, b:5, c:6; };
struct Gap { unsigned int :3; unsigned int a:5; unsigned int :0; unsigned int b:4; };
struct Cross { unsigned int a:20, b:20; };
struct Nested { struct { unsigned int a:3, b:5; } n; unsigned int a:3, b:5; };
struct Reuse { unsigned int a:24; unsigned char b; };
struct Signed { signed int a:3, b:5; };
struct Bits global_bits = { 5, 17, 37 };
int main(void) {
    struct Bits x; struct Gap y; struct Cross z; struct Nested n;
    struct Reuse r; struct Signed s; unsigned char *p;
    memset(&x, 0, sizeof x); x.a=5; x.b=17; x.c=37; p=(unsigned char *)&x;
    printf("packed %u %u %u %u %u\n", (unsigned)p[0], (unsigned)p[1], (unsigned)x.a, (unsigned)x.b, (unsigned)x.c);
    memset(&y, 0, sizeof y); y.a=17; y.b=9; p=(unsigned char *)&y;
    printf("barrier %u %u %lu\n", (unsigned)p[0], (unsigned)p[4], (unsigned long)sizeof y);
    memset(&z, 0, sizeof z); z.a=0xabcde; z.b=0x54321; p=(unsigned char *)&z;
    printf("straddle %u %u %u %u %u %u\n", (unsigned)p[0], (unsigned)p[1], (unsigned)p[2], (unsigned)p[4], (unsigned)p[5], (unsigned)p[6]);
    memset(&n, 0, sizeof n); n.n.a=5; n.n.b=17; n.a=6; n.b=19; p=(unsigned char *)&n;
    printf("nested %u %u\n", (unsigned)p[0], (unsigned)p[4]);
    memset(&r, 0, sizeof r); r.a=0x123456; r.b=0x78; p=(unsigned char *)&r;
    printf("reuse %u %u %u %u\n", (unsigned)p[0], (unsigned)p[1], (unsigned)p[2], (unsigned)p[3]);
    memset(&s, 0, sizeof s); s.a=-1; s.b=-7; p=(unsigned char *)&s;
    printf("signed %u %d %d\n", (unsigned)p[0], s.a, s.b);
    p=(unsigned char *)&global_bits;
    printf("initialised %u %u\n", (unsigned)p[0], (unsigned)p[1]);
    return 0;
}
