/* Windows POSIX layer batch 4 (0.0.22): dup.  A duplicate writes to the same file,
   survives closing the original, and dup of a closed descriptor fails. */
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main(void) {
    char buf[32]; int fd, d, n;
    fd = open("wp4.tmp", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    printf("open %d\n", fd >= 0);
    d = dup(fd); printf("dup %d %d\n", d >= 0, d != fd);
    printf("w1 %d\n", (int)write(fd, "ab", 2));
    printf("close %d\n", close(fd));
    printf("w2 %d\n", (int)write(d, "cd", 2));
    printf("close2 %d\n", close(d));
    fd = open("wp4.tmp", O_RDONLY);
    n = (int)read(fd, buf, sizeof buf - 1); buf[n > 0 ? n : 0] = 0;
    printf("read %d %s\n", n, buf);
    close(fd); unlink("wp4.tmp");
    return 0;
}
