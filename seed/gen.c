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

/* Python facts have unbounded integers.  E3's decimal magnitude limits use
   the full uint64 range, while seed/json.h deliberately keeps JSON inputs in
   signed range.  This internal value kind covers those typed TSV cells. */
enum { JUINT = JNULL + 1 };

typedef struct { char *key, *target; int seq; } Edge;
typedef struct { char *name; char mode; Edge *edge; size_t n, cap; int *key_index; } State;
typedef struct { State *state; size_t n, cap; size_t *state_index, index_cap; char **seq; size_t ns, cs, *seq_index, seq_index_cap; char **labels; size_t nl, cl; } Graph;
typedef struct { int key; char *target, *actions; } Rule;
typedef struct { char *name; Rule *rule; size_t n, cap; char *def_target, *def_actions; } RuleState;
typedef struct { RuleState *state; size_t n, cap; } RuleSet;
typedef struct { char *s; size_t n, cap; } Buffer;
/* Values preserve the insertion order of object keys, as assemble.load_facts does. */
typedef struct Value Value;
typedef struct { char *key; Value *value; } Member;
struct Value {
    int kind; /* JOBJ/JARR/JSTR/JINT */
    char *s; long long number; unsigned long long unumber;
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
    case JUINT: snprintf(n, sizeof(n), "%llu", v->unumber); buf_add(b, n, strlen(n)); break;
    case JBOOL: buf_add(b, v->number ? "true" : "false", v->number ? 4 : 5); break;
    case JNULL: buf_add(b, "null", 4); break;
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
    Value *v = value_new(JSTR); v->s = copy(s); v->n = strlen(s); return v;
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
    if (n->type == JSTR) { v->s = copy_n(d->buf + n->str, (size_t)n->slen); v->n = (size_t)n->slen; }
    else if (n->type == JINT || n->type == JBOOL) v->number = n->ival;
    else if (n->type == JNULL) { /* null is a fact value, not a missing key */ }
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
static const char *value_scalar_text(Value *v, char number[64]) {
    if (!v) return NULL;
    if (v->kind == JSTR) return v->s;
    if (v->kind == JINT) { snprintf(number, 64, "%lld", v->number); return number; }
    if (v->kind == JUINT) { snprintf(number, 64, "%llu", v->unumber); return number; }
    die("expected scalar value"); return NULL;
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
static char *value_json_text(const Value *v);
static int actions_depend_on_key(const char *raw, Value *sequences) {
    if (strstr(raw, "observation")) return 1;
    if (strstr(raw, "\"@\"") && sequences && sequences->n) {
        char *text = value_json_text(sequences);
        int found = strstr(text, "observation") != NULL;
        free(text); return found;
    }
    return 0;
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
    case JUINT: fprintf(f, "%llu", v->unumber); break;
    case JBOOL: fputs(v->number ? "true" : "false", f); break;
    case JNULL: fputs("null", f); break;
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
        if (errno == ERANGE && text[0] != '-') {
            unsigned long long u;
            errno = 0; u = strtoull(text, &end, 10);
            if (errno || end == text || *end || text[0] == '+') die("invalid fact integer");
            v = value_new(JUINT); v->unumber = u; return v;
        }
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
    static struct { char *stem; Value *value; } *cache;
    static size_t cache_n, cache_cap;
    char path[1024]; FILE *f; char *s; Value *root = value_new(JOBJ);
    char *header[128] = {0}, *types[128] = {0}; int nh = 0, typed = -1;
    Value *table = NULL; char *table_name = NULL;
    size_t ci;
    for (ci = 0; ci < cache_n; ci++)
        if (!strcmp(cache[ci].stem, stem)) return cache[ci].value;
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
    /* A header-only facts file is an empty map in the Python constructor. */
    if (typed < 0) typed = 1;
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
    if (cache_n == cache_cap) {
        cache_cap = cache_cap ? cache_cap * 2 : 16;
        cache = grow(cache, cache_cap, sizeof(*cache));
    }
    cache[cache_n].stem = copy(stem);
    cache[cache_n++].value = root;
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
        if (!v) { fprintf(stderr, "unknown fact path: %s\n", path); die("unknown fact path"); }
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
static Value *value_field(Value *env, const char *name) {
    const char *open = strchr(name, '['); size_t n = strlen(name);
    if (open && n && name[n - 1] == ']') {
        char *root = copy_n(name, (size_t)(open - name));
        char *key = copy_n(open + 1, n - (size_t)(open - name) - 2);
        Value *v = value_get(value_get(env, root), key);
        free(root); free(key); return v;
    }
    return value_get(env, name);
}
static char *interpolate(const char *text, Value *env) {
    size_t cap = strlen(text) + 64, n = 0, i; char *out = grow(NULL, cap, 1);
    for (i = 0; text[i]; i++) {
        const char *part = text + i; size_t len = 1; char number[64];
        if ((text[i] == '{' && text[i + 1] == '{') ||
            (text[i] == '}' && text[i + 1] == '}')) {
            part = text + i; i++;
        } else if (text[i] == '{') {
            const char *end = strchr(text + i + 1, '}'); Value *v; char *key;
            if (!end) die("unterminated interpolation");
            key = copy_n(text + i + 1, (size_t)(end - text - i - 1));
            v = value_field(env, key);
            if (!v || (v->kind != JSTR && v->kind != JINT && v->kind != JUINT)) {
                fprintf(stderr, "seed-gen: missing interpolation fact %s\n", key);
                die("unknown interpolation fact");
            }
            free(key);
            if (v->kind == JINT) { snprintf(number, sizeof(number), "%lld", v->number); part = number; }
            else if (v->kind == JUINT) { snprintf(number, sizeof(number), "%llu", v->unumber); part = number; }
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
                    char number[64]; const char *text = value_scalar_text(actual, number);
                    if (!text || strcmp(text, expect)) match = 0;
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
/* The manifest's string freshrows form is a row-ordered label table next to
   the manifest.  Keep allocation in row order: P.fresh uses one graph-wide
   counter, so sorting by key would change the generated network bytes. */
static Value *fresh_bindings_file(Value *opts, const char *root) {
    Value *bindings = value_new(JOBJ), *spec = value_get(opts, "freshrows");
    char path[1024]; FILE *f; char *s; int header = 0;
    if (!spec || spec->kind != JSTR) return bindings;
    if (snprintf(path, sizeof(path), "%s/%s", root, spec->s) >= (int)sizeof(path))
        die("freshrows path too long");
    f = fopen(path, "rb"); if (!f) die("cannot open freshrows table");
    while ((s = line(f))) {
        char *field[3], *label; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, field, 3);
        if (n != 3) die("freshrows column count");
        if (!header++) {
            if (strcmp(field[0], "owner") || strcmp(field[1], "kind") || strcmp(field[2], "key"))
                die("freshrows header mismatch");
        } else {
            label = fresh_label(field[0], field[1]);
            value_put(bindings, field[2], value_string(label));
            free(label);
        }
        free(s);
    }
    if (ferror(f) || fclose(f)) die("freshrows read failed");
    if (!header) die("empty freshrows table");
    return bindings;
}
static void numeric_file_bindings(Value *opts, Value *facts, Value *bindings) {
    Value *specs = value_get(opts, "freshrows");
    if (!specs || specs->kind != JARR || specs->n != 1)
        die("numeric freshrows declaration missing");
    Value *spec = specs->items[0].value, *where = value_get(spec, "where");
    const char *file = value_text(value_get(spec, "file"));
    char path[1024]; FILE *f; char *s; int header = 0;
    if (snprintf(path, sizeof(path), "exec/parse/%s", file) >= (int)sizeof(path))
        die("numeric freshrows path too long");
    f = fopen(path, "rb"); if (!f) die("cannot open numeric freshrows");
    while ((s = line(f))) {
        char *field[4]; int n;
        if (!*s) { free(s); continue; }
        if (s[0] == '#') {
            if (!header && strcmp(s, "# section\tbinding\tprefix\tkind"))
                die("numeric freshrows header mismatch");
            header = 1; free(s); continue;
        }
        n = fields_tab(s, field, 4);
        if (n != 4 || !header) die("numeric freshrows column count");
        if (where && where->kind == JOBJ) {
            Value *selected = value_get(where, "section");
            if (!selected || selected->kind != JSTR) die("numeric freshrows section missing");
            if (strcmp(field[0], selected->s)) { free(s); continue; }
        }
        Value *ctx = value_new(JOBJ); char *key, *owner, *kind, *label;
        for (size_t i = 0; i < facts->n; i++)
            value_put(ctx, facts->items[i].key, facts->items[i].value);
        value_put(ctx, "section", value_string(field[0]));
        value_put(ctx, "binding", value_string(field[1]));
        value_put(ctx, "prefix", value_string(field[2]));
        value_put(ctx, "kind", value_string(field[3]));
        key = interpolate(value_text(value_get(spec, "key")), ctx);
        owner = interpolate(value_text(value_get(spec, "owner")), ctx);
        kind = interpolate(value_text(value_get(spec, "kind")), ctx);
        label = fresh_label(owner, kind);
        value_put(bindings, key, value_string(label));
        free(key); free(owner); free(kind); free(label); free(s);
    }
    if (ferror(f) || fclose(f) || !header) die("numeric freshrows read failed");
}
static void declared_bindmap(Value *opts, Value *facts, Value *bindings) {
    Value *map = value_get(opts, "bindmap");
    if (!map || map->kind != JOBJ) die("expected declared bindmap");
    for (size_t i = 0; i < map->n; i++) {
        Value *src = map->items[i].value, *value;
        const char *s = value_text(src);
        if (!strncmp(s, "@str:", 5)) {
            char *expanded = interpolate(s + 5, facts);
            value = value_string(expanded); free(expanded);
        } else value = value_path(facts, s);
        value_put(bindings, map->items[i].key, value);
    }
}
static void direct_bindings_ex(Value *bindings, Value *sequences,
                               const char *text, Value *facts) {
    char *parts = copy(text), *p = parts;
    if (!strcmp(text, "-") || !*text) { free(parts); return; }
    while (*p) {
        char *comma = strchr(p, ','), *eq = strchr(p, '='); Value *v;
        if (comma) *comma = 0;
        if (!eq || eq == p || !eq[1]) die("invalid binding cell");
        *eq = 0;
        if (!strncmp(eq + 1, "@str:", 5) || !strncmp(eq + 1, "@fmt:", 5)) {
            char *s = interpolate(eq + 6, facts);
            v = value_string(s); free(s);
        } else if (!strncmp(eq + 1, "@bytes:", 7)) {
            Value *source = eq[8] == '=' ? value_path(facts, eq + 9) : NULL;
            if (source && source->kind != JSTR) die("byte source is not text");
            const unsigned char *bytes = (const unsigned char *)(source ? source->s : eq + 8);
            size_t count = source ? source->n : strlen((const char *)bytes), i;
            Value *actions = value_new(JARR);
            if (!sequences) die("byte binding requires sequences");
            for (i = 0; i < count; i++) {
                Value *act = value_new(JARR), *byte = value_new(JINT);
                byte->number = bytes[i];
                value_put(act, NULL, value_string("SBOUT"));
                value_put(act, NULL, byte);
                value_put(actions, NULL, act);
            }
            value_put(sequences, p, actions);
            if (!comma) break;
            p = comma + 1; continue;
        } else if (!strncmp(eq + 1, "fresh:", 6)) {
            char *scope = copy(eq + 7), *kind = strrchr(scope, ':'), *label;
            char *owner;
            if (!kind || !kind[1]) die("invalid fresh binding");
            *kind++ = 0;
            if (strncmp(scope, "P:", 2) && strncmp(scope, "U:", 2))
                die("unsupported fresh binding scope");
            owner = interpolate(scope + 2, facts);
            label = fresh_label(owner, kind);
            v = value_string(label); free(label); free(owner); free(scope);
        } else v = value_path(facts, eq[1] == '$' ? eq + 2 : eq + 1);
        value_put(bindings, p, v);
        if (!comma) break; p = comma + 1;
    }
    free(parts);
}
static void direct_bindings(Value *bindings, const char *text, Value *facts) {
    direct_bindings_ex(bindings, NULL, text, facts);
}
static Value *map_cell(Value *source, Value *ctx) {
    const char *s; size_t n;
    if (source->kind != JSTR) return source;
    s = source->s; n = strlen(s);
    if (n > 1 && s[0] == '$') return value_path(ctx, s + 1);
    if (n >= 3 && s[0] == '{' && s[n - 1] == '}' && !strchr(s + 1, '{')) {
        char *key = copy_n(s + 1, n - 2); Value *v = value_field(ctx, key);
        free(key); if (!v) die("unknown mapseq cell"); return v;
    }
    { char *text = interpolate(s, ctx); Value *v = value_string(text); free(text); return v; }
}
static int map_equal(Value *a, Value *b) {
    if (!a || !b || a->kind != b->kind) return 0;
    if (a->kind == JSTR) return !strcmp(a->s, b->s);
    if (a->kind == JINT || a->kind == JBOOL) return a->number == b->number;
    if (a->kind == JUINT) return a->unumber == b->unumber;
    if (a->kind == JNULL) return 1;
    die("unsupported mapseq predicate value"); return 0;
}
static void map_actions(Value *out, Value *actions, Value *ctx) {
    size_t i, c;
    if (!actions || actions->kind != JARR) die("invalid mapseq acts");
    for (i = 0; i < actions->n; i++) {
        Value *input = actions->items[i].value, *action;
        if (input->kind != JARR || !input->n) die("invalid mapseq action");
        if (!strcmp(value_text(input->items[0].value), "@out") ||
            !strcmp(value_text(input->items[0].value), "@bytes")) {
            int output = !strcmp(value_text(input->items[0].value), "@out");
            Value *value; const unsigned char *s;
            if (input->n != 2) die("invalid mapseq byte expansion");
            value = map_cell(input->items[1].value, ctx);
            s = (const unsigned char *)value_text(value);
            for (; *s; s++) {
                Value *byte = value_new(JINT); action = value_new(JARR); byte->number = *s;
                value_put(action, NULL, value_string(output ? "OUT" : "SBOUT"));
                value_put(action, NULL, byte); value_put(out, NULL, action);
            }
            continue;
        }
        action = value_new(JARR);
        for (c = 0; c < input->n; c++)
            value_put(action, NULL, map_cell(input->items[c].value, ctx));
        value_put(out, NULL, action);
    }
}
static void map_old_part(Value *out, Value *part, Value *facts) {
    Value *over = value_get(part, "over"), *where = value_get(part, "where");
    Value *rows = over ? value_path(facts, value_text(over)) : NULL;
    size_t i, n = rows ? rows->n : 1;
    if (rows && rows->kind != JARR) die("mapseq over is not a list");
    for (i = 0; i < n; i++) {
        Value *ctx = value_new(JOBJ); size_t j; int take = 1;
        Value *item = rows ? rows->items[i].value : NULL;
        for (j = 0; j < facts->n; j++) value_put(ctx, facts->items[j].key, facts->items[j].value);
        if (item) {
            if (item->kind != JOBJ) die("mapseq item is not an object");
            for (j = 0; j < item->n; j++) value_put(ctx, item->items[j].key, item->items[j].value);
        }
        if (where) {
            if (where->kind != JOBJ) die("invalid mapseq where");
            for (j = 0; j < where->n; j++) {
                Value *actual = value_get(ctx, where->items[j].key);
                Value *expect = where->items[j].value;
                if (expect->kind == JSTR && actual && actual->kind == JINT) {
                    char number[64];
                    snprintf(number, sizeof(number), "%lld", actual->number);
                    if (strcmp(number, expect->s)) take = 0;
                } else if (!map_equal(actual, expect)) take = 0;
            }
        }
        if (take) map_actions(out, value_get(part, "acts"), ctx);
    }
}
static Value *mapseq_construct(Value *opts, Value *facts) {
    Value *spec = value_get(opts, "mapseq"), *out = value_new(JOBJ); size_t k; int pass;
    if (!spec) return out;
    if (spec->kind != JOBJ) die("mapseq is not an object");
    /* assemble.Run.mapseq first evaluates fixed names, then formatted names.
       This insertion order is observable in the returned fact dictionary. */
    for (pass = 0; pass < 2; pass++) for (k = 0; k < spec->n; k++) {
        const char *namefmt = spec->items[k].key;
        Value *decl = spec->items[k].value;
        if (!!strchr(namefmt, '{') != pass) continue;
        if (decl->kind == JARR) {
            Value *acts = value_new(JARR); size_t p;
            for (p = 0; p < decl->n; p++) map_old_part(acts, decl->items[p].value, facts);
            value_put(out, namefmt, acts); continue;
        }
        Value *rows = value_path(facts, value_text(value_get(decl, "over")));
        Value *parts = value_get(decl, "parts"); size_t i;
        if (rows->kind != JARR || !parts || parts->kind != JARR) die("invalid mapseq rows or parts");
        for (i = 0; i < rows->n; i++) {
            Value *row = rows->items[i].value, *ctx = value_new(JOBJ), *acts = value_new(JARR);
            char *name; size_t j, p;
            if (row->kind != JOBJ) die("mapseq row is not an object");
            for (j = 0; j < facts->n; j++) value_put(ctx, facts->items[j].key, facts->items[j].value);
            for (j = 0; j < row->n; j++) value_put(ctx, row->items[j].key, row->items[j].value);
            name = interpolate(namefmt, ctx);
            for (p = 0; p < parts->n; p++) {
                Value *part = parts->items[p].value;
                Value *splice = value_get(part, "splice"), *where = value_get(part, "where");
                Value *source = value_get(part, "acts"); int take = 1;
                if (splice) {
                    Value *refs = value_get(row, value_text(splice)); size_t r;
                    if (!refs || refs->kind != JARR) die("mapseq splice is not a list");
                    for (r = 0; r < refs->n; r++) {
                        Value *named = value_get(out, value_text(refs->items[r].value)); size_t a;
                        if (!named || named->kind != JARR) die("mapseq forward or unknown reference");
                        for (a = 0; a < named->n; a++) value_put(acts, NULL, named->items[a].value);
                    }
                    continue;
                }
                if (where) {
                    if (where->kind != JOBJ) die("mapseq where is not an object");
                    for (j = 0; j < where->n; j++) {
                        Value *actual = value_get(row, where->items[j].key);
                        Value *expect = where->items[j].value;
                        if (expect->kind == JSTR) {
                            char *expected = interpolate(expect->s, ctx);
                            char number[64]; const char *text = value_scalar_text(actual, number);
                            if (!text || strcmp(text, expected)) take = 0;
                            free(expected);
                        } else if (!map_equal(actual, expect)) take = 0;
                    }
                }
                if (!take) continue;
                if (!source || source->kind != JARR) die("mapseq acts is not a list");
                for (j = 0; j < source->n; j++) {
                    Value *input = source->items[j].value, *action; size_t c;
                    if (input->kind != JARR || !input->n) die("invalid mapseq action");
                    if (!strcmp(value_text(input->items[0].value), "@bytes") ||
                        !strcmp(value_text(input->items[0].value), "@out")) {
                        int output = !strcmp(value_text(input->items[0].value), "@out");
                        Value *value; const unsigned char *s;
                        if (input->n != 2) die("invalid byte expansion action");
                        value = map_cell(input->items[1].value, ctx); s = (const unsigned char *)value_text(value);
                        for (; *s; s++) {
                            Value *byte = value_new(JINT); action = value_new(JARR);
                            byte->number = *s;
                            value_put(action, NULL, value_string(output ? "OUT" : "SBOUT")); value_put(action, NULL, byte);
                            value_put(acts, NULL, action);
                        }
                        continue;
                    }
                    action = value_new(JARR);
                    for (c = 0; c < input->n; c++) value_put(action, NULL, map_cell(input->items[c].value, ctx));
                    value_put(acts, NULL, action);
                }
            }
            value_put(out, name, acts); free(name);
        }
    }
    return out;
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

/* A template's `each` and `over` columns are finite products of fact lists.
   This reader is intentionally stage-neutral: it expands declaration rows,
   leaving graph installation and edits to the later constructor step. */
typedef struct { char *block, *each, *over, *kind, *a, *b, *c, *d; int depth; } TRow;
typedef struct { TRow *row; size_t n, cap; } TRows;
typedef void (*TVisit)(Value *, void *);

static Value *scope_add(Value *scope, const char *key, Value *value) {
    Value *next = value_new(JOBJ); size_t i;
    for (i = 0; i < scope->n; i++) value_put(next, scope->items[i].key, scope->items[i].value);
    value_put(next, key, value);
    return next;
}
static void tuple_walk(char *spec, Value *facts, Value *scope, TVisit visit, void *ctx) {
    char *comma, *colon, *dot, *name, *source; Value *list; size_t i;
    if (!strcmp(spec, "-") || !*spec) { visit(scope, ctx); return; }
    comma = strchr(spec, ','); if (comma) *comma = 0;
    colon = strchr(spec, ':');
    if (colon) {
        *colon = 0; name = spec; source = colon + 1;
        dot = strchr(source, '.'); if (!dot) die("invalid dependent template fact");
        *dot = 0; list = value_get(value_get(scope, source), dot + 1);
    } else { name = spec; list = value_get(facts, name); }
    if (!list || list->kind != JARR) die("unknown template fact list");
    for (i = 0; i < list->n; i++) {
        Value *next = scope_add(scope, name, list->items[i].value);
        if (comma) tuple_walk(comma + 1, facts, next, visit, ctx);
        else visit(next, ctx);
    }
    if (colon) { *colon = ':'; *dot = '.'; }
    if (comma) *comma = ',';
}
static char *template_subst_ctx(const char *text, Value *scope, Value *labels,
                                Value *rowlabels, Value *prev, const char *fresh_owner) {
    Buffer out = {0}; size_t i;
    for (i = 0; text[i]; i++) {
        if (text[i] == '{') {
            const char *end = strchr(text + i + 1, '}'); char *path, *dot, number[64];
            Value *value; const char *piece;
            if (!end) die("unterminated template variable");
            path = copy_n(text + i + 1, (size_t)(end - text - i - 1));
            if (!strncmp(path, "fresh:", 6) || !strncmp(path, "fresh@row:", 10)) {
                int row = !strncmp(path, "fresh@row:", 10);
                char *name = path + (row ? 10 : 6), *kind = strchr(name, ':');
                Value *map = row ? rowlabels : labels;
                if (!fresh_owner || !kind || !kind[1]) die("invalid template fresh label");
                *kind++ = 0;
                value = value_get(map, name);
                if (!value) {
                    char *label = fresh_label(fresh_owner, kind);
                    value = value_string(label); value_put(map, name, value); free(label);
                }
            } else if (!strncmp(path, "prev:", 5)) {
                value = value_get(prev, path + 5);
            } else {
                dot = strchr(path, '.'); if (dot) *dot++ = 0;
                value = value_get(scope, path); if (dot) value = value_get(value, dot);
            }
            piece = value_scalar_text(value, number);
            if (!piece) die("unbound template variable");
            buf_add(&out, piece, strlen(piece)); free(path); i = (size_t)(end - text);
        } else buf_char(&out, text[i]);
    }
    return out.s ? out.s : copy("");
}
static char *template_subst(const char *text, Value *scope) {
    return template_subst_ctx(text, scope, NULL, NULL, NULL, NULL);
}
typedef struct { TRows *rows; Value *facts; Buffer *out; size_t first, last; int depth;
                 Value *labels, *prev, *modes; const char *fresh_owner; } TGroup;
static void template_groups(TRows *rows, Value *facts, Value *scope, Buffer *out,
                            size_t first, size_t last, int depth, Value *labels,
                            Value *prev, Value *modes, const char *fresh_owner);
static void template_group_visit(Value *scope, void *arg) {
    TGroup *g = arg; size_t i = g->first;
    while (i < g->last) {
        TRow *r = &g->rows->row[i];
        if (r->depth == g->depth) {
            char *a, *b, *c, *d; Value *rowlabels = value_new(JOBJ);
            if (strcmp(r->kind, "rule") && strcmp(r->kind, "rule:r")) die("template graph edit not yet supported");
            a = template_subst_ctx(r->a, scope, g->labels, rowlabels, g->prev, g->fresh_owner);
            b = template_subst_ctx(r->b, scope, g->labels, rowlabels, g->prev, g->fresh_owner);
            c = template_subst_ctx(r->c, scope, g->labels, rowlabels, g->prev, g->fresh_owner);
            d = template_subst_ctx(r->d, scope, g->labels, rowlabels, g->prev, g->fresh_owner);
            for (size_t j = 0; j < rowlabels->n; j++)
                value_put(g->prev, rowlabels->items[j].key, rowlabels->items[j].value);
            if (g->modes && !strcmp(r->kind, "rule:r"))
                value_put(g->modes, a, value_string("r"));
            buf_add(g->out, a, strlen(a)); buf_char(g->out, '\t');
            buf_add(g->out, b, strlen(b)); buf_char(g->out, '\t');
            buf_add(g->out, c, strlen(c)); buf_char(g->out, '\t');
            buf_add(g->out, d, strlen(d)); buf_char(g->out, '\n');
            free(a); free(b); free(c); free(d); i++;
        } else {
            size_t end = i + 1;
            if (r->depth != g->depth + 1) die("template loop depth jump");
            while (end < g->last && g->rows->row[end].depth > g->depth) end++;
            template_groups(g->rows, g->facts, scope, g->out, i, end, g->depth + 1,
                            g->labels, g->prev, g->modes, g->fresh_owner);
            i = end;
        }
    }
}
static void template_groups(TRows *rows, Value *facts, Value *scope, Buffer *out,
                            size_t first, size_t last, int depth, Value *labels,
                            Value *prev, Value *modes, const char *fresh_owner) {
    size_t i = first;
    while (i < last) {
        TRow *r = &rows->row[i]; size_t end = i + 1; TGroup group;
        if (r->depth != depth || !strcmp(r->over, "=")) die("invalid template loop");
        while (end < last && (rows->row[end].depth > depth ||
               (rows->row[end].depth == depth && !strcmp(rows->row[end].over, "=")))) end++;
        group = (TGroup){rows, facts, out, i, end, depth, labels, prev, modes, fresh_owner};
        tuple_walk(r->over, facts, scope, template_group_visit, &group);
        i = end;
    }
}
static void template_block_visit(Value *scope, void *arg) {
    TGroup *g = arg;
    Value *labels = value_new(JOBJ), *prev = value_new(JOBJ);
    template_groups(g->rows, g->facts, scope, g->out, g->first, g->last, 0,
                    labels, prev, g->modes, g->fresh_owner);
}
static void template_rows(TRows *rows, Value *facts, Buffer *out, size_t first, size_t last,
                          const char *fresh_owner, Value *modes) {
    TGroup group = {rows, facts, out, first, last, 0, NULL, NULL, modes, fresh_owner};
    Value *scope = value_new(JOBJ);
    tuple_walk(rows->row[first].each, facts, scope, template_block_visit, &group);
}
static Buffer expand_template_file_fresh(const char *path, Value *facts,
                                         const char *section, const char *fresh_owner,
                                         Value *modes) {
    FILE *f; char *s; TRows rows = {0}; Buffer expanded = {0}; size_t i;
    f = fopen(path, "rb"); if (!f) die("cannot open template");
    while ((s = line(f))) {
        char *field[9]; int n; TRow *r; char *over;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, field, 9); if (n != 9) die("template column count");
        if (strcmp(field[0], section)) { free(s); continue; }
        if (rows.n == rows.cap) { rows.cap = rows.cap ? rows.cap * 2 : 32; rows.row = grow(rows.row, rows.cap, sizeof(*rows.row)); }
        r = &rows.row[rows.n++];
        over = field[3]; r->depth = 0; while (*over == '.') { r->depth++; over++; }
        *r = (TRow){copy(field[1]), copy(field[2]), copy(over), copy(field[4]),
                    copy(field[5]), copy(field[6]), copy(field[7]), copy(field[8]), r->depth};
        free(s);
    }
    if (ferror(f) || fclose(f) || !rows.n) die("template read failed");
    for (i = 0; i < rows.n;) {
        size_t end = i + 1;
        while (end < rows.n && !strcmp(rows.row[end].block, rows.row[i].block)) {
            if (strcmp(rows.row[end].each, rows.row[i].each)) die("template block has two each lists");
            end++;
        }
        template_rows(&rows, facts, &expanded, i, end, fresh_owner, modes); i = end;
    }
    return expanded;
}
static Buffer expand_template_file(const char *path, Value *facts, const char *section) {
    Buffer expanded = expand_template_file_fresh(path, facts, section, NULL, NULL);
    return expanded;
}
static Buffer expand_template_section(const char *stage, const char *section) {
    char path[1024]; Value *facts = load_fact(!strcmp(stage, "lex") ? "lex-gen" : stage);
    if (snprintf(path, sizeof(path), "exec/%s/gen-template.tsv", stage) >= (int)sizeof(path))
        die("template path too long");
    Buffer expanded = expand_template_file(path, facts, section);
    return expanded;
}
static void inspect_template(const char *stage, const char *section, const char *outpath) {
    Buffer expanded = expand_template_section(stage, section);
    FILE *out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    if (expanded.n && fwrite(expanded.s, 1, expanded.n, out) != expanded.n) die("template write failed");
    if (fclose(out)) die("template close failed");
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
static void rule_keys_classes(RuleState *s, const char *keys, const char *target,
                              const char *actions, Value *classes) {
    if (keys[0] == '@') {
        Value *v = value_get(classes, keys + 1);
        if (!v || v->kind != JARR || !v->n) die("unknown rule class");
        for (size_t i = 0; i < v->n; i++) {
            Value *item = v->items[i].value;
            if (item->kind != JINT || item->number < 0 || item->number > 256)
                die("invalid rule class member");
            rule_add(s, (int)item->number, target, actions);
        }
    } else rule_keys(s, keys, target, actions);
}
static size_t name_hash(const char *name);
static void action_labels(Graph *g, const char *actions);
static void seq_index_grow(Graph *g) {
    size_t cap = g->seq_index_cap ? g->seq_index_cap * 2 : 128;
    size_t *slots = grow(NULL, cap, sizeof(*slots));
    memset(slots, 0, cap * sizeof(*slots));
    for (size_t i = 0; i < g->ns; i++) {
        size_t h = name_hash(g->seq[i]) & (cap - 1);
        while (slots[h]) h = (h + 1) & (cap - 1);
        slots[h] = i + 1;
    }
    free(g->seq_index); g->seq_index = slots; g->seq_index_cap = cap;
}
static int seq(Graph *g, const char *actions) {
    size_t h;
    if (!g->seq_index_cap || (g->ns + 1) * 2 >= g->seq_index_cap) seq_index_grow(g);
    h = name_hash(actions) & (g->seq_index_cap - 1);
    while (g->seq_index[h]) {
        size_t i = g->seq_index[h] - 1;
        if (!strcmp(g->seq[i], actions)) return (int)i;
        h = (h + 1) & (g->seq_index_cap - 1);
    }
    if (g->ns >= INT_MAX) die("too many sequences");
    if (g->ns == g->cs) { g->cs = g->cs ? g->cs * 2 : 64; g->seq = grow(g->seq, g->cs, sizeof(*g->seq)); }
    g->seq[g->ns] = copy(actions); g->seq_index[h] = g->ns + 1;
    action_labels(g, actions);
    return (int)g->ns++;
}
static size_t name_hash(const char *name) {
    size_t h = (size_t)2166136261u;
    for (; *name; name++) h = (h ^ (unsigned char)*name) * (size_t)16777619u;
    return h;
}
static void state_index_grow(Graph *g) {
    size_t cap = g->index_cap ? g->index_cap * 2 : 128;
    while (cap <= (g->n + 1) * 2) {
        if (cap > SIZE_MAX / 2) die("state index too large");
        cap *= 2;
    }
    size_t *slots = grow(NULL, cap, sizeof(*slots));
    memset(slots, 0, cap * sizeof(*slots));
    for (size_t i = 0; i < g->n; i++) {
        size_t h = name_hash(g->state[i].name) & (cap - 1);
        while (slots[h]) h = (h + 1) & (cap - 1);
        slots[h] = i + 1;
    }
    free(g->state_index); g->state_index = slots; g->index_cap = cap;
}
static State *graph_state(Graph *g, const char *name, char mode) {
    size_t h; State *s;
    if (!g->index_cap || (g->n + 1) * 2 >= g->index_cap) state_index_grow(g);
    h = name_hash(name) & (g->index_cap - 1);
    while (g->state_index[h]) {
        s = &g->state[g->state_index[h] - 1];
        if (!strcmp(s->name, name)) {
            if (s->mode != mode) die("state mode conflict");
            return s;
        }
        h = (h + 1) & (g->index_cap - 1);
    }
    if (g->n == g->cap) { g->cap = g->cap ? g->cap * 2 : 64; g->state = grow(g->state, g->cap, sizeof(*g->state)); }
    s = &g->state[g->n++]; memset(s, 0, sizeof(*s)); s->name = copy(name); s->mode = mode;
    if (mode == 'b' || mode == 'r') {
        s->key_index = grow(NULL, 257, sizeof(*s->key_index));
        memset(s->key_index, 0, 257 * sizeof(*s->key_index));
    }
    g->state_index[h] = g->n;
    return s;
}
static int numeric_key(const char *key) {
    unsigned n = 0;
    if (!*key) return -1;
    for (; *key; key++) {
        if (*key < '0' || *key > '9') return -1;
        n = n * 10 + (unsigned)(*key - '0');
        if (n > 256) return -1;
    }
    return (int)n;
}
static int edge_has(const State *s, const char *key) {
    int n = numeric_key(key);
    if (s->key_index && n >= 0) return s->key_index[n] != 0;
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
        int op = -1, arg;
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
    if (edge_has(s, key)) return;
    if (s->n == s->cap) { s->cap = s->cap ? s->cap * 2 : 32; s->edge = grow(s->edge, s->cap, sizeof(*s->edge)); }
    e = &s->edge[s->n++]; e->key = copy(key); e->target = copy(target); e->seq = seq(g, actions);
    { int n = numeric_key(key); if (s->key_index && n >= 0) s->key_index[n] = (int)s->n; }
}
static void edge_set(Graph *g, const char *name, char mode, const char *key,
                     const char *target, const char *actions) {
    State *s = graph_state(g, name, mode); size_t i; int id = seq(g, actions);
    int n = numeric_key(key);
    if (s->key_index && n >= 0 && s->key_index[n]) {
        Edge *e = &s->edge[s->key_index[n] - 1];
        e->target = copy(target); e->seq = id; return;
    }
    for (i = 0; i < s->n; i++) if (!strcmp(s->edge[i].key, key)) {
        s->edge[i].target = copy(target); s->edge[i].seq = id; return;
    }
    if (s->n == s->cap) { s->cap = s->cap ? s->cap * 2 : 32; s->edge = grow(s->edge, s->cap, sizeof(*s->edge)); }
    s->edge[s->n++] = (Edge){copy(key), copy(target), id};
    if (s->key_index && n >= 0) s->key_index[n] = (int)s->n;
}
typedef struct { char *key, *target, *actions; } DRule;
typedef struct { char *name; DRule *rules; size_t n, cap; char *def_target, *def_actions; } DRow;
typedef struct { DRow *rows; size_t n, cap; } DTable;
static DRow *drow(DTable *t, const char *name) {
    size_t i;
    for (i = 0; i < t->n; i++) if (!strcmp(t->rows[i].name, name)) return &t->rows[i];
    if (t->n == t->cap) { t->cap = t->cap ? t->cap * 2 : 32; t->rows = grow(t->rows, t->cap, sizeof(*t->rows)); }
    memset(&t->rows[t->n], 0, sizeof(*t->rows));
    t->rows[t->n].name = copy(name); return &t->rows[t->n++];
}
static DRule *drule(DRow *r, const char *key) {
    size_t i;
    for (i = 0; i < r->n; i++) if (!strcmp(r->rules[i].key, key)) return &r->rules[i];
    if (r->n == r->cap) { r->cap = r->cap ? r->cap * 2 : 16; r->rules = grow(r->rules, r->cap, sizeof(*r->rules)); }
    memset(&r->rules[r->n], 0, sizeof(*r->rules));
    r->rules[r->n].key = copy(key); return &r->rules[r->n++];
}
static int domain_has(Value *domain, const char *key) {
    size_t i; char number[64];
    for (i = 0; i < domain->n; i++) {
        const char *one = value_scalar_text(domain->items[i].value, number);
        if (!strcmp(one, key)) return 1;
    }
    return 0;
}
static Value *numeric_domain(int lo, int hi) {
    Value *v = value_new(JARR); int i;
    if (lo < 0 || hi < lo || hi > 1000000) die("invalid numeric domain");
    for (i = lo; i < hi; i++) { Value *n = value_new(JINT); n->number = i; value_put(v, NULL, n); }
    return v;
}
static void keys_simple(Value *out, const char *text, Value *domain, Value *classes) {
    char *work = copy(text), *part = work;
    while (*part) {
        char *comma = strchr(part, ','), *dash; size_t i;
        if (comma) *comma = 0;
        if (!strcmp(part, "*")) {
            for (i = 0; i < domain->n; i++) {
                char number[64];
                const char *s = value_scalar_text(domain->items[i].value, number);
                value_put(out, NULL, value_string(s));
            }
        } else if (*part == '@') {
            Value *members = value_get(classes, part + 1);
            if (!members || members->kind != JARR || !members->n) die("unknown key class");
            for (i = 0; i < members->n; i++) {
                char number[64]; const char *s = value_scalar_text(members->items[i].value, number);
                if (!domain_has(domain, s)) die("key class outside domain");
                value_put(out, NULL, value_string(s));
            }
        } else if (domain_has(domain, part)) value_put(out, NULL, value_string(part));
        else if ((dash = strchr(part, '-')) != NULL) {
            char *end; long lo, hi, k;
            *dash = 0; lo = strtol(part, &end, 10); if (*end) die("invalid key range");
            hi = strtol(dash + 1, &end, 10); if (*end || lo < 0 || hi < lo || hi > 1000000)
                die("invalid key range");
            for (k = lo; k <= hi; k++) {
                char number[64]; snprintf(number, sizeof(number), "%ld", k);
                if (!domain_has(domain, number)) die("key range outside domain");
                value_put(out, NULL, value_string(number));
            }
        } else die("unknown observation key");
        if (!comma) break; part = comma + 1;
    }
    free(work);
}
static Value *keys_expand(const char *text, Value *domain, Value *classes) {
    char *work = copy(text), *amp = strchr(work, '&'); Value *out = value_new(JARR);
    if (!amp) keys_simple(out, work, domain, classes);
    else {
        size_t i; Value *other;
        *amp = 0; keys_simple(out, work, domain, classes);
        other = keys_expand(amp + 1, domain, classes);
        for (i = 0; i < out->n;) {
            size_t j; int found = 0;
            for (j = 0; j < other->n; j++)
                if (!strcmp(value_text(out->items[i].value), value_text(other->items[j].value))) found = 1;
            if (!found) { memmove(&out->items[i], &out->items[i + 1], (out->n - i - 1) * sizeof(*out->items)); out->n--; }
            else i++;
        }
    }
    free(work); return out;
}
static void drow_put(DRow *r, const char *key, const char *target, const char *actions,
                     int overlay, int fill_only) {
    DRule *slot = drule(r, key);
    if (slot->target && !overlay && !fill_only) die("overlapping delta rules");
    if (slot->target && fill_only) return;
    slot->target = copy(target); slot->actions = copy(actions);
}
static void install_delta_text(Graph *g, FILE *f, char mode, Value *domain, Value *classes,
                               Value *sequences, int overlay, int ordered, Value *skip, const char *lexer) {
    DTable table = {0}; char *s; size_t i, j;
    while ((s = line(f))) {
        char *field[4]; int n; DRow *r; Value *keys;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, field, 4); if (n != 4) die("delta table column count");
        r = drow(&table, field[0]);
        if (!strcmp(field[1], "*") && !overlay) {
            if (r->def_target) die("repeated delta default");
            r->def_target = copy(!strcmp(field[2], "$lexer") ? lexer : field[2]); r->def_actions = copy(field[3]); free(s); continue;
        }
        keys = keys_expand(field[1], domain, classes);
        for (i = 0; i < keys->n; i++) {
            const char *key = value_text(keys->items[i].value);
            drow_put(r, key, !strcmp(field[2], "$lexer") ? lexer : field[2], field[3], overlay, overlay && !strcmp(field[1], "*"));
        }
        free(s);
    }
    if (ferror(f)) die("delta table read failed");
    for (i = 0; i < table.n; i++) {
        DRow *r = &table.rows[i]; int omit = 0;
        if (skip) for (j = 0; j < skip->n; j++) if (!strcmp(value_text(skip->items[j].value), r->name)) omit = 1;
        if (omit) continue;
        if (!overlay) for (j = 0; j < domain->n; j++) {
            char number[64]; const char *key = value_scalar_text(domain->items[j].value, number);
            DRule *slot = drule(r, key);
            if (!slot->target) {
                if (!r->def_target) die("incomplete delta state");
                slot->target = r->def_target; slot->actions = r->def_actions;
            }
        }
        if (ordered) {
            for (j = 0; j < domain->n; j++) {
                char number[64]; const char *key = value_scalar_text(domain->items[j].value, number);
                DRule *rule = drule(r, key); char *acts;
                if (!rule->target) die("ordered delta state incomplete");
                acts = expand_actions(rule->actions, NULL, sequences, (int)domain->items[j].value->number);
                edge_set(g, r->name, mode, key, rule->target, acts); free(acts);
            }
        } else for (j = 0; j < r->n; j++) {
            DRule *rule = &r->rules[j]; char *acts = expand_actions(rule->actions, NULL, sequences,
                                       mode == 't' ? -1 : atoi(rule->key));
            edge_set(g, r->name, mode, rule->key, rule->target, acts); free(acts);
        }
    }
}
static FILE *buffer_file(const Buffer *b) {
    FILE *f = tmpfile();
    if (!f) die("cannot open temporary table");
    if (b->n && fwrite(b->s, 1, b->n, f) != b->n) die("temporary table write failed");
    rewind(f); return f;
}
static void output(FILE *f, const Graph *g);
static void output_graph(FILE *f, const Graph *g, const char *start, const Value *tok_names);
static Value *lex_sequences_flags(int argc, char **argv) {
    FILE *f = fopen("exec/lex/output-manifest.tsv", "rb"); char *s;
    if (!f) die("cannot open lex output manifest");
    while ((s = line(f))) {
        char *field[9]; int n; Value *opts, *facts, *sequences;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, field, 9); if (n != 9) die("lex output manifest columns");
        if (!when_true(field[3], argc, argv)) { free(s); continue; }
        opts = value_json(field[8], "lex output options");
        facts = load_facts_expr(field[4]); sequences = mapseq_construct(opts, facts);
        fclose(f); free(s); return sequences;
    }
    die("missing lex output row"); return NULL;
}
static Value *lex_sequences(void) { return lex_sequences_flags(0, NULL); }
static void inspect_pp_start(const char *outpath) {
    FILE *manifest = fopen("exec/pp/body-manifest.tsv", "rb"), *table, *out;
    char *s; Value *sequences = NULL; Graph g = {0};
    if (!manifest) die("cannot open pp body manifest");
    while ((s = line(manifest))) {
        char *field[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, field, 9); if (n != 9) die("pp manifest column count");
        if (!strcmp(field[0], "let") && !strcmp(field[4], "pp-gen+pp-layout")) {
            Value *facts = load_facts_expr(field[4]), *opts = value_json(field[8], "pp output options");
            sequences = mapseq_construct(opts, facts);
            free(s); break;
        }
        free(s);
    }
    if (ferror(manifest) || fclose(manifest) || !sequences) die("pp start sequences missing");
    table = fopen("exec/pp/start-byte.tsv", "rb"); if (!table) die("cannot open pp start table");
    install_delta_text(&g, table, 'b', numeric_domain(0, 257), NULL, sequences, 0, 0, NULL, "START");
    if (fclose(table)) die("pp start table close failed");
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output(out, &g); if (fclose(out)) die("output close failed");
}
static void inspect_delta_template(const char *section, const char *outpath) {
    Graph g = {0}; Buffer rows = expand_template_section("lex", section);
    Value *facts = load_fact("lex-gen"), *classes = value_get(facts, "classes");
    Value *domain = numeric_domain(0, 257), *sequences = lex_sequences();
    FILE *f = buffer_file(&rows), *out;
    install_delta_text(&g, f, 'b', domain, classes, sequences, 1, 0, NULL, "DISPATCH");
    if (fclose(f)) die("temporary table close failed");
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output(out, &g); if (fclose(out)) die("output close failed");
}
static void install_lex_table(Graph *g, const char *stem, Value *opts,
                              Value *facts, Value *sequences, const char *lexer) {
    Value *domain, *classes = NULL, *skip = NULL, *d, *dk;
    char path[1024]; const char *mode; int ordered; FILE *f;
    mode = value_text(value_get(opts, "mode"));
    d = value_get(opts, "domain"); dk = value_get(opts, "domain_keys");
    if (d) {
        if (d->kind != JARR || d->n != 2 || d->items[0].value->kind != JINT ||
            d->items[1].value->kind != JINT) die("invalid lex table domain");
        domain = numeric_domain((int)d->items[0].value->number, (int)d->items[1].value->number);
    } else if (dk) domain = value_path(facts, value_text(dk));
    else domain = numeric_domain(0, 257);
    d = value_get(opts, "classes"); if (d) classes = value_path(facts, value_text(d));
    skip = value_get(opts, "skip"); d = value_get(opts, "ordered");
    ordered = d && d->kind == JBOOL && d->number;
    if (snprintf(path, sizeof(path), "exec/lex/%s", stem) >= (int)sizeof(path)) die("lex table path too long");
    f = fopen(path, "rb"); if (!f) die("cannot open lex table");
    install_delta_text(g, f, mode[0], domain, classes, sequences, 0, ordered, skip, lexer);
    if (fclose(f)) die("lex table close failed");
}
static void inspect_delta_table(const char *stem, const char *outpath) {
    Graph g = {0}; FILE *manifest = fopen("exec/lex/gen-manifest.tsv", "rb"), *out;
    char *s; int found = 0;
    if (!manifest) die("cannot open lex manifest");
    while ((s = line(manifest))) {
        char *field[9]; int n; Value *opts, *facts;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, field, 9); if (n != 9) die("lex manifest column count");
        if (strcmp(field[0], "table") || strcmp(field[1], stem) || !when_true(field[3], 0, NULL)) {
            free(s); continue;
        }
        if (++found > 1) die("ambiguous lex table");
        opts = value_json(field[8], "lex table options"); facts = load_facts_expr(field[4]);
        install_lex_table(&g, stem, opts, facts, lex_sequences(), "DISPATCH");
        free(s);
    }
    if (ferror(manifest) || fclose(manifest) || found != 1) die("lex table not found");
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output(out, &g); if (fclose(out)) die("output close failed");
}
static void finish_lex(Graph *g, const char *start) {
    Value *facts = load_fact("lex-domains"), *rows = value_get(facts, "lex-domains");
    size_t i, j; int *reach; size_t *todo, nreach = 0, ntodo = 0;
    if (!rows || rows->kind != JARR) die("missing lex domains");
    for (i = 0; i < g->n; i++) for (j = 0; j < g->state[i].n; j++) {
        const char *target = g->state[i].edge[j].target; size_t k;
        if (!strcmp(target, "HALT")) continue;
        for (k = 0; k < g->n && strcmp(g->state[k].name, target); k++);
        if (k == g->n) { fprintf(stderr, "lex target state missing: %s -> %s\n", g->state[i].name, target); die("lex target state missing"); }
    }
    for (i = 0; i < g->n; i++) {
        State *st = &g->state[i]; Value *domain = NULL;
        for (j = 0; j < rows->n; j++) {
            Value *row = rows->items[j].value, *mode = value_get(row, "mode");
            if (mode && mode->kind == JSTR && mode->s[0] == st->mode) {
                Value *v = value_get(row, "values");
                domain = v->kind == JINT ? numeric_domain(0, (int)v->number) : v;
                break;
            }
        }
        if (!domain || domain->kind != JARR) die("lex mode lacks domain");
        for (j = 0; j < domain->n; j++) {
            char number[64]; const char *key = value_scalar_text(domain->items[j].value, number);
            if (!edge_has(st, key)) edge_set(g, st->name, st->mode, key, "HALT", "[[\"REJECT\",\"unreachable\"]]");
        }
    }
    reach = grow(NULL, g->n, sizeof(*reach)); memset(reach, 0, g->n * sizeof(*reach));
    todo = grow(NULL, g->n, sizeof(*todo));
    for (i = 0; i < g->n && strcmp(g->state[i].name, start); i++);
    if (i == g->n) die("lex start missing");
    todo[ntodo++] = i; reach[i] = 1;
    while (ntodo) {
        State *st = &g->state[todo[--ntodo]];
        for (j = 0; j < st->n; j++) {
            size_t k; const char *target = st->edge[j].target;
            if (!strcmp(target, "HALT")) continue;
            for (k = 0; k < g->n && strcmp(g->state[k].name, target); k++);
            if (k == g->n) die("lex target state missing after fill");
            if (!reach[k]) { reach[k] = 1; todo[ntodo++] = k; }
        }
    }
    for (i = 0, j = 0; i < g->n; i++) if (reach[i]) g->state[j++] = g->state[i];
    g->n = j; free(reach); free(todo);
}
static int value_equal(const Value *a, const Value *b) {
    size_t i;
    if (!a || !b || a->kind != b->kind || a->n != b->n) return 0;
    if (a->kind == JSTR) return !strcmp(a->s, b->s);
    if (a->kind == JINT || a->kind == JBOOL) return a->number == b->number;
    if (a->kind == JUINT) return a->unumber == b->unumber;
    for (i = 0; i < a->n; i++) {
        if (a->kind == JOBJ && strcmp(a->items[i].key, b->items[i].key)) return 0;
        if (!value_equal(a->items[i].value, b->items[i].value)) return 0;
    }
    return 1;
}
static char *value_json_text(const Value *v) {
    FILE *f = tmpfile(); long n; char *text;
    if (!f) die("cannot open temporary JSON");
    value_write(f, v);
    if (fflush(f) || (n = ftell(f)) < 0 || fseek(f, 0, SEEK_SET)) die("temporary JSON write failed");
    text = grow(NULL, (size_t)n + 1, 1);
    if (fread(text, 1, (size_t)n, f) != (size_t)n || fclose(f)) die("temporary JSON read failed");
    text[n] = 0; return text;
}
static State *find_state(Graph *g, const char *name) {
    size_t i;
    for (i = 0; i < g->n; i++) if (!strcmp(g->state[i].name, name)) return &g->state[i];
    die("template edit state missing"); return NULL;
}
typedef struct { Graph *graph; TRow *row; } EditContext;
static void edit_row_visit(Value *scope, void *arg) {
    EditContext *ctx = arg; Graph *g = ctx->graph; TRow *r = ctx->row;
    char *a = template_subst(r->a, scope), *b = template_subst(r->b, scope);
    char *c = template_subst(r->c, scope), *d = template_subst(r->d, scope);
    size_t i, j;
    if (!strcmp(r->kind, "rewrite-tail")) {
        Value *pattern = value_json(a, "rewrite-tail pattern");
        Value *replacement = value_json(d, "rewrite-tail replacement");
        if (pattern->kind != JARR || !pattern->n || replacement->kind != JARR)
            die("invalid rewrite-tail arrays");
        if (strcmp(b, "-")) die("rewrite-tail skip list is not supported yet");
        for (i = 0; i < g->n; i++) {
            State *st = &g->state[i];
            for (j = 0; j < st->n; j++) {
                Edge *e = &st->edge[j]; Value *acts = value_json(g->seq[e->seq], "edge actions");
                size_t k, old = acts->n; int match = old >= pattern->n;
                if (!match) continue;
                for (k = 0; k < pattern->n; k++)
                    if (!value_equal(acts->items[old - pattern->n + k].value,
                                     pattern->items[k].value)) { match = 0; break; }
                if (!match) continue;
                acts->n -= pattern->n;
                for (k = 0; k < replacement->n; k++)
                    value_put(acts, NULL, replacement->items[k].value);
                e->seq = seq(g, value_json_text(acts));
                if (*c && strcmp(c, "-")) e->target = copy(c);
            }
        }
    } else if (!strcmp(r->kind, "redirect")) {
        State *st = find_state(g, a); Edge *e = NULL;
        for (i = 0; i < st->n; i++) if (!strcmp(st->edge[i].key, b)) { e = &st->edge[i]; break; }
        if (!e) die("template redirect edge missing");
        e->target = copy(c); e->seq = seq(g, value_json_text(value_json(d, "redirect actions")));
    } else die("unsupported lex graph edit");
    free(a); free(b); free(c); free(d);
}
typedef struct { Graph *graph; TRow *row; } EditBlock;
static void edit_block_visit(Value *scope, void *arg) {
    EditBlock *block = arg; EditContext ctx = {block->graph, block->row};
    tuple_walk(block->row->over, load_fact("lex-gen"), scope, edit_row_visit, &ctx);
}
static void sourcefacts_lex_edits(Graph *g) {
    FILE *f = fopen("exec/lex/sourcefacts-template.tsv", "rb"); char *s;
    Value *facts = load_fact("lex-gen");
    if (!f) die("cannot open lex sourcefacts template");
    while ((s = line(f))) {
        char *field[9]; int n; TRow row; EditBlock block; Value *scope;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, field, 9); if (n != 9 || strcmp(field[0], "envelope"))
            die("invalid lex sourcefacts template row");
        row = (TRow){field[1], field[2], field[3], field[4], field[5], field[6], field[7], field[8], 0};
        block = (EditBlock){g, &row}; scope = value_new(JOBJ);
        tuple_walk(row.each, facts, scope, edit_block_visit, &block);
        free(s);
    }
    if (ferror(f) || fclose(f)) die("lex sourcefacts template read failed");
}
static void construct_lex(Graph *g, int argc, char **argv, const char *start) {
    FILE *f = fopen("exec/lex/gen-manifest.tsv", "rb"); char *s; Value *sequences = lex_sequences_flags(argc, argv);
    int envelope = has_flag(argc, argv, "typed") || has_flag(argc, argv, "positions") ||
                   has_flag(argc, argv, "locations") || has_flag(argc, argv, "sourcefacts");
    const char *inner_start = "DISPATCH";
    if (!f) die("cannot open lex manifest");
    while ((s = line(f))) {
        char *field[9]; int n; Value *opts, *facts;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, field, 9); if (n != 9) die("lex manifest column count");
        if (strcmp(field[3], "fact:envelope") ? !when_true(field[3], argc, argv) : !envelope) {
            free(s); continue;
        }
        if (!strcmp(field[0], "table")) {
            opts = value_json(field[8], "lex table options"); facts = load_facts_expr(field[4]);
            install_lex_table(g, field[1], opts, facts, sequences, inner_start);
        } else if (!strcmp(field[0], "template") && !strcmp(field[1], "gen")) {
            Buffer lines = expand_template_section("lex", field[2]);
            Value *domain = numeric_domain(0, 257), *classes;
            FILE *rows = buffer_file(&lines);
            facts = load_facts_expr(field[4]); classes = value_get(facts, "classes");
            install_delta_text(g, rows, 'b', domain, classes, sequences, 1, 0, NULL, "DISPATCH");
            if (fclose(rows)) die("lex template close failed");
        } else if (!strcmp(field[0], "template") && !strcmp(field[1], "sourcefacts")) {
            sourcefacts_lex_edits(g);
        } else if (!strcmp(field[0], "let")) {
            if (!strcmp(field[7], "START=@str:LOC.magic0")) inner_start = "LOC.magic0";
            else if (!strcmp(field[7], "START=@str:SF.start")) inner_start = "SF.start";
        } else if (strcmp(field[0], "call")) die("unsupported lex manifest op");
        free(s);
    }
    if (ferror(f) || fclose(f)) die("lex manifest read failed");
    finish_lex(g, start);
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
static void finish(Graph *g);
/* Four-column form shared by opt/lower/parse rows. The caller supplies only
   source facts and generated bindings; the row reader owns the rule order. */
static const char *bound_name(const char *s, Value *bindings) {
    return *s == '$' ? value_text(value_get(bindings, s + 1)) : s;
}
static void install_plain_classes(Graph *g, const char *path, char mode,
                                  Value *bindings, Value *sequences, Value *classes) {
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
        } else rule_keys_classes(st, field[1], bound_name(field[2], bindings), field[3], classes);
        free(s);
    }
    if (ferror(f) || fclose(f)) die("rule read failed");
    if (!rules.n) die("empty rule file");
    for (size_t i = 0; i < rules.n; i++) {
        RuleState *st = &rules.state[i]; char key[16];
        int variable = st->def_actions && actions_depend_on_key(st->def_actions, sequences);
        char *fixed = (!st->def_actions || variable) ? NULL :
                      expand_actions(st->def_actions, bindings, sequences, 0);
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
                actions = variable ? expand_actions(st->def_actions, bindings, sequences, k) : fixed;
                number_text(k, key); edge_add(g, st->name, mode, key, st->def_target, actions);
                if (variable) free(actions);
            }
        }
        free(fixed);
    }
}
static void install_plain(Graph *g, const char *path, char mode,
                          Value *bindings, Value *sequences) {
    install_plain_classes(g, path, mode, bindings, sequences, NULL);
}
static void install_section_classes(Graph *g, const char *path, const char *section,
                                    char mode, Value *bindings, Value *sequences,
                                    Value *classes) {
    FILE *f = fopen(path, "rb"); char *s; RuleSet rules = {0};
    if (!f) die("cannot open section rule file");
    while ((s = line(f))) {
        char *field[5]; int n; RuleState *st;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, field, 5); if (n != 5) die("section rule column count");
        if (strcmp(field[0], section)) { free(s); continue; }
        if (!*field[1] || !*field[3]) die("empty section state or target");
        st = rule_state(&rules, bound_name(field[1], bindings));
        if (!strcmp(field[2], "*")) {
            if (st->def_target) die("repeated section default");
            st->def_target = copy(bound_name(field[3], bindings));
            st->def_actions = copy(field[4]);
        } else rule_keys_classes(st, field[2], bound_name(field[3], bindings), field[4], classes);
        free(s);
    }
    if (ferror(f) || fclose(f)) die("section rule read failed");
    for (size_t i = 0; i < rules.n; i++) {
        RuleState *st = &rules.state[i]; char key[16];
        int variable = st->def_actions && actions_depend_on_key(st->def_actions, sequences);
        char *fixed = (!st->def_actions || variable) ? NULL :
                      expand_actions(st->def_actions, bindings, sequences, 0);
        for (size_t j = 0; j < st->n; j++) {
            char *actions = expand_actions(st->rule[j].actions, bindings, sequences, st->rule[j].key);
            number_text(st->rule[j].key, key);
            edge_add(g, st->name, mode, key, st->rule[j].target, actions); free(actions);
        }
        for (int k = 0; k <= 256; k++) {
            int found = 0;
            for (size_t j = 0; j < st->n; j++) if (st->rule[j].key == k) { found = 1; break; }
            if (!found) {
                char *actions;
                if (!st->def_target) die("incomplete section state");
                actions = variable ? expand_actions(st->def_actions, bindings, sequences, k) : fixed;
                number_text(k, key); edge_add(g, st->name, mode, key, st->def_target, actions);
                if (variable) free(actions);
            }
        }
        free(fixed);
    }
}
static void install_section(Graph *g, const char *path, const char *section,
                            char mode, Value *bindings, Value *sequences) {
    install_section_classes(g, path, section, mode, bindings, sequences, NULL);
}
static void install_prn_call(Graph *g) {
    FILE *f = fopen("exec/parse/prn-manifest.tsv", "rb"); char *s;
    Value *instances = NULL; char *factexpr = NULL, *bind = NULL, *opts_text = NULL;
    int loops = 0, body = 0;
    if (!f) die("cannot open called manifest");
    while ((s = line(f))) {
        char *field[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, field, 9); if (n != 9) die("called manifest column count");
        if (!strcmp(field[0], "foreach")) {
            Value *facts, *opts;
            if (loops++ || strcmp(field[4], "numeric-instances")) die("unsupported foreach declaration");
            facts = load_fact(field[4]); opts = value_json(field[8], "foreach options");
            if (strcmp(value_text(value_get(opts, "over")), field[4])) die("unsupported foreach source");
            instances = value_path(facts, field[4]);
        } else if (!strcmp(field[0], ".rows")) {
            if (body++ || strcmp(field[1], "numeric") || strcmp(field[2], "prn")) die("unsupported foreach body");
            factexpr = copy(field[4]); bind = copy(field[7]); opts_text = copy(field[8]);
        } else die("unsupported called DSL op");
        free(s);
    }
    if (ferror(f) || fclose(f)) die("called manifest read failed");
    if (loops != 1 || body != 1 || !instances || instances->kind != JARR) die("incomplete called manifest");
    for (size_t i = 0; i < instances->n; i++) {
        Value *item = instances->items[i].value, *facts = load_facts_expr(factexpr);
        Value *opts = value_json(opts_text, "called row options");
        Value *bindings, *empty = value_new(JOBJ);
        const char *name = value_text(value_get(item, "name"));
        value_put(facts, "name", value_string(name));
        value_put(facts, "width", value_get(item, "width"));
        bindings = fresh_bindings(opts, facts); direct_bindings(bindings, bind, facts);
        install_section(g, "exec/parse/numeric-byte.tsv", "prn", 'b', bindings, empty);
        install_section(g, "exec/parse/numeric-result.tsv", "prn", 'r', bindings, empty);
    }
}
static void install_numout_call(Graph *g) {
    FILE *f = fopen("exec/parse/numout-manifest.tsv", "rb"); char *s;
    int rows = 0;
    if (!f) die("cannot open numout manifest");
    while ((s = line(f))) {
        char *field[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, field, 9);
        if (n != 9 || strcmp(field[0], "rows") || strcmp(field[1], "numeric") ||
            strcmp(field[2], "numout") || strcmp(field[3], "-"))
            die("unsupported numout manifest row");
        {
            Value *facts = load_facts_expr(field[4]);
            Value *opts = value_json(field[8], "numout options");
            Value *bindings = fresh_bindings(opts, facts);
            Value *empty = value_new(JOBJ);
            direct_bindings(bindings, field[7], facts);
            install_section(g, "exec/parse/numeric-byte.tsv", "numout", 'b', bindings, empty);
            install_section(g, "exec/parse/numeric-result.tsv", "numout", 'r', bindings, empty);
        }
        rows++;
        free(s);
    }
    if (ferror(f) || fclose(f) || rows != 1) die("numout manifest read failed");
}
static Value *classes_for(Value *opts, Value *facts) {
    Value *spec = value_get(opts, "classmap"), *classes = value_new(JOBJ);
    if (!spec) return classes;
    if (spec->kind != JOBJ) die("invalid classmap");
    for (size_t i = 0; i < spec->n; i++) {
        Value *source = value_path(facts, value_text(spec->items[i].value));
        Value *array = value_new(JARR);
        if (source->kind == JARR) array = source;
        else value_put(array, NULL, source);
        value_put(classes, spec->items[i].key, array);
    }
    return classes;
}
static void install_opt_rows(Graph *g, const char *stem, const char *section,
                             Value *opts, Value *facts, const char *bind,
                             Value *sequences, Value **exported) {
    char path[1024]; Value *bindings = fresh_bindings(opts, facts);
    Value *bindmap = value_get(opts, "bindmap"), *classes = classes_for(opts, facts);
    if (bindmap) {
        Value *map = value_path(facts, value_text(bindmap));
        if (map->kind != JOBJ) die("bindmap is not an object");
        for (size_t i = 0; i < map->n; i++) value_put(bindings, map->items[i].key, map->items[i].value);
    }
    direct_bindings(bindings, bind, facts);
    if (exported) *exported = bindings;
    for (int i = 0; i < 2; i++) {
        if (snprintf(path, sizeof(path), "exec/opt/%s-%s.tsv", stem, i ? "result" : "byte") >= (int)sizeof(path))
            die("opt rule path too long");
        if (section && strcmp(section, "-"))
            install_section_classes(g, path, section, i ? 'r' : 'b', bindings, sequences, classes);
        else install_plain_classes(g, path, i ? 'r' : 'b', bindings, sequences, classes);
    }
}
static void install_opt_answer(Graph *g, Value *facts, Value *bindings) {
    FILE *f = fopen("exec/opt/setup-template.tsv", "rb"); char *s; int rows = 0;
    Value *y = value_get(facts, "Y");
    if (!f || !y || y->kind != JARR) die("invalid opt answer template");
    while ((s = line(f))) {
        char *field[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, field, 9);
        if (n != 9 || strcmp(field[0], "answer") || strcmp(field[1], "answer") ||
            strcmp(field[4], "rule") || strcmp(field[8], "[]")) die("unsupported opt answer template row");
        if (!strcmp(field[3], "Y")) {
            for (size_t i = 0; i < y->n; i++) {
                Value *item = y->items[i].value;
                char key[32]; const char *name = bound_name(field[5], bindings);
                Value *ix = value_get(item, "i");
                if (!ix || ix->kind != JINT) die("invalid opt answer index");
                number_text((int)ix->number, key);
                edge_add(g, name, 'r', key, value_text(value_get(item, "target")), "[]");
            }
        } else if (!strcmp(field[3], "-")) {
            const char *name = bound_name(field[5], bindings);
            for (int k = 0; k <= 256; k++) {
                char key[16]; number_text(k, key);
                edge_add(g, name, 'r', key, field[7], "[]");
            }
        } else die("unsupported opt answer iterator");
        rows++; free(s);
    }
    if (ferror(f) || fclose(f) || rows != 2) die("opt answer template read failed");
}
static void inspect_pp_emit(const char *outpath, size_t index) {
    FILE *manifest = fopen("exec/pp/autoinc-manifest.tsv", "rb"), *out;
    Value *facts = load_facts_expr("pp-autoinc-gen+pp-layout"), *emits = value_get(facts, "emits");
    Graph g = {0}; char *s; int found = 0;
    if (!manifest || !emits || emits->kind != JARR || index >= emits->n) die("invalid pp emit index");
    value_put(facts, "it", emits->items[index].value);
    {
        Value *it = value_get(facts, "it"), *special = value_get(it, "special"), *terminal = value_get(it, "terminal");
        char entry[64], test[64], need[64], next[64];
        if (!special || !terminal || special->kind != JBOOL || terminal->kind != JBOOL) die("invalid pp emit facts");
        if (special->number) { strcpy(entry, "AEM"); strcpy(test, "AEMR"); strcpy(need, "RTP"); }
        else {
            if (snprintf(entry, sizeof(entry), "AEM%lld", value_get(it, "id")->number) >= (int)sizeof(entry) ||
                snprintf(test, sizeof(test), "AEM%lldr", value_get(it, "id")->number) >= (int)sizeof(test) ||
                snprintf(need, sizeof(need), "NEED%lld", value_get(it, "id")->number) >= (int)sizeof(need)) die("pp emit name too long");
        }
        if (terminal->number) strcpy(next, "ACP0");
        else if (snprintf(next, sizeof(next), "AEM%lld", value_get(it, "end")->number) >= (int)sizeof(next))
            die("pp emit successor too long");
        value_put(facts, "emit_entry", value_string(entry)); value_put(facts, "emit_test", value_string(test));
        value_put(facts, "emit_need", value_string(need)); value_put(facts, "emit_next", value_string(next));
    }
    while ((s = line(manifest))) {
        char *field[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, field, 9); if (n != 9) die("pp autoinc manifest column count");
        if (!strcmp(field[0], ".rows") && !strcmp(field[1], "autoinc-emit")) {
            Value *bindings = value_new(JOBJ), *opts = value_json(field[8], "pp emit options");
            Value *sequences = mapseq_construct(opts, facts);
            direct_bindings(bindings, field[7], facts);
            install_plain(&g, "exec/pp/autoinc-emit-byte.tsv", 'b', bindings, sequences);
            install_plain(&g, "exec/pp/autoinc-emit-result.tsv", 'r', bindings, sequences);
            found++;
        }
        free(s);
    }
    if (ferror(manifest) || fclose(manifest) || found != 1) die("pp emit row missing");
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output(out, &g); if (fclose(out)) die("output close failed");
}
static void inspect_pp_object_predefine(const char *outpath) {
    FILE *manifest = fopen("exec/pp/body-manifest.tsv", "rb"), *out;
    Graph g = {0}; Value *empty = value_new(JOBJ); char *s; int found = 0;
    if (!manifest) die("cannot open pp body manifest");
    while ((s = line(manifest))) {
        char *field[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, field, 9); if (n != 9) die("pp manifest column count");
        if (!strcmp(field[0], "rows") && !strcmp(field[1], "object-predefine")) {
            if (strcmp(field[2], "-") || strcmp(field[4], "-") || strcmp(field[7], "-") ||
                strcmp(field[8], "-")) die("unsupported pp object-predefine row");
            install_plain(&g, "exec/pp/object-predefine-byte.tsv", 'b', empty, NULL);
            install_plain(&g, "exec/pp/object-predefine-result.tsv", 'r', empty, NULL);
            found++;
        }
        free(s);
    }
    if (ferror(manifest) || fclose(manifest) || found != 1) die("pp object-predefine row missing");
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output(out, &g); if (fclose(out)) die("output close failed");
}
static void pp_install_stem(Graph *g, const char *stem, const char *section,
                            Value *bindings, Value *sequences) {
    char path[1024];
    for (int i = 0; i < 2; i++) {
        if (snprintf(path, sizeof(path), "exec/pp/%s-%s.tsv", stem, i ? "result" : "byte") >= (int)sizeof(path))
            die("pp stem path too long");
        if (section && strcmp(section, "-"))
            install_section(g, path, section, i ? 'r' : 'b', bindings, sequences);
        else install_plain(g, path, i ? 'r' : 'b', bindings, sequences);
    }
}
static void pp_install_prefix(Graph *g, int no_autoinc) {
    FILE *manifest = fopen("exec/pp/body-manifest.tsv", "rb");
    Value *sequences = NULL; char *s; int start = 0, rows = 0;
    char *flags[] = {"--no-autoinc"};
    if (!manifest) die("cannot open pp body manifest");
    while ((s = line(manifest))) {
        char *field[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, field, 9); if (n != 9) die("pp manifest column count");
        if (!strcmp(field[0], "call")) { free(s); break; }
        if (!when_true(field[3], no_autoinc, flags)) { free(s); continue; }
        if (!strcmp(field[0], "let") && !strcmp(field[4], "pp-gen+pp-layout")) {
            sequences = mapseq_construct(value_json(field[8], "pp prefix options"), load_facts_expr(field[4]));
        } else if (!strcmp(field[0], "table") && !strcmp(field[1], "start-byte.tsv")) {
            FILE *table = fopen("exec/pp/start-byte.tsv", "rb");
            if (!table || !sequences || start++) die("invalid pp prefix start");
            install_delta_text(g, table, 'b', numeric_domain(0, 257), NULL, sequences, 0, 0, NULL, "START");
            if (fclose(table)) die("pp start table close failed");
        } else if (!strcmp(field[0], "rows") &&
                   (!strcmp(field[1], "cli") || !strcmp(field[1], "text") ||
                    !strcmp(field[1], "macro") || !strcmp(field[1], "pragma"))) {
            Value *bindings = value_new(JOBJ), *facts = load_facts_expr(field[4]);
            if (strcmp(field[2], "-") || strcmp(field[8], "-")) die("unsupported pp prefix row");
            direct_bindings(bindings, field[7], facts);
            pp_install_stem(g, field[1], NULL, bindings,
                            !strcmp(field[1], "cli") ? sequences : NULL);
            rows++;
        } else die("unsupported pp prefix manifest row");
        free(s);
    }
    if (ferror(manifest) || fclose(manifest) || start != 1 || rows != 4)
        die("incomplete pp prefix");
}
static void inspect_pp_prefix(const char *outpath, int no_autoinc) {
    Graph g = {0}; FILE *out;
    pp_install_prefix(&g, no_autoinc);
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output(out, &g); if (fclose(out)) die("output close failed");
}
typedef struct { int depth; char *raw; char *field[9]; } PPRow;
static Value *pp_env_copy(Value *env) {
    Value *copy = value_new(JOBJ);
    for (size_t i = 0; i < env->n; i++) value_put(copy, env->items[i].key, env->items[i].value);
    return copy;
}
static int pp_fact_when(const char *when, Value *facts) {
    int neg = *when == '!'; const char *key = when + neg;
    Value *v;
    if (strncmp(key, "fact:", 5)) die("unsupported pp fact condition");
    v = value_get(facts, key + 5);
    return !!(v && ((v->kind == JBOOL || v->kind == JINT) ? v->number : v->n)) != neg;
}
static void pp_autoinc_block(Graph *g, PPRow *rows, size_t lo, size_t hi, int depth, Value *env) {
    for (size_t i = lo; i < hi;) {
        PPRow *r = &rows[i]; char **f = r->field; size_t j = i + 1;
        Value *facts = load_facts_expr(f[4]), *opts;
        while (j < hi && rows[j].depth > depth) j++;
        if (r->depth != depth) die("invalid pp manifest nesting");
        for (size_t k = 0; k < env->n; k++) value_put(facts, env->items[k].key, env->items[k].value);
        opts = !strcmp(f[8], "-") ? value_new(JOBJ) : value_json(f[8], "pp autoinc options");
        if (!strcmp(f[0] + depth, "foreach")) {
            Value *over = value_get(opts, "over"), *items = value_path(facts, value_text(over));
            Value *as = value_get(opts, "as"), *pre = value_get(opts, "pre");
            if (items->kind != JARR || j == i + 1) die("invalid pp foreach");
            for (size_t n = 0; n < items->n; n++) {
                Value *child = pp_env_copy(env), *ctx = pp_env_copy(facts);
                value_put(child, as ? value_text(as) : "it", items->items[n].value);
                value_put(ctx, as ? value_text(as) : "it", items->items[n].value);
                if (pre) for (size_t p = 0; p < pre->n; p++) {
                    Value *pair = pre->items[p].value;
                    if (pair->kind != JARR || pair->n != 2) die("invalid pp foreach pre");
                    value_put(child, value_text(pair->items[0].value),
                              value_path(ctx, value_text(pair->items[1].value)));
                }
                pp_autoinc_block(g, rows, i + 1, j, depth + 1, child);
            }
        } else if (strncmp(f[3], "fact:", 5) && strncmp(f[3], "!fact:", 6) && strcmp(f[3], "-")) {
            die("unsupported pp autoinc condition");
        } else if (!strcmp(f[3], "-") || pp_fact_when(f[3], facts)) {
            if (!strcmp(f[0] + depth, "let")) {
                Value *b = value_new(JOBJ);
                direct_bindings(b, f[7], facts);
                for (size_t k = 0; k < b->n; k++) value_put(env, b->items[k].key, b->items[k].value);
            } else if (!strcmp(f[0] + depth, "rows")) {
                Value *bindings = value_new(JOBJ), *sequences = mapseq_construct(opts, facts);
                Value *classmap = value_get(opts, "classmap"), *classes = NULL;
                char path[1024];
                direct_bindings(bindings, f[7], facts);
                if (classmap) {
                    classes = value_new(JOBJ);
                    for (size_t k = 0; k < classmap->n; k++)
                        value_put(classes, classmap->items[k].key,
                                  value_path(facts, value_text(classmap->items[k].value)));
                }
                for (int k = 0; k < 2; k++) {
                    if (snprintf(path, sizeof(path), "exec/pp/%s-%s.tsv", f[1], k ? "result" : "byte") >= (int)sizeof(path))
                        die("pp autoinc path too long");
                    if (strcmp(f[2], "-"))
                        install_section_classes(g, path, f[2], k ? 'r' : 'b', bindings, sequences, classes);
                    else install_plain_classes(g, path, k ? 'r' : 'b', bindings, sequences, classes);
                }
            } else die("unsupported pp autoinc op");
        }
        i = j;
    }
}
static void pp_install_call_autoinc(Graph *g) {
    FILE *f = fopen("exec/pp/autoinc-manifest.tsv", "rb");
    PPRow *rows = NULL; size_t n = 0, cap = 0; char *s;
    Value *env = value_new(JOBJ);
    if (!f) die("cannot open pp autoinc manifest");
    while ((s = line(f))) {
        PPRow *r; char *op; int cols, depth = 0;
        if (!*s || *s == '#') { free(s); continue; }
        if (n == cap) { cap = cap ? cap * 2 : 32; rows = grow(rows, cap, sizeof(*rows)); }
        r = &rows[n++]; r->raw = s; cols = fields_tab(s, r->field, 9);
        if (cols != 9) die("pp autoinc manifest column count");
        op = r->field[0]; while (op[depth] == '.') depth++;
        r->depth = depth;
    }
    if (ferror(f) || fclose(f)) die("pp autoinc manifest read failed");
    pp_autoinc_block(g, rows, 0, n, 0, env);
}
static void inspect_pp_call_autoinc(const char *outpath) {
    Graph g = {0}; FILE *out;
    pp_install_call_autoinc(&g);
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output(out, &g); if (fclose(out)) die("output close failed");
}
static void inspect_pp_through_autoinc(const char *outpath, int no_autoinc) {
    Graph g = {0}; FILE *out;
    pp_install_prefix(&g, no_autoinc);
    if (!no_autoinc) pp_install_call_autoinc(&g);
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output(out, &g); if (fclose(out)) die("output close failed");
}
static void pp_install_body_before_dsw(Graph *g) {
    FILE *f = fopen("exec/pp/body-manifest.tsv", "rb"); char *s;
    Value *seqenv = NULL;
    int active = 0, scans = 0, obj = 0, assembly = 0, linedir = 0;
    if (!f) die("cannot open pp body manifest");
    while ((s = line(f))) {
        char *field[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, field, 9); if (n != 9) die("pp body column count");
        if (!strcmp(field[0], "let") && !strcmp(field[4], "pp-gen+pp-layout")) {
            seqenv = mapseq_construct(value_json(field[8], "pp body sequences"), load_facts_expr(field[4]));
        }
        if (!active && !strcmp(field[0], "call") && !strcmp(field[1], "autoinc")) {
            active = 1; free(s); continue;
        }
        if (!active) { free(s); continue; }
        if (!strcmp(field[0], "template") && !strcmp(field[1], "dsw")) { free(s); break; }
        if (!strcmp(field[0], "rows") && !strcmp(field[1], "directive-scan")) {
            Value *facts = load_facts_expr(field[4]), *bind = value_new(JOBJ);
            direct_bindings(bind, field[7], facts); pp_install_stem(g, field[1], NULL, bind, NULL); scans++;
        } else if (!strcmp(field[0], "rows") && !strcmp(field[1], "object-predefine")) {
            pp_install_stem(g, field[1], NULL, value_new(JOBJ), NULL); obj++;
        } else if (!strcmp(field[0], "rows") && !strcmp(field[1], "assembly") &&
                   !strcmp(field[2], "predefine")) {
            Value *facts = load_facts_expr(field[4]), *bind = value_new(JOBJ), *seqs = value_new(JOBJ);
            if (!seqenv || !value_get(seqenv, "objname")) die("pp object name sequence missing");
            direct_bindings(bind, field[7], facts); value_put(seqs, "name", value_get(seqenv, "objname"));
            pp_install_stem(g, field[1], field[2], bind, seqs); assembly++;
        } else if (!strcmp(field[0], "rows") && !strcmp(field[1], "linedir")) {
            Value *facts = load_facts_expr(field[4]), *bind = value_new(JOBJ);
            direct_bindings(bind, field[7], facts); pp_install_stem(g, field[1], NULL, bind, NULL); linedir++;
        } else if (!strcmp(field[0], "rows") && !strcmp(field[1], "shared-predefine")) {
            /* This row belongs to --shared-predefines, not the default product route. */
        } else if (!strcmp(field[0], "foreach")) {
            Value *facts = load_facts_expr(field[4]);
            Value *predef = value_get(value_get(facts, "predef"), "lnx/x86_64");
            Value *opts = value_json("{\"mapseq\":{\"pname\":[{\"acts\":[[\"SBCLR\"],[\"@bytes\",\"{it[name]}\"]]}]}}", "pp predefine sequences");
            if (!predef || predef->kind != JARR || strcmp(field[8], "{\"over\": \"predef.{target}\"}"))
                die("unsupported pp target predefinitions");
            for (size_t i = 0; i < predef->n; i++) {
                Value *ctx = load_facts_expr("pp-gen+pp-layout"), *bind = value_new(JOBJ);
                Value *seqs;
                value_put(ctx, "it", predef->items[i].value);
                value_put(bind, "name", value_get(predef->items[i].value, "name"));
                value_put(bind, "entry", value_get(predef->items[i].value, "entry"));
                value_put(bind, "resume", value_get(predef->items[i].value, "resume"));
                value_put(bind, "next", value_get(predef->items[i].value, "next"));
                value_put(bind, "F_BODY", value_get(ctx, "F_BODY"));
                seqs = mapseq_construct(opts, ctx);
                value_put(seqs, "name", value_get(seqs, "pname"));
                pp_install_stem(g, "assembly", "predefine", bind, seqs);
            }
            free(s); break;
        } else die("unsupported pp body row before dsw");
        free(s);
    }
    if (ferror(f) || fclose(f) || scans != 1 || obj != 1 || assembly != 1 || linedir != 1)
        die("incomplete pp body before dsw");
}
static void pp_install_dsw(Graph *g) {
    FILE *f = fopen("exec/pp/body-manifest.tsv", "rb"); char *s; int found = 0;
    if (!f) die("cannot open pp body manifest");
    while ((s = line(f))) {
        char *field[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, field, 9); if (n != 9) die("pp body column count");
        if (!strcmp(field[0], "template") && !strcmp(field[1], "dsw")) {
            Value *facts = load_facts_expr(field[4]), *opts = value_json(field[8], "pp dsw options");
            Value *domain = value_path(facts, value_text(value_get(opts, "domain_keys")));
            Buffer lines = expand_template_file("exec/pp/dsw-template.tsv", facts, field[2]);
            FILE *table = buffer_file(&lines);
            if (strcmp(value_text(value_get(opts, "mode")), "r")) die("pp dsw mode changed");
            install_delta_text(g, table, 'r', domain, NULL, NULL, 0, 0, NULL, "START");
            if (fclose(table)) die("pp dsw table close failed");
            found++;
        }
        free(s);
    }
    if (ferror(f) || fclose(f) || found != 1) die("pp dsw declaration missing");
}
static void pp_install_body_after_dsw(Graph *g) {
    FILE *f = fopen("exec/pp/body-manifest.tsv", "rb"); char *s; int active = 0;
    int actions = 0, rows = 0, escape = 0, accept = 0;
    if (!f) die("cannot open pp body manifest");
    while ((s = line(f))) {
        char *field[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, field, 9); if (n != 9) die("pp body column count");
        if (!active && !strcmp(field[0], "template") && !strcmp(field[1], "dsw")) {
            active = 1; free(s); continue;
        }
        if (!active) { free(s); continue; }
        if (!strcmp(field[0], "call") && !strcmp(field[1], "sourcefacts")) { free(s); break; }
        if (!strcmp(field[0], "foreach") && !strcmp(field[4], "pp-gen+pp-layout")) {
            Value *facts = load_facts_expr(field[4]), *items = value_get(facts, "dactions");
            char *body = line(f);
            if (!items || items->kind != JARR || !body) die("invalid pp directive action loop");
            for (size_t i = 0; i < items->n; i++) {
                Value *ctx = load_facts_expr(field[4]), *bind = value_new(JOBJ);
                Value *it = items->items[i].value;
                char *bf[9]; char *local = copy(body);
                if (fields_tab(local, bf, 9) != 9 || strcmp(bf[0], ".rows") ||
                    strcmp(bf[1], "directive-action")) die("unsupported pp directive action body");
                value_put(ctx, "it", it); value_put(ctx, "name", value_get(it, "name"));
                value_put(ctx, "section", value_get(it, "section"));
                direct_bindings(bind, bf[7], ctx);
                pp_install_stem(g, bf[1], value_text(value_get(it, "section")), bind, NULL);
                free(local);
            }
            actions++; free(body);
        } else if (!strcmp(field[0], "rows") && !strcmp(field[1], "assembly") &&
                   !strcmp(field[2], "accept")) {
            pp_install_stem(g, "assembly", "accept", value_new(JOBJ), NULL); accept++;
        } else if (!strcmp(field[0], "template") && !strcmp(field[1], "escape")) {
            Value *facts = load_facts_expr(field[4]), *opts = value_json(field[8], "pp escape options");
            Value *domain = value_path(facts, value_text(value_get(opts, "domain_keys")));
            Buffer lines = expand_template_file("exec/pp/escape-template.tsv", facts, field[2]);
            FILE *table = buffer_file(&lines);
            if (strcmp(value_text(value_get(opts, "mode")), "b")) die("pp escape mode changed");
            install_delta_text(g, table, 'b', domain, NULL, NULL, 0, 0, NULL, "START");
            if (fclose(table)) die("pp escape table close failed");
            escape++;
        } else if (!strcmp(field[0], "rows") &&
                   (!strcmp(field[1], "directive-body") || !strcmp(field[1], "include-location") ||
                    !strcmp(field[1], "rescan") || !strcmp(field[1], "literal") ||
                    !strcmp(field[1], "expression") || !strcmp(field[1], "reduce") ||
                    !strcmp(field[1], "hash"))) {
            Value *facts = load_facts_expr(field[4]), *bind = value_new(JOBJ);
            Value *opts = !strcmp(field[8], "-") ? NULL : value_json(field[8], "pp row options");
            Value *bindmap = opts ? value_get(opts, "bindmap") : NULL;
            if (bindmap) {
                Value *extra = value_path(facts, value_text(bindmap));
                if (extra->kind != JOBJ) die("pp bindmap is not an object");
                for (size_t i = 0; i < extra->n; i++)
                    value_put(bind, extra->items[i].key, extra->items[i].value);
            }
            direct_bindings(bind, field[7], facts);
            pp_install_stem(g, field[1], NULL, bind, NULL); rows++;
        } else if (!strcmp(field[0], "call") && !strcmp(field[1], "locations")) {
            /* No --locations flag in the default product route. */
        } else if (!strcmp(field[0], ".rows")) {
            /* Consumed with the preceding foreach declaration. */
        } else die("unsupported pp body row after dsw");
        free(s);
    }
    if (ferror(f) || fclose(f) || actions != 1 || rows != 7 || escape != 1 || accept != 1)
        die("incomplete pp body after dsw");
}
static void pp_move_state(Graph *g, const char *from, const char *to) {
    size_t i;
    for (i = 0; i < g->n; i++) if (!strcmp(g->state[i].name, from)) break;
    if (i == g->n) die("pp sourcefacts state to move missing");
    for (size_t j = 0; j < g->n; j++) if (!strcmp(g->state[j].name, to))
        die("pp sourcefacts destination exists");
    {
        State st = g->state[i];
        memmove(g->state + i, g->state + i + 1, (g->n - i - 1) * sizeof(*g->state));
        st.name = copy(to); g->state[g->n - 1] = st;
    }
    free(g->state_index); g->state_index = NULL; g->index_cap = 0;
    state_index_grow(g);
}
static void pp_copy_state(Graph *g, const char *from, const char *to) {
    State *old = find_state(g, from), *st;
    size_t n = old->n;
    char mode = old->mode;
    if (g->n == g->cap) {
        g->cap = g->cap ? g->cap * 2 : 64;
        g->state = grow(g->state, g->cap, sizeof(*g->state));
    }
    st = graph_state(g, to, mode);
    if (st->n) die("pp sourcefacts copy destination exists");
    old = find_state(g, from); /* graph_state may have reallocated the state array */
    if (st->cap < n) { st->cap = n; st->edge = grow(st->edge, n, sizeof(*st->edge)); }
    for (size_t i = 0; i < n; i++) {
        st->edge[i].key = copy(old->edge[i].key);
        st->edge[i].target = copy(old->edge[i].target);
        st->edge[i].seq = old->edge[i].seq;
        if (st->key_index) {
            int key = numeric_key(st->edge[i].key);
            if (key >= 0) st->key_index[key] = (int)i + 1;
        }
    }
    st->n = n;
}
static void pp_sourcefacts_edit(Graph *g, const char *section) {
    FILE *f = fopen("exec/pp/sourcefacts-template.tsv", "rb"); char *s; int edits = 0;
    if (!f) die("cannot open pp sourcefacts template");
    while ((s = line(f))) {
        char *field[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, field, 9); if (n != 9) die("pp sourcefacts template column count");
        if (strcmp(field[0], section)) { free(s); continue; }
        if (strcmp(field[1], "entry") || strcmp(field[2], "-") || strcmp(field[3], "-"))
            die("unsupported pp sourcefacts edit loop");
        if (!strcmp(field[4], "move-state")) pp_move_state(g, field[5], field[6]);
        else if (!strcmp(field[4], "label")) label_add(g, field[5]);
        else if (!strcmp(field[4], "rewrite-tail")) {
            if (strcmp(field[5], "[[\"ACCEPT\"]]") || *field[6] ||
                strcmp(field[7], "SF.finish") || strcmp(field[8], "[]"))
                die("unsupported pp sourcefacts tail rewrite");
            {
                size_t old_n = g->ns;
                int *rewritten = grow(NULL, old_n, sizeof(*rewritten));
                for (size_t i = 0; i < old_n; i++) rewritten[i] = -1;
                for (size_t i = 0; i < g->n; i++) {
                    State *st = &g->state[i];
                    for (size_t j = 0; j < st->n; j++) {
                        Edge *e = &st->edge[j]; int id = e->seq;
                        if ((size_t)id >= old_n) die("sourcefacts sequence index changed");
                        if (rewritten[id] == -1) {
                            rewritten[id] = -2;
                            if (strstr(g->seq[id], "\"ACCEPT\"")) {
                                Value *acts = value_json(g->seq[id], "pp sourcefacts edge actions");
                                if (acts->n) {
                                    Value *last = acts->items[acts->n - 1].value;
                                    if (last->kind == JARR && last->n == 1 &&
                                        !strcmp(value_text(last->items[0].value), "ACCEPT")) {
                                        acts->n--;
                                        rewritten[id] = seq(g, value_json_text(acts));
                                    }
                                }
                            }
                        }
                        if (rewritten[id] >= 0) {
                            e->target = copy(field[7]); e->seq = rewritten[id];
                        }
                    }
                }
                free(rewritten);
            }
        } else if (!strcmp(field[4], "copy")) pp_copy_state(g, field[6], field[5]);
        else die("unsupported pp sourcefacts edit");
        edits++; free(s);
    }
    if (ferror(f) || fclose(f) || edits != (!strcmp(section, "pre") ? 3 : 1))
        die("pp sourcefacts edits missing");
}
static void pp_install_sourcefacts(Graph *g) {
    Value *empty = value_new(JOBJ);
    pp_sourcefacts_edit(g, "pre");
    pp_install_stem(g, "sourcefacts", NULL, empty, NULL);
    pp_sourcefacts_edit(g, "post");
}
static void inspect_pp_through_linedir(const char *outpath, int with_dsw) {
    Graph g = {0}; FILE *out;
    pp_install_prefix(&g, 0); pp_install_call_autoinc(&g); pp_install_body_before_dsw(&g);
    if (with_dsw) pp_install_dsw(&g);
    if (with_dsw > 1) pp_install_body_after_dsw(&g);
    if (with_dsw > 2) { pp_install_sourcefacts(&g); finish(&g); }
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output(out, &g); if (fclose(out)) die("output close failed");
}
static void inspect_pp_header(const char *outpath, size_t index) {
    FILE *manifest = fopen("exec/pp/autoinc-manifest.tsv", "rb"), *out;
    Value *facts = load_facts_expr("pp-autoinc-gen+pp-layout"), *headers = value_get(facts, "headers");
    Graph g = {0}; char *s; int found = 0;
    if (!manifest || !headers || headers->kind != JARR || index >= headers->n) die("invalid pp header index");
    value_put(facts, "it", headers->items[index].value);
    {
        Value *it = value_get(facts, "it"), *terminal = value_get(it, "terminal"); char next[64];
        if (terminal->kind != JBOOL) die("invalid pp header terminal");
        if (terminal->number) strcpy(next, "AEM");
        else if (snprintf(next, sizeof(next), "AH%lld_0", value_get(it, "end")->number) >= (int)sizeof(next))
            die("pp header successor too long");
        value_put(facts, "header_next", value_string(next));
    }
    while ((s = line(manifest))) {
        char *field[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, field, 9); if (n != 9) die("pp autoinc manifest column count");
        if (!strcmp(field[0], ".rows") && !strcmp(field[1], "assembly") &&
            !strcmp(field[2], "header")) {
            Value *bindings = value_new(JOBJ);
            direct_bindings(bindings, field[7], facts);
            install_section(&g, "exec/pp/assembly-byte.tsv", "header", 'b', bindings, NULL);
            install_section(&g, "exec/pp/assembly-result.tsv", "header", 'r', bindings, NULL);
            found++;
        } else if (!strcmp(field[0], "..rows") && !strcmp(field[1], "autoinc-name")) {
            Value *names = value_get(value_get(facts, "it"), "names"), *opts = value_json(field[8], "pp name options");
            char path[1024];
            for (size_t j = 0; j < names->n; j++) {
                Value *bindings = value_new(JOBJ), *sequences;
                value_put(facts, "n", names->items[j].value);
                direct_bindings(bindings, field[7], facts);
                sequences = mapseq_construct(opts, facts);
                for (int k = 0; k < 2; k++) {
                    if (snprintf(path, sizeof(path), "exec/pp/autoinc-name-%s.tsv", k ? "result" : "byte") >= (int)sizeof(path))
                        die("pp name path too long");
                    install_plain(&g, path, k ? 'r' : 'b', bindings, sequences);
                }
            }
            found++;
        }
        free(s);
    }
    if (ferror(manifest) || fclose(manifest) || found != 2) die("pp header rows missing");
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output(out, &g); if (fclose(out)) die("output close failed");
}
static void inspect_pp_body(const char *outpath, size_t index) {
    FILE *manifest = fopen("exec/pp/autoinc-manifest.tsv", "rb"), *out;
    Value *facts = load_facts_expr("pp-autoinc-gen+pp-layout"), *bodies = value_get(facts, "bodies");
    Graph g = {0}; char *s; int found = 0;
    if (!manifest || !bodies || bodies->kind != JARR || index >= bodies->n) die("invalid pp body index");
    value_put(facts, "it", bodies->items[index].value);
    {
        Value *it = value_get(facts, "it"), *terminal = value_get(it, "terminal"); char next[64];
        if (terminal->kind != JBOOL) die("invalid pp body terminal");
        if (terminal->number) strcpy(next, "LNDEF");
        else if (snprintf(next, sizeof(next), "LNB%lld", value_get(it, "end")->number) >= (int)sizeof(next))
            die("pp body successor too long");
        value_put(facts, "body_next", value_string(next));
    }
    while ((s = line(manifest))) {
        char *field[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, field, 9); if (n != 9) die("pp autoinc manifest column count");
        if (!strcmp(field[0], ".rows") &&
            (!strcmp(field[1], "ftrim-libc-body") ||
             (!strcmp(field[1], "assembly") && !strcmp(field[2], "predefine")))) {
            Value *bindings = value_new(JOBJ), *sequences = NULL;
            Value *opts = !strcmp(field[8], "-") ? NULL : value_json(field[8], "pp body options");
            char path[1024]; int assembly = !strcmp(field[1], "assembly");
            direct_bindings(bindings, field[7], facts);
            if (opts) sequences = mapseq_construct(opts, facts);
            for (int i = 0; i < 2; i++) {
                if (snprintf(path, sizeof(path), "exec/pp/%s-%s.tsv", field[1], i ? "result" : "byte") >= (int)sizeof(path))
                    die("pp body path too long");
                if (assembly) install_section(&g, path, "predefine", i ? 'r' : 'b', bindings, sequences);
                else install_plain(&g, path, i ? 'r' : 'b', bindings, sequences);
            }
            found++;
        }
        free(s);
    }
    if (ferror(manifest) || fclose(manifest) || found != 2) die("pp body rows missing");
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output(out, &g); if (fclose(out)) die("output close failed");
}
static void inspect_pp_key(const char *outpath, size_t index) {
    FILE *manifest = fopen("exec/pp/autoinc-manifest.tsv", "rb"), *out;
    Value *facts = load_facts_expr("pp-autoinc-gen+pp-layout"), *keys = value_get(facts, "keys");
    Graph g = {0}; char *s; int found = 0;
    if (!manifest || !keys || keys->kind != JARR || index >= keys->n) die("invalid pp key index");
    value_put(facts, "it", keys->items[index].value);
    while ((s = line(manifest))) {
        char *field[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, field, 9); if (n != 9) die("pp autoinc manifest column count");
        if (!strcmp(field[0], ".rows") && !strcmp(field[1], "ftrim-libc-key")) {
            Value *it = value_get(facts, "it"), *terminal = value_get(it, "terminal");
            Value *bindings = value_new(JOBJ), *opts = value_json(field[8], "pp key options");
            Value *sequences; char path[1024], next[64];
            if (terminal->kind != JBOOL) die("invalid pp key terminal");
            if (terminal->number) strcpy(next, "LNB0");
            else if (snprintf(next, sizeof(next), "LNK%lld", value_get(it, "end")->number) >= (int)sizeof(next))
                die("pp key successor too long");
            value_put(facts, "key_next", value_string(next));
            direct_bindings(bindings, field[7], facts);
            sequences = mapseq_construct(opts, facts);
            for (int i = 0; i < 2; i++) {
                if (snprintf(path, sizeof(path), "exec/pp/ftrim-libc-key-%s.tsv", i ? "result" : "byte") >= (int)sizeof(path))
                    die("pp key path too long");
                install_plain(&g, path, i ? 'r' : 'b', bindings, sequences);
            }
            found++;
        }
        free(s);
    }
    if (ferror(manifest) || fclose(manifest) || found != 1) die("pp key row missing");
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output(out, &g); if (fclose(out)) die("output close failed");
}
static void inspect_pp_autoinc(const char *stem, const char *outpath) {
    FILE *manifest = fopen("exec/pp/autoinc-manifest.tsv", "rb"), *out;
    Graph g = {0}; char *s; int found = 0;
    if (!manifest) die("cannot open pp autoinc manifest");
    while ((s = line(manifest))) {
        char *field[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, field, 9); if (n != 9) die("pp autoinc manifest column count");
        if (!strcmp(field[0], "rows") && !strcmp(field[1], stem)) {
            Value *facts = load_facts_expr(field[4]), *bindings = value_new(JOBJ);
            Value *opts = value_json(field[8], "pp autoinc options"), *sequences = mapseq_construct(opts, facts);
            Value *classes = NULL, *classmap = value_get(opts, "classmap");
            char path[1024];
            direct_bindings(bindings, field[7], facts);
            if (classmap) {
                classes = value_new(JOBJ);
                for (size_t j = 0; j < classmap->n; j++)
                    value_put(classes, classmap->items[j].key,
                              value_path(facts, value_text(classmap->items[j].value)));
            }
            for (int i = 0; i < 2; i++) {
                if (snprintf(path, sizeof(path), "exec/pp/%s-%s.tsv", stem, i ? "result" : "byte") >= (int)sizeof(path))
                    die("pp autoinc path too long");
                install_plain_classes(&g, path, i ? 'r' : 'b', bindings, sequences, classes);
            }
            found++;
        }
        free(s);
    }
    if (ferror(manifest) || fclose(manifest) || found != 1) die("pp autoinc row missing");
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output(out, &g); if (fclose(out)) die("output close failed");
}
static void inspect_pp_locations(const char *outpath) {
    FILE *manifest = fopen("exec/pp/locations-manifest.tsv", "rb"), *out;
    Graph g = {0}; char *s; int found = 0;
    if (!manifest) die("cannot open pp locations manifest");
    while ((s = line(manifest))) {
        char *field[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, field, 9); if (n != 9) die("pp locations manifest column count");
        if (!strcmp(field[0], "rows") && !strcmp(field[1], "location")) {
            Value *facts = load_facts_expr("locations+pp-layout"), *bindings = value_new(JOBJ);
            const char *names[] = {"SPLB", "IRLN", "IRNL", "IRNAME"};
            char path[1024]; size_t j;
            for (j = 0; j < 4; j++) value_put(bindings, names[j], value_get(facts, names[j]));
            for (int i = 0; i < 2; i++) {
                if (snprintf(path, sizeof(path), "exec/pp/location-%s.tsv", i ? "result" : "byte") >= (int)sizeof(path))
                    die("pp location path too long");
                install_plain(&g, path, i ? 'r' : 'b', bindings, NULL);
            }
            found++;
        }
        free(s);
    }
    if (ferror(manifest) || fclose(manifest) || found != 1) die("pp locations row missing");
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output(out, &g); if (fclose(out)) die("output close failed");
}
static void inspect_pp_template(const char *stem, const char *outpath) {
    FILE *manifest = fopen("exec/pp/body-manifest.tsv", "rb"), *out;
    Graph g = {0}; char *s; int found = 0;
    if (!manifest) die("cannot open pp body manifest");
    while ((s = line(manifest))) {
        char *field[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, field, 9); if (n != 9) die("pp manifest column count");
        if (!strcmp(field[0], "template") && !strcmp(field[1], stem)) {
            Value *facts = load_facts_expr(field[4]), *opts = value_json(field[8], "pp template options");
            Value *domain = value_path(facts, value_text(value_get(opts, "domain_keys")));
            const char *mode = value_text(value_get(opts, "mode"));
            char path[1024]; Buffer lines; FILE *rows;
            if (snprintf(path, sizeof(path), "exec/pp/%s-template.tsv", stem) >= (int)sizeof(path))
                die("pp template path too long");
            lines = expand_template_file(path, facts, field[2]); rows = buffer_file(&lines);
            install_delta_text(&g, rows, mode[0], domain, NULL, NULL, 0, 0, NULL, "START");
            if (fclose(rows)) die("pp template close failed");
            found++;
        }
        free(s);
    }
    if (ferror(manifest) || fclose(manifest) || found != 1) die("pp template row missing or repeated");
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output(out, &g); if (fclose(out)) die("output close failed");
}
static void inspect_pp_assembly(const char *section, const char *outpath) {
    FILE *manifest = fopen("exec/pp/body-manifest.tsv", "rb"), *out;
    Graph g = {0}; char *s; Value *mapseq = NULL; int found = 0;
    if (!manifest) die("cannot open pp body manifest");
    while ((s = line(manifest))) {
        char *field[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, field, 9); if (n != 9) die("pp manifest column count");
        if (!strcmp(field[0], "let") && !strcmp(field[4], "pp-gen+pp-layout")) {
            mapseq = mapseq_construct(value_json(field[8], "pp output options"),
                                      load_facts_expr(field[4]));
        } else if (!strcmp(field[0], "rows") && !strcmp(field[1], "assembly") &&
                   !strcmp(field[2], section)) {
            Value *facts = load_facts_expr(field[4]), *bindings = value_new(JOBJ);
            Value *sequences = value_new(JOBJ);
            if (strcmp(field[8], "-") || !strcmp(field[3], "locations")) die("unsupported pp assembly row");
            direct_bindings(bindings, field[7], facts);
            if (mapseq && value_get(mapseq, "objname"))
                value_put(sequences, "name", value_get(mapseq, "objname"));
            install_section(&g, "exec/pp/assembly-byte.tsv", section, 'b', bindings, sequences);
            install_section(&g, "exec/pp/assembly-result.tsv", section, 'r', bindings, sequences);
            found++;
        }
        free(s);
    }
    if (ferror(manifest) || fclose(manifest) || found != 1) die("pp assembly row missing or repeated");
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output(out, &g); if (fclose(out)) die("output close failed");
}
static void inspect_pp_rows(const char *stem, const char *outpath, int no_autoinc) {
    FILE *manifest = fopen("exec/pp/body-manifest.tsv", "rb"), *out;
    Graph g = {0}; char *s; int found = 0;
    if (!manifest) die("cannot open pp body manifest");
    while ((s = line(manifest))) {
        char *field[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, field, 9); if (n != 9) die("pp manifest column count");
        if (!strcmp(field[0], "rows") && !strcmp(field[1], stem) &&
            when_true(field[3], no_autoinc, no_autoinc ? (char *[]){"--no-autoinc"} : NULL)) {
            Value *facts = load_facts_expr(field[4]), *bindings = value_new(JOBJ);
            if (strcmp(field[2], "-") || strcmp(field[6], "-") || strcmp(field[8], "-"))
                die("pp row has unsupported section, sequence, or options");
            direct_bindings(bindings, field[7], facts);
            pp_install_stem(&g, stem, NULL, bindings, NULL);
            found++;
        }
        free(s);
    }
    if (ferror(manifest) || fclose(manifest) || found != 1) die("pp row missing or repeated");
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output(out, &g); if (fclose(out)) die("output close failed");
}
static void inspect_pp_cli(const char *outpath, int no_autoinc) {
    FILE *manifest = fopen("exec/pp/body-manifest.tsv", "rb"), *out;
    Graph g = {0}; Value *sequences = NULL; char *s; int found = 0;
    if (!manifest) die("cannot open pp body manifest");
    while ((s = line(manifest))) {
        char *field[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, field, 9); if (n != 9) die("pp manifest column count");
        if (!strcmp(field[0], "let") && !strcmp(field[4], "pp-gen+pp-layout")) {
            Value *facts = load_facts_expr(field[4]), *opts = value_json(field[8], "pp output options");
            sequences = mapseq_construct(opts, facts);
        } else if (!strcmp(field[0], "rows") && !strcmp(field[1], "cli") &&
                   !strcmp(field[3], no_autoinc ? "no-autoinc" : "!no-autoinc")) {
            Value *facts = load_facts_expr(field[4]), *bindings = value_new(JOBJ);
            char path[1024];
            direct_bindings(bindings, field[7], facts);
            for (int i = 0; i < 2; i++) {
                if (snprintf(path, sizeof(path), "exec/pp/cli-%s.tsv", i ? "result" : "byte") >= (int)sizeof(path))
                    die("pp CLI path too long");
                install_plain(&g, path, i ? 'r' : 'b', bindings, sequences);
            }
            found++;
        }
        free(s);
    }
    if (ferror(manifest) || fclose(manifest) || found != 1) die("pp CLI row missing");
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output(out, &g); if (fclose(out)) die("output close failed");
}
static void construct_opt(Graph *g, int o2) {
    FILE *f = fopen("exec/opt/gen-manifest.tsv", "rb"); char *s;
    Value *seqenv = value_new(JOBJ), *rounds = value_new(JOBJ), *empty = value_new(JOBJ);
    int prn = 0, numout = 0, scans = 0, maps = 0, setup = 0, answer = 0;
    int roundlet = 0, common = 0, level = 0, extra = 0;
    char *flags[] = {"--o2"};
    fresh_count = 0;
    if (!f) die("cannot open opt manifest");
    while ((s = line(f))) {
        char *field[9], path[1024]; int n;
        Value *opts;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, field, 9); if (n != 9) die("opt manifest column count");
        if (!when_true(field[3], o2, flags)) { free(s); continue; }
        opts = !strcmp(field[8], "-") ? value_new(JOBJ) : value_json(field[8], "opt manifest options");
        if (!strcmp(field[0], "call") && !strcmp(field[1], "../parse/prn")) {
            if (prn++) die("duplicate opt prn call");
            install_prn_call(g);
        } else if (!strcmp(field[0], "call") && !strcmp(field[1], "../parse/numout")) {
            if (numout++) die("duplicate opt numout call");
            install_numout_call(g);
        } else if (!strcmp(field[0], "rows") && !strcmp(field[1], "scans")) {
            Value *facts = load_facts_expr(field[4]), *bindings;
            if (scans++ || strcmp(field[2], "-")) die("unsupported opt scans row");
            bindings = fresh_bindings(opts, facts); direct_bindings(bindings, field[7], facts);
            install_plain(g, "exec/opt/scans-byte.tsv", 'b', bindings, empty);
            install_plain(g, "exec/opt/scans-result.tsv", 'r', bindings, empty);
        } else if (!strcmp(field[0], "rows") && o2 &&
                   (!strcmp(field[1], "analysis") || !strcmp(field[1], "local") ||
                    !strcmp(field[1], "parsers") || !strcmp(field[1], "stfuse") ||
                    !strcmp(field[1], "peep"))) {
            Value *facts = load_facts_expr(field[4]); Value *bindings = NULL;
            install_opt_rows(g, field[1], field[2], opts, facts, field[7], empty, &bindings);
            if (!strcmp(field[1], "peep")) value_put(empty, "PP_b85", value_get(bindings, "PP_b85"));
            extra++;
        } else if (!strcmp(field[0], "let") && value_get(opts, "mapseq")) {
            Value *facts = load_facts_expr(field[4]); size_t i;
            if (maps++) die("duplicate opt mapseq");
            seqenv = mapseq_construct(opts, facts);
            for (i = 0; i < seqenv->n; i++) if (!seqenv->items[i].value) die("empty mapped sequence");
        } else if (!strcmp(field[0], "template") && !strcmp(field[1], "setup") && o2) {
            Value *facts = load_facts_expr(field[4]); Value *bindings = value_new(JOBJ);
            if (answer++ || strcmp(field[2], "answer") || strcmp(field[7], "dispatch=$PP_b85"))
                die("unsupported opt answer declaration");
            value_put(bindings, "dispatch", value_get(empty, "PP_b85"));
            install_opt_answer(g, facts, bindings);
        } else if (!strcmp(field[0], "rows") && !strcmp(field[1], "setup")) {
            Value *seq = value_new(JOBJ), *start = value_get(seqenv, o2 ? "start2" : "start1");
            if (setup++ || strcmp(field[2], "start") || !start ||
                strcmp(field[6], o2 ? "data=$start2" : "data=$start1"))
                die("unsupported opt setup row");
            value_put(seq, "data", start);
            install_section(g, "exec/opt/setup-byte.tsv", "start", 'b', empty, seq);
            install_section(g, "exec/opt/setup-result.tsv", "start", 'r', empty, seq);
        } else if (!strcmp(field[0], "let") && value_get(opts, "freshrows")) {
            Value *facts = load_facts_expr(field[4]);
            if (roundlet++) die("duplicate opt rounds binding");
            rounds = fresh_bindings(opts, facts); direct_bindings(rounds, field[7], facts);
        } else if (!strcmp(field[0], "rows") && !strcmp(field[1], "rounds")) {
            const char *section = field[2];
            if (!strcmp(section, "common")) common++;
            else if (!strcmp(section, o2 ? "2" : "1")) level++;
            else die("unsupported opt round section");
            if (snprintf(path, sizeof(path), "exec/opt/rounds-byte.tsv") >= (int)sizeof(path)) die("rule path too long");
            install_section(g, path, section, 'b', rounds, empty);
            install_section(g, "exec/opt/rounds-result.tsv", section, 'r', rounds, empty);
        } else die("unsupported enabled opt DSL op");
        free(s);
    }
    if (ferror(f) || fclose(f)) die("opt manifest read failed");
    if (prn != 1 || numout != o2 || scans != 1 || maps != 1 || setup != 1 ||
        answer != o2 || extra != (o2 ? 5 : 0) || roundlet != 1 || common != 1 || level != 1)
        die("incomplete opt manifest");
    finish(g);
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
/* First nested nativeabi call: modelsignature -> modelinput.  This command
   exposes a small graph slice while the full nativeabi manifest interpreter
   is being assembled; it reads the same call row and result table as Python. */
