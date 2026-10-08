/* Real fork competition probe. No TUI and no automatic lock-file cleanup. */
#include "reload_owner.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

static int child_attempt(const char *path, int inherited, int want_success, int release) {
    pid_t pid;
    int status, i;
    pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        int fd = -1, rc;
        char why[128];
        if (inherited >= 0) close(inherited); /* child inherits fd, NOT parent's lock */
        rc = reload_owner_acquire(path, &fd, why, sizeof why);
        if (want_success) {
            if (rc != 0 || fd < 0 || !(fcntl(fd, F_GETFD, 0) & FD_CLOEXEC)) _exit(11);
            if (release && (reload_owner_release(&fd, why, sizeof why) != 0 || fd != -1)) _exit(12);
        } else if (rc != -1 || fd != -1 || strcmp(why, "ownership lock unavailable")) _exit(13);
        _exit(0); /* release==0 tests process exit, not library release */
    }
    for (i = 0; i < 400; i++) {
        pid_t done = waitpid(pid, &status, WNOHANG);
        if (done == pid) return WIFEXITED(status) && WEXITSTATUS(status) == 0 ? 0 : -1;
        if (done < 0) return -1;
        usleep(10000);
    }
    kill(pid, SIGKILL);
    waitpid(pid, &status, 0);
    return -1;
}

int main(int argc, char **argv) {
    int fd = -1;
    char why[256];
    if (argc != 3) return 2;
    if (!strcmp(argv[1], "check")) {
        if (reload_owner_acquire(argv[2], &fd, why, sizeof why) != 0) {
            if (fd != -1) return 2;
            printf("FAIL: %s\n", why); return 1;
        }
        if (!(fcntl(fd, F_GETFD, 0) & FD_CLOEXEC) ||
            reload_owner_release(&fd, why, sizeof why) != 0 || fd != -1) return 2;
        puts("PASS: acquire/release"); return 0;
    }
    if (strcmp(argv[1], "selftest")) return 2;
    if (reload_owner_acquire(argv[2], &fd, why, sizeof why) != 0) {
        printf("FAIL: parent acquire %s\n", why); return 1;
    }
    if (child_attempt(argv[2], fd, 0, 0) != 0) { puts("FAIL: child acquired while parent held"); return 1; }
    puts("PASS: parent holds, fork child refused");
    if (reload_owner_release(&fd, why, sizeof why) != 0 || fd != -1) return 1;
    if (child_attempt(argv[2], -1, 1, 1) != 0) { puts("FAIL: child cannot acquire after release"); return 1; }
    puts("PASS: child acquired after parent release");
    if (child_attempt(argv[2], -1, 1, 0) != 0) { puts("FAIL: exiting holder"); return 1; }
    if (reload_owner_acquire(argv[2], &fd, why, sizeof why) != 0) { puts("FAIL: exit did not release"); return 1; }
    if (reload_owner_release(&fd, why, sizeof why) != 0 || fd != -1) return 1;
    puts("PASS: process exit released ownership");
    return 0;
}
