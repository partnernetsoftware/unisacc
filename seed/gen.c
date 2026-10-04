/* C99 seed graph constructor, first supported stage: prune.
   The manifest/TSV syntax and graph ordering are specified in seed/GEN-DESIGN.md.
   Other stages and DSL operations fail by name until implemented. */
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "json.h"

typedef struct { char *key, *target; int seq; } Edge;
typedef struct { char *name; char mode; Edge *edge; size_t n, cap; } State;
typedef struct { State *state; size_t n, cap; char **seq; size_t ns, cs; char **labels; size_t nl, cl; } Graph;
typedef struct { int key; char *target, *actions; } Rule;
typedef struct { char *name; Rule *rule; size_t n, cap; char *def_target, *def_actions; } RuleState;
typedef struct { RuleState *state; size_t n, cap; } RuleSet;
typedef struct { char *s; size_t n, cap; } Buffer;
/* Values preserve the insertion order of object keys, as assemble.load_facts does. */
typedef struct Value Value;
typedef struct { char *key; Value *value; } Member;
struct Value {
    int kind; /* JOBJ/JARR/JSTR/JINT */
    char *s; long long number;
    Member *items; size_t n, cap;
};

static void die(const char *why) { fprintf(stderr, "seed-gen: %s\n", why); exit(1); }
static void *grow(void *p, size_t n, size_t unit) {
    if (n > SIZE_MAX / unit) die("size overflow");
    p = realloc(p, n * unit); if (!p) die("out of memory"); return p;
}
static char *copy(const char *s) {
    size_t n = strlen(s) + 1; char *p = malloc(n);
    if (!p) die("out of memory"); memcpy(p, s, n); return p;
}
static char *copy_n(const char *s, size_t n) {
    char *p = grow(NULL, n + 1, 1);
    memcpy(p, s, n); p[n] = 0; return p;
}
static void buf_add(Buffer *b, const char *s, size_t n) {
    if (n > SIZE_MAX - b->n - 1) die("buffer size overflow");
    if (b->n + n + 1 > b->cap) {
        size_t cap = b->cap ? b->cap : 128;
        while (cap < b->n + n + 1) { if (cap > SIZE_MAX / 2) die("buffer too large"); cap *= 2; }
        b->s = grow(b->s, cap, 1); b->cap = cap;
    }
    memcpy(b->s + b->n, s, n); b->n += n; b->s[b->n] = 0;
}
static void buf_char(Buffer *b, char c) { buf_add(b, &c, 1); }
static void buf_quote(Buffer *b, const char *s) {
    const unsigned char *p = (const unsigned char *)s; char x[7];
    buf_char(b, '"');
    for (; *p; p++) {
        if (*p == '"' || *p == '\\') { buf_char(b, '\\'); buf_char(b, (char)*p); }
        else if (*p < 32) { snprintf(x, sizeof(x), "\\u%04x", (unsigned)*p); buf_add(b, x, 6); }
        else buf_char(b, (char)*p);
    }
    buf_char(b, '"');
}
static void buf_value(Buffer *b, const Value *v) {
    size_t i; char n[64];
    switch (v->kind) {
    case JSTR: buf_quote(b, v->s); break;
    case JINT: snprintf(n, sizeof(n), "%lld", v->number); buf_add(b, n, strlen(n)); break;
    case JOBJ: case JARR:
        buf_char(b, v->kind == JOBJ ? '{' : '[');
        for (i = 0; i < v->n; i++) {
            if (i) buf_char(b, ',');
            if (v->kind == JOBJ) { buf_quote(b, v->items[i].key); buf_char(b, ':'); }
            buf_value(b, v->items[i].value);
        }
        buf_char(b, v->kind == JOBJ ? '}' : ']'); break;
    default: die("unknown value kind");
    }
}
static Value *value_new(int kind) {
    Value *v = grow(NULL, 1, sizeof(*v));
    memset(v, 0, sizeof(*v)); v->kind = kind; return v;
}
static Value *value_string(const char *s) {
    Value *v = value_new(JSTR); v->s = copy(s); return v;
}
static void value_put(Value *parent, const char *key, Value *child) {
    size_t i;
    if (parent->kind != JOBJ && parent->kind != JARR) die("value is not a container");
    if (parent->kind == JOBJ) {
        if (!key) die("missing object key");
        for (i = 0; i < parent->n; i++) if (!strcmp(parent->items[i].key, key)) {
            parent->items[i].value = child; return;
        }
    }
    if (parent->n == parent->cap) {
        parent->cap = parent->cap ? parent->cap * 2 : 8;
        parent->items = grow(parent->items, parent->cap, sizeof(*parent->items));
    }
    parent->items[parent->n].key = key ? copy(key) : NULL;
    parent->items[parent->n++].value = child;
}
static Value *value_get(Value *v, const char *key) {
    size_t i;
    if (!v || v->kind != JOBJ) return NULL;
    for (i = 0; i < v->n; i++) if (!strcmp(v->items[i].key, key)) return v->items[i].value;
    return NULL;
}
static Value *value_from_json(JDoc *d, int ix) {
    JNode *n = &d->n[ix]; Value *v = value_new(n->type); int child;
    if (n->type == JSTR) v->s = copy_n(d->buf + n->str, (size_t)n->slen);
    else if (n->type == JINT) v->number = n->ival;
    else if (n->type == JOBJ || n->type == JARR) {
        for (child = jkid(d, ix); child >= 0; child = jnext(d, child)) {
            JNode *c = &d->n[child];
            char *key = n->type == JOBJ ? copy_n(d->buf + c->key, (size_t)c->klen) : NULL;
            value_put(v, key, value_from_json(d, child)); free(key);
        }
    } else die("unsupported JSON value");
    return v;
}
static Value *value_json(const char *s, const char *label) {
    JDoc d; Value *v;
    jparse_text(&d, s, (long)strlen(s), label);
    v = value_from_json(&d, 0); jfree(&d); return v;
}
static const char *value_text(Value *v) {
    if (!v || v->kind != JSTR) die("expected string value");
    return v->s;
}
static void expanded_action(Buffer *b, Value *action, Value *bindings, int key) {
    size_t i;
    if (!action || action->kind != JARR || !action->n) die("invalid action");
    buf_char(b, '[');
    for (i = 0; i < action->n; i++) {
        Value *v = action->items[i].value;
        if (i) buf_char(b, ',');
        if (v->kind == JARR && v->n == 2) {
            const char *tag = value_text(v->items[0].value);
            Value *arg = v->items[1].value;
            if (!strcmp(tag, "constant")) {
                Value *bound = value_get(bindings, value_text(arg));
                if (!bound) die("unknown constant binding");
                buf_value(b, bound); continue;
            }
            if (!strcmp(tag, "observation")) {
                long long shift = arg->number, number;
                Value q = {0};
                if (arg->kind != JINT || shift < 0 || shift > 63 || key < 0) die("invalid observation substitution");
                number = ((unsigned long long)key) << shift;
                q.kind = JINT; q.number = number; buf_value(b, &q); continue;
            }
        }
        buf_value(b, v);
    }
    buf_char(b, ']');
}
static char *expand_actions(const char *raw, Value *bindings, Value *sequences, int key) {
    Value *v = value_json(raw, "rule actions"); Buffer b = {0}; size_t i, n = 0;
    if (v->kind != JARR) die("action sequence is not a list");
    buf_char(&b, '[');
    for (i = 0; i < v->n; i++) {
        Value *a = v->items[i].value; size_t j;
        if (!a || a->kind != JARR || !a->n) die("invalid rule action");
        if (!strcmp(value_text(a->items[0].value), "@")) {
            Value *seqv;
            if (a->n != 2) die("invalid named sequence");
            seqv = value_get(sequences, value_text(a->items[1].value));
            if (!seqv || seqv->kind != JARR) die("unknown named sequence");
            for (j = 0; j < seqv->n; j++) {
                if (n++) buf_char(&b, ',');
                expanded_action(&b, seqv->items[j].value, bindings, key);
            }
        } else {
            if (n++) buf_char(&b, ',');
            expanded_action(&b, a, bindings, key);
        }
    }
    buf_char(&b, ']'); return b.s;
}
static char *line(FILE *f) {
    size_t n = 0, cap = 256; char *s = grow(NULL, cap, 1); int c;
    while ((c = fgetc(f)) != EOF && c != '\n') {
        if (n + 1 >= cap) { if (cap > SIZE_MAX / 2) die("line too long"); cap *= 2; s = grow(s, cap, 1); }
        s[n++] = (char)c;
    }
    if (c == EOF && n == 0) { free(s); return NULL; }
    if (n && s[n - 1] == '\r') n--;
    s[n] = 0; return s;
}
static int key_number(const char *s) {
    char *end; long n;
    errno = 0; n = strtol(s, &end, 10);
    if (errno || end == s || *end || n < 0 || n > 256) die("invalid observation key");
    return (int)n;
}
static void number_text(int n, char out[16]) { if (snprintf(out, 16, "%d", n) < 0) die("number formatting"); }
static void quoted(FILE *f, const char *s) {
    const unsigned char *p = (const unsigned char *)s; fputc('"', f);
    for (; *p; p++) {
        if (*p == '"' || *p == '\\') { fputc('\\', f); fputc(*p, f); }
        else if (*p < 32) { if (fprintf(f, "\\u%04x", (unsigned)*p) < 0) die("write failed"); }
        else fputc(*p, f);
    }
    fputc('"', f);
}
static void value_write(FILE *f, const Value *v) {
    size_t i;
    switch (v->kind) {
    case JSTR: quoted(f, v->s); break;
    case JINT: fprintf(f, "%lld", v->number); break;
    case JOBJ: case JARR:
        fputc(v->kind == JOBJ ? '{' : '[', f);
        for (i = 0; i < v->n; i++) {
            if (i) fputc(',', f);
            if (v->kind == JOBJ) { quoted(f, v->items[i].key); fputc(':', f); }
            value_write(f, v->items[i].value);
        }
        fputc(v->kind == JOBJ ? '}' : ']', f); break;
    default: die("unknown value kind");
    }
}
static int fields_tab(char *s, char **field, int max) {
    int n = 0; char *p = s;
    while (n < max) {
        char *tab = strchr(p, '\t');
        field[n++] = p;
        if (!tab) return n;
        *tab = 0; p = tab + 1;
    }
    if (strchr(p, '\t')) die("too many TSV fields");
    return n;
}
static Value *fact_cell(const char *type, const char *text) {
    if (!strcmp(type, "json")) return value_json(text, "fact JSON cell");
    if (!strcmp(type, "int")) {
        char *end; long long n; Value *v;
        errno = 0; n = strtoll(text, &end, 10);
        if (errno || end == text || *end) die("invalid fact integer");
        v = value_new(JINT); v->number = n; return v;
    }
    if (!strcmp(type, "str")) {
        char *out = grow(NULL, strlen(text) + 1, 1); size_t j = 0, i;
        for (i = 0; text[i]; i++) {
            if (text[i] == '\\' && text[i + 1]) {
                i++; out[j++] = text[i] == 't' ? '\t' : text[i] == 'n' ? '\n' : text[i];
            } else out[j++] = text[i];
        }
        out[j] = 0;
        { Value *v = value_string(out); free(out); return v; }
    }
    die("unknown fact cell type"); return NULL;
}
static Value *header_cell(const char *text) {
    unsigned char c = (unsigned char)text[0];
    if (c == '[' || c == '{' || c == '"' || (c >= '0' && c <= '9') ||
        (c == '-' && text[1] >= '0' && text[1] <= '9'))
        return value_json(text, "header fact cell");
    return value_string(text);
}
static Value *load_fact(const char *stem) {
    char path[1024]; FILE *f; char *s; Value *root = value_new(JOBJ);
    char *header[128] = {0}, *types[128] = {0}; int nh = 0, typed = -1;
    Value *table = NULL; char *table_name = NULL;
    if (snprintf(path, sizeof(path), "exec/facts/%s.tsv", stem) >= (int)sizeof(path)) die("fact path too long");
    f = fopen(path, "rb"); if (!f) die("cannot open fact table");
    while ((s = line(f))) {
        char *field[128]; int n, i;
        if (!*s) { free(s); continue; }
        if (typed < 0 && s[0] != '#') typed = s[0] == '=' || s[0] == '@' || s[0] == '\t' ? 1 : 0;
        if (s[0] == '#') {
            if (typed < 0 && nh == 0 && s[1] == ' ') {
                char *h = copy(s + 2); nh = fields_tab(h, field, 128);
                for (i = 0; i < nh; i++) header[i] = copy(field[i]); free(h);
            }
            free(s); continue;
        }
        n = fields_tab(s, field, 128);
        if (!typed) {
            Value *row = value_new(JOBJ);
            if (n != nh || nh == 0) die("header fact column count");
            if (!table) { table = value_new(JARR); value_put(root, stem, table); }
            for (i = 0; i < n; i++) value_put(row, header[i], header_cell(field[i]));
            value_put(table, NULL, row);
        } else if (field[0][0] == '=') {
            if (n != 3) die("scalar fact column count");
            value_put(root, field[0] + 1, fact_cell(field[1], field[2]));
        } else if (field[0][0] == '@') {
            if (n < 2) die("fact table header count");
            table_name = copy(field[0] + 1); table = value_new(JARR);
            value_put(root, table_name, table); nh = n - 1;
            for (i = 0; i < nh; i++) {
                char *colon = strchr(field[i + 1], ':'); if (!colon) die("fact column type missing");
                *colon = 0; header[i] = copy(field[i + 1]); types[i] = copy(colon + 1);
            }
        } else {
            Value *row = value_new(JOBJ);
            if (!table || field[0][0] || n != nh + 1) die("fact table row count");
            for (i = 0; i < nh; i++) value_put(row, header[i], fact_cell(types[i], field[i + 1]));
            value_put(table, NULL, row);
        }
        free(s);
    }
    if (ferror(f) || fclose(f)) die("fact read failed");
    if (typed < 0) die("empty fact table");
    if (!typed && table && nh == 2 && !strcmp(header[0], "name") && !strcmp(header[1], "value")) {
        Value *map = value_new(JOBJ); size_t i;
        value_put(root, copy_n(stem, strlen(stem)), table);
        for (i = 0; i < table->n; i++) {
            Value *row = table->items[i].value, *name = value_get(row, "name"), *val = value_get(row, "value");
            if (!name || name->kind != JSTR) die("invalid name/value fact");
            value_put(map, name->s, val);
            if (!value_get(root, name->s)) value_put(root, name->s, val);
        }
        { char key[1024]; if (snprintf(key, sizeof(key), "%s!", stem) >= (int)sizeof(key)) die("fact key too long"); value_put(root, key, map); }
    }
    return root;
}
static Value *load_facts_expr(const char *expr) {
    Value *merged = value_new(JOBJ); char *parts, *p;
    if (!strcmp(expr, "-") || !*expr) return merged;
    parts = copy(expr); p = parts;
    while (*p) {
        char *plus = strchr(p, '+'); Value *one; size_t i;
        if (plus) *plus = 0;
        one = load_fact(p);
        for (i = 0; i < one->n; i++) value_put(merged, one->items[i].key, one->items[i].value);
        if (!plus) break; p = plus + 1;
    }
    free(parts); return merged;
}
static Value *value_path(Value *root, const char *path) {
    char *parts = copy(path), *p = parts; Value *v = root;
    while (*p) {
        char *dot = strchr(p, '.');
        if (dot) *dot = 0;
        if (v && v->kind == JARR) {
            char *end; unsigned long i = strtoul(p, &end, 10);
            if (end == p || *end || i >= v->n) die("invalid fact list index");
            v = v->items[i].value;
        } else v = value_get(v, p);
        if (!v) die("unknown fact path");
        if (!dot) break; p = dot + 1;
    }
    free(parts); return v;
}
static unsigned long fresh_count;
static char *fresh_label(const char *owner, const char *kind) {
    char name[1024]; const char *dot = strchr(owner, '.');
    size_t prefix = dot ? (size_t)(dot - owner) : strlen(owner);
    if (++fresh_count > 1000000) die("fresh label limit");
    if (snprintf(name, sizeof(name), "%.*s.%s%lu", (int)prefix, owner, kind, fresh_count) >= (int)sizeof(name))
        die("fresh label too long");
    return copy(name);
}
static int has_flag(int argc, char **argv, const char *name) {
    int i; char flag[128];
    if (snprintf(flag, sizeof(flag), "--%s", name) >= (int)sizeof(flag)) die("flag too long");
    for (i = 0; i < argc; i++) if (!strcmp(argv[i], flag)) return 1;
    return 0;
}
static int when_true(const char *when, int argc, char **argv) {
    char *parts, *p;
    if (!strcmp(when, "-") || !*when) return 1;
    parts = copy(when); p = parts;
    while (*p) {
        char *amp = strchr(p, '&'); int neg = *p == '!', value;
        if (amp) *amp = 0;
        value = has_flag(argc, argv, p + neg);
        if (value == neg) { free(parts); return 0; }
        if (!amp) break; p = amp + 1;
    }
    free(parts); return 1;
}
static char *interpolate(const char *text, Value *env) {
    size_t cap = strlen(text) + 64, n = 0, i; char *out = grow(NULL, cap, 1);
    for (i = 0; text[i]; i++) {
        const char *part = text + i; size_t len = 1; char number[64];
        if (text[i] == '{') {
            const char *end = strchr(text + i + 1, '}'); Value *v; char *key;
            if (!end) die("unterminated interpolation");
            key = copy_n(text + i + 1, (size_t)(end - text - i - 1));
            v = value_get(env, key); free(key);
            if (!v || (v->kind != JSTR && v->kind != JINT)) die("unknown interpolation fact");
            if (v->kind == JINT) { snprintf(number, sizeof(number), "%lld", v->number); part = number; }
            else part = v->s;
            len = strlen(part); i = (size_t)(end - text);
        }
        if (n + len + 1 > cap) {
            while (n + len + 1 > cap) { if (cap > SIZE_MAX / 2) die("interpolation too long"); cap *= 2; }
            out = grow(out, cap, 1);
        }
        memcpy(out + n, part, len); n += len;
    }
    out[n] = 0; return out;
}
static Value *fresh_bindings(Value *opts, Value *facts) {
    Value *bindings = value_new(JOBJ), *specs = value_get(opts, "freshrows");
    size_t s;
    if (!specs) return bindings;
    if (specs->kind != JARR) die("unsupported freshrows shape");
    for (s = 0; s < specs->n; s++) {
        Value *spec = specs->items[s].value;
        Value *rows = value_path(facts, value_text(value_get(spec, "over")));
        Value *where = value_get(spec, "where"); size_t i;
        if (rows->kind != JARR) die("freshrows source is not a list");
        for (i = 0; i < rows->n; i++) {
            Value *row = rows->items[i].value, *ctx = value_new(JOBJ);
            char *key, *owner, *kind, *label; size_t j; int match = 1;
            if (row->kind != JOBJ) die("freshrows item is not an object");
            for (j = 0; j < facts->n; j++) value_put(ctx, facts->items[j].key, facts->items[j].value);
            for (j = 0; j < row->n; j++) value_put(ctx, row->items[j].key, row->items[j].value);
            if (where) {
                if (where->kind != JOBJ) die("freshrows where is not an object");
                for (j = 0; j < where->n; j++) {
                    Value *actual = value_get(row, where->items[j].key);
                    char *expect = interpolate(value_text(where->items[j].value), ctx);
                    if (!actual || actual->kind != JSTR || strcmp(actual->s, expect)) match = 0;
                    free(expect);
                }
            }
            if (!match) continue;
            key = interpolate(value_text(value_get(spec, "key")), ctx);
            owner = interpolate(value_text(value_get(spec, "owner")), ctx);
            kind = interpolate(value_text(value_get(spec, "kind")), ctx);
            label = fresh_label(owner, kind);
            value_put(bindings, key, value_string(label));
            free(key); free(owner); free(kind); free(label);
        }
    }
    return bindings;
}
static void direct_bindings(Value *bindings, const char *text, Value *facts) {
    char *parts = copy(text), *p = parts;
    if (!strcmp(text, "-") || !*text) { free(parts); return; }
    while (*p) {
        char *comma = strchr(p, ','), *eq = strchr(p, '='); Value *v;
        if (comma) *comma = 0;
        if (!eq || eq == p || !eq[1]) die("invalid binding cell");
        *eq = 0;
        if (!strncmp(eq + 1, "@str:", 5)) {
            char *s = interpolate(eq + 6, facts);
            v = value_string(s); free(s);
        } else v = value_path(facts, eq + 1);
        value_put(bindings, p, v);
        if (!comma) break; p = comma + 1;
    }
    free(parts);
}
static void let_bind(Value *env, const char *bind) {
    char *parts = copy(bind), *p = parts;
    if (!strcmp(bind, "-") || !*bind) { free(parts); return; }
    while (*p) {
        char *comma = strchr(p, ','), *eq = strchr(p, '='); Value *v; char *s;
        if (comma) *comma = 0;
        if (!eq || eq == p || eq[1] == 0) die("invalid let binding");
        *eq = 0;
        if (strncmp(eq + 1, "@str:", 5)) die("unsupported let value");
        s = interpolate(eq + 6, env); v = value_string(s); free(s);
        value_put(env, p, v);
        if (!comma) break; p = comma + 1;
    }
    free(parts);
}
/* Diagnostic entry for the first reusable manifest feature. It never skips an
   operation in a real graph build: callers explicitly request only let rows. */
