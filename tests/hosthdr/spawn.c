/* <spawn.h>: posix_spawn / posix_spawnp: adddup2/addclose into a pipe, addopen, exec failure */
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <spawn.h>
#include <sys/wait.h>
/* not named environ: since H1″ <unistd.h> #defines environ to the shared _unisa_env */
static char *spawnenv[2] = { "PATH=/usr/bin:/bin", 0 };
int main(void) {
    pid_t pid; int st; int fd[2]; char buf[64]; int n; int r;
    char *a1[4]; char *a2[3]; char *a3[2];
    posix_spawn_file_actions_t fa; posix_spawnattr_t at;
    a1[0] = "sh"; a1[1] = "-c"; a1[2] = "exit 7"; a1[3] = 0;
    r = posix_spawn(&pid, "/bin/sh", 0, 0, a1, spawnenv);
    waitpid(pid, &st, 0); printf("spawn r=%d pid>0 %d exit %d\n", r, pid > 0, WEXITSTATUS(st));
    pipe(fd); posix_spawnattr_init(&at); posix_spawn_file_actions_init(&fa);
    posix_spawn_file_actions_adddup2(&fa, fd[1], 1);
    posix_spawn_file_actions_addclose(&fa, fd[0]);
    a2[0] = "echo"; a2[1] = "hello spawn"; a2[2] = 0;
    r = posix_spawnp(&pid, "echo", &fa, &at, a2, spawnenv);
    close(fd[1]); posix_spawn_file_actions_destroy(&fa);
    n = (int)read(fd[0], buf, sizeof buf - 1); if (n < 0) n = 0; buf[n] = 0; close(fd[0]);
    waitpid(pid, &st, 0); printf("spawnp r=%d out [%s] exit %d\n", r, buf, WEXITSTATUS(st));
    a3[0] = "/no/such/prog"; a3[1] = 0;
    r = posix_spawn(&pid, "/no/such/prog", 0, &at, a3, spawnenv);
    if (r == 0) { waitpid(pid, &st, 0); r = WEXITSTATUS(st) == 127 ? -1 : -2; }
    printf("missing failed %d\n", r != 0 && r != -2);
    posix_spawn_file_actions_init(&fa);
    posix_spawn_file_actions_addopen(&fa, 0, "/dev/null", O_RDONLY, 0);
    a1[2] = "read x; exit $?";
    r = posix_spawn(&pid, "/bin/sh", &fa, &at, a1, spawnenv);
    waitpid(pid, &st, 0); printf("addopen r=%d exit %d\n", r, WEXITSTATUS(st));
    posix_spawn_file_actions_destroy(&fa); posix_spawnattr_destroy(&at);
    return 0;
}
