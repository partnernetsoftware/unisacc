/* <stdarg.h> for the unisa C subset.  A variadic function takes all of its
 * arguments on the tape stack, arg[k] at [FP + 16 + 8k], so a va_list is a
 * pointer to the next slot and the macros are compiler intrinsics. */
#ifndef _UNISA_STDARG_H
#define _UNISA_STDARG_H
typedef char *va_list;
/* A va_list here is a pointer to the next argument slot, so copying one is
 * an assignment.  C99 requires the macro; it requires va_end on the copy
 * too, which for this representation is a no-op the compiler folds. */
#define va_copy(d, s) ((d) = (s))
#define __va_copy(d, s) ((d) = (s))
#endif
