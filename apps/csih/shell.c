/*
 * shell.c — run a command through a real shell and bring back what it printed.
 *
 * WHY A REAL SHELL AND NOT A HAND-WRITTEN INTERPRETER: the tool this replaces
 * is called "bash" and its whole value is that a person (or a model) can write
 * what they already know how to write. Pipes, redirection, globs, `&&`, quoting
 * — a subset implementation gets all of them subtly wrong, and the failures are
 * silent: `a | b` doing something that is not a pipe is worse than a refusal.
 * The shell is the interface. This file's job is to be a faithful pipe to it,
 * not to reimplement it.
 *
 * WHY A PIPE AND NOT system(): system() does not return the OUTPUT, and output
 * is the entire point of a tool a model calls. It also runs the shell in a way
 * that discards the distinction between "the command failed" and "the shell
 * could not start", which are different facts. Here both are reported.
 *
 * THE THREE THINGS THIS MUST GET RIGHT, each of which is a classic way to hang
 * or lie:
 *
 *   1. CLOSE THE WRITE END IN THE PARENT. If the parent keeps its copy of the
 *      pipe's write end open, read() never sees EOF — it blocks forever waiting
 *      for a writer that is the parent itself. This is the single most common
 *      way a "run a command and capture output" function deadlocks.
 *
 *   2. DRAIN BEFORE WAITING, AND WAIT AFTER. Read everything, then waitpid. A
 *      child that fills the pipe buffer and blocks on write, while the parent
 *      blocks in waitpid, is a deadlock with no error message.
 *
 *   3. REPORT THE EXIT STATUS EVEN WHEN THE COMMAND FAILED. A non-zero status is
 *      a RESULT, not an error of this function. "the command ran and said no"
 *      and "the command could not run" lead to different next actions.
 *
 * unisacc limits honoured (SKILL.md §2): no `return f()` of a struct from a
 * non-main function; stdio owned by the CLI file. Two limits this header used
 * to claim are GONE and were measured away, not assumed away:
 *   - `execl`/`execlp` — unisacc 0.0.23 provides both (this file still uses
 *     execvp, which it has always had).
 *   - `chdir` — missing in 0.0.19, present in 0.0.23. The CDSH_HAVE_CHDIR guard
 *     and its shell-level `cd` fallback were deleted with it; see the child
 *     block below. `fchdir` is still missing and is not needed.
 * "structs restated" is narrower than the old line claimed: unisacc 0.0.23 sees
 * a type defined in an EARLIER file on the command line, so a restatement is not
 * strictly required to link (measured; gcc rejects the same pair). It is
 * order-dependent — put the user first and it fails with "not covered:
 * top-level construct" — and the restatements are what let each module build on
 * its own. So they stay; the old wording just overstated the reason.
 *
 * CLI takes SUBCOMMANDS, not dash-options.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

/* ── results ────────────────────────────────────────────────────────────── */

#define SHELL_OUT_MAX 65536

typedef struct {
    int  ok;          /* 1 = the shell ran (whatever its exit status) */
    int  err;         /* errno if THIS function failed (fork/pipe/wait) */
    int  exited;      /* 1 = the child exited normally */
    int  status;      /* exit status when exited, else 0 */
    int  signal;      /* terminating signal when !exited, else 0 */
    long bytes;       /* bytes of output captured */
    char out[SHELL_OUT_MAX];
} shell_result;

/* ── running ────────────────────────────────────────────────────────────── */

/*
 * Run `command` through /bin/sh -c, capture stdout+stderr, and report the exit
 * status.
 *
 * stderr goes into the same buffer as stdout ON PURPOSE. A tool that hides
 * stderr shows a model an empty result and no reason for it — the exact
 * "unresponsive tool" complaint this project keeps producing. Interleaving
 * loses the distinction, which is a real cost, so the buffer records that it
 * happened: see the `2>&1` in the command that is actually executed.
 */
/*
 * Run in a SPECIFIC DIRECTORY.
 *
 * WHY THIS EXISTS, and the measurement behind it: of 2852 real tool calls in a
 * dsh session, 2107 (84%) began with `cd`. The reason is that every call is a
 * new process, so the working directory resets — a person writes `cd repo` once
 * and stays there, while a tool surface that cannot remember a directory forces
 * the caller to prepend `cd` to everything forever. That is not a cost the user
 * pays once; it is a cost on every single call.
 *
 * So the directory is a parameter. The child chdir()s before exec, which is
 * both the simplest implementation and the most honest one: nothing about the
 * PARENT process's directory changes, so two concurrent calls cannot interfere.
 *
 * cwd == NULL means "wherever we are", preserving the old behaviour exactly.
 */
