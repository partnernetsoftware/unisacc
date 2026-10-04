/* netdb.h -- 0.0.19 R19-1.  getaddrinfo resolves numeric IPv4 addresses,
 * "localhost" and names listed in /etc/hosts; it does NOT query DNS (no
 * resolver is bundled): other names fail with EAI_NONAME, stated rather than
 * faked.  struct addrinfo per OS: macOS puts ai_canonname before ai_addr. */
#ifndef _UNISA_NETDB_H
#define _UNISA_NETDB_H
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#ifdef __APPLE__
struct addrinfo { int ai_flags; int ai_family; int ai_socktype; int ai_protocol; socklen_t ai_addrlen; char *ai_canonname; struct sockaddr *ai_addr; struct addrinfo *ai_next; };
#define EAI_NONAME 8
#define EAI_FAMILY 5
#define EAI_MEMORY 6
#define EAI_SERVICE 9
#define AI_PASSIVE 1
#define AI_NUMERICHOST 4
#else
struct addrinfo { int ai_flags; int ai_family; int ai_socktype; int ai_protocol; socklen_t ai_addrlen; struct sockaddr *ai_addr; char *ai_canonname; struct addrinfo *ai_next; };
#define EAI_NONAME (-2)
#define EAI_FAMILY (-6)
#define EAI_MEMORY (-10)
#define EAI_SERVICE (-8)
#define AI_PASSIVE 1
#define AI_NUMERICHOST 4
#endif
#ifndef _WIN32
/* 0.0.25 N1: on Linux and macOS these are prototypes with no body, so both routes forward them to
 * the system C library (src/fwdstub.c): real DNS -- A/AAAA, search domains, mDNS -- with the
 * system's struct addrinfo, which the per-OS layout above matches.  Windows keeps the bundled
 * numeric/localhost//etc/hosts resolver below (the product refuses forwarding there, prd 3.12). */
int getaddrinfo(const char *__u_node, const char *__u_service, const struct addrinfo *__u_hints, struct addrinfo **__u_res);
void freeaddrinfo(struct addrinfo *__u_ai);
const char *gai_strerror(int __u_e);
#else
#if !__UNISA_FTRIM_LIBC || __UN__unisa_hosts
static int _unisa_hosts(const char *__u_name, struct in_addr *__u_out) {
    static char __u_buf[65536]; long __u_fd; long __u_n; long __u_i; long __u_ls; char __u_ip[64]; int __u_k;
    if (strcmp(__u_name, "localhost") == 0) { __u_out->s_addr = htonl(INADDR_LOOPBACK); return 1; }
    __u_fd = __open("/etc/hosts", 0, 0); if (__u_fd < 0) return 0;
    __u_n = __read(__u_fd, __u_buf, 65535); __close(__u_fd); if (__u_n <= 0) return 0;
    __u_buf[__u_n] = 0; __u_i = 0;
    while (__u_i < __u_n) {
        __u_ls = __u_i; while (__u_i < __u_n && __u_buf[__u_i] != 10) __u_i = __u_i + 1; __u_buf[__u_i] = 0;
        {   char *__u_p; __u_p = __u_buf + __u_ls; while (*__u_p == 32 || *__u_p == 9) __u_p = __u_p + 1;
            if (*__u_p && *__u_p != 35) {
                __u_k = 0; while (*__u_p && *__u_p != 32 && *__u_p != 9 && __u_k < 63) { __u_ip[__u_k] = *__u_p; __u_k = __u_k + 1; __u_p = __u_p + 1; } __u_ip[__u_k] = 0;
                while (*__u_p) {
                    char *__u_w; while (*__u_p == 32 || *__u_p == 9) __u_p = __u_p + 1; if (*__u_p == 0 || *__u_p == 35) break;
                    __u_w = __u_p; while (*__u_p && *__u_p != 32 && *__u_p != 9) __u_p = __u_p + 1;
                    if ((size_t)(__u_p - __u_w) == strlen(__u_name) && memcmp(__u_w, __u_name, strlen(__u_name)) == 0 && inet_pton(AF_INET, __u_ip, __u_out) == 1) return 1;
                }
            } }
        __u_i = __u_i + 1;
    }
    return 0;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_getaddrinfo
static int getaddrinfo(const char *__u_node, const char *__u_service, const struct addrinfo *__u_hints, struct addrinfo **__u_res) {
    struct addrinfo *__u_ai; struct sockaddr_in *__u_sa; struct in_addr __u_a; long __u_port; const char *__u_q;
    if (__u_hints && __u_hints->ai_family != AF_UNSPEC && __u_hints->ai_family != AF_INET) return EAI_FAMILY;
    __u_port = 0;
    if (__u_service) { __u_q = __u_service; while (*__u_q) { if (*__u_q < 48 || *__u_q > 57) return EAI_SERVICE; __u_port = __u_port * 10 + (*__u_q - 48); __u_q = __u_q + 1; } if (__u_port > 65535) return EAI_SERVICE; }
    if (__u_node == 0) __u_a.s_addr = (__u_hints && (__u_hints->ai_flags & AI_PASSIVE)) ? INADDR_ANY : htonl(INADDR_LOOPBACK);
    else if (inet_pton(AF_INET, __u_node, &__u_a) != 1) {
        if ((__u_hints && (__u_hints->ai_flags & AI_NUMERICHOST)) || !_unisa_hosts(__u_node, &__u_a)) return EAI_NONAME;
    }
    __u_ai = (struct addrinfo *)calloc(1, sizeof(struct addrinfo) + sizeof(struct sockaddr_in));
    if (__u_ai == 0) return EAI_MEMORY;
    __u_sa = (struct sockaddr_in *)(__u_ai + 1);
#ifdef __APPLE__
    __u_sa->sin_len = sizeof(struct sockaddr_in);
#endif
    __u_sa->sin_family = AF_INET; __u_sa->sin_port = htons((uint16_t)__u_port); __u_sa->sin_addr = __u_a;
    __u_ai->ai_family = AF_INET; __u_ai->ai_socktype = __u_hints ? __u_hints->ai_socktype : 0; __u_ai->ai_protocol = __u_hints ? __u_hints->ai_protocol : 0;
    __u_ai->ai_addrlen = sizeof(struct sockaddr_in); __u_ai->ai_addr = (struct sockaddr *)__u_sa;
    *__u_res = __u_ai; return 0;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_freeaddrinfo
static void freeaddrinfo(struct addrinfo *__u_ai) { free(__u_ai); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_gai_strerror
static const char *gai_strerror(int __u_e) { return __u_e == EAI_NONAME ? "Name not resolved (no DNS resolver is bundled)" : "getaddrinfo failed"; }
#endif
#endif
#endif
