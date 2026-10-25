/* D3: fread returns a byte pushed back by ungetc first and sets eof on a short
   read; fclose frees the stream state so a reused descriptor starts clean.
   tmpfile, not a fixed path: ccrun runs two routes of one probe at once. */
#include <stdio.h>
int main(void) {
    FILE *f; char b[8]; long n; int bad = 0;
    f = tmpfile(); fputs("abc", f); rewind(f);
    if (fgetc(f) != 'a') bad = bad + 1;
    ungetc('Z', f);
    n = fread(b, 1, 8, f);
    if (n != 3 || b[0] != 'Z' || b[1] != 'b' || b[2] != 'c') bad = bad + 2;
    if (!feof(f)) bad = bad + 4;
    fclose(f);
    f = tmpfile(); fputs("a", f); rewind(f);   /* likely the same descriptor */
    if (feof(f)) bad = bad + 8;
    if (fgetc(f) != 'a') bad = bad + 16;
    fclose(f);
    printf("stream_state %d\n", bad);
    return bad != 0;
}
