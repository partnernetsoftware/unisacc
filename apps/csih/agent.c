/*
 * agent.c — the LLM-driven loop that turns csih into an agent.
 *
 * LIBRARY, NO `main` (main is in agent_cli.c) — the same unisacc rule as every
 * other module: one main per program, so a module that keeps its own cannot be
 * linked into anything else.
 *
 * WHAT THIS IS
 * ------------
 * Given a user prompt, drive a real chat model through a fixed two-point
 * decision per round:
 *   1. ACTION PHASE — the model's "quick decision": it returns ONE action as a
 *      JSON object — `exec` (run a shell command) or `file` (read/write/edit) —
 *      and the harness executes it and feeds the result back. It may also return
 *      `answer` to say the round's work is done.
 *   2. ROUND-END DECISION — after the answer, the harness asks once more:
 *      continue (jump to another round on the same goal) or stop (end).
 *
 * The model is the decider. gate.c's statistical threshold is NOT used; the only
 * hard limits here are MAX_ROUNDS / MAX_ACTIONS, which exist so a confused model
 * cannot loop forever (a fence, not a judge).
 *
 * TOOLS EXPOSED TO THE MODEL — exactly two, by design (user requirement):
 *   - exec  → shell_run_in()  (real /bin/sh -c, with csih's cwd persistence)
 *   - file  → file_read / file_write / edit_replace
 * They are called at the library level (not through tools.c's string dispatcher)
 * so a double-quote in a file's text cannot be mis-split by shell-style quoting.
 *
 * WORKING FILES — mind keeps two pages under ~/.csih: 思维树.md
 * (markdown-tree-dag) and 记忆宫殿.md (mermaid-flowchart-memory-palace).
 * They follow the user, not the working directory. agent_run seeds them
 * if missing. op=add still appends one "- " note; it does not rewrite the page.
 *
 * The endpoint URL is an argument. net.c can POST https:// directly (libcurl)
 * and resolve names. deepseek-proxy.py remains only for a caller that still
 * wants a plaintext hop on 127.0.0.1.
 *
 * unisacc limits honoured (SKILL.md §2): no `return f()` of a struct from a
 * non-main function (assign to a local first); stdio stays in agent_cli.c.
 * Every type this file uses is restated below, which is what lets the module
 * compile on its own: unisacc can see a type an EARLIER file on the command line
 * defined, but that is order-dependent, and the restatement removes the
 * dependency.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "csih_home.h"
#include "plugin_api.h"

/* ── restated declarations (so this file compiles alone) ─────────────────── */

typedef enum { J_NULL, J_BOOL, J_NUM, J_STR, J_ARR, J_OBJ } jkind;
typedef struct jvalue {
    jkind kind;
    int    b;
    double n;
    char  *s;
    struct jvalue **items;  size_t len;
    char **keys; struct jvalue **vals; size_t nkeys;
} jvalue;

jvalue *json_parse(const char *text, size_t len, char *errbuf, size_t errlen);
size_t  json_value_end(const char *text, size_t len);
void    jfree(jvalue *v);
size_t  json_escape(const char *in, char *out, size_t outlen, size_t *in_used);
size_t  json_rec(char *out, size_t cap,
                 const char *k1, const char *v1,
                 const char *k2, const char *v2,
                 const char *k3, const char *v3);
size_t  json_msg(char *buf, size_t cap, int comma,
                 const char *role, const char *lead, const char *content);
size_t  json_model(char *out, size_t cap, const char *msgs, size_t n);
jvalue *jget(jvalue *obj, const char *key);
const char *jstr(jvalue *v);
double  jnum(jvalue *v, double dflt);

typedef struct {
    int  ok;      /* 1 = success, 0 = failure */
    int  err;     /* errno at the failure point, 0 on success */
    long bytes;   /* bytes read or written */
} file_result;
file_result file_read(const char *path, char *buf, size_t cap);
file_result file_write(const char *path, const char *text, size_t len);
file_result file_append_line(const char *path, const char *line);
static int page_path(char *out, int outlen, const char *cwd, const char *which);

int plugin_kind(const char *name);
const char *plugin_name(int kind);
int plugin_catalog(char *out, int outlen);

typedef struct {
    int  ok;
    int  err;
    long count;   /* occurrences of `old` found (0, 1, or N) */
    long bytes;
} edit_result;
const char *plugin_page(const char *name);
edit_result edit_replace(const char *path, const char *old_text,
                         const char *new_text);

#define SHELL_OUT_MAX 65536
typedef struct {
    int  ok;          /* 1 = the shell ran (whatever its exit status) */
    int  err;         /* errno if THIS function failed (fork/pipe/wait) */
    int  exited;
    int  status;
    int  signal;
    int  timed_out;   /* 1 = killed for exceeding CSIH_EXEC_TIMEOUT_SEC */
    long bytes;
    char out[SHELL_OUT_MAX];
} shell_result;
shell_result shell_run_in(const char *command, const char *cwd);

#define NET_BODY_MAX  65536
#define NET_HDR_MAX   16384
typedef struct {
    int  ok;
    int  err;
    int  status;
    long body_bytes;
    int  chunked;
    char header[NET_HDR_MAX];
    char body[NET_BODY_MAX];
} net_response;
int net_async_begin(const char *method, const char *url, const char *content_type, const char *body);
int net_async_pump(int wait_ms);
net_response net_async_end(void);
void net_reset(void);
void net_turn_clock(void);
net_response net_http(const char *method, const char *url,
                     const char *content_type, const char *body);

typedef struct {
    char **lines;
    size_t count;
    size_t bad;
} session_records;
session_records session_read(const char *path);
void session_free(session_records *r);
int session_append(const char *path, const char *record);

/* forward declaration (agent_run_cb calls it below) */
static int agent_build_messages(const char *transcript, const char *tail,
                                const char *extra_system, char *out, size_t outlen);

/* The agent reports progress through this callback so a UI (the TUI) can show
 * each step live without the library itself knowing what a terminal is. A NULL
 * callback means "run silently" — the original agent_cli path. */
typedef void (*agent_event_fn)(const char *line, void *ud);

/* ── a step the model emits ──────────────────────────────────────────────── */

typedef enum {
    ACT_ERR = 0, ACT_EXEC, ACT_READ, ACT_WRITE, ACT_EDIT,
    ACT_ANSWER, ACT_GO_CONTINUE, ACT_GO_STOP, ACT_MIND
} agent_kind;

typedef struct {
    int   kind;
    char  cmd[4096];
    char  why[160];   /* one sentence shown for an exec; the command stays collapsed */
    char  op[64];
    char  path[1024];
    char  text[4096];
    char  old[4096];
    char  nw[4096];
    int   line;    /* 1-based start for file read; 0 means line 1 */
    int   nlines;  /* how many lines; 0 means the default window */
} agent_step;

static int agent_mind(const agent_step *s, const char *cwd, char *out, int outlen);
static agent_step g_mind_step;
static const char *g_mind_cwd;
static void plugin_register_once(cdsh_plugin *row, int *flag);
static cdsh_plugin mind_row, file_row, exec_row;
static int mind_registered, file_registered, exec_registered;
static int agent_file(const agent_step *s, const char *cwd, char *out, int outlen);
static agent_step g_file_step;
static const char *g_file_cwd;
static agent_step g_exec_step;
static const char *g_exec_cwd;

#define AGENT_CONTENT_MAX 4096
#define AGENT_ANSWER_MAX  4096
#define AGENT_RESULT_MAX  8192

/* The bytes after Catalog: in the system message. The TUI keeps this
 * collapsed until the rule is opened, then shows ten scrollable rows.
 * Keep the operating rules in the opening so the panel and the model
 * see the same policy. */

static const char *AGENT_SYSTEM_PROMPT =
"三件、不要别的字。why 一句中文。bin/envelope 0:grkwjcgmcsih <标题> <正文>。\n"
"思维树用 markdown-tree-dag：├── 与 └── 是包含，══> 是跨枝，不是散文。记忆宫殿用 mermaid-flowchart-memory-palace：一段 mermaid flowchart。在 ~/.csih。\n"
"csih。只许 file、exec、mind。每步一个 JSON。answer 不是第四件工具。exec 必须带 why。新功能先讨论、不得先写入。bin/envelope 0:grkwjcgmcsih。\n"
"\n"
"JSON:\n"
"  {\"act\":\"file\",\"op\":\"read\",\"path\":\"<path>\",\"line\":1,\"n\":120}\n"
"  {\"act\":\"file\",\"op\":\"write\",\"path\":\"<path>\",\"text\":\"<full contents>\"}\n"
"  {\"act\":\"file\",\"op\":\"edit\",\"path\":\"<path>\",\"old\":\"<old>\",\"new\":\"<new>\"}\n"
"  {\"act\":\"exec\",\"cmd\":\"<shell command>\",\"why\":\"<一句说明>\"}\n"
"  {\"act\":\"mind\",\"op\":\"add|read\",\"target\":\"tree|palace\",\"text\":\"...\"}\n"
"  {\"act\":\"answer\",\"text\":\"<final reply>\"}\n"
"\n"
"- 一次只跑第一个 JSON。看完这一步的结果，再写下一步。\n"
"- 读文件用 line 和 n。这一窗没到文件末尾时，结果里写下一窗的 line。\n"
"- file 读写普通文件。exec 只跑 /bin/sh -c。纯 cd <dir> 记住目录，后面的 exec 和 file 跟着走，不要每步再 cd。\n"
"- mind 只碰两页，都在 ~/.csih，不跟工作目录。tree 是 ~/.csih/思维树.md，格式 markdown-tree-dag：一层缩进的 markdown 树，├── 与 └── 表示包含，══> 表示跨枝依赖，不是散文。palace 是 ~/.csih/记忆宫殿.md，格式 mermaid-flowchart-memory-palace：一整段 ```mermaid flowchart，边表示树里放不好的关系。没有目录就建。op=add 只追加一行短注，仍以 \"- \" 开头，不会改写整页。op=read 返回该文件。这两页不要用 file 或 exec。\n"
"- 停在工具结果写明的工作目录。用户没点别的目录就不要搜整盘。\n"
"- 先做用户的任务。要记住或计划时用 mind。做完就 answer，不要再调用工具。\n"
"- 已经 answer 之后，下一步就输出 {\"go\":\"stop\"} 结束；只有还剩具体一步没做时才 continue。\n"
"- 还没 answer 时，前两次 stop 不结束、会再问一次；做完就 answer，再 stop。\n"
"- 用户这句话是唯一任务。用户没写 tmux、窗口或窗名，就不要 exec tmux，也不要在 answer 里谈窗口。\n"
"- 用户说不用工具时，第一步就 answer。\n"
"- 若出现「上文有省略」，那一句只说明较早的工具结果或助手行被拿掉了。留下的用户原话没有改写。\n";

