/* Test watchdog only: POSIX process ownership, never product runtime code. */
#define _POSIX_C_SOURCE 200809L
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif

static volatile sig_atomic_t interrupted;
static void interrupt(int sig) {
    if (!interrupted) interrupted = sig == SIGALRM ? 142 : 128 + sig;
}
static int has(pid_t *owned, size_t n, pid_t pid) {
    for (size_t i = 0; i < n; i++) if (owned[i] == pid) return 1;
    return 0;
}
static int freeze_tree(pid_t root) {
    size_t n = 1, capacity = 32;
    pid_t *owned = malloc(capacity * sizeof *owned);
    int failed = 0;
    if (!owned) { kill(-root, SIGKILL); kill(root, SIGKILL); return -1; }
    owned[0] = root;
    kill(root, SIGSTOP);
    for (;;) {
        int added = 0;
        FILE *f = popen("/bin/ps -axo pid=,ppid=", "r");
        long pid, parent;
        if (!f) { failed = 1; break; }
        while (fscanf(f, "%ld %ld", &pid, &parent) == 2) {
            if (pid <= 0 || !has(owned, n, (pid_t)parent) || has(owned, n, (pid_t)pid)) continue;
            if (n == capacity) {
                size_t next = capacity * 2;
                pid_t *larger = realloc(owned, next * sizeof *owned);
                if (!larger) { failed = 1; break; }
                owned = larger; capacity = next;
            }
            owned[n++] = (pid_t)pid;
            kill((pid_t)pid, SIGSTOP);
            added = 1;
        }
        if (pclose(f) != 0) failed = 1;
        if (failed || !added) break;
    }
    /* Nested setsid changes process groups, but not the frozen parent ledger. */
    kill(-root, SIGKILL);
    for (size_t i = 0; i < n; i++) kill(owned[i], SIGKILL);
    free(owned);
    if (failed) fprintf(stderr, "bound: could not fully enumerate descendants\n");
    return failed ? -1 : 0;
}
static int save_status(const char *path, int status) {
    if (!path) return 0;
    FILE *f = fopen(path, "w");
    if (!f) return -1;
    int bad = fprintf(f, "%d\n", status) < 0;
    if (fclose(f)) bad = 1;
    return bad ? -1 : 0;
}
int main(int argc, char **argv) {
    const char *status_path = NULL;
    int first = 1, status = 0;
    if (argc > 2 && !strcmp(argv[first], "--status")) {
        status_path = argv[first + 1]; first += 2;
    }
    if (argc < first + 2 || !argv[first][0]) goto usage;
    unsigned seconds = 0;
    for (const char *p = argv[first]; *p; p++) {
        if (*p < '0' || *p > '9' || seconds > 60) goto usage;
        seconds = seconds * 10 + (unsigned)(*p - '0');
    }
    if (seconds < 1 || seconds > 60) goto usage;
    struct sigaction action;
    memset(&action, 0, sizeof action); action.sa_handler = interrupt;
    sigemptyset(&action.sa_mask);
    sigaction(SIGALRM, &action, NULL); sigaction(SIGTERM, &action, NULL); sigaction(SIGINT, &action, NULL);
#ifdef __linux__
    /* Adopt detached grandchildren when their command parent exits. */
    if (prctl(PR_SET_CHILD_SUBREAPER, 1, 0, 0, 0)) { perror("bound: subreaper"); return 2; }
#endif
    pid_t child = fork();
    if (child < 0) { perror("bound: fork"); return 2; }
    if (!child) {
        signal(SIGALRM, SIG_DFL); signal(SIGTERM, SIG_DFL); signal(SIGINT, SIG_DFL);
        if (setsid() < 0) { perror("bound: setsid"); _exit(127); }
        execvp(argv[first + 1], argv + first + 1);
        perror("bound: exec"); _exit(127);
    }
    alarm(seconds);
    int terminated = 0;
    for (;;) {
        if (interrupted && !terminated) {
            alarm(0); freeze_tree(child); terminated = 1;
        }
        pid_t got = waitpid(child, &status, 0);
        if (got == child) break;
        if (got < 0 && errno == EINTR) continue;
        if (got < 0) { perror("bound: waitpid"); return 2; }
    }
    alarm(0);
    if (interrupted && !terminated) freeze_tree(child);
    /* Clean ordinary background children on success without a ps startup. */
    kill(-child, SIGKILL);
#ifdef __linux__
    /* /proc avoids launching a ps child that would enter our own ledger. */
    char children_path[96];
    snprintf(children_path, sizeof children_path, "/proc/self/task/%ld/children", (long)getpid());
    FILE *children = fopen(children_path, "r");
    if (children) {
        long pid;
        while (fscanf(children, "%ld", &pid) == 1) if (pid > 0) freeze_tree((pid_t)pid);
        fclose(children);
        while (waitpid(-1, NULL, 0) > 0) {}
    }
#endif
    if (save_status(status_path, status)) { perror("bound: status file"); return 2; }
    if (interrupted) return interrupted;
    return WIFEXITED(status) ? WEXITSTATUS(status) : WIFSIGNALED(status) ? 128 + WTERMSIG(status) : 2;
usage:
    fprintf(stderr, "usage: bound [--status FILE] SECONDS(1..60) COMMAND [ARG...]\n");
    return 2;
}
