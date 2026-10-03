/* examples/win/posix.c -- the wrapped route: POSIX names on Windows.
 *
 * Windows has two supported routes, and this probe is route two. A program
 * written against open/read/write/stat/opendir/clock_gettime compiles for
 * win/x86_64 and win/arm64 unchanged, and runs on Windows -- no Win32 header,
 * no user32, no kernel32 prototype. The layer underneath is thin: on Windows
 * an fd IS the HANDLE value, and the wide Win32 calls it needs (CreateFileA is
 * seven arguments, which no forward can make -- see outparam.c) are reached
 * through the compiler's own PE import list instead of through a forward.
 *
 * That is the whole reason both routes can coexist: the import table has no
 * argument-count limit but a fixed name list, and a forward reaches any name
 * in four DLLs but only four arguments. Wide calls belong to the library,
 * thin wrappers belong to the forward.
 *
 * What this probe does NOT cover, measured on this machine, so do not assume:
 * fileno, fork, execvp, wait, waitpid, socket, strdup, poll, ioctl and struct
 * winsize are refused by name on a win target. See examples/win/README.md.
 *
 * Build and run:
 *   out/ua-ref-win.exe -b win/x86_64 -o posix.exe examples/win/posix.c
 *   ./posix.exe
 */
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <setjmp.h>
#include <sys/stat.h>

static jmp_buf jb;

static void boom(void)
{
    longjmp(jb, 7);
}

int main(void)
{
    struct stat st;
    struct timespec ts;
    struct dirent *ent;
    DIR *dir;
    char buf[32];
    FILE *f;
    int fd;
    int n;
    int entries;
    int v;

    /* Route two, end to end: open, write, lseek, read, close. */
    fd = open("unisacc-win-posix.tmp", O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd < 0) {
        printf("open failed\n");
        return 1;
    }
    if (write(fd, "hello", 5) != 5) {
        printf("write failed\n");
        return 1;
    }
    lseek(fd, 0, 0);
    n = (int)read(fd, buf, 32);
    close(fd);
    buf[n > 0 ? n : 0] = 0;
    printf("read back [%s]\n", buf);

    if (stat("unisacc-win-posix.tmp", &st) == 0) {
        printf("stat size %lu\n", (unsigned long)st.st_size);
        remove("unisacc-win-posix.tmp");
    } else {
        printf("stat failed\n");
    }

    /* Directory iteration under a POSIX name, not FindFirstFile. */
    entries = 0;
    dir = opendir(".");
    if (dir != 0) {
        while ((ent = readdir(dir)) != 0) {
            entries = entries + 1;
        }
        closedir(dir);
    }
    printf("dir entries %d\n", entries);

    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        printf("monotonic %lu\n", (unsigned long)ts.tv_sec);
    }
    nanosleep(&(struct timespec){0, 1000000}, 0);
    printf("pid %d cwd changed %d\n", getpid(), chdir(".") == 0);

    v = setjmp(jb);
    if (v == 0) {
        boom();
    }
    printf("setjmp %d\n", v);
    f = 0;
    printf("stdio still fine %d\n", f == 0);
    return 0;
}