# superharness / super-agent 后续 Todo Tree

日期：2026-11-02
状态：计划。实现仍只在 `out/superharness/`；不碰 `src/`、`cx/`、`kernel/`、`tools/`、`tests/`。
设计依据：`plans/super-agent-omp-study-2026-11-02.md`（DeepSeek Super-agent 研究 + omp 映射 + D1–D7）。

## 0. 基线校正

- 设计研究**已经存在**：上一条“0%”是文件盘点误报；以本文件为准。
- 当前不是从零开始：D1、D2、D3a/D3b、D4、D5a 已完成，D3c、D5b、D6、D7 未完成。
- 按原 D1–D7 里程碑约 **4.5/7**：
  - D1 ✅ schema/intake/CLI/server/扩展桥
  - D2 ✅ dry-run Router + 单真实 worker
  - D3a/D3b ✅ fake RPC + 真实 `router --real --once`；**D3c ❌ 常驻/重启**
  - D4 ✅ 单只读 worker、工具白名单、outbox、超时/崩溃/超 token
  - D5a ✅ 一次孤儿/PGID 精确清理；**D5b ❌ 常驻 watchdog、语义进展检查、自动连续派发**
  - D6/D7 ❌ 影子、runbook、成本/延迟数据
- 当前 `out/superharness` 测试 27/27 是最近一次实跑基线；新增功能必须保持全绿。

## 1. Todo Tree

```text
superharness MVP
├── P0 基线与文档                         [进行中]
│   ├── [x] 设计研究写入 plans/super-agent-omp-study-2026-11-02.md
│   ├── [x] D1–D5a 代码/接口/测试落盘
│   ├── [ ] 校正 omp-board 状态：D3c/D5b 未完成，不再写“D5 完成”
│   ├── [ ] 冻结结束后确认 D1–D5 回归基线（bun test + 一次 doctor）
│   └── [ ] 明确“不碰 unisacc 产品源”的边界与证据路径
│
├── P1 D3c 常驻 RouterRunner                [最高优先]
│   ├── [ ] D3c-1 循环：new trigger → new_session → prompt → decision → 下一条
│   ├── [ ] D3c-2 router_session/router_turn 心跳、状态、turn 幂等
│   ├── [ ] D3c-3 `router --watch` 常驻；`router status`/`router stop`
│   ├── [ ] D3c-4 崩溃/failed 后 10 分钟内最多重启 1 次；不误杀 worker
│   ├── [ ] D3c-5 doctor 增加 Router 行（PID/PGID/starttime/最后 turn）
│   ├── [ ] D3c-6 测试：3 trigger 串行、同 PID、3 turns、kill 后 reclaim
│   └── [ ] D3c-7 验收：无 orphan、预算含 Router、无第二 Router
│
├── P2 D5b 连续派发 + 常驻 Watchdog         [高优先]
│   ├── [ ] D5b-1 `dispatch --watch`：pending → worker → completed/failed → 下一条
│   ├── [ ] D5b-2 maxWorkers=2 槽位；同一 ticket 单 active dispatch
│   ├── [ ] D5b-3 槽满排队，不丢 trigger、不无限重试
│   ├── [ ] D5b-4 常驻 watchdog：60s 检查心跳/最后 report/token，先 steer 后 abort
│   ├── [ ] D5b-5 只按台账 pid+starttime+PGID 操作；禁止 pkill -f
│   ├── [ ] D5b-6 自动派发与 Router 共存：route 只建 pending，dispatch 消费
│   └── [ ] D5b-7 测试：2 active + 1 queued、超时/崩溃后继续下一条
│
├── P4 Source / 人向通道                    [MVP 必需]
│   ├── [ ] P4-1 envelope mailbox Source：目录扫描、去重、thread 绑定
│   ├── [ ] P4-2 回信走 outbox → envelope；不直接以 worker 名义发
│   ├── [ ] P4-3 `outbox send --once`；失败重试/幂等/审计
│   ├── [ ] P4-4 CLI 人类指令与 envelope 共用 Trigger 模型
│   └── [ ] P4-5 测试：同一邮件重复投递只入库一次；回信不重复
│
├── P5 政策 / 授权 / 安全                   [MVP 必需]
│   ├── [ ] P5-1 Gate fast path：source/actor/命令/路径白名单
│   ├── [ ] P5-2 Mandate：ticket 授权范围、强度、政策版本
│   ├── [ ] P5-3 worker 只在 work/<ticket>/；默认 read-only；写操作须显式授权
│   ├── [ ] P5-4 凭据不进入 prompt/日志；token 文件 0600
│   └── [ ] P5-5 测试：越权路径/工具/来源被拒，不建 dispatch
│
├── P6 记忆 / Chain of Recall               [MVP 后半]
│   ├── [ ] P6-1 Router 上下文：最近 N trigger + 活跃 ticket 卡片 + thread 摘要
│   ├── [ ] P6-2 `harness_read` 按 trigger/ticket/thread ID 召回
│   ├── [ ] P6-3 超预算内容外置为对象 + 引用，不整段塞 prompt
│   ├── [ ] P6-4 inline/spill 预算单测；召回不重复读全仓
│   └── [ ] P6-5 测试：长历史只保留摘要，按 ID 能取回原文
│
├── P7 影子模式 / 对照                     [进入 cc 委托前]
│   ├── [ ] P7-1 shadow：只记录、不回复、不派 worker
│   ├── [ ] P7-2 跑 10 条真实 envelope trigger
│   ├── [ ] P7-3 对照 cc 人工决策：accept/ignore/attach 差异
│   ├── [ ] P7-4 成本/延迟：token、turns、wall time、误判率
│   └── [ ] P7-5 门槛：误判可解释、无孤儿、预算可控才进阶段 1
│
├── P8 Runbook / 发布门槛                  [收尾]
│   ├── [ ] P8-1 启停/恢复/排障：server、router、dispatch、watchdog
│   ├── [ ] P8-2 故障剧本：RPC 半帧、omp 升级、Session 锁、端口漂移
│   ├── [ ] P8-3 成本模型：Router + N worker + watchdog
│   ├── [ ] P8-4 回滚：停 harness 不影响 cc/omp-* 现有窗口
│   └── [ ] P8-5 一页报告，决定是否进入 cc 委托阶段 2
│
└── P9 设计加分项（非 MVP，先不做）
    ├── [ ] 多 Worker 订阅同一 Thread、订阅直通
    ├── [ ] GitHub Source（只读 issue/PR + 评论）
    ├── [ ] 推测执行与成本回收
    ├── [ ] WebSocket/SSE、移动端
    └── [ ] 计算/状态/环境分离（云 stateless + 设备 daemon）
```

