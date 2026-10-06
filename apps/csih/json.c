/*
 * json.c — a minimal JSON reader for the C99 harness. LIBRARY, NO `main`.
 *
 * WHY NO `main` HERE (changed 2026-10-01, loop.c): a source file run directly
 * is a program, and two files with `main` cannot be combined —
 *     unisacc gate.c json.c session.c loop.c    →  arm64: main:
 * which reads like a missing backend feature and is not. loop.c drives the
 * three libraries, so they drop `main` and each keeps a CLI file
 * (json_cli.c). The self-test stays here, callable as `json_run_selftest`.
 *
 * WHY THIS LAYER, AND WHY NOW: the harness's session format is JSONL — one
 * JSON object per line. Reading it is the cheapest useful thing the C99 side
 * can own, because it needs ONLY stdio/stdlib/string/ctype: none of the headers
 * that unisacc 0.0.18 has yet to provide (termios, stat, socket). So it can be
 * written and verified TODAY, and it shrinks what is left to port later.
 *
 * Deliberately NOT a general JSON library. It is a scanner + a value tree over
 * the subset the session format actually uses: objects, arrays, strings,
 * numbers, true/false/null. No unicode escapes beyond \uXXXX → UTF-8, no
 * streaming, no DOM mutation. The measured need is "read a record and pull
 * fields out of it", and a bigger library would be more surface to get wrong.
 *
 * Writing is one function, json_escape(): a string body (no surrounding
 * quotes) that is always legal UTF-8. A stray lead byte must not reach a
 * strict peer — that was HTTP 400, "invalid unicode code point".
 *
 * json_value_end() is the one string-aware span. Braces inside a string do
 * not end the value, and nesting stops at JSON_MAX_DEPTH. Callers use it
 * to cut the first value out of model prose. It does not build a tree.
 *
 * The CLI takes SUBCOMMANDS, not dash-options: measured on unisacc 0.0.17,
 * running a source directly reserves the dash flags for the compiler itself.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Self-contained: the type and prototypes live here, not in a separate header.
 * WHY: unisacc runs a source DIRECTLY, so a file is a program, not a translation
 * unit to be linked. Splitting into .h/.c and then linking two objects was me
 * importing gcc habits — it produced a "duplicate symbol _main" that has no
 * meaning in a world where each .c IS the executable. */

typedef enum { J_NULL, J_BOOL, J_NUM, J_STR, J_ARR, J_OBJ } jkind;

typedef struct jvalue {
    jkind kind;
    int    b;
    double n;
    char  *s;
    struct jvalue **items;  size_t len;
    char **keys; struct jvalue **vals; size_t nkeys;
} jvalue;

#define JSON_MAX_DEPTH 32

jvalue *json_parse(const char *text, size_t len, char *errbuf, size_t errlen);
size_t  json_value_end(const char *text, size_t len);
void    jfree(jvalue *v);
size_t  json_escape(const char *in, char *out, size_t outlen, size_t *in_used);
jvalue *jget(jvalue *obj, const char *key);
const char *jstr(jvalue *v);
double  jnum(jvalue *v, double dflt);
size_t  jlen(jvalue *v);



typedef struct {
    const char *p;
    const char *end;
    char err[128];
    int  failed;
    int  depth;
} jparser;

static void *jalloc(size_t n) { return calloc(1, n ? n : 1); }

static jvalue *jnew(jkind k) {
    jvalue *v = (jvalue *)jalloc(sizeof(jvalue));
    if (v) v->kind = k;
    return v;
}

void jfree(jvalue *v) {
    size_t i;
    if (!v) return;
    if (v->s) free(v->s);
    for (i = 0; i < v->len; i++) jfree(v->items[i]);
    if (v->items) free(v->items);
    for (i = 0; i < v->nkeys; i++) { free(v->keys[i]); jfree(v->vals[i]); }
    if (v->keys) free(v->keys);
    if (v->vals) free(v->vals);
    free(v);
}

static void jfail(jparser *ps, const char *msg) {
    if (!ps->failed) {
        ps->failed = 1;
        snprintf(ps->err, sizeof(ps->err), "%s", msg);
    }
}

