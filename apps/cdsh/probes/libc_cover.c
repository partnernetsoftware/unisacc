/* libc_cover.c — errno count plus "does this name compile".
 *   unisacc probes/libc_cover.c probes/u_run.c
 * Calls sit in a never-taken branch so execvp does not run. No gcc.
 * Exit 1 if any errno name or function is missing.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int u_spawn(const char *bin, char **args, int nargs, const char *cwd,
            char *out, int cap, int *rc);

static const char *errnames[] = {
    "EPERM","ENOENT","ESRCH","EINTR","EIO","ENXIO","E2BIG","ENOEXEC","EBADF",
    "ECHILD","EAGAIN","ENOMEM","EACCES","EFAULT","EBUSY","EEXIST","EXDEV",
    "ENODEV","ENOTDIR","EISDIR","EINVAL","ENFILE","EMFILE","ENOTTY","ETXTBSY",
    "EFBIG","ENOSPC","ESPIPE","EROFS","EMLINK","EPIPE","EDOM","ERANGE",
    "EDEADLK","ENAMETOOLONG","ENOLCK","ENOSYS","ENOTEMPTY","ELOOP","ENOTSUP",
    "EOVERFLOW","ECANCELED","EILSEQ",
};

static const char *fn[] = {
    "fsync","rmdir","access","readlink","symlink","dup","fdopen","rename",
    "unlink","mkdir","lseek","ftruncate","chmod","fstat","isatty","getenv",
    "getpid","fork","waitpid","getcwd","strftime","localtime_r","telldir",
    "execl","execlp","execvp","pipe","dup2",
};
static const char *call[] = {
    "(0?fsync(0):0)", "(0?rmdir(\"/tmp/nope\"):0)", "(0?access(\"/tmp\",0):0)",
    "(0?readlink(\"/tmp\",0,0):0)", "(0?symlink(\"a\",\"b\"):0)", "(0?dup(0):0)",
    "(0?fdopen(0,\"r\"):0)", "(0?rename(\"/tmp/a\",\"/tmp/b\"):0)",
    "(0?unlink(\"/tmp/nope\"):0)", "(0?mkdir(\"/tmp/nope\",0755):0)",
    "(0?lseek(0,0,0):0)", "(0?ftruncate(0,0):0)", "(0?chmod(\"x\",0644):0)",
    "(0?fstat(0,0):0)", "isatty(0)", "getenv(\"PATH\")", "getpid()",
    "(0?fork():0)", "(0?waitpid(0,0,0):0)", "(0?getcwd(0,0):0)",
    "(0?strftime(0,0,\"\",0):0)", "(0?localtime_r(0,0):0)", "(0?telldir(0):0)",
    "execl(\"/bin/sh\",\"sh\",\"-c\",\"true\",(char*)0)",
    "execlp(\"sh\",\"sh\",\"-c\",\"true\",(char*)0)",
    "(0?execvp(\"sh\",av):0)", "(0?pipe(pfd):0)", "(0?dup2(0,0):0)",
};

static int is_num(const char *s) {
    if (!s || !*s) return 0;
    while (*s && *s != '\n') { if (*s < '0' || *s > '9') return 0; s++; }
    return 1;
}

int main(void) {
    const char *u = getenv("UNISACC");
    char dir[64], src[80], body[1200], out[800];
    int i, n, rc, miss = 0, have = 0, fmiss = 0;
    FILE *f;
    char *args[1];
    if (!u || !u[0]) u = "/Users/wjc/repos/unisacc/unisacc.com";
    snprintf(dir, sizeof dir, "/tmp/cdsh-libc-%d", (int)getpid());
    if (mkdir(dir, 0700) != 0) { printf("no temp\n"); return 2; }
    snprintf(src, sizeof src, "%s/c.c", dir);
    args[0] = src;
    n = (int)(sizeof errnames / sizeof errnames[0]);
    printf("── errno constants ──\n");
    printf("  MISSING:");
    for (i = 0; i < n; i++) {
        snprintf(body, sizeof body,
            "#include <stdio.h>\n#include <errno.h>\nint main(void){ printf(\"%%d\\n\",(int)%s); return 0; }\n",
            errnames[i]);
        f = fopen(src, "w"); if (!f) { miss++; printf(" %s", errnames[i]); continue; }
        fputs(body, f); fclose(f);
        u_spawn(u, args, 1, NULL, out, (int)sizeof out, &rc);
        if (rc == 0 && is_num(out)) have++;
        else { miss++; printf(" %s", errnames[i]); }
    }
    printf("\n  defined: %d / %d\n", have, n);

    printf("── functions ──\n");
    n = (int)(sizeof fn / sizeof fn[0]);
    for (i = 0; i < n; i++) {
        snprintf(body, sizeof body,
            "#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n"
            "#include <unistd.h>\n#include <time.h>\n#include <dirent.h>\n"
            "#include <errno.h>\n#include <fcntl.h>\n#include <signal.h>\n"
            "#include <sys/stat.h>\n#include <sys/types.h>\n#include <sys/wait.h>\n"
            "int main(void){ char *av[2]; int pfd[2]; av[0]=(char*)\"sh\"; av[1]=0; (void)%s; return 0; }\n",
            call[i]);
        f = fopen(src, "w"); if (!f) { fmiss++; printf("  MISSING %s\n", fn[i]); continue; }
        fputs(body, f); fclose(f);
        u_spawn(u, args, 1, NULL, out, (int)sizeof out, &rc);
        if (rc == 0 && !strstr(out, "undefined function") && !strstr(out, "not covered") && !strstr(out, "error"))
            printf("  ok       %s\n", fn[i]);
        else {
            fmiss++;
            printf("  MISSING  %s %s\n", fn[i], out);
        }
    }
    printf("Functions listed: %d. Missing: %d. Errno missing: %d.\n",
           (int)(sizeof fn / sizeof fn[0]), fmiss, miss);
    remove(src); rmdir(dir);
    return (miss || fmiss) ? 1 : 0;
}
