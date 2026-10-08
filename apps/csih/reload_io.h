/* reload_io.h — 热更新 handoff 公共接口。
 * 缩栈核验分支。可信祖先 / 单写者。
 * 现仅 save 有实现(见 reload_io.c)；load/consume 未实现。
 */
#ifndef RELOAD_IO_H
#define RELOAD_IO_H

#include <stddef.h>
#include <sys/types.h>

/* 一次热更新 handoff 的身份令牌。 */
typedef struct {
    dev_t  dev;
    ino_t  ino;
    char   session_id[4096];
    char   handoff_id[4096];
    char   candidate_hash[65];
} reload_io_token;

/* save: 现有签名，写 path/text 并记录 why/cap；实现见 reload_io.c。 */
int reload_io_save(const char *path, const char *text, size_t len,
                   char *why, size_t cap);

/* load: 读出 path，校验 expected_session/handoff/hash；
 * 成功不删文件；out 写回，token 带出处身份。未实现。 */
int reload_io_load(const char *path,
                   const char *expected_session,
                   const char *expected_handoff,
                   const char *expected_hash,
                   char *out, size_t out_cap, size_t *out_len,
                   reload_io_token *token,
                   char *why, size_t cap);

/* consume: 仅在已提交恢复后调用，按 token 收尾；未实现。 */
int reload_io_consume(const char *path,
                      const reload_io_token *token,
                      char *why, size_t cap);

#endif /* RELOAD_IO_H */
