#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
int main(void){
    int p[2]; FILE *f;
    signal(13, SIG_IGN);
    if (pipe(p)) return 1;
    close(p[0]);
    f = fdopen(p[1], "w");
    if (!f) return 1;
    if (fwrite("x", 1, 1, f) != 1) return 2;
    if (errno != 0) return 3;
    if (fflush(f) == 0) return 4;
    if (errno == 0) return 5;
    return 0;
}
