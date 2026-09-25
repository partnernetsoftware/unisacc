/* Minimal <stdio.h> for the unisa C subset.
 * printf is desugared by the walker against its static format string [W-9];
 * everything else here is ordinary C over the `.sys` gate, so it needs no
 * linker and no compiler support.  A FILE * is a file descriptor in a
 * pointer's clothing. */
#ifndef _UNISA_STDIO_H
#define _UNISA_STDIO_H
#include <stddef.h>
#include <stdarg.h>
#include <errno.h>
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
/* A FILE * is its descriptor and nothing is buffered, so the file offset
   is the stream's position: fseek and ftell are lseek.  [S-15 D2] */
static int fseek(FILE *f, long off, int whence) {
    return __lseek(_unisa_fd(f), off, whence) < 0 ? -1 : 0;
}
static long ftell(FILE *f) { return __lseek(_unisa_fd(f), 0, SEEK_CUR); }
static void rewind(FILE *f) { __lseek(_unisa_fd(f), 0, SEEK_SET); }
static int remove(const char *path) { return __unlink((char *)path) < 0 ? -1 : 0; }
static int rename(const char *from, const char *to) {
    return __rename((char *)from, (char *)to) < 0 ? -1 : 0;
}
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

/* ---- floating conversions, exactly ------------------------------------
 * A double is m * 2^e with m < 2^53, so its value is a FINITE decimal:
 * m * 2^e when e >= 0, and m * 5^-e / 10^-e when e < 0.  Build that integer
 * exactly (base 10^9 limbs), then every conversion is a rounding of one digit
 * string at one place -- nearest, ties to even, which is what both platform
 * libcs do with the exact value.  No floating arithmetic is used, so the
 * digits cannot depend on how this very code was compiled. */
#define _U_NL 132                    /* 1188 decimal digits: 5^1074 * 2^53 fits */
#define _U_ND 1200

/* digits of v, most significant first; returns their count, and *x is how
   many of them are before the decimal point (<= 0 for |v| < 1) */
static int _u_dexp(unsigned long bits, char *dig, int *x) {
    unsigned long lim[_U_NL];
    unsigned long m;
    unsigned long t;
    unsigned long carry;
    unsigned long mul;
    int n;
    int e;
    int k;
    int j;
    int nd;
    int ex;
    int first;
    ex = (bits >> 52) & 2047;
    m = bits & 4503599627370495;              /* 2^52 - 1 */
    if (ex == 0) e = 0 - 1074; else { m = m | 4503599627370496; e = ex - 1075; }
    if (m == 0) { dig[0] = 48; *x = 1; return 1; }
    lim[0] = m % 1000000000; lim[1] = (m / 1000000000) % 1000000000;
    lim[2] = m / 1000000000000000000; n = 3;
    while (n > 1) { if (lim[n - 1] != 0) break; n = n - 1; }
    k = e; if (k < 0) k = 0 - k;
    while (k > 0) {                           /* by 2^e, or by 5^-e */
        if (e > 0) { if (k >= 29) { mul = 536870912; k = k - 29; }
                     else { mul = 1; while (k > 0) { mul = mul * 2; k = k - 1; } } }
        else { if (k >= 13) { mul = 1220703125; k = k - 13; }
               else { mul = 1; while (k > 0) { mul = mul * 5; k = k - 1; } } }
        carry = 0; j = 0;
        while (j < n) { t = lim[j] * mul + carry; lim[j] = t % 1000000000;
                        carry = t / 1000000000; j = j + 1; }
        while (carry) { lim[n] = carry % 1000000000; carry = carry / 1000000000; n = n + 1; }
    }
    nd = 0; j = n - 1; first = 1;
    while (j >= 0) {
        /* a limb's nine digits, low first into d9, then out high first */
        char d9[9];
        t = lim[j]; k = 8;
        while (k >= 0) { d9[k] = 48 + t % 10; t = t / 10; k = k - 1; }
        k = 0;
        while (k < 9) {
            if (first == 0 || d9[k] != 48) { dig[nd] = d9[k]; nd = nd + 1; first = 0; }
            k = k + 1;
        }
        j = j - 1;
    }
    ex = (bits >> 52) & 2047;
    k = ex == 0 ? 1074 : 1075 - ex;           /* -e: digits after the point */
    if (k < 0) k = 0;
    *x = nd - k;
    return nd;
}

