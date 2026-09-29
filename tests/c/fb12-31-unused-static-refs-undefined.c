/* R13-0b: external trial of 0.0.12, real-world batch.  Source: ~/unisacc-feedback-0.0.12-realworld/31-unused-static-refs-undefined.c (+ .out.txt for the gcc-vs-unisacc record). */
/* An UNUSED static (inline) function that calls an external function
   which is declared but never defined makes the build fail with
   "unisacc: error: undefined function 'ext_never_defined'".  gcc/clang
   never emit the unused static function, so nothing references the
   symbol and the program links.  Header-only wrapper sets hit this:
   miniz.h's zlib-compat wrappers (static inline deflateInit ->
   mz_deflateInit, ...) make any program that includes miniz.h but does
   not link miniz.c's deflate part fail with 19 such errors. */
#include <stdio.h>
int ext_never_defined(int);
static inline int wrapper(int x) { return ext_never_defined(x); }
int main(void) { printf("ok\n"); return 0; }
