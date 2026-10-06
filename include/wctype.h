/* wctype.h -- 0.0.30 H3.  The wide classification and mapping functions forward to the system
 * C library on Linux and macOS, so each host's own tables (and its "C" locale) decide them. */
#ifndef _UNISA_WCTYPE_H
#define _UNISA_WCTYPE_H
#include <wchar.h>
#ifndef _UNISA_WINT_T
#define _UNISA_WINT_T
typedef unsigned int wint_t;
#endif
#ifndef _WIN32
typedef unsigned long wctype_t;
typedef const int *wctrans_t;
int iswalnum(wint_t __u_c);
int iswalpha(wint_t __u_c);
int iswblank(wint_t __u_c);
int iswcntrl(wint_t __u_c);
int iswdigit(wint_t __u_c);
int iswgraph(wint_t __u_c);
int iswlower(wint_t __u_c);
int iswprint(wint_t __u_c);
int iswpunct(wint_t __u_c);
int iswspace(wint_t __u_c);
int iswupper(wint_t __u_c);
int iswxdigit(wint_t __u_c);
wint_t towlower(wint_t __u_c);
wint_t towupper(wint_t __u_c);
wctype_t wctype(const char *__u_name);
int iswctype(wint_t __u_c, wctype_t __u_t);
wctrans_t wctrans(const char *__u_name);
wint_t towctrans(wint_t __u_c, wctrans_t __u_t);
#endif
#endif
