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

#define TL_HASH 262144
int tl_hat[TL_HASH]; int tl_hlen[TL_HASH]; int tl_hkind[TL_HASH];   /* per unit: offset+1 into tl_t */
char *tl_t;
char tl_gpool[1048576]; int tl_gend;                                 /* global objects already defined */
int tl_gat[65536]; int tl_glen[65536]; int tl_ng;
int tl_inits;                                                        /* units that had an __init_u */
char tl_in[33554432];

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
    if (tl_ng >= 65536 || tl_gend + n >= 1048576) { __write(2, "link: too many global objects\n", 30); __exit(1); }
    tl_gat[tl_ng] = tl_gend; tl_glen[tl_ng] = n;
    k = 0; while (k < n) { tl_gpool[tl_gend + k] = s[k]; k = k + 1; }
    tl_gend = tl_gend + n; tl_ng = tl_ng + 1;
    return 0;
}
int tl_o(int c) {
    if (nout >= MAXOUT) { __write(2, "link: program tape too large\n", 29); __exit(1); }
    out[nout] = c; nout = nout + 1; return 0;
}
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
                tl_os(t + i, q - i);
                tl_operands(t, q, e, u);
            } else {
                if (t[e - 1] == 58) {
                    if (tl_is(t + i, e - 1 - i, "__init_u")) tl_inits = tl_inits | 0;
                    tl_name(t + i, e - 1 - i, u); tl_o(58);
                } else tl_os(t + i, e - i);
            } }
        }
        tl_o(10);
        i = e + 1;
    }
    return 0;
}
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
int isobject(char *p) {
    int n; n = 0; while (p[n]) n = n + 1;
    if (n >= 2 && p[n - 2] == 46 && p[n - 1] == 111) return 1;                                   /* .o */
    if (n >= 4 && p[n - 4] == 46 && p[n - 3] == 111 && p[n - 2] == 98 && p[n - 1] == 106) return 1;  /* .obj */
    return 0;
}
/* join the unit objects into the program tape in `out` */
int fe_link(char **paths, int npath) {
    int u; int fd; long n; long len; char *t; int k;
    model_dims(); setup();
    nout = 0; tl_ng = 0; tl_gend = 0;
    u = 0;
    while (u < npath) {
        fd = ropen(paths[u]);
        if (fd < 0) return enoinput(paths[u]);
        n = __read(fd, tl_in, 33554432);
        __close(fd);
        if (n < 0 || n >= 33554432) { printf("link: cannot read %s\n", paths[u]); return 1; }
        t = tl_tape(tl_in, n, &len);
        if (t == 0) {
            __write(2, "unisacc: error: ", 16); __write(2, paths[u], blen(paths[u]));
            __write(2, " carries no unit tape (compile it with -c -b os/arch -funit; objects from other compilers are not linked yet)\n", 108);
            return 1;
        }
        if (u == 0) { k = 0; while (tl_target[k]) { tl_first[k] = tl_target[k]; k = k + 1; } tl_first[k] = 0; }
        else { if (strsame(tl_first, tl_target) == 0) {
            __write(2, "unisacc: error: the objects were compiled for different targets (", 65); __write(2, tl_first, blen(tl_first));
            __write(2, ", ", 2); __write(2, tl_target, blen(tl_target)); __write(2, ")\n", 2); return 1; } }
        tl_unit(t, (int)len, u);
        u = u + 1;
    }
    tl_os("__init:\n", 8);
    k = 0; while (k < npath) { tl_os("  call __init_u", 15); tl_num(k); tl_o(10); k = k + 1; }
    tl_os("  ret\n", 6);
    return 0;
}
