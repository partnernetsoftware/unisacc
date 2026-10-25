/* typedef is a storage class, so it is a declaration and may appear inside
   a block -- and an inner one shadows an outer name of the same spelling.
   A typedef name can only START a specifier list; the second one is the
   thing being declared. */
#include <stdio.h>
typedef int h;
int main(void) {
    h a = 1;
    {
        typedef enum { e, f, g } h;
        h b = g;
        printf("%d %d %d\n", (int)b, e, f);
    }
    { h c = 9; printf("%d %d\n", a, c); }
    return 0;
}
