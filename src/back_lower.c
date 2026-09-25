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
/* Which target this very binary runs on, for run mode [S-9], and the
   anonymous-mapping flags of that OS (MAP_PRIVATE | MAP_ANON). */
#ifdef __linux__
#define BK_HOST_OS 0
#define BK_MAP_ANON 34
#else
#ifdef _WIN32
#define BK_HOST_OS 2
#define BK_MAP_ANON 0
#else
#define BK_HOST_OS 1
#define BK_MAP_ANON 4098
#endif
#endif
#ifdef __aarch64__
#define BK_HOST_ARCH 1
#else
#define BK_HOST_ARCH 0
#endif

#define BK_MAXI 1048576              /* tape instructions */
#define BK_MAXT 2097152              /* lowered instructions */
#define BK_MAXN 262144              /* names: labels and data symbols */
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
int bkds_id[BK_MAXZ]; int bknds;     /* data symbols, each one ONCE */
int bk_dsym(int id) {
    int i;
    /* lower.zero_last reads a DICT of symbols, so a name appears once there
       however many times the tape names it.  A tentative definition that is
       later completed reaches this twice; the second registration used to
       make bk_repack rewrite the same symbol's address twice, the second
       time from an address it had already moved. */
    i = 0; while (i < bknds) { if (bkds_id[i] == id) return 0; i = i + 1; }
    if (bknds >= BK_MAXZ) { __write(2, "back end: too many data symbols\n", 32); __exit(1); }
    bkds_id[bknds] = id; bknds = bknds + 1; return 0;
}

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

/* Initialised blobs first, all-zero blobs after, each keeping its address
   mod 8 -- lower.zero_last, the same rule on the same blobs (a symbol's
   bytes up to the next symbol).  A stored run lies inside its own symbol's
   blob, so a blob is nonzero exactly when one of its runs has a nonzero
   byte.  The zeros end up as one tail the image does not store. */
long bkb_s[BK_MAXZ]; long bkb_e[BK_MAXZ]; long bkb_new[BK_MAXZ];
int bkb_nz[BK_MAXZ]; int bkb_r0[BK_MAXZ]; int bkb_r1[BK_MAXZ]; int bknb;
long bkt_at[BK_MAXZ]; int bkt_off[BK_MAXZ]; int bkt_len[BK_MAXZ];
int bk_repack(void) {
    int i; int b; int a; int r; int pass; int nt; long o; long cur; long k; long at;
    /* The blob starts are the symbols' offsets, SORTED and deduplicated --
       lower.zero_last says `sorted(set(...))` and means it.  A symbol is
       allocated in declaration order, which is address order only until a
       later declaration lands earlier; on a program this size it does (the
       first descent in unisacc's own 1874 data symbols is at 282), and the
       walk that used to assume the order ran off the end of bkb_s. */
    bknb = 0;
    i = 0;
    while (i < bknds) {
        o = bksym_addr[bkds_id[i]] - BK_DATA_BASE;
        b = bknb;                           /* insertion sort: nearly sorted */
        while (b > 0 && bkb_s[b - 1] > o) b = b - 1;
        if (b == 0 || bkb_s[b - 1] != o) {  /* a repeat is not a new blob */
            if (bknb >= BK_MAXZ) { __write(2, "back end: too many blobs\n", 25); __exit(1); }
            k = bknb;
            while (k > b) { bkb_s[k] = bkb_s[k - 1]; k = k - 1; }
            bkb_s[b] = o; bknb = bknb + 1;
        }
        i = i + 1;
    }
    if (bknb == 0 || bkb_s[0] != 0) {       /* the bytes before the first one */
        b = bknb;
        while (b > 0) { bkb_s[b] = bkb_s[b - 1]; b = b - 1; }
        bkb_s[0] = 0; bknb = bknb + 1;
    }
    b = 0; a = 0;
    while (b < bknb) {
        if (b + 1 < bknb) bkb_e[b] = bkb_s[b + 1]; else bkb_e[b] = bkdlen;
        bkb_nz[b] = 0; bkb_r0[b] = a;
        while (a < bknd && bkd_at[a] < bkb_e[b]) {
            k = 0;
            while (k < bkd_len[a]) { if (bkdata[bkd_off[a] + k]) { bkb_nz[b] = 1; break; } k = k + 1; }
            a = a + 1;
        }
        bkb_r1[b] = a;
        b = b + 1;
    }
    nt = bknd; r = 0;
    while (r < nt) { bkt_at[r] = bkd_at[r]; bkt_off[r] = bkd_off[r]; bkt_len[r] = bkd_len[r]; r = r + 1; }
    cur = 0; pass = 1;
    while (pass >= 0) {
        b = 0;
        while (b < bknb) {
            if (bkb_nz[b] == pass) {
                cur = cur + ((bkb_s[b] - cur) % 8 + 8) % 8;
                bkb_new[b] = cur; cur = cur + bkb_e[b] - bkb_s[b];
            }
            b = b + 1;
        }
        pass = pass - 1;
    }
    bknd = 0; bknz = 0; bkdlen = 0; pass = 1;
    while (pass >= 0) {
        b = 0;
        while (b < bknb) {
            if (bkb_nz[b] == pass) {
                r = bkb_r0[b];
                while (r < bkb_r1[b]) {
                    at = bkb_new[b] + bkt_at[r] - bkb_s[b];
                    bk_zeros(at - bkdlen);
                    bkd_at[bknd] = at; bkd_off[bknd] = bkt_off[r]; bkd_len[bknd] = bkt_len[r];
                    bknd = bknd + 1; bkdlen = at + bkt_len[r];
                    r = r + 1;
                }
            }
            b = b + 1;
        }
        pass = pass - 1;
    }
    bk_zeros(cur - bkdlen);
    /* bkb_s is sorted, and the symbols are not, so each one is looked up --
       a binary search, because 1874 symbols against 1874 blobs is a walk
       long enough to notice. */
    i = 0;
    while (i < bknds) {
        int lo; int hi;
        o = bksym_addr[bkds_id[i]] - BK_DATA_BASE;
        lo = 0; hi = bknb - 1; b = 0 - 1;
        while (lo <= hi) {
            int mid;
            mid = lo + (hi - lo) / 2;
            if (bkb_s[mid] == o) { b = mid; lo = hi + 1; }
            else { if (bkb_s[mid] < o) lo = mid + 1; else hi = mid - 1; }
        }
        if (b < 0) { __write(2, "back end: lost a data symbol\n", 29); __exit(1); }
        bksym_addr[bkds_id[i]] = BK_DATA_BASE + bkb_new[b];
        i = i + 1;
    }
    return 0;
}
/* the data up to its last nonzero byte: what an image stores */
long bk_nzlen(void) {
    int a; long k;
    a = bknd - 1;
    while (a >= 0) {
        k = bkd_len[a] - 1;
        while (k >= 0) { if (bkdata[bkd_off[a] + k]) return bkd_at[a] + k + 1; k = k - 1; }
        a = a - 1;
    }
    return 0;
}