static void jskip_ws(jparser *ps) {
    while (ps->p < ps->end && (*ps->p==' '||*ps->p=='\t'||*ps->p=='\n'||*ps->p=='\r')) ps->p++;
}

static int jhex4(const char *s, unsigned *out) {
    unsigned v = 0; int i;
    for (i = 0; i < 4; i++) {
        char c = s[i]; unsigned d;
        if (c>='0'&&c<='9') d = (unsigned)(c-'0');
        else if (c>='a'&&c<='f') d = (unsigned)(c-'a'+10);
        else if (c>='A'&&c<='F') d = (unsigned)(c-'A'+10);
        else return 0;
        v = v*16u + d;
    }
    *out = v; return 1;
}

/* Append one UTF-8 encoded code point. */
static size_t put_utf8(char *dst, unsigned cp) {
    if (cp < 0x80) { dst[0] = (char)cp; return 1; }
    if (cp < 0x800) { dst[0]=(char)(0xC0|(cp>>6)); dst[1]=(char)(0x80|(cp&0x3F)); return 2; }
    if (cp < 0x10000) { dst[0]=(char)(0xE0|(cp>>12)); dst[1]=(char)(0x80|((cp>>6)&0x3F)); dst[2]=(char)(0x80|(cp&0x3F)); return 3; }
    dst[0]=(char)(0xF0|(cp>>18)); dst[1]=(char)(0x80|((cp>>12)&0x3F));
    dst[2]=(char)(0x80|((cp>>6)&0x3F)); dst[3]=(char)(0x80|(cp&0x3F)); return 4;
}

static char *jparse_string_raw(jparser *ps) {
    size_t cap = 32, len = 0;
    char *out;
    if (ps->p >= ps->end || *ps->p != '"') { jfail(ps, "expected string"); return NULL; }
    ps->p++;
    out = (char *)jalloc(cap);
    if (!out) { jfail(ps, "oom"); return NULL; }
    while (ps->p < ps->end && *ps->p != '"') {
        unsigned char c = (unsigned char)*ps->p;
        if (len + 8 >= cap) {
            char *n = (char *)realloc(out, cap * 2);
            if (!n) { free(out); jfail(ps, "oom"); return NULL; }
            out = n; cap *= 2;
        }
        if (c == '\\') {
            ps->p++;
            if (ps->p >= ps->end) { free(out); jfail(ps, "truncated escape"); return NULL; }
            switch (*ps->p) {
                case '"': out[len++]='"'; ps->p++; break;
                case '\\': out[len++]='\\'; ps->p++; break;
                case '/': out[len++]='/'; ps->p++; break;
                case 'b': out[len++]='\b'; ps->p++; break;
                case 'f': out[len++]='\f'; ps->p++; break;
                case 'n': out[len++]='\n'; ps->p++; break;
                case 'r': out[len++]='\r'; ps->p++; break;
                case 't': out[len++]='\t'; ps->p++; break;
                case 'u': {
                    unsigned cp;
                    if (ps->p + 5 > ps->end || !jhex4(ps->p + 1, &cp)) { free(out); jfail(ps, "bad \\u"); return NULL; }
                    ps->p += 5;
                    /* A surrogate pair is two \u escapes; combine them. */
                    if (cp >= 0xD800 && cp <= 0xDBFF && ps->p + 6 <= ps->end &&
                        ps->p[0] == '\\' && ps->p[1] == 'u') {
                        unsigned lo;
                        if (jhex4(ps->p + 2, &lo) && lo >= 0xDC00 && lo <= 0xDFFF) {
                            cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                            ps->p += 6;
                        }
                    }
                    /* A leftover surrogate is not a Unicode scalar. Emitting
                     * it would put ill-formed UTF-8 back on the wire. */
                    if (cp >= 0xD800 && cp <= 0xDFFF) {
                        free(out); jfail(ps, "bad \\u"); return NULL;
                    }
                    len += put_utf8(out + len, cp);
                    break;
                }
                default: free(out); jfail(ps, "unknown escape"); return NULL;
            }
        } else {
            out[len++] = (char)c;
            ps->p++;
        }
    }
    if (ps->p >= ps->end) { free(out); jfail(ps, "unterminated string"); return NULL; }
    ps->p++;                     /* closing quote */
    out[len] = '\0';
    return out;
}

