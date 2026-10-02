/* Four loader symbols are the only native bootstrap dependencies.
   macOS: the image imports them (LC_LOAD_DYLIB libSystem).  Linux (0.0.21
   R21-4a'): a cc-built compiler takes them from its own libc; a compiler
   built by unisacc for Linux has none (its image stays static), and the
   in-memory run refuses host forwarding there rather than call address 0. */
#if (defined(__APPLE__) || defined(__linux__)) && !defined(__UNISA__)
#include <dlfcn.h>
#endif
static long host_dl_slot(int i) {
#ifdef __APPLE__
#ifdef __UNISA__
    if (i == 0) return __hostaddr0();
    if (i == 1) return __hostaddr1();
    if (i == 2) return __hostaddr2();
    if (i == 3) return __hostaddr3();
#else
    if (i == 0) return (long)dlopen;
    if (i == 1) return (long)dlsym;
    if (i == 2) return (long)dlclose;
    if (i == 3) return (long)dlerror;
#endif
#endif
#if defined(__linux__) && !defined(__UNISA__)
    if (i == 0) return (long)dlopen;
    if (i == 1) return (long)dlsym;
    if (i == 2) return (long)dlclose;
    if (i == 3) return (long)dlerror;
#endif
    return 0;
}