/* keep r digits of dig[0..nd), rounding to nearest, ties to even; returns
   the new count, and bumps *x when the rounding carries out (9.99 -> 10.0) */
static int _u_round(char *dig, int nd, int r, int *x) {
    int up;
    int j;
    if (r >= nd) return nd;
    if (r < 0) { dig[0] = 48; return 0; }
    up = 0;
    if (dig[r] > 53) up = 1;
    if (dig[r] == 53) {
        j = r + 1;
        while (j < nd) { if (dig[j] != 48) { up = 1; break; } j = j + 1; }
        if (up == 0) { if (r > 0) { if ((dig[r - 1] - 48) & 1) up = 1; } }
    }
    nd = r;
    if (up) {
        j = r - 1;
        while (j >= 0) {
            if (dig[j] != 57) { dig[j] = dig[j] + 1; break; }
            dig[j] = 48; j = j - 1;
        }
        if (j < 0) {                              /* carried out of the top */
            j = nd; while (j > 0) { dig[j] = dig[j - 1]; j = j - 1; }
            dig[0] = 49; nd = nd + 1; *x = *x + 1;
        }
    }
    return nd;
}

/* the digit at position p of the number (0 = first integer digit) */
static int _u_dat(char *dig, int nd, int x, int p) {
    if (p < 0) return 48;
    if (p >= nd) return 48;
    return dig[p];
}

/* %f %e %g (and upper case) of `bits` into out[]; returns the length.
   Sign, width and padding are the caller's. */
