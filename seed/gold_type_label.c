/* seed/gold_type_label.c -- the type net's labels in rule form, C99.
 *
 *   gold-type-label table          one line "T1<TAB>OP<TAB>T2<TAB>LABEL" for every triple of
 *                                  TYS x TOPS x TYS (15 x 19 x 15 = 4275 rows), in table order
 *   gold-type-label call T1 OP T2  one line "LABEL" for that triple
 *
 * type_label(t1, op, t2) and its helpers (_promote, _uns, _arith, _farith, RANK, NUM, FLT).  Unknown type or
 * operator names are not refusals: type_label answers "illegal" for them (the fall-through of the function).
 * A wrong argument count is refused at the command line.
 * Second derivation of the frozen authority weights/gold/type.tsv (gate gold-label-type, tests/goldlabelcheck.cx):
 * `table` reproduces its key rows byte for byte, in the net's key order; the rule form here and the table form
 * there are written independently.  Ported 0.0.41 (K6-21) from gold.py of the Python seed, which is not read.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *TYS[] = {"void", "i8", "i16", "i32", "i64", "u8", "u16", "u32", "u64", "ptr", "arr", "struct", "fn", "f32", "f64"};
#define NTYS ((int)(sizeof TYS / sizeof TYS[0]))
static const char *TOPS[] = {"+", "-", "*", "/", "%", "<", "==", "=", "&", "[]", ".", "call", "sizeof", ",", "un*", "|", "^", "<<", ">>"};
#define NTOPS ((int)(sizeof TOPS / sizeof TOPS[0]))
static const char *NUM[] = {"i8", "i16", "i32", "i64", "u8", "u16", "u32", "u64"};
#define NNUM ((int)(sizeof NUM / sizeof NUM[0]))
static const char *FLT[] = {"f32", "f64"};
#define NFLT ((int)(sizeof FLT / sizeof FLT[0]))

/* RANK of gold.py: rank of an integer type, 9 for anything else (dict.get default) */
static int rank(const char *t) {
    static const struct { const char *t; int r; } R[] = {{"i8", 1}, {"u8", 1}, {"i16", 2}, {"u16", 2}, {"i32", 3}, {"u32", 3}, {"i64", 4}, {"u64", 4}};
    size_t i;
    for (i = 0; i < sizeof R / sizeof R[0]; i++) if (!strcmp(R[i].t, t)) return R[i].r;
    return 9;
}
static int in_num(const char *t) { int i; for (i = 0; i < NNUM; i++) if (!strcmp(NUM[i], t)) return 1; return 0; }
static int in_flt(const char *t) { int i; for (i = 0; i < NFLT; i++) if (!strcmp(FLT[i], t)) return 1; return 0; }
static int in_numflt(const char *t) { return in_num(t) || in_flt(t); }

/* _promote(t): anything of lower rank than int becomes "i32"; otherwise t */
static const char *promote(const char *t) { return rank(t) < 3 ? "i32" : t; }

/* _uns(t1, t2): is the result of the usual arithmetic conversions unsigned */
static int uns(const char *t1, const char *t2) {
    const char *a = promote(t1), *b = promote(t2);
    int ua = a[0] == 'u', ub = b[0] == 'u';
    const char *u, *sg;
    if (!(ua || ub)) return 0;
    if (ua && ub) return 1;
    u = ua ? a : b; sg = ua ? b : a;
    return rank(u) >= rank(sg);
}

/* _arith(t1, t2): the wider operand's rank, signed or unsigned */
static const char *arith(const char *t1, const char *t2) {
    int r = rank(promote(t1)) > rank(promote(t2)) ? rank(promote(t1)) : rank(promote(t2));
    if (uns(t1, t2)) return r >= 4 ? "u64" : "u32";
    return r >= 4 ? "i64" : "i32";
}

/* _farith(t1, t2): the wider floating type */
static const char *farith(const char *t1, const char *t2) { return (!strcmp(t1, "f64") || !strcmp(t2, "f64")) ? "f64" : "f32"; }

/* type_label(t1, op, t2): the branch order of gold.py */
static const char *type_label(const char *t1, const char *op, const char *t2) {
    if (!strcmp(op, "sizeof")) return "i64";
    if (!strcmp(op, "un*")) return !strcmp(t1, "ptr") ? "i64" : "illegal";
    if (!strcmp(op, "call")) return !strcmp(t1, "fn") ? "i64" : "illegal";
    if (!strcmp(op, "[]")) return (!strcmp(t1, "ptr") || !strcmp(t1, "arr")) ? "i64" : "illegal";
    if (!strcmp(op, ".")) return !strcmp(t1, "struct") ? "i64" : "illegal";
    if (!strcmp(op, "=")) return t1;
    if (!strcmp(op, "&")) {
        if (in_num(t1) && in_num(t2) && !(!strcmp(t1, "i64") && !strcmp(t2, "i64"))) return arith(t1, t2);
        return (!strcmp(t1, "i64") && !strcmp(t2, "i64")) ? "ptr" : "illegal";
    }
    if (!strcmp(op, "<<") || !strcmp(op, ">>")) return (in_num(t1) && in_num(t2)) ? promote(t1) : "illegal";
    if (!strcmp(op, "|") || !strcmp(op, "^")) return (in_num(t1) && in_num(t2)) ? arith(t1, t2) : "illegal";
    if (!strcmp(op, ",")) return t2;
    if (!strcmp(op, "<") || !strcmp(op, "==")) {
        if ((in_flt(t1) || in_flt(t2)) && !(in_numflt(t1) && in_numflt(t2))) return "illegal";
        return "i32";
    }
    if ((!strcmp(op, "+") || !strcmp(op, "-") || !strcmp(op, "*") || !strcmp(op, "/")) && (in_flt(t1) || in_flt(t2))) {
        if (in_numflt(t1) && in_numflt(t2)) return farith(t1, t2);
        return "illegal";
    }
    if (!strcmp(op, "+") || !strcmp(op, "-") || !strcmp(op, "*") || !strcmp(op, "/") || !strcmp(op, "%")) {
        if ((!strcmp(op, "+") || !strcmp(op, "-")) && (!strcmp(t1, "ptr") || !strcmp(t1, "arr")) && in_num(t2)) return "ptr";
        if (!strcmp(op, "+") && in_num(t1) && (!strcmp(t2, "ptr") || !strcmp(t2, "arr"))) return "ptr";
        if (!strcmp(op, "-") && !strcmp(t1, "ptr") && !strcmp(t2, "ptr")) return "i64";
        if (in_num(t1) && in_num(t2)) return arith(t1, t2);
    }
    return "illegal";
}

int main(int argc, char **argv) {
    int i, j, k;
    if (argc == 2 && !strcmp(argv[1], "table")) {
        for (i = 0; i < NTYS; i++)
            for (j = 0; j < NTOPS; j++)
                for (k = 0; k < NTYS; k++)
                    printf("%s\t%s\t%s\t%s\n", TYS[i], TOPS[j], TYS[k], type_label(TYS[i], TOPS[j], TYS[k]));
        return 0;
    }
    if (argc == 5 && !strcmp(argv[1], "call")) {
        printf("%s\n", type_label(argv[2], argv[3], argv[4]));
        return 0;
    }
    fprintf(stderr, "usage: gold-type-label table | call T1 OP T2\n");
    return 2;
}
