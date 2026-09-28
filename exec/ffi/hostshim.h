/* HOST TEST ONLY: exact native signatures replace the compiler intrinsics.
 * This verifies the runtime header's libffi use, not unisacc's ABI bridge. */
#include <dlfcn.h>
#include <ffi/ffi.h>
static void *__hostaddr(int i) {
    if (i == 0) return (void *)dlopen;
    if (i == 1) return (void *)dlsym;
    if (i == 2) return (void *)dlclose;
    if (i == 3) return (void *)dlerror;
    return 0;
}
static void *__hostaddr0(void) { return __hostaddr(0); }
static void *__hostaddr1(void) { return __hostaddr(1); }
static void *__hostaddr2(void) { return __hostaddr(2); }
static void *__hostaddr3(void) { return __hostaddr(3); }
static long __hostcall(void *f, long *a) {
    if (f == (void *)dlopen) return (long)dlopen((char *)a[0], (int)a[1]);
    if (f == (void *)dlsym) return (long)dlsym((void *)a[0], (char *)a[1]);
    if (f == (void *)dlclose) return dlclose((void *)a[0]);
    if (f == (void *)dlerror) return (long)dlerror();
    if (f == (void *)ffi_prep_cif)
        return ffi_prep_cif((ffi_cif *)a[0], (ffi_abi)a[1], (unsigned)a[2],
                           (ffi_type *)a[3], (ffi_type **)a[4]);
    if (f == (void *)ffi_prep_cif_var)
        return ffi_prep_cif_var((ffi_cif *)a[0], (ffi_abi)a[1], (unsigned)a[2],
                               (unsigned)a[3], (ffi_type *)a[4], (ffi_type **)a[5]);
    if (f == (void *)ffi_call) {
        ffi_call((ffi_cif *)a[0], (void (*)(void))a[1], (void *)a[2], (void **)a[3]);
        return 0;
    }
    return -777;
}