static int _u_ffmt(char *out, unsigned long bits, int c, int prec, int alt) {
    char dig[_U_ND];
    int nd;
    int x;
    int n;
    int j;
    int e;
    int ee;
    int style;
    int P;
    int upper;
    int strip;
    upper = c == 70 || c == 69 || c == 71;
    if (((bits >> 52) & 2047) == 2047) {
        char *w;
        if (bits & 4503599627370495) w = upper ? "NAN" : "nan";
        else w = upper ? "INF" : "inf";
        out[0] = w[0]; out[1] = w[1]; out[2] = w[2];
        return 3;
    }
    if (prec < 0) prec = 6;
    nd = _u_dexp(bits & 9223372036854775807, dig, &x);
    if (dig[0] == 48) x = 1;                  /* zero: one integer digit */
    style = c | 32;                           /* f e g */
    strip = 0;
    if (style == 103) {
        /* C99 7.19.6.1p8: P significant digits; the exponent X that %e would
           show decides between the two styles */
        P = prec; if (P == 0) P = 1;
        {   char d2[_U_ND]; int n2; int x2;
            j = 0; while (j < nd) { d2[j] = dig[j]; j = j + 1; }
            x2 = x; n2 = _u_round(d2, nd, P, &x2);
            e = x2 - 1;
            if (dig[0] == 48) e = 0;
        }
        if (P > e && e >= 0 - 4) { style = 102; prec = P - 1 - e; }
        else { style = 101; prec = P - 1; }
        if (alt == 0) strip = 1;
    }
    n = 0;
    if (style == 102) {
        nd = _u_round(dig, nd, x + prec, &x);
        if (x <= 0) { out[n] = 48; n = n + 1; }
        else { j = 0; while (j < x) { out[n] = _u_dat(dig, nd, x, j); n = n + 1; j = j + 1; } }
        if (prec > 0 || alt) { out[n] = 46; n = n + 1; }
        j = 0;
        while (j < prec) { out[n] = _u_dat(dig, nd, x, x + j); n = n + 1; j = j + 1; }
    } else {
        if (dig[0] == 48) e = 0;
        else { nd = _u_round(dig, nd, prec + 1, &x); e = x - 1; }
        out[n] = _u_dat(dig, nd, x, 0); n = n + 1;
        if (prec > 0 || alt) { out[n] = 46; n = n + 1; }
        j = 1;
        while (j <= prec) { out[n] = _u_dat(dig, nd, x, j); n = n + 1; j = j + 1; }
        if (strip) {
            while (n > 0) { if (out[n - 1] != 48) break; n = n - 1; }
            if (n > 0) { if (out[n - 1] == 46) n = n - 1; }
            strip = 0;
        }
        out[n] = upper ? 69 : 101; n = n + 1;
        if (e < 0) { out[n] = 45; ee = 0 - e; } else { out[n] = 43; ee = e; }
        n = n + 1;
        if (ee >= 100) { out[n] = 48 + ee / 100; n = n + 1; }
        out[n] = 48 + (ee / 10) % 10; n = n + 1;
        out[n] = 48 + ee % 10; n = n + 1;
    }
    if (strip) {                              /* %g without '#' */
        j = 0;
        while (j < n) { if (out[j] == 46) break; j = j + 1; }
        if (j < n) {
            while (n > 0) { if (out[n - 1] != 48) break; n = n - 1; }
            if (n > 0) { if (out[n - 1] == 46) n = n - 1; }
        }
    }
    return n;
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
    int plus;
    int space;
    int alt;
    int neg;
    long j;
    long sv;
    unsigned long uv;
    char *sp;
    char buf[24];
    char fbuf[1300];
    n = 0;
    i = 0;
    while (fmt[i]) {
        if (fmt[i] != 37) { _u_put(out, cap, &n, f, fmt[i]); i = i + 1; continue; }
        i = i + 1;
        left = 0; zero = 0; width = 0; prec = 0 - 1;
        plus = 0; space = 0; alt = 0;
        while (fmt[i] == 45 | fmt[i] == 48 | fmt[i] == 43 | fmt[i] == 32
               | fmt[i] == 35) {
            if (fmt[i] == 45) left = 1;
            if (fmt[i] == 48) zero = 1;
            if (fmt[i] == 43) plus = 1;
            if (fmt[i] == 32) space = 1;
            if (fmt[i] == 35) alt = 1;
            i = i + 1;
        }
        if (fmt[i] == 42) { width = va_arg(ap, int); i = i + 1;     /* `*` */
            if (width < 0) { left = 1; width = 0 - width; } }
        while (fmt[i] >= 48) { if (fmt[i] > 57) break;
            width = width * 10 + (fmt[i] - 48); i = i + 1; }
        if (fmt[i] == 46) {
            i = i + 1; prec = 0;
            if (fmt[i] == 42) { prec = va_arg(ap, int); i = i + 1;
                if (prec < 0) prec = 0 - 1; }
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
        sp = NULL; sign = 0; base = 10; upper = 0; neg = 0;
        if (c == 102 | c == 70 | c == 101 | c == 69 | c == 103 | c == 71) {
            /* %f %e %g: a double -- a float argument was promoted to one */
            double dv;
            unsigned long bits;
            dv = va_arg(ap, double);
            bits = *(unsigned long *)&dv;
            neg = (bits >> 63) & 1;
            len = _u_ffmt(fbuf + 1, bits, c, prec, alt);
            start = 1; sp = fbuf;
            if (((bits >> 52) & 2047) == 2047) zero = 0;   /* inf, nan pad with spaces */
            sign = neg;
        } else {
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
                    if (lng == 0) sv = (int)sv;       /* an int is 32 bits */
                    if (sv < 0) { sign = 1; uv = 0 - sv; } else uv = sv;
                    neg = sign;
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
                /* C99 7.19.6.1p5: an integer's precision is the MINIMUM
                   number of digits -- `%.2x` of 0 is "00".  Zeros go in
                   before the sign does. */
                while (len < prec) {
                    if (start <= 1) break;
                    start = start - 1; buf[start] = 48; len = len + 1;
                }
                sp = buf;
                if (prec >= 0) zero = 0;          /* 7.19.6.1p6: 0 ignored */
            }
        }
        }
        /* the sign character: '-', or '+' / ' ' when asked for (signed
           conversions only).  With '0' the zeros go AFTER it: -0042 */
        {   int sc;
            sc = 0;
            if (c == 100 | c == 105 | c == 102 | c == 70 | c == 101 | c == 69
                | c == 103 | c == 71) {
                if (sign) sc = 45; else { if (plus) sc = 43; else { if (space) sc = 32; } }
            }
            k = width - len;
            if (sc) k = k - 1;
            if (left == 0) { if (zero == 0) {
                while (k > 0) { _u_put(out, cap, &n, f, 32); k = k - 1; } } }
            if (sc) _u_put(out, cap, &n, f, sc);
            if (left == 0) { if (zero) {
                while (k > 0) { _u_put(out, cap, &n, f, 48); k = k - 1; } } }
            j = 0;
            while (j < len) { _u_put(out, cap, &n, f, sp[start + j] & 255); j = j + 1; }
            if (left) { while (k > 0) { _u_put(out, cap, &n, f, 32); k = k - 1; } }
        }
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
/* The compiler desugars `printf` against a STATIC format string [W-9] -- the
 * fast path, and the common one.  A format that is not a literal cannot be
 * desugared at all, so it becomes an ordinary variadic call on this. */
static int printf(const char *fmt, ...) {
    va_list ap; int r;
    va_start(ap, fmt);
    r = _u_vfmt(NULL, 0, stdout, fmt, ap);
    va_end(ap);
    return r;
}
/* ---- sscanf: the conversions a program reads numbers and words with --
   d i u x o c s f e g, the l and h length modifiers, a field width, `*`
   to discard, `%%`, and white space in the format matching any amount of
   it.  Returns the number of conversions stored, or EOF when the input
   ran out before the first one -- the same contract as the platform's,
   which is what a program comparing the two observes. [S-15 D2] */
static int _u_isspace(int c) { return c == 32 || (c >= 9 && c <= 13); }
static int _u_digit(int c, int base) {
    int v;
    v = 0 - 1;
    if (c >= 48 && c <= 57) v = c - 48;
    if (c >= 97 && c <= 122) v = c - 97 + 10;
    if (c >= 65 && c <= 90) v = c - 65 + 10;
    if (v >= base) return 0 - 1;
    return v;
}
static int vsscanf(const char *in, const char *fmt, va_list ap) {
    long i; long j; int stored; int c; int width; int skip; int lng; int base;
    int neg; int any; long v; unsigned long uv; double d; double scale; int exp; int eneg;
    char *sp; int *ip; long *lp; short *hp; double *dp; float *fp; long n;
    i = 0; j = 0; stored = 0;
    while (fmt[j]) {
        c = fmt[j];
        if (_u_isspace(c)) { while (_u_isspace(in[i])) i = i + 1; j = j + 1; continue; }
        if (c != 37) { if (in[i] != c) break; i = i + 1; j = j + 1; continue; }
        j = j + 1;
        if (fmt[j] == 37) { if (in[i] != 37) break; i = i + 1; j = j + 1; continue; }
        skip = 0; width = 0; lng = 0;
        if (fmt[j] == 42) { skip = 1; j = j + 1; }
        while (fmt[j] >= 48 && fmt[j] <= 57) { width = width * 10 + (fmt[j] - 48); j = j + 1; }
        if (fmt[j] == 104) { lng = 0 - 1; j = j + 1; if (fmt[j] == 104) { lng = 0 - 2; j = j + 1; } }
        if (fmt[j] == 108) { lng = 1; j = j + 1; if (fmt[j] == 108) j = j + 1; }
        if (fmt[j] == 122 || fmt[j] == 106 || fmt[j] == 116) { lng = 1; j = j + 1; }
        if (fmt[j] == 76) { lng = 1; j = j + 1; }
        c = fmt[j]; j = j + 1;
        if (width == 0) width = 1000000000;
        if (c == 99) {                                   /* %c: no space skip */
            if (width == 1000000000) width = 1;
            if (in[i] == 0) { if (stored == 0) return EOF; return stored; }
            if (skip == 0) sp = va_arg(ap, char *);
            n = 0;
            while (n < width) { if (in[i] == 0) break; if (skip == 0) sp[n] = in[i]; i = i + 1; n = n + 1; }
            if (skip == 0) stored = stored + 1;
            continue;
        }
        while (_u_isspace(in[i])) i = i + 1;
        if (in[i] == 0) { if (stored == 0) return EOF; return stored; }
        if (c == 115) {                                  /* %s */
            if (skip == 0) sp = va_arg(ap, char *);
            n = 0;
            while (n < width) { if (in[i] == 0) break; if (_u_isspace(in[i])) break;
                                if (skip == 0) sp[n] = in[i]; i = i + 1; n = n + 1; }
            if (skip == 0) { sp[n] = 0; stored = stored + 1; }
            continue;
        }
        if (c == 100 || c == 105 || c == 117 || c == 120 || c == 88 || c == 111) {
            base = 10;
            if (c == 120 || c == 88) base = 16;
            if (c == 111) base = 8;
            neg = 0; any = 0; uv = 0; n = 0;
            if (in[i] == 45 || in[i] == 43) { if (n < width) { neg = in[i] == 45; i = i + 1; n = n + 1; } }
            if (c == 105 || base == 16) { if (in[i] == 48) { if (in[i + 1] == 120 || in[i + 1] == 88) {
                if (_u_digit(in[i + 2], 16) >= 0) { if (n + 2 < width) { base = 16; i = i + 2; n = n + 2; } } } } }
            if (c == 105) { if (base == 10) { if (in[i] == 48) base = 8; } }
            while (n < width) {
                v = _u_digit(in[i], base);
                if (v < 0) break;
                uv = uv * base + v; i = i + 1; n = n + 1; any = 1;
            }
            if (any == 0) break;
            if (neg) uv = 0 - uv;
            if (skip == 0) {
                if (lng == 1) { lp = va_arg(ap, long *); *lp = (long)uv; }
                else { if (lng == 0 - 1) { hp = va_arg(ap, short *); *hp = (short)uv; }
                       else { if (lng == 0 - 2) { sp = va_arg(ap, char *); *sp = (char)uv; }
                              else { ip = va_arg(ap, int *); *ip = (int)uv; } } }
                stored = stored + 1;
            }
            continue;
        }
        if (c == 102 || c == 101 || c == 103 || c == 70 || c == 69 || c == 71 || c == 97) {
            neg = 0; any = 0; d = 0.0; n = 0;
            if (in[i] == 45 || in[i] == 43) { if (n < width) { neg = in[i] == 45; i = i + 1; n = n + 1; } }
            while (n < width) { v = _u_digit(in[i], 10); if (v < 0) break; d = d * 10.0 + v; i = i + 1; n = n + 1; any = 1; }
            if (in[i] == 46) { if (n < width) {
                i = i + 1; n = n + 1; scale = 0.1;
                while (n < width) { v = _u_digit(in[i], 10); if (v < 0) break;
                                    d = d + v * scale; scale = scale * 0.1; i = i + 1; n = n + 1; any = 1; }
            } }
            if (any == 0) break;
            if (in[i] == 101 || in[i] == 69) { if (n < width) {
                exp = 0; eneg = 0; j = j; 
                if (in[i + 1] == 45 || in[i + 1] == 43) { eneg = in[i + 1] == 45;
                    if (_u_digit(in[i + 2], 10) >= 0) { i = i + 2; n = n + 2;
                        while (n < width) { v = _u_digit(in[i], 10); if (v < 0) break; exp = exp * 10 + v; i = i + 1; n = n + 1; } } }
                else { if (_u_digit(in[i + 1], 10) >= 0) { i = i + 1; n = n + 1;
                        while (n < width) { v = _u_digit(in[i], 10); if (v < 0) break; exp = exp * 10 + v; i = i + 1; n = n + 1; } } }
                while (exp > 0) { if (eneg) d = d / 10.0; else d = d * 10.0; exp = exp - 1; }
            } }
            if (neg) d = 0.0 - d;
            if (skip == 0) {
                if (lng == 1) { dp = va_arg(ap, double *); *dp = d; }
                else { fp = va_arg(ap, float *); *fp = (float)d; }
                stored = stored + 1;
            }
            continue;
        }
        if (c == 110) {                                  /* %n */
            if (skip == 0) { ip = va_arg(ap, int *); *ip = (int)i; }
            continue;
        }
        break;                                           /* an unknown conversion ends the scan */
    }
    return stored;
}
static int sscanf(const char *in, const char *fmt, ...) {
    va_list ap; int r;
    va_start(ap, fmt);
    r = vsscanf(in, fmt, ap);
    va_end(ap);
    return r;
}

/* perror: the message, a colon, and errno's text -- the eleven codes
   <errno.h> defines, "Unknown error N" for the rest. */
static char *strerror(int e) {
    if (e == 0) return "Success";
    if (e == 1) return "Operation not permitted";
    if (e == 2) return "No such file or directory";
    if (e == 5) return "Input/output error";
    if (e == 9) return "Bad file descriptor";
    if (e == 12) return "Cannot allocate memory";
    if (e == 13) return "Permission denied";
    if (e == 17) return "File exists";
    if (e == 22) return "Invalid argument";
    if (e == 28) return "No space left on device";
    if (e == 33) return "Numerical argument out of domain";
    if (e == 34) return "Numerical result out of range";
    return "Unknown error";
}
static void perror(const char *s) {
    if (s) { if (*s) { fputs(s, stderr); fputs(": ", stderr); } }
    fputs(strerror(errno), stderr);
    fputc(10, stderr);
}

#endif
