/* unisacc -- the compiler itself, in the C subset it compiles.
 *
 * Every table decision goes through infer(), the same integer kernel and the
 * same model blob the Python driver uses.  Classic code (this file) walks; the
 * tables decide.  [T-1] [T-2]
 *
 * Stage 1: the lexer.  `unisacc -tokens f.c` prints the token stream.
 */

#define MAXSRC 4194304   /* 4 MB: unisacc.c itself had reached 1,042,362 of the old 1 MB */
#define MAXTOK 2097152   /* about one token per two bytes of the 4 MB MAXSRC */

char src[MAXSRC];
int nsrc;
int inf(int st, int *key, int head);

int tkind[MAXTOK];      /* index into TOKV */
int tpos[MAXTOK];       /* offset of the token text in src */
int tlen[MAXTOK];
int ntok;

char tbuf[256];

/* ---- vocabulary helpers: TOKV etc are NUL-separated packed strings ---- */
int vfind(char *v, int n, char *s, int slen);

/* A vocabulary is a run of NUL-separated names, and `voff`/`vlen` used to
   walk it from the front on every call -- which the lexer does per token.
   A sampling profile of the self-compile found the strlen that inner loop
   compiles to at the top, so each vocabulary is indexed ONCE, on first
   use.  There are few enough of them to find by pointer. */
#define V_NLISTS 64      /* 16 was fewer than the vocabularies: the rest walked */
#define V_NIDX 1024
char *v_lists[V_NLISTS]; int v_idx[V_NLISTS * V_NIDX]; int v_n[V_NLISTS];
int v_nlists;

char *v_last; int v_lastslot;  /* the lexer asks the same list back to back */
/* the list pointer -> slot, remembered in a small direct-mapped cache:
   callers alternate between vocabularies, so "the last one" alone missed,
   and the walk below was the self-compile's top line at -O2 */
char *vc_ptr[16]; int vc_slot[16];
int v_slot(char *v) {
    int i; int p; int n; int h;
    if (v == v_last) return v_lastslot;
    h = (int)(((long)v >> 3) & 15);
    if (vc_ptr[h] == v) { v_last = v; v_lastslot = vc_slot[h]; return vc_slot[h]; }
    i = 0;
    while (i < v_nlists) {
        if (v_lists[i] == v) { v_last = v; v_lastslot = i; vc_ptr[h] = v; vc_slot[h] = i; return i; }
        i = i + 1;
    }
    if (v_nlists >= V_NLISTS) return 0 - 1;     /* fall back to the walk */
    i = v_nlists; v_nlists = v_nlists + 1;
    v_lists[i] = v;
    p = 0; n = 0;
    while (n < V_NIDX) {
        v_idx[i * V_NIDX + n] = p; n = n + 1;
        while (v[p]) p = p + 1;
        p = p + 1;
        if (v[p] == 0) break;
    }
    v_n[i] = n;
    v_last = v; v_lastslot = i;
    return i;
}

int voff(char *v, int idx) {
    int s; int p; int i;
    s = v_slot(v);
    if (s >= 0 && idx >= 0 && idx < v_n[s]) return v_idx[s * V_NIDX + idx];
    p = 0; i = 0;
    while (i < idx) { while (v[p]) p = p + 1; p = p + 1; i = i + 1; }
    return p;
}

/* vfind by hash.  The walk above was 44% of the self-built compiler
   compiling itself: the lexer asks it for every identifier, and the parser
   for every `tidx(",", 1)`.  Each vocabulary gets a table on first use;
   a name maps to its FIRST index, which is what the walk returns. */
#define VH_SIZE 4096
int vh_tab[V_NLISTS * VH_SIZE]; int vh_built[V_NLISTS];
int vhash(char *s, int n) {
    int h; int k;
    h = n; k = 0;
    while (k < n) { h = (h * 31 + (s[k] & 255)) & 16777215; k = k + 1; }
    return h & (VH_SIZE - 1);
}
int vsame(char *v, int p, char *s, int slen) {
    int k;
    k = 0;
    while (k < slen) { if (v[p + k] != s[k]) return 0; if (v[p + k] == 0) return 0; k = k + 1; }
    return v[p + k] == 0;
}
int vfind_walk(char *v, int n, char *s, int slen) {
    int i; int p; int k; int ok;
    p = 0;
    i = 0;
    while (i < n) {
        k = 0;
        ok = 1;
        while (v[p + k]) {
            if (k >= slen) { ok = 0; }
            if (ok) { if (v[p + k] != s[k]) ok = 0; }
            k = k + 1;
        }
        if (ok) { if (k == slen) return i; }
        p = p + k + 1;
        i = i + 1;
    }
    return 0 - 1;
}
/* Most calls name a literal -- `tidx(",", 1)`, 313 of them -- so the same
   (list, pointer, length) comes back again and again.  A hit is only a
   guess about the pointer's CONTENT, which can change (the lexer passes
   src + i), so it is re-checked; only found names are kept, and a name's
   first index is still its first index when the same bytes come back. */
#define VC_SIZE 1024
char *vc_s[VC_SIZE]; char *vc_v[VC_SIZE]; int vc_l[VC_SIZE]; int vc_r[VC_SIZE];
int vfind(char *v, int n, char *s, int slen) {
    int sl; int i; int h; int e; int b; int L; int c;
    sl = v_slot(v);
    if (sl < 0) return vfind_walk(v, n, s, slen);
    if (v_n[sl] < n) return vfind_walk(v, n, s, slen);
    c = ((int)((long)s & 1048575) * 7 + slen) & (VC_SIZE - 1);
    if (vc_s[c] == s) { if (vc_v[c] == v) { if (vc_l[c] == slen) {
        e = vc_r[c];
        if (vsame(v, v_idx[sl * V_NIDX + e], s, slen)) { if (e < n) return e; return 0 - 1; }
    } } }
    b = sl * VH_SIZE;
    if (vh_built[sl] == 0) {
        vh_built[sl] = 1;
        i = 0;
        while (i < v_n[sl]) {
            L = 0; while (v[v_idx[sl * V_NIDX + i] + L]) L = L + 1;
            h = vhash(v + v_idx[sl * V_NIDX + i], L);
            while (vh_tab[b + h]) {
                e = vh_tab[b + h] - 1;
                if (vsame(v, v_idx[sl * V_NIDX + e], v + v_idx[sl * V_NIDX + i], L)) break;
                h = (h + 1) & (VH_SIZE - 1);
            }
            if (vh_tab[b + h] == 0) vh_tab[b + h] = i + 1;
            i = i + 1;
        }
    }
    h = vhash(s, slen);
    while (vh_tab[b + h]) {
        e = vh_tab[b + h] - 1;
        if (vsame(v, v_idx[sl * V_NIDX + e], s, slen)) {
            vc_s[c] = s; vc_v[c] = v; vc_l[c] = slen; vc_r[c] = e;
            if (e < n) return e;
            return 0 - 1;
        }
        h = (h + 1) & (VH_SIZE - 1);
    }
    return 0 - 1;
}

int vlen(char *v, int idx) {
    int p; int k;
    p = voff(v, idx);
    k = 0;
    while (v[p + k]) k = k + 1;
    return k;
}

/* ---- character predicates (needed by both pp and lex) ---------------- */
int isal(int c) {
    if (c >= 97) { if (c <= 122) return 1; }
    if (c >= 65) { if (c <= 90) return 1; }
    if (c == 95) return 1;
    return 0;
}
int isdi(int c) { if (c >= 48) { if (c <= 57) return 1; } return 0; }

/* ---- the preprocessor: the pp table decides every directive [W-1] ----- */
#define MAXMAC 2048
/* A macro lives from its #define to its #undef, and a redefinition replaces
   it from THAT point on (C99 6.10.3.5).  This preprocessor used to process
   every directive first and expand the text with the FINAL table
   afterwards -- `#define V 1`, `a = V`, `#undef V`, `#define V 2`, `b = V`
   gave a = b = 2 -- and #undef did nothing at all.  Now every directive
   that changes the table opens a new SEGMENT of the source, each entry
   knows the segments it is live in, [macfrom, macto), and expansion
   resolves a name in the segment of the text it is expanding.  A
   redefinition is a new entry; the old one is closed, not overwritten. */
#define SEGINF 1000000000
#define MAXSEG 65536
#define MACPOOL 131072
#define MAXMPARAM 12
char macname[MAXMAC * 32];
int macval[MAXMAC];      /* the numeric value, when the body is one number */
int machas[MAXMAC];      /* ...and whether it was */
/* The replacement list, as TEXT.  A numeric value is enough for `#if`, and
   for nothing else: `#define STR "x"` and `#define MAX(a,b) ...` are most of
   what real C does with the preprocessor. */
char macpool[MACPOOL]; int nmacpool;
int macboff[MAXMAC]; int macblen[MAXMAC];
int macfn[MAXMAC];       /* 1 when the name was followed by `(` with no space */
int macvar[MAXMAC];      /* 1 when the last parameter is `...` */
int macnp[MAXMAC];
int macpoff[MAXMAC * MAXMPARAM]; int macplen[MAXMAC * MAXMPARAM];
int nmac;
int macfrom[MAXMAC]; int macto[MAXMAC];   /* live in segments [from, to) */
int macprev[MAXMAC];     /* the previous entry with the same name, or -1 */
int curseg;              /* the table's segment while preprocessing */
int segpos[MAXSEG]; int nsegpos;   /* segment k+1 starts at segpos[k] */
int pp_seg = 0 - 1;      /* -1: resolve against the table as it stands now
                            (while preprocessing, and after); otherwise the
                            segment of the text being expanded */
int srcseg[MAXSRC];      /* the segment of each byte of src */
int eseg;                /* the segment of what eput is writing */
/* #pragma push_macro / pop_macro: the saved entry (or -1) per push */
#define MAXPUSH 256
char pushname[MAXPUSH * 32]; int pushent[MAXPUSH]; int npush;

int mfindt(int t);
/* Every identifier in the source is looked up here -- that is what the
   preprocessor DOES -- and the table holds hundreds of macros once the
   headers are in, so the linear scan was the front end's hottest loop on
   a sampling profile of the self-compile.
   An open-addressed index over the same table: the answer is the same
   entry, found without walking.  `mh_n` tracks how many entries the index
   has seen, so a table that grew (or was reset) is re-indexed rather than
   answered from a stale index. */
#define MACH 8192
int mac_h[MACH]; int mh_n = 0 - 1;

int mac_hash(char *s, int n) {
    int i; int h;
    h = n * 131;
    i = 0;
    while (i < n) { h = h * 31 + (s[i] & 255); i = i + 1; }
    h = h & (MACH - 1);
    if (h < 0) h = 0 - h;
    return h;
}

int mac_is(int i, char *s, int n) {     /* is entry i the name s[0..n)? */
    int k;
    k = 0;
    while (macname[i * 32 + k]) {
        if (k >= n) return 0;
        if (macname[i * 32 + k] != s[k]) return 0;
        k = k + 1;
    }
    return k == n;
}

int mac_reindex(void) {
    int i; int h;
    i = 0; while (i < MACH) { mac_h[i] = 0; i = i + 1; }
    i = 0;
    while (i < nmac) {
        { int L; L = 0; while (macname[i * 32 + L]) L = L + 1;
          h = mac_hash(macname + i * 32, L);
          /* a later entry of the same name takes the slot: the index
             holds the NEWEST, and macprev reaches the older ones */
          while (mac_h[h]) {
              if (mac_is(mac_h[h] - 1, macname + i * 32, L)) break;
              h = (h + 1) & (MACH - 1);
          } }
        mac_h[h] = i + 1;
        i = i + 1;
    }
    mh_n = nmac;
    return 0;
}

/* The newest entry spelled s[0..n), live or not, or -1. */
int mac_newest(char *s, int n) {
    int h;
    if (mh_n != nmac) mac_reindex();
    h = mac_hash(s, n);
    while (mac_h[h]) {
        if (mac_is(mac_h[h] - 1, s, n)) return mac_h[h] - 1;
        h = (h + 1) & (MACH - 1);
    }
    return 0 - 1;
}

int mfind(char *s, int n) {
    int m;
    m = mac_newest(s, n);
    if (pp_seg < 0) {                   /* now: only the newest can be live */
        if (m >= 0) { if (macto[m] < SEGINF) return 0 - 1; }
        return m;
    }
    while (m >= 0) {                    /* the entry live in pp_seg */
        if (macfrom[m] <= pp_seg && pp_seg < macto[m]) return m;
        m = macprev[m];
    }
    return 0 - 1;
}

