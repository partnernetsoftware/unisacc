/* <locale.h> for the unisa C subset.  There is exactly one locale, the C
 * locale: this is not a limitation to be worked around, it is what a compiler
 * with no locale database can honestly offer.  The category numbers are
 * glibc's, so a program that passes LC_ALL to setlocale behaves the same way
 * it would there. */
#ifndef _UNISA_LOCALE_H
#define _UNISA_LOCALE_H

#include <limits.h>     /* CHAR_MAX, for the lconv fields that mean "no value" */

#define LC_CTYPE     0
#define LC_NUMERIC   1
#define LC_TIME      2
#define LC_COLLATE   3
#define LC_MONETARY  4
#define LC_ALL       6

struct lconv {
    char *decimal_point;
    char *thousands_sep;
    char *grouping;
    char *int_curr_symbol;
    char *currency_symbol;
    char *mon_decimal_point;
    char *mon_thousands_sep;
    char *mon_grouping;
    char *positive_sign;
    char *negative_sign;
    char int_frac_digits;
    char frac_digits;
    char p_cs_precedes;
    char p_sep_by_space;
    char n_cs_precedes;
    char n_sep_by_space;
    char p_sign_posn;
    char n_sign_posn;
    char int_p_cs_precedes;
    char int_p_sep_by_space;
    char int_n_cs_precedes;
    char int_n_sep_by_space;
    char int_p_sign_posn;
    char int_n_sign_posn;
};

#if !__UNISA_FTRIM_LIBC || __UN_setlocale
static char *setlocale(int __u_cat, const char *__u_loc) {
    (void)__u_cat;
    /* NULL asks "what is it now", and "" asks for the environment's choice.
     * With no locale database the answer is the C locale either way; any name
     * that is not the C locale is a request this library cannot honour, and C
     * says a failed setlocale returns NULL. */
    if (__u_loc == 0) return "C";
    if (__u_loc[0] == 0) return "C";
    if (__u_loc[0] == 'C' && __u_loc[1] == 0) return "C";
    if (__u_loc[0] == 'P' && __u_loc[1] == 'O' && __u_loc[2] == 'S'
        && __u_loc[3] == 'I' && __u_loc[4] == 'X' && __u_loc[5] == 0) return "C";
    return 0;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_localeconv
static struct lconv *localeconv(void) {
    /* The C locale: a decimal point and nothing else.  CHAR_MAX in a char
     * field means "this information is not available", which is exactly true
     * here, and it is what the standard prescribes for the C locale. */
    static struct lconv __u_lc;
    static int __u_done;
    if (!__u_done) {
        __u_lc.decimal_point = ".";
        __u_lc.thousands_sep = "";
        __u_lc.grouping = "";
        __u_lc.int_curr_symbol = "";
        __u_lc.currency_symbol = "";
        __u_lc.mon_decimal_point = "";
        __u_lc.mon_thousands_sep = "";
        __u_lc.mon_grouping = "";
        __u_lc.positive_sign = "";
        __u_lc.negative_sign = "";
        __u_lc.int_frac_digits = CHAR_MAX;
        __u_lc.frac_digits = CHAR_MAX;
        __u_lc.p_cs_precedes = CHAR_MAX;
        __u_lc.p_sep_by_space = CHAR_MAX;
        __u_lc.n_cs_precedes = CHAR_MAX;
        __u_lc.n_sep_by_space = CHAR_MAX;
        __u_lc.p_sign_posn = CHAR_MAX;
        __u_lc.n_sign_posn = CHAR_MAX;
        __u_lc.int_p_cs_precedes = CHAR_MAX;
        __u_lc.int_p_sep_by_space = CHAR_MAX;
        __u_lc.int_n_cs_precedes = CHAR_MAX;
        __u_lc.int_n_sep_by_space = CHAR_MAX;
        __u_lc.int_p_sign_posn = CHAR_MAX;
        __u_lc.int_n_sign_posn = CHAR_MAX;
        __u_done = 1;
    }
    return &__u_lc;
}
#endif

#endif
