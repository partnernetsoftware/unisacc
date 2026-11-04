# Orca 参考点调研（2026-11-02）

作者：omp-go2
性质：只读调研；未改仓库源码。目标是把 Orca 的**设计点**映射到 unisacc 工具链 / omp 编排组 / 即将做的 harness，并给出可落地的小步改动与成本。

## 0. 候选与选择

| 候选 | 链接 | 类型 | 与我们的相关性 | 置信度 |
|---|---|---|---|---|
| **Orca ADE**（stablyai/orca） | https://www.onorca.dev/ · https://github.com/stablyai/orca | Agent Development Environment：并行 worktree、Run/Task/Dispatch 编排、usage/status、session history、hooks/memory、hibernation、插件、skills | **最高**：请求里的 agent 编排、多窗口协作、状态/记忆、护栏、可观测性、安装升级，它都有明确文档 | 高 |
| Orca Agent Collaboration（orca.space） | https://orca.space/ | Agent 团队协作层：agent identity、scoped ownership、通信、credential vault、work management | 高（概念层），但 beta、文档少 | 中 |
| Orca（tryorca.com，SoFriendly/orca） | https://tryorca.com/ · https://github.com/SoFriendly/orca | Git + terminal + AI coding：live diff、smart commit、notes、Monaco | 中：偏单机 IDE，不是多 agent 编排 | 中 |
| Orca Platform | https://orcaplatform.ai/ | 企业 AI workforce/agent 平台 | 低：业务 agent，不是 coding harness | 低 |
| Orca Security | https://orca.security/ | 云安全 | 无关 | 高（无关） |
| Orca DB | https://orcadb.ai/ | ML 平台 | 无关 | 高（无关） |
| Microsoft Orca | 模型（Orca LLM） | 模型 | 无关 | 高（无关） |
| Orca 屏幕阅读器 | GNOME Orca | 无障碍 | 无关 | 高（无关） |

**选择：`stablyai/orca`（Orca ADE）**。理由：MIT 开源、112.7k stars、文档覆盖了我们正在做的三件事——harness 编排、omp 多窗口协作、状态行/用量可观测；且它明确支持 `oh-my-pi`（README 的 Supported Agents 列表），说明它和 omp 是同一生态位。

> 若董事长指的是 `orca.space`（“agent collaboration”）或 `tryorca.com`（Git+AI coding），请 cc 确认；本文先按 ADE 取参考点。`orca.space` 的 identity/scoped ownership/credential vault 我会在 §2.12 作为第二候选补充。

## 1. 参考点总表

