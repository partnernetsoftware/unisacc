/* seed/unisa_gold_pp_label.c -- C99 reference for unisa.gold pp_label (K6-26), finite-domain samples.
 *
 *   unisa-gold-pp-label table        "DIR<TAB>DEFINED<TAB>ACT" for every DIRS x DEFINED key (9 x 2 = 18 rows)
 *   unisa-gold-pp-label call DIR F   one line "ACT" for that key
 *
 * Mirrors unisa/gold.py pp_label(d, f): on is true only for the exact value "1" of f; endif pops; define,
 * undef and include are macros; ifdef takes when on; ifndef takes when off; if, elif and else take when on;
 * every other directive is skipped.  There is no refusal inside the function.  Point-sample equivalence over
 * the finite domain only; not the gold stage closure; not build(); keeps gold.py.
 */
#include <stdio.h>
#include <string.h>

static const char *DIRS[] = {"ifdef", "ifndef", "if", "elif", "else", "endif", "define", "include", "undef"};
#define NDIRS ((int)(sizeof DIRS / sizeof DIRS[0]))
static const char *DEFINED[] = {"0", "1"};
#define NDEF ((int)(sizeof DEFINED / sizeof DEFINED[0]))

static int eq(const char *x, const char *y) { return !strcmp(x, y); }

/* pp_label(d, f) */
static const char *pp_label(const char *d, const char *f) {
    int on = eq(f, "1");
    if (eq(d, "endif")) return "pop";
    if (eq(d, "define") || eq(d, "undef")) return "macro";
    if (eq(d, "include")) return "macro";
    if (eq(d, "ifdef")) return on ? "take" : "skip";
    if (eq(d, "ifndef")) return !on ? "take" : "skip";
    if (eq(d, "if") || eq(d, "elif") || eq(d, "else")) return on ? "take" : "skip";
    return "skip";
}

int main(int argc, char **argv) {
    int i, j;
    if (argc == 2 && !strcmp(argv[1], "table")) {
        for (i = 0; i < NDIRS; i++)
            for (j = 0; j < NDEF; j++)
                printf("%s\t%s\t%s\n", DIRS[i], DEFINED[j], pp_label(DIRS[i], DEFINED[j]));
        return 0;
    }
    if (argc == 4 && !strcmp(argv[1], "call")) {
        printf("%s\n", pp_label(argv[2], argv[3]));
        return 0;
    }
    fprintf(stderr, "usage: unisa-gold-pp-label table | call DIR F\n");
    return 2;
}
