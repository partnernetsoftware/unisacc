/* fwrite probe, REAL: run under `ulimit -f 1` with SIGXFSZ ignored.
   3000 B then 5000 B cannot both fit; the probe must not report complete
   success for both.  argv[1] is the file to write. */
#include <stdio.h>
static char a[3000];
static char b[5000];
int main(int argc, char **argv) {
    FILE *f; long r1; long r2; long i;
    for (i = 0; i < 3000; i++) a[i] = 'a';
    for (i = 0; i < 5000; i++) b[i] = 'b';
    f = fopen(argv[1], "w");
    if (!f) { printf("open failed\n"); return 2; }
    r1 = fwrite(a, 1, 3000, f);
    r2 = fwrite(b, 1, 5000, f);
    fclose(f);
    if (r1 == 3000 && r2 == 5000) { printf("r1=%ld r2=%ld FALSE-SUCCESS\n", r1, r2); return 1; }
    printf("r1=%ld r2=%ld error-reported\n", r1, r2);
    return 0;
}
