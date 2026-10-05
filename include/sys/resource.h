/* sys/resource.h -- 0.0.28 H2.  getrlimit/setrlimit/getrusage/getpriority/setpriority forward to the
 * system C library on Linux and macOS; the RLIMIT_* numbers and RLIM_INFINITY follow each OS. */
#ifndef _UNISA_SYS_RESOURCE_H
#define _UNISA_SYS_RESOURCE_H
#include <sys/types.h>
#include <sys/time.h>
#ifndef _WIN32
typedef unsigned long long rlim_t;
struct rlimit { rlim_t rlim_cur; rlim_t rlim_max; };
struct rusage { struct timeval ru_utime; struct timeval ru_stime; long ru_maxrss; long ru_ixrss; long ru_idrss;
                long ru_isrss; long ru_minflt; long ru_majflt; long ru_nswap; long ru_inblock; long ru_oublock;
                long ru_msgsnd; long ru_msgrcv; long ru_nsignals; long ru_nvcsw; long ru_nivcsw; };
#define RLIMIT_CPU   0
#define RLIMIT_FSIZE 1
#define RLIMIT_DATA  2
#define RLIMIT_STACK 3
#define RLIMIT_CORE  4
#ifdef __APPLE__
#define RLIMIT_AS      5
#define RLIMIT_RSS     5
#define RLIMIT_MEMLOCK 6
#define RLIMIT_NPROC   7
#define RLIMIT_NOFILE  8
#define RLIM_INFINITY  ((rlim_t)0x7fffffffffffffffULL)
#else
#define RLIMIT_RSS     5
#define RLIMIT_NPROC   6
#define RLIMIT_NOFILE  7
#define RLIMIT_MEMLOCK 8
#define RLIMIT_AS      9
#define RLIM_INFINITY  ((rlim_t)-1)
#endif
#define RUSAGE_SELF     0
#define RUSAGE_CHILDREN (-1)
#define PRIO_PROCESS 0
#define PRIO_PGRP    1
#define PRIO_USER    2
int getrlimit(int __u_res, struct rlimit *__u_rl);
int setrlimit(int __u_res, const struct rlimit *__u_rl);
int getrusage(int __u_who, struct rusage *__u_ru);
int getpriority(int __u_which, unsigned int __u_who);
int setpriority(int __u_which, unsigned int __u_who, int __u_prio);
#endif
#endif
