/* exec/c/run.c -- the generic delta executor in C: the machine of
   exec/pp/sim.py, action for action, on the integer table exec/c/tbl.py
   writes, or the integer threshold network exec/c/net.py constructs.
   It knows no language; transition decisions belong to the loaded model.

       run TABLE INPUT [SRCPATH] [INCLUDE_DIR]
       run --chain INPUT SRCPATH INCLUDE_DIR MODEL...
       run --bundle PACKAGE ROUTE INPUT [SRCPATH] [INCLUDE_DIR]

   INCLUDE_DIR (where the bundled headers are) should be ABSOLUTE: a relative
   one read from another CWD is ENOENT, i.e. "absent", not an error.

   exit 0 accept (o on stdout); 1 reject (the reason, then e, on stderr);
   2 a bad table; 3 out of steps (UNISA_MAXSTEPS, default 2e8). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>   /* clock_gettime for UNISA_STAGE_TIMES */

/* Standalone runner default; product drivers may override at compile time. */
#ifndef UNISA_DEFAULT_MAXSTEPS
#define UNISA_DEFAULT_MAXSTEPS 200000000LL
#endif

#ifdef UNISA_CORE_BLOB
#define UNISA_CORE_EXTERNAL
#endif
#include "core.h"
#ifndef UNISA_CORE_EXTERNAL
#include "core.c"
#endif

#ifndef UNISA_RUNTIME_STATE
#define UNISA_RUNTIME_STATE static
#endif
#ifndef UNISA_RUNTIME_PANIC
#define UNISA_RUNTIME_PANIC(m) do { fprintf(stderr,"run: %s\n",m); exit(2); } while (0)
#endif
static void die(const char *m) { UNISA_RUNTIME_PANIC(m); }
#include "codec.h"
#include "packagefooter.h"
static void *xrealloc(void *p, size_t n) { p = realloc(p, n ? n : 1); if (!p) die("out of memory"); return p; }

/* File operations are OS adaptation, not compiler actions. The native
   POSIX gates return -errno; the host wrapper normalises libc to that form.
   Windows native gates lack errno classification. Optional lookup follows
   the product's header-search contract: an unsuccessful open tries the next
   source. Explicit inputs and reported read/close errors still fail. */
#ifdef __UNISA__
static long io_open(const char *path) {
#ifdef _WIN32
    return __open((char *)path, 0x80000000, 3);
#else
    return __open((char *)path, 0, 0);
#endif
}
static long io_read(long fd, void *p, long n) { return __read(fd, p, n); }
static long io_close(long fd) { return __close(fd); }
#else
#include <unistd.h>
#include <fcntl.h>
static long io_open(const char *path) {
    int flags = O_RDONLY;
#ifdef _WIN32
    /* CRT text mode rewrites CRLF and stops on 0x1a in model packages. */
    flags |= O_BINARY;
#endif
    int fd = open(path, flags); return fd < 0 ? -errno : fd;
}
static long io_read(long fd, void *p, long n) { long r = read((int)fd, p, (size_t)n); return r < 0 ? -errno : r; }
static long io_close(long fd) { return close((int)fd); }
#endif
static unsigned char *readstream(long fd, const char *path, int *len) {
    int n = 0, cap = 65536; unsigned char *b = xrealloc(0, cap);
    for (;;) {
        if (n == cap) { if (cap > INT32_MAX/2) die("file too large"); cap *= 2; b = xrealloc(b, cap); }
        long r = io_read(fd, b+n, cap-n);
        if (r < 0) { fprintf(stderr, "run: cannot read %s\n", path); die("cannot read input"); }
        if (!r) break;
        if (r > cap-n) die("invalid read length");
        n += (int)r;
    }
    *len = n; return b;
}
static unsigned char *readfile(const char *path, int *len, int optional) {
    long fd = io_open(path);
    if (fd < 0) {
#if defined(__UNISA__) && defined(_WIN32)
        if (optional) return 0;
#else
        if (optional && fd == -ENOENT) return 0;
#endif
        fprintf(stderr, "run: cannot open %s\n", path); die("cannot open input");
    }
    unsigned char *b = readstream(fd,path,len);
    if (io_close(fd) < 0) die("close failed");
    return b;
}

/* Decimal model reader: no scanf dependency, and 64-bit arguments are
   checked before multiply/add (including the INT64_MIN magnitude). */
