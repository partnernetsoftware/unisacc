/* glob.h -- 0.0.28 H2.  glob/globfree forward to the system C library on Linux and macOS.  glob_t
 * and the flag and error values follow each OS.  The directory callbacks (GLOB_ALTDIRFUNC) are not
 * offered; without them macOS x86-64's plain `glob` symbol returns the same paths as glob$INODE64. */
#ifndef _UNISA_GLOB_H
#define _UNISA_GLOB_H
#include <stddef.h>
#ifndef _WIN32
#ifdef __APPLE__
typedef struct { size_t gl_pathc; int gl_matchc; size_t gl_offs; int gl_flags; char **gl_pathv;
                 int (*gl_errfunc)(const char *, int); void (*gl_closedir)(void *); void *(*gl_readdir)(void *);
                 void *(*gl_opendir)(const char *); int (*gl_lstat)(const char *, void *); int (*gl_stat)(const char *, void *); } glob_t;
#define GLOB_APPEND   0x0001
#define GLOB_DOOFFS   0x0002
#define GLOB_ERR      0x0004
#define GLOB_MARK     0x0008
#define GLOB_NOCHECK  0x0010
#define GLOB_NOSORT   0x0020
#define GLOB_NOESCAPE 0x2000
#define GLOB_NOSPACE  (-1)
#define GLOB_ABORTED  (-2)
#define GLOB_NOMATCH  (-3)
#else
typedef struct { size_t gl_pathc; char **gl_pathv; size_t gl_offs; int gl_flags;
                 void (*gl_closedir)(void *); void *(*gl_readdir)(void *); void *(*gl_opendir)(const char *);
                 int (*gl_lstat)(const char *, void *); int (*gl_stat)(const char *, void *); } glob_t;
#define GLOB_ERR      (1 << 0)
#define GLOB_MARK     (1 << 1)
#define GLOB_NOSORT   (1 << 2)
#define GLOB_DOOFFS   (1 << 3)
#define GLOB_NOCHECK  (1 << 4)
#define GLOB_APPEND   (1 << 5)
#define GLOB_NOESCAPE (1 << 6)
#define GLOB_NOSPACE  1
#define GLOB_ABORTED  2
#define GLOB_NOMATCH  3
#endif
int glob(const char *__u_pattern, int __u_flags, int (*__u_errfunc)(const char *, int), glob_t *__u_g);
void globfree(glob_t *__u_g);
#endif
#endif
