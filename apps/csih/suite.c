/* suite.c — the one argv table for csih selftests. No main.
 *
 * check.c walks every row in-process. The TUI does not link this file:
 * net.c already pulls netdb.h, and one more translation unit overflows
 * unisacc's structure-id capacity. suite_cli.c is the process the TUI
 * starts with shell_run_in. Its rows subcommand prints the slices that
 * name one basename.
 *
 * The tui and agent rows are the images that actually link. They do not
 * name suite.c or probes/u_run.c. Those two files are the suite row.
 * The tui row starts with tui.c and lists the flat table of csih.sh.
 * No unit textually includes another unit; tui.c has main.
 */
/* No #include. Same ceiling as above if this file is ever linked into the TUI. */
int snprintf(char *s, unsigned long n, const char *fmt, ...);
char *realpath(const char *path, char *resolved);
int printf(const char *fmt, ...);
char *getenv(const char *name);
unsigned long strlen(const char *s);
int strcmp(const char *a, const char *b);
char *strstr(const char *s, const char *n);
char *strrchr(const char *s, int c);
char *strchr(const char *s, int c);

#define SUITE_N 17
#define SUITE_MAX_SRC 40

static const char *slice_name[SUITE_N] = {
    "gate", "json", "session", "render", "term", "tui", "chat", "edit",
    "tools", "shell", "net", "llm", "clock", "agent", "file", "loop", "suite"
};
static const int slice_n[SUITE_N] = {
    2, 2, 3, 2, 2, 28, 6, 3, 5, 2, 2, 6, 3, 9, 2, 4, 2
};
static const char *slice_src[SUITE_N][SUITE_MAX_SRC] = {
    { "gate.c", "gate_cli.c" },
    { "json.cx", "json_cli.c" },
    { "json.cx", "session.c", "session_cli.c" },
    { "render.c", "cols.cx", "render_cli.c" },
    { "term.c", "term_cli.c" },
    { "tui.c", "render.c", "term.c", "chat.c", "clock.c", "tools.c",
      "cols.cx", "home.cx", "file.c", "shell.c", "edit.c", "gate.c",
      "json.cx", "session.c", "agent.c", "plugin.c", "net.c",
      "reload_state.c", "reload_session_decode.c", "reload_session_encode.c",
      "reload_io.c", "reload_load.c", "reload_consume.c", "journal_checkpoint.c",
      "reload_owner.c", "csih_message.c", "csih_message_io.c", "context_index.c" },
    { "chat.c", "clock.c", "gate.c", "json.cx", "session.c", "chat_cli.c" },
    { "edit.c", "file.c", "edit_cli.c" },
    { "tools.c", "file.c", "shell.c", "edit.c", "tools_cli.c" },
    { "shell.c", "shell_cli.c" },
    { "net.c", "net_cli.c" },
    { "llm.c", "llm_cli.c", "json.cx", "net.c", "gate.c", "session.c" },
    { "clock.c", "clock_cli.c", "gate.c" },
    { "agent.c", "agent_cli.c", "file.c", "edit.c", "shell.c",
      "json.cx", "session.c", "net.c", "plugin.c" },
    { "file.c", "file_cli.c" },
    { "gate.c", "json.cx", "session.c", "loop.c" },
    { "suite.c", "suite_cli.c" },
};

static int suite_red;
static char suite_why[180];

void suite_mark(int red, const char *why) {
    suite_red = red ? 1 : 0;
    if (!suite_red) { suite_why[0] = 0; return; }
    snprintf(suite_why, sizeof suite_why, "%s", why ? why : "slice red");
}

int suite_slice_count(void) { return SUITE_N; }

/* Run mode (no -o) takes the public headers only through -include, so every row
 * carries the same three globals as csih.sh. The forward file cannot resolve a
 * relative -include, so the paths are absolute: the app dir from getcwd(). */
