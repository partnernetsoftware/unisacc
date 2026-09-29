/* A quoted #include inside a header ("h2.h" in lib/h1.h) is looked up
   relative to the MAIN file's directory (lib/sub/), not the directory of
   the header that contains it (lib/).  gcc, clang and MSVC all search the
   including file's directory first.  Hit by sbase: libutil/eprintf.c
   includes "../util.h", which includes "arg.h" and "compat.h". */
#include <stdio.h>
#include "../h1.h"
int main(void) { printf("%d\n", H2_VAL); return 0; }
