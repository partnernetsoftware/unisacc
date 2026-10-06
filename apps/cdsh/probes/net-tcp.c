/* 连一个本地 TCP 端口；用 `nc -l 9099` 起服务端。 */
#include <stdio.h>
#include <stdlib.h>          /* atoi — 少了它 gcc 会报 implicit declaration */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
int main(int argc, char **argv) {
    int port = argc > 1 ? atoi(argv[1]) : 9099;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { printf("UNAVAILABLE socket\n"); return 1; }
    struct sockaddr_in a;
    a.sin_family = AF_INET;
    a.sin_port = htons((unsigned short)port);
    a.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (connect(fd, (struct sockaddr *)&a, sizeof(a)) != 0) { printf("UNAVAILABLE connect (is a listener up?)\n"); return 1; }
    printf("ok connected\n");
    return 0;
}
