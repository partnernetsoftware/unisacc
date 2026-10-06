/* D3: fread returns a byte pushed back by ungetc first and sets eof on a short
   read; fclose frees the stream state so a reused descriptor starts clean. */
#include <stdio.h>
int main(void) {
    FILE *f; char b[8]; long n; int bad = 0;
    f = fopen("/tmp/unisa_d3_probe.txt", "w"); fputs("abc", f); fclose(f);
    f = fopen("/tmp/unisa_d3_probe.txt", "r");
    if (fgetc(f) != 'a') bad = bad + 1;
    ungetc('Z', f);
    n = fread(b, 1, 8, f);
    if (n != 3 || b[0] != 'Z' || b[1] != 'b' || b[2] != 'c') bad = bad + 2;
    if (!feof(f)) bad = bad + 4;
    fclose(f);
    f = fopen("/tmp/unisa_d3_probe.txt", "r");
    if (feof(f)) bad = bad + 8;
    if (fgetc(f) != 'a') bad = bad + 16;
    fclose(f); remove("/tmp/unisa_d3_probe.txt");
    printf("stream_state %d\n", bad);
    return bad != 0;
}
