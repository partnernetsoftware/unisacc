# csih 自迭代闭环

执行层已经在：改过的 `.c` 下一次被拉起就是新行为。这一页只写验证怎么走同一条路，让改动能留下。

## 现状

`check.c` 的 `main` 用 `u_spawn`（`probes/u_run.c`）对每一组源文件再起一个 unisacc，子命令是 `selftest` 或探针。模块自测、`loop.c`、`probes/tui_e2e.c`、`probes/agent_roundtrip.c` 都已经在这条路上。

断点不在「能不能跑」，在三处：

1. 模型用 `file` 的 write/edit 改源文件成功之后，`agent_exec` 会用 `shell_run_in` 问 `suite_cli rows`，再对每一行拉起 unisacc 子命令 `selftest`。
2. 子进程退出码和第一行输出写回这一步的工具结果；红旗（任一相关 slice 的 rc 非 0）为真时，harness 拒绝 `answer` 和 `go:stop`，理由里带那一行失败。
3. 磁盘上的改动留着，不自动回滚，模型可以再改。

另外一条已测过的限制：unisacc 进程里的 `setenv` 过不了下一次 `exec`（`probes/agent_roundtrip.c` 开头）。子进程要的环境只能由宿主在拉起时放进 environ，或走已经存在的 `u_spawn_sh`。

正在跑的那一扇 TUI 不会热更新。上一轮窗 `0:18` 就是这样：旧进程继续用旧行为，新源文件要等这一扇被重新拉起。

内环已接：`suite.c` 是唯一 argv 表，`suite_cli.c` 是它的进程。TUI 不链这张表（`net.c` 的 `netdb.h` 已顶到 struct id）。`agent.c` 在写或改 csih 源文件成功后，用已有的 `shell_run_in` 拉起 `suite.c suite_cli.c rows <文件名>`，再按打印出的 argv 各起一次。本进程里的红旗为真时拒绝 `answer` 和 `go:stop`。`check.c` 的模块自测仍在进程内走这张表。

## 三环，一个拉起函数

只保留 `u_spawn` / `u_spawn_sh`。不另起编译器，不另写 shell 套件。

### 一张表

一张 C 表是唯一的 argv 清单。`check.c` 和代理门禁都读它。每一行：名字、源文件列表、环（`slice` 或 `outer`）。

一个被改到的 `.c` / `.h` 对应「源文件列表里点了它的那些 slice 行」。`json.c`、`session.c`、`gate.c` 再加跑 `loop` 那一行。`plugin.c` 出现在 agent 行和 tui 行里，和 `run.c` 对齐。

### 内环：每次改自己的源文件

门禁在 harness 里，不做成模型可以不叫的工具。

`agent_exec` 在 `file` 的 write/edit 成功、且路径落在 csih 源文件上之后，用 `shell_run_in` 问 `suite_cli rows`，再按它打印的每一行拉起 unisacc，子命令 `selftest`。子进程的退出码和第一行输出写回这一步的工具结果（`check.c` 的 `first_line` 已经是这个形状）。

`answer` 和 `go:stop` 在任一相关 slice 的 rc 非 0 时由 harness 拒绝，理由里带那一行失败。模型可以再改。磁盘上的改动留着，不自动回滚：回滚会把要修的差分藏掉。

内环不跑网络，不跑 DeepSeek。`agent_roundtrip` / `llm_roundtrip` 继续打桩，桩留在外环。

### 外环：才允许换上新的自己

整份 `check.c`（`unisacc check.c suite.c probes/u_run.c`）是晋升。红，就不重启 TUI，旧的那一扇继续当产品。绿，再重启窗。运行中的进程永远不是被改过的那一版。

Python 探针（`probes/term_size.py`）只留在外环，不再增加。

## 不放进闭环的事

- 不把「记得跑测试」写进系统提示当唯一约束。提示会忘，拒绝 `answer` 不会。
- 不在内环打真实模型。网络红不等于这改动红。
- 不在运行中的进程里热补。闭环的最后一步就是重新拉起。
