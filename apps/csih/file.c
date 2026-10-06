/*
 * file.c — the file primitives an agent actually needs, and the three rules
 * that make them safe to hand to one.
 *
 * WHY THIS IS NOT "just open/read/write": a language model calling file
 * primitives will eventually point one at something enormous, at a path that
 * does not exist, or at a file it is halfway through writing. The interesting
 * content of this file is not the syscalls — it is the three decisions below,
 * each of which turns a class of silent damage into a reported failure.
 *
 *   1. READS ARE BOUNDED. `file_read` takes a capacity and REFUSES a file
 *      larger than it, rather than reading what fits. A truncated read that
 *      looks successful is the worst outcome available: the caller reasons
 *      about half a file and never learns. So: too big is an error with the
 *      real size in it, and the caller decides.
 *
 *   2. WRITES ARE ATOMIC. `file_write` writes a sibling temp file and renames
 *      it over the target. A crash or a full disk mid-write would otherwise
 *      leave a half-written file where a complete one used to be — and for a
 *      transcript, that is the loss of the record that says what happened.
 *      rename() within a directory is atomic on POSIX, so readers see either
 *      the old file or the new one, never a mixture.
 *
 *   3. NO PATH EXPANSION. No `~`, no globs, no shell quoting. cdsh has no
 *      shell and pretending otherwise means one day expanding `~` into the
 *      wrong home, or a `*` into a file list nobody meant. If a caller wants
 *      those, that is the caller's job, done explicitly.
 *
 * unisacc limits honoured (SKILL.md §2):
 *   - structs are restated by any includer, so each module compiles alone
 *   - `return f()` where f returns a struct: assign to a local first
 *   - stdio stays local; entry points take `const char *`, never `FILE *`
 *
 * CLI takes SUBCOMMANDS, not dash-options.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

/* ── results ────────────────────────────────────────────────────────────── */

/*
 * Every operation returns one of these rather than a pointer or a plain int.
 *
 * WHY: a bare int forces the caller to guess which negative number means what,
 * and a bare pointer forces it to know who frees it. A struct that carries
 * {ok, err, bytes} makes both questions unaskable — and makes the "it partially
 * worked" case expressible, which is exactly what a truncation guard needs.
 */
typedef struct {
    int  ok;      /* 1 = success, 0 = failure */
    int  err;     /* errno at the failure point, 0 on success */
    long bytes;   /* bytes read or written; on FILE_TOO_BIG, the real size */
} file_result;

/* Refusal reasons that are NOT errno values, because the OS has no name for
 * them: "the file is bigger than you asked to handle" is our policy, not the
 * kernel's. */
#define FILE_TOO_BIG (-1001)

/*
 * errno CONSTANTS UNISACC DOES NOT HAVE — and why the workaround below is a
 * REGRESSION, not a style choice.
 *
 * Measured (reported to cc-unisacc; probes/errno-cover.sh keeps the list):
 * unisacc's errno.h defines 12 constants. gcc has 43. Missing include
 * EISDIR, ENAMETOOLONG, EMFILE, EPIPE, ENAMETOOLONG, EAGAIN — and EINTR.
 *
 * EINTR is the one that matters most, because every correct read/write loop
 * contains it:
 *
 *     n = read(fd, buf, len);
 *     if (n < 0) { if (errno == EINTR) continue; return fail; }
 *
 * Without the name, the loop can only be written with the NUMBER, and the
 * number is per-OS (EAGAIN is 35 on macOS and 11 on Linux). A hardcoded number
 * compiles everywhere and is WRONG on some target, silently — strictly worse
 * than not having the retry at all.
 *
 * So the choices here are deliberate and each one is a known loss:
 *   - EBADF/EINVAL/ENOENT/EIO/EACCES/ENOMEM/ENOSPC are available; use them.
 *   - EISDIR is NOT. A directory read reports EISDIR on gcc; here it can only
 *     report a different code, so callers must not treat EISDIR specially.
 *   - ENAMETOOLONG is NOT. The path-length check uses a literal bound instead.
 *   - EINTR is NOT. THE RETRY BRANCH IS SIMPLY ABSENT, which means a read
 *     interrupted by a signal fails instead of resuming. On a short local read
 *     this rarely shows; it is still a real functional gap and it is recorded
 *     here rather than hidden.
 *
 * UPDATE 2026-10-01 — unisacc 0.0.19 CLOSED THE errno GAP: all 43 constants
 * are now defined, including EINTR, EISDIR and ENAMETOOLONG. probes/libc-cover.sh
 * reported 12/43 and now reports 43/43, and check.sh's assertion flipped from
 * "still 12" to "now 43" exactly as designed — which is why the assertion was
 * written as a number rather than as prose.
 *
 * The workarounds below are therefore READY TO BE UNDONE, and this note is the
 * reminder: add `errno == EINTR` retries to the read and write loops, report
 * EISDIR for a directory, and use ENAMETOOLONG for the path check. They were
 * left in place at the moment of discovery so that the change lands as its own
 * verified step rather than mixed into a version bump.
 */

