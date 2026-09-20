/* <stdarg.h> for the unisa C subset.  A variadic function takes all of its
 * arguments on the tape stack, arg[k] at [FP + 16 + 8k], so a va_list is a
 * pointer to the next slot and the macros are compiler intrinsics. */
#ifndef _UNISA_STDARG_H
#define _UNISA_STDARG_H
typedef char *va_list;
#endif
