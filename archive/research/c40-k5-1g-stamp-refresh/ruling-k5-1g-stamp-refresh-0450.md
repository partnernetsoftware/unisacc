# 代裁 ≈04:50 SGT — 授刷戳（单键 reviewed_trees.exec → e9d2314a…）（机房主任）

## 事实
- 共享 tip **699d310d**（full `699d310d7b983123370142d90080c57c5b457c8f`；`git fetch` 后 origin/main 仍是它；699d 之后 tip 上无新 exec 变动）。本地旁支 0e3518e6（paper-a research）非本线产品。
- A 材料齐：`/tmp/cc40-prep/next/stamp-audit-699d310d.md`；差集 4M+1A；候选戳 `e9d2314a3dcba3bc378075217f6c1819550e81fb32f7f44313c79bd461760c34`。
- cdx 语义真审 PASS：`/tmp/cc40-prep/next/stamp-audit-699d310d-true-review-cdx.md` 末行「PASS 可进刷戳裁」。
- cdx2 完备核齐（1–7 依据齐；末行仅可进裁；无越权绿暗示）。
- 当前 `tests/gatedeps.json` → `families.compilercheck.reviewed_trees.exec` 旧值 = `2d52b9bc856caf12e939c40e29a860fb5a236e71415abfb173257b02971aea27`（与预期一致）。

## 代裁（宿主）— 授刷戳
1. **只改一个键**：`tests/gatedeps.json` → `families.compilercheck.reviewed_trees.exec`
2. **新值（唯一）**：`e9d2314a3dcba3bc378075217f6c1819550e81fb32f7f44313c79bd461760c34`
3. **停止点**：写键 → commit（可附极短 research/plans 台账一行：「699d310d exec 真审后刷戳，依据 stamp-audit + cdx PASS + 机房主任 04:4x 裁」）→ FF 推 origin/main（**禁止 force**）→ 回执。
4. **前置锁 tip**：落地前须再 `git fetch` 确认 origin/main 仍是 699d310d；若 tip 上 699d 之后已有新 exec 变动 → **停手回报**，勿刷。

## 明确不授
- 不跑 compilercheck / gate-infra 当本刀验收
- 不 bump；不开 Draft；不公开发布
- 不改其他 `reviewed_trees` / guards
- inventory 不碰（仍交政委）
- 不为 KPI 自动扩刷；699d 之后新 exec 变动另审另裁

## 分工
- **cc**：持刀落地（写键→commit→FF 推→回执）。停点如上。勿越权改他键/跑门/bump。
- **cdx**：只读核写值=目标且差集仅该键；**勿代写**、勿改仓、勿跑门。
- **cdx2**：只读盯墙钟/漏证/越权（有无多键改动、force、跑门、bump）；不代裁、不写仓。

## 度量
- 刷戳授权 ≈**100%**（本裁落地）；落地执行本刀开跑 ≈**5%**；门闩≈99.9%；K5-1g≈100%。
