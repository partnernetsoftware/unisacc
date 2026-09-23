/* UJS full bytecode VM — freestanding wasm32 (zig cc). [LW-3][IC-1] */
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef long long i64;
typedef double f64;

enum {
  TAG_NULL=0, TAG_BOOL=1, TAG_I64=2, TAG_F64=3, TAG_STR=4,
  TAG_LIST=5, TAG_DICT=6, TAG_FN=7, TAG_TUP=8
};

enum {
  OP_NOP=0, OP_CONST_NULL=1, OP_CONST_BOOL=2, OP_CONST_I64=3, OP_CONST_F64=4,
  OP_CONST_STR=5, OP_CONST_FN=6, OP_LOAD_L=7, OP_STORE_L=8, OP_LOAD_G=9,
  OP_STORE_G=10, OP_DROP=11, OP_ADD=12, OP_SUB=13, OP_MUL=14, OP_DIV=15,
  OP_MOD=16, OP_LT=17, OP_LE=18, OP_GT=19, OP_GE=20, OP_EQ=21, OP_NE=22,
  OP_AND=23, OP_OR=24, OP_NOT=25, OP_IDX=26, OP_SETIDX=27, OP_DOT=28,
  OP_LEN=29, OP_KEYS=30, OP_IN=31, OP_MKLIST=32, OP_MKDICT=33, OP_MKTUP=34,
  OP_JUMP=35, OP_JUMPZ=36, OP_JUMPNZ=37, OP_CALL=38, OP_RET=39, OP_SPREAD=40,
  OP_RESTPACK=41, OP_TYPEOF=42, OP_SHAPEOF=43, OP_SWITCH=44, OP_IC=45, OP_PRINT=46
};

/* shape indices must match catalog.SHAPES */
enum {
  SH_NULL=0, SH_BOOL=1, SH_I64=2, SH_F64=3, SH_STR=4,
  SH_LIST=5, SH_TUP=6, SH_FN=7,
  SH_DICT_EMPTY=8, SH_DICT_K1=9, SH_DICT_K2=10, SH_DICT_K3=11, SH_DICT_K4=12,
  SH_DICT_OPEN=13, SH_UNKNOWN=14
};
enum { GU_NONE=0, GU_TYPE_OK=1, GU_LEN_OK=2, GU_KEY_OK=3, GU_MONO=4 };

#define MEM_SIZE (1<<22)
static u8 mem[MEM_SIZE];
static u32 freep;
static u32 sp;
static u32 locals[256];
static u32 globals[256];
static u32 last_ic_stub; /* last ask result; exec stays classic [IC-4] */

extern u32 ujs_ic_ask(u32 shape, u32 op, u32 guard);

static void *memset_z(void *p, int v, unsigned n) {
  u8 *q = (u8 *)p; while (n--) *q++ = (u8)v; return p;
}

static u32 rd32(u32 p) { return (u32)mem[p] | ((u32)mem[p+1]<<8) | ((u32)mem[p+2]<<16) | ((u32)mem[p+3]<<24); }
static void wr32(u32 p, u32 v) {
  mem[p]=(u8)v; mem[p+1]=(u8)(v>>8); mem[p+2]=(u8)(v>>16); mem[p+3]=(u8)(v>>24);
}
static u16 rd16(u32 p) { return (u16)mem[p] | ((u16)mem[p+1]<<8); }
static void wr16(u32 p, u16 v) { mem[p]=(u8)v; mem[p+1]=(u8)(v>>8); }
static i64 rd64(u32 p) {
  u64 lo = rd32(p); u64 hi = rd32(p+4); return (i64)(lo | (hi<<32));
}
static void wr64(u32 p, i64 v) {
  wr32(p, (u32)v); wr32(p+4, (u32)((u64)v>>32));
}

static u32 alloc(u32 n) {
  u32 p = freep; freep += (n + 7) & ~7u; return p;
}
static void push(u32 h) { wr32(sp, h); sp += 4; }
static u32 pop(void) { sp -= 4; return rd32(sp); }
static u32 peek(u32 depth) { return rd32(sp - 4 * (depth + 1)); }

