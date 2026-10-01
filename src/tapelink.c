/* ---- the unisacc linker: our own unit objects, joined at the TAPE level ----
   (0.0.17 R17-2, docs/toolchain.md §7.)  An object compiled with `-c -b T
   -funit` is a real ELF / Mach-O / COFF object for system tools, and also
   carries its unit tape in a non-loaded section, after the marker
   "UNISATAPE1 <length>\n".  `unisacc a.o b.o [-b T] -o prog` (or, by default,
   runs them) reads those tapes and writes ONE program tape:
     - every name a unit defines and does not `.global` is renamed `__u<k>_name`
       (static functions, string pools, internal labels, the carried library
       bodies -- each unit keeps its own);
     - `.global` names stay as they are and meet the other units' `.extern`
       references by name; a global object defined in several units is one
       object (the first `.bss` wins, as C's common definitions do -- this is
       how every unit's copy of `errno` becomes one);
     - each unit's initialiser `__init_u` becomes `__init_u<k>`, and the program
       gets `__init`, which calls them in link order;
     - `.global` / `.extern` records are dropped: the result is a whole program.
   The joined tape then goes through the ordinary back end, so every one of
   the six targets is available and the bytes are the back end's own.
   Not handled (named, not guessed): objects from other compilers (they carry
   no tape: refused), a C name that is spelled like a tape register (r0..r7). */

#ifndef TL_PRODUCT
int tl_fail(char *m) { __write(2, m, blen(m)); __exit(1); return 1; }
#else
int tl_fail(char *m);                          /* the product driver's */
#endif
#define TL_HASH 262144
int tl_hat[TL_HASH]; int tl_hlen[TL_HASH]; int tl_hkind[TL_HASH];   /* per unit: offset+1 into tl_t */
char *tl_t;
char tl_gpool[1048576]; int tl_gend;                                 /* global objects already defined */
int tl_gat[65536]; int tl_glen[65536]; int tl_ng;
int tl_inits;                                                        /* units that had an __init_u */
#ifndef TL_PRODUCT
char tl_in[33554432];
#endif

int tl_isid(int c) {
    return (c >= 65 && c <= 90) || (c >= 97 && c <= 122) || (c >= 48 && c <= 57) || c == 95 || c == 46 || c == 36;
}
int tl_hash(char *s, int n) {
    unsigned long h; int k; h = 5381; k = 0;
    while (k < n) { h = (h * 33 + (s[k] & 255)) & 4294967295; k = k + 1; }
    return (int)(h & (TL_HASH - 1));
}
int tl_same(char *a, int n, char *b, int m) {
    int k; if (n != m) return 0;
    k = 0; while (k < n) { if (a[k] != b[k]) return 0; k = k + 1; }
    return 1;
}
int tl_is(char *s, int n, char *lit) { int k; k = 0; while (lit[k]) k = k + 1; return tl_same(s, n, lit, k); }
int tl_slot(char *s, int n) {               /* the slot holding s, or the empty slot where it would go */
    int h; h = tl_hash(s, n);
    while (tl_hat[h]) {
        if (tl_same(tl_t + tl_hat[h] - 1, tl_hlen[h], s, n)) return h;
        h = (h + 1) & (TL_HASH - 1);
    }
    return h;
}
int tl_kind(char *s, int n) { int h; h = tl_slot(s, n); return tl_hat[h] ? tl_hkind[h] : 0; }
int tl_put(char *s, int n, int kind) {
    int h; h = tl_slot(s, n);
    if (tl_hat[h] == 0) { tl_hat[h] = (int)(s - tl_t) + 1; tl_hlen[h] = n; tl_hkind[h] = kind; }
    else { if (kind == 2) tl_hkind[h] = 2; }      /* `.global` wins over a definition seen first */
    return 0;
}
int tl_gseen(char *s, int n) {               /* a global object some earlier unit defined */
    int i; i = 0;
    while (i < tl_ng) { if (tl_same(tl_gpool + tl_gat[i], tl_glen[i], s, n)) return 1; i = i + 1; }
    return 0;
}
int tl_gadd(char *s, int n) {
    int k;
    if (tl_ng >= 65536 || tl_gend + n >= 1048576) tl_fail("link: too many global objects\n");
    tl_gat[tl_ng] = tl_gend; tl_glen[tl_ng] = n;
    k = 0; while (k < n) { tl_gpool[tl_gend + k] = s[k]; k = k + 1; }
    tl_gend = tl_gend + n; tl_ng = tl_ng + 1;
    return 0;
}
#ifndef TL_PRODUCT
int tl_o(int c) {
    if (nout >= MAXOUT) { __write(2, "link: program tape too large\n", 29); __exit(1); }
    out[nout] = c; nout = nout + 1; return 0;
}
#else
int tl_o(int c);                               /* the product driver's: appends to its Buf */
#endif
int tl_os(char *s, int n) { int k; k = 0; while (k < n) { tl_o(s[k] & 255); k = k + 1; } return 0; }
int tl_num(int v) { char d[16]; int n; n = 0; if (v == 0) { tl_o(48); return 0; }
    while (v > 0) { d[n] = 48 + v % 10; v = v / 10; n = n + 1; }
    while (n > 0) { n = n - 1; tl_o(d[n]); } return 0; }
