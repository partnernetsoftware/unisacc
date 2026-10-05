/* dlfcn.h -- 0.0.27 H1.  On Linux and macOS these are prototypes with no body: both routes forward
 * them to the system C library (src/fwdstub.c), so a program loads and looks up real shared
 * libraries.  Loading a library with RTLD_GLOBAL also makes its functions reachable by the
 * forwarder by name (examples/https/post.c).  Windows has no dlfcn; LoadLibrary/GetProcAddress
 * are the native route there. */
#ifndef _UNISA_DLFCN_H
#define _UNISA_DLFCN_H
#ifndef _WIN32
#ifdef __APPLE__
#define RTLD_LAZY   0x1
#define RTLD_NOW    0x2
#define RTLD_LOCAL  0x4
#define RTLD_GLOBAL 0x8
#define RTLD_DEFAULT ((void *)(0 - 2))
#else
#define RTLD_LAZY   0x1
#define RTLD_NOW    0x2
#define RTLD_LOCAL  0x0
#define RTLD_GLOBAL 0x100
#define RTLD_DEFAULT ((void *)0)
#endif
void *dlopen(const char *__u_path, int __u_mode);
void *dlsym(void *__u_handle, const char *__u_name);
int dlclose(void *__u_handle);
char *dlerror(void);
#endif
#endif
