/*
 * tui.c — the main loop. This is the file that finally puts term.c and
 * render.c together, and it is deliberately the SMALLEST of the three.
 *
 * WHAT IT DOES, in full: wait for a byte or for the frame interval to elapse,
 * fold that into the screen state, render the state to a frame, diff the frame
 * against the previous one, write the difference. That is the entire TUI. No
 * virtual tree, no reconciler, no widget layer — see TUI-INTERFACE.md §2 for
 * why copying Ink's reconciler would import React's complexity for a UI that
 * has a handful of screens.
 *
 * WHAT IS DELIBERATELY NOT HERE: any string formatting (render.c), any ANSI
 * sequence (render.c's frame_diff), any syscall (term.c). Every one of those
 * splits exists so the decision-making is testable without a terminal, and
 * this file — the untestable part — holds nothing but the wiring.
 *
 * TWO DESIGN DECISIONS CAME FROM AN INDEPENDENT REVIEW (dsh-new), NOT FROM ME,
 * and both corrected a plan of mine that was wrong:
 *
 *   1. "Forget strcmp's cost; the cost is writing one character at a time."
 *      I had been about to add long-line protection around the line compare.
 *      Wrong target. frame_diff() already builds a whole line into the buffer
 *      before writing, and tui_write_line() below asserts it goes out in ONE
 *      write() — that is what the review said to protect.
 *
 *   2. "A line containing a raw NUL makes strcmp stop early and MISS a change."
 *      Screen lines should never contain NUL, so this is asserted rather than
 *      handled: silently diffing two lines as equal is a redraw that never
 *      happens, which presents to a user as a frozen screen. That failure mode
 *      is worth a hard stop.
 *
 * CLI takes SUBCOMMANDS, not dash-options (unisacc reserves the dashes), and
 * `tui selftest` runs with NO terminal attached — the loop is driven by an
 * injected key source, so the part that would normally need a human at a
 * keyboard is exercised in CI.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* Include the HEADERS, never the .c files. The first version did
 * `#include "render.c"` / `#include "term.c"`, which dragged in two more
 * `main`s and two more `failures` counters and would not link — the same
 * "two mains" rule this project has enforced all along, broken by a new route. */
#include "render_api.h"
#include "term_api.h"
#include "csih_home.h"
#include "csih_cols.h"

#define tui_next_cp csih_next_cp
#define tui_cp_cols csih_cp_cols

/*
 * chat.c's turn function, restated rather than #include'd. A restatement is what
 * lets this file compile on its own: unisacc can see a type an EARLIER file on
 * the command line defined, but that is order-dependent, so the boundary is
 * declared here. (render_api.h / term_api.h are shared headers, not restatements
 * — a header is fine; `#include`-ing a .c file is the thing that is not.)
 */
typedef struct {
    int    ok;
    int    cont;
    double p;
    char   verdict[192];
} chat_turn;

chat_turn chat_turn_run(const char *path, const char *user_text, double threshold);

/* tools.c's dispatcher, restated for the same reason as everything else here. */
typedef struct { int ok; char out[65536]; } tool_result;
tool_result tool_run(const char *line);

/* agent.c's loop, restated so this file compiles on its own. The on_event
 * callback lets the TUI show each step live; extra_system lets it hand the model
 * environment the library cannot see (e.g. live tmux windows). */
typedef struct {
    int  ok;
    int  stopped;
    int  rounds;
    int  actions;
    int  err;
    char answer[4096];
    char reason[320];
} agent_result;
int agent_turn_begin(const char *prompt, const char *transcript,
                     const char *endpoint, const char *model, const char *cwd,
                     const char *extra_system,
                     void (*on_event)(const char *line, void *ud), void *ud);
int agent_turn_step(int wait_ms);
agent_result agent_turn_take(void);
const char *agent_model_rules(void);
void agent_set_spill(int on);
int agent_mentions_window(const char *s);
int agent_transcript_path(char *out, size_t outlen);
void agent_turn_seal(int ok, int stopped, int rounds, int actions, int err,
                     const char *answer);

/* shell.c's dispatcher, restated so the TUI can enumerate tmux windows. */
typedef struct { int ok; int err; int exited; int status; int signal; long bytes; char out[65536]; } shell_result;
shell_result shell_run_in(const char *command, const char *cwd);
void net_set_tick(void (*fn)(void));
void net_cancel(void);
void net_progress(int *sec, int *tokens, long *sent);
void net_recv(long *n);
void net_cache(int *hit, int *miss, int *seen);
int  net_deepseek_forget(void);

/* How long a loop waits before redrawing anyway. Frames only change on input in
 * this design, so this is a safety net rather than the animation clock — but
 * keeping it means a state change from elsewhere cannot leave the screen stale
 * forever. */
#define TUI_FRAME_MS 250

/* ── screen state ───────────────────────────────────────────────────────── */

#define TUI_INPUT_MAX 4096 /* a paste is one buffer, not one 240-byte line */
#define TUI_LOOP_MAX  8   /* goal resubmits, same order of magnitude as a turn */
#define TUI_LOG_VIEW  20  /* agent log rows; a shorter terminal uses fewer */

typedef struct {
    char input[TUI_INPUT_MAX];   /* what the user has typed */
    int  ninput;
    char pending[8][TUI_INPUT_MAX]; /* lines typed while a call is in flight */
    int  npending;
    int  quit;                   /* leave the program */
    int  ask_exit;               /* empty Ctrl-C: say Ctrl-D quits, do not quit */
    int  pasting;                /* inside ESC [ 200 ~ ... ESC [ 201 ~ */
    int  more_keys;              /* 1 when this key is not the last of the burst */
    int  cancel;                 /* stop this turn and return to the input */
    int  busy;                   /* 1 while a model call is in flight */
    int  busy_tick;              /* ticks when busy became 1 */
    int  ticks;                  /* loop iterations; not printed in agent mode */
    const char *notice;          /* one-line feedback, e.g. an unknown key */
    const char *transcript;      /* where turns are recorded; NULL = no chat */
    char  last[192];             /* most recent verdict, so it can be shown */
    char  output[1024];          /* last tool output, shown in the frame */

    /*
     * A QUEUE of keys, not a single key — because one read() can return many.
     *
     * This is not a theoretical concern: a pty delivers a whole typed line in
     * one chunk, and the first version of this loop called term_read_key()
     * once per poll tick, so `hello from tui\r` produced ONE key ('h') and
     * silently dropped the other fourteen bytes. The symptom was a transcript
     * line reading {"role":"user","text":""} after visibly typing a sentence —
     * a data-loss bug that looks like a rendering bug.
     *
     * So: drain what arrived into this queue, then consume one key per
     * iteration. Keys are never discarded; only the polling is one-per-tick.
     */
    term_key_t queue[64];
    int  qhead, qtail;

    int  mode;            /* 0 = chat, 1 = agent */
    int  cols;            /* usable width, from term_size */
    int  rows;            /* usable height, from term_size */
    char log[40][240];    /* agent event scrollback (ring buffer) */
    int  nlog;
    int  ex_on[40];             /* 1 when this log slot is a tool row */
    int  ex_open[40];           /* 1 shows the tool body; 0 is the summary */
    char ex_tag[40][12];
    char ex_why[40][96];
    char ex_cmd[40][120];
    char ex_body[40][1024];     /* tool output, drawn only while open */
    char last_answer[1024];
    char errline[240];        /* last error, shown on the status row, not in the log */
    char goal[TUI_INPUT_MAX]; /* /goal text; empty until set */
    int  loop_on;             /* /loop arms automatic resubmit */
    int  loop_left;           /* remaining auto-submits this arming */
    int  sys_open;            /* 0: system rule only. 1: ten body rows */
    int  sys_top;             /* first wrapped row shown while open */
    int  mind_top;            /* first folded row of the mind block */
    int  log_skip;            /* newest log entries hidden; 0 follows the tail */
} tui_state;

static void tui_state_init(tui_state *st, const char *transcript) {
    memset(st, 0, sizeof *st);
    st->transcript = transcript;
    st->mode = 0;
    st->cols = 40;
    st->rows = 24;
    st->notice = 0;
    (void)transcript;
}

/* Forward declarations: the key handler (defined below) dispatches to
 * tui_run_agent, which drives the agent and redraws live. */
static void tui_redraw(tui_state *st);
static int tui_run_agent(tui_state *st);
static void tui_apply_key(tui_state *st, int kind, char ch);
static void tui_submit(tui_state *st);
static void tui_log_hold(tui_state *st);
static void tui_log_scroll(tui_state *st, int delta);
static int tui_log_room(const tui_state *st);
static r_frame tui_render_state(tui_state *st, int cols);

/* ── the key queue ──────────────────────────────────────────────────────── */

static int tui_q_empty(const tui_state *st) { return st->qhead == st->qtail; }

/* One slot stays empty so head==tail means empty. Reading more than this
 * from the tty drops the overflow: those bytes are already gone. */
static int tui_q_room(const tui_state *st) {
    int cap = (int)(sizeof st->queue / sizeof st->queue[0]);
    int used = (st->qtail - st->qhead + cap) % cap;
    return cap - 1 - used;
}

static void tui_q_push(tui_state *st, term_key_t k) {
    int next = (st->qtail + 1) % (int)(sizeof st->queue / sizeof st->queue[0]);
    if (next == st->qhead) return;   /* full: drop rather than corrupt the ring */
    st->queue[st->qtail] = k;
    st->qtail = next;
}

static term_key_t tui_q_pop(tui_state *st) {
    term_key_t k;
    memset(&k, 0, sizeof k);
    k.kind = TERM_KEY_NONE;
    if (tui_q_empty(st)) return k;
    k = st->queue[st->qhead];
    st->qhead = (st->qhead + 1) % (int)(sizeof st->queue / sizeof st->queue[0]);
    return k;
}

/* ── output ─────────────────────────────────────────────────────────────── */

/*
 * Write a whole string with ONE write() call.
 *
 * A loop of putchar() per character is the classic TUI performance bug the
 * review named. One write per changed line also matters for a second reason: a
 * line written in pieces can be interleaved with output from another process
 * sharing the terminal, producing torn lines that look like a rendering bug.
 *
 * Returns 0 on success, -1 if the terminal is gone (nothing to do about it
 * except stop looping, which the caller does).
 */
static int tui_write_all(const char *s, size_t n) {
    size_t off = 0;
    while (off < n) {
        ssize_t w = write(1, s + off, n - off);
        if (w <= 0) return -1;
        off += (size_t)w;
    }
    return 0;
}

/*
 * Every line must cover `width` terminal columns. Byte length is not enough:
 * one CJK character is 3 bytes and 2 columns, so a byte-padded line stops
 * early and the old glyphs stay on the right. An embedded NUL still fails,
 * because the visible width then falls short.
 *
 * The first version of this function compared strlen(x) != strlen(x), which is
 * always false: a check that can never fire, written to look like a safeguard.
 * That is worse than no check, because it is quoted as one. The review's point
 * stands — frame_diff() uses strcmp, which stops at a NUL, so a line with an
 * embedded NUL compares equal to a longer line and the redraw never happens,
 * which a user sees as a frozen screen.
 */
static int tui_disp_width(const char *s);

static int tui_frame_bad_width(const r_frame *f, int width) {
    int i;
    for (i = 0; i < f->n; i++) {
        if (tui_disp_width(f->lines[i]) != width) return 1;
    }
    return 0;
}

/* ── the loop, with its inputs injected ─────────────────────────────────── */

/*
 * One iteration: fold a key (or a timeout) into the state.
 *
 * Split from the loop so it can be driven by a scripted key sequence in the
 * self-test. `key_kind` is a term_key_kind; -1 means "the wait timed out".
 */
static void tui_enqueue(tui_state *st) {
    if (!st->input[0]) return;
    if (st->npending >= 8) { st->notice = "待发送已满"; return; }
    snprintf(st->pending[st->npending], TUI_INPUT_MAX, "%s", st->input);
    st->npending++;
    st->input[0] = '\0';
    st->ninput = 0;
}

static void tui_dequeue(tui_state *st) {
    if (st->npending <= 0) return;
    snprintf(st->input, TUI_INPUT_MAX, "%s", st->pending[0]);
    st->ninput = (int)strlen(st->input);
    memmove(st->pending[0], st->pending[1], (size_t)(st->npending - 1) * TUI_INPUT_MAX);
    st->npending--;
}

static void tui_log_make_room(tui_state *st) {
    if (st->nlog < 40) return;
    memmove(st->log[0], st->log[1], sizeof st->log[0] * 39);
    memmove(st->ex_on, st->ex_on + 1, sizeof st->ex_on[0] * 39);
    memmove(st->ex_open, st->ex_open + 1, sizeof st->ex_open[0] * 39);
    memmove(st->ex_tag[0], st->ex_tag[1], sizeof st->ex_tag[0] * 39);
    memmove(st->ex_why[0], st->ex_why[1], sizeof st->ex_why[0] * 39);
    memmove(st->ex_cmd[0], st->ex_cmd[1], sizeof st->ex_cmd[0] * 39);
    memmove(st->ex_body[0], st->ex_body[1], sizeof st->ex_body[0] * 39);
    st->nlog = 39;
    st->ex_on[39] = 0;
    st->ex_open[39] = 0;
    st->ex_tag[39][0] = 0;
    st->ex_why[39][0] = 0;
    st->ex_cmd[39][0] = 0;
    st->ex_body[39][0] = 0;
}

