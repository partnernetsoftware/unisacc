/* Test watchdog only: POSIX process ownership, never product runtime code.

   r5: no signal carries a deadline.  Every wait is a polling loop over the monotonic clock: waitpid(WNOHANG)
   with nanosleep slices, poll() on the ps pipe with the remaining time, and a non-blocking read.  A deadline
   cannot be missed because nothing blocks without a timeout, and there is no alarm/flag race.
   Signals TERM/INT/HUP only record an interruption, as before. */
#define _XOPEN_SOURCE 700
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <dirent.h>
#include <ctype.h>
int bg_seconds(const char *);
int bg_scale(int, const char *, const char *);
int bg_reap_step(int, int, int, int, int);
int bg_reap_cause(int);
int bg_combine(int, int);
int bg_reason_enum(int);
int bg_reason_deadline(int, int);
int bg_stop(int, int);
int bg_tree_room(int);
int bg_ps_room(int);
#ifdef __linux__
#include <sys/prctl.h>
#endif

#define TOTAL_EXTRA_SECS 2.0
/* Outer-wall epsilon: the only slack the contract allows past the hard deadline, for the one bounded final action after it
   (one SIGKILL round, one WNOHANG reap, the exit).  Every harness reads this constant; no case writes its own slack. */
#define CLEANUP_EPSILON_SECS 0.25
/* The ps snapshot's own kill point, before the end of its budget: the SIGKILL goes out here, and the reap of that
   child must finish inside the same budget, so the snapshot never runs past its share of the deadline. */
#define SNAP_REAP_SLACK_SECS 0.05
/* Cleanup reserve: the command is killed at hard_at minus this, so every cleanup step runs inside the reserve and the
   hard deadline is never pushed back.  reserve = min(CLEANUP_RESERVE_MAX_SECS, CLEANUP_RESERVE_FRAC * window). */
#define CLEANUP_RESERVE_MAX_SECS 2.0
#define CLEANUP_RESERVE_FRAC 0.10
/* Diagnostic contract (defined once; the tests read these texts from this file).  The two causes are different:
   an enumeration failure is a fact about ps or its pipe; a deadline is the clock.  Both are named when both hold. */
#define BOUND_REASON_DEADLINE "hard deadline reached"
#define BOUND_REASON_ENUM "process enumeration failed"
#define CAUSE_ENUM 1
#define CAUSE_DEADLINE 2
/* Exit code of every stop that is not green.  It never depends on the reason text. */
#define BOUND_STOP_RC 142
/* The unreaped report: the one spelling.  tests/bound.py, tests/boundcheck.py and tests/boundstall_r7g.py read it from here. */
#define BOUND_UNREAPED_FMT "bound: %s (not green); unreaped child pid %ld;"
#define TREE_MAX 4096
#define PS_OUTPUT_MAX (16u * 1024u * 1024u)   /* a ps listing larger than this is a refusal, not a green */
#define SLICE_SECS 0.01

static volatile sig_atomic_t interrupted;
static double hard_at;            /* the one absolute hard deadline (CLOCK_MONOTONIC), shared with children */
static pid_t tree[TREE_MAX];
static size_t tree_n;
static pid_t ps_child;            /* the ps process while it is unreaped; named in the report */

static double mono_now(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec + (double)t.tv_nsec / 1e9;
}
static int past_hard(void) { return mono_now() >= hard_at; }
static void nap(double seconds) {
    struct timespec t;
    if (seconds <= 0) return;
    t.tv_sec = (time_t)seconds;
    t.tv_nsec = (long)((seconds - (double)t.tv_sec) * 1e9);
    nanosleep(&t, NULL);
}
static double slice(double until) {
    double left = until - mono_now();
    if (left <= 0) return 0;
    return left < SLICE_SECS ? left : SLICE_SECS;
}
static void interrupt(int sig) {
    if (!interrupted) interrupted = 128 + sig;
}

/* Polling reap: never blocks.  Returns 1 reaped, 0 deadline reached first, -2 interrupted by a signal.
   At the deadline it still makes one WNOHANG attempt (the finite last check). */
