/* ---- GNU assembly text for unisacc's ELF objects (0.0.18 R18-1, R18-2) ----
   `-S -b lnx/ARCH` writes the object that `-c -b lnx/ARCH` would write, as
   GNU assembly (AT&T syntax on x86-64): at_dis() reads the ELF bytes -- text,
   data, bss, the symbol table in its order, the relocations, the
   `.unisa.tape` blob -- and prints them.  `unisacc as FILE.s -o FILE.o`
   (at_main) reads such text back and writes the same ELF, byte for byte.

   One encoder serves both directions.  Every instruction the disassembler
   prints is assembled again by at_enc() and must give back the same bytes
   and the same relocation; anything that does not (a word the tables do not
   know, a branch into the middle of an instruction) is printed as `.byte` /
   `.inst` instead.  So the round trip -S | as == -c holds by construction,
   and the gate counts the fallbacks.

   The subset is what the back end emits (measured over the 186 tests/c
   programs: 59 x86-64 and 48 arm64 mnemonics, archive/plans/v0.0.18.md).  The x86-64
   encoder picks the same encoding the system assembler would, except a
   redundant REX 0x40 on narrow stores, which is written `{rex}`.
   Only the ELF layout bk_elfobj() writes is read; other objects are refused.
   No libc: the reference and the product driver (AT_PRODUCT) share it. */

#ifndef AT_PRODUCT
int at_fd; int at_wfail;                       /* at_wfail: a dry run, nothing written */
int at_wr(char *p, long n) { if (at_wfail) return 0; __write(at_fd, p, n); return 0; }
#else
int at_wr(char *p, long n);
#endif
char *at_err;
char *at_cap; long at_capn;                    /* -S: bk_object writes into memory */
#ifndef AT_PRODUCT
char at_src[33554432];
#endif

/* ---- small helpers ---- */
int at_slen(char *s) { int n; n = 0; while (s[n]) n = n + 1; return n; }
int at_is(char *a, int an, char *b) {          /* a[0..an) equals the C string b */
    int k; k = 0;
    while (k < an) { if (b[k] == 0 || b[k] != a[k]) return 0; k = k + 1; }
    return b[k] == 0;
}
int at_idc(int c) {
    return (c >= 65 && c <= 90) || (c >= 97 && c <= 122) || (c >= 48 && c <= 57) || c == 95 || c == 46 || c == 36;
}
int at_sp(int c) { return c == 32 || c == 9 || c == 13; }
unsigned long at_rd(char *p, int n) {          /* little-endian */
    unsigned long v; int k; v = 0; k = n - 1;
    while (k >= 0) { v = (v << 8) | (p[k] & 255); k = k - 1; }
    return v;
}

/* ---- output, through at_wr ---- */
char at_ob[65536]; int at_on;
int at_flush(void) { if (at_on) at_wr(at_ob, at_on); at_on = 0; return 0; }
int at_c(int c) { at_ob[at_on] = c; at_on = at_on + 1; if (at_on == 65536) at_flush(); return 0; }
int at_s(char *s) { while (*s) { at_c(*s); s = s + 1; } return 0; }
int at_sn(char *s, int n) { int k; k = 0; while (k < n) { at_c(s[k]); k = k + 1; } return 0; }
/* the current instruction's text, assembled again before it is printed */
char at_lb[512]; int at_ln;
int at_lc(int c) { if (at_ln < 500) { at_lb[at_ln] = c; at_ln = at_ln + 1; } return 0; }
int at_ls(char *s) { while (*s) { at_lc(*s); s = s + 1; } return 0; }
int at_lsn(char *s, int n) { int k; k = 0; while (k < n) { at_lc(s[k]); k = k + 1; } return 0; }
int at_lhex(unsigned long v) {
    char d[24]; int n; n = 0;
    at_ls("0x");
    if (v == 0) { at_lc(48); return 0; }
    while (v) { d[n] = "0123456789abcdef"[v & 15]; v = v >> 4; n = n + 1; }
    while (n > 0) { n = n - 1; at_lc(d[n]); }
    return 0;
}
int at_lnum(long v) { if (v < 0) { at_lc(45); return at_lhex((unsigned long)0 - (unsigned long)v); } return at_lhex(v); }
int at_ldec(long v) {
    char d[24]; int n; n = 0;
    if (v < 0) { at_lc(45); v = 0 - v; }
    if (v == 0) { at_lc(48); return 0; }
    while (v) { d[n] = 48 + v % 10; v = v / 10; n = n + 1; }
    while (n > 0) { n = n - 1; at_lc(d[n]); }
    return 0;
}

/* ---- numbers and symbol expressions: [name][+|-number] or number ---- */
int at_num(char *s, int n, long *v) {          /* the whole of s[0..n) is a number */
    int k; int neg; unsigned long x; int d;
    k = 0; neg = 0; x = 0;
    if (n > 0 && s[0] == 45) { neg = 1; k = 1; }
    if (n > 0 && s[0] == 43) k = 1;
    if (k >= n) return 0;
    if (n - k > 2 && s[k] == 48 && (s[k + 1] == 120 || s[k + 1] == 88)) {
        k = k + 2; if (k >= n) return 0;
        while (k < n) {
            d = s[k];
            if (d >= 48 && d <= 57) d = d - 48; else { if (d >= 97 && d <= 102) d = d - 87; else { if (d >= 65 && d <= 70) d = d - 55; else return 0; } }
            x = (x << 4) | d; k = k + 1;
        }
    } else {
        while (k < n) { d = s[k]; if (d < 48 || d > 57) return 0; x = x * 10 + (d - 48); k = k + 1; }
    }
    *v = neg ? (long)((unsigned long)0 - x) : (long)x;
    return 1;
}
/* expression: name (may be empty) and addend */
char *at_xn; int at_xl; long at_xa;
int at_expr(char *s, int n) {
    int k;
    while (n > 0 && at_sp(s[0])) { s = s + 1; n = n - 1; }
    while (n > 0 && at_sp(s[n - 1])) n = n - 1;
    at_xn = s; at_xl = 0; at_xa = 0;
    if (n == 0) return 1;                       /* empty: no displacement */
    if (at_num(s, n, &at_xa)) return 1;
    if (!(at_idc(s[0]) && !(s[0] >= 48 && s[0] <= 57))) return 0;
    k = 0; while (k < n && at_idc(s[k])) k = k + 1;
    at_xl = k;
    if (k == n) return 1;
    if (s[k] != 43 && s[k] != 45) return 0;
    if (s[k] == 43) return at_num(s + k + 1, n - k - 1, &at_xa);
    return at_num(s + k, n - k, &at_xa);
}

/* ---- symbols, as the assembler sees them ----
   sect: 0 undefined, 1 .text, 2 .data, 3 .bss.  In the disassembler's check
   (at_dismode) names are not looked up: `.L<hex>` is that text offset,
   `.Ldata`/`.Lbss` the section starts, anything else undefined. */
#define AT_MAXS 262144
#define AT_MAXT 16777216                       /* text, data, tape: bytes */
int at_nsym; int at_sname[AT_MAXS]; int at_snl[AT_MAXS]; int at_ssect[AT_MAXS]; long at_sval[AT_MAXS];
char at_sglob[AT_MAXS]; char at_sref[AT_MAXS]; char at_sdef[AT_MAXS]; char at_splaced[AT_MAXS];
int at_unk;                                     /* the target is a text label not yet placed (first layout round) */
int at_lround; long at_fwd;                      /* layout round; how far this item moved since the last round */
char at_pool[8388608]; int at_pooln;
#define AT_HASH 524288
int at_hash[AT_HASH];                           /* symbol index + 1 */
int at_dismode;
int at_hfind(char *s, int n) {
    unsigned long h; int k; int i;
    h = 5381; k = 0; while (k < n) { h = h * 33 + (s[k] & 255); k = k + 1; }
    i = (int)(h & (AT_HASH - 1));
    while (at_hash[i]) {
        int j; j = at_hash[i] - 1;
        if (at_snl[j] == n) { k = 0; while (k < n && at_pool[at_sname[j] + k] == s[k]) k = k + 1; if (k == n) return i; }
        i = (i + 1) & (AT_HASH - 1);
    }
    return i;
}
int at_sym(char *s, int n) {                    /* find or create */
    int i; int j;
    i = at_hfind(s, n);
    if (at_hash[i]) return at_hash[i] - 1;
    if (at_nsym >= AT_MAXS || at_pooln + n + 1 >= 8388608) { at_err = "as: too many symbols"; return 0 - 1; }
    j = at_nsym; at_nsym = at_nsym + 1;
    at_sname[j] = at_pooln; at_snl[j] = n;
    { int k; k = 0; while (k < n) { at_pool[at_pooln] = s[k]; at_pooln = at_pooln + 1; k = k + 1; } }
    at_pool[at_pooln] = 0; at_pooln = at_pooln + 1;
    at_ssect[j] = 0; at_sval[j] = 0; at_sglob[j] = 0; at_sref[j] = 0; at_sdef[j] = 0;
    at_hash[i] = j + 1;
    return j;
}
int at_local(char *s, int n) { return n >= 2 && s[0] == 46 && s[1] == 76; }   /* `.L...`: never in the symbol table */
/* resolve the expression just parsed: section and value; *id is the symbol
   (for an undefined one) or -1 */
int at_rsect; long at_rval; int at_rid;
int at_resolve(void) {
    int j;
    at_rid = 0 - 1;
    if (at_xl == 0) { at_rsect = 0 - 1; at_rval = at_xa; return 1; }   /* a plain number */
    if (at_dismode) {
        if (at_is(at_xn, at_xl, ".Ldata")) { at_rsect = 2; at_rval = at_xa; return 1; }
        if (at_is(at_xn, at_xl, ".Lbss")) { at_rsect = 3; at_rval = at_xa; return 1; }
        if (at_xl > 2 && at_local(at_xn, at_xl)) {
            long v; char tmp[24]; int k;
            if (at_xl - 2 > 20) return 0;
            tmp[0] = 48; tmp[1] = 120; k = 2; while (k < at_xl) { tmp[k] = at_xn[k]; k = k + 1; }
            if (!at_num(tmp, at_xl, &v)) return 0;
            at_rsect = 1; at_rval = v + at_xa; return 1;
        }
        at_rsect = 0; at_rval = at_xa; return 1;
    }
    j = at_sym(at_xn, at_xl);
    if (j < 0) return 0;
    at_sref[j] = 1;
    if (at_sdef[j]) {
        at_rsect = at_ssect[j]; at_rval = at_sval[j] + at_xa;
        if (at_ssect[j] == 1) {
            if (!at_splaced[j]) at_unk = 1;
            /* a label ahead, placed last round, has moved at least as far as we have */
            else { if (at_splaced[j] != at_lround) at_rval = at_rval + at_fwd; }
        }
        return 1;
    }
    at_rsect = 0; at_rval = at_xa; at_rid = j; return 1;
}

/* ---- relocations produced by one instruction ----
   type: ELF type; rsect 1..3 section-relative, 0 a symbol (rid / name) */
int at_nr; int at_roff[2]; int at_rtype[2]; int at_rs[2]; long at_radd[2]; int at_rsym[2]; char *at_rnm[2]; int at_rnl[2];
int at_addrel(int off, int type, long add) {
    at_roff[at_nr] = off; at_rtype[at_nr] = type; at_rs[at_nr] = at_rsect; at_radd[at_nr] = add;
    at_rsym[at_nr] = at_rid; at_rnm[at_nr] = at_xn; at_rnl[at_nr] = at_xl;
    at_nr = at_nr + 1; return 0;
}

/* ---- operand split: commas at bracket depth 0 ---- */
char *at_op[6]; int at_opl[6]; int at_nop;
char *at_mn; int at_mnl; int at_rex; int at_d32;   /* mnemonic; `{rex}` / `{disp32}` given */
int at_split(char *s, int n) {
    int k; int d; int st;
    at_rex = 0; at_d32 = 0; at_nop = 0;
    while (n > 0 && at_sp(s[0])) { s = s + 1; n = n - 1; }
    while (n > 0 && at_sp(s[n - 1])) n = n - 1;
    if (n >= 5 && s[0] == 123 && at_is(s, 5, "{rex}")) {
        at_rex = 1; s = s + 5; n = n - 5;
        while (n > 0 && at_sp(s[0])) { s = s + 1; n = n - 1; }
    }
    if (n >= 8 && s[0] == 123 && at_is(s, 8, "{disp32}")) {   /* a branch kept in its long form */
        at_d32 = 1; s = s + 8; n = n - 8;
        while (n > 0 && at_sp(s[0])) { s = s + 1; n = n - 1; }
    }
    k = 0; while (k < n && !at_sp(s[k])) k = k + 1;
    at_mn = s; at_mnl = k;
    while (k < n && at_sp(s[k])) k = k + 1;
    if (k >= n) return 1;
    d = 0; st = k;
    while (k <= n) {
        if (k == n || (s[k] == 44 && d == 0)) {
            int a; int b; a = st; b = k;
            while (a < b && at_sp(s[a])) a = a + 1;
            while (b > a && at_sp(s[b - 1])) b = b - 1;
            if (at_nop >= 6) return 0;
            at_op[at_nop] = s + a; at_opl[at_nop] = b - a; at_nop = at_nop + 1;
            st = k + 1;
        } else {
            if (s[k] == 40 || s[k] == 91) d = d + 1;
            if (s[k] == 41 || s[k] == 93) d = d - 1;
        }
        k = k + 1;
    }
    return 1;
}