static void nativeabi_modelinput_call(Graph *g) {
    FILE *f = fopen("exec/modelsignature-manifest.tsv", "rb");
    char *s; int found = 0;
    if (!f) die("cannot open modelsignature manifest");
    while ((s = line(f))) {
        char *field[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, field, 9);
        if (n != 9) die("modelsignature manifest column count");
        if (!strcmp(field[0], "call") && !strcmp(field[1], "modelinput")) {
            Value *facts = load_facts_expr(field[4]);
            Value *bindings = value_new(JOBJ), *sequences = value_new(JOBJ);
            value_put(facts, "fail", value_string("DEAD"));
            direct_bindings_ex(bindings, sequences, field[7], facts);
            {
                FILE *sub = fopen("exec/modelinput-manifest.tsv", "rb");
                char *row; int subfound = 0;
                if (!sub) die("cannot open modelinput manifest");
                for (size_t i = 0; i < bindings->n; i++)
                    value_put(facts, bindings->items[i].key, bindings->items[i].value);
                while ((row = line(sub))) {
                    char *col[9]; int cols;
                    if (!*row || *row == '#') { free(row); continue; }
                    cols = fields_tab(row, col, 9);
                    if (cols != 9 || strcmp(col[0], "rows") || strcmp(col[1], "modelinput") || strcmp(col[2], "u64"))
                        die("unsupported modelinput row");
                    direct_bindings_ex(bindings, sequences, col[7], facts);
                    subfound++;
                    free(row);
                }
                if (ferror(sub) || fclose(sub) || subfound != 1) die("modelinput row not found");
            }
            install_section(g, "exec/modelinput-result.tsv", "u64", 'r', bindings, sequences);
            found++;
        }
        free(s);
    }
    if (ferror(f) || fclose(f) || found != 1) die("modelinput call not found");
}
static void inspect_nativeabi_modelinput(const char *outpath) {
    Graph g = {0}; FILE *out;
    nativeabi_modelinput_call(&g);
    finish(&g);
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output_graph(out, &g, "START", NULL);
    if (fclose(out)) die("output close failed");
}
static Value *nativeabi_rows(Graph *g, const char *stem, const char *section,
                             const char *factexpr, const char *bind, const char *opttext,
                             Value *env, Value *prior) {
    char path[1024]; Value *facts = load_facts_expr(factexpr);
    Value *opts = !strcmp(opttext, "-") ? value_new(JOBJ) : value_json(opttext, "nativeabi row options");
    Value *bindings, *seqs = value_new(JOBJ), *bindmap = value_get(opts, "bindmap");
    Value *classes = NULL, *classmap = value_get(opts, "classmap"), *seqenv = value_get(opts, "seqenv");
    for (size_t i = 0; i < env->n; i++) value_put(facts, env->items[i].key, env->items[i].value);
    bindings = value_new(JOBJ);
    if (prior) for (size_t i = 0; i < prior->n; i++)
        value_put(bindings, prior->items[i].key, prior->items[i].value);
    {
        Value *fresh = value_get(opts, "freshrows") && value_get(opts, "freshrows")->kind == JSTR
            ? fresh_bindings_file(opts, "exec") : fresh_bindings(opts, facts);
        for (size_t i = 0; i < fresh->n; i++)
            value_put(bindings, fresh->items[i].key, fresh->items[i].value);
    }
    if (bindmap) {
        Value *map = value_path(facts, value_text(bindmap));
        if (map->kind != JOBJ) die("nativeabi bindmap is not an object");
        for (size_t i = 0; i < map->n; i++) value_put(bindings, map->items[i].key, map->items[i].value);
    }
    direct_bindings_ex(bindings, seqs, bind, facts);
    if (seqenv) {
        Value *names = value_path(facts, value_text(seqenv));
        if (names->kind != JARR) die("nativeabi seqenv is not a list");
        for (size_t i = 0; i < names->n; i++) {
            const char *name = value_text(names->items[i].value);
            Value *sequence = value_get(env, name);
            if (!sequence) die("nativeabi sequence absent from env");
            value_put(seqs, name, sequence);
        }
    }
    if (classmap) {
        if (classmap->kind != JOBJ) die("nativeabi classmap is not an object");
        classes = value_new(JOBJ);
        for (size_t i = 0; i < classmap->n; i++) {
            Value *v = value_path(facts, value_text(classmap->items[i].value));
            if (v->kind != JARR) { Value *one = value_new(JARR); value_put(one, NULL, v); v = one; }
            value_put(classes, classmap->items[i].key, v);
        }
    }
    if (snprintf(path, sizeof(path), "exec/%s-byte.tsv", stem) >= (int)sizeof(path)) die("nativeabi rule path too long");
    install_section_classes(g, path, section, 'b', bindings, seqs, classes);
    if (snprintf(path, sizeof(path), "exec/%s-result.tsv", stem) >= (int)sizeof(path)) die("nativeabi rule path too long");
    install_section_classes(g, path, section, 'r', bindings, seqs, classes);
    return bindings;
}
static void nativeabi_calls(Graph *g) {
    Value *env = value_new(JOBJ); FILE *f; char *s;
    value_put(env, "fail", value_string("DEAD"));
    f = fopen("exec/modelsignature-manifest.tsv", "rb"); if (!f) die("cannot open modelsignature manifest");
    while ((s = line(f))) {
        char *col[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, col, 9); if (n != 9) die("modelsignature row count");
        if (!strcmp(col[0], "rows")) nativeabi_rows(g, col[1], col[2], col[4], col[7], col[8], env, NULL);
        else if (!strcmp(col[0], "call") && !strcmp(col[1], "modelinput")) nativeabi_modelinput_call(g);
        else if (strcmp(col[0], "let")) die("unsupported modelsignature declaration");
        free(s);
    }
    if (ferror(f) || fclose(f)) die("modelsignature read failed");
    f = fopen("exec/modelgraphequality-manifest.tsv", "rb"); if (!f) die("cannot open modelgraphequality manifest");
    while ((s = line(f))) {
        char *col[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, col, 9); if (n != 9) die("modelgraphequality row count");
        if (!strcmp(col[0], "rows")) nativeabi_rows(g, col[1], col[2], col[4], col[7], col[8], env, NULL);
        else if (strcmp(col[0], "let") && strcmp(col[0], "call")) die("unsupported modelgraphequality declaration");
        free(s);
    }
    if (ferror(f) || fclose(f)) die("modelgraphequality read failed");
}
static void inspect_nativeabi_calls(const char *outpath) {
    Graph g = {0}; FILE *out;
    nativeabi_calls(&g); finish(&g);
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output_graph(out, &g, "START", NULL);
    if (fclose(out)) die("output close failed");
}
static void nativeabi_template_at(Graph *g, const char *path, const char *section,
                                  const char *fresh_owner) {
    Value *facts = load_fact("nativeabi"), *seqs = value_new(JOBJ);
    Buffer expanded = expand_template_file_fresh(path, facts,
                                                 section, fresh_owner, NULL);
    FILE *f = tmpfile();
    if (!f) die("cannot create template stream");
    if (expanded.n && fwrite(expanded.s, 1, expanded.n, f) != expanded.n) die("template write failed");
    if (fflush(f) || fseek(f, 0, SEEK_SET)) die("template rewind failed");
    install_delta_text(g, f, 'r', numeric_domain(0, 257), NULL, seqs, 0, 0, NULL, "START");
    if (fclose(f)) die("template close failed");
    free(expanded.s);
}
static void inspect_nativeabi_head(const char *outpath, int phase) {
    Graph g = {0}; FILE *manifest, *out; char *linebuf;
    Value *env = value_new(JOBJ), *nc = NULL, *ol = NULL;
    int seen = 0, limit = phase + 4;
    manifest = fopen("exec/nativeabi/gen-manifest.tsv", "rb");
    if (!manifest) die("cannot open nativeabi manifest");
    while ((linebuf = line(manifest))) {
        char *col[9], path[1024]; int n;
        if (!*linebuf || *linebuf == '#') { free(linebuf); continue; }
        if (seen == limit) { free(linebuf); break; }
        n = fields_tab(linebuf, col, 9);
        if (n != 9) die("nativeabi manifest column count");
        seen++;
        if (!strcmp(col[0], "let")) {
            Value *opts = value_json(col[8], "nativeabi let options");
            Value *facts = load_facts_expr(col[4]), *seqs = mapseq_construct(opts, facts);
            for (size_t i = 0; i < seqs->n; i++)
                value_put(env, seqs->items[i].key, seqs->items[i].value);
        } else if (!strcmp(col[0], "call")) {
            if (strcmp(col[1], "../modelgraphequality"))
                die("unsupported nativeabi call");
            nativeabi_calls(&g);
        } else if (!strcmp(col[0], "rows")) {
            Value *opts = value_json(col[8], "nativeabi row options");
            Value *acc = value_get(opts, "accumulate"), **prior;
            if (!acc || acc->kind != JSTR) die("nativeabi row lacks accumulator");
            if (!strcmp(acc->s, "nc")) prior = &nc;
            else if (!strcmp(acc->s, "ol")) prior = &ol;
            else die("unknown nativeabi accumulator");
            if (snprintf(path, sizeof(path), "nativeabi/%s", col[1]) >= (int)sizeof(path))
                die("nativeabi row path too long");
            *prior = nativeabi_rows(&g, path, col[2], col[4], col[7], col[8], env, *prior);
        } else if (!strcmp(col[0], "template")) {
            if (strncmp(col[5], "P:", 2)) die("unsupported nativeabi fresh scope");
            if (snprintf(path, sizeof(path), "exec/nativeabi/%s-template.tsv", col[1]) >= (int)sizeof(path))
                die("nativeabi template path too long");
            nativeabi_template_at(&g, path, col[2], col[5] + 2);
        } else die("unsupported nativeabi manifest operation");
        free(linebuf);
    }
    if (ferror(manifest) || fclose(manifest) || seen != limit)
        die("nativeabi manifest incomplete");
    finish(&g);
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output_graph(out, &g, phase >= 8 ? "NC.START" : "START", NULL);
    if (fclose(out)) die("output close failed");
}
static void output_graph(FILE *f, const Graph *g, const char *start, const Value *tok_names) {
    Buffer b = {0}; char number[32];
    buf_add(&b, "{\"start\":", 9); buf_quote(&b, start); buf_add(&b, ",\"states\":{", 11);
    for (size_t i = 0; i < g->n; i++) {
        const State *s = &g->state[i];
        if (i) buf_char(&b, ','); buf_quote(&b, s->name);
        buf_add(&b, ":[\"", 3); buf_char(&b, s->mode); buf_add(&b, "\",{", 3);
        for (size_t j = 0; j < s->n; j++) {
            if (j) buf_char(&b, ','); buf_quote(&b, s->edge[j].key); buf_add(&b, ":[", 2);
            buf_quote(&b, s->edge[j].target);
            snprintf(number, sizeof(number), ",%d]", s->edge[j].seq);
            buf_add(&b, number, strlen(number));
        }
        buf_add(&b, "}]", 2);
    }
    buf_add(&b, "},\"seqs\":[", 10);
    for (size_t i = 0; i < g->ns; i++) { if (i) buf_char(&b, ','); buf_add(&b, g->seq[i], strlen(g->seq[i])); }
    buf_char(&b, ']');
    if (tok_names) { buf_add(&b, ",\"tok_names\":", 13); buf_value(&b, tok_names); }
    buf_char(&b, '}');
    if (fwrite(b.s, 1, b.n, f) != b.n || ferror(f)) die("write failed");
    free(b.s);
}
static void output(FILE *f, const Graph *g) { output_graph(f, g, "START", NULL); }
/* parse2base's token prelude is a declared vocabulary plus two E3 additions.
   Keep this slice separate from the graph until the parse2 manifest walker is
   able to install its tokenizer and procedures in the same order as Python. */
