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
/* ---- writing out2: every rewritten line goes through these ---- */
int ol_c(int c) { out2[nout2] = c; nout2 = nout2 + 1; return 0; }
int ol_put(int p, int e) { while (p < e) { ol_c(out[p]); p = p + 1; } return 0; }
int ol_puts(char *t) { int k; k = 0; while (t[k]) { ol_c(t[k]); k = k + 1; } return 0; }
int pk_nl(void) { return ol_c(10); }
/* [J9] every line copied verbatim is remembered -- where it lands in out2
   and which line it was -- so the next round's ol_prep can take the line's
   facts over instead of reading its text again */
int em_pos[OPT_MAXL]; int em_src[OPT_MAXL]; int em_n; int em_gen;
int ol_emit(int l) {
    if (em_n < OPT_MAXL) { em_pos[em_n] = nout2; em_src[em_n] = l; em_n = em_n + 1; }
    ol_put(ol_s[l], ol_e[l]); return pk_nl();
}
int pk_emitreg(int r) {
    ol_c(114);
    if (r >= 10) ol_c(48 + r / 10);
    return ol_c(48 + r % 10);
}
int pk_emitnum(long v) {
    char b[24]; int n;
    n = 0;
    if (v == 0) { b[0] = 48; n = 1; }
    while (v > 0) { b[n] = 48 + v % 10; v = v / 10; n = n + 1; }
    while (n > 0) { n = n - 1; ol_c(b[n]); }
    return 0;
}
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
/* what a tape op IS -- simple (explicit operands only), and its peep
   classes -- is the `opinfo` table's answer [I2].  Asked once per op word
   and remembered: the answer is a function of the word. */
#define OI_MAX 128
int oi_done[OI_MAX]; int oi_simple[OI_MAX]; int oi_acls[OI_MAX]; int oi_bcls[OI_MAX];
/* the op word -> vocabulary index, remembered by the word's bytes: a
   program has a few dozen distinct op words and every line asks.  A
   direct-mapped slot holds the whole word, so a hit is the same answer
   vfind gives. [J9] */
#define OW_SLOTS 512
char ow_txt[OW_SLOTS * 16]; int ow_len[OW_SLOTS]; int ow_idx[OW_SLOTS];
int oi_idx(char *w, int n) {
    int i; int h; int k; int b;
    h = n;
    k = 0; while (k < n) { h = (h * 31 + (w[k] & 255)) & (OW_SLOTS - 1); k = k + 1; }
    if (n <= 16 && ow_len[h] == n) {
        b = h * 16; k = 0;
        while (k < n && ow_txt[b + k] == w[k]) k = k + 1;
        if (k == n) return ow_idx[h];
    }
    i = vfind(BF_OPINFO_0, NBF_OPINFO_0, w, n);
    if (i < 0) i = vfind(BF_OPINFO_0, NBF_OPINFO_0, "other", 5);
    if (n <= 16) {
        b = h * 16; k = 0; while (k < n) { ow_txt[b + k] = w[k]; k = k + 1; }
        ow_len[h] = n; ow_idx[h] = i;
    }
    return i;
}
int oi_ask(int i) {
    int key[4]; int c;
    if (i < 0 || i >= OI_MAX) return 0 - 1;
    if (oi_done[i]) return i;
    key[0] = i; key[1] = 0; key[2] = 0; key[3] = 0;
    c = inf(S_OPINFO, key, HD_OPINFO_SIMPLE);
    oi_simple[i] = c == vfind(BH_OPINFO_SIMPLE, NBH_OPINFO_SIMPLE, "1", 1);
    c = inf(S_OPINFO, key, HD_OPINFO_ACLS);
    oi_acls[i] = vfind(BF_PEEP_0, NBF_PEEP_0, BH_OPINFO_ACLS + voff(BH_OPINFO_ACLS, c), vlen(BH_OPINFO_ACLS, c));
    c = inf(S_OPINFO, key, HD_OPINFO_BCLS);
    oi_bcls[i] = vfind(BF_PEEP_1, NBF_PEEP_1, BH_OPINFO_BCLS + voff(BH_OPINFO_BCLS, c), vlen(BH_OPINFO_BCLS, c));
    oi_done[i] = 1;
    return i;
}
/* the opinfo index of line l's op word, looked up once per line per round:
   every rewrite asked it again by name, and the lookups were a quarter of
   an -O2 compile.  ol_gen moves on whenever the lines are rebuilt. */