/* ---- the tape's code -------------------------------------------------- */
/* operand kinds */
#define BK_R 1                      /* a tape register, value 0..7 */
#define BK_I 2                      /* an immediate */
#define BK_N 3                      /* a name: label or data symbol */
int bkop[BK_MAXI];
int bkak[BK_MAXI * 8]; long bkav[BK_MAXI * 8];   /* up to 8: `.sys6` */
int bkni;
int bkentry;                        /* the _start label's pc */

/* the tape's op names, in one packed list; an op is its index here */
char *BKOPS = "imm\000mov\000add64\000sub64\000mul64\000xor64\000and64\000or64\000shl64\000shr64\000lshr64\000.div\000.mod\000.udiv\000.umod\000slt64\000sle64\000ult64\000ule64\000eq\000ne\000load64\000store64\000.ld\000.st\000.lea\000.zero\000jump\000jumpz\000call\000callr\000ret\000.frame\000.arg\000.print\000.write\000.exit\000.sys\000.sys6\000.argc\000.argv\000nop\000fadd64\000fsub64\000fmul64\000fdiv64\000flt64\000fle64\000feq64\000fadd32\000fsub32\000fmul32\000fdiv32\000flt32\000fle32\000feq32\000cvtid\000cvtud\000cvtis\000cvtus\000cvtdi\000cvtdu\000cvtsd\000cvtds\000fsqrt64\000fsqrt32\000";
#define BKNOPS 66              /* BKOPS entries: `.sys6` made it 66 */
/* operand shapes, one char per operand: r i L s */
char *BKSHAPE = "ri\000rr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rri\000rir\000rrii\000riri\000rs\000rii\000L\000rL\000L\000r\000\000i\000ir\000r\000rr\000r\000srrr\000srrrrrr\000r\000rr\000\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rrr\000rr\000rr\000rr\000rr\000rr\000rr\000rr\000rr\000rr\000rr\000";

/* the i-th entry of a packed list: its start */
/* A catalog is a run of NUL-separated names.  Walking it from the front on
   every lookup made this the hottest thing in the compiler: `bk_cop` scans
   all 70 catalog ops and calls bk_nth for each, so one op lookup costs
   O(n^2) character scanning -- and there is one per lowered instruction.
   A sampling profile of the self-compile put 57% of the time inside the
   strlen the inner loop compiles to.
   So each list is indexed ONCE, the first time it is asked for, and the
   lists are few enough (nine) to find by pointer in a linear scan. */
#define BK_NLISTS 16
#define BK_NIDX 1024
char *bk_lists[BK_NLISTS]; int bk_idx[BK_NLISTS * BK_NIDX];
int bk_idxn[BK_NLISTS]; int bk_nlists;