int mdef(char *s, int n, int v, int has) {
    int k; int old;
    old = mac_newest(s, n);
    if (nmac >= MAXMAC) { __write(2, "too many macro definitions\n", 27); __exit(1); }
    /* the live definition ends where this one begins */
    if (old >= 0) { if (macto[old] >= SEGINF) macto[old] = curseg; }
    macfrom[nmac] = curseg; macto[nmac] = SEGINF; macprev[nmac] = old;
    k = 0;
    while (k < n) { if (k < 31) macname[nmac * 32 + k] = s[k]; k = k + 1; }
    if (n < 32) macname[nmac * 32 + n] = 0;
    macval[nmac] = v; machas[nmac] = has;
    macboff[nmac] = 0; macblen[nmac] = 0; macfn[nmac] = 0; macnp[nmac] = 0;
    macvar[nmac] = 0;
    nmac = nmac + 1;
    return 0;
}

/* Copy a C string into the macro pool and return its offset. */
int macstashs(char *t, int n) {
    int off; int k;
    off = nmacpool; k = 0;
    while (k < n) {
        if (nmacpool >= MACPOOL) { __write(2, "macro pool full\n", 16); __exit(1); }
        macpool[nmacpool] = t[k]; nmacpool = nmacpool + 1;
        k = k + 1;
    }
    return off;
}

/* Copy a stretch of source into the macro pool and return its offset. */
int macstash(int from, int to) {
    int off; int k;
    off = nmacpool;
    k = from;
    while (k < to) {
        if (nmacpool >= MACPOOL) { __write(2, "macro pool full\n", 16); __exit(1); }
        macpool[nmacpool] = src[k]; nmacpool = nmacpool + 1;
        k = k + 1;
    }
    return off;
}


/* ---- #if: a real integer constant expression ------------------------- */
/* C99 6.10.1: the controlling expression is macro-expanded FIRST -- after
   `defined X` has been replaced by 0 or 1 -- and then evaluated as an
   integer constant expression in intmax_t / uintmax_t.  The evaluator used
   to read the directive's own text and take a macro's value from the
   leading decimal digits of its body, so `#define X 0x10` made `#if X`
   false and `#define Y (1)` made `#if Y` 0.  Now the line is expanded by
   the same expander as the program text, into ebuf, and evaluated there. */
int wsat(int i);
char *ppb;           /* the expanded expression */
int ppp;             /* cursor into ppb */
int ppe;             /* one past its end */
int ppu;             /* the value just computed is unsigned (uintmax_t) */
int ppskip;          /* > 0: inside an operand that is not evaluated */
int ppdiv0;          /* a division by zero was evaluated */
int ppnow;           /* expanding an #if line: resolve names in the table as it stands */

long ppcond(void);
int srcis(int p, int L, char *nm);

int ppwsc(int c) { if (c == 32) return 1; if (c == 9) return 1; if (c == 10) return 1; return 0; }
void ppws(void) { while (ppp < ppe) { if (ppwsc(ppb[ppp] & 255) == 0) break; ppp = ppp + 1; } }

int ppop(char *w, int n) {           /* consume this operator if it is here */
    int k;
    ppws();
    if (ppp + n > ppe) return 0;
    k = 0;
    while (k < n) { if ((ppb[ppp + k] & 255) != (w[k] & 255)) return 0; k = k + 1; }
    ppp = ppp + n;
    return 1;
}

/* the next operator is exactly this one, not a longer one starting with it */
int ppop1(int c, int no1, int no2) {
    ppws();
    if (ppp >= ppe) return 0;
    if ((ppb[ppp] & 255) != c) return 0;
    if (ppp + 1 < ppe) { if ((ppb[ppp + 1] & 255) == no1) return 0;
                         if ((ppb[ppp + 1] & 255) == no2) return 0; }
    ppp = ppp + 1;
    return 1;
}

int pphexv(int c) {
    if (c >= 48 && c <= 57) return c - 48;
    if (c >= 97 && c <= 102) return c - 87;
    if (c >= 65 && c <= 70) return c - 55;
    return 0 - 1;
}

long ppnum(void) {                   /* a decimal, hex or octal constant */
    unsigned long v; int c; int base; int d; int big;
    v = 0; base = 10; big = 0;
    if ((ppb[ppp] & 255) == 48) {
        base = 8; ppp = ppp + 1;
        if (ppp < ppe) { c = ppb[ppp] & 255;
            if (c == 120 || c == 88) { base = 16; ppp = ppp + 1; } }
    }
    while (ppp < ppe) {
        d = pphexv(ppb[ppp] & 255);
        if (d < 0 || d >= base) break;
        v = v * base + d;
        ppp = ppp + 1;
    }
    ppu = 0;
    if ((v >> 63) != 0) ppu = 1;      /* does not fit intmax_t */
    while (ppp < ppe) { c = ppb[ppp] & 255;
        if (c == 117 || c == 85) { ppu = 1; ppp = ppp + 1; continue; }
        if (c == 108 || c == 76) { ppp = ppp + 1; continue; }
        break; }
    return v;
}

long ppchar(void) {                  /* ppb[ppp] is the opening quote */
    long v; int c; int d; int n;
    ppp = ppp + 1;
    v = 0;
    while (ppp < ppe) {
        c = ppb[ppp] & 255;
        if (c == 39) { ppp = ppp + 1; break; }
        ppp = ppp + 1;
        if (c == 92 && ppp < ppe) {
            c = ppb[ppp] & 255; ppp = ppp + 1;
            if (c == 110) c = 10;
            else if (c == 116) c = 9;
            else if (c == 114) c = 13;
            else if (c == 97) c = 7;
            else if (c == 98) c = 8;
            else if (c == 102) c = 12;
            else if (c == 118) c = 11;
            else if (c == 120) {
                c = 0;
                while (ppp < ppe) { d = pphexv(ppb[ppp] & 255); if (d < 0) break;
                                    c = c * 16 + d; ppp = ppp + 1; }
            }
            else if (c >= 48 && c <= 55) {
                c = c - 48; n = 1;
                while (ppp < ppe && n < 3) { d = ppb[ppp] & 255;
                    if (d < 48 || d > 55) break;
                    c = c * 8 + d - 48; ppp = ppp + 1; n = n + 1; }
            }
        }
        v = (v << 8) | (c & 255);
    }
    ppu = 0;
    return v;
}

long ppprim(void) {
    long v; int paren; int c;
    ppws();
    ppu = 0;
    if (ppp >= ppe) return 0;
    if (ppop1(33, 61, 0)) { v = ppprim(); ppu = 0; if (v) return 0; return 1; }
    if (ppop("~", 1)) { v = ppprim(); return ~v; }
    if (ppop1(45, 45, 61)) { v = ppprim(); return 0 - v; }
    if (ppop1(43, 43, 61)) return ppprim();
    if (ppop("(", 1)) { v = ppcond(); c = ppu; ppop(")", 1); ppu = c; return v; }
    c = ppb[ppp] & 255;
    if (isdi(c)) return ppnum();
    if (c == 39) return ppchar();
    if (c == 76 && ppp + 1 < ppe) { if ((ppb[ppp + 1] & 255) == 39) {
        ppp = ppp + 1; return ppchar(); } }
    if (isal(c)) {
        /* what is left of an identifier after expansion is 0, C99 6.10.1p4 --
           except `defined`, which a macro body may have produced */
        int ns; ns = ppp;
        while (ppp < ppe) { if (isal(ppb[ppp] & 255) == 0) {
                                if (isdi(ppb[ppp] & 255) == 0) break; }
                            ppp = ppp + 1; }
        if (ppp - ns == 7) { if (vsame("defined", 0, ppb + ns, 7)) {
            int s2;
            paren = 0;
            if (ppop("(", 1)) paren = 1;
            ppws(); s2 = ppp;
            while (ppp < ppe) { if (isal(ppb[ppp] & 255) == 0) {
                                    if (isdi(ppb[ppp] & 255) == 0) break; }
                                ppp = ppp + 1; }
            v = 0;
            if (mfind(ppb + s2, ppp - s2) >= 0) v = 1;
            if (paren) ppop(")", 1);
            ppu = 0;
            return v;
        } }
        return 0;
    }
    ppp = ppp + 1;
    return 0;
}

/* intmax_t division truncates toward zero; the one overflowing case,
   INTMAX_MIN / -1, is left to wrap */
long ppdiv(long a, long b, int u, int rem) {
    unsigned long ua; unsigned long ub;
    if (b == 0) { if (ppskip == 0) ppdiv0 = 1; return 0; }
    if (u) { ua = a; ub = b; if (rem) return ua % ub; return ua / ub; }
    if (b == 0 - 1) { if (rem) return 0; return 0 - a; }
    if (rem) return a % b;
    return a / b;
}

long ppmul(void) {
    long v; long r; int u;
    v = ppprim(); u = ppu;
    while (1) {
        if (ppop1(42, 61, 0)) { r = ppprim(); u = u | ppu; v = v * r; }
        else if (ppop1(47, 61, 0)) { r = ppprim(); u = u | ppu; v = ppdiv(v, r, u, 0); }
        else if (ppop1(37, 61, 0)) { r = ppprim(); u = u | ppu; v = ppdiv(v, r, u, 1); }
        else break;
    }
    ppu = u;
    return v;
}

long ppadd(void) {
    long v; long r; int u;
    v = ppmul(); u = ppu;
    while (1) {
        if (ppop1(43, 43, 61)) { r = ppmul(); u = u | ppu; v = v + r; }
        else if (ppop1(45, 45, 61)) { r = ppmul(); u = u | ppu; v = v - r; }
        else break;
    }
    ppu = u;
    return v;
}

long ppshift(void) {
    long v; long r; int u; unsigned long uv;
    v = ppadd(); u = ppu;
    while (1) {
        if (ppop("<<", 2)) { r = ppadd(); v = v << (r & 63); }
        else if (ppop(">>", 2)) { r = ppadd();
            if (u) { uv = v; uv = uv >> (r & 63); v = uv; }
            else v = v >> (r & 63); }
        else break;
    }
    ppu = u;
    return v;
}

/* a < b in the common type */
int pplt(long a, long b, int u) {
    unsigned long ua; unsigned long ub;
    if (u) { ua = a; ub = b; if (ua < ub) return 1; return 0; }
    if (a < b) return 1;
    return 0;
}

long pprel(void) {
    long v; long r; int u;
    v = ppshift(); u = ppu;
    while (1) {
        if (ppop("<=", 2)) { r = ppshift(); u = u | ppu; v = 1 - pplt(r, v, u); }
        else if (ppop(">=", 2)) { r = ppshift(); u = u | ppu; v = 1 - pplt(v, r, u); }
        else if (ppop1(60, 60, 61)) { r = ppshift(); u = u | ppu; v = pplt(v, r, u); }
        else if (ppop1(62, 62, 61)) { r = ppshift(); u = u | ppu; v = pplt(r, v, u); }
        else break;
        u = 0;
    }
    ppu = u;
    return v;
}

long ppeq(void) {
    long v; long r;
    v = pprel();
    while (1) {
        if (ppop("==", 2)) { r = pprel(); if (v == r) v = 1; else v = 0; ppu = 0; }
        else if (ppop("!=", 2)) { r = pprel(); if (v != r) v = 1; else v = 0; ppu = 0; }
        else break;
    }
    return v;
}

long ppband(void) {
    long v; long r; int u;
    v = ppeq(); u = ppu;
    while (ppop1(38, 38, 61)) { r = ppeq(); u = u | ppu; v = v & r; }
    ppu = u;
    return v;
}

long ppbxor(void) {
    long v; long r; int u;
    v = ppband(); u = ppu;
    while (ppop1(94, 61, 0)) { r = ppband(); u = u | ppu; v = v ^ r; }
    ppu = u;
    return v;
}

long ppbor(void) {
    long v; long r; int u;
    v = ppbxor(); u = ppu;
    while (ppop1(124, 124, 61)) { r = ppbxor(); u = u | ppu; v = v | r; }
    ppu = u;
    return v;
}

long ppland(void) {
    long v; long r;
    v = ppbor();
    while (ppop("&&", 2)) {
        if (v == 0) ppskip = ppskip + 1;
        r = ppbor();
        if (v == 0) ppskip = ppskip - 1;
        if (v) { if (r) v = 1; else v = 0; } else v = 0;
        ppu = 0;
    }
    return v;
}

long pplor(void) {
    long v; long r;
    v = ppland();
    while (ppop("||", 2)) {
        if (v) ppskip = ppskip + 1;
        r = ppland();
        if (v) ppskip = ppskip - 1;
        if (v) v = 1; else { if (r) v = 1; else v = 0; }
        ppu = 0;
    }
    return v;
}

long ppcond(void) {
    long v; long a; long b; int u;
    v = pplor();
    if (ppop("?", 1)) {
        if (v == 0) ppskip = ppskip + 1;
        a = ppcond(); u = ppu;
        if (v == 0) ppskip = ppskip - 1;
        ppop(":", 1);
        if (v) ppskip = ppskip + 1;
        b = ppcond(); u = u | ppu;
        if (v) ppskip = ppskip - 1;
        ppu = u;
        if (v) return a;
        return b;
    }
    return v;
}

int ppexpandline(int from, int to);  /* expands src[from..to) into ppb/ppe */

