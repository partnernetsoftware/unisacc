/*
 * chat.c — one turn of conversation, as a function the TUI can call.
 *
 * WHAT THIS CLOSES: until now csih had a working gate (gate.c), a working
 * transcript (session.c), a working loop that proved they compose (loop.c), and
 * a working TUI (tui.c) — but the TUI's Enter key did nothing. The pieces were
 * all green and the program still could not hold a conversation. This file is
 * the joint: it takes what the user typed, appends it, runs the gate over what
 * is now the newest probability in the transcript, and appends the verdict.
 *
 * WHY A SEPARATE FILE FROM loop.c: loop.c owns a `main` (it is the runnable
 * closed-loop demo). Including it would drag a second `main` into any program
 * that wants a chat turn — the exact failure that cost this project three
 * builds already, measured twice: gcc says `duplicate symbol '_main'`, unisacc
 * says `arm64: main:` and names nothing. So the turn logic lives here, with no
 * `main`, and loop.c keeps its demo. Both call the same three libraries; only
 * one of them can be linked into the TUI.
 *
 * WHAT IT IS NOT: a model client. There is no network here and no subprocess
 * (unisacc's fork family is still missing — see probes/f22.sh). The gate is fed
 * whatever probability the transcript already holds, which is what makes this
 * file testable TODAY and still correct when a real model is attached: the
 * model's job is to append a `p` record, and that is the only thing this file
 * reads.
 *
 * unisacc limits honoured here (SKILL.md §2, and each one was measured):
 *   - jvalue and the json prototypes come from json.h (-include in csih.sh);
 *     nothing from json is restated here
 *   - `return f()` where f returns a struct is rejected from a non-main
 *     function: assign to a local first
 *   - the entry points here take `const char *` and stdio stays local, which
 *     keeps this library a pure string→decision unit
 *
 * CLI takes SUBCOMMANDS, not dash-options.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct {
    char **lines;     /* owned */
    size_t count;
    size_t bad;
} session_records;

int session_append(const char *path, const char *record);
session_records session_read(const char *path);
void session_free(session_records *r);

typedef struct { double threshold; } gate_config;
typedef struct {
    int    cont;
    double p;
    int    ok;
    char   why[128];
} gate_decision;

gate_decision gate_decide(const char *reply, const gate_config *cfg);

/* clock.c: real monotonic time, for transcript timestamps. unisacc 0.0.20
 * (R20-6) provides struct timespec + clock_gettime; before that this symbol
 * was undefined and these records could not carry a time at all. We do NOT
 * fall back to a fake clock — a transcript without timing is still honest,
 * it just omits the field, whereas a made-up number would be a lie. */
long clock_now_ms(void);

/* ── the turn ───────────────────────────────────────────────────────────── */

typedef struct {
    int    ok;            /* the turn completed at all */
    int    cont;          /* the gate's verdict */
    double p;             /* the probability the gate saw */
    char   verdict[192];  /* what to show the user */
} chat_turn;

/*
 * Escape a string for embedding in a JSON string literal.
 *
 * Needed because the user's text goes straight into the transcript, and a
 * transcript is JSON Lines. Without this, typing a double quote would write a
 * line that does not parse — and the failure would appear LATER, as a corrupt
 * transcript, not at the moment of typing. Escaping at the boundary is the only
 * place the mistake is cheap to find.
 *
 * Returns the number of bytes that WOULD be written (like snprintf), so callers
 * can detect truncation instead of silently losing the tail of a line.
 */
static size_t chat_escape(const char *in, char *out, size_t outlen) {
    return json_escape(in, out, outlen, NULL);
}

/*
 * Find the newest `"p": <number>` in the transcript and hand it to the gate as
 * TEXT, not as a double.
 *
 * Text on purpose: gate.c reads "the last number in the string", so routing the
 * stored value back through the same parser means the gate and the transcript
 * can never disagree about what a probability means. Re-deriving from the
 * parsed double would be a second interpretation — and a chance for the two to
 * drift apart without either being obviously wrong.
 *
 * The LAST record wins because the transcript grows downward: the newest
 * statement of p is the tail. A missing p anywhere is reported as such rather
 * than defaulted to 0, because "no probability was ever stated" and "the
 * probability was zero" lead to opposite decisions.
 */