static jvalue *jparse_value(jparser *ps);

static jvalue *jparse_array(jparser *ps) {
    jvalue *v = jnew(J_ARR);
    size_t cap = 8;
    if (!v) { jfail(ps, "oom"); return NULL; }
    v->items = (jvalue **)jalloc(cap * sizeof(jvalue *));
    if (!v->items) { free(v); jfail(ps, "oom"); return NULL; }
    ps->p++;                     /* [ */
    jskip_ws(ps);
    if (ps->p < ps->end && *ps->p == ']') { ps->p++; return v; }
    for (;;) {
        jvalue *item;
        if (v->len == cap) {
            jvalue **n = (jvalue **)realloc(v->items, cap * 2 * sizeof(jvalue *));
            if (!n) { jfail(ps, "oom"); return v; }
            v->items = n; cap *= 2;
        }
        item = jparse_value(ps);
        if (ps->failed) return v;
        v->items[v->len++] = item;
        jskip_ws(ps);
        if (ps->p < ps->end && *ps->p == ',') { ps->p++; jskip_ws(ps); continue; }
        if (ps->p < ps->end && *ps->p == ']') { ps->p++; return v; }
        jfail(ps, "expected , or ]"); return v;
    }
}

static jvalue *jparse_object(jparser *ps) {
    jvalue *v = jnew(J_OBJ);
    size_t cap = 8;
    if (!v) { jfail(ps, "oom"); return NULL; }
    v->keys = (char **)jalloc(cap * sizeof(char *));
    v->vals = (jvalue **)jalloc(cap * sizeof(jvalue *));
    if (!v->keys || !v->vals) { jfail(ps, "oom"); return v; }
    ps->p++;                     /* { */
    jskip_ws(ps);
    if (ps->p < ps->end && *ps->p == '}') { ps->p++; return v; }
    for (;;) {
        char *k;
        jvalue *val;
        jskip_ws(ps);
        k = jparse_string_raw(ps);
        if (ps->failed) return v;
        jskip_ws(ps);
        if (ps->p >= ps->end || *ps->p != ':') { free(k); jfail(ps, "expected :"); return v; }
        ps->p++;
        jskip_ws(ps);
        val = jparse_value(ps);
        if (ps->failed) { free(k); return v; }
        if (v->nkeys == cap) {
            char **nk = (char **)realloc(v->keys, cap * 2 * sizeof(char *));
            jvalue **nv = (jvalue **)realloc(v->vals, cap * 2 * sizeof(jvalue *));
            if (!nk || !nv) { free(k); jfail(ps, "oom"); return v; }
            v->keys = nk; v->vals = nv; cap *= 2;
        }
        v->keys[v->nkeys] = k;
        v->vals[v->nkeys] = val;
        v->nkeys++;
        jskip_ws(ps);
        if (ps->p < ps->end && *ps->p == ',') { ps->p++; continue; }
        if (ps->p < ps->end && *ps->p == '}') { ps->p++; return v; }
        jfail(ps, "expected , or }"); return v;
    }
}

