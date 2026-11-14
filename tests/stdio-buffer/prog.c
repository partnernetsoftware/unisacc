/* SPEED-17 stdout buffering test program; built by tools/check-stdio-buffer.cx with the product
 * (-I <root>/include) and with gcc -O0, run as `prog MODE` with stdout and stderr on one pipe. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <spawn.h>
#include <sys/wait.h>

extern char **environ;

int main(int argc, char **argv) {
    const char *m = argc > 1 ? argv[1] : "";
    int i, st;
    pid_t p;
    if (!strcmp(m, "exit")) { printf("one\n"); fputs("two", stdout); exit(3); }
    if (!strcmp(m, "ret")) { printf("one\n"); fputs("two", stdout); return 4; }
    if (!strcmp(m, "abort")) { printf("lost\n"); abort(); }
    if (!strcmp(m, "fork")) {
        printf("a\n");
        p = fork();
        if (p == 0) { printf("child\n"); return 0; }
        waitpid(p, &st, 0);
        printf("parent\n");
        return 0;
    }
    if (!strcmp(m, "puts")) { printf("1\n"); puts("2"); fputs("3", stdout); puts(""); printf("4\n"); return 0; }
    if (!strcmp(m, "system")) { printf("1\n"); system("echo 2"); printf("3\n"); return 0; }
    if (!strcmp(m, "execl")) { printf("1\n"); execl("/bin/echo", "echo", "2", (char *)0); return 9; }
    if (!strcmp(m, "pspawn")) {
        char *av[3];
        av[0] = "echo"; av[1] = "2"; av[2] = 0;
        printf("1\n");
        if (posix_spawn(&p, "/bin/echo", 0, 0, av, environ) != 0) return 9;
        waitpid(p, &st, 0);
        printf("3\n");
        return 0;
    }
    if (!strcmp(m, "stderr")) { printf("o1\n"); fprintf(stderr, "e1\n"); printf("o2\n"); fprintf(stderr, "e2\n"); return 0; }
    if (!strcmp(m, "fflush")) { printf("x"); fflush(NULL); write(1, "W", 1); printf("y\n"); return 0; }
    if (!strcmp(m, "fflush1")) { printf("x"); fflush(stdout); write(1, "W", 1); printf("y\n"); return 0; }
    if (!strcmp(m, "nbuf")) { setvbuf(stdout, NULL, _IONBF, 0); printf("a"); write(1, "W", 1); printf("b\n"); return 0; }
    if (!strcmp(m, "fbuf")) { setvbuf(stdout, NULL, _IOFBF, 4096); printf("a"); write(1, "W", 1); printf("b\n"); return 0; }
    if (!strcmp(m, "lbuf")) { setvbuf(stdout, NULL, _IOLBF, 4096); printf("a"); write(1, "W", 1); printf("b\n"); return 0; }
    if (!strcmp(m, "big")) {
        unsigned long h = 1469598103UL;
        for (i = 0; i < 20000; i++) { printf("line %d of the big output %lu\n", i, h); h = h * 31UL + (unsigned long)i; }
        printf("end %lu\n", h);
        return 0;
    }
    if (!strcmp(m, "huge")) {
        static char blk[20000];
        memset(blk, 'z', sizeof blk - 1);
        printf("pre\n"); fwrite(blk, 1, sizeof blk - 1, stdout); printf("\npost\n");
        return 0;
    }
    if (!strcmp(m, "prompt")) {
        char b[16];
        fputs("prompt> ", stdout);
        if (fgets(b, sizeof b, stdin)) printf("got\n"); else printf("eof\n");
        return 0;
    }
    return 7;
}