| # | 设计点 | Orca 怎么做（来源） | 对应到我们 | 小步改动 | 成本 |
|---|---|---|---|---|---|
| 1 | Worktree-native 隔离 | 每个任务一个 git worktree；base/start-from；共享 gitignored 目录；`.worktreeinclude` 复制 `.env`；后台创建；删除时保留未合并分支 | harness 的 `work/<ticket>` 目录；omp subagent 共享 cwd | harness `prepare` 改用 `git worktree`；加 `superharness.yaml` 的 `setup/sharedDirectories/include` | 中 |
| 2 | 持久编排模型 | Run（namespace+coordinator inbox）/ Task（pending/ready/dispatched/completed/failed/blocked）/ Dispatch（一次尝试）/ Message（status/dispatch/worker_done/escalation/question/heartbeat）/ Decision gate | harness 的 Ticket/Assignment/Worker/Outbox；cc 的 envelope | 加 `dispatch` 表与 `gate` 表；报告必须带 task+dispatch id；僵死重试不能完成错 dispatch | 中 |
| 3 | Worker contract | 每个 worker 收 preamble；`worker_done` 只发一次（含 outcome/summary/files）；长任务 heartbeat；用 `ask` 阻塞提问；`worker-read/release/retain` 分离读取与回收 | harness `reportTicket`；omp worker 的 report/ask | 扩展 report schema：`outcome`、`files_modified`、`dispatch_id`、`question`；加 `harness_ask` | 低 |
| 4 | 底部 usage/状态行 | 状态栏只显示 active account；Usage popover 按最紧 limit 排序；Detailed/Compact；窄窗先隐藏 mini bar，再每 provider 一个百分比，再 `+N` chip；80% warning chip；`% used`/`% remaining` 可选；余额与 limit 并列 | 我们刚做的 `balances` 行 + `/bal` 表 | 加 `% used/remaining` 设置、80% warning、`+N` overflow、按最紧排序；`/bal` 增加状态文案 | 低 |
| 5 | 状态检测与 hook 端点 | OSC 状态检测（Cursor CLI）；Orca 管理 working/waiting/done hook；端点写盘并每次 hook 重新 source，app 重启后长会话仍可达；Codex hook 只在变化时写，避免反复授权 | omp extension 的 `session_start/turn_end`；harness 的 `SUPERHARNESS_URL` | 扩展改为每次调用读 harness 写入的 endpoint.env（路径由 harness 决定，本仓已退役的 superharness 曾用 `out/` 下同名目录）；harness 重启后 worker 仍能回报 | 很低 |
| 6 | Hibernation + resume | 只在 done、非前台、无按键、可 resume、idle≥30m、无未结算 Dispatch、无 live subagent roster 时休眠；整个 worktree 一起休眠；resume 用捕获的启动命令/env + provider resume flag；失败则新 prompt | harness 的 worker 生命周期；procguard/watchdog | 加 `hibernated` 状态与 `resume_cmd/env`；watchdog 只在安全谓词全满足时暂停 | 中 |
| 7 | Session history 索引 | 扫描各 agent 的 on-disk transcript；右侧栏 Workspace/Project/All；按 Project/Folder/Agent 分组；搜索 title/cwd/branch/model/preview；Resume 新终端/新 chat；复制 resume 命令/session id/log path；`orca search` CLI | omp 的 `~/.omp/agent/sessions`；harness 的 `sessions/<ticket>` | harness `status` 增加 session 索引：ticket、cwd、last activity、resume 命令 | 低 |
| 8 | Chat ↔ terminal 分离与 handoff | 结构化 chat 把会话状态放在 Orca；tab/对话可跨重启恢复；`/clear`、`/compact`、rewind、steer/queue、inline diff、background-task strip、plan checklist、goal controls；可在 chat/terminal 之间移动；Hand Off 给另一个 agent | omp 的 TUI/RPC；harness 的 ticket 状态与 worker 会话 | harness 加 `/ticket <id>` 视图与 `handoff <ticket> <worker>`；状态与执行面分离 | 低-中 |
| 9 | 批量 diff review | 在任意 AI diff 行上留言；多行范围；评论跟随行移动；一次性“Send to agent”批量发送；Resolve/re-review；批量避免 agent 来回摇摆 | cc 审 agent 改动；envelope 逐条 review | 定义 `review-note` 消息（file/line/comment），按 ticket 批量一次发；cc 用一条 envelope | 低 |
| 10 | 通知/收件箱 | 区分“agent 真完成”与“暂停”；系统通知/声音/chip；持久 bell；mark unread；按机器路由；未读 worktree 加粗不挂 badge | cc 的 envelope 注意力；harness outbox | 给 outbox message 加 `kind=progress|done|question|blocked`；只有 done/question/blocked 唤醒 cc | 低 |
| 11 | Skills registry / 版本匹配 | 公共包是 thin stub：告诉 agent 何时用 Orca、用 `orca skills get <topic>` 拉当前版本的完整指南；flag 在二进制里，避免 skill 与 app 漂移；后台更新；共享 skill 不可变版本 | `moltbaby/skills/omp`；未来 harness skill | 写薄 `SKILL.md`，完整指南由 `superharness guide` 输出；skill 不复制 flag | 低 |
| 12 | 插件 consent / 内容 vs 代码 | 插件分 content-only 与“opens/edits files”；运行代码的插件先征得同意，之后有本机访问；官方/社区标注 | omp extensions/hooks；harness tools | 工具/插件 manifest 加 `runs_code`、`source=official|community`；代码型先审批 | 低 |
| 13 | 安装/升级体验 | 桌面/移动多平台；Homebrew/AUR；skills `npx` 安装；app 内后台更新、update nudge、Needs attention、失败持久提示；版本匹配文档 | `moltbaby` install.sh/upcheck.sh；omp wrappers | harness 加 `doctor`/`update`；扩展加版本检查与 endpoint reload | 低-中 |
| 14 | Pane/tab 布局 | 每个 tab 只放一个东西（terminal/editor/browser/diff/PR）；拖拽 split；嵌套；边界位置按 worktree 保存；active tab 色条；`Copy Tab ID` 给 agent 精确指向 | tmux 窗口/窗格；cc+omp-* 拓扑 | 给每个 worker 稳定 `ticket_id/worker_id`；harness 存 `layout` 字段；tmux 布局脚本按 ticket 恢复 | 低-中 |
| 15 | 并行 race | 同一 prompt 开 3 个 worktree/3 个 agent，比较 diff，选 winner，删除 loser | harness 的并发 worker；cc 的“第二意见” | `superharness race --agents 3`；硬上限 3；比较 `files_modified`/diff；loser 归档 | 中 |