static void trace_mark(const char *what);
/* One trace line per wait return: the pid, the return value (1 reaped, 0 deadline, -1 error, -2 interrupted), the time. */
static void trace_wait(pid_t pid, int ret) {
    char buf[96];
    snprintf(buf, sizeof buf, "wait pid=%ld ret=%d", (long)pid, ret);
    trace_mark(buf);
}
static int reap_until(pid_t pid, int *status, double until, int honor_interrupt) {
    for (;;) {
        pid_t got = waitpid(pid, status, WNOHANG);
        if (got == pid) { trace_wait(pid, 1); return 1; }
        if (got < 0 && errno != EINTR) return -1;
        if (honor_interrupt && interrupted) { trace_wait(pid, -2); return -2; }
        if (mono_now() >= until) { trace_wait(pid, 0); return 0; }
        nap(slice(until));
    }
}

/* Pure decision for one cleanup reap step, given waitpid(-1, WNOHANG)'s result (got, errno err) and whether the hard
   deadline has passed.  Only ECHILD ends the cleanup cleanly; EINTR is retried inside the same deadline; any other
   error is a named enumeration failure, never a silent success. */
enum { REAP_STEP_CONTINUE, REAP_STEP_SLEEP, REAP_STEP_DONE, REAP_STEP_DEADLINE, REAP_STEP_ENUM };
static int reap_step(pid_t got, int err, int past) {
    return bg_reap_step((int)got, err, past, ECHILD, EINTR);
}

static int has(pid_t pid) {
    for (size_t i = 0; i < tree_n; i++) if (tree[i] == pid) return 1;
    return 0;
}

/* Bounded ps snapshot.  The ps child is ours: its pipe is non-blocking and read only after poll() says it is
   readable, with the remaining time as the poll timeout.  Past the hard deadline, or past PS_OUTPUT_MAX, the ps
   child is SIGKILLed and the snapshot is refused; its reap is bounded like any other. */
/* Maps a reap result to a cause: reaped -> none; the clock reached first (0) -> deadline; waitpid failed (-1) ->
   enumeration failure.  The reap error is not a deadline and is never reported as one. */
static int reap_cause(int reap) { return bg_reap_cause(reap); }
/* The snapshot's own cause wins; otherwise the reap result decides. */
/* Causes are bit flags (CAUSE_ENUM | CAUSE_DEADLINE): a snapshot cause never hides a reap cause, so when both hold,
   both are kept and both are named. */
static int combine_cause(int snapshot_cause, int reap) { return bg_combine(snapshot_cause, reap); }

/* The snapshot's own budget: a third of the remaining time, at most 0.5 s, so the adopted sweep always keeps time. */
/* Monotonic cleanup timestamps: written only when UNISACC_BOUND_CLEANUP_TRACE names a file.  Marks: kill (after the
   known-tree kills), scan_start, scan_end, exit.  They let a later review see the order instead of guessing. */