/* `defined X` and `defined ( X )` become 1 or 0 in place -- the directive
   line is blanked afterwards anyway -- so that expansion cannot touch X. */
int ppdefined(int from, int to) {
    int j; int k; int ns; int ne; int paren; int v;
    j = from;
    while (j < to) {
        if (isal(src[j] & 255) == 0) { j = j + 1; continue; }
        if (j > from) { if (isdi(src[j - 1] & 255)) { j = j + 1; continue; } }
        k = j;
        while (k < to) { if (isal(src[k] & 255) == 0) { if (isdi(src[k] & 255) == 0) break; } k = k + 1; }
        if (k - j != 7 || srcis(j, 7, "defined") == 0) { j = k; continue; }
        while (k < to && wsat(k)) k = k + 1;
        paren = 0;
        if (k < to) { if ((src[k] & 255) == 40) { paren = 1; k = k + 1; } }
        while (k < to && wsat(k)) k = k + 1;
        ns = k;
        while (k < to) { if (isal(src[k] & 255) == 0) { if (isdi(src[k] & 255) == 0) break; } k = k + 1; }
        ne = k;
        if (paren) { while (k < to && wsat(k)) k = k + 1;
                     if (k < to) { if ((src[k] & 255) == 41) k = k + 1; } }
        v = 0;
        if (ne > ns) { if (mfind(src + ns, ne - ns) >= 0) v = 1; }
        src[j] = 48 + v; j = j + 1;
        while (j < k) { src[j] = 32; j = j + 1; }
    }
    return 0;
}

int ppeval(int from, int to, int live) {
    long v;
    ppdefined(from, to);
    ppexpandline(from, to);
    ppp = 0; ppskip = 0; ppdiv0 = 0;
    if (live == 0) ppskip = 1;          /* a skipped group's #elif is not diagnosed */
    v = ppcond();
    if (v) return 1;
    return 0;
}

int takest[64];      /* per nesting level: are we taking? */
int seenst[64];      /* has a branch already been taken? */
int ndepth;

int wsat(int i) { if (src[i] == 32) return 1; if (src[i] == 9) return 1; return 0; }

/* Rewrite src in place: directives and skipped lines become blanks, so
 * positions and line numbers survive. */
/* ---- #include ---------------------------------------------------------
   The file is spliced IN PLACE of the directive and the scan resumes at the
   splice point, so a header's own directives -- its include guard first of
   all -- are processed by the same loop with the same macro state.  `"x"` is
   looked up beside the input file, then in include/; `<x>` in include/ only.
   A header we do not carry is skipped, as the Python driver skips it. */
/* Open for reading.  Windows has no open(2): the gate there is CreateFileA,
   which takes an access mask and a disposition -- <stdio.h>'s fopen makes
   the same choice.  _WIN32 is predefined when unisacc is built FOR Windows. */
int wopen(char *path) {              /* create/truncate, for -o */
    if (path[0] == 45 && path[1] == 0) return 1;   /* `-o -`: stdout */
#ifdef _WIN32
    return __open(path, 0x40000000, 2);   /* GENERIC_WRITE, CREATE_ALWAYS */
#else
#ifdef __linux__
    return __open(path, 1 | 64 | 512, 493);    /* WRONLY|CREAT|TRUNC, 0755 */
#else
    return __open(path, 1 | 512 | 1024, 493);
#endif
#endif
}

int ropen(char *path) {
#ifdef _WIN32
    return __open(path, 0x80000000, 3);      /* GENERIC_READ, OPEN_EXISTING */
#else
    return __open(path, 0);
#endif
}

/* tcc-style options the driver fills in before calling fe_tape [S-11]:
   `-I dir` is searched before the built-in headers, `-D NAME[=n]` is
   predefined like any other macro. */
char *optinc; int noptd; char *optd[16];
/* The rest of what a Makefile passes [S-15 C1]: -U names, -include files,
   -nostdinc, and stdin as an input. */
int noptu; char *optu[16];
int nopti; char *opti[8];
int nostdinc;
/* -MD / -MF FILE [S-15 C2]: every file the preprocessor OPENED, in order,
   for a `target: deps` line make can read.  The built-in header copies are
   not files and are not listed -- cc lists its system headers because they
   are on disk; ours travel inside the binary. */
char *depfile; int wantdeps;
char deppool[65536]; int ndeppool; int ndeps;
int prelines;               /* lines -include put before the user's own */

/* ---- where a byte came from, so an error can say file:line:col --------
   The buffer the parser sees is not the file the user wrote: continuation
   lines are joined (fewer lines) and every `#include` is replaced by the
   header's text (many more).  Two small tables record exactly that, and
   `err_at` walks them backwards.  Macro expansion rewrites in place and
   never adds or removes a newline, so it does not enter into it. [S-12] */
#define MAXIREG 512
long ireg_ln[MAXIREG];    /* the line the header's text starts on */
long ireg_nl[MAXIREG];    /* how many lines it is */
int ireg_nm[MAXIREG];     /* its name, as an offset into `fnpool` */
int nireg;
#define MAXSPL 4096
long spl_at[MAXSPL]; int nspl;   /* joined continuation lines */
char fnpool[8192]; int nfnpool;
char incname[64];                 /* the header being spliced, for the table */
int nautoinc;                     /* `#include` lines WE put at the top */
char *srcpath;                    /* the file being compiled, for `"x.h"` */
char optincdir[512]; int optincdl; /* -I, normalised with a trailing slash */

#define MAXINC 131072
char incbuf[MAXINC];
char incpath[512];
int nincl;

/* The headers we carry: `#include <stdio.h>` has to work when the compiler
   is a single file somewhere else entirely, with no include/ to read.  The
   filesystem still wins when it has the header, so editing include/ during
   development takes effect straight away. [S-11] */
int hdr_find(int nm, int nl) {
    int i; int p; int k; int ok;
    i = 0;
    while (i < NHDR) {
        p = voff(HDR_NAMES, i);
        ok = 1; k = 0;
        while (k < nl) {
            if (HDR_NAMES[p + k] != src[nm + k]) { ok = 0; break; }
            k = k + 1;
        }
        if (ok) { if (HDR_NAMES[p + nl] == 0) return i; }
        i = i + 1;
    }
    return 0 - 1;
}

int hdr_read(int nm, int nl) {       /* -> bytes in incbuf, or -1 */
    int i; char *t; int n;
    if (nostdinc) return 0 - 1;        /* -nostdinc: only -I and the file's dir */
    i = hdr_find(nm, nl);
    if (i < 0) return 0 - 1;
    t = hdr_text(i);
    n = 0;
    while (t[n]) { if (n < MAXINC) incbuf[n] = t[n]; n = n + 1; }
    if (n >= MAXINC) { __write(2, "header too large\n", 17); __exit(1); }
    return n;
}

int err_at(long p, char *msg);
int inctry(char *dir, int dl, int nm, int nl) {
    int k; int p; int fd; int n; int j; int grow;
    p = 0; k = 0;
    while (k < dl) { if (p < 500) { incpath[p] = dir[k]; p = p + 1; } k = k + 1; }
    k = 0;
    while (k < nl) { if (p < 510) { incpath[p] = src[nm + k]; p = p + 1; } k = k + 1; }
    incpath[p] = 0;
    fd = ropen(incpath);
    if (fd < 0) return 0 - 1;
    n = __read(fd, incbuf, MAXINC);
    __close(fd);
    if (n < 0) return 0 - 1;
    if (wantdeps) {                    /* a file that was really read */
        k = 0;
        while (incpath[k] && ndeppool < 65534) { deppool[ndeppool] = incpath[k]; ndeppool = ndeppool + 1; k = k + 1; }
        if (ndeppool < 65535) { deppool[ndeppool] = 0; ndeppool = ndeppool + 1; ndeps = ndeps + 1; }
    }
    return n;
}

/* the .d file: `target: input deps...`, one per line as make expects */
int writedeps(char *target, char **inputs, int ninput) {
    int fd; int k; int q; char *nm; char buf[4];
    if (depfile == 0) return 0;
    fd = wopen(depfile);
    if (fd < 0) { printf("cannot write %s\n", depfile); return 1; }
    nm = target; k = 0; while (nm[k]) k = k + 1; __write(fd, nm, k);
    __write(fd, ":", 1);
    q = 0;
    while (q < ninput) {
        nm = inputs[q]; k = 0; while (nm[k]) k = k + 1;
        __write(fd, " \\\n  ", 5); __write(fd, nm, k);
        q = q + 1;
    }
    q = 0; k = 0;
    while (q < ndeps) {
        int L; L = 0; while (deppool[k + L]) L = L + 1;
        __write(fd, " \\\n  ", 5); __write(fd, deppool + k, L);
        k = k + L + 1; q = q + 1;
    }
    buf[0] = 10; __write(fd, buf, 1);
    if (fd != 1) __close(fd);
    return 0;
}

/* Replace src[ls..le) with the included text.  Returns 1 if it did. */
int incdo(int ls, int le, int from) {
    int j; int q; int nm; int nl; int n; int k; int dl; int grow; char *a;
    j = from;
    while (j < le) { if (wsat(j) == 0) break; j = j + 1; }
    q = src[j] & 255;
    if (q != 34) { if (q != 60) return 0; }
    j = j + 1; nm = j;
    while (j < le) {
        if (q == 34) { if ((src[j] & 255) == 34) break; }
        else { if ((src[j] & 255) == 62) break; }
        j = j + 1;
    }
    nl = j - nm;
    if (nincl > 200) return 0;               /* a header that includes itself */
    {   int q2;                              /* the name, before the shift */
        incname[0] = 0; q2 = 0;
        while (q2 < nl && q2 < 62) { incname[q2] = src[nm + q2]; q2 = q2 + 1; }
        incname[q2] = 0;
    }
    n = 0 - 1;
    /* an absolute path names its file outright: it was being joined to
       the source's directory, so `#include "/abs/x.h"` silently opened
       nothing and the program compiled without it */
    if (src[nm] == 47) n = inctry("", 0, nm, nl);
    else if (q == 34) {
        a = srcpath;
        dl = 0; k = 0;
        while (a[k]) { if (a[k] == 47) dl = k + 1; k = k + 1; }
        n = inctry(a, dl, nm, nl);
    }
    if (n < 0) { if (optincdl) n = inctry(optincdir, optincdl, nm, nl); }
    if (n < 0) n = inctry("include/", 8, nm, nl);
    if (n < 0) n = hdr_read(nm, nl);     /* the copy we carry [S-11] */
    if (n < 0) {
        /* C99 6.10.2p4: a header that cannot be found is a constraint
           violation.  This returned 0 and the line simply vanished -- the
           program compiled without whatever it was meant to declare. */
        err_at(nm, "no such file for #include");
        __exit(1);
    }
    grow = n + 1 - (le - ls);
    if (nsrc + grow >= MAXSRC) { __write(2, "source too large\n", 17); __exit(1); }
    /* shift the tail, then drop the file in, plus a newline of its own */
    if (grow > 0) {
        k = nsrc - 1;
        while (k >= le) { src[k + grow] = src[k]; k = k - 1; }
    } else {
        k = le;
        while (k < nsrc) { src[k + grow] = src[k]; k = k + 1; }
    }
    k = 0;
    while (k < n) { src[ls + k] = incbuf[k]; k = k + 1; }
    src[ls + n] = 10;
    nsrc = nsrc + grow;
    nincl = nincl + 1;
    /* remember the swap, so a position inside the header can name it and a
       position after it can be counted back to the user's own line [S-12].
       Includes are spliced left to right, and a nested one lands INSIDE the
       region just recorded, so the table stays in order. */
    if (nireg < MAXIREG) {
        long c2; long ln;
        ln = 1; c2 = 0;
        while (c2 < ls) { if (src[c2] == 10) ln = ln + 1; c2 = c2 + 1; }
        ireg_ln[nireg] = ln;
        ireg_nl[nireg] = 1; c2 = 0;
        while (c2 < n) { if (incbuf[c2] == 10) ireg_nl[nireg] = ireg_nl[nireg] + 1; c2 = c2 + 1; }
        ireg_nm[nireg] = nfnpool;
        q = 0;
        while (incname[q] && nfnpool < 8000) { fnpool[nfnpool] = incname[q]; nfnpool = nfnpool + 1; q = q + 1; }
        fnpool[nfnpool] = 0; nfnpool = nfnpool + 1;
        nireg = nireg + 1;
    }
    return 1;
}

int splice(void);
int decomment(void);

/* ---- diagnostics ------------------------------------------------------
   `file:line:col: error: ...`, then the line, then a caret -- the shape
   every C programmer already reads.  The position walks back through the
   include swaps (§ the tables above); the line printed is the one the
   PARSER saw, which differs from the file only where a macro expanded. */
int blen(char *s);

int ec2(int c) { char b[1]; b[0] = c; __write(2, b, 1); return 0; }

