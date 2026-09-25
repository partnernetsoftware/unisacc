/* ---- encoding, shared ------------------------------------------------- */
char *bkout;                        /* where the current instruction's bytes go */
int bkol;                           /* how many so far */
long bk_textva; long bk_shift; int bk_sizing;
#define BK_NIMP 14                  /* pe.IMPORTS */
char *BK_IMPS = "GetStdHandle\000WriteFile\000ReadFile\000CloseHandle\000CreateFileA\000ExitProcess\000GetCommandLineA\000VirtualAlloc\000VirtualProtect\000VirtualFree\000FlushInstructionCache\000SetFilePointer\000DeleteFileA\000MoveFileExA\000";
long bk_imp[BK_NIMP];               /* Windows: the IAT slot of each import */
long toff[BK_MAXT + 1];             /* each lowered instruction's byte offset */
/* Branch relaxation, the same rounds as assemble.py [S-10 #1]: tshort[i]
   says instruction i is a branch encoded in its short form; tjk[i] is that
   form's length (0: not a relaxable branch) and tjt[i] its target label. */
char tshort[BK_MAXT]; char tfit[BK_MAXT]; int tjk[BK_MAXT]; int tjt[BK_MAXT];
long tsz[BK_MAXT];

int ob(int b) { bkout[bkol] = b; bkol = bkol + 1; return 0; }
int ow(unsigned long v) {           /* a 32-bit little-endian word */
    ob(v & 255); ob((v >> 8) & 255); ob((v >> 16) & 255); ob((v >> 24) & 255); return 0;
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
/* the reloc stage's answer IS the displacement field (emit_arm.RELFIELD) */
long a_relf(int i, long d) {
    char *r;
    if (tkg_rel[i] < 0) { __write(2, "back end: branch without a reloc answer\n", 40); __exit(1); }
    r = bk_nth(BH_RELOC_Y, tkg_rel[i]);
    if (strsame(r, "arm26")) return a_disp(d, 26) & 0x3FFFFFF;
    if (strsame(r, "arm19")) return (a_disp(d, 19) & 0x7FFFF) << 5;
    __write(2, "back end: reloc class has no arm field\n", 39); __exit(1);
    return 0;
}
int a_ldr(int rt, int rn, long off) { ow(0xF9400000 | ((off / 8) << 10) | (rn << 5) | rt); return 0; }
int a_str(int rt, int rn, long off) { ow(0xF9000000 | ((off / 8) << 10) | (rn << 5) | rt); return 0; }
int a_movz(int d, long v) { ow(0xD2800000 | ((v & 0xFFFF) << 5) | d); return 0; }
int a_movn(int d, long v) { ow(0x92800000 | (((0 - v - 1) & 0xFFFF) << 5) | d); return 0; }
/* the Windows command-line splitter, the same words emit_arm/emit_x86 hold */
long BK_WA_ARM[34] = {0x39400004, 0x7100809F, 0x54000060, 0x7100249F, 0x54000061, 0x91000400, 0x17FFFFFA, 0x34000364, 0xF100FC5F, 0x5400032A, 0xF8227861, 0x91000442, 0xD2800005, 0x39400004, 0x34000264, 0x7100889F, 0x54000081, 0xD24000A5, 0x91000400, 0x17FFFFFA, 0xB50000A5, 0x7100809F, 0x540000E0, 0x7100249F, 0x540000A0, 0x39000024, 0x91000421, 0x91000400, 0x17FFFFF1, 0x3900003F, 0x91000421, 0x91000400, 0x17FFFFE0, 0x3900003F};
char BK_WA_X86[93] = {15, 182, 6, 60, 32, 116, 4, 60, 9, 117, 5, 72, 255, 198, 235, 240, 132, 192, 116, 73, 72, 131, 249, 63, 125, 67, 73, 137, 60, 200, 72, 255, 193, 69, 49, 201, 15, 182, 6, 132, 192, 116, 47, 60, 34, 117, 9, 65, 131, 241, 1, 72, 255, 198, 235, 236, 69, 133, 201, 117, 8, 60, 32, 116, 14, 60, 9, 116, 10, 136, 7, 72, 255, 199, 72, 255, 198, 235, 213, 198, 7, 0, 72, 255, 199, 72, 255, 198, 235, 166, 198, 7, 0};
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
/* body, then the abi table's `retconv` tail; the import is `winimp` [I4] */
int a_wintail(int rc, long pc, long written) {
    char *n; n = bk_nth(BH_ABI_RETCONV, rc);
    if (strsame(n, "wcount")) { a_adrp_add(A_IP0, pc, written); a_ldr(0, A_IP0, 0); return 0; }
    if (strsame(n, "bool_inv")) { ow(0xF100001F); ow(0x9A9F17E0); return 0; }   /* cmp; cset eq */
    if (strsame(n, "bool_neg")) { ow(0x7100001F); ow(0xDA9F13E0); return 0; }   /* cmp w0; csetm eq */
    if (strsame(n, "dword_sx")) { ow(0x93407C00); return 0; }                   /* sxtw x0, w0 */
    return 0;
}
int a_winbody(int i, long off);
int a_winapi(int i, long off) {
    int s; long pc;
    s = bkol; pc = bk_textva + off;
    if (a_winbody(i, off) == 0) return 0;
    a_wintail(tkg_rc[i], pc + (bkol - s), bk_written + bk_shift);
    return 1;
}
int a_winbody(int i, long off) {
    long pc; long hstd; long written; char *nm; int s;
    pc = bk_textva + off;
    hstd = bk_hstd + bk_shift; written = bk_written + bk_shift;
    nm = bk_nth(BF_ABI_0, tkg_cop[i]);
    s = bkol;
    if (strsame(nm, "exit")) { a_callimp(pc, bk_impof(tkg_wi[i])); return 1; }
    if (strsame(nm, "write") || strsame(nm, "read")) {
        a_fd2handle(pc, hstd);
        a_adrp_add(3, pc + (bkol - s), written);
        ow(0xAA1F03E4);                              /* mov x4, xzr */
        a_callimp(pc + (bkol - s), bk_impof(tkg_wi[i]));
        return 1;
    }
    if (strsame(nm, "mmap")) {          /* VirtualAlloc, four args in x0..x3 */
        a_callimp(pc, bk_impof(tkg_wi[i]));
        return 1;
    }
    if (strsame(nm, "mprotect")) {      /* VirtualProtect(addr,n,prot,&old) */
        long scr0; long scr1;
        scr0 = bk_scr0 + bk_shift; scr1 = bk_scr1 + bk_shift;
        a_adrp_add(3, pc, written);
        a_callimp(pc + (bkol - s), bk_impof(tkg_wi[i]));
        /* arm64 Windows will not execute code that is only in the data
           cache, and a protection change does not flush it.  The current
           process is the pseudo-handle -1. [S-9] */
        a_movn(0, 0 - 1);
        a_adrp_add(A_IP0, pc + (bkol - s), scr0); a_ldr(1, A_IP0, 0);
        a_adrp_add(A_IP0, pc + (bkol - s), scr1); a_ldr(2, A_IP0, 0);
        a_callimp(pc + (bkol - s), 10);          /* FlushInstructionCache */
        return 1;
    }
    if (strsame(nm, "munmap")) {        /* VirtualFree(addr, 0, MEM_RELEASE) */
        ow(0xD2900002);
        ow(0xAA1F03E1);
        a_callimp(pc + (bkol - s), bk_impof(tkg_wi[i]));
        return 1;
    }
    if (strsame(nm, "close")) {
        a_fd2handle(pc, hstd);
        a_callimp(pc + (bkol - s), bk_impof(tkg_wi[i]));
        return 1;
    }
    if (strsame(nm, "open")) {
        ow(0xAA0203E4);                              /* x4 = x2 */
        a_movz(2, 3);
        ow(0xAA1F03E3);
        a_movz(5, 0x80);
        ow(0xAA1F03E6);
        a_callimp(pc + (bkol - s), bk_impof(tkg_wi[i]));
        return 1;
    }
    if (strsame(nm, "lseek")) {
        ow(0xAA0203E3);                              /* x3 = x2 (method) */
        ow(0xAA1F03E2);                              /* x2 = 0 */
        a_fd2handle(pc + (bkol - s), hstd);
        a_callimp(pc + (bkol - s), bk_impof(tkg_wi[i]));
        return 1;
    }
    if (strsame(nm, "unlink") || strsame(nm, "rename")) {
        if (strsame(nm, "rename")) a_movz(2, 1);
        a_callimp(pc + (bkol - s), bk_impof(tkg_wi[i]));
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
    if (strsame(o, "cvtid")) { ow(0x9E620000 | (a[1] << 5) | 16); a_fmov_from(a[0], 16, 1); return 1; }
    if (strsame(o, "cvtud")) { ow(0x9E630000 | (a[1] << 5) | 16); a_fmov_from(a[0], 16, 1); return 1; }
    if (strsame(o, "cvtis")) { ow(0x9E220000 | (a[1] << 5) | 16); a_fmov_from(a[0], 16, 0); return 1; }
    if (strsame(o, "cvtus")) { ow(0x9E230000 | (a[1] << 5) | 16); a_fmov_from(a[0], 16, 0); return 1; }
    if (strsame(o, "cvtdi")) { a_fmov_to(16, a[1], 1); ow(0x9E780000 | (16 << 5) | a[0]); return 1; }
    if (strsame(o, "cvtdu")) { a_fmov_to(16, a[1], 1); ow(0x9E790000 | (16 << 5) | a[0]); return 1; }
    if (strsame(o, "cvtsd")) { a_fmov_to(16, a[1], 0); ow(0x1E22C000 | (16 << 5) | 16); a_fmov_from(a[0], 16, 1); return 1; }
    if (strsame(o, "cvtds")) { a_fmov_to(16, a[1], 1); ow(0x1E624000 | (16 << 5) | 16); a_fmov_from(a[0], 16, 0); return 1; }
    if (strsame(o, "fsqrt64")) { a_fmov_to(16, a[1], 1); ow(0x1E61C000 | (16 << 5) | 16); a_fmov_from(a[0], 16, 1); return 1; }
    if (strsame(o, "fsqrt32")) { a_fmov_to(16, a[1], 0); ow(0x1E21C000 | (16 << 5) | 16); a_fmov_from(a[0], 16, 0); return 1; }
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
    if (op == TO_SEXT) { ow(0x93400000 | ((8 * a[2] - 1) << 10) | (a[1] << 5) | a[0]); return 1; }   /* [J9] */
    if (op == TO_ADDI) { ow(0x91000000 | (a[2] << 10) | (a[1] << 5) | a[0]); return 1; }   /* [J9] */
    if (op == TO_SUBI) { ow(0xD1000000 | (a[2] << 10) | (a[1] << 5) | a[0]); return 1; }
    if (op == TO_LSLI) { ow(0xD3400000 | (((64 - a[2]) & 63) << 16) | ((63 - a[2]) << 10) | (a[1] << 5) | a[0]); return 1; }
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
    if (op == TO_WINARGS) {
        int s; s = bkol;
        a_callimp(pc, 6);                                        /* GetCommandLineA */
        ow(0xAA0003E1); ow(0xD2800002);
        a_adrp_add(3, pc + (bkol - s), a[2] + bk_shift);
        k = 0; while (k < 34) { ow(BK_WA_ARM[k]); k = k + 1; }
        a_adrp_add(A_IP0, pc + (bkol - s), a[0] + bk_shift); a_str(2, A_IP0, 0);
        a_adrp_add(A_IP0, pc + (bkol - s), a[1] + bk_shift); a_str(3, A_IP0, 0);
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
        if (strsame(g, "winapi")) return a_winapi(i, off);
        if (strsame(g, "svc80")) ow(0xD4001001); else ow(0xD4000001);
        /* Darwin puts a failed syscall in the CARRY flag and returns errno
           POSITIVE: `b.cc +8` over `neg x0, x0`, so the caller sees -errno
           the way it does everywhere else [I-20] */
        if (bkos == 1) { ow(0x54000043); ow(0xCB0003E0); }
        return 1;
    }
    o = bk_nth(BKOPS, op);
    if (strsame(o, "mov")) { ow(0xAA0003E0 | (a[1] << 16) | a[0]); return 1; }
    if (strsame(o, "imm")) {
        unsigned long u; u = a[1];
        ow(0xD2800000 | ((u & 0xFFFF) << 5) | a[0]);
        k = 1;
        while (k <= 3) { v = (u >> (16 * k)) & 0xFFFF;
            if (v) ow(0xF2800000 | (k << 21) | (v << 5) | a[0]); k = k + 1; }
        return 1;
    }
    {   unsigned long alu; int j;              /* [I5] catalog.ENCSPEC */
        j = enc_ix(ENC_ARM_ALU3, NENC_ARM_ALU3, o);
        if (j >= 0) { alu = ENC_ARM_ALU3_V(j); ow(alu | (a[2] << 16) | (a[1] << 5) | a[0]); return 1; }
    }
    if (strsame(o, "mul64")) { ow(0x9B007C00 | (a[2] << 16) | (a[1] << 5) | a[0]); return 1; }
    if (strsame(o, "load64")) { a_mem(0, a[0], a[1], a[2], 8); return 1; }
    if (strsame(o, "store64")) { a_mem(1, a[2], a[0], a[1], 8); return 1; }
    {   int cc; int j;                         /* [I5] catalog.ENCSPEC */
        cc = 0 - 1; j = enc_ix(ENC_ARM_INVCOND, NENC_ARM_INVCOND, o);
        if (j >= 0) cc = ENC_ARM_INVCOND_V(j);
        if (cc >= 0) {
            ow(0xEB00001F | (a[2] << 16) | (a[1] << 5));
            ow(0x9A9F07E0 | (cc << 12) | a[0]);
            return 1;
        }
    }
    if (strsame(o, ".frame")) {
        n = a[0];
        if (n > 0 - 4096 && n < 4096) {
            ow((n >= 0 ? 0xD1000000 : 0x91000000) | ((n >= 0 ? n : 0 - n) << 10) | (7 << 5) | 7);
            return 1;
        }
        a_movimm(A_IP0, n >= 0 ? n : 0 - n, 0);
        ow((n >= 0 ? 0xCB000000 : 0x8B000000) | (A_IP0 << 16) | (7 << 5) | 7);
        return 1;
    }
    if (strsame(o, ".lea")) { a_adrp_add(a[0], pc, bk_leaaddr(i)); return 1; }
    if (strsame(o, ".ld")) { a_mem(0, a[0], a[1], a[2], a[3]); return 1; }
    if (strsame(o, ".st")) { a_mem(1, a[2], a[0], a[1], a[3]); return 1; }
    if (a_fp(o, a)) return 1;
    if (strsame(o, ".zero")) {
        long kk; int wd;
        kk = 0;
        while (kk < a[2]) {
            wd = 8; while (kk + wd > a[2]) wd = wd / 2;
            a_mem(1, 31, a[0], a[1] + kk, wd);
            kk = kk + wd;
        }
        return 1;
    }
    if (strsame(o, "callr")) {
        a_adr(A_IP1, pc, pc + 16);
        ow(0xD1002000 | (7 << 5) | 7);
        ow(0xF9000000 | (7 << 5) | A_IP1);
        ow(0xD63F0000 | (a[0] << 5));      /* blr: pairs with ret for the return predictor */
        return 1;
    }
    if (strsame(o, ".div") || strsame(o, ".udiv")) {
        ow((strsame(o, ".div") ? 0x9AC00C00 : 0x9AC00800) | (a[2] << 16) | (a[1] << 5) | a[0]);
        return 1;
    }
    if (strsame(o, ".mod") || strsame(o, ".umod")) {
        ow((strsame(o, ".mod") ? 0x9AC00C00 : 0x9AC00800) | (a[2] << 16) | (a[1] << 5) | A_IP1);
        ow(0x9B008000 | (a[2] << 16) | (a[1] << 10) | (A_IP1 << 5) | a[0]);
        return 1;
    }
    if (strsame(o, "ret")) {
        ow(0xF9400000 | (7 << 5) | A_IP1);
        ow(0x91002000 | (7 << 5) | 7);
        ow(0xD65F0000 | (A_IP1 << 5));     /* ret x17: same jump as br, predicted */
        return 1;
    }
    if (strsame(o, "nop")) { ow(0xD503201F); return 1; }
    if (strsame(o, "jump")) {
        ow(0x14000000 | a_relf(i, bk_label(a[0]) - off));
        return 1;
    }
    if (strsame(o, "call")) {
        a_adr(A_IP1, pc, pc + 16);
        ow(0xD1002000 | (7 << 5) | 7);
        ow(0xF9000000 | (7 << 5) | A_IP1);
        ow(0x94000000 | a_relf(i, bk_label(a[0]) - (off + 12)));   /* bl */
        return 1;
    }
    if (strsame(o, "jumpz")) {
        ow(0xB4000000 | a_relf(i, bk_label(a[1]) - off) | a[0]);
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
    /* SHORT form -- see emit_x86.mov_ri: mov r32, imm32 zero-extends. */
    if ((imm >> 32) == 0) {
        if (d >= 8) ob(0x41);
        ob(0xB8 + (d & 7)); ow(imm & 0xFFFFFFFF);
        return 0;
    }
    x_rex(1, 0, 0, d >> 3); ob(0xB8 + (d & 7));
    ow(imm & 0xFFFFFFFF); ow(imm >> 32);
    return 0;
}
int x_d32(long v) { ow(v & 0xFFFFFFFF); return 0; }
int x_rel8(long d) {                  /* a short branch's displacement */
    if (bk_sizing == 0) { if (d < 0 - 128 || d > 127) {
        __write(2, "back end: rel8 out of range\n", 28); __exit(1); } }
    ob(d & 255);
    return 0;
}
int x_rel(int i, long d) {          /* emit_x86.RELBYTES */
    char *r;
    if (tkg_rel[i] < 0) { __write(2, "back end: branch without a reloc answer\n", 40); __exit(1); }
    r = bk_nth(BH_RELOC_Y, tkg_rel[i]);
    if (strsame(r, "rel32")) { x_d32(d); return 0; }
    __write(2, "back end: reloc class has no x86 field\n", 39); __exit(1);
    return 0;
}
/* mem: opc may be two bytes (0x0F 0xBE); w is REX.W */
int x_mem(int opc, int opc2, int r, int b, long disp, int w) {
    x_rex(w, r >> 3, 0, b >> 3);
    ob(opc); if (opc2 >= 0) ob(opc2);
    /* SHORT forms -- see emit_x86.mem.  disp is a frame offset or a small
       literal, never an address, so the width is final in the sizing pass. */
    /* rm == 4 means a SIB follows; 0x24 is the SIB naming rsp itself, which
       the tape stack pointer now is (emit_x86.mem does the same) */
    if (disp == 0 && (b & 7) != 5) { x_modrm(0, r, b); if ((b & 7) == 4) ob(0x24); return 0; }
    if (disp >= 0 - 128 && disp <= 127) { x_modrm(1, r, b); if ((b & 7) == 4) ob(0x24); ob(disp & 255); return 0; }
    x_modrm(2, r, b); if ((b & 7) == 4) ob(0x24); x_d32(disp);
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
/* `op r64, imm` with /opc -- see emit_x86.alu_imm.  SHORT form: 0x83 takes a
   sign-extended imm8, four bytes rather than seven.  n is a frame size or a
   literal, never an address, so the width is final in the sizing pass. */
int x_aluimm(int d, int opc, long n) {
    x_rex(1, 0, 0, d >> 3);
    if (n >= 0 - 128 && n <= 127) { ob(0x83); x_modrm(3, opc, d); ob(n & 255); return 0; }
    ob(0x81); x_modrm(3, opc, d); x_d32(n);
    return 0;
}
int x_spadj(long n, int opc) {                  /* on the tape SP: rsp */
    return x_aluimm(X_RSP, opc, n);
}
int x_push(int r) { if (r >= 8) x_rex(0, 0, 0, 1); ob(0x50 | (r & 7)); return 0; }
int x_pop(int r)  { if (r >= 8) x_rex(0, 0, 0, 1); ob(0x58 | (r & 7)); return 0; }
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
/* which IAT slot the abi table's `winimp` names [I4] */
int bk_impof(int c) {
    char *n; int L;
    n = bk_nth(BH_ABI_WINIMP, c); L = 0; while (n[L]) L = L + 1;
    return vfind(BK_IMPS, BK_NIMP, n, L);
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
/* a WinAPI gate is the op's own moves and call (x_winbody), then the
   conversion of its answer to POSIX's (x_wintail): the import is the abi
   table's `winimp`, the tail its `retconv` [I4] */
int x_wintail(int rc, long pc, long written) {
    char *n; n = bk_nth(BH_ABI_RETCONV, rc);
    if (strsame(n, "wcount")) { x_rip(0x8B, X_RAX, pc + 7, written); return 0; }
    if (strsame(n, "bool_inv")) {
        x_rex(1, 0, 0, 0); ob(0x83); x_modrm(3, 7, 0); ob(0);   /* cmp rax, 0 */
        ob(0x0F); ob(0x94); ob(0xC0);                          /* sete al */
        x_rex(1, 0, 0, 0); ob(0x0F); ob(0xB6); ob(0xC0);       /* movzx rax, al */
        return 0;
    }
    if (strsame(n, "bool_neg")) {
        ob(0x85); ob(0xC0); ob(0x0F); ob(0x94); ob(0xC0);   /* test eax; sete al */
        ob(0x48); ob(0x0F); ob(0xB6); ob(0xC0);              /* movzx rax, al */
        ob(0x48); ob(0xF7); ob(0xD8);                        /* neg rax: 0 / -1 */
        return 0;
    }
    if (strsame(n, "dword_sx")) { ob(0x48); ob(0x63); ob(0xC0); return 0; }   /* movsxd */
    return 0;
}
int x_winbody(int i, long off);
int x_winapi(int i, long off) {
    int s; long pc;
    s = bkol; pc = bk_textva + off;
    if (x_winbody(i, off) == 0) return 0;
    x_wintail(tkg_rc[i], pc + (bkol - s), bk_written + bk_shift);
    return 1;
}
int x_winbody(int i, long off) {
    long pc; long hstd; long written; char *nm; int s;
    pc = bk_textva + off;
    hstd = bk_hstd + bk_shift; written = bk_written + bk_shift;
    nm = bk_nth(BF_ABI_0, tkg_cop[i]);
    s = bkol;
    if (strsame(nm, "exit")) {
        x_alignpre(0); x_callimp(pc + (bkol - s), bk_impof(tkg_wi[i])); x_alignpost();
        return 1;
    }
    if (strsame(nm, "write") || strsame(nm, "read")) {
        x_fd2handle(pc, hstd);
        x_rip(0x8D, X_R9, pc + (bkol - s) + 7, written);
        x_alignpre(1);
        x_stackarg(32, 0);
        x_callimp(pc + (bkol - s), bk_impof(tkg_wi[i]));
        x_alignpost();
        return 1;
    }
    if (strsame(nm, "mmap")) {
        x_alignpre(0); x_callimp(pc + (bkol - s), bk_impof(tkg_wi[i])); x_alignpost();
        return 1;
    }
    if (strsame(nm, "mprotect")) {
        long scr0; long scr1;
        scr0 = bk_scr0 + bk_shift; scr1 = bk_scr1 + bk_shift;
        x_rip(0x8D, X_R9, pc + (bkol - s) + 7, written);   /* r9 = &old */
        x_alignpre(0);
        x_callimp(pc + (bkol - s), bk_impof(tkg_wi[i]));
        x_alignpost();
        /* flush the instruction cache -- see the arm64 gate */
        x_movri(X_RCX, 0 - 1);
        x_rip(0x8B, X_RDX, pc + (bkol - s) + 7, scr0);
        x_rip(0x8B, X_R8, pc + (bkol - s) + 7, scr1);
        x_alignpre(0);
        x_callimp(pc + (bkol - s), 10);          /* FlushInstructionCache */
        x_alignpost();
        return 1;
    }
    if (strsame(nm, "munmap")) {
        x_movri(X_R8, 0x8000);            /* MEM_RELEASE */
        x_movri(X_RDX, 0);                /* dwSize must be 0 */
        x_alignpre(0); x_callimp(pc + (bkol - s), bk_impof(tkg_wi[i])); x_alignpost();
        return 1;
    }
    if (strsame(nm, "close")) {
        x_fd2handle(pc, hstd);
        x_alignpre(0);
        x_callimp(pc + (bkol - s), bk_impof(tkg_wi[i]));
        x_alignpost();
        return 1;
    }
    if (strsame(nm, "open")) {
        x_movrr(X_RAX, X_R8);
        x_alignpre(3);
        x_rex(1, 0, 0, 0); ob(0x89); x_modrm(1, 0, 4); ob(0x24); ob(0x20);
        x_movri(X_R8, 3);
        x_movri(X_R9, 0);
        x_stackarg(40, 0x80);
        x_stackarg(48, 0);
        x_callimp(pc + (bkol - s), bk_impof(tkg_wi[i]));
        x_alignpost();
        return 1;
    }
    if (strsame(nm, "lseek")) {        /* SetFilePointer(h, low, NULL, method) */
        x_movrr(X_R9, X_R8); x_movri(X_R8, 0);
        x_fd2handle(pc + (bkol - s), hstd);
        x_alignpre(0); x_callimp(pc + (bkol - s), bk_impof(tkg_wi[i])); x_alignpost();
        return 1;
    }
    if (strsame(nm, "unlink") || strsame(nm, "rename")) {
        if (strsame(nm, "rename")) x_movri(X_R8, 1);  /* REPLACE_EXISTING */
        x_alignpre(0);
        x_callimp(pc + (bkol - s), bk_impof(tkg_wi[i]));
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
    {   int jp;                                      /* jns over neg + mov */
        ob(0x79); jp = bkol; ob(0);                  /* the mov's width is a
                                                        SHORT-form choice, so
                                                        measure, never count */
        ob(0x48); ob(0xF7); ob(0xD8);                /* neg rax */
        x_movri(12, 1);
        bkout[jp] = bkol - jp - 1;
    }
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
    if (strsame(o, "cvtid")) { x_sseg(0xF2, 0x2A, 0, a[1], 1, 0); x_movqg(a[0], 0); return 1; }
    if (strsame(o, "cvtis")) { x_sseg(0xF3, 0x2A, 0, a[1], 1, 0); x_movdg(a[0], 0); return 1; }
    if (strsame(o, "cvtud")) { x_u2f(a[1], 1); x_movqg(a[0], 0); return 1; }
    if (strsame(o, "cvtus")) { x_u2f(a[1], 0); x_movdg(a[0], 0); return 1; }
    if (strsame(o, "cvtdi")) { x_movqx(0, a[1]); x_sseg(0xF2, 0x2C, 0, a[0], 1, 1); return 1; }
    if (strsame(o, "cvtdu")) {
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
    if (strsame(o, "cvtsd")) { x_movdx(0, a[1]); x_ssex(0xF3, 0x5A, 0, 0, 0 - 1); x_movqg(a[0], 0); return 1; }
    if (strsame(o, "cvtds")) { x_movqx(0, a[1]); x_ssex(0xF2, 0x5A, 0, 0, 0 - 1); x_movdg(a[0], 0); return 1; }
    if (strsame(o, "fsqrt64")) { x_movqx(0, a[1]); x_ssex(0xF2, 0x51, 0, 0, 0 - 1); x_movqg(a[0], 0); return 1; }
    if (strsame(o, "fsqrt32")) { x_movdx(0, a[1]); x_ssex(0xF3, 0x51, 0, 0, 0 - 1); x_movdg(a[0], 0); return 1; }
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
        if (a[2] == 0) {                /* Darwin: dyld calls us, rdi/rsi */
            x_rip(0x89, 7, pc + 7, a[0] + bk_shift);
            x_rip(0x89, 6, pc + 14, a[1] + bk_shift);
            return 1;
        }
        x_rex(1, 0, 0, 0); ob(0x8B); x_modrm(0, 0, 4); ob(0x24);
        x_rip(0x89, X_RAX, pc + (bkol - s) + 7, a[0] + bk_shift);
        x_rex(1, 0, 0, 0); ob(0x8D); x_modrm(1, 0, 4); ob(0x24); ob(0x08);
        x_rip(0x89, X_RAX, pc + (bkol - s) + 7, a[1] + bk_shift);
        return 1;
    }
    if (op == TO_PUSH) { x_push(a[0]); return 1; }
    if (op == TO_POP) { x_pop(a[0]); return 1; }
    if (op == TO_ARGVGET) {
        int t;
        x_rip(0x8B, X_R11, pc + 7, a[2] + bk_shift);
        t = a[1];
        x_rex(1, 1, t >> 3, 1); ob(0x8B); x_modrm(0, 11, 4); ob(0xC3 | ((t & 7) << 3));
        x_movrr(a[0], X_R11);
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
    if (op == TO_WINARGS) {
        int s; s = bkol;
        x_alignpre(0);
        x_callimp(pc + (bkol - s), 6);
        x_alignpost();
        ob(0x48); ob(0x89); ob(0xC6); ob(0x48); ob(0x89); ob(0xC7); ob(0x31); ob(0xC9);
        x_rip(0x8D, 8, pc + (bkol - s) + 7, a[2] + bk_shift);
        k = 0; while (k < 93) { ob(BK_WA_X86[k] & 255); k = k + 1; }
        x_rip(0x89, 1, pc + (bkol - s) + 7, a[0] + bk_shift);
        x_rip(0x89, 8, pc + (bkol - s) + 7, a[1] + bk_shift);
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
        if (strsame(bk_nth(BH_ENC_Y, tkg_form[i]), "winapi")) return x_winapi(i, off);
        ob(0x0F); ob(0x05);
        /* Darwin: CF set means failure, rax holds errno -- `jnc +3` over
           `neg rax` [I-20] */
        if (bkos == 1) { ob(0x73); ob(0x03); ob(0x48); ob(0xF7); ob(0xD8); }
        return 1;
    }
    o = bk_nth(BKOPS, op);
    if (strsame(o, "mov")) { x_movrr(a[0], a[1]); return 1; }
    if (strsame(o, "imm")) { x_movri(a[0], a[1]); return 1; }
    {   int alu; int j;                        /* [I5] catalog.ENCSPEC */
        alu = 0 - 1; j = enc_ix(ENC_X86_ALU2, NENC_X86_ALU2, o);
        if (j >= 0) alu = ENC_X86_ALU2_V(j);
        if (alu >= 0) {
            s2 = x_alias(a[0], a[1], a[2]);
            if (a[0] != a[1]) x_movrr(a[0], a[1]);
            x_alu(alu, a[0], s2);
            return 1;
        }
    }
    if (enc_ix(ENC_X86_SHIFTEXT, NENC_X86_SHIFTEXT, o) >= 0) {
        x_movrr(X_R11, a[1]); x_movrr(X_RBX, a[2]);
        x_push(X_RCX);
        x_movrr(X_RCX, X_RBX);
        x_rex(1, 0, 0, 1); ob(0xD3);
        x_modrm(3, ENC_X86_SHIFTEXT_V(enc_ix(ENC_X86_SHIFTEXT, NENC_X86_SHIFTEXT, o)), X_R11);
        x_pop(X_RCX);
        x_movrr(a[0], X_R11);
        return 1;
    }
    if (strsame(o, "mul64")) {
        s2 = x_alias(a[0], a[1], a[2]);
        if (a[0] != a[1]) x_movrr(a[0], a[1]);
        x_rex(1, a[0] >> 3, 0, s2 >> 3); ob(0x0F); ob(0xAF); x_modrm(3, a[0], s2);
        return 1;
    }
    if (strsame(o, "load64")) { x_load(a[0], a[1], a[2], 8); return 1; }
    if (strsame(o, "store64")) { x_store(a[2], a[0], a[1], 8); return 1; }
    {   int cc; int j;                         /* [I5] catalog.ENCSPEC */
        cc = 0 - 1; j = enc_ix(ENC_X86_SETCC, NENC_X86_SETCC, o);
        if (j >= 0) cc = ENC_X86_SETCC_V(j);
        if (cc >= 0) { x_cmpset(cc, a[0], a[1], a[2]); return 1; }
    }
    if (strsame(o, ".frame")) {
        long n; n = a[0];
        x_aluimm(X_RSP, n >= 0 ? 5 : 0, n >= 0 ? n : 0 - n);
        return 1;
    }
    if (strsame(o, ".lea")) { x_rip(0x8D, a[0], pc + 7, bk_leaaddr(i)); return 1; }
    if (strsame(o, ".ld")) { x_load(a[0], a[1], a[2], a[3]); return 1; }
    if (strsame(o, ".st")) { x_store(a[2], a[0], a[1], a[3]); return 1; }
    if (x_fp(o, a)) return 1;
    if (strsame(o, ".zero")) {
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
    if (strsame(o, "callr")) {           /* call r64: FF /2 [S-15 B1] */
        if (a[0] >= 8) x_rex(0, 0, 0, 1);
        ob(0xFF); x_modrm(3, 2, a[0]);
        return 1;
    }
    if (strsame(o, ".div") || strsame(o, ".mod") || strsame(o, ".udiv") || strsame(o, ".umod")) {
        x_spadj(16, 5);
        x_mem(0x89, 0 - 1, X_RAX, X_RSP, 0, 1);
        x_mem(0x89, 0 - 1, X_RDX, X_RSP, 8, 1);
        x_movrr(X_R11, a[2]);
        x_movrr(X_RAX, a[1]);
        if (o[1] == 117) {                            /* .udiv .umod */
            x_rex(1, 0, 0, 0); ob(0x31); x_modrm(3, 2, 2);
            x_rex(1, 0, 0, 1); ob(0xF7); x_modrm(3, 6, 11);
        } else {
            ob(0x48); ob(0x99);
            x_rex(1, 0, 0, 1); ob(0xF7); x_modrm(3, 7, 11);
        }
        x_movrr(X_R11, (strsame(o, ".div") || strsame(o, ".udiv")) ? X_RAX : X_RDX);
        x_mem(0x8B, 0 - 1, X_RAX, X_RSP, 0, 1);
        x_mem(0x8B, 0 - 1, X_RDX, X_RSP, 8, 1);
        x_spadj(16, 0);
        x_movrr(a[0], X_R11);
        return 1;
    }
    if (strsame(o, "ret")) { ob(0xC3); return 1; }   /* the machine's own */
    if (strsame(o, "nop")) { ob(0x90); return 1; }
    if (strsame(o, "jump")) {
        tjk[i] = 2; tjt[i] = a[0];
        if (tshort[i]) { ob(0xEB); x_rel8(bk_label(a[0]) - (off + 2)); return 1; }
        ob(0xE9); x_rel(i, bk_label(a[0]) - (off + 5)); return 1;
    }
    if (strsame(o, "call")) {            /* call rel32: the return address
                                              lands where the old sequence put
                                              it, so ret and the frame walk
                                              are unchanged */
        ob(0xE8); x_rel(i, bk_label(a[0]) - (off + 5));
        return 1;
    }
    if (strsame(o, "jumpz")) {
        int r; r = a[0];
        x_rex(1, r >> 3, 0, r >> 3); ob(0x85); x_modrm(3, r, r);
        tjk[i] = 3 + 2; tjt[i] = a[1];
        if (tshort[i]) { ob(0x74); x_rel8(bk_label(a[1]) - (off + 3 + 2)); return 1; }
        ob(0x0F); ob(0x84); x_rel(i, bk_label(a[1]) - (off + 3 + 6));
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
long bk_idata_len; long bk_iat_off; long bk_cfg_off;
int bk_idata_layout(void) {
    long off; int k; int L; char *e;
    off = 40 + (BK_NIMP + 1) * 8 * 2;               /* desc + ILT + IAT */
    k = 0;
    while (k < BK_NIMP) {
        e = bk_nth(BK_IMPS, k); L = 0; while (e[L]) L = L + 1;
        L = 2 + L + 1; if (L % 2) L = L + 1;
        off = off + L; k = k + 1;
    }
    off = off + 13;                                  /* KERNEL32.dll and its NUL */
    off = (off + 7) / 8 * 8;
    bk_cfg_off = off; bk_iat_off = 40 + (BK_NIMP + 1) * 8;   /* after the ILT */
    bk_idata_len = off + 320;                        /* LOADCFG 0x140 */
    return 0;
}
long bk_macho_hdrs(void) { return 32 + (72 + 152 + 232 + 72 + 32 + 56 + 24 + 24 + 48 + 24 + 80 + 16) + 156; }

int bk_assemble(void) {
    long off; int i; int ok; int id;
    bk_sizing = 1; bk_textva = 0; bk_shift = 0;
    i = 0;
    while (i < tkn) { tshort[i] = 0; tfit[i] = 0; tjk[i] = 0; i = i + 1; }
    /* every size once, long forms */
    i = 0;
    while (i < tkn) {
        bkout = bkscr; bkol = 0;
        ok = bk_enc(i, 0);
        if (ok) tsz[i] = bkol; else tsz[i] = bkarch ? 4 : 2;
        i = i + 1;
    }
    /* rounds: lay out, mark every branch whose short form reaches, repeat */
    while (1) {
        int nfit; long d; int t;
        off = 0; i = 0;
        while (i < tkn) { toff[i] = off; off = off + tsz[i]; i = i + 1; }
        toff[tkn] = off;
        nfit = 0; i = 0;
        while (i < tkn) {
            if (tjk[i]) { if (tshort[i] == 0) {
                t = bklab_tpc[tjt[i]];
                d = (t < 0 ? 0 : toff[t]) - (toff[i] + tjk[i]);
                if (d >= 0 - 128 && d <= 127) { tfit[i] = 1; nfit = nfit + 1; }
            } }
            i = i + 1;
        }
        if (nfit == 0) break;
        i = 0;
        while (i < tkn) {
            if (tfit[i]) { tshort[i] = 1; tfit[i] = 0; tsz[i] = tjk[i]; }
            i = i + 1;
        }
    }
    bktlen = off;
    /* image.layout: where text and data land, known before encoding */
    bk_idata_layout();
    if (bk_runmode) {
        /* two mappings: text goes read-execute once it is written, data
           stays writable, so nothing is ever both [macOS forbids W^X] */
        /* room for the import slots at the end of the text: they must be
           within reach of a rip-relative call, and read-only suits them */
        bk_runtsz = bk_round((bktlen > 1 ? bktlen : 1) + 128, 16384);
        bk_rundsz = bk_round(bkdlen + 65536, 16384);
#ifdef _WIN32
        /* ONE region, PAGE_EXECUTE_READWRITE: the gate calls its imports
           rip-relative, and two separate allocations can land more than
           2 GB apart -- which is an access violation, not a bad call. */
        bk_runtext = __mmap(0, bk_runtsz + bk_rundsz, 0x3000, 4, 0, 0);
        bk_rundata = bk_runtext + bk_runtsz;
        {   long tbl; int q;
            tbl = bk_runtext + bk_runtsz - 8 * 16;   /* the import slots */
            q = 0;
            while (q < BK_NIMP) {
                char *c; long v; int b;
                c = (char *)(tbl + 8 * q); v = bk_impval[q]; b = 0;
                while (b < 8) { c[b] = (v >> (8 * b)) & 255; b = b + 1; }
                bk_imp[q] = tbl + 8 * q;
                q = q + 1;
            }
        }
#else
        bk_runtext = __mmap(0, bk_runtsz, 3, BK_MAP_ANON, 0 - 1, 0);
        bk_rundata = __mmap(0, bk_rundsz, 3, BK_MAP_ANON, 0 - 1, 0);
#endif
        if (bk_runtext == 0 - 1 || bk_rundata == 0 - 1 ||
            bk_runtext == 0 || bk_rundata == 0) {
            __write(2, "run: cannot map memory\n", 23); __exit(1);
        }
        bk_textva = bk_runtext;
        bk_datava = bk_rundata;
    } else { if (bkos == 1) {
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
        i = 0; while (i < BK_NIMP) { bk_imp[i] = 5368709120 + rd + bk_iat_off + 8 * i; i = i + 1; }
    } } }
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

