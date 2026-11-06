/* W45-WIN-FFI pread diagnosis: why did open() fail on win/x86_64?
   Prints, for three path spellings, open()'s rc and errno; then asks the loader
   directly which names ucrtbase/kernel32 actually resolve (bare vs underscore).
   Nothing here is an assertion: it is a read-only report for the fix owner. */
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <unisacc_ffi.h>
int main(int argc, char **argv) {
    const char *p;
    int fd;
    printf("cwd-probe argc=%d\n", argc);
    p = argc > 1 ? argv[1] : "tests/cases/win-ffi-pread.c";
    errno = 0; fd = open(p, O_RDONLY);
    printf("open relative        path=%s rc=%d errno=%d\n", p, fd, errno);
    if (fd >= 0) close(fd);
    errno = 0; fd = open("C:\\Windows\\win.ini", O_RDONLY);
    printf("open absolute bs     rc=%d errno=%d\n", fd, errno);
    if (fd >= 0) close(fd);
    errno = 0; fd = open("C:/Windows/win.ini", O_RDONLY);
    printf("open absolute fwd    rc=%d errno=%d\n", fd, errno);
    if (fd >= 0) close(fd);
    p = argc > 2 ? argv[2] : "win-ffi/diag-open.c";
    errno = 0; fd = open(p, O_RDONLY);
    printf("open self            path=%s rc=%d errno=%d\n", p, fd, errno);
    if (fd >= 0) close(fd);
    {   const char *names[10] = {"open", "_open", "read", "_read", "close", "_close", "strdup", "_strdup", "lseek", "_lseek"};
        int i;
        for (i = 0; i < 10; i += 2) {
            void *a = uffi_dlsym(0, names[i]), *b = uffi_dlsym(0, names[i + 1]);
            printf("dlsym %-8s=%s  %-8s=%s\n", names[i], a ? "FOUND" : "missing", names[i + 1], b ? "FOUND" : "missing");
        }
    }
    return 0;
}
