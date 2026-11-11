/* L1b sys6 acceptance probe (POSIX targets).  Every __syscall6 here is a write(1, buf, n) whose
 * six sources (number + five arguments) live in six locals; each case feeds them to the gate in a
 * different order of declaration, so the lowering sees the sources in permuted registers.  buf is a
 * frame-relative address (the original r7 frame) and the stray arguments carry sentinels that must
 * not reach the call.  NAMED cases spell the number as a constant, DYN cases load it through a
 * volatile so the gate gets a dynamic number.  Output: one line per case, then "sys6 ok N".
 * Expected stdout is fixed (expect.txt); exit status 0. */
#if defined(__APPLE__)
#  if defined(__x86_64__)
#    define NR_WRITE (0x2000000L + 4)
#  else
#    define NR_WRITE 4L
#  endif
#elif defined(__x86_64__)
#  define NR_WRITE 1L
#else
#  define NR_WRITE 64L
#endif

static volatile long vnr = NR_WRITE;
static int bad;

static void fill(char *b, int k) {
    char *t = "case ?\n"; int i;
    for (i = 0; t[i]; i++) b[i] = t[i];
    b[5] = (char)('a' + k);
}

static void check(long r, int k) { if (r != 7) { bad++; (void)k; } }

/* NAMED: constant number; the five arguments come from locals declared in six different orders */
static void named(void) {
    char buf[16];
    { long a = 1, b = (long)buf, c = 7, d = 0x5a5a, e = -1; fill(buf, 0); check(__syscall6(NR_WRITE, a, b, c, d, e), 0); }
    { long e = -1, d = 0x5a5a, c = 7, b = (long)buf, a = 1; fill(buf, 1); check(__syscall6(NR_WRITE, a, b, c, d, e), 1); }
    { long c = 7, a = 1, e = -1, b = (long)buf, d = 0x5a5a; fill(buf, 2); check(__syscall6(NR_WRITE, a, b, c, d, e), 2); }
    { long b = (long)buf, e = -1, a = 1, d = 0x5a5a, c = 7; fill(buf, 3); check(__syscall6(NR_WRITE, a, b, c, d, e), 3); }
    { long d = 0x5a5a, c = 7, b = (long)buf, e = -1, a = 1; fill(buf, 4); check(__syscall6(NR_WRITE, a, b, c, d, e), 4); }
    /* arguments are expressions over the frame address itself */
    { fill(buf, 5); check(__syscall6(NR_WRITE, 1, (long)&buf[0], 7, (long)&buf[8], (long)buf - 1), 5); }
}

/* DYN: the number is a load, and also sits in a permuted position among the sources */
static void dyn(void) {
    char buf[16];
    { long n = vnr, a = 1, b = (long)buf, c = 7, d = 0x5a5a, e = -1; fill(buf, 6); check(__syscall6(n, a, b, c, d, e), 6); }
    { long e = -1, d = 0x5a5a, c = 7, b = (long)buf, a = 1, n = vnr; fill(buf, 7); check(__syscall6(n, a, b, c, d, e), 7); }
    { long b = (long)buf, n = vnr, d = 0x5a5a, a = 1, e = -1, c = 7; fill(buf, 8); check(__syscall6(n, a, b, c, d, e), 8); }
    { long c = 7, e = -1, n = vnr, b = (long)buf, a = 1, d = 0x5a5a; fill(buf, 9); check(__syscall6(n, a, b, c, d, e), 9); }
    { long a = 1, d = 0x5a5a, b = (long)buf, n = vnr, c = 7, e = -1; fill(buf, 10); check(__syscall6(n, a, b, c, d, e), 10); }
    { fill(buf, 11); check(__syscall6(vnr, 1, (long)buf, 7, (long)&buf[8], (long)buf - 1), 11); }
}

int main(void) {
    char ok[] = "sys6 ok 12\n", no[] = "sys6 BAD\n";
    named(); dyn();
    if (bad) { __syscall6(NR_WRITE, 1, (long)no, 9, 0, 0); return 1; }
    __syscall6(NR_WRITE, 1, (long)ok, 11, 0, 0);
    return 0;
}
