/* M2 subset: UJS → \\0asm inside this module.
 * Build: ujs/scripts/build-compiler-wasm.sh → ujs/core/compiler.wasm
 * Cover: let/while/if/else/return · i64+f64 arith · list/len/idx/setidx.
 * Gaps: str literal / fn / full sim host parity — ship still open.
 */
#include <stdint.h>

/* freestanding — no libc */
static void *memcpy(void *d, const void *s, uint32_t n) {
  uint8_t *D = d; const uint8_t *S = s;
  for (uint32_t i = 0; i < n; i++) D[i] = S[i];
  return d;
}
static void *memset(void *d, int c, uint32_t n) {
  uint8_t *D = d;
  for (uint32_t i = 0; i < n; i++) D[i] = (uint8_t)c;
  return d;
}
static uint32_t strlen(const char *s) {
  uint32_t n = 0; while (s[n]) n++; return n;
}
static int strcmp(const char *a, const char *b) {
  while (*a && *a == *b) { a++; b++; }
  return (unsigned char)*a - (unsigned char)*b;
}
static char *strncpy(char *d, const char *s, uint32_t n) {
  uint32_t i = 0;
  for (; i < n && s[i]; i++) d[i] = s[i];
  for (; i < n; i++) d[i] = 0;
  return d;
}

enum { TAG_I64 = 2, TAG_F64 = 3, TAG_STR = 4, TAG_LIST = 5, TAG_DICT = 6,
        HEAP0 = 4096, GBASE = 2048, SCRATCH0 = 1024,
        HOST_SCRATCH = 950000, MEM_PAGES = 128 };
enum { TY_ERR = 0, TY_I64 = 1, TY_F64 = 2, TY_LIST = 3, TY_DICT = 4 };

static uint8_t  g_heap[256 * 1024];
static uint32_t g_bump;
static uint32_t g_out_ptr, g_out_len, g_meta_ptr, g_meta_len, g_err_ptr, g_err_len;

static uint32_t hall(uint32_t n) {
  if (g_bump < 64) g_bump = 64;
  uint32_t p = g_bump;
  uint32_t nx = (g_bump + n + 7u) & ~7u;
  if (nx > sizeof(g_heap)) return 0;
  g_bump = nx;
  return p;
}
static uint32_t poff(uint32_t o) { return (uint32_t)(uintptr_t)(g_heap + o); }

__attribute__((export_name("alloc")))
uint32_t alloc(uint32_t n) { uint32_t o = hall(n); return o ? poff(o) : 0; }
__attribute__((export_name("out_ptr")))  uint32_t out_ptr(void)  { return g_out_ptr; }
__attribute__((export_name("out_len")))  uint32_t out_len(void)  { return g_out_len; }
__attribute__((export_name("meta_ptr"))) uint32_t meta_ptr(void) { return g_meta_ptr; }
__attribute__((export_name("meta_len"))) uint32_t meta_len(void) { return g_meta_len; }
__attribute__((export_name("err_ptr")))  uint32_t err_ptr(void)  { return g_err_ptr; }
__attribute__((export_name("err_len")))  uint32_t err_len(void)  { return g_err_len; }

static void set_err(const char *m) {
  uint32_t n = (uint32_t)strlen(m);
  uint32_t o = hall(n + 1);
  if (!o) { g_err_ptr = g_err_len = 0; return; }
  memcpy(g_heap + o, m, n + 1);
  g_err_ptr = poff(o); g_err_len = n;
}

/* ---- buf ---- */
typedef struct { uint32_t off, n, cap; } Buf;
static uint8_t *bd(Buf *b) { return g_heap + b->off; }
static int binit(Buf *b, uint32_t cap) {
  uint32_t o = hall(cap); if (!o) return 0;
  b->off = o; b->n = 0; b->cap = cap; return 1;
}
static int bres(Buf *b, uint32_t need) {
  if (b->n + need <= b->cap) return 1;
  uint32_t nc = b->cap ? b->cap * 2 : 256;
  while (nc < b->n + need) nc *= 2;
  uint32_t o = hall(nc); if (!o) return 0;
  memcpy(g_heap + o, bd(b), b->n);
  b->off = o; b->cap = nc; return 1;
}
static int bu8(Buf *b, uint8_t v) { if (!bres(b,1)) return 0; bd(b)[b->n++] = v; return 1; }
static int bleu(Buf *b, uint32_t v) {
  for (;;) {
    uint8_t x = (uint8_t)(v & 0x7f); v >>= 7;
    if (v) x |= 0x80;
    if (!bu8(b, x)) return 0;
    if (!v) return 1;
  }
}
static int blei64(Buf *b, int64_t v) {
  for (;;) {
    uint8_t x = (uint8_t)(v & 0x7f); v >>= 7;
    int done = ((v == 0) && !(x & 0x40)) || ((v == -1) && (x & 0x40));
    if (!done) x |= 0x80;
    if (!bu8(b, x)) return 0;
    if (done) return 1;
  }
}
static int blei32(Buf *b, int32_t v) {
  for (;;) {
    uint8_t x = (uint8_t)(v & 0x7f); v >>= 7;
    int done = ((v == 0) && !(x & 0x40)) || ((v == -1) && (x & 0x40));
    if (!done) x |= 0x80;
    if (!bu8(b, x)) return 0;
    if (done) return 1;
  }
}

static int bi32(Buf *b, int32_t v) {
  return bu8(b, 0x41) && blei32(b, v);
}
static int braw(Buf *b, const uint8_t *s, uint32_t n) {
  if (!bres(b, n)) return 0; memcpy(bd(b) + b->n, s, n); b->n += n; return 1;
}
static int bstr(Buf *b, const char *s) {
  uint32_t n = (uint32_t)strlen(s); if (!bleu(b, n)) return 0; return braw(b, (const uint8_t *)s, n);
}
static int bf64(Buf *b, int64_t bits) {
  uint8_t u[8];
  uint64_t x = (uint64_t)bits;
  for (int i = 0; i < 8; i++) { u[i] = (uint8_t)(x & 0xff); x >>= 8; }
  return braw(b, u, 8);
}
static int sect(Buf *m, uint8_t id, Buf *body) {
  if (!bu8(m, id) || !bleu(m, body->n)) return 0;
  return braw(m, bd(body), body->n);
}

static int64_t d_bits(double d) {
  union { double d; int64_t i; } u; u.d = d; return u.i;
}

/* ---- lex ---- */
enum {
  T_EOF, T_NUM, T_FNUM, T_ID, T_LET, T_WHILE, T_RETURN, T_IF, T_ELSE,
  T_LP, T_RP, T_LB, T_RB, T_LS, T_RS, T_COMMA, T_COLON, T_DOT, T_SEMI, T_EQ,
  T_PLUS, T_MINUS, T_STAR, T_SLASH, T_PCT,
  T_LT, T_GT, T_LE, T_GE, T_EQEQ, T_NE, T_BAD
};
typedef struct {
  const char *src; uint32_t len, pos;
  int tok; int64_t num; double fnum; char id[64];
} Lex;
static int id0(char c) {
  return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||c=='_';
}
static int idc(char c) { return id0(c)||(c>='0'&&c<='9'); }
static int dig(char c) { return c>='0'&&c<='9'; }
static int spc(char c) { return c==' '||c=='\t'||c=='\n'||c=='\r'; }

static void next(Lex *L) {
  for (;;) {
    while (L->pos < L->len && spc(L->src[L->pos])) L->pos++;
    if (L->pos + 1 < L->len && L->src[L->pos] == '/' && L->src[L->pos + 1] == '/') {
      L->pos += 2;
      while (L->pos < L->len && L->src[L->pos] != '\n') L->pos++;
      continue;
    }
    break;
  }
  if (L->pos >= L->len) { L->tok = T_EOF; return; }
  char c = L->src[L->pos];
  if (dig(c)) {
    int64_t iv = 0;
    while (L->pos < L->len && dig(L->src[L->pos]))
      iv = iv * 10 + (L->src[L->pos++] - '0');
    if (L->pos < L->len && L->src[L->pos] == '.') {
      L->pos++;
      /* ratio tnum/fden — avoids place*=0.1 ulp drift vs exact IEEE */
      int64_t tnum = iv, fden = 1;
      while (L->pos < L->len && dig(L->src[L->pos])) {
        tnum = tnum * 10 + (L->src[L->pos++] - '0');
        fden *= 10;
      }
      L->fnum = (double)tnum / (double)fden; L->tok = T_FNUM; return;
    }
    L->num = iv; L->tok = T_NUM; return;
  }
  if (id0(c)) {
    uint32_t n = 0;
    while (L->pos < L->len && idc(L->src[L->pos]) && n + 1 < sizeof L->id)
      L->id[n++] = L->src[L->pos++];
    L->id[n] = 0;
    if (!strcmp(L->id,"let")) L->tok = T_LET;
    else if (!strcmp(L->id,"while")) L->tok = T_WHILE;
    else if (!strcmp(L->id,"return")) L->tok = T_RETURN;
    else if (!strcmp(L->id,"if")) L->tok = T_IF;
    else if (!strcmp(L->id,"else")) L->tok = T_ELSE;
    else L->tok = T_ID;
    return;
  }
  L->pos++;
  if (c=='('){L->tok=T_LP;return;} if (c==')'){L->tok=T_RP;return;}
  if (c=='{'){L->tok=T_LB;return;} if (c=='}'){L->tok=T_RB;return;}
  if (c=='['){L->tok=T_LS;return;} if (c==']'){L->tok=T_RS;return;}
  if (c==','){L->tok=T_COMMA;return;}
  if (c==':'){L->tok=T_COLON;return;}
  if (c=='.'){L->tok=T_DOT;return;}
  if (c==';'){L->tok=T_SEMI;return;}
  if (c=='+'){L->tok=T_PLUS;return;} if (c=='*'){L->tok=T_STAR;return;}
  if (c=='/'){L->tok=T_SLASH;return;} if (c=='%'){L->tok=T_PCT;return;}
  if (c=='-'){L->tok=T_MINUS;return;}
  if (c=='<'){ if (L->pos<L->len&&L->src[L->pos]=='='){L->pos++;L->tok=T_LE;} else L->tok=T_LT; return; }
  if (c=='>'){ if (L->pos<L->len&&L->src[L->pos]=='='){L->pos++;L->tok=T_GE;} else L->tok=T_GT; return; }
  if (c=='='){
    if (L->pos<L->len&&L->src[L->pos]=='='){
      L->pos++;
      if (L->pos<L->len&&L->src[L->pos]=='=') L->pos++; /* === */
      L->tok=T_EQEQ;
    } else L->tok=T_EQ;
    return;
  }
  if (c=='!' && L->pos<L->len && L->src[L->pos]=='='){ L->pos++; L->tok = T_NE; return; }
  L->tok = T_BAD;
}

