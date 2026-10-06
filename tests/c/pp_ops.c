/* T3' probe: #if operators, #line forms, #pragma push/pop_macro spacing, \u identifiers */
#include <stdio.h>
#if +1 != 0 && 3 - 1 == 2 && !0 && 1 + +2 == 3 && -1 < 0 && (2 != 3) && ~0 == -1
#define OPS 1
#else
#define OPS 0
#endif
#define M 5
#if defined M && defined( M ) && !defined	N && defined  (M)
#define DEF 1
#else
#define DEF 0
#endif
#pragma push_macro( "M" )
#undef M
#define M 6
static int inner = M;
#pragma pop_macro ( "M" )
#pragma  push_macro("M")
#pragma pop_macro("M")
#line 100
static int l100 = __LINE__;
#line   200   "named.c"  
static int l200 = __LINE__;
#line 300 "x.c"
#if (1 ? 2 : 3) == 2 && (8 >> 1) == 4 && (1 << 3) == 8 && 7 % 4 == 3 && (5 | 2) == 7 && (6 ^ 3) == 5 && (6 & 3) == 2 && 2 >= 2 && 3 <= 4 && (0 || 1) && 1 > 0
#define OPS2 1
#else
#define OPS2 0
#endif
#define ID(x) x
static const char *sa = ID("a\"b'"), *sb = ID("\\");
static int ca = ID('\''), cb = ID('"');
static int cmps = 0
#if 1 == 2
 + 1
#endif
#if 2 < 1
 + 2
#endif
#if 1 > 2
 + 4
#endif
#if 2 <= 1
 + 8
#endif
#if 1 >= 2
 + 16
#endif
#if 1 != 1
 + 32
#endif
#if 2 == 2
 + 64
#endif
#if 1 < 2
 + 128
#endif
#if 1 <= 1
 + 256
#endif
#if 2 >= 1
 + 512
#endif
;
#define SUM(...) sum(0, __VA_ARGS__)
#define FIRST(a, ...) a
#define Z() 7
#define ONE(a) (a + 0)
static int sum(int a, int b) { return a + b; }
#if 0
#elif 1 - 1
#elif defined Z
#define EL 1
#else
#define EL 0
#endif
#ifndef	UNSET
#  ifdef   Z
#define NDEF 1
#  endif
#endif
int main(void) {
    int va = SUM(5), fz = Z(), fe = FIRST(9, 1, 2) + EL + NDEF;
    printf("%d %d %d %d ", cmps, va, fz, fe); printf("%d %s %s %d %d %d %d %d %d %d %d\n", OPS2, sa, sb, ca, cb, OPS, DEF, inner, M, l100, l200);
    return 0;
}