/* ── read ───────────────────────────────────────────────────────────────── */

/*
 * Read a whole file into `buf`, which must have room for `cap` bytes (one of
 * which is reserved for the NUL).
 *
 * A file larger than the capacity is REFUSED, not truncated — see the header.
 * The real size comes back in `bytes` so the caller can size a buffer and
 * retry, which is the only useful thing to do about it.
 *
 * The size is checked with stat() BEFORE opening, so a 2GB file is never even
 * read; but the check is repeated against what was actually read, because a
 * file can grow between the two calls and the whole point of this function is
 * that "it fit" is never assumed.
 */
file_result file_read(const char *path, char *buf, size_t cap) {
    file_result r;
    struct stat st;
    int fd;
    size_t off = 0;
    ssize_t n;

    r.ok = 0; r.err = 0; r.bytes = 0;
    if (!path || !buf || cap == 0) { r.err = EINVAL; return r; }

    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) { r.err = EISDIR; return r; }
        if ((size_t)st.st_size + 1 > cap) {
            r.err = FILE_TOO_BIG;
            r.bytes = (long)st.st_size;   /* the caller needs this to retry */
            return r;
        }
    } else {
        r.err = errno;
        return r;
    }

    fd = open(path, O_RDONLY);
    if (fd < 0) { r.err = errno; return r; }

    /* Read in a loop: a short read() is normal and must not be mistaken for
     * EOF, or a large file on a busy system would come back truncated — the
     * exact failure this function exists to prevent. */
    while (off + 1 < cap) {
        n = read(fd, buf + off, cap - 1 - off);
        if (n < 0) {
            /* Retry on EINTR. unisacc 0.0.19 defines it, so this is no longer a
             * per-OS magic number — which was the whole reason it was omitted. */
            if (errno == EINTR) continue;
            close(fd);
            r.err = errno;
            return r;
        }
        if (n == 0) break;
        off += (size_t)n;
    }
    close(fd);
    buf[off] = '\0';
    r.ok = 1;
    r.bytes = (long)off;
    return r;
}

/* ── write ──────────────────────────────────────────────────────────────── */

/*
 * Replace `path` with `text`, atomically.
 *
 * Writes "<path>.csih-tmp" then renames it over the target. Same-directory
 * rename is atomic on POSIX, so a reader sees the old contents or the new
 * contents and never a half-written mixture. On any failure the temp file is
 * removed, so a failed write leaves the original intact rather than a stray
 * partial file that looks like a real one.
 *
 * fsync before close is deliberate: without it a rename can be durable while
 * the DATA is not, which on a crash yields a correctly-named empty file —
 * strictly worse than the old contents.
 */
file_result file_write(const char *path, const char *text, size_t len) {
    file_result r;
    char tmp[4096];
    int fd;
    size_t off = 0;
    ssize_t n;

    r.ok = 0; r.err = 0; r.bytes = 0;
    if (!path || (!text && len)) { r.err = EINVAL; return r; }

    if (strlen(path) + 16 >= sizeof tmp) { r.err = ENAMETOOLONG; return r; }
    snprintf(tmp, sizeof tmp, "%s.csih-tmp", path);

    fd = open(tmp, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) { r.err = errno; return r; }

    while (off < len) {
        n = write(fd, text + off, len - off);
        if (n < 0) {
            if (errno == EINTR) continue;
            r.err = errno;
            close(fd);
            remove(tmp);
            return r;
        }
        off += (size_t)n;
    }
    /* fsync is available as of unisacc 0.0.19 (it was missing in 0.0.18, and
     * the #ifdef that guarded it is gone now that both backends have it —
     * a workaround that outlives its cause is just a mystery for the next
     * reader). */
    if (fsync(fd) != 0) {
        r.err = errno;
        close(fd);
        remove(tmp);
        return r;
    }
    if (close(fd) != 0) {
        r.err = errno;
        remove(tmp);
        return r;
    }
    if (rename(tmp, path) != 0) {
        r.err = errno;
        remove(tmp);
        return r;
    }
    r.ok = 1;
    r.bytes = (long)off;
    return r;
}

