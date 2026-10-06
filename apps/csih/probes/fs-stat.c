/* 读出文件大小与 mtime。 */
#include <stdio.h>
#include <sys/stat.h>
int main(int argc, char **argv) {
    const char *p = argc > 1 ? argv[1] : "/tmp";
    struct stat s;
    if (stat(p, &s) != 0) { printf("UNAVAILABLE stat\n"); return 1; }
    printf("ok size %ld\n", (long)s.st_size);
    return 0;
}
