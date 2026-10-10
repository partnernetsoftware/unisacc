/*
 * llm.c — the missing step of PRD §5.3: "the model says something".
 *
 * LIBRARY, NO `main` (main is in llm_cli.c) — unisacc allows one `main` per
 * program, and combining two files that have one is `arm64: main:`, which reads
 * like a missing backend feature and is not (SKILL.md §2b).
 *
 * WHY THIS FILE EXISTS
 * --------------------
 * shell.c + net.c + gate.c were each green, and PRD §5.3 said the loop was one
 * step short: nothing turned an HTTP RESPONSE into a transcript RECORD that the
 * gate could then judge. That missing step is this file, and it is deliberately
 * the only thing it does:
 *
 *     user text ──> build request ──> net_http ──> parse body ──> model record
 *                                        │                            │
 *                                   (refuse)                    session_append
 *                                                                     │
 *                                                              gate_decide ──> verdict
 *
 * THE THREE THINGS THAT MUST NOT BE GLOSSED OVER
 * ----------------------------------------------
 * 1. A NETWORK FAILURE IS NOT A MODEL REPLY. If the socket refuses, or the
 *    status is 5xx, or the body is not JSON, this file returns ok=0 with the
 *    reason — it does NOT synthesize a low `p` and let the gate "decide". A
 *    gate standing on a fabricated probability is exactly the silent wrong
 *    answer gate.c was written to prevent (see gate.c on "no number = fail").
 *
 * 2. `p` MUST BE A NUMBER IN THE REPLY, AND ABSENT IS NOT ZERO. gate.c takes a
 *    probability; if the reply has no usable `p`, we refuse with
 *    LLM_NO_PROBABILITY rather than defaulting to 0.0. Defaulting would make
 *    "the model said stop" and "I could not read the model" identical to every
 *    caller, and they lead to different next steps.
 *
 * 3. THE PROMPT GOES IN A JSON BODY, SO IT MUST BE ESCAPED. The same boundary
 *    as chat.c's user text: a quote in the prompt must not produce a body that
 *    is not JSON. Escaping happens here, at the boundary, where it is cheap.
 *
 * WHAT THIS IS NOT
 * ----------------
 * Not a chat protocol. There is no message array, no system prompt, no
 * streaming, no retry, no token accounting. It is one POST and one parse,
 * because that is what "close the loop once" needs; a protocol belongs on top
 * of a step that is known to work.
 *
 * unisacc limits honoured (SKILL.md §2): no `return f()` of a struct from a
 * non-main function; stdio stays in the CLI. Types are restated, not shared, so
 * that this module compiles on its own: unisacc can see a type an EARLIER file
 * on the command line defined, but that is order-dependent.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── restated declarations (so this file compiles alone) ─────────────────── */

typedef struct {
    int  ok; int err; int status; long body_bytes; int chunked;
    char header[16384];
    char body[65536];
} net_response;

net_response net_http(const char *method, const char *url,
                      const char *content_type, const char *body);

typedef struct {
    double threshold;       /* continue when p >= threshold */
} gate_config;

typedef struct {
    int    cont;            /* 1 = continue, 0 = stop */
    double p;               /* the parsed probability */
    int    ok;              /* 0 when no number could be read at all */
    char   why[128];
} gate_decision;

gate_decision gate_decide(const char *reply, const gate_config *cfg);

double  jnum(jvalue *v, double dflt);

int session_append(const char *path, const char *record);

/* ── results ────────────────────────────────────────────────────────────── */

#define LLM_TEXT_MAX 4096

/* Ours, not the OS's — same pattern as net.c's NET_REFUSED_* and file.c's
 * FILE_TOO_BIG. Distinguishable from errno so a caller can tell "the network
 * broke" from "the reply was unusable". */