static void tui_note_err(tui_state *st, const char *s);

static void tui_log_plain(tui_state *st, const char *s) {
    tui_log_make_room(st);
    st->ex_on[st->nlog] = 0;
    st->ex_open[st->nlog] = 0;
    st->ex_tag[st->nlog][0] = 0;
    st->ex_why[st->nlog][0] = 0;
    st->ex_cmd[st->nlog][0] = 0;
    st->ex_body[st->nlog][0] = 0;
    snprintf(st->log[st->nlog], sizeof st->log[0], "%s", s ? s : "");
    st->nlog++;
    tui_log_hold(st);
    tui_note_err(st, s);
}

static void tui_log_line(tui_state *st, const char *s) {
    tui_log_plain(st, s);
}

/* First N bytes of a tool call, cut on a UTF-8 boundary. A longer call
 * keeps an ellipsis so the closed row stays a summary. */
static void tui_condense(char *dst, int dstmax, const char *src, int maxb) {
    int i = 0;
    if (!dst || dstmax < 1) return;
    if (!src) src = "";
    while (src[i] && i + 1 < dstmax && i < maxb) {
        dst[i] = src[i];
        i++;
    }
    while (i > 0 && ((unsigned char)dst[i - 1] & 0xC0) == 0x80) i--;
    if (src[i] && i + 4 < dstmax) {
        memcpy(dst + i, "…", 3);
        i += 3;
    }
    dst[i] = 0;
}

/* `exec<TAB>why<TAB>cmd` or `fold<TAB>tag<TAB>why<TAB>call`.
 * Closed row shows why and a short call. The body stays hidden. */
static int tui_exec_fields(const char *line, char *why, int wn, char *cmd, int cn) {
    const char *p, *tab;
    int n;
    if (!line || strncmp(line, "exec\t", 5) != 0) return 0;
    p = line + 5;
    tab = strchr(p, '\t');
    if (!tab) return 0;
    n = (int)(tab - p);
    if (n >= wn) n = wn - 1;
    if (n < 0) n = 0;
    memcpy(why, p, (size_t)n);
    why[n] = 0;
    snprintf(cmd, (size_t)cn, "%s", tab + 1);
    if (!why[0]) snprintf(why, (size_t)wn, "%s", "(无解释)");
    return 1;
}

/* fold<TAB>tag<TAB>why<TAB>call. tag is the word in front of [展开]. */
static int tui_fold_fields(const char *line, char *tag, int tn,
                           char *why, int wn, char *cmd, int cn) {
    const char *p, *t1, *t2;
    int n;
    if (!line || strncmp(line, "fold\t", 5) != 0) return 0;
    p = line + 5;
    t1 = strchr(p, '\t');
    if (!t1) return 0;
    n = (int)(t1 - p);
    if (n >= tn) n = tn - 1;
    if (n < 0) n = 0;
    memcpy(tag, p, (size_t)n);
    tag[n] = 0;
    p = t1 + 1;
    t2 = strchr(p, '\t');
    if (!t2) return 0;
    n = (int)(t2 - p);
    if (n >= wn) n = wn - 1;
    if (n < 0) n = 0;
    memcpy(why, p, (size_t)n);
    why[n] = 0;
    snprintf(cmd, (size_t)cn, "%s", t2 + 1);
    if (!tag[0]) snprintf(tag, (size_t)tn, "tool");
    if (!why[0]) snprintf(why, (size_t)wn, "%s", "(无解释)");
    return 1;
}

/* Tool output is `  │ ...`. It belongs to the tool row above it. */
static const char *tui_bar_text(const char *line) {
    if (!line || strncmp(line, "  │ ", 6) != 0) return 0;
    return line + 6;
}

static void tui_body_add(tui_state *st, int i, const char *s) {
    char *b = st->ex_body[i];
    size_t n = strlen(b);
    size_t cap = sizeof st->ex_body[0];
    if (!s) s = "";
    if (n && n + 1 < cap) b[n++] = '\n';
    while (*s && n + 1 < cap) b[n++] = *s++;
    b[n] = 0;
}

static void tui_log_event(tui_state *st, const char *line) {
    char tag[12], why[96], cmd[120];
    const char *bar = tui_bar_text(line);
    if (bar && st->nlog > 0 && st->ex_on[st->nlog - 1]) {
        tui_body_add(st, st->nlog - 1, bar);
        tui_note_err(st, line);
        return;
    }
    tui_log_make_room(st);
    tag[0] = 0;
    if (tui_exec_fields(line, why, (int)sizeof why, cmd, (int)sizeof cmd))
        snprintf(tag, sizeof tag, "exec");
    else if (!tui_fold_fields(line, tag, (int)sizeof tag, why, (int)sizeof why,
                              cmd, (int)sizeof cmd))
        tag[0] = 0;
    if (tag[0]) {
        st->ex_on[st->nlog] = 1;
        st->ex_open[st->nlog] = 0;
        snprintf(st->ex_tag[st->nlog], sizeof st->ex_tag[0], "%s", tag);
        snprintf(st->ex_why[st->nlog], sizeof st->ex_why[0], "%s", why);
        snprintf(st->ex_cmd[st->nlog], sizeof st->ex_cmd[0], "%s", cmd);
        st->ex_body[st->nlog][0] = 0;
        snprintf(st->log[st->nlog], sizeof st->log[0], "%s[展开]> %s", tag, why);
    } else {
        st->ex_on[st->nlog] = 0;
        st->ex_open[st->nlog] = 0;
        st->ex_tag[st->nlog][0] = 0;
        st->ex_why[st->nlog][0] = 0;
        st->ex_cmd[st->nlog][0] = 0;
        st->ex_body[st->nlog][0] = 0;
        snprintf(st->log[st->nlog], sizeof st->log[0], "%s", line ? line : "");
    }
    st->nlog++;
    tui_log_hold(st);
    tui_note_err(st, line);
}

/* One status row under the hint. Kernel lines such as nfs still hit the tty,
 * but anything the harness itself saw is copied here so it does not have to
 * sit in the middle of the frame. */
static int tui_looks_err(const char *s) {
    if (!s) return 0;
    if (strstr(s, "error:")) return 1;
    if (strstr(s, "not covered")) return 1;
    if (strstr(s, "not responding")) return 1;
    if (strstr(s, "is alive again")) return 1;
    if (strstr(s, "✗")) return 1;
    if (strstr(s, "exit=") && !strstr(s, "exit=0")) return 1;
    return 0;
}

static void tui_note_err(tui_state *st, const char *s) {
    int i, o = 0;
    if (!s) return;
    /* A step trace indented "  \u2502 .../x.c:..." is the tool's own body text,
     * so an exit=1 printed there is not a turn error. Real tool failures and
     * plain exit=/error: lines still paint 错误>. */
    if (s[0] == ' ' && s[1] == ' ' &&
        (unsigned char)s[2] == 0xE2 && (unsigned char)s[3] == 0x94 && (unsigned char)s[4] == 0x82 &&
        strstr(s, ".c:"))
        return;
    if (!tui_looks_err(s)) return;
    for (i = 0; s[i] && o + 1 < (int)sizeof st->errline; i++) {
        char c = s[i];
        if (c == '\n' || c == '\r' || c == '\t') c = ' ';
        st->errline[o++] = c;
    }
    st->errline[o] = 0;
}

/* The line a live turn actually runs. The goal text stays intact; this only
 * tells the model to continue that goal instead of starting over. */
static void tui_goal_prompt(const tui_state *st, char *out, int n) {
    snprintf(out, (size_t)n,
             "目标：%s。继续这一目标，做完就停。要加功能先 bin/envelope 0:grkwjcgmcsih，不得先写入。",
             st->goal);
}

static void tui_drop_goal_pending(tui_state *st) {
    char want[TUI_INPUT_MAX];
    int i, w = 0;
    if (!st->goal[0]) { return; }
    tui_goal_prompt(st, want, (int)sizeof want);
    for (i = 0; i < st->npending; i++) {
        if (!strcmp(st->pending[i], want)) continue;
        if (w != i) memcpy(st->pending[w], st->pending[i], TUI_INPUT_MAX);
        w++;
    }
    st->npending = w;
}

static void tui_queue_goal(tui_state *st) {
    char line[TUI_INPUT_MAX];
    if (!st->goal[0] || st->loop_left <= 0 || st->npending >= 8) return;
    tui_goal_prompt(st, line, (int)sizeof line);
    snprintf(st->pending[st->npending], TUI_INPUT_MAX, "%s", line);
    st->npending++;
    st->loop_left--;
}

/* Slash commands never start a model turn. /loop on queues the goal so the
 * live loop can start it while idle. Returns 1 when the line was one. */
static int tui_slash(tui_state *st) {
    if (!strcmp(st->input, "/goal")) {
        st->input[0] = '\0';
        st->ninput = 0;
        return 1;
    }
    if (!strncmp(st->input, "/goal ", 6)) {
        if (!st->input[6]) {
            tui_drop_goal_pending(st);
            st->goal[0] = '\0';
            st->loop_on = 0;
            st->loop_left = 0;
        } else {
            snprintf(st->goal, sizeof st->goal, "%s", st->input + 6);
        }
        st->input[0] = '\0';
        st->ninput = 0;
        return 1;
    }
    if (!strncmp(st->input, "/loop", 5) && (st->input[5] == '\0' || st->input[5] == ' ')) {
        if (!st->goal[0] || st->loop_on) {
            if (st->goal[0]) tui_drop_goal_pending(st);
            st->loop_on = 0;
            st->loop_left = 0;
        } else {
            st->loop_on = 1;
            st->loop_left = TUI_LOOP_MAX;
            if (!st->busy) tui_queue_goal(st);
        }
        st->input[0] = '\0';
        st->ninput = 0;
        return 1;
    }
    /* Re-read the key file on the next call. Pages are already read every
     * frame. The command itself is consumed. A busy turn is left running. */
    if (!strcmp(st->input, "/reload")) {
        if (st->busy)
            tui_log_plain(st, "reload: 请求中，没重载");
        else if (net_deepseek_forget())
            tui_log_plain(st, "reload: 页每帧已重读，密钥缓存已清");
        else
            tui_log_plain(st, "reload: 密钥文件打不开，沿用已有密钥");
        st->input[0] = '\0';
        st->ninput = 0;
        return 1;
    }
    return 0;
}

static void tui_type(tui_state *st, const char *s) {
    for (; s && *s; s++) tui_apply_key(st, TERM_KEY_CHAR, *s);
    tui_apply_key(st, TERM_KEY_ENTER, 0);
}

static int tui_frame_has(tui_state *st, const char *needle) {
    r_frame f = tui_render_state(st, 40);
    int i;
    for (i = 0; i < f.n; i++)
        if (f.lines[i] && strstr(f.lines[i], needle)) return 1;
    return 0;
}

/* One byte into the input. A paste newline uses this too. */
static void tui_input_byte(tui_state *st, char ch) {
    st->ask_exit = 0;
    if (st->ninput < TUI_INPUT_MAX - 1) {
        st->input[st->ninput++] = ch;
        st->input[st->ninput] = '\0';
    } else {
        st->notice = "input full";
    }
}

static void tui_submit(tui_state *st) {
    st->ask_exit = 0;
    if (!strcmp(st->input, "/exit") || !strcmp(st->input, "/quit")) {
        st->input[0] = '\0';
        st->ninput = 0;
        st->quit = 1;
        return;
    }
    /* In agent mode, Enter hands the prompt to the DeepSeek loop (which
     * drives the file/exec tools and, via exec, the live tmux windows). */
    if (st->mode == 1) {
        if (!st->input[0]) return;
        if (tui_slash(st)) return;
        if (st->busy) { tui_enqueue(st); return; }
        tui_run_agent(st);
        return;
    }
    /* Enter runs a real turn: the line is recorded, the gate judges the
     * newest probability in the transcript, and the verdict is written
     * back. The verdict goes to `last` (shown in the frame) AND into the
     * transcript (so the next turn can see it) — a verdict only on screen
     * would not be a closed loop. */
    if (st->input[0]) {
        tool_result tr = tool_run(st->input);
        /* Show the first line of the result: the frame is a fixed number of
         * rows, and a long listing would push everything else off screen. */
        {
            size_t i = 0;
            while (tr.out[i] && tr.out[i] != '\n' && i < sizeof st->output - 1) i++;
            memcpy(st->output, tr.out, i);
            st->output[i] = '\0';
            if (tr.out[i] == '\n') st->notice = "ran (output continues; see 'read')";
            else st->notice = tr.ok ? "ok" : "failed";
        }
    }
    if (st->transcript) {
        chat_turn t = chat_turn_run(st->transcript, st->input, 0.5);
        snprintf(st->last, sizeof st->last, "%s", t.verdict);
    } else if (!st->input[0]) {
        st->notice = "empty";
    }
    st->ninput = 0;
    st->input[0] = '\0';
}

