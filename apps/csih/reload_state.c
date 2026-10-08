/*
 * reload_state.c — validate the /reload-code state-v1 snapshot. LIBRARY, NO main.
 *
 * WHY: the hot-reload protocol ships a small JSON state blob between the old
 * and new process. A malformed blob must be refused before it can steer a
 * reload, so this file owns exactly one question: is this text a valid v1
 * state? It reuses the json.c reader (json_parse/jget/jfree) and adds no new
 * JSON parser. The CLI (reload_state_cli.c) owns main and its selftest.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── the slice of json.c this library uses (types restated so this file
 *    compiles standalone; json.c is an earlier file in the same unit) ───── */
typedef enum { J_NULL, J_BOOL, J_NUM, J_STR, J_ARR, J_OBJ } jkind;
typedef struct jvalue {
    jkind kind;
    int    b;
    double n;
    char  *s;
    struct jvalue **items;  size_t len;
    char **keys; struct jvalue **vals; size_t nkeys;
} jvalue;

jvalue *json_parse(const char *text, size_t len, char *errbuf, size_t errlen);
void    jfree(jvalue *v);
jvalue *jget(jvalue *obj, const char *key);
const char *jstr(jvalue *v);
double  jnum(jvalue *v, double dflt);
size_t  jlen(jvalue *v);

#define RS_MAX_STR   4095
#define RS_MAX_QUEUE 8

static int rs_fail(char *why, size_t cap, const char *msg) {
    if (why && cap) snprintf(why, cap, "%s", msg);
    return 0;
}

/* A field is present exactly once: jget returns the first, we count all. */
static int rs_count_key(jvalue *obj, const char *key) {
    int n = 0;
    for (size_t i = 0; i < obj->nkeys; i++)
        if (!strcmp(obj->keys[i], key)) n++;
    return n;
}

/* Every key must be one of the v1 whitelist; anything else is rejected. */
static int rs_all_known(jvalue *obj, char *why, size_t cap) {
    static const char *ok[] = {
        "version", "session_id", "handoff_id", "candidate_hash",
        "goal", "cwd", "role", "peer", "journal", "input", "pending_queue"
    };
    for (size_t i = 0; i < obj->nkeys; i++) {
        int known = 0;
        for (size_t k = 0; k < sizeof(ok)/sizeof(ok[0]); k++)
            if (!strcmp(obj->keys[i], ok[k])) { known = 1; break; }
        if (!known) {
            char b[96];
            snprintf(b, sizeof b, "unknown key: %.48s", obj->keys[i]);
            return rs_fail(why, cap, b);
        }
    }
    return 1;
}

/* Identity/role strings: non-empty and limited to a safe printable set. */
static int rs_safe_id(const char *s, int allow_empty) {
    if (!s) return 0;
    if (!*s) return allow_empty ? 1 : 0;
    for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
        if (*p >= 0x80) return 0;      /* non-ASCII bytes are refused */
        if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
            (*p >= '0' && *p <= '9')) continue;
        if (*p == '_' || *p == '-' || *p == '.' || *p == ':') continue;
        return 0;
    }
    return 1;
}

/* 64 lowercase-or-upper hex, exactly. */
static int rs_hash64(const char *s) {
    if (!s) return 0;
    for (int i = 0; i < 64; i++) {
        char c = s[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
              (c >= 'A' && c <= 'F'))) return 0;
    }
    return s[64] == '\0';
}

static int rs_absolute(const char *s) {
    return s && s[0] == '/';
}

/* Free-text fields (goal/input): present, string, <=4095 bytes. Any
 * characters are allowed; identity restrictions do not apply. */
static int rs_text_field(jvalue *obj, const char *key, char *why, size_t cap) {
    jvalue *v = jget(obj, (char *)key);
    char b[96];
    if (!v || v->kind != J_STR) {
        snprintf(b, sizeof b, "%s: missing or not string", key);
        return rs_fail(why, cap, b);
    }
    if (rs_count_key(obj, key) != 1) {
        snprintf(b, sizeof b, "%s: duplicate", key);
        return rs_fail(why, cap, b);
    }
    if (!rs_upto4095(jstr(v))) {
        snprintf(b, sizeof b, "%s: over 4095", key);
        return rs_fail(why, cap, b);
    }
    return 1;
}

static int rs_upto4095(const char *s) {
    return s && strlen(s) <= RS_MAX_STR;
}

/* offset must be a non-negative exact integer (no fraction, sign, inf, nan),
 * within [0, 9007199254740991] (2^53-1, the largest integer a double holds
 * exactly). A larger literal such as 9007199254740993 rounds to 2^53 in the
 * double reader, so the bound is the safe upper limit, not the wrap point. */
static int rs_offset_ok(jvalue *v) {
    if (!v || v->kind != J_NUM) return 0;
    double d = v->n;
    if (d < 0) return 0;
    if (d != d) return 0;                        /* NaN */
    if (d > 9007199254740991.0) return 0;        /* 2^53-1: safe int max */
    if (d != (double)(long long)d) return 0;     /* fraction */
    return 1;
}

