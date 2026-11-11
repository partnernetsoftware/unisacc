# bound.py 超时后 stop() 为何耗时 889s（只读分析；只是设计）

**证据边界：** 现状来自读 `tests/bound.py` 源码与本机 /proc 采样；未在 c9f4d792 上重放失败命令（会吃 CPU，待 cc 授权单次运行）。结论为“候选 + 排除”，不是定论。

## 1. 现象

`python3 tests/bound.py 58 ./exec/parse2/librarytypedreadercheck.sh --prepare-base`（HEAD c9f4d792，需先 `--prepare-tools`）：rc 142，墙钟 889 s。58 s 的限时没有截住。日志 `/home/box/c43/reader---prepare-base.log` 仅一行 `parse2/librarytyped states 10131`，没有时间戳，无法看出卡在哪一步。

## 2. stop() 的执行序列（tests/bound.py，超时后调用）

1. 忽略 TERM/INT/HUP（阻止清理被打断）。
2. SIGSTOP 直接子进程。
3. 循环：`/bin/ps -axo pid=,ppid=` 找 owned 的后代，逐个 SIGSTOP，直到不再新增（无上限）。
4. killpg(SIGKILL) + 对 owned 逐个 SIGKILL。
5. 最多 50 轮：`scoped_pids(token)`（扫 /proc/*/environ 找带令牌的进程），SIGSTOP 新增者。
6. 对 `scoped_pids | owned` 再 SIGKILL。
7. `process.wait()`，**无超时**。

## 3. 候选与排除

| 候选 | 证据 | 判断 |
|---|---|---|
| 步骤 5 的 50 轮扫描太慢 | 本机 `scoped_pids` 一次 0.010 s，50 次 0.33 s（367 个进程，D 态 0 个） | 正常负载下排除；只有某次 /proc/*/environ 读取阻塞时才会慢 |
| 步骤 3 的后代循环不收敛（分叉风暴） | 无上限，但每轮只 SIGSTOP 新出现者，分叉需要不断产生 | 未见证据；需轮数计时确认 |
| **步骤 7 `process.wait()` 无超时，直接子进程处于 D 态** | SIGKILL 对 D 态挂起，直到 I/O 返回才生效；本任务写 1.49 MiB 的 typed 网络，正是 I/O 重的阶段；wait 无上限解释了“最终返回而非永远挂”的 889 s | **首要候选**，与 rc 142 一致（超时路径返回） |
| 步骤 5/6 对 D 态进程的 environ 读取阻塞 | 读 /proc/<pid>/environ 需要目标进程的 mm 锁，D 态进程可能持有 | 次要候选；与 D 态候选同源 |
| 外层 `tests/bound` 与内层 bound 的嵌套 | prepare-base 脚本内每个步骤是 `tests/bound 55`；外层 58 s 的计时只覆盖最外层 | 需确认外层是否就是 tests/bound.py（脚本第 2 行已见 `tests/bound 55`） |

结论：首要怀疑 **步骤 7 的无超时 wait 叠加 D 态子进程**；它解释 889 s 的量级，但未在失败现场证实。

## 4. 复现与计时（仓外，待授权）

已在 `/home/box/c43/bound-probe/bound.py` 做了与 tests/bound.py 行为相同的计时副本：设置 `BOUND_TRACE=<路径>` 时，stop() 每个阶段追加一行（相对时间与阶段名）：`stop begin`、`tree round N owned=K`、`tree done`、`killpg + SIGKILL owned`、`scope loop N fresh=K`、`scope loop end`、`final SIGKILL sweep`、`process.wait returned`。不设置时行为与原文件相同。

建议授权一次运行：用该副本替换 `tests/bound.py` 的调用（仓外拷贝的 tests 树），执行同一条失败命令，`BOUND_TRACE` 记录。若 trace 停在 `final SIGKILL sweep` 与 `process.wait returned` 之间，即证实候选 7；若停在 `scope loop` 则是候选 5/6。

## 5. 修法草稿（待证据后定）

1. **给 stop() 一个总时限**（例如 10 s）：超时后直接 `os.waitpid(WNOHANG)` 放弃，返回 142，并把未回收的 pid 写入日志；不再无限等待。
2. **process.wait 改为有限等待**：`wait(timeout=...)`，超时则记录“直接子进程未回收（疑似 D 态）”后返回；不得因此把结果判为绿。
3. **扫描加上界**：`scoped_pids` 的单次扫描限制时间，超时跳过该 pid。
4. 把 stop 各阶段的时间戳写入作业 sidecar（与 gate timing 同一旁路目录），让下次出现可直接读。

修法不改变超时判定（142）与清理顺序；只让“清理超时”变成可见的失败记录，而不是无限期占用墙钟。

## 6. 本笔不做的事

- 不改 tests/bound.py 与门禁登记；不在主检出提交修法。
- 不运行失败命令（需授权）。
- 不改 cdx 的 reader 脏树文件。
