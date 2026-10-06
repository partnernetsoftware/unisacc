/* errno_cover.c — which errno names compile and print a number.
 *   unisacc probes/errno_cover.c probes/u_run.c
 * Exit 0 only when every name below prints a number. No gcc.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int u_spawn(const char *bin, char **args, int nargs, const char *cwd,
            char *out, int cap, int *rc);

static const char *names[] = {
    "EPERM","ENOENT","ESRCH","EINTR","EIO","ENXIO","E2BIG","ENOEXEC","EBADF",
    "ECHILD","EAGAIN","ENOMEM","EACCES","EFAULT","EBUSY","EEXIST","EXDEV",
    "ENODEV","ENOTDIR","EISDIR","EINVAL","ENFILE","EMFILE","ENOTTY","ETXTBSY",
    "EFBIG","ENOSPC","ESPIPE","EROFS","EMLINK","EPIPE","EDOM","ERANGE",
    "EDEADLK","ENAMETOOLONG","ENOLCK","ENOSYS","ENOTEMPTY","ELOOP","ENOTSUP",
    "EOVERFLOW","ECANCELED","EILSEQ",
};

static int is_num(const char *s) {
    if (!s || !*s) return 0;
    while (*s && *s != '\n') { if (*s < '0' || *s > '9') return 0; s++; }
    return 1;
}

int main(void) {
    const char *u = getenv("UNISACC");
    char dir[64], src[80], body[160], out[512];
    int i, n, rc, have = 0, miss = 0;
    FILE *f;
    if (!u || !u[0]) u = "/Users/wjc/repos/unisacc/unisacc.com";
    n = (int)(sizeof names / sizeof names[0]);
    snprintf(dir, sizeof dir, "/tmp/cdsh-errno-%d", (int)getpid());
    if (mkdir(dir, 0700) != 0) { printf("no temp\n"); return 2; }
    snprintf(src, sizeof src, "%s/c.c", dir);
    printf("MISSING:");
    for (i = 0; i < n; i++) {
        char *args[1];
        snprintf(body, sizeof body,
            "#include <stdio.h>\n#include <errno.h>\nint main(void){ printf(\"%%d\\n\",(int)%s); return 0; }\n",
            names[i]);
        f = fopen(src, "w");
        if (!f) { miss++; printf(" %s", names[i]); continue; }
        fputs(body, f); fclose(f);
        args[0] = src;
        u_spawn(u, args, 1, NULL, out, (int)sizeof out, &rc);
        if (rc == 0 && is_num(out)) have++;
        else { miss++; printf(" %s", names[i]); }
    }
    remove(src); rmdir(dir);
    printf("\nunisacc errno coverage: %d defined, %d missing (of %d)\n", have, miss, n);
    return miss ? 1 : 0;
}
