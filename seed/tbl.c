/* seed/tbl.c -- exec/c/tbl.py in C99: a delta (JSON) as the flat integer table exec/c/run.c reads.
 * 0.0.25 self-hosting road B2.  Built by the previous unisacc.com (tests/seedconstructmatrix.py) and
 * byte-compared with the Python original on every product delta.
 *
 *   seed-tbl delta.json out.tbl
 *
 * Every name becomes a number in first-use order, exactly as tbl.py numbers them: states in the
 * delta's order (plus row targets that have no row), registers, strings and extra stack symbols as
 * the action sequences first mention them.  The opcode list is exec/c/core.h's (CORE_ACTION_ARITIES
 * and the enum order); tbl.py cross-checks the same list against core.h.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "json.h"

static const char *OPN[] = {"ADV","MARK","JUMP","LDI","COPYW","ALU","ALUI","CMP","CMPI","RLD","LDX","STX",
    "OUT","OUTW","COPY","COPYT","SPAN","SPANT","SPAN2","OLAST","ODROP","OLEN","OCUT","ORES","OFILL","OCLR",
    "OSEL","SETOT","XATTR","PUSH","POP","INTERN","BLOBSAVE","INPUSH","INPUSHX","INPUSHXE","INPOP","SBCLR",
    "SBOUT","SBSPAN","SBBLOB","SBINTERN","SBSAVE","SBFIND","BLEN","BYTE","XLEN","DIVMOD10","SWAP","ACCEPT",
    "REJECT","A64","A64I","C64","C64U","INC"};
static const char *OPK[] = {"","r","r","ri","rr","arrr","arri","rr","ri","r","rri","rir","i","r","","","r","r",
    "rr","","","r","rr","ri","rri","","i","r","r","g","","rrr","rrr","r","r","rr","","","i","rr","r","r","r","r",
    "rr","r","r","r","","","s","arrr","arri","rr","rr","r"};
#define NOPS ((int)(sizeof(OPN) / sizeof(OPN[0])))
static const char *ALUOPS[] = {"add","sub","mul","div","rem","and","or","xor","shl","sar","sdiv","srem","udiv","urem","not","shr"};

/* an insertion-ordered set of byte strings: index by first insertion */
typedef struct { char **s; long *len; long n, cap; long *tab; long tcap; } Set;
static unsigned long hsh(const char *p, long n) { unsigned long h = 1469598103UL; long k; for (k = 0; k < n; k++) h = (h ^ (unsigned char)p[k]) * 16777619UL; return h; }
static void die(const char *m) { fprintf(stderr, "seed-tbl: %s\n", m); exit(2); }
static long sfind(Set *t, const char *p, long n) {
    unsigned long h; long k;
    if (!t->tcap) return -1;
    h = hsh(p, n) & (unsigned long)(t->tcap - 1);
    for (;;) {
        k = t->tab[h];
        if (k < 0) return -1;
        if (t->len[k] == n && memcmp(t->s[k], p, (size_t)n) == 0) return k;
        h = (h + 1) & (unsigned long)(t->tcap - 1);
    }
}
static void sgrow(Set *t) {
    long c = t->tcap ? t->tcap * 2 : 1024, k;
    long *tab = (long *)malloc((size_t)c * sizeof(long));
    if (!tab) die("out of memory");
    for (k = 0; k < c; k++) tab[k] = -1;
    for (k = 0; k < t->n; k++) {
        unsigned long h = hsh(t->s[k], t->len[k]) & (unsigned long)(c - 1);
        while (tab[h] >= 0) h = (h + 1) & (unsigned long)(c - 1);
        tab[h] = k;
    }
    free(t->tab); t->tab = tab; t->tcap = c;
}
static long sadd(Set *t, const char *p, long n) {
    long k = sfind(t, p, n);
    unsigned long h;
    if (k >= 0) return k;
    if ((t->n + 1) * 2 > t->tcap) sgrow(t);
    if (t->n == t->cap) {
        long c = t->cap ? t->cap * 2 : 1024;
        t->s = (char **)realloc(t->s, (size_t)c * sizeof(char *));
        t->len = (long *)realloc(t->len, (size_t)c * sizeof(long));
        if (!t->s || !t->len) die("out of memory");
        t->cap = c;
    }
    t->s[t->n] = (char *)malloc((size_t)(n + 1));
    if (!t->s[t->n]) die("out of memory");
    memcpy(t->s[t->n], p, (size_t)n); t->s[t->n][n] = 0; t->len[t->n] = n;
    h = hsh(p, n) & (unsigned long)(t->tcap - 1);
    while (t->tab[h] >= 0) h = (h + 1) & (unsigned long)(t->tcap - 1);
    t->tab[h] = t->n;
    return t->n++;
}

