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
#include <time.h>

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
#ifdef UNISA_CX_CODEC
/* Transitional ABI: the context is owned by this runtime instance; library
 * builds may make UNISA_RUNTIME_STATE thread-local. */
typedef struct { uint32_t table[4][256]; } CxCodecCrc;
int cxcodec_crc_init(CxCodecCrc *);
int cxcodec_crc(CxCodecCrc *,const char *,int,uint32_t *);
int cxcodec_inflate(unsigned char *,unsigned long,const unsigned char *,unsigned long);
UNISA_RUNTIME_STATE CxCodecCrc cx_runtime_crc;
static int crc_init(void) { return cxcodec_crc_init(&cx_runtime_crc); }
static uint32_t crc(char *p,int n) { uint32_t out; if(cxcodec_crc(&cx_runtime_crc,p,n,&out))die("invalid CRC input"); return out; }
static int unisa_inflate(unsigned char *out,unsigned long outlen,const unsigned char *in,unsigned long inlen) { return cxcodec_inflate(out,outlen,in,inlen); }
#else
#include "codec.h"
#endif
#ifdef UNISA_CX_PACKAGEFOOTER
typedef struct {
    long file_size, footer_offset, package_offset, package_length;
    long certificate_offset, certificate_length;
    int padding;
} PackageFooter;
int packagefooter_locate_bytes(const unsigned char *, long, PackageFooter *);
#else
#include "packagefooter.h"
#endif
static void *xrealloc(void *p, size_t n) { p = realloc(p, n ? n : 1); if (!p) die("out of memory"); return p; }
/* Package bytes and decoded model tables are owned by their cache (one per
   library context; one static slot for the command line), not by the
   per-call runtime allocation domain. The library routes these through an
   untracked allocator; the command line uses plain realloc/free. */
#ifndef UNISA_PACKAGE_REALLOC
#define UNISA_PACKAGE_REALLOC realloc
#define UNISA_PACKAGE_FREE free
#endif
/* UNISA_PACKAGE_CACHE (defined by the library) selects the per-context cache
   entry points instead of the command-line package()/unpackage() pair. */
static void *mrealloc(void *p, size_t n) { p = UNISA_PACKAGE_REALLOC(p, n ? n : 1); if (!p) die("out of memory"); return p; }
static void mfree(void *p) { UNISA_PACKAGE_FREE(p); }

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
/* Identity of an input file as far as the build can observe it; off is the
   read position at stamp time (size - off is what a read will see). */
typedef struct { long long size, off, mtime, mtime_ns, ctime, ctime_ns, ino, dev; unsigned tag; int valid; } FileStamp;
/* No stat gate: the stamp is the size plus an FNV-1a tag of the first and last
   4 KiB (two seeks and <= 8 KiB of reads per call). A same-size replacement
   that differs only in the middle is not seen. The offset is restored. */
#ifdef UNISA_CX_SOURCE_FRAME
int cxsf_stamp_unisa(long, long (*)(long, long, int), long (*)(long, unsigned char *, long),
                     void (*)(const char *), long long *, long long *, unsigned *, int *);
static long us_seek(long fd, long off, int whence) { return __lseek(fd, off, whence); }
static long us_read(long fd, unsigned char *p, long n) { return __read(fd, p, n); }
#else
static unsigned stamp_tag(long fd, long at, unsigned h, int *ok) {
    unsigned char b[4096]; long n = 0, r;
    if (__lseek(fd, at, 0) < 0) { *ok = 0; return h; }
    while (n < 4096) { r = __read(fd, b + n, 4096 - n); if (r < 0) { *ok = 0; return h; } if (!r) break; n += r; }
    for (long i = 0; i < n; i++) h = (h ^ b[i]) * 16777619u;
    return h;
}
#endif
static int io_stamp(long fd, FileStamp *s) {
    memset(s, 0, sizeof *s);
#ifdef UNISA_CX_SOURCE_FRAME
    return cxsf_stamp_unisa(fd, us_seek, us_read, die,
                            &s->size, &s->off, &s->tag, &s->valid) == 1;
#else
    long cur = __lseek(fd, 0, 1); if (cur < 0) return 0;
    long end = __lseek(fd, 0, 2); if (end < 0) return 0;
    int ok = 1; unsigned h = 2166136261u;
    h = stamp_tag(fd, 0, h, &ok); if (ok && end > 4096) h = stamp_tag(fd, end - 4096, h, &ok);
    if (__lseek(fd, cur, 0) < 0) die("cannot read input");
    if (!ok) return 0;
    s->size = end; s->off = cur; s->tag = h; s->valid = 1; return 1;
#endif
}
#else
#include <unistd.h>
#include <fcntl.h>
#ifdef UNISA_CX_SOURCE_FRAME
int cxsf_open_flags(int, int, int);
long cxsf_open_errno(long, long);
long cxsf_read_errno(long, long);
long cxsf_close_rc(long, long);
#endif
static long io_open(const char *path) {
#ifdef UNISA_CX_SOURCE_FRAME
#ifdef _WIN32
    /* CRT text mode rewrites CRLF and stops on 0x1a in model packages. */
    int flags = cxsf_open_flags(O_RDONLY, O_BINARY, 1);
#else
    int flags = cxsf_open_flags(O_RDONLY, 0, 0);
#endif
#else
    int flags = O_RDONLY;
#ifdef _WIN32
    /* CRT text mode rewrites CRLF and stops on 0x1a in model packages. */
    flags |= O_BINARY;
#endif
#endif
    int fd = open(path, flags);
#ifdef UNISA_CX_SOURCE_FRAME
    return cxsf_open_errno(fd, errno);
#else
    return fd < 0 ? -errno : fd;
#endif
}
static long io_read(long fd, void *p, long n) {
    long r = read((int)fd, p, (size_t)n);
#ifdef UNISA_CX_SOURCE_FRAME
    return cxsf_read_errno(r, errno);
#else
    return r < 0 ? -errno : r;
#endif
}
static long io_close(long fd) {
    long r = close((int)fd);
#ifdef UNISA_CX_SOURCE_FRAME
    return cxsf_close_rc(r, errno);
#else
    return r;
#endif
}
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
/* Identity of an input file as far as the build can observe it; off is the
   read position at stamp time (size - off is what a read will see). */
typedef struct { long long size, off, mtime, mtime_ns, ctime, ctime_ns, ino, dev; unsigned tag; int valid; } FileStamp;
#ifdef UNISA_CX_SOURCE_FRAME
int cxsf_stamp_reg(int, int, long long, long long, long long, long long, long long, long long, long long,
                   long long *, long long *, long long *, long long *, long long *, long long *,
                   long long *, long long *, unsigned *, int *);
#endif
static int io_stamp(long fd, FileStamp *s) {
    memset(s, 0, sizeof *s);
#ifdef UNISA_CX_SOURCE_FRAME
    int stat_ok = 0, is_reg = 0;
    long long size = 0, mtime = 0, mtime_ns = 0, ctime = 0, ctime_ns = 0, ino = 0, dev = 0;
#ifdef _WIN32
    struct _stat64 st;
    if (!_fstat64((int)fd, &st)) stat_ok = 1;
#else
    struct stat st;
    if (!fstat((int)fd, &st)) stat_ok = 1;
#endif
    if (stat_ok) {
#ifdef S_ISREG
        is_reg = S_ISREG(st.st_mode) ? 1 : 0;
#else
        is_reg = ((st.st_mode & S_IFMT) == S_IFREG) ? 1 : 0;
#endif
        size = st.st_size; mtime = st.st_mtime; ctime = st.st_ctime;
        ino = (long long)st.st_ino; dev = (long long)st.st_dev;
#if defined(__linux__) && defined(st_mtime)
        mtime_ns = st.st_mtim.tv_nsec; ctime_ns = st.st_ctim.tv_nsec;
#elif defined(__APPLE__)
        mtime_ns = st.st_mtimespec.tv_nsec; ctime_ns = st.st_ctimespec.tv_nsec;
#endif
    }
    return cxsf_stamp_reg(stat_ok, is_reg, size, mtime, mtime_ns, ctime, ctime_ns, ino, dev,
                          &s->size, &s->off, &s->mtime, &s->mtime_ns, &s->ctime, &s->ctime_ns,
                          &s->ino, &s->dev, &s->tag, &s->valid) == 1;
#else
#ifdef _WIN32
    struct _stat64 st; if (_fstat64((int)fd, &st)) return 0;
#else
    struct stat st; if (fstat((int)fd, &st)) return 0;
#endif
#ifdef S_ISREG
    if (!S_ISREG(st.st_mode)) return 0;
#else
    if ((st.st_mode & S_IFMT) != S_IFREG) return 0;
#endif
    s->size = st.st_size; s->mtime = st.st_mtime; s->ctime = st.st_ctime;
    s->ino = (long long)st.st_ino; s->dev = (long long)st.st_dev;
#if defined(__linux__) && defined(st_mtime)
    s->mtime_ns = st.st_mtim.tv_nsec; s->ctime_ns = st.st_ctim.tv_nsec;
#elif defined(__APPLE__)
    s->mtime_ns = st.st_mtimespec.tv_nsec; s->ctime_ns = st.st_ctimespec.tv_nsec;
#endif
    s->valid = 1; return 1;
#endif
}
#endif
/* Regular files are read into one buffer sized from the stamp (one spare
   byte lets the final read observe EOF); streams keep the doubling buffer. */
static unsigned char *readstream(long fd, const char *path, int *len);
#ifdef UNISA_CX_SOURCE_FRAME
const char *cxsf_open_reason(long fd, int windows, long enoent, long enotdir, long eacces);
int cxsf_open_gate(long fd, int optional, int unisa_windows, long enoent);
int cxsf_open_line(char *dst, int cap, const char *path, const char *reason);
int cxsf_read_cap(int stamped, long long size, long long off);
int cxsf_read_grow(int cap, int *next);
int cxsf_read_take(long r, int cap, int n, int *out);
int cxsf_readfile(const char *, int *, int, int, long, char *, int,
                  long (*)(const char *), const char *(*)(long),
                  void (*)(const char *), void (*)(const char *, const char *),
                  void (*)(const char *),
                  unsigned char *(*)(long, const char *, int *),
                  long (*)(long), unsigned char **);
int cxsf_read_err(char *, int, const char *);
int cxsf_close_fail(long, void (*)(const char *));
int cxsf_readstream(long, const char *, int *, char *, int,
                    int (*)(long, long long *, long long *),
                    void *(*)(void *, size_t),
                    long (*)(long, unsigned char *, long),
                    void (*)(const char *), void (*)(const char *), void (*)(const char *),
                    unsigned char **, unsigned char **);
static int rf_stamp(long fd, long long *size, long long *off) {
    FileStamp st;
    if (!io_stamp(fd, &st)) return 0;
    *size = st.size;
    *off = st.off;
    return 1;
}
static void rf_read_say(const char *line) { fputs(line, stderr); }
static void rf_read_raw(const char *path) { fprintf(stderr, "run: cannot read %s\n", path); }
static long rf_read(long fd, unsigned char *p, long n) { return io_read(fd, p, n); }
static unsigned char *readstream_with(long fd, const char *path, int *len, void *(*grow)(void *, size_t), unsigned char **own) {
    unsigned char *tmp = 0, *b = 0, **slot = own ? own : &tmp;
    char line[4096];
    int code = cxsf_readstream(fd, path, len, line, (int)sizeof line,
                               rf_stamp, grow, rf_read, rf_read_say, rf_read_raw,
                               die, slot, &b);
    if (code == 4) return 0;
    return b;
}
static const char *io_open_reason(long fd) {
#ifdef _WIN32
    return cxsf_open_reason(fd, 1, 0, 0, 0);
#else
    return cxsf_open_reason(fd, 0, ENOENT, ENOTDIR, EACCES);
#endif
}
static void rf_open_say(const char *line) { fputs(line, stderr); }
static void rf_open_raw(const char *path, const char *reason) {
    fprintf(stderr, "run: cannot open %s%s%s\n", path,
            reason ? ": " : "", reason ? reason : "");
}
static unsigned char *readfile(const char *path, int *len, int optional) {
    unsigned char *b = 0;
    char line[4096];
    int code;
#if defined(__UNISA__) && defined(_WIN32)
    code = cxsf_readfile(path, len, optional, 1, 0, line, (int)sizeof line,
                         io_open, io_open_reason, rf_open_say, rf_open_raw,
                         die, readstream, io_close, &b);
#else
    code = cxsf_readfile(path, len, optional, 0, ENOENT, line, (int)sizeof line,
                         io_open, io_open_reason, rf_open_say, rf_open_raw,
                         die, readstream, io_close, &b);
#endif
    if (code == 1 || code == 4) return 0;
    return b;
}
#else
int cxsf_close_fail(long, void (*)(const char *));
static unsigned char *readstream_with(long fd, const char *path, int *len, void *(*grow)(void *, size_t), unsigned char **own) {
    int n = 0, cap = 65536; FileStamp st; unsigned char *tmp;
    if (!own) own = &tmp;   /* *own holds the buffer from the first allocation on, so a die() can still free it */
    /* A size the stamp cannot represent (a directory under the lseek stamp) is not a
       presize: fall back to the doubling buffer so the read reports the real error. */
    if (io_stamp(fd, &st) && st.size > st.off && st.size - st.off <= INT32_MAX/2) cap = (int)(st.size - st.off) + 1;
    unsigned char *b = *own = grow(0, cap);
    for (;;) {
        if (n == cap) { if (cap > INT32_MAX/2) die("file too large"); cap *= 2; b = *own = grow(b, cap); }
        long r = io_read(fd, b+n, cap-n);
        if (r < 0) { fprintf(stderr, "run: cannot read %s\n", path); die("cannot read input"); }
        if (!r) break;
        if (r > cap-n) die("invalid read length");
        n += (int)r;
    }
    *len = n; return b;
}
static const char *io_open_reason(long fd) {
#ifdef _WIN32
    (void)fd; return 0;
#else
    if (fd == -ENOENT) return "ENOENT";
    if (fd == -ENOTDIR) return "ENOTDIR";
    if (fd == -EACCES) return "EACCES";
    return 0;
#endif
}
static unsigned char *readfile(const char *path, int *len, int optional) {
    long fd = io_open(path);
    if (fd < 0) {
#if defined(__UNISA__) && defined(_WIN32)
        if (optional) return 0;
#else
        if (optional && fd == -ENOENT) return 0;
#endif
        const char *reason = io_open_reason(fd);
        fprintf(stderr, "run: cannot open %s%s%s\n", path,
                reason ? ": " : "", reason ? reason : "");
        die("cannot open input");
    }
    unsigned char *b = readstream(fd,path,len);
    cxsf_close_fail(io_close(fd), die);
    return b;
}
#endif
static unsigned char *readstream(long fd, const char *path, int *len) { return readstream_with(fd, path, len, xrealloc, 0); }

/* Decimal model reader: no scanf dependency, and 64-bit arguments are
   checked before multiply/add (including the INT64_MIN magnitude). */
