/* <stdlib.h> for the unisa C subset.  The allocator is a bump allocator over
 * a static arena: `free` is a no-op and the arena is part of the image, which
 * is honest about what this compiler is for.  Nothing here needs a linker. */
#ifndef _UNISA_STDLIB_H
#define _UNISA_STDLIB_H
#include <stddef.h>
#define NULL 0
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define RAND_MAX 32767

int exit();

#define _UNISA_ARENA 65536
static char _unisa_heap[_UNISA_ARENA];
static long _unisa_brk = 0;

static void *malloc(long n) {
    char *p;
    n = (n + 15) & ~15;
    if (_unisa_brk + n > _UNISA_ARENA) return NULL;
    p = _unisa_heap + _unisa_brk;
    _unisa_brk = _unisa_brk + n;
    return p;
}

static void free(void *p) { }

/* `exit` is the one libc function that cannot be written in C: it must not
 * return.  `__exit` is the tape's gate to the OS, so this is a one-line
 * wrapper around it -- and `abort` is the same call with the status a shell
 * reports for SIGABRT. */
static void exit(int code) { __exit(code); }
static void abort(void) { __exit(134); }

static void *calloc(long n, long sz) {
    char *p; long total; long i;
    total = n * sz;
    p = (char *)malloc(total);
    if (p == NULL) return NULL;
    i = 0;
    while (i < total) { p[i] = 0; i = i + 1; }
    return p;
}

static void *realloc(void *old, long n) {
    char *p; char *o; long i;
    p = (char *)malloc(n);
    if (p == NULL) return NULL;
    if (old != NULL) {
        o = (char *)old;
        i = 0;
        while (i < n) { p[i] = o[i]; i = i + 1; }
    }
    return p;
}

static int abs(int v) { if (v < 0) return 0 - v; return v; }

static int atoi(const char *s) {
    int v; int sign; long i;
    v = 0; sign = 1; i = 0;
    while (s[i] == 32) i = i + 1;
    if (s[i] == 45) { sign = 0 - 1; i = i + 1; }
    else { if (s[i] == 43) i = i + 1; }
    while (s[i] >= 48) { if (s[i] > 57) break; v = v * 10 + (s[i] - 48); i = i + 1; }
    return v * sign;
}

static long _unisa_seed = 1;
static int rand(void) {
    _unisa_seed = _unisa_seed * 1103515245 + 12345;
    return (int)((_unisa_seed >> 16) & 32767);
}
static void srand(int s) { _unisa_seed = s; }
#endif
