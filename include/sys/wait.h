/* sys/wait.h -- 0.0.19 R19-5: waitpid/wait over wait4 (generic gate), and the
 * status macros (the same encoding on Linux and macOS).  Windows: waitpid on a posix_spawn HANDLE. */
#ifndef _UNISA_SYS_WAIT_H
#define _UNISA_SYS_WAIT_H
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#define WNOHANG 1
#define WUNTRACED 2
#define WIFEXITED(s) (((s) & 0x7f) == 0)
#define WEXITSTATUS(s) (((s) >> 8) & 0xff)
#define WIFSIGNALED(s) (((s) & 0x7f) != 0 && ((s) & 0x7f) != 0x7f)
#define WTERMSIG(s) ((s) & 0x7f)
#define WIFSTOPPED(s) (((s) & 0xff) == 0x7f)
#define WSTOPSIG(s) (((s) >> 8) & 0xff)
#if !defined(_WIN32) || !defined(_UNISA_NO_HOSTCALL)
#if !__UNISA_FTRIM_LIBC || __UN_waitpid
static pid_t waitpid(pid_t __u_pid, int *__u_st, int __u_opt) {
#ifdef _WIN32
    /* Windows (0.0.22): a pid is the process HANDLE from posix_spawn; the exit code is
       encoded as an exited status.  pid -1 has no meaning here and answers ECHILD. */
    static long __u_w, __u_x, __u_c; unsigned int __u_code;
    if (__u_pid <= 0) { errno = ECHILD; return -1; }
    if (!__u_w) { __u_w = _ux_sym("WaitForSingleObject"); __u_x = _ux_sym("GetExitCodeProcess"); __u_c = _ux_sym("CloseHandle"); }
    if ((_ux_call(__u_w, (long)__u_pid, (__u_opt & WNOHANG) ? 0 : 0xFFFFFFFFL, 0, 0) & 0xFFFFFFFFL) == 0x102) return 0;
    __u_code = 0;
    if (!(_ux_call(__u_x, (long)__u_pid, (long)&__u_code, 0, 0) & 0xFFFFFFFFL)) return _ux_fail();
    _ux_call(__u_c, (long)__u_pid, 0, 0, 0);
    if (__u_st) *__u_st = (int)((__u_code & 0xff) << 8);
    return __u_pid;
#else
    return (pid_t)_unisa_ret(__syscall6(_UNISA_SC(_UNISA_NR_wait4), (long)__u_pid, (long)__u_st, (long)__u_opt, 0, 0));
#endif
}
#endif
#endif
#ifndef _WIN32
#if !__UNISA_FTRIM_LIBC || __UN_wait
static pid_t wait(int *__u_st) { return waitpid(-1, __u_st, 0); }
#endif
#endif
#endif
