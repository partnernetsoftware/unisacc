/* C99 6.5.17p2: evaluating the left operand reads a volatile lvalue.
   Tape parity alone cannot prove this: both compiler routes currently omit
   the read.  See the system-cc assembly evidence in the A3 handoff. */
volatile int observed = 7;
int ordinary = 7;
int main(void) {
    (observed, 0);
    (ordinary, 0);
    return 0;
}
