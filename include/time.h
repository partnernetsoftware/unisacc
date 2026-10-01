/* <time.h> for the unisa C subset: the types and the arithmetic, not the
 * clock.  `time` and `clock` need a system call the catalog does not carry
 * on every target (macOS has no clock_gettime syscall -- the abi audit
 * [A-51] found the number that used to stand in for it), so they are not
 * declared here: a program that calls them is refused at compile time
 * rather than handed a made-up time. */
#ifndef _UNISA_TIME_H
#define _UNISA_TIME_H
#include <stddef.h>
#define NULL 0
#define CLOCKS_PER_SEC 1000000
#ifndef _UNISA_TIME_T
#define _UNISA_TIME_T
typedef long time_t;
#endif
typedef long clock_t;
struct tm {
    int tm_sec; int tm_min; int tm_hour; int tm_mday; int tm_mon;
    int tm_year; int tm_wday; int tm_yday; int tm_isdst;
};
/* time (0.0.18 R18-5): seconds from gettimeofday on Linux and macOS (the
   clock note above still holds for clock; Windows has no time yet) */
#if !defined(_WIN32) && (!__UNISA_FTRIM_LIBC || __UN_time)
static time_t time(time_t *__u_t) {
    long __u_tv[2]; long __u_r;
    __u_tv[0] = 0; __u_tv[1] = 0;
    __u_r = __gettimeofday((char *)__u_tv, 0, 0);
    if (__u_r < 0) return (time_t)(0 - 1);
    if (__u_t) *__u_t = __u_tv[0];
    return __u_tv[0];
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_difftime
static double difftime(time_t __u_a, time_t __u_b) { return (double)(__u_a - __u_b); }
#endif
#endif
