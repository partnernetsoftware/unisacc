/* sys/sysctl.h -- 0.0.34 L2.  sysctl/sysctlbyname forward to the system C library on macOS
 * (SQLite asks hw.ncpu).  Linux has no sysctl(2) worth offering; not declared there. */
#ifndef _UNISA_SYS_SYSCTL_H
#define _UNISA_SYS_SYSCTL_H
#include <sys/types.h>
#ifdef __APPLE__
#define CTL_KERN 1
#define CTL_HW   6
#define HW_NCPU  3
#define HW_MEMSIZE 24
int sysctl(int *__u_name, unsigned __u_namelen, void *__u_old, size_t *__u_oldlen, void *__u_new, size_t __u_newlen);
int sysctlbyname(const char *__u_name, void *__u_old, size_t *__u_oldlen, void *__u_new, size_t __u_newlen);
#endif
#endif