static void tui_apply_key(tui_state *st, int kind, char ch) {
    switch (kind) {
    case -1:
        st->ticks++;
        return;
    case TERM_KEY_CHAR:
        tui_input_byte(st, ch);
        return;
    case TERM_KEY_BACKSPACE:
        st->ask_exit = 0;
        /* One code point. A continuation byte is not its own character. */
        if (st->ninput > 0) {
            int i = st->ninput - 1;
            while (i > 0 && ((unsigned char)st->input[i] & 0xC0) == 0x80) i--;
            st->input[i] = '\0';
            st->ninput = i;
        }
        return;
    case TERM_KEY_PASTE_ON:
        st->pasting = 1;
        return;
    case TERM_KEY_PASTE_OFF:
        /* The whole paste is one message. Newlines inside it were not Enter. */
        st->pasting = 0;
        if (st->input[0]) tui_submit(st);
        return;
    case TERM_KEY_ENTER:
        /* A newline that still has keys behind it is part of a paste or a
         * burst, not a send. A lone Enter sends whatever is in the box. */
        if (st->pasting || st->more_keys) {
            tui_input_byte(st, '\n');
            return;
        }
        tui_submit(st);
        return;
    case TERM_KEY_CTRL_C:
        /* A typed line is cleared. An empty line does not quit: say so.
         * While a turn is in flight and the line is already empty, cancel
         * that turn. Exit is Ctrl-D on an empty line, or /exit, or /quit. */
        if (st->ninput > 0) {
            st->input[0] = '\0';
            st->ninput = 0;
            st->ask_exit = 0;
            return;
        }
        if (st->busy) { st->cancel = 1; net_cancel(); return; }
        st->ask_exit = 1;
        return;
    case TERM_KEY_CTRL_D:
        if (st->ninput > 0) return;
        if (st->busy) { st->cancel = 1; net_cancel(); }
        st->quit = 1;
        return;
    case TERM_KEY_UP:
        if (st->mode != 1) { st->notice = "arrow keys not bound yet"; return; }
        tui_log_scroll(st, 1);
        return;
    case TERM_KEY_DOWN:
        if (st->mode != 1) { st->notice = "arrow keys not bound yet"; return; }
        tui_log_scroll(st, -1);
        return;
    case TERM_KEY_PGUP:
        if (st->mode != 1) { st->notice = "arrow keys not bound yet"; return; }
        tui_log_scroll(st, tui_log_room(st));
        return;
    case TERM_KEY_PGDN:
        if (st->mode != 1) { st->notice = "arrow keys not bound yet"; return; }
        tui_log_scroll(st, -tui_log_room(st));
        return;
    case TERM_KEY_HOME:
        if (st->mode != 1) { st->notice = "arrow keys not bound yet"; return; }
        tui_log_scroll(st, 40);
        return;
    case TERM_KEY_END:
        if (st->mode != 1) { st->notice = "arrow keys not bound yet"; return; }
        st->log_skip = 0;
        return;
    case TERM_KEY_UNKNOWN:
        /* Never swallowed: an ignored key reads as "the UI is broken". */
        st->notice = "unknown key (ignored)";
        return;
    default:
        st->notice = "arrow keys not bound yet";
        return;
    }
}

/* Rows under the mind rule. Left is 思维树.md, right is 记忆宫殿.md.
 * state: 0 has lines, 1 file missing, 2 file empty.
 * Fixed rows are title, goal, input, hint, error, the two rules, and this block.
 * System body and pending lines are counted at the call. */
#define TUI_MIND_N 12
#define TUI_FIXED_ROWS (7 + TUI_MIND_N)

/* Mind pages are read whole and folded to the column width, one screen row
 * per folded piece. Continuation rows start with two spaces. */
#define TUI_MIND_SRC  400
#define TUI_MIND_WRAP 800

/* Terminal columns, not bytes. The measure itself is csih_cols.h. */
static int tui_disp_width(const char *s) {
    int w = 0;
    if (!s) return 0;
    while (*s) {
        unsigned int cp = 0;
        int n = tui_next_cp(s, &cp);
        if (n <= 0) break;
        w += tui_cp_cols(cp);
        s += n;
    }
    return w;
}

static int tui_utf8_fit(const char *s, int cols, char *out) {
    int i = 0, n = 0, w = 0;
    if (!s) s = "";
    if (cols < 0) cols = 0;
    while (s[i] && w < cols) {
        unsigned int cp = 0;
        int need = tui_next_cp(s + i, &cp);
        int cw;
        if (need <= 0) break;
        cw = tui_cp_cols(cp);
        if (w + cw > cols) break;
        memcpy(out + n, s + i, (size_t)need);
        n += need;
        i += need;
        w += cw;
    }
    out[n] = '\0';
    return n;
}

static void tui_pad_disp(char *s, int cols) {
    int n = (int)strlen(s);
    int w = tui_disp_width(s);
    while (w < cols && n < R_LINE_MAX) {
        s[n++] = ' ';
        w++;
    }
    s[n] = '\0';
}

/* Fences and a bare `flowchart LR` are mermaid wrappers, not the map. */
static int tui_mind_noise(const char *s) {
    const char *p = s ? s : "";
    char c;
    while (*p == ' ' || *p == '\t') p++;
    if (!strncmp(p, "```", 3)) return 1;
    if (strncmp(p, "flowchart", 9) != 0) return 0;
    c = p[9];
    return c == '\0' || c == ' ' || c == '\t';
}

static void tui_page_lines(const char *name, char got[][R_LINE_MAX + 1],
                           int maxlines, int *nlines, int *state) {
    char path[512], buf[512];
    FILE *fp;
    const char *home = getenv("HOME");
    *nlines = 0;
    *state = 1;
    if (!home || !home[0]) return;
    csih_home_bind(home);
    snprintf(path, sizeof path, "%s/.csih/%s", home, name);
    fp = fopen(path, "r");
    if (!fp) {
        snprintf(path, sizeof path, "%s/.cdsh/%s", home, name);
        fp = fopen(path, "r");
    }
    if (!fp) return;
    *state = 2;
    while (fgets(buf, sizeof buf, fp)) {
        size_t len = strlen(buf);
        while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) buf[--len] = '\0';
        if (!buf[0] || tui_mind_noise(buf)) continue;
        *state = 0;
        if (*nlines >= maxlines) break;
        (*nlines)++;
        snprintf(got[*nlines - 1], R_LINE_MAX + 1, "%s", buf);
    }
    fclose(fp);
}

static void tui_col_put(char *side, int width, const char *text) {
    int n = tui_utf8_fit(text ? text : "", width, side);
    int w = tui_disp_width(side);
    while (w < width && n < R_LINE_MAX) {
        side[n++] = ' ';
        w++;
    }
    side[n] = '\0';
}

/* Column of the vertical bar between 思维树 and 记忆宫殿. -1 if the row
 * is too narrow to spare one column. */
static int tui_mind_bar(int cols, int *left_w, int *right_w) {
    int left, right;
    if (cols < 3) {
        if (left_w) *left_w = cols > 0 ? cols : 1;
        if (right_w) *right_w = 0;
        return -1;
    }
    left = (cols - 1) / 2;
    right = cols - left - 1;
    if (left_w) *left_w = left;
    if (right_w) *right_w = right;
    return left;
}

/* `label` then '-' out to `width` columns. The label is the rule. */
static int tui_label_rule(char *dst, int dstmax, const char *label, int width) {
    int li = 0, o = 0, w = 0;
    if (!dst || dstmax < 1) return 0;
    dst[0] = '\0';
    if (width < 1) return 0;
    if (!label) label = "";
    while (label[li] && o + 4 < dstmax && w < width) {
        unsigned int cp = 0;
        int need = tui_next_cp(label + li, &cp);
        int cw;
        if (need <= 0) break;
        cw = tui_cp_cols(cp);
        if (w + cw > width) break;
        memcpy(dst + o, label + li, (size_t)need);
        o += need;
        li += need;
        w += cw;
    }
    while (w < width && o + 1 < dstmax) {
        dst[o++] = '-';
        w++;
    }
    dst[o] = '\0';
    return o;
}

/* Screen row of the system rule, and the half-open body range under it.
 * Filled by the render that just ran. A click or a wheel looks here. */
#define TUI_SYS_N     10
#define TUI_SYS_WRAP  80
#define TUI_MIND_SRC  200
#define TUI_MIND_WRAP 80
static int tui_sys_rule_y;
static int tui_sys_y0;
static int tui_sys_y1;
static int tui_mind_y0;
static int tui_mind_y1;

/* Top edge of the system block. Collapsed is [展开], open is [收缩]. */
static void tui_sys_rule(r_state *rs, char line[][R_LINE_MAX + 1], int *n,
                         int cols, const tui_state *st) {
    const char *label;
    if (*n >= 63 || cols < 1) return;
    label = (st && st->sys_open) ? "-<系统提示词>[收缩]>" : "-<系统提示词>[展开]>";
    tui_sys_rule_y = *n + 2;
    tui_label_rule(line[*n], R_LINE_MAX + 1, label, cols);
    rs->body[*n] = line[*n];
    (*n)++;
}

/* Top edge of the mind block. Titles sit on the rule, split by ┬. */
static void tui_mind_rule(r_state *rs, char line[][R_LINE_MAX + 1], int *n, int cols) {
    char left[R_LINE_MAX + 1], right[R_LINE_MAX + 1];
    int half, rest, bar;
    if (*n >= 63 || cols < 1) return;
    bar = tui_mind_bar(cols, &half, &rest);
    if (bar < 0) {
        tui_label_rule(line[*n], R_LINE_MAX + 1, "-<思维树>", cols);
    } else {
        tui_label_rule(left, (int)sizeof left, "-<思维树>", half);
        tui_label_rule(right, (int)sizeof right, "-<记忆宫殿>", rest);
        snprintf(line[*n], R_LINE_MAX + 1, "%s┬%s", left, right);
    }
    rs->body[*n] = line[*n];
    (*n)++;
}

/* Ten wrapped rows of the system prompt when open. Closed draws none.
 * The window starts at sys_top. A short prompt is padded to ten rows. */
static void tui_sys_block(r_state *rs, char line[][R_LINE_MAX + 1], int *n,
                          int cols, tui_state *st) {
    static char flat[4096];
    static char wrap[TUI_SYS_WRAP][R_LINE_MAX + 1];
    const char *src = agent_model_rules();
    const char *p;
    int i = 0, o = 0, row, width, used, nrows, top, maxtop;
    tui_sys_y0 = 0;
    tui_sys_y1 = 0;
    if (!st || !st->sys_open) return;
    if (!src) src = "";
    while (src[i] && o + 1 < (int)sizeof flat) {
        char c = src[i++];
        if (c == '\n' || c == '\r' || c == '\t') c = ' ';
        if (c == ' ' && (o == 0 || flat[o - 1] == ' ')) continue;
        flat[o++] = c;
    }
    flat[o] = '\0';
    p = flat;
    width = cols;
    if (width < 1) width = 1;
    nrows = 0;
    while (*p && nrows < TUI_SYS_WRAP) {
        used = tui_utf8_fit(p, width, wrap[nrows]);
        if (used <= 0) {
            unsigned int cp = 0;
            int need = tui_next_cp(p, &cp);
            if (need <= 0) break;
            p += need;
            continue;
        }
        nrows++;
        p += used;
        while (*p == ' ') p++;
    }
    top = st->sys_top;
    maxtop = nrows > TUI_SYS_N ? nrows - TUI_SYS_N : 0;
    if (top < 0) top = 0;
    if (top > maxtop) top = maxtop;
    /* Wheel may run several times before the next draw. Remember the
     * clamped row so one wheel-up leaves the end instead of unwinding. */
    st->sys_top = top;
    tui_sys_y0 = *n + 2;
    for (row = 0; row < TUI_SYS_N && *n < 63; row++) {
        const char *piece = (top + row < nrows) ? wrap[top + row] : "";
        snprintf(line[*n], R_LINE_MAX + 1, "%s", piece);
        rs->body[*n] = line[*n];
        (*n)++;
    }
    tui_sys_y1 = *n + 2;
}

/* Fold a page into rows fitted to `width`. Stops at maxrows.
 * Returns the number of source lines consumed. *rows is the fitted count. */
