/* R19-5: fork, pipe, dup2, execvp, waitpid, getcwd, getpid -- dsh's needs */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
int main(void) {
    int fds[2]; char buf[64]; int st; pid_t pid; long n; char cwd[1024];
    if (pipe(fds) != 0) { printf("pipe failed\n"); return 1; }
    pid = fork();
    if (pid == 0) {                       /* child: write through the pipe, exit 7 */
        close(fds[0]); write(fds[1], "hello from child", 16); close(fds[1]); _exit(7);
    }
    close(fds[1]);
    n = read(fds[0], buf, sizeof buf - 1); buf[n > 0 ? n : 0] = 0; close(fds[0]);
    if (waitpid(pid, &st, 0) != pid) { printf("waitpid failed\n"); return 1; }
    printf("read: %s\n", buf);
    printf("exited %d status %d\n", WIFEXITED(st), WEXITSTATUS(st));
    pid = fork();
    if (pid == 0) {                       /* child: stdout to the pipe-less dup, then exec */
        char *argv[3]; argv[0] = "sh"; argv[1] = "-c"; argv[2] = 0;
        { char *a[4]; a[0] = "sh"; a[1] = "-c"; a[2] = "exit 3"; a[3] = 0; execvp("sh", a); }
        _exit(99);
    }
    waitpid(pid, &st, 0);
    printf("exec child status %d\n", WEXITSTATUS(st));
    printf("dup2 %d\n", dup2(1, 9) == 9);
    printf("getcwd %d pid>0 %d\n", getcwd(cwd, sizeof cwd) != 0 && cwd[0] == '/', getpid() > 0);
    return 0;
}
