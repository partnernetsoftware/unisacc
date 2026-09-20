/* Minimal <stdio.h> for the unisa C subset.
 * printf is desugared by the walker against its static format string [W-9];
 * everything else here is ordinary C over the `.sys` gate, so it needs no
 * linker and no compiler support.  A FILE * is a file descriptor in a
 * pointer's clothing. */
#ifndef _UNISA_STDIO_H
#define _UNISA_STDIO_H
#include <stddef.h>
#include <stdarg.h>
#define NULL 0
#define EOF (0-1)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#define BUFSIZ 4096

typedef struct _UNISA_FILE FILE;
#define stdin  ((FILE *)0)
#define stdout ((FILE *)1)
#define stderr ((FILE *)2)

int printf();

static int _unisa_fd(FILE *f) { return (int)(long)f; }

static long _unisa_len(const char *s) {
    long n;
    n = 0;
    while (s[n]) n = n + 1;
    return n;
}

static char _unisa_ch;

static int fputc(int c, FILE *f) {
    _unisa_ch = c;
    __write(_unisa_fd(f), &_unisa_ch, 1);
    return c;
}

static int putchar(int c) { return fputc(c, stdout); }

static int fputs(const char *s, FILE *f) {
    __write(_unisa_fd(f), (char *)s, _unisa_len(s));
    return 0;
}

static int puts(const char *s) {
    fputs(s, stdout);
    _unisa_ch = 10;
    __write(1, &_unisa_ch, 1);
    return 0;
}

static long fwrite(const void *p, long sz, long n, FILE *f) {
    __write(_unisa_fd(f), (char *)p, sz * n);
    return n;
}

static long fread(void *p, long sz, long n, FILE *f) {
    long got;
    got = __read(_unisa_fd(f), (char *)p, sz * n);
    if (got < 0) return 0;
    return got / sz;
}

static FILE *fopen(const char *path, const char *mode) {
    int fd;
    int flags;
    flags = 0;
    if (mode[0] == 119) flags = 577;      /* 'w': O_WRONLY|O_CREAT|O_TRUNC */
    if (mode[0] == 97) flags = 521;       /* 'a': O_WRONLY|O_CREAT|O_APPEND */
    fd = __open((char *)path, flags);
    if (fd < 0) return NULL;
    return (FILE *)(long)fd;
}

static int fclose(FILE *f) { return __close(_unisa_fd(f)); }
static int fflush(FILE *f) { return 0; }

/* ---- a runtime formatter ------------------------------------------------
 * `printf` is desugared by the walker against its static format string, which
 * is the fast path and the one that does not need varargs.  Everything that
 * takes a format at RUN time is written here, in the subset itself. */

static void _u_put(char *out, long cap, long *n, FILE *f, int c) {
    char ch;
    if (out != NULL) {
        if (cap < 0 | *n < cap - 1) out[*n] = c;
    } else {
        ch = c;
        __write(_unisa_fd(f), &ch, 1);
    }
    *n = *n + 1;
}

static long _u_digits(char *buf, unsigned long v, int base, int upper) {
    long i;
    int d;
    i = 24;
    if (v == 0) { i = i - 1; buf[i] = 48; return i; }
    while (v) {
        d = v % base;
        v = v / base;
        i = i - 1;
        if (d < 10) buf[i] = 48 + d;
        else { if (upper) buf[i] = 55 + d; else buf[i] = 87 + d; }
    }
    return i;
}

