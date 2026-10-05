/* fnmatch.h -- 0.0.28 H2.  fnmatch forwards to the system C library on Linux and macOS; the flag
 * bits differ between glibc and the BSD libc. */
#ifndef _UNISA_FNMATCH_H
#define _UNISA_FNMATCH_H
#ifndef _WIN32
#define FNM_NOMATCH 1
#ifdef __APPLE__
#define FNM_NOESCAPE 0x01
#define FNM_PATHNAME 0x02
#define FNM_PERIOD   0x04
#else
#define FNM_PATHNAME 0x01
#define FNM_NOESCAPE 0x02
#define FNM_PERIOD   0x04
#endif
int fnmatch(const char *__u_pattern, const char *__u_string, int __u_flags);
#endif
#endif
