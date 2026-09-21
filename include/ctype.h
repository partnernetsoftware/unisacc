/* <ctype.h> for the unisa C subset.  ASCII only: there is no locale, and
 * `char` is signed, so every function takes the value as an int the way C99
 * says and compares it directly. */
#ifndef _UNISA_CTYPE_H
#define _UNISA_CTYPE_H

static int isdigit(int c) { return c >= '0' && c <= '9'; }
static int isxdigit(int c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')
        || (c >= 'A' && c <= 'F');
}
static int islower(int c) { return c >= 'a' && c <= 'z'; }
static int isupper(int c) { return c >= 'A' && c <= 'Z'; }
static int isalpha(int c) { return islower(c) || isupper(c); }
static int isalnum(int c) { return isalpha(c) || isdigit(c); }
static int isspace(int c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\v'
        || c == '\f' || c == '\r';
}
static int isblank(int c) { return c == ' ' || c == '\t'; }
static int iscntrl(int c) { return (c >= 0 && c < 32) || c == 127; }
static int isprint(int c) { return c >= 32 && c < 127; }
static int isgraph(int c) { return c > 32 && c < 127; }
static int ispunct(int c) { return isgraph(c) && !isalnum(c); }
static int toupper(int c) { if (islower(c)) return c - 32; return c; }
static int tolower(int c) { if (isupper(c)) return c + 32; return c; }

#endif
