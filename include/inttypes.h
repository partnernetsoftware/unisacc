/* <inttypes.h> for the unisa C subset: the printf/scanf length prefixes
 * C99 added, on top of <stdint.h>'s types.
 *
 * On every target this compiler has, `long` is 64 bits and `int` is 32, so
 * the 64-bit prefix is "l" and the 32-bit one is empty -- the same choice
 * an LP64 platform's own header makes.
 */
#ifndef _UNISA_INTTYPES_H
#define _UNISA_INTTYPES_H
#include <stdint.h>

#define PRId8   "d"
#define PRId16  "d"
#define PRId32  "d"
#define PRId64  "ld"
#define PRIi8   "i"
#define PRIi16  "i"
#define PRIi32  "i"
#define PRIi64  "li"
#define PRIu8   "u"
#define PRIu16  "u"
#define PRIu32  "u"
#define PRIu64  "lu"
#define PRIx8   "x"
#define PRIx16  "x"
#define PRIx32  "x"
#define PRIx64  "lx"
#define PRIX64  "lX"
#define PRIo64  "lo"
#define PRIdPTR "ld"
#define PRIuPTR "lu"
#define PRIxPTR "lx"
#define PRIdMAX "ld"
#define PRIuMAX "lu"

#define SCNd32  "d"
#define SCNd64  "ld"
#define SCNu32  "u"
#define SCNu64  "lu"
#define SCNx64  "lx"

typedef long intmax_t;
typedef unsigned long uintmax_t;
static long imaxabs(long v) { if (v < 0) return 0 - v; return v; }
#endif
