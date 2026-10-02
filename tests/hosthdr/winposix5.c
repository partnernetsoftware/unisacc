/* Windows POSIX layer batch 5 (0.0.22): posix_spawnp + waitpid, with stdout
   redirected by a file action.  The child is the platform's shell. */
#include <stdio.h>
#include <spawn.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef _WIN32
static char *exit3[] = {"cmd.exe", "/c", "exit 3", 0};
static char *echo[] = {"cmd.exe", "/c", "echo hi there", 0};
#else
static char *exit3[] = {"sh", "-c", "exit 3", 0};
static char *echo[] = {"sh", "-c", "echo hi there", 0};
#endif

int main(void) {
    pid_t pid; int st, r, fd, n, i; char buf[64];
    posix_spawn_file_actions_t fa;
    r = posix_spawnp(&pid, exit3[0], 0, 0, exit3, 0);
    printf("spawn %d\n", r);
    st = -1; printf("wait %d\n", waitpid(pid, &st, 0) == pid);
    printf("exited %d code %d\n", WIFEXITED(st), WEXITSTATUS(st));
    posix_spawn_file_actions_init(&fa);
    posix_spawn_file_actions_addopen(&fa, 1, "wp5.tmp", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    r = posix_spawnp(&pid, echo[0], &fa, 0, echo, 0);
    printf("spawn2 %d\n", r);
    st = -1; waitpid(pid, &st, 0); printf("code2 %d\n", WEXITSTATUS(st));
    posix_spawn_file_actions_destroy(&fa);
    fd = open("wp5.tmp", O_RDONLY); n = (int)read(fd, buf, sizeof buf - 1); close(fd); unlink("wp5.tmp");
    printf("out ");
    for (i = 0; i < n; i++) if (buf[i] != '\r') putchar(buf[i]);
    return 0;
}