UNISA_RUNTIME_STATE unsigned char *LB; UNISA_RUNTIME_STATE int LP, LN, LBIN;
static void lskip(void) { if(!LBIN)while (LP < LN && LB[LP] <= 32) LP++; }
#ifdef UNISA_CX_MODEL_NUMBER
int cxmn_tag(const unsigned char *,long,long *,int,int *);
int cxmn_hex(int);
static int lchar(void) {
    long pos=LP; int value=0, rc=cxmn_tag(LB,LN,&pos,LBIN,&value); LP=(int)pos;
    if(rc==1)die("truncated binary model tag");
    if(rc==2)die("truncated model");
    if(rc==3)die("bad model tag");
    if(rc)die("invalid model tag input");
    return value;
}
#else
static int lchar(void) {
    if(LBIN){if(LP==LN)die("truncated binary model tag");return LB[LP++];}
    lskip(); if (LP == LN) die("truncated model");
    int c = LB[LP++]; if (LP < LN && LB[LP] > 32) die("bad model tag"); return c;
}
#endif
static void ltag(int c) { if (lchar() != c) die("bad model tag"); }
#ifdef UNISA_CX_MODEL_NUMBER
int cxmn_number(const unsigned char *,long,long *,int,long long *);
int cxmn_budget(const char *,long long *);
static I lnum(void) {
    long pos=LP; long long value=0;
    int rc=cxmn_number(LB,LN,&pos,LBIN,&value); LP=(int)pos;
    if(rc==1)die("truncated binary model number");
    if(rc==2)die("binary model number overflow");
    if(rc==3)die("noncanonical binary model number");
    if(rc==4)die("long binary model number");
    if(rc==5)die("bad model number");
    if(rc==6)die("model number overflow");
    if(rc==7)die("missing model number");
    if(rc)die("invalid model number input");
    return (I)value;
}
#else
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
#endif
static int lint(void) { I n = lnum(); if (n < INT32_MIN || n > INT32_MAX) die("model index overflow"); return (int)n; }

/* ---- the table ---- */
UNISA_RUNTIME_STATE int NS, NQ, NRG, NSTR, START;
UNISA_RUNTIME_STATE char **STR; UNISA_RUNTIME_STATE int *STRL;
UNISA_RUNTIME_STATE int *QOFF, *QLEN; UNISA_RUNTIME_STATE I *QA; UNISA_RUNTIME_STATE int NQA;          /* actions, flattened */
UNISA_RUNTIME_STATE int ISNET, TOPMAX;
UNISA_RUNTIME_STATE int *NLO, *NHI, *BN, *BQ, *RETS; UNISA_RUNTIME_STATE unsigned char **RETOK;
UNISA_RUNTIME_STATE int *SMODE; UNISA_RUNTIME_STATE int **ROWK, **ROWN, **ROWQ; UNISA_RUNTIME_STATE int *ROWC;

static int hexv(int c) {
#ifdef UNISA_CX_MODEL_NUMBER
    int value=cxmn_hex(c); if(value>=0)return value;
#else
    if (c >= '0' && c <= '9') return c-'0'; if (c >= 'a' && c <= 'f') return c-'a'+10;
#endif
    die("bad hex string"); return 0;
}

