/*
 * agent_cli.c — the `main` for agent.c.
 *
 * Same rule as every other module: unisacc runs a source as a program, a
 * program has one `main`, so agent.c (a library) cannot carry it. The CLI also
 * owns all stdio — agent.c never prints. After a run it reads the transcript
 * back and prints a compact trace, because the transcript is the single source
 * of truth for what the agent did.
 *
 * CLI takes SUBCOMMANDS, not dash-options (unisacc reserves the dashes).
 *
 * Usage:
 *   agent_cli selftest
 *   agent_cli agent <prompt...>        (runs the full loop against the endpoint)
 *
 * Env overrides (so the same binary drives a stub or the real proxy):
 *   CDSH_ENDPOINT   default https://api.deepseek.com/v1/chat/completions
 *   CDSH_MODEL      default deepseek-chat
 *   CDSH_CWD        working dir for tools + the two working files (default ".")
 *   CDSH_TRANSCRIPT transcript path (default a fresh /tmp file)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unistd.h>

typedef enum {
    ACT_ERR = 0, ACT_EXEC, ACT_READ, ACT_WRITE, ACT_EDIT,
    ACT_ANSWER, ACT_GO_CONTINUE, ACT_GO_STOP, ACT_MIND
} agent_kind;

typedef struct {
    int   kind;
    char  cmd[4096];
    char  why[160];
    char  op[64];
    char  path[1024];
    char  text[4096];
    char  old[4096];
    char  nw[4096];
} agent_step;

typedef struct {
    int  ok;
    int  stopped;
    int  rounds;
    int  actions;
    int  err;
    char answer[4096];
    char reason[128];
} agent_result;

agent_step agent_parse(const char *content);
int  agent_exec(const agent_step *s, const char *cwd, char *out, size_t outlen);
int  agent_note_cd(char *cwd, size_t cwdlen, const char *cmd, char *out, size_t outlen);
int  agent_object_count(const char *s);
int  agent_tool_record(char *rec, size_t recsz, const char *name, const char *text);
int  agent_chat_role(const char *role, char *out, size_t outlen, int *wrap);
const char *agent_model_rules(void);
agent_result agent_run(const char *prompt, const char *transcript,
                       const char *endpoint, const char *model, const char *cwd);
agent_result agent_run_cb(const char *prompt, const char *transcript,
                          const char *endpoint, const char *model, const char *cwd,
                          const char *extra_system,
                          void (*on_event)(const char *line, void *ud), void *ud);

static void print_event(const char *line, void *ud) {
    (void)ud;
    printf("%s\n", line ? line : "");
    fflush(stdout);
}

/* ── offline self-test (no network, no model) ───────────────────────────── */

static int a_failures = 0;
static void a_expect(int cond, const char *what) {
    if (!cond) { printf("  FAIL %s\n", what); a_failures++; }
    else printf("  ok   %s\n", what);
}

