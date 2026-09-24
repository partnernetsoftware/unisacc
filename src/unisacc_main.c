/* unisacc -- the compiler itself, in the C subset it compiles.
 *
 * Every table decision goes through infer(), the same integer kernel and the
 * same model blob the Python driver uses.  Classic code (this file) walks; the
 * tables decide.  [T-1] [T-2]
 *
 * Stage 1: the lexer.  `unisacc -tokens f.c` prints the token stream.
 */

#define MAXSRC 1048576
#define MAXTOK 131072

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
int v_slot(char *v) {
    int i; int p; int n;
    if (v == v_last) return v_lastslot;
    i = 0;
    while (i < v_nlists) {
        if (v_lists[i] == v) { v_last = v; v_lastslot = i; return i; }
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
int wsat(int i);
int ppp;             /* cursor into src, while an #if is being evaluated */
int ppe;             /* one past the directive's last character */

int ppcond(void);

void ppws(void) { while (ppp < ppe) { if (wsat(ppp) == 0) break; ppp = ppp + 1; } }

int pplit(char *w, int n) {          /* does the cursor sit on this word? */
    int k;
    if (ppp + n > ppe) return 0;
    k = 0;
    while (k < n) { if ((src[ppp + k] & 255) != (w[k] & 255)) return 0; k = k + 1; }
    if (ppp + n < ppe) { if (isal(src[ppp + n] & 255)) return 0;
                         if (isdi(src[ppp + n] & 255)) return 0; }
    return 1;
}

int ppop(char *w, int n) {           /* consume this operator if it is here */
    int k;
    ppws();
    if (ppp + n > ppe) return 0;
    k = 0;
    while (k < n) { if ((src[ppp + k] & 255) != (w[k] & 255)) return 0; k = k + 1; }
    ppp = ppp + n;
    return 1;
}

int ppnum(void) {                    /* a decimal, hex or octal constant */
    int v; int c; int base;
    v = 0; base = 10;
    if (src[ppp] == 48) {
        if (ppp + 1 < ppe) { c = src[ppp + 1] & 255;
            if (c == 120 | c == 88) { base = 16; ppp = ppp + 2; }
            else { base = 8; ppp = ppp + 1; } }
        else ppp = ppp + 1;
    }
    while (ppp < ppe) {
        c = src[ppp] & 255;
        if (isdi(c)) { if (c - 48 >= base) break; v = v * base + (c - 48); }
        else { if (base == 16) {
                   if (c >= 97) { if (c <= 102) { v = v * 16 + (c - 87); }
                                  else break; }
                   else { if (c >= 65) { if (c <= 70) { v = v * 16 + (c - 55); }
                                         else break; }
                          else break; } }
               else break; }
        ppp = ppp + 1;
    }
    while (ppp < ppe) { c = src[ppp] & 255;
        if (c == 117 | c == 85 | c == 108 | c == 76) ppp = ppp + 1; else break; }
    return v;
}

int ppprim(void) {
    int v; int ns; int ne2; int m; int paren;
    ppws();
    if (ppp >= ppe) return 0;
    if (ppop("!", 1)) { if (ppprim()) return 0; return 1; }
    if (ppop("~", 1)) return 0 - ppprim() - 1;
    if (ppop("-", 1)) return 0 - ppprim();
    if (ppop("+", 1)) return ppprim();
    if (ppop("(", 1)) { v = ppcond(); ppop(")", 1); return v; }
    if (isdi(src[ppp] & 255)) return ppnum();
    if (src[ppp] == 39) {                        /* a character constant */
        ppp = ppp + 1;
        v = src[ppp] & 255;
        if (v == 92) { ppp = ppp + 1; v = src[ppp] & 255;
                       if (v == 110) v = 10; if (v == 116) v = 9;
                       if (v == 48) v = 0; if (v == 114) v = 13; }
        ppp = ppp + 1;
        if (src[ppp] == 39) ppp = ppp + 1;
        return v;
    }
    if (pplit("defined", 7)) {
        ppp = ppp + 7;
        ppws();
        paren = 0;
        if (ppop("(", 1)) paren = 1;
        ppws();
        ns = ppp;
        while (ppp < ppe) { if (isal(src[ppp] & 255) == 0) {
                                if (isdi(src[ppp] & 255) == 0) break; }
                            ppp = ppp + 1; }
        ne2 = ppp;
        if (paren) ppop(")", 1);
        if (mfind(src + ns, ne2 - ns) >= 0) return 1;
        return 0;
    }
    if (isal(src[ppp] & 255)) {
        ns = ppp;
        while (ppp < ppe) { if (isal(src[ppp] & 255) == 0) {
                                if (isdi(src[ppp] & 255) == 0) break; }
                            ppp = ppp + 1; }
        m = mfind(src + ns, ppp - ns);
        if (m >= 0) { if (machas[m]) return macval[m]; }
        return 0;                                /* C99 6.10.1: unknown is 0 */
    }
    ppp = ppp + 1;
    return 0;
}

int ppmul(void) {
    int v; int r;
    v = ppprim();
    while (1) {
        ppws();
        if (ppop("*", 1)) { v = v * ppprim(); }
        else { if (ppop("/", 1)) { r = ppprim(); if (r) v = v / r; else v = 0; }
        else { if (ppop("%", 1)) { r = ppprim(); if (r) v = v % r; else v = 0; }
        else break; } }
    }
    return v;
}

int ppadd(void) {
    int v;
    v = ppmul();
    while (1) {
        ppws();
        if (ppop("+", 1)) v = v + ppmul();
        else { if (ppop("-", 1)) v = v - ppmul(); else break; }
    }
    return v;
}

int ppshift(void) {
    int v;
    v = ppadd();
    while (1) {
        ppws();
        if (ppop("<<", 2)) v = v << ppadd();
        else { if (ppop(">>", 2)) v = v >> ppadd(); else break; }
    }
    return v;
}

int pprel(void) {
    int v;
    v = ppshift();
    while (1) {
        ppws();
        if (ppop("<=", 2)) { if (v <= ppshift()) v = 1; else v = 0; }
        else { if (ppop(">=", 2)) { if (v >= ppshift()) v = 1; else v = 0; }
        else { if (ppop("<", 1)) { if (v < ppshift()) v = 1; else v = 0; }
        else { if (ppop(">", 1)) { if (v > ppshift()) v = 1; else v = 0; }
        else break; } } }
    }
    return v;
}

int ppeq(void) {
    int v;
    v = pprel();
    while (1) {
        ppws();
        if (ppop("==", 2)) { if (v == pprel()) v = 1; else v = 0; }
        else { if (ppop("!=", 2)) { if (v != pprel()) v = 1; else v = 0; }
        else break; }
    }
    return v;
}

int ppband(void) {
    int v;
    v = ppeq();
    while (1) { ppws();
        if (ppp + 1 < ppe) { if (src[ppp] == 38) { if (src[ppp+1] == 38) break; } }
        if (ppop("&", 1)) v = v & ppeq(); else break; }
    return v;
}

int ppbxor(void) {
    int v;
    v = ppband();
    while (1) { ppws(); if (ppop("^", 1)) v = v ^ ppband(); else break; }
    return v;
}

int ppbor(void) {
    int v;
    v = ppbxor();
    while (1) { ppws();
        if (ppp + 1 < ppe) { if (src[ppp] == 124) { if (src[ppp+1] == 124) break; } }
        if (ppop("|", 1)) v = v | ppbxor(); else break; }
    return v;
}

int ppland(void) {
    int v; int r;
    v = ppbor();
    while (1) { ppws(); if (ppop("&&", 2)) { r = ppbor();
                    if (v) { if (r) v = 1; else v = 0; } else v = 0; }
                else break; }
    return v;
}

int pplor(void) {
    int v; int r;
    v = ppland();
    while (1) { ppws(); if (ppop("||", 2)) { r = ppland();
                    if (v) v = 1; else { if (r) v = 1; else v = 0; } }
                else break; }
    return v;
}

int ppcond(void) {
    int v; int a; int b;
    v = pplor();
    ppws();
    if (ppop("?", 1)) {
        a = ppcond();
        ppop(":", 1);
        b = ppcond();
        if (v) return a;
        return b;
    }
    return v;
}

int ppeval(int from, int to) {
    ppp = from; ppe = to;
    if (ppcond()) return 1;
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
int mdef1(char *s) { int n; n = 0; while (s[n]) n = n + 1; return mdefb(s, n, "1", 1, 1); }

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
                if (d == 2) { if (ppeval(ns, i)) flag = 1; }   /* #if */
                if (d == 3) { if (ppeval(ns, i)) flag = 1; }   /* #elif */
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
#define SX_MAXOCC 262144
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
int srcappend(char *t, int n) {
    int k;
    if (nsrc + n >= MAXSRC) { __write(2, "source too large\n", 17); __exit(1); }
    k = 0;
    while (k < n) { src[nsrc] = t[k]; nsrc = nsrc + 1; k = k + 1; }
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

int emitarg(int p) {
    if (p < nargs) { if (p < MAXMPARAM) eputsrc(argo[p], argo[p] + argl[p]); }
    return 0;
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
            tkind[ntok] = 3; tpos[ntok] = i; tlen[ntok] = j + 1 - i;
            ntok = ntok + 1;
            i = j + 1;
        } else {
        if (a == 8) {                          /* op: maximal munch */
            best = 0 - 1; bl = 0;
            k = 0;
            while (k < NTOKV) {
                L = vlen(TOKV, k);
                if (L > bl) {
                    p = voff(TOKV, k);
                    if (isal(TOKV[p] & 255) == 0) {
                        j = 0;
                        while (j < L) { if (at(i + j) != (TOKV[p + j] & 255)) break; j = j + 1; }
                        if (j == L) { best = k; bl = L; }
                    }
                }
                k = k + 1;
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


/* ===================== stage 5: parser + tape emitter =================== */
/* Recursive descent mirroring the Python walker.  Every production choice
 * goes through infer(S_PARSE, ...) -- classic control flow, neural table. */

#define MAXOUT 4194304
#define MAXSYM 4096

char out[MAXOUT];
int nout;
/* The __init body.  It was 64 KB and unchecked: c-testsuite 00205's one
   initialiser writes 92 KB of it, and the overflow ran over whatever the
   linker put next -- the symbol table on Linux ("unknown identifier"),
   something harmless on macOS. */
#define MAXIBUF 4194304
char ibuf[MAXIBUF];
int nibuf;
int toinit;              /* 1 => ec() writes the __init body instead */
int hasinit;

char symname[MAXSYM * 32];
int symkind[MAXSYM];        /* 0 global  1 local  2 function */
int symoff[MAXSYM];         /* local: frame offset */
int symelem[MAXSYM];        /* element width for [] and unary * */
int symptr[MAXSYM];         /* 1 for pointers and arrays */
int symbytes[MAXSYM];       /* what `sizeof` reports for the whole object */
int symdim2[MAXSYM];        /* inner dimension of `a[n][m]`, else 0 */
int symdim3[MAXSYM];        /* `a[n][m][k]`: k, and symdim2 is m*k */
int symunit[MAXSYM];        /* which input file declared it */
int symvar[MAXSYM];         /* a function that takes `...` */
int symuns[MAXSYM];         /* the (element) type is unsigned */
int symtok[MAXSYM];         /* the token that declared it */
int symused[MAXSYM];        /* read at least once (not just assigned) */
int symbool[MAXSYM];        /* ...and it is _Bool, which normalises on store */
int symfp[MAXSYM];          /* holds a function pointer: 1 register, 2 stacked */
int symvla[MAXSYM];         /* a VLA: the frame slot holding its byte count */
int symptrd[MAXSYM]; int symbase[MAXSYM]; int symlab[MAXSYM];
/* Floating point.  A value's floating kind is 0 (an integer), 4 (float) or
   8 (double); like the unsigned bit it describes the object a pointer points
   to, so `*p` of a `double *` is a double. */
int symflt[MAXSYM];
int sympk[MAXSYM * 8];         /* a function's parameter kinds (fkind), first 8 */
int symnpk[MAXSYM];            /* how many; -1: no prototype seen */
int symfpret[MAXSYM];        /* calling it yields a function pointer */
int symrfst[MAXSYM];         /* ...whose call returns a pointer to this struct */
int symcst[MAXSYM];          /* a pointer variable: its call returns this struct's pointer */         /* a VLA: the frame slot holding its byte count */
int symstruct[MAXSYM];      /* index into the struct table, or -1 */

/* ---- struct and union ------------------------------------------------
   Members carry a real C layout -- an `int` member is 4 bytes at a 4-byte
   boundary -- even though a local scalar still lives in an 8-byte slot.
   The two are different questions, and only the struct has to answer the
   first one. */
#define MAXSTRUCT 128
#define MAXMEMB 1024
char stname[MAXSTRUCT * 32];
int stfirst[MAXSTRUCT]; int stcount[MAXSTRUCT];
/* tags are block-scoped (C99 6.2.1): the block depth a tag was defined at,
   and whether that block has closed */
int stdepth[MAXSTRUCT]; int stdead[MAXSTRUCT]; int bdepth;
int vlaslot[64];          /* per block depth: where the pre-VLA stack pointer is kept */
int curvla;               /* the thing in r0 is a VLA: its byte-count slot */
int stsize[MAXSTRUCT]; int stalign[MAXSTRUCT]; int stunion[MAXSTRUCT];
int stopen[MAXSTRUCT];      /* being defined: the tag's type is incomplete */
int nstruct;
char mbname[MAXMEMB * 32];
int mboff[MAXMEMB];     /* byte offset inside the struct */
int mbbytes[MAXMEMB];   /* what `sizeof` reports for the member */
int mbwidth[MAXMEMB];   /* the load/store width: 0 means "aggregate" */
int mbelem[MAXMEMB];    /* element size, for [] on an array member */
int mbptr[MAXMEMB];
int mbstruct[MAXMEMB];
int mbuns[MAXMEMB];
int mbbool[MAXMEMB];        /* the member is _Bool */
/* 1: a member that takes no initialiser slot -- every union member after the
   first (C99 6.7.8p17: a brace list initialises a union's FIRST member) */
int mbskip[MAXMEMB];
int mbpst[MAXMEMB];       /* a pointer member: the struct it points to, or -1 */
int mbflt[MAXMEMB]; int mbptrd[MAXMEMB];
int nmemb;
int declstruct;         /* the struct declspec() just saw, or -1 */
int decldim2;           /* `a[n][m]` -- m, so the first index strides a row */
int decldim3;           /* `a[n][m][k]` -- k; decldim2 is then m*k */
int declunsigned;       /* the specifier said `unsigned` */
int declfp;             /* the declarator was `(*name)(...)`: 1, or 2 if `...` */
int declspecptr;        /* the specifier itself was a pointer typedef */

/* ---- typedef ---------------------------------------------------------
   The lexer cannot mark these: it runs over the whole file before the
   parser sees a line, so a name typedef'd on line 10 is already an ordinary
   identifier by then.  The parser resolves them by text instead. */
#define MAXTD 256
char tdname[MAXTD * 32];
int tdw[MAXTD]; int tdsz[MAXTD]; int tdstruct[MAXTD]; int tdptr[MAXTD];
int tduns[MAXTD];
int tdfp[MAXTD];          /* a function-pointer typedef: 1, or 2 if variadic */
int tdfpst[MAXTD];        /* ...and the struct its call returns a pointer to */
int tdflt[MAXTD]; int tdpd[MAXTD];
int declspecfp;           /* what declspec's typedef said about that */
int declenum;             /* the specifier was an enum */
int enumneg;              /* some enumerator seen so far is negative */
int ntd;
int retst; int rett;      /* the function being walked returns this struct by value, or -1 */
int fnresume;             /* where a nested declarator's body starts, or -1 */
/* Function pointers as VALUES.  curfn: r0 holds one, so `*` of it is itself
   (C99 6.5.3.2p4) -- it loaded eight bytes of code.  curfnst: the struct a
   call through it returns a pointer to, or -1.  fpretfp: the declarator
   just parsed points to a function that itself returns a function pointer. */
int curfn; int curfnst; int fpretfp; int vcst; int vcfn;
int curbool;                /* the lvalue in hand is _Bool [C99 6.3.1.2] */
int fntok = 0 - 1;          /* the name token of the function being walked,
                               for C99's predefined `__func__` */
int curflt; int declflt; int retflt; int retkind; int retsz; int retuns; int slotflt;
/* Pointer DEPTH.  `int **q` is a pointer to a pointer: *q is itself eight
   bytes, and only **q is the int.  A 0/1 pointer flag read *q as a 4-byte
   int -- right in the interpreter, whose addresses fit in 32 bits, and a
   segfault natively.  declpd counts the stars of the declarator being read;
   curpd is the depth of the pointer in r0 and curbase its innermost
   pointee's width. */
int declpd; int declspecpd; int declbase; int curpd; int curbase;
/* `static` inside a function: the object has static storage, named `ls<N>`
   (N the name's token, unique in the unit), initialised once in __init.
   As a frame slot it was whatever the stack last held -- the interpreter's
   fresh, reused stack counted 1 2 3 by luck; a native image did not. */
int declstatic;
int declbool;               /* the declared type is _Bool */
int initflt;              /* an initialiser's element kind, for its slots */
int declspecfpst;         /* a function-pointer typedef's call-result struct */
int havepre;              /* binary()'s leftmost operand is already in r0 */
int initisarr; int initrows; int initrows3; /* the next initaggr is an array; its row lengths */
int fpdim;       /* `(*fs[2])(...)`: an array of that many pointers, or 0 */
int fpadim;      /* `(*p)[4]`: a pointer to an array of that many, or 0 */
int fpfn;        /* `(*f(params))(...)`: f is a FUNCTION; its params start here */
int curstruct;          /* the struct the thing in r0 is, or -1 */
int curdim2;            /* ...and its inner dimension, if it has one */
int curdim3;
int curuns;             /* ...and whether its type is unsigned */
int binuns;             /* the operator in hand works unsigned */
int binwid;             /* ...and the width it wraps at */
int fnvar;              /* the function being compiled takes `...` */
int fnnfixed;           /* ...and this many named parameters */
int nsym;
int scopebase;              /* first local of the current function */

int tp;                     /* current token */
int nlab;                   /* label counter */
int frameoff;               /* bytes of locals allocated so far */
int framemax;
int retlab;

int P_END; int P_GLOBAL; int P_TYPEDEF; int P_STRUCT; int P_ENUM;
int P_DECL; int P_IF; int P_WHILE; int P_FOR; int P_DO; int P_SWITCH;
int P_CASE; int P_DEFAULT; int P_RETURN; int P_BREAK; int P_CONTINUE;
int P_BLOCK; int P_EXPR; int P_NEG; int P_NOT; int P_DEREF; int P_ADDR;
int P_SIZEOF; int P_PRIM; int P_INDEX; int P_CALL; int P_INC; int P_FIELD;
int P_DONE; int P_FNSIG; int P_VARDEF; int P_GOTO;
int P_BNOT; int P_UPLUS; int P_PREINC;
int T_EOF; int T_TYPE; int T_ID; int T_NUM; int T_STR;

int pidx(char *n, int L) { return vfind(PRODV, NPRODV, n, L); }
int tidx(char *n, int L) { return vfind(TOKV, NTOKV, n, L); }

/* forward declarations: our own compiler resolves calls late, but a reference
 * compiler wants them up front */
int expr(void);
int exprc(void);
int unary(void);
int cexpr(void);
int cond(void);
int mbfind(int si, int t);
int eload(int w);
int estore(int w);
int is_typeat(int i);
int typesize(void);
int declspec(void);
int primary(void);
int postfix(void);
int vcall(int var);
int fpdecl(void);
int eatstar(void);
/* which target this binary itself runs on, for `-run` [S-9] */
#ifdef __linux__
#ifdef __aarch64__
#define HOST_TARGET "lnx/arm64"
#else
#define HOST_TARGET "lnx/x86_64"
#endif
#else
#ifdef __aarch64__
#define HOST_TARGET "osx/arm64"
#else
#define HOST_TARGET "osx/x86_64"
#endif
#endif

int bk_build(char *t, int n, char *target);
int bkfd = 1;    /* where the image goes: -o, else stdout.  DEFINED here,
                    once: two definitions of one global become two .bss
                    lines, and the two back ends disagree about which of the
                    two addresses the name means. */
long bk_run(char *t, int n, long argc, long argv);   /* -run [S-9] */
char *runargv[256];
int symlea(int i, int t, char *reg);
int dkind(int flt);
int setkind(int k);
int fltlit(int t);
unsigned long fdec2bin(int p, int n, int f32);
unsigned long fhex2bin(int p, int n, int f32);
int fconv(int from, int to);
int fkind(void);
int argconv(int si, int k);
int callres(int si);
int ftruthy(void);
long fone(int k);
int skipparen(void);
int iswide(int t);
int wdecode(int t, int *cp);
int strw(int t);
int wcp[4096];
int initcountat(int j);
int initaggr(int isglobal, int gt, int off, int w, int sst, int nbytes);
int structslots(int sst);
int cplitexpr(int w, int sst, int isarr, int n);
int emit_binop(int k);
int stmt(void);
int block(void);
int local_decl(void);
int pf_call(int t);
int do_printf(void);
int is_typetok(void);
int push(void);
int pop1(void);
int loadval(void);
int zext(int w);
int newlab(void);
int elab(char *p, int n);
int en(long v);
int ec(int c);
int etok(int i);
long numval(int t);
int alloc_local(int n);
int sadd(int t, int kind, int off, int elem);
int addlit(char *b, int n);
int decode(int t, char *buf);

int ec(int c) {
    if (toinit) {
        if (nibuf >= MAXIBUF) { __write(2, "initialiser code too large\n", 27); __exit(1); }
        ibuf[nibuf] = c; nibuf = nibuf + 1; return 0;
    }
    if (nout >= MAXOUT) { __write(2, "output buffer full\n", 19); __exit(1); }
    out[nout] = c; nout = nout + 1; return 0;
}

/* ---- irsel: every instruction name is chosen by the net [W-6] -----------
   The emitter writes `@family.flavor` where the Python walker calls
   `recipe(family, flavor)`, and the name is resolved HERE, at the one exit
   every emitted byte passes through -- so no tape mnemonic in this file is
   a hand-made choice.  The recipe -> mnemonic step afterwards is spelling
   (RECIPE_OP on the Python side), not a decision. */
int nask[16];

/* The oracle is a PURE FUNCTION of (stage, key, head) over a domain of
   6,650 keys, and the compiler asks it once per token and once per lowered
   instruction -- so the same question arrives thousands of times and a
   sampling profile of the self-compile put `infer` at the top.
   The answers are memoised.  This does not decide anything: a miss still
   runs the net, the ANSWER is the net's, and `nask` still counts every
   ask, so [A-33] `stages.sh` and `-c -v` see exactly what they saw before.
   Direct-mapped, so a collision costs one extra inference and never a
   wrong answer -- the tag is compared before the value is used. */
#define INFC 16384
/* The slot stores the WHOLE question -- stage, head and all four key
   fields -- and a hit compares every one of them.  The first version
   stored a hash and compared the hash: over the full domain two of the
   15,221 questions share one, so a hit could hand back another question's
   answer without a sound.  And that hash overflowed a signed int, which is
   undefined behaviour, so clang and gcc computed different ones and the
   wrong answer moved between compilers -- Linux CI (gcc) refused a C99
   program the Mac (clang) compiled.  Now the hash only chooses the slot;
   it cannot change an answer, and it cannot overflow: each step is
   reduced before the next multiply. */
int infc_st[INFC]; int infc_hd[INFC]; int infc_k0[INFC]; int infc_k1[INFC];
int infc_k2[INFC]; int infc_k3[INFC]; int infc_val[INFC]; int infc_live[INFC];

int inf(int st, int *key, int head) {
    int r; int h;
    h = st & (INFC - 1);
    h = (h * 31 + (head & 1023)) & (INFC - 1);
    h = (h * 31 + (key[0] & 1023)) & (INFC - 1);
    h = (h * 31 + (key[1] & 1023)) & (INFC - 1);
    h = (h * 31 + (key[2] & 1023)) & (INFC - 1);
    h = (h * 31 + (key[3] & 1023)) & (INFC - 1);
    nask[st] = nask[st] + 1;
    if (infc_live[h]) {
        if (infc_st[h] == st && infc_hd[h] == head && infc_k0[h] == key[0]
            && infc_k1[h] == key[1] && infc_k2[h] == key[2]
            && infc_k3[h] == key[3]) return infc_val[h];
    }
    r = infer(st, key, head);
    if (r < 0) { __write(2, "oracle: key outside the stage's domain\n", 39); __exit(1); }
    infc_st[h] = st; infc_hd[h] = head; infc_k0[h] = key[0];
    infc_k1[h] = key[1]; infc_k2[h] = key[2]; infc_k3[h] = key[3];
    infc_val[h] = r; infc_live[h] = 1;
    return r;
}

/* `unisacc --check-oracle` [A-49]: every question the model can be asked,
   through the cache, compared with the net answering it directly.  Twice --
   forwards and backwards -- because which question evicts which depends on
   the order, and a cache that answers wrong answers wrong only for SOME
   order.  The first version of the cache stored a hash of the question
   instead of the question; this is the check that would have caught it. */
int oracle_n;
int oracle_pass(int dir) {
    int s; int h; int f; int m; int bad; int n; int r0; int r1; int done;
    int key[4]; int lim[4];
    bad = 0; n = 0;
    s = dir > 0 ? 0 : NSTAGE - 1;
    while (s >= 0 && s < NSTAGE) {
        m = STAGE_M[s];
        h = 0;
        while (h < 12) {
            if (STAGE_NCLS[s * 12 + h] > 0) {
                f = 0;
                while (f < 4) {
                    lim[f] = f < m ? STAGE_VN[(s << 2) + f] : 1;
                    key[f] = dir > 0 ? 0 : lim[f] - 1;
                    f = f + 1;
                }
                done = 0;
                while (done == 0) {
                    r1 = inf(s, key, h);
                    r0 = infer(s, key, h);
                    n = n + 1;
                    if (r0 != r1) bad = bad + 1;
                    /* the odometer, in this pass's direction */
                    f = 0;
                    while (1) {
                        if (f >= 4) { done = 1; break; }
                        if (dir > 0) {
                            key[f] = key[f] + 1;
                            if (key[f] < lim[f]) break;
                            key[f] = 0;
                        } else {
                            key[f] = key[f] - 1;
                            if (key[f] >= 0) break;
                            key[f] = lim[f] - 1;
                        }
                        f = f + 1;
                    }
                }
            }
            h = h + 1;
        }
        s = s + dir;
    }
    oracle_n = n;
    return bad;
}

int irsel(char *fam, int fl, char *flav, int vl) {
    int key[4];
    key[0] = vfind(IRFAMV, NIRFAMV, fam, fl);
    key[1] = vfind(IRFLAV, NIRFLAV, flav, vl);
    key[2] = 0; key[3] = 0;
    if (key[0] < 0) { __write(2, "irsel: no family\n", 17); __exit(1); }
    if (key[1] < 0) { __write(2, "irsel: no flavor\n", 17); __exit(1); }
    return inf(S_IRSEL, key, 0);
}

int recrev(int r) {                   /* does recipe r end in `_rev`? */
    int p; int L;
    p = voff(IRRECV, r); L = vlen(IRRECV, r);
    if (L < 4) return 0;
    return srcin(IRRECV + p + L - 4, 4, "_rev");
}

int emitrecipe(int r) {
    int p; int L; int k;
    p = voff(IRRECV, r); L = vlen(IRRECV, r);
    if (recrev(r)) L = L - 4;          /* spelled as its op; es() swaps */
    if (srcin(IRRECV + p, L, "bad")) { __write(2, "irsel: bad recipe\n", 18); __exit(1); }
    if (srcin(IRRECV + p, L, "callpush")) { ec(99); ec(97); ec(108); ec(108); return 0; }
    /* the builtins are spelled with a leading dot */
    if (srcin(IRRECV + p, L, "lea")) ec(46);
    if (srcin(IRRECV + p, L, "ld")) ec(46);
    if (srcin(IRRECV + p, L, "st")) ec(46);
    if (srcin(IRRECV + p, L, "zero")) ec(46);
    if (srcin(IRRECV + p, L, "arg")) ec(46);
    if (srcin(IRRECV + p, L, "frame")) ec(46);
    if (srcin(IRRECV + p, L, "print")) ec(46);
    if (srcin(IRRECV + p, L, "write")) ec(46);
    if (srcin(IRRECV + p, L, "exit")) ec(46);
    k = 0;
    while (k < L) { ec(IRRECV[p + k] & 255); k = k + 1; }
    return 0;
}

int es(char *s) {
    int i; int fs; int fe; int vs; int ve; int r; int k;
    i = 0;
    while (s[i]) {
        if ((s[i] & 255) == 64) {                   /* @family.flavor */
            fs = i + 1; fe = fs;
            while (s[fe]) { if ((s[fe] & 255) == 46) break; fe = fe + 1; }
            vs = fe + 1; ve = vs;
            /* a flavor may have digits: `i2d`, `s2d` */
            while (s[ve]) { if (isal(s[ve] & 255) == 0) { if (isdi(s[ve] & 255) == 0) break; } ve = ve + 1; }
            r = irsel(s + fs, fe - fs, s + vs, ve - vs);
            emitrecipe(r);
            i = ve;
            /* a `_rev` recipe swaps the two sources: " rD, rA, rB" is
               written " rD, rB, rA" -- the net decided, not the caller */
            if (recrev(r)) {
                int c1; int c2; int e;
                c1 = i; while (s[c1] && s[c1] != 44) c1 = c1 + 1;
                c2 = c1 + 1; while (s[c2] && s[c2] != 44) c2 = c2 + 1;
                e = c2 + 1; while (s[e] && s[e] != 10) e = e + 1;
                k = i; while (k <= c1) { ec(s[k] & 255); k = k + 1; }
                k = c2 + 1; while (k < e) { ec(s[k] & 255); k = k + 1; }
                ec(44);
                k = c1 + 1; while (k < c2) { ec(s[k] & 255); k = k + 1; }
                i = e;
            }
            continue;
        }
        ec(s[i] & 255);
        i = i + 1;
    }
    return 0;
}

int en(long v) {
    char b[24]; int n; int neg; unsigned long u;
    neg = 0;
    /* through an unsigned: -v of LONG_MIN is itself, and a signed loop over
       it printed a bare '-' -- the sign bit of a double is exactly that */
    u = v;
    if (v < 0) { neg = 1; u = 0 - u; }
    n = 0;
    if (u == 0) { b[0] = 48; n = 1; }
    while (u > 0) { b[n] = 48 + (u % 10); u = u / 10; n = n + 1; }
    if (neg) ec(45);
    while (n > 0) { n = n - 1; ec(b[n] & 255); }
    return 0;
}

/* C99 6.4.4.1: an integer constant may be hexadecimal, octal or decimal, and
   may carry a u/U/l/L suffix.  Four copies of `v = v * 10 + (c - 48)` got all
   three wrong -- `0xff` came out 7794, `010` came out 10, and `5L` came out
   78, because 'L' - '0' is 28. */
long numval(int t) {
    int k; int n; long v; int c; int base; int d;
    k = 0; n = tlen[t]; v = 0; base = 10;
    /* A character constant is a NUMBER token too, and it had never been
       evaluated: the decimal loop read `'b'` as quote-minus-'0' and so on,
       and nothing noticed until <stdio.h> was compiled here and every
       `putchar('c')` wrote a NUL. */
    if ((src[tpos[t]] & 255) == 39) {
        c = src[tpos[t] + 1] & 255;
        if (c != 92) return c;
        c = src[tpos[t] + 2] & 255;
        if (c == 110) return 10;                  /* \n */
        if (c == 116) return 9;                   /* \t */
        if (c == 114) return 13;                  /* \r */
        if (c == 118) return 11;                  /* \v */
        if (c == 102) return 12;                  /* \f */
        if (c == 98) return 8;                    /* \b */
        if (c == 97) return 7;                    /* \a */
        if (c == 120) {                           /* \xHH */
            k = 3; v = 0;
            while (k < n - 1) {
                c = src[tpos[t] + k] & 255; d = 0 - 1;
                if (isdi(c)) d = c - 48;
                if (c >= 97) { if (c <= 102) d = c - 87; }
                if (c >= 65) { if (c <= 70) d = c - 55; }
                if (d < 0) break;
                v = v * 16 + d; k = k + 1;
            }
            return v;
        }
        if (isdi(c)) {                            /* \ooo */
            k = 2; v = 0;
            while (k < n - 1) {
                c = src[tpos[t] + k] & 255;
                if (c < 48) break;
                if (c > 55) break;
                v = v * 8 + (c - 48); k = k + 1;
            }
            return v;
        }
        return c;                                 /* \\ \' \" \? */
    }
    if (n > 1) { if ((src[tpos[t]] & 255) == 48) {
        c = src[tpos[t] + 1] & 255;
        if (c == 120) { base = 16; k = 2; }
        else { if (c == 88) { base = 16; k = 2; }
               else { base = 8; k = 1; } }
    } }
    while (k < n) {
        c = src[tpos[t] + k] & 255;
        d = 0 - 1;
        if (isdi(c)) d = c - 48;
        if (base == 16) {
            if (c >= 97) { if (c <= 102) d = c - 87; }
            if (c >= 65) { if (c <= 70) d = c - 55; }
        }
        if (d < 0) break;                 /* a suffix, or the end */
        if (d >= base) break;
        v = v * base + d;
        k = k + 1;
    }
    return v;
}

/* One string, in one place: a release is identifiable from the binary. */
#define UNISACC_VERSION "0.0.6"
/* Are two NUL-terminated strings the same? */
int strpre(char *a, char *p) {          /* a starts with p */
    int k;
    k = 0;
    while (p[k]) { if (a[k] != p[k]) return 0; k = k + 1; }
    return 1;
}
int strsame(char *a, char *b) {
    int k;
    k = 0;
    while (a[k] && b[k]) { if (a[k] != b[k]) return 0; k = k + 1; }
    return a[k] == b[k];
}
int curunit;                /* which input file is being walked */
int ustat_is(int t);
int etok(int i) {
    int k; int u;
    k = 0; while (k < tlen[i]) { ec(src[tpos[i] + k] & 255); k = k + 1; }
    /* the suffix travels with the name, so a definition and every use of it
       inside the unit spell it the same way */
    if (ustat_is(i)) {
        es("__u"); u = curunit;
        if (u >= 10) ec(48 + u / 10);
        ec(48 + u % 10);
    }
    return 0;
}

int newlab(void) { nlab = nlab + 1; return nlab; }
int elab(char *p, int n) { es(p); en(n); return 0; }

/* ---- symbols --------------------------------------------------------- */
/* sfind by hash chain.  The backward walk over every symbol was 10% of
   the self-built compiler compiling itself.  A chain holds indices newest
   first, so its first live match is the walk's first match.  Scopes pop
   by lowering nsym; the dead entries (>= nsym) sit at the FRONT of their
   chains, are skipped on lookup, and are unlinked when their slot is
   reused.  A name of 32 or more characters is stored truncated, so what
   it matches is not its hash's business: those go on one chain of their
   own (SH_LONG) that every lookup merges in, and a long query walks. */
#define SH_SIZE 8192
#define SH_LONG SH_SIZE
int sh_head[SH_SIZE + 1];                 /* index + 1, 0: empty */
int sh_link[MAXSYM]; int sh_b[MAXSYM]; int sh_hi;
int sh_bucket(int t) {
    if (tlen[t] >= 32) return SH_LONG;
    return vhash(src + tpos[t], tlen[t]) & (SH_SIZE - 1);
}
int sh_push(int i, int t) {
    int ob; int b;
    if (i < sh_hi) {                      /* reusing a dead slot */
        ob = sh_b[i];
        while (sh_head[ob] - 1 >= i) sh_head[ob] = sh_link[sh_head[ob] - 1];
    } else sh_hi = i + 1;
    b = sh_bucket(t);
    sh_b[i] = b; sh_link[i] = sh_head[b]; sh_head[b] = i + 1;
    return 0;
}
int sfind_is(int i, int t) {
    int k; int ok; int n;
    n = tlen[t];
    k = 0; ok = 1;
    while (symname[i * 32 + k]) {
        if (k >= n) ok = 0;
        if (ok) { if (symname[i * 32 + k] != src[tpos[t] + k]) ok = 0; }
        k = k + 1;
    }
    if (ok) { if (k == n) return 1; }
    return 0;
}
int sfind_walk(int t) {
    int i;
    i = nsym - 1;
    while (i >= 0) { if (sfind_is(i, t)) return i; i = i - 1; }
    return 0 - 1;
}
int sfind(int t) {
    int a; int b;
    if (tlen[t] >= 32) return sfind_walk(t);
    a = sh_head[sh_bucket(t)] - 1; b = sh_head[SH_LONG] - 1;
    while (a >= nsym) a = sh_link[a] - 1;
    while (b >= nsym) b = sh_link[b] - 1;
    while (a >= 0 || b >= 0) {
        if (a > b) { if (sfind_is(a, t)) return a; a = sh_link[a] - 1; }
        else { if (sfind_is(b, t)) return b; b = sh_link[b] - 1; }
    }
    return 0 - 1;
}

char *inputs[64]; int ninput;   /* the input files, in order */
/* Two units may each have a `static helper`, and they are DIFFERENT
   functions; with no linker there is one namespace, so the later unit's
   statics carry a suffix.  Unit 0 keeps its names, which is why compiling
   one file produces the same tape it always did. */
#define MAXUSTAT 512
char ustat[MAXUSTAT * 32]; int ustat_u[MAXUSTAT]; int nustat;
int ustat_add(int t) {
    int k;
    if (curunit == 0) return 0;
    if (nustat >= MAXUSTAT) return 0;
    k = 0;
    while (k < tlen[t] && k < 31) { ustat[nustat * 32 + k] = src[tpos[t] + k]; k = k + 1; }
    ustat[nustat * 32 + k] = 0;
    ustat_u[nustat] = curunit; nustat = nustat + 1;
    return 0;
}
/* The function called `nm` that unit 0 defined, or -1: unit 0's statics
   keep their names, so this is the label a call would use. */
int symfn(char *nm, int L) {
    int i; int k; int ok;
    i = 0;
    while (i < nsym) {
        if (symkind[i] == 2) { if (symunit[i] == 0) {
            ok = 1; k = 0;
            while (k < L) { if (symname[i * 32 + k] != nm[k]) ok = 0; k = k + 1; }
            if (symname[i * 32 + L] != 0) ok = 0;
            if (ok) return i;
        } }
        i = i + 1;
    }
    return 0 - 1;
}
/* Is this token a static of the unit being walked? */
int ustat_is(int t) {
    int i; int k; int ok;
    if (curunit == 0) return 0;
    i = 0;
    while (i < nustat) {
        if (ustat_u[i] == curunit) {
            k = 0; ok = 1;
            while (ustat[i * 32 + k]) {
                if (k >= tlen[t]) { ok = 0; break; }
                if (ustat[i * 32 + k] != src[tpos[t] + k]) { ok = 0; break; }
                k = k + 1;
            }
            if (ok) { if (k == tlen[t]) return 1; }
        }
        i = i + 1;
    }
    return 0;
}
int pponly;                 /* -E: stop after the preprocessor */
int gdup;                   /* this global was already defined further up */
int declptr;                /* set by the declarator being processed */
int declsz;                 /* the declared type's size, for `sizeof` */
int declbytes;              /* ...times the array length, if it is one */
char lbuf[131072];      /* a string literal can be the whole model blob */
int needslen; int needchb; int needxb;

int sh_push(int i, int t);
int sadd(int t, int kind, int off, int elem) {
    int k;
    if (nsym >= MAXSYM) { __write(2, "symbol table full\n", 18); __exit(1); }
    k = 0;
    while (k < tlen[t]) { if (k < 31) symname[nsym * 32 + k] = src[tpos[t] + k]; k = k + 1; }
    if (tlen[t] < 32) symname[nsym * 32 + tlen[t]] = 0;
    symkind[nsym] = kind; symoff[nsym] = off; symelem[nsym] = elem;
    symptr[nsym] = declptr;
    symbytes[nsym] = declbytes;
    symstruct[nsym] = declstruct;
    symdim2[nsym] = decldim2;
    symdim3[nsym] = decldim3;
    symvar[nsym] = 0;
    symunit[nsym] = curunit;
    symuns[nsym] = declunsigned;
    symtok[nsym] = t; symused[nsym] = 0;
    symbool[nsym] = declbool;
    symfp[nsym] = declfp;
    symvla[nsym] = 0;
    symfpret[nsym] = 0; symrfst[nsym] = 0 - 1; symcst[nsym] = 0 - 1;
    symflt[nsym] = declflt; symnpk[nsym] = 0 - 1;
    symptrd[nsym] = 0; symlab[nsym] = 0 - 1;
    if (declptr) symptrd[nsym] = declpd > 0 ? declpd : 1;
    symbase[nsym] = declbase;
    sh_push(nsym, t);
    nsym = nsym + 1;
    return nsym - 1;
}

int mfindt(int t) { return mfind(src + tpos[t], tlen[t]); }

/* ---- token helpers --------------------------------------------------- */
int kind(int i) { if (i >= ntok) return T_EOF; return tkind[i]; }
int cur(void) { return kind(tp); }
int adv(void) { tp = tp + 1; return tp - 1; }
int eat(int k) { if (cur() == k) { tp = tp + 1; return 1; } return 0; }

int need(int k, char *what) {
    char msg[64]; int n; int q;
    if (cur() == k) { tp = tp + 1; return 1; }
    /* "expected ';'" -- and the caret goes on the token that is NOT it,
       which is where the eye looks anyway */
    n = 0;
    while ("expected "[n]) { msg[n] = "expected "[n]; n = n + 1; }
    msg[n] = 39; n = n + 1;                         /* a quote */
    q = 0;
    while (what[q] && n < 60) { msg[n] = what[q]; n = n + 1; q = q + 1; }
    msg[n] = 39; n = n + 1;
    msg[n] = 0;
    err_tok(tp, msg);
    return 0;
}

int tdfind(int t);
int infunc;                        /* inside a function body */

/* [W-5] the scope table, asked at the same points and with the same key as
   the Python walker's `sc.act(ctx, tok)`.  Its answer steers nothing on
   EITHER side yet -- see prd.md E-52 -- but a stage the self-hosted
   compiler does not even ask is a stage it has quietly dropped. */
int scopedecl;
int scopeact(char *ctx, int cl, int t) {
    int key[4]; int k;
    k = vfind(SKINDV, NSKINDV, "id", 2);
    if (kind(t) == T_TYPE) k = vfind(SKINDV, NSKINDV, "type_kw", 7);
    /* struct/union/enum open a type name exactly as `int` does */
    if (kind(t) == tidx("struct", 6)) k = vfind(SKINDV, NSKINDV, "type_kw", 7);
    if (kind(t) == tidx("union", 5)) k = vfind(SKINDV, NSKINDV, "type_kw", 7);
    if (kind(t) == tidx("enum", 4)) k = vfind(SKINDV, NSKINDV, "type_kw", 7);
    if (tdfind(t) >= 0) k = vfind(SKINDV, NSKINDV, "typedef_id", 10);
    if (kind(t) == tidx("*", 1)) k = vfind(SKINDV, NSKINDV, "star", 4);
    if (kind(t) == tidx("(", 1)) k = vfind(SKINDV, NSKINDV, "lparen", 6);
    /* a name in declarator or member position is a name by position: a
       typedef name there is being shadowed (C99 6.2.1p4) */
    if (scopedecl) k = vfind(SKINDV, NSKINDV, "id", 2);
    key[0] = vfind(SCTXV, NSCTXV, ctx, cl); key[1] = k; key[2] = 0; key[3] = 0;
    return inf(S_SCOPE, key, 0);
}

int lbind; int gbind;
int actis(int a, char *nm, int L) { return a == vfind(SACTV, NSACTV, nm, L); }

int scopefail(char *what, int t) {
    /* The table would not bind this name where the walker stands.  On a
       valid program that is the two disagreeing -- an internal alarm; on
       the programs people actually hand a compiler it is a missing `;`
       before a declarator, and it is reported as that, at the token. */
    err_tok(t, "expected ';' before this declarator");
    return 0;
}

/* [W-5] The table DECIDES where a declared name binds: its answer picks the
   symbol's storage -- 0 a global (a data label), 1 a frame slot. */
int scopebind(char *ctx, int cl, int t) {
    int a;
    scopedecl = 1; a = scopeact(ctx, cl, t); scopedecl = 0;
    if (actis(a, "bind_global", 11)) return 0;
    if (actis(a, "bind_local", 10)) return 1;
    if (actis(a, "bind_param", 10)) return 1;
    scopefail("a binding", t);
    return 0 - 1;
}

int scopewant(char *ctx, int cl, int t, char *nm, int L) {
    if (actis(scopeact(ctx, cl, t), nm, L) == 0) scopefail(nm, t);
    return 0;
}

/* Project the current token onto the parse table's TOK axis.  A typedef name
   lexes as an identifier -- the lexer runs over the whole file before the
   parser sees a line -- but the GRAMMAR has to see a type, or `myint x;`
   goes to the table's `expr` default and is parsed as an expression. */
int tokclass(void) {
    int k;
    k = cur();
    if (k == T_ID) { if (tdfind(tp) >= 0) return T_TYPE; }
    return k;
}

int ask(int nt) { int key[4]; key[0] = nt; key[1] = tokclass(); key[2] = 0; key[3] = 0;
                  return inf(S_PARSE, key, 0); }

/* ---- expressions ----------------------------------------------------- */
int expr(void);

int push(void) { es("  @call.frame 8\n  @mem.store [r7+0], r0\n"); return 0; }
int pop1(void) { es("  @mem.load r1, [r7+0]\n  @call.frame -8\n"); return 0; }

int lvalue;        /* 1 when r0 holds an ADDRESS, not a value */
int curelem;       /* element width of the thing in r0 */
int curptr;        /* 1 when the VALUE in r0 is a pointer (scales +/-) */
int cursize;       /* what `sizeof` would report for the thing in r0 */

/* storage width vs element width: a `char *p` is stored in 8 bytes but its
 * element is 1.  Conflating them stores a single byte of the pointer. */
int stw(void) { if (curptr) return 8; return curelem; }

/* `int` is four bytes here, the same as everywhere else in C, so a load has
   to say how wide it is.  Width 0 means an AGGREGATE -- a struct or an array
   -- whose value is its address, so there is nothing to load. */
/* A struct VALUE is its address; copying one is n bytes [r1] -> [r0].  r0
   survives, so the destination is still the result afterwards. */
int scopy(int n) {
    int k; int w;
    k = 0;
    while (k < n) {
        w = 8;
        while (k + w > n) w = w / 2;
        if (w == 8) { es("  @mem.load r2, [r1+"); en(k); es("]\n  @mem.store [r0+"); en(k); es("], r2\n"); }
        else { es("  @mem.ld r2, [r1+"); en(k); es("], "); en(w);
               es("\n  @mem.st [r0+"); en(k); es("], r2, "); en(w); ec(10); }
        k = k + w;
    }
    return 0;
}

/* Copy the struct value in r0 into a fresh frame slot and leave ITS address
   in r0: a value that outlives the next call (a callee's return buffer is
   shared by every call of it). */
int stemp(int st) {
    int o;
    o = alloc_local(stsize[st]);
    es("  mov r1, r0\n  @lit.imm r0, "); en(o); es("\n  @alu.sub r0, r6, r0\n");
    scopy(stsize[st]);
    return 0;
}

/* ---- bit-fields ------------------------------------------------------
   A bit-field has no address, so its "width" carries everything a load or a
   store needs: the storage unit's size, where in it the field starts, how
   wide it is, and whether it is signed.  Every path that loads or stores
   goes through eload/estore, so reads, writes, ++, `op=` and initialisers
   all work on bit-fields without knowing about them. */
#define BFTAG 1048576
int bfenc(int width, int bofs, int ub, int sig) {
    int ul;
    ul = 0;
    if (ub == 2) ul = 1;
    if (ub == 4) ul = 2;
    if (ub == 8) ul = 3;
    return BFTAG + width + 128 * bofs + 16384 * ul + 65536 * sig;
}
int bfwidth(int w) { return (w - BFTAG) % 128; }
int bfofs(int w) { return ((w - BFTAG) / 128) % 128; }
int bfunit(int w) { int ul; ul = ((w - BFTAG) / 16384) % 4;
    if (ul == 0) return 1; if (ul == 1) return 2; if (ul == 2) return 4; return 8; }
int bfsig(int w) { return (w - BFTAG) / 65536; }

int eload(int w) {
    if (w >= BFTAG) {
        /* up to the top of the register, then back down: an arithmetic
           shift for a signed field, so the sign question answers itself */
        if (bfunit(w) == 8) es("  @mem.load r0, [r0+0]\n");
        else { es("  @mem.ld r0, [r0+0], "); en(bfunit(w)); ec(10); }
        es("  @lit.imm r1, "); en(64 - bfofs(w) - bfwidth(w));
        es("\n  @alu.shl r0, r0, r1\n  @lit.imm r1, "); en(64 - bfwidth(w));
        if (bfsig(w)) es("\n  @alu.shr r0, r0, r1\n");
        else es("\n  @alu.lshr r0, r0, r1\n");
        return 0;
    }
    if (w == 0) return 0;
    if (w == 8) { es("  @mem.load r0, [r0+0]\n"); return 0; }
    es("  @mem.ld r0, [r0+0], "); en(w); ec(10);
    return 0;
}

/* A u8/u16/u32 value in a register is always zero-extended.  Every narrow
   unsigned RESULT is masked here -- a binary op's, a compound
   assignment's, ++x's, -x's, ~x's -- so widening it to long, returning it
   or passing it needs nothing more.  The OPERANDS of a narrow unsigned op
   were already masked (emit_binop); the results were not, and
   `21 - (v & 1023)`, int minus u32 and so a u32, stayed a negative
   64-bit value when it was widened to long.  The eight-class fuzz
   [S-15 A3] found it, in both front ends at once. */
int zext(int w) {
    if (w >= 8) return 0;
    es("  @lit.imm r2, ");
    if (w == 1) en(255);
    if (w == 2) en(65535);
    if (w == 4) en(4294967295);
    es("\n  @alu.and r0, r0, r2\n");
    return 0;
}

int estore(int w) {                              /* [r1] = r0 */
    if (w >= BFTAG) {
        /* read, clear the field's bits, or the new ones in, write back;
           r0 -- the assigned value -- survives */
        long mask;
        mask = 1;
        mask = (mask << bfwidth(w)) - 1;
        es("  @lit.imm r2, "); en(mask); es("\n  @alu.and r3, r0, r2\n");
        es("  @lit.imm r2, "); en(bfofs(w)); es("\n  @alu.shl r3, r3, r2\n");
        if (bfunit(w) == 8) es("  @mem.load r4, [r1+0]\n");
        else { es("  @mem.ld r4, [r1+0], "); en(bfunit(w)); ec(10); }
        es("  @lit.imm r2, "); en(0 - (mask << bfofs(w)) - 1);
        es("\n  @alu.and r4, r4, r2\n  @alu.or r4, r4, r3\n");
        if (bfunit(w) == 8) es("  @mem.store [r1+0], r4\n");
        else { es("  @mem.st [r1+0], r4, "); en(bfunit(w)); ec(10); }
        return 0;
    }
    if (w == 0) return 0;
    if (w == 8) { es("  @mem.store [r1+0], r0\n"); return 0; }
    es("  @mem.st [r1+0], r0, "); en(w); ec(10);
    return 0;
}

int loadval(void) {
    int w;
    if (lvalue) {
        w = stw();
        eload(w);
        /* `.ld` sign-extends; an unsigned narrow object must not.  A
           uint8_t of 0xa2 printed as ffffffffffffffa2 until this. */
        if (curuns) { if (curptr == 0) {
            if (w == 1) es("  @lit.imm r2, 255\n  @alu.and r0, r0, r2\n");
            if (w == 2) es("  @lit.imm r2, 65535\n  @alu.and r0, r0, r2\n");
            if (w == 4) es("  @lit.imm r2, 4294967295\n  @alu.and r0, r0, r2\n");
        } }
        lvalue = 0;
    }
    return 0;
}

int primary(void);

/* `(T){...}` as an expression (C99 6.5.2.5): an unnamed object, initialised
   in place.  Inside a function it lives in the frame; at file scope it has
   static storage duration, so it is a data object of its own.  The cursor
   is on the `{`. */
int ncl;
int cplitexpr(int w, int sst, int isarr, int n) {
    int size; int off; int id; int save; int el;
    el = declsz;
    if (sst >= 0) el = stsize[sst];
    if (declptr) el = 8;
    if (isarr) {
        if (n == 0) {
            n = initcountat(tp);
            if (sst >= 0) n = (n + structslots(sst) - 1) / structslots(sst);
        }
        size = n * el;
    } else size = el;
    if (sst >= 0) w = el;
    if (infunc) {
        off = alloc_local(size);
        initisarr = isarr;
        initaggr(0, 0, off, w, sst, size);
        es("  @lit.imm r0, "); en(off); es("\n  @alu.sub r0, r6, r0\n");
    } else {
        /* named after the `{` token, not a counter: expr() parses
           speculatively and rewinds, so this can run twice for one literal
           and must name the same object both times */
        id = tp;
        save = toinit; toinit = 0;
        es(".bss __cl"); en(id); ec(32); en(size); ec(10);
        toinit = save;
        initisarr = isarr;
        initaggr(2, id, 0, w, sst, size);
        es("  @mem.lea r0, __cl"); en(id); ec(10);
    }
    if (isarr) { lvalue = 0; curptr = 1; curelem = w; if (sst >= 0) curelem = el; }
    else { if (sst >= 0) { lvalue = 1; curptr = 0; curelem = 0; }
           else { lvalue = 1; curptr = declptr; curelem = w; if (declptr) curelem = 8; } }
    curstruct = sst; cursize = size; curdim2 = 0; curdim3 = 0;
    return postfix();
}

int unary(void) {
    int p;
    curfn = 0; curfnst = 0 - 1;
    p = ask(2);
    if (p == P_NEG) { adv(); unary(); loadval();
        if (curflt) { if (curptr == 0) {        /* -x flips the sign bit, -0.0 too */
            es("  @lit.imm r1, "); en(curflt == 8 ? (long)1 << 63 : 2147483648); es("\n  @alu.xor r0, r0, r1\n");
            return 0; } }
        es("  @lit.imm r1, 0\n  @alu.sub r0, r1, r0\n");
        if (curptr == 0) { if (curuns) { if (cursize == 4) zext(4); } }
        return 0; }
    if (p == P_NOT) { adv(); unary(); loadval();
        if (curflt) { if (curptr == 0) {        /* !x is x == 0.0 */
            es("  @lit.imm r1, 0\n");
            if (curflt == 8) es("  @fpu.deq r0, r0, r1\n"); else es("  @fpu.seq r0, r0, r1\n");
            setkind(0); cursize = 4; curelem = 4;
            return 0; } }
        es("  @lit.imm r1, 0\n  @alu.eq r0, r0, r1\n"); return 0; }
    if (p == P_DEREF) {
        adv(); unary(); loadval();
        if (curfn) { lvalue = 0; return 0; }     /* *fp is fp */
        lvalue = 1;
        if (curpd >= 2) {                        /* *q of int **q is an int * */
            curpd = curpd - 1; curptr = 1;
            curelem = curpd >= 2 ? 8 : curbase;
        } else { curptr = 0; curpd = 0;
                 /* and as wide as the pointee: `*b` of an `int *b` kept the
                    POINTER's eight bytes, so it sat on the type axis as a
                    long -- `%d` with it warned, and `*u * 3` of an
                    `unsigned *u` was never narrowed */
                 if (curelem > 0) { if (curelem <= 8) { if (curstruct < 0) cursize = curelem; } }
                 /* `*p` of a struct pointer is the struct: an aggregate,
                    whose value is its address, so nothing more is loaded.
                    `sum(*p)` used to load its first eight bytes and pass
                    THOSE as the address -- a segfault in the callee */
                 if (curstruct >= 0) curelem = 0; }
        return 0;
    }
    if (p == P_BNOT) {                  /* ~x is x ^ -1 */
        adv(); unary(); loadval();
        es("  @lit.imm r1, -1\n  @alu.xor r0, r0, r1\n");
        if (curptr == 0) { if (curuns) { if (cursize == 4) zext(4); } }
        lvalue = 0; curptr = 0; return 0;
    }
    if (p == P_UPLUS) { adv(); unary(); loadval(); lvalue = 0; return 0; }
    if (p == P_PREINC) {                /* ++x is x += 1, and its value is the new x */
        int op; int e;
        op = cur(); adv();
        unary();
        if (lvalue == 0) err_tok(tp, "++ and -- need an lvalue");
        e = stw();
        lvalue = 0;
        push();                                  /* address */
        eload(e);
        if (curptr == 0) { if (e < BFTAG) { if (curuns) zext(e); } }
        if (curflt) { if (curptr == 0) {         /* a float steps by 1.0 */
            int fk; fk = curflt;
            es("  @lit.imm r1, "); en(fone(fk)); ec(10);
            if (op == tidx("++", 2)) es(fk == 8 ? "  @fpu.dadd r0, r0, r1\n" : "  @fpu.sadd r0, r0, r1\n");
            else es(fk == 8 ? "  @fpu.dsub r0, r0, r1\n" : "  @fpu.ssub r0, r0, r1\n");
            pop1();
            estore(e);
            setkind(fk);
            return 0;
        } }
        es("  @lit.imm r1, ");
        if (curptr) en(curelem); else en(1);
        ec(10);
        if (op == tidx("++", 2)) es("  @alu.add r0, r0, r1\n");
        else es("  @alu.sub r0, r0, r1\n");
        if (curptr == 0) { if (e < BFTAG) { if (curuns) zext(e); } }
        pop1();
        estore(e);
        return 0;
    }
    if (p == P_SIZEOF) {
        int sz; int nsave;
        adv();
        if (cur() == tidx("(", 1)) {
            /* the TABLE decides whether `sizeof (` opens a type name */
            if (actis(scopeact("sizeof", 6, tp + 1), "type_name", 9)) {
                adv();
                declspec();                          /* handles struct too */
                sz = declsz;
                while (eatstar()) sz = 8;
                if (cur() == tidx("[", 1)) {
                    adv(); sz = sz * cexpr(); need(tidx("]", 1), "]");
                }
                need(tidx(")", 1), ")");
                es("  @lit.imm r0, "); en(sz); ec(10);
                lvalue = 0; curelem = 8; curptr = 0;
                return 0;
            }
        }
        /* sizeof EXPR: walk it for its type and throw the code away -- the
           operand of sizeof is not evaluated [C99 6.5.3.4p2]. */
        nsave = nout;
        cursize = 8;                       /* an expression with no symbol */
        curvla = 0;
        unary();
        nout = nsave;
        sz = cursize;
        if (curvla) { es("  @mem.load r0, [r6-"); en(curvla); es("]\n"); curvla = 0; }
        else { es("  @lit.imm r0, "); en(sz); ec(10); }
        lvalue = 0; curelem = 8; curptr = 0;
        return 0;
    }
    if (p == P_ADDR) { adv(); unary();
        /* &x: a pointer one deeper than x */
        if (lvalue) {
            if (curptr) { curpd = curpd + 1; curelem = 8; }
            else { curbase = curelem; if (curstruct >= 0) { curbase = stsize[curstruct]; }
                   curelem = curbase; curpd = 1; if (curflt) curelem = curflt; }
            curptr = 1;
        }
        lvalue = 0; return 0; }
    /* a cast: `(TYPE) unary`.  The table calls this `prim`, because `(` is
       all it can see -- whether a type name follows is the walker's job. */
    if (cur() == tidx("(", 1)) {
        if (is_typeat(tp + 1)) {
            int cw; int csz; int cuns; int cst; int carr; int cn; int cflt; int ok; int cpd;
            adv();
            cw = declspec(); csz = declsz; cuns = declunsigned; cst = declstruct; cflt = declflt;
            declptr = declspecptr; declpd = declspecpd;
            while (eatstar()) { declptr = 1; csz = 8; }
            cpd = declpd;
            if (cur() == tidx("(", 1)) { if (kind(tp + 1) == tidx("*", 1)) {
                adv(); while (eatstar()) { }
                need(tidx(")", 1), ")");
                if (cur() == tidx("(", 1)) skipparen();
                declptr = 1; csz = 8;
            } }
            carr = 0; cn = 0;
            if (cur() == tidx("[", 1)) {
                adv(); carr = 1;
                if (cur() != tidx("]", 1)) cn = cexpr();
                need(tidx("]", 1), "]");
            }
            need(tidx(")", 1), ")");
            if (cur() == tidx("{", 1)) return cplitexpr(cw, cst, carr, cn);
            unary(); loadval();
            /* C99 6.3.1.4-5 at a cast: to or from a floating type */
            ok = fkind();
            if (declptr == 0) {
                if (cflt) fconv(ok, cflt);
                else { if (ok >= 4) fconv(ok, (cuns && csz == 8) ? 1 : 0); }
            }
            /* narrowing is observable: `(char)300` is 44.  The tape has
               sized load/store, so a round trip through a stack slot is the
               whole of it -- and it sign-extends on the way back. */
            if (declptr == 0) { if (cflt == 0) {
                if (csz < 8) {
                    if (cuns) {
                        /* unsigned: keep the low bits, zero the rest --
                           `(unsigned char)255 >> 4` is 15, not -1 */
                        if (csz == 1) es("  @lit.imm r2, 255\n  @alu.and r0, r0, r2\n");
                        if (csz == 2) es("  @lit.imm r2, 65535\n  @alu.and r0, r0, r2\n");
                        if (csz == 4) es("  @lit.imm r2, 4294967295\n  @alu.and r0, r0, r2\n");
                    } else {
                        es("  @call.frame 8\n  @mem.st [r7+0], r0, "); en(csz);
                        es("\n  @mem.ld r0, [r7+0], "); en(csz);
                        es("\n  @call.frame -8\n");
                    }
                }
            } }
            lvalue = 0; curptr = declptr; cursize = csz; curuns = cuns; curflt = cflt;
            curelem = cw;
            curpd = 0; curbase = cw;
            if (declptr) { curpd = cpd > 0 ? cpd : 1; if (curpd >= 2) curelem = 8; }
            if (declptr) curelem = cw;
            /* `(struct S *)p` -- the member access after it needs the type */
            curstruct = 0 - 1;
            if (declptr) { if (cst >= 0) { curstruct = cst; curelem = stsize[cst]; } }
            return 0;
        }
    }
    return primary();
}

int postfix(void) {
    int p; int e;
    while (1) {
        p = ask(3);
        if (p == P_CALL) {
            /* a call through whatever the expression produced: `pick()(3, 4)`,
               `s->f(5, 6)`.  The parse table said `call`; do it. */
            vcst = curfnst;
            loadval();
            return vcall(0);
        }
        if (p == P_INC) {
            int op; int e2; int step; int ce;
            op = cur(); adv();
            e2 = stw();
            /* a pointer steps by its ELEMENT (C99 6.5.6p8) -- it stepped by
               one byte, and only char pointers had ever been tested */
            ce = curelem;
            step = 1;
            if (curptr) step = curelem;
            lvalue = 0;
            push();                                  /* address */
            eload(e2);
            if (curflt) { if (curptr == 0) {
                /* (x + 1) - 1 is not x in floating point: keep the old value
                   itself.  Stack: address, old value. */
                int fk; fk = curflt;
                push();
                es("  @lit.imm r1, "); en(fone(fk)); ec(10);
                if (op == tidx("++", 2)) es(fk == 8 ? "  @fpu.dadd r0, r0, r1\n" : "  @fpu.sadd r0, r0, r1\n");
                else es(fk == 8 ? "  @fpu.dsub r0, r0, r1\n" : "  @fpu.ssub r0, r0, r1\n");
                es("  @mem.load r1, [r7+8]\n");
                estore(e2);
                es("  @mem.load r0, [r7+0]\n  @call.frame -16\n");
                setkind(fk);
                continue;
            } }
            push();                                  /* old value */
            es("  @lit.imm r0, "); en(step); ec(10);
            if (op == tidx("++", 2)) emit_binop(tidx("+", 1));
            else emit_binop(tidx("-", 1));
            pop1();                                  /* address */
            estore(e2);
            es("  @lit.imm r2, "); en(step); ec(10);
            if (op == tidx("++", 2)) es("  @alu.sub r0, r0, r2\n");
            else es("  @alu.add r0, r0, r2\n");
            /* the value is the old pointer, still pointing at elements */
            curelem = e2;
            if (curptr) curelem = ce;
        } else {
        if (p == P_FIELD) {
            int isarrow; int mi; int mt;
            isarrow = 0;
            if (cur() == tidx("->", 2)) isarrow = 1;
            adv();
            if (isarrow) loadval();          /* the pointer's VALUE is the base */
            if (curstruct < 0) {
                err_tok(tp, "member access on something that is not a struct");
                __exit(1);
            }
            scopedecl = 1; scopewant("field", 5, tp, "field", 5); scopedecl = 0;
            mt = adv();
            mi = mbfind(curstruct, mt);
            if (mi < 0) {
                err_tok(mt, "no such member");
                __exit(1);
            }
            if (mboff[mi]) {
                es("  @lit.imm r2, "); en(mboff[mi]);
                es("\n  @alu.add r0, r0, r2\n");
            }
            lvalue = 1;
            curelem = mbwidth[mi];
            curptr = mbptr[mi];
            if (curptr) curelem = mbelem[mi];
            curpd = 0; curbase = mbelem[mi];
            if (curptr) { curpd = mbptrd[mi]; if (curpd >= 2) curelem = 8; }
            cursize = mbbytes[mi];
            curstruct = mbstruct[mi];
            /* `p->q->b`: a pointer member hands its pointee on */
            if (mbptr[mi]) { curstruct = mbpst[mi]; if (curstruct >= 0) curelem = stsize[curstruct]; }
            curuns = mbuns[mi]; curbool = mbbool[mi];
            curflt = mbflt[mi];
            if (mbwidth[mi] == 0) { if (mbptr[mi] == 0) {
                /* an array or a nested struct: the value IS the address */
                if (mbstruct[mi] < 0) { lvalue = 0; curptr = 1;
                                        curelem = mbelem[mi]; }
            } }
        } else {
        if (p == P_INDEX) {
            int row; int uu; int ist; int ifl; int ipd; int ibase;
            curvla = 0;
            ipd = curpd; ibase = curbase;
            adv();
            ist = curstruct;                 /* the index expression resets it */
            ifl = curflt;
            e = curelem;
            row = curdim2;
            uu = curuns;                     /* the ELEMENT's, not the index's */
            if (row > 0) e = e * row;        /* the first index of `a[n][m]` */
            loadval();
            push();
            expr(); loadval();
            if (e != 1) { es("  @lit.imm r2, "); en(e); es("\n  @alu.mul r0, r0, r2\n"); }
            pop1();
            es("  @alu.add r0, r1, r0\n");
            need(vfind(TOKV, NTOKV, "]", 1), "]");
            cursize = e;
            curuns = uu;
            if (row > 0) {
                /* a row is itself an array: its VALUE is its address */
                curelem = e / row; curptr = 1; lvalue = 0;
                /* a[i] of a[n][m][k] is itself an [m][k]: its rows are k long */
                curdim2 = curdim3; curdim3 = 0;
            } else { lvalue = 1; curelem = e; curptr = 0; }
            /* an element of a struct array is itself an aggregate: its value
               is its address, and assigning it copies the whole struct */
            curstruct = ist;
            curflt = ifl;
            if (ist >= 0) { if (row == 0) curelem = 0; }
            /* an element of an array of pointers, or q[i] of int **q, is a
               pointer itself */
            curpd = 0;
            if (row == 0) { if (ipd >= 2) {
                curptr = 1; curpd = ipd - 1; curbase = ibase;
                curelem = curpd >= 2 ? 8 : ibase;
            } }
        } else {
            return 0;
        } } }
    }
    return 0;
}

int pf_call(int t);

/* A call through a variable that holds a function's address.  The callee
   goes on the stack UNDER the arguments; the convention is the pointee's --
   stacked if it is variadic (declfp 2) or the call has more than six. */
int icparen;
int vcall(int var);
int fpdecl(void);
int initcountat(int j);
int initaggr(int isglobal, int gt, int off, int w, int sst, int nbytes);
int structslots(int sst);
int cplitexpr(int w, int sst, int isarr, int n);
int icall(int si, int t) {
    int n; int k; int st;
    if (symkind[si] == 0) { symlea(si, t, "r0"); es("  @mem.load r0, [r0+0]\n"); }
    else { es("  @mem.load r0, [r6-"); en(symoff[si]); es("]\n"); }
    adv();
    if (icparen) { icparen = 0; need(tidx(")", 1), ")"); }
    vcst = symcst[si];
    vcfn = symfpret[si];
    return vcall(symfp[si] == 2);
}

/* The callee's address is in r0 and the cursor is on the `(`.  `var` says
   the pointee is variadic, which puts every argument on the tape stack. */
int vcall(int var) {
    int n; int k; int st;
    push();
    need(tidx("(", 1), "(");
    n = 0;
    while (cur() != tidx(")", 1)) {
        if (cur() == T_EOF) break;
        expr(); loadval(); argconv(0 - 1, n); push(); n = n + 1;
        if (eat(tidx(",", 1)) == 0) break;
    }
    need(tidx(")", 1), ")");
    st = 0;
    if (var) st = 1;
    if (n > 6) st = 1;
    if (st) {
        k = 0;
        while (k < n / 2) {
            es("  @mem.load r2, [r7+"); en(8 * k); es("]\n");
            es("  @mem.load r1, [r7+"); en(8 * (n - 1 - k)); es("]\n");
            es("  @mem.store [r7+"); en(8 * k); es("], r1\n");
            es("  @mem.store [r7+"); en(8 * (n - 1 - k)); es("], r2\n");
            k = k + 1;
        }
        es("  @mem.load r5, [r7+"); en(8 * n); es("]\n  @call.callr r5\n");
        es("  @call.frame -"); en(8 * (n + 1)); ec(10);
    } else {
        if (n > 5) { printf("indirect call: six register arguments leave no register for the callee\n"); __exit(1); }
        k = n - 1;
        while (k >= 0) { es("  @mem.load r"); en(k); es(", [r7+0]\n  @call.frame -8\n"); k = k - 1; }
        es("  @mem.load r5, [r7+0]\n  @call.frame -8\n  @call.callr r5\n");
    }
    lvalue = 0; curelem = 8; curptr = 0; curuns = 0; cursize = 8; curstruct = 0 - 1;
    curfn = 0; curfnst = 0 - 1; curflt = 0;
    /* the pointee returns a struct pointer: `go()()->zerofunc` */
    if (vcst >= 0) { curstruct = vcst; curptr = 1; curelem = stsize[vcst]; }
    vcst = 0 - 1;
    if (vcfn) { curfn = 1; vcfn = 0; }
    return postfix();
}

int primary(void) {
    int t; int i; long v; int k;
    t = cur();
    curuns = 0; curflt = 0;
    if (t == T_NUM) { if (fltlit(tp)) {
        /* a floating constant: its bits, rounded once from the decimal */
        int fk; fk = fltlit(tp);
        es("  @lit.imm r0, ");
        if (tlen[tp] > 1 && (src[tpos[tp]] & 255) == 48
            && ((src[tpos[tp] + 1] & 255) == 120 || (src[tpos[tp] + 1] & 255) == 88))
            en(fhex2bin(tpos[tp], tlen[tp], fk == 4));
        else en(fdec2bin(tpos[tp], tlen[tp], fk == 4));
        ec(10);
        adv();
        setkind(fk);
        return postfix();
    } }
    if (t == T_NUM) {
        v = numval(tp);
        /* C99 6.4.4.1: a constant is an int if it fits, else a long; an
           l/L suffix makes it a long outright.  `sizeof 1L` is 8. */
        cursize = 4;
        if (v > 2147483647) cursize = 8;
        if (v < 0 - 2147483647) cursize = 8;
        k = 0;
        while (k < tlen[tp]) {
            if ((src[tpos[tp] + k] & 255) == 108) cursize = 8;
            if ((src[tpos[tp] + k] & 255) == 76) cursize = 8;
            k = k + 1;
        }
        if ((src[tpos[tp]] & 255) == 39) cursize = 4;      /* 'x' is an int */
        /* C99 6.4.4.1: a HEX or OCTAL constant takes the first of int,
           unsigned int, long, unsigned long that holds it -- so 0xffffffff
           is an unsigned int, and `s == 0xffffffff` compares in 32 bits. */
        if ((src[tpos[tp]] & 255) == 48) { if (tlen[tp] > 1) {
            if (v > 2147483647) { if (v <= 4294967295) { cursize = 4; curuns = 1; } }
        } }
        k = 0;
        while (k < tlen[tp]) {
            if ((src[tpos[tp] + k] & 255) == 117) curuns = 1;   /* u */
            if ((src[tpos[tp] + k] & 255) == 85) curuns = 1;    /* U */
            k = k + 1;
        }
        /* C99 6.4.4.1 again: a decimal constant with a u suffix is the
           first of unsigned int, unsigned long that holds it -- so
           3655922532u is 32 bits wide, and `3655922532u * p` wraps there.
           The int-or-long rule above had made it a long, and the widened
           fuzz [S-15 A3] found the product never being narrowed. */
        if (curuns) { if (v >= 0) { if (v <= 4294967295) {
            int hasl; hasl = 0; k = 0;
            while (k < tlen[tp]) {
                if ((src[tpos[tp] + k] & 255) == 108) hasl = 1;
                if ((src[tpos[tp] + k] & 255) == 76) hasl = 1;
                k = k + 1;
            }
            if (hasl == 0) cursize = 4;
        } } }
        adv();
        es("  @lit.imm r0, "); en(v); ec(10);
        lvalue = 0; curelem = cursize; curptr = 0;
        return postfix();
    }
    if (t == T_STR) { if (iswide(tp)) {
        int wn; int wk;
        wn = wdecode(adv(), wcp);
        wk = 0;
        while (wk <= wn) {                          /* with its terminator */
            int wv; wv = 0;
            if (wk < wn) wv = wcp[wk];
            lbuf[wk * 4] = wv & 255; lbuf[wk * 4 + 1] = (wv >> 8) & 255;
            lbuf[wk * 4 + 2] = (wv >> 16) & 255; lbuf[wk * 4 + 3] = (wv >> 24) & 255;
            wk = wk + 1;
        }
        i = addlit(lbuf, (wn + 1) * 4);
        es("  @mem.lea r0, S"); en(i); ec(10);
        lvalue = 0; curelem = 4; curptr = 1;
        return postfix();
    } }
    /* C99 6.4.2.2: inside a function, `__func__` is declared as if by
       `static const char __func__[] = "name";`.  It is not an identifier
       to look up -- there is no such symbol -- so it is answered here,
       with the name of the function being walked. */
    if (t == T_ID) { if (srcis(tpos[tp], tlen[tp], "__func__")) {
        int fl;
        adv();
        if (fntok < 0) { i = addlit("", 0); }
        else {
            fl = 0;
            while (fl < tlen[fntok] && fl < 120) {
                lbuf[fl] = src[tpos[fntok] + fl]; fl = fl + 1;
            }
            lbuf[fl] = 0;
            i = addlit(lbuf, fl + 1);
        }
        es("  @mem.lea r0, S"); en(i); ec(10);
        lvalue = 0; curelem = 1; curptr = 1;
        return postfix();
    } }
    if (t == T_STR) {
        i = nlab; nlab = nlab + 1;
        i = addlit(lbuf, decode(adv(), lbuf));
        es("  @mem.lea r0, S"); en(i); ec(10);
        lvalue = 0; curelem = 1; curptr = 1;
        return postfix();
    }
    if (t == vfind(TOKV, NTOKV, "(", 1)) {
        /* `(*fp)(args)`: dereferencing a function pointer gives back the
           designator (C99 6.5.3.2p4), so it is the call `fp(args)` */
        if (kind(tp + 1) == tidx("*", 1)) { if (kind(tp + 2) == T_ID) {
            if (kind(tp + 3) == tidx(")", 1)) { if (kind(tp + 4) == tidx("(", 1)) {
                i = sfind(tp + 2);
                if (i >= 0) symused[i] = 1;
                if (i >= 0) { if (symfp[i]) {
                    adv(); adv(); icparen = 1;
                    return icall(i, tp);
                } }
            } }
        } }
        /* `( expr )` is a parenthesised expression, and inside the
           parentheses the COMMA OPERATOR is in scope: `(a++, b++, c)`
           evaluates all three and has c's value.  This used to call
           expr(), which stops at the comma, so the `)` never arrived. */
        adv(); exprc(); need(vfind(TOKV, NTOKV, ")", 1), ")");
        return postfix();
    }
    if (t == T_ID) {
        scopewant("expr", 4, tp, "lookup", 6);
        { int ui; ui = sfind(tp); if (ui >= 0) { if (kind(tp + 1) == tidx("(", 1)) symused[ui] = 1; } }
        if (kind(tp + 1) == vfind(TOKV, NTOKV, "(", 1)) {
            i = sfind(tp);
            /* a function is never a pointer variable, even when it RETURNS
               one (`binop pick(void)` carries binop's fp flag) */
            if (i >= 0) { if (symfp[i]) { if (symkind[i] != 2) return icall(i, tp); } }
            return pf_call(adv());
        }
        i = mfindt(tp);
        if (i >= 0) { if (machas[i]) {
            es("  @lit.imm r0, "); en(macval[i]); ec(10);
            adv(); lvalue = 0; curelem = 8; curptr = 0;
            return postfix();
        } }
        i = sfind(tp);
        if (i < 0) err_tok(tp, "unknown identifier");
        if (i >= 0) {
            /* written, not read: `x = ...` as a statement of its own.  Any
               other position -- `*x = `, `a = x = 0`, `f(x = 1)` -- reads
               x or uses the assignment's value, as clang counts it. */
            if (kind(tp + 1) != tidx("=", 1)) symused[i] = 1;
            else { if (tp > 0) {
                int pk; pk = kind(tp - 1);
                if (pk != tidx(";", 1) && pk != tidx("{", 1) && pk != tidx("}", 1) && pk != tidx(")", 1)) symused[i] = 1;
            } }
        }
        cursize = symbytes[i];           /* what `sizeof` reports for it */
        curvla = symvla[i];
        curflt = symflt[i];
        curstruct = symstruct[i];
        curdim2 = symdim2[i];
        curdim3 = symdim3[i];
        curuns = symuns[i]; curbool = symbool[i];
        if (symkind[i] == 2) {           /* a function designator */
            es("  @mem.lea r0, "); etok(tp); ec(10);
            adv(); lvalue = 0; curelem = 8; curptr = 0; cursize = 8;
            curfn = 1; curfnst = 0 - 1; curflt = 0;
            return postfix();
        }
        if (symkind[i] == 4) {           /* enum constant: an int (6.7.2.2p3) */
            es("  @lit.imm r0, "); en(symoff[i]); ec(10);
            adv(); lvalue = 0; curelem = 8;
            curptr = 0; cursize = 4; curuns = 0; curflt = 0; curstruct = 0 - 1; curfn = 0;
            return postfix();
        }
        if (symkind[i] == 5) {               /* global array -> its address */
            symlea(i, tp, "r0");
            curelem = symelem[i]; curptr = 1;
            curpd = symptrd[i]; curbase = symbase[i]; if (curpd >= 2) curelem = 8;
            adv(); lvalue = 0;
            return postfix();
        }
        if (symkind[i] == 0) symlea(i, tp, "r0");
        else { es("  @lit.imm r2, "); en(symoff[i]); es("\n  @alu.sub r0, r6, r2\n"); }
        curelem = symelem[i];
        curptr = symptr[i];
        /* a pointer to a struct steps by the STRUCT: its element width was
           the specifier's 8, so `r++` on a 16-byte struct went half-way */
        if (curptr) { if (symstruct[i] >= 0) { if (symkind[i] != 3) curelem = stsize[symstruct[i]]; } }
        curpd = 0; curbase = symbase[i];
        if (curptr) { curpd = symptrd[i]; if (curpd >= 2) curelem = 8; }
        adv();
        lvalue = 1;
        if (symkind[i] == 3) { lvalue = 0; curptr = 1; }   /* array -> address */
        if (symstruct[i] >= 0) { if (symptr[i] == 0) {
            /* a struct object: assignable, and its value is its address */
            lvalue = 1; curptr = 0; curelem = 0;
        } }
        return postfix();
    }
    err_tok(tp, "this is not the start of an expression");
    return 0;
}


/* ---- string literal pool --------------------------------------------- */
#define MAXPOOL 1048576
char pool[MAXPOOL];
#define MAXLIT 4096
int plpos[MAXLIT];
int pllen[MAXLIT];
int npool;
int poolend;

int addlit(char *b, int n) {
    int k;
    if (poolend + n >= MAXPOOL) { __write(2, "literal pool full\n", 18); __exit(1); }
    if (npool >= MAXLIT) { __write(2, "too many literals\n", 18); __exit(1); }
    plpos[npool] = poolend; pllen[npool] = n;
    k = 0;
    while (k < n) { pool[poolend + k] = b[k]; k = k + 1; }
    poolend = poolend + n;
    npool = npool + 1;
    return npool - 1;
}

int hexd(int v) { if (v < 10) return 48 + v; return 87 + v; }

int emit_pool(void) {
    int i; int k; int c;
    i = 0;
    while (i < npool) {
        es(".str S"); en(i); es(" \"");
        k = 0;
        while (k < pllen[i]) {
            c = pool[plpos[i] + k] & 255;
            if (c == 34) { ec(92); ec(34); }
            else { if (c == 92) { ec(92); ec(92); }
            else { if (c >= 32) { if (c < 127) ec(c); else {
                       ec(92); ec(120); ec(hexd(c / 16)); ec(hexd(c % 16)); } }
                   else { ec(92); ec(120); ec(hexd(c / 16)); ec(hexd(c % 16)); } } }
            k = k + 1;
        }
        es("\\x00\"\n");          /* C strings are NUL terminated */
        i = i + 1;
    }
    return 0;
}

/* decode a C string literal token into buf, return its length */
int decode(int t, char *buf) {
    int k; int n; int c;
    k = 1; n = 0;
    while (k < tlen[t] - 1) {
        c = src[tpos[t] + k] & 255;
        if (c == 34) {                 /* the seam between two literals */
            k = k + 1;
            while (k < tlen[t] - 1) {
                c = src[tpos[t] + k] & 255;
                if (c == 34) { k = k + 1; break; }
                k = k + 1;
            }
            continue;
        }
        if (c == 92) {
            k = k + 1;
            c = src[tpos[t] + k] & 255;
            if (c == 110) c = 10;
            else { if (c == 116) c = 9; else {
                   if (c >= 48) { if (c <= 55) {    /* \N, \NN, \NNN octal */
                       int ov; int od;
                       ov = 0; od = 0;
                       while (od < 3) {
                           c = src[tpos[t] + k] & 255;
                           if (c < 48) break;
                           if (c > 55) break;
                           ov = ov * 8 + (c - 48);
                           k = k + 1; od = od + 1;
                       }
                       k = k - 1;
                       c = ov & 255;
                   } }
                   if (c == 114) c = 13; else {
                   if (c == 120) {            /* \xNN */
                       int h1; int h2;
                       h1 = src[tpos[t] + k + 1] & 255;
                       h2 = src[tpos[t] + k + 2] & 255;
                       if (h1 >= 97) h1 = h1 - 87; else { if (h1 >= 65) h1 = h1 - 55; else h1 = h1 - 48; }
                       if (h2 >= 97) h2 = h2 - 87; else { if (h2 >= 65) h2 = h2 - 55; else h2 = h2 - 48; }
                       c = h1 * 16 + h2;
                       k = k + 2;
                   } } } }
        }
        buf[n] = c; n = n + 1;
        k = k + 1;
    }
    return n;
}


/* -Wformat [S-15 C4]: one conversion against the argument just walked.
   printf's format is read at compile time either way, so the type is
   known; the wording is clang's. */
int pf_check(int c, int lng, int at) {
    if (curcall) return 0;
    if (c == 115) { if (curptr == 0) { if (curfn == 0)
        warn_at(tpos[at], "format specifies type 'char *' but the argument has an integer type [-Wformat]"); } }
    if (c == 112) { if (curptr == 0) { if (curfn == 0)
        warn_at(tpos[at], "format specifies type 'void *' but the argument has an integer type [-Wformat]"); } }
    if (c == 100 || c == 105 || c == 117 || c == 120 || c == 88 || c == 111 || c == 99) {
        if (curptr) warn_at(tpos[at], "format specifies an integer type but the argument is a pointer [-Wformat]");
        else { if (curflt) warn_at(tpos[at], "format specifies an integer type but the argument has a floating type [-Wformat]");
        else { if (lng == 0) { if (cursize == 8) { if (curstruct < 0)
            warn_at(tpos[at], "format specifies type 'int' but the argument has type 'long' [-Wformat]"); } }
        else { if (cursize < 8)
            warn_at(tpos[at], "format specifies type 'long' but the argument has type 'int' [-Wformat]"); } } }
    }
    if (c == 102 || c == 101 || c == 103 || c == 70 || c == 69 || c == 71) { if (curflt == 0)
        warn_at(tpos[at], "format specifies type 'double' but the argument has an integer type [-Wformat]"); }
    return 0;
}

/* The arguments walked once for their TYPES and the emitter rewound, the
   way expr() rewinds a false start: the real walk -- do_printf's or the
   ordinary call's, when the format needs the runtime printf -- follows
   as if nothing happened.  An error found here parks the cursor like any
   other and is not walked into twice. */
int pf_dryrun(int ft) {
    char fb[4096]; int n; int k; int c; int lng; int at;
    int save; int nsave; int isave; int psave; int pesave;
    if (warnall == 0) return 0;
    save = tp; nsave = nout; isave = nibuf; psave = npool; pesave = poolend;
    n = decode(ft, fb);
    tp = ft + 1;
    k = 0;
    while (k < n) {
        c = fb[k] & 255;
        if (c != 37) { k = k + 1; continue; }
        k = k + 1;
        while (k < n) { c = fb[k] & 255; if (c == 45 || c == 43 || c == 32 || c == 35 || c == 48) { k = k + 1; continue; } break; }
        if (k < n) { if (fb[k] == 42) { k = k + 1; if (eat(tidx(",", 1))) { expr(); loadval(); } } }
        while (k < n) { if (isdi(fb[k] & 255) == 0) break; k = k + 1; }
        if (k < n) { if (fb[k] == 46) { k = k + 1;
            if (k < n) { if (fb[k] == 42) { k = k + 1; if (eat(tidx(",", 1))) { expr(); loadval(); } } }
            while (k < n) { if (isdi(fb[k] & 255) == 0) break; k = k + 1; } } }
        lng = 0;
        while (k < n) {
            c = fb[k] & 255;
            if (c == 104 || c == 76 || c == 122 || c == 106 || c == 116) { k = k + 1; continue; }
            if (c == 108) { lng = 1; k = k + 1; continue; }
            break;
        }
        if (k >= n) break;
        c = fb[k] & 255; k = k + 1;
        if (c == 37) continue;
        if (eat(tidx(",", 1)) == 0) break;          /* too few arguments: not this check's */
        at = tp; curcall = 0; expr(); loadval();
        pf_check(c, lng, at);
        if (panic) break;
    }
    if (panic == 0) { tp = save; nout = nsave; nibuf = isave; npool = psave; poolend = pesave; }
    return 0;
}

int do_printf(void) {
    int lng;
    int t; int k; int n; int c; int m; int id; int pass; int j; int slot[32];
    char fbuf[4096];
    need(vfind(TOKV, NTOKV, "(", 1), "(");
    if (cur() != T_STR) { printf("printf needs a literal format\n"); __exit(1); }
    t = adv();
    n = decode(t, fbuf);
    /* Two passes over the format, as the Python walker does: pass 0
       evaluates every argument into a frame slot of its own, pass 1 writes.
       Interleaving was observable -- `printf("a %d\n", f())` wrote `a `
       before calling f, and f's own output came out after it. */
    pass = 0;
    while (pass < 2) {
    k = 0; m = 0; j = 0;
    while (k < n) {
        c = fbuf[k] & 255;
        if (c == 37) {
            k = k + 1;
            /* flags, width, precision and the length modifiers.  None of
               them change WHICH conversion this is, and the length modifier
               least of all -- `%ld` prints the same 64-bit value `%d` does,
               and refusing it only refused the program. */
            while (k < n) {
                c = fbuf[k] & 255;
                if (c == 45) { k = k + 1; continue; }        /* - */
                if (c == 43) { k = k + 1; continue; }        /* + */
                if (c == 32) { k = k + 1; continue; }
                if (c == 35) { k = k + 1; continue; }        /* # */
                if (c == 48) { k = k + 1; continue; }        /* 0 */
                break;
            }
            while (k < n) { if (isdi(fbuf[k] & 255) == 0) break; k = k + 1; }
            if (k < n) { if ((fbuf[k] & 255) == 46) {
                k = k + 1;
                while (k < n) { if (isdi(fbuf[k] & 255) == 0) break; k = k + 1; }
            } }
            lng = 0;
            while (k < n) {
                c = fbuf[k] & 255;
                if (c == 104) { k = k + 1; continue; }       /* h */
                if (c == 108) { lng = 1; k = k + 1; continue; }       /* l */
                if (c == 76) { k = k + 1; continue; }        /* L */
                if (c == 122) { k = k + 1; continue; }       /* z */
                if (c == 106) { k = k + 1; continue; }       /* j */
                if (c == 116) { k = k + 1; continue; }       /* t */
                break;
            }
            c = fbuf[k] & 255;
            if (c == 37) { lbuf[m] = 37; m = m + 1; k = k + 1; }
            else if (pass == 0) {
                need(vfind(TOKV, NTOKV, ",", 1), ",");
                expr(); loadval();
                if (j >= 32) { printf("printf: more than 32 arguments\n"); __exit(1); }
                slot[j] = alloc_local(8);
                es("  @mem.store [r6-"); en(slot[j]); es("], r0\n");
                j = j + 1; m = 0; k = k + 1;
            } else {
                if (m > 0) {
                    id = addlit(lbuf, m);
                    es("  @mem.lea r0, S"); en(id); es("\n  @lit.imm r1, "); en(m);
                    es("\n  @lit.write r0, r1\n");
                    m = 0;
                }
                es("  @mem.load r0, [r6-"); en(slot[j]); es("]\n");
                j = j + 1;
                /* which routine prints this conversion: the pfconv stage */
                {   char cc[2]; int key[4]; int h; char *hn;
                    cc[0] = c; cc[1] = 0;
                    key[0] = vfind(BF_PFCONV_0, NBF_PFCONV_0, cc, 1);
                    if (key[0] < 0) { printf("printf: unsupported conversion\n"); __exit(1); }
                    key[1] = 0; key[2] = 0; key[3] = 0;
                    h = inf(S_PFCONV, key, 0);
                    hn = BH_PFCONV_Y + voff(BH_PFCONV_Y, h);
                    c = 0;
                    if (hn[0] == 105) c = 100;                       /* int */
                    if (hn[0] == 117) c = 117;                       /* u32 */
                    if (hn[0] == 104) c = 120;                       /* hex */
                    if (hn[0] == 72) c = 88;                         /* HEX */
                    if (hn[0] == 111) c = 111;                       /* oct */
                    if (hn[0] == 99) c = 99;                         /* chr */
                    if (hn[0] == 115) c = 115;                       /* str */
                }
                /* c now names the ROUTINE the net chose, not the letter */
                if (c == 100) es("  @lit.print r0\n");
                else { if (c == 117) {           /* the low 32 bits */
                    es("  @lit.imm r2, 4294967295\n  @alu.and r0, r0, r2\n  @lit.print r0\n");
                } else { if (c == 120) { needxb = 1;         /* hex */
                    es("  @lit.imm r1, 16\n  @lit.imm r2, 97\n  @call.call __itoab\n"
                       "  @lit.write r0, r1\n");
                } else { if (c == 88) { needxb = 1;          /* HEX */
                    es("  @lit.imm r1, 16\n  @lit.imm r2, 65\n  @call.call __itoab\n"
                       "  @lit.write r0, r1\n");
                } else { if (c == 111) { needxb = 1;         /* oct */
                    es("  @lit.imm r1, 8\n  @lit.imm r2, 97\n  @call.call __itoab\n"
                       "  @lit.write r0, r1\n");
                } else { if (c == 99) {          /* chr */
                    es("  mov r2, r0\n  @mem.lea r0, __chb\n  @mem.st [r0+0], r2, 1\n"
                       "  @lit.imm r1, 1\n  @lit.write r0, r1\n");
                    needchb = 1;
                } else { if (c == 115) {         /* %s */
                    es("  @call.frame 8\n  @mem.store [r7+0], r0\n  @call.call __slen\n"
                       "  mov r1, r0\n  @mem.load r0, [r7+0]\n  @call.frame -8\n"
                       "  @lit.write r0, r1\n");
                    needslen = 1;
                } else { printf("printf: unsupported conversion\n"); __exit(1); }
                } } } } } }
                k = k + 1;
            }
        } else { lbuf[m] = c; m = m + 1; k = k + 1; }
    }
    if (pass == 0) need(vfind(TOKV, NTOKV, ")", 1), ")");
    pass = pass + 1;
    }
    if (m > 0) {
        id = addlit(lbuf, m);
        es("  @mem.lea r0, S"); en(id); es("\n  @lit.imm r1, "); en(m);
        es("\n  @lit.write r0, r1\n");
    }
    es("  @lit.imm r0, 0\n");
    lvalue = 0; curelem = 8;
    return 0;
}

/* ---- calls ----------------------------------------------------------- */
int isname(int t, char *nm, int L) {
    int k;
    if (tlen[t] != L) return 0;
    k = 0;
    while (k < L) { if ((src[tpos[t] + k] & 255) != (nm[k] & 255)) return 0; k = k + 1; }
    return 1;
}

int sysargs(int n) {                 /* pop n args into r0..r2, zero the rest */
    int k;
    k = n - 1;
    while (k >= 0) { es("  @mem.load r"); en(k); es(", [r7+0]\n  @call.frame -8\n"); k = k - 1; }
    k = n;
    while (k < 3) { es("  @lit.imm r"); en(k); es(", 0\n"); k = k + 1; }
    return 0;
}

int sysargs6(int n) {                /* the same, into r0..r5: `mmap` [.sys6] */
    int k;
    k = n - 1;
    while (k >= 0) { es("  @mem.load r"); en(k); es(", [r7+0]\n  @call.frame -8\n"); k = k - 1; }
    k = n;
    while (k < 6) { es("  @lit.imm r"); en(k); es(", 0\n"); k = k + 1; }
    return 0;
}

/* Does this literal format need the RUNTIME formatter -- a flag, a width,
   a precision?  The desugared printf writes each conversion bare; the
   library's `_u_vfmt` does the padding, and it is ordinary C that this
   compiler now compiles, so the two front ends format through the same
   code. */
int fmtneedsrt(int ft) {
    int k; int e; int c;
    k = tpos[ft] + 1; e = tpos[ft] + tlen[ft] - 1;
    while (k < e) {
        if ((src[k] & 255) == 92) { k = k + 2; continue; }
        if ((src[k] & 255) == 37) {
            c = src[k + 1] & 255;
            if (c == 37) { k = k + 2; continue; }
            if (c == 102 || c == 70 || c == 101 || c == 69 || c == 103 || c == 71
                || c == 97 || c == 65) return 1;          /* %f %e %g %a */
            if (c == 108 || c == 122 || c == 106 || c == 116) {   /* %lu %lx %lo */
                int c2; c2 = src[k + 2] & 255;
                if (c2 == 108) c2 = src[k + 3] & 255;
                if (c2 == 117 || c2 == 120 || c2 == 88 || c2 == 111) return 1;
            }
            if (c == 45) return 1;              /* - */
            if (c == 43) return 1;              /* + */
            if (c == 32) return 1;
            if (c == 35) return 1;              /* # */
            if (c == 46) return 1;              /* . */
            if (isdi(c)) return 1;              /* 0 or a width */
        }
        k = k + 1;
    }
    return 0;
}

/* An argument to parameter k of function si: converted to the parameter's
   type when a prototype gave one, else the default argument promotions --
   float becomes double (C99 6.5.2.2p6-7), which is what `...` receives. */
int argconv(int si, int k) {
    if (si >= 0) { if (symkind[si] == 2) { if (symnpk[si] > k) { if (k < 8) {
        fconv(fkind(), sympk[si * 8 + k]);
        return 0;
    } } } }
    if (curflt == 4) { if (curptr == 0) fconv(4, 8); }
    return 0;
}
/* what a call of si hands back: a float or double, when it returns one */
int callres(int si) {
    curflt = 0; curcall = 1;
    if (si >= 0) { if (symkind[si] == 2) { if (symptr[si] == 0) { if (symflt[si]) {
        curflt = symflt[si]; cursize = curflt; curelem = curflt;
    } } } }
    return 0;
}

int pf_call(int t) {
    int n; int k; int psi;
    /* a call's value is an i64 on the type axis until return types are
       tracked; the fields describe the RESULT, not whatever came before */
    cursize = 8; curuns = 0; curstruct = 0 - 1; curdim2 = 0; curdim3 = 0;
    curcall = 1;                          /* whichever way the call is made */
    if (isname(t, "printf", 6)) {
        int ps; int useit;
        useit = 0;
        ps = sfind(t);
        if (ps >= 0) { if (symvar[ps]) {
            if (kind(tp + 1) == T_STR) { if (fmtneedsrt(tp + 1)) useit = 1; }
        } }
        if (ps >= 0) { if (symvar[ps]) { if (kind(tp + 1) != T_STR) useit = 1; } }
        if (kind(tp + 1) == T_STR) pf_dryrun(tp + 1);
        if (useit == 0) return do_printf();
        /* else: an ordinary variadic call on <stdio.h>'s printf */
    }
    if (isname(t, "__argc", 6)) {
        need(tidx("(", 1), "("); need(tidx(")", 1), ")");
        es("  .argc r0\n");
        lvalue = 0; curelem = 8; curptr = 0;
        return postfix();
    }
    if (isname(t, "__argv", 6)) {
        need(tidx("(", 1), "(");
        expr(); loadval();
        need(tidx(")", 1), ")");
        es("  .argv r0, r0\n");
        lvalue = 0; curelem = 1; curptr = 1;
        return postfix();
    }
    if (isname(t, "__mmap", 6)) {        /* six arguments: the .sys6 gate */
        need(tidx("(", 1), "(");
        n = 0;
        while (cur() != tidx(")", 1)) {
            if (cur() == T_EOF) break;
            expr(); loadval(); push(); n = n + 1;
            if (eat(tidx(",", 1)) == 0) break;
        }
        need(tidx(")", 1), ")");
        sysargs6(n);
        es("  .sys6 mmap, r0, r1, r2, r3, r4, r5\n");
        lvalue = 0; curelem = 8; curptr = 0;
        return postfix();
    }
    if (isname(t, "__open", 6) || isname(t, "__read", 6) ||
        isname(t, "__write", 7) || isname(t, "__close", 7) ||
        isname(t, "__mprotect", 10) || isname(t, "__munmap", 8) ||
        isname(t, "__exit", 6)) {
        need(tidx("(", 1), "(");
        n = 0;
        while (cur() != tidx(")", 1)) {
            if (cur() == T_EOF) break;
            expr(); loadval(); push(); n = n + 1;
            if (eat(tidx(",", 1)) == 0) break;
        }
        need(tidx(")", 1), ")");
        sysargs(n);
        es("  .sys ");
        if (isname(t, "__open", 6)) es("open");
        else { if (isname(t, "__read", 6)) es("read");
        else { if (isname(t, "__write", 7)) es("write");
        else { if (isname(t, "__close", 7)) es("close");
        else { if (isname(t, "__mprotect", 10)) es("mprotect");
        else { if (isname(t, "__munmap", 8)) es("munmap");
        else es("exit"); } } } } }
        es(", r0, r1, r2\n");
        lvalue = 0; curelem = 8; curptr = 0;
        return postfix();
    }
    if (isname(t, "va_start", 8)) {          /* ap = &arg[nfixed] */
        need(tidx("(", 1), "(");
        unary(); lvalue = 0; push();
        if (eat(tidx(",", 1))) { expr(); loadval(); }
        need(tidx(")", 1), ")");
        es("  @lit.imm r0, "); en(16 + 8 * fnnfixed); es("\n  @alu.add r0, r6, r0\n");
        pop1(); es("  @mem.store [r1+0], r0\n  @lit.imm r0, 0\n");
        lvalue = 0; curelem = 8; curptr = 0;
        return postfix();
    }
    if (isname(t, "va_end", 6)) {
        need(tidx("(", 1), "(");
        expr(); loadval();
        need(tidx(")", 1), ")");
        es("  @lit.imm r0, 0\n");
        lvalue = 0; curelem = 8; curptr = 0;
        return postfix();
    }
    if (isname(t, "__builtin_sqrt", 14) || isname(t, "__builtin_sqrtf", 15)) {
        /* the hardware square root, correctly rounded (C99 F.9.4.5) */
        int sk; sk = tlen[t] == 15 ? 4 : 8;
        need(tidx("(", 1), "(");
        expr(); loadval(); fconv(fkind(), sk);
        need(tidx(")", 1), ")");
        es(sk == 8 ? "  @fpu.dsqrt r0, r0\n" : "  @fpu.ssqrt r0, r0\n");
        setkind(sk);
        return postfix();
    }
    if (isname(t, "va_arg", 6)) {           /* *ap as T, then ap += 8 */
        int vw; int vp; int vfl;
        need(tidx("(", 1), "(");
        unary(); lvalue = 0;
        push();                                   /* &ap */
        es("  @mem.load r0, [r0+0]\n");
        push();                                   /* ap */
        need(tidx(",", 1), ",");
        vw = declspec(); vp = declspecptr; vfl = declflt;
        while (eatstar()) vp = 1;
        need(tidx(")", 1), ")");
        es("  @lit.imm r2, 8\n  @alu.add r0, r0, r2\n");
        es("  @mem.load r1, [r7+8]\n  @mem.store [r1+0], r0\n");   /* ap += 8 */
        pop1();                                   /* r1 = old ap */
        es("  @call.frame -8\n  mov r0, r1\n");
        if (vp) eload(8); else eload(vw);
        lvalue = 0; curelem = vw; curptr = vp;
        cursize = vw; if (vp) cursize = 8;
        curflt = vfl;
        return postfix();
    }
    need(vfind(TOKV, NTOKV, "(", 1), "(");
    n = 0;
    psi = sfind(t);
    if (psi >= 0) symused[psi] = 1;
    while (cur() != vfind(TOKV, NTOKV, ")", 1)) {
        if (cur() == T_EOF) break;
        expr(); loadval();
        if (curstruct >= 0) { if (curptr == 0) { if (curelem == 0) stemp(curstruct); } }
        argconv(psi, n);
        push(); n = n + 1;
        if (eat(vfind(TOKV, NTOKV, ",", 1)) == 0) break;
    }
    need(vfind(TOKV, NTOKV, ")", 1), ")");
    {
        int si; int st;
        si = sfind(t);
        st = 0;
        if (si >= 0) st = symvar[si];
        if (n > 6) st = 1;
        if (st) {
            /* Pushed in source order, arg[n-1] is nearest; reverse the block
               so arg[k] sits at [SP + 8k] and, after the call and the
               callee's frame push, at [FP + 16 + 8k]. */
            k = 0;
            while (k < n / 2) {
                es("  @mem.load r2, [r7+"); en(8 * k); es("]\n");
                es("  @mem.load r1, [r7+"); en(8 * (n - 1 - k)); es("]\n");
                es("  @mem.store [r7+"); en(8 * k); es("], r1\n");
                es("  @mem.store [r7+"); en(8 * (n - 1 - k)); es("], r2\n");
                k = k + 1;
            }
            es("  @call.call "); etok(t); ec(10);
            if (n > 0) { es("  @call.frame -"); en(8 * n); ec(10); }
            lvalue = 0; curelem = 8;
            callres(si);
            return postfix();
        }
    }
    k = n - 1;
    while (k >= 0) {
        es("  @mem.load r"); en(k); es(", [r7+0]\n  @call.frame -8\n");
        k = k - 1;
    }
    es("  @call.call "); etok(t); ec(10);
    lvalue = 0; curelem = 8;
    callres(sfind(t));
    {
        int si; si = sfind(t);
        if (si >= 0) { if (symfpret[si]) { curfn = 1; curfnst = symrfst[si]; } }
        if (si >= 0) { if (symkind[si] == 2) { if (symstruct[si] >= 0) {
            if (symptr[si] == 0) { curstruct = symstruct[si]; curelem = 0; curptr = 0; }
            /* a pointer to a struct: `get()->f` needs to know which */
            else { curstruct = symstruct[si]; curptr = 1; curelem = stsize[symstruct[si]]; }
        } } }
    }
    return postfix();
}

/* ---- binary expressions --------------------------------------------- */
int BOP[16];
int BLEV[16];
int nbop;

int binop_level(int k) { int i; i = 0; while (i < nbop) { if (BOP[i] == k) return BLEV[i]; i = i + 1; } return 0 - 1; }

int emit_binop(int k) {
    pop1();
    /* narrow_pair on the Python side: an unsigned operation below 64 bits
       converts BOTH operands first, or (int)-1 != 0xffffffffu comes out true */
    if (binuns) { if (binwid < 8) {
        es("  @lit.imm r2, ");
        if (binwid == 1) en(255);
        if (binwid == 2) en(65535);
        if (binwid == 4) en(4294967295);
        es("\n  @alu.and r0, r0, r2\n  @alu.and r1, r1, r2\n");
    } }
    if (binuns) {
        if (k == vfind(TOKV, NTOKV, "/", 1))  { es("  .udiv r0, r1, r0\n"); return 0; }
        if (k == vfind(TOKV, NTOKV, "%", 1))  { es("  .umod r0, r1, r0\n"); return 0; }
        if (k == vfind(TOKV, NTOKV, "<", 1))  { es("  @alu.ult r0, r1, r0\n"); return 0; }
        if (k == vfind(TOKV, NTOKV, ">", 1))  { es("  @alu.ugt r0, r1, r0\n"); return 0; }
        if (k == vfind(TOKV, NTOKV, "<=", 2)) { es("  @alu.ule r0, r1, r0\n"); return 0; }
        if (k == vfind(TOKV, NTOKV, ">=", 2)) { es("  @alu.uge r0, r1, r0\n"); return 0; }
        if (k == vfind(TOKV, NTOKV, ">>", 2)) { es("  @alu.lshr r0, r1, r0\n"); return 0; }
    }
    if (k == vfind(TOKV, NTOKV, "+", 1))  { es("  @alu.add r0, r1, r0\n"); return 0; }
    if (k == vfind(TOKV, NTOKV, "-", 1))  { es("  @alu.sub r0, r1, r0\n"); return 0; }
    if (k == vfind(TOKV, NTOKV, "*", 1))  { es("  @alu.mul r0, r1, r0\n"); return 0; }
    if (k == vfind(TOKV, NTOKV, "/", 1))  { es("  .div r0, r1, r0\n"); return 0; }
    if (k == vfind(TOKV, NTOKV, "%", 1))  { es("  .mod r0, r1, r0\n"); return 0; }
    if (k == vfind(TOKV, NTOKV, "<", 1))  { es("  @alu.lt r0, r1, r0\n"); return 0; }
    if (k == vfind(TOKV, NTOKV, ">", 1))  { es("  @alu.gt r0, r1, r0\n"); return 0; }
    if (k == vfind(TOKV, NTOKV, "<=", 2)) { es("  @alu.le r0, r1, r0\n"); return 0; }
    if (k == vfind(TOKV, NTOKV, ">=", 2)) { es("  @alu.ge r0, r1, r0\n"); return 0; }
    if (k == vfind(TOKV, NTOKV, "==", 2)) { es("  @alu.eq r0, r1, r0\n"); return 0; }
    if (k == vfind(TOKV, NTOKV, "!=", 2)) { es("  @alu.ne r0, r1, r0\n"); return 0; }
    if (k == vfind(TOKV, NTOKV, "&", 1))  { es("  @alu.and r0, r1, r0\n"); return 0; }
    if (k == vfind(TOKV, NTOKV, "|", 1))  { es("  @alu.or r0, r1, r0\n"); return 0; }
    if (k == vfind(TOKV, NTOKV, "^", 1))  { es("  @alu.xor r0, r1, r0\n"); return 0; }
    if (k == vfind(TOKV, NTOKV, "<<", 2)) { es("  @alu.shl r0, r1, r0\n"); return 0; }
    es("  @alu.shr r0, r1, r0\n");
    return 0;
}

/* ---- the type table [W-4] ----------------------------------------------
   The result type of `t1 op t2` is the NET's answer, keyed exactly as the
   Python walker keys it: both operands projected onto the TYS axis, the
   operator onto the canonical TOPS axis.  Signedness and wrap width come
   from the `+` row, which is the usual arithmetic conversion.  Rules for
   these used to be written out here by hand; they were a second copy of the
   table, and a copy is exactly what this compiler is not supposed to have. */
/* ---- floating constants: decimal to binary, exactly --------------------
   A constant's bits must be the ones the Python front end gets from
   float(text): the value rounded ONCE, to nearest, ties to even.  There is
   no strtod to call, and a loop of multiplications by ten rounds at every
   step, so this is done in integers: the digits as a big number M and a
   power of ten, M * 10^e exactly when e >= 0, and when e < 0 the quotient
   M * 2^s / 10^-e with its remainder as the sticky bit.  One rounding, at
   the precision the result actually has -- a subnormal's included. */
#define FB_L 160                       /* 32-bit limbs: 5120 bits */
unsigned long fbA[FB_L]; int fbAn;     /* the working number */
unsigned long fbD[FB_L]; int fbDn;     /* the divisor, 10^k */
unsigned long fbQ[FB_L]; int fbQn;
int fb_bits(unsigned long *a, int n) {  /* bit length */
    unsigned long t; int b;
    while (n > 0) { if (a[n - 1] != 0) break; n = n - 1; }
    if (n == 0) return 0;
    t = a[n - 1]; b = 0;
    while (t) { b = b + 1; t = t >> 1; }
    return (n - 1) * 32 + b;
}
int fb_bit(unsigned long *a, int n, int i) {
    if (i < 0) return 0;
    if (i / 32 >= n) return 0;
    return (a[i / 32] >> (i % 32)) & 1;
}
int fb_mul(unsigned long *a, int n, unsigned long m) {   /* a *= m, m < 2^32 */
    unsigned long c; unsigned long t; int j;
    c = 0; j = 0;
    while (j < n) { t = a[j] * m + c; a[j] = t & 4294967295; c = t >> 32; j = j + 1; }
    if (c) { if (n < FB_L) { a[n] = c; n = n + 1; } }
    return n;
}
int fb_add(unsigned long *a, int n, unsigned long v) {   /* a += v, carried */
    int j; unsigned long t;
    j = 0;
    while (v) {
        if (j >= n) { if (n < FB_L) { a[n] = 0; n = n + 1; } else break; }
        t = a[j] + v; a[j] = t & 4294967295; v = t >> 32; j = j + 1;
    }
    return n;
}
int fb_shl1(unsigned long *a, int n) {                   /* a <<= 1 */
    unsigned long c; unsigned long t; int j;
    c = 0; j = 0;
    while (j < n) { t = (a[j] << 1) | c; c = t >> 32; a[j] = t & 4294967295; j = j + 1; }
    if (c) { if (n < FB_L) { a[n] = c; n = n + 1; } }
    return n;
}
int fb_cmp(unsigned long *a, int an, unsigned long *b, int bn) {
    int j;
    while (an > 0) { if (a[an - 1] != 0) break; an = an - 1; }
    while (bn > 0) { if (b[bn - 1] != 0) break; bn = bn - 1; }
    if (an != bn) return an > bn ? 1 : 0 - 1;
    j = an - 1;
    while (j >= 0) { if (a[j] != b[j]) return a[j] > b[j] ? 1 : 0 - 1; j = j - 1; }
    return 0;
}
int fb_sub(unsigned long *a, int an, unsigned long *b, int bn) {   /* a -= b, a >= b */
    long t; long br; int j;
    br = 0; j = 0;
    while (j < an) {
        t = (long)a[j] - br;
        if (j < bn) t = t - (long)b[j];
        if (t < 0) { t = t + 4294967296; br = 1; } else br = 0;
        a[j] = t; j = j + 1;
    }
    return an;
}
/* the bits of the constant in src[p..p+n): binary64, or binary32 if f32 */
/* A hexadecimal floating constant, C99 6.4.4.2: `0x1.8p1`.  Unlike a
   decimal one it names a binary value EXACTLY -- that is why it exists --
   so there is no long division here: the hex digits are the significand,
   `p` is a power of two, and the only rounding is the final fit into 52
   (or 23) bits, to nearest, ties to even. */
unsigned long fhex2bin(int p, int n, int f32) {
    int k; int c; int d; int fr; int nf; int sticky; int top; int e2;
    int mbits; int bias; int emax; int neg; int shift; int biased;
    long ev;
    unsigned long m; unsigned long half; unsigned long rem; unsigned long out;
    mbits = f32 ? 23 : 52; bias = f32 ? 127 : 1023; emax = f32 ? 255 : 2047;
    k = 2; m = 0; fr = 0; nf = 0; sticky = 0;
    while (k < n) {
        c = src[p + k] & 255;
        if (c == 46) { fr = 1; k = k + 1; continue; }
        d = 0 - 1;
        if (c >= 48 && c <= 57) d = c - 48;
        if (c >= 97 && c <= 102) d = c - 87;
        if (c >= 65 && c <= 70) d = c - 55;
        if (d < 0) break;
        /* keep the first sixteen hex digits exactly; any nonzero digit
           past them only matters as "something was below" for rounding */
        if ((m >> 60) == 0) { m = m * 16 + d; if (fr) nf = nf + 1; }
        else { if (d) sticky = 1; if (fr == 0) nf = nf - 1; }
        k = k + 1;
    }
    ev = 0; neg = 0;
    if (k < n) { if ((src[p + k] & 255) == 112 || (src[p + k] & 255) == 80) {
        k = k + 1;
        if (k < n) { if ((src[p + k] & 255) == 45) { neg = 1; k = k + 1; }
                     else { if ((src[p + k] & 255) == 43) k = k + 1; } }
        while (k < n) {
            c = src[p + k] & 255;
            if (c < 48 || c > 57) break;
            if (ev < 100000) ev = ev * 10 + (c - 48);
            k = k + 1;
        }
    } }
    if (neg) ev = 0 - ev;
    if (m == 0) return 0;
    /* value = m * 2^(ev - 4*nf); put the top bit of m at position mbits */
    top = 63; while (((m >> top) & 1) == 0) top = top - 1;
    e2 = (int)ev - 4 * nf + top;          /* the unbiased exponent */
    biased = e2 + bias;
    if (biased >= emax) return f32 ? 0x7F800000 : 0x7FF0000000000000;  /* inf */
    shift = top - mbits;                   /* >0: drop bits, <0: add zeros */
    if (biased <= 0) shift = shift + (1 - biased);   /* subnormal */
    if (shift > 0) {
        if (shift >= 64) { rem = m; m = 0; half = 0; }
        else {
            rem = m & (((unsigned long)1 << shift) - 1);
            half = (unsigned long)1 << (shift - 1);
            m = m >> shift;
        }
        if (shift < 64) {
            if (rem > half || (rem == half && (sticky || (m & 1)))) m = m + 1;
            else { if (rem == half) { if (sticky) m = m + 1; } }
        }
    } else { m = m << (0 - shift); }
    if (biased <= 0) {
        /* a subnormal: the exponent field is zero, and rounding may carry
           it into the smallest normal, which the addition does for free */
        return m;
    }
    /* rounding may have carried into a new top bit */
    if ((m >> (mbits + 1)) & 1) { m = m >> 1; biased = biased + 1; }
    if (biased >= emax) return f32 ? 0x7F800000 : 0x7FF0000000000000;
    out = ((unsigned long)biased << mbits) | (m & (((unsigned long)1 << mbits) - 1));
    return out;
}

unsigned long fdec2bin(int p, int n, int f32) {
    int e10; int k; int seen; int c; int fr; int neg; int j;
    int prec; int emin; int bias; int mbits; int lead; int keep; int drop;
    int s; int L; int sticky; int exp;
    unsigned long m; unsigned long bitsout;
    fbAn = 1; fbA[0] = 0; e10 = 0; fr = 0; seen = 0; k = 0;
    while (k < n) {                       /* the significand's digits */
        c = src[p + k] & 255;
        if (c == 46) { fr = 1; k = k + 1; continue; }
        if (c < 48 || c > 57) break;
        fbAn = fb_mul(fbA, fbAn, 10);
        fbAn = fb_add(fbA, fbAn, c - 48);
        if (fr) e10 = e10 - 1;
        seen = 1; k = k + 1;
    }
    if (k < n) { c = src[p + k] & 255;
        if (c == 101 || c == 69) {        /* the exponent */
            int ev; int es;
            k = k + 1; es = 1; ev = 0;
            if ((src[p + k] & 255) == 45) { es = 0 - 1; k = k + 1; }
            else { if ((src[p + k] & 255) == 43) k = k + 1; }
            while (k < n) { c = src[p + k] & 255; if (c < 48 || c > 57) break;
                            if (ev < 100000) ev = ev * 10 + (c - 48); k = k + 1; }
            e10 = e10 + es * ev;
        } }
    prec = 53; emin = 0 - 1022; bias = 1023; mbits = 52;
    if (f32) { prec = 24; emin = 0 - 126; bias = 127; mbits = 23; }
    if (fb_bits(fbA, fbAn) == 0) return 0;
    if (e10 > 400) e10 = 400;             /* past any double: overflows below */
    if (e10 < 0 - 800) return 0;           /* below any subnormal */
    s = 0;
    if (e10 >= 0) {
        j = 0; while (j < e10) { fbAn = fb_mul(fbA, fbAn, 10); j = j + 1; }
        sticky = 0;
        fbQn = fbAn; j = 0; while (j < fbAn) { fbQ[j] = fbA[j]; j = j + 1; }
    } else {
        /* Q = floor(M * 2^s / 10^k), s large enough for prec+2 bits */
        fbDn = 1; fbD[0] = 1;
        j = 0; while (j < 0 - e10) { fbDn = fb_mul(fbD, fbDn, 10); j = j + 1; }
        s = prec + 3 + fb_bits(fbD, fbDn) - fb_bits(fbA, fbAn);
        if (s < 0) s = 0;
        j = 0; while (j < s) { fbAn = fb_shl1(fbA, fbAn); j = j + 1; }
        /* long division, a bit at a time: R in fbA's place, Q built up */
        {   unsigned long R[FB_L]; int Rn; int i; int nb;
            Rn = 1; R[0] = 0; fbQn = fbAn; j = 0; while (j < fbQn) { fbQ[j] = 0; j = j + 1; }
            nb = fb_bits(fbA, fbAn); i = nb - 1;
            while (i >= 0) {
                Rn = fb_shl1(R, Rn);
                R[0] = R[0] | fb_bit(fbA, fbAn, i);
                if (fb_cmp(R, Rn, fbD, fbDn) >= 0) {
                    Rn = fb_sub(R, Rn, fbD, fbDn);
                    fbQ[i / 32] = fbQ[i / 32] | ((unsigned long)1 << (i % 32));
                }
                i = i - 1;
            }
            sticky = fb_bits(R, Rn) != 0;
        }
    }
    L = fb_bits(fbQ, fbQn);
    lead = L - 1 - s;                     /* the leading bit's binary exponent */
    keep = prec;
    if (lead < emin) keep = prec - (emin - lead);   /* subnormal: fewer bits */
    if (keep < 0) keep = 0 - 1;
    drop = L - keep;                      /* bits below the rounding point */
    m = 0; j = L - 1;
    while (j >= drop) { m = (m << 1) | fb_bit(fbQ, fbQn, j); j = j - 1; }   /* 0 below bit 0 */
    if (keep < 0) m = 0;
    {   int half; int rest; int jj;
        half = fb_bit(fbQ, fbQn, drop - 1);
        rest = sticky; jj = drop - 2;
        while (jj >= 0) { if (fb_bit(fbQ, fbQn, jj)) { rest = 1; break; } jj = jj - 1; }
        if (drop <= 0) { half = 0; rest = 0; }
        if (half) { if (rest || (m & 1)) m = m + 1; }
    }
    exp = drop - s;                       /* value = m * 2^exp */
    if (m >> prec) { m = m >> 1; exp = exp + 1; }     /* rounded up to 2^prec */
    if (m == 0) return 0;
    lead = exp + prec - 1;
    if (m < ((unsigned long)1 << (prec - 1))) {        /* subnormal */
        bitsout = m;
    } else {
        if (lead + bias >= 2 * bias + 1) {             /* overflow: inf */
            if (f32) return 2139095040;
            return 9218868437227405312;
        }
        bitsout = ((unsigned long)(lead + bias) << mbits) | (m & (((unsigned long)1 << mbits) - 1));
    }
    return bitsout;
}

/* is the NUM token t a floating constant?  0, 4 (float) or 8 (double) */
int fltlit(int t) {
    int k; int c; int hex;
    k = 0; hex = 0;
    if (tlen[t] > 1) { if ((src[tpos[t]] & 255) == 48) { c = src[tpos[t] + 1] & 255; if (c == 120 || c == 88) hex = 1; } }
    if (hex) {
        /* hex is an integer unless it has a binary exponent: `0x1.8p1` */
        while (k < tlen[t]) {
            c = src[tpos[t] + k] & 255;
            if (c == 112 || c == 80) {
                c = src[tpos[t] + tlen[t] - 1] & 255;
                if (c == 102 || c == 70) return 4;
                return 8;
            }
            k = k + 1;
        }
        return 0;
    }
    if ((src[tpos[t]] & 255) == 39) return 0;          /* a character constant */
    while (k < tlen[t]) {
        c = src[tpos[t] + k] & 255;
        if (c == 46 || c == 101 || c == 69) {
            c = src[tpos[t] + tlen[t] - 1] & 255;
            if (c == 102 || c == 70) return 4;
            return 8;
        }
        k = k + 1;
    }
    return 0;
}

int tyax(void) {                   /* the value in hand, on the TYS axis */
    if (curptr) return vfind(TYSV, NTYSV, "ptr", 3);
    if (curstruct >= 0) return vfind(TYSV, NTYSV, "struct", 6);
    if (curflt == 8) return vfind(TYSV, NTYSV, "f64", 3);
    if (curflt == 4) return vfind(TYSV, NTYSV, "f32", 3);
    if (curuns) {
        if (cursize == 1) return vfind(TYSV, NTYSV, "u8", 2);
        if (cursize == 2) return vfind(TYSV, NTYSV, "u16", 3);
        if (cursize == 4) return vfind(TYSV, NTYSV, "u32", 3);
        return vfind(TYSV, NTYSV, "u64", 3);
    }
    if (cursize == 1) return vfind(TYSV, NTYSV, "i8", 2);
    if (cursize == 2) return vfind(TYSV, NTYSV, "i16", 3);
    if (cursize == 4) return vfind(TYSV, NTYSV, "i32", 3);
    return vfind(TYSV, NTYSV, "i64", 3);
}

/* The value in hand as a conversion kind: 8 double, 4 float, 1 u64 (the
   one integer the signed conversions get wrong), 0 any other integer --
   narrower unsigned values are already zero-extended in the register. */
int fkind(void) {
    if (curptr) return 1;
    if (curflt) return curflt;
    if (curuns) { if (cursize == 8) return 1; }
    return 0;
}
/* r0 from kind `from` to kind `to` (C99 6.3.1.4-5); integer to integer is
   the store's business.  Every op is the irsel net's `fpu` family. */
int fconv(int from, int to) {
    /* Conversion kind 9 is "to _Bool": C99 6.3.1.2 makes it a COMPARISON,
       not a truncation -- 0 if the value compares equal to 0, else 1 --
       so it is handled before the float conversions, and after them, so
       that `_Bool b = 0.5;` is 1 rather than (int)0.5. */
    if (to == 9) {
        /* A float compares against 0.0, not against (int)value: C99 says
           `_Bool b = 0.5;` is 1, and truncating first would make it 0. */
        if (from == 8) {
            es("  @lit.imm r1, 0\n  @fpu.deq r0, r0, r1\n"
               "  @lit.imm r1, 1\n  @alu.xor r0, r0, r1\n");
            return 0;
        }
        if (from == 4) {
            es("  @lit.imm r1, 0\n  @fpu.seq r0, r0, r1\n"
               "  @lit.imm r1, 1\n  @alu.xor r0, r0, r1\n");
            return 0;
        }
        es("  @lit.imm r2, 0\n  @alu.ne r0, r0, r2\n");
        return 0;
    }
    if (from == 9) from = 0;
    if (from == to) return 0;
    if (from < 4) { if (to < 4) return 0; }
    if (from == 4) {
        es("  @fpu.s2d r0, r0\n");
        if (to == 8) return 0;
        from = 8;
    }
    if (from == 8) {
        if (to == 4) { es("  @fpu.d2s r0, r0\n"); return 0; }
        if (to == 1) es("  @fpu.d2u r0, r0\n"); else es("  @fpu.d2i r0, r0\n");
        return 0;
    }
    if (to == 8) { if (from == 1) es("  @fpu.u2d r0, r0\n"); else es("  @fpu.i2d r0, r0\n"); return 0; }
    if (from == 1) es("  @fpu.u2s r0, r0\n"); else es("  @fpu.i2s r0, r0\n");
    return 0;
}
/* ...and the value's description follows it */
int setkind(int k) {
    lvalue = 0; curptr = 0; curstruct = 0 - 1; curdim2 = 0; curdim3 = 0;
    if (k >= 4) { curflt = k; cursize = k; curelem = k; curuns = 0; return 0; }
    curflt = 0;
    if (k == 1) { curuns = 1; cursize = 8; curelem = 8; }
    return 0;
}
/* a floating value about to be TESTED becomes 0 or 1: 1 unless it compares
   equal to zero, so -0.0 is false and NaN is true (C99 6.8.4.1) */
int ftruthy(void) {
    if (curflt == 0) return 0;
    if (curptr) return 0;
    es("  @lit.imm r1, 0\n");
    if (curflt == 8) es("  @fpu.deq r0, r0, r1\n"); else es("  @fpu.seq r0, r0, r1\n");
    es("  @lit.imm r1, 1\n  @alu.xor r0, r0, r1\n");
    curflt = 0; cursize = 4; curelem = 4;
    return 0;
}
/* the bits of 1.0 in a floating kind */
long fone(int k) { if (k == 8) return 4607182418800017408; return 1065353216; }

/* the data label of global symbol i (token t): g_NAME, or a static local's */
int symlea(int i, int t, char *reg) {
    es("  @mem.lea "); es(reg);
    if (symlab[i] >= 0) { es(", ls"); en(symlab[i]); }
    else { es(", g_"); etok(t); }
    ec(10);
    return 0;
}

int tyask(int l, char *op, int ol, int r) {
    int key[4];
    key[0] = l; key[1] = vfind(TOPSV, NTOPSV, op, ol); key[2] = r; key[3] = 0;
    return inf(S_TYPE, key, 0);
}

int tyis(int t, char *nm, int L) { return t == vfind(TYOUTV, NTYOUTV, nm, L); }

/* a type's size and signedness: the tyinfo stage, as sema.py asks it */
int tyinfo(int t, int head) {
    int key[4]; key[0] = t; key[1] = 0; key[2] = 0; key[3] = 0;
    return inf(S_TYINFO, key, head);
}

int tysize(int t) {                /* bytes, for an integer result */
    int c;
    c = tyinfo(t, HD_TYINFO_SIZE);
    return BH_TYINFO_SIZE[voff(BH_TYINFO_SIZE, c)] - 48;
}

int tyuns(int t) {
    int c;
    c = tyinfo(t, HD_TYINFO_UNS);
    return BH_TYINFO_UNS[voff(BH_TYINFO_UNS, c)] - 48;
}

/* the operator token, projected onto the table's canonical TOPS axis */
int tycanon(int k, char *buf) {
    int p; int L;
    p = voff(TOKV, k); L = vlen(TOKV, k);
    if (k == tidx(">", 1)) { buf[0] = 60; return 1; }
    if (k == tidx("<=", 2)) { buf[0] = 60; return 1; }
    if (k == tidx(">=", 2)) { buf[0] = 60; return 1; }
    if (k == tidx("!=", 2)) { buf[0] = 61; buf[1] = 61; return 2; }
    buf[0] = TOKV[p];
    if (L > 1) buf[1] = TOKV[p + 1];
    return L;
}

int binary(int level) {
    int k; int e; int lp; int lax; int rax; int ck; int res; int cl;
    int lf; int rf; int lk; int lfr; int rfr; int re; int rp; int lpd; int rpd; int lbase; int rbase;
    char cb[4];
    if (level > 7) {
        if (havepre) { havepre = 0; return 0; }     /* parsed already, by expr */
        unary(); return 0;
    }
    binary(level + 1);
    while (1) {
        k = cur();
        if (binop_level(k) != level) break;
        loadval();
        e = curelem;
        lp = curptr;
        lfr = curflt;                              /* float, or a pointee's */
        lf = 0; if (curptr == 0) { if (curstruct < 0) lf = curflt; }
        lk = fkind();
        lpd = curpd; lbase = curbase;
        lax = tyax();
        adv();
        push();
        binary(level + 1);
        loadval();
        rf = 0; if (curptr == 0) { if (curstruct < 0) rf = curflt; }
        rfr = curflt; re = curelem; rp = curptr; rpd = curpd; rbase = curbase;
        rax = tyax();
        ck = tyask(lax, "+", 1, rax);              /* the conversion row */
        cl = tycanon(k, cb);
        res = tyask(lax, cb, cl, rax);             /* this operator's row */
        if (lf || rf) {
            /* A floating operand: both go to the common type the TYPE TABLE
               names in its `+` row (the usual arithmetic conversion), and
               the op is the irsel net's fpu family. */
            int cf; int isarith;
            if (tyis(res, "illegal", 7)) err_tok(tp, "this operator takes no floating operand");
            cf = 4; if (tyis(ck, "f64", 3)) cf = 8;
            fconv(fkind(), cf);                    /* rhs, in r0 */
            es("  @call.frame 8\n  @mem.store [r7+0], r0\n  @mem.load r0, [r7+8]\n");
            fconv(lk, cf);                         /* lhs, from under it */
            es("  mov r1, r0\n  @mem.load r0, [r7+0]\n  @call.frame -16\n");
            isarith = 0;
            if (k == tidx("+", 1)) { es(cf == 8 ? "  @fpu.dadd r0, r1, r0\n" : "  @fpu.sadd r0, r1, r0\n"); isarith = 1; }
            if (k == tidx("-", 1)) { es(cf == 8 ? "  @fpu.dsub r0, r1, r0\n" : "  @fpu.ssub r0, r1, r0\n"); isarith = 1; }
            if (k == tidx("*", 1)) { es(cf == 8 ? "  @fpu.dmul r0, r1, r0\n" : "  @fpu.smul r0, r1, r0\n"); isarith = 1; }
            if (k == tidx("/", 1)) { es(cf == 8 ? "  @fpu.ddiv r0, r1, r0\n" : "  @fpu.sdiv r0, r1, r0\n"); isarith = 1; }
            /* a > b is b < a; != is not == -- which is also NaN's answer */
            if (k == tidx("<", 1)) es(cf == 8 ? "  @fpu.dlt r0, r1, r0\n" : "  @fpu.slt r0, r1, r0\n");
            if (k == tidx(">", 1)) es(cf == 8 ? "  @fpu.dgt r0, r1, r0\n" : "  @fpu.sgt r0, r1, r0\n");
            if (k == tidx("<=", 2)) es(cf == 8 ? "  @fpu.dle r0, r1, r0\n" : "  @fpu.sle r0, r1, r0\n");
            if (k == tidx(">=", 2)) es(cf == 8 ? "  @fpu.dge r0, r1, r0\n" : "  @fpu.sge r0, r1, r0\n");
            if (k == tidx("==", 2)) es(cf == 8 ? "  @fpu.deq r0, r1, r0\n" : "  @fpu.seq r0, r1, r0\n");
            if (k == tidx("!=", 2)) { es(cf == 8 ? "  @fpu.deq r0, r1, r0\n" : "  @fpu.seq r0, r1, r0\n");
                                      es("  @lit.imm r1, 1\n  @alu.xor r0, r0, r1\n"); }
            if (isarith) setkind(cf);
            else { setkind(0); cursize = 4; curelem = 4; }
            continue;
        }
        binuns = tyuns(ck);
        binwid = tysize(ck);
        if (lp == 0) { if (rp) { if (re > 1) { if (k == tidx("+", 1)) {
            /* `n + p`: the INTEGER is on the stack; scale it there */
            es("  @call.frame 8\n  @mem.store [r7+0], r0\n  @mem.load r0, [r7+8]\n");
            es("  @lit.imm r2, "); en(re); es("\n  @alu.mul r0, r0, r2\n  @mem.store [r7+8], r0\n");
            es("  @mem.load r0, [r7+0]\n  @call.frame -8\n");
        } } } }
        if (lp) { if (e > 1) {
            if (k == tidx("+", 1)) { es("  @lit.imm r2, "); en(e); es("\n  @alu.mul r0, r0, r2\n"); }
            if (k == tidx("-", 1)) { if (curptr == 0) {
                es("  @lit.imm r2, "); en(e); es("\n  @alu.mul r0, r0, r2\n"); } }
        } }
        emit_binop(k);
        /* C99 6.5.6p9: the difference of two pointers counts ELEMENTS */
        if (lp) { if (curptr) { if (e > 1) { if (k == tidx("-", 1)) {
            es("  @lit.imm r2, "); en(e); es("\n  .div r0, r0, r2\n");
        } } } }
        if (tyis(res, "ptr", 3) == 0) { if (tyuns(res)) zext(tysize(res)); }
        binuns = 0; binwid = 8;
        lvalue = 0; curstruct = 0 - 1; curdim2 = 0; curdim3 = 0;
        if (tyis(res, "ptr", 3)) { curelem = e; curptr = lp; curuns = 0; cursize = 8;
                                   curflt = lfr; curpd = lpd; curbase = lbase;
                                   if (lp == 0) { curptr = 1; curelem = re; curflt = rfr;
                                                  curpd = rpd; curbase = rbase; } }
        else { curptr = 0; cursize = tysize(res); curelem = cursize;
               curuns = tyuns(res); curflt = 0; }
    }
    return 0;
}

int land(void) {
    int end;
    binary(0);
    while (cur() == vfind(TOKV, NTOKV, "&&", 2)) {
        loadval(); ftruthy(); adv();
        end = newlab();
        elab("  @ctrl.jumpz r0, L", end); ec(10);
        binary(0); loadval(); ftruthy();
        es("  @lit.imm r1, 0\n  @alu.ne r0, r0, r1\n");
        elab("L", end); es(":\n");
        setkind(0); cursize = 4; curelem = 4; curptr = 0;   /* an int (6.5.13p3) */
    }
    return 0;
}

int lor(void) {
    int end; int rhs;
    land();
    while (cur() == vfind(TOKV, NTOKV, "||", 2)) {
        loadval(); ftruthy(); adv();
        end = newlab(); rhs = newlab();
        elab("  @ctrl.jumpz r0, L", rhs); ec(10);
        es("  @lit.imm r0, 1\n");
        elab("  @ctrl.jump L", end); ec(10);
        elab("L", rhs); es(":\n");
        land(); loadval(); ftruthy();
        es("  @lit.imm r1, 0\n  @alu.ne r0, r0, r1\n");
        elab("L", end); es(":\n");
        setkind(0); cursize = 4; curelem = 4; curptr = 0;   /* an int (6.5.14p3) */
    }
    return 0;
}

/* `a ? b : c` -- only one arm is evaluated, so each gets its own label and
   the value they share is r0. */
int cond(void) {
    int els; int end; int p1; int e1;
    lor();
    if (cur() != tidx("?", 1)) return 0;
    loadval(); ftruthy(); adv();
    els = newlab(); end = newlab();
    elab("  @ctrl.jumpz r0, L", els); ec(10);
    {   int save; int nsave; int k1; int k2; int a1; int cf;
        save = tp; nsave = nout;
        expr(); loadval(); k1 = fkind(); a1 = tyax(); p1 = curptr; e1 = curelem;
        elab("  @ctrl.jump L", end); ec(10);
        elab("L", els); es(":\n");
        need(tidx(":", 1), ":");
        cond(); loadval(); k2 = fkind();
        if ((k1 >= 4 || k2 >= 4) && k1 != k2) {
            /* C99 6.5.15p5: the arms meet in their common type.  The first
               was emitted before the second's type was known: again. */
            cf = tyis(tyask(a1, "+", 1, tyax()), "f64", 3) ? 8 : 4;
            tp = save; nout = nsave;
            expr(); loadval(); fconv(fkind(), cf);
            elab("  @ctrl.jump L", end); ec(10);
            elab("L", els); es(":\n");
            need(tidx(":", 1), ":");
            cond(); loadval(); fconv(fkind(), cf);
            setkind(cf);
        }
    }
    elab("L", end); es(":\n");
    /* C99 6.5.15p6: one arm a pointer and the other a null pointer
       constant -- the result is the pointer's type */
    if (p1) { if (curptr == 0) { curptr = 1; curelem = e1; } }
    lvalue = 0;
    return 0;
}

int exprc(void) {                   /* the comma operator */
    expr();
    while (cur() == tidx(",", 1)) { adv(); expr(); }
    return 0;
}

int aop(void) {                     /* += -= *= /= -> the plain operator */
    if (cur() == tidx("+=", 2)) return tidx("+", 1);
    if (cur() == tidx("-=", 2)) return tidx("-", 1);
    if (cur() == tidx("*=", 2)) return tidx("*", 1);
    if (cur() == tidx("/=", 2)) return tidx("/", 1);
    if (cur() == tidx("%=", 2)) return tidx("%", 1);
    if (cur() == tidx("&=", 2)) return tidx("&", 1);
    if (cur() == tidx("|=", 2)) return tidx("|", 1);
    if (cur() == tidx("^=", 2)) return tidx("^", 1);
    if (cur() == tidx("<<=", 3)) return tidx("<<", 2);
    if (cur() == tidx(">>=", 3)) return tidx(">>", 2);
    return 0 - 1;
}

/* -Wint-conversion: a pointer target given an integer that is not the
   null pointer constant.  The value's facts are in cur*, the target's
   were taken before the right-hand side; a function designator is a
   pointer in this sense and a lone `0` (NULL expands to it) is exempt. */
int intptr_check(int tptr, int rt) {
    if (tptr == 0) return 0;
    if (curcall) return 0;
    if (curptr || curfn || curflt) return 0;
    if (curstruct >= 0) return 0;
    if (tp == rt + 1) { if (kind(rt) == T_NUM) { if (numval(rt) == 0) return 0; } }
    warn_at(tpos[rt], "incompatible integer to pointer conversion [-Wint-conversion]");
    return 0;
}

int expr(void) {
    int save; int nsave; int e; int op; int isave; int psave; int pesave;
    int bl;                     /* the assignment target is _Bool */
    save = tp; nsave = nout; isave = nibuf; psave = npool; pesave = poolend;
    unary();
    if (lvalue) {
        op = aop();
        if (op >= 0) {
            int ptrl; int pel; int ak; int aax;
            adv();
            ptrl = curptr; pel = curelem;
            ak = fkind(); aax = tyax();
            e = stw();
            lvalue = 0;
            push();                                  /* address */
            eload(e);
            if (ptrl == 0) { if (e < BFTAG) { if (tyuns(aax)) zext(e); } }
            push();                                  /* old value */
            expr(); loadval();
            if (ak >= 4 || (curflt && curptr == 0)) {
                /* `x op= y` is `x = x op y` (6.5.16.2p3): in the common
                   type the table names, then back to x's */
                int cf;
                cf = tyis(tyask(aax, "+", 1, tyax()), "f64", 3) ? 8 : 4;
                fconv(fkind(), cf);
                es("  @call.frame 8\n  @mem.store [r7+0], r0\n  @mem.load r0, [r7+8]\n");
                fconv(ak, cf);
                es("  mov r1, r0\n  @mem.load r0, [r7+0]\n  @call.frame -16\n");
                if (op == tidx("+", 1)) es(cf == 8 ? "  @fpu.dadd r0, r1, r0\n" : "  @fpu.sadd r0, r1, r0\n");
                if (op == tidx("-", 1)) es(cf == 8 ? "  @fpu.dsub r0, r1, r0\n" : "  @fpu.ssub r0, r1, r0\n");
                if (op == tidx("*", 1)) es(cf == 8 ? "  @fpu.dmul r0, r1, r0\n" : "  @fpu.smul r0, r1, r0\n");
                if (op == tidx("/", 1)) es(cf == 8 ? "  @fpu.ddiv r0, r1, r0\n" : "  @fpu.sdiv r0, r1, r0\n");
                fconv(cf, ak);
                pop1();                              /* address */
                estore(e);
                setkind(ak);
                return 0;
            }
            /* `p += n` moves n ELEMENTS, as `p = p + n` does */
            if (ptrl) { if (pel > 1) {
                if (op == tidx("+", 1) || op == tidx("-", 1)) {
                    es("  @lit.imm r2, "); en(pel); es("\n  @alu.mul r0, r0, r2\n");
                }
            } }
            /* `x op= y` is done in the common type (6.5.16.2p3): unsigned
               when the table says so, or `h >>= 1` on a u64 shifted in
               the sign; then the value is x's, masked to x's width */
            if (ptrl == 0) {
                int ck2; ck2 = tyask(aax, "+", 1, tyax());
                binuns = tyuns(ck2); binwid = tysize(ck2);
            }
            emit_binop(op);                          /* pops old value */
            binuns = 0; binwid = 8;
            if (ptrl == 0) { if (e < BFTAG) { if (tyuns(aax)) zext(e); } }
            pop1();                                  /* address */
            estore(e);
            curelem = e;
            if (ptrl) { curptr = 1; curelem = pel; }
            return 0;
        }
        if (cur() == vfind(TOKV, NTOKV, "=", 1)) {
            int ak; int tptr; int rt;
            adv();
            tptr = curptr; rt = tp; curcall = 0;
            ak = fkind();
            if (curptr == 0) { if (curflt) ak = curflt; }
            e = stw();
            bl = curbool;               /* the TARGET's type, before the RHS */
            lvalue = 0;
            if (e == 0) { if (curstruct >= 0) {
                int ast; ast = curstruct;
                push();                              /* destination */
                expr(); loadval();
                es("  mov r1, r0\n  @mem.load r0, [r7+0]\n  @call.frame -8\n");
                scopy(stsize[ast]);
                lvalue = 0; curstruct = ast; curelem = 0; curptr = 0;
                return 0;
            } }
            push();
            expr(); loadval();
            intptr_check(tptr, rt);
            fconv(fkind(), ak);                      /* C99 6.5.16.1p2 */
            /* C99 6.3.1.2: converting to _Bool gives 0 if the value
               compares equal to 0, and 1 otherwise -- it is not a
               truncation, which is what storing one byte would be. */
            if (bl) es("  @lit.imm r2, 0\n  @alu.ne r0, r0, r2\n");
            pop1();
            estore(e);
            if (ak >= 4) setkind(ak);
            return 0;
        }
    }
    /* Not an assignment: what unary() just parsed is the LEFTMOST operand
       of the conditional expression.  Hand it down rather than rewinding
       and parsing it again -- the rewind re-parsed every operand once per
       enclosing parenthesis, 2^depth, and c-testsuite 00200 took 14 s in
       the Linux VM, past selfgap's limit.  The Python walker had the same
       rewind and the same fix. */
    havepre = 1;
    return cond();
}

/* ---- statements and declarations ------------------------------------- */
int brkstack[32]; int cntstack[32]; int nloop;
/* the block depth each break / continue target lives at: jumping out of a
   block that allocated a VLA has to give the stack back first */
int brkdep[32]; int cntdep[32];
int vlaback(int dep) {
    int d;
    d = dep + 1;
    while (d <= bdepth) {
        if (vlaslot[d]) { es("  @mem.load r7, [r6-"); en(vlaslot[d]); es("]\n"); return 0; }
        d = d + 1;
    }
    return 0;
}
/* ---- switch ----------------------------------------------------------
   The dispatch chain is emitted AFTER the body, because a case label is only
   known once the body has been walked.  The control value goes to a frame
   slot first: the chain reloads it for every comparison, and the body may
   have clobbered every register by then. */
int swval[256]; int swlab[256]; int nswv;
int swdef[16]; int swslot[16]; int nsw;

int patchnum(int at, int w, int v) {
    int k; int d;
    k = w - 1;
    if (v == 0) { out[at + k] = 48; k = k - 1; }
    while (v > 0) { if (k >= 0) out[at + k] = 48 + (v % 10); v = v / 10; k = k - 1; }
    while (k >= 0) { out[at + k] = 32; k = k - 1; }
    return 0;
}

int cexpr(void);

int catom(void) {
    int v; int k; int i;
    if (cur() == T_NUM) {
        v = 0; k = 0;
        v = numval(tp);
        adv();
        return v;
    }
    if (cur() == tidx("(", 1)) { adv(); v = cexpr(); need(tidx(")", 1), ")"); return v; }
    if (cur() == T_ID) {
        i = mfindt(tp);
        if (i >= 0) { if (machas[i]) { adv(); return macval[i]; } }
        i = sfind(tp);
        if (i >= 0) { if (symkind[i] == 4) { adv(); return symoff[i]; } }
    }
    err_tok(tp, "a constant is required here");
    __exit(1);
    return 0;
}

/* An integer constant expression (C99 6.6), with C's precedence.  It was
   a left-to-right chain of + - * /, so `N + 1 * 2` in an array bound was
   (N + 1) * 2, and `[1 && 1]` or `[1 ? 3 : 9]` were refused outright. */
int cprec(int k) {
    if (k == tidx("||", 2)) return 1;
    if (k == tidx("&&", 2)) return 2;
    if (k == tidx("|", 1)) return 3;
    if (k == tidx("^", 1)) return 4;
    if (k == tidx("&", 1)) return 5;
    if (k == tidx("==", 2)) return 6;
    if (k == tidx("!=", 2)) return 6;
    if (k == tidx("<", 1)) return 7;
    if (k == tidx(">", 1)) return 7;
    if (k == tidx("<=", 2)) return 7;
    if (k == tidx(">=", 2)) return 7;
    if (k == tidx("<<", 2)) return 8;
    if (k == tidx(">>", 2)) return 8;
    if (k == tidx("+", 1)) return 9;
    if (k == tidx("-", 1)) return 9;
    if (k == tidx("*", 1)) return 10;
    if (k == tidx("/", 1)) return 10;
    if (k == tidx("%", 1)) return 10;
    return 0;
}
long cunary(void) {
    int w;
    if (eat(tidx("-", 1))) return 0 - cunary();
    if (eat(tidx("+", 1))) return cunary();
    if (eat(tidx("!", 1))) { if (cunary()) return 0; return 1; }
    if (eat(tidx("~", 1))) return (0 - cunary()) - 1;
    if (cur() == tidx("sizeof", 6)) { if (kind(tp + 1) == tidx("(", 1)) { if (is_typeat(tp + 2)) {
        adv(); adv();
        w = declspec(); w = declsz;
        while (eatstar()) w = 8;
        need(tidx(")", 1), ")");
        return w;
    } } }
    /* a cast inside a constant expression: `(int)7` */
    if (cur() == tidx("(", 1)) { if (is_typeat(tp + 1)) {
        adv(); declspec(); while (eatstar()) { }
        need(tidx(")", 1), ")");
        return cunary();
    } }
    return catom();
}
long cbin(int minp) {
    long v; long r; int k; int pr;
    v = cunary();
    while (1) {
        k = cur(); pr = cprec(k);
        if (pr == 0) break;
        if (pr < minp) break;
        adv();
        r = cbin(pr + 1);
        if (pr == 1) { if (v) v = 1; else { if (r) v = 1; else v = 0; } }
        else { if (pr == 2) { if (v) { if (r) v = 1; else v = 0; } else v = 0; }
        else { if (k == tidx("|", 1)) v = v | r;
        else { if (k == tidx("^", 1)) v = v ^ r;
        else { if (k == tidx("&", 1)) v = v & r;
        else { if (k == tidx("==", 2)) v = v == r;
        else { if (k == tidx("!=", 2)) v = v != r;
        else { if (k == tidx("<", 1)) v = v < r;
        else { if (k == tidx(">", 1)) v = v > r;
        else { if (k == tidx("<=", 2)) v = v <= r;
        else { if (k == tidx(">=", 2)) v = v >= r;
        else { if (k == tidx("<<", 2)) v = v << r;
        else { if (k == tidx(">>", 2)) v = v >> r;
        else { if (k == tidx("+", 1)) v = v + r;
        else { if (k == tidx("-", 1)) v = v - r;
        else { if (k == tidx("*", 1)) v = v * r;
        else { if (k == tidx("/", 1)) v = v / r;
        else v = v % r; } } } } } } } } } } } } } } } }
    }
    return v;
}
/* Is the bound that starts at token j (up to its `]`) a constant
   expression?  Anything naming an object makes the array variable-length;
   `[1 && 1]` does not. */
int isconstdim(int j) {
    int depth; int k; int i;
    depth = 0;
    while (j < ntok) {
        k = kind(j);
        if (k == tidx("[", 1)) depth = depth + 1;
        if (k == tidx("]", 1)) { if (depth == 0) return 1; depth = depth - 1; }
        if (k == T_ID) {
            i = mfindt(j);
            if (i >= 0) { if (machas[i]) { j = j + 1; continue; } }
            i = sfind(j);
            if (i < 0) return 0;
            if (symkind[i] != 4) return 0;          /* not an enum constant */
        }
        j = j + 1;
    }
    return 1;
}

int cexpr(void) {
    long c; long a; long b;
    c = cbin(1);
    if (eat(tidx("?", 1))) {
        a = cexpr(); need(tidx(":", 1), ":"); b = cexpr();
        if (c) return a;
        return b;
    }
    return c;
}

/* `[k]` after `a[n][m]`: record k, make the row m*k, and refuse a fourth
   dimension out loud -- one was silently dropped, and sizeof came out 24
   for an int[2][3][5]. */
int dim3n;
int dim3decl(void) {
    dim3n = 1; decldim3 = 0;
    if (cur() != tidx("[", 1)) return 0;
    adv(); decldim3 = cexpr(); need(tidx("]", 1), "]");
    dim3n = decldim3;
    decldim2 = decldim2 * decldim3;
    if (cur() == tidx("[", 1)) { printf("arrays of more than three dimensions are not supported\n"); __exit(1); }
    return 0;
}

int alloc_local(int n) { frameoff = frameoff + n; if (frameoff > framemax) framemax = frameoff; return frameoff; }

int is_typetok(void) {
    if (cur() == T_TYPE) return 1;
    if (cur() == vfind(TOKV, NTOKV, "struct", 6)) return 1;
    if (cur() == vfind(TOKV, NTOKV, "union", 5)) return 1;
    return 0;
}

int stbody(int si);

int tdfind(int t) {
    int i; int k; int ok;
    if (kind(t) != T_ID) return 0 - 1;
    i = ntd - 1;                       /* backwards: an inner one shadows */
    while (i >= 0) {
        ok = 1; k = 0;
        while (k < tlen[t]) {
            if (k > 30) ok = 0;
            else { if (tdname[i * 32 + k] != src[tpos[t] + k]) ok = 0; }
            k = k + 1;
        }
        if (ok) { if (tdname[i * 32 + tlen[t]] == 0) return i; }
        i = i - 1;
    }
    return 0 - 1;
}

int tdadd(int t, int w, int sz, int si, int isptr) {
    int k;
    if (ntd >= MAXTD) { __write(2, "too many typedefs\n", 18); __exit(1); }
    k = 0;
    while (k < tlen[t]) { if (k < 31) tdname[ntd * 32 + k] = src[tpos[t] + k]; k = k + 1; }
    if (k > 31) k = 31;
    tdname[ntd * 32 + k] = 0;
    tdw[ntd] = w; tdsz[ntd] = sz; tdstruct[ntd] = si; tdptr[ntd] = isptr;
    tduns[ntd] = declunsigned;
    tdfp[ntd] = 0; tdfpst[ntd] = 0 - 1; tdflt[ntd] = declflt;
    ntd = ntd + 1;
    return ntd - 1;
}

int stfind(int t) {
    int i; int k; int ok;
    i = nstruct - 1;                     /* newest first: the innermost wins */
    while (i >= 0) {
        if (stdead[i]) { i = i - 1; continue; }
        ok = 1; k = 0;
        while (k < tlen[t]) {
            if (k > 30) ok = 0;
            else { if (stname[i * 32 + k] != src[tpos[t] + k]) ok = 0; }
            k = k + 1;
        }
        if (ok) { if (stname[i * 32 + tlen[t]] == 0) return i; }
        i = i - 1;
    }
    return 0 - 1;
}

int stnew(int t, int isunion) {
    int k;
    if (nstruct >= MAXSTRUCT) { __write(2, "too many structs\n", 17); __exit(1); }
    k = 0;
    if (t >= 0) {
        while (k < tlen[t]) { if (k < 31) stname[nstruct * 32 + k] = src[tpos[t] + k]; k = k + 1; }
    }
    if (k > 31) k = 31;
    stname[nstruct * 32 + k] = 0;
    stfirst[nstruct] = nmemb; stcount[nstruct] = 0;
    stdepth[nstruct] = bdepth; stdead[nstruct] = 0;
    stsize[nstruct] = 0; stalign[nstruct] = 1; stunion[nstruct] = isunion;
    nstruct = nstruct + 1;
    return nstruct - 1;
}

/* `struct` or `union`, with or without a tag, with or without a body. */
int stparse(int isunion) {
    int si; int t;
    adv();
    si = 0 - 1;
    if (cur() == T_ID) {
        t = tp;
        si = stfind(t);
        if (si < 0) si = stnew(t, isunion);
        /* a DEFINITION in a deeper block shadows the outer tag */
        else { if (kind(tp + 1) == tidx("{", 1)) { if (stdepth[si] < bdepth) si = stnew(t, isunion); } }
        adv();
    } else si = stnew(0 - 1, isunion);
    if (cur() == tidx("{", 1)) stbody(si);
    return si;
}

int mbfind(int si, int t) {
    int i; int e; int k; int ok;
    i = stfirst[si]; e = i + stcount[si];
    while (i < e) {
        ok = 1; k = 0;
        while (k < tlen[t]) {
            if (k > 30) ok = 0;
            else { if (mbname[i * 32 + k] != src[tpos[t] + k]) ok = 0; }
            k = k + 1;
        }
        if (ok) { if (mbname[i * 32 + tlen[t]] == 0) return i; }
        i = i + 1;
    }
    return 0 - 1;
}

int is_typeat(int i) {
    if (kind(i) == T_TYPE) return 1;
    if (kind(i) == vfind(TOKV, NTOKV, "struct", 6)) return 1;
    if (kind(i) == vfind(TOKV, NTOKV, "union", 5)) return 1;
    if (kind(i) == vfind(TOKV, NTOKV, "enum", 4)) return 1;   /* `(enum E) x` */
    if (tdfind(i) >= 0) return 1;
    return 0;
}

/* The size `sizeof` reports, which is NOT the storage width this compiler
   uses: locals live in 8-byte slots whatever their type, but `sizeof(int)`
   is 4 because that is what the type is.  [G-2] is about the width
   arithmetic is EVALUATED at, and says nothing about this. */
/* specifier keywords -> a type KIND (parsing, as sema.py's declspec does);
   the kind's size is the tyinfo stage's answer, not a number written here.
   kb: 0 void, 1 char, 2 short, 3 int, 4 long, 5 float, 6 double */
int kindsize(int kb, int un) {
    declvoid = kb == 0;
    char *nm; int L;
    nm = un ? "u32" : "i32";
    if (kb == 0) nm = "void";
    if (kb == 1) nm = un ? "u8" : "i8";
    if (kb == 2) nm = un ? "u16" : "i16";
    if (kb == 4) nm = un ? "u64" : "i64";
    if (kb == 5) nm = "f32";
    if (kb == 6) nm = "f64";
    L = 0; while (nm[L]) L = L + 1;
    return tysize(vfind(TYOUTV, NTYOUTV, nm, L));
}

int laststmt;            /* the body's last top-level statement returns, loops forever or exits */
int typesize(void) {
    int kb; int un;
    kb = 3; un = 0;
    while (is_typeat(tp)) {
        if (srcis(tpos[tp], tlen[tp], "char")) kb = 1;
        if (srcis(tpos[tp], tlen[tp], "short")) kb = 2;
        if (srcis(tpos[tp], tlen[tp], "long")) kb = 4;
        if (srcis(tpos[tp], tlen[tp], "void")) kb = 0;
        if (srcis(tpos[tp], tlen[tp], "unsigned")) un = 1;
        adv();
    }
    return kindsize(kb, un);
}

/* `enum [tag] { a, b = 3, c }` -- the constants go into the symbol table and
   the type itself is just an int. */
int enumspec(void) {
    int v; int t; int k; int neg;
    adv();
    if (cur() == T_ID) { if (kind(tp) != tidx("{", 1)) adv(); }
    if (cur() != tidx("{", 1)) return 4;          /* `enum E x;` -- a use */
    adv();
    v = 0;
    while (cur() != tidx("}", 1)) {
        if (cur() == T_EOF) break;
        t = adv();
        if (eat(tidx("=", 1))) {
            /* a constant expression: `B = A + 1`, `C = LAST_OTHER_CODE` */
            v = cexpr();
            if (v < 0) enumneg = 1;
        }
        declbytes = 4; declstruct = 0 - 1;
        sadd(t, 4, v, 8);                          /* 4 = enum constant */
        v = v + 1;
        if (eat(tidx(",", 1)) == 0) break;
    }
    need(tidx("}", 1), "}");
    return 4;
}

/* `*`, and the qualifiers that may follow it: `char *const p`,
   `int (*const x)`.  They change nothing this compiler tracks. */
int isqual(int t) {
    if (kind(t) != T_TYPE) return 0;
    if (srcis(tpos[t], tlen[t], "const")) return 1;
    if (srcis(tpos[t], tlen[t], "volatile")) return 1;
    if (srcis(tpos[t], tlen[t], "restrict")) return 1;
    return 0;
}
int eatstar(void) {
    if (eat(tidx("*", 1)) == 0) return 0;
    declpd = declpd + 1;
    while (isqual(tp)) adv();
    return 1;
}

/* Words that qualify a declaration without naming its type: they may come
   before a typedef name or `struct` (`const wchar_t *s`, `static struct S
   x`) and after one (`wchar_t const`).  Each is still a type word to the
   scope table, and is asked as one. */
int isspecq(int t) {
    if (kind(t) != T_TYPE) return 0;
    if (isqual(t)) return 1;
    if (srcis(tpos[t], tlen[t], "static")) return 1;
    if (srcis(tpos[t], tlen[t], "extern")) return 1;
    if (srcis(tpos[t], tlen[t], "register")) return 1;
    if (srcis(tpos[t], tlen[t], "inline")) return 1;
    if (srcis(tpos[t], tlen[t], "auto")) return 1;
    return 0;
}
int skipspecq(void) {
    while (isspecq(tp)) {
        if (srcis(tpos[tp], tlen[tp], "static")) declstatic = 1;
        if (infunc) scopewant("local", 5, tp, "type_name", 9);
        else scopewant("top", 3, tp, "type_name", 9);
        adv();
    }
    return 0;
}

int declspec(void) {                       /* -> element width */
    int w; int td; int kb;
    w = 8; kb = 3;
    declsz = 4;                            /* the size `sizeof` reports */
    declstruct = 0 - 1;
    declspecptr = 0;
    declunsigned = 0;
    declspecfp = 0; declspecfpst = 0 - 1;
    declenum = 0; declflt = 0; declspecpd = 0; declstatic = 0; declbool = 0;
    skipspecq();
    td = tdfind(tp);
    if (td >= 0) {
        adv();
        declsz = tdsz[td]; declstruct = tdstruct[td]; declspecptr = tdptr[td];
        declunsigned = tduns[td];
        declspecfp = tdfp[td];
        declspecfpst = tdfpst[td];
        declflt = tdflt[td];
        declspecpd = tdptr[td] ? tdpd[td] : 0;
        declbase = tdw[td];
        if (declstruct >= 0) declbase = stsize[declstruct];
        skipspecq();
        return tdw[td];
    }
    if (cur() == tidx("struct", 6)) {
        declstruct = stparse(0);
        declsz = stsize[declstruct];
        declbase = declsz;
        skipspecq();
        return 8;
    }
    if (cur() == tidx("union", 5)) {
        declstruct = stparse(1);
        declsz = stsize[declstruct];
        declbase = declsz;
        skipspecq();
        return 8;
    }
    if (cur() == tidx("enum", 4)) { declenum = 1; declsz = enumspec(); declbase = declsz; skipspecq(); return declsz; }
    while (is_typetok()) {
        if (infunc) scopewant("local", 5, tp, "type_name", 9);
        else scopewant("top", 3, tp, "type_name", 9);
        if (tlen[tp] == 4) { if (src[tpos[tp]] == 99) w = 1; }   /* char */
        if (srcis(tpos[tp], tlen[tp], "char")) kb = 1;
        if (srcis(tpos[tp], tlen[tp], "short")) kb = 2;
        if (srcis(tpos[tp], tlen[tp], "long")) kb = 4;
        if (srcis(tpos[tp], tlen[tp], "void")) kb = 0;
        /* C99 6.2.5p2: _Bool holds 0 or 1 and nothing else.  One byte and
           unsigned; what makes it a boolean rather than a narrow integer
           is the CONVERSION rule, not the width. */
        if (srcis(tpos[tp], tlen[tp], "_Bool")) { kb = 1; declunsigned = 1; declbool = 1; }
        if (srcis(tpos[tp], tlen[tp], "unsigned")) declunsigned = 1;
        if (srcis(tpos[tp], tlen[tp], "float")) { kb = 5; declflt = 4; }
        if (srcis(tpos[tp], tlen[tp], "double")) { kb = 6; declflt = 8; }  /* long double too */
        adv();
    }
    declsz = kindsize(kb, declunsigned);   /* the size: tyinfo's answer */
    /* The element width IS the type's size.  It used to be 1 for char and 8
       for everything else, which made an `int` array eight bytes per element
       and left no room for a struct member to be four. */
    w = declsz;
    declbase = w;
    return w;
}

/* The member list.  Alignment is the C rule -- each member starts on a
   boundary of its own alignment, the struct's alignment is the widest of
   them, and the total is rounded up to that -- because a struct's layout is
   observable through `sizeof` and through every pointer into it. */
int stbody(int si) {
    int off; int al; int w; int sz; int n; int t; int k;
    int msz; int mal; int mw; int mel; int mst; int mo; int muns;
    int own[256]; int nown; int j; int bitpos; int bw; int isbf; int menum; int mflt;
    int flex; int marr;                   /* this member is `name[]`: a flexible array */
    int mbl;                    /* ...and this one is _Bool */
    nown = 0; bitpos = 0; flex = 0;
    stopen[si] = 1;             /* this tag is INCOMPLETE until the `}` */
    need(tidx("{", 1), "{");
    stfirst[si] = nmemb; stcount[si] = 0;
    off = 0; al = 1;
    while (cur() != tidx("}", 1)) {
        if (cur() == T_EOF) break;
        w = declspec();
        sz = declsz; mst = declstruct; muns = declunsigned; menum = declenum; mflt = declflt;

        mbl = declbool;   /* saved like muns: declspec runs again per member */
        if (cur() == tidx(";", 1)) { if (mst >= 0) {
            /* An anonymous member (C11 6.7.2.1p13): its members are members
               of this aggregate, at its offset.  Spliced in by copy. */
            int a; int e; int first;
            msz = stsize[mst]; mal = stalign[mst];
            if (mal > 8) mal = 8;
            if ((bitpos + 7) / 8 > off) off = (bitpos + 7) / 8;
            if (stunion[si]) mo = 0;
            else {
                while (off - (off / mal) * mal) off = off + 1;
                mo = off; off = off + msz;
            }
            if (stunion[si]) { if (msz > off) off = msz; }
            if (mal > al) al = mal;
            bitpos = off * 8;
            a = stfirst[mst]; e = a + stcount[mst]; first = 1;
            while (a < e) {
                if (nmemb >= MAXMEMB) { __write(2, "too many members\n", 17); __exit(1); }
                k = 0;
                while (k < 32) { mbname[nmemb * 32 + k] = mbname[a * 32 + k]; k = k + 1; }
                mboff[nmemb] = mo + mboff[a]; mbbytes[nmemb] = mbbytes[a];
                mbwidth[nmemb] = mbwidth[a]; mbelem[nmemb] = mbelem[a];
                mbptr[nmemb] = mbptr[a]; mbstruct[nmemb] = mbstruct[a];
                mbuns[nmemb] = mbuns[a];
                mbskip[nmemb] = mbskip[a];
                mbpst[nmemb] = mbpst[a];
                mbflt[nmemb] = mbflt[a];
                mbptrd[nmemb] = mbptrd[a];
                if (stunion[mst]) { if (first == 0) mbskip[nmemb] = 1; }
                first = 0;
                if (nown >= 256) { __write(2, "too many members\n", 17); __exit(1); }
                own[nown] = nmemb; nown = nown + 1;
                nmemb = nmemb + 1;
                stcount[si] = stcount[si] + 1;
                a = a + 1;
            }
            adv();
            continue;
        } }
        while (1) {
            declptr = declspecptr; declpd = declspecpd;
            while (eatstar()) declptr = 1;
            t = 0 - 1;
            n = 1; marr = 0;
            if (cur() == tidx("(", 1)) { if (kind(tp + 1) == tidx("*", 1)) {
                /* `int (*fptr)();` -- a pointer member, called through
                   its value; `(*f[4])()` is an array of them */
                t = fpdecl(); declptr = 1; mst = 0 - 1;
                if (fpdim > 0) n = fpdim;
            } }
            flex = 0;
            if (t < 0) { if (cur() != tidx(":", 1)) t = adv(); }   /* `unsigned : 2;` names nothing */
            if (cur() == tidx("[", 1)) { marr = 1;
                /* C99 6.7.2.1p16: the LAST member of a struct may have an
                   incomplete array type -- `char d[];` -- and contributes
                   nothing to sizeof.  The bytes are whatever the caller
                   allocated past the struct, so the member is an offset
                   with a length of zero. */
                adv();
                if (cur() == tidx("]", 1)) { n = 0; flex = 1; } else n = cexpr();
                need(tidx("]", 1), "]");
            }
            isbf = 0;
            if (eat(tidx(":", 1))) {
                /* A bit-field.  C99 6.7.2.1 leaves the layout to the
                   implementation; this is what both our targets' ABIs do,
                   and what the Python walker does: pack in declaration
                   order, start a new storage unit when the field would
                   straddle one, and a width of 0 forces that break. */
                int unit; int pos; int uoff; int sig;
                bw = cexpr(); isbf = 1;
                unit = sz * 8;
                if (sz > al) al = sz;
                if (stunion[si]) {
                    if (sz > off) off = sz;
                    pos = 0;
                } else {
                    pos = bitpos;
                    if (off * 8 > pos) pos = off * 8;
                    if (bw == 0) {
                        pos = (pos + unit - 1) / unit * unit;
                        bitpos = pos;
                        if ((bitpos + 7) / 8 > off) off = (bitpos + 7) / 8;
                    } else {
                        if (pos % unit + bw > unit) pos = (pos + unit - 1) / unit * unit;
                        bitpos = pos + bw;
                        if ((bitpos + 7) / 8 > off) off = (bitpos + 7) / 8;
                    }
                }
                if (t < 0 || bw == 0) {
                    if (eat(tidx(",", 1)) == 0) break;
                    continue;
                }
                uoff = pos / unit * sz;
                /* plain int is signed here, as in gcc; an enum is unsigned
                   unless an enumerator is negative, or 148 in eight bits
                   reads back as -108 */
                sig = 1;
                if (muns) sig = 0;
                if (menum) { if (enumneg == 0) sig = 0; }
                mo = uoff; msz = sz; mw = bfenc(bw, pos - uoff * 8, sz, sig); mel = sz;
            }
            /* C99 6.7.2.1p2: a member cannot have an incomplete type, and
               the struct being defined is incomplete until its `}`.
               `struct S { struct S inner; }` used to be accepted and
               sizeof was whatever the half-built entry held.  Checked HERE,
               after the declarator: a POINTER to the same tag is how every
               list is written, and declptr is only known once the stars
               have been eaten. */
            if (isbf == 0) { if (mst >= 0) { if (declptr == 0) {
                if (stopen[mst]) {
                    err_tok(t, "a struct cannot contain itself by value");
                    __exit(1);
                } } } }
            if (isbf == 0) {
            msz = sz; mal = sz; mw = sz; mel = sz;
            if (mst >= 0) { msz = stsize[mst]; mal = stalign[mst]; mw = 0; mel = msz; }
            if (declptr) { msz = 8; mal = 8; mw = 8; mel = sz; if (mst >= 0) mel = stsize[mst]; }
            if (mal > 8) mal = 8;
            /* an array member is an aggregate whatever its length: `char
               x[1]` was `n > 1`-tested here and stayed a scalar, so `a.x`
               of a struct parameter loaded the byte instead of taking
               its address -- the -Wformat calibration found it, and the
               program it came from crashed */
            if (marr) { if (flex == 0) { mel = msz; msz = msz * n; mw = 0; } }
            /* The flexible member is an ARRAY of length zero: `mw = 0` is
               what marks a member as "its name is its address", and its
               size contributes nothing to the struct (C99 6.7.2.1p16).
               Without this it stayed a scalar and `s->d[0]` dereferenced
               the member's VALUE as a pointer. */
            if (flex) { mel = msz; msz = 0; mw = 0; }
            /* a plain member starts at the next byte, whatever bits precede it */
            if ((bitpos + 7) / 8 > off) off = (bitpos + 7) / 8;
            if (stunion[si]) mo = 0;
            else {
                while (off - (off / mal) * mal) off = off + 1;
                mo = off; off = off + msz;
            }
            if (stunion[si]) { if (msz > off) off = msz; }
            if (mal > al) al = mal;
            bitpos = off * 8;
            }
            if (nmemb >= MAXMEMB) { __write(2, "too many members\n", 17); __exit(1); }
            k = 0;
            while (k < tlen[t]) { if (k < 31) mbname[nmemb * 32 + k] = src[tpos[t] + k]; k = k + 1; }
            if (k > 31) k = 31;
            mbname[nmemb * 32 + k] = 0;
            mboff[nmemb] = mo; mbbytes[nmemb] = msz; mbwidth[nmemb] = mw;
            mbelem[nmemb] = mel; mbptr[nmemb] = declptr;
            mbstruct[nmemb] = 0 - 1;
            if (declptr == 0) { if (isbf == 0) mbstruct[nmemb] = mst; }
            mbpst[nmemb] = 0 - 1;
            if (declptr) mbpst[nmemb] = mst;
            mbflt[nmemb] = mflt;
            mbptrd[nmemb] = declptr ? (declpd > 0 ? declpd : 1) : 0;
            mbuns[nmemb] = muns; mbbool[nmemb] = mbl;
            mbskip[nmemb] = 0;
            if (stunion[si]) { if (stcount[si] > 0) mbskip[nmemb] = 1; }
            if (nown >= 256) { __write(2, "too many members\n", 17); __exit(1); }
            own[nown] = nmemb; nown = nown + 1;
            nmemb = nmemb + 1;
            stcount[si] = stcount[si] + 1;
            if (eat(tidx(",", 1)) == 0) break;
        }
        need(tidx(";", 1), ";");
    }
    need(tidx("}", 1), "}");
    /* A struct's members are the range [stfirst, stfirst + stcount).  A
       member whose type is DEFINED inline -- `union { ... } u;` -- put its
       own members in the middle of that range, and `u` fell off the end.
       Re-append ours as one block; the scattered originals are dead. */
    stopen[si] = 0;             /* complete from here on */
    stfirst[si] = nmemb;
    j = 0;
    while (j < nown) {
        if (nmemb >= MAXMEMB) { __write(2, "too many members\n", 17); __exit(1); }
        k = 0;
        while (k < 32) { mbname[nmemb * 32 + k] = mbname[own[j] * 32 + k]; k = k + 1; }
        mboff[nmemb] = mboff[own[j]]; mbbytes[nmemb] = mbbytes[own[j]];
        mbwidth[nmemb] = mbwidth[own[j]]; mbelem[nmemb] = mbelem[own[j]];
        mbptr[nmemb] = mbptr[own[j]]; mbstruct[nmemb] = mbstruct[own[j]];
        mbuns[nmemb] = mbuns[own[j]];
        mbbool[nmemb] = mbbool[own[j]];
        mbskip[nmemb] = mbskip[own[j]];
        mbpst[nmemb] = mbpst[own[j]];
        mbflt[nmemb] = mbflt[own[j]];
        mbptrd[nmemb] = mbptrd[own[j]];
        nmemb = nmemb + 1;
        j = j + 1;
    }
    if ((bitpos + 7) / 8 > off) off = (bitpos + 7) / 8;
    while (off - (off / al) * al) off = off + 1;
    stsize[si] = off; stalign[si] = al;
    return si;
}

int stmt(void);

int block(void) {
    int savesym; int saveoff; int savetd; int savest; int k;
    need(vfind(TOKV, NTOKV, "{", 1), "{");
    savesym = nsym; saveoff = frameoff; savetd = ntd; savest = nstruct;
    bdepth = bdepth + 1;
    vlaslot[bdepth] = 0;
    laststmt = 0;
    while (cur() != vfind(TOKV, NTOKV, "}", 1)) {
        if (cur() == T_EOF) { err_tok(tp, "unterminated block: the file ends inside it"); break; }
        stmt();
    }
    adv();
    if (vlaslot[bdepth]) {
        es("  @mem.load r7, [r6-"); en(vlaslot[bdepth]); es("]\n");
        vlaslot[bdepth] = 0;
    }
    /* -Wunused-variable: declared in this block, never read.  Written
       and never read counts too -- clang calls that "set but not used",
       on the same line.  Prototypes declared in a block are not
       variables; parameters were added before the block and are not
       in this range. */
    k = savesym;
    while (k < nsym) {
        if (symused[k] == 0) { if (symkind[k] == 1 || symkind[k] == 3) { if (symtok[k] >= 0) {
            char wm[64]; int q; int r;
            q = 0;
            while ("unused variable '"[q]) { wm[q] = "unused variable '"[q]; q = q + 1; }
            r = 0;
            while (symname[k * 32 + r] && r < 24) { wm[q] = symname[k * 32 + r]; q = q + 1; r = r + 1; }
            wm[q] = 39; q = q + 1;
            r = 0;
            while (" [-Wunused-variable]"[r]) { wm[q] = " [-Wunused-variable]"[r]; q = q + 1; r = r + 1; }
            wm[q] = 0;
            warn_at(tpos[symtok[k]], wm);
        } } }
        k = k + 1;
    }
    nsym = savesym; frameoff = saveoff; ntd = savetd;
    bdepth = bdepth - 1;
    while (savest < nstruct) { stdead[savest] = 1; savest = savest + 1; }
    return 0;
}

/* `typedef TYPE name, *other;` -- one entry per declarator. */
int do_typedef(void) {
    int tw; int tsz; int tsi; int tptr; int nt;
    adv();
    tw = declspec(); tsz = declsz; tsi = declstruct;
    while (1) {
        tptr = declspecptr; declpd = declspecpd;
        while (eatstar()) tptr = 1;
        /* `typedef int (*binop)(int, int);` -- a pointer to a function */
        if (cur() == tidx("(", 1)) { if (kind(tp + 1) == tidx("*", 1)) {
            nt = fpdecl();
            tdadd(nt, 8, 8, 0 - 1, 1);
            tdfp[ntd - 1] = declfp;
            if (tptr) tdfpst[ntd - 1] = tsi;      /* `struct S *(*fty)()` */
            if (eat(tidx(",", 1))) continue;
            break;
        } }
        nt = adv();
        if (cur() == tidx("(", 1)) {          /* a function-pointer typedef */
            while (cur() != tidx(";", 1)) { if (cur() == T_EOF) break; adv(); }
            tdadd(nt, 8, 8, 0 - 1, 1);
            break;
        }
        if (cur() == tidx("[", 1)) {
            adv(); tsz = tsz * cexpr(); need(tidx("]", 1), "]");
        }
        if (tptr) { tdadd(nt, tw, 8, tsi, 1); tdpd[ntd - 1] = declpd > 0 ? declpd : 1; }
        else tdadd(nt, tw, tsz, tsi, 0);
        if (eat(tidx(",", 1)) == 0) break;
    }
    need(tidx(";", 1), ";");
    return 0;
}

/* ---- aggregate initialisers -----------------------------------------
   `int a[] = {1,2,3}`, `char s[] = "hi"`, `struct P p = {1,2}`, for locals
   and for globals alike -- a global's stores go into `__init`, which is why
   both cases are one function with a different way of naming the base. */
/* Brace elision (C99 6.7.8p17): an aggregate element takes its SHARE of a
   flat list, so `PT a[] = {1,2,3, 4,5,6}` is two PTs, not six.  The list is
   therefore walked as a sequence of SCALAR SLOTS -- element index times the
   element's size, plus the member's offset, plus the sub-index when the
   member is itself an array. */
int slotoff; int slotw;

int structslots(int sst) {
    int per; int mi; int e;
    per = 0; mi = stfirst[sst]; e = stfirst[sst] + stcount[sst];
    while (mi < e) {
        if (mbskip[mi]) { mi = mi + 1; continue; }
        if (mbstruct[mi] >= 0) {
            /* a struct member takes its own scalars' share, per element */
            per = per + (mbbytes[mi] / stsize[mbstruct[mi]]) * structslots(mbstruct[mi]);
        } else { if (mbwidth[mi] == 0) {
            if (mbelem[mi] > 0) per = per + mbbytes[mi] / mbelem[mi];
            else per = per + 1;
        } else per = per + 1; }
        mi = mi + 1;
    }
    if (per <= 0) per = 1;
    return per;
}

int slotat(int i, int w, int sst) {
    int per; int el; int k; int mi; int e; int cnt; int sub;
    slotflt = initflt;
    if (sst < 0) { slotoff = i * w; slotw = w; return 0; }
    per = structslots(sst);
    el = i / per; k = i - el * per;
    mi = stfirst[sst]; e = stfirst[sst] + stcount[sst]; cnt = 0;
    while (mi < e) {
        if (mbskip[mi]) { mi = mi + 1; continue; }
        if (mbstruct[mi] >= 0) {
            sub = (mbbytes[mi] / stsize[mbstruct[mi]]) * structslots(mbstruct[mi]);
            if (k < cnt + sub) {
                int base; base = el * stsize[sst] + mboff[mi];
                slotat(k - cnt, 0, mbstruct[mi]);      /* recurse into it */
                slotoff = base + slotoff;
                return 0;
            }
            cnt = cnt + sub;
            mi = mi + 1;
            continue;
        }
        if (mbwidth[mi] == 0) {
            sub = 1;
            if (mbelem[mi] > 0) sub = mbbytes[mi] / mbelem[mi];
            if (k < cnt + sub) {
                slotoff = el * stsize[sst] + mboff[mi] + (k - cnt) * mbelem[mi];
                slotflt = mbflt[mi];
                slotw = mbelem[mi];
                if (slotw == 0) slotw = 8;
                return 0;
            }
            cnt = cnt + sub;
        } else {
            if (k == cnt) {
                slotoff = el * stsize[sst] + mboff[mi];
                slotflt = mbflt[mi];
                if (mbptr[mi]) slotflt = 0;
                slotw = mbwidth[mi];
                return 0;
            }
            cnt = cnt + 1;
        }
        mi = mi + 1;
    }
    slotoff = el * stsize[sst]; slotw = 8;
    return 0;
}

int initaddr(int isglobal, int gt, int off, int delta) {
    if (isglobal) {
        /* 2: an unnamed static object -- a compound literal at file scope */
        if (isglobal == 2) { es("  @mem.lea r1, __cl"); en(gt); }
        else { if (isglobal == 3) { es("  @mem.lea r1, ls"); en(gt); }   /* a static local */
               else { es("  @mem.lea r1, g_"); etok(gt); } }
        if (delta) { es("\n  @lit.imm r2, "); en(delta); es("\n  @alu.add r1, r1, r2"); }
        ec(10);
    } else {
        es("  @lit.imm r2, "); en(off - delta); es("\n  @alu.sub r1, r6, r2\n");
    }
    return 0;
}

/* How many elements the initialiser supplies, for an unsized `[]`.  The
   cursor is on the `]`. */
int initcount(void) {
    int j; int depth; int n; int k; int c; int pos;
    char buf[4096];
    j = tp;
    while (j < ntok) { if (kind(j) == tidx("=", 1)) break; j = j + 1; }
    j = j + 1;
    if (iswide(j)) return wdecode(j, wcp) + 1;
    if (kind(j) == T_STR) { return decode(j, buf) + 1; }
    return initcountat(j);
}

/* The same, for the braced list that starts at token j. */
int initcountat(int j) {
    int depth; int n; int k; int c; int pos;
    if (kind(j) != tidx("{", 1)) return 1;
    depth = 0; n = 0; k = 0; pos = 0;
    while (j < ntok) {
        c = kind(j);
        if (c == tidx("{", 1)) { depth = depth + 1; j = j + 1; continue; }
        if (c == tidx("}", 1)) {
            depth = depth - 1;
            if (depth == 0) { if (k) pos = pos + 1; break; }
            j = j + 1; continue;
        }
        if (c == tidx(",", 1)) {
            if (k) { pos = pos + 1; k = 0; }
            if (pos > n) n = pos;
            j = j + 1; continue;
        }
        /* `[k] =`: the next element is element k (C99 6.7.8p6) */
        if (depth == 1) { if (c == tidx("[", 1)) { if (kind(j + 1) == T_NUM) {
            pos = numval(j + 1); j = j + 4; continue;
        } } }
        k = 1; j = j + 1;
    }
    if (pos > n) n = pos;
    return n;
}

/* ---- wide string literals ---------------------------------------------
   `L"..."`: the source is read a byte at a time, so the literal is still
   UTF-8 here; it decodes to code points, and each element of the array is
   four bytes -- wchar_t is four bytes on every target, so one tape still
   lowers to six.  Adjacent literals are one token with seams, as for
   narrow ones.  Returns the number of code points written to cp. */
int iswide(int t) {
    if (kind(t) != T_STR) return 0;
    if ((src[tpos[t]] & 255) == 76) return 1;             /* L */
    return 0;
}
int strw(int t) { if (iswide(t)) return 4; return 1; }
int hexv(int c) {
    if (c >= 97) return c - 87;
    if (c >= 65) return c - 55;
    return c - 48;
}
int wdecode(int t, int *cp) {
    int k; int e; int n; int c; int m; int need;
    k = 0; e = tlen[t]; n = 0;
    while (k < e) {
        /* to the next opening quote: past `L`, blanks and a seam */
        while (k < e) { if ((src[tpos[t] + k] & 255) == 34) break; k = k + 1; }
        k = k + 1;
        while (k < e) {
            c = src[tpos[t] + k] & 255;
            if (c == 34) { k = k + 1; break; }
            if (c == 92) {
                k = k + 1; c = src[tpos[t] + k] & 255;
                if (c == 110) c = 10;
                else { if (c == 116) c = 9;
                else { if (c == 114) c = 13;
                else { if (c == 48) { c = 0; }
                else { if (c == 120) {
                    c = 0; k = k + 1;
                    while (k < e) {
                        m = src[tpos[t] + k] & 255;
                        if (isdi(m) == 0) { if (m < 65 || m > 102) break; if (m > 70) { if (m < 97) break; } }
                        c = c * 16 + hexv(m); k = k + 1;
                    }
                    k = k - 1;
                } } } } }
                cp[n] = c; n = n + 1; k = k + 1;
                continue;
            }
            /* UTF-8: the lead byte says how many continuation bytes follow */
            need = 0;
            if (c >= 240) { need = 3; c = c & 7; }
            else { if (c >= 224) { need = 2; c = c & 15; }
            else { if (c >= 192) { need = 1; c = c & 31; } } }
            k = k + 1;
            while (need > 0) { c = c * 64 + (src[tpos[t] + k] & 63); k = k + 1; need = need - 1; }
            cp[n] = c; n = n + 1;
        }
    }
    return n;
}

int initstr(int isglobal, int gt, int off, int cap) {
    int t; int n; int k;
    char buf[4096];
    if (iswide(tp)) {
        /* four bytes an element; cap counts ELEMENTS */
        t = adv();
        n = wdecode(t, wcp);
        if (cap > 0) {
            initaddr(isglobal, gt, off, 0);
            es("  @mem.zero r1, 0, "); en(cap * 4); ec(10);
        }
        k = 0;
        while (k < n) {
            if (k >= cap) break;
            es("  @lit.imm r0, "); en(wcp[k]); ec(10);
            initaddr(isglobal, gt, off, k * 4);
            estore(4);
            k = k + 1;
        }
        return 0;
    }
    t = adv();
    n = decode(t, buf);
    if (cap > 0) {
        initaddr(isglobal, gt, off, 0);
        es("  @mem.zero r1, 0, "); en(cap); ec(10);
    }
    k = 0;
    while (k <= n) {
        int c;
        if (k >= cap) break;
        /* `decode` fills buf[0..n) and does not terminate it; the NUL is
           part of the object, so write it explicitly. */
        c = 0;
        if (k < n) c = buf[k] & 255;
        es("  @lit.imm r0, "); en(c); ec(10);
        initaddr(isglobal, gt, off, k);
        estore(1);
        k = k + 1;
    }
    return 0;
}

/* Braces are FLATTENED: C lets an aggregate element take its share of a flat
   list, and this subset has no multi-dimensional declarator to tell the
   shapes apart anyway. */
/* `(T){...}` -- or `((T){...})` -- in initialiser position is the braced
   list itself (C99 6.5.2.5).  On a match the cursor is left on the `{` and
   the number of extra `(` to close afterwards is returned; else -1. */
int cplit(void) {
    int j; int k; int depth;
    j = tp; k = 0;
    while (kind(j) == tidx("(", 1)) { j = j + 1; k = k + 1; }
    if (k == 0) return 0 - 1;
    if (is_typeat(j) == 0) return 0 - 1;
    depth = 0;
    while (j < ntok) {
        if (kind(j) == tidx("(", 1)) depth = depth + 1;
        if (kind(j) == tidx(")", 1)) { if (depth == 0) break; depth = depth - 1; }
        j = j + 1;
    }
    if (kind(j + 1) != tidx("{", 1)) return 0 - 1;
    tp = j + 1;
    return k - 1;
}

/* The slots one member takes in the flat list, and where a member starts. */
int mbslots(int mi) {
    if (mbskip[mi]) return 0;
    if (mbstruct[mi] >= 0) return (mbbytes[mi] / stsize[mbstruct[mi]]) * structslots(mbstruct[mi]);
    if (mbwidth[mi] == 0) { if (mbelem[mi] > 0) return mbbytes[mi] / mbelem[mi]; return 1; }
    return 1;
}
int membstart(int sst, int mt) {
    int mi; int e; int n;
    mi = stfirst[sst]; e = mi + stcount[sst]; n = 0;
    while (mi < e) { if (mi == mt) return n; n = n + mbslots(mi); mi = mi + 1; }
    return n;
}
/* the member of sst holding relative slot rel; its first slot in *ms */
int memberat(int sst, int rel, int *ms) {
    int mi; int e; int n; int c;
    mi = stfirst[sst]; e = mi + stcount[sst]; n = 0;
    while (mi < e) {
        c = mbslots(mi);
        if (rel < n + c) { *ms = n; return mi; }
        n = n + c; mi = mi + 1;
    }
    *ms = n;
    return 0 - 1;
}

/* Set by a caller right before initaggr: the object is an ARRAY (an array
   of one struct is the same size as the struct, so the size cannot tell),
   and its rows are that many elements long for `a[n][m]`. */

/* An initialiser list, walked as SCALAR SLOTS with a context per brace
   level: which sub-aggregate the level is (its first slot, how many slots,
   and a struct or an array's element share).  A designator resolves in its
   level's context, and a closing brace moves the cursor to the END of its
   sub-aggregate -- C99 6.7.8p20: `{1, {4}, 9}` puts the 9 in the member after
   the braced one, whatever the braced list left out. */
int initaggr(int isglobal, int gt, int off, int w, int sst, int nbytes) {
    int i; int depth; int delta; int ew; int isarr; int rows; int per; int rows3;
    int myflt; int sk;
    int cxbase[32]; int cxslots[32]; int cxst[32]; int cxel[32]; int cxelst[32]; int cxrow[32];
    int cxsub[32];
    isarr = initisarr; rows = initrows; rows3 = initrows3;
    myflt = initflt;          /* nested literals set it for themselves */
    initisarr = 0; initrows = 0; initrows3 = 0;
    /* C99 6.7.8p21: what the initialiser does not mention is ZERO.  Clearing
       the object first is the whole of that rule, and the tape has an op for
       it -- element-wise stores would also have to know which elements were
       skipped. */
    if (nbytes > 0) {
        initaddr(isglobal, gt, off, 0);
        es("  @mem.zero r1, 0, "); en(nbytes); ec(10);
    }
    per = 1;
    if (sst >= 0) per = structslots(sst);
    /* level 1: the object itself */
    cxbase[1] = 0; cxrow[1] = 0; cxst[1] = 0 - 1; cxel[1] = 0; cxelst[1] = 0 - 1; cxsub[1] = 0;
    if (isarr) {
        cxel[1] = per; cxelst[1] = sst;
        if (rows > 0) { cxel[1] = per * rows; cxrow[1] = rows; cxsub[1] = rows3; }
        cxslots[1] = 1000000;
    } else {
        if (sst >= 0) { cxst[1] = sst; cxslots[1] = per; }
        else { cxel[1] = 1; cxslots[1] = 1000000; }   /* a scalar, or a flat array */
    }
    i = 0; depth = 0;
    while (1) {
        if (cur() == tidx("{", 1)) {
            adv(); depth = depth + 1;
            if (depth >= 32) { printf("initialiser nested too deeply\n"); __exit(1); }
            if (depth > 1) {
                int d; int ms; int mi; int el;
                d = depth - 1;
                /* what the cursor is at, in the enclosing level */
                cxst[depth] = 0 - 1; cxel[depth] = 0; cxelst[depth] = 0 - 1; cxrow[depth] = 0;
                cxsub[depth] = 0;
                cxbase[depth] = i; cxslots[depth] = 1;          /* a braced scalar */
                if (cxst[d] >= 0) {
                    mi = memberat(cxst[d], i - cxbase[d], &ms);
                    if (mi >= 0) {
                        cxbase[depth] = cxbase[d] + ms; cxslots[depth] = mbslots(mi);
                        if (mbstruct[mi] >= 0) {
                            if (mbbytes[mi] == stsize[mbstruct[mi]]) cxst[depth] = mbstruct[mi];
                            else { cxel[depth] = structslots(mbstruct[mi]); cxelst[depth] = mbstruct[mi]; }
                        } else { if (mbslots(mi) > 1) cxel[depth] = 1;
                                 else { cxbase[depth] = i; cxslots[depth] = 1; } }
                    }
                } else { if (cxel[d] > 0) {
                    el = (i - cxbase[d]) / cxel[d];
                    cxbase[depth] = cxbase[d] + el * cxel[d];
                    cxslots[depth] = cxel[d];
                    if (cxrow[d] > 0) {                  /* a row of a[n][m] */
                        cxel[depth] = cxel[d] / cxrow[d]; cxelst[depth] = cxelst[d];
                        if (cxsub[d] > 0) {              /* ...whose elements are rows */
                            cxel[depth] = (cxel[d] / cxrow[d]) * cxsub[d];
                            cxrow[depth] = cxsub[d];
                        }
                    } else { if (cxelst[d] >= 0) cxst[depth] = cxelst[d];
                             else { cxbase[depth] = i; cxslots[depth] = 1; } }
                } }
                i = cxbase[depth];
            }
            continue;
        }
        if (cur() == tidx("}", 1)) {
            adv();
            if (depth <= 1) break;
            i = cxbase[depth] + cxslots[depth];
            depth = depth - 1;
            continue;
        }
        if (cur() == tidx(",", 1)) { adv(); continue; }
        if (cur() == T_EOF) break;
        /* C99 6.7.8p17: a designator moves the cursor within ITS level, and
           the elements after it continue from there */
        if (cur() == tidx("[", 1)) {
            int k;
            adv(); k = cexpr(); need(tidx("]", 1), "]"); need(tidx("=", 1), "=");
            if (cxel[depth] > 0) i = cxbase[depth] + k * cxel[depth];
            continue;
        }
        if (cur() == tidx(".", 1)) { if (cxst[depth] >= 0) {
            int mi;
            adv();
            mi = mbfind(cxst[depth], tp);
            if (mi < 0) err_tok(tp, "no such member");
            adv(); need(tidx("=", 1), "=");
            i = cxbase[depth] + membstart(cxst[depth], mi);
            continue;
        } }
        initflt = myflt;
        slotat(i, w, sst);
        delta = slotoff; ew = slotw; sk = slotflt;
        expr(); loadval();
        fconv(fkind(), sk);                      /* to the slot's type */
        initaddr(isglobal, gt, off, delta);
        estore(ew);
        i = i + 1;
    }
    return 0;
}

/* `(*name)(params)` -- a pointer to a function.  The cursor is on the `(`;
   returns the name's token and leaves declfp saying which calling
   convention the pointee uses, because an indirect call has to know it. */
int skipparen(void) {                 /* over a balanced (...) at the cursor */
    int depth; int var;
    depth = 0; var = 0;
    while (cur() != T_EOF) {
        if (cur() == tidx("(", 1)) depth = depth + 1;
        if (cur() == tidx(")", 1)) { depth = depth - 1; if (depth == 0) { adv(); break; } }
        if (cur() == tidx("...", 3)) var = 1;
        adv();
    }
    return var;
}
int fpdecl(void) {
    int t; int var;
    adv();
    while (eatstar()) { }
    /* the name may be absent: `int (*[4])(int)` as a parameter type */
    t = 0 - 1;
    if (cur() == tidx("(", 1)) { if (kind(tp + 1) == tidx("*", 1)) {
        /* `(* (*p)(int a, int b))(int c, int d)`: the declarator nests.  The
           inner one is p, a pointer to a function; this level's suffix says
           what THAT function returns -- another function pointer. */
        t = fpdecl();
        need(tidx(")", 1), ")");
        if (cur() == tidx("(", 1)) { skipparen(); fpretfp = 1; }
        return t;
    } }
    if (cur() == T_ID) t = adv();
    fpdim = 0; fpadim = 0; fpfn = 0 - 1; fpretfp = 0;
    /* `(*pick(int which))(int, int)`: pick takes (int which) and RETURNS
       the pointer -- the declarator nests, and the inner list is pick's */
    if (cur() == tidx("(", 1)) { fpfn = tp; skipparen(); }
    while (cur() == tidx("[", 1)) {             /* an array of pointers */
        adv(); fpdim = 1;
        if (cur() != tidx("]", 1)) fpdim = cexpr();
        need(tidx("]", 1), "]");
    }
    need(tidx(")", 1), ")");
    declfp = 0;
    if (cur() == tidx("(", 1)) {
        var = skipparen();
        declfp = 1;
        if (var) declfp = 2;
    } else { if (cur() == tidx("[", 1)) {
        /* `(*p)[4]`: a pointer to arrays of 4 -- p[1] strides a whole row */
        adv(); fpadim = cexpr(); need(tidx("]", 1), "]");
    } }
    /* `(*const x)` is just a parenthesised pointer: declfp stays 0 */
    return t;
}

int lfp; int lfpret; int lflt0; int gflt0;
/* the conversion kind of the object being declared */
int dkind(int flt) {
    if (declbool) { if (declptr == 0) return 9; }   /* the _Bool conversion */
    if (declptr) return 1;
    if (flt) return flt;
    if (declunsigned) { if (declsz == 8) return 1; }
    return 0;
}
int local_decl(void) {
    int w; int t; int off; int n; int nelem; int sst; int isarr; int apd; int lstat;
    if (cur() == tidx("typedef", 7)) return do_typedef();
    w = declspec();
    sst = declstruct;
    lflt0 = declflt;          /* an initialiser's casts would overwrite it */
    lstat = declstatic;
    if (cur() == tidx(";", 1)) { adv(); return 0; }  /* `struct X { ... };` */
    while (1) {
        declstruct = sst;
        decldim2 = 0; decldim3 = 0; declfp = declspecfp;
        declptr = declspecptr; declpd = declspecpd;
        declflt = lflt0;
        while (eatstar()) { declptr = 1; }
        if (cur() == tidx("(", 1)) { if (kind(tp + 1) == tidx("*", 1)) {
            t = fpdecl(); declptr = 1; lfpret = fpretfp; sst = 0 - 1; declstruct = 0 - 1;
            if (fpdim > 0) {
                /* `int (*fs[2])(int, int)`: an array of pointers */
                lbind = scopebind("local", 5, t);
                off = alloc_local(fpdim * 8);
                declbytes = fpdim * 8; declfp = 0;
                sadd(t, lbind, off, 8);
                symkind[nsym - 1] = 3;
                if (eat(tidx("=", 1))) { initisarr = 1; initaggr(0, 0, off, 8, 0 - 1, fpdim * 8); }
                if (eat(tidx(",", 1))) continue;
                break;
            }
            if (fpadim > 0) { decldim2 = fpadim; declptr = 1; }
        } else t = adv(); }
        else t = adv();
        /* `int f(char *);` in a block: a prototype, not an object.  Calls
           resolve by name, so there is nothing to allocate. */
        if (cur() == tidx("(", 1)) {
            int depth; depth = 0;
            while (cur() != T_EOF) {
                if (cur() == tidx("(", 1)) depth = depth + 1;
                if (cur() == tidx(")", 1)) { depth = depth - 1; if (depth == 0) { adv(); break; } }
                adv();
            }
            if (eat(tidx(",", 1))) continue;
            need(tidx(";", 1), ";");
            return 0;
        }
        lbind = scopebind("local", 5, t);
        n = 1; isarr = 0;
        lfp = declfp;
        if (lstat) {
            int ew; long nb; int lk2;
            if (cur() == tidx("[", 1)) {
                adv(); isarr = 1;
                if (cur() == tidx("]", 1)) {
                    n = initcount();
                    if (sst >= 0) { int per; per = structslots(sst); n = (n + per - 1) / per; }
                } else n = cexpr();
                need(tidx("]", 1), "]");
            }
            ew = w;
            if (sst >= 0) ew = declsz;
            if (declptr) { if (isarr) { if (declpd > 0) ew = 8; } else ew = 8; }
            nb = n * ew;
            if (declptr == 0) { if (sst >= 0) { if (isarr == 0) nb = declsz; } }
            es(".bss ls"); en(t); ec(32); en(nb < 8 ? 8 : nb); ec(10);
            declbytes = nb;
            sadd(t, 0, 0, isarr ? ew : w);
            symlab[nsym - 1] = t;
            if (isarr) { symkind[nsym - 1] = 5; symptr[nsym - 1] = 1; symptrd[nsym - 1] = declpd + 1; }
            else { if (declptr == 0) { if (sst >= 0) {
                symkind[nsym - 1] = 5; symptr[nsym - 1] = 1; symelem[nsym - 1] = declsz; } } }
            if (eat(tidx("=", 1))) {
                /* once, at program start: into __init, like a global's */
                lk2 = dkind(lflt0);
                initflt = lflt0;
                toinit = 1; hasinit = 1;
                if (cur() == tidx("{", 1)) {
                    if (isarr) { initisarr = 1; }
                    initaggr(3, t, 0, isarr ? ew : w, sst, nb);
                } else { if (cur() == T_STR) { if (isarr) { initstr(3, t, 0, n); }
                    else { expr(); loadval(); es("  @mem.lea r1, ls"); en(t); ec(10); estore(8); } }
                else { int sptr; sptr = declptr;   /* before the RHS */
                    expr(); loadval(); fconv(fkind(), lk2);
                    es("  @mem.lea r1, ls"); en(t); ec(10);
                    if (sptr) estore(8); else estore(w); } }
                toinit = 0;
            }
            if (eat(tidx(",", 1))) continue;
            break;
        }
        if (cur() == vfind(TOKV, NTOKV, "[", 1)) { if (isconstdim(tp + 1) == 0) {
            /* A variable-length array (C99 6.7.5.2).  Its size is known only
               now, so its storage comes off the tape stack here; the name is
               a pointer to it, and a second slot keeps its byte count for
               `sizeof`.  The first one in a block saves the stack pointer,
               and the block's end -- or a break/continue out of it -- puts
               it back, or a loop would eat the stack. */
            int el; int szs;
            adv(); expr(); loadval(); need(tidx("]", 1), "]");
            el = w;
            if (sst >= 0) el = declsz;
            if (declptr) el = 8;
            es("  @lit.imm r2, "); en(el); es("\n  @alu.mul r0, r0, r2\n");
            szs = alloc_local(8);
            es("  @mem.store [r6-"); en(szs); es("], r0\n");
            if (vlaslot[bdepth] == 0) {
                vlaslot[bdepth] = alloc_local(8);
                es("  @mem.store [r6-"); en(vlaslot[bdepth]); es("], r7\n");
            }
            es("  @lit.imm r2, 7\n  @alu.add r0, r0, r2\n  @lit.imm r2, -8\n"
               "  @alu.and r0, r0, r2\n  @alu.sub r7, r7, r0\n");
            off = alloc_local(8);
            es("  @mem.store [r6-"); en(off); es("], r7\n");
            declptr = 1; declbytes = 8;
            sadd(t, lbind, off, el);
            symvla[nsym - 1] = szs;
            if (eat(tidx(",", 1))) continue;
            break;
        } }
        if (cur() == vfind(TOKV, NTOKV, "[", 1)) {
            adv();
            isarr = 1;
            /* `int a[] = {1,2,3}` -- the initialiser says how long it is,
               and for an array of structs it says how many SCALARS */
            if (cur() == vfind(TOKV, NTOKV, "]", 1)) {
                n = initcount();
                if (sst >= 0) {
                    int per; per = structslots(sst);
                    n = (n + per - 1) / per;
                }
            }
            else n = cexpr();
            need(vfind(TOKV, NTOKV, "]", 1), "]");
            /* `a[n][m]` is n*m elements in a row; the FIRST index strides a
               whole row, which is what decldim2 records */
            decldim2 = 0; decldim3 = 0;
            if (cur() == vfind(TOKV, NTOKV, "[", 1)) {
                adv();
                decldim2 = cexpr();
                need(vfind(TOKV, NTOKV, "]", 1), "]");
                n = n * decldim2;
                dim3decl();
                n = n * dim3n;
            }
            if (sst >= 0) w = declsz;
            apd = declpd;
            if (apd > 0) w = 8;            /* an array of POINTERS: 8 each */
            off = alloc_local(n * w);
            declptr = 1;
            declbytes = n * declsz;
            if (apd > 0) declbytes = n * 8;
            sadd(t, lbind, off, w);
            symkind[nsym - 1] = 3;         /* an array name denotes its address */
            symptrd[nsym - 1] = apd + 1;
        } else {
            /* a struct variable needs its whole body, not one slot, and its
               NAME denotes its address the way an array's does */
            if (declptr == 0) { if (sst >= 0) {
                off = alloc_local(declsz);
                declbytes = declsz;
                sadd(t, lbind, off, w);
                symkind[nsym - 1] = 3;
                if (eat(tidx("=", 1))) {
                    int cpn; cpn = 0 - 1;
                    if (cur() == tidx("(", 1)) cpn = cplit();
                    if (cpn >= 0 || cur() == tidx("{", 1)) {
                        initaggr(0, 0, off, w, sst, declsz);
                        while (cpn > 0) { need(tidx(")", 1), ")"); cpn = cpn - 1; }
                    } else {
                        /* `struct S b = a;` -- a whole-struct copy */
                        es("  @lit.imm r0, "); en(off); es("\n  @alu.sub r0, r6, r0\n");
                        push(); expr(); loadval();
                        es("  mov r1, r0\n  @mem.load r0, [r7+0]\n  @call.frame -8\n");
                        scopy(declsz);
                    }
                }
                if (eat(tidx(",", 1))) continue;
                break;
            } }
            off = alloc_local(8);
            declbytes = declsz;
            if (declptr) declbytes = 8;
            sadd(t, lbind, off, w);
            if (lfp) { symfpret[nsym - 1] = lfpret; symcst[nsym - 1] = declspecfpst; }
            lfpret = 0;
        }
        if (eat(vfind(TOKV, NTOKV, "=", 1))) {
            int lk; int lptr;
            lk = dkind(lflt0);
            /* Whether the thing being initialised is a POINTER, taken now:
               the initialiser can contain a type name of its own -- a
               compound literal `(int[]){5,6,7}`, a cast, a sizeof -- and
               parsing it rewrites the global `declptr`.  `int *a =
               (int[]){...}` stored the pointer 4 bytes wide, the element
               width, and read it back 8: the top half was whatever the
               stack held.  It passed here by luck and printed garbage on
               a GitHub runner. */
            lptr = declptr;
            initflt = lflt0;
            if (cur() == tidx("{", 1)) {
                if (isarr) { initisarr = 1; initrows = decldim2; initrows3 = decldim3; }
                initaggr(0, 0, off, w, sst, n * w);
            }
            else { if (cur() == T_STR) { if (isarr) { if (w == strw(tp)) {
                initstr(0, 0, off, n);
            } else { expr(); loadval();
                es("  @lit.imm r2, "); en(off); es("\n  @alu.sub r1, r6, r2\n");
                estore(8); } }
            else { int rt; rt = tp; curcall = 0; expr(); loadval();
                intptr_check(lptr, rt);
                fconv(fkind(), lk);                  /* C99 6.7.8p11 */
                es("  @lit.imm r2, "); en(off); es("\n  @alu.sub r1, r6, r2\n");
                if (lptr) estore(8); else estore(w); } }
            else { int rt; rt = tp; curcall = 0; expr(); loadval();
                intptr_check(lptr, rt);
                fconv(fkind(), lk);                  /* C99 6.7.8p11 */
                es("  @lit.imm r2, "); en(off); es("\n  @alu.sub r1, r6, r2\n");
                if (lptr) estore(8); else estore(w); } }
        }
        if (eat(vfind(TOKV, NTOKV, ",", 1)) == 0) break;
    }
    need(vfind(TOKV, NTOKV, ";", 1), ";");
    return 0;
}

/* -Wreturn-type without a flow graph: after each statement, `laststmt`
   says whether control can fall out of its end.  A return, an endless
   loop and a call that does not come back cannot; a block takes its last
   statement's answer; `if` with `else` takes both branches'; everything
   else can (a switch whose every case returns is judged pessimistically,
   and the corpus check in tests/warn.sh is what bounds the damage). */
int stmt_(void);
int stmt(void) {
    int inf; int r;
    inf = 0;
    if (cur() == tidx("return", 6)) inf = 1;
    if (cur() == tidx("while", 5)) { if (kind(tp + 2) == T_NUM) { if (kind(tp + 3) == tidx(")", 1)) {
        if (numval(tp + 2) != 0) inf = 1; } } }
    if (cur() == tidx("for", 3)) { if (kind(tp + 2) == tidx(";", 1)) { if (kind(tp + 3) == tidx(";", 1)) inf = 1; } }
    if (kind(tp) == T_ID) {
        if (isname(tp, "exit", 4) || isname(tp, "abort", 5) || isname(tp, "__exit", 6) || isname(tp, "_exit", 5)) inf = 1;
    }
    laststmt = 0;
    r = stmt_();
    if (inf) laststmt = 1;
    return r;
}
int stmt_(void) {
    int p; int a; int b; int c; int top;
    /* C99 6.8.1: a label prefixes a STATEMENT.  It is spotted before the
       table is asked, because `name :` is not a production -- the grammar
       sees an identifier and would call it an expression. */
    if (kind(tp) == T_ID) {
        if (kind(tp + 1) == tidx(":", 1)) {
            int lt;
            lt = adv(); adv();
            es("u_"); etok(lt); es(":\n");
            if (cur() != tidx("}", 1)) stmt();
            return 0;
        }
    }
    p = ask(1);
    if (p == P_BLOCK) return block();
    if (p == P_DECL) return local_decl();
    if (p == P_IF) {
        adv(); need(vfind(TOKV, NTOKV, "(", 1), "(");
        expr(); loadval(); ftruthy(); need(vfind(TOKV, NTOKV, ")", 1), ")");
        a = newlab();
        elab("  @ctrl.jumpz r0, L", a); ec(10);
        stmt();
        c = laststmt;                            /* the then-branch's answer */
        if (cur() == vfind(TOKV, NTOKV, "else", 4)) {
            b = newlab();
            elab("  @ctrl.jump L", b); ec(10);
            elab("L", a); es(":\n");
            adv(); stmt();
            elab("L", b); es(":\n");
            laststmt = c && laststmt;
        } else { elab("L", a); es(":\n"); laststmt = 0; }
        return 0;
    }
    if (p == P_WHILE) {
        adv();
        top = newlab(); a = newlab();
        elab("L", top); es(":\n");
        need(vfind(TOKV, NTOKV, "(", 1), "(");
        expr(); loadval(); ftruthy(); need(vfind(TOKV, NTOKV, ")", 1), ")");
        elab("  @ctrl.jumpz r0, L", a); ec(10);
        brkstack[nloop] = a; cntstack[nloop] = top;
        brkdep[nloop] = bdepth; cntdep[nloop] = bdepth; nloop = nloop + 1;
        stmt();
        nloop = nloop - 1;
        elab("  @ctrl.jump L", top); ec(10);
        elab("L", a); es(":\n");
        return 0;
    }
    if (p == P_FOR) {
        int stept; int bodyt; int aftert;
        adv(); need(tidx("(", 1), "(");
        if (eat(tidx(";", 1)) == 0) {
            if (is_typetok()) local_decl();
            else { exprc(); need(tidx(";", 1), ";"); }
        }
        top = newlab(); a = newlab(); c = newlab();
        elab("L", top); es(":\n");
        if (cur() != tidx(";", 1)) {
            expr(); loadval(); ftruthy();
            elab("  @ctrl.jumpz r0, L", a); ec(10);
        }
        need(tidx(";", 1), ";");
        stept = tp;
        b = 0;
        while (1) {                                  /* skip the step clause */
            if (cur() == T_EOF) break;               /* parked there by an error */
            if (cur() == tidx("(", 1)) b = b + 1;
            if (cur() == tidx(")", 1)) { if (b == 0) break; b = b - 1; }
            adv();
        }
        adv();
        bodyt = tp;
        brkstack[nloop] = a; cntstack[nloop] = c;
        brkdep[nloop] = bdepth; cntdep[nloop] = bdepth; nloop = nloop + 1;
        stmt();
        nloop = nloop - 1;
        aftert = tp;
        elab("L", c); es(":\n");
        /* not after an error: the cursor is parked at EOF and stays there */
        if (panic == 0) { if (stept != bodyt - 1) { tp = stept; exprc(); } }
        tp = aftert;
        elab("  @ctrl.jump L", top); ec(10);
        elab("L", a); es(":\n");
        return 0;
    }
    if (p == P_SWITCH) {
        int slot; int disp; int end; int k; int cbase; int mysw;
        adv(); need(tidx("(", 1), "(");
        expr(); loadval();
        need(tidx(")", 1), ")");
        slot = alloc_local(8);
        es("  @lit.imm r2, "); en(slot);
        es("\n  @alu.sub r1, r6, r2\n  @mem.store [r1+0], r0\n");
        disp = newlab(); end = newlab();
        elab("  @ctrl.jump L", disp); ec(10);
        cbase = nswv; mysw = nsw;
        swdef[mysw] = 0 - 1; swslot[mysw] = slot;
        nsw = nsw + 1;
        /* C99 6.8.6.2p1: `continue` belongs to the enclosing ITERATION
           statement.  A switch takes over `break` and nothing else. */
        brkstack[nloop] = end; brkdep[nloop] = bdepth;
        if (nloop > 0) { cntstack[nloop] = cntstack[nloop - 1]; cntdep[nloop] = cntdep[nloop - 1]; }
        else { cntstack[nloop] = end; cntdep[nloop] = bdepth; }
        nloop = nloop + 1;
        stmt();
        nloop = nloop - 1;
        nsw = nsw - 1;
        elab("  @ctrl.jump L", end); ec(10);
        elab("L", disp); es(":\n");
        k = cbase;
        while (k < nswv) {
            es("  @lit.imm r2, "); en(slot);
            es("\n  @alu.sub r1, r6, r2\n  @mem.load r0, [r1+0]\n");
            es("  @lit.imm r1, "); en(swval[k]); ec(10);
            es("  @alu.ne r0, r0, r1\n");
            elab("  @ctrl.jumpz r0, L", swlab[k]); ec(10);
            k = k + 1;
        }
        nswv = cbase;
        if (swdef[mysw] >= 0) { elab("  @ctrl.jump L", swdef[mysw]); ec(10); }
        else { elab("  @ctrl.jump L", end); ec(10); }
        elab("L", end); es(":\n");
        return 0;
    }
    if (p == P_CASE) {
        int v; int lab;
        adv(); v = cexpr(); need(tidx(":", 1), ":");
        lab = newlab();
        swval[nswv] = v; swlab[nswv] = lab; nswv = nswv + 1;
        elab("L", lab); es(":\n");
        /* C99 6.8.1: a label prefixes a STATEMENT -- but `case 1: }` is
           common enough in the wild to tolerate. */
        if (cur() != tidx("}", 1)) stmt();
        return 0;
    }
    if (p == P_DEFAULT) {
        int lab;
        adv(); need(tidx(":", 1), ":");
        lab = newlab();
        swdef[nsw - 1] = lab;
        elab("L", lab); es(":\n");
        if (cur() != tidx("}", 1)) stmt();
        return 0;
    }
    if (p == P_RETURN) {
        adv();
        if (cur() != vfind(TOKV, NTOKV, ";", 1)) { expr(); loadval();
            if (retst < 0) {
                fconv(fkind(), retkind);
                /* a uint32_t function must not hand back 64 bits of whatever
                   the arithmetic left above its width */
                if (retkind == 0) { if (retsz < 8) { if (1) {
                    if (retuns) {
                        if (retsz == 1) es("  @lit.imm r2, 255\n  @alu.and r0, r0, r2\n");
                        if (retsz == 2) es("  @lit.imm r2, 65535\n  @alu.and r0, r0, r2\n");
                        if (retsz == 4) es("  @lit.imm r2, 4294967295\n  @alu.and r0, r0, r2\n");
                    } else {
                        es("  @call.frame 8\n  @mem.st [r7+0], r0, "); en(retsz);
                        es("\n  @mem.ld r0, [r7+0], "); en(retsz);
                        es("\n  @call.frame -8\n");
                    }
                } } }
            }
        }
        if (retst >= 0) {
            es("  mov r1, r0\n  @mem.lea r0, __rv_"); etok(rett); ec(10);
            scopy(stsize[retst]);
        }
        need(vfind(TOKV, NTOKV, ";", 1), ";");
        elab("  @ctrl.jump R", retlab); ec(10);
        return 0;
    }
    if (p == P_GOTO) {
        int gt;
        adv(); gt = adv();
        es("  @ctrl.jump u_"); etok(gt); ec(10);
        need(tidx(";", 1), ";");
        return 0;
    }
    if (p == P_BREAK) {
        adv(); need(vfind(TOKV, NTOKV, ";", 1), ";");
        vlaback(brkdep[nloop - 1]);
        elab("  @ctrl.jump L", brkstack[nloop - 1]); ec(10);
        return 0;
    }
    if (p == P_CONTINUE) {
        adv(); need(vfind(TOKV, NTOKV, ";", 1), ";");
        vlaback(cntdep[nloop - 1]);
        elab("  @ctrl.jump L", cntstack[nloop - 1]); ec(10);
        return 0;
    }
    if (p == P_DO) {
        adv();
        top = newlab(); a = newlab(); c = newlab();
        elab("L", top); es(":\n");
        brkstack[nloop] = a; cntstack[nloop] = c;
        brkdep[nloop] = bdepth; cntdep[nloop] = bdepth; nloop = nloop + 1;
        stmt();
        nloop = nloop - 1;
        elab("L", c); es(":\n");
        need(tidx("while", 5), "while");
        need(tidx("(", 1), "(");
        expr(); loadval(); ftruthy();
        need(tidx(")", 1), ")");
        need(tidx(";", 1), ";");
        elab("  @ctrl.jumpz r0, L", a); ec(10);
        elab("  @ctrl.jump L", top); ec(10);
        elab("L", a); es(":\n");
        return 0;
    }
    if (eat(vfind(TOKV, NTOKV, ";", 1))) return 0;
    exprc();
    need(vfind(TOKV, NTOKV, ";", 1), ";");
    return 0;
}


int function(int t, int w) {
    int np; int pw; int pt; int off; int fpatch; int k; int start; int fnvoid;
    int fsym; int stacked; int npar; int depth; int c; int any; int havename;
    int pst; int nsp; int spsym[16]; int pfl;
    /* `static int helper(...)` in the second unit is not the `helper` in
       the first: recorded here, before the label is emitted */
    if (declstatic) ustat_add(t);
    fnvoid = declvoid;                       /* before the parameters' types overwrite it */
    fntok = t;
    start = nout; nsp = 0; pst = 0 - 1;
    fsym = nsym - 1;
    scopewant("top", 3, tp, "fn_name", 7);  /* lparen -> fn_name */
    need(vfind(TOKV, NTOKV, "(", 1), "(");
    /* Look ahead at the whole parameter list before emitting a byte of it:
       a variadic function, or one with more parameters than there are
       argument registers, takes EVERY argument on the tape stack, arg[k] at
       [FP + 16 + 8k] [W-13] [W-15].  That has to be known before the first
       parameter is stored. */
    fnvar = 0; npar = 0; depth = 0; any = 0; k = tp;
    while (k < ntok) {
        c = kind(k);
        if (c == tidx("(", 1)) depth = depth + 1;
        if (c == tidx(")", 1)) { if (depth == 0) break; depth = depth - 1; }
        if (c == tidx("...", 3)) fnvar = 1;
        else { if (c == tidx(",", 1)) { if (depth == 0) npar = npar + 1; }
               else any = 1; }
        k = k + 1;
    }
    if (any) npar = npar + 1;
    if (fnvar) npar = npar - 1;              /* the `...` is not a parameter */
    stacked = 0;
    if (fnvar) stacked = 1;
    if (npar > 6) stacked = 1;
    fnnfixed = npar;
    if (fsym >= 0) symvar[fsym] = stacked;
    scopebase = nsym;
    frameoff = 0; framemax = 0;
    np = 0;
    etok(t); es(":\n");
    es("  @call.frame 8\n  @mem.store [r7+0], r6\n  mov r6, r7\n  @call.frame ");
    fpatch = nout; es("      "); ec(10);
    while (cur() != vfind(TOKV, NTOKV, ")", 1)) {
        if (cur() == T_EOF) break;
        if (eat(tidx("...", 3))) break;
        pw = declspec();
        pst = declstruct; pfl = declflt;
        declptr = declspecptr; declpd = declspecpd; declfp = declspecfp;
        while (eatstar()) declptr = 1;
        havename = 0;
        if (cur() == tidx("(", 1)) {
            if (kind(tp + 1) == tidx("*", 1)) {
                pt = fpdecl(); declptr = 1; pw = 8;
                if (pt >= 0) havename = 1;
                if (fpdim > 0) declfp = 0;       /* an array of them decays */
            } else {
                /* `int f1(int (), int)`: a function type, adjusted to a
                   pointer to it (C99 6.7.5.3p8) */
                skipparen(); declptr = 1; pw = 8;
            }
        }
        if (cur() == T_ID) { pt = adv(); havename = 1; }
        /* `int a[n]`, `int a[static 5]`: an array parameter IS a pointer
           (C99 6.7.5.3p7), whatever the brackets say */
        while (cur() == tidx("[", 1)) {
            while (cur() != tidx("]", 1)) { if (cur() == T_EOF) break; adv(); }
            adv(); declptr = 1;
        }
        if (havename) {
            if (scopebind("param", 5, pt) != 1) scopefail("a parameter", pt);
            off = alloc_local(8);
            /* The SLOT is eight bytes; the parameter is its declared size.
               `declbytes = 8` here made every `unsigned p` a u64 on the
               type axis, so `b * p` was never narrowed to 32 bits and
               `(b * p) / 13u` divided the whole 64-bit product -- found
               by the widened fuzz [S-15 A3], not by any written probe. */
            declbytes = declptr ? 8 : declsz;
            sadd(pt, 1, off, pw);
            if (pst >= 0) { if (declptr == 0) {
                if (nsp >= 16) { printf("more than 16 struct parameters\n"); __exit(1); }
                spsym[nsp] = nsym - 1; nsp = nsp + 1;
            } }
            if (stacked) {
                /* the registers are free in this convention */
                es("  @mem.load r1, [r6+"); en(16 + 8 * np); es("]\n");
                es("  @lit.imm r5, "); en(off); es("\n  @alu.sub r5, r6, r5\n  @mem.store [r5+0], r1\n");
            } else {
                /* Straight into the slot: the tape takes a negative
                   displacement, so no scratch register is needed.  There was
                   one -- r5, "free" -- until a function had six parameters
                   and r5 WAS the sixth, overwritten before it was stored. */
                es("  @mem.store [r6-"); en(off); es("], r"); en(np); ec(10);
            }
        }
        /* the parameter's kind, for callers to convert to */
        if (fsym >= 0) { if (np < 8) {
            int pk; pk = 0;
            if (declptr) pk = 1;
            else { if (pfl) pk = pfl; else { if (declunsigned) { if (declsz == 8) pk = 1; } } }
            sympk[fsym * 8 + np] = pk;
        } }
        np = np + 1;
        if (eat(vfind(TOKV, NTOKV, ",", 1)) == 0) break;
    }
    if (fsym >= 0) symnpk[fsym] = np;
    need(vfind(TOKV, NTOKV, ")", 1), ")");
    /* `int (*pick(int w))(int, int) {`: the body follows the OUTER list */
    if (fnresume >= 0) { tp = fnresume; fnresume = 0 - 1; }
    /* a prototype, not a definition -- `int f(int), g(int), gv;` lists
       several, and the caller carries on after a comma */
    if (cur() == tidx(";", 1) || cur() == tidx(",", 1)) {
        if (cur() == tidx(",", 1)) { nout = start; nsym = scopebase; return 2; }
        adv();
        nout = start;                     /* unemit the prologue */
        nsym = scopebase;
        return 0;
    }
    /* A struct parameter arrived as the address of the caller's copy; take
       one of our own, so writing to it does not reach the caller. */
    k = 0;
    while (k < nsp) {
        off = alloc_local(stsize[symstruct[spsym[k]]]);
        es("  @mem.load r1, [r6-"); en(symoff[spsym[k]]); es("]\n");
        es("  @lit.imm r0, "); en(off); es("\n  @alu.sub r0, r6, r0\n");
        scopy(stsize[symstruct[spsym[k]]]);
        symoff[spsym[k]] = off;
        k = k + 1;
    }
    retst = 0 - 1;
    if (fsym >= 0) { if (symstruct[fsym] >= 0) { if (symptr[fsym] == 0) retst = symstruct[fsym]; } }
    /* what `return` converts to (C99 6.8.6.4p3) */
    retkind = 0; retsz = w; retuns = 0;
    if (fsym >= 0) {
        retuns = symuns[fsym];
        if (symptr[fsym]) { retkind = 1; retsz = 8; }
        else { if (symflt[fsym]) retkind = symflt[fsym];
               else { if (retuns) { if (w == 8) retkind = 1; } } }
    }
    rett = t;
    retlab = newlab();
    infunc = 1;
    block();
    infunc = 0;
    /* -Wreturn-type, the way it can be judged without a flow graph: the
       body's last top-level statement is a return, an endless loop, or a
       call that does not come back.  main is exempt (C99 5.1.2.2.3). */
    if (fnvoid == 0) { if (declptr == 0 || 1) { if (retst < 0) { if (laststmt == 0) { if (panic == 0) {
        if (isname(t, "main", 4) == 0)
            warn_at(tpos[tp - 1], "non-void function does not return a value in all control paths [-Wreturn-type]");
    } } } } }
    elab("R", retlab); es(":\n");
    es("  mov r7, r6\n  @mem.load r6, [r7+0]\n  @call.frame -8\n  @ctrl.ret\n");
    patchnum(fpatch, 6, (framemax + 7) / 8 * 8);
    if (retst >= 0) { es(".bss __rv_"); etok(t); ec(32); en(stsize[retst]); ec(10); }
    nsym = scopebase;
    return 0;
}

int unit(void) {
    int p; int w; int t; int n; int k; int isarr; int gstruct; int cpn; int gfpfn; int gk; int gpd; int gfpd;
    while (1) {
        if (panic) {
            /* the walker unwound to EOF after an error: drop the broken
               construct's locals, and walk on from the next one */
            panic = 0;
            if (infunc) { nsym = scopebase; infunc = 0; }
            nloop = 0; retst = 0 - 1;
            tp = resync(errtop);
        }
        errtop = tp;
        p = ask(0);
        if (p == P_END) break;
        if (p == P_TYPEDEF) { do_typedef(); continue; }
        /* P_ENUM: `enum E { ... };`, `enum E x = C;` and `enum { ... } x;`
           are all declarations, and declspec's enumspec takes each of them.
           This branch used to insist on a body, so a USE was refused. */
        w = declspec();
        gstruct = declstruct;
        gflt0 = declflt;
        if (cur() == tidx(";", 1)) { adv(); continue; }  /* `struct X {...};` */
        while (1) {
            declstruct = gstruct;
            decldim2 = 0; decldim3 = 0; declfp = declspecfp;
            declptr = declspecptr; declpd = declspecpd;
            declflt = gflt0;
            while (eatstar()) declptr = 1;
            gfpfn = 0; gfpd = 0;
            if (cur() == tidx("(", 1)) { if (kind(tp + 1) == tidx("*", 1)) {
                t = fpdecl(); declptr = 1; gstruct = 0 - 1; declstruct = 0 - 1; gfpd = 1;
                if (fpfn >= 0) { fnresume = tp; tp = fpfn; gfpfn = 1; }
            } else t = adv(); }
            else t = adv();
            gbind = scopebind("top", 3, t);
            p = ask(4);
            if (p == P_FNSIG) {
                declbytes = 8;
                declfp = 0;
                sadd(t, 2, 0, 8);
                /* `binop pick(void)`, `int (*f1(int, int))(int, int)`: a call
                   of it yields a function pointer */
                if (declspecfp || gfpfn) symfpret[nsym - 1] = 1;
                symrfst[nsym - 1] = declspecfpst;
                if (function(t, w) == 2) { adv(); continue; }
                break;
            }
            n = 1;
            isarr = 0;
            /* `void (*tab[4])(void)` at file scope: an array of fpdim
               pointers, as the local path has long known.  Here it was one
               8-byte scalar indexed by BYTE -- atexit's handler table was
               the first global one anybody wrote [S-15 D2]. */
            if (gfpd) { if (gfpfn == 0) { if (fpdim > 0) {
                n = fpdim; isarr = 1; declpd = 1; declfp = 0;
            } } }
            if (cur() == vfind(TOKV, NTOKV, "[", 1)) {
                adv();
                if (cur() == vfind(TOKV, NTOKV, "]", 1)) {
                    n = initcount();
                    if (gstruct >= 0) {
                        int per; per = structslots(gstruct);
                        n = (n + per - 1) / per;
                    }
                }
                else n = cexpr();
                need(vfind(TOKV, NTOKV, "]", 1), "]");
                isarr = 1;
                decldim2 = 0; decldim3 = 0;
                if (cur() == vfind(TOKV, NTOKV, "[", 1)) {
                    adv();
                    decldim2 = cexpr();
                    need(vfind(TOKV, NTOKV, "]", 1), "]");
                    n = n * decldim2;
                    dim3decl();
                    n = n * dim3n;
                }
                if (gstruct >= 0) w = declsz;
            }
            gpd = declpd;
            if (isarr) { if (gpd > 0) w = 8; }          /* an array of pointers */
            declbytes = n * declsz;
            if (isarr) { if (gpd > 0) declbytes = n * 8; }
            if (declptr) { if (isarr == 0) declbytes = 8; }
            /* `int x;` followed by `int x = 5;` is ONE object: C calls the
               first a tentative definition and the second completes it.
               Emitting `.bss g_x` twice reserved space nobody uses -- both
               back ends intern the name, so the second blob is dead -- and
               it put a symbol out of address order, which is the shape that
               took the back end down on the compiler itself [E-61]. */
            /* a file-scope static belongs to THIS unit; record it before
               anything spells the name, so the definition already carries
               the suffix */
            if (declstatic) ustat_add(t);
            /* ...which also means a second unit's `static hidden` is NOT a
               redeclaration of the first unit's: it needs its own storage,
               under its own name */
            k = sfind(t);
            gdup = k >= 0 && symunit[k] == curunit;
            sadd(t, gbind, 0, w);
            /* a struct global is an aggregate: its name is its address */
            if (declptr == 0) { if (gstruct >= 0) { if (isarr == 0) {
                symkind[nsym - 1] = 5; symptr[nsym - 1] = 1;
                symelem[nsym - 1] = declsz; } } }
            /* a global array name denotes its address, exactly like a local
               one -- without this `read(fd, src, n)` passes the first eight
               BYTES OF src as the pointer */
            if (isarr) { symkind[nsym - 1] = 5; symptr[nsym - 1] = 1; symptrd[nsym - 1] = gpd + 1; }
            if (gdup == 0) {
                es(".bss g_"); etok(t); ec(32);
                /* an ARRAY needs its full storage; a bare pointer needs 8.
                   Do not conflate the two -- `char src[MAXSRC]` getting 8
                   bytes puts the next global straight on top of the source
                   buffer. */
                if (isarr) { if (gstruct >= 0) en(n * declsz); else en(n * w); }
                else { if (declptr) en(8); else {
                    if (gstruct >= 0) en(declsz); else en(n * w); } }
                ec(10);
            }
            if (cur() == tidx("=", 1)) {
                adv();
                toinit = 1; hasinit = 1;
                gk = dkind(gflt0);
                initflt = gflt0;
                cpn = 0 - 1;
                if (cur() == tidx("(", 1)) cpn = cplit();
                if (cur() == tidx("{", 1)) {
                    if (isarr) { initisarr = 1; initrows = decldim2; initrows3 = decldim3; initaggr(1, t, 0, w, gstruct, n * w); }
                    else { if (gstruct >= 0) initaggr(1, t, 0, w, gstruct, declsz);
                           else initaggr(1, t, 0, w, 0 - 1, n * w); }
                    while (cpn > 0) { need(tidx(")", 1), ")"); cpn = cpn - 1; }
                }
                else { if (cur() == T_STR) { if (isarr) { if (w == strw(tp)) {
                    initstr(1, t, 0, n);
                } else { expr(); loadval();
                    es("  @mem.lea r1, g_"); etok(t); ec(10); estore(8); } }
                else { expr(); loadval();
                    fconv(fkind(), gk);
                    es("  @mem.lea r1, g_"); etok(t); ec(10);
                    if (declptr) estore(8); else estore(w); } }
                else { expr(); loadval();
                    fconv(fkind(), gk);
                    es("  @mem.lea r1, g_"); etok(t); ec(10);
                    if (declptr) estore(8); else estore(w); } }
                toinit = 0;
            }
            if (eat(vfind(TOKV, NTOKV, ",", 1)) == 0) break;
        }
        eat(vfind(TOKV, NTOKV, ";", 1));
    }
    return 0;
}

int bop(char *n, int L, int lev) { BOP[nbop] = tidx(n, L); BLEV[nbop] = lev; nbop = nbop + 1; return 0; }

int setup_tables(void) {
    P_END = pidx("end", 3);          P_GLOBAL = pidx("global", 6);
    P_TYPEDEF = pidx("typedef", 7);  P_STRUCT = pidx("struct", 6);
    P_ENUM = pidx("enum", 4);        P_DECL = pidx("decl", 4);
    P_IF = pidx("if", 2);            P_WHILE = pidx("while", 5);
    P_FOR = pidx("for", 3);          P_DO = pidx("do", 2);
    P_SWITCH = pidx("switch", 6);    P_CASE = pidx("case", 4);
    P_DEFAULT = pidx("default", 7);  P_RETURN = pidx("return", 6);
    P_BREAK = pidx("break", 5);      P_CONTINUE = pidx("continue", 8);
    P_BLOCK = pidx("block", 5);      P_EXPR = pidx("expr", 4);
    P_NEG = pidx("neg", 3);          P_NOT = pidx("not", 3);
    P_DEREF = pidx("deref", 5);      P_ADDR = pidx("addr", 4);
    P_SIZEOF = pidx("sizeof", 6);    P_PRIM = pidx("prim", 4);
    P_INDEX = pidx("index", 5);      P_CALL = pidx("call", 4);
    P_INC = pidx("inc", 3);          P_FIELD = pidx("field", 5);
    P_DONE = pidx("done", 4);        P_FNSIG = pidx("fn_sig", 6);
    P_VARDEF = pidx("var_def", 7);   P_GOTO = pidx("goto", 4);
    P_BNOT = pidx("bnot", 4);        P_UPLUS = pidx("uplus", 5);
    P_PREINC = pidx("preinc", 6);
    T_EOF = tidx("eof", 3);          T_TYPE = tidx("type", 4);
    T_ID = tidx("id", 2);            T_NUM = tidx("num", 3);
    T_STR = tidx("str", 3);
    nbop = 0;
    bop("|", 1, 0);  bop("^", 1, 1);  bop("&", 1, 2);
    bop("==", 2, 3); bop("!=", 2, 3);
    bop("<", 1, 4);  bop(">", 1, 4);  bop("<=", 2, 4); bop(">=", 2, 4);
    bop("<<", 2, 5); bop(">>", 2, 5);
    bop("+", 1, 6);  bop("-", 1, 6);
    bop("*", 1, 7);  bop("/", 1, 7);  bop("%", 1, 7);
    return 0;
}

int setup(void) {
    int i;
    setup_tables();
    i = 0;
    while (i < 128) { OPCH[i] = 0; i = i + 1; }
    i = 0;
    while (i < NTOKV) {
        int p; p = voff(TOKV, i);
        if (isal(TOKV[p] & 255) == 0) {
            if ((TOKV[p] & 255) != 0) OPCH[TOKV[p] & 255] = 1;
        }
        i = i + 1;
    }
    OPCH[47] = 1; OPCH[42] = 1;
    return 0;
}

/* The front end as a FUNCTION: read `path`, compile it for target `t`, and
   leave the tape text in out[0..nout).  unisacc's own main calls it, and so
   does unisaccrun, which then runs the tape instead of writing it out. */
/* A tape, read as it stands.  `unisacc x.tape -b os/arch` puts the back end
   under test on its own: the layout suite enumerates tapes that no C program
   would produce, and a tape is the only input both back ends take. */
int fe_read(char *path) {
    int fd;
    /* the back end asks the model too (isel, abi, reloc), and the model is
       what fe_tape sets up on the way past */
    model_dims();
    setup();
    fd = ropen(path);
    if (fd < 0) { printf("cannot open %s\n", path); return 1; }
    nout = __read(fd, out, MAXOUT);
    __close(fd);
    if (nout < 0) { printf("cannot read %s\n", path); return 1; }
    if (nout >= MAXOUT) { __write(2, "tape too large\n", 15); __exit(1); }
    return 0;
}

/* Does the path end in ".c"?  Under `-run` this is what separates the
   inputs from the program's own arguments. */
int isdotc(char *p) {
    int n;
    if (p[0] == 45 && p[1] == 0) return 1;         /* `-`: standard input */
    n = 0; while (p[n]) n = n + 1;
    if (n < 2) return 0;
    if (p[n - 2] == 46 && p[n - 1] == 99) return 1;     /* .c */
    if (n >= 5) { if (p[n - 5] == 46 && p[n - 4] == 116 && p[n - 3] == 97
                   && p[n - 2] == 112 && p[n - 1] == 101) return 1; }  /* .tape */
    return 0;
}

/* Does the path end in ".tape"? */
int istape(char *p) {
    int n;
    n = 0; while (p[n]) n = n + 1;
    if (n < 5) return 0;
    return p[n - 5] == 46 && p[n - 4] == 116 && p[n - 3] == 97
        && p[n - 2] == 112 && p[n - 1] == 101;
}

/* Read, preprocess and lex ONE file.  Split out of fe_tape so that several
   files can be walked into one tape: preprocessing is per file (include
   guards, `#define` state, `__FILE__`), only the walk is shared -- the same
   division driver.compile_sources makes on the Python side. */
int fe_load(char *path, char *t) {
    int fd; int k;
    srcpath = path;
    optincdl = 0;
    if (optinc) {
        k = 0;
        while (optinc[k] && k < 500) { optincdir[k] = optinc[k]; k = k + 1; }
        if (k > 0 && optincdir[k - 1] != 47) { optincdir[k] = 47; k = k + 1; }
        optincdir[k] = 0; optincdl = k;
    }
    toinit = 0; hasinit = 0;
    fnresume = 0 - 1;
    /* `-` is standard input: the whole of it, in pieces */
    if (path[0] == 45 && path[1] == 0) {
        int got; nsrc = 0;
        while (nsrc < MAXSRC - 1) {
            got = __read(0, src + nsrc, MAXSRC - 1 - nsrc);
            if (got <= 0) break;
            nsrc = nsrc + got;
        }
    } else {
        fd = ropen(path);
        if (fd < 0) { printf("cannot open input\n"); return 1; }
        nsrc = __read(fd, src, MAXSRC);
        __close(fd);
    }
    /* -include FILE: as if `#include "FILE"` were the first line.  The
       lines it adds sit BEFORE the user's, so err_at subtracts them. */
    prelines = 0;
    if (nopti > 0) {
        int ni; int total; int q;
        total = 0;
        ni = 0; while (ni < nopti) { total = total + 12 + blen(opti[ni]); ni = ni + 1; }
        if (nsrc + total >= MAXSRC - 1) { printf("source too large\n"); return 1; }
        q = nsrc - 1; while (q >= 0) { src[q + total] = src[q]; q = q - 1; }
        q = 0; ni = 0;
        while (ni < nopti) {
            int k2; char *nm; nm = opti[ni];
            k2 = 0; while ("#include \""[k2]) { src[q] = "#include \""[k2]; q = q + 1; k2 = k2 + 1; }
            k2 = 0; while (nm[k2]) { src[q] = nm[k2]; q = q + 1; k2 = k2 + 1; }
            src[q] = 34; src[q + 1] = 10; q = q + 2;
            prelines = prelines + 1; ni = ni + 1;
        }
        nsrc = nsrc + total;
    }
    if (nsrc >= MAXSRC - 1) { printf("source too large\n"); return 1; }
    /* a shebang line belongs to the shell, not to C: blank it, keeping the
       newline so every later position still reports the right line */
    if (nsrc > 1) { if (src[0] == 35) { if (src[1] == 33) {
        k = 0;
        while (k < nsrc && src[k] != 10) { src[k] = 32; k = k + 1; }
    } } }
    tgt = t;
    splice();
    decomment();
    autoinc();
    preprocess();
    expandsrc();
    if (pponly) {                       /* -E: the text, not a program */
        __write(bkfd, src, nsrc);
        return 2;
    }
    if (lex() < 0) return 1;
    tp = 0;
    return 0;
}

/* The whole front end: the prologue, every unit, the epilogue.  `paths` is
   one file or several, and several make ONE program -- there is no linker,
   so a call in the first unit reaches a definition in the last the same way
   it reaches one further down its own file. */
int fe_units(char **paths, int npath, char *t) {
    int k; int u; int r;
    /* The model answers the `@stage.op` forms the prologue itself contains,
       so it is set up BEFORE anything is emitted.  With this after the
       first file was loaded, `@call.call __init` came out as `add64` -- a
       wrong tape, quietly. */
    model_dims();
    setup();
    nout = 0; nsym = 0; nlab = 0; npool = 0;
    poolend = 0; nloop = 0;
    /* `__init` ACCUMULATES: every unit's global initialisers run there, so
       this is reset once for the program, not once per file.  Resetting it
       per file silently dropped unit one's initialisers -- the program ran
       and printed zeros. */
    nibuf = 0; nustat = 0;
    es("_start:\n  @call.call __init\n");
    es("  .argc r0\n  .lea r1, __argvv\n  imm r2, 0\n__argv_top:\n"
       "  slt64 r3, r2, r0\n  jumpz r3, __argv_done\n  .argv r4, r2\n"
       "  imm r5, 8\n  mul64 r5, r2, r5\n  add64 r5, r1, r5\n"
       "  store64 [r5+0], r4\n  imm r5, 1\n  add64 r2, r2, r5\n"
       "  jump __argv_top\n__argv_done:\n");
    es("  @call.call main\n  @ctrl.jump __main_ret\n.bss __argvv 32768\n");
    u = 0;
    while (u < npath) {
        curunit = u;
        r = fe_load(paths[u], t);
        if (r) return r;
        unit();
        u = u + 1;
    }
    es("__init:\n");
    k = 0; while (k < nibuf) { out[nout] = ibuf[k]; nout = nout + 1; k = k + 1; }
    es("  @ctrl.ret\n");
    /* A return from main is exit(status) (C99 5.1.2.2.3).  When the program
       carries our <stdlib.h>, `exit` is a function in it -- the one that
       runs the atexit handlers -- so the entry stub returns through it
       rather than leaving by `.exit` with the handlers unrun.  Only after
       every unit is walked is it known whether there is one. */
    es("__main_ret:\n");
    if (symfn("exit", 4) >= 0) es("  @call.call exit\n");
    es("  @lit.exit r0\n");
    if (needslen) {
        es("__slen:\n  mov r2, r0\n  @lit.imm r1, 0\n"
           "__slen_top:\n  @alu.add r4, r2, r1\n  @mem.ld r5, [r4+0], 1\n"
           "  @ctrl.jumpz r5, __slen_end\n  @lit.imm r5, 1\n  @alu.add r1, r1, r5\n"
           "  @ctrl.jump __slen_top\n__slen_end:\n  mov r0, r1\n  @ctrl.ret\n");
    }
    if (needchb) es(".bss __chb 8\n");
    if (needxb) {
        es(".bss __xbuf 24\n"
           "__itoab:\n  @call.frame 16\n  @mem.store [r7+0], r1\n"
           "  @mem.store [r7+8], r2\n  mov r2, r0\n  @mem.lea r1, __xbuf\n"
           "  @lit.imm r3, 24\n  @alu.add r1, r1, r3\n  @lit.imm r4, 0\n"
           "itoab_loop:\n  @mem.load r3, [r7+0]\n  .umod r5, r2, r3\n"
           "  .udiv r2, r2, r3\n  @lit.imm r3, 10\n  @alu.lt r0, r5, r3\n"
           "  @ctrl.jumpz r0, itoab_alpha\n  @lit.imm r3, 48\n  @ctrl.jump itoab_add\n"
           "itoab_alpha:\n  @mem.load r3, [r7+8]\n  @lit.imm r0, 10\n"
           "  @alu.sub r5, r5, r0\n"
           "itoab_add:\n  @alu.add r5, r5, r3\n  @lit.imm r3, 1\n"
           "  @alu.sub r1, r1, r3\n  @mem.st [r1+0], r5, 1\n  @alu.add r4, r4, r3\n"
           "  @ctrl.jumpz r2, itoab_done\n  @ctrl.jump itoab_loop\n"
           "itoab_done:\n  @call.frame -16\n  mov r0, r1\n  mov r1, r4\n  @ctrl.ret\n");
    }
    emit_pool();
    if (nerr == 0) nerr = undef_calls();
    if (nerr == 0 && optlevel > 0) opt_stack();
    if (nwarn > 0) { if (nerr == 0) {
        en2(nwarn); __write(2, nwarn == 1 ? " warning generated.\n" : " warnings generated.\n", nwarn == 1 ? 20 : 21);
    } }
    if (nerr > 0) {
        en2(nerr); __write(2, nerr == 1 ? " error generated.\n" : " errors generated.\n", nerr == 1 ? 18 : 19);
        return 1;
    }
    return 0;
}

/* A call to a function no unit defines.  There is no linker, so such a
   call was written to the tape as-is, and the back end resolved the
   missing label to offset 0 -- the program jumped into its own entry and
   hung or crashed.  The Python front end has always refused it; this is
   the same refusal, made after every unit is walked (a later unit may
   define the name), and shaped like a linker's, since by then the calling
   unit's text is gone. */
#define UD_SIZE 65536
int ud_tab[UD_SIZE];
int ud_same(int a, int b) {          /* do the names at out+a, out+b match */
    int ea; int eb;
    while (1) {
        ea = out[a] == 10 || out[a] == 58;   /* a label ends in `:`, a call in */
        eb = out[b] == 10 || out[b] == 58;   /* a newline: both are the end */
        if (ea || eb) return ea && eb;
        if (out[a] != out[b]) return 0;
        a = a + 1; b = b + 1;
    }
    return 0;
}
int ud_len(int a) { int n; n = 0; while (out[a + n] != 10 && out[a + n] != 58) n = n + 1; return n; }
int ud_slot(int a) {                  /* its slot: found, or the free one */
    int h;
    h = vhash(out + a, ud_len(a)) * 16 + (ud_len(a) & 15);
    h = h & (UD_SIZE - 1);
    while (ud_tab[h]) { if (ud_same(ud_tab[h] - 1, a)) return h; h = (h + 1) & (UD_SIZE - 1); }
    return h;
}
int undef_calls(void) {
    int i; int e; int h; int bad;
    i = 0; while (i < UD_SIZE) { ud_tab[i] = 0; i = i + 1; }
    i = 0;                            /* every `name:` that starts a line */
    while (i < nout) {
        e = i; while (e < nout) { if (out[e] == 10) break; e = e + 1; }
        if (e > i + 1) { if (out[e - 1] == 58) { if (out[i] != 32) { if (out[i] != 46) {
            h = ud_slot(i); if (ud_tab[h] == 0) ud_tab[h] = i + 1;
        } } } }
        i = e + 1;
    }
    bad = 0; i = 0;
    while (i < nout) {
        e = i; while (e < nout) { if (out[e] == 10) break; e = e + 1; }
        /* `  call NAME` -- es() has already asked irsel for the spelling */
        if (e - i > 7) { if (out[i] == 32) { if (out[i + 1] == 32) { if (out[i + 2] == 99) {
            if (out[i + 3] == 97) { if (out[i + 4] == 108) { if (out[i + 5] == 108) { if (out[i + 6] == 32) {
                h = ud_slot(i + 7);
                if (ud_tab[h] == 0) {
                    ud_tab[h] = i + 8;        /* report each name once */
                    __write(2, "unisacc: error: undefined function '", 36);
                    __write(2, out + i + 7, e - i - 7);
                    __write(2, "'\n", 2);
                    bad = bad + 1;
                }
            } } } } } } } }
        i = e + 1;
    }
    return bad;
}

/* ---- -O1: the stack top in a register [H1] -------------------------------
   The walker evaluates `a op b` by pushing a, computing b, popping a:

       .frame 8 / store64 [r7+0], rX / S... / load64 rY, [r7+0] / .frame -8

   When S is straight-line code that never names rY or r7 and has no
   implicit operands, the slot is a register move:  mov rY, rX / S...
   (with S empty and X = Y the pair is nothing at all).  A tape -> tape
   rewrite, so both back ends lower the same optimised tape and closure
   still holds byte for byte; at -O0 the tape is exactly the walker's. */
#define OPT_MAXL 1048576
int ol_s[OPT_MAXL]; int ol_e[OPT_MAXL]; int ol_n;   /* line starts, and where each ends (its newline, or nout) */
char out2[MAXOUT]; int nout2;
int ol_len(int l) { return ol_e[l] - ol_s[l]; }
int ol_is(int l, char *t) {          /* line l is exactly t */
    int k; int p; p = ol_s[l]; k = 0;
    while (t[k]) { if (out[p + k] != t[k]) return 0; k = k + 1; }
    return k == ol_len(l);
}
int ol_reg(int p, int e) {            /* rN spanning out[p..e) -> N, else -1 */
    int n;
    if (e - p < 2 || out[p] != 114) return 0 - 1;
    p = p + 1; n = 0;
    while (p < e) { if (isdi(out[p] & 255) == 0) return 0 - 1; n = n * 10 + out[p] - 48; p = p + 1; }
    return n;
}
int ol_push(int l) {                  /* `  store64 [r7+0], rX` -> X */
    char *t; int k; int p;
    t = "  store64 [r7+0], "; p = ol_s[l]; k = 0;
    while (t[k]) { if (out[p + k] != t[k]) return 0 - 1; k = k + 1; }
    return ol_reg(p + k, p + ol_len(l));
}
int ol_pop(int l) {                   /* `  load64 rY, [r7+0]` -> Y */
    char *t; int k; int p; int e; int q;
    t = "  load64 "; p = ol_s[l]; k = 0;
    while (t[k]) { if (out[p + k] != t[k]) return 0 - 1; k = k + 1; }
    e = p + ol_len(l); q = p + k;
    while (q < e && out[q] != 44) q = q + 1;
    if (e - q != 8) return 0 - 1;
    t = ", [r7+0]"; k = 0;
    while (k < 8) { if (out[q + k] != t[k]) return 0 - 1; k = k + 1; }
    return ol_reg(p + 9, q);
}
int ol_names(int l, int r) {          /* does line l name register r */
    int p; int e; int n;
    p = ol_s[l]; e = p + ol_len(l);
    while (p < e) {
        if (out[p] == 114) { if (p == ol_s[l] || isal(out[p - 1] & 255) == 0) {
            if (p + 1 < e && isdi(out[p + 1] & 255)) {
                n = 0; p = p + 1;
                while (p < e && isdi(out[p] & 255)) { n = n * 10 + out[p] - 48; p = p + 1; }
                if (n == r) return 1;
                continue;
            }
        } }
        p = p + 1;
    }
    return 0;
}
int ol_simple(int l) {                /* straight-line, explicit operands only */
    char *ok; int p; int e; int k; int w; int i;
    p = ol_s[l]; e = p + ol_len(l);
    if (e - p < 3 || out[p] != 32 || out[p + 1] != 32) return 0;
    p = p + 2; w = p; while (w < e && out[w] != 32) w = w + 1;
    ok = "imm\0load64\0store64\0sub64\0.ld\0.lea\0.st\0add64\0mul64\0eq\0mov\0slt64\0and64\0ne\0sle64\0or64\0shl64\0shr64\0lshr64\0xor64\0ult64\0";
    i = vfind(ok, 21, out + p, w - p);
    return i >= 0;
}
int ol_emit(int l) { int k; k = 0; while (k < ol_len(l)) { out2[nout2] = out[ol_s[l] + k]; nout2 = nout2 + 1; k = k + 1; }
    out2[nout2] = 10; nout2 = nout2 + 1; return 0; }
int ol_mov(int y, int x) {
    char b[32]; int n;
    out2[nout2] = 32; out2[nout2 + 1] = 32; nout2 = nout2 + 2;
    b[0] = 109; b[1] = 111; b[2] = 118; b[3] = 32; n = 4;
    out2[nout2] = 109; out2[nout2 + 1] = 111; out2[nout2 + 2] = 118; out2[nout2 + 3] = 32; nout2 = nout2 + 4;
    out2[nout2] = 114; nout2 = nout2 + 1;
    if (y >= 10) { out2[nout2] = 48 + y / 10; nout2 = nout2 + 1; }
    out2[nout2] = 48 + y % 10; out2[nout2 + 1] = 44; out2[nout2 + 2] = 32; out2[nout2 + 3] = 114; nout2 = nout2 + 4;
    if (x >= 10) { out2[nout2] = 48 + x / 10; nout2 = nout2 + 1; }
    out2[nout2] = 48 + x % 10; out2[nout2 + 1] = 10; nout2 = nout2 + 2;
    return n;
}
/* Is register z dead at line `from`?  A bounded forward walk over the
   tape's own control flow: fall through, follow `jump`, take both ways of
   `jumpz`.  A pure write of z first on every path: dead.  A read, a call
   (the callee reads its arguments from r0..r5), anything outside the
   whitelist, or running out of budget: assume live.  `ret` hands back r0
   (and r1 for a pair), so z in r3..r5 is dead there. */
int ol_lab(int p, int e) {           /* the line index of label out[p..e) */
    int h; int q; int lo; int hi; int m; int k;
    h = vhash(out + p, e - p) * 16 + ((e - p) & 15); h = h & (UD_SIZE - 1);
    while (ud_tab[h]) {
        q = ud_tab[h] - 1; k = 0;
        while (k < e - p && out[q + k] == out[p + k]) k = k + 1;
        if (k == e - p && out[q + k] == 58) {
            lo = 0; hi = ol_n - 1;
            while (lo < hi) { m = (lo + hi + 1) / 2; if (ol_s[m] <= q) lo = m; else hi = m - 1; }
            return lo;
        }
        h = (h + 1) & (UD_SIZE - 1);
    }
    return 0 - 1;
}
int ol_labels(void) {                /* ud_tab: every `name:` line of out */
    int l; int h; int p;
    l = 0; while (l < UD_SIZE) { ud_tab[l] = 0; l = l + 1; }
    l = 0;
    while (l < ol_n) {
        p = ol_s[l];
        if (ol_len(l) > 1) { if (out[ol_e[l] - 1] == 58) { if (out[p] != 32) { if (out[p] != 46) {
            h = vhash(out + p, ol_len(l) - 1) * 16 + ((ol_len(l) - 1) & 15); h = h & (UD_SIZE - 1);
            while (ud_tab[h]) h = (h + 1) & (UD_SIZE - 1);
            ud_tab[h] = p + 1;
        } } } }
        l = l + 1;
    }
    return 0;
}
int ol_word(int l, char *w) {        /* the op of line l is w */
    int p; int k; p = ol_s[l] + 2; k = 0;
    if (out[ol_s[l]] != 32) return 0;
    while (w[k]) { if (out[p + k] != w[k]) return 0; k = k + 1; }
    return out[p + k] == 32 || p + k == ol_e[l];
}
int ol_firstreg(int l) {             /* the first rN on the line, or -1 */
    int p; int e; int n;
    p = ol_s[l] + 2; e = ol_e[l];
    while (p < e && out[p] != 32) p = p + 1;
    while (p < e) {
        if (out[p] == 114 && isal(out[p - 1] & 255) == 0 && p + 1 < e && isdi(out[p + 1] & 255)) {
            n = 0; p = p + 1;
            while (p < e && isdi(out[p] & 255)) { n = n * 10 + out[p] - 48; p = p + 1; }
            return n;
        }
        if (out[p] == 91) return 0 - 1;     /* `[`: a memory operand comes first */
        p = p + 1;
    }
    return 0 - 1;
}
int ol_writes(int l, int z) {        /* z is written and not read on line l */
    int p; int e; int n; int first;
    if (ol_word(l, "store64") || ol_word(l, ".st")) return 0;
    if (ol_firstreg(l) != z) return 0;
    p = ol_s[l] + 2; e = ol_e[l]; first = 1;
    while (p < e && out[p] != 32) p = p + 1;
    while (p < e) {
        if (out[p] == 114 && isal(out[p - 1] & 255) == 0 && p + 1 < e && isdi(out[p + 1] & 255)) {
            n = 0; p = p + 1;
            while (p < e && isdi(out[p] & 255)) { n = n * 10 + out[p] - 48; p = p + 1; }
            if (first == 0 && n == z) return 0;
            first = 0;
            continue;
        }
        p = p + 1;
    }
    return 1;
}
/* Liveness of one register at block granularity, for -O2.  Blocks start
   at a label and after a jump, jumpz or ret.  A block uses z first by
   reading it (live in), by writing it (dead in), or not at all (live in
   iff a successor is).  A call reads z iff the callee's entry block has z
   live; anything outside the whitelist is taken to read every register;
   `ret` hands back r0/r1 only.  The least fixed point is the liveness. */
#define BL_MAX 131072
int bl_of[OPT_MAXL]; int bl_s[BL_MAX]; int bl_n; char bl_live[6 * BL_MAX]; int bl_z;
int ol_target(int l) {                /* the block a jump/call names, or -1 */
    int e; int p; int t;
    e = ol_e[l]; p = e; while (p > ol_s[l] && out[p - 1] != 32) p = p - 1;
    t = ol_lab(p, e);
    if (t < 0) return 0 - 1;
    return bl_of[t];
}
int ol_isctl(int l) { return ol_word(l, "jump") || ol_word(l, "jumpz") || ol_word(l, "ret"); }
int bl_split(void) {
    int l; int cut;
    bl_n = 0; cut = 1; l = 0;
    while (l < ol_n) {
        if (out[ol_s[l]] != 32) cut = 1;
        if (cut) { if (bl_n >= BL_MAX) return 0; bl_s[bl_n] = l; bl_n = bl_n + 1; cut = 0; }
        bl_of[l] = bl_n - 1;
        if (out[ol_s[l]] == 32) { if (ol_isctl(l)) cut = 1; }
        l = l + 1;
    }
    return 1;
}
/* Each line once per round: its kind, the registers it reads and writes,
   and the block it jumps to -- the solver then never re-parses text. */
int ol_k[OPT_MAXL]; int ol_rm[OPT_MAXL]; int ol_wm[OPT_MAXL]; int ol_tg[OPT_MAXL];
#define OK_SIMPLE 0
#define OK_LABEL 1
#define OK_RET 2
#define OK_JUMP 3
#define OK_JUMPZ 4
#define OK_CALL 5
#define OK_FRAME 6
#define OK_OTHER 7
int ol_mask(int l) {                  /* every rN the line names */
    int p; int e; int n; int m;
    p = ol_s[l]; e = ol_e[l]; m = 0;
    while (p < e) {
        if (out[p] == 114 && (p == ol_s[l] || isal(out[p - 1] & 255) == 0) && p + 1 < e && isdi(out[p + 1] & 255)) {
            n = 0; p = p + 1;
            while (p < e && isdi(out[p] & 255)) { n = n * 10 + out[p] - 48; p = p + 1; }
            if (n < 16) m = m | (1 << n);
            continue;
        }
        p = p + 1;
    }
    return m;
}
int ol_prep(void) {
    int l; int f; int m;
    l = 0;
    while (l < ol_n) {
        ol_rm[l] = 0; ol_wm[l] = 0; ol_tg[l] = 0 - 1;
        if (out[ol_s[l]] != 32) ol_k[l] = OK_LABEL;
        else if (ol_word(l, "ret")) ol_k[l] = OK_RET;
        else if (ol_word(l, "jump")) { ol_k[l] = OK_JUMP; ol_tg[l] = ol_target(l); }
        else if (ol_word(l, "jumpz")) { ol_k[l] = OK_JUMPZ; ol_tg[l] = ol_target(l); ol_rm[l] = ol_mask(l); }
        else if (ol_word(l, "call")) { ol_k[l] = OK_CALL; ol_tg[l] = ol_target(l); }
        else if (ol_word(l, ".frame")) ol_k[l] = OK_FRAME;
        else if (ol_simple(l)) {
            ol_k[l] = OK_SIMPLE; m = ol_mask(l); f = 0 - 1;
            if (ol_word(l, "store64") == 0 && ol_word(l, ".st") == 0) f = ol_firstreg(l);
            if (f >= 0) {
                ol_wm[l] = 1 << f;
                ol_rm[l] = m & ~(1 << f);
                if (ol_writes(l, f) == 0) ol_rm[l] = ol_rm[l] | (1 << f);
            } else ol_rm[l] = m;
        } else { ol_k[l] = OK_OTHER; ol_rm[l] = 255; }
        l = l + 1;
    }
    return 0;
}
/* from line l on, is z read before written: 1 live, 0 dead */
int ol_scan(int z, int l) {
    int t; int k; int bit; int base;
    bit = 1 << z; base = z * BL_MAX;
    while (l < ol_n) {
        k = ol_k[l];
        if (k == OK_LABEL) return bl_live[base + bl_of[l]];
        if (k == OK_RET) return z <= 1;           /* r0/r1 carry the result */
        if (k == OK_JUMP) { t = ol_tg[l]; return t < 0 ? 1 : bl_live[base + t]; }
        if (k == OK_JUMPZ) {
            if (ol_rm[l] & bit) return 1;
            t = ol_tg[l]; if (t < 0) return 1;
            if (bl_live[base + t]) return 1;
        } else if (k == OK_CALL) {
            t = ol_tg[l]; if (t < 0) return 1;
            if (bl_live[base + t]) return 1;
        } else if (k != OK_FRAME) {
            if (ol_rm[l] & bit) return 1;
            if (ol_wm[l] & bit) return 0;
        }
        l = l + 1;
    }
    return 1;
}
int bl_solve(int z) {
    int b; int changed; int v; int rounds;
    bl_z = z;
    b = 0; while (b < bl_n) { bl_live[z * BL_MAX + b] = 0; b = b + 1; }
    changed = 1; rounds = 0;
    while (changed && rounds < 64) {
        changed = 0; rounds = rounds + 1;
        b = bl_n - 1;
        while (b >= 0) {
            v = ol_scan(z, out[ol_s[bl_s[b]]] != 32 ? bl_s[b] + 1 : bl_s[b]);
            if (v && bl_live[z * BL_MAX + b] == 0) { bl_live[z * BL_MAX + b] = 1; changed = 1; }
            b = b - 1;
        }
    }
    return changed == 0;
}
int ol_zok[8];
int ol_dead(int z, int from) {
    if (ol_zok[z] == 0) return 0;
    return ol_scan(z, from) == 0;
}
/* -O2: a local's load is `imm r2, N / sub64 rD, r6, r2 / .ld rD, [rD+0], W`
   (or load64).  With r2 dead after it, that is one instruction on the frame
   pointer: `.ld rD, [r6-N], W`.  Returns 1 and writes it, or 0. */
int ol_pre(int l, char *t) {         /* line l starts with t -> its length */
    int k; int p; p = ol_s[l]; k = 0;
    while (t[k]) { if (p + k >= ol_e[l] || out[p + k] != t[k]) return 0; k = k + 1; }
    return k;
}
int ol_put(int p, int e) { while (p < e) { out2[nout2] = out[p]; nout2 = nout2 + 1; p = p + 1; } return 0; }
int ol_puts(char *t) { int k; k = 0; while (t[k]) { out2[nout2] = t[k]; nout2 = nout2 + 1; k = k + 1; } return 0; }
int ol_local(int i) {
    int a; int n0; int n1; int d; int q; int k; int ld;
    if (i + 2 >= ol_n || ol_zok[2] == 0) return 0;
    a = ol_pre(i, "  imm r2, "); if (a == 0) return 0;
    n0 = ol_s[i] + a; n1 = ol_e[i];
    k = n0; if (k >= n1) return 0;
    while (k < n1) { if (isdi(out[k] & 255) == 0) return 0; k = k + 1; }
    a = ol_pre(i + 1, "  sub64 r"); if (a == 0) return 0;
    d = out[ol_s[i + 1] + a] - 48;
    if (d < 0 || d > 7 || d == 2 || d == 6 || d == 7) return 0;
    if (ol_len(i + 1) != a + 9) return 0;            /* `sub64 rD, r6, r2` */
    q = ol_s[i + 1] + a + 1;
    if (out[q] != 44 || out[q + 2] != 114 || out[q + 3] != 54 || out[q + 6] != 114 || out[q + 7] != 50) return 0;
    ld = 0;
    k = ol_pre(i + 2, "  .ld r");
    if (k) { if (out[ol_s[i + 2] + k] - 48 == d) ld = 1; }
    if (ld == 0) {
        k = ol_pre(i + 2, "  load64 r");
        if (k == 0 || out[ol_s[i + 2] + k] - 48 != d) return 0;
        ld = 2;
    }
    q = ol_s[i + 2] + k + 1;                          /* `, [rD+0]` */
    if (out[q] != 44 || out[q + 2] != 91 || out[q + 3] != 114 || out[q + 4] - 48 != d) return 0;
    if (out[q + 5] != 43 || out[q + 6] != 48 || out[q + 7] != 93) return 0;
    if (ld == 2 && q + 8 != ol_e[i + 2]) return 0;
    if (ld == 1 && out[q + 8] != 44) return 0;
    if (ol_dead(2, i + 3) == 0) return 0;
    if (ld == 1) ol_puts("  .ld r"); else ol_puts("  load64 r");
    out2[nout2] = 48 + d; nout2 = nout2 + 1;
    ol_puts(", [r6-"); ol_put(n0, n1); ol_puts("]");
    if (ld == 1) ol_put(q + 8, ol_e[i + 2]);
    out2[nout2] = 10; nout2 = nout2 + 1;
    return 1;
}

int opt_round(void) {
    int i; int j; int x; int y; int k; int ok; int hits; int z; int zz; int bad;
    ol_n = 0; i = 0;
    while (i < nout) {
        if (ol_n >= OPT_MAXL) return 0;
        ol_s[ol_n] = i;
        while (i < nout && out[i] != 10) i = i + 1;
        ol_e[ol_n] = i; ol_n = ol_n + 1;
        i = i + 1;
    }
    ol_labels();
    if (optlevel >= 2) { k = 2; while (k <= 5) { ol_zok[k] = 1; k = k + 1; } if (bl_split() == 0) { k = 2; while (k <= 5) { ol_zok[k] = 0; k = k + 1; } }
        else ol_prep();
        k = 2; while (k <= 5) { if (ol_zok[k]) { if (bl_solve(k) == 0) ol_zok[k] = 0; } k = k + 1; } }
    nout2 = 0; hits = 0; i = 0;
    while (i < ol_n) {
        if (i + 3 < ol_n && ol_is(i, "  .frame 8")) {
            x = ol_push(i + 1);
            if (x >= 0) {
                j = i + 2; ok = 0;
                while (j < ol_n && j <= i + 18) {
                    y = ol_pop(j);
                    if (y >= 0) { if (j + 1 < ol_n && ol_is(j + 1, "  .frame -8")) ok = 1; break; }
                    if (ol_simple(j) == 0) break;
                    if (ol_names(j, 7)) break;
                    j = j + 1;
                }
                z = 0 - 1;
                if (ok) {
                    k = i + 2;
                    while (k < j) { if (ol_names(k, y)) ok = 0; k = k + 1; }
                    if (j > i + 2 && x == y) ok = 0;
                    if (ok == 0 && optlevel >= 2) {
                        /* the middle uses y: carry x in a register nobody
                           needs until it is written again */
                        zz = 3;
                        while (zz <= 5 && z < 0) {
                            if (zz != x && zz != y) {
                                k = i + 2; bad = 0;
                                while (k < j) { if (ol_names(k, zz)) bad = 1; k = k + 1; }
                                if (bad == 0) { if (ol_dead(zz, j + 2)) z = zz; }
                            }
                            zz = zz + 1;
                        }
                    }
                }
                if (ok) {
                    if (x != y) ol_mov(y, x);
                    k = i + 2; while (k < j) { ol_emit(k); k = k + 1; }
                    hits = hits + 1;
                    i = j + 2;
                    continue;
                }
                if (z >= 0) {
                    ol_mov(z, x);
                    k = i + 2; while (k < j) { ol_emit(k); k = k + 1; }
                    ol_mov(y, z);
                    hits = hits + 1;
                    i = j + 2;
                    continue;
                }
            }
        }
        if (optlevel >= 2) { if (ol_local(i)) { hits = hits + 1; i = i + 3; continue; } }
        ol_emit(i);
        i = i + 1;
    }
    if (nout2 > 0 && out[nout - 1] != 10) nout2 = nout2 - 1;   /* no newline was there */
    i = 0; while (i < nout2) { out[i] = out2[i]; i = i + 1; }
    nout = nout2;
    return hits;
}
int ol_lines(void) {
    int i;
    ol_n = 0; i = 0;
    while (i < nout) {
        if (ol_n >= OPT_MAXL) return 0;
        ol_s[ol_n] = i;
        while (i < nout && out[i] != 10) i = i + 1;
        ol_e[ol_n] = i; ol_n = ol_n + 1;
        i = i + 1;
    }
    return 1;
}
int ol_commit(void) {
    int i;
    if (nout2 > 0 && out[nout - 1] != 10) nout2 = nout2 - 1;   /* no newline was there */
    i = 0; while (i < nout2) { out[i] = out2[i]; i = i + 1; }
    nout = nout2;
    return 0;
}
/* ---- -O2: the peep table [H2] -------------------------------------------
   The structural half: find an instruction and its neighbour (or, for a
   jump, the instruction it lands on), and say how their operands relate.
   Whether that relation licenses a rewrite -- and which -- is the `peep`
   table's answer, asked of its net exactly as unisa/opt.py asks it. */
int pp_word(int l, char *w) {         /* the op word of line l into w */
    int p; int e; int k;
    k = 0;
    if (l >= ol_n || out[ol_s[l]] != 32) { w[0] = 0; return 0; }
    p = ol_s[l] + 2; e = ol_e[l];
    while (p < e && out[p] != 32 && k < 15) { w[k] = out[p]; k = k + 1; p = p + 1; }
    w[k] = 0;
    return k;
}
int pp_acls(int l) {                  /* index into BF_PEEP_0 */
    char w[16]; int n; char *c;
    n = pp_word(l, w);
    c = "other";
    if (n) {
        if (vfind("store64\0load64\0imm\0mov\0jump\0jumpz\0", 6, w, n) >= 0) c = w;
        else if (vfind("add64\0sub64\0mul64\0shl64\0shr64\0lshr64\0or64\0xor64\0and64\0eq\0ne\0slt64\0sle64\0ult64\0", 14, w, n) >= 0) c = "alu";
    }
    n = 0; while (c[n]) n = n + 1;
    return vfind(BF_PEEP_0, NBF_PEEP_0, c, n);
}
int pp_bcls(int l) {                  /* index into BF_PEEP_1 */
    char w[16]; int n; int i;
    if (l >= ol_n) return vfind(BF_PEEP_1, NBF_PEEP_1, "none", 4);
    n = pp_word(l, w);
    if (n) {
        i = vfind(BF_PEEP_1, NBF_PEEP_1, w, n);
        if (i >= 0 && vfind("other\0none\0", 2, w, n) < 0) return i;
    }
    return vfind(BF_PEEP_1, NBF_PEEP_1, "other", 5);
}
int pp_islab(int l) { return out[ol_s[l]] != 32 && out[ol_s[l]] != 46 && ol_len(l) > 1 && out[ol_e[l] - 1] == 58; }
int pp_real(int l) {                  /* the first line at or after l that is not a label */
    while (l < ol_n && pp_islab(l)) l = l + 1;
    return l;
}
int pp_lastword(int l) {              /* where the line's last token starts */
    int p; p = ol_e[l]; while (p > ol_s[l] && out[p - 1] != 32) p = p - 1; return p;
}
int pp_same(int p, int e, int q, int f) {    /* out[p..e) == out[q..f) */
    if (e - p != f - q) return 0;
    while (p < e) { if (out[p] != out[q]) return 0; p = p + 1; q = q + 1; }
    return 1;
}
/* `  store64 [M], rX` -> X, M at pp_m0..pp_m1; else -1 */
int pp_m0; int pp_m1; int pp_n0; int pp_n1;
int pp_store(int l) {
    int p; int e; int q;
    if (ol_pre(l, "  store64 [") == 0) return 0 - 1;
    p = ol_s[l] + 11; e = ol_e[l]; q = p;
    while (q < e && out[q] != 93) q = q + 1;
    if (q + 3 >= e || out[q + 1] != 44 || out[q + 2] != 32) return 0 - 1;
    pp_m0 = p; pp_m1 = q;
    return ol_reg(q + 3, e);
}
/* `  load64 rY, [M]` -> Y, M at pp_n0..pp_n1 */
int pp_load(int l) {
    int p; int e; int q; int y;
    if (ol_pre(l, "  load64 ") == 0) return 0 - 1;
    p = ol_s[l] + 9; e = ol_e[l]; q = p;
    while (q < e && out[q] != 44) q = q + 1;
    y = ol_reg(p, q);
    if (y < 0 || q + 3 >= e || out[q + 1] != 32 || out[q + 2] != 91 || out[e - 1] != 93) return 0 - 1;
    pp_n0 = q + 3; pp_n1 = e - 1;
    return y;
}
/* the same store shape as the SECOND of a pair: M at pp_n0..pp_n1 */
int pp_store2(int l) {
    int y; int a; int b;
    a = pp_m0; b = pp_m1;
    y = pp_store(l);
    pp_n0 = pp_m0; pp_n1 = pp_m1; pp_m0 = a; pp_m1 = b;
    return y;
}
/* `  imm rK, N` -> K, N in pp_imm (decimal digits only, at most 18) */
long pp_imm;
int pp_immat(int l) {
    int p; int e; int q; int k; long v;
    if (ol_pre(l, "  imm r") == 0) return 0 - 1;
    p = ol_s[l] + 6; e = ol_e[l]; q = p;
    while (q < e && out[q] != 44) q = q + 1;
    k = ol_reg(p, q);
    if (k < 0 || q + 2 >= e || out[q + 1] != 32) return 0 - 1;
    q = q + 2; v = 0;
    if (e - q > 18) return 0 - 1;
    while (q < e) { if (isdi(out[q] & 255) == 0) return 0 - 1; v = v * 10 + out[q] - 48; q = q + 1; }
    pp_imm = v;
    return k;
}
/* `  OP rD, rS, rT` -> 1, with the three in pp_d pp_sr pp_t */
int pp_d; int pp_sr; int pp_t;
int pp_three(int l) {
    int p; int e; int q;
    if (l >= ol_n || out[ol_s[l]] != 32) return 0;
    p = ol_s[l] + 2; e = ol_e[l];
    while (p < e && out[p] != 32) p = p + 1;
    p = p + 1; q = p; while (q < e && out[q] != 44) q = q + 1;
    pp_d = ol_reg(p, q); if (pp_d < 0 || q + 2 >= e) return 0;
    p = q + 2; q = p; while (q < e && out[q] != 44) q = q + 1;
    pp_sr = ol_reg(p, q); if (pp_sr < 0 || q + 2 >= e) return 0;
    pp_t = ol_reg(q + 2, e); if (pp_t < 0) return 0;
    return 1;
}
/* `  mov rY, rX` -> 1, Y and X in pp_d, pp_sr */
int pp_movat(int l) {
    int p; int e; int q;
    if (ol_pre(l, "  mov r") == 0) return 0;
    p = ol_s[l] + 6; e = ol_e[l]; q = p;
    while (q < e && out[q] != 44) q = q + 1;
    pp_d = ol_reg(p, q); if (pp_d < 0 || q + 2 >= e) return 0;
    pp_sr = ol_reg(q + 2, e);
    return pp_sr >= 0;
}
int pp_emitreg(int r) {
    out2[nout2] = 114; nout2 = nout2 + 1;
    if (r >= 10) { out2[nout2] = 48 + r / 10; nout2 = nout2 + 1; }
    out2[nout2] = 48 + r % 10; nout2 = nout2 + 1;
    return 0;
}
int pp_emitnum(long v) {
    char b[24]; int n;
    n = 0;
    if (v == 0) { b[0] = 48; n = 1; }
    while (v > 0) { b[n] = 48 + v % 10; v = v / 10; n = n + 1; }
    while (n > 0) { n = n - 1; out2[nout2] = b[n]; nout2 = nout2 + 1; }
    return 0;
}
int pp_nl(void) { out2[nout2] = 10; nout2 = nout2 + 1; return 0; }
int pp_rel(char *nm) { int n; n = 0; while (nm[n]) n = n + 1; return vfind(BF_PEEP_2, NBF_PEEP_2, nm, n); }
int pp_act(char *nm) { int n; n = 0; while (nm[n]) n = n + 1; return vfind(BH_PEEP_Y, NBH_PEEP_Y, nm, n); }
int pp_asks;
int peep_round(void) {
    int i; int n; int a; int b; int rel; int act; int key[4]; int t; int u; int k; int x; int y;
    int hits; int p; int e; int q; long v; int lg; int d; int none;
    if (ol_lines() == 0) return 0;
    ol_labels();
    k = 0; while (k <= 5) { ol_zok[k] = 1; k = k + 1; }
    if (bl_split() == 0) { k = 0; while (k <= 5) { ol_zok[k] = 0; k = k + 1; } }
    else {
        ol_prep();
        k = 0; while (k <= 5) { if (bl_solve(k) == 0) ol_zok[k] = 0; k = k + 1; }
    }
    none = pp_rel("none");
    nout2 = 0; hits = 0; i = 0; n = ol_n;
    while (i < n) {
        if (out[ol_s[i]] != 32) { ol_emit(i); i = i + 1; continue; }
        a = pp_acls(i); b = pp_bcls(i + 1); rel = none;
        t = 0 - 1; x = 0 - 1; y = 0 - 1; u = 0; v = 0;
        if (ol_word(i, "jump") || ol_word(i, "jumpz")) {
            p = pp_lastword(i); e = ol_e[i];
            k = i + 1;
            while (k < n && pp_islab(k)) {
                if (pp_same(ol_s[k], ol_e[k] - 1, p, e)) u = 1;
                k = k + 1;
            }
            if (u) { rel = pp_rel("to_next"); b = pp_bcls(pp_real(i + 1)); }
            else {
                t = ol_lab(p, e);
                if (t >= 0) {
                    t = pp_real(t + 1);
                    if (t < n && ol_word(t, "jump")) {
                        q = pp_lastword(t);
                        if (pp_same(q, ol_e[t], p, e) == 0) { rel = pp_rel("to_jump"); b = pp_bcls(t); }
                    }
                }
            }
        } else if (i + 1 < n && pp_store(i) >= 0) {
            x = pp_store(i);
            y = pp_load(i + 1);
            if (y < 0) y = pp_store2(i + 1);
            /* not the expression stack's own slot: [r7+..] is what B1 turns
               into a real push and pop on x86, and a pop made a mov is a
               pop that costs more */
            if (y >= 0 && pp_same(pp_m0, pp_m1, pp_n0, pp_n1) && (out[pp_m0] != 114 || out[pp_m0 + 1] != 55)) {
                if (x == y) rel = pp_rel("same_slot_same_reg"); else rel = pp_rel("same_slot");
            }
        } else if (i + 1 < n && pp_immat(i) >= 0) {
            x = pp_immat(i); v = pp_imm;
            if (x <= 5 && ol_word(i + 1, "mov") == 0 && pp_three(i + 1)) {
                if (pp_t == x && pp_sr != x && ol_dead(x, i + 2)) {
                    if (v == 0) rel = pp_rel("const0");
                    else if (v == 1) rel = pp_rel("const1");
                    else if ((v & (v - 1)) == 0) rel = pp_rel("pow2");
                }
            } else if (x <= 5 && pp_movat(i + 1)) {
                if (pp_sr == x && pp_d != x && ol_dead(x, i + 2)) rel = pp_rel("copy_dead");
            }
        }
        if (rel == none) {
            if (ol_k[i] == OK_SIMPLE && ol_word(i, "store64") == 0 && ol_word(i, ".st") == 0) {
                d = ol_firstreg(i);
                if (d >= 0 && d <= 5) { if ((ol_wm[i] & (1 << d)) && ol_dead(d, i + 1)) rel = pp_rel("a_dead"); }
            }
        }
        if (rel == none) { ol_emit(i); i = i + 1; continue; }
        key[0] = a; key[1] = b; key[2] = rel; key[3] = 0;
        act = inf(S_PEEP, key, 0); pp_asks = pp_asks + 1;
        if (act == pp_act("keep")) { ol_emit(i); i = i + 1; continue; }
        hits = hits + 1;
        if (act == pp_act("load_to_mov")) {
            ol_emit(i);
            ol_puts("  mov "); pp_emitreg(y); ol_puts(", "); pp_emitreg(x); pp_nl();
            i = i + 2; continue;
        }
        if (act == pp_act("drop_b")) { ol_emit(i); i = i + 2; continue; }
        if (act == pp_act("drop_a")) { i = i + 1; continue; }
        if (act == pp_act("retarget")) {
            p = pp_lastword(i); ol_put(ol_s[i], p);
            q = pp_lastword(t); ol_put(q, ol_e[t]); pp_nl();
            i = i + 1; continue;
        }
        if (act == pp_act("to_mov")) {
            ol_puts("  mov "); pp_emitreg(pp_d); ol_puts(", "); pp_emitreg(pp_sr); pp_nl();
            i = i + 2; continue;
        }
        if (act == pp_act("to_shl")) {
            lg = 0; while (v > 1) { v = v / 2; lg = lg + 1; }
            ol_puts("  imm "); pp_emitreg(x); ol_puts(", "); pp_emitnum(lg); pp_nl();
            ol_puts("  shl64 "); pp_emitreg(pp_d); ol_puts(", "); pp_emitreg(pp_sr); ol_puts(", "); pp_emitreg(x); pp_nl();
            i = i + 2; continue;
        }
        if (act == pp_act("fold_imm")) {
            ol_puts("  imm "); pp_emitreg(pp_d); ol_puts(", "); pp_emitnum(pp_imm); pp_nl();
            i = i + 2; continue;
        }
        ol_emit(i); i = i + 1; hits = hits - 1;     /* an action this code does not know */
    }
    ol_commit();
    return hits;
}

int opt_stack(void) {
    int r;
    r = 0;
    while (r < 4) { if (opt_round() == 0) break; r = r + 1; }
    if (optlevel >= 2) { r = 0; while (r < 4) { if (peep_round() == 0) break; r = r + 1; } }
    return 0;
}

#ifndef UNISACC_NO_MAIN
/* The command line, tinycc-shaped:

     unisacc -run FILE.c [args...]     compile and run, nothing on disk
     unisacc FILE.c -b os/arch         write the executable
     unisacc FILE.c -c                 write the tape
     unisacc FILE.c -t os/arch         the tape for a target
     unisacc FILE.c                    the token dump (the lexer's instrument)
     -I dir, -D name[=n]               as everywhere else

   Flags are taken from anywhere on the line, because this tool grew up
   writing them AFTER the file and its own suites still spell it that way;
   the first argument that is not a flag is the input, and for `-run` what
   follows the input belongs to the PROGRAM. */
int main(void) {
    int fd; int i; int p; int L; int k; int fi; int runit; int dump; int verb;
    char *a; char *t; long e; int n;
    char *outpath;
    int (*entry)(long, long);
    t = "lnx/x86_64"; fi = 0; runit = 0; dump = 0; verb = 0; outpath = 0;
    ninput = 0;
    i = 1;
    while (i < __argc()) {
        a = __argv(i);
        if (a[0] == 45 && a[1] == 45 && a[1 + 1] == 0) {   /* `--`: argv starts */
            i = i + 1; break;
        }
        if (strsame(a, "--check-oracle")) {
            int b1; int b2;
            model_dims(); setup();
            b1 = oracle_pass(1); b2 = oracle_pass(0 - 1);
            printf("oracle  questions %d   cached answers that differ from the net: %d\n",
                   oracle_n, b1 + b2);
            return (b1 + b2) ? 1 : 0;
        }
        /* `-version` / `--version`: which build is this.  `-v` is taken --
           it prints how many times each stage was asked. */
        if (strsame(a, "-version") || strsame(a, "--version")) {
            printf("unisacc %s\n", UNISACC_VERSION);
            return 0;
        }
        if (a[0] == 45 && a[1]) {
            if (a[1] == 73) {                              /* -I */
                if (a[2]) optinc = a + 2; else { i = i + 1; optinc = __argv(i); }
            } else { if (a[1] == 68) {                     /* -D */
                if (noptd < 16) {
                    if (a[2]) optd[noptd] = a + 2; else { i = i + 1; optd[noptd] = __argv(i); }
                    noptd = noptd + 1;
                }
            } else { if (a[1] == 114) { runit = 1;         /* -run */
            /* -S writes the tape: the tape IS this compiler's assembly-level
               IR, the same level gcc's -S stops at.  -c is kept as a synonym
               because the suites have always spelled it that way -- but
               there are no object files and no linker here, so `-c` never
               means what it means to gcc, and the usage line says so. */
            } else { if (a[1] == 99 || a[1] == 83) { dump = 1;   /* -c, -S */
            } else { if (a[1] == 118) { verb = 1;          /* -v */
            } else { if (a[1] == 111) {                    /* -o */
                if (a[2]) outpath = a + 2; else { i = i + 1; outpath = __argv(i); }
            } else { if (a[1] == 85) {                     /* -U */
                if (noptu < 16) {
                    if (a[2]) optu[noptu] = a + 2; else { i = i + 1; optu[noptu] = __argv(i); }
                    noptu = noptu + 1;
                }
            } else { if (strsame(a, "-include")) {         /* -include FILE */
                i = i + 1;
                if (nopti < 8) { opti[nopti] = __argv(i); nopti = nopti + 1; }
            } else { if (strsame(a, "-Wall") || strsame(a, "-Wextra")) { warnall = 1;
            } else { if (strpre(a, "-ferror-limit=")) { maxerr = 0; k = 14;
                while (a[k] >= 48 && a[k] <= 57) { maxerr = maxerr * 10 + (a[k] - 48); k = k + 1; }
            } else { if (strsame(a, "-nostdinc")) { nostdinc = 1;
            } else { if (strsame(a, "-MD") || strsame(a, "-MMD")) { wantdeps = 1; depfile = depfile ? depfile : "";
            } else { if (strsame(a, "-MF")) { i = i + 1; wantdeps = 1; depfile = __argv(i);
            } else { if (strsame(a, "-MT") || strsame(a, "-MQ")) { i = i + 1;
            } else { if (strsame(a, "-MP") || strsame(a, "-M") || strsame(a, "-MM")) {
            /* -l and -L: the library is in the headers, so there is nothing
               to link and nothing to search.  -x c: the only language. */
            } else { if (a[1] == 108 || a[1] == 76) {
                if (a[2] == 0) i = i + 1;
            } else { if (a[1] == 120) {                    /* -x LANG */
                if (a[2] == 0) i = i + 1;
            } else { if (a[1] == 69) { pponly = 1; dump = 1;  /* -E */
            } else { if (a[1] == 98 || a[1] == 116) {      /* -b, -t */
                if (a[1] == 98) dump = 2; else dump = 1;
                i = i + 1; t = __argv(i);
            /* Flags a build system passes that mean nothing here: there is
               one dialect (C99), one optimisation level, and no separate
               debug info.  A compiler that REFUSES them cannot be dropped
               into an existing Makefile, which is most of what `CC=` is. */
            } else { if (a[1] == 87 || a[1] == 119 || a[1] == 103
                      || a[1] == 79 || a[1] == 102 || a[1] == 115
                      || a[1] == 112 || a[1] == 109) {    /* -W -w -g -O -f -std -pipe -m */
                if (a[1] == 79) {             /* -O, -O0..-O3, -Os: [H1] */
                    optlevel = 1;
                    if (a[2] >= 48 && a[2] <= 57) optlevel = a[2] - 48;
                }
            } else { printf("unisacc: unknown option %s\n", a); return 1; } } } } } } } } } } } } } } } } } } } }
        } else {
            /* Several inputs make ONE program.  Under `-run` the line also
               carries the PROGRAM's arguments, so the inputs are the `.c`
               files at the front: the first argument that is not one ends
               the list and begins argv.  `--` ends it explicitly, for a
               program whose own first argument is a .c file. */
            if (fi == 0) fi = i;
            if (runit) {
                if (isdotc(a) == 0) break;
            }
            if (ninput < 64) { inputs[ninput] = a; ninput = ninput + 1; }
        }
        i = i + 1;
    }
    if (fi == 0) {
        model_dims(); setup();
        printf("usage: unisacc [-run] [-E] [-I dir] [-D name[=n]]"
               " FILE.c [FILE.c...] [-S | -b os/arch] [-o out]"
               " [-- args...]\n"
               "  -S  write the tape, this compiler's assembly-level IR"
               " (-c is a synonym: there are no object files)\n");
        return 1;
    }
    if (depfile) { if (depfile[0] == 0) {
        static char dname[520]; char *base; int k; int dot;
        base = outpath ? outpath : inputs[0];
        k = 0; dot = 0 - 1;
        while (base[k] && k < 512) { dname[k] = base[k]; if (base[k] == 46) dot = k; if (base[k] == 47) dot = 0 - 1; k = k + 1; }
        if (dot < 0) dot = k;
        dname[dot] = 46; dname[dot + 1] = 100; dname[dot + 2] = 0;
        depfile = dname;
    } }
    if (runit) {
        if (fe_units(inputs, ninput, HOST_TARGET)) return 1;
        /* argv[0] is the program, which is its first source file; the rest
           of the line follows the inputs */
        n = __argc() - (fi + ninput) + 1;
        if (n > 255) n = 255;
        if (n < 1) n = 1;
        runargv[0] = __argv(fi);
        k = 1;
        while (k < n) { runargv[k] = __argv(fi + ninput + k - 1); k = k + 1; }
        runargv[n] = 0;
        /* bk_run writes argc/argv into the program's own cells, so this call
           assumes nobody's calling convention */
        e = bk_run(out, nout, n, (long)runargv);
        entry = (int (*)(long, long))e;
        return entry(0, 0);
    }
    if (dump) {
        int ofd; int r;
        /* the destination is opened FIRST: `-E -o x.i` writes from inside
           the front end, before there is a tape to write */
        ofd = 1;
        if (outpath) {
            ofd = wopen(outpath);
            if (ofd < 0) { printf("cannot write %s\n", outpath); return 1; }
        }
        bkfd = ofd;
        if (istape(__argv(fi))) { if (fe_read(__argv(fi))) return 1; }
        else {
            r = fe_units(inputs, ninput, t);
            if (r == 2) { if (ofd != 1) __close(ofd); return 0; }  /* -E is done */
            if (r) return 1;
        }
        if (depfile) { if (writedeps(outpath ? outpath : "a.out", inputs, ninput)) return 1; }
        if (dump == 2) { bk_build(out, nout, t); if (ofd != 1) __close(ofd); return 0; }
        __write(ofd, out, nout);
        if (ofd != 1) __close(ofd);
        /* `-c -v`: how many times each stage was asked, so a test can check
           that no table-shaped stage is decided in code */
        if (verb) {
            nout = 0;
            es("asked pp "); en(nask[S_PP]); es(" lex "); en(nask[S_LEX]);
            es(" parse "); en(nask[S_PARSE]); es(" type "); en(nask[S_TYPE]);
            es(" scope "); en(nask[S_SCOPE]); es(" irsel "); en(nask[S_IRSEL]);
            es(" tyinfo "); en(nask[S_TYINFO]); es(" pfconv "); en(nask[S_PFCONV]);
            ec(10);
            __write(2, out, nout);
        }
        return 0;
    }
    /* the token dump: the lexer's instrument.  No header selection here --
       the Python side it is compared with does that in its driver. */
    nibuf = 0; toinit = 0; hasinit = 0;
    fnresume = 0 - 1;
    model_dims();
    setup();
    srcpath = __argv(fi);
    fd = ropen(srcpath);
    if (fd < 0) { printf("cannot open input\n"); return 1; }
    nsrc = __read(fd, src, MAXSRC);
    __close(fd);
    if (nsrc >= MAXSRC - 1) { printf("source too large\n"); return 1; }
    tgt = "lnx/x86_64";
    splice();
    decomment();
    preprocess();
    expandsrc();
    if (lex() < 0) return 1;
    i = 0;
    while (i < ntok) {
        p = voff(TOKV, tkind[i]);
        L = vlen(TOKV, tkind[i]);
        __write(1, TOKV + p, L);
        if (tkind[i] == 2) { __write(1, "=", 1); __write(1, src + tpos[i], tlen[i]); }
        if (tkind[i] == 3) { __write(1, "=", 1); __write(1, src + tpos[i], tlen[i]); }
        if (tkind[i] == 4) { __write(1, "=", 1); __write(1, src + tpos[i], tlen[i]); }
        __write(1, "\n", 1);
        i = i + 1;
    }
    printf("%d tokens\n", ntok);
    return 0;
}
#endif