int ol_opi_v[OPT_MAXL]; int ol_opi_g[OPT_MAXL]; int ol_gen;
int ol_opi(int l) {
    int p; int e; int w; int i;
    if (ol_opi_g[l] == ol_gen) return ol_opi_v[l];
    p = ol_s[l]; e = ol_e[l]; i = 0 - 1;
    if (out[p] == 32) {
        p = p + 2; w = p;
        while (w < e && out[w] != 32 && w - p < 15) w = w + 1;
        if (w > p) i = oi_ask(oi_idx(out + p, w - p));
    }
    ol_opi_g[l] = ol_gen; ol_opi_v[l] = i;
    return i;
}
int ol_simple(int l) {                /* straight-line, explicit operands only */
    int p; int e; int i;
    p = ol_s[l]; e = p + ol_len(l);
    if (e - p < 3 || out[p] != 32 || out[p + 1] != 32) return 0;
    i = ol_opi(l);
    return i >= 0 && oi_simple[i];
}
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
#define BL_MAX 262144
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
int pv_k[OPT_MAXL]; int pv_rm[OPT_MAXL]; int pv_wm[OPT_MAXL]; int pv_n; int pv_gen;
int ol_from[OPT_MAXL]; int ol_from_ok;
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
/* ol_word's answer for the words ol_prep tells apart, from the word's
   span: the same test (the word, then a space or the end of the line) */
