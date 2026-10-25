/* 0.0.33 W2: getaddrinfo forwards to ws2_32 on Windows (system resolver, WSAStartup by the header's
 * constructor).  localhost and a public name must resolve; the bundled reader could not do the second. */
#include <stdio.h>
#include <netdb.h>
static int one(const char *name) {
    struct addrinfo h = {0}, *r = 0; int e;
    h.ai_family = AF_INET; h.ai_socktype = SOCK_STREAM;
    e = getaddrinfo(name, "443", &h, &r);
    if (e) { printf("%s err %d %s\n", name, e, gai_strerror(e)); return 1; }
    printf("%s family %d len %d port %d\n", name, r->ai_family, (int)r->ai_addrlen,
           ntohs(((struct sockaddr_in *)r->ai_addr)->sin_port));
    freeaddrinfo(r); return 0;
}
int main(void) { return one("localhost") | one("github.com"); }
