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
#ifdef __APPLE__
#define FILENAME_MAX 1024   /* 0.0.28 H4: the host's value (SQLite sizes path buffers by it) */
#else
#define FILENAME_MAX 4096
#endif

typedef struct _UNISA_FILE FILE;
/* A FILE* IS the descriptor cast to a pointer (fopen returns `(FILE *)(long)fd`),
   so the three standard streams used to be ((FILE *)0/1/2) -- which made
   `stdin == NULL` TRUE.  Programs test the result of fopen for NULL and then
   use the stream, so the standard ones must not look like NULL.  They are
   distinguishable, non-NULL, and `_unisa_fd` maps them back to 0/1/2, so no
   caller has to know.  The values are far above any real descriptor. */
#define _UNISA_STDIO_BASE 4611686018427387904L      /* 1 << 62 */
#define stdin  ((FILE *)(_UNISA_STDIO_BASE + 0))
#define stdout ((FILE *)(_UNISA_STDIO_BASE + 1))
#define stderr ((FILE *)(_UNISA_STDIO_BASE + 2))

int printf();

#if !__UNISA_FTRIM_LIBC || __UN__unisa_fd
static int _unisa_fd(FILE *__u_f) {
    long __u_v;
    __u_v = (long)__u_f;
    if (__u_v >= _UNISA_STDIO_BASE) return (int)(__u_v - _UNISA_STDIO_BASE);
    return (int)__u_v;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN__unisa_len
static long _unisa_len(const char *__u_s) {
    long __u_n;
    __u_n = 0;
    while (__u_s[__u_n]) __u_n = __u_n + 1;
    return __u_n;
}
#endif

static char _unisa_ch;

/* D3': the stream table and _u_st_wput come before the writers, which all go through it (a 4 KB
   buffer for streams fopen opened; the standard streams are written straight through). */
#if !__UNISA_FTRIM_LIBC || __UN__u_st_slot || __UN_setvbuf
/* ---- stream state ------------------------------------------------------
   A FILE* is a descriptor and nothing more (see the top of this file), so
   there is nowhere on the stream to remember that a read hit EOF or failed,
   or to hold a byte pushed back by ungetc.  A small fixed table keyed by the
   descriptor keeps that state.  Eight streams is what a program that uses
   feof() at all is likely to hold open; a ninth simply forgets its flags
   rather than failing, because a wrong answer from feof() is worse than none.
   [R13-0 #06 / N15] */
#include <sys/_exit.h>
#if !__UNISA_FTRIM_LIBC || __UN__u_st_slot
static int _u_st_slot(FILE *__u_f) {
    int __u_fd; int __u_i;
    __u_fd = _unisa_fd(__u_f);
    __u_i = 0;
    while (__u_i < _u_st_n) {
        if (_u_st_fd[__u_i] == __u_fd) return __u_i;
        __u_i = __u_i + 1;
    }
    if (_u_st_n >= _U_NST) return 0 - 1;
    __u_i = _u_st_n; _u_st_n = _u_st_n + 1;
    _u_st_fd[__u_i] = __u_fd;
    _u_st_eof[__u_i] = 0; _u_st_err[__u_i] = 0; _u_st_ung[__u_i] = 0 - 1;
    _u_st_bpos[__u_i] = 0; _u_st_blen[__u_i] = 0; _u_st_wlen[__u_i] = 0; _u_st_ro[__u_i] = 0;
    return __u_i;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__u_st_wput
static long _u_st_wput(FILE *__u_f, const char *__u_p, long __u_n) {
    int __u_fd; int __u_i;
    __u_fd = _unisa_fd(__u_f);
    if (__u_n <= 0) return 0;
    if ((long)__u_f >= _UNISA_STDIO_BASE || __u_fd <= 2) return _u_st_raw(__u_fd, __u_p, __u_n);
    __u_i = _u_st_slot(__u_f);
    if (__u_i < 0) return _u_st_raw(__u_fd, __u_p, __u_n);
    if (_u_st_ro[__u_i]) { _u_st_err[__u_i] = 1; errno = 9; return 0; }   /* EBADF, as glibc and BSD answer at once */
    if (_u_st_bpos[__u_i] < _u_st_blen[__u_i] || _u_st_ung[__u_i] >= 0)
        __lseek(__u_fd, 0 - (_u_st_blen[__u_i] - _u_st_bpos[__u_i]) - (_u_st_ung[__u_i] >= 0), SEEK_CUR);
    _u_st_bpos[__u_i] = 0; _u_st_blen[__u_i] = 0; _u_st_ung[__u_i] = 0 - 1;
    /* exit flushes; naming exit here keeps its body under -ftrim-libc,
       so a return from main goes through it (front_parse.c __main_ret) */
    if (__u_n < 0) exit(1);   /* never taken: a call (not an address, which Windows forward
                                 programs refuse as a callback) that keeps exit under -ftrim-libc */
    if (_u_st_wlen[__u_i] + __u_n > _U_BUFSZ) { if (_u_st_wflush(__u_i) != 0) return 0; }
    if (__u_n >= _U_BUFSZ) return _u_st_raw(__u_fd, __u_p, __u_n);
    _u_st_copy(_u_st_buf + __u_i * _U_BUFSZ + _u_st_wlen[__u_i], __u_p, __u_n);
    _u_st_wlen[__u_i] = _u_st_wlen[__u_i] + (int)__u_n;
    return __u_n;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_fputc
static int fputc(int __u_c, FILE *__u_f) {
    _unisa_ch = __u_c;
    _u_st_wput(__u_f, &_unisa_ch, 1);
    return __u_c;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_putchar
static int putchar(int __u_c) { return fputc(__u_c, stdout); }
#endif

#if !__UNISA_FTRIM_LIBC || __UN_fputs
static int fputs(const char *__u_s, FILE *__u_f) {
    _u_st_wput(__u_f, __u_s, _unisa_len(__u_s));
    return 0;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_puts
static int puts(const char *__u_s) {
    fputs(__u_s, stdout);
    _unisa_ch = 10;
    __write(1, &_unisa_ch, 1);
    return 0;
}
#endif

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
#if !__UNISA_FTRIM_LIBC || __UN_fwrite
static long fwrite(const void *__u_p, long __u_sz, long __u_n, FILE *__u_f) {
    if (__u_sz <= 0 || __u_n <= 0) return 0;
    if (__u_n > 0x7fffffffffffffff / __u_sz) return 0;
    return _u_st_wput(__u_f, (const char *)__u_p, __u_sz * __u_n) / __u_sz;
}
#endif

/* Unbuffered: a FILE * here is a file descriptor, so there is nowhere to
   keep a buffer and every character costs a read.  Correct, not fast. */

#if !__UNISA_FTRIM_LIBC || __UN_feof
static int feof(FILE *__u_f) {
    int __u_i; __u_i = _u_st_slot(__u_f);
    if (__u_i < 0) return 0;
    return _u_st_eof[__u_i];
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_ferror
static int ferror(FILE *__u_f) {
    int __u_i; __u_i = _u_st_slot(__u_f);
    if (__u_i < 0) return 0;
    return _u_st_err[__u_i];
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_clearerr
static void clearerr(FILE *__u_f) {
    int __u_i; __u_i = _u_st_slot(__u_f);
    if (__u_i < 0) return;
    _u_st_eof[__u_i] = 0; _u_st_err[__u_i] = 0;
}
#endif
/* One byte of pushback (C99 7.19.7.11 guarantees at least one).  Pushing back
   EOF fails, and pushing anything back CLEARS the end-of-file indicator. */
#if !__UNISA_FTRIM_LIBC || __UN_ungetc
static int ungetc(int __u_c, FILE *__u_f) {
    int __u_i;
    if (__u_c == EOF) return EOF;
    __u_i = _u_st_slot(__u_f);
    if (__u_i < 0) return EOF;
    _u_st_ung[__u_i] = __u_c & 255;
    _u_st_eof[__u_i] = 0;
    return __u_c & 255;
}
#endif
#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2
/* Buffering is not modelled (every read and write goes straight to the
   descriptor), so a request to change it is honoured by doing nothing --
   which C99 7.19.7.4 permits: the call may be made at any time, and only
   _IOFBF/_IOLBF/_IONBF are defined.  Returning non-zero would mean failure. */
#if !__UNISA_FTRIM_LIBC || __UN_setvbuf
static int setvbuf(FILE *__u_f, char *__u_buf, int __u_mode, long __u_size) {
    if (__u_f == NULL) return 0 - 1;
    return 0;
}
#endif
#endif

#if !__UNISA_FTRIM_LIBC || __UN_fread
static long fread(void *__u_p, long __u_sz, long __u_n, FILE *__u_f) {
    long __u_got; long __u_want; long __u_pre; int __u_i;
    __u_want = __u_sz * __u_n;
    if (__u_want <= 0) return 0;
    /* a byte pushed back by ungetc comes out first [C99 7.19.7.11]; a short
       read sets the end-of-file indicator and a failed one the error
       indicator [C99 7.19.8.1] */
    __u_pre = 0; __u_i = _u_st_slot(__u_f);
    if (__u_i >= 0 && _u_st_wlen[__u_i] > 0) _u_st_wflush(__u_i);
    if (__u_i >= 0 && _u_st_ung[__u_i] >= 0) {
        ((char *)__u_p)[0] = (char)_u_st_ung[__u_i];
        _u_st_ung[__u_i] = 0 - 1; __u_pre = 1;
    }
    /* buffered bytes next, then at most one descriptor read: straight into
       the caller for a request of a buffer or more, else a buffer refill */
    if (__u_i >= 0 && __u_want > __u_pre && _u_st_bpos[__u_i] < _u_st_blen[__u_i]) {
        long __u_k; __u_k = _u_st_blen[__u_i] - _u_st_bpos[__u_i];
        if (__u_k > __u_want - __u_pre) __u_k = __u_want - __u_pre;
        _u_st_copy((char *)__u_p + __u_pre, _u_st_buf + __u_i * _U_BUFSZ + _u_st_bpos[__u_i], __u_k);
        _u_st_bpos[__u_i] = _u_st_bpos[__u_i] + (int)__u_k; __u_pre = __u_pre + __u_k;
        if (__u_pre == __u_want) return __u_n;
    }
    __u_got = 0;
    if (__u_want > __u_pre) {
        if (__u_i >= 0 && __u_want - __u_pre < _U_BUFSZ) {
            __u_got = __read(_unisa_fd(__u_f), _u_st_buf + __u_i * _U_BUFSZ, _U_BUFSZ);
            _u_st_bpos[__u_i] = 0; _u_st_blen[__u_i] = __u_got > 0 ? (int)__u_got : 0;
            if (__u_got > __u_want - __u_pre) __u_got = __u_want - __u_pre;
            if (__u_got > 0) { _u_st_copy((char *)__u_p + __u_pre, _u_st_buf + __u_i * _U_BUFSZ, __u_got); _u_st_bpos[__u_i] = (int)__u_got; }
        } else __u_got = __read(_unisa_fd(__u_f), (char *)__u_p + __u_pre, __u_want - __u_pre);
    }
    if (__u_got < 0) { if (__u_i >= 0) _u_st_err[__u_i] = 1; __u_got = 0; }
    __u_got = __u_got + __u_pre;
    if (__u_got < __u_want && __u_i >= 0) _u_st_eof[__u_i] = 1;
    return __u_got / __u_sz;
}
#endif

/* O_* are not portable numbers: Linux and the BSDs picked different bits,
   and we hardcoded Linux's.  On macOS that turned "w" into flags nobody
   accepts; the file was never created and every read of it came back
   empty. */
/* O_RDONLY is 0, O_WRONLY is 1 and O_RDWR is 2 on Linux and on the BSDs
   alike, which is why only the CREAT/TRUNC/APPEND bits need per-host values. */
#define _U_O_WRONLY  1
#define _U_O_RDWR    2
#ifdef __linux__
#define _U_O_CREAT   64
#define _U_O_TRUNC   512
#define _U_O_APPEND  1024
#else
#define _U_O_CREAT   512
#define _U_O_TRUNC   1024
#define _U_O_APPEND  8
#endif

#if !__UNISA_FTRIM_LIBC || __UN_fopen
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
    int __u_plus;
    /* The trailing `+` means UPDATE: read AND write.  It was ignored, so "w+"
       behaved as "w" (O_WRONLY) and a tmpfile could be written but never read
       back -- fread failed and the answer looked like an empty file.  O_RDWR
       is what the `+` asks for, on the modes that allow it. */
    __u_plus = 0;
    { int __u_j; __u_j = 0; while (__u_mode[__u_j]) {
        if (__u_mode[__u_j] == 43) __u_plus = 1;
        __u_j = __u_j + 1; } }
    __u_flags = 0;                            /* 'r': O_RDONLY */
    if (__u_mode[0] == 114 && __u_plus) __u_flags = _U_O_RDWR;
    if (__u_mode[0] == 119)                   /* 'w' */
        __u_flags = _U_O_WRONLY | _U_O_CREAT | _U_O_TRUNC;
    if (__u_mode[0] == 119 && __u_plus)
        __u_flags = _U_O_RDWR | _U_O_CREAT | _U_O_TRUNC;
    if (__u_mode[0] == 97)                    /* 'a' */
        __u_flags = _U_O_WRONLY | _U_O_CREAT | _U_O_APPEND;
    if (__u_mode[0] == 97 && __u_plus)
        __u_flags = _U_O_RDWR | _U_O_CREAT | _U_O_APPEND;
    /* 0644.  Passing no mode at all left it at 0, so the file we had just
       created could not be opened again. */
    __u_fd = __open((char *)__u_path, __u_flags, 420);
#endif
    if (__u_fd < 0) {
        /* The gate answers -errno on Linux and Darwin (see the __write audit
           above); Windows' CreateFileA gate answers -1.  Without this line
           errno stayed 0 and sbase said "fopen /nonexistent: Success"
           [R13-0b #25].  A bare -1 is reported as ENOENT: an open that
           failed with no code is almost always a missing file. */
        errno = 0 - __u_fd;
#ifdef _WIN32
        if (errno == 1) errno = ENOENT;
#endif
        return NULL;
    }
    {   int __u_s2; int __u_j2; int __u_ro; __u_ro = __u_mode[0] == 114; __u_j2 = 0;
        while (__u_mode[__u_j2]) { if (__u_mode[__u_j2] == 43) __u_ro = 0; __u_j2 = __u_j2 + 1; }
        __u_s2 = _u_st_slot((FILE *)(long)__u_fd); if (__u_s2 >= 0) _u_st_ro[__u_s2] = __u_ro; }
    return (FILE *)(long)__u_fd;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_fgetc
static int fgetc(FILE *__u_f) {
    unsigned char __u_c;
    int __u_i;
    /* a byte pushed back by ungetc comes out before the descriptor does, and
       reading it is not an error or an end-of-file [C99 7.19.7.11] */
    __u_i = _u_st_slot(__u_f);
    if (__u_i >= 0) {
        if (_u_st_ung[__u_i] >= 0) {
            __u_c = (unsigned char)_u_st_ung[__u_i];
            _u_st_ung[__u_i] = 0 - 1;
            return (int)__u_c;
        }
    }
    if (fread(&__u_c, 1, 1, __u_f) != 1) {
        /* the flag feof() reports is set HERE, where the short read happened;
           without it feof() answered 0 forever.  A successful read clears it
           [C99 7.19.7.1, 7.19.7.2] */
        if (__u_i >= 0) _u_st_eof[__u_i] = 1;
        return EOF;
    }
    if (__u_i >= 0) { _u_st_eof[__u_i] = 0; _u_st_err[__u_i] = 0; }
    return (int)__u_c;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_getc
static int getc(FILE *__u_f) { return fgetc(__u_f); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_getchar
static int getchar(void) { return fgetc(stdin); }
#endif

#if !__UNISA_FTRIM_LIBC || __UN_vprintf
static int vprintf(const char *__u_fmt, va_list __u_ap) {
    return _u_vfmt(NULL, 0 - 1, NULL, __u_fmt, __u_ap);
}
#endif

#define L_tmpnam 64
#define TMP_MAX 238328
/* tmpnam (C99 7.19.4.4): "/tmp/untm" plus a counter.  No existence check
   (the name may be taken by another process; C99 only promises it is
   different from earlier results in this program). */
#if !__UNISA_FTRIM_LIBC || __UN_tmpnam
static char *tmpnam(char *__u_s) {
    static char __u_own[64]; static int __u_n;
    int __u_v; int __u_k; char *__u_p;
    __u_p = __u_s ? __u_s : __u_own;
    __u_p[0] = 47; __u_p[1] = 116; __u_p[2] = 109; __u_p[3] = 112; __u_p[4] = 47;
    __u_p[5] = 117; __u_p[6] = 110; __u_p[7] = 116; __u_p[8] = 109;
    __u_n = __u_n + 1; __u_v = __u_n; __u_k = 9;
    while (__u_k < 19) { __u_p[__u_k] = (char)(48 + __u_v % 10); __u_v = __u_v / 10; __u_k = __u_k + 1; }
    __u_p[19] = 0;
    return __u_p;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_tmpfile
/* A file with no name that disappears when it is closed: open it, unlink it
   immediately, and hand back the stream.  /tmp because the subset has no
   TMPDIR lookup and every host this compiler targets has one.
   The name has to be unique without a pid or mkstemp, so it is a counter mixed
   through a small LCG (not rand(): stdio.h must not pull in stdlib.h, and a program
   that includes only <stdio.h> would otherwise stop at "undefined function 'rand'");
   a collision shows up as fopen failing, and then we try again.
   [R13-0 #06 / N15] */
#if !__UNISA_FTRIM_LIBC || __UN_tmpfile
static FILE *tmpfile(void) {
    static int __u_seq;
    char __u_p[64];
    int __u_i; int __u_try;
    FILE *__u_f;
    __u_try = 0;
    while (__u_try < 16) {
        char __u_pfx[6];
        int __u_v;
        __u_pfx[0] = 47; __u_pfx[1] = 116; __u_pfx[2] = 109; __u_pfx[3] = 112; __u_pfx[4] = 47;
        __u_i = 0;
        while (__u_i < 5) { __u_p[__u_i] = __u_pfx[__u_i]; __u_i = __u_i + 1; }
        __u_p[5] = 117; __u_p[6] = 110;                  /* "un" */
        __u_v = (int)((unsigned int)__u_seq * 1103515245u + 12345u);
        __u_seq = __u_seq + 1;
        /* _u_digits fills a buffer of 24 from the END and returns where the
           digits start, so the decimal text lands in __u_d[__u_s..23] */
        {   char __u_d[24];
            long __u_s;
            long __u_j;
            __u_s = _u_digits(__u_d, (unsigned long)__u_v & 2147483647, 10, 0);
            __u_j = __u_s;
            __u_i = 7;
            while (__u_j < 24) { __u_p[__u_i] = __u_d[__u_j]; __u_i = __u_i + 1; __u_j = __u_j + 1; }
        }
        __u_p[__u_i] = 46; __u_p[__u_i + 1] = 116; __u_p[__u_i + 2] = 109; __u_p[__u_i + 3] = 112;
        __u_p[__u_i + 4] = 0;
        __u_f = fopen(__u_p, "w+");
        if (__u_f != NULL) { __unlink(__u_p); return __u_f; }
        __u_try = __u_try + 1;
    }
    return NULL;
}
#endif
#endif

#if !__UNISA_FTRIM_LIBC || __UN_fgets
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
#endif

/* freopen: open the new file and move it onto the stream's descriptor, so
   the same FILE * (stdin/stdout/stderr included) now refers to it.  Windows:
   a HANDLE cannot be renumbered, so the new stream is returned instead. */
#ifndef __UNISA_PYFRONT   /* the Python control-group front end folds one tape across targets */
#if !__UNISA_FTRIM_LIBC || __UN_freopen
static FILE *freopen(const char *__u_path, const char *__u_mode, FILE *__u_f) {
    FILE *__u_n; long __u_r;
    if (__u_path == 0) { errno = EINVAL; return NULL; }
    _u_st_wflush(_u_st_slot(__u_f));
    __u_n = fopen(__u_path, __u_mode);
    if (__u_n == NULL) return NULL;
#ifdef _WIN32
    return __u_n;
#else
#ifdef __APPLE__
#ifdef __x86_64__
    __u_r = __syscall6(0x2000000L + 90, (long)__u_n, (long)__u_f, 0, 0, 0);
#else
    __u_r = __syscall6(90, (long)__u_n, (long)__u_f, 0, 0, 0);
#endif
#elif defined(__x86_64__)
    __u_r = __syscall6(33, (long)__u_n, (long)__u_f, 0, 0, 0);
#else
    __u_r = __syscall6(24, (long)__u_n, (long)__u_f, 0, 0, 0);     /* dup3 */
#endif
    __close((long)__u_n);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return NULL; }
    /* the old stream's buffer and flags belong to the file it had: drop them,
       take the new mode, and free the slot of the descriptor just closed
       (lua's loadfile reopens "rb" after one getc and reads from the start) */
    {   int __u_i; int __u_k;
        __u_k = _u_st_slot(__u_n); __u_i = _u_st_slot(__u_f);
        if (__u_i >= 0) {
            _u_st_eof[__u_i] = 0; _u_st_err[__u_i] = 0; _u_st_ung[__u_i] = 0 - 1;
            _u_st_bpos[__u_i] = 0; _u_st_blen[__u_i] = 0; _u_st_wlen[__u_i] = 0;
            if (__u_k >= 0) _u_st_ro[__u_i] = _u_st_ro[__u_k];
        }
        if (__u_k >= 0) {
            _u_st_n = _u_st_n - 1;
            _u_st_fd[__u_k] = _u_st_fd[_u_st_n]; _u_st_eof[__u_k] = _u_st_eof[_u_st_n];
            _u_st_err[__u_k] = _u_st_err[_u_st_n]; _u_st_ung[__u_k] = _u_st_ung[_u_st_n];
            _u_st_copy(_u_st_buf + __u_k * _U_BUFSZ, _u_st_buf + _u_st_n * _U_BUFSZ, _U_BUFSZ);
            _u_st_bpos[__u_k] = _u_st_bpos[_u_st_n]; _u_st_blen[__u_k] = _u_st_blen[_u_st_n];
            _u_st_wlen[__u_k] = _u_st_wlen[_u_st_n]; _u_st_ro[__u_k] = _u_st_ro[_u_st_n];
        }
    }
    return __u_f;
#endif
}
#endif
#endif
#if !__UNISA_FTRIM_LIBC || __UN_fclose
/* The standard streams are not ours to close: `fclose(stdin)` on a POSIX libc
   is defined to do the work and fail, but closing descriptor 0 here would take
   the process's input away from every later read. */
#if !__UNISA_FTRIM_LIBC || __UN_fclose
static int fclose(FILE *__u_f) {
    int __u_i;
    if ((long)__u_f >= _UNISA_STDIO_BASE) return 0;
    /* free the stream's state slot: a later fopen that gets the same
       descriptor must not inherit this stream's eof/err/pushback */
    __u_i = _u_st_slot(__u_f);
    if (__u_i >= 0) {
        _u_st_wflush(__u_i);
        _u_st_n = _u_st_n - 1;
        _u_st_fd[__u_i] = _u_st_fd[_u_st_n]; _u_st_eof[__u_i] = _u_st_eof[_u_st_n];
        _u_st_err[__u_i] = _u_st_err[_u_st_n]; _u_st_ung[__u_i] = _u_st_ung[_u_st_n];
        _u_st_copy(_u_st_buf + __u_i * _U_BUFSZ, _u_st_buf + _u_st_n * _U_BUFSZ, _U_BUFSZ);
        _u_st_bpos[__u_i] = _u_st_bpos[_u_st_n]; _u_st_blen[__u_i] = _u_st_blen[_u_st_n];
        _u_st_wlen[__u_i] = _u_st_wlen[_u_st_n]; _u_st_ro[__u_i] = _u_st_ro[_u_st_n];
    }
    return __close(_unisa_fd(__u_f));
}
#endif
#endif
/* The stream position is the descriptor offset less what the read buffer
   and the pushback still hold; fseek and rewind drop both (and clear eof,
   C99 7.19.9.2).  [S-15 D2, D3] */
#if !__UNISA_FTRIM_LIBC || __UN_fseek
static int fseek(FILE *__u_f, long __u_off, int __u_whence) {
    int __u_i; __u_i = _u_st_slot(__u_f);
    if (__u_i >= 0) _u_st_wflush(__u_i);
    if (__u_i >= 0) {
        if (__u_whence == SEEK_CUR) __u_off = __u_off - (_u_st_blen[__u_i] - _u_st_bpos[__u_i]) - (_u_st_ung[__u_i] >= 0);
        _u_st_bpos[__u_i] = 0; _u_st_blen[__u_i] = 0; _u_st_ung[__u_i] = 0 - 1; _u_st_eof[__u_i] = 0;
    }
    return __lseek(_unisa_fd(__u_f), __u_off, __u_whence) < 0 ? -1 : 0;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_ftell
static long ftell(FILE *__u_f) {
    long __u_r; int __u_i;
    __u_i = _u_st_slot(__u_f);
    if (__u_i >= 0) _u_st_wflush(__u_i);
    __u_r = __lseek(_unisa_fd(__u_f), 0, SEEK_CUR);
    if (__u_r >= 0 && __u_i >= 0) __u_r = __u_r - (_u_st_blen[__u_i] - _u_st_bpos[__u_i]) - (_u_st_ung[__u_i] >= 0);
    return __u_r;
}
#endif
/* POSIX spellings lua's LUA_USE_POSIX build asks for (0.0.34 L1b): off_t is
   long here, so fseeko/ftello are fseek/ftell; there is one thread, so the
   stream locks have nothing to do and getc_unlocked is getc. */
#if !__UNISA_FTRIM_LIBC || __UN_fseeko
static int fseeko(FILE *__u_f, long __u_off, int __u_whence) { return fseek(__u_f, __u_off, __u_whence); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_ftello
static long ftello(FILE *__u_f) { return ftell(__u_f); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_flockfile
static void flockfile(FILE *__u_f) { (void)__u_f; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_funlockfile
static void funlockfile(FILE *__u_f) { (void)__u_f; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_getc_unlocked
static int getc_unlocked(FILE *__u_f) { return fgetc(__u_f); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_rewind
static void rewind(FILE *__u_f) { fseek(__u_f, 0, SEEK_SET); clearerr(__u_f); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_remove
static int remove(const char *__u_path) { return __unlink((char *)__u_path) < 0 ? -1 : 0; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_rename
static int rename(const char *__u_from, const char *__u_to) {
    return __rename((char *)__u_from, (char *)__u_to) < 0 ? -1 : 0;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_fflush
static int fflush(FILE *__u_f) {
    if (__u_f == NULL) { _u_st_flushall(); return 0; }
    if ((long)__u_f >= _UNISA_STDIO_BASE || _unisa_fd(__u_f) <= 2) return 0;
    return _u_st_wflush(_u_st_slot(__u_f));
}
#endif

/* ---- a runtime formatter ------------------------------------------------
 * `printf` is desugared by the walker against its static format string, which
 * is the fast path and the one that does not need varargs.  Everything that
 * takes a format at RUN time is written here, in the subset itself. */

#if !__UNISA_FTRIM_LIBC || __UN__u_put
static void _u_put(char *__u_out, long __u_cap, long *__u_n, FILE *__u_f, int __u_c) {
    char __u_ch;
    if (__u_out != NULL) {
        if (__u_cap < 0 | *__u_n < __u_cap - 1) __u_out[*__u_n] = __u_c;
    } else {
        __u_ch = __u_c;
        _u_st_wput(__u_f, &__u_ch, 1);
    }
    *__u_n = *__u_n + 1;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN__u_digits
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
#endif

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
#if !__UNISA_FTRIM_LIBC || __UN__u_dexp
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
#endif

/* keep r digits of dig[0..nd), rounding to nearest, ties to even; returns
   the new count, and bumps *x when the rounding carries out (9.99 -> 10.0) */
#if !__UNISA_FTRIM_LIBC || __UN__u_round
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
#endif

/* the digit at position p of the number (0 = first integer digit) */
#if !__UNISA_FTRIM_LIBC || __UN__u_dat
static int _u_dat(char *__u_dig, int __u_nd, int __u_x, int __u_p) {
    if (__u_p < 0) return 48;
    if (__u_p >= __u_nd) return 48;
    return __u_dig[__u_p];
}
#endif

/* %f %e %g (and upper case) of `bits` into out[]; returns the length.
   Sign, width and padding are the caller's. */
#if !__UNISA_FTRIM_LIBC || __UN__u_ffmt
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
    __u_upper = __u_c == 70 || __u_c == 69 || __u_c == 71 || __u_c == 65;
    if (((__u_bits >> 52) & 2047) == 2047) {
        char *__u_w;
        if (__u_bits & 4503599627370495) __u_w = __u_upper ? "NAN" : "nan";
        else __u_w = __u_upper ? "INF" : "inf";
        __u_out[0] = __u_w[0]; __u_out[1] = __u_w[1]; __u_out[2] = __u_w[2];
        return 3;
    }
    if ((__u_c | 32) == 97) {                     /* %a %A: C99 7.19.6.1p8 */
        unsigned long __u_m; int __u_lead; int __u_hd; int __u_d;
        __u_m = __u_bits & 4503599627370495; __u_e = (int)((__u_bits >> 52) & 2047);
        if (__u_e == 0) {                         /* subnormal: normalised, as BSD libc prints it */
            __u_lead = 0; __u_e = 0;
            if (__u_m) { __u_lead = 1; __u_e = 0 - 1022; while ((__u_m & 4503599627370496) == 0) { __u_m = __u_m << 1; __u_e = __u_e - 1; } __u_m = __u_m & 4503599627370495; }
        }
        else { __u_lead = 1; __u_e = __u_e - 1023; }
        __u_nd = 13;
        if (__u_prec >= 0 && __u_prec < 13) {
            unsigned long __u_rem; unsigned long __u_half; int __u_sh;
            __u_sh = (13 - __u_prec) * 4;
            __u_rem = __u_m & (((unsigned long)1 << __u_sh) - 1); __u_half = (unsigned long)1 << (__u_sh - 1);
            __u_m = __u_m >> __u_sh;
            if (__u_rem > __u_half || (__u_rem == __u_half && (__u_m & 1))) __u_m = __u_m + 1;
            if (__u_m >> (__u_prec * 4)) { __u_m = __u_m & (((unsigned long)1 << (__u_prec * 4)) - 1); __u_lead = __u_lead + 1; }
            __u_nd = __u_prec; __u_hd = __u_prec;
        } else {
            if (__u_prec < 0) { while (__u_nd > 0 && (__u_m & 15) == 0) { __u_m = __u_m >> 4; __u_nd = __u_nd - 1; } __u_hd = __u_nd; }
            else __u_hd = __u_prec;
        }
        __u_n = 0;
        __u_out[0] = 48; __u_out[1] = __u_upper || __u_c == 65 ? 88 : 120; __u_out[2] = 48 + __u_lead; __u_n = 3;
        if (__u_hd > 0 || __u_alt) { __u_out[__u_n] = 46; __u_n = __u_n + 1; }
        __u_j = 0;
        while (__u_j < __u_hd) {
            __u_d = 0;
            if (__u_j < __u_nd) __u_d = (int)((__u_m >> ((__u_nd - 1 - __u_j) * 4)) & 15);
            __u_out[__u_n] = __u_d < 10 ? 48 + __u_d : (__u_c == 65 ? 55 : 87) + __u_d; __u_n = __u_n + 1; __u_j = __u_j + 1;
        }
        __u_out[__u_n] = __u_c == 65 ? 80 : 112; __u_n = __u_n + 1;
        if (__u_e < 0) { __u_out[__u_n] = 45; __u_ee = 0 - __u_e; } else { __u_out[__u_n] = 43; __u_ee = __u_e; }
        __u_n = __u_n + 1;
        if (__u_ee >= 1000) { __u_out[__u_n] = 48 + __u_ee / 1000; __u_n = __u_n + 1; }
        if (__u_ee >= 100) { __u_out[__u_n] = 48 + (__u_ee / 100) % 10; __u_n = __u_n + 1; }
        if (__u_ee >= 10) { __u_out[__u_n] = 48 + (__u_ee / 10) % 10; __u_n = __u_n + 1; }
        __u_out[__u_n] = 48 + __u_ee % 10; __u_n = __u_n + 1;
        return __u_n;
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
#endif

#if !__UNISA_FTRIM_LIBC || __UN__u_vfmt
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
    int __u_hh;
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
        __u_lng = 0; __u_hh = 0;
        while (__u_fmt[__u_i] == 104 | __u_fmt[__u_i] == 108 | __u_fmt[__u_i] == 122 | __u_fmt[__u_i] == 106
               | __u_fmt[__u_i] == 116 | __u_fmt[__u_i] == 76) {
            if (__u_fmt[__u_i] == 104) __u_hh = __u_hh + 1;   /* h, hh: narrow */
            else __u_lng = 1;                                /* l, z, j, t, L are 64-bit */
            __u_i = __u_i + 1;
        }
        __u_c = __u_fmt[__u_i];
        __u_i = __u_i + 1;
        if (__u_c == 37) { _u_put(__u_out, __u_cap, &__u_n, __u_f, 37); continue; }
        __u_sp = NULL; __u_sign = 0; __u_base = 10; __u_upper = 0; __u_neg = 0;
        if (__u_c == 102 | __u_c == 70 | __u_c == 101 | __u_c == 69 | __u_c == 103 | __u_c == 71 | __u_c == 97 | __u_c == 65) {
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
                    /* C99 7.19.6.1p7: hh converts to signed/unsigned char and h
                       to short, BEFORE the value is printed -- `%hhd` of 300 is
                       44, and going through a char keeps the sign (`%hhd` of 200
                       is -56).  Only the 32-bit narrowing was done, so 300 and
                       70000 printed whole. */
                    if (__u_hh == 2) { __u_sv = (signed char)__u_sv; }
                    else { if (__u_hh == 1) { __u_sv = (short)__u_sv; }
                           else { if (__u_lng == 0) __u_sv = (int)__u_sv; } }
                    if (__u_sv < 0) { __u_sign = 1; __u_uv = 0 - __u_sv; } else __u_uv = __u_sv;
                    __u_neg = __u_sign;
                } else {
                    if (__u_c == 120) { __u_base = 16; }
                    if (__u_c == 88) { __u_base = 16; __u_upper = 1; }
                    if (__u_c == 111) { __u_base = 8; }
                    if (__u_c == 112) { __u_base = 16; }
                    __u_uv = va_arg(__u_ap, unsigned long);
                    if (__u_hh == 2) __u_uv = __u_uv & 255;       /* %hhu, %hhx */
                    else { if (__u_hh == 1) __u_uv = __u_uv & 65535;   /* %hu, %hx */
                           else { if (__u_lng == 0) {
                               if (__u_c == 117 | __u_c == 120 | __u_c == 88 | __u_c == 111)
                                   __u_uv = __u_uv & 4294967295;
                           } } }
                }
                __u_start = _u_digits(__u_buf, __u_uv, __u_base, __u_upper);
                __u_len = 24 - __u_start;
                /* C99 7.19.6.1p8: precision 0 and value 0 give no digits (%#o still gives "0") */
                if (__u_prec == 0 && __u_uv == 0 && __u_c != 112) { __u_start = 24; __u_len = 0; }
                /* %p: 0x and the hex digits, as both host C libraries print it (7.19.6.1p8
                   leaves the form implementation-defined) */
                if (__u_c == 112) { __u_start = __u_start - 2; __u_buf[__u_start] = 48;
                                    __u_buf[__u_start + 1] = 120; __u_len = __u_len + 2; }
                /* C99 7.19.6.1p6 -- the `#` flag: %#x / %#X get 0x / 0X, %#o gets a
                   leading 0.  The flag was parsed (__u_alt) and passed to the FLOAT
                   formatter, but the integer path never looked at it, so `%#x` of
                   255 printed `ff`.  For %#o a single leading zero is enough even
                   when the digits already start with one (`%#o` of 8 is "010", not
                   "0010").  A value of 0 gets no prefix for x/X (7.19.6.1p6 says
                   the result is "0" either way). */
                if (__u_alt) {
                    if (__u_c == 111) {
                        if (__u_buf[__u_start] != 48) {
                            __u_start = __u_start - 1; __u_buf[__u_start] = 48; __u_len = __u_len + 1; } }
                    else { if ((__u_c == 120 | __u_c == 88) && __u_uv != 0) {
                        __u_start = __u_start - 2;
                        __u_buf[__u_start] = 48;
                        __u_buf[__u_start + 1] = __u_c == 88 ? 88 : 120;
                        __u_len = __u_len + 2; } }
                }
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
                | __u_c == 103 | __u_c == 71 | __u_c == 97 | __u_c == 65) {
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
#endif

#if !__UNISA_FTRIM_LIBC || __UN_vsprintf
static int vsprintf(char *__u_b, const char *__u_fmt, va_list __u_ap) {
    return _u_vfmt(__u_b, 0 - 1, NULL, __u_fmt, __u_ap);
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_vsnprintf
static int vsnprintf(char *__u_b, long __u_cap, const char *__u_fmt, va_list __u_ap) {
    return _u_vfmt(__u_b, __u_cap, NULL, __u_fmt, __u_ap);
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_vfprintf
static int vfprintf(FILE *__u_f, const char *__u_fmt, va_list __u_ap) {
    return _u_vfmt(NULL, 0, __u_f, __u_fmt, __u_ap);
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_sprintf
static int sprintf(char *__u_b, const char *__u_fmt, ...) {
    va_list __u_ap; int __u_r;
    va_start(__u_ap, __u_fmt);
    __u_r = _u_vfmt(__u_b, 0 - 1, NULL, __u_fmt, __u_ap);
    va_end(__u_ap);
    return __u_r;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_snprintf
static int snprintf(char *__u_b, long __u_cap, const char *__u_fmt, ...) {
    va_list __u_ap; int __u_r;
    va_start(__u_ap, __u_fmt);
    __u_r = _u_vfmt(__u_b, __u_cap, NULL, __u_fmt, __u_ap);
    va_end(__u_ap);
    return __u_r;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_fprintf
static int fprintf(FILE *__u_f, const char *__u_fmt, ...) {
    va_list __u_ap; int __u_r;
    va_start(__u_ap, __u_fmt);
    __u_r = _u_vfmt(NULL, 0, __u_f, __u_fmt, __u_ap);
    va_end(__u_ap);
    return __u_r;
}
#endif
/* The compiler desugars `printf` against a STATIC format string [W-9] -- the
 * fast path, and the common one.  A format that is not a literal cannot be
 * desugared at all, so it becomes an ordinary variadic call on this. */
#if !__UNISA_FTRIM_LIBC || __UN_printf
static int printf(const char *__u_fmt, ...) {
    va_list __u_ap; int __u_r;
    va_start(__u_ap, __u_fmt);
    __u_r = _u_vfmt(NULL, 0, stdout, __u_fmt, __u_ap);
    va_end(__u_ap);
    return __u_r;
}
#endif
/* ---- sscanf: the conversions a program reads numbers and words with --
   d i u x o c s f e g, the l and h length modifiers, a field width, `*`
   to discard, `%%`, and white space in the format matching any amount of
   it.  Returns the number of conversions stored, or EOF when the input
   ran out before the first one -- the same contract as the platform's,
   which is what a program comparing the two observes. [S-15 D2] */
#if !__UNISA_FTRIM_LIBC || __UN__u_isspace
static int _u_isspace(int __u_c) { return __u_c == 32 || (__u_c >= 9 && __u_c <= 13); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN__u_digit
static int _u_digit(int __u_c, int __u_base) {
    int __u_v;
    __u_v = 0 - 1;
    if (__u_c >= 48 && __u_c <= 57) __u_v = __u_c - 48;
    if (__u_c >= 97 && __u_c <= 122) __u_v = __u_c - 97 + 10;
    if (__u_c >= 65 && __u_c <= 90) __u_v = __u_c - 65 + 10;
    if (__u_v >= __u_base) return 0 - 1;
    return __u_v;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_vsscanf
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
#endif
#if !__UNISA_FTRIM_LIBC || __UN_sscanf
static int sscanf(const char *__u_in, const char *__u_fmt, ...) {
    va_list __u_ap; int __u_r;
    va_start(__u_ap, __u_fmt);
    __u_r = vsscanf(__u_in, __u_fmt, __u_ap);
    va_end(__u_ap);
    return __u_r;
}
#endif

/* perror: the message, a colon, and errno's text -- the eleven codes
   <errno.h> defines, "Unknown error N" for the rest. */
#if !__UNISA_FTRIM_LIBC || __UN_strerror
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
    if (__u_e == 25) return "Inappropriate ioctl for device";
    if (__u_e == 28) return "No space left on device";
    if (__u_e == 33) return "Numerical argument out of domain";
    if (__u_e == 34) return "Numerical result out of range";
    return "Unknown error";
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_perror
static void perror(const char *__u_s) {
    if (__u_s) { if (*__u_s) { fputs(__u_s, stderr); fputs(": ", stderr); } }
    fputs(strerror(errno), stderr);
    fputc(10, stderr);
}
#endif

/* getline (POSIX 2008), 0.0.18 R18-5: the line with its newline, the buffer
   grown with realloc; -1 at end of file with nothing read. */
#if !__UNISA_FTRIM_LIBC || __UN_getline
#include <stdlib.h>
#if !__UNISA_FTRIM_LIBC || __UN_getline
static long getline(char **__u_line, size_t *__u_cap, FILE *__u_f) {
    size_t __u_n; int __u_c; char *__u_p;
    if (__u_line == NULL || __u_cap == NULL) { errno = EINVAL; return -1; }
    if (*__u_line == NULL || *__u_cap == 0) {
        __u_p = (char *)realloc(*__u_line, 128); if (__u_p == NULL) { errno = ENOMEM; return -1; }
        *__u_line = __u_p; *__u_cap = 128;
    }
    __u_n = 0;
    while (1) {
        __u_c = fgetc(__u_f);
        if (__u_c == EOF) break;
        if (__u_n + 2 > *__u_cap) {
            __u_p = (char *)realloc(*__u_line, *__u_cap * 2); if (__u_p == NULL) { errno = ENOMEM; return -1; }
            *__u_line = __u_p; *__u_cap = *__u_cap * 2;
        }
        (*__u_line)[__u_n] = (char)__u_c; __u_n = __u_n + 1;
        if (__u_c == 10) break;
    }
    if (__u_n == 0) return -1;
    (*__u_line)[__u_n] = 0;
    return (long)__u_n;
}
#endif
#endif
/* fdopen (POSIX), 0.0.19: a FILE * IS its descriptor here (see FILE above);
   0..2 are the standard streams' values.  The mode is not checked against the
   descriptor's flags (POSIX allows EINVAL; not detected). */
#if !__UNISA_FTRIM_LIBC || __UN_fdopen
static FILE *fdopen(int __u_fd, const char *__u_mode) {
    if (__u_fd < 0) { errno = 9; return NULL; }   /* EBADF */
    if (__u_fd < 3) return (FILE *)(_UNISA_STDIO_BASE + __u_fd);
    return (FILE *)(long)__u_fd;
}
#endif
/* popen/pclose (POSIX), 0.0.34: a pipe (socketpair on macOS, as pipe() in unistd.h), a fork, and
   /bin/sh -c in the child with the pipe end on fd 0 or 1 -- the raw calls system() uses, so this
   header still needs no <unistd.h>.  pclose waits for the child and returns its wait status. */
#if !defined(_WIN32) && !defined(__UNISA_PYFRONT)   /* as system(): the Python front end folds one tape across targets */
#if defined(__APPLE__) && defined(__x86_64__)
#define _U_PO_SC(n) (0x2000000L + (n))
#else
#define _U_PO_SC(n) (n)
#endif
#if !__UNISA_FTRIM_LIBC || __UN_popen || __UN_pclose
static int _u_po_fd[16]; static long _u_po_pid[16]; static int _u_po_n;
#endif
#if !__UNISA_FTRIM_LIBC || __UN_popen
static FILE *popen(const char *__u_cmd, const char *__u_mode) {
    static char *__u_env[512]; char *__u_argv[4]; int __u_p[2]; long __u_pid; long __u_me; long __u_r;
    int __u_k; int __u_n; int __u_rd; int __u_mine; int __u_kid;
    __u_rd = __u_mode[0] == 114;
    if ((!__u_rd && __u_mode[0] != 119) || _u_po_n >= 16) { errno = 22; return NULL; }
#ifdef __APPLE__
    __u_r = __syscall6(_U_PO_SC(135), 1, 1, 0, (long)__u_p, 0);
#elif defined(__x86_64__)
    __u_r = __syscall6(293, (long)__u_p, 0, 0, 0, 0);
#else
    __u_r = __syscall6(59, (long)__u_p, 0, 0, 0, 0);
#endif
    if (__u_r < 0) { errno = (int)(0 - __u_r); return NULL; }
    __u_mine = __u_rd ? __u_p[0] : __u_p[1]; __u_kid = __u_rd ? __u_p[1] : __u_p[0];
    __u_argv[0] = "sh"; __u_argv[1] = "-c"; __u_argv[2] = (char *)__u_cmd; __u_argv[3] = 0;
    __u_n = 0; __u_k = __argc() + 1;
    while (__u_n < 511 && __argv(__u_k) != 0) { __u_env[__u_n] = __argv(__u_k); __u_n = __u_n + 1; __u_k = __u_k + 1; }
    __u_env[__u_n] = 0;
    _u_st_flushall();
#ifdef __APPLE__
    __u_me = __syscall6(_U_PO_SC(20), 0, 0, 0, 0, 0);
    __u_pid = __syscall6(_U_PO_SC(2), 0, 0, 0, 0, 0);
    if (__u_pid >= 0 && __syscall6(_U_PO_SC(20), 0, 0, 0, 0, 0) != __u_me) __u_pid = 0;
#elif defined(__x86_64__)
    __u_me = 0; __u_pid = __syscall6(57, 0, 0, 0, 0, 0);
#else
    __u_me = 0; __u_pid = __syscall6(220, 17, 0, 0, 0, 0);
#endif
    if (__u_pid < 0) { __close(__u_p[0]); __close(__u_p[1]); errno = (int)(0 - __u_pid); return NULL; }
    if (__u_pid == 0) {
        __u_k = __u_rd ? 1 : 0;
        if (__u_kid != __u_k) {
#ifdef __APPLE__
            __syscall6(_U_PO_SC(90), __u_kid, __u_k, 0, 0, 0);
#elif defined(__x86_64__)
            __syscall6(33, __u_kid, __u_k, 0, 0, 0);
#else
            __syscall6(24, __u_kid, __u_k, 0, 0, 0);
#endif
            __close(__u_kid);
        }
        __close(__u_mine);
        for (__u_k = 0; __u_k < _u_po_n; __u_k = __u_k + 1) __close(_u_po_fd[__u_k]);
#if defined(__APPLE__) || defined(__x86_64__)
        __syscall6(_U_PO_SC(59), (long)"/bin/sh", (long)__u_argv, (long)__u_env, 0, 0);
#else
        __syscall6(221, (long)"/bin/sh", (long)__u_argv, (long)__u_env, 0, 0);
#endif
        __exit(127);
    }
    __close(__u_kid);
    _u_po_fd[_u_po_n] = __u_mine; _u_po_pid[_u_po_n] = __u_pid; _u_po_n = _u_po_n + 1;
    return fdopen(__u_mine, __u_mode);
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_pclose
static int pclose(FILE *__u_f) {
    long __u_pid; int __u_k; int __u_fd; int __u_st;
    __u_fd = _unisa_fd(__u_f); __u_pid = -1;
    for (__u_k = 0; __u_k < _u_po_n; __u_k = __u_k + 1) if (_u_po_fd[__u_k] == __u_fd) {
        __u_pid = _u_po_pid[__u_k]; _u_po_n = _u_po_n - 1;
        _u_po_fd[__u_k] = _u_po_fd[_u_po_n]; _u_po_pid[__u_k] = _u_po_pid[_u_po_n]; break; }
    if (__u_pid < 0) { errno = 10; return -1; }   /* ECHILD */
    fclose(__u_f);
    __u_st = 0;
#ifdef __APPLE__
    if (__syscall6(_U_PO_SC(7), __u_pid, (long)&__u_st, 0, 0, 0) < 0) return -1;
#elif defined(__x86_64__)
    if (__syscall6(61, __u_pid, (long)&__u_st, 0, 0, 0) < 0) return -1;
#else
    if (__syscall6(260, __u_pid, (long)&__u_st, 0, 0, 0) < 0) return -1;
#endif
    return __u_st;
}
#endif
#endif
#endif
