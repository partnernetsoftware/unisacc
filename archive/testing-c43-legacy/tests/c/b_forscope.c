/* Loop declarations have a scope, including nested loops and shadowed names.
   Conditional initializers allocate labels before the loop labels. */
#include <stdio.h>
typedef long Index;
int main(void) {
    int i = 7, total = 0;
    for (int i = 1 ? 0 : 9; i < 4; i++) {
        if (i == 1) continue;
        for (Index i = 0; i < 2; i++) total += i;
        total += i;
        if (i == 3) break;
    }
    int j = 6;
    for (j = (i == 7 ? 0 : 8); j < 2; j++) total += j;
    printf("%d %d %d\n", i, j, total);
    return i != 7 || j != 2 || total != 9;
}
