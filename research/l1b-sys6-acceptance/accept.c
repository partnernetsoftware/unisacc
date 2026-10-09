/* L1b sys6 acceptance (run: unisacc.com -run research/l1b-sys6-acceptance/accept.c -- CC OUTDIR [RUN]).
 *
 * CC is the compiler under test (old reference, prototype, or the formal candidate).  Two probes:
 *   probe  sys6probe.c -- C argument evaluation into __syscall6 (every call is `.sys6 syscall,
 *          r0..r5` on the tape, so it is evidence for evaluation, not for operand permutation);
 *   tape   sys6tape.tmpl -- raw tape: dynamic syscall with permuted sources, r7 as a direct
 *          source, named mmap with permuted sources.  @NR_WRITE@/@MAP_ANON@ are filled per target.
 * For the four POSIX targets each probe gives an image; the Linux targets also give -S text (Mach-O
 * has no -S, osx is judged by image bytes).  The two Windows targets compile winfix.list, whose
 * images must stay byte-identical.  Output: "KIND TARGET sha256" lines.  A failed compile prints
 * FAIL and is never hashed: every output is removed first and written through tmp-NAME, renamed only
 * on success, so a stale file cannot pass.  RUN runs what this host can run (osx/arm64,
 * osx/x86_64 under Rosetta).  Every compile and run is bounded by tests/bound. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *posix[] = { "osx/arm64", "osx/x86_64", "lnx/arm64", "lnx/x86_64", 0 };
static char *win[] = { "win/x86_64", "win/arm64", 0 };
static char *D = "research/l1b-sys6-acceptance";
static char *CC, *OUT;
static int bad;

static void dash(char *o, char *t) { strcpy(o, t); o[strcspn(o, "/")] = '-'; }

static int sh(char *cmd) { int r = system(cmd); if (r) fprintf(stderr, "accept: rc %d: %s\n", r, cmd); return r; }

static void sha(char *kind, char *t, char *path) {
    char cmd[2048], h[128]; FILE *p;
    snprintf(cmd, sizeof cmd, "shasum -a 256 < '%s'", path);
    p = popen(cmd, "r"); h[0] = 0;
    if (!p || !fgets(h, sizeof h, p) || strlen(h) < 64) { printf("%s\t%s\tFAIL no-output\n", kind, t); bad = 1; }
    else { h[64] = 0; printf("%s\t%s\t%s\n", kind, t, h); }
    if (p) pclose(p);
}

/* compile SRC with FLAGS to OUT/NAME; hash it only if the compile succeeded */
static void build(char *kind, char *tag, char *src, char *flags, char *name) {
    char out[1024], tmp[1024], cmd[4096];   /* tmp keeps the extension: -S writes text only to *.s */
    snprintf(out, sizeof out, "%s/%s", OUT, name);
    snprintf(tmp, sizeof tmp, "%s/tmp-%s", OUT, name);
    remove(out);
    snprintf(cmd, sizeof cmd, "rm -f '%s' && tests/bound 30 '%s' '%s' %s -o '%s' && mv '%s' '%s'",
             tmp, CC, src, flags, tmp, tmp, out);
    if (sh(cmd)) { printf("%s\t%s\tFAIL compile\n", kind, tag); bad = 1; remove(out); return; }
    sha(kind, tag, out);
}

static int fill(char *tmpl, char *dst, char *nr, char *anon) {
    FILE *i = fopen(tmpl, "r"), *o = fopen(dst, "w"); char line[512], *p;
    if (!i || !o) return 1;
    while (fgets(line, sizeof line, i)) {
        if ((p = strstr(line, "@NR_WRITE@"))) { *p = 0; fprintf(o, "%s%s%s", line, nr, p + 10); }
        else if ((p = strstr(line, "@MAP_ANON@"))) { *p = 0; fprintf(o, "%s%s%s", line, anon, p + 10); }
        else fputs(line, o);
    }
    fclose(i); return fclose(o) != 0;
}

static void run(char *t, char *name, char *expect) {
    char cmd[4096], n[64];
    dash(n, t);
    snprintf(cmd, sizeof cmd, "f='%s/%s'; [ -f \"$f\" ] && rm -f \"$f.out\" && tests/bound 10 \"$f\" > \"$f.out\" && cmp -s \"$f.out\" '%s/%s'",
             OUT, name, D, expect);
    if (sh(cmd)) { printf("run\t%s:%s\tFAIL\n", t, name); bad = 1; } else printf("run\t%s:%s\tok\n", t, name);
}

int main(int argc, char **argv) {
    char n[64], name[256], tape[1024], tag[600], line[512]; int i; FILE *f;
    if (argc < 3) { fprintf(stderr, "usage: accept.c CC OUTDIR [RUN]\n"); return 2; }
    CC = argv[1]; OUT = argv[2];
    for (i = 0; posix[i]; i++) {
        int osx = posix[i][0] == 'o', x86 = strstr(posix[i], "x86") != 0;
        char *nr = osx ? (x86 ? "33554436" : "4") : (x86 ? "1" : "64");
        char *anon = osx ? "4098" : "34", flags[128];
        dash(n, posix[i]);
        snprintf(flags, sizeof flags, "-b %s", posix[i]);
        snprintf(name, sizeof name, "probe-%s.img", n);
        build("image", posix[i], "research/l1b-sys6-acceptance/sys6probe.c", flags, name);
        snprintf(tape, sizeof tape, "%s/tape-%s.tape", OUT, n);
        if (fill("research/l1b-sys6-acceptance/sys6tape.tmpl", tape, nr, anon)) { printf("tape\t%s\tFAIL fill\n", posix[i]); bad = 1; continue; }
        snprintf(name, sizeof name, "tape-%s.img", n);
        build("tapeimage", posix[i], tape, flags, name);
        if (!osx) {
            snprintf(flags, sizeof flags, "-S -b %s", posix[i]);
            snprintf(name, sizeof name, "probe-%s.s", n);
            build("lower", posix[i], "research/l1b-sys6-acceptance/sys6probe.c", flags, name);
            snprintf(name, sizeof name, "tape-%s.s", n);
            build("tapelower", posix[i], tape, flags, name);
        }
    }
    f = fopen("research/l1b-sys6-acceptance/winfix.list", "r");
    if (!f) { fprintf(stderr, "accept: no winfix.list\n"); return 2; }
    while (fgets(line, sizeof line, f)) {
        line[strcspn(line, "\r\n")] = 0; if (!line[0] || line[0] == '#') continue;
        for (i = 0; win[i]; i++) {
            char b[256], flags[64], *s = strrchr(line, '/');
            dash(n, win[i]); strcpy(b, s ? s + 1 : line); b[strcspn(b, ".")] = 0;
            snprintf(flags, sizeof flags, "-b %s", win[i]);
            snprintf(name, sizeof name, "win-%s-%s.exe", b, n);
            snprintf(tag, sizeof tag, "%s:%s", win[i], line);
            build("winimage", tag, line, flags, name);
        }
    }
    fclose(f);
    if (argc > 3 && !strcmp(argv[3], "RUN")) {
        run("osx/arm64", "probe-osx-arm64.img", "expect.txt");
        run("osx/x86_64", "probe-osx-x86_64.img", "expect.txt");
        run("osx/arm64", "tape-osx-arm64.img", "expect-tape.txt");
        run("osx/x86_64", "tape-osx-x86_64.img", "expect-tape.txt");
    }
    printf("accept\t%s\n", bad ? "BAD" : "ok");
    return bad;
}
