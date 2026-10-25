/* #if over a real constant expression, octal escapes, braced scalars,
   designators that move the cursor */
#define N 3
#if defined(N) && N > 2 && !defined(NOPE)
int ok = 1;
#elif !defined(N)
int ok = 0;
#else
int ok = 0;
#endif
#if (N << 1) == 6 && (N == 3 ? 1 : 0)
int ok2 = 1;
#else
int ok2 = 0;
#endif
int a[] = {5, [2] = 2, 3};
struct S { int p; int q; };
struct S s = ((struct S){7, 8});
int braced = {9};
int main() {
    printf("%d %d %d %d %d %d %d\n", ok, ok2, (int)(sizeof(a) / sizeof(a[0])),
           a[2], a[3], s.q, braced);
    printf("%d %d %d %d %d [%s]\n", '\1', '\10', '\100', '\x01', '\x0e',
           "a\102c\x64");
    return 0;
}
