# superharness D3 计划：Router 常驻会话 / RPC

状态：待 cc 批准，未实现。D2 的 dry-run Router、单 worker、台账/doctor 原样保留。
目标：让 Router 成为**一个常驻 omp RPC 会话**，按 trigger 串行决策；不改变 worker 的一次一进程模型。

## 1. 目标与非目标

- 目标：`router --real` 启动一个 RouterRunner；它拥有一个 `omp --mode rpc` 子进程，接收 new trigger，调用 `harness_read`/`harness_route`，把决策落 `router_decision`。
- 目标：同一时刻最多 1 个 Router、最多 2 个 worker；Router 不 spawn worker，只做 accept/ignore。
- 非目标：自动派发 worker（仍用 `worker` 命令）、WebSocket、移动端、跨机 Router、模型训练。
- 非目标：修改 `CXWORK2`/workspace；superharness 仍是 `out/` 原型。

## 2. 拓扑

```
harness server (127.0.0.1:8787, token 0600)
        │  /read /route /report (现有)
RouterRunner (one, Bun) ── owns ──> omp --mode rpc --model <routerModel> -e extension.ts
        │                                      │
        └── SQLite state.db <──────── harness_route/harness_read
Worker (≤2, one-shot omp -p, D2) ── /report ──> server
```

- RouterRunner 与 server 共享 `SUPERHARNESS_DB`；server 不启动 Router。
- Router 进程由 harness 以精确 PID/PGID/starttime 记账；worker 仍是 D2 的 `spawnOwned`。
- Router 不接收 `/route` 之外的写权限；它不直接写 DB。

## 3. RPC 合同

- 启动：`omp --mode rpc --model <routerModel> --max-time 1200 --no-session -e extension.ts`；stdin/stdout 为 JSONL。
- 输入命令：`{"id":"rN","type":"prompt","message":...}`；每 trigger 一次；`{"id":"rN","type":"new_session"}` 清空上下文；`{"type":"abort"}` 超时取消。
- 输出：只认 `response`/`prompt_result`/session settle；否则视为 malformed，关闭并按失败处理。
- 每个 turn 的 prompt 固定包含：trigger_id、thread、source、actor、text、允许的工具、`accept|ignore` 规则、禁止执行工作。
- 决策以 `harness_route` 的 HTTP 响应为准；RPC 的文本回答不是决策。
- 每 trigger 最多 1 次 turn、1 次重试（仅传输失败）；重复触发不重复建 ticket。

## 4. 数据模型（新增）

| 表 | 关键字段 | 用途 |
|---|---|---|
| `router_session` | id, pid, pgid, start_ticks, state(starting/idle/busy/stopped/failed), model, started_at, ended_at, last_turn_at, error | 常驻进程台账 |
| `router_turn` | id, session_id, trigger_id, state(queued/running/decided/timeout/failed), started_at, ended_at, tokens, error; unique(trigger_id, session_id) | 单次决策/幂等 |
| `router_decision` | 复用；加 session_id, turn_id（或旁表关联） | 审计 |
| `process_ledger` | 复用；Router 行 ticket_id 置 `-router-` | 精确清理 |

## 5. 循环与幂等

1. `tick`（1s 或 CLI 触发）：读 `newTriggers`；若 Router 不健康先按台账回收/重启（每 10 分钟最多 1 次）。
2. 串行处理：取 1 条 trigger → `new_session` → `prompt` → 等待 decision/超时 → `router_turn` 终态 → 下一条。
3. `harness_route` 成功后 trigger.disposition 改变；重复 tick 不会再次处理。`router_decision` 以 trigger_id 幂等。
4. 槽满/预算不足：不取新 trigger，保持 new；报告 `deferred`，不排队无限重试。
5. 超时 120s/turn；abort 后 turn=timeout，trigger 保持 new，下一轮最多 1 次重试；连续 2 次失败标 failed 并告警。

## 6. 安全约束（不放松）

- `maxRouter=1`；`maxWorkers=2`；`maxProcesses=6` 必须把 Router 计入（新增 `assertProcessSlot`），不能只看 worker。
- `maxDepth=1`：Router 的 `SUPERHARNESS_DEPTH=1`；任何从 Router 再 spawn 的尝试被拒绝。
- spawn 预算：Router 启动消耗 1 次；per-trigger turn 不重复消耗 spawn 预算；turn token 预算默认 200k，超限失败不续跑。
- 精确清理：只按 `router_session`/`process_ledger` 的 `pid+starttime` 校验后 `kill(-pgid)`；先 TERM 后 KILL；绝不 `pkill -f`、不按命令行匹配。
- 服务绑定：`127.0.0.1`、token 文件 0600；RPC 子进程只继承 endpoint/env，不新增公网监听。
- 干跑优先：`SUPERHARNESS_DRY_RUN=1`（默认）时 `router --real` 拒绝启动；先跑 `router --dry-run` 与 fake RPC 单测。
- 日志不写 token、prompt 全文可选脱敏；失败只记 trigger_id/错误码。

## 7. 实现切片

- D3a：`RouterRpc` 接口 + fake agent；单测幂等/超时/malformed/槽满，不启动 omp。
- D3b：真实 RPC 客户端（JSONL 编解码），`router --once` 只处理 1 条 trigger；不 spawn worker。
- D3c：RouterRunner 常驻循环、`router_session`/`router_turn`、reclaim、`router status`/`stop`、doctor 增加 Router 行。
- D3d：故障注入（kill 精确 PGID、RPC 半帧、重复 trigger）与 3 条串行 smoke；失败路径全部有终态。
- 每个切片先 dry-run/fake，再真实一次；真实 omp 只用 cheap model、max 1200s、单 Router。

## 8. 验收（D3 完成条件）

1. fake RPC：3 trigger → 3 decision，1 ticket/trigger，无重复；超时/malformed 有终态。
2. 真实 `--once`：1 trigger → decision 落库，Router 进程退出/台账为 exited，无 orphan，spawn 预算 +1。
3. 常驻：3 trigger 串行，Router PID 不变、turn 3 次、`router status` 可见；无第二 Router。
4. 故障：精确 PGID 杀 Router → reclaim 标 failed，不误杀 worker；下轮最多重启 1 次。
5. 安全：127.0.0.1、token 0600、无 token 日志；`maxProcesses` 含 Router 后仍拒绝超限。
6. `bun test` 全绿；`router --dry-run` 与 D2 worker 回归不退化。

## 9. 待 cc 确认

- Router 模型（默认 cheap `codebuddy/deepseek-v4.1-flash`）与 turn 预算 200k/120s 是否接受。
- `router --real` 是前台服务还是 `proc://` 常驻服务（建议独立 `router-runner`，便于精确停）。
- Router 决策后是否自动调用 `worker`：建议 D3 只落 pending dispatch，自动派发留 D4。
