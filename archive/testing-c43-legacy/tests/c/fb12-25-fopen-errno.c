/* R13-0b: external trial of 0.0.12, real-world batch.  Source: ~/unisacc-feedback-0.0.12-realworld/25-fopen-errno.c (+ .out.txt for the gcc-vs-unisacc record). */
/* fopen() failure does not set errno (the __open gate returns -errno but
   the bundled fopen just returns NULL), so perror/strerror(errno) print
   "Success".  sbase wc/cat etc. report "fopen /nonexistent: Success". */
#include <stdio.h>
#include <errno.h>
#include <string.h>
int main(void) {
    FILE *f;
    errno = 0;
    f = fopen("/nonexistent/file", "r");
    printf("f=%d errno_is_ENOENT=%d msg=%s\n", f != NULL, errno == ENOENT, strerror(errno));
    return 0;
}
