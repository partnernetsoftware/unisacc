/* rowcov.c -- native row-coverage aggregator (unisacc.com -run tests/rowcov.c -- ...).
 *
 * The C half of the native rowcov (cdx 2026-10-09: runtime edge log UNISACC_EDGE_LOG, three slices
 * after L1b).  tests/rowcov.py simulates every probe in Python (26-40 s per job); with the executors
 * logging the edges they take, this program only merges.  Until the hooks land it is exercised by
 * --selftest and can be checked against rowcov.py's own output (--seen).  Interface, all TSV:
 *
 *   EDGE LOG   (from UNISACC_EDGE_LOG, one or more files)  model  state_id  key  probe
 *              state_id is the executor's number (>= 0, in STATES); key -1 is BOT and is read as the
 *              key name "BOT"; one line per distinct edge per execution
 *   STATES     (the same model's state dictionary)          state_id  state_name
 *   EDGES      (edges that count: not REJECT unreachable)   state_name  key
 *   ROWS       (build side table, UNISACC_ROW_LOG)         state_name  key  manifest_path  line
 *   --seen F   (alternative to logs: rowcov.py's taken set) state_name  key  [probe]
 *
 *   rowcov.c -- --model M --states S --edges E --rows R [--floor N] [--cov OUT] LOG...
 *   rowcov.c -- --states S --edges E --rows R --seen F ...
 *   rowcov.c -- --selftest
 * --taken OUT writes the taken edge set (state TAB key, unsorted) for a set comparison.
 * Prints the same two lines rowcov.py's merge prints (edges/taken, rows/covered) and, with --floor,
 * fails when covered rows fall below it.  A log line for another model, an unknown state id, or a
 * malformed line is an error, never skipped: a coverage number from a mixed log is not a number.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAXS 65536
#define MAXE 1048576

static char *sname[MAXS]; static int nsname;            /* state id (index) -> name; ids 0.. */
typedef struct { char *st; char *key; char *path; int line; int taken; char *wit; } Row;
static char **ekey; static int *etaken; static char **ewit; static int ne; static char *takenf;   /* counted edges "state\tkey" */
static Row *rows; static int nrows;

/* the whole field is a decimal integer (atoi reads "abc" as 0) */
static int number(char *s, int *out) {
    char *e; long v;
    if (!*s) return 0;
    v = strtol(s, &e, 10);
    if (*e || v < -2147483647L || v > 2147483647L) return 0;
    *out = (int)v; return 1;
}
static char *sdup(char *s) { char *d = malloc(strlen(s) + 1); strcpy(d, s); return d; }
static void die(char *what, char *where, int ln) { fprintf(stderr, "rowcov: %s (%s:%d)\n", what, where, ln); exit(1); }

/* split a tab line into at most n fields; returns the count */
static int fields(char *l, char **f, int n) {
    int k = 0; l[strcspn(l, "\r\n")] = 0;
    if (*l == 0 || *l == '#') return 0;
    f[k++] = l;
    while (*l && k < n) { if (*l == '\t') { *l = 0; f[k++] = l + 1; } l++; }
    return k;
}

/* open-addressed hash of "state\tkey" -> edge index */
#define HB 2097152
static int hidx[HB];
static unsigned hash(char *a, char *b) { unsigned h = 2166136261u; while (*a) h = (h ^ (unsigned char)*a++) * 16777619u; h = (h ^ 9) * 16777619u; while (*b) h = (h ^ (unsigned char)*b++) * 16777619u; return h; }
static int find(char *st, char *key) {
    unsigned h = hash(st, key) & (HB - 1);
    while (hidx[h]) {
        int i = hidx[h] - 1; char *t = strchr(ekey[i], '\t');
        if ((int)(t - ekey[i]) == (int)strlen(st) && !strncmp(ekey[i], st, t - ekey[i]) && !strcmp(t + 1, key)) return i;
        h = (h + 1) & (HB - 1);
    }
    return -1;
}
static void add_edge(char *st, char *key) {
    unsigned h; char *k;
    if (find(st, key) >= 0) return;
    if (ne >= MAXE) die("too many edges", "EDGES", ne);
    k = malloc(strlen(st) + strlen(key) + 2); sprintf(k, "%s\t%s", st, key);
    ekey[ne] = k; etaken[ne] = 0; ewit[ne] = 0;
    h = hash(st, key) & (HB - 1); while (hidx[h]) h = (h + 1) & (HB - 1);
    hidx[h] = ++ne;
}

static void take(char *st, char *key, char *probe) {
    int i = find(st, key);
    if (i < 0) return;                         /* a REJECT-unreachable edge or a rejection: not counted */
    if (!etaken[i]) { etaken[i] = 1; ewit[i] = probe ? sdup(probe) : 0; }
}

