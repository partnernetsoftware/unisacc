#include <stdio.h>
/* feof/ferror/clearerr without fgetc: the stream-state table must still be
   compiled in when only these are referenced (D3). */
int main(void) {
    FILE *f = fopen("/dev/null", "r");
    if (!f) return 1;
    printf("%d %d\n", feof(f) != 0, ferror(f) != 0);
    clearerr(f);
    printf("%d\n", feof(f) != 0);
    return fclose(f);
}
