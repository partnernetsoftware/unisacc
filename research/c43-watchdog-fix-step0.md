# c43 看门狗修法草稿（第0步，只是设计与回归，2026-10-15）

**不落 tests/**：登记窗在 cdx。补丁为 `research/c43-watchdog-patch-draft.diff`（对 `tests/bound.py`），回归为 `research/c43-watchdog-regression.sh`。交窗后由 cc 派落。

## 1. 样本

- 样本一（`c43-watchdog-356s.md`）：墙钟 356 秒。
- 样本二（cc 报告）：`lib-variadic-native`，墙钟 344 秒，内层 cc ASan 编译 20 秒 TimeoutExpired。**日志已被 14 秒的绿色重跑覆盖**（`/tmp/c43acc.lib-variadic-native.log`，rc0），本机无法取证；机制与样本一相同，依据为 cc 的报告与 `bound.py` 源码。

## 2. 机制（已复现）

后代靠父子链找到。中间进程先退出后，它的子进程被 reparent 到 PID 1 且在自己的 session 里，外层超时的遍历与 killpg 都够不着。回归 R1 在原 `bound.py` 上失败（孤儿 `sleep 37` 超时后仍存活）；R2 失败（调用方的管道等待 38 秒才 EOF）。

## 3. 修法（草稿：`research/c43-watchdog-patch-draft.diff`）

1. **范围标记**：每次 `bound` 生成一个随机令牌，写入子进程环境变量 `UNISACC_BOUND_CHAIN`，内容为“外层链 + 本层令牌”。嵌套的 `bound` 会追加自己的令牌，因此外层能看到内层的全部后代。
2. **按标记收集**：Linux 读取 `/proc/<pid>/environ`；macOS 用 `ps -axwwE` 读取环境（**未在 macOS 验证**，本机是 Linux）。收集不依赖父子链，所以孤儿与 `setsid` 的后代都能找到。
3. **超时清理**：先 SIGSTOP 直到没有新增，再 SIGKILL 全部标记进程，最后等待直接子进程。原有的父子链遍历保留。
4. **调用方不等 EOF**：超时清理后，持有管道写端的后代已死，EOF 正常到达。因此不需要改调用方，修复本身解决了 EOF 问题。

## 4. 回归（已在本机运行）

| 用例 | 原 `bound.py` | 草稿 |
|---|---|---|
| R1 孤儿内层子进程在外层超时后残留 | 失败（残留，rc142） | 通过（rc142，无残留） |
| R2 调用方经管道读取，等待 EOF | 失败（38 秒） | 通过（3 秒） |
| 正常退出（`exit 3`） | — | rc=3，输出正确 |
| 普通超时（`sleep 30` 限 2 秒） | — | rc=142 |

回归脚本：`sh research/c43-watchdog-regression.sh BOUND_PY`，两条用例，总计约 10 秒。

## 5. 限制与未完成

- **macOS 分支未验证**：`ps -axwwE -o pid=,command=` 的输出格式需要在 Darwin 上核对；Darwin 宿主不在本机。CLAUDE.md 规定 macOS 是正式覆盖之一，因此落地前必须在 m4pro 上跑一遍回归。
- **`tests/bound.c` 未修**：门禁里大多数作业用 C 版。C 版需要同样的标记与扫描（Linux 读 `/proc`，macOS 用 `ps`），这是另一笔补丁，本草稿不包含。
- **未证明样本一、二的原因**：本修法消除了已复现的漏洞，但 356 与 344 秒的全部墙钟来源仍未取证（见 `c43-watchdog-356s.md` 第 3 节）。
- **不改其他行为**：正常路径无额外开销（扫描只在超时时发生）。

## 6. 交窗后的顺序

1. 落 `tests/bound.py`（本草稿）与 `tests/bound.c`（另笔）。
2. 把回归脚本接入门禁（登记窗）。
3. 在 m4pro 上核对 macOS 分支，再推广。
