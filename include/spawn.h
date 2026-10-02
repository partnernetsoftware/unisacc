/* <spawn.h>: posix_spawn/posix_spawnp over fork + file actions + execve/execvp.
 * The child applies the file actions (at most 16) in order, then execs; an
 * action or exec failure ends the child with _exit(127).  Returns 0 or an errno value (errno
 * itself is not set), the child's pid through the out parameter.  Attributes
 * are accepted and ignored (init/destroy only).  Windows: not yet. */
#ifndef _UNISA_SPAWN_H
#define _UNISA_SPAWN_H
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#ifndef _WIN32
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
#if !__UNISA_FTRIM_LIBC || __UN_posix_spawn
static int posix_spawn(pid_t *__u_pid, const char *__u_path, const posix_spawn_file_actions_t *__u_fa,
                       const posix_spawnattr_t *__u_at, char *const __u_argv[], char *const __u_envp[]) {
    (void)__u_at; return _unisa_spawn(__u_pid, __u_path, __u_fa, __u_argv, __u_envp, 0);
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_posix_spawnp
static int posix_spawnp(pid_t *__u_pid, const char *__u_file, const posix_spawn_file_actions_t *__u_fa,
                        const posix_spawnattr_t *__u_at, char *const __u_argv[], char *const __u_envp[]) {
    (void)__u_at; (void)__u_envp; return _unisa_spawn(__u_pid, __u_file, __u_fa, __u_argv, __u_envp, 1);
}
#endif
#endif
#endif
