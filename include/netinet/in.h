/* netinet/in.h -- 0.0.19 R19-1.  sockaddr_in per OS (macOS: length byte first). */
#ifndef _UNISA_NETINET_IN_H
#define _UNISA_NETINET_IN_H
#include <stdint.h>
#include <sys/socket.h>
typedef uint16_t in_port_t;
typedef uint32_t in_addr_t;
struct in_addr { in_addr_t s_addr; };
struct in6_addr { unsigned char s6_addr[16]; };
#ifdef __APPLE__
struct sockaddr_in { unsigned char sin_len; unsigned char sin_family; in_port_t sin_port; struct in_addr sin_addr; char sin_zero[8]; };
struct sockaddr_in6 { unsigned char sin6_len; unsigned char sin6_family; in_port_t sin6_port; uint32_t sin6_flowinfo; struct in6_addr sin6_addr; uint32_t sin6_scope_id; };
#else
struct sockaddr_in { sa_family_t sin_family; in_port_t sin_port; struct in_addr sin_addr; char sin_zero[8]; };
struct sockaddr_in6 { sa_family_t sin6_family; in_port_t sin6_port; uint32_t sin6_flowinfo; struct in6_addr sin6_addr; uint32_t sin6_scope_id; };
#endif
#define IPPROTO_IP 0
#define IPPROTO_TCP 6
#define IPPROTO_UDP 17
#define INADDR_ANY ((in_addr_t)0)
#define INADDR_LOOPBACK ((in_addr_t)0x7f000001)
#define INADDR_NONE ((in_addr_t)0xffffffff)
#define INET_ADDRSTRLEN 16
#define INET6_ADDRSTRLEN 46
static uint16_t htons(uint16_t __u_x) { return (uint16_t)((__u_x << 8) | (__u_x >> 8)); }
static uint16_t ntohs(uint16_t __u_x) { return htons(__u_x); }
static uint32_t htonl(uint32_t __u_x) { return (__u_x >> 24) | ((__u_x >> 8) & 0xff00) | ((__u_x << 8) & 0xff0000) | (__u_x << 24); }
static uint32_t ntohl(uint32_t __u_x) { return htonl(__u_x); }
#endif
