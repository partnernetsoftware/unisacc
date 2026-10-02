/* <spawn.h>: posix_spawn/posix_spawnp over fork + file actions + execve/execvp.
 * The child applies the file actions (at most 16) in order, then execs; an
 * action or exec failure ends the child with _exit(127).  Returns 0 or an errno value (errno
 * itself is not set), the child's pid through the out parameter.  Attributes
 * are accepted and ignored (init/destroy only).
 * Windows (0.0.22 POSIX batch 5): CreateProcessA with a quoted command line; the pid is the
 * process HANDLE (waitpid waits on it).  File actions may only target fds 0-2 (they become the
 * child's standard handles); addclose is accepted and has no effect (handles are not inherited
 * unless named); any other target answers ENOSYS. */
#ifndef _UNISA_SPAWN_H
#define _UNISA_SPAWN_H
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
typedef struct { int __u_flags; } posix_spawnattr_t;
#define _UNISA_SPAWN_MAX 16
struct _unisa_spawn_act { int __u_op; int __u_fd; int __u_nfd; int __u_oflag; mode_t __u_mode; char __u_path[256]; };
typedef struct { int __u_n; struct _unisa_spawn_act __u_a[_UNISA_SPAWN_MAX]; } posix_spawn_file_actions_t;
#if !__UNISA_FTRIM_LIBC || __UN_posix_spawnattr_init
static int posix_spawnattr_init(posix_spawnattr_t *__u_at) { __u_at->__u_flags = 0; return 0; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_posix_spawnattr_destroy
static int posix_spawnattr_destroy(posix_spawnattr_t *__u_at) { (void)__u_at; return 0; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_posix_spawn_file_actions_init
static int posix_spawn_file_actions_init(posix_spawn_file_actions_t *__u_fa) { __u_fa->__u_n = 0; return 0; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_posix_spawn_file_actions_destroy
static int posix_spawn_file_actions_destroy(posix_spawn_file_actions_t *__u_fa) { __u_fa->__u_n = 0; return 0; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_posix_spawn_file_actions_adddup2
static int posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t *__u_fa, int __u_fd, int __u_nfd) {
    if (__u_fd < 0 || __u_nfd < 0) return EBADF;
    if (__u_fa->__u_n >= _UNISA_SPAWN_MAX) return ENOMEM;
    __u_fa->__u_a[__u_fa->__u_n].__u_op = 1; __u_fa->__u_a[__u_fa->__u_n].__u_fd = __u_fd; __u_fa->__u_a[__u_fa->__u_n].__u_nfd = __u_nfd;
    __u_fa->__u_n = __u_fa->__u_n + 1; return 0;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_posix_spawn_file_actions_addclose
static int posix_spawn_file_actions_addclose(posix_spawn_file_actions_t *__u_fa, int __u_fd) {
    if (__u_fd < 0) return EBADF;
    if (__u_fa->__u_n >= _UNISA_SPAWN_MAX) return ENOMEM;
    __u_fa->__u_a[__u_fa->__u_n].__u_op = 2; __u_fa->__u_a[__u_fa->__u_n].__u_fd = __u_fd;
    __u_fa->__u_n = __u_fa->__u_n + 1; return 0;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_posix_spawn_file_actions_addopen
static int posix_spawn_file_actions_addopen(posix_spawn_file_actions_t *__u_fa, int __u_fd, const char *__u_p, int __u_of, mode_t __u_m) {
    int __u_i; struct _unisa_spawn_act *__u_x;
    if (__u_fd < 0) return EBADF;
    if (__u_fa->__u_n >= _UNISA_SPAWN_MAX) return ENOMEM;
    __u_x = &__u_fa->__u_a[__u_fa->__u_n];
    __u_i = 0; while (__u_p[__u_i]) { if (__u_i >= 255) return ENAMETOOLONG; __u_x->__u_path[__u_i] = __u_p[__u_i]; __u_i = __u_i + 1; }
    __u_x->__u_path[__u_i] = 0;
    __u_x->__u_op = 3; __u_x->__u_fd = __u_fd; __u_x->__u_oflag = __u_of; __u_x->__u_mode = __u_m;
    __u_fa->__u_n = __u_fa->__u_n + 1; return 0;
}
#endif
#ifndef _WIN32
#if !__UNISA_FTRIM_LIBC || __UN__unisa_spawn
static int _unisa_spawn(pid_t *__u_pid, const char *__u_path, const posix_spawn_file_actions_t *__u_fa,
                        char *const __u_argv[], char *const __u_envp[], int __u_usepath) {
    pid_t __u_c; int __u_i; int __u_r; const struct _unisa_spawn_act *__u_x;
    __u_c = fork();
    if (__u_c < 0) return errno;
    if (__u_c == 0) {
        if (__u_fa != 0) {
            __u_i = 0;
            while (__u_i < __u_fa->__u_n) {
                __u_x = &__u_fa->__u_a[__u_i];
                if (__u_x->__u_op == 1) { if (dup2(__u_x->__u_fd, __u_x->__u_nfd) < 0) _exit(127); }
                else if (__u_x->__u_op == 2) { close(__u_x->__u_fd); }
                else {
                    __u_r = open(__u_x->__u_path, __u_x->__u_oflag, __u_x->__u_mode);
                    if (__u_r < 0) _exit(127);
                    if (__u_r != __u_x->__u_fd) { if (dup2(__u_r, __u_x->__u_fd) < 0) _exit(127); close(__u_r); }
                }
                __u_i = __u_i + 1;
            }
        }
        if (__u_usepath) execvp(__u_path, __u_argv);
        else if (__u_envp != 0) execve(__u_path, __u_argv, __u_envp);
        else execv(__u_path, __u_argv);
        _exit(127);
    }
    if (__u_pid != 0) *__u_pid = __u_c;
    return 0;
}
#endif
#elif !defined(_UNISA_NO_HOSTCALL)
#if !__UNISA_FTRIM_LIBC || __UN__unisa_inherit
static long _unisa_inherit(long __u_h) {            /* an inheritable duplicate, or 0 */
    static long __u_dh, __u_gp; long __u_a[10] = {0}; long __u_n, __u_self;
    if (!__u_dh) { __u_dh = _ux_sym("DuplicateHandle"); __u_gp = _ux_sym("GetCurrentProcess"); }
    __u_self = _ux_call(__u_gp, 0, 0, 0, 0); __u_n = 0;
    __u_a[0] = __u_self; __u_a[1] = __u_h; __u_a[2] = __u_self; __u_a[3] = (long)&__u_n; __u_a[4] = 0; __u_a[5] = 1; __u_a[6] = 2;
    if (!(__hostcall(__u_dh, __u_a) & 0xFFFFFFFFL)) return 0;
    return __u_n;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__unisa_cmdarg
static int _unisa_cmdarg(char *__u_d, int __u_k, int __u_max, const char *__u_s) {   /* MS C runtime quoting */
    int __u_i, __u_b, __u_q;
    __u_q = __u_s[0] == 0;
    for (__u_i = 0; __u_s[__u_i]; __u_i = __u_i + 1) if (__u_s[__u_i] == 32 || __u_s[__u_i] == 9 || __u_s[__u_i] == 34) __u_q = 1;
    if (!__u_q) { for (__u_i = 0; __u_s[__u_i]; __u_i = __u_i + 1) { if (__u_k >= __u_max) return -1; __u_d[__u_k] = __u_s[__u_i]; __u_k = __u_k + 1; } return __u_k; }
    if (__u_k >= __u_max) return -1; __u_d[__u_k] = 34; __u_k = __u_k + 1;
    for (__u_i = 0; ; __u_i = __u_i + 1) {
        __u_b = 0; while (__u_s[__u_i] == 92) { __u_b = __u_b + 1; __u_i = __u_i + 1; }
        if (__u_s[__u_i] == 0) __u_b = __u_b * 2; else if (__u_s[__u_i] == 34) __u_b = __u_b * 2 + 1;
        while (__u_b > 0) { if (__u_k >= __u_max) return -1; __u_d[__u_k] = 92; __u_k = __u_k + 1; __u_b = __u_b - 1; }
        if (__u_s[__u_i] == 0) break;
        if (__u_k >= __u_max) return -1; __u_d[__u_k] = __u_s[__u_i]; __u_k = __u_k + 1;
    }
    if (__u_k >= __u_max) return -1; __u_d[__u_k] = 34; return __u_k + 1;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__unisa_wspawn
static int _unisa_wspawn(pid_t *__u_pid, const char *__u_path, const posix_spawn_file_actions_t *__u_fa,
                        char *const __u_argv[], char *const __u_envp[], int __u_usepath) {
    static long __u_cp, __u_gs, __u_ch; static char __u_cmd[8192]; static char __u_env[16384];
    long __u_si[13], __u_pi[3], __u_std[3], __u_a[10] = {0}; char *__u_sb; int __u_i, __u_k, __u_r, __u_n, __u_j;
    const struct _unisa_spawn_act *__u_x;
    if (!__u_cp) { __u_cp = _ux_sym("CreateProcessA"); __u_gs = _ux_sym("GetStdHandle"); __u_ch = _ux_sym("CloseHandle"); }
    for (__u_i = 0; __u_i < 3; __u_i = __u_i + 1) __u_std[__u_i] = _ux_call(__u_gs, -10 - __u_i, 0, 0, 0);
    if (__u_fa != 0) for (__u_i = 0; __u_i < __u_fa->__u_n; __u_i = __u_i + 1) {
        __u_x = &__u_fa->__u_a[__u_i];
        if (__u_x->__u_op == 2) continue;
        __u_n = __u_x->__u_op == 1 ? __u_x->__u_nfd : __u_x->__u_fd;
        if (__u_n < 0 || __u_n > 2) return ENOSYS;
        if (__u_x->__u_op == 1) __u_std[__u_n] = __u_x->__u_fd <= 2 ? __u_std[__u_x->__u_fd] : (long)__u_x->__u_fd;
        else { __u_r = open(__u_x->__u_path, __u_x->__u_oflag, __u_x->__u_mode); if (__u_r < 0) return errno; __u_std[__u_n] = (long)__u_r; }
    }
    for (__u_i = 0; __u_i < 13; __u_i = __u_i + 1) __u_si[__u_i] = 0;
    __u_sb = (char *)__u_si; *(int *)__u_sb = 104; *(int *)(__u_sb + 60) = 0x100;   /* cb, STARTF_USESTDHANDLES */
    for (__u_i = 0; __u_i < 3; __u_i = __u_i + 1) __u_si[10 + __u_i] = _unisa_inherit(__u_std[__u_i]);
    __u_k = 0;
    for (__u_i = 0; __u_argv[__u_i]; __u_i = __u_i + 1) {
        if (__u_i) { if (__u_k >= 8190) return E2BIG; __u_cmd[__u_k] = 32; __u_k = __u_k + 1; }
        __u_k = _unisa_cmdarg(__u_cmd, __u_k, 8190, __u_argv[__u_i]); if (__u_k < 0) return E2BIG;
    }
    __u_cmd[__u_k] = 0;
    if (__u_envp != 0) {
        __u_k = 0;
        for (__u_i = 0; __u_envp[__u_i]; __u_i = __u_i + 1) {
            for (__u_j = 0; __u_envp[__u_i][__u_j]; __u_j = __u_j + 1) { if (__u_k >= 16380) return E2BIG; __u_env[__u_k] = __u_envp[__u_i][__u_j]; __u_k = __u_k + 1; }
            __u_env[__u_k] = 0; __u_k = __u_k + 1;
        }
        __u_env[__u_k] = 0; __u_env[__u_k + 1] = 0;
    }
    __u_pi[0] = 0; __u_pi[1] = 0; __u_pi[2] = 0;
    __u_a[0] = __u_usepath ? 0 : (long)__u_path; __u_a[1] = (long)__u_cmd; __u_a[2] = 0; __u_a[3] = 0; __u_a[4] = 1;
    __u_a[5] = 0; __u_a[6] = __u_envp != 0 ? (long)__u_env : 0; __u_a[7] = 0; __u_a[8] = (long)__u_si; __u_a[9] = (long)__u_pi;
    __u_r = (int)(__hostcall(__u_cp, __u_a) & 0xFFFFFFFFL);
    __u_n = __u_r ? 0 : _ux_errno();
    for (__u_i = 0; __u_i < 3; __u_i = __u_i + 1) if (__u_si[10 + __u_i]) _ux_call(__u_ch, __u_si[10 + __u_i], 0, 0, 0);
    if (!__u_r) return __u_n;
    _ux_call(__u_ch, __u_pi[1], 0, 0, 0);
    if (__u_pid != 0) *__u_pid = (pid_t)__u_pi[0];
    return 0;
}
#endif
#endif
#if !defined(_WIN32) || !defined(_UNISA_NO_HOSTCALL)
#if !__UNISA_FTRIM_LIBC || __UN_posix_spawn
static int posix_spawn(pid_t *__u_pid, const char *__u_path, const posix_spawn_file_actions_t *__u_fa,
                       const posix_spawnattr_t *__u_at, char *const __u_argv[], char *const __u_envp[]) {
#ifdef _WIN32
    (void)__u_at; return _unisa_wspawn(__u_pid, __u_path, __u_fa, __u_argv, __u_envp, 0);
#else
    (void)__u_at; return _unisa_spawn(__u_pid, __u_path, __u_fa, __u_argv, __u_envp, 0);
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_posix_spawnp
static int posix_spawnp(pid_t *__u_pid, const char *__u_file, const posix_spawn_file_actions_t *__u_fa,
                        const posix_spawnattr_t *__u_at, char *const __u_argv[], char *const __u_envp[]) {
#ifdef _WIN32
    (void)__u_at; return _unisa_wspawn(__u_pid, __u_file, __u_fa, __u_argv, __u_envp, 1);
#else
    (void)__u_at; (void)__u_envp; return _unisa_spawn(__u_pid, __u_file, __u_fa, __u_argv, __u_envp, 1);
#endif
}
#endif
#endif
#endif
