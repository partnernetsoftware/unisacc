/* stagemap.c -- the compiler-stage axis of the gate (run with: unisacc.com -run tests/stagemap.c).
 *
 * gatelayers says how broad a suite's claim is; this says which stage it tests, so a change to
 * one stage runs that stage's suites first and the whole chain after.  Stages follow prd.md
 * section 3: E2 pp, E1 lex, E3 parse/type/tape, E4 opt+prune, lower, E5 enc, E6 image/load/runtime.
 * Suites about no single stage go in: chain (whole route vs a reference), lib (shipped library and
 * host interop), build (seed, self-hosting, packaging), meta (ledgers, inventories, runner rules).
 * The table only labels; it never removes a suite.  A suite no rule matches is an error.  `also`
 * marks a plausible second stage: overlaps are listed for review, never merged here.
 *
 * Suite names come on stdin (tests/gate.sh --list --com).
 *   --check           every name has a stage; print counts
 *   --stage S [...]   print the names in these stages
 *   --list            print stage, name, also
 *   --overlaps        print names with an also-stage
 * Patterns: '*' matches any run; a name is tried as is, without a trailing -N shard, and without
 * a -lnx-/-osx-/-win- target suffix; a com- prefix is the product route of the same suite.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static char *rules[] = {
    "meta", "c99-ledger decision-ledger ledgercheck finite-template dsl-ops manifest-entries-* script-inventory"
            " subtract-safety* freezecheck revivedscan facts-export publish-order gate-infra gate-layers"
            " tsv-build-account pipeline-cache docs kernel source-layout front-bounds bound fresh-order*"
            " modelbenchcheck package-footer ape-version proc-enum qprefix full-signature referee-* tools"
            " lib-bindings-registry",
    "E2", "exec-pp-* exec-pploc exec-macros pptruth shared-e2-* rowcov-pp* prevheaders",
    "E1", "exec-lex* exec-f1-sidecar strconvert-* rowcov-lex*",
    "E3", "exec-parse* exec-r21-e3 exec-unit* exec-diag exec-errors* exec-f1-attributes exec-*warn*"
          " exec-neg exec-e3self exec-decimal tapebin* tape-reader rowcov-parse2* declmatrix declshape"
          " volatile-comma diag* c99 staticinit staticunits tagforward parserbounds hostile cli ccparity"
          " formatonce warn* seed-construct-parse2 seedparse2*",
    "E4", "opt-run exec-e4* exec-prune*",
    "lower", "exec-lower exec-armlower exec-winlower exec-lowdata exec-sparse rowcov-lower* forward*"
             " ccinterop ffi-* exec-bridge-linux windows-resolver-host exec-bind* libneed",
    "E5", "exec-e5 exec-arm exec-sha exec-asm* rowcov-enc* asmtext combo exec-memory-asm exec-memx86-asm",
    "E6", "exec-elf* exec-armelf exec-mach* exec-x86win exec-armwin exec-pe* exec-object* exec-modelobject"
          " elfobj linkunits run fat exec-memory-* exec-memx86-* exec-memwin* exec-binaryio exec-crcllp64"
          " malloc memalign hosthdr syscall6 winposix auditnet exec-container exec-package exec-net"
          " exec-codec exec-core exec-embedded target-package lifecycle exec-srcelf",
    "chain", "difftest* csmithdiff* corpus* ccrun* closure* stages exec-chain exec-native-* exec-formats*"
             " exec-multi-* multi fb12-multi realprog luatests* sqlspeed* minicon apps-* bigclosure",
    "lib", "lib-*",
    "build", "seed* exec-*self* exec-selfprep-* exec-bootstrap-* nativeboot-* comboot-* exec-driver-*"
             " exec-tableself",
    0
};
static char *also[] = {
    "build", "rowcov-*-build rowcov-*-union strconvert-seed seedparse2* seed-construct-parse2 closure-*"
             " bigclosure exec-e3self exec-e4self",
    "E6", "exec-bind* combo asmtext",
    "lib", "forward* ccinterop ffi-*",
    "meta", "tapebin*",
    "chain", "exec-memory-* exec-memx86-* exec-memwin*",
    "E3", "exec-multi-* multi staticunits linkunits",
    0
};

static int glob(char *p, char *s) {
    if (*p == 0) return *s == 0;
    if (*p == '*') { for (;; s++) { if (glob(p + 1, s)) return 1; if (*s == 0) return 0; } }
    return *p == *s && glob(p + 1, s + 1);
}

/* does any space-separated pattern in list match word? */
static int inlist(char *list, char *word) {
    char pat[128]; int i;
    while (*list) {
        while (*list == ' ') list++;
        i = 0; while (*list && *list != ' ' && i < 127) pat[i++] = *list++;
        pat[i] = 0;
        if (i && glob(pat, word)) return 1;
    }
    return 0;
}

