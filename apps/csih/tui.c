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
#include "csih_cols_pub.h"
#include "reload_session.h"
#include "reload_io.h"
#include "reload_owner.h"
#include "csih_message_io.h"
#include <fcntl.h>
#include <poll.h>

int journal_checkpoint(const char *path, long long *offset, char *why, size_t cap);

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
#include "agent_result.h"
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
int agent_role_configure(const char *role, const char *peer);
void agent_turn_seal(int ok, int stopped, int rounds, int actions, int err,
                     const char *answer);

/* shell.c's dispatcher, restated so the TUI can enumerate tmux windows. */
typedef struct { int ok; int err; int exited; int status; int signal; long bytes; char out[65536]; } shell_result;
shell_result shell_run_in(const char *command, const char *cwd);
void net_set_tick(void (*fn)(void));
void net_cancel(void);
void net_progress(int *sec, int *tokens, long *sent, int *rsec);
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
#define TUI_LOG_VIEW  40  /* agent log rows; a shorter terminal uses fewer */

typedef struct {
    char input[TUI_INPUT_MAX];   /* what the user has typed */
    int  ninput;
    char pending[8][TUI_INPUT_MAX]; /* lines typed while a call is in flight */
    int  npending;
    char history[16][TUI_INPUT_MAX]; /* accepted manual lines, oldest first */
    int  nhistory;
    int  history_pos;              /* browsing cursor; == nhistory means tail */
    int  history_browsing;         /* 1 while Up/Down is walking history */
    char history_draft[TUI_INPUT_MAX]; /* draft saved on the first Up */
    int  quit;                   /* leave the program */
    int  ask_exit;               /* empty Ctrl-C: say Ctrl-D quits, do not quit */
    int  pasting;                /* inside ESC [ 200 ~ ... ESC [ 201 ~ */
    int  more_keys;              /* 1 when this key is not the last of the burst */
    int  cancel;                 /* stop this turn and return to the input */
    int  busy;                   /* 1 while a model call is in flight */
    int  busy_tick;              /* ticks when busy became 1 */
    char phase[64];              /* what the in-flight request is doing now */
    int  ended;                  /* 1 when phase holds a stop cause, not a live call */
    int  ticks;                  /* loop iterations; not printed in agent mode */
    const char *notice;          /* one-line feedback, e.g. an unknown key */
    const char *transcript;      /* where turns are recorded; NULL = no chat */
    char journal_path[4096];     /* owned path after inactive snapshot apply */
    reload_session_state *owned; /* explicit startup context, heap owned by main */
    char owned_dir[4096];
    int owner_fd;
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
    int  mind_open;           /* 0: mind rule only. 1: folded mind rows */
    int  log_skip;            /* newest log entries hidden; 0 follows the tail */
    char mail_id[33];         /* in-flight message id, empty when idle */
    int  mail_active;         /* 1 while a take()ed message is being handled */
    int  mail_blocked;        /* 1 when take() yielded nothing usable */
    long mail_next_ms;        /* next poll deadline, from clock_now_ms */
    char mail_error[256];     /* last take/finish failure reason */
} tui_state;

long clock_now_ms(void);
int session_append(const char *path, const char *line);
size_t json_rec(char *buf, size_t cap, const char *key1, const char *val1,
                const char *key2, const char *val2,
                const char *key3, const char *val3);

static int tui_mail_meta(tui_state *st, const char *id, const char *result, const char *reason);
static void tui_mail_complete(tui_state *st, const char *result, const char *reason);
static void tui_mail_poll(tui_state *st, int tasks_allowed);

static void tui_state_init(tui_state *st, const char *transcript) {
    memset(st, 0, sizeof *st);
    st->transcript = transcript;
    st->owner_fd = -1;
    st->mode = 0;
    st->cols = 40;
    st->rows = 24;
    st->notice = 0;
    (void)transcript;
}

/* These helpers prepare an inactive restore only; they do not transfer
 * startup context, journal ownership, or a running request. */
static int tui_reload_error(char *why, size_t cap, const char *message) {
    if (why && cap) snprintf(why, cap, "%s", message);
    return -1;
}

static int tui_reload_valid(const reload_session_state *snapshot,
                            char *why, size_t cap) {
    char *scratch = malloc(1048576);
    size_t len = 0;
    int rc;
    if (!scratch) return tui_reload_error(why, cap, "reload allocation failed");
    rc = reload_session_encode_v2(snapshot, scratch, 1048576, &len, why, cap);
    free(scratch);
    return rc;
}

static int tui_reload_capture(const tui_state *st,
                              const reload_session_state *metadata,
                              reload_session_state *out, char *why, size_t cap) {
    size_t i;
    if (!out) return tui_reload_error(why, cap, "reload null output");
    if (!st || !metadata || out == metadata) {
        memset(out, 0, sizeof *out);
        return tui_reload_error(why, cap, "reload null or aliased input");
    }
    memset(out, 0, sizeof *out);
    if (st->busy) return tui_reload_error(why, cap, "reload busy");
    if (st->npending < 0 || st->npending > 8 || st->nhistory < 0 || st->nhistory > 16 ||
        st->history_pos < 0 || st->history_pos > st->nhistory)
        return tui_reload_error(why, cap, "reload counts out of range");
    memcpy(out->session_id, metadata->session_id, sizeof out->session_id);
    memcpy(out->handoff_id, metadata->handoff_id, sizeof out->handoff_id);
    memcpy(out->candidate_hash, metadata->candidate_hash, sizeof out->candidate_hash);
    memcpy(out->cwd, metadata->cwd, sizeof out->cwd);
    memcpy(out->role, metadata->role, sizeof out->role);
    memcpy(out->peer, metadata->peer, sizeof out->peer);
    memcpy(out->journal_path, metadata->journal_path, sizeof out->journal_path);
    out->journal_offset = metadata->journal_offset;
    memcpy(out->goal, st->goal, sizeof out->goal);
    memcpy(out->input, st->input, sizeof out->input);
    memcpy(out->history_draft, st->history_draft, sizeof out->history_draft);
    out->npending = (size_t)st->npending;
    out->nhistory = (size_t)st->nhistory;
    out->history_pos = (size_t)st->history_pos;
    out->history_browsing = st->history_browsing;
    out->loop_on = st->loop_on;
    out->loop_left = st->loop_left;
    for (i = 0; i < out->npending; i++)
        memcpy(out->pending[i], st->pending[i], sizeof out->pending[i]);
    for (i = 0; i < out->nhistory; i++)
        memcpy(out->history[i], st->history[i], sizeof out->history[i]);
    if (tui_reload_valid(out, why, cap) != 0) {
        memset(out, 0, sizeof *out);
        return -1;
    }
    return 0;
}

static int tui_reload_apply(tui_state *st, const reload_session_state *snapshot,
                            char *why, size_t cap) {
    size_t i;
    if (!st || !snapshot) return tui_reload_error(why, cap, "reload null input");
    if (st->busy) return tui_reload_error(why, cap, "reload busy");
    if (tui_reload_valid(snapshot, why, cap) != 0) return -1;
    memcpy(st->goal, snapshot->goal, sizeof st->goal);
    memcpy(st->input, snapshot->input, sizeof st->input);
    st->ninput = (int)strlen(st->input);
    memset(st->pending, 0, sizeof st->pending);
    memset(st->history, 0, sizeof st->history);
    st->npending = (int)snapshot->npending;
    st->nhistory = (int)snapshot->nhistory;
    for (i = 0; i < snapshot->npending; i++)
        memcpy(st->pending[i], snapshot->pending[i], sizeof st->pending[i]);
    for (i = 0; i < snapshot->nhistory; i++)
        memcpy(st->history[i], snapshot->history[i], sizeof st->history[i]);
    memcpy(st->history_draft, snapshot->history_draft, sizeof st->history_draft);
    st->history_pos = (int)snapshot->history_pos;
    st->history_browsing = snapshot->history_browsing;
    st->loop_on = snapshot->loop_on;
    st->loop_left = snapshot->loop_left;
    memcpy(st->journal_path, snapshot->journal_path, sizeof st->journal_path);
    st->transcript = st->journal_path;
    return 0;
}

/* Export/prepare assume idle single-writer ownership and trusted ancestors.
 * prepare intentionally uses checkpoint: it fsyncs the journal while checking
 * its current exact length, without reading/replaying or consuming anything. */
static int tui_reload_export(const tui_state *st,
                             const reload_session_state *metadata,
                             const char *path, char *why, size_t cap) {
    reload_session_state *work = NULL, *snapshot = NULL;
    char *encoded = NULL;
    size_t len = 0;
    long long offset = 0;
    int rc = -1;
    if (!st || !metadata || !path)
        return tui_reload_error(why, cap, "reload null input");
    if (st->busy) return tui_reload_error(why, cap, "reload busy");
    if (!memchr(metadata->journal_path, 0, sizeof metadata->journal_path))
        return tui_reload_error(why, cap, "journal path not terminated");
    work = malloc(sizeof *work);
    snapshot = malloc(sizeof *snapshot);
    encoded = malloc(131073);
    if (!work || !snapshot || !encoded) {
        tui_reload_error(why, cap, "reload allocation failed");
        goto done;
    }
    memcpy(work, metadata, sizeof *work);
    if (journal_checkpoint(work->journal_path, &offset, why, cap) != 0) goto done;
    work->journal_offset = (unsigned long long)offset;
    if (tui_reload_capture(st, work, snapshot, why, cap) != 0) goto done;
    if (reload_session_encode_v2(snapshot, encoded, 131073, &len, why, cap) != 0) goto done;
    rc = reload_io_save(path, encoded, len, why, cap);
    if (rc == -2)
        tui_reload_error(why, cap, "state replaced; durability not confirmed");
 done:
    free(encoded);
    free(snapshot);
    free(work);
    return rc;
}

static int tui_reload_prepare_bound(const char *path, const char *expected_session,
                              const char *expected_handoff, const char *expected_hash,
                              const char *expected_journal,
                              reload_session_state *out, reload_io_token *token,
                              char *why, size_t cap) {
    char *text = NULL;
    size_t len = 0;
    long long offset = 0;
    int rc = -1;
    if (out) memset(out, 0, sizeof *out);
    if (token) memset(token, 0, sizeof *token);
    if (!path || !expected_session || !expected_handoff || !expected_hash || !out || !token)
        return tui_reload_error(why, cap, "reload null input");
    text = malloc(131073);
    if (!text) return tui_reload_error(why, cap, "reload allocation failed");
    if (reload_io_load(path, expected_session, expected_handoff, expected_hash,
                       text, 131073, &len, token, why, cap) != 0) goto done;
    if (reload_session_decode_v2(text, len, out, why, cap) != 0) goto done;
    if (expected_journal && strcmp(out->journal_path, expected_journal)) {
        tui_reload_error(why, cap, "snapshot journal not bound to owned directory");
        goto done;
    }
    if (journal_checkpoint(out->journal_path, &offset, why, cap) != 0) goto done;
    if ((unsigned long long)offset != out->journal_offset) {
        tui_reload_error(why, cap, "journal length != snapshot offset");
        goto done;
    }
    rc = 0;
 done:
    free(text);
    if (rc != 0) {
        memset(out, 0, sizeof *out);
        memset(token, 0, sizeof *token);
    }
    return rc;
}

