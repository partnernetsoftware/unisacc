/* ---- the image, streamed to fd 1 ------------------------------------- */
char bkwb[65536]; int bkwn; long bkwtot;

/* ---- SHA-256, for the Mach-O ad-hoc signature [I-19] -------------------
   Only what a signature needs: feed the image's bytes through, take a digest
   every 4 KB page.  Everything is masked to 32 bits by hand -- this subset
   has no uint32_t arithmetic of its own. */
long SHA_K[64] = {
    1116352408, 1899447441, 3049323471, 3921009573,
    961987163, 1508970993, 2453635748, 2870763221,
    3624381080, 310598401, 607225278, 1426881987,
    1925078388, 2162078206, 2614888103, 3248222580,
    3835390401, 4022224774, 264347078, 604807628,
    770255983, 1249150122, 1555081692, 1996064986,
    2554220882, 2821834349, 2952996808, 3210313671,
    3336571891, 3584528711, 113926993, 338241895,
    666307205, 773529912, 1294757372, 1396182291,
    1695183700, 1986661051, 2177026350, 2456956037,
    2730485921, 2820302411, 3259730800, 3345764771,
    3516065817, 3600352804, 4094571909, 275423344,
    430227734, 506948616, 659060556, 883997877,
    958139571, 1322822218, 1537002063, 1747873779,
    1955562222, 2024104815, 2227730452, 2361852424,
    2428436474, 2756734187, 3204031479, 3329325298
};
long sha_h[8]; char sha_blk[64]; int sha_bn; long sha_tot;

long sh_rr(long x, int n) {          /* rotate right, 32-bit */
    return ((x >> n) | (x << (32 - n))) & 4294967295;
}

int sha_init(void) {
    sha_h[0] = 1779033703; sha_h[1] = 3144134277; sha_h[2] = 1013904242;
    sha_h[3] = 2773480762; sha_h[4] = 1359893119; sha_h[5] = 2600822924;
    sha_h[6] = 528734635; sha_h[7] = 1541459225;
    sha_bn = 0; sha_tot = 0;
    return 0;
}

