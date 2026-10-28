/* <stdint.h> for the unisa C subset.
 *
 * The exact-width names map straight onto the subset's i8/i16/i32/i64 --
 * and the UNSIGNED spellings really are unsigned.  They used to be aliases
 * for the signed ones, on the theory that the width was what mattered; it is
 * not.  `uint8_t b = 0xa2; printf("%.2x", b)` then printed
 * `ffffffffffffffa2`, because the load sign-extended.  tiny-AES-c is written
 * in uint8_t from end to end, and that one typedef was the whole difference
 * between a correct AES block and a screenful of f's. */
#ifndef _UNISA_STDINT_H
#define _UNISA_STDINT_H
typedef char           int8_t;
typedef unsigned char  uint8_t;
typedef short          int16_t;
typedef unsigned short uint16_t;
typedef int            int32_t;
typedef unsigned int   uint32_t;
/* The 64-bit and pointer-sized names are `long` on the LP64 targets and
   `long long` on LLP64 Windows (_WIN64), where `long` is 4 bytes. */
#if defined(_WIN64)
typedef long long          int64_t;
typedef unsigned long long uint64_t;
typedef long long          intptr_t;
typedef unsigned long long uintptr_t;
typedef long long          intmax_t;
typedef unsigned long long uintmax_t;
#else
typedef long           int64_t;
typedef unsigned long  uint64_t;
typedef long           intptr_t;
typedef unsigned long  uintptr_t;
typedef long           intmax_t;
typedef unsigned long  uintmax_t;
#endif

#define INT8_MIN    (-128)
#define INT8_MAX    127
#define UINT8_MAX   255
#define INT16_MIN   (-32768)
#define INT16_MAX   32767
#define UINT16_MAX  65535
#define INT32_MIN   (-2147483647 - 1)
#define INT32_MAX   2147483647
#define UINT32_MAX  4294967295
#define INT64_MIN   (-9223372036854775807 - 1)
#define INT64_MAX   9223372036854775807
/* The UL suffix is not decoration: without it this literal is too large for
   every signed type and the reference refuses to fold the expression at all
   (fb12-08).  gcc's own <stdint.h> carries it for the same reason. */
#if defined(_WIN64)
#define UINT64_MAX  18446744073709551615ULL
#else
#define UINT64_MAX  18446744073709551615UL
#endif

/* C99 7.18 lists more than the INTn/UINTn limits.  These are the rest, all
   written for a 64-bit target as the rest of the subset is.  Only names the
   reference can actually evaluate go in here: no function-like limits beyond
   INTn_C/UINTn_C, which fold to a plain cast of their argument. */
#if defined(_WIN64)
#define SIZE_MAX    18446744073709551615ULL
#define PTRDIFF_MIN (-9223372036854775807LL - 1)
#define PTRDIFF_MAX 9223372036854775807LL
#define INTPTR_MIN  (-9223372036854775807LL - 1)
#define INTPTR_MAX  9223372036854775807LL
#define UINTPTR_MAX 18446744073709551615ULL
#define INTMAX_MIN  (-9223372036854775807LL - 1)
#define INTMAX_MAX  9223372036854775807LL
#define UINTMAX_MAX 18446744073709551615ULL
#define INT64_C(v)  v##LL
#define UINT64_C(v) v##ULL
#else
#define SIZE_MAX    18446744073709551615UL
#define PTRDIFF_MIN (-9223372036854775807L - 1)
#define PTRDIFF_MAX 9223372036854775807L
#define INTPTR_MIN  (-9223372036854775807L - 1)
#define INTPTR_MAX  9223372036854775807L
#define UINTPTR_MAX 18446744073709551615UL
#define INTMAX_MIN  (-9223372036854775807L - 1)
#define INTMAX_MAX  9223372036854775807L
#define UINTMAX_MAX 18446744073709551615UL
#define INT64_C(v)  v##L
#define UINT64_C(v) v##UL
#endif
#define INT8_C(v)   v
#define INT16_C(v)  v
#define INT32_C(v)  v
#define UINT8_C(v)  v
#define UINT16_C(v) v
#define UINT32_C(v) v##U
#endif