#ifdef UNISA_CX_MODEL_DECODE
static void loadbytes(unsigned char *data, int len);
#else
static void loadbytes(unsigned char *data, int len) {
    LB = data; LN = len; LBIN=len>=8 && !memcmp(data,"UNINETB1",8); LP=LBIN?8:0;
    int kind = lchar(); NS=lint(); NQ=lint(); NRG=lint(); NSTR=lint(); START=lint();
    if ((kind != 'T' && kind != 'N') || NS <= 0 || NQ <= 0 || NRG < 0 || NSTR < 0 || START < 0 || START >= NS) die("bad header");
    if (NS > len || NQ > len || NSTR > len || NSTR == INT32_MAX) die("bad model count extent");
    ISNET = kind == 'N'; TOPMAX = NS - 1;
    if (ISNET) { TOPMAX=lint(); if (TOPMAX < NS-1 || TOPMAX == INT32_MAX) die("bad network domain"); }
    STR = mrealloc(0, sizeof *STR * (NSTR + 1));
    memset(STR, 0, sizeof *STR * (NSTR + 1));
    STRL = mrealloc(0, sizeof *STRL * (NSTR + 1));
    for (int k = 0; k < NSTR; k++) {
        ltag('S');
        if (LBIN) {
            int n = lint(); if (n < 0 || n > LN-LP || n == INT32_MAX) die("bad binary model string");
            STR[k]=mrealloc(0,n+1); STRL[k]=n; memcpy(STR[k],LB+LP,n);
            STR[k][n]=0; LP+=n; continue;
        }
        lskip(); int begin = LP;
        while (LP < LN && LB[LP] > 32) LP++;
        int size = LP-begin; unsigned char *buf = LB+begin;
        int n = size == 1 && buf[0] == '-' ? 0 : size/2;
        if (size == 0 || ((size & 1) && !(size == 1 && buf[0] == '-'))) die("bad string");
        STR[k] = mrealloc(0, n + 1); STRL[k] = n;
        for (int j = 0; j < n; j++) STR[k][j] = (char)(hexv(buf[2 * j]) * 16 + hexv(buf[2 * j + 1]));
        STR[k][n] = 0;
    }
    QOFF = mrealloc(0, sizeof(int) * NQ); QLEN = mrealloc(0, sizeof(int) * NQ);
    int cap = 1 << 16; QA = mrealloc(0, sizeof(I) * cap); NQA = 0;
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
            cap *= 2; QA = mrealloc(QA, sizeof(I) * cap);
        }
        for (int i = 0; i < copied; i++) QA[NQA++] = QA[from+i];
        for (int a = 0; a < n; a++) {
            int op = lint(); if (op < 0 || op >= NOP_) die("bad opcode");
            if (NQA > INT32_MAX-6) die("sequence extent overflow");
            if (NQA+6 > cap) {
                if (cap > INT32_MAX/2) die("sequence capacity exceeded");
                cap *= 2; QA = mrealloc(QA, sizeof(I) * cap);
            }
            QA[NQA++] = op;
            for (int j = 0; j < ARITY[op]; j++) QA[NQA++] = lnum();
        }
    }
    SMODE = mrealloc(0, sizeof(int) * NS); ROWC = mrealloc(0, sizeof(int) * NS);
    ROWK = mrealloc(0, sizeof(int *) * NS); memset(ROWK, 0, sizeof(int *) * NS);
    ROWN = mrealloc(0, sizeof(int *) * NS); memset(ROWN, 0, sizeof(int *) * NS);
    ROWQ = mrealloc(0, sizeof(int *) * NS); memset(ROWQ, 0, sizeof(int *) * NS);
    NLO = mrealloc(0, sizeof(int) * NS); NHI = mrealloc(0, sizeof(int) * NS);
    BN = mrealloc(0, sizeof(int) * NS); BQ = mrealloc(0, sizeof(int) * NS);
    RETS = mrealloc(0, sizeof(int) * NS); RETOK = mrealloc(0, sizeof(unsigned char *) * NS);
    for (int s = 0; s < NS; s++) { RETS[s] = -1; RETOK[s] = 0; }
    for (int s = 0; s < NS; s++) {
        if (ISNET) {
            ltag('H'); int m=lint(), lo=lint(), hi=lint(), n=lint(), bn=lint(), bq=lint();
            if (m == 3) {   /* a stack bank with a declared return set */
                int rs = lint(), rn = lint(), prev = -1;
                if (rs < 0 || rs >= NQ || rn < 1 || rn > NS) die("bad return declaration");
                RETS[s] = rs; RETOK[s] = mrealloc(0, NS); memset(RETOK[s], 0, NS);
                for (int j = 0; j < rn; j++) {
                    int k = lint(); if (k <= prev || k >= NS) die("bad return declaration");
                    RETOK[s][k] = 1; prev = k;
                }
                m = 1;
            }
            if (m < 0 || m > 2 ||
                lo != (m == 1 ? -1 : 0) || hi != (m == 1 ? TOPMAX : 256) || n < 0 || (I)n > (I)hi - lo) die("bad network bank");
            SMODE[s] = m; NLO[s] = lo; NHI[s] = hi; ROWC[s] = n; BN[s] = bn; BQ[s] = bq;
            ROWK[s] = mrealloc(0, sizeof(int) * n); ROWN[s] = mrealloc(0, sizeof(int) * n); ROWQ[s] = mrealloc(0, sizeof(int) * n);
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
            ROWK[s] = mrealloc(0, sizeof(int) * n); ROWN[s] = mrealloc(0, sizeof(int) * n); ROWQ[s] = mrealloc(0, sizeof(int) * n);
            for (int j = 0; j < n; j++) { ROWK[s][j]=lint(); ROWN[s][j]=lint(); ROWQ[s][j]=lint(); }
        } else {                                        /* keyed 0..256: direct */
            ROWN[s] = mrealloc(0, sizeof(int) * 257); ROWQ[s] = mrealloc(0, sizeof(int) * 257); ROWK[s] = 0;
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
#endif

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
/* A parsed package: the file bytes plus the directory tables the globals
   above point into. It is owned by a cache, built once per package stamp. */
typedef struct UnisaPackage {
    char *path; FileStamp stamp; int complete;
    unsigned char *file, *b; int n, m, s, ver;
    int *off, *len, *raw; uint32_t *crc; Stage *stages; Resource *res; int nr;
} Package;
/* Decoded model tables of one stage. The core reads them through a const
   CoreModel and never writes them, so a decoded model is reusable as is. */
typedef struct UnisaModel {
    int loaded;
    int ns, nq, nrg, nstr, start, isnet, topmax, nqa;
    char **str; int *strl; int *qoff, *qlen; I *qa;
    int *smode, *rowc, **rowk, **rown, **rowq; int *nlo, *nhi, *bn, *bq, *rets; unsigned char **retok;
} Model;
typedef struct UnisaCache { Package package; Model *models; int nmodels; } Cache;
UNISA_RUNTIME_STATE Cache *CACHE;          /* set by the library for the current call; the command line keeps it null */
UNISA_RUNTIME_STATE int MODEL_LOADING;     /* a decode is in progress: the globals own fresh tables not yet captured */
/* Borrowed process inputs override carried resources by exact byte key.
   The caller keeps these immutable spans alive for the whole route. */
typedef struct { const unsigned char *name; int n; const unsigned char *data; int len; } ResourceInput;
UNISA_RUNTIME_STATE ResourceInput *RI; UNISA_RUNTIME_STATE int NRI;
static char *pword_owned(void) {
    lskip(); int first = LP;
    while (LP < LN && LB[LP] > 32) {
        int c = LB[LP++];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.' || c == '/')) die("bad package name");
    }
    int n = LP-first; if (!n) die("missing package name");
    char *v = mrealloc(0, n+1); memcpy(v, LB+first, n); v[n] = 0; return v;
}
/* Safe on a partially parsed package: every table is null until allocated,
   and stage names are zeroed before they are filled. */
#ifdef UNISA_CX_PACKAGE_LIFETIME
void cxpl_release(Package *, void (*)(void *));
int cxpl_stamp_match(const long long *, const long long *, int, unsigned, unsigned, int, int);
int cxpl_reuse(int, const char *, const char *, int);
int cxpl_settle(int);
int cxpl_slot(const void *, int, int);
static void package_release(Package *p) { cxpl_release(p,mfree); }
#else
static void package_release(Package *p) {
    if (p->stages) for (int i = 0; i < p->s; i++) {
        mfree(p->stages[i].route); mfree(p->stages[i].name); mfree(p->stages[i].in); mfree(p->stages[i].out);
    }
    mfree(p->stages); mfree(p->off); mfree(p->len); mfree(p->raw); mfree(p->crc); mfree(p->res); mfree(p->file); mfree(p->path);
    memset(p, 0, sizeof *p);
}
#endif
static void package_bind(const Package *p) {
    PFILE = p->file; PB = p->b; PN = p->n; PM = p->m; PS = p->s; PVER = p->ver;
    POFF = p->off; PLEN = p->len; PRAW = p->raw; PCRC = p->crc; STAGES = p->stages; RES = p->res; NR = p->nr;
    if (PVER == 3) crc_init();
}
static void package_unbind(void) {
    PFILE = PB = 0; PN = PM = PS = PVER = 0; POFF = PLEN = PRAW = 0; PCRC = 0; STAGES = 0; RES = 0; NR = 0;
}
#ifdef UNISA_CX_PACKAGE_DECODE
typedef struct {
    const unsigned char *bytes; long length, at;
    void *(*allocate)(void *,size_t); void (*release)(void *);
    Package package; const char *why;
} CxPackageDecode;
int cxpk_decode(CxPackageDecode *,const unsigned char *,long);
static void *package_decode_alloc(void *p,size_t n) { return UNISA_PACKAGE_REALLOC(p,n); }
#endif
/* Parse the open package file into p (p must be released/zeroed). The fd is
   consumed and closed. Every format edge is checked before any route runs. */
static void package_parse(Package *p, long fd, const char *path) {
    p->file = 0; readstream_with(fd, path, &p->n, mrealloc, &p->file); p->b = p->file;   /* p->file owns the buffer if the read dies */
#ifdef UNISA_CX_SOURCE_FRAME
    if (cxsf_close_fail(io_close(fd), die)) return;
#else
    if (io_close(fd) < 0) die("close failed");
#endif
    if (p->n < 2 || p->b[0] != 'P' || p->b[1] != ' ') {
        PackageFooter footer;
        /* Signature records follow the package footer in Authenticode APEs.
           Locate only within the validated file/certificate extents. This is
           format handling, not a signature trust decision. */
        if (!packagefooter_locate_bytes(p->file, p->n, &footer)) die("bad or missing package footer");
        p->b += (int)footer.package_offset; p->n = (int)footer.package_length;
    }
#ifdef UNISA_CX_PACKAGE_DECODE
    CxPackageDecode d={0};d.allocate=package_decode_alloc;d.release=mfree;
    if(cxpk_decode(&d,p->b,p->n))die(d.why?d.why:"invalid package input");
    unsigned char *file=p->file;char *saved_path=p->path;FileStamp stamp=p->stamp;
    *p=d.package;p->file=file;p->path=saved_path;p->stamp=stamp;
    if(p->ver==3)crc_init();
#else
    LBIN = 0; LB = p->b; LN = p->n; LP = 0;
    ltag('P'); int version = lint();
    if (version != 1 && version != 2 && version != 3) die("unknown package version");
    p->ver = version; if (version == 3) crc_init();
    int pm = lint(), ps = lint(), nr = version >= 2 ? lint() : 0;
    if (nr < 0 || nr > p->n/5) die("bad resource count");
    p->res = mrealloc(0, sizeof(Resource)*nr); p->nr = nr;
    if (pm <= 0 || ps <= 0 || pm > p->n/8 || ps > p->n/10) die("bad package count");
    p->off = mrealloc(0, sizeof(int)*pm); p->len = mrealloc(0, sizeof(int)*pm);
    p->raw = mrealloc(0, sizeof(int)*pm); p->crc = mrealloc(0, sizeof(uint32_t)*pm); p->m = pm;
    p->stages = mrealloc(0, sizeof(Stage)*ps); memset(p->stages, 0, sizeof(Stage)*ps); p->s = ps;
    Stage *stages = p->stages;
    for (int i = 0; i < ps; i++) {
        ltag('D'); Stage *s = &stages[i];
        s->route = pword_owned(); s->name = pword_owned(); s->in = pword_owned(); s->out = pword_owned(); s->model = lint();
        if (s->model < 0 || s->model >= pm) die("bad package model index");
        int previous = -1;
        for (int j = 0; j < i; j++) if (!strcmp(s->route, stages[j].route)) {
            if (!strcmp(s->name, stages[j].name)) die("duplicate package stage");
            previous = j;
        }
        if (previous >= 0 && strcmp(stages[previous].out, s->in)) die("package format mismatch");
    }
    for (int i = 0; i < pm; i++) {
        ltag('M'); int n = lint(); p->raw[i]=n; p->crc[i]=0;
        if (version == 3) {
            p->raw[i]=lint(); int codec=lint(); I checksum=lnum();
            if (p->raw[i]<=0 || codec!=1 || checksum<0 || checksum>(I)UINT32_MAX) die("bad compressed model header");
            p->crc[i]=(uint32_t)checksum;
        }
        if (LP >= LN || LB[LP++] != 10 || n <= 0 || n > LN-LP) die("bad package model extent");
        if (version < 3 && LB[LP] != 'N') die("package requires networks");
        p->off[i] = LP; p->len[i] = n; LP += n;
    }
    for (int i = 0; i < nr; i++) {
        ltag('F'); int n = lint(), len = lint();
        if (LP >= LN || LB[LP++] != 10 || n <= 0 || len < 0 || n > LN-LP || len > LN-LP-n) die("bad resource extent");
        p->res[i].name = LP; p->res[i].n = n; p->res[i].data = LP+n; p->res[i].len = len;
        for (int j = 0; j < i; j++)
            if (p->res[j].n == n && !memcmp(p->b+p->res[j].name, p->b+LP, n)) die("duplicate resource name");
        LP += n+len;
    }
    if (LP != LN) die("trailing package data");
    p->complete = 1;
#endif
}
#ifdef UNISA_CX_SOURCE_FRAME
int cxsf_package_gate(long fd);
static long package_open(const char *path) {
    long fd = io_open(path);
    if (cxsf_package_gate(fd)) {
        char line[4096];
        if (!cxsf_open_line(line, (int)sizeof line, path, 0)) fputs(line, stderr);
        else fprintf(stderr, "run: cannot open %s\n", path);
        die("cannot open input");
    }
    return fd;
}
#else
static long package_open(const char *path) {
    long fd = io_open(path);
    if (fd < 0) { fprintf(stderr, "run: cannot open %s\n", path); die("cannot open input"); }
    return fd;
}
#endif
#ifndef UNISA_PACKAGE_CACHE
/* Command-line package: parsed on every call, released by unpackage(). */
UNISA_RUNTIME_STATE Package CLIPKG;
static void package(const char *path) {
    package_release(&CLIPKG);
    package_parse(&CLIPKG, package_open(path), path);
    package_bind(&CLIPKG);
}
static void unpackage(void) {
    package_release(&CLIPKG); package_unbind();
}
#endif

#ifdef UNISA_CX_MODEL_LIFETIME
int mv_clear(int **sc, int nsc, void ***slot, int nslot);
int mv_capture(void *model, int **sc, int nsc, void ***slot, int nslot);
int mv_bind(void *model, int **sc, int nsc, void ***slot, int nslot);
static void mv_addrs(int **sc, void ***slot) {
    sc[0] = &NS; sc[1] = &NQ; sc[2] = &NRG; sc[3] = &NSTR; sc[4] = &START; sc[5] = &ISNET; sc[6] = &TOPMAX; sc[7] = &NQA;
    slot[0] = (void **)&STR; slot[1] = (void **)&STRL; slot[2] = (void **)&QOFF; slot[3] = (void **)&QLEN; slot[4] = (void **)&QA;
    slot[5] = (void **)&SMODE; slot[6] = (void **)&ROWC; slot[7] = (void **)&ROWK; slot[8] = (void **)&ROWN; slot[9] = (void **)&ROWQ;
    slot[10] = (void **)&NLO; slot[11] = (void **)&NHI; slot[12] = (void **)&BN; slot[13] = (void **)&BQ; slot[14] = (void **)&RETS; slot[15] = (void **)&RETOK;
}
static void model_clear(void) {
    int *sc[8]; void **slot[16];
    mv_addrs(sc, slot);
    if (mv_clear(sc, 8, slot, 16)) die("model view");
}
static void model_capture(Model *m) {
    int *sc[8]; void **slot[16];
    mv_addrs(sc, slot);
    if (mv_capture(m, sc, 8, slot, 16)) die("model view");
}
static void model_bind(const Model *m) {
    int *sc[8]; void **slot[16];
    mv_addrs(sc, slot);
    if (mv_bind((void *)m, sc, 8, slot, 16)) die("model view");
}
#else
static void model_clear(void) {
    NS = NQ = NRG = NSTR = START = ISNET = TOPMAX = NQA = 0;
    STR = 0; STRL = QOFF = QLEN = 0; QA = 0;
    SMODE = ROWC = 0; ROWK = ROWN = ROWQ = 0; NLO = NHI = BN = BQ = RETS = 0; RETOK = 0;
}
static void model_capture(Model *m) {
    m->ns = NS; m->nq = NQ; m->nrg = NRG; m->nstr = NSTR; m->start = START; m->isnet = ISNET; m->topmax = TOPMAX; m->nqa = NQA;
    m->str = STR; m->strl = STRL; m->qoff = QOFF; m->qlen = QLEN; m->qa = QA;
    m->smode = SMODE; m->rowc = ROWC; m->rowk = ROWK; m->rown = ROWN; m->rowq = ROWQ;
    m->nlo = NLO; m->nhi = NHI; m->bn = BN; m->bq = BQ; m->rets = RETS; m->retok = RETOK;
}
static void model_bind(const Model *m) {
    NS = m->ns; NQ = m->nq; NRG = m->nrg; NSTR = m->nstr; START = m->start; ISNET = m->isnet; TOPMAX = m->topmax; NQA = m->nqa;
    STR = m->str; STRL = m->strl; QOFF = m->qoff; QLEN = m->qlen; QA = m->qa;
    SMODE = m->smode; ROWC = m->rowc; ROWK = m->rowk; ROWN = m->rown; ROWQ = m->rowq;
    NLO = m->nlo; NHI = m->nhi; BN = m->bn; BQ = m->bq; RETS = m->rets; RETOK = m->retok;
}
#endif
#ifdef UNISA_CX_MODEL_DECODE
/* The CX decoder owns partial results locally and releases them before an
 * error reaches the runtime's longjmp/exit boundary. A successful model is
 * moved into the existing runtime view for the current stage. */
typedef struct CxDecode { Model model; void *(*allocate)(void *,size_t); void (*release)(void *); const char *why; } CxDecode;
int cxmd_decode(CxDecode *,const unsigned char *,long);
void cxmd_take(CxDecode *,Model *);
static void *model_decode_alloc(void *p,size_t n) { return UNISA_PACKAGE_REALLOC(p,n); }
#ifdef UNISA_CX_MODELLOAD
int ml_load(void *dec, void *model, const unsigned char *data, long len,
            int (*decode)(void *, const unsigned char *, long),
            const char *(*whyof)(void *),
            void (*take)(void *, void *),
            void (*bindfn)(void *),
            void (*fail)(const char *));
static int load_decode(void *p, const unsigned char *data, long n) { return cxmd_decode((CxDecode *)p, data, n); }
static const char *load_why(void *p) { return ((CxDecode *)p)->why; }
static void load_take(void *p, void *m) { cxmd_take((CxDecode *)p, (Model *)m); }
static void load_bind(void *p) { model_bind((const Model *)p); }
static void load_fail(const char *s) { die(s); }
#endif
static void loadbytes(unsigned char *data, int len) {
    CxDecode d={0}; Model model={0};
    d.allocate=model_decode_alloc; d.release=mfree;
#ifdef UNISA_CX_MODELLOAD
    ml_load(&d, &model, data, len, load_decode, load_why, load_take, load_bind, load_fail);
#else
    if(cxmd_decode(&d,data,len)) die(d.why ? d.why : "invalid model input");
    cxmd_take(&d,&model); model_bind(&model);
#endif
}
#endif
/* Safe on a partially decoded model (see loadbytes: arrays of pointers are zeroed when allocated). */
#ifdef UNISA_CX_MODEL_LIFETIME
void cxml_release(Model *, void (*)(void *));
static void model_release(Model *m) { cxml_release(m,mfree); }
#else
static void model_release(Model *m) {
    if (m->str) for (int i = 0; i < m->nstr; i++) mfree(m->str[i]);
    if (m->rowk) for (int i = 0; i < m->ns; i++) mfree(m->rowk[i]);
    if (m->rown) for (int i = 0; i < m->ns; i++) mfree(m->rown[i]);
    if (m->rowq) for (int i = 0; i < m->ns; i++) mfree(m->rowq[i]);
    if (m->retok) for (int i = 0; i < m->ns; i++) mfree(m->retok[i]);
    mfree(m->str); mfree(m->strl); mfree(m->qoff); mfree(m->qlen); mfree(m->qa);
    mfree(m->smode); mfree(m->rowc); mfree(m->rowk); mfree(m->rown); mfree(m->rowq);
    mfree(m->nlo); mfree(m->nhi); mfree(m->bn); mfree(m->bq); mfree(m->rets); mfree(m->retok);
    memset(m, 0, sizeof *m);
}
#endif
/* Model lifetime is one stage. No model-specific state survives unload. */
static void unload(void) {
    Model m; model_capture(&m); model_release(&m); model_clear();
}

#ifdef UNISA_PACKAGE_CACHE
/* Context cache (library only): the package is parsed once per path+stamp
   and its decoded models are kept by model index until the package changes
   or the cache is released. On a longjmp out of a decode, cache_settle()
   frees the tables the globals still own; a package that failed to parse is
   never reused. */
static int stamp_equal(const FileStamp *a, const FileStamp *b) {
#ifdef UNISA_CX_PACKAGE_LIFETIME
    long long ax[8], bx[8];
    ax[0] = a->size; ax[1] = a->off; ax[2] = a->mtime; ax[3] = a->mtime_ns;
    ax[4] = a->ctime; ax[5] = a->ctime_ns; ax[6] = a->ino; ax[7] = a->dev;
    bx[0] = b->size; bx[1] = b->off; bx[2] = b->mtime; bx[3] = b->mtime_ns;
    bx[4] = b->ctime; bx[5] = b->ctime_ns; bx[6] = b->ino; bx[7] = b->dev;
    return cxpl_stamp_match(ax, bx, 8, a->tag, b->tag, a->valid, b->valid);
#else
    return a->valid && b->valid && a->size == b->size && a->off == b->off && a->tag == b->tag &&
           a->mtime == b->mtime && a->mtime_ns == b->mtime_ns && a->ctime == b->ctime && a->ctime_ns == b->ctime_ns &&
           a->ino == b->ino && a->dev == b->dev;
#endif
}
static void cache_release(Cache *k) {
    if (k->models) for (int i = 0; i < k->nmodels; i++) model_release(&k->models[i]);
    mfree(k->models); k->models = 0; k->nmodels = 0;
    package_release(&k->package);
}
static void cache_settle(void) {
#ifdef UNISA_CX_PACKAGE_LIFETIME
    if (cxpl_settle(MODEL_LOADING)) { Model m; model_capture(&m); model_release(&m); MODEL_LOADING = 0; }
#else
    if (MODEL_LOADING) { Model m; model_capture(&m); model_release(&m); MODEL_LOADING = 0; }
#endif
    model_clear(); package_unbind(); CACHE = 0;
}
static void cache_package(Cache *k, const char *path) {
    Package *p = &k->package;
    long fd = package_open(path);
    FileStamp stamp; io_stamp(fd, &stamp);
#ifdef UNISA_CX_PACKAGE_LIFETIME
    if (cxpl_reuse(p->complete, path, p->path, p->complete && p->path && stamp_equal(&stamp, &p->stamp))) {
#else
    if (p->complete && p->path && !strcmp(p->path, path) && stamp_equal(&stamp, &p->stamp)) {
#endif
#ifdef UNISA_CX_SOURCE_FRAME
        if (cxsf_close_fail(io_close(fd), die)) return;
#else
        if (io_close(fd) < 0) die("close failed");
#endif
    } else {
        cache_release(k);
        size_t n = strlen(path) + 1; p->path = mrealloc(0, n); memcpy(p->path, path, n);
        p->stamp = stamp;
        package_parse(p, fd, path);
        k->models = mrealloc(0, sizeof(Model) * p->m); memset(k->models, 0, sizeof(Model) * p->m); k->nmodels = p->m;
    }
    package_bind(p); CACHE = k;
}
#endif

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

#include "resourceledger.h"
/* Exact resource lookup and filesystem naming belong to the host adapter. */
void core_host_panic(const char *reason) { die(reason); }
UNISA_RUNTIME_STATE const char *INCDIR=0;
/* Optional host IO ledger. It records successful disk reads, not include
   syntax or carried/process resources. Cached requests need only one make
   prerequisite; no language decision is made here. */
UNISA_RUNTIME_STATE int FILE_READ_RECORD=0, FILE_READ_COUNT=0;
UNISA_RUNTIME_STATE char **FILE_READ_PATHS=0;
UNISA_RUNTIME_STATE int FILE_ABSENT_COUNT=0;
UNISA_RUNTIME_STATE char **FILE_ABSENT_PATHS=0;
#ifdef UNISA_CX_SOURCE_FRAME
int cxsf_file_keep(int, char **, int, char **, int, const char *,
                   void *(*)(void *, size_t), char ***, int *);
#endif
static void record_file_read(const char *path) {
#ifdef UNISA_CX_SOURCE_FRAME
    char **list = FILE_READ_PATHS;
    int n = FILE_READ_COUNT;
    cxsf_file_keep(FILE_READ_RECORD, FILE_READ_PATHS, FILE_READ_COUNT,
                   0, 0, path, xrealloc, &list, &n);
    FILE_READ_PATHS = list;
    FILE_READ_COUNT = n;
#else
    if (!FILE_READ_RECORD) return;
    for (int i=0;i<FILE_READ_COUNT;i++) if (!strcmp(FILE_READ_PATHS[i],path)) return;
    FILE_READ_PATHS=xrealloc(FILE_READ_PATHS,(FILE_READ_COUNT+1)*sizeof(char *));
    int n=strlen(path)+1;
    FILE_READ_PATHS[FILE_READ_COUNT]=xrealloc(0,n);
    memcpy(FILE_READ_PATHS[FILE_READ_COUNT++],path,n);
#endif
}
/* When recording for E3 reuse, remember optional disk probes that were absent
   so a later-created path invalidates the cache.  readfile(optional) returns
   NULL only for ENOENT on reuse-enabled Linux. */
static void record_file_absent(const char *path) {
#ifdef UNISA_CX_SOURCE_FRAME
    char **list = FILE_ABSENT_PATHS;
    int n = FILE_ABSENT_COUNT;
    cxsf_file_keep(FILE_READ_RECORD, FILE_ABSENT_PATHS, FILE_ABSENT_COUNT,
                   FILE_READ_PATHS, FILE_READ_COUNT, path, xrealloc, &list, &n);
    FILE_ABSENT_PATHS = list;
    FILE_ABSENT_COUNT = n;
#else
    if (!FILE_READ_RECORD) return;
    for (int i=0;i<FILE_ABSENT_COUNT;i++) if (!strcmp(FILE_ABSENT_PATHS[i],path)) return;
    for (int i=0;i<FILE_READ_COUNT;i++) if (!strcmp(FILE_READ_PATHS[i],path)) return;
    FILE_ABSENT_PATHS=xrealloc(FILE_ABSENT_PATHS,(FILE_ABSENT_COUNT+1)*sizeof(char *));
    int n=strlen(path)+1;
    FILE_ABSENT_PATHS[FILE_ABSENT_COUNT]=xrealloc(0,n);
    memcpy(FILE_ABSENT_PATHS[FILE_ABSENT_COUNT++],path,n);
#endif
}

/* S step 1 strict resource flow (opt-in: --strict-resource-flow --resource-ledger DIR).  The ledger is the only source;
   RI, package resources and disk are not consulted.  A key the ledger does not list is MISSING: a named interface failure. */
UNISA_RUNTIME_STATE int RF_STRICT=0, RF_LOADED=0;
UNISA_RUNTIME_STATE RLLedger RF_LEDGER;
UNISA_RUNTIME_STATE char RF_LEDGER_DIR[4096];
static int rf_default_fetch(const unsigned char *p,int n,unsigned char **bytes,int *len);
UNISA_RUNTIME_STATE int RF_TRACE_STARTED;
static int rf_trace_finish(void) {
    if (RF_STRICT && RF_LEDGER.trace) {
        if (!RF_TRACE_STARTED) { fprintf(stderr,"unisacc: INTERFACE_ERROR resource-trace-not-started\n");return 2; }
        if (RF_LEDGER.cursor != RF_LEDGER.n) { fprintf(stderr,"unisacc: INTERFACE_ERROR resource-trace-unconsumed event=%d remaining=%d\n",RF_LEDGER.cursor,RF_LEDGER.n-RF_LEDGER.cursor);return 2; }
    }
    return 0;
}
UNISA_RUNTIME_STATE const char *rf_capture_set = 0;
static const char *rf_capture_dir(void) {
    UNISA_RUNTIME_STATE int init=0;UNISA_RUNTIME_STATE const char *d=0;
    if (rf_capture_set && *rf_capture_set) return rf_capture_set;
    if (!init) {init=1;d=getenv("UNISACC_RF_CAPTURE");
#ifdef UNISA_CX_FILEMODE
        d=bx_env_blank(d);
#else
        if(d&&!*d)d=0;
#endif
    }return d;
}
/* SC2 is an ordered host-fetch trace. Repeated keys may legitimately answer
 * differently in another execution/unit. No answer or test verdict is cached. */
UNISA_RUNTIME_STATE int RF_CAPC,RF_CAPACTIVE;
UNISA_RUNTIME_STATE long RF_CAPOWNER;
UNISA_RUNTIME_STATE char RF_CAPDIR[4096];
enum { RF_CAP_MAX = 4096 };
#ifdef UNISA_CX_SOURCE_FRAME
int cxsf_cap_full(int, int);
int cxsf_cap_join(char *, int, const char *, const char *);
int cxsf_cap_stage(char *, int, const char *, long);
int cxsf_cap_line(char *, int, int, int, const char *);
int cxsf_cap_reg(int, int, int, long long);
int cxsf_cap_take(long, int, unsigned long long, unsigned long long, unsigned long long *);
int cxsf_cap_session(int, int, int, int);
int cxsf_cap_key(int, int);
int cxsf_cap_answer(int, int, int);
int cxsf_cap_hex(char *, int, const unsigned char *, int);
int cxsf_cap_name(char *, int, int);
int cxsf_cap_row(char *, int, int, const char *, const char *, int, const char *);
int cxsf_cap_args(int, int, int, int);
int cxsf_cap_ledger(int, int, int, long long, int *);
int cxsf_cap_dent(const char *, int);
int cxsf_cap_place(int);
int cxsf_cap_idline(char *, int, const char *, const char *);
int cxsf_cap_dirline(char *, int, int);
int cxsf_e3_args(int, int, int, int);
int cxsf_e3_errno(int, int);
int cxsf_e3_span(int, unsigned long long, unsigned long long);
int cxsf_e3_line(char *, int, int, int);
int cxsf_e3_ready(int, int, int, int, int);
int cxsf_e3_row(int, int, const unsigned char *, const unsigned char *, int, int, const char *);
int cxsf_e3_tail(const unsigned char *, int, const unsigned char *, int);
int cxsf_e3_match(const unsigned char *, int, const unsigned char *, int);
int cxsf_e3_same(const unsigned char *, int, const unsigned char *, int);
int cxsf_e3_id(const char *, const char *, const char *, const char *);
int cxsf_e3_cmp_line(char *, int, int, const char *, const char *);
int cxsf_route_gate(int, const char *);
int cxsf_route_e3(int, const char *);
int cxsf_route_line(char *, int, int, const char *);
int cxsf_stage_join(char *, int, const char *, const char *);
int cxsf_stage_err(char *, int, int, const char *);
int cxsf_snap_count(int, int);
int cxsf_snap_line(char *, int);
int cxsf_snap_slot(const unsigned char *, int, const unsigned char *, int, const unsigned char **, int *, const unsigned char **, int *);
int cxsf_fetch_miss(char *, int, int, const char *, int);
int cxsf_fetch_mismatch(char *, int, const char *, int);
int cxsf_fetch_hand(int, unsigned char *, int, unsigned char **, int *);
int cxsf_fetch_row(const unsigned char *, int, const unsigned char *, int, const unsigned char *, int, unsigned char **, int *);
int cxsf_fetch_path(char *, int, const unsigned char *, int, const char *);
int cxsf_fetch_file(const void *, const char *, void (*)(const char *), void (*)(const char *));
#endif
/* Same capacity contract for front-end rows and the e3-identity tip row. */
static int rf_cap_full(void) {
#ifdef UNISA_CX_SOURCE_FRAME
    return cxsf_cap_full(RF_CAPC, RF_CAP_MAX);
#else
    return RF_CAPC >= RF_CAP_MAX;
#endif
}
static int rf_cap_path(char *p, size_t cap, const char *dir, const char *name, char *err, size_t errn) {
#ifdef UNISA_CX_SOURCE_FRAME
    if (!cxsf_cap_join(p, cap > 2147483647u ? 2147483647 : (int)cap, dir, name)) return 0;
    if (cxsf_cap_line(err, errn > 2147483647u ? 2147483647 : (int)errn, 0, 0, 0))
        snprintf(err, errn, "INTERFACE_ERROR capture-path-too-long");
    return 1;
#else
    int z = snprintf(p, cap, "%s/%s", dir, name);
    if (z < 0 || (size_t)z >= cap) { snprintf(err, errn, "INTERFACE_ERROR capture-path-too-long"); return 1; }
    return 0;
#endif
}
#ifndef _WIN32
/* Existing files are never opened for truncation. A private mkdir reserves the
 * staging name and link publishes each new file without replacing a target. */
static int rf_cap_new_file(const char *dir, const char *name, const unsigned char *bytes, size_t n, char *err, size_t errn) {
    char tmp[4096], from[4096], to[4096]; FILE *f; int bad = 0, saved = 0;
#ifdef UNISA_CX_SOURCE_FRAME
    if (cxsf_cap_stage(tmp, (int)sizeof tmp, dir, (long)getpid())
        || cxsf_cap_join(from, (int)sizeof from, tmp, "bytes")
        || cxsf_cap_join(to, (int)sizeof to, dir, name)) {
        if (cxsf_cap_line(err, (int)errn, 0, 0, 0))
            snprintf(err, errn, "INTERFACE_ERROR capture-path-too-long");
        return 1;
    }
#else
    int z = snprintf(tmp, sizeof tmp, "%s/.capture-%ld", dir, (long)getpid());
    if (z < 0 || (size_t)z >= sizeof tmp || rf_cap_path(from,sizeof from,tmp,"bytes",err,errn) || rf_cap_path(to,sizeof to,dir,name,err,errn)) {
        snprintf(err,errn,"INTERFACE_ERROR capture-path-too-long"); return 1;
    }
#endif
    if (mkdir(tmp,0700)) {
#ifdef UNISA_CX_SOURCE_FRAME
        if (cxsf_cap_line(err, (int)errn, 1, 0, 0))
#endif
        snprintf(err,errn,"INTERFACE_ERROR capture-staging-create"); return 1; }
#ifdef UNISA_CX_FILEMODE
    f = fopen(from,bx_file_mode(1));
#else
    f = fopen(from,"wb");
#endif
    if (!f) bad = 1;
    else {
        if (n && fwrite(bytes,1,n,f) != n) bad = 1;
        if (fflush(f)) bad = 1;
        if (fclose(f)) bad = 1;
    }
    if (bad) {
#ifdef UNISA_CX_SOURCE_FRAME
        if (cxsf_cap_line(err, (int)errn, 2, 0, name))
#endif
        snprintf(err,errn,"INTERFACE_ERROR capture-file-write %s",name);
    } else if (rl_snap_link(from,to)) { saved = errno; bad = 1;
#ifdef UNISA_CX_SOURCE_FRAME
        if (cxsf_cap_line(err, (int)errn, 3, saved == EEXIST, name))
#endif
        snprintf(err,errn,saved == EEXIST ? "INTERFACE_ERROR capture-target-exists %s" : "INTERFACE_ERROR capture-publish-failed %s",name); }
    if (unlink(from) && !bad) { bad = 1;
#ifdef UNISA_CX_SOURCE_FRAME
        if (cxsf_cap_line(err, (int)errn, 4, 0, 0))
#endif
        snprintf(err,errn,"INTERFACE_ERROR capture-staging-cleanup"); }
    if (rmdir(tmp) && !bad) { bad = 1;
#ifdef UNISA_CX_SOURCE_FRAME
        if (cxsf_cap_line(err, (int)errn, 4, 0, 0))
#endif
        snprintf(err,errn,"INTERFACE_ERROR capture-staging-cleanup"); }
    return bad;
}
static int rf_cap_append(const char *path, const unsigned char *bytes, size_t n, int require_empty, char *err, size_t errn) {
    struct stat st; size_t at = 0;
    int fd = open(path,O_WRONLY|O_APPEND|O_NONBLOCK|O_NOFOLLOW|O_CLOEXEC);
    if (fd < 0) {
#ifdef UNISA_CX_SOURCE_FRAME
        if (cxsf_cap_line(err, (int)errn, 5, 0, 0))
#endif
        snprintf(err,errn,"INTERFACE_ERROR capture-ledger-open"); return 1; }
#ifdef UNISA_CX_SOURCE_FRAME
    {
        int stat_ok = !fstat(fd,&st);
        int is_reg = stat_ok && S_ISREG(st.st_mode);
        if (cxsf_cap_reg(stat_ok, is_reg, require_empty, stat_ok ? (long long)st.st_size : 0)) {
            close(fd);
            if (cxsf_cap_line(err, (int)errn, 6, 0, 0))
                snprintf(err,errn,"INTERFACE_ERROR capture-ledger-not-empty-regular");
            return 1;
        }
    }
#else
    if (fstat(fd,&st) || !S_ISREG(st.st_mode) || (require_empty && st.st_size != 0)) {
        close(fd); snprintf(err,errn,"INTERFACE_ERROR capture-ledger-not-empty-regular"); return 1;
    }
#endif
    while (at < n) {
        ssize_t z = write(fd,bytes+at,n-at);
#ifdef UNISA_CX_SOURCE_FRAME
        {
            unsigned long long next = at;
            int took = cxsf_cap_take((long)z, errno == EINTR, at, n, &next);
            if (took == 1) continue;
            if (took == 0) { at = (size_t)next; continue; }
            close(fd);
            if (cxsf_cap_line(err, (int)errn, 7, 0, 0))
                snprintf(err,errn,"INTERFACE_ERROR capture-ledger-write");
            return 1;
        }
#else
        if (z < 0 && errno == EINTR) continue;
        if (z <= 0) { close(fd); snprintf(err,errn,"INTERFACE_ERROR capture-ledger-write"); return 1; }
        at += (size_t)z;
#endif
    }
    if (close(fd)) {
#ifdef UNISA_CX_SOURCE_FRAME
        if (cxsf_cap_line(err, (int)errn, 8, 0, 0))
#endif
        snprintf(err,errn,"INTERFACE_ERROR capture-ledger-close"); return 1; }
    return 0;
}
#endif
static void rf_capture_record(const unsigned char *p,int n,int owned,unsigned char *bytes,int len) {
#ifdef _WIN32
    (void)p;(void)n;(void)owned;(void)bytes;(void)len;die("INTERFACE_ERROR capture-unsupported-platform");
#else
    char path[4096],hex[1025],sha[65],name[64],row[1280],err[512];int z,present=owned!=0;const char *dir=rf_capture_dir();
#ifdef UNISA_CX_SOURCE_FRAME
    if (cxsf_cap_session(dir != 0, RF_CAPACTIVE, dir && strcmp(dir, RF_CAPDIR) == 0, RF_CAPOWNER == (long)getpid()))
        die("INTERFACE_ERROR capture-session-not-initialized");
    if (cxsf_cap_key(n, p != 0)) die("INTERFACE_ERROR capture-key-length");
    if (cxsf_cap_answer(owned, len, bytes != 0)) die("INTERFACE_ERROR capture-answer-invalid");
#else
    if (!dir || !RF_CAPACTIVE || strcmp(dir,RF_CAPDIR) || RF_CAPOWNER!=(long)getpid()) die("INTERFACE_ERROR capture-session-not-initialized");
    if (n<=0||n>512||!p) die("INTERFACE_ERROR capture-key-length");
    if (owned<0||owned>2||len<0||(present&&len>0&&!bytes)) die("INTERFACE_ERROR capture-answer-invalid");
#endif
    if (rf_cap_full()) die("INTERFACE_ERROR capture-capacity-exceeded");
#ifdef UNISA_CX_SOURCE_FRAME
    if (cxsf_cap_hex(hex, (int)sizeof hex, p, n)) die("INTERFACE_ERROR capture-key-length");
#else
    for(int i=0;i<n;i++){hex[2*i]="0123456789abcdef"[p[i]>>4];hex[2*i+1]="0123456789abcdef"[p[i]&15];}hex[2*n]=0;
#endif
    if(present){
        unisa_sha256_hex(bytes,(size_t)len,sha);
#ifdef UNISA_CX_SOURCE_FRAME
        if (cxsf_cap_name(name, (int)sizeof name, RF_CAPC)) die("INTERFACE_ERROR capture-row-too-long");
#else
        snprintf(name,sizeof name,"resource-%03d.bytes",RF_CAPC);
#endif
        if(rf_cap_new_file(dir,name,bytes,(size_t)len,err,sizeof err))die(err);
#ifdef UNISA_CX_SOURCE_FRAME
        z = cxsf_cap_row(row, (int)sizeof row, 1, hex, name, len, sha) ? -1 : (int)strlen(row);
#else
        z=snprintf(row,sizeof row,"%s\t1\t%s\t%d\t%s\n",hex,name,len,sha);
#endif
    }else
#ifdef UNISA_CX_SOURCE_FRAME
        z = cxsf_cap_row(row, (int)sizeof row, 0, hex, 0, 0, 0) ? -1 : (int)strlen(row);
#else
        z=snprintf(row,sizeof row,"%s\t0\t-\t0\t-\n",hex);
#endif
    if(z<0||(size_t)z>=sizeof row)die("INTERFACE_ERROR capture-row-too-long");
    if(rf_cap_path(path,sizeof path,dir,"resources.tsv",err,sizeof err)||rf_cap_append(path,(const unsigned char*)row,(size_t)z,0,err,sizeof err))die(err);
    RF_CAPC++;
#endif
}
/* SC2 captures actual answers, not a snapshot map. Its initialization guard
 * owns the legacy-empty-file check through close, then is removed. It is not
 * a whole-ledger COMMIT or a durable filesystem transaction. */
static int rf_identity_write_capture(const char *dir,const char *stage,const unsigned char *input,int n,char *err,size_t errn){
#ifdef _WIN32
    (void)dir;(void)stage;(void)input;(void)n;snprintf(err,errn,"INTERFACE_ERROR capture-unsupported-platform");return 1;
#else
    char path[4096],guard[4096],line[512],hex[65];struct stat st;int empty=0,bad=1;
#ifdef UNISA_CX_SOURCE_FRAME
    if (cxsf_cap_args(dir != 0, stage != 0, n, input != 0)) {
        if (cxsf_cap_dirline(err, (int)errn, 0))
            snprintf(err,errn,"INTERFACE_ERROR capture-bad-arguments");
        return 1;
    }
#else
    if(!dir||!stage||n<0||(n>0&&!input)){snprintf(err,errn,"INTERFACE_ERROR capture-bad-arguments");return 1;}
#endif
    if(rf_cap_path(path,sizeof path,dir,"resources.tsv",err,errn)||rf_cap_path(guard,sizeof guard,dir,".capture-begin",err,errn))return 1;
    if(mkdir(dir,0700)&&errno!=EEXIST){
#ifdef UNISA_CX_SOURCE_FRAME
        if (cxsf_cap_dirline(err, (int)errn, 1))
#endif
        snprintf(err,errn,"INTERFACE_ERROR capture-dir-create");return 1;}
    if(mkdir(guard,0700)){
#ifdef UNISA_CX_SOURCE_FRAME
        if (cxsf_cap_dirline(err, (int)errn, 2))
#endif
        snprintf(err,errn,"INTERFACE_ERROR capture-session-busy");return 1;}
    errno=0;
#ifdef UNISA_CX_SOURCE_FRAME
    {
        int stat_ok = !lstat(path,&st);
        int is_reg = stat_ok && S_ISREG(st.st_mode);
        int absent = !stat_ok && errno == ENOENT;
        int cls = cxsf_cap_ledger(stat_ok, is_reg, absent, stat_ok ? (long long)st.st_size : 0, &empty);
        if (cls) {
            int kind = cls == 1 ? 3 : cls == 2 ? 4 : 5;
            if (cxsf_cap_dirline(err, (int)errn, kind)) {
                if (cls == 1) snprintf(err,errn,"INTERFACE_ERROR capture-ledger-not-regular");
                else if (cls == 2) snprintf(err,errn,"INTERFACE_ERROR capture-ledger-rows-before-identity");
                else snprintf(err,errn,"INTERFACE_ERROR capture-ledger-stat");
            }
            goto done;
        }
    }
#else
    if(!lstat(path,&st)){
        if(!S_ISREG(st.st_mode)){snprintf(err,errn,"INTERFACE_ERROR capture-ledger-not-regular");goto done;}
        if(st.st_size!=0){snprintf(err,errn,"INTERFACE_ERROR capture-ledger-rows-before-identity");goto done;}empty=1;
    }else if(errno!=ENOENT){snprintf(err,errn,"INTERFACE_ERROR capture-ledger-stat");goto done;}
#endif
    {DIR *d=opendir(dir);struct dirent *e;if(!d){
#ifdef UNISA_CX_SOURCE_FRAME
        if (cxsf_cap_dirline(err, (int)errn, 6))
#endif
        snprintf(err,errn,"INTERFACE_ERROR capture-dir-open");goto done;}
     for(;;){errno=0;e=readdir(d);if(!e){int saved=errno,closed=closedir(d);if(saved||closed){
#ifdef UNISA_CX_SOURCE_FRAME
        if (cxsf_cap_dirline(err, (int)errn, 7))
#endif
        snprintf(err,errn,"INTERFACE_ERROR capture-dir-read-close");goto done;}break;}
#ifdef UNISA_CX_SOURCE_FRAME
      if (cxsf_cap_dent(e->d_name, empty)) { closedir(d);
        if (cxsf_cap_dirline(err, (int)errn, 8))
            snprintf(err,errn,"INTERFACE_ERROR capture-dir-not-empty");
        goto done; }
      continue;
#else
      if(!strcmp(e->d_name,".")||!strcmp(e->d_name,"..")||!strcmp(e->d_name,".capture-begin")||(empty&&!strcmp(e->d_name,"resources.tsv")))continue;
      closedir(d);snprintf(err,errn,"INTERFACE_ERROR capture-dir-not-empty");goto done;
#endif
     }}
    rl_input_hex(input,n,hex);
#ifdef UNISA_CX_SOURCE_FRAME
    if (cxsf_cap_idline(line,(int)sizeof line,stage,hex)) {
        if (cxsf_cap_dirline(err, (int)errn, 10))
            snprintf(err,errn,"INTERFACE_ERROR capture-stage-label");
        goto done;
    }
#else
    if(!rl_identity_line(line,sizeof line,stage,hex)){snprintf(err,errn,"INTERFACE_ERROR capture-stage-label");goto done;}line[3]='2';
#endif
#ifdef UNISA_CX_SOURCE_FRAME
    bad=cxsf_cap_place(empty)?rf_cap_append(path,(const unsigned char*)line,strlen(line),1,err,errn):rf_cap_new_file(dir,"resources.tsv",(const unsigned char*)line,strlen(line),err,errn);
#else
    bad=empty?rf_cap_append(path,(const unsigned char*)line,strlen(line),1,err,errn):rf_cap_new_file(dir,"resources.tsv",(const unsigned char*)line,strlen(line),err,errn);
#endif
done:
    if(rmdir(guard)&&!bad){bad=1;
#ifdef UNISA_CX_SOURCE_FRAME
        if (cxsf_cap_dirline(err, (int)errn, 9))
#endif
        snprintf(err,errn,"INTERFACE_ERROR capture-session-cleanup");}
    if(!bad){strcpy(RF_CAPDIR,dir);RF_CAPC=0;RF_CAPOWNER=(long)getpid();RF_CAPACTIVE=1;}
    return bad;
#endif
}
/* Shared identity label constructor. Both the gate and witnesses use the
   caller's actual stage/route; no independent label grammar in a runner. */
static int rf_stage_label(char *dst,size_t cap,const char *stage_name,const char *route) {
#ifdef UNISA_CX_SOURCE_FRAME
    return cxsf_stage_join(dst, cap > 2147483647u ? 2147483647 : (int)cap, stage_name, route) == 0;
#else
    int n;
    if (!dst || !stage_name || !route || !cap) return 0;
    n=snprintf(dst,cap,"%s:%s",stage_name,route);
    return n>=0 && (size_t)n<cap && rl_stage_label_ok(dst);
#endif
}
static int rf_identity_gate(const unsigned char *input,int n,int mode,const char *route,const char *stage_name) {
    char err[512], stage[160];
    /* Image RI: e3 identity may bind …/image/O* when mode is image.
       Tape keeps its existing mode==2+/tape/O contract. Other routes still refuse. */
#ifdef UNISA_CX_SOURCE_FRAME
    if (cxsf_route_gate(mode, route)) {
        if (cxsf_route_line(err, (int)sizeof err, 0, route))
            fprintf(stderr, "unisacc: INTERFACE_ERROR resource-route-unsupported %s (strict and capture cover tape and image routes only)\n", route?route:"-");
        else fprintf(stderr, "unisacc: %s\n", err);
        return 2;
    }
#else
    {
    int tape_ok = mode == 2 && route && strstr(route,"/tape/O");
    int image_ok = mode == 0 && route && strstr(route,"/image/O");
    if (!tape_ok && !image_ok) { fprintf(stderr, "unisacc: INTERFACE_ERROR resource-route-unsupported %s (strict and capture cover tape and image routes only)\n", route?route:"-"); return 2; }
    }
#endif
    if (!rf_stage_label(stage,sizeof stage,stage_name,route)) { fprintf(stderr,"unisacc: INTERFACE_ERROR capture-stage-label\n"); return 2; }
    if (RF_STRICT && rl_check_identity(&RF_LEDGER, stage, input, n, err, sizeof err)) { fprintf(stderr, "unisacc: %s\n", err); return 2; }
    if (rf_capture_dir() && rf_identity_write_capture(rf_capture_dir(), stage, input, n, err, sizeof err)) { fprintf(stderr, "unisacc: %s\n", err); return 2; }
    if (RF_STRICT) { RF_LEDGER.cursor = 0; RF_TRACE_STARTED = 1; }
    return 0;
}
/* E3 identity material: E1 token bytes + attribute stream + route is in the stage label. */
#ifdef UNISA_CX_SOURCE_FRAME
int cxsf_pair(const unsigned char *,int,const unsigned char *,int,unsigned char **,int *,void *(*)(void *,size_t));
static unsigned char *rf_e3_identity_bytes(const unsigned char *tokens,int tn,const unsigned char *attrs,int an,int *out_n) {
    unsigned char *frame=0;
    return cxsf_pair(tokens,tn,attrs,an,&frame,out_n,realloc) ? 0 : frame;
}
#else
static unsigned char *rf_e3_identity_bytes(const unsigned char *tokens,int tn,const unsigned char *attrs,int an,int *out_n) {
    size_t total; unsigned char *b; int j;
    if (tn < 0 || an < 0 || (tn > 0 && !tokens) || (an > 0 && !attrs) || !out_n) return 0;
    total = 8 + (size_t)tn + (size_t)an;
    if (total > 0x7fffffff) return 0;
    b = malloc(total ? total : 1); if (!b) return 0;
    for (j = 0; j < 4; j++) b[j] = (unsigned char)(((unsigned)tn >> (8 * j)) & 255);
    if (tn) memcpy(b + 4, tokens, (size_t)tn);
    for (j = 0; j < 4; j++) b[4 + tn + j] = (unsigned char)(((unsigned)an >> (8 * j)) & 255);
    if (an) memcpy(b + 8 + tn, attrs, (size_t)an);
    *out_n = (int)total; return b;
}
#endif
/* Parse exact single-line e3 companion bytes (must end in '\n', no NULs/extra lines). */
#ifdef UNISA_CX_SOURCE_FRAME
int cxsf_companion(const unsigned char *,int,char *,size_t,char *);
static int rf_parse_e3_identity_bytes(const unsigned char *b,int n,char *stage,size_t stage_cap,char hex[65]) {
    return cxsf_companion(b,n,stage,stage_cap,hex);
}
#else
static int rf_parse_e3_identity_bytes(const unsigned char *b,int n,char *stage,size_t stage_cap,char hex[65]) {
    const char *s, *t, *v; size_t sn, i;
    if (n < 20 || !b || b[n - 1] != '\n') return 0;
    /* Exact single-line receipt: no NULs, no trailing junk after the newline. */
    for (i = 0; i < (size_t)(n - 1); i++) if (b[i] == 0 || b[i] == '\n') return 0;
    if (n - 1 >= (int)sizeof(char) * 512) return 0;
    { char line[512]; if ((size_t)n > sizeof line) return 0; memcpy(line,b,(size_t)n); line[n - 1] = 0;
      if (strncmp(line,"#SC2\tstage=",11)) return 0;
      s = line + 11; t = strchr(s,'\t'); if (!t) return 0;
      sn = (size_t)(t - s); if (sn == 0 || sn >= stage_cap) return 0;
      memcpy(stage,s,sn); stage[sn] = 0;
      v = t + 1; if (strncmp(v,"input_sha256=",13) || strlen(v + 13) != 64) return 0;
      for (i = 0; i < 64; i++) {
        char c = v[13 + i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return 0;
        hex[i] = c;
      }
      hex[64] = 0;
      if (v[13 + 64] != 0) return 0;
    }
    return 1;
}
#endif
/* Key that binds the e3 companion into the same SC2 ordered pack as front-end events. */
static const unsigned char RF_E3_ID_KEY[] = {'\0','r','i','/','e','3','-','i','d','e','n','t','i','t','y'};
enum { RF_E3_ID_KEY_N = 15, RF_E3_ID_MAX = 512 };
/* Bounded regular full-file read of e3-identity.tsv. Authority is the entire file
 * (size, unique trailing LF, no NUL/extra lines, 64 lowercase hex) — never fgets. */
static int rf_e3_read_companion(const char *dir, unsigned char **out, int *out_n, char *err, size_t errn) {
#ifdef _WIN32
    (void)dir; (void)out; (void)out_n;
#ifdef UNISA_CX_SOURCE_FRAME
    if (cxsf_e3_line(err, (int)errn, 4, 0))
#endif
    snprintf(err, errn, "INTERFACE_ERROR resource-e3-identity-unsupported-platform");
    return 1;
#else
    char path[4096]; unsigned char *b; size_t n = 0;
#ifdef UNISA_CX_SOURCE_FRAME
    if (cxsf_e3_args(dir != 0, dir && *dir, out != 0, out_n != 0)) {
        if (cxsf_e3_line(err, (int)errn, 0, 0))
            snprintf(err, errn, "INTERFACE_ERROR resource-e3-identity-absent");
        return 1;
    }
    if (cxsf_cap_join(path, (int)sizeof path, dir, "e3-identity.tsv")) {
        if (cxsf_e3_line(err, (int)errn, 1, 0))
            snprintf(err, errn, "INTERFACE_ERROR resource-e3-identity-path");
        return 1;
    }
#else
    int z;
    if (!dir || !*dir || !out || !out_n) {
        snprintf(err, errn, "INTERFACE_ERROR resource-e3-identity-absent"); return 1;
    }
    z = snprintf(path, sizeof path, "%s/e3-identity.tsv", dir);
    if (z < 0 || (size_t)z >= sizeof path) {
        snprintf(err, errn, "INTERFACE_ERROR resource-e3-identity-path"); return 1;
    }
#endif
    errno = 0; b = rl_readfile(path, &n);
    if (!b) {
#ifdef UNISA_CX_SOURCE_FRAME
        {
            int cls = cxsf_e3_errno(errno == ENOENT, errno == ELOOP);
            int kind = cls == 0 ? 0 : cls == 1 ? 2 : 5;
            if (cxsf_e3_line(err, (int)errn, kind, errno)) {
                if (cls == 0) snprintf(err, errn, "INTERFACE_ERROR resource-e3-identity-absent");
                else if (cls == 1) snprintf(err, errn, "INTERFACE_ERROR resource-e3-identity-symlink");
                else snprintf(err, errn, "INTERFACE_ERROR resource-e3-identity-read-failed errno=%d", errno);
            }
        }
#else
        if (errno == ENOENT) snprintf(err, errn, "INTERFACE_ERROR resource-e3-identity-absent");
        else if (errno == ELOOP) snprintf(err, errn, "INTERFACE_ERROR resource-e3-identity-symlink");
        else snprintf(err, errn, "INTERFACE_ERROR resource-e3-identity-read-failed errno=%d", errno);
#endif
        return 1;
    }
#ifdef UNISA_CX_SOURCE_FRAME
    {
        int span = cxsf_e3_span(1, (unsigned long long)n, (unsigned long long)RF_E3_ID_MAX);
        if (span) {
            free(b);
            if (cxsf_e3_line(err, (int)errn, span == 3 ? 3 : 0, 0))
                snprintf(err, errn, n ? "INTERFACE_ERROR resource-e3-identity-too-large" : "INTERFACE_ERROR resource-e3-identity-absent");
            return 1;
        }
    }
#else
    if (n == 0 || n > (size_t)RF_E3_ID_MAX) {
        free(b);
        snprintf(err, errn, n ? "INTERFACE_ERROR resource-e3-identity-too-large" : "INTERFACE_ERROR resource-e3-identity-absent");
        return 1;
    }
#endif
    *out = b; *out_n = (int)n; return 0;
#endif
}
/* Second-segment e3 identity: companion e3-identity.tsv is SC2 row \0ri/e3-identity blob.
 * Strict path re-reads that regular file in full and requires exact bytes == expected line. */
static int rf_e3_second_identity(const unsigned char *input,int n,int mode,const char *route) {
    char stage[160], err[512], line[512], hex[65], got_stage[160], got_hex[65];
    const char *dir; int ln;
#ifdef UNISA_CX_SOURCE_FRAME
    if (cxsf_route_e3(mode, route)) {
        if (cxsf_route_line(err, (int)sizeof err, 1, route))
            fprintf(stderr,"unisacc: INTERFACE_ERROR resource-route-unsupported %s (e3 second identity is image-only)\n", route?route:"-");
        else fprintf(stderr,"unisacc: %s\n", err);
        return 2;
    }
#else
    if (mode != 0 || !route || !strstr(route,"/image/O")) {
        fprintf(stderr,"unisacc: INTERFACE_ERROR resource-route-unsupported %s (e3 second identity is image-only)\n", route?route:"-");
        return 2;
    }
#endif
    if (!rf_stage_label(stage,sizeof stage,"e3",route)) {
        fprintf(stderr,"unisacc: INTERFACE_ERROR capture-stage-label\n"); return 2;
    }
    rl_input_hex(input,n,hex);
#ifdef UNISA_CX_SOURCE_FRAME
    if (cxsf_cap_idline(line,(int)sizeof line,stage,hex)) { fprintf(stderr,"unisacc: INTERFACE_ERROR capture-stage-label\n"); return 2; }
#else
    if (!rl_identity_line(line,sizeof line,stage,hex)) { fprintf(stderr,"unisacc: INTERFACE_ERROR capture-stage-label\n"); return 2; }
    line[3] = '2';
#endif
    ln = (int)strlen(line);
    if (ln <= 0 || ln >= (int)sizeof line) { fprintf(stderr,"unisacc: INTERFACE_ERROR capture-stage-label\n"); return 2; }
    /* Capture: capacity first (same 4096 contract as rf_capture_record), then companion + SC2 row.
     * Refuse before creating e3-identity.tsv so a full pack never leaves a partial companion. */
    if (rf_capture_dir()) {
#ifdef _WIN32
        fprintf(stderr,"unisacc: INTERFACE_ERROR capture-unsupported-platform\n"); return 2;
#else
        char e3path[4096], path[4096], row[1280], hkey[64], sha[65]; int rz;
        dir = rf_capture_dir();
        if (rf_cap_full()) {
            fprintf(stderr,"unisacc: INTERFACE_ERROR capture-capacity-exceeded\n"); return 2;
        }
        if (rf_cap_new_file(dir,"e3-identity.tsv",(const unsigned char *)line,(size_t)ln,err,sizeof err)) {
            fprintf(stderr,"unisacc: %s\n", err); return 2;
        }
#ifdef UNISA_CX_SOURCE_FRAME
        if (cxsf_cap_hex(hkey, (int)sizeof hkey, RF_E3_ID_KEY, RF_E3_ID_KEY_N)) {
            fprintf(stderr,"unisacc: INTERFACE_ERROR capture-key-length\n"); return 2;
        }
#else
        { int i;
        for (i = 0; i < RF_E3_ID_KEY_N; i++) {
            hkey[2 * i] = "0123456789abcdef"[RF_E3_ID_KEY[i] >> 4];
            hkey[2 * i + 1] = "0123456789abcdef"[RF_E3_ID_KEY[i] & 15];
        }
        hkey[2 * RF_E3_ID_KEY_N] = 0; }
#endif
        unisa_sha256_hex((const unsigned char *)line,(size_t)ln,sha);
#ifdef UNISA_CX_SOURCE_FRAME
        rz = cxsf_cap_row(row,(int)sizeof row,1,hkey,"e3-identity.tsv",ln,sha) ? -1 : (int)strlen(row);
#else
        rz = snprintf(row,sizeof row,"%s\t1\te3-identity.tsv\t%d\t%s\n",hkey,ln,sha);
#endif
        if (rz < 0 || (size_t)rz >= sizeof row) {
            if (!rf_cap_path(e3path,sizeof e3path,dir,"e3-identity.tsv",err,sizeof err)) unlink(e3path);
            fprintf(stderr,"unisacc: INTERFACE_ERROR capture-row-too-long\n"); return 2;
        }
        if (rf_cap_path(path,sizeof path,dir,"resources.tsv",err,sizeof err) ||
            rf_cap_append(path,(const unsigned char *)row,(size_t)rz,0,err,sizeof err)) {
            if (!rf_cap_path(e3path,sizeof e3path,dir,"e3-identity.tsv",err,sizeof err)) unlink(e3path);
            fprintf(stderr,"unisacc: %s\n", err); return 2;
        }
        RF_CAPC++;
        return 0;
#endif
    }
    if (!RF_STRICT) return 0;
    /* Strict: SC2 next event is e3-identity row; companion file is re-read in full (no fgets). */
#ifdef UNISA_CX_SOURCE_FRAME
    if (cxsf_e3_ready(RF_TRACE_STARTED, RF_LEDGER.trace, RF_LEDGER_DIR[0] != 0, RF_LEDGER.cursor, RF_LEDGER.n)) {
        fprintf(stderr,"unisacc: INTERFACE_ERROR resource-e3-identity-absent\n"); return 2;
    }
#else
    if (!RF_TRACE_STARTED || !RF_LEDGER.trace || !RF_LEDGER_DIR[0]) {
        fprintf(stderr,"unisacc: INTERFACE_ERROR resource-e3-identity-absent\n"); return 2;
    }
    if (RF_LEDGER.cursor >= RF_LEDGER.n) {
        fprintf(stderr,"unisacc: INTERFACE_ERROR resource-e3-identity-absent\n"); return 2;
    }
#endif
    {
        const RLRow *r = &RF_LEDGER.rows[RF_LEDGER.cursor];
        unsigned char *fileb = 0; int filen = 0;
#ifdef UNISA_CX_SOURCE_FRAME
        {
            int row = cxsf_e3_row(r->kn, RF_E3_ID_KEY_N, r->key, RF_E3_ID_KEY, r->present, r->value != 0, r->blob);
            if (row == 2) {
                if (cxsf_e3_cmp_line(err, (int)sizeof err, 0, 0, 0))
                    fprintf(stderr,"unisacc: INTERFACE_ERROR resource-e3-identity-malformed\n");
                else fprintf(stderr,"unisacc: %s\n", err);
                return 2;
            }
            if (row) {
                fprintf(stderr,"unisacc: INTERFACE_ERROR resource-e3-identity-absent\n"); return 2;
            }
        }
#else
        if (r->kn != RF_E3_ID_KEY_N || memcmp(r->key,RF_E3_ID_KEY,RF_E3_ID_KEY_N) || !r->present || !r->value) {
            fprintf(stderr,"unisacc: INTERFACE_ERROR resource-e3-identity-absent\n"); return 2;
        }
        if (strcmp(r->blob,"e3-identity.tsv")) {
            fprintf(stderr,"unisacc: INTERFACE_ERROR resource-e3-identity-malformed\n"); return 2;
        }
#endif
        if (rf_e3_read_companion(RF_LEDGER_DIR,&fileb,&filen,err,sizeof err)) {
            fprintf(stderr,"unisacc: %s\n", err); return 2;
        }
        /* Full-file authority: size, unique trailing LF, no NUL/extra lines, 64 lowercase hex. */
#ifdef UNISA_CX_SOURCE_FRAME
        if (!rf_parse_e3_identity_bytes(fileb,filen,got_stage,sizeof got_stage,got_hex)) {
            int tail = cxsf_e3_tail(fileb, filen, (const unsigned char *)line, ln);
            if (cxsf_e3_cmp_line(err, (int)sizeof err, tail == 1 ? 1 : 0, 0, 0))
                fprintf(stderr, tail == 1 ? "unisacc: INTERFACE_ERROR resource-e3-identity-trailing\n"
                                          : "unisacc: INTERFACE_ERROR resource-e3-identity-malformed\n");
            else fprintf(stderr,"unisacc: %s\n", err);
            free(fileb); return 2;
        }
        if (cxsf_e3_match(fileb, filen, (const unsigned char *)line, ln)) {
            if (cxsf_e3_cmp_line(err, (int)sizeof err, 2, 0, 0))
                fprintf(stderr,"unisacc: INTERFACE_ERROR resource-identity-mismatch input e3-second\n");
            else fprintf(stderr,"unisacc: %s\n", err);
            free(fileb); return 2;
        }
        if (cxsf_e3_same(r->value, r->vn, fileb, filen)) {
            if (cxsf_e3_cmp_line(err, (int)sizeof err, 3, 0, 0))
                fprintf(stderr,"unisacc: INTERFACE_ERROR resource-e3-identity-inconsistent\n");
            else fprintf(stderr,"unisacc: %s\n", err);
            free(fileb); return 2;
        }
        if (cxsf_e3_id(got_stage, stage, got_hex, hex)) {
            if (cxsf_e3_cmp_line(err, (int)sizeof err, 4, got_stage, stage))
                fprintf(stderr,"unisacc: INTERFACE_ERROR resource-identity-mismatch stage ledger=%s run=%s\n", got_stage, stage);
            else fprintf(stderr,"unisacc: %s\n", err);
            free(fileb); return 2;
        }
#else
        if (!rf_parse_e3_identity_bytes(fileb,filen,got_stage,sizeof got_stage,got_hex)) {
            /* Prefix-equal but longer => trailing junk after the identity line. */
            if (filen > ln && ln > 0 && !memcmp(fileb,line,(size_t)ln))
                fprintf(stderr,"unisacc: INTERFACE_ERROR resource-e3-identity-trailing\n");
            else
                fprintf(stderr,"unisacc: INTERFACE_ERROR resource-e3-identity-malformed\n");
            free(fileb); return 2;
        }
        if (filen != ln || memcmp(fileb,line,(size_t)ln)) {
            fprintf(stderr,"unisacc: INTERFACE_ERROR resource-identity-mismatch input e3-second\n");
            free(fileb); return 2;
        }
        if (r->vn != filen || memcmp(r->value,fileb,(size_t)filen)) {
            fprintf(stderr,"unisacc: INTERFACE_ERROR resource-e3-identity-inconsistent\n");
            free(fileb); return 2;
        }
        if (strcmp(got_stage,stage) || strcmp(got_hex,hex)) {
            fprintf(stderr,"unisacc: INTERFACE_ERROR resource-identity-mismatch stage ledger=%s run=%s\n", got_stage, stage);
            free(fileb); return 2;
        }
#endif
        free(fileb);
        RF_LEDGER.cursor++;
    }
    return 0;
}
/* SC1 multi-source identity input: the source frame, per source in order: u32le name length, name, u32le byte count, bytes.
   Built from the sources, not from the unit outputs, so it exists before the first unit row is recorded. */
#ifdef UNISA_CX_SOURCE_FRAME
int cxsf_encode(int,const char **,const unsigned char **,const int *,unsigned char **,int *,void *(*)(void *,size_t));
static unsigned char *rf_source_frame(int count,const char **names,const unsigned char **bytes,const int *lens,int *out_n) {
    unsigned char *frame=0;
    return cxsf_encode(count,names,bytes,lens,&frame,out_n,realloc) ? 0 : frame;
}
#else
static unsigned char *rf_source_frame(int count,const char **names,const unsigned char **bytes,const int *lens,int *out_n) {
    size_t total=0; int i, j; unsigned char *b, *p;
    for (i=0;i<count;i++) total+=8+strlen(names[i])+(size_t)lens[i];
    b=malloc(total?total:1); if (!b) return 0;
    p=b;
    for (i=0;i<count;i++) {
        size_t nl=strlen(names[i]);
        for (j=0;j<4;j++) *p++=(unsigned char)(nl>>(8*j));
        memcpy(p,names[i],nl); p+=nl;
        for (j=0;j<4;j++) *p++=(unsigned char)((unsigned)lens[i]>>(8*j));
        if (lens[i]) memcpy(p,bytes[i],(size_t)lens[i]);
        p+=lens[i];
    }
    *out_n=(int)total; return b;
}
#endif
/* RI snapshot overlay (standalone import only; API and default runs never set it) */
static RLLedger RI_SNAP; static int RI_SNAP_ON = 0;
/* shared RI-array writer: converts the caller's ResourceInput rows (ATTR slot included as its key) to the row interface */
static inline int rf_snapshot_write_ri(const char *dir, const char *stage_label, const unsigned char *input, int inputn,
                                       const ResourceInput *ri, int nri, char *err, size_t errn) {
    RLBlob b[RL_SNAP_MAX]; int i;
    if (!rl_stage_label_ok(stage_label)) {
#ifdef UNISA_CX_SOURCE_FRAME
        if (cxsf_stage_err(err, errn > 2147483647u ? 2147483647 : (int)errn, 0, stage_label))
#endif
        snprintf(err, errn, "INTERFACE_ERROR snapshot-stage-label %s", stage_label);
        return 1;
    }
#ifdef UNISA_CX_SOURCE_FRAME
    if (cxsf_snap_count(nri, RL_SNAP_MAX)) {
        if (cxsf_snap_line(err, errn > 2147483647u ? 2147483647 : (int)errn))
            snprintf(err, errn, "INTERFACE_ERROR snapshot-too-many-blobs");
        return 1;
    }
#else
    if (nri < 0 || nri > RL_SNAP_MAX) { snprintf(err, errn, "INTERFACE_ERROR snapshot-too-many-blobs"); return 1; }
#endif
    for (i = 0; i < nri; i++) {
#ifdef UNISA_CX_SOURCE_FRAME
        if (cxsf_snap_slot(ri[i].name, ri[i].n, ri[i].data, ri[i].len, &b[i].key, &b[i].kn, &b[i].data, &b[i].len))
            return 1;
#else
        b[i].key = ri[i].name; b[i].kn = ri[i].n; b[i].data = ri[i].data; b[i].len = ri[i].len;
#endif
    }
    return rl_snapshot_write(dir, stage_label, input, inputn, b, nri, err, errn);
}
static inline int rf_snapshot_load_ri(const char *dir, const char *upstream_stage_label, RLLedger *L, int observe, char *err, size_t errn) {
    return rl_snapshot_load(dir, upstream_stage_label, L, observe, err, errn);
}
/* capture header for an independent consumer (standalone lower): the directory must be absent or empty; the header is the
   consumer's own identity (its label and its real input bytes), written by the same identity writer as the e3 gate */
static inline int rf_capture_begin(const char *dir, const char *stage_label, const unsigned char *input, int inputn, char *err, size_t errn) {
#ifdef UNISA_CX_SOURCE_FRAME
    if (cxsf_cap_args(1, stage_label != 0, inputn, input != 0)) {
        if (cxsf_cap_dirline(err, (int)errn, 0))
            snprintf(err,errn,"INTERFACE_ERROR capture-bad-arguments");
        return 1;
    }
#else
    if (!stage_label || inputn < 0 || (inputn > 0 && !input)) { snprintf(err,errn,"INTERFACE_ERROR capture-bad-arguments");return 1; }
#endif
    if (!rl_stage_label_ok(stage_label)) {
#ifdef UNISA_CX_SOURCE_FRAME
        if (cxsf_stage_err(err, errn > 2147483647u ? 2147483647 : (int)errn, 1, stage_label))
#endif
        snprintf(err, errn, "INTERFACE_ERROR capture-stage-label %s", stage_label);
        return 1;
    }
    return rf_identity_write_capture(dir, stage_label, input, inputn, err, errn);
}
int core_host_fetch(const unsigned char *p,int n,unsigned char **bytes,int *len) {
    static const unsigned char empty[1]={0};
    if(n<0||(!p&&n>0)||!bytes||!len)die("INTERFACE_ERROR resource-fetch-bad-arguments");
    if(!p)p=empty;*bytes=0;*len=0;
    if(RF_STRICT){
        static char msg[1280];char h[1025];const RLRow *r=0;
        if(n>512)die("resource key too long");
#ifdef UNISA_CX_SOURCE_FRAME
        if (cxsf_cap_hex(h, (int)sizeof h, p, n)) die("resource key too long");
#else
        for(int i=0;i<n;i++){h[2*i]="0123456789abcdef"[p[i]>>4];h[2*i+1]="0123456789abcdef"[p[i]&15];}h[2*n]=0;
#endif
        if(RF_LEDGER.trace){
#ifdef UNISA_CX_SOURCE_FRAME
            int index=0, code=cxsf_ledger_next(RF_LEDGER.rows,RF_LEDGER.n,RF_TRACE_STARTED,&RF_LEDGER.cursor,p,n,&index);
            if(code==1)die("INTERFACE_ERROR resource-trace-not-started");
            if(code==2){
                if (cxsf_fetch_miss(msg, (int)sizeof msg, 1, h, RF_LEDGER.cursor))
                    snprintf(msg,sizeof msg,"MISSING_EXPLICIT_RESOURCE %s event=%d",h,RF_LEDGER.cursor);
                die(msg);
            }
            if(code==3){
                if (cxsf_fetch_mismatch(msg, (int)sizeof msg, h, RF_LEDGER.cursor))
                    snprintf(msg,sizeof msg,"INTERFACE_ERROR resource-trace-key-mismatch %s event=%d",h,RF_LEDGER.cursor);
                die(msg);
            }
            r=&RF_LEDGER.rows[index];
#else
            if(!RF_TRACE_STARTED)die("INTERFACE_ERROR resource-trace-not-started");
            if(RF_LEDGER.cursor>=RF_LEDGER.n){snprintf(msg,sizeof msg,"MISSING_EXPLICIT_RESOURCE %s event=%d",h,RF_LEDGER.cursor);die(msg);}
            r=&RF_LEDGER.rows[RF_LEDGER.cursor];
            if(r->kn!=n||memcmp(r->key,p,(size_t)n)){snprintf(msg,sizeof msg,"INTERFACE_ERROR resource-trace-key-mismatch %s event=%d",h,RF_LEDGER.cursor);die(msg);}
            RF_LEDGER.cursor++;
#endif
        }else r=rl_find(&RF_LEDGER,p,n);
        if(!r){
#ifdef UNISA_CX_SOURCE_FRAME
            if (cxsf_fetch_miss(msg, (int)sizeof msg, 0, h, 0))
#endif
            snprintf(msg,sizeof msg,"MISSING_EXPLICIT_RESOURCE %s",h);
            die(msg);
        }
        /* Strict: ledger answers only. Do not re-read host disk for inc/hdr (offline replay). */
#ifdef UNISA_CX_SOURCE_FRAME
        {
            int owned = cxsf_fetch_hand(r->present, r->value, r->vn, bytes, len);
            if (owned == 4) return 1;
            if(rf_capture_dir())rf_capture_record(p,n,r->present,*bytes,*len);
            return owned;
        }
#else
        if(r->present){*bytes=r->value;*len=r->vn;}
        if(rf_capture_dir())rf_capture_record(p,n,r->present,*bytes,*len);
        return r->present?1:0;
#endif
    }
    if(RI_SNAP_ON){
        const RLRow *r=rl_find(&RI_SNAP,p,n);
        if(r){
#ifdef UNISA_CX_SOURCE_FRAME
            int owned = cxsf_fetch_hand(r->present, r->value, r->vn, bytes, len);
            if (owned == 4) return 1;
            if(rf_capture_dir())rf_capture_record(p,n,r->present,*bytes,*len);
            return owned;
#else
            if(r->present){*bytes=r->value;*len=r->vn;}
            if(rf_capture_dir())rf_capture_record(p,n,r->present,*bytes,*len);
            return r->present?1:0;
#endif
        }
    }
    int owned=rf_default_fetch(p,n,bytes,len);
    if(rf_capture_dir())rf_capture_record(p,n,owned,*bytes,*len);
    return owned;
}
static int rf_default_fetch(const unsigned char *p,int n,unsigned char **bytes,int *len) {
#ifdef UNISA_CX_SOURCE_FRAME
    for (int j=0;j<NRI;j++) {
        int hit = cxsf_fetch_row(p, n, RI[j].name, RI[j].n, RI[j].data, RI[j].len, bytes, len);
        if (hit == 4) return 0;
        if (hit == 1) return 1;
    }
    for (int j=0;j<NR;j++) {
        int hit = cxsf_fetch_row(p, n, PB + RES[j].name, RES[j].n, PB + RES[j].data, RES[j].len, bytes, len);
        if (hit == 4) return 0;
        if (hit == 1) return 1;
    }
#else
    for (int j=0;j<NRI;j++) if (RI[j].n==n && !memcmp(RI[j].name,p,n)) {
        *bytes=(unsigned char *)RI[j].data; *len=RI[j].len; return 1;
    }
    for (int j=0;j<NR;j++) if (RES[j].n==n && !memcmp(PB+RES[j].name,p,n)) {
        *bytes=PB+RES[j].data; *len=RES[j].len; return 1;
    }
#endif
    char path[4096];
#ifdef UNISA_CX_SOURCE_FRAME
    if (cxsf_fetch_path(path, (int)sizeof path, p, n, INCDIR)) return 0;
#else
    /* S2: a logical include key \0inc/q|a/NAME names a cwd-relative file: the prefix is stripped here, on the host side */
    if (n>=7 && p[0]==0 && !memcmp(p+1,"inc/",4) && (p[5]=='q'||p[5]=='a') && p[6]=='/') { p+=7; n-=7; }
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
#endif
    *bytes=readfile(path,len,1);
#ifdef UNISA_CX_SOURCE_FRAME
    {
        int owned = cxsf_fetch_file(*bytes, path, record_file_read, record_file_absent);
        if (owned == 4) return 0;
        return owned;
    }
#else
    if (*bytes) record_file_read(path);
    else record_file_absent(path);
    return *bytes ? 2 : 0;
#endif
}
#ifdef UNISA_RUNTIME_LIBRARY
/* CLI byte framing is host IO, not a machine action. */
static void bput(Buf *o,int c,I at) {
    if (o->n>=o->cap) { o->cap=o->cap ? o->cap*2 : 256;
        o->b=xrealloc(o->b,o->cap); o->at=xrealloc(o->at,sizeof(I)*o->cap); }
    o->b[o->n]=(unsigned char)c; o->at[o->n]=at; o->n++;
}
#endif
#ifndef UNISA_PRODUCT_BUILD
/* The standalone executor accepts a positive decimal budget, never strtol's
 * partial prefix, sign/whitespace coercion or saturated overflow result. */
static int runtime_step_budget(const char *s,I *value) {
#ifdef UNISA_CX_MODEL_NUMBER
    long long parsed;
    if (!cxmn_budget(s,&parsed)) return 0;
    *value=(I)parsed;
    return 1;
#else
    I v=0,limit=9223372036854775807LL;
    if (!s || !*s) return 0;
    for (;*s;s++) {
        if (*s<'0' || *s>'9') return 0;
        int digit=*s-'0';
        if (v>(limit-digit)/10) return 0;
        v=v*10+digit;
    }
    if (!v) return 0;
    *value=v;return 1;
#endif
}
#endif
#ifdef UNISA_CX_RUNEXEC
int ex_budget(int product, const char *budget, long long fallback,
              int (*parse)(const char *, long long *),
              long long *steps, char *why, long cap);
int ex_lines(int status, const char *reason, int reason_n, long long steps,
             char *reject, long reject_cap, char *runmsg, long run_cap,
             char *blown, long blown_cap);
int ex_err(const void *bytes, int n, void **owned);
static int ex_parse(const char *s, long long *v) {
#ifndef UNISA_PRODUCT_BUILD
    I parsed;
    if (!runtime_step_budget(s, &parsed)) return 0;
    *v = (long long)parsed;
    return 1;
#else
    (void)s; (void)v; return 0;
#endif
}
#endif
static int execute(unsigned char *input,int inputn,const char *src,Buf *result) {
    CoreModel m; core_model(&m);
    I maxsteps=UNISA_DEFAULT_MAXSTEPS;
#ifdef UNISA_CX_RUNEXEC
    {
        long long chosen = maxsteps;
        char why[160];
        const char *budget = 0;
        int gate;
#ifndef UNISA_PRODUCT_BUILD
        budget = getenv("UNISA_MAXSTEPS");
#endif
        gate = ex_budget(
#ifdef UNISA_PRODUCT_BUILD
            1
#else
            0
#endif
            , budget, (long long)UNISA_DEFAULT_MAXSTEPS, ex_parse, &chosen, why, (long)sizeof why);
        if (gate == 2) { fputs(why, stderr); memset(result, 0, sizeof *result); return 2; }
        if (gate == 1) {
            fprintf(stderr, "run: UNISA_MAXSTEPS must be a positive decimal integer <= 9223372036854775807\n");
            memset(result, 0, sizeof *result);
            return 2;
        }
        maxsteps = (I)chosen;
    }
#else
#ifdef UNISA_PRODUCT_BUILD
    /* shipped: a build constant, the host environment has no say */
#else
    {
        const char *budget=getenv("UNISA_MAXSTEPS");
        if (budget && !runtime_step_budget(budget,&maxsteps)) {
            fprintf(stderr,"run: UNISA_MAXSTEPS must be a positive decimal integer <= 9223372036854775807\n");
            memset(result,0,sizeof *result);
            return 2;
        }
    }
#endif
#endif
    CoreResult r; int status=core_run(&m,input,inputn,src,maxsteps,&r);
#ifdef UNISA_RUNTIME_DIAGNOSTIC
    UNISA_RUNTIME_DIAGNOSTIC(status,r.reason,r.reason_n,&r.err);
#endif
#ifdef UNISA_CX_RUNEXEC
    {
        char reject[1024], runmsg[1024], blown[256];
        int lines = ex_lines(status, r.reason, r.reason_n, (long long)maxsteps,
                             reject, (long)sizeof reject, runmsg, (long)sizeof runmsg,
                             blown, (long)sizeof blown);
        if (lines == 0) {
            if (reject[0]) fputs(reject, stderr);
            if (runmsg[0]) fputs(runmsg, stderr);
            if (blown[0]) fputs(blown, stderr);
        } else {
            if (status==1 && r.reason_n) fprintf(stderr,"reject: %.*s\n",r.reason_n,r.reason);
            if (status==2) fprintf(stderr,"run: %s\n",r.reason);
            if (status==3) fprintf(stderr,"step budget exhausted (budget %lld steps)\n",(long long)maxsteps);
        }
        {
            void *owned = 0;
            int gate = ex_err(r.err.b, r.err.n, &owned);
            if (gate == 1) fwrite(owned, 1, (size_t)r.err.n, stderr);
            if (gate == 4) free(r.err.b);
            else free(owned);
        }
    }
#else
    if (status==1 && r.reason_n) fprintf(stderr,"reject: %.*s\n",r.reason_n,r.reason);
    if (status==2) fprintf(stderr,"run: %s\n",r.reason);
    if (status==3) fprintf(stderr,"step budget exhausted (budget %lld steps)\n",(long long)maxsteps);
    if (r.err.n) fwrite(r.err.b,1,r.err.n,stderr);
    free(r.err.b);
#endif
    result->b=r.out.b; result->n=r.out.n;
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
#ifdef UNISA_CX_ATTRIBUTE_FRAME
int cxaf_strip(unsigned char **,long *,long *,unsigned char *,long *,unsigned,void *(*)(void *,size_t));
int cxaf_merge(unsigned char **,long *,long *,const unsigned char *,long,unsigned,void *(*)(void *,size_t));
static void attr_publish(void) {
    if (ATTR_SLOT >= 0) { RI[ATTR_SLOT].data = ATTRS.b; RI[ATTR_SLOT].len = ATTRS.n; }
}
static void attr_error(int rc) {
    if (rc == 2) die("malformed USLATTR1 side-car");
    if (rc == 4) die("attribute table too large");
    if (rc == 6) die("out of memory");
    die("invalid attribute table");
}
static int attr_strip(Buf *out) {
    long n = ATTRS.n, cap = ATTRS.cap, in = out->n;
    int rc = cxaf_strip(&ATTRS.b, &n, &cap, out->b, &in, (unsigned)ATTR_UNIT, realloc);
    if (rc != 0 && rc != 1) attr_error(rc);
    ATTRS.n = (int)n; ATTRS.cap = (int)cap; out->n = (int)in;
    if (rc == 1) attr_publish();
    return rc;
}
#else
static void attr_put(int c) {      /* bput is only in the runtime-library build */
    if (ATTRS.n >= ATTRS.cap) { ATTRS.cap = ATTRS.cap ? ATTRS.cap * 2 : 256; ATTRS.b = xrealloc(ATTRS.b, ATTRS.cap); }
    ATTRS.b[ATTRS.n++] = (unsigned char)c;
}
static unsigned attr_u32(const unsigned char *p) { return p[0] | p[1] << 8 | p[2] << 16 | (unsigned)p[3] << 24; }
static int attr_strip(Buf *out) {
    if (out->n < 16 || memcmp(out->b, "USLATTR1", 8)) return 0;
    unsigned count = attr_u32(out->b + 8);
    long head = 16 + 5L * count;
    if (count > 1000000u || head > out->n || attr_u32(out->b + 12) != (unsigned long)(out->n - head)) die("malformed USLATTR1 side-car");
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
#endif
#ifdef UNISA_CX_ROUTESPAN
typedef struct { int ready, ended, count, stop; } RsCursor;
void rs_reset(RsCursor *c, int first_absent, int last_absent);
int rs_take(RsCursor *c, const char *stage_route, const char *stage_name, const char *route, const char *first, const char *last);
int rs_ok(const RsCursor *c);
#endif
#ifdef UNISA_CX_ROUTESTEP
int rt_open(int ver, int cached, const unsigned char *plain, int plain_len,
            unsigned char *owned, int raw_len, unsigned expect,
            int (*inflate)(unsigned char *, unsigned long, const unsigned char *, unsigned long),
            unsigned (*crc32)(char *, int),
            int *use_owned, int *out_len, char *why, long cap);
void rt_swap(unsigned char **in_b, int *in_n, unsigned char *out_b, int out_n, unsigned char **release);
int rt_after(int rc, int stop);
int rt_strip(int rc);
int sh_apply(int opened, int use_owned, int out_len, int has_slot,
             const char *why, void *slot,
             unsigned char *plain, int plain_len, unsigned char *owned,
             void (*fail)(const char *),
             void (*bindfn)(void *),
             void (*clear)(void),
             void (*set_loading)(int),
             void (*load)(unsigned char *, int),
             void (*release)(void *),
             void (*clear_lb)(void),
             void (*capture)(void *),
             void (*mark)(void *));
static void sh_fail(const char *s) { die(s); }
static void sh_bind(void *slot) { model_bind((const Model *)slot); }
static void sh_clear(void) { model_clear(); }
static void sh_loading(int v) { MODEL_LOADING = v; }
static void sh_load(unsigned char *b, int n) { loadbytes(b, n); }
static void sh_release(void *p) { free(p); }
static void sh_lb(void) { LB = 0; }
static void sh_capture(void *slot) { model_capture((Model *)slot); }
static void sh_mark(void *slot) { ((Model *)slot)->loaded = 1; }
#endif
#ifdef UNISA_CX_ROUTETIME
long long rr_ns(long long a_sec, long long a_nsec, long long b_sec, long long b_nsec);
int rr_wanted(const char *value);
int rr_model(const char *stage, int cached, long long ns, char *out, long cap);
int rr_infer(const char *stage, int rc, long long ns, char *out, long cap);
#endif
static int runroute_range(const char *route, const char *first_stage, const char *last_stage, Buf *in, const char *src) {
#ifdef UNISA_CX_ROUTESPAN
    RsCursor span;
    rs_reset(&span, first_stage == 0, last_stage == 0);
#else
    int count = 0, ready = first_stage == 0, ended = last_stage == 0;
#endif
    for (int i = 0; i < PS; i++) {
#ifdef UNISA_CX_ROUTESPAN
        if (!rs_take(&span, STAGES[i].route, STAGES[i].name, route, first_stage, last_stage)) continue;
#else
        if (strcmp(STAGES[i].route, route)) continue;
        if (!ready) {
            if (strcmp(STAGES[i].name,first_stage)) continue;
            ready=1;
        }
#endif
        int m = STAGES[i].model;
#ifdef UNISA_CX_PACKAGE_LIFETIME
        Model *slot = cxpl_slot(CACHE, m, CACHE ? CACHE->nmodels : 0) ? &CACHE->models[m] : 0;
#else
        Model *slot = CACHE && m < CACHE->nmodels ? &CACHE->models[m] : 0;
#endif
        /* UNISACC_RR_TIME phase stamps (stderr, same family as the driver's RR_TIME lines): the model
         * decode of this stage (inflate/CRC/parse, or a cache bind) and its inference, both measured
         * inside this process with the monotonic clock; nothing is printed without the variable. */
        static int rr_stamp = -1; struct timespec rr_a, rr_b, rr_c; int rr_cached = slot && slot->loaded;
#ifdef UNISA_CX_ROUTETIME
        if (rr_stamp < 0) rr_stamp = rr_wanted(getenv("UNISACC_RR_TIME"));
#else
        if (rr_stamp < 0) rr_stamp = getenv("UNISACC_RR_TIME") != 0;
#endif
        if (rr_stamp) clock_gettime(CLOCK_MONOTONIC, &rr_a);
#ifdef UNISA_CX_ROUTESTEP
        {
            int cached = slot && slot->loaded;
            unsigned char *bytes = 0, *owned = 0;
            int len = 0, use_owned = 0, out_len = 0, opened;
            char why[80];
            if (!cached) { bytes = PB + POFF[m]; len = PLEN[m]; }
            if (!cached && PVER == 3) owned = xrealloc(0, (size_t)PRAW[m]);
            opened = rt_open(PVER, cached, bytes, len, owned,
                             (!cached && PVER == 3) ? PRAW[m] : 0,
                             (!cached && PVER == 3) ? PCRC[m] : 0,
                             unisa_inflate, crc, &use_owned, &out_len, why, (long)sizeof why);
            if (sh_apply(opened, use_owned, out_len, slot != 0, why, slot,
                         bytes, len, owned,
                         sh_fail, sh_bind, sh_clear, sh_loading,
                         sh_load, sh_release, sh_lb, sh_capture, sh_mark) == 3)
                die("slot handoff");
        }
#else
        if (slot && slot->loaded) model_bind(slot);
        else {
            unsigned char *bytes = PB+POFF[m], *owned = 0; int len = PLEN[m];
            if (PVER == 3) {
                owned = xrealloc(0, PRAW[m]);
                if (unisa_inflate(owned, PRAW[m], bytes, len)) die("invalid compressed model");
                if (crc((char *)owned, PRAW[m]) != PCRC[m]) die("compressed model CRC");
                if (PRAW[m] < 9 || memcmp(owned,"UNINETB1N",9)) die("package requires binary networks");
                bytes = owned; len = PRAW[m];
            }
            model_clear(); MODEL_LOADING = 1;
            loadbytes(bytes, len); free(owned); LB = 0;
            if (slot) { model_capture(slot); slot->loaded = 1; }
            MODEL_LOADING = 0;
        }
#endif
        if (rr_stamp) {
            clock_gettime(CLOCK_MONOTONIC, &rr_b);
#ifdef UNISA_CX_ROUTETIME
            {
                long long ns = rr_ns((long long)rr_a.tv_sec, (long long)rr_a.tv_nsec, (long long)rr_b.tv_sec, (long long)rr_b.tv_nsec);
                char line[1024];
                if (rr_model(STAGES[i].name, rr_cached, ns, line, (long)sizeof line) == 0) fputs(line, stderr);
                else fprintf(stderr, "RR_TIME\tmodel\tstage=%s\tsource=%s\tns=%lld\n", STAGES[i].name, rr_cached ? "cache" : "inflate", ns);
            }
#else
            fprintf(stderr, "RR_TIME\tmodel\tstage=%s\tsource=%s\tns=%lld\n", STAGES[i].name, rr_cached ? "cache" : "inflate",
                    (long long)(rr_b.tv_sec - rr_a.tv_sec) * 1000000000LL + (rr_b.tv_nsec - rr_a.tv_nsec));
#endif
        }
        Buf out = {0}; int rc = execute(in->b, in->n, src, &out);
        if (rr_stamp) {
            clock_gettime(CLOCK_MONOTONIC, &rr_c);
#ifdef UNISA_CX_ROUTETIME
            {
                long long ns = rr_ns((long long)rr_b.tv_sec, (long long)rr_b.tv_nsec, (long long)rr_c.tv_sec, (long long)rr_c.tv_nsec);
                char line[1024];
                if (rr_infer(STAGES[i].name, rc, ns, line, (long)sizeof line) == 0) fputs(line, stderr);
                else fprintf(stderr, "RR_TIME\tinference\tstage=%s\trc=%d\tns=%lld\n", STAGES[i].name, rc, ns);
            }
#else
            fprintf(stderr, "RR_TIME\tinference\tstage=%s\trc=%d\tns=%lld\n", STAGES[i].name, rc,
                    (long long)(rr_c.tv_sec - rr_b.tv_sec) * 1000000000LL + (rr_c.tv_nsec - rr_b.tv_nsec));
#endif
        }
#ifdef UNISA_CX_ROUTESTEP
        if (rt_strip(rc)) attr_strip(&out);
#else
        if (!rc) attr_strip(&out);
#endif
        if (!slot) unload();
#ifdef UNISA_CX_ROUTESTEP
        {
            unsigned char *release = 0;
            rt_swap(&in->b, &in->n, out.b, out.n, &release);
            free(release);
        }
#else
        free(in->b); in->b = out.b; in->n = out.n;
#endif
#ifdef UNISA_CX_ROUTESTEP
        {
            int stop_now = 0;
#ifdef UNISA_CX_ROUTESPAN
            stop_now = span.stop;
#endif
            int next = rt_after(rc, stop_now);
            if (next == 1) return rc;
            if (next == 2) break;
        }
#ifndef UNISA_CX_ROUTESPAN
        count++;
        if (last_stage && !strcmp(STAGES[i].name,last_stage)) { ended=1; break; }
#endif
#else
        if (rc) return rc;
#ifdef UNISA_CX_ROUTESPAN
        if (span.stop) break;
#else
        count++;
        if (last_stage && !strcmp(STAGES[i].name,last_stage)) { ended=1; break; }
#endif
#endif
    }
#ifdef UNISA_CX_ROUTESPAN
    if (!rs_ok(&span)) die("unknown package route or stage boundary");
#else
    if (!count || !ended) die("unknown package route or stage boundary");
#endif
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
    /* standalone options (leading, each takes a value): --package PRODUCT --ri-snapshot SNAP --upstream-stage LABEL --stage LABEL --capture-out OUT */
    int k=1,run_observe=0;const char *run_pkg=0,*run_snap=0,*run_ul=0,*run_stage=0,*run_out=0,*run_ledger=0;
    RF_TRACE_STARTED=0;
    while(k<argc){
        if(!strcmp(argv[k],"--observe-callables")){run_observe=1;k++;continue;}
        if(!strcmp(argv[k],"--strict-resource-flow")){RF_STRICT=1;k++;continue;}
        if(strcmp(argv[k],"--package")&&strcmp(argv[k],"--ri-snapshot")&&strcmp(argv[k],"--upstream-stage")&&strcmp(argv[k],"--stage")&&strcmp(argv[k],"--capture-out")&&strcmp(argv[k],"--resource-ledger"))break;
        if(k+1>=argc){fprintf(stderr,"run: %s needs a value\n",argv[k]);return 2;}
        if(!strcmp(argv[k],"--package"))run_pkg=argv[k+1];else if(!strcmp(argv[k],"--ri-snapshot"))run_snap=argv[k+1];
        else if(!strcmp(argv[k],"--upstream-stage"))run_ul=argv[k+1];else if(!strcmp(argv[k],"--stage"))run_stage=argv[k+1];
        else if(!strcmp(argv[k],"--resource-ledger"))run_ledger=argv[k+1];else run_out=argv[k+1];k+=2;
    }
    if(run_ledger){char err[512];rl_clear(&RF_LEDGER);RF_LOADED=0;if(rl_load(run_ledger,&RF_LEDGER,err,sizeof err)){fprintf(stderr,"run: %s\n",err);return 2;}RF_LOADED=1;}
    if(RF_STRICT&&(!RF_LOADED||!run_stage||run_snap)){fprintf(stderr,"run: INTERFACE_ERROR strict-replay-needs-ledger-stage-and-no-overlay\n");return 2;}
    if(run_ledger&&!RF_STRICT){fprintf(stderr,"run: INTERFACE_ERROR ledger-needs-strict-resource-flow\n");return 2;}
    if (k > 1) {   /* options were given: the remaining argv is copied into a heap array; without options argv is untouched */
        char **nargv = malloc(sizeof(char *) * (size_t)(argc - k + 2)); int m = 1;
        if (!nargv) { fprintf(stderr, "run: out of memory\n"); return 2; }
        nargv[0] = argv[0]; for (; k < argc; k++) nargv[m++] = argv[k];
        argc = m; argv = nargv;
    }
    if (argc == 4 && !strcmp(argv[1], "--check-net")) { if(RF_STRICT){fprintf(stderr,"run: INTERFACE_ERROR strict-replay-needs-execution-input\n");return 2;}return checknet(argv[2], argv[3]); }
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
    static const unsigned char f1one = 1; static ResourceInput sres[3];
    sres[0].name = (const unsigned char *)"\0cli/f1"; sres[0].n = 7; sres[0].data = &f1one; sres[0].len = 1;
    sres[1].name = (const unsigned char *)"\0cli/attributes"; sres[1].n = 15; sres[1].data = 0; sres[1].len = 0;
    sres[2].name = (const unsigned char *)"\0cli/source"; sres[2].n = 11; sres[2].data = (const unsigned char *)src; sres[2].len = strlen(src);
    /* --ri-snapshot: the snapshot is the producer's complete RI, so the standalone built-ins (cli/f1, cli/attributes, cli/source)
       are not added; a key the snapshot lacks answers as the API does (default fetch, absent).  Without a snapshot: unchanged. */
    if (!RI && !run_snap) { RI = sres; NRI = 3; ATTR_SLOT = 1; }
    { const char *ap = getenv("UNISA_ATTRIBUTES");
#ifdef UNISA_CX_FILEMODE
      if (bx_env_set(ap))
#else
      if (ap && *ap)
#endif
      { int an = 0; unsigned char *ab = readfile(ap, &an, 1);
        if (ab) { ATTRS.b = ab; ATTRS.n = an; ATTRS.cap = an; sres[1].data = ab; sres[1].len = an; } } }
    Buf in = {0}; in.b = readfile(argv[inputarg], &in.n, 0);
    if (run_pkg) package(run_pkg);   /* PRODUCT resources stay loaded for this process; MODEL is loaded below */
    if (run_snap) {   /* overlay: the snapshot binds to the consumer's actual INPUT and to its own input.bin + COMMIT */
        char err[512];
        if (!run_ul) { fprintf(stderr, "run: --ri-snapshot needs --upstream-stage\n"); return 2; }
        if (rf_snapshot_load_ri(run_snap, run_ul, &RI_SNAP, run_observe, err, sizeof err)) {   /* the upstream identity is verified inside the snapshot (header vs its own input.bin); the consumer INPUT is its own identity and is not compared with it */ fprintf(stderr, "run: %s\n", err); return 2; }
        RI_SNAP_ON = 1;
    }
    if(RF_STRICT){char err[512];if(rl_check_identity(&RF_LEDGER,run_stage,in.b,in.n,err,sizeof err)){fprintf(stderr,"run: %s\n",err);return 2;}RF_LEDGER.cursor=0;RF_TRACE_STARTED=1;}
    if(!run_out&&rf_capture_dir())run_out=rf_capture_dir();
    if (run_out) {   /* consumer capture: a new (empty) directory, distinct from the snapshot, with the consumer's own header */
        char err[512];
        if (!run_stage) { fprintf(stderr, "run: --capture-out needs --stage\n"); return 2; }
#if !defined(_WIN32)
        /* Same platform contract as rl_snapshot_load: unsupported builds have
           already returned its named refusal before reaching this guard. */
        { struct stat sa, sb;
        if (run_snap && !stat(run_snap, &sa) && !stat(run_out, &sb) && sa.st_dev == sb.st_dev && sa.st_ino == sb.st_ino) { fprintf(stderr, "run: INTERFACE_ERROR snapshot-output-same-path\n"); return 2; }
        }
#endif
        if (rf_capture_begin(run_out, run_stage, in.b, in.n, err, sizeof err)) { fprintf(stderr, "run: %s\n", err); return 2; }
        rf_capture_set = run_out;
    }
    int run_attributes = 0; /* Attribute output commits only after the whole chain accepts. */
    if (bundled) {
        int rc = runroute(argv[routearg], &in, src); unpackage();
        if (rc) return rc;
    } else {
        int first = chain ? 5 : 1, end = chain ? argc : 2;
        for (int i = first; i < end; i++) {
            load(argv[i]);
            Buf out = {0}; int rc = execute(in.b, in.n, src, &out);
            if (!rc && attr_strip(&out)) run_attributes = 1;
            unload(); free(in.b);
            if (rc) return rc;
            in.b = out.b; in.n = out.n;
        }
    }
    if(rf_trace_finish())return 2;
    if (run_attributes) {
        const char *ap = getenv("UNISA_ATTRIBUTES");
#ifdef UNISA_CX_FILEMODE
        if (bx_env_set(ap)) {
            FILE *af = fopen(ap, bx_file_mode(1)); int bad = !af;
#else
        if (ap && *ap) {
            FILE *af = fopen(ap, "wb"); int bad = !af;
#endif
            if (af) { if (fwrite(ATTRS.b,1,ATTRS.n,af)!=(size_t)ATTRS.n) bad=1; if(fclose(af)) bad=1; }
            if(bad)die("cannot write UNISA_ATTRIBUTES");
        }
    }
    if (fwrite(in.b, 1, in.n, stdout) != (size_t)in.n || fclose(stdout)) die("cannot write output");
    free(in.b); return 0;
}

#endif
