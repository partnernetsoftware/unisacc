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

/* O_* are not portable numbers: Linux and the BSDs picked different bits,
   and we hardcoded Linux's.  On macOS that turned "w" into flags nobody
   accepts; the file was never created and every read of it came back
   empty. */
#ifdef __linux__
#define _U_O_CREAT   64
#define _U_O_TRUNC   512
#define _U_O_APPEND  1024
#else
#define _U_O_CREAT   512
#define _U_O_TRUNC   1024
#define _U_O_APPEND  8
#endif

static FILE *fopen(const char *path, const char *mode) {
    int fd;
#ifdef _WIN32
    /* Windows has no open(2), and CreateFileA wants its own shapes.  They
       are built HERE and not in the encoder, because this is the only place
       that knows whether the program is being compiled for Windows. */
    long access;
    long disp;
    access = 0x80000000;                  /* GENERIC_READ  */
    disp = 3;                             /* OPEN_EXISTING */
    if (mode[0] == 119) { access = 0x40000000; disp = 2; }   /* 'w' CREATE_ALWAYS */
    if (mode[0] == 97)  { access = 0x40000000; disp = 4; }   /* 'a' OPEN_ALWAYS   */
    fd = __open((char *)path, access, disp);
#else
    int flags;
    flags = 0;                            /* 'r': O_RDONLY */
    if (mode[0] == 119)                   /* 'w' */
        flags = 1 | _U_O_CREAT | _U_O_TRUNC;
    if (mode[0] == 97)                    /* 'a' */
        flags = 1 | _U_O_CREAT | _U_O_APPEND;
    /* 0644.  Passing no mode at all left it at 0, so the file we had just
       created could not be opened again. */
    fd = __open((char *)path, flags, 420);
#endif
    if (fd < 0) return NULL;
    return (FILE *)(long)fd;
}

/* Unbuffered: a FILE * here is a file descriptor, so there is nowhere to
   keep a buffer and every character costs a read.  Correct, not fast. */
static int fgetc(FILE *f) {
    unsigned char c;
    if (fread(&c, 1, 1, f) != 1) return EOF;
    return (int)c;
}

static int getc(FILE *f) { return fgetc(f); }
static int getchar(void) { return fgetc(stdin); }

static char *fgets(char *s, int n, FILE *f) {
    int i;
    int c;
    if (n <= 0) return NULL;
    i = 0;
    while (i < n - 1) {
        c = fgetc(f);
        if (c == EOF) break;
        s[i] = (char)c;
        i = i + 1;
        if (c == 10) break;               /* '\n' ends the line, and stays */
    }
    if (i == 0) return NULL;
    s[i] = 0;
    return s;
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
