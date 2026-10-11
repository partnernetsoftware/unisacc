# 诊断副本 + 同 SHA flaky 探针请示页（1354；证据不足具名修域）

墙钟：2026-10-11 13:56 SGT。授源：政委续刀 + 机房主任代裁「授权跑同 SHA flaky 探针一次（不改源、不 bump、不开 Draft）」。

## 为何不写具名最小修域
- artifacts=[]；无 bs_B 字节、无 core/PC、无分步 rc。
- H1（B 镜像崩溃）仍最强工作假设，但 **不能** 落到具体文件/op；窄修域另裁条件未满足。
- 故本页交付：**诊断副本规格** + **同 SHA flaky 探针执行记录**（代裁已授一次）。

## A. 诊断副本规格（材料；本刀不改仓源）
目的：在固定 tip `82e1a9f0` 上，于批准的 GitHub 原生 Darwin runner 采「B 崩溃证据包」，不改判定、不放宽验收、不入默认门禁。

建议（仓外脚本 / 一次性 workflow_dispatch 诊断，**须另裁落地**；本刀只写规格）：
1. 绑定清单：SELF、UA、weights/gold、B.tape、bs_B/bs_U 签前签后 SHA、runner/image/arch、monotonic 起止。
2. 分步 rc：UA -c → from-tape compile → codesign（显式检查 rc）→ `bs_B SELF -c`；U 支对称；每步 stderr 保全。
3. 崩溃采集：若步④非 0，保留 `bs_B`、尝试 `sample`/`lldb` batch / DiagnosticReports 副本；记 signal 与异常 PC。
4. EXIT 前 **copy 出 scratch**（当前 trap 会删）；上传为 artifact（仅诊断 run）。
5. 禁：改产品源、关签名、以串行替代并行当「已修」、把诊断绿写入 Draft 前置。

## B. 同 SHA flaky 探针（代裁授权 · 本刀执行一次）

| 项 | 值 |
|---|---|
| 入口 | `gh run rerun --failed 38111789528`（仅失败 job；不新 dispatch main；不重跑 linux 成功格） |
| 固定 SHA | `82e1a9f06e9e39f805e25f0ad285b7306acb9937` |
| 范围 | macos-14 `114388705403`；macos-15 `114388705429`；macos-15-intel `114388705401` 各随 failed-rerun 一次 |
| 原红 | attempt1 日志已保全（sha256 见终表）；新 attempt **并列**，禁止覆盖/撤销原红 |
| 解读 | 一次绿 = 该 attempt 未复现，≠ 根因闭合 / ≠ 六格 PASS；仍红同形 = 重复症状，支持非纯偶发 |
| 硬禁 | 不改源、不 bump、不开 Draft、不放宽、不 m4pro、不以 U 代 B |

### 执行状态（本刀填）
- 授权：代裁 1354 **已授** 跑一次。
- 触发时刻：2026-10-11 13:56 SGT / 2026-10-11T05:56:41Z
- 命令：`gh run rerun --failed 38111789528 -R partnernetsoftware/unisacc`
- 触发结果：**见下方 probe-trigger 回执 / 本页补记**（成功后记新 attempt 号）。

## C. 探针后仍缺什么（预期）
即使 attempt2 仍红同形，**仍缺** core/PC/分步 rc（artifacts 仍空）。下一窄修域仍须诊断副本采证后另裁；不得凭 139 直接改 backend/签名策略。

## 探针触发回执（已执行）
- 时刻：2026-10-11 13:56:44 SGT / 2026-10-11T05:56:44Z
- `gh_exit=0`；`run_attempt=2`；status=queued→三 mac 已 queued/in_progress
- head_sha 仍 `82e1a9f06e9e39f805e25f0ad285b7306acb9937`
- attempt2 job ids（新）：macos-14 `114402137149`；macos-15 `114402137315`；macos-15-intel `114402137352`
- linux 两格 attempt2 侧记为 completed/success（未作为失败重跑目标）
- URL：https://github.com/partnernetsoftware/unisacc/actions/runs/38111789528
- 原件：`probe-trigger-1354.txt`