static int tui_mind_fold(char src[][R_LINE_MAX + 1], int n,
                         char dst[][R_LINE_MAX + 1], int maxrows,
                         int *rows, int width, int state) {
    int i, out = 0;
    if (n <= 0) {
        if (state == 1) snprintf(dst[0], R_LINE_MAX + 1, "(无)");
        else if (state == 2) snprintf(dst[0], R_LINE_MAX + 1, "(空)");
        else dst[0][0] = '\0';
        if (maxrows > 0) out = 1;
        if (rows) *rows = out;
        return n;
    }
    if (width < 1) width = 1;
    for (i = 0; i < n; i++) {
        const char *p = src[i];
        int used, first = 1;
        if (!p[0]) p = " ";
        while (*p && out < maxrows) {
            int w = width;
            if (!first) {
                /* Continuation of a wrapped note: indent two spaces so the
                 * next line reads as the same note, not a new one. */
                int k;
                for (k = 0; k < 2 && k < w - 1; k++) dst[out][k] = ' ';
                used = tui_utf8_fit(p, w - k, dst[out] + k);
                if (used > 0 && p[used]) {
                    /* Prefer a break at space/punctuation over a cut mid-token. */
                    int b = 0, bo = 0;
                    while (b < used) {
                        unsigned int cp = 0;
                        int need = tui_next_cp(p + b, &cp);
                        if (need <= 0) break;
                        b += need;
                        if (cp == ' ' || cp == '-' || cp == '/' || cp == ',' || cp == '.' ||
                            (cp >= 0x3000 && cp <= 0x303F) || (cp >= 0xFF01 && cp <= 0xFF60))
                            bo = b;
                    }
                    if (bo > 0) used = bo;
                }
                if (used <= 0) {
                    unsigned int cp = 0;
                    int need = tui_next_cp(p, &cp);
                    if (need <= 0) break;
                    p += need;
                    continue;
                }
                dst[out][k + used] = '\0';
                p += used;
                out++;
                continue;
            }
            used = tui_utf8_fit(p, w, dst[out]);
            if (used > 0 && p[used]) {
                /* Prefer a break at space/punctuation over a cut mid-token. */
                int b = 0, bo = 0;
                while (b < used) {
                    unsigned int cp = 0;
                    int need = tui_next_cp(p + b, &cp);
                    if (need <= 0) break;
                    b += need;
                    if (cp == ' ' || cp == '-' || cp == '/' || cp == ',' || cp == '.' ||
                        (cp >= 0x3000 && cp <= 0x303F) || (cp >= 0xFF01 && cp <= 0xFF60))
                        bo = b;
                }
                if (bo > 0) used = bo;
            }
            if (used <= 0) {
                unsigned int cp = 0;
                int need = tui_next_cp(p, &cp);
                if (need <= 0) break;
                p += need;
                continue;
            }
            dst[out][used] = '\0';
            p += used;
            out++;
            first = 0;
        }
        if (out >= maxrows) break;
    }
    if (rows) *rows = out;
    return n;
}

static void tui_mind_block(r_state *rs, char line[][R_LINE_MAX + 1], int *n, int cols, tui_state *st) {
    static char tree[TUI_MIND_SRC][R_LINE_MAX + 1];
    static char palace[TUI_MIND_SRC][R_LINE_MAX + 1];
    static char lwrap[TUI_MIND_WRAP][R_LINE_MAX + 1];
    static char rwrap[TUI_MIND_WRAP][R_LINE_MAX + 1];
    char left[R_LINE_MAX + 1], right[R_LINE_MAX + 1];
    int tn = 0, pn = 0, ts = 1, ps = 1, half, rest, bar;
    int lrows = 0, rrows = 0, total, maxtop, top, row, i;
    tui_mind_y0 = 0;
    tui_mind_y1 = 0;
    if (!st) return;
    tui_page_lines("思维树.md", tree, TUI_MIND_SRC, &tn, &ts);
    tui_page_lines("记忆宫殿.md", palace, TUI_MIND_SRC, &pn, &ps);
    bar = tui_mind_bar(cols, &half, &rest);
    tn = tui_mind_fold(tree, tn, lwrap, TUI_MIND_WRAP, &lrows, half, ts);
    pn = tui_mind_fold(palace, pn, rwrap, TUI_MIND_WRAP, &rrows, rest, ps);
    (void)tn;
    (void)pn;
    total = lrows > rrows ? lrows : rrows;
    if (total < 1) total = 1;
    maxtop = total > TUI_MIND_N ? total - TUI_MIND_N : 0;
    top = st->mind_top;
    if (top < 0) top = 0;
    if (top > maxtop) top = maxtop;
    st->mind_top = top;
    if (*n >= 63) return;
    tui_mind_y0 = *n + 2;
    for (row = 0; row < TUI_MIND_N && *n < 63; row++) {
        int idx = top + row;
        const char *lt = (idx >= 0 && idx < lrows) ? lwrap[idx] : "";
        const char *rt = (idx >= 0 && idx < rrows) ? rwrap[idx] : "";
        tui_col_put(left, half, lt);
        tui_col_put(right, rest, rt);
        if (bar < 0)
            snprintf(line[*n], R_LINE_MAX + 1, "%s%s", left, right);
        else
            snprintf(line[*n], R_LINE_MAX + 1, "%s│%s", left, right);
        rs->body[*n] = line[*n];
        (*n)++;
    }
    tui_mind_y1 = *n + 2;
}

/* Build the frame for a state. The two mind lines also follow the files on
 * disk, so a later redraw can differ after mind writes a page. */
/* Screen row of each visible exec prefix or its one-line command.
 * Filled by the render that just ran. A click looks here, not at pixels. */
#define TUI_HIT_MAX 80
static int tui_hit_y[TUI_HIT_MAX];
static int tui_hit_i[TUI_HIT_MAX];
static int tui_hit_n;

static void tui_hit_add(int y, int logi) {
    if (tui_hit_n >= TUI_HIT_MAX) return;
    tui_hit_y[tui_hit_n] = y;
    tui_hit_i[tui_hit_n] = logi;
    tui_hit_n++;
}

static void tui_click(tui_state *st, int x, int y) {
    int h;
    (void)x;
    if (y == tui_sys_rule_y && tui_sys_rule_y > 0) {
        st->sys_open = !st->sys_open;
        st->sys_top = 0;
        return;
    }
    for (h = 0; h < tui_hit_n; h++) {
        int i;
        if (tui_hit_y[h] != y) continue;
        i = tui_hit_i[h];
        if (i < 0 || i >= st->nlog || !st->ex_on[i]) return;
        st->ex_open[i] = !st->ex_open[i];
        return;
    }
}

/* Log entries the viewport can hold before an open exec shrinks it. */
static int tui_log_room(const tui_state *st) {
    int rows = st->rows > 0 ? st->rows : 24;
    int sys_rows = st->sys_open ? TUI_SYS_N : 0;
    int room = rows - TUI_FIXED_ROWS - sys_rows - st->npending;
    if (room < 1) room = 1;
    if (room > TUI_LOG_VIEW) room = TUI_LOG_VIEW;
    return room;
}

/* Visible log entries are [start, end). end hides log_skip newest rows. */
static void tui_log_range(tui_state *st, int *start_out, int *end_out, int *show_out) {
    int show = tui_log_room(st);
    int skip = st->log_skip;
    int end, start, extra, i;
    if (skip < 0) skip = 0;
    end = st->nlog - skip;
    if (end < 0) end = 0;
    start = end - show;
    if (start < 0) start = 0;
    extra = 0;
    for (i = start; i < end; i++)
        if (st->ex_on[i] && st->ex_open[i]) extra++;
    if (extra > 0 && show > extra) show -= extra;
    {
        /* Opening a row spends a slot. Keep that row on screen. */
        int old_start = start;
        int limit = end;
        start = limit - show;
        if (start < 0) start = 0;
        for (i = old_start; i < limit; i++) {
            if (i < start && st->ex_on[i] && st->ex_open[i]) {
                start = i;
                end = start + show;
                if (end > limit) end = limit;
                break;
            }
        }
    }
    if (start_out) *start_out = start;
    if (end_out) *end_out = end;
    if (show_out) *show_out = show;
}

static void tui_log_clamp(tui_state *st) {
    int show = tui_log_room(st);
    int end, start, extra, i, max;
    if (st->log_skip < 0) st->log_skip = 0;
    end = st->nlog - st->log_skip;
    if (end < 0) end = 0;
    start = end - show;
    if (start < 0) start = 0;
    extra = 0;
    for (i = start; i < end; i++)
        if (st->ex_on[i] && st->ex_open[i]) extra++;
    if (extra > 0 && show > extra) show -= extra;
    max = st->nlog - show;
    if (max < 0) max = 0;
    if (st->log_skip > max) st->log_skip = max;
}

/* A new line while reading history stays below the window. */
static void tui_log_hold(tui_state *st) {
    if (st->log_skip > 0) {
        st->log_skip++;
        tui_log_clamp(st);
    }
}

static void tui_log_scroll(tui_state *st, int delta) {
    if (!st || st->mode != 1) return;
    st->log_skip += delta;
    tui_log_clamp(st);
}

/* Screen rows of the log viewport. Filled by the render that just ran. */
static int tui_log_y0;
static int tui_log_y1;

/* Wheel over the open system rows still moves that block.
 * dir 1 moves down, dir 2 moves up. A wheel on the log viewport
 * moves history: up shows older entries, down returns toward the tail.
 * Three entries per notch. Anywhere else is ignored. */
static void tui_wheel(tui_state *st, int y, int dir) {
    if (st->sys_open && tui_sys_y0 > 0 && y >= tui_sys_y0 && y < tui_sys_y1) {
        if (dir == 1) st->sys_top++;
        else if (dir == 2 && st->sys_top > 0) st->sys_top--;
        return;
    }
    if (tui_mind_y0 > 0 && y >= tui_mind_y0 && y < tui_mind_y1) {
        if (dir == 1) st->mind_top++;
        else if (dir == 2 && st->mind_top > 0) st->mind_top--;
        return;
    }
    if (tui_log_y0 > 0 && y >= tui_log_y0 && y < tui_log_y1) {
        if (dir == 2) tui_log_scroll(st, 3);
        else if (dir == 1) tui_log_scroll(st, -3);
    }
}

/* One frame row. A raw newline would split the redraw. Draw it as ⏎,
 * and stop on a code-point boundary so a long line is not cut in half.
 * dstmax is the byte budget of one row; the text is wrapped by display
 * columns, so a long event line becomes several rows instead of being
 * cut at the buffer edge. */
static int tui_put_folded_n(char *dst, int dstmax, const char *prefix,
                            const char *src, int cols, int cont) {
    int o = 0, i = 0, w = 0;
    int plen = 0;
    if (!dst || dstmax < 1) return i;
    if (!prefix) prefix = "";
    if (!src) src = "";
    /* Continuation rows keep the prefix column blank so the wrapped
     * body lines up under the first row. */
    while (prefix[o] && o + 1 < dstmax) {
        dst[o] = cont ? ' ' : prefix[o];
        o++;
        plen++;
    }
    if (prefix[o]) { dst[o] = '\0'; return i; }
    w = cont ? 0 : tui_disp_width(prefix);
    if (cols < 1) cols = R_LINE_MAX;
    while (src[i]) {
        unsigned int cp = 0;
        int need, cw;
        if (src[i] == '\n' || src[i] == '\r') {
            if (w + 1 > cols) break;
            if (o + 4 > dstmax) break;
            memcpy(dst + o, "⏎", 3);
            o += 3;
            w += 1;
            if (src[i] == '\r' && src[i + 1] == '\n') i++;
            i++;
            continue;
        }
        need = tui_next_cp(src + i, &cp);
        if (need <= 0) break;
        cw = tui_cp_cols(cp);
        if (w + cw > cols) break;
        if (o + need >= dstmax) break;
        memcpy(dst + o, src + i, (size_t)need);
        o += need;
        i += need;
        w += cw;
    }
    dst[o] = '\0';
    (void)plen;
    return i;
}

/* One folded row, wrapped to `cols` display columns. Kept as a thin
 * wrapper so existing callers keep working. */
static void tui_put_folded(char *dst, int dstmax, const char *prefix, const char *src) {
    int o = 0, i = 0;
    if (!dst || dstmax < 1) return;
    if (!prefix) prefix = "";
    if (!src) src = "";
    while (prefix[o] && o + 1 < dstmax) {
        dst[o] = prefix[o];
        o++;
    }
    if (prefix[o]) { dst[o] = '\0'; return; }
    while (src[i] && o + 1 < dstmax) {
        unsigned int cp = 0;
        int need;
        if (src[i] == '\n' || src[i] == '\r') {
            if (o + 4 > dstmax) break;
            memcpy(dst + o, "⏎", 3);
            o += 3;
            if (src[i] == '\r' && src[i + 1] == '\n') i++;
            i++;
            continue;
        }
        need = tui_next_cp(src + i, &cp);
        if (need <= 0) break;
        if (o + need >= dstmax) break;
        memcpy(dst + o, src + i, (size_t)need);
        o += need;
        i += need;
    }
    dst[o] = '\0';
}

/* One input row. A paste keeps its newlines, drawn as ⏎, and the row
 * stays within `cols` so a long paste cannot fail the width check. */
static void tui_input_row(char *dst, int dstmax, const char *input, int cols) {
    char shown[R_LINE_MAX + 1];
    char piece[R_LINE_MAX + 1];
    int width;
    tui_put_folded(shown, (int)sizeof shown, "", input ? input : "");
    width = cols - 2;
    if (width < 1) width = 1;
    tui_utf8_fit(shown, width, piece);
    snprintf(dst, (size_t)dstmax, "> %s", piece);
}

