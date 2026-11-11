/* c43 2c2 fixed-typed black-box probe (DRAFT, not built by any gate; revised after cdx's review).
 * Structure follows tests/liblatebind2c1.c: public header only.  Entry points that do not exist yet are NOT
 * declared at link time: each is resolved with dlsym(RTLD_DEFAULT) at run time, and a missing one fails by name
 * ("MISSING-API <name>") or, for private observers, is reported PENDING.  Nothing is faked, nothing is green
 * by default.
 *
 * Corrections from cdx's review (this revision):
 *   - us_type_descriptor.width is in BYTES: int is width 4 (was 32).
 *   - us_signature.kind is the signature kind (0 function, 1 data), NOT the call kind.  The 2c2 call kind is
 *     not a public field, so P3 tests an undeclared signature kind, and the call-kind refusal is PENDING.
 *   - count=3 reads args[0..2]: the arity case now declares a three-element array (no out-of-bounds read).
 *   - The typed plan is USLSIG3 bytes.  No public getter is added: the bytes come from a PRIVATE observer,
 *     resolved by dlsym; a missing observer is PENDING, never a pass.
 *   - New check P0: after compile and before relocate, the native plan must be EMPTY.  The current build is
 *     non-empty; this is recorded as a current RED (FAIL), not as a pass.
 *
 * usage: liblatebind2c2 PACKAGE TARGET EXPECT_DIR
 *   EXPECT_DIR holds the bytes of the 2a/2b/2c1 fragments and of the no-typed baseline, recorded at the
 *   2c1 landing HEAD (the P6 baseline).  Byte comparison is PENDING until the comparison is written.
 *
 * Status rule: P1..P7 may NOT be reported green from this draft.  A PENDING line is not a pass; a FAIL is red.
 */
#define _POSIX_C_SOURCE 200809L
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "exec/c/libunisacc.h"

typedef const void *(*fragment_fn)(us_context *, size_t *);
typedef int (*declare_fn)(us_context *, const char *, const us_signature *);
typedef const void *(*observer_fn)(us_context *, size_t *);

/* public entry points that exist today */
static const char *API_DECLARE = "us_declare_injected_import";
static const char *API_E3_FRAGMENT = "us_library_e3_fragment";           /* 2a/2b */
static const char *API_LOWER_UNBOUND = "us_library_lower_unbound";       /* 2a/2b */
static const char *API_DATA_FRAGMENT = "us_library_data_fragment";       /* 2c1 */
/* PRIVATE observers, draft names: not public API, must not be added to libunisacc.h for this probe */
static const char *OBS_TYPED_PLAN = "us_private_typed_plan_bytes";       /* USLSIG3 bytes of the typed plan */
static const char *OBS_NATIVE_PLAN = "us_private_native_plan_bytes";     /* native plan after compile, before relocate */

static int bad, pending;
static void *resolve(const char *name, int required) {
    void *p = dlsym(RTLD_DEFAULT, name);
    if (!p) {
        if (required) { printf("MISSING-API %s\n", name); bad = 1; }
        else { printf("PENDING private observer %s absent (not a pass)\n", name); pending = 1; }
    }
    return p;
}
static void check(int ok, const char *what) { printf("%s %s\n", ok ? "ok  " : "FAIL", what); if (!ok) bad = 1; }
static void pend(const char *what) { printf("PENDING %s\n", what); pending = 1; }

/* a type descriptor: depth, base, shape, kind (1 integer), width in BYTES (4), uns */
#define INT4 {0, 0, 0, 1, 4, 0}
static const us_type_descriptor INT_ARGS2[2] = {INT4, INT4};
static const us_type_descriptor INT_ARGS3[3] = {INT4, INT4, INT4};   /* count=3 must have three entries */