/* ---- IR ---- */
enum {
  OP_CONST, OP_FCONST, OP_LOAD, OP_STORE,
  OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD,
  OP_FADD, OP_FSUB, OP_FMUL, OP_FDIV,
  OP_LT, OP_GT, OP_LE, OP_GE, OP_EQ, OP_NE, OP_NEG, OP_FNEG,
  OP_WBEGIN, OP_WCOND, OP_WEND,
  OP_IF0, OP_IF1, OP_IF2,
  OP_BOXI, OP_BOXF, OP_MKLIST, OP_MKLIST_DYN, OP_LSET, OP_IDXGET, OP_IDXSET, OP_LEN, OP_IWRAP,
  OP_GLOAD, OP_GSTORE,
  OP_MKDICT, OP_DSET, OP_DOT, OP_SCONST,
  OP_I2F, OP_I2F_R, OP_F2I,
  OP_FLT, OP_FGT, OP_FLE, OP_FGE, OP_FEQ, OP_FNE,
  OP_RET, OP_FRET, OP_HRET
};
enum { MAXC = 16384, MAXL = 128, MAXG = 64, MAXN = 32 };
typedef struct { uint8_t op; int64_t a; } Ins;
typedef struct {
  Ins code[MAXC]; int nc;
  char names[MAXL][MAXN]; uint8_t lty[MAXL]; uint8_t lety[MAXL]; int nl;
  char gnames[MAXG][MAXN]; uint8_t gty[MAXG]; uint8_t glety[MAXG]; int ng;
  int scratch; /* i32 temp for list/dict init, or -1 */
  int fscratch; /* f64 temp for i64→f64 under, or -1 */
  int list_elem_ty; /* TY_I64/TY_F64 of last list literal */
  const char *err;
} Prog;

static int emit(Prog *P, uint8_t op, int64_t a) {
  if (P->nc >= MAXC) { P->err = "code overflow"; return 0; }
  P->code[P->nc].op = op; P->code[P->nc].a = a; P->nc++; return 1;
}
static int loc(Prog *P, const char *name, int create) {
  for (int i = 0; i < P->nl; i++) if (!strcmp(P->names[i], name)) return i;
  if (!create) return -1;
  if (P->nl >= MAXL) { P->err = "too many locals"; return -1; }
  strncpy(P->names[P->nl], name, MAXN - 1);
  P->names[P->nl][MAXN - 1] = 0;
  P->lty[P->nl] = TY_I64;
  P->lety[P->nl] = TY_I64;
  return P->nl++;
}
static int glob(Prog *P, const char *name, int create) {
  for (int i = 0; i < P->ng; i++) if (!strcmp(P->gnames[i], name)) return i;
  if (!create) return -1;
  if (P->ng >= MAXG) { P->err = "too many globals"; return -1; }
  strncpy(P->gnames[P->ng], name, MAXN - 1);
  P->gnames[P->ng][MAXN - 1] = 0;
  P->gty[P->ng] = TY_F64; /* inject-friendly; assign overrides */
  P->glety[P->ng] = TY_F64;
  return P->ng++;
}
static int ensure_scratch(Prog *P) {
  if (P->scratch >= 0) return P->scratch;
  if (P->nl >= MAXL) { P->err = "too many locals"; return -1; }
  P->names[P->nl][0] = 0;
  P->lty[P->nl] = TY_LIST;
  P->lety[P->nl] = TY_I64;
  P->scratch = P->nl;
  return P->nl++;
}
static int ensure_fscratch(Prog *P) {
  if (P->fscratch >= 0) return P->fscratch;
  if (P->nl >= MAXL) { P->err = "too many locals"; return -1; }
  P->names[P->nl][0] = 0;
  P->lty[P->nl] = TY_F64;
  P->lety[P->nl] = TY_F64;
  P->fscratch = P->nl;
  return P->nl++;
}

/* pack short ASCII key into int64 (len in low 8 bits, chars in higher bytes) */
static int64_t pack_key(const char *s) {
  uint64_t v = 0; uint32_t n = 0;
  while (s[n] && n < 7) { v |= ((uint64_t)(uint8_t)s[n]) << (8 * (n + 1)); n++; }
  v |= n;
  return (int64_t)v;
}

static int pexpr(Lex *L, Prog *P);
static int pstmt(Lex *L, Prog *P);

static int expect(Lex *L, int t, Prog *P, const char *m) {
  if (L->tok != t) { P->err = m; return 0; }
  next(L); return 1;
}

static int bin_finish(Prog *P, int ty, int ty2, uint8_t oi, uint8_t of) {
  if (!ty || !ty2) return 0;
  if (ty == ty2) {
    if (ty != TY_I64 && ty != TY_F64) { P->err = "bad bin"; return 0; }
    if (!emit(P, ty == TY_F64 ? of : oi, 0)) return 0;
    return ty;
  }
  /* i64/f64 mix → f64; emit OP_I2F on the i64 side before op via marker */
  if (!((ty == TY_I64 && ty2 == TY_F64) || (ty == TY_F64 && ty2 == TY_I64))) {
    P->err = "mixed num types"; return 0;
  }
  if (ty == TY_I64) {
    if (ensure_fscratch(P) < 0) return 0;
    if (!emit(P, OP_I2F_R, 0)) return 0;
  } else {
    if (!emit(P, OP_I2F, 0)) return 0;
  }
  if (!emit(P, of, 0)) return 0;
  return TY_F64;
}

static int pprim(Lex *L, Prog *P) {
  if (L->tok == T_NUM) {
    if (!emit(P, OP_CONST, L->num)) return 0; next(L); return TY_I64;
  }
  if (L->tok == T_FNUM) {
    if (!emit(P, OP_FCONST, d_bits(L->fnum))) return 0; next(L); return TY_F64;
  }
  if (L->tok == T_LS) {
    /* [e, e, ...] — homogeneous i64 or f64 elems */
    next(L);
    int scratch = ensure_scratch(P); if (scratch < 0) return 0;
    int n = 0;
    if (L->tok == T_RS) {
      next(L);
      if (!emit(P, OP_MKLIST, 0)) return 0;
      P->list_elem_ty = TY_I64;
      return TY_LIST;
    }
    uint32_t save_pos = L->pos; int save_tok = L->tok;
    int64_t save_num = L->num; double save_fnum = L->fnum;
    char save_id[64]; strncpy(save_id, L->id, 64);
    n = 1;
    int depth = 0;
    for (;;) {
      if (L->tok == T_EOF) { P->err = "list]"; return 0; }
      if (L->tok == T_LS) depth++;
      else if (L->tok == T_RS) {
        if (depth == 0) break;
        depth--;
      } else if (L->tok == T_COMMA && depth == 0) n++;
      next(L);
    }
    L->pos = save_pos; L->tok = save_tok; L->num = save_num; L->fnum = save_fnum;
    strncpy(L->id, save_id, 64);
    if (n > 64) { P->err = "list too long"; return 0; }
    if (!emit(P, OP_MKLIST, n)) return 0;
    if (!emit(P, OP_STORE, scratch)) return 0;
    int elem_ty = 0;
    for (int i = 0; i < n; i++) {
      if (!emit(P, OP_LOAD, scratch)) return 0;
      int ty = pexpr(L, P); if (!ty) return 0;
      if (ty != TY_I64 && ty != TY_F64) { P->err = "list elem"; return 0; }
      if (!elem_ty) elem_ty = ty;
      else if (ty != elem_ty) { P->err = "list mixed"; return 0; }
      if (!emit(P, ty == TY_F64 ? OP_BOXF : OP_BOXI, 0)) return 0;
      if (!emit(P, OP_LSET, i)) return 0;
      if (i + 1 < n) {
        if (!expect(L, T_COMMA, P, "list ,")) return 0;
      }
    }
    if (!expect(L, T_RS, P, "list ]")) return 0;
    if (!emit(P, OP_LOAD, scratch)) return 0;
    P->list_elem_ty = elem_ty;
    P->lety[scratch] = (uint8_t)elem_ty;
    return TY_LIST;
  }
  if (L->tok == T_LB) {
    /* { k: v, ... } — ID keys, i64/f64 vals */
    next(L);
    int scratch = ensure_scratch(P); if (scratch < 0) return 0;
    if (L->tok == T_RB) {
      next(L);
      if (!emit(P, OP_MKDICT, 0)) return 0;
      return TY_DICT;
    }
    /* count pairs */
    uint32_t save_pos = L->pos; int save_tok = L->tok;
    int64_t save_num = L->num; double save_fnum = L->fnum;
    char save_id[64]; strncpy(save_id, L->id, 64);
    int n = 1, depth = 0;
    for (;;) {
      if (L->tok == T_EOF) { P->err = "dict}"; return 0; }
      if (L->tok == T_LB || L->tok == T_LS) depth++;
      else if (L->tok == T_RB || L->tok == T_RS) {
        if (depth == 0 && L->tok == T_RB) break;
        if (depth) depth--;
      } else if (L->tok == T_COMMA && depth == 0) n++;
      next(L);
    }
    L->pos = save_pos; L->tok = save_tok; L->num = save_num; L->fnum = save_fnum;
    strncpy(L->id, save_id, 64);
    if (n > 32) { P->err = "dict too long"; return 0; }
    if (!emit(P, OP_MKDICT, n)) return 0;
    if (!emit(P, OP_STORE, scratch)) return 0;
    for (int i = 0; i < n; i++) {
      if (L->tok != T_ID) { P->err = "dict key"; return 0; }
      int64_t key = pack_key(L->id);
      next(L);
      if (!expect(L, T_COLON, P, "dict :")) return 0;
      if (!emit(P, OP_LOAD, scratch)) return 0;
      if (!emit(P, OP_SCONST, key)) return 0;
      int vty = pexpr(L, P); if (!vty) return 0;
      if (vty == TY_I64) { if (!emit(P, OP_BOXI, 0)) return 0; }
      else if (vty == TY_F64) { if (!emit(P, OP_BOXF, 0)) return 0; }
      else if (vty != TY_LIST && vty != TY_DICT) { P->err = "dict val"; return 0; }
      if (!emit(P, OP_DSET, i)) return 0;
      if (i + 1 < n) {
        if (!expect(L, T_COMMA, P, "dict ,")) return 0;
      }
    }
    if (!expect(L, T_RB, P, "dict }")) return 0;
    if (!emit(P, OP_LOAD, scratch)) return 0;
    return TY_DICT;
  }
  if (L->tok == T_ID) {
    if (!strcmp(L->id, "len")) {
      next(L);
      if (!expect(L, T_LP, P, "len (")) return 0;
      /* len(name) on inject global → mark LIST before load */
      if (L->tok == T_ID) {
        int li = loc(P, L->id, 0);
        if (li < 0) {
          int g = glob(P, L->id, 1); if (g < 0) return 0;
          P->gty[g] = TY_LIST;
          P->list_elem_ty = P->glety[g] ? P->glety[g] : TY_F64;
          if (!emit(P, OP_GLOAD, (int64_t)g | ((int64_t)P->gty[g] << 16))) return 0;
          next(L);
          if (!expect(L, T_RP, P, "len )")) return 0;
          if (!emit(P, OP_LEN, 0)) return 0;
          return TY_I64;
        }
      }
      int ty = pexpr(L, P); if (!ty) return 0;
      if (ty != TY_LIST && ty != TY_DICT) { P->err = "len(list)"; return 0; }
      if (!expect(L, T_RP, P, "len )")) return 0;
      if (!emit(P, OP_LEN, 0)) return 0;
      return TY_I64;
    }
    /* list(n) → zeroed list of length n (i64 elems); for inject buffers */
    if (!strcmp(L->id, "list")) {
      next(L);
      if (!expect(L, T_LP, P, "list (")) return 0;
      int ty = pexpr(L, P); if (!ty) return 0;
      if (ty != TY_I64) { P->err = "list(n) i64"; return 0; }
      if (!expect(L, T_RP, P, "list )")) return 0;
      if (!emit(P, OP_IWRAP, 0)) return 0; /* i32 for mk_list */
      if (!emit(P, OP_MKLIST_DYN, 0)) return 0;
      P->list_elem_ty = TY_I64;
      return TY_LIST;
    }
    int i = loc(P, L->id, 0);
    if (i >= 0) {
      if (P->lty[i] == TY_LIST) P->list_elem_ty = P->lety[i];
      if (!emit(P, OP_LOAD, i)) return 0; next(L); return P->lty[i];
    }
    int g = glob(P, L->id, 1); if (g < 0) return 0;
    /* peek: name[ → inject list */
    next(L);
    if (L->tok == T_LS && P->gty[g] != TY_LIST && P->gty[g] != TY_DICT)
      P->gty[g] = TY_LIST;
    if (P->gty[g] == TY_LIST) P->list_elem_ty = P->glety[g] ? P->glety[g] : TY_F64;
    if (!emit(P, OP_GLOAD, (int64_t)g | ((int64_t)P->gty[g] << 16))) return 0;
    return P->gty[g];
  }
  if (L->tok == T_LP) {
    next(L); int ty = pexpr(L, P); if (!ty) return 0;
    if (!expect(L, T_RP, P, "expected )")) return 0;
    return ty;
  }
  if (L->tok == T_MINUS) {
    next(L); int ty = pprim(L, P); if (!ty) return 0;
    if (!emit(P, ty == TY_F64 ? OP_FNEG : OP_NEG, 0)) return 0;
    return ty;
  }
  P->err = "bad primary"; return 0;
}

