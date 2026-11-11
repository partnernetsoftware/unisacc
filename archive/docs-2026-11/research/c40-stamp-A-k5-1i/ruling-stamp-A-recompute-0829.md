# 代裁 ≈08:29 SGT — 独立复算认键（40ec2903 / 828a6f67）（机房主任执行器）

## 入口
- 消息 v70 tip 曾指 40cbc16e；fetch 后 origin/main tip **40cbc16e**（含 K5-1i 材料+真审+写回执；相对 40ec2903 仅 research 前移）。
- 写键已发生：`40ec2903` 将 `families.compilercheck.reviewed_trees.exec` 1d0e0c5c→828a6f67（巡检 08:22 已见）。
- 约束：**828a6f67 本机未复算 → 不计写入核验通过**；禁把未复算本机值计通过。本刀只做只读复算 + research 回执/ack。

## 锁 tree-ish
**4470a3f97a99c17493f8d9db117deaab83260313**（写键父；材料 tip；与 848bc986 六审核目录相等）。

## 复算结果
| 项 | 值 |
|---|---|
| 算法 | `/tmp/cc40-prep/gi1/inv.py` sha ff730eea…（gatequeue 同类） |
| inv_rc | **0** |
| exec members | 1412 |
| stamp 全长 | **828a6f67417de1998627ef73d1346ff9709ef3cccd1cb7f2ebd904baf09a4775** |
| 与目标 | **吻合** |
| JSON | 与材料 inv-848bc986.json 逐字节相同（sha 365b55de…） |

## 裁定
1. **认 `40ec2903` 写键有效**（同 1b81c2e4 / 0736 复算认键口径）。
2. 回执+ack 入 `research/c40-stamp-A-k5-1i/`（仅 research）；**勿再改 gatedeps**。
3. 「本机未复算」卡点 **解除**。

## 不授
bump / Draft / 跑门 / 刷 guard / K5 新实现 / 改测期望；勿冲撞 guard 旧债材料刀（本刀 research 另 wt）。

## 分工（paste）
- **cc**：仅 research 入仓（本目录回执/ack + 本裁定副本）；FF 到 tip；勿改 gatedeps；**不自 push 由本执行器**——cc 普通快进推送即可。
- **cdx**：只读核全长吻合与 ack 口径；勿代写。
- **cdx2**：只读盯越权（改 gatedeps / 跑门 / bump / 刷 guard）；KPI。

## 度量
- 独立复算核验 ≈**100%** PASS（本机）
- 写键 40ec2903 认键 ≈**100%**
- gatedeps 本刀改动 ≈**0%**（禁改）
- TRUE_PROGRESS：**yes**（本机复算吻合 → 写入核验卡点解除）