UNISA_RUNTIME_STATE unsigned char *LB; UNISA_RUNTIME_STATE int LP, LN, LBIN;
static void lskip(void) { if(!LBIN)while (LP < LN && LB[LP] <= 32) LP++; }
static int lchar(void) {
    if(LBIN){if(LP==LN)die("truncated binary model tag");return LB[LP++];}
    lskip(); if (LP == LN) die("truncated model");
    int c = LB[LP++]; if (LP < LN && LB[LP] > 32) die("bad model tag"); return c;
}
static void ltag(int c) { if (lchar() != c) die("bad model tag"); }
static I lnum(void) {
    if(LBIN){uint64_t v=0;int shift=0;
        for(int k=0;k<10;k++){
            if(LP==LN)die("truncated binary model number");int c=LB[LP++];
            if(k==9 && c>1)die("binary model number overflow");
            v|=(uint64_t)(c&127)<<shift;
            if(c<128){if(k && !c)die("noncanonical binary model number");return (I)((v>>1)^(0-(v&1)));}
            shift+=7;
        }die("long binary model number");
    }
    lskip(); int neg = 0, count = 0;
    if (LP < LN && LB[LP] == '-') { neg = 1; LP++; }
    uint64_t v = 0, lim = neg ? 9223372036854775808ull : 9223372036854775807ull;
    while (LP < LN && LB[LP] > 32) {
        int d = LB[LP++] - '0'; if (d < 0 || d > 9) die("bad model number");
        if (v > (lim - d)/10) die("model number overflow");
        v = v*10+d; count++;
    }
    if (!count) die("missing model number");
    return neg ? (I)(0-v) : (I)v;
}
static int lint(void) { I n = lnum(); if (n < INT32_MIN || n > INT32_MAX) die("model index overflow"); return (int)n; }

/* ---- the table ---- */
UNISA_RUNTIME_STATE int NS, NQ, NRG, NSTR, START;
UNISA_RUNTIME_STATE char **STR; UNISA_RUNTIME_STATE int *STRL;
UNISA_RUNTIME_STATE int *QOFF, *QLEN; UNISA_RUNTIME_STATE I *QA; UNISA_RUNTIME_STATE int NQA;          /* actions, flattened */
UNISA_RUNTIME_STATE int ISNET, TOPMAX;
UNISA_RUNTIME_STATE int *NLO, *NHI, *BN, *BQ, *RETS; UNISA_RUNTIME_STATE unsigned char **RETOK;
UNISA_RUNTIME_STATE int *SMODE; UNISA_RUNTIME_STATE int **ROWK, **ROWN, **ROWQ; UNISA_RUNTIME_STATE int *ROWC;

static int hexv(int c) { if (c >= '0' && c <= '9') return c-'0'; if (c >= 'a' && c <= 'f') return c-'a'+10; die("bad hex string"); return 0; }

