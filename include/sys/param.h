/* sys/param.h -- 0.0.34 L2.  The path and host-name limits and MIN/MAX, per OS (SQLite's
 * Apple locking style sizes buffers with MAXPATHLEN). */
#ifndef _UNISA_SYS_PARAM_H
#define _UNISA_SYS_PARAM_H
#include <sys/types.h>
#include <limits.h>
#ifdef _WIN32
#define MAXPATHLEN 260
#elif defined(__APPLE__)
#define MAXPATHLEN 1024
#else
#define MAXPATHLEN 4096
#endif
#ifdef __APPLE__
#define MAXHOSTNAMELEN 256
#else
#define MAXHOSTNAMELEN 64
#endif
#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif
#endif