## 2. 详细参考点

### 2.1 Worktree-native 隔离（最高优先）

**Orca 怎么做**：每个任务一个真实 `git worktree`；repo 有 base ref，worktree 有 start-from ref；每个 worktree 自己的 branch、文件、agent 终端；删除 worktree 同时删 branch（保留未合并分支供 review）；`orca.yaml` 的 `worktree.sharedDirectories` 共享 `node_modules`/`.cache`，`.worktreeinclude` 复制 `.env` 等 gitignored 文件；创建在后台进行，失败可 Retry。来源：https://www.onorca.dev/docs/model/worktrees

**对应到我们**：
- harness 当前 `work/<ticket>` 是普通目录；应改成 `git worktree add`，避免 worker 改到主检出。
- `superharness.yaml` 可定义 `setup`（worktree 创建后跑一次）、`sharedDirectories`、`include`；与 unisacc 的 `AGENTS.md` 不冲突。
- omp subagent 仍共享 cwd，所以 harness 模板必须显式把 `cwd` 指到 worktree。

**小步**：先只做 `git worktree add <out>/worktrees/<ticket> <base>` + `setup` 命令；不做 shared/include，等第一个真实 worker 需要时再加。成本：中（1–2 天）。

### 2.2 Run / Task / Dispatch / Message / Gate（harness 的核心借鉴）

**Orca 怎么做**：
- Run = durable namespace + coordinator inbox，不调度、不放 worker。
- Task 状态：`pending`、`ready`、`dispatched`、`completed`、`failed`、`blocked`。
- Dispatch = 一次 attempt on a terminal；`worker_done`/heartbeat 的生命周期权威来自 active dispatch。
- Message 类型：`status`、`dispatch`、`worker_done`、`escalation`、`question`、`heartbeat`。
- Decision gate 阻塞 task 直到 coordinator resolve。
- 完成消息必须带 `taskId` + `dispatchId`，防 stale retry 完成错 dispatch。
来源：https://www.onorca.dev/docs/cli/orchestration

**对应到我们**：
- harness 已有 `ticket` / `worker` / `outbox`，但没有“attempt”层；worker 重启/重试会覆盖同一行，无法区分旧回报。
- 加 `dispatch` 表：`id`、`ticket_id`、`attempt`、`state`、`terminal/pid`、`started_at`、`heartbeat_at`、`ended_at`。
- `report` 必须带 `dispatch_id`；不匹配就拒绝（stale report）。
- 加 `gate` 表：`ticket_id`、`question`、`options`、`resolution`、`resolved_at`。

**小步**：先加 `dispatch` 表与 `report(dispatch_id)` 校验，再加 gate。成本：中（2–3 天）。

### 2.3 Worker contract（低成本、立刻可做）

**Orca 怎么做**：worker 收到 preamble；`worker_done` 恰好一次（成功/失败都要），带 `--outcome succeeded|failed`、短 summary、task/dispatch id、`--files-modified`；长任务发 heartbeat；阻塞问题用 `orca orchestration ask`，不要本地 TUI 等；`worker-read` 在 `worker-release` 后读输出，`worker-retain` 用于调试。来源：https://www.onorca.dev/docs/cli/orchestration

**对应到我们**：
- harness 的 `reportTicket` 现在只有 `status/summary/reply`；加 `outcome`、`files_modified`、`dispatch_id`、`question`。
- cc 的 envelope 汇报也应带 ticket id + attempt id，避免“旧 worker 的完成”被当成当前。
- 加 `harness_ask`（问题+选项+超时），答案进 gate/outbox。

**小步**：先扩 report schema 与 `harness_report` 工具；再加 `harness_ask`。成本：低（0.5–1 天）。

### 2.4 底部 usage/状态行（直接对应我们刚做的 `balances`）

