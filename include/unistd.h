/* Minimal POSIX <unistd.h> (0.0.17 R17-5, ordered by the realprog corpus):
 * the file-descriptor calls the bundled library already makes through its
 * own syscall intrinsics, under their POSIX names.  Return values follow
 * POSIX: -1 with errno set on failure.  isatty arrived in 0.0.18.  Not provided (yet): fork/exec,
 * getpid, sleep, pipes (POSIX L2). */
#ifndef _UNISA_UNISTD_H
#define _UNISA_UNISTD_H
#include <stddef.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/_ret.h>
#include <sys/_win.h>
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
#if !__UNISA_FTRIM_LIBC || __UN_isatty
static int isatty(int __u_fd) {               /* a terminal answers the attribute request (R18-10) */
#ifdef _WIN32
    static long __u_f;                        /* GetFileType == FILE_TYPE_CHAR */
    if (!__u_f) __u_f = _ux_sym("GetFileType");
    if ((_ux_call(__u_f, (long)__u_fd, 0, 0, 0) & 0xFFFFFFFFL) == 2) return 1;
    errno = ENOTTY; return 0;
#else
    unsigned char __u_t[128]; long __u_r;
#ifdef __APPLE__
    __u_r = __ioctl(__u_fd, 0x40487413L, (char *)__u_t);
#else
    __u_r = __ioctl(__u_fd, 0x5401L, (char *)__u_t);
#endif
    if (__u_r < 0) { errno = (int)(0 - __u_r); return 0; } return 1;
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_ftruncate
static int ftruncate(int __u_fd, off_t __u_len) {   /* R18-5 (kilo saves through it) */
#ifdef _WIN32
    static long __u_sp; static long __u_se; long __u_cur; int __u_ok;   /* the file position is kept, as POSIX has it */
    if (!__u_sp) { __u_sp = _ux_sym("SetFilePointerEx"); __u_se = _ux_sym("SetEndOfFile"); }
    if (__u_len < 0) { errno = EINVAL; return -1; }
    __u_cur = 0;
    if (!(_ux_call(__u_sp, (long)__u_fd, 0, (long)&__u_cur, 1) & 0xFFFFFFFFL)) return _ux_fail();
    if (!(_ux_call(__u_sp, (long)__u_fd, (long)__u_len, 0, 0) & 0xFFFFFFFFL)) return _ux_fail();
    __u_ok = (int)(_ux_call(__u_se, (long)__u_fd, 0, 0, 0) & 0xFFFFFFFFL);
    if (!__u_ok) __u_ok = 0 - _ux_errno();
    _ux_call(__u_sp, (long)__u_fd, __u_cur, 0, 0);
    if (__u_ok < 0) { errno = 0 - __u_ok; return -1; }
    return 0;
#else
    long __u_r; __u_r = __ftruncate(__u_fd, (long)__u_len, 0);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; }
    return 0;
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_unlink
static int unlink(const char *__u_path) {
#ifdef _WIN32
    static long __u_f;                        /* DeleteFileA: a BOOL */
    if (!__u_f) __u_f = _ux_sym("DeleteFileA");
    if (_ux_call(__u_f, (long)__u_path, 0, 0, 0) & 0xFFFFFFFFL) return 0;
    return _ux_fail();
#else
    long __u_r; __u_r = __unlink((char *)__u_path);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; }
    return 0;
#endif
}
#endif
/* ---- processes (0.0.19 R19-5), through the generic gate __syscall6 (R19-9):
   the call numbers live here, per OS/arch; nothing in the compiler's tables.
   macOS x86-64 numbers carry the BSD class 0x2000000.  macOS fork returns the
   same value in both processes (the child mark is in a second register), so
   the child is told apart by its pid; macOS has no pipe2, so pipe is a
   socketpair (AF_UNIX stream: bidirectional, stated rather than hidden). */
