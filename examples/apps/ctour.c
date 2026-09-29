/* ctour: a guided tour of the C that unisacc compiles, one line per check.
 *
 * About a hundred lines of checks, grouped into sections: struct
 * passing and returning by value, unions, bit-fields, static initializers,
 * varargs (int, double, vsnprintf), function pointers and callback tables,
 * qsort/bsearch, switch (dense and sparse), goto, static locals, 1-D VLAs,
 * compound literals, designated initializers, the preprocessor (# ## and
 * __VA_ARGS__), <stdint.h>/<stdbool.h>/<limits.h>/<ctype.h>/<math.h>,
 * printf/sprintf/sscanf formatting, strings and memory, 64-bit and unsigned
 * arithmetic, float/double conversion, inf/nan and deep recursion.
 *
 * Every line prints the same bytes as host cc (gcc/glibc, clang/libSystem)
 * on every target; no file, environment or clock input is read, so the
 * output is deterministic.
 *
 * Deliberately NOT in the tour (unisacc 0.0.12, reported for 0.0.13):
 *   - printf's `#` flag (%#x, %#o) is ignored;
 *   - printf's hh/h length modifiers (%hhd, %hd) do not narrow;
 *   - `*p = s;` struct assignment through a dereferenced pointer is
 *     refused ("not covered: width"; `p[0] = s;` works);
 *   - 2-D VLAs (`int m[n][4];`) fail to parse;
 *   - assert() without another printf use fails under the default
 *     -ftrim-libc ("undefined function 'printf'");
 *   - stdin compares equal to NULL; __LINE__/__FILE__ are unsupported
 *     (LOG() below spells its location out for that reason).
 * Also left out: undefined behaviour (signed overflow, unsequenced
 * modification) and implementation-defined results that differ from host
 * cc by design: sizeof(long double) is 8, %p of NULL prints 0x0, plain
 * char signedness, and out-of-range values stored in signed bit-fields.
 *
 *   unisacc -run examples/apps/ctour.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <ctype.h>
#include <math.h>

#define STR(x) #x
#define CAT(a, b) a##b
#define LOG(fmt, ...) printf("[%s:%d] " fmt "\n", "ctour.c", 99, __VA_ARGS__)

/* ---- structs, unions, bit-fields, static data ---- */
struct P { int x, y; };
struct Big { long a, b, c; double d; char name[16]; };
union U { int i; float f; unsigned char b[4]; };
struct BF { unsigned a : 3; unsigned b : 5; int c : 4; unsigned d : 20; };
struct SBF { signed int s : 4; unsigned int u : 4; };
typedef struct { double x, y; } V2;
typedef struct { char c; double d; short s; } Pad;
struct Mixed { int i; float f; double d; char c; };
struct F2 { float x, y; };
struct Node { int v; struct Node *next; };
struct Rec { char name[12]; int score; double w; };
struct Q { int a[3]; struct { char tag; short v; } in; };
union Shape { struct { int kind; double r; } circle; struct { int kind; int w, h; } rectangle; };
enum Color { RED, GREEN = 5, BLUE };

static int gtab[5] = { 1, 2, 3 };
static const char *gnames[] = { "zero", "one", "two" };
static struct P gp = { 7, -3 };
static struct Big gbig = { 1, 2, 3, 4.5, "global" };
static double gd = 1.0 / 3.0;
static int gt[4] = { 10, 20, 30, 40 };
static int *gpt = &gt[2];
static char words[3][8] = { "alpha", "beta", "gamma" };
static struct { int k; const char *v; } table[] = { { 1, "one" }, { 2, "two" }, { 3, "three" } };
static struct Q gq = { { 1, 2, 3 }, { 'k', -9 } };
static int bigarr[200000];

struct P addp(struct P a, struct P b) { struct P r; r.x = a.x + b.x; r.y = a.y + b.y; return r; }
struct Big mkbig(long k) { struct Big b; b.a = k; b.b = k * 2; b.c = k * 3; b.d = k / 4.0; strcpy(b.name, "made"); return b; }
double sumbig(struct Big b) { return b.a + b.b + b.c + b.d; }
V2 v2add(V2 a, V2 b) { V2 r = { a.x + b.x, a.y + b.y }; return r; }
V2 v2scale(V2 a, float k) { a.x *= k; a.y *= k; return a; }
struct Mixed mix(int i, float f, double d, char c) { struct Mixed m; m.i = i; m.f = f; m.d = d; m.c = c; return m; }
struct F2 f2mk(float a) { struct F2 r; r.x = a; r.y = a * 2; return r; }
float fhalf(float x) { return x / 2; }
struct Rec mkrec(const char *n, int s) { struct Rec r; strncpy(r.name, n, 11); r.name[11] = 0; r.score = s; r.w = s / 8.0; return r; }
typedef struct Rec (*mkfn)(const char *, int);

