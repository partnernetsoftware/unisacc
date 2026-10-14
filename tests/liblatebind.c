/* liblatebind.c PACKAGE TARGET -- library late binding, step 2a: the declaration contract.
   Test-only native process, built and run by tests/liblatebindcheck.cx.  It links a private
   copy of the runtime (as tests/libunisacclifecycle.c does) and uses only public API, plus the
   non-stable us_binding_contract getter added for 0.0.43.

   The contract is the address-free description of what the caller declared.  Checked:
     same      two contexts that declare the same functions with different host addresses
               produce byte-identical contracts; a third that declares them in another order
               produces the same bytes too
     shape     "USLCON1\n" header, length > 12, no 8-byte window equal to either context's
               host addresses (little-endian), repeated queries return equal bytes
     change    declaring one more function changes the contract
     refuse    an unknown target returns NULL with length 0 and a named error
     compat    the old path still works: compile, relocate, call a function that calls the
               injected host function, and get each context's own host result
   Prints one line per group; exit 0 only when every group holds. */
#include "exec/c/libunisacc.c"

static long add_a(long x, long y) { return x + y + 1000; }
static long add_b(long x, long y) { return x + y + 2000; }
static long twice_a(long x) { return 2 * x + 10; }
static long twice_b(long x) { return 2 * x + 20; }
static int bad;
static void report(int ok, const char *what) { printf("  %s %s\n", ok ? "ok  " : "FAIL", what); if (!ok) bad++; }

static us_type_descriptor integer(void) { us_type_descriptor t; memset(&t, 0, sizeof t); t.kind = 1; t.width = 8; return t; }

static us_context *declare(const char *package, int b, int reversed, int extra) {
    static us_type_descriptor two[2], one[1];
    us_signature add, twice;
    us_context *c = us_new(package);
    if (!c) return 0;
    two[0] = two[1] = one[0] = integer();
    memset(&add, 0, sizeof add); add.result = integer(); add.args = two; add.count = 2;
    memset(&twice, 0, sizeof twice); twice.result = integer(); twice.args = one; twice.count = 1;
    if (!reversed) {
        if (us_add_symbol(c, "hostadd", (void *)(b ? add_b : add_a), &add)) goto fail;
        if (us_add_symbol(c, "hosttwice", (void *)(b ? twice_b : twice_a), &twice)) goto fail;
    } else {
        if (us_add_symbol(c, "hosttwice", (void *)(b ? twice_b : twice_a), &twice)) goto fail;
        if (us_add_symbol(c, "hostadd", (void *)(b ? add_b : add_a), &add)) goto fail;
    }
    if (extra && us_add_symbol(c, "hostthird", (void *)twice_a, &twice)) goto fail;
    return c;
fail:
    fprintf(stderr, "declaration failed: %s\n", us_error(c)); us_free(c); return 0;
}

/* a private copy of the contract: the getter's bytes are only valid until the next query */
static unsigned char *contract(us_context *c, const char *target, size_t *n) {
    const void *p = us_binding_contract(c, target, n); unsigned char *copy;
    if (!p) return 0;
    copy = malloc(*n ? *n : 1); if (copy) memcpy(copy, p, *n);
    return copy;
}
static int holds_word(const unsigned char *b, size_t n, uint64_t w) {
    size_t i; unsigned char le[8]; int k;
    for (k = 0; k < 8; k++) le[k] = (unsigned char)(w >> (8 * k));
    for (i = 0; i + 8 <= n; i++) if (!memcmp(b + i, le, 8)) return 1;
    return 0;
}

int main(int argc, char **argv) {
    const char *pkg, *target; size_t na, nb, nr, n2, nx, nz; unsigned char *a, *b, *r, *a2, *x; const void *z;
    us_context *ca, *cb, *cr, *cx;
    if (argc != 3) { fprintf(stderr, "usage: liblatebind PACKAGE TARGET\n"); return 2; }
    pkg = argv[1]; target = argv[2];
    ca = declare(pkg, 0, 0, 0); cb = declare(pkg, 1, 0, 0); cr = declare(pkg, 0, 1, 0); cx = declare(pkg, 0, 0, 1);
    if (!ca || !cb || !cr || !cx) { printf("  FAIL contexts could not be set up\n"); return 1; }

    a = contract(ca, target, &na); b = contract(cb, target, &nb); r = contract(cr, target, &nr);
    a2 = contract(ca, target, &n2); x = contract(cx, target, &nx);
    if (!a || !b || !r || !a2 || !x) { printf("  FAIL contract query failed: %s\n", us_error(ca)); return 1; }

    report(na == nb && !memcmp(a, b, na), "same: different host addresses give the same contract");
    report(na == nr && !memcmp(a, r, na), "same: declaration order does not change the contract");
    report(na > 12 && !memcmp(a, "USLCON1\n", 8), "shape: USLCON1 header");
    report(!holds_word(a, na, (uint64_t)(uintptr_t)add_a) && !holds_word(a, na, (uint64_t)(uintptr_t)twice_a) &&
           !holds_word(b, nb, (uint64_t)(uintptr_t)add_b) && !holds_word(b, nb, (uint64_t)(uintptr_t)twice_b),
           "shape: no host address in the contract");
    report(na == n2 && !memcmp(a, a2, na), "shape: a repeated query returns the same bytes");
    report(nx != na || memcmp(a, x, na), "change: one more declaration changes the contract");

    nz = 99; z = us_binding_contract(ca, "nonesuch/arch", &nz);
    report(!z && nz == 0 && us_error(ca) && *us_error(ca), "refuse: an unknown target is refused by name");

    {
        static const char *SRC = "long hostadd(long a, long b); long hosttwice(long a);\n"
                                 "long answer(void) { return hostadd(hosttwice(5), 7); }\n";
        long (*fa)(void), (*fb)(void); int ok = 1;
        ok = !us_add_source(ca, "a.c", SRC) && !us_compile(ca, target, 1) && !us_relocate(ca) &&
             !us_add_source(cb, "b.c", SRC) && !us_compile(cb, target, 1) && !us_relocate(cb);
        if (!ok) printf("  info compat: %s / %s\n", us_error(ca), us_error(cb));
        fa = ok ? (long (*)(void))us_sym(ca, "answer") : 0; fb = ok ? (long (*)(void))us_sym(cb, "answer") : 0;
        report(fa && fb && fa() == 1027 && fb() == 2037, "compat: each context calls its own host functions");
    }
    free(a); free(b); free(r); free(a2); free(x);
    us_free(ca); us_free(cb); us_free(cr); us_free(cx);
    printf("liblatebind 2a  %d failures\n", bad);
    return bad ? 1 : 0;
}