static int ppostfix(Lex *L, Prog *P) {
  int ty = pprim(L, P); if (!ty) return 0;
  for (;;) {
    if (L->tok == T_LS) {
      if (ty != TY_LIST) { P->err = "index non-list"; return 0; }
      next(L);
      int ity = pexpr(L, P); if (!ity) return 0;
      if (ity != TY_I64) { P->err = "index type"; return 0; }
      if (!expect(L, T_RS, P, "expected ]")) return 0;
      /* a = elem type hint in OP_IDXGET: 0=i64 1=f64 — set from list_elem_ty / lety */
      int et = P->list_elem_ty ? P->list_elem_ty : TY_I64;
      if (!emit(P, OP_IDXGET, et == TY_F64 ? 1 : 0)) return 0;
      ty = et;
    } else if (L->tok == T_DOT) {
      if (ty != TY_DICT) { P->err = "dot non-dict"; return 0; }
      next(L);
      if (L->tok != T_ID) { P->err = "dot name"; return 0; }
      if (!emit(P, OP_SCONST, pack_key(L->id))) return 0;
      next(L);
      if (!emit(P, OP_DOT, 0)) return 0;
      ty = TY_I64; /* subset: dot yields unboxed i64 (or f64 via tag — i64 for fold) */
    } else break;
  }
  return ty;
}
static int pmul(Lex *L, Prog *P) {
  int ty = ppostfix(L, P); if (!ty) return 0;
  for (;;) {
    uint8_t oi, of;
    if (L->tok==T_STAR) { oi=OP_MUL; of=OP_FMUL; }
    else if (L->tok==T_SLASH) { oi=OP_DIV; of=OP_FDIV; }
    else if (L->tok==T_PCT) {
      if (ty == TY_F64) { P->err = "f64 mod"; return 0; }
      oi=OP_MOD; of=OP_MOD;
    } else break;
    next(L); int ty2 = ppostfix(L, P); if (!ty2) return 0;
    ty = bin_finish(P, ty, ty2, oi, of); if (!ty) return 0;
  }
  return ty;
}
static int padd(Lex *L, Prog *P) {
  int ty = pmul(L, P); if (!ty) return 0;
  for (;;) {
    uint8_t oi, of;
    if (L->tok==T_PLUS) { oi=OP_ADD; of=OP_FADD; }
    else if (L->tok==T_MINUS) { oi=OP_SUB; of=OP_FSUB; }
    else break;
    next(L); int ty2 = pmul(L, P); if (!ty2) return 0;
    ty = bin_finish(P, ty, ty2, oi, of); if (!ty) return 0;
  }
  return ty;
}
static int pcmp(Lex *L, Prog *P) {
  int ty = padd(L, P); if (!ty) return 0;
  uint8_t opi, opf;
  if (L->tok==T_LT) { opi=OP_LT; opf=OP_FLT; }
  else if (L->tok==T_GT) { opi=OP_GT; opf=OP_FGT; }
  else if (L->tok==T_LE) { opi=OP_LE; opf=OP_FLE; }
  else if (L->tok==T_GE) { opi=OP_GE; opf=OP_FGE; }
  else if (L->tok==T_EQEQ) { opi=OP_EQ; opf=OP_FEQ; }
  else if (L->tok==T_NE) { opi=OP_NE; opf=OP_FNE; }
  else return ty;
  next(L); int ty2 = padd(L, P); if (!ty2) return 0;
  if (ty == TY_I64 && ty2 == TY_I64) {
    if (!emit(P, opi, 0)) return 0;
    return TY_I64;
  }
  if (ty == TY_I64 && ty2 == TY_F64) {
    if (ensure_fscratch(P) < 0) return 0;
    if (!emit(P, OP_I2F_R, 0)) return 0;
  } else if (ty == TY_F64 && ty2 == TY_I64) {
    if (!emit(P, OP_I2F, 0)) return 0;
  } else if (!(ty == TY_F64 && ty2 == TY_F64)) {
    P->err = "mixed num types"; return 0;
  }
  if (!emit(P, opf, 0)) return 0;
  return TY_I64;
}
static int pexpr(Lex *L, Prog *P) { return pcmp(L, P); }

static int pblock(Lex *L, Prog *P) {
  if (!expect(L, T_LB, P, "expected {")) return 0;
  while (L->tok != T_RB && L->tok != T_EOF)
    if (!pstmt(L, P)) return 0;
  return expect(L, T_RB, P, "expected }");
}

static int pstmt(Lex *L, Prog *P) {
  if (L->tok == T_BAD) { P->err = "bad token"; return 0; }
  if (L->tok == T_LET) {
    next(L);
    if (L->tok != T_ID) { P->err = "let name"; return 0; }
    int i = loc(P, L->id, 1); if (i < 0) return 0;
    next(L);
    if (!expect(L, T_EQ, P, "expected =")) return 0;
    int ty = pexpr(L, P); if (!ty) return 0;
    P->lty[i] = (uint8_t)ty;
    if (ty == TY_LIST) P->lety[i] = (uint8_t)P->list_elem_ty;
    if (!emit(P, OP_STORE, i)) return 0;
    return expect(L, T_SEMI, P, "expected ;");
  }
  if (L->tok == T_WHILE) {
    next(L);
    if (!emit(P, OP_WBEGIN, 0)) return 0;
    if (!expect(L, T_LP, P, "while (")) return 0;
    int ty = pexpr(L, P); if (!ty) return 0;
    if (ty != TY_I64) { P->err = "while cond"; return 0; }
    if (!expect(L, T_RP, P, "while )")) return 0;
    if (!emit(P, OP_WCOND, 0)) return 0;
    if (!pblock(L, P)) return 0;
    return emit(P, OP_WEND, 0);
  }
  if (L->tok == T_IF) {
    next(L);
    if (!expect(L, T_LP, P, "if (")) return 0;
    int ty = pexpr(L, P); if (!ty) return 0;
    if (ty != TY_I64) { P->err = "if cond"; return 0; }
    if (!expect(L, T_RP, P, "if )")) return 0;
    if (!emit(P, OP_IF0, 0)) return 0;
    if (!pblock(L, P)) return 0;
    if (!emit(P, OP_IF1, 0)) return 0;
    if (L->tok == T_ELSE) {
      next(L);
      /* else if (...) … — reuse if path; else still requires { block */
      if (L->tok == T_IF) {
        if (!pstmt(L, P)) return 0;
      } else {
        if (!pblock(L, P)) return 0;
      }
    }
    return emit(P, OP_IF2, 0);
  }
  if (L->tok == T_RETURN) {
    next(L);
    int ty = pexpr(L, P); if (!ty) return 0;
    uint8_t op = OP_RET;
    if (ty == TY_F64) op = OP_FRET;
    else if (ty == TY_LIST || ty == TY_DICT) op = OP_HRET;
    if (!emit(P, op, 0)) return 0;
    return expect(L, T_SEMI, P, "expected ;");
  }
  if (L->tok == T_ID) {
    char name[MAXN];
    strncpy(name, L->id, MAXN - 1); name[MAXN - 1] = 0;
    next(L);
    if (L->tok == T_LS) {
      /* xs[i] = e  (local or global list) */
      int is_g = 0;
      int li = loc(P, name, 0);
      if (li < 0) {
        li = glob(P, name, 1); if (li < 0) return 0;
        is_g = 1;
        if (P->gty[li] != TY_LIST && P->gty[li] != TY_I64) {
          /* first use as list via setidx — mark as list */
        }
        P->gty[li] = TY_LIST;
      } else if (P->lty[li] != TY_LIST) { P->err = "setidx non-list"; return 0; }
      if (!emit(P, is_g ? OP_GLOAD : OP_LOAD,
                is_g ? ((int64_t)li | ((int64_t)P->gty[li] << 16)) : (int64_t)li)) return 0;
      next(L);
      int ity = pexpr(L, P); if (!ity) return 0;
      if (ity != TY_I64) { P->err = "index type"; return 0; }
      if (!emit(P, OP_IWRAP, 0)) return 0;
      if (!expect(L, T_RS, P, "expected ]")) return 0;
      if (!expect(L, T_EQ, P, "expected =")) return 0;
      int vty = pexpr(L, P); if (!vty) return 0;
      int et = is_g ? P->glety[li] : P->lety[li];
      if (!et) et = TY_I64;
      if (vty != et && !(et == TY_I64 && vty == TY_I64)) {
        if (vty != TY_I64 && vty != TY_F64) { P->err = "setidx val"; return 0; }
        /* allow setting elem type on first write */
        if (is_g) P->glety[li] = (uint8_t)vty; else P->lety[li] = (uint8_t)vty;
        et = vty;
      }
      if (vty != et) { P->err = "setidx val"; return 0; }
      if (!emit(P, vty == TY_F64 ? OP_BOXF : OP_BOXI, 0)) return 0;
      if (!emit(P, OP_IDXSET, 0)) return 0;
      return expect(L, T_SEMI, P, "expected ;");
    }
    if (L->tok != T_EQ) { P->err = "expected ="; return 0; }
    int is_g = 0;
    int i = loc(P, name, 0);
    if (i < 0) {
      i = glob(P, name, 1); if (i < 0) return 0;
      is_g = 1;
    }
    next(L);
    int ty = pexpr(L, P); if (!ty) return 0;
    if (is_g) {
      P->gty[i] = (uint8_t)ty;
      if (ty == TY_LIST) P->glety[i] = (uint8_t)P->list_elem_ty;
    } else {
      /* inject list elems are f64; allow trunc into i64 locals (char codes) */
      if (ty == TY_F64 && (int)P->lty[i] == TY_I64) {
        if (!emit(P, OP_F2I, 0)) return 0;
        ty = TY_I64;
      } else if (ty != (int)P->lty[i]) { P->err = "assign type"; return 0; }
    }
    if (!emit(P, is_g ? OP_GSTORE : OP_STORE,
              is_g ? ((int64_t)i | ((int64_t)P->gty[i] << 16)) : (int64_t)i)) return 0;
    return expect(L, T_SEMI, P, "expected ;");
  }
  P->err = "bad stmt"; return 0;
}