static jvalue *jparse_value(jparser *ps) {
    jskip_ws(ps);
    if (ps->p >= ps->end) { jfail(ps, "unexpected end"); return NULL; }
    if (*ps->p == '{' || *ps->p == '[') {
        jvalue *nest;
        if (ps->depth >= JSON_MAX_DEPTH) { jfail(ps, "too deep"); return NULL; }
        ps->depth++;
        nest = (*ps->p == '{') ? jparse_object(ps) : jparse_array(ps);
        ps->depth--;
        return nest;
    }
    switch (*ps->p) {
        case '"': { jvalue *v = jnew(J_STR); if (!v) { jfail(ps,"oom"); return NULL; }
                    v->s = jparse_string_raw(ps); if (ps->failed) { jfree(v); return NULL; } return v; }
        case 't': if (ps->end - ps->p >= 4 && !strncmp(ps->p,"true",4)) { jvalue *v=jnew(J_BOOL); if(!v){jfail(ps,"oom");return NULL;} v->b=1; ps->p+=4; return v; } break;
        case 'f': if (ps->end - ps->p >= 5 && !strncmp(ps->p,"false",5)) { jvalue *v=jnew(J_BOOL); if(!v){jfail(ps,"oom");return NULL;} v->b=0; ps->p+=5; return v; } break;
        case 'n': if (ps->end - ps->p >= 4 && !strncmp(ps->p,"null",4)) { ps->p+=4; return jnew(J_NULL); } break;
        default: break;
    }
    if (*ps->p == '-' || isdigit((unsigned char)*ps->p)) {
        char buf[64], *endptr;
        size_t n = 0;
        jvalue *v;
        int lead;
        if (*ps->p == '-') buf[n++] = *ps->p++;
        while (ps->p < ps->end && n < sizeof(buf) - 1 &&
               (isdigit((unsigned char)*ps->p) || *ps->p=='.' || *ps->p=='e' || *ps->p=='E' ||
                ((*ps->p=='+'||*ps->p=='-') && n > 0 && (buf[n-1]=='e'||buf[n-1]=='E'))))
            buf[n++] = *ps->p++;
        buf[n] = '\0';
        lead = (ps->p < ps->end && (isdigit((unsigned char)*ps->p) || *ps->p=='.' ||
                *ps->p=='e' || *ps->p=='E' || *ps->p=='+' || *ps->p=='-'));
        if (n == 0 || lead) { jfail(ps, "bad number"); return NULL; }
        v = jnew(J_NUM);
        if (!v) { jfail(ps, "oom"); return NULL; }
        v->n = strtod(buf, &endptr);
        if (endptr != buf + n ||
            (buf[0] == '0' && n > 1 && isdigit((unsigned char)buf[1])) ||
            (buf[0] == '-' && n > 2 && buf[1] == '0' && isdigit((unsigned char)buf[2]))) {
            jfree(v);
            jfail(ps, "bad number");
            return NULL;
        }
        return v;
    }
    jfail(ps, "unexpected token");
    return NULL;
}

jvalue *json_parse(const char *text, size_t len, char *errbuf, size_t errlen) {
    jparser ps;
    jvalue *v;
    ps.p = text; ps.end = text + len; ps.failed = 0; ps.err[0] = '\0'; ps.depth = 0;
    v = jparse_value(&ps);
    if (!ps.failed) {
        jskip_ws(&ps);
        /* Trailing content is an error: a JSONL reader that silently accepts
         * `{...} garbage` would hide a corrupt log line. */
        if (ps.p != ps.end) jfail(&ps, "trailing content");
    }
    if (ps.failed) {
        if (errbuf) snprintf(errbuf, errlen, "%s", ps.err);
        jfree(v);
        return NULL;
    }
    return v;
}

/* Bytes from text[0] through the first JSON value, including its leading
 * whitespace. 0 means there is no closed value. Braces and quotes inside a
 * string do not change depth. Nesting above JSON_MAX_DEPTH is not a value. */
