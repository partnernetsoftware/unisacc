#include <stdio.h>
int _getpid(void);
unsigned int GetTickCount(void);
int lstrlenA(const char *s);
int main(void) {
    printf("pid>0 %d tick>0 %d len %d\n", _getpid() > 0, GetTickCount() > 0, lstrlenA("forwarded"));
    return 0;
}
