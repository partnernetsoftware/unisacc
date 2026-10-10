/* llm_stub.c — C port of llm_stub.py.
 * Localhost HTTP stub: every POST gets the same JSON reply
 *   {"p": 0.83, "text": "continue"}
 * Build:  /bin/sh unisacc.com probes/llm_stub.c -o OUT
 * Run:    OUT [port]          (default 19000)
 */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define REQ_CAP 65536

static void serve_one(int c) {
    static char req[REQ_CAP];
    const char *body = "{\"p\": 0.83, \"text\": \"continue\"}";
    char head[256];
    size_t have = 0, want = 0, hdr_end = 0;
    int got_hdr = 0;
    ssize_t k;
    char *cl;

    for (;;) {
        if (have + 1 >= REQ_CAP) break;
        k = read(c, req + have, REQ_CAP - 1 - have);
        if (k <= 0) break;
        have += (size_t)k;
        req[have] = 0;
        if (!got_hdr) {
            char *e = strstr(req, "\r\n\r\n");
            if (e) {
                got_hdr = 1;
                hdr_end = (size_t)(e - req) + 4;
                want = 0;
                cl = strstr(req, "Content-Length:");
                if (!cl) cl = strstr(req, "content-length:");
                if (cl) want = (size_t)strtoul(cl + 15, NULL, 10);
            }
        }
        if (got_hdr && have >= hdr_end + want) break;
    }
    (void)hdr_end;
    snprintf(head, sizeof head,
             "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n"
             "Content-Length: %d\r\nConnection: close\r\n\r\n",
             (int)strlen(body));
    write(c, head, strlen(head));
    write(c, body, strlen(body));
}

int main(int argc, char **argv) {
    int port = argc > 1 ? atoi(argv[1]) : 19000;
    int s, c, one = 1;
    struct sockaddr_in a;

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) { printf("FAIL socket\n"); return 1; }
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
    memset(&a, 0, sizeof a);
    a.sin_family = AF_INET;
    a.sin_port = htons((unsigned short)port);
    a.sin_addr.s_addr = htonl(0x7f000001);
    if (bind(s, (struct sockaddr *)&a, sizeof a) != 0) { printf("FAIL bind\n"); return 1; }
    if (listen(s, 16) != 0) { printf("FAIL listen\n"); return 1; }
    for (;;) {
        c = accept(s, NULL, NULL);
        if (c < 0) continue;
        serve_one(c);
        close(c);
    }
    return 0;
}
