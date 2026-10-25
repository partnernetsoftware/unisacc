/* structs by value: assignment, parameters, return */
struct P { int x; int y; int z; };
struct P mk(int a) {
    struct P p;
    p.x = a; p.y = a * 2; p.z = a * 3;
    return p;
}
int sum(struct P p) { p.x = 100; return p.x + p.y + p.z; }
struct Big { int v[9]; };
struct Big mkbig(int s) {
    struct Big b;
    int i;
    for (i = 0; i < 9; i = i + 1) b.v[i] = s + i;
    return b;
}
int main() {
    struct P a;
    struct P b;
    struct Big g;
    a.x = 1; a.y = 2; a.z = 3;
    b = a;
    b.y = 20;
    g = mkbig(10);
    printf("%d %d %d %d %d %d\n", b.x, b.y, b.z, a.y, sum(a), a.x);
    a = mk(5);
    printf("%d %d %d %d %d %d\n", a.x, a.y, a.z, sum(mk(2)), mk(7).y, g.v[8]);
    return 0;
}
