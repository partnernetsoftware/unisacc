/* anonymous union in an initialiser list, zero-arg macros, block prototypes */
#define ZERO_0() 7
#define ZERO_VAR(...) 8
#define ZERO_1_VAR(A, ...) 9
struct S1 { int a; int b; };
struct S2 {
    int a;
    int b;
    union { int c; int d; };
    struct S1 s;
};
struct S2 v = {1, 2, 3, {4, 5}};
int f1(char *s);
int main() {
    int f2(char *);
    printf("%d %d %d %d %d\n", ZERO_0(), ZERO_VAR(1, 2), ZERO_1_VAR(1, 2),
           f1("a"), f2("b"));
    printf("%d %d %d %d %d\n", v.a, v.b, v.c, v.s.a, v.s.b);
    return 0;
}
int f1(char *s) { return 1; }
int f2(char *s) { return 2; }
