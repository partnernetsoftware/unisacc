/* examples/cx-lab/cxlab.h -- lab-only reusable helpers, factored out of lex_table.cx and
 * time_run.cx once the same code showed up twice: a generic TSV row loader (comment lines and
 * the header row skipped, up to NCOLS tab-separated fields per row) and a timing/median pair
 * for "run a shell command a few times, keep the middle". Nothing here reads or writes outside
 * whatever path the caller passes in; it has no opinion about exec/, weights/, or kernel/.
 * Pure C99 over cx.h / libc -- no new syntax, just ordinary header reuse across cx scripts.
 */
#ifndef _CXLAB_H
#define _CXLAB_H
#include <cx.h>
#include <sys/time.h>

#define CXLAB_MAXCOL 6
#define CXLAB_COLW 32

typedef struct { char f[CXLAB_MAXCOL][CXLAB_COLW]; } cxlab_row_t;

/* splits one line on '\t' into row->f[0..ncols-1]; trailing/missing fields are "" */
static void cxlab_split_tsv(const char *line, cxlab_row_t *row, int ncols) {
    int col = 0, i; const char *p = line;
    for (col = 0; col < ncols; col++) row->f[col][0] = 0;
    col = 0;
    while (*p && col < ncols) {
        i = 0;
        while (*p && *p != '\t' && *p != '\n') { if (i < CXLAB_COLW - 1) row->f[col][i++] = *p; p++; }
        row->f[col][i] = 0;
        col++;
        if (*p == '\t') p++;
    }
}

/* loads `path`'s data rows (comment lines starting with '#' and the first real line, the
 * header, are both skipped) into `rows`, up to `maxrows`; returns the row count, or -1 if the
 * file could not be opened. */
static int cxlab_load_tsv(const char *path, cxlab_row_t *rows, int maxrows, int ncols) {
    size_t len; char *text = cx_read(path, &len), *line, *nl;
    int n = 0, header_skipped = 0;
    if (!text) return -1;
    line = text;
    while (line && *line && n < maxrows) {
        nl = strchr(line, '\n');
        if (nl) *nl = 0;
        if (line[0] != '#' && line[0] != 0) {
            if (!header_skipped) header_skipped = 1;
            else { cxlab_split_tsv(line, &rows[n], ncols); n++; }
        }
        line = nl ? nl + 1 : 0;
    }
    free(text);
    return n;
}

/* NOT safe to call right after cx_run/cx_run_timeout in this build: a forked-and-waited child
 * leaves this process's gettimeofday() reading ~2^32 microseconds (~4294.967296s) high on the
 * very next call (confirmed by probe: two back-to-back calls read ~0.000000s apart, but the
 * first call after one cx_run_timeout("true", ...) jumps by 4294.979211s). Real runtime bug in
 * this unisacc.com build's fork/gettimeofday interaction, not a lab bug; cxlab_timed_median
 * below works around it instead of calling this straddling a fork. Fine to use when nothing
 * between two calls has forked. */
static double cxlab_now(void) {
    struct timeval t; gettimeofday(&t, 0);
    return (double)t.tv_sec + (double)t.tv_usec / 1e6;
}

/* in-place sort then middle element; odd n is the usual case callers want */
static double cxlab_median(double *xs, int n) {
    int i, j; double tmp;
    for (i = 0; i < n - 1; i++)
        for (j = i + 1; j < n; j++)
            if (xs[j] < xs[i]) { tmp = xs[i]; xs[i] = xs[j]; xs[j] = tmp; }
    return xs[n / 2];
}

/* times `cmd` (a shell string) `repeats` times and returns the median elapsed seconds; *rc_out,
 * if given, gets the last run's real exit code. secs bounds each run.
 *
 * Does NOT wrap cmd with our own cxlab_now() before/after cx_run_timeout -- see the warning on
 * cxlab_now(): this process's clock is unreliable right after a fork+wait. Instead the elapsed
 * time is measured *inside* the forked child, by bash's own `time` builtin (TIMEFORMAT=%R),
 * which prints just the seconds to stdout; `time` does not change the timed command's exit
 * status, so the wrapper's reported rc is still cmd's real exit code, not bash's or time's. */
static double cxlab_timed_median(const char *cmd, int repeats, int secs, int *rc_out) {
    double xs[16]; int i, rc = 0; char *out; size_t n; char wrapped[1024];
    if (repeats > 16) repeats = 16;
    snprintf(wrapped, sizeof wrapped,
             "bash -c 'TIMEFORMAT=%%R; { time ( %s >/dev/null 2>/dev/null ) ; } 2>&1'", cmd);
    for (i = 0; i < repeats; i++) {
        rc = cx_run_timeout(wrapped, secs, &out, &n);
        xs[i] = out ? strtod(out, 0) : 0.0;
        free(out);
    }
    if (rc_out) *rc_out = rc;
    return cxlab_median(xs, repeats);
}

#endif