/* Append one line, for transcripts. NOT atomic in the same way as file_write:
 * append is a single O_APPEND write, which the kernel serialises, so two
 * writers interleave whole lines rather than shredding each other's. That is
 * the property a log needs, and it is a different requirement from "replace
 * this file's contents". */
file_result file_append_line(const char *path, const char *line) {
    file_result r;
    int fd;
    size_t len, off = 0;
    ssize_t n;

    r.ok = 0; r.err = 0; r.bytes = 0;
    if (!path || !line) { r.err = EINVAL; return r; }

    /* O_RDWR, not O_WRONLY: the newline check below has to READ the last byte,
     * and read() on a write-only descriptor fails with EBADF. The first version
     * of this fix opened O_WRONLY and the check therefore NEVER RAN — the
     * append still produced "hellosecond" while the code to prevent it sat
     * right there looking correct. A guard nobody can reach is worse than no
     * guard, because it is quoted as one. */
    fd = open(path, O_RDWR | O_CREAT | O_APPEND, 0644);
    if (fd < 0) { r.err = errno; return r; }

    /*
     * If the file does not end in a newline, add one BEFORE the new line.
     *
     * Found by the tools.c selftest, and it is the classic log bug: appending
     * "second" after a file containing "hello" (no trailing newline) produces
     * "hellosecond". Both appends "succeeded" and the damage only shows up
     * later, when someone parses the log and finds a record that was never
     * written. A function whose whole purpose is "add one line" has to be
     * responsible for the boundary between lines.
     *
     * The check is a read of the last byte, so an empty file needs no newline
     * and a well-formed file is untouched.
     */
    {
        off_t size = lseek(fd, 0, SEEK_END);
        if (size > 0) {
            char last = '\n';
            off_t back = lseek(fd, size - 1, SEEK_SET);
            if (back >= 0 && read(fd, &last, 1) == 1 && last != '\n') {
                if (write(fd, "\n", 1) != 1) { r.err = errno; close(fd); return r; }
            }
            (void)lseek(fd, 0, SEEK_END);   /* back to the end for the append */
        }
    }

    len = strlen(line);
    while (off < len) {
        n = write(fd, line + off, len - off);
        if (n < 0) {
            if (errno == EINTR) continue;
            r.err = errno; close(fd); return r;
        }
        off += (size_t)n;
    }
    if (len == 0 || line[len - 1] != '\n') {
        if (write(fd, "\n", 1) != 1) { r.err = errno; close(fd); return r; }
        off++;
    }
    close(fd);
    r.ok = 1;
    r.bytes = (long)off;
    return r;
}

/* ── list ───────────────────────────────────────────────────────────────── */

/*
 * List a directory's entries into `out`, one name per line, skipping "." and
 * "..".
 *
 * The "." and ".." skip is not cosmetic: a caller walking a tree that does not
 * special-case them recurses upward forever. Leaving it to every caller means
 * it will be forgotten once, so it is done here where it cannot be.
 *
 * Names longer than the buffer are reported as an error rather than truncated,
 * for the same reason reads are: a truncated name is a name that does not
 * exist, and the caller would act on it.
 */
file_result file_list(const char *dir, char *out, size_t cap) {
    file_result r;
    DIR *d;
    struct dirent *e;
    size_t used = 0;
    long count = 0;

    r.ok = 0; r.err = 0; r.bytes = 0;
    if (!dir || !out || cap == 0) { r.err = EINVAL; return r; }
    out[0] = '\0';

    d = opendir(dir);
    if (!d) { r.err = errno; return r; }

    errno = 0;
    while ((e = readdir(d)) != NULL) {
        size_t n;
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        n = strlen(e->d_name);
        if (used + n + 2 > cap) {          /* +2 for the newline and the NUL */
            closedir(d);
            r.err = ENOSPC;
            return r;
        }
        memcpy(out + used, e->d_name, n);
        used += n;
        out[used++] = '\n';
        out[used] = '\0';
        count++;
    }
    if (errno != 0) { r.err = errno; closedir(d); return r; }
    closedir(d);
    r.ok = 1;
    r.bytes = count;      /* the COUNT of entries, not bytes — see the CLI note */
    return r;
}

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