static void read_states(char *p) {
    FILE *f = fopen(p, "r"); char l[4096], *v[3]; int ln = 0, n, id;
    if (!f) die("cannot open states", p, 0);
    while (fgets(l, sizeof l, f)) {
        ln++; n = fields(l, v, 3); if (!n) continue;
        if (n != 2) die("states line needs: id name", p, ln);
        if (!number(v[0], &id)) die("state id is not a number", p, ln);
        if (id < 0 || id >= MAXS || sname[id]) die("bad or duplicate state id", p, ln);
        sname[id] = sdup(v[1]); if (id >= nsname) nsname = id + 1;
    }
    fclose(f);
}
static void read_edges(char *p) {
    FILE *f = fopen(p, "r"); char l[4096], *v[3]; int ln = 0, n;
    if (!f) die("cannot open edges", p, 0);
    while (fgets(l, sizeof l, f)) { ln++; n = fields(l, v, 3); if (!n) continue; if (n != 2) die("edges line needs: state key", p, ln); add_edge(v[0], v[1]); }
    fclose(f);
}
static void read_rows(char *p) {
    FILE *f = fopen(p, "r"); char l[8192], *v[5]; int ln = 0, n, cap = 4096;
    if (!f) die("cannot open rows", p, 0);
    rows = malloc(cap * sizeof *rows);
    while (fgets(l, sizeof l, f)) {
        ln++; n = fields(l, v, 5); if (!n) continue;
        if (n != 4) die("rows line needs: state key path line", p, ln);
        if (!strncmp(v[2], "synthetic", 9)) continue;     /* as rowcov.py: synthesised rows are not manifest rows */
        if (nrows == cap) { cap *= 2; rows = realloc(rows, cap * sizeof *rows); }
        rows[nrows].st = sdup(v[0]); rows[nrows].key = sdup(v[1]); rows[nrows].path = sdup(v[2]);
        rows[nrows].line = atoi(v[3]); rows[nrows].taken = 0; rows[nrows].wit = 0; nrows++;
    }
    fclose(f);
}
static void read_log(char *p, char *model) {
    FILE *f = fopen(p, "r"); char l[8192], *v[5]; int ln = 0, n, id;
    if (!f) die("cannot open edge log", p, 0);
    while (fgets(l, sizeof l, f)) {
        ln++; n = fields(l, v, 5); if (!n) continue;
        if (n != 4) die("edge log line needs: model state_id key probe", p, ln);
        if (strcmp(v[0], model)) die("edge log from another model", p, ln);
        if (!number(v[1], &id) || id < 0 || id >= nsname || !sname[id]) die("edge log state id not in the state dictionary", p, ln);
        take(sname[id], strcmp(v[2], "-1") ? v[2] : "BOT", v[3]);   /* cdx: BOT is key -1, not a state */
    }
    fclose(f);
}
static void read_seen(char *p) {
    FILE *f = fopen(p, "r"); char l[8192], *v[4]; int ln = 0, n;
    if (!f) die("cannot open seen", p, 0);
    while (fgets(l, sizeof l, f)) { ln++; n = fields(l, v, 4); if (!n) continue; if (n < 2) die("seen line needs: state key [probe]", p, ln); take(v[0], v[1], n > 2 ? v[2] : 0); }
    fclose(f);
}

static int byrow(const void *x, const void *y) {
    const Row *a = x, *b = y; int c = strcmp(a->path, b->path);
    return c ? c : (a->line > b->line) - (a->line < b->line);
}

/* rows: a row is covered when any counted edge it wrote was taken; witness = that edge's probe */
static int report(char *stage, int floor, char *cov) {
    int i, hit = 0, covered = 0, rows_n = 0, j;
    for (i = 0; i < ne; i++) hit += etaken[i];
    printf("rowcov %s  edges %d   taken %d   (%.1f%%)\n", stage, ne, hit, ne ? 100.0 * hit / ne : 0.0);
    for (i = 0; i < nrows; i++) {
        int e = find(rows[i].st, rows[i].key);
        if (e >= 0 && etaken[e]) { rows[i].taken = 1; if (!rows[i].wit) rows[i].wit = ewit[e]; }
    }
    /* distinct (path, line): sort once, then one pass */
    qsort(rows, nrows, sizeof *rows, byrow);
    for (i = 0; i < nrows; i = j) {
        int any = 0;
        for (j = i; j < nrows && rows[j].line == rows[i].line && !strcmp(rows[j].path, rows[i].path); j++) any |= rows[j].taken;
        rows_n++; covered += any;
    }
    printf("rowcov %s  rows %d   covered %d   (%.1f%%)\n", stage, rows_n, covered, rows_n ? 100.0 * covered / rows_n : 0.0);
    if (takenf) {        /* the taken edge set itself, for a line-by-line comparison (equal counts prove nothing) */
        FILE *o = fopen(takenf, "w"); if (!o) die("cannot write taken", takenf, 0);
        for (i = 0; i < ne; i++) if (etaken[i]) fprintf(o, "%s\n", ekey[i]);
        fclose(o);
    }
    if (cov) {
        FILE *o = fopen(cov, "w"); if (!o) die("cannot write cov", cov, 0);
        for (i = 0; i < nrows; i++) fprintf(o, "%s\t%d\t%s\t%s\n", rows[i].path, rows[i].line, rows[i].taken ? "taken" : "-", rows[i].wit ? rows[i].wit : "-");
        fclose(o);
    }
    if (floor >= 0 && covered < floor) { printf("rowcov %s  FELL below the baseline %d\n", stage, floor); return 1; }
    return 0;
}

