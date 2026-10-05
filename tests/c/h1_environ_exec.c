/* 0.0.28 E20: execv and execvp pass the CURRENT environ, as POSIX requires of the exec functions without e */
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
extern char **environ;
int main(void) {
    char *env[] = { "UNISA_E20=seen", "PATH=/usr/bin:/bin", 0 };
    char *argv[] = { "sh", "-c", "echo \"$UNISA_E20\"", 0 };
    int st = 0; pid_t p;
    environ = env;
    fflush(stdout);
    p = fork();
    if (p == 0) { execv("/bin/sh", argv); _exit(127); }
    waitpid(p, &st, 0);
    p = fork();
    if (p == 0) { execvp("sh", argv); _exit(127); }
    waitpid(p, &st, 0);
    printf("done %d\n", WIFEXITED(st) ? WEXITSTATUS(st) : -1);
    return 0;
}
