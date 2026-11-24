/* Public bridge configuration. Generated forwarding stubs include the internal
 * implementation directly, so their translation unit has no public exports. */
#ifndef _UNISACC_FFI_H
#define _UNISACC_FFI_H
#include <sys/_ffi_forward.h>

#ifndef _WIN32
#if !__UNISA_FTRIM_LIBC || __UN_uffi_call_types
void uffi_set_bridge_mode(int m) { __uffi_mode = m; __uffi_err = (m == UFFI_MODE_UNKNOWN) ? "UNISA_FFI_BRIDGE: unknown value" : 0; }
const char *uffi_last_error(void) { return __uffi_err ? __uffi_err : ""; }
#endif
#endif
#endif
