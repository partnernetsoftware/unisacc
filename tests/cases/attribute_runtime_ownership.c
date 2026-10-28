/* Verify that the library's attribute frame allocation participates in the
 * per-call cleanup domain. This intentionally includes the host adapter so
 * the test can observe its private allocation ledger. */
#include "exec/c/libunisacc.c"

int main(void) {
    unsigned char frame[16] = {'U','S','L','A','T','T','R','1'};
    Buf input = {0};
    input.b = frame;
    input.n = sizeof frame;
    ATTR_SLOT = -1;
    if (attr_strip(&input) != 1 || ATTRS.n != 4 || input.n != 0) return 2;
    if (!allocations || !*allocation_slot(ATTRS.b)) {
        /* Do not hide a failed check by asking tracked_free to free an
         * untracked allocation. */
        free(ATTRS.b);
        ATTRS.b = 0;
        cleanup();
        fputs("FAIL attribute allocation untracked\n", stderr);
        return 1;
    }
    cleanup();
    ATTRS.b = 0; ATTRS.n = 0; ATTRS.cap = 0;
    if (allocations) return 2;
    puts("PASS attribute allocation tracked and cleaned");
    return 0;
}
