#ifndef _UNISA_STDDEF_H
#define _UNISA_STDDEF_H
#define NULL 0
/* A shared guard: <sys/types.h> declares the same shorthand types, and two
 * identical typedefs are legal in C11 but were only tolerated in C99 -- so
 * whichever header comes first defines them and the other defers. */
#ifndef _UNISA_SIZE_T
#define _UNISA_SIZE_T
typedef long size_t;
#endif
typedef long ptrdiff_t;
typedef int wchar_t;
/* offsetof is omitted on purpose: our macro expansion parenthesises an
 * argument that is not a single token, and a parenthesised TYPE NAME is
 * not a type name.  Write `(long)&((T *)0)->m` directly if you need it. */
#endif
