# 代裁 ≈07:14 SGT — 授刷戳（条件式单键 reviewed_trees.exec → 1d0e0c5c…）（机房主任）

## 事实
- origin/main tip **81b86a47**（#95 paper-a；经 a7069c89）。相对 **a7069c89**：六审核目录 + gatequeue/gatedeps **差集空**（仅 research）。
- Latest **v0.0.39**；无 Draft v0.0.40（陈旧 Draft v0.0.31 忽略）。
- 材料齐：`/tmp/cc40-prep/stamp-A-0646/stamp-A-materials-0646.md` + inv/exec-delta。
- cdx 真审 **PASS**：`/tmp/cc40-prep/stamp-A-0646/stamp-A-true-review-cdx-0705.md` 末行「PASS 可进刷戳裁」；§5 1–5 勾选有据；第6只读候选全长 **1d0e0c5cf47ba3befd18d391491cb24d343dfc242aad7993a454ee57557f1825**（1412 成员与原 JSON 同）。
- 当前 `families.compilercheck.reviewed_trees.exec` = **fbb129700659fe8f225eb53a64fb384898f118b405d0a98f16ef2221bdd56336**（旧值只应出现 1 次）。
- 写键 fbb12970 **CLOSED**（勿重开旧认键叙事）；本刀是新候选写入。

## 代裁（宿主/卫生）— 授条件式单键刷戳
按材料 §4，**全部**满足才写，否则停手回报：
1. `git fetch` 后 origin/main 上相对基线路径：e418bdcf 之后 **exec 等六审核目录 + gatequeue/gatedeps** 差集仍恰为材料第2节两项（A gen-delta.sh + M prepare.sh）；等价验收：相对 a7069c89 该集合差集空亦可。
2. 写前复算 exec 戳（同 inv.py / 真审算法）**恰好**等于全长 `1d0e0c5cf47ba3befd18d391491cb24d343dfc242aad7993a454ee57557f1825`。
3. gatedeps 里旧值 `fbb12970…` 全长只出现 **1** 次。

落地：
- **只改一键**：`tests/gatedeps.json` → `families.compilercheck.reviewed_trees.exec` → 新值全长 `1d0e0c5c…`（上）。
- JSON 还原比对确认未改其他键；写后复算仍等于该戳。
- commit 文案仿前例：`gatedeps: compilercheck reviewed_trees.exec fbb12970 -> 1d0e0c5c after stamp-A true review (机房主任 07:14)`；可附极短 research 台账一行（可选）。
- **FF 推** origin/main（**禁止 force**）→ 回执路径写清。

## 明确不授
- 不跑 compilercheck / gate-infra / 门当本刀验收
- 不 bump；不开 Draft v0.0.40；不公开发布
- 不改其他 reviewed_trees / guards / DIFF / REFERENCE_KEYS
- 不授条件式以外的扩写；tip 若六目录差集扩大 → 停手回报，不混新旧
- 不删测、不改归因、不方向性大改

## 分工
- **cc**：持刀落地（锁 tip→写前复算→单键写→还原比对→写后复算→commit→FF→回执）。停点如上。
- **cdx**：只读核写值=目标且 diff 仅该键；**勿代写**、勿改仓、勿跑门。
- **cdx2**：只读盯墙钟/漏证/越权（多键、force、跑门、bump、Draft）；回执完备性；KPI Idle≤5m；不写仓不代跑。

## 度量
- 真审 ≈**100%** PASS；刷戳授权 ≈**100%**；落地执行本刀开跑 ≈**5%**
- K5-1h/memkey ≈**100%** CLOSED
- 在研 **0.0.40** 可度量收口约 **99.5%**（刷戳在途；Draft 未授）
- Latest **v0.0.39**；无 Draft v0.0.40
