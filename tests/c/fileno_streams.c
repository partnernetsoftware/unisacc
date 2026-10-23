/* fileno (POSIX) names the descriptor of a stream; the carried libc had none
   ("undefined function 'fileno'" under -run and when compiling). */
#include <stdio.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif
int main(void) {
    FILE *f; int fd, bad = 0;
    if (fileno(stdin) != 0 || fileno(stdout) != 1 || fileno(stderr) != 2) bad |= 1;
    f = fopen("fileno_streams.tmp", "w+");
    if (!f) return 99;
    fd = fileno(f);
    if (fd <= 2) bad |= 2;
    fputs("abc", f); fflush(f);
#ifndef _WIN32   /* the descriptor is the stream's: read back through it */
    {   char buf[8];
        if (lseek(fd, 0, SEEK_SET) != 0 || read(fd, buf, 3) != 3 || memcmp(buf, "abc", 3) != 0) bad |= 4; }
#endif
    fclose(f); remove("fileno_streams.tmp");
    printf("fileno bad=%d\n", bad);
    return bad;
}
