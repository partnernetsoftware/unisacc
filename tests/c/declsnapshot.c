/* A type name in one initializer must not change the next declarator. */
#include <stdio.h>
static int from_byte(unsigned char x) { return x ? 0 : -1; }
static int local_pair(void) {
    char p[2];
    p[0] = 1; p[1] = 0;
    int hi = from_byte((unsigned char)p[0]), lo = from_byte((unsigned char)p[1]);
    return hi < 0 || lo < 0;
}
int ga = (unsigned char)1, gb = -1;
unsigned int gu = (int)1, gv = 4294967295U;
typedef unsigned short Small;
typedef Small Arr[sizeof(char)], Other;
struct Members { int a[sizeof(unsigned char)], b; unsigned short c, d; };
struct DimS { char a[sizeof((long *)0)], b; };
union DimU { char a[sizeof((long *)0)], b; };
struct BitS { unsigned a:sizeof((int *)0), b:1; };
unsigned proto(int), proto_object;
static int parameter_dims(unsigned a[][sizeof((int)0)], int b) {
    return (a[0][0] >> 31) == 1 && b < 0;
}
static int more_contexts(void) {
    static int a = (unsigned char)1, b = -1;
    unsigned block_proto(int), object = 4294967295U;
    unsigned arr[1][sizeof(int)] = {4294967295U};
    struct BitS bits = {0,0};
    int compound = (unsigned char){1}, signed_next = -1;
    int size = sizeof(unsigned char), signed_size_next = -1;
    int loop = 0;
    for (int x = (unsigned char)0, y = -1; x < 1; x++) loop = y < 0;
    bits.a = 3; bits.b = 1; proto_object = 4294967295U;
    return a == 1 && b < 0 && (object >> 31) == 1 &&
           (proto_object >> 31) == 1 && parameter_dims(arr, -1) &&
           sizeof(struct DimS) == sizeof(long *) + 1 &&
           sizeof(union DimU) == sizeof(long *) && bits.a == 3 && bits.b == 1 &&
           compound == 1 && signed_next < 0 && size == 1 && signed_size_next < 0 && loop;
}
int main(void) {
    int a = (unsigned char)1, b = -1;
    unsigned int u = (int)1, v = 4294967295U;
    short sh = (unsigned char)1, sn = -1;
    const volatile long la = (char)1, lb = -1;
    Small sa = (int)1, sb = 65535;
    Arr ar; Other other = 65535; struct Members m;
    char dim[sizeof((long *)0)], tail;
    ar[0] = 65535; m.b = -1; m.c = 65535; m.d = 65535;
    printf("%d %d %d %d %d %d %d %d %d %d\n", local_pair(), b < 0,
           v > 2147483647U, sn < 0, lb < 0, sb == 65535,
           gb < 0, gv > 2147483647U, ar[0] == other,
           m.b < 0 && m.c == m.d && sizeof(dim) == sizeof(long *) && sizeof(tail) == 1 && more_contexts());
    return 0;
}
