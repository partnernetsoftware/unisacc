/* vsnprintf(NULL, 0, fmt, ap) only measures, and vprintf writes to stdout.  The carried
   libc decided on the buffer, not the stream: with no buffer the bytes went to FILE * 0,
   descriptor 0 -- a write to stdin, which blocked when nobody read it (the reported hang
   once the string reached a few hundred bytes) and lost vprintf's output.  Descriptor 0
   is a file here (open takes the lowest free descriptor), so any stray byte is seen;
   a lost vprintf line shows against the reference's output. */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#ifndef _WIN32   /* the descriptor trap is POSIX; Windows keeps the value checks */
#include <fcntl.h>
#include <unistd.h>
#endif
static int measure(const char *fmt, ...) {
    va_list ap; int n;
    va_start(ap, fmt); n = vsnprintf(NULL, 0, fmt, ap); va_end(ap);
    return n;
}
static int into(char *b, long cap, const char *fmt, ...) {
    va_list ap; int n;
    va_start(ap, fmt); n = vsnprintf(b, cap, fmt, ap); va_end(ap);
    return n;
}
static void say(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt); vprintf(fmt, ap); va_end(ap);
}
int main(void) {
    static char s[1001]; char small[8]; const char *fmt = "%s|%d"; int bad = 0, n, k;
#ifndef _WIN32
    int fd;
    close(0);
    fd = open("vsnprintf_measure.tmp", O_RDWR | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) return 99;
#endif
    memset(s, 'x', 1000); s[1000] = 0;
    for (k = 250; k <= 1000; k += 250) { s[k] = 0; if (measure("%s", s) != k) bad |= 1; s[k] = 'x'; }
    s[1000] = 0;
    if (snprintf(NULL, 0, fmt, s, 42) != 1003) bad |= 2;        /* a run-time format */
    n = into(small, sizeof small, "%s", s);
    if (n != 1000 || strcmp(small, "xxxxxxx") != 0) bad |= 4;
    if (into(small, 0, "%d", 12345) != 5) bad |= 4;
    say("vprintf %d %s\n", 7, "ok");
    fflush(stdout);
#ifndef _WIN32
    if (lseek(fd, 0, SEEK_END) != 0) bad |= 8;                      /* nothing reached descriptor 0 */
    close(fd); unlink("vsnprintf_measure.tmp");
#endif
    printf("vsnprintf bad=%d\n", bad);
    return bad;
}