/* ---- wasm binary ---- */
static int emit_cmp(Buf *c, uint8_t op) {
  uint8_t w;
  switch (op) {
  case OP_LT: w=0x53; break; case OP_GT: w=0x55; break;
  case OP_LE: w=0x57; break; case OP_GE: w=0x59; break;
  case OP_EQ: w=0x51; break; case OP_NE: w=0x52; break;
  default: return 0;
  }
  if (!bu8(c, w)) return 0;
  return bu8(c, 0xad); /* i64.extend_i32_u */
}

static int emit_ops(Buf *c, Prog *P) {
  for (int i = 0; i < P->nc; i++) {
    Ins *ins = &P->code[i];
    switch (ins->op) {
    case OP_CONST:
      if (!bu8(c, 0x42) || !blei64(c, ins->a)) return 0; break;
    case OP_FCONST:
      if (!bu8(c, 0x44) || !bf64(c, ins->a)) return 0; break;
    case OP_LOAD:
      if (!bu8(c, 0x20) || !bleu(c, (uint32_t)ins->a)) return 0; break;
    case OP_STORE:
      if (!bu8(c, 0x21) || !bleu(c, (uint32_t)ins->a)) return 0; break;
    case OP_ADD: if (!bu8(c, 0x7c)) return 0; break;
    case OP_SUB: if (!bu8(c, 0x7d)) return 0; break;
    case OP_MUL: if (!bu8(c, 0x7e)) return 0; break;
    case OP_DIV: if (!bu8(c, 0x7f)) return 0; break;
    case OP_MOD: if (!bu8(c, 0x81)) return 0; break;
    case OP_FADD: if (!bu8(c, 0xa0)) return 0; break;
    case OP_FSUB: if (!bu8(c, 0xa1)) return 0; break;
    case OP_FMUL: if (!bu8(c, 0xa2)) return 0; break;
    case OP_FDIV: if (!bu8(c, 0xa3)) return 0; break;
    case OP_LT: case OP_GT: case OP_LE: case OP_GE: case OP_EQ: case OP_NE:
      if (!emit_cmp(c, ins->op)) return 0; break;
    case OP_NEG:
      if (!bu8(c, 0x42) || !blei64(c, -1) || !bu8(c, 0x7e)) return 0; break;
    case OP_FNEG:
      if (!bu8(c, 0x9a)) return 0; break; /* f64.neg */
    case OP_WBEGIN:
      if (!bu8(c, 0x02) || !bu8(c, 0x40)) return 0;
      if (!bu8(c, 0x03) || !bu8(c, 0x40)) return 0;
      break;
    case OP_WCOND:
      if (!bu8(c, 0xa7)) return 0;
      if (!bu8(c, 0x45)) return 0;
      if (!bu8(c, 0x0d) || !bleu(c, 1)) return 0;
      break;
    case OP_WEND:
      if (!bu8(c, 0x0c) || !bleu(c, 0)) return 0;
      if (!bu8(c, 0x0b) || !bu8(c, 0x0b)) return 0;
      break;
    case OP_IF0:
      if (!bu8(c, 0xa7)) return 0;
      if (!bu8(c, 0x04) || !bu8(c, 0x40)) return 0;
      break;
    case OP_IF1:
      if (!bu8(c, 0x05)) return 0;
      break;
    case OP_IF2:
      if (!bu8(c, 0x0b)) return 0;
      break;
    case OP_BOXI:
      if (!bu8(c, 0x10) || !bleu(c, 0)) return 0; /* call mk_i64 */
      break;
    case OP_BOXF:
      if (!bu8(c, 0x10) || !bleu(c, 4)) return 0; /* call mk_f64 */
      break;
    case OP_MKLIST:
      if (!bu8(c, 0x41) || !blei32(c, (int32_t)ins->a)) return 0;
      if (!bu8(c, 0x10) || !bleu(c, 6)) return 0; /* call mk_list */
      break;
    case OP_MKLIST_DYN:
      /* n already i32 on stack */
      if (!bu8(c, 0x10) || !bleu(c, 6)) return 0;
      break;
    case OP_LSET:
      if (!bu8(c, 0x41) || !blei32(c, (int32_t)ins->a)) return 0;
      if (!bu8(c, 0x10) || !bleu(c, 8)) return 0;
      break;
    case OP_IDXGET:
      if (!bu8(c, 0xa7)) return 0;
      if (!bu8(c, 0x10) || !bleu(c, 7)) return 0; /* list_get */
      if (ins->a)
        { if (!bu8(c, 0x10) || !bleu(c, 5)) return 0; } /* f64_of */
      else
        { if (!bu8(c, 0x10) || !bleu(c, 3)) return 0; } /* i64_of */
      break;
    case OP_IWRAP:
      if (!bu8(c, 0xa7)) return 0;
      break;
    case OP_I2F:
      if (!bu8(c, 0xb9)) return 0;
      break;
    case OP_F2I:
      if (!bu8(c, 0xb0)) return 0; /* i64.trunc_f64_s */
      break;
    case OP_I2F_R: {
      int fs = ensure_fscratch(P); if (fs < 0) return 0;
      /* stack i64 f64 → local.set fs; convert; local.get fs */
      if (!bu8(c, 0x21) || !bleu(c, (uint32_t)fs)) return 0;
      if (!bu8(c, 0xb9)) return 0;
      if (!bu8(c, 0x20) || !bleu(c, (uint32_t)fs)) return 0;
      break;
    }
    case OP_FLT: case OP_FGT: case OP_FLE: case OP_FGE: case OP_FEQ: case OP_FNE: {
      uint8_t w = 0x63; /* f64.lt */
      if (ins->op == OP_FGT) w = 0x64;
      else if (ins->op == OP_FLE) w = 0x65;
      else if (ins->op == OP_FGE) w = 0x66;
      else if (ins->op == OP_FEQ) w = 0x61;
      else if (ins->op == OP_FNE) w = 0x62;
      if (!bu8(c, w)) return 0;
      if (!bu8(c, 0xad)) return 0; /* extend to i64 */
      break;
    }
    case OP_IDXSET:
      if (!bu8(c, 0x10) || !bleu(c, 9)) return 0;
      break;
    case OP_LEN:
      if (!bu8(c, 0x10) || !bleu(c, 10)) return 0;
      if (!bu8(c, 0xad)) return 0;
      break;
    case OP_GLOAD: {
      uint32_t slot = (uint32_t)(ins->a & 0xffff);
      uint8_t ty = (uint8_t)((ins->a >> 16) & 0xff);
      if (!ty) ty = P->gty[slot];
      uint32_t addr = GBASE + slot * 4u;
      if (!bi32(c, (int32_t)addr)) return 0;
      if (!bu8(c, 0x28) || !bu8(c, 0x02) || !bleu(c, 0)) return 0;
      if (ty == TY_F64) {
        if (!bu8(c, 0x10) || !bleu(c, 5)) return 0;
      } else if (ty == TY_I64) {
        if (!bu8(c, 0x10) || !bleu(c, 3)) return 0;
      }
      break;
    }
    case OP_GSTORE: {
      uint32_t slot = (uint32_t)(ins->a & 0xffff);
      uint8_t ty = (uint8_t)((ins->a >> 16) & 0xff);
      if (!ty) ty = P->gty[slot];
      if (ty == TY_I64) {
        if (!bu8(c, 0x10) || !bleu(c, 0)) return 0;
      } else if (ty == TY_F64) {
        if (!bu8(c, 0x10) || !bleu(c, 4)) return 0;
      }
      if (!bi32(c, (int32_t)slot)) return 0;
      if (!bu8(c, 0x10) || !bleu(c, 15)) return 0;
      break;
    }
    case OP_SCONST: {
      /* pack_key → bytes at SCRATCH0, call mk_str */
      uint64_t v = (uint64_t)ins->a;
      uint32_t n = (uint32_t)(v & 0xff);
      for (uint32_t i = 0; i < n; i++) {
        uint8_t ch = (uint8_t)((v >> (8 * (i + 1))) & 0xff);
        if (!bi32(c, (int32_t)(SCRATCH0 + i))) return 0;
        if (!bi32(c, (int32_t)(ch))) return 0;
        if (!bu8(c, 0x3a) || !bu8(c, 0x00) || !bleu(c, 0)) return 0;
      }
      if (!bi32(c, (int32_t)(SCRATCH0))) return 0;
      if (!bi32(c, (int32_t)(n))) return 0;
      if (!bu8(c, 0x10) || !bleu(c, 11)) return 0; /* mk_str */
      break;
    }
    case OP_MKDICT:
      if (!bu8(c, 0x41) || !blei32(c, (int32_t)ins->a)) return 0;
      if (!bu8(c, 0x10) || !bleu(c, 12)) return 0; /* mk_dict */
      break;
    case OP_DSET:
      /* stack h k v → push i → dict_set_hkvi */
      if (!bu8(c, 0x41) || !blei32(c, (int32_t)ins->a)) return 0;
      if (!bu8(c, 0x10) || !bleu(c, 16)) return 0;
      break;
    case OP_DOT:
      /* stack d k → dict_get → i64_of */
      if (!bu8(c, 0x10) || !bleu(c, 17)) return 0; /* dict_get */
      if (!bu8(c, 0x10) || !bleu(c, 3)) return 0; /* i64_of */
      break;
    case OP_RET:
      if (!bu8(c, 0x10) || !bleu(c, 0)) return 0;
      if (!bu8(c, 0x0f)) return 0;
      break;
    case OP_FRET:
      if (!bu8(c, 0x10) || !bleu(c, 4)) return 0;
      if (!bu8(c, 0x0f)) return 0;
      break;
    case OP_HRET:
      if (!bu8(c, 0x0f)) return 0; /* handle already on stack */
      break;
    default: return 0;
    }
  }
  return 1;
}

