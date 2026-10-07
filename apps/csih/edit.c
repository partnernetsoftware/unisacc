/*
 * edit.c — the exact-match replace that dsh's own use of it implies.
 *
 * WHY THIS IS NOT "read, strstr, overwrite": that is the one-line version, and
 * every failure it has is silent. A model that calls edit with an `old` it
 * mis-remembered gets a no-op that reads exactly like success; one that picks
 * an `old` that appears twice gets the FIRST occurrence rewritten, which is
 * the second-hardest bug in the file to notice (the first being a partial
 * write). So the whole content of this module is a decision about when to
 * REFUSE:
 *
 *   - 0 occurrences  → fail. There is nothing to replace, and "I edited it"
 *                      is a lie the caller will build on.
 *   - >1 occurrence  → fail, and say HOW MANY. Rewriting all of them guesses
 *                      that the caller wanted every one; rewriting the first
 *                      guesses which. Both guesses are invisible when wrong,
 *                      and the fix (quote more context) is the caller's to
 *                      make because only the caller knows the file.
 *   - exactly 1      → replace, write back atomically through file_write().
 *
 * The count is reported as a NUMBER so the caller can act on it: "3 times" is
 * a fact you can extend context for; "not unique" is a shrug.
 *
 * NOT A CREATOR. A missing file fails. Creating an empty file and then failing
 * to find anything in it would leave litter behind on an error path, and
 * `write` is the tool whose job that is — a tool that makes files as a side
 * effect of failing to edit them blurs the two.
 *
 * TEXT, NOT BYTES. Matching and measuring go through strstr/strlen, so a NUL
 * byte in the file ends the view: everything after it is invisible to the
 * match and would be dropped by the write-back. This is a REAL limit, stated
 * rather than discovered later — a whole-file byte editor needs lengths
 * threaded through every step (match, splice, write) and that is a different
 * function. For the files this tool exists for (source, transcripts, configs)
 * a NUL is not a legitimate byte, so the honest boundary is to say so here.
 *
 * unisacc limits honoured (SKILL.md §2):
 *   - file_result is RESTATED here, exactly as in tools.c and file_cli.c, so
 *     each of them compiles on its own (unisacc can see an earlier file's type,
 *     but that is order-dependent). The duplication is the rule, not an oversight
 *   - no `return f()` where f returns a struct: every result is assigned to a
 *     local first
 *   - stdio stays in the CLI; entry points take `const char *`, never FILE *
 *   - no `main` here (edit_cli.c owns it; unisacc links one program, one main)
 *
 * CLI takes SUBCOMMANDS, not dash-options (unisacc reserves the dashes).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

/* ── restated (so this file compiles alone) ─────────────────────────────── */

typedef struct {
    int  ok;      /* 1 = success, 0 = failure */
    int  err;     /* errno at the failure point, 0 on success */
    long bytes;   /* bytes read or written; on EDIT_TOO_BIG, the real size */
} file_result;

file_result file_read(const char *path, char *buf, size_t cap);
file_result file_write(const char *path, const char *text, size_t len);

/*
 * file.c's policy code for "the file did not fit the buffer you gave me",
 * RESTATED here for the same reason file_result is: unisacc shares no symbols
 * or types across translation units. The value must stay in step with file.c's
 * or edit_replace would translate a too-big read into a generic error and lose
 * the real size; the selftest asserts the pass-through, so a drift breaks the
 * test rather than the caller.
 */
#define FILE_TOO_BIG (-1001)

/* ── results ────────────────────────────────────────────────────────────── */

/*
 * edit_result carries {ok, err, count} rather than only an exit status.
 *
 * `count` is the number of occurrences found. On the >1 failure it is what
 * tells the caller how much more context to add, which is the only useful
 * response to that failure. On success it is always 1 — kept rather than
 * dropped so that a caller can assert the invariant it was promised instead of
 * trusting it.
 *
 * `bytes` mirrors file.c's meaning: the size of the file AFTER the edit, so a
 * caller can watch a file grow from the outside.
 */
typedef struct {
    int  ok;
    int  err;     /* errno, or one of the EDIT_* reasons below */
    long count;   /* occurrences of `old` found (0, 1, or N) */
    long bytes;   /* bytes written back to the file */
} edit_result;

/*
 * Refusal reasons that are NOT errno values, for the same reason file.c has
 * FILE_TOO_BIG: "the text is not unique" is our policy, and the kernel has no
 * name for it. Distinct codes, not one generic EDIT_FAILED, because a caller
 * retrying on "not found" (context was wrong) must not retry on "ambiguous"
 * (the file itself needs a look).
 */
#define EDIT_NOT_FOUND  (-2001)   /* `old` appears 0 times */
#define EDIT_NOT_UNIQUE (-2002)   /* `old` appears more than once */
#define EDIT_TOO_BIG    (-2003)   /* file does not fit the buffer we hold */

