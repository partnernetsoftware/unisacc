# 代裁 ≈07:05 SGT — 审核戳 A 材料齐 → 授权 cdx 真审（机房主任）

## 事实
- origin/main tip **81b86a47**（#95 paper-a；经 98e0f4db #94）。相对 **a7069c89**：六审核目录 + gatequeue/gatedeps **差集空**（仅 research）。
- Latest **v0.0.39**；无 Draft v0.0.40（陈旧 Draft v0.0.31 忽略）。
- K5-1h + parse2gen↔memkey **CLOSED**；写键 fbb12970 **CLOSED**（勿重开）。
- 材料齐：`/tmp/cc40-prep/stamp-A-0646/stamp-A-materials-0646.md` + inv-a7069c89.{json,out,rc} + exec-delta.diff；inv_rc=0；exec 1412；候选全长 **1d0e0c5cf47ba3befd18d391491cb24d343dfc242aad7993a454ee57557f1825**；差集恰 A gen-delta.sh + M prepare.sh。
- cc / cdx / cdx2 / grk ≈06:48–06:49 材料核齐；§5 全空；无人勾选/刷戳/改仓。
- 三窗 Idle ≈06:49 起等真审授（Idle 已超 KPI）。

## 代裁（宿主/卫生；真审≠刷戳）
1. **授权真审**：由 **cdx** 对材料 §5 **第 1–5 项**做真实内容审核（独立审；材料作者 cc **不自评**）。
   - 依据：材料全文（含口径更正一二）+ exec-delta.diff + tip **a7069c89**（或当前 tip 81b86a47，exec 字节同）上两文件只读全文。
   - tip a706→81b86a47 仅 research：不重开材料刀。
   - 产出仓外：`/tmp/cc40-prep/stamp-A-0646/stamp-A-true-review-cdx-0705.md`（逐项勾/不勾 + 一句依据；末行「PASS 可进刷戳裁」或「FAIL 列缺口」）。
   - 可把勾选结果追加到材料副本末尾；**不改仓内文件**。
2. **§5 第 6 项**：本刀只允许 **只读重算**并在回执记录候选戳（应为 1d0e0c5c…全长）；**不授**写入 `tests/gatedeps.json`（刷戳 / 条件式单键另裁）。
3. **明确不授**：刷戳/改 gatedeps；§4 条件式单键执行；bump/Draft；改 DIFF/REFERENCE_KEYS；删测/归因；重开 fbb12970；跑门当验收；实现刀。
4. tip 若再动且 **exec 等六审核目录差集扩大** → 停手回报，不混新旧。

## 分工
- **cdx**：真审持刀 → 写回执 → 停。勿刷戳、勿改仓、勿跑门。
- **cc**：Standby。勿叠写材料、勿自评勾选、勿改仓。材料已齐。
- **cdx2**：只读盯真审回执完备性（1–5+第6只读是否都有依据、有无越权暗示刷戳）+ 墙钟/漏证/提效（ruling≤2、Idle≤5m 本轮已破，本 paste 恢复）；不写仓不代跑。

## 度量
- K5-1h/memkey ≈**100%** CLOSED
- 审核戳 A 材料 ≈**100%**；真审本刀开跑 ≈**5%**；刷戳 0%（未授）
- 在研 **0.0.40** 可度量收口约 **99%**（主路径收口；真审在途；刷戳/Draft 未授）
- Latest **v0.0.39**；无 Draft v0.0.40
