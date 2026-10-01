/* sys/socket.h -- 0.0.19 R19-1 (dsh: the harness's HTTP/SSE).  Linux and
 * macOS through the generic gate; constants and sockaddr layouts per OS (macOS
 * sockaddr starts with a length byte).  Not on Windows yet (Winsock: 0.0.20). */
#ifndef _UNISA_SYS_SOCKET_H
#define _UNISA_SYS_SOCKET_H
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
typedef unsigned int socklen_t;
typedef unsigned short sa_family_t;
#ifdef __APPLE__
struct sockaddr { unsigned char sa_len; unsigned char sa_family; char sa_data[14]; };
struct sockaddr_storage { unsigned char ss_len; unsigned char ss_family; char __ss_pad[126]; };
#define AF_INET6 30
#define SOL_SOCKET 0xffff
#define SO_REUSEADDR 0x0004
#define SO_KEEPALIVE 0x0008
#define SO_RCVBUF 0x1002
#define SO_SNDBUF 0x1001
#define SO_ERROR 0x1007
#define MSG_NOSIGNAL 0
#else
struct sockaddr { sa_family_t sa_family; char sa_data[14]; };
struct sockaddr_storage { sa_family_t ss_family; char __ss_pad[126]; };
#define AF_INET6 10
#define SOL_SOCKET 1
#define SO_REUSEADDR 2
#define SO_KEEPALIVE 9
#define SO_RCVBUF 8
#define SO_SNDBUF 7
#define SO_ERROR 4
#define MSG_NOSIGNAL 0x4000
#endif
#define AF_UNSPEC 0
#define AF_UNIX 1
#define AF_INET 2
#define PF_INET AF_INET
#define SOCK_STREAM 1
#define SOCK_DGRAM 2
#define SHUT_RD 0
#define SHUT_WR 1
#define SHUT_RDWR 2
#define MSG_PEEK 2
#ifndef _WIN32
#ifdef __APPLE__
#define _UNISA_NR_socket 97
#define _UNISA_NR_connect 98
#define _UNISA_NR_bind 104
#define _UNISA_NR_listen 106
#define _UNISA_NR_accept 30
#define _UNISA_NR_sendto 133
#define _UNISA_NR_recvfrom 29
#define _UNISA_NR_setsockopt 105
#define _UNISA_NR_getsockopt 118
#define _UNISA_NR_getsockname 32
#define _UNISA_NR_getpeername 31
#define _UNISA_NR_shutdown 134
#elif defined(__x86_64__)
#define _UNISA_NR_socket 41
#define _UNISA_NR_connect 42
#define _UNISA_NR_bind 49
#define _UNISA_NR_listen 50
#define _UNISA_NR_accept 43
#define _UNISA_NR_sendto 44
#define _UNISA_NR_recvfrom 45
#define _UNISA_NR_setsockopt 54
#define _UNISA_NR_getsockopt 55
#define _UNISA_NR_getsockname 51
#define _UNISA_NR_getpeername 52
#define _UNISA_NR_shutdown 48
#else
#define _UNISA_NR_socket 198
#define _UNISA_NR_connect 203
#define _UNISA_NR_bind 200
#define _UNISA_NR_listen 201
#define _UNISA_NR_accept 202
#define _UNISA_NR_sendto 206
#define _UNISA_NR_recvfrom 207
#define _UNISA_NR_setsockopt 208
#define _UNISA_NR_getsockopt 209
#define _UNISA_NR_getsockname 204
#define _UNISA_NR_getpeername 205
#define _UNISA_NR_shutdown 210
#endif
/* the raw call; each body names _unisa_ret itself, so the on-demand library
   sees the dependency (a macro hiding it left socket() unlinkable unless
   another header had pulled _unisa_ret in -- dsh, 0.0.19) */
