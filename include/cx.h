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
#include <dirent.h>
#include <sys/stat.h>
#endif

static char *cx_read(const char *__u_path, size_t *__u_len);
static int cx_write(const char *__u_path, const void *__u_buf, size_t __u_len);
static int cx_run(const char *__u_cmd, char **__u_out, size_t *__u_len);
static int cx_run_timeout(const char *__u_cmd, int __u_secs, char **__u_out, size_t *__u_len);
static void cx_sha256(const void *__u_buf, size_t __u_len, unsigned char __u_md[32]);
static void cx_sha256_hex(const void *__u_buf, size_t __u_len, char __u_hex[65]);
static int cx_runv_timeout(char *const __u_argv[], int __u_secs, char **__u_out, size_t *__u_len);
static int cx_walk(const char *__u_dir, int (*__u_fn)(const char *__u_path, void *__u_ctx), void *__u_ctx);

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

/* FIPS 180-4 SHA-256: the shared implementation lives in include/unisa_sha256.h (one copy for product and cx). */
#include "unisa_sha256.h"
#if !__UNISA_FTRIM_LIBC || __UN_cx_sha256
static void cx_sha256(const void *__u_buf, size_t __u_len, unsigned char __u_md[32]) { unisa_sha256(__u_buf, __u_len, __u_md); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_cx_sha256_hex
static void cx_sha256_hex(const void *__u_buf, size_t __u_len, char __u_hex[65]) { unisa_sha256_hex(__u_buf, __u_len, __u_hex); }
#endif
/* 0.0.35 K0: argv processes (no /bin/sh; cx_run/cx_run_timeout above still use sh and are
   being retired) and a directory walk. */
/* execvp ARGV (no shell); stdout into *out; SECS <= 0 means no limit.  Exit code, 128+N on signal N,
   124 on timeout (the whole process group is killed), 127 when ARGV[0] cannot be executed, -1 on error. */
#if !__UNISA_FTRIM_LIBC || __UN_cx_runv_timeout
static int cx_runv_timeout(char *const __u_argv[], int __u_secs, char **__u_out, size_t *__u_len) {
#ifdef _WIN32
    (void)__u_argv; (void)__u_secs; if (__u_out) *__u_out = 0; if (__u_len) *__u_len = 0; return -1;
#else
    char __u_tmp[] = "/tmp/cxrunXXXXXX"; int __u_fd = mkstemp(__u_tmp), __u_st = 0, __u_rc;
    long __u_ticks = (long)__u_secs * 100; pid_t __u_pid, __u_w; FILE *__u_f;
    if (__u_fd < 0) return -1;
    __u_pid = fork();
    if (__u_pid < 0) { close(__u_fd); unlink(__u_tmp); return -1; }
    if (__u_pid == 0) {
        setpgid(0, 0); dup2(__u_fd, 1); close(__u_fd);
        execvp(__u_argv[0], __u_argv); _exit(127);
    }
    close(__u_fd);
    for (;;) {
        __u_w = waitpid(__u_pid, &__u_st, WNOHANG);
        if (__u_w == __u_pid) { __u_rc = (__u_st & 0x7f) ? 128 + (__u_st & 0x7f) : (__u_st >> 8) & 0xff; break; }
        if (__u_w < 0) { __u_rc = -1; break; }
        if (__u_secs > 0 && __u_ticks-- <= 0) { kill(-__u_pid, SIGKILL); kill(__u_pid, SIGKILL); waitpid(__u_pid, &__u_st, 0); __u_rc = 124; break; }
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

/* Depth-first walk of DIR in name order; FN gets every regular file's path.  A nonzero FN result stops
   the walk and is returned; -1 when a directory cannot be read. */
#if !__UNISA_FTRIM_LIBC || __UN__cx_cmp
static int _cx_cmp(const void *__u_a, const void *__u_b) { return strcmp(*(char *const *)__u_a, *(char *const *)__u_b); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_cx_walk
static int cx_walk(const char *__u_dir, int (*__u_fn)(const char *__u_path, void *__u_ctx), void *__u_ctx) {
#ifdef _WIN32
    (void)__u_dir; (void)__u_fn; (void)__u_ctx; return -1;
#else
    DIR *__u_d = opendir(__u_dir); struct dirent *__u_e; char **__u_v = 0; size_t __u_n = 0, __u_cap = 0, __u_i; int __u_rc = 0;
    if (!__u_d) return -1;
    while ((__u_e = readdir(__u_d))) {
        size_t __u_l;
        if (!strcmp(__u_e->d_name, ".") || !strcmp(__u_e->d_name, "..")) continue;
        if (__u_n == __u_cap) { char **__u_g = realloc(__u_v, (__u_cap = __u_cap ? 2 * __u_cap : 16) * sizeof *__u_v); if (!__u_g) { __u_rc = -1; break; } __u_v = __u_g; }
        __u_l = strlen(__u_dir) + strlen(__u_e->d_name) + 2;
        if (!(__u_v[__u_n] = malloc(__u_l))) { __u_rc = -1; break; }
        snprintf(__u_v[__u_n++], __u_l, "%s/%s", __u_dir, __u_e->d_name);
    }
    closedir(__u_d);
    if (__u_n) qsort(__u_v, __u_n, sizeof *__u_v, _cx_cmp);
    for (__u_i = 0; __u_i < __u_n; __u_i++) {
        struct stat __u_s;
        if (!__u_rc && lstat(__u_v[__u_i], &__u_s) == 0) {
            if (S_ISDIR(__u_s.st_mode)) __u_rc = cx_walk(__u_v[__u_i], __u_fn, __u_ctx);
            else if (S_ISREG(__u_s.st_mode)) __u_rc = __u_fn(__u_v[__u_i], __u_ctx);
        }
        free(__u_v[__u_i]);
    }
    free(__u_v);
    return __u_rc;
#endif
}
#endif
#endif
