/*
 * clock_cli.c — driver for clock.c (owns `main`).
 *
 * Subcommands: `now` (print monotonic ms), `bench <n>` (time n gate_decide
 * calls, report per-decision latency in microseconds), `selftest`.
 *
 * gate.c is linked for `bench`; its types are restated here so this file
 * compiles on its own (unisacc can see an earlier file's type, but that is
 * order-dependent).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct { double threshold; } gate_config;
typedef struct { int cont; double p; int ok; char why[128]; } gate_decision;
gate_decision gate_decide(const char *reply, const gate_config *cfg);
long clock_now_ms(void);
long clock_now_us(void);

static int fails = 0;
static void expect(int c, const char *w) {
    if (!c) { printf("  FAIL %s\n", w); fails++; }
    else    printf("  ok   %s\n", w);
}

int selftest(void) {
    fails = 0;
    expect(clock_now_ms() > 0, "monotonic clock returns a positive ms");
    expect(clock_now_us() > 0, "monotonic clock returns a positive us");
    long a = clock_now_us(), b = clock_now_us();
    expect(b >= a, "clock is monotonic non-decreasing");
    printf("clock: %s\n", fails ? "FAILED" : "all cases pass");
    return fails ? 1 : 0;
}

/* Time n in-process gate_decide calls (the dominant per-decision cost) and
 * report mean + min latency in microseconds. This is the number the README
 * justifies the C layer with — measured by the tool itself, not inferred. */
int bench(int n) {
    gate_config cfg; cfg.threshold = 0.7;
    const char *reply = "0.83";
    long total0 = clock_now_us();
    long best = -1;
    for (int i = 0; i < n; i++) {
        long s = clock_now_us();
        volatile gate_decision d = gate_decide(reply, &cfg); (void)d;
        long e = clock_now_us();
        long dt = e - s;
        if (best < 0 || dt < best) best = dt;
    }
    long total1 = clock_now_us();
    long span = total1 - total0;
    double us_per = (double)span * 1000.0 / (double)n;  /* span is us */
    printf("bench: %d gate_decide calls\n", n);
    printf("  mean  %.3f us/decision\n", us_per);
    printf("  min   %ld us (coarse; clock is monotonic us)\n", best);
    printf("  total %ld us\n", span);
    return 0;
}

int main(int argc, char **argv) {
    if (argc >= 2 && strcmp(argv[1], "selftest") == 0) return selftest();
    if (argc >= 2 && strcmp(argv[1], "now") == 0) {
        printf("%ld\n", clock_now_ms());
        return 0;
    }
    if (argc >= 2 && strcmp(argv[1], "bench") == 0) {
        int n = (argc >= 3) ? atoi(argv[2]) : 200000;
        if (n <= 0) n = 200000;
        return bench(n);
    }
    printf("usage: clock [selftest|now|bench <n>]\n");
    return 2;
}
