/* designated initialisers, address-of initialisers, enum as a type */
struct P { int x; int y; int z; };
enum E { A, B, C = 7, D };
int g = 42;
int *gp = &g;
char gbuf[8] = "hi";
char *gq = gbuf;
struct P a = {.y = 5, .x = 1};
int arr[6] = {[4] = 9, [1] = 3};
enum E ge = C;
int f(int, char *);
int f(int n, char *s) { return n + (int)s[0]; }
int main() {
    struct P b = {.z = 7};
    int c[4] = {[2] = 8};
    enum E e;
    e = D;
    printf("%d %s %d %d %d %d %d %d %d\n", *gp, gq, a.x, a.y, a.z,
           arr[1], arr[4], b.z, c[2]);
    printf("%d %d %d %d\n", (int)A, (int)ge, (int)e, f(1, "A"));
    return 0;
}
