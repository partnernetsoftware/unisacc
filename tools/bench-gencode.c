/* tools/bench-gencode.c -- 生成代码运行时性能的被测对象 (C44-SPEED-01 杠杆 5).
 *
 * 同一份源分别用 unisacc、gcc -O0、gcc -O2 编译, 三档对照才公平:
 * sha256sum 用了 SIMD/内建指令, 单拿它比会把"没优化"误读成"代码生成差"。
 *
 *   ./bench-gencode SIZE_MB WORKLOAD
 *
 * WORKLOAD: sha | sort | memcpy | all
 * 输出 (stdout, 一行):
 *   RESULT <workload> <checksum_hex> <bytes> <ms>
 * checksum 是三档编译器之间唯一必须逐字节相同的字段; ms 由源内计时给出。
 * 数据用固定种子生成, 不读文件, 三个 workload 互不影响。 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

static uint32_t bg_x = 2463534242u;
static uint32_t bg_next(void) {
    bg_x ^= bg_x << 13; bg_x ^= bg_x >> 17; bg_x ^= bg_x << 5;
    return bg_x;
}

static long bg_now_ns(void) {
    struct timespec t;
    if (clock_gettime(CLOCK_MONOTONIC, &t) != 0) return -1;
    return (long)t.tv_sec * 1000000000L + t.tv_nsec;
}

/* ---- minimal SHA-256 (FIPS 180-4), pure C, no intrinsics ---- */
static const uint32_t bg_k[64] = {
    0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
    0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
    0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
    0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
    0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
    0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
    0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
    0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u
};
#define BG_ROR(x,n) (((x) >> (n)) | ((x) << (32 - (n))))
static void bg_block(uint32_t h[8], const unsigned char *p) {
    uint32_t w[64], a, b, c, d, e, f, g, k, t1, t2;
    int i;
    for (i = 0; i < 16; i++)
        w[i] = ((uint32_t)p[4*i] << 24) | ((uint32_t)p[4*i+1] << 16) | ((uint32_t)p[4*i+2] << 8) | (uint32_t)p[4*i+3];
    for (i = 16; i < 64; i++) {
        uint32_t s0 = BG_ROR(w[i-15],7) ^ BG_ROR(w[i-15],18) ^ (w[i-15] >> 3);
        uint32_t s1 = BG_ROR(w[i-2],17) ^ BG_ROR(w[i-2],19) ^ (w[i-2] >> 10);
        w[i] = w[i-16] + s0 + w[i-7] + s1;
    }
    a=h[0]; b=h[1]; c=h[2]; d=h[3]; e=h[4]; f=h[5]; g=h[6]; k=h[7];
    for (i = 0; i < 64; i++) {
        uint32_t S1 = BG_ROR(e,6) ^ BG_ROR(e,11) ^ BG_ROR(e,25);
        uint32_t ch = (e & f) ^ ((~e) & g);
        t1 = k + S1 + ch + bg_k[i] + w[i];
        t2 = (BG_ROR(a,2) ^ BG_ROR(a,13) ^ BG_ROR(a,22)) + ((a & b) ^ (a & c) ^ (b & c));
        k=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
    }
    h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d; h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=k;
}
static void bg_sha256(const unsigned char *p, long n, char hex[65]) {
    uint32_t h[8] = {0x6a09e667u,0xbb67ae85u,0x3c6ef372u,0xa54ff53au,0x510e527fu,0x9b05688cu,0x1f83d9abu,0x5be0cd19u};
    static const char hx[] = "0123456789abcdef";
    unsigned char tail[128];
    long i = 0, left, tl;
    while (n - i >= 64) { bg_block(h, p + i); i += 64; }
    left = n - i;
    memcpy(tail, p + i, (size_t)left);
    tail[left] = 0x80;
    tl = left + 1 < 56 ? 64 : 128;
    memset(tail + left + 1, 0, (size_t)(tl - left - 1));
    {
        unsigned long long bits = (unsigned long long)n * 8u;
        int j;
        for (j = 0; j < 8; j++) tail[tl - 1 - j] = (unsigned char)(bits >> (8 * j));
    }
    bg_block(h, tail);
    if (tl == 128) bg_block(h, tail + 64);
    for (i = 0; i < 8; i++) {
        int j;
        for (j = 0; j < 4; j++) {
            unsigned char b = (unsigned char)(h[i] >> (24 - 8 * j));
            hex[(int)i * 8 + j * 2] = hx[b >> 4];
            hex[(int)i * 8 + j * 2 + 1] = hx[b & 15];
        }
    }
    hex[64] = 0;
}

static int bg_cmp(const void *a, const void *b) {
    int32_t x = *(const int32_t *)a, y = *(const int32_t *)b;
    return x < y ? -1 : (x > y ? 1 : 0);
}

int main(int argc, char **argv) {
    long mb = 8, i;
    const char *wl;
    unsigned char *buf, *dst;
    int32_t *arr;
    long bytes, nn;
    long long t0, t1;
    char hex[65];
    if (argc < 3) { fprintf(stderr, "usage: SIZE_MB sha|sort|memcpy|all\n"); return 2; }
    mb = atol(argv[1]);
    wl = argv[2];
    if (mb < 1) mb = 8;
    bytes = mb * 1048576L;
    nn = bytes / 4;
    buf = (unsigned char *)malloc((size_t)bytes);
    dst = (unsigned char *)malloc((size_t)bytes);
    arr = (int32_t *)malloc((size_t)nn * sizeof(int32_t));
    if (!buf || !dst || !arr) return 1;
    for (i = 0; i < bytes; i++) buf[i] = (unsigned char)bg_next();
    for (i = 0; i < nn; i++) arr[i] = (int32_t)bg_next();
    if (!strcmp(wl, "sha") || !strcmp(wl, "all")) {
        t0 = bg_now_ns();
        bg_sha256(buf, bytes, hex);
        t1 = bg_now_ns();
        printf("RESULT sha %s %ld %.3f\n", hex, bytes, (double)(t1 - t0) / 1e6);
    }
    if (!strcmp(wl, "sort") || !strcmp(wl, "all")) {
        t0 = bg_now_ns();
        qsort(arr, (size_t)nn, sizeof(int32_t), bg_cmp);
        t1 = bg_now_ns();
        {
            unsigned long long sum = 0;
            for (i = 0; i < nn; i++) sum += (unsigned long long)(arr[i] & 0xffff);
            printf("RESULT sort %016llx %ld %.3f\n", sum, nn, (double)(t1 - t0) / 1e6);
        }
    }
    if (!strcmp(wl, "memcpy") || !strcmp(wl, "all")) {
        t0 = bg_now_ns();
        memcpy(dst, buf, (size_t)bytes);
        t1 = bg_now_ns();
        {
            unsigned long long sum = 0;
            for (i = 0; i < bytes; i += 4096) sum += dst[i];
            printf("RESULT memcpy %016llx %ld %.3f\n", sum, bytes, (double)(t1 - t0) / 1e6);
        }
    }
    return 0;
}
