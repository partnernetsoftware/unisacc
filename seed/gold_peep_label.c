/* seed/gold_peep_label.c -- the peep net's labels in rule form, C99.
 *
 *   gold-peep-label table         "A<TAB>B<TAB>REL<TAB>ACT" for every PEEP_A x PEEP_B x PEEP_REL key
 *                                 (8 x 17 x 12 = 1632 rows) in table order
 *   gold-peep-label call A B REL  one line "ACT" for that key
 *
 * peep_label(a, b, rel) with the tuples PEEP_A, PEEP_B and PEEP_REL.  The rules are tested in order and some fall
 * through: the store64 block answers only for b in {load64, store64} and otherwise continues.  The answer is one
 * of the PEEP_ACT names (keep when no rule matches); there is no refusal inside the function (a value outside the
 * tuples simply matches no rule and answers keep).
 * Second derivation of the frozen authority weights/gold/peep.tsv (gate gold-label-peep, tests/goldlabelcheck.cx):
 * `table` reproduces its key rows byte for byte, in the net's key order; the rule form here and the table form
 * there are written independently.  Ported 0.0.41 (K6-24) from gold.py of the Python seed, which is not read.
 */
#include <stdio.h>
#include <string.h>

static const char *PEEP_A[] = {"store64", "load64", "imm", "mov", "jump", "jumpz", "alu", "other"};
#define NA ((int)(sizeof PEEP_A / sizeof PEEP_A[0]))
static const char *PEEP_B[] = {"store64", "load64", "imm", "mov", "jump", "jumpz", "add64", "sub64", "mul64",
                               "shl64", "shr64", "lshr64", "or64", "xor64", "and64", "other", "none"};
#define NB ((int)(sizeof PEEP_B / sizeof PEEP_B[0]))
static const char *PEEP_REL[] = {"none", "same_slot", "same_slot_same_reg", "const0", "const1", "pow2", "copy_dead",
                                 "to_next", "to_jump", "a_dead", "dest_to_mov", "copy_into"};
#define NREL ((int)(sizeof PEEP_REL / sizeof PEEP_REL[0]))

static int eq(const char *x, const char *y) { return !strcmp(x, y); }
static int in(const char *x, const char **t, size_t n) { size_t i; for (i = 0; i < n; i++) if (!strcmp(t[i], x)) return 1; return 0; }

/* peep_label(a, b, rel): the rules in gold.py's order; a block that does not match falls through */
static const char *peep_label(const char *a, const char *b, const char *rel) {
    static const char *CONST0_B[] = {"add64", "sub64", "or64", "xor64", "shl64", "shr64", "lshr64"};
    if ((eq(rel, "same_slot") || eq(rel, "same_slot_same_reg")) && eq(a, "store64")) {
        if (eq(b, "load64")) return eq(rel, "same_slot") ? "load_to_mov" : "drop_b";
        if (eq(b, "store64")) return "drop_a";
    }
    if (eq(rel, "const0") && eq(a, "imm") && in(b, CONST0_B, sizeof CONST0_B / sizeof CONST0_B[0])) return "to_mov";
    if (eq(rel, "const1") && eq(a, "imm") && eq(b, "mul64")) return "to_mov";
    if (eq(rel, "copy_dead") && eq(a, "imm") && eq(b, "mov")) return "fold_imm";
    if (eq(rel, "to_next") && (eq(a, "jump") || eq(a, "jumpz"))) return "drop_a";
    if (eq(rel, "to_jump") && (eq(a, "jump") || eq(a, "jumpz")) && eq(b, "jump")) return "retarget";
    if (eq(rel, "a_dead") && (eq(a, "imm") || eq(a, "mov") || eq(a, "alu") || eq(a, "load64"))) return "drop_a";
    if (eq(rel, "dest_to_mov") && eq(b, "mov") && (eq(a, "imm") || eq(a, "load64") || eq(a, "mov") || eq(a, "alu") || eq(a, "other")))
        return "retarget_dest";
    if (eq(rel, "copy_into") && eq(a, "mov") && !eq(b, "none")) return "fold_copy";
    return "keep";
}

int main(int argc, char **argv) {
    int i, j, k;
    if (argc == 2 && !strcmp(argv[1], "table")) {
        for (i = 0; i < NA; i++)
            for (j = 0; j < NB; j++)
                for (k = 0; k < NREL; k++)
                    printf("%s\t%s\t%s\t%s\n", PEEP_A[i], PEEP_B[j], PEEP_REL[k], peep_label(PEEP_A[i], PEEP_B[j], PEEP_REL[k]));
        return 0;
    }
    if (argc == 5 && !strcmp(argv[1], "call")) {
        printf("%s\n", peep_label(argv[2], argv[3], argv[4]));
        return 0;
    }
    fprintf(stderr, "usage: gold-peep-label table | call A B REL\n");
    return 2;
}
