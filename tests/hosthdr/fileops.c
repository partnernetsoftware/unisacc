/* 0.0.19 (dsh): fsync, dup, access, symlink, readlink, rmdir, fdopen, EINTR */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
int main(void) {
    char buf[64]; long n; int fd; int d; FILE *f;
    fd = open("fo.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    printf("write %ld fsync %d\n", (long)write(fd, "abc\n", 4), fsync(fd));
    d = dup(fd); printf("dup ok %d\n", d > fd);
    close(d); close(fd);
    printf("access exists %d missing %d errno-enoent %d\n", access("fo.txt", F_OK), access("nope.txt", R_OK), errno == ENOENT);
    unlink("fo.link");
    printf("symlink %d\n", symlink("fo.txt", "fo.link"));
    n = readlink("fo.link", buf, sizeof buf - 1); if (n >= 0) buf[n] = 0;
    printf("readlink %ld %s\n", n, n >= 0 ? buf : "-");
    mkdir("fo.dir", 0755); printf("rmdir %d again %d enoent %d\n", rmdir("fo.dir"), rmdir("fo.dir"), errno == ENOENT);
    fd = open("fo.txt", O_RDONLY); f = fdopen(fd, "r");
    printf("fdopen %d first %c\n", f != NULL, f ? fgetc(f) : '-');
    printf("EINTR %d EISDIR %d ENAMETOOLONG-set %d\n", EINTR, EISDIR, ENAMETOOLONG > 0);
    unlink("fo.link"); unlink("fo.txt");
    return 0;
}