int en2(long v) {                      /* a number, to stderr */
    char b[24]; int n; int k;
    if (v == 0) { ec2(48); return 0; }
    n = 0;
    while (v > 0 && n < 23) { b[n] = 48 + v % 10; v = v / 10; n = n + 1; }
    k = n - 1;
    while (k >= 0) { ec2(b[k] & 255); k = k - 1; }
    return 0;
}

int err_line(long p) {                 /* print the line containing p */
    long a; long b;
    a = p;
    while (a > 0 && src[a - 1] != 10) a = a - 1;
    b = p;
    while (b < nsrc && src[b] != 10) b = b + 1;
    __write(2, "  ", 2);
    __write(2, src + a, b - a);
    __write(2, "\n  ", 3);
    a = a;
    while (a < p) { __write(2, src[a] == 9 ? "\t" : " ", 1); a = a + 1; }
    __write(2, "^\n", 2);
    return 0;
}

int warnall;             /* -Wall: the warnings below are printed [S-15 C4] */
int panic;               /* defined with the error reporter: the walker is unwinding */
int curcall;             /* the value in hand came from a call (its type is not tracked) */
int declvoid;            /* the last base type scanned was `void` */
int nwarn;
int diag_at(long p, char *msg, char *kind);
int err_at(long p, char *msg) { return diag_at(p, msg, ": error: "); }
/* A warning: the same file:line:col shape, `warning:`, and clang's own
   [-W...] tag at the end of the message so a suite can map the kinds.
   Only under -Wall -- every suite that reads stderr as "refused" stays
   as it is, and so does a Makefile that does not ask. */
int warnonly;            /* diag_at: this one is a warning -- not in a header */
int warn_at(long p, char *msg) {
    if (warnall == 0) return 0;
    if (panic) return 0;
    nwarn = nwarn + 1;
    warnonly = 1;
    return diag_at(p, msg, ": warning: ");
}
int diag_at(long p, char *msg, char *kind) {
    long q; long line; long col; int i; int inside; char *fname;
    if (p < 0) p = 0;
    if (p > nsrc) p = nsrc;
    /* the line and column IN THE BUFFER -- macro expansion rewrites a line
       but never adds or removes one, so the line survives it */
    line = 1 - prelines; col = 1; q = 0;
    while (q < p) {
        if (src[q] == 10) { line = line + 1; col = 1; } else col = col + 1;
        q = q + 1;
    }
    /* Undo the header splices, innermost first.  A region spans the lines
       [ln, ln + nl]: `nl` lines of header text, and then the blank left
       where the `#include` line's own newline still is.  So a line after it
       sits `nl` lines further down than it does in the file. */
    inside = 0 - 1;
    i = nireg - 1;
    while (i >= 0) {
        if (line > ireg_ln[i] + ireg_nl[i]) line = line - ireg_nl[i];
        else { if (line >= ireg_ln[i]) { inside = i; line = line - ireg_ln[i] + 1; break; } }
        i = i - 1;
    }
    fname = inside >= 0 ? fnpool + ireg_nm[inside] : srcpath;
    if (warnonly) { warnonly = 0; if (inside >= 0) { nwarn = nwarn - 1; return 0; } }
    if (inside < 0) {
        line = line - nautoinc;   /* the headers we added on the user's behalf */
        /* each joined continuation line is a line the file has that this
           buffer does not */
        i = 0;
        while (i < nspl) { if (spl_at[i] < p) line = line + 1; i = i + 1; }
    }
    __write(2, fname, blen(fname));
    ec2(58); en2(line); ec2(58); en2(col);
    __write(2, kind, blen(kind));
    __write(2, msg, blen(msg));
    ec2(10);
    err_line(p);
    return 0;
}

/* More than one error per run [S-15 C3].  There is no longjmp here, so an
   error cannot unwind the walker; instead it PARKS the token pointer at
   the end of the file.  Every loop in the walker stops at EOF -- the
   hostile suite's truncated inputs made sure of that -- so the recursion
   unwinds by itself, emitting garbage the driver will never assemble
   (nerr > 0 means no image).  Back at the top level, unit() re-syncs to
   the token after the construct the error was in and walks on.  Errors
   raised while parked are cascades of the first and are not reported. */
int tp; int ntok;        /* the walker's cursor and its bound: defined with the lexer, used here */
int kind(int i);
int undef_calls(void);
int opt_stack(void);
int optlevel;            /* -O: 0 writes the tape as walked [H1] */
int tidx(char *s, int L);
int nerr;                /* errors reported so far */
int maxerr = 20;         /* -ferror-limit=N; 0 is no limit (clang's rule) */
int errtop;              /* where the top-level construct being walked began */

int err_tok(int t, char *msg) {        /* ...at a token */
    if (panic) return 0;
    if (t < 0 || t >= ntok) err_at(nsrc, msg); else err_at(tpos[t], msg);
    nerr = nerr + 1;
    if (maxerr > 0) { if (nerr >= maxerr) {
        __write(2, "too many errors emitted, stopping now\n", 38);
        __exit(1);
    } }
    panic = 1;
    tp = ntok;
    return 0;
}

/* After an error: the token after the construct it was in.  From where
   that construct began, skip a balanced brace block or a `;` at depth 0;
   the `;` that closes `struct S {...};` goes with the block. */
int resync(int from) {
    int k; int depth; int c;
    k = from; depth = 0;
    while (k < ntok) {
        c = kind(k);
        if (c == tidx("{", 1)) depth = depth + 1;
        if (c == tidx("}", 1)) {
            depth = depth - 1;
            if (depth <= 0) { k = k + 1; if (kind(k) == tidx(";", 1)) k = k + 1; return k; }
        }
        if (c == tidx(";", 1)) { if (depth == 0) return k + 1; }
        k = k + 1;
    }
    return ntok;
}

/* The target's predefined macros, the same set unisa/front/pp.py gives:
   `#ifdef __linux__` in <stdio.h> picks the O_* bits, `#ifdef _WIN32` the
   Win32 file calls, so a tape is compiled FOR an OS.  `-b os/arch` and
   `-t os/arch` name it; a bare tape is lnx/x86_64, as it is on the Python
   side and as the VM reads it. */
char *tgt;
int mdefb(char *s, int n, char *body, int bl, long v) {
    int i;
    mdef(s, n, v, 1);
    i = mfind(s, n);
    if (i < 0) return 0;
    macboff[i] = macstashs(body, bl); macblen[i] = bl;
    return 0;
}

/* `-D NAME` and the target's own macros: defined as 1, and they expand to
   "1" as well -- a macro with a value but no body expands to NOTHING, which
   turns `printf("%d", LEVEL)` into `printf("%d", )`. */
int mdef1(char *s) { return mdefb(s, blen(s), "1", 1, 1); }

int blen(char *s) { int n; n = 0; while (s[n]) n = n + 1; return n; }
int predef(void) {
    char *t; int i;
    /* -D NAME or -D NAME=<integer> */
    i = 0;
    while (i < noptd) {
        char *a; int n; long v;
        a = optd[i]; n = 0; v = 1;
        while (a[n] && a[n] != 61) n = n + 1;      /* '=' */
        if (a[n] == 61) {
            int k; int neg;
            k = n + 1; neg = 0; v = 0;
            if (a[k] == 45) { neg = 1; k = k + 1; }
            while (a[k] >= 48 && a[k] <= 57) { v = v * 10 + (a[k] - 48); k = k + 1; }
            if (neg) v = 0 - v;
            mdefb(a, n, a + n + 1, blen(a + n + 1), v);
        } else mdefb(a, n, "1", 1, 1);
        i = i + 1;
    }
    t = tgt;
    if (t[0] == 108) { mdef1("__linux__"); mdef1("__unix__"); mdef1("__ELF__"); }
    /* -U NAME: applied after every predefinition, so it can remove one */
    i = 0;
    while (i < noptu) {
        int m; int n; n = 0; while (optu[i][n]) n = n + 1;
        m = mac_newest(optu[i], n);
        if (m >= 0) macto[m] = 0;               /* live in no segment */
        i = i + 1;
    }
    if (t[0] == 111) { mdef1("__APPLE__"); mdef1("__MACH__"); mdef1("__unix__"); }
    if (t[0] == 119) { mdef1("_WIN32"); mdef1("_WIN64"); }
    if (t[4] == 120) mdef1("__x86_64__"); else mdef1("__aarch64__");
    mdef1("__LP64__");
    mdef1("__UNISA__");
    return 0;
}

int srcis(int p, int L, char *nm);
/* A directive at line start `ls` is about to change the table: the text
   after it is a new segment. */
int newseg(int ls) {
    if (nsegpos >= MAXSEG) { __write(2, "too many macro changes\n", 23); __exit(1); }
    curseg = curseg + 1;
    segpos[nsegpos] = ls; nsegpos = nsegpos + 1;
    return 0;
}

/* #pragma push_macro("X") / pop_macro("X") at src[p..e): 1 if it was one */
int pushpop(int ls, int p, int e) {
    int ispush; int n0; int n1; int m; int k; int j; int q;
    while (p < e && wsat(p)) p = p + 1;
    ispush = 0 - 1;
    if (p + 10 <= e) { if (srcis(p, 10, "push_macro")) { ispush = 1; p = p + 10; } }
    if (ispush < 0) { if (p + 9 <= e) { if (srcis(p, 9, "pop_macro")) { ispush = 0; p = p + 9; } } }
    if (ispush < 0) return 0;
    while (p < e && (src[p] & 255) != 34) p = p + 1;
    if (p >= e) return 0;
    n0 = p + 1; n1 = n0;
    while (n1 < e && (src[n1] & 255) != 34) n1 = n1 + 1;
    if (n1 >= e || n1 == n0 || n1 - n0 > 31) return 0;
    if (ispush) {
        if (npush >= MAXPUSH) return 1;
        k = 0;
        while (k < n1 - n0) { pushname[npush * 32 + k] = src[n0 + k]; k = k + 1; }
        pushname[npush * 32 + k] = 0;
        pushent[npush] = mfind(src + n0, n1 - n0);
        npush = npush + 1;
        return 1;
    }
    /* pop: the most recent push of this name */
    j = npush - 1;
    while (j >= 0) {
        k = 0; q = 1;
        while (k < n1 - n0) { if (pushname[j * 32 + k] != src[n0 + k]) q = 0; k = k + 1; }
        if (q) { if (pushname[j * 32 + k] == 0) break; }
        j = j - 1;
    }
    if (j < 0) return 1;
    m = pushent[j];
    k = j; while (k + 1 < npush) {       /* drop that push */
        q = 0; while (q < 32) { pushname[k * 32 + q] = pushname[(k + 1) * 32 + q]; q = q + 1; }
        pushent[k] = pushent[k + 1]; k = k + 1;
    }
    npush = npush - 1;
    newseg(ls);
    { int cur; cur = mfind(src + n0, n1 - n0); if (cur >= 0) macto[cur] = curseg; }
    if (m >= 0) {                        /* the saved definition, again */
        mdef(src + n0, n1 - n0, macval[m], machas[m]);
        k = nmac - 1;
        macboff[k] = macboff[m]; macblen[k] = macblen[m];
        macfn[k] = macfn[m]; macnp[k] = macnp[m]; macvar[k] = macvar[m];
        q = 0;
        while (q < MAXMPARAM) {
            macpoff[k * MAXMPARAM + q] = macpoff[m * MAXMPARAM + q];
            macplen[k * MAXMPARAM + q] = macplen[m * MAXMPARAM + q];
            q = q + 1;
        }
    }
    return 1;
}

int ppdiv0at(int ls, int live) {
    if (ppdiv0) { if (live) { err_at(ls, "division by zero in #if"); nerr = nerr + 1; } }
    ppdiv0 = 0;
    return 0;
}

