/* sys/_ret.h -- the one place a raw "-errno" result becomes -1 + errno.
 * Shared by every header whose bodies call the kernel (0.0.21: <time.h>
 * used it without including <unistd.h>, so clock_gettime alone failed). */
#ifndef _UNISA_RET_H
#define _UNISA_RET_H
#include <errno.h>
#if !__UNISA_FTRIM_LIBC || __UN__unisa_ret
static long _unisa_ret(long __u_r) { if (__u_r < 0 && __u_r > -4096) { errno = (int)(0 - __u_r); return -1; } return __u_r; }
#endif
#endif
