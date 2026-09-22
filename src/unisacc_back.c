/* ==== the back end: tape -> lowered program -> machine code -> image ======
   The self-hosting closure [S-6] [S-7 item 7].  This is unisa/lower.py,
   assemble.py, emit_arm.py, emit_x86.py and image/{elf,macho,pe}.py, carried
   into the subset, so unisacc can go from C to an executable with no Python
   at all.  It is a PORT: every byte must be the one the Python back end
   writes for the same tape, and tests/closure.sh checks exactly that, for
   every probe on every target.

   What is table-shaped is still asked of the nets, as lower.py asks them:
   isel (op, arch) -> symbol, abi (op, os, arch) -> sysno, argument and
   return registers, gate, and enc (op, os, arch) -> form, which wins.  The
   rest -- register maps, instruction bytes, header layouts, the relocation
   arithmetic -- is structure [T-1], and is classic code here as there.

   Zero data is never held: a `.bss` is a length, and the image writer emits
   its zeros as it goes.  A compiler that stored its own zero-filled globals
   could not compile itself -- the buffer would be part of what it stores. */

#define BK_DATA_BASE 256            /* tape.DATA_BASE */
#define BK_MAXI 262144              /* tape instructions */
#define BK_MAXT 524288              /* lowered instructions */
#define BK_MAXN 131072              /* names: labels and data symbols */
#define BK_NPOOL 2097152            /* their spellings */
#define BK_MAXDATA 4194304          /* initialised data bytes (zeros are not stored) */
#define BK_MAXZ 65536               /* zero runs */
#define BK_MAXTEXT 8388608          /* machine code */

/* ---- names ------------------------------------------------------------- */
char bkpool[BK_NPOOL]; int bkpoolend;
int bkname_at[BK_MAXN]; int bkname_len[BK_MAXN]; int bknn;
int bksym_addr[BK_MAXN];            /* a data symbol's address, or -1 */
int bklab_pc[BK_MAXN];              /* a code label's tape pc, or -1 */
int bkhash[262144];                 /* open addressing: name id + 1, or 0 */

int bk_hashof(char *s, int n) {
    unsigned long h; int k;
    h = 5381; k = 0;
    while (k < n) { h = (h * 33 + (s[k] & 255)) & 4294967295; k = k + 1; }
    return h & 262143;
}
int bk_same(int id, char *s, int n) {
    int k;
    if (bkname_len[id] != n) return 0;
    k = 0;
    while (k < n) { if (bkpool[bkname_at[id] + k] != s[k]) return 0; k = k + 1; }
    return 1;
}
/* the id of a name, made if new */
int bk_name(char *s, int n) {
    int h; int id; int k;
    h = bk_hashof(s, n);
    while (bkhash[h]) {
        if (bk_same(bkhash[h] - 1, s, n)) return bkhash[h] - 1;
        h = (h + 1) & 262143;
    }
    if (bknn >= BK_MAXN || bkpoolend + n >= BK_NPOOL) { __write(2, "back end: too many names\n", 25); __exit(1); }
    id = bknn; bknn = bknn + 1;
    bkname_at[id] = bkpoolend; bkname_len[id] = n;
    k = 0; while (k < n) { bkpool[bkpoolend] = s[k]; bkpoolend = bkpoolend + 1; k = k + 1; }
    bksym_addr[id] = 0 - 1; bklab_pc[id] = 0 - 1;
    bkhash[h] = id + 1;
    return id;
}
int bk_find(char *s, int n) {       /* -1 when absent */
    int h;
    h = bk_hashof(s, n);
    while (bkhash[h]) {
        if (bk_same(bkhash[h] - 1, s, n)) return bkhash[h] - 1;
        h = (h + 1) & 262143;
    }
    return 0 - 1;
}

/* ---- the tape's data: initialised bytes, and runs of zeros ------------- */
char bkdata[BK_MAXDATA];
long bkdlen;                        /* the data's full length, zeros included */
int bkstored;                       /* how much of bkdata is in use */
long bkz_at[BK_MAXZ]; long bkz_len[BK_MAXZ]; int bknz;
long bkd_at[BK_MAXZ]; int bkd_off[BK_MAXZ]; int bkd_len[BK_MAXZ]; int bknd;

int bk_zeros(long n) {
    if (n <= 0) return 0;
    if (bknz > 0) { if (bkz_at[bknz - 1] + bkz_len[bknz - 1] == bkdlen) {
        bkz_len[bknz - 1] = bkz_len[bknz - 1] + n; bkdlen = bkdlen + n; return 0; } }
    if (bknz >= BK_MAXZ) { __write(2, "back end: too many zero runs\n", 29); __exit(1); }
    bkz_at[bknz] = bkdlen; bkz_len[bknz] = n; bknz = bknz + 1;
    bkdlen = bkdlen + n;
    return 0;
}
int bk_bytes(char *b, int n) {
    int k;
    if (n <= 0) return 0;
    if (bkstored + n >= BK_MAXDATA || bknd >= BK_MAXZ) { __write(2, "back end: data too large\n", 25); __exit(1); }
    bkd_at[bknd] = bkdlen; bkd_off[bknd] = bkstored; bkd_len[bknd] = n; bknd = bknd + 1;
    k = 0; while (k < n) { bkdata[bkstored + k] = b[k]; k = k + 1; }
    bkstored = bkstored + n; bkdlen = bkdlen + n;
    return 0;
}

/* ---- the tape's code -------------------------------------------------- */
/* operand kinds */
#define BK_R 1                      /* a tape register, value 0..7 */
#define BK_I 2                      /* an immediate */
#define BK_N 3                      /* a name: label or data symbol */
int bkop[BK_MAXI];
int bkak[BK_MAXI * 4]; long bkav[BK_MAXI * 4];
int bkni;
int bkentry;                        /* the _start label's pc */

/* the tape's op names, in one packed list; an op is its index here */
char *BKOPS = "imm\000mov\000add64\000sub64\000mul64\000xor64\000and64\000or64\000shl64\000shr64\000lshr64\000.div\000.mod\000.udiv\000.umod\000slt64\000sle64\000ult64\000ule64\000eq\000ne\000load64\000store64\000.ld\000.st\000.lea\000.zero\000jump\000jumpz\000call\000callr\000ret\000.frame\000.arg\000.print\000.write\000.exit\000.sys\000.argc\000.argv\000nop\000fadd64\000fsub64\000fmul64\000fdiv64\000flt64\000fle64\000feq64\000fadd32\000fsub32\000fmul32\000fdiv32\000flt32\000fle32\000feq32\000cvtid\000cvtud\000cvtis\000cvtus\000cvtdi\000cvtdu\000cvtsd\000cvtds\000fsqrt64\000fsqrt32\000";
#define BKNOPS 65
/* operand shapes, one char per operand: r i L s */
char *BKSHAPE = "ri\000rr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rri\000rir\000rrii\000riri\000rs\000rii\000L\000rL\000L\000r\000\000i\000ir\000r\000rr\000r\000srrr\000r\000rr\000\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rr\000rr\000rr\000rr\000rr\000rr\000rr\000rr\000rr\000rr\000";

/* the i-th entry of a packed list: its start */
char *bk_nth(char *list, int i) {
    int k; k = 0;
    while (i > 0) { while (list[k]) k = k + 1; k = k + 1; i = i - 1; }
    return list + k;
}
int bk_opof(char *s, int n) {
    int i; int k; char *e;
    i = 0;
    while (i < BKNOPS) {
        e = bk_nth(BKOPS, i); k = 0;
        while (k < n) { if (e[k] != s[k]) break; k = k + 1; }
        if (k == n) { if (e[n] == 0) return i; }
        i = i + 1;
    }
    return 0 - 1;
}
int bk_is(int op, char *nm) {       /* is op the tape op spelled nm? */
    char *e; int k;
    e = bk_nth(BKOPS, op); k = 0;
    while (nm[k]) { if (e[k] != nm[k]) return 0; k = k + 1; }
    return e[k] == 0;
}

/* ---- parse the tape text ---------------------------------------------- */
long bk_num(char *s, int n) {       /* int(tok, 0): decimal, 0x, a sign */
    long v; int k; int neg; int c; int base;
    v = 0; k = 0; neg = 0; base = 10;
    if (k < n) { if (s[k] == 45) { neg = 1; k = k + 1; } else { if (s[k] == 43) k = k + 1; } }
    if (k + 1 < n) { if (s[k] == 48) { if ((s[k + 1] | 32) == 120) { base = 16; k = k + 2; } } }
    while (k < n) {
        c = s[k] & 255;
        if (c >= 48 && c <= 57) c = c - 48;
        else { if ((c | 32) >= 97 && (c | 32) <= 102) c = (c | 32) - 87; else break; }
        v = v * base + c; k = k + 1;
    }
    if (neg) v = 0 - v;
    return v;
}
int bk_hex(int c) { if (c >= 48 && c <= 57) return c - 48; return (c | 32) - 87; }
int bk_isreg(char *s, int n) {
    if (n == 2) { if (s[0] == 114) { if (s[1] >= 48 && s[1] <= 55) return s[1] - 48 + 1; } }
    return 0;
}
char bkstrbuf[1048576];
int bk_parse(char *t, int n) {
    int i; int e; int j; int k; int op; int na; int s0; int s1; char *sh; int id;
    i = 0; bkni = 0; bkentry = 0 - 1;
    while (i < n) {
        e = i; while (e < n) { if (t[e] == 10) break; e = e + 1; }
        j = i; while (j < e) { if (t[j] != 32 && t[j] != 9) break; j = j + 1; }
        if (j >= e) { i = e + 1; continue; }
        if (t[j] == 46 && t[j + 1] == 98 && t[j + 2] == 115 && t[j + 3] == 115 && t[j + 4] == 32) {
            /* .bss NAME N: zeros, 8-aligned */
            long cnt;
            j = j + 5; s0 = j; while (t[j] != 32) j = j + 1; s1 = j;
            cnt = bk_num(t + j + 1, e - j - 1);
            bk_zeros((8 - bkdlen % 8) % 8);
            id = bk_name(t + s0, s1 - s0);
            bksym_addr[id] = BK_DATA_BASE + bkdlen;
            bk_zeros(cnt);
            i = e + 1; continue;
        }
        if (t[j] == 46 && t[j + 1] == 115 && t[j + 2] == 116 && t[j + 3] == 114 && t[j + 4] == 32) {
            /* .str NAME "..." */
            int m;
            j = j + 5; s0 = j; while (t[j] != 32) j = j + 1; s1 = j;
            while (t[j] != 34) j = j + 1;
            j = j + 1; m = 0;
            while (j < e) {
                if (t[j] == 34) break;
                if (t[j] == 92) {
                    int c; c = t[j + 1];
                    if (c == 120) { bkstrbuf[m] = bk_hex(t[j + 2]) * 16 + bk_hex(t[j + 3]); j = j + 4; }
                    else {
                        if (c == 110) bkstrbuf[m] = 10;
                        else { if (c == 116) bkstrbuf[m] = 9; else { if (c == 114) bkstrbuf[m] = 13;
                        else { if (c == 48) bkstrbuf[m] = 0; else bkstrbuf[m] = c; } } }
                        j = j + 2;
                    }
                    m = m + 1; continue;
                }
                bkstrbuf[m] = t[j]; m = m + 1; j = j + 1;
            }
            id = bk_name(t + s0, s1 - s0);
            if (bksym_addr[id] < 0) {
                bksym_addr[id] = BK_DATA_BASE + bkdlen;
                bk_bytes(bkstrbuf, m);
            }
            i = e + 1; continue;
        }
        if (t[e - 1] == 58) {               /* NAME: */
            id = bk_name(t + j, e - 1 - j);
            bklab_pc[id] = bkni;
            if (e - 1 - j == 6) { if (bk_same(id, "_start", 6)) bkentry = bkni; }
            i = e + 1; continue;
        }
        /* an instruction: the op, then operands split on space , [ ] and +/- in [r+K] */
        s0 = j; while (j < e && t[j] != 32) j = j + 1;
        op = bk_opof(t + s0, j - s0);
        if (op < 0) { __write(2, "back end: unknown tape op\n", 26); __exit(1); }
        if (bkni >= BK_MAXI) { __write(2, "back end: tape too long\n", 24); __exit(1); }
        sh = bk_nth(BKSHAPE, op);
        na = 0;
        while (sh[na]) {
            int q;
            while (j < e && (t[j] == 32 || t[j] == 44 || t[j] == 91 || t[j] == 93)) j = j + 1;
            q = j;
            if (sh[na] == 114) {             /* a register: r0..r7 */
                bkak[bkni * 4 + na] = BK_R; bkav[bkni * 4 + na] = t[j + 1] - 48;
                j = j + 2;
            } else { if (sh[na] == 105) {    /* an immediate, maybe +K/-K after a register */
                while (j < e && t[j] != 32 && t[j] != 44 && t[j] != 93) j = j + 1;
                bkak[bkni * 4 + na] = BK_I; bkav[bkni * 4 + na] = bk_num(t + q, j - q);
            } else {                         /* L or s: a name, or a number */
                while (j < e && t[j] != 32 && t[j] != 44) j = j + 1;
                if ((t[q] >= 48 && t[q] <= 57) || t[q] == 45) {
                    bkak[bkni * 4 + na] = BK_I; bkav[bkni * 4 + na] = bk_num(t + q, j - q);
                } else {
                    bkak[bkni * 4 + na] = BK_N; bkav[bkni * 4 + na] = bk_name(t + q, j - q);
                }
            } }
            na = na + 1;
        }
        bkop[bkni] = op;
        bkni = bkni + 1;
        i = e + 1;
    }
    if (bkentry < 0) bkentry = 0;
    return bkni;
}

