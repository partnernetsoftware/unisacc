/* cx.h -- 0.0.34 X1: the small toolkit .cx scripts (#!/usr/bin/env unisacc.com -run) need to replace
 * .sh/.py helpers: run a command and take its output, read/write a whole file, sha256, and a command
 * under a time limit.  Plain C99 over the carried libc.  Windows has no fork, so
 * cx_run and cx_runv_timeout still return -1 there.  Every body sits alone in its own guard (tests/libneed.sh). */
#ifndef _UNISA_CX_H
#define _UNISA_CX_H
/* Edition 1 (plans/cx-modules.md CX1; `#pragma unisa cx 1` is implied): edition 0's toolkit and headers, plus the
   POSIX.1-2008 core headers listed at the end of this file.  The carried libc tests no feature macro, so the one below
   only matters to the system cc oracle (cc -std=c99 -x c -include include/cx.h): strict C99 glibc hides popen, mkstemp,
   kill, lstat and usleep; _DEFAULT_SOURCE is POSIX.1-2008 plus usleep there, and nothing on macOS or the BSDs, whose
   headers show everything unless a POSIX level is asked for (a _POSIX_C_SOURCE here would hide usleep on FreeBSD). */
#define __UNISA_CX_EDITION 1
#if !defined(_WIN32) && !defined(_DEFAULT_SOURCE) && !defined(_POSIX_C_SOURCE) && !defined(_XOPEN_SOURCE) && !defined(_GNU_SOURCE)
#define _DEFAULT_SOURCE 1
#endif
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/stat.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#include <spawn.h>
#endif

static char *cx_read(const char *__u_path, size_t *__u_len);
static int cx_write(const char *__u_path, const void *__u_buf, size_t __u_len);
static int cx_run(const char *__u_cmd, char **__u_out, size_t *__u_len);
static int cx_run_timeout(const char *__u_cmd, int __u_secs, char **__u_out, size_t *__u_len);
static void cx_sha256(const void *__u_buf, size_t __u_len, unsigned char __u_md[32]);
static void cx_sha256_hex(const void *__u_buf, size_t __u_len, char __u_hex[65]);
static int cx_runv_timeout(char *const __u_argv[], int __u_secs, char **__u_out, size_t *__u_len);
static int cx_walk(const char *__u_dir, int (*__u_fn)(const char *__u_path, void *__u_ctx), void *__u_ctx);
static int cx_remove(const char *__u_path);
static int cx_rmtree(const char *__u_path);
static int cx_trash(const char *__u_path);

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

/* the whole file, as _cx_slurp; NULL when it cannot be opened or a read fails -- a directory
   included (fopen "rb" opens one on POSIX and the first read fails with EISDIR; it was an empty
   buffer until 0.0.43, where Python's open() raises) */
