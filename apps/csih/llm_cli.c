/*
 * llm_cli.c — the `main` for llm.c, moved out so llm.c can be a library (with
 * every other module, per unisacc's one-main-per-program rule).
 *
 * WHY THIS FILE EXISTS (2026-10-02): llm.c is the PRD §5.3 step — "the model
 * says something" — and it is a LIBRARY with no `main`. This file is the driver:
 * it owns the stdio and turns a turn result into a printed verdict + exit code.
 *
 * THIS FILE OWNS the stdio, the same reason as gate_cli.c: it keeps the library
 * a pure string→turn function so the printing lives here. An earlier note blamed
 * a cross-file `FILE *` prototype for mis-lowering `%.4f`; unisacc 0.0.23 does
 * not do that (measured 2026-10-04), so the claim is gone.
 *
 * Every struct this CLI uses is restated below and MUST match llm.c
 * byte-for-byte, because llm_turn_run returns an llm_turn across the file
 * boundary and this file reads its fields. Restating is what lets this file
 * compile on its own; unisacc can see an earlier file's type, but that is
 * order-dependent.
 *
 * CLI takes SUBCOMMANDS, not dash-options (unisacc reserves the dashes):
 *     llm_cli selftest
 *     llm_cli run <url> <transcript> <user_text> [threshold]
 *
 * BUILD (either backend; the two must agree):
 *     unisacc llm.c llm_cli.c selftest
 *     cc -std=c99 llm.c llm_cli.c -o /tmp/llm && /tmp/llm selftest
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── the slice of llm.c this CLI uses (types restated so this file compiles
 *    on its own; see header note) ─────────────────────────────────────────── */
typedef struct { double threshold; } gate_config;

typedef struct {
    int    ok;                   /* 1 = a model record was appended */
    int    err;                  /* ours when ok == 0 */
    int    status;               /* HTTP status, for the caller to print */
    int    appended;             /* 1 = the record really reached the file */
    double p;                    /* the probability the gate judged */
    int    decision;             /* 0 = stop, 1 = continue (valid only if ok) */
    char   text[4096];           /* the model's text */
    char   reason[128];
} llm_turn;

llm_turn      llm_turn_run(const char *url, const char *transcript,
                           const char *user_text, gate_config cfg);
int           llm_run_selftest(void);

int main(int argc, char **argv)
{
    const char *cmd = argc > 1 ? argv[1] : "";
    if (!strcmp(cmd, "selftest")) return llm_run_selftest();

    if (!strcmp(cmd, "run")) {
        if (argc < 5) {
            printf("usage: llm_cli run <url> <transcript> <user_text> [threshold]\n");
            return 64;
        }
        gate_config cfg;
        cfg.threshold = argc > 5 ? atof(argv[5]) : 0.5;
        llm_turn t = llm_turn_run(argv[2], argv[3], argv[4], cfg);
        if (t.ok) {
            printf("ok decision=%s p=%.4f status=%d appended=%d\n",
                   t.decision ? "continue" : "stop", t.p, t.status, t.appended);
            return t.decision ? 0 : 1;
        }
        printf("failed (err %d): %s\n", t.err, t.reason);
        /* A network failure or an unusable reply is NOT a "stop" verdict — the
         * caller must be able to tell them apart, so the exit code is its own
         * band (2) rather than the 0/1 continue/stop of a real decision. */
        return 2;
    }

    printf("usage: llm_cli selftest | run <url> <transcript> <user_text> [threshold]\n");
    return 64;
}
