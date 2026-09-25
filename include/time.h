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
typedef long time_t;
typedef long clock_t;
struct tm {
    int tm_sec; int tm_min; int tm_hour; int tm_mday; int tm_mon;
    int tm_year; int tm_wday; int tm_yday; int tm_isdst;
};
static double difftime(time_t __u_a, time_t __u_b) { return (double)(__u_a - __u_b); }
#endif
