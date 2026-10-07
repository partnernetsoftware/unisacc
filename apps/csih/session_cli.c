/*
 * session_cli.c — the `main` for session.c, moved out so session.c can be a
 * library.
 *
 * WHY: see gate_cli.c. One `main` per combined program is a hard unisacc rule —
 * two files with `main` fail as
 *     reject: not covered: ARM64 operand or instruction
 *     arm64: main:
 * which reads like a missing backend feature. loop.c drives the libraries, so
 * session.c drops `main`, its self-test is reachable as `session_run_selftest`,
 * and this file owns the verbs `read` / `append`.
 *
 * unisacc shares no TYPES across files, so `session_records` is restated here
 * (a requirement, not a style choice).
 *
 * The CLI takes SUBCOMMANDS, not dash-options (unisacc reserves the dashes).
 *
 * BUILD (either backend; the two must agree):
 *     unisacc json.c session.c session_cli.c selftest
 *     cc -std=c99 json.c session.c session_cli.c -o /tmp/session && /tmp/session selftest
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── the slice of session.c/json.c this CLI uses (types restated) ───────── */
typedef struct {
    char **lines;     /* owned */
    size_t count;
    size_t bad;       /* lines that were not parseable JSON objects */
} session_records;

int session_append(const char *path, const char *record);
session_records session_read(const char *path);
void session_free(session_records *r);
int session_run_selftest(void);

int main(int argc, char **argv) {
    if (argc > 1 && !strcmp(argv[1], "selftest")) {
        return session_run_selftest();
    }
    if (argc > 2 && !strcmp(argv[1], "read")) {
        session_records r = session_read(argv[2]);
        size_t i;
        for (i = 0; i < r.count; i++) printf("%s\n", r.lines[i]);
        printf("records %lu bad %lu\n", (unsigned long)r.count, (unsigned long)r.bad);
        { int bad = r.bad > 0; session_free(&r); return bad ? 1 : 0; }
    }
    if (argc > 3 && !strcmp(argv[1], "append")) {
        int rc = session_append(argv[2], argv[3]);
        if (rc) { printf("append failed rc=%d\n", rc); return rc; }
        printf("appended\n");
        return 0;
    }
    printf("usage: session selftest | read <file> | append <file> <json>\n");
    return 64;
}

/* ── self-test ──────────────────────────────────────────────────────────── */

static int failures = 0;
static void expect(int cond, const char *what) {
    if (!cond) { printf("FAIL %s\n", what); failures++; }
}

static void rm_file(const char *p) { remove(p); }

/* Returns 0 when all cases pass. */
int session_run_selftest(void) {
    const char *tmp = "/tmp/cdsh-session-selftest.jsonl";
    session_records r;

    rm_file(tmp);

    /* Append three, read three. */
    expect(session_append(tmp, "{\"role\":\"user\",\"text\":\"hi\"}") == 0, "append 1");
    expect(session_append(tmp, "{\"role\":\"assistant\",\"text\":\"yo\"}") == 0, "append 2");
    expect(session_append(tmp, "{\"role\":\"tool\",\"name\":\"bash\"}") == 0, "append 3");
    r = session_read(tmp);
    expect(r.count == 3, "three records read back");
    expect(r.bad == 0, "none malformed");
    session_free(&r);

    /* A newline inside a record must be REFUSED: it would split one entry into
     * two on read, which is a corruption nobody notices until much later. */
    expect(session_append(tmp, "{\"a\":1}\n{\"b\":2}") == 2, "embedded newline refused");
    r = session_read(tmp);
    expect(r.count == 3, "the refusal did not write anything");
    session_free(&r);

    /* Empty and NULL are refused rather than writing a blank line. */
    expect(session_append(tmp, "") == 1, "empty record refused");
    expect(session_append(tmp, NULL) == 1, "NULL record refused");

    /* A malformed line is COUNTED, not silently dropped. */
    {
        FILE *f = fopen(tmp, "a");
        if (f) { fprintf(f, "not json\n"); fclose(f); }
    }
    r = session_read(tmp);
    expect(r.count == 3, "good records survive");
    expect(r.bad == 1, "the malformed line is counted, not lost");
    session_free(&r);

    /* Reading a missing file is an empty result, not a crash. */
    r = session_read("/tmp/cdsh-does-not-exist-xyz.jsonl");
    expect(r.count == 0 && r.bad == 0, "missing file is empty, not fatal");
    session_free(&r);

    rm_file(tmp);

    printf("%s\n", failures ? "SELFTEST FAILED" : "selftest ok");
    return failures == 0 ? 0 : 1;
}

/* The CLI (`main`, `read`, `append`) lives in session_cli.c — see gate_cli.c
 * for why `main` cannot be here and why the CLI, not the library, prints. */
