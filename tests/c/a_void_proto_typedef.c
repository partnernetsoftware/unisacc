/* a void prototype must not leak its void-ness into the next declarator: typedef objects after it stay objects */
typedef int Scalar;
typedef struct { int a; } Model;
typedef void *VoidPtr;
typedef int (*Fn)(int);

void f(void);
static int twice(int x) { return x * 2; }

int main(void) {
    static Scalar s = 4;
    static Model m = { 5 };
    VoidPtr p = &s;
    Fn g = twice;
    if (*(Scalar *)p != 4) return 1;
    if (m.a != 5) return 2;
    if (g(3) != 6) return 3;
    return 0;
}
