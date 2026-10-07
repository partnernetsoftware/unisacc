/*
 * json_cli.c — the `main` for json.c, moved out so json.c can be a library.
 *
 * WHY: see gate_cli.c. One `main` per combined program is a hard unisacc rule —
 * two files with `main` fail as
 *     reject: not covered: ARM64 operand or instruction
 *     arm64: main:
 * which reads like a missing backend feature. loop.c drives the libraries, so
 * json.c drops `main`. This file owns stdin and json_run_selftest, so the
 * library does not carry assertions.
 *
 * The CLI takes SUBCOMMANDS, not dash-options (unisacc reserves the dashes).
 *
 * BUILD (either backend; the two must agree):
 *     unisacc json.c json_cli.c selftest
 *     cc -std=c99 json.c json_cli.c -o /tmp/json && /tmp/json selftest
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── the slice of json.c this CLI uses (types restated so this file compiles
 *    on its own; unisacc can see an earlier file's type, but that is
 *    order-dependent) ──────────────────────────────────────────────────────── */
typedef enum { J_NULL, J_BOOL, J_NUM, J_STR, J_ARR, J_OBJ } jkind;
typedef struct jvalue {
    jkind kind;
    int    b;
    double n;
    char  *s;
    struct jvalue **items;  size_t len;
    char **keys; struct jvalue **vals; size_t nkeys;
} jvalue;

jvalue *json_parse(const char *text, size_t len, char *errbuf, size_t errlen);
size_t  json_value_end(const char *text, size_t len);
void    jfree(jvalue *v);
size_t  json_escape(const char *in, char *out, size_t outlen, size_t *in_used);
size_t  json_rec(char *out, size_t cap,
                 const char *k1, const char *v1,
                 const char *k2, const char *v2,
                 const char *k3, const char *v3);
size_t  json_msg(char *buf, size_t cap, int comma,
                 const char *role, const char *lead, const char *content);
jvalue *jget(jvalue *obj, const char *key);
const char *jstr(jvalue *v);
double  jnum(jvalue *v, double dflt);
size_t  jlen(jvalue *v);

static int failures = 0;
static void expect(int cond, const char *what) {
    if (!cond) { printf("FAIL %s\n", what); failures++; }
}

