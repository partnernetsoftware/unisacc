/*
 * gate.c — the continuation decision, in C99. LIBRARY, NO `main`.
 *
 * WHY NO `main` HERE (changed 2026-10-01, loop.c): a source file run directly
 * is a program, and two files with `main` cannot be combined —
 *     unisacc gate.c json.c session.c loop.c    →  arm64: main:
 * which reads like a missing backend feature and is not. loop.c is the program
 * that drives all three, so the LIBRARIES drop `main` and the CLI moved to
 * gate_cli.c. The self-test lives in that file (`gate_run_selftest`) so
 * `unisacc gate.c gate_cli.c selftest` proves this module alone.
 *
 * The CLI takes SUBCOMMANDS, not dash-options. Measured on unisacc 0.0.17:
 * running a source directly reserves the dash flags for the compiler itself —
 * `unisacc gate.c --self-test` dies with "model driver: option not migrated"
 * and `-x` with "missing compatibility argument", while a bare word passes
 * through to the program (argc/argv verified). So `selftest` and
 * `threshold N`, not `--self-test` and `--threshold N`.
 *
 * That is a real constraint for a C99 harness: a tool's own flags collide with
 * the compiler's when the source IS the executable. Same reason the JS side
 * never had to care.
 *
 * WHY THIS PIECE FIRST: of everything dsh does, this is the part the
 * measurements say costs the most per unit of value. The JS version spends
 * ~1686ms per observe-then-decide cycle, of which the tool itself is 30ms
 * (1.8%) — the rest is a model round trip. And the decision itself, reduced to
 * its actual information content, is ONE NUMBER (see PRD R-3: 178 chars of
 * JSON became 3 chars of probability).
 *
 * A number does not need a runtime. Pulling this layer into C99 makes the
 * cheap half of the loop genuinely cheap, and it is buildable TODAY with the
 * headers unisacc has: stdio, stdlib, string, ctype, math, stdint. No termios
 * (that is the TUI), no sys/stat (that is the file tools), no socket (that is
 * the LLM call) — none of which this file touches.
 *
 * It is deliberately a library with a CLI wrapper, so the JS side can shell
 * out to it before any larger port is attempted.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#define GATE_MAX_TEXT 65536
#define GATE_DEFAULT_THRESHOLD 0.5

/* What the caller asked us to decide. */
typedef struct {
    double threshold;   /* continue when p >= threshold */
} gate_config;

/* The verdict, plus the number it came from — never one without the other. */
typedef struct {
    int    cont;        /* 1 = continue, 0 = stop */
    double p;           /* the parsed probability */
    int    ok;          /* 0 when no number could be read at all */
    char   why[128];
} gate_decision;

/*
 * Read the LAST number in the text as a probability.
 *
 * The rules are the ones the JS version had to learn by measurement:
 *   - scan for the last number, not the first: a model that reasons aloud
 *     puts its answer at the end
 *   - accept a trailing '%'
 *   - clamp instead of rejecting: an out-of-range value is a slip, and
 *     clamping is the safe direction for a probability
 *   - a reply with NO number is a failure, not a "stop" — a broken route must
 *     not look like a decisive arbiter
 */
static int parse_last_number(const char *text, double *out)
{
    const char *p = text;
    const char *last_start = NULL;
    size_t last_len = 0;
    int    last_pct = 0;

    while (*p) {
        if (isdigit((unsigned char)*p) ||
            (*p == '-' && isdigit((unsigned char)p[1])) ||
            (*p == '.' && isdigit((unsigned char)p[1]))) {
            const char *start = p;
            if (*p == '-') p++;
            while (isdigit((unsigned char)*p)) p++;
            if (*p == '.') { p++; while (isdigit((unsigned char)*p)) p++; }
            last_start = start;
            last_len = (size_t)(p - start);
            last_pct = (*p == '%');
            if (last_pct) p++;
        } else {
            p++;
        }
    }
    if (last_start == NULL) return 0;

    char buf[64];
    size_t n = last_len < sizeof(buf) - 1 ? last_len : sizeof(buf) - 1;
    memcpy(buf, last_start, n);
    buf[n] = '\0';

    char *end = NULL;
    double v = strtod(buf, &end);
    if (end == buf) return 0;
    if (last_pct) v /= 100.0;

    /* Clamp: out of range is a slip, and clamping is the safe direction. */
    if (v < 0.0) v = 0.0;
    if (v > 1.0) v = 1.0;
    *out = v;
    return 1;
}

/* Decide. Kept separate from parsing: what counts as "continue" is policy. */
static gate_decision decide(const char *reply, const gate_config *cfg)
{
    gate_decision d;
    memset(&d, 0, sizeof(d));
    double p = 0.0;
    if (!parse_last_number(reply, &p)) {
        d.ok = 0;
        d.cont = 0;
        snprintf(d.why, sizeof(d.why), "no number in reply");
        return d;
    }
    d.ok = 1;
    d.p = p;
    d.cont = (p >= cfg->threshold) ? 1 : 0;
    if (d.cont) snprintf(d.why, sizeof(d.why), "p=%.2f >= %.2f", p, cfg->threshold);
    else        snprintf(d.why, sizeof(d.why), "p=%.2f < %.2f", p, cfg->threshold);
    return d;
}

/* ── library entry points ───────────────────────────────────────────────── */

/* Decide on a reply string. Exposed (non-static) so loop.c — a DIFFERENT
 * translation unit — can decide without restating the policy. loop.c restates
 * gate_config/gate_decision so it compiles on its own; unisacc can see an
 * earlier file's type, but that is order-dependent.
 *
 * MEASURED unisacc LIMITATION (0.0.17, 2026-10-01): `return decide(...)` —
 * returning a struct-returning call straight out of a non-main function — is
 * rejected as "unknown identifier" at the call. Assigning to a local first and
 * returning THAT is accepted, and both forms are the same to gcc. So the local
 * is not decoration; it is what makes this file build on unisacc. */
gate_decision gate_decide(const char *reply, const gate_config *cfg)
{
    gate_decision d = decide(reply, cfg);
    return d;
}

/* Do the reading/printing OUTSIDE this library, so the library stays a pure
 * string→decision function with no `FILE *` in its signature. An earlier note
 * here said a cross-file `FILE *` prototype made unisacc mis-lower `%.4f`; that
 * is not true on 0.0.23 (measured 2026-10-04, six targets), so the split stands
 * on its own as a design choice rather than as a workaround. */

/*
 * Self-test, callable from gate_cli.c. Kept in the library file (not the CLI)
 * so the assertions live next to the policy they check — the same reason the
 * JS side keeps its tests beside the code.
 */

/* gate_cli.c owns the self-test. */
