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

int wflush(void) { if (bkwn) __write(bkfd, bkwb, bkwn); bkwn = 0; return 0; }
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

int bk_elf(void) {
    long tend; long doff; long L;
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
    long hdrs; long textsz; long datasz; long datavm; long link; long v; long L;
    long sigoff; long siglen; long linksz; long slots; long cdlen; int i;
    hdrs = bk_macho_hdrs();
    L = bk_nzlen();
    textsz = bk_round(hdrs + bktlen, 16384);
    datavm = bk_round(bkdlen > 1 ? bkdlen : 1, 16384);   /* mapped */
    datasz = bk_round(L, 16384);                         /* stored */
    link = textsz + datasz;
    v = 4294967296;
    w32(0xFEEDFACF); w32(bkarch ? 0x0100000C : 0x01000007); w32(bkarch ? 0 : 3);
    w32(2); w32(12); w32(hdrs - 32 - 156); w32(0x200085); w32(0);
    bk_seg("__PAGEZERO", 0, v, 0, 0, 0, 0, 0);
    bk_seg("__TEXT", v, textsz, 0, textsz, 5, 5, 1);
    bk_sect("__text", "__TEXT", v + hdrs, bktlen, hdrs, 0x80000400);
    bk_seg("__DATA", v + textsz, datavm, textsz, datasz, 3, 3, 2);
    bk_sect("__data", "__DATA", v + textsz, L, textsz, 0);
    bk_sect("__bss", "__DATA", v + textsz + L, bkdlen - L, 0, 1);      /* S_ZEROFILL */
    /* __LINKEDIT holds the string table, then the ad-hoc signature */
    sigoff = (link + 8 + 15) / 16 * 16;
    slots = (sigoff + 4095) / 4096;
    cdlen = 88 + 6;                                  /* fixed part + "unisa\0" */
    siglen = 12 + 8 + cdlen + 32 * slots;
    linksz = sigoff - link + siglen;
    bk_seg("__LINKEDIT", v + textsz + datavm, bk_round(linksz, 16384), link, linksz, 1, 1, 0);
    w32(0xE); w32(32); w32(12); wname("/usr/lib/dyld", 20);
    w32(0xC); w32(56); w32(24); w32(0); w32(0x10000); w32(0x10000); wname("/usr/lib/libSystem.B.dylib", 32);
    w32(0x80000028); w32(24); w64(hdrs + bk_entry); w64(0);
    w32(0x32); w32(24); w32(1); w32(13 << 16); w32(13 << 16); w32(0);
    w32(0x80000022); w32(48); wz(40);
    w32(2); w32(24); w32(link); w32(0); w32(link); w32(8);
    w32(0xB); w32(80); wz(72);
    w32(0x1D); w32(16); w32(sigoff); w32(siglen);    /* LC_CODE_SIGNATURE */
    wz(156);
    wtext(); wz(textsz - hdrs - bktlen);
    wdata(L); wz(link - textsz - L);
    wz(8);
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
    long nmrva[BK_NIMP]; long off; long dllrva; int j; int L; char *e; long cfgstart; long nzl; long raw;
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
        else { if (j == 12) { w32(rd_rva + bk_iat_off); w32((BK_NIMP + 1) * 8); }
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
    off = 40 + (BK_NIMP + 1) * 8 * 2; j = 0;
    while (j < BK_NIMP) {
        e = bk_nth(BK_IMPS, j); L = 0; while (e[L]) L = L + 1;
        nmrva[j] = rd_rva + off;
        L = 2 + L + 1; if (L % 2) L = L + 1;
        off = off + L; j = j + 1;
    }
    dllrva = rd_rva + off;
    w32(rd_rva + 40); w32(0); w32(0); w32(dllrva);
    w32(rd_rva + 40 + (BK_NIMP + 1) * 8);           /* the IAT */
    wz(20);
    j = 0; while (j < BK_NIMP) { w64(nmrva[j]); j = j + 1; } w64(0);
    j = 0; while (j < BK_NIMP) { w64(nmrva[j]); j = j + 1; } w64(0);
    j = 0;
    while (j < BK_NIMP) {
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
