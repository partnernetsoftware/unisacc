/* Windows POSIX layer batch 1 (0.0.21): unistd/fcntl/sys/stat/time over the
   system.  Portable: prints only deterministic facts, so the same probe runs
   under hosthdr (macOS/Linux, against cc/gcc) and tests/winposix.sh (Windows
   images against the macOS cc build). */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>

int main(void) {
    char buf[512]; int fd; long n; struct stat st; struct timespec a, b; time_t t;
#ifdef _WIN32
    chdir("C:\\u");                    /* the guest's scratch directory */
#endif
    unlink("wp_f.txt"); unlink("wp_g.txt"); rmdir("wp_d");
    printf("getpid>0 %d\n", getpid() > 0);
    printf("getcwd %d\n", getcwd(buf, sizeof buf) != 0 && buf[0] != 0);
    fd = open("wp_f.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    printf("open %d\n", fd >= 0);
    n = write(fd, "hello world\n", 12);
    printf("write %ld\n", n);
    printf("fstat %d", fstat(fd, &st) == 0); printf(" reg %d size %ld\n", S_ISREG(st.st_mode) != 0, (long)st.st_size);
    printf("close %d\n", close(fd));
    fd = open("wp_f.txt", O_WRONLY | O_APPEND, 0);
    write(fd, "more\n", 5); close(fd);
    printf("stat %d", stat("wp_f.txt", &st)); printf(" reg %d dir %d size %ld\n", S_ISREG(st.st_mode) != 0, S_ISDIR(st.st_mode) != 0, (long)st.st_size);
    fd = open("wp_f.txt", O_RDONLY, 0); memset(buf, 0, sizeof buf);
    n = read(fd, buf, sizeof buf - 1); close(fd);
    printf("read %ld [%s]\n", n, buf[16] == '\n' ? "ok" : "bad");
    errno = 0; fd = open("wp_f.txt", O_WRONLY | O_CREAT | O_EXCL, 0644);
    printf("excl %d eexist %d\n", fd, errno == EEXIST);
    printf("access F %d W %d\n", access("wp_f.txt", F_OK), access("wp_f.txt", W_OK));
    errno = 0; printf("access missing %d enoent %d\n", access("wp_none.txt", F_OK), errno == ENOENT);
    printf("rename %d\n", rename("wp_f.txt", "wp_g.txt"));
    printf("stat old %d new %d\n", stat("wp_f.txt", &st), stat("wp_g.txt", &st));
    printf("unlink %d again %d\n", unlink("wp_g.txt"), unlink("wp_g.txt"));
    printf("mkdir %d", mkdir("wp_d", 0755)); printf(" again %d eexist %d\n", mkdir("wp_d", 0755), errno == EEXIST);
    printf("stat dir %d", stat("wp_d", &st)); printf(" isdir %d\n", S_ISDIR(st.st_mode) != 0);
    printf("chdir %d", chdir("wp_d")); printf(" back %d\n", chdir(".."));
    printf("rmdir %d again %d\n", rmdir("wp_d"), rmdir("wp_d"));
    clock_gettime(CLOCK_MONOTONIC, &a); usleep(20000); clock_gettime(CLOCK_MONOTONIC, &b);
    printf("monotonic %d\n", (b.tv_sec - a.tv_sec) * 1000000000L + (b.tv_nsec - a.tv_nsec) >= 15000000L);
    a.tv_sec = 0; a.tv_nsec = 1000000; printf("nanosleep %d\n", nanosleep(&a, 0));
    clock_gettime(CLOCK_REALTIME, &a); t = time(0);
    printf("realtime %d time %d\n", a.tv_sec > 1700000000L, t >= a.tv_sec && t - a.tv_sec < 5);
    printf("sleep %u\n", sleep(0));
    isatty(0);
    return 0;
}
