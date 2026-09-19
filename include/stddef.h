#ifndef _UNISA_STDDEF_H
#define _UNISA_STDDEF_H
#define NULL 0
typedef long size_t;
typedef long ptrdiff_t;
typedef int wchar_t;
/* offsetof is omitted on purpose: our macro expansion parenthesises an
 * argument that is not a single token, and a parenthesised TYPE NAME is
 * not a type name.  Write `(long)&((T *)0)->m` directly if you need it. */
#endif
