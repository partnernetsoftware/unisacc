# WF2 填尾 短方案（cc 2026-10-10 ~22:05，机房主任 21:57 第4项）
范围：只证已在 main 的入窗/填尾机制启用读数；**不改代码、不改预算/重试契约、不跑全量/com 分片、不加白名单**。

## 获批入窗判据（照现行 tests/gatequeue.py@4ec76cb1 写明，非新规则）
1. 有效历史：entry(n) 仅当 history[n].id == 本轮该套件 stamp；否则冷先验（>=5 条有效 ok 取中位，否则 30 s）。
2. 估计 = min(span-2, max(2, max(ok,lb)×1.3+1))；只排序/准入，不定 limit、不判结果。
3. 准入：estimate ≤ left-1 且前驱已决；fullwindow 名单中的套件仅在 left ≥ span-3 时入；kind = solo(已重试) / full(left ≥ span-3) / tail。
4. limit = max(1, int(left)-1)，受父 deadline 约束（不放宽）。
5. tail 尝试 rc142 → DEFER（不是结果），记 lb=2×elapsed，进 fullwindow；full 尝试 142 → 单独重试一次，第二次为结果。

## 首观察窗
- 干净 wt @origin/main，真实 queue.sh fresh，QUEUE_SUITES = 12 个 contract 套件（tools-1 tools-2 tools-3 k2b queuestart-launch queuetimeout queuetimer gqalive observation attemptchain exittable stagelog），JOBS 默认 4，让后段自然出现 tail 准入。
- 采：历史文件（unisacc-gate-times-*.json）窗前/窗后快照及 sha；release-queue.log 全部 START(limit/kind/left)/DONE/DEFER；attemptchain --results --strict 回放；results.json；final rc（期望 65）；stagelog 配对。
- 读数：每个 tail START 用窗前历史+stamp 复算 entry 是否有效与 estimate ≤ left-1；DEFER/142 账；预算：每个 limit ≤ left-1 且窗口不超 window。
- 不可得者记 UNKNOWN（日志不记 estimate 来源，只能离线复算）；不推算墙钟。

## 不证
产品/com 填尾、真实外层超时、提效量化、多窗历史漂移。
