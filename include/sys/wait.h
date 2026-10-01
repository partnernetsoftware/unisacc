/* sys/wait.h -- 0.0.19 R19-5: waitpid/wait over wait4 (generic gate), and the
 * status macros (the same encoding on Linux and macOS).  Not on Windows. */
#ifndef _UNISA_SYS_WAIT_H
#define _UNISA_SYS_WAIT_H
#include <sys/types.h>
#include <unistd.h>
#define WNOHANG 1
#define WUNTRACED 2
#define WIFEXITED(s) (((s) & 0x7f) == 0)
#define WEXITSTATUS(s) (((s) >> 8) & 0xff)
#define WIFSIGNALED(s) (((s) & 0x7f) != 0 && ((s) & 0x7f) != 0x7f)
#define WTERMSIG(s) ((s) & 0x7f)
#define WIFSTOPPED(s) (((s) & 0xff) == 0x7f)
#define WSTOPSIG(s) (((s) >> 8) & 0xff)
#ifndef _WIN32
#if !__UNISA_FTRIM_LIBC || __UN_waitpid || __UN_wait
static pid_t waitpid(pid_t __u_pid, int *__u_st, int __u_opt) {
    return (pid_t)_unisa_ret(__syscall6(_UNISA_SC(_UNISA_NR_wait4), (long)__u_pid, (long)__u_st, (long)__u_opt, 0, 0));
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_wait
static pid_t wait(int *__u_st) { return waitpid(-1, __u_st, 0); }
#endif
#endif
#endif
