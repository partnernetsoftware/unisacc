/* Minimal directory API: Linux getdents64, macOS getdirentries64 (0.0.18); Windows rejects it.
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
#ifdef __APPLE__
#define _UNISA_DIRNAME 21
#else
#define _UNISA_DIRNAME 19
#endif
typedef struct {
    int fd, pos, len, failed;
    long basep;                                  /* macOS getdirentries64 position */
    long index;                                  /* entries returned so far (telldir) */
    struct dirent ent;
    unsigned char buf[_UNISA_DIRBUF];
} DIR;
#ifndef _WIN32   /* Windows has no directory calls yet: refused by name until the Windows POSIX layer (0.0.21 item 4a) */
#if !__UNISA_FTRIM_LIBC || __UN_opendir
static DIR *opendir(const char *__u_path) {
    DIR *__u_d; long __u_fd;
#ifdef __APPLE__
    __u_fd = __open((char *)__u_path, 0x100000, 0); /* macOS O_DIRECTORY */
#else
#ifdef __aarch64__
    __u_fd = __open((char *)__u_path, 16384, 0); /* Linux arm64 O_DIRECTORY */
#else
    __u_fd = __open((char *)__u_path, 65536, 0); /* Linux x86_64 O_DIRECTORY */
#endif
#endif
    if (__u_fd < 0) { errno = (int)(0 - __u_fd); return 0; }
    __u_d = (DIR *)malloc(sizeof(DIR));
    if (__u_d == 0) { __close(__u_fd); errno = ENOMEM; return 0; }
    __u_d->fd = (int)__u_fd; __u_d->pos = 0; __u_d->len = 0; __u_d->failed = 0; __u_d->basep = 0; __u_d->index = 0;
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
#ifdef __APPLE__
        __u_n = __getdirentries64(__u_d->fd, __u_d->buf, _UNISA_DIRBUF, (char *)&__u_d->basep, 0, 0);   /* R18-10 */
#else
        __u_n = __getdents64(__u_d->fd, __u_d->buf, _UNISA_DIRBUF);
#endif
        if (__u_n < 0) { __u_d->failed = (int)(0 - __u_n); errno = __u_d->failed; return 0; }
        if (__u_n == 0) return 0;
        if (__u_n > _UNISA_DIRBUF) { __u_d->failed = EIO; errno = EIO; return 0; }
        __u_d->len = (int)__u_n; __u_d->pos = 0;
    }
    __u_left = __u_d->len - __u_d->pos;
    if (__u_left < 24) { __u_d->failed = EIO; errno = EIO; return 0; }
    __u_r = __u_d->buf + __u_d->pos;
    __u_rl = __u_r[16] | (__u_r[17] << 8);
    /* linux_dirent64: d_type at 18, name at 19, records 8-aligned; macOS
       struct direntry: d_namlen at 18, d_type at 20, name at 21, 4-aligned */
    if (__u_rl < 24 || (__u_rl & (_UNISA_DIRNAME == 21 ? 3 : 7)) || __u_rl > __u_left) {
        __u_d->failed = EIO; errno = EIO; return 0;
    }
    for (__u_i = 0; __u_i < 256 && _UNISA_DIRNAME + __u_i < __u_rl && __u_r[_UNISA_DIRNAME + __u_i]; __u_i++) ;
    if (__u_i == 0 || __u_i >= 256 || _UNISA_DIRNAME + __u_i >= __u_rl) {
        __u_d->failed = EIO; errno = EIO; return 0;
    }
    __u_d->ent.d_name[__u_i] = 0;
    while (__u_i > 0) { __u_i--; __u_d->ent.d_name[__u_i] = (char)__u_r[_UNISA_DIRNAME + __u_i]; }
    __u_d->ent.d_ino = 0;
    for (__u_i = 7; __u_i >= 0; __u_i--)
        __u_d->ent.d_ino = (__u_d->ent.d_ino << 8) | __u_r[__u_i];
    __u_d->ent.d_type = __u_r[_UNISA_DIRNAME - 1]; __u_d->pos += __u_rl; __u_d->index = __u_d->index + 1;
    return &__u_d->ent;
}
#endif
/* 0.0.19 (dsh): rewinddir/telldir/seekdir.  A position is the number of
   entries already returned; seekdir rewinds and reads forward to it (POSIX
   only promises telldir's value is meaningful to seekdir on the same DIR). */
#if !__UNISA_FTRIM_LIBC || __UN_rewinddir
static void rewinddir(DIR *__u_d) {
    if (__u_d == 0) return;
    __lseek(__u_d->fd, 0, 0);
    __u_d->pos = 0; __u_d->len = 0; __u_d->failed = 0; __u_d->basep = 0; __u_d->index = 0;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_telldir
static long telldir(DIR *__u_d) { return __u_d ? __u_d->index : -1; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_seekdir
static void seekdir(DIR *__u_d, long __u_loc) {
    if (__u_d == 0) return;
    rewinddir(__u_d);
    while (__u_d->index < __u_loc) { if (readdir(__u_d) == 0) break; }
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
#endif
