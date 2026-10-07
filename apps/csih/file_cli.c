/*
 * file_cli.c — the `main` for file.c, moved out so file.c can be a library.
 * Same rule as every other module here; see render_cli.c for the failure that
 * established it (three mains in one link).
 *
 * CLI takes SUBCOMMANDS, not dash-options (unisacc reserves the dashes).
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct { int ok; int err; long bytes; } file_result;

file_result file_read(const char *path, char *buf, size_t cap);
file_result file_write(const char *path, const char *text, size_t len);
file_result file_append_line(const char *path, const char *line);
file_result file_list(const char *dir, char *out, size_t cap);
int         file_run_selftest(void);

int main(int argc, char **argv) {
    const char *cmd = argc > 1 ? argv[1] : "";
    char buf[65536];
    file_result r;

    if (!strcmp(cmd, "selftest")) return file_run_selftest();

    if (!strcmp(cmd, "read")) {
        r = file_read(argc > 2 ? argv[2] : "", buf, sizeof buf);
        if (!r.ok) { printf("error %d bytes %ld\n", r.err, r.bytes); return 1; }
        fputs(buf, stdout);
        return 0;
    }
    if (!strcmp(cmd, "write")) {
        const char *path = argc > 2 ? argv[2] : "";
        const char *text = argc > 3 ? argv[3] : "";
        r = file_write(path, text, strlen(text));
        printf("%s %ld\n", r.ok ? "ok" : "error", r.bytes);
        return r.ok ? 0 : 1;
    }
    if (!strcmp(cmd, "append")) {
        r = file_append_line(argc > 2 ? argv[2] : "", argc > 3 ? argv[3] : "");
        printf("%s %ld\n", r.ok ? "ok" : "error", r.bytes);
        return r.ok ? 0 : 1;
    }
    if (!strcmp(cmd, "list")) {
        r = file_list(argc > 2 ? argv[2] : ".", buf, sizeof buf);
        if (!r.ok) { printf("error %d\n", r.err); return 1; }
        printf("%ld entries\n", r.bytes);
        fputs(buf, stdout);
        return 0;
    }
    printf("usage: file_cli selftest | read <path> | write <path> <text> | "
           "append <path> <line> | list <dir>\n");
    return 64;
}

#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

#define FILE_TOO_BIG (-1001)

/* ── self-test ──────────────────────────────────────────────────────────── */

static int failures = 0;
static void expect(int cond, const char *what) {
    if (!cond) { printf("FAIL %s\n", what); failures++; }
}

static void run_selftest(void) {
    const char *base = "/tmp/cdsh-file-selftest";
    char path[512], buf[4096];
    file_result r;

    mkdir(base, 0755);

    /* --- write then read: the round trip ------------------------------- */
    snprintf(path, sizeof path, "%s/a.txt", base);
    r = file_write(path, "hello\n", 6);
    expect(r.ok == 1 && r.bytes == 6, "write reports the bytes it wrote");
    r = file_read(path, buf, sizeof buf);
    expect(r.ok == 1 && r.bytes == 6, "read reports the bytes it read");
    expect(!strcmp(buf, "hello\n"), "the content round-trips exactly");

    /* --- the size guard: REFUSE, do not truncate ----------------------- */
    /* This is the decision the whole file turns on. A truncating read would
     * return ok=1 with six bytes of a thirteen-byte file, and the caller would
     * reason about half a document without ever knowing. */
    snprintf(path, sizeof path, "%s/big.txt", base);
    r = file_write(path, "0123456789abc", 13);
    expect(r.ok == 1, "a 13-byte file can be written");
    r = file_read(path, buf, 8);            /* only 8 bytes of room */
    expect(r.ok == 0, "reading a too-large file FAILS rather than truncating");
    expect(r.err == FILE_TOO_BIG, "and says specifically that it was too big");
    expect(r.bytes == 13, "and reports the real size so the caller can retry");

    /* --- a missing file is an error with a reason ---------------------- */
    r = file_read("/tmp/cdsh-file-selftest/definitely-not-here", buf, sizeof buf);
    expect(r.ok == 0, "a missing file fails");
    expect(r.err == ENOENT, "and reports ENOENT");

    /* --- a directory is not a file ------------------------------------- */
    r = file_read(base, buf, sizeof buf);
    expect(r.ok == 0 && r.err == EISDIR, "reading a directory says EISDIR");

    /* --- atomic replace: old contents must not survive ------------------ */
    snprintf(path, sizeof path, "%s/c.txt", base);
    (void)file_write(path, "AAAAAAAAAA", 10);
    (void)file_write(path, "BB", 2);        /* shorter: a stale tail would show */
    r = file_read(path, buf, sizeof buf);
    expect(r.ok == 1, "the replacement is readable");
    expect(!strcmp(buf, "BB"), "a shorter write fully replaces the longer one");

    /* --- the temp file must NOT be left behind ------------------------- */
    {
        char tmp[512];
        struct stat st;
        snprintf(tmp, sizeof tmp, "%s/c.txt.csih-tmp", base);
        expect(stat(tmp, &st) != 0, "no temp file is left after a successful write");
    }

    /* --- listing ------------------------------------------------------- */
    r = file_list(base, buf, sizeof buf);
    expect(r.ok == 1, "a directory can be listed");
    expect(strstr(buf, "a.txt") != NULL, "the listing contains a.txt");
    expect(strstr(buf, "big.txt") != NULL, "the listing contains big.txt");
    expect(strstr(buf, ".\n") == NULL, "the listing skips .");
    expect(strstr(buf, "..\n") == NULL, "the listing skips ..");
    expect(r.bytes >= 3, "the listing reports how many entries it found");

    /* A too-small buffer must fail, not silently return a partial listing —
     * a partial listing is a file set that does not exist. */
    r = file_list(base, buf, 4);
    expect(r.ok == 0, "a too-small listing buffer fails rather than truncating");

    /* --- append -------------------------------------------------------- */
    snprintf(path, sizeof path, "%s/log.jsonl", base);
    remove(path);
    expect(file_append_line(path, "one").ok == 1, "append creates the file");
    expect(file_append_line(path, "two\n").ok == 1, "append adds a second line");
    r = file_read(path, buf, sizeof buf);
    expect(r.ok == 1, "the log is readable");
    expect(!strcmp(buf, "one\ntwo\n"), "each line is newline-terminated exactly once");

    /* --- no path expansion: a literal ~ is a literal directory ---------- */
    /* Guarding this in a test because "helpfully" expanding ~ is the kind of
     * feature that gets added later and breaks the rule silently. */
    r = file_read("~/definitely-not-a-real-path", buf, sizeof buf);
    expect(r.ok == 0, "~ is not expanded (there is no such literal directory)");

    /* clean up */
    remove("/tmp/cdsh-file-selftest/a.txt");
    remove("/tmp/cdsh-file-selftest/big.txt");
    remove("/tmp/cdsh-file-selftest/c.txt");
    remove("/tmp/cdsh-file-selftest/log.jsonl");
    rmdir(base);   /* available as of unisacc 0.0.19 */
}

int file_run_selftest(void) {
    run_selftest();
    printf("%s\n", failures ? "SELFTEST FAILED" : "selftest ok");
    return failures == 0 ? 0 : 1;
}
