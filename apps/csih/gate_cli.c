/*
 * gate_cli.c — the `main` for gate.c, moved out so gate.c can be a library.
 *
 * WHY THIS FILE EXISTS (2026-10-01): unisacc runs each source as a program, and
 * a program has exactly one `main`. Combining gate.c + json.c + session.c +
 * loop.c is the whole point of loop.c, and two mains make that fail with
 *     reject: not covered: ARM64 operand or instruction
 *     arm64: main:
 * which reads like a missing backend feature. So each module drops `main` and
 * gets a CLI file; gate.c's logic moved to non-static entry points
 * (gate_decide / gate_run_selftest).
 *
 * THIS FILE OWNS the stdio: it reads stdin and prints the verdict, so gate.c
 * stays a pure string→decision library with no `FILE *` in its signature. That
 * split is a design choice, not a workaround. An earlier note here blamed a
 * cross-file `FILE *` prototype for mis-lowering `%.4f`; unisacc 0.0.23 does not
 * do that (measured 2026-10-04: the same shape builds for all six targets), so
 * the claim is gone and only the split remains.
 *
 * The CLI takes SUBCOMMANDS, not dash-options (unisacc reserves the dashes).
 * Every struct this CLI uses is restated below, which is what lets it compile on
 * its own; unisacc can see an earlier file's type, but that is order-dependent.
 *
 * BUILD (either backend; the two must agree):
 *     unisacc gate.c gate_cli.c selftest
 *     cc -std=c99 gate.c gate_cli.c -o /tmp/gate && /tmp/gate selftest
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── the slice of gate.c this CLI uses (types restated so this file compiles
 *    on its own; see header note) ─────────────────────────────────────────── */
typedef struct { double threshold; } gate_config;
typedef struct {
    int    cont;
    double p;
    int    ok;
    char   why[128];
} gate_decision;

gate_decision gate_decide(const char *reply, const gate_config *cfg);
int gate_run_selftest(const gate_config *cfg);

#define GATE_CLI_MAX_TEXT 65536

int main(int argc, char **argv)
{
    gate_config cfg;
    static char text[GATE_CLI_MAX_TEXT];
    size_t len;
    gate_decision d;
    int i;

    cfg.threshold = 0.5;
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "threshold") == 0 && i + 1 < argc) {
            cfg.threshold = strtod(argv[++i], NULL);
        } else if (strcmp(argv[i], "selftest") == 0) {
            return gate_run_selftest(&cfg);
        }
    }

    len = fread(text, 1, sizeof(text) - 1, stdin);
    text[len] = '\0';
    d = gate_decide(text, &cfg);
    if (!d.ok) { printf("STOP reason=\"%s\"\n", d.why); return 2; }
    printf("%s p=%.4f reason=\"%s\"\n", d.cont ? "CONTINUE" : "STOP", d.p, d.why);
    return d.cont ? 0 : 1;
}

int gate_run_selftest(const gate_config *cfg)
{
    struct { const char *in; int ok; double p; int cont; } cases[] = {
        { "0.83",            1, 0.83, 1 },
        { "0",               1, 0.00, 0 },
        { "The probability is 0.9", 1, 0.90, 1 },
        { "0.2\nActually 0.7",      1, 0.70, 1 },
        { "80%",             1, 0.80, 1 },
        { "1.4",             1, 1.00, 1 },
        { "-0.2",            1, 0.00, 0 },
        { "no number here",  0, 0.00, 0 },
        { "",                0, 0.00, 0 },
    };
    size_t n = sizeof(cases) / sizeof(cases[0]), k, bad = 0;
    for (k = 0; k < n; k++) {
        gate_decision d = gate_decide(cases[k].in, cfg);
        int ok = (d.ok == cases[k].ok) &&
                 (!d.ok || (d.p > cases[k].p - 0.001 && d.p < cases[k].p + 0.001)) &&
                 (d.cont == cases[k].cont);
        if (!ok) {
            bad++;
            printf("FAIL [%s] got ok=%d p=%.3f cont=%d\n", cases[k].in, d.ok, d.p, d.cont);
        }
    }
    printf("%lu/%lu self-test cases pass\n",
           (unsigned long)(n - bad), (unsigned long)n);
    return bad == 0 ? 0 : 1;
}
