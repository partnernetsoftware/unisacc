/* C99 7.19.9.1 fgetpos and 7.19.9.3 fsetpos: save the position after reading
 * five bytes of a file this program writes, read on, restore the position and
 * read the same bytes again; fgetpos at the start agrees with ftell 0 and
 * fsetpos to it reads the first byte.  The file is removed before exit.
 * The exit code is 0 when every check passes; each set bit names a failed
 * check.  Nothing is printed. */
#include <stdio.h>
#include <string.h>
int main(void)
{
    FILE *f;
    fpos_t start, mid;
    char b[16];
    int sum = 0;
    f = fopen("unisa_fpos_probe.tmp", "w");
    if (f == NULL) return 99;
    fputs("hello world", f);
    fclose(f);
    f = fopen("unisa_fpos_probe.tmp", "r");
    if (f == NULL) return 98;
    sum += (fgetpos(f, &start) == 0 && ftell(f) == 0) * 1;
    memset(b, 0, sizeof b);
    sum += (fread(b, 1, 5, f) == 5 && strcmp(b, "hello") == 0 && fgetpos(f, &mid) == 0 && ftell(f) == 5) * 2;
    memset(b, 0, sizeof b);
    sum += (fread(b, 1, 6, f) == 6 && strcmp(b, " world") == 0 && ftell(f) == 11) * 4;
    memset(b, 0, sizeof b);
    sum += (fsetpos(f, &mid) == 0 && ftell(f) == 5 && fread(b, 1, 6, f) == 6 && strcmp(b, " world") == 0) * 8;
    memset(b, 0, sizeof b);
    sum += (fsetpos(f, &start) == 0 && ftell(f) == 0 && fread(b, 1, 1, f) == 1 && b[0] == 'h') * 16;
    sum += (fsetpos(f, &mid) == 0 && fgetc(f) == ' ' && fgetc(f) == 'w') * 32;
    fclose(f);
    sum += (remove("unisa_fpos_probe.tmp") == 0) * 64;
    return 127 - sum;
}
