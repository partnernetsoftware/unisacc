/* R13-0c #36: `#if` over a function-like unknown identifier, written at top
   level rather than under a false `defined(...)`.

   #35 was the Python front end refusing this shape.  #36 is the product: the
   E2 network answers `not covered: #if expression` where the C reference
   evaluates the unknown name to 0 and takes the false branch.  This is why
   the two are separate entries -- miniz never reaches it, because its
   `defined(__has_feature)` is false and the group is skipped, so this does not
   block fb12-23.

   Expected output is the C reference's.  Listed in tests/fb12.knownfail until
   the E2 side covers it (cc-unisacc). */
#include <stdio.h>

#if __has_feature(undefined_behavior_sanitizer)
#define TOPLEVEL 1
#else
#define TOPLEVEL 0
#endif

#if __has_builtin(__builtin_expect)
#define TOPBUILTIN 1
#else
#define TOPBUILTIN 0
#endif

#if __has_include(<stdio.h>)
#define TOPINCLUDE 1
#else
#define TOPINCLUDE 0
#endif

int main(void) {
    printf("toplevel=%d\n", TOPLEVEL);
    printf("topbuiltin=%d\n", TOPBUILTIN);
    printf("topinclude=%d\n", TOPINCLUDE);
    return 0;
}