static void trace_mark(const char *what) {
    const char *path = getenv("UNISACC_BOUND_CLEANUP_TRACE");
    if (!path || !*path) return;
    FILE *f = fopen(path, "a");
    if (!f) return;
    fprintf(f, "%.6f pid=%ld %s\n", mono_now(), (long)getpid(), what);   /* every event carries the writing PID */
    fclose(f);
}
static double snapshot_until(void) {
    double now = mono_now();
    double share = (hard_at - now) / 3.0;
    if (share > 0.5) share = 0.5;
    if (share < 0) share = 0;
    return now + share;
}
static int ps_snapshot(char **out, size_t *outlen) {
    int fds[2];
    *out = NULL; *outlen = 0;
    if (pipe(fds)) return CAUSE_ENUM;
    pid_t ps = fork();
    if (ps < 0) { close(fds[0]); close(fds[1]); return CAUSE_ENUM; }
    if (!ps) {
        const char *path = getenv("UNISACC_BOUND_PS");   /* test seam: the fixture stands in for ps */
        if (!path || !*path) path = "/bin/ps";
        close(fds[0]);
        if (dup2(fds[1], 1) < 0) _exit(127);
        close(fds[1]);
        execl(path, "ps", "-axo", "pid=,ppid=", (char *)NULL);
        _exit(127);
    }
    ps_child = ps;
    close(fds[1]);
    size_t cap = 4096, n = 0;
    char *buf = malloc(cap);
    int cause = 0;
    int flags = fcntl(fds[0], F_GETFL);
    if (!buf || flags < 0 || fcntl(fds[0], F_SETFL, flags | O_NONBLOCK) < 0) cause = CAUSE_ENUM;
    double until = snapshot_until();
    double kill_at = until - SNAP_REAP_SLACK_SECS;
    if (kill_at < mono_now()) kill_at = mono_now();
    while (!cause) {
        if (mono_now() >= kill_at) { trace_mark("ps_deadline"); cause = CAUSE_DEADLINE; break; }
        if (bg_ps_room(n > 2147483647u ? 16777215 : (int)n) == 1) { cause = CAUSE_ENUM; break; }
        if (n + 1 >= cap) {
            char *larger = realloc(buf, cap * 2);
            if (!larger) { cause = CAUSE_ENUM; break; }
            buf = larger; cap *= 2;
        }
        struct pollfd pf = {fds[0], POLLIN, 0};
        double left = kill_at - mono_now();
        int ms = (int)((left < SLICE_SECS ? left : SLICE_SECS) * 1000.0);
        if (ms < 1) ms = 1;
        int r = poll(&pf, 1, ms);
        if (r < 0) { if (errno == EINTR) continue; cause = CAUSE_ENUM; break; }
        if (r == 0) continue;
        ssize_t got = read(fds[0], buf + n, cap - n - 1);
        if (got < 0) { if (errno == EAGAIN || errno == EINTR) continue; cause = CAUSE_ENUM; break; }
        if (got == 0) break;   /* EOF: ps finished writing */
        n += (size_t)got;
    }
    close(fds[0]);
    if (cause) kill(ps, SIGKILL);
    int status = 0;
    int reap = reap_until(ps, &status, until, 0);
    if (reap == 1) ps_child = 0;
    {
        char mark[96];
        snprintf(mark, sizeof mark, "ps_kill pid=%ld reaped=%d", (long)ps, reap == 1);
        trace_mark(mark);
    }
    cause = combine_cause(cause, reap);
    /* A snapshot is only as good as ps's own exit: exit 0 is required, anything else is an enumeration failure. */
    if (!cause && (!WIFEXITED(status) || WEXITSTATUS(status) != 0)) cause = CAUSE_ENUM;
    if (cause) { free(buf); return cause; }
    buf[n] = 0;
    *out = buf; *outlen = n;
    return 0;
}

/* Returns 0 when the tree is frozen, otherwise the cause (CAUSE_ENUM or CAUSE_DEADLINE). */
static int freeze_tree(pid_t root) {
    int cause = 0;
    sigset_t blocked, previous;
    sigemptyset(&blocked);
    sigaddset(&blocked, SIGTERM); sigaddset(&blocked, SIGINT); sigaddset(&blocked, SIGHUP);
    sigprocmask(SIG_BLOCK, &blocked, &previous);
    if (!has(root)) {
        if (bg_tree_room((int)tree_n) == 1) cause = CAUSE_ENUM;
        else tree[tree_n++] = root;
    }
    kill(root, SIGSTOP);
    for (;;) {
        int added = 0;
        char *text = NULL; size_t len = 0;
        if (cause) break;
        if (past_hard()) { cause = CAUSE_DEADLINE; break; }
        cause = ps_snapshot(&text, &len);
        if (cause) break;
        char *p = text;
        while (!cause && *p) {
            if (mono_now() >= hard_at) { cause = CAUSE_DEADLINE; break; }
            char *e1 = NULL, *e2 = NULL;
            long pid = strtol(p, &e1, 10);
            if (e1 == p) { p++; continue; }
            long parent = strtol(e1, &e2, 10);
            if (e2 == e1) break;
            p = e2;
            if (pid <= 0 || !has((pid_t)parent) || has((pid_t)pid)) continue;
            if (bg_tree_room((int)tree_n) == 1) { cause = CAUSE_ENUM; break; }
            tree[tree_n++] = (pid_t)pid;
            kill((pid_t)pid, SIGSTOP);
            added = 1;
        }
        free(text);
        if (cause || !added) break;
    }
    /* Kill the whole known tree, including after a deadline (no new ps pass then). */
    kill(-root, SIGKILL);
    for (size_t i = 0; i < tree_n; i++) kill(tree[i], SIGKILL);
    sigprocmask(SIG_SETMASK, &previous, NULL);
    return cause;
}

