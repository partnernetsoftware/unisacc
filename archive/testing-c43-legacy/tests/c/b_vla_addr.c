/* 0.0.34 L2 (fa35fcd5, 18283e47): &array is the array's storage for a VLA and for a
   sizeof(member) bound (a constant, not a VLA); sqlite's dbFileVers read */
#include <stdio.h>
#include <string.h>
typedef struct P { int a; char vers[16]; int b; } P;
struct S { char v[5]; int w; };
static int rd(void *buf, int n) { memset(buf, 7, n); return 0; }
static int f(P *p, int n, int *out) {
    int keep = 3; int *pk = &keep;
    char a[sizeof(p->vers)];
    char v[n];
    char s[sizeof(struct S)];
    rd(&a, sizeof(a)); rd(&v, n);
    *pk = keep + a[15] + v[n - 1];
    *out = (int)sizeof(a) * 100 + (int)sizeof(s);
    return keep;
}
int main(void) { P p; int o = 0; memset(&p, 0, sizeof p); printf("%d", f(&p, 6, &o)); printf(" %d\n", o); return 0; }
