/*
 * session.c — read and write the harness's session records. LIBRARY, NO `main`.
 *
 * WHY NO `main` HERE (changed 2026-10-01, loop.c): a source file run directly
 * is a program, and two files with `main` cannot be combined —
 *     unisacc gate.c json.c session.c loop.c    →  arm64: main:
 * which reads like a missing backend feature and is not. loop.c drives the
 * three libraries, so they drop `main`; session's CLI lives in session_cli.c
 * and its self-test is reachable as `session_run_selftest`.
 *
 * WHY THIS LAYER: the session log is what an LLM sees as context, and it is
 * plain JSONL — one object per line. Reading and appending it needs only
 * stdio/stdlib/string/ctype, so like gate.c and json.c it can be built and
 * verified BEFORE unisacc grows termios/stat/socket. It is also the layer that
 * makes the C99 side useful early: an append-only transcript is the piece every
 * other feature leans on.
 *
 * WHAT IT IS NOT: an agent. It does not decide, call a model, or know what a
 * turn is. It stores and returns records, and it is strict about their shape —
 * a transcript that silently drops a malformed line is worse than one that
 * refuses, because the loss is invisible until someone reads the history and
 * finds a hole.
 *
 * The CLI takes SUBCOMMANDS, not dash-options: measured on unisacc 0.0.17,
 * running a source directly leaves the dash flags to the compiler itself.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* session.c uses json.c's reader. NO include of the body, NO header file, NO
 * compile-then-link: unisacc takes several sources and resolves them itself.
 *
 * `main` lives in ONE file only. When json.c and session.c each carried their
 * own `main`, combining them failed as
 *     reject: not covered: ARM64 operand or instruction / arm64: main:
 * which reads like a missing backend feature and sent me chasing stack arrays,
 * code size, and individual function bodies. All three were dead ends. Both
 * libraries now have no `main`; the drivers (session_cli.c, loop.c) carry it.
 *
 * What multi-file does NOT do is share TYPES textually: session.c restates the
 * little it needs, which is why the declarations below are duplicated from
 * json.c. The restatement is what lets this file compile on its own — unisacc
 * can see a type an EARLIER file on the command line defined, but that is
 * order-dependent, so the duplication removes the dependency rather than
 * papering over a missing feature.
 *
 * (I got here by three wrong turns — `#include "json.c"` gave a duplicate
 * `failures`, splitting into .h/.c then linking with cc gave a duplicate
 * `_main`, and pasting the reader in was overkill. All three were gcc habits.) */

/* The slice of json.c this file uses. Types are restated so this file compiles
 * alone: multi-file resolution is not a preprocessor, nothing is textually
 * shared, and relying on an earlier file's definition would make the build
 * order-dependent. */
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

/* ── append ─────────────────────────────────────────────────────────────── */

/*
 * Append one record. Returns 0 on success.
 *
 * The newline is written as part of the record, not by the caller: JSONL is
 * newline-DELIMITED, and a writer that forgets the terminator produces a file
 * whose last line silently merges with the next append.
 */
int session_append(const char *path, const char *record) {
    FILE *f;
    size_t n;
    if (!path || !record || !*record) return 1;
    /* A newline INSIDE the record would split one entry into two on read.
     * Reject rather than escape: the caller built a raw line, so a newline in
     * it is a caller bug, and silently mangling it creates a corrupt transcript. */
    if (strchr(record, '\n') || strchr(record, '\r')) return 2;
    f = fopen(path, "a");
    if (!f) return 3;
    n = fwrite(record, 1, strlen(record), f);
    if (n != strlen(record)) { fclose(f); return 4; }
    if (fputc('\n', f) == EOF) { fclose(f); return 5; }
    /* fflush before fclose so a crash between them cannot lose the record
     * silently — an audit trail that loses its tail is not an audit trail. */
    if (fclose(f) != 0) return 6;
    return 0;
}

/* ── read ───────────────────────────────────────────────────────────────── */

typedef struct {
    char **lines;     /* owned */
    size_t count;
    size_t bad;       /* lines that were not parseable JSON objects */
} session_records;

void session_free(session_records *r) {
    size_t i;
    if (!r) return;
    for (i = 0; i < r->count; i++) free(r->lines[i]);
    free(r->lines);
    r->lines = NULL;
    r->count = 0;
    r->bad = 0;
}

/*
 * Read every record. Blank lines are skipped (a trailing newline is normal),
 * but a line that is present and malformed is COUNTED, not discarded — the
 * caller gets both the good records and the number that failed, so it can
 * decide whether a gap is acceptable rather than never learning about it.
 */
session_records session_read(const char *path) {
    session_records out;
    FILE *f;
    char *buf;
    const size_t CAP = 1u << 20;
    memset(&out, 0, sizeof(out));

    f = fopen(path, "r");
    if (!f) return out;
    buf = (char *)malloc(CAP);
    if (!buf) { fclose(f); return out; }

    while (fgets(buf, (int)CAP, f)) {
        size_t len = strlen(buf);
        char err[128];
        jvalue *v;
        char **grown;
        while (len && (buf[len-1] == '\n' || buf[len-1] == '\r')) buf[--len] = '\0';
        if (!len) continue;
        v = json_parse(buf, len, err, sizeof(err));
        if (!v || v->kind != J_OBJ) {
            /* Counted, not dropped in silence. */
            out.bad++;
            jfree(v);
            continue;
        }
        jfree(v);
        grown = (char **)realloc(out.lines, (out.count + 1) * sizeof(char *));
        if (!grown) break;
        out.lines = grown;
        out.lines[out.count] = (char *)malloc(len + 1);
        if (!out.lines[out.count]) break;
        memcpy(out.lines[out.count], buf, len + 1);
        out.count++;
    }
    free(buf);
    fclose(f);
    return out;
}

/* ── self-test ──────────────────────────────────────────────────────────── */

static int failures = 0;
static void expect(int cond, const char *what) {
    if (!cond) { printf("FAIL %s\n", what); failures++; }
}

static void rm_file(const char *p) { remove(p); }

/* Non-static so session_cli.c can call it across files while the assertions
 * stay next to the code they check. Returns 0 when all cases pass. */
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
