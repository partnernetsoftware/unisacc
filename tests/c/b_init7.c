/* Initialisers by brace level (C99 6.7.8): a closing brace moves on to the
   member AFTER the braced one however much it left out; a designator
   resolves in its own level; three-dimensional arrays nest rows of rows.
   The flat walker put the 9 below into s.b, and dropped a[..][..][k]. */
struct In { int x; int y; };
struct Out { int a; struct In s; int b; int arr[3]; };
struct Out o1 = {1, {4}, 9, {[2] = 5}};
struct Out o2 = {.s = {.y = 6}, .b = 7, .arr = {1, [2] = 3}};
int cube[2][3][4] = { { {1, 2}, {3} }, { [1] = {4, [3] = 5} } };
int flat[2][2][2] = {1, 2, 3, 4, 5, 6, 7, 8};
int main(void) {
    struct Out l = {2, {3, 4}, 5, {6}};
    int lc[2][2][3] = {{{1}, {2, 3}}, {{4, 5, 6}}};
    printf("%d %d %d %d %d %d\n", o1.a, o1.s.x, o1.s.y, o1.b, o1.arr[0], o1.arr[2]);
    printf("%d %d %d %d %d %d\n", o2.a, o2.s.x, o2.s.y, o2.b, o2.arr[0], o2.arr[2]);
    printf("%d %d %d %d %d %d\n", cube[0][0][1], cube[0][1][0], cube[1][1][0],
           cube[1][1][3], cube[1][0][0], (int)sizeof cube);
    printf("%d %d %d %d\n", flat[0][1][1], flat[1][0][1], flat[1][1][0], flat[1][1][1]);
    printf("%d %d %d %d %d\n", l.a, l.s.y, l.b, l.arr[0], l.arr[1]);
    printf("%d %d %d %d %d\n", lc[0][0][0], lc[0][1][1], lc[1][0][2], lc[1][1][0], (int)sizeof lc);
    return 0;
}