static JDoc D;
static Set names, regs, strs, syms;
/* a JSON value as a dictionary key the way Python sees it: strings and ints never collide */
static long vkey(long v, char *buf, long cap) {
    JNode *n = &D.n[v];
    if (n->type == JSTR) {
        if (n->slen + 1 > cap) die("name too long");
        buf[0] = 's'; memcpy(buf + 1, D.buf + n->str, (size_t)n->slen); return n->slen + 1;
    }
    if (n->type == JINT) return sprintf(buf, "i%lld", n->ival);
    die("register, symbol or string must be a string or an integer");
    return 0;
}
static char kb[1 << 16];
static long state_ix(const char *p, long n) { return sfind(&names, p, n); }
static long sym(long v) {
    JNode *n = &D.n[v];
    long k, l;
    if (n->type == JSTR && (k = state_ix(D.buf + n->str, n->slen)) >= 0) return k;
    l = vkey(v, kb, sizeof kb);
    return names.n + sadd(&syms, kb, l);
}

typedef struct { long next, seq; } Ent;
static int entcmp(Ent a, Ent b) { return a.next != b.next ? (a.next < b.next ? -1 : 1) : a.seq != b.seq ? (a.seq < b.seq ? -1 : 1) : 0; }
static int entqs(const void *x, const void *y) { return entcmp(*(const Ent *)x, *(const Ent *)y); }

static char *out; static long olen, ocap;
static void emit(const char *p, long n) {
    if (olen + n > ocap) {
        long c = ocap ? ocap * 2 : 1 << 20;
        while (c < olen + n) c *= 2;
        out = (char *)realloc(out, (size_t)c);
        if (!out) die("out of memory");
        ocap = c;
    }
    memcpy(out + olen, p, (size_t)n); olen += n;
}
static void emits(const char *s) { emit(s, (long)strlen(s)); }
static void emitl(long v) { char b[32]; emit(b, sprintf(b, "%ld", v)); }

