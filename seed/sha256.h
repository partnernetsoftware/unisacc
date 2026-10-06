/* seed/sha256.h -- SHA-256 for the seed tools (ident.c, compilerpack.c); C99. */
#include <stdint.h>
#include <string.h>
typedef struct { uint32_t s[8]; uint64_t n; unsigned char b[64]; size_t k; } Sha;
static const uint32_t K[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
#define ROR(x, r) (((x) >> (r)) | ((x) << (32 - (r))))
static void sha_block(Sha *h, const unsigned char *p) {
    uint32_t w[64], a, b, c, d, e, f, g, x, t1, t2; int i;
    for (i = 0; i < 16; i++)
        w[i] = (uint32_t)p[4*i] << 24 | (uint32_t)p[4*i+1] << 16 | (uint32_t)p[4*i+2] << 8 | p[4*i+3];
    for (i = 16; i < 64; i++)
        w[i] = w[i-16] + (ROR(w[i-15], 7) ^ ROR(w[i-15], 18) ^ (w[i-15] >> 3)) + w[i-7]
             + (ROR(w[i-2], 17) ^ ROR(w[i-2], 19) ^ (w[i-2] >> 10));
    a = h->s[0]; b = h->s[1]; c = h->s[2]; d = h->s[3]; e = h->s[4]; f = h->s[5]; g = h->s[6]; x = h->s[7];
    for (i = 0; i < 64; i++) {
        t1 = x + (ROR(e, 6) ^ ROR(e, 11) ^ ROR(e, 25)) + ((e & f) ^ (~e & g)) + K[i] + w[i];
        t2 = (ROR(a, 2) ^ ROR(a, 13) ^ ROR(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
        x = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
    }
    h->s[0] += a; h->s[1] += b; h->s[2] += c; h->s[3] += d; h->s[4] += e; h->s[5] += f; h->s[6] += g; h->s[7] += x;
}
static void sha_init(Sha *h) {
    static const uint32_t iv[8] = {0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    memcpy(h->s, iv, sizeof iv); h->n = 0; h->k = 0;
}
static void sha_put(Sha *h, const void *data, size_t n) {
    const unsigned char *p = data;
    h->n += n;
    while (n) {
        size_t m = 64 - h->k; if (m > n) m = n;
        memcpy(h->b + h->k, p, m); h->k += m; p += m; n -= m;
        if (h->k == 64) { sha_block(h, h->b); h->k = 0; }
    }
}
static void sha_end(Sha *h, unsigned char out[32]) {
    uint64_t bits = h->n * 8; unsigned char pad = 0x80, z = 0, len[8]; int i;
    sha_put(h, &pad, 1);
    while (h->k != 56) sha_put(h, &z, 1);
    for (i = 0; i < 8; i++) len[i] = (unsigned char)(bits >> (56 - 8 * i));
    sha_put(h, len, 8);
    for (i = 0; i < 32; i++) out[i] = (unsigned char)(h->s[i / 4] >> (24 - 8 * (i % 4)));
}
