/* sys/mman.h -- 0.0.27 H1.  mmap/munmap/mprotect forward to the system C library on Linux and macOS
 * (the bundled allocator keeps its own __mmap gate).  Flag values follow each OS. */
#ifndef _UNISA_SYS_MMAN_H
#define _UNISA_SYS_MMAN_H
#include <sys/types.h>
#ifndef _WIN32
#define PROT_NONE  0x0
#define PROT_READ  0x1
#define PROT_WRITE 0x2
#define PROT_EXEC  0x4
#define MAP_SHARED  0x01
#define MAP_PRIVATE 0x02
#define MAP_FIXED   0x10
#ifdef __APPLE__
#define MAP_ANON 0x1000
#else
#define MAP_ANON 0x20
#endif
#define MAP_ANONYMOUS MAP_ANON
#define MAP_FAILED ((void *)(0 - 1))
void *mmap(void *__u_addr, size_t __u_len, int __u_prot, int __u_flags, int __u_fd, off_t __u_off);
int munmap(void *__u_addr, size_t __u_len);
int mprotect(void *__u_addr, size_t __u_len, int __u_prot);
#endif
#endif
