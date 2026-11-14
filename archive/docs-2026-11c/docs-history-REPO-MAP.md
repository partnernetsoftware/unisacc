# docs/REPO-MAP.md 迁出的历史记录

来源文档：docs/REPO-MAP.md。迁移日期：2026-11-13。
说明：这是历史记录，不是当前指引。以下为被精简行的原文（括号内为原行号）。

## (REPO-MAP.md:22)

| `tools/cxlib/` | 工具共享库(.cx):`rechunk`(自动再分块)、`netpack`、`resolver`、`releasecandidate`、`wholefrontmerge`、`kernelsha` 等,以及 `mem/sys/sysx/text` 子库。`toolsloc.cx` 对应的 toolsloc 已退役,文件仅保留 | 否 | 主线 |

## (REPO-MAP.md:25)

| `build/` | 构造图与前提规则表(2026-11 新增):`graph.tsv`(一个候选的构造图,205 节点,含 split_dim/split_min,由 `tools/graph-check.cx` 校验)、`hardwire.tsv`(构造工具中「第二真值」硬连线模式清单,G1)、`preflight-rules.tsv`(0.0.50 候选链撞到的隐性前提,9 行,是 0.0.51 PF 预检工具的输入,omp-opus 整理) | 否 | 构造链维护者(G0/G1/PF 工作包) |

## (REPO-MAP.md:36)

| `plans/` | 版本计划与设计记录(约 90 个文件)。现行:`v0.0.52-unisacc.md`(模型层迭代占位)、`y-design-050.md`(Y 包设计)、`rechunk-design.md`、`standing-goal.md`、`design-review-log.md`;`release-notes-0.0.48` 与 `0.0.57` 至 `0.0.58`(0.0.46、0.0.55、0.0.56 的说明在 `archive/docs-2026-11d/plans/`,见其 INDEX.md)。过期的进 `archive/plans/`,2026-11 文档清理移走的在 `archive/docs-2026-11/plans/`,第二轮在 `archive/docs-2026-11b/`(含 `v0.0.50`、`v0.0.51` 计划与 `release-notes-0.0.47`,见其 INDEX.md) | 否 | 主线,交接单不再写带日期的 plans |