**Orca 怎么做**：
- 状态栏始终反映 **active account**；其他账号在 switcher 里各自显示。
- 点 usage segment 打开 popover：每个 provider 一行，按**最紧 limit 排前**，含 plan、soonest reset、per-window bars。
- Detailed（全部窗口）vs Compact（每 provider 只显示最紧窗口）。
- 窄窗：先隐藏 mini bar/label，再每 provider 一个百分比，再 `+N` chip；80%+ 的 provider 最后折叠。
- 80% warning chip；`% used` vs `% remaining` 可在 Settings 选；余额与 limit 并列；无数据时显示 `Loading usage…`/`not signed in`/`Usage unavailable`/`No usage data`。
来源：https://www.onorca.dev/docs/agents/usage-tracking

**对应到我们**：
- 我们的 `balances` 行已做到“单行、固定顺序、当前 provider 第一、剩余口径、`↻` 重置、宽度截断”。
- 缺口：① `% used`/`% remaining` 设置；② 80% warning；③ `+N` overflow；④ 按最紧排序（现在固定顺序）；⑤ “未登录/不可用”状态文案；⑥ 多账号（active vs others）。
- `/bal` 表已有 Provider/Pool/剩余/已用%/重置/备注；可加“状态”列与“最紧优先”排序。

**小步**：在 `balances.ts` 加 `USAGE_DISPLAY=remaining|used`（默认 remaining）、80% `!` 前缀、`+N` chip；probe 加合成断言。成本：低（0.5 天）。这条可以直接排到我们上次的 usage 任务后面。

### 2.5 状态检测与 hook 端点（harness 立即可用）

**Orca 怎么做**：Cursor CLI 用 OSC 状态事件；Orca 管理 agent status hooks（working/waiting/done），端点写盘到 `{userData}/agent-hooks/endpoint.env`，每次 hook 重新 source，所以长会话在 app 重启后仍能 POST 到新 server；Codex hook 只在变化时写，避免反复授权；关掉 hooks 会移除托管 hook，重开不重启 app。来源：https://www.onorca.dev/docs/agents/hooks-memory · https://www.onorca.dev/docs/agents/cursor-cli

**对应到我们**：
- 我们的 harness 扩展若在启动时缓存 `SUPERHARNESS_URL`，harness 重启后会 POST 到死端口。
- 改成每次工具调用前读 harness 写的 endpoint.env（`URL=...`、`TOKEN=...`），harness 启动时原子写。
- 这比“让 worker 自己重连”简单，且能覆盖 omp 长会话。

**小步**：扩展加 `resolveEndpoint()`，probe 模拟 endpoint 文件变更。成本：很低（1–2 小时）。

### 2.6 Hibernation + resume（进程/额度控制）

**Orca 怎么做**：只在**全部**满足时休眠：agent done、不在 active/foreground worktree、无按键、可 resume、idle≥30m（1m–24h 可调）、无 mobile 驱动、无未结算 Dispatch、无 live subagent/teammate roster；同一 worktree 多个 agent pane 一起休眠；手动 sleep 保留 finished/interrupted 会话；resume 用捕获的启动命令/env + provider resume flag；resume 失败则新 prompt，旧 transcript 留在 history。来源：https://www.onorca.dev/docs/agents/hibernation

**对应到我们**：
- harness 的 watchdog 现在只会“kill/abort”；应区分 `hibernated`（保留 session，可 resume）与 `stopped`（结束）。
- 安全谓词直接照搬：无 active ticket、无 heartbeat、无 live subagent/job、无未答 gate、idle≥N。
- resume 命令必须保存：omp `--resume <id>` 或 `--restore <session>`；workdir/env 也要保存。

**小步**：在 worker 表加 `hibernated_at`、`resume_cmd`、`resume_env`；watchdog 先只做“标记 hibernated”，不自动 kill；下轮再做 resume。成本：中（1–2 天）。

### 2.7 Session history 索引（记忆/恢复）

**Orca 怎么做**：扫描各 agent 的 on-disk transcript；右侧栏 Agent Session History；scope Workspace/Project/All；group Project/Folder/Agent；搜索 title/cwd/branch/model/preview；Resume 新 terminal 或 new chat；Copy resume command/session id/log path；跨主机索引搜索；`orca search` CLI。来源：https://www.onorca.dev/docs/agents/session-history

**对应到我们**：
- omp 已有 session 文件；harness 有 `sessions/<ticket>`，但没有索引/搜索。
- 加 `harness status --sessions`：ticket、worker、cwd、last activity、resume 命令、last summary。
- 不需要立刻做全文搜索；先做“按 ticket 找 session”。

**小步**：扩展 `status`/`read resource=sessions`；成本：低（0.5 天）。

