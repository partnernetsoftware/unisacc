/* R13-0c #35: `#if` over an unknown name followed by `(`, and a skipped
   group that was evaluated anyway.

   Two defects, one shape.  miniz.h:211 has

       #if defined(__has_feature)
       #if __has_feature(undefined_behavior_sanitizer)

   and the Python front end refused the file outright:

       ValueError: unconsumed token in #if: (

   Neither `__has_feature` nor `__has_builtin` nor `__has_include` is a macro
   here, so all three are ordinary unknown identifiers -- and the C reference
   evaluates every one of them to 0 without complaint (measured: six forms,
   all FALSE, all accepted).  Two things were wrong on the Python side:

     1. an unknown name followed by `(` left the `(` in the token stream, so
        the expression was reported as unconsumed.  The parenthesised group is
        now consumed and skipped, and the whole thing is 0.
     2. a skipped group was still evaluated.  `defined(__has_feature)` is
        false, so C99 6.10.1 says the inner `#if` is counted for nesting and
        nothing more -- but it was still put through the evaluator.  That is
        what turned a false outer condition into a hard error.

   Both are signed-value errors, not semantic ones: nothing here asks for
   clang's `__has_feature` behaviour, only for the C reference's.
   `__has_include` doing a real search is deliberately not implemented (TODO).

   gcc takes different branches for `__has_builtin` and `__has_include` (both
   are true there), so this file compares the unisacc routes only: the C
   reference on `src/` and the Python front end must agree, branch for branch.
   On the product, `#if __has_feature(x)` written at top level is R13-0c #36
   and is listed in tests/difftest.com.knownfail -- it does not affect the
   paths below, which reach it only under a false `defined`. */
#include <stdio.h>

#if defined(__has_feature)
#define HAS_FEATURE_DEFINED 1
#else
#define HAS_FEATURE_DEFINED 0
#endif

#if defined(__has_builtin)
#define HAS_BUILTIN_DEFINED 1
#else
#define HAS_BUILTIN_DEFINED 0
#endif

#if __has_feature(undefined_behavior_sanitizer)
#define HAS_FEATURE 1
#else
#define HAS_FEATURE 0
#endif

#if __has_builtin(__builtin_expect)
#define HAS_BUILTIN 1
#else
#define HAS_BUILTIN 0
#endif

#if __has_include(<stdio.h>)
#define HAS_INCLUDE 1
#else
#define HAS_INCLUDE 0
#endif

/* the short-circuit form, which failed at the parsing stage, not evaluation */
#if 0 && __has_feature(anything_at_all)
#define SHORT_CIRCUIT 1
#else
#define SHORT_CIRCUIT 0
#endif

/* the skipped group: every line here is counted for nesting and never
   evaluated, so the condition inside may be anything at all */
#if 0
#if __has_feature(x)
#endif
#if defined(__has_builtin)
#else
#endif
#endif

int main(void) {
    /* Each of the six is 0 on both unisacc routes.  They are printed one per
       line so a wrong branch is visible in the output, not only in an exit
       status. */
    printf("defined_has_feature=%d\n", HAS_FEATURE_DEFINED);
    printf("defined_has_builtin=%d\n", HAS_BUILTIN_DEFINED);
    printf("has_feature=%d\n", HAS_FEATURE);
    printf("has_builtin=%d\n", HAS_BUILTIN);
    printf("has_include=%d\n", HAS_INCLUDE);
    printf("short_circuit=%d\n", SHORT_CIRCUIT);
    return 0;
}
