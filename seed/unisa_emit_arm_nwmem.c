/* seed/unisa_emit_arm_nwmem.c -- C99 reference for unisa.emit_arm N / w / movimm / mem (K6-19).
 *
 *   unisa-emit-arm-nwmem N TOKEN              the register number of TOKEN (python N(r))
 *   unisa-emit-arm-nwmem w VALUE              the 4 little-endian bytes of VALUE & 0xFFFFFFFF, hex
 *   unisa-emit-arm-nwmem mem STORE RT RN OFF WD
 *                                             the bytes of mem(store, rt, rn, off, wd), hex
 *       STORE in 0 1; RT, RN register numbers; OFF a signed decimal; WD the access width
 *
 * Mirrors unisa/emit_arm.py N(), w(), movimm() (the far-offset helper mem() relies on) and mem() with
 * the scaled (imm12) and unscaled (imm9, signed) forms and the IP0 far path.  Refused (python: IndexError,
 * ValueError, KeyError, ZeroDivisionError): an empty register token, a non-integer after 'x', a width
 * that is not 1/2/4/8, a width of zero.  Point-sample equivalence over the listed legal domain and the
 * refusal points only; not the full encode(), not relocations, WinAPI, fp, or emit_x86; keeps emit_arm.py.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IP0 16u

static void refuse(const char *why, const char *what) {
    fprintf(stderr, "unisa-emit-arm-nwmem: %s %s\n", why, what ? what : "");
    exit(1);
}

/* a 4-byte little-endian word, appended to the byte buffer */
typedef struct { unsigned char b[4096]; size_t n; } Out;
static void put_w(Out *o, uint32_t v) {
    unsigned char e[4]; int k;
    for (k = 0; k < 4; k++) e[k] = (unsigned char)(v >> (8 * k));
    if (o->n + 4 > sizeof o->b) refuse("output overflow", NULL);
    memcpy(o->b + o->n, e, 4); o->n += 4;
}

static uint32_t w_word(int64_t v) { return (uint32_t)(uint64_t)v; }   /* python: v & 0xFFFFFFFF */

/* python N(r): int(r[1:]) if r[0] == 'x' else 31.  Empty token -> IndexError (refused).  The integer
 * after 'x' is an optional sign and decimal digits (python int() also takes underscores, not covered). */
static int64_t N_of(const char *r) {
    if (r[0] == '\0') refuse("empty register token (python: IndexError)", NULL);
    if (r[0] != 'x') return 31;
    {
        const char *p = r + 1; int neg = 0; int64_t v = 0; size_t nd = 0;
        if (*p == '+' || *p == '-') { neg = *p == '-'; p++; }
        for (; *p; p++) {
            if (*p < '0' || *p > '9') refuse("non-integer register number (python: ValueError)", r);
            v = v * 10 + (*p - '0'); nd++;
            if (v > 1000000000LL) refuse("register number out of range", r);
        }
        if (nd == 0) refuse("non-integer register number (python: ValueError)", r);
        return neg ? -v : v;
    }
}

/* movimm(d, v) with fixed = 0: MOVZ then MOVK for each non-zero 16-bit part above the first */
static void movimm(Out *o, uint32_t d, int64_t v) {
    uint64_t uv = (uint64_t)v; int sh;
    put_w(o, 0xD2800000u | (uint32_t)((uv & 0xFFFF) << 5) | d);
    for (sh = 1; sh <= 3; sh++) {
        uint64_t part = (uv >> (16 * sh)) & 0xFFFF;
        if (part) put_w(o, 0xF2800000u | ((uint32_t)sh << 21) | (uint32_t)(part << 5) | d);
    }
}