/* ---- facts from the nets, as lower.facts() asks them ------------------- */
int bkos; int bkarch;               /* 0 lnx 1 osx 2 win; 0 x86_64 1 arm64 */
int bkf_form; int bkf_gate; long bkf_sysno; int bkf_hasno;
int bkf_arg[3]; int bkf_ret;        /* machine register numbers, -1 none */
int bkf_sym;                        /* the symbol class index (isel), unused in bytes */

/* a class index's spelling -> a machine register number (-1 for none) */
int bk_regnum(char *nm) {
    char *x86; int i; int k; char *e;
    if (nm[0] == 120) {               /* x0..x8 */
        return bk_num(nm + 1, 2);
    }
    x86 = "rax\000rcx\000rdx\000rbx\000rsp\000rbp\000rsi\000rdi\000r8\000r9\000r10\000r11\000r12\000r13\000r14\000r15\000";
    i = 0;
    while (i < 16) {
        e = bk_nth(x86, i); k = 0;
        while (e[k] && nm[k] == e[k]) k = k + 1;
        if (e[k] == 0 && nm[k] == 0) return i;
        i = i + 1;
    }
    return 0 - 1;                     /* none */
}
/* ask isel, abi and enc for catalog op `cop` (an index into BF_ABI_0) */
int bk_facts(int cop) {
    int key[4]; int c; char *nm;
    key[0] = cop; key[1] = bkarch; key[2] = 0; key[3] = 0;
    bkf_sym = inf(S_ISEL, key, HD_ISEL_SYMBOL);
    key[0] = cop; key[1] = bkos; key[2] = bkarch; key[3] = 0;
    c = inf(S_ABI, key, HD_ABI_SYSNO);
    nm = bk_nth(BH_ABI_SYSNO, c);
    bkf_hasno = 1;
    if (nm[0] == 110) bkf_hasno = 0;  /* none */
    else bkf_sysno = bk_num(nm, 64);
    bkf_arg[0] = bk_regnum(bk_nth(BH_ABI_ARG0, inf(S_ABI, key, HD_ABI_ARG0)));
    bkf_arg[1] = bk_regnum(bk_nth(BH_ABI_ARG1, inf(S_ABI, key, HD_ABI_ARG1)));
    bkf_arg[2] = bk_regnum(bk_nth(BH_ABI_ARG2, inf(S_ABI, key, HD_ABI_ARG2)));
    bkf_ret = bk_regnum(bk_nth(BH_ABI_RET, inf(S_ABI, key, HD_ABI_RET)));
    bkf_gate = inf(S_ABI, key, HD_ABI_GATE);
    bkf_form = inf(S_ENC, key, 0);        /* enc is os-aware, and wins */
    return 0;
}
int bk_cop(char *nm) {                  /* a catalog op's index */
    int i; int k; char *e;
    i = 0;
    while (i < NBF_ABI_0) {
        e = bk_nth(BF_ABI_0, i); k = 0;
        while (e[k] && nm[k] == e[k]) k = k + 1;
        if (e[k] == 0 && nm[k] == 0) return i;
        i = i + 1;
    }
    return 0 - 1;
}
int bk_gateis(char *nm) { char *e; int k; e = bk_nth(BH_ABI_GATE, bkf_gate); k = 0;
    while (nm[k]) { if (e[k] != nm[k]) return 0; k = k + 1; } return e[k] == 0; }
int bk_formis(char *nm) { char *e; int k; e = bk_nth(BH_ENC_Y, bkf_form); k = 0;
    while (nm[k]) { if (e[k] != nm[k]) return 0; k = k + 1; } return e[k] == 0; }

/* ---- lowering: lower.py ------------------------------------------------ */
/* A lowered instruction: tape ops keep their index; the target's own are
   numbered past them. */
#define TO_SETREG 100
#define TO_SETMEM 101
#define TO_ITOA 102
#define TO_GATE 103
#define TO_WINSAVE 104
#define TO_WINREST 105
#define TO_WINSTDH 106
#define TO_SPINIT 107
#define TO_ARGSAVE 108
#define TO_ARGVGET 109
/* setreg's source kinds */
#define SK_IMM 1
#define SK_REG 2
#define SK_MEM 3
#define SK_ADDR 4
int tkop[BK_MAXT]; long tka[BK_MAXT * 4]; int tkk[BK_MAXT * 4]; int tkn;
/* gate metadata: form, gate, catalog op, return register */
int tkg_form[BK_MAXT]; int tkg_gate[BK_MAXT]; int tkg_cop[BK_MAXT]; int tkg_ret[BK_MAXT];
int bklab_tpc[BK_MAXN];            /* a label's lowered pc */
int bklab_first[BK_MAXI + 1]; int bklab_next[BK_MAXN];
long bk_scr0; long bk_scr1; long bk_plen; long bk_pbuf; long bk_argc; long bk_argv;
long bk_hstd; long bk_written; long bk_save; long bk_stacktop; long bk_bss;
int bk_rmap[8];                     /* tape register -> machine register */
int bk_nr;                          /* the syscall-number register */

int tk(int op, long a0, long a1, long a2, long a3) {
    if (tkn >= BK_MAXT) { __write(2, "back end: lowered program too long\n", 35); __exit(1); }
    tkop[tkn] = op; tka[tkn * 4] = a0; tka[tkn * 4 + 1] = a1; tka[tkn * 4 + 2] = a2; tka[tkn * 4 + 3] = a3;
    tkk[tkn * 4] = BK_I; tkk[tkn * 4 + 1] = BK_I; tkk[tkn * 4 + 2] = BK_I; tkk[tkn * 4 + 3] = BK_I;
    tkg_form[tkn] = 0; tkg_gate[tkn] = 0; tkg_cop[tkn] = 0 - 1; tkg_ret[tkn] = 0 - 1;
    tkn = tkn + 1;
    return tkn - 1;
}
/* setreg dst, (kind, value) -- the kind rides in a3 */
int tk_setreg(int dst, int kind, long v) { return tk(TO_SETREG, dst, v, 0, kind); }

/* lower.syscall_seq */
int bk_syscall(int cop, int k0, long v0, int k1, long v1, int k2, long v2, int xreg, int xk, long xv) {
    int g;
    bk_facts(cop);
    if (bkf_hasno) tk_setreg(bk_nr, SK_IMM, bkf_sysno);
    if (bkos == 2) tk(TO_WINSAVE, bk_save, 0, 0, 0);
    tk_setreg(bkf_arg[0], k0, v0);
    tk_setreg(bkf_arg[1], k1, v1);
    tk_setreg(bkf_arg[2], k2, v2);
    if (xreg >= 0) tk_setreg(xreg, xk, xv);
    g = tk(TO_GATE, 0, 0, 0, 0);
    tkg_form[g] = bkf_form; tkg_gate[g] = bkf_gate; tkg_cop[g] = cop; tkg_ret[g] = bkf_ret;
    if (bkos == 2) tk(TO_WINREST, bk_save, bkf_ret, 0, 0);
    return 0;
}

