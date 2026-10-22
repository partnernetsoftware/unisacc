/* 0.0.43: int (**p)(int) and int (**p) declarators keep both pointer levels.  Both the model
   (FPD.moreptr) and the classic reference (fpdecl) skipped the extra stars, so (*p)(x) called p's own
   value: v0.0.42 segfaults.  research/c43-array-pointer-member-miscompile.md */
#include <stdio.h>
static int twice(int x) { return 2 * x; }
static int thrice(int x) { return 3 * x; }
int (*fp)(int) = twice;
int (**gfpp)(int) = &fp;
static int call(int (**p)(int)) { return (*p)(7); }
static void set(int (**p)(int)) { *p = thrice; }
int v = 5;
int *pv = &v;
static int get(int (**p)) { return **p; }
int main(void) {
    int (**fpp)(int) = &fp;
    int (*arr[2])(int) = {twice, thrice};
    int (**q)(int) = arr;
    int (*f)(int) = 0;
    printf("%d %d %d %d\n", (*fpp)(21), (**fpp)(5), (*gfpp)(1), call(&fp));
    printf("%d %d %zu %zu\n", q[1](3), (*(q + 1))(4), sizeof fpp, sizeof *fpp);
    set(&f);
    printf("%d %d\n", f(4), get(&pv));
    return 0;
}