static int chat_last_p(const char *path, char *out, size_t outlen, double *pout) {
    session_records recs = session_read(path);
    int found = 0;
    size_t i;
    if (recs.lines == NULL && recs.count == 0) {
        /* An empty transcript is normal on the first turn, not an error. */
        session_free(&recs);
        return 0;
    }
    for (i = recs.count; i > 0; i--) {
        const char *line = recs.lines[i - 1];
        jvalue *v;
        char err[128];
        err[0] = '\0';
        v = json_parse(line, strlen(line), err, sizeof err);
        if (v) {
            jvalue *pv = jget(v, "p");
            if (pv && pv->kind == J_NUM) {
                double d = jnum(pv, 0.0);
                snprintf(out, outlen, "%.6f", d);
                if (pout) *pout = d;
                found = 1;
                jfree(v);
                break;
            }
            jfree(v);
        }
    }
    session_free(&recs);
    return found;
}

/*
 * Run one turn: record what the user said, let the gate judge the newest
 * probability in the transcript, and record the verdict.
 *
 * The verdict is written INTO the transcript, not just returned — that is what
 * makes the next turn able to see it, and it is the same property check.sh
 * asserts for loop.c ("the verdict is IN the transcript (loop closed, not just
 * printed)"). A verdict that only reaches the screen is not a closed loop.
 */
chat_turn chat_turn_run(const char *path, const char *user_text, double threshold) {
    chat_turn t;
    char esc[2048];
    char rec[2304];
    char ptext[64];
    double p = 0.0;
    gate_config cfg;
    gate_decision d;

    memset(&t, 0, sizeof t);
    t.ok = 0;
    cfg.threshold = threshold;

    /* 1. record the user's line (escaped, so a quote cannot corrupt the log) */
    chat_escape(user_text ? user_text : "", esc, sizeof esc);
    {
        long ts = clock_now_ms();   /* -1 == clock unavailable (honest sentinel) */
        snprintf(rec, sizeof rec, "{\"role\":\"user\",\"text\":\"%s\",\"ts\":%ld}",
                 esc, ts);
    }
    if (session_append(path, rec) != 0) {
        snprintf(t.verdict, sizeof t.verdict, "cannot write transcript");
        return t;
    }

    /* 2. judge the newest stated probability */
    if (!chat_last_p(path, ptext, sizeof ptext, &p)) {
        snprintf(t.verdict, sizeof t.verdict, "no probability stated yet");
        t.ok = 1;                  /* the turn is fine; there is just nothing to judge */
        return t;
    }
    d = gate_decide(ptext, &cfg);

    /* 3. write the verdict back, so the transcript carries the whole loop */
    {
        char wesc[256];
        long ts = clock_now_ms();
        chat_escape(d.why, wesc, sizeof wesc);
        snprintf(rec, sizeof rec,
                 "{\"role\":\"gate\",\"verdict\":\"%s\",\"p\":%.6f,\"ts\":%ld}",
                 d.cont ? "CONTINUE" : "STOP", d.p, ts);
        (void)wesc;
        session_append(path, rec);
    }

    t.ok = 1;
    t.cont = d.cont;
    t.p = d.p;
    snprintf(t.verdict, sizeof t.verdict, "%s (p=%.2f)",
             d.cont ? "CONTINUE" : "STOP", d.p);
    return t;
}

/*
 * Append a probability, as a model would.
 *
 * This exists so the TUI and the tests can produce the one thing gate.c needs
 * without a network client. When a real model is attached it will append the
 * same shape — that is the entire contract between csih and its model.
 */
int chat_note_p(const char *path, double p) {
    char rec[160];
    long ts = clock_now_ms();
    snprintf(rec, sizeof rec, "{\"role\":\"model\",\"p\":%.6f,\"ts\":%ld}", p, ts);
    return session_append(path, rec);
}

/* chat_cli.c owns the self-test. */
