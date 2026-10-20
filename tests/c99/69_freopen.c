/* freopen [C99 7.19.5.4]: open a first temporary file, freopen the same
   FILE * onto a second one, write there, and read the second file back. */
#include <stdio.h>
#include <string.h>
int main(void) {
    FILE *f, *g; char b[16]; size_t n;
    f = fopen("unisa_freopen_a.tmp", "w");
    if (f == 0) { puts("fopen a failed"); return 1; }
    fputs("first", f);
    g = freopen("unisa_freopen_b.tmp", "w", f);
    if (g == 0) { puts("freopen failed"); return 2; }
    fputs("second", g);
    fclose(g);
    g = fopen("unisa_freopen_b.tmp", "r");
    if (g == 0) { puts("fopen b failed"); return 3; }
    n = fread(b, 1, sizeof b - 1, g); b[n] = 0;
    fclose(g);
    printf("b holds [%s]\n", b);
    remove("unisa_freopen_a.tmp");
    remove("unisa_freopen_b.tmp");
    return strcmp(b, "second") == 0 ? 0 : 4;
}
