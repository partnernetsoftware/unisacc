/* regex.h -- 0.0.28 H2.  POSIX regcomp/regexec/regerror/regfree forward to the system C library on
 * Linux and macOS.  regex_t is opaque storage of each OS's size; regoff_t is int on glibc and off_t
 * on macOS; REG_NOSUB and REG_NEWLINE swap bits between the two. */
#ifndef _UNISA_REGEX_H
#define _UNISA_REGEX_H
#include <stddef.h>
#ifndef _WIN32
#ifdef __APPLE__
typedef long regoff_t;
typedef struct { int re_magic; size_t re_nsub; const char *re_endp; void *re_g; } regex_t;
#define REG_NOSUB   0x0004
#define REG_NEWLINE 0x0008
#else
typedef int regoff_t;
typedef struct { void *__u_buffer; size_t __u_allocated; size_t __u_used; size_t __u_syntax; char *__u_fastmap;
                 unsigned char *__u_translate; size_t re_nsub; unsigned int __u_bits; } regex_t;
#define REG_NOSUB   (1 << 3)
#define REG_NEWLINE (1 << 2)
#endif
typedef struct { regoff_t rm_so; regoff_t rm_eo; } regmatch_t;
#define REG_EXTENDED 1
#define REG_ICASE    2
#define REG_NOTBOL   1
#define REG_NOTEOL   2
#define REG_NOMATCH  1
int regcomp(regex_t *__u_re, const char *__u_pattern, int __u_cflags);
int regexec(const regex_t *__u_re, const char *__u_string, size_t __u_nmatch, regmatch_t *__u_m, int __u_eflags);
size_t regerror(int __u_err, const regex_t *__u_re, char *__u_buf, size_t __u_size);
void regfree(regex_t *__u_re);
#endif
#endif
