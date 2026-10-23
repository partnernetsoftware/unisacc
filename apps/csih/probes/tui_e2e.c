/* tui_e2e.c — pty witness stays probes/tui_e2e.py (openpty). This file is the entry.
 *   unisacc probes/tui_e2e.c
 * The python drives csih.sh, which is unisacc.com csih.c plus the file table.
 */
#include "../../../include/unisa_safedel.h"   /* the one deletion guard (commissar ruling 2026-10-23); first: its feature macro */
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

/* P (a tree) through the one guard, never a raw unlink/rmdir; a refusal is reported, the probe's verdict stands */
static void sd_gone(const char *p) {
    char why[600];
    if (unisa_safedel(p, SAFEDEL_PURGE | SAFEDEL_RECURSIVE, 0, why, (int)sizeof why)) fprintf(stderr, "probe: %s\n", why);
}
int main(void) {
    const char *u = getenv("UNISACC");
    char dir[1024], cwd[512];
    char *av[8];
    int st;
    pid_t pid;
    const char *srcs =
        "tui.c render.c term.c chat.c clock.c tools.c file.c shell.c edit.c gate.c json.c session.c agent.c plugin.c net.c";
    if (!u || !u[0]) u = "/Users/wjc/repos/unisacc/unisacc.com";
    if (!getcwd(cwd, sizeof cwd)) return 2;
    snprintf(dir, sizeof dir, "%s/cdsh-tuie-%d", getenv("TMPDIR") && *getenv("TMPDIR") ? getenv("TMPDIR") : "/tmp", (int)getpid());   /* the guard's temp root */
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
    sd_gone(dir);
    return WIFEXITED(st) ? WEXITSTATUS(st) : 1;
}
