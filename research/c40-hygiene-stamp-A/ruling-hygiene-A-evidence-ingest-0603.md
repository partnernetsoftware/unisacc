# 代裁 ≈06:03 SGT — 卫生 stamp A 证据入仓（research-only）（机房主任）

## 消息入口评审 v62 tip bd6e48ee
- origin/main tip 已核实：**bd6e48ee**（full `bd6e48ee04011bb7bf0678bd38f8b5bc2ac178b8`；Merge #91 paper-a named-diff ledger）。
- 刷戳提交 **e418bdcf**（`gatedeps: … e9d2314a → fbb12970`，机房主任 05:52）所引用材料当时只在 box `/tmp`，m4pro/仓内不可核 → **不计通过**（宿主卫生缺口，非撤戳）。
- 现 tip 上 `tests/gatedeps.json` → `families.compilercheck.reviewed_trees.exec` = **fbb129700659fe8f225eb53a64fb384898f118b405d0a98f16ef2221bdd56336**（已落；本刀**不改** gatedeps）。

## 事实（box 上现仍存在，已核实）
| 源路径 | sha256（前 16） | 拟仓内名 |
|---|---|---|
| `/tmp/cc40-prep/next/hygiene-stamp-A-materials-0506.md` | bfd2781e49d834bf | `hygiene-stamp-A-materials-0506.md` |
| `/tmp/cc40-prep/next/hygiene-stamp-A-materials-0537.md` | 2f73591297b40308 | `hygiene-stamp-A-materials-0537.md` |
| `/tmp/cc40-prep/next/hygiene-stamp-A-true-review-cdx.md` | 1033318e4f650a68 | `hygiene-stamp-A-true-review-cdx.md` |
| `/tmp/unisacc-cdx2/observe-10m/ruling-hygiene-stamp-A-true-review-0539.md` | 972737e55660adb2 | `ruling-hygiene-stamp-A-true-review-0539.md` |
| `/tmp/unisacc-cdx2/observe-10m/ruling-hygiene-stamp-A-refresh-0552.md` | bc804da0ccb7ad65 | `ruling-hygiene-stamp-A-refresh-0552.md` |
| `/tmp/cc40-prep/next/hygiene-stamp-A-refresh-0552-review-cdx.md` | 66b4344837260118 | `hygiene-stamp-A-refresh-0552-review-cdx.md` |
| 本裁定（可选一并入） | — | `ruling-hygiene-A-evidence-ingest-0603.md` |

## 代裁（宿主卫生；可与 K5-1h 并行，勿冲撞其六文件实现域）
1. **原样**把上表证据拷入仓库目录：`research/c40-hygiene-stamp-A/`（新建；风格对齐 `research/c40-gate-infra-delta/`）。
2. **commit 说明**须指向 tip **e418bdcf** / 戳 **fbb12970**（及本消息入口 tip **bd6e48ee**）；例：
   `research(c40-hygiene-stamp-A): land hygiene true-review + refresh materials for e418bdcf/fbb12970 (机房主任 06:03)`
3. **pathspec 仅** `research/c40-hygiene-stamp-A/`（+可选极短 `plans/` 台账一行，指向仓内路径）。
4. 可另开 worktree（建议基 tip **bd6e48ee**）或 stash 策略自选；**禁止弄丢 K5-1h 私有改**（当前 cc 私有 WT：`/tmp/cc40-prep/k5-1h/wt`，HEAD bd6e48ee，已有 `exec/pipeline/prepare.sh` M + `exec/parse2gen/`）。
5. push/PR 按既有 research 流；**本执行器不自己改仓 push**——持刀为 **cc**。

## 明确不授
- 不改 `tests/gatedeps.json` / 任何 reviewed_trees
- 不重跑门、不 bump、不开 Draft、不公开发布
- 不改 K5-1h 实现文件（helper/prepare/夹具/schema/测试期望）
- 不撤 e418bdcf 戳；本刀只补仓内可核材料

## 分工
- **cc**：持刀 research 入仓。cwd 建议独立 worktree 或确认共享检出不覆盖 k5-1h/wt；**pathspec 勿碰 exec/tests**；拷贝用 `cp` 原样（保留文件名）；commit→FF/PR→回报仓内路径与 sha。可与 K5-1h 并行，但本刀不得碰六域文件。
- **cdx**：只读核：差集仅 `research/c40-hygiene-stamp-A/**`（±plans 一行）；六文件字节/sha256 与上表源一致；commit 文含 e418bdcf/fbb12970；无 gatedeps/exec/tests 夹带。短评 mux；勿代写。
- **cdx2**：只读盯：墙钟、是否误改 gatedeps/K5-1h、是否弄丢私有 WT 改；续 observe；不代裁、不写仓。

## 度量
- 证据入仓授权 ≈**100%**（本裁）；落地执行待 cc ≈**0%**；刷戳本身已在 tip（e418bdcf）；消息入口「材料可核」缺口待本刀闭合后可复议。
- K5-1h 私有实现在途（helper+prepare），与本刀正交。

## paste
- 方式：load-buffer + paste-buffer + Enter → 0:1 / 0:2 / 0:3
- 本文件：`/tmp/unisacc-cdx2/observe-10m/ruling-hygiene-A-evidence-ingest-0603.md`
- latest：`ruling-hygiene-A-evidence-ingest-latest.md` → 本文件