int json_run_selftest(void) {
    char err[128];
    jvalue *v;

    v = json_parse("{\"a\":1,\"b\":\"x\"}", strlen("{\"a\":1,\"b\":\"x\"}"), err, sizeof(err));
    expect(v != NULL, "object parses");
    if (v) {
        expect(jnum(jget(v, "a"), -1) == 1, "number field");
        expect(jstr(jget(v, "b")) && !strcmp(jstr(jget(v, "b")), "x"), "string field");
        expect(jget(v, "zz") == NULL, "missing key is NULL");
        jfree(v);
    }

    {
        const char *line = "{\"role\":\"user\",\"n\":3,\"ok\":true,\"t\":[\"a\",\"b\"],\"s\":\"\\u4e2d\\u6587\"}";
        v = json_parse(line, strlen(line), err, sizeof(err));
        expect(v != NULL, "session-shaped line parses");
        if (v) {
            expect(jlen(jget(v, "t")) == 2, "array length");
            expect(jstr(jget(v, "s")) && !strcmp(jstr(jget(v, "s")), "\xe4\xb8\xad\xe6\x96\x87"), "\\u escape becomes UTF-8");
            expect(jget(v, "ok") && jget(v, "ok")->kind == J_BOOL && jget(v, "ok")->b == 1, "boolean");
            jfree(v);
        }
    }

    expect(json_parse("{\"a\":}", strlen("{\"a\":}"), err, sizeof(err)) == NULL, "missing value rejected");
    expect(json_parse("{\"a\":1} junk", strlen("{\"a\":1} junk"), err, sizeof(err)) == NULL, "trailing content rejected");
    expect(json_parse("[1,2", strlen("[1,2"), err, sizeof(err)) == NULL, "unterminated array rejected");
    expect(json_parse("\"unterminated", strlen("\"unterminated"), err, sizeof(err)) == NULL, "unterminated string rejected");
    expect(json_parse("", strlen(""), err, sizeof(err)) == NULL, "empty input rejected");

    v = json_parse("\"a\\nb\\t\\\"c\\\\d\"", strlen("\"a\\nb\\t\\\"c\\\\d\""), err, sizeof(err));
    expect(v && !strcmp(jstr(v), "a\nb\t\"c\\d"), "escapes decode");
    jfree(v);

    v = json_parse("\"\\ud83d\\ude00\"", strlen("\"\\ud83d\\ude00\""), err, sizeof(err));
    expect(v && jstr(v) && (unsigned char)jstr(v)[0] == 0xF0, "surrogate pair becomes 4-byte UTF-8");
    jfree(v);

    {
        char esc[64], wrapped[80];
        size_t seen = 0;
        json_escape("a\"b\\c\n\x01", esc, sizeof esc, &seen);
        expect(seen == 7, "escape consumes the whole input");
        expect(!strcmp(esc, "a\\\"b\\\\c\\n\\u0001"), "quotes, slash, newline, control");
        snprintf(wrapped, sizeof wrapped, "\"%s\"", esc);
        v = json_parse(wrapped, strlen(wrapped), err, sizeof err);
        expect(v && jstr(v) && !strcmp(jstr(v), "a\"b\\c\n\x01"), "escape round-trips");
        jfree(v);
    }
    {
        char raw[8], esc[32], wrapped[40];
        size_t seen = 0;
        raw[0] = (char)0xE7;
        raw[1] = (char)0xE5; raw[2] = (char)0xA6; raw[3] = (char)0x82;
        raw[4] = (char)0xE4; raw[5] = (char)0xBD; raw[6] = (char)0x95;
        raw[7] = 0;
        json_escape(raw, esc, sizeof esc, &seen);
        expect(seen == 7, "stray lead is consumed, not stuck");
        snprintf(wrapped, sizeof wrapped, "\"%s\"", esc);
        v = json_parse(wrapped, strlen(wrapped), err, sizeof err);
        expect(v && jstr(v) && !strcmp(jstr(v), "\xe5\xa6\x82\xe4\xbd\x95"), "stray lead dropped, 如何 kept");
        jfree(v);
    }
    {
        char tiny[4];
        size_t seen = 99;
        json_escape("\xe4\xb8\x80Z", tiny, 3, &seen);
        expect(tiny[0] == 0 && seen == 0, "3-byte character does not fit in 2 bytes");
        seen = 99;
        json_escape("\xe4\xb8\x80Z", tiny, 4, &seen);
        expect(!strcmp(tiny, "\xe4\xb8\x80") && seen == 3, "one complete character, then stop");
        seen = 99;
        json_escape("\"", tiny, 2, &seen);
        expect(tiny[0] == 0 && seen == 0, "quote escape is not a lone backslash");
    }

    expect(json_parse("\"\\ud800\"", strlen("\"\\ud800\""), err, sizeof err) == NULL,
           "lone high surrogate rejected");
    expect(json_parse("\"\\ude00\"", strlen("\"\\ude00\""), err, sizeof err) == NULL,
           "lone low surrogate rejected");
    expect(json_parse("1e", strlen("1e"), err, sizeof err) == NULL, "truncated exponent rejected");
    expect(json_parse("01", strlen("01"), err, sizeof err) == NULL, "leading zero rejected");
    {
        const char *span = " {\"a\":\"}\"} ";
        size_t e = json_value_end(span, strlen(span));
        expect(e == 10 && span[e - 1] == '}', "span closes after the brace inside the string");
        expect(json_value_end("{", 1) == 0, "unclosed object is not a span");
    }
    {
        char deep[80];
        int i;
        for (i = 0; i < 33; i++) deep[i] = '[';
        deep[33] = '1';
        for (i = 0; i < 33; i++) deep[34 + i] = ']';
        deep[67] = 0;
        expect(json_parse(deep, 67, err, sizeof err) == NULL, "nesting past 32 is rejected");
        expect(json_value_end(deep, 67) == 0, "a too-deep span is not a value");
    }

    {
        char buf[80];
        size_t n = json_rec(buf, sizeof buf, "role", "user", "text", "a\"b", NULL, NULL);
        expect(n > 0 && !strcmp(buf, "{\"role\":\"user\",\"text\":\"a\\\"b\"}"),
               "a record escapes its value");
        expect(json_rec(buf, 8, "role", "user", "text", "hello", NULL, NULL) == 0
               && buf[0] == 0, "a short record buffer stays empty");
        strcpy(buf, "stale");
        expect(json_msg(buf, 4, 0, "user", NULL, "hi") == 0 && buf[0] == 0,
               "a too-small message buffer is cleared, not left stale");
        n = json_msg(buf, sizeof buf, 1, "user", NULL, "x");
        expect(n > 0 && !strcmp(buf, ",{\"role\":\"user\",\"content\":\"x\"}"),
               "a message is role and content");
        n = json_msg(buf, sizeof buf, 0, "user", "[tool]\n", "z");
        expect(n > 0 && !strcmp(buf, "{\"role\":\"user\",\"content\":\"[tool]\\nz\"}"),
               "a message lead is escaped with the content");
    }

    v = json_parse("{}", strlen("{}"), err, sizeof(err)); expect(v != NULL, "{} parses"); jfree(v);
    v = json_parse("[]", strlen("[]"), err, sizeof(err)); expect(v != NULL, "[] parses"); jfree(v);

    v = json_parse("-1.5e3", strlen("-1.5e3"), err, sizeof(err));
    expect(v && jnum(v, 0) < -1499.0 && jnum(v, 0) > -1501.0, "negative exponent number");
    jfree(v);

    printf("%s\n", failures ? "SELFTEST FAILED" : "selftest ok");
    return failures == 0 ? 0 : 1;
}

/* Read JSONL from stdin, report one summary line per record. */
static int run_stream(void) {
    /* HEAP, not a 1 MB stack array. Measured on unisacc 0.0.17 / osx-arm64:
     * a stack array of 1<<20 is rejected with "not covered: ARM64 operand or
     * instruction", while 262144 compiles fine — so the limit is somewhere
     * between 256 KB and 1 MB. A heap buffer sidesteps the question entirely,
     * and is the right choice anyway for a line buffer whose size is a policy
     * (how long may one session record be?) rather than a fact about the
     * machine. Fixed size on the stack also silently truncates a long line. */
    const size_t CAP = 1u << 20;
    char *line = (char *)malloc(CAP);
    long n = 0, ok = 0, bad = 0;
    if (!line) { printf("out of memory\n"); return 2; }
    while (fgets(line, (int)CAP, stdin)) {
        size_t len = strlen(line);
        char err[128];
        jvalue *v;
        while (len && (line[len-1]=='\n' || line[len-1]=='\r')) line[--len] = '\0';
        if (!len) continue;
        n++;
        v = json_parse(line, len, err, sizeof(err));
        if (v) { ok++; jfree(v); }
        else { bad++; printf("line %ld: %s\n", n, err); }
    }
    printf("records %ld ok %ld bad %ld\n", n, ok, bad);
    free(line);
    return bad == 0 ? 0 : 1;
}

int main(int argc, char **argv) {
    /* Subcommands, not dash-options — see the header note. */
    if (argc > 1 && !strcmp(argv[1], "selftest")) return json_run_selftest();
    return run_stream();
}
