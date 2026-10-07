/*
 * edit_cli.c — the `main` for edit.c, moved out so edit.c can be a library.
 * Same rule as every other module here; see render_cli.c for the failure that
 * established it (three mains in one link).
 *
 * CLI takes SUBCOMMANDS, not dash-options (unisacc reserves the dashes).
 */
#include <stdio.h>
#include <string.h>

typedef struct { int ok; int err; long count; long bytes; } edit_result;

edit_result edit_replace(const char *path, const char *old_text,
                         const char *new_text);
int         edit_run_selftest(void);

int main(int argc, char **argv) {
    const char *cmd = argc > 1 ? argv[1] : "";
    edit_result r;

    if (!strcmp(cmd, "selftest")) return edit_run_selftest();

    if (!strcmp(cmd, "replace")) {
        /* usage: edit_cli replace <path> <old> <new> */
        r = edit_replace(argc > 2 ? argv[2] : "",
                         argc > 3 ? argv[3] : "",
                         argc > 4 ? argv[4] : "");
        if (r.ok) {
            printf("ok 1 replacement\n");
            return 0;
        }
        /* The two interesting refusals are NAMED with their counts, because a
         * caller that only sees an exit code cannot tell "your context was
         * wrong" from "the file is genuinely ambiguous" — and those want
         * different next moves. */
        if (r.err == -2001) {
            printf("error not found\n");
        } else if (r.err == -2002) {
            printf("error appears %ld times, need more context\n", r.count);
        } else {
            printf("error %d\n", r.err);
        }
        return 1;
    }

    printf("usage: edit_cli selftest | replace <path> <old> <new>\n");
    return 64;
}

#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

#define FILE_TOO_BIG (-1001)
#define EDIT_NOT_FOUND  (-2001)
#define EDIT_NOT_UNIQUE (-2002)
#define EDIT_BUF_MAX (1024L * 1024L)

typedef struct { int ok; int err; long bytes; } file_result;
file_result file_read(const char *path, char *buf, size_t cap);
file_result file_write(const char *path, const char *text, size_t len);

/* ── self-test ──────────────────────────────────────────────────────────── */

static int failures = 0;
static void expect(int cond, const char *what) {
    if (!cond) { printf("FAIL %s\n", what); failures++; }
}

/* Read a file back through file_read and compare byte for byte. Returns 1 on
 * an exact match. Used to assert "the rest of the file is untouched" without
 * trusting strlen on binary-ish content. */
static int file_is(const char *path, const char *want) {
    char buf[65536];
    file_result fr = file_read(path, buf, sizeof buf);
    if (!fr.ok) return 0;
    return fr.bytes == (long)strlen(want) && !memcmp(buf, want, (size_t)fr.bytes + 1);
}