/* ---- varargs ---- */
int sumv(int n, ...) { va_list ap; int s = 0, i; va_start(ap, n); for (i = 0; i < n; i++) s += va_arg(ap, int); va_end(ap); return s; }
double sumd(int n, ...) { va_list ap; double s = 0; int i; va_start(ap, n); for (i = 0; i < n; i++) s += va_arg(ap, double); va_end(ap); return s; }
double favg(int n, ...) { va_list ap; double s = 0; int i; va_start(ap, n); for (i = 0; i < n; i++) s += va_arg(ap, double); va_end(ap); return s / n; }
void logf2(char *out, const char *fmt, ...) { va_list ap; va_start(ap, fmt); vsnprintf(out, 100, fmt, ap); va_end(ap); }

/* ---- functions, pointers, recursion ---- */
int fib(int n) { return n < 2 ? n : fib(n - 1) + fib(n - 2); }
static int sq(int x) { return x * x; }
static int neg(int x) { return -x; }
static inline int add3(int a, int b, int c) { return a + b + c; }
int apply(int (*f)(int), int v) { return f(v); }
int many(int a, int b, int c, int d, int e, int f, int g, int h, int i, int j) { return a+b*2+c*3+d*4+e*5+f*6+g*7+h*8+i*9+j*10; }
double manyd(double a, double b, double c, double d, double e, double f, double g, double h, double i, double j) { return a+b*2+c*3+d*4+e*5+f*6+g*7+h*8+i*9+j*10; }
static int add(int a, int b) { return a + b; }
static int mul(int a, int b) { return a * b; }
static struct { const char *name; int (*fn)(int, int); } ops2[] = { { "add", add }, { "mul", mul } };
int is_even(int n);
int is_odd(int n) { return n == 0 ? 0 : is_even(n - 1); }
int is_even(int n) { return n == 0 ? 1 : is_odd(n - 1); }
int depth(int n) { char pad[64]; pad[0] = (char)n; if (n == 0) return 0; return 1 + depth(n - 1) + pad[0] * 0; }
int cmpint(const void *a, const void *b) { return *(const int *)a - *(const int *)b; }
int byscore(const void *a, const void *b) { const struct Rec *x = a, *y = b; return y->score - x->score; }
int byname(const void *a, const void *b) { return strcmp(((const struct Rec *)a)->name, ((const struct Rec *)b)->name); }
void each(struct Rec *r, int n, void (*cb)(struct Rec)) { int i; for (i = 0; i < n; i++) cb(r[i]); }
void show(struct Rec r) { printf("  %-6s %3d %.3f\n", r.name, r.score, r.w); }
const char *sw(int k) { switch (k) { case 0: return "zero"; case 1: case 2: return "small"; case 10: return "ten"; default: return "other"; } }
int sparse(int k) { switch (k) { case -5: return 1; case 1000: return 2; case 1 << 20: return 3; case 'x': return 4; } return 0; }
int counter(void) { static int c = 0; return ++c; }

