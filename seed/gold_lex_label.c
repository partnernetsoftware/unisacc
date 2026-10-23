/* seed/gold_lex_label.c -- the lex net's labels in rule form, C99.
 *
 *   gold-lex-label table        one line "C<TAB>PEEK<TAB>LABEL" for every CHARC class x CHARC peek class
 *                               (12 x 12 = 144 rows, the net's own key domain)
 *   gold-lex-label call C PEEK  one line "LABEL" (a refusal for a class outside CHARC)
 *
 * lex_label(c, peek): the two comment starts (slash followed by slash or star), the dot rule (a dot followed by a
 * digit is a number, any other dot is an operator), and the class table for every other class.  Only slash, star
 * and d are distinguished as peek values.  A class outside CHARC is refused (rc 1, KeyError in the rule).
 * Second derivation of the frozen authority weights/gold/lex.tsv (gate gold-label-lex, tests/goldlabelcheck.cx):
 * `table` reproduces its key rows byte for byte, in the net's key order; the rule form here and the table form
 * there are written independently.  Ported 0.0.41 (K6-20) from gold.py of the Python seed, which is not read.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *CHARC[] = {"ws", "nl", "A", "d", "q", "sq", "slash", "star", "punct", "eof", "other", "dot"};
#define NCHARC ((int)(sizeof CHARC / sizeof CHARC[0]))

static int in_charc(const char *c) { int i; for (i = 0; i < NCHARC; i++) if (!strcmp(CHARC[i], c)) return 1; return 0; }

/* lex_label(c, peek): the comment/number special cases first, then the class table (gold.py) */
static const char *lex_label(const char *c, const char *peek) {
    if (!strcmp(c, "slash")) {
        if (!strcmp(peek, "slash")) return "linecmt";
        if (!strcmp(peek, "star")) return "cmt";
    }
    if (!strcmp(c, "dot")) return !strcmp(peek, "d") ? "num" : "op";
    if (!strcmp(c, "ws")) return "skip";
    if (!strcmp(c, "nl")) return "nl";
    if (!strcmp(c, "A")) return "ident";
    if (!strcmp(c, "d")) return "num";
    if (!strcmp(c, "q")) return "str";
    if (!strcmp(c, "sq")) return "charlit";
    if (!strcmp(c, "slash") || !strcmp(c, "star") || !strcmp(c, "punct")) return "op";
    if (!strcmp(c, "eof")) return "skip";
    if (!strcmp(c, "other")) return "bad";
    fprintf(stderr, "gold-lex-label: unknown class %s\n", c);
    exit(1);
}

int main(int argc, char **argv) {
    int i, j;
    if (argc == 2 && !strcmp(argv[1], "table")) {
        for (i = 0; i < NCHARC; i++)
            for (j = 0; j < NCHARC; j++)   /* the peek field ranges over the same classes */
                printf("%s\t%s\t%s\n", CHARC[i], CHARC[j], lex_label(CHARC[i], CHARC[j]));
        return 0;
    }
    if (argc == 4 && !strcmp(argv[1], "call")) {
        if (!in_charc(argv[2])) { fprintf(stderr, "gold-lex-label: unknown class %s\n", argv[2]); return 1; }
        printf("%s\n", lex_label(argv[2], argv[3]));
        return 0;
    }
    fprintf(stderr, "usage: gold-lex-label table | call C PEEK\n");
    return 2;
}
