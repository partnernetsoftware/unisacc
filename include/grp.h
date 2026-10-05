/* grp.h -- 0.0.28 H2.  getgrgid/getgrnam forward to the system C library on Linux and macOS
 * (struct group has the same layout on both). */
#ifndef _UNISA_GRP_H
#define _UNISA_GRP_H
#include <sys/types.h>
#ifndef _WIN32
struct group { char *gr_name; char *gr_passwd; gid_t gr_gid; char **gr_mem; };
struct group *getgrgid(gid_t __u_gid);
struct group *getgrnam(const char *__u_name);
#endif
#endif
