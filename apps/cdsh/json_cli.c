/*
 * json_cli.c — the `main` for json.c, moved out so json.c can be a library.
 *
 * WHY: see gate_cli.c. One `main` per combined program is a hard unisacc rule —
 * two files with `main` fail as
 *     reject: not covered: ARM64 operand or instruction
 *     arm64: main:
 * which reads like a missing backend feature. loop.c drives the libraries, so
 * json.c drops `main` and its self-test is reachable as `json_run_selftest`.
 *
 * This file owns the stdin reading and the summary print, so json.c stays a pure
 * string→value library. An earlier note here blamed a cross-file `FILE *`
 * prototype for mis-lowering `%.4f`; unisacc 0.0.23 does not do that (measured
 * 2026-10-04: the same shape builds for all six targets), so that claim is gone.
 *
 * The CLI takes SUBCOMMANDS, not dash-options (unisacc reserves the dashes).
 *
 * BUILD (either backend; the two must agree):
 *     unisacc json.c json_cli.c selftest
 *     cc -std=c99 json.c json_cli.c -o /tmp/json && /tmp/json selftest
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── the slice of json.c this CLI uses (types restated so this file compiles
 *    on its own; unisacc can see an earlier file's type, but that is
 *    order-dependent) ──────────────────────────────────────────────────────── */
typedef enum { J_NULL, J_BOOL, J_NUM, J_STR, J_ARR, J_OBJ } jkind;
typedef struct jvalue {
    jkind kind;
    int    b;
    double n;
    char  *s;
    struct jvalue **items;  size_t len;
    char **keys; struct jvalue **vals; size_t nkeys;
} jvalue;

jvalue *json_parse(const char *text, size_t len, char *errbuf, size_t errlen);
void    jfree(jvalue *v);
int     json_run_selftest(void);

/* Read JSONL from stdin, report one summary line per record. */
static int run_stream(void) {
    /* HEAP, not a 1 MB stack array. Measured on unisacc 0.0.17 / osx-arm64:
     * a stack array of 1<<20 is rejected with "not covered: ARM64 operand or
     * instruction", while 262144 compiles fine — so the limit is somewhere
     * between 256 KB and 1 MB. A heap buffer sidesteps the question entirely,
     * and is the right choice anyway for a line buffer whose size is a policy
     * (how long may one session record be?) rather than a fact about the
     * machine. Fixed size on the stack also silently truncates a long line. */
    const size_t CAP = 1u << 20;
    char *line = (char *)malloc(CAP);
    long n = 0, ok = 0, bad = 0;
    if (!line) { printf("out of memory\n"); return 2; }
    while (fgets(line, (int)CAP, stdin)) {
        size_t len = strlen(line);
        char err[128];
        jvalue *v;
        while (len && (line[len-1]=='\n' || line[len-1]=='\r')) line[--len] = '\0';
        if (!len) continue;
        n++;
        v = json_parse(line, len, err, sizeof(err));
        if (v) { ok++; jfree(v); }
        else { bad++; printf("line %ld: %s\n", n, err); }
    }
    printf("records %ld ok %ld bad %ld\n", n, ok, bad);
    free(line);
    return bad == 0 ? 0 : 1;
}

int main(int argc, char **argv) {
    /* Subcommands, not dash-options — see the header note. */
    if (argc > 1 && !strcmp(argv[1], "selftest")) return json_run_selftest();
    return run_stream();
}