static void structs(void)
{
    struct P a = { 1, 2 }, b = { 30, 40 }, c;
    struct Big big;
    union U u;
    struct BF bf;
    struct SBF sb;
    V2 va = { 1.5, 2.5 }, vb = { .y = 10, .x = 20 }, vc, vs;
    struct Mixed m;
    struct F2 fv;
    union Shape sh;
    struct Node nodes[3] = { { 1, 0 }, { 2, 0 }, { 3, 0 } }, *p;
    printf("== structs, unions, bit-fields\n");
    c = addp(a, b); printf("addp %d %d\n", c.x, c.y);
    big = mkbig(10); printf("big %ld %ld %ld %.2f %s sum=%.2f\n", big.a, big.b, big.c, big.d, big.name, sumbig(big));
    printf("gbig %ld %.1f %s sum=%.1f\n", gbig.c, gbig.d, gbig.name, sumbig(gbig));
    vc = v2add(va, vb); vs = v2scale(vc, 0.5f); printf("v2 %.2f %.2f scaled %.2f %.2f\n", vc.x, vc.y, vs.x, vs.y);
    m = mix(3, 2.5f, 1e100, 'z'); printf("mix %d %.2f %g %c\n", m.i, m.f, m.d, m.c);
    fv = f2mk(1.25f); printf("F2 %.2f %.2f fhalf %.3f\n", fv.x, fv.y, fhalf(3.0f));
    printf("sizeof Pad %d offsetof d %d s %d\n", (int)sizeof(Pad), (int)(long)&((Pad *)0)->d, (int)(long)&((Pad *)0)->s);
    u.f = 1.0f; printf("union %08x %d %d\n", u.i, u.b[3], (int)sizeof(u));
    sh.circle.kind = 1; sh.circle.r = 2.0; printf("union circle %d %.1f size %d\n", sh.circle.kind, sh.circle.r, (int)sizeof sh);
    sh.rectangle.kind = 2; sh.rectangle.w = 3; sh.rectangle.h = 4; printf("union rect %d %d\n", sh.circle.kind, sh.rectangle.w * sh.rectangle.h);
    memset(&bf, 0, sizeof bf); bf.a = 5; bf.b = 17; bf.c = -3; bf.d = 1000000;
    printf("bf %u %u %d %u size=%d\n", bf.a, bf.b, bf.c, bf.d, (int)sizeof(bf));
    bf.a += 4; printf("bf unsigned wrap %u\n", bf.a);
    sb.s = -1; sb.u = 15; sb.u++; printf("sbf s %d neg %d u %d\n", sb.s, sb.s < 0, sb.u);
    nodes[0].next = &nodes[1]; nodes[1].next = &nodes[2];
    printf("list");
    for (p = nodes; p; p = p->next) printf(" %d", p->v);
    printf("\nenum %d %d %d\n", RED, GREEN, BLUE);
}

static void statics(void)
{
    printf("== static initializers\n");
    printf("gtab %d %d %d %d %d\n", gtab[0], gtab[1], gtab[2], gtab[3], gtab[4]);
    printf("gnames %s %s gp %d %d gd %.6f\n", gnames[1], gnames[2], gp.x, gp.y, gd);
    printf("gpt %d words %s %s %c table %s\n", *gpt, words[1], words[2], words[0][4], table[2].v);
    printf("gq %d %c %d\n", gq.a[2], gq.in.tag, gq.in.v);
    bigarr[199999] = 5; printf("static big %d\n", bigarr[199999] + bigarr[0]);
    { int arr[5] = { [1] = 10, [3] = 30 }; printf("desig arr %d %d %d\n", arr[0], arr[1], arr[3]); }
    { struct Q q = { .in = { .v = 7 }, .a = { [2] = 9 } }; printf("desig struct %d %d\n", q.a[2], q.in.v); }
    { int *q = (int[]){ 1, 2, 3 }; printf("compound literal %d\n", q[2]); }
}