size_t json_value_end(const char *text, size_t len) {
    size_t i = 0;
    char expect[JSON_MAX_DEPTH];
    int depth;
    if (!text) return 0;
    while (i < len && (text[i] == ' ' || text[i] == '\t' || text[i] == '\n' || text[i] == '\r'))
        i++;
    if (i >= len) return 0;
    if (text[i] == '{' || text[i] == '[') {
        depth = 0;
        expect[depth++] = (text[i] == '{') ? '}' : ']';
        i++;
        while (i < len && depth > 0) {
            if (text[i] == '"') {
                i++;
                while (i < len) {
                    if (text[i] == '\\') {
                        if (i + 1 >= len) return 0;
                        i += 2;
                        continue;
                    }
                    if (text[i] == '"') { i++; break; }
                    i++;
                }
                continue;
            }
            if (text[i] == '{' || text[i] == '[') {
                if (depth >= JSON_MAX_DEPTH) return 0;
                expect[depth++] = (text[i] == '{') ? '}' : ']';
                i++;
                continue;
            }
            if (text[i] == '}' || text[i] == ']') {
                depth--;
                if (text[i] != expect[depth]) return 0;
                i++;
                continue;
            }
            i++;
        }
        return depth == 0 ? i : 0;
    }
    if (text[i] == '"') {
        i++;
        while (i < len) {
            if (text[i] == '\\') {
                if (i + 1 >= len) return 0;
                i += 2;
                continue;
            }
            if (text[i] == '"') return i + 1;
            i++;
        }
        return 0;
    }
    if (i + 4 <= len && text[i] == 't' && text[i+1] == 'r' && text[i+2] == 'u' && text[i+3] == 'e')
        return i + 4;
    if (i + 5 <= len && text[i] == 'f' && text[i+1] == 'a' && text[i+2] == 'l' && text[i+3] == 's' && text[i+4] == 'e')
        return i + 5;
    if (i + 4 <= len && text[i] == 'n' && text[i+1] == 'u' && text[i+2] == 'l' && text[i+3] == 'l')
        return i + 4;
    if (text[i] == '-' || isdigit((unsigned char)text[i])) {
        size_t j = i;
        if (text[j] == '-') j++;
        if (j >= len || !isdigit((unsigned char)text[j])) return 0;
        while (j < len && isdigit((unsigned char)text[j])) j++;
        if (j < len && text[j] == '.') {
            j++;
            while (j < len && isdigit((unsigned char)text[j])) j++;
        }
        if (j < len && (text[j] == 'e' || text[j] == 'E')) {
            j++;
            if (j < len && (text[j] == '+' || text[j] == '-')) j++;
            while (j < len && isdigit((unsigned char)text[j])) j++;
        }
        return j;
    }
    return 0;
}

/* ── lookup ─────────────────────────────────────────────────────────────── */

jvalue *jget(jvalue *obj, const char *key) {
    size_t i;
    if (!obj || obj->kind != J_OBJ) return NULL;
    for (i = 0; i < obj->nkeys; i++)
        if (!strcmp(obj->keys[i], key)) return obj->vals[i];
    return NULL;
}

const char *jstr(jvalue *v) { return (v && v->kind == J_STR) ? v->s : NULL; }

/* How many bytes form one Unicode scalar at s[0..avail). 0 means s[0] is not
 * the start of a well-formed sequence (overlong, surrogate, or truncated). */
