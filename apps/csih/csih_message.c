/* csih_message.c — 逐封消息校验库（分片实现，首片仅声明与 helper）
 * 本片不含 schema/NUL/公函数体，无 main。完整 spec 见下片。 */
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include "csih_message.h"


/* 失败：写 why 并返回 0。NULL why / cap 0 安全。 */
static int cm_fail(char *why, size_t cap, const char *msg) {
    if (why && cap) snprintf(why, cap, "%s", msg ? msg : "invalid");
    return 0;
}

/* 一个合法 UTF-8 序列的字节数，非法返回 0（照搬 json.c utf8_one）。 */
static int cm_utf8_one(const unsigned char *s, size_t avail) {
    unsigned char c;
    if (!s || avail == 0) return 0;
    c = s[0];
    if (c < 0x80) return 1;
    if (c < 0xC2 || c > 0xF4) return 0;
    if (c <= 0xDF) {
        if (avail < 2 || (s[1] & 0xC0) != 0x80) return 0;
        return 2;
    }
    if (c <= 0xEF) {
        if (avail < 3 || (s[1] & 0xC0) != 0x80 || (s[2] & 0xC0) != 0x80) return 0;
        if (c == 0xE0 && s[1] < 0xA0) return 0;
        if (c == 0xED && s[1] >= 0xA0) return 0;
        return 3;
    }
    if (avail < 4 || (s[1] & 0xC0) != 0x80 || (s[2] & 0xC0) != 0x80 || (s[3] & 0xC0) != 0x80)
        return 0;
    if (c == 0xF0 && s[1] < 0x90) return 0;
    if (c == 0xF4 && s[1] >= 0x90) return 0;
    return 4;
}

/* 整段字节须为良构 UTF-8；坏字节返回 0。 */
static int cm_utf8_valid(const char *text, size_t len) {
    size_t i = 0;
    while (i < len) {
        int n = cm_utf8_one((const unsigned char *)text + i, len - i);
        if (n == 0) return 0;
        i += (size_t)n;
    }
    return 1;
}

/* 拒真实 NUL 字节与真实 JSON \u0000 escape；\u0000（双反斜杠）字面放行。 */
static int cm_no_nul(const char *text, size_t len, char *why, size_t cap) {
    for (size_t i = 0; i < len; i++) {
        if (text[i] == '\0')
            return cm_fail(why, cap, "embedded NUL not representable");
        if (text[i] == '\\' && i + 1 < len) {
            if (text[i + 1] == 'u' && i + 5 < len &&
                text[i + 2] == '0' && text[i + 3] == '0' &&
                text[i + 4] == '0' && text[i + 5] == '0')
                return cm_fail(why, cap, "embedded NUL not representable");
            i++;  /* 跳过已转义的下一个字节；字面 \\\\u0000 保持原样 */
        }
    }
    return 1;
}

/* session 标识：非 NULL、非空、<=64，仅 [A-Za-z0-9:_-]。字节显式比较，不用 locale。 */
static int cm_safe_session(const char *s) {
    size_t n;
    if (!s) return 0;
    n = strlen(s);
    if (n == 0 || n > 64) return 0;
    for (size_t i = 0; i < n; i++) {
        unsigned char c = (unsigned char)s[i];
        int ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                 (c >= '0' && c <= '9') || c == ':' || c == '_' || c == '-';
        if (!ok) return 0;
    }
    return 1;
}

/* 校验一条逐封消息。失败0（why 写原因），task成功1，notice成功2，why=ok。
 * 模板校验仅格式，来源身份不在本类型保证内。 */
