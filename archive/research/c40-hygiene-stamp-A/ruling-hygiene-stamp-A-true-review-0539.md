# 代裁 ≈05:39 SGT — A 材料齐（0537 刷新三方核完）→ 授权 cdx 真审勾选（机房主任）

## 事实
- origin/main tip **ac39786a**（paper-a GATE cut #89；相对 dbb4b10d 仅 research 4 文件；**exec/gatequeue/gatedeps 差集空**）。
- Latest **v0.0.39**；无 Draft v0.0.40（陈旧 Draft v0.0.31 忽略）。
- A 材料：`/tmp/cc40-prep/next/hygiene-stamp-A-materials-0506.md` + 刷新 `hygiene-stamp-A-materials-0537.md`；0537 确认 470→dbb exec 无差、JSON 逐字节同 d98c77ec…、候选 **fbb129700659fe8f225eb53a64fb384898f118b405d0a98f16ef2221bdd56336**、§5 全空。
- cc / cdx / cdx2 / grk 0537 刷新核完；材料≠真审≠刷戳；无人勾选/刷戳/改仓/跑门。
- 共享检出仍 **06855852**（detached）；勿动共享写者。
- 三窗 Idle，等真审授。

## 代裁（宿主/卫生；真审≠刷戳）
1. **授权真审**：由 **cdx** 对 0506§5 清单 **第 1–6 项**做真实内容审核（独立审；材料作者 cc 不自评）。
   - 依据：0506 全文（含收窄）+ 0537 刷新段 + `/tmp/cc40-prep/sa2/exec-delta.diff` + tip **ac39786a**（或 dbb4b10d，exec 字节同）上三 helper 源码只读。
   - tip 自 dbb4b10d→ac39786a 仅 research：不重开材料刀；真审基准仍为 06855852→470b2b49 三 helper M（0537 已证后续 tip exec 无扩）。
   - 产出仓外：`/tmp/cc40-prep/next/hygiene-stamp-A-true-review-cdx.md`（逐项勾/不勾 + 一句依据；末行「PASS 可进刷戳裁」或「FAIL 列缺口」）。
   - 可把勾选结果追加到材料副本末尾；**不改仓内文件**。
2. **§5 第 6 项**：本刀只允许只读重算并在回执记录候选戳（应为 fbb12970…）；**不授**写入 `tests/gatedeps.json`（刷戳另裁）。
3. **明确不授**：刷戳/改 gatedeps/跑门当验收；bump/Draft/force；改 DIFF/删测/归因；inventory 口径；K5-1h 实现/改授；动共享检出 push。
4. tip 若再动且 **exec 差集扩大** → 停手回报，不混新旧。

## 分工
- **cdx**：真审持刀 → 写回执 → 停。勿刷戳、勿改仓、勿跑门。
- **cc**：Standby。勿叠写材料、勿自评勾选、勿改仓。材料已齐。
- **cdx2**：只读盯真审回执完备性（1–6 是否都有依据、有无越权暗示刷戳）+ 墙钟/漏证/提效（ruling≤2、Idle≤5m）；短记可续 hygiene-0458-watch。

## 度量
- A 材料 ≈**100%**；真审本刀开跑 ≈**5%**；刷戳 0%（未授）；门闩≈99.9%；K5-1h 仍 Standby（改授待决）。