static int tui_reload_prepare(const char *path, const char *session,
                              const char *handoff, const char *hash,
                              reload_session_state *out, reload_io_token *token,
                              char *why, size_t cap) {
    return tui_reload_prepare_bound(path, session, handoff, hash, NULL, out, token, why, cap);
}

/* Runtime capacities are those of the real agent, not the wider codec. */
static int tui_owned_id(const char *s, int empty) {
    size_t i;
    if (!s || (!empty && !s[0]) || strlen(s) > 4095) return 0;
    for (i = 0; s[i]; i++) {
        unsigned char c = (unsigned char)s[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || strchr("_-.:", c))) return 0;
    }
    return 1;
}

static int tui_owned_runtime(const reload_session_state *m, char *why, size_t cap) {
    size_t i;
    if (strlen(m->journal_path) >= 512 || strlen(m->cwd) >= 1024 ||
        m->cwd[0] != '/' || strlen(m->peer) > 48 || strlen(m->role) >= 16)
        return tui_reload_error(why, cap, "owned runtime path/peer capacity exceeded");
    for (i = 0; m->peer[i]; i++) {
        unsigned char c = (unsigned char)m->peer[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || strchr(":_-", c)))
            return tui_reload_error(why, cap, "owned runtime peer invalid");
    }
    return 0;
}

static int tui_owned_start(tui_state *st, const char *dir, const char *session,
                           const char *hash, const char *state_path, const char *handoff,
                           reload_io_token *token, char *why, size_t cap) {
    char lock[4096], journal[4096];
    const char *v;
    size_t i;
    int fd, dfd;
    struct stat sb;
    reload_session_state *m;
    if (!dir || dir[0] != '/' || strlen(dir) > 4000 || dir[strlen(dir)-1] == '/' ||
        !tui_owned_id(session, 0) || !hash || strlen(hash) != 64 ||
        (state_path && !tui_owned_id(handoff, 0)))
        return tui_reload_error(why, cap, "owned arguments invalid");
    for (i = 0; i < 64; i++)
        if (!strchr("0123456789abcdefABCDEF", hash[i]))
            return tui_reload_error(why, cap, "owned hash must be 64 hex (explicit binding only)");
    snprintf(lock, sizeof lock, "%s/owner.lock", dir);
    snprintf(journal, sizeof journal, "%s/journal.jsonl", dir);
    if (strlen(journal) >= 512)
        return tui_reload_error(why, cap, "owned journal exceeds agent capacity");
    if (reload_owner_acquire(lock, &st->owner_fd, why, cap) != 0) return -1;
    m = malloc(sizeof *m);
    if (!m) return tui_reload_error(why, cap, "owned context allocation failed");
    memset(m, 0, sizeof *m); st->owned = m;
    snprintf(st->owned_dir, sizeof st->owned_dir, "%s", dir);
    if (state_path) {
        if (tui_reload_prepare_bound(state_path, session, handoff, hash, journal, m, token, why, cap) != 0) return -1;
        if (strcmp(m->journal_path, journal))
            return tui_reload_error(why, cap, "snapshot journal not bound to owned directory");
    } else {
        snprintf(m->session_id, sizeof m->session_id, "%s", session);
        snprintf(m->candidate_hash, sizeof m->candidate_hash, "%s", hash);
        snprintf(m->handoff_id, sizeof m->handoff_id, "initial");
        snprintf(m->journal_path, sizeof m->journal_path, "%s", journal);
        v = getenv("CSIH_CWD");
        if (v && v[0]) {
            if (strlen(v) >= 1024) return tui_reload_error(why, cap, "owned cwd exceeds agent capacity");
            snprintf(m->cwd, sizeof m->cwd, "%s", v);
        } else if (!getcwd(m->cwd, sizeof m->cwd))
            return tui_reload_error(why, cap, "owned getcwd failed");
        v = getenv("CSIH_ROLE");
        if (!v || !v[0]) v = "agent";
        if (!tui_owned_id(v, 0)) return tui_reload_error(why, cap, "owned role invalid");
        snprintf(m->role, sizeof m->role, "%s", v);
        v = getenv("CSIH_PEER");
        if (!v) v = "";
        if (!tui_owned_id(v, 1)) return tui_reload_error(why, cap, "owned peer invalid");
        snprintf(m->peer, sizeof m->peer, "%s", v);
    }
    if (tui_owned_runtime(m, why, cap) != 0) return -1;
    dfd = open(m->cwd, O_RDONLY | O_DIRECTORY);
    if (dfd < 0) return tui_reload_error(why, cap, "owned cwd not enterable directory");
    if (close(dfd) != 0 || chdir(m->cwd) != 0)
        return tui_reload_error(why, cap, "owned cwd activation failed");
    if (!state_path) {
        fd = open(journal, O_RDWR | O_CREAT | O_NOFOLLOW | O_CLOEXEC, 0600);
        if (fd < 0) return tui_reload_error(why, cap, "owned journal open failed");
        if (fstat(fd, &sb) != 0 || !S_ISREG(sb.st_mode) || sb.st_uid != getuid() ||
            (sb.st_mode & 0777) != 0600) {
            close(fd); return tui_reload_error(why, cap, "owned journal not trusted regular file");
        }
        if (close(fd) != 0) return tui_reload_error(why, cap, "owned journal close failed");
    }
    if (tui_reload_apply(st, m, why, cap) != 0) return -1;
    if (setenv("CSIH_CWD", m->cwd, 1) != 0 || setenv("CSIH_ROLE", m->role, 1) != 0 ||
        setenv("CSIH_PEER", m->peer, 1) != 0)
        return tui_reload_error(why, cap, "owned startup environment failed");
    if (agent_role_configure(m->role, m->peer) != 0)
        return tui_reload_error(why, cap, "owned agent role configuration failed");
    return 0;
}