static void loadbytes(unsigned char *data, int len) {
    LB = data; LN = len; LBIN=len>=8 && !memcmp(data,"UNINETB1",8); LP=LBIN?8:0;
    int kind = lchar(); NS=lint(); NQ=lint(); NRG=lint(); NSTR=lint(); START=lint();
    if ((kind != 'T' && kind != 'N') || NS <= 0 || NQ <= 0 || NRG < 0 || NSTR < 0 || START < 0 || START >= NS) die("bad header");
    if (NS > len || NQ > len || NSTR > len || NSTR == INT32_MAX) die("bad model count extent");
    ISNET = kind == 'N'; TOPMAX = NS - 1;
    if (ISNET) { TOPMAX=lint(); if (TOPMAX < NS-1 || TOPMAX == INT32_MAX) die("bad network domain"); }
    STR = xrealloc(0, sizeof *STR * (NSTR + 1)); STRL = xrealloc(0, sizeof *STRL * (NSTR + 1));
    for (int k = 0; k < NSTR; k++) {
        ltag('S');
        if (LBIN) {
            int n = lint(); if (n < 0 || n > LN-LP || n == INT32_MAX) die("bad binary model string");
            STR[k]=xrealloc(0,n+1); STRL[k]=n; memcpy(STR[k],LB+LP,n);
            STR[k][n]=0; LP+=n; continue;
        }
        lskip(); int begin = LP;
        while (LP < LN && LB[LP] > 32) LP++;
        int size = LP-begin; unsigned char *buf = LB+begin;
        int n = size == 1 && buf[0] == '-' ? 0 : size/2;
        if (size == 0 || ((size & 1) && !(size == 1 && buf[0] == '-'))) die("bad string");
        STR[k] = xrealloc(0, n + 1); STRL[k] = n;
        for (int j = 0; j < n; j++) STR[k][j] = (char)(hexv(buf[2 * j]) * 16 + hexv(buf[2 * j + 1]));
        STR[k][n] = 0;
    }
    QOFF = xrealloc(0, sizeof(int) * NQ); QLEN = xrealloc(0, sizeof(int) * NQ);
    int cap = 1 << 16; QA = xrealloc(0, sizeof(I) * cap); NQA = 0;
    for (int k = 0; k < NQ; k++) {
        int tag = lchar(), prefix = 0, copied = 0, from = 0, n;
        if (tag == 'C') {
            int ref = lint(); prefix = lint(); n = lint();
            if (ref < 0 || ref >= k || prefix <= 0 || prefix > QLEN[ref] ||
                n < 0 || n > INT32_MAX-prefix) die("bad sequence prefix");
            from = QOFF[ref]; int end = from;
            for (int a = 0; a < prefix; a++) end += 1 + ARITY[QA[end]];
            copied = end-from;
        } else {
            if (tag != 'Q') die("bad sequence tag");
            n = lint(); if (n < 0) die("bad sequence");
        }
        QOFF[k] = NQA; QLEN[k] = prefix+n;
        if (NQA > INT32_MAX-copied) die("sequence extent overflow");
        while (NQA+copied > cap) {
            if (cap > INT32_MAX/2) die("sequence capacity exceeded");
            cap *= 2; QA = xrealloc(QA, sizeof(I) * cap);
        }
        for (int i = 0; i < copied; i++) QA[NQA++] = QA[from+i];
        for (int a = 0; a < n; a++) {
            int op = lint(); if (op < 0 || op >= NOP_) die("bad opcode");
            if (NQA > INT32_MAX-6) die("sequence extent overflow");
            if (NQA+6 > cap) {
                if (cap > INT32_MAX/2) die("sequence capacity exceeded");
                cap *= 2; QA = xrealloc(QA, sizeof(I) * cap);
            }
            QA[NQA++] = op;
            for (int j = 0; j < ARITY[op]; j++) QA[NQA++] = lnum();
        }
    }
    SMODE = xrealloc(0, sizeof(int) * NS); ROWC = xrealloc(0, sizeof(int) * NS);
    ROWK = xrealloc(0, sizeof(int *) * NS); ROWN = xrealloc(0, sizeof(int *) * NS); ROWQ = xrealloc(0, sizeof(int *) * NS);
    NLO = xrealloc(0, sizeof(int) * NS); NHI = xrealloc(0, sizeof(int) * NS);
    BN = xrealloc(0, sizeof(int) * NS); BQ = xrealloc(0, sizeof(int) * NS);
    RETS = xrealloc(0, sizeof(int) * NS); RETOK = xrealloc(0, sizeof(unsigned char *) * NS);
    for (int s = 0; s < NS; s++) { RETS[s] = -1; RETOK[s] = 0; }
    for (int s = 0; s < NS; s++) {
        if (ISNET) {
            ltag('H'); int m=lint(), lo=lint(), hi=lint(), n=lint(), bn=lint(), bq=lint();
            if (m == 3) {   /* a stack bank with a declared return set */
                int rs = lint(), rn = lint(), prev = -1;
                if (rs < 0 || rs >= NQ || rn < 1 || rn > NS) die("bad return declaration");
                RETS[s] = rs; RETOK[s] = xrealloc(0, NS); memset(RETOK[s], 0, NS);
                for (int j = 0; j < rn; j++) {
                    int k = lint(); if (k <= prev || k >= NS) die("bad return declaration");
                    RETOK[s][k] = 1; prev = k;
                }
                m = 1;
            }
            if (m < 0 || m > 2 ||
                lo != (m == 1 ? -1 : 0) || hi != (m == 1 ? TOPMAX : 256) || n < 0 || (I)n > (I)hi - lo) die("bad network bank");
            SMODE[s] = m; NLO[s] = lo; NHI[s] = hi; ROWC[s] = n; BN[s] = bn; BQ[s] = bq;
            ROWK[s] = xrealloc(0, sizeof(int) * n); ROWN[s] = xrealloc(0, sizeof(int) * n); ROWQ[s] = xrealloc(0, sizeof(int) * n);
            int prev = lo;
            for (int j = 0; j < n; j++) {
                ROWK[s][j]=lint(); ROWN[s][j]=lint(); ROWQ[s][j]=lint();
                if (ROWK[s][j] <= prev || ROWK[s][j] > hi) die("bad network unit");
                prev = ROWK[s][j];
            }
            continue;
        }
        ltag('R'); int m=lint(), n=lint(), dn=lint(), dq=lint();
        if (m < 0 || m > 2 || n < 0) die("bad state");
        SMODE[s] = m; ROWC[s] = n;
        if (m == 1) {                                   /* keyed by the stack top: a short list */
            ROWK[s] = xrealloc(0, sizeof(int) * n); ROWN[s] = xrealloc(0, sizeof(int) * n); ROWQ[s] = xrealloc(0, sizeof(int) * n);
            for (int j = 0; j < n; j++) { ROWK[s][j]=lint(); ROWN[s][j]=lint(); ROWQ[s][j]=lint(); }
        } else {                                        /* keyed 0..256: direct */
            ROWN[s] = xrealloc(0, sizeof(int) * 257); ROWQ[s] = xrealloc(0, sizeof(int) * 257); ROWK[s] = 0;
            for (int j = 0; j < 257; j++) { ROWN[s][j] = dn; ROWQ[s][j] = dq; }
            for (int j = 0; j < n; j++) { int k=lint(), nx=lint(), q=lint(); if (k < 0 || k > 256 || nx < -1 || nx >= NS) die("bad row"); ROWN[s][k] = nx; ROWQ[s][k] = q; }
        }
    }
    lskip(); if (LP != LN) die("trailing model data");
    if (!ISNET) {
        for (int s = 0; s < NS; s++) if (SMODE[s] == 1)
            for (int j = 0; j < ROWC[s]; j++) if (ROWK[s][j] > TOPMAX) TOPMAX = ROWK[s][j];
        for (int i = 0; i < NQA; i += 1 + ARITY[QA[i]])
            if (QA[i] == PUSH && QA[i+1] > TOPMAX) TOPMAX = (int)QA[i+1];
    }
}

