#include <stdio.h>
char *strsignal(int sig);
#include <unistd.h>   /* 0.0.36 M1: getuid/geteuid are bundled system calls now; strsignal is the forward */
int main(void){ printf("%s|%d\n", strsignal(2), getuid() == geteuid()); return 0; }