int bk_lower(void) {
    long base; int pc; int op; int k; int cw;
    int x86map[8]; int j;
    /* data: pad to 8, then the scratch cells -- and on Windows the save area
       and the tape's own stack (that one bss, not file) */
    bk_zeros((8 - bkdlen % 8) % 8);
    base = BK_DATA_BASE + bkdlen;
    bk_scr0 = base; bk_scr1 = base + 8; bk_plen = base + 16; bk_pbuf = base + 24;
    bk_argc = base + 48; bk_argv = base + 56;
    bk_hstd = base + 104; bk_written = base + 128; bk_save = base + 136;
    bk_stacktop = base + 200 + 65536;
    if (bkos == 2) bk_zeros(200); else bk_zeros(104);
    bk_bss = 0; if (bkos == 2) bk_bss = 65536;
    /* REGMAP: rax rdi rsi rdx rcx r8 r9 r10 / x0..x7 */
    x86map[0] = 0; x86map[1] = 7; x86map[2] = 6; x86map[3] = 2; x86map[4] = 1;
    x86map[5] = 8; x86map[6] = 9; x86map[7] = 10;
    j = 0; while (j < 8) { bk_rmap[j] = bkarch ? j : x86map[j]; j = j + 1; }
    /* NR_REG: rax on x86_64; x8 on lnx/arm64, x16 on osx and win arm64 */
    bk_nr = 0; if (bkarch) { if (bkos == 0) bk_nr = 8; else bk_nr = 16; }
    tkn = 0;
    /* for each tape pc, the labels that point at it: one chain per pc,
       built once -- a scan of every name per instruction is quadratic */
    j = 0; while (j <= bkni) { bklab_first[j] = 0 - 1; j = j + 1; }
    j = 0;
    while (j < bknn) {
        bklab_tpc[j] = 0 - 1;
        if (bklab_pc[j] >= 0) { bklab_next[j] = bklab_first[bklab_pc[j]]; bklab_first[bklab_pc[j]] = j; }
        j = j + 1;
    }
    pc = 0;
    while (pc <= bkni) {
        j = bklab_first[pc];
        while (j >= 0) { bklab_tpc[j] = tkn; j = bklab_next[j]; }
        if (pc == bkni) break;
        if (pc == bkentry) {
            /* bind the tape SP at the ENTRY: a real process has a real stack */
            if (bkos == 2) tk(TO_WINSTDH, bk_hstd, 0, 0, 0);
            tk(TO_SPINIT, bk_rmap[7], bkos == 2 ? bk_stacktop : 0 - 1, 0, 0);
            if (bkos != 2) tk(TO_ARGSAVE, bk_argc, bk_argv, bkos == 0, 0);
        }
        op = bkop[pc];
        if (bk_is(op, ".write")) {
            tk(TO_SETMEM, bk_scr0, bk_rmap[bkav[pc * 4]], 0, 0);
            tk(TO_SETMEM, bk_scr1, bk_rmap[bkav[pc * 4 + 1]], 0, 0);
            bk_syscall(bk_cop("write"), SK_IMM, 1, SK_MEM, bk_scr0, SK_MEM, bk_scr1, 0 - 1, 0, 0);
        } else { if (bk_is(op, ".print")) {
            tk(TO_SETMEM, bk_scr0, bk_rmap[bkav[pc * 4]], 0, 0);
            tk(TO_ITOA, bk_scr0, bk_pbuf, bk_plen, 0);
            bk_syscall(bk_cop("write"), SK_IMM, 1, SK_ADDR, bk_pbuf, SK_MEM, bk_plen, 0 - 1, 0, 0);
        } else { if (bk_is(op, ".sys")) {
            char nm[32]; int id; int L;
            tk(TO_SETMEM, bk_scr0, bk_rmap[bkav[pc * 4 + 1]], 0, 0);
            tk(TO_SETMEM, bk_scr1, bk_rmap[bkav[pc * 4 + 2]], 0, 0);
            tk(TO_SETMEM, bk_plen, bk_rmap[bkav[pc * 4 + 3]], 0, 0);
            id = bkav[pc * 4]; L = bkname_len[id]; if (L > 31) L = 31;
            k = 0; while (k < L) { nm[k] = bkpool[bkname_at[id] + k]; k = k + 1; } nm[L] = 0;
            cw = bk_cop(nm);
            if (bkos == 0 && bkarch == 1 && bk_same(id, "open", 4)) {
                /* Linux/arm64 has no `open`: the number is openat's, whose
                   first argument is a directory fd, AT_FDCWD */
                bk_syscall(cw, SK_IMM, 0 - 100, SK_MEM, bk_scr0, SK_MEM, bk_scr1, 3, SK_MEM, bk_plen);
            } else {
                bk_syscall(cw, SK_MEM, bk_scr0, SK_MEM, bk_scr1, SK_MEM, bk_plen, 0 - 1, 0, 0);
            }
            bk_facts(cw);
            tk(bk_opof("mov", 3), bk_rmap[0], bkf_ret < 0 ? 31 : bkf_ret, 0, 0);
        } else { if (bk_is(op, ".exit")) {
            tk(TO_SETMEM, bk_scr0, bk_rmap[bkav[pc * 4]], 0, 0);
            bk_syscall(bk_cop("exit"), SK_MEM, bk_scr0, SK_IMM, 0, SK_IMM, 0, 0 - 1, 0, 0);
        } else { if (bk_is(op, ".argc")) {
            tk_setreg(bk_rmap[bkav[pc * 4]], SK_MEM, bk_argc);
        } else { if (bk_is(op, ".argv")) {
            tk(TO_ARGVGET, bk_rmap[bkav[pc * 4]], bk_rmap[bkav[pc * 4 + 1]], bk_argv, 0);
        } else { if (bk_is(op, ".arg")) {
            bk_facts(bk_cop("add64"));                /* a plain move */
            tk(bk_opof("mov", 3), bk_rmap[bkav[pc * 4]], bk_rmap[bkav[pc * 4 + 1]], 0, 0);
        } else {
            /* every other op, with its registers mapped; its facts are asked
               as lower.py asks them (jumps also ask reloc) */
            char *sh; int n; int key[4];
            sh = bk_nth(BKSHAPE, op);
            if (bk_is(op, "jump") || bk_is(op, "jumpz") || bk_is(op, "call")) {
                key[0] = bk_is(op, "jump") ? 0 : (bk_is(op, "jumpz") ? 1 : 2); key[1] = bkarch; key[2] = 0; key[3] = 0;
                inf(S_RELOC, key, 0);
                bk_facts(bk_cop(bk_is(op, "call") ? "call" : (bk_is(op, "jumpz") ? "jumpz" : "jump")));
            } else {
                char nm2[16]; char *e; int L2;
                e = bk_nth(BKOPS, op); L2 = 0;
                while (e[L2] && L2 < 15) { nm2[L2] = e[L2]; L2 = L2 + 1; } nm2[L2] = 0;
                if (bk_cop(nm2) >= 0) bk_facts(bk_cop(nm2));
            }
            n = tk(op, 0, 0, 0, 0);
            k = 0;
            while (sh[k]) {
                if (bkak[pc * 4 + k] == BK_R) tka[n * 4 + k] = bk_rmap[bkav[pc * 4 + k]];
                else tka[n * 4 + k] = bkav[pc * 4 + k];
                tkk[n * 4 + k] = bkak[pc * 4 + k];
                k = k + 1;
            }
        } } } } } } }
        pc = pc + 1;
    }
    return tkn;
}

/* ---- encoding, shared ------------------------------------------------- */
char *bkout;                        /* where the current instruction's bytes go */
int bkol;                           /* how many so far */
long bk_textva; long bk_shift; int bk_sizing;
long bk_imp[8];                     /* Windows: the IAT slot of each import */
long toff[BK_MAXT + 1];             /* each lowered instruction's byte offset */

int ob(int b) { bkout[bkol] = b; bkol = bkol + 1; return 0; }
int ow(unsigned long v) {           /* a 32-bit little-endian word */
    ob(v & 255); ob((v >> 8) & 255); ob((v >> 16) & 255); ob((v >> 24) & 255); return 0;
}
int oq(unsigned long v) { ow(v & 0xFFFFFFFF); ow(v >> 32); return 0; }
int bk_str_is(char *a, char *b) {
    int k; k = 0;
    while (a[k] && b[k]) { if (a[k] != b[k]) return 0; k = k + 1; }
    return a[k] == b[k];
}
/* a label's byte offset -- 0 while sizing, as size() passes {k: 0} */
long bk_label(int id) {
    int t;
    if (bk_sizing) return 0;
    t = bklab_tpc[id];
    if (t < 0) return 0;
    return toff[t];
}
/* the address `.lea` names: a data symbol, a code label, or a number */
long bk_leaaddr(int i) {
    int id;
    if (bk_sizing) return 0;
    if (tkk[i * 4 + 1] == BK_N) {
        id = tka[i * 4 + 1];
        if (bksym_addr[id] >= 0) return bksym_addr[id] + bk_shift;
        if (bklab_tpc[id] >= 0) return bk_textva + bk_label(id);
        return 0;
    }
    return tka[i * 4 + 1] + bk_shift;
}

