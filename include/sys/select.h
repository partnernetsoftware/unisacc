/* sys/select.h -- 0.0.19 R19-1.  select over poll (one implementation for
 * every target: Linux/arm64 has no select call).  FD_SETSIZE 1024. */
#ifndef _UNISA_SYS_SELECT_H
#define _UNISA_SYS_SELECT_H
#include <sys/time.h>
#include <poll.h>
#include <string.h>
#define FD_SETSIZE 1024
typedef struct { unsigned long fds_bits[FD_SETSIZE / 64]; } fd_set;
#define FD_ZERO(s) memset((s), 0, sizeof(fd_set))
#define FD_SET(d, s) ((s)->fds_bits[(d) / 64] |= (1UL << ((d) % 64)))
#define FD_CLR(d, s) ((s)->fds_bits[(d) / 64] &= ~(1UL << ((d) % 64)))
#define FD_ISSET(d, s) (((s)->fds_bits[(d) / 64] >> ((d) % 64)) & 1UL)
#if !__UNISA_FTRIM_LIBC || __UN_select
static int select(int __u_n, fd_set *__u_r, fd_set *__u_w, fd_set *__u_e, struct timeval *__u_t) {
    static struct pollfd __u_p[FD_SETSIZE]; int __u_m; int __u_d; int __u_rc; int __u_ms; int __u_cnt;
    __u_m = 0; if (__u_n > FD_SETSIZE) __u_n = FD_SETSIZE;
    for (__u_d = 0; __u_d < __u_n; __u_d++) {
        short __u_ev; __u_ev = 0;
        if (__u_r && FD_ISSET(__u_d, __u_r)) __u_ev = __u_ev | POLLIN;
        if (__u_w && FD_ISSET(__u_d, __u_w)) __u_ev = __u_ev | POLLOUT;
        if (__u_e && FD_ISSET(__u_d, __u_e)) __u_ev = __u_ev | POLLPRI;
        if (__u_ev) { __u_p[__u_m].fd = __u_d; __u_p[__u_m].events = __u_ev; __u_p[__u_m].revents = 0; __u_m = __u_m + 1; }
    }
    __u_ms = __u_t ? (int)(__u_t->tv_sec * 1000 + __u_t->tv_usec / 1000) : -1;
    __u_rc = poll(__u_p, (unsigned long)__u_m, __u_ms);
    if (__u_rc < 0) return -1;
    if (__u_r) FD_ZERO(__u_r); if (__u_w) FD_ZERO(__u_w); if (__u_e) FD_ZERO(__u_e);
    __u_cnt = 0;
    for (__u_d = 0; __u_d < __u_m; __u_d++) {
        short __u_rv; __u_rv = __u_p[__u_d].revents;
        if (__u_r && (__u_rv & (POLLIN | POLLHUP | POLLERR))) { FD_SET(__u_p[__u_d].fd, __u_r); __u_cnt = __u_cnt + 1; }
        if (__u_w && (__u_rv & (POLLOUT | POLLERR))) { FD_SET(__u_p[__u_d].fd, __u_w); __u_cnt = __u_cnt + 1; }
        if (__u_e && (__u_rv & POLLPRI)) { FD_SET(__u_p[__u_d].fd, __u_e); __u_cnt = __u_cnt + 1; }
    }
    return __u_cnt;
}
#endif
#endif
