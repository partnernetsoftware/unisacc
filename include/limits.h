/* <limits.h> for the unisa C subset.
 *
 * These are the LIMITS OF THE TYPES, which is what a program asks for.  They
 * are not a description of how this compiler evaluates arithmetic: [G-2] says
 * `int` expressions are evaluated at 64 bits, so `INT_MAX + 1` does not wrap
 * here the way it does elsewhere.  That is a documented deviation of the
 * subset, and it does not change what INT_MAX is. */
#ifndef _UNISA_LIMITS_H
#define _UNISA_LIMITS_H

#define CHAR_BIT    8
#define SCHAR_MIN   (-128)
#define SCHAR_MAX   127
#define UCHAR_MAX   255
#define CHAR_MIN    SCHAR_MIN       /* plain char is signed here */
#define CHAR_MAX    SCHAR_MAX
#define MB_LEN_MAX  1

#define SHRT_MIN    (-32768)
#define SHRT_MAX    32767
#define USHRT_MAX   65535

#define INT_MIN     (-2147483647 - 1)
#define INT_MAX     2147483647
#define UINT_MAX    4294967295

#define LONG_MIN    (-9223372036854775807 - 1)
#define LONG_MAX    9223372036854775807
#define ULONG_MAX   18446744073709551615

#define LLONG_MIN   LONG_MIN
#define LLONG_MAX   LONG_MAX
#define ULLONG_MAX  ULONG_MAX

#endif