static int rs_journal(jvalue *obj, char *why, size_t cap) {
    jvalue *path, *off;
    if (!obj || obj->kind != J_OBJ) return rs_fail(why, cap, "journal: not object");
    if (obj->nkeys != 2) return rs_fail(why, cap, "journal: need path+offset");
    path = jget(obj, "path");
    off  = jget(obj, "offset");
    if (!path || !off) return rs_fail(why, cap, "journal: missing path/offset");
    if (path->kind != J_STR) return rs_fail(why, cap, "journal.path: not string");
    if (!rs_absolute(jstr(path))) return rs_fail(why, cap, "journal.path: not absolute");
    if (!rs_upto4095(jstr(path))) return rs_fail(why, cap, "journal.path: too long");
    if (off->kind != J_NUM) return rs_fail(why, cap, "journal.offset: not number");
    if (!rs_offset_ok(off)) return rs_fail(why, cap, "journal.offset: not non-neg integer");
    return 1;
}

static int rs_queue(jvalue *v, char *why, size_t cap) {
    if (!v || v->kind != J_ARR) return rs_fail(why, cap, "pending_queue: not array");
    if (v->len > RS_MAX_QUEUE) return rs_fail(why, cap, "pending_queue: over 8");
    for (size_t i = 0; i < v->len; i++) {
        if (v->items[i]->kind != J_STR)
            return rs_fail(why, cap, "pending_queue: item not string");
        if (!rs_upto4095(jstr(v->items[i])))
            return rs_fail(why, cap, "pending_queue: item over 4095");
    }
    return 1;
}

static int rs_str_field(jvalue *obj, const char *key, int allow_empty,
                        int need_abs, char *why, size_t cap) {
    jvalue *v = jget(obj, (char *)key);
    char b[96];
    if (!v || v->kind != J_STR) {
        snprintf(b, sizeof b, "%s: missing or not string", key);
        return rs_fail(why, cap, b);
    }
    if (rs_count_key(obj, key) != 1) {
        snprintf(b, sizeof b, "%s: duplicate", key);
        return rs_fail(why, cap, b);
    }
    if (!rs_upto4095(jstr(v))) {
        snprintf(b, sizeof b, "%s: over 4095", key);
        return rs_fail(why, cap, b);
    }
    if (need_abs ? !rs_absolute(jstr(v)) : !rs_safe_id(jstr(v), allow_empty)) {
        snprintf(b, sizeof b, "%s: bad value", key);
        return rs_fail(why, cap, b);
    }
    return 1;
}

int reload_state_validate(const char *text, size_t len, char *why, size_t cap) {
    static const char *req[] = {
        "version", "session_id", "handoff_id", "candidate_hash",
        "goal", "cwd", "role", "peer", "journal", "input", "pending_queue"
    };
    char err[128];
    jvalue *root, *v;

    if (why && cap) why[0] = '\0';
    if (!text) return rs_fail(why, cap, "no input");

    root = json_parse(text, len, err, sizeof err);
    if (!root) {
        char b[160];
        snprintf(b, sizeof b, "parse: %s", err);
        return rs_fail(why, cap, b);
    }
    if (root->kind != J_OBJ) { jfree(root); return rs_fail(why, cap, "root: not object"); }

    if (!rs_all_known(root, why, cap)) { jfree(root); return 0; }

    for (size_t i = 0; i < sizeof(req)/sizeof(req[0]); i++) {
        if (!jget(root, (char *)req[i])) {
            char b[96];
            snprintf(b, sizeof b, "missing: %s", req[i]);
            jfree(root);
            return rs_fail(why, cap, b);
        }
        if (rs_count_key(root, req[i]) != 1) {
            char b[96];
            snprintf(b, sizeof b, "%s: duplicate", req[i]);
            jfree(root);
            return rs_fail(why, cap, b);
        }
    }

    /* version: must be the integer 1, not "1" and not 1.5 */
    v = jget(root, "version");
    if (v->kind != J_NUM || v->n != 1.0) {
        jfree(root); return rs_fail(why, cap, "version: not integer 1");
    }
    if (rs_count_key(root, "version") != 1) {
        jfree(root); return rs_fail(why, cap, "version: duplicate");
    }

    if (!rs_str_field(root, "session_id", 0, 0, why, cap)) { jfree(root); return 0; }
    if (!rs_str_field(root, "handoff_id", 0, 0, why, cap)) { jfree(root); return 0; }
    if (!rs_str_field(root, "role", 0, 0, why, cap)) { jfree(root); return 0; }
    if (!rs_str_field(root, "peer", 1, 0, why, cap)) { jfree(root); return 0; }
    if (!rs_text_field(root, "goal", why, cap)) { jfree(root); return 0; }
    if (!rs_text_field(root, "input", why, cap)) { jfree(root); return 0; }
    if (!rs_str_field(root, "cwd", 0, 1, why, cap)) { jfree(root); return 0; }

    /* candidate_hash: exactly 64 hex */
    v = jget(root, "candidate_hash");
    if (v->kind != J_STR) { jfree(root); return rs_fail(why, cap, "candidate_hash: not string"); }
    if (rs_count_key(root, "candidate_hash") != 1) {
        jfree(root); return rs_fail(why, cap, "candidate_hash: duplicate");
    }
    if (!rs_hash64(jstr(v))) { jfree(root); return rs_fail(why, cap, "candidate_hash: not 64 hex"); }

    if (!rs_journal(jget(root, "journal"), why, cap)) { jfree(root); return 0; }
    if (!rs_queue(jget(root, "pending_queue"), why, cap)) { jfree(root); return 0; }

    jfree(root);
    if (why && cap) snprintf(why, cap, "ok");
    return 1;
}







