/* pwd.h -- 0.0.27 H1.  getpwuid/getpwnam forward to the system C library on Linux and macOS; the
 * struct passwd layout follows each OS (macOS carries pw_change, pw_class and pw_expire). */
#ifndef _UNISA_PWD_H
#define _UNISA_PWD_H
#include <sys/types.h>
#ifndef _WIN32
#ifdef __APPLE__
struct passwd { char *pw_name; char *pw_passwd; uid_t pw_uid; gid_t pw_gid; long pw_change; char *pw_class; char *pw_gecos; char *pw_dir; char *pw_shell; long pw_expire; };
#else
struct passwd { char *pw_name; char *pw_passwd; uid_t pw_uid; gid_t pw_gid; char *pw_gecos; char *pw_dir; char *pw_shell; };
#endif
struct passwd *getpwuid(uid_t __u_uid);
struct passwd *getpwnam(const char *__u_name);
#endif
#endif
