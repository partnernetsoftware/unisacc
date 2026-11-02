# 代裁 ≈07:36 SGT — 独立复算认键（1b81c2e4 / 1d0e0c5c）（机房主任执行器）

## 入口
- 消息 v67 tip 曾指 7396209b；fetch 后 origin/main tip **3c6f8ba7**（含 paper-a #97 等；相对 a706 六审核目录差集仍空）。
- 写键已发生：`1b81c2e4` 将 `families.compilercheck.reviewed_trees.exec` fbb12970→1d0e0c5c（巡检 07:23 已闭合）。
- 约束：**1d0e0c5c 本机未复算 → 不计独立核验通过**；禁把未复算本机值计通过。本刀只做只读复算 + research 回执/ack。

## 锁 tree-ish
**a7069c89d7588d1168e509513eb6a8541b181a2e**（材料 tip；六目录相对 tip 空差）。

## 复算结果
| 项 | 值 |
|---|---|
| 算法 | `/tmp/cc40-prep/gi1/inv.py` sha ff730eea…（gatequeue 同类） |
| inv_rc | **0** |
| exec members | 1412 |
| stamp 全长 | **1d0e0c5cf47ba3befd18d391491cb24d343dfc242aad7993a454ee57557f1825** |
| 与目标 | **吻合** |
| JSON | 与材料 inv-a7069c89.json 逐字节相同（sha 1e5e7deb…） |

## 裁定
1. **认 `1b81c2e4` 写键有效**（同 fbb12970 复算认键口径）。
2. 回执+ack 入 `research/c40-stamp-A-k5-1h/`（仅 research）；**勿再改 gatedeps**。
3. 「本机未复算」卡点 **解除**。

## 不授
bump / Draft / 跑门 / K5-1i 实现 / 改测期望；勿打断 P0 seedparse2 卫生（本刀 research 另 wt）。

## 分工（paste）
- **cc**：仅 research 入仓（本目录两份回执/ack + 本裁定副本）；FF；勿改 gatedeps。
- **cdx**：只读核全长吻合与 ack 口径；勿代写。
- **cdx2**：只读盯越权（改 gatedeps / 跑门 / bump）；KPI。

## 度量
- 独立复算核验 ≈**100%** PASS（本机）
- 写键 1b81c2e4 认键 ≈**100%**
- gatedeps 本刀改动 ≈**0%**（禁改）
