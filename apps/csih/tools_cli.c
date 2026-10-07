/*
 * tools_cli.c — the `main` for tools.c, moved out so tools.c can be a library
 * linked into the TUI. Same rule as every other module here.
 *
 * CLI takes SUBCOMMANDS, not dash-options (unisacc reserves the dashes).
 */
#include <stdio.h>
#include <string.h>

typedef struct { int ok; char out[65536]; } tool_result;
tool_result tool_run(const char *line);
int         tool_run_selftest(void);

int main(int argc, char **argv) {
    const char *cmd = argc > 1 ? argv[1] : "";
    tool_result r;
    if (!strcmp(cmd, "selftest")) return tool_run_selftest();
    if (!strcmp(cmd, "run")) {
        /* usage: tools_cli run "<command line>" */
        r = tool_run(argc > 2 ? argv[2] : "");
        printf("%s\n", r.out);
        return r.ok ? 0 : 1;
    }
    printf("usage: tools_cli selftest | run \"<command line>\"\n");
    return 64;
}

#include <sys/stat.h>
#include <unistd.h>

typedef struct { int ok; int err; long bytes; } file_result;
file_result file_write(const char *path, const char *text, size_t len);

#define TOOL_MAX_ARGS 16
typedef struct {
    char *argv[TOOL_MAX_ARGS];
    int argc;
    char buf[2048];
} tool_args;
int tool_split(const char *line, tool_args *a);

/* ── self-test ──────────────────────────────────────────────────────────── */

static int failures = 0;
static void expect(int cond, const char *what) {
    if (!cond) { printf("FAIL %s\n", what); failures++; }
}

