/* sys/file.h -- 0.0.28 H4.  flock forwards to the system C library on Linux and macOS (SQLite's
 * Apple locking style); the LOCK_* values are the same on both. */
#ifndef _UNISA_SYS_FILE_H
#define _UNISA_SYS_FILE_H
#include <fcntl.h>
#ifndef _WIN32
#define LOCK_SH 1
#define LOCK_EX 2
#define LOCK_NB 4
#define LOCK_UN 8
int flock(int __u_fd, int __u_op);
#endif
#endif
