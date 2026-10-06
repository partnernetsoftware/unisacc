/* u_run.c — spawn unisacc.com and keep its own wait status. No main.
 *
 * unisacc.com is an APE. execv on it returns ENOEXEC. /bin/sh -c exec is how
 * a terminal starts it, and exec replaces the shell so the status is unisacc's.
 * This is not a C compiler.
 */
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int u_spawn(const char *bin, char **args, int nargs, const char *cwd,
            char *out, int cap, int *rc) {
    int p[2], i, st;
    pid_t pid;
    size_t off = 0;
    if (cap < 1 || !bin || !rc) return -1;
    out[0] = 0;
    *rc = 127;
    if (pipe(p) != 0) return -1;
    pid = fork();
    if (pid < 0) { close(p[0]); close(p[1]); return -1; }
    if (pid == 0) {
        char *argv[32];
        int dn, n = 0;
        if (nargs + 8 > 32) _exit(127);
        argv[n++] = (char *)"sh";
        argv[n++] = (char *)"-c";
        if (cwd && cwd[0]) {
            /* $0 is dummy. Save cwd and bin, shift them off, exec bin on the rest. */
            argv[n++] = (char *)"d=$1; b=$2; shift 2; cd \"$d\" && exec \"$b\" \"$@\"";
            argv[n++] = (char *)"_";
            argv[n++] = (char *)cwd;
            argv[n++] = (char *)bin;
        } else {
            argv[n++] = (char *)"exec \"$0\" \"$@\"";
            argv[n++] = (char *)bin;
        }
        for (i = 0; i < nargs; i++) argv[n++] = args[i];
        argv[n] = NULL;
        dn = open("/dev/null", 0);
        if (dn >= 0) { dup2(dn, 0); close(dn); }
        dup2(p[1], 1);
        dup2(p[1], 2);
        close(p[0]);
        close(p[1]);
        execv("/bin/sh", argv);
        _exit(127);
    }
    close(p[1]);
    while ((int)off + 1 < cap) {
        ssize_t k = read(p[0], out + off, (size_t)cap - 1 - off);
        if (k < 0) { if (errno == EINTR) continue; break; }
        if (k == 0) break;
        off += (size_t)k;
    }
    out[off] = 0;
    close(p[0]);
    if (waitpid(pid, &st, 0) < 0) return -1;
    *rc = WIFEXITED(st) ? WEXITSTATUS(st) : 128;
    return 0;
}

/* Like u_spawn, but `script` is the sh -c body. $0 is bin, $@ is args.
 * Use this to `export` names: unisacc setenv() does not put variables into
 * the OS environ that a later execve child (another unisacc.com) will see. */
int u_spawn_sh(const char *bin, char **args, int nargs, const char *script,
               char *out, int cap, int *rc) {
    int p[2], i, st;
    pid_t pid;
    size_t off = 0;
    if (cap < 1 || !bin || !rc || !script) return -1;
    out[0] = 0;
    *rc = 127;
    if (pipe(p) != 0) return -1;
    pid = fork();
    if (pid < 0) { close(p[0]); close(p[1]); return -1; }
    if (pid == 0) {
        char *argv[32];
        int dn, n = 0;
        if (nargs + 6 > 32) _exit(127);
        argv[n++] = (char *)"sh";
        argv[n++] = (char *)"-c";
        argv[n++] = (char *)script;
        argv[n++] = (char *)bin;
        for (i = 0; i < nargs; i++) argv[n++] = args[i];
        argv[n] = NULL;
        dn = open("/dev/null", 0);
        if (dn >= 0) { dup2(dn, 0); close(dn); }
        dup2(p[1], 1);
        dup2(p[1], 2);
        close(p[0]);
        close(p[1]);
        execv("/bin/sh", argv);
        _exit(127);
    }
    close(p[1]);
    while ((int)off + 1 < cap) {
        ssize_t k = read(p[0], out + off, (size_t)cap - 1 - off);
        if (k < 0) { if (errno == EINTR) continue; break; }
        if (k == 0) break;
        off += (size_t)k;
    }
    out[off] = 0;
    close(p[0]);
    if (waitpid(pid, &st, 0) < 0) return -1;
    *rc = WIFEXITED(st) ? WEXITSTATUS(st) : 128;
    return 0;
}

/* SIGTERM. kill() is in the published unisacc from 0.0.25. */
void u_stop(int pid) {
    if (pid <= 0) return;
    kill(pid, 15);
}
