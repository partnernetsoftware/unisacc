/* 0.0.28 H1': POSIX environ, declared the way programs declare it, read and assigned */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
extern char **environ;
int main(void) {
    int n = 0, path = 0; char **e; char *mine[2];
    for (e = environ; *e; e++) { n++; if (strncmp(*e, "PATH=", 5) == 0) path = 1; }
    mine[0] = "UNISA_ONLY=1"; mine[1] = 0;
    environ = mine;
    printf("%d %d %s\n", n > 0, path, environ[0]);
    return 0;
}
