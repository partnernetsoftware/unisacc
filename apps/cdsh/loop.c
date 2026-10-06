/*
 * loop.c — the smallest CLOSED LOOP over the three cdsh libraries.
 *
 *   session file ──read──> a record ──take a field as probability──> gate
 *        ▲                                                             │
 *        └──────────────── verdict written back ◄─────────────────────┘
 *
 * WHY THIS EXISTS: gate.c, json.c and session.c each passed alone on both
 * backends, but nothing proved they COMPOSE. This file is that proof, and it is
 * the shape dsh actually runs: a session log goes in, a decision comes out, and
 * the decision is appended to the same log so the next turn sees it.
 *
 * ── the two-`main` choice (task asked for the reason) ────────────────────
 * I chose (a): the three libraries drop `main`, and their self-tests moved to
 * gate_cli.c / json_cli.c / session_cli.c. Measured why, not preferred by
 * default:
 *
 *   (b) "keep the mains and dodge the second one at combine time" needs a way
 *   to hide a `main` from the compiler. unisacc 0.0.17 does accept
 *   `-D NAME` and `#ifndef NAME ... main ... #endif` works — I measured
 *   `unisacc -D LIBCLI_NO_MAIN lib.c drv.c` → runs. BUT a `#define` inside one
 *   source does NOT reach another source: unisacc compiles each file on its
 *   own, so `loop.c` writing `#define LIBCLI_NO_MAIN` changed nothing and the
 *   combined build still died with `arm64: main:`. The only way to hide all
 *   three mains is to put `-D …` on EVERY multi-file command line, including
 *   the one the task specifies as `unisacc loop.c <其他.c...> selftest` — and a
 *   Makefile-less harness that must remember three -D flags will eventually
 *   forget one and get `arm64: main:`, which reads like a missing backend
 *   feature (SKILL.md §2b). (a) makes the combine command carry no options at
 *   all, and leaves each module's selftest reachable via its own CLI file.
 *   So (b) is possible but fragile; (a) is the one that keeps the failure from
 *   coming back.
 *
 * ── measured unisacc limits this file works around ──────────────────────
 * 1. Every struct this file names is restated below, so it compiles on its own.
 *    unisacc can see a type an EARLIER file on the command line defined, but
 *    that is order-dependent, and the restatement is what removes it.
 * 2. `return f()` where f returns a struct is rejected from a non-main
 *    function; assign to a local first. Done in `read_last_p`.
 * 3. The library entry points this file calls take `const char *`, never
 *    `FILE *`: stdio stays inside whichever file's call graph owns it, so each
 *    library here keeps a pure string→value interface.
 *
 * CLI takes SUBCOMMANDS, not dash-options: running a source directly reserves
 * the dash flags for the compiler itself (SKILL.md §3).
 *
 * BUILD (either backend; the outputs must agree):
 *     unisacc gate.c json.c session.c loop.c selftest
 *     cc -std=c99 gate.c json.c session.c loop.c -o /tmp/loop && /tmp/loop selftest
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── restated declarations (so this file compiles alone; see note 1) ─────── */

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
jvalue *jget(jvalue *obj, const char *key);
double  jnum(jvalue *v, double dflt);
const char *jstr(jvalue *v);

typedef struct {
    char **lines;     /* owned */
    size_t count;
    size_t bad;
} session_records;

int session_append(const char *path, const char *record);
session_records session_read(const char *path);
void session_free(session_records *r);

typedef struct { double threshold; } gate_config;
typedef struct {
    int    cont;
    double p;
    int    ok;
    char   why[128];
} gate_decision;

gate_decision gate_decide(const char *reply, const gate_config *cfg);

/* ── the loop ───────────────────────────────────────────────────────────── */

#define LOOP_MAX_LINE (1u << 16)

/*
 * Pull the probability out of the most recent record that carries the field.
 *
 * The LAST record, not the first: the transcript grows downward, so the newest
 * statement of the probability is the tail — the same reason gate.c scans for
 * the last number. Scanning backwards also means an early record with no `p`
 * is not fatal; only "no record anywhere has the field" is.
 *
 * The value is handed to the gate as TEXT (snprintf'd), not as a double,
 * because gate.c's rule is "read the last number in the text" — routing the
 * stored number back through the SAME parser means the loop and the gate can
 * never disagree about what a probability means. A JSON number and a model's
 * prose number are not different problems.
 */
