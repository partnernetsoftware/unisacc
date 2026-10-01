/* R20-6 (dsh): clock_gettime, nanosleep, sleep/usleep, execl/execlp/execle */
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
int main(void) {
    struct timespec a, b, q; int st; pid_t pid; long ms; char *env[2];
    printf("rt %d\n", clock_gettime(CLOCK_REALTIME, &a) == 0 && a.tv_sec > 1700000000 && a.tv_nsec >= 0 && a.tv_nsec < 1000000000);
    clock_gettime(CLOCK_MONOTONIC, &a);
    q.tv_sec = 0; q.tv_nsec = 30000000;
    printf("nanosleep %d\n", nanosleep(&q, 0));
    usleep(20000);
    clock_gettime(CLOCK_MONOTONIC, &b);
    ms = (b.tv_sec - a.tv_sec) * 1000 + (b.tv_nsec - a.tv_nsec) / 1000000;
    printf("slept>=50ms %d <2s %d\n", ms >= 49, ms < 2000);
    q.tv_nsec = 1000000000;
    printf("einval %d\n", nanosleep(&q, 0) == -1);
    pid = fork(); if (pid == 0) { execl("/bin/sh", "sh", "-c", "exit 4", (char *)0); _exit(99); }
    waitpid(pid, &st, 0); printf("execl %d\n", WEXITSTATUS(st));
    pid = fork(); if (pid == 0) { execlp("sh", "sh", "-c", "exit 5", (char *)0); _exit(99); }
    waitpid(pid, &st, 0); printf("execlp %d\n", WEXITSTATUS(st));
    env[0] = "UV=6"; env[1] = 0;
    pid = fork(); if (pid == 0) { execle("/bin/sh", "sh", "-c", "exit $UV", (char *)0, env); _exit(99); }
    waitpid(pid, &st, 0); printf("execle %d\n", WEXITSTATUS(st));
    printf("sleep %u\n", sleep(0));
    return 0;
}
