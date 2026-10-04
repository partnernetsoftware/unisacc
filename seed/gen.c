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

static void die(const char *why) { fprintf(stderr, "seed-gen: %s\n", why); exit(1); }
static void *grow(void *p, size_t n, size_t unit) {
    if (n > SIZE_MAX / unit) die("size overflow");
    p = realloc(p, n * unit); if (!p) die("out of memory"); return p;
}
static char *copy(const char *s) {
    size_t n = strlen(s) + 1; char *p = malloc(n);
    if (!p) die("out of memory"); memcpy(p, s, n); return p;
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
    if ((argc != 3 && argc != 4) || strcmp(argv[1], "prune")) die("usage: seed-gen prune OUT.json [RULE_DIR]");
    manifest(&g, argc == 4 ? argv[3] : "exec/prune");
    finish(&g);
    out = fopen(argv[2], "wb"); if (!out) die("cannot open output");
    output(out, &g); if (fclose(out)) die("output close failed");
    fprintf(stderr, "prune states %lu\n", (unsigned long)g.n);
    return 0;
}