static void run_selftest(void) {
    const char *base = "/tmp/cdsh-edit-selftest";
    char path[512];
    edit_result r;
    file_result fr;

    mkdir(base, 0755);

    /* --- the happy path: exactly one match ------------------------------ */
    snprintf(path, sizeof path, "%s/one.txt", base);
    fr = file_write(path, "alpha beta gamma\n", 17);
    expect(fr.ok == 1, "the fixture writes");

    r = edit_replace(path, "beta", "BETA");
    expect(r.ok == 1, "a unique match succeeds");
    expect(r.count == 1, "and reports exactly one occurrence");
    expect(r.bytes == 17, "same length: bytes written equals the file size");
    expect(file_is(path, "alpha BETA gamma\n"), "only the matched text changed");

    /* --- replacement SHORTER than the match ----------------------------- */
    r = edit_replace(path, "BETA ", "");
    expect(r.ok == 1, "replacing with empty text (a deletion) succeeds");
    expect(file_is(path, "alpha gamma\n"), "the shorter result is exact");

    /* --- replacement LONGER than the match ------------------------------ */
    r = edit_replace(path, "gamma", "a very long replacement indeed");
    expect(r.ok == 1, "replacing with longer text succeeds");
    expect(file_is(path, "alpha a very long replacement indeed\n"),
           "the longer result is exact");

    /* --- 0 occurrences: FAIL, and do not touch the file ----------------- */
    r = edit_replace(path, "NOWHERE-IN-THIS-FILE", "x");
    expect(r.ok == 0, "a missing pattern fails");
    expect(r.err == EDIT_NOT_FOUND, "and says specifically not-found");
    expect(r.count == 0, "and reports zero occurrences");
    expect(file_is(path, "alpha a very long replacement indeed\n"),
           "and the file is byte-for-byte unchanged after the failure");

    /* --- >1 occurrences: FAIL, name the count, do not touch the file ---- */
    snprintf(path, sizeof path, "%s/two.txt", base);
    (void)file_write(path, "x\ny\nx\ny\nx\n", 10);
    r = edit_replace(path, "x", "Z");
    expect(r.ok == 0, "an ambiguous pattern fails");
    expect(r.err == EDIT_NOT_UNIQUE, "and says specifically not-unique");
    expect(r.count == 3, "and reports HOW MANY occurrences, not just 'many'");
    expect(file_is(path, "x\ny\nx\ny\nx\n"),
           "and NOTHING was rewritten — no partial edit on a refusal");

    /* --- ambiguity is resolved by quoting more context ------------------ */
    /* "x\ny\nx" would NOT work: it matches at offset 0 and again at offset 4,
     * so it is ambiguous too. The context has to reach a byte that only the
     * intended span has — here, the leading "x\ny\nx\ny\nx" distinguishes
     * the third x by its Y. This is exactly the feedback the count gives a
     * caller: 3 occurrences, add context, retry. */
    r = edit_replace(path, "y\nx\ny", "y\nZ\ny");
    expect(r.ok == 1, "more context makes it unique");
    expect(file_is(path, "x\ny\nZ\ny\nx\n"), "and replaces the intended span");

    /* --- the rest of the file is byte-for-byte identical ---------------- */
    /* A larger fixture whose single edit is deep in the middle: any off-by-one
     * in the tail shift shows up as a changed byte far from the edit. */
    snprintf(path, sizeof path, "%s/rest.txt", base);
    (void)file_write(path, "line0\nline1\nline2\nTARGET\nline4\nline5\n", 37);
    r = edit_replace(path, "TARGET", "X");
    expect(r.ok == 1, "a middle-of-file edit succeeds");
    expect(file_is(path, "line0\nline1\nline2\nX\nline4\nline5\n"),
           "every byte outside the match is preserved");

    /* --- a file that does not exist: FAIL, and do not create it --------- */
    snprintf(path, sizeof path, "%s/no-such-file.txt", base);
    remove(path);
    r = edit_replace(path, "a", "b");
    expect(r.ok == 0, "editing a missing file fails");
    expect(r.err == ENOENT, "and reports ENOENT, not a made-up reason");
    {
        struct stat st;
        expect(stat(path, &st) != 0, "and did NOT create the file as a side effect");
    }

    /* --- an empty pattern is refused rather than counted forever -------- */
    snprintf(path, sizeof path, "%s/one.txt", base);
    r = edit_replace(path, "", "x");
    expect(r.ok == 0 && r.err == EINVAL, "an empty pattern is refused");

    /* --- a directory is not editable ------------------------------------ */
    /* The reason this is here and not left implicit: read-then-write on a
     * directory is the shape that one day replaces a directory with a file.
     * The stat guard rejects it before file_read can report EISDIR, so the
     * assertion is on EINVAL (our refusal) rather than on the OS's. */
    r = edit_replace(base, "a", "b");
    expect(r.ok == 0 && r.err == EINVAL, "a directory target is refused");

    /* --- too big to hold: refuse, name the size, do not touch the file --- */
    /* file_read hands back the real size in `bytes`; edit_replace must pass it
     * through, because it is the number a caller needs to decide whether to
     * retry with a bigger buffer or give up. The file must be UNCHANGED —
     * a too-big file is not "mostly editable". */
    snprintf(path, sizeof path, "%s/huge.txt", base);
    {
        static char big[EDIT_BUF_MAX + 64];
        size_t i;
        file_result wr;
        for (i = 0; i < sizeof big - 1; i++) big[i] = 'A';
        big[sizeof big - 1] = '\0';
        wr = file_write(path, big, sizeof big - 1);
        expect(wr.ok == 1, "a too-big fixture can be written");
    }
    r = edit_replace(path, "A", "B");
    expect(r.ok == 0, "a file larger than the buffer is refused");
    expect(r.err == FILE_TOO_BIG, "with file_read's FILE_TOO_BIG reason intact");
    expect(r.bytes == (long)(EDIT_BUF_MAX + 63),
           "and the real size, so a caller can size a retry");
    {
        static char check[1024];
        file_result fr2 = file_read(path, check, sizeof check);
        /* Too big to read, but the first bytes show the file still starts with
         * 'A', i.e. no B replaced an A on the refusal path. */
        expect(fr2.ok == 0 && fr2.err == FILE_TOO_BIG,
               "the refused file was not rewritten");
    }

    /* --- no temp file survives a successful edit ------------------------ */
    {
        struct stat st;
        char tmp[512];
        snprintf(tmp, sizeof tmp, "%s/rest.txt.csih-tmp", base);
        expect(stat(tmp, &st) != 0, "no .csih-tmp file is left behind");
    }

    /* --- a no-op edit (new == old) is still exactly one replacement ----- */
    snprintf(path, sizeof path, "%s/same.txt", base);
    (void)file_write(path, "abc", 3);
    r = edit_replace(path, "abc", "abc");
    expect(r.ok == 1, "replacing text with itself succeeds");
    expect(file_is(path, "abc"), "and leaves the file identical");

    /* clean up */
    remove("/tmp/cdsh-edit-selftest/one.txt");
    remove("/tmp/cdsh-edit-selftest/two.txt");
    remove("/tmp/cdsh-edit-selftest/rest.txt");
    remove("/tmp/cdsh-edit-selftest/same.txt");
    remove("/tmp/cdsh-edit-selftest/huge.txt");
    rmdir(base);
}

int edit_run_selftest(void) {
    run_selftest();
    printf("%s\n", failures ? "SELFTEST FAILED" : "selftest ok");
    return failures == 0 ? 0 : 1;
}