#ifndef UNISA_RUNTIME_LIBRARY
static void load(const char *path) {
    int n = 0; unsigned char *data = readfile(path, &n, 0);
    loadbytes(data, n); free(data); LB = 0;
}
#endif

/* A package carries generic named byte-stream routes, not compiler stages.
   Shared model spans are kept once. All directory bounds and format edges
   are checked before a route is executed. Model bodies use the same loader. */
typedef struct { char *route; char *name; char *in; char *out; int model; } Stage;
UNISA_RUNTIME_STATE unsigned char *PFILE, *PB; UNISA_RUNTIME_STATE int PN, PM, PS;
UNISA_RUNTIME_STATE int *POFF, *PLEN, *PRAW; UNISA_RUNTIME_STATE uint32_t *PCRC; UNISA_RUNTIME_STATE int PVER; UNISA_RUNTIME_STATE Stage *STAGES;
typedef struct { int name, n, data, len; } Resource;
UNISA_RUNTIME_STATE Resource *RES; UNISA_RUNTIME_STATE int NR;
/* Borrowed process inputs override carried resources by exact byte key.
   The caller keeps these immutable spans alive for the whole route. */
typedef struct { const unsigned char *name; int n; const unsigned char *data; int len; } ResourceInput;
UNISA_RUNTIME_STATE ResourceInput *RI; UNISA_RUNTIME_STATE int NRI;
static char *pword(void) {
    lskip(); int first = LP;
    while (LP < LN && LB[LP] > 32) {
        int c = LB[LP++];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.' || c == '/')) die("bad package name");
    }
    int n = LP-first; if (!n) die("missing package name");
    char *v = xrealloc(0, n+1); memcpy(v, LB+first, n); v[n] = 0; return v;
}
static void package(const char *path) {
    PFILE = readfile(path, &PN, 0); PB = PFILE;
    if (PN < 2 || PB[0] != 'P' || PB[1] != ' ') {
        PackageFooter footer;
        /* Signature records follow the package footer in Authenticode APEs.
           Locate only within the validated file/certificate extents. This is
           format handling, not a signature trust decision. */
        if (!packagefooter_locate_bytes(PFILE, PN, &footer)) die("bad or missing package footer");
        PB += (int)footer.package_offset; PN = (int)footer.package_length;
    }
    LBIN = 0; LB = PB; LN = PN; LP = 0;
    ltag('P'); int version = lint();
    if (version != 1 && version != 2 && version != 3) die("unknown package version");
    PVER = version; if (version == 3) crc_init();
    PM = lint(); PS = lint(); NR = version >= 2 ? lint() : 0;
    if (NR < 0 || NR > PN/5) die("bad resource count");
    RES = xrealloc(0, sizeof(Resource)*NR);
    if (PM <= 0 || PS <= 0 || PM > PN/8 || PS > PN/10) die("bad package count");
    POFF = xrealloc(0, sizeof(int)*PM); PLEN = xrealloc(0, sizeof(int)*PM);
    PRAW = xrealloc(0, sizeof(int)*PM); PCRC = xrealloc(0, sizeof(uint32_t)*PM);
    STAGES = xrealloc(0, sizeof(Stage)*PS);
    for (int i = 0; i < PS; i++) {
        ltag('D'); Stage *s = &STAGES[i];
        s->route = pword(); s->name = pword(); s->in = pword(); s->out = pword(); s->model = lint();
        if (s->model < 0 || s->model >= PM) die("bad package model index");
        int previous = -1;
        for (int j = 0; j < i; j++) if (!strcmp(s->route, STAGES[j].route)) {
            if (!strcmp(s->name, STAGES[j].name)) die("duplicate package stage");
            previous = j;
        }
        if (previous >= 0 && strcmp(STAGES[previous].out, s->in)) die("package format mismatch");
    }
    for (int i = 0; i < PM; i++) {
        ltag('M'); int n = lint(); PRAW[i]=n; PCRC[i]=0;
        if (version == 3) {
            PRAW[i]=lint(); int codec=lint(); I checksum=lnum();
            if (PRAW[i]<=0 || codec!=1 || checksum<0 || checksum>(I)UINT32_MAX) die("bad compressed model header");
            PCRC[i]=(uint32_t)checksum;
        }
        if (LP >= LN || LB[LP++] != 10 || n <= 0 || n > LN-LP) die("bad package model extent");
        if (version < 3 && LB[LP] != 'N') die("package requires networks");
        POFF[i] = LP; PLEN[i] = n; LP += n;
    }
    for (int i = 0; i < NR; i++) {
        ltag('F'); int n = lint(), len = lint();
        if (LP >= LN || LB[LP++] != 10 || n <= 0 || len < 0 || n > LN-LP || len > LN-LP-n) die("bad resource extent");
        RES[i].name = LP; RES[i].n = n; RES[i].data = LP+n; RES[i].len = len;
        for (int j = 0; j < i; j++)
            if (RES[j].n == n && !memcmp(PB+RES[j].name, PB+LP, n)) die("duplicate resource name");
        LP += n+len;
    }
    if (LP != LN) die("trailing package data");
}
static void unpackage(void) {
    for (int i = 0; i < PS; i++) {
        free(STAGES[i].route); free(STAGES[i].name); free(STAGES[i].in); free(STAGES[i].out);
    }
    free(STAGES); free(POFF); free(PLEN); free(PRAW); free(PCRC); free(RES); RES = 0; NR = 0; free(PFILE);
}