/* a name, renamed for unit u when it is this unit's own */
int tl_name(char *s, int n, int u) {
    if (tl_is(s, n, "__init_u")) { tl_os("__init_u", 8); tl_num(u); return 0; }
    if (tl_is(s, n, "_start") || tl_is(s, n, "__init")) { tl_os(s, n); return 0; }
    if (tl_kind(s, n) == 1) { tl_os("__u", 3); tl_num(u); tl_o(95); }
    tl_os(s, n);
    return 0;
}
/* the operands of a line from p to e: identifiers renamed, strings copied */
int tl_operands(char *t, int p, int e, int u) {
    int q;
    while (p < e) {
        if (t[p] == 34) {
            q = p + 1;
            while (q < e && t[q] != 34) { if (t[q] == 92) q = q + 1; q = q + 1; }
            if (q < e) q = q + 1;
            tl_os(t + p, q - p); p = q; continue;
        }
        if (tl_isid(t[p] & 255) && (t[p] < 48 || t[p] > 57) && t[p] != 46 && t[p] != 36) {
            q = p; while (q < e && tl_isid(t[q] & 255)) q = q + 1;
            tl_name(t + p, q - p, u); p = q; continue;
        }
        if (tl_isid(t[p] & 255)) {            /* a number or a register-like word: as it is */
            q = p; while (q < e && tl_isid(t[q] & 255)) q = q + 1;
            tl_os(t + p, q - p); p = q; continue;
        }
        tl_o(t[p] & 255); p = p + 1;
    }
    return 0;
}
int tl_word(char *t, int p, int e) { int q; q = p; while (q < e && tl_isid(t[q] & 255)) q = q + 1; return q; }
int tl_unit(char *t, int n, int u) {
    int i; int e; int j; int q; int pass;
    tl_t = t;
    i = 0; while (i < TL_HASH) { tl_hat[i] = 0; i = i + 1; }
    pass = 0;
    while (pass < 2) {                        /* 0: .global names; 1: everything defined here */
        i = 0;
        while (i < n) {
            e = i; while (e < n && t[e] != 10) e = e + 1;
            if (e > i) {
                if (t[i] == 46) {
                    j = i + 1; q = tl_word(t, j, e);
                    if (pass == 0 && tl_same(t + j, q - j, "global", 6)) { j = q + 1; q = tl_word(t, j, e); tl_put(t + j, q - j, 2); }
                    if (pass == 1 && (tl_same(t + j, q - j, "bss", 3) || tl_same(t + j, q - j, "str", 3))) {
                        j = q + 1; q = tl_word(t, j, e); tl_put(t + j, q - j, 1);
                    }
                } else { if (pass == 1 && t[i] != 32 && t[i] != 9 && t[e - 1] == 58) tl_put(t + i, e - 1 - i, 1); }
            }
            i = e + 1;
        }
        pass = pass + 1;
    }
    i = 0;
    while (i < n) {
        e = i; while (e < n && t[e] != 10) e = e + 1;
        if (e > i) {
            if (t[i] == 46) {
                j = i + 1; q = tl_word(t, j, e);
                if (tl_same(t + j, q - j, "global", 6) || tl_same(t + j, q - j, "extern", 6)) { i = e + 1; continue; }
                if (tl_same(t + j, q - j, "bss", 3)) {
                    int a; int b; a = q + 1; b = tl_word(t, a, e);
                    if (tl_kind(t + a, b - a) == 2) {
                        if (tl_gseen(t + a, b - a)) { i = e + 1; continue; }   /* one object for every unit */
                        tl_gadd(t + a, b - a);
                    }
                }
                tl_os(t + i, q - i);           /* the directive word itself */
                tl_operands(t, q, e, u);
            } else { if (t[i] == 32 || t[i] == 9) {
                j = i; while (j < e && (t[j] == 32 || t[j] == 9)) j = j + 1;
                q = j; while (q < e && t[q] != 32 && t[q] != 9) q = q + 1;   /* the op, as it is */
                /* `.sys NAME, ...`: NAME is a system call, never a unit's name
                   (a unit that carries `exit` or `rename` must not rename it) */
                if (tl_same(t + j, q - j, ".sys", 4) || tl_same(t + j, q - j, ".sys6", 5)) {
                    while (q < e && (t[q] == 32 || t[q] == 9)) q = q + 1;
                    while (q < e && t[q] != 44) q = q + 1;
                }
                tl_os(t + i, q - i);
                tl_operands(t, q, e, u);
            } else {
                if (t[e - 1] == 58) {
                    if (tl_is(t + i, e - 1 - i, "__init_u")) tl_inits = tl_inits | 0;
                    /* R19-8: an exported name defined by two units (two `main`s) */
                    if (tl_kind(t + i, e - 1 - i) == 2 && e - 1 - i < 250 && !tl_is(t + i, e - 1 - i, "__init_u")) {
                        char fk[256]; int q; fk[0] = 1; q = 0;
                        while (q < e - 1 - i) { fk[q + 1] = t[i + q]; q = q + 1; }
                        if (tl_gseen(fk, q + 1)) {
                            char msg[320]; int m; char *pre; pre = "link: multiple definitions of ";
                            m = 0; while (pre[m]) { msg[m] = pre[m]; m = m + 1; }
                            q = 0; while (q < e - 1 - i) { msg[m] = t[i + q]; m = m + 1; q = q + 1; }
                            msg[m] = 10; msg[m + 1] = 0;
                            tl_fail(msg);
                        }
                        tl_gadd(fk, q + 1);
                    }
                    tl_name(t + i, e - 1 - i, u); tl_o(58);
                } else tl_os(t + i, e - i);
            } }
        }
        tl_o(10);
        i = e + 1;
    }
    return 0;
}
/* ---- archives (R17-3): GNU/BSD `ar` format, written and read in memory ----
   `unisacc ar rcs|t|x LIB.a [members]`.  Member names longer than 15 bytes
   use the BSD `#1/<len>` form, which GNU ar, llvm-ar and ld64 all read.  The
   unisacc linker takes `.a` inputs and pulls in a member only when it
   defines a `.global` name that is still undefined (the classic rule), so a
   library of unit objects links like one made by a system `ar`. */
