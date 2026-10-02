/* macOS system ABI calls through the unisacc native bootstrap bridge.
 * No FILE or va_list from the bundled library crosses this interface.
 * Argument values are addresses of objects in the declared native type;
 * variadic callers supply C default promotions (int and double).
 * ffi_type objects and call implementations come from the host libffi.
 * This header allocates no memory and owns no host pointers returned by calls.
 */
#ifndef _UNISACC_FFI_H
#define _UNISACC_FFI_H
#ifndef __APPLE__
#error unisacc_ffi currently requires macOS
#endif
#if defined(__aarch64__) || defined(__arm64__)
#define UFFI_ABI 1
#define UFFI_CIF_SIZE 40
#else
#ifdef __x86_64__
#define UFFI_ABI 2
#define UFFI_CIF_SIZE 32
#else
#error unisacc_ffi requires arm64 or x86_64
#endif
#endif
#define UFFI_VOID 0
#define UFFI_INT 1
#define UFFI_UINT 2
#define UFFI_LONG 3
#define UFFI_ULONG 4
#define UFFI_POINTER 5
#define UFFI_DOUBLE 6
#define UFFI_FLOAT 7
#define UFFI_SINT8 8
#define UFFI_UINT8 9
#define UFFI_SINT16 10
#define UFFI_UINT16 11
#define UFFI_MAX_ARGS 32
/* Layout from Apple's SDK ffi/ffi.h and ffitarget_{arm64,x86}.h.
 * Native unsigned fields have the same representation as these int fields.
 */
/* the loader's RTLD_DEFAULT: macOS (void *)-2, glibc (void *)0 */
#ifdef __APPLE__
#define UFFI_RTLD_DEFAULT (0 - 2)
#else
#define UFFI_RTLD_DEFAULT 0
#endif
struct uffi_cif {
    int __u_abi;
    int __u_nargs;
    void **__u_arg_types;
    void *__u_rtype;
    int __u_bytes;
    int __u_flags;
#if defined(__aarch64__) || defined(__arm64__)
    int __u_aarch64_nfixedargs;
#endif
};
static void *__uffi_handle;
static void *__uffi_prep;
static void *__uffi_prep_var;
static void *__uffi_call;
static void *__uffi_types[12];
static int __uffi_ready;

