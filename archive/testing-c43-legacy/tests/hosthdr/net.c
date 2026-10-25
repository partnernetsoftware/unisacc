/* 0.0.19 R19-1 (dsh): loopback TCP between a forked client and the server,
 * select, inet_pton/ntop, getaddrinfo on a numeric host and localhost */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
int main(void) {
    int s, c, one = 1, st; struct sockaddr_in a; socklen_t al = sizeof a; char buf[64]; long n; fd_set rs; struct timeval tv;
    struct addrinfo h, *r; char txt[INET_ADDRSTRLEN];
    s = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
    memset(&a, 0, sizeof a); a.sin_family = AF_INET; a.sin_port = htons(0); a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(s, (struct sockaddr *)&a, sizeof a) != 0 || listen(s, 1) != 0) { printf("bind/listen failed\n"); return 1; }
    getsockname(s, (struct sockaddr *)&a, &al);
    if (fork() == 0) {
        int k = socket(AF_INET, SOCK_STREAM, 0);
        if (connect(k, (struct sockaddr *)&a, sizeof a) != 0) _exit(2);
        send(k, "ping over tcp", 13, 0); n = recv(k, buf, sizeof buf, 0); close(k);
        _exit(n == 4 && memcmp(buf, "pong", 4) == 0 ? 0 : 3);
    }
    FD_ZERO(&rs); FD_SET(s, &rs); tv.tv_sec = 5; tv.tv_usec = 0;
    printf("select %d ready %d\n", select(s + 1, &rs, 0, 0, &tv), FD_ISSET(s, &rs) != 0);
    c = accept(s, 0, 0);
    n = recv(c, buf, sizeof buf - 1, 0); buf[n > 0 ? n : 0] = 0;
    printf("server got: %s\n", buf);
    send(c, "pong", 4, 0); close(c); close(s);
    wait(&st); printf("client status %d\n", WEXITSTATUS(st));
    inet_ntop(AF_INET, &a.sin_addr, txt, sizeof txt); printf("addr %s port>0 %d\n", txt, ntohs(a.sin_port) > 0);
    memset(&h, 0, sizeof h); h.ai_family = AF_INET; h.ai_socktype = SOCK_STREAM;
    if (getaddrinfo("127.0.0.1", "80", &h, &r) == 0) { struct sockaddr_in *q = (struct sockaddr_in *)r->ai_addr; inet_ntop(AF_INET, &q->sin_addr, txt, sizeof txt); printf("gai numeric %s %d\n", txt, ntohs(q->sin_port)); freeaddrinfo(r); }
    if (getaddrinfo("localhost", "8080", &h, &r) == 0) { struct sockaddr_in *q = (struct sockaddr_in *)r->ai_addr; inet_ntop(AF_INET, &q->sin_addr, txt, sizeof txt); printf("gai localhost %s %d\n", txt, ntohs(q->sin_port)); freeaddrinfo(r); }
    return 0;
}
