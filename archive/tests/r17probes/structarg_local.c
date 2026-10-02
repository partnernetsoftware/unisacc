/* R17-8: string-initialised local struct passed by value. */
#include <stdio.h>
struct pair { char x[2]; };
int show(struct pair p) {
    printf("%d %d\n", (int)p.x[0], (int)p.x[1]);
    return 0;
}
int main(void) {
    struct pair local = {"12"};
    return show(local);
}
