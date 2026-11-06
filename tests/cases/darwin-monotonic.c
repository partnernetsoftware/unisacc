/* Mac-only candidate smoke probe; compiled and run by candidate-qualify.cx. */
#include <stdio.h>
#include <time.h>
#include <errno.h>

int main(void) {
    struct timespec before, after, delay = {0, 50000000};
    long long delta;
    int invalid_result;
    errno = 0;
    invalid_result = clock_gettime((clockid_t)-1, &before);
    if (invalid_result != -1 || errno != EINVAL) {
        printf("CLOCK_MONOTONIC\tFAIL\tinvalid clockid result=%d errno=%d expected_result=-1 expected_errno=%d\n",
            invalid_result, errno, EINVAL);
        return 1;
    }
    puts("CLOCK_MONOTONIC\tPASS\tinvalid clockid returns -1 and EINVAL");
    if (clock_gettime(CLOCK_MONOTONIC, &before)) {
        puts("CLOCK_MONOTONIC\tFAIL\tfirst read");
        return 1;
    }
    if (nanosleep(&delay, 0)) {
        puts("CLOCK_MONOTONIC\tFAIL\tnanosleep50ms");
        return 1;
    }
    if (clock_gettime(CLOCK_MONOTONIC, &after)) {
        puts("CLOCK_MONOTONIC\tFAIL\tsecond read");
        return 1;
    }
    delta = ((long long)after.tv_sec - (long long)before.tv_sec) * 1000000000LL
        + (long long)after.tv_nsec - (long long)before.tv_nsec;
    printf("CLOCK_MONOTONIC\t%s\tdelta_ns=%lld\tnanosleep_ms=50\trange_ms=[50,500]\n",
        delta >= 50000000LL && delta <= 500000000LL ? "PASS" : "FAIL", delta);
    return delta >= 0 && delta >= 50000000LL && delta <= 500000000LL ? 0 : 1;
}