int op_is(int p, int n, char *w) {
    int k; k = 0;
    while (k < n) { if (w[k] == 0 || out[p + k] != w[k]) return 0; k = k + 1; }
    return w[k] == 0;
}
int ol_prep(void) {
    /* One pass over each line's text: the kind from the op word, and the
       operand registers read once for the mask, the first register and
       whether that first register is read again -- ol_mask, ol_firstreg
       and ol_writes each re-read the line, and three reads per line per
       round were 40% of the optimiser.  Same answers. [J9] */
    int l; int f; int m; int p; int e; int w; int wn; int n; int first; int sawbr; int again; int j; int use;
    /* the facts of a line copied verbatim from the last round depend only on
       its text, so they are taken over; only a branch's block (tg) is
       looked up again, the labels having moved [J9] */
    use = ol_from_ok && pv_gen == ol_gen - 1;
    if (pv_gen == ol_gen - 1) {
        j = 0; while (j < pv_n) { pv_k[j] = ol_k[j]; pv_rm[j] = ol_rm[j]; pv_wm[j] = ol_wm[j]; j = j + 1; }
    }
    l = 0;
    while (l < ol_n) {
        ol_rm[l] = 0; ol_wm[l] = 0; ol_tg[l] = 0 - 1;
        if (use && ol_from[l] >= 0) {
            j = ol_from[l];
            ol_k[l] = pv_k[j]; ol_rm[l] = pv_rm[j]; ol_wm[l] = pv_wm[j];
            if (ol_k[l] == OK_JUMP || ol_k[l] == OK_JUMPZ || ol_k[l] == OK_CALL) ol_tg[l] = ol_target(l);
            l = l + 1; continue;
        }
        p = ol_s[l]; e = ol_e[l];
        if (out[p] != 32) { ol_k[l] = OK_LABEL; l = l + 1; continue; }
        w = p + 2; wn = 0; while (w + wn < e && out[w + wn] != 32) wn = wn + 1;
        /* the operands, from after the word */
        m = 0; f = 0 - 1; first = 1; sawbr = 0; again = 0;
        p = w + wn;
        while (p < e) {
            if (out[p] == 91) sawbr = 1;
            if (out[p] == 114 && isal(out[p - 1] & 255) == 0 && p + 1 < e && isdi(out[p + 1] & 255)) {
                n = 0; p = p + 1;
                while (p < e && isdi(out[p] & 255)) { n = n * 10 + out[p] - 48; p = p + 1; }
                if (n < 16) m = m | (1 << n);
                if (first) { if (sawbr == 0) f = n; first = 0; }
                else { if (f >= 0 && n == f) again = 1; }
                continue;
            }
            p = p + 1;
        }
        if (op_is(w, wn, "ret")) ol_k[l] = OK_RET;
        else if (op_is(w, wn, "jump")) { ol_k[l] = OK_JUMP; ol_tg[l] = ol_target(l); }
        else if (op_is(w, wn, "jumpz")) { ol_k[l] = OK_JUMPZ; ol_tg[l] = ol_target(l); ol_rm[l] = m; }
        else if (op_is(w, wn, "call")) { ol_k[l] = OK_CALL; ol_tg[l] = ol_target(l); }
        else if (op_is(w, wn, ".frame")) ol_k[l] = OK_FRAME;
        else if (ol_simple(l)) {
            ol_k[l] = OK_SIMPLE;
            if (op_is(w, wn, "store64") || op_is(w, wn, ".st")) f = 0 - 1;
            if (f >= 0) {
                ol_wm[l] = 1 << f;
                ol_rm[l] = m & ~(1 << f);
                if (again) ol_rm[l] = ol_rm[l] | (1 << f);
            } else ol_rm[l] = m;
        } else { ol_k[l] = OK_OTHER; ol_rm[l] = 255; }
        l = l + 1;
    }
    pv_gen = ol_gen; pv_n = ol_n;
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
/* A block's liveness for z depends only on the live values of the blocks it
   can reach before its first read or write of z.  So each block is scanned
   ONCE into a summary -- the blocks consulted along the way (a jumpz's or a
   call's target) and how the scan ends (read: live, write: dead, or another
   block's value) -- and the rounds below evaluate summaries instead of
   rescanning every instruction: the scans were 60% of an -O2 compile.
   Same order, same rounds, same answers as ol_scan would give. */
int sm_start[BL_MAX]; int sm_n[BL_MAX]; int sm_term[BL_MAX]; int sm_dep[OPT_MAXL];
int bl_summarise(int z) {
    int b; int l; int k; int t; int bit; int nd; int term;
    bit = 1 << z; nd = 0; b = 0;
    while (b < bl_n) {
        sm_start[b] = nd; term = 0 - 1;             /* -1 live, -2 dead, >=0 that block */
        l = out[ol_s[bl_s[b]]] != 32 ? bl_s[b] + 1 : bl_s[b];
        while (1) {
            if (l >= ol_n) { term = 0 - 1; break; }
            k = ol_k[l];
            if (k == OK_LABEL) { term = bl_of[l]; break; }
            if (k == OK_RET) { term = z <= 1 ? 0 - 1 : 0 - 2; break; }
            if (k == OK_JUMP) { t = ol_tg[l]; term = t < 0 ? 0 - 1 : t; break; }
            if (k == OK_JUMPZ || k == OK_CALL) {
                if (k == OK_JUMPZ && (ol_rm[l] & bit)) { term = 0 - 1; break; }
                t = ol_tg[l]; if (t < 0) { term = 0 - 1; break; }
                if (nd >= OPT_MAXL) return 0;
                sm_dep[nd] = t; nd = nd + 1;
            } else if (k != OK_FRAME) {
                if (ol_rm[l] & bit) { term = 0 - 1; break; }
                if (ol_wm[l] & bit) { term = 0 - 2; break; }
            }
            l = l + 1;
        }
        sm_n[b] = nd - sm_start[b]; sm_term[b] = term;
        b = b + 1;
    }
    return 1;
}
int bl_solve(int z) {
    int b; int changed; int v; int rounds; int base; int j; int e;
    bl_z = z; base = z * BL_MAX;
    b = 0; while (b < bl_n) { bl_live[base + b] = 0; b = b + 1; }
    if (bl_summarise(z) == 0) return 0;
    changed = 1; rounds = 0;
    while (changed && rounds < 64) {
        changed = 0; rounds = rounds + 1;
        b = bl_n - 1;
        while (b >= 0) {
            if (bl_live[base + b] == 0) {
                v = sm_term[b] == 0 - 1 ? 1 : (sm_term[b] == 0 - 2 ? 0 : bl_live[base + sm_term[b]]);
                j = sm_start[b]; e = j + sm_n[b];
                while (v == 0 && j < e) { v = bl_live[base + sm_dep[j]]; j = j + 1; }
                if (v) { bl_live[base + b] = 1; changed = 1; }
            }
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

int ol_lines(void);
int opt_round(void) {
    int i; int j; int x; int y; int k; int ok; int hits; int z; int zz; int bad;
    if (ol_lines() == 0) return 0;    /* one line builder: it also retires ol_opi's cache */
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
/* ol_from: line -> the last round's line it copies, or -1 */
int ol_lines(void) {
    int i; int k;
    ol_gen = ol_gen + 1;              /* every cached ol_opi answer is stale */
    ol_n = 0; i = 0;
    while (i < nout) {
        if (ol_n >= OPT_MAXL) return 0;
        ol_s[ol_n] = i;
        while (i < nout && out[i] != 10) i = i + 1;
        ol_e[ol_n] = i; ol_n = ol_n + 1;
        i = i + 1;
    }
    ol_from_ok = em_gen == ol_gen - 1 && em_gen > 0;
    k = 0; i = 0;
    while (i < ol_n) {
        ol_from[i] = 0 - 1;
        if (ol_from_ok && k < em_n && em_pos[k] == ol_s[i]) { ol_from[i] = em_src[k]; k = k + 1; }
        i = i + 1;
    }
    em_n = 0; em_gen = ol_gen;
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
int pk_acls(int l) {                  /* index into BF_PEEP_0, from opinfo */
    int i;
    i = ol_opi(l);
    if (i < 0) return vfind(BF_PEEP_0, NBF_PEEP_0, "other", 5);
    return oi_acls[i];
}
int pk_bcls(int l) {                  /* index into BF_PEEP_1, from opinfo */
    int i;
    if (l >= ol_n) return vfind(BF_PEEP_1, NBF_PEEP_1, "none", 4);
    i = ol_opi(l);
    if (i < 0) return vfind(BF_PEEP_1, NBF_PEEP_1, "other", 5);
    return oi_bcls[i];
}
int pk_islab(int l) { return out[ol_s[l]] != 32 && out[ol_s[l]] != 46 && ol_len(l) > 1 && out[ol_e[l] - 1] == 58; }
int pk_real(int l) {                  /* the first line at or after l that is not a label */
    while (l < ol_n && pk_islab(l)) l = l + 1;
    return l;
}
int pk_lastword(int l) {              /* where the line's last token starts */
    int p; p = ol_e[l]; while (p > ol_s[l] && out[p - 1] != 32) p = p - 1; return p;
}
int pk_same(int p, int e, int q, int f) {    /* out[p..e) == out[q..f) */
    if (e - p != f - q) return 0;
    while (p < e) { if (out[p] != out[q]) return 0; p = p + 1; q = q + 1; }
    return 1;
}
/* `  store64 [M], rX` -> X, M at pk_m0..pk_m1; else -1 */
int pk_m0; int pk_m1; int pk_n0; int pk_n1;
int pk_store(int l) {
    int p; int e; int q;
    if (ol_pre(l, "  store64 [") == 0) return 0 - 1;
    p = ol_s[l] + 11; e = ol_e[l]; q = p;
    while (q < e && out[q] != 93) q = q + 1;
    if (q + 3 >= e || out[q + 1] != 44 || out[q + 2] != 32) return 0 - 1;
    pk_m0 = p; pk_m1 = q;
    return ol_reg(q + 3, e);
}
/* `  load64 rY, [M]` -> Y, M at pk_n0..pk_n1 */
int pk_load(int l) {
    int p; int e; int q; int y;
    if (ol_pre(l, "  load64 ") == 0) return 0 - 1;
    p = ol_s[l] + 9; e = ol_e[l]; q = p;
    while (q < e && out[q] != 44) q = q + 1;
    y = ol_reg(p, q);
    if (y < 0 || q + 3 >= e || out[q + 1] != 32 || out[q + 2] != 91 || out[e - 1] != 93) return 0 - 1;
    pk_n0 = q + 3; pk_n1 = e - 1;
    return y;
}
/* the same store shape as the SECOND of a pair: M at pk_n0..pk_n1 */
int pk_store2(int l) {
    int y; int a; int b;
    a = pk_m0; b = pk_m1;
    y = pk_store(l);
    pk_n0 = pk_m0; pk_n1 = pk_m1; pk_m0 = a; pk_m1 = b;
    return y;
}
/* `  imm rK, N` -> K, N in pk_imm (decimal digits only, at most 18) */
long pk_imm;
int pk_immat(int l) {
    int p; int e; int q; int k; long v;
    if (ol_pre(l, "  imm r") == 0) return 0 - 1;
    p = ol_s[l] + 6; e = ol_e[l]; q = p;
    while (q < e && out[q] != 44) q = q + 1;
    k = ol_reg(p, q);
    if (k < 0 || q + 2 >= e || out[q + 1] != 32) return 0 - 1;
    q = q + 2; v = 0;
    if (e - q > 18) return 0 - 1;
    while (q < e) { if (isdi(out[q] & 255) == 0) return 0 - 1; v = v * 10 + out[q] - 48; q = q + 1; }
    pk_imm = v;
    return k;
}
/* `  OP rD, rS, rT` -> 1, with the three in pk_d pk_sr pk_t */
int pk_d; int pk_sr; int pk_t;
int pk_three(int l) {
    int p; int e; int q;
    if (l >= ol_n || out[ol_s[l]] != 32) return 0;
    p = ol_s[l] + 2; e = ol_e[l];
    while (p < e && out[p] != 32) p = p + 1;
    p = p + 1; q = p; while (q < e && out[q] != 44) q = q + 1;
    pk_d = ol_reg(p, q); if (pk_d < 0 || q + 2 >= e) return 0;
    p = q + 2; q = p; while (q < e && out[q] != 44) q = q + 1;
    pk_sr = ol_reg(p, q); if (pk_sr < 0 || q + 2 >= e) return 0;
    pk_t = ol_reg(q + 2, e); if (pk_t < 0) return 0;
    return 1;
}
/* `  mov rY, rX` -> 1, Y and X in pk_d, pk_sr */
int pk_movat(int l) {
    int p; int e; int q;
    if (ol_pre(l, "  mov r") == 0) return 0;
    p = ol_s[l] + 6; e = ol_e[l]; q = p;
    while (q < e && out[q] != 44) q = q + 1;
    pk_d = ol_reg(p, q); if (pk_d < 0 || q + 2 >= e) return 0;
    pk_sr = ol_reg(q + 2, e);
    return pk_sr >= 0;
}
int pk_rel(char *nm) { return vfind(BF_PEEP_2, NBF_PEEP_2, nm, blen(nm)); }
/* an action's class index, by name -- asked for every candidate, so the
   answers (fixed for the table in hand) are kept by the name's address [J5] */
char *pa_nm[16]; int pa_v[16]; int pa_n;
int pk_act(char *nm) {
    int n; int k;
    k = 0; while (k < pa_n) { if (pa_nm[k] == nm) return pa_v[k]; k = k + 1; }
    n = 0; while (nm[n]) n = n + 1;
    n = vfind(BH_PEEP_Y, NBH_PEEP_Y, nm, n);
    if (pa_n < 16) { pa_nm[pa_n] = nm; pa_v[pa_n] = n; pa_n = pa_n + 1; }
    return n;
}
/* emit line l with register a written as b: every token (all) or only the
   first -- the destination (all == 0) [H4] */
int pk_rereg(int l, int a, int b, int all) {
    int p; int e; int n; int q; int done;
    p = ol_s[l]; e = ol_e[l]; done = 0;
    while (p < e) {
        if (done == 0 && out[p] == 114 && isal(out[p - 1] & 255) == 0 && p + 1 < e && isdi(out[p + 1] & 255)) {
            q = p + 1; n = 0;
            while (q < e && isdi(out[q] & 255)) { n = n * 10 + out[q] - 48; q = q + 1; }
            if (n == a) { pk_emitreg(b); p = q; if (all == 0) done = 1; continue; }
            ol_put(p, q); p = q;
            if (all == 0) done = 1;       /* the first register was not a */
            continue;
        }
        out2[nout2] = out[p]; nout2 = nout2 + 1; p = p + 1;
    }
    pk_nl();
    return 0;
}
/* -O2: a local's store, the mirror of ol_local [H4].  The walker writes
       imm r2, N / sub64 rA, r6, r2 / S... / .st [rA+0], rV, W
   and with rA and r2 dead after the store (S straight-line, naming neither)
   that is S... / .st [r6-N], rV, W -- two instructions fewer per store.
   Returns the line after the store, having written the result; 0 if not. */
int pk_stfuse(int i) {
    int k; int a; int j; int q; int p; int e; int v; int st; int n0; int n1;
    if (i + 3 >= ol_n) return 0;
    k = ol_pre(i, "  imm r2, "); if (k == 0) return 0;
    n0 = ol_s[i] + k; n1 = ol_e[i];
    if (n0 >= n1) return 0;
    q = n0; while (q < n1) { if (isdi(out[q] & 255) == 0) return 0; q = q + 1; }
    if (ol_pre(i + 1, "  sub64 r") == 0 || ol_len(i + 1) != 18) return 0;
    q = ol_s[i + 1];
    a = out[q + 9] - 48;
    if (a < 0 || a > 5 || a == 2) return 0;
    if (out[q + 10] != 44 || out[q + 12] != 114 || out[q + 13] != 54 || out[q + 16] != 114 || out[q + 17] != 50) return 0;
    j = i + 2;
    while (j < ol_n && j < i + 11) {
        st = 0;
        if (ol_pre(j, "  .st [r")) st = 1;
        else if (ol_pre(j, "  store64 [r")) st = 2;
        if (st) {
            p = ol_s[j] + (st == 1 ? 8 : 12); e = ol_e[j];
            if (out[p] - 48 != a || out[p + 1] != 43 || out[p + 2] != 48 || out[p + 3] != 93) return 0;
            if (out[p + 4] != 44 || out[p + 5] != 32 || out[p + 6] != 114) return 0;
            q = p + 7; v = 0;
            if (q >= e || isdi(out[q] & 255) == 0) return 0;
            while (q < e && isdi(out[q] & 255)) { v = v * 10 + out[q] - 48; q = q + 1; }
            if (v == a || v == 2) return 0;
            if (st == 2 && q != e) return 0;
            if (st == 1 && (q >= e || out[q] != 44)) return 0;
            if (ol_dead(a, j + 1) == 0 || ol_dead(2, j + 1) == 0) return 0;
            k = i + 2; while (k < j) { ol_emit(k); k = k + 1; }
            if (st == 1) ol_puts("  .st [r6-"); else ol_puts("  store64 [r6-");
            ol_put(n0, n1); ol_puts("], ");
            pk_emitreg(v);
            if (st == 1) ol_put(q, e);
            pk_nl();
            return j + 1;
        }
        if (ol_simple(j) == 0) return 0;
        if (ol_names(j, a) || ol_names(j, 2)) return 0;
        j = j + 1;
    }
    return 0;
}
int pk_asks;
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
    none = pk_rel("none");
    nout2 = 0; hits = 0; i = 0; n = ol_n;
    while (i < n) {
        if (out[ol_s[i]] != 32) { ol_emit(i); i = i + 1; continue; }
        k = pk_stfuse(i);
        if (k > 0) { hits = hits + 1; i = k; continue; }
        a = pk_acls(i); b = pk_bcls(i + 1); rel = none;
        t = 0 - 1; x = 0 - 1; y = 0 - 1; u = 0; v = 0;
        if (ol_word(i, "jump") || ol_word(i, "jumpz")) {
            p = pk_lastword(i); e = ol_e[i];
            k = i + 1;
            while (k < n && pk_islab(k)) {
                if (pk_same(ol_s[k], ol_e[k] - 1, p, e)) u = 1;
                k = k + 1;
            }
            if (u) { rel = pk_rel("to_next"); b = pk_bcls(pk_real(i + 1)); }
            else {
                t = ol_lab(p, e);
                if (t >= 0) {
                    t = pk_real(t + 1);
                    if (t < n && ol_word(t, "jump")) {
                        q = pk_lastword(t);
                        if (pk_same(q, ol_e[t], p, e) == 0) { rel = pk_rel("to_jump"); b = pk_bcls(t); }
                    }
                }
            }
        } else if (i + 1 < n && pk_store(i) >= 0) {
            x = pk_store(i);
            y = pk_load(i + 1);
            if (y < 0) y = pk_store2(i + 1);
            /* not the expression stack's own slot: [r7+..] is what B1 turns
               into a real push and pop on x86, and a pop made a mov is a
               pop that costs more */
            if (y >= 0 && pk_same(pk_m0, pk_m1, pk_n0, pk_n1) && (out[pk_m0] != 114 || out[pk_m0 + 1] != 55)) {
                if (x == y) rel = pk_rel("same_slot_same_reg"); else rel = pk_rel("same_slot");
            }
        } else if (i + 1 < n && pk_immat(i) >= 0) {
            x = pk_immat(i); v = pk_imm;
            if (x <= 5 && ol_word(i + 1, "mov") == 0 && pk_three(i + 1)) {
                if (pk_t == x && pk_sr != x && ol_dead(x, i + 2)) {
                    if (v == 0) rel = pk_rel("const0");
                    else if (v == 1) rel = pk_rel("const1");
                    else if ((v & (v - 1)) == 0) rel = pk_rel("pow2");
                }
            } else if (x <= 5 && pk_movat(i + 1)) {
                if (pk_sr == x && pk_d != x && ol_dead(x, i + 2)) rel = pk_rel("copy_dead");
            }
        }
        if (rel == none && i + 1 < n) {            /* [H4] */
            int ya; int xa; int yb; int db;
            if (pk_movat(i)) {
                ya = pk_d; xa = pk_sr;
                if (ya <= 5 && ya != xa && ol_k[i + 1] == OK_SIMPLE
                    && (ol_rm[i + 1] & (1 << ya)) && (ol_wm[i + 1] & (1 << ya)) == 0
                    && ol_dead(ya, i + 2)) { rel = pk_rel("copy_into"); y = ya; x = xa; }
            }
            if (rel == none && ol_k[i] == OK_SIMPLE && ol_word(i, "store64") == 0 && ol_word(i, ".st") == 0) {
                db = ol_firstreg(i);
                if (db >= 0 && db <= 5 && (ol_wm[i] & (1 << db)) && pk_movat(i + 1)) {
                    yb = pk_d;
                    if (pk_sr == db && yb != db && ol_dead(db, i + 2)) { rel = pk_rel("dest_to_mov"); x = db; y = yb; }
                }
            }
        }
        if (rel == none) {
            if (ol_k[i] == OK_SIMPLE && ol_word(i, "store64") == 0 && ol_word(i, ".st") == 0) {
                d = ol_firstreg(i);
                if (d >= 0 && d <= 5) { if ((ol_wm[i] & (1 << d)) && ol_dead(d, i + 1)) rel = pk_rel("a_dead"); }
            }
        }
        if (rel == none) { ol_emit(i); i = i + 1; continue; }
        key[0] = a; key[1] = b; key[2] = rel; key[3] = 0;
        act = inf(S_PEEP, key, 0); pk_asks = pk_asks + 1;
        if (act == pk_act("keep")) { ol_emit(i); i = i + 1; continue; }
        hits = hits + 1;
        if (act == pk_act("load_to_mov")) {
            ol_emit(i);
            ol_puts("  mov "); pk_emitreg(y); ol_puts(", "); pk_emitreg(x); pk_nl();
            i = i + 2; continue;
        }
        if (act == pk_act("drop_b")) { ol_emit(i); i = i + 2; continue; }
        if (act == pk_act("drop_a")) { i = i + 1; continue; }
        if (act == pk_act("retarget")) {
            p = pk_lastword(i); ol_put(ol_s[i], p);
            q = pk_lastword(t); ol_put(q, ol_e[t]); pk_nl();
            i = i + 1; continue;
        }
        if (act == pk_act("to_mov")) {
            ol_puts("  mov "); pk_emitreg(pk_d); ol_puts(", "); pk_emitreg(pk_sr); pk_nl();
            i = i + 2; continue;
        }
        if (act == pk_act("to_shl")) {
            lg = 0; while (v > 1) { v = v / 2; lg = lg + 1; }
            ol_puts("  imm "); pk_emitreg(x); ol_puts(", "); pk_emitnum(lg); pk_nl();
            ol_puts("  shl64 "); pk_emitreg(pk_d); ol_puts(", "); pk_emitreg(pk_sr); ol_puts(", "); pk_emitreg(x); pk_nl();
            i = i + 2; continue;
        }
        if (act == pk_act("retarget_dest")) {       /* A writes rY; B goes */
            pk_rereg(i, x, y, 0);
            i = i + 2; continue;
        }
        if (act == pk_act("fold_copy")) {           /* A goes; B reads rX */
            pk_rereg(i + 1, y, x, 1);
            i = i + 2; continue;
        }
        if (act == pk_act("fold_imm")) {
            ol_puts("  imm "); pk_emitreg(pk_d); ol_puts(", "); pk_emitnum(pk_imm); pk_nl();
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
