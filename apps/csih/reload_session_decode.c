/*
 * reload_session_decode.c — decode a strict v2 session blob into
 * reload_session_state. LIBRARY, NO main.
 *
 * WHY: the v2 handoff ships one JSON blob; the new process must fill its
 * in-memory session from it. This file owns exactly one question: given a
 * validated v2 text, produce the struct. It adds no new JSON parser and no
 * new validator: reload_state_validate_v2() runs first, json.c reads after.
 * encode, IO and the TUI live elsewhere.
 */
#include "reload_session.h"
#include <string.h>
#include <stdio.h>

/* ── the slice of json.c used here (types restated verbatim from
 *    reload_state.c so this file compiles standalone; json.c is an earlier
 *    file in the same unit). No jitem/jbool exist: arrays use items/len,
 *    bools use the b member. ────────────────────────────────────────── */
#ifndef CSIH_RELOAD_JSON_TYPES
#define CSIH_RELOAD_JSON_TYPES
typedef enum { J_NULL, J_BOOL, J_NUM, J_STR, J_ARR, J_OBJ } jkind;
typedef struct jvalue {
    jkind kind;
    int    b;
    double n;
    char  *s;
    struct jvalue **items;  size_t len;
    char **keys; struct jvalue **vals; size_t nkeys;
} jvalue;
#endif

jvalue *json_parse(const char *text, size_t len, char *errbuf, size_t errlen);
jvalue *jget(jvalue *obj, const char *key);
const char *jstr(jvalue *v);
void    jfree(jvalue *v);

int reload_state_validate_v2(const char *text, size_t len, char *why, size_t cap);

/* ── helpers: copy a validated string, read a field ────────────────── */

/* copy v->s into dst[cap], NUL-terminated; caller guarantees v is J_STR and
 * that strlen(v->s) < cap (validation already bounded text to 4095). */
static void copy_str(char *dst, size_t cap, jvalue *v) {
    size_t n = strlen(jstr(v));
    if (n >= cap) n = cap - 1;
    memcpy(dst, jstr(v), n);
    dst[n] = '\0';
}

/* ── the one public entry point ────────────────────────────────────── */

int reload_session_decode_v2(const char *text, size_t len,
                             reload_session_state *out,
                             char *why, size_t cap) {
    char err[128];
    jvalue *root, *v, *j, *arr;

    if (!out) {
        if (why && cap) snprintf(why, cap, "null output");
        return -1;
    }
    memset(out, 0, sizeof *out);
    if (why && cap) why[0] = '\0';

    /* strict: refuse anything the v2 validator refuses; no v1 fallback. */
    if (reload_state_validate_v2(text, len, why, cap) != 1) {
        return -1;
    }
    if (!text) { return -1; }

    root = json_parse(text, len, err, sizeof err);
    if (!root) {
        if (why && cap) snprintf(why, cap, "parse: %s", err);
        return -1;
    }

    copy_str(out->session_id, sizeof out->session_id, jget(root, "session_id"));
    copy_str(out->handoff_id, sizeof out->handoff_id, jget(root, "handoff_id"));
    copy_str(out->candidate_hash, sizeof out->candidate_hash, jget(root, "candidate_hash"));
    copy_str(out->goal, sizeof out->goal, jget(root, "goal"));
    out->loop_on = jget(root, "loop_on")->b;
    out->loop_left = (int)jget(root, "loop_left")->n;
    copy_str(out->cwd, sizeof out->cwd, jget(root, "cwd"));
    copy_str(out->role, sizeof out->role, jget(root, "role"));
    copy_str(out->peer, sizeof out->peer, jget(root, "peer"));
    copy_str(out->input, sizeof out->input, jget(root, "input"));
    copy_str(out->history_draft, sizeof out->history_draft, jget(root, "history_draft"));
    /* journal.path / journal.offset -> journal_path / journal_offset */
    j = jget(root, "journal");
    copy_str(out->journal_path, sizeof out->journal_path, jget(j, "path"));
    out->journal_offset = (unsigned long long)jget(j, "offset")->n;

    /* pending_queue -> pending / npending */
    arr = jget(root, "pending_queue");
    out->npending = arr ? arr->len : 0;
    for (size_t i = 0; i < out->npending && i < 8; i++) {
        copy_str(out->pending[i], sizeof out->pending[i], arr->items[i]);
    }
    /* history / history_pos / history_browsing */
    arr = jget(root, "history");
    out->nhistory = arr ? arr->len : 0;
    for (size_t i = 0; i < out->nhistory && i < 16; i++) {
        copy_str(out->history[i], sizeof out->history[i], arr->items[i]);
    }
    v = jget(root, "history_pos");
    out->history_pos = (size_t)v->n;
    v = jget(root, "history_browsing");
    out->history_browsing = v->b;

    jfree(root);
    if (why && cap) snprintf(why, cap, "ok");
    return 0;
}
