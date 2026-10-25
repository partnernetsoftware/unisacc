/* declarator corners: qualifiers in brackets, (* const x), [*], casts in
   constant expressions, empty macro arguments */
#define PICK(a, b) a
typedef int myint;
myint gx = (myint)7;
void foo(int[5]);
void fooc(int x[const 5]);
void foos(int x[static 5]);
void foop(int (*const x));
void foovm(int x[const *]);
int f1(char *s);
int main() {
    int f2(char *);
    int v;
    v = PICK(, 5) 3;            /* an empty argument expands to nothing */
    printf("%d %d %d %d\n", gx, v, f1("a"), f2("b"));
    return 0;
}
void foo(int a[5]) { }
void fooc(int x[const 5]) { }
void foos(int x[static 5]) { }
void foop(int (*const x)) { }
void foovm(int x[const 5]) { }
int f1(char *s) { return 1; }
int f2(char *s) { return 2; }
