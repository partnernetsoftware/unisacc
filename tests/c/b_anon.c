/* anonymous struct/union members, function addresses, calls through values */
typedef int (*binop)(int, int);
struct S {
    int a;
    union { int b1; int b2; };
    struct { int c; int d; };
};
int add(int a, int b) { return a + b; }
binop pick(void) { return add; }
struct T { binop f; };
struct T t = {add};
struct T *get(void) { return &t; }
int main() {
    struct S s;
    s.a = 1;
    s.b1 = 2;
    s.c = 3;
    s.d = 4;
    printf("%d %d %d %d %d %d\n", s.a, s.b1, s.b2, s.c, s.d, (int)sizeof(s));
    printf("%d %d\n", pick()(3, 4), get()->f(5, 6));
    return 0;
}