int preprocess(void) {
    int i; int ls; int j; int ws; int we; int live; int d; int flag;
    int a; int k; int ns; int ne;
    int key[4];
    ndepth = 0;
    nmac = 0; mh_n = 0 - 1;
    curseg = 0; nsegpos = 0; npush = 0; pp_seg = 0 - 1;
    predef();
    i = 0;
    while (i < nsrc) {
        ls = i;
        while (i < nsrc) { if (src[i] == 10) break; i = i + 1; }
        /* live = every open level is taking */
        live = 1;
        k = 0;
        while (k < ndepth) { if (takest[k] == 0) live = 0; k = k + 1; }
        j = ls;
        while (j < i) { if (wsat(j) == 0) break; j = j + 1; }
        if (src[j] == 35) {                      /* '#' */
            j = j + 1;
            while (j < i) { if (wsat(j) == 0) break; j = j + 1; }
            ws = j;
            while (j < i) { if (isal(src[j] & 255) == 0) break; j = j + 1; }
            we = j;
            d = vfind(DIRV, NDIRV, src + ws, we - ws);
            while (j < i) { if (wsat(j) == 0) break; j = j + 1; }
            ns = j;
            while (j < i) { if (isal(src[j] & 255) == 0) { if (isdi(src[j] & 255) == 0) break; } j = j + 1; }
            ne = j;
            flag = 0;
            if (d < 0) { if (live) { if (we - ws == 6) { if (srcis(ws, 6, "pragma")) {
                pushpop(ls, we, i);
            } } } }
            if (d >= 0) {
                if (d == 0) { if (mfind(src + ns, ne - ns) >= 0) flag = 1; }
                /* The table's key is `defined` -- whether the name IS
                   defined -- and the table itself inverts for ifndef.
                   Inverting here too meant every `#ifndef GUARD` block was
                   skipped: a double negation that went unseen only because
                   unisacc.c has no include guards of its own. */
                if (d == 1) { if (mfind(src + ns, ne - ns) >= 0) flag = 1; }
                if (d == 2) { if (ppeval(ns, i, live)) flag = 1; ppdiv0at(ls, live); }   /* #if */
                if (d == 3) {                                  /* #elif */
                    int ol; ol = 0;
                    if (ndepth > 0) { if (seenst[ndepth-1] == 0) { ol = 1;
                        k = 0; while (k < ndepth - 1) { if (takest[k] == 0) ol = 0; k = k + 1; } } }
                    if (ppeval(ns, i, ol)) flag = 1; ppdiv0at(ls, ol); }
                if (d == 4) { if (ndepth > 0) { if (seenst[ndepth-1] == 0) flag = 1; } }
                if (d > 4) flag = 1;
                key[0] = d; key[1] = flag; key[2] = 0; key[3] = 0;
                a = inf(S_PP, key, 0);          /* the table decides */
                /* the table files `include` under `macro` [G-4 corrected] */
                if (d == 7) { if (live) { if (a == 3) {
                    if (incdo(ls, i, we)) {
                        /* phases 2 and 3 for the new text; both are no-ops
                           on text that has already been through them */
                        splice(); decomment();
                        i = ls;
                        continue;
                    }
                } } }
                if (d < 3) {                      /* ifdef ifndef if */
                    takest[ndepth] = 0;
                    if (a == 0) { if (live) takest[ndepth] = 1; }
                    seenst[ndepth] = takest[ndepth];
                    ndepth = ndepth + 1;
                } else {
                if (d == 3) {                     /* elif */
                    if (ndepth > 0) {
                        takest[ndepth-1] = 0;
                        if (a == 0) { if (seenst[ndepth-1] == 0) takest[ndepth-1] = 1; }
                        if (takest[ndepth-1]) seenst[ndepth-1] = 1;
                    }
                } else {
                if (d == 4) {                     /* else */
                    if (ndepth > 0) {
                        takest[ndepth-1] = 0;
                        if (a == 0) { if (seenst[ndepth-1] == 0) takest[ndepth-1] = 1; }
                        if (takest[ndepth-1]) seenst[ndepth-1] = 1;
                    }
                } else {
                if (a == 2) { if (ndepth > 0) ndepth = ndepth - 1; }   /* pop */
                else {
                if (a == 3) {                     /* macro: define / undef */
                    /* #undef NAME: the live definition ends here */
                    if (live) { if (d == 8) {
                        int um; um = mfind(src + ns, ne - ns);
                        if (um >= 0) { newseg(ls); macto[um] = curseg; }
                    } }
                    if (live) { if (d == 6) {
                        newseg(ls);
                        int vs; int vv; int vh; int mi; int np; int ps; int pe;
                        int po[MAXMPARAM]; int pl[MAXMPARAM]; int pk; int isvar;
                        vs = ne;
                        np = 0 - 1; isvar = 0;
                        /* function-like ONLY when `(` touches the name:
                           `#define A (x)` is an object-like macro whose body
                           happens to start with a parenthesis. */
                        if (vs < i) { if ((src[vs] & 255) == 40) {
                            np = 0; vs = vs + 1;
                            while (vs < i) {
                                while (vs < i) { if (wsat(vs) == 0) break; vs = vs + 1; }
                                if (vs < i) { if ((src[vs] & 255) == 41) break; }
                                ps = vs;
                                if ((src[vs] & 255) == 46) {     /* `...` */
                                    while (vs < i) { if ((src[vs] & 255) != 46) break; vs = vs + 1; }
                                    if (np < MAXMPARAM) {
                                        po[np] = macstashs("__VA_ARGS__", 11); pl[np] = 11;
                                    }
                                    np = np + 1;
                                    isvar = 1;
                                    while (vs < i) { if (wsat(vs) == 0) break; vs = vs + 1; }
                                    break;
                                }
                                while (vs < i) {
                                    if (isal(src[vs] & 255)) { vs = vs + 1; continue; }
                                    if (isdi(src[vs] & 255)) { vs = vs + 1; continue; }
                                    break;
                                }
                                pe = vs;
                                if (np < MAXMPARAM) {
                                    po[np] = macstash(ps, pe); pl[np] = pe - ps;
                                }
                                np = np + 1;
                                while (vs < i) { if (wsat(vs) == 0) break; vs = vs + 1; }
                                if (vs < i) { if ((src[vs] & 255) == 44) { vs = vs + 1; continue; } }
                                break;
                            }
                            if (vs < i) { if ((src[vs] & 255) == 41) vs = vs + 1; }
                        } }
                        while (vs < i) { if (wsat(vs) == 0) break; vs = vs + 1; }
                        /* The numeric probe needs its OWN cursor: `vs` is
                           where the replacement list starts, and scanning the
                           digits with it left `#define N 5` with an empty
                           body -- every use of N then expanded to nothing. */
                        vv = 0; vh = 0; pe = vs;
                        if (pe < i) { if (isdi(src[pe] & 255)) {
                            vh = 1;
                            while (pe < i) { if (isdi(src[pe] & 255) == 0) break;
                                             vv = vv * 10 + ((src[pe] & 255) - 48);
                                             pe = pe + 1; } } }
                        mdef(src + ns, ne - ns, vv, vh);
                        mi = mfind(src + ns, ne - ns);
                        if (mi >= 0) {
                            /* the replacement list runs to the end of the
                               line -- phase 2 has already joined continuations
                               and phase 3 has already removed comments */
                            macblen[mi] = i - vs;
                            macboff[mi] = macstash(vs, i);
                            if (np >= 0) {
                                macfn[mi] = 1; macnp[mi] = np; macvar[mi] = isvar;
                                pk = 0;
                                while (pk < np) {
                                    if (pk < MAXMPARAM) {
                                        macpoff[mi * MAXMPARAM + pk] = po[pk];
                                        macplen[mi * MAXMPARAM + pk] = pl[pk];
                                    }
                                    pk = pk + 1;
                                }
                            }
                        }
                    } }
                } } } } }
            }
            k = ls;
            while (k < i) { src[k] = 32; k = k + 1; }   /* blank the directive */
        } else {
            if (live == 0) {
                k = ls;
                while (k < i) { src[k] = 32; k = k + 1; }
            }
        }
        i = i + 1;
    }
    return 0;
}

/* C99 phase 3: every comment becomes one space, BEFORE the directives of
   phase 4 are processed.  Leaving it to the lexer -- which is phase 7 --
   means `#define N 32  // note` captures the note as part of the replacement
   list, and every later use of N comments out the rest of its own line.  The
   Python side paid for this once already (E-45); with text macros on this
   side it is the same bug.  A block comment leaves its newlines behind so
   nothing below it moves. */
/* ---- on-demand headers ------------------------------------------------
   There is no linker: the C library is ordinary C in include/, defined
   `static`.  A program that calls strlen without including <string.h> --
   most C ever written -- used to be refused, or, where a call happened to
   resolve, answered wrong.  The Python driver retries with the header
   appended; this does the same in one pass, before preprocessing: for every
   function one of our headers DEFINES, if the program calls it and does not
   define it, the header is appended.  Appending keeps line numbers, and the
   include guards make a second copy free.  printf is special: the walker
   desugars it, so <stdio.h> is only needed for a format that needs the
   runtime formatter (a width, a flag, a precision). */
int idch(int c) { if (isal(c)) return 1; return isdi(c); }

/* Where each identifier occurs, built once for autoinc.  srcfind scanned
   the whole source per header function asked about -- 39% of the
   self-built compiler compiling itself.  An occurrence is a maximal run of
   identifier characters, which is exactly what the scan's two boundary
   tests accept.  Valid only while autoinc runs (srcix_on).  Prepending a
   header line moves every position by the same amount, so incappend adds
   to sx_pre instead of rebuilding; the prepended bytes themselves (ending
   in a newline, so no identifier straddles the seam) are walked. */
#define SX_SIZE 65536
#define SX_MAXOCC 1048576
int sx_tab[SX_SIZE];          /* name id + 1 */
int sx_pos[SX_SIZE]; int sx_len[SX_SIZE]; int sx_cnt[SX_SIZE]; int sx_at[SX_SIZE];
int sx_occ[SX_MAXOCC]; int sx_names; int srcix_on; int sx_pre;
int sx_find_at(char *nm, int nl, int sh);
int sx_find(char *nm, int nl) { return sx_find_at(nm, nl, 0); }
int sx_find_at(char *nm, int nl, int sh) {   /* name id, or < 0 */
    int h; int e; int k; int ok;
    h = vhash(nm, nl) * 16 + (nl & 15);
    h = h & (SX_SIZE - 1);
    while (sx_tab[h]) {
        e = sx_tab[h] - 1;
        if (sx_len[e] == nl) {
            ok = 1; k = 0;
            while (k < nl) { if ((src[sh + sx_pos[e] + k] & 255) != (nm[k] & 255)) { ok = 0; break; } k = k + 1; }
            if (ok) return e;
        }
        h = (h + 1) & (SX_SIZE - 1);
    }
    return 0 - h - 2;                 /* not found: -(free slot) - 2 */
}
int srcix_build(void) {
    int i; int j; int e; int pass; int tot;
    srcix_on = 0; sx_pre = 0;
    i = 0; while (i < SX_SIZE) { sx_tab[i] = 0; i = i + 1; }
    sx_names = 0;
    pass = 0;
    while (pass < 2) {
        i = 0;
        while (i < nsrc) {
            if (idch(src[i] & 255)) {
                j = i; while (j < nsrc) { if (idch(src[j] & 255) == 0) break; j = j + 1; }
                e = sx_find(src + i, j - i);
                if (e < 0) {
                    if (pass == 1) return 0;               /* cannot happen */
                    if (sx_names >= SX_SIZE / 2) return 0; /* too many: walk */
                    sx_tab[0 - e - 2] = sx_names + 1;
                    e = sx_names; sx_names = sx_names + 1;
                    sx_pos[e] = i; sx_len[e] = j - i; sx_cnt[e] = 0;
                }
                if (pass == 0) sx_cnt[e] = sx_cnt[e] + 1;
                else { sx_occ[sx_at[e]] = i; sx_at[e] = sx_at[e] + 1; }
                i = j;
            } else i = i + 1;
        }
        if (pass == 0) {
            tot = 0; e = 0;
            while (e < sx_names) { sx_at[e] = tot; tot = tot + sx_cnt[e]; e = e + 1; }
            if (tot > SX_MAXOCC) return 0;
        }
        pass = pass + 1;
    }
    e = 0; while (e < sx_names) { sx_at[e] = sx_at[e] - sx_cnt[e]; e = e + 1; }
    srcix_on = 1;
    return 1;
}
/* where the identifier nm[0..nl) next occurs in src from `from`, or -1 */
int srcfind_walk(char *nm, int nl, int from);
int srcfind_lim(char *nm, int nl, int from, int end);
int srcfind(char *nm, int nl, int from) {
    int e; int lo; int hi; int m;
    if (srcix_on == 0) return srcfind_walk(nm, nl, from);
    if (nl <= 0) return srcfind_walk(nm, nl, from);
    e = 0; while (e < nl) { if (idch(nm[e] & 255) == 0) return srcfind_walk(nm, nl, from); e = e + 1; }
    if (from < sx_pre) {                  /* the prepended lines: walk them */
        m = srcfind_lim(nm, nl, from, sx_pre);
        if (m >= 0) return m;
        from = sx_pre;
    }
    e = sx_find_at(nm, nl, sx_pre);
    if (e < 0) return 0 - 1;
    from = from - sx_pre;
    lo = sx_at[e]; hi = sx_at[e] + sx_cnt[e];
    while (lo < hi) { m = (lo + hi) / 2; if (sx_occ[m] < from) lo = m + 1; else hi = m; }
    if (lo < sx_at[e] + sx_cnt[e]) return sx_occ[lo] + sx_pre;
    return 0 - 1;
}
int srcfind_lim(char *nm, int nl, int from, int end);
int srcfind_walk(char *nm, int nl, int from) { return srcfind_lim(nm, nl, from, nsrc); }
int srcfind_lim(char *nm, int nl, int from, int end) {
    int i; int k;
    i = from;
    while (i + nl <= end) {
        k = 0;
        while (k < nl) { if ((src[i + k] & 255) != (nm[k] & 255)) break; k = k + 1; }
        if (k == nl) {
            if (i == 0 || idch(src[i - 1] & 255) == 0) {
                if (i + nl >= nsrc || idch(src[i + nl] & 255) == 0) return i;
            }
        }
        i = i + 1;
    }
    return 0 - 1;
}
int skipws(int i) {
    while (i < nsrc) { if (wsat(i) == 0) { if ((src[i] & 255) != 10) break; } i = i + 1; }
    return i;
}
/* 1: called somewhere; 2: defined (its `)` is followed by `{`) */
int srcuse(char *nm, int nl) {
    int i; int j; int d; int called;
    called = 0; i = srcfind(nm, nl, 0);
    while (i >= 0) {
        j = skipws(i + nl);
        if (j < nsrc) { if ((src[j] & 255) == 40) {
            called = 1;
            d = 0;
            while (j < nsrc) {
                if ((src[j] & 255) == 40) d = d + 1;
                if ((src[j] & 255) == 41) { d = d - 1; if (d == 0) break; }
                j = j + 1;
            }
            j = skipws(j + 1);
            if (j < nsrc) { if ((src[j] & 255) == 123) return 2; }
        } }
        i = srcfind(nm, nl, i + 1);
    }
    return called;
}
/* does some literal printf format need the runtime formatter? */
int rtprintf(void) {
    int i; int j; int c;
    i = srcfind("printf", 6, 0);
    while (i >= 0) {
        j = skipws(i + 6);
        if (j < nsrc) { if ((src[j] & 255) == 40) {
            j = skipws(j + 1);
            if (j < nsrc) { if ((src[j] & 255) == 34) {
                j = j + 1;
                while (j < nsrc) {
                    c = src[j] & 255;
                    if (c == 34) break;
                    if (c == 92) { j = j + 2; continue; }
                    if (c == 37) {
                        c = src[j + 1] & 255;
                        if (c == 45 || c == 43 || c == 32 || c == 35 || c == 46) return 1;
                        if (isdi(c)) return 1;
                        j = j + 2; continue;
                    }
                    j = j + 1;
                }
            } }
        } }
        i = srcfind("printf", 6, i + 1);
    }
    return 0;
}
/* PREPENDED, where the user would have written it.  The Python driver can
   append, because it resolves calls at the end of the unit; this walker
   picks a call's convention AT the call, so a variadic library function
   has to be defined before its first use. */