static int agent_run_selftest(void) {
    agent_step s;
    char out[8192];
    char path[256];
    int fd;

    a_failures = 0;
    printf("agent selftest:\n");

    /* parser: each shape resolves to the right kind */
    s = agent_parse("{\"act\":\"exec\",\"cmd\":\"echo hi\"}");
    a_expect(s.kind == ACT_EXEC, "parse exec");
    a_expect(!strcmp(s.cmd, "echo hi"), "  exec cmd");
    s = agent_parse("{\"act\":\"exec\",\"cmd\":\"echo hi\",\"why\":\"看输出\"}");
    a_expect(s.kind == ACT_EXEC && !strcmp(s.why, "看输出"), "parse exec why");

    s = agent_parse("{\"act\":\"file\",\"op\":\"read\",\"path\":\"/x\"}");
    a_expect(s.kind == ACT_READ, "parse file read");

    s = agent_parse("{\"act\":\"file\",\"op\":\"write\",\"path\":\"/x\",\"text\":\"hi\"}");
    a_expect(s.kind == ACT_WRITE, "parse file write");

    s = agent_parse("{\"act\":\"answer\",\"text\":\"done\"}");
    a_expect(s.kind == ACT_ANSWER, "parse answer");
    s = agent_parse("{\"act\":\"mind\",\"op\":\"add\",\"target\":\"palace\",\"text\":\"note\"}");
    a_expect(s.kind == ACT_MIND && !strcmp(s.path, "palace") && !strcmp(s.op, "add"), "parse mind target");
    s = agent_parse("{\"act\":\"mind\",\"op\":\"read\",\"which\":\"tree\"}");
    a_expect(s.kind == ACT_MIND && !strcmp(s.path, "tree"), "parse mind which");

    s = agent_parse("{\"go\":\"stop\"}");
    a_expect(s.kind == ACT_GO_STOP, "parse go:stop");

    s = agent_parse("{\"go\":\"continue\"}");
    a_expect(s.kind == ACT_GO_CONTINUE, "parse go:continue");

    s = agent_parse("total nonsense");
    a_expect(s.kind == ACT_ERR, "garbage is ACT_ERR");
    s = agent_parse("Sure.\n{\"act\":\"exec\",\"cmd\":\"pwd\"}\n{\"act\":\"exec\",\"cmd\":\"ls\"}");
    a_expect(s.kind == ACT_EXEC && strcmp(s.cmd, "pwd") == 0,
             "prose plus extra objects keeps the first");
    a_expect(agent_object_count("Sure.\n{\"act\":\"exec\",\"cmd\":\"pwd\"}\n{\"act\":\"exec\",\"cmd\":\"ls\"}") == 2,
             "two objects are counted");
    {
        char big[6001], rec[8256];
        size_t i, n;
        for (i = 0; i + 2 < sizeof big; i += 3) {
            big[i] = (char)0xe6; big[i + 1] = (char)0x8a; big[i + 2] = (char)0x80;
        }
        big[sizeof big - 1] = 0;
        a_expect(agent_tool_record(rec, sizeof rec, "file", big) == 1, "big tool record fits");
        n = strlen(rec);
        a_expect(n + 1 < sizeof rec && rec[n - 1] == '}' && rec[n - 2] == '"',
                 "  record is closed JSON");
    }

    s = agent_parse("```json\n{\"act\":\"exec\",\"cmd\":\"ls\"}\n```");
    a_expect(s.kind == ACT_EXEC, "fenced JSON is stripped");

    {
        char role[16];
        int wrap = 1;
        a_expect(agent_chat_role("assistant", role, sizeof role, &wrap) == 1
                 && !strcmp(role, "assistant") && wrap == 0,
                 "assistant role stays");
        wrap = 0;
        a_expect(agent_chat_role("tool", role, sizeof role, &wrap) == 1
                 && !strcmp(role, "user") && wrap == 1,
                 "tool role becomes a user turn");
        a_expect(agent_chat_role("decision", role, sizeof role, &wrap) == 1
                 && wrap == 1, "decision role is not sent raw");
    }

    {
        const char *rules = agent_model_rules();
        a_expect(rules
                 && strstr(rules, "bin/envelope")
                 && strstr(rules, "0:grkwjcgmcdsh")
                 && strstr(rules, "新功能先讨论")
                 && strstr(rules, "不得先写入")
                 && strstr(rules, "why")
                 && strstr(rules, "markdown-tree-dag")
                 && strstr(rules, "mermaid-flowchart-memory-palace")
                 && strstr(rules, "csih。"),
                 "rules: bin/envelope 0:grkwjcgmcdsh 新功能先讨论、不得先写入 markdown-tree-dag mermaid-flowchart-memory-palace");
    }

    /* executor: exec really runs */
    s = agent_parse("{\"act\":\"exec\",\"cmd\":\"echo hi\"}");
    a_expect(agent_exec(&s, "/tmp", out, sizeof out) == 1, "exec runs");
    a_expect(strstr(out, "hi") != NULL, "  exec output captured");

    /* a long result is stored whole; the return text is only a pointer */
    memset(&s, 0, sizeof s); s.kind = ACT_EXEC;
    strncpy(s.cmd, "dd if=/dev/zero bs=2500 count=1 2>/dev/null | tr '\\0' A", sizeof s.cmd - 1);
    a_expect(agent_exec(&s, "/tmp", out, sizeof out) == 1, "long exec runs");
    a_expect(strstr(out, "full: ") != NULL && strstr(out, "/.csih/tool/") != NULL,
             "  long result names a tool file");
    {
        const char *p = strstr(out, "full: ");
        char spath[640];
        FILE *sf;
        size_t k = 0;
        if (p) p += 6;
        while (p && *p && *p != ' ' && *p != '\n' && k + 1 < sizeof spath) spath[k++] = *p++;
        spath[k] = 0;
        sf = k ? fopen(spath, "r") : NULL;
        a_expect(sf != NULL, "  spill file exists");
        if (sf) {
            char blk[2600];
            size_t n = fread(blk, 1, sizeof blk, sf);
            fclose(sf);
            a_expect(n == 2500 && blk[0] == 'A' && blk[2499] == 'A',
                     "  spill file keeps all 2500 bytes");
            remove(spath);
        }
    }

    /* executor: file write then read round-trips */
    snprintf(path, sizeof path, "/tmp/cdsh-agent-test-%d.txt", (int)getpid());
    remove(path);
    s = agent_parse("{\"act\":\"file\",\"op\":\"write\",\"path\":\"PLACE\",\"text\":\"hello-world\"}");
    /* path is a fixed placeholder above; set it directly */
    memset(&s, 0, sizeof s); s.kind = ACT_WRITE;
    strncpy(s.path, path, sizeof s.path - 1);
    strncpy(s.text, "hello-world", sizeof s.text - 1);
    a_expect(agent_exec(&s, "/tmp", out, sizeof out) == 1, "file write runs");
    a_expect(strstr(out, "wrote") != NULL, "  write reported bytes");

    memset(&s, 0, sizeof s); s.kind = ACT_READ;
    strncpy(s.path, path, sizeof s.path - 1);
    a_expect(agent_exec(&s, "/tmp", out, sizeof out) == 1, "file read runs");
    a_expect(strstr(out, "hello-world") != NULL, "  read returned the content");

    remove(path);

    /* relative path lands in the agent cwd, not the process cwd */
    {
        char dir[64], rel[80], got[32];
        FILE *f;
        snprintf(dir, sizeof dir, "/tmp/cdsh-rel-%d", (int)getpid());
        snprintf(rel, sizeof rel, "%s/rel.txt", dir);
        mkdir(dir, 0700);
        remove(rel);
        memset(&s, 0, sizeof s); s.kind = ACT_WRITE;
        strncpy(s.path, "rel.txt", sizeof s.path - 1);
        strncpy(s.text, "hello", sizeof s.text - 1);
        a_expect(agent_exec(&s, dir, out, sizeof out) == 1, "relative write runs");
        f = fopen(rel, "r");
        a_expect(f != NULL, "  relative file is under cwd");
        if (f) {
            size_t n = fread(got, 1, sizeof got - 1, f);
            fclose(f);
            got[n] = 0;
            a_expect(strcmp(got, "hello") == 0, "  relative content");
        }
        remove(rel);
        rmdir(dir);
    }

    /* dsh: a bare cd must stick, the next file step uses the new directory */
    {
        char base[64], inner[80], cwd[128], out2[256], got[16];
        FILE *f;
        snprintf(base, sizeof base, "/tmp/cdsh-acd-%d", (int)getpid());
        snprintf(inner, sizeof inner, "%s/inner", base);
        mkdir(base, 0700);
        mkdir(inner, 0700);
        snprintf(cwd, sizeof cwd, "%s", base);
        a_expect(agent_note_cd(cwd, sizeof cwd, "cd \"inner\"", out2, sizeof out2) == 1,
                 "cd is a persistent builtin");
        a_expect(strstr(cwd, "/inner") != NULL, "  cwd moved into inner");
        memset(&s, 0, sizeof s); s.kind = ACT_WRITE;
        strncpy(s.path, "there.txt", sizeof s.path - 1);
        strncpy(s.text, "in", sizeof s.text - 1);
        a_expect(agent_exec(&s, cwd, out, sizeof out) == 1, "write after cd");
        snprintf(inner, sizeof inner, "%s/there.txt", cwd);
        f = fopen(inner, "r");
        a_expect(f != NULL, "  file is in the new directory");
        if (f) { size_t n = fread(got, 1, sizeof got - 1, f); fclose(f); got[n] = 0;
                 a_expect(strcmp(got, "in") == 0, "  content after cd"); }
        a_expect(agent_note_cd(cwd, sizeof cwd, "cd /tmp/cdsh-no-such-dir", out2, sizeof out2) == 1
                 && strstr(cwd, "/inner") != NULL, "failed cd leaves cwd");
        memset(&s, 0, sizeof s); s.kind = ACT_EXEC;
        strncpy(s.cmd, "cat there.txt", sizeof s.cmd - 1);
        a_expect(agent_exec(&s, cwd, out, sizeof out) == 1, "exec reads the file just written");
        a_expect(strstr(out, "exit=0") != NULL && strstr(out, "in") != NULL,
                 "  exec output includes the file text");
        remove(inner);
        snprintf(inner, sizeof inner, "%s", cwd);
        rmdir(inner);
        rmdir(base);
    }

    if (a_failures) { printf("agent: %d FAILED\n", a_failures); return 1; }
    printf("agent: all cases pass\n");
    return 0;
}

