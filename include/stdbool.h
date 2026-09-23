/* <stdbool.h> for the unisa C subset.  C99 7.16: three macros over _Bool,
 * which the compiler now has as a one-byte unsigned type whose conversion
 * rule is "0 if the value compares equal to 0, 1 otherwise" -- so
 * `bool b = 2;` stores 1, as the standard requires. */
#ifndef _UNISA_STDBOOL_H
#define _UNISA_STDBOOL_H
#define bool _Bool
#define true 1
#define false 0
#define __bool_true_false_are_defined 1
#endif
