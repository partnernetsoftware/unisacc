# 代裁 ≈11:53 CST — 独立复算认键 families.compilercheck.reviewed_trees.src（机房主任执行器；续跑）

## 入口
- 续跑被打断：冻后 src reviewed_trees 独立复算认键。
- fetch tip **10c2e9b7**；gatedeps src 戳前缀曾报 `35ab1353…`。
- 约束：独立复算全长才认键；吻合 → 认+回执入 research、**勿改 gatedeps**；不符 → 停手。勿打断首观察补证。

## 复算结果
| 项 | 值 |
|---|---|
| tip（tree-ish） | `10c2e9b768f422f8646e930aace687321f528d4f`（= origin/main = ls-remote） |
| 键 | `families.compilercheck.reviewed_trees.src` |
| gatedeps 所记 | `35ab135379d27bc933628e70453c5c8502db547236bf7a550b70141393ad6f56` |
| 本机复算 **全长** | **`35ab135379d27bc933628e70453c5c8502db547236bf7a550b70141393ad6f56`** |
| 与目标 | **吻合** |
| 成员数 | 17（inventory_suffixes 过滤；`all_files_trees` 仅 include；`excluded_dirs` 空） |
| 算法 | `git -c tar.umask=022 archive HEAD src` → digest=`[st_mode, sha256(content)]` → `sha256(json.dumps(map, sort_keys=True))`（同 `tests/refresh_gatedeps.py` / gatequeue） |

## 裁定
1. **认 `families.compilercheck.reviewed_trees.src` = `35ab1353…ad6f56` 键有效**（冻后 tip 独立复算全长吻合）。
2. 回执+ack 入 `research/c40-freeze-src-recompute/`；**勿改 gatedeps**（已是目标值）。
3. 「未核 src 目录戳 → 不计通过」对本键 **解除**（维持 1140；本 tip 重核仍吻合）。
4. 不因本 ACK 把 queuecheck provenance 红、.cx 三绿、Draft/首观察计通过。
5. **未打断** 0:1/2/3 首观察补证（1152）；本刀未向三窗 paste 抢占。

## 不授
bump / Draft / 公开发布 / 改 gatedeps / 跑全量门 / 改验收措辞 / 打断首观察补证。

## 度量
- 独立复算核验 ≈**100%** PASS（本机全长）
- gatedeps 本刀改动 ≈**0%**
- TRUE_PROGRESS：**yes**（tip 前进后 src 戳重核仍吻合）
- 卡点：首观察补证 1152（0:1 持刀）；Draft/seal 另裁；queuecheck provenance 宿主条件（既有）

— 机房主任执行器 2026-10-11 11:55 CST
