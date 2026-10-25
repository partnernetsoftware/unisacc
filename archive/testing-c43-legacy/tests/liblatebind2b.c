/* liblatebind2b.c PACKAGE TARGET -- library late binding, step 2b: one ordinary-function
   path whose model fragments carry no host address.  Test-only native process, built and run
   by tests/liblatebindcheck.cx once exec/c/libunisacc.h declares the 2b entry points.

   Interfaces (non-stable, research/c43-late-binding-cut2b-step0.md):
     us_library_e3_fragment(c, &n)    the model-marked ordinary wrapper fragment of the E3 tape
     us_library_lower_unbound(c, &n)  the matching unbound lower fragment (spans included); a
                                      read-only receipt produced by us_relocate, so it is queried
                                      after relocate (successful or refused for a missing provider)
     us_declare_injected_import(c, name, sig)  declare an ordinary function, no address

   Proof boundary: the fragments only.  The whole lower input still carries runtime-bridge
   addresses (mapping, argv, process slots); nothing here claims otherwise.

   Checked:
     same      two contexts, same declarations and source, different hostadd addresses: the
               E3 fragment and the unbound lower fragment are byte-identical, non-empty
     clean     neither fragment holds an 8-byte little-endian window equal to either address
     bound     after relocate each context gets its own host result
     missing   an injected-only ordinary import with no provider: compile succeeds, relocate
               fails with a diagnostic naming missing-provider and the function, and no
               callable symbol is published
   Exit 0 only when every group holds. */
#include "exec/c/libunisacc.c"

static long add_a(long x, long y) { return x + y + 1000; }
static long add_b(long x, long y) { return x + y + 2000; }
static int bad;
static void report(int ok, const char *what) { printf("  %s %s\n", ok ? "ok  " : "FAIL", what); if (!ok) bad++; }
static us_type_descriptor integer(void) { us_type_descriptor t; memset(&t, 0, sizeof t); t.kind = 1; t.width = 8; return t; }
static const char *SRC = "long hostadd(long a, long b);\nlong answer(void) { return hostadd(20, 7); }\n";

static us_signature add_signature(us_type_descriptor two[2]) {
    us_signature s; memset(&s, 0, sizeof s);
    two[0] = two[1] = integer(); s.result = integer(); s.args = two; s.count = 2; return s;
}
static unsigned char *copy(const void *p, size_t n) { unsigned char *c = malloc(n ? n : 1); if (c && p) memcpy(c, p, n); return c; }
static int holds(const unsigned char *b, size_t n, uint64_t w) {
    unsigned char le[8]; size_t i; int k;
    for (k = 0; k < 8; k++) le[k] = (unsigned char)(w >> (8 * k));
    for (i = 0; i + 8 <= n; i++) if (!memcmp(b + i, le, 8)) return 1;
    return 0;
}

int main(int argc, char **argv) {
    us_type_descriptor two[2]; us_signature sig = add_signature(two);
    us_context *ca, *cb, *cm; size_t ea = 0, eb = 0, la = 0, lb = 0; const void *p;
    unsigned char *e1, *e2, *l1, *l2; const char *target;
    if (argc != 3) { fprintf(stderr, "usage: liblatebind2b PACKAGE TARGET\n"); return 2; }
    target = argv[2];
    ca = us_new(argv[1]); cb = us_new(argv[1]); cm = us_new(argv[1]);
    if (!ca || !cb || !cm) { printf("  FAIL contexts could not be created\n"); return 1; }
    if (us_add_symbol(ca, "hostadd", (void *)add_a, &sig) || us_add_symbol(cb, "hostadd", (void *)add_b, &sig) ||
        us_add_source(ca, "a.c", SRC) || us_add_source(cb, "b.c", SRC) ||
        us_compile(ca, target, 1) || us_compile(cb, target, 1)) {
        printf("  FAIL setup: %s / %s\n", us_error(ca), us_error(cb)); return 1;
    }
    p = us_library_e3_fragment(ca, &ea); e1 = p ? copy(p, ea) : 0;
    p = us_library_e3_fragment(cb, &eb); e2 = p ? copy(p, eb) : 0;
    {   int ra = us_relocate(ca), rb = us_relocate(cb);
        if (ra || rb) printf("  info relocate: %s / %s\n", ra ? us_error(ca) : "ok", rb ? us_error(cb) : "ok"); }
    p = us_library_lower_unbound(ca, &la); l1 = p ? copy(p, la) : 0;
    p = us_library_lower_unbound(cb, &lb); l2 = p ? copy(p, lb) : 0;
    if (!e1 || !e2 || !l1 || !l2) { printf("  FAIL fragment query failed: %s / %s\n", us_error(ca), us_error(cb)); return 1; }
    report(ea > 0 && la > 0, "same: both fragments are non-empty (otherwise nothing is checked)");
    report(ea == eb && !memcmp(e1, e2, ea), "same: the E3 fragment does not depend on the host address");
    report(la == lb && !memcmp(l1, l2, la), "same: the unbound lower fragment does not depend on the host address");
    report(!holds(e1, ea, (uintptr_t)add_a) && !holds(e1, ea, (uintptr_t)add_b) &&
           !holds(l1, la, (uintptr_t)add_a) && !holds(l1, la, (uintptr_t)add_b), "clean: no host address in either fragment");
    {
        long (*fa)(void) = 0, (*fb)(void) = 0;
        fa = (long (*)(void))us_sym(ca, "answer"); fb = (long (*)(void))us_sym(cb, "answer");
        report(fa && fb && fa() == 1027 && fb() == 2027, "bound: each context calls its own host function after relocate");
    }
    {
        int ok = !us_declare_injected_import(cm, "hostadd", &sig) && !us_add_source(cm, "m.c", SRC) && !us_compile(cm, target, 1);
        const char *e;
        if (!ok) { printf("  info missing: setup failed: %s\n", us_error(cm)); }
        ok = ok && us_relocate(cm) != 0;
        e = us_error(cm);
        if (ok) { size_t mn = 0; const void *mp = us_library_lower_unbound(cm, &mn);
                  if (!mp || !mn) printf("  info missing: no unbound lower receipt after the refused relocate\n"); }
        ok = ok && e && strstr(e, "missing-provider") && strstr(e, "hostadd");
        ok = ok && !us_sym(cm, "answer");
        report(ok, "missing: an injected-only import without a provider is refused at relocate by name");
    }
    free(e1); free(e2); free(l1); free(l2); us_free(ca); us_free(cb); us_free(cm);
    printf("liblatebind 2b  %d failures\n", bad);
    return bad ? 1 : 0;
}
