/* seed/gold_parse_label.c -- the parse net's labels in rule form, C99.
 *
 *   gold-parse-label table        "NT<TAB>TOK<TAB>LABEL" for every NT x TOK key of the parse net (5 x 68 = 340
 *                                 rows; the net's domain, in key order)
 *   gold-parse-label call NT TOK  one line "LABEL" for that key (NT outside the five is refused: rc 1)
 *
 * parse_label(nt, tok): the rules of each nonterminal in order, the fall-through to PARSE_DEFAULT when no rule
 * answers, the SAME_NAME keyword list, and the unary map (default prim).  The tables NT, TOKS, SAME_NAME and
 * PARSE_DEFAULT are embedded.  The label set is the net's head (weights/gold/parse.tsv #head).
 * Second derivation of the frozen authority weights/gold/parse.tsv (gate gold-label-parse, tests/goldlabelcheck.cx):
 * `table` reproduces its key rows byte for byte, in the net's key order; the rule form here and the table form
 * there are written independently.  Ported 0.0.41 (K6-29) from gold.py of the Python seed, which is not read.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *NT[] = {"top", "stmt", "unary", "postfix", "after_name"};
#define NNT ((int)(sizeof NT / sizeof NT[0]))
static const char *TOKS[] = {"eof", "type", "id", "num", "str", "if", "else", "while", "for", "do", "switch", "case", "default", "return", "break", "continue", "sizeof", "struct", "typedef", "enum", "{", "}", "(", ")", "[", "]", ";", ",", "=", "+=", "-=", "*=", "/=", "?", ":", "+", "-", "*", "/", "%", "==", "!=", "<", ">", "<=", ">=", "&&", "||", "!", "&", "++", "--", ".", "->", "goto", "|", "^", "<<", ">>", "union", "~", "%=", "&=", "|=", "^=", "<<=", ">>=", "..."};
#define NTOK ((int)(sizeof TOKS / sizeof TOKS[0]))
static const char *SAME_NAME[] = {"if", "while", "for", "do", "switch", "case", "default", "return", "break", "continue", "goto"};
#define NSAME ((int)(sizeof SAME_NAME / sizeof SAME_NAME[0]))
static const struct { const char *nt, *label; } PARSE_DEFAULT[] = {{"top", "global"}, {"stmt", "expr"}, {"unary", "prim"}, {"postfix", "done"}, {"after_name", "var_def"}};
#define NDEF ((int)(sizeof PARSE_DEFAULT / sizeof PARSE_DEFAULT[0]))

static int eq(const char *x, const char *y) { return !strcmp(x, y); }
static int in(const char *x, const char **t, size_t n) { size_t i; for (i = 0; i < n; i++) if (!strcmp(t[i], x)) return 1; return 0; }

static const char *default_for(const char *nt) {
    size_t i;
    for (i = 0; i < NDEF; i++) if (eq(PARSE_DEFAULT[i].nt, nt)) return PARSE_DEFAULT[i].label;
    fprintf(stderr, "gold-parse-label: no default for nonterminal %s\n", nt);
    exit(1);
}

/* parse_label(nt, tok): the rules of gold.py in order; a rule that does not answer falls through to the default */
static const char *parse_label(const char *nt, const char *tok) {
    if (eq(nt, "top")) {
        if (eq(tok, "eof")) return "end";
        if (eq(tok, "typedef") || eq(tok, "struct") || eq(tok, "enum")) return tok;
        if (eq(tok, "union")) return "struct";
    } else if (eq(nt, "after_name")) {
        if (eq(tok, "(")) return "fn_sig";
        if (eq(tok, "{")) return "fn_sig";
    } else if (eq(nt, "stmt")) {
        if (eq(tok, "type") || eq(tok, "struct") || eq(tok, "enum") || eq(tok, "typedef") || eq(tok, "union")) return "decl";
        if (eq(tok, "{")) return "block";
        if (in(tok, SAME_NAME, NSAME)) return tok;
    } else if (eq(nt, "unary")) {
        if (eq(tok, "-")) return "neg";
        if (eq(tok, "+")) return "uplus";
        if (eq(tok, "!")) return "not";
        if (eq(tok, "~")) return "bnot";
        if (eq(tok, "*")) return "deref";
        if (eq(tok, "&")) return "addr";
        if (eq(tok, "sizeof")) return "sizeof";
        if (eq(tok, "++") || eq(tok, "--")) return "preinc";
        return "prim";
    } else if (eq(nt, "postfix")) {
        if (eq(tok, "[")) return "index";
        if (eq(tok, "(")) return "call";
        if (eq(tok, "++") || eq(tok, "--")) return "inc";
        if (eq(tok, ".") || eq(tok, "->")) return "field";
    }
    return default_for(nt);
}

static int known_nt(const char *nt) { return in(nt, NT, NNT); }

int main(int argc, char **argv) {
    int i, j;
    if (argc == 2 && !strcmp(argv[1], "table")) {
        for (i = 0; i < NNT; i++)
            for (j = 0; j < NTOK; j++)
                printf("%s\t%s\t%s\n", NT[i], TOKS[j], parse_label(NT[i], TOKS[j]));
        return 0;
    }
    if (argc == 4 && !strcmp(argv[1], "call")) {
        if (!known_nt(argv[2])) {
            fprintf(stderr, "gold-parse-label: unknown nonterminal %s\n", argv[2]);
            return 1;
        }
        printf("%s\n", parse_label(argv[2], argv[3]));
        return 0;
    }
    fprintf(stderr, "usage: gold-parse-label table | call NT TOK\n");
    return 2;
}
