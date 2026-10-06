/* 印出行列数。 */
#include <stdio.h>
#if defined(__linux__) || defined(__APPLE__)
#  include <sys/ioctl.h>
#  include <unistd.h>
    /* TIOCGWINSZ in a struct winsize */
#endif
int main(void) {
#if defined(__linux__) || defined(__APPLE__)
    struct winsize ws;
    if (ioctl(1, TIOCGWINSZ, &ws) != 0) { printf("UNAVAILABLE ioctl TIOCGWINSZ\n"); return 1; }
    printf("ok rows %u cols %u\n", (unsigned)ws.ws_row, (unsigned)ws.ws_col);
    return 0;
#else
    printf("UNAVAILABLE get console size is not POSIX on this target\n");
    return 1;
#endif
}
