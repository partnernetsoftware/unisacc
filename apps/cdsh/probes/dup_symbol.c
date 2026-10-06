/* dup_symbol.c — duplicate-definition probe, run by unisacc itself.
 *
 *   unisacc probes/dup_symbol.c
 *
 * No cc/gcc/clang. The old probes/dup-symbol.sh recorded head's exit code
 * (`u=$(unisacc k.c 2>&1 | head -1); urc=$?`) and called a real reject "rc=0".
 * This file fork/execs unisacc.com and takes the status from waitpid.
 *
 * Exit 0 when: two-file globals are not merged, a same-file redefinition
 * exits non-zero, two mains say "multiple definitions", and statics stay
 * isolated. Anything else prints STILL BROKEN and exits 1.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

static char g_out[4096];

static int write_file(const char *path, const char *body) {
    FILE *f = fopen(path, "w");
    if (!f) return -1;
    fputs(body, f);
    fclose(f);
    return 0;
}

/* Run unisacc on srcs[0..n). Fills g_out. Returns the child's exit code,
 * or 127 if we could not even start it. */
static int run_u(const char *unisacc, char **srcs, int n) {
    int p[2];
    pid_t pid;
    int st, i;
    size_t off = 0;
    if (pipe(p) != 0) return 127;
    pid = fork();
    if (pid < 0) { close(p[0]); close(p[1]); return 127; }
    if (pid == 0) {
        /* unisacc.com is an APE (MZqF). This runtime's execv returns
         * ENOEXEC on it. /bin/sh is what the terminal uses to start it;
         * `exec` replaces the shell so waitpid still sees unisacc's status.
         * Not a C compiler. */
        char *argv[12];
        int dn;
        if (n + 5 > 12) _exit(127);
        argv[0] = (char *)"sh";
        argv[1] = (char *)"-c";
        argv[2] = (char *)"exec \"$0\" \"$@\"";
        argv[3] = (char *)unisacc;
        for (i = 0; i < n; i++) argv[4 + i] = srcs[i];
        argv[4 + n] = NULL;
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
    while (off + 1 < sizeof g_out) {
        ssize_t k = read(p[0], g_out + off, sizeof g_out - 1 - off);
        if (k < 0) { if (errno == EINTR) continue; break; }
        if (k == 0) break;
        off += (size_t)k;
    }
    g_out[off] = 0;
    close(p[0]);
    if (waitpid(pid, &st, 0) < 0) return 127;
    if (!WIFEXITED(st)) return 128;
    return WEXITSTATUS(st);
}

static void first_line(const char *src, char *dst, size_t n) {
    size_t i = 0;
    while (src[i] && src[i] != '\n' && i + 1 < n) { dst[i] = src[i]; i++; }
    dst[i] = 0;
    if (i == 0) snprintf(dst, n, "<no output>");
}

int main(void) {
    const char *u = getenv("UNISACC");
    char dir[64];
    char p1[64], p2[64], pk[64], m1[64], m2[64], d1[64], d2[64];
    char line[512];
    char *srcs[2];
    int rc, bad = 0;
    if (!u || !u[0]) u = "/Users/wjc/repos/unisacc/unisacc.com";
    snprintf(dir, sizeof dir, "/tmp/cdsh-dup-%d", (int)getpid());
    if (mkdir(dir, 0700) != 0) { printf("no temp dir\n"); return 2; }
    snprintf(p1, sizeof p1, "%s/h1.c", dir);
    snprintf(p2, sizeof p2, "%s/h2.c", dir);
    snprintf(pk, sizeof pk, "%s/k.c", dir);
    snprintf(m1, sizeof m1, "%s/m1.c", dir);
    snprintf(m2, sizeof m2, "%s/m2.c", dir);
    snprintf(d1, sizeof d1, "%s/d1.c", dir);
    snprintf(d2, sizeof d2, "%s/d2.c", dir);

    printf("duplicate-symbol probe — unisacc only\n\n");

    write_file(p1, "int shared = 1;\nvoid set1(void){ shared = 111; }\nint get1(void){ return shared; }\n");
    write_file(p2,
        "#include <stdio.h>\n"
        "int shared = 2;\n"
        "void set1(void); int get1(void);\n"
        "void set2(void){ shared = 222; }\n"
        "int get2(void){ return shared; }\n"
        "int main(void){\n"
        "  printf(\"get1=%d get2=%d\", get1(), get2());\n"
        "  set1(); printf(\" after-set1=%d/%d\", get1(), get2());\n"
        "  set2(); printf(\" after-set2=%d/%d\", get1(), get2());\n"
        "  return 0;\n}\n");
    srcs[0] = p1; srcs[1] = p2;
    rc = run_u(u, srcs, 2);
    first_line(g_out, line, sizeof line);
    printf("1. same global in two files\n   unisacc: %s (rc=%d)\n", line, rc);
    if (rc == 127 || strstr(g_out, "after-set1=111/111") || g_out[0] == 0) {
        printf("   → STILL BROKEN: unisacc merged the two globals or did not run\n\n");
        bad++;
    } else {
        printf("   → not merged\n\n");
    }

    write_file(pk, "int dup = 1;\nint dup = 2;\nint main(void){ return dup; }\n");
    srcs[0] = pk;
    rc = run_u(u, srcs, 1);
    first_line(g_out, line, sizeof line);
    printf("2. same global defined twice in one file\n   unisacc: %s (rc=%d)\n", line, rc);
    if (rc == 0 || rc == 127) {
        printf("   → STILL BROKEN: redefinition exited %d\n\n", rc);
        bad++;
    } else {
        printf("   → rejected (rc=%d)\n\n", rc);
    }

    write_file(m1, "int main(void){ return 0; }\n");
    write_file(m2, "int main(void){ return 0; }\n");
    srcs[0] = m1; srcs[1] = m2;
    rc = run_u(u, srcs, 2);
    first_line(g_out, line, sizeof line);
    printf("3. two mains\n   unisacc: %s (rc=%d)\n", line, rc);
    if (!strstr(g_out, "multiple definitions") && !strstr(g_out, "redefinition") && !strstr(g_out, "duplicate")) {
        printf("   → STILL BROKEN: message does not name a duplicate definition\n\n");
        bad++;
    } else {
        printf("   → message names the duplicate\n\n");
    }

    write_file(d1, "static int failures = 0;\nvoid bump1(void){ failures += 1; }\nint get1(void){ return failures; }\n");
    write_file(d2,
        "#include <stdio.h>\n"
        "static int failures = 0;\n"
        "void bump2(void){ failures += 10; }\n"
        "int get2(void){ return failures; }\n"
        "void bump1(void); int get1(void);\n"
        "int main(void){ bump1(); bump2(); printf(\"f1=%d f2=%d\", get1(), get2()); return 0; }\n");
    srcs[0] = d1; srcs[1] = d2;
    rc = run_u(u, srcs, 2);
    first_line(g_out, line, sizeof line);
    printf("4. colliding static\n   unisacc: %s (rc=%d)\n", line, rc);
    if (!strstr(g_out, "f1=1 f2=10")) {
        printf("   → STILL BROKEN: statics are not isolated\n");
        bad++;
    } else {
        printf("   → statics isolated\n");
    }

    /* best-effort cleanup; ignore failures */
    remove(p1); remove(p2); remove(pk); remove(m1); remove(m2); remove(d1); remove(d2);
    rmdir(dir);
    return bad ? 1 : 0;
}