int sha_block(char *p) {
    long w[64]; long a; long b; long c; long d; long e; long f; long g; long h;
    long s0; long s1; long ch; long maj; long t1; long t2;
    int i;
    i = 0;
    while (i < 16) {
        /* through longs: a byte over 127 shifted left 24 overflows a 32-bit
           int, and the reference compiler's int IS 32 bits */
        long b0; long b1; long b2; long b3;
        b0 = p[i * 4] & 255; b1 = p[i * 4 + 1] & 255;
        b2 = p[i * 4 + 2] & 255; b3 = p[i * 4 + 3] & 255;
        w[i] = (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
        i = i + 1;
    }
    while (i < 64) {
        s0 = sh_rr(w[i - 15], 7) ^ sh_rr(w[i - 15], 18) ^ (w[i - 15] >> 3);
        s1 = sh_rr(w[i - 2], 17) ^ sh_rr(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = (w[i - 16] + s0 + w[i - 7] + s1) & 4294967295;
        i = i + 1;
    }
    a = sha_h[0]; b = sha_h[1]; c = sha_h[2]; d = sha_h[3];
    e = sha_h[4]; f = sha_h[5]; g = sha_h[6]; h = sha_h[7];
    i = 0;
    while (i < 64) {
        s1 = sh_rr(e, 6) ^ sh_rr(e, 11) ^ sh_rr(e, 25);
        ch = (e & f) ^ ((e ^ 4294967295) & g);
        t1 = (h + s1 + ch + SHA_K[i] + w[i]) & 4294967295;
        s0 = sh_rr(a, 2) ^ sh_rr(a, 13) ^ sh_rr(a, 22);
        maj = (a & b) ^ (a & c) ^ (b & c);
        t2 = (s0 + maj) & 4294967295;
        h = g; g = f; f = e; e = (d + t1) & 4294967295;
        d = c; c = b; b = a; a = (t1 + t2) & 4294967295;
        i = i + 1;
    }
    sha_h[0] = (sha_h[0] + a) & 4294967295; sha_h[1] = (sha_h[1] + b) & 4294967295;
    sha_h[2] = (sha_h[2] + c) & 4294967295; sha_h[3] = (sha_h[3] + d) & 4294967295;
    sha_h[4] = (sha_h[4] + e) & 4294967295; sha_h[5] = (sha_h[5] + f) & 4294967295;
    sha_h[6] = (sha_h[6] + g) & 4294967295; sha_h[7] = (sha_h[7] + h) & 4294967295;
    return 0;
}

int sha_byte(int v) {
    sha_blk[sha_bn] = v; sha_bn = sha_bn + 1; sha_tot = sha_tot + 1;
    if (sha_bn == 64) { sha_block(sha_blk); sha_bn = 0; }
    return 0;
}

int sha_final(char *out) {            /* 32 bytes */
    long bits; int i;
    bits = sha_tot * 8;
    sha_byte(128);
    while (sha_bn != 56) sha_byte(0);
    i = 7;
    while (i >= 0) { sha_byte((bits >> (8 * i)) & 255); i = i - 1; }
    i = 0;
    while (i < 8) {
        out[i * 4] = (sha_h[i] >> 24) & 255; out[i * 4 + 1] = (sha_h[i] >> 16) & 255;
        out[i * 4 + 2] = (sha_h[i] >> 8) & 255; out[i * 4 + 3] = sha_h[i] & 255;
        i = i + 1;
    }
    return 0;
}

int wflush(void) {
    if (bkwn) {
        if (at_cap) {                            /* -S: the object stays in memory for src/asmtext.c */
            int k; if (at_capn + bkwn > 33554432) { __write(2, "unisacc: error: object too large for -S\n", 41); __exit(1); }
            k = 0; while (k < bkwn) { at_cap[at_capn + k] = bkwb[k]; k = k + 1; }
            at_capn = at_capn + bkwn;
        } else __write(bkfd, bkwb, bkwn);
    }
    bkwn = 0; return 0;
}
/* while signing, every byte written also goes through SHA-256, and each
   4 KB page's digest is kept: the signature is the last thing in the file,
   so it can be written from these once the rest is out [I-19] */
#define BK_MAXPAGE 8192
int bk_sign; int bk_npage; int bk_pagen; char bk_hashes[BK_MAXPAGE * 32];

int wb(int b) {
    bkwb[bkwn] = b; bkwn = bkwn + 1; bkwtot = bkwtot + 1;
    if (bk_sign) {
        sha_byte(b & 255);
        bk_pagen = bk_pagen + 1;
        if (bk_pagen == 4096) {
            if (bk_npage >= BK_MAXPAGE) { __write(2, "sign: image too large\n", 22); __exit(1); }
            sha_final(bk_hashes + bk_npage * 32);
            bk_npage = bk_npage + 1; bk_pagen = 0; sha_init();
        }
    }
    if (bkwn == 65536) wflush();
    return 0;
}

int bk_sign_end(void) {               /* the partial last page, if any */
    if (bk_pagen > 0) {
        if (bk_npage >= BK_MAXPAGE) { __write(2, "sign: image too large\n", 22); __exit(1); }
        sha_final(bk_hashes + bk_npage * 32);
        bk_npage = bk_npage + 1; bk_pagen = 0;
    }
    bk_sign = 0;
    return 0;
}

/* big-endian, for the signature blob (everything else here is little) */
int wb32(long v) { wb((v >> 24) & 255); wb((v >> 16) & 255); wb((v >> 8) & 255); wb(v & 255); return 0; }
int wb64(long v) { wb32((v >> 32) & 4294967295); wb32(v & 4294967295); return 0; }
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
int wdata(long lim) {                /* the data's first lim bytes */
    int a; int z; long p; long k;
    a = 0; z = 0; p = 0;
    while (p < lim) {
        if (a < bknd && bkd_at[a] == p) {
            k = 0; while (k < bkd_len[a] && p + k < lim) { wb(bkdata[bkd_off[a] + k]); k = k + 1; }
            p = p + bkd_len[a]; a = a + 1; continue;
        }
        if (z < bknz && bkz_at[z] == p) {
            k = bkz_len[z]; if (p + k > lim) k = lim - p;
            wz(k); p = p + bkz_len[z]; z = z + 1; continue;
        }
        __write(2, "back end: data gap\n", 19); __exit(1);
    }
    return 0;
}

/* R21-4a': the dynamic form, written only when the program forwards to the
   host libc.  The smallest ELF ld.so accepts: PT_INTERP, a read-only region
   of dynstr/dynsym/hash/rela/dynamic before the code, and four GLOB_DAT
   slots at the start of the data segment -- the same four names and the
   same place (32 bytes below the data base) as the Mach-O eager bind.
   DT_NEEDED libc.so.6: glibc 2.34+ carries dlopen and friends in libc. */
int bk_elf_dyn(void) {
    long tend; long doff; long L; long B; long slots; int i; char *interp; int il;
    L = bk_nzlen(); B = 4194304;
    tend = 784 + bktlen; doff = bk_round(tend, 4096); slots = B + doff;
    interp = bkarch ? "/lib/ld-linux-aarch64.so.1" : "/lib64/ld-linux-x86-64.so.2";
    il = 0; while (interp[il]) il = il + 1;
    wb(127); wb(69); wb(76); wb(70); wb(2); wb(1); wb(1); wb(0); wz(8);
    w16(2); w16(bkarch ? 183 : 62); w32(1);
    w64(B + 784 + bk_entry); w64(64); w64(0);
    w32(0); w16(64); w16(56); w16(4); w16(0); w16(0); w16(0);
    w32(3); w32(4); w64(288); w64(B + 288); w64(B + 288); w64(il + 1); w64(il + 1); w64(1);   /* PT_INTERP */
    w32(1); w32(5); w64(0); w64(B); w64(B); w64(tend); w64(tend); w64(4096);
    w32(1); w32(6); w64(doff); w64(B + doff); w64(B + doff); w64(32 + L); w64(32 + bkdlen); w64(4096);
    w32(2); w32(4); w64(608); w64(B + 608); w64(B + 608); w64(176); w64(176); w64(8);           /* PT_DYNAMIC */
    wname(interp, 32);                                                       /* 288..320 */
    wb(0); wname("libc.so.6", 10); wname("dlopen", 7); wname("dlsym", 6); wname("dlclose", 8); wname("dlerror", 8);   /* 320..360 */
    wz(24);                                                                  /* dynsym 360..480 */
    i = 0; while (i < 4) { w32(i == 0 ? 11 : (i == 1 ? 18 : (i == 2 ? 24 : 32))); wb(0x12); wb(0); w16(0); w64(0); w64(0); i = i + 1; }
    w32(1); w32(5); wz(24);                                                  /* hash 480..512: one empty bucket */
    i = 0; while (i < 4) { w64(slots + 8 * i); w64(((long)(i + 1) << 32) | (bkarch ? 1025 : 6)); w64(0); i = i + 1; }   /* rela 512..608 */
    w64(1); w64(1); w64(4); w64(B + 480); w64(5); w64(B + 320); w64(6); w64(B + 360);       /* dynamic 608..784 */
    w64(10); w64(40); w64(11); w64(24); w64(7); w64(B + 512); w64(8); w64(96); w64(9); w64(24);
    w64(30); w64(8); w64(0); w64(0);                                         /* DT_FLAGS BIND_NOW; DT_NULL */
    wtext(); wz(doff - tend); wz(32); wdata(L);
    return 0;
}
int bk_elf(void) {
    long tend; long doff; long L;
    if (bk_dyn) return bk_elf_dyn();
    L = bk_nzlen();
    tend = 176 + bktlen;
    doff = bk_round(tend, 4096);
    wb(127); wb(69); wb(76); wb(70); wb(2); wb(1); wb(1); wb(0); wz(8);
    w16(2); w16(bkarch ? 183 : 62); w32(1);
    w64(4194304 + 176 + bk_entry); w64(64); w64(0);
    w32(0); w16(64); w16(56); w16(2); w16(0); w16(0); w16(0);
    w32(1); w32(5); w64(0); w64(4194304); w64(4194304); w64(tend); w64(tend); w64(4096);
    w32(1); w32(6); w64(doff); w64(4194304 + doff); w64(4194304 + doff); w64(L); w64(bkdlen); w64(4096);
    wtext(); wz(doff - tend); wdata(L);
    return 0;
}

/* ---- the relocatable object (-c -b lnx/ARCH), docs/toolchain.md §4 -----
   Eight sections; .text and .data are the image's bytes (the relocated
   fields excepted), .bss is the zero tail; one Rela per recorded relocation;
   every tape name a local symbol and `_start` the only global. */
int bk_oname(int id) {                       /* a tape name into the string table */
    int k; k = 0;
    while (k < bkname_len[id]) { wb(bkpool[bkname_at[id] + k]); k = k + 1; }
    wb(0); return bkname_len[id] + 1;
}
/* The object's symbol plan, shared by the three writers: every defined
   name is a symbol -- local, or global when the tape said `.global` (and
   `_start`) -- and every name a relocation targets without a definition
   here is undefined.  Order: locals, globals, undefined (ELF and Mach-O
   require it).  Exported and undefined DATA names drop the tape's `g_`
   prefix, so they meet C's names (`g_banner` is `banner`). */
/* -funit objects carry their unit tape for the unisacc linker (src/tapelink.c),
   in a section no system linker loads: "UNISATAPE1 <length>\n" + the tape. */
char *bk_objtape; long bk_objtapen; char *bk_objtarget;
long bk_tapelen(void) {                        /* the blob's length, 0 when not a unit object */
    long v; long d; int tl;
    if (unitmode == 0 || bk_objtape == 0) return 0;
    v = bk_objtapen; d = 1; while (v >= 10) { v = v / 10; d = d + 1; }
    tl = 0; while (bk_objtarget[tl]) tl = tl + 1;
    return 11 + tl + 1 + d + 1 + bk_objtapen;
}
int bk_tapew(void) {                           /* "UNISATAPE1 <os/arch> <length>\n" + tape */
    char dg[24]; long v; int n; long k;
    wname("UNISATAPE1 ", 11);
    k = 0; while (bk_objtarget[k]) { wb(bk_objtarget[k]); k = k + 1; } wb(32);
    v = bk_objtapen; n = 0; if (v == 0) { dg[0] = 48; n = 1; }
    while (v > 0) { dg[n] = 48 + v % 10; v = v / 10; n = n + 1; }
    while (n > 0) { n = n - 1; wb(dg[n]); }
    wb(10);
    k = 0; while (k < bk_objtapen) { wb(bk_objtape[k]); k = k + 1; }
    return 0;
}
int bksp_kind[BK_MAXN]; long bksp_pos[BK_MAXN]; int bksp_n1; int bksp_n2; int bksp_n3;
int bk_def(int id) { return bklab_tpc[id] >= 0 || bksym_addr[id] >= 0; }
int bk_symplan(void) {
    int id; int k; int start; int kind; long p;
    start = bk_find("_start", 6);
    id = 0;
    while (id < bknn) {
        bksp_kind[id] = 0;
        if (bk_def(id)) bksp_kind[id] = (bklink_global[id] || id == start) ? 2 : 1;
        id = id + 1;
    }
    k = 0;
    while (k < bknro) { if (bkro_sect[k] >= 1000) { id = bkro_sect[k] - 1000; if (bk_def(id) == 0) bksp_kind[id] = 3; } k = k + 1; }
    bksp_n1 = 0; bksp_n2 = 0; bksp_n3 = 0; kind = 1;
    while (kind <= 3) {
        id = 0;
        while (id < bknn) {
            if (bksp_kind[id] == kind) {
                if (kind == 1) p = bksp_n1; else { if (kind == 2) p = bksp_n2; else p = bksp_n3; }
                bksp_pos[id] = p;
                if (kind == 1) bksp_n1 = bksp_n1 + 1; else { if (kind == 2) bksp_n2 = bksp_n2 + 1; else bksp_n3 = bksp_n3 + 1; }
            }
            id = id + 1;
        }
        kind = kind + 1;
    }
    id = 0;                                   /* positions in the one ordered list */
    while (id < bknn) {
        if (bksp_kind[id] == 2) bksp_pos[id] = bksp_pos[id] + bksp_n1;
        if (bksp_kind[id] == 3) bksp_pos[id] = bksp_pos[id] + bksp_n1 + bksp_n2;
        id = id + 1;
    }
    return 0;
}
int bk_sprefix(int id) {                       /* chars of the tape name dropped: `g_` on exported/undefined data */
    if (bksp_kind[id] >= 2 && bkname_len[id] > 2 && bkpool[bkname_at[id]] == 103 && bkpool[bkname_at[id] + 1] == 95) return 2;
    /* cc interop: the thunk calls `__ccx_NAME`, the cc symbol NAME */
    if (bksp_kind[id] == 3 && bkname_len[id] > 6 && bkpool[bkname_at[id]] == 95 && bkpool[bkname_at[id] + 1] == 95 && bkpool[bkname_at[id] + 2] == 99
        && bkpool[bkname_at[id] + 3] == 99 && bkpool[bkname_at[id] + 4] == 120 && bkpool[bkname_at[id] + 5] == 95) return 6;
    /* cc interop inbound: the wrapper `__ccw_NAME` is the exported NAME */
    if (bksp_kind[id] == 2 && bkname_len[id] > 6 && bkpool[bkname_at[id]] == 95 && bkpool[bkname_at[id] + 1] == 95 && bkpool[bkname_at[id] + 2] == 99
        && bkpool[bkname_at[id] + 3] == 99 && bkpool[bkname_at[id] + 4] == 119 && bkpool[bkname_at[id] + 5] == 95) return 6;
    return 0;
}
/* cc interop inbound: the exported NAME is the wrapper, so the body's own
   local symbol is written NAME.body (one name, one symbol -- an assembler
   rejects a local and a global spelled alike) */
int bk_hasccw(int id) {
    int j; int n; int k; char *a;
    if (!bk_anyccw || bksp_kind[id] != 1) return 0;
    n = bkname_len[id]; a = bkpool + bkname_at[id];
    j = 0;
    while (j < bknn) {
        if (bkname_len[j] == n + 6) {
            char *b; b = bkpool + bkname_at[j];
            if (b[0] == 95 && b[1] == 95 && b[2] == 99 && b[3] == 99 && b[4] == 119 && b[5] == 95) {
                k = 0; while (k < n && b[6 + k] == a[k]) k = k + 1;
                if (k == n) return 1;
            }
        }
        j = j + 1;
    }
    return 0;
}
int bk_snamelen(int id, int us) { return bkname_len[id] - bk_sprefix(id) + us + (bk_hasccw(id) ? 5 : 0); }
int bk_sname_w(int id, int us) {               /* name + NUL; us: a leading `_` (Mach-O C names) */
    int k;
    if (us) wb(95);
    k = bk_sprefix(id);
    while (k < bkname_len[id]) { wb(bkpool[bkname_at[id] + k]); k = k + 1; }
    if (bk_hasccw(id)) { wb(46); wb(98); wb(111); wb(100); wb(121); }   /* .body */
    wb(0); return 0;
}
int bk_osym(long name, int info, int shndx, long value) {
    w32(name); wb(info); wb(0); w16(shndx); w64(value); w64(0); return 0;
}
int bk_oshdr(long name, long type, long flags, long off, long size, long link, long info, long align, long entsize) {
    w32(name); w32(type); w64(flags); w64(0); w64(off); w64(size); w32(link); w32(info); w64(align); w64(entsize); return 0;
}
int bk_elfobj(void) {
    long L; long bss; long tend; long doff; long roff; long soff; long stoff; long shoff; long shsoff;
    long nsym; long strlen; int id; int k; long name; int kind; long tlen; long toff8;
    L = bk_objnz; bss = bkdlen - L;
    bk_symplan();
    nsym = 4 + bksp_n1 + bksp_n2 + bksp_n3; strlen = 1; id = 0;
    while (id < bknn) { if (bksp_kind[id]) strlen = strlen + bk_snamelen(id, 0) + 1; id = id + 1; }
    tend = 64 + bktlen;
    doff = bk_round(tend, 16);
    roff = bk_round(doff + L, 8);
    soff = bk_round(roff + 24 * bknro, 8);
    stoff = soff + 24 * nsym;
    shsoff = stoff + strlen;
    tlen = bk_tapelen(); toff8 = shsoff + (tlen ? 67 : 55);
    shoff = bk_round(toff8 + tlen, 8);
    wb(127); wb(69); wb(76); wb(70); wb(2); wb(1); wb(1); wb(0); wz(8);
    w16(1); w16(bkarch ? 183 : 62); w32(1);
    w64(0); w64(0); w64(shoff);
    w32(0); w16(64); w16(0); w16(0); w16(64); w16(tlen ? 9 : 8); w16(7);
    wtext(); wz(doff - tend); wdata(L); wz(roff - (doff + L));
    k = 0;
    while (k < bknro) {
        long sym; long ty; sym = bkro_sect[k]; ty = bkro_type[k];
        if (sym >= 1000) sym = 4 + bksp_pos[sym - 1000];
        if (ty == 900) ty = 4;                    /* R_X86_64_PLT32 */
        if (ty == 901) ty = 283;                  /* R_AARCH64_CALL26 */
        w64(bkro_off[k]); w32(ty); w32(sym); w64(bkro_add[k]); k = k + 1;
    }
    wz(soff - (roff + 24 * bknro));
    bk_osym(0, 0, 0, 0);
    bk_osym(0, 3, 1, 0); bk_osym(0, 3, 2, 0); bk_osym(0, 3, 3, 0);    /* STT_SECTION */
    name = 1; kind = 1;
    while (kind <= 3) {
        id = 0;
        while (id < bknn) {
            if (bksp_kind[id] == kind) {
                int bind; bind = kind == 1 ? 0 : 16;
                if (kind == 3) bk_osym(name, bind, 0, 0);
                else { if (bklab_tpc[id] >= 0) bk_osym(name, bind | (kind == 2 ? 2 : 0), 1, toff[bklab_tpc[id]]);
                else { long v; v = bksym_addr[id] - BK_DATA_BASE;
                    if (v >= L) bk_osym(name, bind | (kind == 2 ? 1 : 0), 3, v - L); else bk_osym(name, bind | (kind == 2 ? 1 : 0), 2, v); } }
                name = name + bk_snamelen(id, 0) + 1;
            }
            id = id + 1;
        }
        kind = kind + 1;
    }
    wb(0); kind = 1;
    while (kind <= 3) { id = 0; while (id < bknn) { if (bksp_kind[id] == kind) bk_sname_w(id, 0); id = id + 1; } kind = kind + 1; }
    wname("", 1); wname(".text", 6); wname(".data", 6); wname(".bss", 5); wname(".rela.text", 11);
    wname(".symtab", 8); wname(".strtab", 8); wname(".shstrtab", 10);
    if (tlen) { wname(".unisa.tape", 12); bk_tapew(); }
    wz(shoff - (toff8 + tlen));
    bk_oshdr(0, 0, 0, 0, 0, 0, 0, 0, 0);
    bk_oshdr(1, 1, 6, 64, bktlen, 0, 0, 16, 0);                   /* .text  PROGBITS AX */
    bk_oshdr(7, 1, 3, doff, L, 0, 0, 16, 0);                      /* .data  PROGBITS WA */
    bk_oshdr(13, 8, 3, doff + L, bss, 0, 0, 16, 0);               /* .bss   NOBITS   WA */
    bk_oshdr(18, 4, 64, roff, 24 * bknro, 5, 1, 8, 24);           /* .rela.text  INFO_LINK -> .text */
    bk_oshdr(29, 2, 0, soff, 24 * nsym, 6, 4 + bksp_n1, 8, 24);   /* .symtab: sh_info = first global */
    bk_oshdr(37, 3, 0, stoff, strlen, 0, 0, 1, 0);
    bk_oshdr(45, 3, 0, shsoff, tlen ? 67 : 55, 0, 0, 1, 0);
    if (tlen) bk_oshdr(55, 1, 0, toff8, tlen, 0, 0, 1, 0);         /* .unisa.tape: PROGBITS, not loaded */
    return 0;
}
/* The Mach-O object (-c -b osx/ARCH): MH_OBJECT, one unnamed segment with
   __text/__data/__bss, LC_BUILD_VERSION, LC_SYMTAB, LC_DYSYMTAB.  ld64 wants
   EXTERNAL relocations on arm64, so each section gets a local anchor symbol
   (ltmp0..2) and every relocation names its anchor; arm64 carries a nonzero
   addend in a preceding ARM64_RELOC_ADDEND, x86_64 in the displacement field. */
int bk_oname_w(int id) {                     /* a tape name as a Mach-O local: _name */
    int k; wb(95); k = 0;
    while (k < bkname_len[id]) { wb(bkpool[bkname_at[id] + k]); k = k + 1; }
    wb(0); return bkname_len[id] + 2;
}
int bk_mrel(long addr, long sym, int pcrel, int ext, int type) {
    w32(addr); w32((sym & 16777215) | (pcrel << 24) | (2 << 25) | (ext << 27) | (type << 28)); return 0;
}
int bk_msym(long strx, int type, int sect, long value) { w32(strx); wb(type); wb(sect); w16(0); w64(value); return 0; }
int bk_machoobj(void) {
    long L; long bss; long ad; long ab; long offt; long offd; long roff; long nrel; long soff; long stoff;
    long nsym; long strsz; long name; long cmds; int id; int k; int start; long v; long tlen; long to; long at;
    L = bk_objnz; bss = bkdlen - L;
    ad = bk_round(bktlen, 16); ab = ad + bk_round(L, 16);
    start = bk_find("_start", 6);
    bk_symplan();
    nrel = 0; k = 0;
    while (k < bknro) { nrel = nrel + 1; if (bkarch && bkro_add[k] != 0 && bkro_type[k] != 901) nrel = nrel + 1; k = k + 1; }
    nsym = 3 + bksp_n1 + bksp_n2 + bksp_n3; strsz = 1 + 3 * 6; id = 0;
    while (id < bknn) { if (bksp_kind[id]) strsz = strsz + bk_snamelen(id, id != start) + 1; id = id + 1; }
    tlen = bk_tapelen();
    cmds = 72 + 80 * (tlen ? 4 : 3) + 24 + 24 + 80;
    offt = bk_round(32 + cmds, 16);
    offd = offt + ad;
    to = bk_round(offd + L, 8); at = ab + bk_round(bss, 16);
    roff = tlen ? bk_round(to + tlen, 8) : to;
    soff = roff + 8 * nrel;
    stoff = soff + 16 * nsym;
    w32(0xFEEDFACF); w32(bkarch ? 0x0100000C : 0x01000007); w32(bkarch ? 0 : 3);
    w32(1); w32(4); w32(cmds); w32(0); w32(0);
    w32(25); w32(72 + 80 * (tlen ? 4 : 3)); wname("", 16);
    w64(0); w64(tlen ? at + tlen : ab + bss); w64(offt); w64(tlen ? to + tlen - offt : ad + L); w32(7); w32(7); w32(tlen ? 4 : 3); w32(0);
    wname("__text", 16); wname("__TEXT", 16); w64(0); w64(bktlen); w32(offt); w32(4); w32(roff); w32(nrel); w32(0x80000400); w32(0); w32(0); w32(0);
    wname("__data", 16); wname("__DATA", 16); w64(ad); w64(L); w32(offd); w32(4); w32(0); w32(0); w32(0); w32(0); w32(0); w32(0);
    wname("__bss", 16); wname("__DATA", 16); w64(ab); w64(bss); w32(0); w32(4); w32(0); w32(0); w32(1); w32(0); w32(0); w32(0);
    if (tlen) { wname("__unisa_tape", 16); wname("__UNISA", 16); w64(at); w64(tlen); w32(to); w32(0); w32(0); w32(0); w32(0x02000000); w32(0); w32(0); w32(0); }   /* S_ATTR_DEBUG: not loaded */
    w32(0x32); w32(24); w32(1); w32(13 << 16); w32(13 << 16); w32(0);          /* LC_BUILD_VERSION macOS 13 */
    w32(2); w32(24); w32(soff); w32(nsym); w32(stoff); w32(strsz);              /* LC_SYMTAB */
    w32(0xB); w32(80); w32(0); w32(3 + bksp_n1); w32(3 + bksp_n1); w32(bksp_n2); w32(3 + bksp_n1 + bksp_n2); w32(bksp_n3); wz(48);   /* LC_DYSYMTAB */
    wz(offt - 32 - cmds);
    /* x86_64 SIGNED keeps its addend IN the displacement (S + field - (P+4));
       the encoder left 0 there and recorded the ELF-style A = offset - 4 */
    if (bkarch == 0) { k = 0; while (k < bknro) {
        long f; f = bkro_add[k] + 4;
        bktext[bkro_off[k]] = f & 255; bktext[bkro_off[k] + 1] = (f >> 8) & 255;
        bktext[bkro_off[k] + 2] = (f >> 16) & 255; bktext[bkro_off[k] + 3] = (f >> 24) & 255;
        k = k + 1; } }
    wtext(); wz(ad - bktlen); wdata(L); wz(to - offd - L);
    if (tlen) { bk_tapew(); wz(roff - to - tlen); }
    k = 0;
    while (k < bknro) {
        long a; long add; long s; a = bkro_off[k]; add = bkro_add[k];
        s = bkro_sect[k] >= 1000 ? 3 + bksp_pos[bkro_sect[k] - 1000] : bkro_sect[k] - 1;
        if (bkro_type[k] == 901) bk_mrel(a, s, 1, 1, 2);                          /* ARM64_RELOC_BRANCH26 */
        else { if (bkro_type[k] == 900) bk_mrel(a, s, 1, 1, 2);                   /* X86_64_RELOC_BRANCH */
        else { if (bkarch) {
            if (add != 0) bk_mrel(a, add, 0, 0, 10);                              /* ARM64_RELOC_ADDEND */
            if (bkro_type[k] == 275) bk_mrel(a, s, 1, 1, 3);                       /* ARM64_RELOC_PAGE21 */
            else bk_mrel(a, s, 0, 1, 4);                                           /* ARM64_RELOC_PAGEOFF12 */
        } else bk_mrel(a, s, 1, 1, 1); } }                                         /* X86_64_RELOC_SIGNED */
        k = k + 1;
    }
    bk_msym(1, 14, 1, 0); bk_msym(7, 14, 2, ad); bk_msym(13, 14, 3, ab);          /* ltmp0..2 */
    name = 19; k = 1;
    while (k <= 3) {
        id = 0;
        while (id < bknn) {
            if (bksp_kind[id] == k) {
                int ty; ty = k == 1 ? 14 : 15;                                     /* N_SECT, N_SECT|N_EXT */
                if (k == 3) bk_msym(name, 1, 0, 0);                               /* N_UNDF|N_EXT */
                else { if (bklab_tpc[id] >= 0) bk_msym(name, ty, 1, toff[bklab_tpc[id]]);
                else { v = bksym_addr[id] - BK_DATA_BASE;
                    if (v >= L) bk_msym(name, ty, 3, ab + v - L); else bk_msym(name, ty, 2, ad + v); } }
                name = name + bk_snamelen(id, id != start) + 1;
            }
            id = id + 1;
        }
        k = k + 1;
    }
    wb(0); wname("ltmp0", 6); wname("ltmp1", 6); wname("ltmp2", 6);
    k = 1;
    while (k <= 3) { id = 0; while (id < bknn) { if (bksp_kind[id] == k) bk_sname_w(id, id != start); id = id + 1; } k = k + 1; }
    return 0;
}
/* The COFF object (-c -b win/ARCH): three sections, section symbols as
   relocation targets, every tape name a static symbol, `_start` external,
   and each Windows import an undefined external `__imp_Name` (link against
   kernel32.lib).  AMD64 REL32 keeps its addend in the displacement; ARM64
   PAGEBASE_REL21 keeps a byte addend in immhi:immlo and PAGEOFFSET_12A the
   low 12 bits in imm12 -- the encoder left all of them zero. */
int bk_cname(char *s, int n, long *strpos) {          /* 8-byte short name, or 0 + string-table offset */
    int k;
    if (n <= 8) { k = 0; while (k < n) { wb(s[k]); k = k + 1; } while (k < 8) { wb(0); k = k + 1; } return 0; }
    w32(0); w32(*strpos); *strpos = *strpos + n + 1; return 0;
}
int bk_csym(char *s, int n, long *strpos, long value, int sect, int type, int cls) {
    bk_cname(s, n, strpos); w32(value); w16(sect); w16(type); wb(cls); wb(0); return 0;
}
int bk_coffobj(void) {
    long L; long bss; long offt; long offr; long offd; long offs; long nsym; long strpos; long v; long f; long tlen; long offtp;
    int id; int k; int start; int nimp; char *nm; int len; int base_imp;
    L = bk_objnz; bss = bkdlen + bk_bss - L;      /* + the arm64 tape stack (bk_bss), which is not in bkdlen */
    if (bknro > 65535) { __write(2, "object: more than 65535 relocations in .text\n", 45); __exit(1); }
    start = bk_find("_start", 6);
    bk_symplan();
    k = 0;
    while (k < bknro) {                                   /* the addends go into the instruction fields */
        long o; long add; o = bkro_off[k]; add = bkro_add[k];
        if ((bkro_sect[k] < 100 || bkro_sect[k] >= 1000) && bkro_type[k] != 901) {
            if (bkarch == 0) {
                f = add + 4;
                bktext[o] = f & 255; bktext[o + 1] = (f >> 8) & 255; bktext[o + 2] = (f >> 16) & 255; bktext[o + 3] = (f >> 24) & 255;
            } else {
                long w;
                if (add < 0 - 1048576 || add >= 1048576) { __write(2, "object: COFF ARM64 addend beyond 1 MB\n", 38); __exit(1); }
                w = (bktext[o] & 255) | ((bktext[o + 1] & 255) << 8) | ((bktext[o + 2] & 255) << 16) | ((long)(bktext[o + 3] & 255) << 24);
                if (bkro_type[k] == 275) w = w | ((add & 3) << 29) | (((add >> 2) & 0x7FFFF) << 5);
                else w = w | ((add & 0xFFF) << 10);
                bktext[o] = w & 255; bktext[o + 1] = (w >> 8) & 255; bktext[o + 2] = (w >> 16) & 255; bktext[o + 3] = (w >> 24) & 255;
            }
        }
        k = k + 1;
    }
    nsym = 3 + bksp_n1 + bksp_n2 + bksp_n3; base_imp = nsym; nimp = BK_NIMP; nsym = nsym + nimp;
    tlen = bk_tapelen();
    offt = tlen ? 192 : 144;
    offr = bk_round(offt + bktlen, 4);
    offd = bk_round(offr + 10 * bknro, 16);
    offtp = bk_round(offd + L, 4);
    offs = bk_round(offtp + tlen, 4);
    w16(bkarch ? 0xAA64 : 0x8664); w16(tlen ? 4 : 3); w32(0); w32(offs); w32(nsym); w16(0); w16(0);
    wname(".text", 8); w32(0); w32(0); w32(bktlen); w32(offt); w32(offr); w32(0); w16(bknro); w16(0); w32(0x60500020);
    wname(".data", 8); w32(0); w32(0); w32(L); w32(offd); w32(0); w32(0); w16(0); w16(0); w32(0xC0500040);
    wname(".bss", 8); w32(0); w32(0); w32(bss); w32(0); w32(0); w32(0); w16(0); w16(0); w32(0xC0500080);
    if (tlen) { wname(".unisatp", 8); w32(0); w32(0); w32(tlen); w32(offtp); w32(0); w32(0); w16(0); w16(0); w32(0x00100A00); }   /* LNK_INFO|LNK_REMOVE */
    wz(offt - (tlen ? 180 : 140));
    wtext(); wz(offr - offt - bktlen);
    k = 0;
    while (k < bknro) {
        long sym; int ty;
        if (bkro_sect[k] >= 1000) sym = 3 + bksp_pos[bkro_sect[k] - 1000];
        else sym = bkro_sect[k] >= 100 ? base_imp + bkro_sect[k] - 100 : bkro_sect[k] - 1;
        if (bkarch) ty = bkro_type[k] == 901 ? 3 : (bkro_type[k] == 275 ? 4 : 6); else ty = 4;   /* BRANCH26 / PAGEBASE_REL21 / PAGEOFFSET_12A; REL32 */
        w32(bkro_off[k]); w32(sym); w16(ty);
        k = k + 1;
    }
    wz(offd - offr - 10 * bknro);
    wdata(L); wz(offtp - offd - L);
    if (tlen) bk_tapew();
    wz(offs - offtp - tlen);
    strpos = 4;
    bk_csym(".text", 5, &strpos, 0, 1, 0, 3); bk_csym(".data", 5, &strpos, 0, 2, 0, 3); bk_csym(".bss", 4, &strpos, 0, 3, 0, 3);
    k = 1;
    while (k <= 3) {
        id = 0;
        while (id < bknn) {
            if (bksp_kind[id] == k) {
                char *sn; int sl; int cls; cls = k == 1 ? 3 : 2;           /* STATIC / EXTERNAL */
                sn = bkpool + bkname_at[id] + bk_sprefix(id); sl = bk_snamelen(id, 0);
                if (k == 3) bk_csym(sn, sl, &strpos, 0, 0, 0, 2);
                else { if (bklab_tpc[id] >= 0) bk_csym(sn, sl, &strpos, toff[bklab_tpc[id]], 1, k == 2 ? 0x20 : 0, cls);
                else { v = bksym_addr[id] - BK_DATA_BASE;
                    if (v >= L) bk_csym(sn, sl, &strpos, v - L, 3, 0, cls); else bk_csym(sn, sl, &strpos, v, 2, 0, cls); } }
            }
            id = id + 1;
        }
        k = k + 1;
    }
    nm = BK_IMPS; k = 0;
    while (k < nimp) {
        char b[48]; int q;
        len = 0; while (nm[len]) len = len + 1;
        b[0] = 95; b[1] = 95; b[2] = 105; b[3] = 109; b[4] = 112; b[5] = 95;   /* __imp_ */
        q = 0; while (q < len && q < 40) { b[6 + q] = nm[q]; q = q + 1; }
        bk_csym(b, 6 + q, &strpos, 0, 0, 0, 2);
        nm = nm + len + 1; k = k + 1;
    }
    w32(strpos);                                          /* the string table, sized first */
    k = 1;
    while (k <= 3) {
        id = 0;
        while (id < bknn) { if (bksp_kind[id] == k && bk_snamelen(id, 0) > 8) bk_sname_w(id, 0); id = id + 1; }
        k = k + 1;
    }
    nm = BK_IMPS; k = 0;
    while (k < nimp) {
        len = 0; while (nm[len]) len = len + 1;
        if (6 + len > 8) { int q; wname("__imp_", 6); q = 0; while (q < len) { wb(nm[q]); q = q + 1; } wb(0); }
        nm = nm + len + 1; k = k + 1;
    }
    return 0;
}
int bk_object(char *t, int n, char *target) {
    bk_objtape = t; bk_objtapen = n; bk_objtarget = target;   /* the unit tape, before pruning (tp_prune writes elsewhere) */
    bkos = 0; if (target[0] == 111) bkos = 1;
    if (target[0] == 119) bkos = 2;
    bkarch = 0; if (target[4] == 97) bkarch = 1;
    bk_objmode = 1;
    t = tp_prune(t, n); n = tp_prune_length;
    bk_parse(t, n);
    bk_repack();
    bk_lower();
    bk_assemble();
    bkwn = 0; bkwtot = 0;
    if (bkos == 1) bk_machoobj(); else { if (bkos == 2) bk_coffobj(); else bk_elfobj(); }
    wflush();
    bk_objmode = 0;
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
/* The same eager libSystem binding declaration as image/macho.py. */
int bk_dlbind(void) {
    int i; int j; char *name;
    i = 0;
    while (i < 4) {
        name = i == 0 ? "dlopen" : (i == 1 ? "dlsym" : (i == 2 ? "dlclose" : "dlerror"));
        wb(0x11); wb(0x51); wb(0x72); wb(8 * i); wb(0x40); wb(95);
        j = 0; while (name[j]) { wb(name[j]); j = j + 1; }
        wb(0); wb(0x90); i = i + 1;
    }
    wb(0); return 0;
}
int bk_macho(void) {
    long hdrs; long textsz; long datasz; long datavm; long link; long v; long L;
    long sigoff; long siglen; long linksz; long slots; long cdlen; int i;
    hdrs = bk_macho_hdrs();
    L = bk_nzlen();
    textsz = bk_round(hdrs + bktlen, 16384);
    datavm = bk_round(bkdlen + 32, 16384);   /* mapped */
    datasz = bk_round(L + 32, 16384);                         /* stored */
    link = textsz + datasz;
    v = 4294967296;
    w32(0xFEEDFACF); w32(bkarch ? 0x0100000C : 0x01000007); w32(bkarch ? 0 : 3);
    w32(2); w32(12); w32(hdrs - 32 - 156); w32(0x200085); w32(0);
    bk_seg("__PAGEZERO", 0, v, 0, 0, 0, 0, 0);
    bk_seg("__TEXT", v, textsz, 0, textsz, 5, 5, 1);
    bk_sect("__text", "__TEXT", v + hdrs, bktlen, hdrs, 0x80000400);
    bk_seg("__DATA", v + textsz, datavm, textsz, datasz, 3, 3, 2);
    bk_sect("__data", "__DATA", v + textsz, L + 32, textsz, 0);
    bk_sect("__bss", "__DATA", v + textsz + 32 + L, bkdlen - L, 0, 1);      /* S_ZEROFILL */
    /* __LINKEDIT holds the string table, then the ad-hoc signature */
    sigoff = (link + 8 + 58 + 15) / 16 * 16;
    slots = (sigoff + 4095) / 4096;
    cdlen = 88 + 6;                                  /* fixed part + "unisa\0" */
    siglen = 12 + 8 + cdlen + 32 * slots;
    linksz = sigoff - link + siglen;
    bk_seg("__LINKEDIT", v + textsz + datavm, bk_round(linksz, 16384), link, linksz, 1, 1, 0);
    w32(0xE); w32(32); w32(12); wname("/usr/lib/dyld", 20);
    w32(0xC); w32(56); w32(24); w32(0); w32(0x10000); w32(0x10000); wname("/usr/lib/libSystem.B.dylib", 32);
    w32(0x80000028); w32(24); w64(hdrs + bk_entry); w64(0);
    w32(0x32); w32(24); w32(1); w32(13 << 16); w32(13 << 16); w32(0);
    w32(0x80000022); w32(48); wz(8); w32(link + 8); w32(58); wz(24);
    w32(2); w32(24); w32(link); w32(0); w32(link); w32(8);
    w32(0xB); w32(80); wz(72);
    w32(0x1D); w32(16); w32(sigoff); w32(siglen);    /* LC_CODE_SIGNATURE */
    wz(156);
    wtext(); wz(textsz - hdrs - bktlen);
    wz(32); wdata(L); wz(link - textsz - 32 - L);
    wz(8); bk_dlbind();
    wz(sigoff - bkwtot);
    /* the hashes cover everything written so far -- this header included */
    bk_sign_end();
    wb32(0xFADE0CC0); wb32(12 + 8 + cdlen + 32 * slots); wb32(1);   /* SuperBlob */
    wb32(0); wb32(20);                                              /* slot 0 */
    wb32(0xFADE0C02); wb32(cdlen + 32 * slots); wb32(0x20400); wb32(2);
    wb32(cdlen); wb32(88); wb32(0); wb32(slots); wb32(sigoff);
    wb(32); wb(2); wb(0); wb(12); wb32(0);
    wb32(0); wb32(0); wb32(0); wb64(0);
    wb64(0); wb64(textsz); wb64(1);
    wb(117); wb(110); wb(105); wb(115); wb(97); wb(0);              /* "unisa" */
    i = 0;
    while (i < slots * 32) { wb(bk_hashes[i] & 255); i = i + 1; }
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
    long nmrva[BK_NIMPMAX]; long off; long dllrva; int j; int L; char *e; long cfgstart; long nzl; long raw;
    rd_rva = 4096 + bk_round(bktlen, 4096);
    dt_rva = rd_rva + bk_round(bk_idata_len, 4096);
    cookie_rva = dt_rva + bk_round(bkdlen > 1 ? bkdlen : 1, 8);
    rd_file = 1024 + bk_round(bktlen, 512);
    dt_file = rd_file + bk_round(bk_idata_len, 512);
    dlen2 = bk_round(bkdlen > 1 ? bkdlen : 1, 8) + 8;
    dvs = dlen2 + bk_bss;
    /* stored: up to the last nonzero byte; the rest and the cookie are zero-filled */
    nzl = bk_nzlen(); raw = bk_round(nzl > 1 ? nzl : 1, 8);
    rl_rva = dt_rva + bk_round(dvs, 4096);
    rl_file = dt_file + bk_round(raw, 512);
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
        else { if (j == 12) { w32(rd_rva + bk_iat_off); w32((bk_nimp() + 1) * 8); }
        else { w32(0); w32(0); } } } }
        j = j + 1;
    }
    bk_pesect(".text", 4096, bktlen, 1024, bktlen, 0x60000020);
    bk_pesect(".rdata", rd_rva, bk_idata_len, rd_file, bk_idata_len, 0x40000040);
    bk_pesect(".data", dt_rva, dvs, dt_file, raw, 0xC0000040);
    bk_pesect(".reloc", rl_rva, 12, rl_file, 12, 0x42000040);
    wz(1024 - bkwtot);
    wtext(); wz(bk_round(bktlen, 512) - bktlen);
    /* the import section, at rd_rva */
    off = 40 + (bk_nimp() + 1) * 8 * 2; j = 0;
    while (j < bk_nimp()) {
        e = bk_nth(BK_IMPS, j); L = 0; while (e[L]) L = L + 1;
        nmrva[j] = rd_rva + off;
        L = 2 + L + 1; if (L % 2) L = L + 1;
        off = off + L; j = j + 1;
    }
    dllrva = rd_rva + off;
    w32(rd_rva + 40); w32(0); w32(0); w32(dllrva);
    w32(rd_rva + 40 + (bk_nimp() + 1) * 8);           /* the IAT */
    wz(20);
    j = 0; while (j < bk_nimp()) { w64(nmrva[j]); j = j + 1; } w64(0);
    j = 0; while (j < bk_nimp()) { w64(nmrva[j]); j = j + 1; } w64(0);
    j = 0;
    while (j < bk_nimp()) {
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
    /* the stored data, 8-padded */
    wdata(nzl); wz(raw - nzl);
    wz(bk_round(raw, 512) - raw);
    /* .reloc: one block, the cookie pointer in the load config, DIR64 */
    reloc_rva = rd_rva + bk_cfg_off + 0x58;
    page = reloc_rva / 4096 * 4096;
    w32(page); w32(12); w16((10 << 12) | (reloc_rva - page)); w16(0);
    wz(512 - 12);
    return 0;
}

/* `unisacc FILE -b os/arch`: the tape in t[0..n), as an image on fd 1 */
/* ---- Windows: where are WriteFile and friends? -------------------------
   Code in memory has no import table, so the gate's `call [slot]` needs real
   slots.  This process HAS them: it is a PE with the same imports, so we find
   our own image, walk its import descriptors and take the address of each
   slot BY NAME -- by name, because the order is only ours if the compiler
   that built this binary was ours. [S-9] */
long bk_rd32(long p) {
    char *c; c = (char *)p;
    return (c[0] & 255) | ((c[1] & 255) << 8) | ((c[2] & 255) << 16) | ((long)(c[3] & 255) << 24);
}
long bk_rd64(long p) { return bk_rd32(p) | (bk_rd32(p + 4) << 32); }

int bk_win_imports(long here) {
    long base; long pe; long dd; long imp; long d; long ilt; long iat; long k;
    long nrva; char *nm; int i; int j; int ok;
    base = here & (0 - 4096);
    while (base > 65536) {                     /* the MZ our image starts with */
        char *c; c = (char *)base;
        if (c[0] == 77 && c[1] == 90) {
            pe = base + bk_rd32(base + 0x3C);
            if (bk_rd32(pe) == 0x00004550) break;      /* "PE\0\0" */
        }
        base = base - 4096;
    }
    if (base <= 65536) { __write(2, "run: cannot find this image\n", 28); __exit(1); }
    dd = pe + 24 + 112;                        /* the data directories */
    imp = base + bk_rd32(dd + 8);              /* entry 1: the import table */
    d = imp;
    while (bk_rd32(d + 12)) {                  /* until the null descriptor */
        ilt = bk_rd32(d);                      /* OriginalFirstThunk */
        if (ilt == 0) ilt = bk_rd32(d + 16);
        ilt = base + ilt;
        iat = base + bk_rd32(d + 16);
        k = 0;
        while (bk_rd64(ilt + 8 * k)) {
            nrva = bk_rd64(ilt + 8 * k);
            if (nrva > 0) { if ((nrva >> 63) == 0) {
                nm = (char *)(base + nrva + 2);         /* past the hint */
                i = 0;
                while (i < BK_NIMP) {
                    char *e; e = bk_nth(BK_IMPS, i);
                    ok = 1; j = 0;
                    while (e[j] || nm[j]) {
                        if (e[j] != nm[j]) { ok = 0; break; }
                        j = j + 1;
                    }
                    if (ok) bk_impval[i] = bk_rd64(iat + 8 * k);
                    i = i + 1;
                }
            } }
            k = k + 1;
        }
        d = d + 20;
    }
    i = 0;
    while (i < BK_NIMP) {
        if (bk_impval[i] == 0) {
            __write(2, "run: this image does not import ", 32);
            __write(2, bk_nth(BK_IMPS, i), 12); __write(2, "\n", 1);
            __exit(1);
        }
        i = i + 1;
    }
    return 0;
}

/* Compile the tape into memory and hand back the entry address. [S-9] */
long bk_run(char *t, int n, long argc, long argv) {
    int a; long k; char *d; char *c;
    bkos = BK_HOST_OS; bkarch = BK_HOST_ARCH; bk_runmode = 1;
#ifdef _WIN32
    /* the gate calls through import slots; in memory there is no import
       table, so it uses this process's own [S-9] */
    bk_win_imports((long)bk_win_imports);
#endif
    t = tp_prune(t, n); n = tp_prune_length;
    bk_parse(t, n);
    bk_repack();
    bk_lower();
    bk_assemble();
    /* the data: mapped pages are already zero, so only the stored runs move */
    a = 0;
    while (a < bknd) {
        d = (char *)(bk_rundata + bkd_at[a]);
        k = 0; while (k < bkd_len[a]) { d[k] = bkdata[bkd_off[a] + k]; k = k + 1; }
        a = a + 1;
    }
    /* argc and argv, straight into the program's own cells */
    c = (char *)(bk_rundata + bk_argc - BK_DATA_BASE);
    k = 0; while (k < 8) { c[k] = (argc >> (8 * k)) & 255; k = k + 1; }
    c = (char *)(bk_rundata + bk_argv - BK_DATA_BASE);
    k = 0; while (k < 8) { c[k] = (argv >> (8 * k)) & 255; k = k + 1; }
    d = (char *)bk_runtext;
    k = 0; while (k < bktlen) { d[k] = bktext[k]; k = k + 1; }
#ifdef _WIN32
    /* PAGE_EXECUTE_READ over the TEXT only, which is the sequence Windows
       expects: newly written code is still in the data cache, and on arm64
       the first instruction raises STATUS_ILLEGAL_INSTRUCTION unless the
       range is both re-protected and flushed (the gate does the flush).
       The data half stays writable -- the program's stack is in it. */
    if (__mprotect(bk_runtext, bk_runtsz, 0x20) != 0) {
#else
    if (__mprotect(bk_runtext, bk_runtsz, 5) != 0) {      /* READ | EXEC */
#endif
        __write(2, "run: cannot make the code executable\n", 37); __exit(1);
    }
    return bk_runtext + bk_entry;
}

int bk_build(char *t, int n, char *target) {
    bkos = 0;
    if (target[0] == 111) bkos = 1;                  /* osx */
    if (target[0] == 119) bkos = 2;                  /* win */
    bkarch = 0; if (target[4] == 97) bkarch = 1;     /* .../arm64 */
    t = tp_prune(t, n); n = tp_prune_length;
    bk_parse(t, n);
    bk_repack();
    bk_lower();
    bk_assemble();
    bkwn = 0; bkwtot = 0;
    if (bkos == 1) { bk_sign = 1; bk_npage = 0; bk_pagen = 0; sha_init(); }
    if (bkos == 0) bk_elf();
    if (bkos == 1) bk_macho();
    if (bkos == 2) bk_pe();
    wflush();
    return 0;
}
