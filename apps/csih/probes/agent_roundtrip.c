/* agent_roundtrip.c — agent loop against probes/agent-stub.py. No gcc.
 *   unisacc probes/agent_roundtrip.c probes/u_run.c
 *
 * CSIH_* must be in the OS environ of the inner unisacc.com. unisacc's
 * setenv() is an in-process override table; exec of another unisacc.com
 * does not see it. Host sh `export` does.
 */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

int u_spawn_sh(const char *bin, char **args, int nargs, const char *script,
               char *out, int cap, int *rc);
void u_stop(int pid);

static int contains(const char *path, const char *needle) {
    FILE *f = fopen(path, "r");
    char buf[8192];
    size_t n;
    if (!f) return 0;
    n = fread(buf, 1, sizeof buf - 1, f);
    fclose(f);
    buf[n] = 0;
    return strstr(buf, needle) != NULL;
}

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
    char dir[64], work[80], tr[96], ep[96], script[512], out[4096];
    char *pav[4], *args[16];
    int rc, st, bad = 0, port;
    pid_t pid;
    const char *stub = "probes/agent-stub.py";
    if (!u || !u[0]) u = "/Users/wjc/repos/unisacc/unisacc.com";
    if (access(stub, 0) != 0) stub = "agent-stub.py";
    snprintf(dir, sizeof dir, "/tmp/csih-agent-%d", (int)getpid());
    snprintf(work, sizeof work, "%s/work", dir);
    snprintf(tr, sizeof tr, "%s/tr.jsonl", dir);
    if (mkdir(dir, 0700) || mkdir(work, 0700)) { printf("no temp\n"); return 2; }
    port = 18000 + ((int)getpid() % 20000);
    {
        char portbuf[8];
        snprintf(portbuf, sizeof portbuf, "%d", port);
        pid = fork();
        if (pid == 0) {
            pav[0] = (char *)"python3"; pav[1] = (char *)stub; pav[2] = portbuf; pav[3] = NULL;
            execvp("python3", pav);
            _exit(127);
        }
    }
    if (wait_port(port) != 0) { u_stop(pid); printf("  FAIL  stub did not listen\n"); return 1; }
    snprintf(ep, sizeof ep, "http://127.0.0.1:%d/v1/chat/completions", port);
    snprintf(script, sizeof script,
        "export CSIH_ENDPOINT='%s'; export CSIH_MODEL=stub; "
        "export CSIH_CWD='%s'; export CSIH_TRANSCRIPT='%s'; exec \"$0\" \"$@\"",
        ep, work, tr);
    args[0] = (char *)"agent.c"; args[1] = (char *)"agent_cli.c";
    args[2] = (char *)"file.c"; args[3] = (char *)"edit.c";
    args[4] = (char *)"shell.c"; args[5] = (char *)"json.c";
    args[6] = (char *)"session.c"; args[7] = (char *)"net.c";
    args[8] = (char *)"plugin.c";
    args[9] = (char *)"agent"; args[10] = (char *)"请做一个简单任务";
    u_spawn_sh(u, args, 11, script, out, (int)sizeof out, &rc);
    if (contains(tr, "hello-from-agent")) printf("  ok    exec action actually ran\n");
    else { printf("  FAIL  exec action not seen rc=%d out=%.160s\n", rc, out); bad++; }
    if (contains(tr, "\"role\":\"decision\",\"go\":\"stop\"")) printf("  ok    loop reached go:stop\n");
    else { printf("  FAIL  no go:stop\n"); bad++; }
    if (contains(tr, "完成了")) printf("  ok    final answer recorded\n");
    else { printf("  FAIL  no answer\n"); bad++; }
    u_stop(pid); waitpid(pid, &st, 0);
    printf(bad ? "agent roundtrip: %d FAIL\n" : "agent roundtrip: PASS\n", bad);
    return bad ? 1 : 0;
}