int main(int argc, char **argv) {
    long root, st, sq, k, nseq = 0;
    char **qline; long *qlen;
    FILE *f;
    if (argc != 3) { fprintf(stderr, "usage: seed-tbl delta.json out.tbl\n"); return 2; }
    jparse_file(&D, argv[1]);
    root = 0;
    st = jget(&D, root, "states"); sq = jget(&D, root, "seqs");
    if (st < 0 || sq < 0) die("delta needs states and seqs");
    for (k = jkid(&D, st); k >= 0; k = jnext(&D, k)) sadd(&names, D.buf + D.n[k].key, D.n[k].klen);
    for (k = jkid(&D, st); k >= 0; k = jnext(&D, k)) {
        long row = jnext(&D, jkid(&D, k)), e;
        for (e = jkid(&D, row); e >= 0; e = jnext(&D, e)) {
            long nx = jkid(&D, e);
            if (D.n[nx].type != JSTR) die("row target must be a state name");
            sadd(&names, D.buf + D.n[nx].str, D.n[nx].slen);   /* a target with no row gets an empty one */
        }
    }
    /* action sequences, numbering registers, strings and extra symbols on first use */
    for (k = jkid(&D, sq); k >= 0; k = jnext(&D, k)) nseq++;
    qline = (char **)calloc((size_t)(nseq + 1), sizeof(char *)); qlen = (long *)calloc((size_t)(nseq + 1), sizeof(long));
    if (!qline || !qlen) die("out of memory");
    {
        long i = 0, s;
        for (s = jkid(&D, sq); s >= 0; s = jnext(&D, s), i++) {
            long a, cnt = 0, save = olen;
            for (a = jkid(&D, s); a >= 0; a = jnext(&D, a)) cnt++;
            emits("Q "); emitl(cnt);
            for (a = jkid(&D, s); a >= 0; a = jnext(&D, a)) {
                long op = jkid(&D, a), code, arg; const char *kinds; int j;
                for (code = 0; code < NOPS; code++) if (jstris(&D, op, OPN[code])) break;
                if (code == NOPS) die("unknown action");
                kinds = OPK[code];
                emits(" "); emitl(code);
                for (j = 0, arg = jnext(&D, op); kinds[j]; j++, arg = jnext(&D, arg)) {
                    long v;
                    if (arg < 0) die("too few action arguments");
                    switch (kinds[j]) {
                    case 'r': v = sadd(&regs, kb, vkey(arg, kb, sizeof kb)); break;
                    case 'g': v = sym(arg); break;
                    case 's':
                        if (D.n[arg].type == JINT) { long l = sprintf(kb, "%lld", D.n[arg].ival); v = sadd(&strs, kb, l); }
                        else if (D.n[arg].type == JSTR) v = sadd(&strs, D.buf + D.n[arg].str, D.n[arg].slen);
                        else { die("bad string argument"); v = 0; }
                        break;
                    case 'a': {
                        int x;
                        for (x = 0; x < 16; x++) if (jstris(&D, arg, ALUOPS[x])) break;
                        if (x == 16) die("unknown alu op");
                        v = x; break;
                    }
                    default:
                        if (D.n[arg].type == JINT) v = (long)D.n[arg].ival;
                        else if (D.n[arg].type == JSTR) { memcpy(kb, D.buf + D.n[arg].str, (size_t)D.n[arg].slen); kb[D.n[arg].slen] = 0; v = atol(kb); }
                        else { die("bad immediate"); v = 0; }
                    }
                    emits(" "); emitl(v);
                }
                if (arg >= 0) die("too many action arguments");
            }
            if (!cnt) emits(" ");                /* "Q 0 " as tbl.py writes an empty join */
            emits("\n");
            qlen[i] = olen - save;
            qline[i] = (char *)malloc((size_t)qlen[i]);
            if (!qline[i]) die("out of memory");
            memcpy(qline[i], out + save, (size_t)qlen[i]);
            olen = save;
        }
    }
    /* rows: computed before writing, since t rows may still number new stack symbols */
    {
        char **rline = (char **)calloc((size_t)names.n + 1, sizeof(char *)); long *rlen = (long *)calloc((size_t)names.n + 1, sizeof(long));
        long ni, nst = names.n, nrow = 0, *rownode = (long *)malloc((size_t)(names.n + 1) * sizeof(long)), e0;
        if (!rownode) die("out of memory");
        for (e0 = jkid(&D, st); e0 >= 0; e0 = jnext(&D, e0)) rownode[nrow++] = e0;
        Ent full[257];
        if (!rline || !rlen) die("out of memory");
        for (ni = 0; ni < nst; ni++) {
            long sv = -1, row = -1, save = olen, m = 0, ents = 0, e;
            Ent dflt; dflt.next = -1; dflt.seq = 0;
            if (ni < nrow) sv = rownode[ni];      /* names 0..nrow-1 are the delta's states, in order */
            if (sv >= 0) {
                long mode = jkid(&D, sv);
                row = jnext(&D, mode);
                m = jstris(&D, mode, "b") ? 0 : jstris(&D, mode, "t") ? 1 : jstris(&D, mode, "r") ? 2 : (die("bad mode"), 0);
            }
            if (m != 1) {
                long cnt[257]; long x, best = -1;
                for (x = 0; x < 257; x++) { full[x].next = -1; full[x].seq = 0; }
                if (row >= 0) for (e = jkid(&D, row); e >= 0; e = jnext(&D, e)) {
                    long key, nx = jkid(&D, e), sqi = jnext(&D, nx);
                    memcpy(kb, D.buf + D.n[e].key, (size_t)D.n[e].klen); kb[D.n[e].klen] = 0;
                    key = atol(kb);
                    if (key < 0 || key > 256) continue;
                    full[key].next = state_ix(D.buf + D.n[nx].str, D.n[nx].slen);
                    full[key].seq = (long)D.n[sqi].ival;
                }
                /* the most frequent (next, seq); among equals the smallest, as max(sorted(cnt), key=cnt.get) */
                {   Ent srt[257]; long run = 0, bestn = 0;
                    memcpy(srt, full, sizeof srt); qsort(srt, 257, sizeof(Ent), entqs);
                    for (x = 0; x < 257; x++) {        /* ascending runs: the first longest run is the smallest of the most frequent */
                        run = (x && !entcmp(srt[x], srt[x - 1])) ? run + 1 : 1;
                        if (run > bestn) { bestn = run; best = x; }
                    }
                    dflt = srt[best]; (void)cnt; }
                for (x = 0; x < 257; x++) if (entcmp(full[x], dflt)) ents++;
                emits("R "); emitl(m); emits(" "); emitl(ents); emits(" "); emitl(dflt.next); emits(" "); emitl(dflt.seq); emits(" ");
                for (x = 0, e = 0; x < 257; x++) if (entcmp(full[x], dflt)) {
                    if (e++) emits(" ");
                    emitl(x); emits(" "); emitl(full[x].next); emits(" "); emitl(full[x].seq);
                }
            } else {
                for (e = jkid(&D, row); e >= 0; e = jnext(&D, e)) ents++;
                emits("R 1 "); emitl(ents); emits(" -1 0 ");
                for (e = jkid(&D, row); e >= 0; e = jnext(&D, e)) {
                    long nx = jkid(&D, e), sqi = jnext(&D, nx), key;
                    if (D.n[e].klen == 3 && memcmp(D.buf + D.n[e].key, "BOT", 3) == 0) key = -1;
                    else { long l = D.n[e].klen; kb[0] = 's'; memcpy(kb + 1, D.buf + D.n[e].key, (size_t)l);
                           key = state_ix(D.buf + D.n[e].key, l); if (key < 0) key = names.n + sadd(&syms, kb, l + 1); }
                    if (e != jkid(&D, row)) emits(" ");
                    emitl(key); emits(" "); emitl(state_ix(D.buf + D.n[nx].str, D.n[nx].slen)); emits(" "); emitl((long)D.n[sqi].ival);
                }
            }
            emits("\n");
            rlen[ni] = olen - save; rline[ni] = (char *)malloc((size_t)rlen[ni]);
            if (!rline[ni]) die("out of memory");
            memcpy(rline[ni], out + save, (size_t)rlen[ni]); olen = save;
        }
        {
            long start = jget(&D, root, "start"), six;
            six = start >= 0 ? state_ix(D.buf + D.n[start].str, D.n[start].slen) : state_ix("DISPATCH", 8);
            emits("T "); emitl(names.n); emits(" "); emitl(nseq); emits(" "); emitl(regs.n); emits(" "); emitl(strs.n); emits(" "); emitl(six); emits("\n");
        }
        for (k = 0; k < strs.n; k++) {
            long j;
            emits("S ");
            if (!strs.len[k]) emits("-");
            for (j = 0; j < strs.len[k]; j++) { char b[3]; sprintf(b, "%02x", (unsigned char)strs.s[k][j]); emit(b, 2); }
            emits("\n");
        }
        for (k = 0; k < nseq; k++) emit(qline[k], qlen[k]);
        for (k = 0; k < nst; k++) emit(rline[k], rlen[k]);
    }
    f = fopen(argv[2], "wb");
    if (!f || fwrite(out, 1, (size_t)olen, f) != (size_t)olen || fclose(f)) die("cannot write the table");
    return 0;
}
