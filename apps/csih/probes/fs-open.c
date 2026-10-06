/* 以 O_CREAT|O_WRONLY 建文件并写入。 */
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
int main(void) {
    int fd = open("/tmp/cdsh-probe.tmp", O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) { printf("UNAVAILABLE open\n"); return 1; }
    ssize_t n = write(fd, "x", 1);
    close(fd);
    if (n != 1) { printf("UNAVAILABLE write\n"); return 1; }
    printf("ok wrote %ld\n", (long)n);
    return 0;
}
