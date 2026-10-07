/* sys/mount.h -- 0.0.34 L2.  statfs/fstatfs forward to the system C library on macOS (SQLite's
 * osOpen reads f_fstypename and f_flags & MNT_LOCAL under __APPLE__).  arm64 has only the 64-bit
 * inode layout; x86-64's plain symbols (no $INODE64 suffix, as in glob.h) use the older layout,
 * so the struct follows the architecture.  Linux keeps statfs in <sys/vfs.h>; not offered here. */
#ifndef _UNISA_SYS_MOUNT_H
#define _UNISA_SYS_MOUNT_H
#include <sys/types.h>
#ifdef __APPLE__
typedef struct { int val[2]; } fsid_t;
#define MFSNAMELEN 15
#define MNAMELEN   90
#define MNT_RDONLY 0x00000001
#define MNT_LOCAL  0x00001000
#ifdef __aarch64__
#define MFSTYPENAMELEN 16
struct statfs {
    unsigned int f_bsize; int f_iosize;
    unsigned long f_blocks; unsigned long f_bfree; unsigned long f_bavail;
    unsigned long f_files; unsigned long f_ffree;
    fsid_t f_fsid; unsigned int f_owner;
    unsigned int f_type; unsigned int f_flags; unsigned int f_fssubtype;
    char f_fstypename[16]; char f_mntonname[1024]; char f_mntfromname[1024];
    unsigned int f_flags_ext; unsigned int f_reserved[7];
};
#else
struct statfs {
    short f_otype; short f_oflags; long f_bsize; long f_iosize;
    long f_blocks; long f_bfree; long f_bavail; long f_files; long f_ffree;
    fsid_t f_fsid; unsigned int f_owner; short f_reserved1; short f_type; long f_flags;
    long f_reserved2[2];
    char f_fstypename[MFSNAMELEN]; char f_mntonname[MNAMELEN]; char f_mntfromname[MNAMELEN];
    char f_reserved3; long f_reserved4[4];
};
#endif
int statfs(const char *__u_path, struct statfs *__u_b);
int fstatfs(int __u_fd, struct statfs *__u_b);
#endif
#endif
