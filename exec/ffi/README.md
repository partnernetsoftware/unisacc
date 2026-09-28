# Native libffi runtime

`include/unisacc_ffi.h` uses four compiler bootstrap intrinsics
`__hostaddr0()` through `__hostaddr3()` (dlopen, dlsym, dlclose, dlerror)
and `__hostcall(fn, argv)`, where argv holds exactly six 64-bit values.
The bridge invokes native ABI functions, not the tape calling convention.

`uffi_call` takes a return kind, an explicit argument kind vector, addresses
of argument values, an argument count, a fixed count (`-1` for a fixed
signature), and result storage. `uffi_call_types` accepts native ffi_type
pointers directly. A call is never performed after preparation fails.
Variadic arguments must already have default promotions; float and narrow
integer variadic types are rejected. Non-void result storage must be at
least eight bytes and suitably aligned. A pointer argument is the address
of a pointer value, not the pointed-to buffer itself.

The current declaration supports macOS arm64 and x86_64 only. ABI and CIF
layout come from the local Apple SDK's `ffi/ffi.h`,
`ffi/ffitarget_arm64.h`, and `ffi/ffitarget_x86.h`: arm64 ABI 1, CIF 40
bytes with aarch64_nfixedargs; x86_64 ABI 2, CIF 32 bytes. Runtime checks
also require 64-bit long and pointers. Other platforms fail preprocessing.
Initialization does not allocate, so it can precede forwarding malloc.
It keeps its libffi handle alive. Runtime static initialization is not
thread safe; callbacks and closure generation are not provided here.

`hostcheck.sh` compiles a host-only shim that calls the bootstrap and libffi
entry points using their actual native prototypes. This verifies declaration
layout and typed calls to real system functions, including mixed variadic
arguments. It **does not verify unisacc's compiler intrinsics or assembly
bridge**. `probe.c` without UFFI_HOST_SHIM is the corresponding unisacc
integration probe once those intrinsics exist.

No bundled FILE or va_list crosses this interface. System stdio forwarding
must adopt system FILE objects and supply typed vararg vectors; copying the
bundled va_list representation into host vprintf is invalid. Returned host
allocation pointers must be released by the same host allocation family.
