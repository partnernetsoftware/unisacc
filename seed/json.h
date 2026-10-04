/* seed/json.h -- a bounded JSON reader for the C99 seed constructors (0.0.25 B2; shared with B4).
 *
 * Reads a whole document into a flat node array: objects keep their keys in document order
 * (Python dicts do, and the constructors' output depends on that order), arrays keep theirs.
 * Values: object, array, string, integer, true/false (JBOOL, ival 1/0) and null (JNULL).  Floats are
 * refused by name -- the deltas and facts these tools read have none.  Strings are decoded to bytes; a \u escape above
 * 0xFF becomes '?', which is what Python's .encode("latin-1", "replace") writes for it.
 * Every allocation is checked; an error prints where and exits 2.
 *
 *   JDoc d; jparse_file(&d, path);  d.n[0] is the root
 *   jparse_text(&d, text, len, label) parses a string in memory (B4: TSV action fields); jfree(&d)
 *   jkid(&d, i): first child, jnext(&d, i): next sibling, -1 at the end
 */
#ifndef SEED_JSON_H
#define SEED_JSON_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { JOBJ = 1, JARR, JSTR, JINT, JBOOL, JNULL };   /* JBOOL: ival 1 true / 0 false (B4: lex facts) */
typedef struct { int type; int kid, next, last; long key, klen; long str, slen; long long ival; } JNode;
typedef struct { JNode *n; long nn, cap; char *buf; long blen, bcap; const char *src; long len, pos; const char *path; } JDoc;

