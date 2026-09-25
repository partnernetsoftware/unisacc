/* fwrite probes, SIMULATED: __write is replaced by a scripted shim so the
   partial, zero and error answers can be forced.  The shim writes nothing;
   it records calls and answers from a script.  Exit status = failures. */
static long shim_ans[8];
static long shim_k;
static long shim_calls;
static long shim(int fd, char *p, long n) {
    long r;
    shim_calls = shim_calls + 1;
    r = shim_ans[shim_k];
    shim_k = shim_k + 1;
    if (r == 99999) return n;          /* "all of it" */
    return r;
}
#define __write shim
#include <stdio.h>
#undef __write
static char buf[64];
static int fails;
static void set(long a, long b, long c) {
    shim_ans[0] = a; shim_ans[1] = b; shim_ans[2] = c;
    shim_k = 0; shim_calls = 0;
}
static void check(char *name, long got, long want, long wantcalls) {
    long calls;
    calls = shim_calls;
    if (got != want || calls != wantcalls) { fails = fails + 1; printf("FAIL "); }
    else printf("ok ");
    printf("%s ret=%ld want=%ld calls=%ld want=%ld\n", name, got, want, calls, wantcalls);
}
int main(void) {
    long r;
    set(99999, 99999, 99999); r = fwrite(buf, 0, 5, stdout);
    check("sz0", r, 0, 0);
    set(99999, 99999, 99999); r = fwrite(buf, 4, 0, stdout);
    check("n0", r, 0, 0);
    set(99999, 99999, 99999); r = fwrite(buf, 4, 5, stdout);
    check("complete", r, 5, 1);
    set(7, 99999, 99999); r = fwrite(buf, 4, 5, stdout);
    check("short_then_ok", r, 5, 2);
    set(7, 0 - 28, 99999); r = fwrite(buf, 4, 5, stdout);
    check("short_then_err", r, 1, 2);
    set(0, 99999, 99999); r = fwrite(buf, 4, 5, stdout);
    check("zero_stops", r, 0, 1);
    set(9, 0, 99999); r = fwrite(buf, 4, 5, stdout);
    check("round_down", r, 2, 2);
    set(99999, 99999, 99999); r = fwrite(buf, 0x4000000000000000, 4, stdout);
    check("overflow", r, 0, 0);
    set(99999, 99999, 99999); r = fwrite(buf, 0 - 1, 4, stdout);
    check("negative_sz", r, 0, 0);
    set(99999, 99999, 99999); r = fwrite(buf, 4, 0 - 1, stdout);
    check("negative_n", r, 0, 0);
    set(500, 99999, 99999); r = fwrite(buf, 4, 5, stdout);
    check("over_answer", r, 0, 1);
    return fails;
}
