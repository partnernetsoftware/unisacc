/* <stdint.h> for the unisa C subset.  The subset has i8/i16/i32/i64, so the
 * exact-width names map straight onto them; the unsigned spellings are the
 * same widths, since the subset's arithmetic is signed throughout. */
#ifndef _UNISA_STDINT_H
#define _UNISA_STDINT_H
typedef char int8_t;
typedef char uint8_t;
typedef short int16_t;
typedef short uint16_t;
typedef int int32_t;
typedef int uint32_t;
typedef long int64_t;
typedef long uint64_t;
typedef long intptr_t;
typedef long uintptr_t;
typedef long intmax_t;
typedef long uintmax_t;
#define INT8_MAX 127
#define INT16_MAX 32767
#define INT32_MAX 2147483647
#endif
