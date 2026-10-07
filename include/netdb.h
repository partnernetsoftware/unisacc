/* netdb.h -- 0.0.19 R19-1; 0.0.25 N1 (macOS/Linux) and 0.0.33 W2 (Windows) forward getaddrinfo to the
 * system resolver.  struct addrinfo per OS: macOS and Windows put ai_canonname before ai_addr. */
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
#elif defined(_WIN32)
struct addrinfo { int ai_flags; int ai_family; int ai_socktype; int ai_protocol; size_t ai_addrlen; char *ai_canonname; struct sockaddr *ai_addr; struct addrinfo *ai_next; };
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
/* 0.0.33 W2: Windows forwards too -- getaddrinfo/freeaddrinfo are ws2_32.dll exports (the written
 * image resolves ucrtbase/kernel32/ws2_32 through GetProcAddress, src/main.c:309), so names go to the
 * system resolver (DNS, hosts, search suffixes) instead of the bundled hosts-file reader.  ws2_32 needs
 * WSAStartup before its first call; a constructor does it once (WSAStartup counts, so a program that
 * calls it again stays balanced).  gai_strerrorA is an inline in Microsoft's headers, not an export:
 * it stays a body here.  Windows struct addrinfo: size_t ai_addrlen, ai_canonname before ai_addr. */
#undef EAI_NONAME
#undef EAI_FAMILY
#undef EAI_MEMORY
#undef EAI_SERVICE
#define EAI_NONAME 11001
#define EAI_FAMILY 10047
#define EAI_MEMORY 8
#define EAI_SERVICE 10109
int getaddrinfo(const char *__u_node, const char *__u_service, const struct addrinfo *__u_hints, struct addrinfo **__u_res);
void freeaddrinfo(struct addrinfo *__u_ai);
int WSAStartup(unsigned short __u_version, void *__u_data);
#if !__UNISA_FTRIM_LIBC || __UN_getaddrinfo
__attribute__((constructor)) static void _unisa_wsa_init(void) {
    static long __u_wsadata[64];             /* WSADATA is 408 bytes on Win64 */
    WSAStartup(0x0202, __u_wsadata);
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_gai_strerror
static const char *gai_strerror(int __u_e) {
    return __u_e == EAI_NONAME ? "No such host is known" : __u_e == EAI_SERVICE ? "The specified class was not found"
         : __u_e == EAI_FAMILY ? "Address family not supported" : __u_e == EAI_MEMORY ? "Not enough memory" : "getaddrinfo failed";
}
#endif
#endif
#endif
