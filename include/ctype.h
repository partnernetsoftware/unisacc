/* <ctype.h> for the unisa C subset.  ASCII only: there is no locale, and
 * `char` is signed, so every function takes the value as an int the way C99
 * says and compares it directly. */
#ifndef _UNISA_CTYPE_H
#define _UNISA_CTYPE_H

#if !__UNISA_FTRIM_LIBC || __UN_isdigit
static int isdigit(int __u_c) { return __u_c >= '0' && __u_c <= '9'; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_isxdigit
static int isxdigit(int __u_c) {
    return (__u_c >= '0' && __u_c <= '9') || (__u_c >= 'a' && __u_c <= 'f')
        || (__u_c >= 'A' && __u_c <= 'F');
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_islower
static int islower(int __u_c) { return __u_c >= 'a' && __u_c <= 'z'; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_isupper
static int isupper(int __u_c) { return __u_c >= 'A' && __u_c <= 'Z'; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_isalpha
static int isalpha(int __u_c) { return islower(__u_c) || isupper(__u_c); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_isalnum
static int isalnum(int __u_c) { return isalpha(__u_c) || isdigit(__u_c); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_isspace
static int isspace(int __u_c) {
    return __u_c == ' ' || __u_c == '\t' || __u_c == '\n' || __u_c == '\v'
        || __u_c == '\f' || __u_c == '\r';
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_isblank
static int isblank(int __u_c) { return __u_c == ' ' || __u_c == '\t'; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_iscntrl
static int iscntrl(int __u_c) { return (__u_c >= 0 && __u_c < 32) || __u_c == 127; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_isprint
static int isprint(int __u_c) { return __u_c >= 32 && __u_c < 127; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_isgraph
static int isgraph(int __u_c) { return __u_c > 32 && __u_c < 127; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_ispunct
static int ispunct(int __u_c) { return isgraph(__u_c) && !isalnum(__u_c); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_toupper
static int toupper(int __u_c) { if (islower(__u_c)) return __u_c - 32; return __u_c; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_tolower
static int tolower(int __u_c) { if (isupper(__u_c)) return __u_c + 32; return __u_c; }
#endif

#endif
