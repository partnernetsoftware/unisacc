/* cc/clang compatibility stand-in for unisacc's native FFI bridge (include/unisacc_ffi.h).
 * Only for the cc build of csih (check.sh step 4): dlopen/dlsym are real, uffi_call is
 * NOT implemented here (the bridge needs the unisacc host libffi), so it returns -1 and
 * the network path reports NET_NO_CURL under cc. Never shipped; the unisacc build keeps
 * using include/unisacc_ffi.h. */
#ifndef CSIH_CC_UNISACC_FFI_H
#define CSIH_CC_UNISACC_FFI_H

#include <dlfcn.h>
#include <stddef.h>

#define UFFI_VOID 0
#define UFFI_INT 1
#define UFFI_UINT 2
#define UFFI_LONG 3
#define UFFI_ULONG 4
#define UFFI_POINTER 5
#define UFFI_DOUBLE 6
#define UFFI_RTLD_DEFAULT (0 - 2)

static void *uffi_dlopen(const char *path, int mode) { return dlopen(path, mode); }
static void *uffi_dlsym(void *handle, const char *name) { return dlsym(handle, name); }
static int uffi_call(void *fn, int ret_kind, int *kinds, void **vals, int nargs, int nfixed, void *ret) {
    (void)fn; (void)ret_kind; (void)kinds; (void)vals; (void)nargs; (void)nfixed; (void)ret;
    return -1;
}

#endif