### 2.8 Chat ↔ terminal 分离与 handoff

**Orca 怎么做**：结构化 chat 把会话状态放在 Orca，tab/对话跨重启恢复；`/clear`、`/compact`、rewind、steer/queue、inline diff、background-task strip、plan checklist、goal controls；可在 chat/terminal 间移动；Hand Off 给另一个 agent。来源：https://www.onorca.dev/docs/agents/native-chat

**对应到我们**：
- 我们的“状态”在 SQLite，执行在 omp TUI/RPC；已经分离，但缺“移动/交接”。
- harness 加 `handoff <ticket> <new_worker>`：保留 ticket、session、workdir，换执行者；报告要记录 handoff 原因。
- `/ticket <id>` 视图展示 ticket 卡、material、dispatch、last report、gate。

**小步**：先做 `handoff` CLI 与 `ticket` 视图；成本：低-中（1 天）。

### 2.9 批量 diff review（cc 直接可用）

**Orca 怎么做**：在 diff 任意行留言，支持多行范围；评论跟随行移动；一次“Send to agent”批量发送；Resolve/re-review；批量避免 agent 来回摇摆。来源：https://www.onorca.dev/docs/review/annotate-ai-diff

**对应到我们**：
- cc 现在可能逐条 envelope 给 agent 反馈；改成“一个 ticket 的 review notes 批量一次发”。
- 定义 `review-note` 消息：`ticket_id`、`file`、`line`、`comment`、`severity`；harness 聚合后一次投递。
- 这对 unisacc 的 review 文化很合适：一次审完，一次发。

**小步**：先写协议 + 一个 `review-notes.md` 模板；工具化等 harness。成本：低（协议 0.5 天）。

### 2.10 通知/收件箱（注意力预算）

**Orca 怎么做**：区分“agent 真完成”与“暂停”；完成发系统通知/声音/chip；持久 bell；mark unread；按机器路由；未读 worktree 加粗不挂 badge；structured chat 成功只标 unread，不发系统通知。来源：https://www.onorca.dev/docs/notifications

**对应到我们**：
- cc 的注意力是瓶颈；harness 的 outbox 应只对 `done`/`question`/`blocked` 唤醒 cc，`progress` 只落库。
- envelope 可以带 `kind`，cc 侧按 kind 决定是否打断。
- 这与 Super-agent 的“只在需要时说话”一致。

**小步**：outbox 加 `kind` 列；`reportTicket` 映射；cc 只看非 progress。成本：低（0.5 天）。

### 2.11 Skills registry / 版本匹配（moltbaby 直接可用）

**Orca 怎么做**：公共 skill 是 thin stub：告诉 agent 何时用 Orca、用 `orca skills get <topic>` 拉当前版本完整指南；命令 flag 在二进制里，skill 不复制，避免漂移；app 内后台更新、update nudge、Needs attention；共享 skill 不可变版本；扫描 OMP skills 目录 `~/.omp/agent/skills`。来源：https://www.onorca.dev/docs/cli/skills

**对应到我们**：
- `moltbaby/skills/omp` 已经是“薄 skill + 仓内文档”的形态；可以更彻底：`SKILL.md` 只写触发与 `omp skill guide`，完整命令由 CLI 输出。
- 未来 harness 也发一个 `skills/superharness/SKILL.md` stub + `superharness guide`。
- Orca 扫描 OMP skills 目录，说明我们的 skill 可以被 Orca 直接发现；反之亦然。

**小步**：给 harness 加 `guide` 命令；skill stub 指向它。成本：低（0.5 天）。

### 2.12 插件 consent / 内容 vs 代码（护栏）

**Orca 怎么做**：插件页分 content-only 与“opens and edits files”；运行代码的插件先征得同意，之后有本机访问；官方/社区标注；56 个插件。来源：https://www.onorca.dev/plugins

**对应到我们**：
- omp extensions/hooks 目前默认可执行代码；harness 工具也应标注 `runs_code`。
- 在 `superharness.yaml` 的 tools/plugins 里加 `runs_code: true|false`、`source: official|community`；`runs_code` 首次使用需 cc 批准（或 `--auto-approve` 仅限隔离 worktree）。

**小步**：manifest 字段 + 审批记录；成本：低（0.5 天）。

### 2.13 安装/升级体验