int main(int argc, char **argv) {
    if (argc != 4) { fprintf(stderr, "usage: liblatebind2c2 PACKAGE TARGET EXPECT_DIR\n"); return 2; }
    declare_fn declare = (declare_fn)resolve(API_DECLARE, 1);
    fragment_fn e3 = (fragment_fn)resolve(API_E3_FRAGMENT, 1);
    fragment_fn lower = (fragment_fn)resolve(API_LOWER_UNBOUND, 1);
    fragment_fn data = (fragment_fn)resolve(API_DATA_FRAGMENT, 1);
    observer_fn typed = (observer_fn)resolve(OBS_TYPED_PLAN, 0);
    observer_fn native = (observer_fn)resolve(OBS_NATIVE_PLAN, 0);
    if (bad) { printf("probe: stopped before any case: missing public entry points\n"); return 3; }

    us_context *c = us_new(argv[1]);
    check(c != NULL, "P1 context opens");
    /* signature kind 0 = function; the result and two arguments are 4-byte integers */
    us_signature sig = {0};
    sig.kind = 0;
    sig.result = (us_type_descriptor)INT4;
    sig.args = INT_ARGS2; sig.count = 2; sig.variadic = 0;
    check(declare(c, "add2", &sig) == 0, "P1 fixed-typed declaration accepted");
    if (typed) {
        size_t n = 0; const void *p = typed(c, &n);
        check(p != NULL && n > 0, "P1 typed plan (USLSIG3 bytes) is non-empty");
    } else pend("P1 typed plan bytes (private observer absent)");

    /* P0: after compile and before relocate, the native plan must be EMPTY.  Current build: non-empty (RED). */
    if (native) {
        size_t n = 0; const void *p = native(c, &n);
        check(p == NULL || n == 0, "P0 native plan is empty before relocate (CURRENT RED: plan is non-empty)");
    } else pend("P0 native plan emptiness (private observer absent)");

    /* P2: the call site expects 2 arguments, the declaration has 3 -> refused, names add2 (reason capture PENDING) */
    us_signature bad_arity = sig; bad_arity.count = 3; bad_arity.args = INT_ARGS3;
    us_context *c2 = us_new(argv[1]);
    check(declare(c2, "add2", &bad_arity) != 0, "P2 arity mismatch refused");
    pend("P2 reason text 'arity-mismatch' naming add2 (stderr capture not written)");

    /* P3: an undeclared signature kind (the call kind is not a public field) -> refused */
    us_signature bad_kind = sig; bad_kind.kind = 9;
    us_context *c3 = us_new(argv[1]);
    check(declare(c3, "add2", &bad_kind) != 0, "P3 undeclared signature kind refused");
    pend("P3 call-kind refusal 'unsupported-callkind' (call kind is not public; not tested here)");

    /* P4: count > 0 with no argument array -> refused */
    us_signature no_args = sig; no_args.args = NULL;
    us_context *c4 = us_new(argv[1]);
    check(declare(c4, "add2", &no_args) != 0, "P4 missing argument array refused");
    pend("P4 reason text 'missing-nargs' (stderr capture not written)");

    /* P5: no provider -> the call refuses by name; the plan must not be instantiated */
    pend("P5 call refuses with missing-provider naming add2 (call path and stderr capture not written)");

    /* P6: the no-typed baseline must match the recorded bytes at the 2c1 landing HEAD */
    size_t bn = 0; const void *base = e3(c, &bn);
    check(base != NULL, "P6 2a/2b fragment is produced for the baseline program");
    pend("P6 byte compare against EXPECT_DIR (argv[3]) not written");

    /* P7: 2a/2b/2c1 fragments are unchanged with the typed class present */
    size_t dn = 0, ln = 0; const void *dat = data(c, &dn); const void *low = lower(c, &ln);
    check(dat != NULL && low != NULL, "P7 2c1 data fragments are produced with the typed class present");
    pend("P7 byte compare of 2a/2b/2c1 fragments and receipts against EXPECT_DIR (argv[3]) not written");

    us_free(c2); us_free(c3); us_free(c4); us_free(c);
    printf("liblatebind2c2 %s%s\n", bad ? "FAILED (red present)" : "no red", pending ? " (PENDING checks: not a pass)" : "");
    return bad ? 1 : (pending ? 4 : 0);
}
