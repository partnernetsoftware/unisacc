/*
 * reload_session_encode.c — encode reload_session_state into the strict v2
 * JSON blob. LIBRARY, NO main.
 *
 * WHY: the old process ships one JSON blob to the new process; this file
 * owns exactly one question: build that v2 text and its byte length. It
 * reuses the real json_escape() for string bodies and the real
 * reload_state_validate_v2() to refuse anything it cannot decode back.
 */
#include "reload_session.h"
#include <string.h>
#include <stdio.h>

/* Real helpers from earlier files in this unit. Declarations verbatim. */
size_t json_escape(const char *in, char *out, size_t outlen, size_t *in_used);
int reload_state_validate_v2(const char *text, size_t len,
                             char *why, size_t cap);

/* Small accumulating builder. failed latches on first overflow; buf[0]
 * stays NUL when the build is invalid so callers can zero on failure. */
typedef struct { char *buf; size_t cap; size_t n; int failed; } sb_t;

static void sb_append(sb_t *b, const char *s) {
    size_t len;
    if (b->failed) return;
    if (!s) s = "";
    len = strlen(s);
    if (b->n >= b->cap || len >= b->cap - b->n) { b->failed = 1; return; }
    memcpy(b->buf + b->n, s, len);
    b->n += len;
    b->buf[b->n] = '\0';
}

static void sb_why(char *why, size_t cap, const char *msg) {
    if (why && cap) snprintf(why, cap, "%s", msg);
}

/* Copied verbatim from json.c utf8_one: one valid UTF-8 sequence length,
 * or 0 if the bytes at s are not a well-formed UTF-8 character. */
static int sb_utf8_one(const unsigned char *s, size_t avail) {
    unsigned char c;
    if (!s || avail == 0) return 0;
    c = s[0];
    if (c < 0x80) return 1;
    if (c < 0xC2 || c > 0xF4) return 0;
    if (c <= 0xDF) {
        if (avail < 2 || (s[1] & 0xC0) != 0x80) return 0;
        return 2;
    }
    if (c <= 0xEF) {
        if (avail < 3 || (s[1] & 0xC0) != 0x80 || (s[2] & 0xC0) != 0x80) return 0;
        if (c == 0xE0 && s[1] < 0xA0) return 0;
        if (c == 0xED && s[1] >= 0xA0) return 0;
        return 3;
    }
    if (avail < 4 || (s[1] & 0xC0) != 0x80 || (s[2] & 0xC0) != 0x80 || (s[3] & 0xC0) != 0x80)
        return 0;
    if (c == 0xF0 && s[1] < 0x90) return 0;
    if (c == 0xF4 && s[1] >= 0x90) return 0;
    return 4;
}

/* Append the JSON string form of s to the builder. field_cap must bound
 * the source: we insist on a NUL inside it, then walk real UTF-8 chars
 * so json_escape can never skip a broken byte, and require it to consume
 * the whole string. On any doubt, fail and leave nothing half-written:
 * the caller clears buf/out_len when failed. */
static void sb_string(sb_t *b, const char *s, size_t field_cap) {
    size_t slen, used = 0, wrote;
    const unsigned char *p;
    size_t avail;
    char q[2];
    if (b->failed) return;
    if (!s || !memchr(s, '\0', field_cap)) { b->failed = 1; return; }
    slen = strlen(s);
    for (p = (const unsigned char *)s, avail = slen; avail > 0; ) {
        int k = sb_utf8_one(p, avail);
        if (k == 0) { b->failed = 1; return; }
        p += k; avail -= (size_t)k;
    }
    q[0] = '"'; q[1] = '\0';
    sb_append(b, q);
    if (b->failed) return;
    wrote = json_escape(s, b->buf + b->n, b->cap - b->n, &used);
    if (b->failed) return;
    if (used != slen || wrote >= b->cap - b->n) { b->failed = 1; return; }
    b->n += wrote;
    b->buf[b->n] = '\0';
    q[0] = '"'; q[1] = '\0';
    sb_append(b, q);
}

/* Key/value pair with a leading comma rule: 'first' tracks whether a
 * comma must precede. Key is a trusted C literal so no escaping needed
 * beyond quotes; value bounds come from the caller (sizeof field). */
static void sb_field(sb_t *b, int *first, const char *key,
                     const char *value, size_t field_cap) {
    char k[128];
    if (b->failed) return;
    if (!*first) sb_append(b, ",");
    *first = 0;
    snprintf(k, sizeof k, "\"%s\":", key);
    sb_append(b, k);
    if (b->failed) return;
    sb_string(b, value, field_cap);
}

/* Encode st as strict v2 JSON into buf (cap bytes, NUL-terminated).
 * out_len gets byte length excluding NUL. On any doubt we clear the
 * output, set why, and return -1. The result must pass
 * reload_state_validate_v2 before we accept it. */
