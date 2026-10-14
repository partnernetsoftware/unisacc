/* seed/unisa_vm.c -- C99 reference for unisa.vm (K6-17), run() point samples.
 *
 *   unisa-vm run MAXSTEPS SCRIPT
 *       SCRIPT lines (fields separated by blanks; blank lines skipped):
 *         L NAME                    a label at the next instruction (Tape.label)
 *         I OP ARG...               an instruction (Tape.emit): ARG i = int(x,0); r/L/s = the token
 *       prints "CODE n", "STEPS n", "OUT hex" for VM(tape, max_steps=MAXSTEPS).run().
 *
 * Mirrors unisa/vm.py run() for the subset: integer arithmetic and compares (incl. the signed and
 * unsigned division and remainder with div-by-zero halt 136), shifts, imm/mov, load64/store64/.ld/.st
 * with the 16 MB flat memory and the fault halt 139, jump/jumpz/call/ret through a stack in memory,
 * .frame, .print (decimal, then the gate clobber r0=0, r1=r2=POISON), .exit, nop, end-of-code halt
 * with r0 & 0xFF, and the step limit halt 124 (checked before each instruction).  Unknown ops and wrong
 * operand counts are refused at build time (python: assert in emit); an unknown register or label is
 * refused at run time (python: KeyError), with nothing printed.  A halt is a normal result, not a refusal.
 * Not covered (refused by this reference): floating-point ops, .lea, .zero, .write, callr, callm,
 * .sys/.sys6/.argc/.argv and the .hostcall/.librarycall/.libraryaddr/.libraryfn/.hostaddr family, and any access that
 * would straddle the end of memory.  Point-sample equivalence over the listed programs only; not a full
 * opcode sweep; not a real OS file or syscall product closure; keeps unisa/vm.py and the closed deps.
 */
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEM_SIZE 0x1000000ull
#define STACK_TOP 0x1000000ull
#define SP 7
#define POISON 0xDEAD5EA1DEAD5EA1ull
#define MAXTOK 16
#define ULLONG_MAX_SAFE ULLONG_MAX

static char *dupn(const char *p) { size_t n = strlen(p); char *r = malloc(n + 1); if (!r) exit(1); memcpy(r, p, n + 1); return r; }

static void die(const char *why, const char *what) {
    fprintf(stderr, "unisa-vm: %s %s\n", why, what ? what : "");
    exit(1);
}

typedef struct { const char *op, *shape; } OpShape;
static const OpShape SH[] = {
    {"imm", "ri"}, {"mov", "rr"}, {"add64", "rrr"}, {"sub64", "rrr"}, {"mul64", "rrr"},
    {"xor64", "rrr"}, {"and64", "rrr"}, {"or64", "rrr"}, {"shl64", "rrr"}, {"shr64", "rrr"},
    {"lshr64", "rrr"}, {".div", "rrr"}, {".mod", "rrr"}, {".udiv", "rrr"}, {".umod", "rrr"},
    {"slt64", "rrr"}, {"sle64", "rrr"}, {"ult64", "rrr"}, {"ule64", "rrr"}, {"eq", "rrr"}, {"ne", "rrr"},
    {"load64", "rri"}, {"store64", "rir"}, {".ld", "rrii"}, {".st", "riri"}, {".lea", "rs"},
    {".zero", "rii"}, {"jump", "L"}, {"jumpz", "rL"}, {"call", "L"}, {"callr", "r"}, {"callm", "ri"},
    {".hostcall", "rr"}, {".librarycall", "rr"}, {".libraryaddr", "rsiiii"}, {".libraryfn", "ri"}, {".hostaddr", "ri"},
    {"ret", ""}, {".frame", "i"}, {".arg", "ir"}, {".print", "r"}, {".write", "rr"}, {".exit", "r"},
    {".sys", "srrr"}, {".sys6", "srrrrrr"}, {".argc", "r"}, {".argv", "rr"}, {"nop", ""},
};
#define NSH ((int)(sizeof SH / sizeof SH[0]))
static int op_index(const char *op) { int i; for (i = 0; i < NSH; i++) if (!strcmp(SH[i].op, op)) return i; return -1; }

