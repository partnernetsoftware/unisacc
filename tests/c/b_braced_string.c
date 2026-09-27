#include <stdio.h>
/* Character strings in one brace pair, for all three storage durations. */
char gu[] = {"abc"};
char gc[] = {"a" "bc",};
char gz[6] = {"abc",};
char ge[3] = {"abc"};
unsigned char gu8[] = {"abc",};
char *ptrs[] = {"abc"};
int main(void) {
    unsigned char lu8[4] = {"abc"};
    static unsigned char su8[] = {"abc",};
    char lu[] = {"abc"};
    char lc[] = {"a" "bc",};
    char lz[6] = {"abc",};
    char le[3] = {"abc"};
    static char su[] = {"abc"};
    static char sc[] = {"a" "bc",};
    static char sz[6] = {"abc",};
    static char se[3] = {"abc"};
    printf("%ld %d %d %ld %d %d %d\n", (long)sizeof gu, gu[0], gu[3],
           (long)sizeof gc, gc[2], gz[5], ge[2]);
    printf("%ld %d %d %ld %d %d %d\n", (long)sizeof lu, lu[0], lu[3],
           (long)sizeof lc, lc[2], lz[5], le[2]);
    printf("%ld %d %d %ld %d %d %d\n", (long)sizeof su, su[0], su[3],
           (long)sizeof sc, sc[2], sz[5], se[2]);
    printf("%ld %d\n", (long)(sizeof ptrs / sizeof ptrs[0]), ptrs[0][0]);
    printf("%ld %d %d %ld %d\n", (long)sizeof gu8, gu8[0], lu8[3],
           (long)sizeof su8, su8[2]);
    return 0;
}
