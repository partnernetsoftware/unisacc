/* ASan/UBSan fuzz driver for csih_message validate/decode (untrusted envelopes). Harness only. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* csih_message.h comes from -include in check.sh (no source #include). */
static unsigned long long rng = 0xdeadbeefcafef00dULL;
static unsigned rnd(void) { rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17; return (unsigned)rng; }
int main(int argc, char **argv) {
    long iters = argc > 1 ? atol(argv[1]) : 200000, i, ok = 0;
    static const char seed[] = "{\"version\":1,\"id\":\"0123456789abcdef0123456789abcdef\",\"kind\":\"task\",\"session\":\"csih2\",\"body\":\"hello \\u4e2d\"}";
    char buf[1024], why[256]; csih_message m;
    for (i = 0; i < iters; i++) {
        size_t n = sizeof seed - 1, len, k;
        memcpy(buf, seed, n);
        for (k = (rnd() % 3 == 0) ? rnd() % 4 : 0; k > 0; k--) buf[rnd() % n] = (char)(rnd() % 3 ? rnd() : 0);
        len = (rnd() % 4 == 0) ? rnd() % 1000 : n;
        csih_message_validate(buf, len, "csih2", why, sizeof why);
        if (csih_message_decode(buf, len, "csih2", &m, why, sizeof why) > 0) ok++;
    }
    printf("iters=%ld decoded_ok=%ld\n", i, ok);
    return 0;
}
