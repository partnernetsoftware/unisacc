/* c43 2c2 fixed-typed black-box probe (DRAFT, not built by any gate).
 * Structure follows tests/liblatebind2c1.c: public header only, no private context fields, no adapter or
 * model inspection.  Entry points that do not exist yet are NOT declared at link time: each is resolved with
 * dlsym(RTLD_DEFAULT) at run time, and a missing entry fails by name ("MISSING-API <name>").  Nothing is faked.
 *
 * usage: liblatebind2c2 PACKAGE TARGET EXPECT_DIR
 *   EXPECT_DIR holds the bytes of the 2a/2b/2c1 fragments and of the no-typed baseline, recorded at the
 *   2c1 landing HEAD (the P6 baseline).  Byte comparison only; no value is assumed here.
 *
 * Cases (research/c43-late-binding-cut2c2-step0.md):
 *   P1 accept: injected fixed-typed function int(int,int); typed fragment and ABI certificate non-empty;
 *      after the provider is registered the call returns the sum.
 *   P2 refuse arity: call site expects 2 args, declaration has 3 -> "arity-mismatch", names the symbol.
 *   P3 refuse callkind: signature with an undeclared callkind -> "unsupported-callkind".
 *   P4 refuse missing nargs: count > 0 with args == NULL -> "missing-nargs".
 *   P5 unbound not instantiated: no provider -> compile may succeed, the plan is not instantiated,
 *      the call refuses with "missing-provider" naming the symbol.  No address lookup by name.
 *   P6 no-callback regression: a program with no typed declarations emits fragments equal to EXPECT_DIR.
 *   P7 plug-in proof: the 2a/2b/2c1 fragments and receipts are byte-identical to EXPECT_DIR when the typed
 *      class is present in the same process.  Adding the class must not move any existing fragment.
 */
#define _POSIX_C_SOURCE 200809L
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "exec/c/libunisacc.h"

/* Draft names of the 2c2 entries; not stable until cc approves the 2c2 surface. */
typedef const void *(*fragment_fn)(us_context *, size_t *);
typedef int (*declare_fn)(us_context *, const char *, const us_signature *);
static const char *API_TYPED_FRAGMENT = "us_library_typed_fragment";     /* model plan requirement, E3 */
static const char *API_TYPED_CERT = "us_library_typed_certificate";      /* ABI certificate */
static const char *API_DECLARE = "us_declare_injected_import";            /* exists in the public header */
static const char *API_DATA_FRAGMENT = "us_library_data_fragment";       /* 2c1 */
static const char *API_E3_FRAGMENT = "us_library_e3_fragment";           /* 2a/2b */
static const char *API_LOWER_UNBOUND = "us_library_lower_unbound";       /* 2a/2b */

static int bad;
static void *resolve(const char *name) {
    void *p = dlsym(RTLD_DEFAULT, name);
    if (!p) { printf("MISSING-API %s\n", name); bad = 1; }
    return p;
}
static void check(int ok, const char *what) {
    printf("%s %s\n", ok ? "ok  " : "FAIL", what);
    if (!ok) bad = 1;
}

/* depth, base, shape, kind, width, uns: kind 1 integer, width 32 */
static const us_type_descriptor INT_ARGS[2] = {{0, 0, 0, 1, 32, 0}, {0, 0, 0, 1, 32, 0}};
static int pending;   /* checks not yet written: reported as PENDING, never as ok */
static void pend(const char *what) { printf("PENDING %s\n", what); pending = 1; }

int main(int argc, char **argv) {
    if (argc != 4) { fprintf(stderr, "usage: liblatebind2c2 PACKAGE TARGET EXPECT_DIR\n"); return 2; }
    /* Resolve every entry first, so the first missing one is named before any case runs. */
    declare_fn declare = (declare_fn)resolve(API_DECLARE);
    fragment_fn typed = (fragment_fn)resolve(API_TYPED_FRAGMENT);
    fragment_fn cert = (fragment_fn)resolve(API_TYPED_CERT);
    fragment_fn e3 = (fragment_fn)resolve(API_E3_FRAGMENT);
    fragment_fn data = (fragment_fn)resolve(API_DATA_FRAGMENT);
    fragment_fn lower = (fragment_fn)resolve(API_LOWER_UNBOUND);
    if (bad) { printf("probe: %s\n", "stopped before any case: missing entry points"); return 3; }

    us_context *c = us_new(argv[1]);
    check(c != NULL, "P1 context opens");
    us_signature sig = {0};
    sig.kind = 0;                     /* function */
    sig.result = (us_type_descriptor){0, 0, 0, 1, 32, 0};   /* integer */
    sig.args = INT_ARGS; sig.count = 2; sig.variadic = 0;
    check(declare(c, "add2", &sig) == 0, "P1 fixed-typed declaration accepted");
    size_t n = 0;
    check(typed(c, &n) != NULL && n > 0, "P1 typed plan requirement is non-empty");
    check(cert(c, &n) != NULL && n > 0, "P1 ABI certificate is non-empty");
    /* P2..P5 each use a fresh context; each refusal must name its reason and the symbol. */
    us_signature bad_arity = sig; bad_arity.count = 3;
    us_context *c2 = us_new(argv[1]);
    check(declare(c2, "add2", &bad_arity) != 0, "P2 arity mismatch refused");
    pend("P2 reason text 'arity-mismatch' naming add2 (stderr capture not written)");
    us_signature bad_kind = sig; bad_kind.kind = 9;
    us_context *c3 = us_new(argv[1]);
    check(declare(c3, "add2", &bad_kind) != 0, "P3 undeclared callkind refused");
    pend("P3 reason text 'unsupported-callkind' (stderr capture not written)");
    us_signature no_args = sig; no_args.args = NULL;
    us_context *c4 = us_new(argv[1]);
    check(declare(c4, "add2", &no_args) != 0, "P4 missing nargs refused");
    pend("P4 reason text 'missing-nargs' (stderr capture not written)");
    /* P5: no provider; the plan must not be instantiated, and the call must refuse by name. */
    check(typed(c, &n) != NULL, "P5 plan requirement exists before binding (no instantiation)");
    pend("P5 call refuses with missing-provider naming add2 (call path and stderr capture not written)");
    /* P6: a program with no typed declarations must match the recorded baseline bytes. */
    size_t bn = 0; const void *base = e3(c, &bn);
    check(base != NULL, "P6 2a/2b fragment is produced for the baseline program");
    pend("P6 byte compare of the fragment against EXPECT_DIR (argv[3]) not written");
    /* P7: the same 2a/2b/2c1 fragments with the typed class present; compare with EXPECT_DIR by the runner. */
    size_t dn = 0; const void *dat = data(c, &dn);
    size_t ln = 0; const void *low = lower(c, &ln);
    check(dat != NULL && low != NULL, "P7 2c1 data fragments are produced with the typed class present");
    pend("P7 byte compare of 2a/2b/2c1 fragments and receipts against EXPECT_DIR (argv[3]) not written");
    us_free(c2); us_free(c3); us_free(c4); us_free(c);
    printf("liblatebind2c2 %s%s\n", bad ? "FAILED" : "ok", pending ? " (with PENDING checks: not a pass)" : "");
    return bad ? 1 : (pending ? 4 : 0);
}
