/* sys/_win.h -- 0.0.21 Windows POSIX layer, batch 1: the shared helpers that
 * let <unistd.h>, <fcntl.h>, <sys/stat.h> and <time.h> wrap POSIX names thinly
 * over kernel32 (no home-grown libc).  The host channel is the program's IAT:
 * __hostaddr0..3 = LoadLibraryA GetProcAddress FreeLibrary GetLastError, and
 * __hostcall(fn, long[10]) makes the Win64/AAPCS64 call.  kernel32's handle is
 * looked up once; each caller caches its own procedure in a static.
 * On Windows an fd IS the HANDLE value (as the bundled stdio already has it). */
#ifndef _UNISA_SYS_WIN_H
#define _UNISA_SYS_WIN_H
#ifdef _WIN32
#include <errno.h>
#if !__UNISA_FTRIM_LIBC || __UN__ux_sym
static long _ux_sym(const char *__u_name) {          /* GetProcAddress(kernel32, name) */
    static long __u_k32;
    long __u_a[10] = {0};
    __u_a[1] = 0; __u_a[2] = 0; __u_a[3] = 0; __u_a[4] = 0; __u_a[5] = 0;
    if (!__u_k32) { __u_a[0] = (long)"kernel32.dll"; __u_k32 = __hostcall(__hostaddr0(), __u_a); }
    __u_a[0] = __u_k32; __u_a[1] = (long)__u_name;
    return __hostcall(__hostaddr1(), __u_a);
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__ux_call
static long _ux_call(long __u_fn, long __u_x0, long __u_x1, long __u_x2, long __u_x3) {
    long __u_a[10] = {0};
    __u_a[0] = __u_x0; __u_a[1] = __u_x1; __u_a[2] = __u_x2; __u_a[3] = __u_x3; __u_a[4] = 0; __u_a[5] = 0;
    return __hostcall(__u_fn, __u_a);
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__ux_errno
static int _ux_errno(void) {                         /* GetLastError -> errno */
    long __u_a[10] = {0}; long __u_e;
    __u_a[0] = 0; __u_a[1] = 0; __u_a[2] = 0; __u_a[3] = 0; __u_a[4] = 0; __u_a[5] = 0;
    __u_e = __hostcall(__hostaddr3(), __u_a) & 0xFFFFFFFFL;
    if (__u_e == 2 || __u_e == 3) return ENOENT;
    if (__u_e == 5) return EACCES;
    if (__u_e == 6) return EBADF;
    if (__u_e == 80 || __u_e == 183) return EEXIST;
    if (__u_e == 145) return ENOTEMPTY;
    if (__u_e == 32) return EBUSY;
    if (__u_e == 87) return EINVAL;
    return EIO;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__ux_fail
static int _ux_fail(void) { errno = _ux_errno(); return -1; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN__ux_ft2unix
static long _ux_ft2unix(const unsigned int *__u_ft) {   /* FILETIME (100 ns since 1601) -> unix seconds */
    long __u_t; __u_t = ((long)__u_ft[1] << 32) | (long)__u_ft[0];
    return (__u_t - 116444736000000000L) / 10000000L;
}
#endif
#endif
#endif
