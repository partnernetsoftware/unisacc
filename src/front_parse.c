/* ===================== stage 5: parser + tape emitter =================== */
/* Recursive descent mirroring the Python walker.  Every production choice
 * goes through infer(S_PARSE, ...) -- classic control flow, neural table. */

#define MAXOUT 33554432   /* a tape runs to ~5 bytes per source byte: 4 MB of C needs ~20 MB */
#define MAXSYM 65536   /* tests/scale.sh: 12,000 globals filled the old 4,096 */

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
int unevaluated;         /* nesting of non-evaluated sizeof operands */
int hasinit;

char symname[MAXSYM * NAMEW];
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
int sympk[MAXSYM * 8];         /* append-only pool of complete parameter kinds */
int nsympk;
int sympkfirst[MAXSYM];        /* this declaration's first kind in the pool */
int symnpk[MAXSYM];            /* how many; -1: no prototype seen */
int symretw[MAXSYM];        /* a function's int return width: 1 2 4, or 0 for 8/other */
int symfpret[MAXSYM];        /* calling it yields a function pointer */
int symrfst[MAXSYM];         /* ...whose call returns a pointer to this struct */
int symcst[MAXSYM];          /* a pointer variable: its call returns this struct's pointer */         /* a VLA: the frame slot holding its byte count */
int symstruct[MAXSYM];      /* index into the struct table, or -1 */

/* ---- struct and union ------------------------------------------------
   Members carry a real C layout -- an `int` member is 4 bytes at a 4-byte
   boundary -- even though a local scalar still lives in an 8-byte slot.
   The two are different questions, and only the struct has to answer the
   first one. */
/* 512, not 128: miniz alone defines 11 structs, but the tables below are
   shared across translation units and are NOT deduplicated -- the same
   header included by each unit registers its tags again.  Measured on the
   fb12-23 fixture: 128 refuses, 129 is the first refusal, and the real
   program needs more than that once its units are counted.  [R13-0c #33] */
#define MAXSTRUCT 512
/* MAXMEMB is the same kind of shared table, and it is the one miniz hits
   after the other two are raised: it is the TOTAL across every struct, not
   per-struct (that is the separate `nown >= 256` check).  Measured: 500
   members pass, 1000 refuse.  miniz reaches it because its members are
   registered once per unit as well.  [R13-0c #33] */
#define MAXMEMB 4096
char stname[MAXSTRUCT * NAMEW];
int stfirst[MAXSTRUCT]; int stcount[MAXSTRUCT];
/* tags are block-scoped (C99 6.2.1): the block depth a tag was defined at,
   and whether that block has closed */
int stdepth[MAXSTRUCT]; int stdead[MAXSTRUCT]; int bdepth;
int vlaslot[64];          /* per block depth: where the pre-VLA stack pointer is kept */
int curvla;               /* the thing in r0 is a VLA: its byte-count slot */
int stsize[MAXSTRUCT]; int stalign[MAXSTRUCT]; int stunion[MAXSTRUCT];
int stopen[MAXSTRUCT];      /* being defined: the tag's type is incomplete */
int nstruct;
char mbname[MAXMEMB * NAMEW];
int mboff[MAXMEMB];     /* byte offset inside the struct */
int mbbytes[MAXMEMB];   /* what `sizeof` reports for the member */
int mbwidth[MAXMEMB];   /* the load/store width: 0 means "aggregate" */
int mbarr[MAXMEMB];     /* an array member, including a one-element struct array */
int mbdim2[MAXMEMB];    /* `T m[n][k]`: elements per row (k, or k*j for [n][k][j]); 0 for a 1-D member */
int mbdim3[MAXMEMB];    /* `T m[n][k][j]`: j; 0 otherwise */
int mbelem[MAXMEMB];    /* element size, for [] on an array member */
int mbptr[MAXMEMB];
int mbbase[MAXMEMB];    /* final pointee width, separate from array element width */
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
/* 1024, not 256, for the same reason as MAXSTRUCT above: the typedef table is
   shared across units and not deduplicated.  Measured: 256 refuses, 257 is the
   first refusal; the fb12-23 fixture needs 276 (91+92+93) across three units,
   which is why 256 was just barely not enough.  [R13-0c #33] */
#define MAXTD 1024
char tdname[MAXTD * NAMEW];
int tdw[MAXTD]; int tdsz[MAXTD]; int tdstruct[MAXTD]; int tdptr[MAXTD];
int tduns[MAXTD];
int tdbool[MAXTD];
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
/* The width a call's value comes back as, when the callee is not a symbol we
   can look up: `((double (*)(void)) vp)()` knows its return type only from
   the cast.  vcst already carries a struct-pointer result and vcfn "returns a
   function pointer"; vcflt is the same channel for a float.  Without it the
   call result was an i64 and `printf("%g", ((double (*)(void))vp)())` printed
   the BITS of 7.0 (4.61957e+18). */
int vcflt;
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
long cexpr(void);
int cond(void);
int mbfind(int si, int t);
int eload(int w);
int estore(int w);
int is_typeat(int i);
void declsave(void);
void declrestore(void);
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
/* what a plain `unisacc FILE.c` builds: this machine, Windows included */
#ifdef _WIN32
#ifdef __aarch64__
#define DEFAULT_TARGET "win/arm64"
#else
#define DEFAULT_TARGET "win/x86_64"
#endif
#else
#define DEFAULT_TARGET HOST_TARGET
#endif

int bk_build(char *t, int n, char *target);
int bk_object(char *t, int n, char *target);      /* -c -b lnx/ARCH: relocatable ELF (docs/toolchain.md) */
int bkfd = 1;    /* where the image goes: -o, else stdout.  DEFINED here,
                    once: two definitions of one global become two .bss
                    lines, and the two back ends disagree about which of the
                    two addresses the name means. */
long bk_run(char *t, int n, long argc, long argv);   /* -run [S-9] */
char *runargv[4096];   /* argv, NULL, envp, NULL */
int symlea(int i, int t, char *reg);
int dkind(int flt);
int valuekind(int width, int ptr, int uns, int flt, int bl);
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
int decode(int t, char *buf, int capacity);

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

/* The oracle's answers come from DENSE [J1]: every answer of every stage,
   computed when the kernel is emitted BY RUNNING the constructed net over
   the stage's full domain -- the net in compiled form, not the gold.  An
   ask is a bounds check (the P-2 domain assertion) and one load.  It used
   to be a memo in front of infer(): a hash, six compares, and on a miss or
   a collision a full evaluation.  `--check-oracle` walks every question and
   compares this table with infer() evaluating the weights directly, so the
   table and the net are proved equal on each build.  nask still counts
   every ask ([A-33] stages.sh and `-c -v` read it). */