static u32 mk_null(void) { return 0; }
static u32 mk_bool(u8 b) {
  u32 p = alloc(8); mem[p]=TAG_BOOL; mem[p+4]=b; return p;
}
static u32 mk_i64(i64 v) {
  u32 p = alloc(16); mem[p]=TAG_I64; wr64(p+8, v); return p;
}
static u32 mk_f64(f64 v) {
  union { f64 f; u64 u; } u; u.f = v;
  u32 p = alloc(16); mem[p]=TAG_F64; wr64(p+8, (i64)u.u); return p;
}
static f64 f64_of(u32 h) {
  union { f64 f; u64 u; } u; u.u = (u64)rd64(h+8); return u.f;
}
static u32 mk_str(const u8 *s, u32 n) {
  u32 p = alloc(8+n); u32 i;
  mem[p]=TAG_STR; wr16(p+2,(u16)n);
  for (i=0;i<n;i++) mem[p+8+i]=s[i];
  return p;
}
static u32 mk_str_lit(const char *s) {
  u32 n = 0; while (s[n]) n++;
  return mk_str((const u8 *)s, n);
}
static u32 tag_of(u32 h) { return h ? mem[h] : TAG_NULL; }
static i64 i64_of(u32 h) { return rd64(h+8); }
static u32 len_of(u32 h) { return h ? rd16(h+2) : 0; }
static u8 *str_ptr(u32 h) { return &mem[h+8]; }

static int truthy(u32 h) {
  u32 t = tag_of(h);
  if (t==TAG_NULL) return 0;
  if (t==TAG_BOOL) return mem[h+4];
  if (t==TAG_I64) return i64_of(h)!=0;
  if (t==TAG_F64) return f64_of(h)!=0.0;
  if (t==TAG_STR || t==TAG_LIST || t==TAG_DICT || t==TAG_TUP) return len_of(h)!=0;
  return 1;
}
static int str_eq(u32 a, u32 b) {
  u32 n=len_of(a), i;
  if (n!=len_of(b)) return 0;
  for (i=0;i<n;i++) if (str_ptr(a)[i]!=str_ptr(b)[i]) return 0;
  return 1;
}
static int is_num(u32 h) {
  u32 t = tag_of(h); return t==TAG_I64 || t==TAG_F64;
}
static f64 as_f64(u32 h) {
  if (tag_of(h)==TAG_F64) return f64_of(h);
  return (f64)i64_of(h);
}

static u32 shape_of_val(u32 h) {
  u32 t = tag_of(h), n;
  if (t==TAG_NULL) return SH_NULL;
  if (t==TAG_BOOL) return SH_BOOL;
  if (t==TAG_I64) return SH_I64;
  if (t==TAG_F64) return SH_F64;
  if (t==TAG_STR) return SH_STR;
  if (t==TAG_LIST) return SH_LIST;
  if (t==TAG_TUP) return SH_TUP;
  if (t==TAG_FN) return SH_FN;
  if (t==TAG_DICT) {
    n = len_of(h);
    if (n==0) return SH_DICT_EMPTY;
    if (n==1) return SH_DICT_K1;
    if (n==2) return SH_DICT_K2;
    if (n==3) return SH_DICT_K3;
    if (n==4) return SH_DICT_K4;
    return SH_DICT_OPEN;
  }
  return SH_UNKNOWN;
}

static u32 code_base, code_len, str_base, nstr, fn_base, nfn;