/* ==== x86-64 ==== */
char *at_r64[16] = { "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15" };
char *at_r32[16] = { "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi", "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d" };
char *at_r16[16] = { "ax", "cx", "dx", "bx", "sp", "bp", "si", "di", "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w" };
char *at_r8[16] = { "al", "cl", "dl", "bl", "spl", "bpl", "sil", "dil", "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b" };
char *at_r8h[4] = { "ah", "ch", "dh", "bh" };
/* a register name (without %): class 8/16/32/64, 128 = xmm; *hi for ah..bh */
int at_xreg(char *s, int n, int *cls, int *hi) {
    int k;
    *hi = 0;
    k = 0;
    while (k < 16) {
        if (at_is(s, n, at_r64[k])) { *cls = 64; return k; }
        if (at_is(s, n, at_r32[k])) { *cls = 32; return k; }
        if (at_is(s, n, at_r16[k])) { *cls = 16; return k; }
        if (at_is(s, n, at_r8[k])) { *cls = 8; return k; }
        k = k + 1;
    }
    k = 0; while (k < 4) { if (at_is(s, n, at_r8h[k])) { *cls = 8; *hi = 1; return k + 4; } k = k + 1; }
    if (n >= 4 && n <= 5 && at_is(s, 3, "xmm")) {
        long v; if (at_num(s + 3, n - 3, &v) && v >= 0 && v < 16) { *cls = 128; return (int)v; }
    }
    return 0 - 1;
}
/* a parsed x86 operand */
#define XO_REG 1
#define XO_IMM 2
#define XO_MEM 3
#define XO_TGT 4
#define XO_IND 5                                /* *%reg */
int xo_k[4]; int xo_cls[4]; int xo_r[4]; int xo_hi[4]; long xo_imm[4];
int xo_base[4]; int xo_idx[4]; int xo_sc[4]; int xo_rip[4];
char *xo_xn[4]; int xo_xl[4]; long xo_xa[4];   /* displacement / target expression */
int at_xop(int i, char *s, int n) {
    int k; int cls; int hi; int r;
    xo_base[i] = 0 - 1; xo_idx[i] = 0 - 1; xo_sc[i] = 1; xo_rip[i] = 0; xo_xn[i] = s; xo_xl[i] = 0; xo_xa[i] = 0;
    if (n >= 2 && s[0] == 37) {
        r = at_xreg(s + 1, n - 1, &cls, &hi); if (r < 0) return 0;
        xo_k[i] = XO_REG; xo_cls[i] = cls; xo_r[i] = r; xo_hi[i] = hi; return 1;
    }
    if (n >= 3 && s[0] == 42 && s[1] == 37) {
        r = at_xreg(s + 2, n - 2, &cls, &hi); if (r < 0 || cls != 64) return 0;
        xo_k[i] = XO_IND; xo_cls[i] = cls; xo_r[i] = r; return 1;
    }
    if (n >= 2 && s[0] == 36) {
        if (!at_num(s + 1, n - 1, &xo_imm[i])) return 0;
        xo_k[i] = XO_IMM; return 1;
    }
    k = 0; while (k < n && s[k] != 40) k = k + 1;
    if (k == n && n > 4 && at_is(s + n - 4, 4, "@PLT")) { k = n - 4; n = k; }
    if (!at_expr(s, k)) return 0;
    xo_xn[i] = at_xn; xo_xl[i] = at_xl; xo_xa[i] = at_xa;
    if (k == n) { xo_k[i] = XO_TGT; return 1; }
    if (s[n - 1] != 41) return 0;
    {   char *p; int m; int c; int a; int j; int part;
        p = s + k + 1; m = n - k - 2; part = 0; a = 0;
        while (a <= m) {
            j = a; while (j < m && p[j] != 44) j = j + 1;
            {   int b; int e; b = a; e = j;
                while (b < e && at_sp(p[b])) b = b + 1;
                while (e > b && at_sp(p[e - 1])) e = e - 1;
                if (part == 0) {
                    if (e > b) {
                        if (p[b] != 37) return 0;
                        if (at_is(p + b + 1, e - b - 1, "rip")) xo_rip[i] = 1;
                        else { r = at_xreg(p + b + 1, e - b - 1, &c, &hi); if (r < 0 || c != 64) return 0; xo_base[i] = r; }
                    }
                } else { if (part == 1) {
                    if (e - b < 2 || p[b] != 37) return 0;
                    r = at_xreg(p + b + 1, e - b - 1, &c, &hi); if (r < 0 || c != 64 || r == 4) return 0; xo_idx[i] = r;
                } else { if (part == 2) {
                    long v; if (!at_num(p + b, e - b, &v)) return 0;
                    if (v != 1 && v != 2 && v != 4 && v != 8) return 0; xo_sc[i] = (int)v;
                } else return 0; } }
            }
            part = part + 1; a = j + 1;
        }
        if (xo_rip[i] && xo_idx[i] >= 0) return 0;
        if (xo_rip[i] == 0 && xo_base[i] < 0) return 0;     /* absolute addresses: not in the subset */
    }
    xo_k[i] = XO_MEM;
    return 1;
}
/* forms */
#define XF_NONE 0
#define XF_RRM 1        /* op %reg, rm      (reg field = first operand) */
#define XF_RMR 2        /* op rm, %reg      (reg field = second operand) */
#define XF_MR 3         /* lea mem, %reg */
#define XF_RM 4         /* op rm            (/ext) */
#define XF_CLRM 5       /* op %cl, rm       (/ext) */
#define XF_I8RM 6       /* op $imm8, rm     (/ext, sign-extended) */
#define XF_I32RM 7      /* op $imm32, rm    (/ext) */
#define XF_OI32 8       /* movl $imm32, %r32 */
#define XF_OI64 9       /* movabsq $imm64, %r64 */
#define XF_O 10         /* push/pop %r64 */
#define XF_J8 11
#define XF_J32 12
#define XF_CALL 13
#define XF_IND 14       /* call *%r64 */
#define XF_XX 15        /* op %xmm_s, %xmm_d   (reg = d) */
#define XF_XXI 16       /* same, then a fixed imm8 (cmpXXsd) */
#define XF_GX 17        /* op %gpr, %xmm       (reg = xmm) */
#define XF_XG 18        /* op %xmm, %gpr       (reg = gpr) */
#define XF_XGR 19       /* op %xmm, %gpr       (reg = xmm, rm = gpr: 66 0F 7E) */
#define XF_I8RMU 20     /* movb $imm8, rm (unsigned byte) */
struct at_xrow { char *mn; int pfx; int w; int op; int op2; int ext; int form; int sz; int sz2; int imm; };
struct at_xrow at_xt[] = {
    { "retq", 0, 0, 0xC3, -1, -1, XF_NONE, 0, 0, 0 }, { "nop", 0, 0, 0x90, -1, -1, XF_NONE, 0, 0, 0 },
    { "cqto", 0, 1, 0x99, -1, -1, XF_NONE, 0, 0, 0 }, { "syscall", 0, 0, 0x0F, 0x05, -1, XF_NONE, 0, 0, 0 },
    { "ud2", 0, 0, 0x0F, 0x0B, -1, XF_NONE, 0, 0, 0 },
    { "movq", 0, 1, 0x89, -1, -1, XF_RRM, 64, 64, 0 }, { "movl", 0, 0, 0x89, -1, -1, XF_RRM, 32, 32, 0 },
    { "movw", 0x66, 0, 0x89, -1, -1, XF_RRM, 16, 16, 0 }, { "movb", 0, 0, 0x88, -1, -1, XF_RRM, 8, 8, 0 },
    { "movq", 0, 1, 0x8B, -1, -1, XF_RMR, 64, 64, 0 }, { "movl", 0, 0, 0x8B, -1, -1, XF_RMR, 32, 32, 0 },
    { "movslq", 0, 1, 0x63, -1, -1, XF_RMR, 64, 32, 0 },
    { "movsbq", 0, 1, 0x0F, 0xBE, -1, XF_RMR, 64, 8, 0 }, { "movswq", 0, 1, 0x0F, 0xBF, -1, XF_RMR, 64, 16, 0 },
    { "movzbq", 0, 1, 0x0F, 0xB6, -1, XF_RMR, 64, 8, 0 }, { "movzwq", 0, 1, 0x0F, 0xB7, -1, XF_RMR, 64, 16, 0 },
    { "movzbl", 0, 0, 0x0F, 0xB6, -1, XF_RMR, 32, 8, 0 },
    { "leaq", 0, 1, 0x8D, -1, -1, XF_MR, 64, 64, 0 },
    { "addq", 0, 1, 0x01, -1, -1, XF_RRM, 64, 64, 0 }, { "subq", 0, 1, 0x29, -1, -1, XF_RRM, 64, 64, 0 },
    { "cmpq", 0, 1, 0x39, -1, -1, XF_RRM, 64, 64, 0 }, { "testq", 0, 1, 0x85, -1, -1, XF_RRM, 64, 64, 0 },
    { "andq", 0, 1, 0x21, -1, -1, XF_RRM, 64, 64, 0 }, { "orq", 0, 1, 0x09, -1, -1, XF_RRM, 64, 64, 0 },
    { "xorq", 0, 1, 0x31, -1, -1, XF_RRM, 64, 64, 0 },
    { "testl", 0, 0, 0x85, -1, -1, XF_RRM, 32, 32, 0 }, { "xorl", 0, 0, 0x31, -1, -1, XF_RRM, 32, 32, 0 },
    { "cmpl", 0, 0, 0x39, -1, -1, XF_RRM, 32, 32, 0 },
    { "imulq", 0, 1, 0x0F, 0xAF, -1, XF_RMR, 64, 64, 0 },
    { "addq", 0, 1, 0x83, -1, 0, XF_I8RM, 64, 0, 0 }, { "orq", 0, 1, 0x83, -1, 1, XF_I8RM, 64, 0, 0 },
    { "andq", 0, 1, 0x83, -1, 4, XF_I8RM, 64, 0, 0 }, { "subq", 0, 1, 0x83, -1, 5, XF_I8RM, 64, 0, 0 },
    { "xorq", 0, 1, 0x83, -1, 6, XF_I8RM, 64, 0, 0 }, { "cmpq", 0, 1, 0x83, -1, 7, XF_I8RM, 64, 0, 0 },
    { "addq", 0, 1, 0x81, -1, 0, XF_I32RM, 64, 0, 0 }, { "orq", 0, 1, 0x81, -1, 1, XF_I32RM, 64, 0, 0 },
    { "andq", 0, 1, 0x81, -1, 4, XF_I32RM, 64, 0, 0 }, { "subq", 0, 1, 0x81, -1, 5, XF_I32RM, 64, 0, 0 },
    { "xorq", 0, 1, 0x81, -1, 6, XF_I32RM, 64, 0, 0 }, { "cmpq", 0, 1, 0x81, -1, 7, XF_I32RM, 64, 0, 0 },
    { "movq", 0, 1, 0xC7, -1, 0, XF_I32RM, 64, 0, 0 }, { "movb", 0, 0, 0xC6, -1, 0, XF_I8RMU, 8, 0, 0 },
    { "movl", 0, 0, 0xB8, -1, -1, XF_OI32, 32, 0, 0 }, { "movabsq", 0, 1, 0xB8, -1, -1, XF_OI64, 64, 0, 0 },
    { "pushq", 0, 0, 0x50, -1, -1, XF_O, 64, 0, 0 }, { "popq", 0, 0, 0x58, -1, -1, XF_O, 64, 0, 0 },
    { "notq", 0, 1, 0xF7, -1, 2, XF_RM, 64, 0, 0 }, { "negq", 0, 1, 0xF7, -1, 3, XF_RM, 64, 0, 0 },
    { "divq", 0, 1, 0xF7, -1, 6, XF_RM, 64, 0, 0 }, { "idivq", 0, 1, 0xF7, -1, 7, XF_RM, 64, 0, 0 },
    { "incq", 0, 1, 0xFF, -1, 0, XF_RM, 64, 0, 0 }, { "decq", 0, 1, 0xFF, -1, 1, XF_RM, 64, 0, 0 },
    { "callq", 0, 0, 0xFF, -1, 2, XF_IND, 64, 0, 0 },
    { "shlq", 0, 1, 0xD3, -1, 4, XF_CLRM, 64, 0, 0 }, { "shrq", 0, 1, 0xD3, -1, 5, XF_CLRM, 64, 0, 0 },
    { "sarq", 0, 1, 0xD3, -1, 7, XF_CLRM, 64, 0, 0 },
    { "shlq", 0, 1, 0xD1, -1, 4, XF_RM, 64, 0, 0 }, { "shrq", 0, 1, 0xD1, -1, 5, XF_RM, 64, 0, 0 },
    { "sarq", 0, 1, 0xD1, -1, 7, XF_RM, 64, 0, 0 },
    { "seto", 0, 0, 0x0F, 0x90, 0, XF_RM, 8, 0, 0 }, { "setno", 0, 0, 0x0F, 0x91, 0, XF_RM, 8, 0, 0 },
    { "setb", 0, 0, 0x0F, 0x92, 0, XF_RM, 8, 0, 0 }, { "setae", 0, 0, 0x0F, 0x93, 0, XF_RM, 8, 0, 0 },
    { "sete", 0, 0, 0x0F, 0x94, 0, XF_RM, 8, 0, 0 }, { "setne", 0, 0, 0x0F, 0x95, 0, XF_RM, 8, 0, 0 },
    { "setbe", 0, 0, 0x0F, 0x96, 0, XF_RM, 8, 0, 0 }, { "seta", 0, 0, 0x0F, 0x97, 0, XF_RM, 8, 0, 0 },
    { "sets", 0, 0, 0x0F, 0x98, 0, XF_RM, 8, 0, 0 }, { "setns", 0, 0, 0x0F, 0x99, 0, XF_RM, 8, 0, 0 },
    { "setp", 0, 0, 0x0F, 0x9A, 0, XF_RM, 8, 0, 0 }, { "setnp", 0, 0, 0x0F, 0x9B, 0, XF_RM, 8, 0, 0 },
    { "setl", 0, 0, 0x0F, 0x9C, 0, XF_RM, 8, 0, 0 }, { "setge", 0, 0, 0x0F, 0x9D, 0, XF_RM, 8, 0, 0 },
    { "setle", 0, 0, 0x0F, 0x9E, 0, XF_RM, 8, 0, 0 }, { "setg", 0, 0, 0x0F, 0x9F, 0, XF_RM, 8, 0, 0 },
    { "jo", 0, 0, 0x70, -1, -1, XF_J8, 0, 0, 0 }, { "jno", 0, 0, 0x71, -1, -1, XF_J8, 0, 0, 0 },
    { "jb", 0, 0, 0x72, -1, -1, XF_J8, 0, 0, 0 }, { "jae", 0, 0, 0x73, -1, -1, XF_J8, 0, 0, 0 },
    { "je", 0, 0, 0x74, -1, -1, XF_J8, 0, 0, 0 }, { "jne", 0, 0, 0x75, -1, -1, XF_J8, 0, 0, 0 },
    { "jbe", 0, 0, 0x76, -1, -1, XF_J8, 0, 0, 0 }, { "ja", 0, 0, 0x77, -1, -1, XF_J8, 0, 0, 0 },
    { "js", 0, 0, 0x78, -1, -1, XF_J8, 0, 0, 0 }, { "jns", 0, 0, 0x79, -1, -1, XF_J8, 0, 0, 0 },
    { "jp", 0, 0, 0x7A, -1, -1, XF_J8, 0, 0, 0 }, { "jnp", 0, 0, 0x7B, -1, -1, XF_J8, 0, 0, 0 },
    { "jl", 0, 0, 0x7C, -1, -1, XF_J8, 0, 0, 0 }, { "jge", 0, 0, 0x7D, -1, -1, XF_J8, 0, 0, 0 },
    { "jle", 0, 0, 0x7E, -1, -1, XF_J8, 0, 0, 0 }, { "jg", 0, 0, 0x7F, -1, -1, XF_J8, 0, 0, 0 },
    { "jmp", 0, 0, 0xEB, -1, -1, XF_J8, 0, 0, 0 },
    { "jo", 0, 0, 0x0F, 0x80, -1, XF_J32, 0, 0, 0 }, { "jno", 0, 0, 0x0F, 0x81, -1, XF_J32, 0, 0, 0 },
    { "jb", 0, 0, 0x0F, 0x82, -1, XF_J32, 0, 0, 0 }, { "jae", 0, 0, 0x0F, 0x83, -1, XF_J32, 0, 0, 0 },
    { "je", 0, 0, 0x0F, 0x84, -1, XF_J32, 0, 0, 0 }, { "jne", 0, 0, 0x0F, 0x85, -1, XF_J32, 0, 0, 0 },
    { "jbe", 0, 0, 0x0F, 0x86, -1, XF_J32, 0, 0, 0 }, { "ja", 0, 0, 0x0F, 0x87, -1, XF_J32, 0, 0, 0 },
    { "js", 0, 0, 0x0F, 0x88, -1, XF_J32, 0, 0, 0 }, { "jns", 0, 0, 0x0F, 0x89, -1, XF_J32, 0, 0, 0 },
    { "jp", 0, 0, 0x0F, 0x8A, -1, XF_J32, 0, 0, 0 }, { "jnp", 0, 0, 0x0F, 0x8B, -1, XF_J32, 0, 0, 0 },
    { "jl", 0, 0, 0x0F, 0x8C, -1, XF_J32, 0, 0, 0 }, { "jge", 0, 0, 0x0F, 0x8D, -1, XF_J32, 0, 0, 0 },
    { "jle", 0, 0, 0x0F, 0x8E, -1, XF_J32, 0, 0, 0 }, { "jg", 0, 0, 0x0F, 0x8F, -1, XF_J32, 0, 0, 0 },
    { "jmp", 0, 0, 0xE9, -1, -1, XF_J32, 0, 0, 0 }, { "callq", 0, 0, 0xE8, -1, -1, XF_CALL, 0, 0, 0 },
    { "addsd", 0xF2, 0, 0x0F, 0x58, -1, XF_XX, 0, 0, 0 }, { "mulsd", 0xF2, 0, 0x0F, 0x59, -1, XF_XX, 0, 0, 0 },
    { "subsd", 0xF2, 0, 0x0F, 0x5C, -1, XF_XX, 0, 0, 0 }, { "divsd", 0xF2, 0, 0x0F, 0x5E, -1, XF_XX, 0, 0, 0 },
    { "sqrtsd", 0xF2, 0, 0x0F, 0x51, -1, XF_XX, 0, 0, 0 }, { "cvtsd2ss", 0xF2, 0, 0x0F, 0x5A, -1, XF_XX, 0, 0, 0 },
    { "addss", 0xF3, 0, 0x0F, 0x58, -1, XF_XX, 0, 0, 0 }, { "mulss", 0xF3, 0, 0x0F, 0x59, -1, XF_XX, 0, 0, 0 },
    { "subss", 0xF3, 0, 0x0F, 0x5C, -1, XF_XX, 0, 0, 0 }, { "divss", 0xF3, 0, 0x0F, 0x5E, -1, XF_XX, 0, 0, 0 },
    { "sqrtss", 0xF3, 0, 0x0F, 0x51, -1, XF_XX, 0, 0, 0 }, { "cvtss2sd", 0xF3, 0, 0x0F, 0x5A, -1, XF_XX, 0, 0, 0 },
    { "ucomisd", 0x66, 0, 0x0F, 0x2E, -1, XF_XX, 0, 0, 0 }, { "ucomiss", 0, 0, 0x0F, 0x2E, -1, XF_XX, 0, 0, 0 },
    { "cmpeqsd", 0xF2, 0, 0x0F, 0xC2, -1, XF_XXI, 0, 0, 0 }, { "cmpltsd", 0xF2, 0, 0x0F, 0xC2, -1, XF_XXI, 0, 0, 1 },
    { "cmplesd", 0xF2, 0, 0x0F, 0xC2, -1, XF_XXI, 0, 0, 2 }, { "cmpunordsd", 0xF2, 0, 0x0F, 0xC2, -1, XF_XXI, 0, 0, 3 },
    { "cmpneqsd", 0xF2, 0, 0x0F, 0xC2, -1, XF_XXI, 0, 0, 4 }, { "cmpnltsd", 0xF2, 0, 0x0F, 0xC2, -1, XF_XXI, 0, 0, 5 },
    { "cmpnlesd", 0xF2, 0, 0x0F, 0xC2, -1, XF_XXI, 0, 0, 6 }, { "cmpordsd", 0xF2, 0, 0x0F, 0xC2, -1, XF_XXI, 0, 0, 7 },
    { "cmpeqss", 0xF3, 0, 0x0F, 0xC2, -1, XF_XXI, 0, 0, 0 }, { "cmpltss", 0xF3, 0, 0x0F, 0xC2, -1, XF_XXI, 0, 0, 1 },
    { "cmpless", 0xF3, 0, 0x0F, 0xC2, -1, XF_XXI, 0, 0, 2 }, { "cmpunordss", 0xF3, 0, 0x0F, 0xC2, -1, XF_XXI, 0, 0, 3 },
    { "cmpneqss", 0xF3, 0, 0x0F, 0xC2, -1, XF_XXI, 0, 0, 4 }, { "cmpnltss", 0xF3, 0, 0x0F, 0xC2, -1, XF_XXI, 0, 0, 5 },
    { "cmpnless", 0xF3, 0, 0x0F, 0xC2, -1, XF_XXI, 0, 0, 6 }, { "cmpordss", 0xF3, 0, 0x0F, 0xC2, -1, XF_XXI, 0, 0, 7 },
    { "cvtsi2sdq", 0xF2, 1, 0x0F, 0x2A, -1, XF_GX, 64, 0, 0 }, { "cvtsi2ssq", 0xF3, 1, 0x0F, 0x2A, -1, XF_GX, 64, 0, 0 },
    { "cvttsd2si", 0xF2, 1, 0x0F, 0x2C, -1, XF_XG, 64, 0, 0 }, { "cvttss2si", 0xF3, 1, 0x0F, 0x2C, -1, XF_XG, 64, 0, 0 },
    { "movq", 0x66, 1, 0x0F, 0x6E, -1, XF_GX, 64, 0, 0 }, { "movd", 0x66, 0, 0x0F, 0x6E, -1, XF_GX, 32, 0, 0 },
    { "movq", 0x66, 1, 0x0F, 0x7E, -1, XF_XGR, 64, 0, 0 }, { "movd", 0x66, 0, 0x0F, 0x7E, -1, XF_XGR, 32, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};
/* encoding one x86 instruction */
char *at_eo; int at_el;                         /* output, length */
int at_eb(int b) { at_eo[at_el] = b; at_el = at_el + 1; return 0; }
int at_e32(long v) { at_eb(v & 255); at_eb((v >> 8) & 255); at_eb((v >> 16) & 255); at_eb((v >> 24) & 255); return 0; }
int at_fits8(long v) { return v >= -128 && v <= 127; }
int at_fits32(long v) { return v >= -2147483648L && v <= 2147483647L; }
/* the REX byte for: W, reg field, rm operand i (or -1), opcode register (or -1) */
int at_xrexw(int w, int reg, int i, int oreg, int byteregs) {
    int r; int x; int b; int need;
    r = reg >= 8; x = 0; b = 0; need = byteregs;
    if (i >= 0) {
        if (xo_k[i] == XO_REG) b = xo_r[i] >= 8;
        if (xo_k[i] == XO_MEM) { if (xo_base[i] >= 8) b = 1; if (xo_idx[i] >= 8) x = 1; }
    }
    if (oreg >= 8) b = 1;
    if (w || r || x || b || need || at_rex) { at_eb(64 | (w << 3) | (r << 2) | (x << 1) | b); return 1; }
    return 0;
}
/* modrm (+ SIB + displacement) for rm operand i with reg field reg; records
   the rip displacement position.  Returns 0 if the operand cannot be encoded. */
int at_mpos; long at_mdisp; int at_mreloc;      /* rip field: where; value; relocated */
int at_xmodrm(int reg, int i) {
    int b; int mod; long d;
    at_mpos = 0 - 1;
    if (xo_k[i] == XO_REG) { at_eb(192 | ((reg & 7) << 3) | (xo_r[i] & 7)); return 1; }
    if (xo_k[i] != XO_MEM) return 0;
    if (xo_rip[i]) { at_eb(((reg & 7) << 3) | 5); at_mpos = at_el; at_e32(0); return 1; }
    if (xo_xl[i]) return 0;                     /* a symbol in a base+disp operand */
    d = xo_xa[i]; b = xo_base[i];
    if (!at_fits32(d)) return 0;
    if (d == 0 && (b & 7) != 5) mod = 0; else { if (at_fits8(d)) mod = 1; else mod = 2; }
    if (xo_idx[i] >= 0) {
        int s; s = xo_sc[i] == 1 ? 0 : (xo_sc[i] == 2 ? 1 : (xo_sc[i] == 4 ? 2 : 3));
        at_eb((mod << 6) | ((reg & 7) << 3) | 4);
        at_eb((s << 6) | ((xo_idx[i] & 7) << 3) | (b & 7));
    } else {
        at_eb((mod << 6) | ((reg & 7) << 3) | (b & 7));
        if ((b & 7) == 4) at_eb(0x24);
    }
    if (mod == 1) at_eb(d & 255);
    if (mod == 2) at_e32(d);
    return 1;
}
/* a byte register that needs REX (spl..dil), or one that forbids it (ah..bh) */
int at_xbyte(int i, int *forbid) {
    if (xo_k[i] != XO_REG || xo_cls[i] != 8) return 0;
    if (xo_hi[i]) { *forbid = 1; return 0; }
    return xo_r[i] >= 4 && xo_r[i] < 8;
}
/* fill the rip displacement once the length is known; relocations as needed */
int at_xrip(int i, long addr) {
    long end;
    if (at_mpos < 0) return 1;
    end = addr + at_el;
    at_xn = xo_xn[i]; at_xl = xo_xl[i]; at_xa = xo_xa[i];
    if (!at_resolve()) return 0;
    if (at_rsect == 0 - 1) return 0;            /* a bare number: no absolute rip forms */
    if (at_rsect == 1) {
        long d; d = at_rval - end;
        if (!at_fits32(d)) return 0;
        at_eo[at_mpos] = d & 255; at_eo[at_mpos + 1] = (d >> 8) & 255; at_eo[at_mpos + 2] = (d >> 16) & 255; at_eo[at_mpos + 3] = (d >> 24) & 255;
        return 1;
    }
    at_addrel(at_mpos, 2, at_rval - (at_el - at_mpos));     /* R_X86_64_PC32 */
    return 1;
}
int at_regcls(int i, int cls) { return xo_k[i] == XO_REG && xo_cls[i] == cls; }
int at_rmcls(int i, int cls) { return (xo_k[i] == XO_REG && xo_cls[i] == cls) || xo_k[i] == XO_MEM; }
/* try one row; 1 on success (bytes at at_eo) */
int at_xtry(struct at_xrow *r, long addr, int forcelong) {
    int f; int forbid; int byt;
    f = r->form; forbid = 0; at_el = 0; at_nr = 0; at_mpos = 0 - 1;
    if (f == XF_NONE) {
        if (at_nop != 0) return 0;
        if (r->pfx) at_eb(r->pfx);
        if (r->w || at_rex) at_eb(64 | (r->w << 3));
        at_eb(r->op); if (r->op2 >= 0) at_eb(r->op2);
        return 1;
    }
    if (f == XF_RRM || f == XF_RMR || f == XF_MR) {
        int reg; int rm; int rsz; int msz;
        if (at_nop != 2) return 0;
        if (f == XF_RRM) { reg = 0; rm = 1; rsz = r->sz; msz = r->sz; }
        else { reg = 1; rm = 0; rsz = r->sz; msz = r->sz2; }
        if (!at_regcls(reg, rsz)) return 0;
        if (f == XF_MR) { if (xo_k[rm] != XO_MEM) return 0; }
        else { if (!at_rmcls(rm, msz)) return 0; }
        /* reg-reg movq/movl is the 0x89 form, as assemblers pick it */
        if (f == XF_RMR && xo_k[rm] == XO_REG && (r->op == 0x8B)) return 0;
        byt = at_xbyte(reg, &forbid) | at_xbyte(rm, &forbid);
        if (r->pfx) at_eb(r->pfx);
        if (at_xrexw(r->w, xo_r[reg], rm, 0 - 1, byt) && forbid) return 0;
        at_eb(r->op); if (r->op2 >= 0) at_eb(r->op2);
        if (!at_xmodrm(xo_r[reg], rm)) return 0;
        return at_xrip(rm, addr);
    }
    if (f == XF_RM || f == XF_CLRM || f == XF_I8RM || f == XF_I32RM || f == XF_I8RMU) {
        int rm; long imm;
        rm = 0; imm = 0;
        if (f == XF_RM) { if (at_nop != 1) return 0; }
        else {
            if (at_nop != 2) return 0; rm = 1;
            if (f == XF_CLRM) { if (!(at_regcls(0, 8) && xo_r[0] == 1 && xo_hi[0] == 0)) return 0; }
            else {
                if (xo_k[0] != XO_IMM) return 0; imm = xo_imm[0];
                if (f == XF_I8RM && !at_fits8(imm)) return 0;
                if (f == XF_I8RMU && !(imm >= -128 && imm <= 255)) return 0;
                if (f == XF_I32RM && !at_fits32(imm)) return 0;
            }
        }
        if (!at_rmcls(rm, r->sz)) return 0;
        byt = at_xbyte(rm, &forbid);
        if (r->pfx) at_eb(r->pfx);
        if (at_xrexw(r->w, 0, rm, 0 - 1, byt) && forbid) return 0;
        at_eb(r->op); if (r->op2 >= 0) at_eb(r->op2);
        if (!at_xmodrm(r->ext, rm)) return 0;
        if (f == XF_I8RM || f == XF_I8RMU) at_eb(imm & 255);
        if (f == XF_I32RM) at_e32(imm);
        return at_xrip(rm, addr);
    }
    if (f == XF_OI32 || f == XF_OI64) {
        long imm;
        if (at_nop != 2 || xo_k[0] != XO_IMM || !at_regcls(1, r->sz)) return 0;
        imm = xo_imm[0];
        if (f == XF_OI32 && !(imm >= -2147483648L && imm <= 4294967295L)) return 0;
        at_xrexw(r->w, 0, 0 - 1, xo_r[1], 0);
        at_eb(r->op + (xo_r[1] & 7));
        at_e32(imm);
        if (f == XF_OI64) at_e32((unsigned long)imm >> 32);
        return 1;
    }
    if (f == XF_O) {
        if (at_nop != 1 || !at_regcls(0, 64)) return 0;
        at_xrexw(0, 0, 0 - 1, xo_r[0], 0);
        at_eb(r->op + (xo_r[0] & 7));
        return 1;
    }
    if (f == XF_IND) {
        if (at_nop != 1 || xo_k[0] != XO_IND) return 0;
        xo_k[0] = XO_REG;
        at_xrexw(0, 0, 0, 0 - 1, 0);
        at_eb(r->op); at_xmodrm(r->ext, 0);
        xo_k[0] = XO_IND;
        return 1;
    }
    if (f == XF_J8 || f == XF_J32 || f == XF_CALL) {
        long d; int n;
        if (at_nop != 1 || xo_k[0] != XO_TGT) return 0;
        at_xn = xo_xn[0]; at_xl = xo_xl[0]; at_xa = xo_xa[0];
        if (!at_resolve()) return 0;
        if (at_rsect == 0) {                    /* a name defined elsewhere: calls only */
            if (f != XF_CALL) return 0;
            at_eb(0xE8); at_addrel(1, 4, at_rval - 4); at_e32(0);   /* R_X86_64_PLT32 */
            return 1;
        }
        if (at_rsect != 1) return 0;
        n = f == XF_J8 ? 2 : (r->op == 0x0F ? 6 : 5);
        d = at_rval - (addr + n);
        if (f == XF_J8) {
            if (forcelong || at_d32) return 0;
            if (at_unk) d = 0;                  /* first round: forward branches start short */
            if (!at_fits8(d)) return 0;
            at_eb(r->op); at_eb(d & 255); return 1;
        }
        if (at_unk) d = 0;
        if (!at_fits32(d)) return 0;
        at_eb(r->op); if (r->op2 >= 0) at_eb(r->op2); at_e32(d);
        return 1;
    }
    if (f == XF_XX || f == XF_XXI) {
        if (at_nop != 2 || !at_regcls(0, 128) || !at_regcls(1, 128)) return 0;
        if (r->pfx) at_eb(r->pfx);
        at_xrexw(0, xo_r[1], 0, 0 - 1, 0);
        at_eb(r->op); at_eb(r->op2); at_xmodrm(xo_r[1], 0);
        if (f == XF_XXI) at_eb(r->imm);
        return 1;
    }
    if (f == XF_GX || f == XF_XG || f == XF_XGR) {
        int x; int g;
        if (at_nop != 2) return 0;
        if (f == XF_GX) { g = 0; x = 1; } else { x = 0; g = 1; }
        if (!at_regcls(x, 128) || !at_regcls(g, r->sz)) return 0;
        if (r->pfx) at_eb(r->pfx);
        if (f == XF_XG) { at_xrexw(r->w, xo_r[g], x, 0 - 1, 0); at_eb(r->op); at_eb(r->op2); at_xmodrm(xo_r[g], x); }
        else { at_xrexw(r->w, xo_r[x], g, 0 - 1, 0); at_eb(r->op); at_eb(r->op2); at_xmodrm(xo_r[x], g); }
        return 1;
    }
    return 0;
}
/* the x86 line at s[0..n): its bytes at out, length returned; -1 when it is
   not in the subset.  at_short tells the layout whether a short branch form
   was taken (so it can be forced long). */
int at_short; int at_j32;
int at_xenc(char *s, int n, long addr, int forcelong, char *out) {
    int k; int i;
    at_eo = out; at_unk = 0;
    if (!at_split(s, n)) return 0 - 1;
    i = 0; while (i < at_nop) { if (i >= 4 || !at_xop(i, at_op[i], at_opl[i])) return 0 - 1; i = i + 1; }
    k = 0;
    while (at_xt[k].mn) {
        if (at_is(at_mn, at_mnl, at_xt[k].mn)) {
            if (at_xtry(&at_xt[k], addr, forcelong)) { at_short = at_xt[k].form == XF_J8; at_j32 = at_xt[k].form == XF_J32; return at_el; }
        }
        k = k + 1;
    }
    return 0 - 1;
}

/* ---- x86 decoding: bytes -> text in at_lb (checked by at_xenc afterwards) ---- */
int at_xname(int cls, int r, int rexp) {
    at_lc(37);
    if (cls == 64) return at_ls(at_r64[r]);
    if (cls == 32) return at_ls(at_r32[r]);
    if (cls == 16) return at_ls(at_r16[r]);
    if (cls == 128) { at_ls("xmm"); return at_ldec(r); }
    if (r >= 4 && r < 8 && !rexp) return at_ls(at_r8h[r - 4]);
    return at_ls(at_r8[r]);
}
/* relocation context for the instruction being decoded */
long at_dbase;                                  /* text offset of the instruction */
int at_drel; long at_drelo; int at_dtype; int at_dsym; long at_dadd;   /* a reloc in it */
char *at_dsnm[AT_MAXS]; int at_dsnl[AT_MAXS];   /* ELF symbol names, by index */
int at_dsshn[AT_MAXS]; long at_dsval[AT_MAXS];
char *at_tx; long at_txl;                       /* the text being decoded */
char at_tgt[AT_MAXT]; char at_bnd[AT_MAXT];     /* label wanted at (1 branch target, 2 symbol); instruction starts at */
/* print a target: section+offset, an ELF symbol, or a text label */
int at_ltgt(int sym, long add) {
    if (sym == 1 && add >= 0 && add <= at_txl) at_tgt[add] = at_tgt[add] | 1;   /* a label is printed there */
    if (sym == 1) { at_ls(".L"); { char d[24]; int n; unsigned long v; v = add; n = 0; if (v == 0) { d[0] = 48; n = 1; } while (v) { d[n] = "0123456789abcdef"[v & 15]; v = v >> 4; n = n + 1; } while (n > 0) { n = n - 1; at_lc(d[n]); } } return 0; }
    if (sym == 2) at_ls(".Ldata"); else { if (sym == 3) at_ls(".Lbss"); else at_lsn(at_dsnm[sym], at_dsnl[sym]); }
    if (add > 0) { at_lc(43); at_lhex(add); }
    if (add < 0) { at_lc(45); at_lhex(0 - add); }
    return 0;
}
int at_xmemprint(char *p, int len, int *k, int mod, int rm, int rex, int *ok) {
    int b; int idx; int sc; long d;
    if (rm == 4) {
        int sib; if (*k >= len) { *ok = 0; return 0; }
        sib = p[*k] & 255; *k = *k + 1;
        sc = 1 << (sib >> 6); idx = ((sib >> 3) & 7) | ((rex & 2) << 2); b = (sib & 7) | ((rex & 1) << 3);
        if (idx == 4) idx = 0 - 1;
        if ((sib & 7) == 5 && mod == 0) { *ok = 0; return 0; }
    } else { b = rm | ((rex & 1) << 3); idx = 0 - 1; sc = 1; }
    if (mod == 0 && rm == 5) {                  /* rip */
        long fo; fo = *k;
        if (*k + 4 > len) { *ok = 0; return 0; }
        d = (int)at_rd(p + *k, 4); *k = *k + 4;
        /* displacement is relative to the END: printed once the length is known */
        *ok = 2; return (int)fo;
    }
    d = 0;
    if (mod == 1) { if (*k >= len) { *ok = 0; return 0; } d = (signed char)p[*k]; *k = *k + 1; }
    if (mod == 2) { if (*k + 4 > len) { *ok = 0; return 0; } d = (int)at_rd(p + *k, 4); *k = *k + 4; }
    if (d) at_lnum(d);
    at_lc(40); at_xname(64, b, 1);
    if (idx >= 0) { at_lc(44); at_xname(64, idx, 1); at_lc(44); at_ldec(sc); }
    at_lc(41);
    return 0;
}
/* decode one x86 instruction at text offset off; 0 if unknown, else length */
int at_xdec(long off) {
    char *p; int len; int k; int pfx; int rex; int op; int op2; int r; int m;
    p = at_tx + off; len = (int)(at_txl - off); if (len > 15) len = 15;
    k = 0; pfx = 0; rex = 0;
    if (k < len && ((p[k] & 255) == 0x66 || (p[k] & 255) == 0xF2 || (p[k] & 255) == 0xF3)) { pfx = p[k] & 255; k = k + 1; }
    if (k < len && (p[k] & 0xF0) == 0x40) { rex = p[k] & 15; rex = rex | 16; k = k + 1; }
    if (k >= len) return 0;
    op = p[k] & 255; k = k + 1; op2 = 0 - 1;
    if (op == 0x0F) { if (k >= len) return 0; op2 = p[k] & 255; k = k + 1; }
    m = 0;
    while (at_xt[m].mn) {
        struct at_xrow *t; int f;
        t = &at_xt[m]; f = t->form;
        if (t->pfx == pfx && t->op2 == op2 && (t->w == ((rex >> 3) & 1))) {
            int opm; opm = (f == XF_OI32 || f == XF_OI64 || f == XF_O) ? (op & 0xF8) : op;
            if (opm == t->op) {
                int kk; int ok; int ripf; int mod; int reg; int rm; int modrm;
                kk = k; ok = 1; ripf = 0 - 1; at_ln = 0;
                if (rex == 16) at_ls("{rex} ");
                if (f == XF_NONE) { at_ls(t->mn); return kk; }
                if (f == XF_OI32 || f == XF_OI64) {
                    long imm; int rr; rr = (op & 7) | ((rex & 1) << 3);
                    if (f == XF_OI32) { if (kk + 4 > len) return 0; imm = (long)at_rd(p + kk, 4); kk = kk + 4; }
                    else { if (kk + 8 > len) return 0; imm = (long)at_rd(p + kk, 8); kk = kk + 8; }
                    at_ls(t->mn); at_lc(9); at_lc(36); at_lnum(imm); at_ls(", "); at_xname(t->sz, rr, 1);
                    return kk;
                }
                if (f == XF_O) { at_ln = 0; if (rex == 16) at_ls("{rex} "); at_ls(t->mn); at_lc(9); at_xname(64, (op & 7) | ((rex & 1) << 3), 1); return kk; }
                if (f == XF_J8 || f == XF_J32 || f == XF_CALL) {
                    long d; long tg;
                    if (f == XF_J8) { if (kk >= len) return 0; d = (signed char)p[kk]; kk = kk + 1; }
                    else {
                        if (kk + 4 > len) return 0;
                        if (at_drel && at_drelo == off + kk) {          /* call to a name defined elsewhere */
                            if (f != XF_CALL || at_dtype != 4 || at_dadd != -4 || at_dsym < 4) return 0;
                            at_ls(t->mn); at_lc(9); at_lsn(at_dsnm[at_dsym], at_dsnl[at_dsym]); at_ls("@PLT");
                            return kk + 4;
                        }
                        d = (int)at_rd(p + kk, 4); kk = kk + 4;
                    }
                    tg = off + kk + d;
                    if (tg < 0 || tg > at_txl) return 0;
                    at_tgt[tg] = at_tgt[tg] | 1;
                    /* a long branch whose target a short one reaches: an
                       assembler would shorten it, so say it stays long */
                    if (f == XF_J32 && (at_fits8(d) || at_fits8(tg - (off + 2)))) at_ls("{disp32} ");
                    at_ls(t->mn); at_lc(9); at_ltgt(1, tg);
                    return kk;
                }
                if (kk >= len) { m = m + 1; continue; }
                modrm = p[kk] & 255; kk = kk + 1;
                mod = modrm >> 6; reg = ((modrm >> 3) & 7) | ((rex & 4) << 1); rm = modrm & 7;
                if (t->ext >= 0 && ((modrm >> 3) & 7) != t->ext) { m = m + 1; continue; }
                if (f == XF_IND && mod != 3) { m = m + 1; continue; }
                /* operands, printed into a scratch area then assembled in AT&T order */
                {   char a1[160]; int l1; char a2[160]; int l2; int save; long imm; int fo;
                    l1 = 0; l2 = 0; fo = 0 - 1; imm = 0;
                    /* the rm operand */
                    save = at_ln;
                    if (mod == 3) {
                        int cls; int rr; rr = rm | ((rex & 1) << 3);
                        cls = t->sz;
                        if (f == XF_RMR) cls = t->sz2;
                        if (f == XF_XX || f == XF_XXI || f == XF_XG) cls = 128;
                        if (f == XF_IND) at_lc(42);
                        at_xname(cls, rr, rex != 0);
                    } else {
                        if (f == XF_XX || f == XF_XXI || f == XF_XG || f == XF_GX || f == XF_XGR || f == XF_IND) { m = m + 1; continue; }
                        ok = 1;
                        fo = at_xmemprint(p, len, &kk, mod, rm, rex, &ok);
                        if (ok == 0) return 0;
                        if (ok == 2) ripf = fo;
                    }
                    l1 = at_ln - save; { int q; q = 0; while (q < l1) { a1[q] = at_lb[save + q]; q = q + 1; } } at_ln = save;
                    if (f == XF_I8RM) { if (kk >= len) return 0; imm = (signed char)p[kk]; kk = kk + 1; }
                    if (f == XF_I8RMU) { if (kk >= len) return 0; imm = p[kk] & 255; kk = kk + 1; }
                    if (f == XF_I32RM) { if (kk + 4 > len) return 0; imm = (int)at_rd(p + kk, 4); kk = kk + 4; }
                    if (f == XF_XXI) { if (kk >= len || (p[kk] & 255) != t->imm) { m = m + 1; continue; } kk = kk + 1; }
                    /* now a rip operand can be printed: the end is known */
                    if (ripf >= 0) {
                        long d; d = (int)at_rd(p + ripf, 4);
                        at_ln = 0;
                        if (at_drel && at_drelo == off + ripf) {
                            if (at_dtype != 2 || d != 0) return 0;
                            at_ltgt(at_dsym, at_dadd + (kk - ripf));
                        } else {
                            long tg; tg = off + kk + d;
                            if (tg < 0 || tg > at_txl) return 0;
                            at_tgt[tg] = at_tgt[tg] | 1; at_ltgt(1, tg);
                        }
                        at_ls("(%rip)");
                        l1 = at_ln; { int q; q = 0; while (q < l1) { a1[q] = at_lb[q]; q = q + 1; } }
                        at_ln = 0;
                    }
                    /* the reg operand */
                    if (f == XF_RRM || f == XF_RMR || f == XF_MR) at_xname(t->sz, reg, rex != 0);
                    if (f == XF_XX || f == XF_XXI || f == XF_GX || f == XF_XGR) at_xname(128, reg, 1);
                    if (f == XF_XG) at_xname(t->sz, reg, 1);
                    l2 = at_ln - save; { int q; q = 0; while (q < l2) { a2[q] = at_lb[save + q]; q = q + 1; } }
                    at_ln = 0;
                    if (rex == 16) at_ls("{rex} ");
                    at_ls(t->mn); at_lc(9);
                    if (f == XF_RRM) { at_lsn(a2, l2); at_ls(", "); at_lsn(a1, l1); }
                    else { if (f == XF_RMR || f == XF_MR || f == XF_XX || f == XF_XXI || f == XF_GX || f == XF_XG) { at_lsn(a1, l1); at_ls(", "); at_lsn(a2, l2); }
                    else { if (f == XF_XGR) { at_lsn(a2, l2); at_ls(", "); at_lsn(a1, l1); }
                    else { if (f == XF_CLRM) { at_ls("%cl, "); at_lsn(a1, l1); }
                    else { if (f == XF_I8RM || f == XF_I8RMU || f == XF_I32RM) { at_lc(36); at_lnum(imm); at_ls(", "); at_lsn(a1, l1); }
                    else at_lsn(a1, l1); } } } }
                    return kk;
                }
            }
        }
        m = m + 1;
    }
    return 0;
}

/* ==== arm64 ==== */
/* operand kinds in a row's spec string, one char each:
   d n m a  : X register at Rd/Rn/Rm/Ra (31 = xzr)
   D N      : X register or sp at Rd/Rn (31 = sp)
   w W M    : W register at Rd/Rn/Rm (31 = wzr)
   f g h    : D (double) register at Rd/Rn/Rm
   s t u    : S (single) register at Rd/Rn/Rm
   i        : #imm12 [21:10]          k : #imm16 [20:5]       K : #-(imm16+1) (movn)
   H        : optional ", lsl #16*hw" [22:21]
   S        : optional ", lsl #imm6" [15:10] (shifted register)
   8 4 2 1  : [Xn|sp, #imm12*size] (scaled unsigned offset)
   U        : [Xn|sp, #simm9] [20:12]
   R        : [Xn|sp, Xm, lsl #3]
   c        : condition for cset/csetm (the inverted field [15:12])
   B        : b.cond's condition in the mnemonic ("b.?")
   l        : label imm19 [23:5]       L : label imm26 [25:0]
   A        : adr label                P : adrp target (always relocated)
   Z        : #shift for lsl (ubfm)    Y : #shift for lsr/asr (immr, imms = 63)
   b        : logical (bitmask) immediate, 64-bit
   o        : `:lo12:` target of add (relocated) */
struct at_arow { unsigned int mask; unsigned int match; char *mn; char *spec; };
struct at_arow at_at[] = {
    { 0xFFE0FFE0, 0xAA0003E0, "mov", "dm" },
    { 0xFFFFFC00, 0x91000000, "mov", "DN" },
    { 0xFFC00000, 0x91000000, "add", "DNi" }, { 0xFFC00000, 0x91000000, "add", "DNo" },
    { 0xFFC0001F, 0xF100001F, "cmp", "Ni" }, { 0xFFC0001F, 0x7100001F, "cmp", "Wi" },
    { 0xFFC00000, 0xD1000000, "sub", "DNi" },
    { 0xFFE0001F, 0xEB00001F, "cmp", "nmS" },
    { 0xFFE00000, 0x8B000000, "add", "dnmS" }, { 0xFFE00000, 0xCB000000, "sub", "dnmS" },
    { 0xFFE00000, 0x8A000000, "and", "dnmS" }, { 0xFFE00000, 0xAA000000, "orr", "dnmS" },
    { 0xFFE00000, 0xCA000000, "eor", "dnmS" },
    { 0xFF800000, 0x92000000, "and", "Dnb" }, { 0xFF800000, 0xB2000000, "orr", "Dnb" },
    { 0xFF800000, 0xD2000000, "eor", "Dnb" },
    { 0xFFE0FC00, 0x9B007C00, "mul", "dnm" }, { 0xFFE08000, 0x9B008000, "msub", "dnma" },
    { 0xFFE08000, 0x9B000000, "madd", "dnma" },
    { 0xFFE0FC00, 0x9AC00800, "udiv", "dnm" }, { 0xFFE0FC00, 0x9AC00C00, "sdiv", "dnm" },
    { 0xFFE0FC00, 0x9AC02000, "lsl", "dnm" }, { 0xFFE0FC00, 0x9AC02400, "lsr", "dnm" },
    { 0xFFE0FC00, 0x9AC02800, "asr", "dnm" },
    { 0xFFFF0FE0, 0x9A9F07E0, "cset", "dc" }, { 0xFFFF0FE0, 0xDA9F03E0, "csetm", "dc" },
    { 0xFFFFFC00, 0x93401C00, "sxtb", "dW" }, { 0xFFFFFC00, 0x93403C00, "sxth", "dW" },
    { 0xFFFFFC00, 0x93407C00, "sxtw", "dW" },
    { 0xFFC00000, 0xD3400000, "lsl", "dnZ" }, { 0xFFC0FC00, 0xD340FC00, "lsr", "dnY" },
    { 0xFFC0FC00, 0x9340FC00, "asr", "dnY" },
    { 0xFFE00000, 0xD2800000, "mov", "dk" }, { 0xFF800000, 0xD2800000, "movz", "dkH" },
    { 0xFFE00000, 0x92800000, "mov", "dK" }, { 0xFF800000, 0x92800000, "movn", "dkH" },
    { 0xFF800000, 0xF2800000, "movk", "dkH" },
    { 0xFFC00000, 0xF9400000, "ldr", "d8" }, { 0xFFC00000, 0xF9000000, "str", "d8" },
    { 0xFFC00000, 0xB9800000, "ldrsw", "d4" }, { 0xFFC00000, 0x79800000, "ldrsh", "d2" },
    { 0xFFC00000, 0x39800000, "ldrsb", "d1" }, { 0xFFC00000, 0xB9400000, "ldr", "w4" },
    { 0xFFC00000, 0x79400000, "ldrh", "w2" }, { 0xFFC00000, 0x39400000, "ldrb", "w1" },
    { 0xFFC00000, 0xB9000000, "str", "w4" }, { 0xFFC00000, 0x79000000, "strh", "w2" },
    { 0xFFC00000, 0x39000000, "strb", "w1" },
    { 0xFFE00C00, 0xF8400000, "ldur", "dU" }, { 0xFFE00C00, 0xF8000000, "stur", "dU" },
    { 0xFFE00C00, 0xB8800000, "ldursw", "dU" }, { 0xFFE00C00, 0x78800000, "ldursh", "dU" },
    { 0xFFE00C00, 0x38800000, "ldursb", "dU" }, { 0xFFE00C00, 0xB8400000, "ldur", "wU" },
    { 0xFFE00C00, 0x78400000, "ldurh", "wU" }, { 0xFFE00C00, 0x38400000, "ldurb", "wU" },
    { 0xFFE00C00, 0xB8000000, "stur", "wU" }, { 0xFFE00C00, 0x78000000, "sturh", "wU" },
    { 0xFFE00C00, 0x38000000, "sturb", "wU" },
    { 0xFFE0FC00, 0xF8607800, "ldr", "dR" }, { 0xFFE0FC00, 0xF8207800, "str", "dR" },
    { 0xFC000000, 0x14000000, "b", "L" }, { 0xFC000000, 0x94000000, "bl", "L" },
    { 0xFF000000, 0xB4000000, "cbz", "dl" }, { 0xFF000000, 0xB5000000, "cbnz", "dl" },
    { 0xFF000000, 0x34000000, "cbz", "wl" }, { 0xFF000000, 0x35000000, "cbnz", "wl" },
    { 0xFF000010, 0x54000000, "b.?", "Bl" },
    { 0x9F000000, 0x10000000, "adr", "dA" }, { 0x9F000000, 0x90000000, "adrp", "dP" },
    { 0xFFFFFC1F, 0xD63F0000, "blr", "n" }, { 0xFFFFFC1F, 0xD61F0000, "br", "n" },
    { 0xFFFFFFFF, 0xD65F03C0, "ret", "" }, { 0xFFFFFC1F, 0xD65F0000, "ret", "n" },
    { 0xFFE0001F, 0xD4000001, "svc", "k" }, { 0xFFE0001F, 0xD4200000, "brk", "k" },
    { 0xFFFFFFFF, 0xD503201F, "nop", "" },
    { 0xFFE0FC00, 0x1E602800, "fadd", "fgh" }, { 0xFFE0FC00, 0x1E603800, "fsub", "fgh" },
    { 0xFFE0FC00, 0x1E600800, "fmul", "fgh" }, { 0xFFE0FC00, 0x1E601800, "fdiv", "fgh" },
    { 0xFFE0FC00, 0x1E202800, "fadd", "stu" }, { 0xFFE0FC00, 0x1E203800, "fsub", "stu" },
    { 0xFFE0FC00, 0x1E200800, "fmul", "stu" }, { 0xFFE0FC00, 0x1E201800, "fdiv", "stu" },
    { 0xFFE0FC1F, 0x1E602000, "fcmp", "gh" }, { 0xFFE0FC1F, 0x1E202000, "fcmp", "tu" },
    { 0xFFFFFC00, 0x9E670000, "fmov", "fn" }, { 0xFFFFFC00, 0x9E660000, "fmov", "dg" },
    { 0xFFFFFC00, 0x1E270000, "fmov", "sW" }, { 0xFFFFFC00, 0x1E260000, "fmov", "wt" },
    { 0xFFFFFC00, 0x9E620000, "scvtf", "fn" }, { 0xFFFFFC00, 0x9E630000, "ucvtf", "fn" },
    { 0xFFFFFC00, 0x9E220000, "scvtf", "sn" }, { 0xFFFFFC00, 0x9E230000, "ucvtf", "sn" },
    { 0xFFFFFC00, 0x9E780000, "fcvtzs", "dg" }, { 0xFFFFFC00, 0x9E790000, "fcvtzu", "dg" },
    { 0xFFFFFC00, 0x1E22C000, "fcvt", "ft" }, { 0xFFFFFC00, 0x1E624000, "fcvt", "sg" },
    { 0xFFFFFC00, 0x1E61C000, "fsqrt", "fg" }, { 0xFFFFFC00, 0x1E21C000, "fsqrt", "st" },
    { 0, 0, 0, 0 }
};
char *at_cond[16] = { "eq", "ne", "hs", "lo", "mi", "pl", "vs", "vc", "hi", "ls", "ge", "lt", "gt", "le", "al", "nv" };
/* DecodeBitMasks for a 64-bit logical immediate; 0 when the fields are not valid */
int at_bmdec(int nbit, int immr, int imms, unsigned long *out) {
    int len; int esize; int levels; int s; int r; unsigned long welem; unsigned long v; int k;
    len = 6; if (nbit == 0) { len = 5; while (len > 0 && ((imms >> len) & 1)) len = len - 1; }
    if (nbit == 0 && len < 1) return 0;
    esize = 1 << len; levels = esize - 1;
    s = imms & levels; r = immr & levels;
    if (s == levels) return 0;
    welem = (s + 1 == 64) ? ~(unsigned long)0 : (((unsigned long)1 << (s + 1)) - 1);
    if (r) { welem = (welem >> r) | (welem << (esize - r)); }
    if (esize < 64) welem = welem & (((unsigned long)1 << esize) - 1);
    v = 0; k = 0; while (k < 64) { v = v | (welem << k); k = k + esize; }
    *out = v;
    return 1;
}
int at_bmenc(unsigned long v, unsigned int *field) {   /* N:immr:imms at [22:10], by search */
    int nbit; int immr; int imms; unsigned long x;
    nbit = 0;
    while (nbit < 2) {
        imms = 0;
        while (imms < 64) {
            immr = 0;
            while (immr < 64) {
                if (at_bmdec(nbit, immr, imms, &x) && x == v) { *field = (nbit << 22) | (immr << 16) | (imms << 10); return 1; }
                immr = immr + 1;
            }
            imms = imms + 1;
        }
        nbit = nbit + 1;
    }
    return 0;
}
/* ---- arm64 operand parse helpers ---- */
int at_areg(char *s, int n, int want) {       /* want: 'x' 'X'(sp ok) 'w' 'd' 's'; register number or -1 */
    long v;
    if (n < 2) return 0 - 1;
    if (want == 120 || want == 88) {
        if (at_is(s, n, "xzr")) return want == 120 ? 31 : 0 - 1;
        if (at_is(s, n, "sp")) return want == 88 ? 31 : 0 - 1;
        if (s[0] != 120) return 0 - 1;
    } else {
        if (want == 119 && at_is(s, n, "wzr")) return 31;
        if (s[0] != want) return 0 - 1;
    }
    if (!at_num(s + 1, n - 1, &v) || v < 0 || v > 30 || (s[1] == 48 && n > 2)) {
        if (!((want == 100 || want == 115) && at_num(s + 1, n - 1, &v) && v == 31)) return 0 - 1;
    }
    return (int)v;
}
int at_aimm(char *s, int n, long *v) {          /* #number */
    if (n < 2 || s[0] != 35) return 0;
    return at_num(s + 1, n - 1, v);
}
/* "lsl #N" */
int at_alsl(char *s, int n, long *v) {
    if (n < 6 || !at_is(s, 4, "lsl ")) return 0;
    s = s + 4; n = n - 4; while (n > 0 && at_sp(s[0])) { s = s + 1; n = n - 1; }
    return at_aimm(s, n, v);
}
/* memory "[base]" / "[base, #off]" / "[base, xm, lsl #3]": fills at_mb, at_mo, at_mm */
int at_mb; long at_mo; int at_mm;
int at_amem(char *s, int n, int kind) {
    int k; int e; char *q; int ql;
    if (n < 4 || s[0] != 91 || s[n - 1] != 93) return 0;
    s = s + 1; n = n - 2;
    k = 0; while (k < n && s[k] != 44) k = k + 1;
    e = k; while (e > 0 && at_sp(s[e - 1])) e = e - 1;
    at_mb = at_areg(s, e, 88); if (at_mb < 0) return 0;
    at_mo = 0; at_mm = 0 - 1;
    if (k == n) return kind != 82;
    q = s + k + 1; ql = n - k - 1; while (ql > 0 && at_sp(q[0])) { q = q + 1; ql = ql - 1; }
    if (kind == 82) {
        long sh; int c;
        c = 0; while (c < ql && q[c] != 44) c = c + 1;
        { int ce; ce = c; while (ce > 0 && at_sp(q[ce - 1])) ce = ce - 1; at_mm = at_areg(q, ce, 120); }
        if (at_mm < 0 || c >= ql) return 0;
        q = q + c + 1; ql = ql - c - 1; while (ql > 0 && at_sp(q[0])) { q = q + 1; ql = ql - 1; }
        if (!at_alsl(q, ql, &sh) || sh != 3) return 0;
        return 1;
    }
    return at_aimm(q, ql, &at_mo);
}
/* encode the arm64 line at s[0..n) as a word; -1 when not in the subset */
unsigned int at_aw;
int at_atry(struct at_arow *r, long addr) {
    char *sp; int oi; unsigned int w; int c;
    w = r->match; at_nr = 0;
    sp = r->spec; oi = 0;
    /* b.cond: the condition is in the mnemonic */
    if (r->mn[0] == 98 && r->mn[1] == 46) {
        int q; if (at_mnl < 3 || at_mn[1] != 46) return 0;
        q = 0; while (q < 16 && !at_is(at_mn + 2, at_mnl - 2, at_cond[q])) q = q + 1;
        if (q >= 14) return 0;
        w = w | q;
    } else { if (!at_is(at_mn, at_mnl, r->mn)) return 0; }
    while (*sp) {
        char *o; int ol; c = *sp;
        if (c == 66) { sp = sp + 1; continue; }   /* B: done above */
        if (c == 72 || c == 83) {               /* optional shift operand */
            long v;
            if (oi >= at_nop) { sp = sp + 1; continue; }
            if (!at_alsl(at_op[oi], at_opl[oi], &v)) return 0;
            if (c == 72) { if (v < 0 || v > 48 || v % 16) return 0; w = w | ((unsigned int)(v / 16) << 21); }
            else { if (v < 0 || v > 63) return 0; w = w | ((unsigned int)v << 10); }
            oi = oi + 1; sp = sp + 1; continue;
        }
        if (oi >= at_nop) return 0;
        o = at_op[oi]; ol = at_opl[oi];
        if (c == 100 || c == 110 || c == 109 || c == 97 || c == 68 || c == 78 || c == 119 || c == 87 || c == 77
            || c == 102 || c == 103 || c == 104 || c == 115 || c == 116 || c == 117) {
            int want; int sh; int rr;
            want = 120;
            if (c == 68 || c == 78) want = 88;
            if (c == 119 || c == 87 || c == 77) want = 119;
            if (c == 102 || c == 103 || c == 104) want = 100;
            if (c == 115 || c == 116 || c == 117) want = 115;
            sh = 0;
            if (c == 110 || c == 78 || c == 87 || c == 103 || c == 116) sh = 5;
            if (c == 109 || c == 77 || c == 104 || c == 117) sh = 16;
            if (c == 97) sh = 10;
            rr = at_areg(o, ol, want); if (rr < 0) return 0;
            w = w | ((unsigned int)rr << sh);
        } else { if (c == 105) {                 /* #imm12 */
            long v; if (!at_aimm(o, ol, &v) || v < 0 || v > 4095) return 0;
            w = w | ((unsigned int)v << 10);
        } else { if (c == 111) {                 /* :lo12:expr */
            if (ol < 7 || !at_is(o, 6, ":lo12:")) return 0;
            if (!at_expr(o + 6, ol - 6) || at_xl == 0) return 0;
            if (!at_resolve()) return 0;
            at_addrel(0, 277, at_rsect == 0 ? at_rval : at_rval);
        } else { if (c == 107) {                 /* #imm16 */
            long v; if (!at_aimm(o, ol, &v) || v < 0 || v > 65535) return 0;
            w = w | ((unsigned int)v << 5);
        } else { if (c == 75) {                  /* #-(imm16+1) */
            long v; if (!at_aimm(o, ol, &v) || v > -1 || v < -65536) return 0;
            w = w | ((unsigned int)(0 - v - 1) << 5);
        } else { if (c == 56 || c == 52 || c == 50 || c == 49) {
            int size; size = c - 48;
            if (!at_amem(o, ol, 0)) return 0;
            if (at_mo < 0 || at_mo % size || at_mo / size > 4095) return 0;
            w = w | ((unsigned int)(at_mo / size) << 10) | ((unsigned int)at_mb << 5);
        } else { if (c == 85) {
            if (!at_amem(o, ol, 0)) return 0;
            if (at_mo < -256 || at_mo > 255) return 0;
            w = w | ((unsigned int)(at_mo & 0x1FF) << 12) | ((unsigned int)at_mb << 5);
        } else { if (c == 82) {
            if (!at_amem(o, ol, 82)) return 0;
            w = w | ((unsigned int)at_mm << 16) | ((unsigned int)at_mb << 5);
        } else { if (c == 99) {                  /* condition; the field holds its inverse */
            int q; q = 0; while (q < 14 && !at_is(o, ol, at_cond[q])) q = q + 1;
            if (q >= 14) return 0;
            w = w | ((unsigned int)(q ^ 1) << 12);
        } else { if (c == 108 || c == 76 || c == 65) {   /* labels */
            long d; int bits;
            if (!at_expr(o, ol) || at_xl == 0) return 0;
            if (!at_resolve()) return 0;
            if (at_rsect == 0 && c == 76 && r->match == 0x94000000) {   /* bl to a name defined elsewhere */
                at_addrel(0, 283, at_rval);
            } else {
                if (at_rsect != 1) return 0;
                d = at_unk ? 0 : at_rval - addr;
                if (c == 65) {
                    if (d < -1048576 || d > 1048575) return 0;
                    w = w | ((unsigned int)(d & 3) << 29) | ((unsigned int)((d >> 2) & 0x7FFFF) << 5);
                } else {
                    if (d & 3) return 0;
                    bits = c == 76 ? 26 : 19;
                    d = d >> 2;
                    if (d < -((long)1 << (bits - 1)) || d >= ((long)1 << (bits - 1))) return 0;
                    if (c == 76) w = w | (unsigned int)(d & 0x3FFFFFF); else w = w | ((unsigned int)(d & 0x7FFFF) << 5);
                }
            }
        } else { if (c == 80) {                  /* adrp: always relocated */
            if (!at_expr(o, ol) || at_xl == 0) return 0;
            if (!at_resolve()) return 0;
            at_addrel(0, 275, at_rval);
        } else { if (c == 90) {                  /* lsl #s = ubfm immr=-s, imms=63-s */
            long v; if (!at_aimm(o, ol, &v) || v < 1 || v > 63) return 0;
            w = w | ((unsigned int)((64 - v) & 63) << 16) | ((unsigned int)(63 - v) << 10);
        } else { if (c == 89) {
            long v; if (!at_aimm(o, ol, &v) || v < 0 || v > 63) return 0;
            w = w | ((unsigned int)v << 16);
        } else { if (c == 98) {
            long v; unsigned int fld; if (!at_aimm(o, ol, &v)) return 0;
            if (!at_bmenc((unsigned long)v, &fld)) return 0;
            w = w | fld;
        } else return 0; } } } } } } } } } } } } }
        oi = oi + 1; sp = sp + 1;
    }
    if (oi != at_nop) return 0;
    at_aw = w;
    return 1;
}
int at_aenc(char *s, int n, long addr, char *out) {
    int k;
    at_eo = out; at_el = 0; at_unk = 0;
    if (!at_split(s, n) || at_rex) return 0 - 1;
    k = 0;
    while (at_at[k].mn) {
        if (at_atry(&at_at[k], addr)) {
            /* relocated fields carry zero (the linker fills them) */
            at_e32(at_aw);
            return 4;
        }
        k = k + 1;
    }
    return 0 - 1;
}
/* ---- arm64 decoding into at_lb ---- */
int at_aname(int kind, int r) {
    if (kind == 120) { if (r == 31) return at_ls("xzr"); at_lc(120); return at_ldec(r); }
    if (kind == 88) { if (r == 31) return at_ls("sp"); at_lc(120); return at_ldec(r); }
    if (kind == 119) { if (r == 31) return at_ls("wzr"); at_lc(119); return at_ldec(r); }
    at_lc(kind); return at_ldec(r);
}
int at_adec_row(struct at_arow *r, unsigned int w, long off) {
    char *sp; int first; int c;
    at_ln = 0;
    if (r->mn[0] == 98 && r->mn[1] == 46) { at_ls("b."); if ((w & 15) >= 14) return 0; at_ls(at_cond[w & 15]); }
    else at_ls(r->mn);
    sp = r->spec; first = 1;
    while (*sp) {
        c = *sp;
        if (c == 66) { sp = sp + 1; continue; }
        if (c == 72) { int hw; hw = (w >> 21) & 3; if (hw) { at_ls(", lsl #"); at_ldec(hw * 16); } sp = sp + 1; continue; }
        if (c == 83) { int a; a = (w >> 10) & 63; if (a) { at_ls(", lsl #"); at_ldec(a); } sp = sp + 1; continue; }
        if (first) at_lc(9); else at_ls(", ");
        first = 0;
        if (c == 100) at_aname(120, w & 31);
        if (c == 110) at_aname(120, (w >> 5) & 31);
        if (c == 109) at_aname(120, (w >> 16) & 31);
        if (c == 97) at_aname(120, (w >> 10) & 31);
        if (c == 68) at_aname(88, w & 31);
        if (c == 78) at_aname(88, (w >> 5) & 31);
        if (c == 119) at_aname(119, w & 31);
        if (c == 87) at_aname(119, (w >> 5) & 31);
        if (c == 77) at_aname(119, (w >> 16) & 31);
        if (c == 102) at_aname(100, w & 31);
        if (c == 103) at_aname(100, (w >> 5) & 31);
        if (c == 104) at_aname(100, (w >> 16) & 31);
        if (c == 115) at_aname(115, w & 31);
        if (c == 116) at_aname(115, (w >> 5) & 31);
        if (c == 117) at_aname(115, (w >> 16) & 31);
        if (c == 105) {
            if (at_drel && at_drelo == off) return 0;   /* the `o` row prints this one */
            at_lc(35); at_lhex((w >> 10) & 4095);
        }
        if (c == 111) {
            if (!(at_drel && at_drelo == off && at_dtype == 277) || ((w >> 10) & 4095)) return 0;
            at_ls(":lo12:"); at_ltgt(at_dsym, at_dadd);
        }
        if (c == 107) { at_lc(35); at_lhex((w >> 5) & 65535); }
        if (c == 75) { at_lc(35); at_lnum(0 - (long)((w >> 5) & 65535) - 1); }
        if (c == 56 || c == 52 || c == 50 || c == 49) {
            long o; o = ((w >> 10) & 4095) * (c - 48);
            at_lc(91); at_aname(88, (w >> 5) & 31); if (o) { at_ls(", #"); at_lhex(o); } at_lc(93);
        }
        if (c == 85) {
            long o; o = (w >> 12) & 511; if (o & 256) o = o - 512;
            at_lc(91); at_aname(88, (w >> 5) & 31); if (o) { at_ls(", #"); at_lnum(o); } at_lc(93);
        }
        if (c == 82) { at_lc(91); at_aname(88, (w >> 5) & 31); at_ls(", "); at_aname(120, (w >> 16) & 31); at_ls(", lsl #3]"); }
        if (c == 99) { int q; q = ((w >> 12) & 15) ^ 1; if (q >= 14) return 0; at_ls(at_cond[q]); }
        if (c == 108 || c == 76 || c == 65) {
            long d; long tg;
            if (c == 76 && at_drel && at_drelo == off) {
                if (at_dtype != 283 || (w & 0x3FFFFFF) || at_dsym < 4) return 0;
                at_ltgt(at_dsym, at_dadd);
            } else {
                if (c == 76) { d = w & 0x3FFFFFF; if (d & 0x2000000) d = d - 0x4000000; d = d * 4; }
                else { if (c == 108) { d = (w >> 5) & 0x7FFFF; if (d & 0x40000) d = d - 0x80000; d = d * 4; }
                else { d = (((w >> 5) & 0x7FFFF) << 2) | ((w >> 29) & 3); if (d & 0x100000) d = d - 0x200000; } }
                tg = off + d;
                if (tg < 0 || tg > at_txl) return 0;
                at_tgt[tg] = at_tgt[tg] | 1; at_ltgt(1, tg);
            }
        }
        if (c == 80) {
            if (!(at_drel && at_drelo == off && at_dtype == 275) || (w & 0x60FFFFE0)) return 0;
            at_ltgt(at_dsym, at_dadd);
        }
        if (c == 90) {
            int immr; int imms; immr = (w >> 16) & 63; imms = (w >> 10) & 63;
            if (imms == 63 || ((imms + 1) & 63) != immr) return 0;
            at_lc(35); at_ldec(63 - imms);
        }
        if (c == 89) { at_lc(35); at_ldec((w >> 16) & 63); }
        if (c == 98) {
            unsigned long v; if (!at_bmdec((w >> 22) & 1, (w >> 16) & 63, (w >> 10) & 63, &v)) return 0;
            at_lc(35); at_lhex(v);
        }
        sp = sp + 1;
    }
    return 1;
}

/* ==== the disassembler ==== */
int at_arch;                                    /* 0 x86-64, 1 arm64 */
char at_chk[32];
long at_nfall;                                  /* instructions printed as bytes */
int at_secto(char *o, long n, int i, long *off, long *size, long shoff) {
    char *h; h = o + shoff + 64 * i;
    *off = (long)at_rd(h + 24, 8); *size = (long)at_rd(h + 32, 8);
    if (*off < 0 || *size < 0 || (at_rd(h + 4, 4) != 8 && *off + *size > n)) return 0;
    return 1;
}
/* check the instruction just printed: assembled again it must give these bytes */
long at_rofs[262144]; int at_rty[262144]; int at_rsy[262144]; long at_rad[262144]; int at_nrel;
int at_verify(long off, int len) {
    int got; int k;
    at_dismode = 1;
    if (at_arch) got = at_aenc(at_lb, at_ln, off, at_chk);
    else got = at_xenc(at_lb, at_ln, off, 0, at_chk);
    at_dismode = 0;
    if (got != len) return 0;
    k = 0; while (k < len) { if (at_chk[k] != at_tx[off + k]) return 0; k = k + 1; }
    /* the relocation must agree too */
    if (at_drel) {
        if (at_nr != 1 || off + at_roff[0] != at_drelo || at_rtype[0] != at_dtype || at_radd[0] != at_dadd) return 0;
        if (at_dsym >= 1 && at_dsym <= 3) { if (at_rs[0] != at_dsym) return 0; }
        else { if (at_rs[0] != 0 || at_rnl[0] != at_dsnl[at_dsym]) return 0;
            k = 0; while (k < at_rnl[0]) { if (at_rnm[0][k] != at_dsnm[at_dsym][k]) return 0; k = k + 1; } }
    } else { if (at_nr != 0) return 0; }
    return 1;
}
int at_bytes(char *p, long n, int perline) {
    long k; int c;
    k = 0;
    while (k < n) {
        at_s("\t.byte\t");
        c = 0;
        while (c < perline && k < n) {
            if (c) at_s(", ");
            at_ln = 0; at_lhex(p[k] & 255); at_sn(at_lb, at_ln);
            c = c + 1; k = k + 1;
        }
        at_c(10);
    }
    return 0;
}
/* the instruction at text offset off: length (0: not decodable); text in at_lb */
long at_relcur;
int at_decode(long off) {
    int len; long e;
    /* the relocation inside this instruction, if any */
    at_drel = 0;
    while (at_relcur < at_nrel && at_rofs[at_relcur] < off) at_relcur = at_relcur + 1;
    e = off + (at_arch ? 4 : 15);
    if (at_relcur < at_nrel && at_rofs[at_relcur] < e) {
        at_drel = 1; at_drelo = at_rofs[at_relcur]; at_dtype = at_rty[at_relcur]; at_dsym = at_rsy[at_relcur]; at_dadd = at_rad[at_relcur];
    }
    if (at_arch) {
        unsigned int w; int k;
        if (off + 4 > at_txl) return 0;
        w = (unsigned int)at_rd(at_tx + off, 4);
        k = 0;
        while (at_at[k].mn) {
            if ((w & at_at[k].mask) == at_at[k].match && at_adec_row(&at_at[k], w, off) && at_verify(off, 4)) return 4;
            k = k + 1;
        }
        return 0;
    }
    len = at_xdec(off);
    if (len <= 0) return 0;
    if (at_drel && at_drelo >= off + len) at_drel = 0;
    if (!at_verify(off, len)) return 0;
    return len;
}
int at_dis(char *o, long n) {
    long shoff; int shnum; long toff; long dsz; long doff; long bsz; long roff; long rsz; long soff; long ssz; long stoff; long stsz; long tpoff; long tpsz; long x;
    int i; int nsym; int nloc; long off;
    at_err = 0; at_on = 0; at_nfall = 0;
    if (n < 64 || at_rd(o, 4) != 0x464C457F || (o[4] & 255) != 2 || at_rd(o + 16, 2) != 1) { at_err = "not an ELF relocatable object"; return 1; }
    x = (long)at_rd(o + 18, 2);
    if (x == 62) at_arch = 0; else { if (x == 183) at_arch = 1; else { at_err = "ELF machine is neither x86-64 nor arm64"; return 1; } }
    shoff = (long)at_rd(o + 40, 8); shnum = (int)at_rd(o + 60, 2);
    if ((shnum != 8 && shnum != 9) || shoff + 64 * shnum > n) { at_err = "not an object unisacc wrote (section count)"; return 1; }
    if (!at_secto(o, n, 1, &toff, &at_txl, shoff) || !at_secto(o, n, 2, &doff, &dsz, shoff) || !at_secto(o, n, 3, &x, &bsz, shoff)
        || !at_secto(o, n, 4, &roff, &rsz, shoff) || !at_secto(o, n, 5, &soff, &ssz, shoff) || !at_secto(o, n, 6, &stoff, &stsz, shoff)) { at_err = "section outside the file"; return 1; }
    tpoff = 0; tpsz = 0;
    if (shnum == 9 && !at_secto(o, n, 8, &tpoff, &tpsz, shoff)) { at_err = "section outside the file"; return 1; }
    if (at_txl >= AT_MAXT) { at_err = "text too large"; return 1; }
    at_tx = o + toff;
    nsym = (int)(ssz / 24); nloc = (int)at_rd(o + shoff + 64 * 5 + 44, 4);
    if (nsym > AT_MAXS || nsym < 4) { at_err = "symbol table"; return 1; }
    i = 0;
    while (i < nsym) {
        char *s; long nm; s = o + soff + 24 * i;
        nm = (long)at_rd(s, 4); if (nm >= stsz) { at_err = "symbol name"; return 1; }
        at_dsnm[i] = o + stoff + nm; at_dsnl[i] = at_slen(at_dsnm[i]);
        at_dsshn[i] = (int)at_rd(s + 6, 2); at_dsval[i] = (long)at_rd(s + 8, 8);
        if (i >= 4) {
            int q; q = 0;
            if (at_dsnl[i] == 0 || (at_dsnm[i][0] >= 48 && at_dsnm[i][0] <= 57) || at_local(at_dsnm[i], at_dsnl[i])) { at_err = "a symbol name assembly cannot spell"; return 1; }
            while (q < at_dsnl[i]) { if (!at_idc(at_dsnm[i][q])) { at_err = "a symbol name assembly cannot spell"; return 1; } q = q + 1; }
        }
        i = i + 1;
    }
    at_nrel = (int)(rsz / 24);
    if (at_nrel > 262144) { at_err = "too many relocations"; return 1; }
    i = 0;
    while (i < at_nrel) {
        char *r; r = o + roff + 24 * i;
        at_rofs[i] = (long)at_rd(r, 8); at_rty[i] = (int)at_rd(r + 8, 4); at_rsy[i] = (int)at_rd(r + 12, 4); at_rad[i] = (long)at_rd(r + 16, 8);
        if (at_rsy[i] >= nsym || at_rsy[i] < 1 || (i > 0 && at_rofs[i] <= at_rofs[i - 1])) { at_err = "relocations out of order"; return 1; }
        i = i + 1;
    }
    /* pass 1: instruction starts and branch targets */
    off = 0; while (off <= at_txl) { at_tgt[off] = 0; at_bnd[off] = 0; off = off + 1; }
    at_relcur = 0; off = 0;
    while (off < at_txl) {
        int len; at_bnd[off] = 1;
        len = at_decode(off);
        if (len <= 0) len = at_arch ? 4 : 1;
        if (at_arch && off + 4 > at_txl) len = (int)(at_txl - off);
        off = off + len;
    }
    i = 4; while (i < nsym) { if (at_dsshn[i] == 1 && at_dsval[i] <= at_txl) at_tgt[at_dsval[i]] = at_tgt[at_dsval[i]] | 2; i = i + 1; }
    /* header: the target, the symbol table in its order */
    at_s(at_arch ? "# unisacc -S lnx/arm64\n" : "# unisacc -S lnx/x86_64\n");
    at_s("\t.text\n\t.p2align 4\n");
    i = 4;
    while (i < nsym) {
        int glob; glob = i >= nloc;
        at_s(glob ? "\t.globl\t" : "\t.local\t"); at_sn(at_dsnm[i], at_dsnl[i]); at_c(10);
        if (glob && at_dsshn[i] >= 1 && at_dsshn[i] <= 3) {
            at_s("\t.type\t"); at_sn(at_dsnm[i], at_dsnl[i]);
            at_s(at_dsshn[i] == 1 ? (at_arch ? ",%function\n" : ",@function\n") : (at_arch ? ",%object\n" : ",@object\n"));
        }
        i = i + 1;
    }
    /* pass 2: the text */
    at_relcur = 0; off = 0;
    while (off <= at_txl) {
        int len; int j; int clean;
        if (at_tgt[off] || off == at_txl) {
            i = 4; while (i < nsym) { if (at_dsshn[i] == 1 && at_dsval[i] == off) { at_sn(at_dsnm[i], at_dsnl[i]); at_s(":\n"); } i = i + 1; }
            if (at_tgt[off] & 1) { at_ln = 0; at_ltgt(1, off); at_sn(at_lb, at_ln); at_s(":\n"); }
        }
        if (off == at_txl) break;
        len = at_bnd[off] ? at_decode(off) : 0;
        clean = len > 0;
        if (clean) { j = 1; while (j < len) { if (at_tgt[off + j]) clean = 0; j = j + 1; } }
        if (clean) { at_c(9); at_sn(at_lb, at_ln); at_c(10); off = off + len; continue; }
        at_nfall = at_nfall + 1;
        if (at_arch && off + 4 <= at_txl && !at_tgt[off + 1] && !at_tgt[off + 2] && !at_tgt[off + 3]) {
            at_s("\t.inst\t"); at_ln = 0; at_lhex(at_rd(at_tx + off, 4)); at_sn(at_lb, at_ln); at_c(10); off = off + 4;
            /* a relocation here would be lost */
            while (at_relcur < at_nrel && at_rofs[at_relcur] < off) { if (at_rofs[at_relcur] >= off - 4) { at_err = "a relocated word the tables do not know"; at_flush(); return 1; } at_relcur = at_relcur + 1; }
            continue;
        }
        while (at_relcur < at_nrel && at_rofs[at_relcur] < off) at_relcur = at_relcur + 1;
        if (at_relcur < at_nrel && at_rofs[at_relcur] == off) { at_err = "a relocated field the tables do not know"; at_flush(); return 1; }
        at_bytes(at_tx + off, 1, 1); off = off + 1;
    }
    /* data: bytes, with the symbols defined in it */
    at_s("\t.data\n\t.p2align 4\n.Ldata:\n");
    off = 0;
    while (off <= dsz) {
        long nx;
        i = 4; while (i < nsym) { if (at_dsshn[i] == 2 && at_dsval[i] == off) { at_sn(at_dsnm[i], at_dsnl[i]); at_s(":\n"); } i = i + 1; }
        if (off == dsz) break;
        nx = dsz; i = 4; while (i < nsym) { if (at_dsshn[i] == 2 && at_dsval[i] > off && at_dsval[i] < nx) nx = at_dsval[i]; i = i + 1; }
        at_bytes(o + doff + off, nx - off, 16);
        off = nx;
    }
    at_s("\t.bss\n\t.p2align 4\n.Lbss:\n");
    off = 0;
    while (off <= bsz) {
        long nx;
        i = 4; while (i < nsym) { if (at_dsshn[i] == 3 && at_dsval[i] == off) { at_sn(at_dsnm[i], at_dsnl[i]); at_s(":\n"); } i = i + 1; }
        if (off == bsz) break;
        nx = bsz; i = 4; while (i < nsym) { if (at_dsshn[i] == 3 && at_dsval[i] > off && at_dsval[i] < nx) nx = at_dsval[i]; i = i + 1; }
        at_s("\t.zero\t"); at_ln = 0; at_ldec(nx - off); at_sn(at_lb, at_ln); at_c(10);
        off = nx;
    }
    if (tpsz) {
        long k;
        at_s(at_arch ? "\t.section\t.unisa.tape,\"\",%progbits\n" : "\t.section\t.unisa.tape,\"\",@progbits\n");
        k = 0;
        while (k < tpsz) {
            int c; int col; at_s("\t.ascii\t\""); col = 0;
            while (k < tpsz && col < 96) {
                c = o[tpoff + k] & 255;
                if (c == 34 || c == 92) { at_c(92); at_c(c); }
                else { if (c >= 32 && c < 127) at_c(c);
                else { at_c(92); at_c(48 + ((c >> 6) & 7)); at_c(48 + ((c >> 3) & 7)); at_c(48 + (c & 7)); } }
                col = col + 1; k = k + 1;
            }
            at_s("\"\n");
        }
    }
    at_flush();
    return 0;
}

/* ==== nm: the symbol table, as GNU/LLVM nm prints it (R18-4, 0.0.18) ==== */
int at_nmord[AT_MAXS];
int at_nmlt(int a, int b) {                    /* name a sorts before name b (bytes) */
    int k; k = 0;
    while (k < at_dsnl[a] && k < at_dsnl[b]) {
        if ((at_dsnm[a][k] & 255) != (at_dsnm[b][k] & 255)) return (at_dsnm[a][k] & 255) < (at_dsnm[b][k] & 255);
        k = k + 1;
    }
    return at_dsnl[a] < at_dsnl[b];
}
int at_nm(char *o, long n) {
    long shoff; int shnum; long soff; long ssz; long stoff; long stsz; int nsym; int nloc; int i; int m;
    at_err = 0; at_on = 0;
    if (n < 64 || at_rd(o, 4) != 0x464C457F || (o[4] & 255) != 2 || at_rd(o + 16, 2) != 1) { at_err = "not an ELF relocatable object"; return 1; }
    shoff = (long)at_rd(o + 40, 8); shnum = (int)at_rd(o + 60, 2);
    if ((shnum != 8 && shnum != 9) || shoff + 64 * shnum > n) { at_err = "not an object unisacc wrote (section count)"; return 1; }
    if (!at_secto(o, n, 5, &soff, &ssz, shoff) || !at_secto(o, n, 6, &stoff, &stsz, shoff)) { at_err = "section outside the file"; return 1; }
    nsym = (int)(ssz / 24); nloc = (int)at_rd(o + shoff + 64 * 5 + 44, 4);
    if (nsym > AT_MAXS) { at_err = "symbol table"; return 1; }
    m = 0; i = 4;
    while (i < nsym) {
        char *s; long nm; s = o + soff + 24 * i;
        nm = (long)at_rd(s, 4); if (nm >= stsz) { at_err = "symbol name"; return 1; }
        at_dsnm[i] = o + stoff + nm; at_dsnl[i] = at_slen(at_dsnm[i]);
        at_dsshn[i] = (int)at_rd(s + 6, 2); at_dsval[i] = (long)at_rd(s + 8, 8);
        {   int j; j = m; while (j > 0 && at_nmlt(i, at_nmord[j - 1])) { at_nmord[j] = at_nmord[j - 1]; j = j - 1; } at_nmord[j] = i; }
        m = m + 1; i = i + 1;
    }
    i = 0;
    while (i < m) {
        int j; int c; j = at_nmord[i];
        if (at_dsshn[j] == 0) at_s("                 U ");
        else {
            c = at_dsshn[j] == 1 ? 116 : (at_dsshn[j] == 2 ? 100 : 98);
            if (j >= nloc) c = c - 32;
            {   char d[17]; int k; unsigned long v; v = at_dsval[j]; k = 15;
                while (k >= 0) { d[k] = "0123456789abcdef"[v & 15]; v = v >> 4; k = k - 1; }
                at_sn(d, 16); }
            at_c(32); at_c(c); at_c(32);
        }
        at_sn(at_dsnm[j], at_dsnl[j]); at_c(10);
        i = i + 1;
    }
    at_flush();
    return 0;
}

/* ==== the assembler ==== */
/* items: one per text line that emits (instruction or bytes) or defines a label */
#define AT_MAXI 2097152
char *at_il[AT_MAXI]; int at_ilen[AT_MAXI]; int at_iline[AT_MAXI]; long at_iaddr[AT_MAXI]; char at_ik[AT_MAXI]; char at_ilong[AT_MAXI]; int at_isym[AT_MAXI]; int at_isz[AT_MAXI];
int at_ni;
char at_ttext[AT_MAXT]; long at_tn;              /* final text */
char at_tdata[AT_MAXT]; long at_dn; long at_bn;
char at_ttape[AT_MAXT]; long at_tpn;
long at_aoff[262144]; int at_atype[262144]; int at_asect[262144]; int at_asym[262144]; long at_aadd[262144]; int at_nar;
int at_cursec;                                  /* 1 text 2 data 3 bss 4 tape */
int at_lineno;
int at_aerr(char *m) { at_err = m; return 1; }
/* define a label in the current section at the current offset */
int at_deflabel(char *s, int n, long val) {
    int j; j = at_sym(s, n); if (j < 0) return 1;
    if (at_sdef[j] && !(at_ssect[j] == at_cursec && at_sval[j] == val)) return at_aerr("as: symbol defined twice");
    at_sdef[j] = 1; at_ssect[j] = at_cursec; at_sval[j] = val;
    return 0;
}
/* data directives: bytes into buf at *len */
int at_datadir(char *s, int n, char *buf, long *len, int nobits) {
    int k; k = 0; while (k < n && !at_sp(s[k])) k = k + 1;
    if (at_is(s, k, ".byte")) {
        char *p; int pl; int a; int b;
        if (nobits) return at_aerr("as: .byte in .bss");
        p = s + k; pl = n - k; a = 0;
        while (a < pl) {
            long v; b = a; while (b < pl && p[b] != 44) b = b + 1;
            { int x; int y; x = a; y = b; while (x < y && at_sp(p[x])) x = x + 1; while (y > x && at_sp(p[y - 1])) y = y - 1;
              if (!at_num(p + x, y - x, &v) || v < -128 || v > 255) return at_aerr("as: bad .byte value"); }
            if (*len >= AT_MAXT) return at_aerr("as: section too large");
            buf[*len] = v & 255; *len = *len + 1; a = b + 1;
        }
        return 0;
    }
    if (at_is(s, k, ".zero") || at_is(s, k, ".skip") || at_is(s, k, ".space")) {
        long v; int x; x = k; while (x < n && at_sp(s[x])) x = x + 1;
        if (!at_num(s + x, n - x, &v) || v < 0 || *len + v >= AT_MAXT) return at_aerr("as: bad .zero size");
        if (!nobits) { long q; q = 0; while (q < v) { buf[*len + q] = 0; q = q + 1; } }
        *len = *len + v; return 0;
    }
    if (at_is(s, k, ".ascii") || at_is(s, k, ".asciz") || at_is(s, k, ".string")) {
        int x; int z; z = !at_is(s, k, ".ascii");
        if (nobits) return at_aerr("as: .ascii in .bss");
        x = k; while (x < n && at_sp(s[x])) x = x + 1;
        if (x >= n || s[x] != 34) return at_aerr("as: .ascii needs a string");
        x = x + 1;
        while (x < n && s[x] != 34) {
            int c; c = s[x] & 255;
            if (c == 92 && x + 1 < n) {
                x = x + 1; c = s[x] & 255;
                if (c >= 48 && c <= 55) { int v; int d; v = 0; d = 0; while (d < 3 && x < n && s[x] >= 48 && s[x] <= 55) { v = v * 8 + (s[x] - 48); x = x + 1; d = d + 1; } c = v & 255; x = x - 1; }
                else { if (c == 110) c = 10; else { if (c == 116) c = 9; else { if (c == 114) c = 13; else { if (c == 48) c = 0; } } } }
            }
            if (*len >= AT_MAXT) return at_aerr("as: section too large");
            buf[*len] = c; *len = *len + 1; x = x + 1;
        }
        if (x >= n) return at_aerr("as: unterminated string");
        if (z) { buf[*len] = 0; *len = *len + 1; }
        return 0;
    }
    if (at_is(s, k, ".p2align") || at_is(s, k, ".balign") || at_is(s, k, ".align")) {
        long v; long a; int x; int e; x = k; while (x < n && at_sp(s[x])) x = x + 1;
        e = x; while (e < n && s[e] != 44) e = e + 1;
        { int y; y = e; while (y > x && at_sp(s[y - 1])) y = y - 1; if (!at_num(s + x, y - x, &v)) return at_aerr("as: bad alignment"); }
        a = at_is(s, k, ".p2align") ? ((long)1 << v) : v;
        if (a <= 0 || a > 4096 || (a & (a - 1))) return at_aerr("as: bad alignment");
        if (a > 16) return at_aerr("as: alignment above 16 is not in the subset");
        while (*len % a) { if (!nobits) buf[*len] = 0; *len = *len + 1; }
        return 0;
    }
    return at_aerr("as: directive not in the subset");
}
/* one pass over the source: sections, labels, directives; text lines become items */
int at_parse(char *src, long n) {
    long p; at_ni = 0; at_cursec = 1; at_dn = 0; at_bn = 0; at_tpn = 0; at_lineno = 0;
    p = 0;
    while (p < n) {
        long e; char *s; int l; int k;
        e = p; while (e < n && src[e] != 10) e = e + 1;
        s = src + p; l = (int)(e - p); p = e + 1; at_lineno = at_lineno + 1;
        /* comments: `#` at line start; `//` (arm64) or `#` after an operand (x86) */
        {   int q; int inq; int lead; q = 0; inq = 0; lead = 1;
            while (q < l) {
                if (s[q] == 34) inq = !inq;
                if (!inq) {
                    if (s[q] == 47 && q + 1 < l && s[q + 1] == 47) break;
                    /* x86: # starts a comment anywhere; arm64 only at line start (# is an immediate) */
                    if (s[q] == 35 && (at_arch == 0 || lead)) break;
                }
                if (!at_sp(s[q])) lead = 0;
                q = q + 1;
            }
            l = q;
        }
        while (l > 0 && at_sp(s[0])) { s = s + 1; l = l - 1; }
        while (l > 0 && at_sp(s[l - 1])) l = l - 1;
        /* labels (several may share a line) */
        while (l > 0) {
            k = 0; while (k < l && at_idc(s[k])) k = k + 1;
            if (k > 0 && k < l && s[k] == 58 && !(s[0] >= 48 && s[0] <= 57)) {
                if (at_cursec == 1) {
                    if (at_ni >= AT_MAXI) return at_aerr("as: too many lines");
                    at_iline[at_ni] = at_lineno; at_ik[at_ni] = 2; at_il[at_ni] = s; at_ilen[at_ni] = k; at_isym[at_ni] = at_sym(s, k); at_ni = at_ni + 1;
                } else {
                    if (at_cursec == 4) return at_aerr("as: labels in .unisa.tape");
                    if (at_deflabel(s, k, at_cursec == 2 ? at_dn : at_bn)) return 1;
                }
                s = s + k + 1; l = l - k - 1;
                while (l > 0 && at_sp(s[0])) { s = s + 1; l = l - 1; }
            } else break;
        }
        if (l == 0) continue;
        k = 0; while (k < l && !at_sp(s[k])) k = k + 1;
        if (s[0] == 46) {
            /* directives */
            if (at_is(s, k, ".text")) { at_cursec = 1; continue; }
            if (at_is(s, k, ".data")) { at_cursec = 2; continue; }
            if (at_is(s, k, ".bss")) { at_cursec = 3; continue; }
            if (at_is(s, k, ".section")) {
                int x; int y; x = k; while (x < l && at_sp(s[x])) x = x + 1;
                y = x; while (y < l && s[y] != 44 && !at_sp(s[y])) y = y + 1;
                if (at_is(s + x, y - x, ".unisa.tape")) { at_cursec = 4; continue; }
                if (at_is(s + x, y - x, ".text")) { at_cursec = 1; continue; }
                if (at_is(s + x, y - x, ".data")) { at_cursec = 2; continue; }
                if (at_is(s + x, y - x, ".bss")) { at_cursec = 3; continue; }
                return at_aerr("as: section not in the subset (.text .data .bss .unisa.tape)");
            }
            if (at_is(s, k, ".globl") || at_is(s, k, ".global") || at_is(s, k, ".local")) {
                int x; int j; x = k; while (x < l && at_sp(s[x])) x = x + 1;
                { int y; y = x; while (y < l && at_idc(s[y])) y = y + 1; if (y != l || y == x) return at_aerr("as: one name per .globl/.local"); }
                j = at_sym(s + x, l - x); if (j < 0) return 1;
                if (s[1] == 103) at_sglob[j] = 1;
                continue;
            }
            if (at_is(s, k, ".type") || at_is(s, k, ".size") || at_is(s, k, ".file") || at_is(s, k, ".ident")) continue;
            if (at_is(s, k, ".p2align") || at_is(s, k, ".balign") || at_is(s, k, ".align")) {
                if (at_cursec == 1) {
                    /* text alignment: only at offset 0 (the section is 16-aligned) */
                    int q; q = 0; while (q < at_ni) { if (at_ik[q] != 2) return at_aerr("as: alignment inside .text is not in the subset"); q = q + 1; }
                    continue;
                }
            }
            if (at_cursec == 1) {
                if (at_is(s, k, ".byte") || (at_arch == 1 && at_is(s, k, ".inst"))) {
                    if (at_ni >= AT_MAXI) return at_aerr("as: too many lines");
                    at_iline[at_ni] = at_lineno; at_ik[at_ni] = 1; at_il[at_ni] = s; at_ilen[at_ni] = l; at_ni = at_ni + 1;
                    continue;
                }
                return at_aerr("as: directive not in the subset in .text");
            }
            if (at_cursec == 2) { if (at_datadir(s, l, at_tdata, &at_dn, 0)) return 1; continue; }
            if (at_cursec == 3) { if (at_datadir(s, l, at_tdata, &at_bn, 1)) return 1; continue; }
            if (at_datadir(s, l, at_ttape, &at_tpn, 0)) return 1;
            continue;
        }
        if (at_cursec != 1) return at_aerr("as: instructions outside .text");
        if (at_ni >= AT_MAXI) return at_aerr("as: too many lines");
        at_iline[at_ni] = at_lineno; at_ik[at_ni] = 0; at_il[at_ni] = s; at_ilen[at_ni] = l; at_ilong[at_ni] = 0; at_ni = at_ni + 1;
    }
    return 0;
}
/* raw text bytes from a `.byte` / `.inst` item */
int at_rawitem(int i, char *out) {
    char *s; int l; int k; int n;
    s = at_il[i]; l = at_ilen[i]; k = 0; while (k < l && !at_sp(s[k])) k = k + 1;
    if (at_is(s, k, ".inst")) {
        long v; int x; x = k; while (x < l && at_sp(s[x])) x = x + 1;
        if (!at_num(s + x, l - x, &v)) return 0 - 1;
        out[0] = v & 255; out[1] = (v >> 8) & 255; out[2] = (v >> 16) & 255; out[3] = (v >> 24) & 255;
        return 4;
    }
    n = 0;
    {   int a; int b; a = k;
        while (a < l) {
            long v; b = a; while (b < l && s[b] != 44) b = b + 1;
            { int x; int y; x = a; y = b; while (x < y && at_sp(s[x])) x = x + 1; while (y > x && at_sp(s[y - 1])) y = y - 1;
              if (!at_num(s + x, y - x, &v) || v < -128 || v > 255 || n >= 32) return 0 - 1; }
            out[n] = v & 255; n = n + 1; a = b + 1;
        }
    }
    return n;
}
/* lay out .text until stable, then write it with its relocations */
int at_layout(void) {
    int changed; int round; int i; long addr; char buf[64];
    /* every text label is known to be text before the first round */
    i = 0;
    while (i < at_ni) {
        if (at_ik[i] == 2) {
            int j; j = at_isym[i]; if (j < 0) return 1;
            if (at_sdef[j]) { at_lineno = at_iline[i]; return at_aerr("as: symbol defined twice"); }
            at_sdef[j] = 1; at_ssect[j] = 1; at_sval[j] = 0; at_splaced[j] = 0;
        }
        i = i + 1;
    }
    round = 0; changed = 1;
    while (changed) {
        changed = 0; addr = 0; round = round + 1; at_lround = round;
        if (round > 64) return at_aerr("as: branch layout does not settle");
        i = 0;
        while (i < at_ni) {
            int len;
            if (at_ik[i] == 2) {
                int j; j = at_isym[i];
                if (at_sval[j] != addr || !at_splaced[j]) changed = 1;
                at_sval[j] = addr; at_splaced[j] = round;
                i = i + 1; continue;
            }
            at_fwd = round == 1 ? 0 : addr - at_iaddr[i];
            at_iaddr[i] = addr;
            if (at_ik[i] == 1) len = at_rawitem(i, buf);
            else {
                if (at_arch) len = at_aenc(at_il[i], at_ilen[i], addr, buf);
                else {
                    len = at_xenc(at_il[i], at_ilen[i], addr, at_ilong[i], buf);
                    if (len >= 0 && at_j32) at_ilong[i] = 1;     /* a branch that grew stays long */
                }
            }
            if (len < 0) { at_lineno = at_iline[i]; return at_aerr("as: instruction not in the subset"); }
            if (at_isz[i] != len) { at_isz[i] = len; changed = 1; }
            addr = addr + len; i = i + 1;
        }
        if (addr >= AT_MAXT) return at_aerr("as: text too large");
    }
    /* final: bytes and relocations (every label placed, nothing moves) */
    at_lround = round; at_fwd = 0;
    at_tn = 0; at_nar = 0; i = 0;
    while (i < at_ni) {
        int len; int k;
        if (at_ik[i] == 2) { i = i + 1; continue; }
        if (at_ik[i] == 1) { len = at_rawitem(i, at_ttext + at_tn); at_nr = 0; }
        else {
            if (at_arch) len = at_aenc(at_il[i], at_ilen[i], at_tn, at_ttext + at_tn);
            else len = at_xenc(at_il[i], at_ilen[i], at_tn, at_ilong[i], at_ttext + at_tn);
        }
        if (len != at_isz[i]) { at_lineno = at_iline[i]; return at_aerr("as: layout changed in the final pass"); }
        k = 0;
        while (k < at_nr) {
            if (at_nar >= 262144) return at_aerr("as: too many relocations");
            at_aoff[at_nar] = at_tn + at_roff[k]; at_atype[at_nar] = at_rtype[k]; at_asect[at_nar] = at_rs[k];
            at_asym[at_nar] = at_rsym[k]; at_aadd[at_nar] = at_radd[k];
            if (at_rs[k] == 0) at_sref[at_rsym[k]] = 2;
            at_nar = at_nar + 1; k = k + 1;
        }
        at_tn = at_tn + len; i = i + 1;
    }
    return 0;
}
/* ELF output, exactly the layout of bk_elfobj */
char at_wbuf[65536]; int at_wn;
int at_wb(int b) { at_wbuf[at_wn] = b; at_wn = at_wn + 1; if (at_wn == 65536) { at_wr(at_wbuf, at_wn); at_wn = 0; } return 0; }
int at_w16(long v) { at_wb(v & 255); at_wb((v >> 8) & 255); return 0; }
int at_w32(long v) { at_w16(v & 65535); at_w16((v >> 16) & 65535); return 0; }
int at_w64(long v) { at_w32(v & 4294967295L); at_w32((v >> 32) & 4294967295L); return 0; }
int at_wz(long n) { while (n > 0) { at_wb(0); n = n - 1; } return 0; }
int at_wstr(char *s, int n) { int k; k = 0; while (k < n) { at_wb(s[k]); k = k + 1; } return 0; }
long at_round(long v, long a) { return (v + a - 1) / a * a; }
int at_kind[AT_MAXS]; int at_pos[AT_MAXS];
int at_elf(void) {
    long L; long bss; long tend; long doff; long roff; long soff; long stoff; long shsoff; long shoff; long nsym; long strl; long tlen; long toff8;
    int j; int kind; int n1; int n2; int n3; long name; int k;
    L = at_dn; bss = at_bn;
    j = 0; n1 = 0; n2 = 0; n3 = 0;
    while (j < at_nsym) {
        at_kind[j] = 0;
        if (!at_local(at_pool + at_sname[j], at_snl[j])) {
            if (at_sdef[j]) at_kind[j] = at_sglob[j] ? 2 : 1;
            else { if (at_sref[j] == 2 || at_sglob[j]) at_kind[j] = 3; }
        } else { if (!at_sdef[j] && at_sref[j]) return at_aerr("as: a .L label is used but not defined"); }
        j = j + 1;
    }
    kind = 1;
    while (kind <= 3) {
        j = 0;
        while (j < at_nsym) {
            if (at_kind[j] == kind) {
                if (kind == 1) { at_pos[j] = n1; n1 = n1 + 1; } else { if (kind == 2) { at_pos[j] = n2; n2 = n2 + 1; } else { at_pos[j] = n3; n3 = n3 + 1; } }
            }
            j = j + 1;
        }
        kind = kind + 1;
    }
    j = 0; while (j < at_nsym) { if (at_kind[j] == 2) at_pos[j] = at_pos[j] + n1; if (at_kind[j] == 3) at_pos[j] = at_pos[j] + n1 + n2; j = j + 1; }
    nsym = 4 + n1 + n2 + n3; strl = 1;
    j = 0; while (j < at_nsym) { if (at_kind[j]) strl = strl + at_snl[j] + 1; j = j + 1; }
    tend = 64 + at_tn;
    doff = at_round(tend, 16);
    roff = at_round(doff + L, 8);
    soff = at_round(roff + 24 * at_nar, 8);
    stoff = soff + 24 * nsym;
    shsoff = stoff + strl;
    tlen = at_tpn; toff8 = shsoff + (tlen ? 67 : 55);
    shoff = at_round(toff8 + tlen, 8);
    at_wn = 0;
    at_wb(127); at_wb(69); at_wb(76); at_wb(70); at_wb(2); at_wb(1); at_wb(1); at_wb(0); at_wz(8);
    at_w16(1); at_w16(at_arch ? 183 : 62); at_w32(1);
    at_w64(0); at_w64(0); at_w64(shoff);
    at_w32(0); at_w16(64); at_w16(0); at_w16(0); at_w16(64); at_w16(tlen ? 9 : 8); at_w16(7);
    at_wstr(at_ttext, (int)at_tn); at_wz(doff - tend);
    { long q; q = 0; while (q < L) { at_wb(at_tdata[q]); q = q + 1; } }
    at_wz(roff - (doff + L));
    k = 0;
    while (k < at_nar) {
        long sym;
        if (at_asect[k] >= 1 && at_asect[k] <= 3) sym = at_asect[k]; else sym = 4 + at_pos[at_asym[k]];
        at_w64(at_aoff[k]); at_w32(at_atype[k]); at_w32(sym); at_w64(at_aadd[k]);
        k = k + 1;
    }
    at_wz(soff - (roff + 24 * at_nar));
    at_w32(0); at_wb(0); at_wb(0); at_w16(0); at_w64(0); at_w64(0);
    k = 1; while (k <= 3) { at_w32(0); at_wb(3); at_wb(0); at_w16(k); at_w64(0); at_w64(0); k = k + 1; }
    name = 1; kind = 1;
    while (kind <= 3) {
        j = 0;
        while (j < at_nsym) {
            if (at_kind[j] == kind) {
                int info; int shn; long val;
                info = kind == 1 ? 0 : 16; shn = 0; val = 0;
                if (kind != 3) {
                    shn = at_ssect[j]; val = at_sval[j];
                    if (kind == 2) info = info | (shn == 1 ? 2 : 1);
                }
                at_w32(name); at_wb(info); at_wb(0); at_w16(shn); at_w64(val); at_w64(0);
                name = name + at_snl[j] + 1;
            }
            j = j + 1;
        }
        kind = kind + 1;
    }
    at_wb(0); kind = 1;
    while (kind <= 3) { j = 0; while (j < at_nsym) { if (at_kind[j] == kind) { at_wstr(at_pool + at_sname[j], at_snl[j]); at_wb(0); } j = j + 1; } kind = kind + 1; }
    at_wstr("", 1); at_wstr(".text", 6); at_wstr(".data", 6); at_wstr(".bss", 5); at_wstr(".rela.text", 11);
    at_wstr(".symtab", 8); at_wstr(".strtab", 8); at_wstr(".shstrtab", 10);
    if (tlen) { at_wstr(".unisa.tape", 12); { long q; q = 0; while (q < tlen) { at_wb(at_ttape[q]); q = q + 1; } } }
    at_wz(shoff - (toff8 + tlen));
#define AT_SH(nm, ty, fl, of, sz, ln, inf, al, es) { at_w32(nm); at_w32(ty); at_w64(fl); at_w64(0); at_w64(of); at_w64(sz); at_w32(ln); at_w32(inf); at_w64(al); at_w64(es); }
    AT_SH(0, 0, 0, 0, 0, 0, 0, 0, 0);
    AT_SH(1, 1, 6, 64, at_tn, 0, 0, 16, 0);
    AT_SH(7, 1, 3, doff, L, 0, 0, 16, 0);
    AT_SH(13, 8, 3, doff + L, bss, 0, 0, 16, 0);
    AT_SH(18, 4, 64, roff, 24 * at_nar, 5, 1, 8, 24);
    AT_SH(29, 2, 0, soff, 24 * nsym, 6, 4 + n1, 8, 24);
    AT_SH(37, 3, 0, stoff, strl, 0, 0, 1, 0);
    AT_SH(45, 3, 0, shsoff, tlen ? 67 : 55, 0, 0, 1, 0);
    if (tlen) AT_SH(55, 1, 0, toff8, tlen, 0, 0, 1, 0);
    if (at_wn) at_wr(at_wbuf, at_wn);
    at_wn = 0;
    return 0;
}
/* assemble src[0..n) for arch (0 x86-64, 1 arm64); the object goes through at_wr */
int at_asm(char *src, long n, int arch) {
    int k;
    at_err = 0; at_arch = arch; at_nsym = 0; at_pooln = 0;
    k = 0; while (k < AT_HASH) { at_hash[k] = 0; k = k + 1; }
    /* the -S header names the target */
    if (n > 15 && at_is(src, 13, "# unisacc -S ")) {
        if (n > 23 && at_is(src + 13, 10, "lnx/x86_64")) at_arch = 0;
        if (n > 22 && at_is(src + 13, 9, "lnx/arm64")) at_arch = 1;
    }
    if (at_parse(src, n)) return 1;
    k = 0; while (k < at_ni) { at_isz[k] = 0 - 1; k = k + 1; }
    if (at_layout()) return 1;
    return at_elf();
}

#ifndef AT_PRODUCT
/* `unisacc as [-b lnx/ARCH] FILE.s [-o FILE.o]` */
int at_say(char *m) { __write(2, m, blen(m)); return 0; }
/* `unisacc nm FILE.o`, `unisacc objdump -d FILE.o` (Linux objects; R18-4) */
int at_view(int argc) {
    char *a; char *f; int fd; long n; int dis;
    a = __argv(1); dis = a[0] == 111;
    f = __argv(dis ? 3 : 2);
    if (dis && (argc != 4 || !strsame(__argv(2), "-d"))) { at_say("usage: unisacc objdump -d FILE.o\n"); return 2; }
    if (!dis && argc != 3) { at_say("usage: unisacc nm FILE.o\n"); return 2; }
    fd = ropen(f); if (fd < 0) { at_say("cannot read "); at_say(f); at_say("\n"); return 1; }
    n = __read(fd, at_src, 33554432); __close(fd);
    if (n < 0 || n >= 33554432) { at_say("input too large\n"); return 1; }
    at_fd = 1; at_wfail = 0;
    if (dis ? at_dis(at_src, n) : at_nm(at_src, n)) { at_say(dis ? "objdump: " : "nm: "); at_say(f); at_say(": "); at_say(at_err); at_say(" (Linux objects; Mach-O and COFF: 0.0.19)\n"); return 1; }
    return 0;
}
int at_main(int argc) {
    int i; char *in; char *out; int arch; int fd; long n; char *a;
    in = 0; out = "a.out"; arch = 0 - 1;
    i = 2;
    while (i < argc) {
        a = __argv(i);
        if (strsame(a, "-o")) { i = i + 1; out = __argv(i); }
        else { if (strsame(a, "-b")) { i = i + 1; a = __argv(i);
            if (strsame(a, "lnx/x86_64")) arch = 0; else { if (strsame(a, "lnx/arm64")) arch = 1;
            else { at_say("as: targets are lnx/x86_64 and lnx/arm64 (Mach-O and COFF text: 0.0.19)\n"); return 2; } } }
        else { if (a[0] == 45 && a[1]) { __write(2, "as: unknown option ", 19); __write(2, a, blen(a)); __write(2, "\n", 1); return 2; }
        else in = a; } }
        i = i + 1;
    }
    if (in == 0 || out == 0) { at_say("usage: unisacc as [-b lnx/x86_64|lnx/arm64] FILE.s [-o FILE.o]\n"); return 2; }
    fd = ropen(in); if (fd < 0) { __write(2, "as: cannot read ", 16); __write(2, in, blen(in)); __write(2, "\n", 1); return 1; }
    n = __read(fd, at_src, 33554432); __close(fd);
    if (n < 0 || n >= 33554432) { __write(2, "as: input too large\n", 20); return 1; }
    if (arch < 0) {
        arch = HOST_TARGET[4] == 97 ? 1 : 0;   /* this machine's architecture */
        if (n > 22 && at_is(at_src, 22, "# unisacc -S lnx/arm64")) arch = 1;
        if (n > 23 && at_is(at_src, 23, "# unisacc -S lnx/x86_64")) arch = 0;
    }
    /* a failed assembly must not leave a file behind: check first, write after */
    at_fd = 0 - 1;
    at_wfail = 1;
    if (at_asm(at_src, n, arch)) {
        at_say("as: "); at_say(in); at_say(":"); at_ln = 0; at_ldec(at_lineno); __write(2, at_lb, at_ln);
        at_say(": "); at_say(at_err ? at_err : "error"); at_say("\n");
        return 1;
    }
    at_wfail = 0;
    fd = wopen(out); if (fd < 0) { __write(2, "as: cannot write ", 17); __write(2, out, blen(out)); __write(2, "\n", 1); return 1; }
    at_fd = fd;
    at_asm(at_src, n, arch);
    __close(fd);
    return 0;
}
#endif
