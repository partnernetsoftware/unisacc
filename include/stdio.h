/* Minimal <stdio.h> for the unisa C subset.
 * printf is desugared by the walker against its static format string [W-9];
 * everything else here is ordinary C over the `.sys` gate, so it needs no
 * linker and no compiler support.  A FILE * is a file descriptor in a
 * pointer's clothing. */
#ifndef _UNISA_STDIO_H
#define _UNISA_STDIO_H
#include <stddef.h>
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
#endif
