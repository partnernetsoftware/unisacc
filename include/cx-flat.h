/* include/cx-flat.h: written by tools/cx-flatten.cx from include/cx-nested.h and the headers it includes, do not edit; unisacc reads it through include/cx.h */
/* cx.h -- 0.0.34 X1: the small toolkit .cx scripts (#!/usr/bin/env unisacc.com -run) need to replace
 * .sh/.py helpers: run a command and take its output, read/write a whole file, sha256, and a command
 * under a time limit.  Plain C99 over the carried libc.  Windows has no fork: cx_run and
 * cx_runv_timeout call posix_spawnp, and cx_walk calls opendir.  Every body sits alone in its own guard (tests/libneed.sh). */
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

/* Minimal <stdio.h> for the unisa C subset.
 * printf is desugared by the walker against its static format string [W-9];
 * everything else here is ordinary C over the `.sys` gate, so it needs no
 * linker and no compiler support.  A FILE * is a file descriptor in a
 * pointer's clothing. */
#ifndef _UNISA_STDIO_H
#define _UNISA_STDIO_H

#ifndef _UNISA_STDDEF_H
#define _UNISA_STDDEF_H
#define NULL 0
/* A shared guard: <sys/types.h> declares the same shorthand types, and two
 * identical typedefs are legal in C11 but were only tolerated in C99 -- so
 * whichever header comes first defines them and the other defers. */
#if defined(_WIN64)
/* LLP64 Windows: long is 4 bytes, so the pointer-sized types are long long. */
#ifndef _UNISA_SIZE_T
#define _UNISA_SIZE_T
typedef unsigned long long size_t;
#endif
typedef long long ptrdiff_t;
#else
#ifndef _UNISA_SIZE_T
#define _UNISA_SIZE_T
typedef unsigned long size_t;
#endif
typedef long ptrdiff_t;
#endif
typedef int wchar_t;
/* offsetof is a compiler intrinsic (__builtin_offsetof reads the struct
 * table), not pointer arithmetic on a null pointer.  It accepts a type name
 * that macro expansion has parenthesised. */
#define offsetof(T, m) __builtin_offsetof(T, m)
#endif


/* <stdarg.h> for the unisa C subset.  A variadic function takes all of its
 * arguments on the tape stack, arg[k] at [FP + 16 + 8k], so a va_list is a
 * pointer to the next slot and the macros are compiler intrinsics. */
#ifndef _UNISA_STDARG_H
#define _UNISA_STDARG_H
typedef char *va_list;
/* A va_list here is a pointer to the next argument slot, so copying one is
 * an assignment.  C99 requires the macro; it requires va_end on the copy
 * too, which for this representation is a no-op the compiler folds. */
#define va_copy(d, s) ((d) = (s))
#define __va_copy(d, s) ((d) = (s))
#endif


/* <errno.h> for the unisa C subset.  `errno` is an ordinary int in the
 * image: the library functions here set it where C says they must, and the
 * headers that wrap system calls turn a failed call's result into errno.
 * The values are the TARGET kernel's (a raw system call returns them), so
 * they differ by OS: Linux (asm-generic, both architectures) and macOS.
 * 0.0.19: the full POSIX set (dsh 2026-10-01: EINTR et al. were missing). */
#ifndef _UNISA_ERRNO_H
#define _UNISA_ERRNO_H
static int errno = 0;
#ifdef __APPLE__
#define EPERM            1
#define ENOENT           2
#define ESRCH            3
#define EINTR            4
#define EIO              5
#define ENXIO            6
#define E2BIG            7
#define ENOEXEC          8
#define EBADF            9
#define ECHILD           10
#define EAGAIN           35
#define ENOMEM           12
#define EACCES           13
#define EFAULT           14
#define ENOTBLK          15
#define EBUSY            16
#define EEXIST           17
#define EXDEV            18
#define ENODEV           19
#define ENOTDIR          20
#define EISDIR           21
#define EINVAL           22
#define ENFILE           23
#define EMFILE           24
#define ENOTTY           25
#define ETXTBSY          26
#define EFBIG            27
#define ENOSPC           28
#define ESPIPE           29
#define EROFS            30
#define EMLINK           31
#define EPIPE            32
#define EDOM             33
#define ERANGE           34
#define EDEADLK          11
#define ENAMETOOLONG     63
#define ENOLCK           77
#define ENOSYS           78
#define ENOTEMPTY        66
#define ELOOP            62
#define ENOMSG           91
#define EIDRM            90
#define ENOSTR           99
#define ENODATA          96
#define ETIME            101
#define ENOSR            98
#define ENOLINK          97
#define EPROTO           100
#define EMULTIHOP        95
#define EBADMSG          94
#define EOVERFLOW        84
#define EILSEQ           92
#define ENOTSOCK         38
#define EDESTADDRREQ     39
#define EMSGSIZE         40
#define EPROTOTYPE       41
#define ENOPROTOOPT      42
#define EPROTONOSUPPORT  43
#define EOPNOTSUPP       102
#define EAFNOSUPPORT     47
#define EADDRINUSE       48
#define EADDRNOTAVAIL    49
#define ENETDOWN         50
#define ENETUNREACH      51
#define ENETRESET        52
#define ECONNABORTED     53
#define ECONNRESET       54
#define ENOBUFS          55
#define EISCONN          56
#define ENOTCONN         57
#define ETIMEDOUT        60
#define ECONNREFUSED     61
#define EHOSTUNREACH     65
#define EALREADY         37
#define EINPROGRESS      36
#define ESTALE           70
#define EDQUOT           69
#define ECANCELED        89
#define EOWNERDEAD       105
#define ENOTRECOVERABLE  104
#define EWOULDBLOCK      EAGAIN
#define ENOTSUP          45
#else
#define EPERM            1
#define ENOENT           2
#define ESRCH            3
#define EINTR            4
#define EIO              5
#define ENXIO            6
#define E2BIG            7
#define ENOEXEC          8
#define EBADF            9
#define ECHILD           10
#define EAGAIN           11
#define ENOMEM           12
#define EACCES           13
#define EFAULT           14
#define ENOTBLK          15
#define EBUSY            16
#define EEXIST           17
#define EXDEV            18
#define ENODEV           19
#define ENOTDIR          20
#define EISDIR           21
#define EINVAL           22
#define ENFILE           23
#define EMFILE           24
#define ENOTTY           25
#define ETXTBSY          26
#define EFBIG            27
#define ENOSPC           28
#define ESPIPE           29
#define EROFS            30
#define EMLINK           31
#define EPIPE            32
#define EDOM             33
#define ERANGE           34
#define EDEADLK          35
#define ENAMETOOLONG     36
#define ENOLCK           37
#define ENOSYS           38
#define ENOTEMPTY        39
#define ELOOP            40
#define ENOMSG           42
#define EIDRM            43
#define ENOSTR           60
#define ENODATA          61
#define ETIME            62
#define ENOSR            63
#define ENOLINK          67
#define EPROTO           71
#define EMULTIHOP        72
#define EBADMSG          74
#define EOVERFLOW        75
#define EILSEQ           84
#define ENOTSOCK         88
#define EDESTADDRREQ     89
#define EMSGSIZE         90
#define EPROTOTYPE       91
#define ENOPROTOOPT      92
#define EPROTONOSUPPORT  93
#define EOPNOTSUPP       95
#define EAFNOSUPPORT     97
#define EADDRINUSE       98
#define EADDRNOTAVAIL    99
#define ENETDOWN         100
#define ENETUNREACH      101
#define ENETRESET        102
#define ECONNABORTED     103
#define ECONNRESET       104
#define ENOBUFS          105
#define EISCONN          106
#define ENOTCONN         107
#define ETIMEDOUT        110
#define ECONNREFUSED     111
#define EHOSTUNREACH     113
#define EALREADY         114
#define EINPROGRESS      115
#define ESTALE           116
#define EDQUOT           122
#define ECANCELED        125
#define EOWNERDEAD       130
#define ENOTRECOVERABLE  131
#define EWOULDBLOCK      EAGAIN
#define ENOTSUP          EOPNOTSUPP
#define EDEADLOCK        EDEADLK
#endif
#endif


/* sys/_word.h -- the carried library's machine word: a pointer, a handle, a byte count or a
 * host-channel slot. It is `long` on the LP64 targets (lnx, osx) and `long long` on LLP64
 * Windows, so it is 8 bytes everywhere and the same C type as before off Windows.
 * __hostcall reads its argument vector as 8-byte slots (src/back_encode.c BK_HOST_*), and the
 * E3 builtins return 8-byte values (exec/parse2 callcontrol sysemit), so bridge arrays,
 * builtin results and pointer casts use __u_word, never plain `long`.
 * These are macros, not typedefs: off Windows they expand to exactly the tokens the headers
 * had before (`long`, `unsigned long`), so the LP64 preprocessed output stays byte-identical. */
#ifndef _UNISA_SYS_WORD_H
#define _UNISA_SYS_WORD_H
#if defined(_WIN64)
#define __u_word long long
#define __u_uword unsigned long long
_Static_assert(sizeof(__u_word) == 8 && sizeof(__u_word) == sizeof(void *), "__u_word is the 8-byte machine word");
#else
#define __u_word long
#define __u_uword unsigned long
#endif
#endif

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
/* C99 7.19.1p2: an object type recording every position in a file.  The host's
   (glibc _G_fpos_t) is an off_t and an mbstate_t: sixteen bytes. */
typedef struct { __u_word __u_pos; int __u_count; unsigned int __u_wch; } fpos_t;
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

static int printf();   /* 0.0.35 H1: static like its definition (C99 6.2.2p7) */

#if !__UNISA_FTRIM_LIBC || __UN__unisa_fd
static int _unisa_fd(FILE *__u_f) {
    __u_word __u_v;
    __u_v = (__u_word)__u_f;
    if (__u_v >= _UNISA_STDIO_BASE) return (int)(__u_v - _UNISA_STDIO_BASE);
    return (int)__u_v;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN__unisa_len
static __u_word _unisa_len(const char *__u_s) {
    __u_word __u_n;
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

/* sys/_exit.h -- 0.0.33 D3': exit, the atexit table and the stdio flush hook, shared by
 * <stdlib.h> and <stdio.h>.  <stdio.h> needs exit in the program so that returning from main
 * goes through it and flushes buffered streams, but not the rest of <stdlib.h> (its environ
 * constructor and every other name). */
#ifndef _UNISA_SYS_EXIT_H
#define _UNISA_SYS_EXIT_H

static void exit();   /* 0.0.35 H1: static like its definition below (C99 6.2.2p7); before the definition, as <stdlib.h> always had it (0.0.33 D3': after it, the redeclaration typed exit int and E3 refused assert's (printf(...), exit(1))) */
static void (*_unisa_atexit[32])(void);
static int _unisa_natexit = 0;
/* the stdio stream table lives here so exit can flush it by a direct call:
   a stored function address is a callback on Windows forward programs */
#define _U_NST 8
static int _u_st_fd[_U_NST];
static int _u_st_eof[_U_NST];
static int _u_st_err[_U_NST];
static int _u_st_ung[_U_NST];              /* a pushed-back byte, or -1 */
/* D3: a 4 KB read buffer per stream.  Bytes are taken from it before the
   descriptor is asked again; fseek/rewind drop it and ftell subtracts what is
   still unread, so the stream position stays the one C99 7.19.9 describes. */
#define _U_BUFSZ 4096
static char _u_st_buf[_U_NST * _U_BUFSZ];
static int _u_st_bpos[_U_NST];
static int _u_st_blen[_U_NST];
#if !__UNISA_FTRIM_LIBC || __UN__u_st_copy
static void _u_st_copy(char *__u_d, const char *__u_s, __u_word __u_n) {
    __u_word __u_j; __u_j = 0;
    while (__u_j < __u_n) { __u_d[__u_j] = __u_s[__u_j]; __u_j = __u_j + 1; }
}
#endif
static int _u_st_wlen[_U_NST];             /* D3': bytes waiting in the buffer to be written */
static int _u_st_ro[_U_NST];               /* opened "r" without "+": a write fails with EBADF [C99 7.19.5.3] */
static int _u_st_n;


/* D3': the write side of the buffer.  A stream's buffer holds either unread
   bytes or unwritten ones: a write after a read gives the unread part back to
   the descriptor (lseek), and a read, seek, tell, fflush, fclose or exit
   writes the waiting bytes first.  The loop in _u_st_raw stops on what the
   __write gate calls an error: negative on POSIX and the VM, 0 on Windows
   (WriteFile's count); an answer larger than asked is not trusted. */
#if !__UNISA_FTRIM_LIBC || __UN__u_st_raw
static __u_word _u_st_raw(int __u_fd, const char *__u_p, __u_word __u_n) {
    __u_word __u_done; __u_word __u_r;
    __u_done = 0;
    while (__u_done < __u_n) {
        __u_r = __write(__u_fd, (char *)__u_p + __u_done, __u_n - __u_done);
        /* POSIX gates return negative errno; a zero/oversized result is
           an I/O failure (also covers the Windows count-only gate). */
        if (__u_r <= 0 || __u_r > __u_n - __u_done) {
            errno = __u_r < 0 ? (int)(0 - __u_r) : EIO;
            return __u_done;
        }
        __u_done = __u_done + __u_r;
    }
    return __u_n;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__u_st_wflush
static int _u_st_wflush(int __u_i) {
    __u_word __u_k;
    if (__u_i < 0 || _u_st_wlen[__u_i] == 0) return 0;
    __u_k = _u_st_wlen[__u_i]; _u_st_wlen[__u_i] = 0;
    if (_u_st_raw(_u_st_fd[__u_i], _u_st_buf + __u_i * _U_BUFSZ, __u_k) != __u_k) { _u_st_err[__u_i] = 1; return 0 - 1; }
    return 0;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__u_st_flushall
static void _u_st_flushall(void) {
    int __u_i; __u_i = 0;
    while (__u_i < _u_st_n) { _u_st_wflush(__u_i); __u_i = __u_i + 1; }
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_exit
static void exit(int __u_code) {
    /* the atexit handlers, last registered first (C99 7.20.4.3p3) */
    while (_unisa_natexit > 0) {
        _unisa_natexit = _unisa_natexit - 1;
        _unisa_atexit[_unisa_natexit]();
    }
    _u_st_flushall();   /* streams last (C99 7.20.4.3p4) */
    __exit(__u_code);
}
#endif
#endif

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
static __u_word _u_st_wput(FILE *__u_f, const char *__u_p, __u_word __u_n) {
    int __u_fd; int __u_i;
    __u_fd = _unisa_fd(__u_f);
    if (__u_n <= 0) return 0;
    if ((__u_word)__u_f >= _UNISA_STDIO_BASE || __u_fd <= 2) return _u_st_raw(__u_fd, __u_p, __u_n);
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
static __u_word fwrite(const void *__u_p, __u_word __u_sz, __u_word __u_n, FILE *__u_f) {
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
static int setvbuf(FILE *__u_f, char *__u_buf, int __u_mode, __u_word __u_size) {
    if (__u_f == NULL) return 0 - 1;
    return 0;
}
#endif
#endif

#if !__UNISA_FTRIM_LIBC || __UN_fread
static __u_word fread(void *__u_p, __u_word __u_sz, __u_word __u_n, FILE *__u_f) {
    __u_word __u_got; __u_word __u_want; __u_word __u_pre; int __u_i;
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
        __u_word __u_k; __u_k = _u_st_blen[__u_i] - _u_st_bpos[__u_i];
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
    /* Windows has no POSIX file-opening call, and CreateFileA wants its own shapes.  They
       are built HERE and not in the encoder, because this is the only place
       that knows whether the program is being compiled for Windows. */
    __u_word __u_access;
    __u_word __u_disp;
    __u_access = 0x80000000;                  /* GENERIC_READ  */
    __u_disp = 3;                             /* OPEN_EXISTING */
    if (__u_mode[0] == 119) { __u_access = 0x40000000; __u_disp = 2; }   /* 'w' CREATE_ALWAYS */
    if (__u_mode[0] == 97)  { __u_access = 0x40000000; __u_disp = 4; }   /* 'a' OPEN_ALWAYS   */
    __u_fd = (int)__open((char *)__u_path, __u_access, __u_disp);   /* a HANDLE: 32 significant bits, sign-extended back by (FILE *)(__u_word) */
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
           [R13-0b #25].  A bare -1 is reported as ENOENT: an fopen that
           failed with no code is almost always a missing file. */
        errno = 0 - __u_fd;
#ifdef _WIN32
        if (errno == 1) errno = ENOENT;
#endif
        return NULL;
    }
    {   int __u_s2; int __u_j2; int __u_ro; __u_ro = __u_mode[0] == 114; __u_j2 = 0;
        while (__u_mode[__u_j2]) { if (__u_mode[__u_j2] == 43) __u_ro = 0; __u_j2 = __u_j2 + 1; }
        __u_s2 = _u_st_slot((FILE *)(__u_word)__u_fd); if (__u_s2 >= 0) _u_st_ro[__u_s2] = __u_ro; }
    return (FILE *)(__u_word)__u_fd;
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
    return _u_vfmt(NULL, 0, stdout, __u_fmt, __u_ap);
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
            __u_word __u_s;
            __u_word __u_j;
            __u_s = _u_digits(__u_d, (__u_uword)__u_v & 2147483647, 10, 0);
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
    FILE *__u_n; __u_word __u_r;
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
    int __u_i; int __u_flush; int __u_errno; __u_word __u_close;
    __u_flush = 0; __u_errno = 0;
    if ((__u_word)__u_f >= _UNISA_STDIO_BASE) return 0;
    /* free the stream's state slot: a later fopen that gets the same
       descriptor must not inherit this stream's eof/err/pushback */
    __u_i = _u_st_slot(__u_f);
    if (__u_i >= 0) {
        __u_flush = _u_st_wflush(__u_i);
        if (__u_flush) __u_errno = errno;
        _u_st_n = _u_st_n - 1;
        _u_st_fd[__u_i] = _u_st_fd[_u_st_n]; _u_st_eof[__u_i] = _u_st_eof[_u_st_n];
        _u_st_err[__u_i] = _u_st_err[_u_st_n]; _u_st_ung[__u_i] = _u_st_ung[_u_st_n];
        _u_st_copy(_u_st_buf + __u_i * _U_BUFSZ, _u_st_buf + _u_st_n * _U_BUFSZ, _U_BUFSZ);
        _u_st_bpos[__u_i] = _u_st_bpos[_u_st_n]; _u_st_blen[__u_i] = _u_st_blen[_u_st_n];
        _u_st_wlen[__u_i] = _u_st_wlen[_u_st_n]; _u_st_ro[__u_i] = _u_st_ro[_u_st_n];
    }
    /* Release the descriptor even when the buffered output failed. Preserve
       that first failure after the descriptor gate and state-slot cleanup. */
    __u_close = __close(_unisa_fd(__u_f));
    if (__u_flush) { errno = __u_errno; return EOF; }
#ifdef _WIN32
    /* CloseHandle returns BOOL, not a POSIX result or negative errno. */
    if (__u_close == 0) { errno = EIO; return EOF; }
#else
    if (__u_close < 0) { errno = (int)(0 - __u_close); return EOF; }
#endif
    return 0;
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
    __u_word __u_r; int __u_i;
    __u_i = _u_st_slot(__u_f);
    if (__u_i >= 0) _u_st_wflush(__u_i);
    __u_r = __lseek(_unisa_fd(__u_f), 0, SEEK_CUR);
    if (__u_r >= 0 && __u_i >= 0) __u_r = __u_r - (_u_st_blen[__u_i] - _u_st_bpos[__u_i]) - (_u_st_ung[__u_i] >= 0);
#if defined(_WIN64)
    /* the API answers a 32-bit long here; a position it cannot hold is a failure (-1), as the
       old 32-bit __u_r took SetFilePointer's 0xFFFFFFFF failure and offsets >= 2^31 */
    if (__u_r != (long)__u_r) return -1;
    return (long)__u_r;
#else
    return __u_r;
#endif
}
#endif
/* POSIX spellings lua's LUA_USE_POSIX build asks for (0.0.34 L1b): off_t is
   long here, so fseeko/ftello are fseek/ftell; there is one thread, so the
   stream locks have nothing to do and getc_unlocked is getc. */
#if !__UNISA_FTRIM_LIBC || __UN_fseeko
static int fseeko(FILE *__u_f, __u_word __u_off, int __u_whence) {
#if defined(_WIN64)
    /* off_t is 8 bytes, fseek's long 4: an offset long cannot hold is refused, not wrapped */
    if (__u_off != (long)__u_off) return -1;
    return fseek(__u_f, (long)__u_off, __u_whence);
#else
    return fseek(__u_f, __u_off, __u_whence);
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_ftello
static __u_word ftello(FILE *__u_f) { return ftell(__u_f); }
#endif
/* fgetpos/fsetpos (C99 7.19.9.1, 7.19.9.3) over ftell/fseek: the fpos_t holds
   the byte offset in its first field; nonzero on failure. */
#if !__UNISA_FTRIM_LIBC || __UN_fgetpos
static int fgetpos(FILE *__u_f, fpos_t *__u_p) { __u_word __u_r; __u_r = ftell(__u_f); if (__u_r < 0) return -1; __u_p->__u_pos = __u_r; __u_p->__u_count = 0; __u_p->__u_wch = 0; return 0; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_fsetpos
static int fsetpos(FILE *__u_f, const fpos_t *__u_p) {
#if defined(_WIN64)
    /* __u_pos is 8 bytes, the seek offset long is 4: a stored position came from the 32-bit tell */
    if (__u_p->__u_pos != (long)__u_p->__u_pos) return -1;
    return fseek(__u_f, (long)__u_p->__u_pos, SEEK_SET);
#else
    return fseek(__u_f, __u_p->__u_pos, SEEK_SET);
#endif
}
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
    if ((__u_word)__u_f >= _UNISA_STDIO_BASE || _unisa_fd(__u_f) <= 2) return 0;
    return _u_st_wflush(_u_st_slot(__u_f));
}
#endif

/* ---- a runtime formatter ------------------------------------------------
 * `printf` is desugared by the walker against its static format string, which
 * is the fast path and the one that does not need varargs.  Everything that
 * takes a format at RUN time is written here, in the subset itself. */

#if !__UNISA_FTRIM_LIBC || __UN__u_put
static void _u_put(char *__u_out, __u_word __u_cap, __u_word *__u_n, FILE *__u_f, int __u_c) {
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
static __u_word _u_digits(char *__u_buf, __u_uword __u_v, int __u_base, int __u_upper) {
    __u_word __u_i;
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
static int _u_dexp(__u_uword __u_bits, char *__u_dig, int *__u_x) {
    __u_uword __u_lim[_U_NL];
    __u_uword __u_m;
    __u_uword __u_t;
    __u_uword __u_carry;
    __u_uword __u_mul;
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
static int _u_ffmt(char *__u_out, __u_uword __u_bits, int __u_c, int __u_prec, int __u_alt) {
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
        __u_uword __u_m; int __u_lead; int __u_hd; int __u_d;
        __u_m = __u_bits & 4503599627370495; __u_e = (int)((__u_bits >> 52) & 2047);
        if (__u_e == 0) {                         /* subnormal: normalised, as BSD libc prints it */
            __u_lead = 0; __u_e = 0;
            if (__u_m) { __u_lead = 1; __u_e = 0 - 1022; while ((__u_m & 4503599627370496) == 0) { __u_m = __u_m << 1; __u_e = __u_e - 1; } __u_m = __u_m & 4503599627370495; }
        }
        else { __u_lead = 1; __u_e = __u_e - 1023; }
        __u_nd = 13;
        if (__u_prec >= 0 && __u_prec < 13) {
            __u_uword __u_rem; __u_uword __u_half; int __u_sh;
            __u_sh = (13 - __u_prec) * 4;
            __u_rem = __u_m & (((__u_uword)1 << __u_sh) - 1); __u_half = (__u_uword)1 << (__u_sh - 1);
            __u_m = __u_m >> __u_sh;
            if (__u_rem > __u_half || (__u_rem == __u_half && (__u_m & 1))) __u_m = __u_m + 1;
            if (__u_m >> (__u_prec * 4)) { __u_m = __u_m & (((__u_uword)1 << (__u_prec * 4)) - 1); __u_lead = __u_lead + 1; }
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

/* _u_vfmt writes to the buffer OUT (capacity CAP, -1 for unbounded) or, when OUT is NULL, to the stream F:
   exactly one of them.  A caller with neither -- vsnprintf(NULL, 0, ...) measuring, vprintf before 0.0.43 --
   sent its bytes to FILE * 0, which is descriptor 0 (stdin): a blocked write when nobody reads it, the hang the
   .cx ports met, and a lost vprintf line.  The measuring callers pass a one-byte buffer of capacity 0 instead. */
#if !__UNISA_FTRIM_LIBC || __UN__u_vfmt
static int _u_vfmt(char *__u_out, __u_word __u_cap, FILE *__u_f, const char *__u_fmt, va_list __u_ap) {
    __u_word __u_n;
    __u_word __u_i;
    __u_word __u_k;
    __u_word __u_len;
    __u_word __u_start;
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
    __u_word __u_j;
    __u_word __u_sv;
    __u_uword __u_uv;
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
#if defined(_WIN64)
            /* LLP64: a lone l is the 4-byte long and narrows like no modifier; ll, z, j, t, L
               are 64-bit.  __u_lng counts here and is folded to 0/1 after the loop. */
            if (__u_fmt[__u_i] == 104) __u_hh = __u_hh + 1;
            else { if (__u_fmt[__u_i] == 108) __u_lng = __u_lng + 1; else __u_lng = 2; }
#else
            if (__u_fmt[__u_i] == 104) __u_hh = __u_hh + 1;   /* h, hh: narrow */
            else __u_lng = 1;                                /* l, z, j, t, L are 64-bit */
#endif
            __u_i = __u_i + 1;
        }
#if defined(_WIN64)
        __u_lng = __u_lng >= 2;
#endif
        __u_c = __u_fmt[__u_i];
        __u_i = __u_i + 1;
        if (__u_c == 37) { _u_put(__u_out, __u_cap, &__u_n, __u_f, 37); continue; }
        __u_sp = NULL; __u_sign = 0; __u_base = 10; __u_upper = 0; __u_neg = 0;
        if (__u_c == 102 | __u_c == 70 | __u_c == 101 | __u_c == 69 | __u_c == 103 | __u_c == 71 | __u_c == 97 | __u_c == 65) {
            /* %f %e %g: a double -- a float argument was promoted to one */
            double __u_dv;
            __u_uword __u_bits;
            __u_dv = va_arg(__u_ap, double);
            __u_bits = *(__u_uword *)&__u_dv;
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
                    __u_sv = va_arg(__u_ap, __u_word);
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
                    __u_uv = va_arg(__u_ap, __u_uword);
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
static int vsnprintf(char *__u_b, __u_word __u_cap, const char *__u_fmt, va_list __u_ap) {
    char __u_z;
    if (__u_b == NULL) { __u_b = &__u_z; __u_cap = 0; }   /* measure only (C99 7.19.6.12p2) */
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
static int snprintf(char *__u_b, __u_word __u_cap, const char *__u_fmt, ...) {
    va_list __u_ap; int __u_r; char __u_z;
    if (__u_b == NULL) { __u_b = &__u_z; __u_cap = 0; }   /* measure only (C99 7.19.6.5p2) */
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
    __u_word __u_i; __u_word __u_j; int __u_stored; int __u_c; int __u_width; int __u_skip; int __u_lng; int __u_base;
    int __u_neg; int __u_any; long __u_v; __u_uword __u_uv; double __u_d; double __u_scale; int exp; int __u_eneg;
    char *__u_sp; int *__u_ip; long *__u_lp; short *__u_hp; double *__u_dp; float *__u_fp; __u_word __u_n;
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
#if defined(_WIN64)
        /* LLP64: l stores the 4-byte long (__u_lng 1); ll, z, j, t, L store 8 bytes (__u_lng 2) */
        if (__u_fmt[__u_j] == 108) { __u_lng = 1; __u_j = __u_j + 1; if (__u_fmt[__u_j] == 108) { __u_lng = 2; __u_j = __u_j + 1; } }
        if (__u_fmt[__u_j] == 122 || __u_fmt[__u_j] == 106 || __u_fmt[__u_j] == 116) { __u_lng = 2; __u_j = __u_j + 1; }
        if (__u_fmt[__u_j] == 76) { __u_lng = 2; __u_j = __u_j + 1; }
#else
        if (__u_fmt[__u_j] == 108) { __u_lng = 1; __u_j = __u_j + 1; if (__u_fmt[__u_j] == 108) __u_j = __u_j + 1; }
        if (__u_fmt[__u_j] == 122 || __u_fmt[__u_j] == 106 || __u_fmt[__u_j] == 116) { __u_lng = 1; __u_j = __u_j + 1; }
        if (__u_fmt[__u_j] == 76) { __u_lng = 1; __u_j = __u_j + 1; }
#endif
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
#if defined(_WIN64)
                if (__u_lng == 2) { __u_word *__u_qp; __u_qp = va_arg(__u_ap, __u_word *); *__u_qp = (__u_word)__u_uv; }
                else
#endif
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
#if defined(_WIN64)
                if (__u_lng >= 1) { __u_dp = va_arg(__u_ap, double *); *__u_dp = __u_d; }   /* l and L (ll) are double */
#else
                if (__u_lng == 1) { __u_dp = va_arg(__u_ap, double *); *__u_dp = __u_d; }
#endif
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

/* <stdlib.h> for the unisa C subset.  The allocator takes its memory from the
 * OS through the __mmap/__munmap gates and reuses what is freed; nothing here
 * needs a linker. */
#ifndef _UNISA_STDLIB_H
#define _UNISA_STDLIB_H



/* <limits.h> for the unisa C subset.
 *
 * These are the LIMITS OF THE TYPES, which is what a program asks for.  They
 * are not a description of how this compiler evaluates arithmetic: [G-2] says
 * `int` expressions are evaluated at 64 bits, so `INT_MAX + 1` does not wrap
 * here the way it does elsewhere.  That is a documented deviation of the
 * subset, and it does not change what INT_MAX is. */
#ifndef _UNISA_LIMITS_H
#define _UNISA_LIMITS_H

#define CHAR_BIT    8
/* PATH_MAX (POSIX <limits.h>, 0.0.37 C3): the host's own value */
#ifdef _WIN32
#define PATH_MAX    260
#elif defined(__APPLE__)
#define PATH_MAX    1024
#else
#define PATH_MAX    4096
#endif
#define SCHAR_MIN   (-128)
#define SCHAR_MAX   127
#define UCHAR_MAX   255
#define CHAR_MIN    SCHAR_MIN       /* plain char is signed here */
#define CHAR_MAX    SCHAR_MAX
#define MB_LEN_MAX  1

#define SHRT_MIN    (-32768)
#define SHRT_MAX    32767
#define USHRT_MAX   65535

#define INT_MIN     (-2147483647 - 1)
#define INT_MAX     2147483647
/* U: C99 5.2.4.2.1 gives UINT_MAX the type unsigned int.  Without the
   suffix the literal does not fit int and becomes long, so `UINT_MAX + 1`
   was 4294967296 where gcc wraps to 0 (fb12-08b). */
#define UINT_MAX    4294967295U

#if defined(_WIN64)
/* LLP64 Windows: long is 4 bytes, long long is the 8-byte type. */
#define LONG_MIN    (-2147483647L - 1)
#define LONG_MAX    2147483647L
#define ULONG_MAX   4294967295UL

#define LLONG_MIN   (-9223372036854775807LL - 1)
#define LLONG_MAX   9223372036854775807LL
#define ULLONG_MAX  18446744073709551615ULL
#else
#define LONG_MIN    (-9223372036854775807 - 1)
#define LONG_MAX    9223372036854775807
/* The UL suffix matters: without it this literal does not fit any signed
   type, and the reference refuses to fold an expression containing it
   (fb12-08, fb12-08b).  gcc's own <limits.h> writes it the same way.  The
   value is unchanged -- measured equal to gcc before and after. */
#define ULONG_MAX   18446744073709551615UL

#define LLONG_MIN   LONG_MIN
#define LLONG_MAX   LONG_MAX
#define ULLONG_MAX  ULONG_MAX
#endif

#endif


#define NULL 0
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define RAND_MAX 32767

/* The allocator.  Memory comes from the OS in 1 MB chunks (mmap, or
 * VirtualAlloc on Windows); a block is a 16-byte header -- its size, then its
 * class -- and the payload, so every pointer handed out is 16-aligned.
 *   small: header + request <= 64 KB is rounded up to a power of two, 32 ..
 *          65536 (classes 0 .. 11).  A freed block goes on its class's list
 *          (the link lives in the payload) and the next request of that class
 *          takes it back.  Nothing is split or coalesced, so nothing can be
 *          corrupted by it: a block is always exactly its class's size.
 *   large: anything bigger is mapped on its own, rounded to 64 KB, class -1,
 *          and `free` returns it to the OS. */
#ifdef _WIN32
#define _UNISA_MAP(__u_n) __mmap(0, (__u_n), 0x3000, 4, 0, 0)
#else
#ifdef __linux__
#define _UNISA_MAP(__u_n) __mmap(0, (__u_n), 3, 0x22, 0 - 1, 0)
#else
#define _UNISA_MAP(__u_n) __mmap(0, (__u_n), 3, 0x1002, 0 - 1, 0)
#endif
#endif
#define _UNISA_CHUNK 1048576
static char *_unisa_freel[12];
static char *_unisa_cur = 0;
static __u_word _unisa_left = 0;

#if !__UNISA_FTRIM_LIBC || __UN__unisa_map
static char *_unisa_map(__u_word __u_n) {
    __u_word __u_r;
    __u_r = (__u_word)_UNISA_MAP(__u_n);
    if (__u_r == 0 - 1 || __u_r == 0) return NULL;
    return (char *)__u_r;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_malloc
static void *malloc(__u_word __u_n) {
    __u_word __u_sz; int __u_k; char *__u_p;
    if (__u_n < 0 || __u_n > 0x100000000000) return NULL;
    __u_sz = __u_n + 16;
    if (__u_sz > 65536) {
        __u_sz = (__u_sz + 65535) & ~65535;
        __u_p = _unisa_map(__u_sz);
        if (__u_p == NULL) return NULL;
        *(__u_word *)__u_p = __u_sz;
        *(__u_word *)(__u_p + 8) = 0 - 1;
        return __u_p + 16;
    }
    __u_k = 0;
    while ((32 << __u_k) < __u_sz) __u_k = __u_k + 1;
    __u_sz = 32 << __u_k;
    __u_p = _unisa_freel[__u_k];
    if (__u_p != NULL) {
        _unisa_freel[__u_k] = *(char **)(__u_p + 16);
        return __u_p + 16;
    }
    if (_unisa_left < __u_sz) {
        __u_p = _unisa_map(_UNISA_CHUNK);
        if (__u_p == NULL) return NULL;
        _unisa_cur = __u_p;
        _unisa_left = _UNISA_CHUNK;
    }
    __u_p = _unisa_cur;
    _unisa_cur = _unisa_cur + __u_sz;
    _unisa_left = _unisa_left - __u_sz;
    *(__u_word *)__u_p = __u_sz;
    *(__u_word *)(__u_p + 8) = __u_k;
    return __u_p + 16;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_free
static void free(void *__u_q) {
    char *__u_p; __u_word __u_k;
    if (__u_q == NULL) return;
    __u_p = (char *)__u_q - 16;
    __u_k = *(__u_word *)(__u_p + 8);
    if (__u_k < 0) { __munmap(__u_p, *(__u_word *)__u_p); return; }
    *(char **)(__u_p + 16) = _unisa_freel[__u_k];
    _unisa_freel[__u_k] = __u_p;
}
#endif

/* `exit` is the one libc function that cannot be written in C: it must not
 * return.  `__exit` is the tape's gate to the OS, so this is a one-line
 * wrapper around it -- and `abort` is the same call with the status a shell
 * reports for SIGABRT. */

/* sys/_exit.h -- 0.0.33 D3': exit, the atexit table and the stdio flush hook, shared by
 * <stdlib.h> and <stdio.h>.  <stdio.h> needs exit in the program so that returning from main
 * goes through it and flushes buffered streams, but not the rest of <stdlib.h> (its environ
 * constructor and every other name). */
#ifndef _UNISA_SYS_EXIT_H
#define _UNISA_SYS_EXIT_H

static void exit();   /* 0.0.35 H1: static like its definition below (C99 6.2.2p7); before the definition, as <stdlib.h> always had it (0.0.33 D3': after it, the redeclaration typed exit int and E3 refused assert's (printf(...), exit(1))) */
static void (*_unisa_atexit[32])(void);
static int _unisa_natexit = 0;
/* the stdio stream table lives here so exit can flush it by a direct call:
   a stored function address is a callback on Windows forward programs */
#define _U_NST 8
static int _u_st_fd[_U_NST];
static int _u_st_eof[_U_NST];
static int _u_st_err[_U_NST];
static int _u_st_ung[_U_NST];              /* a pushed-back byte, or -1 */
/* D3: a 4 KB read buffer per stream.  Bytes are taken from it before the
   descriptor is asked again; fseek/rewind drop it and ftell subtracts what is
   still unread, so the stream position stays the one C99 7.19.9 describes. */
#define _U_BUFSZ 4096
static char _u_st_buf[_U_NST * _U_BUFSZ];
static int _u_st_bpos[_U_NST];
static int _u_st_blen[_U_NST];
#if !__UNISA_FTRIM_LIBC || __UN__u_st_copy
static void _u_st_copy(char *__u_d, const char *__u_s, __u_word __u_n) {
    __u_word __u_j; __u_j = 0;
    while (__u_j < __u_n) { __u_d[__u_j] = __u_s[__u_j]; __u_j = __u_j + 1; }
}
#endif
static int _u_st_wlen[_U_NST];             /* D3': bytes waiting in the buffer to be written */
static int _u_st_ro[_U_NST];               /* opened "r" without "+": a write fails with EBADF [C99 7.19.5.3] */
static int _u_st_n;


/* D3': the write side of the buffer.  A stream's buffer holds either unread
   bytes or unwritten ones: a write after a read gives the unread part back to
   the descriptor (lseek), and a read, seek, tell, fflush, fclose or exit
   writes the waiting bytes first.  The loop in _u_st_raw stops on what the
   __write gate calls an error: negative on POSIX and the VM, 0 on Windows
   (WriteFile's count); an answer larger than asked is not trusted. */
#if !__UNISA_FTRIM_LIBC || __UN__u_st_raw
static __u_word _u_st_raw(int __u_fd, const char *__u_p, __u_word __u_n) {
    __u_word __u_done; __u_word __u_r;
    __u_done = 0;
    while (__u_done < __u_n) {
        __u_r = __write(__u_fd, (char *)__u_p + __u_done, __u_n - __u_done);
        /* POSIX gates return negative errno; a zero/oversized result is
           an I/O failure (also covers the Windows count-only gate). */
        if (__u_r <= 0 || __u_r > __u_n - __u_done) {
            errno = __u_r < 0 ? (int)(0 - __u_r) : EIO;
            return __u_done;
        }
        __u_done = __u_done + __u_r;
    }
    return __u_n;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__u_st_wflush
static int _u_st_wflush(int __u_i) {
    __u_word __u_k;
    if (__u_i < 0 || _u_st_wlen[__u_i] == 0) return 0;
    __u_k = _u_st_wlen[__u_i]; _u_st_wlen[__u_i] = 0;
    if (_u_st_raw(_u_st_fd[__u_i], _u_st_buf + __u_i * _U_BUFSZ, __u_k) != __u_k) { _u_st_err[__u_i] = 1; return 0 - 1; }
    return 0;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__u_st_flushall
static void _u_st_flushall(void) {
    int __u_i; __u_i = 0;
    while (__u_i < _u_st_n) { _u_st_wflush(__u_i); __u_i = __u_i + 1; }
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_exit
static void exit(int __u_code) {
    /* the atexit handlers, last registered first (C99 7.20.4.3p3) */
    while (_unisa_natexit > 0) {
        _unisa_natexit = _unisa_natexit - 1;
        _unisa_atexit[_unisa_natexit]();
    }
    _u_st_flushall();   /* streams last (C99 7.20.4.3p4) */
    __exit(__u_code);
}
#endif
#endif

#if !__UNISA_FTRIM_LIBC || __UN_abort
static void abort(void) { __exit(134); }
#endif

#if !__UNISA_FTRIM_LIBC || __UN_calloc
static void *calloc(__u_word __u_n, __u_word __u_sz) {
    __u_word *__u_p; __u_word __u_total; __u_word __u_i;
    if (__u_n < 0 || __u_sz < 0) return NULL;
    if (__u_sz != 0 && __u_n > 0x100000000000 / __u_sz) return NULL;
    __u_total = __u_n * __u_sz;
    __u_p = (__u_word *)malloc(__u_total);
    if (__u_p == NULL) return NULL;
    /* a payload is a multiple of 16 bytes, so whole longs stay inside it */
    __u_total = (__u_total + 7) / 8;
    __u_i = 0;
    while (__u_i < __u_total) { __u_p[__u_i] = 0; __u_i = __u_i + 1; }
    return __u_p;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_realloc
static void *realloc(void *__u_old, __u_word __u_n) {
    __u_word *__u_p; __u_word *__u_o; __u_word __u_i; __u_word __u_cap;
    if (__u_old == NULL) return malloc(__u_n);
    if (__u_n < 0) return NULL;
    __u_cap = *(__u_word *)((char *)__u_old - 16) - 16;
    if (__u_n <= __u_cap && (__u_cap <= 65536 || __u_n > __u_cap / 2)) return __u_old;
    __u_p = (__u_word *)malloc(__u_n);
    if (__u_p == NULL) return NULL;
    if (__u_n < __u_cap) __u_cap = __u_n;
    __u_cap = (__u_cap + 7) / 8;
    __u_o = (__u_word *)__u_old;
    __u_i = 0;
    while (__u_i < __u_cap) { __u_p[__u_i] = __u_o[__u_i]; __u_i = __u_i + 1; }
    free(__u_old);
    return __u_p;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_abs
static int abs(int __u_v) { if (__u_v < 0) return 0 - __u_v; return __u_v; }
#endif

#if !__UNISA_FTRIM_LIBC || __UN_atoi
static int atoi(const char *__u_s) {
    int __u_v; int __u_sign; __u_word __u_i;
    __u_v = 0; __u_sign = 1; __u_i = 0;
    while (__u_s[__u_i] == 32) __u_i = __u_i + 1;
    if (__u_s[__u_i] == 45) { __u_sign = 0 - 1; __u_i = __u_i + 1; }
    else { if (__u_s[__u_i] == 43) __u_i = __u_i + 1; }
    while (__u_s[__u_i] >= 48) { if (__u_s[__u_i] > 57) break; __u_v = __u_v * 10 + (__u_s[__u_i] - 48); __u_i = __u_i + 1; }
    return __u_v * __u_sign;
}
#endif

/* strtol/strtoul/strtod: the conversions C89 and C99 put here.  `end` is
 * written when it is not null, which is how callers tell "no digits" from
 * "the value happened to be zero". */
/* Accumulate two 32-bit limbs in nonnegative longs.  Their products by
 * base (<=36) fit signed long, including at ULONG_MAX; no signed wrap or
 * unsigned division is needed.  Overflow still consumes every valid digit. */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_strdigit
static int _unisa_strdigit(int __u_c) {
    if (__u_c >= 48 && __u_c <= 57) return __u_c - 48;
    if (__u_c >= 97 && __u_c <= 122) return __u_c - 97 + 10;
    if (__u_c >= 65 && __u_c <= 90) return __u_c - 65 + 10;
    return 0 - 1;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__unisa_strmag
static int _unisa_strmag(const char *__u_s, char **__u_end, int __u_base,
                       __u_word *__u_hi, __u_word *__u_lo, int *__u_neg) {
    __u_word __u_i; __u_word __u_start; __u_word __u_l; __u_word __u_h; int __u_d; int __u_over; int __u_auto;
    __u_auto = __u_base == 0; __u_i = 0; *__u_hi = 0; *__u_lo = 0; *__u_neg = 0; __u_over = 0;
    if (__u_end) *__u_end = (char *)__u_s;
    if (__u_base != 0 && (__u_base < 2 || __u_base > 36)) return 0;
    while (__u_s[__u_i] == 32 || (__u_s[__u_i] >= 9 && __u_s[__u_i] <= 13)) __u_i = __u_i + 1;
    if (__u_s[__u_i] == 45) { *__u_neg = 1; __u_i = __u_i + 1; }
    else { if (__u_s[__u_i] == 43) __u_i = __u_i + 1; }
    if (__u_base == 0) {
        __u_base = 10;
        if (__u_s[__u_i] == 48) __u_base = 8;
    }
    if (__u_base == 8 || __u_base == 16) {
        if (__u_s[__u_i] == 48 && (__u_s[__u_i+1] == 120 || __u_s[__u_i+1] == 88)) {
            __u_d = _unisa_strdigit(__u_s[__u_i+2]);
            if (__u_d >= 0 && __u_d < 16) {
                /* Only base 0 or 16 may accept a hex prefix. */
                if (__u_base == 16 || __u_auto) { __u_base = 16; __u_i = __u_i + 2; }
            }
        }
    }
    __u_start = __u_i;
    while (__u_s[__u_i]) {
        __u_d = _unisa_strdigit(__u_s[__u_i]);
        if (__u_d < 0 || __u_d >= __u_base) break;
        if (!__u_over) {
            __u_l = *__u_lo * __u_base + __u_d;
            __u_h = *__u_hi * __u_base + __u_l / 4294967296;
            if (__u_h > 4294967295) __u_over = 1;
            else { *__u_hi = __u_h; *__u_lo = __u_l % 4294967296; }
        }
        __u_i = __u_i + 1;
    }
    if (__u_end && __u_i != __u_start) *__u_end = (char *)(__u_s + __u_i);
    return __u_over;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_strtol
static long strtol(const char *__u_s, char **__u_end, int __u_base) {
#if defined(_WIN64)
    /* LLP64: long is 32 bits, so the range is [LONG_MIN, LONG_MAX], not the 64-bit one */
    __u_word __u_hi; __u_word __u_lo; __u_uword __u_m; int __u_neg; int __u_over;
    __u_over = _unisa_strmag(__u_s, __u_end, __u_base, &__u_hi, &__u_lo, &__u_neg);
    __u_m = ((__u_uword)__u_hi << 32) | (__u_uword)__u_lo;
    if (__u_over || (!__u_neg && __u_m > (__u_uword)LONG_MAX) ||
        (__u_neg && __u_m > (__u_uword)LONG_MAX + 1)) {
        errno = ERANGE;
        if (__u_neg) return LONG_MIN;
        return LONG_MAX;
    }
    if (__u_neg) return (long)(0 - (__u_word)__u_m);
    return (long)__u_m;
#else
    long __u_hi; long __u_lo; long __u_v; int __u_neg; int __u_over;
    __u_over = _unisa_strmag(__u_s, __u_end, __u_base, &__u_hi, &__u_lo, &__u_neg);
    if (__u_over || __u_hi > 2147483648 ||
        (__u_hi == 2147483648 && (__u_lo != 0 || !__u_neg))) {
        errno = ERANGE;
        if (__u_neg) return LONG_MIN;
        return LONG_MAX;
    }
    if (__u_hi == 2147483648) return LONG_MIN;
    __u_v = __u_hi * 4294967296 + __u_lo;
    if (__u_neg) return 0 - __u_v;
    return __u_v;
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_strtoul
static unsigned long strtoul(const char *__u_s, char **__u_end, int __u_base) {
#if defined(_WIN64)
    /* LLP64: unsigned long is 32 bits; a magnitude past ULONG_MAX is ERANGE, then '-' negates */
    __u_word __u_hi; __u_word __u_lo; unsigned long __u_v; int __u_neg; int __u_over;
    __u_over = _unisa_strmag(__u_s, __u_end, __u_base, &__u_hi, &__u_lo, &__u_neg);
    if (__u_over || __u_hi != 0) { errno = ERANGE; return (unsigned long)0 - 1; }
    __u_v = (unsigned long)__u_lo;
    if (__u_neg) return (unsigned long)0 - __u_v;
    return __u_v;
#else
    long __u_hi; long __u_lo; unsigned long __u_v; int __u_neg; int __u_over;
    __u_over = _unisa_strmag(__u_s, __u_end, __u_base, &__u_hi, &__u_lo, &__u_neg);
    if (__u_over) { errno = ERANGE; return (unsigned long)0 - 1; }
    __u_v = ((unsigned long)__u_hi << 32) | (unsigned long)__u_lo;
    if (__u_neg) return (unsigned long)0 - __u_v;
    return __u_v;
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_atol
static long atol(const char *__u_s) { return strtol(__u_s, 0, 10); }
#endif

/* This compiler uses 64-bit long and long long on all six targets.  Keep the
 * conversion, overflow clamp, ERANGE and end-pointer rules in one routine. */
#if !__UNISA_FTRIM_LIBC || __UN_strtoll
static long long strtoll(const char *__u_s, char **__u_end, int __u_base) {
#if defined(_WIN64)
    /* LLP64: strtol is 32-bit here, so the 64-bit range is converted and clamped directly */
    __u_word __u_hi; __u_word __u_lo; __u_word __u_v; int __u_neg; int __u_over;
    __u_over = _unisa_strmag(__u_s, __u_end, __u_base, &__u_hi, &__u_lo, &__u_neg);
    if (__u_over || __u_hi > 2147483648 ||
        (__u_hi == 2147483648 && (__u_lo != 0 || !__u_neg))) {
        errno = ERANGE;
        if (__u_neg) return LLONG_MIN;
        return LLONG_MAX;
    }
    if (__u_hi == 2147483648) return LLONG_MIN;
    __u_v = __u_hi * 4294967296 + __u_lo;
    if (__u_neg) return 0 - __u_v;
    return __u_v;
#else
    return strtol(__u_s, __u_end, __u_base);
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_strtoull
static unsigned long long strtoull(const char *__u_s, char **__u_end, int __u_base) {
#if defined(_WIN64)
    /* LLP64: strtoul is 32-bit here, so the 64-bit value is assembled directly */
    __u_word __u_hi; __u_word __u_lo; __u_uword __u_v; int __u_neg; int __u_over;
    __u_over = _unisa_strmag(__u_s, __u_end, __u_base, &__u_hi, &__u_lo, &__u_neg);
    if (__u_over) { errno = ERANGE; return (__u_uword)0 - 1; }
    __u_v = ((__u_uword)__u_hi << 32) | (__u_uword)__u_lo;
    if (__u_neg) return (__u_uword)0 - __u_v;
    return __u_v;
#else
    return strtoul(__u_s, __u_end, __u_base);
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_atoll
static long long atoll(const char *__u_s) { return strtoll(__u_s, 0, 10); }
#endif

/* The fractional part is accumulated as an integer and scaled once, so a
 * long run of digits does not lose the low ones to repeated division. */
/* mkstemp (POSIX, 0.0.34 L1b): replace the trailing XXXXXX and create the file
   exclusively, retrying on EEXIST; the letters come from a counter mixed with the
   address of a local, so two processes running the same binary still differ. */
/* No <fcntl.h> here: an untrimmed tape (the Python front end, --fold) would carry
   every fcntl.h body, and those name host calls Windows lowering refuses.  The
   exclusive-create flags are spelled out instead (O_RDWR|O_CREAT|O_EXCL per OS). */
#ifdef _WIN32

/* sys/_win.h -- 0.0.21 Windows POSIX layer, batch 1: the shared helpers that
 * let <unistd.h>, <fcntl.h>, <sys/stat.h> and <time.h> wrap POSIX names thinly
 * over kernel32 (no home-grown libc).  The host channel is the program's IAT:
 * __hostaddr0..3 = LoadLibraryA GetProcAddress FreeLibrary GetLastError, and
 * __hostcall(fn, __u_word[10]) makes the Win64/AAPCS64 call.  kernel32's handle is
 * looked up once; each caller caches its own procedure in a static.
 * On Windows an fd IS the HANDLE value (as the bundled stdio already has it).
 * Windows is LLP64 (long is 4 bytes): handles, procedures, pointers and the host
 * channel's 8-byte slots are __u_word (<sys/_word.h>), never long. */
#ifndef _UNISA_SYS_WIN_H
#define _UNISA_SYS_WIN_H
#if defined(_WIN32) && !defined(_UNISA_NO_HOSTCALL)   /* the compiler builds itself without the host channel until the product lowers it on Windows (0.0.22) */


#if !__UNISA_FTRIM_LIBC || __UN__ux_sym
static __u_word _ux_sym(const char *__u_name) {          /* GetProcAddress(kernel32, name) */
    static __u_word __u_k32;
    __u_word __u_a[10] = {0};
    __u_a[1] = 0; __u_a[2] = 0; __u_a[3] = 0; __u_a[4] = 0; __u_a[5] = 0;
    if (!__u_k32) { __u_a[0] = (__u_word)"kernel32.dll"; __u_k32 = __hostcall(__hostaddr0(), __u_a); }
    __u_a[0] = __u_k32; __u_a[1] = (__u_word)__u_name;
    return __hostcall(__hostaddr1(), __u_a);
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__ux_call
static __u_word _ux_call(__u_word __u_fn, __u_word __u_x0, __u_word __u_x1, __u_word __u_x2, __u_word __u_x3) {
    __u_word __u_a[10] = {0};
    __u_a[0] = __u_x0; __u_a[1] = __u_x1; __u_a[2] = __u_x2; __u_a[3] = __u_x3; __u_a[4] = 0; __u_a[5] = 0;
    return __hostcall(__u_fn, __u_a);
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__ux_errno
static int _ux_errno(void) {                         /* GetLastError -> errno */
    __u_word __u_a[10] = {0}; __u_word __u_e;
    __u_a[0] = 0; __u_a[1] = 0; __u_a[2] = 0; __u_a[3] = 0; __u_a[4] = 0; __u_a[5] = 0;
    __u_e = __hostcall(__hostaddr3(), __u_a) & 0xFFFFFFFFLL;
    if (__u_e == 2 || __u_e == 3) return ENOENT;
    if (__u_e == 5) return EACCES;
    if (__u_e == 6) return EBADF;
    if (__u_e == 80 || __u_e == 183) return EEXIST;
    if (__u_e == 145) return ENOTEMPTY;
    if (__u_e == 32) return EBUSY;
    if (__u_e == 87) return EINVAL;
    return EIO;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__ux_fail
static int _ux_fail(void) { errno = _ux_errno(); return -1; }
#endif
/* The names above are kept only when a baked library key lists them. A disk
   body that started calling them later is compiled, and the callee is then
   missing. These macros expand in the caller and use the host channel
   directly, same mapping as _ux_sym / _ux_call / _ux_errno. */
#define _ux_load(dst, name) do { \
    static __u_word __uxk; __u_word __uxa[10]; int __uxi; \
    __uxi = 0; while (__uxi < 10) { __uxa[__uxi] = 0; __uxi = __uxi + 1; } \
    if (!__uxk) { __uxa[0] = (__u_word)"kernel32.dll"; __uxk = __hostcall(__hostaddr0(), __uxa); } \
    __uxa[0] = __uxk; __uxa[1] = (__u_word)(name); (dst) = __hostcall(__hostaddr1(), __uxa); \
} while (0)
#define _ux_go(dst, fn, x0, x1, x2, x3) do { \
    __u_word __uxa[10]; int __uxi; \
    __uxi = 0; while (__uxi < 10) { __uxa[__uxi] = 0; __uxi = __uxi + 1; } \
    __uxa[0] = (__u_word)(x0); __uxa[1] = (__u_word)(x1); __uxa[2] = (__u_word)(x2); __uxa[3] = (__u_word)(x3); \
    (dst) = __hostcall((fn), __uxa); \
} while (0)
#define _ux_err(dst) do { \
    __u_word __uxa[10]; __u_word __uxe; int __uxi; \
    __uxi = 0; while (__uxi < 10) { __uxa[__uxi] = 0; __uxi = __uxi + 1; } \
    __uxe = __hostcall(__hostaddr3(), __uxa) & 0xFFFFFFFFLL; \
    if (__uxe == 2 || __uxe == 3) (dst) = ENOENT; \
    else if (__uxe == 5) (dst) = EACCES; \
    else if (__uxe == 6) (dst) = EBADF; \
    else if (__uxe == 80 || __uxe == 183) (dst) = EEXIST; \
    else if (__uxe == 145) (dst) = ENOTEMPTY; \
    else if (__uxe == 32) (dst) = EBUSY; \
    else if (__uxe == 87) (dst) = EINVAL; \
    else (dst) = EIO; \
} while (0)
#if !__UNISA_FTRIM_LIBC || __UN__ux_ft2unix
static __u_word _ux_ft2unix(const unsigned int *__u_ft) {   /* FILETIME (100 ns since 1601) -> unix seconds */
    __u_word __u_t; __u_t = ((__u_word)__u_ft[1] << 32) | (__u_word)__u_ft[0];
    return (__u_t - 116444736000000000LL) / 10000000LL;
}
#endif
#endif
#endif

#endif
#if !__UNISA_FTRIM_LIBC || __UN_mkstemp
static int mkstemp(char *__u_t) {
    static __u_uword __u_seq;
    __u_word __u_n; __u_word __u_j; int __u_try; __u_word __u_r; __u_uword __u_v;
    __u_n = 0; while (__u_t[__u_n]) __u_n = __u_n + 1;
    if (__u_n < 6) { errno = EINVAL; return 0 - 1; }
    __u_j = __u_n - 6; while (__u_j < __u_n) { if (__u_t[__u_j] != 88) { errno = EINVAL; return 0 - 1; } __u_j = __u_j + 1; }
    __u_try = 0;
    while (__u_try < 100) {
        __u_seq = __u_seq + 1;
        __u_v = ((__u_uword)&__u_try >> 4) * 2654435761u + __u_seq * 40503u;
        __u_j = __u_n - 6;
        while (__u_j < __u_n) { __u_t[__u_j] = (char)(97 + __u_v % 26); __u_v = __u_v / 26 + __u_seq * 7; __u_j = __u_j + 1; }
#ifdef _WIN32
        /* GENERIC_READ|GENERIC_WRITE, CREATE_NEW. The gate returns -1; errno comes from GetLastError. */
        __u_r = __open(__u_t, 0xC0000000L, 1);
        if (__u_r >= 0) return (int)__u_r;
        { int __u_e; _ux_err(__u_e); errno = __u_e; }
#else
#ifdef __APPLE__
        __u_r = __open(__u_t, 0xA02, 384);
#else
        __u_r = __open(__u_t, 0302, 384);
#endif
        if (__u_r >= 0) return (int)__u_r;
        errno = (int)(0 - __u_r);
#endif
        if (errno != EEXIST) return 0 - 1;
        __u_try = __u_try + 1;
    }
    return 0 - 1;
}
#endif
/* realpath (POSIX, 0.0.37 C3): open the path and ask the kernel for the name it resolved
   (Apple F_GETPATH; Linux readlink of /proc/self/fd/N), so symlinks, "." and ".." are the
   kernel's answer, not a lexical guess.  The path must exist, as POSIX requires; a file
   the caller cannot open fails with the open's errno.  NULL resolved: malloc(PATH_MAX).
   Windows (2026-10-26, after Cosmopolitan): open the path and ask GetFinalPathNameByHandleA
   (VOLUME_NAME_DOS), then drop a leading \\?\ or turn \\?\UNC\ into \\. The __open gate
   cannot pass FILE_FLAG_BACKUP_SEMANTICS, so a directory does not open; that case, and any
   final-name failure, falls back to GetFileAttributesA plus GetFullPathNameA. The fallback
   is absolute and does not resolve a symlink. This slice has not been executed on Windows. */
#ifndef __UNISA_PYFRONT
#if !__UNISA_FTRIM_LIBC || __UN_realpath
static char *realpath(const char *__u_p, char *__u_out) {
    char __u_tmp[PATH_MAX]; __u_word __u_fd; __u_word __u_r; __u_word __u_n; __u_word __u_k; char *__u_d;
#ifdef _WIN32
    static __u_word __u_attr, __u_full, __u_fin; __u_word __u_a; __u_word __u_h; __u_word __u_off; int __u_e;
    if (!__u_attr) {
        _ux_load(__u_attr, "GetFileAttributesA");
        _ux_load(__u_full, "GetFullPathNameA");
        _ux_load(__u_fin, "GetFinalPathNameByHandleA");
    }
    __u_n = 0;
    __u_h = __open((char *)__u_p, 0x80000000L, 3);          /* GENERIC_READ, OPEN_EXISTING */
    if (__u_h >= 0) {
        /* Length excludes the NUL. Zero, or a size that does not fit, is not a path. */
        _ux_go(__u_n, __u_fin, __u_h, (__u_word)__u_tmp, PATH_MAX, 0);
        __u_n = __u_n & 0xFFFFFFFFL;
        __close(__u_h);
        if (__u_n == 0 || __u_n >= PATH_MAX) __u_n = 0;
    }
    if (__u_n > 0 && __u_n >= 4 && __u_tmp[0] == 92 && __u_tmp[1] == 92 && __u_tmp[2] == 63 && __u_tmp[3] == 92) {
        __u_off = 4;
        if (__u_n >= 8 && (__u_tmp[4] == 85 || __u_tmp[4] == 117) && (__u_tmp[5] == 78 || __u_tmp[5] == 110) &&
            (__u_tmp[6] == 67 || __u_tmp[6] == 99) && __u_tmp[7] == 92) {
            __u_tmp[0] = 92; __u_tmp[1] = 92; __u_off = 8; __u_k = 2;
        } else __u_k = 0;
        __u_a = __u_off;
        while (__u_a <= __u_n) { __u_tmp[__u_k] = __u_tmp[__u_a]; __u_k = __u_k + 1; __u_a = __u_a + 1; }
        __u_n = __u_k - 1;
    }
    if (__u_n == 0) {
        _ux_go(__u_a, __u_attr, (__u_word)__u_p, 0, 0, 0);
        __u_a = __u_a & 0xFFFFFFFFL;
        if (__u_a == 0xFFFFFFFFL) { _ux_err(__u_e); errno = __u_e; return 0; }
        _ux_go(__u_n, __u_full, (__u_word)__u_p, PATH_MAX, (__u_word)__u_tmp, 0);
        __u_n = __u_n & 0xFFFFFFFFL;
        if (__u_n == 0) { _ux_err(__u_e); errno = __u_e; return 0; }
        if (__u_n >= PATH_MAX) { errno = ENAMETOOLONG; return 0; }
    }
    __u_d = __u_out;
    if (!__u_d) { __u_d = (char *)malloc(PATH_MAX); if (!__u_d) { errno = ENOMEM; return 0; } }
    __u_k = 0; while (__u_k <= __u_n) { __u_d[__u_k] = __u_tmp[__u_k]; __u_k = __u_k + 1; }
    return __u_d;
#else
#ifdef __APPLE__
    __u_fd = __open((char *)__u_p, 0, 0);
    if (__u_fd < 0) { errno = (int)(0 - __u_fd); return 0; }
    __u_r = __fcntl(__u_fd, 50, (long)__u_tmp);   /* F_GETPATH */
    __close(__u_fd);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return 0; }
    __u_n = 0; while (__u_tmp[__u_n]) __u_n = __u_n + 1;
#else
    char __u_fdp[32]; long __u_v; char __u_dig[24];
    __u_fd = __open((char *)__u_p, 010000000, 0);   /* O_PATH: no read permission needed */
    if (__u_fd < 0) { errno = (int)(0 - __u_fd); return 0; }
    __u_k = 0; while ("/proc/self/fd/"[__u_k]) { __u_fdp[__u_k] = "/proc/self/fd/"[__u_k]; __u_k = __u_k + 1; }
    __u_v = __u_fd; __u_n = 0;
    do { __u_dig[__u_n] = (char)(48 + __u_v % 10); __u_v = __u_v / 10; __u_n = __u_n + 1; } while (__u_v);
    while (__u_n) { __u_n = __u_n - 1; __u_fdp[__u_k] = __u_dig[__u_n]; __u_k = __u_k + 1; }
    __u_fdp[__u_k] = 0;
#ifdef __x86_64__
    __u_r = __syscall6(89, (long)__u_fdp, (long)__u_tmp, PATH_MAX - 1, 0, 0);          /* readlink */
#else
    __u_r = __syscall6(78, -100, (long)__u_fdp, (long)__u_tmp, PATH_MAX - 1, 0);       /* readlinkat */
#endif
    __close(__u_fd);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return 0; }
    if (__u_r >= PATH_MAX - 1) { errno = ENAMETOOLONG; return 0; }
    __u_n = __u_r; __u_tmp[__u_n] = 0;
#endif
    __u_d = __u_out;
    if (!__u_d) { __u_d = (char *)malloc(PATH_MAX); if (!__u_d) { errno = ENOMEM; return 0; } }
    __u_k = 0; while (__u_k <= __u_n) { __u_d[__u_k] = __u_tmp[__u_k]; __u_k = __u_k + 1; }
    return __u_d;
#endif
}
#endif
#endif
/* mkdtemp (POSIX, 0.0.35 M1): mkstemp's names, but a directory, mode 0700.
   Windows (2026-10-26, after Cosmopolitan): CreateDirectoryA, mode ignored.
   The template must end in XXXXXX. EEXIST retries. Any other error, and a
   retry limit, put XXXXXX back and return NULL. __mkdir stays on the Unix
   branch: it has no Windows import, and the Python front end folds one tape. */
#ifndef __UNISA_PYFRONT
#if !__UNISA_FTRIM_LIBC || __UN_mkdtemp
static char *mkdtemp(char *__u_t) {
    static __u_uword __u_seq;
    __u_word __u_n; __u_word __u_j; int __u_try; __u_word __u_r; __u_uword __u_v;
    __u_n = 0; while (__u_t[__u_n]) __u_n = __u_n + 1;
    if (__u_n < 6) { errno = EINVAL; return 0; }
    __u_j = __u_n - 6; while (__u_j < __u_n) { if (__u_t[__u_j] != 88) { errno = EINVAL; return 0; } __u_j = __u_j + 1; }
    __u_try = 0;
    while (__u_try < 100) {
        __u_seq = __u_seq + 1;
        __u_v = ((__u_uword)&__u_try >> 4) * 2654435761u + __u_seq * 40503u + 17;
        __u_j = __u_n - 6;
        while (__u_j < __u_n) { __u_t[__u_j] = (char)(97 + __u_v % 26); __u_v = __u_v / 26 + __u_seq * 7; __u_j = __u_j + 1; }
#ifdef _WIN32
        {   static __u_word __u_cd; __u_word __u_ok; int __u_e;
            if (!__u_cd) _ux_load(__u_cd, "CreateDirectoryA");
            _ux_go(__u_ok, __u_cd, (__u_word)__u_t, 0, 0, 0);
            __u_ok = __u_ok & 0xFFFFFFFFL;
            if (__u_ok) return __u_t;
            _ux_err(__u_e); errno = __u_e;
            if (errno != EEXIST) {
                __u_j = __u_n - 6; while (__u_j < __u_n) { __u_t[__u_j] = 88; __u_j = __u_j + 1; }
                return 0;
            }
        }
#else
        __u_r = __mkdir(__u_t, 448, 0);
        if (__u_r >= 0) return __u_t;
        errno = (int)(0 - __u_r);
        if (errno != EEXIST) return 0;
#endif
        __u_try = __u_try + 1;
    }
#ifdef _WIN32
    __u_j = __u_n - 6; while (__u_j < __u_n) { __u_t[__u_j] = 88; __u_j = __u_j + 1; }
#endif
    return 0;
}
#endif
#endif
/* strtod's slow path (more than 19 digits, or a power of ten past 22) is a
 * guess a few ulp off; these compare the decimal with the halfway points around
 * it in big integers (base 2^32) and step to the correctly rounded double. */
#define _U_BL 200
#if !__UNISA_FTRIM_LIBC || __UN__u_bn_muls
static void _u_bn_muls(unsigned int *__u_a, int *__u_n, unsigned int __u_k) {
    __u_uword __u_c; int __u_i; __u_c = 0;
    for (__u_i = 0; __u_i < *__u_n; __u_i++) { __u_c = __u_c + (__u_uword)__u_a[__u_i] * __u_k; __u_a[__u_i] = (unsigned int)__u_c; __u_c = __u_c >> 32; }
    if (__u_c && *__u_n < _U_BL) { __u_a[*__u_n] = (unsigned int)__u_c; *__u_n = *__u_n + 1; }
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__u_bn_shl
static void _u_bn_shl(unsigned int *__u_a, int *__u_n, __u_word __u_b) {
    int __u_w; int __u_s; int __u_i;
    __u_w = (int)(__u_b >> 5); __u_s = (int)(__u_b & 31);
    if (*__u_n == 0) return;
    if (__u_s) _u_bn_muls(__u_a, __u_n, 1u << __u_s);
    if (__u_w) {
        if (*__u_n + __u_w > _U_BL) __u_w = _U_BL - *__u_n;
        for (__u_i = *__u_n - 1; __u_i >= 0; __u_i--) __u_a[__u_i + __u_w] = __u_a[__u_i];
        for (__u_i = 0; __u_i < __u_w; __u_i++) __u_a[__u_i] = 0;
        *__u_n = *__u_n + __u_w;
    }
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__u_bn_pow10
static void _u_bn_pow10(unsigned int *__u_a, int *__u_n, __u_word __u_x) {
    while (__u_x >= 9) { _u_bn_muls(__u_a, __u_n, 1000000000u); __u_x = __u_x - 9; }
    while (__u_x > 0) { _u_bn_muls(__u_a, __u_n, 10u); __u_x = __u_x - 1; }
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__u_bn_cmp
static int _u_bn_cmp(unsigned int *__u_a, int __u_na, unsigned int *__u_b, int __u_nb) {
    int __u_i;
    while (__u_na > 0 && __u_a[__u_na - 1] == 0) __u_na = __u_na - 1;
    while (__u_nb > 0 && __u_b[__u_nb - 1] == 0) __u_nb = __u_nb - 1;
    if (__u_na != __u_nb) return __u_na > __u_nb ? 1 : 0 - 1;
    for (__u_i = __u_na - 1; __u_i >= 0; __u_i--) if (__u_a[__u_i] != __u_b[__u_i]) return __u_a[__u_i] > __u_b[__u_i] ? 1 : 0 - 1;
    return 0;
}
#endif
/* sign of D*10^x10 - (2m+1)*2^(e-1), the halfway point above v; D = the digits
   s[0..len) skipping '.', at most 780 significant (the rest only make it bigger) */
#if !__UNISA_FTRIM_LIBC || __UN__u_sd_cmp
static int _u_sd_cmp(const char *__u_s, __u_word __u_len, __u_word __u_x10, double __u_v) {
    static unsigned int __u_d[_U_BL]; static unsigned int __u_h[_U_BL];
    int __u_nd; int __u_nh; int __u_sticky; int __u_c; __u_word __u_i; __u_word __u_cnt; __u_word __u_xx;
    __u_uword __u_bits; __u_uword __u_m; __u_word __u_e;
    __u_nd = 0; __u_sticky = 0; __u_cnt = 0; __u_xx = __u_x10;
    for (__u_i = 0; __u_i < __u_len; __u_i++) {
        int __u_ch; __u_uword __u_c2; int __u_j;
        __u_ch = __u_s[__u_i];
        if (__u_ch == 46) continue;
        if (__u_cnt >= 780) { if (__u_ch != 48) __u_sticky = 1; __u_xx = __u_xx + 1; continue; }
        if (__u_nd == 0 && __u_ch == 48) continue;
        if (__u_nd == 0) { __u_d[0] = 0; __u_nd = 1; }
        _u_bn_muls(__u_d, &__u_nd, 10u);
        __u_c2 = (__u_uword)(__u_ch - 48);
        for (__u_j = 0; __u_c2 && __u_j < __u_nd; __u_j++) { __u_c2 = __u_c2 + __u_d[__u_j]; __u_d[__u_j] = (unsigned int)__u_c2; __u_c2 = __u_c2 >> 32; }
        if (__u_c2 && __u_nd < _U_BL) { __u_d[__u_nd] = (unsigned int)__u_c2; __u_nd = __u_nd + 1; }
        __u_cnt = __u_cnt + 1;
    }
    if (__u_nd == 0) return 0 - 1;
    __u_bits = *(__u_uword *)&__u_v;
    __u_e = (__u_word)((__u_bits >> 52) & 2047); __u_m = __u_bits & 4503599627370495;
    if (__u_e == 0) __u_e = 1; else __u_m = __u_m | 4503599627370496;
    __u_e = __u_e - 1075;
    __u_m = 2 * __u_m + 1;
    __u_nh = 2; __u_h[0] = (unsigned int)__u_m; __u_h[1] = (unsigned int)(__u_m >> 32);
    if (__u_xx > 0) _u_bn_pow10(__u_d, &__u_nd, __u_xx); else _u_bn_pow10(__u_h, &__u_nh, 0 - __u_xx);
    if (__u_e - 1 > 0) _u_bn_shl(__u_h, &__u_nh, __u_e - 1); else _u_bn_shl(__u_d, &__u_nd, 1 - __u_e);
    __u_c = _u_bn_cmp(__u_d, __u_nd, __u_h, __u_nh);
    if (__u_c == 0 && __u_sticky) __u_c = 1;
    return __u_c;
}
#endif
/* walk the guess (finite, >= 0, a few ulp off) to the correctly rounded double */
#if !__UNISA_FTRIM_LIBC || __UN__u_sd_fix
static double _u_sd_fix(const char *__u_s, __u_word __u_len, __u_word __u_x10, double __u_v) {
    __u_uword __u_b; __u_uword __u_lb; int __u_c; int __u_k;
    for (__u_k = 0; __u_k < 64; __u_k++) {
        __u_b = *(__u_uword *)&__u_v;
        if (__u_b >= 9218868437227405312) return __u_v;              /* inf or nan */
        __u_c = _u_sd_cmp(__u_s, __u_len, __u_x10, __u_v);
        if (__u_c > 0 || (__u_c == 0 && (__u_b & 1))) { __u_b = __u_b + 1; __u_v = *(double *)&__u_b; continue; }
        if (__u_b == 0) return __u_v;
        __u_lb = __u_b - 1;
        __u_c = _u_sd_cmp(__u_s, __u_len, __u_x10, *(double *)&__u_lb);
        if (__u_c < 0 || (__u_c == 0 && !(__u_lb & 1))) { __u_v = *(double *)&__u_lb; continue; }
        return __u_v;
    }
    return __u_v;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_strtod
static double strtod(const char *__u_s, char **__u_end) {
    double __u_v; int __u_sign; __u_word __u_i; int __u_any;
    __u_word __u_e; int __u_esign;
    __u_v = 0.0; __u_sign = 1; __u_i = 0; __u_any = 0;
    while (__u_s[__u_i] == 32 || (__u_s[__u_i] >= 9 && __u_s[__u_i] <= 13)) __u_i = __u_i + 1;
    if (__u_s[__u_i] == 45) { __u_sign = 0 - 1; __u_i = __u_i + 1; }
    else { if (__u_s[__u_i] == 43) __u_i = __u_i + 1; }
    /* C99 hexadecimal form: 0x hex-digits [. hex-digits] [p [+-] decimal] */
    if (__u_s[__u_i] == 48 && (__u_s[__u_i + 1] | 32) == 120) {
        __u_word __u_k; int __u_d; __u_word __u_be; int __u_hany;
        __u_k = __u_i + 2; __u_hany = 0; __u_be = 0;
        while (1) {
            __u_d = __u_s[__u_k];
            if (__u_d >= 48 && __u_d <= 57) __u_d = __u_d - 48;
            else { if ((__u_d | 32) >= 97 && (__u_d | 32) <= 102) __u_d = (__u_d | 32) - 87; else break; }
            if (__u_v < 72057594037927936.0) __u_v = __u_v * 16.0 + (double)__u_d; else __u_be = __u_be + 4;
            __u_k = __u_k + 1; __u_hany = 1;
        }
        if (__u_s[__u_k] == 46) {
            __u_k = __u_k + 1;
            while (1) {
                __u_d = __u_s[__u_k];
                if (__u_d >= 48 && __u_d <= 57) __u_d = __u_d - 48;
                else { if ((__u_d | 32) >= 97 && (__u_d | 32) <= 102) __u_d = (__u_d | 32) - 87; else break; }
                if (__u_v < 72057594037927936.0) { __u_v = __u_v * 16.0 + (double)__u_d; __u_be = __u_be - 4; }
                __u_k = __u_k + 1; __u_hany = 1;
            }
        }
        if (__u_hany == 0) { if (__u_end) *__u_end = (char *)(__u_s + __u_i + 1); return 0.0 * (double)__u_sign; }
        if ((__u_s[__u_k] | 32) == 112) {
            __u_word __u_j; __u_j = __u_k + 1; __u_esign = 1;
            if (__u_s[__u_j] == 45) { __u_esign = 0 - 1; __u_j = __u_j + 1; }
            else { if (__u_s[__u_j] == 43) __u_j = __u_j + 1; }
            if (__u_s[__u_j] >= 48 && __u_s[__u_j] <= 57) {
                __u_e = 0;
                while (__u_s[__u_j] >= 48 && __u_s[__u_j] <= 57) { if (__u_e < 100000) __u_e = __u_e * 10 + (__u_s[__u_j] - 48); __u_j = __u_j + 1; }
                __u_k = __u_j; __u_be = __u_be + __u_esign * __u_e;
            }
        }
        if (__u_v != 0.0) {
            while (__u_be > 0) { __u_v = __u_v * 2.0; __u_be = __u_be - 1; }
            while (__u_be < 0) { __u_v = __u_v / 2.0; __u_be = __u_be + 1; if (__u_v == 0.0) break; }
        }
        if (__u_end) *__u_end = (char *)(__u_s + __u_k);
        return __u_v * (double)__u_sign;
    }
    /* Up to 19 significant digits are kept exactly in an integer and scaled
     * once by a power of ten; with at most 2^53 and |power| <= 22 that is a
     * single rounding (Clinger's fast path), so 2^63 and 10^i come out exact. */
    {   __u_uword __u_m; __u_word __u_x10; double __u_p; __u_word __u_q;
        __u_word __u_ds; __u_word __u_de; __u_word __u_fd; __u_word __u_ex; int __u_drop;
        __u_m = 0; __u_x10 = 0; __u_ds = __u_i; __u_fd = 0; __u_ex = 0; __u_drop = 0;
        while (__u_s[__u_i] >= 48 && __u_s[__u_i] <= 57) {
            if (__u_m < 1844674407370955161) __u_m = __u_m * 10 + (__u_uword)(__u_s[__u_i] - 48); else { __u_x10 = __u_x10 + 1; __u_drop = 1; }
            __u_i = __u_i + 1; __u_any = 1;
        }
        if (__u_s[__u_i] == 46) {
            __u_i = __u_i + 1;
            while (__u_s[__u_i] >= 48 && __u_s[__u_i] <= 57) {
                if (__u_m < 1844674407370955161) { __u_m = __u_m * 10 + (__u_uword)(__u_s[__u_i] - 48); __u_x10 = __u_x10 - 1; } else __u_drop = 1;
                __u_i = __u_i + 1; __u_any = 1; __u_fd = __u_fd + 1;
            }
        }
        __u_de = __u_i;
        if (__u_any) { if (__u_s[__u_i] == 101 || __u_s[__u_i] == 69) {
            __u_word __u_j; __u_j = __u_i + 1; __u_esign = 1;
            if (__u_s[__u_j] == 45) { __u_esign = 0 - 1; __u_j = __u_j + 1; }
            else { if (__u_s[__u_j] == 43) __u_j = __u_j + 1; }
            if (__u_s[__u_j] >= 48 && __u_s[__u_j] <= 57) {
                __u_e = 0;
                while (__u_s[__u_j] >= 48 && __u_s[__u_j] <= 57) { if (__u_e < 100000) __u_e = __u_e * 10 + (__u_s[__u_j] - 48); __u_j = __u_j + 1; }
                __u_i = __u_j; __u_x10 = __u_x10 + __u_esign * __u_e; __u_ex = __u_esign * __u_e;
            }
        } }
        while (__u_m != 0 && __u_x10 < 0 && __u_m % 10 == 0) { __u_m = __u_m / 10; __u_x10 = __u_x10 + 1; }
        __u_drop = __u_drop || __u_m > 9007199254740992 || __u_x10 > 22 || __u_x10 < 0 - 22;
        if (__u_m >> 63) __u_v = (double)(__u_word)(__u_m >> 1) * 2.0 + (double)(__u_word)(__u_m & 1); else __u_v = (double)(__u_word)__u_m;
        /* decimal magnitude of the leading digit: past 10^309 the value overflows, below 10^-325
           it underflows to zero -- decided here, because the big-integer walk below only moves a
           few ulp and its integers do not reach such exponents */
        if (__u_m != 0) {
            __u_uword __u_t; __u_word __u_mag; __u_t = __u_m; __u_mag = __u_x10 - 1;
            while (__u_t) { __u_t = __u_t / 10; __u_mag = __u_mag + 1; }
            if (__u_mag > 309) { __u_m = 0; __u_v = 1.0e300 * 1.0e300; __u_drop = 0; }
            else { if (__u_mag < 0 - 325) { __u_m = 0; __u_v = 0.0; __u_drop = 0; errno = ERANGE; } }
        }
        if (__u_m != 0) {
            while (__u_x10 > 22) { __u_v = __u_v * 1.0e22; __u_x10 = __u_x10 - 22; }
            while (__u_x10 < 0 - 22) { __u_v = __u_v / 1.0e22; __u_x10 = __u_x10 + 22; }
            __u_p = 1.0; __u_q = __u_x10 < 0 ? 0 - __u_x10 : __u_x10;
            while (__u_q > 0) { __u_p = __u_p * 10.0; __u_q = __u_q - 1; }
            if (__u_x10 > 0) __u_v = __u_v * __u_p; else { if (__u_x10 < 0) __u_v = __u_v / __u_p; }
        }
        if (__u_m != 0 && __u_drop) {
            if (__u_v > 1.7976931348623157e308) __u_v = 1.7976931348623157e308;
            __u_v = _u_sd_fix(__u_s + __u_ds, __u_de - __u_ds, __u_ex - __u_fd, __u_v);
        }
        /* C99 7.20.1.3p10: overflow, and underflow (zero or subnormal from a nonzero decimal), set ERANGE */
        if (__u_v > 1.7976931348623157e308) errno = ERANGE;
        else { if (__u_m != 0 && __u_v < 2.2250738585072014e-308) errno = ERANGE; }
    }
    if (__u_end) *__u_end = (char *)(__u_s + (__u_any ? __u_i : 0));
    return __u_v * (double)__u_sign;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_strtof
static float strtof(const char *__u_s, char **__u_end) { return (float)strtod(__u_s, __u_end); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_strtold
static double strtold(const char *__u_s, char **__u_end) { return strtod(__u_s, __u_end); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_atof
static double atof(const char *__u_s) { return strtod(__u_s, 0); }
#endif

static __u_word _unisa_seed = 1;
#if !__UNISA_FTRIM_LIBC || __UN_rand
static int rand(void) {
    _unisa_seed = _unisa_seed * 1103515245 + 12345;
    return (int)((_unisa_seed >> 16) & 32767);
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_srand
static void srand(int __u_s) { _unisa_seed = __u_s; }
#endif
/* ---- C99 7.20.6-7.20.7 and the pieces of 7.20.4 a program can rely on
   here: labs, div, qsort, bsearch, atexit.  Real code, like the rest of
   this header: the sort is a heap sort, so its worst case is bounded and
   it needs no stack beyond one element's bytes [S-15 D2]. */
#if !__UNISA_FTRIM_LIBC || __UN_labs
static long labs(long __u_v) { if (__u_v < 0) return 0 - __u_v; return __u_v; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_llabs
static long long llabs(long long __u_v) { if (__u_v < 0) return 0 - __u_v; return __u_v; }
#endif
typedef struct { int quot; int rem; } div_t;
typedef struct { long quot; long rem; } ldiv_t;
typedef struct { long long quot; long long rem; } lldiv_t;
#if !__UNISA_FTRIM_LIBC || __UN_div
static div_t div(int __u_a, int __u_b) { div_t __u_r; __u_r.quot = __u_a / __u_b; __u_r.rem = __u_a % __u_b; return __u_r; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_ldiv
static ldiv_t ldiv(long __u_a, long __u_b) { ldiv_t __u_r; __u_r.quot = __u_a / __u_b; __u_r.rem = __u_a % __u_b; return __u_r; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_lldiv
static lldiv_t lldiv(long long __u_a, long long __u_b) { lldiv_t __u_r; __u_r.quot = __u_a / __u_b; __u_r.rem = __u_a % __u_b; return __u_r; }
#endif

#if !__UNISA_FTRIM_LIBC || __UN__unisa_swap
static void _unisa_swap(char *__u_a, char *__u_b, __u_word __u_n) {
    __u_word __u_i; char __u_t;
    __u_i = 0;
    while (__u_i < __u_n) { __u_t = __u_a[__u_i]; __u_a[__u_i] = __u_b[__u_i]; __u_b[__u_i] = __u_t; __u_i = __u_i + 1; }
}
#endif

/* Sift element `i` of a heap of `n` elements down to its place. */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sift
static void _unisa_sift(char *__u_base, __u_word __u_n, __u_word __u_sz, __u_word __u_i, int (*__u_cmp)(const void *, const void *)) {
    __u_word __u_c;
    while (1) {
        __u_c = 2 * __u_i + 1;
        if (__u_c >= __u_n) return;
        if (__u_c + 1 < __u_n) { if (__u_cmp(__u_base + __u_c * __u_sz, __u_base + (__u_c + 1) * __u_sz) < 0) __u_c = __u_c + 1; }
        if (__u_cmp(__u_base + __u_i * __u_sz, __u_base + __u_c * __u_sz) >= 0) return;
        _unisa_swap(__u_base + __u_i * __u_sz, __u_base + __u_c * __u_sz, __u_sz);
        __u_i = __u_c;
    }
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_qsort
static void qsort(void *__u_v, __u_word __u_n, __u_word __u_sz, int (*__u_cmp)(const void *, const void *)) {
    char *__u_base; __u_word __u_i;
    __u_base = (char *)__u_v;
    if (__u_n < 2) return;
    __u_i = __u_n / 2;
    while (__u_i > 0) { __u_i = __u_i - 1; _unisa_sift(__u_base, __u_n, __u_sz, __u_i, __u_cmp); }
    __u_i = __u_n;
    while (__u_i > 1) {
        __u_i = __u_i - 1;
        _unisa_swap(__u_base, __u_base + __u_i * __u_sz, __u_sz);
        _unisa_sift(__u_base, __u_i, __u_sz, 0, __u_cmp);
    }
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_bsearch
static void *bsearch(const void *__u_key, const void *__u_v, __u_word __u_n, __u_word __u_sz, int (*__u_cmp)(const void *, const void *)) {
    char *__u_base; __u_word __u_lo; __u_word __u_hi; __u_word __u_mid; int __u_c;
    __u_base = (char *)__u_v; __u_lo = 0; __u_hi = __u_n;
    while (__u_lo < __u_hi) {
        __u_mid = __u_lo + (__u_hi - __u_lo) / 2;
        __u_c = __u_cmp(__u_key, __u_base + __u_mid * __u_sz);
        if (__u_c == 0) return __u_base + __u_mid * __u_sz;
        if (__u_c < 0) __u_hi = __u_mid; else __u_lo = __u_mid + 1;
    }
    return 0;
}
#endif

/* atexit: the handlers run, last registered first, from `exit` -- which is
   also where a return from main ends up, since the entry stub calls `exit`
   whenever this header defined one.  Thirty-two is C's minimum (7.20.4.2). */
#if !__UNISA_FTRIM_LIBC || __UN_atexit
static int atexit(void (*__u_fn)(void)) {
    if (_unisa_natexit >= 32) return 0 - 1;
    _unisa_atexit[_unisa_natexit] = __u_fn;
    _unisa_natexit = _unisa_natexit + 1;
    return 0;
}
#endif

/* The environment is where a Unix kernel leaves it: after argv's NULL.
   `__argv(k)` reads that array itself, so the walk starts past argc.  It is
   copied once into environ (below), and environ is the one store: getenv
   reads it, setenv/unsetenv edit it in place (POSIX), and every child --
   execv/execvp/execl*, system, popen -- is handed it, so a variable set
   here is what a child sees (it was a 16-name override table of 255 bytes
   each that only getenv consulted, until 0.0.43). [S-15 D2] */
#ifdef _WIN32
#endif
/* retired with the override table (0.0.43); kept unreferenced so a program that calls no
   environment function keeps its bytes -- delete together with the next bss change */
static char _unisa_envk[16][256];
static int _unisa_envf[16];
/* 0.0.31 H1'': the process environment, one `char **` for the whole program: external linkage,
   a tentative definition in every unit, merged at link (C99 6.9.2).  <unistd.h> names it `environ`.
   A constructor fills it before main from the block the kernel left after argv (0.0.28 F1);
   -ftrim-libc keeps it as a root (unisa/libneed.py ROOTS). */
char **_unisa_env;
static char *_unisa_envv[4096];
#if !__UNISA_FTRIM_LIBC || __UN__unisa_environ_init
__attribute__((constructor)) static void _unisa_environ_init(void) {
    int __u_k; int __u_n;
    if (_unisa_env) return;                 /* another unit's constructor already filled it */
    __u_k = __argc() + 1; __u_n = 0;
    while (__u_n < 4095 && __argv(__u_k) != 0) { _unisa_envv[__u_n] = __argv(__u_k); __u_n = __u_n + 1; __u_k = __u_k + 1; }
    _unisa_envv[__u_n] = 0;
    _unisa_env = _unisa_envv;
}
#endif
/* Windows reads the real environment through the host channel in 0.0.22,
   when the product lowers it (the compiler itself must not need it yet). */
/* the index in environ of the entry "NAME=...", or -1 (NAME holds no '=') */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_envfind
static __u_word _unisa_envfind(const char *__u_name) {
    __u_word __u_k; int __u_i; char *__u_e;
    if (!_unisa_env) _unisa_environ_init();   /* called from a constructor that ran first */
    for (__u_k = 0; (__u_e = _unisa_env[__u_k]) != 0; __u_k++) {
        __u_i = 0;
        while (__u_name[__u_i] && __u_e[__u_i] == __u_name[__u_i]) __u_i = __u_i + 1;
        if (__u_name[__u_i] == 0 && __u_e[__u_i] == 61) return __u_k;
    }
    return -1;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_getenv
static char *getenv(const char *__u_name) {
    __u_word __u_k; int __u_i;
    __u_k = _unisa_envfind(__u_name);
    if (__u_k < 0) return 0;
    __u_i = 0; while (_unisa_env[__u_k][__u_i] != 61) __u_i = __u_i + 1;
    return _unisa_env[__u_k] + __u_i + 1;
}
#endif
/* setenv: "NAME=VALUE" in one malloc'd string, any length, put in environ in place of
   NAME's entry or appended.  An entry is never freed: a pointer getenv returned stays
   valid, as glibc and the BSDs keep them.  Appending past the kernel's copy (or past an
   array the program assigned to environ) moves environ to a malloc'd array twice as big. */
#if !__UNISA_FTRIM_LIBC || __UN_setenv
static char **_unisa_envheap; static __u_word _unisa_envcap;
#endif
#if !__UNISA_FTRIM_LIBC || __UN_setenv
static int setenv(const char *__u_name, const char *__u_val, int __u_over) {
    __u_word __u_k; __u_word __u_n; __u_word __u_i; __u_word __u_j; char *__u_s; char **__u_a;
    if (!__u_name || !__u_name[0]) { errno = EINVAL; return -1; }
    for (__u_i = 0; __u_name[__u_i]; __u_i++) if (__u_name[__u_i] == 61) { errno = EINVAL; return -1; }
    __u_k = _unisa_envfind(__u_name);
    if (__u_k >= 0 && !__u_over) return 0;
    __u_j = 0; while (__u_val[__u_j]) __u_j = __u_j + 1;
    __u_s = (char *)malloc(__u_i + __u_j + 2);
    if (!__u_s) { errno = ENOMEM; return -1; }
    for (__u_j = 0; __u_name[__u_j]; __u_j++) __u_s[__u_j] = __u_name[__u_j];
    __u_s[__u_i] = 61;
    for (__u_j = 0; __u_val[__u_j]; __u_j++) __u_s[__u_i + 1 + __u_j] = __u_val[__u_j];
    __u_s[__u_i + 1 + __u_j] = 0;
    if (__u_k >= 0) { _unisa_env[__u_k] = __u_s; return 0; }
    __u_n = 0; while (_unisa_env[__u_n]) __u_n = __u_n + 1;
    if (!((_unisa_env == _unisa_envv && __u_n + 1 < 4096) || (_unisa_env == _unisa_envheap && __u_n + 1 < _unisa_envcap))) {
        __u_a = (char **)malloc((2 * __u_n + 16) * sizeof(char *));
        if (!__u_a) { free(__u_s); errno = ENOMEM; return -1; }
        for (__u_k = 0; __u_k < __u_n; __u_k++) __u_a[__u_k] = _unisa_env[__u_k];
        if (_unisa_env == _unisa_envheap) free(_unisa_envheap);
        _unisa_envheap = __u_a; _unisa_envcap = 2 * __u_n + 16; _unisa_env = __u_a;
    }
    _unisa_env[__u_n] = __u_s; _unisa_env[__u_n + 1] = 0;
    return 0;
}
#endif
/* unsetenv: every "NAME=..." entry leaves environ, the rest keep their order */
#if !__UNISA_FTRIM_LIBC || __UN_unsetenv
static int unsetenv(const char *__u_name) {
    __u_word __u_k; __u_word __u_m; int __u_i;
    if (!__u_name || !__u_name[0]) { errno = EINVAL; return -1; }
    for (__u_i = 0; __u_name[__u_i]; __u_i++) if (__u_name[__u_i] == 61) { errno = EINVAL; return -1; }
    while ((__u_k = _unisa_envfind(__u_name)) >= 0) {
        __u_m = __u_k; while (_unisa_env[__u_m]) { _unisa_env[__u_m] = _unisa_env[__u_m + 1]; __u_m = __u_m + 1; }
    }
    return 0;
}
#endif

/* system: /bin/sh -c CMD in a child, waited for; the kernel's calls, no
   host library (0.0.21: the compiler must build without the host channel).
   Windows: not yet (0.0.22, through the host channel). */
#if !defined(_WIN32) && !defined(__UNISA_PYFRONT)   /* the Python front end keeps every body and folds one tape across targets */
#if !__UNISA_FTRIM_LIBC || __UN_system
static int system(const char *__u_cmd) {
    char **__u_env; char *__u_argv[4]; long __u_pid; long __u_me; int __u_st;
    if (__u_cmd == 0) return 1;
    __u_argv[0] = "sh"; __u_argv[1] = "-c"; __u_argv[2] = (char *)__u_cmd; __u_argv[3] = 0;
    if (!_unisa_env) _unisa_environ_init();   /* the child gets environ as it stands now */
    __u_env = _unisa_env;
#ifdef __APPLE__
#ifdef __x86_64__
    __u_me = __syscall6(0x2000000L + 20, 0, 0, 0, 0, 0);
    __u_pid = __syscall6(0x2000000L + 2, 0, 0, 0, 0, 0);
    if (__u_pid >= 0 && __syscall6(0x2000000L + 20, 0, 0, 0, 0, 0) != __u_me) __u_pid = 0;
#else
    __u_me = __syscall6(20, 0, 0, 0, 0, 0);
    __u_pid = __syscall6(2, 0, 0, 0, 0, 0);
    if (__u_pid >= 0 && __syscall6(20, 0, 0, 0, 0, 0) != __u_me) __u_pid = 0;
#endif
#elif defined(__x86_64__)
    __u_me = 0; __u_pid = __syscall6(57, 0, 0, 0, 0, 0);
#else
    __u_me = 0; __u_pid = __syscall6(220, 17, 0, 0, 0, 0);
#endif
    if (__u_pid < 0) return -1;
    if (__u_pid == 0) {
#if defined(__APPLE__) && defined(__x86_64__)
        __syscall6(0x2000000L + 59, (long)"/bin/sh", (long)__u_argv, (long)__u_env, 0, 0);
#elif defined(__APPLE__) || defined(__x86_64__)
        __syscall6(59, (long)"/bin/sh", (long)__u_argv, (long)__u_env, 0, 0);
#else
        __syscall6(221, (long)"/bin/sh", (long)__u_argv, (long)__u_env, 0, 0);
#endif
        __exit(127);
    }
    __u_st = 0;
#if defined(__APPLE__) && defined(__x86_64__)
    if (__syscall6(0x2000000L + 7, __u_pid, (long)&__u_st, 0, 0, 0) < 0) return -1;
#elif defined(__APPLE__)
    if (__syscall6(7, __u_pid, (long)&__u_st, 0, 0, 0) < 0) return -1;
#elif defined(__x86_64__)
    if (__syscall6(61, __u_pid, (long)&__u_st, 0, 0, 0) < 0) return -1;
#else
    if (__syscall6(260, __u_pid, (long)&__u_st, 0, 0, 0) < 0) return -1;
#endif
    return __u_st;
}
#endif
#endif
/* mbtowc/wctomb/mbstowcs/wcstombs (C99 7.20.7, 7.20.8) in the C locale,
 * where the multibyte encoding is one byte per character: a byte below 128
 * is the character with the same value, a byte of 128 or more is an invalid
 * multibyte character (EILSEQ), and no shift state exists.  Return values
 * and errno follow the host's C-locale behaviour exactly; a wide character
 * outside 0..127 has no multibyte form.  No function touches errno on
 * success.  mbtowc with n == 0: the empty string still converts to 0, any
 * other string is "incomplete" and gives -1 without errno. */
#if !__UNISA_FTRIM_LIBC || __UN_mbtowc
static int mbtowc(wchar_t *__u_pwc, const char *__u_s, size_t __u_n) {
    unsigned char __u_c;
    if (__u_s == 0) return 0;
    if (__u_s[0] == 0) { if (__u_pwc) *__u_pwc = 0; return 0; }
    if (__u_n == 0) return -1;
    __u_c = (unsigned char)__u_s[0];
    if (__u_c >= 128) { errno = EILSEQ; return -1; }
    if (__u_pwc) *__u_pwc = (wchar_t)__u_c;
    return 1;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_wctomb
static int wctomb(char *__u_s, wchar_t __u_wc) {
    if (__u_s == 0) return 0;
    if (__u_wc < 0 || __u_wc > 127) { errno = EILSEQ; return -1; }
    __u_s[0] = (char)__u_wc;
    return 1;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_mbstowcs
static size_t mbstowcs(wchar_t *__u_dst, const char *__u_src, size_t __u_n) {
    size_t __u_i; unsigned char __u_c;
    __u_i = 0;
    for (;;) {
        if (__u_dst && __u_i >= __u_n) return __u_i;
        __u_c = (unsigned char)__u_src[__u_i];
        if (__u_c >= 128) { errno = EILSEQ; return (size_t)-1; }
        if (__u_dst) __u_dst[__u_i] = (wchar_t)__u_c;
        if (__u_c == 0) return __u_i;
        __u_i = __u_i + 1;
    }
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_wcstombs
static size_t wcstombs(char *__u_dst, const wchar_t *__u_src, size_t __u_n) {
    size_t __u_i; wchar_t __u_wc;
    __u_i = 0;
    for (;;) {
        if (__u_dst && __u_i >= __u_n) return __u_i;
        __u_wc = __u_src[__u_i];
        if (__u_wc < 0 || __u_wc > 127) { errno = EILSEQ; return (size_t)-1; }
        if (__u_dst) __u_dst[__u_i] = (char)__u_wc;
        if (__u_wc == 0) return __u_i;
        __u_i = __u_i + 1;
    }
}
#endif
/* 0.0.34 L2 (SQLite's Apple path): forwarded to the system C library */
#ifdef __APPLE__
long random(void);
void srandomdev(void);
#endif
#endif

#if !__UNISA_FTRIM_LIBC || __UN_getline
static __u_word getline(char **__u_line, size_t *__u_cap, FILE *__u_f) {
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
    return (__u_word)__u_n;
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
    return (FILE *)(__u_word)__u_fd;
}
#endif
/* fileno (POSIX), 0.0.43: fdopen's inverse -- the descriptor the FILE * stands for, 0..2 for
   the standard streams; a null stream is EBADF (glibc would fault) */
#if !__UNISA_FTRIM_LIBC || __UN_fileno
static int fileno(FILE *__u_f) {
    if (__u_f == NULL) { errno = 9; return -1; }   /* EBADF */
    return _unisa_fd(__u_f);
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
/* the child gets environ (<stdlib.h>'s _unisa_env, one object for the program, which a constructor
   there fills before main); a program with no unit including <stdlib.h> has none, and the child gets
   the block the kernel left after argv */
#if !__UNISA_FTRIM_LIBC || __UN_popen
char **_unisa_env;
#endif
#if !__UNISA_FTRIM_LIBC || __UN_popen
static FILE *popen(const char *__u_cmd, const char *__u_mode) {
    static char *__u_envv[512]; char **__u_env; char *__u_argv[4]; int __u_p[2]; long __u_pid; long __u_me; long __u_r;
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
    __u_env = _unisa_env;
    if (!__u_env) {
        __u_n = 0; __u_k = __argc() + 1;
        while (__u_n < 511 && __argv(__u_k) != 0) { __u_envv[__u_n] = __argv(__u_k); __u_n = __u_n + 1; __u_k = __u_k + 1; }
        __u_envv[__u_n] = 0; __u_env = __u_envv;
    }
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



/* <stdlib.h> for the unisa C subset.  The allocator takes its memory from the
 * OS through the __mmap/__munmap gates and reuses what is freed; nothing here
 * needs a linker. */
#ifndef _UNISA_STDLIB_H
#define _UNISA_STDLIB_H



/* <limits.h> for the unisa C subset.
 *
 * These are the LIMITS OF THE TYPES, which is what a program asks for.  They
 * are not a description of how this compiler evaluates arithmetic: [G-2] says
 * `int` expressions are evaluated at 64 bits, so `INT_MAX + 1` does not wrap
 * here the way it does elsewhere.  That is a documented deviation of the
 * subset, and it does not change what INT_MAX is. */
#ifndef _UNISA_LIMITS_H
#define _UNISA_LIMITS_H

#define CHAR_BIT    8
/* PATH_MAX (POSIX <limits.h>, 0.0.37 C3): the host's own value */
#ifdef _WIN32
#define PATH_MAX    260
#elif defined(__APPLE__)
#define PATH_MAX    1024
#else
#define PATH_MAX    4096
#endif
#define SCHAR_MIN   (-128)
#define SCHAR_MAX   127
#define UCHAR_MAX   255
#define CHAR_MIN    SCHAR_MIN       /* plain char is signed here */
#define CHAR_MAX    SCHAR_MAX
#define MB_LEN_MAX  1

#define SHRT_MIN    (-32768)
#define SHRT_MAX    32767
#define USHRT_MAX   65535

#define INT_MIN     (-2147483647 - 1)
#define INT_MAX     2147483647
/* U: C99 5.2.4.2.1 gives UINT_MAX the type unsigned int.  Without the
   suffix the literal does not fit int and becomes long, so `UINT_MAX + 1`
   was 4294967296 where gcc wraps to 0 (fb12-08b). */
#define UINT_MAX    4294967295U

#if defined(_WIN64)
/* LLP64 Windows: long is 4 bytes, long long is the 8-byte type. */
#define LONG_MIN    (-2147483647L - 1)
#define LONG_MAX    2147483647L
#define ULONG_MAX   4294967295UL

#define LLONG_MIN   (-9223372036854775807LL - 1)
#define LLONG_MAX   9223372036854775807LL
#define ULLONG_MAX  18446744073709551615ULL
#else
#define LONG_MIN    (-9223372036854775807 - 1)
#define LONG_MAX    9223372036854775807
/* The UL suffix matters: without it this literal does not fit any signed
   type, and the reference refuses to fold an expression containing it
   (fb12-08, fb12-08b).  gcc's own <limits.h> writes it the same way.  The
   value is unchanged -- measured equal to gcc before and after. */
#define ULONG_MAX   18446744073709551615UL

#define LLONG_MIN   LONG_MIN
#define LLONG_MAX   LONG_MAX
#define ULLONG_MAX  ULONG_MAX
#endif

#endif


#define NULL 0
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define RAND_MAX 32767

/* The allocator.  Memory comes from the OS in 1 MB chunks (mmap, or
 * VirtualAlloc on Windows); a block is a 16-byte header -- its size, then its
 * class -- and the payload, so every pointer handed out is 16-aligned.
 *   small: header + request <= 64 KB is rounded up to a power of two, 32 ..
 *          65536 (classes 0 .. 11).  A freed block goes on its class's list
 *          (the link lives in the payload) and the next request of that class
 *          takes it back.  Nothing is split or coalesced, so nothing can be
 *          corrupted by it: a block is always exactly its class's size.
 *   large: anything bigger is mapped on its own, rounded to 64 KB, class -1,
 *          and `free` returns it to the OS. */
#ifdef _WIN32
#define _UNISA_MAP(__u_n) __mmap(0, (__u_n), 0x3000, 4, 0, 0)
#else
#ifdef __linux__
#define _UNISA_MAP(__u_n) __mmap(0, (__u_n), 3, 0x22, 0 - 1, 0)
#else
#define _UNISA_MAP(__u_n) __mmap(0, (__u_n), 3, 0x1002, 0 - 1, 0)
#endif
#endif
#define _UNISA_CHUNK 1048576
static char *_unisa_freel[12];
static char *_unisa_cur = 0;
static __u_word _unisa_left = 0;

#if !__UNISA_FTRIM_LIBC || __UN__unisa_map
static char *_unisa_map(__u_word __u_n) {
    __u_word __u_r;
    __u_r = (__u_word)_UNISA_MAP(__u_n);
    if (__u_r == 0 - 1 || __u_r == 0) return NULL;
    return (char *)__u_r;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_malloc
static void *malloc(__u_word __u_n) {
    __u_word __u_sz; int __u_k; char *__u_p;
    if (__u_n < 0 || __u_n > 0x100000000000) return NULL;
    __u_sz = __u_n + 16;
    if (__u_sz > 65536) {
        __u_sz = (__u_sz + 65535) & ~65535;
        __u_p = _unisa_map(__u_sz);
        if (__u_p == NULL) return NULL;
        *(__u_word *)__u_p = __u_sz;
        *(__u_word *)(__u_p + 8) = 0 - 1;
        return __u_p + 16;
    }
    __u_k = 0;
    while ((32 << __u_k) < __u_sz) __u_k = __u_k + 1;
    __u_sz = 32 << __u_k;
    __u_p = _unisa_freel[__u_k];
    if (__u_p != NULL) {
        _unisa_freel[__u_k] = *(char **)(__u_p + 16);
        return __u_p + 16;
    }
    if (_unisa_left < __u_sz) {
        __u_p = _unisa_map(_UNISA_CHUNK);
        if (__u_p == NULL) return NULL;
        _unisa_cur = __u_p;
        _unisa_left = _UNISA_CHUNK;
    }
    __u_p = _unisa_cur;
    _unisa_cur = _unisa_cur + __u_sz;
    _unisa_left = _unisa_left - __u_sz;
    *(__u_word *)__u_p = __u_sz;
    *(__u_word *)(__u_p + 8) = __u_k;
    return __u_p + 16;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_free
static void free(void *__u_q) {
    char *__u_p; __u_word __u_k;
    if (__u_q == NULL) return;
    __u_p = (char *)__u_q - 16;
    __u_k = *(__u_word *)(__u_p + 8);
    if (__u_k < 0) { __munmap(__u_p, *(__u_word *)__u_p); return; }
    *(char **)(__u_p + 16) = _unisa_freel[__u_k];
    _unisa_freel[__u_k] = __u_p;
}
#endif

/* `exit` is the one libc function that cannot be written in C: it must not
 * return.  `__exit` is the tape's gate to the OS, so this is a one-line
 * wrapper around it -- and `abort` is the same call with the status a shell
 * reports for SIGABRT. */

/* sys/_exit.h -- 0.0.33 D3': exit, the atexit table and the stdio flush hook, shared by
 * <stdlib.h> and <stdio.h>.  <stdio.h> needs exit in the program so that returning from main
 * goes through it and flushes buffered streams, but not the rest of <stdlib.h> (its environ
 * constructor and every other name). */
#ifndef _UNISA_SYS_EXIT_H
#define _UNISA_SYS_EXIT_H

static void exit();   /* 0.0.35 H1: static like its definition below (C99 6.2.2p7); before the definition, as <stdlib.h> always had it (0.0.33 D3': after it, the redeclaration typed exit int and E3 refused assert's (printf(...), exit(1))) */
static void (*_unisa_atexit[32])(void);
static int _unisa_natexit = 0;
/* the stdio stream table lives here so exit can flush it by a direct call:
   a stored function address is a callback on Windows forward programs */
#define _U_NST 8
static int _u_st_fd[_U_NST];
static int _u_st_eof[_U_NST];
static int _u_st_err[_U_NST];
static int _u_st_ung[_U_NST];              /* a pushed-back byte, or -1 */
/* D3: a 4 KB read buffer per stream.  Bytes are taken from it before the
   descriptor is asked again; fseek/rewind drop it and ftell subtracts what is
   still unread, so the stream position stays the one C99 7.19.9 describes. */
#define _U_BUFSZ 4096
static char _u_st_buf[_U_NST * _U_BUFSZ];
static int _u_st_bpos[_U_NST];
static int _u_st_blen[_U_NST];
#if !__UNISA_FTRIM_LIBC || __UN__u_st_copy
static void _u_st_copy(char *__u_d, const char *__u_s, __u_word __u_n) {
    __u_word __u_j; __u_j = 0;
    while (__u_j < __u_n) { __u_d[__u_j] = __u_s[__u_j]; __u_j = __u_j + 1; }
}
#endif
static int _u_st_wlen[_U_NST];             /* D3': bytes waiting in the buffer to be written */
static int _u_st_ro[_U_NST];               /* opened "r" without "+": a write fails with EBADF [C99 7.19.5.3] */
static int _u_st_n;


/* D3': the write side of the buffer.  A stream's buffer holds either unread
   bytes or unwritten ones: a write after a read gives the unread part back to
   the descriptor (lseek), and a read, seek, tell, fflush, fclose or exit
   writes the waiting bytes first.  The loop in _u_st_raw stops on what the
   __write gate calls an error: negative on POSIX and the VM, 0 on Windows
   (WriteFile's count); an answer larger than asked is not trusted. */
#if !__UNISA_FTRIM_LIBC || __UN__u_st_raw
static __u_word _u_st_raw(int __u_fd, const char *__u_p, __u_word __u_n) {
    __u_word __u_done; __u_word __u_r;
    __u_done = 0;
    while (__u_done < __u_n) {
        __u_r = __write(__u_fd, (char *)__u_p + __u_done, __u_n - __u_done);
        /* POSIX gates return negative errno; a zero/oversized result is
           an I/O failure (also covers the Windows count-only gate). */
        if (__u_r <= 0 || __u_r > __u_n - __u_done) {
            errno = __u_r < 0 ? (int)(0 - __u_r) : EIO;
            return __u_done;
        }
        __u_done = __u_done + __u_r;
    }
    return __u_n;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__u_st_wflush
static int _u_st_wflush(int __u_i) {
    __u_word __u_k;
    if (__u_i < 0 || _u_st_wlen[__u_i] == 0) return 0;
    __u_k = _u_st_wlen[__u_i]; _u_st_wlen[__u_i] = 0;
    if (_u_st_raw(_u_st_fd[__u_i], _u_st_buf + __u_i * _U_BUFSZ, __u_k) != __u_k) { _u_st_err[__u_i] = 1; return 0 - 1; }
    return 0;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__u_st_flushall
static void _u_st_flushall(void) {
    int __u_i; __u_i = 0;
    while (__u_i < _u_st_n) { _u_st_wflush(__u_i); __u_i = __u_i + 1; }
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_exit
static void exit(int __u_code) {
    /* the atexit handlers, last registered first (C99 7.20.4.3p3) */
    while (_unisa_natexit > 0) {
        _unisa_natexit = _unisa_natexit - 1;
        _unisa_atexit[_unisa_natexit]();
    }
    _u_st_flushall();   /* streams last (C99 7.20.4.3p4) */
    __exit(__u_code);
}
#endif
#endif

#if !__UNISA_FTRIM_LIBC || __UN_abort
static void abort(void) { __exit(134); }
#endif

#if !__UNISA_FTRIM_LIBC || __UN_calloc
static void *calloc(__u_word __u_n, __u_word __u_sz) {
    __u_word *__u_p; __u_word __u_total; __u_word __u_i;
    if (__u_n < 0 || __u_sz < 0) return NULL;
    if (__u_sz != 0 && __u_n > 0x100000000000 / __u_sz) return NULL;
    __u_total = __u_n * __u_sz;
    __u_p = (__u_word *)malloc(__u_total);
    if (__u_p == NULL) return NULL;
    /* a payload is a multiple of 16 bytes, so whole longs stay inside it */
    __u_total = (__u_total + 7) / 8;
    __u_i = 0;
    while (__u_i < __u_total) { __u_p[__u_i] = 0; __u_i = __u_i + 1; }
    return __u_p;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_realloc
static void *realloc(void *__u_old, __u_word __u_n) {
    __u_word *__u_p; __u_word *__u_o; __u_word __u_i; __u_word __u_cap;
    if (__u_old == NULL) return malloc(__u_n);
    if (__u_n < 0) return NULL;
    __u_cap = *(__u_word *)((char *)__u_old - 16) - 16;
    if (__u_n <= __u_cap && (__u_cap <= 65536 || __u_n > __u_cap / 2)) return __u_old;
    __u_p = (__u_word *)malloc(__u_n);
    if (__u_p == NULL) return NULL;
    if (__u_n < __u_cap) __u_cap = __u_n;
    __u_cap = (__u_cap + 7) / 8;
    __u_o = (__u_word *)__u_old;
    __u_i = 0;
    while (__u_i < __u_cap) { __u_p[__u_i] = __u_o[__u_i]; __u_i = __u_i + 1; }
    free(__u_old);
    return __u_p;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_abs
static int abs(int __u_v) { if (__u_v < 0) return 0 - __u_v; return __u_v; }
#endif

#if !__UNISA_FTRIM_LIBC || __UN_atoi
static int atoi(const char *__u_s) {
    int __u_v; int __u_sign; __u_word __u_i;
    __u_v = 0; __u_sign = 1; __u_i = 0;
    while (__u_s[__u_i] == 32) __u_i = __u_i + 1;
    if (__u_s[__u_i] == 45) { __u_sign = 0 - 1; __u_i = __u_i + 1; }
    else { if (__u_s[__u_i] == 43) __u_i = __u_i + 1; }
    while (__u_s[__u_i] >= 48) { if (__u_s[__u_i] > 57) break; __u_v = __u_v * 10 + (__u_s[__u_i] - 48); __u_i = __u_i + 1; }
    return __u_v * __u_sign;
}
#endif

/* strtol/strtoul/strtod: the conversions C89 and C99 put here.  `end` is
 * written when it is not null, which is how callers tell "no digits" from
 * "the value happened to be zero". */
/* Accumulate two 32-bit limbs in nonnegative longs.  Their products by
 * base (<=36) fit signed long, including at ULONG_MAX; no signed wrap or
 * unsigned division is needed.  Overflow still consumes every valid digit. */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_strdigit
static int _unisa_strdigit(int __u_c) {
    if (__u_c >= 48 && __u_c <= 57) return __u_c - 48;
    if (__u_c >= 97 && __u_c <= 122) return __u_c - 97 + 10;
    if (__u_c >= 65 && __u_c <= 90) return __u_c - 65 + 10;
    return 0 - 1;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__unisa_strmag
static int _unisa_strmag(const char *__u_s, char **__u_end, int __u_base,
                       __u_word *__u_hi, __u_word *__u_lo, int *__u_neg) {
    __u_word __u_i; __u_word __u_start; __u_word __u_l; __u_word __u_h; int __u_d; int __u_over; int __u_auto;
    __u_auto = __u_base == 0; __u_i = 0; *__u_hi = 0; *__u_lo = 0; *__u_neg = 0; __u_over = 0;
    if (__u_end) *__u_end = (char *)__u_s;
    if (__u_base != 0 && (__u_base < 2 || __u_base > 36)) return 0;
    while (__u_s[__u_i] == 32 || (__u_s[__u_i] >= 9 && __u_s[__u_i] <= 13)) __u_i = __u_i + 1;
    if (__u_s[__u_i] == 45) { *__u_neg = 1; __u_i = __u_i + 1; }
    else { if (__u_s[__u_i] == 43) __u_i = __u_i + 1; }
    if (__u_base == 0) {
        __u_base = 10;
        if (__u_s[__u_i] == 48) __u_base = 8;
    }
    if (__u_base == 8 || __u_base == 16) {
        if (__u_s[__u_i] == 48 && (__u_s[__u_i+1] == 120 || __u_s[__u_i+1] == 88)) {
            __u_d = _unisa_strdigit(__u_s[__u_i+2]);
            if (__u_d >= 0 && __u_d < 16) {
                /* Only base 0 or 16 may accept a hex prefix. */
                if (__u_base == 16 || __u_auto) { __u_base = 16; __u_i = __u_i + 2; }
            }
        }
    }
    __u_start = __u_i;
    while (__u_s[__u_i]) {
        __u_d = _unisa_strdigit(__u_s[__u_i]);
        if (__u_d < 0 || __u_d >= __u_base) break;
        if (!__u_over) {
            __u_l = *__u_lo * __u_base + __u_d;
            __u_h = *__u_hi * __u_base + __u_l / 4294967296;
            if (__u_h > 4294967295) __u_over = 1;
            else { *__u_hi = __u_h; *__u_lo = __u_l % 4294967296; }
        }
        __u_i = __u_i + 1;
    }
    if (__u_end && __u_i != __u_start) *__u_end = (char *)(__u_s + __u_i);
    return __u_over;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_strtol
static long strtol(const char *__u_s, char **__u_end, int __u_base) {
#if defined(_WIN64)
    /* LLP64: long is 32 bits, so the range is [LONG_MIN, LONG_MAX], not the 64-bit one */
    __u_word __u_hi; __u_word __u_lo; __u_uword __u_m; int __u_neg; int __u_over;
    __u_over = _unisa_strmag(__u_s, __u_end, __u_base, &__u_hi, &__u_lo, &__u_neg);
    __u_m = ((__u_uword)__u_hi << 32) | (__u_uword)__u_lo;
    if (__u_over || (!__u_neg && __u_m > (__u_uword)LONG_MAX) ||
        (__u_neg && __u_m > (__u_uword)LONG_MAX + 1)) {
        errno = ERANGE;
        if (__u_neg) return LONG_MIN;
        return LONG_MAX;
    }
    if (__u_neg) return (long)(0 - (__u_word)__u_m);
    return (long)__u_m;
#else
    long __u_hi; long __u_lo; long __u_v; int __u_neg; int __u_over;
    __u_over = _unisa_strmag(__u_s, __u_end, __u_base, &__u_hi, &__u_lo, &__u_neg);
    if (__u_over || __u_hi > 2147483648 ||
        (__u_hi == 2147483648 && (__u_lo != 0 || !__u_neg))) {
        errno = ERANGE;
        if (__u_neg) return LONG_MIN;
        return LONG_MAX;
    }
    if (__u_hi == 2147483648) return LONG_MIN;
    __u_v = __u_hi * 4294967296 + __u_lo;
    if (__u_neg) return 0 - __u_v;
    return __u_v;
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_strtoul
static unsigned long strtoul(const char *__u_s, char **__u_end, int __u_base) {
#if defined(_WIN64)
    /* LLP64: unsigned long is 32 bits; a magnitude past ULONG_MAX is ERANGE, then '-' negates */
    __u_word __u_hi; __u_word __u_lo; unsigned long __u_v; int __u_neg; int __u_over;
    __u_over = _unisa_strmag(__u_s, __u_end, __u_base, &__u_hi, &__u_lo, &__u_neg);
    if (__u_over || __u_hi != 0) { errno = ERANGE; return (unsigned long)0 - 1; }
    __u_v = (unsigned long)__u_lo;
    if (__u_neg) return (unsigned long)0 - __u_v;
    return __u_v;
#else
    long __u_hi; long __u_lo; unsigned long __u_v; int __u_neg; int __u_over;
    __u_over = _unisa_strmag(__u_s, __u_end, __u_base, &__u_hi, &__u_lo, &__u_neg);
    if (__u_over) { errno = ERANGE; return (unsigned long)0 - 1; }
    __u_v = ((unsigned long)__u_hi << 32) | (unsigned long)__u_lo;
    if (__u_neg) return (unsigned long)0 - __u_v;
    return __u_v;
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_atol
static long atol(const char *__u_s) { return strtol(__u_s, 0, 10); }
#endif

/* This compiler uses 64-bit long and long long on all six targets.  Keep the
 * conversion, overflow clamp, ERANGE and end-pointer rules in one routine. */
#if !__UNISA_FTRIM_LIBC || __UN_strtoll
static long long strtoll(const char *__u_s, char **__u_end, int __u_base) {
#if defined(_WIN64)
    /* LLP64: strtol is 32-bit here, so the 64-bit range is converted and clamped directly */
    __u_word __u_hi; __u_word __u_lo; __u_word __u_v; int __u_neg; int __u_over;
    __u_over = _unisa_strmag(__u_s, __u_end, __u_base, &__u_hi, &__u_lo, &__u_neg);
    if (__u_over || __u_hi > 2147483648 ||
        (__u_hi == 2147483648 && (__u_lo != 0 || !__u_neg))) {
        errno = ERANGE;
        if (__u_neg) return LLONG_MIN;
        return LLONG_MAX;
    }
    if (__u_hi == 2147483648) return LLONG_MIN;
    __u_v = __u_hi * 4294967296 + __u_lo;
    if (__u_neg) return 0 - __u_v;
    return __u_v;
#else
    return strtol(__u_s, __u_end, __u_base);
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_strtoull
static unsigned long long strtoull(const char *__u_s, char **__u_end, int __u_base) {
#if defined(_WIN64)
    /* LLP64: strtoul is 32-bit here, so the 64-bit value is assembled directly */
    __u_word __u_hi; __u_word __u_lo; __u_uword __u_v; int __u_neg; int __u_over;
    __u_over = _unisa_strmag(__u_s, __u_end, __u_base, &__u_hi, &__u_lo, &__u_neg);
    if (__u_over) { errno = ERANGE; return (__u_uword)0 - 1; }
    __u_v = ((__u_uword)__u_hi << 32) | (__u_uword)__u_lo;
    if (__u_neg) return (__u_uword)0 - __u_v;
    return __u_v;
#else
    return strtoul(__u_s, __u_end, __u_base);
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_atoll
static long long atoll(const char *__u_s) { return strtoll(__u_s, 0, 10); }
#endif

/* The fractional part is accumulated as an integer and scaled once, so a
 * long run of digits does not lose the low ones to repeated division. */
/* mkstemp (POSIX, 0.0.34 L1b): replace the trailing XXXXXX and create the file
   exclusively, retrying on EEXIST; the letters come from a counter mixed with the
   address of a local, so two processes running the same binary still differ. */
/* No <fcntl.h> here: an untrimmed tape (the Python front end, --fold) would carry
   every fcntl.h body, and those name host calls Windows lowering refuses.  The
   exclusive-create flags are spelled out instead (O_RDWR|O_CREAT|O_EXCL per OS). */
#ifdef _WIN32

/* sys/_win.h -- 0.0.21 Windows POSIX layer, batch 1: the shared helpers that
 * let <unistd.h>, <fcntl.h>, <sys/stat.h> and <time.h> wrap POSIX names thinly
 * over kernel32 (no home-grown libc).  The host channel is the program's IAT:
 * __hostaddr0..3 = LoadLibraryA GetProcAddress FreeLibrary GetLastError, and
 * __hostcall(fn, __u_word[10]) makes the Win64/AAPCS64 call.  kernel32's handle is
 * looked up once; each caller caches its own procedure in a static.
 * On Windows an fd IS the HANDLE value (as the bundled stdio already has it).
 * Windows is LLP64 (long is 4 bytes): handles, procedures, pointers and the host
 * channel's 8-byte slots are __u_word (<sys/_word.h>), never long. */
#ifndef _UNISA_SYS_WIN_H
#define _UNISA_SYS_WIN_H
#if defined(_WIN32) && !defined(_UNISA_NO_HOSTCALL)   /* the compiler builds itself without the host channel until the product lowers it on Windows (0.0.22) */


#if !__UNISA_FTRIM_LIBC || __UN__ux_sym
static __u_word _ux_sym(const char *__u_name) {          /* GetProcAddress(kernel32, name) */
    static __u_word __u_k32;
    __u_word __u_a[10] = {0};
    __u_a[1] = 0; __u_a[2] = 0; __u_a[3] = 0; __u_a[4] = 0; __u_a[5] = 0;
    if (!__u_k32) { __u_a[0] = (__u_word)"kernel32.dll"; __u_k32 = __hostcall(__hostaddr0(), __u_a); }
    __u_a[0] = __u_k32; __u_a[1] = (__u_word)__u_name;
    return __hostcall(__hostaddr1(), __u_a);
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__ux_call
static __u_word _ux_call(__u_word __u_fn, __u_word __u_x0, __u_word __u_x1, __u_word __u_x2, __u_word __u_x3) {
    __u_word __u_a[10] = {0};
    __u_a[0] = __u_x0; __u_a[1] = __u_x1; __u_a[2] = __u_x2; __u_a[3] = __u_x3; __u_a[4] = 0; __u_a[5] = 0;
    return __hostcall(__u_fn, __u_a);
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__ux_errno
static int _ux_errno(void) {                         /* GetLastError -> errno */
    __u_word __u_a[10] = {0}; __u_word __u_e;
    __u_a[0] = 0; __u_a[1] = 0; __u_a[2] = 0; __u_a[3] = 0; __u_a[4] = 0; __u_a[5] = 0;
    __u_e = __hostcall(__hostaddr3(), __u_a) & 0xFFFFFFFFLL;
    if (__u_e == 2 || __u_e == 3) return ENOENT;
    if (__u_e == 5) return EACCES;
    if (__u_e == 6) return EBADF;
    if (__u_e == 80 || __u_e == 183) return EEXIST;
    if (__u_e == 145) return ENOTEMPTY;
    if (__u_e == 32) return EBUSY;
    if (__u_e == 87) return EINVAL;
    return EIO;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__ux_fail
static int _ux_fail(void) { errno = _ux_errno(); return -1; }
#endif
/* The names above are kept only when a baked library key lists them. A disk
   body that started calling them later is compiled, and the callee is then
   missing. These macros expand in the caller and use the host channel
   directly, same mapping as _ux_sym / _ux_call / _ux_errno. */
#define _ux_load(dst, name) do { \
    static __u_word __uxk; __u_word __uxa[10]; int __uxi; \
    __uxi = 0; while (__uxi < 10) { __uxa[__uxi] = 0; __uxi = __uxi + 1; } \
    if (!__uxk) { __uxa[0] = (__u_word)"kernel32.dll"; __uxk = __hostcall(__hostaddr0(), __uxa); } \
    __uxa[0] = __uxk; __uxa[1] = (__u_word)(name); (dst) = __hostcall(__hostaddr1(), __uxa); \
} while (0)
#define _ux_go(dst, fn, x0, x1, x2, x3) do { \
    __u_word __uxa[10]; int __uxi; \
    __uxi = 0; while (__uxi < 10) { __uxa[__uxi] = 0; __uxi = __uxi + 1; } \
    __uxa[0] = (__u_word)(x0); __uxa[1] = (__u_word)(x1); __uxa[2] = (__u_word)(x2); __uxa[3] = (__u_word)(x3); \
    (dst) = __hostcall((fn), __uxa); \
} while (0)
#define _ux_err(dst) do { \
    __u_word __uxa[10]; __u_word __uxe; int __uxi; \
    __uxi = 0; while (__uxi < 10) { __uxa[__uxi] = 0; __uxi = __uxi + 1; } \
    __uxe = __hostcall(__hostaddr3(), __uxa) & 0xFFFFFFFFLL; \
    if (__uxe == 2 || __uxe == 3) (dst) = ENOENT; \
    else if (__uxe == 5) (dst) = EACCES; \
    else if (__uxe == 6) (dst) = EBADF; \
    else if (__uxe == 80 || __uxe == 183) (dst) = EEXIST; \
    else if (__uxe == 145) (dst) = ENOTEMPTY; \
    else if (__uxe == 32) (dst) = EBUSY; \
    else if (__uxe == 87) (dst) = EINVAL; \
    else (dst) = EIO; \
} while (0)
#if !__UNISA_FTRIM_LIBC || __UN__ux_ft2unix
static __u_word _ux_ft2unix(const unsigned int *__u_ft) {   /* FILETIME (100 ns since 1601) -> unix seconds */
    __u_word __u_t; __u_t = ((__u_word)__u_ft[1] << 32) | (__u_word)__u_ft[0];
    return (__u_t - 116444736000000000LL) / 10000000LL;
}
#endif
#endif
#endif

#endif
#if !__UNISA_FTRIM_LIBC || __UN_mkstemp
static int mkstemp(char *__u_t) {
    static __u_uword __u_seq;
    __u_word __u_n; __u_word __u_j; int __u_try; __u_word __u_r; __u_uword __u_v;
    __u_n = 0; while (__u_t[__u_n]) __u_n = __u_n + 1;
    if (__u_n < 6) { errno = EINVAL; return 0 - 1; }
    __u_j = __u_n - 6; while (__u_j < __u_n) { if (__u_t[__u_j] != 88) { errno = EINVAL; return 0 - 1; } __u_j = __u_j + 1; }
    __u_try = 0;
    while (__u_try < 100) {
        __u_seq = __u_seq + 1;
        __u_v = ((__u_uword)&__u_try >> 4) * 2654435761u + __u_seq * 40503u;
        __u_j = __u_n - 6;
        while (__u_j < __u_n) { __u_t[__u_j] = (char)(97 + __u_v % 26); __u_v = __u_v / 26 + __u_seq * 7; __u_j = __u_j + 1; }
#ifdef _WIN32
        /* GENERIC_READ|GENERIC_WRITE, CREATE_NEW. The gate returns -1; errno comes from GetLastError. */
        __u_r = __open(__u_t, 0xC0000000L, 1);
        if (__u_r >= 0) return (int)__u_r;
        { int __u_e; _ux_err(__u_e); errno = __u_e; }
#else
#ifdef __APPLE__
        __u_r = __open(__u_t, 0xA02, 384);
#else
        __u_r = __open(__u_t, 0302, 384);
#endif
        if (__u_r >= 0) return (int)__u_r;
        errno = (int)(0 - __u_r);
#endif
        if (errno != EEXIST) return 0 - 1;
        __u_try = __u_try + 1;
    }
    return 0 - 1;
}
#endif
/* realpath (POSIX, 0.0.37 C3): open the path and ask the kernel for the name it resolved
   (Apple F_GETPATH; Linux readlink of /proc/self/fd/N), so symlinks, "." and ".." are the
   kernel's answer, not a lexical guess.  The path must exist, as POSIX requires; a file
   the caller cannot open fails with the open's errno.  NULL resolved: malloc(PATH_MAX).
   Windows (2026-10-26, after Cosmopolitan): open the path and ask GetFinalPathNameByHandleA
   (VOLUME_NAME_DOS), then drop a leading \\?\ or turn \\?\UNC\ into \\. The __open gate
   cannot pass FILE_FLAG_BACKUP_SEMANTICS, so a directory does not open; that case, and any
   final-name failure, falls back to GetFileAttributesA plus GetFullPathNameA. The fallback
   is absolute and does not resolve a symlink. This slice has not been executed on Windows. */
#ifndef __UNISA_PYFRONT
#if !__UNISA_FTRIM_LIBC || __UN_realpath
static char *realpath(const char *__u_p, char *__u_out) {
    char __u_tmp[PATH_MAX]; __u_word __u_fd; __u_word __u_r; __u_word __u_n; __u_word __u_k; char *__u_d;
#ifdef _WIN32
    static __u_word __u_attr, __u_full, __u_fin; __u_word __u_a; __u_word __u_h; __u_word __u_off; int __u_e;
    if (!__u_attr) {
        _ux_load(__u_attr, "GetFileAttributesA");
        _ux_load(__u_full, "GetFullPathNameA");
        _ux_load(__u_fin, "GetFinalPathNameByHandleA");
    }
    __u_n = 0;
    __u_h = __open((char *)__u_p, 0x80000000L, 3);          /* GENERIC_READ, OPEN_EXISTING */
    if (__u_h >= 0) {
        /* Length excludes the NUL. Zero, or a size that does not fit, is not a path. */
        _ux_go(__u_n, __u_fin, __u_h, (__u_word)__u_tmp, PATH_MAX, 0);
        __u_n = __u_n & 0xFFFFFFFFL;
        __close(__u_h);
        if (__u_n == 0 || __u_n >= PATH_MAX) __u_n = 0;
    }
    if (__u_n > 0 && __u_n >= 4 && __u_tmp[0] == 92 && __u_tmp[1] == 92 && __u_tmp[2] == 63 && __u_tmp[3] == 92) {
        __u_off = 4;
        if (__u_n >= 8 && (__u_tmp[4] == 85 || __u_tmp[4] == 117) && (__u_tmp[5] == 78 || __u_tmp[5] == 110) &&
            (__u_tmp[6] == 67 || __u_tmp[6] == 99) && __u_tmp[7] == 92) {
            __u_tmp[0] = 92; __u_tmp[1] = 92; __u_off = 8; __u_k = 2;
        } else __u_k = 0;
        __u_a = __u_off;
        while (__u_a <= __u_n) { __u_tmp[__u_k] = __u_tmp[__u_a]; __u_k = __u_k + 1; __u_a = __u_a + 1; }
        __u_n = __u_k - 1;
    }
    if (__u_n == 0) {
        _ux_go(__u_a, __u_attr, (__u_word)__u_p, 0, 0, 0);
        __u_a = __u_a & 0xFFFFFFFFL;
        if (__u_a == 0xFFFFFFFFL) { _ux_err(__u_e); errno = __u_e; return 0; }
        _ux_go(__u_n, __u_full, (__u_word)__u_p, PATH_MAX, (__u_word)__u_tmp, 0);
        __u_n = __u_n & 0xFFFFFFFFL;
        if (__u_n == 0) { _ux_err(__u_e); errno = __u_e; return 0; }
        if (__u_n >= PATH_MAX) { errno = ENAMETOOLONG; return 0; }
    }
    __u_d = __u_out;
    if (!__u_d) { __u_d = (char *)malloc(PATH_MAX); if (!__u_d) { errno = ENOMEM; return 0; } }
    __u_k = 0; while (__u_k <= __u_n) { __u_d[__u_k] = __u_tmp[__u_k]; __u_k = __u_k + 1; }
    return __u_d;
#else
#ifdef __APPLE__
    __u_fd = __open((char *)__u_p, 0, 0);
    if (__u_fd < 0) { errno = (int)(0 - __u_fd); return 0; }
    __u_r = __fcntl(__u_fd, 50, (long)__u_tmp);   /* F_GETPATH */
    __close(__u_fd);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return 0; }
    __u_n = 0; while (__u_tmp[__u_n]) __u_n = __u_n + 1;
#else
    char __u_fdp[32]; long __u_v; char __u_dig[24];
    __u_fd = __open((char *)__u_p, 010000000, 0);   /* O_PATH: no read permission needed */
    if (__u_fd < 0) { errno = (int)(0 - __u_fd); return 0; }
    __u_k = 0; while ("/proc/self/fd/"[__u_k]) { __u_fdp[__u_k] = "/proc/self/fd/"[__u_k]; __u_k = __u_k + 1; }
    __u_v = __u_fd; __u_n = 0;
    do { __u_dig[__u_n] = (char)(48 + __u_v % 10); __u_v = __u_v / 10; __u_n = __u_n + 1; } while (__u_v);
    while (__u_n) { __u_n = __u_n - 1; __u_fdp[__u_k] = __u_dig[__u_n]; __u_k = __u_k + 1; }
    __u_fdp[__u_k] = 0;
#ifdef __x86_64__
    __u_r = __syscall6(89, (long)__u_fdp, (long)__u_tmp, PATH_MAX - 1, 0, 0);          /* readlink */
#else
    __u_r = __syscall6(78, -100, (long)__u_fdp, (long)__u_tmp, PATH_MAX - 1, 0);       /* readlinkat */
#endif
    __close(__u_fd);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return 0; }
    if (__u_r >= PATH_MAX - 1) { errno = ENAMETOOLONG; return 0; }
    __u_n = __u_r; __u_tmp[__u_n] = 0;
#endif
    __u_d = __u_out;
    if (!__u_d) { __u_d = (char *)malloc(PATH_MAX); if (!__u_d) { errno = ENOMEM; return 0; } }
    __u_k = 0; while (__u_k <= __u_n) { __u_d[__u_k] = __u_tmp[__u_k]; __u_k = __u_k + 1; }
    return __u_d;
#endif
}
#endif
#endif
/* mkdtemp (POSIX, 0.0.35 M1): mkstemp's names, but a directory, mode 0700.
   Windows (2026-10-26, after Cosmopolitan): CreateDirectoryA, mode ignored.
   The template must end in XXXXXX. EEXIST retries. Any other error, and a
   retry limit, put XXXXXX back and return NULL. __mkdir stays on the Unix
   branch: it has no Windows import, and the Python front end folds one tape. */
#ifndef __UNISA_PYFRONT
#if !__UNISA_FTRIM_LIBC || __UN_mkdtemp
static char *mkdtemp(char *__u_t) {
    static __u_uword __u_seq;
    __u_word __u_n; __u_word __u_j; int __u_try; __u_word __u_r; __u_uword __u_v;
    __u_n = 0; while (__u_t[__u_n]) __u_n = __u_n + 1;
    if (__u_n < 6) { errno = EINVAL; return 0; }
    __u_j = __u_n - 6; while (__u_j < __u_n) { if (__u_t[__u_j] != 88) { errno = EINVAL; return 0; } __u_j = __u_j + 1; }
    __u_try = 0;
    while (__u_try < 100) {
        __u_seq = __u_seq + 1;
        __u_v = ((__u_uword)&__u_try >> 4) * 2654435761u + __u_seq * 40503u + 17;
        __u_j = __u_n - 6;
        while (__u_j < __u_n) { __u_t[__u_j] = (char)(97 + __u_v % 26); __u_v = __u_v / 26 + __u_seq * 7; __u_j = __u_j + 1; }
#ifdef _WIN32
        {   static __u_word __u_cd; __u_word __u_ok; int __u_e;
            if (!__u_cd) _ux_load(__u_cd, "CreateDirectoryA");
            _ux_go(__u_ok, __u_cd, (__u_word)__u_t, 0, 0, 0);
            __u_ok = __u_ok & 0xFFFFFFFFL;
            if (__u_ok) return __u_t;
            _ux_err(__u_e); errno = __u_e;
            if (errno != EEXIST) {
                __u_j = __u_n - 6; while (__u_j < __u_n) { __u_t[__u_j] = 88; __u_j = __u_j + 1; }
                return 0;
            }
        }
#else
        __u_r = __mkdir(__u_t, 448, 0);
        if (__u_r >= 0) return __u_t;
        errno = (int)(0 - __u_r);
        if (errno != EEXIST) return 0;
#endif
        __u_try = __u_try + 1;
    }
#ifdef _WIN32
    __u_j = __u_n - 6; while (__u_j < __u_n) { __u_t[__u_j] = 88; __u_j = __u_j + 1; }
#endif
    return 0;
}
#endif
#endif
/* strtod's slow path (more than 19 digits, or a power of ten past 22) is a
 * guess a few ulp off; these compare the decimal with the halfway points around
 * it in big integers (base 2^32) and step to the correctly rounded double. */
#define _U_BL 200
#if !__UNISA_FTRIM_LIBC || __UN__u_bn_muls
static void _u_bn_muls(unsigned int *__u_a, int *__u_n, unsigned int __u_k) {
    __u_uword __u_c; int __u_i; __u_c = 0;
    for (__u_i = 0; __u_i < *__u_n; __u_i++) { __u_c = __u_c + (__u_uword)__u_a[__u_i] * __u_k; __u_a[__u_i] = (unsigned int)__u_c; __u_c = __u_c >> 32; }
    if (__u_c && *__u_n < _U_BL) { __u_a[*__u_n] = (unsigned int)__u_c; *__u_n = *__u_n + 1; }
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__u_bn_shl
static void _u_bn_shl(unsigned int *__u_a, int *__u_n, __u_word __u_b) {
    int __u_w; int __u_s; int __u_i;
    __u_w = (int)(__u_b >> 5); __u_s = (int)(__u_b & 31);
    if (*__u_n == 0) return;
    if (__u_s) _u_bn_muls(__u_a, __u_n, 1u << __u_s);
    if (__u_w) {
        if (*__u_n + __u_w > _U_BL) __u_w = _U_BL - *__u_n;
        for (__u_i = *__u_n - 1; __u_i >= 0; __u_i--) __u_a[__u_i + __u_w] = __u_a[__u_i];
        for (__u_i = 0; __u_i < __u_w; __u_i++) __u_a[__u_i] = 0;
        *__u_n = *__u_n + __u_w;
    }
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__u_bn_pow10
static void _u_bn_pow10(unsigned int *__u_a, int *__u_n, __u_word __u_x) {
    while (__u_x >= 9) { _u_bn_muls(__u_a, __u_n, 1000000000u); __u_x = __u_x - 9; }
    while (__u_x > 0) { _u_bn_muls(__u_a, __u_n, 10u); __u_x = __u_x - 1; }
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__u_bn_cmp
static int _u_bn_cmp(unsigned int *__u_a, int __u_na, unsigned int *__u_b, int __u_nb) {
    int __u_i;
    while (__u_na > 0 && __u_a[__u_na - 1] == 0) __u_na = __u_na - 1;
    while (__u_nb > 0 && __u_b[__u_nb - 1] == 0) __u_nb = __u_nb - 1;
    if (__u_na != __u_nb) return __u_na > __u_nb ? 1 : 0 - 1;
    for (__u_i = __u_na - 1; __u_i >= 0; __u_i--) if (__u_a[__u_i] != __u_b[__u_i]) return __u_a[__u_i] > __u_b[__u_i] ? 1 : 0 - 1;
    return 0;
}
#endif
/* sign of D*10^x10 - (2m+1)*2^(e-1), the halfway point above v; D = the digits
   s[0..len) skipping '.', at most 780 significant (the rest only make it bigger) */
#if !__UNISA_FTRIM_LIBC || __UN__u_sd_cmp
static int _u_sd_cmp(const char *__u_s, __u_word __u_len, __u_word __u_x10, double __u_v) {
    static unsigned int __u_d[_U_BL]; static unsigned int __u_h[_U_BL];
    int __u_nd; int __u_nh; int __u_sticky; int __u_c; __u_word __u_i; __u_word __u_cnt; __u_word __u_xx;
    __u_uword __u_bits; __u_uword __u_m; __u_word __u_e;
    __u_nd = 0; __u_sticky = 0; __u_cnt = 0; __u_xx = __u_x10;
    for (__u_i = 0; __u_i < __u_len; __u_i++) {
        int __u_ch; __u_uword __u_c2; int __u_j;
        __u_ch = __u_s[__u_i];
        if (__u_ch == 46) continue;
        if (__u_cnt >= 780) { if (__u_ch != 48) __u_sticky = 1; __u_xx = __u_xx + 1; continue; }
        if (__u_nd == 0 && __u_ch == 48) continue;
        if (__u_nd == 0) { __u_d[0] = 0; __u_nd = 1; }
        _u_bn_muls(__u_d, &__u_nd, 10u);
        __u_c2 = (__u_uword)(__u_ch - 48);
        for (__u_j = 0; __u_c2 && __u_j < __u_nd; __u_j++) { __u_c2 = __u_c2 + __u_d[__u_j]; __u_d[__u_j] = (unsigned int)__u_c2; __u_c2 = __u_c2 >> 32; }
        if (__u_c2 && __u_nd < _U_BL) { __u_d[__u_nd] = (unsigned int)__u_c2; __u_nd = __u_nd + 1; }
        __u_cnt = __u_cnt + 1;
    }
    if (__u_nd == 0) return 0 - 1;
    __u_bits = *(__u_uword *)&__u_v;
    __u_e = (__u_word)((__u_bits >> 52) & 2047); __u_m = __u_bits & 4503599627370495;
    if (__u_e == 0) __u_e = 1; else __u_m = __u_m | 4503599627370496;
    __u_e = __u_e - 1075;
    __u_m = 2 * __u_m + 1;
    __u_nh = 2; __u_h[0] = (unsigned int)__u_m; __u_h[1] = (unsigned int)(__u_m >> 32);
    if (__u_xx > 0) _u_bn_pow10(__u_d, &__u_nd, __u_xx); else _u_bn_pow10(__u_h, &__u_nh, 0 - __u_xx);
    if (__u_e - 1 > 0) _u_bn_shl(__u_h, &__u_nh, __u_e - 1); else _u_bn_shl(__u_d, &__u_nd, 1 - __u_e);
    __u_c = _u_bn_cmp(__u_d, __u_nd, __u_h, __u_nh);
    if (__u_c == 0 && __u_sticky) __u_c = 1;
    return __u_c;
}
#endif
/* walk the guess (finite, >= 0, a few ulp off) to the correctly rounded double */
#if !__UNISA_FTRIM_LIBC || __UN__u_sd_fix
static double _u_sd_fix(const char *__u_s, __u_word __u_len, __u_word __u_x10, double __u_v) {
    __u_uword __u_b; __u_uword __u_lb; int __u_c; int __u_k;
    for (__u_k = 0; __u_k < 64; __u_k++) {
        __u_b = *(__u_uword *)&__u_v;
        if (__u_b >= 9218868437227405312) return __u_v;              /* inf or nan */
        __u_c = _u_sd_cmp(__u_s, __u_len, __u_x10, __u_v);
        if (__u_c > 0 || (__u_c == 0 && (__u_b & 1))) { __u_b = __u_b + 1; __u_v = *(double *)&__u_b; continue; }
        if (__u_b == 0) return __u_v;
        __u_lb = __u_b - 1;
        __u_c = _u_sd_cmp(__u_s, __u_len, __u_x10, *(double *)&__u_lb);
        if (__u_c < 0 || (__u_c == 0 && !(__u_lb & 1))) { __u_v = *(double *)&__u_lb; continue; }
        return __u_v;
    }
    return __u_v;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_strtod
static double strtod(const char *__u_s, char **__u_end) {
    double __u_v; int __u_sign; __u_word __u_i; int __u_any;
    __u_word __u_e; int __u_esign;
    __u_v = 0.0; __u_sign = 1; __u_i = 0; __u_any = 0;
    while (__u_s[__u_i] == 32 || (__u_s[__u_i] >= 9 && __u_s[__u_i] <= 13)) __u_i = __u_i + 1;
    if (__u_s[__u_i] == 45) { __u_sign = 0 - 1; __u_i = __u_i + 1; }
    else { if (__u_s[__u_i] == 43) __u_i = __u_i + 1; }
    /* C99 hexadecimal form: 0x hex-digits [. hex-digits] [p [+-] decimal] */
    if (__u_s[__u_i] == 48 && (__u_s[__u_i + 1] | 32) == 120) {
        __u_word __u_k; int __u_d; __u_word __u_be; int __u_hany;
        __u_k = __u_i + 2; __u_hany = 0; __u_be = 0;
        while (1) {
            __u_d = __u_s[__u_k];
            if (__u_d >= 48 && __u_d <= 57) __u_d = __u_d - 48;
            else { if ((__u_d | 32) >= 97 && (__u_d | 32) <= 102) __u_d = (__u_d | 32) - 87; else break; }
            if (__u_v < 72057594037927936.0) __u_v = __u_v * 16.0 + (double)__u_d; else __u_be = __u_be + 4;
            __u_k = __u_k + 1; __u_hany = 1;
        }
        if (__u_s[__u_k] == 46) {
            __u_k = __u_k + 1;
            while (1) {
                __u_d = __u_s[__u_k];
                if (__u_d >= 48 && __u_d <= 57) __u_d = __u_d - 48;
                else { if ((__u_d | 32) >= 97 && (__u_d | 32) <= 102) __u_d = (__u_d | 32) - 87; else break; }
                if (__u_v < 72057594037927936.0) { __u_v = __u_v * 16.0 + (double)__u_d; __u_be = __u_be - 4; }
                __u_k = __u_k + 1; __u_hany = 1;
            }
        }
        if (__u_hany == 0) { if (__u_end) *__u_end = (char *)(__u_s + __u_i + 1); return 0.0 * (double)__u_sign; }
        if ((__u_s[__u_k] | 32) == 112) {
            __u_word __u_j; __u_j = __u_k + 1; __u_esign = 1;
            if (__u_s[__u_j] == 45) { __u_esign = 0 - 1; __u_j = __u_j + 1; }
            else { if (__u_s[__u_j] == 43) __u_j = __u_j + 1; }
            if (__u_s[__u_j] >= 48 && __u_s[__u_j] <= 57) {
                __u_e = 0;
                while (__u_s[__u_j] >= 48 && __u_s[__u_j] <= 57) { if (__u_e < 100000) __u_e = __u_e * 10 + (__u_s[__u_j] - 48); __u_j = __u_j + 1; }
                __u_k = __u_j; __u_be = __u_be + __u_esign * __u_e;
            }
        }
        if (__u_v != 0.0) {
            while (__u_be > 0) { __u_v = __u_v * 2.0; __u_be = __u_be - 1; }
            while (__u_be < 0) { __u_v = __u_v / 2.0; __u_be = __u_be + 1; if (__u_v == 0.0) break; }
        }
        if (__u_end) *__u_end = (char *)(__u_s + __u_k);
        return __u_v * (double)__u_sign;
    }
    /* Up to 19 significant digits are kept exactly in an integer and scaled
     * once by a power of ten; with at most 2^53 and |power| <= 22 that is a
     * single rounding (Clinger's fast path), so 2^63 and 10^i come out exact. */
    {   __u_uword __u_m; __u_word __u_x10; double __u_p; __u_word __u_q;
        __u_word __u_ds; __u_word __u_de; __u_word __u_fd; __u_word __u_ex; int __u_drop;
        __u_m = 0; __u_x10 = 0; __u_ds = __u_i; __u_fd = 0; __u_ex = 0; __u_drop = 0;
        while (__u_s[__u_i] >= 48 && __u_s[__u_i] <= 57) {
            if (__u_m < 1844674407370955161) __u_m = __u_m * 10 + (__u_uword)(__u_s[__u_i] - 48); else { __u_x10 = __u_x10 + 1; __u_drop = 1; }
            __u_i = __u_i + 1; __u_any = 1;
        }
        if (__u_s[__u_i] == 46) {
            __u_i = __u_i + 1;
            while (__u_s[__u_i] >= 48 && __u_s[__u_i] <= 57) {
                if (__u_m < 1844674407370955161) { __u_m = __u_m * 10 + (__u_uword)(__u_s[__u_i] - 48); __u_x10 = __u_x10 - 1; } else __u_drop = 1;
                __u_i = __u_i + 1; __u_any = 1; __u_fd = __u_fd + 1;
            }
        }
        __u_de = __u_i;
        if (__u_any) { if (__u_s[__u_i] == 101 || __u_s[__u_i] == 69) {
            __u_word __u_j; __u_j = __u_i + 1; __u_esign = 1;
            if (__u_s[__u_j] == 45) { __u_esign = 0 - 1; __u_j = __u_j + 1; }
            else { if (__u_s[__u_j] == 43) __u_j = __u_j + 1; }
            if (__u_s[__u_j] >= 48 && __u_s[__u_j] <= 57) {
                __u_e = 0;
                while (__u_s[__u_j] >= 48 && __u_s[__u_j] <= 57) { if (__u_e < 100000) __u_e = __u_e * 10 + (__u_s[__u_j] - 48); __u_j = __u_j + 1; }
                __u_i = __u_j; __u_x10 = __u_x10 + __u_esign * __u_e; __u_ex = __u_esign * __u_e;
            }
        } }
        while (__u_m != 0 && __u_x10 < 0 && __u_m % 10 == 0) { __u_m = __u_m / 10; __u_x10 = __u_x10 + 1; }
        __u_drop = __u_drop || __u_m > 9007199254740992 || __u_x10 > 22 || __u_x10 < 0 - 22;
        if (__u_m >> 63) __u_v = (double)(__u_word)(__u_m >> 1) * 2.0 + (double)(__u_word)(__u_m & 1); else __u_v = (double)(__u_word)__u_m;
        /* decimal magnitude of the leading digit: past 10^309 the value overflows, below 10^-325
           it underflows to zero -- decided here, because the big-integer walk below only moves a
           few ulp and its integers do not reach such exponents */
        if (__u_m != 0) {
            __u_uword __u_t; __u_word __u_mag; __u_t = __u_m; __u_mag = __u_x10 - 1;
            while (__u_t) { __u_t = __u_t / 10; __u_mag = __u_mag + 1; }
            if (__u_mag > 309) { __u_m = 0; __u_v = 1.0e300 * 1.0e300; __u_drop = 0; }
            else { if (__u_mag < 0 - 325) { __u_m = 0; __u_v = 0.0; __u_drop = 0; errno = ERANGE; } }
        }
        if (__u_m != 0) {
            while (__u_x10 > 22) { __u_v = __u_v * 1.0e22; __u_x10 = __u_x10 - 22; }
            while (__u_x10 < 0 - 22) { __u_v = __u_v / 1.0e22; __u_x10 = __u_x10 + 22; }
            __u_p = 1.0; __u_q = __u_x10 < 0 ? 0 - __u_x10 : __u_x10;
            while (__u_q > 0) { __u_p = __u_p * 10.0; __u_q = __u_q - 1; }
            if (__u_x10 > 0) __u_v = __u_v * __u_p; else { if (__u_x10 < 0) __u_v = __u_v / __u_p; }
        }
        if (__u_m != 0 && __u_drop) {
            if (__u_v > 1.7976931348623157e308) __u_v = 1.7976931348623157e308;
            __u_v = _u_sd_fix(__u_s + __u_ds, __u_de - __u_ds, __u_ex - __u_fd, __u_v);
        }
        /* C99 7.20.1.3p10: overflow, and underflow (zero or subnormal from a nonzero decimal), set ERANGE */
        if (__u_v > 1.7976931348623157e308) errno = ERANGE;
        else { if (__u_m != 0 && __u_v < 2.2250738585072014e-308) errno = ERANGE; }
    }
    if (__u_end) *__u_end = (char *)(__u_s + (__u_any ? __u_i : 0));
    return __u_v * (double)__u_sign;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_strtof
static float strtof(const char *__u_s, char **__u_end) { return (float)strtod(__u_s, __u_end); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_strtold
static double strtold(const char *__u_s, char **__u_end) { return strtod(__u_s, __u_end); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_atof
static double atof(const char *__u_s) { return strtod(__u_s, 0); }
#endif

static __u_word _unisa_seed = 1;
#if !__UNISA_FTRIM_LIBC || __UN_rand
static int rand(void) {
    _unisa_seed = _unisa_seed * 1103515245 + 12345;
    return (int)((_unisa_seed >> 16) & 32767);
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_srand
static void srand(int __u_s) { _unisa_seed = __u_s; }
#endif
/* ---- C99 7.20.6-7.20.7 and the pieces of 7.20.4 a program can rely on
   here: labs, div, qsort, bsearch, atexit.  Real code, like the rest of
   this header: the sort is a heap sort, so its worst case is bounded and
   it needs no stack beyond one element's bytes [S-15 D2]. */
#if !__UNISA_FTRIM_LIBC || __UN_labs
static long labs(long __u_v) { if (__u_v < 0) return 0 - __u_v; return __u_v; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_llabs
static long long llabs(long long __u_v) { if (__u_v < 0) return 0 - __u_v; return __u_v; }
#endif
typedef struct { int quot; int rem; } div_t;
typedef struct { long quot; long rem; } ldiv_t;
typedef struct { long long quot; long long rem; } lldiv_t;
#if !__UNISA_FTRIM_LIBC || __UN_div
static div_t div(int __u_a, int __u_b) { div_t __u_r; __u_r.quot = __u_a / __u_b; __u_r.rem = __u_a % __u_b; return __u_r; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_ldiv
static ldiv_t ldiv(long __u_a, long __u_b) { ldiv_t __u_r; __u_r.quot = __u_a / __u_b; __u_r.rem = __u_a % __u_b; return __u_r; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_lldiv
static lldiv_t lldiv(long long __u_a, long long __u_b) { lldiv_t __u_r; __u_r.quot = __u_a / __u_b; __u_r.rem = __u_a % __u_b; return __u_r; }
#endif

#if !__UNISA_FTRIM_LIBC || __UN__unisa_swap
static void _unisa_swap(char *__u_a, char *__u_b, __u_word __u_n) {
    __u_word __u_i; char __u_t;
    __u_i = 0;
    while (__u_i < __u_n) { __u_t = __u_a[__u_i]; __u_a[__u_i] = __u_b[__u_i]; __u_b[__u_i] = __u_t; __u_i = __u_i + 1; }
}
#endif

/* Sift element `i` of a heap of `n` elements down to its place. */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sift
static void _unisa_sift(char *__u_base, __u_word __u_n, __u_word __u_sz, __u_word __u_i, int (*__u_cmp)(const void *, const void *)) {
    __u_word __u_c;
    while (1) {
        __u_c = 2 * __u_i + 1;
        if (__u_c >= __u_n) return;
        if (__u_c + 1 < __u_n) { if (__u_cmp(__u_base + __u_c * __u_sz, __u_base + (__u_c + 1) * __u_sz) < 0) __u_c = __u_c + 1; }
        if (__u_cmp(__u_base + __u_i * __u_sz, __u_base + __u_c * __u_sz) >= 0) return;
        _unisa_swap(__u_base + __u_i * __u_sz, __u_base + __u_c * __u_sz, __u_sz);
        __u_i = __u_c;
    }
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_qsort
static void qsort(void *__u_v, __u_word __u_n, __u_word __u_sz, int (*__u_cmp)(const void *, const void *)) {
    char *__u_base; __u_word __u_i;
    __u_base = (char *)__u_v;
    if (__u_n < 2) return;
    __u_i = __u_n / 2;
    while (__u_i > 0) { __u_i = __u_i - 1; _unisa_sift(__u_base, __u_n, __u_sz, __u_i, __u_cmp); }
    __u_i = __u_n;
    while (__u_i > 1) {
        __u_i = __u_i - 1;
        _unisa_swap(__u_base, __u_base + __u_i * __u_sz, __u_sz);
        _unisa_sift(__u_base, __u_i, __u_sz, 0, __u_cmp);
    }
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_bsearch
static void *bsearch(const void *__u_key, const void *__u_v, __u_word __u_n, __u_word __u_sz, int (*__u_cmp)(const void *, const void *)) {
    char *__u_base; __u_word __u_lo; __u_word __u_hi; __u_word __u_mid; int __u_c;
    __u_base = (char *)__u_v; __u_lo = 0; __u_hi = __u_n;
    while (__u_lo < __u_hi) {
        __u_mid = __u_lo + (__u_hi - __u_lo) / 2;
        __u_c = __u_cmp(__u_key, __u_base + __u_mid * __u_sz);
        if (__u_c == 0) return __u_base + __u_mid * __u_sz;
        if (__u_c < 0) __u_hi = __u_mid; else __u_lo = __u_mid + 1;
    }
    return 0;
}
#endif

/* atexit: the handlers run, last registered first, from `exit` -- which is
   also where a return from main ends up, since the entry stub calls `exit`
   whenever this header defined one.  Thirty-two is C's minimum (7.20.4.2). */
#if !__UNISA_FTRIM_LIBC || __UN_atexit
static int atexit(void (*__u_fn)(void)) {
    if (_unisa_natexit >= 32) return 0 - 1;
    _unisa_atexit[_unisa_natexit] = __u_fn;
    _unisa_natexit = _unisa_natexit + 1;
    return 0;
}
#endif

/* The environment is where a Unix kernel leaves it: after argv's NULL.
   `__argv(k)` reads that array itself, so the walk starts past argc.  It is
   copied once into environ (below), and environ is the one store: getenv
   reads it, setenv/unsetenv edit it in place (POSIX), and every child --
   execv/execvp/execl*, system, popen -- is handed it, so a variable set
   here is what a child sees (it was a 16-name override table of 255 bytes
   each that only getenv consulted, until 0.0.43). [S-15 D2] */
#ifdef _WIN32
#endif
/* retired with the override table (0.0.43); kept unreferenced so a program that calls no
   environment function keeps its bytes -- delete together with the next bss change */
static char _unisa_envk[16][256];
static int _unisa_envf[16];
/* 0.0.31 H1'': the process environment, one `char **` for the whole program: external linkage,
   a tentative definition in every unit, merged at link (C99 6.9.2).  <unistd.h> names it `environ`.
   A constructor fills it before main from the block the kernel left after argv (0.0.28 F1);
   -ftrim-libc keeps it as a root (unisa/libneed.py ROOTS). */
char **_unisa_env;
static char *_unisa_envv[4096];
#if !__UNISA_FTRIM_LIBC || __UN__unisa_environ_init
__attribute__((constructor)) static void _unisa_environ_init(void) {
    int __u_k; int __u_n;
    if (_unisa_env) return;                 /* another unit's constructor already filled it */
    __u_k = __argc() + 1; __u_n = 0;
    while (__u_n < 4095 && __argv(__u_k) != 0) { _unisa_envv[__u_n] = __argv(__u_k); __u_n = __u_n + 1; __u_k = __u_k + 1; }
    _unisa_envv[__u_n] = 0;
    _unisa_env = _unisa_envv;
}
#endif
/* Windows reads the real environment through the host channel in 0.0.22,
   when the product lowers it (the compiler itself must not need it yet). */
/* the index in environ of the entry "NAME=...", or -1 (NAME holds no '=') */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_envfind
static __u_word _unisa_envfind(const char *__u_name) {
    __u_word __u_k; int __u_i; char *__u_e;
    if (!_unisa_env) _unisa_environ_init();   /* called from a constructor that ran first */
    for (__u_k = 0; (__u_e = _unisa_env[__u_k]) != 0; __u_k++) {
        __u_i = 0;
        while (__u_name[__u_i] && __u_e[__u_i] == __u_name[__u_i]) __u_i = __u_i + 1;
        if (__u_name[__u_i] == 0 && __u_e[__u_i] == 61) return __u_k;
    }
    return -1;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_getenv
static char *getenv(const char *__u_name) {
    __u_word __u_k; int __u_i;
    __u_k = _unisa_envfind(__u_name);
    if (__u_k < 0) return 0;
    __u_i = 0; while (_unisa_env[__u_k][__u_i] != 61) __u_i = __u_i + 1;
    return _unisa_env[__u_k] + __u_i + 1;
}
#endif
/* setenv: "NAME=VALUE" in one malloc'd string, any length, put in environ in place of
   NAME's entry or appended.  An entry is never freed: a pointer getenv returned stays
   valid, as glibc and the BSDs keep them.  Appending past the kernel's copy (or past an
   array the program assigned to environ) moves environ to a malloc'd array twice as big. */
#if !__UNISA_FTRIM_LIBC || __UN_setenv
static char **_unisa_envheap; static __u_word _unisa_envcap;
#endif
#if !__UNISA_FTRIM_LIBC || __UN_setenv
static int setenv(const char *__u_name, const char *__u_val, int __u_over) {
    __u_word __u_k; __u_word __u_n; __u_word __u_i; __u_word __u_j; char *__u_s; char **__u_a;
    if (!__u_name || !__u_name[0]) { errno = EINVAL; return -1; }
    for (__u_i = 0; __u_name[__u_i]; __u_i++) if (__u_name[__u_i] == 61) { errno = EINVAL; return -1; }
    __u_k = _unisa_envfind(__u_name);
    if (__u_k >= 0 && !__u_over) return 0;
    __u_j = 0; while (__u_val[__u_j]) __u_j = __u_j + 1;
    __u_s = (char *)malloc(__u_i + __u_j + 2);
    if (!__u_s) { errno = ENOMEM; return -1; }
    for (__u_j = 0; __u_name[__u_j]; __u_j++) __u_s[__u_j] = __u_name[__u_j];
    __u_s[__u_i] = 61;
    for (__u_j = 0; __u_val[__u_j]; __u_j++) __u_s[__u_i + 1 + __u_j] = __u_val[__u_j];
    __u_s[__u_i + 1 + __u_j] = 0;
    if (__u_k >= 0) { _unisa_env[__u_k] = __u_s; return 0; }
    __u_n = 0; while (_unisa_env[__u_n]) __u_n = __u_n + 1;
    if (!((_unisa_env == _unisa_envv && __u_n + 1 < 4096) || (_unisa_env == _unisa_envheap && __u_n + 1 < _unisa_envcap))) {
        __u_a = (char **)malloc((2 * __u_n + 16) * sizeof(char *));
        if (!__u_a) { free(__u_s); errno = ENOMEM; return -1; }
        for (__u_k = 0; __u_k < __u_n; __u_k++) __u_a[__u_k] = _unisa_env[__u_k];
        if (_unisa_env == _unisa_envheap) free(_unisa_envheap);
        _unisa_envheap = __u_a; _unisa_envcap = 2 * __u_n + 16; _unisa_env = __u_a;
    }
    _unisa_env[__u_n] = __u_s; _unisa_env[__u_n + 1] = 0;
    return 0;
}
#endif
/* unsetenv: every "NAME=..." entry leaves environ, the rest keep their order */
#if !__UNISA_FTRIM_LIBC || __UN_unsetenv
static int unsetenv(const char *__u_name) {
    __u_word __u_k; __u_word __u_m; int __u_i;
    if (!__u_name || !__u_name[0]) { errno = EINVAL; return -1; }
    for (__u_i = 0; __u_name[__u_i]; __u_i++) if (__u_name[__u_i] == 61) { errno = EINVAL; return -1; }
    while ((__u_k = _unisa_envfind(__u_name)) >= 0) {
        __u_m = __u_k; while (_unisa_env[__u_m]) { _unisa_env[__u_m] = _unisa_env[__u_m + 1]; __u_m = __u_m + 1; }
    }
    return 0;
}
#endif

/* system: /bin/sh -c CMD in a child, waited for; the kernel's calls, no
   host library (0.0.21: the compiler must build without the host channel).
   Windows: not yet (0.0.22, through the host channel). */
#if !defined(_WIN32) && !defined(__UNISA_PYFRONT)   /* the Python front end keeps every body and folds one tape across targets */
#if !__UNISA_FTRIM_LIBC || __UN_system
static int system(const char *__u_cmd) {
    char **__u_env; char *__u_argv[4]; long __u_pid; long __u_me; int __u_st;
    if (__u_cmd == 0) return 1;
    __u_argv[0] = "sh"; __u_argv[1] = "-c"; __u_argv[2] = (char *)__u_cmd; __u_argv[3] = 0;
    if (!_unisa_env) _unisa_environ_init();   /* the child gets environ as it stands now */
    __u_env = _unisa_env;
#ifdef __APPLE__
#ifdef __x86_64__
    __u_me = __syscall6(0x2000000L + 20, 0, 0, 0, 0, 0);
    __u_pid = __syscall6(0x2000000L + 2, 0, 0, 0, 0, 0);
    if (__u_pid >= 0 && __syscall6(0x2000000L + 20, 0, 0, 0, 0, 0) != __u_me) __u_pid = 0;
#else
    __u_me = __syscall6(20, 0, 0, 0, 0, 0);
    __u_pid = __syscall6(2, 0, 0, 0, 0, 0);
    if (__u_pid >= 0 && __syscall6(20, 0, 0, 0, 0, 0) != __u_me) __u_pid = 0;
#endif
#elif defined(__x86_64__)
    __u_me = 0; __u_pid = __syscall6(57, 0, 0, 0, 0, 0);
#else
    __u_me = 0; __u_pid = __syscall6(220, 17, 0, 0, 0, 0);
#endif
    if (__u_pid < 0) return -1;
    if (__u_pid == 0) {
#if defined(__APPLE__) && defined(__x86_64__)
        __syscall6(0x2000000L + 59, (long)"/bin/sh", (long)__u_argv, (long)__u_env, 0, 0);
#elif defined(__APPLE__) || defined(__x86_64__)
        __syscall6(59, (long)"/bin/sh", (long)__u_argv, (long)__u_env, 0, 0);
#else
        __syscall6(221, (long)"/bin/sh", (long)__u_argv, (long)__u_env, 0, 0);
#endif
        __exit(127);
    }
    __u_st = 0;
#if defined(__APPLE__) && defined(__x86_64__)
    if (__syscall6(0x2000000L + 7, __u_pid, (long)&__u_st, 0, 0, 0) < 0) return -1;
#elif defined(__APPLE__)
    if (__syscall6(7, __u_pid, (long)&__u_st, 0, 0, 0) < 0) return -1;
#elif defined(__x86_64__)
    if (__syscall6(61, __u_pid, (long)&__u_st, 0, 0, 0) < 0) return -1;
#else
    if (__syscall6(260, __u_pid, (long)&__u_st, 0, 0, 0) < 0) return -1;
#endif
    return __u_st;
}
#endif
#endif
/* mbtowc/wctomb/mbstowcs/wcstombs (C99 7.20.7, 7.20.8) in the C locale,
 * where the multibyte encoding is one byte per character: a byte below 128
 * is the character with the same value, a byte of 128 or more is an invalid
 * multibyte character (EILSEQ), and no shift state exists.  Return values
 * and errno follow the host's C-locale behaviour exactly; a wide character
 * outside 0..127 has no multibyte form.  No function touches errno on
 * success.  mbtowc with n == 0: the empty string still converts to 0, any
 * other string is "incomplete" and gives -1 without errno. */
#if !__UNISA_FTRIM_LIBC || __UN_mbtowc
static int mbtowc(wchar_t *__u_pwc, const char *__u_s, size_t __u_n) {
    unsigned char __u_c;
    if (__u_s == 0) return 0;
    if (__u_s[0] == 0) { if (__u_pwc) *__u_pwc = 0; return 0; }
    if (__u_n == 0) return -1;
    __u_c = (unsigned char)__u_s[0];
    if (__u_c >= 128) { errno = EILSEQ; return -1; }
    if (__u_pwc) *__u_pwc = (wchar_t)__u_c;
    return 1;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_wctomb
static int wctomb(char *__u_s, wchar_t __u_wc) {
    if (__u_s == 0) return 0;
    if (__u_wc < 0 || __u_wc > 127) { errno = EILSEQ; return -1; }
    __u_s[0] = (char)__u_wc;
    return 1;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_mbstowcs
static size_t mbstowcs(wchar_t *__u_dst, const char *__u_src, size_t __u_n) {
    size_t __u_i; unsigned char __u_c;
    __u_i = 0;
    for (;;) {
        if (__u_dst && __u_i >= __u_n) return __u_i;
        __u_c = (unsigned char)__u_src[__u_i];
        if (__u_c >= 128) { errno = EILSEQ; return (size_t)-1; }
        if (__u_dst) __u_dst[__u_i] = (wchar_t)__u_c;
        if (__u_c == 0) return __u_i;
        __u_i = __u_i + 1;
    }
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_wcstombs
static size_t wcstombs(char *__u_dst, const wchar_t *__u_src, size_t __u_n) {
    size_t __u_i; wchar_t __u_wc;
    __u_i = 0;
    for (;;) {
        if (__u_dst && __u_i >= __u_n) return __u_i;
        __u_wc = __u_src[__u_i];
        if (__u_wc < 0 || __u_wc > 127) { errno = EILSEQ; return (size_t)-1; }
        if (__u_dst) __u_dst[__u_i] = (char)__u_wc;
        if (__u_wc == 0) return __u_i;
        __u_i = __u_i + 1;
    }
}
#endif
/* 0.0.34 L2 (SQLite's Apple path): forwarded to the system C library */
#ifdef __APPLE__
long random(void);
void srandomdev(void);
#endif
#endif


/* <string.h> for the unisa C subset: real implementations, compiled from
 * source, because there is no linker and nothing to link against.  A program
 * that includes this header carries the functions it uses -- and, today, the
 * ones it does not. */
#ifndef _UNISA_STRING_H
#define _UNISA_STRING_H


#define NULL 0

#if !__UNISA_FTRIM_LIBC || __UN_strlen
static __u_word strlen(const char *__u_s) {
    __u_word __u_n;
    __u_n = 0;
    while (__u_s[__u_n]) __u_n = __u_n + 1;
    return __u_n;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_strcpy
static char *strcpy(char *__u_d, const char *__u_s) {
    __u_word __u_i;
    __u_i = 0;
    while (__u_s[__u_i]) { __u_d[__u_i] = __u_s[__u_i]; __u_i = __u_i + 1; }
    __u_d[__u_i] = 0;
    return __u_d;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_strncpy
static char *strncpy(char *__u_d, const char *__u_s, __u_word __u_n) {
    __u_word __u_i;
    __u_i = 0;
    while (__u_i < __u_n) { __u_d[__u_i] = __u_s[__u_i]; if (__u_s[__u_i] == 0) break; __u_i = __u_i + 1; }
    while (__u_i < __u_n) { __u_d[__u_i] = 0; __u_i = __u_i + 1; }
    return __u_d;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_strcat
static char *strcat(char *__u_d, const char *__u_s) {
    strcpy(__u_d + strlen(__u_d), __u_s);
    return __u_d;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_strcmp
static int strcmp(const char *__u_a, const char *__u_b) {
    __u_word __u_i;
    __u_i = 0;
    while (__u_a[__u_i]) { if (__u_a[__u_i] != __u_b[__u_i]) break; __u_i = __u_i + 1; }
    return (__u_a[__u_i] & 255) - (__u_b[__u_i] & 255);
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_strncmp
static int strncmp(const char *__u_a, const char *__u_b, __u_word __u_n) {
    __u_word __u_i;
    __u_i = 0;
    while (__u_i < __u_n) {
        if (__u_a[__u_i] != __u_b[__u_i]) return (__u_a[__u_i] & 255) - (__u_b[__u_i] & 255);
        if (__u_a[__u_i] == 0) break;
        __u_i = __u_i + 1;
    }
    return 0;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_strchr
static char *strchr(const char *__u_s, int __u_c) {
    __u_word __u_i;
    __u_i = 0;
    while (1) {
        if ((__u_s[__u_i] & 255) == (__u_c & 255)) return (char *)(__u_s + __u_i);
        if (__u_s[__u_i] == 0) return NULL;
        __u_i = __u_i + 1;
    }
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_strrchr
static char *strrchr(const char *__u_s, int __u_c) {
    __u_word __u_i;
    char *__u_r;
    __u_i = 0; __u_r = NULL;
    while (1) {
        if ((__u_s[__u_i] & 255) == (__u_c & 255)) __u_r = (char *)(__u_s + __u_i);
        if (__u_s[__u_i] == 0) return __u_r;
        __u_i = __u_i + 1;
    }
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_strstr
static char *strstr(const char *__u_h, const char *__u_n) {
    __u_word __u_i; __u_word __u_j;
    __u_i = 0;
    while (__u_h[__u_i]) {
        __u_j = 0;
        while (__u_n[__u_j]) { if (__u_h[__u_i + __u_j] != __u_n[__u_j]) break; __u_j = __u_j + 1; }
        if (__u_n[__u_j] == 0) return (char *)(__u_h + __u_i);
        __u_i = __u_i + 1;
    }
    return NULL;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_memset
static void *memset(void *__u_p, int __u_c, __u_word __u_n) {
    char *__u_d; __u_word __u_i;
    __u_d = (char *)__u_p; __u_i = 0;
    if (__u_n >= 16) {   /* D2: eight bytes a step */
        __u_word __u_w; __u_w = __u_c & 255; __u_w = __u_w * 72340172838076673L;
        while ((((__u_word)(__u_d + __u_i)) & 7) != 0) { __u_d[__u_i] = __u_c; __u_i = __u_i + 1; }
        while (__u_i + 8 <= __u_n) { *(__u_word *)(__u_d + __u_i) = __u_w; __u_i = __u_i + 8; }
    }
    while (__u_i < __u_n) { __u_d[__u_i] = __u_c; __u_i = __u_i + 1; }
    return __u_p;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_memcpy
static void *memcpy(void *__u_dst, const void *__u_src, __u_word __u_n) {
    char *__u_d; char *__u_s; __u_word __u_i;
    __u_d = (char *)__u_dst; __u_s = (char *)__u_src; __u_i = 0;
    /* D2: eight bytes a step when both pointers share an alignment: bytes up to an 8-byte
       boundary, aligned long words, then the tail; mismatched alignments stay bytewise */
    if (__u_n >= 16 && ((((__u_word)__u_d) ^ ((__u_word)__u_s)) & 7) == 0) {
        while ((((__u_word)(__u_d + __u_i)) & 7) != 0) { __u_d[__u_i] = __u_s[__u_i]; __u_i = __u_i + 1; }
        while (__u_i + 8 <= __u_n) { *(__u_word *)(__u_d + __u_i) = *(__u_word *)(__u_s + __u_i); __u_i = __u_i + 8; }
    }
    while (__u_i < __u_n) { __u_d[__u_i] = __u_s[__u_i]; __u_i = __u_i + 1; }
    return __u_dst;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_memmove
static void *memmove(void *__u_dst, const void *__u_src, __u_word __u_n) {
    char *__u_d; char *__u_s; __u_word __u_i;
    __u_d = (char *)__u_dst; __u_s = (char *)__u_src;
    if (__u_d < __u_s) return memcpy(__u_dst, __u_src, __u_n);
    __u_i = __u_n;
    while (__u_i > 0) { __u_i = __u_i - 1; __u_d[__u_i] = __u_s[__u_i]; }
    return __u_dst;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_memcmp
static int memcmp(const void *__u_a, const void *__u_b, __u_word __u_n) {
    char *__u_x; char *__u_y; __u_word __u_i;
    __u_x = (char *)__u_a; __u_y = (char *)__u_b; __u_i = 0;
    while (__u_i < __u_n) {
        if (__u_x[__u_i] != __u_y[__u_i]) return (__u_x[__u_i] & 255) - (__u_y[__u_i] & 255);
        __u_i = __u_i + 1;
    }
    return 0;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_strncat
static char *strncat(char *__u_d, const char *__u_s, __u_word __u_n) {
    __u_word __u_i; __u_word __u_j;
    __u_i = 0; while (__u_d[__u_i]) __u_i = __u_i + 1;
    __u_j = 0;
    while (__u_j < __u_n) { if (__u_s[__u_j] == 0) break; __u_d[__u_i + __u_j] = __u_s[__u_j]; __u_j = __u_j + 1; }
    __u_d[__u_i + __u_j] = 0;
    return __u_d;
}
#endif

/* strtok keeps its place between calls, as the standard says it must; the
   place is one static pointer, which is why strtok is not reentrant. */
static char *_unisa_tok = 0;
#if !__UNISA_FTRIM_LIBC || __UN_strtok
static char *strtok(char *__u_s, const char *__u_delim) {
    char *__u_start; __u_word __u_k; int __u_isd;
    if (__u_s == 0) __u_s = _unisa_tok;
    if (__u_s == 0) return 0;
    /* skip leading delimiters */
    while (*__u_s) {
        __u_isd = 0; __u_k = 0;
        while (__u_delim[__u_k]) { if (__u_delim[__u_k] == *__u_s) __u_isd = 1; __u_k = __u_k + 1; }
        if (__u_isd == 0) break;
        __u_s = __u_s + 1;
    }
    if (*__u_s == 0) { _unisa_tok = 0; return 0; }
    __u_start = __u_s;
    while (*__u_s) {
        __u_isd = 0; __u_k = 0;
        while (__u_delim[__u_k]) { if (__u_delim[__u_k] == *__u_s) __u_isd = 1; __u_k = __u_k + 1; }
        if (__u_isd) { *__u_s = 0; _unisa_tok = __u_s + 1; return __u_start; }
        __u_s = __u_s + 1;
    }
    _unisa_tok = 0;
    return __u_start;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_memchr
static void *memchr(const void *__u_p, int __u_c, __u_word __u_n) {
    const unsigned char *__u_s; __u_word __u_i;
    __u_s = (const unsigned char *)__u_p;
    __u_i = 0;
    while (__u_i < __u_n) {
        if (__u_s[__u_i] == (unsigned char)__u_c) return (void *)(__u_s + __u_i);
        __u_i = __u_i + 1;
    }
    return NULL;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_strspn
static __u_word strspn(const char *__u_s, const char *__u_accept) {
    __u_word __u_i; __u_word __u_j; int __u_found;
    __u_i = 0;
    while (__u_s[__u_i]) {
        __u_found = 0; __u_j = 0;
        while (__u_accept[__u_j]) {
            if (__u_s[__u_i] == __u_accept[__u_j]) __u_found = 1;
            __u_j = __u_j + 1;
        }
        if (!__u_found) break;
        __u_i = __u_i + 1;
    }
    return __u_i;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_strpbrk
static char *strpbrk(const char *__u_s, const char *__u_accept) {
    __u_word __u_i; __u_word __u_j;
    __u_i = 0;
    while (__u_s[__u_i]) {
        __u_j = 0;
        while (__u_accept[__u_j]) {
            if (__u_s[__u_i] == __u_accept[__u_j]) return (char *)(__u_s + __u_i);
            __u_j = __u_j + 1;
        }
        __u_i = __u_i + 1;
    }
    return NULL;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_strcspn
static __u_word strcspn(const char *__u_s, const char *__u_reject) {
    __u_word __u_i; __u_word __u_j;
    __u_i = 0;
    while (__u_s[__u_i]) {
        __u_j = 0;
        while (__u_reject[__u_j]) {
            if (__u_s[__u_i] == __u_reject[__u_j]) return __u_i;
            __u_j = __u_j + 1;
        }
        __u_i = __u_i + 1;
    }
    return __u_i;
}
#endif

/* The bundled locale has only the C collation order. */
#if !__UNISA_FTRIM_LIBC || __UN_strcoll
static int strcoll(const char *__u_a, const char *__u_b) {
    return strcmp(__u_a, __u_b);
}
#endif

/* 0.0.28 H4 (SQLite's shell): POSIX strdup, forwarded to the system C library on Linux and macOS */
#ifndef _WIN32
char *strdup(const char *__u_s);
#endif
/* C99 7.21.4.5 strxfrm: prototype-only, forwarded to the host's C library on Linux and macOS
 * (the bundled locale is C, whose transformation is the identity). */
#ifndef _WIN32
size_t strxfrm(char *__u_d, const char *__u_s, size_t __u_n);
#endif
/* 0.0.34 L2 (SQLite's Apple path): forwarded to the system C library */
#ifdef __APPLE__
size_t strlcpy(char *__u_d, const char *__u_s, size_t __u_n);
size_t strlcat(char *__u_d, const char *__u_s, size_t __u_n);
#endif
#endif

#ifndef _WIN32

/* Minimal POSIX <unistd.h> (0.0.17 R17-5, ordered by the realprog corpus):
 * the file-descriptor calls the bundled library already makes through its
 * own syscall intrinsics, under their POSIX names.  Return values follow
 * POSIX: -1 with errno set on failure.  isatty arrived in 0.0.18.  Not provided (yet): fork/exec,
 * getpid, sleep, pipes (POSIX L2). */
#ifndef _UNISA_UNISTD_H
#define _UNISA_UNISTD_H



/* <sys/types.h> for the unisa C subset.
 *
 * TYPES ONLY.  POSIX puts its shorthand scalar types here and the functions
 * that use them in <unistd.h>, <fcntl.h>, <sys/stat.h> and friends; the
 * reference implements none of those functions yet, so none of those headers
 * exists -- a constant or a prototype with no implementation behind it would
 * be inventing an interface.  This header is the part that is real and
 * self-contained: the widths.
 *
 * All of them are 64-bit, like the rest of the subset (prd [G-2]: `int`
 * expressions evaluate in 64 bits).  `ssize_t` is the signed twin of
 * `size_t`; `off_t` is signed so a negative seek works; `pid_t` and `mode_t`
 * are 32-bit on every platform this compiler targets, and `dev_t`/`ino_t`
 * are 64-bit there.  `time_t` lives in <time.h>; `clock_t` in <time.h> too.
 *
 * There is no `struct stat`: it belongs to <sys/stat.h>, which needs `stat`
 * and `fstat` to be worth declaring. */
#ifndef _UNISA_SYS_TYPES_H
#define _UNISA_SYS_TYPES_H

/* LLP64 Windows (_WIN64) spells the 8-byte types `long long`: every type
 * here keeps the same 8-byte width and layout on all targets. */
#if defined(_WIN64)
#ifndef _UNISA_SIZE_T
#define _UNISA_SIZE_T
typedef unsigned long long size_t;
#endif
typedef long long          ssize_t;
typedef long long          off_t;
typedef int                pid_t;
typedef unsigned int       uid_t;
typedef unsigned int       gid_t;
typedef unsigned int       mode_t;
typedef unsigned long long dev_t;
typedef unsigned long long ino_t;
typedef unsigned long long nlink_t;
typedef long long          blksize_t;
typedef long long          blkcnt_t;
#ifndef _UNISA_TIME_T
#define _UNISA_TIME_T
typedef long long          time_t;
#endif
typedef long long          suseconds_t;
typedef unsigned long long useconds_t;
#else
#ifndef _UNISA_SIZE_T
#define _UNISA_SIZE_T
typedef unsigned long size_t;
#endif
typedef long          ssize_t;
typedef long          off_t;
typedef int           pid_t;
typedef unsigned int  uid_t;
typedef unsigned int  gid_t;
typedef unsigned int  mode_t;
typedef unsigned long dev_t;
typedef unsigned long ino_t;
typedef unsigned long nlink_t;
typedef long          blksize_t;
typedef long          blkcnt_t;
#ifndef _UNISA_TIME_T
#define _UNISA_TIME_T
typedef long          time_t;
#endif
typedef long          suseconds_t;
typedef unsigned long useconds_t;
#endif

#endif


/* sys/_ret.h -- the one place a raw "-errno" result becomes -1 + errno.
 * Shared by every header whose bodies call the kernel (0.0.21: <time.h>
 * used it without including <unistd.h>, so clock_gettime alone failed). */
#ifndef _UNISA_RET_H
#define _UNISA_RET_H

#if !__UNISA_FTRIM_LIBC || __UN__unisa_ret
static long _unisa_ret(long __u_r) { if (__u_r < 0 && __u_r > -4096) { errno = (int)(0 - __u_r); return -1; } return __u_r; }
#endif
#endif



/* sys/_win.h -- 0.0.21 Windows POSIX layer, batch 1: the shared helpers that
 * let <unistd.h>, <fcntl.h>, <sys/stat.h> and <time.h> wrap POSIX names thinly
 * over kernel32 (no home-grown libc).  The host channel is the program's IAT:
 * __hostaddr0..3 = LoadLibraryA GetProcAddress FreeLibrary GetLastError, and
 * __hostcall(fn, __u_word[10]) makes the Win64/AAPCS64 call.  kernel32's handle is
 * looked up once; each caller caches its own procedure in a static.
 * On Windows an fd IS the HANDLE value (as the bundled stdio already has it).
 * Windows is LLP64 (long is 4 bytes): handles, procedures, pointers and the host
 * channel's 8-byte slots are __u_word (<sys/_word.h>), never long. */
#ifndef _UNISA_SYS_WIN_H
#define _UNISA_SYS_WIN_H
#if defined(_WIN32) && !defined(_UNISA_NO_HOSTCALL)   /* the compiler builds itself without the host channel until the product lowers it on Windows (0.0.22) */


#if !__UNISA_FTRIM_LIBC || __UN__ux_sym
static __u_word _ux_sym(const char *__u_name) {          /* GetProcAddress(kernel32, name) */
    static __u_word __u_k32;
    __u_word __u_a[10] = {0};
    __u_a[1] = 0; __u_a[2] = 0; __u_a[3] = 0; __u_a[4] = 0; __u_a[5] = 0;
    if (!__u_k32) { __u_a[0] = (__u_word)"kernel32.dll"; __u_k32 = __hostcall(__hostaddr0(), __u_a); }
    __u_a[0] = __u_k32; __u_a[1] = (__u_word)__u_name;
    return __hostcall(__hostaddr1(), __u_a);
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__ux_call
static __u_word _ux_call(__u_word __u_fn, __u_word __u_x0, __u_word __u_x1, __u_word __u_x2, __u_word __u_x3) {
    __u_word __u_a[10] = {0};
    __u_a[0] = __u_x0; __u_a[1] = __u_x1; __u_a[2] = __u_x2; __u_a[3] = __u_x3; __u_a[4] = 0; __u_a[5] = 0;
    return __hostcall(__u_fn, __u_a);
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__ux_errno
static int _ux_errno(void) {                         /* GetLastError -> errno */
    __u_word __u_a[10] = {0}; __u_word __u_e;
    __u_a[0] = 0; __u_a[1] = 0; __u_a[2] = 0; __u_a[3] = 0; __u_a[4] = 0; __u_a[5] = 0;
    __u_e = __hostcall(__hostaddr3(), __u_a) & 0xFFFFFFFFLL;
    if (__u_e == 2 || __u_e == 3) return ENOENT;
    if (__u_e == 5) return EACCES;
    if (__u_e == 6) return EBADF;
    if (__u_e == 80 || __u_e == 183) return EEXIST;
    if (__u_e == 145) return ENOTEMPTY;
    if (__u_e == 32) return EBUSY;
    if (__u_e == 87) return EINVAL;
    return EIO;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__ux_fail
static int _ux_fail(void) { errno = _ux_errno(); return -1; }
#endif
/* The names above are kept only when a baked library key lists them. A disk
   body that started calling them later is compiled, and the callee is then
   missing. These macros expand in the caller and use the host channel
   directly, same mapping as _ux_sym / _ux_call / _ux_errno. */
#define _ux_load(dst, name) do { \
    static __u_word __uxk; __u_word __uxa[10]; int __uxi; \
    __uxi = 0; while (__uxi < 10) { __uxa[__uxi] = 0; __uxi = __uxi + 1; } \
    if (!__uxk) { __uxa[0] = (__u_word)"kernel32.dll"; __uxk = __hostcall(__hostaddr0(), __uxa); } \
    __uxa[0] = __uxk; __uxa[1] = (__u_word)(name); (dst) = __hostcall(__hostaddr1(), __uxa); \
} while (0)
#define _ux_go(dst, fn, x0, x1, x2, x3) do { \
    __u_word __uxa[10]; int __uxi; \
    __uxi = 0; while (__uxi < 10) { __uxa[__uxi] = 0; __uxi = __uxi + 1; } \
    __uxa[0] = (__u_word)(x0); __uxa[1] = (__u_word)(x1); __uxa[2] = (__u_word)(x2); __uxa[3] = (__u_word)(x3); \
    (dst) = __hostcall((fn), __uxa); \
} while (0)
#define _ux_err(dst) do { \
    __u_word __uxa[10]; __u_word __uxe; int __uxi; \
    __uxi = 0; while (__uxi < 10) { __uxa[__uxi] = 0; __uxi = __uxi + 1; } \
    __uxe = __hostcall(__hostaddr3(), __uxa) & 0xFFFFFFFFLL; \
    if (__uxe == 2 || __uxe == 3) (dst) = ENOENT; \
    else if (__uxe == 5) (dst) = EACCES; \
    else if (__uxe == 6) (dst) = EBADF; \
    else if (__uxe == 80 || __uxe == 183) (dst) = EEXIST; \
    else if (__uxe == 145) (dst) = ENOTEMPTY; \
    else if (__uxe == 32) (dst) = EBUSY; \
    else if (__uxe == 87) (dst) = EINVAL; \
    else (dst) = EIO; \
} while (0)
#if !__UNISA_FTRIM_LIBC || __UN__ux_ft2unix
static __u_word _ux_ft2unix(const unsigned int *__u_ft) {   /* FILETIME (100 ns since 1601) -> unix seconds */
    __u_word __u_t; __u_t = ((__u_word)__u_ft[1] << 32) | (__u_word)__u_ft[0];
    return (__u_t - 116444736000000000LL) / 10000000LL;
}
#endif
#endif
#endif

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif
#if !__UNISA_FTRIM_LIBC || __UN_read
static ssize_t read(int __u_fd, void *__u_buf, size_t __u_n) {
    __u_word __u_r; __u_r = __read(__u_fd, (char *)__u_buf, (__u_word)__u_n);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; }
    return __u_r;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_write
static ssize_t write(int __u_fd, const void *__u_buf, size_t __u_n) {
    __u_word __u_r; __u_r = __write(__u_fd, (char *)__u_buf, (__u_word)__u_n);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; }
    return __u_r;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_close
static int close(int __u_fd) {
    __u_word __u_r; __u_r = __close(__u_fd);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; }
    return 0;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_lseek
static off_t lseek(int __u_fd, off_t __u_off, int __u_whence) {
    __u_word __u_r; __u_r = __lseek(__u_fd, (__u_word)__u_off, __u_whence);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; }
    return __u_r;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_isatty
static int isatty(int __u_fd) {               /* a terminal answers the attribute request (R18-10) */
#ifdef _WIN32
    static __u_word __u_f;                        /* GetFileType == FILE_TYPE_CHAR */
    if (!__u_f) __u_f = _ux_sym("GetFileType");
    if ((_ux_call(__u_f, (__u_word)__u_fd, 0, 0, 0) & 0xFFFFFFFFL) == 2) return 1;
    errno = ENOTTY; return 0;
#else
    unsigned char __u_t[128]; __u_word __u_r;
#ifdef __APPLE__
    __u_r = __ioctl(__u_fd, 0x40487413L, (char *)__u_t);
#else
    __u_r = __ioctl(__u_fd, 0x5401L, (char *)__u_t);
#endif
    if (__u_r < 0) { errno = (int)(0 - __u_r); return 0; } return 1;
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_ftruncate
static int ftruncate(int __u_fd, off_t __u_len) {   /* R18-5 (kilo saves through it) */
#ifdef _WIN32
    static __u_word __u_sp; static __u_word __u_se; __u_word __u_cur; int __u_ok;   /* the file position is kept, as POSIX has it */
    if (!__u_sp) { __u_sp = _ux_sym("SetFilePointerEx"); __u_se = _ux_sym("SetEndOfFile"); }
    if (__u_len < 0) { errno = EINVAL; return -1; }
    __u_cur = 0;
    if (!(_ux_call(__u_sp, (__u_word)__u_fd, 0, (__u_word)&__u_cur, 1) & 0xFFFFFFFFL)) return _ux_fail();
    if (!(_ux_call(__u_sp, (__u_word)__u_fd, (__u_word)__u_len, 0, 0) & 0xFFFFFFFFL)) return _ux_fail();
    __u_ok = (int)(_ux_call(__u_se, (__u_word)__u_fd, 0, 0, 0) & 0xFFFFFFFFL);
    if (!__u_ok) __u_ok = 0 - _ux_errno();
    _ux_call(__u_sp, (__u_word)__u_fd, __u_cur, 0, 0);
    if (__u_ok < 0) { errno = 0 - __u_ok; return -1; }
    return 0;
#else
    __u_word __u_r; __u_r = __ftruncate(__u_fd, (__u_word)__u_len, 0);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; }
    return 0;
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_unlink
static int unlink(const char *__u_path) {
#ifdef _WIN32
    static __u_word __u_f;                        /* DeleteFileA: a BOOL */
    if (!__u_f) __u_f = _ux_sym("DeleteFileA");
    if (_ux_call(__u_f, (__u_word)__u_path, 0, 0, 0) & 0xFFFFFFFFL) return 0;
    return _ux_fail();
#else
    __u_word __u_r; __u_r = __unlink((char *)__u_path);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; }
    return 0;
#endif
}
#endif
/* *at() flags (0.0.35 M1, minicon's loader): each kernel's own values */
#ifndef AT_FDCWD
#ifdef __APPLE__
#define AT_FDCWD (0 - 2)
#define AT_SYMLINK_NOFOLLOW 0x20
#define AT_REMOVEDIR 0x80
#else
#define AT_FDCWD (0 - 100)
#define AT_SYMLINK_NOFOLLOW 0x100
#define AT_REMOVEDIR 0x200
#endif
#endif
#if !__UNISA_FTRIM_LIBC || __UN_unlinkat
static int unlinkat(int __u_dfd, const char *__u_p, int __u_fl) {
#ifdef _WIN32
    if (__u_dfd != AT_FDCWD) { errno = ENOSYS; return -1; }
    return (__u_fl & AT_REMOVEDIR) ? rmdir(__u_p) : unlink(__u_p);
#else
    __u_word __u_r;
#if defined(__APPLE__)
#if defined(__x86_64__)
    __u_r = __syscall6(0x2000000L + 472, (__u_word)__u_dfd, (__u_word)__u_p, (__u_word)__u_fl, 0, 0);
#else
    __u_r = __syscall6(472, (__u_word)__u_dfd, (__u_word)__u_p, (__u_word)__u_fl, 0, 0);
#endif
#elif defined(__x86_64__)
    __u_r = __syscall6(263, (__u_word)__u_dfd, (__u_word)__u_p, (__u_word)__u_fl, 0, 0);
#else
    __u_r = __syscall6(35, (__u_word)__u_dfd, (__u_word)__u_p, (__u_word)__u_fl, 0, 0);
#endif
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; } return 0;
#endif
}
#endif
/* ---- processes (0.0.19 R19-5), through the generic gate __syscall6 (R19-9):
   the call numbers live here, per OS/arch; nothing in the compiler's tables.
   macOS x86-64 numbers carry the BSD class 0x2000000.  macOS fork returns the
   same value in both processes (the child mark is in a second register), so
   the child is told apart by its pid; macOS has no pipe2, so pipe is a
   socketpair (AF_UNIX stream: bidirectional, stated rather than hidden). */
#ifndef _WIN32
#if defined(__APPLE__) && defined(__x86_64__)
#define _UNISA_SC(n) (0x2000000L + (n))
#else
#define _UNISA_SC(n) ((__u_word)(n))
#endif
#ifdef __APPLE__
#define _UNISA_NR_getpid 20
#define _UNISA_NR_fork 2
#define _UNISA_NR_execve 59
#define _UNISA_NR_wait4 7
#define _UNISA_NR_dup2 90
#define _UNISA_NR_getppid 39
#elif defined(__x86_64__)
#define _UNISA_NR_getpid 39
#define _UNISA_NR_fork 57
#define _UNISA_NR_execve 59
#define _UNISA_NR_wait4 61
#define _UNISA_NR_dup2 33
#define _UNISA_NR_pipe2 293
#define _UNISA_NR_getcwd 79
#define _UNISA_NR_getppid 110
#else
#define _UNISA_NR_getpid 172
#define _UNISA_NR_clone 220
#define _UNISA_NR_execve 221
#define _UNISA_NR_wait4 260
#define _UNISA_NR_dup3 24
#define _UNISA_NR_pipe2 59
#define _UNISA_NR_getcwd 17
#define _UNISA_NR_getppid 173
#endif
/* _exit: no atexit handlers, no stdio flush */
#if !__UNISA_FTRIM_LIBC || __UN__exit
static void _exit(int __u_code) { __exit(__u_code); }
#endif

#endif
#if !__UNISA_FTRIM_LIBC || __UN_getpid
static pid_t getpid(void) {
#ifdef _WIN32
    static __u_word __u_f;
    if (!__u_f) __u_f = _ux_sym("GetCurrentProcessId");
    return (pid_t)(_ux_call(__u_f, 0, 0, 0, 0) & 0xFFFFFFFFL);
#else
    return (pid_t)__syscall6(_UNISA_SC(_UNISA_NR_getpid), 0, 0, 0, 0, 0);
#endif
}
#endif
#ifndef _WIN32
#if !__UNISA_FTRIM_LIBC || __UN_getppid
static pid_t getppid(void) { return (pid_t)__syscall6(_UNISA_SC(_UNISA_NR_getppid), 0, 0, 0, 0, 0); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_fork
static pid_t fork(void) {
    __u_word __u_r;
#ifdef __APPLE__
    long __u_me; __u_me = getpid();
    __u_r = __syscall6(_UNISA_SC(_UNISA_NR_fork), 0, 0, 0, 0, 0);
    if (__u_r >= 0 && getpid() != __u_me) return 0;   /* the child */
#elif defined(__x86_64__)
    __u_r = __syscall6(_UNISA_NR_fork, 0, 0, 0, 0, 0);
#else
    __u_r = __syscall6(_UNISA_NR_clone, 17, 0, 0, 0, 0);    /* SIGCHLD, no new stack: fork */
#endif
    return (pid_t)_unisa_ret(__u_r);
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_execve
static int execve(const char *__u_path, char *const __u_argv[], char *const __u_envp[]) {
    return (int)_unisa_ret(__syscall6(_UNISA_SC(_UNISA_NR_execve), (__u_word)__u_path, (__u_word)__u_argv, (__u_word)__u_envp, 0, 0));
}
#endif
/* 0.0.31 H1'': POSIX `environ` is <stdlib.h>'s _unisa_env, one object for the whole program
   (a tentative definition with external linkage in every unit, merged at link, C99 6.9.2), so an
   assignment in one unit is what getenv and execv see in every other. */

#define environ _unisa_env
/* execv passes the current environ (POSIX; 0.0.28 E20) */
#if !__UNISA_FTRIM_LIBC || __UN_execv
static int execv(const char *__u_path, char *const __u_argv[]) { return execve(__u_path, __u_argv, environ); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_execvp

#if !__UNISA_FTRIM_LIBC || __UN_execvp
static int execvp(const char *__u_file, char *const __u_argv[]) {
    char __u_buf[1024]; const char *__u_p; int __u_i; int __u_j; int __u_k;
    __u_i = 0; while (__u_file[__u_i]) { if (__u_file[__u_i] == 47) return execv(__u_file, __u_argv); __u_i = __u_i + 1; }
    __u_p = getenv("PATH"); if (__u_p == 0) __u_p = "/usr/bin:/bin";
    while (1) {
        __u_j = 0;
        while (__u_p[__u_j] && __u_p[__u_j] != 58 && __u_j < 900) { __u_buf[__u_j] = __u_p[__u_j]; __u_j = __u_j + 1; }
        if (__u_j == 0) { __u_buf[0] = 46; __u_j = 1; }
        __u_buf[__u_j] = 47; __u_k = 0;
        while (__u_file[__u_k] && __u_j + 1 + __u_k < 1023) { __u_buf[__u_j + 1 + __u_k] = __u_file[__u_k]; __u_k = __u_k + 1; }
        __u_buf[__u_j + 1 + __u_k] = 0;
        execv(__u_buf, __u_argv);
        while (*__u_p && *__u_p != 58) __u_p = __u_p + 1;
        if (*__u_p == 0) break;
        __u_p = __u_p + 1;
    }
    return -1;
}
#endif
#endif
/* execl family (0.0.20 R20-6, dsh): the variadic list gathered into an
   array, then execve/execvp; at most 255 arguments */

#if !__UNISA_FTRIM_LIBC || __UN_execl
static int execl(const char *__u_path, const char *__u_a0, ...) {
    char *__u_v[256]; int __u_n; va_list __u_ap;
    __u_v[0] = (char *)__u_a0; __u_n = 1; va_start(__u_ap, __u_a0);
    while (__u_v[__u_n - 1] && __u_n < 256) { __u_v[__u_n] = va_arg(__u_ap, char *); __u_n = __u_n + 1; }
    va_end(__u_ap);
    if (__u_v[__u_n - 1]) return (int)_unisa_ret(0 - 7);    /* E2BIG */
    return execve(__u_path, __u_v, environ);
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_execlp
static int execlp(const char *__u_file, const char *__u_a0, ...) {
    char *__u_v[256]; int __u_n; va_list __u_ap;
    __u_v[0] = (char *)__u_a0; __u_n = 1; va_start(__u_ap, __u_a0);
    while (__u_v[__u_n - 1] && __u_n < 256) { __u_v[__u_n] = va_arg(__u_ap, char *); __u_n = __u_n + 1; }
    va_end(__u_ap);
    if (__u_v[__u_n - 1]) return (int)_unisa_ret(0 - 7);
    return execvp(__u_file, __u_v);
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_execle
static int execle(const char *__u_path, const char *__u_a0, ...) {
    char *__u_v[256]; char **__u_e; int __u_n; va_list __u_ap;
    __u_v[0] = (char *)__u_a0; __u_n = 1; va_start(__u_ap, __u_a0);
    while (__u_v[__u_n - 1] && __u_n < 256) { __u_v[__u_n] = va_arg(__u_ap, char *); __u_n = __u_n + 1; }
    if (__u_v[__u_n - 1]) { va_end(__u_ap); return (int)_unisa_ret(0 - 7); }
    __u_e = va_arg(__u_ap, char **); va_end(__u_ap);
    return execve(__u_path, __u_v, __u_e);
}
#endif
#endif
/* sleep / usleep over the shared nanosleep primitive (sys/_timespec.h; Sleep on Windows) */

/* sys/_timespec.h -- 0.0.20 R20-6 (dsh): struct timespec and the one sleep
 * primitive that <time.h> nanosleep and <unistd.h> sleep/usleep share.
 * Linux: the nanosleep system call.  macOS has no nanosleep call, so the
 * wait is select(0, 0, 0, 0, &tv) -- microsecond granularity, stated. */
#ifndef _UNISA_TIMESPEC_H
#define _UNISA_TIMESPEC_H

#ifndef _UNISA_TIME_T
#define _UNISA_TIME_T
typedef __u_word time_t;
#endif
#ifndef _UNISA_TIMESPEC
#define _UNISA_TIMESPEC
struct timespec { __u_word tv_sec; long tv_nsec; };   /* one definition with sys/stat.h */
#endif

#if !__UNISA_FTRIM_LIBC || __UN__unisa_nanosleep
static long _unisa_nanosleep(const struct timespec *__u_q) {
#ifdef _WIN32
    static __u_word __u_f;                        /* Sleep(ms), rounded up */
    if (!__u_f) __u_f = _ux_sym("Sleep");
    _ux_call(__u_f, __u_q->tv_sec * 1000 + (__u_q->tv_nsec + 999999) / 1000000, 0, 0, 0);
    return 0;
#elif defined(__APPLE__)
    __u_word __u_tv[2];
    __u_tv[0] = __u_q->tv_sec; __u_tv[1] = (__u_q->tv_nsec + 999) / 1000;
#ifdef __x86_64__
    return __syscall6(0x2000000L + 93, 0, 0, 0, 0, (__u_word)__u_tv);
#else
    return __syscall6(93, 0, 0, 0, 0, (__u_word)__u_tv);
#endif
#elif defined(__x86_64__)
    return __syscall6(35, (__u_word)__u_q, 0, 0, 0, 0);
#else
    return __syscall6(101, (__u_word)__u_q, 0, 0, 0, 0);
#endif
}
#endif
#endif

#if !__UNISA_FTRIM_LIBC || __UN_sleep
static unsigned sleep(unsigned __u_s) { struct timespec __u_q; __u_q.tv_sec = __u_s; __u_q.tv_nsec = 0; _unisa_nanosleep(&__u_q); return 0; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_usleep
static int usleep(unsigned __u_us) { struct timespec __u_q; __u_q.tv_sec = __u_us / 1000000; __u_q.tv_nsec = (long)(__u_us % 1000000) * 1000; return (int)_unisa_ret(_unisa_nanosleep(&__u_q)); }
#endif
#ifndef _WIN32
#if !__UNISA_FTRIM_LIBC || __UN_dup2
static int dup2(int __u_old, int __u_new) {
#if defined(__APPLE__) || defined(__x86_64__)
    return (int)_unisa_ret(__syscall6(_UNISA_SC(_UNISA_NR_dup2), __u_old, __u_new, 0, 0, 0));
#else
    if (__u_old == __u_new) { __u_word __u_r; __u_r = __fcntl(__u_old, 1, 0); return __u_r < 0 ? (int)_unisa_ret(__u_r) : __u_new; }   /* F_GETFD: is OLD a valid descriptor */
    return (int)_unisa_ret(__syscall6(_UNISA_NR_dup3, __u_old, __u_new, 0, 0, 0));
#endif
}
#endif
#endif
/* Windows: pipe is CreatePipe (two HANDLEs, not inheritable).  dup is
   DuplicateHandle (below); dup2 stays refused by name there: a HANDLE cannot be
   placed at a chosen number. */
#if !__UNISA_FTRIM_LIBC || __UN_pipe
static int pipe(int __u_fds[2]) {
#ifdef _WIN32
    static __u_word __u_cp; __u_word __u_h[2];
    if (!__u_cp) __u_cp = _ux_sym("CreatePipe");
    __u_h[0] = 0; __u_h[1] = 0;
    if (!(_ux_call(__u_cp, (__u_word)&__u_h[0], (__u_word)&__u_h[1], 0, 0) & 0xFFFFFFFFL)) return _ux_fail();
    __u_fds[0] = (int)__u_h[0]; __u_fds[1] = (int)__u_h[1];
    return 0;
#elif defined(__APPLE__)
    return (int)_unisa_ret(__syscall6(_UNISA_SC(135), 1, 1, 0, (__u_word)__u_fds, 0));   /* socketpair(AF_UNIX, SOCK_STREAM) */
#else
    return (int)_unisa_ret(__syscall6(_UNISA_NR_pipe2, (__u_word)__u_fds, 0, 0, 0, 0));
#endif
}
#endif
#define F_OK 0
#define X_OK 1
#define W_OK 2
#define R_OK 4
#ifndef _WIN32
#ifdef __APPLE__
#define _UNISA_NR_fsync 95
#define _UNISA_NR_dup 41
#define _UNISA_NR_rmdir 137
#define _UNISA_NR_access 33
#define _UNISA_NR_readlink 58
#define _UNISA_NR_symlink 57
#elif defined(__x86_64__)
#define _UNISA_NR_fsync 74
#define _UNISA_NR_dup 32
#define _UNISA_NR_rmdir 84
#define _UNISA_NR_access 21
#define _UNISA_NR_readlink 89
#define _UNISA_NR_symlink 88
#else
#define _UNISA_NR_fsync 82
#define _UNISA_NR_dup 23
#define _UNISA_NR_unlinkat 35
#define _UNISA_NR_faccessat 48
#define _UNISA_NR_readlinkat 78
#define _UNISA_NR_symlinkat 36
#endif
#endif
#if !__UNISA_FTRIM_LIBC || __UN_fsync
static int fsync(int __u_fd) {
#ifdef _WIN32
    static __u_word __u_ff;
    if (!__u_ff) __u_ff = _ux_sym("FlushFileBuffers");
    if (!(_ux_call(__u_ff, (__u_word)__u_fd, 0, 0, 0) & 0xFFFFFFFFL)) return _ux_fail();
    return 0;
#else
    return (int)_unisa_ret(__syscall6(_UNISA_SC(_UNISA_NR_fsync), __u_fd, 0, 0, 0, 0));
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_dup
static int dup(int __u_fd) {
#ifdef _WIN32
    /* 0.0.22 Windows POSIX batch 4: DuplicateHandle(self, h, self, &new, 0, FALSE,
       DUPLICATE_SAME_ACCESS) -- seven arguments, carried by the ten-slot host call. */
    static __u_word __u_dh, __u_gp; __u_word __u_a[10] = {0}; __u_word __u_h, __u_self;
    if (!__u_dh) { __u_dh = _ux_sym("DuplicateHandle"); __u_gp = _ux_sym("GetCurrentProcess"); }
    __u_self = _ux_call(__u_gp, 0, 0, 0, 0); __u_h = 0;
    __u_a[0] = __u_self; __u_a[1] = (__u_word)__u_fd; __u_a[2] = __u_self; __u_a[3] = (__u_word)&__u_h;
    __u_a[4] = 0; __u_a[5] = 0; __u_a[6] = 2;
    if (!(__hostcall(__u_dh, __u_a) & 0xFFFFFFFFL)) return _ux_fail();
    return (int)__u_h;
#else
    return (int)_unisa_ret(__syscall6(_UNISA_SC(_UNISA_NR_dup), __u_fd, 0, 0, 0, 0));
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_rmdir
static int rmdir(const char *__u_p) {
#ifdef _WIN32
    static __u_word __u_f;
    if (!__u_f) __u_f = _ux_sym("RemoveDirectoryA");
    if (_ux_call(__u_f, (__u_word)__u_p, 0, 0, 0) & 0xFFFFFFFFL) return 0;
    return _ux_fail();
#elif defined(__APPLE__) || defined(__x86_64__)
    return (int)_unisa_ret(__syscall6(_UNISA_SC(_UNISA_NR_rmdir), (__u_word)__u_p, 0, 0, 0, 0));
#else
    return (int)_unisa_ret(__syscall6(_UNISA_NR_unlinkat, -100, (__u_word)__u_p, 0x200, 0, 0));   /* AT_FDCWD, AT_REMOVEDIR */
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_access
static int access(const char *__u_p, int __u_mode) {
#ifdef _WIN32
    static __u_word __u_f; __u_word __u_a;            /* GetFileAttributesA; X_OK is existence */
    if (!__u_f) __u_f = _ux_sym("GetFileAttributesA");
    __u_a = _ux_call(__u_f, (__u_word)__u_p, 0, 0, 0) & 0xFFFFFFFFL;
    if (__u_a == 0xFFFFFFFFL) return _ux_fail();
    if ((__u_mode & W_OK) && (__u_a & 1) && !(__u_a & 0x10)) { errno = EACCES; return -1; }   /* READONLY file */
    return 0;
#elif defined(__APPLE__) || defined(__x86_64__)
    return (int)_unisa_ret(__syscall6(_UNISA_SC(_UNISA_NR_access), (__u_word)__u_p, __u_mode, 0, 0, 0));
#else
    return (int)_unisa_ret(__syscall6(_UNISA_NR_faccessat, -100, (__u_word)__u_p, __u_mode, 0, 0));
#endif
}
#endif
#ifndef _WIN32
#if !__UNISA_FTRIM_LIBC || __UN_readlink
static long readlink(const char *__u_p, char *__u_buf, size_t __u_n) {
#if defined(__APPLE__) || defined(__x86_64__)
    return _unisa_ret(__syscall6(_UNISA_SC(_UNISA_NR_readlink), (__u_word)__u_p, (__u_word)__u_buf, (__u_word)__u_n, 0, 0));
#else
    return _unisa_ret(__syscall6(_UNISA_NR_readlinkat, -100, (__u_word)__u_p, (__u_word)__u_buf, (__u_word)__u_n, 0));
#endif
}
#endif
/* 0.0.36 M1: the user and group ids are direct system calls (0.0.27 H1 forwarded them to the system
   C library, which a static unisacc.com on Linux cannot reach: minicon's loader calls geteuid) */
#ifdef __APPLE__
#define _UNISA_NR_getuid 24
#define _UNISA_NR_geteuid 25
#define _UNISA_NR_getgid 47
#define _UNISA_NR_getegid 43
#elif defined(__x86_64__)
#define _UNISA_NR_getuid 102
#define _UNISA_NR_geteuid 107
#define _UNISA_NR_getgid 104
#define _UNISA_NR_getegid 108
#else
#define _UNISA_NR_getuid 174
#define _UNISA_NR_geteuid 175
#define _UNISA_NR_getgid 176
#define _UNISA_NR_getegid 177
#endif
#if !__UNISA_FTRIM_LIBC || __UN_getuid
static uid_t getuid(void) { return (uid_t)__syscall6(_UNISA_SC(_UNISA_NR_getuid), 0, 0, 0, 0, 0); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_geteuid
static uid_t geteuid(void) { return (uid_t)__syscall6(_UNISA_SC(_UNISA_NR_geteuid), 0, 0, 0, 0, 0); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_getgid
static gid_t getgid(void) { return (gid_t)__syscall6(_UNISA_SC(_UNISA_NR_getgid), 0, 0, 0, 0, 0); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_getegid
static gid_t getegid(void) { return (gid_t)__syscall6(_UNISA_SC(_UNISA_NR_getegid), 0, 0, 0, 0, 0); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_ttyname
static char *ttyname(int __u_fd) {             /* 0.0.24 D2: the terminal's path, or 0 with errno */
    static char __u_tn[1024]; __u_word __u_r;
    if (!isatty(__u_fd)) return 0;
#ifdef __APPLE__
    __u_r = __fcntl(__u_fd, 50, (__u_word)__u_tn);  /* F_GETPATH */
    if (__u_r < 0) { errno = (int)(0 - __u_r); return 0; }
#else
    {   char __u_p[32]; int __u_k; int __u_d; char __u_dig[12];
        { const char *__u_s; __u_s = "/proc/self/fd/"; for (__u_k = 0; __u_k < 14; __u_k = __u_k + 1) __u_p[__u_k] = __u_s[__u_k]; } __u_k = 0; __u_d = __u_fd;
        do { __u_dig[__u_k] = (char)(48 + __u_d % 10); __u_d = __u_d / 10; __u_k = __u_k + 1; } while (__u_d);
        __u_d = 14; while (__u_k) { __u_k = __u_k - 1; __u_p[__u_d] = __u_dig[__u_k]; __u_d = __u_d + 1; } __u_p[__u_d] = 0;
        __u_r = readlink(__u_p, __u_tn, sizeof(__u_tn) - 1);
        if (__u_r < 0) return 0;
        __u_tn[__u_r] = 0; }
#endif
    return __u_tn;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_symlink
static int symlink(const char *__u_old, const char *__u_new) {
#if defined(__APPLE__) || defined(__x86_64__)
    return (int)_unisa_ret(__syscall6(_UNISA_SC(_UNISA_NR_symlink), (__u_word)__u_old, (__u_word)__u_new, 0, 0, 0));
#else
    return (int)_unisa_ret(__syscall6(_UNISA_NR_symlinkat, (__u_word)__u_old, -100, (__u_word)__u_new, 0, 0));
#endif
}
#endif
#endif
#if !__UNISA_FTRIM_LIBC || __UN_chdir
static int chdir(const char *__u_p) {
#ifdef _WIN32
    static __u_word __u_f;
    if (!__u_f) __u_f = _ux_sym("SetCurrentDirectoryA");
    if (_ux_call(__u_f, (__u_word)__u_p, 0, 0, 0) & 0xFFFFFFFFL) return 0;
    return _ux_fail();
#elif defined(__APPLE__)
    return (int)_unisa_ret(__syscall6(_UNISA_SC(12), (__u_word)__u_p, 0, 0, 0, 0));
#elif defined(__x86_64__)
    return (int)_unisa_ret(__syscall6(80, (__u_word)__u_p, 0, 0, 0, 0));
#else
    return (int)_unisa_ret(__syscall6(49, (__u_word)__u_p, 0, 0, 0, 0));
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_getcwd
static char *getcwd(char *__u_buf, size_t __u_size) {
#ifdef _WIN32
    static __u_word __u_f; __u_word __u_n;            /* GetCurrentDirectoryA (backslashes kept) */
    if (!__u_f) __u_f = _ux_sym("GetCurrentDirectoryA");
    __u_n = _ux_call(__u_f, (__u_word)__u_size, (__u_word)__u_buf, 0, 0) & 0xFFFFFFFFL;
    if (__u_n == 0) { _ux_fail(); return 0; }
    if ((size_t)__u_n >= __u_size) { errno = ERANGE; return 0; }
    return __u_buf;
#elif defined(__APPLE__)
    char __u_tmp[1024]; __u_word __u_fd; __u_word __u_r; size_t __u_n;
    __u_fd = __open(".", 0x100000, 0);           /* O_RDONLY | O_DIRECTORY */
    if (__u_fd < 0) { errno = (int)(0 - __u_fd); return 0; }
    __u_r = __fcntl(__u_fd, 50, (__u_word)__u_tmp);  /* F_GETPATH (needs MAXPATHLEN bytes) */
    __close(__u_fd);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return 0; }
    __u_n = 0; while (__u_tmp[__u_n]) __u_n = __u_n + 1;
    if (__u_n + 1 > __u_size) { errno = 34; return 0; }   /* ERANGE */
    __u_n = 0; while ((__u_buf[__u_n] = __u_tmp[__u_n]) != 0) __u_n = __u_n + 1;
    return __u_buf;
#else
    __u_word __u_r; __u_r = __syscall6(_UNISA_NR_getcwd, (__u_word)__u_buf, (__u_word)__u_size, 0, 0, 0);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return 0; }
    return __u_buf;
#endif
}
#endif
/* 0.0.28 H4 (SQLite's unix VFS): forwarded to the system C library on Linux and macOS */
#ifndef _WIN32
ssize_t pread(int __u_fd, void *__u_buf, size_t __u_n, off_t __u_off);
ssize_t pwrite(int __u_fd, const void *__u_buf, size_t __u_n, off_t __u_off);
int fchown(int __u_fd, uid_t __u_uid, gid_t __u_gid);
/* 0.0.34 X1 (cx.h cx_run_timeout kills the whole group): forwarded */
int setpgid(pid_t __u_pid, pid_t __u_pgid);
pid_t getpgrp(void);
long sysconf(int __u_name);
#ifdef __APPLE__
#define _SC_PAGESIZE 29
#else
#define _SC_PAGESIZE 30
#endif
#define _SC_PAGE_SIZE _SC_PAGESIZE
ssize_t readlink(const char *__u_path, char *__u_buf, size_t __u_n);
#endif
/* 0.0.34 L2 (SQLite's Apple locking style): forwarded */
#ifdef __APPLE__
typedef unsigned char uuid_t[16];
int fsctl(const char *__u_path, unsigned long __u_req, void *__u_data, unsigned int __u_opt);
int gethostuuid(uuid_t __u_id, const struct timespec *__u_wait);
#endif
#endif


/* <signal.h> for the unisa C subset -- the in-process half of it.
 * `signal` records a handler and `raise` calls it (and leaves it installed,
 * as BSD and glibc do); SIG_DFL for the signals
 * whose default is to terminate ends the process with the shell's status
 * for that signal (128 + n), which is what a program that raises SIGABRT
 * and is run from a shell observes.  No signal ever arrives from OUTSIDE:
 * there is no sigaction system call here, so a handler installed for
 * SIGINT is never called by Ctrl-C.  That is stated rather than faked. */
#ifndef _UNISA_SIGNAL_H
#define _UNISA_SIGNAL_H

typedef int sig_atomic_t;
#define SIG_DFL ((void (*)(int))0)
#define SIG_IGN ((void (*)(int))1)
#define SIG_ERR ((void (*)(int))(0-1))
#define SIGINT  2
#define SIGILL  4
#define SIGABRT 6
#define SIGFPE  8
#define SIGSEGV 11
#define SIGHUP  1
#define SIGQUIT 3
#define SIGKILL 9
#define SIGTERM 15
#define SIGWINCH 28                 /* same number on Linux and macOS; recorded, never delivered (above) */
#define _UNISA_NSIG 32
static void (*_unisa_sig[_UNISA_NSIG])(int);

#if !__UNISA_FTRIM_LIBC || __UN_signal
static void (*signal(int __u_sig, void (*__u_fn)(int)))(int) {
    void (*__u_old)(int);
    if (__u_sig <= 0 || __u_sig >= _UNISA_NSIG) return SIG_ERR;
    __u_old = _unisa_sig[__u_sig];
    _unisa_sig[__u_sig] = __u_fn;
    return __u_old;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_raise
static int raise(int __u_sig) {
    void (*__u_fn)(int);
    if (__u_sig <= 0 || __u_sig >= _UNISA_NSIG) return 0 - 1;
    __u_fn = _unisa_sig[__u_sig];
    if (__u_fn == SIG_IGN) return 0;
    if (__u_fn != SIG_DFL) {
        __u_fn(__u_sig);
        return 0;
    }
    __exit(128 + __u_sig);
    return 0;
}
#endif

/* sigaction and the signal sets (0.0.34 L1b, for lua built with LUA_USE_POSIX).
 * They share signal()'s in-process table, so the same rule holds: a handler is
 * recorded and raise() calls it; nothing arrives from outside.  The mask and
 * flags are kept for the caller to read back and otherwise have no effect;
 * sigprocmask answers the empty set. */

typedef __u_uword sigset_t;
struct sigaction {
    void (*sa_handler)(int);
    sigset_t sa_mask;
    int sa_flags;
};
#define SA_RESTART   2
#define SA_RESETHAND 4
#define SA_NODEFER   16
#define SIG_BLOCK    1
#define SIG_UNBLOCK  2
#define SIG_SETMASK  3
#if !__UNISA_FTRIM_LIBC || __UN_sigemptyset
static int sigemptyset(sigset_t *__u_s) { *__u_s = 0; return 0; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_sigfillset
static int sigfillset(sigset_t *__u_s) { *__u_s = ~(sigset_t)0; return 0; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_sigaddset
static int sigaddset(sigset_t *__u_s, int __u_sig) {
    if (__u_sig <= 0 || __u_sig >= _UNISA_NSIG) { errno = EINVAL; return 0 - 1; }
    *__u_s = *__u_s | ((sigset_t)1 << __u_sig); return 0;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_sigdelset
static int sigdelset(sigset_t *__u_s, int __u_sig) {
    if (__u_sig <= 0 || __u_sig >= _UNISA_NSIG) { errno = EINVAL; return 0 - 1; }
    *__u_s = *__u_s & ~((sigset_t)1 << __u_sig); return 0;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_sigismember
static int sigismember(const sigset_t *__u_s, int __u_sig) {
    if (__u_sig <= 0 || __u_sig >= _UNISA_NSIG) { errno = EINVAL; return 0 - 1; }
    return (int)((*__u_s >> __u_sig) & 1);
}
#endif
static sigset_t _unisa_sigmask[_UNISA_NSIG];
static int _unisa_sigflags[_UNISA_NSIG];
#if !__UNISA_FTRIM_LIBC || __UN_sigaction
static int sigaction(int __u_sig, const struct sigaction *__u_act, struct sigaction *__u_old) {
    if (__u_sig <= 0 || __u_sig >= _UNISA_NSIG || ((__u_sig == SIGKILL) && __u_act)) { errno = EINVAL; return 0 - 1; }
    if (__u_old) {
        __u_old->sa_handler = _unisa_sig[__u_sig];
        __u_old->sa_mask = _unisa_sigmask[__u_sig];
        __u_old->sa_flags = _unisa_sigflags[__u_sig];
    }
    if (__u_act) {
        _unisa_sig[__u_sig] = __u_act->sa_handler;
        _unisa_sigmask[__u_sig] = __u_act->sa_mask;
        _unisa_sigflags[__u_sig] = __u_act->sa_flags;
    }
    return 0;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_sigprocmask
static int sigprocmask(int __u_how, const sigset_t *__u_set, sigset_t *__u_old) {
    if (__u_old) *__u_old = 0;
    if (__u_set && (__u_how < SIG_BLOCK || __u_how > SIG_SETMASK)) { errno = EINVAL; return 0 - 1; }
    return 0;
}
#endif

/* kill (0.0.24, cdsh request): send a signal to another process.  POSIX targets
 * use the kernel call; signal 0 only checks that the process exists.  Windows
 * (pid = the posix_spawn HANDLE, as in waitpid): any nonzero signal ends the
 * process with exit code 128 + sig, which waitpid then reports; there is no
 * delivery to a handler there. */



#if !__UNISA_FTRIM_LIBC || __UN_kill
static int kill(pid_t __u_pid, int __u_sig) {
#ifdef _WIN32
    static __u_word __u_t;
    if (__u_pid <= 0 || __u_sig < 0 || __u_sig >= _UNISA_NSIG) { errno = EINVAL; return -1; }
    if (__u_sig == 0) return 0;
    if (!__u_t) __u_t = _ux_sym("TerminateProcess");
    if (!(_ux_call(__u_t, (__u_word)__u_pid, 128 + __u_sig, 0, 0) & 0xFFFFFFFFL)) return (int)_ux_fail();
    return 0;
#elif defined(__APPLE__)
#if defined(__x86_64__)
    return (int)_unisa_ret(__syscall6(0x2000000L + 37, (__u_word)__u_pid, (__u_word)__u_sig, 1, 0, 0));
#else
    return (int)_unisa_ret(__syscall6(37, (__u_word)__u_pid, (__u_word)__u_sig, 1, 0, 0));
#endif
#elif defined(__x86_64__)
    return (int)_unisa_ret(__syscall6(62, (__u_word)__u_pid, (__u_word)__u_sig, 0, 0, 0));
#else
    return (int)_unisa_ret(__syscall6(129, (__u_word)__u_pid, (__u_word)__u_sig, 0, 0, 0));
#endif
}
#endif
#endif


/* sys/wait.h -- 0.0.19 R19-5: waitpid/wait over wait4 (generic gate), and the
 * status macros (the same encoding on Linux and macOS).  Windows: waitpid on a posix_spawn HANDLE. */
#ifndef _UNISA_SYS_WAIT_H
#define _UNISA_SYS_WAIT_H




#define WNOHANG 1
#define WUNTRACED 2
#define WIFEXITED(s) (((s) & 0x7f) == 0)
#define WEXITSTATUS(s) (((s) >> 8) & 0xff)
#define WIFSIGNALED(s) (((s) & 0x7f) != 0 && ((s) & 0x7f) != 0x7f)
#define WTERMSIG(s) ((s) & 0x7f)
#define WIFSTOPPED(s) (((s) & 0xff) == 0x7f)
#define WSTOPSIG(s) (((s) >> 8) & 0xff)
#if !defined(_WIN32) || !defined(_UNISA_NO_HOSTCALL)
#if !__UNISA_FTRIM_LIBC || __UN_waitpid
static pid_t waitpid(pid_t __u_pid, int *__u_st, int __u_opt) {
#ifdef _WIN32
    /* Windows (0.0.22): a pid is the process HANDLE from posix_spawn; the exit code is
       encoded as an exited status.  pid -1 has no meaning here and answers ECHILD. */
    static __u_word __u_w, __u_x, __u_c; unsigned int __u_code;
    if (__u_pid <= 0) { errno = ECHILD; return -1; }
    if (!__u_w) { __u_w = _ux_sym("WaitForSingleObject"); __u_x = _ux_sym("GetExitCodeProcess"); __u_c = _ux_sym("CloseHandle"); }
    if ((_ux_call(__u_w, (__u_word)__u_pid, (__u_opt & WNOHANG) ? 0 : 0xFFFFFFFFL, 0, 0) & 0xFFFFFFFFL) == 0x102) return 0;
    __u_code = 0;
    if (!(_ux_call(__u_x, (__u_word)__u_pid, (__u_word)&__u_code, 0, 0) & 0xFFFFFFFFL)) return _ux_fail();
    _ux_call(__u_c, (__u_word)__u_pid, 0, 0, 0);
    if (__u_st) *__u_st = (int)((__u_code & 0xff) << 8);
    return __u_pid;
#else
    return (pid_t)_unisa_ret(__syscall6(_UNISA_SC(_UNISA_NR_wait4), (__u_word)__u_pid, (__u_word)__u_st, (__u_word)__u_opt, 0, 0));
#endif
}
#endif
#endif
#ifndef _WIN32
#if !__UNISA_FTRIM_LIBC || __UN_wait
static pid_t wait(int *__u_st) { return waitpid(-1, __u_st, 0); }
#endif
#endif
#endif


/* Minimal directory API: Linux getdents64, macOS getdirentries64 (0.0.18); Windows
 * FindFirstFileA/FindNextFileA/FindClose (0.0.21 batch 2: "." and ".." are
 * returned there too, in the system's order -- portable code sorts or filters).
 * EOF sets errno=0; malformed kernel records set EIO and stop this stream. */
#ifndef _UNISA_DIRENT_H
#define _UNISA_DIRENT_H




struct dirent {
    __u_uword d_ino;
    unsigned char d_type;
    char d_name[256];
};
#define _UNISA_DIRBUF 4096
#ifdef __APPLE__
#define _UNISA_DIRNAME 21
#else
#define _UNISA_DIRNAME 19
#endif
typedef struct {
    int fd, pos, len, failed;
    __u_word basep;                                  /* macOS getdirentries64 position */
    long index;                                  /* entries returned so far (telldir) */
    struct dirent ent;
    unsigned char buf[_UNISA_DIRBUF];
} DIR;
/* Windows layout inside DIR: fd unused, basep = the find handle (0 = none yet,
   -1 = exhausted), buf[0..319] = WIN32_FIND_DATAA (cFileName at 44),
   buf[512..] = the "path\\*" pattern kept for rewinddir. */
#if !__UNISA_FTRIM_LIBC || __UN_opendir
static DIR *opendir(const char *__u_path) {
    DIR *__u_d; __u_word __u_fd;
#ifdef _WIN32
    static __u_word __u_ff; static __u_word __u_fc; __u_word __u_h; int __u_n;
    if (!__u_ff) { __u_ff = _ux_sym("FindFirstFileA"); __u_fc = _ux_sym("FindClose"); }
    for (__u_n = 0; __u_path[__u_n]; __u_n++) ;
    if (__u_n == 0) { errno = ENOENT; return 0; }
    if (__u_n > 3000) { errno = ENAMETOOLONG; return 0; }
    __u_d = (DIR *)malloc(sizeof(DIR));
    if (__u_d == 0) { errno = ENOMEM; return 0; }
    for (__u_fd = 0; __u_fd < __u_n; __u_fd++) __u_d->buf[512 + __u_fd] = (unsigned char)__u_path[__u_fd];
    if (__u_path[__u_n - 1] != '\\' && __u_path[__u_n - 1] != '/') __u_d->buf[512 + __u_n++] = '\\';
    __u_d->buf[512 + __u_n] = '*'; __u_d->buf[513 + __u_n] = 0;
    __u_h = _ux_call(__u_ff, (__u_word)(__u_d->buf + 512), (__u_word)__u_d->buf, 0, 0);
    if (__u_h == -1 || __u_h == 0) { __u_fd = _ux_errno(); free(__u_d); errno = (int)__u_fd; return 0; }
    if (!(__u_d->buf[0] & 0x10)) { _ux_call(__u_fc, __u_h, 0, 0, 0); free(__u_d); errno = ENOTDIR; return 0; }  /* "x\\*" of a file */
    __u_d->fd = -1; __u_d->pos = 1; __u_d->len = 0; __u_d->failed = 0; __u_d->basep = __u_h; __u_d->index = 0;
    errno = 0; return __u_d;
#else
#ifdef __APPLE__
    __u_fd = __open((char *)__u_path, 0x100000, 0); /* macOS O_DIRECTORY */
#else
#ifdef __aarch64__
    __u_fd = __open((char *)__u_path, 16384, 0); /* Linux arm64 O_DIRECTORY */
#else
    __u_fd = __open((char *)__u_path, 65536, 0); /* Linux x86_64 O_DIRECTORY */
#endif
#endif
    if (__u_fd < 0) { errno = (int)(0 - __u_fd); return 0; }
    __u_d = (DIR *)malloc(sizeof(DIR));
    if (__u_d == 0) { __close(__u_fd); errno = ENOMEM; return 0; }
    __u_d->fd = (int)__u_fd; __u_d->pos = 0; __u_d->len = 0; __u_d->failed = 0; __u_d->basep = 0; __u_d->index = 0;
    errno = 0; return __u_d;
#endif
}
#endif
/* fdopendir/dirfd (POSIX, 0.0.35 M1): the stream takes over FD, which must be an open directory */
#if !__UNISA_FTRIM_LIBC || __UN_fdopendir
static DIR *fdopendir(int __u_fd) {
    DIR *__u_d;
#ifdef _WIN32
    (void)__u_fd; errno = ENOSYS; return 0;
#else
    if (__u_fd < 0) { errno = EBADF; return 0; }
    __u_d = (DIR *)malloc(sizeof(DIR));
    if (__u_d == 0) { errno = ENOMEM; return 0; }
    __u_d->fd = __u_fd; __u_d->pos = 0; __u_d->len = 0; __u_d->failed = 0; __u_d->basep = 0; __u_d->index = 0;
    return __u_d;
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_dirfd
static int dirfd(DIR *__u_d) { if (!__u_d || __u_d->fd < 0) { errno = EINVAL; return -1; } return __u_d->fd; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_readdir
static struct dirent *readdir(DIR *__u_d) {
    unsigned char *__u_r; int __u_rl, __u_i, __u_left; __u_word __u_n;
    errno = 0;
    if (__u_d == 0) { errno = EBADF; return 0; }
    if (__u_d->failed) { errno = __u_d->failed; return 0; }
#ifdef _WIN32
    {   static __u_word __u_fn; static __u_word __u_ff; __u_word __u_e;
        if (!__u_fn) { __u_fn = _ux_sym("FindNextFileA"); __u_ff = _ux_sym("FindFirstFileA"); }
        if (__u_d->basep == -1) return 0;
        if (__u_d->basep == 0) {                     /* after rewinddir */
            __u_d->basep = _ux_call(__u_ff, (__u_word)(__u_d->buf + 512), (__u_word)__u_d->buf, 0, 0);
            if (__u_d->basep == -1 || __u_d->basep == 0) { __u_d->basep = -1; return 0; }
        } else if (!__u_d->pos) {
            if (!(_ux_call(__u_fn, __u_d->basep, (__u_word)__u_d->buf, 0, 0) & 0xFFFFFFFFL)) {
                __u_e = _ux_errno(); errno = 0;
                if (__u_e != EIO) { __u_d->failed = (int)__u_e; errno = (int)__u_e; }   /* ERROR_NO_MORE_FILES maps to EIO: the end */
                return 0;
            }
        }
        __u_d->pos = 0;
        for (__u_i = 0; __u_i < 255 && __u_d->buf[44 + __u_i]; __u_i++) __u_d->ent.d_name[__u_i] = (char)__u_d->buf[44 + __u_i];
        __u_d->ent.d_name[__u_i] = 0;
        __u_d->ent.d_ino = 0; __u_d->ent.d_type = (__u_d->buf[0] & 0x10) ? 4 : 8;   /* DT_DIR / DT_REG */
        __u_d->index = __u_d->index + 1;
        return &__u_d->ent;
    }
#else
    if (__u_d->pos == __u_d->len) {
#ifdef __APPLE__
        __u_n = __getdirentries64(__u_d->fd, __u_d->buf, _UNISA_DIRBUF, (char *)&__u_d->basep, 0, 0);   /* R18-10 */
#else
        __u_n = __getdents64(__u_d->fd, __u_d->buf, _UNISA_DIRBUF);
#endif
        if (__u_n < 0) { __u_d->failed = (int)(0 - __u_n); errno = __u_d->failed; return 0; }
        if (__u_n == 0) return 0;
        if (__u_n > _UNISA_DIRBUF) { __u_d->failed = EIO; errno = EIO; return 0; }
        __u_d->len = (int)__u_n; __u_d->pos = 0;
    }
    __u_left = __u_d->len - __u_d->pos;
    if (__u_left < 24) { __u_d->failed = EIO; errno = EIO; return 0; }
    __u_r = __u_d->buf + __u_d->pos;
    __u_rl = __u_r[16] | (__u_r[17] << 8);
    /* linux_dirent64: d_type at 18, name at 19, records 8-aligned; macOS
       struct direntry: d_namlen at 18, d_type at 20, name at 21, 4-aligned */
    if (__u_rl < 24 || (__u_rl & (_UNISA_DIRNAME == 21 ? 3 : 7)) || __u_rl > __u_left) {
        __u_d->failed = EIO; errno = EIO; return 0;
    }
    for (__u_i = 0; __u_i < 256 && _UNISA_DIRNAME + __u_i < __u_rl && __u_r[_UNISA_DIRNAME + __u_i]; __u_i++) ;
    if (__u_i == 0 || __u_i >= 256 || _UNISA_DIRNAME + __u_i >= __u_rl) {
        __u_d->failed = EIO; errno = EIO; return 0;
    }
    __u_d->ent.d_name[__u_i] = 0;
    while (__u_i > 0) { __u_i--; __u_d->ent.d_name[__u_i] = (char)__u_r[_UNISA_DIRNAME + __u_i]; }
    __u_d->ent.d_ino = 0;
    for (__u_i = 7; __u_i >= 0; __u_i--)
        __u_d->ent.d_ino = (__u_d->ent.d_ino << 8) | __u_r[__u_i];
    __u_d->ent.d_type = __u_r[_UNISA_DIRNAME - 1]; __u_d->pos += __u_rl; __u_d->index = __u_d->index + 1;
    return &__u_d->ent;
#endif
}
#endif
/* 0.0.19 (dsh): rewinddir/telldir/seekdir.  A position is the number of
   entries already returned; seekdir rewinds and reads forward to it (POSIX
   only promises telldir's value is meaningful to seekdir on the same DIR). */
#if !__UNISA_FTRIM_LIBC || __UN_rewinddir
static void rewinddir(DIR *__u_d) {
    if (__u_d == 0) return;
#ifdef _WIN32
    {   static __u_word __u_fc;
        if (!__u_fc) __u_fc = _ux_sym("FindClose");
        if (__u_d->basep != -1 && __u_d->basep != 0) _ux_call(__u_fc, __u_d->basep, 0, 0, 0);
        __u_d->basep = 0; __u_d->pos = 1; __u_d->failed = 0; __u_d->index = 0;
        return;
    }
#else
    __lseek(__u_d->fd, 0, 0);
#endif
    __u_d->pos = 0; __u_d->len = 0; __u_d->failed = 0; __u_d->basep = 0; __u_d->index = 0;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_telldir
static long telldir(DIR *__u_d) { return __u_d ? __u_d->index : -1; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_seekdir
static void seekdir(DIR *__u_d, long __u_loc) {
    if (__u_d == 0) return;
    rewinddir(__u_d);
    while (__u_d->index < __u_loc) { if (readdir(__u_d) == 0) break; }
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_closedir
static int closedir(DIR *__u_d) {
    __u_word __u_r;
    if (__u_d == 0) { errno = EBADF; return -1; }
#ifdef _WIN32
    {   static __u_word __u_fc;
        if (!__u_fc) __u_fc = _ux_sym("FindClose");
        __u_r = 0;
        if (__u_d->basep != -1 && __u_d->basep != 0) __u_r = _ux_call(__u_fc, __u_d->basep, 0, 0, 0) & 0xFFFFFFFFL;
        else __u_r = 1;
        free(__u_d);
        if (!__u_r) { errno = EBADF; return -1; }
        return 0;
    }
#else
    __u_r = __close(__u_d->fd); free(__u_d);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; }
    return 0;
#endif
}
#endif
#endif


/* <sys/stat.h> (0.0.18 R18-10): struct stat and stat/lstat/fstat/chmod/mkdir
 * over the kernel's own calls -- no host libc.  The layout is the kernel's for
 * each target (Linux x86_64 and arm64 differ; macOS is stat64).  Windows
 * (0.0.21 batch 1): a __u_word-field struct stat and stat/fstat/mkdir over
 * kernel32 (sys/_win.h); lstat is stat there (no symlinks followed or not),
 * chmod maps S_IWUSR to the read-only attribute (0.0.21 batch 2). */
#ifndef _UNISA_SYS_STAT_H
#define _UNISA_SYS_STAT_H




#define S_IFMT   0170000
#define S_IFIFO  0010000
#define S_IFCHR  0020000
#define S_IFDIR  0040000
#define S_IFBLK  0060000
#define S_IFREG  0100000
#define S_IFLNK  0120000
#define S_IFSOCK 0140000
#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)
#define S_ISLNK(m)  (((m) & S_IFMT) == S_IFLNK)
#define S_ISCHR(m)  (((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)  (((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)
#define S_IRWXU 0700
#define S_IRUSR 0400
#define S_IWUSR 0200
#define S_IXUSR 0100
#define S_IRWXG 070
#define S_IRGRP 040
#define S_IWGRP 020
#define S_IXGRP 010
#define S_IRWXO 07
#define S_IROTH 04
#define S_IWOTH 02
#define S_IXOTH 01
#ifndef _UNISA_TIMESPEC
#define _UNISA_TIMESPEC
struct timespec { __u_word tv_sec; long tv_nsec; };
#endif
#ifdef _WIN32
struct stat {
    __u_word st_dev; __u_word st_ino; __u_word st_mode; __u_word st_nlink; __u_word st_uid; __u_word st_gid;
    __u_word st_rdev; __u_word st_size; __u_word st_atime; __u_word st_mtime; __u_word st_ctime;
};
#elif defined(__APPLE__)
struct stat {                                   /* struct stat64 */
    int st_dev; unsigned short st_mode; unsigned short st_nlink; __u_uword st_ino;
    unsigned int st_uid; unsigned int st_gid; int st_rdev;
    struct timespec st_atimespec, st_mtimespec, st_ctimespec, st_birthtimespec;
    __u_word st_size; __u_word st_blocks; int st_blksize; unsigned int st_flags; unsigned int st_gen;
    int st_lspare; __u_word st_qspare[2];
};
#define st_atime st_atimespec.tv_sec
#define st_mtime st_mtimespec.tv_sec
#define st_ctime st_ctimespec.tv_sec
#else
#ifdef __aarch64__
struct stat {                                   /* asm-generic */
    __u_uword st_dev; __u_uword st_ino; unsigned int st_mode; unsigned int st_nlink;
    unsigned int st_uid; unsigned int st_gid; __u_uword st_rdev; __u_uword __u_pad1;
    __u_word st_size; int st_blksize; int __u_pad2; __u_word st_blocks;
    __u_word st_atime; __u_word st_atime_nsec; __u_word st_mtime; __u_word st_mtime_nsec; __u_word st_ctime; __u_word st_ctime_nsec;
    unsigned int __u_unused[2];
};
#else
struct stat {                                   /* x86_64 */
    __u_uword st_dev; __u_uword st_ino; __u_uword st_nlink;
    unsigned int st_mode; unsigned int st_uid; unsigned int st_gid; int __u_pad0;
    __u_uword st_rdev; __u_word st_size; __u_word st_blksize; __u_word st_blocks;
    __u_word st_atime; __u_word st_atime_nsec; __u_word st_mtime; __u_word st_mtime_nsec; __u_word st_ctime; __u_word st_ctime_nsec;
    __u_word __u_unused[3];
};
#endif
#endif
#if !__UNISA_FTRIM_LIBC || __UN_stat
static int stat(const char *__u_p, struct stat *__u_b) {
#ifdef _WIN32
    static __u_word __u_f; unsigned int __u_d[9];   /* GetFileAttributesExA: WIN32_FILE_ATTRIBUTE_DATA */
    if (!__u_f) __u_f = _ux_sym("GetFileAttributesExA");
    if (!(_ux_call(__u_f, (__u_word)__u_p, 0, (__u_word)__u_d, 0) & 0xFFFFFFFFL)) return _ux_fail();
    __u_b->st_dev = 0; __u_b->st_ino = 0; __u_b->st_nlink = 1; __u_b->st_uid = 0; __u_b->st_gid = 0; __u_b->st_rdev = 0;
    __u_b->st_mode = (__u_d[0] & 0x10) ? (S_IFDIR | 0755) : (S_IFREG | ((__u_d[0] & 1) ? 0444 : 0644));
    __u_b->st_ctime = _ux_ft2unix(__u_d + 1); __u_b->st_atime = _ux_ft2unix(__u_d + 3); __u_b->st_mtime = _ux_ft2unix(__u_d + 5);
    __u_b->st_size = ((__u_word)__u_d[7] << 32) | (__u_word)__u_d[8];
    return 0;
#else
    __u_word __u_r; __u_r = __stat((char *)__u_p, (char *)__u_b, 0);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; } return 0;
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_lstat
static int lstat(const char *__u_p, struct stat *__u_b) {
#ifdef _WIN32
    return stat(__u_p, __u_b);
#else
    __u_word __u_r; __u_r = __lstat((char *)__u_p, (char *)__u_b, 256);   /* AT_SYMLINK_NOFOLLOW for newfstatat */
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; } return 0;
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_fstat
static int fstat(int __u_fd, struct stat *__u_b) {
#ifdef _WIN32
    static __u_word __u_ft; static __u_word __u_fi; __u_word __u_k; unsigned int __u_d[13];
    if (!__u_ft) { __u_ft = _ux_sym("GetFileType"); __u_fi = _ux_sym("GetFileInformationByHandle"); }
    __u_b->st_dev = 0; __u_b->st_ino = 0; __u_b->st_nlink = 1; __u_b->st_uid = 0; __u_b->st_gid = 0; __u_b->st_rdev = 0;
    __u_b->st_size = 0; __u_b->st_atime = 0; __u_b->st_mtime = 0; __u_b->st_ctime = 0;
    __u_k = _ux_call(__u_ft, (__u_word)__u_fd, 0, 0, 0) & 0xFFFFFFFFL;
    if (__u_k == 2) { __u_b->st_mode = S_IFCHR | 0666; return 0; }            /* FILE_TYPE_CHAR */
    if (__u_k == 3) { __u_b->st_mode = S_IFIFO | 0666; return 0; }            /* FILE_TYPE_PIPE */
    if (!(_ux_call(__u_fi, (__u_word)__u_fd, (__u_word)__u_d, 0, 0) & 0xFFFFFFFFL)) return _ux_fail();
    __u_b->st_mode = (__u_d[0] & 0x10) ? (S_IFDIR | 0755) : (S_IFREG | ((__u_d[0] & 1) ? 0444 : 0644));
    __u_b->st_ctime = _ux_ft2unix(__u_d + 1); __u_b->st_atime = _ux_ft2unix(__u_d + 3); __u_b->st_mtime = _ux_ft2unix(__u_d + 5);
    __u_b->st_dev = __u_d[7]; __u_b->st_size = ((__u_word)__u_d[8] << 32) | (__u_word)__u_d[9];
    __u_b->st_nlink = __u_d[10]; __u_b->st_ino = ((__u_word)__u_d[11] << 32) | (__u_word)__u_d[12];
    return 0;
#else
    __u_word __u_r; __u_r = __fstat(__u_fd, (char *)__u_b, 0);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; } return 0;
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_chmod
static int chmod(const char *__u_p, mode_t __u_m) {
#ifdef _WIN32
    static __u_word __u_ga; static __u_word __u_sa; __u_word __u_at;
    if (!__u_ga) { __u_ga = _ux_sym("GetFileAttributesA"); __u_sa = _ux_sym("SetFileAttributesA"); }
    __u_at = _ux_call(__u_ga, (__u_word)__u_p, 0, 0, 0) & 0xFFFFFFFFL;
    if (__u_at == 0xFFFFFFFFL) return _ux_fail();
    __u_at = (__u_m & 0200) ? (__u_at & ~1L) : (__u_at | 1L);         /* FILE_ATTRIBUTE_READONLY */
    if (!(_ux_call(__u_sa, (__u_word)__u_p, __u_at, 0, 0) & 0xFFFFFFFFL)) return _ux_fail();
    return 0;
#else
    __u_word __u_r; __u_r = __chmod((char *)__u_p, (__u_word)__u_m, 0);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; } return 0;
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_mkdir
static int mkdir(const char *__u_p, mode_t __u_m) {
#ifdef _WIN32
    static __u_word __u_f;                         /* CreateDirectoryA (the mode is not mapped) */
    if (!__u_f) __u_f = _ux_sym("CreateDirectoryA");
    if (_ux_call(__u_f, (__u_word)__u_p, 0, 0, 0) & 0xFFFFFFFFL) return 0;
    return _ux_fail();
#else
    __u_word __u_r; __u_r = __mkdir((char *)__u_p, (__u_word)__u_m, 0);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; } return 0;
#endif
}
#endif
/* *at() flags (0.0.35 M1, minicon's loader): each kernel's own values */
#ifndef AT_FDCWD
#ifdef __APPLE__
#define AT_FDCWD (0 - 2)
#define AT_SYMLINK_NOFOLLOW 0x20
#define AT_REMOVEDIR 0x80
#else
#define AT_FDCWD (0 - 100)
#define AT_SYMLINK_NOFOLLOW 0x100
#define AT_REMOVEDIR 0x200
#endif
#endif
#if !__UNISA_FTRIM_LIBC || __UN_fstatat
static int fstatat(int __u_dfd, const char *__u_p, struct stat *__u_b, int __u_fl) {
#ifdef _WIN32
    if (__u_dfd != AT_FDCWD) { errno = ENOSYS; return -1; }
    return stat(__u_p, __u_b);
#else
    __u_word __u_r;
#if defined(__APPLE__)
#if defined(__x86_64__)
    __u_r = __syscall6(0x2000000L + 470, (__u_word)__u_dfd, (__u_word)__u_p, (__u_word)__u_b, (__u_word)__u_fl, 0);
#else
    __u_r = __syscall6(470, (__u_word)__u_dfd, (__u_word)__u_p, (__u_word)__u_b, (__u_word)__u_fl, 0);
#endif
#elif defined(__x86_64__)
    __u_r = __syscall6(262, (__u_word)__u_dfd, (__u_word)__u_p, (__u_word)__u_b, (__u_word)__u_fl, 0);
#else
    __u_r = __syscall6(79, (__u_word)__u_dfd, (__u_word)__u_p, (__u_word)__u_b, (__u_word)__u_fl, 0);
#endif
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; } return 0;
#endif
}
#endif
/* 0.0.28 H4: forwarded to the system C library on Linux and macOS */
#ifndef _WIN32
int fchmod(int __u_fd, mode_t __u_mode);
#endif
#endif

#else

/* Minimal POSIX <unistd.h> (0.0.17 R17-5, ordered by the realprog corpus):
 * the file-descriptor calls the bundled library already makes through its
 * own syscall intrinsics, under their POSIX names.  Return values follow
 * POSIX: -1 with errno set on failure.  isatty arrived in 0.0.18.  Not provided (yet): fork/exec,
 * getpid, sleep, pipes (POSIX L2). */
#ifndef _UNISA_UNISTD_H
#define _UNISA_UNISTD_H



/* <sys/types.h> for the unisa C subset.
 *
 * TYPES ONLY.  POSIX puts its shorthand scalar types here and the functions
 * that use them in <unistd.h>, <fcntl.h>, <sys/stat.h> and friends; the
 * reference implements none of those functions yet, so none of those headers
 * exists -- a constant or a prototype with no implementation behind it would
 * be inventing an interface.  This header is the part that is real and
 * self-contained: the widths.
 *
 * All of them are 64-bit, like the rest of the subset (prd [G-2]: `int`
 * expressions evaluate in 64 bits).  `ssize_t` is the signed twin of
 * `size_t`; `off_t` is signed so a negative seek works; `pid_t` and `mode_t`
 * are 32-bit on every platform this compiler targets, and `dev_t`/`ino_t`
 * are 64-bit there.  `time_t` lives in <time.h>; `clock_t` in <time.h> too.
 *
 * There is no `struct stat`: it belongs to <sys/stat.h>, which needs `stat`
 * and `fstat` to be worth declaring. */
#ifndef _UNISA_SYS_TYPES_H
#define _UNISA_SYS_TYPES_H

/* LLP64 Windows (_WIN64) spells the 8-byte types `long long`: every type
 * here keeps the same 8-byte width and layout on all targets. */
#if defined(_WIN64)
#ifndef _UNISA_SIZE_T
#define _UNISA_SIZE_T
typedef unsigned long long size_t;
#endif
typedef long long          ssize_t;
typedef long long          off_t;
typedef int                pid_t;
typedef unsigned int       uid_t;
typedef unsigned int       gid_t;
typedef unsigned int       mode_t;
typedef unsigned long long dev_t;
typedef unsigned long long ino_t;
typedef unsigned long long nlink_t;
typedef long long          blksize_t;
typedef long long          blkcnt_t;
#ifndef _UNISA_TIME_T
#define _UNISA_TIME_T
typedef long long          time_t;
#endif
typedef long long          suseconds_t;
typedef unsigned long long useconds_t;
#else
#ifndef _UNISA_SIZE_T
#define _UNISA_SIZE_T
typedef unsigned long size_t;
#endif
typedef long          ssize_t;
typedef long          off_t;
typedef int           pid_t;
typedef unsigned int  uid_t;
typedef unsigned int  gid_t;
typedef unsigned int  mode_t;
typedef unsigned long dev_t;
typedef unsigned long ino_t;
typedef unsigned long nlink_t;
typedef long          blksize_t;
typedef long          blkcnt_t;
#ifndef _UNISA_TIME_T
#define _UNISA_TIME_T
typedef long          time_t;
#endif
typedef long          suseconds_t;
typedef unsigned long useconds_t;
#endif

#endif


/* sys/_ret.h -- the one place a raw "-errno" result becomes -1 + errno.
 * Shared by every header whose bodies call the kernel (0.0.21: <time.h>
 * used it without including <unistd.h>, so clock_gettime alone failed). */
#ifndef _UNISA_RET_H
#define _UNISA_RET_H

#if !__UNISA_FTRIM_LIBC || __UN__unisa_ret
static long _unisa_ret(long __u_r) { if (__u_r < 0 && __u_r > -4096) { errno = (int)(0 - __u_r); return -1; } return __u_r; }
#endif
#endif



/* sys/_win.h -- 0.0.21 Windows POSIX layer, batch 1: the shared helpers that
 * let <unistd.h>, <fcntl.h>, <sys/stat.h> and <time.h> wrap POSIX names thinly
 * over kernel32 (no home-grown libc).  The host channel is the program's IAT:
 * __hostaddr0..3 = LoadLibraryA GetProcAddress FreeLibrary GetLastError, and
 * __hostcall(fn, __u_word[10]) makes the Win64/AAPCS64 call.  kernel32's handle is
 * looked up once; each caller caches its own procedure in a static.
 * On Windows an fd IS the HANDLE value (as the bundled stdio already has it).
 * Windows is LLP64 (long is 4 bytes): handles, procedures, pointers and the host
 * channel's 8-byte slots are __u_word (<sys/_word.h>), never long. */
#ifndef _UNISA_SYS_WIN_H
#define _UNISA_SYS_WIN_H
#if defined(_WIN32) && !defined(_UNISA_NO_HOSTCALL)   /* the compiler builds itself without the host channel until the product lowers it on Windows (0.0.22) */


#if !__UNISA_FTRIM_LIBC || __UN__ux_sym
static __u_word _ux_sym(const char *__u_name) {          /* GetProcAddress(kernel32, name) */
    static __u_word __u_k32;
    __u_word __u_a[10] = {0};
    __u_a[1] = 0; __u_a[2] = 0; __u_a[3] = 0; __u_a[4] = 0; __u_a[5] = 0;
    if (!__u_k32) { __u_a[0] = (__u_word)"kernel32.dll"; __u_k32 = __hostcall(__hostaddr0(), __u_a); }
    __u_a[0] = __u_k32; __u_a[1] = (__u_word)__u_name;
    return __hostcall(__hostaddr1(), __u_a);
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__ux_call
static __u_word _ux_call(__u_word __u_fn, __u_word __u_x0, __u_word __u_x1, __u_word __u_x2, __u_word __u_x3) {
    __u_word __u_a[10] = {0};
    __u_a[0] = __u_x0; __u_a[1] = __u_x1; __u_a[2] = __u_x2; __u_a[3] = __u_x3; __u_a[4] = 0; __u_a[5] = 0;
    return __hostcall(__u_fn, __u_a);
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__ux_errno
static int _ux_errno(void) {                         /* GetLastError -> errno */
    __u_word __u_a[10] = {0}; __u_word __u_e;
    __u_a[0] = 0; __u_a[1] = 0; __u_a[2] = 0; __u_a[3] = 0; __u_a[4] = 0; __u_a[5] = 0;
    __u_e = __hostcall(__hostaddr3(), __u_a) & 0xFFFFFFFFLL;
    if (__u_e == 2 || __u_e == 3) return ENOENT;
    if (__u_e == 5) return EACCES;
    if (__u_e == 6) return EBADF;
    if (__u_e == 80 || __u_e == 183) return EEXIST;
    if (__u_e == 145) return ENOTEMPTY;
    if (__u_e == 32) return EBUSY;
    if (__u_e == 87) return EINVAL;
    return EIO;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__ux_fail
static int _ux_fail(void) { errno = _ux_errno(); return -1; }
#endif
/* The names above are kept only when a baked library key lists them. A disk
   body that started calling them later is compiled, and the callee is then
   missing. These macros expand in the caller and use the host channel
   directly, same mapping as _ux_sym / _ux_call / _ux_errno. */
#define _ux_load(dst, name) do { \
    static __u_word __uxk; __u_word __uxa[10]; int __uxi; \
    __uxi = 0; while (__uxi < 10) { __uxa[__uxi] = 0; __uxi = __uxi + 1; } \
    if (!__uxk) { __uxa[0] = (__u_word)"kernel32.dll"; __uxk = __hostcall(__hostaddr0(), __uxa); } \
    __uxa[0] = __uxk; __uxa[1] = (__u_word)(name); (dst) = __hostcall(__hostaddr1(), __uxa); \
} while (0)
#define _ux_go(dst, fn, x0, x1, x2, x3) do { \
    __u_word __uxa[10]; int __uxi; \
    __uxi = 0; while (__uxi < 10) { __uxa[__uxi] = 0; __uxi = __uxi + 1; } \
    __uxa[0] = (__u_word)(x0); __uxa[1] = (__u_word)(x1); __uxa[2] = (__u_word)(x2); __uxa[3] = (__u_word)(x3); \
    (dst) = __hostcall((fn), __uxa); \
} while (0)
#define _ux_err(dst) do { \
    __u_word __uxa[10]; __u_word __uxe; int __uxi; \
    __uxi = 0; while (__uxi < 10) { __uxa[__uxi] = 0; __uxi = __uxi + 1; } \
    __uxe = __hostcall(__hostaddr3(), __uxa) & 0xFFFFFFFFLL; \
    if (__uxe == 2 || __uxe == 3) (dst) = ENOENT; \
    else if (__uxe == 5) (dst) = EACCES; \
    else if (__uxe == 6) (dst) = EBADF; \
    else if (__uxe == 80 || __uxe == 183) (dst) = EEXIST; \
    else if (__uxe == 145) (dst) = ENOTEMPTY; \
    else if (__uxe == 32) (dst) = EBUSY; \
    else if (__uxe == 87) (dst) = EINVAL; \
    else (dst) = EIO; \
} while (0)
#if !__UNISA_FTRIM_LIBC || __UN__ux_ft2unix
static __u_word _ux_ft2unix(const unsigned int *__u_ft) {   /* FILETIME (100 ns since 1601) -> unix seconds */
    __u_word __u_t; __u_t = ((__u_word)__u_ft[1] << 32) | (__u_word)__u_ft[0];
    return (__u_t - 116444736000000000LL) / 10000000LL;
}
#endif
#endif
#endif

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif
#if !__UNISA_FTRIM_LIBC || __UN_read
static ssize_t read(int __u_fd, void *__u_buf, size_t __u_n) {
    __u_word __u_r; __u_r = __read(__u_fd, (char *)__u_buf, (__u_word)__u_n);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; }
    return __u_r;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_write
static ssize_t write(int __u_fd, const void *__u_buf, size_t __u_n) {
    __u_word __u_r; __u_r = __write(__u_fd, (char *)__u_buf, (__u_word)__u_n);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; }
    return __u_r;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_close
static int close(int __u_fd) {
    __u_word __u_r; __u_r = __close(__u_fd);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; }
    return 0;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_lseek
static off_t lseek(int __u_fd, off_t __u_off, int __u_whence) {
    __u_word __u_r; __u_r = __lseek(__u_fd, (__u_word)__u_off, __u_whence);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; }
    return __u_r;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_isatty
static int isatty(int __u_fd) {               /* a terminal answers the attribute request (R18-10) */
#ifdef _WIN32
    static __u_word __u_f;                        /* GetFileType == FILE_TYPE_CHAR */
    if (!__u_f) __u_f = _ux_sym("GetFileType");
    if ((_ux_call(__u_f, (__u_word)__u_fd, 0, 0, 0) & 0xFFFFFFFFL) == 2) return 1;
    errno = ENOTTY; return 0;
#else
    unsigned char __u_t[128]; __u_word __u_r;
#ifdef __APPLE__
    __u_r = __ioctl(__u_fd, 0x40487413L, (char *)__u_t);
#else
    __u_r = __ioctl(__u_fd, 0x5401L, (char *)__u_t);
#endif
    if (__u_r < 0) { errno = (int)(0 - __u_r); return 0; } return 1;
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_ftruncate
static int ftruncate(int __u_fd, off_t __u_len) {   /* R18-5 (kilo saves through it) */
#ifdef _WIN32
    static __u_word __u_sp; static __u_word __u_se; __u_word __u_cur; int __u_ok;   /* the file position is kept, as POSIX has it */
    if (!__u_sp) { __u_sp = _ux_sym("SetFilePointerEx"); __u_se = _ux_sym("SetEndOfFile"); }
    if (__u_len < 0) { errno = EINVAL; return -1; }
    __u_cur = 0;
    if (!(_ux_call(__u_sp, (__u_word)__u_fd, 0, (__u_word)&__u_cur, 1) & 0xFFFFFFFFL)) return _ux_fail();
    if (!(_ux_call(__u_sp, (__u_word)__u_fd, (__u_word)__u_len, 0, 0) & 0xFFFFFFFFL)) return _ux_fail();
    __u_ok = (int)(_ux_call(__u_se, (__u_word)__u_fd, 0, 0, 0) & 0xFFFFFFFFL);
    if (!__u_ok) __u_ok = 0 - _ux_errno();
    _ux_call(__u_sp, (__u_word)__u_fd, __u_cur, 0, 0);
    if (__u_ok < 0) { errno = 0 - __u_ok; return -1; }
    return 0;
#else
    __u_word __u_r; __u_r = __ftruncate(__u_fd, (__u_word)__u_len, 0);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; }
    return 0;
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_unlink
static int unlink(const char *__u_path) {
#ifdef _WIN32
    static __u_word __u_f;                        /* DeleteFileA: a BOOL */
    if (!__u_f) __u_f = _ux_sym("DeleteFileA");
    if (_ux_call(__u_f, (__u_word)__u_path, 0, 0, 0) & 0xFFFFFFFFL) return 0;
    return _ux_fail();
#else
    __u_word __u_r; __u_r = __unlink((char *)__u_path);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; }
    return 0;
#endif
}
#endif
/* *at() flags (0.0.35 M1, minicon's loader): each kernel's own values */
#ifndef AT_FDCWD
#ifdef __APPLE__
#define AT_FDCWD (0 - 2)
#define AT_SYMLINK_NOFOLLOW 0x20
#define AT_REMOVEDIR 0x80
#else
#define AT_FDCWD (0 - 100)
#define AT_SYMLINK_NOFOLLOW 0x100
#define AT_REMOVEDIR 0x200
#endif
#endif
#if !__UNISA_FTRIM_LIBC || __UN_unlinkat
static int unlinkat(int __u_dfd, const char *__u_p, int __u_fl) {
#ifdef _WIN32
    if (__u_dfd != AT_FDCWD) { errno = ENOSYS; return -1; }
    return (__u_fl & AT_REMOVEDIR) ? rmdir(__u_p) : unlink(__u_p);
#else
    __u_word __u_r;
#if defined(__APPLE__)
#if defined(__x86_64__)
    __u_r = __syscall6(0x2000000L + 472, (__u_word)__u_dfd, (__u_word)__u_p, (__u_word)__u_fl, 0, 0);
#else
    __u_r = __syscall6(472, (__u_word)__u_dfd, (__u_word)__u_p, (__u_word)__u_fl, 0, 0);
#endif
#elif defined(__x86_64__)
    __u_r = __syscall6(263, (__u_word)__u_dfd, (__u_word)__u_p, (__u_word)__u_fl, 0, 0);
#else
    __u_r = __syscall6(35, (__u_word)__u_dfd, (__u_word)__u_p, (__u_word)__u_fl, 0, 0);
#endif
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; } return 0;
#endif
}
#endif
/* ---- processes (0.0.19 R19-5), through the generic gate __syscall6 (R19-9):
   the call numbers live here, per OS/arch; nothing in the compiler's tables.
   macOS x86-64 numbers carry the BSD class 0x2000000.  macOS fork returns the
   same value in both processes (the child mark is in a second register), so
   the child is told apart by its pid; macOS has no pipe2, so pipe is a
   socketpair (AF_UNIX stream: bidirectional, stated rather than hidden). */
#ifndef _WIN32
#if defined(__APPLE__) && defined(__x86_64__)
#define _UNISA_SC(n) (0x2000000L + (n))
#else
#define _UNISA_SC(n) ((__u_word)(n))
#endif
#ifdef __APPLE__
#define _UNISA_NR_getpid 20
#define _UNISA_NR_fork 2
#define _UNISA_NR_execve 59
#define _UNISA_NR_wait4 7
#define _UNISA_NR_dup2 90
#define _UNISA_NR_getppid 39
#elif defined(__x86_64__)
#define _UNISA_NR_getpid 39
#define _UNISA_NR_fork 57
#define _UNISA_NR_execve 59
#define _UNISA_NR_wait4 61
#define _UNISA_NR_dup2 33
#define _UNISA_NR_pipe2 293
#define _UNISA_NR_getcwd 79
#define _UNISA_NR_getppid 110
#else
#define _UNISA_NR_getpid 172
#define _UNISA_NR_clone 220
#define _UNISA_NR_execve 221
#define _UNISA_NR_wait4 260
#define _UNISA_NR_dup3 24
#define _UNISA_NR_pipe2 59
#define _UNISA_NR_getcwd 17
#define _UNISA_NR_getppid 173
#endif
/* _exit: no atexit handlers, no stdio flush */
#if !__UNISA_FTRIM_LIBC || __UN__exit
static void _exit(int __u_code) { __exit(__u_code); }
#endif

#endif
#if !__UNISA_FTRIM_LIBC || __UN_getpid
static pid_t getpid(void) {
#ifdef _WIN32
    static __u_word __u_f;
    if (!__u_f) __u_f = _ux_sym("GetCurrentProcessId");
    return (pid_t)(_ux_call(__u_f, 0, 0, 0, 0) & 0xFFFFFFFFL);
#else
    return (pid_t)__syscall6(_UNISA_SC(_UNISA_NR_getpid), 0, 0, 0, 0, 0);
#endif
}
#endif
#ifndef _WIN32
#if !__UNISA_FTRIM_LIBC || __UN_getppid
static pid_t getppid(void) { return (pid_t)__syscall6(_UNISA_SC(_UNISA_NR_getppid), 0, 0, 0, 0, 0); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_fork
static pid_t fork(void) {
    __u_word __u_r;
#ifdef __APPLE__
    long __u_me; __u_me = getpid();
    __u_r = __syscall6(_UNISA_SC(_UNISA_NR_fork), 0, 0, 0, 0, 0);
    if (__u_r >= 0 && getpid() != __u_me) return 0;   /* the child */
#elif defined(__x86_64__)
    __u_r = __syscall6(_UNISA_NR_fork, 0, 0, 0, 0, 0);
#else
    __u_r = __syscall6(_UNISA_NR_clone, 17, 0, 0, 0, 0);    /* SIGCHLD, no new stack: fork */
#endif
    return (pid_t)_unisa_ret(__u_r);
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_execve
static int execve(const char *__u_path, char *const __u_argv[], char *const __u_envp[]) {
    return (int)_unisa_ret(__syscall6(_UNISA_SC(_UNISA_NR_execve), (__u_word)__u_path, (__u_word)__u_argv, (__u_word)__u_envp, 0, 0));
}
#endif
/* 0.0.31 H1'': POSIX `environ` is <stdlib.h>'s _unisa_env, one object for the whole program
   (a tentative definition with external linkage in every unit, merged at link, C99 6.9.2), so an
   assignment in one unit is what getenv and execv see in every other. */

#define environ _unisa_env
/* execv passes the current environ (POSIX; 0.0.28 E20) */
#if !__UNISA_FTRIM_LIBC || __UN_execv
static int execv(const char *__u_path, char *const __u_argv[]) { return execve(__u_path, __u_argv, environ); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_execvp

#if !__UNISA_FTRIM_LIBC || __UN_execvp
static int execvp(const char *__u_file, char *const __u_argv[]) {
    char __u_buf[1024]; const char *__u_p; int __u_i; int __u_j; int __u_k;
    __u_i = 0; while (__u_file[__u_i]) { if (__u_file[__u_i] == 47) return execv(__u_file, __u_argv); __u_i = __u_i + 1; }
    __u_p = getenv("PATH"); if (__u_p == 0) __u_p = "/usr/bin:/bin";
    while (1) {
        __u_j = 0;
        while (__u_p[__u_j] && __u_p[__u_j] != 58 && __u_j < 900) { __u_buf[__u_j] = __u_p[__u_j]; __u_j = __u_j + 1; }
        if (__u_j == 0) { __u_buf[0] = 46; __u_j = 1; }
        __u_buf[__u_j] = 47; __u_k = 0;
        while (__u_file[__u_k] && __u_j + 1 + __u_k < 1023) { __u_buf[__u_j + 1 + __u_k] = __u_file[__u_k]; __u_k = __u_k + 1; }
        __u_buf[__u_j + 1 + __u_k] = 0;
        execv(__u_buf, __u_argv);
        while (*__u_p && *__u_p != 58) __u_p = __u_p + 1;
        if (*__u_p == 0) break;
        __u_p = __u_p + 1;
    }
    return -1;
}
#endif
#endif
/* execl family (0.0.20 R20-6, dsh): the variadic list gathered into an
   array, then execve/execvp; at most 255 arguments */

#if !__UNISA_FTRIM_LIBC || __UN_execl
static int execl(const char *__u_path, const char *__u_a0, ...) {
    char *__u_v[256]; int __u_n; va_list __u_ap;
    __u_v[0] = (char *)__u_a0; __u_n = 1; va_start(__u_ap, __u_a0);
    while (__u_v[__u_n - 1] && __u_n < 256) { __u_v[__u_n] = va_arg(__u_ap, char *); __u_n = __u_n + 1; }
    va_end(__u_ap);
    if (__u_v[__u_n - 1]) return (int)_unisa_ret(0 - 7);    /* E2BIG */
    return execve(__u_path, __u_v, environ);
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_execlp
static int execlp(const char *__u_file, const char *__u_a0, ...) {
    char *__u_v[256]; int __u_n; va_list __u_ap;
    __u_v[0] = (char *)__u_a0; __u_n = 1; va_start(__u_ap, __u_a0);
    while (__u_v[__u_n - 1] && __u_n < 256) { __u_v[__u_n] = va_arg(__u_ap, char *); __u_n = __u_n + 1; }
    va_end(__u_ap);
    if (__u_v[__u_n - 1]) return (int)_unisa_ret(0 - 7);
    return execvp(__u_file, __u_v);
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_execle
static int execle(const char *__u_path, const char *__u_a0, ...) {
    char *__u_v[256]; char **__u_e; int __u_n; va_list __u_ap;
    __u_v[0] = (char *)__u_a0; __u_n = 1; va_start(__u_ap, __u_a0);
    while (__u_v[__u_n - 1] && __u_n < 256) { __u_v[__u_n] = va_arg(__u_ap, char *); __u_n = __u_n + 1; }
    if (__u_v[__u_n - 1]) { va_end(__u_ap); return (int)_unisa_ret(0 - 7); }
    __u_e = va_arg(__u_ap, char **); va_end(__u_ap);
    return execve(__u_path, __u_v, __u_e);
}
#endif
#endif
/* sleep / usleep over the shared nanosleep primitive (sys/_timespec.h; Sleep on Windows) */

/* sys/_timespec.h -- 0.0.20 R20-6 (dsh): struct timespec and the one sleep
 * primitive that <time.h> nanosleep and <unistd.h> sleep/usleep share.
 * Linux: the nanosleep system call.  macOS has no nanosleep call, so the
 * wait is select(0, 0, 0, 0, &tv) -- microsecond granularity, stated. */
#ifndef _UNISA_TIMESPEC_H
#define _UNISA_TIMESPEC_H

#ifndef _UNISA_TIME_T
#define _UNISA_TIME_T
typedef __u_word time_t;
#endif
#ifndef _UNISA_TIMESPEC
#define _UNISA_TIMESPEC
struct timespec { __u_word tv_sec; long tv_nsec; };   /* one definition with sys/stat.h */
#endif

#if !__UNISA_FTRIM_LIBC || __UN__unisa_nanosleep
static long _unisa_nanosleep(const struct timespec *__u_q) {
#ifdef _WIN32
    static __u_word __u_f;                        /* Sleep(ms), rounded up */
    if (!__u_f) __u_f = _ux_sym("Sleep");
    _ux_call(__u_f, __u_q->tv_sec * 1000 + (__u_q->tv_nsec + 999999) / 1000000, 0, 0, 0);
    return 0;
#elif defined(__APPLE__)
    __u_word __u_tv[2];
    __u_tv[0] = __u_q->tv_sec; __u_tv[1] = (__u_q->tv_nsec + 999) / 1000;
#ifdef __x86_64__
    return __syscall6(0x2000000L + 93, 0, 0, 0, 0, (__u_word)__u_tv);
#else
    return __syscall6(93, 0, 0, 0, 0, (__u_word)__u_tv);
#endif
#elif defined(__x86_64__)
    return __syscall6(35, (__u_word)__u_q, 0, 0, 0, 0);
#else
    return __syscall6(101, (__u_word)__u_q, 0, 0, 0, 0);
#endif
}
#endif
#endif

#if !__UNISA_FTRIM_LIBC || __UN_sleep
static unsigned sleep(unsigned __u_s) { struct timespec __u_q; __u_q.tv_sec = __u_s; __u_q.tv_nsec = 0; _unisa_nanosleep(&__u_q); return 0; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_usleep
static int usleep(unsigned __u_us) { struct timespec __u_q; __u_q.tv_sec = __u_us / 1000000; __u_q.tv_nsec = (long)(__u_us % 1000000) * 1000; return (int)_unisa_ret(_unisa_nanosleep(&__u_q)); }
#endif
#ifndef _WIN32
#if !__UNISA_FTRIM_LIBC || __UN_dup2
static int dup2(int __u_old, int __u_new) {
#if defined(__APPLE__) || defined(__x86_64__)
    return (int)_unisa_ret(__syscall6(_UNISA_SC(_UNISA_NR_dup2), __u_old, __u_new, 0, 0, 0));
#else
    if (__u_old == __u_new) { __u_word __u_r; __u_r = __fcntl(__u_old, 1, 0); return __u_r < 0 ? (int)_unisa_ret(__u_r) : __u_new; }   /* F_GETFD: is OLD a valid descriptor */
    return (int)_unisa_ret(__syscall6(_UNISA_NR_dup3, __u_old, __u_new, 0, 0, 0));
#endif
}
#endif
#endif
/* Windows: pipe is CreatePipe (two HANDLEs, not inheritable).  dup is
   DuplicateHandle (below); dup2 stays refused by name there: a HANDLE cannot be
   placed at a chosen number. */
#if !__UNISA_FTRIM_LIBC || __UN_pipe
static int pipe(int __u_fds[2]) {
#ifdef _WIN32
    static __u_word __u_cp; __u_word __u_h[2];
    if (!__u_cp) __u_cp = _ux_sym("CreatePipe");
    __u_h[0] = 0; __u_h[1] = 0;
    if (!(_ux_call(__u_cp, (__u_word)&__u_h[0], (__u_word)&__u_h[1], 0, 0) & 0xFFFFFFFFL)) return _ux_fail();
    __u_fds[0] = (int)__u_h[0]; __u_fds[1] = (int)__u_h[1];
    return 0;
#elif defined(__APPLE__)
    return (int)_unisa_ret(__syscall6(_UNISA_SC(135), 1, 1, 0, (__u_word)__u_fds, 0));   /* socketpair(AF_UNIX, SOCK_STREAM) */
#else
    return (int)_unisa_ret(__syscall6(_UNISA_NR_pipe2, (__u_word)__u_fds, 0, 0, 0, 0));
#endif
}
#endif
#define F_OK 0
#define X_OK 1
#define W_OK 2
#define R_OK 4
#ifndef _WIN32
#ifdef __APPLE__
#define _UNISA_NR_fsync 95
#define _UNISA_NR_dup 41
#define _UNISA_NR_rmdir 137
#define _UNISA_NR_access 33
#define _UNISA_NR_readlink 58
#define _UNISA_NR_symlink 57
#elif defined(__x86_64__)
#define _UNISA_NR_fsync 74
#define _UNISA_NR_dup 32
#define _UNISA_NR_rmdir 84
#define _UNISA_NR_access 21
#define _UNISA_NR_readlink 89
#define _UNISA_NR_symlink 88
#else
#define _UNISA_NR_fsync 82
#define _UNISA_NR_dup 23
#define _UNISA_NR_unlinkat 35
#define _UNISA_NR_faccessat 48
#define _UNISA_NR_readlinkat 78
#define _UNISA_NR_symlinkat 36
#endif
#endif
#if !__UNISA_FTRIM_LIBC || __UN_fsync
static int fsync(int __u_fd) {
#ifdef _WIN32
    static __u_word __u_ff;
    if (!__u_ff) __u_ff = _ux_sym("FlushFileBuffers");
    if (!(_ux_call(__u_ff, (__u_word)__u_fd, 0, 0, 0) & 0xFFFFFFFFL)) return _ux_fail();
    return 0;
#else
    return (int)_unisa_ret(__syscall6(_UNISA_SC(_UNISA_NR_fsync), __u_fd, 0, 0, 0, 0));
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_dup
static int dup(int __u_fd) {
#ifdef _WIN32
    /* 0.0.22 Windows POSIX batch 4: DuplicateHandle(self, h, self, &new, 0, FALSE,
       DUPLICATE_SAME_ACCESS) -- seven arguments, carried by the ten-slot host call. */
    static __u_word __u_dh, __u_gp; __u_word __u_a[10] = {0}; __u_word __u_h, __u_self;
    if (!__u_dh) { __u_dh = _ux_sym("DuplicateHandle"); __u_gp = _ux_sym("GetCurrentProcess"); }
    __u_self = _ux_call(__u_gp, 0, 0, 0, 0); __u_h = 0;
    __u_a[0] = __u_self; __u_a[1] = (__u_word)__u_fd; __u_a[2] = __u_self; __u_a[3] = (__u_word)&__u_h;
    __u_a[4] = 0; __u_a[5] = 0; __u_a[6] = 2;
    if (!(__hostcall(__u_dh, __u_a) & 0xFFFFFFFFL)) return _ux_fail();
    return (int)__u_h;
#else
    return (int)_unisa_ret(__syscall6(_UNISA_SC(_UNISA_NR_dup), __u_fd, 0, 0, 0, 0));
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_rmdir
static int rmdir(const char *__u_p) {
#ifdef _WIN32
    static __u_word __u_f;
    if (!__u_f) __u_f = _ux_sym("RemoveDirectoryA");
    if (_ux_call(__u_f, (__u_word)__u_p, 0, 0, 0) & 0xFFFFFFFFL) return 0;
    return _ux_fail();
#elif defined(__APPLE__) || defined(__x86_64__)
    return (int)_unisa_ret(__syscall6(_UNISA_SC(_UNISA_NR_rmdir), (__u_word)__u_p, 0, 0, 0, 0));
#else
    return (int)_unisa_ret(__syscall6(_UNISA_NR_unlinkat, -100, (__u_word)__u_p, 0x200, 0, 0));   /* AT_FDCWD, AT_REMOVEDIR */
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_access
static int access(const char *__u_p, int __u_mode) {
#ifdef _WIN32
    static __u_word __u_f; __u_word __u_a;            /* GetFileAttributesA; X_OK is existence */
    if (!__u_f) __u_f = _ux_sym("GetFileAttributesA");
    __u_a = _ux_call(__u_f, (__u_word)__u_p, 0, 0, 0) & 0xFFFFFFFFL;
    if (__u_a == 0xFFFFFFFFL) return _ux_fail();
    if ((__u_mode & W_OK) && (__u_a & 1) && !(__u_a & 0x10)) { errno = EACCES; return -1; }   /* READONLY file */
    return 0;
#elif defined(__APPLE__) || defined(__x86_64__)
    return (int)_unisa_ret(__syscall6(_UNISA_SC(_UNISA_NR_access), (__u_word)__u_p, __u_mode, 0, 0, 0));
#else
    return (int)_unisa_ret(__syscall6(_UNISA_NR_faccessat, -100, (__u_word)__u_p, __u_mode, 0, 0));
#endif
}
#endif
#ifndef _WIN32
#if !__UNISA_FTRIM_LIBC || __UN_readlink
static long readlink(const char *__u_p, char *__u_buf, size_t __u_n) {
#if defined(__APPLE__) || defined(__x86_64__)
    return _unisa_ret(__syscall6(_UNISA_SC(_UNISA_NR_readlink), (__u_word)__u_p, (__u_word)__u_buf, (__u_word)__u_n, 0, 0));
#else
    return _unisa_ret(__syscall6(_UNISA_NR_readlinkat, -100, (__u_word)__u_p, (__u_word)__u_buf, (__u_word)__u_n, 0));
#endif
}
#endif
/* 0.0.36 M1: the user and group ids are direct system calls (0.0.27 H1 forwarded them to the system
   C library, which a static unisacc.com on Linux cannot reach: minicon's loader calls geteuid) */
#ifdef __APPLE__
#define _UNISA_NR_getuid 24
#define _UNISA_NR_geteuid 25
#define _UNISA_NR_getgid 47
#define _UNISA_NR_getegid 43
#elif defined(__x86_64__)
#define _UNISA_NR_getuid 102
#define _UNISA_NR_geteuid 107
#define _UNISA_NR_getgid 104
#define _UNISA_NR_getegid 108
#else
#define _UNISA_NR_getuid 174
#define _UNISA_NR_geteuid 175
#define _UNISA_NR_getgid 176
#define _UNISA_NR_getegid 177
#endif
#if !__UNISA_FTRIM_LIBC || __UN_getuid
static uid_t getuid(void) { return (uid_t)__syscall6(_UNISA_SC(_UNISA_NR_getuid), 0, 0, 0, 0, 0); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_geteuid
static uid_t geteuid(void) { return (uid_t)__syscall6(_UNISA_SC(_UNISA_NR_geteuid), 0, 0, 0, 0, 0); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_getgid
static gid_t getgid(void) { return (gid_t)__syscall6(_UNISA_SC(_UNISA_NR_getgid), 0, 0, 0, 0, 0); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_getegid
static gid_t getegid(void) { return (gid_t)__syscall6(_UNISA_SC(_UNISA_NR_getegid), 0, 0, 0, 0, 0); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_ttyname
static char *ttyname(int __u_fd) {             /* 0.0.24 D2: the terminal's path, or 0 with errno */
    static char __u_tn[1024]; __u_word __u_r;
    if (!isatty(__u_fd)) return 0;
#ifdef __APPLE__
    __u_r = __fcntl(__u_fd, 50, (__u_word)__u_tn);  /* F_GETPATH */
    if (__u_r < 0) { errno = (int)(0 - __u_r); return 0; }
#else
    {   char __u_p[32]; int __u_k; int __u_d; char __u_dig[12];
        { const char *__u_s; __u_s = "/proc/self/fd/"; for (__u_k = 0; __u_k < 14; __u_k = __u_k + 1) __u_p[__u_k] = __u_s[__u_k]; } __u_k = 0; __u_d = __u_fd;
        do { __u_dig[__u_k] = (char)(48 + __u_d % 10); __u_d = __u_d / 10; __u_k = __u_k + 1; } while (__u_d);
        __u_d = 14; while (__u_k) { __u_k = __u_k - 1; __u_p[__u_d] = __u_dig[__u_k]; __u_d = __u_d + 1; } __u_p[__u_d] = 0;
        __u_r = readlink(__u_p, __u_tn, sizeof(__u_tn) - 1);
        if (__u_r < 0) return 0;
        __u_tn[__u_r] = 0; }
#endif
    return __u_tn;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_symlink
static int symlink(const char *__u_old, const char *__u_new) {
#if defined(__APPLE__) || defined(__x86_64__)
    return (int)_unisa_ret(__syscall6(_UNISA_SC(_UNISA_NR_symlink), (__u_word)__u_old, (__u_word)__u_new, 0, 0, 0));
#else
    return (int)_unisa_ret(__syscall6(_UNISA_NR_symlinkat, (__u_word)__u_old, -100, (__u_word)__u_new, 0, 0));
#endif
}
#endif
#endif
#if !__UNISA_FTRIM_LIBC || __UN_chdir
static int chdir(const char *__u_p) {
#ifdef _WIN32
    static __u_word __u_f;
    if (!__u_f) __u_f = _ux_sym("SetCurrentDirectoryA");
    if (_ux_call(__u_f, (__u_word)__u_p, 0, 0, 0) & 0xFFFFFFFFL) return 0;
    return _ux_fail();
#elif defined(__APPLE__)
    return (int)_unisa_ret(__syscall6(_UNISA_SC(12), (__u_word)__u_p, 0, 0, 0, 0));
#elif defined(__x86_64__)
    return (int)_unisa_ret(__syscall6(80, (__u_word)__u_p, 0, 0, 0, 0));
#else
    return (int)_unisa_ret(__syscall6(49, (__u_word)__u_p, 0, 0, 0, 0));
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_getcwd
static char *getcwd(char *__u_buf, size_t __u_size) {
#ifdef _WIN32
    static __u_word __u_f; __u_word __u_n;            /* GetCurrentDirectoryA (backslashes kept) */
    if (!__u_f) __u_f = _ux_sym("GetCurrentDirectoryA");
    __u_n = _ux_call(__u_f, (__u_word)__u_size, (__u_word)__u_buf, 0, 0) & 0xFFFFFFFFL;
    if (__u_n == 0) { _ux_fail(); return 0; }
    if ((size_t)__u_n >= __u_size) { errno = ERANGE; return 0; }
    return __u_buf;
#elif defined(__APPLE__)
    char __u_tmp[1024]; __u_word __u_fd; __u_word __u_r; size_t __u_n;
    __u_fd = __open(".", 0x100000, 0);           /* O_RDONLY | O_DIRECTORY */
    if (__u_fd < 0) { errno = (int)(0 - __u_fd); return 0; }
    __u_r = __fcntl(__u_fd, 50, (__u_word)__u_tmp);  /* F_GETPATH (needs MAXPATHLEN bytes) */
    __close(__u_fd);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return 0; }
    __u_n = 0; while (__u_tmp[__u_n]) __u_n = __u_n + 1;
    if (__u_n + 1 > __u_size) { errno = 34; return 0; }   /* ERANGE */
    __u_n = 0; while ((__u_buf[__u_n] = __u_tmp[__u_n]) != 0) __u_n = __u_n + 1;
    return __u_buf;
#else
    __u_word __u_r; __u_r = __syscall6(_UNISA_NR_getcwd, (__u_word)__u_buf, (__u_word)__u_size, 0, 0, 0);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return 0; }
    return __u_buf;
#endif
}
#endif
/* 0.0.28 H4 (SQLite's unix VFS): forwarded to the system C library on Linux and macOS */
#ifndef _WIN32
ssize_t pread(int __u_fd, void *__u_buf, size_t __u_n, off_t __u_off);
ssize_t pwrite(int __u_fd, const void *__u_buf, size_t __u_n, off_t __u_off);
int fchown(int __u_fd, uid_t __u_uid, gid_t __u_gid);
/* 0.0.34 X1 (cx.h cx_run_timeout kills the whole group): forwarded */
int setpgid(pid_t __u_pid, pid_t __u_pgid);
pid_t getpgrp(void);
long sysconf(int __u_name);
#ifdef __APPLE__
#define _SC_PAGESIZE 29
#else
#define _SC_PAGESIZE 30
#endif
#define _SC_PAGE_SIZE _SC_PAGESIZE
ssize_t readlink(const char *__u_path, char *__u_buf, size_t __u_n);
#endif
/* 0.0.34 L2 (SQLite's Apple locking style): forwarded */
#ifdef __APPLE__
typedef unsigned char uuid_t[16];
int fsctl(const char *__u_path, unsigned long __u_req, void *__u_data, unsigned int __u_opt);
int gethostuuid(uuid_t __u_id, const struct timespec *__u_wait);
#endif
#endif


/* <signal.h> for the unisa C subset -- the in-process half of it.
 * `signal` records a handler and `raise` calls it (and leaves it installed,
 * as BSD and glibc do); SIG_DFL for the signals
 * whose default is to terminate ends the process with the shell's status
 * for that signal (128 + n), which is what a program that raises SIGABRT
 * and is run from a shell observes.  No signal ever arrives from OUTSIDE:
 * there is no sigaction system call here, so a handler installed for
 * SIGINT is never called by Ctrl-C.  That is stated rather than faked. */
#ifndef _UNISA_SIGNAL_H
#define _UNISA_SIGNAL_H

typedef int sig_atomic_t;
#define SIG_DFL ((void (*)(int))0)
#define SIG_IGN ((void (*)(int))1)
#define SIG_ERR ((void (*)(int))(0-1))
#define SIGINT  2
#define SIGILL  4
#define SIGABRT 6
#define SIGFPE  8
#define SIGSEGV 11
#define SIGHUP  1
#define SIGQUIT 3
#define SIGKILL 9
#define SIGTERM 15
#define SIGWINCH 28                 /* same number on Linux and macOS; recorded, never delivered (above) */
#define _UNISA_NSIG 32
static void (*_unisa_sig[_UNISA_NSIG])(int);

#if !__UNISA_FTRIM_LIBC || __UN_signal
static void (*signal(int __u_sig, void (*__u_fn)(int)))(int) {
    void (*__u_old)(int);
    if (__u_sig <= 0 || __u_sig >= _UNISA_NSIG) return SIG_ERR;
    __u_old = _unisa_sig[__u_sig];
    _unisa_sig[__u_sig] = __u_fn;
    return __u_old;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_raise
static int raise(int __u_sig) {
    void (*__u_fn)(int);
    if (__u_sig <= 0 || __u_sig >= _UNISA_NSIG) return 0 - 1;
    __u_fn = _unisa_sig[__u_sig];
    if (__u_fn == SIG_IGN) return 0;
    if (__u_fn != SIG_DFL) {
        __u_fn(__u_sig);
        return 0;
    }
    __exit(128 + __u_sig);
    return 0;
}
#endif

/* sigaction and the signal sets (0.0.34 L1b, for lua built with LUA_USE_POSIX).
 * They share signal()'s in-process table, so the same rule holds: a handler is
 * recorded and raise() calls it; nothing arrives from outside.  The mask and
 * flags are kept for the caller to read back and otherwise have no effect;
 * sigprocmask answers the empty set. */

typedef __u_uword sigset_t;
struct sigaction {
    void (*sa_handler)(int);
    sigset_t sa_mask;
    int sa_flags;
};
#define SA_RESTART   2
#define SA_RESETHAND 4
#define SA_NODEFER   16
#define SIG_BLOCK    1
#define SIG_UNBLOCK  2
#define SIG_SETMASK  3
#if !__UNISA_FTRIM_LIBC || __UN_sigemptyset
static int sigemptyset(sigset_t *__u_s) { *__u_s = 0; return 0; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_sigfillset
static int sigfillset(sigset_t *__u_s) { *__u_s = ~(sigset_t)0; return 0; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_sigaddset
static int sigaddset(sigset_t *__u_s, int __u_sig) {
    if (__u_sig <= 0 || __u_sig >= _UNISA_NSIG) { errno = EINVAL; return 0 - 1; }
    *__u_s = *__u_s | ((sigset_t)1 << __u_sig); return 0;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_sigdelset
static int sigdelset(sigset_t *__u_s, int __u_sig) {
    if (__u_sig <= 0 || __u_sig >= _UNISA_NSIG) { errno = EINVAL; return 0 - 1; }
    *__u_s = *__u_s & ~((sigset_t)1 << __u_sig); return 0;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_sigismember
static int sigismember(const sigset_t *__u_s, int __u_sig) {
    if (__u_sig <= 0 || __u_sig >= _UNISA_NSIG) { errno = EINVAL; return 0 - 1; }
    return (int)((*__u_s >> __u_sig) & 1);
}
#endif
static sigset_t _unisa_sigmask[_UNISA_NSIG];
static int _unisa_sigflags[_UNISA_NSIG];
#if !__UNISA_FTRIM_LIBC || __UN_sigaction
static int sigaction(int __u_sig, const struct sigaction *__u_act, struct sigaction *__u_old) {
    if (__u_sig <= 0 || __u_sig >= _UNISA_NSIG || ((__u_sig == SIGKILL) && __u_act)) { errno = EINVAL; return 0 - 1; }
    if (__u_old) {
        __u_old->sa_handler = _unisa_sig[__u_sig];
        __u_old->sa_mask = _unisa_sigmask[__u_sig];
        __u_old->sa_flags = _unisa_sigflags[__u_sig];
    }
    if (__u_act) {
        _unisa_sig[__u_sig] = __u_act->sa_handler;
        _unisa_sigmask[__u_sig] = __u_act->sa_mask;
        _unisa_sigflags[__u_sig] = __u_act->sa_flags;
    }
    return 0;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_sigprocmask
static int sigprocmask(int __u_how, const sigset_t *__u_set, sigset_t *__u_old) {
    if (__u_old) *__u_old = 0;
    if (__u_set && (__u_how < SIG_BLOCK || __u_how > SIG_SETMASK)) { errno = EINVAL; return 0 - 1; }
    return 0;
}
#endif

/* kill (0.0.24, cdsh request): send a signal to another process.  POSIX targets
 * use the kernel call; signal 0 only checks that the process exists.  Windows
 * (pid = the posix_spawn HANDLE, as in waitpid): any nonzero signal ends the
 * process with exit code 128 + sig, which waitpid then reports; there is no
 * delivery to a handler there. */



#if !__UNISA_FTRIM_LIBC || __UN_kill
static int kill(pid_t __u_pid, int __u_sig) {
#ifdef _WIN32
    static __u_word __u_t;
    if (__u_pid <= 0 || __u_sig < 0 || __u_sig >= _UNISA_NSIG) { errno = EINVAL; return -1; }
    if (__u_sig == 0) return 0;
    if (!__u_t) __u_t = _ux_sym("TerminateProcess");
    if (!(_ux_call(__u_t, (__u_word)__u_pid, 128 + __u_sig, 0, 0) & 0xFFFFFFFFL)) return (int)_ux_fail();
    return 0;
#elif defined(__APPLE__)
#if defined(__x86_64__)
    return (int)_unisa_ret(__syscall6(0x2000000L + 37, (__u_word)__u_pid, (__u_word)__u_sig, 1, 0, 0));
#else
    return (int)_unisa_ret(__syscall6(37, (__u_word)__u_pid, (__u_word)__u_sig, 1, 0, 0));
#endif
#elif defined(__x86_64__)
    return (int)_unisa_ret(__syscall6(62, (__u_word)__u_pid, (__u_word)__u_sig, 0, 0, 0));
#else
    return (int)_unisa_ret(__syscall6(129, (__u_word)__u_pid, (__u_word)__u_sig, 0, 0, 0));
#endif
}
#endif
#endif


/* <sys/stat.h> (0.0.18 R18-10): struct stat and stat/lstat/fstat/chmod/mkdir
 * over the kernel's own calls -- no host libc.  The layout is the kernel's for
 * each target (Linux x86_64 and arm64 differ; macOS is stat64).  Windows
 * (0.0.21 batch 1): a __u_word-field struct stat and stat/fstat/mkdir over
 * kernel32 (sys/_win.h); lstat is stat there (no symlinks followed or not),
 * chmod maps S_IWUSR to the read-only attribute (0.0.21 batch 2). */
#ifndef _UNISA_SYS_STAT_H
#define _UNISA_SYS_STAT_H




#define S_IFMT   0170000
#define S_IFIFO  0010000
#define S_IFCHR  0020000
#define S_IFDIR  0040000
#define S_IFBLK  0060000
#define S_IFREG  0100000
#define S_IFLNK  0120000
#define S_IFSOCK 0140000
#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)
#define S_ISLNK(m)  (((m) & S_IFMT) == S_IFLNK)
#define S_ISCHR(m)  (((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)  (((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)
#define S_IRWXU 0700
#define S_IRUSR 0400
#define S_IWUSR 0200
#define S_IXUSR 0100
#define S_IRWXG 070
#define S_IRGRP 040
#define S_IWGRP 020
#define S_IXGRP 010
#define S_IRWXO 07
#define S_IROTH 04
#define S_IWOTH 02
#define S_IXOTH 01
#ifndef _UNISA_TIMESPEC
#define _UNISA_TIMESPEC
struct timespec { __u_word tv_sec; long tv_nsec; };
#endif
#ifdef _WIN32
struct stat {
    __u_word st_dev; __u_word st_ino; __u_word st_mode; __u_word st_nlink; __u_word st_uid; __u_word st_gid;
    __u_word st_rdev; __u_word st_size; __u_word st_atime; __u_word st_mtime; __u_word st_ctime;
};
#elif defined(__APPLE__)
struct stat {                                   /* struct stat64 */
    int st_dev; unsigned short st_mode; unsigned short st_nlink; __u_uword st_ino;
    unsigned int st_uid; unsigned int st_gid; int st_rdev;
    struct timespec st_atimespec, st_mtimespec, st_ctimespec, st_birthtimespec;
    __u_word st_size; __u_word st_blocks; int st_blksize; unsigned int st_flags; unsigned int st_gen;
    int st_lspare; __u_word st_qspare[2];
};
#define st_atime st_atimespec.tv_sec
#define st_mtime st_mtimespec.tv_sec
#define st_ctime st_ctimespec.tv_sec
#else
#ifdef __aarch64__
struct stat {                                   /* asm-generic */
    __u_uword st_dev; __u_uword st_ino; unsigned int st_mode; unsigned int st_nlink;
    unsigned int st_uid; unsigned int st_gid; __u_uword st_rdev; __u_uword __u_pad1;
    __u_word st_size; int st_blksize; int __u_pad2; __u_word st_blocks;
    __u_word st_atime; __u_word st_atime_nsec; __u_word st_mtime; __u_word st_mtime_nsec; __u_word st_ctime; __u_word st_ctime_nsec;
    unsigned int __u_unused[2];
};
#else
struct stat {                                   /* x86_64 */
    __u_uword st_dev; __u_uword st_ino; __u_uword st_nlink;
    unsigned int st_mode; unsigned int st_uid; unsigned int st_gid; int __u_pad0;
    __u_uword st_rdev; __u_word st_size; __u_word st_blksize; __u_word st_blocks;
    __u_word st_atime; __u_word st_atime_nsec; __u_word st_mtime; __u_word st_mtime_nsec; __u_word st_ctime; __u_word st_ctime_nsec;
    __u_word __u_unused[3];
};
#endif
#endif
#if !__UNISA_FTRIM_LIBC || __UN_stat
static int stat(const char *__u_p, struct stat *__u_b) {
#ifdef _WIN32
    static __u_word __u_f; unsigned int __u_d[9];   /* GetFileAttributesExA: WIN32_FILE_ATTRIBUTE_DATA */
    if (!__u_f) __u_f = _ux_sym("GetFileAttributesExA");
    if (!(_ux_call(__u_f, (__u_word)__u_p, 0, (__u_word)__u_d, 0) & 0xFFFFFFFFL)) return _ux_fail();
    __u_b->st_dev = 0; __u_b->st_ino = 0; __u_b->st_nlink = 1; __u_b->st_uid = 0; __u_b->st_gid = 0; __u_b->st_rdev = 0;
    __u_b->st_mode = (__u_d[0] & 0x10) ? (S_IFDIR | 0755) : (S_IFREG | ((__u_d[0] & 1) ? 0444 : 0644));
    __u_b->st_ctime = _ux_ft2unix(__u_d + 1); __u_b->st_atime = _ux_ft2unix(__u_d + 3); __u_b->st_mtime = _ux_ft2unix(__u_d + 5);
    __u_b->st_size = ((__u_word)__u_d[7] << 32) | (__u_word)__u_d[8];
    return 0;
#else
    __u_word __u_r; __u_r = __stat((char *)__u_p, (char *)__u_b, 0);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; } return 0;
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_lstat
static int lstat(const char *__u_p, struct stat *__u_b) {
#ifdef _WIN32
    return stat(__u_p, __u_b);
#else
    __u_word __u_r; __u_r = __lstat((char *)__u_p, (char *)__u_b, 256);   /* AT_SYMLINK_NOFOLLOW for newfstatat */
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; } return 0;
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_fstat
static int fstat(int __u_fd, struct stat *__u_b) {
#ifdef _WIN32
    static __u_word __u_ft; static __u_word __u_fi; __u_word __u_k; unsigned int __u_d[13];
    if (!__u_ft) { __u_ft = _ux_sym("GetFileType"); __u_fi = _ux_sym("GetFileInformationByHandle"); }
    __u_b->st_dev = 0; __u_b->st_ino = 0; __u_b->st_nlink = 1; __u_b->st_uid = 0; __u_b->st_gid = 0; __u_b->st_rdev = 0;
    __u_b->st_size = 0; __u_b->st_atime = 0; __u_b->st_mtime = 0; __u_b->st_ctime = 0;
    __u_k = _ux_call(__u_ft, (__u_word)__u_fd, 0, 0, 0) & 0xFFFFFFFFL;
    if (__u_k == 2) { __u_b->st_mode = S_IFCHR | 0666; return 0; }            /* FILE_TYPE_CHAR */
    if (__u_k == 3) { __u_b->st_mode = S_IFIFO | 0666; return 0; }            /* FILE_TYPE_PIPE */
    if (!(_ux_call(__u_fi, (__u_word)__u_fd, (__u_word)__u_d, 0, 0) & 0xFFFFFFFFL)) return _ux_fail();
    __u_b->st_mode = (__u_d[0] & 0x10) ? (S_IFDIR | 0755) : (S_IFREG | ((__u_d[0] & 1) ? 0444 : 0644));
    __u_b->st_ctime = _ux_ft2unix(__u_d + 1); __u_b->st_atime = _ux_ft2unix(__u_d + 3); __u_b->st_mtime = _ux_ft2unix(__u_d + 5);
    __u_b->st_dev = __u_d[7]; __u_b->st_size = ((__u_word)__u_d[8] << 32) | (__u_word)__u_d[9];
    __u_b->st_nlink = __u_d[10]; __u_b->st_ino = ((__u_word)__u_d[11] << 32) | (__u_word)__u_d[12];
    return 0;
#else
    __u_word __u_r; __u_r = __fstat(__u_fd, (char *)__u_b, 0);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; } return 0;
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_chmod
static int chmod(const char *__u_p, mode_t __u_m) {
#ifdef _WIN32
    static __u_word __u_ga; static __u_word __u_sa; __u_word __u_at;
    if (!__u_ga) { __u_ga = _ux_sym("GetFileAttributesA"); __u_sa = _ux_sym("SetFileAttributesA"); }
    __u_at = _ux_call(__u_ga, (__u_word)__u_p, 0, 0, 0) & 0xFFFFFFFFL;
    if (__u_at == 0xFFFFFFFFL) return _ux_fail();
    __u_at = (__u_m & 0200) ? (__u_at & ~1L) : (__u_at | 1L);         /* FILE_ATTRIBUTE_READONLY */
    if (!(_ux_call(__u_sa, (__u_word)__u_p, __u_at, 0, 0) & 0xFFFFFFFFL)) return _ux_fail();
    return 0;
#else
    __u_word __u_r; __u_r = __chmod((char *)__u_p, (__u_word)__u_m, 0);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; } return 0;
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_mkdir
static int mkdir(const char *__u_p, mode_t __u_m) {
#ifdef _WIN32
    static __u_word __u_f;                         /* CreateDirectoryA (the mode is not mapped) */
    if (!__u_f) __u_f = _ux_sym("CreateDirectoryA");
    if (_ux_call(__u_f, (__u_word)__u_p, 0, 0, 0) & 0xFFFFFFFFL) return 0;
    return _ux_fail();
#else
    __u_word __u_r; __u_r = __mkdir((char *)__u_p, (__u_word)__u_m, 0);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; } return 0;
#endif
}
#endif
/* *at() flags (0.0.35 M1, minicon's loader): each kernel's own values */
#ifndef AT_FDCWD
#ifdef __APPLE__
#define AT_FDCWD (0 - 2)
#define AT_SYMLINK_NOFOLLOW 0x20
#define AT_REMOVEDIR 0x80
#else
#define AT_FDCWD (0 - 100)
#define AT_SYMLINK_NOFOLLOW 0x100
#define AT_REMOVEDIR 0x200
#endif
#endif
#if !__UNISA_FTRIM_LIBC || __UN_fstatat
static int fstatat(int __u_dfd, const char *__u_p, struct stat *__u_b, int __u_fl) {
#ifdef _WIN32
    if (__u_dfd != AT_FDCWD) { errno = ENOSYS; return -1; }
    return stat(__u_p, __u_b);
#else
    __u_word __u_r;
#if defined(__APPLE__)
#if defined(__x86_64__)
    __u_r = __syscall6(0x2000000L + 470, (__u_word)__u_dfd, (__u_word)__u_p, (__u_word)__u_b, (__u_word)__u_fl, 0);
#else
    __u_r = __syscall6(470, (__u_word)__u_dfd, (__u_word)__u_p, (__u_word)__u_b, (__u_word)__u_fl, 0);
#endif
#elif defined(__x86_64__)
    __u_r = __syscall6(262, (__u_word)__u_dfd, (__u_word)__u_p, (__u_word)__u_b, (__u_word)__u_fl, 0);
#else
    __u_r = __syscall6(79, (__u_word)__u_dfd, (__u_word)__u_p, (__u_word)__u_b, (__u_word)__u_fl, 0);
#endif
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; } return 0;
#endif
}
#endif
/* 0.0.28 H4: forwarded to the system C library on Linux and macOS */
#ifndef _WIN32
int fchmod(int __u_fd, mode_t __u_mode);
#endif
#endif


/* Minimal directory API: Linux getdents64, macOS getdirentries64 (0.0.18); Windows
 * FindFirstFileA/FindNextFileA/FindClose (0.0.21 batch 2: "." and ".." are
 * returned there too, in the system's order -- portable code sorts or filters).
 * EOF sets errno=0; malformed kernel records set EIO and stop this stream. */
#ifndef _UNISA_DIRENT_H
#define _UNISA_DIRENT_H




struct dirent {
    __u_uword d_ino;
    unsigned char d_type;
    char d_name[256];
};
#define _UNISA_DIRBUF 4096
#ifdef __APPLE__
#define _UNISA_DIRNAME 21
#else
#define _UNISA_DIRNAME 19
#endif
typedef struct {
    int fd, pos, len, failed;
    __u_word basep;                                  /* macOS getdirentries64 position */
    long index;                                  /* entries returned so far (telldir) */
    struct dirent ent;
    unsigned char buf[_UNISA_DIRBUF];
} DIR;
/* Windows layout inside DIR: fd unused, basep = the find handle (0 = none yet,
   -1 = exhausted), buf[0..319] = WIN32_FIND_DATAA (cFileName at 44),
   buf[512..] = the "path\\*" pattern kept for rewinddir. */
#if !__UNISA_FTRIM_LIBC || __UN_opendir
static DIR *opendir(const char *__u_path) {
    DIR *__u_d; __u_word __u_fd;
#ifdef _WIN32
    static __u_word __u_ff; static __u_word __u_fc; __u_word __u_h; int __u_n;
    if (!__u_ff) { __u_ff = _ux_sym("FindFirstFileA"); __u_fc = _ux_sym("FindClose"); }
    for (__u_n = 0; __u_path[__u_n]; __u_n++) ;
    if (__u_n == 0) { errno = ENOENT; return 0; }
    if (__u_n > 3000) { errno = ENAMETOOLONG; return 0; }
    __u_d = (DIR *)malloc(sizeof(DIR));
    if (__u_d == 0) { errno = ENOMEM; return 0; }
    for (__u_fd = 0; __u_fd < __u_n; __u_fd++) __u_d->buf[512 + __u_fd] = (unsigned char)__u_path[__u_fd];
    if (__u_path[__u_n - 1] != '\\' && __u_path[__u_n - 1] != '/') __u_d->buf[512 + __u_n++] = '\\';
    __u_d->buf[512 + __u_n] = '*'; __u_d->buf[513 + __u_n] = 0;
    __u_h = _ux_call(__u_ff, (__u_word)(__u_d->buf + 512), (__u_word)__u_d->buf, 0, 0);
    if (__u_h == -1 || __u_h == 0) { __u_fd = _ux_errno(); free(__u_d); errno = (int)__u_fd; return 0; }
    if (!(__u_d->buf[0] & 0x10)) { _ux_call(__u_fc, __u_h, 0, 0, 0); free(__u_d); errno = ENOTDIR; return 0; }  /* "x\\*" of a file */
    __u_d->fd = -1; __u_d->pos = 1; __u_d->len = 0; __u_d->failed = 0; __u_d->basep = __u_h; __u_d->index = 0;
    errno = 0; return __u_d;
#else
#ifdef __APPLE__
    __u_fd = __open((char *)__u_path, 0x100000, 0); /* macOS O_DIRECTORY */
#else
#ifdef __aarch64__
    __u_fd = __open((char *)__u_path, 16384, 0); /* Linux arm64 O_DIRECTORY */
#else
    __u_fd = __open((char *)__u_path, 65536, 0); /* Linux x86_64 O_DIRECTORY */
#endif
#endif
    if (__u_fd < 0) { errno = (int)(0 - __u_fd); return 0; }
    __u_d = (DIR *)malloc(sizeof(DIR));
    if (__u_d == 0) { __close(__u_fd); errno = ENOMEM; return 0; }
    __u_d->fd = (int)__u_fd; __u_d->pos = 0; __u_d->len = 0; __u_d->failed = 0; __u_d->basep = 0; __u_d->index = 0;
    errno = 0; return __u_d;
#endif
}
#endif
/* fdopendir/dirfd (POSIX, 0.0.35 M1): the stream takes over FD, which must be an open directory */
#if !__UNISA_FTRIM_LIBC || __UN_fdopendir
static DIR *fdopendir(int __u_fd) {
    DIR *__u_d;
#ifdef _WIN32
    (void)__u_fd; errno = ENOSYS; return 0;
#else
    if (__u_fd < 0) { errno = EBADF; return 0; }
    __u_d = (DIR *)malloc(sizeof(DIR));
    if (__u_d == 0) { errno = ENOMEM; return 0; }
    __u_d->fd = __u_fd; __u_d->pos = 0; __u_d->len = 0; __u_d->failed = 0; __u_d->basep = 0; __u_d->index = 0;
    return __u_d;
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_dirfd
static int dirfd(DIR *__u_d) { if (!__u_d || __u_d->fd < 0) { errno = EINVAL; return -1; } return __u_d->fd; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_readdir
static struct dirent *readdir(DIR *__u_d) {
    unsigned char *__u_r; int __u_rl, __u_i, __u_left; __u_word __u_n;
    errno = 0;
    if (__u_d == 0) { errno = EBADF; return 0; }
    if (__u_d->failed) { errno = __u_d->failed; return 0; }
#ifdef _WIN32
    {   static __u_word __u_fn; static __u_word __u_ff; __u_word __u_e;
        if (!__u_fn) { __u_fn = _ux_sym("FindNextFileA"); __u_ff = _ux_sym("FindFirstFileA"); }
        if (__u_d->basep == -1) return 0;
        if (__u_d->basep == 0) {                     /* after rewinddir */
            __u_d->basep = _ux_call(__u_ff, (__u_word)(__u_d->buf + 512), (__u_word)__u_d->buf, 0, 0);
            if (__u_d->basep == -1 || __u_d->basep == 0) { __u_d->basep = -1; return 0; }
        } else if (!__u_d->pos) {
            if (!(_ux_call(__u_fn, __u_d->basep, (__u_word)__u_d->buf, 0, 0) & 0xFFFFFFFFL)) {
                __u_e = _ux_errno(); errno = 0;
                if (__u_e != EIO) { __u_d->failed = (int)__u_e; errno = (int)__u_e; }   /* ERROR_NO_MORE_FILES maps to EIO: the end */
                return 0;
            }
        }
        __u_d->pos = 0;
        for (__u_i = 0; __u_i < 255 && __u_d->buf[44 + __u_i]; __u_i++) __u_d->ent.d_name[__u_i] = (char)__u_d->buf[44 + __u_i];
        __u_d->ent.d_name[__u_i] = 0;
        __u_d->ent.d_ino = 0; __u_d->ent.d_type = (__u_d->buf[0] & 0x10) ? 4 : 8;   /* DT_DIR / DT_REG */
        __u_d->index = __u_d->index + 1;
        return &__u_d->ent;
    }
#else
    if (__u_d->pos == __u_d->len) {
#ifdef __APPLE__
        __u_n = __getdirentries64(__u_d->fd, __u_d->buf, _UNISA_DIRBUF, (char *)&__u_d->basep, 0, 0);   /* R18-10 */
#else
        __u_n = __getdents64(__u_d->fd, __u_d->buf, _UNISA_DIRBUF);
#endif
        if (__u_n < 0) { __u_d->failed = (int)(0 - __u_n); errno = __u_d->failed; return 0; }
        if (__u_n == 0) return 0;
        if (__u_n > _UNISA_DIRBUF) { __u_d->failed = EIO; errno = EIO; return 0; }
        __u_d->len = (int)__u_n; __u_d->pos = 0;
    }
    __u_left = __u_d->len - __u_d->pos;
    if (__u_left < 24) { __u_d->failed = EIO; errno = EIO; return 0; }
    __u_r = __u_d->buf + __u_d->pos;
    __u_rl = __u_r[16] | (__u_r[17] << 8);
    /* linux_dirent64: d_type at 18, name at 19, records 8-aligned; macOS
       struct direntry: d_namlen at 18, d_type at 20, name at 21, 4-aligned */
    if (__u_rl < 24 || (__u_rl & (_UNISA_DIRNAME == 21 ? 3 : 7)) || __u_rl > __u_left) {
        __u_d->failed = EIO; errno = EIO; return 0;
    }
    for (__u_i = 0; __u_i < 256 && _UNISA_DIRNAME + __u_i < __u_rl && __u_r[_UNISA_DIRNAME + __u_i]; __u_i++) ;
    if (__u_i == 0 || __u_i >= 256 || _UNISA_DIRNAME + __u_i >= __u_rl) {
        __u_d->failed = EIO; errno = EIO; return 0;
    }
    __u_d->ent.d_name[__u_i] = 0;
    while (__u_i > 0) { __u_i--; __u_d->ent.d_name[__u_i] = (char)__u_r[_UNISA_DIRNAME + __u_i]; }
    __u_d->ent.d_ino = 0;
    for (__u_i = 7; __u_i >= 0; __u_i--)
        __u_d->ent.d_ino = (__u_d->ent.d_ino << 8) | __u_r[__u_i];
    __u_d->ent.d_type = __u_r[_UNISA_DIRNAME - 1]; __u_d->pos += __u_rl; __u_d->index = __u_d->index + 1;
    return &__u_d->ent;
#endif
}
#endif
/* 0.0.19 (dsh): rewinddir/telldir/seekdir.  A position is the number of
   entries already returned; seekdir rewinds and reads forward to it (POSIX
   only promises telldir's value is meaningful to seekdir on the same DIR). */
#if !__UNISA_FTRIM_LIBC || __UN_rewinddir
static void rewinddir(DIR *__u_d) {
    if (__u_d == 0) return;
#ifdef _WIN32
    {   static __u_word __u_fc;
        if (!__u_fc) __u_fc = _ux_sym("FindClose");
        if (__u_d->basep != -1 && __u_d->basep != 0) _ux_call(__u_fc, __u_d->basep, 0, 0, 0);
        __u_d->basep = 0; __u_d->pos = 1; __u_d->failed = 0; __u_d->index = 0;
        return;
    }
#else
    __lseek(__u_d->fd, 0, 0);
#endif
    __u_d->pos = 0; __u_d->len = 0; __u_d->failed = 0; __u_d->basep = 0; __u_d->index = 0;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_telldir
static long telldir(DIR *__u_d) { return __u_d ? __u_d->index : -1; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_seekdir
static void seekdir(DIR *__u_d, long __u_loc) {
    if (__u_d == 0) return;
    rewinddir(__u_d);
    while (__u_d->index < __u_loc) { if (readdir(__u_d) == 0) break; }
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_closedir
static int closedir(DIR *__u_d) {
    __u_word __u_r;
    if (__u_d == 0) { errno = EBADF; return -1; }
#ifdef _WIN32
    {   static __u_word __u_fc;
        if (!__u_fc) __u_fc = _ux_sym("FindClose");
        __u_r = 0;
        if (__u_d->basep != -1 && __u_d->basep != 0) __u_r = _ux_call(__u_fc, __u_d->basep, 0, 0, 0) & 0xFFFFFFFFL;
        else __u_r = 1;
        free(__u_d);
        if (!__u_r) { errno = EBADF; return -1; }
        return 0;
    }
#else
    __u_r = __close(__u_d->fd); free(__u_d);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; }
    return 0;
#endif
}
#endif
#endif


/* sys/wait.h -- 0.0.19 R19-5: waitpid/wait over wait4 (generic gate), and the
 * status macros (the same encoding on Linux and macOS).  Windows: waitpid on a posix_spawn HANDLE. */
#ifndef _UNISA_SYS_WAIT_H
#define _UNISA_SYS_WAIT_H




#define WNOHANG 1
#define WUNTRACED 2
#define WIFEXITED(s) (((s) & 0x7f) == 0)
#define WEXITSTATUS(s) (((s) >> 8) & 0xff)
#define WIFSIGNALED(s) (((s) & 0x7f) != 0 && ((s) & 0x7f) != 0x7f)
#define WTERMSIG(s) ((s) & 0x7f)
#define WIFSTOPPED(s) (((s) & 0xff) == 0x7f)
#define WSTOPSIG(s) (((s) >> 8) & 0xff)
#if !defined(_WIN32) || !defined(_UNISA_NO_HOSTCALL)
#if !__UNISA_FTRIM_LIBC || __UN_waitpid
static pid_t waitpid(pid_t __u_pid, int *__u_st, int __u_opt) {
#ifdef _WIN32
    /* Windows (0.0.22): a pid is the process HANDLE from posix_spawn; the exit code is
       encoded as an exited status.  pid -1 has no meaning here and answers ECHILD. */
    static __u_word __u_w, __u_x, __u_c; unsigned int __u_code;
    if (__u_pid <= 0) { errno = ECHILD; return -1; }
    if (!__u_w) { __u_w = _ux_sym("WaitForSingleObject"); __u_x = _ux_sym("GetExitCodeProcess"); __u_c = _ux_sym("CloseHandle"); }
    if ((_ux_call(__u_w, (__u_word)__u_pid, (__u_opt & WNOHANG) ? 0 : 0xFFFFFFFFL, 0, 0) & 0xFFFFFFFFL) == 0x102) return 0;
    __u_code = 0;
    if (!(_ux_call(__u_x, (__u_word)__u_pid, (__u_word)&__u_code, 0, 0) & 0xFFFFFFFFL)) return _ux_fail();
    _ux_call(__u_c, (__u_word)__u_pid, 0, 0, 0);
    if (__u_st) *__u_st = (int)((__u_code & 0xff) << 8);
    return __u_pid;
#else
    return (pid_t)_unisa_ret(__syscall6(_UNISA_SC(_UNISA_NR_wait4), (__u_word)__u_pid, (__u_word)__u_st, (__u_word)__u_opt, 0, 0));
#endif
}
#endif
#endif
#ifndef _WIN32
#if !__UNISA_FTRIM_LIBC || __UN_wait
static pid_t wait(int *__u_st) { return waitpid(-1, __u_st, 0); }
#endif
#endif
#endif


/* <spawn.h>: posix_spawn/posix_spawnp over fork + file actions + execve/execvp.
 * The child applies the file actions (at most 16) in order, then execs; an
 * action or exec failure ends the child with _exit(127).  Returns 0 or an errno value (errno
 * itself is not set), the child's pid through the out parameter.  Attributes
 * are accepted and ignored (init/destroy only).
 * Windows (0.0.22 POSIX batch 5): CreateProcessA with a quoted command line; the pid is the
 * process HANDLE (waitpid waits on it).  File actions may only target fds 0-2 (they become the
 * child's standard handles); addclose is accepted and has no effect (handles are not inherited
 * unless named); any other target answers ENOSYS. */
#ifndef _UNISA_SPAWN_H
#define _UNISA_SPAWN_H




/* <fcntl.h> (0.0.18 R18-10): the O_* and F_* constants of each target's
 * kernel, open() and fcntl() over its own calls.  Windows (0.0.21 batch 1):
 * the mingw O_* values and open() over CreateFileA; the fd is the HANDLE. */
#ifndef _UNISA_FCNTL_H
#define _UNISA_FCNTL_H




#ifdef _WIN32
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR   2
#define O_ACCMODE 3
#define O_APPEND 0x8
#define O_CREAT  0x100
#define O_TRUNC  0x200
#define O_EXCL   0x400
#else
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR   2
#define O_ACCMODE 3
#ifdef __APPLE__
#define O_NONBLOCK  0x4
#define O_APPEND    0x8
#define O_CREAT     0x200
#define O_TRUNC     0x400
#define O_EXCL      0x800
#define O_NOFOLLOW  0x100
#define O_DIRECTORY 0x100000
#define O_CLOEXEC   0x1000000
#else
#define O_CREAT     0100
#define O_EXCL      0200
#define O_NOCTTY    0400
#define O_TRUNC     01000
#define O_APPEND    02000
#define O_NONBLOCK  04000
#define O_CLOEXEC   02000000
#ifdef __aarch64__
#define O_DIRECTORY 040000
#define O_NOFOLLOW  0100000
#else
#define O_DIRECTORY 0200000
#define O_NOFOLLOW  0400000
#endif
#endif
#define F_DUPFD 0
#define F_GETFD 1
#define F_SETFD 2
#define F_GETFL 3
#define F_SETFL 4
#define FD_CLOEXEC 1
#endif
#if !__UNISA_FTRIM_LIBC || __UN_open
static int open(const char *__u_p, int __u_flags, int __u_mode) {
    __u_word __u_r;
#ifdef _WIN32
    __u_word __u_acc; __u_word __u_disp;              /* the __open gate is CreateFileA(path, access, disposition) */
    __u_acc = 0x80000000L;                                        /* GENERIC_READ */
    if ((__u_flags & O_ACCMODE) == O_WRONLY) __u_acc = 0x40000000L;
    if ((__u_flags & O_ACCMODE) == O_RDWR) __u_acc = 0xC0000000L;
    __u_disp = 3;                                                 /* OPEN_EXISTING */
    if (__u_flags & O_CREAT) {
        if (__u_flags & O_EXCL) __u_disp = 1;                     /* CREATE_NEW */
        else if (__u_flags & O_TRUNC) __u_disp = 2;               /* CREATE_ALWAYS */
        else __u_disp = 4;                                        /* OPEN_ALWAYS */
    } else if (__u_flags & O_TRUNC) __u_disp = 5;                 /* TRUNCATE_EXISTING */
    __u_r = __open((char *)__u_p, __u_acc, __u_disp);
    if (__u_r < 0) return _ux_fail();
    if (__u_flags & O_APPEND) __lseek((int)__u_r, 0, 2);          /* SetFilePointer to the end */
    return (int)__u_r;
#else
    __u_r = __open((char *)__u_p, __u_flags, __u_mode);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; } return (int)__u_r;
#endif
}
#endif
#ifndef _WIN32
/* 0.0.28 H4 (SQLite's locks): struct flock and the record-lock commands, per OS */
#ifdef __APPLE__
struct flock { off_t l_start; off_t l_len; pid_t l_pid; short l_type; short l_whence; };
#define F_GETLK  7
#define F_SETLK  8
#define F_SETLKW 9
#define F_RDLCK  1
#define F_UNLCK  2
#define F_WRLCK  3
#else
struct flock { short l_type; short l_whence; off_t l_start; off_t l_len; pid_t l_pid; };
#define F_GETLK  5
#define F_SETLK  6
#define F_SETLKW 7
#define F_RDLCK  0
#define F_WRLCK  1
#define F_UNLCK  2
#endif
#endif
#ifndef _WIN32
#if !__UNISA_FTRIM_LIBC || __UN_fcntl
static int fcntl(int __u_fd, int __u_cmd, __u_word __u_arg) {
    __u_word __u_r; __u_r = __fcntl(__u_fd, __u_cmd, __u_arg);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; } return (int)__u_r;
}
#endif
#endif
#endif


typedef struct { int __u_flags; } posix_spawnattr_t;
#define _UNISA_SPAWN_MAX 16
struct _unisa_spawn_act { int __u_op; int __u_fd; int __u_nfd; int __u_oflag; mode_t __u_mode; char __u_path[256]; };
typedef struct { int __u_n; struct _unisa_spawn_act __u_a[_UNISA_SPAWN_MAX]; } posix_spawn_file_actions_t;
#if !__UNISA_FTRIM_LIBC || __UN_posix_spawnattr_init
static int posix_spawnattr_init(posix_spawnattr_t *__u_at) { __u_at->__u_flags = 0; return 0; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_posix_spawnattr_destroy
static int posix_spawnattr_destroy(posix_spawnattr_t *__u_at) { (void)__u_at; return 0; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_posix_spawn_file_actions_init
static int posix_spawn_file_actions_init(posix_spawn_file_actions_t *__u_fa) { __u_fa->__u_n = 0; return 0; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_posix_spawn_file_actions_destroy
static int posix_spawn_file_actions_destroy(posix_spawn_file_actions_t *__u_fa) { __u_fa->__u_n = 0; return 0; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_posix_spawn_file_actions_adddup2
static int posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t *__u_fa, int __u_fd, int __u_nfd) {
    if (__u_fd < 0 || __u_nfd < 0) return EBADF;
    if (__u_fa->__u_n >= _UNISA_SPAWN_MAX) return ENOMEM;
    __u_fa->__u_a[__u_fa->__u_n].__u_op = 1; __u_fa->__u_a[__u_fa->__u_n].__u_fd = __u_fd; __u_fa->__u_a[__u_fa->__u_n].__u_nfd = __u_nfd;
    __u_fa->__u_n = __u_fa->__u_n + 1; return 0;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_posix_spawn_file_actions_addclose
static int posix_spawn_file_actions_addclose(posix_spawn_file_actions_t *__u_fa, int __u_fd) {
    if (__u_fd < 0) return EBADF;
    if (__u_fa->__u_n >= _UNISA_SPAWN_MAX) return ENOMEM;
    __u_fa->__u_a[__u_fa->__u_n].__u_op = 2; __u_fa->__u_a[__u_fa->__u_n].__u_fd = __u_fd;
    __u_fa->__u_n = __u_fa->__u_n + 1; return 0;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_posix_spawn_file_actions_addopen
static int posix_spawn_file_actions_addopen(posix_spawn_file_actions_t *__u_fa, int __u_fd, const char *__u_p, int __u_of, mode_t __u_m) {
    int __u_i; struct _unisa_spawn_act *__u_x;
    if (__u_fd < 0) return EBADF;
    if (__u_fa->__u_n >= _UNISA_SPAWN_MAX) return ENOMEM;
    __u_x = &__u_fa->__u_a[__u_fa->__u_n];
    __u_i = 0; while (__u_p[__u_i]) { if (__u_i >= 255) return ENAMETOOLONG; __u_x->__u_path[__u_i] = __u_p[__u_i]; __u_i = __u_i + 1; }
    __u_x->__u_path[__u_i] = 0;
    __u_x->__u_op = 3; __u_x->__u_fd = __u_fd; __u_x->__u_oflag = __u_of; __u_x->__u_mode = __u_m;
    __u_fa->__u_n = __u_fa->__u_n + 1; return 0;
}
#endif
#ifndef _WIN32
#if !__UNISA_FTRIM_LIBC || __UN__unisa_spawn
static int _unisa_spawn(pid_t *__u_pid, const char *__u_path, const posix_spawn_file_actions_t *__u_fa,
                        char *const __u_argv[], char *const __u_envp[], int __u_usepath) {
    pid_t __u_c; int __u_i; int __u_r; const struct _unisa_spawn_act *__u_x;
    __u_c = fork();
    if (__u_c < 0) return errno;
    if (__u_c == 0) {
        if (__u_fa != 0) {
            __u_i = 0;
            while (__u_i < __u_fa->__u_n) {
                __u_x = &__u_fa->__u_a[__u_i];
                if (__u_x->__u_op == 1) { if (dup2(__u_x->__u_fd, __u_x->__u_nfd) < 0) _exit(127); }
                else if (__u_x->__u_op == 2) { close(__u_x->__u_fd); }
                else {
                    __u_r = open(__u_x->__u_path, __u_x->__u_oflag, __u_x->__u_mode);
                    if (__u_r < 0) _exit(127);
                    if (__u_r != __u_x->__u_fd) { if (dup2(__u_r, __u_x->__u_fd) < 0) _exit(127); close(__u_r); }
                }
                __u_i = __u_i + 1;
            }
        }
        if (__u_usepath) execvp(__u_path, __u_argv);
        else if (__u_envp != 0) execve(__u_path, __u_argv, __u_envp);
        else execv(__u_path, __u_argv);
        _exit(127);
    }
    if (__u_pid != 0) *__u_pid = __u_c;
    return 0;
}
#endif
#elif !defined(_UNISA_NO_HOSTCALL)
#if !__UNISA_FTRIM_LIBC || __UN__unisa_inherit
static __u_word _unisa_inherit(__u_word __u_h) {            /* an inheritable duplicate, or 0 */
    static __u_word __u_dh, __u_gp; __u_word __u_a[10] = {0}; __u_word __u_n, __u_self;
    if (!__u_dh) { __u_dh = _ux_sym("DuplicateHandle"); __u_gp = _ux_sym("GetCurrentProcess"); }
    __u_self = _ux_call(__u_gp, 0, 0, 0, 0); __u_n = 0;
    __u_a[0] = __u_self; __u_a[1] = __u_h; __u_a[2] = __u_self; __u_a[3] = (__u_word)&__u_n; __u_a[4] = 0; __u_a[5] = 1; __u_a[6] = 2;
    if (!(__hostcall(__u_dh, __u_a) & 0xFFFFFFFFL)) return 0;
    return __u_n;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__unisa_cmdarg
static int _unisa_cmdarg(char *__u_d, int __u_k, int __u_max, const char *__u_s) {   /* MS C runtime quoting */
    int __u_i, __u_b, __u_q;
    __u_q = __u_s[0] == 0;
    for (__u_i = 0; __u_s[__u_i]; __u_i = __u_i + 1) if (__u_s[__u_i] == 32 || __u_s[__u_i] == 9 || __u_s[__u_i] == 34) __u_q = 1;
    if (!__u_q) { for (__u_i = 0; __u_s[__u_i]; __u_i = __u_i + 1) { if (__u_k >= __u_max) return -1; __u_d[__u_k] = __u_s[__u_i]; __u_k = __u_k + 1; } return __u_k; }
    if (__u_k >= __u_max) return -1; __u_d[__u_k] = 34; __u_k = __u_k + 1;
    for (__u_i = 0; ; __u_i = __u_i + 1) {
        __u_b = 0; while (__u_s[__u_i] == 92) { __u_b = __u_b + 1; __u_i = __u_i + 1; }
        if (__u_s[__u_i] == 0) __u_b = __u_b * 2; else if (__u_s[__u_i] == 34) __u_b = __u_b * 2 + 1;
        while (__u_b > 0) { if (__u_k >= __u_max) return -1; __u_d[__u_k] = 92; __u_k = __u_k + 1; __u_b = __u_b - 1; }
        if (__u_s[__u_i] == 0) break;
        if (__u_k >= __u_max) return -1; __u_d[__u_k] = __u_s[__u_i]; __u_k = __u_k + 1;
    }
    if (__u_k >= __u_max) return -1; __u_d[__u_k] = 34; return __u_k + 1;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__unisa_wspawn
static int _unisa_wspawn(pid_t *__u_pid, const char *__u_path, const posix_spawn_file_actions_t *__u_fa,
                        char *const __u_argv[], char *const __u_envp[], int __u_usepath) {
    static __u_word __u_cp, __u_gs, __u_ch; static char __u_cmd[8192]; static char __u_env[16384];
    __u_word __u_si[13], __u_pi[3], __u_std[3], __u_a[10] = {0}; char *__u_sb; int __u_i, __u_k, __u_r, __u_n, __u_j;
    const struct _unisa_spawn_act *__u_x;
    if (!__u_cp) { __u_cp = _ux_sym("CreateProcessA"); __u_gs = _ux_sym("GetStdHandle"); __u_ch = _ux_sym("CloseHandle"); }
    for (__u_i = 0; __u_i < 3; __u_i = __u_i + 1) __u_std[__u_i] = _ux_call(__u_gs, -10 - __u_i, 0, 0, 0);
    if (__u_fa != 0) for (__u_i = 0; __u_i < __u_fa->__u_n; __u_i = __u_i + 1) {
        __u_x = &__u_fa->__u_a[__u_i];
        if (__u_x->__u_op == 2) continue;
        __u_n = __u_x->__u_op == 1 ? __u_x->__u_nfd : __u_x->__u_fd;
        if (__u_n < 0 || __u_n > 2) return ENOSYS;
        if (__u_x->__u_op == 1) __u_std[__u_n] = __u_x->__u_fd <= 2 ? __u_std[__u_x->__u_fd] : (__u_word)__u_x->__u_fd;
        else { __u_r = open(__u_x->__u_path, __u_x->__u_oflag, __u_x->__u_mode); if (__u_r < 0) return errno; __u_std[__u_n] = (__u_word)__u_r; }
    }
    for (__u_i = 0; __u_i < 13; __u_i = __u_i + 1) __u_si[__u_i] = 0;
    __u_sb = (char *)__u_si; *(int *)__u_sb = 104; *(int *)(__u_sb + 60) = 0x100;   /* cb, STARTF_USESTDHANDLES */
    for (__u_i = 0; __u_i < 3; __u_i = __u_i + 1) __u_si[10 + __u_i] = _unisa_inherit(__u_std[__u_i]);
    __u_k = 0;
    for (__u_i = 0; __u_argv[__u_i]; __u_i = __u_i + 1) {
        if (__u_i) { if (__u_k >= 8190) return E2BIG; __u_cmd[__u_k] = 32; __u_k = __u_k + 1; }
        __u_k = _unisa_cmdarg(__u_cmd, __u_k, 8190, __u_argv[__u_i]); if (__u_k < 0) return E2BIG;
    }
    __u_cmd[__u_k] = 0;
    if (__u_envp != 0) {
        __u_k = 0;
        for (__u_i = 0; __u_envp[__u_i]; __u_i = __u_i + 1) {
            for (__u_j = 0; __u_envp[__u_i][__u_j]; __u_j = __u_j + 1) { if (__u_k >= 16380) return E2BIG; __u_env[__u_k] = __u_envp[__u_i][__u_j]; __u_k = __u_k + 1; }
            __u_env[__u_k] = 0; __u_k = __u_k + 1;
        }
        __u_env[__u_k] = 0; __u_env[__u_k + 1] = 0;
    }
    __u_pi[0] = 0; __u_pi[1] = 0; __u_pi[2] = 0;
    __u_a[0] = __u_usepath ? 0 : (__u_word)__u_path; __u_a[1] = (__u_word)__u_cmd; __u_a[2] = 0; __u_a[3] = 0; __u_a[4] = 1;
    __u_a[5] = 0; __u_a[6] = __u_envp != 0 ? (__u_word)__u_env : 0; __u_a[7] = 0; __u_a[8] = (__u_word)__u_si; __u_a[9] = (__u_word)__u_pi;
    __u_r = (int)(__hostcall(__u_cp, __u_a) & 0xFFFFFFFFL);
    __u_n = __u_r ? 0 : _ux_errno();
    for (__u_i = 0; __u_i < 3; __u_i = __u_i + 1) if (__u_si[10 + __u_i]) _ux_call(__u_ch, __u_si[10 + __u_i], 0, 0, 0);
    if (!__u_r) return __u_n;
    _ux_call(__u_ch, __u_pi[1], 0, 0, 0);
    if (__u_pid != 0) *__u_pid = (pid_t)__u_pi[0];
    return 0;
}
#endif
#endif
#if !defined(_WIN32) || !defined(_UNISA_NO_HOSTCALL)
#if !__UNISA_FTRIM_LIBC || __UN_posix_spawn
static int posix_spawn(pid_t *__u_pid, const char *__u_path, const posix_spawn_file_actions_t *__u_fa,
                       const posix_spawnattr_t *__u_at, char *const __u_argv[], char *const __u_envp[]) {
#ifdef _WIN32
    (void)__u_at; return _unisa_wspawn(__u_pid, __u_path, __u_fa, __u_argv, __u_envp, 0);
#else
    (void)__u_at; return _unisa_spawn(__u_pid, __u_path, __u_fa, __u_argv, __u_envp, 0);
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_posix_spawnp
static int posix_spawnp(pid_t *__u_pid, const char *__u_file, const posix_spawn_file_actions_t *__u_fa,
                        const posix_spawnattr_t *__u_at, char *const __u_argv[], char *const __u_envp[]) {
#ifdef _WIN32
    (void)__u_at; return _unisa_wspawn(__u_pid, __u_file, __u_fa, __u_argv, __u_envp, 1);
#else
    (void)__u_at; (void)__u_envp; return _unisa_spawn(__u_pid, __u_file, __u_fa, __u_argv, __u_envp, 1);
#endif
}
#endif
#endif
#endif

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
   buffer until 0.0.43, where Python's open() raises). Windows fopen does not fail that way, so
   a directory is rejected with stat. */
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
    {   struct stat __u_st;
        if (stat(__u_path, &__u_st) == 0 && S_ISDIR(__u_st.st_mode)) {
            fclose(__u_f);
            if (__u_len) *__u_len = 0;
            return 0;
        }
    }
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
    char *__u_argv[4];
    __u_argv[0] = "sh"; __u_argv[1] = "-c"; __u_argv[2] = (char *)__u_cmd; __u_argv[3] = 0;
    return cx_runv_timeout(__u_argv, 0, __u_out, __u_len);
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
    char *__u_argv[4];
    __u_argv[0] = "sh"; __u_argv[1] = "-c"; __u_argv[2] = (char *)__u_cmd; __u_argv[3] = 0;
    return cx_runv_timeout(__u_argv, __u_secs, __u_out, __u_len);
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

#ifndef UNISA_SHA256_H
#define UNISA_SHA256_H
/* sha256.h: FIPS 180-4 SHA-256, the one implementation for the product runtime (exec/c) and for include/cx.h.
   Allocation-free (cx.h is compiled by the product's trimmed libc): full blocks are read from the input, the padded
   tail is at most two blocks on the stack.  Byte output unisa_sha256, hex output unisa_sha256_hex.
   Checked against the standard vectors (empty, abc, million-a) in both the cx and product paths. */
#if !__UNISA_FTRIM_LIBC || __UN_unisa_rotr
static unsigned long unisa_rotr(unsigned long x, int k) { return ((x >> k) | (x << (32 - k))) & 0xffffffffUL; }
#endif

#if !__UNISA_FTRIM_LIBC || __UN_unisa_sha256_block
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
#endif
#if !__UNISA_FTRIM_LIBC || __UN_unisa_sha256
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
#endif
#if !__UNISA_FTRIM_LIBC || __UN_unisa_sha256_hex
static void unisa_sha256_hex(const void *in, size_t len, char hex[65]) {
    unsigned char md[32]; int i;
    unisa_sha256(in, len, md);
    for (i = 0; i < 32; i++) { hex[2*i] = "0123456789abcdef"[md[i] >> 4]; hex[2*i+1] = "0123456789abcdef"[md[i] & 15]; }
    hex[64] = 0;
}
#endif
#endif

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
    char __u_tmp[512], *__u_b; const char *__u_td; posix_spawn_file_actions_t __u_fa;
    pid_t __u_pid = 0, __u_w; int __u_fd, __u_st = 0, __u_rc, __u_spr, __u_k; long __u_ticks; FILE *__u_f;
    if (!__u_argv || !__u_argv[0]) return -1;
    __u_td = getenv("TMPDIR");
    if (!__u_td || !__u_td[0]) __u_td = getenv("TEMP");
    if (!__u_td || !__u_td[0]) __u_td = ".";
    __u_k = snprintf(__u_tmp, sizeof __u_tmp, "%s/cxrunXXXXXX", __u_td);
    if (__u_k < 0 || __u_k >= (int)sizeof __u_tmp) return -1;
    __u_fd = mkstemp(__u_tmp);
    if (__u_fd < 0) return -1;
    if (close(__u_fd) != 0) { unlink(__u_tmp); return -1; }
    if (posix_spawn_file_actions_init(&__u_fa) != 0) { unlink(__u_tmp); return -1; }
    if (posix_spawn_file_actions_addopen(&__u_fa, 1, __u_tmp, O_WRONLY | O_CREAT | O_TRUNC, 0600) != 0) {
        posix_spawn_file_actions_destroy(&__u_fa); unlink(__u_tmp); return -1;
    }
    __u_spr = posix_spawnp(&__u_pid, __u_argv[0], &__u_fa, 0, __u_argv, 0);
    posix_spawn_file_actions_destroy(&__u_fa);
    if (__u_spr != 0) { unlink(__u_tmp); return __u_spr == ENOENT ? 127 : -1; }
    __u_ticks = (long)__u_secs * 100; __u_rc = -1;
    for (;;) {
        __u_w = waitpid(__u_pid, &__u_st, WNOHANG);
        if (__u_w == __u_pid) { __u_rc = (__u_st & 0x7f) ? 128 + (__u_st & 0x7f) : (__u_st >> 8) & 0xff; break; }
        if (__u_w < 0) { __u_rc = -1; break; }
        if (__u_secs > 0 && __u_ticks-- <= 0) {
            kill(-__u_pid, SIGKILL); kill(__u_pid, SIGKILL); waitpid(__u_pid, &__u_st, 0); __u_rc = 124; break;
        }
        usleep(10000);
    }
    __u_f = fopen(__u_tmp, "rb");
    if (__u_f) {
        __u_b = _cx_slurp(__u_f, __u_len); fclose(__u_f);
        if (__u_out) *__u_out = __u_b; else free(__u_b);
    } else if (__u_out) *__u_out = 0;
    unlink(__u_tmp);
    return __u_rc;
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
}
#endif

/* Edition 1 headers, after the toolkit, where the .cx tools used to include them (so a tool that drops its own copy
   keeps its order of library bodies), and environ, which glibc declares only under _GNU_SOURCE.  Still explicit in
   a .cx, to revisit at edition 2: utime.h, sys/time.h, sys/ioctl.h (rare), math.h (libm semantics), and time.h,
   whose static objects (the day and month names, the zone buffer) sit outside any -ftrim-libc guard and would add
   143 bytes to every .cx program; guarding them would change the bytes of .c programs that include it. */

/* <stdint.h> for the unisa C subset.
 *
 * The exact-width names map straight onto the subset's i8/i16/i32/i64 --
 * and the UNSIGNED spellings really are unsigned.  They used to be aliases
 * for the signed ones, on the theory that the width was what mattered; it is
 * not.  `uint8_t b = 0xa2; printf("%.2x", b)` then printed
 * `ffffffffffffffa2`, because the load sign-extended.  tiny-AES-c is written
 * in uint8_t from end to end, and that one typedef was the whole difference
 * between a correct AES block and a screenful of f's. */
#ifndef _UNISA_STDINT_H
#define _UNISA_STDINT_H
typedef char           int8_t;
typedef unsigned char  uint8_t;
typedef short          int16_t;
typedef unsigned short uint16_t;
typedef int            int32_t;
typedef unsigned int   uint32_t;
/* The 64-bit and pointer-sized names are `long` on the LP64 targets and
   `long long` on LLP64 Windows (_WIN64), where `long` is 4 bytes. */
#if defined(_WIN64)
typedef long long          int64_t;
typedef unsigned long long uint64_t;
typedef long long          intptr_t;
typedef unsigned long long uintptr_t;
typedef long long          intmax_t;
typedef unsigned long long uintmax_t;
#else
typedef long           int64_t;
typedef unsigned long  uint64_t;
typedef long           intptr_t;
typedef unsigned long  uintptr_t;
typedef long           intmax_t;
typedef unsigned long  uintmax_t;
#endif

#define INT8_MIN    (-128)
#define INT8_MAX    127
#define UINT8_MAX   255
#define INT16_MIN   (-32768)
#define INT16_MAX   32767
#define UINT16_MAX  65535
#define INT32_MIN   (-2147483647 - 1)
#define INT32_MAX   2147483647
#define UINT32_MAX  4294967295
#define INT64_MIN   (-9223372036854775807 - 1)
#define INT64_MAX   9223372036854775807
/* The UL suffix is not decoration: without it this literal is too large for
   every signed type and the reference refuses to fold the expression at all
   (fb12-08).  gcc's own <stdint.h> carries it for the same reason. */
#if defined(_WIN64)
#define UINT64_MAX  18446744073709551615ULL
#else
#define UINT64_MAX  18446744073709551615UL
#endif

/* C99 7.18 lists more than the INTn/UINTn limits.  These are the rest, all
   written for a 64-bit target as the rest of the subset is.  Only names the
   reference can actually evaluate go in here: no function-like limits beyond
   INTn_C/UINTn_C, which fold to a plain cast of their argument. */
#if defined(_WIN64)
#define SIZE_MAX    18446744073709551615ULL
#define PTRDIFF_MIN (-9223372036854775807LL - 1)
#define PTRDIFF_MAX 9223372036854775807LL
#define INTPTR_MIN  (-9223372036854775807LL - 1)
#define INTPTR_MAX  9223372036854775807LL
#define UINTPTR_MAX 18446744073709551615ULL
#define INTMAX_MIN  (-9223372036854775807LL - 1)
#define INTMAX_MAX  9223372036854775807LL
#define UINTMAX_MAX 18446744073709551615ULL
#define INT64_C(v)  v##LL
#define UINT64_C(v) v##ULL
#else
#define SIZE_MAX    18446744073709551615UL
#define PTRDIFF_MIN (-9223372036854775807L - 1)
#define PTRDIFF_MAX 9223372036854775807L
#define INTPTR_MIN  (-9223372036854775807L - 1)
#define INTPTR_MAX  9223372036854775807L
#define UINTPTR_MAX 18446744073709551615UL
#define INTMAX_MIN  (-9223372036854775807L - 1)
#define INTMAX_MAX  9223372036854775807L
#define UINTMAX_MAX 18446744073709551615UL
#define INT64_C(v)  v##L
#define UINT64_C(v) v##UL
#endif
#define INT8_C(v)   v
#define INT16_C(v)  v
#define INT32_C(v)  v
#define UINT8_C(v)  v
#define UINT16_C(v) v
#define UINT32_C(v) v##U
#endif




/* <ctype.h> for the unisa C subset.  ASCII only: there is no locale, and
 * `char` is signed, so every function takes the value as an int the way C99
 * says and compares it directly. */
#ifndef _UNISA_CTYPE_H
#define _UNISA_CTYPE_H

#if !__UNISA_FTRIM_LIBC || __UN_isdigit
static int isdigit(int __u_c) { return __u_c >= '0' && __u_c <= '9'; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_isxdigit
static int isxdigit(int __u_c) {
    return (__u_c >= '0' && __u_c <= '9') || (__u_c >= 'a' && __u_c <= 'f')
        || (__u_c >= 'A' && __u_c <= 'F');
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_islower
static int islower(int __u_c) { return __u_c >= 'a' && __u_c <= 'z'; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_isupper
static int isupper(int __u_c) { return __u_c >= 'A' && __u_c <= 'Z'; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_isalpha
static int isalpha(int __u_c) { return islower(__u_c) || isupper(__u_c); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_isalnum
static int isalnum(int __u_c) { return isalpha(__u_c) || isdigit(__u_c); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_isspace
static int isspace(int __u_c) {
    return __u_c == ' ' || __u_c == '\t' || __u_c == '\n' || __u_c == '\v'
        || __u_c == '\f' || __u_c == '\r';
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_isblank
static int isblank(int __u_c) { return __u_c == ' ' || __u_c == '\t'; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_iscntrl
static int iscntrl(int __u_c) { return (__u_c >= 0 && __u_c < 32) || __u_c == 127; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_isprint
static int isprint(int __u_c) { return __u_c >= 32 && __u_c < 127; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_isgraph
static int isgraph(int __u_c) { return __u_c > 32 && __u_c < 127; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_ispunct
static int ispunct(int __u_c) { return isgraph(__u_c) && !isalnum(__u_c); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_toupper
static int toupper(int __u_c) { if (islower(__u_c)) return __u_c - 32; return __u_c; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_tolower
static int tolower(int __u_c) { if (isupper(__u_c)) return __u_c + 32; return __u_c; }
#endif

#endif

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


/* <fcntl.h> (0.0.18 R18-10): the O_* and F_* constants of each target's
 * kernel, open() and fcntl() over its own calls.  Windows (0.0.21 batch 1):
 * the mingw O_* values and open() over CreateFileA; the fd is the HANDLE. */
#ifndef _UNISA_FCNTL_H
#define _UNISA_FCNTL_H




#ifdef _WIN32
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR   2
#define O_ACCMODE 3
#define O_APPEND 0x8
#define O_CREAT  0x100
#define O_TRUNC  0x200
#define O_EXCL   0x400
#else
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR   2
#define O_ACCMODE 3
#ifdef __APPLE__
#define O_NONBLOCK  0x4
#define O_APPEND    0x8
#define O_CREAT     0x200
#define O_TRUNC     0x400
#define O_EXCL      0x800
#define O_NOFOLLOW  0x100
#define O_DIRECTORY 0x100000
#define O_CLOEXEC   0x1000000
#else
#define O_CREAT     0100
#define O_EXCL      0200
#define O_NOCTTY    0400
#define O_TRUNC     01000
#define O_APPEND    02000
#define O_NONBLOCK  04000
#define O_CLOEXEC   02000000
#ifdef __aarch64__
#define O_DIRECTORY 040000
#define O_NOFOLLOW  0100000
#else
#define O_DIRECTORY 0200000
#define O_NOFOLLOW  0400000
#endif
#endif
#define F_DUPFD 0
#define F_GETFD 1
#define F_SETFD 2
#define F_GETFL 3
#define F_SETFL 4
#define FD_CLOEXEC 1
#endif
#if !__UNISA_FTRIM_LIBC || __UN_open
static int open(const char *__u_p, int __u_flags, int __u_mode) {
    __u_word __u_r;
#ifdef _WIN32
    __u_word __u_acc; __u_word __u_disp;              /* the __open gate is CreateFileA(path, access, disposition) */
    __u_acc = 0x80000000L;                                        /* GENERIC_READ */
    if ((__u_flags & O_ACCMODE) == O_WRONLY) __u_acc = 0x40000000L;
    if ((__u_flags & O_ACCMODE) == O_RDWR) __u_acc = 0xC0000000L;
    __u_disp = 3;                                                 /* OPEN_EXISTING */
    if (__u_flags & O_CREAT) {
        if (__u_flags & O_EXCL) __u_disp = 1;                     /* CREATE_NEW */
        else if (__u_flags & O_TRUNC) __u_disp = 2;               /* CREATE_ALWAYS */
        else __u_disp = 4;                                        /* OPEN_ALWAYS */
    } else if (__u_flags & O_TRUNC) __u_disp = 5;                 /* TRUNCATE_EXISTING */
    __u_r = __open((char *)__u_p, __u_acc, __u_disp);
    if (__u_r < 0) return _ux_fail();
    if (__u_flags & O_APPEND) __lseek((int)__u_r, 0, 2);          /* SetFilePointer to the end */
    return (int)__u_r;
#else
    __u_r = __open((char *)__u_p, __u_flags, __u_mode);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; } return (int)__u_r;
#endif
}
#endif
#ifndef _WIN32
/* 0.0.28 H4 (SQLite's locks): struct flock and the record-lock commands, per OS */
#ifdef __APPLE__
struct flock { off_t l_start; off_t l_len; pid_t l_pid; short l_type; short l_whence; };
#define F_GETLK  7
#define F_SETLK  8
#define F_SETLKW 9
#define F_RDLCK  1
#define F_UNLCK  2
#define F_WRLCK  3
#else
struct flock { short l_type; short l_whence; off_t l_start; off_t l_len; pid_t l_pid; };
#define F_GETLK  5
#define F_SETLK  6
#define F_SETLKW 7
#define F_RDLCK  0
#define F_WRLCK  1
#define F_UNLCK  2
#endif
#endif
#ifndef _WIN32
#if !__UNISA_FTRIM_LIBC || __UN_fcntl
static int fcntl(int __u_fd, int __u_cmd, __u_word __u_arg) {
    __u_word __u_r; __u_r = __fcntl(__u_fd, __u_cmd, __u_arg);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return -1; } return (int)__u_r;
}
#endif
#endif
#endif

extern char **environ;
#endif
/* Deletion goes through the one guard, include/unisa_safedel.h (commissar ruling 2026-10-23): refused unless the
   target lies strictly under $TMPDIR (or /tmp) or a repository's out/ or .trash/, is no home, cwd, repository or
   ancestor of them, holds no .git and is within the entry ceiling.  0 on success (a missing path included), else the
   guard's code (3 refused, 1 failed) with one `cx_...: why` line on stderr.  cx_remove: a file, a link or an empty
   directory, for good; cx_rmtree: a tree, for good; cx_trash: a tree moved into the .trash of its root.  After the
   edition 1 headers, so a tool's library bodies keep their order. */

#ifndef UNISA_SAFEDEL_H
#define UNISA_SAFEDEL_H
/* unisa_safedel.h: the one deletion guard (commissar ruling 2026-10-23, after the 05:22 wipe of the checkout: an empty
   `--tools=` became Path('') == '.' and shutil.rmtree emptied the repository, .git included).  Every deletion in the
   repository goes through unisa_safedel(): in .cx through cx_remove/cx_rmtree/cx_trash (include/cx.h), from sh, make,
   CI and Python through the argv-only CLI tests/safedel.cx.  C99 over the carried libc; POSIX only (on Windows every
   call is refused: fail closed).  Every body sits alone in its own -ftrim-libc guard (tests/libneed.sh).

   int unisa_safedel(const char *path, unsigned flags, long max_files, char *why, int whylen)
     flags SAFEDEL_TRASH (0, the default) | SAFEDEL_PURGE | SAFEDEL_DRYRUN | SAFEDEL_RECURSIVE;
     SAFEDEL_OK 0, SAFEDEL_FAILED 1 (an I/O error after every check passed), SAFEDEL_REFUSED 3; WHY gets one line.
   The checks, in this order, on the resolved target (realpath; a symlink resolves as realpath(its parent)/its name
   and is removed as a link, never followed; a trailing slash on a symlink means its directory):
     1. refusal list: NULL, "", a leading "~", "/" (or only slashes), a last component "." or "..", $HOME or an
        ancestor of it, the cwd or an ancestor of it, the cwd's repository root or an ancestor of it, a path inside a
        .git directory, a repository root (a directory with a .git entry); a top-level symlink whose destination is
        refused by checks 1-2 (a link into a repository, to the cwd, ...);
     2. whitelist: inside a repository (nearest ancestor with a .git entry) the target must lie strictly under its
        out/ or .trash/; elsewhere strictly under a temp root: realpath($TMPDIR) and realpath(/tmp), both whitelisted
        (a relative TMPDIR, /, $HOME or an ancestor of $HOME is no temp root; with neither root everything outside
        repositories is refused); a temp root itself or an ancestor of one is refused;
     3. the walk (lstat only, links never followed) counts every entry, the target included, and refuses on an entry
        named .git, on a mount point, deeper than SAFEDEL_DEPTH; a non-empty directory needs SAFEDEL_RECURSIVE;
     4. more entries than max_files (<= 0: SAFEDEL_MAX_FILES) is refused.
   SAFEDEL_DRYRUN stops there.  Trash (the default): rename into HOME/.trash/<UTC stamp>-<pid>-<n>/<name>, HOME =
   the target's repository or its temp root (the deeper one when both hold it); .trash must be a real directory owned by the caller, closed to others.
   Purge: unlink and rmdir depth first, each directory re-checked (device and inode) when it is opened.  A missing
   target whose path passes the checks is success (rm -f).
   int unisa_safedel_purge_trash(const char *home, long hours, long max_files, unsigned flags, char *why, int whylen)
     purges the entries of HOME/.trash (HOME = a temp root or a repository root) older than HOURS, the ceiling over
     their summed entries; SAFEDEL_DRYRUN only counts.  The clock is the file system's (the mtime of a directory
     made for the purpose), so no <time.h> statics land in every .cx program. */
#if !defined(_WIN32) && !defined(_DEFAULT_SOURCE) && !defined(_POSIX_C_SOURCE) && !defined(_XOPEN_SOURCE) && !defined(_GNU_SOURCE)
#define _DEFAULT_SOURCE 1
#endif




#ifndef _WIN32



#endif
#ifdef __UNISA__

#elif !defined(__u_word) && defined(_WIN64)   /* a host compiler over its own libc (seed/compilerpack.c): no carried sys/_word.h */
#define __u_word long long
#define __u_uword unsigned long long
#elif !defined(__u_word)
#define __u_word long
#define __u_uword unsigned long
#endif

#define SAFEDEL_TRASH 0u
#define SAFEDEL_PURGE 1u
#define SAFEDEL_DRYRUN 2u
#define SAFEDEL_RECURSIVE 4u
#define SAFEDEL_OK 0
#define SAFEDEL_FAILED 1
#define SAFEDEL_REFUSED 3
#define SAFEDEL_MAX_FILES 1000L
#define SAFEDEL_DEPTH 128
#define SAFEDEL_PATHLEN 4096

struct unisa_safedel_env { char home[SAFEDEL_PATHLEN], cwd[SAFEDEL_PATHLEN], cwdrepo[SAFEDEL_PATHLEN], tmp[2][SAFEDEL_PATHLEN]; const char *tmpwhy[2]; };

static int unisa_safedel(const char *path, unsigned flags, __u_word max_files, char *why, int whylen);
static int unisa_safedel_purge_trash(const char *home, __u_word hours, __u_word max_files, unsigned flags, char *why, int whylen);

/* RC, with "PATH: A B" in WHY */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sd_say
static int _unisa_sd_say(char *why, int whylen, int rc, const char *path, const char *a, const char *b) {
    if (why && whylen > 0) snprintf(why, (size_t)whylen, "%s: %s%s", path ? path : "(null)", a ? a : "", b ? b : "");
    return rc;
}
#endif
/* 1 when P lies strictly below ROOT (both absolute and resolved) */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sd_below
static int _unisa_sd_below(const char *p, const char *root) {
    size_t n = strlen(root);
    if (!n || p[0] != '/') return 0;
    if (n == 1) return root[0] == '/' && p[1] != 0;
    return strncmp(p, root, n) == 0 && p[n] == '/' && p[n + 1] != 0;
}
#endif
/* 1 when P is ROOT or lies below it */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sd_within
static int _unisa_sd_within(const char *p, const char *root) { return root[0] && (strcmp(p, root) == 0 || _unisa_sd_below(p, root)); }
#endif
/* OUT = A/B (SAFEDEL_PATHLEN bytes); -1 when too long */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sd_join
static int _unisa_sd_join(char *out, const char *a, const char *b) {
    size_t n = strlen(a);
    int k = snprintf(out, SAFEDEL_PATHLEN, "%s%s%s", a, (n && a[n - 1] == '/') ? "" : "/", b);
    return k > 0 && k < SAFEDEL_PATHLEN ? 0 : -1;
}
#endif
/* realpath of P into OUT (SAFEDEL_PATHLEN bytes); 0, or -1 with errno */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sd_real
static int _unisa_sd_real(const char *p, char *out) {
    char *r = realpath(p, 0); size_t n;
    if (!r) return -1;
    n = strlen(r);
    if (n >= SAFEDEL_PATHLEN || r[0] != '/') { free(r); errno = ENAMETOOLONG; return -1; }
    memcpy(out, r, n + 1); free(r); return 0;
}
#endif
/* the nearest of P (absolute) and its ancestors with a .git entry, into OUT: 1, or 0 when there is none */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sd_repo
static int _unisa_sd_repo(const char *p, char *out) {
#ifdef _WIN32
    (void)p; (void)out; return 0;
#else
    char d[SAFEDEL_PATHLEN], g[SAFEDEL_PATHLEN]; struct stat st; size_t n = strlen(p); char *s;
    if (n >= SAFEDEL_PATHLEN || p[0] != '/') return 0;
    memcpy(d, p, n + 1);
    for (;;) {
        if (_unisa_sd_join(g, d, ".git") == 0 && lstat(g, &st) == 0) { memcpy(out, d, strlen(d) + 1); return 1; }
        if (d[1] == 0) return 0;
        s = strrchr(d, '/');
        if (s == d) d[1] = 0; else *s = 0;
    }
#endif
}
#endif
/* 1 when a component of P is .git */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sd_ingit
static int _unisa_sd_ingit(const char *p) {
    const char *s = p;
    if (!strcmp(p, ".git") || !strncmp(p, ".git/", 5)) return 1;
    while ((s = strstr(s, "/.git")) != 0) { if (s[5] == 0 || s[5] == '/') return 1; s += 5; }
    return 0;
}
#endif
/* a fresh number per call in this process (the trash and clock directory names) */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sd_seq
static __u_uword _unisa_sd_seq(void) { static __u_uword n; return n++; }
#endif
/* $HOME, the cwd, the cwd's repository and the temp root, resolved */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sd_env
static void _unisa_sd_env(struct unisa_safedel_env *e) {
#ifdef _WIN32
    e->home[0] = e->cwd[0] = e->cwdrepo[0] = e->tmp[0][0] = e->tmp[1][0] = 0; e->tmpwhy[0] = e->tmpwhy[1] = "Windows";
#else
    const char *h = getenv("HOME"), *t = getenv("TMPDIR"); char c[SAFEDEL_PATHLEN]; int i;
    e->home[0] = e->cwd[0] = e->cwdrepo[0] = e->tmp[0][0] = e->tmp[1][0] = 0; e->tmpwhy[0] = e->tmpwhy[1] = 0;
    if (h && h[0] && _unisa_sd_real(h, e->home) != 0) e->home[0] = 0;
    if (!getcwd(c, sizeof c) || _unisa_sd_real(c, e->cwd) != 0) e->cwd[0] = 0;
    if (e->cwd[0] && !_unisa_sd_repo(e->cwd, e->cwdrepo)) e->cwdrepo[0] = 0;
    if (!t || !t[0]) t = "/tmp";
    for (i = 0; i < 2; i++) {   /* root 0: $TMPDIR (/tmp when unset or empty); root 1: /tmp */
        const char *r = i ? "/tmp" : t;
        if (r[0] != '/') e->tmpwhy[i] = "TMPDIR is not absolute";
        else if (_unisa_sd_real(r, e->tmp[i]) != 0) e->tmpwhy[i] = "the temp root does not resolve";
        else if (!strcmp(e->tmp[i], "/")) e->tmpwhy[i] = "the temp root is /";
        else if (e->home[0] && _unisa_sd_within(e->home, e->tmp[i])) e->tmpwhy[i] = "the temp root is $HOME or an ancestor of it";
        if (e->tmpwhy[i]) e->tmp[i][0] = 0;
    }
    if (e->tmp[1][0] && !strcmp(e->tmp[0], e->tmp[1])) e->tmp[1][0] = 0;   /* one root, once */
#endif
}
#endif
/* checks 1-2 on the resolved T; HOME gets the trash home (the repository or the temp root).  DEST: T is where a
   top-level symlink points, and only checks 1 and the repository rule apply (removing a link to /usr/bin/cc is fine) */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sd_policy
static int _unisa_sd_policy(const struct unisa_safedel_env *e, const char *path, const char *t, int dest, char *home, char *why, int whylen) {
    char r[SAFEDEL_PATHLEN], o[SAFEDEL_PATHLEN], x[SAFEDEL_PATHLEN];
    if (t[0] != '/' || t[1] == 0) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "resolves to /", "");
    if (e->home[0] && _unisa_sd_within(e->home, t)) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "is $HOME or an ancestor of it: ", t);
    if (_unisa_sd_within(e->cwd, t)) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "is the cwd or an ancestor of it: ", t);
    if (e->cwdrepo[0] && _unisa_sd_within(e->cwdrepo, t)) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "is the repository root or an ancestor of it: ", t);
    if (_unisa_sd_ingit(t)) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "lies inside a .git directory: ", t);
    if (_unisa_sd_repo(t, r)) {
        if (!strcmp(r, t)) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "is a repository root (it has a .git entry): ", t);
        if (_unisa_sd_join(o, r, "out") || _unisa_sd_join(x, r, ".trash") || (!_unisa_sd_below(t, o) && !_unisa_sd_below(t, x)))
            return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "lies in a repository outside its out/ and .trash/: ", t);
        memcpy(home, r, strlen(r) + 1);
        return SAFEDEL_OK;
    }
    if (dest) return SAFEDEL_OK;
    if (!e->tmp[0][0] && !e->tmp[1][0]) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "is outside every repository and there is no temp root: ", e->tmpwhy[0]);
    {   /* the deeper of the temp roots that holds T (TMPDIR=/tmp/x: /tmp/x/y trashes into /tmp/x/.trash); a temp root
           itself, or an ancestor of one ($TMPDIR=/tmp/x from /tmp's view), is never a target */
        int i, k = -1;
        for (i = 0; i < 2; i++) if (e->tmp[i][0] && _unisa_sd_within(e->tmp[i], t)) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "is a temp root or an ancestor of one: ", t);
        for (i = 0; i < 2; i++) if (e->tmp[i][0] && _unisa_sd_below(t, e->tmp[i]) && (k < 0 || strlen(e->tmp[i]) > strlen(e->tmp[k]))) k = i;
        if (k < 0) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "is not strictly under a whitelisted root ($TMPDIR, /tmp, <repo>/out, <repo>/.trash): ", t);
        memcpy(home, e->tmp[k], strlen(e->tmp[k]) + 1);
    }
    return SAFEDEL_OK;
}
#endif
/* the lexical refusals and the resolution of PATH into T; then the policy.  *ABSENT: nothing is there */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sd_check
static int _unisa_sd_check(const struct unisa_safedel_env *e, const char *path, char *t, char *home, int *islink, int *absent, char *why, int whylen) {
#ifdef _WIN32
    (void)e; (void)t; (void)home; (void)islink; (void)absent;
    return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "deletion is not supported on Windows", "");
#else
    char p[SAFEDEL_PATHLEN], dir[SAFEDEL_PATHLEN], rd[SAFEDEL_PATHLEN], dest[SAFEDEL_PATHLEN], dh[SAFEDEL_PATHLEN], dw[512];
    const char *base; char *s; size_t n; int slash = 0, rc; struct stat st;
    *islink = 0; *absent = 0; home[0] = 0;
    if (!path || !path[0]) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, "\"\"", "empty path", "");
    if (path[0] == '~') return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "starts with ~ (an unexpanded home)", "");
    n = strlen(path);
    if (n >= SAFEDEL_PATHLEN) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, "(long path)", "path too long", "");
    memcpy(p, path, n + 1);
    while (n > 1 && p[n - 1] == '/') { p[--n] = 0; slash = 1; }
    if (!strcmp(p, "/")) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "is /", "");
    s = strrchr(p, '/'); base = s ? s + 1 : p;
    if (!strcmp(base, ".") || !strcmp(base, "..")) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "ends in . or ..", "");
    if (!e->cwd[0]) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "the cwd cannot be determined", "");
    if (!s) { dir[0] = '.'; dir[1] = 0; }
    else if (s == p) { dir[0] = '/'; dir[1] = 0; }
    else { memcpy(dir, p, (size_t)(s - p)); dir[s - p] = 0; }
    if (lstat(p, &st) != 0) {
        if (errno != ENOENT && errno != ENOTDIR) return _unisa_sd_say(why, whylen, SAFEDEL_FAILED, path, "cannot lstat: ", strerror(errno));
        *absent = 1;
        if (_unisa_sd_real(dir, rd) != 0) { memcpy(t, p, n + 1); return SAFEDEL_OK; }   /* not even the parent exists */
        if (_unisa_sd_join(t, rd, base)) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "path too long", "");
    } else if (S_ISLNK(st.st_mode) && !slash) {
        *islink = 1;
        if (_unisa_sd_real(dir, rd) != 0) return _unisa_sd_say(why, whylen, SAFEDEL_FAILED, path, "cannot resolve the parent: ", strerror(errno));
        if (_unisa_sd_join(t, rd, base)) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "path too long", "");
        if (_unisa_sd_real(p, dest) == 0 && (rc = _unisa_sd_policy(e, path, dest, 1, dh, dw, (int)sizeof dw)) != 0)
            return _unisa_sd_say(why, whylen, rc, path, "is a symlink to a protected path; ", dw);
    } else if (_unisa_sd_real(p, t) != 0) return _unisa_sd_say(why, whylen, SAFEDEL_FAILED, path, "cannot resolve: ", strerror(errno));
    return _unisa_sd_policy(e, path, t, 0, home, why, whylen);
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sd_free
static void _unisa_sd_free(char **v, size_t n) { size_t i; for (i = 0; i < n; i++) free(v[i]); free(v); }
#endif
/* the names in directory P, which must still be the directory ST describes (device and inode); 0, or -1 */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sd_list
static int _unisa_sd_list(const char *p, const struct stat *st, char ***out, size_t *outn) {
#ifdef _WIN32
    (void)p; (void)st; *out = 0; *outn = 0; return -1;
#else
    DIR *d = opendir(p); struct dirent *x; struct stat ds; char **v = 0, **g; size_t n = 0, cap = 0, l; int bad = 0;
    *out = 0; *outn = 0;
    if (!d) return -1;
    if (fstat(dirfd(d), &ds) != 0 || ds.st_dev != st->st_dev || ds.st_ino != st->st_ino) { closedir(d); return -1; }
    while (!bad && (x = readdir(d)) != 0) {
        if (!strcmp(x->d_name, ".") || !strcmp(x->d_name, "..")) continue;
        if (n == cap) {
            cap = cap ? 2 * cap : 16;
            g = (char **)realloc(v, cap * sizeof *v);
            if (!g) { bad = 1; break; }
            v = g;
        }
        l = strlen(x->d_name) + 1;
        if (!(v[n] = (char *)malloc(l))) { bad = 1; break; }
        memcpy(v[n++], x->d_name, l);
    }
    closedir(d);
    if (bad) { _unisa_sd_free(v, n); return -1; }
    *out = v; *outn = n; return 0;
#endif
}
#endif
/* check 3: count P (described by ST) and what lies below it into *CNT, stopping once it passes CAP; refuse a .git
   entry, another device than ROOT's, a depth past SAFEDEL_DEPTH */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sd_walk
static int _unisa_sd_walk(const char *p, const struct stat *st, const struct stat *root, int depth, __u_word *cnt, __u_word cap, char *why, int whylen) {
#ifdef _WIN32
    (void)p; (void)st; (void)root; (void)depth; (void)cnt; (void)cap;
    return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, "", "Windows", "");
#else
    char **v; size_t n, i; char c[SAFEDEL_PATHLEN]; struct stat cs; int rc = 0;
    *cnt += 1;
    if (*cnt > cap || !S_ISDIR(st->st_mode)) return SAFEDEL_OK;
    if (depth >= SAFEDEL_DEPTH) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, p, "is nested deeper than SAFEDEL_DEPTH", "");
    if (_unisa_sd_list(p, st, &v, &n) != 0) return _unisa_sd_say(why, whylen, SAFEDEL_FAILED, p, "cannot read the directory (or it changed)", "");
    for (i = 0; i < n && !rc && *cnt <= cap; i++) {
        if (!strcmp(v[i], ".git")) rc = _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, p, "contains a .git entry", "");
        else if (_unisa_sd_join(c, p, v[i])) rc = _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, p, "an entry's path is too long", "");
        else if (lstat(c, &cs) != 0) rc = _unisa_sd_say(why, whylen, SAFEDEL_FAILED, c, "cannot lstat: ", strerror(errno));
        else if (cs.st_dev != root->st_dev) rc = _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, c, "is another file system (a mount point)", "");
        else rc = _unisa_sd_walk(c, &cs, root, depth + 1, cnt, cap, why, whylen);
    }
    _unisa_sd_free(v, n);
    return rc;
#endif
}
#endif
/* permanent removal of P, depth first; each directory is re-checked when it is opened */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sd_purge
static int _unisa_sd_purge(const char *p, const struct stat *root, int depth, char *why, int whylen) {
#ifdef _WIN32
    (void)p; (void)root; (void)depth;
    return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, "", "Windows", "");
#else
    char **v; size_t n, i; char c[SAFEDEL_PATHLEN]; struct stat st; int rc = 0;
    if (lstat(p, &st) != 0) return errno == ENOENT ? SAFEDEL_OK : _unisa_sd_say(why, whylen, SAFEDEL_FAILED, p, "cannot lstat: ", strerror(errno));
    if (st.st_dev != root->st_dev) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, p, "is another file system (a mount point)", "");
    if (!S_ISDIR(st.st_mode)) {
        if (unlink(p) != 0 && errno != ENOENT) return _unisa_sd_say(why, whylen, SAFEDEL_FAILED, p, "cannot unlink: ", strerror(errno));
        return SAFEDEL_OK;
    }
    if (depth >= SAFEDEL_DEPTH) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, p, "is nested deeper than SAFEDEL_DEPTH", "");
    if (_unisa_sd_list(p, &st, &v, &n) != 0) return _unisa_sd_say(why, whylen, SAFEDEL_FAILED, p, "cannot read the directory (or it changed)", "");
    for (i = 0; i < n && !rc; i++) {
        if (!strcmp(v[i], ".git")) rc = _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, p, "contains a .git entry", "");
        else if (_unisa_sd_join(c, p, v[i])) rc = _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, p, "an entry's path is too long", "");
        else rc = _unisa_sd_purge(c, root, depth + 1, why, whylen);
    }
    _unisa_sd_free(v, n);
    if (!rc && rmdir(p) != 0) rc = _unisa_sd_say(why, whylen, SAFEDEL_FAILED, p, "cannot rmdir: ", strerror(errno));
    return rc;
#endif
}
#endif
/* the file system's clock: the mtime of a directory made (and removed) in DIR; -1 on error */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sd_now
static __u_word _unisa_sd_now(const char *dir) {
#ifdef _WIN32
    (void)dir; return -1;
#else
    char p[SAFEDEL_PATHLEN]; struct stat st; long t = -1; int k;
    k = snprintf(p, sizeof p, "%s/.now-%ld-%lu", dir, (long)getpid(), _unisa_sd_seq());
    if (k <= 0 || k >= (int)sizeof p || mkdir(p, 0700) != 0) return -1;
    if (lstat(p, &st) == 0) t = (long)st.st_mtime;
    rmdir(p);
    return t;
#endif
}
#endif
/* T (seconds since 1970, UTC) as YYYYMMDDTHHMMSSZ */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sd_stamp
static void _unisa_sd_stamp(__u_word t, char out[20]) {
    __u_word z = t / 86400, s = t % 86400, era, doe, yoe, y, doy, mp, d, m;
    if (s < 0) { s += 86400; z -= 1; }
    z += 719468; era = (z >= 0 ? z : z - 146096) / 146097; doe = z - era * 146097;
    yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; y = yoe + era * 400;
    doy = doe - (365 * yoe + yoe / 4 - yoe / 100); mp = (5 * doy + 2) / 153;
    d = doy - (153 * mp + 2) / 5 + 1; m = mp < 10 ? mp + 3 : mp - 9; if (m <= 2) y += 1;
#if defined(_WIN64)
    snprintf(out, 20, "%04lld%02lld%02lldT%02lld%02lld%02lldZ", y, m, d, s / 3600, s / 60 % 60, s % 60);
#else
    snprintf(out, 20, "%04ld%02ld%02ldT%02ld%02ld%02ldZ", y, m, d, s / 3600, s / 60 % 60, s % 60);
#endif
}
#endif
/* the time a trash entry name starts with (YYYYMMDDTHHMMSSZ), or -1 */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sd_unstamp
static __u_word _unisa_sd_unstamp(const char *nm) {
    __u_word v[6], y, m, era, yoe, doy, doe; int i, j, k = 0, w;
    for (i = 0; i < 6; i++) {
        if (i == 3 && nm[k++] != 'T') return -1;
        w = i == 0 ? 4 : 2; v[i] = 0;
        for (j = 0; j < w; j++, k++) { if (nm[k] < '0' || nm[k] > '9') return -1; v[i] = v[i] * 10 + (nm[k] - '0'); }
    }
    if (nm[k] != 'Z' || v[1] < 1 || v[1] > 12 || v[2] < 1 || v[2] > 31 || v[3] > 23 || v[4] > 59 || v[5] > 60) return -1;
    y = v[0]; m = v[1]; if (m <= 2) y -= 1;
    era = (y >= 0 ? y : y - 399) / 400; yoe = y - era * 400;
    doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + v[2] - 1; doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return (era * 146097 + doe - 719468) * 86400 + v[3] * 3600 + v[4] * 60 + v[5];
}
#endif
/* HOME/.trash: made 0700 when missing; it must be a real directory of this user, closed to others.  0, or a refusal */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sd_trashdir
static int _unisa_sd_trashdir(const char *home, char *tr, int make, char *why, int whylen) {
#ifdef _WIN32
    (void)home; (void)tr; (void)make;
    return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, "", "Windows", "");
#else
    struct stat st;
    if (_unisa_sd_join(tr, home, ".trash")) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, home, "path too long", "");
    if (make && mkdir(tr, 0700) != 0 && errno != EEXIST) return _unisa_sd_say(why, whylen, SAFEDEL_FAILED, tr, "cannot make: ", strerror(errno));
    if (lstat(tr, &st) != 0) return _unisa_sd_say(why, whylen, SAFEDEL_FAILED, tr, "cannot lstat: ", strerror(errno));
    if (!S_ISDIR(st.st_mode)) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, tr, "is not a directory (a symlink?)", "");
    if (st.st_uid != geteuid() || (st.st_mode & 022)) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, tr, "is not this user's or is writable by others", "");
    return SAFEDEL_OK;
#endif
}
#endif
/* unisa_safedel, with the counted entries in *COUNT */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sd_run
static int _unisa_sd_run(const char *path, unsigned flags, __u_word max_files, __u_word *count, char *why, int whylen) {
#ifdef _WIN32
    (void)flags; (void)max_files; *count = 0;
    return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "deletion is not supported on Windows", "");
#else
    struct unisa_safedel_env e; char t[SAFEDEL_PATHLEN], home[SAFEDEL_PATHLEN], tr[SAFEDEL_PATHLEN], sd[SAFEDEL_PATHLEN], dest[SAFEDEL_PATHLEN], stamp[20];
    int islink, absent, rc, k, er; struct stat st; long cnt = 0, now; unsigned long i;
    *count = 0;
    if (max_files <= 0) max_files = SAFEDEL_MAX_FILES;
    _unisa_sd_env(&e);
    rc = _unisa_sd_check(&e, path, t, home, &islink, &absent, why, whylen);
    if (rc) return rc;
    if (absent) return _unisa_sd_say(why, whylen, SAFEDEL_OK, path, "absent (nothing to delete)", "");
    if (lstat(t, &st) != 0) return _unisa_sd_say(why, whylen, SAFEDEL_FAILED, path, "cannot lstat: ", strerror(errno));
    {   /* a mount point (a bind mount of $HOME under /tmp, say) is another file system than its parent: refused */
        struct stat ps; size_t l = (size_t)(strrchr(t, '/') - t);
        memcpy(sd, t, l ? l : 1); sd[l ? l : 1] = 0;
        if (lstat(sd, &ps) != 0) return _unisa_sd_say(why, whylen, SAFEDEL_FAILED, path, "cannot lstat the parent: ", strerror(errno));
        if (ps.st_dev != st.st_dev) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "is a mount point", "");
    }
    if (islink || !S_ISDIR(st.st_mode)) cnt = 1;
    else {
        rc = _unisa_sd_walk(t, &st, &st, 0, &cnt, max_files, why, whylen);
        if (rc) return rc;
        if (cnt > 1 && !(flags & SAFEDEL_RECURSIVE)) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "is a non-empty directory (not recursive)", "");
    }
    if (cnt > max_files) {
        char m[96]; snprintf(m, sizeof m, "more than %ld entries, the ceiling (pass an explicit force count)", max_files);
        return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, m, "");
    }
    *count = cnt;
    if (flags & SAFEDEL_DRYRUN) {
        char m[96]; snprintf(m, sizeof m, "would %s %ld entries: ", (flags & SAFEDEL_PURGE) ? "purge" : "trash", cnt);
        return _unisa_sd_say(why, whylen, SAFEDEL_OK, path, m, t);
    }
    if (flags & SAFEDEL_PURGE) {
        rc = _unisa_sd_purge(t, &st, 0, why, whylen);
        if (!rc) { char m[64]; snprintf(m, sizeof m, "purged %ld entries: ", cnt); _unisa_sd_say(why, whylen, SAFEDEL_OK, path, m, t); }
        return rc;
    }
    rc = _unisa_sd_trashdir(home, tr, 1, why, whylen);
    if (rc) return rc;
    if (_unisa_sd_within(tr, t)) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "is the trash or holds it (purge it instead)", "");
    now = _unisa_sd_now(tr);
    if (now < 0) return _unisa_sd_say(why, whylen, SAFEDEL_FAILED, tr, "cannot read the clock: ", strerror(errno));
    _unisa_sd_stamp(now, stamp);
    for (i = 0;; i++) {
        k = snprintf(sd, sizeof sd, "%s/%s-%ld-%lu", tr, stamp, (long)getpid(), _unisa_sd_seq());
        if (k <= 0 || k >= (int)sizeof sd) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, tr, "path too long", "");
        if (mkdir(sd, 0700) == 0) break;
        if (errno != EEXIST || i >= 1000) return _unisa_sd_say(why, whylen, SAFEDEL_FAILED, sd, "cannot make: ", strerror(errno));
    }
    if (_unisa_sd_join(dest, sd, strrchr(t, '/') + 1)) { rmdir(sd); return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "path too long", ""); }
    if (rename(t, dest) != 0) {
        er = errno; rmdir(sd);
        if (er == EXDEV) return _unisa_sd_say(why, whylen, SAFEDEL_FAILED, path, "lies on another file system than its trash; not moved (purge it instead)", "");
        return _unisa_sd_say(why, whylen, SAFEDEL_FAILED, path, "cannot move into the trash: ", strerror(er));
    }
    { char m[64]; snprintf(m, sizeof m, "trashed %ld entries to ", cnt); _unisa_sd_say(why, whylen, SAFEDEL_OK, path, m, dest); }
    return SAFEDEL_OK;
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_unisa_safedel
static int unisa_safedel(const char *path, unsigned flags, __u_word max_files, char *why, int whylen) {
    __u_word n; return _unisa_sd_run(path, flags, max_files, &n, why, whylen);
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_unisa_safedel_purge_trash
static int unisa_safedel_purge_trash(const char *home, __u_word hours, __u_word max_files, unsigned flags, char *why, int whylen) {
#ifdef _WIN32
    (void)hours; (void)max_files; (void)flags;
    return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, home, "deletion is not supported on Windows", "");
#else
    struct unisa_safedel_env e; char r[SAFEDEL_PATHLEN], tr[SAFEDEL_PATHLEN], c[SAFEDEL_PATHLEN], m[128];
    struct stat st; char **v; char *pick; size_t n, i; long now, t0, total = 0, k, entries = 0; int rc = 0;
    if (!home || !home[0]) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, "\"\"", "empty trash home", "");
    if (hours < 0) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, home, "a negative age", "");
    if (max_files <= 0) max_files = SAFEDEL_MAX_FILES;
    _unisa_sd_env(&e);
    if (_unisa_sd_real(home, r) != 0) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, home, "does not resolve", "");
    if (!(e.tmp[0][0] && !strcmp(r, e.tmp[0])) && !(e.tmp[1][0] && !strcmp(r, e.tmp[1]))) {
        if (_unisa_sd_join(c, r, ".git") || lstat(c, &st) != 0)
            return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, home, "is neither a temp root nor a repository root: ", r);
    }
    if (_unisa_sd_join(tr, r, ".trash")) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, home, "path too long", "");
    if (lstat(tr, &st) != 0) {
        if (errno == ENOENT) return _unisa_sd_say(why, whylen, SAFEDEL_OK, tr, "no trash", "");
        return _unisa_sd_say(why, whylen, SAFEDEL_FAILED, tr, "cannot lstat: ", strerror(errno));
    }
    rc = _unisa_sd_trashdir(r, tr, 0, why, whylen);
    if (rc) return rc;
    if (lstat(tr, &st) != 0 || (now = _unisa_sd_now(tr)) < 0) return _unisa_sd_say(why, whylen, SAFEDEL_FAILED, tr, "cannot read the clock", "");
    if (_unisa_sd_list(tr, &st, &v, &n) != 0) return _unisa_sd_say(why, whylen, SAFEDEL_FAILED, tr, "cannot read the directory", "");
    pick = (char *)calloc(n ? n : 1, 1);
    if (!pick) { _unisa_sd_free(v, n); return _unisa_sd_say(why, whylen, SAFEDEL_FAILED, tr, "out of memory", ""); }
    for (i = 0; i < n && !rc; i++) {
        struct stat es;
        if (!strncmp(v[i], ".now-", 5) || _unisa_sd_join(c, tr, v[i]) || lstat(c, &es) != 0) continue;
        t0 = _unisa_sd_unstamp(v[i]);
        if (t0 < 0) t0 = (long)es.st_mtime;
        if (now - t0 < hours * 3600) continue;
        rc = _unisa_sd_run(c, SAFEDEL_PURGE | SAFEDEL_RECURSIVE | SAFEDEL_DRYRUN, max_files, &k, why, whylen);
        if (!rc) { pick[i] = 1; total += k; entries++; }
    }
    if (!rc && total > max_files) {
        snprintf(m, sizeof m, "%ld expired entries hold more than %ld, the ceiling (pass an explicit force count)", entries, max_files);
        rc = _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, tr, m, "");
    }
    if (!rc && (flags & SAFEDEL_DRYRUN)) {
        snprintf(m, sizeof m, "would purge %ld expired entries (%ld files)", entries, total);
        _unisa_sd_say(why, whylen, SAFEDEL_OK, tr, m, "");
    } else if (!rc) {
        for (i = 0; i < n && !rc; i++)
            if (pick[i] && !_unisa_sd_join(c, tr, v[i])) rc = _unisa_sd_run(c, SAFEDEL_PURGE | SAFEDEL_RECURSIVE, total, &k, why, whylen);
        if (!rc) { snprintf(m, sizeof m, "purged %ld expired entries (%ld files)", entries, total); _unisa_sd_say(why, whylen, SAFEDEL_OK, tr, m, ""); }
    }
    free(pick); _unisa_sd_free(v, n);
    return rc;
#endif
}
#endif
#endif

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
