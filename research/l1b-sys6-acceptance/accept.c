/* L1b sys6 acceptance (run: unisacc.com -run research/l1b-sys6-acceptance/accept.c -- CC OUTDIR [RUN]).
 *
 * CC is the compiler under test (old reference, prototype, or the formal candidate).  For the four
 * POSIX targets it writes the image of sys6probe.c; for the two Linux targets also the lowered
 * text (-S; Mach-O has no -S, so osx is judged by image bytes).  For the two Windows targets it
 * writes images of winfix.list sources, which must stay byte-identical (sys6 on Windows is not in
 * this slice).  Prints "KIND TARGET sha256" lines, sorted, ready to diff against expect-*.tsv.
 * With RUN=1 it also runs the native image(s) this host can run and checks expect.txt.
 * Every compile and run is bounded by tests/bound. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *posix[] = { "osx/arm64", "osx/x86_64", "lnx/arm64", "lnx/x86_64", 0 };
static char *win[] = { "win/x86_64", "win/arm64", 0 };
static char *D = "research/l1b-sys6-acceptance";
static int bad;

static void dash(char *o, char *t) { strcpy(o, t); o[strcspn(o, "/")] = '-'; }

static void sha(char *kind, char *t, char *path) {
    char cmd[2048], h[128]; FILE *p;
    snprintf(cmd, sizeof cmd, "shasum -a 256 < '%s' 2>/dev/null", path);
    p = popen(cmd, "r"); h[0] = 0;
    if (!p || !fgets(h, sizeof h, p) || strlen(h) < 64) { printf("%s\t%s\tMISSING\n", kind, t); bad = 1; }
    else { h[64] = 0; printf("%s\t%s\t%s\n", kind, t, h); }
    if (p) pclose(p);
}

static int sh(char *cmd) { int r = system(cmd); if (r) fprintf(stderr, "accept: rc %d: %s\n", r, cmd); return r; }

int main(int argc, char **argv) {
    char cmd[4096], n[64], out[1024], src[512], line[512]; int i; FILE *f;
    if (argc < 3) { fprintf(stderr, "usage: accept.c CC OUTDIR [RUN]\n"); return 2; }
    for (i = 0; posix[i]; i++) {
        dash(n, posix[i]);
        snprintf(out, sizeof out, "%s/probe-%s.img", argv[2], n);
        snprintf(cmd, sizeof cmd, "tests/bound 30 '%s' %s/sys6probe.c -b %s -o '%s'", argv[1], D, posix[i], out);
        sh(cmd); sha("image", posix[i], out);
        if (posix[i][0] == 'l') {
            snprintf(out, sizeof out, "%s/probe-%s.s", argv[2], n);
            snprintf(cmd, sizeof cmd, "tests/bound 30 '%s' %s/sys6probe.c -S -b %s -o '%s'", argv[1], D, posix[i], out);
            sh(cmd); sha("lower", posix[i], out);
        }
    }
    snprintf(src, sizeof src, "%s/winfix.list", D);
    f = fopen(src, "r"); if (!f) { fprintf(stderr, "accept: no %s\n", src); return 2; }
    while (fgets(line, sizeof line, f)) {
        line[strcspn(line, "\r\n")] = 0; if (!line[0] || line[0] == '#') continue;
        for (i = 0; win[i]; i++) {
            char tag[600], b[256], *s = strrchr(line, '/');
            dash(n, win[i]); strcpy(b, s ? s + 1 : line); b[strcspn(b, ".")] = 0;
            snprintf(out, sizeof out, "%s/win-%s-%s.exe", argv[2], b, n);
            snprintf(cmd, sizeof cmd, "tests/bound 30 '%s' '%s' -b %s -o '%s'", argv[1], line, win[i], out);
            sh(cmd); snprintf(tag, sizeof tag, "%s:%s", win[i], line); sha("winimage", tag, out);
        }
    }
    fclose(f);
    if (argc > 3 && !strcmp(argv[3], "RUN")) {
        char *host[] = { "osx/arm64", "osx/x86_64", 0 };
        for (i = 0; host[i]; i++) {
            dash(n, host[i]);
            snprintf(cmd, sizeof cmd, "chmod +x '%s/probe-%s.img' && tests/bound 10 '%s/probe-%s.img' > '%s/run-%s.txt' && cmp -s '%s/run-%s.txt' %s/expect.txt",
                     argv[2], n, argv[2], n, argv[2], n, argv[2], n, D);
            if (sh(cmd)) { printf("run\t%s\tFAIL\n", host[i]); bad = 1; } else printf("run\t%s\tok\n", host[i]);
        }
    }
    return bad;
}
