/* seed/gold_prec_label.c -- the prec net's labels in rule form, C99.
 *
 *   gold-prec-label table        one line "OP<TAB>LABEL" for every PREC_OPS entry, in PREC_OPS order
 *   gold-prec-label call OP      one line "LABEL" for OP
 *
 * prec_label(op) with its tables: PREC_OPS (the 19 domain entries), _PREC (the binding strength 1..10 of each
 * binary operator).  prec_label returns the decimal strength of an operator in _PREC and "none" for anything
 * else.  Refused (usage, rc 2): a missing or extra argument, an unknown mode.
 * Second derivation of the frozen authority weights/gold/prec.tsv (gate gold-label-prec, tests/goldlabelcheck.cx):
 * `table` reproduces its key rows byte for byte, in the net's key order; the rule form here and the table form
 * there are written independently.  Ported 0.0.41 (K6-18) from gold.py of the Python seed, which is not read.
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
    if (argc == 3 && !strcmp(argv[1], "call")) {
        printf("%s\n", prec_label(argv[2], buf, sizeof buf));
        return 0;
    }
    fprintf(stderr, "usage: gold-prec-label table | call OP\n");
    return 2;
}