/* Model lifetime is one stage. No model-specific state survives unload. */
static void unload(void) {
    for (int i = 0; i < NSTR; i++) free(STR[i]);
    for (int i = 0; i < NS; i++) { free(ROWK[i]); free(ROWN[i]); free(ROWQ[i]); }
    free(STR); free(STRL); free(QOFF); free(QLEN); free(QA);
    free(SMODE); free(ROWC); free(ROWK); free(ROWN); free(ROWQ);
    free(NLO); free(NHI); free(BN); free(BQ);
    for (int i = 0; i < NS; i++) free(RETOK[i]);
    free(RETS); free(RETOK);
}

static void core_model(CoreModel *m) {
    m->ns=NS;
    m->nq=NQ;
    m->nrg=NRG;
    m->start=START;
    m->isnet=ISNET;
    m->str=STR;
    m->strl=STRL;
    m->qoff=QOFF;
    m->qlen=QLEN;
    m->qa=QA;
    m->mode=SMODE;
    m->count=ROWC;
    m->keys=ROWK;
    m->next=ROWN;
    m->seq=ROWQ;
    m->lo=NLO;
    m->hi=NHI;
    m->base_next=BN;
    m->base_seq=BQ;
    m->ret_seq=RETS;
    m->ret_ok=RETOK;
}
#ifndef UNISA_RUNTIME_LIBRARY
static void transition(int q,int key,int *nx,int *sq) {
    CoreModel m; core_model(&m);
    const char *why=core_transition(&m,q,key,nx,sq); if (why) die(why);
}