static void functions(void)
{
    int (*ops[2])(int) = { sq, neg };
    int arr[8] = { 5, 3, 9, 1, 7, 2, 8, 6 };
    struct Rec rs[4], key, *hit, copy;
    mkfn mk = mkrec;
    int i;
    printf("== functions, callbacks, control flow\n");
    printf("sumv %d sumd %.3f favg %.3f\n", sumv(4, 1, 2, 3, 4), sumd(3, 0.5, 0.25, 0.125), favg(3, 1.0, 2.5f, 4.0));
    printf("fib20 %d mutual %d %d depth %d\n", fib(20), is_even(10), is_odd(7), depth(1000));
    printf("fp %d %d %d ops %s %d %s %d\n", apply(sq, 7), ops[0](5), ops[1](5), ops2[0].name, ops2[0].fn(3, 4), ops2[1].name, ops2[1].fn(3, 4));
    printf("add3 %d many %d manyd %.1f\n", add3(1, 2, 3), many(1,2,3,4,5,6,7,8,9,10), manyd(1,2,3,4,5,6,7,8,9,10));
    qsort(arr, 8, sizeof(int), cmpint);
    for (i = 0; i < 8; i++) printf("%d%c", arr[i], i == 7 ? '\n' : ',');
    rs[0] = mk("dave", 70); rs[1] = mk("alice", 95); rs[2] = mk("carol", 88); rs[3] = mk("bob", 61);
    qsort(rs, 4, sizeof rs[0], byscore); printf("by score:\n"); each(rs, 4, show);
    qsort(rs, 4, sizeof rs[0], byname); strcpy(key.name, "carol");
    hit = bsearch(&key, rs, 4, sizeof rs[0], byname); printf("bsearch carol -> %d\n", hit ? hit->score : -1);
    copy = rs[0]; copy.name[0] = 'X'; printf("copy %s orig %s\n", copy.name, rs[0].name);
    { static const int keys[] = { 0, 2, 10, 7 }; for (i = 0; i < 4; i++) printf("sw %d %s\n", keys[i], sw(keys[i])); }
    printf("sparse %d %d %d %d %d\n", sparse(-5), sparse(1000), sparse(1 << 20), sparse('x'), sparse(7));
    i = 0;
again:
    i++; if (i < 5) goto again;
    printf("goto %d\n", i);
    { int a, b, found = -1;
      for (a = 0; a < 10; a++) for (b = 0; b < 10; b++) if (a * b == 42) { found = a * 10 + b; goto done; }
done: printf("goto nested %d\n", found); }
    for (i = 0; i < 3; i++) counter();
    printf("counter %d\n", counter());
    { int j; for (i = 0, j = 10; i < j; i++, j--) { } printf("comma %d %d\n", i, j); }
    { char s[20]; int k = 0; do { s[k] = (char)('0' + k); k++; } while (k < 5); s[k] = 0; printf("do-while %s\n", s); }
    { int n = 5; printf("ternary chain %s\n", n > 3 ? n > 4 ? "big" : "mid" : "small"); }
    { int n = 5, vla[n]; for (i = 0; i < n; i++) vla[i] = i * i; printf("vla %d sizeof %d\n", vla[4], (int)sizeof(vla)); }
    { int grid[3][4], j, (*pa)[4];
      for (i = 0; i < 3; i++) for (j = 0; j < 4; j++) grid[i][j] = i * 10 + j;
      pa = grid; printf("grid %d %d %d\n", grid[2][3], pa[1][2], *(*(pa + 2) + 1)); }
}

static void preprocessor(void)
{
    printf("== preprocessor\n");
    printf("stringify %s cat %d\n", STR(a + b), CAT(1, 2));
    LOG("x=%d", 42);
    printf("literal concat: " "one " "two\n");
    printf("escapes: tab[\t] hex[\x41] oct[\101] quote[\"]\n");
}

static void types(int argc)
{
    bool flag = 7;
    long long ll = 9000000000LL * 3;
    unsigned long long ull = 0xFFFFFFFFFFFFFFFFULL / 7;
    uint8_t u8 = 250; uint16_t u16 = 65535; uint32_t u32 = 4000000000u;
    unsigned char c1 = 200, c2 = 200;
    long long sh = 1;
    float fa = 16777216.0f, farr[4] = { 0.5f, 1.5f, 2.5f, 3.5f };
    double dsum = 0, ld;
    int neg1 = -1, i;
    unsigned int un = 1, u = 0;
    printf("== integer and floating types\n");
    printf("bool %d size %d\n", flag, (int)sizeof(bool));
    printf("ll %lld ull %llu\n", ll, ull);
    u8 += 10; u16 += 2; u32 += 500000000u;
    printf("wrap u8 %d u16 %d u32 %u\n", u8, u16, u32);
    printf("uchar mul %d uchar cast %d\n", c1 * c2, (unsigned char)456);
    printf("shift %lld %llu %d\n", sh << 62, (unsigned long long)1 << 63, -16 >> 2);
    printf("div %d mod %d\n", -7 / 2, -7 % 2);
    { long l = -9; printf("ldiv %ld %ld %lu\n", l / 4, l % 4, (unsigned long)l / 4); }
    { unsigned long long v = 12345678901234567ULL; printf("ull mod %llu div %llu\n", v % 1000, v / 1000); }
    printf("unsigned %u %d cmp %d %d\n", (unsigned)-1, (unsigned)-1 > 0, (unsigned)neg1 > un, u - 1 > 0);
    printf("uint wrap %d\n", 4294967295u + (unsigned)(argc - 1) + 1u == 0);
    { int a = 2000000000, b = 2000000000; long r = (long)a + b; printf("widen before add %ld\n", r); }
    { int e = 0; e |= 1 << 3; e ^= 0xff; e &= ~2; printf("bitops %d\n", e); }
    printf("limits %d %d %ld\n", INT_MAX, INT_MIN, LONG_MAX);
    printf("conv %d %d %.1f %d\n", (int)3.99, (int)-3.99, (double)7 / 2, (int)(0.1 + 0.2 == 0.3));
    printf("float cmp %d %d\n", 0.1f < 0.2f, 1e308 * 10 > 1e308);
    printf("float add %.1f\n", (double)(fa + 1.0f));
    for (i = 0; i < 4; i++) dsum += farr[i];
    printf("farr sum %.2f\n", dsum);
    { float ff = 0.1f; double dd = ff; printf("f->d %.10f\n", dd); }
    { double d = -0.0; printf("negzero %.1f %g\n", d, 1.0 / 3); }
    { double inf = 1e308 * 10, nan = inf - inf; printf("inf %f nan %d\n", inf, nan != nan); }
    ld = 3.125L; printf("long double value %.3f\n", ld);
    printf("math sqrt %.4f pow %.4f sin %.4f exp %.4f log %.4f floor %.1f fmod %.2f\n",
           sqrt(2.0), pow(2, 0.5), sin(1.0), exp(1.0), log(10.0), floor(-2.5), fmod(7.5, 2));
}