/* ── the run command ────────────────────────────────────────────────────── */

static void print_trace(const char *transcript) {
    /* Read the transcript and print a compact, readable trace. Kept here (not in
     * agent.c) so the library stays stdio-free. */
    FILE *f = fopen(transcript, "r");
    char buf[8192];
    if (!f) return;
    printf("\n--- trace (%s) ---\n", transcript);
    while (fgets(buf, sizeof buf, f)) {
        size_t n = strlen(buf);
        while (n && (buf[n-1] == '\n' || buf[n-1] == '\r')) buf[--n] = '\0';
        printf("%s\n", buf);
    }
    fclose(f);
}

static int run_agent(int argc, char **argv) {
    const char *endpoint = getenv("CDSH_ENDPOINT");
    const char *model    = getenv("CDSH_MODEL");
    const char *cwd      = getenv("CDSH_CWD");
    const char *transcript = getenv("CDSH_TRANSCRIPT");
    char prompt[4096];
    char def_transcript[256];
    char real_cwd[1024];
    agent_result r;
    int i, n;
    size_t k;

    if (!endpoint) endpoint = "https://api.deepseek.com/v1/chat/completions";
    if (!model)    model = "deepseek-chat";
    if (!cwd) {
        if (!getcwd(real_cwd, sizeof real_cwd)) strcpy(real_cwd, ".");
        cwd = real_cwd;
    }
    if (!transcript) {
        snprintf(def_transcript, sizeof def_transcript, "/tmp/cdsh-agent-%d.jsonl", (int)getpid());
        remove(def_transcript);
        transcript = def_transcript;
    }

    /* join the rest of argv as the prompt */
    prompt[0] = '\0';
    for (i = 2, k = 0; i < argc && k < sizeof prompt - 2; i++) {
        if (i > 2) prompt[k++] = ' ';
        n = snprintf(prompt + k, sizeof prompt - k, "%s", argv[i]);
        if (n < 0) break;
        k += (size_t)n;
    }

    printf("agent: endpoint=%s model=%s cwd=%s\n", endpoint, model, cwd);
    fflush(stdout);
    r = agent_run_cb(prompt, transcript, endpoint, model, cwd, NULL, print_event, NULL);

    if (!r.ok) {
        printf("agent FAILED: %s (err=%d)\n", r.reason, r.err);
        print_trace(transcript);
        return 1;
    }
    printf("\n%s\n", r.answer[0] ? r.answer : "(no answer text)");
    printf("rounds=%d actions=%d stopped=%s\n",
           r.rounds, r.actions, r.stopped ? "yes (go:stop)" : "no (MAX_ROUNDS)");
    print_trace(transcript);
    return 0;
}

int main(int argc, char **argv) {
    const char *cmd = argc > 1 ? argv[1] : "";
    if (!strcmp(cmd, "selftest")) return agent_run_selftest();
    if (!strcmp(cmd, "agent")) return run_agent(argc, argv);
    printf("usage: agent_cli selftest | agent <prompt...>\n");
    return 64;
}
