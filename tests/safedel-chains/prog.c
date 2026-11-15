/* safedel chains-root whitelist test program; built by tools/check-safedel-chains.cx with -I <root>/include so that
 * <unisa_safedel.h> is the workspace header.  argv[1] = a fresh empty directory BASE; HOME=BASE/home, TMPDIR=BASE/tmp
 * (siblings, so neither is an ancestor of the other).  Prints "PASS<TAB>name" / "FAIL<TAB>name" and "TOTAL<TAB>n". */
#include <unisa_safedel.h>
#include <unistd.h>
#include <sys/stat.h>

static int g_total;
static void ok(int c, const char *name) { g_total++; printf("%s\t%s\n", c ? "PASS" : "FAIL", name); }
static char B[2048];
static void P(char *out, const char *rel) { snprintf(out, 4096, "%s/%s", B, rel); }
static void mk(const char *rel) { char p[4096]; P(p, rel); mkdir(p, 0755); }
static void touch(const char *rel) { char p[4096]; FILE *f; P(p, rel); f = fopen(p, "w"); if (f) { fputs("x", f); fclose(f); } }
static int exists(const char *rel) { char p[4096]; struct stat st; P(p, rel); return lstat(p, &st) == 0; }
static int del(const char *rel, char *why) { char p[4096]; P(p, rel); return unisa_safedel(p, SAFEDEL_PURGE | SAFEDEL_RECURSIVE, 0, why, 512); }
static int delabs(const char *p, char *why) { return unisa_safedel(p, SAFEDEL_PURGE | SAFEDEL_RECURSIVE, 0, why, 512); }

static void fixture(void) {
    mk("home"); mk("home/.cache"); mk("home/.cache/unisacc"); mk("home/.cache/unisacc/chains");
    mk("home/.cache/unisacc/chains/c1"); mk("home/.cache/unisacc/chains/c1/sub"); touch("home/.cache/unisacc/chains/c1/sub/f");
    mk("home/.cache/unisacc/chains-evil"); touch("home/.cache/unisacc/chains-evil/f");
    mk("home/.cache/unisacc/other"); touch("home/.cache/unisacc/other/f");
    mk("home/plain"); touch("home/plain/f");
    mk("tmp"); mk("tmp/t1"); touch("tmp/t1/f");
    mk("outside"); touch("outside/f");
}