/* Pure reason matrix: which texts a stop names.  An enumeration failure names itself; a deadline names itself when
   the clock is the cause, or when the clock has reached the hard deadline at the report (both are then named). */
static void reason_flags(int cause, int past, int *enumeration, int *deadline) {
    *enumeration = bg_reason_enum(cause);
    *deadline = bg_reason_deadline(cause, past);
}
/* The composed reason text for a stop.  Pure: no I/O, no pids. */
static void reason_text(int cause, int past, char *out, size_t cap) {
    int enumeration, deadline;
    reason_flags(cause, past, &enumeration, &deadline);
    out[0] = 0;
    if (enumeration) strncat(out, BOUND_REASON_ENUM, cap - strlen(out) - 1);
    if (enumeration && deadline) strncat(out, "; ", cap - strlen(out) - 1);
    if (deadline) strncat(out, BOUND_REASON_DEADLINE, cap - strlen(out) - 1);
}
/* The exit code of a stop.  It takes the cause but ignores it: the reason text is diagnostic, never a code. */
static int stop_rc(int cause, int past) { return bg_stop(cause, past); }

/* One named report for every stop, and the stop's exit code.  Both causes are named when both hold. */
/* The fixed cleanup sequence, the same on every exit path (normal, deadline, enumeration failure).
   (1) kill the child's process group; (2) kill the known tree; (3) sweep the adopted: every live process whose parent is
   this bound (PR_SET_CHILD_SUBREAPER adopts a setsid descendant when its parent dies, and the descendant keeps the
   inherited stdout pipe), SIGKILL it and record it in the tree; repeat until a sweep finds none.  A zombie is reaped,
   not killed.  Nothing here waits: kills are immediate, and the loop is bounded by a fixed round count. */
#ifdef __linux__
static int sweep_adopted(pid_t child) {
    DIR *d = opendir("/proc");
    if (!d) return -1;
    int live_hits = 0;
    struct dirent *e;
    while ((e = readdir(d))) {
        if (mono_now() >= hard_at) {                       /* the shared deadline stops the walk mid-pass */
            trace_mark("deadline-hit in-scan");
            closedir(d);
            return -2;
        }
        if (!isdigit((unsigned char)e->d_name[0])) continue;
        pid_t q = (pid_t)strtol(e->d_name, NULL, 10);
        if (q <= 0 || q == getpid() || q == child) continue;
        char path[64];
        snprintf(path, sizeof path, "/proc/%ld/stat", (long)q);
        FILE *f = fopen(path, "r");
        if (!f) continue;
        char buf[512];
        size_t n = fread(buf, 1, sizeof buf - 1, f);
        fclose(f);
        buf[n] = 0;
        char *rp = strrchr(buf, ')');
        char st = 0;
        long ppid = -1;
        if (!rp || sscanf(rp + 2, "%c %ld", &st, &ppid) != 2) continue;
        if (ppid != (long)getpid() || st == 'Z') continue;   /* zombies are reaped by waitpid, not killed */
        kill(q, SIGKILL);
        if (!has(q) && bg_tree_room((int)tree_n) == 0) tree[tree_n++] = q;
        live_hits++;
    }
    closedir(d);
    return live_hits;
}
#else
static int sweep_adopted(pid_t child) { (void)child; return 0; }
#endif
/* Returns 0 when the adopted sweep proved there is nothing left, CAUSE_ENUM when /proc could not be read (no proof),
   CAUSE_DEADLINE when the shared hard deadline came first.  Every wait is bounded by hard_at; no round count is a time
   budget. */
