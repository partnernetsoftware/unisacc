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
#include <sys/_word.h>
typedef int sig_atomic_t;
#define SIG_DFL ((void (*)(int))0)
#define SIG_IGN ((void (*)(int))1)
#define SIG_ERR ((void (*)(int))(0-1))
/* BEGIN generated signal targets */
/* Generated from exec/facts/signal-targets.tsv; do not edit.
 * Windows numbers are unisa compatibility values, not native UCRT signals. */
#define SIGHUP 1
#define SIGINT 2
#define SIGQUIT 3
#define SIGILL 4
#define SIGABRT 6
#define SIGFPE 8
#define SIGKILL 9
#if defined(__APPLE__)
#define SIGUSR1 30
#elif defined(_WIN32)
#define SIGUSR1 10
#else
#define SIGUSR1 10
#endif
#define SIGSEGV 11
#if defined(__APPLE__)
#define SIGUSR2 31
#elif defined(_WIN32)
#define SIGUSR2 12
#else
#define SIGUSR2 12
#endif
#define SIGPIPE 13
#define SIGTERM 15
#define SIGWINCH 28
/* END generated signal targets */
#define _UNISA_NSIG 32
typedef __u_uword sigset_t;
#define SA_RESTART   2
#define SA_RESETHAND 4
#define SA_NODEFER   16
#define SIG_BLOCK    1
#define SIG_UNBLOCK  2
#define SIG_SETMASK  3
/* Process-wide signal table and mask state: tentative definitions with external
 * linkage (C99 6.9.2), merged at link so multi-TU programs share one table and mask. */
#if defined(RT_EXTERN)
extern void (*_unisa_sig[_UNISA_NSIG])(int);
extern sigset_t _unisa_sigmask[_UNISA_NSIG];
extern int _unisa_sigflags[_UNISA_NSIG];
extern sigset_t _unisa_sigblocked;
extern sigset_t _unisa_sigpending;
#else
void (*_unisa_sig[_UNISA_NSIG])(int);
sigset_t _unisa_sigmask[_UNISA_NSIG];
int _unisa_sigflags[_UNISA_NSIG];
sigset_t _unisa_sigblocked;
sigset_t _unisa_sigpending;
#endif