static int utf8_one(const unsigned char *s, size_t avail) {
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

/* JSON string body. No surrounding quotes.
 * Invalid UTF-8 bytes are skipped, not copied: a strict parser rejects them
 * with HTTP 400. An escape or a multibyte character is written whole or not
 * at all, so a short buffer cannot end on '\' or on a split code point.
 * *in_used (if non-NULL) is how far `in` was read. It is short of strlen(in)
 * only when `out` filled up. Returns bytes written, not counting the NUL. */
size_t json_escape(const char *in, char *out, size_t outlen, size_t *in_used) {
    size_t i = 0, used = 0, avail;
    if (!in) in = "";
    avail = strlen(in);
    if (outlen == 0) {
        if (in_used) *in_used = 0;
        return 0;
    }
    while (i < avail) {
        int n = utf8_one((const unsigned char *)in + i, avail - i);
        int k;
        if (n <= 0) { i++; continue; }
        if (n == 1) {
            unsigned char c = (unsigned char)in[i];
            const char *rep = NULL;
            char buf[8];
            size_t rn;
            if (c == '"') rep = "\\\"";
            else if (c == '\\') rep = "\\\\";
            else if (c == '\n') rep = "\\n";
            else if (c == '\r') rep = "\\r";
            else if (c == '\t') rep = "\\t";
            else if (c < 0x20) {
                snprintf(buf, sizeof buf, "\\u%04x", c);
                rep = buf;
            }
            if (rep) {
                rn = strlen(rep);
                if (used + rn + 1 > outlen) break;
                for (k = 0; k < (int)rn; k++) out[used++] = rep[k];
            } else {
                if (used + 2 > outlen) break;
                out[used++] = (char)c;
            }
        } else {
            if (used + (size_t)n + 1 > outlen) break;
            for (k = 0; k < n; k++) out[used++] = in[i + (size_t)k];
        }
        i += (size_t)n;
    }
    out[used] = '\0';
    if (in_used) *in_used = i;
    return used;
}
double jnum(jvalue *v, double dflt) { return (v && v->kind == J_NUM) ? v->n : dflt; }
size_t jlen(jvalue *v) {
    if (!v) return 0;
    if (v->kind == J_ARR) return v->len;
    if (v->kind == J_OBJ) return v->nkeys;
    return 0;
}

/* ── self-test ──────────────────────────────────────────────────────────── */

static int failures = 0;
static void expect(int cond, const char *what) {
    if (!cond) { printf("FAIL %s\n", what); failures++; }
}

/*
 * Runs the reader's cases. Non-static so json_cli.c can call it across files
 * while the assertions stay next to the code they check.
 */
int json_run_selftest(void) {
    char err[128];
    jvalue *v;

    v = json_parse("{\"a\":1,\"b\":\"x\"}", strlen("{\"a\":1,\"b\":\"x\"}"), err, sizeof(err));
    expect(v != NULL, "object parses");
    if (v) {
        expect(jnum(jget(v, "a"), -1) == 1, "number field");
        expect(jstr(jget(v, "b")) && !strcmp(jstr(jget(v, "b")), "x"), "string field");
        expect(jget(v, "zz") == NULL, "missing key is NULL");
        jfree(v);
    }

    /* A REAL session line: nested, unicode escape, boolean. */
    {
        const char *line = "{\"role\":\"user\",\"n\":3,\"ok\":true,\"t\":[\"a\",\"b\"],\"s\":\"\\u4e2d\\u6587\"}";
        v = json_parse(line, strlen(line), err, sizeof(err));
        expect(v != NULL, "session-shaped line parses");
        if (v) {
            expect(jlen(jget(v, "t")) == 2, "array length");
            expect(jstr(jget(v, "s")) && !strcmp(jstr(jget(v, "s")), "\xe4\xb8\xad\xe6\x96\x87"), "\\u escape becomes UTF-8");
            expect(jget(v, "ok") && jget(v, "ok")->kind == J_BOOL && jget(v, "ok")->b == 1, "boolean");
            jfree(v);
        }
    }

    /* Malformed input must FAIL, not silently produce something. */
    expect(json_parse("{\"a\":}", strlen("{\"a\":}"), err, sizeof(err)) == NULL, "missing value rejected");
    expect(json_parse("{\"a\":1} junk", strlen("{\"a\":1} junk"), err, sizeof(err)) == NULL, "trailing content rejected");
    expect(json_parse("[1,2", strlen("[1,2"), err, sizeof(err)) == NULL, "unterminated array rejected");
    expect(json_parse("\"unterminated", strlen("\"unterminated"), err, sizeof(err)) == NULL, "unterminated string rejected");
    expect(json_parse("", strlen(""), err, sizeof(err)) == NULL, "empty input rejected");

    /* Escapes that matter for a transcript. */
    v = json_parse("\"a\\nb\\t\\\"c\\\\d\"", strlen("\"a\\nb\\t\\\"c\\\\d\""), err, sizeof(err));
    expect(v && !strcmp(jstr(v), "a\nb\t\"c\\d"), "escapes decode");
    jfree(v);

    /* Surrogate pair → 4-byte UTF-8. */
    v = json_parse("\"\\ud83d\\ude00\"", strlen("\"\\ud83d\\ude00\""), err, sizeof(err));
    expect(v && jstr(v) && (unsigned char)jstr(v)[0] == 0xF0, "surrogate pair becomes 4-byte UTF-8");
    jfree(v);

    /* Writer: quotes, controls, and a round trip through the reader. */
    {
        char esc[64], wrapped[80];
        size_t seen = 0;
        json_escape("a\"b\\c\n\x01", esc, sizeof esc, &seen);
        expect(seen == 7, "escape consumes the whole input");
        expect(!strcmp(esc, "a\\\"b\\\\c\\n\\u0001"), "quotes, slash, newline, control");
        snprintf(wrapped, sizeof wrapped, "\"%s\"", esc);
        v = json_parse(wrapped, strlen(wrapped), err, sizeof err);
        expect(v && jstr(v) && !strcmp(jstr(v), "a\"b\\c\n\x01"), "escape round-trips");
        jfree(v);
    }
    /* The HTTP 400: a stray E7 (not a continuation) before 如何. */
    {
        char raw[8], esc[32], wrapped[40];
        size_t seen = 0;
        raw[0] = (char)0xE7;
        raw[1] = (char)0xE5; raw[2] = (char)0xA6; raw[3] = (char)0x82; /* 如 */
        raw[4] = (char)0xE4; raw[5] = (char)0xBD; raw[6] = (char)0x95; /* 何 */
        raw[7] = 0;
        json_escape(raw, esc, sizeof esc, &seen);
        expect(seen == 7, "stray lead is consumed, not stuck");
        snprintf(wrapped, sizeof wrapped, "\"%s\"", esc);
        v = json_parse(wrapped, strlen(wrapped), err, sizeof err);
        expect(v && jstr(v) && !strcmp(jstr(v), "\xe5\xa6\x82\xe4\xbd\x95"), "stray lead dropped, 如何 kept");
        jfree(v);
    }
    /* A short buffer must not cut a code point or an escape in half. */
    {
        char tiny[4];
        size_t seen = 99;
        json_escape("\xe4\xb8\x80Z", tiny, 3, &seen);
        expect(tiny[0] == 0 && seen == 0, "3-byte character does not fit in 2 bytes");
        seen = 99;
        json_escape("\xe4\xb8\x80Z", tiny, 4, &seen);
        expect(!strcmp(tiny, "\xe4\xb8\x80") && seen == 3, "one complete character, then stop");
        seen = 99;
        json_escape("\"", tiny, 2, &seen);
        expect(tiny[0] == 0 && seen == 0, "quote escape is not a lone backslash");
    }

    expect(json_parse("\"\\ud800\"", strlen("\"\\ud800\""), err, sizeof err) == NULL,
           "lone high surrogate rejected");
    expect(json_parse("\"\\ude00\"", strlen("\"\\ude00\""), err, sizeof err) == NULL,
           "lone low surrogate rejected");
    expect(json_parse("1e", strlen("1e"), err, sizeof err) == NULL, "truncated exponent rejected");
    expect(json_parse("01", strlen("01"), err, sizeof err) == NULL, "leading zero rejected");
    {
        const char *span = " {\"a\":\"}\"} ";
        size_t e = json_value_end(span, strlen(span));
        expect(e == 11 && span[e - 1] == '}', "span closes after the brace inside the string");
        expect(json_value_end("{", 1) == 0, "unclosed object is not a span");
    }
    {
        char deep[80];
        int i;
        for (i = 0; i < 33; i++) deep[i] = '[';
        deep[33] = '1';
        for (i = 0; i < 33; i++) deep[34 + i] = ']';
        deep[67] = 0;
        expect(json_parse(deep, 67, err, sizeof err) == NULL, "nesting past 32 is rejected");
        expect(json_value_end(deep, 67) == 0, "a too-deep span is not a value");
    }

    /* Empty containers are legal. */
    v = json_parse("{}", strlen("{}"), err, sizeof(err)); expect(v != NULL, "{} parses"); jfree(v);
    v = json_parse("[]", strlen("[]"), err, sizeof(err)); expect(v != NULL, "[] parses"); jfree(v);

    /* Negative and exponent numbers. */
    /* Length must be strlen, not "looks about right" — passed 7 for a 6-char
     * literal on the first attempt, which handed the parser the NUL as input
     * and failed a correct parser. Caught by this very test. */
    v = json_parse("-1.5e3", strlen("-1.5e3"), err, sizeof(err));
    expect(v && jnum(v, 0) < -1499.0 && jnum(v, 0) > -1501.0, "negative exponent number");
    jfree(v);

    printf("%s\n", failures ? "SELFTEST FAILED" : "selftest ok");
    return failures == 0 ? 0 : 1;
}

/* The CLI (json_cli.c) owns `main`, the stdin reading, and the summary print, so
 * this library exposes only string→value entry points and keeps stdio out of its
 * interface. An earlier note blamed a cross-file `FILE *` prototype for
 * mis-lowering `%.4f`; unisacc 0.0.23 does not do that (measured 2026-10-04), so
 * the claim is gone and only the split remains. */
