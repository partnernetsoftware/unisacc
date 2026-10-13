#ifndef _UNISA_STDDEF_H
#define _UNISA_STDDEF_H
#define NULL 0
/* A shared guard: <sys/types.h> declares the same shorthand types, and two
 * identical typedefs are legal in C11 but were only tolerated in C99 -- so
 * whichever header comes first defines them and the other defers. */
#ifndef _UNISA_SIZE_T
#define _UNISA_SIZE_T
typedef unsigned long size_t;
#endif
typedef long ptrdiff_t;
typedef int wchar_t;
/* offsetof is a compiler intrinsic (__builtin_offsetof reads the struct
 * table), not pointer arithmetic on a null pointer.  It accepts a type name
 * that macro expansion has parenthesised. */
#define offsetof(T, m) __builtin_offsetof(T, m)
#endif
