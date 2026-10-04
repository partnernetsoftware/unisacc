/* <signal.h> for the unisa C subset -- the in-process half of it.
 * `signal` records a handler and `raise` calls it (and leaves it installed,
 * as BSD and glibc do); SIG_DFL for the signals
 * whose default is to terminate ends the process with the shell's status
 * for that signal (128 + n), which is what a program that raises SIGABRT
 * and is run from a shell observes.  No signal ever arrives from OUTSIDE:
 * there is no sigaction system call here, so a handler installed for
 * SIGINT is never called by Ctrl-C.  That is stated rather than faked. */
#ifndef _UNISA_SIGNAL_H
#define _UNISA_SIGNAL_H
typedef int sig_atomic_t;
#define SIG_DFL ((void (*)(int))0)
#define SIG_IGN ((void (*)(int))1)
#define SIG_ERR ((void (*)(int))(0-1))
#define SIGINT  2
#define SIGILL  4
#define SIGABRT 6
#define SIGFPE  8
#define SIGSEGV 11
#define SIGHUP  1
#define SIGQUIT 3
#define SIGKILL 9
#define SIGTERM 15
#define SIGWINCH 28                 /* same number on Linux and macOS; recorded, never delivered (above) */
#define _UNISA_NSIG 32
static void (*_unisa_sig[_UNISA_NSIG])(int);

#if !__UNISA_FTRIM_LIBC || __UN_signal
static void (*signal(int __u_sig, void (*__u_fn)(int)))(int) {
    void (*__u_old)(int);
    if (__u_sig <= 0 || __u_sig >= _UNISA_NSIG) return SIG_ERR;
    __u_old = _unisa_sig[__u_sig];
    _unisa_sig[__u_sig] = __u_fn;
    return __u_old;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_raise
static int raise(int __u_sig) {
    void (*__u_fn)(int);
    if (__u_sig <= 0 || __u_sig >= _UNISA_NSIG) return 0 - 1;
    __u_fn = _unisa_sig[__u_sig];
    if (__u_fn == SIG_IGN) return 0;
    if (__u_fn != SIG_DFL) {
        __u_fn(__u_sig);
        return 0;
    }
    __exit(128 + __u_sig);
    return 0;
}
#endif

/* kill (0.0.24, cdsh request): send a signal to another process.  POSIX targets
 * use the kernel call; signal 0 only checks that the process exists.  Windows
 * (pid = the posix_spawn HANDLE, as in waitpid): any nonzero signal ends the
 * process with exit code 128 + sig, which waitpid then reports; there is no
 * delivery to a handler there. */
#include <sys/types.h>
#include <sys/_ret.h>
#include <sys/_win.h>
#if !__UNISA_FTRIM_LIBC || __UN_kill
static int kill(pid_t __u_pid, int __u_sig) {
#ifdef _WIN32
    static long __u_t;
    if (__u_pid <= 0 || __u_sig < 0 || __u_sig >= _UNISA_NSIG) { errno = EINVAL; return -1; }
    if (__u_sig == 0) return 0;
    if (!__u_t) __u_t = _ux_sym("TerminateProcess");
    if (!(_ux_call(__u_t, (long)__u_pid, 128 + __u_sig, 0, 0) & 0xFFFFFFFFL)) return (int)_ux_fail();
    return 0;
#elif defined(__APPLE__)
#if defined(__x86_64__)
    return (int)_unisa_ret(__syscall6(0x2000000L + 37, (long)__u_pid, (long)__u_sig, 1, 0, 0));
#else
    return (int)_unisa_ret(__syscall6(37, (long)__u_pid, (long)__u_sig, 1, 0, 0));
#endif
#elif defined(__x86_64__)
    return (int)_unisa_ret(__syscall6(62, (long)__u_pid, (long)__u_sig, 0, 0, 0));
#else
    return (int)_unisa_ret(__syscall6(129, (long)__u_pid, (long)__u_sig, 0, 0, 0));
#endif
}
#endif
#endif
