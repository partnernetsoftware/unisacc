#include <stdio.h>
int getpagesize(void);
int gethostname(char *name, unsigned long len);
long sysconf(int name);
int main(void) {
    char h[256]; int r;
    h[0] = 0; r = gethostname(h, sizeof h);
    printf("pagesize>0 %d hostname rc %d nonempty %d sysconf>0 %d\n", getpagesize() > 0, r, h[0] != 0, sysconf(1) > 0 || sysconf(2) > 0);
    return 0;
}
