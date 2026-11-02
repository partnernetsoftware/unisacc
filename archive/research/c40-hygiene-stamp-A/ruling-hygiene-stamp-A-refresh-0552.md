# 代裁 ≈05:52 SGT — 授刷戳（单键 reviewed_trees.exec → fbb12970…）（机房主任）

## 事实
- origin/main tip **b0548cd1**（full `b0548cd155b07848a256eec877ac9c78d4d70a1d`；相对 ac39786a 仅 paper-a research 4 文件 #90；**exec/include/kernel/src/unisa/weights 差集空**）。
- Latest **v0.0.39**；无 Draft v0.0.40（陈旧 Draft v0.0.31 忽略）。
- A 材料齐：`/tmp/cc40-prep/next/hygiene-stamp-A-materials-0537.md`（+0506）；候选戳 **fbb129700659fe8f225eb53a64fb384898f118b405d0a98f16ef2221bdd56336**。
- cdx 独立真审 **PASS**：`/tmp/cc40-prep/next/hygiene-stamp-A-true-review-cdx.md`（§5 1–6 全勾；末行「PASS 可进刷戳裁」；独立复算 1411 成员全等）。
- 当前 `tests/gatedeps.json` → `families.compilercheck.reviewed_trees.exec` 旧值 = `e9d2314a3dcba3bc378075217f6c1819550e81fb32f7f44313c79bd461760c34`（与预期一致）。
- 共享检出仍 **06855852**（detached）；勿动共享写者；落地用可写 worktree/分支。
- 三窗 Idle（真审已结），等刷戳授。

## 代裁（宿主/卫生）— 授刷戳
1. **只改一个键**：`tests/gatedeps.json` → `families.compilercheck.reviewed_trees.exec`
2. **新值（唯一）**：`fbb129700659fe8f225eb53a64fb384898f118b405d0a98f16ef2221bdd56336`
3. **停点**：写前 fetch 锁 tip → 写前独立复算候选仍为 fbb12970… → 只写该键并用 JSON 还原比对 → 写后复算 → commit（可附极短 research/plans 一行：「b0548cd1/ac39786a exec 卫生真审后刷戳，依据 0537+cdx PASS+机房主任 05:52 裁」）→ **FF** 推 origin/main（**禁止 force**）→ 回执仓外。
4. **前置锁 tip**：落地前须再 `git fetch` 确认 origin/main 仍是 **b0548cd1** 或 tip 上相对 b0548 **无新 exec 变动**；若 tip 上已有新 exec 变动 → **停手回报**，勿刷。

## 明确不授
- 不跑 compilercheck / gate-infra / 满门当本刀验收
- 不 bump；不开 Draft；不公开发布
- 不改其他 `reviewed_trees` / guards / inventory
- inventory 口径仍交政委；K5-1h 改授仍待决
- 不为 KPI 合并隐去本轮另授；本链材料/真审/刷戳三次具名授如实计

## 分工
- **cc**：持刀落地（锁 tip→写键→commit→FF 推→回执）。停点如上。勿越权改他键/跑门/bump。
- **cdx**：只读核写值=目标且差集仅该键；**勿代写**、勿改仓、勿跑门。
- **cdx2**：只读盯（1）真审回执完备补记 PASS 已落；（2）刷戳墙钟/漏证/越权（多键、force、跑门、bump）；续 hygiene-0458-watch；不代裁、不写仓。

## 度量
- A 真审 ≈**100%** PASS；刷戳授权 ≈**100%**（本裁）；落地执行本刀开跑 ≈**5%**；门闩≈99.95%；K5-1h≈10% Standby。