char *bk_lastlist; int bk_lastslot;  /* calls cluster: the same list, again */
char *bkc_ptr[16]; int bkc_slot[16];  /* pointer -> slot, direct-mapped (see v_slot) */
int bk_index(char *list) {           /* the slot holding this list's offsets */
    int i; int k; int n; int h;
    if (list == bk_lastlist) return bk_lastslot;
    h = (int)(((long)list >> 3) & 15);
    if (bkc_ptr[h] == list) { bk_lastlist = list; bk_lastslot = bkc_slot[h]; return bkc_slot[h]; }
    i = 0;
    while (i < bk_nlists) {
        if (bk_lists[i] == list) { bk_lastlist = list; bk_lastslot = i; bkc_ptr[h] = list; bkc_slot[h] = i; return i; }
        i = i + 1;
    }
    if (bk_nlists >= BK_NLISTS) return 0 - 1;   /* fall back to the walk */
    i = bk_nlists; bk_nlists = bk_nlists + 1;
    bk_lists[i] = list;
    k = 0; n = 0;
    /* the catalog ends where an empty name would start: two NULs in a row */
    while (n < BK_NIDX) {
        bk_idx[i * BK_NIDX + n] = k; n = n + 1;
        while (list[k]) k = k + 1;
        k = k + 1;
        if (list[k] == 0) break;
    }
    bk_idxn[i] = n;
    bk_lastlist = list; bk_lastslot = i;
    return i;
}

