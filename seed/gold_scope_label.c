/* seed/gold_scope_label.c -- the scope net's labels in rule form, C99.
 *
 *   gold-scope-label table          "CTX<TAB>KIND<TAB>LABEL" for every CTX x KIND key (6 x 5 = 30 rows)
 *   gold-scope-label call CTX KIND  one line "LABEL" for that key
 *
 * scope_label(ctx, kind) with CTX, KIND and TYPEKW.  Per context: top binds a global identifier, names a type
 * keyword or typedef, and names a function on an lparen; param binds an identifier; local binds an identifier and
 * names a type; sizeof names a type; field names a field on an identifier.  Every other combination answers
 * "lookup".  There is no refusal inside the function.
 * Second derivation of the frozen authority weights/gold/scope.tsv (gate gold-label-scope, tests/goldlabelcheck.cx):
 * `table` reproduces its key rows byte for byte, in the net's key order; the rule form here and the table form
 * there are written independently.  Ported 0.0.41 (K6-25) from gold.py of the Python seed, which is not read.
 */
#include <stdio.h>
#include <string.h>

static const char *CTX[] = {"top", "param", "local", "expr", "sizeof", "field"};
#define NCTX ((int)(sizeof CTX / sizeof CTX[0]))
static const char *KIND[] = {"type_kw", "id", "typedef_id", "star", "lparen"};
#define NKIND ((int)(sizeof KIND / sizeof KIND[0]))
static const char *TYPEKW[] = {"type_kw", "typedef_id"};

static int eq(const char *x, const char *y) { return !strcmp(x, y); }
static int is_typekw(const char *k) { return eq(k, TYPEKW[0]) || eq(k, TYPEKW[1]); }

/* scope_label(ctx, kind): the per-context rules of gold.py; anything unmatched is "lookup" */
static const char *scope_label(const char *ctx, const char *kind) {
    if (eq(ctx, "top")) {
        if (is_typekw(kind)) return "type_name";
        if (eq(kind, "id")) return "bind_global";
        if (eq(kind, "lparen")) return "fn_name";
    } else if (eq(ctx, "param")) {
        if (eq(kind, "id")) return "bind_param";
    } else if (eq(ctx, "local")) {
        if (eq(kind, "id")) return "bind_local";
        if (is_typekw(kind)) return "type_name";
    } else if (eq(ctx, "sizeof")) {
        if (is_typekw(kind)) return "type_name";
    } else if (eq(ctx, "field")) {
        if (eq(kind, "id")) return "field";
    }
    return "lookup";
}

int main(int argc, char **argv) {
    int i, j;
    if (argc == 2 && !strcmp(argv[1], "table")) {
        for (i = 0; i < NCTX; i++)
            for (j = 0; j < NKIND; j++)
                printf("%s\t%s\t%s\n", CTX[i], KIND[j], scope_label(CTX[i], KIND[j]));
        return 0;
    }
    if (argc == 4 && !strcmp(argv[1], "call")) {
        printf("%s\n", scope_label(argv[2], argv[3]));
        return 0;
    }
    fprintf(stderr, "usage: gold-scope-label table | call CTX KIND\n");
    return 2;
}
