int printf(const char *, ...);
int g = 7;
struct S { int x; };
struct S s = {9};
int *p = &g;
int f(int a, int b) { return a + b; }
int main(void) {
    int a = ((g), 246);
    int b = ((s.x), 247);
    int c = (((*p)), 248);
    int d = (g) + 1;
    int e = f((g), 3);
    int h = ((int)g, 249);
    int k = (g) ? 1 : 0;
    printf("%d %d %d %d %d %d %d\n", a, b, c, d, e, h, k);
    return a != 246 || b != 247 || c != 248 || d != 8 || e != 10 || h != 249 || k != 1;
}
