/* tmpnam [C99 7.19.4.4]: tmpnam(NULL) returns a non-null name that can be
   opened for writing, written, and removed.  The name itself is not printed
   (it differs between libcs); only what was done with it. */
#include <stdio.h>
#include <string.h>
int main(void) {
    char *p = tmpnam(0);
    FILE *f;
    if (p == 0 || *p == 0 || strlen(p) >= L_tmpnam) { puts("tmpnam null"); return 1; }
    f = fopen(p, "w");
    if (f == 0) { puts("fopen failed"); return 2; }
    if (fputs("tmpnam", f) < 0) { puts("write failed"); return 3; }
    fclose(f);
    if (remove(p) != 0) { puts("remove failed"); return 4; }
    puts("tmpnam name usable, written, removed");
    return 0;
}