static uint32_t ld_scaled(int store, int wd, int *ok) {
    switch (wd) {
    case 8: *ok = 1; return store ? 0xF9000000u : 0xF9400000u;
    case 4: *ok = 1; return store ? 0xB9000000u : 0xB9800000u;
    case 2: *ok = 1; return store ? 0x79000000u : 0x79800000u;
    case 1: *ok = 1; return store ? 0x39000000u : 0x39800000u;
    default: *ok = 0; return 0;
    }
}
static uint32_t ld_unscaled(int store, int wd, int *ok) {
    switch (wd) {
    case 8: *ok = 1; return store ? 0xF8000000u : 0xF8400000u;
    case 4: *ok = 1; return store ? 0xB8000000u : 0xB8800000u;
    case 2: *ok = 1; return store ? 0x78000000u : 0x78800000u;
    case 1: *ok = 1; return store ? 0x38000000u : 0x38800000u;
    default: *ok = 0; return 0;
    }
}

/* mem(store, rt, rn, off, wd) */
static void mem(Out *o, int store, uint32_t rt, uint32_t rn, int64_t off, int64_t wd) {
    int ok = 0; uint32_t base;
    if (wd == 0) refuse("width zero (python: ZeroDivisionError)", NULL);
    /* scaled: off >= 0, off % wd == 0, off // wd < 4096 */
    if (off >= 0 && off % wd == 0 && (off / wd) < 4096) {
        base = ld_scaled(store, (int)wd, &ok);
        if (!ok) refuse("width not in {1,2,4,8} (python: KeyError)", NULL);
        put_w(o, base | ((uint32_t)(off / wd) << 10) | (rn << 5) | rt);
        return;
    }
    /* unscaled signed imm9: -256 <= off < 256 */
    if (off >= -256 && off < 256) {
        base = ld_unscaled(store, (int)wd, &ok);
        if (!ok) refuse("width not in {1,2,4,8} (python: KeyError)", NULL);
        put_w(o, base | ((((uint32_t)off) & 0x1FFu) << 12) | (rn << 5) | rt);
        return;
    }
    /* far path: the offset goes through IP0 */
    movimm(o, IP0, off < 0 ? -off : off);
    put_w(o, (off < 0 ? 0xCB000000u : 0x8B000000u) | (IP0 << 16) | (rn << 5) | IP0);
    base = ld_scaled(store, (int)wd, &ok);
    if (!ok) refuse("width not in {1,2,4,8} (python: KeyError)", NULL);
    put_w(o, base | (IP0 << 5) | rt);
}

static void hex_out(const Out *o) { size_t k; for (k = 0; k < o->n; k++) printf("%02x", o->b[k]); printf("\n"); }

static int64_t dec_signed(const char *s, const char *what) {
    const char *p = s; int neg = 0; int64_t v = 0; size_t nd = 0;
    if (*p == '+' || *p == '-') { neg = *p == '-'; p++; }
    for (; *p; p++) {
        if (*p < '0' || *p > '9') refuse("not a decimal integer", what);
        if (v > 1000000000000LL) refuse("integer out of range", what);
        v = v * 10 + (*p - '0'); nd++;
    }
    if (nd == 0) refuse("not a decimal integer", what);
    return neg ? -v : v;
}

int main(int argc, char **argv) {
    Out o; memset(&o, 0, sizeof o);
    if (argc == 3 && !strcmp(argv[1], "N")) { printf("%lld\n", (long long)N_of(argv[2])); return 0; }
    if (argc == 3 && !strcmp(argv[1], "w")) { put_w(&o, w_word(dec_signed(argv[2], argv[2]))); hex_out(&o); return 0; }
    if (argc == 7 && !strcmp(argv[1], "mem")) {
        int store = (int)dec_signed(argv[2], argv[2]);
        int64_t rt = dec_signed(argv[3], argv[3]), rn = dec_signed(argv[4], argv[4]);
        int64_t off = dec_signed(argv[5], argv[5]), wd = dec_signed(argv[6], argv[6]);
        if (store != 0 && store != 1) refuse("STORE must be 0 or 1", argv[2]);
        if (rt < 0 || rt > 31 || rn < 0 || rn > 31) refuse("register number out of range", NULL);
        mem(&o, store, (uint32_t)rt, (uint32_t)rn, off, wd);
        hex_out(&o);
        return 0;
    }
    fprintf(stderr, "usage: unisa-emit-arm-nwmem N TOKEN | w VALUE | mem STORE RT RN OFF WD\n");
    return 2;
}