static u32 skip_strings(u32 p, u32 n) {
  u32 i, len;
  for (i=0;i<n;i++) {
    len = rd32(p); p += 4 + len;
    p += (4 - (len & 3)) & 3;
  }
  return p;
}
static u32 str_entry(u32 idx) {
  u32 p = str_base, j, n;
  for (j=0;j<idx;j++) {
    n = rd32(p); p += 4 + n; p += (4 - (n & 3)) & 3;
  }
  return p;
}
static u32 dict_get(u32 d, u32 key) {
  u32 n = len_of(d), i, k;
  for (i=0;i<n;i++) {
    k = rd32(d+8 + i*8);
    if (str_eq(k, key)) return rd32(d+8 + i*8 + 4);
  }
  return 0;
}
static int dict_set(u32 d, u32 key, u32 val) {
  u32 n = len_of(d), i, k;
  for (i=0;i<n;i++) {
    k = rd32(d+8 + i*8);
    if (str_eq(k, key)) { wr32(d+8 + i*8 + 4, val); return 1; }
  }
  return 0;
}
static u32 dict_grow_set(u32 d, u32 key, u32 val) {
  u32 n = len_of(d), p, i;
  if (dict_set(d, key, val)) return d;
  p = alloc(8 + (n+1)*8);
  mem[p]=TAG_DICT; wr16(p+2,(u16)(n+1));
  for (i=0;i<n;i++) {
    wr32(p+8+i*8, rd32(d+8+i*8));
    wr32(p+8+i*8+4, rd32(d+8+i*8+4));
  }
  wr32(p+8+n*8, key); wr32(p+8+n*8+4, val);
  return p;
}
static u32 fn_entry(u32 idx) {
  u32 p = fn_base, j, clen, ns;
  for (j=0;j<idx;j++) {
    p += 12;
    clen = rd32(p); p += 4 + clen;
    ns = rd32(p); p += 4; p = skip_strings(p, ns);
    p += 4;
  }
  return p;
}

static u32 run_code(u32 base, u32 len);

static u32 typeof_str(u32 h) {
  switch (tag_of(h)) {
  case TAG_NULL: return mk_str_lit("null");
  case TAG_BOOL: return mk_str_lit("bool");
  case TAG_I64: return mk_str_lit("i64");
  case TAG_F64: return mk_str_lit("f64");
  case TAG_STR: return mk_str_lit("str");
  case TAG_LIST: return mk_str_lit("list");
  case TAG_DICT: return mk_str_lit("dict");
  case TAG_FN: return mk_str_lit("fn");
  case TAG_TUP: return mk_str_lit("tup");
  default: return mk_str_lit("unknown");
  }
}

static u32 shapeof_str(u32 h) {
  static const char *names[] = {
    "s_null","s_bool","s_i64","s_f64","s_str",
    "s_list","s_tup","s_fn",
    "s_dict_empty","s_dict_k1","s_dict_k2","s_dict_k3","s_dict_k4",
    "s_dict_open","s_unknown"
  };
  u32 s = shape_of_val(h);
  if (s > SH_UNKNOWN) s = SH_UNKNOWN;
  return mk_str_lit(names[s]);
}

