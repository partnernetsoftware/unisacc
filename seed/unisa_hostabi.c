/* seed/unisa_hostabi.c -- C99 reference for unisa.hostabi (K6-2).
 *
 *   unisa-hostabi dump TSV     parse the declaration table and print the constants that
 *                              unisa/hostabi.py exports (ARM_FN, ARM_ARGV, ARM_BODY,
 *                              X86_BODY, WIN_ARM_BODY, WIN_X86_BODY), one per line:
 *                                NAME COUNT HEX
 *                              HEX is the words (8 hex digits each) or bytes (2 each)
 *                              concatenated.  Any malformed or missing row is refused by name.
 *
 * Point-sample equivalence only: the constant set for the table as given.  Not a product
 * closure member; the product keeps unisa/hostabi.py.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXLINE 4096
#define MAXROWS 512

typedef struct { char arch[32]; char part[32]; char hex[2048]; } Row;

static Row rows[MAXROWS];
static int nrows;
static char obuf[1 << 16];
static size_t olen;

static void emit(const char *fmt_name, int count, const char *hex) {
    int k = snprintf(obuf + olen, sizeof obuf - olen, "%s %d %s\n", fmt_name, count, hex);
    if (k < 0 || (size_t)k >= sizeof obuf - olen) { fprintf(stderr, "unisa-hostabi: output overflow\n"); exit(1); }
    olen += (size_t)k;
}

static void die(const char *why, const char *what) {
    fprintf(stderr, "unisa-hostabi: %s %s\n", why, what);
    exit(1);
}

static void load(const char *path) {
    FILE *f = fopen(path, "r");
    char line[MAXLINE];
    if (!f) die("cannot open", path);
    while (fgets(line, sizeof line, f)) {
        char *a, *p, *h, *tab1, *tab2;
        size_t n = strlen(line);
        if (n && line[n - 1] == '\n') line[--n] = 0;
        if (n == 0 || line[0] == '#') continue;
        tab1 = strchr(line, '\t'); if (!tab1) die("malformed row (no tab)", line);
        tab2 = strchr(tab1 + 1, '\t'); if (!tab2) die("malformed row (one tab)", line);
        a = line; p = tab1 + 1; h = tab2 + 1;
        *tab1 = 0; *tab2 = 0;
        if (nrows >= MAXROWS) die("too many rows", path);
        if (strlen(a) >= sizeof rows[0].arch || strlen(p) >= sizeof rows[0].part || strlen(h) >= sizeof rows[0].hex)
            die("row field too long", line);
        strcpy(rows[nrows].arch, a); strcpy(rows[nrows].part, p); strcpy(rows[nrows].hex, h);
        nrows++;
    }
    fclose(f);
}

static int hexval(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/* first row matching arch/part; returns its hex string or dies by name */
static const char *first(const char *arch, const char *part) {
    int i;
    for (i = 0; i < nrows; i++)
        if (!strcmp(rows[i].arch, arch) && !strcmp(rows[i].part, part)) return rows[i].hex;
    fprintf(stderr, "unisa-hostabi: missing row %s %s\n", arch, part);
    exit(1);
}

/* validate even-length lowercase/uppercase hex; returns digit count */
static size_t check_hex(const char *h, const char *what) {
    size_t i, n = strlen(h);
    if (n == 0) { fprintf(stderr, "unisa-hostabi: empty %s\n", what); exit(1); }
    for (i = 0; i < n; i++)
        if (hexval(h[i]) < 0) { fprintf(stderr, "unisa-hostabi: bad hex in %s\n", what); exit(1); }
    return n;
}

/* words: count every row of arch/part; each row is one 32-bit word (8 hex digits) */
static void print_words(const char *name, const char *arch, const char *part) {
    int i, count = 0;
    static char acc[MAXROWS * 8 + 1];
    acc[0] = 0;
    for (i = 0; i < nrows; i++) {
        if (strcmp(rows[i].arch, arch) || strcmp(rows[i].part, part)) continue;
        if (check_hex(rows[i].hex, name) != 8) { fprintf(stderr, "unisa-hostabi: %s word width != 8\n", name); exit(1); }
        strcat(acc, rows[i].hex);
        count++;
    }
    if (count == 0) { fprintf(stderr, "unisa-hostabi: missing row %s %s\n", arch, part); exit(1); }
    emit(name, count, acc);
}

/* single value: one row, hex is the whole word */
static void print_one(const char *name, const char *arch, const char *part) {
    const char *h = first(arch, part);
    if (check_hex(h, name) != 8) { fprintf(stderr, "unisa-hostabi: %s width != 8\n", name); exit(1); }
    emit(name, 1, h);
}

/* bytes: first row; hex digit pairs; odd length refused */
static void print_bytes(const char *name, const char *arch, const char *part) {
    const char *h = first(arch, part);
    size_t n = check_hex(h, name);
    if (n % 2) { fprintf(stderr, "unisa-hostabi: %s odd hex length\n", name); exit(1); }
    emit(name, (int)(n / 2), h);
}

int main(int argc, char **argv) {
    if (argc != 3 || strcmp(argv[1], "dump")) {
        fprintf(stderr, "usage: unisa-hostabi dump TSV\n");
        return 2;
    }
    load(argv[2]);
    print_one("ARM_FN", "arm64", "fn");
    print_one("ARM_ARGV", "arm64", "argv");
    print_words("ARM_BODY", "arm64", "body");
    print_bytes("X86_BODY", "x86_64", "body");
    print_words("WIN_ARM_BODY", "win_arm64", "body");
    print_bytes("WIN_X86_BODY", "win_x86_64", "body");
    fputs(obuf, stdout);
    return 0;
}
