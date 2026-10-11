/* seed/unisa_gold_binsel_label.c -- C99 reference for unisa.gold binsel_label (K6-22), finite-domain samples.
 *
 *   unisa-gold-binsel-label table        "OP<TAB>SIGN<TAB>FLAVOUR" for every BINSEL_OPS x ("s", "u") key
 *                                        (16 x 2 = 32 rows, the stage's own domain), in table order
 *   unisa-gold-binsel-label call OP SIGN one line "FLAVOUR" for that pair
 *
 * Mirrors unisa/gold.py binsel_label(op, sign) with BINSEL_OPS, _BIN_S (signed flavour of each operator) and
 * _BIN_U (the unsigned differences: division, remainder, ordering and >>).  An unsigned sign picks the _BIN_U
 * entry when there is one; every other sign picks the _BIN_S entry.  An operator outside _BIN_S is refused
 * (python: KeyError).  A missing argument is refused at the command line.  Point-sample equivalence over the
 * stage's finite domain and the listed points only; not the gold stage closure; not build(); keeps gold.py.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *BINSEL_OPS[] = {"+", "-", "*", "/", "%", "<", ">", "<=", ">=", "==", "!=", "&", "|", "^", "<<", ">>"};
#define NOPS ((int)(sizeof BINSEL_OPS / sizeof BINSEL_OPS[0]))
static const char *SIGNS[] = {"s", "u"};
#define NSIGNS ((int)(sizeof SIGNS / sizeof SIGNS[0]))

typedef struct { const char *op, *flav; } Pair;
static const Pair BIN_S[] = {
    {"+", "add"}, {"-", "sub"}, {"*", "mul"}, {"/", "div"}, {"%", "mod"}, {"<", "lt"}, {">", "gt"},
    {"<=", "le"}, {">=", "ge"}, {"==", "eq"}, {"!=", "ne"}, {"&", "and"}, {"|", "or"}, {"^", "xor"},
    {"<<", "shl"}, {">>", "shr"},
};
static const Pair BIN_U[] = {
    {"/", "udiv"}, {"%", "umod"}, {"<", "ult"}, {">", "ugt"}, {"<=", "ule"}, {">=", "uge"}, {">>", "lshr"},
};

static const char *lookup(const char *op, const Pair *t, size_t n) {
    size_t i;
    for (i = 0; i < n; i++) if (!strcmp(t[i].op, op)) return t[i].flav;
    return NULL;
}

/* binsel_label(op, sign) */
static const char *binsel_label(const char *op, const char *sign) {
    const char *f;
    if (!strcmp(sign, "u")) {
        f = lookup(op, BIN_U, sizeof BIN_U / sizeof BIN_U[0]);
        if (f) return f;
    }
    f = lookup(op, BIN_S, sizeof BIN_S / sizeof BIN_S[0]);
    if (f) return f;
    fprintf(stderr, "unisa-gold-binsel-label: operator outside _BIN_S (python: KeyError) %s\n", op);
    exit(1);
}

int main(int argc, char **argv) {
    int i, j;
    if (argc == 2 && !strcmp(argv[1], "table")) {
        for (i = 0; i < NOPS; i++)
            for (j = 0; j < NSIGNS; j++)
                printf("%s\t%s\t%s\n", BINSEL_OPS[i], SIGNS[j], binsel_label(BINSEL_OPS[i], SIGNS[j]));
        return 0;
    }
    if (argc == 4 && !strcmp(argv[1], "call")) {
        printf("%s\n", binsel_label(argv[2], argv[3]));
        return 0;
    }
    fprintf(stderr, "usage: unisa-gold-binsel-label table | call OP SIGN\n");
    return 2;
}