static int hexval(int c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
static int int_lit(const char *s, long long *out) {   /* python int(s, 0) */
    const char *p = s; int neg = 0; unsigned long long v = 0;
    if (*p == '+' || *p == '-') { neg = *p == '-'; p++; }
    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
        p += 2; if (!*p) return 0;
        for (; *p; p++) { int d = hexval((unsigned char)*p); if (d < 0 || v > (ULLONG_MAX_SAFE >> 4)) return 0; v = v * 16 + (unsigned)d; }
    } else {
        if (!*p) return 0;
        if (p[0] == '0' && strspn(p, "0") != strlen(p)) return 0;
        for (; *p; p++) { if (*p < '0' || *p > '9' || v > (ULLONG_MAX_SAFE - 9) / 10) return 0; v = v * 10 + (unsigned)(*p - '0'); }
    }
    if (neg ? v > (1ull << 63) : v > (1ull << 63) - 1) return 0;
    *out = neg ? (long long)(0 - v) : (long long)v;
    return 1;
}

typedef struct { int isint; long long iv; char *s; } Arg;
typedef struct { int op; int n; Arg a[MAXTOK]; } Ins;
static Ins *code; static size_t ncode, capcode;
typedef struct { char *name; size_t pc; } Lab;
static Lab *lbls; static size_t nlab, caplab;

static void add_label(const char *name) {
    size_t i;
    for (i = 0; i < nlab; i++) if (!strcmp(lbls[i].name, name)) { lbls[i].pc = ncode; return; }
    if (nlab == caplab) { caplab = caplab ? caplab * 2 : 16; lbls = realloc(lbls, caplab * sizeof *lbls); if (!lbls) die("out of memory", NULL); }
    lbls[nlab].name = dupn(name); lbls[nlab].pc = ncode; nlab++;
}
static int find_label(const char *name, size_t *pc) {
    size_t i;
    for (i = 0; i < nlab; i++) if (!strcmp(lbls[i].name, name)) { *pc = lbls[i].pc; return 1; }
    return 0;
}

static void emit(int oi, int ntok, char **tok) {
    const char *sh = SH[oi].shape; int i;
    Ins *in;
    if ((int)strlen(sh) != ntok) die("operand count (python: assert)", SH[oi].op);
    if (ncode == capcode) { capcode = capcode ? capcode * 2 : 64; code = realloc(code, capcode * sizeof *code); if (!code) die("out of memory", NULL); }
    in = &code[ncode++];
    memset(in, 0, sizeof *in);
    in->op = oi; in->n = ntok;
    for (i = 0; i < ntok; i++) {
        if (sh[i] == 'i') {
            if (!int_lit(tok[i], &in->a[i].iv)) die("not an integer (python: ValueError)", tok[i]);
            in->a[i].isint = 1;
        } else in->a[i].s = dupn(tok[i]);
    }
}

static int split_ws(char *s, char **tok, int max) {
    int n = 0;
    for (;;) {
        while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') s++;
        if (!*s) break;
        if (n >= max) die("too many fields", NULL);
        tok[n++] = s;
        while (*s && *s != ' ' && *s != '\t' && *s != '\r' && *s != '\n') s++;
        if (*s) *s++ = 0;
    }
    return n;
}

/* ---- the machine (vm.VM) ---- */
static uint64_t R[8];
static unsigned char *mem;
static unsigned char *outb; static size_t outn, outcap;
static uint64_t steps, max_steps;

static void out_add(const void *p, size_t k) {
    if (outn + k > outcap) { size_t nc = outcap ? outcap * 2 : 256; while (nc < outn + k) nc *= 2; outb = realloc(outb, nc); if (!outb) die("out of memory", NULL); outcap = nc; }
    memcpy(outb + outn, p, k); outn += k;
}

