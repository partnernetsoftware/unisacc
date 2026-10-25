/* freshorderhook.h -- the fresh-order recorder for seed/gen.c, as UNISACC_FRESH_LOG is for exec/build/procs.py
   (0.0.24 T1d (c); 0.0.43: the fresh-order gate moved from gen.py to the C seed constructor, ~8 s for parse2 against
   40-76 s).  seed/gen.c is not edited: tests/freshordercheck.cx builds it as
       cc -std=c99 -O2 -w -Iseed -include tests/freshorderhook.h seed/gen.c
   so every snprintf in it goes through freshorder_snprintf, which formats exactly as snprintf does.  fresh_label() is
   the one snprintf with the format "%.*s.%s%lu" (prefix, owner, kind, n -> the label OWNER.KINDn); for that format,
   with UNISACC_FRESH_LOG=PATH set, one row "n<TAB>owner<TAB>kind" (owner cut at its first '.', as the label and
   procs.py cut it) is appended to PATH per allocation, in allocation order.  These are procs.py's first three
   columns; its fourth (the Python caller file:line) has no C counterpart.  Unset: no file, output unchanged. */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static FILE *freshorder_log;
static void freshorder_close(void) {
    if (freshorder_log && (ferror(freshorder_log) | fclose(freshorder_log))) { fputs("freshorderhook: log write failed\n", stderr); _Exit(3); }
}
static int freshorder_snprintf(char *buf, size_t size, const char *fmt, ...) {
    static int opened;
    va_list ap; int r, prefix; const char *owner, *kind, *path; unsigned long n;
    va_start(ap, fmt); r = vsnprintf(buf, size, fmt, ap); va_end(ap);
    if (strcmp(fmt, "%.*s.%s%lu")) return r;
    if (!opened) {
        opened = 1; path = getenv("UNISACC_FRESH_LOG");
        if (path && *path) {
            if (!(freshorder_log = fopen(path, "a"))) { fputs("freshorderhook: cannot open UNISACC_FRESH_LOG\n", stderr); _Exit(3); }
            atexit(freshorder_close);
        }
    }
    if (freshorder_log) {
        va_start(ap, fmt);
        prefix = va_arg(ap, int); owner = va_arg(ap, const char *); kind = va_arg(ap, const char *); n = va_arg(ap, unsigned long);
        va_end(ap);
        fprintf(freshorder_log, "%lu\t%.*s\t%s\n", n, prefix, owner, kind);
    }
    return r;
}
#define snprintf freshorder_snprintf
