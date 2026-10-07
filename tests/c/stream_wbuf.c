/* D3': write buffering on fopen streams -- what a program can see must not
   change: ftell counts buffered bytes, fseek/fread/fclose/fflush write them
   first, a write after a read (and the seek C99 requires) lands at the read position, and a big fwrite
   keeps its order with the small writes around it.  Matches the system libc. */
#include <stdio.h>
#include <string.h>
int main(void) {
    const char *p = "stream_wbuf.tmp";
    char big[10000], back[10100];
    long i, n;
    FILE *f = fopen(p, "w+");
    if (!f) return 1;
    fputc('A', f); fputs("bc", f); fprintf(f, "%d|", 42);
    printf("tell %ld\n", ftell(f));
    for (i = 0; i < 10000; i++) big[i] = (char)('a' + i % 26);
    fwrite(big, 1, 10000, f);
    fputs("END", f);
    printf("tell %ld\n", ftell(f));
    fseek(f, 0, SEEK_SET);
    n = (long)fread(back, 1, 6, f); back[n] = 0;
    printf("head %ld %s\n", n, back);
    fseek(f, 0, SEEK_CUR);               /* C99 7.19.5.3p6: a seek between read and write */
    fputs("XY", f);                      /* lands at offset 6, after the read */
    fflush(f);
    fseek(f, 0, SEEK_END);
    printf("size %ld\n", ftell(f));
    fseek(f, 4, SEEK_SET);
    n = (long)fread(back, 1, 6, f); back[n] = 0;
    printf("mid %s\n", back);
    fclose(f);
    f = fopen(p, "r");
    n = (long)fread(back, 1, sizeof back, f);
    printf("all %ld %c%c %s %d\n", n, back[6], back[7], back + n - 3,
           memcmp(back + 8, big + 2, 9998) == 0);
    fclose(f);
    f = fopen(p, "a");
    for (i = 0; i < 5000; i++) fputc('z', f);
    fclose(f);
    f = fopen(p, "r"); fseek(f, 0, SEEK_END); printf("append %ld\n", ftell(f)); fclose(f);
    remove(p);
    return 0;
}