static int checknet(const char *table, const char *net) {
    load(table); if (ISNET) die("expected reference table");
    int ns = NS, nq = NQ, nr = NRG, nstr = NSTR, start = START, top = TOPMAX, nqa = NQA;
    int *modes = SMODE, *qlen = QLEN, *sl = STRL; I *qa = QA; char **str = STR;
    size_t total = 0;
    for (int q = 0; q < ns; q++) total += modes[q] == 1 ? (size_t)top + 2 : 257;
    int *en = xrealloc(0, total * sizeof(int)), *es = xrealloc(0, total * sizeof(int));
    size_t at = 0;
    for (int q = 0; q < ns; q++) for (int k = modes[q] == 1 ? -1 : 0, hi = modes[q] == 1 ? top : 256; k <= hi; k++)
        transition(q, k, &en[at], &es[at]), at++;
    load(net);
    if (!ISNET || NS != ns || NQ != nq || NRG != nr || NSTR != nstr || START != start || TOPMAX != top || NQA != nqa ||
        memcmp(qa, QA, sizeof(I)*nqa) || memcmp(qlen, QLEN, sizeof(int)*nq)) die("network changed action declarations");
    for (int i = 0; i < nstr; i++) if (sl[i] != STRL[i] || memcmp(str[i], STR[i], sl[i])) die("network changed strings");
    at = 0;
    for (int q = 0; q < ns; q++) {
        if (modes[q] != SMODE[q]) die("network changed observation mode");
        for (int k = NLO[q]; k <= NHI[q]; k++) {
            int n, s; transition(q, k, &n, &s);
            if (n != en[at] || s != es[at]) { fprintf(stderr, "network differs: state %d key %d\n", q, k); return 1; }
            at++;
        }
    }
    printf("network = table: %zu observations, %d states; actions/strings identical\n", at, ns);
    return 0;
}

#endif

/* Exact resource lookup and filesystem naming belong to the host adapter. */
void core_host_panic(const char *reason) { die(reason); }
UNISA_RUNTIME_STATE const char *INCDIR=0;
/* Optional host IO ledger. It records successful disk reads, not include
   syntax or carried/process resources. Cached requests need only one make
   prerequisite; no language decision is made here. */
UNISA_RUNTIME_STATE int FILE_READ_RECORD=0, FILE_READ_COUNT=0;
UNISA_RUNTIME_STATE char **FILE_READ_PATHS=0;
static void record_file_read(const char *path) {
    if (!FILE_READ_RECORD) return;
    for (int i=0;i<FILE_READ_COUNT;i++) if (!strcmp(FILE_READ_PATHS[i],path)) return;
    FILE_READ_PATHS=xrealloc(FILE_READ_PATHS,(FILE_READ_COUNT+1)*sizeof(char *));
    int n=strlen(path)+1;
    FILE_READ_PATHS[FILE_READ_COUNT]=xrealloc(0,n);
    memcpy(FILE_READ_PATHS[FILE_READ_COUNT++],path,n);
}

int core_host_fetch(const unsigned char *p,int n,unsigned char **bytes,int *len) {
    for (int j=0;j<NRI;j++) if (RI[j].n==n && !memcmp(RI[j].name,p,n)) {
        *bytes=(unsigned char *)RI[j].data; *len=RI[j].len; return 1;
    }
    for (int j=0;j<NR;j++) if (RES[j].n==n && !memcmp(PB+RES[j].name,p,n)) {
        *bytes=PB+RES[j].data; *len=RES[j].len; return 1;
    }
    char path[4096];
    int header=n>=5 && !memcmp(p,"\0hdr/",5);
    /* A bundled header that cannot be produced is NOT this layer's error to
       report: the resource is simply absent (return 0), which is what the E2
       stage already turns into its own `no such file for #include` reject.
       Dying here pre-empted that reject and cost the diagnostic its position
       and the header's name -- both of which E2 has and this function does
       not, since it is handed a resource key and never sees a token.  With no
       -I at all, and with an -I whose directory lacks the header, the answer is
       the same: absent.  [R13-0b #07] */
    if (!header) { snprintf(path,sizeof path,"%.*s",n,p); }
    else {
        if (!INCDIR) return 0;
        snprintf(path,sizeof path,"%s/%.*s",INCDIR,n-5,p+5);
    }
    if ((int)strlen(path)!=(header ? (int)strlen(INCDIR)+1+n-5 : n)) return 0;
    *bytes=readfile(path,len,1);
    if (*bytes) record_file_read(path);
    return *bytes ? 2 : 0;
}
#ifdef UNISA_RUNTIME_LIBRARY
/* CLI byte framing is host IO, not a machine action. */
static void bput(Buf *o,int c,I at) {
    if (o->n>=o->cap) { o->cap=o->cap ? o->cap*2 : 256;
        o->b=xrealloc(o->b,o->cap); o->at=xrealloc(o->at,sizeof(I)*o->cap); }
    o->b[o->n]=(unsigned char)c; o->at[o->n]=at; o->n++;
}
#endif
static int execute(unsigned char *input,int inputn,const char *src,Buf *result) {
    CoreModel m; core_model(&m);
    I maxsteps=getenv("UNISA_MAXSTEPS") ? strtol(getenv("UNISA_MAXSTEPS"),0,10) : UNISA_DEFAULT_MAXSTEPS;
    CoreResult r; int status=core_run(&m,input,inputn,src,maxsteps,&r);
#ifdef UNISA_RUNTIME_DIAGNOSTIC
    UNISA_RUNTIME_DIAGNOSTIC(status,r.reason,r.reason_n,&r.err);
#endif
    if (status==1 && r.reason_n) fprintf(stderr,"reject: %.*s\n",r.reason_n,r.reason);
    if (status==2) fprintf(stderr,"run: %s\n",r.reason);
    if (status==3) fprintf(stderr,"timeout\n");
    if (r.err.n) fwrite(r.err.b,1,r.err.n,stderr);
    free(r.err.b); result->b=r.out.b; result->n=r.out.n;
    return status;
}

