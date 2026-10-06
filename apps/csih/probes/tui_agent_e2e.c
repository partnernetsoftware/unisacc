/* tui_agent_e2e.c — start agent-stub.py, then the pty witness.
 *   unisacc probes/tui_agent_e2e.c
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
    const char *u = getenv("UNISACC");
    char dir[64], cwd[512], here[540];
    char *stav[4], *av[8];
    int st;
    pid_t stub, pid;
    if (!u || !u[0]) u = "/Users/wjc/repos/unisacc/unisacc.com";
    if (!getcwd(cwd, sizeof cwd)) return 2;
    snprintf(dir, sizeof dir, "/tmp/cdsh-tuia-%d", (int)getpid());
    snprintf(here, sizeof here, "%s/probes", cwd);
    if (mkdir(dir, 0700) != 0) return 2;
    stub = fork();
    if (stub == 0) {
        stav[0] = (char *)"python3";
        stav[1] = (char *)"probes/agent-stub.py";
        stav[2] = (char *)"8137";
        stav[3] = NULL;
        execvp("python3", stav);
        _exit(127);
    }
    usleep(500000);
    pid = fork();
    if (pid == 0) {
        av[0] = (char *)"python3";
        av[1] = (char *)"probes/tui_agent_e2e.py";
        av[2] = dir;
        av[3] = (char *)u;
        av[4] = here;
        av[5] = (char *)"8137";
        av[6] = NULL;
        execvp("python3", av);
        _exit(127);
    }
    if (waitpid(pid, &st, 0) < 0) st = 1;
    {
        char buf[16], *kav[3];
        int ks;
        pid_t kk = fork();
        snprintf(buf, sizeof buf, "%d", (int)stub);
        if (kk == 0) {
            kav[0] = (char *)"kill"; kav[1] = buf; kav[2] = NULL;
            execv("/bin/kill", kav);
            _exit(127);
        }
        if (kk > 0) waitpid(kk, &ks, 0);
    }
    waitpid(stub, NULL, 0);
    return WIFEXITED(st) ? WEXITSTATUS(st) : 1;
}