static void put(char *p, char *text) { FILE *f = fopen(p, "w"); fputs(text, f); fclose(f); }

static int selftest(void) {
    char *d = getenv("TMPDIR"), b[5][512]; int rc, i;
    if (!d) d = "/tmp";
    for (i = 0; i < 5; i++) sprintf(b[i], "%s/rowcov-selftest-%d-%d.tsv", d, (int)getpid(), i);
    put(b[0], "0\tS0\n1\tS1\n2\tS2\n");
    put(b[1], "S0\tBOT\nS0\tb\nS1\ta\nS2\tz\n");                     /* 4 counted edges */
    put(b[2], "S0\tBOT\tm.tsv\t3\nS0\tb\tm.tsv\t3\nS1\ta\tm.tsv\t9\nS2\tz\tm.tsv\t12\nS9\tq\tsynthetic\t1\n");
    put(b[3], "M\t0\t-1\tp1.c\nM\t1\ta\tp2.c\nM\t1\ta\tp1.c\nM\t2\tREJ\tp2.c\n");
    for (i = 0; i < 2; i++) {
        /* run the same pipeline in-process */
        nsname = 0; memset(sname, 0, sizeof sname); ne = 0; memset(hidx, 0, sizeof hidx); nrows = 0;
        read_states(b[0]); read_edges(b[1]); read_rows(b[2]);
        if (i == 0) read_log(b[3], "M"); else { put(b[4], "S0\tBOT\tp1.c\nS1\ta\tp2.c\n"); read_seen(b[4]); }
        { int hit = 0, j; for (j = 0; j < ne; j++) hit += etaken[j];
          if (ne != 4 || hit != 2) { fprintf(stderr, "rowcov selftest: edges %d taken %d, want 4 2\n", ne, hit); return 1; } }
        rc = report("selftest", 2, 0);       /* rows m.tsv:3 m.tsv:9 m.tsv:12 -> covered 3 and 9 */
        if (rc) { fprintf(stderr, "rowcov selftest: floor 2 failed\n"); return 1; }
        if (report("selftest", 3, 0) != 1) { fprintf(stderr, "rowcov selftest: floor 3 must fail\n"); return 1; }
    }
    for (i = 0; i < 5; i++) remove(b[i]);
    printf("rowcov selftest: ok (log and seen routes, BOT key -1, rejection not counted, synthetic rows excluded, floor)\n");
    return 0;
}

int main(int argc, char **argv) {
    char *model = 0, *states = 0, *edges = 0, *rowsf = 0, *seen = 0, *cov = 0, *stage = "?";
    int floor = -1, i, nlog = 0; char *logs[4096];
    ekey = malloc(MAXE * sizeof *ekey); etaken = malloc(MAXE * sizeof *etaken); ewit = malloc(MAXE * sizeof *ewit);
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--selftest")) return selftest();
        else if (!strcmp(argv[i], "--model") && i + 1 < argc) model = argv[++i];
        else if (!strcmp(argv[i], "--states") && i + 1 < argc) states = argv[++i];
        else if (!strcmp(argv[i], "--edges") && i + 1 < argc) edges = argv[++i];
        else if (!strcmp(argv[i], "--rows") && i + 1 < argc) rowsf = argv[++i];
        else if (!strcmp(argv[i], "--seen") && i + 1 < argc) seen = argv[++i];
        else if (!strcmp(argv[i], "--floor") && i + 1 < argc) floor = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--cov") && i + 1 < argc) cov = argv[++i];
        else if (!strcmp(argv[i], "--taken") && i + 1 < argc) takenf = argv[++i];
        else if (!strcmp(argv[i], "--stage") && i + 1 < argc) stage = argv[++i];
        else if (argv[i][0] == '-') { fprintf(stderr, "rowcov: unknown option %s\n", argv[i]); return 2; }
        else if (nlog < 4096) logs[nlog++] = argv[i];
    }
    if (!edges || !rowsf || (!seen && (!states || !model || !nlog))) {
        fprintf(stderr, "usage: rowcov.c -- --stage S --model M --states S --edges E --rows R [--floor N] [--cov OUT] LOG...\n"
                        "       rowcov.c -- --stage S --edges E --rows R --seen F [--floor N]\n       rowcov.c -- --selftest\n");
        return 2;
    }
    if (states) read_states(states);
    read_edges(edges); read_rows(rowsf);
    if (seen) read_seen(seen); else for (i = 0; i < nlog; i++) read_log(logs[i], model);
    return report(stage, floor, cov);
}
