/* seed/unisa_libneed.c -- C99 reference for unisa.libneed (K6-6), point-sample functions.
 *
 *   unisa-libneed consts              PREFIX, ROOTS, MACRO_NAME_LIMIT
 *   unisa-libneed guard NAME          the guard line for a carried body
 *   unisa-libneed bodies FILE         (name, start, end) of each static function body in FILE
 *   unisa-libneed table DIR           the sorted carried-body names and, for each name that needs
 *                                     any, its sorted closure over carried bodies; then the roots
 *                                     closure.  DIR is walked recursively for *.h (scratch only).
 *
 * Mirrors unisa/libneed.py: _headers, bodies, _skip, _name (first non-keyword identifier followed by
 * '('), DEFINE (#define NAME rest), table (closure over bodies and macros, skipping _UNISA_/__UNISA_
 * macro names), roots, guard.  Refusals (both sides) by name, nothing printed for the refused command:
 * duplicate carried body name, need-macro name too long, more than 1024 bodies.
 *
 * Point-sample equivalence over the listed fixtures only; not a product closure member; keeps
 * unisa/libneed.py; does not touch the product include/ tree.
 */
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define PREFIX "__UN_"
#define MACRO_NAME_LIMIT 63
static const char *ROOTS_LIST[] = { "exit", "_unisa_environ_init" };
#define NROOTS 2

static void die(const char *why, const char *what) {
    fprintf(stderr, "unisa-libneed: %s %s\n", why, what ? what : "");
    exit(1);
}

static const char *KEYWORDS[] = { "void", "char", "short", "int", "long", "float", "double", "signed",
    "unsigned", "const", "volatile", "static", "inline", "struct", "union", "enum", "_Bool", "__attribute__", NULL };