static int read_last_p(const char *path, const char *field, double *out,
                       int *records_seen, int *bad_seen)
{
    session_records r = session_read(path);
    size_t i = r.count;
    int found = 0;
    *records_seen = (int)r.count;
    *bad_seen = (int)r.bad;

    while (i > 0) {
        jvalue *v;
        jvalue *pv;
        char err[128];
        i--;
        v = json_parse(r.lines[i], strlen(r.lines[i]), err, sizeof(err));
        if (!v) continue;                     /* counted by session_read */
        pv = jget(v, field);
        if (pv && pv->kind == J_NUM && jnum(pv, -1.0) >= 0.0) {
            *out = jnum(pv, 0.0);
            found = 1;
            jfree(v);
            break;
        }
        jfree(v);
    }
    session_free(&r);
    return found;
}

/*
 * Decide, then write the verdict back as a new record.
 *
 * The verdict record is built with snprintf into a fixed buffer, and a value
 * that does not fit is a hard error rather than a truncation: a truncated JSON
 * line is exactly the "invisible hole in the transcript" session.c refuses to
 * create, and it would be written by the very loop that is supposed to be the
 * example of doing it right.
 *
 * Returns the gate code: 0 = continue, 1 = stop, 2 = no number / no field.
 * The write-back still happens on STOP (that is the point of a closed loop:
 * the next turn must see that the loop chose to stop); it is skipped only when
 * there was nothing to decide (2), because recording a decision that was never
 * made would be a lie in the log.
 */
static int run_file(const char *path, const char *field, double threshold, int quiet)
{
    gate_config cfg;
    gate_decision d;
    double p = 0.0;
    int records_seen = 0, bad_seen = 0;
    char numbuf[64];
    char rec[512];
    int n;

    cfg.threshold = threshold;

    if (!read_last_p(path, field, &p, &records_seen, &bad_seen)) {
        if (!quiet)
            printf("NO-DECISION field=%s records=%d bad=%d reason=\"no %s field in any record\"\n",
                   field, records_seen, bad_seen, field);
        return 2;
    }

    /* Round-trip the number through text so the gate's own "last number" rule
     * is what interprets it (see read_last_p). */
    snprintf(numbuf, sizeof(numbuf), "%.6f", p);
    d = gate_decide(numbuf, &cfg);

    if (!quiet)
        printf("%s p=%.4f threshold=%.2f field=%s reason=\"%s\"\n",
               d.cont ? "CONTINUE" : "STOP", d.p, threshold, field, d.why);

    /* The verdict record: a normal session record, so the next read sees it as
     * just another line. "role" keeps it in the same shape the transcript uses. */
    n = snprintf(rec, sizeof(rec),
                 "{\"role\":\"gate\",\"field\":\"%s\",\"p\":%.4f,\"threshold\":%.2f,"
                 "\"verdict\":\"%s\"}",
                 field, d.p, threshold, d.cont ? "CONTINUE" : "STOP");
    if (n < 0 || (size_t)n >= sizeof(rec)) {
        if (!quiet) printf("verdict record too long; not written\n");
        return 2;
    }
    if (session_append(path, rec) != 0) {
        if (!quiet) printf("could not append verdict to %s\n", path);
        return 2;
    }
    if (!quiet) printf("wrote verdict role=gate verdict=%s\n", d.cont ? "CONTINUE" : "STOP");

    return d.cont ? 0 : 1;
}

/* ── self-test: does the loop actually close? ───────────────────────────── */

static int failures = 0;
static void expect(int cond, const char *what) {
    if (!cond) { printf("FAIL %s\n", what); failures++; }
}

/*
 * The closed-loop proof is not "does it print CONTINUE" — it is that the
 * verdict came back INTO the record set. So the assertions read the file again
 * (through session_read, not by trusting the writer) and look for the appended
 * record. If write-back were broken, the readback test would fail even though
 * the decision was right.
 */
static int count_field(const char *path, const char *field, const char *want)
{
    session_records r = session_read(path);
    size_t i;
    int hits = 0;
    for (i = 0; i < r.count; i++) {
        char err[128];
        jvalue *v = json_parse(r.lines[i], strlen(r.lines[i]), err, sizeof(err));
        if (v) {
            const char *s = jstr(jget(v, field));
            if (s && !strcmp(s, want)) hits++;
            jfree(v);
        }
    }
    session_free(&r);
    return hits;
}

