/* sys/_timespec.h -- 0.0.20 R20-6 (dsh): struct timespec and the one sleep
 * primitive that <time.h> nanosleep and <unistd.h> sleep/usleep share.
 * Linux: the nanosleep system call.  macOS has no nanosleep call, so the
 * wait is select(0, 0, 0, 0, &tv) -- microsecond granularity, stated. */
#ifndef _UNISA_TIMESPEC_H
#define _UNISA_TIMESPEC_H
#ifndef _UNISA_TIME_T
#define _UNISA_TIME_T
typedef long time_t;
#endif
#ifndef _UNISA_TIMESPEC
#define _UNISA_TIMESPEC
struct timespec { long tv_sec; long tv_nsec; };   /* one definition with sys/stat.h */
#endif
#ifndef _WIN32
#if !__UNISA_FTRIM_LIBC || __UN__unisa_nanosleep
static long _unisa_nanosleep(const struct timespec *__u_q) {
#ifdef __APPLE__
    long __u_tv[2];
    __u_tv[0] = __u_q->tv_sec; __u_tv[1] = (__u_q->tv_nsec + 999) / 1000;
#ifdef __x86_64__
    return __syscall6(0x2000000L + 93, 0, 0, 0, 0, (long)__u_tv);
#else
    return __syscall6(93, 0, 0, 0, 0, (long)__u_tv);
#endif
#elif defined(__x86_64__)
    return __syscall6(35, (long)__u_q, 0, 0, 0, 0);
#else
    return __syscall6(101, (long)__u_q, 0, 0, 0, 0);
#endif
}
#endif
#endif
#endif