static void inspect_parse2_tokens(const char *outpath) {
    Value *facts = load_fact("parse-tokens"), *words = value_get(facts, "WORDS");
    Value *tk = value_get(facts, "TK"), *result = value_new(JOBJ);
    const char *extra[] = {"type=extern", "type=_Bool"};
    long long maximum = 0; size_t i; FILE *out;
    if (!words || words->kind != JARR || !tk || tk->kind != JOBJ)
        die("invalid parse token facts");
    for (i = 0; i < tk->n; i++) {
        Value *v = tk->items[i].value;
        if (!v || v->kind != JINT || v->number < 0) die("invalid parse token number");
        if (v->number > maximum) maximum = v->number;
    }
    for (i = 0; i < sizeof(extra) / sizeof(extra[0]); i++) {
        Value *id = value_new(JINT);
        if (value_get(tk, extra[i])) die("duplicate parse2 token");
        if (maximum == LLONG_MAX) die("parse token number overflow");
        id->number = ++maximum;
        value_put(words, NULL, value_string(extra[i]));
        value_put(tk, extra[i], id);
    }
    value_put(result, "WORDS", words); value_put(result, "TK", tk);
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    value_write(out, result); if (fclose(out)) die("output close failed");
}
static void build_parse2_token_graph(Graph *g) {
    FILE *manifest = fopen("exec/parse/tokens2-manifest.tsv", "rb");
    Value *facts = load_fact("k2-gen2-tokens"), *modes = value_new(JOBJ);
    Value *sequences = value_new(JOBJ), *bindings; char *s; int template_seen = 0, rows_seen = 0;
    if (!manifest) die("cannot open parse2 token manifest");
    while ((s = line(manifest))) {
        char *field[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, field, 9);
        if (n != 9) die("parse2 token manifest column count");
        if (!strcmp(field[0], "template") && !strcmp(field[1], "tokenfacts")) {
            Buffer expanded; FILE *rules;
            if (template_seen++ || rows_seen || strcmp(field[2], "facts") ||
                strcmp(field[4], "k2-gen2-tokens")) die("unexpected parse2 token template");
            expanded = expand_template_file_fresh("exec/parse/tokenfacts-template.tsv",
                                                  facts, "facts", NULL, modes);
            rules = buffer_file(&expanded);
            install_delta_text(g, rules, 'b', numeric_domain(0, 257), NULL,
                               sequences, 0, 0, NULL, "START");
            if (fclose(rules)) die("parse2 token template close failed");
            for (size_t i = 0; i < modes->n; i++) {
                const char *name = modes->items[i].key; size_t j;
                if (strcmp(value_text(modes->items[i].value), "r"))
                    die("unsupported parse2 token mode");
                for (j = 0; j < g->n; j++) if (!strcmp(g->state[j].name, name)) {
                    g->state[j].mode = 'r'; break;
                }
                if (j == g->n) die("parse2 token mode state missing");
            }
            if (!modes->n) die("parse2 token mode overrides missing");
            free(expanded.s);
        } else if (!strcmp(field[0], "rows") && !strcmp(field[1], "tokenread")) {
            Value *rowfacts;
            if (!template_seen || rows_seen++ || strcmp(field[4], "k2-gen2-tokens+parse-constants") ||
                strcmp(field[6], "truncated=@rej:not covered: truncated token dump"))
                die("unexpected parse2 token rows");
            rowfacts = load_facts_expr(field[4]); bindings = value_new(JOBJ);
            value_put(sequences, "truncated", value_json("[[\"REJECT\",\"not covered: truncated token dump\"]]", "token truncation"));
            direct_bindings(bindings, field[7], rowfacts);
            install_plain(g, "exec/parse/tokenread-byte.tsv", 'b', bindings, sequences);
            install_plain(g, "exec/parse/tokenread-result.tsv", 'r', bindings, sequences);
        } else die("unsupported parse2 token manifest row");
        free(s);
    }
    if (ferror(manifest) || fclose(manifest) || template_seen != 1 || rows_seen != 1)
        die("incomplete parse2 token manifest");
}
static void inspect_parse2_token_graph(const char *outpath) {
    Graph g = {0}; FILE *out;
    build_parse2_token_graph(&g);
    finish(&g);
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output_graph(out, &g, "START", NULL); if (fclose(out)) die("output close failed");
}
/* The first nested parse2 body call installs gen2's startup marker.  Execute
   the stage-edits declaration on the token graph, in manifest order.  These
   are graph operations, not C syntax decisions. */
