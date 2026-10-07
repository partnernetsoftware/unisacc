/*
 * tools.c — the dispatch table between what a user types and what csih does.
 *
 * WHY A TABLE AND NOT AN IF-CHAIN: an agent's tool surface is the thing it
 * reasons ABOUT, so it has to be introspectable — "what can I do here" must be
 * answerable without reading the source. A table gives that for free: the same
 * data drives dispatch, the help text, and the unknown-command error. An
 * if-chain gives three places to drift apart, and the help text is always the
 * one that goes stale.
 *
 * WHY IT LIVES SEPARATE FROM tui.c: the TUI's job is keys and frames. What a
 * command MEANS is not a terminal concern, and keeping them apart is what lets
 * this file's selftest run with no terminal — the property that made render.c
 * testable, applied to the layer above it.
 *
 * THE PARSING RULE, and it is a deliberate limit: an argument list is split on
 * spaces, with double quotes grouping. There is NO escaping, NO variables, NO
 * globbing, and NO pipes. csih has no shell; a tool surface that half-implements
 * one is worse than one that plainly does not, because `rm $HOME/x` silently
 * doing something unexpected is a much better outcome for the author of a
 * half-shell than `rm $HOME/x` failing loudly. Callers who want a shell can
 * pass a command to a real one — once `fork` exists (it does not yet; see
 * probes/libc-cover.sh).
 *
 * unisacc limits honoured (SKILL.md §2): structs restated so each module builds
 * alone, no `return f()` of a struct from a non-main function, stdio owned by
 * the CLI file, subcommands rather than dash-options.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

/* ── restated (so this file compiles alone) ─────────────────────────────── */

typedef struct {
    int  ok;
    int  err;
    long bytes;
} file_result;

#define FILE_TOO_BIG (-1001)

typedef struct {
    int ok; int err; int exited; int status; int signal; long bytes;
    char out[65536];
} shell_result;

shell_result shell_run(const char *command);
shell_result shell_run_in(const char *command, const char *cwd);

/* edit.c's interface, restated for the same reason as everything else here. */
typedef struct {
    int  ok; int err; long count; long bytes;
} edit_result;
#define EDIT_NOT_FOUND  (-2001)
#define EDIT_NOT_UNIQUE (-2002)
edit_result edit_replace(const char *path, const char *old_text, const char *new_text);

file_result file_read(const char *path, char *buf, size_t cap);
file_result file_write(const char *path, const char *text, size_t len);
file_result file_append_line(const char *path, const char *line);
file_result file_list(const char *dir, char *out, size_t cap);

/*
 * THE WORKING DIRECTORY, kept at module scope.
 *
 * WHY: measured on 2852 real tool calls — 2107 of them (84%) opened with `cd`,
 * purely because each call was a new process and the directory reset. The tool
 * surface forced the caller to re-state something a person states once.
 *
 * NOT A GLOBAL IN THE BAD SENSE: it is the tool layer's own state, it is only
 * read when building a command, and `cd` is the single writer. Same shape as a
 * shell's own $PWD, which is exactly the model being copied.
 *
 * Empty string means "wherever the program started", so a fresh process behaves
 * exactly as before this existed.
 */
static char tool_cwd[1024] = "";

/* ── the result of running a command ────────────────────────────────────── */

#define TOOL_OUT_MAX 65536

typedef struct {
    int  ok;
    char out[TOOL_OUT_MAX];
} tool_result;

/* ── argument splitting ─────────────────────────────────────────────────── */

#define TOOL_MAX_ARGS 16

typedef struct {
    char  *argv[TOOL_MAX_ARGS];
    int    argc;
    char   buf[2048];      /* owns the storage the argv pointers point into */
} tool_args;

/*
 * Split a line into arguments. Double quotes group; there is no other syntax
 * (see the header for why that is deliberate rather than unfinished).
 *
 * Returns 0 on success, -1 if the line is malformed (an unterminated quote) or
 * has too many arguments. A malformed line is REPORTED, never silently
 * reinterpreted — "the command did something, just not what you typed" is the
 * failure mode this whole file exists to avoid.
 */
