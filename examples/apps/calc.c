/* calc: a recursive-descent calculator with variables.
   Grammar:  stmt := NAME '=' expr | expr
             expr := term (('+'|'-') term)*
             term := unary (('*'|'/'|'%') unary)*
             unary := '-' unary | atom
             atom := NUMBER | NAME | '(' expr ')'                     */
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define NVARS 26

static const char *src;
static long vars[NVARS];
static int error;

static void skip(void) { while (*src == ' ') src++; }

static long expr(void);

static long atom(void) {
    long v = 0;
    skip();
    if (*src == '(') {
        src++;
        v = expr();
        skip();
        if (*src == ')') src++; else error = 1;
        return v;
    }
    if (isdigit((unsigned char)*src)) {
        while (isdigit((unsigned char)*src)) { v = v * 10 + (*src - '0'); src++; }
        return v;
    }
    if (*src >= 'a' && *src <= 'z') { v = vars[*src - 'a']; src++; return v; }
    error = 1;
    return 0;
}

static long unary(void) {
    skip();
    if (*src == '-') { src++; return -unary(); }
    return atom();
}

static long term(void) {
    long v = unary();
    for (;;) {
        char op;
        long r;
        skip();
        op = *src;
        if (op != '*' && op != '/' && op != '%') return v;
        src++;
        r = unary();
        if (op != '*' && r == 0) { error = 2; return 0; }
        if (op == '*') v = v * r; else if (op == '/') v = v / r; else v = v % r;
    }
}

static long expr(void) {
    long v = term();
    for (;;) {
        char op;
        skip();
        op = *src;
        if (op != '+' && op != '-') return v;
        src++;
        if (op == '+') v = v + term(); else v = v - term();
    }
}

static void run(const char *line) {
    int target = -1;
    long v;
    src = line;
    error = 0;
    skip();
    if (src[0] >= 'a' && src[0] <= 'z') {
        const char *p = src + 1;
        while (*p == ' ') p++;
        if (*p == '=') { target = src[0] - 'a'; src = p + 1; }
    }
    v = expr();
    skip();
    if (*src != 0 && !error) error = 1;
    if (error == 1) { printf("%-24s -> syntax error\n", line); return; }
    if (error == 2) { printf("%-24s -> division by zero\n", line); return; }
    if (target >= 0) { vars[target] = v; printf("%-24s -> %c = %ld\n", line, 'a' + target, v); }
    else printf("%-24s -> %ld\n", line, v);
}

int main(void) {
    static const char *lines[] = {
        "1 + 2 * 3",
        "(1 + 2) * 3",
        "x = 7",
        "y = x * x - 1",
        "-(y % 5) + x / 2",
        "z = (x + y) * (x - y)",
        "100 / (x - 7)",
        "2 * (3 + 4",
        "z / -4",
    };
    int i;
    for (i = 0; i < (int)(sizeof lines / sizeof lines[0]); i++) run(lines[i]);
    return 0;
}
