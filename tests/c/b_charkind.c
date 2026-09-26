#include <stdio.h>
/* Character constants have type int, even when their contents spell a
   numeric suffix.  Keep real unsigned suffixes as the contrast. */
int main(void) {
    printf("%d %d %d %d %d\n", -1 < 'U', -1 < 'u',
           -1 < 'L', -1 < 'l', -1 < 85U);
    return 0;
}