static int is_keyword(const char *s, size_t n) {
    int i;
    for (i = 0; KEYWORDS[i]; i++) if (strlen(KEYWORDS[i]) == n && !strncmp(KEYWORDS[i], s, n)) return 1;
    return 0;
}
static int ident_start(char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_'; }
static int ident_char(char c) { return ident_start(c) || (c >= '0' && c <= '9'); }

/* ---- growable string list (sorted on demand) ---- */
typedef struct { char **v; size_t n, cap; } SList;
static void sl_add(SList *l, const char *s, size_t n) {
    if (l->n == l->cap) { l->cap = l->cap ? l->cap * 2 : 64; l->v = realloc(l->v, l->cap * sizeof *l->v); if (!l->v) die("out of memory", NULL); }
    l->v[l->n] = malloc(n + 1); if (!l->v[l->n]) die("out of memory", NULL);
    memcpy(l->v[l->n], s, n); l->v[l->n][n] = 0; l->n++;
}
static int cmp_str(const void *a, const void *b) { return strcmp(*(char *const *)a, *(char *const *)b); }
static void sl_sort_uniq(SList *l) {
    size_t i, k = 0;
    if (!l->n) return;
    qsort(l->v, l->n, sizeof *l->v, cmp_str);
    for (i = 1; i < l->n; i++) if (strcmp(l->v[i], l->v[k])) l->v[++k] = l->v[i]; else free(l->v[i]);
    l->n = k + 1;
}

/* ---- body scanning (python bodies(): HEAD regex + _name + _skip brace matcher) ---- */
typedef struct { char *name; size_t a, b; } Body;

static size_t skip_lit(const char *t, size_t n, size_t i) {
    char c = t[i];
    if (c == '"' || c == '\'') {
        size_t j = i + 1;
        while (j < n && t[j] != c) j += (t[j] == '\\') ? 2 : 1;
        return j + 1;
    }
    if (i + 1 < n && t[i] == '/' && t[i+1] == '*') {
        size_t j = i + 2;
        while (j + 1 < n && !(t[j] == '*' && t[j+1] == '/')) j++;
        if (j + 1 >= n) die("unterminated comment", NULL);
        return j + 2;
    }
    if (i + 1 < n && t[i] == '/' && t[i+1] == '/') {
        size_t j = i; while (j < n && t[j] != '\n') j++;
        if (j >= n) die("unterminated line comment", NULL);
        return j;
    }
    return i;
}

/* name of a declarator head: first identifier followed by '(' that is not a keyword */
static char *head_name(const char *h, size_t hn) {
    size_t i = 0;
    while (i < hn) {
        if (ident_start(h[i])) {
            size_t s = i, k, e;
            while (i < hn && ident_char(h[i])) i++;
            e = i; k = e;
            while (k < hn && (h[k] == ' ' || h[k] == '\t')) k++;
            if (k < hn && h[k] == '(' && !is_keyword(h + s, e - s)) {
                char *r = malloc(e - s + 1); if (!r) die("out of memory", NULL);
                memcpy(r, h + s, e - s); r[e - s] = 0; return r;
            }
        } else i++;
    }
    return NULL;
}

/* Match HEAD's one optional leading __attribute__(([^)]*)) group, then
 * whitespace and the original static word boundary. This deliberately does
 * not extend HEAD to nested attribute arguments or repeated prefixes.
 * Keep the original offset for Body.a/refs, but start name lookup at static:
 * attribute identifiers are metadata, not a function declarator. */
static int body_head(const char *t, size_t n, size_t i, size_t *declarator) {
    static const char prefix[] = "__attribute__((";
    size_t p = i, k = sizeof prefix - 1;
    if (n - p >= k && !memcmp(t + p, prefix, k)) {
        p += k;
        while (p < n && t[p] != ')') p++;
        if (n - p < 2 || t[p] != ')' || t[p + 1] != ')') return 0;
        p += 2;
        while (p < n && (t[p] == ' ' || t[p] == '\t' || t[p] == '\n' ||
                        t[p] == '\r' || t[p] == '\f' || t[p] == '\v')) p++;
    }
    if (n - p < 6 || memcmp(t + p, "static", 6) ||
        (p + 6 < n && ident_char(t[p + 6]))) return 0;
    *declarator = p;
    return 1;
}

static void scan_bodies(const char *t, size_t n, Body **out, size_t *nout, size_t *cap) {
    size_t i = 0;
    while (i < n) {
        int at_line_start = (i == 0 || t[i-1] == '\n');
        size_t declarator;
        if (at_line_start && body_head(t, n, i, &declarator)) {
            size_t j = declarator, hend;
            while (j < n && t[j] != '{' && t[j] != ';') j++;
            if (j < n && t[j] == '{') {
                char *name = head_name(t + declarator, j - declarator);
                if (name) {
                    size_t k = j, depth = 0;
                    for (;;) {
                        size_t m = skip_lit(t, n, k);
                        if (m != k) { k = m; if (k >= n) break; continue; }
                        if (t[k] == '{') depth++;
                        else if (t[k] == '}') { depth--; if (depth == 0) break; }
                        k++;
                        if (k >= n) die("unbalanced body", name);
                    }
                    if (*nout == *cap) { *cap = *cap ? *cap * 2 : 32; *out = realloc(*out, *cap * sizeof **out); if (!*out) die("out of memory", NULL); }
                    (*out)[*nout].name = name; (*out)[*nout].a = i; (*out)[*nout].b = k + 1; (*nout)++;
                    i = k + 1; continue;
                }
                hend = j; (void)hend;
            }
        }
        i++;
    }
}

static char *read_all(const char *path, size_t *n) {
    FILE *f = fopen(path, "rb"); char *b; size_t cap = 1 << 16, len = 0;
    if (!f) die("cannot open", path);
    b = malloc(cap); if (!b) die("out of memory", NULL);
    for (;;) {
        size_t r = fread(b + len, 1, cap - len, f); len += r;
        if (len == cap) { cap *= 2; b = realloc(b, cap); if (!b) die("out of memory", NULL); continue; }
        if (!r) break;
    }
    fclose(f); *n = len; return b;
}

/* ---- header walk: sorted relative *.h paths under DIR ---- */
static void walk(const char *dir, const char *rel, SList *hs) {
    DIR *d = opendir(dir); struct dirent *e;
    if (!d) return;
    while ((e = readdir(d))) {
        char full[4096], sub[4096];
        size_t nl = strlen(e->d_name);
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        /* cx-flat.h is generated from the other headers (tools/cx-flatten.cx), not carried source */
        if (!rel[0] && !strcmp(e->d_name, "cx-flat.h")) continue;
        snprintf(full, sizeof full, "%s/%s", dir, e->d_name);
        snprintf(sub, sizeof sub, rel[0] ? "%s/%s" : "%s%s", rel, e->d_name);
        {
            struct stat st; if (stat(full, &st)) continue;
            if (S_ISDIR(st.st_mode)) walk(full, sub, hs);
            else if (nl > 2 && !strcmp(e->d_name + nl - 2, ".h")) sl_add(hs, sub, strlen(sub));
        }
    }
    closedir(d);
}

/* ---- table: names of bodies, closure over bodies and macros ---- */
typedef struct { char *name; int body; SList refs; } Node;
static Node *nodes; static size_t nn, ncap;
static Node *find_node(const char *s) {
    size_t i; for (i = 0; i < nn; i++) if (!strcmp(nodes[i].name, s)) return &nodes[i];
    return NULL;
}
static Node *add_node(const char *s, int body) {
    Node *x;
    if (nn == ncap) { ncap = ncap ? ncap * 2 : 64; nodes = realloc(nodes, ncap * sizeof *nodes); if (!nodes) die("out of memory", NULL); }
    x = &nodes[nn++];
    x->name = malloc(strlen(s) + 1); if (!x->name) die("out of memory", NULL);
    strcpy(x->name, s); x->body = body; memset(&x->refs, 0, sizeof x->refs);
    return x;
}
static void idents_into(const char *t, size_t a, size_t b, SList *refs) {
    size_t i = a;
    while (i < b) {
        if (ident_start(t[i])) { size_t s = i; while (i < b && ident_char(t[i])) i++; sl_add(refs, t + s, i - s); }
        else i++;
    }
}

static int cmp_nodes(const void *a, const void *b) { return strcmp(((const Node *)a)->name, ((const Node *)b)->name); }

static void do_table(const char *dir) {
    SList hs = {0,0,0}; size_t i, j;
    walk(dir, "", &hs);
    sl_sort_uniq(&hs);
    for (i = 0; i < hs.n; i++) {
        char full[4096]; size_t n; char *text;
        snprintf(full, sizeof full, "%s/%s", dir, hs.v[i]);
        text = read_all(full, &n);
        {
            Body *bs = NULL; size_t nb = 0, cap = 0;
            scan_bodies(text, n, &bs, &nb, &cap);
            for (j = 0; j < nb; j++) {
                Node *x;
                if (find_node(bs[j].name)) die("carried body defined twice", bs[j].name);
                x = add_node(bs[j].name, 1);
                idents_into(text, bs[j].a, bs[j].b, &x->refs);
                free(bs[j].name);
            }
            free(bs);
        }
        /* #define NAME rest (line form) */
        {
            size_t p = 0;
            while (p < n) {
                size_t ls = p, le = p;
                while (le < n && text[le] != '\n') le++;
                if (le - ls >= 8 && !strncmp(text + ls, "#", 1)) {
                    size_t q = ls + 1;
                    while (q < le && (text[q] == ' ' || text[q] == '\t')) q++;
                    if (q + 6 <= le && !strncmp(text + q, "define", 6) && q + 6 < le && (text[q+6] == ' ' || text[q+6] == '\t')) {
                        size_t r = q + 6;
                        while (r < le && (text[r] == ' ' || text[r] == '\t')) r++;
                        if (r < le && ident_start(text[r])) {
                            size_t s = r; char *nm;
                            while (r < le && ident_char(text[r])) r++;
                            nm = malloc(r - s + 1); if (!nm) die("out of memory", NULL);
                            memcpy(nm, text + s, r - s); nm[r - s] = 0;
                            if (strncmp(nm, "_UNISA_", 7) && strncmp(nm, "__UNISA_", 8) && !find_node(nm)) {
                                Node *x = add_node(nm, 0);
                                idents_into(text, r, le, &x->refs);
                            }
                            free(nm);
                        }
                    }
                }
                p = le + 1;
            }
        }
        free(text);
    }
    /* body and macro names are unique: sort the node table for stable lookups */
    qsort(nodes, nn, sizeof *nodes, cmp_nodes);
    /* python table(): every carried body name must fit the macro name limit (asserted over all bodies) */
    {
        size_t k;
        for (k = 0; k < nn; k++)
            if (nodes[k].body && strlen(PREFIX) + strlen(nodes[k].name) > MACRO_NAME_LIMIT) die("need macro too long", nodes[k].name);
    }
    /* names list: carried bodies, sorted */
    {
        size_t k; int first = 1;
        printf("NAMES");
        for (k = 0; k < nn; k++) if (nodes[k].body) { printf("%s%s", first ? " " : " ", nodes[k].name); first = 0; }
        printf("\n");
    }
    /* enforce the python assertions on the carried-body set */
    {
        size_t k, nbod = 0;
        for (k = 0; k < nn; k++) if (nodes[k].body) nbod++;
        if (nbod > 1024) die("src/front_pp.c ftrim_libc_mark holds 1024 bodies", NULL);
    }
    /* closure: for every node, DFS over refs that are nodes, skipping self; keep carried bodies */
    {
        size_t k;
        SList roots_union = {0,0,0};
        for (k = 0; k < nn; k++) {
            SList stack = {0,0,0}, seen = {0,0,0};
            size_t s, need_n = 0; char **need = NULL;
            sl_add(&stack, nodes[k].name, strlen(nodes[k].name));
            while (stack.n) {
                char *x = stack.v[--stack.n];
                int dup = 0; size_t q;
                for (q = 0; q < seen.n; q++) if (!strcmp(seen.v[q], x)) { dup = 1; break; }
                if (dup) { free(x); continue; }
                sl_add(&seen, x, strlen(x));
                {
                    Node *nx = find_node(x);
                    if (nx) for (q = 0; q < nx->refs.n; q++) {
                        char *t = nx->refs.v[q];
                        if (find_node(t) && strcmp(t, x)) sl_add(&stack, t, strlen(t));
                    }
                }
                free(x);
            }
            for (s = 0; s < seen.n; s++) { Node *nx = find_node(seen.v[s]); if (nx && nx->body) { need_n++; need = realloc(need, need_n * sizeof *need); need[need_n-1] = seen.v[s]; } }
            if (need_n) {
                size_t m; int ok = 1;
                qsort(need, need_n, sizeof *need, cmp_str);
                if (nodes[k].body && strlen(nodes[k].name) + strlen(PREFIX) > MACRO_NAME_LIMIT) ok = 0;
                for (m = 0; m < need_n; m++) if (strlen(need[m]) + strlen(PREFIX) > MACRO_NAME_LIMIT) ok = 0;
                if (!ok) die("need macro too long", nodes[k].name);
                printf("KEY %s", nodes[k].name);
                for (m = 0; m < need_n; m++) printf(" %s", need[m]);
                printf("\n");
            }
            for (s = 0; s < seen.n; s++) free(seen.v[s]);
            free(seen.v); free(stack.v); free(need);
        }
        /* roots: closure of the ROOTS names over carried bodies */
        for (k = 0; k < NROOTS; k++) {
            Node *r = find_node(ROOTS_LIST[k]);
            if (!r) continue;
            (void)r;
        }
        printf("ROOTS_CLOSURE");
        {
            SList rs = {0,0,0}; size_t q;
            for (k = 0; k < NROOTS; k++) {
                SList stack = {0,0,0}, seen = {0,0,0}; size_t s;
                sl_add(&stack, ROOTS_LIST[k], strlen(ROOTS_LIST[k]));
                while (stack.n) {
                    char *x = stack.v[--stack.n]; int dup = 0;
                    for (q = 0; q < seen.n; q++) if (!strcmp(seen.v[q], x)) { dup = 1; break; }
                    if (dup) { free(x); continue; }
                    sl_add(&seen, x, strlen(x));
                    { Node *nx = find_node(x); if (nx) for (q = 0; q < nx->refs.n; q++) { char *t = nx->refs.v[q]; if (find_node(t) && strcmp(t, x)) sl_add(&stack, t, strlen(t)); } }
                    free(x);
                }
                for (s = 0; s < seen.n; s++) { Node *nx = find_node(seen.v[s]); if (nx && nx->body) sl_add(&rs, seen.v[s], strlen(seen.v[s])); }
                free(seen.v); free(stack.v);
            }
            sl_sort_uniq(&rs);
            for (q = 0; q < rs.n; q++) printf(" %s", rs.v[q]);
            free(rs.v);
        }
        printf("\n");
        free(roots_union.v);
    }
    for (i = 0; i < nn; i++) { free(nodes[i].name); free(nodes[i].refs.v); }
    free(nodes); nodes = NULL; nn = ncap = 0;
    for (i = 0; i < hs.n; i++) free(hs.v[i]);
    free(hs.v);
}

static void do_bodies(const char *file) {
    size_t n; char *text = read_all(file, &n);
    Body *bs = NULL; size_t nb = 0, cap = 0, i;
    scan_bodies(text, n, &bs, &nb, &cap);
    for (i = 0; i < nb; i++) {
        printf("BODY %s %zu %zu\n", bs[i].name, bs[i].a, bs[i].b);
        free(bs[i].name);
    }
    free(bs); free(text);
}

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: unisa-libneed consts|guard NAME|bodies FILE|table DIR\n"); return 2; }
    if (!strcmp(argv[1], "consts") && argc == 2) {
        size_t k; printf("PREFIX %s\nLIMIT %d\nROOTS", PREFIX, MACRO_NAME_LIMIT);
        for (k = 0; k < NROOTS; k++) printf(" %s", ROOTS_LIST[k]);
        printf("\n"); return 0;
    }
    if (!strcmp(argv[1], "guard") && argc == 3) {
        if (strlen(argv[2]) + strlen(PREFIX) > MACRO_NAME_LIMIT) die("need macro too long", argv[2]);
        printf("#if !__UNISA_FTRIM_LIBC || %s%s\n", PREFIX, argv[2]); return 0;
    }
    if (!strcmp(argv[1], "bodies") && argc == 3) { do_bodies(argv[2]); return 0; }
    if (!strcmp(argv[1], "table") && argc == 3) { do_table(argv[2]); return 0; }
    fprintf(stderr, "usage: unisa-libneed consts|guard NAME|bodies FILE|table DIR\n");
    return 2;
}
