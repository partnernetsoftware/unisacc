/* seed/gold_irsel_label.c -- the irsel net's labels in rule form, C99.
 *
 *   gold-irsel-label table          "FAMILY<TAB>FLAVOUR<TAB>RECIPE" for every FAMILIES x flavour key (6 x 70 = 420
 *                                   rows, the net's own domain: the flavours in IRSEL_MAP order, each asked of
 *                                   every family; a pair absent from the mapping answers "bad")
 *   gold-irsel-label call FAM FLAV  one line "RECIPE" for that pair
 *
 * irsel_label(fam, flav) = IRSEL_MAP[fam].get(flav, "bad").  An unknown family is a refusal (rc 1, KeyError in the
 * rule).  A known family with an unknown flavour, including a flavour that belongs to another family, answers
 * "bad".  The 70 mapping rows are embedded below.
 * Second derivation of the frozen authority weights/gold/irsel.tsv (gate gold-label-irsel, tests/goldlabelcheck.cx):
 * `table` reproduces its key rows byte for byte, in the net's key order; the rule form here and the table form
 * there are written independently.  Ported 0.0.41 (K6-28) from gold.py of the Python seed, which is not read.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { const char *fam, *flav, *recipe; } Row;
static const Row IRSEL[] = {
    {"alu", "add", "add64"},
    {"alu", "sub", "sub64"},
    {"alu", "mul", "mul64"},
    {"alu", "lt", "slt64"},
    {"alu", "le", "sle64"},
    {"alu", "gt", "slt64_rev"},
    {"alu", "ge", "sle64_rev"},
    {"alu", "eq", "eq"},
    {"alu", "ne", "ne"},
    {"alu", "neg", "sub64"},
    {"alu", "and", "and64"},
    {"alu", "or", "or64"},
    {"alu", "xor", "xor64"},
    {"alu", "shl", "shl64"},
    {"alu", "shr", "shr64"},
    {"alu", "ult", "ult64"},
    {"alu", "ule", "ule64"},
    {"alu", "ugt", "ult64_rev"},
    {"alu", "uge", "ule64_rev"},
    {"alu", "lshr", "lshr64"},
    {"alu", "div", "div"},
    {"alu", "mod", "mod"},
    {"alu", "udiv", "udiv"},
    {"alu", "umod", "umod"},
    {"mem", "load", "load64"},
    {"mem", "store", "store64"},
    {"mem", "lea", "lea"},
    {"mem", "ld", "ld"},
    {"mem", "st", "st"},
    {"mem", "zero", "zero"},
    {"ctrl", "jump", "jump"},
    {"ctrl", "jumpz", "jumpz"},
    {"ctrl", "ret", "ret"},
    {"call", "call", "call"},
    {"call", "push", "callpush"},
    {"call", "arg", "arg"},
    {"call", "frame", "frame"},
    {"call", "callr", "callr"},
    {"lit", "imm", "imm"},
    {"lit", "print", "print"},
    {"lit", "write", "write"},
    {"lit", "exit", "exit"},
    {"fpu", "dadd", "fadd64"},
    {"fpu", "dsub", "fsub64"},
    {"fpu", "dmul", "fmul64"},
    {"fpu", "ddiv", "fdiv64"},
    {"fpu", "dlt", "flt64"},
    {"fpu", "dle", "fle64"},
    {"fpu", "deq", "feq64"},
    {"fpu", "sadd", "fadd32"},
    {"fpu", "ssub", "fsub32"},
    {"fpu", "smul", "fmul32"},
    {"fpu", "sdiv", "fdiv32"},
    {"fpu", "slt", "flt32"},
    {"fpu", "sle", "fle32"},
    {"fpu", "seq", "feq32"},
    {"fpu", "i2d", "cvtid"},
    {"fpu", "u2d", "cvtud"},
    {"fpu", "i2s", "cvtis"},
    {"fpu", "u2s", "cvtus"},
    {"fpu", "d2i", "cvtdi"},
    {"fpu", "d2u", "cvtdu"},
    {"fpu", "s2d", "cvtsd"},
    {"fpu", "d2s", "cvtds"},
    {"fpu", "dsqrt", "fsqrt64"},
    {"fpu", "ssqrt", "fsqrt32"},
    {"fpu", "dgt", "flt64_rev"},
    {"fpu", "dge", "fle64_rev"},
    {"fpu", "sgt", "flt32_rev"},
    {"fpu", "sge", "fle32_rev"},
};
#define NROWS ((int)(sizeof IRSEL / sizeof IRSEL[0]))

static const char *FAMILIES[] = {"alu", "mem", "ctrl", "call", "lit", "fpu"};
#define NFAM ((int)(sizeof FAMILIES / sizeof FAMILIES[0]))

static int known_family(const char *f) { int i; for (i = 0; i < NFAM; i++) if (!strcmp(FAMILIES[i], f)) return 1; return 0; }

/* irsel_label(fam, flav) = IRSEL_MAP[fam].get(flav, "bad") */
static const char *irsel_label(const char *fam, const char *flav) {
    int i;
    if (!known_family(fam)) {
        fprintf(stderr, "gold-irsel-label: unknown family %s\n", fam);
        exit(1);
    }
    for (i = 0; i < NROWS; i++)
        if (!strcmp(IRSEL[i].fam, fam) && !strcmp(IRSEL[i].flav, flav)) return IRSEL[i].recipe;
    return "bad";
}

/* a flavour first seen at row I (the flavour domain in mapping order) */
static int first_flavour(int i) { int k; for (k = 0; k < i; k++) if (!strcmp(IRSEL[k].flav, IRSEL[i].flav)) return 0; return 1; }

int main(int argc, char **argv) {
    int i, f;
    if (argc == 2 && !strcmp(argv[1], "table")) {
        for (f = 0; f < NFAM; f++)
            for (i = 0; i < NROWS; i++)
                if (first_flavour(i)) printf("%s\t%s\t%s\n", FAMILIES[f], IRSEL[i].flav, irsel_label(FAMILIES[f], IRSEL[i].flav));
        return 0;
    }
    if (argc == 4 && !strcmp(argv[1], "call")) {
        printf("%s\n", irsel_label(argv[2], argv[3]));
        return 0;
    }
    fprintf(stderr, "usage: gold-irsel-label table | call FAM FLAV\n");
    return 2;
}
