/* <termios.h> (0.0.18 R18-10): terminal attributes for a TUI -- raw mode,
 * echo, canonical input -- as the kernel's own ioctl requests (TCGETS/TCSETS
 * on Linux, TIOCGETA/TIOCSETA on macOS).  The struct is the kernel's layout:
 * Linux's has 19 control characters and no speed fields (the request writes
 * exactly that); macOS's has 20 and 64-bit flag words.  Windows: its console
 * API is a separate column (plans/v0.2.x.md harness). */
#ifndef _UNISA_TERMIOS_H
#define _UNISA_TERMIOS_H
#include <errno.h>
#ifndef _WIN32
typedef unsigned char cc_t;
#ifdef __APPLE__
typedef unsigned long tcflag_t;
typedef unsigned long speed_t;
#define NCCS 20
struct termios { tcflag_t c_iflag, c_oflag, c_cflag, c_lflag; cc_t c_cc[NCCS]; speed_t c_ispeed, c_ospeed; };
#define IGNBRK 0x1
#define BRKINT 0x2
#define PARMRK 0x8
#define INPCK  0x10
#define ISTRIP 0x20
#define INLCR  0x40
#define IGNCR  0x80
#define ICRNL  0x100
#define IXON   0x200
#define OPOST  0x1
#define CSIZE  0x300
#define CS8    0x300
#define PARENB 0x1000
#define ECHOE  0x2
#define ECHO   0x8
#define ECHONL 0x10
#define ISIG   0x80
#define ICANON 0x100
#define IEXTEN 0x400
#define VEOF 0
#define VINTR 8
#define VMIN 16
#define VTIME 17
#define _UNISA_TCGET 0x40487413
#define _UNISA_TCSET 0x80487414          /* + TCSANOW/TCSADRAIN/TCSAFLUSH */
#else
typedef unsigned int tcflag_t;
typedef unsigned int speed_t;
#define NCCS 19
struct termios { tcflag_t c_iflag, c_oflag, c_cflag, c_lflag; cc_t c_line; cc_t c_cc[NCCS]; };
#define IGNBRK 0000001
#define BRKINT 0000002
#define PARMRK 0000010
#define INPCK  0000020
#define ISTRIP 0000040
#define INLCR  0000100
#define IGNCR  0000200
#define ICRNL  0000400
#define IXON   0002000
#define OPOST  0000001
#define CSIZE  0000060
#define CS8    0000060
#define PARENB 0000400
#define ISIG   0000001
#define ICANON 0000002
#define ECHO   0000010
#define ECHOE  0000020
#define ECHONL 0000100
#define IEXTEN 0100000
#define VINTR 0
#define VEOF 4
#define VTIME 5
#define VMIN 6
#define _UNISA_TCGET 0x5401
#define _UNISA_TCSET 0x5402              /* + TCSANOW/TCSADRAIN/TCSAFLUSH: TCSETS/TCSETSW/TCSETSF */
#endif
#define TCSANOW   0
#define TCSADRAIN 1
#define TCSAFLUSH 2
#if !__UNISA_FTRIM_LIBC || __UN_tcgetattr
static int tcgetattr(int __u_fd, struct termios *__u_t) {
    long __u_r; __u_r = __ioctl(__u_fd, (long)_UNISA_TCGET, (char *)__u_t);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; } return 0;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_tcsetattr
static int tcsetattr(int __u_fd, int __u_act, const struct termios *__u_t) {
    long __u_r;
    if (__u_act < 0 || __u_act > 2) { errno = EINVAL; return -1; }
    __u_r = __ioctl(__u_fd, (long)_UNISA_TCSET + __u_act, (char *)__u_t);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; } return 0;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_cfmakeraw
static void cfmakeraw(struct termios *__u_t) {   /* what each host's libc does, bit for bit */
#ifdef __APPLE__
    __u_t->c_iflag &= ~(tcflag_t)(0x2000 | 0x400 | INPCK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON | 0x4);   /* IMAXBEL IXOFF IGNPAR */
    __u_t->c_iflag |= IGNBRK;
    __u_t->c_oflag &= ~(tcflag_t)OPOST;
    __u_t->c_lflag &= ~(tcflag_t)(ECHO | ECHOE | 0x4 | ECHONL | ICANON | ISIG | IEXTEN | 0x80000000UL | 0x400000 | 0x20000000);   /* ECHOK NOFLSH TOSTOP PENDIN */
    __u_t->c_cflag &= ~(tcflag_t)(CSIZE | PARENB);
    __u_t->c_cflag |= CS8 | 0x800;                                                                 /* CREAD */
#else
    __u_t->c_iflag &= ~(tcflag_t)(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    __u_t->c_oflag &= ~(tcflag_t)OPOST;
    __u_t->c_lflag &= ~(tcflag_t)(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    __u_t->c_cflag &= ~(tcflag_t)(CSIZE | PARENB);
    __u_t->c_cflag |= CS8;
#endif
    __u_t->c_cc[VMIN] = 1; __u_t->c_cc[VTIME] = 0;
}
#endif
#endif
#endif
