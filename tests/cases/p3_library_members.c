/* p3_library_members.c PACKAGE TARGET PHASE -- test-only native host for tests/p3-library-members.cx.
   Links a private copy of the runtime and uses only the public library API. Each phase runs on
   fresh contexts and prints exactly one line, "OK <phase> <detail>" or "ERR <phase> <us_error>":
     contract  address-free binding contract query        (library-contract route)
     compile   injected host functions, compile, relocate,
               call: two contexts keep their own hosts      (library-contract + library-selection)
     carrier   script export of a union by value, compile
               and relocate                                 (<target>/nativeabi, if certified)
   The exit status is 0 for OK and 1 for ERR; argument errors exit 2. */
#include "exec/c/libunisacc.c"

static long add_a(long x, long y) { return x + y + 1000; }
static long add_b(long x, long y) { return x + y + 2000; }
static long twice_a(long x) { return 2 * x + 10; }
static long twice_b(long x) { return 2 * x + 20; }

static us_type_descriptor integer(void) { us_type_descriptor t; memset(&t, 0, sizeof t); t.kind = 1; t.width = 8; return t; }

static us_context *declare(const char *package, int b) {
    static us_type_descriptor two[2], one[1];
    us_signature add, twice;
    us_context *c = us_new(package);
    if (!c) return 0;
    two[0] = two[1] = one[0] = integer();
    memset(&add, 0, sizeof add); add.result = integer(); add.args = two; add.count = 2;
    memset(&twice, 0, sizeof twice); twice.result = integer(); twice.args = one; twice.count = 1;
    if (us_add_symbol(c, "hostadd", (void *)(b ? add_b : add_a), &add) ||
        us_add_symbol(c, "hosttwice", (void *)(b ? twice_b : twice_a), &twice)) {
        printf("ERR declare %s\n", us_error(c)); us_free(c); return 0;
    }
    return c;
}

static int phase_contract(const char *pkg, const char *target) {
    us_context *c = declare(pkg, 0); size_t n = 0; const void *p;
    if (!c) return 1;
    p = us_binding_contract(c, target, &n);
    if (!p || n <= 12 || memcmp(p, "USLCON1\n", 8)) { printf("ERR contract %s\n", us_error(c)); us_free(c); return 1; }
    printf("OK contract USLCON1 bytes=%zu\n", n); us_free(c); return 0;
}

static int phase_compile(const char *pkg, const char *target) {
    static const char *SRC = "long hostadd(long a, long b); long hosttwice(long a);\n"
                             "long answer(void) { return hostadd(hosttwice(5), 7); }\n";
    us_context *a = declare(pkg, 0), *b = declare(pkg, 1); long (*fa)(void), (*fb)(void); int rc = 1;
    if (!a || !b) { us_free(a); us_free(b); return 1; }
    if (us_add_source(a, "a.c", SRC) || us_compile(a, target, 1) || us_relocate(a)) printf("ERR compile %s\n", us_error(a));
    else if (us_add_source(b, "b.c", SRC) || us_compile(b, target, 1) || us_relocate(b)) printf("ERR compile %s\n", us_error(b));
    else {
        fa = (long (*)(void))us_sym(a, "answer"); fb = (long (*)(void))us_sym(b, "answer");
        if (fa && fb && fa() == 1027 && fb() == 2037) { printf("OK compile answers=1027,2037\n"); rc = 0; }
        else printf("ERR compile wrong-host-answers\n");
    }
    us_free(a); us_free(b); return rc;
}

static int phase_carrier(const char *pkg, const char *target) {
    static const char *SRC = "union mix { double d; long u; };\n"
                             "union mix bump(union mix x) { x.u += 17; return x; }\n";
    us_context *c = us_new(pkg); int rc = 1;
    if (!c) { printf("ERR carrier context\n"); return 1; }
    if (us_add_source(c, "u.c", SRC) || us_compile(c, target, 1) || us_relocate(c)) printf("ERR carrier %s\n", us_error(c));
    else { printf("OK carrier export=%s\n", us_sym(c, "bump") ? "present" : "absent"); rc = 0; }
    us_free(c); return rc;
}

int main(int argc, char **argv) {
    if (argc != 4) { fprintf(stderr, "usage: p3_library_members PACKAGE TARGET contract|compile|carrier\n"); return 2; }
    if (!strcmp(argv[3], "contract")) return phase_contract(argv[1], argv[2]);
    if (!strcmp(argv[3], "compile")) return phase_compile(argv[1], argv[2]);
    if (!strcmp(argv[3], "carrier")) return phase_carrier(argv[1], argv[2]);
    fprintf(stderr, "unknown phase %s\n", argv[3]); return 2;
}