#define LLM_NO_PROBABILITY (-3001)   /* parsed, but no usable `p` field */
#define LLM_BAD_JSON       (-3002)   /* body is not a JSON object */
#define LLM_TEXT_TOO_LONG  (-3003)   /* model text exceeds LLM_TEXT_MAX */
#define LLM_NO_TEXT        (-3004)   /* object has no string field to record */
#define LLM_NO_URL         (-3005)   /* no endpoint URL was given */

typedef struct {
    int    ok;                   /* 1 = a model record was appended */
    int    err;                  /* ours when ok == 0 */
    int    status;               /* HTTP status, for the caller to print */
    int    appended;             /* 1 = the record really reached the file */
    double p;                    /* the probability the gate judged */
    int    decision;             /* 0 = stop, 1 = continue (valid only if ok) */
    char   text[LLM_TEXT_MAX];   /* the model's text, escaped-into-record form */
    char   reason[128];
} llm_turn;

/* ── escaping (the same boundary chat.c guards for user text) ───────────── */

/*
 * Escape `in` for a JSON string literal. Returns bytes that WOULD be written,
 * snprintf-style, so truncation is detectable rather than silent.
 */
static size_t llm_escape(const char *in, char *out, size_t outlen, size_t *in_used) {
    return json_escape(in, out, outlen, in_used);
}

/*
 * Pull the model's reply text and probability out of a response body.
 *
 * The accepted shapes, in order — the first one found wins, and this ORDER is
 * the contract:
 *   {"p":0.83,"text":"..."}          csih's own shape, what llm_server.py emits
 *   {"choices":[{"message":{"content":"..."}}],"p":0.83}  OpenAI-ish, p beside it
 * Alternates are tried only when the previous shape is absent, never merged:
 * a body carrying both must resolve to one record, not two.
 */
int llm_extract(const char *body, size_t len, llm_turn *t) {
    char errbuf[128];
    jvalue *root, *v;
    const char *text = NULL;
    int have_p = 0;

    root = json_parse(body, len, errbuf, sizeof errbuf);
    if (!root) { t->err = LLM_BAD_JSON; snprintf(t->reason, sizeof t->reason, "body is not JSON"); return -1; }
    if (root->kind != J_OBJ) { jfree(root); t->err = LLM_BAD_JSON; snprintf(t->reason, sizeof t->reason, "body is not a JSON object"); return -1; }

    /* p: must be a NUMBER present in the object. `jnum` needs a default, so the
     * presence test is done on the value's kind, not on the returned double —
     * otherwise 0.0 and "absent" would be the same thing. */
    v = jget(root, "p");
    if (v && v->kind == J_NUM) { t->p = v->n; have_p = 1; }

    v = jget(root, "text");
    if (v && v->kind == J_STR) text = jstr(v);

    if (!text) {
        v = jget(root, "content");
        if (v && v->kind == J_STR) text = jstr(v);
    }
    if (!text) {
        v = jget(root, "choices");
        if (v && v->kind == J_ARR && v->len > 0) {
            jvalue *c0 = v->items[0];
            jvalue *m = c0 && c0->kind == J_OBJ ? jget(c0, "message") : NULL;
            jvalue *ct = m && m->kind == J_OBJ ? jget(m, "content") : NULL;
            if (ct && ct->kind == J_STR) text = jstr(ct);
        }
    }

    if (!have_p) {
        jfree(root);
        t->err = LLM_NO_PROBABILITY;
        snprintf(t->reason, sizeof t->reason, "reply has no numeric p (absent is not zero)");
        return -1;
    }
    if (!text) {
        jfree(root);
        t->err = LLM_NO_TEXT;
        snprintf(t->reason, sizeof t->reason, "reply has no string text/content");
        return -1;
    }
    {
        size_t seen = 0;
        llm_escape(text, t->text, sizeof t->text, &seen);
        if (text[seen]) {
            jfree(root);
            t->err = LLM_TEXT_TOO_LONG;
            snprintf(t->reason, sizeof t->reason, "model text exceeds %d bytes", LLM_TEXT_MAX);
            return -1;
        }
    }
    jfree(root);
    return 0;
}

