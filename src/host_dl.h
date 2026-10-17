#include "internal_types.h"
/* Four loader symbols are the only native bootstrap dependencies.
   macOS: the image imports them (LC_LOAD_DYLIB libSystem).  Linux (0.0.21
   R21-4a'): a cc-built compiler takes them from its own libc; a compiler
   built by unisacc for Linux has none, so that its own image stays static
   and starts without a dynamic loader (musl, scratch, glibc < 2.34).  Slot 0
   there means "no loader in this process": the product driver then builds
   a forwarding program as the dynamic -o image and execs it from an
   anonymous file (exec/c/compiler.c, forward_exec); the library and the
   reference compiler's in-memory run refuse rather than call address 0. */
#if (defined(__APPLE__) || defined(__linux__)) && !defined(__UNISA__)
#include <dlfcn.h>
#endif
#if defined(_WIN32) && !defined(__UNISA__)
#include <windows.h>
#endif
static us_addr host_dl_slot(int i) {
#ifdef __APPLE__
#ifdef __UNISA__
    if (i == 0) return __hostaddr0();
    if (i == 1) return __hostaddr1();
    if (i == 2) return __hostaddr2();
    if (i == 3) return __hostaddr3();
#else
    if (i == 0) return (us_addr)dlopen;
    if (i == 1) return (us_addr)dlsym;
    if (i == 2) return (us_addr)dlclose;
    if (i == 3) return (us_addr)dlerror;
#endif
#endif
#if defined(__linux__) && !defined(__UNISA__)
    if (i == 0) return (us_addr)dlopen;
    if (i == 1) return (us_addr)dlsym;
    if (i == 2) return (us_addr)dlclose;
    if (i == 3) return (us_addr)dlerror;
#endif
#if defined(_WIN32) && !defined(__UNISA__)
    if (i == 0) return (us_addr)LoadLibraryA;
    if (i == 1) return (us_addr)GetProcAddress;
    if (i == 2) return (us_addr)FreeLibrary;
    if (i == 3) return (us_addr)GetLastError;
#endif
    return 0;
}