int incappend(char *h, int hl) {
    int n; int k;
    n = 12 + hl;                            /* `#include <` h `>\n` */
    if (nsrc + n >= MAXSRC) { __write(2, "source too large\n", 17); __exit(1); }
    k = nsrc - 1;
    while (k >= 0) { src[k + n] = src[k]; k = k - 1; }
    k = 0;
    while (k < 10) { src[k] = "#include <"[k]; k = k + 1; }
    while (k < 10 + hl) { src[k] = h[k - 10]; k = k + 1; }
    src[k] = 62; src[k + 1] = 10;
    nsrc = nsrc + n;
    nautoinc = nautoinc + 1;      /* a line the user did not write [S-12] */
    sx_pre = sx_pre + n;          /* every indexed position just moved */
    return 0;
}
/* the header's definitions: a line opening `static`, whose name is the
   identifier before its first `(`, and which opens a body on that line */
int hdrneeded(int n) {
    int i; int e; int p; int b; int a; int k; int hasbody;
    i = 0;
    while (i < n) {
        e = i;
        while (e < n) { if ((incbuf[e] & 255) == 10) break; e = e + 1; }
        if (e - i > 7) { if (incbuf[i] == 115) { if (incbuf[i + 1] == 116) { if (incbuf[i + 2] == 97) {
          if (incbuf[i + 3] == 116) { if (incbuf[i + 4] == 105) { if (incbuf[i + 5] == 99) {
            p = i; while (p < e) { if ((incbuf[p] & 255) == 40) break; p = p + 1; }
            hasbody = 0; k = p;
            while (k < e) { if ((incbuf[k] & 255) == 123) hasbody = 1; k = k + 1; }
            if (p < e) { if (hasbody) {
                b = p;
                while (b > i) { if ((incbuf[b - 1] & 255) != 32) break; b = b - 1; }
                a = b;
                while (a > i) { if (idch(incbuf[a - 1] & 255) == 0) break; a = a - 1; }
                if (b > a) {
                    /* printf is the walker's; see rtprintf */
                    k = 1;
                    if (b - a == 6) { if (srcfind("printf", 6, 0) >= 0) {
                        if (incbuf[a] == 112) { if (incbuf[a + 1] == 114) { if (incbuf[a + 2] == 105) {
                            if (incbuf[a + 3] == 110) { if (incbuf[a + 4] == 116) { if (incbuf[a + 5] == 102) k = 0; } } } } } } }
                    if (k) { if (srcuse(incbuf + a, b - a) == 1) return 1; }
                }
            } }
        } } } } } } }
        i = e + 1;
    }
    return 0;
}
int autoinc(void) {
    char *hs; int k; int st; int fd; int n; int p;
    hs = "assert.h ctype.h stdlib.h string.h wchar.h stdio.h ";
    k = 0;
    while (hs[k]) {
        st = k;
        while (hs[k] != 32) k = k + 1;
        p = 0;
        while (p < 8) { incpath[p] = "include/"[p]; p = p + 1; }
        n = st;
        while (n < k) { incpath[p] = hs[n]; p = p + 1; n = n + 1; }
        incpath[p] = 0;
        fd = ropen(incpath);
        n = 0 - 1;
        if (fd >= 0) { n = __read(fd, incbuf, MAXINC); __close(fd); }
        else if (nostdinc == 0) {
            /* not beside us: the copy we carry, as #include falls back to.
               Only the relative path was tried, so outside the source tree
               -- which is everywhere the shipped binary runs -- nothing was
               appended, strcpy stayed undefined, and the program jumped
               into garbage [S-11] */
            p = vfind(HDR_NAMES, NHDR, hs + st, k - st);
            if (p >= 0) {
                char *t; t = hdr_text(p); n = 0;
                while (t[n]) { if (n < MAXINC) incbuf[n] = t[n]; n = n + 1; }
                if (n >= MAXINC) n = 0 - 1;
            }
        }
        if (n > 0) { if (srcix_on == 0) srcix_build();
                     if (hdrneeded(n)) incappend(hs + st, k - st); }
        k = k + 1;
    }
    if (srcix_on == 0) srcix_build();
    if (rtprintf()) incappend("stdio.h", 7);
    srcix_on = 0;
    return 0;
}

int decomment(void) {
    int i; int o; int c; int q;
    i = 0; o = 0;
    while (i < nsrc) {
        c = src[i] & 255;
        if (c == 34) { q = 34; }
        else { if (c == 39) { q = 39; } else { q = 0; } }
        if (q) {
            src[o] = src[i]; o = o + 1; i = i + 1;
            while (i < nsrc) {
                if ((src[i] & 255) == 92) {
                    src[o] = src[i]; o = o + 1; i = i + 1;
                    if (i < nsrc) { src[o] = src[i]; o = o + 1; i = i + 1; }
                    continue;
                }
                src[o] = src[i]; o = o + 1;
                if ((src[i] & 255) == q) { i = i + 1; break; }
                i = i + 1;
            }
            continue;
        }
        if (c == 47) {
            if (i + 1 < nsrc) {
                if ((src[i + 1] & 255) == 47) {          /* // */
                    while (i < nsrc) { if ((src[i] & 255) == 10) break; i = i + 1; }
                    src[o] = 32; o = o + 1;
                    continue;
                }
                if ((src[i + 1] & 255) == 42) {          /* slash-star */
                    int nl; int start;
                    nl = 0; start = i; i = i + 2;
                    while (i < nsrc) {
                        if ((src[i] & 255) == 10) nl = nl + 1;
                        if ((src[i] & 255) == 42) { if (i + 1 < nsrc) {
                            if ((src[i + 1] & 255) == 47) { i = i + 2; break; } } }
                        i = i + 1;
                    }
                    /* C99 5.1.1.2p1 phase 3: a comment has to be closed.
                       Running off the end used to be silent, so a stray
                       `/*` swallowed the rest of the file and whatever it
                       left compiled. */
                    if (i >= nsrc) {
                        err_at(start, "unterminated comment");
                        __exit(1);
                    }
                    src[o] = 32; o = o + 1;
                    while (nl > 0) { src[o] = 10; o = o + 1; nl = nl - 1; }
                    continue;
                }
            }
        }
        src[o] = src[i]; o = o + 1; i = i + 1;
    }
    nsrc = o;
    return 0;
}

/* ---- macro expansion, C99 phase 4 ------------------------------------
   The old preprocessor kept a NUMBER per macro, which is enough for `#if`
   and for nothing else: `#define STR "x"` and `#define MAX(a,b) ...` are
   most of what real C does with the preprocessor.  This substitutes text.

   Two rules are worth stating because getting them wrong is silent:
     * a macro is function-like only when `(` TOUCHES the name in the
       `#define` -- `#define A (x)` is an object-like macro whose body starts
       with a parenthesis;
     * every parameter goes in on ONE pass over the body.  Substituting them
       in turn rewrites text an earlier argument just put there, and
       `FF(d,a,b,c)` on `#define FF(a,b,c,d)` then writes the wrong variable
       (E-45 caught exactly that on the Python side). */
char ebuf[MAXSRC]; int nebuf;
int argo[MAXMPARAM]; int argl[MAXMPARAM]; int nargs;

int ebseg[MAXSRC];       /* the segment each byte of ebuf came from */
int eput(int c) {
    if (nebuf >= MAXSRC) { __write(2, "macro expansion overflow\n", 25); __exit(1); }
    ebuf[nebuf] = c; ebseg[nebuf] = eseg; nebuf = nebuf + 1;
    return 0;
}

int eputsrc(int from, int to) {
    int k; k = from;
    while (k < to) { eput(src[k] & 255); k = k + 1; }
    return 0;
}

/* A universal character name inside an identifier, C99 6.4.3: `\u` and
   four hex digits, or `\U` and eight.  How many bytes it spans at j, or 0.
   The identifier keeps its SPELLING -- `caf\u00e9` is one name, spelled
   the same wherever it is used -- which is all a program can observe. */
int ishexc(int c) {
    if (c >= 48 && c <= 57) return 1;
    if (c >= 97 && c <= 102) return 1;
    if (c >= 65 && c <= 70) return 1;
    return 0;
}
int ucnlen(int j) {
    int n; int k;
    if (j + 1 >= nsrc) return 0;
    if ((src[j] & 255) != 92) return 0;
    n = 0;
    if ((src[j + 1] & 255) == 117) n = 4;
    if ((src[j + 1] & 255) == 85) n = 8;
    if (n == 0) return 0;
    if (j + 2 + n > nsrc) return 0;
    k = 0;
    while (k < n) { if (ishexc(src[j + 2 + k] & 255) == 0) return 0; k = k + 1; }
    return 2 + n;
}

int identend(int i) {
    int j; int u; j = i;
    while (j < nsrc) {
        if (isal(src[j] & 255)) { j = j + 1; continue; }
        if (isdi(src[j] & 255)) { j = j + 1; continue; }
        u = ucnlen(j);
        if (u) { j = j + u; continue; }
        break;
    }
    return j;
}

/* Is the identifier at macpool[bo..bo+n) one of this macro's parameters? */
int paramat(int mi, int bo, int n) {
    int p; int L; int ok; int j;
    p = 0;
    while (p < macnp[mi]) {
        if (p < MAXMPARAM) {
            L = macplen[mi * MAXMPARAM + p];
            if (L == n) {
                ok = 1; j = 0;
                while (j < L) {
                    if (macpool[macpoff[mi * MAXMPARAM + p] + j] != macpool[bo + j]) ok = 0;
                    j = j + 1;
                }
                if (ok) return p;
            }
        }
        p = p + 1;
    }
    return 0 - 1;
}

/* `i` is at the `(`.  Returns the index just past the matching `)`, or -1. */
/* An argument's leading and trailing whitespace is not part of it.  `##`
   pastes the TEXT, so `CAT(cat, ab)` with the space kept produces `cat ab`
   and the paste silently does not happen. */
int argpush(int st, int en) {
    while (st < en) { if (wsat(st) == 0) break; st = st + 1; }
    while (en > st) { if (wsat(en - 1) == 0) break; en = en - 1; }
    if (nargs < MAXMPARAM) { argo[nargs] = st; argl[nargs] = en - st; }
    nargs = nargs + 1;
    return 0;
}

