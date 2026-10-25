/* fopen/fwrite/fgetc/fgets over the real open(2).  Linux/arm64 has no
   `open` syscall -- it is `openat`, with a directory fd in front -- and the
   O_* bit values differ between Linux and the BSDs, so this probe only
   passes if both of those are right on every target. */
#include <stdio.h>
int main(void) {
    FILE *f;
    char line[16];
    int c;
    int n;
    f = fopen("u_probe.txt", "w");
    if (f == NULL) { printf("no write\n"); return 1; }
    fwrite("ab\ncd\n", 1, 6, f);
    fclose(f);
    f = fopen("u_probe.txt", "r");
    if (f == NULL) { printf("no read\n"); return 1; }
    n = 0;
    while ((c = fgetc(f)) != EOF) n = n + c;
    fclose(f);
    printf("sum %d\n", n);
    f = fopen("u_probe.txt", "r");
    while (fgets(line, 16, f) != NULL) printf("line: %s", line);
    fclose(f);
    return 0;
}