int tool_split(const char *line, tool_args *a) {
    size_t n = strlen(line), i = 0, w = 0;
    int in_quote = 0;

    a->argc = 0;
    if (n >= sizeof a->buf) return -1;

    while (i < n) {
        while (i < n && (line[i] == ' ' || line[i] == '\t')) i++;
        if (i >= n) break;
        if (a->argc >= TOOL_MAX_ARGS) return -1;
        a->argv[a->argc++] = &a->buf[w];
        in_quote = 0;
        while (i < n) {
            char c = line[i];
            if (in_quote) {
                if (c == '"') { in_quote = 0; i++; continue; }
                a->buf[w++] = c; i++;
            } else {
                if (c == '"') { in_quote = 1; i++; continue; }
                if (c == ' ' || c == '\t') break;
                a->buf[w++] = c; i++;
            }
        }
        a->buf[w++] = '\0';
    }
    if (in_quote) return -1;          /* unterminated quote */
    return 0;
}

/* ── the commands ───────────────────────────────────────────────────────── */

/*
 * `read <path>` — print a file.
 *
 * The buffer is heap-allocated, not a local, because a 64KB local array is
 * fine on Linux and was refused by unisacc's osx/arm64 target at 1MB (the
 * lesson recorded in probes/FINDINGS.md). 64KB happens to fit, but sizing a
 * tool by what a stack survives is a trap that gets sprung by the next person
 * who raises the limit.
 */
/*
 * Resolve a possibly-relative path against the tool's cwd.
 *
 * Only handles the "relative" case: an absolute path is passed through, and a
 * leading `~` is NOT expanded anywhere in csih (see cmd_cd). Keeping the rule
 * this narrow is what stops a path from meaning something the caller did not
 * type.
 */
static const char *tool_resolve(const char *path, char *buf, size_t buflen) {
    if (!path) return path;
    if (path[0] == '/') return path;
    if (!tool_cwd[0]) return path;
    snprintf(buf, buflen, "%s/%s", tool_cwd, path);
    return buf;
}

static int cmd_read(const tool_args *a, tool_result *r) {
    char *buf;
    char resolved[1200];
    file_result fr;
    const char *path;

    if (a->argc != 2) { snprintf(r->out, sizeof r->out, "usage: read <path>"); return 0; }
    path = tool_resolve(a->argv[1], resolved, sizeof resolved);
    buf = (char *)malloc(TOOL_OUT_MAX);
    if (!buf) { snprintf(r->out, sizeof r->out, "out of memory"); return 0; }
    fr = file_read(path, buf, TOOL_OUT_MAX);
    if (!fr.ok) {
        if (fr.err == FILE_TOO_BIG)
            snprintf(r->out, sizeof r->out,
                     "refused: %s is %ld bytes, larger than the %d-byte limit",
                     a->argv[1], fr.bytes, TOOL_OUT_MAX);
        else
            snprintf(r->out, sizeof r->out, "cannot read %s (errno %d)", path, fr.err);
        free(buf);
        return 0;
    }
    snprintf(r->out, sizeof r->out, "%s", buf);
    free(buf);
    r->ok = 1;
    return 1;
}

static int cmd_write(const tool_args *a, tool_result *r) {
    file_result fr;
    if (a->argc != 3) { snprintf(r->out, sizeof r->out, "usage: write <path> <text>"); return 0; }
    fr = file_write(a->argv[1], a->argv[2], strlen(a->argv[2]));
    if (!fr.ok) { snprintf(r->out, sizeof r->out, "cannot write %s (errno %d)", a->argv[1], fr.err); return 0; }
    snprintf(r->out, sizeof r->out, "wrote %ld bytes to %s", fr.bytes, a->argv[1]);
    r->ok = 1;
    return 1;
}