#define _UNISA_SYSC(nr, a, b, c, d, e) __syscall6(_UNISA_SC(nr), (long)(a), (long)(b), (long)(c), (long)(d), (long)(e))
#if !__UNISA_FTRIM_LIBC || __UN_socket
static int socket(int __u_d, int __u_t, int __u_p) { return (int)_unisa_ret(_UNISA_SYSC(_UNISA_NR_socket, __u_d, __u_t, __u_p, 0, 0)); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_connect
static int connect(int __u_s, const struct sockaddr *__u_a, socklen_t __u_l) { return (int)_unisa_ret(_UNISA_SYSC(_UNISA_NR_connect, __u_s, __u_a, __u_l, 0, 0)); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_bind
static int bind(int __u_s, const struct sockaddr *__u_a, socklen_t __u_l) { return (int)_unisa_ret(_UNISA_SYSC(_UNISA_NR_bind, __u_s, __u_a, __u_l, 0, 0)); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_listen
static int listen(int __u_s, int __u_n) { return (int)_unisa_ret(_UNISA_SYSC(_UNISA_NR_listen, __u_s, __u_n, 0, 0, 0)); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_accept
static int accept(int __u_s, struct sockaddr *__u_a, socklen_t *__u_l) { return (int)_unisa_ret(_UNISA_SYSC(_UNISA_NR_accept, __u_s, __u_a, __u_l, 0, 0)); }
#endif
/* sendto/recvfrom take six arguments; the gate carries five, so they go
   through sendmsg/recvmsg (three), with the per-OS struct msghdr below */
struct iovec { void *iov_base; size_t iov_len; };
#ifdef __APPLE__
struct msghdr { void *msg_name; socklen_t msg_namelen; struct iovec *msg_iov; int msg_iovlen; void *msg_control; socklen_t msg_controllen; int msg_flags; };
#define _UNISA_NR_sendmsg 28
#define _UNISA_NR_recvmsg 27
#elif defined(__x86_64__)
struct msghdr { void *msg_name; socklen_t msg_namelen; struct iovec *msg_iov; size_t msg_iovlen; void *msg_control; size_t msg_controllen; int msg_flags; };
#define _UNISA_NR_sendmsg 46
#define _UNISA_NR_recvmsg 47
#else
struct msghdr { void *msg_name; socklen_t msg_namelen; struct iovec *msg_iov; size_t msg_iovlen; void *msg_control; size_t msg_controllen; int msg_flags; };
#define _UNISA_NR_sendmsg 211
#define _UNISA_NR_recvmsg 212
#endif
#if !__UNISA_FTRIM_LIBC || __UN_sendmsg
static long sendmsg(int __u_s, const struct msghdr *__u_m, int __u_f) { return _unisa_ret(_UNISA_SYSC(_UNISA_NR_sendmsg, __u_s, __u_m, __u_f, 0, 0)); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_sendto
static long sendto(int __u_s, const void *__u_b, size_t __u_n, int __u_f, const struct sockaddr *__u_a, socklen_t __u_l) {
    struct msghdr __u_m; struct iovec __u_v;
    __u_v.iov_base = (void *)__u_b; __u_v.iov_len = __u_n;
    __u_m.msg_name = (void *)__u_a; __u_m.msg_namelen = __u_a ? __u_l : 0; __u_m.msg_iov = &__u_v; __u_m.msg_iovlen = 1;
    __u_m.msg_control = 0; __u_m.msg_controllen = 0; __u_m.msg_flags = 0;
    return sendmsg(__u_s, &__u_m, __u_f);
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_send
static long send(int __u_s, const void *__u_b, size_t __u_n, int __u_f) { return sendto(__u_s, __u_b, __u_n, __u_f, 0, 0); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_recvmsg
static long recvmsg(int __u_s, struct msghdr *__u_m, int __u_f) { return _unisa_ret(_UNISA_SYSC(_UNISA_NR_recvmsg, __u_s, __u_m, __u_f, 0, 0)); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_recvfrom
static long recvfrom(int __u_s, void *__u_b, size_t __u_n, int __u_f, struct sockaddr *__u_a, socklen_t *__u_l) {
    struct msghdr __u_m; struct iovec __u_v; long __u_r;
    __u_v.iov_base = __u_b; __u_v.iov_len = __u_n;
    __u_m.msg_name = __u_a; __u_m.msg_namelen = (__u_a && __u_l) ? *__u_l : 0; __u_m.msg_iov = &__u_v; __u_m.msg_iovlen = 1;
    __u_m.msg_control = 0; __u_m.msg_controllen = 0; __u_m.msg_flags = 0;
    __u_r = recvmsg(__u_s, &__u_m, __u_f);
    if (__u_r >= 0 && __u_a && __u_l) *__u_l = __u_m.msg_namelen;
    return __u_r;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_recv
static long recv(int __u_s, void *__u_b, size_t __u_n, int __u_f) { return recvfrom(__u_s, __u_b, __u_n, __u_f, 0, 0); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_setsockopt
static int setsockopt(int __u_s, int __u_lv, int __u_o, const void *__u_v, socklen_t __u_l) { return (int)_unisa_ret(_UNISA_SYSC(_UNISA_NR_setsockopt, __u_s, __u_lv, __u_o, __u_v, __u_l)); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_getsockopt
static int getsockopt(int __u_s, int __u_lv, int __u_o, void *__u_v, socklen_t *__u_l) { return (int)_unisa_ret(_UNISA_SYSC(_UNISA_NR_getsockopt, __u_s, __u_lv, __u_o, __u_v, __u_l)); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_getsockname
static int getsockname(int __u_s, struct sockaddr *__u_a, socklen_t *__u_l) { return (int)_unisa_ret(_UNISA_SYSC(_UNISA_NR_getsockname, __u_s, __u_a, __u_l, 0, 0)); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_getpeername
static int getpeername(int __u_s, struct sockaddr *__u_a, socklen_t *__u_l) { return (int)_unisa_ret(_UNISA_SYSC(_UNISA_NR_getpeername, __u_s, __u_a, __u_l, 0, 0)); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_shutdown
static int shutdown(int __u_s, int __u_h) { return (int)_unisa_ret(_UNISA_SYSC(_UNISA_NR_shutdown, __u_s, __u_h, 0, 0, 0)); }
#endif
#endif
#endif
