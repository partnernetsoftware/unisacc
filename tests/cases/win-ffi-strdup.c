/* W45-WIN-FFI probe: strdup is forwarded to the host C library on every target
   (Windows: GetProcAddress over ucrtbase/msvcrt via <unisacc_ffi.h> uffi_dlsym). */
#include <string.h>
#include <stdio.h>
int main(void) {
    char *s = strdup("hi");
    if (!s) return 1;
    puts(s);
    return 0;
}
