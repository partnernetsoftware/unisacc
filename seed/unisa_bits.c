/* seed/unisa_bits.c -- C99 reference for unisa.bits (K6-1).
 *
 *   unisa-bits mask64
 *   unisa-bits round-up V A
 *   unisa-bits round-up-batch   # stdin lines: V A -> stdout one decimal per line
 *
 * MASK64 and round_up match unisa/bits.py for non-negative inputs whose
 * mathematical result fits in uint64.  Alignment A==0 is refused by name.
 * Does not replace the Python module; product paths stay on bits.py.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MASK64_U ((uint64_t)-1)

static void die(const char *why) {
    fprintf(stderr, "unisa-bits: %s\n", why);
    exit(1);
}

/* Python: (v + a - 1) // a * a  for int >= 0.
   Equivalent when the result fits in uint64:
     rem = v % a;  return rem ? v + (a - rem) : v
   Overflow of that sum is refused (Python would return a bigint). */
static uint64_t round_up_u(uint64_t v, uint64_t a) {
    uint64_t rem, add;
    if (a == 0) die("alignment zero");
    rem = v % a;
    if (rem == 0) return v;
    add = a - rem;
    if (add > MASK64_U - v) die("round_up overflow");
    return v + add;
}

static int parse_u64(const char *s, uint64_t *out) {
    char *end = 0;
    unsigned long long v;
    if (!s || !*s) return 0;
    errno = 0;
    v = strtoull(s, &end, 10);
    if (errno || end == s || *end != '\0') return 0;
    *out = (uint64_t)v;
    return 1;
}

static void cmd_mask64(void) {
    printf("%llu\n", (unsigned long long)MASK64_U);
}

static void cmd_round_up(int argc, char **argv) {
    uint64_t v, a, r;
    if (argc != 2) die("round-up needs V A");
    if (!parse_u64(argv[0], &v) || !parse_u64(argv[1], &a)) die("bad integer");
    r = round_up_u(v, a);
    printf("%llu\n", (unsigned long long)r);
}

static void cmd_round_up_batch(void) {
    char line[256];
    while (fgets(line, sizeof line, stdin)) {
        char *p = line, *sp;
        uint64_t v, a, r;
        size_t n = strlen(line);
        if (n && line[n - 1] == '\n') line[n - 1] = 0;
        if (!line[0] || line[0] == '#') continue;
        while (*p == ' ' || *p == '\t') p++;
        sp = p;
        while (*sp && *sp != ' ' && *sp != '\t') sp++;
        if (!*sp) die("round-up-batch needs V A per line");
        *sp++ = 0;
        while (*sp == ' ' || *sp == '\t') sp++;
        if (!parse_u64(p, &v) || !parse_u64(sp, &a)) die("bad integer");
        r = round_up_u(v, a);
        printf("%llu\n", (unsigned long long)r);
    }
    if (ferror(stdin)) die("stdin read");
}

int main(int argc, char **argv) {
    if (argc < 2) die("usage: unisa-bits mask64|round-up V A|round-up-batch");
    if (strcmp(argv[1], "mask64") == 0) {
        if (argc != 2) die("mask64 takes no args");
        cmd_mask64();
        return 0;
    }
    if (strcmp(argv[1], "round-up") == 0) {
        cmd_round_up(argc - 2, argv + 2);
        return 0;
    }
    if (strcmp(argv[1], "round-up-batch") == 0) {
        if (argc != 2) die("round-up-batch takes no args");
        cmd_round_up_batch();
        return 0;
    }
    die("unknown command");
    return 1;
}
