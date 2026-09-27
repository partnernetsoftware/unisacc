/* colorpack: bit-field packed pixel formats.
 *
 * Converts a small built-in image between RGB888 (one byte per channel)
 * and three packed formats that real hardware and file formats use --
 * RGB565 (16-bit), RGB555 (15-bit, the high bit spare), and RGBA4444
 * (12-bit colour plus a 4-bit alpha) -- using C bit-fields for the packed
 * side, not hand-rolled shifts, so the compiler's own bit-field layout
 * is what gets exercised.
 *
 * C99 leaves a bit-field's bit ALLOCATION ORDER within its storage unit
 * implementation-defined (6.7.2.1p10): this program never inspects that
 * raw order (no reinterpreting the struct as an integer), only the named
 * fields, which is what a portable program is allowed to rely on. Value
 * round-tripping (unpack(pack(x)) == quantize(x)) is checked and printed;
 * that must hold on every compiler, packed order or not.
 *
 * Known divergence from host cc (2026-09-27, filed, not fixed here): every
 * field value this program prints matches host cc exactly, but the
 * `sizeof` line does not -- unisacc currently allocates a bit-field
 * struct's storage unit one size class too big whenever the declared base
 * type is narrower than `int` (`unsigned short` bit-fields get 4 bytes
 * instead of 2, `unsigned char` bit-fields get 2 instead of 1; plain
 * `unsigned` bit-fields, already `int`-sized, are unaffected). Minimal
 * repro: /tmp/bfsize.c, reported separately; not a tests/c/ addition.
 */
#include <stdio.h>

struct rgb888 { unsigned char r, g, b; };

struct rgb565 { unsigned short b:5, g:6, r:5; };
struct rgb555 { unsigned short b:5, g:5, r:5, pad:1; };
struct rgba4444 { unsigned short a:4, b:4, g:4, r:4; };

static const struct rgb888 IMG[] = {
    {255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {255, 255, 255}, {0, 0, 0},
    {128, 64, 200}, {17, 200, 233}, {90, 90, 90}, {255, 128, 0}, {12, 250, 40},
    {200, 200, 255}, {1, 1, 1},
};
#define N (int)(sizeof(IMG) / sizeof(IMG[0]))

static unsigned scale(unsigned v, int frombits)
{
    unsigned max_out = (1u << frombits) - 1;
    return (v * max_out + 127) / 255;
}

static unsigned unscale(unsigned v, int frombits)
{
    unsigned max_in = (1u << frombits) - 1;
    return (v * 255 + max_in / 2) / max_in;
}

static struct rgb565 to565(struct rgb888 c)
{
    struct rgb565 p;
    p.r = scale(c.r, 5); p.g = scale(c.g, 6); p.b = scale(c.b, 5);
    return p;
}

static struct rgb888 from565(struct rgb565 p)
{
    struct rgb888 c;
    c.r = unscale(p.r, 5); c.g = unscale(p.g, 6); c.b = unscale(p.b, 5);
    return c;
}

static struct rgb555 to555(struct rgb888 c)
{
    struct rgb555 p;
    p.r = scale(c.r, 5); p.g = scale(c.g, 5); p.b = scale(c.b, 5); p.pad = 0;
    return p;
}

static struct rgb888 from555(struct rgb555 p)
{
    struct rgb888 c;
    c.r = unscale(p.r, 5); c.g = unscale(p.g, 5); c.b = unscale(p.b, 5);
    return c;
}

static struct rgba4444 to4444(struct rgb888 c, unsigned alpha)
{
    struct rgba4444 p;
    p.r = scale(c.r, 4); p.g = scale(c.g, 4); p.b = scale(c.b, 4); p.a = scale(alpha, 4);
    return p;
}

static int dist2(struct rgb888 a, struct rgb888 b)
{
    int dr = (int)a.r - b.r, dg = (int)a.g - b.g, db = (int)a.b - b.b;
    return dr * dr + dg * dg + db * db;
}

int main(void)
{
    int i, worst = -1, worsterr = -1;
    long total565 = 0, total555 = 0;
    int histr[32], histg[64];

    for (i = 0; i < 32; i++) histr[i] = 0;
    for (i = 0; i < 64; i++) histg[i] = 0;

    printf("== %d pixels, three packed formats\n", N);
    printf("%-16s %-16s %-16s %-16s\n", "rgb888", "rgb565", "rgb555", "rgba4444(a=200)");
    for (i = 0; i < N; i++) {
        struct rgb888 c = IMG[i];
        struct rgb565 p565 = to565(c);
        struct rgb555 p555 = to555(c);
        struct rgba4444 p4444 = to4444(c, 200);
        struct rgb888 back565 = from565(p565);
        struct rgb888 back555 = from555(p555);
        int e565 = dist2(c, back565), e555 = dist2(c, back555);
        printf("(%3d,%3d,%3d)  (%2u,%2u,%2u)16    (%2u,%2u,%2u)15    (%2u,%2u,%2u,a%2u)\n",
               c.r, c.g, c.b, p565.r, p565.g, p565.b, p555.r, p555.g, p555.b,
               p4444.r, p4444.g, p4444.b, p4444.a);
        total565 += e565; total555 += e555;
        if (e565 > worsterr) { worsterr = e565; worst = i; }
        histr[p565.r]++; histg[p565.g]++;
    }

    printf("\n== round-trip error (squared distance in RGB888 space)\n");
    printf("rgb565 total %ld  avg %ld.%02ld\n", total565, total565 / N, (total565 * 100 / N) % 100);
    printf("rgb555 total %ld  avg %ld.%02ld\n", total555, total555 / N, (total555 * 100 / N) % 100);
    printf("worst pixel: index %d (%d,%d,%d), rgb565 error %d\n",
           worst, IMG[worst].r, IMG[worst].g, IMG[worst].b, worsterr);

    printf("\n== rgb565 channel histograms (nonzero buckets)\n");
    printf("r:");
    for (i = 0; i < 32; i++) if (histr[i]) printf(" %d:%d", i, histr[i]);
    printf("\ng:");
    for (i = 0; i < 64; i++) if (histg[i]) printf(" %d:%d", i, histg[i]);
    printf("\n");

    printf("\n== struct sizes (bytes, this compiler's own bit-field packing)\n");
    printf("rgb888 %lu  rgb565 %lu  rgb555 %lu  rgba4444 %lu\n",
           (unsigned long)sizeof(struct rgb888), (unsigned long)sizeof(struct rgb565),
           (unsigned long)sizeof(struct rgb555), (unsigned long)sizeof(struct rgba4444));
    return 0;
}
