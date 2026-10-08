/* Minimal directory API: Linux getdents64, macOS getdirentries64 (0.0.18); Windows
 * FindFirstFileA/FindNextFileA/FindClose (0.0.21 batch 2: "." and ".." are
 * returned there too, in the system's order -- portable code sorts or filters).
 * EOF sets errno=0; malformed kernel records set EIO and stop this stream. */
#ifndef _UNISA_DIRENT_H
#define _UNISA_DIRENT_H
#include <stdlib.h>
#include <errno.h>
#include <sys/_win.h>
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
/* Windows layout inside DIR: fd unused, basep = the find handle (0 = none yet,
   -1 = exhausted), buf[0..319] = WIN32_FIND_DATAA (cFileName at 44),
   buf[512..] = the "path\\*" pattern kept for rewinddir. */
#if !__UNISA_FTRIM_LIBC || __UN_opendir
static DIR *opendir(const char *__u_path) {
    DIR *__u_d; long __u_fd;
#ifdef _WIN32
    static long __u_ff; static long __u_fc; long __u_h; int __u_n;
    if (!__u_ff) { __u_ff = _ux_sym("FindFirstFileA"); __u_fc = _ux_sym("FindClose"); }
    for (__u_n = 0; __u_path[__u_n]; __u_n++) ;
    if (__u_n == 0) { errno = ENOENT; return 0; }
    if (__u_n > 3000) { errno = ENAMETOOLONG; return 0; }
    __u_d = (DIR *)malloc(sizeof(DIR));
    if (__u_d == 0) { errno = ENOMEM; return 0; }
    for (__u_fd = 0; __u_fd < __u_n; __u_fd++) __u_d->buf[512 + __u_fd] = (unsigned char)__u_path[__u_fd];
    if (__u_path[__u_n - 1] != '\\' && __u_path[__u_n - 1] != '/') __u_d->buf[512 + __u_n++] = '\\';
    __u_d->buf[512 + __u_n] = '*'; __u_d->buf[513 + __u_n] = 0;
    __u_h = _ux_call(__u_ff, (long)(__u_d->buf + 512), (long)__u_d->buf, 0, 0);
    if (__u_h == -1 || __u_h == 0) { __u_fd = _ux_errno(); free(__u_d); errno = (int)__u_fd; return 0; }
    if (!(__u_d->buf[0] & 0x10)) { _ux_call(__u_fc, __u_h, 0, 0, 0); free(__u_d); errno = ENOTDIR; return 0; }  /* "x\\*" of a file */
    __u_d->fd = -1; __u_d->pos = 1; __u_d->len = 0; __u_d->failed = 0; __u_d->basep = __u_h; __u_d->index = 0;
    errno = 0; return __u_d;
#else
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
#endif
}
#endif
/* fdopendir/dirfd (POSIX, 0.0.35 M1): the stream takes over FD, which must be an open directory */
#if !__UNISA_FTRIM_LIBC || __UN_fdopendir
static DIR *fdopendir(int __u_fd) {
    DIR *__u_d;
#ifdef _WIN32
    (void)__u_fd; errno = ENOSYS; return 0;
#else
    if (__u_fd < 0) { errno = EBADF; return 0; }
    __u_d = (DIR *)malloc(sizeof(DIR));
    if (__u_d == 0) { errno = ENOMEM; return 0; }
    __u_d->fd = __u_fd; __u_d->pos = 0; __u_d->len = 0; __u_d->failed = 0; __u_d->basep = 0; __u_d->index = 0;
    return __u_d;
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_dirfd
static int dirfd(DIR *__u_d) { if (!__u_d || __u_d->fd < 0) { errno = EINVAL; return -1; } return __u_d->fd; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_readdir
static struct dirent *readdir(DIR *__u_d) {
    unsigned char *__u_r; int __u_rl, __u_i, __u_left; long __u_n;
    errno = 0;
    if (__u_d == 0) { errno = EBADF; return 0; }
    if (__u_d->failed) { errno = __u_d->failed; return 0; }
#ifdef _WIN32
    {   static long __u_fn; static long __u_ff; long __u_e;
        if (!__u_fn) { __u_fn = _ux_sym("FindNextFileA"); __u_ff = _ux_sym("FindFirstFileA"); }
        if (__u_d->basep == -1) return 0;
        if (__u_d->basep == 0) {                     /* after rewinddir */
            __u_d->basep = _ux_call(__u_ff, (long)(__u_d->buf + 512), (long)__u_d->buf, 0, 0);
            if (__u_d->basep == -1 || __u_d->basep == 0) { __u_d->basep = -1; return 0; }
        } else if (!__u_d->pos) {
            if (!(_ux_call(__u_fn, __u_d->basep, (long)__u_d->buf, 0, 0) & 0xFFFFFFFFL)) {
                __u_e = _ux_errno(); errno = 0;
                if (__u_e != EIO) { __u_d->failed = (int)__u_e; errno = (int)__u_e; }   /* ERROR_NO_MORE_FILES maps to EIO: the end */
                return 0;
            }
        }
        __u_d->pos = 0;
        for (__u_i = 0; __u_i < 255 && __u_d->buf[44 + __u_i]; __u_i++) __u_d->ent.d_name[__u_i] = (char)__u_d->buf[44 + __u_i];
        __u_d->ent.d_name[__u_i] = 0;
        __u_d->ent.d_ino = 0; __u_d->ent.d_type = (__u_d->buf[0] & 0x10) ? 4 : 8;   /* DT_DIR / DT_REG */
        __u_d->index = __u_d->index + 1;
        return &__u_d->ent;
    }
#else
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
#endif
}
#endif
/* 0.0.19 (dsh): rewinddir/telldir/seekdir.  A position is the number of
   entries already returned; seekdir rewinds and reads forward to it (POSIX
   only promises telldir's value is meaningful to seekdir on the same DIR). */
#if !__UNISA_FTRIM_LIBC || __UN_rewinddir
static void rewinddir(DIR *__u_d) {
    if (__u_d == 0) return;
#ifdef _WIN32
    {   static long __u_fc;
        if (!__u_fc) __u_fc = _ux_sym("FindClose");
        if (__u_d->basep != -1 && __u_d->basep != 0) _ux_call(__u_fc, __u_d->basep, 0, 0, 0);
        __u_d->basep = 0; __u_d->pos = 1; __u_d->failed = 0; __u_d->index = 0;
        return;
    }
#else
    __lseek(__u_d->fd, 0, 0);
#endif
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
#ifdef _WIN32
    {   static long __u_fc;
        if (!__u_fc) __u_fc = _ux_sym("FindClose");
        __u_r = 0;
        if (__u_d->basep != -1 && __u_d->basep != 0) __u_r = _ux_call(__u_fc, __u_d->basep, 0, 0, 0) & 0xFFFFFFFFL;
        else __u_r = 1;
        free(__u_d);
        if (!__u_r) { errno = EBADF; return -1; }
        return 0;
    }
#else
    __u_r = __close(__u_d->fd); free(__u_d);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; }
    return 0;
#endif
}
#endif
#endif