int collectargs(int i) {
    int depth; int st; int c;
    nargs = 0; depth = 1;
    i = i + 1; st = i;
    while (i < nsrc) {
        c = src[i] & 255;
        if (c == 34) {                                   /* a string */
            i = i + 1;
            while (i < nsrc) {
                if ((src[i] & 255) == 92) { i = i + 2; continue; }
                if ((src[i] & 255) == 34) break;
                i = i + 1;
            }
            i = i + 1; continue;
        }
        if (c == 39) {                                   /* a character */
            i = i + 1;
            while (i < nsrc) {
                if ((src[i] & 255) == 92) { i = i + 2; continue; }
                if ((src[i] & 255) == 39) break;
                i = i + 1;
            }
            i = i + 1; continue;
        }
        if (c == 40) depth = depth + 1;
        if (c == 41) {
            depth = depth - 1;
            if (depth == 0) {
                argpush(st, i);
                return i + 1;
            }
        }
        if (c == 44) { if (depth == 1) {
            argpush(st, i);
            st = i + 1;
        } }
        i = i + 1;
    }
    return 0 - 1;
}


int emitstring(int p) {          /* #param */
    int k; int e; int c;
    eput(34);
    if (p < nargs) { if (p < MAXMPARAM) {
        k = argo[p]; e = k + argl[p];
        while (k < e) {
            c = src[k] & 255;
            if (c == 34) eput(92);
            if (c == 92) eput(92);
            eput(c);
            k = k + 1;
        }
    } }
    eput(34);
    return 0;
}

int emitrange(int from, int to, int depth);

int emitbody(int mi, int isfn, int depth) {
    int b; int e; int j; int je; int k; int c; int pi; int pastejust;
    b = macboff[mi]; e = b + macblen[mi];
    j = b; pastejust = 0;
    while (j < e) {
        c = macpool[j] & 255;
        if (c == 35) {                                   /* `#` or `##` */
            if (j + 1 < e) { if ((macpool[j + 1] & 255) == 35) {
                while (nebuf > 0) {
                    if (ebuf[nebuf - 1] != 32) { if (ebuf[nebuf - 1] != 9) break; }
                    nebuf = nebuf - 1;
                }
                j = j + 2;
                while (j < e) {
                    if ((macpool[j] & 255) != 32) { if ((macpool[j] & 255) != 9) break; }
                    j = j + 1;
                }
                /* `, ## __VA_ARGS__` with NO variable arguments: the comma
                   goes too.  Strictly a GNU extension -- C99 6.10.3p4 wants
                   at least one argument for `...` -- but it is in so much
                   real code (every logging macro) that refusing it refuses
                   the code. */
                if (isfn) { if (j + 11 <= e) {
                    int q; int isva; isva = 1; q = 0;
                    while (q < 11) {
                        if ((macpool[j + q] & 255) != ("__VA_ARGS__"[q] & 255)) isva = 0;
                        q = q + 1;
                    }
                    if (isva) {
                        int vp; vp = macnp[mi] - 1;
                        if (vp >= nargs || argl[vp] == 0) {
                            if (nebuf > 0) { if (ebuf[nebuf - 1] == 44) nebuf = nebuf - 1; }
                            j = j + 11;
                            pastejust = 0;
                            continue;
                        }
                    }
                } }
                pastejust = 1;
                continue;
            } }
            if (isfn) {
                k = j + 1;
                while (k < e) { if ((macpool[k] & 255) != 32) break; k = k + 1; }
                if (k < e) { if (isal(macpool[k] & 255)) {
                    je = k;
                    while (je < e) {
                        if (isal(macpool[je] & 255)) { je = je + 1; continue; }
                        if (isdi(macpool[je] & 255)) { je = je + 1; continue; }
                        break;
                    }
                    pi = paramat(mi, k, je - k);
                    if (pi >= 0) { emitstring(pi); j = je; continue; }
                } }
            }
        }
        if (c == 34) {                                   /* a literal body */
            eput(c); j = j + 1;
            while (j < e) {
                if ((macpool[j] & 255) == 92) { eput(92); j = j + 1;
                    if (j < e) { eput(macpool[j] & 255); j = j + 1; } continue; }
                eput(macpool[j] & 255);
                if ((macpool[j] & 255) == 34) { j = j + 1; break; }
                j = j + 1;
            }
            continue;
        }
        if (isal(c)) {
            je = j;
            while (je < e) {
                if (isal(macpool[je] & 255)) { je = je + 1; continue; }
                if (isdi(macpool[je] & 255)) { je = je + 1; continue; }
                break;
            }
            pi = 0 - 1;
            if (isfn) pi = paramat(mi, j, je - j);
            if (pi >= 0) {
                /* C99 6.10.3.1: an argument is fully macro-expanded BEFORE
                   it is substituted -- unless it is an operand of # or ##,
                   which are handled above and use the raw text.  Without
                   this, `XSTR(VER)` stringizes `VER` instead of its value. */
                if (pi < nargs) { if (pi < MAXMPARAM) {
                    emitrange(argo[pi], argo[pi] + argl[pi], depth + 1);
                } }
            }
            else { k = j; while (k < je) { eput(macpool[k] & 255); k = k + 1; } }
            j = je;
            /* A paste makes ONE token; whatever follows it in the body is a
               different one.  `#define Q(A,B) A ## B+` used with `Q(+,)3`
               must give `+ +3`, not `++3`. */
            if (pastejust) { eput(32); pastejust = 0; }
            continue;
        }
        eput(c); j = j + 1;
        if (pastejust) { eput(32); pastejust = 0; }
    }
    return 0;
}

int gchanged;

/* One pass over src[from..to), writing the expansion into ebuf.  `depth` is
   how deep we are inside macro arguments -- bounded, because a macro that
   mentions itself would otherwise never finish. */
int emitrange(int from, int to, int depth) {
    int i; int j; int k; int c; int m; int ni;
    int sargo[MAXMPARAM]; int sargl[MAXMPARAM]; int snargs;
    i = from;
    if (depth > 8) { eputsrc(from, to); return 0; }
    while (i < to) {
        c = src[i] & 255;
        if (c == 34) {
            eput(c); i = i + 1;
            while (i < to) {
                if ((src[i] & 255) == 92) { eput(92); i = i + 1;
                    if (i < to) { eput(src[i] & 255); i = i + 1; } continue; }
                eput(src[i] & 255);
                if ((src[i] & 255) == 34) { i = i + 1; break; }
                i = i + 1;
            }
            continue;
        }
        if (c == 39) {
            eput(c); i = i + 1;
            while (i < to) {
                if ((src[i] & 255) == 92) { eput(92); i = i + 1;
                    if (i < to) { eput(src[i] & 255); i = i + 1; } continue; }
                eput(src[i] & 255);
                if ((src[i] & 255) == 39) { i = i + 1; break; }
                i = i + 1;
            }
            continue;
        }
        if (isal(c)) {
            j = identend(i);
            if (j > to) j = to;
            /* the name means what it meant WHERE it is */
            eseg = srcseg[i]; pp_seg = eseg;
            if (ppnow) { eseg = 0; pp_seg = 0 - 1; }
            /* C99 6.10.9: `_Pragma ( string-literal )` is the operator form
               of `#pragma`, and this compiler honours no pragma -- the
               directive form is already dropped -- so the operator form is
               dropped too, whole, the moment it is seen. */
            if (j - i == 7) { if ((src[i] & 255) == 95 && (src[i+1] & 255) == 80
                && (src[i+2] & 255) == 114 && (src[i+3] & 255) == 97
                && (src[i+4] & 255) == 103 && (src[i+5] & 255) == 109
                && (src[i+6] & 255) == 97) {                  /* _Pragma */
                k = j;
                while (k < to) { if (wsat(k) == 0) break; k = k + 1; }
                if (k < to) { if ((src[k] & 255) == 40) {
                    int dep; int inq;
                    dep = 0; inq = 0;
                    while (k < to) {
                        c = src[k] & 255;
                        if (inq) {
                            if (c == 92) k = k + 1;
                            else { if (c == 34) inq = 0; }
                        } else {
                            if (c == 34) inq = 1;
                            if (c == 40) dep = dep + 1;
                            if (c == 41) { dep = dep - 1; if (dep == 0) { k = k + 1; break; } }
                        }
                        k = k + 1;
                    }
                    eput(32);
                    i = k;
                    continue;
                } }
            } }
            m = mfind(src + i, j - i);
            if (m >= 0) {
                if (macfn[m]) {
                    k = j;
                    while (k < to) { if (wsat(k) == 0) { if ((src[k] & 255) != 10) break; } k = k + 1; }
                    if (k < to) { if ((src[k] & 255) == 40) {
                        ni = collectargs(k);
                        if (ni > 0) {
                            if (macnp[m] == 0) { if (nargs == 1) {
                                if (argl[0] == 0) nargs = 0; } }
                            /* `P("x")` for `P(fmt, ...)`: the variable part is
                               empty, not missing */
                            if (macvar[m]) { if (nargs == macnp[m] - 1) {
                                if (nargs < MAXMPARAM) {
                                    argo[nargs] = ni - 1; argl[nargs] = 0;
                                    nargs = nargs + 1;
                                } } }
                            /* `...` takes everything that is left, commas
                               included -- it is ONE argument spelled
                               __VA_ARGS__ */
                            if (macvar[m]) { if (nargs > macnp[m]) {
                                if (macnp[m] > 0) { if (nargs <= MAXMPARAM) {
                                    argl[macnp[m] - 1] =
                                        argo[nargs - 1] + argl[nargs - 1]
                                        - argo[macnp[m] - 1];
                                    nargs = macnp[m];
                                } }
                            } }
                            /* A space on each side: this substitutes TEXT,
                               and a body ending in `+` next to a source `+`
                               would re-lex as `++`.  A real preprocessor
                               works on tokens and cannot merge them. */
                            eput(32);
                            k = 0;
                            while (k < MAXMPARAM) {
                                sargo[k] = argo[k]; sargl[k] = argl[k];
                                k = k + 1;
                            }
                            snargs = nargs;
                            emitbody(m, 1, depth);
                            k = 0;
                            while (k < MAXMPARAM) {
                                argo[k] = sargo[k]; argl[k] = sargl[k];
                                k = k + 1;
                            }
                            nargs = snargs;
                            eput(32);
                            i = ni; gchanged = 1;
                            continue;
                        }
                    } }
                } else {
                    eput(32);
                    emitbody(m, 0, depth);
                    eput(32);
                    i = j; gchanged = 1;
                    continue;
                }
            }
            eputsrc(i, j);
            i = j;
            continue;
        }
        eput(c); i = i + 1;
    }
    return 0;
}

/* An #if line, expanded with the table as it stands (not by segment). */
int ppexpandline(int from, int to) {
    int sv; int base; int n; int k; int r; int sn;
    /* rescanning is done in rounds over the whole text; an #if line gets
       its own rounds, in scratch space past the end of the source */
    sv = nebuf; sn = nsrc; base = nsrc + 1; n = to - from;
    if (base + 4 * n + 4096 >= MAXSRC) { ppb = src + from; ppe = n; return 0; }
    k = 0; while (k < n) { src[base + k] = src[from + k]; k = k + 1; }
    ppnow = 1;
    r = 0;
    while (r < 8) {
        nebuf = 0; gchanged = 0; nsrc = base + n;
        emitrange(base, base + n, 0);
        nsrc = sn;
        if (base + nebuf + 4096 >= MAXSRC) break;
        k = 0; while (k < nebuf) { src[base + k] = ebuf[k]; k = k + 1; }
        n = nebuf;
        if (gchanged == 0) break;
        r = r + 1;
    }
    ppnow = 0; pp_seg = 0 - 1;
    ppb = src + base; ppe = n;
    nebuf = sv;
    return 0;
}

int expround(void) {
    nebuf = 0; gchanged = 0;
    emitrange(0, nsrc, 0);
    return gchanged;
}

int expandsrc(void) {
    int r; int k; int sg;
    /* every byte's segment: how many table changes precede it */
    sg = 0; k = 0; r = 0;
    while (r < nsrc) {
        while (k < nsegpos && segpos[k] <= r) { sg = sg + 1; k = k + 1; }
        srcseg[r] = sg; r = r + 1;
    }
    r = 0;
    while (r < 8) {
        if (expround() == 0) break;
        k = 0;
        while (k < nebuf) { src[k] = ebuf[k]; srcseg[k] = ebseg[k]; k = k + 1; }
        nsrc = nebuf;
        r = r + 1;
    }
    pp_seg = 0 - 1;
    return 0;
}

/* ---- character classes: the lex table's key axis --------------------- */
int OPCH[128];

int charclass(int c) {
    if (c < 0) return 9;                       /* eof */
    if (c == 10) return 1;                     /* nl */
    if (c == 32) return 0;
    if (c == 9) return 0;
    if (c == 13) return 0;
    if (isal(c)) return 2;                     /* A */
    if (isdi(c)) return 3;                     /* d */
    if (c == 34) return 4;                     /* q */
    if (c == 39) return 5;                     /* sq */
    if (c == 47) return 6;                     /* slash */
    if (c == 42) return 7;                     /* star */
    if (c == 46) return 11;                    /* dot: `.5` may start a number */
    if (c < 128) { if (OPCH[c]) return 8; }    /* punct */
    return 10;                                 /* other */
}

int at(int i) { if (i >= nsrc) return 0 - 1; return src[i] & 255; }