static void run_selftest(void) {
    const char *path = "/tmp/cdsh-loop-selftest.jsonl";
    FILE *f;

    remove(path);

    /* A three-record transcript; the probability is in the LAST one, and the
     * first line deliberately has no `p` so "scan backwards" is exercised. */
    expect(session_append(path, "{\"role\":\"user\",\"text\":\"go\"}") == 0, "seed 1");
    expect(session_append(path, "{\"role\":\"user\",\"text\":\"0.20\"}") == 0, "seed 2 (no p field)");
    expect(session_append(path, "{\"role\":\"model\",\"p\":0.83}") == 0, "seed 3 (p=0.83)");

    /* CONTINUE at threshold 0.5: p=0.83 >= 0.5. */
    expect(run_file(path, "p", 0.5, 1) == 0, "p=0.83 → CONTINUE (rc 0)");
    expect(count_field(path, "verdict", "CONTINUE") == 1, "CONTINUE verdict landed back in the file");

    /* The verdict is now the newest record — and it carries no `p`, so a second
     * run must find `p` in the PREVIOUS record: the append did not break reads. */
    expect(run_file(path, "p", 0.5, 1) == 0, "the appended record does not block the read-back");
    expect(count_field(path, "verdict", "CONTINUE") == 2, "two CONTINUE verdicts now");

    /* Raise the threshold past 0.83: the SAME file now decides STOP — proving
     * the threshold is the policy input, not a constant in the record. */
    expect(run_file(path, "p", 0.9, 1) == 1, "p=0.83 < 0.9 → STOP (rc 1)");
    expect(count_field(path, "verdict", "STOP") == 1, "STOP verdict landed back in the file");

    /* A newer record with a lower p must win, because the LAST p is the answer. */
    expect(session_append(path, "{\"role\":\"model\",\"p\":0.10}") == 0, "newest p=0.10");
    expect(run_file(path, "p", 0.5, 1) == 1, "last record wins → STOP");
    expect(count_field(path, "verdict", "STOP") == 2, "second STOP recorded");

    /* A transcript with no such field is NO-DECISION (rc 2), not a STOP.
     * "judged to stop" and "could not judge" must stay distinct (gate.c's rule),
     * and nothing may be written when no decision was made. */
    {
        const char *empty = "/tmp/cdsh-loop-nofield.jsonl";
        int before;
        remove(empty);
        expect(session_append(empty, "{\"role\":\"user\",\"text\":\"hi\"}") == 0, "seed no-field file");
        before = count_field(empty, "verdict", "STOP") + count_field(empty, "verdict", "CONTINUE");
        expect(run_file(empty, "p", 0.5, 1) == 2, "no p field → NO-DECISION (rc 2)");
        expect(count_field(empty, "verdict", "STOP") + count_field(empty, "verdict", "CONTINUE") == before,
               "no verdict written when there was no decision");
        remove(empty);
    }

    /* A missing file is NO-DECISION too, not a crash and not a fabricated write. */
    remove("/tmp/cdsh-loop-missing.jsonl");
    expect(run_file("/tmp/cdsh-loop-missing.jsonl", "p", 0.5, 1) == 2, "missing file → NO-DECISION");

    /* A malformed line is COUNTED by session_read and skipped; a good p next to
     * it still decides. This is the compose check: the loop inherits session.c's
     * strictness rather than re-implementing parsing.
     */
    {
        const char *mixed = "/tmp/cdsh-loop-mixed.jsonl";
        remove(mixed);
        f = fopen(mixed, "a");
        if (f) { fprintf(f, "not json\n{\"role\":\"model\",\"p\":0.77}\n"); fclose(f); }
        expect(run_file(mixed, "p", 0.5, 1) == 0, "malformed line skipped, p=0.77 still decides");
        expect(count_field(mixed, "verdict", "CONTINUE") == 1, "verdict written to the mixed file");
        remove(mixed);
    }

    remove(path);
}

/* ── CLI ────────────────────────────────────────────────────────────────── */

int main(int argc, char **argv)
{
    /* Subcommands, never `--run`/`--field`: unisacc eats the dashes (SKILL §3). */
    if (argc > 1 && !strcmp(argv[1], "selftest")) {
        run_selftest();
        printf("%s\n", failures ? "SELFTEST FAILED" : "selftest ok");
        return failures == 0 ? 0 : 1;
    }

    /* loop run <file> [field <name>] [threshold <x>] */
    if (argc > 2 && !strcmp(argv[1], "run")) {
        const char *path = argv[2];
        const char *field = "p";
        double threshold = 0.5;
        int i;
        for (i = 3; i < argc; i++) {
            if (!strcmp(argv[i], "field") && i + 1 < argc) field = argv[++i];
            else if (!strcmp(argv[i], "threshold") && i + 1 < argc) threshold = strtod(argv[++i], NULL);
        }
        return run_file(path, field, threshold, 0);
    }

    printf("usage: loop selftest | run <file> [field <name>] [threshold <x>]\n");
    return 64;
}
