/* Four loader symbols are the only native bootstrap dependencies. */
#if defined(__APPLE__) && !defined(__UNISA__)
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
    return 0;
}
