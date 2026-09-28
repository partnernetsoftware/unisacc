#ifdef UFFI_HOST_SHIM
#include "hostshim.h"
#endif
#include "../../include/unisacc_ffi.h"
#include <stdio.h>
static int failures;
static void check(int ok, char *name) {
    if (!ok) failures = failures + 1;
    printf("%s %s\n", ok ? "PASS" : "FAIL", name);
}
int main(void) {
    void *sys; void *math; void *fn; void *p; void *values[6];
    int kinds[6]; long result; long length; int n; int k;
    char *s; char *fmt; char buffer[80]; char *dest;
    double x; double y;
    check(uffi_init() == 0, "init");
#ifdef UFFI_HOST_SHIM
    check(sizeof(struct uffi_cif) == sizeof(ffi_cif), "sdk-cif-size");
    check(UFFI_ABI == FFI_DEFAULT_ABI, "sdk-abi");
#endif
    sys = uffi_dlopen("/usr/lib/libSystem.B.dylib", 2);
    check(sys != 0, "dlopen");
    fn = uffi_dlsym(sys, "strlen");
    s = "host libc"; kinds[0] = UFFI_POINTER; values[0] = &s; result = 0;
    check(uffi_call(fn, UFFI_ULONG, kinds, values, 1, -1, &result) == 0 && result == 9, "strlen-pointer-size");
    fn = uffi_dlsym(sys, "abs"); k = -17; kinds[0] = UFFI_INT; values[0] = &k; result = 0;
    check(uffi_call(fn, UFFI_INT, kinds, values, 1, -1, &result) == 0 && (int)result == 17, "int-argument-return");
    fn = uffi_dlsym(sys, "labs"); length = -12345678901; kinds[0] = UFFI_LONG; values[0] = &length; result = 0;
    check(uffi_call(fn, UFFI_LONG, kinds, values, 1, -1, &result) == 0 && result == 12345678901, "long-argument-return");
    fn = uffi_dlsym(sys, "malloc"); length = 64; kinds[0] = UFFI_ULONG; values[0] = &length; p = 0;
    check(uffi_call(fn, UFFI_POINTER, kinds, values, 1, -1, &p) == 0 && p != 0, "malloc-pointer-return");
    fn = uffi_dlsym(sys, "free"); kinds[0] = UFFI_POINTER; values[0] = &p;
    check(uffi_call(fn, UFFI_VOID, kinds, values, 1, -1, 0) == 0, "free-void-return");
    math = uffi_dlopen("/usr/lib/libSystem.B.dylib", 2);
    fn = uffi_dlsym(math, "sqrt"); x = 6.25; y = 0;
    kinds[0] = UFFI_DOUBLE; values[0] = &x;
    check(uffi_call(fn, UFFI_DOUBLE, kinds, values, 1, -1, &y) == 0 && y == 2.5, "double-argument-return");
    fn = uffi_dlsym(sys, "snprintf"); dest = buffer; length = 80; fmt = "%d %.1f %s"; k = 42; x = 2.5; s = "ffi";
    kinds[0] = UFFI_POINTER; kinds[1] = UFFI_ULONG; kinds[2] = UFFI_POINTER;
    kinds[3] = UFFI_INT; kinds[4] = UFFI_DOUBLE; kinds[5] = UFFI_POINTER;
    values[0] = &dest; values[1] = &length; values[2] = &fmt;
    values[3] = &k; values[4] = &x; values[5] = &s; result = 0;
    n = uffi_call(fn, UFFI_INT, kinds, values, 6, 3, &result);
    check(n == 0 && (int)result == 10 && buffer[0] == '4' && buffer[3] == '2' && buffer[7] == 'f', "variadic-mixed");
    fn = uffi_dlsym(sys, "getpid"); result = 0;
    check(uffi_call(fn, UFFI_INT, 0, 0, 0, -1, &result) == 0 && result > 0, "zero-arguments");
    check(uffi_call(0, UFFI_INT, 0, 0, 0, -1, &result) == -3, "reject-null-symbol");
    check(uffi_call(fn, UFFI_INT, kinds, values, 33, -1, &result) == -3, "reject-vector-bound");
    check(uffi_call(fn, UFFI_INT, 0, 0, 0, -2, &result) == -3, "reject-fixed-bound");
    kinds[0] = UFFI_FLOAT; values[0] = &x;
    check(uffi_call(fn, UFFI_INT, kinds, values, 1, 0, &result) == -3, "reject-unpromoted-vararg");
    check(uffi_type(12) == 0, "reject-type");
    check(uffi_dlsym(sys, "__unisacc_missing_symbol_928") == 0 && uffi_dlerror() != 0, "dlerror");
    check(uffi_dlclose(math) == 0 && uffi_dlclose(sys) == 0, "dlclose");
    printf("END failures=%d\n", failures);
    return failures != 0;
}
