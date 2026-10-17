/* Full pure matrix for tests/bound.c (r7d).  Compile-only in this cut; the run needs the token.

   Every cell is (snapshot outcome) x (reap outcome) x (hard deadline reached or not).  For each consistent cell the
   test derives the expected cause and expected text by hand, from the contract, and asserts:
     - combine_cause() gives the expected cause;
     - reason_text() gives the expected composed text (enumeration / deadline / both);
     - stop_rc() gives 142, whatever the cause.
   Consistency: a reap that reports the clock (0) implies the clock was reached; a snapshot deadline implies it too.
   Exit 0 when every cell holds, 1 otherwise. */
#define main bound_main_under_test
#include "bound.c"
#undef main

enum { SNAP_OK = 0, SNAP_ENUM = 1, SNAP_DEADLINE = 2 };
enum { REAP_OK = 1, REAP_CLOCK = 0, REAP_ERROR = -1 };

static int fails, cells;

int main(void) {
    const int snaps[] = {SNAP_OK, SNAP_ENUM, SNAP_DEADLINE};
    const int reaps[] = {REAP_OK, REAP_CLOCK, REAP_ERROR};
    const char *snap_name[] = {"snapshot ok", "snapshot enumeration failed", "snapshot deadline"};
    const char *reap_name[] = {"reap ok", "reap clock", "reap error"};
    for (int s = 0; s < 3; s++)
        for (int r = 0; r < 3; r++)
            for (int past = 0; past <= 1; past++) {
                if (reaps[r] == REAP_CLOCK && !past) continue;      /* a clock reap implies the clock was reached */
                if (snaps[s] == SNAP_DEADLINE && !past) continue;   /* a snapshot deadline implies it too */
                /* expected, derived from the contract, not from the code under test */
                /* causes are bit flags: the snapshot's cause and the reap's cause are both kept (owner: both named) */
                int want_cause = (snaps[s] == SNAP_ENUM ? CAUSE_ENUM : snaps[s] == SNAP_DEADLINE ? CAUSE_DEADLINE : 0)
                               | (reaps[r] == REAP_OK ? 0 : reaps[r] == REAP_CLOCK ? CAUSE_DEADLINE : CAUSE_ENUM);
                int want_enum = (want_cause & CAUSE_ENUM) != 0;
                int want_deadline = (want_cause & CAUSE_DEADLINE) != 0 || past;
                char want[200] = "";
                if (want_enum) strcat(want, BOUND_REASON_ENUM);
                if (want_enum && want_deadline) strcat(want, "; ");
                if (want_deadline) strcat(want, BOUND_REASON_DEADLINE);
                char got[200];
                int cause = combine_cause(snaps[s], reaps[r]);
                reason_text(cause, past, got, sizeof got);
                int ok_cause = cause == want_cause;
                int ok_text = strcmp(got, want) == 0;
                int ok_rc = stop_rc(cause, past) == 142 && BOUND_STOP_RC == 142;
                int ok = ok_cause && ok_text && ok_rc;
                cells++;
                if (!ok) fails++;
                printf("%s %-28s %-10s past=%d -> cause=%d text=\"%s\" rc=%d\n", ok ? "PASS" : "FAIL",
                       snap_name[s], reap_name[r], past, cause, got, stop_rc(cause, past));
            }
    /* Texts are fixed by the contract; neither may be empty or mention the other cause's words. */
    int texts_ok = strstr(BOUND_REASON_ENUM, "deadline") == NULL && strstr(BOUND_REASON_DEADLINE, "deadline") != NULL;
    if (!texts_ok) fails++;
    printf("%s texts: enumeration is neutral, deadline names the deadline\n", texts_ok ? "PASS" : "FAIL");
    /* Cleanup reap branch (reap_step): every waitpid(-1, WNOHANG) outcome, with the clock reached or not. */
    struct { const char *label; pid_t got; int err; int expect_past0; int expect_past1; } steps[] = {
        {"reaped one child",            5,  0,      REAP_STEP_CONTINUE, REAP_STEP_CONTINUE},
        {"children remain, not exited", 0,  0,      REAP_STEP_SLEEP,    REAP_STEP_DEADLINE},
        {"ECHILD: nothing left",        -1, ECHILD, REAP_STEP_DONE,     REAP_STEP_DONE},
        {"EINTR: retry within deadline",-1, EINTR,  REAP_STEP_SLEEP,    REAP_STEP_DEADLINE},
        {"EIO: other waitpid error",    -1, EIO,    REAP_STEP_ENUM,     REAP_STEP_ENUM},
    };
    for (size_t i = 0; i < sizeof steps / sizeof steps[0]; i++)
        for (int past = 0; past <= 1; past++) {
            int want = past ? steps[i].expect_past1 : steps[i].expect_past0;
            int got = reap_step(steps[i].got, steps[i].err, past);
            int ok = got == want;
            cells++;
            if (!ok) fails++;
            printf("%s reap_step %-30s past=%d -> action=%d\n", ok ? "PASS" : "FAIL", steps[i].label, past, got);
        }
    /* ECHILD is the only clean end: no other error may map to DONE. */
    int only_echild_done = reap_step(-1, EIO, 0) != REAP_STEP_DONE && reap_step(-1, EINTR, 1) != REAP_STEP_DONE;
    if (!only_echild_done) fails++;
    printf("%s reap_step: only ECHILD ends cleanly\n", only_echild_done ? "PASS" : "FAIL");
    printf("SUMMARY reason matrix: %d cells, %s (%d failing)\n", cells, fails ? "FAIL" : "PASS", fails);
    return fails ? 1 : 0;
}
