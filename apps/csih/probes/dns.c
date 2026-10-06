#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
int main(void) {
    struct addrinfo hints, *res, *p; int rc; char ip[64];
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; hints.ai_socktype = SOCK_STREAM;
    rc = getaddrinfo("localhost", "80", &hints, &res);
    if (rc != 0) { printf("rc=%d %s\n", rc, gai_strerror(rc)); return 0; }
    for (p = res; p; p = p->ai_next) {
        struct sockaddr_in *in = (struct sockaddr_in *)p->ai_addr;
        inet_ntop(AF_INET, &in->sin_addr, ip, sizeof ip);
        printf("-> %s\n", ip);
    }
    freeaddrinfo(res);
    return 0;
}