static int mk_num_body(Buf *c, int is_f64) {
  /* (param i64|f64)(result i32)(local i32) — param type already in type section */
  if (!bleu(c, 1) || !bleu(c, 1) || !bu8(c, 0x7f)) return 0;
  if (!bi32(c, (int32_t)(0))) return 0;
  if (!bu8(c, 0x28) || !bu8(c, 0x02) || !bleu(c, 0)) return 0;
  if (!bu8(c, 0x22) || !bleu(c, 1)) return 0;
  if (!bu8(c, 0x45)) return 0;
  if (!bu8(c, 0x04) || !bu8(c, 0x40)) return 0;
  if (!bi32(c, (int32_t)(HEAP0))) return 0;
  if (!bu8(c, 0x21) || !bleu(c, 1)) return 0;
  if (!bu8(c, 0x0b)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 1)) return 0;
  if (!bi32(c, (int32_t)(is_f64 ? TAG_F64 : TAG_I64))) return 0;
  if (!bu8(c, 0x3a) || !bu8(c, 0x00) || !bleu(c, 0)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 1)) return 0;
  if (!bi32(c, (int32_t)(8))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 0)) return 0;
  if (is_f64) {
    if (!bu8(c, 0x39) || !bu8(c, 0x03) || !bleu(c, 0)) return 0; /* f64.store */
  } else {
    if (!bu8(c, 0x37) || !bu8(c, 0x03) || !bleu(c, 0)) return 0; /* i64.store */
  }
  if (!bi32(c, (int32_t)(0))) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 1)) return 0;
  if (!bi32(c, (int32_t)(16))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bi32(c, (int32_t)(7))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bi32(c, -8)) return 0;
  if (!bu8(c, 0x71)) return 0;
  if (!bu8(c, 0x36) || !bu8(c, 0x02) || !bleu(c, 0)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 1)) return 0;
  return bu8(c, 0x0b);
}

static int emit_locals(Buf *c, Prog *P) {
  if (P->nl <= 0) return bleu(c, 0);
  if (!bleu(c, (uint32_t)P->nl)) return 0;
  for (int i = 0; i < P->nl; i++) {
    if (!bleu(c, 1)) return 0;
    uint8_t wt = 0x7e; /* i64 */
    if (P->lty[i] == TY_F64) wt = 0x7c;
    else if (P->lty[i] == TY_LIST || P->lty[i] == TY_DICT) wt = 0x7f;
    if (!bu8(c, wt)) return 0;
  }
  return 1;
}

/* mk_list(n): tag LIST, len at +2, n × i32 slots at +8 */
static int mk_list_body(Buf *c) {
  /* locals: p, i  (params: n = local 0) */
  if (!bleu(c, 1) || !bleu(c, 2) || !bu8(c, 0x7f)) return 0;
  /* p = freep; if eqz p = HEAP0 */
  if (!bi32(c, (int32_t)(0))) return 0;
  if (!bu8(c, 0x28) || !bu8(c, 0x02) || !bleu(c, 0)) return 0;
  if (!bu8(c, 0x22) || !bleu(c, 1)) return 0;
  if (!bu8(c, 0x45)) return 0;
  if (!bu8(c, 0x04) || !bu8(c, 0x40)) return 0;
  if (!bi32(c, (int32_t)(HEAP0))) return 0;
  if (!bu8(c, 0x21) || !bleu(c, 1)) return 0;
  if (!bu8(c, 0x0b)) return 0;
  /* store tag + len */
  if (!bu8(c, 0x20) || !bleu(c, 1)) return 0;
  if (!bi32(c, (int32_t)(TAG_LIST))) return 0;
  if (!bu8(c, 0x3a) || !bu8(c, 0x00) || !bleu(c, 0)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 1)) return 0;
  if (!bi32(c, (int32_t)(4))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 0)) return 0;
  if (!bu8(c, 0x36) || !bu8(c, 0x02) || !bleu(c, 0)) return 0; /* i32.store len */
  /* freep = align(p + 8 + n*4) */
  if (!bi32(c, (int32_t)(0))) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 1)) return 0;
  if (!bi32(c, (int32_t)(8))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 0)) return 0;
  if (!bi32(c, (int32_t)(2))) return 0;
  if (!bu8(c, 0x74)) return 0; /* i32.shl n<<2 */
  if (!bu8(c, 0x6a)) return 0;
  if (!bi32(c, (int32_t)(7))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bi32(c, -8)) return 0;
  if (!bu8(c, 0x71)) return 0;
  if (!bu8(c, 0x36) || !bu8(c, 0x02) || !bleu(c, 0)) return 0;
  /* zero slots: i=0; loop */
  if (!bi32(c, (int32_t)(0))) return 0;
  if (!bu8(c, 0x21) || !bleu(c, 2)) return 0;
  if (!bu8(c, 0x02) || !bu8(c, 0x40)) return 0; /* block */
  if (!bu8(c, 0x03) || !bu8(c, 0x40)) return 0; /* loop */
  if (!bu8(c, 0x20) || !bleu(c, 2)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 0)) return 0;
  if (!bu8(c, 0x4f)) return 0; /* i32.ge_u */
  if (!bu8(c, 0x0d) || !bleu(c, 1)) return 0; /* br_if block */
  if (!bu8(c, 0x20) || !bleu(c, 1)) return 0;
  if (!bi32(c, (int32_t)(8))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 2)) return 0;
  if (!bi32(c, (int32_t)(2))) return 0;
  if (!bu8(c, 0x74)) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bi32(c, (int32_t)(0))) return 0;
  if (!bu8(c, 0x36) || !bu8(c, 0x02) || !bleu(c, 0)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 2)) return 0;
  if (!bi32(c, (int32_t)(1))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x21) || !bleu(c, 2)) return 0;
  if (!bu8(c, 0x0c) || !bleu(c, 0)) return 0;
  if (!bu8(c, 0x0b) || !bu8(c, 0x0b)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 1)) return 0;
  return bu8(c, 0x0b);
}

static int list_get_body(Buf *c) {
  /* (param h i) (result i32) — load h+8+i*4 */
  if (!bleu(c, 0)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 0)) return 0;
  if (!bi32(c, (int32_t)(8))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 1)) return 0;
  if (!bi32(c, (int32_t)(2))) return 0;
  if (!bu8(c, 0x74)) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x28) || !bu8(c, 0x02) || !bleu(c, 0)) return 0;
  return bu8(c, 0x0b);
}

static int list_set_body(Buf *c, int hvi) {
  /* hvi=1: params h,v,i ; hvi=0: params h,i,v */
  if (!bleu(c, 0)) return 0;
  uint32_t ph = 0, pi = hvi ? 2 : 1, pv = hvi ? 1 : 2;
  if (!bu8(c, 0x20) || !bleu(c, ph)) return 0;
  if (!bi32(c, (int32_t)(8))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, pi)) return 0;
  if (!bi32(c, (int32_t)(2))) return 0;
  if (!bu8(c, 0x74)) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, pv)) return 0;
  if (!bu8(c, 0x36) || !bu8(c, 0x02) || !bleu(c, 0)) return 0;
  return bu8(c, 0x0b);
}


static int len_of_body(Buf *c) {
  /* u32 length at +4 (was u16 at +2; SRC can exceed 65535) */
  if (!bleu(c, 0)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 0)) return 0;
  if (!bi32(c, (int32_t)(4))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x28) || !bu8(c, 0x02) || !bleu(c, 0)) return 0; /* i32.load */
  return bu8(c, 0x0b);
}