static void inspect_lets(const char *stage, const char *outpath, int argc, char **argv) {
    char path[1024]; FILE *f, *out; char *s; Value *env = value_new(JOBJ);
    if (snprintf(path, sizeof(path), "exec/%s/gen-manifest.tsv", stage) >= (int)sizeof(path)) die("manifest path too long");
    f = fopen(path, "rb"); if (!f) die("cannot open manifest");
    while ((s = line(f))) {
        char *field[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, field, 9); if (n != 9) die("manifest column count");
        if (!strcmp(field[0], "let") && when_true(field[3], argc, argv)) {
            if (strcmp(field[4], "-") || strcmp(field[6], "-") || strcmp(field[8], "-"))
                die("unsupported let facts/options");
            let_bind(env, field[7]);
        }
        free(s);
    }
    if (ferror(f) || fclose(f)) die("manifest read failed");
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    value_write(out, env); if (fclose(out)) die("output close failed");
}
static void inspect_fresh(const char *stage, const char *stem, const char *outpath, const char *initial) {
    char path[1024], *end; FILE *f, *out; char *s; int found = 0;
    fresh_count = strtoul(initial, &end, 10);
    if (end == initial || *end || fresh_count > 1000000) die("invalid initial fresh count");
    if (snprintf(path, sizeof(path), "exec/%s/gen-manifest.tsv", stage) >= (int)sizeof(path)) die("manifest path too long");
    f = fopen(path, "rb"); if (!f) die("cannot open manifest");
    while ((s = line(f))) {
        char *field[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, field, 9); if (n != 9) die("manifest column count");
        if (!strcmp(field[0], "rows") && !strcmp(field[1], stem)) {
            Value *facts = load_facts_expr(field[4]), *opts = value_json(field[8], "manifest options");
            Value *bindings = fresh_bindings(opts, facts);
            direct_bindings(bindings, field[7], facts);
            if (++found > 1) die("ambiguous fresh manifest row");
            out = fopen(outpath, "wb"); if (!out) die("cannot open output");
            value_write(out, bindings); if (fclose(out)) die("output close failed");
        }
        free(s);
    }
    if (ferror(f) || fclose(f)) die("manifest read failed");
    if (!found) die("fresh manifest row not found");
}
static RuleState *rule_state(RuleSet *r, const char *name) {
    size_t i; RuleState *s;
    for (i = 0; i < r->n; i++) if (!strcmp(r->state[i].name, name)) return &r->state[i];
    if (r->n == r->cap) { r->cap = r->cap ? r->cap * 2 : 64; r->state = grow(r->state, r->cap, sizeof(*r->state)); }
    s = &r->state[r->n++]; memset(s, 0, sizeof(*s)); s->name = copy(name); return s;
}
static void rule_add(RuleState *s, int key, const char *target, const char *actions) {
    size_t i; Rule *r;
    for (i = 0; i < s->n; i++) if (s->rule[i].key == key) die("overlapping observation rules");
    if (s->n == s->cap) { s->cap = s->cap ? s->cap * 2 : 8; s->rule = grow(s->rule, s->cap, sizeof(*s->rule)); }
    r = &s->rule[s->n++]; r->key = key; r->target = copy(target); r->actions = copy(actions);
}
static void rule_keys(RuleState *s, const char *keys, const char *target, const char *actions) {
    char *work = copy(keys), *part = work;
    while (*part) {
        char *comma = strchr(part, ','), *dash; int lo, hi, k;
        if (comma) *comma = 0;
        dash = strchr(part, '-');
        if (dash) { *dash = 0; lo = key_number(part); hi = key_number(dash + 1); }
        else lo = hi = key_number(part);
        if (lo > hi) die("descending observation range");
        for (k = lo; k <= hi; k++) rule_add(s, k, target, actions);
        if (!comma) break; part = comma + 1;
    }
    free(work);
}
static int seq(Graph *g, const char *actions) {
    size_t i;
    for (i = 0; i < g->ns; i++) if (!strcmp(g->seq[i], actions)) return (int)i;
    if (g->ns >= INT_MAX) die("too many sequences");
    if (g->ns == g->cs) { g->cs = g->cs ? g->cs * 2 : 64; g->seq = grow(g->seq, g->cs, sizeof(*g->seq)); }
    g->seq[g->ns] = copy(actions); return (int)g->ns++;
}
static State *graph_state(Graph *g, const char *name, char mode) {
    size_t i; State *s;
    for (i = 0; i < g->n; i++) if (!strcmp(g->state[i].name, name)) {
        if (g->state[i].mode != mode) die("state mode conflict");
        return &g->state[i];
    }
    if (g->n == g->cap) { g->cap = g->cap ? g->cap * 2 : 64; g->state = grow(g->state, g->cap, sizeof(*g->state)); }
    s = &g->state[g->n++]; memset(s, 0, sizeof(*s)); s->name = copy(name); s->mode = mode; return s;
}
static int edge_has(const State *s, const char *key) {
    size_t i; for (i = 0; i < s->n; i++) if (!strcmp(s->edge[i].key, key)) return 1; return 0;
}
static void label_add(Graph *g, const char *name) {
    size_t i;
    for (i = 0; i < g->nl; i++) if (!strcmp(g->labels[i], name)) return;
    if (g->nl == g->cl) { g->cl = g->cl ? g->cl * 2 : 32; g->labels = grow(g->labels, g->cl, sizeof(*g->labels)); }
    g->labels[g->nl++] = copy(name);
}
static void action_labels(Graph *g, const char *actions) {
    JDoc d; int a;
    jparse_text(&d, actions, (long)strlen(actions), "prune action");
    if (d.n[0].type != JARR) die("action sequence is not an array");
    for (a = jkid(&d, 0); a >= 0; a = jnext(&d, a)) {
        int op, arg;
        if (d.n[a].type != JARR || (op = jkid(&d, a)) < 0 || d.n[op].type != JSTR)
            die("invalid action entry");
        if (!jstris(&d, op, "PUSH")) continue;
        arg = jnext(&d, op);
        if (arg < 0 || d.n[arg].type != JSTR || jnext(&d, arg) >= 0)
            die("invalid PUSH action");
        {
            size_t n = (size_t)d.n[arg].slen; char *name = grow(NULL, n + 1, 1);
            memcpy(name, d.buf + d.n[arg].str, n); name[n] = 0;
            label_add(g, name); free(name);
        }
    }
    jfree(&d);
}
static int label_compare(const void *a, const void *b) {
    return strcmp(*(char *const *)a, *(char *const *)b);
}
static void edge_add(Graph *g, const char *name, char mode, const char *key, const char *target, const char *actions) {
    State *s = graph_state(g, name, mode); Edge *e;
    action_labels(g, actions);
    if (edge_has(s, key)) return;
    if (s->n == s->cap) { s->cap = s->cap ? s->cap * 2 : 32; s->edge = grow(s->edge, s->cap, sizeof(*s->edge)); }
    e = &s->edge[s->n++]; e->key = copy(key); e->target = copy(target); e->seq = seq(g, actions);
}
static void install_file(Graph *g, const char *path, char mode) {
    FILE *f = fopen(path, "rb"); char *s; RuleSet rules = {0};
    if (!f) die("cannot open rule file");
    while ((s = line(f))) {
        char *field[5], *p = s; int i; RuleState *st;
        if (!*s || *s == '#') { free(s); continue; }
        for (i = 0; i < 4; i++) { char *tab = strchr(p, '\t'); if (!tab) die("rule column count"); *tab = 0; field[i] = p; p = tab + 1; }
        if (strchr(p, '\t')) die("rule column count"); field[4] = p;
        if (strcmp(field[0], "main")) { free(s); continue; }
        if (!*field[1] || !*field[3] || field[4][0] != '[') die("invalid prune rule");
        st = rule_state(&rules, field[1]);
        if (!strcmp(field[2], "*")) {
            if (st->def_target) die("repeated default rule");
            st->def_target = copy(field[3]); st->def_actions = copy(field[4]);
        } else rule_keys(st, field[2], field[3], field[4]);
        free(s);
    }
    if (ferror(f) || fclose(f)) die("rule read failed");
    for (size_t i = 0; i < rules.n; i++) {
        RuleState *st = &rules.state[i]; char key[16];
        for (size_t j = 0; j < st->n; j++) {
            number_text(st->rule[j].key, key);
            edge_add(g, st->name, mode, key, st->rule[j].target, st->rule[j].actions);
        }
        for (int k = 0; k <= 256; k++) {
            int found = 0;
            for (size_t j = 0; j < st->n; j++) if (st->rule[j].key == k) { found = 1; break; }
            if (!found) {
                if (!st->def_target) die("incomplete rule state");
                number_text(k, key); edge_add(g, st->name, mode, key, st->def_target, st->def_actions);
            }
        }
    }
}
static void output(FILE *f, const Graph *g);
/* Four-column form shared by opt/lower/parse rows. The caller supplies only
   source facts and generated bindings; the row reader owns the rule order. */
