#include <stdio.h>
char *strsignal(int sig);
unsigned int getuid(void);
unsigned int geteuid(void);
int main(void){ printf("%s|%d\n", strsignal(2), getuid() == geteuid()); return 0; }