typedef struct { int halt; int code; } Stop;   /* a normal halt: code as the python Halt(code) */

static Stop halt(int c) { Stop s; s.halt = 1; s.code = c; return s; }

static int reg_of(const char *t) {   /* python ri[...]: KeyError (refusal) on a name that is not a register */
    if (strlen(t) == 2 && t[0] == 'r' && t[1] >= '0' && t[1] <= '7') return t[1] - '0';
    die("unknown register (python: KeyError)", t);
    return 0;
}
static uint64_t addr_of(uint64_t base, long long off, int *fault) {
    uint64_t a = (base + (uint64_t)off);
    if (a >= MEM_SIZE) { *fault = 1; return a; }
    return a;
}
static uint64_t ldv(uint64_t a, int w, int sgn) {   /* python ld(): w bytes, signed sign-extends w < 8 */
    uint64_t v = 0; int i;
    for (i = 0; i < w; i++) v |= (uint64_t)mem[a + i] << (8 * i);
    if (sgn && w < 8 && ((v >> (w * 8 - 1)) & 1)) v |= ~0ull << (w * 8);
    return v;
}
static void stv(uint64_t a, uint64_t v, int w) { int i; for (i = 0; i < w; i++) mem[a + i] = (unsigned char)(v >> (8 * i)); }

static void gate_clobber(uint64_t ret) { R[0] = ret; R[1] = POISON; R[2] = POISON; }
static int64_t s64(uint64_t x) { return (int64_t)x; }