static int _u_vfmt(char *out, long cap, FILE *f, const char *fmt, va_list ap) {
    long n;
    long i;
    long k;
    long len;
    long start;
    int c;
    int left;
    int zero;
    int width;
    int prec;
    int base;
    int upper;
    int sign;
    int lng;
    long sv;
    unsigned long uv;
    char *sp;
    char buf[24];
    n = 0;
    i = 0;
    while (fmt[i]) {
        if (fmt[i] != 37) { _u_put(out, cap, &n, f, fmt[i]); i = i + 1; continue; }
        i = i + 1;
        left = 0; zero = 0; width = 0; prec = 0 - 1;
        while (fmt[i] == 45 | fmt[i] == 48 | fmt[i] == 43 | fmt[i] == 32
               | fmt[i] == 35) {
            if (fmt[i] == 45) left = 1;
            if (fmt[i] == 48) zero = 1;
            i = i + 1;
        }
        while (fmt[i] >= 48) { if (fmt[i] > 57) break;
            width = width * 10 + (fmt[i] - 48); i = i + 1; }
        if (fmt[i] == 46) {
            i = i + 1; prec = 0;
            while (fmt[i] >= 48) { if (fmt[i] > 57) break;
                prec = prec * 10 + (fmt[i] - 48); i = i + 1; }
        }
        lng = 0;
        while (fmt[i] == 104 | fmt[i] == 108 | fmt[i] == 122 | fmt[i] == 106
               | fmt[i] == 116 | fmt[i] == 76) {
            if (fmt[i] != 104) lng = 1;      /* l, z, j, t, L are 64-bit */
            i = i + 1;
        }
        c = fmt[i];
        i = i + 1;
        if (c == 37) { _u_put(out, cap, &n, f, 37); continue; }
        sp = NULL; sign = 0; base = 10; upper = 0;
        if (c == 115) {
            sp = va_arg(ap, char *);
            if (sp == NULL) sp = "(null)";
            len = _unisa_len(sp);
            if (prec >= 0) { if (prec < len) len = prec; }
            start = 0;
        } else {
            if (c == 99) {
                buf[23] = va_arg(ap, int);
                start = 23; len = 1; sp = buf;
            } else {
                if (c == 100 | c == 105) {
                    sv = va_arg(ap, long);
                    if (sv < 0) { sign = 1; uv = 0 - sv; } else uv = sv;
                } else {
                    if (c == 120) { base = 16; }
                    if (c == 88) { base = 16; upper = 1; }
                    if (c == 111) { base = 8; }
                    if (c == 112) { base = 16; }
                    uv = va_arg(ap, unsigned long);
                    if (lng == 0) {
                        if (c == 117 | c == 120 | c == 88 | c == 111)
                            uv = uv & 4294967295;
                    }
                }
                start = _u_digits(buf, uv, base, upper);
                len = 24 - start;
                if (sign) { start = start - 1; buf[start] = 45; len = len + 1; }
                sp = buf;
            }
        }
        k = width - len;
        if (left == 0) {
            while (k > 0) { _u_put(out, cap, &n, f, zero ? 48 : 32); k = k - 1; }
        }
        k = 0;
        while (k < len) { _u_put(out, cap, &n, f, sp[start + k] & 255); k = k + 1; }
        if (left) { k = width - len;
            while (k > 0) { _u_put(out, cap, &n, f, 32); k = k - 1; } }
    }
    if (out != NULL) { if (cap != 0) {
        if (cap < 0) out[n] = 0; else { if (n < cap) out[n] = 0;
                                        else out[cap - 1] = 0; } } }
    return (int)n;
}

static int vsprintf(char *b, const char *fmt, va_list ap) {
    return _u_vfmt(b, 0 - 1, NULL, fmt, ap);
}
static int vsnprintf(char *b, long cap, const char *fmt, va_list ap) {
    return _u_vfmt(b, cap, NULL, fmt, ap);
}
static int vfprintf(FILE *f, const char *fmt, va_list ap) {
    return _u_vfmt(NULL, 0, f, fmt, ap);
}
static int sprintf(char *b, const char *fmt, ...) {
    va_list ap; int r;
    va_start(ap, fmt);
    r = _u_vfmt(b, 0 - 1, NULL, fmt, ap);
    va_end(ap);
    return r;
}
static int snprintf(char *b, long cap, const char *fmt, ...) {
    va_list ap; int r;
    va_start(ap, fmt);
    r = _u_vfmt(b, cap, NULL, fmt, ap);
    va_end(ap);
    return r;
}
static int fprintf(FILE *f, const char *fmt, ...) {
    va_list ap; int r;
    va_start(ap, fmt);
    r = _u_vfmt(NULL, 0, f, fmt, ap);
    va_end(ap);
    return r;
}
#endif