long tl_dec(char *b, int w) { long v; int k; v = 0; k = 0; while (k < w && b[k] >= 48 && b[k] <= 57) { v = v * 10 + (b[k] - 48); k = k + 1; } return v; }
int tl_field(char *dst, int w, long v, char *s) {   /* left-justified field, space padded */
    char d[24]; int n; int k; k = 0;
    if (s) { while (s[k] && k < w) { dst[k] = s[k]; k = k + 1; } }
    else { n = 0; if (v == 0) d[n++] = 48; while (v > 0) { d[n] = 48 + v % 10; v = v / 10; n = n + 1; }
           while (n > 0 && k < w) { n = n - 1; dst[k] = d[n]; k = k + 1; } }
    while (k < w) { dst[k] = 32; k = k + 1; }
    return 0;
}
/* append one member (name, bytes) to an archive being built in ab; returns the new length */
long tl_ar_add(char *ab, long at, char *name, char *data, long n) {
    char hdr[60]; int nl; int k; long body;
    nl = 0; while (name[nl]) nl = nl + 1;
    k = nl; while (k > 0 && name[k - 1] != 47) k = k - 1;    /* the base name */
    name = name + k; nl = nl - k;
    body = nl > 15 ? n + nl : n;
    if (nl > 15) { char lf[16]; lf[0] = 35; lf[1] = 49; lf[2] = 47; tl_field(lf + 3, 13, nl, 0); k = 0; while (k < 16) { hdr[k] = lf[k]; k = k + 1; } }
    else { tl_field(hdr, 16, 0, name); if (nl < 16) hdr[nl] = 32; }
    tl_field(hdr + 16, 12, 0, 0); tl_field(hdr + 28, 6, 0, 0); tl_field(hdr + 34, 6, 0, 0);
    tl_field(hdr + 40, 8, 0, "100644"); tl_field(hdr + 48, 10, body, 0); hdr[58] = 96; hdr[59] = 10;
    k = 0; while (k < 60) { ab[at + k] = hdr[k]; k = k + 1; } at = at + 60;
    if (nl > 15) { k = 0; while (k < nl) { ab[at + k] = name[k]; k = k + 1; } at = at + nl; }
    k = 0; while (k < n) { ab[at + k] = data[k]; k = k + 1; } at = at + n;
    if (body % 2) { ab[at] = 10; at = at + 1; }
    return at;
}
/* the i-th member of an archive (symbol tables skipped): name into nm, data, length; 0 past the end */
char *tl_ar_member(char *ab, long n, int i, char *nm, long *len) {
    long at; int k; long sz; int nl; int idx; char *d; long dl;
    if (n < 8 || ab[0] != 33 || ab[1] != 60 || ab[2] != 97 || ab[3] != 114) return 0;   /* "!<ar" */
    at = 8; idx = 0;
    while (at + 60 <= n) {
        sz = tl_dec(ab + at + 48, 10); nl = 0;
        if (ab[at] == 35 && ab[at + 1] == 49 && ab[at + 2] == 47) nl = (int)tl_dec(ab + at + 3, 13);
        if (nl > 0) { k = 0; while (k < nl && k < 255 && ab[at + 60 + k]) { nm[k] = ab[at + 60 + k]; k = k + 1; } nm[k] = 0; d = ab + at + 60 + nl; dl = sz - nl; }
        else { k = 0; while (k < 16 && ab[at + k] != 32) { nm[k] = ab[at + k]; k = k + 1; }
               if (k > 1 && nm[k - 1] == 47) k = k - 1; nm[k] = 0; d = ab + at + 60; dl = sz; }
        /* GNU "/" and "//" tables, BSD "__.SYMDEF..." */
        if (!(nm[0] == 47 || nm[0] == 0 || (nm[0] == 95 && nm[1] == 95 && nm[2] == 46 && nm[3] == 83))) {
            if (idx == i) { *len = dl; return d; }
            idx = idx + 1;
        }
        at = at + 60 + sz; if (at % 2) at = at + 1;
    }
    return 0;
}
/* link-time name sets, for pulling archive members: names defined (.global)
   and names wanted (.extern) by the units taken so far */