#define SUITE_GLOBALS_N 6
static char suite_glob_buf[3][600];
static const char *suite_globals[SUITE_GLOBALS_N];
static int suite_globals_ready;

static void suite_globals_init(void) {
    static const char *hdr[3] = {"csih_cols.h", "csih_home.h", "json.h"};
    char dir[512];
    int h;
    if (suite_globals_ready) return;
    if (!realpath(".", dir)) dir[0] = 0;
    for (h = 0; h < 3; h++) {
        snprintf(suite_glob_buf[h], sizeof suite_glob_buf[h], "%s/%s", dir, hdr[h]);
        suite_globals[2 * h] = "-include";
        suite_globals[2 * h + 1] = suite_glob_buf[h];
    }
    suite_globals_ready = 1;
}

int suite_slice_fill(int i, const char **argv, int cap, const char **name) {
    int k, g;
    if (name) *name = NULL;
    if (i < 0 || i >= SUITE_N || cap < 2) return 0;
    if (name) *name = slice_name[i];
    if (cap < SUITE_GLOBALS_N + 2) return 0;
    suite_globals_init();
    for (g = 0; g < SUITE_GLOBALS_N; g++) argv[g] = suite_globals[g];
    for (k = 0; k < slice_n[i] && k + SUITE_GLOBALS_N < cap - 1; k++) argv[SUITE_GLOBALS_N + k] = slice_src[i][k];
    k += SUITE_GLOBALS_N;
    argv[k++] = "selftest";
    return k;
}

int suite_failing(char *why, int n) {
    if (!suite_red) return 0;
    if (why && n > 0) snprintf(why, (size_t)n, "%s", suite_why);
    return 1;
}

static const char *suite_base(const char *path) {
    const char *s;
    if (!path || !path[0]) return "";
    s = strrchr(path, '/');
    return s ? s + 1 : path;
}

static int suite_named(const char *base) {
    int i, k;
    if (!base || !base[0]) return 0;
    for (i = 0; i < SUITE_N; i++)
        for (k = 0; k < slice_n[i]; k++)
            if (!strcmp(slice_src[i][k], base)) return 1;
    return 0;
}

/* 1 when this path is a source of this tree that some slice names. */
int suite_match_path(const char *path, const char *cwd) {
    const char *base = suite_base(path);
    if (!suite_named(base)) return 0;
    if (path && strstr(path, "/apps/csih/")) return 1;
    if (cwd && strstr(cwd, "/apps/csih") && path && !strchr(path, '/')) return 1;
    return 0;
}

int suite_run_selftest(void) {
    int fail = 0;
    const char *root = "/Users/wjc/repos/unisacc/apps/csih";
    if (suite_match_path("/tmp/json.c", "/tmp")) fail++;
    if (!suite_match_path("json.cx", root)) fail++;
    if (!suite_match_path("/Users/wjc/repos/unisacc/apps/csih/plugin.c", "/tmp")) fail++;
    if (suite_match_path("README.md", root)) fail++;
    if (suite_slice_count() < 16) fail++;
    if (!suite_match_path("tui.c", root)) fail++;
    {
        int i, k, tui_n = -1;
        for (i = 0; i < suite_slice_count(); i++) {
            const char *av[48];
            const char *nm = 0;
            int n = suite_slice_fill(i, av, 48, &nm);
            if (nm && !strcmp(nm, "tui")) {
                tui_n = n;
                if (!av[SUITE_GLOBALS_N] || strcmp(av[SUITE_GLOBALS_N], "tui.c")) fail++;
                for (k = 0; k < n; k++) {
                    if (!av[k]) continue;
                    if (!strcmp(av[k], "suite.c")) fail++;
                }
            }
        }
        /* 6 global -include words, 28 sources, and the selftest word: the flat table of csih.sh. */
        if (tui_n != SUITE_GLOBALS_N + 28 + 1) fail++;
    }
    if (fail) printf("FAIL suite match %d\n", fail);
    return fail ? 1 : 0;
}