int reload_session_encode_v2(const reload_session_state *st,
                             char *buf, size_t cap, size_t *out_len,
                             char *why, size_t cap_why) {
    sb_t b;
    int first = 1;
    char num[64];
    if (out_len) *out_len = 0;
    if (!st || !buf || !out_len || cap == 0) {
        sb_why(why, cap_why, "null or zero cap");
        if (buf && cap) buf[0] = '\0';
        return -1;
    }
    buf[0] = '\0';
    b.buf = buf; b.cap = cap; b.n = 0; b.failed = 0;
    if (st->npending > 8 || st->nhistory > 16) {
        sb_why(why, cap_why, "counts out of range");
        buf[0] = '\0'; return -1;
    }
    if (st->history_pos > st->nhistory) {
        sb_why(why, cap_why, "history_pos out of range");
        buf[0] = '\0'; return -1;
    }
    if (st->history_browsing != 0 && st->history_browsing != 1) {
        sb_why(why, cap_why, "history_browsing not bool");
        buf[0] = '\0'; return -1;
    }
    if (st->loop_on != 0 && st->loop_on != 1) {
        sb_why(why, cap_why, "loop_on not bool");
        buf[0] = '\0'; return -1;
    }
    if (st->loop_left < 0 || st->loop_left > 8) {
        sb_why(why, cap_why, "loop_left out of range");
        buf[0] = '\0'; return -1;
    }
    if (st->journal_offset > 9007199254740991ULL) {
        sb_why(why, cap_why, "journal_offset too large");
        buf[0] = '\0'; return -1;
    }
    sb_append(&b, "{");
    sb_append(&b, "\"version\":2");
    first = 0;
    sb_field(&b, &first, "session_id", st->session_id, sizeof st->session_id);
    sb_field(&b, &first, "handoff_id", st->handoff_id, sizeof st->handoff_id);
    sb_field(&b, &first, "candidate_hash", st->candidate_hash, sizeof st->candidate_hash);
    sb_field(&b, &first, "goal", st->goal, sizeof st->goal);
    sb_append(&b, st->loop_on ? ",\"loop_on\":true" : ",\"loop_on\":false");
    snprintf(num, sizeof num, "%d", st->loop_left);
    sb_append(&b, ",\"loop_left\":");
    sb_append(&b, num);
    sb_field(&b, &first, "cwd", st->cwd, sizeof st->cwd);
    sb_field(&b, &first, "role", st->role, sizeof st->role);
    sb_field(&b, &first, "peer", st->peer, sizeof st->peer);
    sb_field(&b, &first, "input", st->input, sizeof st->input);
    sb_field(&b, &first, "history_draft", st->history_draft, sizeof st->history_draft);
    if (b.failed) goto fail;
    if (!first) sb_append(&b, ",");
    first = 1;
    sb_append(&b, "\"journal\":{");
    sb_field(&b, &first, "path", st->journal_path, sizeof st->journal_path);
    snprintf(num, sizeof num, "%llu", st->journal_offset);
    if (!first) sb_append(&b, ",");
    first = 0;
    sb_append(&b, "\"offset\":");
    sb_append(&b, num);
    sb_append(&b, "}");
    if (b.failed) goto fail;
    sb_append(&b, ",\"pending_queue\":[");
    {
        size_t i;
        for (i = 0; i < st->npending; i++) {
            if (i) sb_append(&b, ",");
            sb_string(&b, st->pending[i], sizeof st->pending[i]);
        }
    }
    sb_append(&b, "]");
    if (b.failed) goto fail;
    sb_append(&b, ",\"history\":[");
    {
        size_t i;
        for (i = 0; i < st->nhistory; i++) {
            if (i) sb_append(&b, ",");
            sb_string(&b, st->history[i], sizeof st->history[i]);
        }
    }
    sb_append(&b, "]");
    if (b.failed) goto fail;
    snprintf(num, sizeof num, "%llu", (unsigned long long)st->history_pos);
    sb_append(&b, ",\"history_pos\":");
    sb_append(&b, num);
    sb_append(&b, st->history_browsing ? ",\"history_browsing\":true"
                                       : ",\"history_browsing\":false");
    sb_append(&b, "}");
    if (b.failed) goto fail;
    {
        char vw[256];
        vw[0] = '\0';
        if (reload_state_validate_v2(buf, b.n, vw, sizeof vw) != 1) {
            sb_why(why, cap_why, vw[0] ? vw : "validate failed");
            buf[0] = '\0'; *out_len = 0; return -1;
        }
    }
    *out_len = b.n;
    sb_why(why, cap_why, "ok");
    return 0;
fail:
    sb_why(why, cap_why, "buffer or string error");
    buf[0] = '\0';
    *out_len = 0;
    return -1;
}
/* EOF */