char tl_npool[2097152]; int tl_nend;
int tl_def_at[65536]; int tl_def_len[65536]; int tl_ndef;
int tl_ext_at[65536]; int tl_ext_len[65536]; int tl_next;
int tl_inset(int *at, int *ln, int cnt, char *s, int n) {
    int i; i = 0; while (i < cnt) { if (tl_same(tl_npool + at[i], ln[i], s, n)) return 1; i = i + 1; } return 0;
}
int tl_addname(int *at, int *ln, int *cnt, char *s, int n) {
    int k;
    if (*cnt >= 65536 || tl_nend + n >= 2097152) tl_fail("link: too many names\n");
    if (tl_inset(at, ln, *cnt, s, n)) return 0;
    at[*cnt] = tl_nend; ln[*cnt] = n; k = 0; while (k < n) { tl_npool[tl_nend + k] = s[k]; k = k + 1; }
    tl_nend = tl_nend + n; *cnt = *cnt + 1; return 0;
}
int tl_islink(char *t, long i, long e, char *w) {          /* ".global NAME" / ".extern NAME" */
    int k; k = 0; while (w[k]) { if (i + k >= e || t[i + k] != w[k]) return 0; k = k + 1; } return 1;
}
int tl_note_links(char *t, long n) {
    long i; long e;
    i = 0;
    while (i < n) {
        e = i; while (e < n && t[e] != 10) e = e + 1;
        if (tl_islink(t, i, e, ".global ")) tl_addname(tl_def_at, tl_def_len, &tl_ndef, t + i + 8, (int)(e - i - 8));
        if (tl_islink(t, i, e, ".extern ")) tl_addname(tl_ext_at, tl_ext_len, &tl_next, t + i + 8, (int)(e - i - 8));
        i = e + 1;
    }
    return 0;
}
int tl_needs(char *t, long n) {                              /* defines something still undefined? */
    long i; long e; int l;
    i = 0;
    while (i < n) {
        e = i; while (e < n && t[e] != 10) e = e + 1;
        if (tl_islink(t, i, e, ".global ")) {
            l = (int)(e - i - 8);
            if (tl_inset(tl_ext_at, tl_ext_len, tl_next, t + i + 8, l) && !tl_inset(tl_def_at, tl_def_len, tl_ndef, t + i + 8, l)) return 1;
        }
        i = e + 1;
    }
    return 0;
}
int isarchive(char *p) { int n; n = 0; while (p[n]) n = n + 1; return n >= 2 && p[n - 2] == 46 && p[n - 1] == 97; }

