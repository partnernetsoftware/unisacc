/* <stdbool.h> for the unisa C subset.
 *
 * C99 defines `bool` as a macro for `_Bool`, whose conversion rule is that
 * any nonzero value becomes 1.  This compiler does not have `_Bool` as a
 * type yet (tests/c99.knownfail says so, and it is 0.0.6 work), so `bool`
 * is `int` here: `bool b = 2;` stores 2 rather than 1, and `sizeof(bool)`
 * is 4 rather than 1.  Everything written with `bool`, `true` and `false`
 * as booleans behaves the same way.
 */
#ifndef _UNISA_STDBOOL_H
#define _UNISA_STDBOOL_H
#define bool int
#define true 1
#define false 0
#define __bool_true_false_are_defined 1
#endif
