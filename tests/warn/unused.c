#include <stdio.h>
int main(void) {
    int used = 1, unused;
    int *p; char *s = "x";
    p = &used;
    printf("%d\n", used);
    return 0;
}
