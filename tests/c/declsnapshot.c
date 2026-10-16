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
/* Comma declarators retain specifier width, pointer depth and dimensions. */
static char *global_slots[1], *global_after = "0) return";
static char *global_before = "1) return", *global_last[1];
struct DeclOwner { long x, y; };
static struct DeclOwner (*global_fn)(void), global_object;
static int declarator_widths(void) {
    struct DeclOwner (*fn)(void), object;
    char *slots[1], *after;
    char *before, *last[1];
    char *rows[1][2], *row_tail;
    char *depth_slots[1], **indirect, *depth_tail;
    static char *static_slots[1], *static_tail;
    typedef char *CharPtr;
    CharPtr typedef_slots[1], typedef_tail;
    short values[2] = {0x1234, 0};
    short *short_slots[1], *short_tail;
    after = "2) return"; before = "3) return";
    row_tail = "4) return"; rows[0][1] = row_tail;
    depth_tail = "5) return"; indirect = &depth_tail;
    static_tail = "6) return"; typedef_tail = "7) return";
    short_tail = values;
    object.x = 17; object.y = 25; global_object.x = 19; global_object.y = 23;
    slots[0] = after; last[0] = before; depth_slots[0] = depth_tail;
    static_slots[0] = static_tail; typedef_slots[0] = typedef_tail;
    short_slots[0] = short_tail; global_slots[0] = global_after; global_last[0] = global_before;
    return global_after[0] == '0' && global_before[0] == '1' &&
           after[0] == '2' && before[0] == '3' && rows[0][1][0] == '4' &&
           row_tail[0] == '4' && **indirect == '5' && depth_tail[0] == '5' &&
           static_tail[0] == '6' && typedef_tail[0] == '7' && short_tail[0] == 0x1234 &&
           sizeof(slots) == sizeof(char *) && sizeof(rows) == 2 * sizeof(char *) &&
           sizeof(row_tail) == sizeof(char *) && sizeof(indirect) == sizeof(char **) &&
           sizeof(object) == 2 * sizeof(long) && object.x + object.y == 42 &&
           sizeof(global_object) == 2 * sizeof(long) && global_object.x + global_object.y == 42;
}
typedef struct { char *p; size_t n, cap; } InitOwner;
static InitOwner make_owner(const char *p, const char **end) {
    InitOwner value = {0};
    value.p = (char *)p; value.n = 6; value.cap = 68; *end = p + value.n;
    return value;
}
static int initializer_owner(void) {
    char *q = 0; const char *end = 0;
    InitOwner cast = make_owner("python", (const char **)&q);
    InitOwner sized = make_owner(sizeof((long *)0) == sizeof(long *) ? "python" : "bad", &end);
    InitOwner compound = make_owner((char[]){"python"}, &end);
    InitOwner direct = (InitOwner){(char *)"python", sizeof("python") - 1, 68};
    return cast.n == 6 && cast.cap == 68 && cast.p[0] == 'p' &&
           sized.n == 6 && sized.cap == 68 && sized.p[0] == 'p' &&
           compound.n == 6 && compound.cap == 68 && compound.p[0] == 'p' &&
           direct.n == 6 && direct.cap == 68 && direct.p[0] == 'p';
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
    printf("widths %d\n", declarator_widths());
    printf("init %d\n", initializer_owner());
    return 0;
}