int csih_message_validate(const char *text, size_t len,
                          const char *expected_session,
                          char *why, size_t cap) {
    jvalue *root = NULL, *version, *id, *session, *kind, *body;
    const char *idv, *sv, *kv, *bv;
    int rc = 0;
    char err[256];
    static const char *keys[] = { "version", "id", "session", "kind", "body" };
    size_t nkeys = sizeof(keys) / sizeof(keys[0]);

    if (!text) return cm_fail(why, cap, "text: null");
    if (len == 0 || len > 32768) return cm_fail(why, cap, "len: 1..32768 required");
    if (!cm_safe_session(expected_session)) return cm_fail(why, cap, "expected_session: invalid");
    if (!cm_no_nul(text, len, why, cap)) return 0;
    if (!cm_utf8_valid(text, len)) return cm_fail(why, cap, "text: invalid UTF-8");

    root = json_parse(text, len, err, sizeof err);
    if (!root) { char b[320]; snprintf(b, sizeof b, "parse: %s", err); return cm_fail(why, cap, b); }
    if (root->kind != J_OBJ) { cm_fail(why, cap, "root: not object"); goto done; }

    /* 每键恰一次：遍历 nkeys 拒未知，再逐键 jget 拒缺失。 */
    for (size_t i = 0; i < root->nkeys; i++) {
        int known = 0;
        for (size_t k = 0; k < nkeys; k++)
            if (!strcmp(root->keys[i], keys[k])) { known = 1; break; }
        if (!known) {
            char b[96]; snprintf(b, sizeof b, "unknown key: %.48s", root->keys[i]);
            cm_fail(why, cap, b); goto done;
        }
    }
    if (root->nkeys != nkeys) { cm_fail(why, cap, "need exactly 5 keys"); goto done; }
    for (size_t k = 0; k < nkeys; k++) {
        int cnt = 0;
        for (size_t i = 0; i < root->nkeys; i++)
            if (!strcmp(root->keys[i], keys[k])) cnt++;
        if (cnt == 0) { char b[96]; snprintf(b, sizeof b, "missing: %s", keys[k]); cm_fail(why, cap, b); goto done; }
        if (cnt > 1) { char b[96]; snprintf(b, sizeof b, "duplicate: %s", keys[k]); cm_fail(why, cap, b); goto done; }
    }

    version = jget(root, "version"); id = jget(root, "id");
    session = jget(root, "session"); kind = jget(root, "kind");
    body = jget(root, "body");
    if (!version) { cm_fail(why, cap, "missing: version"); goto done; }
    if (!id)      { cm_fail(why, cap, "missing: id"); goto done; }
    if (!session) { cm_fail(why, cap, "missing: session"); goto done; }
    if (!kind)    { cm_fail(why, cap, "missing: kind"); goto done; }
    if (!body)    { cm_fail(why, cap, "missing: body"); goto done; }

    if (version->kind != J_NUM || version->n != 1.0) { cm_fail(why, cap, "version: must be 1"); goto done; }
    if (id->kind != J_STR) { cm_fail(why, cap, "id: not string"); goto done; }
    idv = jstr(id);
    if (strlen(idv) != 32) { cm_fail(why, cap, "id: need 32 hex"); goto done; }
    for (size_t i = 0; i < 32; i++) {
        char c = idv[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) {
            cm_fail(why, cap, "id: need 32 lowercase hex"); goto done;
        }
    }
    if (session->kind != J_STR) { cm_fail(why, cap, "session: not string"); goto done; }
    sv = jstr(session);
    if (!cm_safe_session(sv)) { cm_fail(why, cap, "session: unsafe"); goto done; }
    if (strcmp(sv, expected_session) != 0) { cm_fail(why, cap, "session: mismatch"); goto done; }
    if (kind->kind != J_STR) { cm_fail(why, cap, "kind: not string"); goto done; }
    kv = jstr(kind);
    if (strcmp(kv, "task") != 0 && strcmp(kv, "notice") != 0) {
        cm_fail(why, cap, "kind: task|notice only"); goto done;
    }
    if (body->kind != J_STR) { cm_fail(why, cap, "body: not string"); goto done; }
    bv = jstr(body);
    if (strlen(bv) == 0) { cm_fail(why, cap, "body: empty"); goto done; }
    if (strlen(bv) > 4095) { cm_fail(why, cap, "body: over 4095 bytes"); goto done; }
    if (!cm_utf8_valid(bv, strlen(bv))) { cm_fail(why, cap, "body: invalid UTF-8"); goto done; }

    if (why && cap) snprintf(why, cap, "ok");
    rc = (strcmp(kv, "notice") == 0) ? 2 : 1;

done:
    jfree(root);
    return rc;
}

/* 解码一条消息到 out。成功返回 1(task)/2(notice)，out->kind 同返回；
 * 失败 0 并清 out。仅模板格式校验，不认证发送者。 */
int csih_message_decode(const char *text, size_t len,
                        const char *expected_session,
                        csih_message *out, char *why, size_t cap) {
    jvalue *root = NULL, *id, *body;
    const char *idv, *bv;
    char err[256];
    int kind;

    if (!out) return cm_fail(why, cap, "out: null");
    memset(out, 0, sizeof *out);

    kind = csih_message_validate(text, len, expected_session, why, cap);
    if (kind == 0) return 0;

    root = json_parse(text, len, err, sizeof err);
    if (!root) {
        char b[320];
        snprintf(b, sizeof b, "parse: %s", err);
        memset(out, 0, sizeof *out);
        return cm_fail(why, cap, b);
    }

    id   = jget(root, "id");
    body = jget(root, "body");
    if (!id || id->kind != J_STR || !body || body->kind != J_STR) {
        jfree(root);
        memset(out, 0, sizeof *out);
        return cm_fail(why, cap, "decode: unexpected field");
    }

    idv = jstr(id);
    bv  = jstr(body);
    snprintf(out->id, sizeof out->id, "%s", idv);
    snprintf(out->body, sizeof out->body, "%s", bv);
    out->kind = kind;
    jfree(root);

    if (why && cap) snprintf(why, cap, "ok");
    return kind;
}

/* MESSAGE_NEXT */
