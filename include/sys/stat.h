/* <sys/stat.h> (0.0.18 R18-10): struct stat and stat/lstat/fstat/chmod/mkdir
 * over the kernel's own calls -- no host libc.  The layout is the kernel's for
 * each target (Linux x86_64 and arm64 differ; macOS is stat64).  Windows
 * (0.0.21 batch 1): a __u_word-field struct stat and stat/fstat/mkdir over
 * kernel32 (sys/_win.h); lstat is stat there (no symlinks followed or not),
 * chmod maps S_IWUSR to the read-only attribute (0.0.21 batch 2). */
#ifndef _UNISA_SYS_STAT_H
#define _UNISA_SYS_STAT_H
#include <sys/types.h>
#include <errno.h>
#include <sys/_word.h>
#include <sys/_win.h>
#define S_IFMT   0170000
#define S_IFIFO  0010000
#define S_IFCHR  0020000
#define S_IFDIR  0040000
#define S_IFBLK  0060000
#define S_IFREG  0100000
#define S_IFLNK  0120000
#define S_IFSOCK 0140000
#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)
#define S_ISLNK(m)  (((m) & S_IFMT) == S_IFLNK)
#define S_ISCHR(m)  (((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)  (((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)
#define S_IRWXU 0700
#define S_IRUSR 0400
#define S_IWUSR 0200
#define S_IXUSR 0100
#define S_IRWXG 070
#define S_IRGRP 040
#define S_IWGRP 020
#define S_IXGRP 010
#define S_IRWXO 07
#define S_IROTH 04
#define S_IWOTH 02
#define S_IXOTH 01
#ifndef _UNISA_TIMESPEC
#define _UNISA_TIMESPEC
struct timespec { __u_word tv_sec; long tv_nsec; };
#endif
#ifdef _WIN32
struct stat {
    __u_word st_dev; __u_word st_ino; __u_word st_mode; __u_word st_nlink; __u_word st_uid; __u_word st_gid;
    __u_word st_rdev; __u_word st_size; __u_word st_atime; __u_word st_mtime; __u_word st_ctime;
};
#elif defined(__APPLE__)
struct stat {                                   /* struct stat64 */
    int st_dev; unsigned short st_mode; unsigned short st_nlink; __u_uword st_ino;
    unsigned int st_uid; unsigned int st_gid; int st_rdev;
    struct timespec st_atimespec, st_mtimespec, st_ctimespec, st_birthtimespec;
    __u_word st_size; __u_word st_blocks; int st_blksize; unsigned int st_flags; unsigned int st_gen;
    int st_lspare; __u_word st_qspare[2];
};
#define st_atime st_atimespec.tv_sec
#define st_mtime st_mtimespec.tv_sec
#define st_ctime st_ctimespec.tv_sec
#else
#ifdef __aarch64__
struct stat {                                   /* asm-generic */
    __u_uword st_dev; __u_uword st_ino; unsigned int st_mode; unsigned int st_nlink;
    unsigned int st_uid; unsigned int st_gid; __u_uword st_rdev; __u_uword __u_pad1;
    __u_word st_size; int st_blksize; int __u_pad2; __u_word st_blocks;
    __u_word st_atime; __u_word st_atime_nsec; __u_word st_mtime; __u_word st_mtime_nsec; __u_word st_ctime; __u_word st_ctime_nsec;
    unsigned int __u_unused[2];
};
#else
struct stat {                                   /* x86_64 */
    __u_uword st_dev; __u_uword st_ino; __u_uword st_nlink;
    unsigned int st_mode; unsigned int st_uid; unsigned int st_gid; int __u_pad0;
    __u_uword st_rdev; __u_word st_size; __u_word st_blksize; __u_word st_blocks;
    __u_word st_atime; __u_word st_atime_nsec; __u_word st_mtime; __u_word st_mtime_nsec; __u_word st_ctime; __u_word st_ctime_nsec;
    __u_word __u_unused[3];
};
#endif
#endif
#if !__UNISA_FTRIM_LIBC || __UN_stat
static int stat(const char *__u_p, struct stat *__u_b) {
#ifdef _WIN32
    static __u_word __u_f; unsigned int __u_d[9];   /* GetFileAttributesExA: WIN32_FILE_ATTRIBUTE_DATA */
    if (!__u_f) __u_f = _ux_sym("GetFileAttributesExA");
    if (!(_ux_call(__u_f, (__u_word)__u_p, 0, (__u_word)__u_d, 0) & 0xFFFFFFFFL)) return _ux_fail();
    __u_b->st_dev = 0; __u_b->st_ino = 0; __u_b->st_nlink = 1; __u_b->st_uid = 0; __u_b->st_gid = 0; __u_b->st_rdev = 0;
    __u_b->st_mode = (__u_d[0] & 0x10) ? (S_IFDIR | 0755) : (S_IFREG | ((__u_d[0] & 1) ? 0444 : 0644));
    __u_b->st_ctime = _ux_ft2unix(__u_d + 1); __u_b->st_atime = _ux_ft2unix(__u_d + 3); __u_b->st_mtime = _ux_ft2unix(__u_d + 5);
    __u_b->st_size = ((__u_word)__u_d[7] << 32) | (__u_word)__u_d[8];
    return 0;
#else
    __u_word __u_r; __u_r = __stat((char *)__u_p, (char *)__u_b, 0);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; } return 0;
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_lstat
static int lstat(const char *__u_p, struct stat *__u_b) {
#ifdef _WIN32
    return stat(__u_p, __u_b);
#else
    __u_word __u_r; __u_r = __lstat((char *)__u_p, (char *)__u_b, 256);   /* AT_SYMLINK_NOFOLLOW for newfstatat */
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; } return 0;
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_fstat
static int fstat(int __u_fd, struct stat *__u_b) {
#ifdef _WIN32
    static __u_word __u_ft; static __u_word __u_fi; __u_word __u_k; unsigned int __u_d[13];
    if (!__u_ft) { __u_ft = _ux_sym("GetFileType"); __u_fi = _ux_sym("GetFileInformationByHandle"); }
    __u_b->st_dev = 0; __u_b->st_ino = 0; __u_b->st_nlink = 1; __u_b->st_uid = 0; __u_b->st_gid = 0; __u_b->st_rdev = 0;
    __u_b->st_size = 0; __u_b->st_atime = 0; __u_b->st_mtime = 0; __u_b->st_ctime = 0;
    __u_k = _ux_call(__u_ft, (__u_word)__u_fd, 0, 0, 0) & 0xFFFFFFFFL;
    if (__u_k == 2) { __u_b->st_mode = S_IFCHR | 0666; return 0; }            /* FILE_TYPE_CHAR */
    if (__u_k == 3) { __u_b->st_mode = S_IFIFO | 0666; return 0; }            /* FILE_TYPE_PIPE */
    if (!(_ux_call(__u_fi, (__u_word)__u_fd, (__u_word)__u_d, 0, 0) & 0xFFFFFFFFL)) return _ux_fail();
    __u_b->st_mode = (__u_d[0] & 0x10) ? (S_IFDIR | 0755) : (S_IFREG | ((__u_d[0] & 1) ? 0444 : 0644));
    __u_b->st_ctime = _ux_ft2unix(__u_d + 1); __u_b->st_atime = _ux_ft2unix(__u_d + 3); __u_b->st_mtime = _ux_ft2unix(__u_d + 5);
    __u_b->st_dev = __u_d[7]; __u_b->st_size = ((__u_word)__u_d[8] << 32) | (__u_word)__u_d[9];
    __u_b->st_nlink = __u_d[10]; __u_b->st_ino = ((__u_word)__u_d[11] << 32) | (__u_word)__u_d[12];
    return 0;
#else
    __u_word __u_r; __u_r = __fstat(__u_fd, (char *)__u_b, 0);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; } return 0;
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_chmod
static int chmod(const char *__u_p, mode_t __u_m) {
#ifdef _WIN32
    static __u_word __u_ga; static __u_word __u_sa; __u_word __u_at;
    if (!__u_ga) { __u_ga = _ux_sym("GetFileAttributesA"); __u_sa = _ux_sym("SetFileAttributesA"); }
    __u_at = _ux_call(__u_ga, (__u_word)__u_p, 0, 0, 0) & 0xFFFFFFFFL;
    if (__u_at == 0xFFFFFFFFL) return _ux_fail();
    __u_at = (__u_m & 0200) ? (__u_at & ~1L) : (__u_at | 1L);         /* FILE_ATTRIBUTE_READONLY */
    if (!(_ux_call(__u_sa, (__u_word)__u_p, __u_at, 0, 0) & 0xFFFFFFFFL)) return _ux_fail();
    return 0;
#else
    __u_word __u_r; __u_r = __chmod((char *)__u_p, (__u_word)__u_m, 0);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; } return 0;
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_mkdir
static int mkdir(const char *__u_p, mode_t __u_m) {
#ifdef _WIN32
    static __u_word __u_f;                         /* CreateDirectoryA (the mode is not mapped) */
    if (!__u_f) __u_f = _ux_sym("CreateDirectoryA");
    if (_ux_call(__u_f, (__u_word)__u_p, 0, 0, 0) & 0xFFFFFFFFL) return 0;
    return _ux_fail();
#else
    __u_word __u_r; __u_r = __mkdir((char *)__u_p, (__u_word)__u_m, 0);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; } return 0;