static Stop run_vm(void) {
    size_t pc = 0; size_t i;
    /* labels: python labels.get("_start", labels.get("main", 0)) */
    { size_t p; if (find_label("_start", &p)) pc = p; else if (find_label("main", &p)) pc = p; }
    for (;;) {
        Ins *in; int oi; const char *sh; int k;
        steps++;
        if (steps > max_steps) return halt(124);
        if (pc >= ncode) return halt((int)(R[0] & 0xFF));
        in = &code[pc];
        oi = in->op; sh = SH[oi].shape;
        pc++;
        {
            const char *op = SH[oi].op;
            /* reg operands are resolved here, where python would raise KeyError */
            int rd = -1, r1 = -1, r2 = -1;
            if (sh[0] == 'r') rd = reg_of(in->a[0].s);
            if (sh[1] == 'r' && (!strcmp(op, "mov") || !strcmp(op, "add64") || !strcmp(op, "sub64") || !strcmp(op, "mul64") || !strcmp(op, "xor64") || !strcmp(op, "and64") || !strcmp(op, "or64") || !strcmp(op, "shl64") || !strcmp(op, "shr64") || !strcmp(op, "lshr64") || !strcmp(op, ".div") || !strcmp(op, ".mod") || !strcmp(op, ".udiv") || !strcmp(op, ".umod") || !strcmp(op, "slt64") || !strcmp(op, "sle64") || !strcmp(op, "ult64") || !strcmp(op, "ule64") || !strcmp(op, "eq") || !strcmp(op, "ne"))) r1 = reg_of(in->a[1].s);
            if (sh[2] == 'r' && (!strcmp(op, "add64") || !strcmp(op, "sub64") || !strcmp(op, "mul64") || !strcmp(op, "xor64") || !strcmp(op, "and64") || !strcmp(op, "or64") || !strcmp(op, "shl64") || !strcmp(op, "shr64") || !strcmp(op, "lshr64") || !strcmp(op, ".div") || !strcmp(op, ".mod") || !strcmp(op, ".udiv") || !strcmp(op, ".umod") || !strcmp(op, "slt64") || !strcmp(op, "sle64") || !strcmp(op, "ult64") || !strcmp(op, "ule64") || !strcmp(op, "eq") || !strcmp(op, "ne"))) r2 = reg_of(in->a[2].s);
            if (!strcmp(op, "imm")) R[rd] = (uint64_t)in->a[1].iv;
            else if (!strcmp(op, "mov")) R[rd] = R[r1];
            else if (!strcmp(op, "add64")) R[rd] = R[r1] + R[r2];
            else if (!strcmp(op, "sub64")) R[rd] = R[r1] - R[r2];
            else if (!strcmp(op, "mul64")) R[rd] = R[r1] * R[r2];
            else if (!strcmp(op, "and64")) R[rd] = R[r1] & R[r2];
            else if (!strcmp(op, "or64")) R[rd] = R[r1] | R[r2];
            else if (!strcmp(op, "xor64")) R[rd] = R[r1] ^ R[r2];
            else if (!strcmp(op, "shl64")) R[rd] = R[r1] << (R[r2] & 63);
            else if (!strcmp(op, "shr64")) R[rd] = (uint64_t)(s64(R[r1]) >> (R[r2] & 63));
            else if (!strcmp(op, "lshr64")) R[rd] = R[r1] >> (R[r2] & 63);
            else if (!strcmp(op, ".udiv") || !strcmp(op, ".umod")) {
                uint64_t x = R[r1], y = R[r2], q;
                if (y == 0) return halt(136);
                q = x / y; R[rd] = !strcmp(op, ".udiv") ? q : x - q * y;
            } else if (!strcmp(op, ".div") || !strcmp(op, ".mod")) {
                int64_t x = s64(R[r1]), y = s64(R[r2]); uint64_t ax, ay, q; int neg;
                if (y == 0) return halt(136);
                ax = x < 0 ? (uint64_t)0 - (uint64_t)x : (uint64_t)x;
                ay = y < 0 ? (uint64_t)0 - (uint64_t)y : (uint64_t)y;
                q = ax / ay; neg = (x < 0) != (y < 0);
                if (neg) q = (uint64_t)0 - q;
                R[rd] = !strcmp(op, ".div") ? q : (uint64_t)(x - (int64_t)q * y);
            }
            else if (!strcmp(op, "slt64")) R[rd] = s64(R[r1]) < s64(R[r2]);
            else if (!strcmp(op, "sle64")) R[rd] = s64(R[r1]) <= s64(R[r2]);
            else if (!strcmp(op, "ult64")) R[rd] = R[r1] < R[r2];
            else if (!strcmp(op, "ule64")) R[rd] = R[r1] <= R[r2];
            else if (!strcmp(op, "eq")) R[rd] = R[r1] == R[r2];
            else if (!strcmp(op, "ne")) R[rd] = R[r1] != R[r2];
            else if (!strcmp(op, "load64")) {
                int fault = 0; int b = reg_of(in->a[1].s); uint64_t a = addr_of(R[b], in->a[2].iv, &fault);
                if (fault) { fprintf(stderr, "vm fault: address %#llx at pc %zu after %llu steps\n", (unsigned long long)a, pc - 1, (unsigned long long)steps); return halt(139); }
                R[rd] = ldv(a, 8, 0);
            }
            else if (!strcmp(op, "store64")) {
                int fault = 0; int bb = reg_of(in->a[0].s); uint64_t a = addr_of(R[bb], in->a[1].iv, &fault);
                int v = reg_of(in->a[2].s);
                if (fault) { fprintf(stderr, "vm fault: address %#llx at pc %zu after %llu steps\n", (unsigned long long)a, pc - 1, (unsigned long long)steps); return halt(139); }
                if (a + 8 > MEM_SIZE) die("access straddles the end of memory (not covered)", NULL);
                stv(a, R[v], 8);
            }
            else if (!strcmp(op, ".ld")) {
                int fault = 0; int b = reg_of(in->a[1].s); uint64_t a = addr_of(R[b], in->a[2].iv, &fault);
                int w = (int)in->a[3].iv;
                if (fault) { fprintf(stderr, "vm fault: address %#llx at pc %zu after %llu steps\n", (unsigned long long)a, pc - 1, (unsigned long long)steps); return halt(139); }
                if (w != 1 && w != 2 && w != 4 && w != 8) die(".ld width not covered", NULL);
                if (a + (uint64_t)w > MEM_SIZE) die("access straddles the end of memory (not covered)", NULL);
                R[rd] = ldv(a, w, 1);
            }
            else if (!strcmp(op, ".st")) {
                int fault = 0; int bb = reg_of(in->a[0].s); uint64_t a = addr_of(R[bb], in->a[1].iv, &fault);
                int v = reg_of(in->a[2].s); int w = (int)in->a[3].iv;
                if (fault) { fprintf(stderr, "vm fault: address %#llx at pc %zu after %llu steps\n", (unsigned long long)a, pc - 1, (unsigned long long)steps); return halt(139); }
                if (w != 1 && w != 2 && w != 4 && w != 8) die(".st width not covered", NULL);
                if (a + (uint64_t)w > MEM_SIZE) die("access straddles the end of memory (not covered)", NULL);
                stv(a, R[v], w);
            }
            else if (!strcmp(op, "jump")) {
                size_t t; if (!find_label(in->a[0].s, &t)) die("unknown label (python: KeyError)", in->a[0].s);
                pc = t;
            }
            else if (!strcmp(op, "jumpz")) {
                int rr = reg_of(in->a[0].s);
                if (R[rr] == 0) { size_t t; if (!find_label(in->a[1].s, &t)) die("unknown label (python: KeyError)", in->a[1].s); pc = t; }
            }
            else if (!strcmp(op, "call")) {
                size_t t; R[SP] = R[SP] - 8;
                stv(R[SP], pc, 8);
                if (!find_label(in->a[0].s, &t)) die("unknown label (python: KeyError)", in->a[0].s);
                pc = t;
            }
            else if (!strcmp(op, "ret")) {
                pc = (size_t)ldv(R[SP], 8, 0); R[SP] = R[SP] + 8;
            }
            else if (!strcmp(op, ".frame")) R[SP] = R[SP] - (uint64_t)in->a[0].iv;
            else if (!strcmp(op, ".print")) {
                char buf[32]; int n = snprintf(buf, sizeof buf, "%lld", (long long)s64(R[rd]));
                out_add(buf, (size_t)n);
                gate_clobber(0);
            }
            else if (!strcmp(op, ".exit")) return halt((int)(R[rd] & 0xFF));
            else if (!strcmp(op, "nop")) { }
            else die("operation not covered by this reference", op);
        }
        (void)k; (void)i;
    }
}