/* ---- arm64: emit_arm.py, line for line ---------------------------------- */
#define A_IP0 16
#define A_IP1 17
unsigned long a_ls(int store, int wd, int scaled) {  /* LDS STS LDU STU */
    if (scaled) {
        if (store) { if (wd == 8) return 0xF9000000; if (wd == 4) return 0xB9000000; if (wd == 2) return 0x79000000; return 0x39000000; }
        if (wd == 8) return 0xF9400000; if (wd == 4) return 0xB9800000; if (wd == 2) return 0x79800000; return 0x39800000;
    }
    if (store) { if (wd == 8) return 0xF8000000; if (wd == 4) return 0xB8000000; if (wd == 2) return 0x78000000; return 0x38000000; }
    if (wd == 8) return 0xF8400000; if (wd == 4) return 0xB8800000; if (wd == 2) return 0x78800000; return 0x38800000;
}
int a_movimm(int d, unsigned long v, int fixed);
int a_mem(int store, int rt, int rn, long off, int wd) {
    long a;
    if (off >= 0) { if (off % wd == 0) { if (off / wd < 4096) {
        ow(a_ls(store, wd, 1) | ((off / wd) << 10) | (rn << 5) | rt); return 0; } } }
    if (off >= 0 - 256) { if (off < 256) {
        ow(a_ls(store, wd, 0) | ((off & 0x1FF) << 12) | (rn << 5) | rt); return 0; } }
    a = off; if (a < 0) a = 0 - a;                   /* past both: via IP0 [I-22] */
    a_movimm(A_IP0, a, 0);
    ow((off < 0 ? 0xCB000000 : 0x8B000000) | (A_IP0 << 16) | (rn << 5) | A_IP0);
    ow(a_ls(store, wd, 1) | (A_IP0 << 5) | rt);
    return 0;
}
int a_movimm(int d, unsigned long v, int fixed) {
    int n; int sh; unsigned long part;
    ow(0xD2800000 | ((v & 0xFFFF) << 5) | d);
    n = 1; sh = 1;
    while (sh <= 3) {
        part = (v >> (16 * sh)) & 0xFFFF;
        if (part || (fixed && n < fixed)) { ow(0xF2800000 | (sh << 21) | (part << 5) | d); n = n + 1; }
        sh = sh + 1;
    }
    return 0;
}
int a_adr(int d, long pc, long target) {
    long imm; imm = target - pc;
    ow(0x10000000 | ((imm & 3) << 29) | (((imm >> 2) & 0x7FFFF) << 5) | d);
    return 0;
}
int a_adrp_add(int d, long pc, long target) {
    long page; long lo12;
    page = (target >> 12) - (pc >> 12);
    lo12 = target & 0xFFF;
    ow(0x90000000 | ((page & 3) << 29) | (((page >> 2) & 0x7FFFF) << 5) | d);
    ow(0x91000000 | (lo12 << 10) | (d << 5) | d);
    return 0;
}
long a_disp(long bytes, int bits) {
    long v; long lim;
    v = bytes >> 2;
    lim = (long)1 << (bits - 1);
    if (v < 0 - lim || v >= lim) { __write(2, "arm64: branch does not fit\n", 27); __exit(1); }
    return v;
}
int a_ldr(int rt, int rn, long off) { ow(0xF9400000 | ((off / 8) << 10) | (rn << 5) | rt); return 0; }
int a_str(int rt, int rn, long off) { ow(0xF9000000 | ((off / 8) << 10) | (rn << 5) | rt); return 0; }
int a_movz(int d, long v) { ow(0xD2800000 | ((v & 0xFFFF) << 5) | d); return 0; }
int a_movn(int d, long v) { ow(0x92800000 | (((0 - v - 1) & 0xFFFF) << 5) | d); return 0; }
int a_callimp(long pc, int k) {
    a_adrp_add(A_IP1, pc, bk_sizing ? 0 : bk_imp[k]);
    a_ldr(A_IP1, A_IP1, 0);
    ow(0xD63F0000 | (A_IP1 << 5));
    return 0;
}
int a_fd2handle(long pc, long hstd) {
    ow(0xF1000C1F);                                  /* cmp x0, #3 */
    ow(0x54000002 | (4 << 5));                       /* b.hs over 12 bytes (+1) */
    a_adrp_add(A_IP0, pc + 8, hstd);
    ow(0xF8607800 | (0 << 16) | (A_IP0 << 5) | 0);   /* ldr x0, [ip0, x0, lsl 3] */
    return 0;
}
int a_winapi(int i, long off) {
    long pc; long hstd; long written; char *nm; int s;
    pc = bk_textva + off;
    hstd = bk_hstd + bk_shift; written = bk_written + bk_shift;
    nm = bk_nth(BF_ABI_0, tkg_cop[i]);
    s = bkol;
    if (bk_str_is(nm, "exit")) { a_callimp(pc, 5); return 1; }
    if (bk_str_is(nm, "write") || bk_str_is(nm, "read")) {
        a_fd2handle(pc, hstd);
        a_adrp_add(3, pc + (bkol - s), written);
        ow(0xAA1F03E4);                              /* mov x4, xzr */
        a_callimp(pc + (bkol - s), bk_str_is(nm, "write") ? 1 : 2);
        a_adrp_add(A_IP0, pc + (bkol - s), written);
        a_ldr(0, A_IP0, 0);
        return 1;
    }
    if (bk_str_is(nm, "close")) {
        a_fd2handle(pc, hstd);
        a_callimp(pc + (bkol - s), 3);
        return 1;
    }
    if (bk_str_is(nm, "open")) {
        ow(0xAA0203E4);                              /* x4 = x2 */
        a_movz(2, 3);
        ow(0xAA1F03E3);
        a_movz(5, 0x80);
        ow(0xAA1F03E6);
        a_callimp(pc + (bkol - s), 4);
        return 1;
    }
    bkol = s;
    return 0;
}
int a_itoa(long pc, long src, long buf, long lenp) {
    int s; s = bkol;
    a_adrp_add(9, pc, src); a_ldr(10, 9, 0);
    a_movz(11, 0);
    ow(0xF100001F | (10 << 5));
    ow(0x54000000 | (3 << 5) | 0xA);
    ow(0xCB000000 | (10 << 16) | (31 << 5) | 10);
    a_movz(11, 1);
    a_movz(12, 10);
    ow(0xAA0003E0 | (10 << 16) | 13);
    a_movz(14, 0);
    ow(0x9AC00800 | (12 << 16) | (13 << 5) | 13);
    ow(0x91000400 | (14 << 5) | 14);
    ow(0xB5000000 | ((((long)0 - 2) & 0x7FFFF) << 5) | 13);
    ow(0x8B000000 | (11 << 16) | (14 << 5) | 14);
    a_adrp_add(9, pc + (bkol - s), lenp); a_str(14, 9, 0);
    a_adrp_add(9, pc + (bkol - s), buf);
    ow(0x8B000000 | (14 << 16) | (9 << 5) | 13);
    ow(0x9AC00800 | (12 << 16) | (10 << 5) | 15);
    ow(0x9B008000 | (12 << 16) | (10 << 10) | (15 << 5) | 17);
    ow(0x91000000 | (48 << 10) | (17 << 5) | 17);
    ow(0xD1000400 | (13 << 5) | 13);
    ow(0x39000000 | (13 << 5) | 17);
    ow(0xAA0003E0 | (15 << 16) | 10);
    ow(0xB5000000 | ((((long)0 - 6) & 0x7FFFF) << 5) | 10);
    ow(0xB4000000 | (3 << 5) | 11);
    a_movz(17, 45);
    ow(0x39000000 | (9 << 5) | 17);
    return 0;
}
/* floating point: through v16/v17 */
int a_fmov_to(int v, int x, int dbl) { ow((dbl ? 0x9E670000 : 0x1E270000) | (x << 5) | v); return 0; }
int a_fmov_from(int x, int v, int dbl) { ow((dbl ? 0x9E660000 : 0x1E260000) | (v << 5) | x); return 0; }
int a_fp(char *o, long *a) {
    int dbl; unsigned long ty; unsigned long fa; int fc;
    fa = 0; fc = 0 - 1;
    if (o[0] == 102 && o[1] == 97) fa = 0x1E202800;               /* fadd */
    if (o[0] == 102 && o[1] == 115 && o[2] == 117) fa = 0x1E203800;  /* fsub */
    if (o[0] == 102 && o[1] == 109) fa = 0x1E200800;              /* fmul */
    if (o[0] == 102 && o[1] == 100) fa = 0x1E201800;              /* fdiv */
    if (o[0] == 102 && o[1] == 108 && o[2] == 116) fc = 0x5;      /* flt: PL */
    if (o[0] == 102 && o[1] == 108 && o[2] == 101) fc = 0x8;      /* fle: HI */
    if (o[0] == 102 && o[1] == 101) fc = 0x1;                     /* feq: NE */
    if (fa || fc >= 0) {
        {   int L; L = 0; while (o[L]) L = L + 1;
            dbl = L >= 2 && o[L - 2] == 54 && o[L - 1] == 52; }  /* ...64 */
        ty = dbl ? 0x00400000 : 0;
        a_fmov_to(16, a[1], dbl); a_fmov_to(17, a[2], dbl);
        if (fa) { ow(fa | ty | (17 << 16) | (16 << 5) | 16); a_fmov_from(a[0], 16, dbl); return 1; }
        ow(0x1E202000 | ty | (17 << 16) | (16 << 5));
        ow(0x9A9F07E0 | (fc << 12) | a[0]);
        return 1;
    }
    if (bk_str_is(o, "cvtid")) { ow(0x9E620000 | (a[1] << 5) | 16); a_fmov_from(a[0], 16, 1); return 1; }
    if (bk_str_is(o, "cvtud")) { ow(0x9E630000 | (a[1] << 5) | 16); a_fmov_from(a[0], 16, 1); return 1; }
    if (bk_str_is(o, "cvtis")) { ow(0x9E220000 | (a[1] << 5) | 16); a_fmov_from(a[0], 16, 0); return 1; }
    if (bk_str_is(o, "cvtus")) { ow(0x9E230000 | (a[1] << 5) | 16); a_fmov_from(a[0], 16, 0); return 1; }
    if (bk_str_is(o, "cvtdi")) { a_fmov_to(16, a[1], 1); ow(0x9E780000 | (16 << 5) | a[0]); return 1; }
    if (bk_str_is(o, "cvtdu")) { a_fmov_to(16, a[1], 1); ow(0x9E790000 | (16 << 5) | a[0]); return 1; }
    if (bk_str_is(o, "cvtsd")) { a_fmov_to(16, a[1], 0); ow(0x1E22C000 | (16 << 5) | 16); a_fmov_from(a[0], 16, 1); return 1; }
    if (bk_str_is(o, "cvtds")) { a_fmov_to(16, a[1], 1); ow(0x1E624000 | (16 << 5) | 16); a_fmov_from(a[0], 16, 0); return 1; }
    if (bk_str_is(o, "fsqrt64")) { a_fmov_to(16, a[1], 1); ow(0x1E61C000 | (16 << 5) | 16); a_fmov_from(a[0], 16, 1); return 1; }
    if (bk_str_is(o, "fsqrt32")) { a_fmov_to(16, a[1], 0); ow(0x1E21C000 | (16 << 5) | 16); a_fmov_from(a[0], 16, 0); return 1; }
    return 0;
}

