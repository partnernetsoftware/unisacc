/* seed/gold_arity_label.c -- the arity net's labels in rule form, C99.
 *
 *   gold-arity-label table                  "PROTO<TAB>NPAR<TAB>NARGS<TAB>LABEL" for every key
 *   gold-arity-label call PROTO NPAR NARGS  one line "LABEL" for that key
 *
 * The key is what the parse (E3) stage knows when a call closes: which prototype form is in view,
 * how many parameters the prototype names, and how many arguments the call site wrote.  It is not a
 * typing question: `weights/gold/type.tsv` answers what `fn call t2` yields and nothing about count,
 * and `weights/gold/scope.tsv` answers what a name binds to and nothing about count.
 *
 * proto:
 *   none      no prototype in view: `()` empty parameter list (C89 6.5.2.2p1 imposes no parameter
 *             constraint), a function pointer, or an implicit declaration.  The count is not checked.
 *   fixed     a prototype with a named parameter list and no `...`.  NPAR must equal NARGS;
 *             NPAR 0 is `(void)` and accepts only NARGS 0.
 *   variadic  last parameter is `...`.  C99 6.7.5.3p1 needs at least one named parameter, so NPAR 1
 *             and up must satisfy NARGS >= NPAR.  NPAR 0 is not a C99 form; it is labelled accept
 *             and left unchecked rather than inventing a refusal the language does not have yet.
 * labels: accept, too-few-arguments, too-many-arguments
 *
 * The domain is the product front end's fixed head: 3 PROTO x 9 NPAR (0..8) x 9 NARGS (0..8) = 243 keys.
 * Diagnostics are not here: the reason text lives beside the other diagnostics
 * (exec/facts/k2-errors.tsv), and the call site's line and column come from the token locations
 * that the parse stage already records.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *PROTO[] = {"none", "fixed", "variadic"};
#define NPROTO ((int)(sizeof PROTO / sizeof PROTO[0]))
#define NCOUNT 9

static int eq(const char *x, const char *y) { return !strcmp(x, y); }

/* arity_label(proto, npar, nargs): the rule the table form must reproduce row for row */
static const char *arity_label(const char *proto, int npar, int nargs) {
    if (eq(proto, "none")) return "accept";
    if (eq(proto, "fixed")) {
        if (nargs < npar) return "too-few-arguments";
        if (nargs > npar) return "too-many-arguments";
        return "accept";
    }
    if (eq(proto, "variadic")) {
        if (npar == 0) return "accept";
        if (nargs < npar) return "too-few-arguments";
        return "accept";
    }
    return "accept";
}

static int count(const char *s, int *out) {
    char *end = 0;
    long v;
    if (!s || !*s) return 0;
    v = strtol(s, &end, 10);
    if (!end || *end || v < 0 || v >= NCOUNT) return 0;
    *out = (int)v;
    return 1;
}

int main(int argc, char **argv) {
    int i, j, k;
    if (argc == 2 && !strcmp(argv[1], "table")) {
        for (i = 0; i < NPROTO; i++)
            for (j = 0; j < NCOUNT; j++)
                for (k = 0; k < NCOUNT; k++)
                    printf("%s\t%d\t%d\t%s\n", PROTO[i], j, k, arity_label(PROTO[i], j, k));
        return 0;
    }
    if (argc == 5 && !strcmp(argv[1], "call")) {
        int npar = 0, nargs = 0;
        if (!count(argv[3], &npar) || !count(argv[4], &nargs)) {
            fprintf(stderr, "gold-arity-label: count outside 0..%d\n", NCOUNT - 1);
            return 2;
        }
        printf("%s\n", arity_label(argv[2], npar, nargs));
        return 0;
    }
    fprintf(stderr, "usage: gold-arity-label table | call PROTO NPAR NARGS\n");
    return 2;
}
