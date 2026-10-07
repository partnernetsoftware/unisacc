/* <sys/ioctl.h> (0.0.18 R18-10): ioctl() and the window size request a TUI
 * needs.  Windows: not here (its console API is a separate column). */
#ifndef _UNISA_SYS_IOCTL_H
#define _UNISA_SYS_IOCTL_H
#include <errno.h>
#ifndef _WIN32
struct winsize { unsigned short ws_row, ws_col, ws_xpixel, ws_ypixel; };
#ifdef __APPLE__
#define TIOCGWINSZ 0x40087468
#define FIONREAD   0x4004667F
#else
#define TIOCGWINSZ 0x5413
#define FIONREAD   0x541B
#endif
#if !__UNISA_FTRIM_LIBC || __UN_ioctl
static int ioctl(int __u_fd, unsigned long __u_req, void *__u_arg) {
    long __u_r; __u_r = __ioctl(__u_fd, (long)__u_req, (char *)__u_arg);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; } return (int)__u_r;
}
#endif
#endif
#ifdef __APPLE__
#define IOCPARM_MASK 0x1fff
#define IOC_VOID 0x20000000
#define IOC_OUT 0x40000000
#define IOC_IN 0x80000000
#define IOC_INOUT (IOC_IN|IOC_OUT)
#define _IOC(inout,group,num,len) ((unsigned long)((inout)|(((len)&IOCPARM_MASK)<<16)|((group)<<8)|(num)))
#define _IO(g,n) _IOC(IOC_VOID,(g),(n),0)
#define _IOR(g,n,t) _IOC(IOC_OUT,(g),(n),sizeof(t))
#define _IOW(g,n,t) _IOC(IOC_IN,(g),(n),sizeof(t))
#define _IOWR(g,n,t) _IOC(IOC_INOUT,(g),(n),sizeof(t))
#endif
#endif
