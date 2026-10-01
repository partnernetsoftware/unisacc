/* Minimal POSIX <unistd.h> (0.0.17 R17-5, ordered by the realprog corpus):
 * the file-descriptor calls the bundled library already makes through its
 * own syscall intrinsics, under their POSIX names.  Return values follow
 * POSIX: -1 with errno set on failure.  Not provided (yet): fork/exec,
 * isatty, getpid, sleep, pipes (plans/v0.0.17.md R17-7). */
#ifndef _UNISA_UNISTD_H
#define _UNISA_UNISTD_H
#include <stddef.h>
#include <errno.h>
#include <sys/types.h>
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif
#if !__UNISA_FTRIM_LIBC || __UN_read
static ssize_t read(int __u_fd, void *__u_buf, size_t __u_n) {
    long __u_r; __u_r = __read(__u_fd, (char *)__u_buf, (long)__u_n);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; }
    return __u_r;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_write
static ssize_t write(int __u_fd, const void *__u_buf, size_t __u_n) {
    long __u_r; __u_r = __write(__u_fd, (char *)__u_buf, (long)__u_n);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; }
    return __u_r;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_close
static int close(int __u_fd) {
    long __u_r; __u_r = __close(__u_fd);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; }
    return 0;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_lseek
static off_t lseek(int __u_fd, off_t __u_off, int __u_whence) {
    long __u_r; __u_r = __lseek(__u_fd, (long)__u_off, __u_whence);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; }
    return __u_r;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_unlink
static int unlink(const char *__u_path) {
    long __u_r; __u_r = __unlink((char *)__u_path);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; }
    return 0;
}
#endif
#endif
