#ifndef CSIH_MESSAGE_H
#define CSIH_MESSAGE_H

#include <stddef.h>

/* 解码后的消息内容。id 定长 33 = 32 hex + NUL；body 定长 4096 上限。 */
typedef struct csih_message {
    char id[33];
    char body[4096];
    int  kind;
} csih_message;

/* 逐封消息模板校验。返回：失败 0，task 1，notice 2。
 * 成功时 why 写 "ok"。仅校验文本格式与 expected session，
 * 不验证、不认证消息发送者的身份。 */
int csih_message_validate(const char *text, size_t len,
                          const char *expected_session,
                          char *why, size_t cap);

/* 解码一条消息到 out。返回：成功 1(task)/2(notice)，且 out->kind 同返回；
 * 失败 0 并清 out（out 非 NULL 时）。实现见 csih_message.c。 */
int csih_message_decode(const char *text, size_t len,
                        const char *expected_session,
                        csih_message *out, char *why, size_t cap);

#endif /* CSIH_MESSAGE_H */