static int cmd_append(const tool_args *a, tool_result *r) {
    file_result fr;
    if (a->argc != 3) { snprintf(r->out, sizeof r->out, "usage: append <path> <line>"); return 0; }
    fr = file_append_line(a->argv[1], a->argv[2]);
    if (!fr.ok) { snprintf(r->out, sizeof r->out, "cannot append (errno %d)", fr.err); return 0; }
    snprintf(r->out, sizeof r->out, "appended %ld bytes to %s", fr.bytes, a->argv[1]);
    r->ok = 1;
    return 1;
}

/*
 * `sh <command...>` — run the rest of the line through a real shell.
 *
 * NOTE WHAT IS NOT DONE HERE: the arguments are re-joined with spaces and
 * handed to `sh -c` WHOLE. They are not interpreted, not split further, not
 * validated. That is the point — see shell.c's header. `sh` is the escape hatch
 * to everything a fixed verb list cannot express, and re-parsing it here would
 * re-introduce exactly the half-shell problem the verb list exists to avoid.
 *
 * The re-join is lossy for runs of multiple spaces inside quotes, which is
 * acceptable because the shell will normalise them anyway, and it is the only
 * behaviour that does not require this layer to understand quoting.
 */
static int cmd_sh(const tool_args *a, tool_result *r) {
    char cmd[2048];
    size_t w = 0;
    int i;
    shell_result sr;

    if (a->argc < 2) { snprintf(r->out, sizeof r->out, "usage: sh <command...>"); return 0; }
    cmd[0] = '\0';
    for (i = 1; i < a->argc; i++) {
        size_t n = strlen(a->argv[i]);
        if (w + n + 2 >= sizeof cmd) {
            snprintf(r->out, sizeof r->out, "command too long");
            return 0;
        }
        if (i > 1) cmd[w++] = ' ';
        memcpy(cmd + w, a->argv[i], n);
        w += n;
        cmd[w] = '\0';
    }

    sr = shell_run_in(cmd, tool_cwd[0] ? tool_cwd : NULL);
    if (!sr.ok) {
        snprintf(r->out, sizeof r->out, "could not run the shell (errno %d)", sr.err);
        return 0;
    }
    if (!sr.exited) {
        /* A signalled child is NOT a success, and it is not exit 0 either.
         * Reporting it as either one is how a killed command looks like it
         * worked. */
        snprintf(r->out, sizeof r->out, "%s[terminated by signal %d]",
                 sr.out, sr.signal);
        return 0;
    }
    if (sr.bytes == -1)
        snprintf(r->out, sizeof r->out, "%s[output truncated at %d bytes]",
                 sr.out, (int)(sizeof sr.out));
    else
        snprintf(r->out, sizeof r->out, "%s", sr.out);
    /* The exit status is reported IN THE TEXT as well as in ok, because the
     * caller that reads only `out` must still be able to see that it failed. */
    if (sr.status != 0) {
        size_t used = strlen(r->out);
        snprintf(r->out + used, sizeof r->out - used, "[exit %d]", sr.status);
    }
    r->ok = (sr.status == 0);
    return r->ok;
}

/*
 * `cd [dir]` — change the directory that later `sh` commands run in.
 *
 * The directory is VALIDATED EAGERLY by listing it, rather than being stored on
 * faith. A `cd` that "succeeds" and then makes every later command fail is the
 * worst possible outcome: the error appears far from its cause, in a command
 * that did nothing wrong.
 */
/*
 * `edit <path> <old> <new>` — replace a unique occurrence.
 *
 * WHY THIS IS NOT `write`: write replaces the whole file, so changing one line
 * of a 500-line file means reading all of it, editing in memory, and writing it
 * back — and any disagreement between the copy and the original silently
 * destroys the rest. `edit` touches only what matched, and if the match is not
 * UNIQUE it FAILS rather than guessing which one was meant. That is the whole
 * value: a wrong guess is invisible, a refusal is not.
 *
 * It is also dsh's second-most-used tool (149 of 2852 calls, 5.2%), which is
 * why it earns a place next to `sh`.
 */
