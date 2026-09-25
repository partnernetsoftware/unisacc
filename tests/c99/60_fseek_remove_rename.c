/* fseek/ftell/rewind, rename, remove [S-15 D2].  A FILE * is its descriptor,
   so these are lseek, rename(2)/renameat2 and unlink(2)/unlinkat -- and on
   Windows SetFilePointer, MoveFileExA and DeleteFileA. */
#include <stdio.h>
int main(void) {
    FILE *f; char b[8]; long n; int r;
    f = fopen("unisa_d2_probe_a.tmp", "w");
    fputs("hello world", f);
    printf("tell after write %ld\n", ftell(f));
    fclose(f);
    f = fopen("unisa_d2_probe_a.tmp", "r");
    fseek(f, 6, SEEK_SET);
    n = fread(b, 1, 5, f); b[n] = 0;
    printf("seek 6 read [%s] tell %ld\n", b, ftell(f));
    fseek(f, -5, SEEK_END);
    n = fread(b, 1, 3, f); b[n] = 0;
    printf("seek end-5 read [%s]\n", b);
    rewind(f); n = fread(b, 1, 2, f); b[n] = 0;
    printf("rewind read [%s]\n", b);
    fclose(f);
    r = rename("unisa_d2_probe_a.tmp", "unisa_d2_probe_b.tmp");
    printf("rename %d, old gone %d\n", r, fopen("unisa_d2_probe_a.tmp", "r") == NULL);
    r = remove("unisa_d2_probe_b.tmp");
    printf("remove %d, gone %d, again %d\n", r, fopen("unisa_d2_probe_b.tmp", "r") == NULL, remove("unisa_d2_probe_b.tmp"));
    return 0;
}
