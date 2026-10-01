/* arpa/inet.h -- 0.0.19 R19-1: IPv4 text forms (inet_pton/ntop for AF_INET;
 * AF_INET6 is refused with EAFNOSUPPORT, stated rather than half-done). */
#ifndef _UNISA_ARPA_INET_H
#define _UNISA_ARPA_INET_H
#include <netinet/in.h>
#if !__UNISA_FTRIM_LIBC || __UN_inet_pton
static int inet_pton(int __u_af, const char *__u_s, void *__u_dst) {
    unsigned long __u_v; int __u_parts; unsigned long __u_n; int __u_digits; unsigned char __u_b[4];
    if (__u_af != AF_INET) { errno = EAFNOSUPPORT; return -1; }
    __u_parts = 0;
    while (__u_parts < 4) {
        __u_n = 0; __u_digits = 0;
        while (*__u_s >= 48 && *__u_s <= 57) { __u_n = __u_n * 10 + (unsigned long)(*__u_s - 48); __u_digits = __u_digits + 1; __u_s = __u_s + 1; if (__u_digits > 3) return 0; }
        if (__u_digits == 0 || __u_n > 255) return 0;
        __u_b[__u_parts] = (unsigned char)__u_n; __u_parts = __u_parts + 1;
        if (__u_parts < 4) { if (*__u_s != 46) return 0; __u_s = __u_s + 1; }
    }
    if (*__u_s) return 0;
    __u_v = 0; ((unsigned char *)__u_dst)[0] = __u_b[0]; ((unsigned char *)__u_dst)[1] = __u_b[1]; ((unsigned char *)__u_dst)[2] = __u_b[2]; ((unsigned char *)__u_dst)[3] = __u_b[3];
    (void)__u_v; return 1;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_inet_addr
static in_addr_t inet_addr(const char *__u_s) { in_addr_t __u_a; return inet_pton(AF_INET, __u_s, &__u_a) == 1 ? __u_a : INADDR_NONE; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_inet_aton
static int inet_aton(const char *__u_s, struct in_addr *__u_a) { return inet_pton(AF_INET, __u_s, __u_a) == 1; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_inet_ntop
static const char *inet_ntop(int __u_af, const void *__u_src, char *__u_dst, socklen_t __u_size) {
    const unsigned char *__u_p; int __u_i; int __u_n; char __u_t[16]; int __u_v;
    if (__u_af != AF_INET) { errno = EAFNOSUPPORT; return 0; }
    __u_p = (const unsigned char *)__u_src; __u_n = 0;
    for (__u_i = 0; __u_i < 4; __u_i++) {
        __u_v = __u_p[__u_i];
        if (__u_v >= 100) { __u_t[__u_n] = (char)(48 + __u_v / 100); __u_n = __u_n + 1; }
        if (__u_v >= 10) { __u_t[__u_n] = (char)(48 + __u_v / 10 % 10); __u_n = __u_n + 1; }
        __u_t[__u_n] = (char)(48 + __u_v % 10); __u_n = __u_n + 1;
        if (__u_i < 3) { __u_t[__u_n] = 46; __u_n = __u_n + 1; }
    }
    if ((socklen_t)__u_n + 1 > __u_size) { errno = ENOSPC; return 0; }
    for (__u_i = 0; __u_i < __u_n; __u_i++) __u_dst[__u_i] = __u_t[__u_i];
    __u_dst[__u_n] = 0; return __u_dst;
}
#endif
static char _unisa_ntoa[16];
#if !__UNISA_FTRIM_LIBC || __UN_inet_ntoa
static char *inet_ntoa(struct in_addr __u_a) { inet_ntop(AF_INET, &__u_a, _unisa_ntoa, 16); return _unisa_ntoa; }
#endif
#endif
