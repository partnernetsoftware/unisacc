#ifndef CSIH_MESSAGE_IO_H
#define CSIH_MESSAGE_IO_H

#include "csih_message.h"

/* 取一条待处理消息（ready→started 持久确认后才算成功）。
 * 返回：
 *   1  task 已取
 *   2  notice 已取
 *   0  无可取 或 锁 busy（out 清零）
 *  -1  未提交错误（out 清零）
 *  -2  已转换但持久化或关闭未确认（不得执行，out 清零）
 * tasks_allowed 为 0/1；为 0 时仍可取 notice。
 * 仅由唯一 ACTIVE owner 调用；可信祖先/协作 mailbox 记录锁前提。
 * 不认证来源，不承诺崩溃下 exactly-once。 */
int csih_message_take(const char *session_dir,
                      const char *expected_session,
                      int tasks_allowed,
                      csih_message *out,
                      char *why, size_t cap);

/* 把已 started 的消息标记为 done。原五键文件保持，不删除，
 * 不自动重试 started；结果原因由 TUI journal 记录。
 * 返回：
 *   0  成功
 *  -1  未提交错误
 *  -2  已转换但持久化或关闭未确认
 * 仅由唯一 ACTIVE owner 调用。不认证来源，不承诺崩溃下 exactly-once。 */
int csih_message_finish(const char *session_dir,
                        const char *expected_session,
                        const char *id,
                        char *why, size_t cap);

#endif /* CSIH_MESSAGE_IO_H */
