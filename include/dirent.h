/* Minimal Linux directory API; unsupported targets reject __getdents64.
 * EOF sets errno=0; malformed kernel records set EIO and stop this stream. */
#ifndef _UNISA_DIRENT_H
#define _UNISA_DIRENT_H
#include <stdlib.h>
#include <errno.h>
struct dirent {
    unsigned long d_ino;
    unsigned char d_type;
    char d_name[256];
};
#define _UNISA_DIRBUF 4096
typedef struct {
    int fd, pos, len, failed;
    struct dirent ent;
    unsigned char buf[_UNISA_DIRBUF];
} DIR;
#if !__UNISA_FTRIM_LIBC || __UN_opendir
static DIR *opendir(const char *__u_path) {
    DIR *__u_d; long __u_fd;
#ifdef __aarch64__
    __u_fd = __open((char *)__u_path, 16384, 0); /* Linux arm64 O_DIRECTORY */
#else
    __u_fd = __open((char *)__u_path, 65536, 0); /* Linux x86_64 O_DIRECTORY */
#endif
    if (__u_fd < 0) { errno = (int)(0 - __u_fd); return 0; }
    __u_d = (DIR *)malloc(sizeof(DIR));
    if (__u_d == 0) { __close(__u_fd); errno = ENOMEM; return 0; }
    __u_d->fd = (int)__u_fd; __u_d->pos = 0; __u_d->len = 0; __u_d->failed = 0;
    errno = 0; return __u_d;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_readdir
static struct dirent *readdir(DIR *__u_d) {
    unsigned char *__u_r; int __u_rl, __u_i, __u_left; long __u_n;
    errno = 0;
    if (__u_d == 0) { errno = EBADF; return 0; }
    if (__u_d->failed) { errno = __u_d->failed; return 0; }
    if (__u_d->pos == __u_d->len) {
        __u_n = __getdents64(__u_d->fd, __u_d->buf, _UNISA_DIRBUF);
        if (__u_n < 0) { __u_d->failed = (int)(0 - __u_n); errno = __u_d->failed; return 0; }
        if (__u_n == 0) return 0;
        if (__u_n > _UNISA_DIRBUF) { __u_d->failed = EIO; errno = EIO; return 0; }
        __u_d->len = (int)__u_n; __u_d->pos = 0;
    }
    __u_left = __u_d->len - __u_d->pos;
    if (__u_left < 24) { __u_d->failed = EIO; errno = EIO; return 0; }
    __u_r = __u_d->buf + __u_d->pos;
    __u_rl = __u_r[16] | (__u_r[17] << 8);
    if (__u_rl < 24 || (__u_rl & 7) || __u_rl > __u_left) {
        __u_d->failed = EIO; errno = EIO; return 0;
    }
    for (__u_i = 0; __u_i < 256 && 19 + __u_i < __u_rl && __u_r[19 + __u_i]; __u_i++) ;
    if (__u_i == 0 || __u_i >= 256 || 19 + __u_i >= __u_rl) {
        __u_d->failed = EIO; errno = EIO; return 0;
    }
    __u_d->ent.d_name[__u_i] = 0;
    while (__u_i > 0) { __u_i--; __u_d->ent.d_name[__u_i] = (char)__u_r[19 + __u_i]; }
    __u_d->ent.d_ino = 0;
    for (__u_i = 7; __u_i >= 0; __u_i--)
        __u_d->ent.d_ino = (__u_d->ent.d_ino << 8) | __u_r[__u_i];
    __u_d->ent.d_type = __u_r[18]; __u_d->pos += __u_rl;
    return &__u_d->ent;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_closedir
static int closedir(DIR *__u_d) {
    long __u_r;
    if (__u_d == 0) { errno = EBADF; return -1; }
    __u_r = __close(__u_d->fd); free(__u_d);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; }
    return 0;
}
#endif
#endif
