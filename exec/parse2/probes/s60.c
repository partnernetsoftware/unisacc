/* Recursive aggregate contexts: static/global/local share the same walker. */
#include <stdio.h>
struct Leaf { int x; int y; };
struct Wrap { struct Leaf a[2]; int v[3]; char tag; };
struct Wrap g = { .a = {{1}, {.y=3}}, .v = {[1+1]=4}, .tag='G' };
int cube[2][2][3] = {{{1}, {2,3}}, {[1] = {[2]=7}}};
double ds[2] = {1, 2.5};
union U { long wide; char tiny; };
struct Choice { union U u; int tail; };
struct Choice choice = {13, 14};
int f(int bump) {
    static struct Wrap s = {{{5}, {6,7}}, {[2]=8}, 'S'};
    static int z[2][2][3] = {{{9}}, {[1] = {10,[2]=11}}};
    static int n = {12};
    struct Wrap l = {.v={2,[2]=3}, .a={{4,5},{6}}, .tag='L'};
    s.a[0].x = s.a[0].x + bump;
    z[1][1][2] = z[1][1][2] + bump;
    n = n + bump;
    printf("%d %d %d %d %d %c %d %d %d %d %d %c\n",
           s.a[0].x,s.a[0].y,s.a[1].y,s.v[0],s.v[2],s.tag,
           z[0][0][0],z[1][1][1],z[1][1][2],n,l.a[1].y,l.tag);
    return 0;
}
int main(void) {
    f(1); f(2);
    printf("%ld %d\n",choice.u.wide,choice.tail);
    printf("%d %d %d %d %c %d %d %d %d\n",g.a[0].y,g.a[1].y,g.v[1],g.v[2],g.tag,
           cube[0][1][1],cube[1][1][2],(int)ds[0],(int)ds[1]);
    return 0;
}
