/* <pthread.h> for the unisa C subset (0.0.31 H3): the minimal POSIX threads set, forwarded to the
 * host's library -- create/join/self/equal/detach, mutex, cond, once, key.  Every function is a
 * prototype without a body, so the driver forwards it (src/fwdstub.c); a start routine, a once
 * initialiser or a key destructor reaches the host as its C-ABI wrapper (front_parse.c cca_emit).
 * The object types are opaque, sized and aligned as the host library lays them out:
 * macOS (both architectures) and glibc on x86_64 / aarch64.  Not on Windows yet. */
#ifndef _UNISA_PTHREAD_H
#define _UNISA_PTHREAD_H
#include <stddef.h>
#include <time.h>
#ifdef _WIN32
#error "<pthread.h>: not on Windows yet (0.0.31 H3 covers macOS and Linux)"
#endif
/* its presence puts the back end in thread mode: per-thread system-call cells */
int __unisa_threads;
#ifdef __APPLE__
typedef struct _unisa_pthread *pthread_t;
typedef struct { long __sig; char __opaque[56]; } pthread_attr_t;
typedef struct { long __sig; char __opaque[56]; } pthread_mutex_t;
typedef struct { long __sig; char __opaque[8]; } pthread_mutexattr_t;
typedef struct { long __sig; char __opaque[40]; } pthread_cond_t;
typedef struct { long __sig; char __opaque[8]; } pthread_condattr_t;
typedef struct { long __sig; char __opaque[8]; } pthread_once_t;
typedef unsigned long pthread_key_t;
#define PTHREAD_MUTEX_INITIALIZER { 0x32AAABA7, { 0 } }
#define PTHREAD_COND_INITIALIZER  { 0x3CB0B1BB, { 0 } }
#define PTHREAD_ONCE_INIT         { 0x30B1BCBA, { 0 } }
#define PTHREAD_CREATE_JOINABLE 1
#define PTHREAD_CREATE_DETACHED 2
#define PTHREAD_MUTEX_NORMAL     0
#define PTHREAD_MUTEX_ERRORCHECK 1
#define PTHREAD_MUTEX_RECURSIVE  2
#else
typedef unsigned long pthread_t;
#ifdef __x86_64__
typedef union { char __size[56]; long __align; } pthread_attr_t;
typedef union { char __size[40]; long __align; } pthread_mutex_t;
#else
typedef union { char __size[64]; long __align; } pthread_attr_t;
typedef union { char __size[48]; long __align; } pthread_mutex_t;
#endif
typedef union { char __size[8]; int __align; } pthread_mutexattr_t;
typedef union { char __size[48]; long __align; } pthread_cond_t;
typedef union { char __size[8]; int __align; } pthread_condattr_t;
typedef int pthread_once_t;
typedef unsigned int pthread_key_t;
#define PTHREAD_MUTEX_INITIALIZER { { 0 } }
#define PTHREAD_COND_INITIALIZER  { { 0 } }
#define PTHREAD_ONCE_INIT 0
#define PTHREAD_CREATE_JOINABLE 0
#define PTHREAD_CREATE_DETACHED 1
#define PTHREAD_MUTEX_NORMAL     0
#define PTHREAD_MUTEX_RECURSIVE  1
#define PTHREAD_MUTEX_ERRORCHECK 2
#endif
int pthread_create(pthread_t *__u_t, const pthread_attr_t *__u_a, void *(*__u_start)(void *), void *__u_arg);
int pthread_join(pthread_t __u_t, void **__u_ret);
int pthread_detach(pthread_t __u_t);
pthread_t pthread_self(void);
int pthread_equal(pthread_t __u_a, pthread_t __u_b);
void pthread_exit(void *__u_ret);
int pthread_attr_init(pthread_attr_t *__u_a);
int pthread_attr_destroy(pthread_attr_t *__u_a);
int pthread_attr_setdetachstate(pthread_attr_t *__u_a, int __u_s);
int pthread_mutex_init(pthread_mutex_t *__u_m, const pthread_mutexattr_t *__u_a);
int pthread_mutex_destroy(pthread_mutex_t *__u_m);
int pthread_mutex_lock(pthread_mutex_t *__u_m);
int pthread_mutex_trylock(pthread_mutex_t *__u_m);
int pthread_mutex_unlock(pthread_mutex_t *__u_m);
int pthread_mutexattr_init(pthread_mutexattr_t *__u_a);
int pthread_mutexattr_destroy(pthread_mutexattr_t *__u_a);
int pthread_mutexattr_settype(pthread_mutexattr_t *__u_a, int __u_k);
int pthread_cond_init(pthread_cond_t *__u_c, const pthread_condattr_t *__u_a);
int pthread_cond_destroy(pthread_cond_t *__u_c);
int pthread_cond_wait(pthread_cond_t *__u_c, pthread_mutex_t *__u_m);
int pthread_cond_timedwait(pthread_cond_t *__u_c, pthread_mutex_t *__u_m, const struct timespec *__u_t);
int pthread_cond_signal(pthread_cond_t *__u_c);
int pthread_cond_broadcast(pthread_cond_t *__u_c);
int pthread_once(pthread_once_t *__u_o, void (*__u_init)(void));
int pthread_key_create(pthread_key_t *__u_k, void (*__u_dtor)(void *));
int pthread_key_delete(pthread_key_t __u_k);
void *pthread_getspecific(pthread_key_t __u_k);
int pthread_setspecific(pthread_key_t __u_k, const void *__u_v);
#endif