static void jdie(JDoc *d, const char *what) {
    fprintf(stderr, "json: %s: %s at byte %ld\n", d->path, what, d->pos);
    exit(2);
}
static long jnew(JDoc *d, int type) {
    if (d->nn == d->cap) {
        long c = d->cap ? d->cap * 2 : 4096;
        JNode *m = (JNode *)realloc(d->n, (size_t)c * sizeof(JNode));
        if (!m) jdie(d, "out of memory (nodes)");
        d->n = m; d->cap = c;
    }
    memset(&d->n[d->nn], 0, sizeof(JNode));
    d->n[d->nn].type = type; d->n[d->nn].kid = -1; d->n[d->nn].next = -1; d->n[d->nn].last = -1;
    return d->nn++;
}
static void jput(JDoc *d, int c) {
    if (d->blen == d->bcap) {
        long cap = d->bcap ? d->bcap * 2 : 65536;
        char *m = (char *)realloc(d->buf, (size_t)cap);
        if (!m) jdie(d, "out of memory (strings)");
        d->buf = m; d->bcap = cap;
    }
    d->buf[d->blen++] = (char)c;
}
static void jws(JDoc *d) {
    while (d->pos < d->len) {
        char c = d->src[d->pos];
        if (c == ' ' || c == '\n' || c == '\r' || c == '\t') d->pos++; else break;
    }
}
static int jhex(JDoc *d) {
    int v = 0, k;
    for (k = 0; k < 4; k++) {
        char c;
        if (d->pos >= d->len) jdie(d, "short \\u escape");
        c = d->src[d->pos++];
        v = v * 16 + (c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : (jdie(d, "bad \\u escape"), 0));
    }
    return v;
}
/* a string at d->pos (on the quote); returns its offset in buf, length in *len */
static long jstring(JDoc *d, long *len) {
    long at = d->blen;
    d->pos++;
    for (;;) {
        char c;
        if (d->pos >= d->len) jdie(d, "unterminated string");
        c = d->src[d->pos++];
        if (c == '"') break;
        if (c != '\\') { jput(d, (unsigned char)c); continue; }
        if (d->pos >= d->len) jdie(d, "unterminated escape");
        c = d->src[d->pos++];
        switch (c) {
        case '"': jput(d, '"'); break;
        case '\\': jput(d, '\\'); break;
        case '/': jput(d, '/'); break;
        case 'b': jput(d, 8); break;
        case 'f': jput(d, 12); break;
        case 'n': jput(d, 10); break;
        case 'r': jput(d, 13); break;
        case 't': jput(d, 9); break;
        case 'u': {
            int v = jhex(d);
            if (v >= 0xD800 && v < 0xDC00) {          /* a surrogate pair is one code point, one '?' */
                if (d->pos + 6 <= d->len && d->src[d->pos] == '\\' && d->src[d->pos + 1] == 'u') { d->pos += 2; (void)jhex(d); }
                v = 0x10000;
            }
            jput(d, v > 0xFF ? '?' : v);
            break;
        }
        default: jdie(d, "bad escape");
        }
    }
    *len = d->blen - at;
    return at;
}
static long jvalue(JDoc *d);
static void jappend(JDoc *d, long parent, long kid) {
    if (d->n[parent].last < 0) d->n[parent].kid = (int)kid; else d->n[d->n[parent].last].next = (int)kid;
    d->n[parent].last = (int)kid;
}
static long jvalue(JDoc *d) {
    long i;
    jws(d);
    if (d->pos >= d->len) jdie(d, "unexpected end");
    switch (d->src[d->pos]) {
    case '{':
        i = jnew(d, JOBJ); d->pos++; jws(d);
        if (d->pos < d->len && d->src[d->pos] == '}') { d->pos++; return i; }
        for (;;) {
            long k, klen, v;
            jws(d);
            if (d->pos >= d->len || d->src[d->pos] != '"') jdie(d, "object key expected");
            k = jstring(d, &klen); jws(d);
            if (d->pos >= d->len || d->src[d->pos] != ':') jdie(d, "':' expected");
            d->pos++;
            v = jvalue(d);
            d->n[v].key = k; d->n[v].klen = klen;
            jappend(d, i, v); jws(d);
            if (d->pos < d->len && d->src[d->pos] == ',') { d->pos++; continue; }
            if (d->pos < d->len && d->src[d->pos] == '}') { d->pos++; return i; }
            jdie(d, "',' or '}' expected");
        }
    case '[':
        i = jnew(d, JARR); d->pos++; jws(d);
        if (d->pos < d->len && d->src[d->pos] == ']') { d->pos++; return i; }
        for (;;) {
            long v = jvalue(d);
            jappend(d, i, v); jws(d);
            if (d->pos < d->len && d->src[d->pos] == ',') { d->pos++; continue; }
            if (d->pos < d->len && d->src[d->pos] == ']') { d->pos++; return i; }
            jdie(d, "',' or ']' expected");
        }
    case '"': {
        long len, at = jstring(d, &len);
        i = jnew(d, JSTR); d->n[i].str = at; d->n[i].slen = len;
        return i;
    }
    case 't': case 'f': case 'n': {
        static const char *lit[3] = {"true", "false", "null"};
        int w = d->src[d->pos] == 't' ? 0 : d->src[d->pos] == 'f' ? 1 : 2;
        long l = (long)strlen(lit[w]);
        if (d->pos + l > d->len || memcmp(d->src + d->pos, lit[w], (size_t)l) != 0) jdie(d, "bad literal");
        d->pos += l;
        i = jnew(d, w == 2 ? JNULL : JBOOL); d->n[i].ival = w == 0;
        return i;
    }
    default: {
        long long v = 0; int neg = 0, digits = 0;
        if (d->src[d->pos] == '-') { neg = 1; d->pos++; }
        while (d->pos < d->len && d->src[d->pos] >= '0' && d->src[d->pos] <= '9') {
            if (v > 922337203685477580LL) jdie(d, "integer overflow");
            v = v * 10 + (d->src[d->pos++] - '0'); digits++;
        }
        if (!digits) jdie(d, "value expected (floats are not used here)");
        if (d->pos < d->len && (d->src[d->pos] == '.' || d->src[d->pos] == 'e' || d->src[d->pos] == 'E')) jdie(d, "float not supported");
        i = jnew(d, JINT); d->n[i].ival = neg ? -v : v;
        return i;
    }
    }
}
/* parse text[0..len) (not copied: it must outlive the document); label names it in errors */
static void jparse_text(JDoc *d, const char *text, long len, const char *label) {
    memset(d, 0, sizeof(*d)); d->path = label; d->src = text; d->len = len;
    if (jvalue(d) != 0) jdie(d, "root is not node 0");
    jws(d);
    if (d->pos != d->len) jdie(d, "trailing data");
}
static void jfree(JDoc *d) { free(d->n); free(d->buf); d->n = 0; d->buf = 0; }
static void jparse_file(JDoc *d, const char *path) {
    FILE *f = fopen(path, "rb");
    long cap = 1 << 20, n = 0;
    char *src;
    memset(d, 0, sizeof(*d)); d->path = path;
    if (!f) { fprintf(stderr, "json: cannot open %s\n", path); exit(2); }
    src = (char *)malloc((size_t)cap);
    if (!src) jdie(d, "out of memory (source)");
    for (;;) {
        size_t got;
        if (n == cap) {
            char *m = (char *)realloc(src, (size_t)(cap * 2));
            if (!m) jdie(d, "out of memory (source)");
            src = m; cap *= 2;
        }
        got = fread(src + n, 1, (size_t)(cap - n), f);
        if (got == 0) break;
        n += (long)got;
    }
    fclose(f);
    d->src = src; d->len = n;
    if (jvalue(d) != 0) jdie(d, "root is not node 0");
    jws(d);
    if (d->pos != d->len) jdie(d, "trailing data");
}
static int jkid(JDoc *d, long i) { return d->n[i].kid; }
static int jnext(JDoc *d, long i) { return d->n[i].next; }
static const char *jkey(JDoc *d, long i) { return d->buf + d->n[i].key; }
static int jkeyis(JDoc *d, long i, const char *s) { return d->n[i].klen == (long)strlen(s) && memcmp(d->buf + d->n[i].key, s, (size_t)d->n[i].klen) == 0; }
static int jstris(JDoc *d, long i, const char *s) { return d->n[i].type == JSTR && d->n[i].slen == (long)strlen(s) && memcmp(d->buf + d->n[i].str, s, (size_t)d->n[i].slen) == 0; }
static long jget(JDoc *d, long obj, const char *key) {
    long k;
    for (k = jkid(d, obj); k >= 0; k = jnext(d, k)) if (jkeyis(d, k, key)) return k;
    return -1;
}
#endif