/* one lowered instruction -> bytes at bkout; 0 when it has no encoding */
int bk_arm(int i, long off) {
    int op; long *a; char *o; long pc; long v; int k; int d; int n;
    op = tkop[i]; a = tka + i * 4;
    pc = bk_textva + off;
    if (op == TO_SETREG) {
        if (a[3] == SK_IMM) { a_movimm(a[0], a[1], 0); return 1; }
        if (a[3] == SK_REG) { ow(0xAA0003E0 | (a[1] << 16) | a[0]); return 1; }
        if (a[3] == SK_ADDR) { a_adrp_add(a[0], pc, a[1] + bk_shift); return 1; }
        a_adrp_add(A_IP1, pc, a[1] + bk_shift); ow(0xF9400000 | (A_IP1 << 5) | a[0]);
        return 1;
    }
    if (op == TO_SETMEM) { a_adrp_add(A_IP1, pc, a[0] + bk_shift); ow(0xF9000000 | (A_IP1 << 5) | a[1]); return 1; }
    if (op == TO_ITOA) { a_itoa(pc, a[0] + bk_shift, a[1] + bk_shift, a[2] + bk_shift); return 1; }
    if (op == TO_ARGSAVE) {
        /* Linux leaves argc at [sp] and argv at sp+8; Darwin hands x0, x1 */
        if (a[2]) { ow(0xF94003E0); ow(0x910023E1); pc = pc + 8; }
        a_adrp_add(A_IP1, pc, a[0] + bk_shift); ow(0xF9000000 | (A_IP1 << 5) | 0);
        a_adrp_add(A_IP1, pc + 12, a[1] + bk_shift); ow(0xF9000000 | (A_IP1 << 5) | 1);
        return 1;
    }
    if (op == TO_ARGVGET) {
        a_adrp_add(A_IP1, pc, a[2] + bk_shift);
        ow(0xF9400000 | (A_IP1 << 5) | A_IP1);
        ow(0x8B000000 | (a[1] << 16) | (3 << 10) | (A_IP1 << 5) | A_IP1);
        ow(0xF9400000 | (A_IP1 << 5) | a[0]);
        return 1;
    }
    if (op == TO_SPINIT) {
        if (a[1] >= 0) { a_adrp_add(a[0], pc, a[1] + bk_shift); return 1; }
        ow(0x91000000 | (31 << 5) | a[0]);
        return 1;
    }
    if (op == TO_WINSAVE) {
        a_adrp_add(A_IP0, pc, a[0] + bk_shift);
        k = 0; while (k < 8) { a_str(k, A_IP0, 8 * k); k = k + 1; }
        return 1;
    }
    if (op == TO_WINREST) {
        int s; s = bkol;
        ow(0xAA000000 | (0 << 16) | (31 << 5) | A_IP1);
        a_adrp_add(A_IP0, pc + (bkol - s), a[0] + bk_shift);
        k = 1; while (k < 8) { a_ldr(k, A_IP0, 8 * k); k = k + 1; }
        ow(0xAA000000 | (A_IP1 << 16) | (31 << 5) | (a[1] < 0 ? 31 : a[1]));
        return 1;
    }
    if (op == TO_WINSTDH) {
        int s; s = bkol;
        k = 0;
        while (k < 3) {
            a_movn(0, 0 - 10 - k);
            a_callimp(pc + (bkol - s), 0);
            a_adrp_add(A_IP0, pc + (bkol - s), a[0] + bk_shift);
            a_str(0, A_IP0, 8 * k);
            k = k + 1;
        }
        return 1;
    }
    if (op == TO_GATE) {
        char *g; g = bk_nth(BH_ABI_GATE, tkg_gate[i]);
        if (bk_str_is(g, "svc80")) { ow(0xD4001001); return 1; }
        if (bk_str_is(g, "winapi")) return a_winapi(i, off);
        ow(0xD4000001);
        return 1;
    }
    o = bk_nth(BKOPS, op);
    if (bk_str_is(o, "mov")) { ow(0xAA0003E0 | (a[1] << 16) | a[0]); return 1; }
    if (bk_str_is(o, "imm")) {
        unsigned long u; u = a[1];
        ow(0xD2800000 | ((u & 0xFFFF) << 5) | a[0]);
        k = 1;
        while (k <= 3) { v = (u >> (16 * k)) & 0xFFFF;
            if (v) ow(0xF2800000 | (k << 21) | (v << 5) | a[0]); k = k + 1; }
        return 1;
    }
    {   unsigned long alu; alu = 0;
        if (bk_str_is(o, "add64")) alu = 0x8B000000;
        if (bk_str_is(o, "sub64")) alu = 0xCB000000;
        if (bk_str_is(o, "xor64")) alu = 0xCA000000;
        if (bk_str_is(o, "and64")) alu = 0x8A000000;
        if (bk_str_is(o, "or64")) alu = 0xAA000000;
        if (bk_str_is(o, "shl64")) alu = 0x9AC02000;
        if (bk_str_is(o, "shr64")) alu = 0x9AC02800;
        if (bk_str_is(o, "lshr64")) alu = 0x9AC02400;
        if (alu) { ow(alu | (a[2] << 16) | (a[1] << 5) | a[0]); return 1; }
    }
    if (bk_str_is(o, "mul64")) { ow(0x9B007C00 | (a[2] << 16) | (a[1] << 5) | a[0]); return 1; }
    if (bk_str_is(o, "load64")) { a_mem(0, a[0], a[1], a[2], 8); return 1; }
    if (bk_str_is(o, "store64")) { a_mem(1, a[2], a[0], a[1], 8); return 1; }
    {   int cc; cc = 0 - 1;
        if (bk_str_is(o, "slt64")) cc = 0xA;
        if (bk_str_is(o, "sle64")) cc = 0xC;
        if (bk_str_is(o, "eq")) cc = 0x1;
        if (bk_str_is(o, "ne")) cc = 0x0;
        if (bk_str_is(o, "ult64")) cc = 0x2;
        if (bk_str_is(o, "ule64")) cc = 0x8;
        if (cc >= 0) {
            ow(0xEB00001F | (a[2] << 16) | (a[1] << 5));
            ow(0x9A9F07E0 | (cc << 12) | a[0]);
            return 1;
        }
    }
    if (bk_str_is(o, ".frame")) {
        n = a[0];
        if (n > 0 - 4096 && n < 4096) {
            ow((n >= 0 ? 0xD1000000 : 0x91000000) | ((n >= 0 ? n : 0 - n) << 10) | (7 << 5) | 7);
            return 1;
        }
        a_movimm(A_IP0, n >= 0 ? n : 0 - n, 0);
        ow((n >= 0 ? 0xCB000000 : 0x8B000000) | (A_IP0 << 16) | (7 << 5) | 7);
        return 1;
    }
    if (bk_str_is(o, ".lea")) { a_adrp_add(a[0], pc, bk_leaaddr(i)); return 1; }
    if (bk_str_is(o, ".ld")) { a_mem(0, a[0], a[1], a[2], a[3]); return 1; }
    if (bk_str_is(o, ".st")) { a_mem(1, a[2], a[0], a[1], a[3]); return 1; }
    if (a_fp(o, a)) return 1;
    if (bk_str_is(o, ".zero")) {
        long kk; int wd;
        kk = 0;
        while (kk < a[2]) {
            wd = 8; while (kk + wd > a[2]) wd = wd / 2;
            a_mem(1, 31, a[0], a[1] + kk, wd);
            kk = kk + wd;
        }
        return 1;
    }
    if (bk_str_is(o, "callr")) {
        a_adr(A_IP1, pc, pc + 16);
        ow(0xD1002000 | (7 << 5) | 7);
        ow(0xF9000000 | (7 << 5) | A_IP1);
        ow(0xD61F0000 | (a[0] << 5));
        return 1;
    }
    if (bk_str_is(o, ".div") || bk_str_is(o, ".udiv")) {
        ow((bk_str_is(o, ".div") ? 0x9AC00C00 : 0x9AC00800) | (a[2] << 16) | (a[1] << 5) | a[0]);
        return 1;
    }
    if (bk_str_is(o, ".mod") || bk_str_is(o, ".umod")) {
        ow((bk_str_is(o, ".mod") ? 0x9AC00C00 : 0x9AC00800) | (a[2] << 16) | (a[1] << 5) | A_IP1);
        ow(0x9B008000 | (a[2] << 16) | (a[1] << 10) | (A_IP1 << 5) | a[0]);
        return 1;
    }
    if (bk_str_is(o, "ret")) {
        ow(0xF9400000 | (7 << 5) | A_IP1);
        ow(0x91002000 | (7 << 5) | 7);
        ow(0xD61F0000 | (A_IP1 << 5));
        return 1;
    }
    if (bk_str_is(o, "nop")) { ow(0xD503201F); return 1; }
    if (bk_str_is(o, "jump")) {
        ow(0x14000000 | (a_disp(bk_label(a[0]) - off, 26) & 0x3FFFFFF));
        return 1;
    }
    if (bk_str_is(o, "call")) {
        a_adr(A_IP1, pc, pc + 16);
        ow(0xD1002000 | (7 << 5) | 7);
        ow(0xF9000000 | (7 << 5) | A_IP1);
        ow(0x14000000 | (a_disp(bk_label(a[0]) - (off + 12), 26) & 0x3FFFFFF));
        return 1;
    }
    if (bk_str_is(o, "jumpz")) {
        ow(0xB4000000 | ((a_disp(bk_label(a[1]) - off, 19) & 0x7FFFF) << 5) | a[0]);
        return 1;
    }
    return 0;
}