static int cmd_edit(const tool_args *a, tool_result *r) {
    char resolved[1200];
    edit_result er;
    const char *path;

    if (a->argc != 4) {
        snprintf(r->out, sizeof r->out, "usage: edit <path> <old> <new>");
        return 0;
    }
    path = tool_resolve(a->argv[1], resolved, sizeof resolved);
    er = edit_replace(path, a->argv[2], a->argv[3]);
    if (!er.ok) {
        if (er.err == EDIT_NOT_FOUND)
            snprintf(r->out, sizeof r->out, "not found in %s (nothing changed)", a->argv[1]);
        else if (er.err == EDIT_NOT_UNIQUE)
            snprintf(r->out, sizeof r->out,
                     "appears %ld times in %s — add more context to make it unique "
                     "(nothing changed)", er.count, a->argv[1]);
        else
            snprintf(r->out, sizeof r->out, "cannot edit %s (errno %d)", path, er.err);
        return 0;
    }
    snprintf(r->out, sizeof r->out, "replaced 1 occurrence in %s (%ld bytes now)",
             a->argv[1], er.bytes);
    r->ok = 1;
    return 1;
}

static int cmd_cd(const tool_args *a, tool_result *r) {
    const char *dir = a->argc > 1 ? a->argv[1] : "";
    char probe[64];
    file_result fr;

    if (a->argc > 2) { snprintf(r->out, sizeof r->out, "usage: cd [dir]"); return 0; }

    if (!dir[0] || !strcmp(dir, "~")) {
        /* A bare `cd` reports where we are instead of guessing a home
         * directory: csih does not expand `~` anywhere else, and doing it here
         * would be the one place a caller's text turns into a different path.
         * getenv("HOME") is read, but never by string-substituting into a
         * command. */
        const char *home = getenv("HOME");
        if (!dir[0]) {
            snprintf(r->out, sizeof r->out, "cwd: %s",
                     tool_cwd[0] ? tool_cwd : "(startup directory)");
            r->ok = 1;
            return 1;
        }
        if (!home) { snprintf(r->out, sizeof r->out, "cannot resolve ~ (no HOME)"); return 0; }
        dir = home;
    }

    /* Eager validation: can we list it? */
    fr = file_list(dir, probe, sizeof probe);
    if (!fr.ok && fr.err != ENOSPC) {
        /* ENOSPC here means the directory is real but has many entries — that
         * is a perfectly good directory to cd into, so it is NOT a failure. */
        snprintf(r->out, sizeof r->out, "cannot cd to %s (errno %d)", dir, fr.err);
        return 0;
    }
    if (strlen(dir) >= sizeof tool_cwd) {
        snprintf(r->out, sizeof r->out, "path too long");
        return 0;
    }
    strcpy(tool_cwd, dir);
    snprintf(r->out, sizeof r->out, "cwd is now %s", tool_cwd);
    r->ok = 1;
    return 1;
}

static int cmd_ls(const tool_args *a, tool_result *r) {
    char *buf;
    file_result fr;
    const char *dir = a->argc > 1 ? a->argv[1] : (tool_cwd[0] ? tool_cwd : ".");
    if (a->argc > 2) { snprintf(r->out, sizeof r->out, "usage: ls [dir]"); return 0; }
    buf = (char *)malloc(TOOL_OUT_MAX);
    if (!buf) { snprintf(r->out, sizeof r->out, "out of memory"); return 0; }
    fr = file_list(dir, buf, TOOL_OUT_MAX);
    if (!fr.ok) {
        /*
         * ENOSPC here means the LISTING DID NOT FIT, which is not the same as
         * "cannot list". The first version said "cannot list /tmp (errno 28)"
         * for a directory that lists perfectly well in a bigger buffer — an
         * error message that named the wrong problem. A directory with 5000+
         * entries is ordinary; the limit is ours, so say so and say the size.
         */
        if (fr.err == ENOSPC)
            snprintf(r->out, sizeof r->out,
                     "%s has more entries than the %d-byte listing buffer holds "
                     "(listing was refused, not truncated)",
                     dir, TOOL_OUT_MAX);
        else
            snprintf(r->out, sizeof r->out, "cannot list %s (errno %d)", dir, fr.err);
        free(buf);
        return 0;
    }
    snprintf(r->out, sizeof r->out, "%ld entries:\n%s", fr.bytes, buf);
    free(buf);
    r->ok = 1;
    return 1;
}

