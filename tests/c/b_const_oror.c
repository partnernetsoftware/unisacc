/* || in a constant expression [I1]: the Python front end's folder began at
   the && level, so `int a[1 || 0]` was refused there and accepted in C. */
#include <stdio.h>
int a[1 || 0];
int b[(0 || 2) + 3];
int main(void) {
    printf("%d %d\n", (int)(sizeof a), (int)(sizeof b));
    return 0;
}