static int mk_str_body(Buf *c) {
  /* (param src n)(result i32)(local p i) — locals at 2,3 */
  if (!bleu(c, 1) || !bleu(c, 2) || !bu8(c, 0x7f)) return 0;
  if (!bi32(c, (int32_t)(0))) return 0;
  if (!bu8(c, 0x28) || !bu8(c, 0x02) || !bleu(c, 0)) return 0;
  if (!bu8(c, 0x22) || !bleu(c, 2)) return 0;
  if (!bu8(c, 0x45)) return 0;
  if (!bu8(c, 0x04) || !bu8(c, 0x40)) return 0;
  if (!bi32(c, (int32_t)(HEAP0))) return 0;
  if (!bu8(c, 0x21) || !bleu(c, 2)) return 0;
  if (!bu8(c, 0x0b)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 2)) return 0;
  if (!bi32(c, (int32_t)(TAG_STR))) return 0;
  if (!bu8(c, 0x3a) || !bu8(c, 0x00) || !bleu(c, 0)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 2)) return 0;
  if (!bi32(c, (int32_t)(4))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 1)) return 0;
  if (!bu8(c, 0x36) || !bu8(c, 0x02) || !bleu(c, 0)) return 0; /* i32.store len */
  if (!bi32(c, (int32_t)(0))) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 2)) return 0;
  if (!bi32(c, (int32_t)(8))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 1)) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bi32(c, (int32_t)(7))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bi32(c, -8)) return 0;
  if (!bu8(c, 0x71)) return 0;
  if (!bu8(c, 0x36) || !bu8(c, 0x02) || !bleu(c, 0)) return 0;
  /* zero 8 payload bytes so short-key i64 compare is stable */
  if (!bu8(c, 0x20) || !bleu(c, 2)) return 0;
  if (!bi32(c, (int32_t)(8))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x42) || !blei64(c, 0)) return 0;
  if (!bu8(c, 0x37) || !bu8(c, 0x00) || !bleu(c, 0)) return 0;
  if (!bi32(c, (int32_t)(0))) return 0;
  if (!bu8(c, 0x21) || !bleu(c, 3)) return 0;
  if (!bu8(c, 0x02) || !bu8(c, 0x40)) return 0;
  if (!bu8(c, 0x03) || !bu8(c, 0x40)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 3)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 1)) return 0;
  if (!bu8(c, 0x4f)) return 0;
  if (!bu8(c, 0x0d) || !bleu(c, 1)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 2)) return 0;
  if (!bi32(c, (int32_t)(8))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 3)) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 0)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 3)) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x2d) || !bu8(c, 0x00) || !bleu(c, 0)) return 0;
  if (!bu8(c, 0x3a) || !bu8(c, 0x00) || !bleu(c, 0)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 3)) return 0;
  if (!bi32(c, (int32_t)(1))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x21) || !bleu(c, 3)) return 0;
  if (!bu8(c, 0x0c) || !bleu(c, 0)) return 0;
  if (!bu8(c, 0x0b) || !bu8(c, 0x0b)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 2)) return 0;
  return bu8(c, 0x0b);
}

/* mk_dict like mk_list but 8-byte pairs */
static int mk_dict_body(Buf *c) {
  if (!bleu(c, 1) || !bleu(c, 2) || !bu8(c, 0x7f)) return 0;
  if (!bi32(c, (int32_t)(0))) return 0;
  if (!bu8(c, 0x28) || !bu8(c, 0x02) || !bleu(c, 0)) return 0;
  if (!bu8(c, 0x22) || !bleu(c, 1)) return 0;
  if (!bu8(c, 0x45)) return 0;
  if (!bu8(c, 0x04) || !bu8(c, 0x40)) return 0;
  if (!bi32(c, (int32_t)(HEAP0))) return 0;
  if (!bu8(c, 0x21) || !bleu(c, 1)) return 0;
  if (!bu8(c, 0x0b)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 1)) return 0;
  if (!bi32(c, (int32_t)(TAG_DICT))) return 0;
  if (!bu8(c, 0x3a) || !bu8(c, 0x00) || !bleu(c, 0)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 1)) return 0;
  if (!bi32(c, (int32_t)(4))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 0)) return 0;
  if (!bu8(c, 0x36) || !bu8(c, 0x02) || !bleu(c, 0)) return 0; /* i32.store len */
  if (!bi32(c, (int32_t)(0))) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 1)) return 0;
  if (!bi32(c, (int32_t)(8))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 0)) return 0;
  if (!bi32(c, (int32_t)(3))) return 0;
  if (!bu8(c, 0x74)) return 0; /* n<<3 */
  if (!bu8(c, 0x6a)) return 0;
  if (!bi32(c, (int32_t)(7))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bi32(c, -8)) return 0;
  if (!bu8(c, 0x71)) return 0;
  if (!bu8(c, 0x36) || !bu8(c, 0x02) || !bleu(c, 0)) return 0;
  if (!bi32(c, (int32_t)(0))) return 0;
  if (!bu8(c, 0x21) || !bleu(c, 2)) return 0;
  if (!bu8(c, 0x02) || !bu8(c, 0x40)) return 0;
  if (!bu8(c, 0x03) || !bu8(c, 0x40)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 2)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 0)) return 0;
  if (!bu8(c, 0x4f)) return 0;
  if (!bu8(c, 0x0d) || !bleu(c, 1)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 1)) return 0;
  if (!bi32(c, (int32_t)(8))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 2)) return 0;
  if (!bi32(c, (int32_t)(3))) return 0;
  if (!bu8(c, 0x74)) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bi32(c, (int32_t)(0))) return 0;
  if (!bu8(c, 0x36) || !bu8(c, 0x02) || !bleu(c, 0)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 1)) return 0;
  if (!bi32(c, (int32_t)(8))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 2)) return 0;
  if (!bi32(c, (int32_t)(3))) return 0;
  if (!bu8(c, 0x74)) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bi32(c, (int32_t)(4))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bi32(c, (int32_t)(0))) return 0;
  if (!bu8(c, 0x36) || !bu8(c, 0x02) || !bleu(c, 0)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 2)) return 0;
  if (!bi32(c, (int32_t)(1))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x21) || !bleu(c, 2)) return 0;
  if (!bu8(c, 0x0c) || !bleu(c, 0)) return 0;
  if (!bu8(c, 0x0b) || !bu8(c, 0x0b)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 1)) return 0;
  return bu8(c, 0x0b);
}

static int gstore_body(Buf *c, int kind) {
  /* (param val, slot) — kind 0=i64 1=f64 2=i32-handle; slots are i32 handles at GBASE+slot*4 */
  if (!bleu(c, 0)) return 0;
  if (!bi32(c, (int32_t)(GBASE))) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 1)) return 0;
  if (!bi32(c, (int32_t)(2))) return 0;
  if (!bu8(c, 0x74)) return 0; /* slot<<2 */
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 0)) return 0;
  if (kind == 0) {
    /* legacy: store i64 raw — unused for handle path */
    if (!bu8(c, 0x37) || !bu8(c, 0x03) || !bleu(c, 0)) return 0;
  } else if (kind == 1) {
    if (!bu8(c, 0x39) || !bu8(c, 0x03) || !bleu(c, 0)) return 0;
  } else {
    if (!bu8(c, 0x36) || !bu8(c, 0x02) || !bleu(c, 0)) return 0;
  }
  return bu8(c, 0x0b);
}

static int dict_set_hkvi_body(Buf *c) {
  /* (param h k v i) store key at h+8+i*8, val at +4 */
  if (!bleu(c, 0)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 0)) return 0;
  if (!bi32(c, (int32_t)(8))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 3)) return 0;
  if (!bi32(c, (int32_t)(3))) return 0;
  if (!bu8(c, 0x74)) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 1)) return 0;
  if (!bu8(c, 0x36) || !bu8(c, 0x02) || !bleu(c, 0)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 0)) return 0;
  if (!bi32(c, (int32_t)(8))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 3)) return 0;
  if (!bi32(c, (int32_t)(3))) return 0;
  if (!bu8(c, 0x74)) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bi32(c, (int32_t)(4))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 2)) return 0;
  if (!bu8(c, 0x36) || !bu8(c, 0x02) || !bleu(c, 0)) return 0;
  return bu8(c, 0x0b);
}

static int dict_get_body(Buf *c) {
  /* (param d key)(result i32)(local n i k) — linear scan, str eq by len+bytes */
  if (!bleu(c, 1) || !bleu(c, 3) || !bu8(c, 0x7f)) return 0;
  /* n = len_of(d) */
  if (!bu8(c, 0x20) || !bleu(c, 0)) return 0;
  if (!bi32(c, (int32_t)(4))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x28) || !bu8(c, 0x02) || !bleu(c, 0)) return 0;
  if (!bu8(c, 0x21) || !bleu(c, 2)) return 0;
  if (!bi32(c, (int32_t)(0))) return 0;
  if (!bu8(c, 0x21) || !bleu(c, 3)) return 0;
  if (!bu8(c, 0x02) || !bu8(c, 0x40)) return 0;
  if (!bu8(c, 0x03) || !bu8(c, 0x40)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 3)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 2)) return 0;
  if (!bu8(c, 0x4f)) return 0;
  if (!bu8(c, 0x0d) || !bleu(c, 1)) return 0;
  /* k = load key handle */
  if (!bu8(c, 0x20) || !bleu(c, 0)) return 0;
  if (!bi32(c, (int32_t)(8))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 3)) return 0;
  if (!bi32(c, (int32_t)(3))) return 0;
  if (!bu8(c, 0x74)) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x28) || !bu8(c, 0x02) || !bleu(c, 0)) return 0;
  if (!bu8(c, 0x21) || !bleu(c, 4)) return 0;
  /* compare len */
  if (!bu8(c, 0x20) || !bleu(c, 4)) return 0;
  if (!bi32(c, (int32_t)(4))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x28) || !bu8(c, 0x02) || !bleu(c, 0)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 1)) return 0;
  if (!bi32(c, (int32_t)(4))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x28) || !bu8(c, 0x02) || !bleu(c, 0)) return 0;
  if (!bu8(c, 0x46)) return 0; /* i32.eq */
  if (!bu8(c, 0x04) || !bu8(c, 0x40)) return 0;
  /* byte-compare: for short keys, compare first 8 bytes as i64 if len<=8 */
  if (!bu8(c, 0x20) || !bleu(c, 4)) return 0;
  if (!bi32(c, (int32_t)(8))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x29) || !bu8(c, 0x00) || !bleu(c, 0)) return 0; /* i64.load align0 */
  if (!bu8(c, 0x20) || !bleu(c, 1)) return 0;
  if (!bi32(c, (int32_t)(8))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x29) || !bu8(c, 0x00) || !bleu(c, 0)) return 0;
  if (!bu8(c, 0x51)) return 0; /* i64.eq */
  if (!bu8(c, 0x04) || !bu8(c, 0x7f)) return 0; /* if result i32 */
  /* return val */
  if (!bu8(c, 0x20) || !bleu(c, 0)) return 0;
  if (!bi32(c, (int32_t)(8))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 3)) return 0;
  if (!bi32(c, (int32_t)(3))) return 0;
  if (!bu8(c, 0x74)) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bi32(c, (int32_t)(4))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x28) || !bu8(c, 0x02) || !bleu(c, 0)) return 0;
  if (!bu8(c, 0x0f)) return 0; /* return */
  if (!bu8(c, 0x05)) return 0;
  if (!bi32(c, (int32_t)(0))) return 0;
  if (!bu8(c, 0x0b)) return 0; /* end if i64.eq */
  if (!bu8(c, 0x1a)) return 0; /* drop */
  if (!bu8(c, 0x0b)) return 0; /* end if len eq */
  if (!bu8(c, 0x20) || !bleu(c, 3)) return 0;
  if (!bi32(c, (int32_t)(1))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x21) || !bleu(c, 3)) return 0;
  if (!bu8(c, 0x0c) || !bleu(c, 0)) return 0;
  if (!bu8(c, 0x0b) || !bu8(c, 0x0b)) return 0;
  if (!bi32(c, (int32_t)(0))) return 0;
  return bu8(c, 0x0b);
}



