/*
 * chat_cli.c — the `main` for chat.c, moved out so chat.c can be a library.
 *
 * Same rule as every other module here (gate/json/session/render/term): unisacc
 * runs a source as a program, a program has one `main`, so a module that keeps
 * its own cannot be linked with any other. chat.c is meant to be linked into
 * the TUI, so its `main` lives here.
 *
 * CLI takes SUBCOMMANDS, not dash-options (unisacc reserves the dashes).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int    ok;
    int    cont;
    double p;
    char   verdict[192];
} chat_turn;

chat_turn chat_turn_run(const char *path, const char *user_text, double threshold);
int       chat_note_p(const char *path, double p);
int       chat_run_selftest(void);

int main(int argc, char **argv) {
    const char *cmd = argc > 1 ? argv[1] : "";
    if (!strcmp(cmd, "selftest")) return chat_run_selftest();
    if (!strcmp(cmd, "say")) {
        /* usage: chat_cli say <transcript> <threshold> <text> */
        chat_turn t;
        const char *path = argc > 2 ? argv[2] : "/tmp/cdsh-chat.jsonl";
        double thr = argc > 3 ? atof(argv[3]) : 0.5;
        const char *text = argc > 4 ? argv[4] : "";
        t = chat_turn_run(path, text, thr);
        printf("%s\n", t.verdict);
        return t.ok ? 0 : 1;
    }
    if (!strcmp(cmd, "note")) {
        /* usage: chat_cli note <transcript> <p> */
        const char *path = argc > 2 ? argv[2] : "/tmp/cdsh-chat.jsonl";
        double p = argc > 3 ? atof(argv[3]) : 0.0;
        return chat_note_p(path, p) == 0 ? 0 : 1;
    }
    printf("usage: chat_cli selftest | say <transcript> <threshold> <text> | note <transcript> <p>\n");
    return 64;
}

typedef struct {
    char **lines;
    size_t count;
    size_t bad;
} session_records;
session_records session_read(const char *path);
void session_free(session_records *r);

/* ── self-test ──────────────────────────────────────────────────────────── */

static int failures = 0;
static void expect(int cond, const char *what) {
    if (!cond) { printf("FAIL %s\n", what); failures++; }
}

static void run_selftest(void) {
    const char *path = "/tmp/cdsh-chat-selftest.jsonl";
    chat_turn t;
    FILE *f;

    /* start from nothing, so the "first turn has no transcript" path is taken */
    f = fopen(path, "w"); if (f) fclose(f);

    /* A turn with no probability yet must NOT be an error and must NOT decide.
     * "nothing to judge" and "judged zero" lead to opposite verdicts. */
    t = chat_turn_run(path, "hello", 0.5);
    expect(t.ok == 1, "a turn with no probability still completes");
    expect(strstr(t.verdict, "no probability") != NULL, "and says so plainly");

    /* Now a probability arrives, as a model would append it. */
    expect(chat_note_p(path, 0.83) == 0, "a probability can be appended");
    t = chat_turn_run(path, "again", 0.5);
    expect(t.ok == 1, "the second turn completes");
    expect(t.cont == 1, "p=0.83 continues at threshold 0.5");
    expect(t.p > 0.82 && t.p < 0.84, "the gate saw the stored probability");

    /* Below threshold stops. */
    expect(chat_note_p(path, 0.20) == 0, "a second probability can be appended");
    t = chat_turn_run(path, "stop now", 0.5);
    expect(t.cont == 0, "p=0.20 stops at threshold 0.5");

    /* The verdict must be IN the file — a verdict only on screen is not a
     * closed loop, and the next turn could not see it. */
    {
        session_records recs = session_read(path);
        int saw_gate = 0, saw_user = 0, saw_ts = 0;
        size_t i;
        for (i = 0; i < recs.count; i++) {
            if (strstr(recs.lines[i], "\"role\":\"gate\"")) saw_gate = 1;
            if (strstr(recs.lines[i], "\"role\":\"user\"")) saw_user = 1;
            if (strstr(recs.lines[i], "\"ts\":")) saw_ts = 1;
        }
        expect(saw_gate, "the verdict is in the transcript");
        expect(saw_user, "the user line is in the transcript");
        expect(saw_ts, "every record carries a monotonic ts (clock.c is load-bearing)");
        expect(recs.bad == 0, "every line written is valid JSON");
        session_free(&recs);
    }

    /* A quote in the user's text must not corrupt the transcript. This is the
     * failure that would otherwise surface later, as a parse error on a line
     * nobody remembers typing. */
    f = fopen(path, "w"); if (f) fclose(f);
    (void)chat_turn_run(path, "he said \"hi\" \\ and\nnewline", 0.5);
    {
        session_records recs = session_read(path);
        expect(recs.bad == 0, "escaped input still parses as JSON");
        session_free(&recs);
    }

    remove(path);
}

int chat_run_selftest(void) {
    run_selftest();
    printf("%s\n", failures ? "SELFTEST FAILED" : "selftest ok");
    return failures == 0 ? 0 : 1;
}
