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
#define SIGTERM 15
#define _UNISA_NSIG 32
static void (*_unisa_sig[_UNISA_NSIG])(int);

static void (*signal(int __u_sig, void (*__u_fn)(int)))(int) {
    void (*__u_old)(int);
    if (__u_sig <= 0 || __u_sig >= _UNISA_NSIG) return SIG_ERR;
    __u_old = _unisa_sig[__u_sig];
    _unisa_sig[__u_sig] = __u_fn;
    return __u_old;
}

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