static void parse2_startup_edits(Graph *g) {
    FILE *manifest = fopen("exec/parse2/gen2parts-manifest.tsv", "rb");
    FILE *templ; char *s; int declared = 0, copied = 0, dropped = 0, rule = 0;
    if (!manifest) die("cannot open gen2parts manifest");
    while ((s = line(manifest))) {
        char *f[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, f, 9);
        if (n != 9) die("gen2parts manifest column count");
        if (!strcmp(f[0], "template") && !strcmp(f[1], "stage-edits") &&
            (!strcmp(f[2], "startup") || !strcmp(f[2], "startup-entry"))) declared++;
        free(s);
    }
    if (ferror(manifest) || fclose(manifest) || declared != 2)
        die("parse2 startup edits not declared");
    templ = fopen("exec/parse2/stage-edits-template.tsv", "rb");
    if (!templ) die("cannot open parse2 stage edits");
    while ((s = line(templ))) {
        char *f[9]; int n; State *st;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, f, 9);
        if (n != 9) die("parse2 stage edit column count");
        if (!strcmp(f[0], "startup") && !strcmp(f[4], "copy-state")) {
            if (copied++ || strcmp(f[3], "-") || strcmp(f[7], "-") || strcmp(f[8], "-"))
                die("invalid parse2 startup copy declaration");
            pp_copy_state(g, f[5], f[6]);
        } else if (!strcmp(f[0], "startup") && !strcmp(f[4], "drop-edge")) {
            size_t i;
            if (!copied || dropped++ || strcmp(f[3], "-")) die("invalid parse2 startup drop declaration");
            st = find_state(g, f[5]);
            for (i = 0; i < st->n && strcmp(st->edge[i].key, f[6]); i++) {}
            if (i == st->n) die("parse2 startup edge to drop missing");
            memmove(st->edge + i, st->edge + i + 1, (st->n - i - 1) * sizeof(*st->edge));
            st->n--;
            if (st->key_index) {
                memset(st->key_index, 0, 257 * sizeof(*st->key_index));
                for (i = 0; i < st->n; i++) {
                    int key = numeric_key(st->edge[i].key);
                    if (key >= 0) st->key_index[key] = (int)i + 1;
                }
            }
        } else if (!strcmp(f[0], "startup-entry") && !strcmp(f[4], "rule")) {
            if (!dropped || rule++ || strcmp(f[3], "-")) die("invalid parse2 startup rule declaration");
            st = find_state(g, f[5]);
            if (edge_has(st, f[6])) die("parse2 startup rule edge already exists");
            edge_add(g, f[5], st->mode, f[6], f[7], f[8]);
        }
        free(s);
    }
    if (ferror(templ) || fclose(templ) || copied != 1 || dropped != 1 || rule != 1)
        die("incomplete parse2 startup edits");
}
static void parse2_drop_state(Graph *g, const char *name) {
    size_t i;
    for (i = 0; i < g->n && strcmp(g->state[i].name, name); i++) {}
    if (i == g->n) die("parse2 template state to drop missing");
    memmove(g->state + i, g->state + i + 1, (g->n - i - 1) * sizeof(*g->state));
    g->n--;
    free(g->state_index); g->state_index = NULL; g->index_cap = 0;
    state_index_grow(g);
}
static void parse2_string_span(Graph *g) {
    FILE *manifest = fopen("exec/parse2/strings-token-span-manifest.tsv", "rb");
    FILE *templ; Value *facts = load_fact("k2-strings-token-span");
    Value *sequences = value_new(JOBJ), *bindings = value_new(JOBJ);
    Value *constants = load_fact("parse-constants");
    char *s; int declared = 0, dropped = 0;
    if (!manifest) die("cannot open parse2 string span manifest");
    while ((s = line(manifest))) {
        char *f[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, f, 9);
        if (n != 9) die("parse2 string span manifest column count");
        if (declared == 0 && !strcmp(f[0], "let") && !strcmp(f[4], "k2-strings-token-span")) declared++;
        else if (declared == 1 && !strcmp(f[0], "template") &&
                 !strcmp(f[1], "strings") && !strcmp(f[2], "token_span_drop")) declared++;
        else if (declared == 2 && !strcmp(f[0], "rows") &&
                 !strcmp(f[1], "strings") && !strcmp(f[2], "token_span")) declared++;
        else die("unexpected parse2 string span manifest row");
        free(s);
    }
    if (ferror(manifest) || fclose(manifest) || declared != 3)
        die("incomplete parse2 string span manifest");
    templ = fopen("exec/parse2/strings-template.tsv", "rb");
    if (!templ) die("cannot open parse2 strings template");
    while ((s = line(templ))) {
        char *f[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, f, 9);
        if (n != 9) die("parse2 strings template column count");
        if (!strcmp(f[0], "token_span_drop")) {
            if (dropped++ || strcmp(f[4], "drop-state")) die("invalid string span drop");
            parse2_drop_state(g, f[5]);
        }
        free(s);
    }
    if (ferror(templ) || fclose(templ) || dropped != 1)
        die("incomplete parse2 strings template");
    for (int i = 0; i < 3; i++) {
        char name[16]; Value *acts = value_new(JARR), *reject = value_new(JARR);
        Value *reason; snprintf(name, sizeof(name), "reject%d", i);
        reason = value_get(facts, name);
        if (!reason || reason->kind != JSTR) die("missing string span rejection fact");
        value_put(reject, NULL, value_string("REJECT"));
        value_put(reject, NULL, reason);
        value_put(acts, NULL, reject);
        value_put(sequences, name, acts);
    }
    value_put(bindings, "TK_STR", value_get(constants, "TK_STR"));
    install_section(g, "exec/parse2/strings-byte.tsv", "token_span", 'b', bindings, sequences);
    install_section(g, "exec/parse2/strings-result.tsv", "token_span", 'r', bindings, sequences);
}
static void parse2_startup_control(Graph *g) {
    FILE *manifest = fopen("exec/parse2/control-manifest.tsv", "rb");
    Value *facts = load_fact("k2-control"), *opts = NULL;
    Value *bindings = value_new(JOBJ), *sequences, *classes;
    char *s; int declarations = 0;
    if (!manifest) die("cannot open parse2 control manifest");
    while ((s = line(manifest))) {
        char *f[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, f, 9);
        if (n != 9) die("parse2 control manifest column count");
        if (!declarations && !strcmp(f[0], "rows") && !strcmp(f[1], "control") &&
            !strcmp(f[3], "!fact:control_export") && !strcmp(f[4], "k2-control"))
            opts = value_json(f[8], "parse2 control options");
        declarations++; free(s);
    }
    if (ferror(manifest) || fclose(manifest) || declarations != 2 || !opts)
        die("unsupported parse2 control manifest");
    /* The startup-marker section has no fresh labels.  Its named action
       sequences still come from the same mapseq declaration as every other
       control section; only the rows below consume them. */
    sequences = mapseq_construct(opts, facts);
    classes = value_get(facts, "classes");
    if (!classes || classes->kind != JOBJ) die("parse2 control classes missing");
    install_section_classes(g, "exec/parse2/control-byte.tsv", "startup-marker", 'b',
                            bindings, sequences, classes);
    install_section_classes(g, "exec/parse2/control-result.tsv", "startup-marker", 'r',
                            bindings, sequences, classes);
}
static void parse2_string_initializer_head(Graph *g) {
    FILE *manifest = fopen("exec/parse2/strings-initializer-manifest.tsv", "rb");
    Value *facts = load_fact("k2-strings"), *reasons = value_get(facts, "rej");
    Value *sequences = value_new(JOBJ), *bindings = value_new(JOBJ);
    char *s; int row = 0;
    if (!manifest || !reasons || reasons->kind != JOBJ)
        die("cannot read string initializer facts");
    while ((s = line(manifest))) {
        char *f[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, f, 9);
        if (n != 9) die("string initializer manifest column count");
        if (!row && (!strcmp(f[0], "let") && !strcmp(f[4], "k2-strings"))) {
            row++;
        } else if (row == 1 && !strcmp(f[0], "rows") && !strcmp(f[1], "strings") &&
                   !strcmp(f[2], "initializer_head") && !strcmp(f[4], "k2-strings")) {
            for (int i = 0; i < 5; i++) {
                char name[16]; Value *acts = value_new(JARR), *reject = value_new(JARR), *reason;
                snprintf(name, sizeof(name), "reject%d", i);
                reason = value_get(reasons, name);
                if (!reason || reason->kind != JSTR) die("missing string initializer rejection fact");
                value_put(reject, NULL, value_string("REJECT"));
                value_put(reject, NULL, reason);
                value_put(acts, NULL, reject);
                value_put(sequences, name, acts);
            }
            direct_bindings_ex(bindings, sequences, f[7], facts);
            install_section(g, "exec/parse2/strings-result.tsv", "initializer_head", 'r',
                            bindings, sequences);
            install_section(g, "exec/parse2/strings-byte.tsv", "initializer_head", 'b',
                            bindings, sequences);
            row++;
        } else if (row == 2 && !strcmp(f[0], "call") && !strcmp(f[1], "strwalk")) {
            free(s); break;
        } else die("unexpected string initializer head row");
        free(s);
    }
    if (ferror(manifest) || fclose(manifest) || row != 2)
        die("incomplete string initializer head");
}
static void parse2_string_walk_head(Graph *g, const char *pre,
                                    const char *body, const char *done) {
    FILE *manifest = fopen("exec/parse2/strwalk-manifest.tsv", "rb");
    Value *facts = load_fact("k2-strings"), *reasons = value_get(facts, "rej");
    Value *sequences = value_new(JOBJ), *bindings = value_new(JOBJ);
    char *s; int row = 0;
    if (!manifest || !reasons || reasons->kind != JOBJ)
        die("cannot read string walker facts");
    value_put(facts, "pre", value_string(pre));
    value_put(facts, "body", value_string(body));
    value_put(facts, "done", value_string(done));
    while ((s = line(manifest))) {
        char *f[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, f, 9);
        if (n != 9) die("string walker manifest column count");
        if (!row && !strcmp(f[0], "let") && !strcmp(f[4], "k2-strings")) row++;
        else if (row == 1 && !strcmp(f[0], "rows") && !strcmp(f[1], "strings") &&
                 !strcmp(f[2], "walk_head") && !strcmp(f[4], "k2-strings")) {
            for (int i = 0; i < 5; i++) {
                char name[16]; Value *acts = value_new(JARR), *reject = value_new(JARR), *reason;
                snprintf(name, sizeof(name), "reject%d", i);
                reason = value_get(reasons, name);
                if (!reason || reason->kind != JSTR) die("missing string walker rejection fact");
                value_put(reject, NULL, value_string("REJECT"));
                value_put(reject, NULL, reason);
                value_put(acts, NULL, reject);
                value_put(sequences, name, acts);
            }
            direct_bindings_ex(bindings, sequences, f[7], facts);
            install_section(g, "exec/parse2/strings-byte.tsv", "walk_head", 'b',
                            bindings, sequences);
            install_section(g, "exec/parse2/strings-result.tsv", "walk_head", 'r',
                            bindings, sequences);
            row++;
        } else if (row == 2 && !strcmp(f[0], "template") &&
                   !strcmp(f[1], "strings") && !strcmp(f[2], "walk_escape")) {
            free(s); break;
        } else die("unexpected string walker head row");
        free(s);
    }
    if (ferror(manifest) || fclose(manifest) || row != 2)
        die("incomplete string walker head");
}
static void parse2_string_walk_escape(Graph *g, const char *pre,
                                      const char *body) {
    FILE *templ = fopen("exec/parse2/strings-template.tsv", "rb");
    Value *facts = load_fact("k2-strings"), *escapes = value_get(facts, "escape");
    Value *bindings = value_new(JOBJ);
    char *s; int found = 0;
    if (!templ || !escapes || escapes->kind != JARR)
        die("cannot read string escape template facts");
    value_put(facts, "pre", value_string(pre));
    value_put(facts, "body", value_string(body));
    direct_bindings(bindings, "walk_es=@str:{pre}.es,body=$body", facts);
    while ((s = line(templ))) {
        char *f[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, f, 9);
        if (n != 9) die("string escape template column count");
        if (!strcmp(f[0], "walk_escape")) {
            if (found++ || strcmp(f[3], "escape") || strcmp(f[4], "rule"))
                die("unsupported string escape declaration");
            for (size_t i = 0; i < escapes->n; i++) {
                Value *scope = value_new(JOBJ);
                char *a, *b, *c, *d;
                value_put(scope, "escape", escapes->items[i].value);
                a = template_subst(f[5], scope); b = template_subst(f[6], scope);
                c = template_subst(f[7], scope); d = template_subst(f[8], scope);
                edge_add(g, bound_name(a, bindings), 'b', b,
                         bound_name(c, bindings), d);
                free(a); free(b); free(c); free(d);
            }
        }
        free(s);
    }
    if (ferror(templ) || fclose(templ) || found != 1)
        die("incomplete string escape template");
}
static void parse2_string_walk_tail(Graph *g, const char *pre,
                                    const char *body, const char *done) {
    FILE *manifest = fopen("exec/parse2/strwalk-manifest.tsv", "rb");
    Value *facts = load_fact("k2-strings"), *reasons = value_get(facts, "rej");
    Value *sequences = value_new(JOBJ), *bindings = value_new(JOBJ);
    char *s; int row = 0;
    if (!manifest || !reasons || reasons->kind != JOBJ)
        die("cannot read string walker tail facts");
    value_put(facts, "pre", value_string(pre));
    value_put(facts, "body", value_string(body));
    value_put(facts, "done", value_string(done));
    while ((s = line(manifest))) {
        char *f[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, f, 9);
        if (n != 9) die("string walker tail manifest column count");
        if (row == 0 && !strcmp(f[0], "let")) row++;
        else if (row == 1 && !strcmp(f[0], "rows") && !strcmp(f[2], "walk_head")) row++;
        else if (row == 2 && !strcmp(f[0], "template") && !strcmp(f[2], "walk_escape")) row++;
        else if (row == 3 && !strcmp(f[0], "rows") && !strcmp(f[1], "strings") &&
                 !strcmp(f[2], "walk_tail") && !strcmp(f[4], "k2-strings")) {
            for (int i = 0; i < 5; i++) {
                char name[16]; Value *acts = value_new(JARR), *reject = value_new(JARR), *reason;
                snprintf(name, sizeof(name), "reject%d", i);
                reason = value_get(reasons, name);
                if (!reason || reason->kind != JSTR) die("missing string walker tail rejection fact");
                value_put(reject, NULL, value_string("REJECT"));
                value_put(reject, NULL, reason);
                value_put(acts, NULL, reject);
                value_put(sequences, name, acts);
            }
            direct_bindings_ex(bindings, sequences, f[7], facts);
            install_section(g, "exec/parse2/strings-byte.tsv", "walk_tail", 'b',
                            bindings, sequences);
            install_section(g, "exec/parse2/strings-result.tsv", "walk_tail", 'r',
                            bindings, sequences);
            row++;
        } else die("unexpected string walker tail row");
        free(s);
    }
    if (ferror(manifest) || fclose(manifest) || row != 4)
        die("incomplete string walker tail");
}
static void parse2_string_initializer_tail(Graph *g) {
    FILE *manifest = fopen("exec/parse2/strings-initializer-manifest.tsv", "rb");
    Value *facts = load_fact("k2-strings"), *reasons = value_get(facts, "rej");
    Value *sequences = value_new(JOBJ), *bindings = value_new(JOBJ);
    char *s; int row = 0;
    if (!manifest || !reasons || reasons->kind != JOBJ)
        die("cannot read string initializer tail facts");
    while ((s = line(manifest))) {
        char *f[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, f, 9);
        if (n != 9) die("string initializer tail manifest column count");
        if (row == 0 && !strcmp(f[0], "let")) row++;
        else if (row == 1 && !strcmp(f[0], "rows") && !strcmp(f[2], "initializer_head")) row++;
        else if (row == 2 && !strcmp(f[0], "call") && !strcmp(f[1], "strwalk")) row++;
        else if (row == 3 && !strcmp(f[0], "rows") && !strcmp(f[1], "strings") &&
                 !strcmp(f[2], "initializer_tail") && !strcmp(f[4], "k2-strings")) {
            for (int i = 0; i < 5; i++) {
                char name[16]; Value *acts = value_new(JARR), *reject = value_new(JARR), *reason;
                snprintf(name, sizeof(name), "reject%d", i);
                reason = value_get(reasons, name);
                if (!reason || reason->kind != JSTR) die("missing string initializer tail rejection fact");
                value_put(reject, NULL, value_string("REJECT"));
                value_put(reject, NULL, reason);
                value_put(acts, NULL, reject);
                value_put(sequences, name, acts);
            }
            direct_bindings_ex(bindings, sequences, f[7], facts);
            install_section(g, "exec/parse2/strings-byte.tsv", "initializer_tail", 'b',
                            bindings, sequences);
            install_section(g, "exec/parse2/strings-result.tsv", "initializer_tail", 'r',
                            bindings, sequences);
            row++;
        } else die("unexpected string initializer tail row");
        free(s);
    }
    if (ferror(manifest) || fclose(manifest) || row != 4)
        die("incomplete string initializer tail");
}
static void parse2_numeric(Graph *g) {
    FILE *manifest = fopen("exec/parse/numeric-manifest.tsv", "rb");
    char *s; int rows = 0;
    if (!manifest) die("cannot open numeric manifest");
    while ((s = line(manifest))) {
        char *f[9]; int n; Value *facts, *opts, *bindings, *lets;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, f, 9);
        if (n != 9 || strcmp(f[0], "rows") || strcmp(f[1], "numeric") ||
            strcmp(f[4], "parse-constants") || rows >= 3)
            die("unsupported numeric manifest row");
        if (strcmp(f[2], rows < 2 ? "prn" : "numout"))
            die("numeric section order changed");
        facts = load_fact("parse-constants");
        opts = value_json(f[8], "numeric manifest options");
        lets = value_get(opts, "let");
        if (lets && lets->kind == JOBJ)
            for (size_t i = 0; i < lets->n; i++)
                value_put(facts, lets->items[i].key, lets->items[i].value);
        bindings = value_new(JOBJ);
        numeric_file_bindings(opts, facts, bindings);
        direct_bindings(bindings, f[7], facts);
        declared_bindmap(opts, facts, bindings);
        install_section(g, "exec/parse/numeric-byte.tsv", f[2], 'b', bindings, NULL);
        install_section(g, "exec/parse/numeric-result.tsv", f[2], 'r', bindings, NULL);
        rows++; free(s);
    }
    if (ferror(manifest) || fclose(manifest) || rows != 3)
        die("incomplete numeric manifest");
}
static void parse2_float_boundary(Graph *g) {
    FILE *manifest = fopen("exec/parse2/floatconst-manifest.tsv", "rb");
    FILE *templ; char *s; int rows = 0, edits = 0;
    if (!manifest) die("cannot open floatconst manifest");
    while ((s = line(manifest))) {
        char *f[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, f, 9);
        if (n != 9) die("floatconst manifest column count");
        if (rows == 0 && !strcmp(f[0], "let") &&
            !strcmp(f[1], "-") && !strcmp(f[8], "{\"mapseq\":{\"reject0\":[{\"acts\":[[\"REJECT\",\"not covered: decimal floating constant\"]]}],\"reject1\":[{\"acts\":[[\"REJECT\",\"not covered: decimal floating suffix\"]]}]}}")) rows++;
        else if (rows == 1 && !strcmp(f[0], "template") &&
                 !strcmp(f[1], "floatconst") && !strcmp(f[2], "boundary") &&
                 !strcmp(f[4], "k2-floatconst")) rows++;
        else { free(s); break; }
        free(s);
        if (rows == 2) break;
    }
    if (fclose(manifest) || rows != 2) die("floatconst boundary manifest changed");
    templ = fopen("exec/parse2/floatconst-template.tsv", "rb");
    if (!templ) die("cannot open floatconst template");
    while ((s = line(templ))) {
        char *f[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, f, 9);
        if (n != 9) die("floatconst template column count");
        if (strcmp(f[0], "boundary")) { free(s); continue; }
        if (edits == 0 && !strcmp(f[4], "move-state"))
            pp_move_state(g, f[5], f[6]);
        else if ((edits == 1 || edits == 2) && !strcmp(f[4], "drop-state"))
            parse2_drop_state(g, f[5]);
        else die("unsupported floatconst boundary edit");
        edits++; free(s);
    }
    if (ferror(templ) || fclose(templ) || edits != 3)
        die("incomplete floatconst boundary edits");
}
static void parse2_float_entry(Graph *g) {
    FILE *manifest = fopen("exec/parse2/floatconst-manifest.tsv", "rb");
    Value *facts = load_fact("k2-floatconst"), *bindings = value_new(JOBJ);
    Value *sequences = value_new(JOBJ); char *s; int rows = 0;
    if (!manifest) die("cannot open floatconst manifest");
    while ((s = line(manifest))) {
        char *f[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, f, 9);
        if (n != 9) die("floatconst manifest column count");
        if (rows == 0 && !strcmp(f[0], "let")) rows++;
        else if (rows == 1 && !strcmp(f[0], "template") &&
                 !strcmp(f[2], "boundary")) rows++;
        else if (rows == 2 && !strcmp(f[0], "rows") &&
                 !strcmp(f[1], "floatconst") && !strcmp(f[2], "entry") &&
                 !strcmp(f[4], "k2-floatconst")) {
            direct_bindings(bindings, f[7], facts);
            rows++;
        } else { free(s); break; }
        free(s);
        if (rows == 3) break;
    }
    if (fclose(manifest) || rows != 3) die("floatconst entry manifest changed");
    for (int i = 0; i < 2; i++) {
        Value *actions = value_new(JARR), *act = value_new(JARR); char key[16];
        snprintf(key, sizeof(key), "reject%d", i);
        value_put(act, NULL, value_string("REJECT"));
        value_put(act, NULL, value_string(i ? "not covered: decimal floating suffix" :
                                           "not covered: decimal floating constant"));
        value_put(actions, NULL, act);
        value_put(sequences, key, actions);
    }
    install_section(g, "exec/parse2/floatconst-byte.tsv", "entry", 'b', bindings, sequences);
    install_section(g, "exec/parse2/floatconst-result.tsv", "entry", 'r', bindings, sequences);
}
static void parse2_float_digits(Graph *g) {
    FILE *manifest = fopen("exec/parse2/floatconst-manifest.tsv", "rb");
    char *s; int row = 0, digit = 0;
    if (!manifest) die("cannot open floatconst manifest");
    while ((s = line(manifest))) {
        char *f[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, f, 9);
        if (n != 9) die("floatconst manifest column count");
        if (row++ < 3) { free(s); continue; }
        if (strcmp(f[0], "template") || strcmp(f[1], "floatconst") ||
            strcmp(f[2], "digit")) { free(s); break; }
        {
            Value *facts = load_facts_expr(f[4]), *bindings = value_new(JOBJ);
            Value *opts = value_json(f[8], "floatconst digit options");
            Value *domain = value_path(facts, value_text(value_get(opts, "domain_keys")));
            Buffer expanded = expand_template_file("exec/parse2/floatconst-template.tsv", facts, "digit");
            Buffer bound = {0}; FILE *table = buffer_file(&expanded); char *rule;
            direct_bindings(bindings, f[7], facts);
            while ((rule = line(table))) {
                char *c[4]; int cols = fields_tab(rule, c, 4);
                if (cols != 4) die("floatconst digit template column count");
                buf_add(&bound, bound_name(c[0], bindings), strlen(bound_name(c[0], bindings)));
                buf_char(&bound, '\t'); buf_add(&bound, c[1], strlen(c[1]));
                buf_char(&bound, '\t');
                buf_add(&bound, bound_name(c[2], bindings), strlen(bound_name(c[2], bindings)));
                buf_char(&bound, '\t'); buf_add(&bound, c[3], strlen(c[3]));
                buf_char(&bound, '\n'); free(rule);
            }
            if (ferror(table) || fclose(table)) die("floatconst digit template read failed");
            table = buffer_file(&bound);
            install_delta_text(g, table, 'b', domain, NULL, NULL, 0, 0, NULL, "START");
            if (fclose(table)) die("floatconst digit table close failed");
        }
        digit++; free(s);
        if (digit == 3) break;
    }
    if (fclose(manifest) || digit != 3) die("incomplete floatconst digit manifest");
}
static void parse2_float_rows(Graph *g) {
    FILE *manifest = fopen("exec/parse2/floatconst-manifest.tsv", "rb");
    Value *sequences = value_new(JOBJ); char *s; int row = 0, installed = 0;
    if (!manifest) die("cannot open floatconst manifest");
    for (int i = 0; i < 2; i++) {
        Value *actions = value_new(JARR), *act = value_new(JARR); char key[16];
        snprintf(key, sizeof(key), "reject%d", i);
        value_put(act, NULL, value_string("REJECT"));
        value_put(act, NULL, value_string(i ? "not covered: decimal floating suffix" :
                                           "not covered: decimal floating constant"));
        value_put(actions, NULL, act);
        value_put(sequences, key, actions);
    }
    while ((s = line(manifest))) {
        char *f[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, f, 9);
        if (n != 9) die("floatconst manifest column count");
        if (row++ < 6) { free(s); continue; }
        if (strcmp(f[0], "rows") || strcmp(f[1], "floatconst") ||
            strcmp(f[4], "k2-floatconst") || strcmp(f[5], "-"))
            die("unsupported floatconst row");
        {
            Value *facts = load_fact("k2-floatconst"), *bindings = value_new(JOBJ);
            value_put(facts, "reject0", value_get(sequences, "reject0"));
            value_put(facts, "reject1", value_get(sequences, "reject1"));
            direct_bindings(bindings, f[7], facts);
            install_section(g, "exec/parse2/floatconst-byte.tsv", f[2], 'b', bindings, sequences);
            install_section(g, "exec/parse2/floatconst-result.tsv", f[2], 'r', bindings, sequences);
        }
        installed++; free(s);
    }
    if (ferror(manifest) || fclose(manifest) || installed != 10)
        die("incomplete floatconst rows");
}
static void parse2_autoscan(Graph *g) {
    FILE *manifest = fopen("exec/parse/autoscan-manifest.tsv", "rb");
    FILE *names; char *s; int rows = 0, labels = 0;
    Value *facts = load_fact("parse-constants"), *bindings = value_new(JOBJ);
    Value *sequences = value_new(JOBJ), *classes = value_new(JOBJ);
    Value *tokens = value_get(load_fact("parse-tokens"), "TK");
    if (!manifest) die("cannot open autoscan manifest");
    while ((s = line(manifest))) {
        char *f[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, f, 9);
        if (n != 9 || strcmp(f[0], "rows") || strcmp(f[1], "autoscan") ||
            strcmp(f[2], "auto") || strcmp(f[4], "parse-constants") || rows++)
            die("unsupported autoscan manifest");
        {
            Value *opts = value_json(f[8], "autoscan options");
            Value *map = value_get(opts, "bindmap"), *tokenmap = value_get(opts, "tokens");
            Value *act = value_new(JARR), *seq = value_new(JARR);
            const char *reject = "reject_header=@rej:";
            if (strncmp(f[6], reject, strlen(reject))) die("autoscan rejection changed");
            value_put(act, NULL, value_string("REJECT"));
            value_put(act, NULL, value_string(f[6] + strlen(reject)));
            value_put(seq, NULL, act); value_put(sequences, "reject_header", seq);
            if (!map || map->kind != JOBJ || !tokenmap || tokenmap->kind != JOBJ)
                die("autoscan map declaration missing");
            for (size_t i = 0; i < map->n; i++) {
                const char *ref = value_text(map->items[i].value);
                if (strncmp(ref, "parse-constants!.", 17)) die("autoscan map fact changed");
                value_put(bindings, map->items[i].key, value_get(facts, ref + 17));
            }
            for (size_t i = 0; i < tokenmap->n; i++) {
                const char *name = value_text(tokenmap->items[i].value);
                Value *code = !strcmp(name, "identifier") ? value_get(facts, "TK_ID") :
                              value_get(tokens, name);
                Value *list = value_new(JARR);
                if (!code || code->kind != JINT) die("autoscan token code missing");
                value_put(list, NULL, code); value_put(classes, tokenmap->items[i].key, list);
            }
        }
        free(s);
    }
    if (ferror(manifest) || fclose(manifest) || rows != 1)
        die("incomplete autoscan manifest");
    names = fopen("exec/parse/autoscan-names.tsv", "rb");
    if (!names) die("cannot open autoscan names");
    while ((s = line(names))) {
        char *f[3]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, f, 3);
        if (n != 3 || (!labels && strcmp(f[0], "AUTO_r1")))
            die("autoscan names changed");
        {
            char owner[256]; char *label;
            if (snprintf(owner, sizeof(owner), "%s.autoscan_%s", f[1], f[0]) >= (int)sizeof(owner))
                die("autoscan owner too long");
            label = fresh_label(owner, f[2]);
            value_put(bindings, f[0], value_string(label)); free(label);
        }
        labels++; free(s);
    }
    if (ferror(names) || fclose(names) || labels != 19) die("autoscan names incomplete");
    install_section_classes(g, "exec/parse/autoscan-byte.tsv", "auto", 'b',
                            bindings, sequences, classes);
    install_section_classes(g, "exec/parse/autoscan-result.tsv", "auto", 'r',
                            bindings, sequences, classes);
}
static void parse2_unary_rows(Graph *g, const char *part, int row_index) {
    char section[64];
    if (snprintf(section, sizeof(section), "%s.all", part) >= (int)sizeof(section))
        die("unarycontrol section too long");
    FILE *manifest = fopen("exec/parse2/unarycontrol-manifest.tsv", "rb");
    FILE *names; char *s, *bind_cell = NULL; int rows = 0, labels = 0;
    Value *facts = load_fact("k2-unary"), *bindings = value_new(JOBJ);
    Value *genfacts = load_fact("k2-gen2"), *sequences = NULL;
    Value *ufacts = value_path(genfacts, "unaryenv.ufacts");
    Value *classes = value_get(facts, "classes"), *seqnames = value_get(facts, "seq_names");
    if (!manifest) die("cannot open unarycontrol manifest");
    while ((s = line(manifest))) {
        char *f[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, f, 9);
        if (n != 9) die("unarycontrol manifest column count");
        if (rows == 0) {
            if (strcmp(f[0], "let")) die("unarycontrol sequence declaration changed");
            sequences = mapseq_construct(value_json(f[8], "unarycontrol sequences"), facts);
        }
        if (rows == row_index) {
            if (strcmp(f[0], "rows") || strcmp(f[1], "unarycontrol") ||
                strcmp(f[2], section) || strcmp(f[4], "k2-unary"))
                die("unarycontrol row declaration changed");
            bind_cell = copy(f[7]);
        }
        rows++; free(s);
        if (rows > row_index) break;
    }
    if (fclose(manifest) || rows != row_index + 1 || !sequences || !classes || !seqnames)
        die("unarycontrol row declaration missing");
    names = fopen("exec/parse2/unarycontrol-fresh.tsv", "rb");
    if (!names) die("cannot open unarycontrol fresh rows");
    while ((s = line(names))) {
        char *f[5]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, f, 5);
        if (n != 5) die("unarycontrol fresh row columns");
        if (!strcmp(f[0], "section")) {
            if (strcmp(f[1], "mode") || strcmp(f[2], "owner") ||
                strcmp(f[3], "kind") || strcmp(f[4], "key"))
                die("unarycontrol fresh header changed");
        } else if (!strcmp(f[0], part)) {
            char *label;
            if (strcmp(f[1], "all")) die("unarycontrol fresh mode changed");
            label = fresh_label(f[2], f[3]);
            value_put(bindings, f[4], value_string(label));
            free(label); labels++;
        }
        free(s);
    }
    if (ferror(names) || fclose(names) || !labels)
        die("unarycontrol head fresh rows missing");
    for (size_t i = 0; i < ufacts->n; i++)
        value_put(bindings, ufacts->items[i].key, ufacts->items[i].value);
    if (!bind_cell) die("unarycontrol row binding missing");
    direct_bindings(bindings, bind_cell, facts);
    free(bind_cell);
    {
        Value *selected = value_new(JOBJ);
        for (size_t i = 0; i < seqnames->n; i++) {
            const char *name = value_text(seqnames->items[i].value);
            Value *seq = value_get(sequences, name);
            if (!seq) die("unarycontrol named sequence missing");
            value_put(selected, name, seq);
        }
        install_section_classes(g, "exec/parse2/unarycontrol-byte.tsv", section, 'b',
                                bindings, selected, classes);
        install_section_classes(g, "exec/parse2/unarycontrol-result.tsv", section, 'r',
                                bindings, selected, classes);
    }
}
static void parse2_unary_head(Graph *g) { parse2_unary_rows(g, "part0", 1); }
static void parse2_unary_part4(Graph *g) { parse2_unary_rows(g, "part4", 4); }
static void parse2_unary_float_d(Graph *g) { parse2_unary_rows(g, "convert_float", 5); }
static void parse2_unary_float_s(Graph *g) { parse2_unary_rows(g, "convert_float", 6); }
static void parse2_unary_int_i(Graph *g) { parse2_unary_rows(g, "convert_int", 7); }
static void parse2_unary_int_u(Graph *g) { parse2_unary_rows(g, "convert_int", 8); }
static void parse2_unary_part6(Graph *g) { parse2_unary_rows(g, "part6", 9); }
static char *parse2_printfallback_head(Graph *g) {
    FILE *manifest = fopen("exec/parse2/printfallback-manifest.tsv", "rb");
    Value *sequences = NULL, *facts = value_new(JOBJ), *bindings = value_new(JOBJ);
    char *s; int row = 0;
    if (!manifest) die("cannot open printfallback manifest");
    while ((s = line(manifest))) {
        char *f[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, f, 9);
        if (n != 9) die("printfallback manifest column count");
        if (row == 0) {
            Value *opts;
            if (strcmp(f[0], "let") || strcmp(f[7], "-") || strcmp(f[8], "-") == 0)
                die("printfallback sequence declaration changed");
            opts = value_json(f[8], "printfallback sequences");
            sequences = mapseq_construct(opts, facts);
            for (size_t i = 0; i < sequences->n; i++)
                value_put(facts, sequences->items[i].key, sequences->items[i].value);
        } else if (row == 1) {
            if (strcmp(f[0], "rows") || strcmp(f[1], "printfallback") ||
                strcmp(f[2], "head") || strcmp(f[4], "k2-printfallback"))
                die("printfallback head declaration changed");
            direct_bindings(bindings, f[7], facts);
        }
        row++; free(s);
        if (row == 2) break;
    }
    if (fclose(manifest) || row != 2 || !sequences) die("printfallback head missing");
    install_section(g, "exec/parse2/printfallback-byte.tsv", "head", 'b', bindings, sequences);
    install_section(g, "exec/parse2/printfallback-result.tsv", "head", 'r', bindings, sequences);
    return copy(value_text(value_get(bindings, "PF_b1")));
}
static void parse2_printfallback_dispatch_int(Graph *g, const char *entry) {
    FILE *manifest = fopen("exec/parse2/printfallback-manifest.tsv", "rb");
    FILE *templ = fopen("exec/parse2/printfallback-template.tsv", "rb");
    Value *facts = NULL, *kinds; char *s; int row = 0, found = 0;
    if (!manifest || !templ) die("cannot open printfallback dispatch inputs");
    while ((s = line(manifest))) {
        char *f[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, f, 9);
        if (n != 9) die("printfallback manifest column count");
        if (row == 2) {
            if (strcmp(f[0], "template") || strcmp(f[1], "printfallback") ||
                strcmp(f[2], "dispatch")) die("printfallback dispatch declaration changed");
            facts = load_facts_expr(f[4]);
        }
        row++; free(s);
        if (row == 3) break;
    }
    if (fclose(manifest) || row != 3 || !facts) die("printfallback dispatch missing");
    kinds = value_get(facts, "kind");
    if (!kinds || kinds->kind != JARR) die("printfallback kind facts missing");
    while ((s = line(templ))) {
        char *f[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, f, 9);
        if (n != 9) die("printfallback template column count");
        if (!strcmp(f[0], "dispatch")) {
            if (found++ || strcmp(f[1], "kind") || strcmp(f[2], "kind") ||
                strcmp(f[4], "fill-edge") || strcmp(f[5], "$PF_b1") ||
                strcmp(f[7], "PF.convert.{kind.name}") || strcmp(f[8], "[]"))
                die("printfallback dispatch template changed");
            for (size_t i = 0; i < kinds->n; i++) {
                Value *kind = kinds->items[i].value;
                char key[16], target[128];
                number_text((int)value_get(kind, "i")->number, key);
                if (snprintf(target, sizeof(target), "PF.convert.%s",
                             value_text(value_get(kind, "name"))) >= (int)sizeof(target))
                    die("printfallback target too long");
                edge_set(g, entry, 'r', key, target, "[]");
            }
        }
        free(s);
    }
    if (ferror(templ) || fclose(templ) || found != 1)
        die("printfallback dispatch template missing");
}
static void parse2_printfallback_body_int(Graph *g, const char *entry) {
    FILE *manifest = fopen("exec/parse2/printfallback-manifest.tsv", "rb");
    Value *sequences = NULL, *seqbindings = value_new(JOBJ);
    Value *bindings = value_new(JOBJ), *ctx = value_new(JOBJ);
    char *s; int row = 0;
    if (!manifest) die("cannot open printfallback body manifest");
    value_put(ctx, "PF_b1", value_string(entry));
    while ((s = line(manifest))) {
        char *f[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, f, 9);
        if (n != 9) die("printfallback manifest column count");
        if (row == 0) {
            if (strcmp(f[0], "let")) die("printfallback sequence declaration changed");
            sequences = mapseq_construct(value_json(f[8], "printfallback sequences"), ctx);
        } else if (row == 3) {
            if (strcmp(f[0], "rows") || strcmp(f[1], "printfallback") ||
                strcmp(f[2], "body") || strcmp(f[4], "k2-printfallback") || !sequences)
                die("printfallback body declaration changed");
            direct_bindings(seqbindings, f[6], sequences);
            direct_bindings(bindings, f[7], ctx);
        }
        row++; free(s);
        if (row == 4) break;
    }
    if (fclose(manifest) || row != 4 || !value_get(seqbindings, "body"))
        die("printfallback body missing");
    install_section(g, "exec/parse2/printfallback-byte.tsv", "body", 'b', bindings, seqbindings);
    install_section(g, "exec/parse2/printfallback-result.tsv", "body", 'r', bindings, seqbindings);
}
static void parse2_unary_compound(Graph *g) {
    FILE *manifest = fopen("exec/parse2/unarycontrol-manifest.tsv", "rb");
    FILE *control = fopen("exec/parse2/control-manifest.tsv", "rb");
    Value *facts = load_fact("k2-control"), *bindings = value_new(JOBJ);
    Value *genfacts = load_fact("k2-gen2");
    Value *extra = value_path(genfacts, "unaryenv.ucx");
    Value *consts = value_get(facts, "consts"), *sequences = NULL;
    Value *classes = value_get(facts, "classes");
    char *s; int rows = 0, declared = 0;
    if (!manifest || !control || !consts || consts->kind != JOBJ ||
        !extra || extra->kind != JOBJ || !classes)
        die("unary compound inputs missing");
    while ((s = line(manifest))) {
        char *f[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, f, 9);
        if (n != 9) die("unarycontrol manifest column count");
        if (rows++ < 2) { free(s); continue; }
        if (strcmp(f[0], "call") || strcmp(f[1], "control") ||
            strcmp(f[4], "k2-unary") || strcmp(f[7], "control_section=@str:compound,statement=@str:STMT,extra=$ucx,seqb=empty"))
            die("unary compound call changed");
        free(s); break;
    }
    if (fclose(manifest) || rows != 3) die("unary compound call missing");
    while ((s = line(control))) {
        char *f[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, f, 9);
        if (n != 9) die("control manifest column count");
        if (!declared && !strcmp(f[0], "rows") && !strcmp(f[1], "control") &&
            !strcmp(f[3], "!fact:control_export")) {
            Value *opts = value_json(f[8], "unary compound control options");
            sequences = mapseq_construct(opts, facts); declared++;
        }
        free(s);
    }
    if (ferror(control) || fclose(control) || declared != 1)
        die("control manifest declaration missing");
    for (size_t i = 0; i < consts->n; i++)
        value_put(bindings, consts->items[i].key, consts->items[i].value);
    for (size_t i = 0; i < extra->n; i++)
        value_put(bindings, extra->items[i].key, extra->items[i].value);
    value_put(bindings, "statement", value_string("STMT"));
    install_section_classes(g, "exec/parse2/control-byte.tsv", "compound", 'b',
                            bindings, sequences, classes);
    install_section_classes(g, "exec/parse2/control-result.tsv", "compound", 'r',
                            bindings, sequences, classes);
}
static void parse2_unary_cast_void(Graph *g) {
    FILE *manifest = fopen("exec/parse2/unarycontrol-manifest.tsv", "rb");
    char *s; int row = 0, found = 0;
    if (!manifest) die("cannot open unarycontrol manifest");
    while ((s = line(manifest))) {
        char *f[9]; int n;
        if (!*s || *s == '#') { free(s); continue; }
        n = fields_tab(s, f, 9);
        if (n != 9) die("unarycontrol manifest column count");
        if (row++ < 3) { free(s); continue; }
        if (strcmp(f[0], "rows") || strcmp(f[1], "width") ||
            strcmp(f[2], "cast-void") || strcmp(f[4], "-"))
            die("unarycontrol cast-void declaration changed");
        found++; free(s); break;
    }
    if (fclose(manifest) || found != 1) die("unarycontrol cast-void missing");
    install_section(g, "exec/parse2/width-byte.tsv", "cast-void", 'b', NULL, NULL);
    install_section(g, "exec/parse2/width-result.tsv", "cast-void", 'r', NULL, NULL);
}
static void inspect_parse2_startup_graph(const char *outpath) {
    Graph g = {0}; FILE *out;
    build_parse2_token_graph(&g);
    parse2_startup_edits(&g);
    finish(&g);
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output_graph(out, &g, "START", NULL); if (fclose(out)) die("output close failed");
}
static void inspect_parse2_startup_control_graph(const char *outpath) {
    Graph g = {0}; FILE *out;
    build_parse2_token_graph(&g);
    parse2_startup_edits(&g);
    parse2_startup_control(&g);
    finish(&g);
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output_graph(out, &g, "START", NULL); if (fclose(out)) die("output close failed");
}
static void inspect_parse2_startup_strings_graph(const char *outpath) {
    Graph g = {0}; FILE *out;
    build_parse2_token_graph(&g);
    parse2_startup_edits(&g);
    parse2_string_span(&g);
    finish(&g);
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output_graph(out, &g, "START", NULL); if (fclose(out)) die("output close failed");
}
static void inspect_parse2_startup_prefix_graph(const char *outpath) {
    Graph g = {0}; FILE *out;
    build_parse2_token_graph(&g);
    parse2_startup_edits(&g);
    parse2_startup_control(&g);
    parse2_string_span(&g);
    finish(&g);
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output_graph(out, &g, "START", NULL); if (fclose(out)) die("output close failed");
}
static void inspect_parse2_initializer_head_graph(const char *outpath) {
    Graph g = {0}; FILE *out;
    build_parse2_token_graph(&g);
    parse2_startup_edits(&g);
    parse2_startup_control(&g);
    parse2_string_span(&g);
    parse2_string_initializer_head(&g);
    finish(&g);
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output_graph(out, &g, "START", NULL); if (fclose(out)) die("output close failed");
}
static void inspect_parse2_strwalk_head_graph(const char *outpath) {
    Graph g = {0}; FILE *out;
    build_parse2_token_graph(&g);
    parse2_startup_edits(&g);
    parse2_startup_control(&g);
    parse2_string_span(&g);
    parse2_string_initializer_head(&g);
    parse2_string_walk_head(&g, "SI.walk", "SI.byte", "SI.end");
    finish(&g);
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output_graph(out, &g, "START", NULL); if (fclose(out)) die("output close failed");
}
static void inspect_parse2_strwalk_escape_graph(const char *outpath) {
    Graph g = {0}; FILE *out;
    build_parse2_token_graph(&g);
    parse2_startup_edits(&g);
    parse2_startup_control(&g);
    parse2_string_span(&g);
    parse2_string_initializer_head(&g);
    parse2_string_walk_head(&g, "SI.walk", "SI.byte", "SI.end");
    parse2_string_walk_escape(&g, "SI.walk", "SI.byte");
    finish(&g);
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output_graph(out, &g, "START", NULL); if (fclose(out)) die("output close failed");
}
static void inspect_parse2_strwalk_graph(const char *outpath) {
    Graph g = {0}; FILE *out;
    build_parse2_token_graph(&g);
    parse2_startup_edits(&g);
    parse2_startup_control(&g);
    parse2_string_span(&g);
    parse2_string_initializer_head(&g);
    parse2_string_walk_head(&g, "SI.walk", "SI.byte", "SI.end");
    parse2_string_walk_escape(&g, "SI.walk", "SI.byte");
    parse2_string_walk_tail(&g, "SI.walk", "SI.byte", "SI.end");
    finish(&g);
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output_graph(out, &g, "START", NULL); if (fclose(out)) die("output close failed");
}
static void inspect_parse2_startup_branch_graph(const char *outpath) {
    Graph g = {0}; FILE *out;
    build_parse2_token_graph(&g);
    parse2_startup_edits(&g);
    parse2_startup_control(&g);
    parse2_string_span(&g);
    parse2_string_initializer_head(&g);
    parse2_string_walk_head(&g, "SI.walk", "SI.byte", "SI.end");
    parse2_string_walk_escape(&g, "SI.walk", "SI.byte");
    parse2_string_walk_tail(&g, "SI.walk", "SI.byte", "SI.end");
    parse2_string_initializer_tail(&g);
    finish(&g);
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output_graph(out, &g, "START", NULL); if (fclose(out)) die("output close failed");
}
static void inspect_parse2_numeric_graph(const char *outpath) {
    Graph g = {0}; FILE *out;
    build_parse2_token_graph(&g);
    parse2_startup_edits(&g);
    parse2_startup_control(&g);
    parse2_string_span(&g);
    parse2_string_initializer_head(&g);
    parse2_string_walk_head(&g, "SI.walk", "SI.byte", "SI.end");
    parse2_string_walk_escape(&g, "SI.walk", "SI.byte");
    parse2_string_walk_tail(&g, "SI.walk", "SI.byte", "SI.end");
    parse2_string_initializer_tail(&g);
    parse2_numeric(&g);
    finish(&g);
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output_graph(out, &g, "START", NULL); if (fclose(out)) die("output close failed");
}
static void inspect_parse2_float_boundary_graph(const char *outpath) {
    Graph g = {0}; FILE *out;
    build_parse2_token_graph(&g);
    parse2_startup_edits(&g);
    parse2_startup_control(&g);
    parse2_string_span(&g);
    parse2_string_initializer_head(&g);
    parse2_string_walk_head(&g, "SI.walk", "SI.byte", "SI.end");
    parse2_string_walk_escape(&g, "SI.walk", "SI.byte");
    parse2_string_walk_tail(&g, "SI.walk", "SI.byte", "SI.end");
    parse2_string_initializer_tail(&g);
    parse2_numeric(&g);
    parse2_float_boundary(&g);
    finish(&g);
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output_graph(out, &g, "START", NULL); if (fclose(out)) die("output close failed");
}
static void inspect_parse2_float_entry_graph(const char *outpath) {
    Graph g = {0}; FILE *out;
    build_parse2_token_graph(&g);
    parse2_startup_edits(&g);
    parse2_startup_control(&g);
    parse2_string_span(&g);
    parse2_string_initializer_head(&g);
    parse2_string_walk_head(&g, "SI.walk", "SI.byte", "SI.end");
    parse2_string_walk_escape(&g, "SI.walk", "SI.byte");
    parse2_string_walk_tail(&g, "SI.walk", "SI.byte", "SI.end");
    parse2_string_initializer_tail(&g);
    parse2_numeric(&g);
    parse2_float_boundary(&g);
    parse2_float_entry(&g);
    finish(&g);
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output_graph(out, &g, "START", NULL); if (fclose(out)) die("output close failed");
}
static void inspect_parse2_float_digits_graph(const char *outpath) {
    Graph g = {0}; FILE *out;
    build_parse2_token_graph(&g);
    parse2_startup_edits(&g);
    parse2_startup_control(&g);
    parse2_string_span(&g);
    parse2_string_initializer_head(&g);
    parse2_string_walk_head(&g, "SI.walk", "SI.byte", "SI.end");
    parse2_string_walk_escape(&g, "SI.walk", "SI.byte");
    parse2_string_walk_tail(&g, "SI.walk", "SI.byte", "SI.end");
    parse2_string_initializer_tail(&g);
    parse2_numeric(&g);
    parse2_float_boundary(&g);
    parse2_float_entry(&g);
    parse2_float_digits(&g);
    finish(&g);
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output_graph(out, &g, "START", NULL); if (fclose(out)) die("output close failed");
}
static void inspect_parse2_float_rows_graph(const char *outpath) {
    Graph g = {0}; FILE *out;
    build_parse2_token_graph(&g);
    parse2_startup_edits(&g);
    parse2_startup_control(&g);
    parse2_string_span(&g);
    parse2_string_initializer_head(&g);
    parse2_string_walk_head(&g, "SI.walk", "SI.byte", "SI.end");
    parse2_string_walk_escape(&g, "SI.walk", "SI.byte");
    parse2_string_walk_tail(&g, "SI.walk", "SI.byte", "SI.end");
    parse2_string_initializer_tail(&g);
    parse2_numeric(&g);
    parse2_float_boundary(&g);
    parse2_float_entry(&g);
    parse2_float_digits(&g);
    parse2_float_rows(&g);
    finish(&g);
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output_graph(out, &g, "START", NULL); if (fclose(out)) die("output close failed");
}
static void inspect_parse2_autoscan_graph(const char *outpath) {
    Graph g = {0}; FILE *out;
    build_parse2_token_graph(&g);
    parse2_startup_edits(&g);
    parse2_startup_control(&g);
    parse2_string_span(&g);
    parse2_string_initializer_head(&g);
    parse2_string_walk_head(&g, "SI.walk", "SI.byte", "SI.end");
    parse2_string_walk_escape(&g, "SI.walk", "SI.byte");
    parse2_string_walk_tail(&g, "SI.walk", "SI.byte", "SI.end");
    parse2_string_initializer_tail(&g);
    parse2_numeric(&g);
    parse2_float_boundary(&g);
    parse2_float_entry(&g);
    parse2_float_digits(&g);
    parse2_float_rows(&g);
    parse2_autoscan(&g);
    finish(&g);
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output_graph(out, &g, "START", NULL); if (fclose(out)) die("output close failed");
}
static void inspect_parse2_unary_head_graph(const char *outpath) {
    Graph g = {0}; FILE *out;
    build_parse2_token_graph(&g);
    parse2_startup_edits(&g);
    parse2_startup_control(&g);
    parse2_string_span(&g);
    parse2_string_initializer_head(&g);
    parse2_string_walk_head(&g, "SI.walk", "SI.byte", "SI.end");
    parse2_string_walk_escape(&g, "SI.walk", "SI.byte");
    parse2_string_walk_tail(&g, "SI.walk", "SI.byte", "SI.end");
    parse2_string_initializer_tail(&g);
    parse2_numeric(&g);
    parse2_float_boundary(&g);
    parse2_float_entry(&g);
    parse2_float_digits(&g);
    parse2_float_rows(&g);
    parse2_autoscan(&g);
    parse2_unary_head(&g);
    finish(&g);
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output_graph(out, &g, "START", NULL); if (fclose(out)) die("output close failed");
}
static void inspect_parse2_unary_compound_graph(const char *outpath) {
    Graph g = {0}; FILE *out;
    build_parse2_token_graph(&g);
    parse2_startup_edits(&g);
    parse2_startup_control(&g);
    parse2_string_span(&g);
    parse2_string_initializer_head(&g);
    parse2_string_walk_head(&g, "SI.walk", "SI.byte", "SI.end");
    parse2_string_walk_escape(&g, "SI.walk", "SI.byte");
    parse2_string_walk_tail(&g, "SI.walk", "SI.byte", "SI.end");
    parse2_string_initializer_tail(&g);
    parse2_numeric(&g);
    parse2_float_boundary(&g);
    parse2_float_entry(&g);
    parse2_float_digits(&g);
    parse2_float_rows(&g);
    parse2_autoscan(&g);
    parse2_unary_head(&g);
    parse2_unary_compound(&g);
    finish(&g);
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output_graph(out, &g, "START", NULL); if (fclose(out)) die("output close failed");
}
static void inspect_parse2_unary_cast_void_graph(const char *outpath) {
    Graph g = {0}; FILE *out;
    build_parse2_token_graph(&g);
    parse2_startup_edits(&g);
    parse2_startup_control(&g);
    parse2_string_span(&g);
    parse2_string_initializer_head(&g);
    parse2_string_walk_head(&g, "SI.walk", "SI.byte", "SI.end");
    parse2_string_walk_escape(&g, "SI.walk", "SI.byte");
    parse2_string_walk_tail(&g, "SI.walk", "SI.byte", "SI.end");
    parse2_string_initializer_tail(&g);
    parse2_numeric(&g);
    parse2_float_boundary(&g);
    parse2_float_entry(&g);
    parse2_float_digits(&g);
    parse2_float_rows(&g);
    parse2_autoscan(&g);
    parse2_unary_head(&g);
    parse2_unary_compound(&g);
    parse2_unary_cast_void(&g);
    finish(&g);
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output_graph(out, &g, "START", NULL); if (fclose(out)) die("output close failed");
}
static void inspect_parse2_unary_part4_graph(const char *outpath) {
    Graph g = {0}; FILE *out;
    build_parse2_token_graph(&g);
    parse2_startup_edits(&g);
    parse2_startup_control(&g);
    parse2_string_span(&g);
    parse2_string_initializer_head(&g);
    parse2_string_walk_head(&g, "SI.walk", "SI.byte", "SI.end");
    parse2_string_walk_escape(&g, "SI.walk", "SI.byte");
    parse2_string_walk_tail(&g, "SI.walk", "SI.byte", "SI.end");
    parse2_string_initializer_tail(&g);
    parse2_numeric(&g);
    parse2_float_boundary(&g);
    parse2_float_entry(&g);
    parse2_float_digits(&g);
    parse2_float_rows(&g);
    parse2_autoscan(&g);
    parse2_unary_head(&g);
    parse2_unary_compound(&g);
    parse2_unary_cast_void(&g);
    parse2_unary_part4(&g);
    finish(&g);
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output_graph(out, &g, "START", NULL); if (fclose(out)) die("output close failed");
}
static void inspect_parse2_unary_float_d_graph(const char *outpath) {
    Graph g = {0}; FILE *out;
    build_parse2_token_graph(&g);
    parse2_startup_edits(&g);
    parse2_startup_control(&g);
    parse2_string_span(&g);
    parse2_string_initializer_head(&g);
    parse2_string_walk_head(&g, "SI.walk", "SI.byte", "SI.end");
    parse2_string_walk_escape(&g, "SI.walk", "SI.byte");
    parse2_string_walk_tail(&g, "SI.walk", "SI.byte", "SI.end");
    parse2_string_initializer_tail(&g);
    parse2_numeric(&g);
    parse2_float_boundary(&g);
    parse2_float_entry(&g);
    parse2_float_digits(&g);
    parse2_float_rows(&g);
    parse2_autoscan(&g);
    parse2_unary_head(&g);
    parse2_unary_compound(&g);
    parse2_unary_cast_void(&g);
    parse2_unary_part4(&g);
    parse2_unary_float_d(&g);
    finish(&g);
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output_graph(out, &g, "START", NULL); if (fclose(out)) die("output close failed");
}
static void inspect_parse2_unary_float_s_graph(const char *outpath) {
    Graph g = {0}; FILE *out;
    build_parse2_token_graph(&g);
    parse2_startup_edits(&g);
    parse2_startup_control(&g);
    parse2_string_span(&g);
    parse2_string_initializer_head(&g);
    parse2_string_walk_head(&g, "SI.walk", "SI.byte", "SI.end");
    parse2_string_walk_escape(&g, "SI.walk", "SI.byte");
    parse2_string_walk_tail(&g, "SI.walk", "SI.byte", "SI.end");
    parse2_string_initializer_tail(&g);
    parse2_numeric(&g);
    parse2_float_boundary(&g);
    parse2_float_entry(&g);
    parse2_float_digits(&g);
    parse2_float_rows(&g);
    parse2_autoscan(&g);
    parse2_unary_head(&g);
    parse2_unary_compound(&g);
    parse2_unary_cast_void(&g);
    parse2_unary_part4(&g);
    parse2_unary_float_d(&g);
    parse2_unary_float_s(&g);
    finish(&g);
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output_graph(out, &g, "START", NULL); if (fclose(out)) die("output close failed");
}
static void inspect_parse2_unary_int_graph(const char *outpath) {
    Graph g = {0}; FILE *out;
    build_parse2_token_graph(&g);
    parse2_startup_edits(&g);
    parse2_startup_control(&g);
    parse2_string_span(&g);
    parse2_string_initializer_head(&g);
    parse2_string_walk_head(&g, "SI.walk", "SI.byte", "SI.end");
    parse2_string_walk_escape(&g, "SI.walk", "SI.byte");
    parse2_string_walk_tail(&g, "SI.walk", "SI.byte", "SI.end");
    parse2_string_initializer_tail(&g);
    parse2_numeric(&g);
    parse2_float_boundary(&g);
    parse2_float_entry(&g);
    parse2_float_digits(&g);
    parse2_float_rows(&g);
    parse2_autoscan(&g);
    parse2_unary_head(&g);
    parse2_unary_compound(&g);
    parse2_unary_cast_void(&g);
    parse2_unary_part4(&g);
    parse2_unary_float_d(&g);
    parse2_unary_float_s(&g);
    parse2_unary_int_i(&g);
    parse2_unary_int_u(&g);
    finish(&g);
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output_graph(out, &g, "START", NULL); if (fclose(out)) die("output close failed");
}
static void inspect_parse2_unary_part6_graph(const char *outpath) {
    Graph g = {0}; FILE *out;
    build_parse2_token_graph(&g);
    parse2_startup_edits(&g);
    parse2_startup_control(&g);
    parse2_string_span(&g);
    parse2_string_initializer_head(&g);
    parse2_string_walk_head(&g, "SI.walk", "SI.byte", "SI.end");
    parse2_string_walk_escape(&g, "SI.walk", "SI.byte");
    parse2_string_walk_tail(&g, "SI.walk", "SI.byte", "SI.end");
    parse2_string_initializer_tail(&g);
    parse2_numeric(&g);
    parse2_float_boundary(&g);
    parse2_float_entry(&g);
    parse2_float_digits(&g);
    parse2_float_rows(&g);
    parse2_autoscan(&g);
    parse2_unary_head(&g);
    parse2_unary_compound(&g);
    parse2_unary_cast_void(&g);
    parse2_unary_part4(&g);
    parse2_unary_float_d(&g);
    parse2_unary_float_s(&g);
    parse2_unary_int_i(&g);
    parse2_unary_int_u(&g);
    parse2_unary_part6(&g);
    finish(&g);
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output_graph(out, &g, "START", NULL); if (fclose(out)) die("output close failed");
}
static void inspect_parse2_printfallback_head_graph(const char *outpath) {
    Graph g = {0}; FILE *out;
    build_parse2_token_graph(&g);
    parse2_startup_edits(&g);
    parse2_startup_control(&g);
    parse2_string_span(&g);
    parse2_string_initializer_head(&g);
    parse2_string_walk_head(&g, "SI.walk", "SI.byte", "SI.end");
    parse2_string_walk_escape(&g, "SI.walk", "SI.byte");
    parse2_string_walk_tail(&g, "SI.walk", "SI.byte", "SI.end");
    parse2_string_initializer_tail(&g);
    parse2_numeric(&g);
    parse2_float_boundary(&g);
    parse2_float_entry(&g);
    parse2_float_digits(&g);
    parse2_float_rows(&g);
    parse2_autoscan(&g);
    parse2_unary_head(&g);
    parse2_unary_compound(&g);
    parse2_unary_cast_void(&g);
    parse2_unary_part4(&g);
    parse2_unary_float_d(&g);
    parse2_unary_float_s(&g);
    parse2_unary_int_i(&g);
    parse2_unary_int_u(&g);
    parse2_unary_part6(&g);
    parse2_printfallback_head(&g);
    finish(&g);
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output_graph(out, &g, "START", NULL); if (fclose(out)) die("output close failed");
}
static void inspect_parse2_printfallback_dispatch_graph(const char *outpath) {
    Graph g = {0}; FILE *out; char *entry;
    build_parse2_token_graph(&g);
    parse2_startup_edits(&g);
    parse2_startup_control(&g);
    parse2_string_span(&g);
    parse2_string_initializer_head(&g);
    parse2_string_walk_head(&g, "SI.walk", "SI.byte", "SI.end");
    parse2_string_walk_escape(&g, "SI.walk", "SI.byte");
    parse2_string_walk_tail(&g, "SI.walk", "SI.byte", "SI.end");
    parse2_string_initializer_tail(&g);
    parse2_numeric(&g);
    parse2_float_boundary(&g);
    parse2_float_entry(&g);
    parse2_float_digits(&g);
    parse2_float_rows(&g);
    parse2_autoscan(&g);
    parse2_unary_head(&g);
    parse2_unary_compound(&g);
    parse2_unary_cast_void(&g);
    parse2_unary_part4(&g);
    parse2_unary_float_d(&g);
    parse2_unary_float_s(&g);
    parse2_unary_int_i(&g);
    parse2_unary_int_u(&g);
    parse2_unary_part6(&g);
    entry = parse2_printfallback_head(&g);
    parse2_printfallback_dispatch_int(&g, entry);
    finish(&g);
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output_graph(out, &g, "START", NULL); if (fclose(out)) die("output close failed");
}
static void inspect_parse2_printfallback_body_graph(const char *outpath) {
    Graph g = {0}; FILE *out; char *entry;
    build_parse2_token_graph(&g);
    parse2_startup_edits(&g);
    parse2_startup_control(&g);
    parse2_string_span(&g);
    parse2_string_initializer_head(&g);
    parse2_string_walk_head(&g, "SI.walk", "SI.byte", "SI.end");
    parse2_string_walk_escape(&g, "SI.walk", "SI.byte");
    parse2_string_walk_tail(&g, "SI.walk", "SI.byte", "SI.end");
    parse2_string_initializer_tail(&g);
    parse2_numeric(&g);
    parse2_float_boundary(&g);
    parse2_float_entry(&g);
    parse2_float_digits(&g);
    parse2_float_rows(&g);
    parse2_autoscan(&g);
    parse2_unary_head(&g);
    parse2_unary_compound(&g);
    parse2_unary_cast_void(&g);
    parse2_unary_part4(&g);
    parse2_unary_float_d(&g);
    parse2_unary_float_s(&g);
    parse2_unary_int_i(&g);
    parse2_unary_int_u(&g);
    parse2_unary_part6(&g);
    entry = parse2_printfallback_head(&g);
    parse2_printfallback_dispatch_int(&g, entry);
    parse2_printfallback_body_int(&g, entry);
    finish(&g);
    out = fopen(outpath, "wb"); if (!out) die("cannot open output");
    output_graph(out, &g, "START", NULL); if (fclose(out)) die("output close failed");
}
int main(int argc, char **argv) {
    Graph g = {0}; FILE *out;
    if (argc == 3 && !strcmp(argv[1], "inspect-parse2-token-graph")) {
        inspect_parse2_token_graph(argv[2]); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-parse2-startup-graph")) {
        inspect_parse2_startup_graph(argv[2]); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-parse2-startup-control-graph")) {
        inspect_parse2_startup_control_graph(argv[2]); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-parse2-startup-strings-graph")) {
        inspect_parse2_startup_strings_graph(argv[2]); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-parse2-startup-prefix-graph")) {
        inspect_parse2_startup_prefix_graph(argv[2]); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-parse2-initializer-head-graph")) {
        inspect_parse2_initializer_head_graph(argv[2]); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-parse2-strwalk-head-graph")) {
        inspect_parse2_strwalk_head_graph(argv[2]); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-parse2-strwalk-escape-graph")) {
        inspect_parse2_strwalk_escape_graph(argv[2]); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-parse2-strwalk-graph")) {
        inspect_parse2_strwalk_graph(argv[2]); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-parse2-startup-branch-graph")) {
        inspect_parse2_startup_branch_graph(argv[2]); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-parse2-numeric-graph")) {
        inspect_parse2_numeric_graph(argv[2]); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-parse2-float-boundary-graph")) {
        inspect_parse2_float_boundary_graph(argv[2]); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-parse2-float-entry-graph")) {
        inspect_parse2_float_entry_graph(argv[2]); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-parse2-float-digits-graph")) {
        inspect_parse2_float_digits_graph(argv[2]); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-parse2-float-rows-graph")) {
        inspect_parse2_float_rows_graph(argv[2]); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-parse2-autoscan-graph")) {
        inspect_parse2_autoscan_graph(argv[2]); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-parse2-unary-head-graph")) {
        inspect_parse2_unary_head_graph(argv[2]); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-parse2-unary-compound-graph")) {
        inspect_parse2_unary_compound_graph(argv[2]); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-parse2-unary-cast-void-graph")) {
        inspect_parse2_unary_cast_void_graph(argv[2]); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-parse2-unary-part4-graph")) {
        inspect_parse2_unary_part4_graph(argv[2]); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-parse2-unary-float-d-graph")) {
        inspect_parse2_unary_float_d_graph(argv[2]); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-parse2-unary-float-s-graph")) {
        inspect_parse2_unary_float_s_graph(argv[2]); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-parse2-unary-int-graph")) {
        inspect_parse2_unary_int_graph(argv[2]); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-parse2-unary-part6-graph")) {
        inspect_parse2_unary_part6_graph(argv[2]); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-parse2-printfallback-head-graph")) {
        inspect_parse2_printfallback_head_graph(argv[2]); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-parse2-printfallback-dispatch-graph")) {
        inspect_parse2_printfallback_dispatch_graph(argv[2]); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-parse2-printfallback-body-graph")) {
        inspect_parse2_printfallback_body_graph(argv[2]); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-parse2-tokens")) {
        inspect_parse2_tokens(argv[2]); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-nativeabi-modelinput")) {
        inspect_nativeabi_modelinput(argv[2]); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-nativeabi-calls")) {
        inspect_nativeabi_calls(argv[2]); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-nativeabi-head")) {
        inspect_nativeabi_head(argv[2], 0); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-nativeabi-trie")) {
        inspect_nativeabi_head(argv[2], 1); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-nativeabi-mid")) {
        inspect_nativeabi_head(argv[2], 2); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-nativeabi-union16")) {
        inspect_nativeabi_head(argv[2], 3); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-nativeabi-tail")) {
        inspect_nativeabi_head(argv[2], 4); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-nativeabi-ordered-main")) {
        inspect_nativeabi_head(argv[2], 5); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-nativeabi-ordered-header")) {
        inspect_nativeabi_head(argv[2], 6); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-nativeabi-ordered-recipe")) {
        inspect_nativeabi_head(argv[2], 7); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "nativeabi")) {
        inspect_nativeabi_head(argv[2], 8); return 0;
    }
    if ((argc == 3 || (argc == 4 && !strcmp(argv[3], "--no-autoinc"))) &&
        !strcmp(argv[1], "inspect-pp-through-autoinc")) {
        inspect_pp_through_autoinc(argv[2], argc == 4); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-pp-through-linedir")) {
        inspect_pp_through_linedir(argv[2], 0); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-pp-through-dsw")) {
        inspect_pp_through_linedir(argv[2], 1); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-pp-through-body")) {
        inspect_pp_through_linedir(argv[2], 2); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "pp")) {
        inspect_pp_through_linedir(argv[2], 3); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-pp-call-autoinc")) {
        inspect_pp_call_autoinc(argv[2]); return 0;
    }
    if ((argc == 3 || (argc == 4 && !strcmp(argv[3], "--no-autoinc"))) &&
        !strcmp(argv[1], "inspect-pp-prefix")) {
        inspect_pp_prefix(argv[2], argc == 4); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-pp-object-predefine")) {
        inspect_pp_object_predefine(argv[2]); return 0;
    }
    if (argc == 4 && !strcmp(argv[1], "inspect-pp-emit")) {
        char *end; unsigned long index = strtoul(argv[2], &end, 10);
        if (end == argv[2] || *end) die("invalid pp emit index");
        inspect_pp_emit(argv[3], index); return 0;
    }
    if (argc == 4 && !strcmp(argv[1], "inspect-pp-header")) {
        char *end; unsigned long index = strtoul(argv[2], &end, 10);
        if (end == argv[2] || *end) die("invalid pp header index");
        inspect_pp_header(argv[3], index); return 0;
    }
    if (argc == 4 && !strcmp(argv[1], "inspect-pp-body")) {
        char *end; unsigned long index = strtoul(argv[2], &end, 10);
        if (end == argv[2] || *end) die("invalid pp body index");
        inspect_pp_body(argv[3], index); return 0;
    }
    if (argc == 4 && !strcmp(argv[1], "inspect-pp-key")) {
        char *end; unsigned long index = strtoul(argv[2], &end, 10);
        if (end == argv[2] || *end) die("invalid pp key index");
        inspect_pp_key(argv[3], index); return 0;
    }
    if (argc == 4 && !strcmp(argv[1], "inspect-pp-autoinc")) {
        inspect_pp_autoinc(argv[2], argv[3]); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-pp-locations")) {
        inspect_pp_locations(argv[2]); return 0;
    }
    if (argc == 4 && !strcmp(argv[1], "inspect-pp-template")) {
        inspect_pp_template(argv[2], argv[3]); return 0;
    }
    if (argc == 4 && !strcmp(argv[1], "inspect-pp-assembly")) {
        inspect_pp_assembly(argv[2], argv[3]); return 0;
    }
    if ((argc == 4 || (argc == 5 && !strcmp(argv[4], "--no-autoinc"))) &&
        !strcmp(argv[1], "inspect-pp-rows")) {
        inspect_pp_rows(argv[2], argv[3], argc == 5); return 0;
    }
    if ((argc == 3 || (argc == 4 && !strcmp(argv[3], "--no-autoinc"))) &&
        !strcmp(argv[1], "inspect-pp-cli")) {
        inspect_pp_cli(argv[2], argc == 4); return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-pp-start")) {
        inspect_pp_start(argv[2]); return 0;
    }
    if (argc >= 3 && argc <= 7 && !strcmp(argv[1], "lex")) {
        Value *tok_names = value_get(load_fact("lex-gen"), "tok_names");
        const char *start = has_flag(argc - 3, argv + 3, "typed") ||
                            has_flag(argc - 3, argv + 3, "positions") ||
                            has_flag(argc - 3, argv + 3, "locations") ||
                            has_flag(argc - 3, argv + 3, "sourcefacts") ? "SF.start" : "DISPATCH";
        construct_lex(&g, argc - 3, argv + 3, start);
        out = fopen(argv[2], "wb"); if (!out) die("cannot open output");
        output_graph(out, &g, start, tok_names);
        if (fclose(out)) die("output close failed");
        fprintf(stderr, "lex states %lu\n", (unsigned long)g.n);
        return 0;
    }
    if (argc == 4 && !strcmp(argv[1], "inspect-delta-template")) {
        inspect_delta_template(argv[2], argv[3]); return 0;
    }
    if (argc == 4 && !strcmp(argv[1], "inspect-delta-table")) {
        inspect_delta_table(argv[2], argv[3]); return 0;
    }
    if (argc == 5 && !strcmp(argv[1], "inspect-template")) {
        inspect_template(argv[2], argv[3], argv[4]); return 0;
    }
    if (argc >= 2 && !strcmp(argv[1], "opt") &&
        !(argc == 3 || (argc == 4 && !strcmp(argv[3], "--o2"))))
        die("opt accepts only --o2");
    if ((argc == 3 || argc == 4) && !strcmp(argv[1], "opt")) {
        construct_opt(&g, argc == 4);
        out = fopen(argv[2], "wb"); if (!out) die("cannot open output");
        output(out, &g); if (fclose(out)) die("output close failed");
        fprintf(stderr, "opt states %lu\n", (unsigned long)g.n);
        return 0;
    }
    if (argc >= 4 && !strcmp(argv[1], "inspect-mapseq")) {
        char path[1024], *s; FILE *f; int found = 0;
        const char *sub = strchr(argv[2], ':');
        if (sub) {
            char *stage = copy_n(argv[2], (size_t)(sub - argv[2]));
            if (snprintf(path, sizeof(path), "exec/%s/%s-manifest.tsv", stage, sub + 1) >= (int)sizeof(path))
                die("manifest path too long");
            free(stage);
        } else if (snprintf(path, sizeof(path), "exec/%s/gen-manifest.tsv", argv[2]) >= (int)sizeof(path))
            die("manifest path too long");
        f = fopen(path, "rb"); if (!f) die("cannot open manifest");
        while ((s = line(f))) {
            char *field[9]; int n;
            if (!*s || *s == '#') { free(s); continue; }
            n = fields_tab(s, field, 9); if (n != 9) die("manifest column count");
            if (!strcmp(field[0], "let") && field[8][0] == '{' &&
                when_true(field[3], argc - 4, argv + 4)) {
                Value *opts = value_json(field[8], "manifest options");
                if (value_get(opts, "mapseq")) {
                    Value *facts = load_facts_expr(field[4]), *bindmap = value_get(opts, "bindmap");
                    Value *sequences;
                    if (bindmap) {
                        Value *source = value_path(facts, value_text(bindmap));
                        if (source->kind != JOBJ) die("mapseq bindmap is not an object");
                        for (size_t i = 0; i < source->n; i++)
                            value_put(facts, source->items[i].key, source->items[i].value);
                    }
                    sequences = mapseq_construct(opts, facts);
                    if (++found > 1) die("ambiguous mapseq row");
                    out = fopen(argv[3], "wb"); if (!out) die("cannot open output");
                    value_write(out, sequences); if (fclose(out)) die("output close failed");
                }
            }
            free(s);
        }
        if (ferror(f) || fclose(f)) die("manifest read failed");
        if (!found) die("mapseq row not found");
        return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "inspect-prn")) {
        install_prn_call(&g);
        out = fopen(argv[2], "wb"); if (!out) die("cannot open output");
        output(out, &g); if (fclose(out)) die("output close failed"); return 0;
    }
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