int main(int argc, char **argv) {
    char why[512], p[4096], q[4096]; int rc;
    if (argc < 2 || strlen(argv[1]) > 1500 || argv[1][0] != '/') { printf("FAIL\targ\n"); return 2; }
    strcpy(B, argv[1]);
    fixture();
    P(p, "home"); setenv("HOME", p, 1); P(p, "tmp"); setenv("TMPDIR", p, 1);

    rc = del("home/.cache/unisacc/chains/c1", why);
    ok(rc == SAFEDEL_OK && !exists("home/.cache/unisacc/chains/c1") && exists("home/.cache/unisacc/chains"), "chains-subdir-deleted");
    rc = del("home/.cache/unisacc/chains/c1", why);
    ok(rc == SAFEDEL_OK, "chains-subdir-absent-ok");
    mk("home/.cache/unisacc/chains/c2"); touch("home/.cache/unisacc/chains/c2/f");
    rc = del("home/.cache/unisacc/chains/c2/f", why);
    ok(rc == SAFEDEL_OK && !exists("home/.cache/unisacc/chains/c2/f"), "chains-file-deleted");
    rc = del("home/.cache/unisacc/chains", why);
    ok(rc == SAFEDEL_REFUSED && exists("home/.cache/unisacc/chains/c2") && strstr(why, "chains root itself") != 0, "chains-root-refused");
    rc = del("home/.cache/unisacc/chains/", why);
    ok(rc == SAFEDEL_REFUSED && exists("home/.cache/unisacc/chains/c2"), "chains-root-trailing-slash-refused");
    rc = del("home/.cache/unisacc/chains/.", why);
    ok(rc == SAFEDEL_REFUSED && exists("home/.cache/unisacc/chains/c2"), "chains-root-dot-refused");
    rc = del("home/.cache/unisacc", why);
    ok(rc == SAFEDEL_REFUSED && exists("home/.cache/unisacc/other/f"), "cache-root-refused");
    rc = del("home/.cache/unisacc/other", why);
    ok(rc == SAFEDEL_REFUSED && exists("home/.cache/unisacc/other/f") && strstr(why, "$HOME/.cache/unisacc/chains") != 0, "cache-sibling-refused-and-message-lists-chains");
    rc = del("home/.cache/unisacc/chains-evil", why);
    ok(rc == SAFEDEL_REFUSED && exists("home/.cache/unisacc/chains-evil/f"), "prefix-sibling-chains-evil-refused");
    rc = del("home/.cache/unisacc/chains-evil/f", why);
    ok(rc == SAFEDEL_REFUSED && exists("home/.cache/unisacc/chains-evil/f"), "prefix-sibling-chains-evil-file-refused");
    rc = del("home/.cache/unisacc/chains/../chains-evil/f", why);
    ok(rc == SAFEDEL_REFUSED && exists("home/.cache/unisacc/chains-evil/f"), "dotdot-escape-refused");
    rc = del("home/.cache/unisacc/chains/c2/../../other/f", why);
    ok(rc == SAFEDEL_REFUSED && exists("home/.cache/unisacc/other/f"), "dotdot-escape-2-refused");
    rc = del("home/plain", why);
    ok(rc == SAFEDEL_REFUSED && exists("home/plain/f"), "home-plain-refused");
    rc = del("home", why);
    ok(rc == SAFEDEL_REFUSED && exists("home/plain/f"), "home-refused");

    /* symlink escape: a link inside chains pointing out */
    P(p, "outside"); P(q, "home/.cache/unisacc/chains/lnk"); symlink(p, q);
    rc = del("home/.cache/unisacc/chains/lnk/f", why);
    ok(rc == SAFEDEL_REFUSED && exists("outside/f"), "symlink-escape-file-refused");
    rc = del("home/.cache/unisacc/chains/lnk/", why);
    ok(rc == SAFEDEL_REFUSED && exists("outside/f"), "symlink-escape-dir-slash-refused");
    rc = del("home/.cache/unisacc/chains/lnk", why);
    ok(rc == SAFEDEL_OK && !exists("home/.cache/unisacc/chains/lnk") && exists("outside/f"), "symlink-itself-removed-target-intact");
    /* a link outside chains pointing into chains: the resolved target is a chains subdir, so it is deletable as a link only */
    P(p, "outside/lnk2"); P(q, "home/.cache/unisacc/chains/c2"); symlink(q, p);
    touch("home/.cache/unisacc/chains/c2/f");
    rc = del("outside/lnk2/f", why);
    ok(rc == SAFEDEL_OK && !exists("home/.cache/unisacc/chains/c2/f"), "link-into-chains-resolved-target-deletable");
    unlink(p);

    /* a symlinked chains root (HOME2/.cache/unisacc/chains -> BASE/real) is no whitelist root */
    mk("home2"); mk("home2/.cache"); mk("home2/.cache/unisacc"); mk("real"); mk("real/c9"); touch("real/c9/f");
    P(p, "real"); P(q, "home2/.cache/unisacc/chains"); symlink(p, q);
    P(p, "home2"); setenv("HOME", p, 1);
    rc = del("home2/.cache/unisacc/chains/c9", why);
    ok(rc == SAFEDEL_REFUSED && exists("real/c9/f"), "symlinked-chains-root-refused");
    /* a symlink in the middle (HOME3/.cache -> BASE/cache3) too */
    mk("home3"); mk("cache3"); mk("cache3/unisacc"); mk("cache3/unisacc/chains"); mk("cache3/unisacc/chains/c8"); touch("cache3/unisacc/chains/c8/f");
    P(p, "cache3"); P(q, "home3/.cache"); symlink(p, q);
    P(p, "home3"); setenv("HOME", p, 1);
    rc = del("home3/.cache/unisacc/chains/c8", why);
    ok(rc == SAFEDEL_REFUSED && exists("cache3/unisacc/chains/c8/f"), "symlinked-cache-middle-refused");

    /* no chains root at all (HOME4 without it) */
    mk("home4"); P(p, "home4"); setenv("HOME", p, 1);
    mk("home4/.cache"); mk("home4/.cache/unisacc");
    mk("home4/.cache/unisacc/z"); touch("home4/.cache/unisacc/z/f");
    rc = del("home4/.cache/unisacc/z", why);
    ok(rc == SAFEDEL_REFUSED && exists("home4/.cache/unisacc/z/f"), "no-chains-root-refused");
    mk("home4/.cache/unisacc/chains"); mk("home4/.cache/unisacc/chains/x"); touch("home4/.cache/unisacc/chains/x/f");
    P(p, "home4"); setenv("HOME", p, 1);
    rc = del("home4/.cache/unisacc/chains/x", why);
    ok(rc == SAFEDEL_OK && !exists("home4/.cache/unisacc/chains/x"), "chains-root-created-later-works");

    /* existing roots unchanged: TMPDIR subdir deletable, TMPDIR itself refused, HOME file refused */
    P(p, "home"); setenv("HOME", p, 1);
    rc = del("tmp/t1", why);
    ok(rc == SAFEDEL_OK && !exists("tmp/t1"), "tmpdir-subdir-still-deletable");
    rc = del("tmp", why);
    ok(rc == SAFEDEL_REFUSED && exists("tmp"), "tmpdir-itself-still-refused");
    rc = delabs("/", why);
    ok(rc == SAFEDEL_REFUSED, "slash-still-refused");
    rc = delabs("/usr", why);
    ok(rc == SAFEDEL_REFUSED, "usr-still-refused");

    printf("TOTAL\t%d\n", g_total);
    return 0;
}
