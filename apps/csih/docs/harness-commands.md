# Harness CLI 命令（P0 已接线，2026-10-10）

以下命令在 agent 模式由本地处理，零模型请求、不写 journal、不进入发送 history。

- `/help`：列出 native 命令与一行用途；忙闲均可。
- `/status`：输出 busy、mode、owned、是否持有 owner、goal 是否存在、loop/剩余预算、history 条数、pending 条数及按键队列长度。忙闲均可，只读业务状态；命令文本消费，反馈追加 UI 日志。owner 仅表示本地所有权 fd，不认证源码身份。
- `/clear`：无参数。忙时立即拒绝，不排队；闲时清屏上日志/展开索引/滚动位置、输入、history 与浏览草稿、旧错误/答案显示。保留 pending（反馈条数）、goal、loop/剩余预算、journal、mind、磁盘 state 与模型上下文；因此后续 pending/loop 仍可能产生输出，不宣称模型忘记。
- `/goal [text]`：设置目标；裸 `/goal` 保留目标；`/goal` 后空格且无文本清目标并解除循环。
- `/loop [text]`：当前切换开关，参数不解析；无目标时保持关闭。
- `/reload`：空闲时清密钥缓存，忙时不动。
- `/reload-code`：仅 managed idle actor。
- `/export-state HANDOFF`：仅 owned idle 且 HANDOFF 合法；失败保留输入。
- `/exit`、`/quit`：退出。

## 命令识别与错误

仅单行 `^/[A-Za-z][A-Za-z0-9_-]*( |$)` 进入命令表；包含后续路径分隔符的路径或多行文本仍是普通 prompt。
未知命令本地报错 `unknown command`，清命令输入，零模型请求。不带参数的命令收到参数时本地报 usage。
`//foo` 绕过命令表，发送字面 `/foo`；忙时只排队一次，history 保存实际发送文本。

## 回归

`./csih.sh selftest` 的真实 Enter 路径覆盖忙闲 help/status、clear 忙时拒绝、闲时清 history 后不重新加入命令、上下键不召回旧草稿、pending 逐字节不变、loop_left 不变及 journal 字节不变。
同套件覆盖未知命令、参数错误、路径/多行输入与字面逃逸。
这是本地 TUI 单元自测；完整外环及运行中窗口部署应分别验收。

## 后续

`/cancel` 与 context epoch 重置仍需单独定义真实范围，不并入 `/clear`。
