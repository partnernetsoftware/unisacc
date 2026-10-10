# Paper A 心跳短报 · 2026-10-10 ~12:39 Asia/Shanghai

- **本拍切口=C**（真缺口钉）
- **主张一句：** 8509（op-74 / EN·arXiv）与 8769（op-87 / CN·tip gold）是已标注的测量双身份；免责已在正文，但单一冻结身份+sha256 未闭合 → **seal-blocking 硬缺口**；禁止互换 CN/EN 数字；与 Softguess（命名卫生）不同类。
- **tip：** `158ae864`（before 本拍 origin/main；after 合并见 PR）← Softguess 钉后 tip 曾为 `a1ac9287`，产品前进不漂 Paper A blob
- **四 blob：** CN `000b7ef1` · EN `93fdf052` · TeX `8122353c` · abs `9e883892` — **SAME**（Paper A blob drift? **no**；同锚 `04a5087b`）
- **挡粘：** Softguess/权重就是/are the logic **仍 NONE×4**；正文键数**未改**（本钉只 research notes）
- **待批：** ~03:44 同口气两板仍有效——先批 P0 粘贴；之后路线Ι 测量身份。**本拍不叠卡。** 本钉供路线Ι 引用。
- **真硬缺口仍开：** **测量身份（本钉）**、表口径、跨平台矩阵
- **证据路径：**
  - 缺口钉：`research/notes/paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md`
  - 本短报：`research/notes/paper-a-heartbeat-report-20261010-1239.md`
  - 先验重测：`research/seal-remeasure-20261009.md` / `research/seal-remeasure-tip.json`
  - Softguess 边界（对照）：`research/notes/paper-a-gap-nail-softguess-seal-boundary-20261010-1230.md`
- **commit/PR：** 见本拍 research-only PR（仅 notes + paper-a-notes 一行指针）
- **禁令核对：** 未改键数/产品/投稿方向；未粘 Softguess；未叠决策卡；未改正文；未伪造新测量（仅复核 pin）。

## verify 摘要

```
tip:           158ae864 (= origin/main at beat start)
vs Softguess:  a1ac9287 (Paper A blobs SAME)
Paper A anchor tip: 04a5087b
blobs:         000b7ef1 / 93fdf052 / 8122353c / 9e883892 (all SAME)
Softguess:     NONE×4
dual-id:       disclaimer still present (CN 8769/569 op-87; EN/arXiv 8509/517 op-74)
gold pin tip:  b8244bd8… still matches; domain sum 8769
cut:           C → measurement identity HARD gap (seal-blocking)
~03:44 boards: untouched
ALL_PASS:      True (research-only)
```