#if !__UNISA_FTRIM_LIBC || __UN_cx_read
static char *cx_read(const char *__u_path, size_t *__u_len) {
    FILE *__u_f = fopen(__u_path, "rb"); char *__u_b;
    if (!__u_f) return 0;
#ifndef _WIN32
    /* A freshly opened stream has no buffered bytes.  Inspect the descriptor
       result directly: the carried stdio has only eight error-state slots. */
    {   size_t __u_n = 0, __u_cap = 4096; long __u_k; char *__u_t;
        __u_b = malloc(__u_cap);
        if (!__u_b) { fclose(__u_f); return 0; }
        while ((__u_k = read(fileno(__u_f), __u_b + __u_n, __u_cap - __u_n - 1)) > 0) {
            __u_n += __u_k;
            if (__u_cap - __u_n < 2) {
                __u_t = realloc(__u_b, __u_cap * 2);
                if (!__u_t) { free(__u_b); fclose(__u_f); return 0; }
                __u_b = __u_t; __u_cap *= 2;
            }
        }
        if (__u_k < 0) { free(__u_b); __u_b = 0; __u_n = 0; }
        else __u_b[__u_n] = 0;
        if (__u_len) *__u_len = __u_n;
    }
#else
    __u_b = _cx_slurp(__u_f, __u_len);
    if (__u_b && ferror(__u_f)) { free(__u_b); __u_b = 0; if (__u_len) *__u_len = 0; }
#endif
    fclose(__u_f); return __u_b;
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

/* Edition 1 headers, after the toolkit, where the .cx tools used to include them (so a tool that drops its own copy
   keeps its order of library bodies), and environ, which glibc declares only under _GNU_SOURCE.  Still explicit in
   a .cx, to revisit at edition 2: utime.h, sys/time.h, sys/ioctl.h (rare), math.h (libm semantics), and time.h,
   whose static objects (the day and month names, the zone buffer) sit outside any -ftrim-libc guard and would add
   143 bytes to every .cx program; guarding them would change the bytes of .c programs that include it. */
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>
/* Return success only when the complete formatted text fits.  A failed
   format leaves an empty destination whenever it has room for a terminator. */
#if !__UNISA_FTRIM_LIBC || __UN_cx_fmt
static int cx_fmt(char *__u_dst, size_t __u_cap, const char *__u_fmt, ...) {
    va_list __u_ap; int __u_n;
    if (__u_dst && __u_cap) __u_dst[0] = 0;
    if (!__u_dst || !__u_fmt || !__u_cap) return -1;
    va_start(__u_ap, __u_fmt);
    __u_n = vsnprintf(__u_dst, __u_cap, __u_fmt, __u_ap);
    va_end(__u_ap);
    if (__u_n < 0 || (size_t)__u_n >= __u_cap) { __u_dst[0] = 0; return -1; }
    return 0;
}
#endif

/* Join a directory and a relative name without ever writing a partial path. */
#if !__UNISA_FTRIM_LIBC || __UN_cx_path_join
static int cx_path_join(char *__u_dst, size_t __u_cap, const char *__u_dir, const char *__u_name) {
    size_t __u_nd, __u_nn, __u_slash, __u_at = 0;
    if (!__u_dst || !__u_cap || !__u_dir || !__u_name || !__u_name[0] || __u_name[0] == '/') {
        if (__u_dst && __u_cap) __u_dst[0] = 0;
        return -1;
    }
    __u_nd = strlen(__u_dir); __u_nn = strlen(__u_name);
    __u_slash = __u_nd && __u_dir[__u_nd - 1] != '/';
    if (__u_nd >= __u_cap || __u_slash > __u_cap - __u_nd - 1 ||
        __u_nn > __u_cap - __u_nd - __u_slash - 1) { __u_dst[0] = 0; return -1; }
    memmove(__u_dst + __u_at, __u_dir, __u_nd); __u_at += __u_nd;
    if (__u_slash) __u_dst[__u_at++] = '/';
    memcpy(__u_dst + __u_at, __u_name, __u_nn); __u_at += __u_nn;
    __u_dst[__u_at] = 0;
    return 0;
}
#endif
#ifndef _WIN32
#include <sys/types.h>
#include <fcntl.h>
extern char **environ;
#endif
/* Deletion goes through the one guard, include/unisa_safedel.h (commissar ruling 2026-10-23): refused unless the
   target lies strictly under $TMPDIR (or /tmp) or a repository's out/ or .trash/, is no home, cwd, repository or
   ancestor of them, holds no .git and is within the entry ceiling.  0 on success (a missing path included), else the
   guard's code (3 refused, 1 failed) with one `cx_...: why` line on stderr.  cx_remove: a file, a link or an empty
   directory, for good; cx_rmtree: a tree, for good; cx_trash: a tree moved into the .trash of its root.  After the
   edition 1 headers, so a tool's library bodies keep their order. */
#include "unisa_safedel.h"
#if !__UNISA_FTRIM_LIBC || __UN__cx_del
static int _cx_del(const char *__u_who, const char *__u_path, unsigned __u_flags) {
    char __u_why[600]; int __u_rc = unisa_safedel(__u_path, __u_flags, 0, __u_why, (int)sizeof __u_why);
    if (__u_rc) fprintf(stderr, "%s: %s\n", __u_who, __u_why);
    return __u_rc;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_cx_remove
static int cx_remove(const char *__u_path) { return _cx_del("cx_remove", __u_path, SAFEDEL_PURGE); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_cx_rmtree
static int cx_rmtree(const char *__u_path) { return _cx_del("cx_rmtree", __u_path, SAFEDEL_PURGE | SAFEDEL_RECURSIVE); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_cx_trash
static int cx_trash(const char *__u_path) { return _cx_del("cx_trash", __u_path, SAFEDEL_TRASH | SAFEDEL_RECURSIVE); }
#endif
#endif