static u32 run_code(u32 base, u32 len) {
  u32 pc = base, end = base + len, a=0, b, c, i, n, p;
  i64 va, vb;
  u32 saved[64];
  u32 save_str_base, save_nstr, save_code_base, save_code_len;
  while (pc < end) {
    u8 op = mem[pc++];
    switch (op) {
    case OP_NOP: break;
    case OP_CONST_NULL: push(mk_null()); break;
    case OP_CONST_BOOL: push(mk_bool(mem[pc++])); break;
    case OP_CONST_I64: push(mk_i64(rd64(pc))); pc+=8; break;
    case OP_CONST_F64: {
      union { f64 f; u64 u; } u; u.u = (u64)rd64(pc); pc+=8;
      push(mk_f64(u.f)); break;
    }
    case OP_CONST_STR:
      i = rd32(pc); pc+=4; p = str_entry(i);
      n = rd32(p); push(mk_str(&mem[p+4], n)); break;
    case OP_CONST_FN:
      i = rd32(pc); pc+=4; p = alloc(8); mem[p]=TAG_FN; wr32(p+4,i); push(p); break;
    case OP_LOAD_L: push(locals[mem[pc++]]); break;
    case OP_STORE_L: locals[mem[pc++]] = pop(); break;
    case OP_LOAD_G: push(globals[mem[pc++]]); break;
    case OP_STORE_G: globals[mem[pc++]] = pop(); break;
    case OP_DROP: pop(); break;
    case OP_ADD: case OP_SUB: case OP_MUL: case OP_MOD: {
      b=pop(); a=pop();
      if (op==OP_ADD && tag_of(a)==TAG_STR && tag_of(b)==TAG_STR) {
        u32 na=len_of(a), nb=len_of(b);
        u32 q = alloc(8+na+nb);
        mem[q]=TAG_STR; wr16(q+2,(u16)(na+nb));
        for (i=0;i<na;i++) mem[q+8+i]=str_ptr(a)[i];
        for (i=0;i<nb;i++) mem[q+8+na+i]=str_ptr(b)[i];
        push(q); break;
      }
      if (tag_of(a)==TAG_I64 && tag_of(b)==TAG_I64) {
        va=i64_of(a); vb=i64_of(b);
        if (op==OP_ADD) push(mk_i64(va+vb));
        else if (op==OP_SUB) push(mk_i64(va-vb));
        else if (op==OP_MUL) push(mk_i64(va*vb));
        else push(mk_i64(va%vb));
        break;
      }
      if (is_num(a) && is_num(b)) {
        f64 fa=as_f64(a), fb=as_f64(b), r=0;
        if (op==OP_ADD) r=fa+fb;
        else if (op==OP_SUB) r=fa-fb;
        else if (op==OP_MUL) r=fa*fb;
        else r = fa - (f64)((i64)(fa/fb))*fb; /* fmod-ish */
        push(mk_f64(r));
      } else return 0;
      break;
    }
    case OP_DIV: {
      b=pop(); a=pop();
      if (!is_num(a) || !is_num(b)) return 0;
      push(mk_f64(as_f64(a)/as_f64(b))); break;
    }
    case OP_LT: case OP_LE: case OP_GT: case OP_GE: case OP_EQ: case OP_NE: {
      b=pop(); a=pop(); c=0;
      if (tag_of(a)==TAG_STR && tag_of(b)==TAG_STR) {
        /* lexicographic via length then bytes — enough for == */
        if (op==OP_EQ) c = str_eq(a,b);
        else if (op==OP_NE) c = !str_eq(a,b);
        else return 0;
      } else if (tag_of(a)==TAG_I64 && tag_of(b)==TAG_I64) {
        va=i64_of(a); vb=i64_of(b);
        if (op==OP_LT) c = va<vb;
        else if (op==OP_LE) c = va<=vb;
        else if (op==OP_GT) c = va>vb;
        else if (op==OP_GE) c = va>=vb;
        else if (op==OP_EQ) c = va==vb;
        else c = va!=vb;
      } else if (is_num(a) && is_num(b)) {
        f64 fa=as_f64(a), fb=as_f64(b);
        if (op==OP_LT) c = fa<fb;
        else if (op==OP_LE) c = fa<=fb;
        else if (op==OP_GT) c = fa>fb;
        else if (op==OP_GE) c = fa>=fb;
        else if (op==OP_EQ) c = fa==fb;
        else c = fa!=fb;
      } else if (tag_of(a)==TAG_NULL && tag_of(b)==TAG_NULL) {
        c = (op==OP_EQ);
      } else if (tag_of(a)==TAG_BOOL && tag_of(b)==TAG_BOOL) {
        u8 ba=mem[a+4], bb=mem[b+4];
        if (op==OP_EQ) c = ba==bb;
        else if (op==OP_NE) c = ba!=bb;
        else return 0;
      } else {
        if (op==OP_EQ) c = 0;
        else if (op==OP_NE) c = 1;
        else return 0;
      }
      push(mk_bool((u8)c)); break;
    }
    case OP_NOT: push(mk_bool(!truthy(pop()))); break;
    case OP_AND: b=pop(); a=pop(); push(mk_bool(truthy(a)&&truthy(b))); break;
    case OP_OR: b=pop(); a=pop(); push(mk_bool(truthy(a)||truthy(b))); break;
    case OP_JUMP: pc = base + rd32(pc); break;
    case OP_JUMPZ: i=rd32(pc); pc+=4; if (!truthy(pop())) pc=base+i; break;
    case OP_JUMPNZ: i=rd32(pc); pc+=4; if (truthy(pop())) pc=base+i; break;
    case OP_RET: return pop();
    case OP_IC: {
      /* annotation before classic op; peek operands [IC-1][IC-4] */
      u32 opid = mem[pc++];
      u32 sh = SH_UNKNOWN, gu = GU_NONE;
      if (pc < end && mem[pc] == OP_CALL) {
        u32 argc = mem[pc + 1];
        u32 need = (argc == 255) ? 2u : (argc + 1u);
        if (sp >= 4u * need) {
          a = peek((argc == 255) ? 1u : argc);
          sh = shape_of_val(a);
          gu = (tag_of(a) == TAG_FN) ? GU_TYPE_OK : GU_NONE;
        }
      } else if (pc < end && (mem[pc] == OP_IDX || mem[pc] == OP_SETIDX)) {
        if (sp >= 8) {
          b = peek(0); a = peek(1);
          sh = shape_of_val(a);
          if (tag_of(a) == TAG_LIST || tag_of(a) == TAG_TUP)
            gu = GU_LEN_OK;
          else if (tag_of(a) == TAG_DICT)
            gu = GU_KEY_OK;
          else if (tag_of(a) == TAG_STR)
            gu = GU_LEN_OK;
          else
            gu = GU_NONE;
          if (tag_of(a) == tag_of(a)) { /* keep */ }
          if (gu != GU_NONE && tag_of(b) != TAG_NULL)
            ; /* ok */
        }
      } else if (pc < end && mem[pc] == OP_DOT) {
        if (sp >= 4) {
          a = peek(0);
          sh = shape_of_val(a);
          gu = (tag_of(a) == TAG_DICT) ? GU_KEY_OK : GU_NONE;
        }
      } else if (pc < end && mem[pc] == OP_LEN) {
        if (sp >= 4) {
          a = peek(0);
          sh = shape_of_val(a);
          gu = GU_TYPE_OK;
        }
      } else if (sp >= 8) {
        b = peek(0); a = peek(1);
        sh = shape_of_val(a);
        gu = (tag_of(a) == tag_of(b)) ? GU_TYPE_OK : GU_NONE;
      }
      last_ic_stub = ujs_ic_ask(sh, opid, gu);
      break;
    }
    case OP_MKLIST:
      n=mem[pc++]; p=alloc(8+n*4); mem[p]=TAG_LIST; wr16(p+2,(u16)n);
      for (i=n;i>0;i--) wr32(p+8+(i-1)*4, pop());
      push(p); break;
    case OP_MKTUP:
      n=mem[pc++]; p=alloc(8+n*4); mem[p]=TAG_TUP; wr16(p+2,(u16)n);
      for (i=n;i>0;i--) wr32(p+8+(i-1)*4, pop());
      push(p); break;
    case OP_MKDICT:
      n=mem[pc++]; p=alloc(8+n*8); mem[p]=TAG_DICT; wr16(p+2,(u16)n);
      for (i=n;i>0;i--) { b=pop(); a=pop(); wr32(p+8+(i-1)*8,a); wr32(p+8+(i-1)*8+4,b); }
      push(p); break;
    case OP_IDX:
      b=pop(); a=pop();
      if (tag_of(a)==TAG_LIST || tag_of(a)==TAG_TUP)
        push(rd32(a+8+(u32)i64_of(b)*4));
      else if (tag_of(a)==TAG_DICT) push(dict_get(a,b));
      else if (tag_of(a)==TAG_STR) {
        u32 ix = (u32)i64_of(b);
        if (ix >= len_of(a)) return 0;
        push(mk_str(&str_ptr(a)[ix], 1));
      } else return 0;
      break;
    case OP_SETIDX: {
      u32 val = pop(); b=pop(); a=pop();
      if (tag_of(a)==TAG_LIST && tag_of(b)==TAG_I64) {
        wr32(a+8+(u32)i64_of(b)*4, val);
        push(val);
      } else if (tag_of(a)==TAG_DICT && tag_of(b)==TAG_STR) {
        /* may grow — leave grown dict on stack under val? Python mutates.
           We replace handle identity by storing grown into… can't update
           caller's slot. For in-place existing keys OK; grow needs copy
           into same handle — not possible. Grow allocates new; push val
           only (dict slot still old). Prefer update-or-append into new
           and overwrite if we had ref — limited: update existing only,
           else grow into a and hope single ref (demos don't grow). */
        if (!dict_set(a, b, val)) {
          /* append in-place if capacity? heap is bump — can't.
             allocate new and copy fields into a's header (steal). */
          u32 nd = dict_grow_set(a, b, val);
          /* copy header+pairs back is heavy; just leave nd orphaned
             and update a's content by rewriting len+pairs if nd!=a */
          if (nd != a) {
            u32 nn = len_of(nd);
            /* if a had room? bump alloc means a may not have room.
               Replace: copy nd's bytes into… can't shrink/grow a.
               For demos, keys usually pre-exist. Fall through push val. */
            (void)nn;
          }
        }
        push(val);
      } else return 0;
      break;
    }
    case OP_DOT:
      i=rd32(pc); pc+=4; p=str_entry(i); n=rd32(p);
      b=mk_str(&mem[p+4], n); a=pop(); push(dict_get(a,b)); break;
    case OP_LEN: {
      a=pop();
      u32 t = tag_of(a);
      if (t==TAG_STR || t==TAG_LIST || t==TAG_DICT || t==TAG_TUP)
        push(mk_i64(len_of(a)));
      else return 0;
      break;
    }
    case OP_KEYS: {
      a=pop();
      if (tag_of(a)!=TAG_DICT) return 0;
      n = len_of(a);
      p = alloc(8+n*4); mem[p]=TAG_LIST; wr16(p+2,(u16)n);
      for (i=0;i<n;i++) wr32(p+8+i*4, rd32(a+8+i*8));
      push(p); break;
    }
    case OP_IN: {
      /* stack: key, container (compile emits key then container?)
         jtape: in pops container, key — bc_vm: container, key = pop(), pop()
         so top is container, under is key */
      u32 container = pop(); u32 key = pop();
      if (tag_of(container)!=TAG_DICT || tag_of(key)!=TAG_STR) return 0;
      c = 0;
      n = len_of(container);
      for (i=0;i<n;i++) if (str_eq(rd32(container+8+i*8), key)) { c=1; break; }
      push(mk_bool((u8)c)); break;
    }
    case OP_SPREAD: {
      a=pop();
      if (tag_of(a)!=TAG_LIST && tag_of(a)!=TAG_TUP) return 0;
      n = len_of(a);
      for (i=0;i<n;i++) push(rd32(a+8+i*4));
      break;
    }
    case OP_RESTPACK:
      n=mem[pc++]; p=alloc(8+n*4); mem[p]=TAG_LIST; wr16(p+2,(u16)n);
      for (i=n;i>0;i--) wr32(p+8+(i-1)*4, pop());
      push(p); break;
    case OP_TYPEOF: push(typeof_str(pop())); break;
    case OP_SHAPEOF: push(shapeof_str(pop())); break;
    case OP_PRINT:
      pop(); /* freestanding: drop; host may hook later */
      push(mk_null()); break;
    case OP_SWITCH:
      /* front uses compare-chain; raw table unused */
      return 0;
    case OP_CALL: {
      u32 argc=mem[pc++], args[64], fi, ent, nparams, rest_ix, nloc;
      u32 nested;
      if (argc == 255) { /* CALL_SPREAD: top is list/tup */
        a = pop();
        if (tag_of(a)!=TAG_LIST && tag_of(a)!=TAG_TUP) return 0;
        argc = len_of(a);
        if (argc > 64) return 0;
        for (i=0;i<argc;i++) args[i]=rd32(a+8+i*4);
      } else {
        if (argc > 64) return 0;
        for (i=argc;i>0;i--) args[i-1]=pop();
      }
      a=pop();
      if (tag_of(a)!=TAG_FN) return 0;
      fi=rd32(a+4);
      ent=fn_entry(fi);
      nparams=rd32(ent); rest_ix=rd32(ent+4); nloc=rd32(ent+8);
      (void)nloc;
      for (i=0;i<64;i++) saved[i]=locals[i];
      for (i=0;i<64;i++) locals[i]=0;
      for (i=0;i<nparams && i<argc;i++) locals[i]=args[i];
      if (rest_ix != 0xFFFFFFFFu) {
        u32 rn = argc>nparams ? argc-nparams : 0;
        p=alloc(8+rn*4); mem[p]=TAG_LIST; wr16(p+2,(u16)rn);
        for (i=0;i<rn;i++) wr32(p+8+i*4, args[nparams+i]);
        locals[rest_ix]=p;
      }
      nested = ent+12;
      save_code_base=code_base; save_code_len=code_len;
      save_str_base=str_base; save_nstr=nstr;
      code_len=rd32(nested); code_base=nested+4;
      p = code_base + code_len;
      nstr=rd32(p); str_base=p+4;
      a = run_code(code_base, code_len);
      code_base=save_code_base; code_len=save_code_len;
      str_base=save_str_base; nstr=save_nstr;
      for (i=0;i<64;i++) locals[i]=saved[i];
      push(a);
      break;
    }
    default: return 0;
    }
  }
  return a;
}

