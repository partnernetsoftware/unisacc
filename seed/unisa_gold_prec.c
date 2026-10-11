/* seed/unisa_gold_prec.c -- C99 reference for unisa.gold prec_label (K6-18), finite-domain samples.
 *
 *   unisa-gold-prec table        one line "OP<TAB>LABEL" for every PREC_OPS entry, in PREC_OPS order
 *   unisa-gold-prec op OP        one line "LABEL" for OP (the boundary points are chosen by the script)
 *
 * Mirrors unisa/gold.py prec_label(op) and its tables: PREC_OPS (the 19 domain entries), _PREC (the
 * binding strength 1..10 of each binary operator), and PREC_LEV.  prec_label returns the decimal
 * strength of an operator in _PREC and "none" for anything else.  Refused (python: TypeError / usage):
 * a missing or extra argument, an unknown mode.  Point-sample equivalence over the finite domain and the
 * listed boundary strings only; not the gold stage closure, not build(), not the weights; keeps gold.py.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* PREC_OPS, in order: the finite domain of the stage (gold.py PREC_OPS) */
static const char *PREC_OPS[] = {
    "||", "&&", "|", "^", "&", "==", "!=", "<", ">", "<=", ">=",
    "<<", ">>", "+", "-", "*", "/", "%", "other",
};
#define NOPS ((int)(sizeof PREC_OPS / sizeof PREC_OPS[0]))

/* _PREC of gold.py: operator -> binding strength (1 loosest .. 10 tightest); "other" is absent */
static const struct { const char *op; int lev; } PREC[] = {
    {"||", 1}, {"&&", 2}, {"|", 3}, {"^", 4}, {"&", 5}, {"==", 6}, {"!=", 6},
    {"<", 7}, {">", 7}, {"<=", 7}, {">=", 7}, {"<<", 8}, {">>", 8}, {"+", 9}, {"-", 9},
    {"*", 10}, {"/", 10}, {"%", 10},
};
#define NPREC ((int)(sizeof PREC / sizeof PREC[0]))

/* prec_label(op): str(_PREC[op]) if op in _PREC else "none" */
static const char *prec_label(const char *op, char *buf, size_t cap) {
    int i;
    for (i = 0; i < NPREC; i++) if (!strcmp(PREC[i].op, op)) { snprintf(buf, cap, "%d", PREC[i].lev); return buf; }
    return "none";
}

int main(int argc, char **argv) {
    char buf[16]; int i;
    if (argc == 2 && !strcmp(argv[1], "table")) {
        for (i = 0; i < NOPS; i++) printf("%s\t%s\n", PREC_OPS[i], prec_label(PREC_OPS[i], buf, sizeof buf));
        return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "op")) {
        printf("%s\n", prec_label(argv[2], buf, sizeof buf));
        return 0;
    }
    fprintf(stderr, "usage: unisa-gold-prec table | op OP\n");
    return 2;
}
