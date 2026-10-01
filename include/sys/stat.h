/* <sys/stat.h> (0.0.18 R18-10): struct stat and stat/lstat/fstat/chmod/mkdir
 * over the kernel's own calls -- no host libc.  The layout is the kernel's for
 * each target (Linux x86_64 and arm64 differ; macOS is stat64).  Windows has
 * no such calls here yet: nothing is declared there (plans/v0.0.19.md). */
#ifndef _UNISA_SYS_STAT_H
#define _UNISA_SYS_STAT_H
#include <sys/types.h>
#include <errno.h>
#ifndef _WIN32
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
struct timespec { long tv_sec; long tv_nsec; };
#endif
#ifdef __APPLE__
struct stat {                                   /* struct stat64 */
    int st_dev; unsigned short st_mode; unsigned short st_nlink; unsigned long st_ino;
    unsigned int st_uid; unsigned int st_gid; int st_rdev;
    struct timespec st_atimespec, st_mtimespec, st_ctimespec, st_birthtimespec;
    long st_size; long st_blocks; int st_blksize; unsigned int st_flags; unsigned int st_gen;
    int st_lspare; long st_qspare[2];
};
#define st_atime st_atimespec.tv_sec
#define st_mtime st_mtimespec.tv_sec
#define st_ctime st_ctimespec.tv_sec
#else
#ifdef __aarch64__
struct stat {                                   /* asm-generic */
    unsigned long st_dev; unsigned long st_ino; unsigned int st_mode; unsigned int st_nlink;
    unsigned int st_uid; unsigned int st_gid; unsigned long st_rdev; unsigned long __u_pad1;
    long st_size; int st_blksize; int __u_pad2; long st_blocks;
    long st_atime; long st_atime_nsec; long st_mtime; long st_mtime_nsec; long st_ctime; long st_ctime_nsec;
    unsigned int __u_unused[2];
};
#else
struct stat {                                   /* x86_64 */
    unsigned long st_dev; unsigned long st_ino; unsigned long st_nlink;
    unsigned int st_mode; unsigned int st_uid; unsigned int st_gid; int __u_pad0;
    unsigned long st_rdev; long st_size; long st_blksize; long st_blocks;
    long st_atime; long st_atime_nsec; long st_mtime; long st_mtime_nsec; long st_ctime; long st_ctime_nsec;
    long __u_unused[3];
};
#endif
#endif
#if !__UNISA_FTRIM_LIBC || __UN_stat
static int stat(const char *__u_p, struct stat *__u_b) {
    long __u_r; __u_r = __stat((char *)__u_p, (char *)__u_b, 0);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; } return 0;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_lstat
static int lstat(const char *__u_p, struct stat *__u_b) {
    long __u_r; __u_r = __lstat((char *)__u_p, (char *)__u_b, 256);   /* AT_SYMLINK_NOFOLLOW for newfstatat */
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; } return 0;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_fstat
static int fstat(int __u_fd, struct stat *__u_b) {
    long __u_r; __u_r = __fstat(__u_fd, (char *)__u_b, 0);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; } return 0;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_chmod
static int chmod(const char *__u_p, mode_t __u_m) {
    long __u_r; __u_r = __chmod((char *)__u_p, (long)__u_m, 0);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; } return 0;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_mkdir
static int mkdir(const char *__u_p, mode_t __u_m) {
    long __u_r; __u_r = __mkdir((char *)__u_p, (long)__u_m, 0);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; } return 0;
}
#endif
#endif
#endif
