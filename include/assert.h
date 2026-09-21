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
#define assert(e) 0
#else
#define assert(e) do { if (!(e)) { printf("assertion failed: %s\n", #e); \
                                   exit(1); } } while (0)
#endif

#endif