**Orca 怎么做**：macOS/Windows/Linux + iOS/Android；Homebrew/AUR；`npx skills add`；app 内后台更新、update nudge、Needs attention、失败持久提示；版本匹配文档；changelog 每日更新。来源：https://www.onorca.dev/download · https://www.onorca.dev/changelog

**对应到我们**：
- `moltbaby/skills/omp` 已有 `install.sh`/`upcheck.sh`；可加“版本匹配文档”与“后台更新提示”。
- harness 加 `superharness doctor`（检查 bun、sqlite、omp、endpoint、worktree）与 `superharness update`（git pull + 重启）。
- 不追求 AppImage/移动端；保持 CLI 薄入口。

**小步**：`doctor` 先做只读检查；成本：低（0.5 天）。

### 2.14 Pane/tab 布局与稳定 ID

**Orca 怎么做**：每个 tab 只放一个东西（terminal/editor/browser/diff/PR）；拖拽 split；嵌套；边界位置按 worktree 保存；active tab 色条；`Copy Tab ID` 给 agent 精确指向；`Cmd-J` jump palette。来源：https://www.onorca.dev/docs/model/tabs-panes-splits

**对应到我们**：
- tmux 窗口是扁平的；但“稳定 ID”可以直接借：每个 worker 的 `ticket_id`/`worker_id` 就是它的 tab ID，envelope 里引用它，不引用窗口名。
- harness 可以存 `layout` 字段（worker 的 tmux window/pane id），重启后恢复。
- cc 的 omp-* 窗口名已经稳定；再加 ticket 映射即可。

**小步**：harness ticket 加 `layout`；envelope 用 ticket id；成本：低（0.5 天）。

### 2.15 并行 race（可选）

**Orca 怎么做**：同一 prompt 开 3 个 worktree/3 个 agent，比较 diff，选 winner，删除 loser；split panes 同屏看。来源：https://www.onorca.dev/docs/first-session · https://www.onorca.dev/docs/recipes/parallel-agents

**对应到我们**：
- harness 已有并发上限 3；可以加 `race` 模式：同一 ticket spec 派 3 个 worker，各自 worktree，比较 `files_modified`/diff/测试结果，选一个，其他归档。
- 必须硬上限 3，深度 1，成本预算；否则就是今天的 `.cx` 递归教训。

**小步**：先不做自动 race；在文档里保留为 P2。成本：中（2–3 天）。

## 3. 优先级建议

| 优先级 | 参考点 | 理由 | 预计成本 |
|---|---|---|---|
| **P0** | 2.5 hook 端点写盘/每次重读；2.3 worker contract 扩展；2.10 outbox `kind`；2.4 usage 行 `% used/remaining` + 80% warning + `+N` | 改动小、直接提升现有 omp/harness 稳定性与可观测性 | 各 0.5–1 天 |
| **P1** | 2.1 worktree 隔离；2.2 Dispatch/Gate；2.7 session 索引；2.9 review-note 批量；2.11 skill stub | 构成 harness 的最小可信编排层 | 各 0.5–2 天 |
| **P2** | 2.6 hibernation；2.8 handoff；2.12 插件 consent；2.13 doctor/update；2.14 layout | 提升长期运行与运维体验 | 各 0.5–2 天 |
| **P3** | 2.15 race | 有价值但成本高、风险大；等 P0/P1 稳定后再评估 | 2–3 天 |

**建议先做 P0 的 2.5 + 2.3 + 2.10**：它们都在 harness/extension 的薄层，不碰 unisacc 主线，且能立刻减少“worker 回报丢失/旧回报覆盖/cc 被 progress 打扰”的问题。

## 4. 未核实 / 风险

1. **候选归属**：本文按 `stablyai/orca`（ADE）取参考点；若董事长指的是 `orca.space` 或 `tryorca.com`，部分参考点（尤其 2.2/2.3/2.4）需要重取。`orca.space` 的 identity/scoped ownership/credential vault 只看了首页，未读文档（beta，文档少）。
2. **Orca 内部实现**：我们只读官方文档与 README，未读其源码；具体数据结构/协议以文档为准，可能与实现有差异。
3. **截图**：本文引用文档页面与首页截图位，未把图片落盘；如需给董事长看，建议直接打开对应 docs 页。
4. **成本估计**：按“单人、已有 harness 骨架”估；实际受 omp RPC/进程管理复杂度影响。
5. **不要照搬**：Orca 是桌面 IDE，重 UI/多平台；我们优先借“状态机/协议/安全谓词”，不借其 UI 复杂度。
