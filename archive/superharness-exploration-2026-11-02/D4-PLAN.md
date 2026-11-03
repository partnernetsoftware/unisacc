# superharness D4 计划：pending dispatch → 一个真实只读 worker

状态：待 cc 批准，未实现。D3b Router 保持；D4 只加“派发一个 worker”，不自动扩成池。

## 1. 目标与非目标

- 目标：`dispatch --once` 取一个 `dispatch.state='pending'`，在 `out/superharness/work/<ticket>/` 启动**一个** worker，完成 read/summarize/report 全链。
- 目标：trigger→route→dispatch→worker→done→outbox 一条真实链；仍单 worker、干跑可切。
- 非目标：多 worker/队列并发、自动继续下一个 dispatch、git worktree、对外 envelope、WebSocket。
- 非目标：D5 watchdog 常驻；D4 的清理靠台账精确 PID/PGID/starttime。

## 2. 流程
```
pending dispatch ──dispatch --once──> worker (omp -p, read-only tools)
        │                                │ harness_report
        └── SQLite ◄── heartbeat ────────┘
                                      └── done/ask/blocked → outbox (pending)
```

- `dispatch --once` 选最早 pending；同一 ticket 只允许一个 active dispatch/worker。
- 工作目录 `work/<ticket>/` 用普通目录创建，不触碰 git；worker cwd 固定该目录。
- worker 模型 `codebuddy/deepseek-v4.1-flash`；可用 `opencode-go` 配置切换。
- worker 工具限 `read/grep/glob/harness_report`（不含 bash/write/edit）；prompt 明确“只读、总结、回报”。

## 3. 契约与预算

- heartbeat：harness 每 5s 更新 `dispatch.heartbeat_at` 与 `worker.last_heartbeat_at`；worker 也可 progress 报告。
- done 恰好一次：沿用 `reportTicket` 的 completed/failed 重复拒绝；`dispatch_id` 必带。
- ask：`harness_report` 带 question → ticket blocked + gate open；worker 退出，不自动重派。
- token：worker 用 `--mode json` 捕获 usage；harness 侧监控累计，超 `tokenBudgetPerWorker` 立即杀进程组并置 dispatch/ticket failed，不等报告。
- 超时：dispatch 启动后 `workerTimeoutMs`（默认 600s）或心跳超时；先 SIGTERM 进程组，宽限后 SIGKILL；只按台账 `pid+starttime`。
- 崩溃：进程退出但无 terminal report → dispatch/ticket failed；有 report 则按 report 终态。
- 槽位：`maxWorkers=2`、`maxProcesses` 仍含 Router；D4 `--once` 只启动 1 个。

## 4. outbox
- 写 outbox 的条件：`ready`（done）、`blocked`、ask（gate open）；`progress` 只入库，不写 outbox；`failed` 只入库（D4 不对外）。
- 新列 `kind`（`done|ask|blocked`）与 `status` 保持；同一 dispatch 的 terminal outbox 只写一次（幂等）。
- 消费者：`main.ts outbox` 只打印 pending 行；`notifyCc` 配置默认 **off**，开启也仅通知 cc，不 envelope 其他窗口。
- 不新增后台消费者；D4 验收用 CLI 打印。

## 5. 实现切片
- D4a：schema `outbox.kind`、`dispatch` heartbeat/timeout 字段、`dispatch --once` 选单 pending；fake worker 单测。
- D4b：真实 worker：read-only tools、workdir、JSON usage 解析、token/超时杀进程组、heartbeat。
- D4c：terminal outbox 映射（done/ask/blocked 幂等），`outbox` CLI；`doctor` 增加 pending/running dispatch 与 worker 计数。
- D4d：失败注入：超时、崩溃、超 token；每个都核 PID/PGID 消失、doctor 无 live/orphan。
- 每片先 dry-run/fake，再一次真实；真实只用 1 worker/1 ticket。

## 6. 验收
1. 真实只读任务：trigger→route→pending dispatch→worker→done→outbox；记录 token、耗时、线程前后、残留。
2. worker 超时：精确杀进程组，dispatch/ticket failed，无残留。
3. worker 崩溃：无 terminal report，dispatch/ticket failed，无残留。
4. worker 超 token：harness 侧中止，failed，无残留。
5. `main.ts outbox` 只显示终态 pending；progress 不在 outbox。
6. 每项最后跑 `doctor`：台账、实际进程、孤儿、spawn/token 预算余量；`bun test` 全绿。

## 7. 待确认

- worker 超时默认 600s 是否合适；JSON usage 解析不到时是否按 0 继续（建议：标记 `usage-unavailable` 但不阻断）。
- `failed` 是否也要 outbox（当前按 cc 原话不写）；若需要再改成 `kind=failed`。
- D4 是否只做 `dispatch --once`，自动连续派发留 D5。