#ifndef _WIN32
#if defined(__APPLE__) && defined(__x86_64__)
#define _UNISA_SC(n) (0x2000000L + (n))
#else
#define _UNISA_SC(n) ((long)(n))
#endif
#ifdef __APPLE__
#define _UNISA_NR_getpid 20
#define _UNISA_NR_fork 2
#define _UNISA_NR_execve 59
#define _UNISA_NR_wait4 7
#define _UNISA_NR_dup2 90
#define _UNISA_NR_getppid 39
#elif defined(__x86_64__)
#define _UNISA_NR_getpid 39
#define _UNISA_NR_fork 57
#define _UNISA_NR_execve 59
#define _UNISA_NR_wait4 61
#define _UNISA_NR_dup2 33
#define _UNISA_NR_pipe2 293
#define _UNISA_NR_getcwd 79
#define _UNISA_NR_getppid 110
#else
#define _UNISA_NR_getpid 172
#define _UNISA_NR_clone 220
#define _UNISA_NR_execve 221
#define _UNISA_NR_wait4 260
#define _UNISA_NR_dup3 24
#define _UNISA_NR_pipe2 59
#define _UNISA_NR_getcwd 17
#define _UNISA_NR_getppid 173
#endif
/* _exit: no atexit handlers, no stdio flush */
#if !__UNISA_FTRIM_LIBC || __UN__exit
static void _exit(int __u_code) { __exit(__u_code); }
#endif
#include <sys/_ret.h>
#endif
#if !__UNISA_FTRIM_LIBC || __UN_getpid
static pid_t getpid(void) {
#ifdef _WIN32
    static long __u_f;
    if (!__u_f) __u_f = _ux_sym("GetCurrentProcessId");
    return (pid_t)(_ux_call(__u_f, 0, 0, 0, 0) & 0xFFFFFFFFL);
#else
    return (pid_t)__syscall6(_UNISA_SC(_UNISA_NR_getpid), 0, 0, 0, 0, 0);
#endif
}
#endif
#ifndef _WIN32
#if !__UNISA_FTRIM_LIBC || __UN_getppid
static pid_t getppid(void) { return (pid_t)__syscall6(_UNISA_SC(_UNISA_NR_getppid), 0, 0, 0, 0, 0); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_fork
static pid_t fork(void) {
    long __u_r;
#ifdef __APPLE__
    long __u_me; __u_me = getpid();
    __u_r = __syscall6(_UNISA_SC(_UNISA_NR_fork), 0, 0, 0, 0, 0);
    if (__u_r >= 0 && getpid() != __u_me) return 0;   /* the child */
#elif defined(__x86_64__)
    __u_r = __syscall6(_UNISA_NR_fork, 0, 0, 0, 0, 0);
#else
    __u_r = __syscall6(_UNISA_NR_clone, 17, 0, 0, 0, 0);    /* SIGCHLD, no new stack: fork */
#endif
    return (pid_t)_unisa_ret(__u_r);
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_execve
static int execve(const char *__u_path, char *const __u_argv[], char *const __u_envp[]) {
    return (int)_unisa_ret(__syscall6(_UNISA_SC(_UNISA_NR_execve), (long)__u_path, (long)__u_argv, (long)__u_envp, 0, 0));
}
#endif
/* 0.0.31 H1'': POSIX `environ` is <stdlib.h>'s _unisa_env, one object for the whole program
   (a tentative definition with external linkage in every unit, merged at link, C99 6.9.2), so an
   assignment in one unit is what getenv and execv see in every other. */
#include <stdlib.h>
#define environ _unisa_env
/* execv passes the current environ (POSIX; 0.0.28 E20) */
#if !__UNISA_FTRIM_LIBC || __UN_execv
static int execv(const char *__u_path, char *const __u_argv[]) { return execve(__u_path, __u_argv, environ); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_execvp
#include <stdlib.h>
#if !__UNISA_FTRIM_LIBC || __UN_execvp
static int execvp(const char *__u_file, char *const __u_argv[]) {
    char __u_buf[1024]; const char *__u_p; int __u_i; int __u_j; int __u_k;
    __u_i = 0; while (__u_file[__u_i]) { if (__u_file[__u_i] == 47) return execv(__u_file, __u_argv); __u_i = __u_i + 1; }
    __u_p = getenv("PATH"); if (__u_p == 0) __u_p = "/usr/bin:/bin";
    while (1) {
        __u_j = 0;
        while (__u_p[__u_j] && __u_p[__u_j] != 58 && __u_j < 900) { __u_buf[__u_j] = __u_p[__u_j]; __u_j = __u_j + 1; }
        if (__u_j == 0) { __u_buf[0] = 46; __u_j = 1; }
        __u_buf[__u_j] = 47; __u_k = 0;
        while (__u_file[__u_k] && __u_j + 1 + __u_k < 1023) { __u_buf[__u_j + 1 + __u_k] = __u_file[__u_k]; __u_k = __u_k + 1; }
        __u_buf[__u_j + 1 + __u_k] = 0;
        execv(__u_buf, __u_argv);
        while (*__u_p && *__u_p != 58) __u_p = __u_p + 1;
        if (*__u_p == 0) break;
        __u_p = __u_p + 1;
    }
    return -1;
}
#endif
#endif
/* execl family (0.0.20 R20-6, dsh): the variadic list gathered into an
   array, then execve/execvp; at most 255 arguments */
#include <stdarg.h>
#if !__UNISA_FTRIM_LIBC || __UN_execl
static int execl(const char *__u_path, const char *__u_a0, ...) {
    char *__u_v[256]; int __u_n; va_list __u_ap;
    __u_v[0] = (char *)__u_a0; __u_n = 1; va_start(__u_ap, __u_a0);
    while (__u_v[__u_n - 1] && __u_n < 256) { __u_v[__u_n] = va_arg(__u_ap, char *); __u_n = __u_n + 1; }
    va_end(__u_ap);
    if (__u_v[__u_n - 1]) return (int)_unisa_ret(0 - 7);    /* E2BIG */
    return execve(__u_path, __u_v, environ);
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_execlp
static int execlp(const char *__u_file, const char *__u_a0, ...) {
    char *__u_v[256]; int __u_n; va_list __u_ap;
    __u_v[0] = (char *)__u_a0; __u_n = 1; va_start(__u_ap, __u_a0);
    while (__u_v[__u_n - 1] && __u_n < 256) { __u_v[__u_n] = va_arg(__u_ap, char *); __u_n = __u_n + 1; }
    va_end(__u_ap);
    if (__u_v[__u_n - 1]) return (int)_unisa_ret(0 - 7);
    return execvp(__u_file, __u_v);
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_execle
static int execle(const char *__u_path, const char *__u_a0, ...) {
    char *__u_v[256]; char **__u_e; int __u_n; va_list __u_ap;
    __u_v[0] = (char *)__u_a0; __u_n = 1; va_start(__u_ap, __u_a0);
    while (__u_v[__u_n - 1] && __u_n < 256) { __u_v[__u_n] = va_arg(__u_ap, char *); __u_n = __u_n + 1; }
    if (__u_v[__u_n - 1]) { va_end(__u_ap); return (int)_unisa_ret(0 - 7); }
    __u_e = va_arg(__u_ap, char **); va_end(__u_ap);
    return execve(__u_path, __u_v, __u_e);
}
#endif
#endif
/* sleep / usleep over the shared nanosleep primitive (sys/_timespec.h; Sleep on Windows) */
#include <sys/_timespec.h>
#if !__UNISA_FTRIM_LIBC || __UN_sleep
static unsigned sleep(unsigned __u_s) { struct timespec __u_q; __u_q.tv_sec = __u_s; __u_q.tv_nsec = 0; _unisa_nanosleep(&__u_q); return 0; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_usleep
static int usleep(unsigned __u_us) { struct timespec __u_q; __u_q.tv_sec = __u_us / 1000000; __u_q.tv_nsec = (long)(__u_us % 1000000) * 1000; return (int)_unisa_ret(_unisa_nanosleep(&__u_q)); }
#endif
#ifndef _WIN32
#if !__UNISA_FTRIM_LIBC || __UN_dup2
static int dup2(int __u_old, int __u_new) {
#if defined(__APPLE__) || defined(__x86_64__)
    return (int)_unisa_ret(__syscall6(_UNISA_SC(_UNISA_NR_dup2), __u_old, __u_new, 0, 0, 0));
#else
    if (__u_old == __u_new) { long __u_r; __u_r = __fcntl(__u_old, 1, 0); return __u_r < 0 ? (int)_unisa_ret(__u_r) : __u_new; }   /* F_GETFD: is it open */
    return (int)_unisa_ret(__syscall6(_UNISA_NR_dup3, __u_old, __u_new, 0, 0, 0));
#endif
}
#endif
#endif
/* Windows: pipe is CreatePipe (two HANDLEs, not inheritable).  dup is
   DuplicateHandle (below); dup2 stays refused by name there: a HANDLE cannot be
   placed at a chosen number. */
#if !__UNISA_FTRIM_LIBC || __UN_pipe
static int pipe(int __u_fds[2]) {
#ifdef _WIN32
    static long __u_cp; long __u_h[2];
    if (!__u_cp) __u_cp = _ux_sym("CreatePipe");
    __u_h[0] = 0; __u_h[1] = 0;
    if (!(_ux_call(__u_cp, (long)&__u_h[0], (long)&__u_h[1], 0, 0) & 0xFFFFFFFFL)) return _ux_fail();
    __u_fds[0] = (int)__u_h[0]; __u_fds[1] = (int)__u_h[1];
    return 0;
#elif defined(__APPLE__)
    return (int)_unisa_ret(__syscall6(_UNISA_SC(135), 1, 1, 0, (long)__u_fds, 0));   /* socketpair(AF_UNIX, SOCK_STREAM) */
#else
    return (int)_unisa_ret(__syscall6(_UNISA_NR_pipe2, (long)__u_fds, 0, 0, 0, 0));
#endif
}
#endif
#define F_OK 0
#define X_OK 1
#define W_OK 2
#define R_OK 4
#ifndef _WIN32
#ifdef __APPLE__
#define _UNISA_NR_fsync 95
#define _UNISA_NR_dup 41
#define _UNISA_NR_rmdir 137
#define _UNISA_NR_access 33
#define _UNISA_NR_readlink 58
#define _UNISA_NR_symlink 57
#elif defined(__x86_64__)
#define _UNISA_NR_fsync 74
#define _UNISA_NR_dup 32
#define _UNISA_NR_rmdir 84
#define _UNISA_NR_access 21
#define _UNISA_NR_readlink 89
#define _UNISA_NR_symlink 88
#else
#define _UNISA_NR_fsync 82
#define _UNISA_NR_dup 23
#define _UNISA_NR_unlinkat 35
#define _UNISA_NR_faccessat 48
#define _UNISA_NR_readlinkat 78
#define _UNISA_NR_symlinkat 36
#endif
#endif
#if !__UNISA_FTRIM_LIBC || __UN_fsync
static int fsync(int __u_fd) {
#ifdef _WIN32
    static long __u_ff;
    if (!__u_ff) __u_ff = _ux_sym("FlushFileBuffers");
    if (!(_ux_call(__u_ff, (long)__u_fd, 0, 0, 0) & 0xFFFFFFFFL)) return _ux_fail();
    return 0;
#else
    return (int)_unisa_ret(__syscall6(_UNISA_SC(_UNISA_NR_fsync), __u_fd, 0, 0, 0, 0));
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_dup
static int dup(int __u_fd) {
#ifdef _WIN32
    /* 0.0.22 Windows POSIX batch 4: DuplicateHandle(self, h, self, &new, 0, FALSE,
       DUPLICATE_SAME_ACCESS) -- seven arguments, carried by the ten-slot host call. */
    static long __u_dh, __u_gp; long __u_a[10] = {0}; long __u_h, __u_self;
    if (!__u_dh) { __u_dh = _ux_sym("DuplicateHandle"); __u_gp = _ux_sym("GetCurrentProcess"); }
    __u_self = _ux_call(__u_gp, 0, 0, 0, 0); __u_h = 0;
    __u_a[0] = __u_self; __u_a[1] = (long)__u_fd; __u_a[2] = __u_self; __u_a[3] = (long)&__u_h;
    __u_a[4] = 0; __u_a[5] = 0; __u_a[6] = 2;
    if (!(__hostcall(__u_dh, __u_a) & 0xFFFFFFFFL)) return _ux_fail();
    return (int)__u_h;
#else
    return (int)_unisa_ret(__syscall6(_UNISA_SC(_UNISA_NR_dup), __u_fd, 0, 0, 0, 0));
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_rmdir
static int rmdir(const char *__u_p) {
#ifdef _WIN32
    static long __u_f;
    if (!__u_f) __u_f = _ux_sym("RemoveDirectoryA");
    if (_ux_call(__u_f, (long)__u_p, 0, 0, 0) & 0xFFFFFFFFL) return 0;
    return _ux_fail();
#elif defined(__APPLE__) || defined(__x86_64__)
    return (int)_unisa_ret(__syscall6(_UNISA_SC(_UNISA_NR_rmdir), (long)__u_p, 0, 0, 0, 0));
#else
    return (int)_unisa_ret(__syscall6(_UNISA_NR_unlinkat, -100, (long)__u_p, 0x200, 0, 0));   /* AT_FDCWD, AT_REMOVEDIR */
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_access
static int access(const char *__u_p, int __u_mode) {
#ifdef _WIN32
    static long __u_f; long __u_a;            /* GetFileAttributesA; X_OK is existence */
    if (!__u_f) __u_f = _ux_sym("GetFileAttributesA");
    __u_a = _ux_call(__u_f, (long)__u_p, 0, 0, 0) & 0xFFFFFFFFL;
    if (__u_a == 0xFFFFFFFFL) return _ux_fail();
    if ((__u_mode & W_OK) && (__u_a & 1) && !(__u_a & 0x10)) { errno = EACCES; return -1; }   /* READONLY file */
    return 0;
#elif defined(__APPLE__) || defined(__x86_64__)
    return (int)_unisa_ret(__syscall6(_UNISA_SC(_UNISA_NR_access), (long)__u_p, __u_mode, 0, 0, 0));
#else
    return (int)_unisa_ret(__syscall6(_UNISA_NR_faccessat, -100, (long)__u_p, __u_mode, 0, 0));
#endif
}
#endif
#ifndef _WIN32
#if !__UNISA_FTRIM_LIBC || __UN_readlink
static long readlink(const char *__u_p, char *__u_buf, size_t __u_n) {
#if defined(__APPLE__) || defined(__x86_64__)
    return _unisa_ret(__syscall6(_UNISA_SC(_UNISA_NR_readlink), (long)__u_p, (long)__u_buf, (long)__u_n, 0, 0));
#else
    return _unisa_ret(__syscall6(_UNISA_NR_readlinkat, -100, (long)__u_p, (long)__u_buf, (long)__u_n, 0));
#endif
}
#endif
/* 0.0.27 H1: the user and group ids forward to the system C library (pwd.h uses them) */
uid_t getuid(void);
uid_t geteuid(void);
gid_t getgid(void);
gid_t getegid(void);
#if !__UNISA_FTRIM_LIBC || __UN_ttyname
static char *ttyname(int __u_fd) {             /* 0.0.24 D2: the terminal's path, or 0 with errno */
    static char __u_tn[1024]; long __u_r;
    if (!isatty(__u_fd)) return 0;
#ifdef __APPLE__
    __u_r = __fcntl(__u_fd, 50, (long)__u_tn);  /* F_GETPATH */
    if (__u_r < 0) { errno = (int)(0 - __u_r); return 0; }
#else
    {   char __u_p[32]; int __u_k; int __u_d; char __u_dig[12];
        { const char *__u_s; __u_s = "/proc/self/fd/"; for (__u_k = 0; __u_k < 14; __u_k = __u_k + 1) __u_p[__u_k] = __u_s[__u_k]; } __u_k = 0; __u_d = __u_fd;
        do { __u_dig[__u_k] = (char)(48 + __u_d % 10); __u_d = __u_d / 10; __u_k = __u_k + 1; } while (__u_d);
        __u_d = 14; while (__u_k) { __u_k = __u_k - 1; __u_p[__u_d] = __u_dig[__u_k]; __u_d = __u_d + 1; } __u_p[__u_d] = 0;
        __u_r = readlink(__u_p, __u_tn, sizeof(__u_tn) - 1);
        if (__u_r < 0) return 0;
        __u_tn[__u_r] = 0; }
#endif
    return __u_tn;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_symlink
static int symlink(const char *__u_old, const char *__u_new) {
#if defined(__APPLE__) || defined(__x86_64__)
    return (int)_unisa_ret(__syscall6(_UNISA_SC(_UNISA_NR_symlink), (long)__u_old, (long)__u_new, 0, 0, 0));
#else
    return (int)_unisa_ret(__syscall6(_UNISA_NR_symlinkat, (long)__u_old, -100, (long)__u_new, 0, 0));
#endif
}
#endif
#endif
#if !__UNISA_FTRIM_LIBC || __UN_chdir
static int chdir(const char *__u_p) {
#ifdef _WIN32
    static long __u_f;
    if (!__u_f) __u_f = _ux_sym("SetCurrentDirectoryA");
    if (_ux_call(__u_f, (long)__u_p, 0, 0, 0) & 0xFFFFFFFFL) return 0;
    return _ux_fail();
#elif defined(__APPLE__)
    return (int)_unisa_ret(__syscall6(_UNISA_SC(12), (long)__u_p, 0, 0, 0, 0));
#elif defined(__x86_64__)
    return (int)_unisa_ret(__syscall6(80, (long)__u_p, 0, 0, 0, 0));
#else
    return (int)_unisa_ret(__syscall6(49, (long)__u_p, 0, 0, 0, 0));
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_getcwd
static char *getcwd(char *__u_buf, size_t __u_size) {
#ifdef _WIN32
    static long __u_f; long __u_n;            /* GetCurrentDirectoryA (backslashes kept) */
    if (!__u_f) __u_f = _ux_sym("GetCurrentDirectoryA");
    __u_n = _ux_call(__u_f, (long)__u_size, (long)__u_buf, 0, 0) & 0xFFFFFFFFL;
    if (__u_n == 0) { _ux_fail(); return 0; }
    if ((size_t)__u_n >= __u_size) { errno = ERANGE; return 0; }
    return __u_buf;
#elif defined(__APPLE__)
    char __u_tmp[1024]; long __u_fd; long __u_r; size_t __u_n;
    __u_fd = __open(".", 0x100000, 0);           /* O_RDONLY | O_DIRECTORY */
    if (__u_fd < 0) { errno = (int)(0 - __u_fd); return 0; }
    __u_r = __fcntl(__u_fd, 50, (long)__u_tmp);  /* F_GETPATH (needs MAXPATHLEN bytes) */
    __close(__u_fd);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return 0; }
    __u_n = 0; while (__u_tmp[__u_n]) __u_n = __u_n + 1;
    if (__u_n + 1 > __u_size) { errno = 34; return 0; }   /* ERANGE */
    __u_n = 0; while ((__u_buf[__u_n] = __u_tmp[__u_n]) != 0) __u_n = __u_n + 1;
    return __u_buf;
#else
    long __u_r; __u_r = __syscall6(_UNISA_NR_getcwd, (long)__u_buf, (long)__u_size, 0, 0, 0);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return 0; }
    return __u_buf;
#endif
}
#endif
/* 0.0.28 H4 (SQLite's unix VFS): forwarded to the system C library on Linux and macOS */
#ifndef _WIN32
ssize_t pread(int __u_fd, void *__u_buf, size_t __u_n, off_t __u_off);
ssize_t pwrite(int __u_fd, const void *__u_buf, size_t __u_n, off_t __u_off);
int fchown(int __u_fd, uid_t __u_uid, gid_t __u_gid);
/* 0.0.34 X1 (cx.h cx_run_timeout kills the whole group): forwarded */
int setpgid(pid_t __u_pid, pid_t __u_pgid);
pid_t getpgrp(void);
long sysconf(int __u_name);
#ifdef __APPLE__
#define _SC_PAGESIZE 29
#else
#define _SC_PAGESIZE 30
#endif
#define _SC_PAGE_SIZE _SC_PAGESIZE
ssize_t readlink(const char *__u_path, char *__u_buf, size_t __u_n);
#endif
/* 0.0.34 L2 (SQLite's Apple locking style): forwarded */
#ifdef __APPLE__
typedef unsigned char uuid_t[16];
int fsctl(const char *__u_path, unsigned long __u_req, void *__u_data, unsigned int __u_opt);
int gethostuuid(uuid_t __u_id, const struct timespec *__u_wait);
#endif
#endif