#if !__UNISA_FTRIM_LIBC || __UN_signal
static void (*signal(int __u_sig, void (*__u_fn)(int)))(int) {
    void (*__u_old)(int);
    if (__u_sig <= 0 || __u_sig >= _UNISA_NSIG) return SIG_ERR;
    __u_old = _unisa_sig[__u_sig];
    _unisa_sig[__u_sig] = __u_fn;
    _unisa_sigmask[__u_sig] = 0;
    _unisa_sigflags[__u_sig] = 0;
    return __u_old;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_raise || __UN_sigprocmask
static void _unisa_sigdeliver(int __u_sig) {
    void (*__u_fn)(int);
    sigset_t __u_prev;
    sigset_t __u_ready;
    int __u_s;
    __u_fn = _unisa_sig[__u_sig];
    if (__u_fn == SIG_IGN) return;
    if (__u_fn == SIG_DFL) { __exit(128 + __u_sig); return; }
    __u_prev = _unisa_sigblocked;
    _unisa_sigblocked = _unisa_sigblocked | _unisa_sigmask[__u_sig];
    if (!(_unisa_sigflags[__u_sig] & SA_NODEFER)) {
        _unisa_sigblocked = _unisa_sigblocked | ((sigset_t)1 << __u_sig);
    }
    _unisa_sigblocked = _unisa_sigblocked & ~((sigset_t)1 << SIGKILL);
    if (_unisa_sigflags[__u_sig] & SA_RESETHAND) {
        _unisa_sig[__u_sig] = SIG_DFL;
    }
    __u_fn(__u_sig);
    _unisa_sigblocked = __u_prev;
    __u_ready = _unisa_sigpending & ~_unisa_sigblocked;
    for (__u_s = 1; __u_s < _UNISA_NSIG; __u_s = __u_s + 1) {
        sigset_t __u_bit = (sigset_t)1 << __u_s;
        if (__u_ready & __u_bit) {
            _unisa_sigpending = _unisa_sigpending & ~__u_bit;
            _unisa_sigdeliver(__u_s);
            __u_ready = _unisa_sigpending & ~_unisa_sigblocked;
        }
    }
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_raise
static int raise(int __u_sig) {
    if (__u_sig <= 0 || __u_sig >= _UNISA_NSIG) return 0 - 1;
    if (__u_sig != SIGKILL && (_unisa_sigblocked & ((sigset_t)1 << __u_sig))) {
        _unisa_sigpending = _unisa_sigpending | ((sigset_t)1 << __u_sig);
        return 0;
    }
    _unisa_sigdeliver(__u_sig);
    return 0;
}
#endif

/* sigaction and the signal sets (0.0.34 L1b, for lua built with LUA_USE_POSIX).
 * They share signal()'s process-wide in-process table, so the same rule holds:
 * a handler is recorded and raise() calls it; nothing arrives from outside.
 * sigprocmask() and sa_mask/SA_NODEFER maintain _unisa_sigblocked/_unisa_sigpending
 * across translation units so blocked signals raised via raise() are deferred
 * until unblocked. */
#include <errno.h>
struct sigaction {
    void (*sa_handler)(int);
    sigset_t sa_mask;
    int sa_flags;
};
#if !__UNISA_FTRIM_LIBC || __UN_sigemptyset
static int sigemptyset(sigset_t *__u_s) { *__u_s = 0; return 0; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_sigfillset
static int sigfillset(sigset_t *__u_s) { *__u_s = ~(sigset_t)0; return 0; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_sigaddset
static int sigaddset(sigset_t *__u_s, int __u_sig) {
    if (__u_sig <= 0 || __u_sig >= _UNISA_NSIG) { errno = EINVAL; return 0 - 1; }
    *__u_s = *__u_s | ((sigset_t)1 << __u_sig); return 0;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_sigdelset
static int sigdelset(sigset_t *__u_s, int __u_sig) {
    if (__u_sig <= 0 || __u_sig >= _UNISA_NSIG) { errno = EINVAL; return 0 - 1; }
    *__u_s = *__u_s & ~((sigset_t)1 << __u_sig); return 0;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_sigismember
static int sigismember(const sigset_t *__u_s, int __u_sig) {
    if (__u_sig <= 0 || __u_sig >= _UNISA_NSIG) { errno = EINVAL; return 0 - 1; }
    return (int)((*__u_s >> __u_sig) & 1);
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_sigaction
static int sigaction(int __u_sig, const struct sigaction *__u_act, struct sigaction *__u_old) {
    if (__u_sig <= 0 || __u_sig >= _UNISA_NSIG || ((__u_sig == SIGKILL) && __u_act)) { errno = EINVAL; return 0 - 1; }
    if (__u_old) {
        __u_old->sa_handler = _unisa_sig[__u_sig];
        __u_old->sa_mask = _unisa_sigmask[__u_sig];
        __u_old->sa_flags = _unisa_sigflags[__u_sig];
    }
    if (__u_act) {
        _unisa_sig[__u_sig] = __u_act->sa_handler;
        _unisa_sigmask[__u_sig] = __u_act->sa_mask;
        _unisa_sigflags[__u_sig] = __u_act->sa_flags;
    }
    return 0;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_sigprocmask
static int sigprocmask(int __u_how, const sigset_t *__u_set, sigset_t *__u_old) {
    sigset_t __u_ready;
    int __u_s;
    if (__u_set && (__u_how < SIG_BLOCK || __u_how > SIG_SETMASK)) { errno = EINVAL; return 0 - 1; }
    if (__u_old) *__u_old = _unisa_sigblocked;
    if (__u_set) {
        if (__u_how == SIG_BLOCK) _unisa_sigblocked = _unisa_sigblocked | *__u_set;
        else if (__u_how == SIG_UNBLOCK) _unisa_sigblocked = _unisa_sigblocked & ~(*__u_set);
        else _unisa_sigblocked = *__u_set;
        _unisa_sigblocked = _unisa_sigblocked & ~((sigset_t)1 << SIGKILL);
        __u_ready = _unisa_sigpending & ~_unisa_sigblocked;
        for (__u_s = 1; __u_s < _UNISA_NSIG; __u_s = __u_s + 1) {
            sigset_t __u_bit = (sigset_t)1 << __u_s;
            if (__u_ready & __u_bit) {
                _unisa_sigpending = _unisa_sigpending & ~__u_bit;
                _unisa_sigdeliver(__u_s);
                __u_ready = _unisa_sigpending & ~_unisa_sigblocked;
            }
        }
    }
    return 0;
}
#endif

/* kill (0.0.24, cdsh request): send a signal to another process.  POSIX targets
 * use the kernel call; signal 0 only checks that the process exists.
 * KNOWN-LIMIT: kill(getpid(), sig) invokes the kernel syscall directly and does
 * not consult the in-process _unisa_sig/_unisa_sigblocked table; use raise(sig)
 * for in-process signal delivery and masking.  Windows
 * (pid = the posix_spawn HANDLE, as in waitpid): any nonzero signal ends the
 * process with exit code 128 + sig, which waitpid then reports; there is no
 * delivery to a handler there. */
#include <sys/types.h>
#include <sys/_ret.h>
#include <sys/_win.h>
#if !__UNISA_FTRIM_LIBC || __UN_kill
static int kill(pid_t __u_pid, int __u_sig) {
#ifdef _WIN32
    static __u_word __u_t;
    if (__u_pid <= 0 || __u_sig < 0 || __u_sig >= _UNISA_NSIG) { errno = EINVAL; return -1; }
    if (__u_sig == 0) return 0;
    if (!__u_t) __u_t = _ux_sym("TerminateProcess");
    if (!(_ux_call(__u_t, (__u_word)__u_pid, 128 + __u_sig, 0, 0) & 0xFFFFFFFFL)) return (int)_ux_fail();
    return 0;
#elif defined(__APPLE__)
#if defined(__x86_64__)
    return (int)_unisa_ret(__syscall6(0x2000000L + 37, (__u_word)__u_pid, (__u_word)__u_sig, 1, 0, 0));
#else
    return (int)_unisa_ret(__syscall6(37, (__u_word)__u_pid, (__u_word)__u_sig, 1, 0, 0));
#endif
#elif defined(__x86_64__)
    return (int)_unisa_ret(__syscall6(62, (__u_word)__u_pid, (__u_word)__u_sig, 0, 0, 0));
#else
    return (int)_unisa_ret(__syscall6(129, (__u_word)__u_pid, (__u_word)__u_sig, 0, 0, 0));
#endif
}
#endif
#endif
