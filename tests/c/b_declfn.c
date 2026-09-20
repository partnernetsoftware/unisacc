/* The declarator grammar nests: a function may RETURN a function pointer,
   an array may hold them, and a cast may name one with no identifier. */
int add2(int a, int b) { return a + b; }
int mul2(int a, int b) { return a * b; }

int (*pick(int which))(int, int) { return which ? mul2 : add2; }

typedef int (*binop)(int, int);
int apply(binop f, int a, int b) { return f(a, b); }
int table(int (*fs[2])(int, int), int i) { return fs[i](3, 4); }

int main(void) {
    int (*fs[2])(int, int);
    int (*p)(int, int);
    int (*q[2])(int, int) = { add2, mul2 };
    fs[0] = add2;
    fs[1] = mul2;
    p = pick(1);
    printf("%d %d %d\n", p(3, 4), pick(0)(3, 4), table(fs, 0));
    printf("%d %d\n", apply(mul2, 5, 6), ((int (*)(int, int)) q[1])(2, 8));
    return 0;
}