int inf(int st, int *key, int head) {
    int f; int m; int x; int v;
    nask[st] = nask[st] + 1;
    m = STAGE_M[st];
    if (head < 0 || head >= STAGE_NH[st]) {
        __write(2, "oracle: key outside the stage's domain\n", 39); __exit(1);
    }
    x = 0; f = 0;
    while (f < m) {                       /* fields past m are not the key's */
        v = STAGE_VN[(st << 2) + f];
        if (key[f] < 0 || key[f] >= v) { __write(2, "oracle: key outside the stage's domain\n", 39); __exit(1); }
        x = x * v + key[f];
        f = f + 1;
    }
    return DENSE[STAGE_DOFF[st] + x * STAGE_NH[st] + head] & 255;
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
        while (h < 16) {
            if (STAGE_NCLS[s * 16 + h] > 0) {   /* HEADS_MAX, as the kernel indexes */
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
    if (srcin(IRRECV + p, L, "div")) ec(46);
    if (srcin(IRRECV + p, L, "mod")) ec(46);
    if (srcin(IRRECV + p, L, "udiv")) ec(46);
    if (srcin(IRRECV + p, L, "umod")) ec(46);
    k = 0;
    while (k < L) { ec(IRRECV[p + k] & 255); k = k + 1; }
    return 0;
}

/* a message and an optional name, on stderr where cc puts them */
int emsg(char *m, char *p) {
    int k; k = 0;
    while (m[k]) k = k + 1;
    __write(2, m, k);
    if (p) { k = 0; while (p[k]) k = k + 1; __write(2, p, k); }
    __write(2, "\n", 1);
    return 1;
}
/* a file we cannot read: named, on stderr, as cc says it */
int enoinput(char *p) {
    int k; k = 0;
    while (p[k]) k = k + 1;
    __write(2, "unisacc: error: cannot open ", 28);
    __write(2, p, k);
    __write(2, "\n", 1);
    return 1;
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

/* a whole instruction line goes to es() in ONE piece: a `_rev` recipe
   finds its operands by the commas of the same string */
char el_b[96]; int el_n;
int el_s(char *t) { int k; k = 0; while (t[k]) { el_b[el_n] = t[k]; el_n = el_n + 1; k = k + 1; } return 0; }
int el_num(long v) {
    char b[24]; int n; unsigned long u;
    u = v; n = 0;
    if (v < 0) { el_s("-"); u = 0 - u; }
    if (u == 0) { b[0] = 48; n = 1; }
    while (u > 0) { b[n] = 48 + (u % 10); u = u / 10; n = n + 1; }
    while (n > 0) { n = n - 1; el_b[el_n] = b[n]; el_n = el_n + 1; }
    return 0;
}
int el_out(void) { el_b[el_n] = 10; el_b[el_n + 1] = 0; el_n = 0; return es(el_b); }
/* one immediate into rR: `  @lit.imm rR, V` */
int eimm(int r, long v) { el_s("  @lit.imm r"); el_num(r); el_s(", "); el_num(v); return el_out(); }
/* rD = the frame address FP - off, through scratch rT */
int eframe(int d, int t, long off) {
    eimm(t, off);
    el_s("  @alu.sub r"); el_num(d); el_s(", r6, r"); el_num(t); return el_out();
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

/* C labels have function scope, independent of the ordinary identifier namespace. */
int userlabel(int t) {
    int k;
    es("u_"); etok(fntok); ec(46);
    k = 0; while (k < tlen[t]) { ec(src[tpos[t] + k] & 255); k = k + 1; }
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
   reused.  A name of NAMEW or more characters is refused outright by
   name_toolong(), so nothing truncated ever reaches the hash: those go on
   one chain of their own (SH_LONG) that every lookup merges in. */
#define SH_SIZE 65536
#define SH_LONG SH_SIZE
int sh_head[SH_SIZE + 1];                 /* index + 1, 0: empty */
int sh_link[MAXSYM]; int sh_b[MAXSYM]; int sh_hi;
int sh_bucket(int t) {
    if (tlen[t] >= NAMEW) return SH_LONG;
    return vhash(src + tpos[t], tlen[t]) & (SH_SIZE - 1);
}
int sh_push(int i, int t) {
    int ob; int b; int j;
    if (i < sh_hi) {                      /* reusing a dead slot */
        /* Every slot in [i, sh_hi) is dead (scopes pop by lowering nsym), and
           each still hangs at the front of ITS bucket's chain.  Unlinking only
           slot i's old bucket left the others in place, and one of them could
           end up BEHIND a live entry: a function's `nm` at slot 964, the next
           function's `nm` at 961 (chain 961 -> 964 -> BH_ABI_ARG2), then slot
           964 reused for `k` in another bucket -- 964's link now belongs to
           that bucket, and the next reuse of 961 pops along it and drops
           BH_ABI_ARG2 from its chain: "unknown identifier" 17,000 lines later
           for a global declared at the top (the compiler compiling itself,
           2026-09-30).  So unlink every dead slot from its own bucket before
           the reuse; each slot is cleaned once per death, so the total cost
           stays linear in the pushes. */
        j = sh_hi - 1;
        while (j >= i) {
            ob = sh_b[j];
            while (sh_head[ob] - 1 >= i) sh_head[ob] = sh_link[sh_head[ob] - 1];
            j = j - 1;
        }
    }
    sh_hi = i + 1;
    b = sh_bucket(t);
    sh_b[i] = b; sh_link[i] = sh_head[b]; sh_head[b] = i + 1;
    return 0;
}
int sfind_is(int i, int t) {
    int k; int ok; int n;
    n = tlen[t];
    k = 0; ok = 1;
    while (symname[i * NAMEW + k]) {
        if (k >= n) ok = 0;
        if (ok) { if (symname[i * NAMEW + k] != src[tpos[t] + k]) ok = 0; }
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
    name_toolong(t);                 /* never route a long name to the walk */
    if (tlen[t] >= NAMEW) return sfind_walk(t);
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
char ustat[MAXUSTAT * NAMEW]; int ustat_u[MAXUSTAT]; int nustat;
int ustat_add(int t) {
    int k;
    name_toolong(t);
    /* Unit 0's statics are registered too, even though etok() gives them no
       suffix -- unit 0 is the one whose names keep their spelling, so the
       suffix is not what marks a name as a static.  The reachability pass in
       undef_calls needs "is this label one of THIS unit's statics?" and a bare
       label is exactly the case it got wrong: an unused `static inline
       wrapper` in unit 0 looked externally visible, so its call to a never
       defined function counted as a real reference.  [R13-0b #31] */
    if (nustat >= MAXUSTAT) return 0;
    k = 0;
    while (k < tlen[t] && k < NAMEW - 1) { ustat[nustat * NAMEW + k] = src[tpos[t] + k]; k = k + 1; }
    ustat[nustat * NAMEW + k] = 0;
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
            while (k < L) { if (symname[i * NAMEW + k] != nm[k]) ok = 0; k = k + 1; }
            if (symname[i * NAMEW + L] != 0) ok = 0;
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
            while (ustat[i * NAMEW + k]) {
                if (k >= tlen[t]) { ok = 0; break; }
                if (ustat[i * NAMEW + k] != src[tpos[t] + k]) { ok = 0; break; }
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
/* C99 6.4.2.1 requires 63 significant characters in an identifier; NAMEW is
   64 so a 63-character name fits with its NUL.  A longer one is refused here,
   with a position, rather than truncated: truncating merged distinct names
   into one symbol -- silently, and only for names sharing a prefix.  Called
   from every place a name enters one of the five stores and from sfind, so a
   long name can never reach the "a constant is required here" path, which
   said nothing about the real problem.  [R13-0c #32] */
int sadd(int t, int kind, int off, int elem) {
    int k;
    name_toolong(t);
    if (nsym >= MAXSYM) { __write(2, "symbol table full\n", 18); __exit(1); }
    k = 0;
    while (k < tlen[t]) { if (k < NAMEW - 1) symname[nsym * NAMEW + k] = src[tpos[t] + k]; k = k + 1; }
    if (tlen[t] < NAMEW) symname[nsym * NAMEW + tlen[t]] = 0;
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
    symretw[nsym] = 0;
    symfpret[nsym] = 0; symrfst[nsym] = 0 - 1; symcst[nsym] = 0 - 1;
    symflt[nsym] = declflt; symnpk[nsym] = 0 - 1;
    sympkfirst[nsym] = nsympk;
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
        eimm(1, 64 - bfofs(w) - bfwidth(w));
        es("  @alu.shl r0, r0, r1\n  @lit.imm r1, "); en(64 - bfwidth(w));
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

int snarrow(int w) {
    if (w >= 8) return 0;
    es("  @call.frame 8\n  @mem.st [r7+0], r0, "); en(w);
    es("\n  @mem.ld r0, [r7+0], "); en(w);
    es("\n  @call.frame -8\n");
    return 0;
}

int estore(int w) {                              /* [r1] = r0 */
    if (w >= BFTAG) {
        /* Store the field and return its converted value, including for
           assignment and prefix/compound expressions. */
        long mask;
        mask = -1;
        if (bfwidth(w) < 64) mask = (unsigned long)mask >> (64 - bfwidth(w));
        eimm(2, mask); es("  @alu.and r3, r0, r2\n");
        eimm(2, bfofs(w)); es("  @alu.shl r3, r3, r2\n");
        if (bfunit(w) == 8) es("  @mem.load r4, [r1+0]\n");
        else { es("  @mem.ld r4, [r1+0], "); en(bfunit(w)); ec(10); }
        eimm(2, ~((unsigned long)mask << bfofs(w)));
        es("  @alu.and r4, r4, r2\n  @alu.or r4, r4, r3\n");
        if (bfunit(w) == 8) es("  @mem.store [r1+0], r4\n");
        else { es("  @mem.st [r1+0], r4, "); en(bfunit(w)); ec(10); }
        eimm(2, 64 - bfwidth(w));
        es("  @alu.shl r0, r0, r2\n");
        if (bfsig(w)) es("  @alu.shr r0, r0, r2\n");
        else es("  @alu.lshr r0, r0, r2\n");
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
    int pd; int base; int uns; int flt; int bl; int slotst;
    pd = declpd; if (declptr && pd == 0) pd = 1;
    base = w; uns = declunsigned; flt = declflt; bl = declbool;
    slotst = sst;
    el = declsz;
    if (sst >= 0) el = stsize[sst];
    if (pd) { el = 8; slotst = 0 - 1; }
    if (isarr) {
        if (n == 0) {
            n = initcountat(tp);
            if (slotst >= 0) n = (n + structslots(slotst) - 1) / structslots(slotst);
        }
        size = n * el;
    } else size = el;
    w = el;
    initflt = valuekind(el, pd, uns, flt, bl);
    if (infunc) {
        off = alloc_local(size);
        initisarr = isarr;
        initaggr(0, 0, off, w, slotst, size);
        eframe(0, 0, off);
    } else {
        /* named after the `{` token, not a counter: expr() parses
           speculatively and rewinds, so this can run twice for one literal
           and must name the same object both times */
        id = tp;
        save = toinit; toinit = 0;
        es(".bss __cl"); en(id); ec(32); en(size); ec(10);
        toinit = save;
        initisarr = isarr;
        initaggr(2, id, 0, w, slotst, size);
        es("  @mem.lea r0, __cl"); en(id); ec(10);
    }
    /* Initializer expressions may change every current/declaration kind.
       Reconstruct this object's descriptor from its saved declared type. */
    setkind(0); curpd = pd; curbase = base; curuns = uns; curflt = flt; curbool = bl;
    curstruct = sst; cursize = size; curdim2 = 0; curdim3 = 0; curvla = 0;
    curptr = pd > 0; curelem = base;
    if (pd >= 2) curelem = 8;
    if (pd == 1 && sst >= 0) curelem = stsize[sst];
    lvalue = 1;
    if (isarr) { lvalue = 0; curptr = 1; curpd = pd + 1; curelem = el; }
    else { if (pd == 0 && sst >= 0) curelem = 0; }
    return postfix();
}

int unary(void) {
    int p;
    curfn = 0; curfnst = 0 - 1;
    p = ask(2);
    if (p == P_NEG) { adv(); unary(); loadval();
        if (curflt) { if (curptr == 0) {        /* -x flips the sign bit, -0.0 too */
            eimm(1, curflt == 8 ? (long)1 << 63 : 2147483648); es("  @alu.xor r0, r0, r1\n");
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
        es("  @lit.imm r1, 0\n  @alu.eq r0, r0, r1\n");
        /* C99 6.5.3.3p5: !x is an int whatever x was -- `a + !q` must not
           scale a by q's element */
        setkind(0); curuns = 0; cursize = 4; curelem = 4; return 0; }
    if (p == P_DEREF) {
        adv(); unary(); loadval();
        if (curfn) { lvalue = 0; return 0; }     /* *fp is fp */
        if (curdim2 > 0) {                       /* *a of a[n][m] is an array row */
            cursize = curelem * curdim2;
            curdim2 = curdim3; curdim3 = 0;
            curptr = 1; curpd = 0; lvalue = 0;
            return 0;
        }
        lvalue = 1;
        if (curpd >= 2) {                        /* *q of int **q is an int * */
            curpd = curpd - 1; curptr = 1; cursize = 8;
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
                 if (curstruct >= 0) { cursize = stsize[curstruct]; curelem = 0; } }
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
        if (curptr == 0) { if (curbool) fconv(0, 9);
            else { if (e < BFTAG) { if (curuns) zext(e); } } }
        pop1();
        estore(e);
        return 0;
    }
    if (p == P_SIZEOF) {
        int sz; int nsave; int isave;
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
                if (declstruct >= 0 && sz == 0) err_tok(tp, "sizeof incomplete structure");
                es("  @lit.imm r0, "); en(sz); ec(10);
                setkind(1); curpd = 0; curbase = 8; curvla = 0; /* size_t: unsigned 64-bit */
                return 0;
            }
        }
        /* sizeof EXPR: walk it for its type and throw the code away -- the
           operand of sizeof is not evaluated [C99 6.5.3.4p2]. */
        nsave = nout; isave = nibuf;
        unevaluated = unevaluated + 1;
        cursize = 8;                       /* an expression with no symbol */
        curvla = 0;
        unary();
        unevaluated = unevaluated - 1;
        nout = nsave; nibuf = isave;
        if (toinit && curvla) err_tok(tp, "static initializer requires a constant size");
        sz = cursize;
        if (curstruct >= 0 && curptr == 0 && stsize[curstruct] == 0) err_tok(tp, "sizeof incomplete structure");
        if (curvla) { es("  @mem.load r0, [r6-"); en(curvla); es("]\n"); curvla = 0; }
        else { es("  @lit.imm r0, "); en(sz); ec(10); }
        setkind(1); curpd = 0; curbase = 8; curvla = 0;
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
            int cw; int csz; int cuns; int cst; int carr; int cn; int cflt; int ok; int cpd; int cb; int cptr;
            adv();
            cw = declspec(); csz = declsz; cuns = declunsigned; cst = declstruct; cflt = declflt; cb = declbool;
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
            cptr = declptr;
            unary(); loadval();
            declptr = cptr;
            /* C99 6.3.1.4-5 at a cast: to or from a floating type */
            ok = fkind();
            if (declptr == 0) {
                if (cb) fconv(ok, 9);
                else { if (cflt) fconv(ok, cflt);
                else { if (ok >= 4) fconv(ok, (cuns && csz == 8) ? 1 : 0); } }
            }
            /* narrowing is observable: `(char)300` is 44.  The tape has
               sized load/store, so a round trip through a stack slot is the
               whole of it -- and it sign-extends on the way back. */
            if (declptr == 0) { if (cflt == 0) {
                if (csz < 8 && cb == 0) {
                    if (cuns) {
                        /* unsigned: keep the low bits, zero the rest --
                           `(unsigned char)255 >> 4` is 15, not -1 */
                        if (csz == 1) es("  @lit.imm r2, 255\n  @alu.and r0, r0, r2\n");
                        if (csz == 2) es("  @lit.imm r2, 65535\n  @alu.and r0, r0, r2\n");
                        if (csz == 4) es("  @lit.imm r2, 4294967295\n  @alu.and r0, r0, r2\n");
                    } else {
                        snarrow(csz);
                    }
                }
            } }
            lvalue = 0; curptr = declptr; cursize = csz; curuns = cuns; curflt = cflt; curbool = cb;
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
            /* The callee's return type is known RIGHT HERE and nowhere else
               for a cast: `((double (*)(void)) vp)()` -- the cast set
               curflt, and vcall's result block would reset it to 0, so the
               double came back as its bit pattern.  Remember it on the same
               channel vcst/vcfn use; a plain `f()` still gets its type from
               the symbol via callres(). */
            /* curptr is 1 here -- the callee is a POINTER to a function, and
               that is exactly the cast case; requiring curptr == 0 threw the
               information away.  What must not leak is a result that is
               itself a pointer or a struct (a float never is). */
            if (curfn == 0 && curstruct < 0) vcflt = curflt;
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
            if (curbool && curptr == 0) {
                /* Normalization loses the arithmetic inverse: save old x. */
                push();
                es("  @lit.imm r1, 1\n");
                if (op == tidx("++", 2)) es("  @alu.add r0, r0, r1\n");
                else es("  @alu.sub r0, r0, r1\n");
                fconv(0, 9);
                es("  @mem.load r1, [r7+8]\n"); estore(e2);
                es("  @mem.load r0, [r7+0]\n  @call.frame -16\n");
                continue;
            }
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
            /* A bit-field can wrap on store.  Keep the value read before the
               update: reversing the converted new value is not its old value
               (for example, an unsigned 5-bit 0 followed by postfix --). */
            if (e2 >= BFTAG) push();                 /* saved old value */
            push();                                  /* operand for emit_binop */
            es("  @lit.imm r0, "); en(step); ec(10);
            if (op == tidx("++", 2)) emit_binop(tidx("+", 1));
            else emit_binop(tidx("-", 1));
            if (e2 >= BFTAG) {
                es("  @mem.load r1, [r7+8]\n");
                estore(e2);
                es("  @mem.load r0, [r7+0]\n  @call.frame -16\n");
                curelem = e2;
                continue;
            }
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
                eimm(2, mboff[mi]);
                es("  @alu.add r0, r0, r2\n");
            }
            lvalue = 1;
            curelem = mbwidth[mi];
            curptr = mbptr[mi];
            if (curptr) curelem = mbelem[mi];
            curpd = 0; curbase = mbelem[mi];
            if (curptr) { curpd = mbptrd[mi]; if (curpd >= 2) curelem = 8; }
            cursize = mbbytes[mi]; curdim2 = mbdim2[mi]; curdim3 = mbdim3[mi];
            curstruct = mbstruct[mi];
            /* `p->q->b`: a pointer member hands its pointee on */
            if (mbptr[mi]) { curstruct = mbpst[mi]; if (curstruct >= 0) curelem = stsize[curstruct]; }
            curuns = mbuns[mi]; curbool = mbbool[mi];
            curflt = mbflt[mi];
            if (mbarr[mi]) {
                /* T *a[N] decays to T **, without loading its first slot. */
                lvalue = 0; curptr = 1; curpd = mbptrd[mi] + 1;
                curelem = mbelem[mi]; curbase = mbbase[mi];
            } else { if (mbwidth[mi] == 0) { if (mbptr[mi] == 0) {
                /* an array or a nested struct: the value IS the address */
                if (mbstruct[mi] < 0 || mbarr[mi]) {
                    lvalue = 0; curptr = 1; curpd = 1;
                    curelem = mbelem[mi]; curbase = curelem;
                }
            } } }
        } else {
        if (p == P_INDEX) {
            int row; int uu; int ist; int ifl; int ipd; int ibase; int d3;
            curvla = 0;
            ipd = curpd; ibase = curbase;
            adv();
            ist = curstruct;                 /* the index expression resets it */
            ifl = curflt;
            e = curelem;
            row = curdim2;
            d3 = curdim3;                    /* saved: the index expression resets it */
            uu = curuns;                     /* the ELEMENT's, not the index's */
            if (row > 0) e = e * row;        /* the first index of `a[n][m]` */
            loadval();
            push();
            expr(); loadval();
            if (e != 1) { eimm(2, e); es("  @alu.mul r0, r0, r2\n"); }
            pop1();
            es("  @alu.add r0, r1, r0\n");
            need(vfind(TOKV, NTOKV, "]", 1), "]");
            cursize = e;
            curuns = uu;
            if (row > 0) {
                /* a row is itself an array: its VALUE is its address */
                curelem = e / row; curptr = 1; lvalue = 0;
                /* a[i] of a[n][m][k] is itself an [m][k]: its rows are k long */
                curdim2 = d3; curdim3 = 0;
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
    vcflt = symflt[si];                   /* the symbol's own return type wins */
    return vcall(symfp[si] == 2);
}

/* The callee's address is in r0 and the cursor is on the `(`.  `var` says
   the pointee is variadic, which puts every argument on the tape stack. */
int vcall(int var) {
    int n; int k; int st; int rp;
    push();
    need(tidx("(", 1), "(");
    n = 0;
    while (cur() != tidx(")", 1)) {
        if (cur() == T_EOF) break;
        expr(); loadval(); argconv(0 - 1, n); push(); n = n + 1;
        if (eat(tidx(",", 1)) == 0) break;
    }
    rp = tp;                     /* the `)`: where the product places the six-argument diagnostic */
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
        k = n - 1;
        while (k >= 0) { es("  @mem.load r"); en(k); es(", [r7+0]\n  @call.frame -8\n"); k = k - 1; }
        if (n == 6) {
            /* All six argument registers are occupied.  The callee remains
               in the next tape-stack slot; lowering chooses an ISA scratch. */
            es("  callm r7, 0\n  @call.frame -8\n");
        } else es("  @mem.load r5, [r7+0]\n  @call.frame -8\n  @call.callr r5\n");
    }
    lvalue = 0; curelem = 8; curptr = 0; curuns = 0; cursize = 8; curstruct = 0 - 1;
    curfn = 0; curfnst = 0 - 1; curflt = 0;
    if (vcflt) { curflt = vcflt; cursize = vcflt; curelem = vcflt; curuns = 0; }
    vcflt = 0;
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
        /* a constant's own kind: these were left from the previous operand -- a unit's first
           expression saw curstruct 0, "a struct", and 1 + 10u lost its unsignedness */
        curuns = 0; curflt = 0; curstruct = 0 - 1;
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
        /* Character contents are not integer suffixes: 'U' is an int. */
        if ((src[tpos[tp]] & 255) != 39) {
            k = 0;
            while (k < tlen[tp]) {
                if ((src[tpos[tp] + k] & 255) == 117) curuns = 1;   /* u */
                if ((src[tpos[tp] + k] & 255) == 85) curuns = 1;    /* U */
                k = k + 1;
            }
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
        /* emit_pool closes every literal with one NUL byte, so the last byte of
           the four-byte terminator is left to it: the object is exactly
           (wn + 1) * 4 bytes, as the network writes it (R14-8 b_strsizeof) */
        i = addlit(lbuf, (wn + 1) * 4 - 1);
        es("  @mem.lea r0, S"); en(i); ec(10);
        setkind(0); cursize = (wn + 1) * 4; curpd = 1; curbase = 4;
        lvalue = 0; curelem = 4; curptr = 1;
        return postfix();
    } }
    /* C99 6.4.2.2: inside a function, `__func__` is declared as if by
       `static const char __func__[] = "name";`.  It is not an identifier
       to look up -- there is no such symbol -- so it is answered here,
       with the name of the function being walked. */
    if (t == T_ID) { if (srcis(tpos[tp], tlen[tp], "__func__")) {
        int fl;
        fl = 0;
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
        setkind(0); cursize = fl + 1; curpd = 1; curbase = 1;
        lvalue = 0; curelem = 1; curptr = 1;
        return postfix();
    } }
    if (t == T_STR) {
        int sl;
        i = nlab; nlab = nlab + 1;
        sl = decode(adv(), lbuf, sizeof(lbuf)); i = addlit(lbuf, sl);
        es("  @mem.lea r0, S"); en(i); ec(10);
        /* a char *: the kind is set whole, so `*"z"` loads its first char --
           a stale curpd/curstruct from before once left it the address */
        setkind(0); cursize = sl + 1; curuns = 0; curpd = 1; curbase = 1;
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
        if (toinit && unevaluated == 0) {
            int ai; ai = sfind(tp);
            if (ai >= 0) { if (symkind[ai] == 1 || symkind[ai] == 3)
                err_tok(tp, "static initializer refers to an automatic object"); }
        }
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
        else { eframe(0, 2, symoff[i]); }
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
#define MAXPOOL 4194304
char pool[MAXPOOL];
#define MAXLIT 16384
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
int decode(int t, char *buf, int capacity) {
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
            if (c >= 48 && c <= 55) {       /* one to three octal digits */
                int ov; int od;
                ov = 0; od = 0;
                while (od < 3 && k < tlen[t] - 1) {
                    c = src[tpos[t] + k] & 255;
                    if (c < 48 || c > 55) break;
                    ov = ov * 8 + c - 48;
                    k = k + 1; od = od + 1;
                }
                k = k - 1; c = ov & 255;
            } else if (c == 120) {          /* all hex digits, within this literal */
                int hv; int hd; int hn;
                hv = 0; hn = 0; k = k + 1;
                while (k < tlen[t] - 1) {
                    hd = src[tpos[t] + k] & 255;
                    if (hd >= 48 && hd <= 57) hd = hd - 48;
                    else if (hd >= 65 && hd <= 70) hd = hd - 55;
                    else if (hd >= 97 && hd <= 102) hd = hd - 87;
                    else break;
                    hv = (hv * 16 + hd) & 255;
                    hn = hn + 1; k = k + 1;
                }
                if (hn == 0) { err_tok(t, "hex escape requires a digit"); return 0; }
                k = k - 1; c = hv;
            } else if (c == 110) c = 10;
            else if (c == 116) c = 9;
            else if (c == 114) c = 13;
            else if (c == 97) c = 7;
            else if (c == 98) c = 8;
            else if (c == 102) c = 12;
            else if (c == 118) c = 11;
        }
        if (n >= capacity) { err_tok(t, "decoded string exceeds buffer capacity"); return 0; }
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

/* Read only the format. The argument has already been parsed once by the
   real call path; warning checks must not allocate labels, locals or types. */
int pf_argcheck(int ft, int which, int at) {
    char fb[4096]; int n; int k; int c; int lng; int arg;
    if (warnall == 0) return 0;
    n = decode(ft, fb, sizeof(fb));
    if (panic || which == 0) return 0;
    k = 0; arg = 0;
    while (k < n) {
        c = fb[k] & 255;
        if (c != 37) { k = k + 1; continue; }
        k = k + 1;
        while (k < n) { c = fb[k] & 255; if (c == 45 || c == 43 || c == 32 || c == 35 || c == 48) { k = k + 1; continue; } break; }
        if (k < n) { if (fb[k] == 42) { k = k + 1; arg = arg + 1; if (arg == which) return 0; } }
        while (k < n) { if (isdi(fb[k] & 255) == 0) break; k = k + 1; }
        if (k < n) { if (fb[k] == 46) { k = k + 1;
            if (k < n) { if (fb[k] == 42) { k = k + 1; arg = arg + 1; if (arg == which) return 0; } }
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
        arg = arg + 1;
        if (arg == which) return pf_check(c, lng, at);
    }
    return 0;
}

int do_printf(void) {
    int lng;
    int t; int k; int n; int c; int m; int id; int pass; int j; int at; int slot[32];
    char fbuf[4096];
    need(vfind(TOKV, NTOKV, "(", 1), "(");
    if (cur() != T_STR) { printf("printf needs a literal format\n"); __exit(1); }
    t = adv();
    n = decode(t, fbuf, sizeof(fbuf));
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
                at = tp;
                if (warnall) curcall = 0;
                expr(); loadval();
                if (warnall) pf_check(c, lng, at);
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
    if (si >= 0) { if (symkind[si] == 2) { if (symnpk[si] > k) {
        fconv(fkind(), sympk[sympkfirst[si] + k]);
        return 0;
    } } }
    if (curflt == 4) { if (curptr == 0) fconv(4, 8); }
    return 0;
}
/* what a call of si hands back: a float or double, when it returns one */
/* The result's pointer-ness is the FUNCTION's, not whatever the last
   argument left behind: `-(f(rows[i]) - 1)` scaled the 1 as if f returned
   the row pointer its argument was, and `*(ip() + 2)` stepped by bytes.
   A call of an unknown name, or of a function returning a scalar, is not a
   pointer; one returning T* steps by T, T** by a pointer. [J10] */
int callptr(int si) {
    curptr = 0; curpd = 0;
    if (si < 0) return 0;
    if (symkind[si] != 2) return 0;
    if (symptr[si] == 0) return 0;
    curptr = 1; curpd = symptrd[si]; curbase = symbase[si];
    curelem = curpd >= 2 ? 8 : symbase[si];
    return 0;
}
int callres(int si) {
    /* signedness from the callee's return type, not the last argument
       evaluated: `long g(unsigned long)`'s g(v)/2 divides signed */
    curflt = 0; curcall = 1; curuns = 0; cursize = 8;
    if (si >= 0) { if (symkind[si] == 2) { if (symptr[si] == 0) curuns = symuns[si]; } }
    if (si >= 0) { if (symkind[si] == 2) { if (symptr[si] == 0) { if (symflt[si]) { curuns = 0;
        curflt = symflt[si]; cursize = curflt; curelem = curflt;
    } } } }
    if (si >= 0) { if (symkind[si] == 2) { if (symretw[si]) { cursize = symretw[si]; curelem = cursize; } } }
    return 0;
}

int pf_call(int t) {
    int n; int k; int psi; int fmt; int at;
    fmt = 0 - 1;
    /* a call's value is an i64 on the type axis until return types are
       tracked; the fields describe the RESULT, not whatever came before */
    cursize = 8; curuns = 0; curstruct = 0 - 1; curdim2 = 0; curdim3 = 0;
    curcall = 1;                          /* whichever way the call is made */
    if (isname(t, "printf", 6)) {
        int ps; int useit;
        useit = 0;
        ps = sfind(t);
        if (ps >= 0) { if (symvar[ps]) {
            useit = 1;   /* printf is an ordinary call whenever <stdio.h>'s is there */
        } }
        if (ps >= 0) { if (symvar[ps]) { if (kind(tp + 1) != T_STR) useit = 1; } }
        if (warnall && kind(tp + 1) == T_STR) {
            fmt = tp + 1; pf_argcheck(fmt, 0, fmt);
            if (panic) return 0;
        }
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
    if (isname(t, "__hostcall", 10)) {
        need(tidx("(", 1), "("); n = 0;
        while (cur() != tidx(")", 1)) {
            if (cur() == T_EOF) break;
            expr(); loadval(); push(); n = n + 1;
            if (n > 2) { err_tok(tp, "hostcall requires two arguments"); return 0; }
            if (eat(tidx(",", 1)) == 0) break;
        }
        need(tidx(")", 1), ")");
        if (n != 2) { err_tok(tp, "hostcall requires two arguments"); return 0; }
        sysargs(n); es("  .hostcall r0, r1\n");
        setkind(0); cursize = 8; curelem = 8; lvalue = 0;
        return postfix();
    }
    if (isname(t, "__hostaddr0", 11) || isname(t, "__hostaddr1", 11) ||
        isname(t, "__hostaddr2", 11) || isname(t, "__hostaddr3", 11)) {
        need(tidx("(", 1), "("); need(tidx(")", 1), ")");
        es("  .hostaddr r0, "); en(src[tpos[t] + 10] - 48); es("\n");
        setkind(0); cursize = 8; curelem = 8; lvalue = 0;
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
    if (isname(t, "__getdents64", 12) || isname(t, "__open", 6) || isname(t, "__read", 6) ||
        isname(t, "__write", 7) || isname(t, "__close", 7) ||
        isname(t, "__mprotect", 10) || isname(t, "__munmap", 8) ||
        isname(t, "__exit", 6) || isname(t, "__lseek", 7) ||
        isname(t, "__unlink", 8) || isname(t, "__rename", 8)) {
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
        if (isname(t, "__getdents64", 12)) es("getdents64");
        else { if (isname(t, "__open", 6)) es("open");
        else { if (isname(t, "__read", 6)) es("read");
        else { if (isname(t, "__write", 7)) es("write");
        else { if (isname(t, "__close", 7)) es("close");
        else { if (isname(t, "__mprotect", 10)) es("mprotect");
        else { if (isname(t, "__munmap", 8)) es("munmap");
        else { if (isname(t, "__lseek", 7)) es("lseek");
        else { if (isname(t, "__unlink", 8)) es("unlink");
        else { if (isname(t, "__rename", 8)) es("rename");
        else es("exit"); } } } } } } } } }
        es(", r0, r1, r2\n");
        lvalue = 0; curelem = 8; curptr = 0;
        return postfix();
    }
    if (isname(t, "va_start", 8)) {          /* ap = &arg[nfixed] */
        need(tidx("(", 1), "(");
        unary(); lvalue = 0; push();
        if (eat(tidx(",", 1))) { expr(); loadval(); }
        need(tidx(")", 1), ")");
        eimm(0, 16 + 8 * fnnfixed); es("  @alu.add r0, r6, r0\n");
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
        at = tp;
        if (fmt >= 0 && n > 0) curcall = 0;
        expr(); loadval();
        if (fmt >= 0 && n > 0) pf_argcheck(fmt, n, at);
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
            callres(si); callptr(si);
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
    callres(sfind(t)); callptr(sfind(t));
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

int cprec(int k);
/* the walker's ladder starts at `|` (prec 3): || and && sit above it */
int binop_level(int k) { int c; c = cprec(k); return c >= 3 ? c - 3 : 0 - 1; }

int bs_done[64]; int bs_val[64];
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
    /* which alu flavour: the `binsel` table, keyed (operator, signedness),
       asked once per pair [I3] -- this was a 23-branch chain */
    {   int key[4]; int i; int c; char b[48]; char *f; int n; int j;
        i = vfind(BF_BINSEL_0, NBF_BINSEL_0, TOKV + voff(TOKV, k), vlen(TOKV, k));
        if (i < 0) i = vfind(BF_BINSEL_0, NBF_BINSEL_0, ">>", 2);
        if (bs_done[i * 2 + (binuns ? 1 : 0)] == 0) {
            key[0] = i; key[1] = binuns ? 1 : 0; key[2] = 0; key[3] = 0;
            bs_val[i * 2 + (binuns ? 1 : 0)] = inf(S_BINSEL, key, 0);
            bs_done[i * 2 + (binuns ? 1 : 0)] = 1;
        }
        c = bs_val[i * 2 + (binuns ? 1 : 0)];
        f = BH_BINSEL_Y + voff(BH_BINSEL_Y, c); n = vlen(BH_BINSEL_Y, c);
        j = 0; while ("  @alu."[j]) { b[j] = "  @alu."[j]; j = j + 1; }
        while (n > 0 && j < 30) { b[j] = *f; f = f + 1; j = j + 1; n = n - 1; }
        n = 0; while (" r0, r1, r0\n"[n]) { b[j] = " r0, r1, r0\n"[n]; j = j + 1; n = n + 1; }
        b[j] = 0;
        es(b);
    }
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
    /* For negative e, M*10^e < 2^(bits(M)+3e). The exponent alone
       cannot prove underflow: a long significand may cancel it. */
    if (e10 < 0) {
        if (fb_bits(fbA, fbAn) + 3 * e10 <= emin - mbits - 1) return 0;
    }
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
    lvalue = 0; curptr = 0; curstruct = 0 - 1; curdim2 = 0; curdim3 = 0; curbool = 0;
    if (k == 9) { curbool = 1; curflt = 0; cursize = 1; curelem = 1; curuns = 1; return 0; }
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
    /* Binary &: same arithmetic result types as |. The table's i64 & i64
       row is the address-of convention, not this binary operation. */
    if (k == tidx("&", 1)) { buf[0] = 124; return 1; }
    buf[0] = TOKV[p];
    if (L > 1) buf[1] = TOKV[p + 1];
    return L;
}

int binary(int level) {
    int k; int e; int lp; int lax; int rax; int ck; int res; int cl;
    int lf; int rf; int lk; int lfr; int rfr; int re; int rp; int lpd; int rpd; int lbase; int rbase;
    int lsst; int rsst;           /* the struct each side points at, if any */
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
        /* The struct a pointer points AT: `p + 2` is still a `struct V *`,
           so `(p + 2)->val` must find the member.  It was dropped here --
           the pointer branch below restored curelem/curptr/curpd/curbase and
           left curstruct at -1, so `->` reported "member access on something
           that is not a struct" for every pointer that had been through
           arithmetic (`p[2].val`, which takes the subscript path, worked).
           Lua's s2v()/setobj macros are written as `&(L->top.p + idx)->val`. */
        lsst = curstruct;
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
        rsst = curstruct;
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
            eimm(2, re); es("  @alu.mul r0, r0, r2\n  @mem.store [r7+8], r0\n");
            es("  @mem.load r0, [r7+0]\n  @call.frame -8\n");
        } } } }
        if (lp) { if (e > 1) {
            if (k == tidx("+", 1)) { eimm(2, e); es("  @alu.mul r0, r0, r2\n"); }
            if (k == tidx("-", 1)) { if (curptr == 0) {
                eimm(2, e); es("  @alu.mul r0, r0, r2\n"); } }
        } }
        emit_binop(k);
        /* C99 6.5.6p9: the difference of two pointers counts ELEMENTS */
        if (lp) { if (curptr) { if (e > 1) { if (k == tidx("-", 1)) {
            eimm(2, e); es("  @alu.div r0, r0, r2\n");
        } } } }
        if (tyis(res, "ptr", 3) == 0) { if (tyuns(res)) zext(tysize(res)); }
        binuns = 0; binwid = 8;
        lvalue = 0; curstruct = 0 - 1; curdim2 = 0; curdim3 = 0;
        if (tyis(res, "ptr", 3)) { curelem = e; curptr = lp; curuns = 0; cursize = 8;
                                   curflt = lfr; curpd = lpd; curbase = lbase;
                                   if (lp) curstruct = lsst;
                                   if (lp == 0) { curptr = 1; curelem = re; curflt = rfr;
                                                  curpd = rpd; curbase = rbase;
                                                  curstruct = rsst; } }
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
        elab("  @ctrl.jumpz r0, __unisacc_L", end); ec(10);
        binary(0); loadval(); ftruthy();
        es("  @lit.imm r1, 0\n  @alu.ne r0, r0, r1\n");
        elab("__unisacc_L", end); es(":\n");
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
        elab("  @ctrl.jumpz r0, __unisacc_L", rhs); ec(10);
        es("  @lit.imm r0, 1\n");
        elab("  @ctrl.jump __unisacc_L", end); ec(10);
        elab("__unisacc_L", rhs); es(":\n");
        land(); loadval(); ftruthy();
        es("  @lit.imm r1, 0\n  @alu.ne r0, r0, r1\n");
        elab("__unisacc_L", end); es(":\n");
        setkind(0); cursize = 4; curelem = 4; curptr = 0;   /* an int (6.5.14p3) */
    }
    return 0;
}

/* `a ? b : c` -- only one arm is evaluated, so each gets its own label and
   the value they share is r0. */
int cond(void) {
    int els; int end; int p1; int e1; int ci;
    lor();
    if (cur() != tidx("?", 1)) return 0;
    loadval(); ftruthy(); adv();
    els = newlab(); end = newlab();
    elab("  @ctrl.jumpz r0, __unisacc_L", els); ec(10);
    {   int save; int nsave; int k1; int k2; int a1; int cf;
        save = tp; nsave = nout;
        /* The middle operand is an `expression`, not an `assignment-expression`
           (C99 6.5.15), so an unparenthesised comma belongs to it:
           `c ? f(), 7 : 0`.  It was parsed at the assignment level, which
           stopped at the comma and then demanded a `:`.  stb_image_write.h's
           stb_sb_free() expands to exactly that shape. */
        exprc(); loadval(); k1 = fkind(); a1 = tyax(); p1 = curptr; e1 = curelem;
        elab("  @ctrl.jump __unisacc_L", end); ec(10);
        elab("__unisacc_L", els); es(":\n");
        need(tidx(":", 1), ":");
        cond(); loadval(); k2 = fkind();
        ci = 0 - 1;
        if (p1 == 0 && curptr == 0 && k1 < 4 && k2 < 4) {
            int ct; ct = tyask(a1, "+", 1, tyax());
            if (!tyis(ct, "illegal", 7)) ci = ct;
        }
        if ((k1 >= 4 || k2 >= 4) && k1 != k2) {
            /* C99 6.5.15p5: the arms meet in their common type.  The first
               was emitted before the second's type was known: again. */
            cf = tyis(tyask(a1, "+", 1, tyax()), "f64", 3) ? 8 : 4;
            tp = save; nout = nsave;
            exprc(); loadval(); fconv(fkind(), cf);
            elab("  @ctrl.jump __unisacc_L", end); ec(10);
            elab("__unisacc_L", els); es(":\n");
            need(tidx(":", 1), ":");
            cond(); loadval(); fconv(fkind(), cf);
            setkind(cf);
        }
    }
    elab("__unisacc_L", end); es(":\n");
    if (ci >= 0) {
        /* Both integer arms meet in the type table's arithmetic row.
           Convert the selected value once, after the branch merge. */
        setkind(0); curuns = tyuns(ci); cursize = tysize(ci); curelem = cursize;
        if (curuns && cursize < 8) zext(cursize);
    }
    /* C99 6.5.15p6: one arm a pointer and the other a null pointer
       constant -- the result is the pointer's type */
    if (p1) { if (curptr == 0) { curptr = 1; curelem = e1; } }
    if (curptr) cursize = 8;       /* array arms decay in ?: */
    lvalue = 0;
    return 0;
}

int exprc(void) {                   /* the comma operator */
    expr();
    while (cur() == tidx(",", 1)) { adv(); expr(); if (curptr) cursize = 8; }
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
            int ptrl; int pel; int ak; int aax; int ck2;
            adv();
            ptrl = curptr; pel = curelem;
            ak = fkind(); aax = tyax(); bl = curbool && curptr == 0; ck2 = aax;
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
                fconv(cf, bl ? 9 : ak);
                pop1();                              /* address */
                estore(e);
                setkind(bl ? 9 : ak);
                return 0;
            }
            /* `p += n` moves n ELEMENTS, as `p = p + n` does */
            if (ptrl) { if (pel > 1) {
                if (op == tidx("+", 1) || op == tidx("-", 1)) {
                    eimm(2, pel); es("  @alu.mul r0, r0, r2\n");
                }
            } }
            /* `x op= y` is done in the common type (6.5.16.2p3): unsigned
               when the table says so, or `h >>= 1` on a u64 shifted in
               the sign; then the value is x's, masked to x's width */
            if (ptrl == 0) {
                char cb[8]; int cl;
                ck2 = tyask(aax, "+", 1, tyax());
                binuns = tyuns(ck2); binwid = tysize(ck2);
                cl = tycanon(op, cb);
                ck2 = tyask(aax, cb, cl, tyax());
            }
            emit_binop(op);                          /* pops old value */
            binuns = 0; binwid = 8;
            if (bl) fconv(0, 9);
            else { if (ptrl == 0) { if (e < BFTAG) {
                if (tyuns(aax)) zext(e);
                else { if (ck2 != aax) snarrow(e); }
            } } }
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
            bl = curbool && curptr == 0; /* the TARGET's type, before the RHS */
            if (bl) ak = 9;
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
long swval[256]; int swlab[256]; int nswv;
int swdef[16]; int swslot[16]; int swsize[16]; int swuns[16]; int nsw;

int patchnum(int at, int w, int v) {
    int k; int d;
    k = w - 1;
    if (v == 0) { out[at + k] = 48; k = k - 1; }
    while (v > 0) { if (k >= 0) out[at + k] = 48 + (v % 10); v = v / 10; k = k - 1; }
    while (k >= 0) { out[at + k] = 32; k = k - 1; }
    return 0;
}

long cexpr(void);

long catom(void) {
    long v; int k; int i;
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
/* a binary operator's binding strength, 1 (||) to 10 (* / %), 0 for
   anything else: the `prec` table's answer, asked once per token kind [I1].
   It was a ladder of 18 comparisons here, and a second table beside it. */
#define PR_MAX 128
int pr_done[PR_MAX]; int pr_val[PR_MAX];
int cprec(int k) {
    int key[4]; int i; int c; char *y;
    if (k < 0 || k >= PR_MAX || k >= NTOKV) return 0;
    if (pr_done[k]) return pr_val[k];
    i = vfind(BF_PREC_0, NBF_PREC_0, TOKV + voff(TOKV, k), vlen(TOKV, k));
    if (i < 0) i = vfind(BF_PREC_0, NBF_PREC_0, "other", 5);
    key[0] = i; key[1] = 0; key[2] = 0; key[3] = 0;
    c = inf(S_PREC, key, 0);
    y = BH_PREC_Y + voff(BH_PREC_Y, c);
    pr_val[k] = 0;
    if (y[0] != 110) {                     /* "none" */
        pr_val[k] = y[0] - 48;
        if (y[1]) pr_val[k] = pr_val[k] * 10 + y[1] - 48;
    }
    pr_done[k] = 1;
    return pr_val[k];
}
long cunary(void) {
    int w;
    if (eat(tidx("-", 1))) return 0 - cunary();
    if (eat(tidx("+", 1))) return cunary();
    if (eat(tidx("!", 1))) { if (cunary()) return 0; return 1; }
    if (eat(tidx("~", 1))) return (0 - cunary()) - 1;
    if (cur() == tidx("sizeof", 6)) {
        /* `sizeof (TYPE)`: the operand's type is spelled out */
        if (kind(tp + 1) == tidx("(", 1)) { if (is_typeat(tp + 2)) {
            int w2; int ds;
            adv(); adv();
            declsave();
            w = declspec(); w = declsz;
            while (eatstar()) w = 8;
            need(tidx(")", 1), ")");
            ds = declstruct; w2 = w;
            declrestore();
            if (ds >= 0 && w2 == 0) err_tok(tp, "sizeof incomplete structure");
            return w2;
        } }
        /* `sizeof (expr)` and `sizeof expr`: the operand is NOT evaluated
           (C99 6.5.3.4p2), so a constant expression only needs its SIZE --
           and for a string literal that is the length plus the NUL it ends
           with (C99 6.4.5p6).  Only the (TYPE) form was modelled, so
           `char d[sizeof("abc")]` was "a constant is required here" while
           `char a[sizeof(void *)]` -- the same operator, a type instead of an
           expression -- worked.  Lua writes sizeof on both. */
        adv();
        if (cur() == tidx("(", 1)) {
            adv();
            if (kind(tp) == T_STR) {
                int sl; char lb[4096];
                sl = decode(adv(), lb, sizeof(lb));
                need(tidx(")", 1), ")");
                return sl + 1;
            }
            w = cexpr(); need(tidx(")", 1), ")");
            return w;
        }
        return 8;                       /* `sizeof x`: a scalar in this subset */
    }
    /* a cast inside a constant expression: `(int)7` */
    if (cur() == tidx("(", 1)) { if (is_typeat(tp + 1)) {
        adv();
        declsave();
        declspec(); while (eatstar()) { }
        need(tidx(")", 1), ")");
        declrestore();
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

long cexpr(void) {
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

/* Shared trailing dimensions for automatic, static and global arrays. */
int dimtail(int n) {
    decldim2 = 0; decldim3 = 0;
    if (cur() == tidx("[", 1)) {
        adv(); decldim2 = cexpr(); need(tidx("]", 1), "]");
        n = n * decldim2;
        dim3decl(); n = n * dim3n;
    }
    return n;
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
            if (k > NAMEW - 2) ok = 0;
            else { if (tdname[i * NAMEW + k] != src[tpos[t] + k]) ok = 0; }
            k = k + 1;
        }
        if (ok) { if (tdname[i * NAMEW + tlen[t]] == 0) return i; }
        i = i - 1;
    }
    return 0 - 1;
}

int tdadd(int t, int w, int sz, int si, int isptr) {
    name_toolong(t);
    int k;
    if (ntd >= MAXTD) { __write(2, "too many typedefs\n", 18); __exit(1); }
    k = 0;
    while (k < tlen[t]) { if (k < NAMEW - 1) tdname[ntd * NAMEW + k] = src[tpos[t] + k]; k = k + 1; }
    if (k > NAMEW - 1) k = NAMEW - 1;
    tdname[ntd * NAMEW + k] = 0;
    tdw[ntd] = w; tdsz[ntd] = sz; tdstruct[ntd] = si; tdptr[ntd] = isptr;
    tduns[ntd] = declunsigned; tdbool[ntd] = declbool;
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
            if (k > NAMEW - 2) ok = 0;
            else { if (stname[i * NAMEW + k] != src[tpos[t] + k]) ok = 0; }
            k = k + 1;
        }
        if (ok) { if (stname[i * NAMEW + tlen[t]] == 0) return i; }
        i = i - 1;
    }
    return 0 - 1;
}

int stnew(int t, int isunion) {
    int k;
    name_toolong(t);
    if (nstruct >= MAXSTRUCT) { __write(2, "too many structs\n", 17); __exit(1); }
    k = 0;
    if (t >= 0) {
        while (k < tlen[t]) { if (k < NAMEW - 1) stname[nstruct * NAMEW + k] = src[tpos[t] + k]; k = k + 1; }
    }
    if (k > NAMEW - 1) k = NAMEW - 1;
    stname[nstruct * NAMEW + k] = 0;
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
        /* A definition or tag-only declaration binds in this block. */
        else { if (kind(tp + 1) == tidx("{", 1) || kind(tp + 1) == tidx(";", 1)) { if (stdepth[si] < bdepth) si = stnew(t, isunion); } }
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
            if (k > NAMEW - 2) ok = 0;
            else { if (mbname[i * NAMEW + k] != src[tpos[t] + k]) ok = 0; }
            k = k + 1;
        }
        if (ok) { if (mbname[i * NAMEW + tlen[t]] == 0) return i; }
        i = i + 1;
    }
    return 0 - 1;
}

/* A constant expression can contain a CAST or a sizeof(TYPE), and both call
   declspec() -- which overwrites the whole decl* family.  Inside an array
   bound that family is the OUTER declaration's: `char b[(int)(2 * sizeof(
   double))]` left declsz 8 (double) where the declarator, and the symbol it
   registers, still meant char's 1.  Snapshot and restore rather than hand-
   copying the fields at each call site: the list must be every decl*
   declspec assigns, or the next one added silently reintroduces this. */
static int declsv[13];
void declsave(void) {
    declsv[0] = declbase;      declsv[1] = declbool;
    declsv[2] = declenum;      declsv[3] = declflt;
    declsv[4] = declspecfp;    declsv[5] = declspecfpst;
    declsv[6] = declspecpd;    declsv[7] = declspecptr;
    declsv[8] = declstatic;    declsv[9] = declstruct;
    declsv[10] = declsz;       declsv[11] = declunsigned;
    declsv[12] = declpd;       /* eatstar() increments it, and it is what
                                  makes the array a pointer array */
}
void declrestore(void) {
    declbase = declsv[0];      declbool = declsv[1];
    declenum = declsv[2];      declflt = declsv[3];
    declspecfp = declsv[4];    declspecfpst = declsv[5];
    declspecpd = declsv[6];    declspecptr = declsv[7];
    declstatic = declsv[8];    declstruct = declsv[9];
    declsz = declsv[10];       declunsigned = declsv[11];
    declpd = declsv[12];
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
        declunsigned = tduns[td]; declbool = tdbool[td];
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
    int mdim2; int mdim3;                 /* the trailing dimensions of `name[n][k][j]` */
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
                while (k < NAMEW) { mbname[nmemb * NAMEW + k] = mbname[a * NAMEW + k]; k = k + 1; }
                mboff[nmemb] = mo + mboff[a]; mbbytes[nmemb] = mbbytes[a];
                mbwidth[nmemb] = mbwidth[a]; mbelem[nmemb] = mbelem[a]; mbarr[nmemb] = mbarr[a];
                mbptr[nmemb] = mbptr[a]; mbstruct[nmemb] = mbstruct[a];
                mbbase[nmemb] = mbbase[a];
                mbuns[nmemb] = mbuns[a];
                mbskip[nmemb] = mbskip[a];
                mbpst[nmemb] = mbpst[a];
                mbdim2[nmemb] = mbdim2[a]; mbdim3[nmemb] = mbdim3[a];
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
            n = 1; marr = 0; mdim2 = 0; mdim3 = 0;
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
                /* `T m[n][k]` (and [n][k][j]): the member is n rows of k
                   elements, and `s.m[i]` is a row whose size is k elements,
                   as for a local a[n][k] (dimtail).  Without this the second
                   `[` was "expected ;" and every 2-D member table was
                   rewritten by hand (miniz, R13-0b#23). */
                if (cur() == tidx("[", 1)) {
                    adv(); mdim2 = cexpr(); need(tidx("]", 1), "]");
                    n = n * mdim2;
                    if (cur() == tidx("[", 1)) {
                        adv(); mdim3 = cexpr(); need(tidx("]", 1), "]");
                        mdim2 = mdim2 * mdim3; n = n * mdim3;
                        if (cur() == tidx("[", 1)) { printf("arrays of more than three dimensions are not supported\n"); __exit(1); }
                    }
                }
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
                    /* Ordinary members already reset bitpos to their byte end.
                       Keep the exact cursor between adjacent bit-fields. */
                    pos = bitpos;
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
            while (k < tlen[t]) { if (k < NAMEW - 1) mbname[nmemb * NAMEW + k] = src[tpos[t] + k]; k = k + 1; }
            if (k > NAMEW - 1) k = NAMEW - 1;
            mbname[nmemb * NAMEW + k] = 0;
            mboff[nmemb] = mo; mbbytes[nmemb] = msz; mbwidth[nmemb] = mw;
            mbelem[nmemb] = mel; mbptr[nmemb] = declptr; mbarr[nmemb] = marr;
            mbdim2[nmemb] = mdim2; mbdim3[nmemb] = mdim3;
            mbbase[nmemb] = sz; if (mst >= 0) mbbase[nmemb] = stsize[mst];
            mbstruct[nmemb] = 0 - 1;
            if (declptr == 0) { if (isbf == 0) mbstruct[nmemb] = mst; }
            mbpst[nmemb] = 0 - 1;
            if (declptr) mbpst[nmemb] = mst;
            mbflt[nmemb] = mflt;
            mbptrd[nmemb] = declptr ? (declpd > 0 ? declpd : 1) : 0;
            mbuns[nmemb] = muns; mbbool[nmemb] = mbl;
            mbskip[nmemb] = 0;
            if (stunion[si]) { if (stcount[si] > 0) mbskip[nmemb] = 1; }
            name_toolong(t);          /* the member's name goes in mbname */
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
        while (k < NAMEW) { mbname[nmemb * NAMEW + k] = mbname[own[j] * NAMEW + k]; k = k + 1; }
        mboff[nmemb] = mboff[own[j]]; mbbytes[nmemb] = mbbytes[own[j]];
        mbwidth[nmemb] = mbwidth[own[j]];
        mbarr[nmemb] = mbarr[own[j]]; mbelem[nmemb] = mbelem[own[j]];
        mbptr[nmemb] = mbptr[own[j]]; mbstruct[nmemb] = mbstruct[own[j]];
        mbbase[nmemb] = mbbase[own[j]];
        mbuns[nmemb] = mbuns[own[j]];
        mbbool[nmemb] = mbbool[own[j]];
        mbskip[nmemb] = mbskip[own[j]];
        mbpst[nmemb] = mbpst[own[j]];
        mbflt[nmemb] = mbflt[own[j]];
        mbdim2[nmemb] = mbdim2[own[j]]; mbdim3[nmemb] = mbdim3[own[j]];
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
            while (symname[k * NAMEW + r] && r < 24) { wm[q] = symname[k * NAMEW + r]; q = q + 1; r = r + 1; }
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
        if (cur() == tidx("(", 1)) {          /* a FUNCTION typedef */
            /* `typedef int binop(int, int);` -- the typedef names a function
               TYPE, not a pointer to one, but the declarator that uses it is
               nearly always `binop *f`, and calling `f(a, b)` has to go
               through a pointer.  tdfp marks the typedef as callable; it was
               left at whatever the previous entry held, so `binop *f; f(4,5)`
               was reported as `undefined function 'f'` (and `expected ';'`
               in the product).  stb_image_write.h writes
               `typedef void stbi_write_func(void *, void *, int);`. */
            while (cur() != tidx(";", 1)) { if (cur() == T_EOF) break; adv(); }
            tdadd(nt, 8, 8, 0 - 1, 1);
            tdfp[ntd - 1] = 1;
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
                slotw = mbelem[mi];
                if (slotw == 0) slotw = 8;
                slotflt = valuekind(slotw, mbptr[mi], mbuns[mi], mbflt[mi], mbbool[mi]);
                return 0;
            }
            cnt = cnt + sub;
        } else {
            if (k == cnt) {
                slotoff = el * stsize[sst] + mboff[mi];
                slotw = mbwidth[mi];
                slotflt = valuekind(slotw, mbptr[mi], mbuns[mi], mbflt[mi], mbbool[mi]);
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
        eframe(1, 2, off - delta);
    }
    return 0;
}

/* How many elements the initialiser supplies, for an unsized `[]`.  The
   cursor is on the `]`. */
/* A single ordinary string may have one brace pair and a trailing comma. */
int bracedstr(int j) {
    int end;
    if (kind(j) != tidx("{", 1)) return 0;
    if (kind(j + 1) != T_STR || iswide(j + 1)) return 0;
    end = j + 2;
    if (kind(end) == tidx(",", 1)) end = end + 1;
    if (kind(end) != tidx("}", 1)) return 0;
    return j + 1;
}
int initcount(int chars) {
    int j; int depth; int n; int k; int c; int pos;
    char buf[4096];
    j = tp;
    while (j < ntok) { if (kind(j) == tidx("=", 1)) break; j = j + 1; }
    j = j + 1;
    if (chars && kind(tp + 1) == tidx("=", 1)) {
        k = bracedstr(j);
        if (k) return decode(k, buf, sizeof(buf)) + 1;
    }
    if (iswide(j)) return wdecode(j, wcp) + 1;
    if (kind(j) == T_STR) { return decode(j, buf, sizeof(buf)) + 1; }
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

int initstr(int isglobal, int gt, int off, int cap, int delta) {
    int t; int n; int k;
    char buf[4096];
    if (iswide(tp)) {
        /* four bytes an element; cap counts ELEMENTS */
        t = adv();
        n = wdecode(t, wcp);
        if (cap > 0) {
            initaddr(isglobal, gt, off, delta);
            es("  @mem.zero r1, 0, "); en(cap * 4); ec(10);
        }
        k = 0;
        while (k < n) {
            if (k >= cap) break;
            es("  @lit.imm r0, "); en(wcp[k]); ec(10);
            initaddr(isglobal, gt, off, delta + k * 4);
            estore(4);
            k = k + 1;
        }
        return 0;
    }
    t = adv();
    n = decode(t, buf, sizeof(buf));
    if (cap > 0) {
        initaddr(isglobal, gt, off, delta);
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
        initaddr(isglobal, gt, off, delta + k);
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
    if (isarr && w == 1 && sst < 0 && rows == 0 && bracedstr(tp)) {
        adv();
        initstr(isglobal, gt, off, nbytes, 0);
        eat(tidx(",", 1));
        need(tidx("}", 1), "}");
        return 0;
    }
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
        /* A string fills one whole row of a two-dimensional character array. */
        if (depth == 1 && isarr && w == 1 && sst < 0 && rows > 0 && rows3 == 0) {
            int braced; int text;
            braced = bracedstr(tp);
            text = cur() == T_STR && iswide(tp) == 0;
            if (braced || text) {
                if (i < 0 || rows > nbytes || i > nbytes - rows)
                    err_tok(tp, "string row exceeds array bounds");
                if (i % rows) err_tok(tp, "string initializer is not at a row boundary");
                if (braced) adv();
                initstr(isglobal, gt, off, rows, i);
                if (braced) { eat(tidx(",", 1)); need(tidx("}", 1), "}"); }
                i = i + rows;
                continue;
            }
        }
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
/* `int (twice)(int x);` -- a parenthesised declarator NAME.  The parentheses
   do not make it a pointer; they exist to stop a function-like macro from
   expanding the name, and every declaration in lua.h uses the form
   (`LUA_API lua_State *(lua_newstate)(lua_Alloc, void *);`).  Only `(*name)`
   was accepted, so the plain parenthesised name was skipped entirely and the
   function stayed undeclared ("undefined function 'twice'"). */
int parenname(void) {
    int t;
    t = 0 - 1;
    adv();                                   /* the `(` */
    if (cur() == T_ID) t = adv();
    need(tidx(")", 1), ")");
    return t;
}
int fpdecl(void) {
    int t; int var; int unsized;
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
    fpdim = 0; fpadim = 0; fpfn = 0 - 1; fpretfp = 0; unsized = 0;
    /* `(*pick(int which))(int, int)`: pick takes (int which) and RETURNS
       the pointer -- the declarator nests, and the inner list is pick's */
    if (cur() == tidx("(", 1)) { fpfn = tp; skipparen(); }
    while (cur() == tidx("[", 1)) {             /* an array of pointers */
        adv(); fpdim = 1;
        if (cur() != tidx("]", 1)) fpdim = cexpr();
        else unsized = 1;
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
    /* The initializer follows the whole declarator, not the inner [].
       Do not scan into the next declaration when no initializer is present. */
    if (unsized) { if (cur() == tidx("=", 1)) fpdim = initcountat(tp + 1); }
    /* `(*const x)` is just a parenthesised pointer: declfp stays 0 */
    return t;
}

int lfp; int lfpret; int lflt0; int gflt0;
/* the conversion kind of the object being declared */
/* Conversion kind of a scalar slot, including elements and members. */
int valuekind(int width, int ptr, int uns, int flt, int bl) {
    if (ptr) return 1;
    if (bl) return 9;
    if (flt) return flt;
    if (uns) { if (width == 8) return 1; }
    return 0;
}
int dkind(int flt) {
    return valuekind(declsz, declptr, declunsigned, flt, declbool);
}
/* Parse one parameter without binding a slot or emitting any instruction.
   Definitions and block prototypes use the same declarator rules. */
int paramtok; int paramnamed; int paramstruct; int paramflt;
int parameter_decl(void) {
    int pw;
    paramtok = 0 - 1;
    pw = declspec();
    paramstruct = declstruct; paramflt = declflt;
    declptr = declspecptr; declpd = declspecpd; declfp = declspecfp;
    while (eatstar()) declptr = 1;
    paramnamed = 0;
    if (cur() == tidx("(", 1)) {
        if (kind(tp + 1) == tidx("*", 1)) {
            paramtok = fpdecl(); declptr = 1; pw = 8;
            if (paramtok >= 0) paramnamed = 1;
            if (fpdim > 0) declfp = 0;       /* an array of them decays */
        } else {
            /* `int f1(int (), int)`: a function type, adjusted to a
               pointer to it (C99 6.7.5.3p8) */
            skipparen(); declptr = 1; pw = 8;
        }
    }
    if (cur() == T_ID) { paramtok = adv(); paramnamed = 1; }
    /* `int a[n]`, `int a[static 5]`: an array parameter IS a pointer
       (C99 6.7.5.3p7), whatever the brackets say */
    while (cur() == tidx("[", 1)) {
        while (cur() != tidx("]", 1)) { if (cur() == T_EOF) break; adv(); }
        adv(); declptr = 1;
    }
    return pw;
}

/* Pool entries outlive local symbol slots, which block exit can reuse. */
int paramkind_add(int pk) {
    if (nsympk >= MAXSYM * 8) { __write(2, "parameter kind pool full\n", 25); __exit(1); }
    sympk[nsympk] = pk; nsympk = nsympk + 1;
    return 0;
}

/* Keep the return declaration in the symbol before parsing parameters.
   The caller restores its declaration base before a comma continuation. */
int block_prototype(int t, int w) {
    int si; int np; int var; int pw;
    declbytes = declptr ? 8 : declsz;
    si = sadd(t, 2, 0, w);
    sympkfirst[si] = nsympk;
    need(tidx("(", 1), "(");
    np = 0; var = 0;
    if (isname(tp, "void", 4) && kind(tp + 1) == tidx(")", 1)) adv();
    while (cur() != tidx(")", 1) && cur() != T_EOF) {
        if (eat(tidx("...", 3))) { var = 1; break; }
        pw = parameter_decl();
        paramkind_add(dkind(paramflt));
        np = np + 1;
        if (eat(tidx(",", 1)) == 0) break;
    }
    need(tidx(")", 1), ")");
    symnpk[si] = np;
    symvar[si] = var || np > 6;
    return 0;
}

int local_decl(void) {
    int w; int t; int off; int n; int nelem; int sst; int isarr; int apd; int lstat; int fpn; int lbool;
    int psave[9];
    if (cur() == tidx("typedef", 7)) return do_typedef();
    w = declspec();
    sst = declstruct;
    lflt0 = declflt; lbool = declbool; /* initializers may contain type names */
    lstat = declstatic;
    if (cur() == tidx(";", 1)) { adv(); return 0; }  /* `struct X { ... };` */
    while (1) {
        declstruct = sst;
        decldim2 = 0; decldim3 = 0; declfp = declspecfp;
        declptr = declspecptr; declpd = declspecpd;
        declflt = lflt0; declbool = lbool; fpn = 0;
        while (eatstar()) { declptr = 1; }
        if (cur() == tidx("(", 1)) { if (kind(tp + 1) == T_ID) {
            /* `int (twice)(int x);` and `int (v) = 5;` -- a parenthesised
               name, not a pointer declarator. */
            t = parenname();
            if (t >= 0) { lfpret = 0; sst = 0 - 1; declstruct = 0 - 1; }
            if (cur() == tidx("(", 1)) { skipparen(); declfp = 1; declptr = 1; }
        } else if (kind(tp + 1) == tidx("*", 1)) {
            t = fpdecl(); declptr = 1; lfpret = fpretfp; sst = 0 - 1; declstruct = 0 - 1;
            if (fpdim > 0) {
                fpn = fpdim; declpd = 1; declfp = 0;
            }
            if (fpn > 0 && lstat == 0) {
                /* `int (*fs[2])(int, int)`: an automatic array of pointers */
                lbind = scopebind("local", 5, t);
                off = alloc_local(fpdim * 8);
                declbytes = fpdim * 8; declfp = 0;
                sadd(t, lbind, off, 8);
                symkind[nsym - 1] = 3; symptrd[nsym - 1] = 2;
                if (eat(tidx("=", 1))) { initisarr = 1; initaggr(0, 0, off, 8, 0 - 1, fpdim * 8); }
                if (eat(tidx(",", 1))) continue;
                break;
            }
            if (fpadim > 0) { decldim2 = fpadim; declptr = 1; }
        } else t = adv(); }
        else t = adv();
        /* A block prototype declares a callable signature, not an object. */
        if (cur() == tidx("(", 1)) {
            psave[0] = declspecptr;
            psave[1] = declspecpd;
            psave[2] = declspecfp;
            psave[3] = declspecfpst;
            psave[4] = declbase;
            psave[5] = declsz;
            psave[6] = declunsigned;
            psave[7] = declenum;
            psave[8] = declvoid;
            block_prototype(t, w);
            declspecptr = psave[0];
            declspecpd = psave[1];
            declspecfp = psave[2];
            declspecfpst = psave[3];
            declbase = psave[4];
            declsz = psave[5];
            declunsigned = psave[6];
            declenum = psave[7];
            declvoid = psave[8];
            if (eat(tidx(",", 1))) continue;
            need(tidx(";", 1), ";");
            return 0;
        }
        lbind = scopebind("local", 5, t);
        n = 1; isarr = 0;
        if (fpn > 0) { n = fpn; isarr = 1; }
        lfp = declfp;
        if (lstat) {
            int ew; long nb; int lk2; int slabel;
            /* Token ordinals restart in every unit. There are at most 64
               inputs and MAXTOK tokens each, so these ranges are disjoint
               and fit an int; unit zero retains its existing labels. */
            slabel = curunit * MAXTOK + t;
            if (cur() == tidx("[", 1)) {
                adv(); isarr = 1;
                if (cur() == tidx("]", 1)) {
                    n = initcount(w == 1 && declpd == 0 && sst < 0);
                    if (sst >= 0) { int per; per = structslots(sst); n = (n + per - 1) / per; }
                } else n = cexpr();
                need(tidx("]", 1), "]");
                n = dimtail(n);
            }
            ew = w;
            if (sst >= 0) ew = declsz;
            if (declptr) { if (isarr) { if (declpd > 0) ew = 8; } else ew = 8; }
            nb = n * ew;
            if (declptr == 0) { if (sst >= 0) { if (isarr == 0) nb = declsz; } }
            es(".bss ls"); en(slabel); ec(32); en(nb < 8 ? 8 : nb); ec(10);
            declbytes = nb;
            sadd(t, 0, 0, isarr ? ew : w);
            symlab[nsym - 1] = slabel;
            if (isarr) { symkind[nsym - 1] = 5; symptr[nsym - 1] = 1; symptrd[nsym - 1] = declpd + 1; }
            else { if (declptr == 0) { if (sst >= 0) {
                symkind[nsym - 1] = 5; symptr[nsym - 1] = 1; symelem[nsym - 1] = declsz; } } }
            if (eat(tidx("=", 1))) {
                /* once, at program start: into __init, like a global's */
                lk2 = dkind(lflt0);
                initflt = valuekind(ew, declpd, declunsigned, lflt0, declbool);
                toinit = 1; hasinit = 1;
                if (cur() == tidx("{", 1)) {
                    if (isarr) { initisarr = 1; initrows = decldim2; initrows3 = decldim3; }
                    initaggr(3, slabel, 0, isarr ? ew : w, sst, nb);
                } else { if (cur() == T_STR) { if (isarr) { initstr(3, slabel, 0, n, 0); }
                    else { expr(); loadval(); es("  @mem.lea r1, ls"); en(slabel); ec(10); estore(8); } }
                else { int sptr; sptr = declptr;   /* before the RHS */
                    expr(); loadval(); fconv(fkind(), lk2);
                    es("  @mem.lea r1, ls"); en(slabel); ec(10);
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
            int el; int szs; int vpd; int vbase; int vuns; int vbool; int vfp;
            int vd2; int vd3;                /* `v[n][k]`, `v[n][k][j]`: constant trailing dimensions */
            vpd = declpd; vbase = declbase; vuns = declunsigned;
            vbool = declbool; vfp = declfp;
            el = w;
            if (sst >= 0) el = declsz;
            if (declptr) el = 8;
            adv(); expr(); loadval(); need(tidx("]", 1), "]");
            /* C99 6.7.5.2: only the first dimension may vary here; the rows
               are a constant number of elements, so `v[i]` is a row and
               `v[i][j]` an element exactly as for a fixed a[n][k] (decldim2).
               A varying inner dimension is still not covered (R13-0 #05). */
            vd2 = 0; vd3 = 0;
            if (cur() == tidx("[", 1)) {
                adv(); vd2 = cexpr(); need(tidx("]", 1), "]");
                if (cur() == tidx("[", 1)) {
                    adv(); vd3 = cexpr(); need(tidx("]", 1), "]");
                    vd2 = vd2 * vd3;
                    if (cur() == tidx("[", 1)) { printf("arrays of more than three dimensions are not supported\n"); __exit(1); }
                }
            }
            eimm(2, vd2 > 0 ? el * vd2 : el); es("  @alu.mul r0, r0, r2\n");
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
            declptr = 1; declbytes = 8; declpd = vpd + 1; declbase = vbase;
            declstruct = sst; declflt = lflt0; declunsigned = vuns;
            declbool = vbool; declfp = vfp; decldim2 = vd2; decldim3 = vd3;
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
                n = initcount(w == 1 && declpd == 0 && sst < 0);
                if (sst >= 0) {
                    int per; per = structslots(sst);
                    n = (n + per - 1) / per;
                }
            }
            else n = cexpr();
            need(vfind(TOKV, NTOKV, "]", 1), "]");
            /* `a[n][m]` is n*m elements in a row; the FIRST index strides a
               whole row, which is what decldim2 records */
            n = dimtail(n);
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
                        eframe(0, 0, off);
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
            initflt = valuekind(w, declpd, declunsigned, lflt0, declbool);
            if (cur() == tidx("{", 1)) {
                if (isarr) { initisarr = 1; initrows = decldim2; initrows3 = decldim3; }
                initaggr(0, 0, off, w, sst, n * w);
            }
            else { if (cur() == T_STR) { if (isarr) { if (w == strw(tp)) {
                initstr(0, 0, off, n, 0);
            } else { expr(); loadval();
                eframe(1, 2, off);
                estore(8); } }
            else { int rt; rt = tp; curcall = 0; expr(); loadval();
                intptr_check(lptr, rt);
                fconv(fkind(), lk);                  /* C99 6.7.8p11 */
                eframe(1, 2, off);
                if (lptr) estore(8); else estore(w); } }
            else { int rt; rt = tp; curcall = 0; expr(); loadval();
                intptr_check(lptr, rt);
                fconv(fkind(), lk);                  /* C99 6.7.8p11 */
                eframe(1, 2, off);
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
            userlabel(lt); es(":\n");
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
        elab("  @ctrl.jumpz r0, __unisacc_L", a); ec(10);
        stmt();
        c = laststmt;                            /* the then-branch's answer */
        if (cur() == vfind(TOKV, NTOKV, "else", 4)) {
            b = newlab();
            elab("  @ctrl.jump __unisacc_L", b); ec(10);
            elab("__unisacc_L", a); es(":\n");
            adv(); stmt();
            elab("__unisacc_L", b); es(":\n");
            laststmt = c && laststmt;
        } else { elab("__unisacc_L", a); es(":\n"); laststmt = 0; }
        return 0;
    }
    if (p == P_WHILE) {
        adv();
        top = newlab(); a = newlab();
        elab("__unisacc_L", top); es(":\n");
        need(vfind(TOKV, NTOKV, "(", 1), "(");
        expr(); loadval(); ftruthy(); need(vfind(TOKV, NTOKV, ")", 1), ")");
        elab("  @ctrl.jumpz r0, __unisacc_L", a); ec(10);
        brkstack[nloop] = a; cntstack[nloop] = top;
        brkdep[nloop] = bdepth; cntdep[nloop] = bdepth; nloop = nloop + 1;
        stmt();
        nloop = nloop - 1;
        elab("  @ctrl.jump __unisacc_L", top); ec(10);
        elab("__unisacc_L", a); es(":\n");
        return 0;
    }
    if (p == P_FOR) {
        int stept; int bodyt; int aftert;
        int savesym; int saveoff; int savetd; int savest;
        savesym = nsym; saveoff = frameoff; savetd = ntd; savest = nstruct;
        bdepth = bdepth + 1; vlaslot[bdepth] = 0;
        adv(); need(tidx("(", 1), "(");
        if (eat(tidx(";", 1)) == 0) {
            if (is_typeat(tp)) local_decl();
            else { exprc(); need(tidx(";", 1), ";"); }
        }
        top = newlab(); a = newlab(); c = newlab();
        elab("__unisacc_L", top); es(":\n");
        if (cur() != tidx(";", 1)) {
            expr(); loadval(); ftruthy();
            elab("  @ctrl.jumpz r0, __unisacc_L", a); ec(10);
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
        elab("__unisacc_L", c); es(":\n");
        /* not after an error: the cursor is parked at EOF and stays there */
        if (panic == 0) { if (stept != bodyt - 1) { tp = stept; exprc(); } }
        tp = aftert;
        elab("  @ctrl.jump __unisacc_L", top); ec(10);
        elab("__unisacc_L", a); es(":\n");
        if (vlaslot[bdepth]) {
            es("  @mem.load r7, [r6-"); en(vlaslot[bdepth]); es("]\n");
            vlaslot[bdepth] = 0;
        }
        nsym = savesym; frameoff = saveoff; ntd = savetd;
        bdepth = bdepth - 1;
        while (savest < nstruct) { stdead[savest] = 1; savest = savest + 1; }
        return 0;
    }
    if (p == P_SWITCH) {
        int slot; int disp; int end; int k; int cbase; int mysw;
        adv(); need(tidx("(", 1), "(");
        expr(); loadval();
        /* Case constants convert to the promoted controlling type. */
        if (nsw >= 16) { err_tok(tp, "switch nesting exceeds capacity"); return 0; }
        swsize[nsw] = cursize; swuns[nsw] = curuns;
        if (cursize < 4) { swsize[nsw] = 4; swuns[nsw] = 0; }
        need(tidx(")", 1), ")");
        slot = alloc_local(8);
        eframe(1, 2, slot);
        es("  @mem.store [r1+0], r0\n");
        disp = newlab(); end = newlab();
        elab("  @ctrl.jump __unisacc_L", disp); ec(10);
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
        elab("  @ctrl.jump __unisacc_L", end); ec(10);
        elab("__unisacc_L", disp); es(":\n");
        k = cbase;
        while (k < nswv) {
            eframe(1, 2, slot);
            es("  @mem.load r0, [r1+0]\n");
            es("  @lit.imm r1, "); en(swval[k]); ec(10);
            es("  @alu.ne r0, r0, r1\n");
            elab("  @ctrl.jumpz r0, __unisacc_L", swlab[k]); ec(10);
            k = k + 1;
        }
        nswv = cbase;
        if (swdef[mysw] >= 0) { elab("  @ctrl.jump __unisacc_L", swdef[mysw]); ec(10); }
        else { elab("  @ctrl.jump __unisacc_L", end); ec(10); }
        elab("__unisacc_L", end); es(":\n");
        return 0;
    }
    if (p == P_CASE) {
        long v; int lab;
        adv(); v = cexpr(); need(tidx(":", 1), ":");
        if (nsw > 0) { if (swsize[nsw - 1] == 4) {
            if (swuns[nsw - 1]) v = v & 4294967295;
            else v = (int)v;
        } }
        if (nsw == 0) { err_tok(tp, "case outside switch"); return 0; }
        if (nswv >= 256) { err_tok(tp, "switch case capacity exceeded"); return 0; }
        lab = newlab();
        swval[nswv] = v; swlab[nswv] = lab; nswv = nswv + 1;
        elab("__unisacc_L", lab); es(":\n");
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
        elab("__unisacc_L", lab); es(":\n");
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
        elab("  @ctrl.jump __unisacc_R", retlab); ec(10);
        return 0;
    }
    if (p == P_GOTO) {
        int gt;
        adv(); gt = adv();
        es("  @ctrl.jump "); userlabel(gt); ec(10);
        need(tidx(";", 1), ";");
        return 0;
    }
    if (p == P_BREAK) {
        adv(); need(vfind(TOKV, NTOKV, ";", 1), ";");
        vlaback(brkdep[nloop - 1]);
        elab("  @ctrl.jump __unisacc_L", brkstack[nloop - 1]); ec(10);
        return 0;
    }
    if (p == P_CONTINUE) {
        adv(); need(vfind(TOKV, NTOKV, ";", 1), ";");
        vlaback(cntdep[nloop - 1]);
        elab("  @ctrl.jump __unisacc_L", cntstack[nloop - 1]); ec(10);
        return 0;
    }
    if (p == P_DO) {
        adv();
        top = newlab(); a = newlab(); c = newlab();
        elab("__unisacc_L", top); es(":\n");
        brkstack[nloop] = a; cntstack[nloop] = c;
        brkdep[nloop] = bdepth; cntdep[nloop] = bdepth; nloop = nloop + 1;
        stmt();
        nloop = nloop - 1;
        elab("__unisacc_L", c); es(":\n");
        need(tidx("while", 5), "while");
        need(tidx("(", 1), "(");
        expr(); loadval(); ftruthy();
        need(tidx(")", 1), ")");
        need(tidx(";", 1), ";");
        elab("  @ctrl.jumpz r0, __unisacc_L", a); ec(10);
        elab("  @ctrl.jump __unisacc_L", top); ec(10);
        elab("__unisacc_L", a); es(":\n");
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
    if (fsym >= 0) sympkfirst[fsym] = nsympk;
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
        if (c == tidx("...", 3)) { if (depth == 0) fnvar = 1; }
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
        pw = parameter_decl();
        pst = paramstruct; pfl = paramflt;
        pt = paramtok; havename = paramnamed;
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
                eframe(5, 5, off); es("  @mem.store [r5+0], r1\n");
            } else {
                /* Straight into the slot: the tape takes a negative
                   displacement, so no scratch register is needed.  There was
                   one -- r5, "free" -- until a function had six parameters
                   and r5 WAS the sixth, overwritten before it was stored. */
                es("  @mem.store [r6-"); en(off); es("], r"); en(np); ec(10);
            }
        }
        /* the parameter's kind, for callers to convert to */
        if (fsym >= 0) paramkind_add(dkind(pfl));
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
        eframe(0, 0, off);
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
    if (fsym >= 0) { if (symbool[fsym] && symptr[fsym] == 0) retkind = 9; }
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
    if (isname(t, "main", 4)) es("  @lit.imm r0, 0\n");   /* reaching main's } returns 0 (C99 5.1.2.2.3) */
    elab("__unisacc_R", retlab); es(":\n");
    es("  mov r7, r6\n  @mem.load r6, [r7+0]\n  @call.frame -8\n  @ctrl.ret\n");
    patchnum(fpatch, 6, (framemax + 7) / 8 * 8);
    if (retst >= 0) { es(".bss __rv_"); etok(t); ec(32); en(stsize[retst]); ec(10); }
    nsym = scopebase;
    return 0;
}

/* Register every file-scope `static` name of the unit about to be walked.
   Registering at the DEFINITION is too late for a forward reference: `etok`
   spells a name with its unit suffix only when ustat_is() already knows it, so
   `static int vfmt(void) { return digits() + 1; }` called `digits` with no
   suffix while its definition emitted the label `digits__u1`, and the call was
   reported as undefined.  Order-dependent, which is why `m1 m2` passed and
   `m2 m1` failed.  [R13-0b, multi gate]

   `lex()` starts each unit at token 0 (`ntok = 0`), so this always scans
   0..ntok and depth can start at 0.  The scan is deliberately shallow: brace
   depth 0 only, so a `static` inside a function or a struct body is not a
   file-scope name.  `static` and the type words are kind T_TYPE, the NAME is
   T_ID -- testing for T_ID alone skipped the very token we look for.  A
   `static` function POINTER variable is registered too, since etok spells it
   with the suffix as well.  Typedefs are skipped: their name is not storage. */
int ustat_prescan(void) {
    int i; int depth; int j; int t; int seenstatic;
    i = 0; depth = 0; seenstatic = 0;
    while (i < ntok) {
        t = kind(i);
        if (t == tidx("{", 1)) { depth = depth + 1; i = i + 1; continue; }
        if (t == tidx("}", 1)) { depth = depth - 1; i = i + 1; continue; }
        if (t == tidx(";", 1)) { seenstatic = 0; i = i + 1; continue; }
        if (depth == 0) {
            if (t == T_TYPE || t == T_ID) {
                if (tlen[i] == 6) {
                    if (srcis(tpos[i], 6, "static")) { seenstatic = 1; i = i + 1; continue; }
                    if (srcis(tpos[i], 6, "typedef")) { seenstatic = 0; i = i + 1; continue; }
                }
            }
            if (seenstatic) {
                if (t == T_ID) {
                    j = i + 1;
                    if (kind(j) == tidx("(", 1)) { ustat_add(i); seenstatic = 0; }
                    else { if (kind(j) == tidx("*", 1)) { ustat_add(i); seenstatic = 0; } }
                }
            }
        }
        i = i + 1;
    }
    return 0;
}

int unit(void) {
    int p; int w; int t; int n; int k; int isarr; int gstruct; int cpn; int gfpfn; int gk; int gpd; int gfpd; int gbool;
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
        gflt0 = declflt; gbool = declbool;
        if (cur() == tidx(";", 1)) { adv(); continue; }  /* `struct X {...};` */
        while (1) {
            declstruct = gstruct;
            decldim2 = 0; decldim3 = 0; declfp = declspecfp;
            declptr = declspecptr; declpd = declspecpd;
            declflt = gflt0; declbool = gbool;
            while (eatstar()) declptr = 1;
            gfpfn = 0; gfpd = 0;
            if (cur() == tidx("(", 1)) { if (kind(tp + 1) == T_ID) {
                /* `int (twice)(int x);` / `char *(greet)(void);` at file
                   scope -- a parenthesised declarator NAME.  Only `(*name)`
                   was accepted here, so a prototype in this form was skipped
                   and the function stayed undeclared.  lua.h declares every
                   API entry point this way, to keep the name from being
                   expanded as a function-like macro. */
                t = parenname();
                gstruct = 0 - 1; declstruct = 0 - 1; gfpd = 0;
                if (cur() == tidx("(", 1)) { skipparen(); declfp = 1; declptr = 1; gfpd = 1; gfpfn = 1; }
            } else if (kind(tp + 1) == tidx("*", 1)) {
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
                /* `unsigned u(void)`: the call's value is 4 bytes wide
                   (6.5.2.2p5), so u() - 1 wraps at 32 bits */
                if (w < 8) { if (w > 0) { if (declptr == 0) { if (gflt0 == 0) {
                    if (gstruct < 0) { if (symfpret[nsym - 1] == 0) symretw[nsym - 1] = w; }
                } } } }
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
                    n = initcount(w == 1 && declpd == 0 && gstruct < 0);
                    if (gstruct >= 0) {
                        int per; per = structslots(gstruct);
                        n = (n + per - 1) / per;
                    }
                }
                else n = cexpr();
                need(vfind(TOKV, NTOKV, "]", 1), "]");
                isarr = 1;
                n = dimtail(n);
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
                if (isarr) { if (gstruct >= 0 && gpd == 0) en(n * declsz); else en(n * w); }
                else { if (declptr) en(8); else {
                    if (gstruct >= 0) en(declsz); else en(n * w); } }
                ec(10);
            }
            if (cur() == tidx("=", 1)) {
                adv();
                toinit = 1; hasinit = 1;
                gk = dkind(gflt0);
                initflt = valuekind(w, gpd, declunsigned, gflt0, declbool);
                cpn = 0 - 1;
                if (cur() == tidx("(", 1)) cpn = cplit();
                if (cur() == tidx("{", 1)) {
                    if (isarr) { initisarr = 1; initrows = decldim2; initrows3 = decldim3; initaggr(1, t, 0, w, gstruct, n * w); }
                    else { if (gstruct >= 0) initaggr(1, t, 0, w, gstruct, declsz);
                           else initaggr(1, t, 0, w, 0 - 1, n * w); }
                    while (cpn > 0) { need(tidx(")", 1), ")"); cpn = cpn - 1; }
                }
                else { if (cur() == T_STR) { if (isarr) { if (w == strw(tp)) {
                    initstr(1, t, 0, n, 0);
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
    if (fd < 0) return enoinput(path);
    nout = __read(fd, out, MAXOUT);
    __close(fd);
    if (nout < 0) { printf("cannot read %s\n", path); return 1; }
    if (nout >= MAXOUT) { __write(2, "tape too large\n", 15); __exit(1); }
    return 0;
}

/* Does the path end in ".c"?  Under `-run` this is what separates the
   inputs from the program's own arguments. */
int istapebin(char *p);
int isdotc(char *p) {
    int n;
    if (p[0] == 45 && p[1] == 0) return 1;         /* `-`: standard input */
    if (istapebin(p)) return 1;
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

int istapebin(char *p) {
    int n;
    n = 0; while (p[n]) n = n + 1;
    if (n < 8) return 0;
    return p[n - 8] == 46 && p[n - 7] == 116 && p[n - 6] == 97
        && p[n - 5] == 112 && p[n - 4] == 101 && p[n - 3] == 98
        && p[n - 2] == 105 && p[n - 1] == 110;
}

/* Read, preprocess and lex ONE file.  Split out of fe_tape so that several
   files can be walked into one tape: preprocessing is per file (include
   guards, `#define` state, `__FILE__`), only the walk is shared -- the same
   division driver.compile_sources makes on the Python side. */
/* `int (twice)(int x) { ... }`, `char *(greet)(void)`, `int (v) = 5`: a
   parenthesised declarator NAME [R13-0b #14].  The parentheses only exist to
   keep a function-like macro from expanding the name (every entry point in
   lua.h is declared this way), so they are dropped from the token stream
   here, once, and every declaration path sees the plain name.  The rule is
   narrow: `( identifier )` right after a type keyword or `*`, followed by
   `(` `=` `;` `,` `[` or `)`.  A cast `(T)x` follows an operator, not a type
   word, so it is never touched; and around a lone identifier in an
   expression (`a * (b)`) the parentheses change nothing anyway. */
int parenfold(void) {
    int i; int j; int prev; int nxt; int k;
    int lp; int rp;
    lp = tidx("(", 1); rp = tidx(")", 1);
    i = 0; j = 0;
    while (i < ntok) {
        if (tkind[i] == lp && i + 2 < ntok && j > 0) { if (tkind[i + 1] == T_ID && tkind[i + 2] == rp) {
            prev = tkind[j - 1];
            nxt = kind(i + 3);
            k = 0;
            if (prev == T_TYPE || prev == tidx("*", 1)) k = 1;
            if (k) { k = 0;
                if (nxt == lp || nxt == rp || nxt == tidx("=", 1) || nxt == tidx(";", 1)
                    || nxt == tidx(",", 1) || nxt == tidx("[", 1)) k = 1; }
            if (k) {
                tkind[j] = tkind[i + 1]; tpos[j] = tpos[i + 1]; tlen[j] = tlen[i + 1];
                j = j + 1; i = i + 3; continue;
            }
        } }
        tkind[j] = tkind[i]; tpos[j] = tpos[i]; tlen[j] = tlen[i];
        j = j + 1; i = i + 1;
    }
    ntok = j;
    return 0;
}

int fe_load(char *path, char *t) {
    int fd; int k;
    srcpath = path;
    /* Source maps belong to this unit; warning counts belong to the program. */
    nspl = 0; nireg = 0; nfnpool = 0; nautoinc = 0; nld = 0;
    /* normalise EACH -I with a trailing slash, in the order given; the search
       walks them in that order */
    optincdl = 0;
    { int q; q = 0;
      while (q < noptinc) {
        char *d; int k2;
        d = optincs[q]; k2 = 0;
        while (d[k2] && k2 < 508) { incdir[q][k2] = d[k2]; k2 = k2 + 1; }
        if (k2 > 0 && incdir[q][k2 - 1] != 47) { incdir[q][k2] = 47; k2 = k2 + 1; }
        incdir[q][k2] = 0; iincdl[q] = k2;
        optincs[q] = incdir[q];
        q = q + 1;
      } }
    toinit = 0; hasinit = 0; unevaluated = 0;
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
        if (fd < 0) return enoinput(path);
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
        if (deponly == 0) __write(bkfd, src, nsrc);   /* -M: only the .d line */
        return 2;
    }
    if (lex() < 0) return 1;
    parenfold();
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
    nout = 0; nsym = 0; nsympk = 0; nlab = 0; npool = 0;
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
        /* before the walk, so a forward reference spells a name the way its
           definition will */
        if (u > 0) ustat_prescan();
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
/* the slot of the LABEL whose text is nm[0..L), or -1.  ud_slot() hashes a
   position in `out`; this hashes a caller-supplied name, which is what a
   `call NAME` operand is. */
int ud_find(char *nm, int L) {
    int h; int k; int ok;
    h = vhash(nm, L) * 16 + (L & 15);
    h = h & (UD_SIZE - 1);
    while (ud_tab[h]) { 
        k = 0; ok = 1;
        while (k < L) { if (out[ud_tab[h] - 1 + k] != nm[k]) { ok = 0; break; } k = k + 1; }
        if (ok) { if (ud_len(ud_tab[h] - 1) == L) return h; }
        h = (h + 1) & (UD_SIZE - 1);
    }
    return 0 - 1;
}
int ud_slot(int a) {                  /* its slot: found, or the free one */
    int h;
    h = vhash(out + a, ud_len(a)) * 16 + (ud_len(a) & 15);
    h = h & (UD_SIZE - 1);
    while (ud_tab[h]) { if (ud_same(ud_tab[h] - 1, a)) return h; h = (h + 1) & (UD_SIZE - 1); }
    return h;
}
/* Which labels does the program actually reach?

   A file-scope `static` that nothing calls is still emitted -- there is no
   source-level dead-code pass -- so an unused wrapper's `call ext_never_defined`
   reaches this check and was reported as an undefined function, while gcc and
   clang never emit the unused static at all and the program links.  miniz.h's
   zlib-compat wrappers are exactly this shape (static inline deflateInit ->
   mz_deflateInit) and made any program that included the header fail with 19
   errors.  [R13-0b #31]

   Reachability is computed over the TAPE, per block: a `name:` line starts a
   block, and a block's `call X` / `jump X` lines are its edges.  Roots are
   `main`, `__init`, every non-static function (they are visible to other
   units, so they cannot be dropped), and the names a global initialiser
   reaches -- `__init` covers the last of those.  A two-unit static already
   carries its unit suffix, so equal spellings cannot merge. */
/* ---- reachability, LINEAR in the size of the tape ----------------------
   The previous version re-scanned the whole tape inside a `while (changed)`
   fixed-point loop, and for every line it scanned back to find the enclosing
   label and compared names character by character.  On a tape that carries the
   libc bodies that is quadratic, and it cost the reference compiler 40x: 0.022 s
   became 0.882 s on `examples/hello.c`.  exec-chain runs the reference on 177
   files, so it went from 22 s to past its 60 s bound.

   One pass now builds the block table, one pass builds the edges, and a work
   queue propagates.  Each block is entered once and each line is touched a
   constant number of times.

     ud_lbl[i]    offset of block i's label line, -1 for a free slot
     ud_beg[i]    offset of the line AFTER that label (the block's body)
     ud_end[i]    offset one past the block's last line
     ud_out[i]    start of block i's edge list in ud_edges
     ud_nout[i]   how many edges
     ud_edges[]   the blocks block i calls or jumps to                    */

#define UD_MAXBLK 32768
#define UD_MAXEDGE 65536
int ud_reach[UD_SIZE];
int ud_lbl[UD_MAXBLK];
int ud_beg[UD_MAXBLK];
int ud_end[UD_MAXBLK];
int ud_out[UD_MAXBLK];
int ud_nout[UD_MAXBLK];
int ud_edges[UD_MAXEDGE];
int ud_nblk;
int ud_nedge;
int ud_queue[UD_MAXBLK];

/* the block whose label is the name nm[0..L), or -1 */
int ud_byname(char *nm, int L) {
    int i; int k; int ok;
    i = 0;
    while (i < ud_nblk) {
        if (ud_lbl[i] >= 0) {
            if (ud_lbl[i] >= 0) { }
            k = 0; ok = 1;
            while (k < L) { if (out[ud_lbl[i] + k] != nm[k]) { ok = 0; break; } k = k + 1; }
            if (ok) { if (out[ud_lbl[i] + L] == 58) return i; }
        }
        i = i + 1;
    }
    return 0 - 1;
}
int ud_eol(int a) { int e; e = a; while (e < nout) { if (out[e] == 10) break; e = e + 1; } return e; }
/* the block containing offset a: the last label at or above it.  ud_lbl is in
   tape order, so a binary search is enough -- no rescanning the tape. */
int ud_block_at(int a) {
    int lo; int hi; int mid;
    lo = 0; hi = ud_nblk - 1;
    if (ud_nblk == 0) return 0 - 1;
    if (a < ud_lbl[0]) return 0 - 1;
    while (lo < hi) {
        mid = (lo + hi + 1) / 2;
        if (ud_lbl[mid] <= a) lo = mid; else hi = mid - 1;
    }
    return lo;
}
/* is the label at `a` (ending at the colon `e-1`) one of THIS unit's statics? */
int ud_isstatic_label(int a, int e) {
    int n; int i; int k; int ok;
    n = e - 1 - a;
    i = 0;
    while (i + 3 < n) {                  /* strip a trailing __u<k> */
        if (out[a + i] == 95) { if (out[a + i + 1] == 95) { if (out[a + i + 2] == 117) n = i; } }
        i = i + 1;
    }
    if (n <= 0) return 0;
    i = 0;
    while (i < nustat) {
        k = 0; ok = 1;
        while (ustat[i * NAMEW + k]) {
            if (k >= n) { ok = 0; break; }
            if (ustat[i * NAMEW + k] != out[a + k]) { ok = 0; break; }
            k = k + 1;
        }
        if (ok) { if (k == n) return 1; }
        i = i + 1;
    }
    return 0;
}
/* The target of a call/jump on the line at `a`, written as a block index into
   *blk, or 0 if the line is not one.  The name is resolved ONCE, here. */
int ud_target(int a, int e, int *blk) {
    int j; int k; int nm; int L; int b;
    if (out[a] != 32) return 0;
    j = a;
    while (j < e && out[j] == 32) j = j + 1;      /* the tape indents by two */
    if (e - j < 5) return 0;
    k = 0;
    if (out[j] == 99) { if (out[j+1] == 97) { if (out[j+2] == 108) { if (out[j+3] == 108) { if (out[j+4] == 32) k = j + 5; } } } }
    if (out[j] == 106) { if (out[j+1] == 117) { if (out[j+2] == 109) { if (out[j+3] == 112) { if (out[j+4] == 32) k = j + 5; } } } }
    if (k == 0) return 0;
    nm = k; L = 0;
    while (nm + L < e && out[nm + L] != 10 && out[nm + L] != 32 && out[nm + L] != 44) L = L + 1;
    if (L == 0) return 0;
    b = ud_byname(out + nm, L);
    *blk = b;
    return 1;
}
int undef_reach(void) {
    int i; int e; int p; int b; int q; int head; int tail; int tgt;
    i = 0; while (i < UD_SIZE) { ud_reach[i] = 0; i = i + 1; }
    ud_nblk = 0; ud_nedge = 0;
    /* PASS 1: one block per `name:` line, in tape order */
    i = 0;
    while (i < nout) {
        e = ud_eol(i);
        if (e > i + 1) { if (out[e - 1] == 58) { if (out[i] != 32) { if (out[i] != 46) {
            if (ud_nblk < UD_MAXBLK) {
                ud_lbl[ud_nblk] = i;
                ud_beg[ud_nblk] = e + 1;
                ud_nout[ud_nblk] = 0;
                ud_reach[ud_nblk] = 0;
                ud_nblk = ud_nblk + 1;
            }
        } } } }
        i = e + 1;
    }
    /* each block's end is the next label's line start (blocks do not nest) */
    i = 0;
    while (i < ud_nblk) {
        if (i + 1 < ud_nblk) ud_end[i] = ud_lbl[i + 1]; else ud_end[i] = nout;
        i = i + 1;
    }
    /* PASS 2: the edges, resolved once */
    i = 0;
    while (i < ud_nblk) {
        ud_out[i] = ud_nedge;
        p = ud_beg[i];
        while (p < ud_end[i]) {
            e = ud_eol(p);
            if (ud_target(p, e, &tgt)) {
                if (tgt >= 0) {
                    if (ud_nedge < UD_MAXEDGE) { ud_edges[ud_nedge] = tgt; ud_nedge = ud_nedge + 1; ud_nout[i] = ud_nout[i] + 1; }
                }
            }
            p = e + 1;
        }
        i = i + 1;
    }
    /* PASS 3: roots, then a work queue -- each block enqueued once */
    head = 0; tail = 0;
    b = ud_byname("main", 4);        if (b >= 0) { if (!ud_reach[b]) { ud_reach[b] = 1; ud_queue[tail++] = b; } }
    b = ud_byname("__init", 6);      if (b >= 0) { if (!ud_reach[b]) { ud_reach[b] = 1; ud_queue[tail++] = b; } }
    i = 0;
    while (i < ud_nblk) {
        e = ud_eol(ud_lbl[i]);
        if (ud_isstatic_label(ud_lbl[i], e) == 0) {
            if (!ud_reach[i]) { ud_reach[i] = 1; if (tail < UD_MAXBLK) ud_queue[tail++] = i; }
        }
        i = i + 1;
    }
    while (head < tail) {
        b = ud_queue[head]; head = head + 1;
        q = ud_out[b];
        i = 0;
        while (i < ud_nout[b]) {
            tgt = ud_edges[q + i];
            if (tgt >= 0) { if (tgt < ud_nblk) { if (!ud_reach[tgt]) { ud_reach[tgt] = 1; if (tail < UD_MAXBLK) ud_queue[tail++] = tgt; } } }
            i = i + 1;
        }
    }
    return 0;
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
    undef_reach();
    bad = 0; i = 0;
    while (i < nout) {
        e = i; while (e < nout) { if (out[e] == 10) break; e = e + 1; }
        /* `  call NAME` -- es() has already asked irsel for the spelling */
        if (e - i > 7) { if (out[i] == 32) { if (out[i + 1] == 32) { if (out[i + 2] == 99) {
            if (out[i + 3] == 97) { if (out[i + 4] == 108) { if (out[i + 5] == 108) { if (out[i + 6] == 32) {
                h = ud_block_at(i);
                /* a call inside a block nothing reaches is not a reference:
                   gcc never emitted that block in the first place */
                if (h >= 0) { if (ud_reach[h] == 0) { i = e + 1; continue; } }
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