/*
 * Buffer sizes. The read cap is a POLICY ceiling, and it is deliberately
 * generous-but-finite: csih is for source files and transcripts, not for
 * rewriting a database. A file larger than this is REFUSED with its real size
 * in `bytes` (file_read reports it) rather than partially rewritten — see
 * file.c's first rule, which this module inherits rather than re-decides.
 */
#define EDIT_BUF_MAX (1024L * 1024L)

/* ── the edit ───────────────────────────────────────────────────────────── */

/*
 * Replace the single occurrence of `old` in `path` with `new`.
 *
 * Atomicity comes from file_write(): the new contents are written to a sibling
 * temp file and renamed over the target, so an interrupted edit leaves the
 * ORIGINAL file intact. That matters more here than anywhere: a partial edit
 * is a file that is neither the old version nor the new one, and it looks like
 * a valid file to everything downstream.
 */
edit_result edit_replace(const char *path, const char *old_text,
                         const char *new_text) {
    edit_result r;
    static char buf[EDIT_BUF_MAX];
    char *out;
    const char *scan;
    size_t old_len, new_len, tail_len, out_len;
    long count = 0;
    struct stat st;
    file_result fr;

    r.ok = 0; r.err = 0; r.count = 0; r.bytes = 0;

    if (!path || !old_text || !new_text) { r.err = EINVAL; return r; }
    old_len = strlen(old_text);
    new_len = strlen(new_text);

    /*
     * An EMPTY `old` is refused here rather than left to the counting loop.
     * "" occurs at every position (including between every pair of bytes), so
     * the loop would report a count that grows with the file and the caller
     * would read a huge number with no idea why. "Replace nothing with
     * something" has no meaning; naming it beats counting it.
     */
    if (old_len == 0) { r.err = EINVAL; return r; }

    /*
     * NOT-A-CREATOR, enforced before the read so the error is ENOENT and not
     * something dressed up by file_read. stat, not access: access can lie
     * about what a later open will do under a different effective identity.
     */
    if (stat(path, &st) != 0) { r.err = errno; return r; }
    if (!S_ISREG(st.st_mode)) { r.err = EINVAL; return r; }

    fr = file_read(path, buf, sizeof buf);
    if (!fr.ok) {
        /*
         * Report file_read's reason unchanged, including FILE_TOO_BIG's real
         * size in `bytes`. Translating it into a generic "cannot read" would
         * throw away the one number the caller needs to size a bigger attempt.
         */
        r.err = fr.err;
        r.bytes = fr.bytes;
        return r;
    }

    /* ── count, do not guess ─────────────────────────────────────────────
     *
     * Scan the WHOLE file before changing anything. The alternative — replace
     * as you go and stop at the second match — leaves the file already
     * rewritten on the failure path, which is the one outcome an invariant
     * check must never produce.
     */
    scan = buf;
    for (;;) {
        const char *hit = strstr(scan, old_text);
        if (!hit) break;
        count++;
        if (count > 1) break;      /* two is enough to refuse; do not count on */
        scan = hit + 1;            /* +1, not +old_len: overlapping matches are
                                    * still matches, and skipping a full
                                    * pattern width could miss them */
    }

    if (count == 0) { r.err = EDIT_NOT_FOUND; return r; }
    if (count > 1) {
        /*
         * Re-count accurately for the message. The loop above stops at 2
         * because the DECISION does not depend on the exact number, but the
         * REPORT does: "appears 7 times" tells the caller how much context to
         * add, "appears more than once" does not.
         */
        count = 0;
        scan = buf;
        for (;;) {
            const char *hit = strstr(scan, old_text);
            if (!hit) break;
            count++;
            scan = hit + 1;
        }
        r.err = EDIT_NOT_UNIQUE;
        r.count = count;
        return r;
    }

    /* ── exactly one: build the new contents, then write once ──────────── */

    tail_len = strlen(buf) - (size_t)(strstr(buf, old_text) - buf) - old_len;
    out_len = strlen(buf) - old_len + new_len;
    if (out_len + 1 > sizeof buf) { r.err = EDIT_TOO_BIG; r.bytes = (long)out_len; return r; }

    /*
     * The output buffer is the file buffer reused, and the copy is ordered so
     * that overlap is safe: the head does not move, and the tail is shifted
     * only after being located relative to the ORIGINAL — so a `new` longer or
     * shorter than `old` both work. Copying the tail forward when it moves
     * right would corrupt it (source and destination overlap), hence memmove.
     */
    out = buf;
    {
        char *at = strstr(buf, old_text);
        size_t head_len = (size_t)(at - buf);
        memmove(out + head_len + new_len, at + old_len, tail_len);
        memcpy(out + head_len, new_text, new_len);
        out[out_len] = '\0';
    }

    fr = file_write(path, out, out_len);
    if (!fr.ok) { r.err = fr.err; return r; }

    r.ok = 1;
    r.err = 0;
    r.count = 1;
    r.bytes = fr.bytes;
    return r;
}

/* edit_cli.c owns the self-test. */