static void run_selftest(void) {
    tool_result r;
    tool_args a;
    const char *f = "/tmp/cdsh-tools-selftest.txt";

    /* --- splitting ------------------------------------------------------ */
    expect(tool_split("write /tmp/x hello", &a) == 0 && a.argc == 3, "three bare words split");
    expect(!strcmp(a.argv[2], "hello"), "the third word is right");
    expect(tool_split("write /tmp/x \"hello world\"", &a) == 0 && a.argc == 3,
           "a quoted argument is one word");
    expect(!strcmp(a.argv[2], "hello world"), "quotes group and are removed");
    expect(tool_split("  ls   ", &a) == 0 && a.argc == 1, "extra spaces are skipped");
    expect(tool_split("write x \"unterminated", &a) != 0,
           "an unterminated quote is REFUSED, not silently accepted");
    expect(tool_split("", &a) == 0 && a.argc == 0, "an empty line has no arguments");

    /* --- dispatch ------------------------------------------------------- */
    r = tool_run("help");
    expect(r.ok == 1, "help succeeds");
    expect(strstr(r.out, "read") && strstr(r.out, "write") && strstr(r.out, "ls"),
           "help lists the commands");

    r = tool_run("nosuchcommand");
    expect(r.ok == 0, "an unknown command fails");
    expect(strstr(r.out, "unknown command") != NULL, "and says the name was unknown");
    expect(strstr(r.out, "read") != NULL, "and names what IS available");

    /* --- edit: the unique-match contract through the tool layer --------- */
    (void)file_write("/tmp/cdsh-edit-test.txt", "alpha\nbeta\ngamma\n", 17);
    r = tool_run("edit /tmp/cdsh-edit-test.txt beta BETA");
    expect(r.ok == 1, "a unique match is replaced");
    r = tool_run("read /tmp/cdsh-edit-test.txt");
    expect(strstr(r.out, "BETA") != NULL, "and the change is on disk");

    r = tool_run("edit /tmp/cdsh-edit-test.txt nosuchthing X");
    expect(r.ok == 0, "a non-existent target fails");
    expect(strstr(r.out, "not found") != NULL, "and says it was not found");
    expect(strstr(r.out, "nothing changed") != NULL,
           "and says explicitly that the file was NOT touched");

    (void)file_write("/tmp/cdsh-edit-test.txt", "dup\ndup\n", 8);
    r = tool_run("edit /tmp/cdsh-edit-test.txt dup X");
    expect(r.ok == 0, "an ambiguous target fails");
    expect(strstr(r.out, "2 times") != NULL, "and reports HOW MANY times it appeared");

    r = tool_run("edit /tmp/cdsh-edit-test.txt");
    expect(r.ok == 0 && strstr(r.out, "usage") != NULL, "wrong arity shows usage");
    remove("/tmp/cdsh-edit-test.txt");

    /* --- the working directory (the 84%-of-calls fix) -------------------- */
    /*
     * The scenario the measurement pointed at: cd ONCE, then run commands that
     * do not mention the directory at all. Before this existed, every one of
     * these would have needed its own `cd`.
     */
    mkdir("/tmp/cdsh-cd-test", 0755);
    (void)file_write("/tmp/cdsh-cd-test/marker.txt", "found\n", 6);

    r = tool_run("cd /tmp/cdsh-cd-test");
    expect(r.ok == 1, "cd succeeds for a real directory");
    expect(strstr(r.out, "cdsh-cd-test") != NULL, "and reports where it went");

    r = tool_run("sh pwd");
    expect(r.ok == 1 && strstr(r.out, "cdsh-cd-test") != NULL,
           "a later sh runs IN that directory without repeating cd");

    r = tool_run("sh cat marker.txt");
    expect(strstr(r.out, "found") != NULL,
           "and a relative path resolves against it");

    r = tool_run("read marker.txt");
    expect(r.ok == 1 && strstr(r.out, "found") != NULL,
           "read also resolves against it");

    r = tool_run("ls");
    expect(r.ok == 1 && strstr(r.out, "marker.txt") != NULL,
           "ls with no argument lists it");

    /* A cd to somewhere that does not exist must FAIL NOW, not silently make
     * every later command fail — the error belongs at the cd. */
    r = tool_run("cd /tmp/cdsh-no-such-directory");
    expect(r.ok == 0, "cd to a missing directory fails immediately");
    r = tool_run("sh pwd");
    expect(strstr(r.out, "cdsh-cd-test") != NULL,
           "and the previous directory is UNCHANGED (a failed cd did not half-apply)");

    /*
     * A `cd` inside a command affects THAT command's shell and nothing else —
     * which is correct, and is what makes `sh` safe to use for anything.
     *
     * The first version of this assertion expected the opposite (that the
     * output would still be cdsh-cd-test), which is simply wrong: `pwd` runs in
     * the subshell that just cd'd. The property worth testing is the one AFTER
     * the command — the tool's own directory must be untouched.
     */
    r = tool_run("sh cd /tmp && pwd");
    expect(strstr(r.out, "/tmp") != NULL, "a cd inside a command takes effect in that command");
    r = tool_run("sh pwd");
    expect(strstr(r.out, "cdsh-cd-test") != NULL,
           "...and does NOT leak into the tool's own directory");

    /* back to a neutral state for the remaining tests */
    r = tool_run("cd /");
    expect(r.ok == 1, "cd / works");

    remove("/tmp/cdsh-cd-test/marker.txt");
    rmdir("/tmp/cdsh-cd-test");

    /* --- sh: the escape hatch to a real shell --------------------------- */
    r = tool_run("sh echo from-the-shell");
    expect(r.ok == 1, "sh runs a command");
    expect(strstr(r.out, "from-the-shell") != NULL, "and its output comes back");

    /* Pipes must reach the SHELL, not be parsed here. */
    r = tool_run("sh printf 'a\\nb\\nc\\n' | wc -l");
    expect(r.ok == 1 && strstr(r.out, "3") != NULL, "a pipeline is the shell's to handle");

    /* A failing command is reported BOTH ways: ok=0 and in the text, so a
     * caller reading only `out` still sees it. */
    r = tool_run("sh exit 3");
    expect(r.ok == 0, "sh reports a non-zero exit through ok");
    expect(strstr(r.out, "exit 3") != NULL, "AND in the visible text");

    r = tool_run("sh");
    expect(r.ok == 0 && strstr(r.out, "usage") != NULL, "sh with no command shows usage");

    /* --- a real round trip through the table ---------------------------- */
    mkdir("/tmp/cdsh-tools-lsdir", 0755);
    (void)file_write("/tmp/cdsh-tools-lsdir/one", "1", 1);
    remove(f);
    r = tool_run("write /tmp/cdsh-tools-selftest.txt hello");
    expect(r.ok == 1, "write through the table works");
    r = tool_run("read /tmp/cdsh-tools-selftest.txt");
    expect(r.ok == 1 && !strcmp(r.out, "hello"), "read gives back what was written");

    r = tool_run("append /tmp/cdsh-tools-selftest.txt second");
    expect(r.ok == 1, "append works");
    r = tool_run("read /tmp/cdsh-tools-selftest.txt");
    /* The file ends with a newline now — appending to "hello" (no trailing
     * newline) inserts the boundary before the new line. Asserting
     * "hello\nsecond" without the final newline was asserting the OLD buggy
     * shape, and it is what kept this test red after the bug was fixed. */
    expect(!strcmp(r.out, "hello\nsecond\n"),
           "append made two clean lines (the boundary newline is inserted)");

    /* --- quoting reaches the file layer intact -------------------------- */
    r = tool_run("write /tmp/cdsh-tools-selftest.txt \"a b c\"");
    expect(r.ok == 1, "a quoted argument writes");
    r = tool_run("read /tmp/cdsh-tools-selftest.txt");
    expect(!strcmp(r.out, "a b c"), "the spaces survived the round trip");

    /* --- usage errors name the shape ------------------------------------ */
    r = tool_run("write onlyonearg");
    expect(r.ok == 0, "wrong arity fails");
    expect(strstr(r.out, "usage:") != NULL, "and prints the usage line");

    r = tool_run("read /tmp/definitely-does-not-exist-cdsh");
    expect(r.ok == 0, "reading a missing file fails");
    expect(strstr(r.out, "cannot read") != NULL, "and says so without crashing");

    /* /tmp on this machine holds 5000+ entries, which does NOT fit the
     * listing buffer — so `ls /tmp` legitimately refuses. Use a directory
     * whose size is ours to control, and assert the refusal separately. */
    r = tool_run("ls /tmp/cdsh-tools-lsdir");
    expect(r.ok == 1, "ls works on a small directory");
    expect(strstr(r.out, "entries") != NULL, "ls reports a count");

    /* --- the limit is REPORTED, not silently applied -------------------- */
    r = tool_run("read /tmp");
    expect(r.ok == 0, "reading a directory through the tool fails cleanly");

    /* A listing too large to fit must SAY that, not claim the directory is
     * unreadable. On this machine /tmp is big enough to exercise it. */
    r = tool_run("ls /tmp");
    if (!r.ok) {
        expect(strstr(r.out, "more entries") != NULL,
               "an oversized listing explains that the BUFFER was the limit");
    }

    remove(f);
    rmdir("/tmp/cdsh-tools-lsdir");
}

int tool_run_selftest(void) {
    run_selftest();
    printf("%s\n", failures ? "SELFTEST FAILED" : "selftest ok");
    return failures == 0 ? 0 : 1;
}
