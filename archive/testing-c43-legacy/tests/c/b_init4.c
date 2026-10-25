/* mixed flat/braced initialisers, designators after them, scoped tags */
struct S1 { int p; int q; };
struct S2 { int a; int b; int c; struct S1 s; };
struct S2 v = {1, 2, 3, {4, 5}};
struct D { int a; int *p; };
int gx = 10;
struct D d = {.p = &gx, .a = 1};
typedef struct { int n; int sub[2]; } S;
S arr[1] = {{1, {2, 3}}};
int m[2][2] = {1, 2, 3, 4};
int n2[2][2] = {{1, 2}, {3, 4}};
struct T;
struct T { int x; };
int main() {
    struct S2 lv = {1, 2, 3, {4, 5}};
    int lm[2][2] = {1, 2, 3, 4};
    struct T outer;
    {
        struct T { int z; };
        struct T inner;
        inner.z = 9;
        printf("%d ", inner.z);
    }
    outer.x = 2;
    printf("%d %d %d %d %d %d %d\n", v.c, v.s.p, v.s.q, arr[0].sub[1],
           m[1][0], n2[1][1], lm[1][1]);
    printf("%d %d %d %d %d\n", lv.s.q, lv.c, d.a, *d.p, outer.x);
    return 0;
}