/* does src[p..p+L) equal the C string nm? */
int srcin(char *a, int L, char *nm) {
    int k;
    k = 0;
    while (k < L) {
        if (nm[k] == 0) return 0;
        if ((a[k] & 255) != (nm[k] & 255)) return 0;
        k = k + 1;
    }
    if (nm[L] != 0) return 0;
    return 1;
}

int srcis(int p, int L, char *nm) {
    int k;
    k = 0;
    while (k < L) {
        if (nm[k] == 0) return 0;
        if ((src[p + k] & 255) != (nm[k] & 255)) return 0;
        k = k + 1;
    }
    if (nm[L] != 0) return 0;
    return 1;
}


/* C99 phase 2: a backslash-newline pair is deleted, joining the two lines.
   It happens before tokenisation, so it applies inside literals too. */
int splice(void) {
    int i; int j;
    i = 0; j = 0;
    while (i < nsrc) {
        if (src[i] == 92) {                      /* backslash */
            /* a joined line means one fewer newline than the file has */
            if (src[i + 1] == 10) {
                if (nspl < MAXSPL) { spl_at[nspl] = j; nspl = nspl + 1; }
                i = i + 2; continue;
            }
            if (src[i + 1] == 13) { if (src[i + 2] == 10) {
                if (nspl < MAXSPL) { spl_at[nspl] = j; nspl = nspl + 1; }
                i = i + 3; continue;
            } }
        }
        src[j] = src[i]; j = j + 1; i = i + 1;
    }
    nsrc = j;
    return 0;
}

/* ---- the lexer ------------------------------------------------------- */
/* Maximal munch looked at every TOKV entry for every operator, asking the
   vocabulary for its length and offset each time: the lexer's share of the
   vocabulary lookups.  The entries that can match are those whose first
   byte is the operator's, so they are listed once per first byte, in TOKV
   order -- the same candidates in the same order, so the same winner. [J9] */
int pm_first[257]; int pm_k[1024]; int pm_len[1024]; int pm_off[1024]; int pm_built;
int pm_build(void) {
    int c; int k; int n; int p;
    n = 0; c = 0;
    while (c < 256) {
        pm_first[c] = n;
        k = 0;
        while (k < NTOKV) {
            p = voff(TOKV, k);
            if (vlen(TOKV, k) > 0 && isal(TOKV[p] & 255) == 0 && (TOKV[p] & 255) == c && n < 1024) {
                pm_k[n] = k; pm_len[n] = vlen(TOKV, k); pm_off[n] = p; n = n + 1;
            }
            k = k + 1;
        }
        c = c + 1;
    }
    pm_first[256] = n; pm_built = 1;
    return 0;
}
int lex(void) {
    int i; int j; int a; int key[4]; int kind; int st;
    int best; int bl; int k; int p; int L;
    int strpfx;                            /* the `L` of an `L"..."`, or -1 */
    int isf;
    i = 0;
    ntok = 0;
    strpfx = 0 - 1;
    while (1) {
        /* every branch below appends at most one token; none checked */
        if (ntok >= MAXTOK - 2) { printf("too many tokens\n"); return 0 - 1; }
        key[0] = charclass(at(i));
        key[1] = charclass(at(i + 1));
        key[2] = 0; key[3] = 0;
        a = inf(S_LEX, key, 0);                /* the table decides [W-2] */
        if (a == 0) {                          /* skip */
            if (at(i) < 0) {
                tkind[ntok] = 0; tpos[ntok] = i; tlen[ntok] = 0;
                ntok = ntok + 1;
                return ntok;
            }
            i = i + 1;
        } else {
        if (a == 1) { i = i + 1; }             /* nl */
        else {
        if (a == 7) { while (at(i) >= 0) { if (at(i) == 10) break; i = i + 1; } }
        else {
        if (a == 6) { i = i + 2;
            while (at(i) >= 0) {
                if (at(i) == 42) { if (at(i+1) == 47) { i = i + 2; break; } }
                i = i + 1; } }
        else {
        if (a == 2) {                          /* ident */
            j = i;
            while (at(j) >= 0) {
                if (isal(at(j)) == 0) { if (isdi(at(j)) == 0) {
                    int u; u = ucnlen(j);      /* `caf\u00e9` is one name */
                    if (u == 0) break;
                    j = j + u; continue;
                } }
                j = j + 1;
            }
            /* GCC spellings this subset ignores.  The Python lexer drops
               them too, and selfhost compares token streams. */
            if (srcis(i, j - i, "__attribute__")
                | srcis(i, j - i, "__asm__") | srcis(i, j - i, "asm")) {
                k = j;
                while (at(k) == 32 | at(k) == 9 | at(k) == 10 | at(k) == 13)
                    k = k + 1;
                if (at(k) == 40) {
                    st = 0;
                    while (at(k) >= 0) {
                        if (at(k) == 40) st = st + 1;
                        if (at(k) == 41) { st = st - 1;
                            if (st == 0) { k = k + 1; break; } }
                        k = k + 1;
                    }
                    i = k;
                    continue;
                }
            }
            if (srcis(i, j - i, "__extension__") | srcis(i, j - i, "__inline")
                | srcis(i, j - i, "__inline__") | srcis(i, j - i, "__restrict")
                | srcis(i, j - i, "__restrict__") | srcis(i, j - i, "__const")
                | srcis(i, j - i, "__volatile__")
                | srcis(i, j - i, "__signed__")) { i = j; continue; }
            /* a wide CHARACTER constant is an int, so drop the prefix.  A
               wide STRING keeps it -- the token is still a string, and its
               TEXT is the whole `L"..."`, which is what the Python lexer
               emits and what lexdiff compares. */
            if (at(j) == 39) {
                if (j - i == 1) { if (at(i) == 76 | at(i) == 117 | at(i) == 85) {
                    i = j; continue; } }
                if (j - i == 2) { if (at(i) == 117) { if (at(i+1) == 56) {
                    i = j; continue; } } }
            }
            if (at(j) == 34) {
                int wide;
                wide = 0;
                if (j - i == 1) { if (at(i) == 76 | at(i) == 117 | at(i) == 85) wide = 1; }
                if (j - i == 2) { if (at(i) == 117) { if (at(i+1) == 56) wide = 1; } }
                if (wide) { strpfx = i; i = j; continue; }
            }
            kind = vfind(TOKV, NTOKV, src + i, j - i);
            if (kind < 0) kind = 2;            /* id */
            if (kind == 0) kind = 2;           /* "eof" is not a keyword */
            if (kind == 1) kind = 2;           /* nor is "type" */
            if (vfind(TYPEV, NTYPEV, src + i, j - i) >= 0) kind = 1;
            tkind[ntok] = kind; tpos[ntok] = i; tlen[ntok] = j - i;
            ntok = ntok + 1;
            i = j;
        } else {
        if (a == 3) {                          /* num */
            j = i;
            if (at(j) == 48) {                 /* 0x... or 0X... */
                if (at(j+1) == 120 || at(j+1) == 88) { if (at(j+1) != 0 - 1) {
                    j = j + 2;
                    while (at(j) >= 0) {
                        if (isdi(at(j))) { j = j + 1; } else {
                        if (at(j) >= 97) { if (at(j) <= 102) { j = j + 1; } else break; }
                        else { if (at(j) >= 65) { if (at(j) <= 70) { j = j + 1; } else break; }
                               else break; } } }
                } }
            }
            while (isdi(at(j))) j = j + 1;
            /* a floating constant (C99 6.4.4.2): a fraction, an exponent or
               both, then at most one of f F l L -- the extent the Python
               lexer takes, so the two token streams stay equal */
            isf = 0;
            /* the hex floating form: 0x HEX [. HEX] p [+-] DEC.  The integer
               scan above stopped at the `.` or the `p`. */
            if (at(i) == 48 && (at(i + 1) == 120 || at(i + 1) == 88)) {
                int k3; k3 = j;
                if (at(k3) == 46) {
                    k3 = k3 + 1;
                    while (isdi(at(k3)) || (at(k3) >= 97 && at(k3) <= 102)
                           || (at(k3) >= 65 && at(k3) <= 70)) k3 = k3 + 1;
                }
                if (at(k3) == 112 || at(k3) == 80) {
                    int k4; k4 = k3 + 1;
                    if (at(k4) == 43 || at(k4) == 45) k4 = k4 + 1;
                    if (isdi(at(k4))) {
                        isf = 1; j = k4; while (isdi(at(j))) j = j + 1;
                    }
                }
            }
            if (at(i) != 48 || (at(i + 1) != 120 && at(i + 1) != 88)) {
                if (at(j) == 46) { isf = 1; j = j + 1; while (isdi(at(j))) j = j + 1; }
                if (at(j) == 101 || at(j) == 69) {
                    int k2; k2 = j + 1;
                    if (at(k2) == 43 || at(k2) == 45) k2 = k2 + 1;
                    if (isdi(at(k2))) { isf = 1; j = k2; while (isdi(at(j))) j = j + 1; }
                }
            }
            if (isf) { if (at(j) == 102 || at(j) == 70 || at(j) == 108 || at(j) == 76) j = j + 1; }
            /* the suffix letters in any order, as the Python lexer takes
               them: `1llu` is one token, not `1ll` then an identifier `u` */
            else { while (at(j) == 117 || at(j) == 85 || at(j) == 108 || at(j) == 76) j = j + 1; }
            tkind[ntok] = 3; tpos[ntok] = i; tlen[ntok] = j - i;
            ntok = ntok + 1;
            i = j;
        } else {
        if (a == 4) {                          /* str */
            int q;
            j = i + 1;
            while (1) {
                while (at(j) >= 0) {
                    if (at(j) == 92) { j = j + 2; } else {
                    if (at(j) == 34) break; else j = j + 1; } }
                /* no closing quote before the end: the token's length used
                   to count one byte past the buffer [E1] */
                if (at(j) < 0) { err_at(i, "missing terminating '\"' character");
                    nerr = nerr + 1; return 0 - 1; }
                /* C concatenates adjacent string literals [W-12] */
                q = j + 1;
                while (at(q) >= 0) {
                    if (at(q) == 32) { q = q + 1; } else {
                    if (at(q) == 9) { q = q + 1; } else {
                    if (at(q) == 10) { q = q + 1; } else {
                    if (at(q) == 13) { q = q + 1; } else break; } } } }
                /* the next literal may carry its own `L`/`u`/`U`/`u8` */
                if (at(q) == 76 | at(q) == 117 | at(q) == 85) {
                    if (at(q + 1) == 34) q = q + 1;
                    else { if (at(q) == 117) { if (at(q + 1) == 56) {
                        if (at(q + 2) == 34) q = q + 2; } } }
                }
                if (at(q) == 34) { j = q + 1; } else break;
            }
            if (strpfx >= 0) { tpos[ntok] = strpfx; tlen[ntok] = j + 1 - strpfx; }
            else { tpos[ntok] = i; tlen[ntok] = j + 1 - i; }
            tkind[ntok] = 4;
            strpfx = 0 - 1;
            ntok = ntok + 1;
            i = j + 1;
        } else {
        if (a == 5) {                          /* charlit: scan to the quote,
                                                  an escape may be several
                                                  characters long */
            j = i + 1;
            while (at(j) >= 0) {
                if (at(j) == 39) break;
                if (at(j) == 92) j = j + 1;
                j = j + 1;
            }
            if (at(j) < 0) { err_at(i, "missing terminating ' character");
                nerr = nerr + 1; return 0 - 1; }
            tkind[ntok] = 3; tpos[ntok] = i; tlen[ntok] = j + 1 - i;
            ntok = ntok + 1;
            i = j + 1;
        } else {
        if (a == 8) {                          /* op: maximal munch */
            best = 0 - 1; bl = 0;
            if (pm_built == 0) pm_build();
            {   int q; int qe; int c0;
                c0 = at(i);
                q = c0 >= 0 && c0 < 256 ? pm_first[c0] : 0; qe = c0 >= 0 && c0 < 256 ? pm_first[c0 + 1] : 0;
                while (q < qe) {
                    k = pm_k[q]; L = pm_len[q];
                    if (L > bl) {
                        p = pm_off[q];
                        j = 0;
                        while (j < L) { if (at(i + j) != (TOKV[p + j] & 255)) break; j = j + 1; }
                        if (j == L) { best = k; bl = L; }
                    }
                    q = q + 1;
                }
            }
            if (best < 0) { printf("lex: stray char at %d\n", i); return 0 - 1; }
            tkind[ntok] = best; tpos[ntok] = i; tlen[ntok] = bl;
            ntok = ntok + 1;
            i = i + bl;
        } else {
            /* file:line:col like every other diagnosis -- this one printed
               a byte offset, and the damaged-corpus instrument [S-15 C3]
               counted it as a silent exit */
            err_at(i, "unexpected character");
            nerr = nerr + 1;
            return 0 - 1;
        } } } } } } } } }
    }
    return ntok;
}


