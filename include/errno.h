/* <errno.h> for the unisa C subset.  `errno` is an ordinary int in the
 * image: the library functions here set it where C says they must (strtol
 * on overflow, the math functions on a domain error), and nothing else
 * does -- a system call that fails returns its error to the caller, and
 * the header that wraps it is what turns that into errno. */
#ifndef _UNISA_ERRNO_H
#define _UNISA_ERRNO_H
static int errno = 0;
#define EPERM   1
#define ENOENT  2
#define EIO     5
#define EBADF   9
#define ENOMEM  12
#define EACCES  13
#define EEXIST  17
#define EINVAL  22
#define ENOSPC  28
#define EDOM    33
#define ERANGE  34
#endif
