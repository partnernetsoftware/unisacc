/* Struct layout is observable -- through sizeof, offsets and every pointer
   into a struct -- so it has to be the platform's.  A member aligns as its
   OWN type does: an int[3] as an int, a struct as its widest member.  The
   Python walker guessed from the size, put an 8-byte struct of ints and a
   12-byte array on 8-byte boundaries, and made a 28-byte struct 36. */
struct In { int x; int y; };
struct Out { int a; struct In s; int b; int arr[3]; };
struct C3 { char c[3]; };
struct M { char a; struct C3 t; short s; char z; };
struct L { char a; long l; char b; };
struct Sh { short s[3]; char c; };
struct N { char a; struct Sh sh; int i; };
union U { char c[5]; int i; };
struct W { char a; union U u; char b; };
#define OFF(T, m) (int)((char *)&((struct T *)0)->m - (char *)0)
int main(void) {
    printf("%d %d %d %d\n", (int)sizeof(struct Out), OFF(Out, s), OFF(Out, b), OFF(Out, arr));
    printf("%d %d %d %d\n", (int)sizeof(struct M), OFF(M, t), OFF(M, s), OFF(M, z));
    printf("%d %d %d\n", (int)sizeof(struct L), OFF(L, l), OFF(L, b));
    printf("%d %d %d %d\n", (int)sizeof(struct N), OFF(N, sh), OFF(N, i), (int)sizeof(struct Sh));
    printf("%d %d %d %d\n", (int)sizeof(union U), (int)sizeof(struct W), OFF(W, u), OFF(W, b));
    return 0;
}
