/* <sys/types.h> for the unisa C subset.
 *
 * TYPES ONLY.  POSIX puts its shorthand scalar types here and the functions
 * that use them in <unistd.h>, <fcntl.h>, <sys/stat.h> and friends; the
 * reference implements none of those functions yet, so none of those headers
 * exists -- a constant or a prototype with no implementation behind it would
 * be inventing an interface.  This header is the part that is real and
 * self-contained: the widths.
 *
 * All of them are 64-bit, like the rest of the subset (prd [G-2]: `int`
 * expressions evaluate in 64 bits).  `ssize_t` is the signed twin of
 * `size_t`; `off_t` is signed so a negative seek works; `pid_t` and `mode_t`
 * are 32-bit on every platform this compiler targets, and `dev_t`/`ino_t`
 * are 64-bit there.  `time_t` lives in <time.h>; `clock_t` in <time.h> too.
 *
 * There is no `struct stat`: it belongs to <sys/stat.h>, which needs `stat`
 * and `fstat` to be worth declaring. */
#ifndef _UNISA_SYS_TYPES_H
#define _UNISA_SYS_TYPES_H

#ifndef _UNISA_SIZE_T
#define _UNISA_SIZE_T
typedef unsigned long size_t;
#endif
typedef long          ssize_t;
typedef long          off_t;
typedef int           pid_t;
typedef unsigned int  uid_t;
typedef unsigned int  gid_t;
typedef unsigned int  mode_t;
typedef unsigned long dev_t;
typedef unsigned long ino_t;
typedef unsigned long nlink_t;
typedef long          blksize_t;
typedef long          blkcnt_t;
#ifndef _UNISA_TIME_T
#define _UNISA_TIME_T
typedef long          time_t;
#endif
typedef long          suseconds_t;
typedef unsigned long useconds_t;

#endif