static int clear_slots_body(Buf *c, int ng) {
  /* zero ng handle slots; leave freep alone */
  if (!bleu(c, 1) || !bleu(c, 1) || !bu8(c, 0x7f)) return 0;
  if (!bi32(c, 0)) return 0;
  if (!bu8(c, 0x21) || !bleu(c, 0)) return 0;
  if (!bu8(c, 0x02) || !bu8(c, 0x40)) return 0;
  if (!bu8(c, 0x03) || !bu8(c, 0x40)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 0)) return 0;
  if (!bi32(c, (int32_t)(ng > 0 ? ng : 1))) return 0;
  if (!bu8(c, 0x4f)) return 0;
  if (!bu8(c, 0x0d) || !bleu(c, 1)) return 0;
  if (!bi32(c, GBASE)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 0)) return 0;
  if (!bi32(c, 2)) return 0;
  if (!bu8(c, 0x74)) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bi32(c, 0)) return 0;
  if (!bu8(c, 0x36) || !bu8(c, 0x02) || !bleu(c, 0)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 0)) return 0;
  if (!bi32(c, 1)) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x21) || !bleu(c, 0)) return 0;
  if (!bu8(c, 0x0c) || !bleu(c, 0)) return 0;
  if (!bu8(c, 0x0b) || !bu8(c, 0x0b)) return 0;
  if (!bi32(c, 0)) return 0;
  return bu8(c, 0x0b);
}

static int host_reset_body(Buf *c, int ng) {
  /* freep = HEAP0; clear ng global handle slots */
  if (!bleu(c, 1) || !bleu(c, 1) || !bu8(c, 0x7f)) return 0; /* local i */
  if (!bi32(c, (int32_t)(0))) return 0;
  if (!bi32(c, (int32_t)(HEAP0))) return 0;
  if (!bu8(c, 0x36) || !bu8(c, 0x02) || !bleu(c, 0)) return 0;
  if (!bi32(c, (int32_t)(0))) return 0;
  if (!bu8(c, 0x21) || !bleu(c, 0)) return 0;
  if (!bu8(c, 0x02) || !bu8(c, 0x40)) return 0;
  if (!bu8(c, 0x03) || !bu8(c, 0x40)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 0)) return 0;
  if (!bi32(c, (int32_t)(ng > 0 ? ng : 1))) return 0;
  if (!bu8(c, 0x4f)) return 0;
  if (!bu8(c, 0x0d) || !bleu(c, 1)) return 0;
  if (!bi32(c, (int32_t)(GBASE))) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 0)) return 0;
  if (!bi32(c, (int32_t)(2))) return 0;
  if (!bu8(c, 0x74)) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bi32(c, (int32_t)(0))) return 0;
  if (!bu8(c, 0x36) || !bu8(c, 0x02) || !bleu(c, 0)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 0)) return 0;
  if (!bi32(c, (int32_t)(1))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x21) || !bleu(c, 0)) return 0;
  if (!bu8(c, 0x0c) || !bleu(c, 0)) return 0;
  if (!bu8(c, 0x0b) || !bu8(c, 0x0b)) return 0;
  if (!bi32(c, (int32_t)(0))) return 0;
  return bu8(c, 0x0b);
}

static int host_set_global_body(Buf *c, int ng) {
  /* (param i h) — store h at GBASE+i*4 if i < ng */
  if (!bleu(c, 0)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 0)) return 0;
  if (!bi32(c, (int32_t)ng)) return 0;
  if (!bu8(c, 0x4f)) return 0; /* ge_u */
  if (!bu8(c, 0x04) || !bu8(c, 0x40)) return 0;
  if (!bu8(c, 0x0f)) return 0; /* return */
  if (!bu8(c, 0x0b)) return 0;
  if (!bi32(c, (int32_t)(GBASE))) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 0)) return 0;
  if (!bi32(c, (int32_t)(2))) return 0;
  if (!bu8(c, 0x74)) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 1)) return 0;
  if (!bu8(c, 0x36) || !bu8(c, 0x02) || !bleu(c, 0)) return 0;
  return bu8(c, 0x0b);
}

static int host_get_global_body(Buf *c, int ng) {
  if (!bleu(c, 0)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 0)) return 0;
  if (!bi32(c, (int32_t)ng)) return 0;
  if (!bu8(c, 0x4f)) return 0;
  if (!bu8(c, 0x04) || !bu8(c, 0x7f)) return 0;
  if (!bi32(c, (int32_t)(0))) return 0;
  if (!bu8(c, 0x05)) return 0;
  if (!bi32(c, (int32_t)(GBASE))) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 0)) return 0;
  if (!bi32(c, (int32_t)(2))) return 0;
  if (!bu8(c, 0x74)) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x28) || !bu8(c, 0x02) || !bleu(c, 0)) return 0;
  if (!bu8(c, 0x0b)) return 0;
  return bu8(c, 0x0b);
}

static int run_step_body(Buf *c) {
  /* call main (func 1) */
  if (!bleu(c, 0)) return 0;
  if (!bu8(c, 0x10) || !bleu(c, 1)) return 0;
  return bu8(c, 0x0b);
}

static int host_dict_key_body(Buf *c) {
  /* (param h i)(result i32) */
  if (!bleu(c, 0)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 0)) return 0;
  if (!bi32(c, (int32_t)(8))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 1)) return 0;
  if (!bi32(c, (int32_t)(3))) return 0;
  if (!bu8(c, 0x74)) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x28) || !bu8(c, 0x02) || !bleu(c, 0)) return 0;
  return bu8(c, 0x0b);
}

static int host_dict_val_body(Buf *c) {
  if (!bleu(c, 0)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 0)) return 0;
  if (!bi32(c, (int32_t)(8))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 1)) return 0;
  if (!bi32(c, (int32_t)(3))) return 0;
  if (!bu8(c, 0x74)) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bi32(c, (int32_t)(4))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x28) || !bu8(c, 0x02) || !bleu(c, 0)) return 0;
  return bu8(c, 0x0b);
}

static int const_i32_ret_body(Buf *c, uint32_t v) {
  if (!bleu(c, 0)) return 0;
  if (!bi32(c, (int32_t)(v))) return 0;
  return bu8(c, 0x0b);
}

static int str_ptr_body(Buf *c) {
  if (!bleu(c, 0)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 0)) return 0;
  if (!bu8(c, 0x45)) return 0;
  if (!bu8(c, 0x04) || !bu8(c, 0x7f)) return 0;
  if (!bi32(c, (int32_t)(0))) return 0;
  if (!bu8(c, 0x05)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 0)) return 0;
  if (!bi32(c, (int32_t)(8))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x0b)) return 0;
  return bu8(c, 0x0b);
}


static int host_dict_set_body(Buf *c) {
  /* (param h i k v) — emit_wasm / run_step order */
  if (!bleu(c, 0)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 0)) return 0;
  if (!bi32(c, (int32_t)(8))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 1)) return 0;
  if (!bi32(c, (int32_t)(3))) return 0;
  if (!bu8(c, 0x74)) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 2)) return 0;
  if (!bu8(c, 0x36) || !bu8(c, 0x02) || !bleu(c, 0)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 0)) return 0;
  if (!bi32(c, (int32_t)(8))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 1)) return 0;
  if (!bi32(c, (int32_t)(3))) return 0;
  if (!bu8(c, 0x74)) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bi32(c, (int32_t)(4))) return 0;
  if (!bu8(c, 0x6a)) return 0;
  if (!bu8(c, 0x20) || !bleu(c, 3)) return 0;
  if (!bu8(c, 0x36) || !bu8(c, 0x02) || !bleu(c, 0)) return 0;
  return bu8(c, 0x0b);
}