## 2. 依赖顺序

1. **P1 D3c** 必须先行：没有常驻 Router，P2 自动派发没有稳定决策源。
2. **P2 D5b** 依赖 P1：先有 Router 的 pending dispatch，再有连续消费。
3. **P4 Source** 可在 P1 期间并行做本地 mailbox 适配；不依赖真实 GitHub。
4. **P5 政策** 与 P4 同批：Source 进来的 trigger 必须先过 Gate。
5. **P6 记忆** 在 P2 稳定后做；否则先用固定摘要，避免过早优化。
6. **P7 影子** 是进入 cc 委托的硬门槛；P8 在影子数据后写。

## 3. 每项验收证据

- 代码：`out/superharness/` 内变更；不出现产品路径 diff。
- 测试：`bun test` 全绿；新增测试必须能抓住对应故障（超时、崩溃、重复、越权、孤儿）。
- 实跑：至少一次真实 cheap model smoke；记录 token、耗时、线程前后、`doctor` 的 live/orphan。
- 进程：所有子进程有 `pid/pgid/starttime` 台账；清理前后核对，禁止模式杀。
- 预算：Router/Worker/Watchdog 都计入 `maxProcesses`；槽位、spawn/hour、token 上限不可绕过。
- 失败：必须有终态；不能把 timeout/crash 当 ready；不能缓存 PASS。

## 4. 下一步（冻结解除后）

1. 先做 **D3c-1/2/3**，跑 3 trigger 串行与 kill-reclaim 测试。
2. 再做 **D5b-1/2/4**，跑 2 active + 1 queued + 超时恢复。
3. 然后接 **P4-1 envelope mailbox** 与 **P5-1 Gate**，跑影子模式。
4. 在 P7 数据出来前，不接 GitHub、不做 WebSocket、不改 unisacc 主线。

## 5. 风险与硬约束

- 进程爆炸：任何 spawn 前先查槽位/深度/线程/spawn 预算；`SUPERHARNESS_DEPTH>0` 拒绝。
- RPC 漂移：锁 omp 版本；用 `rpc-wire.schema.json`；升级先跑 fake/真实各一次。
- 成本：Router 常驻 token 不可忽略；先 cheap model + 每 turn 上限，再谈多 worker。
- 误判完成：report 必须带证据；Watchdog 不能仅凭“进程活着”判健康。
- 与主线冲突：superharness 只动 `out/`；cc 保留 stop/accept 权，Worker 不直接回复人。