static r_frame tui_render_state(tui_state *st, int cols) {
    r_state rs;
    r_frame f;
    /* One buffer per body line: render() copies each into the frame, so the
     * pointers only need to outlive the render() call. A single shared buffer
     * here would mean every body line aliases the last one. */
    char line[64][R_LINE_MAX + 1];
    int n = 0, i;
    char hint[160];

    memset(&rs, 0, sizeof rs);
    tui_hit_n = 0;
    tui_sys_rule_y = 0;
    tui_sys_y0 = 0;
    tui_sys_y1 = 0;
    tui_log_y0 = 0;
    tui_log_y1 = 0;
    rs.title = (st->mode == 1) ? "csih · agent" : "csih";
    rs.width = cols;
    /* Idle agent hint stays fixed. While a call is in flight, show whole
     * seconds only. Cache hit/miss appear only after a real usage object.
     * This line sits under the input and above the mind block. */
    if (st->mode == 1) {
        int sec = 0, tokens = 0, hit = 0, miss = 0, seen = 0;
        long sent = 0, got = 0;
        if (st->busy) {
            net_progress(&sec, &tokens, &sent);
            net_recv(&got);
            net_cache(&hit, &miss, &seen);
            if (seen)
                snprintf(hint, sizeof hint,
                         "请求中 %d秒 %d词元 上传 %ld字节 已收 %ld字节 hit %d miss %d",
                         sec, tokens, sent, got, hit, miss);
            else
                snprintf(hint, sizeof hint,
                         "请求中 %d秒 %d词元 上传 %ld字节 已收 %ld字节",
                         sec, tokens, sent, got);
        } else if (st->ask_exit)
            snprintf(hint, sizeof hint, "要退出请按 Ctrl-D");
        else if (st->log_skip > 0)
            snprintf(hint, sizeof hint, "更早%d · End回底", st->log_skip);
        else if (cols >= 72)
            snprintf(hint, sizeof hint,
                     "Ctrl-C 清空 · Ctrl-D 退出 · Enter 发送 · ↑↓翻历史 PgUp/PgDn Home/End");
        else
            snprintf(hint, sizeof hint, "Ctrl-C 清空 · Ctrl-D 退出 · Enter 发送");
    } else if (st->ask_exit)
        snprintf(hint, sizeof hint, "要退出请按 Ctrl-D · %d", st->ticks);
    else
        snprintf(hint, sizeof hint, "Ctrl-C 清空 · Ctrl-D 退出 · %d", st->ticks);

    if (st->mode == 1) {
        /* Log viewport is TUI_LOG_VIEW when the terminal allows it.
         * Fewer log lines are padded so the block does not collapse.
         * TUI_FIXED_ROWS already includes the mind block. */
        int show, starti = 0, endi = 0, view_at, filled;
        tui_log_clamp(st);
        tui_log_range(st, &starti, &endi, &show);
        view_at = n;
        tui_log_y0 = view_at + 2;
        for (i = starti; i < endi && n < 40; i++) {
            if (st->ex_on[i]) {
                char tag[32];
                char head[220];
                char cond[40];
                const char *kind = st->ex_tag[i][0] ? st->ex_tag[i] : "exec";
                /* Title is frame row 1, so body index n is screen row n+2.
                 * Closed: the explanation and a short call. Open: that line,
                 * the full call when it was shortened, then the tool output. */
                snprintf(tag, sizeof tag, "%s[%s]> ", kind,
                         st->ex_open[i] ? "收缩" : "展开");
                tui_condense(cond, (int)sizeof cond, st->ex_cmd[i], 28);
                if (cond[0])
                    snprintf(head, sizeof head, "%s · %s", st->ex_why[i], cond);
                else
                    snprintf(head, sizeof head, "%s", st->ex_why[i]);
                {
                    const char *p = head;
                    int cont = 0;
                    while (*p && n < 40) {
                        int used = tui_put_folded_n(line[n], (int)sizeof line[n],
                                                    tag, p, cols, cont);
                        rs.body[n] = line[n];
                        tui_hit_add(n + 2, i);
                        n++;
                        if (used <= 0) break;
                        p += used;
                        cont = 1;
                    }
                    if (n == 0) { line[0][0] = '\0'; }
                }
                if (st->ex_open[i] && n < 40 && st->ex_cmd[i][0]
                    && strcmp(st->ex_cmd[i], cond) != 0) {
                    const char *p = st->ex_cmd[i];
                    int cont = 0;
                    while (*p && n < 40) {
                        int used = tui_put_folded_n(line[n], (int)sizeof line[n],
                                                    "  ", p, cols, cont);
                        rs.body[n] = line[n];
                        tui_hit_add(n + 2, i);
                        n++;
                        if (used <= 0) break;
                        p += used;
                        cont = 1;
                    }
                }
                if (st->ex_open[i] && n < 40 && st->ex_body[i][0]) {
                    const char *p = st->ex_body[i];
                    int cont = 0, brows = 0;
                    while (*p && n < 40 && brows < 8) {
                        int used = tui_put_folded_n(line[n], (int)sizeof line[n],
                                                    "  ", p, cols, cont);
                        rs.body[n] = line[n];
                        tui_hit_add(n + 2, i);
                        n++;
                        brows++;
                        if (used <= 0) break;
                        p += used;
                        cont = 1;
                    }
                }
            } else {
                {
                    const char *p = st->log[i];
                    int cont = 0;
                    while (*p && n < 40) {
                        int used = tui_put_folded_n(line[n], (int)sizeof line[n],
                                                    "", p, cols, cont);
                        rs.body[n] = line[n]; n++;
                        if (used <= 0) break;
                        p += used;
                        cont = 1;
                    }
                }
            }
        }
        filled = n - view_at;
        while (filled < show && n < 40) {
            line[n][0] = '\0';
            rs.body[n] = line[n];
            n++;
            filled++;
        }
        tui_log_y1 = n + 2;
        {
            char gfold[R_LINE_MAX + 1];
            char suffix[32];
            int glen;
            snprintf(suffix, sizeof suffix, " · loop> %s", st->loop_on ? "on" : "off");
            glen = (int)sizeof line[n] - (int)strlen("goal> ") - (int)strlen(suffix);
            if (glen < 1) glen = 1;
            tui_put_folded_n(gfold, glen, "", st->goal, glen, 0);
            snprintf(line[n], sizeof line[n], "goal> %s%s", gfold, suffix);
        }
        rs.body[n] = line[n]; n++;
        for (i = 0; i < st->npending && n < 62; i++) {
            {
                const char *p = st->pending[i];
                int cont = 0;
                while (*p && n < 62) {
                    int used = tui_put_folded_n(line[n], (int)sizeof line[n],
                                                "待发送> ", p, cols, cont);
                    rs.body[n] = line[n]; n++;
                    if (used <= 0) break;
                    p += used;
                    cont = 1;
                }
            }
        }
        tui_input_row(line[n], (int)sizeof line[n], st->input, cols);
        rs.body[n] = line[n]; n++;
        snprintf(line[n], sizeof line[n], "%s", hint);
        rs.body[n] = line[n]; n++;
        snprintf(line[n], sizeof line[n], "错误> %s", st->errline[0] ? st->errline : "-");
        rs.body[n] = line[n]; n++;
        tui_sys_rule(&rs, line, &n, cols, st);
        tui_sys_block(&rs, line, &n, cols, st);
        tui_mind_rule(&rs, line, &n, cols);
        tui_mind_block(&rs, line, &n, cols, st);
    } else {
        tui_input_row(line[n], (int)sizeof line[n], st->input, cols);
        rs.body[n] = line[n]; n++;
        snprintf(line[n], sizeof line[n], "%s", hint);
        rs.body[n] = line[n]; n++;
        snprintf(line[n], sizeof line[n], "错误> %s", st->errline[0] ? st->errline : "-");
        rs.body[n] = line[n]; n++;
        tui_sys_rule(&rs, line, &n, cols, st);
        tui_sys_block(&rs, line, &n, cols, st);
        tui_mind_rule(&rs, line, &n, cols);
        tui_mind_block(&rs, line, &n, cols, st);
        snprintf(line[n], sizeof line[n], "%s", st->notice ? st->notice : "");
        rs.body[n] = line[n]; n++;
        snprintf(line[n], sizeof line[n], "ticks %d  cols %d", st->ticks, cols);
        rs.body[n] = line[n]; n++;
        snprintf(line[n], sizeof line[n], "%s", st->last[0] ? st->last : "-");
        rs.body[n] = line[n]; n++;
        snprintf(line[n], sizeof line[n], "%s", st->output[0] ? st->output : "");
        rs.body[n] = line[n]; n++;
    }

    for (i = 0; i < n; i++) tui_pad_disp(line[i], cols);
    rs.nbody = n;
    rs.status = 0;
    f = render(&rs);
    return f;
}

/*
 * Redraw against the module-level previous frame. Kept as ONE function used by
 * both the main loop and the agent event callback, so a live event and the next
 * idle tick stay consistent — they share `tui_prev`.
 */
static r_frame tui_prev;
static int tui_repaint;

/* Rewrite every row. frame_diff skips unchanged lines, so a kernel line
 * (nfs server localhost:/: not responding) stays painted on a rule until
 * that row's text changes. A tick repaint puts the frame back. */
static void tui_paint_all(const r_frame *f) {
    char out[R_FRAME_BYTES + R_MAX_LINES * 24];
    size_t used = 0;
    int i;
    used += (size_t)snprintf(out + used, sizeof out - used, "\x1b[H");
    for (i = 0; i < f->n && used + 32 < sizeof out; i++) {
        used += (size_t)snprintf(out + used, sizeof out - used,
                                 "\x1b[%d;1H%s\x1b[K", i + 1, f->lines[i]);
    }
    if (used + 16 < sizeof out)
        used += (size_t)snprintf(out + used, sizeof out - used, "\x1b[J");
    if (used > 0) tui_write_all(out, used);
}

static void tui_redraw(tui_state *st) {
    r_frame next = tui_render_state(st, st->cols);
    char out[R_FRAME_BYTES + R_MAX_LINES * 16];
    /* A bad-width line would write a shorter frame over a longer one and leave
     * the previous frame's tail on screen — the classic redraw artifact. */
    if (tui_frame_bad_width(&next, st->cols)) return;
    if (tui_repaint) {
        tui_paint_all(&next);
        tui_repaint = 0;
    } else {
        out[0] = '\0';
        frame_diff(&tui_prev, &next, out, sizeof out);
        if (out[0]) tui_write_all(out, strlen(out));
    }
    tui_prev = next;
}

/*
 * The agent event callback: every step the model takes lands here, is appended
 * to the scrollback, and triggers an immediate redraw — so the screen updates
 * live while the (synchronous) agent loop is in flight. `ud` is the tui_state,
 * which is why this is a plain function pointer rather than a closure.
 */
static void tui_agent_event(const char *line, void *ud) {
    tui_state *st = (tui_state *)ud;
    tui_log_event(st, line);
    tui_redraw(st);
}

/*
 * Run the agent on the typed prompt. Enumerates the live tmux windows so the
 * model may drive them for interactive development, then drives agent_run_cb
 * with tui_agent_event as the live hook. On return, the answer and a summary are
 * appended to the scrollback and one final redraw shows them.
 */
static tui_state *net_tick_st;

/* Runs between curl_multi_wait slices. Must not call back into the agent. */
static void tui_during_net(void) {
    tui_state *st = net_tick_st;
    term_key_t batch[16];
    int nk, j, room;
    if (!st) return;
    if (term_wait_readable(0) == 1) {
        room = tui_q_room(st);
        if (room > 16) room = 16;
        if (room <= 0) { st->ticks++; tui_redraw(st); return; }
        nk = term_read_keys(batch, room);
        for (j = 0; j < nk; j++) {
            /* Cancel this turn only. quit would leave the read loop and the
             * pane would sit on the prompt with nobody reading keys. */
            st->more_keys = (j + 1 < nk);
            if (batch[j].kind == TERM_KEY_CTRL_C || batch[j].kind == TERM_KEY_CTRL_D) {
                tui_apply_key(st, (int)batch[j].kind, batch[j].ch);
                continue;
            }
            if (batch[j].kind == TERM_KEY_MOUSE) {
                tui_click(st, batch[j].x, batch[j].y);
                tui_redraw(st);
                continue;
            }
            if (batch[j].kind == TERM_KEY_WHEEL) {
                tui_wheel(st, batch[j].y, (int)(unsigned char)batch[j].ch);
                tui_redraw(st);
                continue;
            }
            /* Apply now so Enter can park the line. Queueing it would run
             * only after this call, and the old code dropped Enter entirely. */
            tui_apply_key(st, (int)batch[j].kind, batch[j].ch);
        }
        st->more_keys = 0;
    }
    st->ticks++;
    tui_repaint = 1;
    tui_redraw(st);
}

static const char *csih_env(const char *neu, const char *old) {
    const char *v = getenv(neu);
    if (v && v[0]) return v;
    v = getenv(old);
    if (v && v[0]) return v;
    return 0;
}

