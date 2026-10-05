/* utime.h -- 0.0.28 H4.  utime forwards to the system C library on Linux and macOS (SQLite's shell). */
#ifndef _UNISA_UTIME_H
#define _UNISA_UTIME_H
#include <sys/types.h>
#ifndef _WIN32
struct utimbuf { long actime; long modtime; };
int utime(const char *__u_path, const struct utimbuf *__u_times);
#endif
#endif