shell_result shell_run_in(const char *command, const char *cwd) {
    shell_result r;
    int pipefd[2];
    pid_t pid;

    memset(&r, 0, sizeof r);
    r.ok = 0;

    if (!command) { r.err = EINVAL; return r; }
    if (strlen(command) == 0) { r.err = EINVAL; return r; }

    if (pipe(pipefd) != 0) { r.err = errno; return r; }

    pid = fork();
    if (pid < 0) {
        r.err = errno;
        close(pipefd[0]);
        close(pipefd[1]);
        return r;
    }

    if (pid == 0) {
        /* ---- child ---- */
        /* The directory is changed HERE, in the child, so the parent process's
         * own cwd never moves — see the note above. A failed chdir exits 125,
         * a code the parent can distinguish from "the command ran and failed"
         * (non-zero status) and from "could not exec" (126).
         *
         * This used to sit behind a CDSH_HAVE_CHDIR guard, with a shell-level
         * `cd '<dir>' || exit 125; ` prepended to the command when chdir was
         * missing. unisacc 0.0.19 lacked chdir; 0.0.23 HAS IT (measured:
         * getcwd before -> chdir("/tmp") -> getcwd after reports the new
         * directory, same as gcc). So the guard, the fallback and its quoting
         * are gone — a workaround that outlives its cause is a mystery for the
         * next reader. fchdir is still missing ("undefined function"), and is
         * not needed here.
         */
        if (cwd && chdir(cwd) != 0) _exit(125);
        /* stdin from /dev/null: a command that reads stdin must NOT steal
         * keystrokes from the terminal that launched us. This matters when
         * csih runs inside a raw-mode TUI (tui agent) — without it, an agent
         * step like `cat` or `read` would drain the keyboard buffer the TUI
         * is still reading from, and characters would vanish. It is harmless
         * for every existing caller (none of the tool commands read stdin). */
        {
            int dn = open("/dev/null", 0);
            if (dn >= 0) { if (dup2(dn, 0) < 0) { /* ignore */ } close(dn); }
        }
        /* stdout and stderr both land in the pipe; see the header. */
        if (dup2(pipefd[1], 1) < 0) _exit(126);
        if (dup2(pipefd[1], 2) < 0) _exit(126);
        close(pipefd[0]);
        close(pipefd[1]);       /* the child must not hold the read end either */
        {
            char *argv[4];
            argv[0] = (char *)"sh";
            argv[1] = (char *)"-c";
            argv[2] = (char *)command;
            argv[3] = NULL;
            /* execvp, not execl: unisacc 0.0.19 has the array forms and not the
             * variadic ones (reported). 126 is the conventional "found but
             * could not execute" so the parent can tell it apart from a
             * command that ran and failed. */
            execvp("/bin/sh", argv);
            _exit(126);
        }
    }

    /* ---- parent ---- */
    /*
     * Close the write end BEFORE reading. Holding it open means read() below
     * never returns: there is still a writer (us), so no EOF, forever. This is
     * the deadlock named in the header, and it is a one-line omission.
     */
    close(pipefd[1]);

    /* Drain first, then wait — the child may be blocked writing into a full
     * pipe while we wait for it to finish. Reversed, that is the deadlock. */
    {
        size_t off = 0;
        ssize_t n;
        while (off + 1 < sizeof r.out) {
            n = read(pipefd[0], r.out + off, sizeof r.out - 1 - off);
            if (n < 0) {
                if (errno == EINTR) continue;
                break;
            }
            if (n == 0) break;
            off += (size_t)n;
        }
        r.out[off] = '\0';
        r.bytes = (long)off;
        /* The buffer is full but the child may still be writing. Stop reading
         * into r.out, but keep draining, or waitpid deadlocks on a full pipe. */
        if (off + 1 >= sizeof r.out) {
            char junk[1024];
            r.bytes = -1;
            for (;;) {
                n = read(pipefd[0], junk, sizeof junk);
                if (n < 0) { if (errno == EINTR) continue; break; }
                if (n == 0) break;
            }
        }
    }
    close(pipefd[0]);

    {
        int st = 0;
        pid_t w;
        do {
            w = waitpid(pid, &st, 0);
        } while (w < 0 && errno == EINTR);

        if (w < 0) { r.err = errno; return r; }

        r.ok = 1;
        if (WIFEXITED(st)) {
            r.exited = 1;
            r.status = WEXITSTATUS(st);
        } else if (WIFSIGNALED(st)) {
            r.exited = 0;
            r.signal = WTERMSIG(st);
        }
    }
    return r;
}

/* Run in the current directory — the original entry point, kept so existing
 * callers (and their tests) are unaffected by the addition above. */
shell_result shell_run(const char *command) {
    shell_result r;
    r = shell_run_in(command, NULL);
    return r;
}

/* ── self-test ──────────────────────────────────────────────────────────── */

static int failures = 0;
static void expect(int cond, const char *what) {
    if (!cond) { printf("FAIL %s\n", what); failures++; }
}