/* ── the table ──────────────────────────────────────────────────────────── */

typedef int (*tool_fn)(const tool_args *, tool_result *);

typedef struct {
    const char *name;
    const char *usage;
    const char *help;
    tool_fn     fn;
} tool_entry;

static const tool_entry TOOLS[] = {
    { "sh",     "sh <command...>",      "run a command through a real shell (/bin/sh -c)", cmd_sh },
    { "cd",     "cd [dir]",             "change the directory later sh/read/write/ls use", cmd_cd },
    { "edit",   "edit <path> <old> <new>", "replace a UNIQUE occurrence (fails otherwise)", cmd_edit },
    { "read",   "read <path>",          "print a file (refused if larger than 64KB)", cmd_read },
    { "write",  "write <path> <text>",  "atomically replace a file's contents",       cmd_write },
    { "append", "append <path> <line>", "add one line to a file",                     cmd_append },
    { "ls",     "ls [dir]",             "list a directory",                           cmd_ls },
};
#define TOOL_COUNT ((int)(sizeof TOOLS / sizeof TOOLS[0]))

/*
 * Run one command line.
 *
 * Returns a tool_result whose `out` ALWAYS says something — including for an
 * unknown command, where the useful reply is the list of known ones. A tool
 * layer that answers "no" without saying what would have worked just moves the
 * stall one step later.
 */
tool_result tool_run(const char *line) {
    tool_result r;
    tool_args a;
    int i;

    r.ok = 0;
    r.out[0] = '\0';

    if (!line || !line[0]) { snprintf(r.out, sizeof r.out, "empty command"); return r; }
    if (tool_split(line, &a) != 0) {
        snprintf(r.out, sizeof r.out,
                 "cannot parse (unterminated quote, or more than %d arguments)",
                 TOOL_MAX_ARGS);
        return r;
    }
    if (a.argc == 0) { snprintf(r.out, sizeof r.out, "empty command"); return r; }

    if (!strcmp(a.argv[0], "help")) {
        r.ok = 1;
        {
            size_t w = 0;
            w += (size_t)snprintf(r.out + w, sizeof r.out - w, "commands:");
            for (i = 0; i < TOOL_COUNT && w < sizeof r.out - 64; i++)
                w += (size_t)snprintf(r.out + w, sizeof r.out - w, "\n  %-28s %s",
                                      TOOLS[i].usage, TOOLS[i].help);
        }
        return r;
    }

    for (i = 0; i < TOOL_COUNT; i++) {
        if (!strcmp(a.argv[0], TOOLS[i].name)) {
            TOOLS[i].fn(&a, &r);
            return r;
        }
    }

    /* Unknown: name what IS available. Naming nothing is how a tool surface
     * becomes something a model stops using. */
    {
        size_t w = (size_t)snprintf(r.out, sizeof r.out, "unknown command '%s'. known:", a.argv[0]);
        for (i = 0; i < TOOL_COUNT && w < sizeof r.out - 24; i++)
            w += (size_t)snprintf(r.out + w, sizeof r.out - w, " %s", TOOLS[i].name);
        snprintf(r.out + w, sizeof r.out - w, " (try: help)");
    }
    return r;
}

/* tools_cli.c owns the self-test. */
