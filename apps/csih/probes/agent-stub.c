/* agent-stub.c — C port of agent-stub.py.
 * Scripted stand-in for the DeepSeek chat/completions API. Same reply shape,
 * same decision logic:
 *   - last real user message contains "Decide only: continue" -> {"go":"stop"}
 *   - no tool-result messages yet                             -> exec action
 *   - otherwise                                               -> answer
 * Tool-result = role "tool", or role "user" whose content starts "[tool]\n".
 * Build:  /bin/sh unisacc.com probes/agent-stub.c -o OUT
 * Run:    OUT [port]          (default 19001)
 */
#include <arpa/inet.h>
#include <ctype.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define REQ_MAX (1 << 22)

typedef struct {
    char *role;     /* NULL when missing or non-string */
    char *content;  /* "" when missing, null or non-string */
} Msg;

typedef struct {
    const char *p;
    const char *end;
    int bad;
} J;

static char *dup_n(const char *s, size_t n) {
    char *d = malloc(n + 1);
    if (!d) exit(1);
    memcpy(d, s, n);
    d[n] = 0;
    return d;
}

static void skip_ws(J *j) {
    while (j->p < j->end && isspace((unsigned char)*j->p)) j->p++;
}

/* Parse a JSON string at j->p (must point at '"'). Decodes escapes into a
 * fresh malloc'd buffer. Returns NULL and sets bad on error. */
static char *parse_str(J *j) {
    const char *q = j->p + 1;
    size_t raw = 0, k = 0;
    char *out, *o;
    if (j->p >= j->end || *j->p != '"') { j->bad = 1; return NULL; }
    while (q < j->end && *q != '"') {
        if (*q == '\\') { q++; if (q >= j->end) { j->bad = 1; return NULL; } }
        q++; raw++;
    }
    if (q >= j->end) { j->bad = 1; return NULL; }
    out = malloc(raw + 1);
    if (!out) exit(1);
    o = out;
    q = j->p + 1;
    while (q < j->end && *q != '"') {
        if (*q != '\\') { *o++ = *q++; continue; }
        q++;
        switch (*q) {
        case 'n': *o++ = '\n'; break;
        case 't': *o++ = '\t'; break;
        case 'r': *o++ = '\r'; break;
        case 'b': *o++ = '\b'; break;
        case 'f': *o++ = '\f'; break;
        case 'u': {
            unsigned cp = 0; int i;
            for (i = 1; i <= 4 && q + i < j->end; i++) {
                char c = q[i];
                cp = cp * 16 + (unsigned)(isdigit((unsigned char)c) ? c - '0'
                                          : (tolower((unsigned char)c) - 'a' + 10));
            }
            q += 4;
            if (cp < 0x80) { *o++ = (char)cp; }
            else if (cp < 0x800) { *o++ = (char)(0xC0 | (cp >> 6)); *o++ = (char)(0x80 | (cp & 0x3F)); }
            else { *o++ = (char)(0xE0 | (cp >> 12)); *o++ = (char)(0x80 | ((cp >> 6) & 0x3F)); *o++ = (char)(0x80 | (cp & 0x3F)); }
            break;
        }
        default: *o++ = *q; break; /* \" \\ \/ */
        }
        q++;
    }
    *o = 0;
    (void)k;
    j->p = q + 1;
    return out;
}

static void skip_value(J *j);

static void skip_value(J *j) {
    char *s;
    skip_ws(j);
    if (j->p >= j->end) { j->bad = 1; return; }
    if (*j->p == '"') { s = parse_str(j); free(s); return; }
    if (*j->p == '{' || *j->p == '[') {
        char close = *j->p == '{' ? '}' : ']';
        int obj = close == '}';
        j->p++;
        skip_ws(j);
        if (j->p < j->end && *j->p == close) { j->p++; return; }
        for (;;) {
            if (obj) {
                skip_ws(j);
                s = parse_str(j); free(s);
                if (j->bad) return;
                skip_ws(j);
                if (j->p >= j->end || *j->p != ':') { j->bad = 1; return; }
                j->p++;
            }
            skip_value(j);
            if (j->bad) return;
            skip_ws(j);
            if (j->p >= j->end) { j->bad = 1; return; }
            if (*j->p == ',') { j->p++; continue; }
            if (*j->p == close) { j->p++; return; }
            j->bad = 1; return;
        }
    }
    /* literal or number */
    while (j->p < j->end && !strchr(",]} \t\r\n", *j->p)) j->p++;
}