/* ---- x86_64: emit_x86.py, line for line ------------------------------- */
#define X_RAX 0
#define X_RCX 1
#define X_RDX 2
#define X_RBX 3
#define X_RSP 4
#define X_R8 8
#define X_R9 9
#define X_R10 10
#define X_R11 11
int x_rex(int w, int r, int x, int b) { ob(64 | (w << 3) | (r << 2) | (x << 1) | b); return 0; }
int x_modrm(int mod, int reg, int rm) { ob((mod << 6) | ((reg & 7) << 3) | (rm & 7)); return 0; }
int x_alu(int opc, int d, int s) { x_rex(1, s >> 3, 0, d >> 3); ob(opc); x_modrm(3, s, d); return 0; }
int x_movrr(int d, int s) { return x_alu(0x89, d, s); }
int x_movri(int d, unsigned long imm) {
    x_rex(1, 0, 0, d >> 3); ob(0xB8 + (d & 7));
    ow(imm & 0xFFFFFFFF); ow(imm >> 32);
    return 0;
}
int x_d32(long v) { ow(v & 0xFFFFFFFF); return 0; }
/* mem: opc may be two bytes (0x0F 0xBE); w is REX.W */
int x_mem(int opc, int opc2, int r, int b, long disp, int w) {
    x_rex(w, r >> 3, 0, b >> 3);
    ob(opc); if (opc2 >= 0) ob(opc2);
    x_modrm(2, r, b); x_d32(disp);
    return 0;
}
int x_load(int r, int b, long disp, int wd) {
    if (wd == 8) return x_mem(0x8B, 0 - 1, r, b, disp, 1);
    if (wd == 4) return x_mem(0x63, 0 - 1, r, b, disp, 1);
    if (wd == 1) return x_mem(0x0F, 0xBE, r, b, disp, 1);
    return x_mem(0x0F, 0xBF, r, b, disp, 1);
}
int x_store(int r, int b, long disp, int wd) {
    if (wd == 8) return x_mem(0x89, 0 - 1, r, b, disp, 1);
    if (wd == 4) return x_mem(0x89, 0 - 1, r, b, disp, 0);
    if (wd == 2) { ob(0x66); return x_mem(0x89, 0 - 1, r, b, disp, 0); }
    return x_mem(0x88, 0 - 1, r, b, disp, 0);
}
/* [rip+disp32], 7 bytes fixed */
int x_rip(int opc, int r, long pcnext, long target) {
    x_rex(1, r >> 3, 0, 0); ob(opc); x_modrm(0, r, 5); x_d32(target - pcnext);
    return 0;
}
int x_cmpset(int cc, int d, int ra, int rb) {
    x_alu(0x39, ra, rb);
    ob(0x41); ob(0x0F); ob(cc); x_modrm(3, 0, X_R11);
    x_rex(1, d >> 3, 0, 1); ob(0x0F); ob(0xB6); x_modrm(3, d, X_R11);
    return 0;
}
int x_spadj(long n, int opc) {                  /* on the tape SP, r10 */
    x_rex(1, 0, 0, X_R10 >> 3); ob(0x81); x_modrm(3, opc, X_R10); x_d32(n);
    return 0;
}
int x_spsub(int n) { x_rex(1, 0, 0, 0); ob(0x83); x_modrm(3, 5, 4); ob(n); return 0; }
int x_alignpre(int extra) {
    int n; n = 32 + ((extra * 8 + 15) / 16) * 16;
    x_movrr(X_RBX, X_RSP);
    x_rex(1, 0, 0, 0); ob(0x83); x_modrm(3, 4, 4); ob(0xF0);
    x_spsub(n);
    return 0;
}
int x_alignpost(void) { return x_movrr(X_RSP, X_RBX); }
int x_stackarg(int slot, long val) {
    x_rex(1, 0, 0, 0); ob(0xC7); x_modrm(1, 0, 4); ob(0x24); ob(slot); x_d32(val);
    return 0;
}
int x_callimp(long pc, int k) {
    x_rip(0x8B, X_RAX, pc + 7, bk_sizing ? 0 : bk_imp[k]);
    ob(0xFF); ob(0xD0);
    return 0;
}
int x_fd2handle(long pc, long hstd) {
    x_rex(1, 0, 0, 0); ob(0x83); x_modrm(3, 7, 1); ob(0x03);    /* cmp rcx, 3 */
    ob(0x73); ob(7 + 4);                                          /* jae over the body */
    x_rip(0x8D, X_R11, pc + 4 + 2 + 7, hstd);
    x_rex(1, 0, 0, 1); ob(0x8B); x_modrm(0, 1, 4); ob(0xCB);
    return 0;
}
int x_winapi(int i, long off) {
    long pc; long hstd; long written; char *nm; int s;
    pc = bk_textva + off;
    hstd = bk_hstd + bk_shift; written = bk_written + bk_shift;
    nm = bk_nth(BF_ABI_0, tkg_cop[i]);
    s = bkol;
    if (bk_str_is(nm, "exit")) {
        x_alignpre(0); x_callimp(pc + (bkol - s), 5); x_alignpost();
        return 1;
    }
    if (bk_str_is(nm, "write") || bk_str_is(nm, "read")) {
        x_fd2handle(pc, hstd);
        x_rip(0x8D, X_R9, pc + (bkol - s) + 7, written);
        x_alignpre(1);
        x_stackarg(32, 0);
        x_callimp(pc + (bkol - s), bk_str_is(nm, "write") ? 1 : 2);
        x_alignpost();
        x_rip(0x8B, X_RAX, pc + (bkol - s) + 7, written);
        return 1;
    }
    if (bk_str_is(nm, "close")) {
        x_fd2handle(pc, hstd);
        x_alignpre(0);
        x_callimp(pc + (bkol - s), 3);
        x_alignpost();
        return 1;
    }
    if (bk_str_is(nm, "open")) {
        x_movrr(X_RAX, X_R8);
        x_alignpre(3);
        x_rex(1, 0, 0, 0); ob(0x89); x_modrm(1, 0, 4); ob(0x24); ob(0x20);
        x_movri(X_R8, 3);
        x_movri(X_R9, 0);
        x_stackarg(40, 0x80);
        x_stackarg(48, 0);
        x_callimp(pc + (bkol - s), 4);
        x_alignpost();
        return 1;
    }
    bkol = s;
    return 0;
}
int x_itoa(long pc, long src, long buf, long lenp) {
    int s; int l0; int l1;
    s = bkol;
    x_rip(0x8B, X_RAX, pc + (bkol - s) + 7, src);
    ob(0x4D); ob(0x31); ob(0xE4);                    /* xor r12, r12 */
    ob(0x48); ob(0x85); ob(0xC0);                    /* test rax, rax */
    ob(0x79); ob(3 + 10);                            /* jns over neg + mov */
    ob(0x48); ob(0xF7); ob(0xD8);                    /* neg rax */
    x_movri(12, 1);
    x_movrr(15, X_RAX); x_movri(X_R11, 10);
    x_movrr(13, X_RAX); ob(0x4D); ob(0x31); ob(0xF6);  /* xor r14, r14 */
    l0 = bkol;
    x_movrr(X_RAX, 13); ob(0x48); ob(0x31); ob(0xD2); ob(0x49); ob(0xF7); ob(0xF3);
    x_movrr(13, X_RAX); ob(0x49); ob(0xFF); ob(0xC6);
    ob(0x4D); ob(0x85); ob(0xED);
    {   int back; back = (0 - (bkol - l0 + 2)) & 255; ob(0x75); ob(back); }
    ob(0x4D); ob(0x01); ob(0xE6);                    /* r14 += r12 */
    x_rip(0x89, 14, pc + (bkol - s) + 7, lenp);
    x_rip(0x8D, X_RBX, pc + (bkol - s) + 7, buf);
    ob(0x4E); ob(0x8D); ob(0x2C); ob(0x33);          /* lea r13, [rbx+r14] */
    l1 = bkol;
    x_movrr(X_RAX, 15); ob(0x48); ob(0x31); ob(0xD2); ob(0x49); ob(0xF7); ob(0xF3);
    x_movrr(15, X_RAX); ob(0x48); ob(0x83); ob(0xC2); ob(0x30);
    ob(0x49); ob(0xFF); ob(0xCD); ob(0x41); ob(0x88); ob(0x55); ob(0x00);
    ob(0x4D); ob(0x85); ob(0xFF);
    {   int back; back = (0 - (bkol - l1 + 2)) & 255; ob(0x75); ob(back); }
    ob(0x4D); ob(0x85); ob(0xE4); ob(0x74); ob(3);
    ob(0xC6); ob(0x03); ob(0x2D);
    return 0;
}
/* SSE: the mandatory prefix, then REX (only if needed), then 0F opc */
int x_sseg(int pfx, int opc, int xmm, int gpr, int w, int gpr_in_reg) {
    int r;
    ob(pfx);
    if (gpr_in_reg) r = 64 | (w << 3) | ((gpr >> 3) << 2);
    else r = 64 | (w << 3) | (gpr >> 3);
    if (r != 64) ob(r);
    ob(0x0F); ob(opc);
    if (gpr_in_reg) x_modrm(3, gpr, xmm); else x_modrm(3, xmm, gpr);
    return 0;
}
int x_ssex(int pfx, int opc, int xd, int xs, int imm) {
    if (pfx) ob(pfx);
    ob(0x0F); ob(opc); x_modrm(3, xd, xs);
    if (imm >= 0) ob(imm);
    return 0;
}
int x_movqx(int x, int g) { return x_sseg(0x66, 0x6E, x, g, 1, 0); }
int x_movqg(int g, int x) { return x_sseg(0x66, 0x7E, x, g, 1, 0); }
int x_movdx(int x, int g) { return x_sseg(0x66, 0x6E, x, g, 0, 0); }
int x_movdg(int g, int x) { return x_sseg(0x66, 0x7E, x, g, 0, 0); }
int x_and1(int g) { x_rex(1, 0, 0, g >> 3); ob(0x83); x_modrm(3, 4, g); ob(1); return 0; }
int x_u2f(int ra, int dbl) {
    int pfx; int bigstart; int bigl; char save[64]; int k; int s;
    pfx = dbl ? 0xF2 : 0xF3;
    x_rex(1, ra >> 3, 0, ra >> 3); ob(0x85); x_modrm(3, ra, ra);     /* test */
    /* js over small (cvtsi2s + jmp) -- sizes are fixed: sse_g with REX.W = 5, jmp = 2 */
    ob(0x78); ob(5 + 2);
    x_sseg(pfx, 0x2A, 0, ra, 1, 0);
    s = bkol;
    ob(0xEB); ob(0);                                  /* patched below */
    bigstart = bkol;
    x_movrr(X_R11, ra); x_rex(1, 0, 0, 1); ob(0xD1); x_modrm(3, 5, 11);
    x_movrr(X_RBX, ra); x_and1(X_RBX); x_alu(0x09, X_R11, X_RBX);
    x_sseg(pfx, 0x2A, 0, X_R11, 1, 0); x_ssex(pfx, 0x58, 0, 0, 0 - 1);
    bigl = bkol - bigstart;
    bkout[s + 1] = bigl;
    return 0;
}
int x_fp(char *o, long *a) {
    int dbl; int pfx; int fa; int fc; int L;
    fa = 0; fc = 0 - 1;
    if (o[0] == 102 && o[1] == 97) fa = 0x58;
    if (o[0] == 102 && o[1] == 115 && o[2] == 117) fa = 0x5C;
    if (o[0] == 102 && o[1] == 109) fa = 0x59;
    if (o[0] == 102 && o[1] == 100) fa = 0x5E;
    if (o[0] == 102 && o[1] == 101) fc = 0;
    if (o[0] == 102 && o[1] == 108 && o[2] == 116) fc = 1;
    if (o[0] == 102 && o[1] == 108 && o[2] == 101) fc = 2;
    L = 0; while (o[L]) L = L + 1;
    dbl = L >= 2 && o[L - 2] == 54 && o[L - 1] == 52;
    if (fa || fc >= 0) {
        pfx = dbl ? 0xF2 : 0xF3;
        if (dbl) { x_movqx(0, a[1]); x_movqx(1, a[2]); } else { x_movdx(0, a[1]); x_movdx(1, a[2]); }
        if (fa) { x_ssex(pfx, fa, 0, 1, 0 - 1); if (dbl) x_movqg(a[0], 0); else x_movdg(a[0], 0); return 1; }
        x_ssex(pfx, 0xC2, 0, 1, fc);
        if (dbl) x_movqg(a[0], 0); else x_movdg(a[0], 0);
        x_and1(a[0]);
        return 1;
    }
    if (bk_str_is(o, "cvtid")) { x_sseg(0xF2, 0x2A, 0, a[1], 1, 0); x_movqg(a[0], 0); return 1; }
    if (bk_str_is(o, "cvtis")) { x_sseg(0xF3, 0x2A, 0, a[1], 1, 0); x_movdg(a[0], 0); return 1; }
    if (bk_str_is(o, "cvtud")) { x_u2f(a[1], 1); x_movqg(a[0], 0); return 1; }
    if (bk_str_is(o, "cvtus")) { x_u2f(a[1], 0); x_movdg(a[0], 0); return 1; }
    if (bk_str_is(o, "cvtdi")) { x_movqx(0, a[1]); x_sseg(0xF2, 0x2C, 0, a[0], 1, 1); return 1; }
    if (bk_str_is(o, "cvtdu")) {
        int s; int bigstart;
        x_movqx(0, a[1]); x_movri(X_R11, 0x43E0000000000000); x_movqx(1, X_R11);
        x_ssex(0x66, 0x2E, 0, 1, 0 - 1);                                /* ucomisd */
        s = bkol; ob(0x73); ob(0);                                       /* jae big */
        x_sseg(0xF2, 0x2C, 0, a[0], 1, 1);
        ob(0xEB); ob(0);
        bigstart = bkol;
        bkout[s + 1] = bigstart - (s + 2);
        x_ssex(0xF2, 0x5C, 0, 1, 0 - 1); x_sseg(0xF2, 0x2C, 0, a[0], 1, 1);
        x_movri(X_R11, (unsigned long)1 << 63); x_alu(0x31, a[0], X_R11);
        bkout[bigstart - 1] = bkol - bigstart;
        return 1;
    }
    if (bk_str_is(o, "cvtsd")) { x_movdx(0, a[1]); x_ssex(0xF3, 0x5A, 0, 0, 0 - 1); x_movqg(a[0], 0); return 1; }
    if (bk_str_is(o, "cvtds")) { x_movqx(0, a[1]); x_ssex(0xF2, 0x5A, 0, 0, 0 - 1); x_movdg(a[0], 0); return 1; }
    if (bk_str_is(o, "fsqrt64")) { x_movqx(0, a[1]); x_ssex(0xF2, 0x51, 0, 0, 0 - 1); x_movqg(a[0], 0); return 1; }
    if (bk_str_is(o, "fsqrt32")) { x_movdx(0, a[1]); x_ssex(0xF3, 0x51, 0, 0, 0 - 1); x_movdg(a[0], 0); return 1; }
    return 0;
}
/* x86's two-operand ALU: dst = s1 op s2 via mov dst,s1 -- unless dst IS s2 */
int x_alias(int d, int s1, int s2) {
    if (d == s1) return s2;
    if (s2 == d) { x_movrr(X_R11, s2); return X_R11; }
    return s2;
}

