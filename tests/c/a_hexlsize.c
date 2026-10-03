/* 0.0.23 A3 (cdx F4 follow-up): a hex/octal constant with an l/L suffix stays long --
   the reference had narrowed 0xFFFFFFFFUL to a 4-byte unsigned int. */
#include <stdio.h>
int main(void) {
    printf("%lu %lu %lu %lu\n", sizeof 0xFFFFFFFFUL, sizeof 0xFFFFFFFFU,
           sizeof 037777777777L, sizeof 0xA51754D42E128A9ALL);
    printf("%d\n", (0xFFFFFFFFUL + 1) > 0xFFFFFFFFUL);
    printf("%d %d\n", -1 < 037777777777L, -1 < 0xFFFFFFFF);   /* L stays signed long; bare hex is unsigned int */
    return 0;
}
