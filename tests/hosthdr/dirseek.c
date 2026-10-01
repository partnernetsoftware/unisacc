/* 0.0.19 (dsh): telldir/seekdir/rewinddir return to the same entry */
#include <stdio.h>
#include <string.h>
#include <dirent.h>
int main(void) {
    DIR *d; struct dirent *e; long pos; char first[256]; char again[256]; int n; int m;
    d = opendir("/");
    if (!d) { printf("opendir failed\n"); return 1; }
    readdir(d); pos = telldir(d);
    e = readdir(d); strcpy(first, e ? e->d_name : "-");
    n = 0; while (readdir(d)) n++;
    seekdir(d, pos); e = readdir(d); strcpy(again, e ? e->d_name : "-");
    rewinddir(d); m = 0; while (readdir(d)) m++;
    printf("seekdir same %d rewind count same %d\n", strcmp(first, again) == 0, m == n + 2);
    closedir(d);
    return 0;
}
