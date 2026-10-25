/* D3: buffered reads keep fgetc/fread/ftell/fseek/ungetc/rewind consistent
   across the 4 KB buffer edge, and many small reads cost few system calls.
   tmpfile, not a fixed path: ccrun runs two routes of one probe at once. */
#include <stdio.h>
int main(void) {
    FILE *f; long i, s, n, t1, t2, t3, t4; char b[6000]; int c;
    f = tmpfile();
    for (i = 0; i < 10000; i++) fputc('a' + (int)(i % 23), f);
    rewind(f);
    s = 0; for (i = 0; i < 4090; i++) s = s + fgetc(f);
    t1 = ftell(f);
    n = fread(b, 1, 20, f);                    /* crosses the buffer edge */
    t2 = ftell(f);
    c = fgetc(f); ungetc(c, f); t3 = ftell(f);
    fseek(f, -5, SEEK_CUR); c = fgetc(f);
    n = n + (long)fread(b, 1, 6000, f);        /* larger than a buffer */
    t4 = ftell(f);
    fseek(f, 9998, SEEK_SET); n = n + (long)fread(b, 1, 10, f);
    printf("%ld %ld %ld %ld %ld %d %ld %d\n", s, t1, t2, t3, t4, c, n, feof(f) != 0);
    rewind(f); s = 0; while ((c = fgetc(f)) != EOF) s = s + c;
    printf("%ld %ld %d\n", s, ftell(f), feof(f) != 0);
    fclose(f);
    return 0;
}