#if !__UNISA_FTRIM_LIBC || __UN_uffi_dlopen
static void *uffi_dlopen(const char *__u_path, int __u_mode) {
    long __u_args[6];
    __u_args[0] = (long)__u_path; __u_args[1] = __u_mode;
    __u_args[2] = 0; __u_args[3] = 0; __u_args[4] = 0; __u_args[5] = 0;
    return (void *)__hostcall(__hostaddr0(), __u_args);
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_uffi_dlsym
static void *uffi_dlsym(void *__u_handle, const char *__u_name) {
    long __u_args[6];
    __u_args[0] = (long)__u_handle; __u_args[1] = (long)__u_name;
    __u_args[2] = 0; __u_args[3] = 0; __u_args[4] = 0; __u_args[5] = 0;
    return (void *)__hostcall(__hostaddr1(), __u_args);
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_uffi_dlclose
static int uffi_dlclose(void *__u_handle) {
    long __u_args[6];
    __u_args[0] = (long)__u_handle; __u_args[1] = 0;
    __u_args[2] = 0; __u_args[3] = 0; __u_args[4] = 0; __u_args[5] = 0;
    return (int)__hostcall(__hostaddr2(), __u_args);
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_uffi_dlerror
static char *uffi_dlerror(void) {
    long __u_args[6];
    __u_args[0] = 0; __u_args[1] = 0; __u_args[2] = 0;
    __u_args[3] = 0; __u_args[4] = 0; __u_args[5] = 0;
    return (char *)__hostcall(__hostaddr3(), __u_args);
}
#endif
/* 0 succeeds; -1 unsupported layout; -2 dlopen/dlsym failure.
 * A failed initialization is retried; the private libffi handle stays live
 * because its ffi_type pointers and entry addresses refer into that image.
 */
#if !__UNISA_FTRIM_LIBC || __UN_uffi_init
static int uffi_init(void) {
    int __u_i;
    if (__uffi_ready) return 0;
    if (sizeof(long) != 8 || sizeof(void *) != 8 ||
        sizeof(struct uffi_cif) != UFFI_CIF_SIZE) return 0 - 1;
    if (__uffi_handle == 0) __uffi_handle = uffi_dlopen("/usr/lib/libffi.dylib", 2);
    if (__uffi_handle == 0) return 0 - 2;
    __uffi_prep = uffi_dlsym(__uffi_handle, "ffi_prep_cif");
    __uffi_prep_var = uffi_dlsym(__uffi_handle, "ffi_prep_cif_var");
    __uffi_call = uffi_dlsym(__uffi_handle, "ffi_call");
    __uffi_types[0] = uffi_dlsym(__uffi_handle, "ffi_type_void");
    __uffi_types[1] = uffi_dlsym(__uffi_handle, "ffi_type_sint32");
    __uffi_types[2] = uffi_dlsym(__uffi_handle, "ffi_type_uint32");
    __uffi_types[3] = uffi_dlsym(__uffi_handle, "ffi_type_sint64");
    __uffi_types[4] = uffi_dlsym(__uffi_handle, "ffi_type_uint64");
    __uffi_types[5] = uffi_dlsym(__uffi_handle, "ffi_type_pointer");
    __uffi_types[6] = uffi_dlsym(__uffi_handle, "ffi_type_double");
    __uffi_types[7] = uffi_dlsym(__uffi_handle, "ffi_type_float");
    __uffi_types[8] = uffi_dlsym(__uffi_handle, "ffi_type_sint8");
    __uffi_types[9] = uffi_dlsym(__uffi_handle, "ffi_type_uint8");
    __uffi_types[10] = uffi_dlsym(__uffi_handle, "ffi_type_sint16");
    __uffi_types[11] = uffi_dlsym(__uffi_handle, "ffi_type_uint16");
    if (__uffi_prep == 0 || __uffi_prep_var == 0 || __uffi_call == 0) return 0 - 2;
    __u_i = 0;
    while (__u_i < 12) {
        if (__uffi_types[__u_i] == 0) return 0 - 2;
        __u_i = __u_i + 1;
    }
    __uffi_ready = 1;
    return 0;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_uffi_type
static void *uffi_type(int __u_kind) {
    if (__u_kind < 0 || __u_kind >= 12 || uffi_init() != 0) return 0;
    return __uffi_types[__u_kind];
}
#endif
/* Low-level typed call; types may also be native ffi_type objects provided
 * by a caller.  Their lifetime and argument storage must cover this call.
 * fixed == -1 selects a fixed signature; otherwise fixed is the number of
 * named arguments.  rvalue storage must be aligned and at least 8 bytes for
 * small integer returns; pass 0 only for a void return type.
 * 0 succeeds, -3 rejects an invalid vector, and positive values are the
 * actual ffi_status from ffi_prep_cif[_var].  No call follows a prep error.
 */
#if !__UNISA_FTRIM_LIBC || __UN_uffi_call_types
static int uffi_call_types(void *__u_fn, void *__u_rtype, void **__u_types,
                           void **__u_values, int __u_n, int __u_fixed,
                           void *__u_result) {
    struct uffi_cif __u_cif;
    long __u_args[6];
    int __u_i; int __u_status;
    __u_status = uffi_init();
    if (__u_status != 0) return __u_status;
    if (__u_fn == 0 || __u_rtype == 0 || __u_n < 0 || __u_n > UFFI_MAX_ARGS ||
        __u_fixed < 0 - 1 || __u_fixed > __u_n) return 0 - 3;
    if (__u_n != 0 && (__u_types == 0 || __u_values == 0)) return 0 - 3;
    if (__u_rtype != __uffi_types[UFFI_VOID] && __u_result == 0) return 0 - 3;
    __u_i = 0;
    while (__u_i < __u_n) {
        if (__u_types[__u_i] == 0 || __u_types[__u_i] == __uffi_types[UFFI_VOID] ||
            __u_values[__u_i] == 0) return 0 - 3;
        if (__u_fixed >= 0 && __u_i >= __u_fixed &&
            (__u_types[__u_i] == __uffi_types[UFFI_FLOAT] ||
             __u_types[__u_i] == __uffi_types[UFFI_SINT8] ||
             __u_types[__u_i] == __uffi_types[UFFI_UINT8] ||
             __u_types[__u_i] == __uffi_types[UFFI_SINT16] ||
             __u_types[__u_i] == __uffi_types[UFFI_UINT16])) return 0 - 3;
        __u_i = __u_i + 1;
    }
    __u_args[0] = (long)&__u_cif; __u_args[1] = UFFI_ABI;
    if (__u_fixed < 0) {
        __u_args[2] = __u_n; __u_args[3] = (long)__u_rtype;
        __u_args[4] = (long)__u_types; __u_args[5] = 0;
        __u_status = (int)__hostcall(__uffi_prep, __u_args);
    } else {
        __u_args[2] = __u_fixed; __u_args[3] = __u_n;
        __u_args[4] = (long)__u_rtype; __u_args[5] = (long)__u_types;
        __u_status = (int)__hostcall(__uffi_prep_var, __u_args);
    }
    if (__u_status != 0) return __u_status;
    __u_args[0] = (long)&__u_cif; __u_args[1] = (long)__u_fn;
    __u_args[2] = (long)__u_result; __u_args[3] = (long)__u_values;
    __u_args[4] = 0; __u_args[5] = 0;
    __hostcall(__uffi_call, __u_args);
    return 0;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_uffi_call
static int uffi_call(void *__u_fn, int __u_return_kind, int *__u_kinds,
                     void **__u_values, int __u_n, int __u_fixed,
                     void *__u_result) {
    void *__u_types[UFFI_MAX_ARGS];
    void *__u_rtype; int __u_i;
    if (__u_n < 0 || __u_n > UFFI_MAX_ARGS || (__u_n != 0 && __u_kinds == 0)) return 0 - 3;
    __u_rtype = uffi_type(__u_return_kind);
    __u_i = 0;
    while (__u_i < __u_n) {
        __u_types[__u_i] = uffi_type(__u_kinds[__u_i]);
        __u_i = __u_i + 1;
    }
    return uffi_call_types(__u_fn, __u_rtype, __u_types, __u_values,
                           __u_n, __u_fixed, __u_result);
}
#endif
#endif
