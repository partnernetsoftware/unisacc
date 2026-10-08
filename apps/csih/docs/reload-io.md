# reload I/O 设计（未实现）

本文件只写设计。当前无任何 save/load/consume 实现；不得据此声称已接管。

## 拟定 C API（前缀统一 reload_io_）

- `reload_io_save(const char *path, const char *text, size_t len, char *why, size_t cap)`
- `reload_io_load(const char *path, const char *expected_session, const char *expected_handoff, const char *expected_hash, char *out, size_t out_cap, size_t *out_len, char *why, size_t cap)`
- `reload_io_consume(const char *path, token, char *why, size_t cap)`，token 为 `reload_io_load` 返回的文件凭据（dev/ino 及三身份，具体类型待 C 定义）

所有函数 `why` 为 `char *why`，容量为 `size_t cap`（不是 const why，也不是 char *cap 建议）。

返回：0 成功、-1 未提交失败。save 额外 -2：已 rename 但目录同步未确认。

`load` 三预期身份均 `const char*`，并返回文件凭据 token。`consume` 接受该 token，须再检查当前文件与 token 一致；只比 JSON 三身份不足以称同一文件。max 大小 131072；`out_cap` 必须能容纳 `len+1`。

## save 语义

保存前先验证 `text` 可解析、身份字段合法、`path` 位于私有可信会话目录。写入方式：同目录独占新建临时文件，权限 0600，完整写出后 `fsync`、`close`，再 `rename` 到目标。`rename` 前任何失败：保留旧文件原样，并清理临时文件。`rename` 已成功但随后目录 `fsync` 失败时，文件可能已提交 —— 此情况必须报告不确定，不得声称旧文件一定还在。

## load 语义

打开时不跟随符号链接；检查普通文件、属主为本人、权限 0600、大小在上限内。完整读入后校验 JSON 格式与 session/handoff/hash 三身份。任一失败只报错，不删除文件。成功后仅返回数据，不自动 consume。

## consume 语义

仅当调用者已提交恢复后才可 consume，且再次确认是同一文件身份。清理失败要如实报告。清理未确认成功前，不宣称接管成功。

## 边界与未实现

- rename 后目录 fsync 失败 → 可能已提交，不得断言旧文件必在。
- 同 session 假定单写者。
- 锁、监督、接管本阶段均未实现。
- 本阶段无实跑。

## 验收清单

1. 往返含 input 队列。
2. 权限检查（0600）。
3. 坏 JSON/错身份不删除文件。
4. 保存失败时旧内容仍在。
5. consume 失败路径如实报告。
