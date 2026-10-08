# Harness CLI 命令设计（草案，≤80行）

只读核实来源：`tui.c` 的 `tui_slash()`(878-960) 与 `tui_submit()`(999-1013)。
现状：**尚无 `/help`、`/clear`、`/status`、`/cancel`**（grep 全仓为空）。
真实已有：`/exit`、`/quit`、`/reload`、`/reload-code`、`/export-state`、`/goal`、`/loop`。
未知斜杠：`tui_slash` 返回 0 后**当作普通 prompt 交模型**（`tui_run_agent` 1013），当前不存在“报错不交模型”的行为，需本设计新增。

## 第一批

### `/help`
- 目的：列出所有 native 命令及一行用途；纯 UI，零模型请求。
- 参数：无。
- 忙/闲：两者都即时打印，不打断在飞轮。
- 保留/清理：只追加本地日志，不动 context、journal、memory、目标、history。
- 反馈：成功→命令清单；失败→无（不产生失败）。
- 验收：输入 `/help` 后 stdout/frame 含全部命令名，且无模型请求。

### `/status`
- 目的：只读汇报 busy、mode、owned/owner、goal 是否有、loop 开关、history 条数、队列长度；零模型请求。
- 参数：无。
- 忙/闲：都允许读取快照，不修改状态。
- 保留/清理：纯读。
- 反馈：成功→状态行；失败→无。
- 验收：忙时与闲时各一次，字段与实际状态一致。

### `/clear`
- 目的：清理**本地 UI 会话面**，非“全忘记”。
- 参数：无（不接受目标名，避免歧义）。
- 忙/闲：**忙时拒绝**并提示（避免清掉在飞轮上下文）；闲时执行。
- 保留/清理范围：
  - 清理：屏上日志、本地输入框、发送 history（并显式排除不进 history）。
  - 不清：原审计 journal、共享 mind 页、`/goal` 目标与 loop 状态、磁盘 state-v2.json。
  - 且**不重置模型上下文 epoch**——已核 `tui.c` 无此 API；因此不得宣称“模型已忘记”。
- 反馈：成功→“cleared UI view; model context and journal unchanged”；忙→“busy: not cleared”。
- 验收：1) 清后 journal 字节不变；2) 旧 draft/history 不再可见；3) `/clear` 本身不入发送 history；4) 不产生模型请求；5) 不清 goal/loop/memory。

### 未知斜杠
- 目的：`/xxx` 未登记时**本地报错**，不进模型猜。
- 行为：打印“unknown command: /xxx（/help 查看）”，清输入，零模型请求。
- 验收：输入 `/foobar` 后无模型调用且屏上有报错行。

## 收进 help 的既有命令（一行用途）
- `/goal [text]` 设定/清空目标；`/loop [on]` 在空闲时自动续跑目标。
- `/reload` 空闲时重载 key 缓存；忙时保持不动。
- `/reload-code` 需 managed idle actor。
- `/export-state HANDOFF` 需 owned idle 状态与合法 HANDOFF。
- `/exit`、`/quit` 退出。

## 下一批（需明确边界后再列）
- `/cancel`：仅在能定义“取消的实际范围”（是否 abort 网络、是否保留已收 transcript、是否可 resume）后才加入；本稿不列。

## 开放问题
- epoch/context 重置 API 尚未存在，`/clear` 语义须与 `tui.c` 同步演进。
- `/clear` 忙时拒绝 vs 排队等待，需主人选一种。

