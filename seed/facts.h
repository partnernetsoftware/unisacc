/* seed/facts.h -- 0.0.29 B4 (cc): the manifest interpreter's environment and cell evaluation, the C side
 * of exec/assemble.py Run.value/cells/one's fact scope.  Included by seed/gen.c after its Value type,
 * value_* helpers, load_fact() and die(); it adds no second value model or TSV reader.
 * Semantics follow assemble.py exactly; anything this subset does not cover dies by name. */
#ifndef SEED_FACTS_H
#define SEED_FACTS_H

/* Python str() of a fact value, as interpolation and @str/@fmt use it */
static char *seed_str(Value *v) {
    char n[64];
    if (!v) die("missing value in interpolation");
    switch (v->kind) {
    case JSTR: return copy(v->s);
    case JINT: snprintf(n, sizeof(n), "%lld", v->number); return copy(n);
    case JUINT: snprintf(n, sizeof(n), "%llu", v->unumber); return copy(n);
    case JBOOL: return copy(v->number ? "True" : "False");
    case JNULL: return copy("None");
    default: die("str() of a list or object is not used by manifests");
    }
    return 0;
}
static int seed_word(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_'; }

/* Value *seed_env_copy(obj): a shallow copy that keeps key order (dict(self.env)) */
static Value *seed_env_copy(Value *obj) {
    Value *out = value_new(JOBJ); size_t i;
    if (!obj) return out;
    if (obj->kind != JOBJ) die("environment is not an object");
    for (i = 0; i < obj->n; i++) value_put(out, obj->items[i].key, obj->items[i].value);
    return out;
}
static void seed_update(Value *into, Value *from) {
    size_t i;
    if (!from) return;
    if (from->kind != JOBJ) die("update from a non-object");
    for (i = 0; i < from->n; i++) value_put(into, from->items[i].key, from->items[i].value);
}

/* Run.one: facts = env, then each `+`-joined fact stem (load:STEM or STEM), then extra */
static Value *seed_facts_scope(Value *env, const char *expr, Value *extra) {
    Value *facts = seed_env_copy(env);
    if (expr && *expr && strcmp(expr, "-")) {
        char *parts = copy(expr), *p = parts;
        for (;;) {
            char *plus = strchr(p, '+');
            if (plus) *plus = 0;
            seed_update(facts, load_fact(!strncmp(p, "load:", 5) ? p + 5 : p));
            if (!plus) break;
            p = plus + 1;
        }
        free(parts);
    }
    seed_update(facts, extra);
    return facts;
}

/* Run.interp: {name} -> str(env[name]) when env has it, else left as written */
static char *seed_interp(const char *v, Value *env) {
    Buffer b = {0}; size_t i = 0;
    buf_add(&b, "", 0);
    while (v[i]) {
        if (v[i] == '{') {
            size_t j = i + 1;
            while (seed_word(v[j])) j++;
            if (j > i + 1 && v[j] == '}') {
                char *name = copy_n(v + i + 1, j - i - 1); Value *x = value_get(env, name);
                if (x) { char *s = seed_str(x); buf_add(&b, s, strlen(s)); free(s); free(name); i = j + 1; continue; }
                free(name);
            }
        }
        buf_char(&b, v[i]); i++;
    }
    return b.s;
}

/* assemble._fmt: {{ }} escapes, {name[key][0]...} paths into facts, a stray brace is an error */
static char *seed_fmt(const char *f, Value *facts) {
    Buffer b = {0}; size_t i = 0;
    buf_add(&b, "", 0);
    while (f[i]) {
        if ((f[i] == '{' && f[i + 1] == '{') || (f[i] == '}' && f[i + 1] == '}')) { buf_char(&b, f[i]); i += 2; continue; }
        if (f[i] == '}') die("@fmt: stray brace");
        if (f[i] == '{') {
            size_t j = i + 1; char *name; Value *x; char *s;
            if (!((f[j] >= 'a' && f[j] <= 'z') || (f[j] >= 'A' && f[j] <= 'Z') || f[j] == '_')) die("@fmt: stray brace");
            while (seed_word(f[j])) j++;
            name = copy_n(f + i + 1, j - i - 1); x = value_get(facts, name);
            if (!x) { fprintf(stderr, "@fmt key: %s\n", name); die("@fmt: unknown fact"); }
            free(name);
            while (f[j] == '[') {
                size_t k = j + 1, e; char *key; int digits = 1;
                while (f[k] && f[k] != ']' && f[k] != '[' && f[k] != '{' && f[k] != '}') k++;
                if (f[k] != ']' || k == j + 1) die("@fmt: stray brace");
                key = copy_n(f + j + 1, k - j - 1);
                for (e = 0; key[e]; e++) if (key[e] < '0' || key[e] > '9') digits = 0;
                if (digits) {
                    unsigned long ix = strtoul(key, 0, 10);
                    if (!x || x->kind != JARR || ix >= x->n) die("@fmt: index out of range");
                    x = x->items[ix].value;
                } else {
                    x = value_get(x, key);
                    if (!x) die("@fmt: unknown key");
                }
                free(key); j = k + 1;
            }
            if (f[j] != '}') die("@fmt: stray brace");
            s = seed_str(x); buf_add(&b, s, strlen(s)); free(s);
            i = j + 1; continue;
        }
        buf_char(&b, f[i]); i++;
    }
    return b.s;
}

/* facts.load.typed_cell('str', ...): \t \n and \X -> X */
static char *seed_unescape(const char *t) {
    Buffer b = {0}; size_t i;
    buf_add(&b, "", 0);
    for (i = 0; t[i]; i++) {
        if (t[i] == '\\' && t[i + 1]) { i++; buf_char(&b, t[i] == 't' ? '\t' : t[i] == 'n' ? '\n' : t[i]); }
        else buf_char(&b, t[i]);
    }
    return b.s;
}

/* Run.value for the data forms: @str, @fmt, @ref, $$DOMAIN:KEY, $NAME and plain fact paths.
 * Graph forms (@rej @bytes @out @seqmap @acts @textf @text fresh:) belong to the dispatcher. */
static Value *seed_eval(const char *cell, Value *facts, Value *env) {
    char *v = seed_interp(cell, env), *arg = strchr(v, ':'); Value *out = 0;
    size_t klen = arg ? (size_t)(arg - v) : strlen(v);
    if (arg) arg++;
    if (klen == 4 && !strncmp(v, "@str", 4)) {
        Buffer b = {0}; size_t i = 0; char *u;
        buf_add(&b, "", 0);
        while (arg[i]) {
            if (arg[i] == '{') {
                size_t j = i + 1;
                while (seed_word(arg[j])) j++;
                if (j > i + 1 && arg[j] == '}') {
                    char *name = copy_n(arg + i + 1, j - i - 1), *s; Value *x = value_get(facts, name);
                    if (!x) { fprintf(stderr, "@str key: %s\n", name); die("@str: unknown fact"); }
                    s = seed_str(x); buf_add(&b, s, strlen(s)); free(s); free(name); i = j + 1; continue;
                }
            }
            buf_char(&b, arg[i]); i++;
        }
        u = seed_unescape(b.s); free(b.s); out = value_string(u); free(u);
    } else if (klen == 4 && !strncmp(v, "@fmt", 4)) {
        char *s = seed_fmt(arg, facts); out = value_string(s); free(s);
    } else if (klen == 4 && !strncmp(v, "@ref", 4)) {
        char *s = seed_fmt(arg, facts); out = value_path(facts, s); free(s);
    } else if (v[0] == '@' || (klen == 5 && !strncmp(v, "fresh", 5))) {
        fprintf(stderr, "cell: %s\n", v); die("graph value form is not a data cell");
    } else if (v[0] == '$' && v[1] == '$') {
        char *colon = strchr(v + 2, ':'), *domain, *key; Value *d; int braced = 0; size_t i;
        domain = colon ? copy_n(v + 2, (size_t)(colon - v - 2)) : copy(v + 2);
        key = copy(colon ? colon + 1 : "");
        for (i = 0; key[i]; i++) if (key[i] == '{' && (seed_word(key[i + 1]))) { size_t j = i + 1; while (seed_word(key[j]) || key[j] == '[' || key[j] == ']') j++; if (key[j] == '}') braced = 1; }
        if (braced) { char *k2 = seed_fmt(key, facts); free(key); key = k2; }
        d = value_get(env, domain);
        if (!d) { fprintf(stderr, "env domain: %s\n", domain); die("$$: unknown environment domain"); }
        out = value_get(d, key);
        if (!out) { fprintf(stderr, "env key: %s:%s\n", domain, key); die("$$: unknown key"); }
        free(domain); free(key);
    } else if (v[0] == '$') {
        out = value_get(env, v + 1);
        if (!out) { fprintf(stderr, "env name: %s\n", v + 1); die("$: unknown environment name"); }
    } else out = value_path(facts, v);
    free(v); return out;
}

/* Run.cells: "a=CELL,b=CELL" -> ordered object; "" and "-" -> NULL */
static Value *seed_bind_cells(const char *cells, Value *facts, Value *env) {
    Value *out; char *all, *p;
    if (!cells || !*cells || !strcmp(cells, "-")) return 0;
    out = value_new(JOBJ); all = copy(cells); p = all;
    for (;;) {
        char *comma = strchr(p, ','), *eq;
        if (comma) *comma = 0;
        eq = strchr(p, '=');
        if (!eq) die("binding cell without '='");   /* partition('=') would bind k='' -> path '' */
        *eq = 0;
        value_put(out, p, seed_eval(eq + 1, facts, env));
        if (!comma) break;
        p = comma + 1;
    }
    free(all); return out;
}
#endif
