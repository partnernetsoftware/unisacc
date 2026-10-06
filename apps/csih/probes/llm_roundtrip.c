/* llm_roundtrip.c — llm.c against probes/llm_stub.py. No gcc.
 *   unisacc probes/llm_roundtrip.c probes/u_run.c
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

static int contains(const char *path, const char *needle) {
    FILE *f = fopen(path, "r");
    char buf[4096];
    size_t n;
    if (!f) return 0;
    n = fread(buf, 1, sizeof buf - 1, f);
    fclose(f);
    buf[n] = 0;
    return strstr(buf, needle) != NULL;
}

int main(void) {
    const char *u = getenv("UNISACC");
    const char *stub = "probes/llm_stub.py";
    char dir[64], tr[96], url[64], out[2048];
    char *pav[4], *args[12];
    int rc, st;
    pid_t pid;
    if (!u || !u[0]) u = "/Users/wjc/repos/unisacc/unisacc.com";
    if (access(stub, 0) != 0) stub = "llm_stub.py";
    snprintf(dir, sizeof dir, "/tmp/cdsh-llm-%d", (int)getpid());
    snprintf(tr, sizeof tr, "%s/session.jsonl", dir);
    if (mkdir(dir, 0700) != 0) { printf("no temp\n"); return 2; }
    pid = fork();
    if (pid == 0) {
        pav[0] = (char *)"python3"; pav[1] = (char *)stub; pav[2] = (char *)"18938"; pav[3] = NULL;
        execvp("python3", pav);
        _exit(127);
    }
    if (wait_port(18938) != 0) { u_stop(pid); printf("  FAIL  stub did not listen\n"); return 1; }
    snprintf(url, sizeof url, "http://127.0.0.1:18938/");
    args[0] = (char *)"llm.c"; args[1] = (char *)"llm_cli.c";
    args[2] = (char *)"json.c"; args[3] = (char *)"net.c";
    args[4] = (char *)"gate.c"; args[5] = (char *)"session.c";
    args[6] = (char *)"run"; args[7] = url; args[8] = tr;
    args[9] = (char *)"hello from cdsh"; args[10] = (char *)"0.5";
    u_spawn(u, args, 11, NULL, out, (int)sizeof out, &rc);
    u_stop(pid); waitpid(pid, &st, 0);
    printf("  unisacc: %.200s (rc=%d)\n", out, rc);
    if (rc != 0) { printf("  FAIL  rc\n"); return 1; }
    if (!strstr(out, "decision=continue") || !strstr(out, "p=0.8300")) {
        printf("  FAIL  verdict\n"); return 1;
    }
    printf("  ok    the reply's p became a continue verdict\n");
    if (!strstr(out, "appended=1")) { printf("  FAIL  not appended\n"); return 1; }
    printf("  ok    the model record was written back\n");
    if (!contains(tr, "\"role\":\"model\"") || !contains(tr, "\"p\":0.8300")) {
        printf("  FAIL  transcript\n"); return 1;
    }
    printf("  ok    the transcript holds the model record\n");
    return 0;
}
