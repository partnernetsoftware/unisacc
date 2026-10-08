/*
 * shell_cli.c — the `main` for shell.c, moved out so shell.c is a library that
 * the TUI can link. Same rule as every other module here.
 *
 * CLI takes SUBCOMMANDS, not dash-options (unisacc reserves the dashes).
 */
#include <stdio.h>
#include <string.h>

typedef struct {
    int ok; int err; int exited; int status; int signal; int timed_out; long bytes;
    char out[65536];
} shell_result;

shell_result shell_run(const char *command);
shell_result shell_run_in(const char *command, const char *cwd);
int          shell_run_selftest(void);

int main(int argc, char **argv) {
    const char *cmd = argc > 1 ? argv[1] : "";
    shell_result r;
    if (!strcmp(cmd, "selftest")) return shell_run_selftest();
    if (!strcmp(cmd, "run")) {
        r = shell_run(argc > 2 ? argv[2] : "");
        if (!r.ok) { printf("could not run (errno %d)\n", r.err); return 1; }
        fputs(r.out, stdout);
        if (!r.exited) printf("[signalled %d]\n", r.signal);
        else if (r.status) printf("[exit %d]\n", r.status);
        return r.exited ? r.status : 1;
    }
    printf("usage: shell_cli selftest | run \"<command>\"\n");
    return 64;
}

#include <sys/stat.h>
#include <unistd.h>

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

    /* --- a runaway command is killed, not waited on forever ------------- */
    setenv("CSIH_EXEC_TIMEOUT_SEC", "1", 1);
    r = shell_run("sleep 3");
    expect(r.timed_out == 1, "a command over the budget is marked timed out");
    expect(r.ok == 1, "and the result is still returned (not an error)");
    expect(r.exited == 0 && r.signal != 0, "and the child died by a signal (killed)");
    r = shell_run("echo alive");
    expect(!strcmp(r.out, "alive\n"), "a normal command still works after a timeout");
    unsetenv("CSIH_EXEC_TIMEOUT_SEC");
}

int shell_run_selftest(void) {
    run_selftest();
    printf("%s\n", failures ? "SELFTEST FAILED" : "selftest ok");
    return failures == 0 ? 0 : 1;
}
