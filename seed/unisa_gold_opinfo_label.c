/* seed/unisa_gold_opinfo_label.c -- C99 reference for unisa.gold opinfo_label (K6-23), finite-domain samples.
 *
 *   unisa-gold-opinfo-label table       "OP<TAB>SIMPLE<TAB>ACLS<TAB>BCLS" for every OPINFO_OPS entry
 *                                       (tape.SHAPE in order, 72 ops incl. the fp ops, then "other": 73 rows)
 *   unisa-gold-opinfo-label call OP     one line "SIMPLE ACLS BCLS" for OP
 *
 * Mirrors unisa/gold.py opinfo_label(op): simple = 1 for OPINFO_SIMPLE; acls = the op itself for the six
 * classes store64, load64, imm, mov, jump, jumpz, "alu" for OPINFO_ALU, else "other"; bcls = the op itself
 * when it is in PEEP_B and is neither "other" nor "none", else "other".  The op list is tape.SHAPE's keys in
 * insertion order (generated from unisa/tape.py, read-only) followed by "other".  An op outside the domain
 * answers the fall-through values (python: the same branches; there is no KeyError).  Point-sample
 * equivalence over the finite domain only; not the gold stage closure; not build(); keeps gold.py and tape.py.
 */
#include <stdio.h>
#include <string.h>

static const char *OPS[] = {
    "imm", "mov", "add64", "sub64", "mul64", "xor64",
    "and64", "or64", "shl64", "shr64", "lshr64", ".div",
    ".mod", ".udiv", ".umod", "slt64", "sle64", "ult64",
    "ule64", "eq", "ne", "load64", "store64", ".ld",
    ".st", ".lea", ".zero", "jump", "jumpz", "call",
    "callr", "callm", ".hostcall", ".librarycall", ".libraryaddr", ".libraryfn", ".hostaddr",
    "ret", ".frame", ".arg", ".print", ".write", ".exit",
    ".sys", ".sys6", ".argc", ".argv", "nop", "fadd64",
    "fsub64", "fmul64", "fdiv64", "flt64", "fle64", "feq64",
    "fadd32", "fsub32", "fmul32", "fdiv32", "flt32", "fle32",
    "feq32", "cvtid", "cvtud", "cvtis", "cvtus", "cvtdi",
    "cvtdu", "cvtsd", "cvtds", "fsqrt64", "fsqrt32", "other"
};
#define NOPS ((int)(sizeof OPS / sizeof OPS[0]))

static const char *SIMPLE[] = {"imm", "load64", "store64", "sub64", ".ld", ".lea", ".st", "add64", "mul64", "eq", "mov",
                               "slt64", "and64", "ne", "sle64", "or64", "shl64", "shr64", "lshr64", "xor64", "ult64"};
static const char *ALU[] = {"add64", "sub64", "mul64", "shl64", "shr64", "lshr64", "or64", "xor64", "and64", "eq", "ne",
                            "slt64", "sle64", "ult64"};
/* PEEP_B of gold.py, the bcls vocabulary (the "none" entry is excluded by the rule, not by the list) */
static const char *PEEP_B[] = {"store64", "load64", "imm", "mov", "jump", "jumpz", "add64", "sub64", "mul64", "shl64",
                               "shr64", "lshr64", "or64", "xor64", "and64", "other", "none"};

static int in(const char *x, const char **t, size_t n) { size_t i; for (i = 0; i < n; i++) if (!strcmp(t[i], x)) return 1; return 0; }
#define IN(x, t) in((x), (t), sizeof (t) / sizeof (t)[0])

/* opinfo_label(op): simple / acls / bcls, the branch order of gold.py */
static void opinfo(const char *op, const char **simple, const char **acls, const char **bcls) {
    *simple = IN(op, SIMPLE) ? "1" : "0";
    if (!strcmp(op, "store64") || !strcmp(op, "load64") || !strcmp(op, "imm") || !strcmp(op, "mov") || !strcmp(op, "jump") || !strcmp(op, "jumpz"))
        *acls = op;
    else if (IN(op, ALU))
        *acls = "alu";
    else
        *acls = "other";
    *bcls = (IN(op, PEEP_B) && strcmp(op, "other") && strcmp(op, "none")) ? op : "other";
}

int main(int argc, char **argv) {
    int i;
    const char *s, *a, *b;
    if (argc == 2 && !strcmp(argv[1], "table")) {
        for (i = 0; i < NOPS; i++) { opinfo(OPS[i], &s, &a, &b); printf("%s\t%s\t%s\t%s\n", OPS[i], s, a, b); }
        return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "call")) {
        opinfo(argv[2], &s, &a, &b);
        printf("%s %s %s\n", s, a, b);
        return 0;
    }
    fprintf(stderr, "usage: unisa-gold-opinfo-label table | call OP\n");
    return 2;
}