int main(int argc, char **argv) {
    FILE *f; char line[1 << 16]; char *tok[MAXTOK + 2]; Stop st; size_t i;
    if (argc != 4 || strcmp(argv[1], "run")) { fprintf(stderr, "usage: unisa-vm run MAXSTEPS SCRIPT\n"); return 2; }
    { long long m; if (!int_lit(argv[2], &m) || m < 0) die("bad max steps", argv[2]); max_steps = (uint64_t)m; }
    mem = calloc(1, MEM_SIZE); if (!mem) die("out of memory", NULL);
    R[SP] = STACK_TOP;
    f = fopen(argv[3], "rb"); if (!f) die("cannot open", argv[3]);
    while (fgets(line, sizeof line, f)) {
        int nt = split_ws(line, tok, MAXTOK + 2);
        if (nt == 0) continue;
        if (!strcmp(tok[0], "L") && nt == 2) add_label(tok[1]);
        else if (!strcmp(tok[0], "I") && nt >= 2) {
            int oi = op_index(tok[1]);
            if (oi < 0) die("unknown tape op (python: assert)", tok[1]);
            emit(oi, nt - 2, tok + 2);
        } else die("script form", tok[0]);
    }
    fclose(f);
    st = run_vm();
    printf("CODE %d\n", st.code);
    printf("STEPS %llu\n", (unsigned long long)steps);
    printf("OUT ");
    for (i = 0; i < outn; i++) printf("%02x", outb[i]);
    printf("\n");
    return 0;
}
