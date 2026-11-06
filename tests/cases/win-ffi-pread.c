/* W45-WIN-FFI probe: pread at offset 0 of this very file (the first three bytes are "/* ").
   Run from the directory that holds it; argv[1] may name the file instead. */
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
int main(int argc, char **argv) {
    char b[4]; int fd; long n;
    fd = open(argc > 1 ? argv[1] : "win-ffi-pread.c", O_RDONLY);
    if (fd < 0) { puts("open-failed"); return 1; }
    n = pread(fd, b, 3, 0);
    b[n > 0 ? n : 0] = 0;
    puts(b);
    return n == 3 ? 0 : 1;
}
