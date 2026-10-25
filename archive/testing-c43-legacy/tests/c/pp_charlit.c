/* T3' probe: #if character literals (octal, hex, simple escapes) and ## pasting */
#include <stdio.h>
#if '\5' == 5 && '\6' == 6 && '\7' == 7 && '\53' == 43 && '\67' == 55
#define OCT 1
#else
#define OCT 0
#endif
#if '\x0' == 0 && '\x1' == 1 && '\x2' == 2 && '\x3' == 3 && '\x5' == 5 && '\x6' == 6 && '\x7' == 7
#define HEX1 1
#else
#define HEX1 0
#endif
#if '\x10' == 16 && '\x21' == 33 && '\x32' == 50 && '\x43' == 67 && '\x54' == 84 && '\x65' == 101 && '\x76' == 118
#define HEX2 1
#else
#define HEX2 0
#endif
#if '\x8' == 8 && '\x9' == 9 && '\xa' == 10 && '\xB' == 11 && '\xc' == 12 && '\xD' == 13 && '\xe' == 14 && '\xF' == 15
#define HEX3 1
#else
#define HEX3 0
#endif
#if '\a' == 7 && '\b' == 8 && '\f' == 12 && '\v' == 11 && '\r' == 13 && '\t' == 9 && '\'' == 39 && '\"' == 34 && '\?' == 63 && '\\' == 92
#define ESC 1
#else
#define ESC 0
#endif
#define CAT(a, b) a ## b
#define CAT3(a, b, c) a ## b ## c
#define STR(x) #x
#define XSTR(x) STR(x)
int main(void) {
    int caf\u00e9 = 2, x\U000000e8 = 3;  /* universal character names in identifiers */
    printf("%d\n", caf\u00e9 + x\U000000e8);
    int CAT(v, 1) = 3, CAT3(w, _, 2) = 4;
    printf("%d %d %d %d %d %d %d %s %s\n", OCT, HEX1, HEX2, HEX3, ESC, v1, w_2, STR('a' "b"), XSTR(CAT(x, 9)));
    return 0;
}
