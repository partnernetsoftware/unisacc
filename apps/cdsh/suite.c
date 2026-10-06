/* suite.c — the one argv table for cdsh selftests. No main.
 *
 * check.c walks every row in-process. The TUI does not link this file:
 * net.c already pulls netdb.h, and one more translation unit overflows
 * unisacc's structure-id capacity. suite_cli.c is the process the TUI
 * starts with shell_run_in. Its rows subcommand prints the slices that
 * name one basename.
 *
 * The tui and agent rows are the images that actually link. They do not
 * name suite.c or probes/u_run.c. Those two files are the suite row.
 * The tui row starts with cdsh.c, which includes tui.c. tui.c is not a
 * second argv slot (structure-id ceiling). rows tui.c still selects it.
 */
/* No #include. Same ceiling as above if this file is ever linked into the TUI. */
int snprintf(char *s, unsigned long n, const char *fmt, ...);
int printf(const char *fmt, ...);
char *getenv(const char *name);
unsigned long strlen(const char *s);
int strcmp(const char *a, const char *b);
char *strstr(const char *s, const char *n);
char *strrchr(const char *s, int c);
char *strchr(const char *s, int c);

#define SUITE_N 17
#define SUITE_MAX_SRC 20

static const char *slice_name[SUITE_N] = {
    "gate", "json", "session", "render", "term", "tui", "chat", "edit",
    "tools", "shell", "net", "llm", "clock", "agent", "file", "loop", "suite"
};
static const int slice_n[SUITE_N] = {
    2, 2, 3, 2, 2, 15, 6, 3, 5, 2, 2, 6, 3, 9, 2, 4, 2
};
static const char *slice_src[SUITE_N][SUITE_MAX_SRC] = {
    { "gate.c", "gate_cli.c" },
    { "json.c", "json_cli.c" },
    { "json.c", "session.c", "session_cli.c" },
    { "render.c", "render_cli.c" },
    { "term.c", "term_cli.c" },
    { "cdsh.c", "render.c", "term.c", "chat.c", "clock.c", "tools.c",
      "file.c", "shell.c", "edit.c", "gate.c", "json.c", "session.c",
      "agent.c", "plugin.c", "net.c" },
    { "chat.c", "clock.c", "gate.c", "json.c", "session.c", "chat_cli.c" },
    { "edit.c", "file.c", "edit_cli.c" },
    { "tools.c", "file.c", "shell.c", "edit.c", "tools_cli.c" },
    { "shell.c", "shell_cli.c" },
    { "net.c", "net_cli.c" },
    { "llm.c", "llm_cli.c", "json.c", "net.c", "gate.c", "session.c" },
    { "clock.c", "clock_cli.c", "gate.c" },
    { "agent.c", "agent_cli.c", "file.c", "edit.c", "shell.c",
      "json.c", "session.c", "net.c", "plugin.c" },
    { "file.c", "file_cli.c" },
    { "gate.c", "json.c", "session.c", "loop.c" },
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

int suite_slice_fill(int i, const char **argv, int cap, const char **name) {
    int k;
    if (name) *name = NULL;
    if (i < 0 || i >= SUITE_N || cap < 2) return 0;
    if (name) *name = slice_name[i];
    for (k = 0; k < slice_n[i] && k < cap - 1; k++) argv[k] = slice_src[i][k];
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
    /* tui.c is compiled as the body of cdsh.c, so it has no argv slot. */
    if (!strcmp(base, "tui.c")) base = "cdsh.c";
    for (i = 0; i < SUITE_N; i++)
        for (k = 0; k < slice_n[i]; k++)
            if (!strcmp(slice_src[i][k], base)) return 1;
    return 0;
}

/* 1 when this path is a source of this tree that some slice names. */
int suite_match_path(const char *path, const char *cwd) {
    const char *base = suite_base(path);
    if (!suite_named(base)) return 0;
    if (path && strstr(path, "/dsh/cdsh/")) return 1;
    if (cwd && strstr(cwd, "/dsh/cdsh") && path && !strchr(path, '/')) return 1;
    return 0;
}

int suite_run_selftest(void) {
    int fail = 0;
    const char *root = "/Users/wjc/repos/moltbaby/skills/llm/dsh/cdsh";
    if (suite_match_path("/tmp/json.c", "/tmp")) fail++;
    if (!suite_match_path("json.c", root)) fail++;
    if (!suite_match_path("/Users/wjc/repos/moltbaby/skills/llm/dsh/cdsh/plugin.c", "/tmp")) fail++;
    if (suite_match_path("README.md", root)) fail++;
    if (suite_slice_count() < 16) fail++;
    if (!suite_match_path("cdsh.c", root)) fail++;
    if (!suite_match_path("tui.c", root)) fail++;
    {
        int i, k, tui_n = -1;
        for (i = 0; i < suite_slice_count(); i++) {
            const char *av[24];
            const char *nm = 0;
            int n = suite_slice_fill(i, av, 24, &nm);
            if (nm && !strcmp(nm, "tui")) {
                tui_n = n;
                if (!av[0] || strcmp(av[0], "cdsh.c")) fail++;
                for (k = 0; k < n; k++) {
                    if (!av[k]) continue;
                    if (!strcmp(av[k], "tui.c") || !strcmp(av[k], "suite.c")) fail++;
                }
            }
        }
        /* 15 sources plus the selftest word. cdsh.c stands in for tui.c. */
        if (tui_n != 16) fail++;
    }
    if (fail) printf("FAIL suite match %d\n", fail);
    return fail ? 1 : 0;
}