static int tui_owned_close(tui_state *st, char *why, size_t cap) {
    int rc = 0;
    if (st->owner_fd >= 0) rc = reload_owner_release(&st->owner_fd, why, cap);
    free(st->owned); st->owned = NULL;
    return rc;
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
static void tui_history_add(tui_state *st, const char *text) {
    if (!text || !text[0]) return;
    if (st->nhistory > 0 && !strcmp(st->history[st->nhistory - 1], text)) {
        st->history_pos = st->nhistory;
        st->history_browsing = 0;
        return;
    }
    if (st->nhistory >= 16) {
        memmove(st->history[0], st->history[1], (size_t)(16 - 1) * TUI_INPUT_MAX);
        st->nhistory = 16 - 1;
    }
    snprintf(st->history[st->nhistory], TUI_INPUT_MAX, "%s", text);
    st->nhistory++;
    st->history_pos = st->nhistory;
    st->history_browsing = 0;
}

static void tui_history_move(tui_state *st, int delta) {
    if (st->nhistory <= 0) return;
    if (!st->history_browsing) {
        if (delta >= 0) return;
        snprintf(st->history_draft, TUI_INPUT_MAX, "%s", st->input);
        st->history_browsing = 1;
        st->history_pos = st->nhistory;
    }
    {
        int pos = st->history_pos + delta;
        if (pos < 0) pos = 0;
        if (pos > st->nhistory) pos = st->nhistory;
        st->history_pos = pos;
    }
    if (st->history_pos >= st->nhistory) {
        snprintf(st->input, TUI_INPUT_MAX, "%s", st->history_draft);
        st->history_browsing = 0;
    } else {
        snprintf(st->input, TUI_INPUT_MAX, "%s", st->history[st->history_pos]);
    }
    st->ninput = (int)strlen(st->input);
}

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
    /* A quote of source or tool body (x.c:/x.h: lines) is never a turn
     * error, even when the quoted text happens to contain exit=1. */
    if (strstr(s, ".c:") || strstr(s, ".h:")) return;
    /* A later success in the same turn clears the stale error row. */
    if (!tui_looks_err(s) &&
        (strstr(s, "exit=0") || strstr(s, "selftest ok") || strstr(s, " ok "))) {
        st->errline[0] = '\0';
        return;
    }
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

/* The line a live turn actually runs. Only the user's goal. The write gate
 * stays in the system prompt, and is not repeated onto this task. */
static void tui_goal_prompt(const tui_state *st, char *out, int n) {
    snprintf(out, (size_t)n,
             "目标：%s。继续这一目标，做完就停。",
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
    if (!st->goal[0] || st->npending >= 8) return;
    if (st->loop_left <= 0) {
        st->loop_on = 0;
        return;
    }
    tui_goal_prompt(st, line, (int)sizeof line);
    snprintf(st->pending[st->npending], TUI_INPUT_MAX, "%s", line);
    st->npending++;
    st->loop_left--;
}

/* Slash commands never start a model turn. /loop on queues the goal so the
 * live loop can start it while idle. Returns 1 when the line was one. */
static int tui_managed_request(tui_state *st);

/* Recognize command tokens only, never absolute paths or multiline pastes. */
static int tui_command_len(const char *text) {
    int i = 1;
    if (!text || text[0] != '/' || strchr(text, '\n') || strchr(text, '\r')) return 0;
    if (!((text[i] >= 'A' && text[i] <= 'Z') || (text[i] >= 'a' && text[i] <= 'z'))) return 0;
    for (i = 2; text[i]; i++) {
        char c = text[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-')) break;
    }
    return text[i] == 0 || text[i] == ' ' ? i : 0;
}

static void tui_command_done(tui_state *st) {
    st->input[0] = 0;
    st->ninput = 0;
}

static int tui_slash(tui_state *st) {
    int token = tui_command_len(st->input);
    char note[240];
    if (!token) return 0;
    if (!strcmp(st->input, "/help")) {
        tui_log_plain(st, "/help: 本地帮助；//foo 发送字面 /foo");
        tui_log_plain(st, "/status: 只读状态；/clear: 清 UI，保留上下文、journal、pending、goal/loop");
        tui_log_plain(st, "/goal [text]: 设目标；裸 /goal 保留；/goal 后空格清目标");
        tui_log_plain(st, "/loop [text]: 切换目标循环，参数当前不解析");
        tui_log_plain(st, "/reload: 空闲重载密钥缓存；/reload-code: managed idle actor");
        tui_log_plain(st, "/export-state HANDOFF: owned idle 状态导出；/exit /quit: 退出");
        tui_command_done(st);
        return 1;
    }
    if (!strcmp(st->input, "/status")) {
        snprintf(note, sizeof note,
            "busy=%d mode=%s owned=%d owner=%d goal=%d loop=%d left=%d history=%d pending=%d keys=%d",
            st->busy, st->mode == 1 ? "agent" : "chat", st->owned != NULL,
            st->owner_fd >= 0, st->goal[0] != 0, st->loop_on, st->loop_left,
            st->nhistory, st->npending, (st->qtail - st->qhead + 64) % 64);
        tui_log_plain(st, note);
        tui_command_done(st);
        return 1;
    }
    if (!strcmp(st->input, "/clear")) {
        if (st->busy) tui_log_plain(st, "busy: not cleared");
        else {
            memset(st->log, 0, sizeof st->log);
            memset(st->ex_on, 0, sizeof st->ex_on);
            memset(st->ex_open, 0, sizeof st->ex_open);
            memset(st->ex_tag, 0, sizeof st->ex_tag);
            memset(st->ex_why, 0, sizeof st->ex_why);
            memset(st->ex_cmd, 0, sizeof st->ex_cmd);
            memset(st->ex_body, 0, sizeof st->ex_body);
            st->nlog = 0; st->log_skip = 0;
            memset(st->history, 0, sizeof st->history);
            memset(st->history_draft, 0, sizeof st->history_draft);
            st->nhistory = 0; st->history_pos = 0; st->history_browsing = 0;
            st->errline[0] = 0; st->last_answer[0] = 0;
            st->last[0] = 0; st->output[0] = 0; st->notice = NULL;
            st->phase[0] = 0; st->ended = 0;
            snprintf(note, sizeof note, "cleared UI view; model context and journal unchanged; pending=%d retained", st->npending);
            tui_log_plain(st, note);
        }
        tui_command_done(st);
        return 1;
    }
    if (st->input[token] == ' ' && ((!strncmp(st->input, "/help", 5) && token == 5) ||
        (!strncmp(st->input, "/status", 7) && token == 7) ||
        (!strncmp(st->input, "/clear", 6) && token == 6) ||
        (!strncmp(st->input, "/reload", 7) && token == 7) ||
        (!strncmp(st->input, "/reload-code", 12) && token == 12) ||
        (!strncmp(st->input, "/exit", 5) && token == 5) ||
        (!strncmp(st->input, "/quit", 5) && token == 5))) {
        snprintf(note, sizeof note, "usage: %.*s (no arguments)", token, st->input);
        tui_log_plain(st, note);
        tui_command_done(st);
        return 1;
    }
    if (!strcmp(st->input, "/reload-code")) {
        if (tui_managed_request(st) != 0) tui_log_plain(st, "reload-code: requires managed idle actor");
        return 1;
    }
    if (!strncmp(st->input, "/export-state", 13) &&
        (st->input[13] == 0 || st->input[13] == ' ')) {
        reload_session_state *m = NULL;
        tui_state *copy = NULL;
        char why[256], path[4096], note[4200];
        const char *handoff = st->input + 13;
        int rc = -1;
        while (*handoff == ' ') handoff++;
        if (!st->owned || st->owner_fd < 0 || st->busy || !tui_owned_id(handoff, 0)) {
            tui_log_plain(st, "export-state: requires owned idle state and explicit valid HANDOFF");
            return 1;
        }
        m = malloc(sizeof *m); copy = malloc(sizeof *copy);
        if (!m || !copy) snprintf(why, sizeof why, "allocation failed");
        else {
            memcpy(m, st->owned, sizeof *m);
            snprintf(m->handoff_id, sizeof m->handoff_id, "%s", handoff);
            memcpy(copy, st, sizeof *copy);
            copy->input[0] = 0; copy->ninput = 0;
            snprintf(path, sizeof path, "%s/state-v2.json", st->owned_dir);
            rc = tui_reload_export(copy, m, path, why, sizeof why);
        }
        if (rc == 0) {
            st->input[0] = 0; st->ninput = 0;
            snprintf(note, sizeof note, "state saved: %s (explicit HASH binding only)", path);
        } else snprintf(note, sizeof note, "export-state: %s", why);
        tui_log_plain(st, note);
        free(copy); free(m);
        return 1;
    }
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
    snprintf(note, sizeof note, "unknown command: %.*s (/help 查看)", token, st->input);
    tui_log_plain(st, note);
    tui_command_done(st);
    return 1;
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

/* Manual Enter path. Records the line into history only when tui_submit
 * actually accepted it (input was non-empty and got cleared). The queue-full
 * path keeps the input, so it is not recorded. */
static void tui_manual_submit(tui_state *st) {
    char captured[TUI_INPUT_MAX];
    snprintf(captured, TUI_INPUT_MAX, "%s", st->input);
    int had = st->input[0] != '\0';
    tui_submit(st);
    if (had && st->input[0] == '\0' &&
        !(st->mode == 1 && tui_command_len(captured)) &&
        strcmp(captured, "/exit") && strcmp(captured, "/quit"))
        tui_history_add(st, st->mode == 1 && captured[0] == '/' && captured[1] == '/' ? captured + 1 : captured);
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
        if (st->input[0] == '/' && st->input[1] == '/') {
            if (st->busy && st->npending >= 8) { st->notice = "待发送已满"; return; }
            memmove(st->input, st->input + 1, (size_t)st->ninput);
            st->ninput--;
        } else if (tui_slash(st)) return;
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
        if (st->input[0]) tui_manual_submit(st);
        return;
    case TERM_KEY_ENTER:
        /* A newline that still has keys behind it is part of a paste or a
         * burst, not a send. A lone Enter sends whatever is in the box. */
        if (st->pasting || st->more_keys) {
            tui_input_byte(st, '\n');
            return;
        }
        tui_manual_submit(st);
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
        if (st->pasting) return;
        tui_history_move(st, -1);
        return;
    case TERM_KEY_DOWN:
        if (st->pasting) return;
        tui_history_move(st, 1);
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

/* Rows under the mind rule when that drawer is open.
 * Fixed rows are title, goal, input, the status line, and the two rules.
 * System body, mind body, and pending lines are counted in tui_log_room. */
#define TUI_MIND_N 12
#define TUI_FIXED_ROWS 7

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
static int tui_mind_rule_y;

static int tui_text_lines(const char *src) {
    int n = 0, any = 0;
    if (!src) return 0;
    for (; *src; src++) {
        if (*src == '\n') {
            if (any) n++;
            any = 0;
        } else if (*src != ' ' && *src != '\t' && *src != '\r')
            any = 1;
    }
    if (any) n++;
    return n;
}

/* Content lines in a mind page. -1 when the file is missing. */
static int tui_page_count(const char *name) {
    char path[512], buf[512];
    FILE *fp;
    const char *home = getenv("HOME");
    int n = 0, any = 0;
    if (!home || !home[0]) return -1;
    csih_home_bind(home);
    snprintf(path, sizeof path, "%s/.csih/%s", home, name);
    fp = fopen(path, "r");
    if (!fp) {
        snprintf(path, sizeof path, "%s/.cdsh/%s", home, name);
        fp = fopen(path, "r");
    }
    if (!fp) return -1;
    while (fgets(buf, sizeof buf, fp)) {
        size_t len = strlen(buf);
        while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) buf[--len] = '\0';
        if (!buf[0] || tui_mind_noise(buf)) continue;
        any = 1;
        if (n < 999) n++;
    }
    fclose(fp);
    return any ? n : 0;
}

static void tui_rule_label(char *dst, int n, const char *name, int open, int count) {
    const char *mark = open ? "收缩" : "展开";
    /* Keep [展开] and [收缩] intact so a click can find the bracket. */
    if (count < 0)
        snprintf(dst, (size_t)n, "-<%s>[%s]> 无", name, mark);
    else if (count > 99)
        snprintf(dst, (size_t)n, "-<%s>[%s]> 99+", name, mark);
    else
        snprintf(dst, (size_t)n, "-<%s>[%s]> %d", name, mark, count);
}

/* Top edge of the system block. Collapsed is [展开], open is [收缩].
 * The number is how many source lines sit behind the rule. */
static void tui_sys_rule(r_state *rs, char line[][R_LINE_MAX + 1], int *n,
                         int cols, const tui_state *st) {
    char label[80];
    if (*n >= 63 || cols < 1) return;
    tui_rule_label(label, (int)sizeof label, "系统提示词", st && st->sys_open,
                   tui_text_lines(agent_model_rules()));
    tui_sys_rule_y = *n + 2;
    tui_label_rule(line[*n], R_LINE_MAX + 1, label, cols);
    rs->body[*n] = line[*n];
    (*n)++;
}

/* Top edge of the mind block. Titles sit on the rule, split by ┬. */
static void tui_mind_rule(r_state *rs, char line[][R_LINE_MAX + 1], int *n, int cols, const tui_state *st) {
    char left[R_LINE_MAX + 1], right[R_LINE_MAX + 1];
    int half, rest, bar;
    if (*n >= 63 || cols < 1) return;
    tui_mind_rule_y = *n + 2;
    bar = tui_mind_bar(cols, &half, &rest);
    tui_rule_label(left, (int)sizeof left, "思维树", st && st->mind_open, tui_page_count("思维树.md"));
    tui_rule_label(right, (int)sizeof right, "记忆宫殿", st && st->mind_open, tui_page_count("记忆宫殿.md"));
    if (bar < 0) {
        tui_label_rule(line[*n], R_LINE_MAX + 1, left, cols);
    } else {
        char lfit[R_LINE_MAX + 1], rfit[R_LINE_MAX + 1];
        tui_label_rule(lfit, (int)sizeof lfit, left, half);
        tui_label_rule(rfit, (int)sizeof rfit, right, rest);
        snprintf(line[*n], R_LINE_MAX + 1, "%s┬%s", lfit, rfit);
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
                /* Continuation of a wrapped note: indent four spaces so the
                 * next line reads as the same note, not a new one. */
                int k;
                for (k = 0; k < 4 && k < w - 1; k++) dst[out][k] = ' ';
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
    if (!st || !st->mind_open) return;
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
    if (y == tui_mind_rule_y && tui_mind_rule_y > 0) {
        st->mind_open = !st->mind_open;
        st->mind_top = 0;
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
    int mind_rows = st->mind_open ? TUI_MIND_N : 0;
    int room = rows - TUI_FIXED_ROWS - sys_rows - mind_rows - st->npending;
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

/* After a request finishes the busy line collapses to shortcuts; keep the
 * last phase and its whole-second cost so idle still shows what just ran. */
static const char *idle_last(tui_state *st) {
    static char buf[96];
    int sec = 0, tokens = 0, rsec = 0;
    long sent = 0;
    /* sec is this turn. rsec is only the last request, and must not replace it. */
    net_progress(&sec, &tokens, &sent, &rsec);
    if (!st->phase[0] || (sec <= 0 && !st->ended)) {
        buf[0] = '\0';
        return buf;
    }
    if (st->ended)
        snprintf(buf, sizeof buf, "上一轮 %s", st->phase);
    else
        snprintf(buf, sizeof buf, "上次 %s %d秒", st->phase, sec);
    return buf;
}

static const char *tui_role_title(const tui_state *st) {
    static char buf[64];
    const char *role = st->owned ? st->owned->role : getenv("CSIH_ROLE");
    const char *peer = st->owned ? st->owned->peer : getenv("CSIH_PEER");
    if (!role || !role[0]) return "csih · agent";
    (void)peer;
    if (!strcmp(role, "write"))
        snprintf(buf, sizeof buf, "csih · 写");
    else if (!strcmp(role, "watch"))
        snprintf(buf, sizeof buf, "csih · 看");
    else
        return "csih · agent";
    return buf;
}

static r_frame tui_render_state(tui_state *st, int cols) {
    r_state rs;
    r_frame f;
    /* One buffer per body line: render() copies each into the frame, so the
     * pointers only need to outlive the render() call. A single shared buffer
     * here would mean every body line aliases the last one. */
    char line[64][R_LINE_MAX + 1];
    int n = 0, i;
    char hint[R_LINE_MAX + 1];

    memset(&rs, 0, sizeof rs);
    tui_hit_n = 0;
    tui_sys_rule_y = 0;
    tui_sys_y0 = 0;
    tui_sys_y1 = 0;
    tui_log_y0 = 0;
    tui_log_y1 = 0;
    rs.title = (st->mode == 1) ? tui_role_title(st) : "csih";
    if (st->owned) {
        static char bound_title[160];
        snprintf(bound_title, sizeof bound_title, "%s · %.64s", rs.title, st->owned->candidate_hash);
        rs.title = bound_title;
    }
    rs.width = cols;
    /* Idle agent hint stays fixed. While a call is in flight, show whole
     * seconds only. Cache hit/miss appear only after a real usage object.
     * This line sits under the input and above the mind block. */
    if (st->mode == 1) {
        int sec = 0, tokens = 0, hit = 0, miss = 0, seen = 0;
        long sent = 0, got = 0;
        if (st->busy) {
            net_progress(&sec, &tokens, &sent, &sec);
            net_recv(&got);
            net_cache(&hit, &miss, &seen);
            if (seen)
                snprintf(hint, sizeof hint,
                         "请求中 %s %d秒 %d词元 上传 %ld字节 已收 %ld字节 hit %d miss %d",
                         st->phase, sec, tokens, sent, got, hit, miss);
            else
                snprintf(hint, sizeof hint,
                         "请求中 %s %d秒 %d词元 上传 %ld字节 已收 %ld字节",
                         st->phase,
                         sec, tokens, sent, got);
        } else if (st->ask_exit)
            snprintf(hint, sizeof hint, "要退出请按 Ctrl-D");
        else if (st->log_skip > 0)
            snprintf(hint, sizeof hint, "更早%d · End回底", st->log_skip);
        else {
            const char *prev = idle_last(st);
            const char *sep = prev[0] ? " · " : "";
            char raw[R_LINE_MAX + 1];
            int fit = cols > 0 ? cols : 1;
            if (fit > R_MAX_COLS) fit = R_MAX_COLS;
            snprintf(raw, sizeof raw, "%s%sEnter 发送", prev, sep);
            tui_utf8_fit(raw, fit, hint);
        }
    } else if (st->ask_exit)
        snprintf(hint, sizeof hint, "要退出请按 Ctrl-D · %d", st->ticks);
    else
        snprintf(hint, sizeof hint, "Ctrl-C 清空 · Ctrl-D 退出 · %d", st->ticks);
    if (st->errline[0]) {
        char with[R_LINE_MAX + 1];
        char fitted[R_LINE_MAX + 1];
        int fit = cols > 0 ? cols : 1;
        if (fit > R_MAX_COLS) fit = R_MAX_COLS;
        snprintf(with, sizeof with, "%s · 错误> %s", hint, st->errline);
        tui_utf8_fit(with, fit, fitted);
        snprintf(hint, sizeof hint, "%s", fitted);
    }

    if (st->mode == 1) {
        /* Log viewport is TUI_LOG_VIEW when the terminal allows it.
         * Fewer log lines are padded so the block does not collapse.
         * Mind and system drawers add their rows only while open. */
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
        (void)filled;
        (void)show;
        tui_log_y1 = n + 2;
        if (st->loop_on) {
            char gfold[R_LINE_MAX + 1];
            char suffix[32];
            int glen;
            snprintf(suffix, sizeof suffix, " · loop> %s", st->loop_on ? "on" : "off");
            glen = (int)sizeof line[n] - (int)strlen("goal> ") - (int)strlen(suffix);
            if (glen < 1) glen = 1;
            tui_put_folded_n(gfold, glen, "", st->goal, glen, 0);
            snprintf(line[n], sizeof line[n], "goal> %s%s", gfold, suffix);
            rs.body[n] = line[n]; n++;
        }
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
        tui_sys_rule(&rs, line, &n, cols, st);
        tui_sys_block(&rs, line, &n, cols, st);
        tui_mind_rule(&rs, line, &n, cols, st);
        tui_mind_block(&rs, line, &n, cols, st);
    } else {
        tui_input_row(line[n], (int)sizeof line[n], st->input, cols);
        rs.body[n] = line[n]; n++;
        snprintf(line[n], sizeof line[n], "%s", hint);
        rs.body[n] = line[n]; n++;
        tui_sys_rule(&rs, line, &n, cols, st);
        tui_sys_block(&rs, line, &n, cols, st);
        tui_mind_rule(&rs, line, &n, cols, st);
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
    /* A folded step carries the live tool and path, tab-separated:
     * fold<TAB><tool><TAB><sub><TAB><path>. Show it as the phase. Anything
     * else means the loop is back to waiting on the model. */
    if (!strncmp(line, "fold\t", 5)) {
        const char *a = line + 5;
        const char *b = strchr(a, '\t');
        const char *c = b ? strchr(b + 1, '\t') : NULL;
        char tool[32], sub[32], path[96];
        tool[0] = sub[0] = path[0] = '\0';
        if (b) {
            snprintf(tool, sizeof tool, "%.*s", (int)(b - a), a);
            if (c) {
                snprintf(sub, sizeof sub, "%.*s", (int)(c - (b + 1)), b + 1);
                snprintf(path, sizeof path, "%s", c + 1);
            }
        }
        if (path[0] && sub[0])
            snprintf(st->phase, sizeof st->phase, "%s %s %s", tool, sub, path);
        else if (path[0])
            snprintf(st->phase, sizeof st->phase, "%s %s", tool, path);
        else if (sub[0])
            snprintf(st->phase, sizeof st->phase, "%s %s", tool[0] ? tool : "工具", sub);
        else
            snprintf(st->phase, sizeof st->phase, "%s", tool[0] ? tool : "工具");
    } else if (st->busy) {
        snprintf(st->phase, sizeof st->phase, "等模型");
    }
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
    tui_mail_poll(st, 0);
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

#include "context_index.inc"
int agent_context_packet(const char *packet);

static int tui_run_prompt(tui_state *st, const char *source_prompt, int clear_manual_input) {
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
    if (st->owned) {
        cwd = st->owned->cwd;
        transcript = st->transcript;
    } else {
        if (agent_transcript_path(journal, sizeof journal) != 0)
            snprintf(journal, sizeof journal, "/tmp/csih-agent-%d.jsonl", (int)getpid());
        transcript = journal;
    }

    /* A send follows the new line. History scroll stays put until then. */
    st->log_skip = 0;
    /* Capture the prompt before clearing the input line. */
    memset(prompt, 0, sizeof prompt);
    strncpy(prompt, source_prompt, sizeof prompt - 1);

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
    if (clear_manual_input) { st->input[0] = '\0'; st->ninput = 0; }
    st->errline[0] = '\0';   /* new you>: drop last round's error */
    tui_redraw(st);

    st->cancel = 0;
    st->busy = 1;
    st->ended = 0;
    st->busy_tick = st->ticks;
    snprintf(st->phase, sizeof st->phase, "等模型");
    tui_redraw(st);
    {char *context=NULL;
        if(!tui_context_index(st,&context) || agent_context_packet(context)!=0){
            free(context);st->busy=0;tui_log_plain(st,"context-index unavailable: capture/encoding failed");tui_redraw(st);return 1;
        }
        free(context);
    }
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

static int tui_run_agent(tui_state *st) {
    return tui_run_prompt(st, st->input, 1);
}

/* Automatic prompts never borrow the manual editor. The real agent copies
 * the dedicated prompt before returning; later keys continue editing input. */
static int tui_run_pending(tui_state *st) {
    char prompt[TUI_INPUT_MAX];
    if (st->busy || st->npending <= 0) return 0;
    snprintf(prompt, sizeof prompt, "%s", st->pending[0]);
    st->npending--;
    memmove(st->pending[0], st->pending[1], (size_t)st->npending * TUI_INPUT_MAX);
    st->pending[st->npending][0] = 0;
    return tui_run_prompt(st, prompt, 0);
}

static void tui_turn_done(tui_state *st) {
    agent_result r = agent_turn_take();
    int cancelled = st->cancel || r.err == -5;
    int from_mail = st->mail_active;
    st->cancel = 0;
    st->busy = 0;
    /* Overwrite the in-flight phase with the stop cause so the idle bottom
     * bar writes how this turn ended, not a stale "等模型". */
    st->ended = 1;
    if (cancelled)
        snprintf(st->phase, sizeof st->phase, "已取消");
    else if (!r.ok)
        snprintf(st->phase, sizeof st->phase, "%s", r.outcome==AGENT_OUTCOME_PARTIAL ? "部分完成" : (r.outcome==AGENT_OUTCOME_UNVERIFIED || r.acceptance==AGENT_ACCEPT_UNVERIFIED) ? "未验证" : "出错");
    else if (r.answer[0])
        snprintf(st->phase, sizeof st->phase, "%s",r.scope==AGENT_SCOPE_ANSWER_ONLY ? "咨询已答" : "语义接受");
    else if (r.stopped)
        snprintf(st->phase, sizeof st->phase, "动作打满未答");
    else
        snprintf(st->phase, sizeof st->phase, "未答");
    if (from_mail)
        tui_mail_complete(st, cancelled ? "cancelled" : r.ok ? (r.scope==AGENT_SCOPE_ANSWER_ONLY ? "answered" : "ok") : r.outcome==AGENT_OUTCOME_PARTIAL ? "partial" : (r.outcome==AGENT_OUTCOME_UNVERIFIED || r.acceptance==AGENT_ACCEPT_UNVERIFIED) ? "unverified" : "failed", r.reason);
    if (cancelled) {
        tui_log_plain(st, "已取消");
        tui_redraw(st);
        return;
    }
    if (!r.ok) {
        if (r.answer[0]) {
            snprintf(st->last_answer, sizeof st->last_answer, "%s", r.answer);
            tui_log_plain(st, r.answer);
        }
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
            /* The answer text is already the ✓ row. This line is only the
             * round result, so a long answer is not cut mid-sentence. */
            if (r.answer[0])
                snprintf(sum, sizeof sum,
                         "↻ 本轮结束 rounds=%d actions=%d · 已答 · %s",
                         r.rounds, r.actions,
                         r.reason[0] ? r.reason : "已结束");
            else
                snprintf(sum, sizeof sum,
                         "↻ 本轮结束 rounds=%d actions=%d · 未作答，动作 %d 次 · %s",
                         r.rounds, r.actions, r.actions,
                         r.reason[0] ? r.reason : "已结束");
            tui_log_plain(st, sum);
        }
    }
    tui_redraw(st);
    if (!cancelled && r.ok && st->loop_on && !from_mail) tui_queue_goal(st);
}


/* One actor per process. The inherited private stream is a trusted host
 * capability, not an authentication protocol. No actor crosses exec. */
static struct {
    int fd, phase, raw, retired, used;
    char session[65], handoff[65], hash[65], dir[4096];
    char next_handoff[65], next_hash[65], frame[1024];
} tui_managed;

static int tui_managed_send(const char *op, int pending) {
    char line[512]; int n;
    n = snprintf(line, sizeof line,
        "{\"op\":\"%s\",\"session\":\"%s\",\"handoff\":\"%s\",\"hash\":\"%s\"}\n",
        op, tui_managed.session, pending ? tui_managed.next_handoff : tui_managed.handoff,
        pending ? tui_managed.next_hash : tui_managed.hash);
    return n > 0 && n < (int)sizeof line && write(tui_managed.fd, line, n) == n ? 0 : -1;
}

static int tui_managed_request(tui_state *st) {
    if (!tui_managed.phase || tui_managed.phase != 3 || st->busy ||
        !st->owned || st->owner_fd < 0) return -1;
    if (tui_managed_send("REQUEST", 0) != 0) { st->quit = 1; return -1; }
    st->input[0] = 0; st->ninput = 0;
    return 0;
}

static int tui_managed_init(const char *dir, const char *sid, const char *hid,
                            const char *hash, const char *fdtext, int standby) {
    struct stat sb; int dfd, fd = 0, flags, i;
    if (!dir || dir[0] != '/' || strlen(dir) > 4000 || dir[strlen(dir)-1] == '/' ||
        !tui_owned_id(sid, 0) || strlen(sid) > 64 || !tui_owned_id(hid, 0) ||
        strlen(hid) > 64 || !hash || strlen(hash) != 64 || !fdtext || !*fdtext) return -1;
    for (i = 0; i < 64; i++) if (!strchr("0123456789abcdefABCDEF", hash[i])) return -1;
    for (i = 0; fdtext[i]; i++) {
        if (fdtext[i] < '0' || fdtext[i] > '9' || fd > 100000) return -1;
        fd = fd * 10 + fdtext[i] - '0';
    }
    if (fd < 3 || fstat(fd, &sb) != 0 || !S_ISSOCK(sb.st_mode)) return -1;
    flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) != 0 ||
        fcntl(fd, F_SETFD, FD_CLOEXEC) != 0) return -1;
    dfd = open(dir, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (dfd < 0) return -1;
    i = fstat(dfd, &sb) == 0 && S_ISDIR(sb.st_mode) && sb.st_uid == getuid() && (sb.st_mode & 0777) == 0700;
    if (close(dfd) != 0 || !i) return -1;
    memset(&tui_managed, 0, sizeof tui_managed);
    tui_managed.fd = fd; tui_managed.phase = standby ? 1 : 2;
    snprintf(tui_managed.dir, sizeof tui_managed.dir, "%s", dir);
    snprintf(tui_managed.session, sizeof tui_managed.session, "%s", sid);
    snprintf(tui_managed.handoff, sizeof tui_managed.handoff, "%s", hid);
    snprintf(tui_managed.hash, sizeof tui_managed.hash, "%s", hash);
    return 0;
}

static int tui_managed_message(tui_state *st) {
    jvalue *root, *o, *sid, *hid, *hash;
    const char *op; char why[256], path[4096], lock[4096];
    int match, pending, rc = -1; size_t i;
    reload_session_state *m = NULL;
    reload_io_token token;
    root = NULL;
    if (!rs_no_embedded_nul(tui_managed.frame, tui_managed.used, why, sizeof why)) goto done;
    root = json_parse(tui_managed.frame, tui_managed.used, why, sizeof why);
    if (!root || root->kind != J_OBJ || root->nkeys != 4) goto done;
    for (i = 0; i < root->nkeys; i++) {
        size_t j;
        if (strcmp(root->keys[i], "op") && strcmp(root->keys[i], "session") &&
            strcmp(root->keys[i], "handoff") && strcmp(root->keys[i], "hash")) goto done;
        for (j = 0; j < i; j++) if (!strcmp(root->keys[i], root->keys[j])) goto done;
    }
    o = jget(root, "op"); sid = jget(root, "session"); hid = jget(root, "handoff"); hash = jget(root, "hash");
    if (!o || !sid || !hid || !hash || o->kind != J_STR || sid->kind != J_STR ||
        hid->kind != J_STR || hash->kind != J_STR || strcmp(sid->s, tui_managed.session)) goto done;
    op = o->s;
    match = !strcmp(hid->s, tui_managed.handoff) && !strcmp(hash->s, tui_managed.hash);
    pending = tui_managed.next_handoff[0] && !strcmp(hid->s, tui_managed.next_handoff) && !strcmp(hash->s, tui_managed.next_hash);
    if (!strcmp(op, "ACTIVATE") && match && tui_managed.phase == 2) {
        tui_managed.phase = 3; tui_repaint = 1; rc = 0;
    } else if (!strcmp(op, "COMMIT") && match && tui_managed.phase == 1) {
        snprintf(path, sizeof path, "%s/handoff-%s.json", tui_managed.dir, tui_managed.handoff);
        if (tui_owned_start(st, tui_managed.dir, tui_managed.session, tui_managed.hash,
            path, tui_managed.handoff, &token, why, sizeof why) != 0) goto fatal;
        if (term_raw_enter() != 0) goto fatal;
        tui_managed.raw = 1;
        if (reload_io_consume(path, &token, why, sizeof why) != 0) goto fatal;
        tui_managed.phase = 2; rc = tui_managed_send("ACK", 0);
    } else if (!strcmp(op, "FREEZE") && tui_managed.phase == 3 && !st->busy &&
               tui_owned_id(hid->s, 0) && strlen(hid->s) <= 64 && strlen(hash->s) == 64) {
        for (i = 0; i < 64; i++) if (!strchr("0123456789abcdefABCDEF", hash->s[i])) goto done;
        if (!strcmp(hid->s, tui_managed.handoff)) goto done;
        m = malloc(sizeof *m); if (!m) goto done;
        memcpy(m, st->owned, sizeof *m);
        snprintf(m->handoff_id, sizeof m->handoff_id, "%s", hid->s);
        snprintf(m->candidate_hash, sizeof m->candidate_hash, "%s", hash->s);
        snprintf(path, sizeof path, "%s/handoff-%s.json", tui_managed.dir, hid->s);
        if (tui_reload_export(st, m, path, why, sizeof why) != 0) goto done;
        snprintf(tui_managed.next_handoff, sizeof tui_managed.next_handoff, "%s", hid->s);
        snprintf(tui_managed.next_hash, sizeof tui_managed.next_hash, "%s", hash->s);
        tui_managed.phase = 4; rc = tui_managed_send("FROZEN", 1);
    } else if (!strcmp(op, "RELEASE") && pending && tui_managed.phase == 4) {
        if (reload_owner_release(&st->owner_fd, why, sizeof why) != 0) {
            tui_managed.phase = 6; rc = tui_managed_send("BLOCKED", 1);
        } else { tui_managed.phase = 5; rc = tui_managed_send("RELEASED", 1); }
    } else if (!strcmp(op, "RETIRE") && pending && tui_managed.phase == 5) {
        tui_managed.retired = 1; st->quit = 1; rc = 0;
    } else if (!strcmp(op, "RESUME") && pending && (tui_managed.phase == 4 || tui_managed.phase == 5)) {
        /* Trusted host must terminate+wait candidate before sending RESUME. */
        if (st->owner_fd < 0) {
            snprintf(lock, sizeof lock, "%s/owner.lock", tui_managed.dir);
            if (reload_owner_acquire(lock, &st->owner_fd, why, sizeof why) != 0) goto done;
        }
        if (term_raw_enter() != 0) goto fatal;
        tui_managed.raw = 1; rc = tui_managed_send("RESUMED", 1);
        if (!rc) { tui_managed.phase = 3; tui_managed.next_handoff[0] = 0; tui_managed.next_hash[0] = 0; tui_repaint = 1; }
    }
    goto done;
fatal:
    st->quit = 1;
done:
    free(m); if (root) jfree(root);
    if (rc != 0 && !st->quit && tui_managed_send("REJECT", 0) != 0) st->quit = 1;
    return rc;
}

static void tui_managed_poll(tui_state *st) {
    char c; int n;
    while (!st->quit) {
        n = read(tui_managed.fd, &c, 1);
        if (n == 0) { st->quit = 1; break; }
        if (n < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) st->quit = 1;
            break;
        }
        if (!c || tui_managed.used >= (int)sizeof tui_managed.frame - 1) { st->quit = 1; break; }
        if (c == '\n') {
            tui_managed.frame[tui_managed.used] = 0;
            tui_managed_message(st); tui_managed.used = 0;
        } else tui_managed.frame[tui_managed.used++] = c;
    }
}

/* Write one audit line for a message lifecycle transition. Only does work when
 * the owned state, transcript and owner fd are all valid. The record carries
 * exactly the three keys the mailbox expects — never role/text, and never any
 * notice body. Returns 1 when the line was appended, 0 otherwise. */
static int tui_mail_meta(tui_state *st, const char *id, const char *result, const char *reason) {
    char record[4096];
    if (!st || !st->owned || !st->transcript[0] || st->owner_fd < 0) return 0;
    if (!id || !result || !reason) return 0;
    if (json_rec(record, sizeof record,
                 "mail_id", id,
                 "result", result,
                 "reason", reason) == 0) return 0;
    if (session_append(st->transcript, record) != 0) return 0;
    return 1;
}

/* Close out the in-flight message. The audit line is written first; if that
 * fails the started state is left in place and the message is marked blocked,
 * with no finish() attempted. When the audit line lands, finish() moves
 * started->done; a non-zero rc also blocks and keeps the id so nothing is
 * replayed automatically. Task success is never claimed here — that is decided
 * by the caller's result. Either way mail_active clears and the id is dropped;
 * a blocked flag persists. */
static void tui_mail_complete(tui_state *st, const char *result, const char *reason) {
    char why[256];
    int rc;
    if (!st || !st->mail_active || !st->mail_id[0]) return;
    why[0] = 0;
    if (!tui_mail_meta(st, st->mail_id, result, reason)) {
        st->mail_blocked = 1;
        tui_log_plain(st, "result not recorded, started kept");
        st->mail_active = 0;
        st->mail_id[0] = 0;
        return;
    }
    rc = csih_message_finish(st->owned_dir, st->owned->session_id,
                             st->mail_id, why, sizeof why);
    if (rc != 0) {
        char note[4200];
        st->mail_blocked = 1;
        snprintf(note, sizeof note,
                 "finish rc=%d why=%s (not replayed automatically)", rc, why);
        tui_log_plain(st, note);
    } else {
        tui_log_plain(st, "message done");
    }
    st->mail_active = 0;
    st->mail_id[0] = 0;
}

/* Poll the private ACTIVE mailbox once. Only the unique ACTIVE owner (phase 3,
 * owned session, valid owner fd) may run this; it is a no-op otherwise. The job
 * of this helper is to hand a taken message into the normal prompt path — it
 * never invents a second execution path, and never ACKs a notice. Every return
 * frees the heap message. Throttled by mail_next_ms so a ready pole returns
 * soon without busy-spinning. */
static void tui_mail_poll(tui_state *st, int tasks_allowed) {
    char inbox[4096], why[256];
    struct stat sb;
    csih_message *m;
    long now;
    int rc;
    if (!st || tui_managed.phase != 3) return;
    if (!st->owned || st->owner_fd < 0 || !st->owned_dir[0]) return;
    now = clock_now_ms();
    if (now < 0) return;
    if (st->mail_next_ms && now < st->mail_next_ms) return;
    st->mail_next_ms = now + 250;
    if (snprintf(inbox, sizeof inbox, "%s/inbox", st->owned_dir) >= (int)sizeof inbox) return;
    if (lstat(inbox, &sb) != 0) {
        if (errno != ENOENT) {
            const char *e = "mailbox unavailable";
            if (strcmp(st->mail_error, e)) { snprintf(st->mail_error, sizeof st->mail_error, "%s", e); tui_log_plain(st, e); }
        }
        return;
    }
    m = malloc(sizeof *m);
    if (!m) return;
    if (st->busy || st->npending > 0 || st->mail_active || st->mail_blocked) tasks_allowed = 0;
    tasks_allowed = !!tasks_allowed;
    why[0] = 0;
    rc = csih_message_take(st->owned_dir, st->owned->session_id, tasks_allowed, m, why, sizeof why);
    if (rc == 0) { free(m); return; }
    if (rc < 0) {
        char note[512];
        snprintf(note, sizeof note, "take rc=%d why=%s", rc, why);
        if (strcmp(st->mail_error, note)) { snprintf(st->mail_error, sizeof st->mail_error, "%s", note); tui_log_plain(st, note); }
        if (rc == -2) st->mail_blocked = 1;
        free(m);
        return;
    }
    if (rc == 1) {
        /* A held task only runs when nothing else occupies the actor. */
        if (!tasks_allowed) {
            tui_log_plain(st, "task held: actor busy");
            st->mail_blocked = 1;
            free(m);
            return;
        }
        snprintf(st->mail_id, sizeof st->mail_id, "%s", m->id);
        st->mail_active = 1;
        if (!tui_mail_meta(st, st->mail_id, "started", "accepted")) {
            /* No audit line: leave started unconsumed and do not execute. */
            tui_log_plain(st, "task not run: audit line failed");
            st->mail_blocked = 1;
            st->mail_active = 0;
            st->mail_id[0] = 0;
            free(m);
            return;
        }
        if (tui_run_prompt(st, m->body, 0) != 0)
            tui_mail_complete(st, "failed", "begin failed");
        free(m);
        return;
    }
    if (rc == 2) {
        /* A notice is shown, never ACKed, never fed to the model. */
        tui_log_plain(st, m->body);
        why[0] = 0;
        if (csih_message_finish(st->owned_dir, st->owned->session_id, m->id, why, sizeof why) != 0) {
            char note[512];
            st->mail_blocked = 1;
            snprintf(note, sizeof note, "notice finish failed: %s", why);
            tui_log_plain(st, note);
        }
        free(m);
        return;
    }
    free(m);
}

int main(int argc, char **argv) {
    const char *cmd = argc > 1 ? argv[1] : "";

    if (!strcmp(cmd, "selftest")) {
        /* Driven by an injected key sequence, so this runs with no terminal. */
        static tui_state st;
        r_frame prev, next;
        char out[4096];
        int failures = 0, cols = 40;
        {
            static tui_state h, before;
            static const char *help_names[10] = {
                "/help", "/status", "/clear", "/goal", "/loop", "/reload",
                "/reload-code", "/export-state", "/exit", "/quit"
            };
            char journal[96], bytes[64];
            const char *audit = "audit unchanged\n";
            FILE *f;
            int i, j, found, n, bad = 0;
            snprintf(journal, sizeof journal, "csih-command-selftest-%ld.jsonl", (long)getpid());
            f = fopen(journal, "wb");
            if (!f) bad = 1;
            else {
                if (fwrite(audit, 1, strlen(audit), f) != strlen(audit)) bad = 1;
                if (fclose(f) != 0) bad = 1;
            }
            tui_state_init(&h, journal); h.mode = 1;
            tui_history_add(&h, "old history");
            snprintf(h.history_draft, sizeof h.history_draft, "old draft");
            h.history_browsing = 1; h.history_pos = 0;
            snprintf(h.goal, sizeof h.goal, "keep goal"); h.loop_on = 1; h.loop_left = 3;
            snprintf(h.pending[0], sizeof h.pending[0], "queued text"); h.npending = 1;
            h.qhead = 3; h.qtail = 5;
            memcpy(&before, &h, sizeof h);
            for (i = 0; i < 2; i++) {
                h.busy = i;
                tui_type(&h, "/help");
                for (j = 0; j < 10; j++) {
                    int k;
                    found = 0;
                    for (k = 0; k < h.nlog; k++) if (strstr(h.log[k], help_names[j])) found = 1;
                    if (!found) bad = 1;
                }
                tui_type(&h, "/status");
                snprintf(bytes, sizeof bytes, "busy=%d mode=agent owned=0 owner=0", i);
                if (!strstr(h.log[h.nlog - 1], bytes) ||
                    !strstr(h.log[h.nlog - 1], "goal=1 loop=1 left=3 history=1 pending=1 keys=2") ||
                    h.busy != i || h.nhistory != 1 || h.npending != 1 || h.input[0]) bad = 1;
            }
            h.busy = 1;
            tui_type(&h, "/clear");
            if (strcmp(h.log[h.nlog - 1], "busy: not cleared") ||
                h.nhistory != 1 || strcmp(h.history_draft, "old draft") ||
                h.history_pos != 0 || !h.history_browsing || !h.busy) bad = 1;
            h.busy = 0; h.ex_open[0] = 1; h.log_skip = 2;
            snprintf(h.phase, sizeof h.phase, "old phase"); h.ended = 1;
            tui_type(&h, "/clear");
            if (h.nlog != 1 || !strstr(h.log[0], "pending=1 retained") ||
                h.nhistory || h.history_pos || h.history_browsing || h.history_draft[0] ||
                h.log_skip || h.ex_open[0] || h.input[0] || h.ninput || h.busy || h.phase[0] || h.ended ||
                h.loop_left != before.loop_left || h.loop_on != before.loop_on ||
                strcmp(h.goal, before.goal) || h.npending != before.npending ||
                memcmp(h.pending, before.pending, sizeof h.pending) || h.transcript != before.transcript) bad = 1;
            tui_apply_key(&h, TERM_KEY_UP, 0); tui_apply_key(&h, TERM_KEY_DOWN, 0);
            if (h.input[0] || h.history_browsing) bad = 1;
            tui_type(&h, "/foobar");
            if (!strstr(h.log[h.nlog - 1], "unknown command: /foobar") || h.busy || h.nhistory || h.npending != 1) bad = 1;
            tui_type(&h, "/clear extra");
            if (!strstr(h.log[h.nlog - 1], "usage: /clear") || h.nlog != 3 || h.busy || h.nhistory) bad = 1;
            f = fopen(journal, "rb");
            if (!f) bad = 1;
            else {
                n = (int)fread(bytes, 1, sizeof bytes - 1, f); bytes[n] = 0;
                if (strcmp(bytes, audit)) bad = 1;
                fclose(f);
            }
            unlink(journal);
            if (bad) { printf("FAIL native help/status/clear invariants\n"); failures++; }
            else printf("  ok   native commands preserve journal, pending, loop and exclude history\n");
            tui_state_init(&h, NULL); h.mode = 1; h.busy = 1;
            tui_type(&h, "//foo");
            tui_type(&h, "/foo/bar");
            tui_type(&h, "/help\nordinary text");
            if (h.npending != 3 || strcmp(h.pending[0], "/foo") ||
                strcmp(h.pending[1], "/foo/bar") || strcmp(h.pending[2], "/help\nordinary text") ||
                h.nhistory != 3 || strcmp(h.history[0], "/foo") || h.nlog ||
                tui_command_len("/foo.bar") || tui_command_len("/9foo") ||
                tui_command_len("/help\rtext") || tui_command_len("/foo\ttext") ||
                !tui_command_len("/foo-bar_2 arg")) {
                printf("FAIL slash token boundaries and literal escape\n"); failures++;
            } else printf("  ok   slash tokens exclude paths/multiline; literal escape queued once\n");
            h.npending = 8;
            tui_type(&h, "//retry");
            if (strcmp(h.input, "//retry") || h.ninput != 7 || h.nhistory != 3) {
                printf("FAIL escaped full queue keeps draft\n"); failures++;
            }
            h.npending = 0;
            tui_manual_submit(&h);
            if (h.npending != 1 || strcmp(h.pending[0], "/retry") || h.nhistory != 4 || h.input[0]) {
                printf("FAIL escaped full queue retry\n"); failures++;
            }
        }
        {
            static tui_state source, fresh, saved;
            static reload_session_state metadata, snapshot, zero;
            char why[256];
            tui_state_init(&source, NULL);
            memset(&metadata, 0, sizeof metadata);
            snprintf(metadata.session_id, sizeof metadata.session_id, "sess");
            snprintf(metadata.handoff_id, sizeof metadata.handoff_id, "handoff");
            memset(metadata.candidate_hash, 'a', 64);
            snprintf(metadata.cwd, sizeof metadata.cwd, "/tmp");
            snprintf(metadata.role, sizeof metadata.role, "agent");
            snprintf(metadata.journal_path, sizeof metadata.journal_path, "/tmp/reload-journal.jsonl");
            metadata.journal_offset = 17;
            snprintf(source.goal, sizeof source.goal, "目标");
            snprintf(source.input, sizeof source.input, "输入");
            source.ninput = (int)strlen(source.input);
            snprintf(source.pending[0], sizeof source.pending[0], "待发送");
            source.npending = 1;
            snprintf(source.history[0], sizeof source.history[0], "历史");
            source.nhistory = 1; source.history_pos = 0; source.history_browsing = 1;
            snprintf(source.history_draft, sizeof source.history_draft, "中文草稿");
            source.loop_on = 1; source.loop_left = 0;
            memcpy(&saved, &source, sizeof saved);
            tui_state_init(&fresh, NULL);
            fresh.mode = 1; fresh.cols = 77; fresh.rows = 31;
            if (tui_reload_capture(&source, &metadata, &snapshot, why, sizeof why) != 0 ||
                memcmp(&source, &saved, sizeof source) ||
                tui_reload_apply(&fresh, &snapshot, why, sizeof why) != 0 ||
                strcmp(fresh.goal, source.goal) || strcmp(fresh.input, source.input) ||
                fresh.ninput != source.ninput || fresh.npending != 1 || fresh.nhistory != 1 ||
                strcmp(fresh.pending[0], source.pending[0]) || strcmp(fresh.history[0], source.history[0]) ||
                strcmp(fresh.history_draft, source.history_draft) || fresh.history_pos != 0 ||
                fresh.history_browsing != 1 || fresh.loop_on != 1 || fresh.loop_left != 0 ||
                fresh.transcript != fresh.journal_path || strcmp(fresh.journal_path, metadata.journal_path) ||
                fresh.mode != 1 || fresh.cols != 77 || fresh.rows != 31) {
                printf("FAIL reload capture/apply restore %s\n", why); failures++;
            }
            memset(snapshot.journal_path, 0, sizeof snapshot.journal_path);
            if (strcmp(fresh.transcript, metadata.journal_path)) {
                printf("FAIL reload journal path ownership\n"); failures++;
            }
            fresh.busy = 1;
            memcpy(&saved, &fresh, sizeof saved);
            if (tui_reload_apply(&fresh, &snapshot, why, sizeof why) != -1 ||
                memcmp(&fresh, &saved, sizeof fresh)) {
                printf("FAIL reload busy apply atomicity\n"); failures++;
            }
            if (tui_reload_capture(&fresh, &metadata, &snapshot, why, sizeof why) != -1 ||
                memcmp(&snapshot, &zero, sizeof snapshot) || memcmp(&fresh, &saved, sizeof fresh)) {
                printf("FAIL reload busy capture zero\n"); failures++;
            }
            fresh.busy = 0;
            memcpy(&saved, &fresh, sizeof saved);
            snapshot.loop_left = 9;
            if (tui_reload_apply(&fresh, &snapshot, why, sizeof why) != -1 ||
                memcmp(&fresh, &saved, sizeof fresh)) {
                printf("FAIL reload bad snapshot atomicity\n"); failures++;
            }
            source.loop_left = 9;
            if (tui_reload_capture(&source, &metadata, &snapshot, why, sizeof why) != -1 ||
                memcmp(&snapshot, &zero, sizeof snapshot)) {
                printf("FAIL reload invalid capture zero\n"); failures++;
            }
            {
                static reload_session_state before_metadata, expected, prepared;
                static reload_io_token token, old_token, zero_token;
                char dir[160];
                char path[4096], journal[4096];
                const char *line = "中文日志\n";
                FILE *f;
                int fixture_ok = 0, seq;
                for (seq = 0; seq < 100; seq++) {
                    snprintf(dir, sizeof dir, "/tmp/csih-reload-bridge-%ld-%d", (long)getpid(), seq);
                    if (mkdir(dir, 0700) == 0) { fixture_ok = 1; break; }
                }
                snprintf(path, sizeof path, "%s/state.json", dir);
                snprintf(journal, sizeof journal, "%s/journal.jsonl", dir);
                if (fixture_ok) {
                    f = fopen(journal, "wb");
                    if (!f) fixture_ok = 0;
                    else {
                        if (fwrite(line, 1, strlen(line), f) != strlen(line)) fixture_ok = 0;
                        if (fclose(f) != 0) fixture_ok = 0;
                    }
                }
                if (!fixture_ok) {
                    printf("FAIL reload bridge fixture\n"); failures++;
                } else {
                    source.loop_left = 0;
                    snprintf(metadata.journal_path, sizeof metadata.journal_path, "%s", journal);
                    metadata.journal_offset = 999;
                    memcpy(&before_metadata, &metadata, sizeof metadata);
                    if (tui_reload_export(&source, &metadata, path, why, sizeof why) != 0 ||
                        memcmp(&metadata, &before_metadata, sizeof metadata)) {
                        printf("FAIL reload bridge export %s\n", why); failures++;
                    }
                    before_metadata.journal_offset = (unsigned long long)strlen(line);
                    if (tui_reload_capture(&source, &before_metadata, &expected, why, sizeof why) != 0 ||
                        tui_reload_prepare(path, metadata.session_id, metadata.handoff_id,
                                           metadata.candidate_hash, &prepared, &token, why, sizeof why) != 0 ||
                        memcmp(&expected, &prepared, sizeof expected) || access(path, F_OK) != 0) {
                        printf("FAIL reload bridge prepare %s\n", why); failures++;
                    }
                    memcpy(&old_token, &token, sizeof token);
                    source.busy = 1;
                    if (tui_reload_export(&source, &metadata, path, why, sizeof why) != -1 ||
                        strcmp(why, "reload busy") ||
                        tui_reload_prepare(path, metadata.session_id, metadata.handoff_id,
                                           metadata.candidate_hash, &prepared, &token, why, sizeof why) != 0 ||
                        memcmp(&expected, &prepared, sizeof expected) ||
                        memcmp(&token, &old_token, sizeof token)) {
                        printf("FAIL reload bridge busy preserves file %s\n", why); failures++;
                    }
                    source.busy = 0;
                    if (tui_reload_prepare(path, "wrong", metadata.handoff_id, metadata.candidate_hash,
                                           &prepared, &token, why, sizeof why) != -1 ||
                        strcmp(why, "session_id mismatch") ||
                        memcmp(&prepared, &zero, sizeof prepared) ||
                        memcmp(&token, &zero_token, sizeof token) || access(path, F_OK) != 0) {
                        printf("FAIL reload bridge wrong identity retains file %s\n", why); failures++;
                    }
                    f = fopen(journal, "ab");
                    if (!f) { printf("FAIL reload bridge append fixture\n"); failures++; }
                    else {
                        int append_ok = fwrite("x", 1, 1, f) == 1;
                        if (fclose(f) != 0) append_ok = 0;
                        if (!append_ok) { printf("FAIL reload bridge append fixture\n"); failures++; }
                    }
                    if (tui_reload_prepare(path, metadata.session_id, metadata.handoff_id, metadata.candidate_hash,
                                           &prepared, &token, why, sizeof why) != -1 ||
                        strcmp(why, "journal length != snapshot offset") ||
                        memcmp(&prepared, &zero, sizeof prepared) ||
                        memcmp(&token, &zero_token, sizeof token) || access(path, F_OK) != 0) {
                        printf("FAIL reload bridge changed journal retains file %s\n", why); failures++;
                    }
                }
                if (fixture_ok) {
                    unlink(path);
                    unlink(journal);
                    rmdir(dir);
                }
            }
        }

  { static tui_state v; r_frame f; int i, saw;
    static const struct { int rows; int room; } cases[3] = { { 24, 17 }, { 40, 33 }, { 60, 40 } };
    int c;
    for (c = 0; c < 3; c++) {
      tui_state_init(&v, NULL); v.mode = 1; v.rows = cases[c].rows;
      snprintf(v.input, sizeof v.input, "HEIGHT_DRAFT");
      v.ninput = (int)strlen("HEIGHT_DRAFT");
      for (i = 0; i < 40; i++) tui_log_line(&v, "L");
      f = tui_render_state(&v, 80);
      saw = 0;
      for (i = 0; i < f.n; i++)
        if (strstr(f.lines[i], "HEIGHT_DRAFT")) saw = 1;
      if (tui_log_room(&v) != cases[c].room || f.n > cases[c].rows || f.n > 63 ||
          tui_frame_bad_width(&f, 80) || !saw) {
        printf("FAIL height%d\n", cases[c].rows); failures++;
      }
    }
  }
        {
            static tui_state h;
            tui_state_init(&h, NULL);
            h.mode = 1;
            h.busy = 1;
            tui_input_byte(&h, 'A');
            if (h.ninput != 1) { printf("FAIL hist A ninput\n"); failures++; }
            tui_apply_key(&h, TERM_KEY_ENTER, 0);
            tui_input_byte(&h, 'B');
            tui_apply_key(&h, TERM_KEY_ENTER, 0);
            if (h.nhistory != 2 || h.npending != 2 ||
                strcmp(h.history[0], "A") || strcmp(h.history[1], "B")) {
                printf("FAIL A/B history\n"); failures++;
            }
            {
                h.npending = 8;
                snprintf(h.input, sizeof(h.input), "C");
                h.ninput = 1;
                tui_apply_key(&h, TERM_KEY_ENTER, 0);
                if (strcmp(h.input, "C") || h.ninput != 1 || h.nhistory != 2 ||
                    h.npending != 8) { printf("FAIL fullQ\n"); failures++; }
                h.npending = 2;
            }
            {
                const char *dr = "\xe4\xb8\xad\xe6\x96\x87\xe8\x8d\x89\xe7\xa8\xbf";
                snprintf(h.input, sizeof(h.input), "%s", dr);
                h.ninput = (int)strlen(dr);
                tui_apply_key(&h, TERM_KEY_UP, 0);
                if (strcmp(h.input, "B")) { printf("FAIL UpB\n"); failures++; }
                tui_apply_key(&h, TERM_KEY_UP, 0);
                if (strcmp(h.input, "A")) { printf("FAIL UpA\n"); failures++; }
                tui_apply_key(&h, TERM_KEY_DOWN, 0);
                if (strcmp(h.input, "B")) { printf("FAIL DnB\n"); failures++; }
                tui_apply_key(&h, TERM_KEY_DOWN, 0);
                if (strcmp(h.input, dr) || h.ninput != (int)strlen(dr) ||
                    h.history_browsing) { printf("FAIL DnD\n"); failures++; }
                snprintf(h.input, sizeof(h.input), "X");
                h.ninput = 1;
                tui_apply_key(&h, TERM_KEY_UP, 0);
                tui_apply_key(&h, TERM_KEY_DOWN, 0);
                if (strcmp(h.input, "X")) { printf("FAIL X\n"); failures++; }
                if (h.log_skip) { printf("FAIL ls\n"); failures++; }
            }
        }

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
            static tui_state b;
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
            static tui_state pz;
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
                static tui_state lz;
                r_frame lf;
                int broke = 0, mark = 0;
                tui_state_init(&lz, NULL);
                lz.mode = 1;
                lz.rows = 54;
                tui_log_plain(&lz, "you> a\nb");
                snprintf(lz.pending[0], TUI_INPUT_MAX, "%s", "c\nd");
                lz.npending = 1;
                snprintf(lz.goal, TUI_INPUT_MAX, "%s", "g\nh");
                lz.loop_on = 1;
                lf = tui_render_state(&lz, 40);
                for (k = 0; k < lf.n; k++) {
                    if (strchr(lf.lines[k], '\n') || strchr(lf.lines[k], '\r')) broke = 1;
                    if (strstr(lf.lines[k], "you> a⏎b")) mark |= 1;
                    if (strstr(lf.lines[k], "待发送> c⏎d")) mark |= 2;
                    if (strstr(lf.lines[k], "goal> g⏎h · loop> on")) mark |= 4;
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
            static tui_state qx;
            tui_state_init(&qx, NULL);
            tui_type(&qx, "/exit");
            if (!qx.quit) { printf("FAIL /exit did not quit\n"); failures++; }
            tui_state_init(&qx, NULL);
            tui_type(&qx, "/quit");
            if (!qx.quit) { printf("FAIL /quit did not quit\n"); failures++; }
        }

        /* A timeout is not a quit and does change the tick line. */
        {
            static tui_state t2;
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
            static tui_state q;
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
            static tui_state g;
            int posts = 0, before, you = 0, i;
            tui_state_init(&g, NULL);
            g.mode = 1;
            tui_type(&g, "/goal");
            if (g.busy || g.goal[0] || tui_frame_has(&g, "goal>")) {
                printf("FAIL bare /goal\n"); failures++;
            } else printf("  ok   /goal omits the empty goal line\n");
            tui_type(&g, "/loop");
            if (g.busy || g.loop_on) {
                printf("FAIL /loop without a goal\n"); failures++;
            } else printf("  ok   /loop without goal stays off\n");
            tui_type(&g, "/goal keep-going");
            if (g.busy || strcmp(g.goal, "keep-going") != 0 || tui_frame_has(&g, "goal>")) {
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
            if (g.goal[0] || g.loop_on || g.npending != before) {
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
                static tui_state tall;
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
                static tui_state wide;
                r_frame fr;
                int k, goal_at = -1;
                tui_state_init(&wide, NULL);
                wide.mode = 1;
                wide.rows = 27;
                snprintf(wide.goal, sizeof wide.goal, "g");
                wide.loop_on = 1;
                tui_log_line(&wide, "only-one");
                fr = tui_render_state(&wide, 40);
                for (k = 0; k < fr.n; k++)
                    if (!strncmp(fr.lines[k], "goal>", 5)) { goal_at = k; break; }
                if (goal_at != 2 || !strstr(fr.lines[1], "only-one")
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
                                    if (strstr(fr.lines[k], "Enter 发送")
                                        && !strstr(fr.lines[k], "PgUp")) saw = 1;
                                if (wide.log_skip != 0 || !saw || !tui_frame_has(&wide, "hist-new")
                                    || tui_frame_bad_width(&fr, 80)) {
                                    printf("FAIL log end or wide hint\n");
                                    failures++;
                                } else {
                                    tui_apply_key(&wide, TERM_KEY_PGUP, 0);
                                    tui_apply_key(&wide, TERM_KEY_HOME, 0);
                                    if (wide.log_skip != 20 || !tui_frame_has(&wide, "hist-01")
                                        || tui_frame_has(&wide, "hist-new")) {
                                        printf("FAIL log home skip %d\n", wide.log_skip);
                                        failures++;
                                    } else {
                                        tui_apply_key(&wide, TERM_KEY_PGDN, 0);
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
                snprintf(wide.goal, sizeof wide.goal, "g");
                wide.loop_on = 1;
                fr = tui_render_state(&wide, 40);
                goal_at = -1;
                for (k = 0; k < fr.n; k++)
                    if (!strncmp(fr.lines[k], "goal>", 5)) { goal_at = k; break; }
                if (goal_at < 1 || (goal_at > 1 && !fr.lines[goal_at - 1][0])) {
                    /* No blank rows should pad the empty log view; goal sits
                     * right below the header. */
                    printf("FAIL empty log view goal at %d\n", goal_at);
                    failures++;
                } else printf("  ok   empty log view unpadded, goal at %d\n", goal_at);
            }
            {
                static tui_state ex;
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
                static tui_state er;
                tui_state_init(&er, NULL);
                er.mode = 1;
                er.rows = 24;
                if (tui_frame_has(&er, "错误>")) {
                    printf("FAIL empty error status\n"); failures++;
                }
                tui_log_event(&er, "exit=1 nfs: not responding");
                if (!tui_frame_has(&er, "错误> ") || !tui_frame_has(&er, "not responding")) {
                    printf("FAIL error status did not take the nfs line\n"); failures++;
                } else printf("  ok   error status keeps the nfs line\n");
            }
            {
                static tui_state mind;
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
                    || paired || !tee || bars != 0 || mf.n > mind.rows
                    || tui_frame_bad_width(&mf, 40)
                    || tui_frame_has(&mind, "```") || tui_frame_has(&mind, "flowchart LR")) {
                    printf("FAIL mind drawer stays closed\n"); failures++;
                } else printf("  ok   rules between hint, system, and mind\n");
                {
                    int my = 0, opened = 0, obars = 0, opaired = 0;
                    for (k = 0; k < mf.n; k++) {
                        if (strstr(mf.lines[k], "-<思维树>") && strstr(mf.lines[k], "[展开]"))
                            my = k + 1;
                    }
                    if (my > 0) tui_click(&mind, 1, my);
                    mf = tui_render_state(&mind, 40);
                    for (k = 0; k < mf.n; k++) {
                        if (strstr(mf.lines[k], "-<思维树>") && strstr(mf.lines[k], "[收缩]"))
                            opened = 1;
                        if (strstr(mf.lines[k], "乙") && strstr(mf.lines[k], "│")
                            && strstr(mf.lines[k], "丑")) opaired = 1;
                        if (strstr(mf.lines[k], "│")) obars++;
                    }
                    if (!opened || !opaired || obars != TUI_MIND_N || mf.n > mind.rows
                        || tui_frame_bad_width(&mf, 40)) {
                        printf("FAIL mind drawer did not open\n"); failures++;
                    } else printf("  ok   mind drawer opens\n");
                }
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
            static tui_state hand;
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

    if (!strcmp(cmd, "run") || !strcmp(cmd, "agent") ||
        !strcmp(cmd, "agent-owned") || !strcmp(cmd, "resume-agent") ||
        !strcmp(cmd, "agent-managed") || !strcmp(cmd, "standby-managed")) {
        /* The real loop. Needs a terminal; refuses politely without one rather
         * than emitting escape sequences into a pipe. `agent` is the same loop
         * in agent mode (Enter drives the DeepSeek loop instead of the gate). */
        static tui_state st;
        static reload_io_token resume_token;
        char owned_why[256];
        int cols, first = 1;
        int managed_mode = !strcmp(cmd, "agent-managed") || !strcmp(cmd, "standby-managed");
        int standby_mode = !strcmp(cmd, "standby-managed");
        int owned_mode = !strcmp(cmd, "agent-owned") || !strcmp(cmd, "resume-agent") || managed_mode;
        int resume_mode = !strcmp(cmd, "resume-agent");
        int agent_mode = strcmp(cmd, "run") != 0;
        if ((owned_mode && !resume_mode && !managed_mode && argc != 5) || (resume_mode && argc != 7) ||
            (managed_mode && argc != (standby_mode ? 7 : 6))) {
            printf("usage: agent-owned DIR SESSION HASH | resume-agent DIR STATE SESSION HANDOFF HASH\n");
            return 64;
        }

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
            if (!owned_mode) for (ai = 2; ai < argc; ai++) {
                if (!strcmp(argv[ai], "spill")) agent_set_spill(1);
                else if (!transcript) transcript = argv[ai];
            }
            tui_state_init(&st, transcript);
        }
        if (managed_mode && tui_managed_init(argv[2], argv[3], standby_mode ? argv[4] : "initial",
                standby_mode ? argv[5] : argv[4], standby_mode ? argv[6] : argv[5], standby_mode) != 0) {
            printf("managed startup refused\n"); return 1;
        }
        if (owned_mode && !standby_mode && tui_owned_start(&st, argv[2], resume_mode ? argv[4] : argv[3],
                 resume_mode ? argv[6] : argv[4], resume_mode ? argv[3] : NULL,
                 resume_mode ? argv[5] : NULL, &resume_token, owned_why, sizeof owned_why) != 0) {
            printf("owned startup refused: %s\n", owned_why);
            if (tui_owned_close(&st, owned_why, sizeof owned_why) != 0)
                printf("ownership return not confirmed: %s\n", owned_why);
            return 1;
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
        if (!standby_mode && term_raw_enter() != 0) {
            printf("cannot enter raw mode\n");
            if (tui_owned_close(&st, owned_why, sizeof owned_why) != 0)
                printf("ownership return not confirmed: %s\n", owned_why);
            return 1;
        }
        /* Commit point: validated UI/context applied, ownership held, raw TTY
         * ready. Consume exactly once; cleanup failure is not a second apply. */
        if (resume_mode && reload_io_consume(argv[3], &resume_token, owned_why, sizeof owned_why) != 0) {
            term_raw_leave();
            printf("restore committed; state cleanup failed: %s; do not reapply\n", owned_why);
            if (tui_owned_close(&st, owned_why, sizeof owned_why) != 0)
                printf("ownership return not confirmed: %s\n", owned_why);
            return 1;
        }
        if (managed_mode) {
            tui_managed.raw = !standby_mode;
            if (tui_managed_send(standby_mode ? "READY" : "ACK", 0) != 0) st.quit = 1;
        }
        if (owned_mode && !standby_mode) tui_log_plain(&st, "owned session; HASH is explicit binding, not verified source identity");

        while (!st.quit && !term_interrupted()) {
            if (managed_mode) {
                struct pollfd ctl;
                tui_managed_poll(&st);
                if (st.quit) break;
                if (tui_managed.phase != 3) {
                    ctl.fd = tui_managed.fd; ctl.events = POLLIN; ctl.revents = 0;
                    poll(&ctl, 1, TUI_FRAME_MS); continue;
                }
            }
            if (st.busy && !agent_turn_step(0)) tui_turn_done(&st);
            if (!st.busy && st.npending > 0 && st.mode == 1) {
                tui_run_pending(&st);
            }
            tui_mail_poll(&st, !st.busy && st.npending == 0 && st.mode == 1);
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
        if (!managed_mode || (!tui_managed.retired && tui_managed.raw && st.owner_fd >= 0)) {
            term_raw_leave();
            tui_write_all("\x1b[?25h\x1b[0m\n", 9);
        }
        if (managed_mode) close(tui_managed.fd);
        /* Successful retirement must not restore TTY state over the new actor. */
        /* show cursor, reset, newline */
        if (tui_owned_close(&st, owned_why, sizeof owned_why) != 0) {
            printf("ownership return not confirmed: %s\n", owned_why); return 1;
        }
        return 0;
    }

    printf("usage: tui selftest | once | run | agent | agent-owned DIR SESSION HASH | resume-agent DIR STATE SESSION HANDOFF HASH\n");
    return 64;
}
