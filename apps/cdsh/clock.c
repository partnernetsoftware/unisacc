/*
 * clock.c — real monotonic time, now that unisacc 0.0.20 (R20-6, 2026-10-02) 内环已接
 * provides `struct timespec` + `clock_gettime`. Before that cdsh had no real
 * clock (PRD §6 listed "no real timing" as a gap caused by the missing struct).
 *
 * LIBRARY, NO `main` (main is in clock_cli.c) — unisacc allows one `main` per
 * program (SKILL.md §2b).
 *
 * WHY A MODULE: timing was the one capability PRD §6 flagged as blocked by the
 * unisacc gap. Now unblocked, cdsh can measure its own decision latency with a
 * real clock instead of borrowing `poll`'s millisecond timeout as a stand-in.
 * One capability = one .c, same as gate/json/session.
 *
 * unisacc limits honoured (SKILL.md §2): types restated, no shared headers.
 */
#include <time.h>

/* Monotonic milliseconds since an arbitrary epoch (boot/process). NOT wall
 * clock, so safe for measuring durations. -1 on failure. */
long clock_now_ms(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return -1;
    return (long)(ts.tv_sec * 1000) + (long)(ts.tv_nsec / 1000000);
}

/* Same clock, microseconds. -1 on failure. */
long clock_now_us(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return -1;
    return (long)(ts.tv_sec * 1000000) + (long)(ts.tv_nsec / 1000);
}