static const char *bound_name(const char *s, Value *bindings) {
    return *s == '$' ? value_text(value_get(bindings, s + 1)) : s;
}
static void install_plain(Graph *g, const char *path, char mode,
                          Value *bindings, Value *sequences) {
    FILE *f = fopen(path, "rb"); char *s; RuleSet rules = {0};
    if (!f) die("cannot open rule file");
    while ((s = line(f))) {
        char *field[4]; int n; RuleState *st;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, field, 4); if (n != 4) die("plain rule column count");
        if (!*field[0] || !*field[2]) die("empty rule state or target");
        st = rule_state(&rules, bound_name(field[0], bindings));
        if (!strcmp(field[1], "*")) {
            if (st->def_target) die("repeated default rule");
            st->def_target = copy(bound_name(field[2], bindings));
            st->def_actions = copy(field[3]);
        } else rule_keys(st, field[1], bound_name(field[2], bindings), field[3]);
        free(s);
    }
    if (ferror(f) || fclose(f)) die("rule read failed");
    if (!rules.n) die("empty rule file");
    for (size_t i = 0; i < rules.n; i++) {
        RuleState *st = &rules.state[i]; char key[16];
        for (size_t j = 0; j < st->n; j++) {
            char *actions = expand_actions(st->rule[j].actions, bindings, sequences, st->rule[j].key);
            number_text(st->rule[j].key, key);
            edge_add(g, st->name, mode, key, st->rule[j].target, actions);
            free(actions);
        }
        for (int k = 0; k <= 256; k++) {
            int found = 0;
            for (size_t j = 0; j < st->n; j++) if (st->rule[j].key == k) { found = 1; break; }
            if (!found) {
                char *actions;
                if (!st->def_target) die("incomplete rule state");
                actions = expand_actions(st->def_actions, bindings, sequences, k);
                number_text(k, key); edge_add(g, st->name, mode, key, st->def_target, actions);
                free(actions);
            }
        }
    }
}
static void inspect_bound_rows(const char *stage, const char *stem, const char *outpath,
                               const char *initial) {
    char manifest_path[1024], path[1024], *end; FILE *f, *out; char *s; int found = 0;
    Graph g = {0}; Value *empty = value_new(JOBJ);
    fresh_count = strtoul(initial, &end, 10);
    if (end == initial || *end || fresh_count > 1000000) die("invalid initial fresh count");
    if (snprintf(manifest_path, sizeof(manifest_path), "exec/%s/gen-manifest.tsv", stage) >= (int)sizeof(manifest_path))
        die("manifest path too long");
    f = fopen(manifest_path, "rb"); if (!f) die("cannot open manifest");
    while ((s = line(f))) {
        char *field[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, field, 9); if (n != 9) die("manifest column count");
        if (!strcmp(field[0], "rows") && !strcmp(field[1], stem)) {
            Value *facts, *opts, *bindings;
            if (++found > 1 || strcmp(field[2], "-") || strcmp(field[6], "-")) die("unsupported bound row shape");
            facts = load_facts_expr(field[4]); opts = value_json(field[8], "manifest options");
            bindings = fresh_bindings(opts, facts); direct_bindings(bindings, field[7], facts);
            if (snprintf(path, sizeof(path), "exec/%s/%s-byte.tsv", stage, stem) >= (int)sizeof(path)) die("rule path too long");
            install_plain(&g, path, 'b', bindings, empty);
            if (snprintf(path, sizeof(path), "exec/%s/%s-result.tsv", stage, stem) >= (int)sizeof(path)) die("rule path too long");
            install_plain(&g, path, 'r', bindings, empty);
        }
        free(s);
    }
    if (ferror(f) || fclose(f)) die("manifest read failed");
    if (!found) die("bound row not found");
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output(out, &g); if (fclose(out)) die("output close failed");
}
static void finish(Graph *g) {
    const char *unreachable = "[[\"REJECT\",\"unreachable\"]]";
    size_t initial; char key[16];
    qsort(g->labels, g->nl, sizeof(*g->labels), label_compare);
    for (size_t i = 0; i < g->nl; i++)
        edge_add(g, "RET", 't', g->labels[i], g->labels[i], "[[\"POP\"]]");
    edge_add(g, "RET", 't', "BOT", "DEAD", unreachable);
    initial = g->n;
    for (size_t i = 0; i < initial; i++) {
        State *s = &g->state[i];
        if (s->mode != 'b' && s->mode != 'r') continue;
        for (int k = 0; k <= 256; k++) {
            number_text(k, key);
            if (!edge_has(s, key)) edge_add(g, s->name, s->mode, key, "DEAD", unreachable);
        }
    }
    for (int k = 0; k <= 256; k++) { number_text(k, key); edge_add(g, "DEAD", 'b', key, "DEAD", unreachable); }
}
static void manifest(Graph *g, const char *dir) {
    char path[1024], rule[1024]; FILE *f; char *s; int rows = 0, labels = 0;
    if (snprintf(path, sizeof(path), "%s/gen-manifest.tsv", dir) >= (int)sizeof(path)) die("manifest path too long");
    f = fopen(path, "rb"); if (!f) die("cannot open manifest");
    while ((s = line(f))) {
        char *field[9], *p = s; int i;
        if (!*s || *s == '#') { free(s); continue; }
        for (i = 0; i < 8; i++) {
            char *tab = strchr(p, '\t');
            if (!tab) die("manifest column count");
            *tab = 0; field[i] = p; p = tab + 1;
        }
        if (strchr(p, '\t')) die("manifest column count"); field[8] = p;
        if (!strcmp(field[0], "rows")) {
            if (rows++ || strcmp(field[1], "prune") || strcmp(field[2], "main")) die("unsupported rows declaration");
            for (i = 3; i < 9; i++) if (strcmp(field[i], "-")) die("unsupported rows option");
            if (snprintf(rule, sizeof(rule), "%s/%s-byte.tsv", dir, field[1]) >= (int)sizeof(rule)) die("rule path too long");
            install_file(g, rule, 'b');
            if (snprintf(rule, sizeof(rule), "%s/%s-result.tsv", dir, field[1]) >= (int)sizeof(rule)) die("rule path too long");
            install_file(g, rule, 'r');
        } else if (!strcmp(field[0], "label")) {
            if (labels++ || strcmp(field[1], "CLASS.r8")) die("unsupported label declaration");
            for (i = 2; i < 9; i++) if (strcmp(field[i], "-")) die("unsupported label option");
            label_add(g, field[1]);
        } else die("unsupported DSL op");
        free(s);
    }
    if (ferror(f) || fclose(f)) die("manifest read failed");
    if (rows != 1 || labels != 1) die("incomplete prune manifest");
}
static void output(FILE *f, const Graph *g) {
    fputs("{\"start\":\"START\",\"states\":{", f);
    for (size_t i = 0; i < g->n; i++) {
        const State *s = &g->state[i];
        if (i) fputc(',', f); quoted(f, s->name); fprintf(f, ":[\"%c\",{", s->mode);
        for (size_t j = 0; j < s->n; j++) {
            if (j) fputc(',', f); quoted(f, s->edge[j].key); fputc(':', f);
            fputc('[', f); quoted(f, s->edge[j].target); fprintf(f, ",%d]", s->edge[j].seq);
        }
        fputs("}]", f);
    }
    fputs("},\"seqs\":[", f);
    for (size_t i = 0; i < g->ns; i++) { if (i) fputc(',', f); fputs(g->seq[i], f); }
    fputs("]}", f);
    if (ferror(f)) die("write failed");
}
int main(int argc, char **argv) {
    Graph g = {0}; FILE *out;
    if (argc == 6 && !strcmp(argv[1], "inspect-bound-rows")) {
        inspect_bound_rows(argv[2], argv[3], argv[4], argv[5]); return 0;
    }
    if (argc == 6 && !strcmp(argv[1], "inspect-fresh")) {
        inspect_fresh(argv[2], argv[3], argv[4], argv[5]); return 0;
    }
    if (argc == 5 && !strcmp(argv[1], "inspect-rows")) {
        Value *empty = value_new(JOBJ);
        if (strcmp(argv[4], "b") && strcmp(argv[4], "r")) die("inspect-rows mode must be b or r");
        install_plain(&g, argv[2], argv[4][0], empty, empty);
        out = fopen(argv[3], "wb"); if (!out) die("cannot open output");
        output(out, &g); if (fclose(out)) die("output close failed");
        return 0;
    }
    if (argc >= 4 && !strcmp(argv[1], "inspect-let")) {
        inspect_lets(argv[2], argv[3], argc - 4, argv + 4); return 0;
    }
    if (argc == 4 && !strcmp(argv[1], "inspect-facts")) {
        Value *v = load_fact(argv[2]);
        out = fopen(argv[3], "wb"); if (!out) die("cannot open output");
        value_write(out, v); if (fclose(out)) die("output close failed");
        return 0;
    }
    if ((argc != 3 && argc != 4) || strcmp(argv[1], "prune")) die("usage: seed-gen prune OUT.json [RULE_DIR]");
    manifest(&g, argc == 4 ? argv[3] : "exec/prune");
    finish(&g);
    out = fopen(argv[2], "wb"); if (!out) die("cannot open output");
    output(out, &g); if (fclose(out)) die("output close failed");
    fprintf(stderr, "prune states %lu\n", (unsigned long)g.n);
    return 0;
}
