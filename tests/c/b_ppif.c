/* #if fully macro-expands its line and then evaluates it in intmax_t /
   uintmax_t (C99 6.10.1): hex, octal and character constants, suffixes,
   parenthesised bodies, function-like macros, the unsigned conversion,
   and operands of && || ?: that are never evaluated. */
#include <stdio.h>
int hexok, parok, zok, unsignedok, shiftok, charok;
int shortok, fnok, bigok, opsok, moreok, emptyok;
#define X 0x10
#if X
int h = 1;
#else
int h = 0;
#endif
#define Y (1)
#if Y
int p = 1;
#else
int p = 0;
#endif
#define Z (X * 2 + 1)
#if Z == 33 && defined(X) && !defined Q
int z = 1;
#else
int z = 0;
#endif
#if -1 > 0u
int u = 1;
#else
int u = 0;
#endif
#if (-1 >> 1) == -1 && 0x7fffffffffffffff + 0 > 0
int s = 1;
#else
int s = 0;
#endif
#if '\x41' == 65 && '\n' == 10 && 'a' == 97 && 017 == 15 && '\101' == 65
int c = 1;
#else
int c = 0;
#endif
#if 0 && (1 / 0)
int sc = 0;
#else
int sc = 1;
#endif
#define M(a, b) ((a) > (b) ? (a) : (b))
#if M(3, 7) == 7 && M(9, 4) == 9
int f = 1;
#else
int f = 0;
#endif
#if 18446744073709551615u / 2 == 9223372036854775807
int b = 1;
#else
int b = 0;
#endif
#if (2 || 1/0) && (1 ? 5 : 1/0) == 5 && ~0 == -1 && (7 % 3) == 1 && -7 / 2 == -3 && -7 % 2 == -1
int o = 1;
#else
int o = 0;
#endif
#if 1L + 2UL == 3 && 10 >= 10 && (3 <= 2) == 0 && (5 & 3) == 1 && (5 ^ 3) == 6 && (5 | 3) == 7
int m = 1;
#else
int m = 0;
#endif
#define E
#if defined E && UNDEFINED_NAME == 0 && (E 1) == 1
int e = 1;
#else
int e = 0;
#endif
#define LVL 2
#if LVL == 1
int lv = 1;
#elif LVL == 0x2
int lv = 2;
#else
int lv = 3;
#endif
#if -1 < 0u ? 0 : 1
int t = 1;
#else
int t = 0;
#endif
int main(void) {
    printf("%d %d %d %d %d %d\n", h, p, z, u, s, c);
    printf("%d %d %d %d %d %d %d %d\n", sc, f, b, o, m, e, lv, t);
    return 0;
}