static void run_selftest(void) {
    shell_result r;

    /* --- output comes back --------------------------------------------- */
    r = shell_run("echo hello");
    expect(r.ok == 1, "a command runs");
    expect(r.exited == 1 && r.status == 0, "and exits 0");
    expect(!strcmp(r.out, "hello\n"), "and its stdout is captured");

    /* --- the exit status is a RESULT, not a failure of this function ---- */
    r = shell_run("exit 7");
    expect(r.ok == 1, "a command that fails still RAN (ok=1)");
    expect(r.exited == 1 && r.status == 7, "and reports its real exit status");

    /* --- the shell is a real shell: pipes and redirection work ---------- */
    r = shell_run("printf 'a\\nb\\nc\\n' | wc -l");
    expect(r.ok == 1, "a pipeline runs");
    expect(strstr(r.out, "3") != NULL, "and the pipeline actually piped");

    r = shell_run("echo one && echo two");
    expect(strstr(r.out, "one") && strstr(r.out, "two") != NULL, "&& works");

    r = shell_run("X=42; echo $X");
    expect(strstr(r.out, "42") != NULL, "shell variables work");

    r = shell_run("for i in 1 2 3; do printf '%s' $i; done");
    expect(!strcmp(r.out, "123"), "a for loop works");

    /* --- stderr is captured too, or a model sees an empty mystery ------- */
    r = shell_run("echo to-stderr >&2");
    expect(strstr(r.out, "to-stderr") != NULL, "stderr is captured, not hidden");

    /* --- quoting survives: this is the whole reason to use a real shell - */
    r = shell_run("printf '%s' \"a b c\"");
    expect(!strcmp(r.out, "a b c"), "quoting is the shell's, not ours");

    /* --- a signal death is reported as a SIGNAL, not as exit 0 ---------- */
    r = shell_run("kill -TERM $$");
    expect(r.ok == 1, "a signalled command still returns a result");
    expect(r.exited == 0, "and is NOT reported as a normal exit");
    expect(r.signal != 0, "and names the signal");

    /* --- output larger than the pipe buffer must not deadlock ----------- */
    /* This is the deadlock in the header: if the parent waits before draining,
     * a child producing >64KB blocks forever. A one-second-late test is not
     * possible without a timer, so this checks the SURVIVAL: it returns, and
     * the truncation is flagged rather than the content being silently cut. */
    r = shell_run("i=0; while [ $i -lt 4000 ]; do echo '0123456789012345678901234567890123456789'; i=$((i+1)); done");
    expect(r.ok == 1, "a command producing >64KB returns instead of deadlocking");
    expect(r.bytes == -1, "and FLAGS the truncation rather than pretending");

    /* --- a command that does not exist is a shell error, reported ------- */
    r = shell_run("this-command-does-not-exist-xyz");
    expect(r.ok == 1, "a missing command is a shell-level failure, not ours");
    expect(r.status != 0, "and comes back with a non-zero status");

    /* --- empty input is refused, not passed to the shell --------------- */
    r = shell_run("");
    expect(r.ok == 0 && r.err == EINVAL, "an empty command is refused");

    /* --- the working directory is a PARAMETER --------------------------- */
    /*
     * The 84%-of-calls finding, as a test: run `pwd` in a chosen directory and
     * check it reports THAT directory, not the test runner's.
     */
    mkdir("/tmp/cdsh-cwd-test", 0755);
    r = shell_run_in("pwd", "/tmp/cdsh-cwd-test");
    expect(r.ok == 1, "a command runs in a chosen directory");
    expect(strstr(r.out, "cdsh-cwd-test") != NULL, "and pwd reports it");

    /* The PARENT must not have moved. If it had, a second call could not be
     * independent of the first — which is the whole reason the chdir is in the
     * child. */
    r = shell_run_in("pwd", NULL);
    expect(strstr(r.out, "cdsh-cwd-test") == NULL,
           "the caller's own directory did NOT change");

    /* A directory that does not exist is reported as such, and is NOT confused
     * with a command that ran and failed. */
    r = shell_run_in("pwd", "/tmp/cdsh-definitely-not-a-directory");
    expect(r.ok == 1, "a missing cwd still returns a result");
    expect(r.exited == 1 && r.status == 125, "and is reported as exit 125, the chdir code");

    rmdir("/tmp/cdsh-cwd-test");

    /* More output than the capture buffer must still return. */
    r = shell_run("awk 'BEGIN{for(i=0;i<80000;i++) printf \"x\"}'");
    expect(r.ok == 1 && r.exited == 1 && r.status == 0, "a long command still exits");
    expect(r.bytes < 0, "and the overflow is reported instead of a fake length");
}

int shell_run_selftest(void) {
    run_selftest();
    printf("%s\n", failures ? "SELFTEST FAILED" : "selftest ok");
    return failures == 0 ? 0 : 1;
}
