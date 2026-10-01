/* <poll.h> (0.0.18 R18-10): poll() over the kernel's poll -- Linux/arm64 has
 * only ppoll, so there the timeout becomes a timespec.  Windows: not yet. */
#ifndef _UNISA_POLL_H
#define _UNISA_POLL_H
#include <errno.h>
#ifndef _WIN32
#define POLLIN   0x001
#define POLLPRI  0x002
#define POLLOUT  0x004
#define POLLERR  0x008
#define POLLHUP  0x010
#define POLLNVAL 0x020
typedef unsigned int nfds_t;
struct pollfd { int fd; short events; short revents; };
#ifndef _UNISA_TIMESPEC
#define _UNISA_TIMESPEC
struct timespec { long tv_sec; long tv_nsec; };
#endif
#if !__UNISA_FTRIM_LIBC || __UN_poll
static int poll(struct pollfd *__u_f, nfds_t __u_n, int __u_ms) {
    long __u_r;
#if defined(__linux__) && defined(__aarch64__)
    struct timespec __u_t; __u_t.tv_sec = __u_ms / 1000; __u_t.tv_nsec = (long)(__u_ms % 1000) * 1000000;
    __u_r = __ppoll((char *)__u_f, (long)__u_n, __u_ms < 0 ? (char *)0 : (char *)&__u_t);
#else
    __u_r = __poll((char *)__u_f, (long)__u_n, (long)__u_ms);
#endif
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; } return (int)__u_r;
}
#endif
#endif
#endif