static void library(int argc)
{
    char buf[128], out[100], *m;
    const char *fmt = "dynamic fmt %d %s\n";
    printf("== formatting, strings, memory\n");
    printf("float %f %.3f %e %g %g\n", 1.5f, 2.25, 2250.0, 0.0001, 123456789.0);
    printf("fmt [%5d] [%-5d] [%05d] [%x] [%X] [%o] [%c] [%s] [%10s] [%-10s] [%%] [%ld] [%lu]\n",
           42, 42, 42, 255, 255, 8, 'A', "str", "right", "left", -5L, 18446744073709551615UL);
    printf("fmt2 [%+d] [% d] [%.2s] [%*d] [%lld]\n", 5, 5, "abcdef", 6, 42, 123456789012LL);
    printf(fmt, 5, "ok");
    { const char *f = argc > 5 ? "%s" : "nonliteral %d %.2f %s\n"; printf(f, 7, 2.5, "ok"); }
    logf2(out, "v=%d s=%s f=%.2f x=%x", 5, "str", 1.25, 255); printf("vsnprintf [%s]\n", out);
    sprintf(out, "%o %x %5.1f|%-8.3e|", 8, 255, 3.14159, 12345.678); printf("sprintf [%s]\n", out);
    snprintf(buf, 10, "%d-%d-%d", 12345, 67890, 1); printf("snprintf [%s]\n", buf);
    { int x = 0, y = 0; sscanf("12 34", "%d %d", &x, &y); printf("sscanf %d %d\n", x, y); }
    printf("atoi %d strtol %ld strtod %.3f\n", atoi("-77"), strtol("ff", 0, 16), strtod("3.5e2", 0));
    m = malloc(100); strcpy(m, "hello"); strcat(m, " world");
    printf("str %s len=%d cmp=%d chr=%s rchr=%s str=%s\n", m, (int)strlen(m), strcmp(m, "hello") > 0, strchr(m, 'o'), strrchr(m, 'o'), strstr(m, "wor"));
    free(m);
    { char str[] = "a,b,,c"; char *t = strtok(str, ","); while (t) { printf("tok[%s]", t); t = strtok(0, ","); } printf("\n"); }
    printf("ctype %d %d %c %c\n", isdigit('7') != 0, isalpha('!') != 0, toupper('q'), tolower('Q'));
    { char b8[8]; memset(b8, 'A', 7); b8[7] = 0; printf("memset %s %d\n", b8, (int)strlen(b8)); }
    { char a[] = "hello"; memmove(a + 1, a, 4); printf("memmove %s memcmp %d\n", a, memcmp("abc", "abd", 3) < 0); }
    { int *big = malloc(1000000 * sizeof(int)), *r, i; for (i = 0; i < 1000000; i++) big[i] = i;
      r = realloc(big, 2000000 * sizeof(int)); printf("realloc keep %d\n", r[999999]); free(r); }
    { char *cz = calloc(10, 1); int z = 0, i; for (i = 0; i < 10; i++) z += cz[i]; printf("calloc zero %d\n", z); free(cz); }
    printf("fopen missing -> %s\n", fopen("/nonexistent-dir/unisacc-ctour", "r") == NULL ? "NULL" : "non-NULL");
}

int main(int argc, char **argv)
{
    structs();
    statics();
    functions();
    preprocessor();
    types(argc);
    library(argc);
    printf("argv0 set %d\n", argv[0] != 0);
    printf("tour done\n");
    return 0;
}
