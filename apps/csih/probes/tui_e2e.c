/* tui_e2e.c — pty witness stays probes/tui_e2e.py (openpty). This file is the entry.
 *   unisacc probes/tui_e2e.c
 * The python drives csih.sh, which is unisacc.com csih.c plus the file table.
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
    const char *u = getenv("UNISACC");
    char dir[64], cwd[512];
    char *av[8];
    int st;
    pid_t pid;
    const char *srcs =
        "tui.c render.c term.c chat.c clock.c tools.c file.c shell.c edit.c gate.c json.c session.c agent.c plugin.c net.c";
    if (!u || !u[0]) u = "/Users/wjc/repos/unisacc/unisacc.com";
    if (!getcwd(cwd, sizeof cwd)) return 2;
    snprintf(dir, sizeof dir, "/tmp/cdsh-tuie-%d", (int)getpid());
    if (mkdir(dir, 0700) != 0) return 2;
    pid = fork();
    if (pid == 0) {
        av[0] = (char *)"python3";
        av[1] = (char *)"probes/tui_e2e.py";
        av[2] = (char *)u;
        av[3] = cwd;
        av[4] = (char *)srcs;
        av[5] = dir;
        av[6] = NULL;
        execvp("python3", av);
        _exit(127);
    }
    if (waitpid(pid, &st, 0) < 0) return 2;
    rmdir(dir);
    return WIFEXITED(st) ? WEXITSTATUS(st) : 1;
}
