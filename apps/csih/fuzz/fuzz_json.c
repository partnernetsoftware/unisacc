/* ASan/UBSan fuzz driver for json.c (untrusted model replies). Test harness, not part of csih. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* json.h comes from -include; json.c is linked as its own unit (no source #include). */
static unsigned long long rng = 0x9e3779b97f4a7c15ULL;
static unsigned rnd(void) { rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17; return (unsigned)rng; }
static const char seed[] = "{\"a\":[1,-2.5e3,true,null,\"x\\u00e4\\n\"],\"b\":{\"c\":\"\\ud83d\\ude00\"}}";
int main(int argc, char **argv) {
    long iters = argc > 1 ? atol(argv[1]) : 200000, i, parsed = 0;
    char buf[512], errb[128], out[1024];
    for (i = 0; i < iters; i++) {
        size_t n = sizeof seed - 1, k, len;
        memcpy(buf, seed, n);
        for (k = (rnd() % 4 == 0) ? rnd() % 4 : 0; k > 0; k--) buf[rnd() % n] = (char)rnd();      /* mutate */
        len = rnd() % (n + 1);                                             /* truncate */
        if (rnd() % 4 == 0) len = rnd() % 480;                             /* random length */
        jvalue *v = json_parse(buf, len, errb, sizeof errb);
        if (v) { parsed++; jget(v, "a"); jstr(v); jnum(v, 0); jlen(v); jfree(v); }
        json_value_end(buf, len);
        { size_t used = 0; json_escape(buf, out, sizeof out, &used); }
    }
    printf("iters=%ld parsed_ok=%ld\n", i, parsed);
    return 0;
}
