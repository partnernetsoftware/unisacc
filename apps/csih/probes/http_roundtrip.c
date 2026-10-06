/* http_roundtrip.c — csih net.c GET against a local python http.server.
 *   unisacc probes/http_roundtrip.c probes/u_run.c
 * python only serves the file. The client under test is unisacc. No gcc.
 */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

int u_spawn(const char *bin, char **args, int nargs, const char *cwd,
            char *out, int cap, int *rc);
void u_stop(int pid);

static int wait_port(int port) {
    int i;
    for (i = 0; i < 40; i++) {
        int s = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in a;
        if (s < 0) return -1;
        memset(&a, 0, sizeof a);
        a.sin_family = AF_INET;
        a.sin_port = htons((unsigned short)port);
        a.sin_addr.s_addr = htonl(0x7f000001);
        if (connect(s, (struct sockaddr *)&a, sizeof a) == 0) { close(s); return 0; }
        close(s);
        usleep(100000);
    }
    return -1;
}

int main(void) {
    const char *u = getenv("UNISACC");
    char dir[64], www[80], idx[96], port[16], url[80], out[2048];
    char *pav[10], *args[4];
    int rc, st;
    pid_t pid;
    FILE *f;
    if (!u || !u[0]) u = "/Users/wjc/repos/unisacc/unisacc.com";
    snprintf(dir, sizeof dir, "/tmp/cdsh-http-%d", (int)getpid());
    snprintf(www, sizeof www, "%s/www", dir);
    snprintf(idx, sizeof idx, "%s/index.txt", www);
    if (mkdir(dir, 0700) || mkdir(www, 0700)) { printf("no temp\n"); return 2; }
    f = fopen(idx, "w"); if (!f) return 2;
    fputs("hello from cdsh\n", f); fclose(f);
    snprintf(port, sizeof port, "18937");
    pid = fork();
    if (pid == 0) {
        pav[0] = (char *)"python3"; pav[1] = (char *)"-m"; pav[2] = (char *)"http.server";
        pav[3] = port; pav[4] = (char *)"--bind"; pav[5] = (char *)"127.0.0.1";
        pav[6] = (char *)"--directory"; pav[7] = www; pav[8] = NULL;
        execvp("python3", pav);
        _exit(127);
    }
    if (wait_port(18937) != 0) { u_stop(pid); printf("  FAIL  server did not listen\n"); return 1; }
    snprintf(url, sizeof url, "http://127.0.0.1:18937/index.txt");
    args[0] = (char *)"net.c"; args[1] = (char *)"net_cli.c";
    args[2] = (char *)"get"; args[3] = url;
    u_spawn(u, args, 4, NULL, out, (int)sizeof out, &rc);
    u_stop(pid); waitpid(pid, &st, 0);
    printf("  unisacc: %s\n", out);
    if (!strstr(out, "hello from cdsh")) { printf("  FAIL  body\n"); return 1; }
    printf("  ok    the response body is the served file\n");
    if (!strstr(out, "status 200")) { printf("  FAIL  status\n"); return 1; }
    printf("  ok    status 200\n");
    remove(idx); rmdir(www); rmdir(dir);
    return 0;
}