int bk_x86(int i, long off) {
    int op; long *a; char *o; long pc; int s2; int k;
    op = tkop[i]; a = tka + i * 4;
    pc = bk_textva + off;
    if (op == TO_SETREG) {
        if (a[3] == SK_IMM) { x_movri(a[0], a[1]); return 1; }
        if (a[3] == SK_REG) { x_movrr(a[0], a[1]); return 1; }
        if (a[3] == SK_ADDR) { x_rip(0x8D, a[0], pc + 7, a[1] + bk_shift); return 1; }
        x_rip(0x8B, a[0], pc + 7, a[1] + bk_shift);
        return 1;
    }
    if (op == TO_SETMEM) { x_rip(0x89, a[1], pc + 7, a[0] + bk_shift); return 1; }
    if (op == TO_ITOA) { x_itoa(pc, a[0] + bk_shift, a[1] + bk_shift, a[2] + bk_shift); return 1; }
    if (op == TO_ARGSAVE) {
        int s; s = bkol;
        x_rex(1, 0, 0, 0); ob(0x8B); x_modrm(0, 0, 4); ob(0x24);
        x_rip(0x89, X_RAX, pc + (bkol - s) + 7, a[0] + bk_shift);
        x_rex(1, 0, 0, 0); ob(0x8D); x_modrm(1, 0, 4); ob(0x24); ob(0x08);
        x_rip(0x89, X_RAX, pc + (bkol - s) + 7, a[1] + bk_shift);
        return 1;
    }
    if (op == TO_ARGVGET) {
        int t;
        x_rip(0x8B, X_R11, pc + 7, a[2] + bk_shift);
        t = a[1];
        x_rex(1, 3, t >> 3, 3); ob(0x8B); x_modrm(0, 11, 4); ob(0xC3 | ((t & 7) << 3));
        return 1;
    }
    if (op == TO_SPINIT) {
        if (a[1] >= 0) { x_rip(0x8D, a[0], pc + 7, a[1] + bk_shift); return 1; }
        x_movrr(a[0], X_RSP);
        return 1;
    }
    if (op == TO_WINSAVE) {
        x_rip(0x8D, X_R11, pc + 7, a[0] + bk_shift);
        k = 0; while (k < 8) { x_mem(0x89, 0 - 1, bk_rmap[k], X_R11, 8 * k, 1); k = k + 1; }
        return 1;
    }
    if (op == TO_WINREST) {
        int s; s = bkol;
        x_movrr(X_R11, X_RAX);
        x_rip(0x8D, X_RBX, pc + (bkol - s) + 7, a[0] + bk_shift);
        k = 1; while (k < 8) { x_mem(0x8B, 0 - 1, bk_rmap[k], X_RBX, 8 * k, 1); k = k + 1; }
        x_movrr(a[1], X_R11);
        return 1;
    }
    if (op == TO_WINSTDH) {
        int s; s = bkol;
        k = 0;
        while (k < 3) {
            x_movri(X_RCX, (0 - 10 - k) & 0xFFFFFFFFFFFFFFFF);
            x_alignpre(0);
            x_callimp(pc + (bkol - s), 0);
            x_alignpost();
            x_rip(0x8D, X_R11, pc + (bkol - s) + 7, a[0] + bk_shift);
            x_mem(0x89, 0 - 1, X_RAX, X_R11, 8 * k, 1);
            k = k + 1;
        }
        return 1;
    }
    if (op == TO_GATE) {
        if (bk_str_is(bk_nth(BH_ENC_Y, tkg_form[i]), "winapi")) return x_winapi(i, off);
        ob(0x0F); ob(0x05);
        return 1;
    }
    o = bk_nth(BKOPS, op);
    if (bk_str_is(o, "mov")) { x_movrr(a[0], a[1]); return 1; }
    if (bk_str_is(o, "imm")) { x_movri(a[0], a[1]); return 1; }
    {   int alu; alu = 0 - 1;
        if (bk_str_is(o, "add64")) alu = 0x01;
        if (bk_str_is(o, "sub64")) alu = 0x29;
        if (bk_str_is(o, "xor64")) alu = 0x31;
        if (bk_str_is(o, "and64")) alu = 0x21;
        if (bk_str_is(o, "or64")) alu = 0x09;
        if (alu >= 0) {
            s2 = x_alias(a[0], a[1], a[2]);
            if (a[0] != a[1]) x_movrr(a[0], a[1]);
            x_alu(alu, a[0], s2);
            return 1;
        }
    }
    if (bk_str_is(o, "shl64") || bk_str_is(o, "shr64") || bk_str_is(o, "lshr64")) {
        x_movrr(X_R11, a[1]); x_movrr(X_RBX, a[2]);
        x_spadj(8, 5); x_mem(0x89, 0 - 1, X_RCX, X_R10, 0, 1);
        x_movrr(X_RCX, X_RBX);
        x_rex(1, 0, 0, 1); ob(0xD3);
        x_modrm(3, bk_str_is(o, "shl64") ? 4 : (bk_str_is(o, "shr64") ? 7 : 5), X_R11);
        x_mem(0x8B, 0 - 1, X_RCX, X_R10, 0, 1); x_spadj(8, 0);
        x_movrr(a[0], X_R11);
        return 1;
    }
    if (bk_str_is(o, "mul64")) {
        s2 = x_alias(a[0], a[1], a[2]);
        if (a[0] != a[1]) x_movrr(a[0], a[1]);
        x_rex(1, a[0] >> 3, 0, s2 >> 3); ob(0x0F); ob(0xAF); x_modrm(3, a[0], s2);
        return 1;
    }
    if (bk_str_is(o, "load64")) { x_load(a[0], a[1], a[2], 8); return 1; }
    if (bk_str_is(o, "store64")) { x_store(a[2], a[0], a[1], 8); return 1; }
    {   int cc; cc = 0 - 1;
        if (bk_str_is(o, "slt64")) cc = 0x9C;
        if (bk_str_is(o, "sle64")) cc = 0x9E;
        if (bk_str_is(o, "eq")) cc = 0x94;
        if (bk_str_is(o, "ne")) cc = 0x95;
        if (bk_str_is(o, "ult64")) cc = 0x92;
        if (bk_str_is(o, "ule64")) cc = 0x96;
        if (cc >= 0) { x_cmpset(cc, a[0], a[1], a[2]); return 1; }
    }
    if (bk_str_is(o, ".frame")) {
        long n; n = a[0];
        x_rex(1, 0, 0, X_R10 >> 3); ob(0x81); x_modrm(3, n >= 0 ? 5 : 0, X_R10);
        x_d32(n >= 0 ? n : 0 - n);
        return 1;
    }
    if (bk_str_is(o, ".lea")) { x_rip(0x8D, a[0], pc + 7, bk_leaaddr(i)); return 1; }
    if (bk_str_is(o, ".ld")) { x_load(a[0], a[1], a[2], a[3]); return 1; }
    if (bk_str_is(o, ".st")) { x_store(a[2], a[0], a[1], a[3]); return 1; }
    if (x_fp(o, a)) return 1;
    if (bk_str_is(o, ".zero")) {
        long kk; int wd;
        ob(0x4D); ob(0x31); ob(0xDB);
        kk = 0;
        while (kk < a[2]) {
            wd = 8; while (kk + wd > a[2]) wd = wd / 2;
            x_store(X_R11, a[0], a[1] + kk, wd);
            kk = kk + wd;
        }
        return 1;
    }
    if (bk_str_is(o, "callr")) {
        x_rip(0x8D, X_R11, pc + 7, pc + 20);
        x_rex(1, 0, 0, 1); ob(0x81); x_modrm(3, 5, 10); x_d32(8);
        x_rex(1, 1, 0, 1); ob(0x89); x_modrm(0, 11, 10);
        x_rex(0, 0, 0, a[0] >> 3); ob(0xFF); x_modrm(3, 4, a[0]);
        return 1;
    }
    if (bk_str_is(o, ".div") || bk_str_is(o, ".mod") || bk_str_is(o, ".udiv") || bk_str_is(o, ".umod")) {
        x_spadj(16, 5);
        x_mem(0x89, 0 - 1, X_RAX, X_R10, 0, 1);
        x_mem(0x89, 0 - 1, X_RDX, X_R10, 8, 1);
        x_movrr(X_R11, a[2]);
        x_movrr(X_RAX, a[1]);
        if (o[1] == 117) {                            /* .udiv .umod */
            x_rex(1, 0, 0, 0); ob(0x31); x_modrm(3, 2, 2);
            x_rex(1, 0, 0, 1); ob(0xF7); x_modrm(3, 6, 11);
        } else {
            ob(0x48); ob(0x99);
            x_rex(1, 0, 0, 1); ob(0xF7); x_modrm(3, 7, 11);
        }
        x_movrr(X_R11, (bk_str_is(o, ".div") || bk_str_is(o, ".udiv")) ? X_RAX : X_RDX);
        x_mem(0x8B, 0 - 1, X_RAX, X_R10, 0, 1);
        x_mem(0x8B, 0 - 1, X_RDX, X_R10, 8, 1);
        x_spadj(16, 0);
        x_movrr(a[0], X_R11);
        return 1;
    }
    if (bk_str_is(o, "ret")) {
        x_rex(1, 1, 0, 1); ob(0x8B); x_modrm(0, 11, 10);
        x_rex(1, 0, 0, 1); ob(0x81); x_modrm(3, 0, 10); x_d32(8);
        x_rex(0, 0, 0, 1); ob(0xFF); x_modrm(3, 4, 11);
        return 1;
    }
    if (bk_str_is(o, "nop")) { ob(0x90); return 1; }
    if (bk_str_is(o, "jump")) { ob(0xE9); x_d32(bk_label(a[0]) - (off + 5)); return 1; }
    if (bk_str_is(o, "call")) {
        x_rip(0x8D, X_R11, pc + 7, pc + 22);
        x_rex(1, 0, 0, 1); ob(0x81); x_modrm(3, 5, 10); x_d32(8);
        x_rex(1, 1, 0, 1); ob(0x89); x_modrm(0, 11, 10);
        ob(0xE9); x_d32(bk_label(a[0]) - (off + 22));
        return 1;
    }
    if (bk_str_is(o, "jumpz")) {
        int r; r = a[0];
        x_rex(1, r >> 3, 0, r >> 3); ob(0x85); x_modrm(3, r, r);
        ob(0x0F); ob(0x84); x_d32(bk_label(a[1]) - (off + 3 + 6));
        return 1;
    }
    return 0;
}

/* ---- the assembler: assemble.py ---------------------------------------- */
char bktext[BK_MAXTEXT]; long bktlen;
char bkscr[8192];
long bk_datava; long bk_entry;
int bk_enc(int i, long off) { if (bkarch) return bk_arm(i, off); return bk_x86(i, off); }
long bk_round(long v, long a) { return (v + a - 1) / a * a; }

/* Windows' import section layout (pe._idata) -- the same arithmetic */
char *BK_IMPS = "GetStdHandle\000WriteFile\000ReadFile\000CloseHandle\000CreateFileA\000ExitProcess\000GetCommandLineA\000VirtualAlloc\000";
long bk_idata_len; long bk_iat_off; long bk_cfg_off;
int bk_idata_layout(void) {
    long off; int k; int L; char *e;
    off = 184;                                      /* nm_off = 40 + 72 + 72 */
    k = 0;
    while (k < 8) {
        e = bk_nth(BK_IMPS, k); L = 0; while (e[L]) L = L + 1;
        L = 2 + L + 1; if (L % 2) L = L + 1;
        off = off + L; k = k + 1;
    }
    off = off + 13;                                  /* KERNEL32.dll and its NUL */
    off = (off + 7) / 8 * 8;
    bk_cfg_off = off; bk_iat_off = 112;
    bk_idata_len = off + 320;                        /* LOADCFG 0x140 */
    return 0;
}
long bk_macho_hdrs(void) { return 32 + (72 + 152 * 2 + 72 + 28 + 56 + 24 + 24 + 48 + 24 + 80) + 256; }

int bk_assemble(void) {
    long off; int i; int ok; int id;
    bk_sizing = 1; bk_textva = 0; bk_shift = 0;
    off = 0;
    i = 0;
    while (i < tkn) {
        toff[i] = off;
        bkout = bkscr; bkol = 0;
        ok = bk_enc(i, 0);
        if (ok) off = off + bkol; else off = off + (bkarch ? 4 : 2);
        i = i + 1;
    }
    toff[tkn] = off;
    bktlen = off;
    /* image.layout: where text and data land, known before encoding */
    bk_idata_layout();
    if (bkos == 1) {
        long h; h = bk_macho_hdrs();
        bk_textva = 4294967296 + h;
        bk_datava = 4294967296 + bk_round(h + bktlen, 16384);
    } else { if (bkos == 0) {
        bk_textva = 4194304 + 176;
        bk_datava = 4194304 + bk_round(176 + bktlen, 4096);
    } else {
        long rd;
        bk_textva = 5368709120 + 4096;
        rd = 4096 + bk_round(bktlen, 4096);
        bk_datava = 5368709120 + rd + bk_round(bk_idata_len, 4096);
        i = 0; while (i < 8) { bk_imp[i] = 5368709120 + rd + bk_iat_off + 8 * i; i = i + 1; }
    } }
    bk_shift = bk_datava - BK_DATA_BASE;
    bk_sizing = 0;
    bkout = bktext; bkol = 0;
    i = 0;
    while (i < tkn) {
        int st; st = bkol;
        ok = bk_enc(i, toff[i]);
        if (ok == 0) {                               /* no encoding: brk / ud2 */
            bkol = st;
            if (bkarch) ow(0xD4200000); else { ob(0x0F); ob(0x0B); }
        }
        i = i + 1;
    }
    if (bkol != bktlen) { __write(2, "back end: sizes moved between passes\n", 37); __exit(1); }
    id = bk_find("_start", 6);
    bk_entry = 0; if (id >= 0) bk_entry = bk_label(id);
    return 0;
}

