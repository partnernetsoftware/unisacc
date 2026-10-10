/* reload_session.h — v2 会话 codec 公共头。
 * codec 已实现；调用方负责静态或堆分配 reload_session_state。
 * v2 严格：不自动降级 v1。
 */
#ifndef RELOAD_SESSION_H
#define RELOAD_SESSION_H

#include <stddef.h>

typedef struct {
    char session_id[4096];
    char handoff_id[4096];
    char candidate_hash[65];
    char goal[4096];
    char cwd[4096];
    char role[4096];
    char peer[4096];
    char journal_path[4096];
    char input[4096];
    char history_draft[4096];
    unsigned long long journal_offset;
    char pending[8][4096];
    char history[16][4096];
    size_t npending;
    size_t nhistory;
    size_t history_pos;
    int history_browsing;
    int loop_on;
    int loop_left;
} reload_session_state;

/* 文本无内嵌 NUL 且为合法 UTF-8；失败写 why 并返回 0。被 reload_state.c 与 tui.c 共用。 */
int rs_no_embedded_nul(const char *text, size_t len, char *why, size_t cap);

/* 解析 v2 会话；成功返回 0，失败返回 -1 并清零输出。 */
int reload_session_decode_v2(const char *text, size_t len,
                             reload_session_state *out,
                             char *why, size_t cap);

/* 序列化 v2 会话；成功返回 0，失败返回 -1 并清零输出。
 * 写出长度经 out_len 返回，不含结尾 NUL。 */
int reload_session_encode_v2(const reload_session_state *st,
                             char *buf, size_t cap, size_t *out_len,
                             char *why, size_t cap_why);

#endif /* RELOAD_SESSION_H */