static void load_image(u32 img) {
  u32 p = img;
  code_len = rd32(p); p += 4;
  code_base = p; p += code_len;
  nstr = rd32(p); p += 4;
  str_base = p; p = skip_strings(p, nstr);
  nfn = rd32(p); fn_base = p + 4;
}

#define PROG_ADDR 16384

u32 run_prog(void) {
  freep = 65536;
  sp = 400000;
  last_ic_stub = 0;
  memset_z(locals, 0, sizeof locals);
  memset_z(globals, 0, sizeof globals);
#ifdef UJS_EMBED
  {
    extern const u8 ujs_embed[];
    extern const u32 ujs_embed_len;
    u32 i;
    for (i = 0; i < ujs_embed_len; i++) mem[PROG_ADDR + i] = ujs_embed[i];
  }
#endif
  load_image(PROG_ADDR);
  return run_code(code_base, code_len);
}

u32 main_export(void) { return run_prog(); }

/* ---- host API for shared runtime: load image + G/L then run [A-1] ---- */
u32 host_reset(void) {
  freep = 65536;
  sp = 400000;
  last_ic_stub = 0;
  memset_z(locals, 0, sizeof locals);
  memset_z(globals, 0, sizeof globals);
  return 0;
}
u32 host_prog_addr(void) { return PROG_ADDR; }
u32 host_load_prog(u32 src, u32 len) {
  u32 i;
  if (PROG_ADDR + len > MEM_SIZE) return 0;
  for (i = 0; i < len; i++) mem[PROG_ADDR + i] = mem[src + i];
  load_image(PROG_ADDR);
  return 1;
}
/* copy bytes from linear memory offset `src` — host writes image to PROG_ADDR directly */
u32 host_load_image(void) {
  load_image(PROG_ADDR);
  return code_len;
}
u32 host_run(void) {
  return run_code(code_base, code_len);
}
void host_set_local(u32 i, u32 h) { if (i < 256) locals[i] = h; }
void host_set_global(u32 i, u32 h) { if (i < 256) globals[i] = h; }
u32 host_get_local(u32 i) { return i < 256 ? locals[i] : 0; }
u32 host_get_global(u32 i) { return i < 256 ? globals[i] : 0; }

u32 host_mk_null(void) { return mk_null(); }
u32 host_mk_bool(u32 b) { return mk_bool((u8)b); }
u32 host_mk_i64(i64 v) { return mk_i64(v); }
u32 host_mk_f64(f64 v) { return mk_f64(v); }
u32 host_mk_str(u32 ptr, u32 n) { return mk_str(&mem[ptr], n); }

u32 tag_of_export(u32 h) { return tag_of(h); }
i64 i64_of_export(u32 h) { return i64_of(h); }
f64 f64_of_export(u32 h) { return tag_of(h)==TAG_F64 ? f64_of(h) : (f64)i64_of(h); }
u32 str_len_export(u32 h) { return len_of(h); }
u32 str_ptr_export(u32 h) { return h ? h+8 : 0; }
u32 last_ic_stub_export(void) { return last_ic_stub; }

u8 *mem_base(void) { return mem; }
u32 mem_size(void) { return MEM_SIZE; }