/* The same bytes spliced into the model request. Selftest reads this pointer. */
const char *agent_model_rules(void) { return AGENT_SYSTEM_PROMPT; }

/* Round-end judgment. With an answer already given, the first stop ends
 * the turn. Before any answer, an early stop is retried up to MAX_JUDGE. */
#define MAX_JUDGE    3
#define MAX_ROUNDS   8
#define MAX_ACTIONS  16
#define MAX_MODEL_REPLIES 128 /* hard cap on model calls per user turn; binds all branches */
#define MAX_CTX_RECS 50      /* last N eligible transcript records sent as context */
/* The journal outlives one process. unisacc's cache re-exec drops setenv,
 * so the default path is derived from HOME, which the host still has. */
#define AGENT_JOURNAL_MAX  (256 * 1024)
#define AGENT_JOURNAL_KEEP 200

/* ── JSON string escaping (for building the request body) ────────────────── */

/* Largest prefix that ends on a complete UTF-8 character. */
static size_t utf8_prefix(const char *s, size_t n) {
    size_t i;
    int cont, need;
    unsigned char c;
    if (!s) return 0;
    i = n;
    cont = 0;
    while (i > 0 && ((unsigned char)s[i - 1] & 0xc0) == 0x80 && cont < 3) {
        i--;
        cont++;
    }
    if (i == 0) return 0;
    c = (unsigned char)s[i - 1];
    if ((c & 0x80) == 0) need = 1;
    else if ((c & 0xe0) == 0xc0) need = 2;
    else if ((c & 0xf0) == 0xe0) need = 3;
    else if ((c & 0xf8) == 0xf0) need = 4;
    else { return i > 0 ? i - 1 : 0; }
    if (need == cont + 1) return n;
    return i > 0 ? i - 1 : 0;
}

static size_t agent_json_str(const char *in, char *out, size_t outlen) {
    return json_escape(in, out, outlen, NULL);
}

/* A tool result longer than this is cut in the transcript.
 * A file under ~/.csih/tool is written only after agent_set_spill(1),
 * which the CLI turns on for the word "spill". */
#define AGENT_SPILL_AT 2000
static int agent_spill_enabled;

void agent_set_spill(int on) { agent_spill_enabled = on ? 1 : 0; }

/* judge is 1-based. Once an answer has been given, the first stop ends the
 * turn (answered != 0). Before any answer, only the last allowed judgment
 * may stop; earlier stops are retried. */
int agent_may_stop_ans(int judge, int maxn, int answered) {
    if (answered) return 1;
    if (maxn < 1) maxn = 1;
    return judge >= maxn;
}

int agent_may_stop(int judge, int maxn) {
    return agent_may_stop_ans(judge, maxn, 0);
}

/* The window list is not part of a question about the language or a library. */
int agent_mentions_window(const char *s) {
    if (!s || !s[0]) return 0;
    if (strstr(s, "tmux")) return 1;
    if (strstr(s, "窗口")) return 1;
    if (strstr(s, "窗名")) return 1;
    if (strstr(s, "capture-pane")) return 1;
    if (strstr(s, "send-keys")) return 1;
    return 0;
}

/* Write every byte of a long tool result under ~/.csih/tool/. The transcript
 * then holds the path and the first few lines, not a cut that drops the rest.
 * Returns 1 when `out` is that short form. Returns 0 if nothing was stored. */
static int agent_spill(const char *body, size_t n, char *out, size_t outlen) {
    const char *home;
    char dir[512], path[640];
    FILE *f;
    size_t i, lines, shown;
    static int seq;
    if (!agent_spill_enabled) return 0;
    if (!body || n <= AGENT_SPILL_AT || !out || outlen < 96) return 0;
    home = getenv("HOME");
    if (!home || !home[0]) return 0;
    csih_home_bind(home);
    snprintf(dir, sizeof dir, "%s/.csih", home);
    mkdir(dir, 0700);
    snprintf(dir, sizeof dir, "%s/.csih/tool", home);
    if (mkdir(dir, 0700) != 0 && errno != EEXIST) return 0;
    snprintf(path, sizeof path, "%s/%d-%d.txt", dir, (int)getpid(), ++seq);
    f = fopen(path, "w");
    if (!f) return 0;
    if (fwrite(body, 1, n, f) != n) { fclose(f); remove(path); return 0; }
    if (fclose(f) != 0) { remove(path); return 0; }
    lines = 0;
    shown = 0;
    for (i = 0; i < n && shown < 480 && lines < 8; i++) {
        if (body[i] == '\n') lines++;
        shown = i + 1;
    }
    shown = utf8_prefix(body, shown);
    snprintf(out, outlen, "full: %s (%zu bytes)\n%.*s%s",
             path, n, (int)shown, body, shown < n ? "\n..." : "");
    return 1;
}

/* A long tool result keeps its head and its tail. The middle is the part
 * that can go. The tail is where a file window names the next line. */
size_t agent_pack_tool(const char *text, char *out, size_t outlen, size_t cap) {
    const char *mark = "\n...(中间略)...\n";
    size_t n, markn, head, tail, ts, used;
    if (!out || outlen < 8) return 0;
    if (!text) text = "";
    n = strlen(text);
    if (cap > outlen - 1) cap = outlen - 1;
    if (cap < 8) cap = 8;
    if (n <= cap) {
        n = utf8_prefix(text, n);
        memcpy(out, text, n);
        out[n] = 0;
        return n;
    }
    markn = strlen(mark);
    if (cap <= markn + 8) {
        n = utf8_prefix(text, cap);
        memcpy(out, text, n);
        out[n] = 0;
        return n;
    }
    head = (cap - markn) * 2 / 3;
    tail = cap - markn - head;
    head = utf8_prefix(text, head);
    if (tail > n) tail = n;
    ts = n - tail;
    while (ts < n && ((unsigned char)text[ts] & 0xc0) == 0x80) ts++;
    used = head + markn + (n - ts);
    if (used > cap && head > used - cap) {
        head = utf8_prefix(text, head - (used - cap));
        used = head + markn + (n - ts);
    }
    if (used > outlen - 1) return 0;
    memcpy(out, text, head);
    memcpy(out + head, mark, markn);
    memcpy(out + head + markn, text + ts, n - ts);
    out[used] = 0;
    return used;
}

/* The record must stay valid JSON. A cut through a UTF-8 byte is a 400. */
int agent_tool_record(char *rec, size_t recsz, const char *name, const char *text) {
    char tmp[AGENT_RESULT_MAX];
    size_t cap = 1600;
    int i;
    if (!rec || recsz < 64) return 0;
    for (i = 0; i < 8; i++) {
        size_t recn;
        agent_pack_tool(text, tmp, sizeof tmp, cap);
        recn = json_rec(rec, recsz, "role", "tool", "name", name ? name : "tool",
                        "text", tmp);
        if (recn > 0 && rec[0] == '{' && rec[recn - 1] == '}') return 1;
        if (cap <= 64) break;
        cap = cap > 240 ? cap - 240 : 64;
    }
    json_rec(rec, recsz, "role", "tool", "name", name ? name : "tool",
             "text", "truncated");
    return 1;
}

/* ── strip a possible ```json ... ``` fence from model content ───────────── */

static void agent_strip_fence(const char *in, char *out, size_t outlen) {
    size_t i = 0, o = 0, n = strlen(in);
    while (i < n && (in[i] == ' ' || in[i] == '\n' || in[i] == '\r' || in[i] == '\t')) i++;
    if (strncmp(in + i, "```json", 7) == 0) { i += 7; while (i < n && in[i] != '\n') i++; if (i<n)i++; }
    else if (strncmp(in + i, "```", 3) == 0) { i += 3; while (i < n && in[i] != '\n') i++; if (i<n)i++; }
    while (i < n && o < outlen - 1) {
        if (strncmp(in + i, "```", 3) == 0) break;
        out[o++] = in[i++];
    }
    while (o > 0 && (out[o-1] == ' ' || out[o-1] == '\n' || out[o-1] == '\r' || out[o-1] == '\t')) o--;
    out[o] = '\0';
}

/* ── parse one model content string into a step ─────────────────────────── */

/* How many JSON objects are in the text. The loop runs only the first.
 * The model sees that result, then writes the next step itself. */
/* The user asked for words only. A later tool call is still a tool call.
 * Bare prose is the answer, even when it contains a brace from C code. */
int agent_user_forbids_tools(const char *prompt) {
    if (!prompt) return 0;
    if (strstr(prompt, "不要用工具")) return 1;
    if (strstr(prompt, "不用工具")) return 1;
    if (strstr(prompt, "不要使用工具")) return 1;
    return 0;
}

int agent_take_prose(agent_step *s, const char *content, int no_tools) {
    if (!s || !no_tools || s->kind != ACT_ERR) return 0;
    if (!content || !content[0]) return 0;
    s->kind = ACT_ANSWER;
    snprintf(s->text, sizeof s->text, "%s", content);
    return 1;
}