static int build_mod(Prog *P, Buf *mod) {
  Buf types, funcs, mems, exps, codes;
  Buf cs[32];
  int i, ng = P->ng > 0 ? P->ng : 1;
  enum { NCORE = 18, NHOST = 14, NFUNCS = NCORE + NHOST };
  if (!binit(&types,160)||!binit(&funcs,48)||!binit(&mems,16)||!binit(&exps,512)||!binit(&codes,16384))
    return 0;
  for (i = 0; i < NFUNCS; i++) if (!binit(&cs[i], i==1 ? 8192 : 256)) return 0;

  if (!bleu(&types, 14)) return 0;
  if (!bu8(&types,0x60)||!bleu(&types,1)||!bu8(&types,0x7e)||!bleu(&types,1)||!bu8(&types,0x7f)) return 0;
  if (!bu8(&types,0x60)||!bleu(&types,0)||!bleu(&types,1)||!bu8(&types,0x7f)) return 0;
  if (!bu8(&types,0x60)||!bleu(&types,1)||!bu8(&types,0x7f)||!bleu(&types,1)||!bu8(&types,0x7f)) return 0;
  if (!bu8(&types,0x60)||!bleu(&types,1)||!bu8(&types,0x7f)||!bleu(&types,1)||!bu8(&types,0x7e)) return 0;
  if (!bu8(&types,0x60)||!bleu(&types,1)||!bu8(&types,0x7c)||!bleu(&types,1)||!bu8(&types,0x7f)) return 0;
  if (!bu8(&types,0x60)||!bleu(&types,1)||!bu8(&types,0x7f)||!bleu(&types,1)||!bu8(&types,0x7c)) return 0;
  if (!bu8(&types,0x60)||!bleu(&types,2)||!bu8(&types,0x7f)||!bu8(&types,0x7f)||!bleu(&types,1)||!bu8(&types,0x7f)) return 0;
  if (!bu8(&types,0x60)||!bleu(&types,3)||!bu8(&types,0x7f)||!bu8(&types,0x7f)||!bu8(&types,0x7f)||!bleu(&types,0)) return 0;
  if (!bu8(&types,0x60)||!bleu(&types,2)||!bu8(&types,0x7f)||!bu8(&types,0x7f)||!bleu(&types,1)||!bu8(&types,0x7f)) return 0;
  if (!bu8(&types,0x60)||!bleu(&types,2)||!bu8(&types,0x7e)||!bu8(&types,0x7f)||!bleu(&types,0)) return 0;
  if (!bu8(&types,0x60)||!bleu(&types,2)||!bu8(&types,0x7c)||!bu8(&types,0x7f)||!bleu(&types,0)) return 0;
  if (!bu8(&types,0x60)||!bleu(&types,2)||!bu8(&types,0x7f)||!bu8(&types,0x7f)||!bleu(&types,0)) return 0;
  if (!bu8(&types,0x60)||!bleu(&types,4)||!bu8(&types,0x7f)||!bu8(&types,0x7f)||!bu8(&types,0x7f)||!bu8(&types,0x7f)||!bleu(&types,0)) return 0;
  if (!bu8(&types,0x60)||!bleu(&types,2)||!bu8(&types,0x7f)||!bu8(&types,0x7f)||!bleu(&types,1)||!bu8(&types,0x7f)) return 0;

  if (!bleu(&funcs, NFUNCS)) return 0;
  uint8_t fty[NFUNCS] = {
    0,1,2,3,4,5,2,6,7,7,2, 8,2, 9,10,11, 12,13,
    1, 1, 1, 11, 2, 6, 6, 1, 1, 1, 2, 2,
    12, /* 30 host_dict_set (h,i,k,v) */
    1  /* 31 host_mk_null */
  };
  for (i = 0; i < NFUNCS; i++) if (!bu8(&funcs, fty[i])) return 0;
  if (!bleu(&mems,1)||!bu8(&mems,0)||!bleu(&mems, MEM_PAGES)) return 0;

  /* exports */
  if (!bleu(&exps, 27)) return 0;
  #define EXP(name, kind, idx) do { \
    if (!bstr(&exps, name)||!bu8(&exps, kind)||!bleu(&exps, idx)) return 0; \
  } while (0)
  EXP("memory", 0x02, 0);
  EXP("main_export", 0x00, 1);
  EXP("tag_of_export", 0x00, 2);
  EXP("i64_of_export", 0x00, 3);
  EXP("f64_of_export", 0x00, 5);
  EXP("run_step", 0x00, 19);
  EXP("host_run", 0x00, 20);
  EXP("host_reset", 0x00, 18);
  EXP("clear_slots", 0x00, 27);
  EXP("host_set_global", 0x00, 21);
  EXP("host_get_global", 0x00, 22);
  EXP("host_mk_i64", 0x00, 0);
  EXP("host_mk_f64", 0x00, 4);
  EXP("host_mk_list", 0x00, 6);
  EXP("host_list_get", 0x00, 7);
  EXP("host_list_set", 0x00, 9);
  EXP("host_len", 0x00, 10);
  EXP("host_mk_str", 0x00, 11);
  EXP("host_mk_dict", 0x00, 12);
  EXP("host_dict_set", 0x00, 30);
  EXP("host_dict_key", 0x00, 23);
  EXP("host_dict_val", 0x00, 24);
  EXP("host_scratch", 0x00, 25);
  EXP("mem_base", 0x00, 26);
  EXP("str_ptr_export", 0x00, 28);
  EXP("str_len_export", 0x00, 29);
  EXP("host_mk_null", 0x00, 31);
  #undef EXP

  if (!mk_num_body(&cs[0], 0)) return 0;
  if (!emit_locals(&cs[1], P)) return 0;
  if (!emit_ops(&cs[1], P)) return 0;
  if (!bu8(&cs[1], 0x00) || !bu8(&cs[1], 0x0b)) return 0;
  if (!bleu(&cs[2],0)) return 0;
  if (!bu8(&cs[2],0x20)||!bleu(&cs[2],0)) return 0;
  if (!bu8(&cs[2],0x2d)||!bu8(&cs[2],0x00)||!bleu(&cs[2],0)||!bu8(&cs[2],0x0b)) return 0;
  if (!bleu(&cs[3],0)) return 0;
  if (!bu8(&cs[3],0x20)||!bleu(&cs[3],0)) return 0;
  if (!bu8(&cs[3],0x41)||!bleu(&cs[3],8)||!bu8(&cs[3],0x6a)) return 0;
  if (!bu8(&cs[3],0x29)||!bu8(&cs[3],0x03)||!bleu(&cs[3],0)||!bu8(&cs[3],0x0b)) return 0;
  if (!mk_num_body(&cs[4], 1)) return 0;
  /* as_f64: i64 handles coerce (JSON 3.0 → host_mk_i64) */
  if (!bleu(&cs[5],0)) return 0;
  if (!bu8(&cs[5],0x20)||!bleu(&cs[5],0)) return 0; /* h */
  if (!bu8(&cs[5],0x2d)||!bu8(&cs[5],0x00)||!bleu(&cs[5],0)) return 0; /* load8_u tag */
  if (!bi32(&cs[5], TAG_F64)) return 0;
  if (!bu8(&cs[5],0x46)) return 0; /* i32.eq */
  if (!bu8(&cs[5],0x04)||!bu8(&cs[5],0x7c)) return 0; /* if f64 */
  if (!bu8(&cs[5],0x20)||!bleu(&cs[5],0)) return 0;
  if (!bi32(&cs[5], 8)) return 0;
  if (!bu8(&cs[5],0x6a)) return 0;
  if (!bu8(&cs[5],0x2b)||!bu8(&cs[5],0x03)||!bleu(&cs[5],0)) return 0; /* f64.load */
  if (!bu8(&cs[5],0x05)) return 0;
  if (!bu8(&cs[5],0x20)||!bleu(&cs[5],0)) return 0;
  if (!bi32(&cs[5], 8)) return 0;
  if (!bu8(&cs[5],0x6a)) return 0;
  if (!bu8(&cs[5],0x29)||!bu8(&cs[5],0x03)||!bleu(&cs[5],0)) return 0; /* i64.load */
  if (!bu8(&cs[5],0xb9)) return 0; /* f64.convert_i64_s */
  if (!bu8(&cs[5],0x0b)) return 0; /* end if */
  if (!bu8(&cs[5],0x0b)) return 0;
  if (!mk_list_body(&cs[6])) return 0;
  if (!list_get_body(&cs[7])) return 0;
  if (!list_set_body(&cs[8], 1)) return 0;
  if (!list_set_body(&cs[9], 0)) return 0;
  if (!len_of_body(&cs[10])) return 0;
  if (!mk_str_body(&cs[11])) return 0;
  if (!mk_dict_body(&cs[12])) return 0;
  if (!gstore_body(&cs[13], 0)) return 0;
  if (!gstore_body(&cs[14], 1)) return 0;
  if (!gstore_body(&cs[15], 2)) return 0;
  if (!dict_set_hkvi_body(&cs[16])) return 0;
  if (!dict_get_body(&cs[17])) return 0;

  if (!host_reset_body(&cs[18], ng)) return 0;
  if (!run_step_body(&cs[19])) return 0;
  if (!run_step_body(&cs[20])) return 0; /* host_run */
  if (!host_set_global_body(&cs[21], ng)) return 0;
  if (!host_get_global_body(&cs[22], ng)) return 0;
  if (!host_dict_key_body(&cs[23])) return 0;
  if (!host_dict_val_body(&cs[24])) return 0;
  if (!const_i32_ret_body(&cs[25], HOST_SCRATCH)) return 0;
  if (!const_i32_ret_body(&cs[26], 0)) return 0; /* mem_base */
  if (!clear_slots_body(&cs[27], ng)) return 0;
  if (!str_ptr_body(&cs[28])) return 0;
  if (!len_of_body(&cs[29])) return 0;
  if (!host_dict_set_body(&cs[30])) return 0;
  if (!const_i32_ret_body(&cs[31], 0)) return 0; /* host_mk_null */

  if (!bleu(&codes, NFUNCS)) return 0;
  for (i = 0; i < NFUNCS; i++) {
    if (!bleu(&codes, cs[i].n) || !braw(&codes, bd(&cs[i]), cs[i].n)) return 0;
  }
  if (!binit(mod, 32768)) return 0;
  static const uint8_t hdr[] = {0,0x61,0x73,0x6d,1,0,0,0};
  if (!braw(mod, hdr, 8)) return 0;
  if (!sect(mod,1,&types)) return 0;
  if (!sect(mod,3,&funcs)) return 0;
  if (!sect(mod,5,&mems)) return 0;
  if (!sect(mod,7,&exps)) return 0;
  if (!sect(mod,10,&codes)) return 0;
  return 1;
}

static int write_meta(Prog *P) {
  char buf[512];
  uint32_t n = 0;
  int first;
  buf[n++] = '{';
  memcpy(buf + n, "\"locals\":[", 10); n += 10;
  first = 1;
  for (int i = 0; i < P->nl; i++) {
    if (!P->names[i][0]) continue;
    if (!first) buf[n++] = ',';
    first = 0;
    buf[n++] = '"';
    for (const char *p = P->names[i]; *p; p++) buf[n++] = *p;
    buf[n++] = '"';
  }
  memcpy(buf + n, "],\"globals\":[", 13); n += 13;
  first = 1;
  for (int i = 0; i < P->ng; i++) {
    if (!first) buf[n++] = ',';
    first = 0;
    buf[n++] = '"';
    for (const char *p = P->gnames[i]; *p; p++) buf[n++] = *p;
    buf[n++] = '"';
  }
  memcpy(buf + n, "],\"slots\":[", 11); n += 11;
  first = 1;
  for (int i = 0; i < P->nl; i++) {
    if (!P->names[i][0]) continue;
    if (!first) buf[n++] = ',';
    first = 0;
    buf[n++] = '"';
    for (const char *p = P->names[i]; *p; p++) buf[n++] = *p;
    buf[n++] = '"';
  }
  buf[n++] = ']'; buf[n++] = '}'; buf[n] = 0;
  uint32_t o = hall(n + 1);
  if (!o) return 0;
  memcpy(g_heap + o, buf, n + 1);
  g_meta_ptr = poff(o); g_meta_len = n;
  return 1;
}

__attribute__((export_name("compile")))
int32_t compile(uint32_t src_ptr, uint32_t src_len) {
  g_bump = 64;
  g_out_ptr = g_out_len = g_meta_ptr = g_meta_len = g_err_ptr = g_err_len = 0;
  if (src_len > 100000) { set_err("src too large"); return 1; }
  const char *src = (const char *)(uintptr_t)src_ptr;

  Prog P; memset(&P, 0, sizeof P); P.scratch = -1; P.fscratch = -1;
  Lex L; L.src = src; L.len = src_len; L.pos = 0;
  next(&L);
  while (L.tok != T_EOF) {
    if (!pstmt(&L, &P)) { set_err(P.err ? P.err : "parse"); return 1; }
  }
  Buf mod;
  if (!build_mod(&P, &mod)) { set_err("emit"); return 1; }
  g_out_ptr = poff(mod.off); g_out_len = mod.n;
  if (!write_meta(&P)) { set_err("meta"); return 1; }
  return 0;
}