static int tui_run_agent(tui_state *st) {
    const char *endpoint = csih_env("CSIH_ENDPOINT", "CDSH_ENDPOINT");
    const char *model    = csih_env("CSIH_MODEL", "CDSH_MODEL");
    const char *cwd      = csih_env("CSIH_CWD", "CDSH_CWD");
    char journal[512];
    const char *transcript;
    char real_cwd[1024];
    char extra[1536];
    char prompt[TUI_INPUT_MAX];
    agent_result r;

    if (!endpoint) endpoint = "https://api.deepseek.com/v1/chat/completions";
    if (!model)    model = "deepseek-chat";
    if (!cwd) { if (!getcwd(real_cwd, sizeof real_cwd)) strcpy(real_cwd, "."); cwd = real_cwd; }
    /* Same file across turns and across the cache re-exec. Do not delete it. */
    if (agent_transcript_path(journal, sizeof journal) != 0)
        snprintf(journal, sizeof journal, "/tmp/csih-agent-%d.jsonl", (int)getpid());
    transcript = journal;

    /* A send follows the new line. History scroll stays put until then. */
    st->log_skip = 0;
    /* Capture the prompt before clearing the input line. */
    memset(prompt, 0, sizeof prompt);
    strncpy(prompt, st->input, sizeof prompt - 1);

    /* A window list only when the user named a window. Otherwise it becomes
     * the last user turn and the model answers tmux instead of the question. */
    extra[0] = '\0';
    if (agent_mentions_window(prompt)) {
        shell_result sr = shell_run_in(
            "tmux list-windows -F '#{window_index} #{window_name}' 2>/dev/null "
            "| grep -v \"^$(tmux display-message -p '#{window_index}') \"",
            NULL);
        if (sr.ok && sr.out[0]) {
            snprintf(extra, sizeof extra,
                "下面是窗口清单，只有用户要求操作窗口时才用。\n%s",
                sr.out);
        }
    }

    /* Record the prompt, then clear the input line so the next frame shows it. */
    {
        char you[240];
        snprintf(you, sizeof you, "you> %s", prompt);
        tui_log_plain(st, you);
    }
    st->input[0] = '\0'; st->ninput = 0;
    st->errline[0] = '\0';   /* new you>: drop last round's error */
    tui_redraw(st);

    st->cancel = 0;
    st->busy = 1;
    st->busy_tick = st->ticks;
    tui_redraw(st);
    if (agent_turn_begin(prompt, transcript, endpoint, model, cwd, extra, tui_agent_event, st) != 0) {
        r = agent_turn_take();
        st->busy = 0;
        {
            char fail[240];
            snprintf(fail, sizeof fail, "✗ agent failed: %s", r.reason);
            tui_log_plain(st, fail);
        }
        tui_redraw(st);
        return 1;
    }
    return 0;
}

static void tui_turn_done(tui_state *st) {
    agent_result r = agent_turn_take();
    int cancelled = st->cancel || r.err == -5;
    st->cancel = 0;
    st->busy = 0;
    if (cancelled) {
        tui_log_plain(st, "已取消");
        tui_redraw(st);
        return;
    }
    if (!r.ok) {
        {
            char fail[240];
            snprintf(fail, sizeof fail, "✗ agent failed: %s", r.reason);
            tui_log_plain(st, fail);
        }
    } else {
        snprintf(st->last_answer, sizeof st->last_answer, "%s",
                 r.answer[0] ? r.answer : "(no answer text)");
        {
            char sum[240];
            snprintf(sum, sizeof sum, "↻ rounds=%d actions=%d stopped=%s",
                     r.rounds, r.actions, r.stopped ? "yes" : "no");
            tui_log_plain(st, sum);
        }
    }
    tui_redraw(st);
    if (!cancelled && r.ok && st->loop_on) tui_queue_goal(st);
}