/* Route dispatch owns only byte-stream lifetimes, never compiler semantics. */
/* 0.0.28 F1 (driver IO only): E1 run with \0cli/f1 prefixes its output with a USLATTR1 side-car
   -- "USLATTR1", u32 count, u32 token_len, count x (u32 ordinal, u8 kind) -- before the ordinary
   E1 bytes.  The driver strips it right after the stage that wrote it, so every later stage sees
   the default E1 bytes, and collects the records, tagged with the unit they came from, into the
   \0cli/attributes resource E3 reads: u32 count, then count x (u32 unit, u32 ordinal, u8 kind).
   No decision is made here: the records are passed through as E1 wrote them. */
UNISA_RUNTIME_STATE Buf ATTRS; UNISA_RUNTIME_STATE int ATTR_UNIT; UNISA_RUNTIME_STATE int ATTR_SLOT = -1;
static void attr_put(int c) {      /* bput is only in the runtime-library build */
    if (ATTRS.n >= ATTRS.cap) { ATTRS.cap = ATTRS.cap ? ATTRS.cap * 2 : 256; ATTRS.b = xrealloc(ATTRS.b, ATTRS.cap); }
    ATTRS.b[ATTRS.n++] = (unsigned char)c;
}
static unsigned attr_u32(const unsigned char *p) { return p[0] | p[1] << 8 | p[2] << 16 | (unsigned)p[3] << 24; }
static int attr_strip(Buf *out) {
    if (out->n < 16 || memcmp(out->b, "USLATTR1", 8)) return 0;
    unsigned count = attr_u32(out->b + 8);
    long head = 16 + 5L * count;
    if (count > 1000000u || head > out->n) die("malformed USLATTR1 side-car");
    if (ATTRS.n == 0) { for (int k = 0; k < 4; k++) attr_put(0); }
    unsigned total = attr_u32(ATTRS.b) + count;
    for (unsigned r = 0; r < count; r++) {
        const unsigned char *q = out->b + 16 + 5 * r;
        for (int k = 0; k < 4; k++) attr_put((ATTR_UNIT >> (8 * k)) & 255);
        for (int k = 0; k < 5; k++) attr_put(q[k]);
    }
    for (int k = 0; k < 4; k++) ATTRS.b[k] = (total >> (8 * k)) & 255;
    memmove(out->b, out->b + head, out->n - head); out->n -= (int)head;
    if (ATTR_SLOT >= 0) { RI[ATTR_SLOT].data = ATTRS.b; RI[ATTR_SLOT].len = ATTRS.n; }
    return 1;
}
static int runroute_range(const char *route, const char *first_stage, const char *last_stage, Buf *in, const char *src) {
    int count = 0, ready = first_stage == 0, ended = last_stage == 0;
    for (int i = 0; i < PS; i++) {
        if (strcmp(STAGES[i].route, route)) continue;
        if (!ready) {
            if (strcmp(STAGES[i].name,first_stage)) continue;
            ready=1;
        }
        int m = STAGES[i].model;
        unsigned char *bytes = PB+POFF[m], *owned = 0; int len = PLEN[m];
        if (PVER == 3) {
            owned = xrealloc(0, PRAW[m]);
            if (unisa_inflate(owned, PRAW[m], bytes, len)) die("invalid compressed model");
            if (crc((char *)owned, PRAW[m]) != PCRC[m]) die("compressed model CRC");
            if (PRAW[m] < 9 || memcmp(owned,"UNINETB1N",9)) die("package requires binary networks");
            bytes = owned; len = PRAW[m];
        }
        loadbytes(bytes, len); free(owned); LB = 0;
        struct timespec st0, st1; int timing = getenv("UNISA_STAGE_TIMES") != 0;
        if (timing) clock_gettime(CLOCK_MONOTONIC, &st0);
        Buf out = {0}; int rc = execute(in->b, in->n, src, &out);
        if (timing) { clock_gettime(CLOCK_MONOTONIC, &st1);   /* 0.0.28 F2: where a compile spends its time */
            fprintf(stderr, "stage %s/%s %.3f s in %d out %d\n", route, STAGES[i].name,
                    (st1.tv_sec - st0.tv_sec) + (st1.tv_nsec - st0.tv_nsec) / 1e9, in->n, out.n); }
        if (!rc) attr_strip(&out);
        unload(); free(in->b); in->b = out.b; in->n = out.n;
        if (rc) return rc;
        count++;
        if (last_stage && !strcmp(STAGES[i].name,last_stage)) { ended=1; break; }
    }
    if (!count || !ended) die("unknown package route or stage boundary");
    return 0;
}
static int runroute_from(const char *route, const char *first_stage, Buf *in, const char *src) {
    return runroute_range(route,first_stage,0,in,src);
}
static int runroute(const char *route, Buf *in, const char *src) {
    return runroute_range(route,0,0,in,src);
}

