/* seed/unisa_linalg.c -- C99 reference for unisa.linalg (K6-5), point-sample primitives.
 *
 *   unisa-linalg OPS
 *       OPS is a text file; each non-comment line is one op with decimal float tokens:
 *         gemv NIN NOUT  x[NIN]  W[NIN*NOUT]  b[NOUT]    -> y, one line "%.17g" per element
 *         relu N  v[N]                                    -> relu(v), one line "%.17g" per element
 *         argmax N  v[N]                                  -> index, one line
 *         margin N  v[N]                                  -> top1 - top2, one line "%.17g"
 *       Mirrors unisa/linalg.py: gemv accumulates i ascending (no reordering, no FMA); relu
 *       maps a <= 0 to 0.0; argmax ties go to the lowest index; argmax of an empty vector is
 *       refused (python: IndexError).  A short token list is refused by name.  Refusal stops
 *       the interpreter at that op (exit 1); nothing is printed for the refused op.
 *
 * Point-sample equivalence over the listed op scripts only; not a product closure member; keeps
 * unisa/linalg.py.  Build with -ffp-contract=off so no multiply-add is fused.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXTOK 8192

static void die(const char *why, const char *what) {
    fprintf(stderr, "unisa-linalg: %s %s\n", why, what ? what : "");
    exit(1);
}

static int split(char *line, char **tok, int max) {
    int n = 0; char *p = line;
    while (*p) {
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
        if (!*p) break;
        if (n >= max) die("too many tokens", NULL);
        tok[n++] = p;
        while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') p++;
        if (*p) *p++ = 0;
    }
    return n;
}

static long long to_int(const char *s, const char *what) {
    char *end = NULL; long long v;
    errno = 0; v = strtoll(s, &end, 10);
    if (errno || end == s || *end) die("not an integer", what);
    return v;
}

static double to_float(const char *s, const char *what) {
    char *end = NULL; double v;
    errno = 0; v = strtod(s, &end);
    if (end == s || *end) die("not a float", what);
    (void)errno;
    return v;
}

static double *vec(char **tok, int at, long long n, const char *what, int ntok) {
    long long i; double *v;
    if (n < 0) die("negative length", what);
    if ((long long)at + n > ntok) die("short token list", what);
    v = malloc((size_t)(n ? n : 1) * sizeof *v); if (!v) die("out of memory", NULL);
    for (i = 0; i < n; i++) v[i] = to_float(tok[at + i], what);
    return v;
}

int main(int argc, char **argv) {
    FILE *f; char line[1 << 16];
    static char *tok[MAXTOK];
    if (argc != 2) { fprintf(stderr, "usage: unisa-linalg OPS\n"); return 2; }
    f = fopen(argv[1], "r"); if (!f) die("cannot open", argv[1]);
    while (fgets(line, sizeof line, f)) {
        int nt; size_t lines = strlen(line);
        if (lines && line[lines-1] == '\n') line[--lines] = 0;
        if (!lines || line[0] == '#') continue;
        nt = split(line, tok, MAXTOK);
        if (nt == 0) continue;
        if (!strcmp(tok[0], "gemv")) {
            long long nin, nout, i, j; double *x, *W, *b, *y; int at;
            if (nt < 3) die("gemv needs NIN NOUT", line);
            nin = to_int(tok[1], "NIN"); nout = to_int(tok[2], "NOUT");
            if (nin < 0 || nout < 0) die("negative dimension", line);
            at = 3;
            x = vec(tok, at, nin, "x", nt); at += (int)nin;
            W = vec(tok, at, nin * nout, "W", nt); at += (int)(nin * nout);
            b = vec(tok, at, nout, "b", nt);
            y = malloc((size_t)(nout ? nout : 1) * sizeof *y); if (!y) die("out of memory", NULL);
            for (j = 0; j < nout; j++) y[j] = b[j];
            for (i = 0; i < nin; i++) {
                double xi = x[i]; long long base = i * nout;
                for (j = 0; j < nout; j++) { double p = xi * W[base + j]; y[j] = y[j] + p; }
            }
            for (j = 0; j < nout; j++) printf("%.17g\n", y[j]);
            free(x); free(W); free(b); free(y);
        }
        else if (!strcmp(tok[0], "relu")) {
            long long n, i; double *v; if (nt < 2) die("relu needs N", line);
            n = to_int(tok[1], "N"); v = vec(tok, 2, n, "v", nt);
            for (i = 0; i < n; i++) printf("%.17g\n", v[i] > 0.0 ? v[i] : 0.0);
            free(v);
        }
        else if (!strcmp(tok[0], "argmax")) {
            long long n, i, bi; double bv, *v; if (nt < 2) die("argmax needs N", line);
            n = to_int(tok[1], "N");
            if (n <= 0) die("argmax of an empty vector (python: IndexError)", line);
            v = vec(tok, 2, n, "v", nt);
            bi = 0; bv = v[0];
            for (i = 1; i < n; i++) if (v[i] > bv) { bv = v[i]; bi = i; }
            printf("%lld\n", bi); free(v);
        }
        else if (!strcmp(tok[0], "margin")) {
            long long n, i; double a, b, x, *v; if (nt < 2) die("margin needs N", line);
            n = to_int(tok[1], "N"); v = vec(tok, 2, n, "v", nt);
            a = b = -1e308;
            for (i = 0; i < n; i++) {
                x = v[i];
                if (x > a) { b = a; a = x; }
                else if (x > b) b = x;
            }
            printf("%.17g\n", a - b); free(v);
        }
        else die("unknown op", tok[0]);
    }
    fclose(f);
    return 0;
}