static void parse_message(J *j, Msg *m) {
    m->role = NULL;
    m->content = dup_n("", 0);
    if (j->p >= j->end || *j->p != '{') { skip_value(j); return; }
    j->p++;
    skip_ws(j);
    if (j->p < j->end && *j->p == '}') { j->p++; return; }
    for (;;) {
        char *key;
        skip_ws(j);
        key = parse_str(j);
        if (j->bad) { free(key); return; }
        skip_ws(j);
        if (j->p >= j->end || *j->p != ':') { j->bad = 1; free(key); return; }
        j->p++;
        skip_ws(j);
        if (strcmp(key, "role") == 0 && j->p < j->end && *j->p == '"') {
            free(m->role);
            m->role = parse_str(j);
        } else if (strcmp(key, "content") == 0 && j->p < j->end && *j->p == '"') {
            free(m->content);
            m->content = parse_str(j);
        } else {
            skip_value(j);
        }
        free(key);
        if (j->bad) return;
        skip_ws(j);
        if (j->p >= j->end) { j->bad = 1; return; }
        if (*j->p == ',') { j->p++; continue; }
        if (*j->p == '}') { j->p++; return; }
        j->bad = 1; return;
    }
}

/* Parse the messages array. Appends into *out (count *n). */
static void parse_messages(J *j, Msg **out, size_t *n) {
    size_t cap = 0;
    j->p++; /* '[' */
    skip_ws(j);
    if (j->p < j->end && *j->p == ']') { j->p++; return; }
    for (;;) {
        Msg m;
        if (*n == cap) {
            cap = cap ? cap * 2 : 16;
            *out = realloc(*out, cap * sizeof(Msg));
            if (!*out) exit(1);
        }
        skip_ws(j);
        parse_message(j, &m);
        if (j->bad) { free(m.role); free(m.content); return; }
        (*out)[(*n)++] = m;
        skip_ws(j);
        if (j->p >= j->end) { j->bad = 1; return; }
        if (*j->p == ',') { j->p++; continue; }
        if (*j->p == ']') { j->p++; return; }
        j->bad = 1; return;
    }
}

/* Top-level: object with a "messages" key. Any parse failure -> no messages
 * (python: messages = [] on exception). */
static void parse_request(const char *buf, size_t len, Msg **out, size_t *n) {
    J j = { buf, buf + len, 0 };
    *out = NULL; *n = 0;
    skip_ws(&j);
    if (j.p >= j.end || *j.p != '{') return;
    j.p++;
    skip_ws(&j);
    if (j.p < j.end && *j.p == '}') return;
    for (;;) {
        char *key;
        skip_ws(&j);
        key = parse_str(&j);
        if (j.bad) { free(key); goto fail; }
        skip_ws(&j);
        if (j.p >= j.end || *j.p != ':') { free(key); goto fail; }
        j.p++;
        skip_ws(&j);
        if (strcmp(key, "messages") == 0 && j.p < j.end && *j.p == '[') {
            parse_messages(&j, out, n);
        } else {
            skip_value(&j);
        }
        free(key);
        if (j.bad) goto fail;
        skip_ws(&j);
        if (j.p >= j.end) goto fail;
        if (*j.p == ',') { j.p++; continue; }
        if (*j.p == '}') return;
        goto fail;
    }
fail:
    {
        size_t i;
        for (i = 0; i < *n; i++) { free((*out)[i].role); free((*out)[i].content); }
        free(*out); *out = NULL; *n = 0;
    }
}

static int is_tool_result(const Msg *m) {
    if (m->role && strcmp(m->role, "tool") == 0) return 1;
    return m->role && strcmp(m->role, "user") == 0 &&
           strncmp(m->content, "[tool]\n", 7) == 0;
}

/* Returns the full HTTP-body JSON reply (static strings; wire escaping as
 * python json.dumps would produce for the inner content). */
