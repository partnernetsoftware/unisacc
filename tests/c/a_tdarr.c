/* typedef'd array types (jmp_buf-shaped): global, local, static, struct
   member, parameter decay, sizeof, indexing -- compared with cc */
#include <stdio.h>
typedef long jb[8];
typedef char name16[16];
typedef struct { int a, b; } P; typedef P P3[3];
jb g; name16 gn = "hello";
struct S { int x; jb b; name16 n; int y; };
static long sum(jb e) { long s = 0; int i; for (i = 0; i < 8; i++) s += e[i]; return s + (long)sizeof e; }
static long first(long *p) { return p[0]; }
int main(void) {
    jb l; struct S s; P3 ps; static jb st; int i;
    for (i = 0; i < 8; i++) { g[i] = i; l[i] = 10 * i; s.b[i] = 100 * i; st[i] = 1000 * i; }
    s.x = 1; s.y = 2; ps[2].b = 42;
    printf("%ld %ld %ld %ld\n", sum(g), sum(l), sum(s.b), sum(st));
    printf("%d %d %d %d\n", (int)sizeof(jb), (int)sizeof g, (int)sizeof l, (int)sizeof s.b);
    printf("%d %d %d %s %c\n", (int)sizeof(P3), ps[2].b, s.y, gn, s.n[0] = 'z');
    printf("%ld %ld\n", first(g), first(l + 3));
    return 0;
}
