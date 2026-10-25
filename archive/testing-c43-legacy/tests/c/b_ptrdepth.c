/* Pointer DEPTH and static locals.  *q of int **q is itself an 8-byte
   pointer; an array of pointers holds 8-byte elements; a static local
   keeps its value across calls.  The self-hosted compiler got all three
   wrong in a way the interpreter hid -- its addresses fit in 32 bits and
   its stack is fresh -- and a native image did not. */
struct N { int v; struct N *next; };
int count(void) { static int n; static int base = 100; n++; return base + n; }
char *names[3] = {"ab", "cd", "ef"};
int main(void) {
    int a; int *p; int **q; int ***r;
    char *s; char **ss; char *arr[2];
    long l; long *lp; long **lpp;
    struct N n1; struct N n2; struct N *np; struct N **npp;
    a = 7; p = &a; q = &p; r = &q;
    **q = 9; printf("%d %d ", a, ***r);
    s = "xyz"; ss = &s; printf("%c %s ", **ss, *ss);
    arr[0] = "hi"; arr[1] = "yo"; printf("%s %s %c ", arr[1], *arr, arr[0][1]);
    l = 1234567890123; lp = &l; lpp = &lp; printf("%ld ", **lpp);
    n1.v = 1; n2.v = 2; n1.next = &n2; np = &n1; npp = &np;
    printf("%d %d ", (*npp)->next->v, np->next->v);
    printf("%s %c ", names[2], *names[1]);
    count(); count();
    printf("%d\n", count());
    return 0;
}
