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

static int _unisa_fd(FILE *__u_f) { return (int)(long)__u_f; }

static long _unisa_len(const char *__u_s) {
    long __u_n;
    __u_n = 0;
    while (__u_s[__u_n]) __u_n = __u_n + 1;
    return __u_n;
}

static char _unisa_ch;

static int fputc(int __u_c, FILE *__u_f) {
    _unisa_ch = __u_c;
    __write(_unisa_fd(__u_f), &_unisa_ch, 1);
    return __u_c;
}

static int putchar(int __u_c) { return fputc(__u_c, stdout); }

static int fputs(const char *__u_s, FILE *__u_f) {
    __write(_unisa_fd(__u_f), (char *)__u_s, _unisa_len(__u_s));
    return 0;
}

static int puts(const char *__u_s) {
    fputs(__u_s, stdout);
    _unisa_ch = 10;
    __write(1, &_unisa_ch, 1);
    return 0;
}

/* fwrite: loops over __write until every byte is out or the gate stops.
 * What __write answers, audited per back end (2026-09-25):
 *   Linux  (arm64 svc / x86_64 syscall): bytes written, or -errno.
 *   Darwin (svc #0x80 / syscall): the kernel flags failure in CARRY with
 *          errno positive; the gate negates it (`b.cc`/`jnc` over `neg`),
 *          so bytes written, or -errno -- same as Linux.
 *   Windows (WriteFile via the winapi gate, retconv `wcount`): the gate
 *          returns *lpNumberOfBytesWritten, NOT the BOOL.  WriteFile zeroes
 *          that count first, so a failure reads back as 0, never negative.
 *   Python VM (unisa/vm.py): fd 1/2 are captured whole (returns n); other
 *          fds return os.write's count, or -1 on OSError.
 *   exec_target (unisa/exec_target.py): count like the VM, but an OSError
 *          is a Trap, not a return value.
 * So "error" is a negative value on POSIX and the VM, and 0 on Windows;
 * both stop the loop below (0 must stop anyway, or it would spin).
 * Overflow: sz and n are signed long.  A negative sz or n, or sz*n above
 * LONG_MAX (tested as n > LONG_MAX / sz before multiplying), describes no
 * object that could exist; fwrite writes nothing and returns 0.  The
 * running offset `done` never exceeds `total`, which fits, and a gate
 * answer larger than what was asked is treated as an error rather than
 * trusted, so `done` cannot overflow either.
 * The return is complete elements: done / sz, rounded down. */
static long fwrite(const void *__u_p, long __u_sz, long __u_n, FILE *__u_f) {
    long __u_total; long __u_done; long __u_r;
    if (__u_sz <= 0 || __u_n <= 0) return 0;
    if (__u_n > 0x7fffffffffffffff / __u_sz) return 0;
    __u_total = __u_sz * __u_n;
    __u_done = 0;
    while (__u_done < __u_total) {
        __u_r = __write(_unisa_fd(__u_f), (char *)__u_p + __u_done, __u_total - __u_done);
        if (__u_r <= 0 || __u_r > __u_total - __u_done) return __u_done / __u_sz;
        __u_done = __u_done + __u_r;
    }
    return __u_n;
}