static const char *decide(const Msg *msgs, size_t n) {
    size_t i, tool_count = 0;
    const char *last_user = NULL;
    for (i = 0; i < n; i++) if (is_tool_result(&msgs[i])) tool_count++;
    for (i = n; i-- > 0;) {
        if (msgs[i].role && strcmp(msgs[i].role, "user") == 0 && !is_tool_result(&msgs[i])) {
            last_user = msgs[i].content;
            break;
        }
    }
    if (last_user && *last_user && strstr(last_user, "Decide only: continue"))
        return "{\"id\": \"stub\", \"choices\": [{\"message\": {\"role\": \"assistant\", \"content\": \"{\\\"go\\\":\\\"stop\\\"}\"}}]}";
    if (tool_count == 0)
        return "{\"id\": \"stub\", \"choices\": [{\"message\": {\"role\": \"assistant\", \"content\": \"{\\\"act\\\":\\\"exec\\\",\\\"cmd\\\":\\\"echo hello-from-agent\\\"}\"}}]}";
    return "{\"id\": \"stub\", \"choices\": [{\"message\": {\"role\": \"assistant\", \"content\": \"{\\\"act\\\":\\\"answer\\\",\\\"text\\\":\\\"\\\\u5b8c\\\\u6210\\\\u4e86\\\"}\"}}]}";
}

static int strncasecmp_local(const char *a, const char *b, size_t n);

/* Read one HTTP request; returns the body bytes (malloc'd) and length. */
static char *read_request(int c, size_t *blen) {
    char *req = malloc(REQ_MAX + 1);
    size_t have = 0, hdr = 0, want = 0;
    int got_hdr = 0;
    char *body;
    if (!req) exit(1);
    for (;;) {
        ssize_t k;
        if (have >= REQ_MAX) break;
        k = read(c, req + have, REQ_MAX - have);
        if (k <= 0) break;
        have += (size_t)k;
        if (!got_hdr) {
            char *e = NULL, *p;
            for (p = req; p + 3 < req + have; p++)
                if (p[0] == '\r' && p[1] == '\n' && p[2] == '\r' && p[3] == '\n') { e = p; break; }
            if (e) {
                char *h, *line;
                got_hdr = 1;
                hdr = (size_t)(e - req) + 4;
                /* case-insensitive Content-Length scan over header lines */
                for (h = req; h < e; ) {
                    line = h;
                    while (h < e && *h != '\r') h++;
                    if (h - line > 15 && strncasecmp_local(line, "content-length:", 15) == 0)
                        want = (size_t)strtoul(line + 15, NULL, 10);
                    while (h < e && (*h == '\r' || *h == '\n')) h++;
                }
            }
        }
        if (got_hdr && have >= hdr + want) break;
    }
    if (!got_hdr) { *blen = 0; free(req); return dup_n("", 0); }
    *blen = have - hdr < want ? have - hdr : want;
    body = dup_n(req + hdr, *blen);
    free(req);
    return body;
}

static int strncasecmp_local(const char *a, const char *b, size_t n) {
    /* case-insensitive compare, used for the Content-Length header name */
    size_t i;
    for (i = 0; i < n; i++) {
        int x = tolower((unsigned char)a[i]), y = tolower((unsigned char)b[i]);
        if (x != y) return x - y;
    }
    return 0;
}

static void serve_one(int c) {
    size_t blen = 0, n = 0, i;
    char *body = read_request(c, &blen);
    Msg *msgs = NULL;
    const char *reply;
    char head[256];
    /* empty or invalid body -> no messages (python json fails -> []) */
    parse_request(body, blen, &msgs, &n);
    reply = decide(msgs, n);
    snprintf(head, sizeof head,
             "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n"
             "Content-Length: %d\r\nConnection: close\r\n\r\n", (int)strlen(reply));
    write(c, head, strlen(head));
    write(c, reply, strlen(reply));
    for (i = 0; i < n; i++) { free(msgs[i].role); free(msgs[i].content); }
    free(msgs);
    free(body);
}

int main(int argc, char **argv) {
    int port = argc > 1 ? atoi(argv[1]) : 19001;
    int s, c, one = 1;
    struct sockaddr_in a;
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) { printf("FAIL socket\n"); return 1; }
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
    memset(&a, 0, sizeof a);
    a.sin_family = AF_INET;
    a.sin_port = htons((unsigned short)port);
    a.sin_addr.s_addr = htonl(0x7f000001);
    if (bind(s, (struct sockaddr *)&a, sizeof a) != 0) { printf("FAIL bind\n"); return 1; }
    if (listen(s, 16) != 0) { printf("FAIL listen\n"); return 1; }
    for (;;) {
        c = accept(s, NULL, NULL);
        if (c < 0) continue;
        serve_one(c);
        close(c);
    }
    return 0;
}
