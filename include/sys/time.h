/* sys/time.h -- 0.0.18 R18-5: struct timeval for programs that include the
 * header (kilo).  0.0.28 H4: gettimeofday and utimes are forwarded to the system C library on Linux
 * and macOS (SQLite's shell). */
#ifndef _UNISA_SYS_TIME_H
#define _UNISA_SYS_TIME_H
#include <time.h>
struct timeval { long tv_sec; long tv_usec; };
/* 0.0.28 H4: forwarded to the system C library on Linux and macOS */
#ifndef _WIN32
int utimes(const char *__u_path, const struct timeval __u_t[2]);
int gettimeofday(struct timeval *__u_tv, void *__u_tz);
#endif
/* 0.0.34 L2 (SQLite's Apple path): forwarded to the system C library */
#ifdef __APPLE__
int futimes(int __u_fd, const struct timeval __u_tv[2]);
#endif
#endif
