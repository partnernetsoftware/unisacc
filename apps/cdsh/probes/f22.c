/* f22.c — one snippet per libc call. Judge by the line it prints.
 *   unisacc probes/f22.c probes/u_run.c
 * No gcc. Each snippet runs in its own empty directory.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int u_spawn(const char *bin, char **args, int nargs, const char *cwd,
            char *out, int cap, int *rc);

static const char *body[] = {
    "#include <stdio.h>\n#include <stdlib.h>\nint main(void){ void*p=malloc(8); void*q=calloc(1,8); void*r=realloc(p,16); free(q); free(r); printf(\"malloc ok\\n\"); return 0; }\n",
    "#include <stdio.h>\n#include <string.h>\nint main(void){ char b[8]; memset(b,0,8); memcpy(b,\"ab\",2); printf(\"mem ok %d\\n\",(int)strlen(b)); return 0; }\n",
    "#include <stdio.h>\n#include <sys/stat.h>\nint main(void){ struct stat s; int rc=stat(\".\",&s); printf(\"stat ok rc=%d big=%d\\n\",rc,(int)(s.st_size>0)); return 0; }\n",
    "#include <stdio.h>\n#include <sys/stat.h>\nint main(void){ struct stat s; int rc=lstat(\".\",&s); printf(\"lstat ok rc=%d\\n\",rc); return 0; }\n",
    "#include <stdio.h>\n#include <sys/stat.h>\n#include <fcntl.h>\n#include <unistd.h>\nint main(void){ int fd=open(\"w.tmp\",O_CREAT|O_WRONLY,0644); if(fd<0){printf(\"open FAILED\\n\");return 1;} write(fd,\"x\",1); close(fd); printf(\"open/write/close ok\\n\"); return 0; }\n",
    "#include <stdio.h>\n#include <sys/stat.h>\n#include <unistd.h>\nint main(void){ rmdir(\"d1\"); int rc=mkdir(\"d1\",0755); printf(\"mkdir ok %d\\n\",rc); return 0; }\n",
    "#include <stdio.h>\n#include <unistd.h>\nint main(void){ int rc=unlink(\"w.tmp\"); printf(\"unlink ok %d\\n\",rc); return 0; }\n",
    "#include <stdio.h>\n#include <dirent.h>\nint main(void){ DIR*d=opendir(\".\"); struct dirent*e; if(!d){printf(\"opendir FAILED\\n\");return 1;} e=readdir(d); printf(\"opendir+readdir ok %d\\n\", e?1:0); return 0; }\n",
    "#include <stdio.h>\n#include <unistd.h>\nint main(void){ printf(\"isatty ok %d\\n\", isatty(1)); return 0; }\n",
    "#include <stdio.h>\n#include <termios.h>\nint main(void){ struct termios t; tcgetattr(0,&t); cfmakeraw(&t); printf(\"tcgetattr+cfmakeraw ok\\n\"); return 0; }\n",
    "#include <stdio.h>\n#include <sys/ioctl.h>\nint main(void){ struct winsize w; int rc=ioctl(1,TIOCGWINSZ,&w); printf(\"ioctl ok rc=%d\\n\",rc); return 0; }\n",
    "#include <stdio.h>\n#include <poll.h>\nint main(void){ struct pollfd p; p.fd=1;p.events=POLLOUT;p.revents=0; printf(\"poll ok %d\\n\",poll(&p,1,0)); return 0; }\n",
    "#include <stdio.h>\n#include <signal.h>\nstatic volatile sig_atomic_t g=0;\nstatic void h(int s){(void)s;g=1;}\nint main(void){ signal(SIGINT,h); raise(SIGINT); printf(\"signal ok %d\\n\",(int)g); return 0; }\n",
    "#include <stdio.h>\n#include <unistd.h>\nint main(void){ printf(\"fork ok %d\\n\", (int)(fork()>=0)); return 0; }\n",
    "#include <stdio.h>\n#include <unistd.h>\nint main(void){ printf(\"getpid ok %d\\n\", (int)(getpid()>0)); return 0; }\n",
    "#include <stdio.h>\n#include <string.h>\n#include <unistd.h>\nint main(void){ char b[256]; char *r = getcwd(b,256); printf(\"getcwd ok %d abs %d len %d\\n\", r?1:0, (r&&b[0]==47)?1:0, r?(int)strlen(b)>1:0); return 0; }\n",
    "#include <stdio.h>\n#include <time.h>\nint main(void){ struct timespec ts; clock_gettime(CLOCK_REALTIME,&ts); printf(\"clock ok %d\\n\",(int)(ts.tv_sec>0)); return 0; }\n",
    "#include <stdio.h>\n#include <dirent.h>\nint main(void){ DIR*d=opendir(\".\"); long l; if(!d){printf(\"FAIL\\n\");return 1;} l=telldir(d); printf(\"telldir ok %d\\n\",(int)(l>=0)); return 0; }\n",
};
static const char *expect[] = {
    "malloc ok", "mem ok 2", "stat ok rc=0 big=1", "lstat ok rc=0",
    "open/write/close ok", "mkdir ok 0", "unlink ok -1", "opendir+readdir ok 1",
    "isatty ok 0", "tcgetattr+cfmakeraw ok", "ioctl ok rc=-1", "poll ok 1",
    "signal ok 1", "fork ok 1", "getpid ok 1", "getcwd ok 1 abs 1 len 1",
    "clock ok 1", "telldir ok 1",
};

int main(void) {
    const char *u = getenv("UNISACC");
    char base[64], dir[80], src[96], out[1024], line[160];
    int i, n, rc, red = 0, same = 0;
    FILE *f;
    if (!u || !u[0]) u = "/Users/wjc/repos/unisacc/unisacc.com";
    n = (int)(sizeof expect / sizeof expect[0]);
    snprintf(base, sizeof base, "/tmp/cdsh-f22-%d", (int)getpid());
    if (mkdir(base, 0700) != 0) { printf("no temp\n"); return 2; }
    for (i = 0; i < n; i++) {
        char *args[1];
        size_t k = 0;
        snprintf(dir, sizeof dir, "%s/%d", base, i);
        snprintf(src, sizeof src, "%s/t.c", dir);
        if (mkdir(dir, 0700) != 0) { red++; printf("  red     mkdir\n"); continue; }
        f = fopen(src, "w");
        if (!f) { red++; continue; }
        fputs(body[i], f);
        fclose(f);
        args[0] = src;
        u_spawn(u, args, 1, dir, out, (int)sizeof out, &rc);
        while (out[k] && out[k] != '\n' && k + 1 < sizeof line) { line[k] = out[k]; k++; }
        line[k] = 0;
        if (strncmp(line, expect[i], strlen(expect[i])) == 0) {
            same++;
            printf("  same    %s\n", line);
        } else {
            red++;
            printf("  red     want=%s got=%s\n", expect[i], line[0] ? line : "<no output>");
        }
        remove(src);
        rmdir(dir);
    }
    rmdir(base);
    printf("\n  %d same, %d red\n", same, red);
    return red ? 1 : 0;
}