int agent_object_count(const char *s) {
    size_t i = 0, nlen;
    int n = 0;
    if (!s) return 0;
    nlen = strlen(s);
    while (i < nlen) {
        size_t e = json_value_end(s + i, nlen - i);
        size_t k = i;
        unsigned char c;
        if (e == 0) {
            while (i < nlen && (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r')) i++;
            if (i >= nlen) break;
            c = (unsigned char)s[i];
            if (c == '{' || c == '[' || c == '"' || c == '-' ||
                (c >= '0' && c <= '9') || c == 't' || c == 'f' || c == 'n')
                break;
            i++;
            continue;
        }
        while (k < i + e && (s[k] == ' ' || s[k] == '\t' || s[k] == '\n' || s[k] == '\r')) k++;
        if (k < nlen && s[k] == '{') n++;
        i += e;
    }
    return n;
}

agent_step agent_parse(const char *content) {
    agent_step s;
    char stripped[AGENT_CONTENT_MAX];
    char errbuf[128];
    jvalue *root, *v;
    const char *p;

    memset(&s, 0, sizeof s);
    s.kind = ACT_ERR;
    if (!content || !*content) return s;

    agent_strip_fence(content, stripped, sizeof stripped);
    /* Models often write a sentence and then several JSON objects.
     * Take the first balanced object. The rest is not a second action. */
    {
        char one[AGENT_CONTENT_MAX];
        const char *q = strchr(stripped, '{');
        size_t e;
        if (!q) return s;
        e = json_value_end(q, strlen(q));
        if (e == 0 || e + 1 > sizeof one) return s;
        memcpy(one, q, e);
        one[e] = 0;
        memcpy(stripped, one, e + 1);
    }
    root = json_parse(stripped, strlen(stripped), errbuf, sizeof errbuf);
    if (!root || root->kind != J_OBJ) { if (root) jfree(root); return s; }

    v = jget(root, "go");
    if (v && v->kind == J_STR) {
        p = jstr(v);
        if (strcmp(p, "continue") == 0) s.kind = ACT_GO_CONTINUE;
        else if (strcmp(p, "stop") == 0) s.kind = ACT_GO_STOP;
        jfree(root); return s;
    }

    v = jget(root, "act");
    if (v && v->kind == J_STR) {
        int k;
        p = jstr(v);
        k = plugin_kind(p);
        if (k == ACT_EXEC) {
            s.kind = ACT_EXEC;
            v = jget(root, "cmd");
            if (v && v->kind == J_STR) { strncpy(s.cmd, jstr(v), sizeof s.cmd - 1); }
            v = jget(root, "why");
            if (v && v->kind == J_STR) { strncpy(s.why, jstr(v), sizeof s.why - 1); }
        } else if (k == ACT_MIND) {
            s.kind = ACT_MIND;
            v = jget(root, "op");
            if (v && v->kind == J_STR) strncpy(s.op, jstr(v), sizeof s.op - 1);   /* op: read|add */
            v = jget(root, "target");
            if (!v || v->kind != J_STR) v = jget(root, "which");
            if (v && v->kind == J_STR) strncpy(s.path, jstr(v), sizeof s.path - 1); /* target or which */
            v = jget(root, "text");
            if (v && v->kind == J_STR) strncpy(s.text, jstr(v), sizeof s.text - 1);
        } else if (k == ACT_READ) {
            jvalue *op = jget(root, "op");
            const char *ops = (op && op->kind == J_STR) ? jstr(op) : "";
            if (strcmp(ops, "read") == 0) {
                s.kind = ACT_READ;
                v = jget(root, "path");
                if (v && v->kind == J_STR) strncpy(s.path, jstr(v), sizeof s.path - 1);
                v = jget(root, "line");
                if (v && v->kind == J_NUM) s.line = (int)v->n;
                else if (v && v->kind == J_STR) s.line = atoi(jstr(v));
                v = jget(root, "n");
                if (v && v->kind == J_NUM) s.nlines = (int)v->n;
                else if (v && v->kind == J_STR) s.nlines = atoi(jstr(v));
            } else if (strcmp(ops, "write") == 0) {
                s.kind = ACT_WRITE;
                v = jget(root, "path");
                if (v && v->kind == J_STR) strncpy(s.path, jstr(v), sizeof s.path - 1);
                v = jget(root, "text");
                if (v && v->kind == J_STR) strncpy(s.text, jstr(v), sizeof s.text - 1);
            } else if (strcmp(ops, "edit") == 0) {
                s.kind = ACT_EDIT;
                v = jget(root, "path");
                if (v && v->kind == J_STR) strncpy(s.path, jstr(v), sizeof s.path - 1);
                v = jget(root, "old");
                if (v && v->kind == J_STR) strncpy(s.old, jstr(v), sizeof s.old - 1);
                v = jget(root, "new");
                if (v && v->kind == J_STR) strncpy(s.nw, jstr(v), sizeof s.nw - 1);
            }
        } else if (k == ACT_ANSWER) {
            s.kind = ACT_ANSWER;
            v = jget(root, "text");
            if (v && v->kind == J_STR) strncpy(s.text, jstr(v), sizeof s.text - 1);
        }
    }
    jfree(root);
    return s;
}

/* ── execute a parsed step; result text (a summary) goes in `out` ────────────
 * returns 1 if the step was a valid tool call that ran, 0 if malformed. */

/* A relative tool path is under the agent cwd, not the process cwd.
 * An absolute path is left alone. */
static void agent_under(const char *cwd, const char *in, char *out, size_t n) {
    if (!in) { if (n) out[0] = 0; return; }
    if (!cwd || !cwd[0] || in[0] == '/') snprintf(out, n, "%s", in);
    else snprintf(out, n, "%s/%s", cwd, in);
}

file_result file_list(const char *dir, char *out, size_t cap);

/* A command that is only `cd` or `cd <dir>` updates cwd for later steps.
 * Returns 1 if this command was that builtin (caller must not also run a shell).
 * `cd foo && bar` is not this builtin. */
int agent_note_cd(char *cwd, size_t cwdlen, const char *cmd, char *out, size_t outlen) {
    const char *p, *end;
    char dir[1024], next[1024], probe[8];
    file_result fr;
    size_t n;
    if (!cwd || !cmd || !out) return 0;
    p = cmd;
    while (*p == ' ' || *p == '\t') p++;
    if (p[0] != 'c' || p[1] != 'd') return 0;
    if (p[2] != 0 && p[2] != ' ' && p[2] != '\t') return 0;
    p += 2;
    while (*p == ' ' || *p == '\t') p++;
    end = p + strlen(p);
    while (end > p && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\n')) end--;
    if (strstr(p, "&&") || strchr(p, ';') || strchr(p, '|')) return 0;
    n = (size_t)(end - p);
    if (n >= sizeof dir) { snprintf(out, outlen, "path too long"); return 1; }
    memcpy(dir, p, n);
    dir[n] = 0;
    if (!dir[0]) { snprintf(out, outlen, "cwd: %s", cwd); return 1; }
    if (n >= 2 && ((dir[0] == '"' && dir[n - 1] == '"') || (dir[0] == '\'' && dir[n - 1] == '\''))) {
        memmove(dir, dir + 1, n - 2);
        dir[n - 2] = 0;
    }
    if (dir[0] == '/') snprintf(next, sizeof next, "%s", dir);
    else snprintf(next, sizeof next, "%s/%s", cwd, dir);
    fr = file_list(next, probe, sizeof probe);
    if (!fr.ok && fr.err != ENOSPC) {
        snprintf(out, outlen, "cannot cd to %s (errno %d)", next, fr.err);
        return 1;
    }
    if (strlen(next) >= cwdlen) { snprintf(out, outlen, "path too long"); return 1; }
    snprintf(cwd, cwdlen, "%s", next);
    snprintf(out, outlen, "cwd is now %s", cwd);
    return 1;
}

/* Red sticks in this process until a later source write is all green.
 * The table is not linked here: shell_run_in asks suite_cli. */
static int agent_red;
static char agent_why[180];

static void agent_mark(int red, const char *why) {
    agent_red = red ? 1 : 0;
    if (!agent_red) { agent_why[0] = 0; return; }
    snprintf(agent_why, sizeof agent_why, "%s", why ? why : "slice red");
}

static int agent_failing(char *why, int n) {
    if (!agent_red) return 0;
    if (why && n > 0) snprintf(why, (size_t)n, "%s", agent_why);
    return 1;
}

static int agent_csih_src(const char *path, const char *cwd) {
    const char *base, *dot;
    if (!path || !path[0]) return 0;
    base = strrchr(path, '/');
    base = base ? base + 1 : path;
    dot = strrchr(base, '.');
    if (!dot || (strcmp(dot, ".c") != 0 && strcmp(dot, ".h") != 0)) return 0;
    if (strstr(path, "/apps/csih/")) return 1;
    if (cwd && strstr(cwd, "/apps/csih") && !strchr(path, '/')) return 1;
    return 0;
}

static void agent_first_line(const char *s, char *line, int n) {
    int p = 0;
    if (!s) s = "";
    while (s[p] && s[p] != '\n' && p + 1 < n) {
        line[p] = s[p];
        p++;
    }
    line[p] = 0;
}

/* Spawn the slice rows that name this source. shell_run_in is already in
 * this program. Linking suite.c here overflows unisacc's struct ids once
 * net.c's netdb.h is in the same image. */
static int agent_slice(const char *path, const char *cwd, char *note, int nlen) {
    const char *base, *bin, *root;
    char cmd[1800], listing[4096];
    shell_result r;
    int any = 0, bad = 0, used = 0, p = 0;
    if (note && nlen > 0) note[0] = 0;
    if (!agent_csih_src(path, cwd)) return 0;
    base = strrchr(path, '/');
    base = base ? base + 1 : path;
    bin = getenv("UNISACC");
    if (!bin || !bin[0]) bin = "/Users/wjc/repos/unisacc/unisacc.com";
    root = (cwd && cwd[0]) ? cwd : ".";
    snprintf(cmd, sizeof cmd, "exec \"%s\" suite.c suite_cli.c rows %s", bin, base);
    r = shell_run_in(cmd, root);
    if (!r.ok || !r.exited || r.status != 0) {
        char line[160], why[180];
        int rc = r.exited ? r.status : -1;
        agent_first_line(r.out, line, (int)sizeof line);
        snprintf(why, sizeof why, "slice rows rc=%d %s", rc, line[0] ? line : "(no output)");
        agent_mark(1, why);
        if (note && nlen > 0) snprintf(note, (size_t)nlen, "%s", why);
        return -1;
    }
    snprintf(listing, sizeof listing, "%s", r.out);
    while (listing[p]) {
        char line[700], name[40], args[640], why[180], first[160];
        int i = 0, a = 0, rc;
        while (listing[p] && listing[p] != '\n' && i + 1 < (int)sizeof line)
            line[i++] = listing[p++];
        if (listing[p] == '\n') p++;
        line[i] = 0;
        if (!line[0]) continue;
        if (!strcmp(line, "none")) break;
        i = 0;
        while (line[i] && line[i] != ' ' && i + 1 < (int)sizeof name) {
            name[i] = line[i];
            i++;
        }
        name[i] = 0;
        if (line[i] == ' ') i++;
        while (line[i] && a + 1 < (int)sizeof args) args[a++] = line[i++];
        args[a] = 0;
        if (!args[0]) continue;
        snprintf(cmd, sizeof cmd, "exec \"%s\" %s", bin, args);
        r = shell_run_in(cmd, root);
        rc = r.exited ? r.status : -1;
        agent_first_line(r.out, first, (int)sizeof first);
        any = 1;
        snprintf(why, sizeof why, "slice %s rc=%d %s", name[0] ? name : "?", rc,
                 first[0] ? first : "(no output)");
        if (note && nlen > used + 8) {
            int w = snprintf(note + used, (size_t)(nlen - used), "%s%s",
                             used ? "; " : "", why);
            if (w > 0) used += w;
        }
        if (rc != 0) { bad = 1; agent_mark(1, why); }
    }
    if (!any) return 0;
    if (bad) return -1;
    agent_mark(0, NULL);
    return 1;
}

static char g_role_force[16];
static char g_peer_force[64];
static int g_watch_mailed;
static int g_watch_deny;

void agent_role_test(const char *role, const char *peer) {
    snprintf(g_role_force, sizeof g_role_force, "%s", role ? role : "");
    snprintf(g_peer_force, sizeof g_peer_force, "%s", peer ? peer : "");
    g_watch_mailed = 0;
}

void agent_watch_reset(void) { g_watch_mailed = 0; g_watch_deny = 0; }

static const char *agent_role_get(void) {
    if (g_role_force[0]) return g_role_force;
    {
        const char *v = getenv("CSIH_ROLE");
        return (v && v[0]) ? v : "";
    }
}

static const char *agent_peer_get(void) {
    if (g_peer_force[0]) return g_peer_force;
    {
        const char *v = getenv("CSIH_PEER");
        return (v && v[0]) ? v : "";
    }
}

static int agent_peer_name_ok(const char *p) {
    int n = 0;
    if (!p) return 0;
    for (; *p; p++, n++) {
        char c = *p;
        int ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
            || (c >= '0' && c <= '9') || c == ':' || c == '_' || c == '-';
        if (!ok || n >= 48) return 0;
    }
    return n > 0;
}

void agent_watch_note(const char *cmd) {
    const char *peer = agent_peer_get();
    if (cmd && strstr(cmd, "envelope") && agent_peer_name_ok(peer) && strstr(cmd, peer))
        g_watch_mailed = 1;
}

int agent_watch_needs_mail(void) {
    return strcmp(agent_role_get(), "watch") == 0 && !g_watch_mailed;
}

static int agent_watch_exec_ok(const char *cmd, const char *peer) {
    if (!cmd || !agent_peer_name_ok(peer)) return 0;
    if (strstr(cmd, "respawn") || strstr(cmd, "csih.sh") || strstr(cmd, "make "))
        return 0;
    if (!strstr(cmd, peer)) return 0;
    if (strstr(cmd, "capture-pane")) return 1;
    if (strstr(cmd, "envelope")) return 1;
    return 0;
}

/* Watch may read, capture the peer, and envelope the peer. It may not write. */
int agent_peer_blocked(int kind, const char *op, const char *cmd, char *why, int n) {
    const char *role = agent_role_get();
    const char *peer = agent_peer_get();
    if (why && n > 0) why[0] = 0;
    if (strcmp(role, "watch") != 0) return 0;
    if (kind == ACT_READ) return 0;
    if (kind == ACT_MIND && op && strcmp(op, "read") == 0) return 0;
    if (kind == ACT_EXEC && agent_watch_exec_ok(cmd, peer)) return 0;
    if (why && n > 1)
        snprintf(why, (size_t)n, "%s",
                 "看客不改源码。只许读、capture-pane 同伴、给同伴发 envelope。");
    return 1;
}

static int agent_role_line(char *dst, int n) {
    const char *role = agent_role_get();
    const char *peer = agent_peer_get();
    if (!dst || n < 8 || !agent_peer_name_ok(peer)) return 0;
    if (!strcmp(role, "write"))
        return snprintf(dst, (size_t)n,
                        "\n写手。同伴 %s。只做同伴信封里的一件。做完执行 /Users/wjc/repos/moltbaby/bin/envelope %s 回一句。不重启。\n",
                        peer, peer);
    if (!strcmp(role, "watch"))
        return snprintf(dst, (size_t)n,
                        "\n看客。同伴 %s。用户点了这个窗口时，只许 tmux capture-pane -p -t %s，以及 /Users/wjc/repos/moltbaby/bin/envelope %s 送一件。不改源码，不编译，不重启。\n",
                        peer, peer, peer);
    return 0;
}

int agent_exec(const agent_step *s, const char *cwd, char *out, size_t outlen) {
    char path[512];
    out[0] = '\0';
    agent_under(cwd, s->path, path, sizeof path);
    switch (s->kind) {
    case ACT_EXEC:
        g_exec_step = *s;
        g_exec_cwd = cwd;
        plugin_register_once(&exec_row, &exec_registered);
        return plugin_run("exec", s->cmd, out, outlen);
    case ACT_READ:
    case ACT_WRITE:
    case ACT_EDIT:
        g_file_step = *s;
        g_file_cwd = cwd;
        plugin_register_once(&file_row, &file_registered);
        return plugin_run("file", s->path, out, outlen);
    case ACT_MIND:
        g_mind_step = *s;
        g_mind_cwd = cwd;
        plugin_register_once(&mind_row, &mind_registered);
        return plugin_run("mind", s->path, out, outlen);
    default:
        snprintf(out, outlen, "unrecognized step");
        return 0;
    }
}

static int agent_file(const agent_step *s, const char *cwd, char *out, int outlen) {
    char path[512];
    agent_under(cwd, s->path, path, sizeof path);
    if (s->kind == ACT_READ) {
        FILE *f;
        char row[2048];
        int start, want, total = 0, shown = 0, last = 0, full = 0;
        size_t used = 0;
        if (!s->path[0]) return 0;
        start = s->line < 1 ? 1 : s->line;
        want = s->nlines < 1 ? 120 : s->nlines;
        if (want > 400) want = 400;
        f = fopen(path, "r");
        if (!f) {
            snprintf(out, outlen, "read failed (err=%d)", errno);
            return 1;
        }
        while (fgets(row, sizeof row, f)) {
            size_t L;
            total++;
            if (total < start || full) continue;
            L = strlen(row);
            while (L && (row[L - 1] == '\n' || row[L - 1] == '\r')) row[--L] = 0;
            if (shown >= want || used + L + 160 >= outlen) { full = 1; continue; }
            used += (size_t)snprintf(out + used, outlen - used, "%s\n", row);
            shown++;
            last = total;
        }
        fclose(f);
        if (shown == 0) {
            snprintf(out, outlen, "line %d 超过文件末尾，共 %d 行", start, total);
            return 1;
        }
        if (last < total) {
            snprintf(out + used, outlen - used,
                     "第 %d-%d 行，共 %d 行。下一窗 line=%d。",
                     start, last, total, last + 1);
        } else {
            snprintf(out + used, outlen - used,
                     "第 %d-%d 行，共 %d 行。", start, last, total);
        }
        return 1;
    }
    if (s->kind == ACT_WRITE) {
        file_result r = file_write(path, s->text, strlen(s->text));
        snprintf(out, outlen, r.ok ? "wrote %ld bytes" : "write failed (err=%ld)",
                 r.ok ? r.bytes : (long)r.err);
        if (r.ok) {
            char note[500];
            int g = agent_slice(path, cwd, note, (int)sizeof note);
            if (g != 0 && note[0]) {
                char merged[AGENT_RESULT_MAX];
                snprintf(merged, sizeof merged, "%s\n%s", out, note);
                snprintf(out, outlen, "%s", merged);
            }
        }
        return (s->path[0] && s->text[0]) ? 1 : 0;
    }
    /* ACT_EDIT */
    {
        edit_result r = edit_replace(path, s->old, s->nw);
        if (!r.ok) snprintf(out, outlen, "edit failed (err=%d)", r.err);
        else if (r.count == 0) snprintf(out, outlen, "edit: old text not found");
        else if (r.count > 1) snprintf(out, outlen, "edit: ambiguous (%ld matches)", r.count);
        else snprintf(out, outlen, "edited (%ld bytes)", r.bytes);
        if (r.ok && r.count == 1) {
            char note[500];
            int g = agent_slice(path, cwd, note, (int)sizeof note);
            if (g != 0 && note[0]) {
                char merged[AGENT_RESULT_MAX];
                snprintf(merged, sizeof merged, "%s\n%s", out, note);
                snprintf(out, outlen, "%s", merged);
            }
        }
        return (s->path[0] && s->old[0]) ? 1 : 0;
    }
}

static int agent_mind(const agent_step *s, const char *cwd, char *out, int outlen) {
        /* which: tree → ~/.csih/思维树.md (markdown-tree-dag),
         * palace → ~/.csih/记忆宫殿.md (mermaid-flowchart-memory-palace).
         * op is s->op: "read" returns the file, "add" appends one
         * "- " note. It does not rewrite the page into that shape. */
        const char *name = plugin_page(s->path);
        char mp[2048];
        if (!name || !name[0]) { snprintf(out, outlen, "mind: target must be tree or palace"); return 0; }
        if (!page_path(mp, (int)sizeof mp, cwd, s->path)) {
            snprintf(out, outlen, "mind: no path for %s", s->path);
            return 0;
        }
        if (!strcmp(s->op, "read")) {
            char *buf = (char *)malloc(NET_BODY_MAX);
            file_result r;
            if (!buf) { snprintf(out, outlen, "oom"); return 0; }
            r = file_read(mp, buf, NET_BODY_MAX);
            if (!r.ok && r.err == -1001)
                snprintf(out, outlen, "full: %s (%ld bytes)\nfile is larger than the read buffer; open that path",
                         mp, r.bytes);
            else if (!r.ok) snprintf(out, outlen, "%s: read failed (err=%d)", name, r.err);
            else if (r.bytes > AGENT_SPILL_AT && agent_spill(buf, (size_t)r.bytes, out, outlen)) {
                /* full page is the file named in out */
            } else {
                long cap = (long)(outlen - 64);
                long n = r.bytes < cap ? r.bytes : cap;
                snprintf(out, outlen, "%s\n%.*s%s", name, (int)n, buf,
                         r.bytes > cap ? "\n... (truncated)" : "");
            }
            free(buf);
            return 1;
        } else if (!strcmp(s->op, "add")) {
            /* file_append_line() owns the line boundary: it creates the file
             * and inserts a separating newline if the file lacks one, so pass
             * the "- " text WITHOUT a trailing \n. */
            char line[4200];
            file_result r;
            snprintf(line, sizeof line, "- %s", s->text);
            r = file_append_line(mp, line);
            if (!r.ok) snprintf(out, outlen, "%s: append failed (err=%d)", name, r.err);
            else snprintf(out, outlen, "%s: appended", name);
            return 1;
        }
        snprintf(out, outlen, "mind: op must be read or add");
        return 0;
}

/* plugin_run("mind") reaches here: the static step kept by agent_exec. */
static int mind_run(const char *arg, char *out, int outlen) {
    (void)arg;
    return agent_mind(&g_mind_step, g_mind_cwd, out, outlen);
}

static cdsh_plugin mind_row = { "mind", "tree|palace add|read", mind_run, 0 };

static void plugin_register_once(cdsh_plugin *row, int *flag) {
    if (!*flag) { *flag = 1; plugin_register(row); }
}

/* plugin_run("file") reaches here: the static step kept by agent_exec. */
static int file_run(const char *arg, char *out, int outlen) {
    (void)arg;
    return agent_file(&g_file_step, g_file_cwd, out, outlen);
}

static cdsh_plugin file_row = { "file", "read|write|edit", file_run, 0 };

/* plugin_run("exec") reaches here; runs the current step's command. */
static int exec_run(const char *arg, char *out, int outlen) {
    const agent_step *s = &g_exec_step;
    const char *cwd = g_exec_cwd;
    shell_result r;
    long cap, n;
    int trunc = 0;
    char head[160];
    (void)arg;
    r = shell_run_in(s->cmd, cwd);
    cap = (long)(outlen - 64);
    n = r.bytes;
    if (n < 0) { n = (long)strlen(r.out); trunc = 1; }
    if (r.timed_out)
        snprintf(head, sizeof head, "cwd=%s\nexit=timeout\n",
                 cwd && cwd[0] ? cwd : ".");
    else
        snprintf(head, sizeof head, "cwd=%s\nexit=%d\n",
                 cwd && cwd[0] ? cwd : ".", r.exited ? r.status : -1);
    if (n > AGENT_SPILL_AT && agent_spill(r.out, (size_t)n, out, outlen)) {
        char merged[AGENT_RESULT_MAX];
        snprintf(merged, sizeof merged, "%s%s%s", head, out,
                 trunc ? "\n(capture stopped at shell buffer)" : "");
        snprintf(out, outlen, "%s", merged);
        return s->cmd[0] ? 1 : 0;
    }
    if (n > cap) { n = cap; trunc = 1; }
    if (n < 0) n = 0;
    snprintf(out, outlen, "%s%.*s%s", head, (int)n, r.out,
             trunc ? "\n... (truncated)" : "");
    return s->cmd[0] ? 1 : 0;
}

static cdsh_plugin exec_row = { "exec", "shell", exec_run, 0 };

/* Chat completions accept only system/user/assistant. Transcript roles such as
 * tool stay on disk, but go out as a user turn (wrap=1) so the model still
 * sees the result. A bare role=tool has no tool_call_id and DeepSeek returns
 * HTTP 400, which the loop then treats as a dead model. */
int agent_chat_role(const char *role, char *out, size_t outlen, int *wrap) {
    if (wrap) *wrap = 0;
    if (!role || !role[0] || !out || outlen < 16) return 0;
    if (!strcmp(role, "system") || !strcmp(role, "user") ||
        !strcmp(role, "assistant")) {
        snprintf(out, outlen, "%s", role);
        return 1;
    }
    snprintf(out, outlen, "user");
    if (wrap) *wrap = 1;
    return 1;
}

/* One journal for this home. An explicit CSIH_TRANSCRIPT / CDSH_TRANSCRIPT
 * still wins (tests and probes). Otherwise $HOME/.cdsh/tui.jsonl.
 * Returns 0 when out holds a path. Does not create or delete the file. */
int agent_transcript_path(char *out, size_t outlen) {
    const char *env, *home;
    if (!out || outlen < 16) return 1;
    out[0] = '\0';
    env = getenv("CSIH_TRANSCRIPT");
    if (!env || !env[0]) env = getenv("CDSH_TRANSCRIPT");
    if (env && env[0]) {
        snprintf(out, outlen, "%s", env);
        return out[0] ? 0 : 1;
    }
    home = getenv("HOME");
    if (!home || !home[0]) return 1;
    snprintf(out, outlen, "%s/.cdsh/tui.jsonl", home);
    return 0;
}

/* First index of the contiguous newest slice that fits.
 * count records, at most max_recs, costs[i] bytes, budget bytes.
 * A full budget drops the oldest. If nothing fits, returns count.
 * The wire pack uses agent_ctx_pick, which drops tool rows before user rows. */
int agent_ctx_start(int count, int max_recs, const int *costs, int budget) {
    int start, i, used;
    if (count < 0) count = 0;
    if (max_recs < 1) max_recs = 1;
    if (budget < 0) budget = 0;
    start = count > max_recs ? count - max_recs : 0;
    used = 0;
    for (i = count - 1; i >= start; i--) {
        int c = costs ? costs[i] : 0;
        if (c < 0) c = 0;
        if (used + c > budget) return i + 1;
        used += c;
    }
    return start;
}

/* One user line, sent only when an older row was left out of this request.
 * It says what was omitted. It does not rewrite any user text. */
static const char *AGENT_OMIT_NOTE =
    "上文有省略。被省略的是较早的工具结果和助手行。用户原话不改写。";

/* Mark which of the n records go out. The newest row always stays.
 * Pass 1 drops the oldest tool rows, pass 2 the oldest assistant rows,
 * pass 3 the oldest remaining rows. The first non-tool user row stays
 * until every other non-newest row is already gone. User text is copied
 * as written, not folded into a summary. */
static void agent_ctx_pick(int n, const int *costs, int budget,
                           char roles[][16], const int *wraps, int *use) {
    int sum = 0, i, pass, head = -1;
    if (n < 0) n = 0;
    if (budget < 0) budget = 0;
    for (i = 0; i < n; i++) {
        int tool = wraps && wraps[i];
        use[i] = 1;
        sum += costs && costs[i] > 0 ? costs[i] : 0;
        if (head < 0 && !tool && roles && roles[i][0] && !strcmp(roles[i], "user"))
            head = i;
    }
    for (pass = 0; pass < 3; pass++) {
        int guard = 0;
        while (sum > budget && guard < n) {
            int victim = -1;
            for (i = 0; i < n - 1; i++) {
                int tool, asst, later, j;
                if (!use[i]) continue;
                tool = wraps && wraps[i];
                asst = roles && roles[i][0] && !strcmp(roles[i], "assistant");
                if (pass == 0 && !tool) continue;
                if (pass == 1 && (tool || !asst)) continue;
                if (i == head) {
                    later = 0;
                    for (j = 0; j < n - 1; j++) {
                        if (j != head && use[j]) { later = 1; break; }
                    }
                    if (later) continue;
                }
                victim = i;
                break;
            }
            if (victim < 0) break;
            use[victim] = 0;
            if (costs && costs[victim] > 0) sum -= costs[victim];
            guard++;
        }
    }
}

static size_t agent_escaped_len(const char *in) {
    size_t n = in ? strlen(in) : 0;
    size_t cap = n * 6 + 8;
    char *buf;
    size_t got;
    if (cap < 8) cap = 8;
    buf = (char *)malloc(cap);
    if (!buf) return n;
    got = json_escape(in ? in : "", buf, cap, NULL);
    free(buf);
    return got;
}

/* Rewrite the journal down to the newest `keep` lines once it passes
 * max_bytes. Returns 1 when the file was replaced. */
int agent_journal_trim(const char *path, long max_bytes, int keep) {
    struct stat st;
    session_records recs;
    FILE *f;
    char tmp[600];
    size_t i, start;
    if (!path || !path[0] || keep < 1 || max_bytes < 1) return 0;
    if (stat(path, &st) != 0 || st.st_size < max_bytes) return 0;
    recs = session_read(path);
    if ((int)recs.count <= keep) { session_free(&recs); return 0; }
    start = recs.count - (size_t)keep;
    snprintf(tmp, sizeof tmp, "%s.csih-tmp", path);
    f = fopen(tmp, "w");
    if (!f) { session_free(&recs); return 0; }
    for (i = start; i < recs.count; i++) {
        if (fputs(recs.lines[i], f) < 0 || fputc('\n', f) == EOF) {
            fclose(f);
            remove(tmp);
            session_free(&recs);
            return 0;
        }
    }
    if (fclose(f) != 0) { remove(tmp); session_free(&recs); return 0; }
    if (rename(tmp, path) != 0) { remove(tmp); session_free(&recs); return 0; }
    session_free(&recs);
    return 1;
}

/* ── build the chat `messages` JSON from the transcript + system + tail ──────
 * writes `{"model":"__MODEL__","messages":[...],"stream":false}` into out.
 * The caller splices the real model name over __MODEL__. Returns byte count.
 * Eligible records go out oldest to newest among the rows that fit.
 * A full body drops tool rows before user rows, and keeps the first
 * user row until the newer rows are gone. When a row is left out, one
 * short note says so. A long tool row keeps its head and its tail.
 * decision records have no text and stay on disk only. */

static int agent_build_messages(const char *transcript, const char *tail,
                                const char *extra_system, char *out, size_t outlen) {
    session_records recs;
    char acc[NET_BODY_MAX];
    size_t a = 0, i, room, reserve, history_room;
    int send_extra;
    enum { CTX_N = MAX_CTX_RECS };
    jvalue *held[CTX_N];
    const char *texts[CTX_N];
    char api_roles[CTX_N][16];
    int wraps[CTX_N];
    int costs[CTX_N];
    int use[CTX_N];
    int nvals = 0, k;

    recs = session_read(transcript);
    memset(held, 0, sizeof held);

    /* System text stays byte-stable (catalog + prompt). The tmux window list
     * is caller text with newlines; it rides a user message below,
     * JSON-escaped, so it never lands inside this string. */
    {
        char sysbuf[NET_BODY_MAX];
        size_t sa = 0;
        sa += (size_t)snprintf(sysbuf + sa, sizeof sysbuf - sa, "Catalog:\n");
        sa += (size_t)plugin_catalog(sysbuf + sa, (int)(sizeof sysbuf - sa));
        sa += (size_t)snprintf(sysbuf + sa, sizeof sysbuf - sa, "\n%s", AGENT_SYSTEM_PROMPT);
        sa += (size_t)agent_role_line(sysbuf + sa, (int)(sizeof sysbuf - sa));
        a += json_msg(acc + a, sizeof acc - a, 0, "system", NULL, sysbuf);
    }

    for (i = 0; i < recs.count; i++) {
        char err[128];
        jvalue *v = json_parse(recs.lines[i], strlen(recs.lines[i]), err, sizeof err);
        const char *role = NULL, *text = NULL;
        char api_role[16];
        int wrap = 0;
        if (!v || v->kind != J_OBJ) { jfree(v); continue; }
        {
            jvalue *rv = jget(v, "role");
            jvalue *tv = jget(v, "text");
            if (rv && rv->kind == J_STR) role = jstr(rv);
            if (tv && tv->kind == J_STR) text = jstr(tv);
        }
        if (!role || !text || !agent_chat_role(role, api_role, sizeof api_role, &wrap)) {
            jfree(v);
            continue;
        }
        if (nvals >= CTX_N) {
            int s;
            jfree(held[0]);
            for (s = 0; s < CTX_N - 1; s++) {
                held[s] = held[s + 1];
                texts[s] = texts[s + 1];
                wraps[s] = wraps[s + 1];
                snprintf(api_roles[s], sizeof api_roles[0], "%s", api_roles[s + 1]);
            }
            nvals--;
        }
        held[nvals] = v;
        texts[nvals] = text;
        snprintf(api_roles[nvals], sizeof api_roles[0], "%s", api_role);
        wraps[nvals] = wrap;
        nvals++;
    }

    /* The caller passes the window list only on the first action call of
     * this user turn. A judgment tail is sent alone. An empty journal is
     * not the signal: the file now survives into the next turn. */
    send_extra = extra_system && extra_system[0] && !(tail && tail[0]);

    room = outlen < sizeof acc ? outlen : sizeof acc;
    room = room > 160 ? room - 160 : 0;
    reserve = 8;
    if (tail && tail[0]) reserve += 48 + agent_escaped_len(tail);
    else if (send_extra) reserve += 48 + agent_escaped_len(extra_system);
    if (a >= room) history_room = a;
    else {
        if (reserve > room - a) reserve = room - a;
        history_room = room - reserve;
    }
    for (k = 0; k < nvals; k++) {
        char packed[1704];
        const char *wire = texts[k];
        int n;
        if (wraps[k]) {
            agent_pack_tool(texts[k], packed, sizeof packed, 1600);
            wire = packed;
        }
        n = 32 + (int)strlen(api_roles[k]) + (int)agent_escaped_len(wire);
        if (wraps[k]) n += 16;
        costs[k] = n;
    }
    {
        int budget = a < history_room ? (int)(history_room - a) : 0;
        int omitted = 0;
        int note_cost = 32 + 4 + (int)agent_escaped_len(AGENT_OMIT_NOTE);
        agent_ctx_pick(nvals, costs, budget, api_roles, wraps, use);
        for (k = 0; k < nvals; k++) if (!use[k]) omitted = 1;
        /* Reserve the note before the second pick so later rows still fit. */
        if (omitted && note_cost > 0 && note_cost < budget) {
            agent_ctx_pick(nvals, costs, budget - note_cost, api_roles, wraps, use);
            omitted = 0;
            for (k = 0; k < nvals; k++) if (!use[k]) omitted = 1;
        } else {
            omitted = 0;
        }
        if (omitted && a + (size_t)note_cost < history_room && a + 32 < sizeof acc) {
            size_t before = a;
            size_t n = json_msg(acc + a, sizeof acc - a, 1, "user", NULL, AGENT_OMIT_NOTE);
            if (n && before + n < history_room) a += n;
            else acc[a] = '\0';
        }
    }
    for (k = 0; k < nvals; k++) {
        char packed[1704];
        const char *wire = texts[k];
        size_t before = a;
        if (!use[k]) continue;
        if (a + 64 >= history_room && k + 1 < nvals) continue;
        if (wraps[k]) {
            agent_pack_tool(texts[k], packed, sizeof packed, 1600);
            wire = packed;
        }
        {
            size_t n = json_msg(acc + a, sizeof acc - a, 1, api_roles[k],
                                 wraps[k] ? "[tool]\n" : NULL, wire);
            if (n) a += n;
        }
        if (a >= history_room && k + 1 < nvals) { a = before; acc[a] = '\0'; use[k] = 0; }
    }

    if (tail && tail[0] && a + 32 < sizeof acc)
        a += json_msg(acc + a, sizeof acc - a, 1, "user", NULL, tail);
    else if (send_extra && a + 32 < sizeof acc)
        a += json_msg(acc + a, sizeof acc - a, 1, "user", NULL, extra_system);

    for (k = 0; k < nvals; k++) jfree(held[k]);
    session_free(&recs);
    if (a >= sizeof acc) a = sizeof acc - 1;
    acc[a] = '\0';
    return (int)json_model(out, outlen, acc, a);
}

/* Build messages for a transcript with no tail and no extra. Selftest uses it. */
int agent_ctx_preview(const char *transcript, char *out, size_t outlen) {
    return agent_build_messages(transcript, NULL, NULL, out, outlen);
}

/* ── one model call: POST messages to endpoint, extract content ──────────── */

static int agent_last_http;
static char agent_last_err[160];

static int agent_call(const char *endpoint, const char *model,
                      const char *messages_json, char *content, size_t clen) {
    char body[NET_BODY_MAX];
    net_response r;
    jvalue *root, *c0, *msg, *ct;
    char err[128];
    char *m;
    size_t pre, rest, need;

    content[0] = '\0';
    snprintf(body, sizeof body, "%s", messages_json);
    m = strstr(body, "__MODEL__");
    if (!m) return -1;                       /* nothing to splice */
    pre = (size_t)(m - body);
    rest = strlen(m + 9);
    need = pre + strlen(model) + rest + 1;
    if (need > sizeof body) return -1;
    memmove(m + strlen(model), m + 9, rest + 1);
    memcpy(m, model, strlen(model));

    r = net_http("POST", endpoint, "application/json", body);
    agent_last_http = r.status;
    agent_last_err[0] = '\0';
    if (!r.ok) {
        if (r.err == -2004) return -5;
        if (r.err == -2005) return -6;
        return -2;
    }
    if (r.status < 200 || r.status >= 300) {
        int i, j = 0;
        const char *b = r.body ? r.body : "";
        for (i = 0; b[i] && j < (int)sizeof agent_last_err - 1; i++) {
            char c = b[i];
            if (c == '\n' || c == '\r' || c == '\t') c = ' ';
            agent_last_err[j++] = c;
        }
        agent_last_err[j] = '\0';
        {
            const char *home = getenv("HOME");
            char path[512];
            FILE *f;
            if (home && home[0]) {
                csih_home_bind(home);
                snprintf(path, sizeof path, "%s/.csih/last-http.txt", home);
                f = fopen(path, "w");
                if (f) {
                    fprintf(f, "status %d\n%s\n", r.status, r.body ? r.body : "");
                    fclose(f);
                }
            }
        }
        return -3;
    }

    root = json_parse(r.body, (size_t)(r.body_bytes > 0 ? r.body_bytes : strlen(r.body)),
                      err, sizeof err);
    if (!root || root->kind != J_OBJ) { if (root) jfree(root); return -4; }
    {
        jvalue *ch = jget(root, "choices");
        if (ch && ch->kind == J_ARR && ch->len > 0) {
            c0 = ch->items[0];
            if (c0 && c0->kind == J_OBJ) {
                msg = jget(c0, "message");
                if (msg && msg->kind == J_OBJ) {
                    ct = jget(msg, "content");
                    if (ct && ct->kind == J_STR) {
                        strncpy(content, jstr(ct), clen - 1);
                        content[clen - 1] = '\0';
                    }
                }
            }
        }
    }
    jfree(root);
    return content[0] ? 0 : -5;
}

/* ── the whole run ───────────────────────────────────────────────────────── */

typedef struct {
    int  ok;
    int  stopped;     /* 1 = ended via go:stop; 0 = hit MAX_ROUNDS */
    int  rounds;
    int  actions;
    int  err;
    char answer[AGENT_ANSWER_MAX];
    char reason[320];
    char last[320];   /* last action's tool + first output line, for the seal line */
} agent_result;

/* Both pages live in ~/.csih. They are not tied to the working directory. */
static int page_path(char *out, int outlen, const char *cwd, const char *which) {
    const char *page = plugin_page(which);
    const char *home;
    char dir[1024];
    (void)cwd;
    if (!page || !page[0] || !out || outlen < 2) return 0;
    home = getenv("HOME");
    if (!home || !home[0]) return 0;
    csih_home_bind(home);
    snprintf(dir, sizeof dir, "%s/.csih", home);
    mkdir(dir, 0750);
    snprintf(out, (size_t)outlen, "%s/%s", dir, page);
    return 1;
}

static void agent_seed_file(const char *path, const char *template) {
    if (!path || !path[0]) return;
    /* Existence, not a short read. file_read refuses a file bigger than its
     * buffer, and treating that as "missing" used to wipe the page. */
    if (access(path, 0) == 0) return;
    file_write(path, template, strlen(template));
}

/* One turn, one static continuation. step() is a single await point:
 * it returns while the HTTPS transfer is still running. */
enum { PH_IDLE = 0, PH_GO = 1, PH_WAIT = 2, PH_DONE = 3, HTTP_ACT = 0, HTTP_END = 1 };

static struct {
    int phase, http_kind, round, action, parse_fail, judge, no_tools;
    int replies;      /* model reply calls this turn, monotonic, never reset by round/action */
    agent_result res;
    agent_event_fn on_event;
    void *ud;
    char endpoint[256];
    char model[80];
    char transcript[512];
    char cwd[1024];
    char run_cwd[1024];
    char extra[1600];
    char messages[NET_BODY_MAX];
    char content[AGENT_CONTENT_MAX];
} AT;

static void at_copy(char *d, int n, const char *s) {
    if (!s) s = "";
    snprintf(d, (size_t)n, "%s", s);
}

static int at_splice_model(void) {
    char *m = strstr(AT.messages, "__MODEL__");
    size_t pre, rest, need;
    if (!m) return -1;
    pre = (size_t)(m - AT.messages);
    rest = strlen(m + 9);
    need = pre + strlen(AT.model) + rest + 1;
    if (need > sizeof AT.messages) return -1;
    memmove(m + strlen(AT.model), m + 9, rest + 1);
    memcpy(m, AT.model, strlen(AT.model));
    return 0;
}

static char judge_nudge[240];

static const char *at_judge_nudge(void) {
    int n = AT.judge + 1;
    if (n < 1) n = 1;
    if (n > MAX_JUDGE) n = MAX_JUDGE;
    if (AT.res.answer[0])
        snprintf(judge_nudge, sizeof judge_nudge,
                 "收尾判断 %d/%d。已经答完，只输出 {\"go\":\"stop\"} 结束；"
                 "只有还剩具体一步没做完才 continue。",
                 n, MAX_JUDGE);
    else
        snprintf(judge_nudge, sizeof judge_nudge,
                 "收尾判断 %d/%d。只输出 {\"go\":\"continue\"} 或 {\"go\":\"stop\"}。"
                 "没做完就 continue；还没 answer 时前两次 stop 不结束。"
                 "失败不能宣称通过；stop 可以结束失败回合并保留原因。",
                 n, MAX_JUDGE);
    return judge_nudge;
}

static int at_start_http(const char *tail) {
    net_response r;
    /* Budget exhausted: never issue another request, even from an early
     * return path that would otherwise re-enter the model. */
    if (AT.replies >= MAX_MODEL_REPLIES) {
        AT.res.ok = 0;
        snprintf(AT.res.reason, sizeof AT.res.reason,
                 "unfinished: reply budget already exhausted (%d), no acceptance green",
                 MAX_MODEL_REPLIES);
        AT.phase = PH_DONE;
        return 0;
    }
    /* Window list once per user turn, on the first action call.
     * Later steps and the judgment must not see it again. */
    agent_build_messages(AT.transcript, tail,
                         (!tail && AT.http_kind == HTTP_ACT
                          && AT.action == 0 && AT.round == 0 && AT.extra[0])
                             ? AT.extra : NULL,
                         AT.messages, sizeof AT.messages);
    if (at_splice_model() != 0) { AT.phase = PH_DONE; AT.res.err = -1; return 0; }
    if (net_async_begin("POST", AT.endpoint, "application/json", AT.messages) != 0) {
        r = net_async_end();
        AT.res.err = (r.err == -2004) ? -5 : -2;
        snprintf(AT.res.reason, sizeof AT.res.reason, "model call failed to start");
        AT.phase = PH_DONE;
        return 0;
    }
    AT.phase = PH_WAIT;
    return 1;
}

static void at_event(const char *line) {
    if (AT.on_event && line) AT.on_event(line, AT.ud);
}

/* One log slot is 240 bytes. A whole answer does not fit in one slot.
 * Split on newlines, then on a UTF-8 boundary, so the TUI can show the rest. */
int agent_event_pack(const char *prefix, const char *text, char rows[][200], int cap) {
    const char *p = text ? text : "";
    int n = 0, pref = 0;
    if (!rows || cap < 1) return 0;
    if (prefix && prefix[0]) {
        snprintf(rows[0], 200, "%s", prefix);
        pref = (int)strlen(rows[0]);
        if (pref > 160) pref = 160;
        rows[0][pref] = 0;
    }
    if (!p[0]) return pref ? 1 : 0;
    while (*p && n < cap) {
        int o = 0;
        if (n == 0 && pref) o = pref;
        while (*p == '\n' || *p == '\r') {
            if (*p == '\r' && p[1] == '\n') p++;
            p++;
            if (o > (n == 0 ? pref : 0)) break;
        }
        while (*p && *p != '\n' && *p != '\r') {
            unsigned char c = (unsigned char)*p;
            int need = 1, k;
            if ((c & 0xe0) == 0xc0) need = 2;
            else if ((c & 0xf0) == 0xe0) need = 3;
            else if ((c & 0xf8) == 0xf0) need = 4;
            else if (c >= 0x80) need = 1;
            if (o + need >= 199) break;
            for (k = 0; k < need && p[k]; k++) rows[n][o++] = p[k];
            if (k < need) { p += k; break; }
            p += need;
        }
        rows[n][o] = 0;
        if (o > 0) n++;
        if (*p == '\n' || *p == '\r') {
            if (*p == '\r' && p[1] == '\n') p++;
            p++;
        }
    }
    if (*p && n > 0) {
        int o = (int)strlen(rows[n - 1]);
        if (o + 3 < 199) memcpy(rows[n - 1] + o, "...", 4);
    }
    return n;
}

static int at_fail(int rc) {
    if (rc == -5) snprintf(AT.res.reason, sizeof AT.res.reason, "model call cancelled");
    else if (rc == -6) snprintf(AT.res.reason, sizeof AT.res.reason, "model call timed out (120s)");
    else if (rc == -3) snprintf(AT.res.reason, sizeof AT.res.reason, "model call failed (http %d) %s", agent_last_http, agent_last_err);
    else snprintf(AT.res.reason, sizeof AT.res.reason, "model call failed (rc=%d)", rc);
    AT.res.err = rc;
    AT.phase = PH_DONE;
    return 0;
}

/* Returns 1 if the turn should keep going. */
static int at_after_http(void) {
    net_response r = net_async_end();
    int rc;
    agent_last_http = r.status;
    agent_last_err[0] = '\0';
    AT.content[0] = '\0';
    if (!r.ok) {
        if (r.err == -2004) return at_fail(-5);
        if (r.err == -2005) return at_fail(-6);
        return at_fail(-2);
    }
    if (r.status < 200 || r.status >= 300) {
        int i, j = 0;
        for (i = 0; r.body[i] && j < (int)sizeof agent_last_err - 1; i++) {
            char c = r.body[i];
            if (c == '\n' || c == '\r' || c == '\t') c = ' ';
            agent_last_err[j++] = c;
        }
        agent_last_err[j] = 0;
        return at_fail(-3);
    }
    {
        char err[128];
        jvalue *root = json_parse(r.body, (size_t)(r.body_bytes > 0 ? r.body_bytes : strlen(r.body)), err, sizeof err);
        jvalue *ch, *c0, *msg, *ct;
        if (!root || root->kind != J_OBJ) { if (root) jfree(root); return at_fail(-4); }
        ch = jget(root, "choices");
        if (ch && ch->kind == J_ARR && ch->len > 0) {
            c0 = ch->items[0];
            if (c0 && c0->kind == J_OBJ) {
                msg = jget(c0, "message");
                if (msg && msg->kind == J_OBJ) {
                    ct = jget(msg, "content");
                    if (ct && ct->kind == J_STR)
                        snprintf(AT.content, sizeof AT.content, "%s", jstr(ct));
                }
            }
        }
        jfree(root);
    }
    if (!AT.content[0]) return at_fail(-4);
    /* One model reply received. Monotonic across the whole turn; never
     * reset by round/action. Every later branch (ACT, END, red flag,
     * judge, nudge) returns through here, so this single gate bounds the
     * total number of model calls. */
    AT.replies++;
    if (AT.replies > MAX_MODEL_REPLIES) {
        AT.res.ok = 0;
        AT.res.stopped = 0;
        snprintf(AT.res.reason, sizeof AT.res.reason,
                 "unfinished: exceeded MAX_MODEL_REPLIES (%d), no acceptance green",
                 MAX_MODEL_REPLIES);
        at_event("  → stop (reply budget exhausted, unfinished)");
        AT.phase = PH_DONE;
        return 0;
    }
    rc = 0;
    (void)rc;
    if (AT.http_kind == HTTP_END) {
        agent_step d = agent_parse(AT.content);
        char dec_rec[128];
        AT.judge++;
        if (d.kind == ACT_GO_CONTINUE) {
            json_rec(dec_rec, sizeof dec_rec, "role", "decision", "go", "continue", NULL, NULL);
            session_append(AT.transcript, dec_rec);
            at_event("  → continue");
            AT.round++;
            AT.action = 0;
            AT.http_kind = HTTP_ACT;
            AT.phase = PH_GO;
            return 1;
        }
        if (!agent_may_stop_ans(AT.judge, MAX_JUDGE, AT.res.answer[0] != 0)) {
            at_event("  → 再判断");
            AT.http_kind = HTTP_END;
            AT.phase = PH_GO;
            return 1;
        }
        json_rec(dec_rec, sizeof dec_rec, "role", "decision", "go", "stop", NULL, NULL);
        session_append(AT.transcript, dec_rec);
        at_event("  → stop (判定可停)");
        snprintf(AT.res.reason, sizeof AT.res.reason, "%s", "judge ok");
        AT.res.stopped = 1;
        AT.res.ok = 1;
        AT.phase = PH_DONE;
        return 0;
    }
    {
        agent_step s = agent_parse(AT.content);
        agent_take_prose(&s, AT.content, AT.no_tools);
        if (s.kind == ACT_GO_STOP || s.kind == ACT_GO_CONTINUE) AT.judge++;
        char asst_rec[AGENT_CONTENT_MAX + 64];
        char result[AGENT_RESULT_MAX];
        char tool_rec[AGENT_RESULT_MAX + 64];
        const char *nm = plugin_name(s.kind);
        if (!nm) nm = "?";
        json_rec(asst_rec, sizeof asst_rec, "role", "assistant", "text", AT.content, NULL, NULL);
        session_append(AT.transcript, asst_rec);
        {
            char ev[256];
            ev[0] = 0;
            if (s.kind == ACT_EXEC) {
                char whyb[72], cmdb[120];
                int wi, wo, ci, co;
                const char *srcw = s.why[0] ? s.why : "(无解释)";
                for (wi = 0, wo = 0; srcw[wi] && wo + 1 < (int)sizeof whyb; wi++) {
                    char c = srcw[wi];
                    if (c == '\n' || c == '\r' || c == '\t') c = ' ';
                    whyb[wo++] = c;
                }
                whyb[wo] = 0;
                for (ci = 0, co = 0; s.cmd[ci] && co + 1 < (int)sizeof cmdb; ci++) {
                    char c = s.cmd[ci];
                    if (c == '\n' || c == '\r' || c == '\t') c = ' ';
                    cmdb[co++] = c;
                }
                cmdb[co] = 0;
                snprintf(ev, sizeof ev, "exec\t%s\t%s", whyb, cmdb);
            }
            else if (s.kind == ACT_READ)
                snprintf(ev, sizeof ev, "fold\tfile\tread\t%s", s.path);
            else if (s.kind == ACT_WRITE)
                snprintf(ev, sizeof ev, "fold\tfile\twrite\t%s", s.path);
            else if (s.kind == ACT_EDIT)
                snprintf(ev, sizeof ev, "fold\tfile\tedit\t%s", s.path);
            else if (s.kind == ACT_MIND)
                snprintf(ev, sizeof ev, "fold\tmind\t%s\t%s",
                         s.op[0] ? s.op : "mind", s.path);
            else if (s.kind == ACT_ANSWER) {
                char arows[12][200];
                char apref[48];
                int ai, an;
                snprintf(apref, sizeof apref, "✓ %s: ", nm);
                an = agent_event_pack(apref, s.text, arows, 12);
                for (ai = 0; ai < an; ai++) at_event(arows[ai]);
                ev[0] = 0;
            }
            else if (s.kind == ACT_GO_STOP)
                snprintf(ev, sizeof ev, "%s",
                         agent_may_stop_ans(AT.judge, MAX_JUDGE, AT.res.answer[0] != 0) ? "  → stop (go=stop)" : "  → 再判断");
            else if (s.kind == ACT_GO_CONTINUE) snprintf(ev, sizeof ev, "  → continue");
            else
                snprintf(ev, sizeof ev, "无法解析");
            if (ev[0]) at_event(ev);
        }
        if (s.kind == ACT_ANSWER || s.kind == ACT_GO_STOP) {
            char why[200];
            if (agent_failing(why, (int)sizeof why)) {
                if (s.kind == ACT_GO_STOP) {
                    /* A red flag means this slice failed. stop may end the
                     * failed round and keep the reason, but it cannot claim
                     * success. No further model call. */
                    json_rec(tool_rec, sizeof tool_rec, "role", "decision", "go", "stop", NULL, NULL);
                    session_append(AT.transcript, tool_rec);
                    at_event("  → stop (red, failed)");
                    snprintf(AT.res.reason, sizeof AT.res.reason, "unfinished: %s", why);
                    AT.res.stopped = 1;
                    AT.res.ok = 0;
                    AT.phase = PH_DONE;
                    return 0;
                }
                json_rec(tool_rec, sizeof tool_rec, "role", "tool", "name", "error", "text", why);
                session_append(AT.transcript, tool_rec);
                at_event(why);
                AT.phase = PH_GO;
                return 1;
            }
        }
        if (s.kind == ACT_GO_STOP) {
            if (!AT.no_tools && agent_watch_needs_mail()) {
                at_event("  → stop (peer mail not delivered)");
                snprintf(AT.res.reason, sizeof AT.res.reason,
                         "%s", "unfinished: peer mail not delivered");
                AT.res.stopped = 1;
                AT.res.ok = 0;
                AT.phase = PH_DONE;
                return 0;
            }
            if (!agent_may_stop_ans(AT.judge, MAX_JUDGE, AT.res.answer[0] != 0)) {
                AT.http_kind = HTTP_END;
                AT.phase = PH_GO;
                return 1;
            }
            json_rec(tool_rec, sizeof tool_rec, "role", "decision", "go", "stop", NULL, NULL);
            session_append(AT.transcript, tool_rec);
            at_event("  → stop (go=stop)");
            snprintf(AT.res.reason, sizeof AT.res.reason, "%s", "go=stop");
            AT.res.stopped = 1;
            AT.res.ok = 1;
            AT.phase = PH_DONE;
            return 0;
        }
        if (s.kind == ACT_GO_CONTINUE) {
            json_rec(tool_rec, sizeof tool_rec, "role", "decision", "go", "continue", NULL, NULL);
            session_append(AT.transcript, tool_rec);
            AT.round++;
            AT.action = 0;
            AT.http_kind = HTTP_ACT;
            AT.phase = PH_GO;
            return 1;
        }
        if (s.kind == ACT_ANSWER && !AT.no_tools && agent_watch_needs_mail()) {
            g_watch_deny++;
            if (g_watch_deny >= 2) {
                at_event("  → stop (peer mail not delivered)");
                snprintf(AT.res.reason, sizeof AT.res.reason,
                         "%s", "unfinished: peer mail not delivered");
                AT.res.stopped = 1;
                AT.res.ok = 0;
                AT.phase = PH_DONE;
                return 0;
            }
            json_rec(tool_rec, sizeof tool_rec, "role", "tool", "name", "error", "text",
                     "先给同伴发 envelope，再 answer；例如 exec bin/envelope 0:<peer> \"<标题>\" \"<正文>\"。");
            session_append(AT.transcript, tool_rec);
            at_event("先给同伴发 envelope");
            AT.phase = PH_GO;
            return 1;
        }
        if (s.kind == ACT_ANSWER) {
            snprintf(AT.res.answer, sizeof AT.res.answer, "%s", s.text);
            /* One supplementary decision after the work of this round.
             * The next continue would be round+1. At the cap, do not ask:
             * continue would be rejected on the next step anyway. */
            if (AT.round + 1 >= MAX_ROUNDS) {
                json_rec(tool_rec, sizeof tool_rec, "role", "decision", "go", "stop", NULL, NULL);
                session_append(AT.transcript, tool_rec);
                AT.res.ok = 1;
                at_event("  → stop (MAX_ROUNDS)");
                snprintf(AT.res.reason, sizeof AT.res.reason, "reached MAX_ROUNDS");
                AT.phase = PH_DONE;
                return 0;
            }
            AT.http_kind = HTTP_END;
            at_event("↻ round-end decision");
            AT.phase = PH_GO;
            return 1;
        }
        if (s.kind == ACT_ERR) {
            json_rec(tool_rec, sizeof tool_rec, "role", "tool", "name", "error", "text",
                     "could not parse your output as a JSON action; emit exactly one {\"act\":...} object");
            session_append(AT.transcript, tool_rec);
            if (++AT.parse_fail >= 3) {
                AT.res.ok = 1;
                snprintf(AT.res.reason, sizeof AT.res.reason, "too many unparseable steps");
                AT.phase = PH_DONE;
                return 0;
            }
            AT.action++;
            AT.phase = PH_GO;
            return 1;
        }
        {
            const char *name = nm;
            if (agent_peer_blocked(s.kind, s.op, s.cmd, result, (int)sizeof result)) {
            } else if (s.kind == ACT_EXEC && agent_note_cd(AT.run_cwd, sizeof AT.run_cwd, s.cmd, result, sizeof result)) {
            } else {
                if (s.kind == ACT_EXEC) agent_watch_note(s.cmd);
                agent_exec(&s, AT.run_cwd[0] ? AT.run_cwd : AT.cwd, result, sizeof result);
            }
            if (agent_object_count(AT.content) > 1 && strlen(result) + 80 < sizeof result)
                strcat(result, "\n[only the first action ran; send one JSON object]");
            agent_tool_record(tool_rec, sizeof tool_rec, name, result);
            session_append(AT.transcript, tool_rec);
            {
                /* Tool name only. The result body stays in the tool row. */
                snprintf(AT.res.last, sizeof AT.res.last, "%s",
                         name && name[0] ? name : "act");
            }
            AT.res.actions++;
            {
                const char *p = result;
                int rows = 0;
                if (!result[0]) at_event("  │ (no output)");
                while (p && *p && rows < 12) {
                    char line[180], ev[200];
                    int i = 0;
                    while (*p && *p != '\n' && i + 1 < (int)sizeof line) line[i++] = *p++;
                    line[i] = 0;
                    if (*p == '\n') p++;
                    if (!line[0]) continue;
                    snprintf(ev, sizeof ev, "  │ %s", line);
                    at_event(ev);
                    rows++;
                }
            }
        }
        AT.action++;
        if (AT.action >= MAX_ACTIONS) {
            /* Action budget spent and no answer yet. Do not spend a model
             * turn deciding: stop once, with the reason written here. */
            snprintf(AT.res.reason, sizeof AT.res.reason,
                     "reached MAX_ACTIONS (%d) with no answer; last %s",
                     MAX_ACTIONS,
                     AT.res.last[0] ? AT.res.last : "(no result)");
            session_append(AT.transcript,
                           "{\"role\":\"decision\",\"go\":\"stop\"}");
            at_event("  → stop (MAX_ACTIONS)");
            AT.res.stopped = 1;
            AT.res.ok = 1;
            AT.phase = PH_DONE;
            return 0;
        }
        AT.phase = PH_GO;
        return 1;
    }
}

int agent_turn_begin(const char *prompt, const char *transcript,
                     const char *endpoint, const char *model, const char *cwd,
                     const char *extra_system,
                     agent_event_fn on_event, void *ud) {
    char user_rec[AGENT_CONTENT_MAX + 64];

    char tree[2048], palace[2048];
    memset(&AT, 0, sizeof AT);
    agent_watch_reset();
    AT.no_tools = agent_user_forbids_tools(prompt);
    net_reset();
    net_turn_clock();
    at_copy(AT.endpoint, (int)sizeof AT.endpoint, endpoint);
    at_copy(AT.model, (int)sizeof AT.model, model);
    at_copy(AT.transcript, (int)sizeof AT.transcript, transcript);
    at_copy(AT.cwd, (int)sizeof AT.cwd, cwd);
    at_copy(AT.run_cwd, (int)sizeof AT.run_cwd, cwd);
    at_copy(AT.extra, (int)sizeof AT.extra, extra_system);
    AT.on_event = on_event;
    AT.ud = ud;
    page_path(tree, (int)sizeof tree, AT.cwd, "tree");
    page_path(palace, (int)sizeof palace, AT.cwd, "palace");
    agent_seed_file(tree,
        "csih\n"
        "├── file\n"
        "├── exec 带 why\n"
        "├── mind 思维树 markdown-tree-dag\n"
        "│   └── 记忆宫殿 mermaid-flowchart-memory-palace\n"
        "══> 新功能先 bin/envelope 0:grkwjcgmcsih\n");
    agent_seed_file(palace,
        "```mermaid\n"
        "flowchart LR\n"
        "  csih --> file & exec & mind\n"
        "  mind --> tree[\"markdown-tree-dag\"]\n"
        "  mind --> palace[\"mermaid-flowchart-memory-palace\"]\n"
        "```\n");
    agent_journal_trim(AT.transcript, AGENT_JOURNAL_MAX, AGENT_JOURNAL_KEEP);
    json_rec(user_rec, sizeof user_rec, "role", "user", "text", prompt ? prompt : "", NULL, NULL);
    if (session_append(AT.transcript, user_rec) != 0) {
        AT.res.err = 1;
        snprintf(AT.res.reason, sizeof AT.res.reason, "cannot write transcript");
        AT.phase = PH_DONE;
        return -1;
    }
    AT.phase = PH_GO;
    AT.http_kind = HTTP_ACT;
    return 0;
}

int agent_turn_step(int wait_ms) {
    if (AT.phase == PH_DONE || AT.phase == PH_IDLE) return 0;
    if (AT.phase == PH_WAIT) {
        if (net_async_pump(wait_ms)) return 1;
        return at_after_http();
    }
    if (AT.round >= MAX_ROUNDS) {
        AT.res.ok = 1;
        snprintf(AT.res.reason, sizeof AT.res.reason, "reached MAX_ROUNDS");
        AT.phase = PH_DONE;
        return 0;
    }
    if (AT.action == 0 && AT.http_kind == HTTP_ACT) {
        AT.res.rounds++;
        AT.parse_fail = 0;
    }
    if (AT.http_kind == HTTP_END) return at_start_http(at_judge_nudge());
    return at_start_http(NULL);
}

agent_result agent_turn_take(void) { agent_result r = AT.res; return r; }

/* Install a finished local result. The TUI end path reads it through
 * agent_turn_take. No socket is opened. */
void agent_turn_seal(int ok, int stopped, int rounds, int actions, int err,
                     const char *answer) {
    memset(&AT.res, 0, sizeof AT.res);
    AT.res.ok = ok;
    AT.res.stopped = stopped;
    AT.res.rounds = rounds;
    AT.res.actions = actions;
    AT.res.err = err;
    if (answer) snprintf(AT.res.answer, sizeof AT.res.answer, "%s", answer);
    AT.phase = PH_DONE;
}

agent_result agent_run_cb_core(const char *prompt, const char *transcript,
                       const char *endpoint, const char *model, const char *cwd,
                       const char *extra_system,
                       agent_event_fn on_event, void *ud) {
    agent_result r;
    if (agent_turn_begin(prompt, transcript, endpoint, model, cwd, extra_system, on_event, ud) != 0) {
        r = agent_turn_take();
        return r;
    }
    while (agent_turn_step(200)) ;
    r = agent_turn_take();
    return r;
}


/*
 * Public, UI-aware entry point: same loop as the core, but lets a caller
 * receive live progress lines (e.g. the TUI) and inject environment it knows
 * about (e.g. available tmux windows) without the library depending on either.
 */
agent_result agent_run_cb(const char *prompt, const char *transcript,
                          const char *endpoint, const char *model, const char *cwd,
                          const char *extra_system,
                          agent_event_fn on_event, void *ud) {
    agent_result r = agent_run_cb_core(prompt, transcript, endpoint, model, cwd,
                                       extra_system, on_event, ud);
    return r;
}

/*
 * No events and no extra system text. The CLI uses agent_run_cb so each step
 * is printed before the next model call.
 */
agent_result agent_run(const char *prompt, const char *transcript,
                       const char *endpoint, const char *model, const char *cwd) {
    agent_result r = agent_run_cb_core(prompt, transcript, endpoint, model, cwd,
                                       NULL, NULL, NULL);
    return r;
}
