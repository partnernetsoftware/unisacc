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

static void (*signal(int sig, void (*fn)(int)))(int) {
    void (*old)(int);
    if (sig <= 0 || sig >= _UNISA_NSIG) return SIG_ERR;
    old = _unisa_sig[sig];
    _unisa_sig[sig] = fn;
    return old;
}

static int raise(int sig) {
    void (*fn)(int);
    if (sig <= 0 || sig >= _UNISA_NSIG) return 0 - 1;
    fn = _unisa_sig[sig];
    if (fn == SIG_IGN) return 0;
    if (fn != SIG_DFL) {
        fn(sig);
        return 0;
    }
    __exit(128 + sig);
    return 0;
}
#endif
