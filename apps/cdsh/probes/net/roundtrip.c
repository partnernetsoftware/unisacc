/* A real loopback TCP round trip — RUN it, do not just compile it. */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>      /* see net-cover.sh's trap section */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(void) {
    int ls, conn, port, st = 0;
    struct sockaddr_in sa; socklen_t sl = sizeof sa;
    pid_t pid;
    ls = socket(AF_INET, SOCK_STREAM, 0);
    if (ls < 0) { printf("socket failed\n"); return 1; }
    memset(&sa, 0, sizeof sa);
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    sa.sin_port = 0;
    if (bind(ls, (struct sockaddr *)&sa, sizeof sa) || listen(ls, 1)) { printf("bind/listen failed\n"); return 1; }
    if (getsockname(ls, (struct sockaddr *)&sa, &sl)) { printf("getsockname failed\n"); return 1; }
    port = ntohs(sa.sin_port);
    pid = fork();
    if (pid == 0) {
        char buf[64]; ssize_t n;
        int c = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in ca;
        memset(&ca, 0, sizeof ca);
        ca.sin_family = AF_INET;
        ca.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        ca.sin_port = htons((unsigned short)port);
        if (connect(c, (struct sockaddr *)&ca, sizeof ca)) _exit(9);
        n = read(c, buf, sizeof buf - 1);
        if (n <= 0) _exit(10);
        buf[n] = 0;
        /* write(), not printf(): a fork'd child that calls _exit() may or may
         * not flush stdio depending on the libc, so printf here made the two
         * backends differ for a reason that has nothing to do with networking.
         * write() is unbuffered, so this probe measures the NETWORK. */
        { char line[80]; int k = snprintf(line, sizeof line, "client got: %s", buf);
          if (k > 0) write(1, line, (size_t)k); }
        write(c, "pong\n", 5);
        close(c);
        _exit(0);
    }
    conn = accept(ls, 0, 0);
    if (conn < 0) { printf("accept failed\n"); return 1; }
    write(conn, "ping\n", 5);
    { char b[64]; ssize_t n = read(conn, b, sizeof b - 1);
      if (n > 0) { char line[80]; int k; b[n] = 0;
        k = snprintf(line, sizeof line, "server got: %s", b);
        if (k > 0) write(1, line, (size_t)k); } }
    close(conn); close(ls);
    waitpid(pid, &st, 0);
    { char line[80]; int k = snprintf(line, sizeof line, "child exited=%d status=%d\n",
                                     WIFEXITED(st), WEXITSTATUS(st));
      if (k > 0) write(1, line, (size_t)k); }
    return 0;
}
