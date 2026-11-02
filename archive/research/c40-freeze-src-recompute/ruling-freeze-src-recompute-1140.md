# 代裁 ≈11:40 SGT — 独立复算认键 families.compilercheck.reviewed_trees.src（机房主任执行器）

## 入口
- 消息入口 v80 tip **6a5c3c8e**；0.0.40 冻结已开（`version.h`→0.0.40）。
- gatedeps 中 version.h/README/queuecheck 与本机一致（README 旧哈希已修正）；gatelayers/docs 绿。
- **未核 src 目录戳 `35ab1353…` → 不计通过**（本刀专核）。
- queuecheck provenance 红 = 既有 `UNISACC_FFI_X86_PROVIDER` 宿主条件，非本轮新增。
- 约束：独立复算全长才认键；吻合 → 认键有效、回执入 research、**勿改 gatedeps**；不符 → 停手。禁 bump/Draft/公开。

## 复算结果
| 项 | 值 |
|---|---|
| tip（tree-ish） | `6a5c3c8e78d27fedd01ef8888c15fe9cddea5546`（= origin/main = ls-remote） |
| 键 | `families.compilercheck.reviewed_trees.src` |
| gatedeps 所记 | `35ab135379d27bc933628e70453c5c8502db547236bf7a550b70141393ad6f56` |
| 本机复算 **全长** | **`35ab135379d27bc933628e70453c5c8502db547236bf7a550b70141393ad6f56`** |
| 与目标 | **吻合** |
| 成员数 | 17（inventory_suffixes 过滤；`all_files_trees` 仅 include；`excluded_dirs` 空） |
| 算法 | `git -c tar.umask=022 archive HEAD src` → digest=`[st_mode, sha256(content)]` → `sha256(json.dumps(map, sort_keys=True))`（同 `tests/refresh_gatedeps.py` / gatequeue） |

## 裁定
1. **认 `families.compilercheck.reviewed_trees.src` = `35ab1353…ad6f56` 键有效**（冻后 tip 独立复算全长吻合）。
2. 回执+ack 入 `research/c40-freeze-src-recompute/`；**勿改 gatedeps**（已是目标值）。
3. 「未核 src 目录戳 → 不计通过」对本键 **解除**。
4. 不因本 ACK 把 queuecheck provenance 红、.cx 三绿、Draft/首观察计通过。

## 不授
bump / Draft / 公开发布 / 改 gatedeps / 跑全量门 / 改验收措辞。

## 分工（paste；并行只读）
- **cc（0:1）**：知悉 src 认键；继续 m4pro .cx 三绿盯；本 research 由执行器落地；勿改 gatedeps。
- **cdx（0:2）**：只读核全长吻合与 ack 口径；短记 `freeze-src-recompute-review-cdx-1140.md`。
- **cdx2（0:3）**：盯越权（改 gatedeps / bump / Draft）；短记 `cdx2-freeze-src-watch-1140.md`。

## 度量
- 独立复算核验 ≈**100%** PASS（本机全长）
- gatedeps 本刀改动 ≈**0%**
- TRUE_PROGRESS：**yes**（src 戳独立复算吻合 → 冻后 src 核验卡点解除）
- 卡点：m4pro .cx 三绿；Draft/首观察另裁；queuecheck provenance 宿主条件（既有）

— 机房主任执行器 2026-10-11 11:42 CST
