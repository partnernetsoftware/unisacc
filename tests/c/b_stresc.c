/* Escape decoding stops at a literal boundary; numeric results are not
   reinterpreted as another escape (octal 162='r', 170='x'). */
#include <stdio.h>
char *p = "\x1" "f" "\x000041" "\162\170" "\a\b\f\v";
int main(void) {
    char q[] = "\101" "7" "\x0" "F";
    int i;
    for (i = 0; i < 9; i++) printf("%d ", (unsigned char)p[i]);
    for (i = 0; i < 5; i++) printf("%d ", (unsigned char)q[i]);
    printf("\n");
    return 0;
}
