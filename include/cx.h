/* cx.h -- 0.0.34 X1: the small toolkit .cx scripts (#!/usr/bin/env unisacc.com -run) need to replace
 * .sh/.py helpers: run a command and take its output, read/write a whole file, sha256, and a command
 * under a time limit.  Plain C99 over the carried libc; POSIX targets (Windows has no fork; the
 * .cx pilot runs on Linux and macOS; on Windows cx_run/cx_run_timeout return -1).  Every body sits alone in its own guard (tests/libneed.sh). */
#ifndef _UNISA_CX_H
#define _UNISA_CX_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#endif

static char *cx_read(const char *__u_path, size_t *__u_len);
static int cx_write(const char *__u_path, const void *__u_buf, size_t __u_len);
static int cx_run(const char *__u_cmd, char **__u_out, size_t *__u_len);
static int cx_run_timeout(const char *__u_cmd, int __u_secs, char **__u_out, size_t *__u_len);
static void cx_sha256(const void *__u_buf, size_t __u_len, unsigned char __u_md[32]);
static void cx_sha256_hex(const void *__u_buf, size_t __u_len, char __u_hex[65]);

/* the whole stream into a malloc'd, NUL-terminated buffer; NULL on error */
#if !__UNISA_FTRIM_LIBC || __UN__cx_slurp
static char *_cx_slurp(FILE *__u_f, size_t *__u_len) {
    size_t __u_n = 0, __u_cap = 4096, __u_k; char *__u_b = malloc(__u_cap), *__u_t;
    if (!__u_b) return 0;
    while ((__u_k = fread(__u_b + __u_n, 1, __u_cap - __u_n - 1, __u_f)) > 0) {
        __u_n += __u_k;
        if (__u_cap - __u_n < 2) {
            __u_t = realloc(__u_b, __u_cap * 2); if (!__u_t) { free(__u_b); return 0; }
            __u_b = __u_t; __u_cap *= 2;
        }
    }
    __u_b[__u_n] = 0; if (__u_len) *__u_len = __u_n; return __u_b;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_cx_read
static char *cx_read(const char *__u_path, size_t *__u_len) {
    FILE *__u_f = fopen(__u_path, "rb"); char *__u_b;
    if (!__u_f) return 0;
    __u_b = _cx_slurp(__u_f, __u_len); fclose(__u_f); return __u_b;
}
#endif

/* 0 on success, -1 when the file cannot be written in full */
#if !__UNISA_FTRIM_LIBC || __UN_cx_write
static int cx_write(const char *__u_path, const void *__u_buf, size_t __u_len) {
    FILE *__u_f = fopen(__u_path, "wb"); int __u_ok;
    if (!__u_f) return -1;
    __u_ok = fwrite(__u_buf, 1, __u_len, __u_f) == __u_len;
    if (fclose(__u_f) != 0) __u_ok = 0;
    return __u_ok ? 0 : -1;
}
#endif

/* /bin/sh -c CMD; its stdout into *out (malloc'd; may be NULL to discard).  Returns the exit
   code (0..255), 128+N for a child killed by signal N, -1 when it could not be started. */
#if !__UNISA_FTRIM_LIBC || __UN_cx_run
static int cx_run(const char *__u_cmd, char **__u_out, size_t *__u_len) {
#ifdef _WIN32
    (void)__u_cmd; if (__u_out) *__u_out = 0; if (__u_len) *__u_len = 0; return -1;   /* no popen yet */
#else
    FILE *__u_f = popen(__u_cmd, "r"); char *__u_b; int __u_st;
    if (!__u_f) return -1;
    __u_b = _cx_slurp(__u_f, __u_len);
    __u_st = pclose(__u_f);
    if (__u_out) *__u_out = __u_b; else free(__u_b);
    if (__u_st == -1) return -1;
    return (__u_st & 0x7f) ? 128 + (__u_st & 0x7f) : (__u_st >> 8) & 0xff;
#endif
}
#endif

/* cx_run with a limit: past SECS seconds the child's process group gets SIGKILL and the
   result is 124 (as timeout(1)).  The output goes through a temporary file, so a child that
   fills a pipe cannot stall the wait. */
#if !__UNISA_FTRIM_LIBC || __UN_cx_run_timeout
static int cx_run_timeout(const char *__u_cmd, int __u_secs, char **__u_out, size_t *__u_len) {
#ifdef _WIN32
    (void)__u_secs; return cx_run(__u_cmd, __u_out, __u_len);
#else
    char __u_tmp[] = "/tmp/cxrunXXXXXX"; int __u_fd = mkstemp(__u_tmp), __u_st = 0, __u_rc;
    long __u_ticks = (long)__u_secs * 100; pid_t __u_pid, __u_w; FILE *__u_f;
    if (__u_fd < 0) return -1;
    __u_pid = fork();
    if (__u_pid < 0) { close(__u_fd); unlink(__u_tmp); return -1; }
    if (__u_pid == 0) {
        setpgid(0, 0); dup2(__u_fd, 1); close(__u_fd);
        execl("/bin/sh", "sh", "-c", __u_cmd, (char *)0); _exit(127);
    }
    close(__u_fd);
    for (;;) {
        __u_w = waitpid(__u_pid, &__u_st, WNOHANG);
        if (__u_w == __u_pid) { __u_rc = (__u_st & 0x7f) ? 128 + (__u_st & 0x7f) : (__u_st >> 8) & 0xff; break; }
        if (__u_w < 0) { __u_rc = -1; break; }
        if (__u_ticks-- <= 0) { kill(-__u_pid, SIGKILL); kill(__u_pid, SIGKILL); waitpid(__u_pid, &__u_st, 0); __u_rc = 124; break; }
        usleep(10000);
    }
    __u_f = fopen(__u_tmp, "rb");
    if (__u_f) {
        char *__u_b = _cx_slurp(__u_f, __u_len); fclose(__u_f);
        if (__u_out) *__u_out = __u_b; else free(__u_b);
    } else if (__u_out) *__u_out = 0;
    unlink(__u_tmp);
    return __u_rc;
#endif
}
#endif

/* FIPS 180-4 SHA-256 */
#if !__UNISA_FTRIM_LIBC || __UN_cx_sha256
static void cx_sha256(const void *__u_buf, size_t __u_len, unsigned char __u_md[32]) {
    static const unsigned long __u_k[64] = {
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
    unsigned long __u_h[8] = {0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    const unsigned char *__u_p = (const unsigned char *)__u_buf;
    unsigned char __u_blk[64]; unsigned long __u_w[64], __u_a[8], __u_t1, __u_t2;
    size_t __u_off = 0, __u_total = __u_len + 9, __u_nblk = (__u_total + 63) / 64, __u_b;
    int __u_i, __u_j;
    for (__u_b = 0; __u_b < __u_nblk; __u_b++) {
        for (__u_i = 0; __u_i < 64; __u_i++, __u_off++) {
            if (__u_off < __u_len) __u_blk[__u_i] = __u_p[__u_off];
            else if (__u_off == __u_len) __u_blk[__u_i] = 0x80;
            else if (__u_off >= __u_nblk * 64 - 8)
                __u_blk[__u_i] = (unsigned char)(((unsigned long long)__u_len * 8) >> (8 * (__u_nblk * 64 - 1 - __u_off)));
            else __u_blk[__u_i] = 0;
        }
        for (__u_i = 0; __u_i < 16; __u_i++)
            __u_w[__u_i] = ((unsigned long)__u_blk[4*__u_i] << 24) | ((unsigned long)__u_blk[4*__u_i+1] << 16) |
                           ((unsigned long)__u_blk[4*__u_i+2] << 8) | __u_blk[4*__u_i+3];
#define _CX_R(x,n) ((((x) >> (n)) | ((x) << (32 - (n)))) & 0xffffffffUL)
        for (__u_i = 16; __u_i < 64; __u_i++) {
            unsigned long __u_s0 = _CX_R(__u_w[__u_i-15],7) ^ _CX_R(__u_w[__u_i-15],18) ^ (__u_w[__u_i-15] >> 3);
            unsigned long __u_s1 = _CX_R(__u_w[__u_i-2],17) ^ _CX_R(__u_w[__u_i-2],19) ^ (__u_w[__u_i-2] >> 10);
            __u_w[__u_i] = (__u_w[__u_i-16] + __u_s0 + __u_w[__u_i-7] + __u_s1) & 0xffffffffUL;
        }
        for (__u_j = 0; __u_j < 8; __u_j++) __u_a[__u_j] = __u_h[__u_j];
        for (__u_i = 0; __u_i < 64; __u_i++) {
            __u_t1 = (__u_a[7] + (_CX_R(__u_a[4],6) ^ _CX_R(__u_a[4],11) ^ _CX_R(__u_a[4],25)) +
                      ((__u_a[4] & __u_a[5]) ^ (~__u_a[4] & __u_a[6])) + __u_k[__u_i] + __u_w[__u_i]) & 0xffffffffUL;
            __u_t2 = ((_CX_R(__u_a[0],2) ^ _CX_R(__u_a[0],13) ^ _CX_R(__u_a[0],22)) +
                      ((__u_a[0] & __u_a[1]) ^ (__u_a[0] & __u_a[2]) ^ (__u_a[1] & __u_a[2]))) & 0xffffffffUL;
            for (__u_j = 7; __u_j > 0; __u_j--) __u_a[__u_j] = __u_a[__u_j-1];
            __u_a[4] = (__u_a[4] + __u_t1) & 0xffffffffUL;
            __u_a[0] = (__u_t1 + __u_t2) & 0xffffffffUL;
        }
#undef _CX_R
        for (__u_j = 0; __u_j < 8; __u_j++) __u_h[__u_j] = (__u_h[__u_j] + __u_a[__u_j]) & 0xffffffffUL;
    }
    for (__u_j = 0; __u_j < 8; __u_j++) {
        __u_md[4*__u_j] = (unsigned char)(__u_h[__u_j] >> 24); __u_md[4*__u_j+1] = (unsigned char)(__u_h[__u_j] >> 16);
        __u_md[4*__u_j+2] = (unsigned char)(__u_h[__u_j] >> 8); __u_md[4*__u_j+3] = (unsigned char)__u_h[__u_j];
    }
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_cx_sha256_hex
static void cx_sha256_hex(const void *__u_buf, size_t __u_len, char __u_hex[65]) {
    unsigned char __u_md[32]; int __u_i;
    cx_sha256(__u_buf, __u_len, __u_md);
    for (__u_i = 0; __u_i < 32; __u_i++) {
        __u_hex[2*__u_i] = "0123456789abcdef"[__u_md[__u_i] >> 4];
        __u_hex[2*__u_i+1] = "0123456789abcdef"[__u_md[__u_i] & 15];
    }
    __u_hex[64] = 0;
}
#endif
#endif
