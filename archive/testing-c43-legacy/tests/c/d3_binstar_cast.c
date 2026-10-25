/* 0.0.28 D3: after a binary `*`, `(T)(x).m` is a cast of a member, not a parenthesised declarator name
   (SQLite's pcache1: `pCache->szAlloc * (i64)(pcache1_g).nInitPage`).  cc prints 8 20 18 4. */
#include <stdio.h>
typedef long long i64;
struct G { int a; int b; };
static struct G g = {3, 4};
struct P { int n; };
int main(void) {
    struct P q = {5}; struct P *p = &q; int x = 6;
    i64 v1 = 2*(i64)(g).b, v2 = p->n * (i64)(g).b, v3 = x * (i64)(g).a, v4 = (i64)(g).b;
    printf("%lld %lld %lld %lld\n", v1, v2, v3, v4);
    return 0;
}
