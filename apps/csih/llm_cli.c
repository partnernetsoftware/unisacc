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

#define LLM_NO_PROBABILITY (-3001)
#define LLM_BAD_JSON       (-3002)
int llm_extract(const char *body, size_t len, llm_turn *t);
int session_append(const char *path, const char *record);

/* ── self-test (callable from llm_cli.c) ────────────────────────────────── */

static int llm_failures = 0;
static void llm_expect(int cond, const char *what) {
    if (!cond) { printf("  FAIL %s\n", what); llm_failures++; }
    else printf("  ok   %s\n", what);
}

int llm_run_selftest(void) {
    llm_turn t;
    gate_config cfg;

    llm_failures = 0;
    cfg.threshold = 0.7;

    /* The extractor is the part that can be tested WITHOUT a network, and it is
     * the part where a silent wrong answer is most likely. */
    /* All extractor cases pass the TRUE length (strlen), not a hand-counted
     * constant: a too-short len silently truncates the body and turns a valid
     * reply into "not JSON" — exactly the self-inflicted failure that hides a
     * real one. This was the cause of 6 false selftest failures. */
#define LLM_X(body) llm_extract((body), strlen(body), &t)

    memset(&t, 0, sizeof t);
    llm_expect(LLM_X("{\"p\":0.83,\"text\":\"go on\"}") == 0, "csih shape parses");
    llm_expect(t.p > 0.82 && t.p < 0.84, "p is read as a number");
    llm_expect(!strcmp(t.text, "go on"), "text is read");

    memset(&t, 0, sizeof t);
    llm_expect(LLM_X("{\"p\":0.2,\"text\":\"halt\"}") == 0, "low p parses too");

    /* absent p is NOT zero — this is the whole reason the check exists */
    memset(&t, 0, sizeof t);
    llm_expect(LLM_X("{\"text\":\"no p here\"}") != 0, "a reply without p is REFUSED");
    llm_expect(t.err == LLM_NO_PROBABILITY, "  and refused for the right reason (not a 0.0 default)");

    memset(&t, 0, sizeof t);
    llm_expect(LLM_X("{\"p\":\"high\",\"text\":\"x\"}") != 0, "a non-numeric p is REFUSED");

    memset(&t, 0, sizeof t);
    llm_expect(LLM_X("not json at all") != 0, "a non-JSON body is REFUSED");
    llm_expect(t.err == LLM_BAD_JSON, "  and named as bad JSON");

    memset(&t, 0, sizeof t);
    llm_expect(LLM_X("{\"p\":0.5}") != 0, "a reply with no text is REFUSED");

    /* OpenAI-ish shape, p beside choices */
    memset(&t, 0, sizeof t);
    llm_expect(LLM_X("{\"p\":0.9,\"choices\":[{\"message\":{\"content\":\"hi\"}}]}") == 0, "choices/message/content parses");
    llm_expect(!strcmp(t.text, "hi"), "  and its content is read");

    /* escaping: a quote in the model's text must not break the record */
    memset(&t, 0, sizeof t);
    llm_expect(LLM_X("{\"p\":0.5,\"text\":\"say \\\"hi\\\"\"}") == 0, "escaped quotes in reply parse");
    llm_expect(strstr(t.text, "\\\"") != NULL, "  and are re-escaped for the record");

#undef LLM_X

    /* the network path: an unroutable/down port must be a failure, not a verdict */
    memset(&t, 0, sizeof t);
    t = llm_turn_run("http://127.0.0.1:1/", NULL, "hello", cfg);
    llm_expect(!t.ok, "a dead port is a FAILURE, not a stop verdict");

    memset(&t, 0, sizeof t);
    t = llm_turn_run("https://127.0.0.1:1/", NULL, "hello", cfg);
    llm_expect(!t.ok, "https to a closed port is a FAILURE, not a verdict");

    /* transcript append path, without a server: exercise session_append via a
     * real file so the write contract is proven even with no endpoint up.
     * session_append creates the file (fopen "a"), and `remove` is available on
     * both backends (session.c's own selftest uses it) — so no mkstemp, which
     * unisacc does NOT provide. */
    {
        const char *tp = "/tmp/cdsh-llm-selftest.jsonl";
        remove(tp);
        llm_expect(session_append(tp, "{\"role\":\"model\",\"text\":\"x\",\"p\":0.5000}") == 0, "model record appends to a real transcript");
        remove(tp);
    }

    if (llm_failures) { printf("llm: %d FAILED\n", llm_failures); return 1; }
    printf("llm: all cases pass\n");
    return 0;
}