static void forms(char *name, char f[3][256]) {
    char *b = strncmp(name, "com-", 4) == 0 ? name + 4 : name; int n, k;
    strncpy(f[0], b, 255); f[0][255] = 0;
    strcpy(f[1], f[0]); n = strlen(f[1]); k = n;
    while (k > 0 && f[1][k - 1] >= '0' && f[1][k - 1] <= '9') k--;
    if (k < n && k > 0 && f[1][k - 1] == '-') f[1][k - 1] = 0;
    strcpy(f[2], f[0]);
    { char *t = strstr(f[2], "-lnx-"); if (!t) t = strstr(f[2], "-osx-"); if (!t) t = strstr(f[2], "-win-"); if (t) *t = 0; }
}

static char *table(char **tab, char *name, char *skip) {
    char f[3][256]; int r, j;
    forms(name, f);
    for (r = 0; tab[r]; r += 2)
        for (j = 0; j < 3; j++)
            if ((!skip || strcmp(skip, tab[r])) && inlist(tab[r + 1], f[j])) return tab[r];
    return 0;
}

int main(int argc, char **argv) {
    static char names[1024][128]; int n = 0, i, j, bad = 0, nover = 0;
    char line[256], *mode = argc > 1 ? argv[1] : "--check";
    while (fgets(line, sizeof line, stdin) && n < 1024) {
        line[strcspn(line, "\r\n")] = 0;
        if (*line) { strncpy(names[n], line, 127); names[n][127] = 0; n++; }
    }
    if (n == 0) { fprintf(stderr, "stagemap: no suite names on stdin\n"); return 1; }
    if (table(rules, "new-suite-without-a-stage", 0)) { fprintf(stderr, "stagemap: unknown names must fail\n"); return 1; }
    for (i = 0; i < n; i++) {
        char *st = table(rules, names[i], 0), *al;
        if (!st) { fprintf(stderr, "stagemap: suite without a stage (add it to tests/stagemap.c): %s\n", names[i]); bad = 1; continue; }
        al = table(also, names[i], st);
        if (al) nover++;
        if (!strcmp(mode, "--list")) printf("%s\t%s\t%s\n", st, names[i], al ? al : "-");
        else if (!strcmp(mode, "--overlaps")) { if (al) printf("%s\t%s\talso %s\n", st, names[i], al); }
        else if (!strcmp(mode, "--stage")) { for (j = 2; j < argc; j++) if (!strcmp(argv[j], st)) printf("%s\n", names[i]); }
    }
    if (bad) return 1;
    if (!strcmp(mode, "--check")) {
        int r, c;
        printf("gate stages: %d suites;", n);
        for (r = 0; rules[r]; r += 2) {
            c = 0; for (i = 0; i < n; i++) if (!strcmp(table(rules, names[i], 0), rules[r])) c++;
            printf(" %s %d", rules[r], c);
        }
        printf("; overlaps %d\n", nover);
    }
    return 0;
}