/*
 * One turn: POST the user text, read the model's `p`, let the gate judge, and
 * append the model record so the next turn can see it.
 *
 * Returns a value; the caller (CLI or another library) prints, which keeps
 * stdio out of this function's output path beyond snprintf.
 */
llm_turn llm_turn_run(const char *url, const char *transcript,
                      const char *user_text, gate_config cfg) {
    llm_turn t;
    char esc[2048];
    char body[2560];
    char rec[LLM_TEXT_MAX + 256];
    net_response r;
    gate_decision d;
    size_t blen;

    memset(&t, 0, sizeof t);
    t.p = 0.0;
    t.decision = 0;

    if (!url || !url[0]) { t.err = LLM_NO_URL; snprintf(t.reason, sizeof t.reason, "no endpoint URL given"); return t; }

    /* 1. the request body. The prompt is escaped HERE because this is the
     *    boundary where a stray quote would otherwise produce non-JSON. */
    llm_escape(user_text ? user_text : "", esc, sizeof esc, NULL);
    blen = (size_t)snprintf(body, sizeof body, "{\"prompt\":\"%s\"}", esc);
    if (blen >= sizeof body) { t.err = LLM_TEXT_TOO_LONG; snprintf(t.reason, sizeof t.reason, "prompt too long for request body"); return t; }

    /* 2. the request itself */
    r = net_http("POST", url, "application/json", body);
    if (!r.ok) {
        t.err = r.err;
        snprintf(t.reason, sizeof t.reason, "network: %s", r.err == -2003 ? "libcurl did not load" : "request failed");
        return t;
    }
    t.status = r.status;
    if (r.status < 200 || r.status >= 300) {
        t.err = r.status;
        snprintf(t.reason, sizeof t.reason, "HTTP %d is not a model reply", r.status);
        return t;
    }

    /* 3. the parse. A 200 whose body is not JSON is still a failure. */
    if (llm_extract(r.body, (size_t)(r.body_bytes > 0 ? r.body_bytes : 0), &t) != 0) return t;

    /* 4. the gate judges the NUMBER, not the prose. We already hold the number
     *    (t.p, read from the reply's `p` field); pass it to the SAME decision
     *    policy every caller uses, so "what counts as continue" stays in one
     *    place (gate.c). Re-parsing t.p from a formatted string also exercises
     *    the shared policy on both backends. */
    {
        char probbuf[32];
        snprintf(probbuf, sizeof probbuf, "%.4f", t.p);
        d = gate_decide(probbuf, &cfg);
    }
    if (!d.ok) { t.err = LLM_NO_PROBABILITY; snprintf(t.reason, sizeof t.reason, "gate could not read a probability"); return t; }
    t.decision = d.cont;

    /* 5. write back, so the next turn sees the model's line. Only after a
     *    successful judgment — a record that the gate could not judge would be
     *    a transcript entry with no meaning attached. */
    if (transcript && transcript[0]) {
        if ((size_t)snprintf(rec, sizeof rec, "{\"role\":\"model\",\"text\":\"%s\",\"p\":%.4f}", t.text, t.p) >= sizeof rec) {
            t.err = LLM_TEXT_TOO_LONG;
            snprintf(t.reason, sizeof t.reason, "record too long for the transcript line");
            return t;
        }
        if (session_append(transcript, rec) != 0) {
            t.err = LLM_NO_TEXT;
            snprintf(t.reason, sizeof t.reason, "cannot write transcript");
            return t;
        }
        t.appended = 1;
    }

    t.ok = 1;
    snprintf(t.reason, sizeof t.reason, "%s (p=%.2f)", d.cont ? "continue" : "stop", t.p);
    return t;
}

/* llm_cli.c owns the self-test. */
