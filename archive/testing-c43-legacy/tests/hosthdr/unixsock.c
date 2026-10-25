/* sys/un.h (dsh): an AF_UNIX stream pair through bind/listen/connect/accept */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
int main(void) {
    struct sockaddr_un a; int s; int c; int k; char buf[16]; long n;
    memset(&a, 0, sizeof a); a.sun_family = AF_UNIX; strcpy(a.sun_path, "u.sock");
    unlink("u.sock");
    s = socket(AF_UNIX, SOCK_STREAM, 0);
    printf("bind %d\n", bind(s, (struct sockaddr *)&a, sizeof a));
    printf("listen %d\n", listen(s, 1));
    c = socket(AF_UNIX, SOCK_STREAM, 0);
    printf("connect %d\n", connect(c, (struct sockaddr *)&a, sizeof a));
    k = accept(s, 0, 0);
    write(c, "ping", 4); n = read(k, buf, sizeof buf - 1); buf[n > 0 ? n : 0] = 0;
    printf("got %s size %d\n", buf, (int)sizeof a);
    close(c); close(k); close(s); unlink("u.sock");
    return 0;
}