#ifdef UNISA_CORE_BLOB
#include "asm/binding.c"
#endif

#ifndef UNISA_RUNTIME_LIBRARY
int main(int argc, char **argv) {
    if (argc == 4 && !strcmp(argv[1], "--check-net")) return checknet(argv[2], argv[3]);
    int chain = argc > 1 && !strcmp(argv[1], "--chain");
    int embedded = argc > 1 && !strcmp(argv[1], "--embedded");
    int bundled = embedded || (argc > 1 && !strcmp(argv[1], "--bundle"));
    if ((!chain && !bundled && argc < 3) || (chain && argc < 6) || (bundled && argc < (embedded ? 4 : 5))) {
        fprintf(stderr, "usage: run MODEL INPUT [SRCPATH] [INCLUDE_DIR]\n       run --chain INPUT SRCPATH INCLUDE_DIR MODEL...\n       run --bundle PACKAGE ROUTE INPUT [SRCPATH] [INCLUDE_DIR]\n       run --embedded ROUTE INPUT [SRCPATH] [INCLUDE_DIR]\n");
        return 2;
    }
    int routearg = embedded ? 2 : 3;
    int inputarg = bundled ? routearg+1 : 2;
    const char *src = argc > inputarg+1 ? argv[inputarg+1] : argv[inputarg];
    if (argc > inputarg+2) INCDIR = argv[inputarg+2];
    if (bundled) {
        const char *path = embedded ? getenv("UNISA_CONTAINER") : argv[2];
        package(path ? path : argv[0]);
    }
    /* 0.0.28 F1: like the driver, ask E1 for its constructor/destructor side-car and hand the
       stripped records to the next stage as \0cli/attributes (stage-by-stage runs pass the stripped
       E1 output on disk, so the records travel in UNISA_ATTRIBUTES when the caller sets it) */
    static const unsigned char f1one = 1; static ResourceInput sres[2];
    sres[0].name = (const unsigned char *)"\0cli/f1"; sres[0].n = 7; sres[0].data = &f1one; sres[0].len = 1;
    sres[1].name = (const unsigned char *)"\0cli/attributes"; sres[1].n = 15; sres[1].data = 0; sres[1].len = 0;
    if (!RI) { RI = sres; NRI = 2; ATTR_SLOT = 1; }
    { const char *ap = getenv("UNISA_ATTRIBUTES"); if (ap && *ap) { int an = 0; unsigned char *ab = readfile(ap, &an, 1);
        if (ab) { ATTRS.b = ab; ATTRS.n = an; ATTRS.cap = an; sres[1].data = ab; sres[1].len = an; } } }
    Buf in = {0}; in.b = readfile(argv[inputarg], &in.n, 0);
    if (bundled) {
        int rc = runroute(argv[routearg], &in, src); unpackage();
        if (rc) return rc;
    } else {
        int first = chain ? 5 : 1, end = chain ? argc : 2;
        for (int i = first; i < end; i++) {
            load(argv[i]);
            Buf out = {0}; int rc = execute(in.b, in.n, src, &out);
            if (!rc && attr_strip(&out)) {
                const char *ap = getenv("UNISA_ATTRIBUTES");
                if (ap && *ap) { FILE *af = fopen(ap, "wb"); if (!af || fwrite(ATTRS.b, 1, ATTRS.n, af) != (size_t)ATTRS.n || fclose(af)) die("cannot write UNISA_ATTRIBUTES"); }
            }
            unload(); free(in.b);
            if (rc) return rc;
            in.b = out.b; in.n = out.n;
        }
    }
    if (fwrite(in.b, 1, in.n, stdout) != (size_t)in.n || fclose(stdout)) die("cannot write output");
    free(in.b); return 0;
}

#endif