/* the tape a unit object carries, or 0; its target goes to tl_target */
char tl_target[32]; char tl_first[32];
char *tl_tape(char *b, long n, long *len) {
    long i; long k; long v; int q;
    i = 0;
    while (i + 11 < n) {
        if (b[i] == 85 && b[i + 1] == 78 && b[i + 2] == 73 && b[i + 3] == 83 && b[i + 4] == 65 && b[i + 5] == 84
            && b[i + 6] == 65 && b[i + 7] == 80 && b[i + 8] == 69 && b[i + 9] == 49 && b[i + 10] == 32) {
            k = i + 11; q = 0;
            while (k < n && b[k] != 32 && q < 31) { tl_target[q] = b[k]; q = q + 1; k = k + 1; }
            tl_target[q] = 0; k = k + 1; v = 0;
            while (k < n && b[k] >= 48 && b[k] <= 57) { v = v * 10 + (b[k] - 48); k = k + 1; }
            if (k < n && b[k] == 10 && k + 1 + v <= n) { *len = v; return b + k + 1; }
        }
        i = i + 1;
    }
    return 0;
}
#ifndef TL_PRODUCT
int isobject(char *p) {
    int n; n = 0; while (p[n]) n = n + 1;
    if (n >= 2 && p[n - 2] == 46 && (p[n - 1] == 111 || p[n - 1] == 97)) return 1;              /* .o, .a */
    if (n >= 4 && p[n - 4] == 46 && p[n - 3] == 111 && p[n - 2] == 98 && p[n - 1] == 106) return 1;  /* .obj */
    return 0;
}
/* join the unit objects (and the archive members they need) into the program tape in `out` */
int tl_takeunit(char *path, char *t, long len, int u) {
    int k;
    if (u == 0) { k = 0; while (tl_target[k]) { tl_first[k] = tl_target[k]; k = k + 1; } tl_first[k] = 0; }
    else { if (strsame(tl_first, tl_target) == 0) {
        __write(2, "unisacc: error: the objects were compiled for different targets (", 65); __write(2, tl_first, blen(tl_first));
        __write(2, ", ", 2); __write(2, tl_target, blen(tl_target)); __write(2, ")\n", 2); return 1; } }
    tl_unit(t, (int)len, u); tl_note_links(t, len);
    return 0;
}
int tl_readfile(char *path, long *n) {
    int fd;
    fd = ropen(path);
    if (fd < 0) return enoinput(path) ? 1 : 1;
    *n = __read(fd, tl_in, 33554432);
    __close(fd);
    if (*n < 0 || *n >= 33554432) { printf("link: cannot read %s\n", path); return 1; }
    return 0;
}
char tl_used[65536];
int fe_link(char **paths, int npath) {
    int u; int p; long n; long len; char *t; int k; int i; int changed; char nm[256];
    model_dims(); setup();
    nout = 0; tl_ng = 0; tl_gend = 0; tl_nend = 0; tl_ndef = 0; tl_next = 0;
    u = 0; p = 0;
    while (p < npath) {                       /* every object is taken */
        if (isarchive(paths[p]) == 0) {
            if (tl_readfile(paths[p], &n)) return 1;
            t = tl_tape(tl_in, n, &len);
            if (t == 0) {
                __write(2, "unisacc: error: ", 16); __write(2, paths[p], blen(paths[p]));
                __write(2, " carries no unit tape (compile it with -c -b os/arch -funit; objects from other compilers are not linked yet)\n", 108);
                return 1;
            }
            if (tl_takeunit(paths[p], t, len, u)) return 1;
            u = u + 1;
        }
        p = p + 1;
    }
    p = 0;
    while (p < npath) {                       /* each archive, in order: members that define a wanted name */
        if (isarchive(paths[p])) {
            if (tl_readfile(paths[p], &n)) return 1;
            k = 0; while (k < 65536) { tl_used[k] = 0; k = k + 1; }
            changed = 1;
            while (changed) {
                changed = 0; i = 0;
                while (1) {
                    char *m; m = tl_ar_member(tl_in, n, i, nm, &len);
                    if (m == 0) break;
                    if (i < 65536 && tl_used[i] == 0) {
                        long tl2; t = tl_tape(m, len, &tl2);
                        if (t && tl_needs(t, tl2)) { if (tl_takeunit(nm, t, tl2, u)) return 1; u = u + 1; tl_used[i] = 1; changed = 1; }
                    }
                    i = i + 1;
                }
            }
        }
        p = p + 1;
    }
    tl_os("__init:\n", 8);
    k = 0; while (k < u) { tl_os("  call __init_u", 15); tl_num(k); tl_o(10); k = k + 1; }
    tl_os("  ret\n", 6);
    return 0;
}
/* `unisacc ar rcs|t|x LIB.a [MEMBER...]` (R17-3) */
char tl_arbuf[33554432];
int tl_ar(int argc) {
    char *op; char *lib; long at; int i; int fd; long n; char nm[256]; char *m; long len; int k;
    if (argc < 4) { __write(2, "usage: unisacc ar rcs|t|x LIB.a [MEMBER...]\n", 44); return 2; }
    op = __argv(2); lib = __argv(3);
    if (op[0] == 114 || op[0] == 113) {       /* r, q (c and s accepted: no index is needed by our linker) */
        tl_arbuf[0] = 33; tl_arbuf[1] = 60; tl_arbuf[2] = 97; tl_arbuf[3] = 114; tl_arbuf[4] = 99; tl_arbuf[5] = 104; tl_arbuf[6] = 62; tl_arbuf[7] = 10;
        at = 8; i = 4;
        while (i < argc) {
            if (tl_readfile(__argv(i), &n)) return 1;
            if (at + n + 400 >= 33554432) { __write(2, "ar: archive too large\n", 22); return 1; }
            at = tl_ar_add(tl_arbuf, at, __argv(i), tl_in, n);
            i = i + 1;
        }
        fd = wopen(lib);
        if (fd < 0) { printf("ar: cannot write %s\n", lib); return 1; }
        __write(fd, tl_arbuf, at); __close(fd);
        return 0;
    }
    if (tl_readfile(lib, &n)) return 1;
    i = 0;
    while (1) {
        m = tl_ar_member(tl_in, n, i, nm, &len);
        if (m == 0) break;
        if (op[0] == 116) { __write(1, nm, blen(nm)); __write(1, "\n", 1); }
        else { if (op[0] == 120) {
            fd = wopen(nm); if (fd < 0) { printf("ar: cannot write %s\n", nm); return 1; }
            k = 0; __write(fd, m, len); __close(fd);
        } else { __write(2, "ar: operation must be r, q, t or x\n", 35); return 2; } }
        i = i + 1;
    }
    if (i == 0 && !(n >= 8 && tl_in[0] == 33)) { __write(2, "ar: not an archive\n", 19); return 1; }
    return 0;
}
#endif