char *bk_nth(char *list, int i) {
    int s; int k;
    s = bk_index(list);
    if (s >= 0 && i >= 0 && i < bk_idxn[s]) return list + bk_idx[s * BK_NIDX + i];
    k = 0;
    while (i > 0) { while (list[k]) k = k + 1; k = k + 1; i = i - 1; }
    return list + k;
}
int bk_opof(char *s, int n) {        /* hashed: see vfind */
    return vfind(BKOPS, BKNOPS, s, n);
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
            id = bk_name(t + s0, s1 - s0);
            /* Tape.string interns: a name already known keeps its address
               and reserves NOTHING the second time.  This used to allocate
               again and move the symbol, so a global defined twice -- which
               is what a tentative definition completed later looks like --
               put the two back ends 8 bytes apart. */
            if (bksym_addr[id] < 0) {
                bk_zeros((8 - bkdlen % 8) % 8);
                bksym_addr[id] = BK_DATA_BASE + bkdlen; bk_dsym(id);
                bk_zeros(cnt);
            }
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
                bksym_addr[id] = BK_DATA_BASE + bkdlen; bk_dsym(id);
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
                bkak[bkni * 8 + na] = BK_R; bkav[bkni * 8 + na] = t[j + 1] - 48;
                j = j + 2;
            } else { if (sh[na] == 105) {    /* an immediate, maybe +K/-K after a register */
                while (j < e && t[j] != 32 && t[j] != 44 && t[j] != 93) j = j + 1;
                bkak[bkni * 8 + na] = BK_I; bkav[bkni * 8 + na] = bk_num(t + q, j - q);
            } else {                         /* L or s: a name, or a number */
                while (j < e && t[j] != 32 && t[j] != 44) j = j + 1;
                if ((t[q] >= 48 && t[q] <= 57) || t[q] == 45) {
                    bkak[bkni * 8 + na] = BK_I; bkav[bkni * 8 + na] = bk_num(t + q, j - q);
                } else {
                    bkak[bkni * 8 + na] = BK_N; bkav[bkni * 8 + na] = bk_name(t + q, j - q);
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
int bkrel;                          /* the reloc answer for the branch being lowered */
/* run mode [S-9]: the program is assembled AT the addresses it will run at,
   in memory this process maps, instead of into an image on disk.  Nothing
   about the code changes -- the same encoders, the same tables -- only where
   text and data are placed and the fact that nobody writes a header. */
long bk_impval[16];                 /* run mode on Windows: the real routines */
int bk_runmode; long bk_runtext; long bk_rundata; long bk_runtsz; long bk_rundsz;
int bkf_nr;                         /* the syscall-number register (abi nrreg) */
int bkf_form; int bkf_gate; long bkf_sysno; int bkf_hasno;
int bkf_arg[6]; int bkf_ret;        /* machine register numbers, -1 none */
int bkf_sym;                        /* the symbol class index (isel), unused in bytes */
int bkf_argshape; int bkf_retconv; int bkf_winimp;   /* [I4] abi heads */

/* a class index's spelling -> a machine register number (-1 for none) */
int bk_regnum(char *nm) {
    char *x86; int i; int k; char *e;
    if (nm[0] == 120) {               /* x0..x8 */
        return bk_num(nm + 1, 2);
    }
    x86 = "rax\000rcx\000rdx\000rbx\000rsp\000rbp\000rsi\000rdi\000r8\000r9\000r10\000r11\000r12\000r13\000r14\000r15\000";
    k = 0; while (nm[k]) k = k + 1;
    i = vfind(x86, 16, nm, k);
    if (i >= 0) return i;
    return 0 - 1;                     /* none */
}
/* ask abi and enc for catalog op `cop` (an index into BF_ABI_0) -- only
   what the bytes use; isel is not a question the lowering obeys */
/* bk_facts depends on (cop, os, arch) alone, and the lowering asks it for
   every syscall-shaped instruction: fourteen table questions each time.
   The facts are kept per cop for the target in hand [J5]; a change of
   target (one process can write several) starts the cache over. */
#define BKF_MAXC 128
int bkfc_ok[BKF_MAXC]; int bkfc_os; int bkfc_arch;
int bkfc_hasno[BKF_MAXC]; long bkfc_sysno[BKF_MAXC]; int bkfc_arg[BKF_MAXC * 6];
int bkfc_ret[BKF_MAXC]; int bkfc_gate[BKF_MAXC]; int bkfc_nr[BKF_MAXC];
int bkfc_shape[BKF_MAXC]; int bkfc_rc[BKF_MAXC]; int bkfc_wi[BKF_MAXC]; int bkfc_form[BKF_MAXC];
int bk_facts_ask(int cop);
int bk_facts(int cop) {
    int k;
    if (bkfc_os != bkos + 1 || bkfc_arch != bkarch + 1) {
        k = 0; while (k < BKF_MAXC) { bkfc_ok[k] = 0; k = k + 1; }
        bkfc_os = bkos + 1; bkfc_arch = bkarch + 1;
    }
    if (cop < 0 || cop >= BKF_MAXC) return bk_facts_ask(cop);
    if (bkfc_ok[cop] == 0) {
        bk_facts_ask(cop);
        bkfc_hasno[cop] = bkf_hasno; bkfc_sysno[cop] = bkf_sysno;
        k = 0; while (k < 6) { bkfc_arg[cop * 6 + k] = bkf_arg[k]; k = k + 1; }
        bkfc_ret[cop] = bkf_ret; bkfc_gate[cop] = bkf_gate; bkfc_nr[cop] = bkf_nr;
        bkfc_shape[cop] = bkf_argshape; bkfc_rc[cop] = bkf_retconv; bkfc_wi[cop] = bkf_winimp;
        bkfc_form[cop] = bkf_form; bkfc_ok[cop] = 1;
        return 0;
    }
    bkf_hasno = bkfc_hasno[cop]; bkf_sysno = bkfc_sysno[cop];
    k = 0; while (k < 6) { bkf_arg[k] = bkfc_arg[cop * 6 + k]; k = k + 1; }
    bkf_ret = bkfc_ret[cop]; bkf_gate = bkfc_gate[cop]; bkf_nr = bkfc_nr[cop];
    bkf_argshape = bkfc_shape[cop]; bkf_retconv = bkfc_rc[cop]; bkf_winimp = bkfc_wi[cop];
    bkf_form = bkfc_form[cop];
    return 0;
}
int bk_facts_ask(int cop) {
    int key[4]; int c; char *nm;
    key[0] = cop; key[1] = bkos; key[2] = bkarch; key[3] = 0;
    c = inf(S_ABI, key, HD_ABI_SYSNO);
    nm = bk_nth(BH_ABI_SYSNO, c);
    bkf_hasno = 1;
    if (nm[0] == 110) bkf_hasno = 0;  /* none */
    else bkf_sysno = bk_num(nm, 64);

    bkf_arg[0] = bk_regnum(bk_nth(BH_ABI_ARG0, inf(S_ABI, key, HD_ABI_ARG0)));
    bkf_arg[1] = bk_regnum(bk_nth(BH_ABI_ARG1, inf(S_ABI, key, HD_ABI_ARG1)));
    bkf_arg[2] = bk_regnum(bk_nth(BH_ABI_ARG2, inf(S_ABI, key, HD_ABI_ARG2)));
    bkf_arg[3] = bk_regnum(bk_nth(BH_ABI_ARG3, inf(S_ABI, key, HD_ABI_ARG3)));
    bkf_arg[4] = bk_regnum(bk_nth(BH_ABI_ARG4, inf(S_ABI, key, HD_ABI_ARG4)));
    bkf_arg[5] = bk_regnum(bk_nth(BH_ABI_ARG5, inf(S_ABI, key, HD_ABI_ARG5)));
    bkf_ret = bk_regnum(bk_nth(BH_ABI_RET, inf(S_ABI, key, HD_ABI_RET)));
    bkf_gate = inf(S_ABI, key, HD_ABI_GATE);
    bkf_nr = bk_regnum(bk_nth(BH_ABI_NRREG, inf(S_ABI, key, HD_ABI_NRREG)));
    bkf_argshape = inf(S_ABI, key, HD_ABI_ARGSHAPE);
    bkf_retconv = inf(S_ABI, key, HD_ABI_RETCONV);
    bkf_winimp = inf(S_ABI, key, HD_ABI_WINIMP);
    bkf_form = inf(S_ENC, key, 0);        /* enc is os-aware, and wins */
    return 0;
}
int bk_cop(char *nm) {                  /* a catalog op's index */
    int k;
    k = 0; while (nm[k]) k = k + 1;
    return vfind(BF_ABI_0, NBF_ABI_0, nm, k);
}

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
#define TO_WINARGS 110
#define TO_PUSH 111
#define TO_POP 112
#define TO_ADDI 113
#define TO_SUBI 114
#define TO_LSLI 115
#define TO_SEXT 116
/* setreg's source kinds */
#define SK_IMM 1
#define SK_REG 2
#define SK_MEM 3
#define SK_ADDR 4
int tkop[BK_MAXT]; long tka[BK_MAXT * 4]; int tkk[BK_MAXT * 4]; int tkn;
/* gate metadata: form, gate, catalog op, return register */
int tkg_rc[BK_MAXT]; int tkg_wi[BK_MAXT];
int bk_impof(int c);                /* the IAT slot `winimp` names */
/* an op's index in one of catalog.ENCSPEC's tables, or -1 [I5] */
int enc_ix(char *tab, int n, char *o) { int L; L = 0; while (o[L]) L = L + 1; return vfind(tab, n, o, L); }
int tkg_rel[BK_MAXT]; int tkg_form[BK_MAXT]; int tkg_gate[BK_MAXT]; int tkg_cop[BK_MAXT]; int tkg_ret[BK_MAXT];
int bklab_tpc[BK_MAXN];            /* a label's lowered pc */
int bklab_first[BK_MAXI + 1]; int bklab_next[BK_MAXN];
long bk_scr0; long bk_scr1; long bk_plen; long bk_pbuf; long bk_argc; long bk_argv;
long bk_sysa; long bk_sysfp; long bk_syssp; long bk_argva; long bk_hstd; long bk_written; long bk_save; long bk_stacktop; long bk_bss;
int bk_rmap[8];                     /* tape register -> machine register */

int tk(int op, long a0, long a1, long a2, long a3) {
    if (tkn >= BK_MAXT) { __write(2, "back end: lowered program too long\n", 35); __exit(1); }
    tkop[tkn] = op; tka[tkn * 4] = a0; tka[tkn * 4 + 1] = a1; tka[tkn * 4 + 2] = a2; tka[tkn * 4 + 3] = a3;
    tkk[tkn * 4] = BK_I; tkk[tkn * 4 + 1] = BK_I; tkk[tkn * 4 + 2] = BK_I; tkk[tkn * 4 + 3] = BK_I;
    tkg_rel[tkn] = 0 - 1; tkg_form[tkn] = 0; tkg_gate[tkn] = 0; tkg_cop[tkn] = 0 - 1; tkg_ret[tkn] = 0 - 1;
    tkn = tkn + 1;
    return tkn - 1;
}
/* setreg dst, (kind, value) -- the kind rides in a3 */
int tk_setreg(int dst, int kind, long v) { return tk(TO_SETREG, dst, v, 0, kind); }
/* A SYSTEM CALL with no number that is not a WinAPI call: the gate would
   enter the kernel with whatever the number register held.  Refused, as
   lower.py refuses.  (Jumps and calls ask bk_facts too and have no number
   by nature; only the syscall sequences come here.) */
int bk_needno(void) {
    if (bkf_hasno == 0 && bkos != 2) {
        __write(2, "back end: no system call for this op on this OS\n", 49);
        __exit(1);
    }
    return 0;
}

/* lower.syscall_seq */
int bk_syscall6(int cop, long cell) {      /* six arguments, all spilled */
    int g; int i;
    bk_facts(cop);
    bk_needno();
    if (bkf_hasno) tk_setreg(bkf_nr, SK_IMM, bkf_sysno);
    /* a WinAPI call clobbers every tape register, the tape STACK POINTER
       included, so it is bracketed here exactly as the three-argument gate
       is.  Without this the compiler returned from VirtualAlloc with a
       garbage stack and fetched its next instruction from nowhere. */
    if (bkos == 2) tk(TO_WINSAVE, bk_save, 0, 0, 0);
    i = 0;
    while (i < 6) {
        if (bkf_arg[i] < 0) break;         /* Win64: the rest go on the stack */
        tk_setreg(bkf_arg[i], SK_MEM, cell + 8 * i);
        i = i + 1;
    }
    g = tk(TO_GATE, 0, 0, 0, 0);
    tkg_form[g] = bkf_form; tkg_gate[g] = bkf_gate; tkg_cop[g] = cop; tkg_ret[g] = bkf_ret;
    tkg_rc[g] = bkf_retconv; tkg_wi[g] = bkf_winimp;
    if (bkos == 2) tk(TO_WINREST, bk_save, bkf_ret, 0, 0);
    return 0;
}

/* a fifth argument, for the one call that has one (renameat2); used once */
int bk_a4k = 0 - 1; long bk_a4v;
int bk_syscall(int cop, int k0, long v0, int k1, long v1, int k2, long v2, int k3, long v3) {
    int g;
    bk_facts(cop);
    bk_needno();
    if (bkf_hasno) tk_setreg(bkf_nr, SK_IMM, bkf_sysno);
    if (bkos == 2) tk(TO_WINSAVE, bk_save, 0, 0, 0);
    tk_setreg(bkf_arg[0], k0, v0);
    tk_setreg(bkf_arg[1], k1, v1);
    tk_setreg(bkf_arg[2], k2, v2);
    if (k3 >= 0) tk_setreg(bkf_arg[3], k3, v3);
    if (bk_a4k >= 0) { tk_setreg(bkf_arg[4], bk_a4k, bk_a4v); bk_a4k = 0 - 1; }
    g = tk(TO_GATE, 0, 0, 0, 0);
    tkg_form[g] = bkf_form; tkg_gate[g] = bkf_gate; tkg_cop[g] = cop; tkg_ret[g] = bkf_ret;
    tkg_rc[g] = bkf_retconv; tkg_wi[g] = bkf_winimp;
    if (bkos == 2) tk(TO_WINREST, bk_save, bkf_ret, 0, 0);
    return 0;
}

/* [J9] arm64: `imm rK, c` then add64/sub64/mul64 taking rK as its second
   source is one instruction with c in it, when rK is provably dead -- the
   same test as lower.py's _fuse_imm and _dead_after, line for line. */
int bk_destfirst(char *sh) {
    return strsame(sh, "rrr") || strsame(sh, "rr") || strsame(sh, "ri")
        || strsame(sh, "rs") || strsame(sh, "rri") || strsame(sh, "rrii");
}
int bk_dead_after(int j, int r) {
    int n; int k; int first; char *sh;
    n = 0;
    while (j < bkni && n < 32) {
        if (bklab_first[j] >= 0) return 0;
        sh = bk_nth(BKSHAPE, bkop[j]);
        if (bk_is(bkop[j], ".write") || bk_destfirst(sh) == 0) return 0;
        k = 0; first = 1;
        while (sh[k]) {
            if (sh[k] == 114) {
                if (first == 0 && bkav[j * 8 + k] == r) return 0;
                first = 0;
            }
            k = k + 1;
        }
        if (bkav[j * 8] == r) return 1;
        j = j + 1; n = n + 1;
    }
    return 0;
}
/* [J9] `.frame 8; .st [r7+0], rX, W; .ld rY, [r7+0], W; .frame -8` is
   rY = rX sign-extended from W bytes: lower.py's _sext_trip */
int bk_sext_trip(int pc) {
    int st; int ld;
    if (pc + 3 >= bkni) return 0;
    if (bklab_first[pc + 1] >= 0 || bklab_first[pc + 2] >= 0 || bklab_first[pc + 3] >= 0) return 0;
    st = pc + 1; ld = pc + 2;
    if (bk_is(bkop[pc], ".frame") == 0 || bkav[pc * 8] != 8) return 0;
    if (bk_is(bkop[pc + 3], ".frame") == 0 || bkav[(pc + 3) * 8] != 0 - 8) return 0;
    if (bk_is(bkop[st], ".st") == 0 || bk_is(bkop[ld], ".ld") == 0) return 0;
    if (bkav[st * 8] != 7 || bkav[st * 8 + 1] != 0 || bkav[ld * 8 + 1] != 7 || bkav[ld * 8 + 2] != 0) return 0;
    if (bkav[st * 8 + 3] != bkav[ld * 8 + 3]) return 0;
    if (bkav[st * 8 + 3] != 1 && bkav[st * 8 + 3] != 2 && bkav[st * 8 + 3] != 4) return 0;
    return 1;
}
int bkf_op; long bkf_v;               /* the fused form: TO_ADDI.., its constant */
int bk_fuse_imm(int pc) {
    int b; long c; long v; int k; int lg;
    if (pc + 1 >= bkni || bklab_first[pc + 1] >= 0) return 0;
    b = pc + 1;
    if (bk_is(bkop[pc], "imm") == 0) return 0;
    if (bk_is(bkop[b], "add64") == 0 && bk_is(bkop[b], "sub64") == 0 && bk_is(bkop[b], "mul64") == 0) return 0;
    k = bkav[pc * 8]; c = bkav[pc * 8 + 1];
    if (bkav[b * 8 + 2] != k || bkav[b * 8 + 1] == k) return 0;
    if (bk_is(bkop[b], "mul64")) {
        if (c <= 0 || (c & (c - 1))) return 0;
        lg = 0; while (c > 1) { c = c / 2; lg = lg + 1; }
        bkf_op = TO_LSLI; v = lg;
    } else {
        v = bk_is(bkop[b], "add64") ? c : 0 - c;
        bkf_op = TO_ADDI;
        if (v < 0) { bkf_op = TO_SUBI; v = 0 - v; }
        if (v > 4095) return 0;
    }
    if (bkav[b * 8] != k && bk_dead_after(pc + 2, k) == 0) return 0;
    bkf_v = v;
    return 1;
}
int bk_genfacts(int op) {             /* the facts the generic path asks for op */
    char nm2[16]; char *e; int L2;
    e = bk_nth(BKOPS, op); L2 = 0;
    while (e[L2] && L2 < 15) { nm2[L2] = e[L2]; L2 = L2 + 1; } nm2[L2] = 0;
    if (bk_cop(nm2) >= 0) bk_facts(bk_cop(nm2));
    return 0;
}

int bk_lower(void) {
    long base; int pc; int op; int k; int cw;
    int j;
    bkrel = 0 - 1;
    /* data: pad to 8, then the scratch cells -- and on Windows the save area
       and the tape's own stack (that one bss, not file) */
    bk_zeros((8 - bkdlen % 8) % 8);
    base = BK_DATA_BASE + bkdlen;
    bk_scr0 = base; bk_scr1 = base + 8; bk_plen = base + 16; bk_pbuf = base + 24;
    bk_argc = base + 48; bk_argv = base + 56;
    bk_sysa = base + 80;                        /* six syscall spill cells */
    bk_sysfp = base + 128; bk_syssp = base + 136;   /* ...and the tape FP/SP */
    bk_hstd = base + 168; bk_written = base + 192; bk_save = base + 200;
    bk_argva = base + 264;                      /* argv[64] on Windows */
    bk_stacktop = base + 776 + 65536;
    if (bkos == 2) bk_zeros(776); else bk_zeros(168);
    bk_bss = 0; if (bkos == 2) bk_bss = 65536;
    /* tape register -> machine register: the regmap stage decides */
    j = 0;
    while (j < 8) {
        int key[4]; key[0] = j; key[1] = bkarch; key[2] = 0; key[3] = 0;
        bk_rmap[j] = bk_regnum(bk_nth(BH_REGMAP_Y, inf(S_REGMAP, key, 0)));
        j = j + 1;
    }
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
            if (bkos == 2 && bk_runmode == 0)
                tk(TO_WINARGS, bk_argc, bk_argv, bk_argva, 0);
            /* x86-64: the tape stack is the process stack, Windows too --
               r7 is rsp and the WinAPI gate aligns and restores it.  arm64
               keeps its own stack on Windows: x7 is not sp. */
            tk(TO_SPINIT, bk_rmap[7], (bkos == 2 && bkarch == 1) ? bk_stacktop : 0 - 1, 0, 0);
            /* In run mode nobody hands over argc/argv: the loader writes
               them into the two cells below before it jumps, so the entry
               takes no arguments and no calling convention is assumed --
               the caller may be a C compiler's or our own. [S-9] */
            if (bkos != 2 && bk_runmode == 0)
                tk(TO_ARGSAVE, bk_argc, bk_argv, bkos == 0, 0);
        }
        op = bkop[pc];
        if (bk_is(op, ".write")) {
            tk(TO_SETMEM, bk_scr0, bk_rmap[bkav[pc * 8]], 0, 0);
            tk(TO_SETMEM, bk_scr1, bk_rmap[bkav[pc * 8 + 1]], 0, 0);
            bk_syscall(bk_cop("write"), SK_IMM, 1, SK_MEM, bk_scr0, SK_MEM, bk_scr1, 0 - 1, 0);
        } else { if (bk_is(op, ".print")) {
            tk(TO_SETMEM, bk_scr0, bk_rmap[bkav[pc * 8]], 0, 0);
            tk(TO_ITOA, bk_scr0, bk_pbuf, bk_plen, 0);
            bk_syscall(bk_cop("write"), SK_IMM, 1, SK_ADDR, bk_pbuf, SK_MEM, bk_plen, 0 - 1, 0);
        } else { if (bk_is(op, ".sys")) {
            char nm[32]; int id; int L;
            tk(TO_SETMEM, bk_scr0, bk_rmap[bkav[pc * 8 + 1]], 0, 0);
            tk(TO_SETMEM, bk_scr1, bk_rmap[bkav[pc * 8 + 2]], 0, 0);
            tk(TO_SETMEM, bk_plen, bk_rmap[bkav[pc * 8 + 3]], 0, 0);
            id = bkav[pc * 8]; L = bkname_len[id]; if (L > 31) L = 31;
            k = 0; while (k < L) { nm[k] = bkpool[bkname_at[id] + k]; k = k + 1; } nm[L] = 0;
            cw = bk_cop(nm);
            /* how the three tape arguments become the call's: the abi
               table's `argshape` [I4] -- Linux/arm64's *at forms take
               AT_FDCWD first, renameat2 twice and flags 0 fifth */
            bk_facts(cw);
            {   char *sh; sh = bk_nth(BH_ABI_ARGSHAPE, bkf_argshape);
                if (strsame(sh, "atfd_1"))
                    bk_syscall(cw, SK_IMM, 0 - 100, SK_MEM, bk_scr0, SK_MEM, bk_scr1, SK_MEM, bk_plen);
                else { if (strsame(sh, "atfd_1_zero"))
                    bk_syscall(cw, SK_IMM, 0 - 100, SK_MEM, bk_scr0, SK_IMM, 0, 0 - 1, 0);
                else { if (strsame(sh, "atfd_2_zero5")) {
                    bk_a4k = SK_IMM; bk_a4v = 0;
                    bk_syscall(cw, SK_IMM, 0 - 100, SK_MEM, bk_scr0, SK_IMM, 0 - 100, SK_MEM, bk_scr1);
                } else
                    bk_syscall(cw, SK_MEM, bk_scr0, SK_MEM, bk_scr1, SK_MEM, bk_plen, 0 - 1, 0);
                } }
            }
            bk_facts(cw);
            tk(bk_opof("mov", 3), bk_rmap[0], bkf_ret < 0 ? 31 : bkf_ret, 0, 0);
        } else { if (bk_is(op, ".sys6")) {
            char nm[32]; int id; int L;
            k = 0;
            while (k < 6) {
                tk(TO_SETMEM, bk_sysa + 8 * k, bk_rmap[bkav[pc * 8 + 1 + k]], 0, 0);
                k = k + 1;
            }
            id = bkav[pc * 8]; L = bkname_len[id]; if (L > 31) L = 31;
            k = 0; while (k < L) { nm[k] = bkpool[bkname_at[id] + k]; k = k + 1; } nm[L] = 0;
            cw = bk_cop(nm);
            /* on x86-64 two of the six argument registers ARE the tape's
               frame and stack pointers: save them across the call [S-9] */
            tk(TO_SETMEM, bk_sysfp, bk_rmap[6], 0, 0);
            tk(TO_SETMEM, bk_syssp, bk_rmap[7], 0, 0);
            bk_syscall6(cw, bk_sysa);
            bk_facts(cw);
            tk(bk_opof("mov", 3), bk_rmap[0], bkf_ret < 0 ? 31 : bkf_ret, 0, 0);
            tk_setreg(bk_rmap[6], SK_MEM, bk_sysfp);
            tk_setreg(bk_rmap[7], SK_MEM, bk_syssp);
        } else { if (bk_is(op, ".exit")) {
            tk(TO_SETMEM, bk_scr0, bk_rmap[bkav[pc * 8]], 0, 0);
            bk_syscall(bk_cop("exit"), SK_MEM, bk_scr0, SK_IMM, 0, SK_IMM, 0, 0 - 1, 0);
        } else { if (bk_is(op, ".argc")) {
            tk_setreg(bk_rmap[bkav[pc * 8]], SK_MEM, bk_argc);
        } else { if (bk_is(op, ".argv")) {
            tk(TO_ARGVGET, bk_rmap[bkav[pc * 8]], bk_rmap[bkav[pc * 8 + 1]], bk_argv, 0);
        } else { if (bk_is(op, ".arg")) {
            bk_facts(bk_cop("add64"));                /* a plain move */
            tk(bk_opof("mov", 3), bk_rmap[bkav[pc * 8]], bk_rmap[bkav[pc * 8 + 1]], 0, 0);
        } else { if (bkarch == 0 && bk_is(op, ".frame") && bkav[pc * 8] == 8
                     && pc + 1 < bkni && bklab_first[pc + 1] < 0
                     && bk_is(bkop[pc + 1], "store64")
                     && bkav[(pc + 1) * 8] == 7 && bkav[(pc + 1) * 8 + 1] == 0) {
            /* `.frame 8; store64 [r7+0], r` is a push [S-15 B1] -- as
               lower.py fuses it, and only when no label can reach the
               second half */
            tk(TO_PUSH, bk_rmap[bkav[(pc + 1) * 8 + 2]], 0, 0, 0);
            pc = pc + 1;
        } else { if (bkarch == 0 && bk_is(op, "load64") && bkav[pc * 8 + 1] == 7
                     && bkav[pc * 8 + 2] == 0
                     && pc + 1 < bkni && bklab_first[pc + 1] < 0
                     && bk_is(bkop[pc + 1], ".frame") && bkav[(pc + 1) * 8] == 0 - 8) {
            tk(TO_POP, bk_rmap[bkav[pc * 8]], 0, 0, 0);
            pc = pc + 1;
        } else { if (bkarch == 1 && bk_sext_trip(pc)) {
            k = 0; while (k < 4) { bk_genfacts(bkop[pc + k]); k = k + 1; }
            tk(TO_SEXT, bk_rmap[bkav[(pc + 2) * 8]], bk_rmap[bkav[(pc + 1) * 8 + 2]], bkav[(pc + 1) * 8 + 3], 0);
            pc = pc + 3;
        } else { if (bkarch == 1 && bk_fuse_imm(pc)) {
            bk_genfacts(bkop[pc]); bk_genfacts(bkop[pc + 1]);
            tk(bkf_op, bk_rmap[bkav[(pc + 1) * 8]], bk_rmap[bkav[(pc + 1) * 8 + 1]], bkf_v, 0);
            pc = pc + 1;
        } else {
            /* every other op, with its registers mapped; its facts are asked
               as lower.py asks them (jumps also ask reloc) */
            char *sh; int n; int key[4];
            sh = bk_nth(BKSHAPE, op);
            if (bk_is(op, "jump") || bk_is(op, "jumpz") || bk_is(op, "call")) {
                key[0] = bk_is(op, "jump") ? 0 : (bk_is(op, "jumpz") ? 1 : 2); key[1] = bkarch; key[2] = 0; key[3] = 0;
                bkrel = inf(S_RELOC, key, 0);
                bk_facts(bk_cop(bk_is(op, "call") ? "call" : (bk_is(op, "jumpz") ? "jumpz" : "jump")));
            } else {
                char nm2[16]; char *e; int L2;
                e = bk_nth(BKOPS, op); L2 = 0;
                while (e[L2] && L2 < 15) { nm2[L2] = e[L2]; L2 = L2 + 1; } nm2[L2] = 0;
                if (bk_cop(nm2) >= 0) bk_facts(bk_cop(nm2));
            }
            n = tk(op, 0, 0, 0, 0);
            tkg_rel[n] = bkrel; bkrel = 0 - 1;
            k = 0;
            while (sh[k]) {
                if (bkak[pc * 8 + k] == BK_R) tka[n * 4 + k] = bk_rmap[bkav[pc * 8 + k]];
                else tka[n * 4 + k] = bkav[pc * 8 + k];
                tkk[n * 4 + k] = bkak[pc * 8 + k];
                k = k + 1;
            }
        } } } } } } } } } } } }
        pc = pc + 1;
    }
    return tkn;
}