#endif
}
#endif
/* *at() flags (0.0.35 M1, minicon's loader): each kernel's own values */
#ifndef AT_FDCWD
#ifdef __APPLE__
#define AT_FDCWD (0 - 2)
#define AT_SYMLINK_NOFOLLOW 0x20
#define AT_REMOVEDIR 0x80
#else
#define AT_FDCWD (0 - 100)
#define AT_SYMLINK_NOFOLLOW 0x100
#define AT_REMOVEDIR 0x200
#endif
#endif
#if !__UNISA_FTRIM_LIBC || __UN_fstatat
static int fstatat(int __u_dfd, const char *__u_p, struct stat *__u_b, int __u_fl) {
#ifdef _WIN32
    if (__u_dfd != AT_FDCWD) { errno = ENOSYS; return -1; }
    return stat(__u_p, __u_b);
#else
    __u_word __u_r;
#if defined(__APPLE__)
#if defined(__x86_64__)
    __u_r = __syscall6(0x2000000L + 470, (__u_word)__u_dfd, (__u_word)__u_p, (__u_word)__u_b, (__u_word)__u_fl, 0);
#else
    __u_r = __syscall6(470, (__u_word)__u_dfd, (__u_word)__u_p, (__u_word)__u_b, (__u_word)__u_fl, 0);
#endif
#elif defined(__x86_64__)
    __u_r = __syscall6(262, (__u_word)__u_dfd, (__u_word)__u_p, (__u_word)__u_b, (__u_word)__u_fl, 0);
#else
    __u_r = __syscall6(79, (__u_word)__u_dfd, (__u_word)__u_p, (__u_word)__u_b, (__u_word)__u_fl, 0);
#endif
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; } return 0;
#endif
}
#endif
/* 0.0.28 H4: forwarded to the system C library on Linux and macOS */
#ifndef _WIN32
int fchmod(int __u_fd, mode_t __u_mode);
#endif
#endif
