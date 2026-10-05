/* <assert.h> for the unisa C subset.
 *
 * The message carries the expression but NOT the file and line: this
 * preprocessor has no `__FILE__` or `__LINE__`, because includes are spliced
 * inline without line markers, so both would be wrong after the first
 * `#include`.  A wrong line number is worse than none. */
#ifndef _UNISA_ASSERT_H
#define _UNISA_ASSERT_H
#include <stdio.h>
#include <stdlib.h>

#undef assert
#ifdef NDEBUG
#define assert(e) ((void)0)
#else
/* an expression, not a statement (C99 7.2.1.1): SQLite's shell writes `x ? 1 : (assert(0), 0)` */
#define assert(e) ((e) ? (void)0 : (printf("assertion failed: %s\n", #e), exit(1)))
#endif

#endif
