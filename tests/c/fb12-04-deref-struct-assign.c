/* R13-0 #04 P0 -- a struct assignment through a dereferenced pointer.
   Expected: compiles and exits 22 on every path.
   Got on 0.0.12 (df8cc9b4): `fb12-04-deref-struct-assign.c:12:12: error:
   not covered: width` for `*pp = b;` and for `*pp = mk(11);`, while
   `pp[0] = b;` is accepted -- a width missing from the lowering/model table
   for a dereferenced struct store, surfacing as a front-end error. */
struct P { int x, y; };
struct P mk(int a) { struct P p; p.x = a; p.y = a * 2; return p; }
int main(void) {
    struct P a, b;
    struct P *pp = &a;
    b.x = 1; b.y = 2;
    *pp = b;
    *pp = mk(11);
    return a.y;
}