/* ---- the image, streamed to fd 1 ------------------------------------- */
char bkwb[65536]; int bkwn; long bkwtot;
int wflush(void) { if (bkwn) __write(1, bkwb, bkwn); bkwn = 0; return 0; }
int wb(int b) { bkwb[bkwn] = b; bkwn = bkwn + 1; bkwtot = bkwtot + 1; if (bkwn == 65536) wflush(); return 0; }
int wz(long n) { while (n > 0) { wb(0); n = n - 1; } return 0; }
int w16(long v) { wb(v & 255); wb((v >> 8) & 255); return 0; }
int w32(long v) { w16(v & 65535); w16((v >> 16) & 65535); return 0; }
int w64(long v) { w32(v & 4294967295); w32((v >> 32) & 4294967295); return 0; }
int wname(char *s, int n) {                       /* s padded with NULs to n */
    int k; k = 0;
    while (s[k] && k < n) { wb(s[k]); k = k + 1; }
    while (k < n) { wb(0); k = k + 1; }
    return 0;
}
int wtext(void) { long k; k = 0; while (k < bktlen) { wb(bktext[k]); k = k + 1; } return 0; }
/* the data, in address order: stored runs and zero runs interleave */
int wdata(void) {
    int a; int z; long p; long k;
    a = 0; z = 0; p = 0;
    while (p < bkdlen) {
        if (a < bknd && bkd_at[a] == p) {
            k = 0; while (k < bkd_len[a]) { wb(bkdata[bkd_off[a] + k]); k = k + 1; }
            p = p + bkd_len[a]; a = a + 1; continue;
        }
        if (z < bknz && bkz_at[z] == p) { wz(bkz_len[z]); p = p + bkz_len[z]; z = z + 1; continue; }
        __write(2, "back end: data gap\n", 19); __exit(1);
    }
    return 0;
}

int bk_elf(void) {
    long tend; long doff;
    tend = 176 + bktlen;
    doff = bk_round(tend, 4096);
    wb(127); wb(69); wb(76); wb(70); wb(2); wb(1); wb(1); wb(0); wz(8);
    w16(2); w16(bkarch ? 183 : 62); w32(1);
    w64(4194304 + 176 + bk_entry); w64(64); w64(0);
    w32(0); w16(64); w16(56); w16(2); w16(0); w16(0); w16(0);
    w32(1); w32(5); w64(0); w64(4194304); w64(4194304); w64(tend); w64(tend); w64(4096);
    w32(1); w32(6); w64(doff); w64(4194304 + doff); w64(4194304 + doff); w64(bkdlen); w64(bkdlen); w64(4096);
    wtext(); wz(doff - tend); wdata();
    return 0;
}

int bk_seg(char *name, long vmaddr, long vmsize, long fileoff, long filesize, int maxp, int initp, int nsects) {
    w32(25); w32(72 + 80 * nsects); wname(name, 16);
    w64(vmaddr); w64(vmsize); w64(fileoff); w64(filesize);
    w32(maxp); w32(initp); w32(nsects); w32(0);
    return 0;
}
int bk_sect(char *sect, char *seg, long addr, long size, long off, long flags) {
    wname(sect, 16); wname(seg, 16); w64(addr); w64(size);
    w32(off); w32(2); w32(0); w32(0); w32(flags); w32(0); w32(0); w32(0);
    return 0;
}
int bk_macho(void) {
    long hdrs; long textsz; long datasz; long link; long v;
    hdrs = bk_macho_hdrs();
    textsz = bk_round(hdrs + bktlen, 16384);
    datasz = bk_round(bkdlen > 1 ? bkdlen : 1, 16384);
    link = textsz + datasz;
    v = 4294967296;
    w32(0xFEEDFACF); w32(bkarch ? 0x0100000C : 0x01000007); w32(bkarch ? 0 : 3);
    w32(2); w32(11); w32(hdrs - 32 - 256); w32(0x200085); w32(0);
    bk_seg("__PAGEZERO", 0, v, 0, 0, 0, 0, 0);
    bk_seg("__TEXT", v, textsz, 0, textsz, 5, 5, 1);
    bk_sect("__text", "__TEXT", v + hdrs, bktlen, hdrs, 0x80000400);
    bk_seg("__DATA", v + textsz, datasz, textsz, datasz, 3, 3, 1);
    bk_sect("__data", "__DATA", v + textsz, bkdlen, textsz, 0);
    bk_seg("__LINKEDIT", v + link, 16384, link, 8, 1, 1, 0);
    w32(0xE); w32(28); w32(12); wname("/usr/lib/dyld", 16);
    w32(0xC); w32(56); w32(24); w32(0); w32(0x10000); w32(0x10000); wname("/usr/lib/libSystem.B.dylib", 32);
    w32(0x80000028); w32(24); w64(hdrs + bk_entry); w64(0);
    w32(0x32); w32(24); w32(1); w32(13 << 16); w32(13 << 16); w32(0);
    w32(0x80000022); w32(48); wz(40);
    w32(2); w32(24); w32(link); w32(0); w32(link); w32(8);
    w32(0xB); w32(80); wz(72);
    wz(256);
    wtext(); wz(textsz - hdrs - bktlen);
    wdata(); wz(link - textsz - bkdlen);
    wz(8);
    return 0;
}

int bk_pesect(char *name, long rva, long vsize, long foff, long fsize, long flags) {
    wname(name, 8); w32(vsize); w32(rva); w32(bk_round(fsize, 512)); w32(foff);
    w32(0); w32(0); w16(0); w16(0); w32(flags);
    return 0;
}
int bk_pe(void) {
    long rd_rva; long dt_rva; long cookie_rva; long rd_file; long dt_file; long dlen2;
    long dvs; long rl_rva; long rl_file; long img; long reloc_rva; long page; long k;
    long nmrva[8]; long off; long dllrva; int j; int L; char *e; long cfgstart;
    rd_rva = 4096 + bk_round(bktlen, 4096);
    dt_rva = rd_rva + bk_round(bk_idata_len, 4096);
    cookie_rva = dt_rva + bk_round(bkdlen > 1 ? bkdlen : 1, 8);
    rd_file = 1024 + bk_round(bktlen, 512);
    dt_file = rd_file + bk_round(bk_idata_len, 512);
    dlen2 = bk_round(bkdlen > 1 ? bkdlen : 1, 8) + 8;
    dvs = dlen2 + bk_bss;
    rl_rva = dt_rva + bk_round(dvs, 4096);
    rl_file = dt_file + bk_round(dlen2, 512);
    img = rl_rva + 4096;                            /* the .reloc is 12 bytes */
    /* DOS stub and PE header */
    wb(77); wb(90); wz(58); w32(64);
    wb(80); wb(69); wb(0); wb(0);
    w16(bkarch ? 0xAA64 : 0x8664); w16(4); w32(0); w32(0); w32(0); w16(240); w16(0x22);
    w16(0x20B); wb(14); wb(0); w32(bk_round(bktlen, 512)); w32(0); w32(0); w32(4096 + bk_entry); w32(4096);
    w64(5368709120); w32(4096); w32(512);
    w16(4); w16(0); w16(0); w16(0); w16(4); w16(0); w32(0);
    w32(img); w32(1024); w32(0); w16(3); w16(0x8160);
    w64(0x100000); w64(0x1000); w64(0x100000); w64(0x1000); w32(0); w32(16);
    j = 0;
    while (j < 16) {
        if (j == 1) { w32(rd_rva); w32(40); }
        else { if (j == 5) { w32(rl_rva); w32(12); }
        else { if (j == 10) { w32(rd_rva + bk_cfg_off); w32(320); }
        else { if (j == 12) { w32(rd_rva + bk_iat_off); w32(72); }
        else { w32(0); w32(0); } } } }
        j = j + 1;
    }
    bk_pesect(".text", 4096, bktlen, 1024, bktlen, 0x60000020);
    bk_pesect(".rdata", rd_rva, bk_idata_len, rd_file, bk_idata_len, 0x40000040);
    bk_pesect(".data", dt_rva, dvs, dt_file, dlen2, 0xC0000040);
    bk_pesect(".reloc", rl_rva, 12, rl_file, 12, 0x42000040);
    wz(1024 - bkwtot);
    wtext(); wz(bk_round(bktlen, 512) - bktlen);
    /* the import section, at rd_rva */
    off = 184; j = 0;
    while (j < 8) {
        e = bk_nth(BK_IMPS, j); L = 0; while (e[L]) L = L + 1;
        nmrva[j] = rd_rva + off;
        L = 2 + L + 1; if (L % 2) L = L + 1;
        off = off + L; j = j + 1;
    }
    dllrva = rd_rva + off;
    w32(rd_rva + 40); w32(0); w32(0); w32(dllrva); w32(rd_rva + 112);
    wz(20);
    j = 0; while (j < 8) { w64(nmrva[j]); j = j + 1; } w64(0);
    j = 0; while (j < 8) { w64(nmrva[j]); j = j + 1; } w64(0);
    j = 0;
    while (j < 8) {
        e = bk_nth(BK_IMPS, j); L = 0; while (e[L]) L = L + 1;
        w16(0); k = 0; while (k < L) { wb(e[k]); k = k + 1; } wb(0);
        if ((2 + L + 1) % 2) wb(0);
        j = j + 1;
    }
    wname("KERNEL32.dll", 13);
    wz(bk_cfg_off - (off + 13));
    cfgstart = bkwtot;
    w32(320); wz(0x58 - 4); w64(5368709120 + cookie_rva); wz(320 - 0x58 - 8);
    wz(bk_round(bk_idata_len, 512) - bk_idata_len);
    /* the data, 8-padded, then 8 bytes of cookie */
    wdata(); wz(dlen2 - bkdlen);
    wz(bk_round(dlen2, 512) - dlen2);
    /* .reloc: one block, the cookie pointer in the load config, DIR64 */
    reloc_rva = rd_rva + bk_cfg_off + 0x58;
    page = reloc_rva / 4096 * 4096;
    w32(page); w32(12); w16((10 << 12) | (reloc_rva - page)); w16(0);
    wz(512 - 12);
    return 0;
}

/* `unisacc FILE -b os/arch`: the tape in t[0..n), as an image on fd 1 */
int bk_build(char *t, int n, char *target) {
    bkos = 0;
    if (target[0] == 111) bkos = 1;                  /* osx */
    if (target[0] == 119) bkos = 2;                  /* win */
    bkarch = 0; if (target[4] == 97) bkarch = 1;     /* .../arm64 */
    bk_parse(t, n);
    bk_lower();
    bk_assemble();
    bkwn = 0; bkwtot = 0;
    if (bkos == 0) bk_elf();
    if (bkos == 1) bk_macho();
    if (bkos == 2) bk_pe();
    wflush();
    return 0;
}
