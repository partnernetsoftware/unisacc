/* GCC attribute spellings, prototypes in a list, pointer-to-array,
   a function-typed parameter */
typedef union U {
    unsigned short u;
    unsigned char b[2];
} __attribute__((packed)) U;
struct S { int a; } __attribute__((aligned(8)));
int f(int a), g(int a), gv;
int f1(int (), int);
static int h(int x) __attribute__((unused));
static int h(int x) { return x + 1; }
int main() {
    char arr[2][4];
    char (*p)[4];
    char *q;
    U v;
    v.u = 258;
    arr[1][2] = 'z';
    p = arr;
    q = p[1];
    gv = 5;
    printf("%c %d %d %d %d %d %d\n", q[2], f(1), g(2), gv, v.b[0], v.b[1],
           h(41));
    return 0;
}
int f(int a) { return a + 10; }
int g(int a) { return a + 20; }