int main(int argc, char **argv) {
    const char *cmd = argc > 1 ? argv[1] : "";

    if (!strcmp(cmd, "selftest")) {
        /* Driven by an injected key sequence, so this runs with no terminal. */
        tui_state st;
        r_frame prev, next;
        char out[4096];
        int failures = 0, cols = 40;

        tui_state_init(&st, NULL);   /* selftest drives keys, not a transcript */
        prev = tui_render_state(&st, cols);

        /* Typing two characters changes exactly the lines that show them. */
        tui_apply_key(&st, TERM_KEY_CHAR, 'h');
        tui_apply_key(&st, TERM_KEY_CHAR, 'i');
        next = tui_render_state(&st, cols);
        if (!strcmp(st.input, "hi")) { /* ok */ } else { printf("FAIL input\n"); failures++; }
        if (frame_diff(&prev, &next, out, sizeof out) < 1) {
            printf("FAIL typing must change the frame\n"); failures++;
        }

        /* An unchanged state must rewrite NOTHING. This is the whole point of
         * the diff, and it is what keeps the loop from flickering. */
        prev = next;
        if (frame_diff(&prev, &next, out, sizeof out) != 0) {
            printf("FAIL an unchanged frame rewrote lines\n"); failures++;
        }

        /* Backspace must be visible, and must not underflow. */
        tui_apply_key(&st, TERM_KEY_BACKSPACE, 0);
        tui_apply_key(&st, TERM_KEY_BACKSPACE, 0);
        tui_apply_key(&st, TERM_KEY_BACKSPACE, 0);   /* one too many */
        if (st.ninput != 0 || st.input[0] != '\0') {
            printf("FAIL backspace underflow\n"); failures++;
        }
        tui_input_byte(&st, (char)0xE4);
        tui_input_byte(&st, (char)0xBD);
        tui_input_byte(&st, (char)0xA0);
        tui_input_byte(&st, 'a');
        tui_apply_key(&st, TERM_KEY_BACKSPACE, 0);
        if (st.ninput != 3 || strcmp(st.input, "你") != 0) {
            printf("FAIL backspace split a code point\n"); failures++;
        } else {
            tui_apply_key(&st, TERM_KEY_BACKSPACE, 0);
            if (st.ninput != 0 || st.input[0] != '\0') {
                printf("FAIL backspace left a partial character\n"); failures++;
            } else printf("  ok   backspace deletes one code point\n");
        }
        {
            tui_state b;
            r_frame bf;
            int k, saw = 0;
            tui_state_init(&b, NULL);
            b.mode = 1;
            b.rows = 54;
            b.busy = 1;
            bf = tui_render_state(&b, 40);
            for (k = 0; k < bf.n; k++)
                if (bf.lines[k] && strstr(bf.lines[k], "已收")) saw = 1;
            if (!saw || tui_frame_bad_width(&bf, 40) || bf.n > b.rows) {
                printf("FAIL busy line hides received bytes\n"); failures++;
            } else printf("  ok   busy line shows received bytes\n");
        }

        /* A paste is one buffer. Newlines inside it are not Enter. */
        {
            tui_state pz;
            r_frame pf;
            int k, saw = 0;
            tui_state_init(&pz, NULL);
            pz.mode = 1;
            pz.rows = 54;
            pz.pasting = 1;
            tui_apply_key(&pz, TERM_KEY_CHAR, 'a');
            tui_apply_key(&pz, TERM_KEY_ENTER, 0);
            tui_apply_key(&pz, TERM_KEY_CHAR, 'b');
            if (strcmp(pz.input, "a\nb") || pz.busy || pz.quit) {
                printf("FAIL paste newline submitted\n"); failures++;
            }
            pz.pasting = 0;
            pz.more_keys = 1;
            tui_apply_key(&pz, TERM_KEY_ENTER, 0);
            if (strcmp(pz.input, "a\nb\n") || pz.busy) {
                printf("FAIL burst newline submitted\n"); failures++;
            } else printf("  ok   paste keeps newlines\n");
            pf = tui_render_state(&pz, 40);
            for (k = 0; k < pf.n; k++)
                if (strstr(pf.lines[k], "⏎")) saw = 1;
            if (!saw || tui_frame_bad_width(&pf, 40)) {
                printf("FAIL paste row width\n"); failures++;
            } else printf("  ok   paste row shows return\n");
            {
                tui_state lz;
                r_frame lf;
                int broke = 0, mark = 0;
                tui_state_init(&lz, NULL);
                lz.mode = 1;
                lz.rows = 54;
                tui_log_plain(&lz, "you> a\nb");
                snprintf(lz.pending[0], TUI_INPUT_MAX, "%s", "c\nd");
                lz.npending = 1;
                snprintf(lz.goal, TUI_INPUT_MAX, "%s", "g\nh");
                lf = tui_render_state(&lz, 40);
                for (k = 0; k < lf.n; k++) {
                    if (strchr(lf.lines[k], '\n') || strchr(lf.lines[k], '\r')) broke = 1;
                    if (strstr(lf.lines[k], "you> a⏎b")) mark |= 1;
                    if (strstr(lf.lines[k], "待发送> c⏎d")) mark |= 2;
                    if (strstr(lf.lines[k], "goal> g⏎h · loop> off")) mark |= 4;
                }
                if (broke || mark != 7 || tui_frame_bad_width(&lf, 40) || lf.n > lz.rows) {
                    printf("FAIL log newline split the frame\n"); failures++;
                } else printf("  ok   log newline is one row\n");
            }
        }

        /* Ctrl-C clears a line. On an empty line it only names Ctrl-D.
         * Ctrl-D on an empty line quits. /exit and /quit quit. */
        tui_apply_key(&st, TERM_KEY_UNKNOWN, 0);
        if (st.quit) { printf("FAIL unknown key quit the loop\n"); failures++; }
        tui_apply_key(&st, TERM_KEY_CHAR, 'x');
        tui_apply_key(&st, TERM_KEY_CTRL_C, 0);
        if (st.quit || st.ninput != 0 || st.input[0] != '\0') {
            printf("FAIL ctrl-c did not clear the line\n"); failures++;
        }
        tui_apply_key(&st, TERM_KEY_CTRL_C, 0);
        if (st.quit || !tui_frame_has(&st, "要退出请按 Ctrl-D")) {
            printf("FAIL empty ctrl-c did not ask for ctrl-d\n"); failures++;
        }
        tui_apply_key(&st, TERM_KEY_CHAR, 'z');
        tui_apply_key(&st, TERM_KEY_CTRL_D, 0);
        if (st.quit || st.ninput != 1) {
            printf("FAIL ctrl-d quit while the line had text\n"); failures++;
        }
        tui_apply_key(&st, TERM_KEY_CTRL_C, 0);
        tui_apply_key(&st, TERM_KEY_CTRL_D, 0);
        if (!st.quit) { printf("FAIL empty ctrl-d did not quit\n"); failures++; }
        {
            tui_state qx;
            tui_state_init(&qx, NULL);
            tui_type(&qx, "/exit");
            if (!qx.quit) { printf("FAIL /exit did not quit\n"); failures++; }
            tui_state_init(&qx, NULL);
            tui_type(&qx, "/quit");
            if (!qx.quit) { printf("FAIL /quit did not quit\n"); failures++; }
        }

        /* A timeout is not a quit and does change the tick line. */
        {
            tui_state t2;
            tui_state_init(&t2, NULL);
            tui_apply_key(&t2, -1, 0);
            if (t2.quit) { printf("FAIL timeout quit\n"); failures++; }
            if (t2.ticks != 1) { printf("FAIL timeout did not tick\n"); failures++; }
        }

        /* Every rendered line covers `cols` terminal columns. CJK makes the
         * byte length longer than the column count. */
        {
            r_frame f = tui_render_state(&st, cols);
            int i;
            for (i = 0; i < f.n; i++) {
                if (tui_disp_width(f.lines[i]) != cols) {
                    printf("FAIL line %d not padded to width\n", i);
                    failures++;
                }
            }
        }

        {
            tui_state q;
            term_key_t k;
            int n = 0;
            memset(&q, 0, sizeof q);
            memset(&k, 0, sizeof k);
            k.kind = TERM_KEY_CHAR;
            k.ch = 'a';
            while (tui_q_room(&q) > 0) { tui_q_push(&q, k); n++; }
            if (n != 63) { printf("FAIL queue room %d\n", n); failures++; }
            tui_q_push(&q, k);
            if (tui_q_room(&q) != 0) { printf("FAIL full queue accepted more\n"); failures++; }
        }
        {
            tui_state g;
            int posts = 0, before, you = 0, i;
            tui_state_init(&g, NULL);
            g.mode = 1;
            tui_type(&g, "/goal");
            if (g.busy || g.goal[0] || !tui_frame_has(&g, "goal>")) {
                printf("FAIL bare /goal\n"); failures++;
            } else printf("  ok   /goal shows empty\n");
            tui_type(&g, "/loop");
            if (g.busy || g.loop_on) {
                printf("FAIL /loop without a goal\n"); failures++;
            } else printf("  ok   /loop without goal stays off\n");
            tui_type(&g, "/goal keep-going");
            if (g.busy || strcmp(g.goal, "keep-going") != 0 || !tui_frame_has(&g, "keep-going")) {
                printf("FAIL /goal text\n"); failures++;
            } else printf("  ok   /goal sets keep-going\n");
            tui_type(&g, "/loop");
            if (g.busy || !g.loop_on || !tui_frame_has(&g, "loop> on")
                || g.npending != 1 || !strstr(g.pending[0], "keep-going")) {
                printf("FAIL /loop on\n"); failures++;
            } else printf("  ok   /loop on queues the goal\n");
            tui_type(&g, "/loop extra");
            if (g.busy || g.loop_on || g.npending != 0) {
                printf("FAIL /loop off\n"); failures++;
            } else printf("  ok   /loop off drops the queue\n");
            before = g.npending;
            agent_turn_seal(1, 1, 1, 0, 0, "done");
            tui_turn_done(&g);
            if (g.npending != before) {
                printf("FAIL loop off still submitted\n"); failures++;
            } else printf("  ok   loop off does not submit\n");
            tui_type(&g, "/loop");
            if (!g.loop_on) { printf("FAIL /loop did not rearm\n"); failures++; }
            for (;;) {
                before = g.npending;
                agent_turn_seal(1, 1, 1, 0, 0, "done");
                tui_turn_done(&g);
                if (g.npending == before + 1 && strstr(g.pending[g.npending - 1], g.goal))
                    posts++;
                else break;
            }
            before = g.npending;
            agent_turn_seal(1, 1, 1, 0, 0, "done");
            tui_turn_done(&g);
            if (posts < 2 || g.npending != before) {
                printf("FAIL loop posts %d pending %d\n", posts, g.npending); failures++;
            } else printf("  ok   loop posts %d\n", posts);
            for (i = 0; i < g.nlog; i++)
                if (!strncmp(g.log[i], "you>", 4)) you++;
            if (you != 0 || g.busy) {
                printf("FAIL slash entered a model turn\n"); failures++;
            } else printf("  ok   slash did not start a model turn\n");
            tui_type(&g, "/goal ");
            before = g.npending;
            agent_turn_seal(1, 1, 1, 0, 0, "done");
            tui_turn_done(&g);
            if (g.goal[0] || g.loop_on || g.npending != before || !tui_frame_has(&g, "loop> off")) {
                printf("FAIL empty /goal left the loop armed\n"); failures++;
            } else printf("  ok   empty /goal disarms loop\n");
            tui_type(&g, "/goal keep-going");
            agent_turn_seal(1, 1, 1, 0, 0, "done");
            tui_turn_done(&g);
            if (g.loop_on || g.npending != before) {
                printf("FAIL new goal resumed the loop\n"); failures++;
            } else printf("  ok   new goal stays idle until /loop\n");
            {
                int nlog, fold, keep;
                tui_log_plain(&g, "keep-me");
                keep = g.nlog - 1;
                g.npending = 1;
                snprintf(g.pending[0], TUI_INPUT_MAX, "%s", "queued");
                g.loop_on = 1;
                g.ex_open[keep] = 1;
                nlog = g.nlog;
                fold = g.ex_open[keep];
                g.busy = 1;
                tui_type(&g, "/reload");
                if (g.quit || !g.busy || g.input[0] || g.nlog != nlog + 1
                    || strcmp(g.log[keep], "keep-me")
                    || !tui_frame_has(&g, "reload: 请求中，没重载")
                    || g.npending != 1 || strcmp(g.pending[0], "queued")
                    || strcmp(g.goal, "keep-going") || !g.loop_on
                    || g.ex_open[keep] != fold) {
                    printf("FAIL busy /reload\n"); failures++;
                } else printf("  ok   busy /reload stays in process\n");
                g.busy = 0;
                nlog = g.nlog;
                tui_type(&g, "/reload");
                if (g.quit || g.busy || g.input[0] || g.nlog != nlog + 1
                    || strcmp(g.log[keep], "keep-me")
                    || !tui_frame_has(&g, "reload: 页每帧已重读，密钥缓存已清")
                    || strcmp(g.goal, "keep-going") || g.npending != 1) {
                    printf("FAIL idle /reload\n"); failures++;
                } else printf("  ok   idle /reload clears the key cache\n");
            }
            {
                tui_state tall;
                r_frame fit;
                int k;
                tui_state_init(&tall, NULL);
                tall.mode = 1;
                tall.rows = 24;
                for (k = 0; k < 40; k++) tui_log_line(&tall, "log-line");
                fit = tui_render_state(&tall, 40);
                if (fit.n > tall.rows) {
                    printf("FAIL agent frame %d rows over %d\n", fit.n, tall.rows); failures++;
                } else printf("  ok   agent frame fits %d\n", fit.n);
            }
            {
                tui_state wide;
                r_frame fr;
                int k, goal_at = -1;
                tui_state_init(&wide, NULL);
                wide.mode = 1;
                wide.rows = 54;
                tui_log_line(&wide, "only-one");
                fr = tui_render_state(&wide, 40);
                for (k = 0; k < fr.n; k++)
                    if (!strncmp(fr.lines[k], "goal>", 5)) { goal_at = k; break; }
                if (goal_at != 1 + TUI_LOG_VIEW || !strstr(fr.lines[1], "only-one")
                    || strncmp(fr.lines[0], "csih · agent", 12) != 0
                    || fr.n > wide.rows || tui_frame_bad_width(&fr, 40)) {
                    printf("FAIL log view want %d got goal at %d\n",
                           1 + TUI_LOG_VIEW, goal_at);
                    failures++;
                } else printf("  ok   log view is %d\n", TUI_LOG_VIEW);
                {
                    int h, saw = 0;
                    char hist[16];
                    for (h = 0; h < 40; h++) {
                        snprintf(hist, sizeof hist, "hist-%02d", h);
                        tui_log_line(&wide, hist);
                    }
                    fr = tui_render_state(&wide, 40);
                    if (tui_log_y0 < 2 || tui_log_y1 <= tui_log_y0
                        || tui_frame_has(&wide, "hist-00")
                        || !tui_frame_has(&wide, "hist-39")
                        || tui_frame_bad_width(&fr, 40)) {
                        printf("FAIL log tail hist y %d..%d\n", tui_log_y0, tui_log_y1);
                        failures++;
                    } else {
                        tui_wheel(&wide, tui_log_y0, 2);
                        fr = tui_render_state(&wide, 40);
                        if (wide.log_skip != 3 || !tui_frame_has(&wide, "hist-17")
                            || tui_frame_has(&wide, "hist-39")
                            || !tui_frame_has(&wide, "更早3")
                            || tui_frame_bad_width(&fr, 40)) {
                            printf("FAIL log wheel skip %d\n", wide.log_skip);
                            failures++;
                        } else {
                            tui_log_line(&wide, "hist-new");
                            if (wide.log_skip != 4 || tui_frame_has(&wide, "hist-new")
                                || !tui_frame_has(&wide, "hist-17")) {
                                printf("FAIL log hold skip %d\n", wide.log_skip);
                                failures++;
                            } else {
                                tui_wheel(&wide, 1, 2);
                                tui_apply_key(&wide, TERM_KEY_END, 0);
                                fr = tui_render_state(&wide, 80);
                                saw = 0;
                                for (k = 0; k < fr.n; k++)
                                    if (strstr(fr.lines[k], "PgUp/PgDn")) saw = 1;
                                if (wide.log_skip != 0 || !saw || !tui_frame_has(&wide, "hist-new")
                                    || tui_frame_bad_width(&fr, 80)) {
                                    printf("FAIL log end or wide hint\n");
                                    failures++;
                                } else {
                                    tui_apply_key(&wide, TERM_KEY_UP, 0);
                                    tui_apply_key(&wide, TERM_KEY_PGUP, 0);
                                    tui_apply_key(&wide, TERM_KEY_HOME, 0);
                                    if (wide.log_skip != 20 || !tui_frame_has(&wide, "hist-01")
                                        || tui_frame_has(&wide, "hist-new")) {
                                        printf("FAIL log home skip %d\n", wide.log_skip);
                                        failures++;
                                    } else {
                                        tui_apply_key(&wide, TERM_KEY_PGDN, 0);
                                        tui_apply_key(&wide, TERM_KEY_DOWN, 0);
                                        if (wide.log_skip != 0) {
                                            printf("FAIL log page down skip %d\n", wide.log_skip);
                                            failures++;
                                        } else printf("  ok   log wheel and keys\n");
                                    }
                                }
                            }
                        }
                    }
                }
                tui_state_init(&wide, NULL);
                wide.mode = 1;
                wide.rows = 54;
                fr = tui_render_state(&wide, 40);
                goal_at = -1;
                for (k = 0; k < fr.n; k++)
                    if (!strncmp(fr.lines[k], "goal>", 5)) { goal_at = k; break; }
                if (goal_at != 1 + TUI_LOG_VIEW || tui_frame_has(&wide, "type a prompt")) {
                    printf("FAIL empty log view goal at %d\n", goal_at);
                    failures++;
                } else printf("  ok   empty log view is %d\n", TUI_LOG_VIEW);
            }
            {
                tui_state ex;
                r_frame fr;
                int k, y = 0, why = 0, cmd = 0, closed = 0;
                tui_state_init(&ex, NULL);
                ex.mode = 1;
                ex.rows = 24;
                tui_log_event(&ex, "exec\t跑一下自测\tcmd-ok");
                tui_log_event(&ex, "  │ BODYMARK-hidden");
                tui_log_event(&ex, "fold\tfile\tread\tjson.c");
                tui_log_event(&ex, "  │ FILEBODY-hidden");
                fr = tui_render_state(&ex, 40);
                for (k = 0; k < fr.n; k++) {
                    if (strstr(fr.lines[k], "exec[展开]>")) {
                        why = 1;
                        y = k + 1;
                    }
                    if (strstr(fr.lines[k], "BODYMARK-hidden")) cmd = 1;
                    if (strstr(fr.lines[k], "FILEBODY-hidden")) cmd = 1;
                }
                if (!why || cmd || y < 1 || !tui_frame_has(&ex, "跑一下自测")
                    || !tui_frame_has(&ex, "cmd-ok")
                    || !tui_frame_has(&ex, "file[展开]>")
                    || !tui_frame_has(&ex, "read · json.c")) {
                    printf("FAIL tool row stays closed\n"); failures++;
                } else {
                    tui_click(&ex, 1, y);
                    fr = tui_render_state(&ex, 40);
                    cmd = 0;
                    for (k = 0; k < fr.n; k++) {
                        if (strstr(fr.lines[k], "exec[收缩]>")) closed = 1;
                        if (strstr(fr.lines[k], "BODYMARK-hidden")) cmd = 1;
                        if (strstr(fr.lines[k], "FILEBODY-hidden")) cmd = 1;
                    }
                    if (!closed || !cmd || tui_frame_has(&ex, "FILEBODY-hidden")) {
                        printf("FAIL tool click did not expand\n");
                        failures++;
                    } else printf("  ok   tool row opens on click\n");
                }
            }
            {
                tui_state er;
                tui_state_init(&er, NULL);
                er.mode = 1;
                er.rows = 24;
                if (!tui_frame_has(&er, "错误> -")) {
                    printf("FAIL empty error status\n"); failures++;
                }
                tui_log_event(&er, "exit=1 nfs: not responding");
                if (!tui_frame_has(&er, "错误> ") || !tui_frame_has(&er, "not responding")) {
                    printf("FAIL error status did not take the nfs line\n"); failures++;
                } else printf("  ok   error status keeps the nfs line\n");
            }
            {
                tui_state mind;
                r_frame mf;
                char oldhome[512];
                const char *home = getenv("HOME");
                int k, in_at = -1, hint_at = -1, sysn = 0, rules = 0, head = 0, paired = 0, tee = 0, labeled = 0, bars = 0, collapsed = 0;
                int mkdir(const char *path, unsigned mode);
                FILE *fp;
                if (home) snprintf(oldhome, sizeof oldhome, "%s", home);
                else oldhome[0] = '\0';
                mkdir("/tmp/cdsh-frame-home", 0750);
                mkdir("/tmp/cdsh-frame-home/.cdsh", 0750);
                fp = fopen("/tmp/cdsh-frame-home/.cdsh/思维树.md", "w");
                if (fp) { fputs("# 思维树\n甲\n乙\n丙\n", fp); fclose(fp); }
                fp = fopen("/tmp/cdsh-frame-home/.cdsh/记忆宫殿.md", "w");
                if (fp) { fputs("```mermaid\nflowchart LR\n# 记忆宫殿\n子\n丑\n```\n", fp); fclose(fp); }
                setenv("HOME", "/tmp/cdsh-frame-home", 1);
                tui_state_init(&mind, NULL);
                mind.mode = 1;
                mind.rows = 24;
                tui_log_line(&mind, "above");
                mf = tui_render_state(&mind, 40);
                for (k = 0; k < mf.n; k++) {
                    if (in_at < 0 && !strncmp(mf.lines[k], "> ", 2)) in_at = k;
                    if (in_at >= 0 && hint_at < 0 && k > in_at
                        && strstr(mf.lines[k], "Enter 发送")) hint_at = k;
                    {
                        const char *rs = mf.lines[k];
                        while (*rs == ' ') rs++;
                        /* A rule starts with ─, or is the labeled system edge.
                         * A tree glyph ├── is not one. */
                        if (hint_at >= 0 && head == 0 && k > hint_at
                            && strstr(mf.lines[k], "-<系统提示词>")) {
                            rules++;
                            labeled = 1;
                            if (strstr(mf.lines[k], "[展开]")) collapsed = 1;
                        } else if (hint_at >= 0 && head == 0 && k > hint_at
                            && (!strncmp(rs, "─", 3) || strstr(mf.lines[k], "┬")))
                            rules++;
                    }
                    /* Collapsed: the label is the whole system block. */
                    if (labeled && rules == 1 && head == 0 && k > hint_at
                        && !strstr(mf.lines[k], "-<系统提示词>"))
                        sysn++;
                    if (sysn == 0 && collapsed && rules >= 2 && k > hint_at
                        && strstr(mf.lines[k], "-<思维树>")
                        && strstr(mf.lines[k], "┬")
                        && strstr(mf.lines[k], "-<记忆宫殿>")) head = 1;
                    if (strstr(mf.lines[k], "乙") && strstr(mf.lines[k], "│")
                        && strstr(mf.lines[k], "丑")) paired = 1;
                    if (strstr(mf.lines[k], "┬")) tee = 1;
                    if (strstr(mf.lines[k], "│")) bars++;
                }
                if (in_at < 0 || hint_at < in_at || sysn != 0 || !collapsed || rules < 2 || !head
                    || !paired || !tee || bars != TUI_MIND_N || mf.n > mind.rows
                    || tui_frame_bad_width(&mf, 40)
                    || tui_frame_has(&mind, "```") || tui_frame_has(&mind, "flowchart LR")) {
                    printf("FAIL mind columns under input\n"); failures++;
                } else printf("  ok   rules between hint, system, and mind\n");
                {
                    int sy = 0, body = 0, phase = 0, by = 0;
                    char first[R_LINE_MAX + 1], next[R_LINE_MAX + 1];
                    mind.rows = 54;
                    mf = tui_render_state(&mind, 40);
                    for (k = 0; k < mf.n; k++) {
                        if (strstr(mf.lines[k], "-<系统提示词>") && strstr(mf.lines[k], "[展开]"))
                            sy = k + 1;
                    }
                    if (sy < 1) {
                        printf("FAIL system rule missing\n"); failures++;
                    } else {
                        tui_click(&mind, 1, sy);
                        mf = tui_render_state(&mind, 40);
                        first[0] = '\0';
                        for (k = 0; k < mf.n; k++) {
                            if (phase == 0 && strstr(mf.lines[k], "[收缩]")) {
                                phase = 1;
                                by = k + 2;
                            } else if (phase == 1 && strstr(mf.lines[k], "-<思维树>")) {
                                phase = 2;
                                break;
                            } else if (phase == 1) {
                                if (body == 0) snprintf(first, sizeof first, "%s", mf.lines[k]);
                                body++;
                            }
                        }
                        if (phase != 2 || body != TUI_SYS_N || mf.n > mind.rows
                            || tui_frame_bad_width(&mf, 40) || !strstr(first, "三件")) {
                            printf("FAIL system open body %d\n", body); failures++;
                        } else {
                            tui_wheel(&mind, by, 1);
                            mf = tui_render_state(&mind, 40);
                            next[0] = '\0';
                            phase = 0;
                            for (k = 0; k < mf.n; k++) {
                                if (phase == 0 && strstr(mf.lines[k], "[收缩]")) phase = 1;
                                else if (phase == 1) {
                                    snprintf(next, sizeof next, "%s", mf.lines[k]);
                                    break;
                                }
                            }
                            if (!next[0] || !strcmp(first, next) || tui_frame_bad_width(&mf, 40)) {
                                printf("FAIL system wheel did not scroll\n"); failures++;
                            } else {
                                char stuck[R_LINE_MAX + 1], back[R_LINE_MAX + 1];
                                int w;
                                for (w = 0; w < 40; w++) tui_wheel(&mind, by, 1);
                                mf = tui_render_state(&mind, 40);
                                stuck[0] = '\0';
                                phase = 0;
                                for (k = 0; k < mf.n; k++) {
                                    if (phase == 0 && strstr(mf.lines[k], "[收缩]")) phase = 1;
                                    else if (phase == 1) {
                                        snprintf(stuck, sizeof stuck, "%s", mf.lines[k]);
                                        break;
                                    }
                                }
                                tui_wheel(&mind, by, 2);
                                mf = tui_render_state(&mind, 40);
                                back[0] = '\0';
                                phase = 0;
                                for (k = 0; k < mf.n; k++) {
                                    if (phase == 0 && strstr(mf.lines[k], "[收缩]")) phase = 1;
                                    else if (phase == 1) {
                                        snprintf(back, sizeof back, "%s", mf.lines[k]);
                                        break;
                                    }
                                }
                                if (!stuck[0] || !back[0] || !strcmp(stuck, back)
                                    || tui_frame_bad_width(&mf, 40)) {
                                    printf("FAIL system wheel stuck past the end\n");
                                    failures++;
                                } else printf("  ok   system opens ten rows and scrolls\n");
                            }
                        }
                    }
                    mind.sys_open = 0;
                    mind.sys_top = 0;
                    mind.rows = 24;
                }
                fp = fopen("/tmp/cdsh-frame-home/.cdsh/记忆宫殿.md", "w");
                if (fp) fclose(fp);
                if (!tui_frame_has(&mind, "(空)")) {
                    printf("FAIL empty palace page\n"); failures++;
                } else printf("  ok   empty palace shows (空)\n");
                remove("/tmp/cdsh-frame-home/.cdsh/记忆宫殿.md");
                if (!tui_frame_has(&mind, "(无)")) {
                    printf("FAIL missing palace page\n"); failures++;
                } else printf("  ok   missing palace shows (无)\n");
                if (oldhome[0]) setenv("HOME", oldhome, 1);
                else unsetenv("HOME");
            }
        }

        /* Mode 0 Enter is the CLI hand. The same line through tool_run (what
         * tools_cli run prints) must be the line the frame shows. No socket. */
        {
            tui_state hand;
            tool_result via_cli;
            char line[1024];
            size_t i;
            const char *path = "/tmp/cdsh-tui-cli.txt";
            tui_state_init(&hand, NULL);
            tui_type(&hand, "sh echo cdsh-hand");
            via_cli = tool_run("sh echo cdsh-hand");
            i = 0;
            while (via_cli.out[i] && via_cli.out[i] != '\n' && i + 1 < sizeof line) {
                line[i] = via_cli.out[i];
                i++;
            }
            line[i] = '\0';
            if (hand.mode != 0 || hand.busy || hand.input[0] || !via_cli.ok
                || strcmp(hand.output, "cdsh-hand") != 0
                || strcmp(hand.output, line) != 0
                || !hand.notice
                || strcmp(hand.notice, "ran (output continues; see 'read')") != 0
                || !tui_frame_has(&hand, "cdsh-hand")
                || !tui_frame_has(&hand, "ran (output continues")) {
                printf("FAIL cli echo did not land on the tui frame\n");
                failures++;
            } else printf("  ok   cli echo matches tui frame\n");
            tui_type(&hand, "write /tmp/cdsh-tui-cli.txt from-cli");
            tui_type(&hand, "read /tmp/cdsh-tui-cli.txt");
            via_cli = tool_run("read /tmp/cdsh-tui-cli.txt");
            i = 0;
            while (via_cli.out[i] && via_cli.out[i] != '\n' && i + 1 < sizeof line) {
                line[i] = via_cli.out[i];
                i++;
            }
            line[i] = '\0';
            if (!via_cli.ok || strcmp(hand.output, "from-cli") != 0
                || strcmp(hand.output, line) != 0
                || !tui_frame_has(&hand, "from-cli")) {
                printf("FAIL cli read did not land on the tui frame\n");
                failures++;
            } else printf("  ok   cli read matches tui frame\n");
            remove(path);
        }

        printf("%s\n", failures ? "SELFTEST FAILED" : "selftest ok");
        return failures == 0 ? 0 : 1;
    }

    if (!strcmp(cmd, "once")) {
        /* Render a single frame to stdout and exit — usable from a pipe, which
         * is how a probe checks the frame without a terminal. */
        tui_state st;
        r_frame f;
        int i, cols = 40;
        tui_state_init(&st, NULL);
        f = tui_render_state(&st, cols);
        for (i = 0; i < f.n; i++) printf("%s|\n", f.lines[i]);
        return 0;
    }

    if (!strcmp(cmd, "run") || !strcmp(cmd, "agent")) {
        /* The real loop. Needs a terminal; refuses politely without one rather
         * than emitting escape sequences into a pipe. `agent` is the same loop
         * in agent mode (Enter drives the DeepSeek loop instead of the gate). */
        tui_state st;
        int cols, first = 1;
        int agent_mode = !strcmp(cmd, "agent");

        if (!isatty(0) || !isatty(1)) {
            printf("tui %s needs a terminal (stdin and stdout must be ttys)\n", cmd);
            return 64;
        }
        /* `tui run [spill] [transcript]` / `tui agent [spill] [transcript]`.
         * spill writes long tool results under ~/.csih/tool. Default is off.
         * tui_state_init clears the struct, so the measured size is applied
         * after it. Doing it before left rows at the 24-line default. */
        {
            int ai;
            const char *transcript = NULL;
            agent_set_spill(0);
            for (ai = 2; ai < argc; ai++) {
                if (!strcmp(argv[ai], "spill")) agent_set_spill(1);
                else if (!transcript) transcript = argv[ai];
            }
            tui_state_init(&st, transcript);
        }
        st.mode = agent_mode ? 1 : 0;
        {
            term_size_t sz = term_size();
            cols = sz.cols;
            st.rows = sz.rows;
            /* render pads to at most R_MAX_COLS. A wider pane used to fail
             * the width check and draw nothing, so the window stayed blank. */
            if (cols > R_MAX_COLS) cols = R_MAX_COLS;
            if (st.rows > R_MAX_LINES) st.rows = R_MAX_LINES;
            if (st.rows < 1) st.rows = 24;
            st.cols = cols;
        }
        r_frame_init(&tui_prev);
        term_install_exit_handler(term_on_signal);
        if (term_raw_enter() != 0) { printf("cannot enter raw mode\n"); return 1; }

        while (!st.quit && !term_interrupted()) {
            if (st.busy && !agent_turn_step(0)) tui_turn_done(&st);
            if (!st.busy && st.npending > 0 && st.mode == 1) {
                tui_dequeue(&st);
                tui_run_agent(&st);
            }
            /* Drain everything that has arrived into the queue FIRST, then
             * consume exactly one key per redraw. Consuming the read directly
             * is what lost 14 of 15 characters: one read returns a whole typed
             * line, and one key per tick threw the rest away. */
            if (tui_q_empty(&st)) {
                int ready = term_wait_readable(TUI_FRAME_MS);
                if (ready < 0) break;             /* terminal gone */
                if (ready == 1) {
                    /* Read in BULK and split into keys. Calling term_read_key()
                     * once per frame was the data-loss bug: one read returns a
                     * whole typed line, and term_parse_key() given fifteen
                     * bytes returns a single UNKNOWN — fourteen characters
                     * gone. See term_read_keys' header. */
                    term_key_t batch[64];
                    int room = tui_q_room(&st);
                    int nk = term_read_keys(batch, room < 64 ? room : 64);
                    int j;
                    for (j = 0; j < nk; j++) tui_q_push(&st, batch[j]);
                } else {
                    tui_apply_key(&st, -1, 0);
                    tui_repaint = 1;
                }
            }
            while (!tui_q_empty(&st)) {
                term_key_t k = tui_q_pop(&st);
                st.more_keys = !tui_q_empty(&st);
                if (k.kind == TERM_KEY_MOUSE) tui_click(&st, k.x, k.y);
                else if (k.kind == TERM_KEY_WHEEL)
                    tui_wheel(&st, k.y, (int)(unsigned char)k.ch);
                else tui_apply_key(&st, (int)k.kind, k.ch);
            }
            st.more_keys = 0;
            {
                term_size_t sz = term_size();
                if (sz.source) {
                    int c = sz.cols, r = sz.rows;
                    if (c > R_MAX_COLS) c = R_MAX_COLS;
                    if (r > R_MAX_LINES) r = R_MAX_LINES;
                    if (c != st.cols || r != st.rows) {
                        st.cols = c;
                        st.rows = r;
                        tui_repaint = 1;
                    }
                }
            }
            /* Redraw against the shared previous frame (tui_redraw also serves
             * the live agent events, so the two stay consistent). */
            tui_redraw(&st);
            if (first) { tui_write_all("\x1b[?25l", 6); first = 0; }
        }
        term_raw_leave();
        tui_write_all("\x1b[?25h\x1b[0m\n", 9);   /* show cursor, reset, newline */
        return 0;
    }

    printf("usage: tui selftest | once | run | agent\n");
    return 64;
}