#ifdef __linux__
static int known_gone(void) {
    for (size_t i = 0; i < tree_n; i++) {
        if (mono_now() >= hard_at) return 0;                /* deadline: finish_cleanup sees past_hard and stops */
        char path[64];
        snprintf(path, sizeof path, "/proc/%ld/stat", (long)tree[i]);
        FILE *f = fopen(path, "r");
        if (!f) continue;                                 /* gone */
        char buf[512];
        size_t n = fread(buf, 1, sizeof buf - 1, f);
        fclose(f);
        buf[n] = 0;
        char *rp = strrchr(buf, ')');
        if (rp && rp[2] != 'Z') return 0;                  /* alive and not a zombie */
    }
    return 1;
}
#endif
static int finish_cleanup(pid_t child) {
    kill(-child, SIGKILL);
    kill(child, SIGKILL);
    for (size_t i = 0; i < tree_n; i++) kill(tree[i], SIGKILL);
    if (ps_child) kill(ps_child, SIGKILL);
    trace_mark("kill");
#ifdef __linux__
    while (!known_gone()) {                               /* the killed parents must die before their children are adopted */
        if (past_hard()) { trace_mark("scan_end deadline-before-scan"); return CAUSE_DEADLINE; }
        nap(slice(hard_at));
    }
#endif
    trace_mark("scan_start");
    while (!past_hard()) {
        int hits = sweep_adopted(child);
        if (hits == -2) { trace_mark("scan_end deadline-mid-pass"); return CAUSE_DEADLINE; }
        if (hits < 0) { trace_mark("scan_end enum"); return CAUSE_ENUM; }
        if (hits == 0) { trace_mark("scan_end clean"); return 0; }
        kill(-child, SIGKILL);
        for (size_t i = 0; i < tree_n; i++) kill(tree[i], SIGKILL);
        nap(slice(hard_at));
    }
    trace_mark("scan_end deadline");
    return CAUSE_DEADLINE;                               /* the deadline came first: no further sweep, no extra time */
}
static int report_stop(pid_t child, int cause) {
    trace_mark("report_start");
    cause |= finish_cleanup(child);                      /* every early return runs the whole sequence, within hard_at */
    while (mono_now() < hard_at && waitpid(-1, NULL, WNOHANG) > 0) {}   /* best effort, within the deadline */
    char reason[160];
    reason_text(cause, past_hard(), reason, sizeof reason);
    fprintf(stderr, BOUND_UNREAPED_FMT, reason, (long)child);
    if (ps_child) fprintf(stderr, " unreaped ps pid %ld;", (long)ps_child);
    fprintf(stderr, " known tree pids:");
    for (size_t i = 0; i < tree_n; i++) fprintf(stderr, " %ld", (long)tree[i]);
    fprintf(stderr, "\n");
    trace_mark("report_end");
    trace_mark("exit stop");
    return stop_rc(cause, past_hard());
}

static int save_status(const char *path, int status) {
    if (!path) return 0;
    FILE *f = fopen(path, "w");
    if (!f) return -1;
    int bad = fprintf(f, "%d\n", status) < 0;
    if (fclose(f)) bad = 1;
    return bad ? -1 : 0;
}

