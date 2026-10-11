/* seed/unisa_gold_irsel_label.c -- C99 reference for unisa.gold irsel_label (K6-28), mapping-table samples.
 *
 *   unisa-gold-irsel-label table        "FAMILY<TAB>FLAVOUR<TAB>RECIPE" for every IRSEL_MAP entry (70 rows,
 *                                       the mapping itself, in gold.py's insertion order; not a Cartesian product)
 *   unisa-gold-irsel-label call FAM FLAV  one line "RECIPE" for that pair
 *
 * Mirrors unisa/gold.py irsel_label(fam, flav) = IRSEL_MAP[fam].get(flav, "bad").  An unknown family is a refusal
 * (python: KeyError).  A known family with an unknown flavour, including a flavour that belongs to another family,
 * answers "bad".  The 70 rows are generated from gold.py's IRSEL_MAP literal and embedded here.  Point-sample
 * equivalence over the mapping and the listed boundary pairs only; not the gold stage closure; not build(); keeps
 * gold.py.
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
        fprintf(stderr, "unisa-gold-irsel-label: unknown family (python: KeyError) %s\n", fam);
        exit(1);
    }
    for (i = 0; i < NROWS; i++)
        if (!strcmp(IRSEL[i].fam, fam) && !strcmp(IRSEL[i].flav, flav)) return IRSEL[i].recipe;
    return "bad";
}

int main(int argc, char **argv) {
    int i;
    if (argc == 2 && !strcmp(argv[1], "table")) {
        for (i = 0; i < NROWS; i++) printf("%s\t%s\t%s\n", IRSEL[i].fam, IRSEL[i].flav, IRSEL[i].recipe);
        return 0;
    }
    if (argc == 4 && !strcmp(argv[1], "call")) {
        printf("%s\n", irsel_label(argv[2], argv[3]));
        return 0;
    }
    fprintf(stderr, "usage: unisa-gold-irsel-label table | call FAM FLAV\n");
    return 2;
}
