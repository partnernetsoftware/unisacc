/* <fcntl.h> (0.0.18 R18-10): the O_* and F_* constants of each target's
 * kernel, open() and fcntl() over its own calls.  Windows: not yet. */
#ifndef _UNISA_FCNTL_H
#define _UNISA_FCNTL_H
#include <sys/types.h>
#include <errno.h>
#ifndef _WIN32
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR   2
#define O_ACCMODE 3
#ifdef __APPLE__
#define O_NONBLOCK  0x4
#define O_APPEND    0x8
#define O_CREAT     0x200
#define O_TRUNC     0x400
#define O_EXCL      0x800
#define O_NOFOLLOW  0x100
#define O_DIRECTORY 0x100000
#define O_CLOEXEC   0x1000000
#else
#define O_CREAT     0100
#define O_EXCL      0200
#define O_NOCTTY    0400
#define O_TRUNC     01000
#define O_APPEND    02000
#define O_NONBLOCK  04000
#define O_CLOEXEC   02000000
#ifdef __aarch64__
#define O_DIRECTORY 040000
#define O_NOFOLLOW  0100000
#else
#define O_DIRECTORY 0200000
#define O_NOFOLLOW  0400000
#endif
#endif
#define F_DUPFD 0
#define F_GETFD 1
#define F_SETFD 2
#define F_GETFL 3
#define F_SETFL 4
#define FD_CLOEXEC 1
#if !__UNISA_FTRIM_LIBC || __UN_open
static int open(const char *__u_p, int __u_flags, int __u_mode) {
    long __u_r; __u_r = __open((char *)__u_p, __u_flags, __u_mode);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; } return (int)__u_r;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_fcntl
static int fcntl(int __u_fd, int __u_cmd, long __u_arg) {
    long __u_r; __u_r = __fcntl(__u_fd, __u_cmd, __u_arg);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; } return (int)__u_r;
}
#endif
#endif
#endif