int main(int argc, char **argv) {
    const char *status_path = NULL;
    int first = 1, status = 0;
    if (argc > 2 && !strcmp(argv[first], "--status")) {
        status_path = argv[first + 1]; first += 2;
    }
    if (argc < first + 2 || !argv[first][0]) goto usage;
    int sec = bg_seconds(argv[first]);
    if (sec < 1) goto usage;
    unsigned seconds = (unsigned)sec;
    trace_mark("bound_start");
    {   /* 0.0.27 C1: on GitHub's hosted runners (GITHUB_ACTIONS=true) UNISACC_TIME_SCALE 1..5 stretches
           every bound with the slower machine; locally the scale is always 1. */
        const char *ga = getenv("GITHUB_ACTIONS"), *sc = getenv("UNISACC_TIME_SCALE");
        int scaled = bg_scale((int)seconds, ga, sc);
        if (scaled < 1) goto usage;
        seconds = (unsigned)scaled;
    }
    struct sigaction action;
    memset(&action, 0, sizeof action); action.sa_handler = interrupt;
    sigemptyset(&action.sa_mask);
    sigaction(SIGTERM, &action, NULL); sigaction(SIGINT, &action, NULL); sigaction(SIGHUP, &action, NULL);
#ifdef __linux__
    /* Adopt detached grandchildren when their command parent exits. */
    if (prctl(PR_SET_CHILD_SUBREAPER, 1, 0, 0, 0)) { perror("bound: subreaper"); return 2; }
#endif
    /* One absolute hard deadline, shared by every layer: min(now+N+2, parent), exported to children.
       The command gets N: its wait ends at hard-2, and the last 2 seconds are for cleanup. */
    double now = mono_now();
    double hard = now + (double)seconds + TOTAL_EXTRA_SECS;
    const char *inherited = getenv("UNISACC_BOUND_DEADLINE_MONO");
    if (inherited) {
        char *end = NULL;
        double parent = strtod(inherited, &end);
        if (end == inherited || *end || !isfinite(parent) || parent <= 0) {
            fprintf(stderr, "bound: bad UNISACC_BOUND_DEADLINE_MONO=%s, refusing to start (not green)\n", inherited);
            return 2;
        }
        if (parent <= now) {
            fprintf(stderr, "bound: inherited deadline already passed, refusing to start (not green)\n");
            return BOUND_STOP_RC;
        }
        if (parent < hard) hard = parent;
    }
    hard_at = hard;
    {
        char hb[80];
        snprintf(hb, sizeof hb, "hard_at %.6f", hard_at);
        trace_mark(hb);
    }
    double window = hard - now;
    double reserve = CLEANUP_RESERVE_FRAC * window;
    if (reserve > CLEANUP_RESERVE_MAX_SECS) reserve = CLEANUP_RESERVE_MAX_SECS;
    double command_at = hard - reserve;
    if (command_at < now) command_at = now;
    {
        char value[64];
        snprintf(value, sizeof value, "%.9f", hard);
        if (setenv("UNISACC_BOUND_DEADLINE_MONO", value, 1)) { perror("bound: setenv"); return 2; }
    }
    pid_t child = fork();
    if (child < 0) { perror("bound: fork"); return 2; }
    if (!child) {
        signal(SIGTERM, SIG_DFL); signal(SIGINT, SIG_DFL); signal(SIGHUP, SIG_DFL);
        if (setsid() < 0) { perror("bound: setsid"); _exit(127); }
        execvp(argv[first + 1], argv + first + 1);
        perror("bound: exec"); _exit(127);
    }
    /* Command wait: polls until the command deadline.  A timeout or a signal goes to the cleanup path. */
    int r = reap_until(child, &status, command_at, 1);
    if (r == -1) { perror("bound: waitpid"); return 2; }
    if (r != 1) {
        if (!interrupted) interrupted = 142;
        int frozen = freeze_tree(child);
        if (frozen != 0) return report_stop(child, frozen);
        int reaped = reap_until(child, &status, hard_at, 0);
        if (reaped != 1) return report_stop(child, reap_cause(reaped));
    }
    /* Cleanup: the fixed sequence, the same on every exit path (see finish_cleanup). */
    int fc = finish_cleanup(child);
    if (fc) return report_stop(child, fc);
#ifdef __linux__
    /* Reap every remaining child by polling; the step is decided by a pure function (reap_step) so the matrix can
       cover every branch: ECHILD ends cleanly, EINTR is retried within the same deadline, another error is named. */
    for (;;) {
        pid_t got = waitpid(-1, NULL, WNOHANG);
        int err = got < 0 ? errno : 0;
        int action = reap_step(got, err, past_hard());
        if (action == REAP_STEP_DONE) break;
        if (action == REAP_STEP_CONTINUE) continue;
        if (action == REAP_STEP_SLEEP) { nap(slice(hard_at)); continue; }
        if (action == REAP_STEP_DEADLINE) return report_stop(child, CAUSE_DEADLINE);
        return report_stop(child, CAUSE_ENUM);      /* REAP_STEP_ENUM: waitpid failed for a reason other than ECHILD */
    }
#endif
    if (past_hard() && !interrupted) return report_stop(child, CAUSE_DEADLINE);
    if (save_status(status_path, status)) { perror("bound: status file"); return 2; }
    trace_mark("exit normal");
    if (interrupted) return interrupted;
    return WIFEXITED(status) ? WEXITSTATUS(status) : WIFSIGNALED(status) ? 128 + WTERMSIG(status) : 2;
usage:
    fprintf(stderr, "usage: bound [--status FILE] SECONDS(1..60) COMMAND [ARG...]\n");
    return 2;
}
