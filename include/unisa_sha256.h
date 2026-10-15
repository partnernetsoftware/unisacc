#ifndef UNISA_SHA256_H
#define UNISA_SHA256_H
/* sha256.h: FIPS 180-4 SHA-256, the one implementation for the product runtime (exec/c) and for include/cx.h.
   Allocation-free (cx.h is compiled by the product's trimmed libc): full blocks are read from the input, the padded
   tail is at most two blocks on the stack.  Byte output unisa_sha256, hex output unisa_sha256_hex.
   Checked against the standard vectors (empty, abc, million-a) in both the cx and product paths. */
static unsigned long unisa_rotr(unsigned long x, int k) { return ((x >> k) | (x << (32 - k))) & 0xffffffffUL; }
static void unisa_sha256_block(unsigned long h[8], const unsigned char *b) {
    static const unsigned long k[64] = {
        0x428a2f98UL,0x71374491UL,0xb5c0fbcfUL,0xe9b5dba5UL,0x3956c25bUL,0x59f111f1UL,0x923f82a4UL,0xab1c5ed5UL,
        0xd807aa98UL,0x12835b01UL,0x243185beUL,0x550c7dc3UL,0x72be5d74UL,0x80deb1feUL,0x9bdc06a7UL,0xc19bf174UL,
        0xe49b69c1UL,0xefbe4786UL,0x0fc19dc6UL,0x240ca1ccUL,0x2de92c6fUL,0x4a7484aaUL,0x5cb0a9dcUL,0x76f988daUL,
        0x983e5152UL,0xa831c66dUL,0xb00327c8UL,0xbf597fc7UL,0xc6e00bf3UL,0xd5a79147UL,0x06ca6351UL,0x14292967UL,
        0x27b70a85UL,0x2e1b2138UL,0x4d2c6dfcUL,0x53380d13UL,0x650a7354UL,0x766a0abbUL,0x81c2c92eUL,0x92722c85UL,
        0xa2bfe8a1UL,0xa81a664bUL,0xc24b8b70UL,0xc76c51a3UL,0xd192e819UL,0xd6990624UL,0xf40e3585UL,0x106aa070UL,
        0x19a4c116UL,0x1e376c08UL,0x2748774cUL,0x34b0bcb5UL,0x391c0cb3UL,0x4ed8aa4aUL,0x5b9cca4fUL,0x682e6ff3UL,
        0x748f82eeUL,0x78a5636fUL,0x84c87814UL,0x8cc70208UL,0x90befffaUL,0xa4506cebUL,0xbef9a3f7UL,0xc67178f2UL};
    unsigned long w[64], a, bb, c, d, e, f, g, hh, t1, t2, s0, s1, S0, S1, mj, ch;
    int i;
    for (i = 0; i < 16; i++) w[i] = ((unsigned long)b[4*i] << 24) | ((unsigned long)b[4*i+1] << 16) | ((unsigned long)b[4*i+2] << 8) | b[4*i+3];
    for (i = 16; i < 64; i++) {
        s0 = unisa_rotr(w[i-15], 7) ^ unisa_rotr(w[i-15], 18) ^ (w[i-15] >> 3);
        s1 = unisa_rotr(w[i-2], 17) ^ unisa_rotr(w[i-2], 19) ^ (w[i-2] >> 10);
        w[i] = (w[i-16] + s0 + w[i-7] + s1) & 0xffffffffUL;
    }
    a = h[0]; bb = h[1]; c = h[2]; d = h[3]; e = h[4]; f = h[5]; g = h[6]; hh = h[7];
    for (i = 0; i < 64; i++) {
        S1 = unisa_rotr(e, 6) ^ unisa_rotr(e, 11) ^ unisa_rotr(e, 25);
        ch = (e & f) ^ ((~e) & 0xffffffffUL & g);
        t1 = (hh + S1 + ch + k[i] + w[i]) & 0xffffffffUL;
        S0 = unisa_rotr(a, 2) ^ unisa_rotr(a, 13) ^ unisa_rotr(a, 22);
        mj = (a & bb) ^ (a & c) ^ (bb & c);
        t2 = (S0 + mj) & 0xffffffffUL;
        hh = g; g = f; f = e; e = (d + t1) & 0xffffffffUL; d = c; c = bb; bb = a; a = (t1 + t2) & 0xffffffffUL;
    }
    h[0] = (h[0] + a) & 0xffffffffUL; h[1] = (h[1] + bb) & 0xffffffffUL; h[2] = (h[2] + c) & 0xffffffffUL; h[3] = (h[3] + d) & 0xffffffffUL;
    h[4] = (h[4] + e) & 0xffffffffUL; h[5] = (h[5] + f) & 0xffffffffUL; h[6] = (h[6] + g) & 0xffffffffUL; h[7] = (h[7] + hh) & 0xffffffffUL;
}
static void unisa_sha256(const void *in, size_t len, unsigned char md[32]) {
    const unsigned char *msg = (const unsigned char *)in;
    unsigned long h[8] = {0x6a09e667UL,0xbb67ae85UL,0x3c6ef372UL,0xa54ff53aUL,0x510e527fUL,0x9b05688cUL,0x1f83d9abUL,0x5be0cd19UL};
    unsigned char tail[128];
    size_t full = len / 64, rest = len % 64, off, j, nt;
    unsigned long long bits = (unsigned long long)len * 8ULL;
    int i;
    for (off = 0; off < full; off++) unisa_sha256_block(h, msg + 64 * off);
    for (j = 0; j < 128; j++) tail[j] = 0;
    for (j = 0; j < rest; j++) tail[j] = msg[64 * full + j];
    tail[rest] = 0x80;
    nt = (rest + 9 <= 64) ? 64 : 128;
    for (j = 0; j < 8; j++) tail[nt - 1 - j] = (unsigned char)(bits >> (8 * j));
    unisa_sha256_block(h, tail);
    if (nt == 128) unisa_sha256_block(h, tail + 64);
    for (i = 0; i < 8; i++) { md[4*i] = (unsigned char)(h[i] >> 24); md[4*i+1] = (unsigned char)(h[i] >> 16); md[4*i+2] = (unsigned char)(h[i] >> 8); md[4*i+3] = (unsigned char)h[i]; }
}
static void unisa_sha256_hex(const void *in, size_t len, char hex[65]) {
    unsigned char md[32]; int i;
    unisa_sha256(in, len, md);
    for (i = 0; i < 32; i++) { hex[2*i] = "0123456789abcdef"[md[i] >> 4]; hex[2*i+1] = "0123456789abcdef"[md[i] & 15]; }
    hex[64] = 0;
}
#endif
