/* R18-10: termios constants and cfmakeraw agree with the host's (no tty needed). */
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <sys/ioctl.h>
int main(void) {
    struct termios t; memset(&t, 0, sizeof t);
    t.c_iflag = ICRNL | IXON | BRKINT; t.c_oflag = OPOST; t.c_lflag = ECHO | ICANON | ISIG | IEXTEN; t.c_cflag = PARENB;
    cfmakeraw(&t);
    printf("iflag %lx oflag %lx lflag %lx cs8 %d parenb %d vmin %d vtime %d\n",
           (unsigned long)t.c_iflag, (unsigned long)t.c_oflag, (unsigned long)t.c_lflag,
           (t.c_cflag & CSIZE) == CS8, (t.c_cflag & PARENB) != 0, t.c_cc[VMIN], t.c_cc[VTIME]);
    printf("TCSANOW %d TCSADRAIN %d TCSAFLUSH %d TIOCGWINSZ %lx\n", TCSANOW, TCSADRAIN, TCSAFLUSH, (unsigned long)TIOCGWINSZ);
    printf("tcgetattr on a non-tty fails: %d\n", tcgetattr(-1, &t) != 0);
    return 0;
}
