/* sys/_exit.h -- 0.0.33 D3': exit, the atexit table and the stdio flush hook, shared by
 * <stdlib.h> and <stdio.h>.  <stdio.h> needs exit in the program so that returning from main
 * goes through it and flushes buffered streams, but not the rest of <stdlib.h> (its environ
 * constructor and every other name). */
#ifndef _UNISA_SYS_EXIT_H
#define _UNISA_SYS_EXIT_H
int exit();   /* before the definition, as <stdlib.h> always had it (0.0.33 D3': after it, the redeclaration typed exit int and E3 refused assert's (printf(...), exit(1))) */
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
static void _u_st_copy(char *__u_d, const char *__u_s, long __u_n) {
    long __u_j; __u_j = 0;
    while (__u_j < __u_n) { __u_d[__u_j] = __u_s[__u_j]; __u_j = __u_j + 1; }
}
#endif
static int _u_st_wlen[_U_NST];             /* D3': bytes waiting in the buffer to be written */
static int _u_st_n;


/* D3': the write side of the buffer.  A stream's buffer holds either unread
   bytes or unwritten ones: a write after a read gives the unread part back to
   the descriptor (lseek), and a read, seek, tell, fflush, fclose or exit
   writes the waiting bytes first.  The loop in _u_st_raw stops on what the
   __write gate calls an error: negative on POSIX and the VM, 0 on Windows
   (WriteFile's count); an answer larger than asked is not trusted. */
#if !__UNISA_FTRIM_LIBC || __UN__u_st_raw
static long _u_st_raw(int __u_fd, const char *__u_p, long __u_n) {
    long __u_done; long __u_r;
    __u_done = 0;
    while (__u_done < __u_n) {
        __u_r = __write(__u_fd, (char *)__u_p + __u_done, __u_n - __u_done);
        if (__u_r <= 0 || __u_r > __u_n - __u_done) return __u_done;
        __u_done = __u_done + __u_r;
    }
    return __u_n;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__u_st_wflush
static int _u_st_wflush(int __u_i) {
    long __u_k;
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