static long fread(void *__u_p, long __u_sz, long __u_n, FILE *__u_f) {
    long __u_got;
    __u_got = __read(_unisa_fd(__u_f), (char *)__u_p, __u_sz * __u_n);
    if (__u_got < 0) return 0;
    return __u_got / __u_sz;
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

static FILE *fopen(const char *__u_path, const char *__u_mode) {
    int __u_fd;
#ifdef _WIN32
    /* Windows has no open(2), and CreateFileA wants its own shapes.  They
       are built HERE and not in the encoder, because this is the only place
       that knows whether the program is being compiled for Windows. */
    long __u_access;
    long __u_disp;
    __u_access = 0x80000000;                  /* GENERIC_READ  */
    __u_disp = 3;                             /* OPEN_EXISTING */
    if (__u_mode[0] == 119) { __u_access = 0x40000000; __u_disp = 2; }   /* 'w' CREATE_ALWAYS */
    if (__u_mode[0] == 97)  { __u_access = 0x40000000; __u_disp = 4; }   /* 'a' OPEN_ALWAYS   */
    __u_fd = __open((char *)__u_path, __u_access, __u_disp);
#else
    int __u_flags;
    __u_flags = 0;                            /* 'r': O_RDONLY */
    if (__u_mode[0] == 119)                   /* 'w' */
        __u_flags = 1 | _U_O_CREAT | _U_O_TRUNC;
    if (__u_mode[0] == 97)                    /* 'a' */
        __u_flags = 1 | _U_O_CREAT | _U_O_APPEND;
    /* 0644.  Passing no mode at all left it at 0, so the file we had just
       created could not be opened again. */
    __u_fd = __open((char *)__u_path, __u_flags, 420);
#endif
    if (__u_fd < 0) return NULL;
    return (FILE *)(long)__u_fd;
}

/* Unbuffered: a FILE * here is a file descriptor, so there is nowhere to
   keep a buffer and every character costs a read.  Correct, not fast. */
static int fgetc(FILE *__u_f) {
    unsigned char __u_c;
    if (fread(&__u_c, 1, 1, __u_f) != 1) return EOF;
    return (int)__u_c;
}

static int getc(FILE *__u_f) { return fgetc(__u_f); }
static int getchar(void) { return fgetc(stdin); }

static char *fgets(char *__u_s, int __u_n, FILE *__u_f) {
    int __u_i;
    int __u_c;
    if (__u_n <= 0) return NULL;
    __u_i = 0;
    while (__u_i < __u_n - 1) {
        __u_c = fgetc(__u_f);
        if (__u_c == EOF) break;
        __u_s[__u_i] = (char)__u_c;
        __u_i = __u_i + 1;
        if (__u_c == 10) break;               /* '\n' ends the line, and stays */
    }
    if (__u_i == 0) return NULL;
    __u_s[__u_i] = 0;
    return __u_s;
}

static int fclose(FILE *__u_f) { return __close(_unisa_fd(__u_f)); }
/* A FILE * is its descriptor and nothing is buffered, so the file offset
   is the stream's position: fseek and ftell are lseek.  [S-15 D2] */
static int fseek(FILE *__u_f, long __u_off, int __u_whence) {
    return __lseek(_unisa_fd(__u_f), __u_off, __u_whence) < 0 ? -1 : 0;
}
static long ftell(FILE *__u_f) { return __lseek(_unisa_fd(__u_f), 0, SEEK_CUR); }
static void rewind(FILE *__u_f) { __lseek(_unisa_fd(__u_f), 0, SEEK_SET); }
static int remove(const char *__u_path) { return __unlink((char *)__u_path) < 0 ? -1 : 0; }
static int rename(const char *__u_from, const char *__u_to) {
    return __rename((char *)__u_from, (char *)__u_to) < 0 ? -1 : 0;
}
static int fflush(FILE *__u_f) { return 0; }

/* ---- a runtime formatter ------------------------------------------------
 * `printf` is desugared by the walker against its static format string, which
 * is the fast path and the one that does not need varargs.  Everything that
 * takes a format at RUN time is written here, in the subset itself. */

static void _u_put(char *__u_out, long __u_cap, long *__u_n, FILE *__u_f, int __u_c) {
    char __u_ch;
    if (__u_out != NULL) {
        if (__u_cap < 0 | *__u_n < __u_cap - 1) __u_out[*__u_n] = __u_c;
    } else {
        __u_ch = __u_c;
        __write(_unisa_fd(__u_f), &__u_ch, 1);
    }
    *__u_n = *__u_n + 1;
}

static long _u_digits(char *__u_buf, unsigned long __u_v, int __u_base, int __u_upper) {
    long __u_i;
    int __u_d;
    __u_i = 24;
    if (__u_v == 0) { __u_i = __u_i - 1; __u_buf[__u_i] = 48; return __u_i; }
    while (__u_v) {
        __u_d = __u_v % __u_base;
        __u_v = __u_v / __u_base;
        __u_i = __u_i - 1;
        if (__u_d < 10) __u_buf[__u_i] = 48 + __u_d;
        else { if (__u_upper) __u_buf[__u_i] = 55 + __u_d; else __u_buf[__u_i] = 87 + __u_d; }
    }
    return __u_i;
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
static int _u_dexp(unsigned long __u_bits, char *__u_dig, int *__u_x) {
    unsigned long __u_lim[_U_NL];
    unsigned long __u_m;
    unsigned long __u_t;
    unsigned long __u_carry;
    unsigned long __u_mul;
    int __u_n;
    int __u_e;
    int __u_k;
    int __u_j;
    int __u_nd;
    int __u_ex;
    int __u_first;
    __u_ex = (__u_bits >> 52) & 2047;
    __u_m = __u_bits & 4503599627370495;              /* 2^52 - 1 */
    if (__u_ex == 0) __u_e = 0 - 1074; else { __u_m = __u_m | 4503599627370496; __u_e = __u_ex - 1075; }
    if (__u_m == 0) { __u_dig[0] = 48; *__u_x = 1; return 1; }
    __u_lim[0] = __u_m % 1000000000; __u_lim[1] = (__u_m / 1000000000) % 1000000000;
    __u_lim[2] = __u_m / 1000000000000000000; __u_n = 3;
    while (__u_n > 1) { if (__u_lim[__u_n - 1] != 0) break; __u_n = __u_n - 1; }
    __u_k = __u_e; if (__u_k < 0) __u_k = 0 - __u_k;
    while (__u_k > 0) {                           /* by 2^e, or by 5^-e */
        if (__u_e > 0) { if (__u_k >= 29) { __u_mul = 536870912; __u_k = __u_k - 29; }
                     else { __u_mul = 1; while (__u_k > 0) { __u_mul = __u_mul * 2; __u_k = __u_k - 1; } } }
        else { if (__u_k >= 13) { __u_mul = 1220703125; __u_k = __u_k - 13; }
               else { __u_mul = 1; while (__u_k > 0) { __u_mul = __u_mul * 5; __u_k = __u_k - 1; } } }
        __u_carry = 0; __u_j = 0;
        while (__u_j < __u_n) { __u_t = __u_lim[__u_j] * __u_mul + __u_carry; __u_lim[__u_j] = __u_t % 1000000000;
                        __u_carry = __u_t / 1000000000; __u_j = __u_j + 1; }
        while (__u_carry) { __u_lim[__u_n] = __u_carry % 1000000000; __u_carry = __u_carry / 1000000000; __u_n = __u_n + 1; }
    }
    __u_nd = 0; __u_j = __u_n - 1; __u_first = 1;
    while (__u_j >= 0) {
        /* a limb's nine digits, low first into d9, then out high first */
        char __u_d9[9];
        __u_t = __u_lim[__u_j]; __u_k = 8;
        while (__u_k >= 0) { __u_d9[__u_k] = 48 + __u_t % 10; __u_t = __u_t / 10; __u_k = __u_k - 1; }
        __u_k = 0;
        while (__u_k < 9) {
            if (__u_first == 0 || __u_d9[__u_k] != 48) { __u_dig[__u_nd] = __u_d9[__u_k]; __u_nd = __u_nd + 1; __u_first = 0; }
            __u_k = __u_k + 1;
        }
        __u_j = __u_j - 1;
    }
    __u_ex = (__u_bits >> 52) & 2047;
    __u_k = __u_ex == 0 ? 1074 : 1075 - __u_ex;           /* -e: digits after the point */
    if (__u_k < 0) __u_k = 0;
    *__u_x = __u_nd - __u_k;
    return __u_nd;
}

/* keep r digits of dig[0..nd), rounding to nearest, ties to even; returns
   the new count, and bumps *x when the rounding carries out (9.99 -> 10.0) */
static int _u_round(char *__u_dig, int __u_nd, int __u_r, int *__u_x) {
    int __u_up;
    int __u_j;
    if (__u_r >= __u_nd) return __u_nd;
    if (__u_r < 0) { __u_dig[0] = 48; return 0; }
    __u_up = 0;
    if (__u_dig[__u_r] > 53) __u_up = 1;
    if (__u_dig[__u_r] == 53) {
        __u_j = __u_r + 1;
        while (__u_j < __u_nd) { if (__u_dig[__u_j] != 48) { __u_up = 1; break; } __u_j = __u_j + 1; }
        if (__u_up == 0) { if (__u_r > 0) { if ((__u_dig[__u_r - 1] - 48) & 1) __u_up = 1; } }
    }
    __u_nd = __u_r;
    if (__u_up) {
        __u_j = __u_r - 1;
        while (__u_j >= 0) {
            if (__u_dig[__u_j] != 57) { __u_dig[__u_j] = __u_dig[__u_j] + 1; break; }
            __u_dig[__u_j] = 48; __u_j = __u_j - 1;
        }
        if (__u_j < 0) {                              /* carried out of the top */
            __u_j = __u_nd; while (__u_j > 0) { __u_dig[__u_j] = __u_dig[__u_j - 1]; __u_j = __u_j - 1; }
            __u_dig[0] = 49; __u_nd = __u_nd + 1; *__u_x = *__u_x + 1;
        }
    }
    return __u_nd;
}

/* the digit at position p of the number (0 = first integer digit) */
static int _u_dat(char *__u_dig, int __u_nd, int __u_x, int __u_p) {
    if (__u_p < 0) return 48;
    if (__u_p >= __u_nd) return 48;
    return __u_dig[__u_p];
}

/* %f %e %g (and upper case) of `bits` into out[]; returns the length.
   Sign, width and padding are the caller's. */
static int _u_ffmt(char *__u_out, unsigned long __u_bits, int __u_c, int __u_prec, int __u_alt) {
    char __u_dig[_U_ND];
    int __u_nd;
    int __u_x;
    int __u_n;
    int __u_j;
    int __u_e;
    int __u_ee;
    int __u_style;
    int __u_P;
    int __u_upper;
    int __u_strip;
    __u_upper = __u_c == 70 || __u_c == 69 || __u_c == 71;
    if (((__u_bits >> 52) & 2047) == 2047) {
        char *__u_w;
        if (__u_bits & 4503599627370495) __u_w = __u_upper ? "NAN" : "nan";
        else __u_w = __u_upper ? "INF" : "inf";
        __u_out[0] = __u_w[0]; __u_out[1] = __u_w[1]; __u_out[2] = __u_w[2];
        return 3;
    }
    if (__u_prec < 0) __u_prec = 6;
    __u_nd = _u_dexp(__u_bits & 9223372036854775807, __u_dig, &__u_x);
    if (__u_dig[0] == 48) __u_x = 1;                  /* zero: one integer digit */
    __u_style = __u_c | 32;                           /* f e g */
    __u_strip = 0;
    if (__u_style == 103) {
        /* C99 7.19.6.1p8: P significant digits; the exponent X that %e would
           show decides between the two styles */
        __u_P = __u_prec; if (__u_P == 0) __u_P = 1;
        {   char __u_d2[_U_ND]; int __u_n2; int __u_x2;
            __u_j = 0; while (__u_j < __u_nd) { __u_d2[__u_j] = __u_dig[__u_j]; __u_j = __u_j + 1; }
            __u_x2 = __u_x; __u_n2 = _u_round(__u_d2, __u_nd, __u_P, &__u_x2);
            __u_e = __u_x2 - 1;
            if (__u_dig[0] == 48) __u_e = 0;
        }
        if (__u_P > __u_e && __u_e >= 0 - 4) { __u_style = 102; __u_prec = __u_P - 1 - __u_e; }
        else { __u_style = 101; __u_prec = __u_P - 1; }
        if (__u_alt == 0) __u_strip = 1;
    }
    __u_n = 0;
    if (__u_style == 102) {
        __u_nd = _u_round(__u_dig, __u_nd, __u_x + __u_prec, &__u_x);
        if (__u_x <= 0) { __u_out[__u_n] = 48; __u_n = __u_n + 1; }
        else { __u_j = 0; while (__u_j < __u_x) { __u_out[__u_n] = _u_dat(__u_dig, __u_nd, __u_x, __u_j); __u_n = __u_n + 1; __u_j = __u_j + 1; } }
        if (__u_prec > 0 || __u_alt) { __u_out[__u_n] = 46; __u_n = __u_n + 1; }
        __u_j = 0;
        while (__u_j < __u_prec) { __u_out[__u_n] = _u_dat(__u_dig, __u_nd, __u_x, __u_x + __u_j); __u_n = __u_n + 1; __u_j = __u_j + 1; }
    } else {
        if (__u_dig[0] == 48) __u_e = 0;
        else { __u_nd = _u_round(__u_dig, __u_nd, __u_prec + 1, &__u_x); __u_e = __u_x - 1; }
        __u_out[__u_n] = _u_dat(__u_dig, __u_nd, __u_x, 0); __u_n = __u_n + 1;
        if (__u_prec > 0 || __u_alt) { __u_out[__u_n] = 46; __u_n = __u_n + 1; }
        __u_j = 1;
        while (__u_j <= __u_prec) { __u_out[__u_n] = _u_dat(__u_dig, __u_nd, __u_x, __u_j); __u_n = __u_n + 1; __u_j = __u_j + 1; }
        if (__u_strip) {
            while (__u_n > 0) { if (__u_out[__u_n - 1] != 48) break; __u_n = __u_n - 1; }
            if (__u_n > 0) { if (__u_out[__u_n - 1] == 46) __u_n = __u_n - 1; }
            __u_strip = 0;
        }
        __u_out[__u_n] = __u_upper ? 69 : 101; __u_n = __u_n + 1;
        if (__u_e < 0) { __u_out[__u_n] = 45; __u_ee = 0 - __u_e; } else { __u_out[__u_n] = 43; __u_ee = __u_e; }
        __u_n = __u_n + 1;
        if (__u_ee >= 100) { __u_out[__u_n] = 48 + __u_ee / 100; __u_n = __u_n + 1; }
        __u_out[__u_n] = 48 + (__u_ee / 10) % 10; __u_n = __u_n + 1;
        __u_out[__u_n] = 48 + __u_ee % 10; __u_n = __u_n + 1;
    }
    if (__u_strip) {                              /* %g without '#' */
        __u_j = 0;
        while (__u_j < __u_n) { if (__u_out[__u_j] == 46) break; __u_j = __u_j + 1; }
        if (__u_j < __u_n) {
            while (__u_n > 0) { if (__u_out[__u_n - 1] != 48) break; __u_n = __u_n - 1; }
            if (__u_n > 0) { if (__u_out[__u_n - 1] == 46) __u_n = __u_n - 1; }
        }
    }
    return __u_n;
}

static int _u_vfmt(char *__u_out, long __u_cap, FILE *__u_f, const char *__u_fmt, va_list __u_ap) {
    long __u_n;
    long __u_i;
    long __u_k;
    long __u_len;
    long __u_start;
    int __u_c;
    int __u_left;
    int __u_zero;
    int __u_width;
    int __u_prec;
    int __u_base;
    int __u_upper;
    int __u_sign;
    int __u_lng;
    int __u_plus;
    int __u_space;
    int __u_alt;
    int __u_neg;
    long __u_j;
    long __u_sv;
    unsigned long __u_uv;
    char *__u_sp;
    char __u_buf[24];
    char __u_fbuf[1300];
    __u_n = 0;
    __u_i = 0;
    while (__u_fmt[__u_i]) {
        if (__u_fmt[__u_i] != 37) { _u_put(__u_out, __u_cap, &__u_n, __u_f, __u_fmt[__u_i]); __u_i = __u_i + 1; continue; }
        __u_i = __u_i + 1;
        __u_left = 0; __u_zero = 0; __u_width = 0; __u_prec = 0 - 1;
        __u_plus = 0; __u_space = 0; __u_alt = 0;
        while (__u_fmt[__u_i] == 45 | __u_fmt[__u_i] == 48 | __u_fmt[__u_i] == 43 | __u_fmt[__u_i] == 32
               | __u_fmt[__u_i] == 35) {
            if (__u_fmt[__u_i] == 45) __u_left = 1;
            if (__u_fmt[__u_i] == 48) __u_zero = 1;
            if (__u_fmt[__u_i] == 43) __u_plus = 1;
            if (__u_fmt[__u_i] == 32) __u_space = 1;
            if (__u_fmt[__u_i] == 35) __u_alt = 1;
            __u_i = __u_i + 1;
        }
        if (__u_fmt[__u_i] == 42) { __u_width = va_arg(__u_ap, int); __u_i = __u_i + 1;     /* `*` */
            if (__u_width < 0) { __u_left = 1; __u_width = 0 - __u_width; } }
        while (__u_fmt[__u_i] >= 48) { if (__u_fmt[__u_i] > 57) break;
            __u_width = __u_width * 10 + (__u_fmt[__u_i] - 48); __u_i = __u_i + 1; }
        if (__u_fmt[__u_i] == 46) {
            __u_i = __u_i + 1; __u_prec = 0;
            if (__u_fmt[__u_i] == 42) { __u_prec = va_arg(__u_ap, int); __u_i = __u_i + 1;
                if (__u_prec < 0) __u_prec = 0 - 1; }
            while (__u_fmt[__u_i] >= 48) { if (__u_fmt[__u_i] > 57) break;
                __u_prec = __u_prec * 10 + (__u_fmt[__u_i] - 48); __u_i = __u_i + 1; }
        }
        __u_lng = 0;
        while (__u_fmt[__u_i] == 104 | __u_fmt[__u_i] == 108 | __u_fmt[__u_i] == 122 | __u_fmt[__u_i] == 106
               | __u_fmt[__u_i] == 116 | __u_fmt[__u_i] == 76) {
            if (__u_fmt[__u_i] != 104) __u_lng = 1;      /* l, z, j, t, L are 64-bit */
            __u_i = __u_i + 1;
        }
        __u_c = __u_fmt[__u_i];
        __u_i = __u_i + 1;
        if (__u_c == 37) { _u_put(__u_out, __u_cap, &__u_n, __u_f, 37); continue; }
        __u_sp = NULL; __u_sign = 0; __u_base = 10; __u_upper = 0; __u_neg = 0;
        if (__u_c == 102 | __u_c == 70 | __u_c == 101 | __u_c == 69 | __u_c == 103 | __u_c == 71) {
            /* %f %e %g: a double -- a float argument was promoted to one */
            double __u_dv;
            unsigned long __u_bits;
            __u_dv = va_arg(__u_ap, double);
            __u_bits = *(unsigned long *)&__u_dv;
            __u_neg = (__u_bits >> 63) & 1;
            __u_len = _u_ffmt(__u_fbuf + 1, __u_bits, __u_c, __u_prec, __u_alt);
            __u_start = 1; __u_sp = __u_fbuf;
            if (((__u_bits >> 52) & 2047) == 2047) __u_zero = 0;   /* inf, nan pad with spaces */
            __u_sign = __u_neg;
        } else {
        if (__u_c == 115) {
            __u_sp = va_arg(__u_ap, char *);
            if (__u_sp == NULL) __u_sp = "(null)";
            __u_len = _unisa_len(__u_sp);
            if (__u_prec >= 0) { if (__u_prec < __u_len) __u_len = __u_prec; }
            __u_start = 0;
        } else {
            if (__u_c == 99) {
                __u_buf[23] = va_arg(__u_ap, int);
                __u_start = 23; __u_len = 1; __u_sp = __u_buf;
            } else {
                if (__u_c == 100 | __u_c == 105) {
                    __u_sv = va_arg(__u_ap, long);
                    if (__u_lng == 0) __u_sv = (int)__u_sv;       /* an int is 32 bits */
                    if (__u_sv < 0) { __u_sign = 1; __u_uv = 0 - __u_sv; } else __u_uv = __u_sv;
                    __u_neg = __u_sign;
                } else {
                    if (__u_c == 120) { __u_base = 16; }
                    if (__u_c == 88) { __u_base = 16; __u_upper = 1; }
                    if (__u_c == 111) { __u_base = 8; }
                    if (__u_c == 112) { __u_base = 16; }
                    __u_uv = va_arg(__u_ap, unsigned long);
                    if (__u_lng == 0) {
                        if (__u_c == 117 | __u_c == 120 | __u_c == 88 | __u_c == 111)
                            __u_uv = __u_uv & 4294967295;
                    }
                }
                __u_start = _u_digits(__u_buf, __u_uv, __u_base, __u_upper);
                __u_len = 24 - __u_start;
                /* %p: 0x and the hex digits, as both host C libraries print it (7.19.6.1p8
                   leaves the form implementation-defined) */
                if (__u_c == 112) { __u_start = __u_start - 2; __u_buf[__u_start] = 48;
                                    __u_buf[__u_start + 1] = 120; __u_len = __u_len + 2; }
                /* C99 7.19.6.1p5: an integer's precision is the MINIMUM
                   number of digits -- `%.2x` of 0 is "00".  Zeros go in
                   before the sign does. */
                while (__u_len < __u_prec) {
                    if (__u_start <= 1) break;
                    __u_start = __u_start - 1; __u_buf[__u_start] = 48; __u_len = __u_len + 1;
                }
                __u_sp = __u_buf;
                if (__u_prec >= 0) __u_zero = 0;          /* 7.19.6.1p6: 0 ignored */
            }
        }
        }
        /* the sign character: '-', or '+' / ' ' when asked for (signed
           conversions only).  With '0' the zeros go AFTER it: -0042 */
        {   int __u_sc;
            __u_sc = 0;
            if (__u_c == 100 | __u_c == 105 | __u_c == 102 | __u_c == 70 | __u_c == 101 | __u_c == 69
                | __u_c == 103 | __u_c == 71) {
                if (__u_sign) __u_sc = 45; else { if (__u_plus) __u_sc = 43; else { if (__u_space) __u_sc = 32; } }
            }
            __u_k = __u_width - __u_len;
            if (__u_sc) __u_k = __u_k - 1;
            if (__u_left == 0) { if (__u_zero == 0) {
                while (__u_k > 0) { _u_put(__u_out, __u_cap, &__u_n, __u_f, 32); __u_k = __u_k - 1; } } }
            if (__u_sc) _u_put(__u_out, __u_cap, &__u_n, __u_f, __u_sc);
            if (__u_left == 0) { if (__u_zero) {
                while (__u_k > 0) { _u_put(__u_out, __u_cap, &__u_n, __u_f, 48); __u_k = __u_k - 1; } } }
            __u_j = 0;
            while (__u_j < __u_len) { _u_put(__u_out, __u_cap, &__u_n, __u_f, __u_sp[__u_start + __u_j] & 255); __u_j = __u_j + 1; }
            if (__u_left) { while (__u_k > 0) { _u_put(__u_out, __u_cap, &__u_n, __u_f, 32); __u_k = __u_k - 1; } }
        }
    }
    if (__u_out != NULL) { if (__u_cap != 0) {
        if (__u_cap < 0) __u_out[__u_n] = 0; else { if (__u_n < __u_cap) __u_out[__u_n] = 0;
                                        else __u_out[__u_cap - 1] = 0; } } }
    return (int)__u_n;
}

static int vsprintf(char *__u_b, const char *__u_fmt, va_list __u_ap) {
    return _u_vfmt(__u_b, 0 - 1, NULL, __u_fmt, __u_ap);
}
static int vsnprintf(char *__u_b, long __u_cap, const char *__u_fmt, va_list __u_ap) {
    return _u_vfmt(__u_b, __u_cap, NULL, __u_fmt, __u_ap);
}
static int vfprintf(FILE *__u_f, const char *__u_fmt, va_list __u_ap) {
    return _u_vfmt(NULL, 0, __u_f, __u_fmt, __u_ap);
}
static int sprintf(char *__u_b, const char *__u_fmt, ...) {
    va_list __u_ap; int __u_r;
    va_start(__u_ap, __u_fmt);
    __u_r = _u_vfmt(__u_b, 0 - 1, NULL, __u_fmt, __u_ap);
    va_end(__u_ap);
    return __u_r;
}
static int snprintf(char *__u_b, long __u_cap, const char *__u_fmt, ...) {
    va_list __u_ap; int __u_r;
    va_start(__u_ap, __u_fmt);
    __u_r = _u_vfmt(__u_b, __u_cap, NULL, __u_fmt, __u_ap);
    va_end(__u_ap);
    return __u_r;
}
static int fprintf(FILE *__u_f, const char *__u_fmt, ...) {
    va_list __u_ap; int __u_r;
    va_start(__u_ap, __u_fmt);
    __u_r = _u_vfmt(NULL, 0, __u_f, __u_fmt, __u_ap);
    va_end(__u_ap);
    return __u_r;
}
/* The compiler desugars `printf` against a STATIC format string [W-9] -- the
 * fast path, and the common one.  A format that is not a literal cannot be
 * desugared at all, so it becomes an ordinary variadic call on this. */
static int printf(const char *__u_fmt, ...) {
    va_list __u_ap; int __u_r;
    va_start(__u_ap, __u_fmt);
    __u_r = _u_vfmt(NULL, 0, stdout, __u_fmt, __u_ap);
    va_end(__u_ap);
    return __u_r;
}
/* ---- sscanf: the conversions a program reads numbers and words with --
   d i u x o c s f e g, the l and h length modifiers, a field width, `*`
   to discard, `%%`, and white space in the format matching any amount of
   it.  Returns the number of conversions stored, or EOF when the input
   ran out before the first one -- the same contract as the platform's,
   which is what a program comparing the two observes. [S-15 D2] */
static int _u_isspace(int __u_c) { return __u_c == 32 || (__u_c >= 9 && __u_c <= 13); }
static int _u_digit(int __u_c, int __u_base) {
    int __u_v;
    __u_v = 0 - 1;
    if (__u_c >= 48 && __u_c <= 57) __u_v = __u_c - 48;
    if (__u_c >= 97 && __u_c <= 122) __u_v = __u_c - 97 + 10;
    if (__u_c >= 65 && __u_c <= 90) __u_v = __u_c - 65 + 10;
    if (__u_v >= __u_base) return 0 - 1;
    return __u_v;
}
static int vsscanf(const char *__u_in, const char *__u_fmt, va_list __u_ap) {
    long __u_i; long __u_j; int __u_stored; int __u_c; int __u_width; int __u_skip; int __u_lng; int __u_base;
    int __u_neg; int __u_any; long __u_v; unsigned long __u_uv; double __u_d; double __u_scale; int exp; int __u_eneg;
    char *__u_sp; int *__u_ip; long *__u_lp; short *__u_hp; double *__u_dp; float *__u_fp; long __u_n;
    __u_i = 0; __u_j = 0; __u_stored = 0;
    while (__u_fmt[__u_j]) {
        __u_c = __u_fmt[__u_j];
        if (_u_isspace(__u_c)) { while (_u_isspace(__u_in[__u_i])) __u_i = __u_i + 1; __u_j = __u_j + 1; continue; }
        if (__u_c != 37) { if (__u_in[__u_i] != __u_c) break; __u_i = __u_i + 1; __u_j = __u_j + 1; continue; }
        __u_j = __u_j + 1;
        if (__u_fmt[__u_j] == 37) { if (__u_in[__u_i] != 37) break; __u_i = __u_i + 1; __u_j = __u_j + 1; continue; }
        __u_skip = 0; __u_width = 0; __u_lng = 0;
        if (__u_fmt[__u_j] == 42) { __u_skip = 1; __u_j = __u_j + 1; }
        while (__u_fmt[__u_j] >= 48 && __u_fmt[__u_j] <= 57) { __u_width = __u_width * 10 + (__u_fmt[__u_j] - 48); __u_j = __u_j + 1; }
        if (__u_fmt[__u_j] == 104) { __u_lng = 0 - 1; __u_j = __u_j + 1; if (__u_fmt[__u_j] == 104) { __u_lng = 0 - 2; __u_j = __u_j + 1; } }
        if (__u_fmt[__u_j] == 108) { __u_lng = 1; __u_j = __u_j + 1; if (__u_fmt[__u_j] == 108) __u_j = __u_j + 1; }
        if (__u_fmt[__u_j] == 122 || __u_fmt[__u_j] == 106 || __u_fmt[__u_j] == 116) { __u_lng = 1; __u_j = __u_j + 1; }
        if (__u_fmt[__u_j] == 76) { __u_lng = 1; __u_j = __u_j + 1; }
        __u_c = __u_fmt[__u_j]; __u_j = __u_j + 1;
        if (__u_width == 0) __u_width = 1000000000;
        if (__u_c == 99) {                                   /* %c: no space skip */
            if (__u_width == 1000000000) __u_width = 1;
            if (__u_in[__u_i] == 0) { if (__u_stored == 0) return EOF; return __u_stored; }
            if (__u_skip == 0) __u_sp = va_arg(__u_ap, char *);
            __u_n = 0;
            while (__u_n < __u_width) { if (__u_in[__u_i] == 0) break; if (__u_skip == 0) __u_sp[__u_n] = __u_in[__u_i]; __u_i = __u_i + 1; __u_n = __u_n + 1; }
            if (__u_skip == 0) __u_stored = __u_stored + 1;
            continue;
        }
        while (_u_isspace(__u_in[__u_i])) __u_i = __u_i + 1;
        if (__u_in[__u_i] == 0) { if (__u_stored == 0) return EOF; return __u_stored; }
        if (__u_c == 115) {                                  /* %s */
            if (__u_skip == 0) __u_sp = va_arg(__u_ap, char *);
            __u_n = 0;
            while (__u_n < __u_width) { if (__u_in[__u_i] == 0) break; if (_u_isspace(__u_in[__u_i])) break;
                                if (__u_skip == 0) __u_sp[__u_n] = __u_in[__u_i]; __u_i = __u_i + 1; __u_n = __u_n + 1; }
            if (__u_skip == 0) { __u_sp[__u_n] = 0; __u_stored = __u_stored + 1; }
            continue;
        }
        if (__u_c == 100 || __u_c == 105 || __u_c == 117 || __u_c == 120 || __u_c == 88 || __u_c == 111) {
            __u_base = 10;
            if (__u_c == 120 || __u_c == 88) __u_base = 16;
            if (__u_c == 111) __u_base = 8;
            __u_neg = 0; __u_any = 0; __u_uv = 0; __u_n = 0;
            if (__u_in[__u_i] == 45 || __u_in[__u_i] == 43) { if (__u_n < __u_width) { __u_neg = __u_in[__u_i] == 45; __u_i = __u_i + 1; __u_n = __u_n + 1; } }
            if (__u_c == 105 || __u_base == 16) { if (__u_in[__u_i] == 48) { if (__u_in[__u_i + 1] == 120 || __u_in[__u_i + 1] == 88) {
                if (_u_digit(__u_in[__u_i + 2], 16) >= 0) { if (__u_n + 2 < __u_width) { __u_base = 16; __u_i = __u_i + 2; __u_n = __u_n + 2; } } } } }
            if (__u_c == 105) { if (__u_base == 10) { if (__u_in[__u_i] == 48) __u_base = 8; } }
            while (__u_n < __u_width) {
                __u_v = _u_digit(__u_in[__u_i], __u_base);
                if (__u_v < 0) break;
                __u_uv = __u_uv * __u_base + __u_v; __u_i = __u_i + 1; __u_n = __u_n + 1; __u_any = 1;
            }
            if (__u_any == 0) break;
            if (__u_neg) __u_uv = 0 - __u_uv;
            if (__u_skip == 0) {
                if (__u_lng == 1) { __u_lp = va_arg(__u_ap, long *); *__u_lp = (long)__u_uv; }
                else { if (__u_lng == 0 - 1) { __u_hp = va_arg(__u_ap, short *); *__u_hp = (short)__u_uv; }
                       else { if (__u_lng == 0 - 2) { __u_sp = va_arg(__u_ap, char *); *__u_sp = (char)__u_uv; }
                              else { __u_ip = va_arg(__u_ap, int *); *__u_ip = (int)__u_uv; } } }
                __u_stored = __u_stored + 1;
            }
            continue;
        }
        if (__u_c == 102 || __u_c == 101 || __u_c == 103 || __u_c == 70 || __u_c == 69 || __u_c == 71 || __u_c == 97) {
            __u_neg = 0; __u_any = 0; __u_d = 0.0; __u_n = 0;
            if (__u_in[__u_i] == 45 || __u_in[__u_i] == 43) { if (__u_n < __u_width) { __u_neg = __u_in[__u_i] == 45; __u_i = __u_i + 1; __u_n = __u_n + 1; } }
            while (__u_n < __u_width) { __u_v = _u_digit(__u_in[__u_i], 10); if (__u_v < 0) break; __u_d = __u_d * 10.0 + __u_v; __u_i = __u_i + 1; __u_n = __u_n + 1; __u_any = 1; }
            if (__u_in[__u_i] == 46) { if (__u_n < __u_width) {
                __u_i = __u_i + 1; __u_n = __u_n + 1; __u_scale = 0.1;
                while (__u_n < __u_width) { __u_v = _u_digit(__u_in[__u_i], 10); if (__u_v < 0) break;
                                    __u_d = __u_d + __u_v * __u_scale; __u_scale = __u_scale * 0.1; __u_i = __u_i + 1; __u_n = __u_n + 1; __u_any = 1; }
            } }
            if (__u_any == 0) break;
            if (__u_in[__u_i] == 101 || __u_in[__u_i] == 69) { if (__u_n < __u_width) {
                exp = 0; __u_eneg = 0; __u_j = __u_j; 
                if (__u_in[__u_i + 1] == 45 || __u_in[__u_i + 1] == 43) { __u_eneg = __u_in[__u_i + 1] == 45;
                    if (_u_digit(__u_in[__u_i + 2], 10) >= 0) { __u_i = __u_i + 2; __u_n = __u_n + 2;
                        while (__u_n < __u_width) { __u_v = _u_digit(__u_in[__u_i], 10); if (__u_v < 0) break; exp = exp * 10 + __u_v; __u_i = __u_i + 1; __u_n = __u_n + 1; } } }
                else { if (_u_digit(__u_in[__u_i + 1], 10) >= 0) { __u_i = __u_i + 1; __u_n = __u_n + 1;
                        while (__u_n < __u_width) { __u_v = _u_digit(__u_in[__u_i], 10); if (__u_v < 0) break; exp = exp * 10 + __u_v; __u_i = __u_i + 1; __u_n = __u_n + 1; } } }
                while (exp > 0) { if (__u_eneg) __u_d = __u_d / 10.0; else __u_d = __u_d * 10.0; exp = exp - 1; }
            } }
            if (__u_neg) __u_d = 0.0 - __u_d;
            if (__u_skip == 0) {
                if (__u_lng == 1) { __u_dp = va_arg(__u_ap, double *); *__u_dp = __u_d; }
                else { __u_fp = va_arg(__u_ap, float *); *__u_fp = (float)__u_d; }
                __u_stored = __u_stored + 1;
            }
            continue;
        }
        if (__u_c == 110) {                                  /* %n */
            if (__u_skip == 0) { __u_ip = va_arg(__u_ap, int *); *__u_ip = (int)__u_i; }
            continue;
        }
        break;                                           /* an unknown conversion ends the scan */
    }
    return __u_stored;
}
static int sscanf(const char *__u_in, const char *__u_fmt, ...) {
    va_list __u_ap; int __u_r;
    va_start(__u_ap, __u_fmt);
    __u_r = vsscanf(__u_in, __u_fmt, __u_ap);
    va_end(__u_ap);
    return __u_r;
}

/* perror: the message, a colon, and errno's text -- the eleven codes
   <errno.h> defines, "Unknown error N" for the rest. */
static char *strerror(int __u_e) {
    if (__u_e == 0) return "Success";
    if (__u_e == 1) return "Operation not permitted";
    if (__u_e == 2) return "No such file or directory";
    if (__u_e == 5) return "Input/output error";
    if (__u_e == 9) return "Bad file descriptor";
    if (__u_e == 12) return "Cannot allocate memory";
    if (__u_e == 13) return "Permission denied";
    if (__u_e == 17) return "File exists";
    if (__u_e == 22) return "Invalid argument";
    if (__u_e == 28) return "No space left on device";
    if (__u_e == 33) return "Numerical argument out of domain";
    if (__u_e == 34) return "Numerical result out of range";
    return "Unknown error";
}
static void perror(const char *__u_s) {
    if (__u_s) { if (*__u_s) { fputs(__u_s, stderr); fputs(": ", stderr); } }
    fputs(strerror(errno), stderr);
    fputc(10, stderr);
}

#endif
