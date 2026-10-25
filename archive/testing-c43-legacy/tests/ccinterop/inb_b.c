/* cc interop slice 2, inbound half B (compiled by cc, no libc) */
int twice(int x);
long sum6x(long a, long b, long c, long d, long e, long f);
char *skip(char *s, int k);
void store(int *p, int v);
unsigned char lowb(int x);
int depth(int n);
long drive(int k) {
    static char msg[] = "callback";
    int v = 0;
    switch (k) {
    case 0: return twice(-7) + twice(100000);
    case 1: return sum6x(1000000000000L, 2, 30, 4, 500, 6);
    case 2: store(&v, 5); return v * 1000 + (skip(msg, 4)[0]);
    case 3: return lowb(0x1234) + 1000 * lowb(-1);
    default: return 100 + depth(k - 4);
    }
}
