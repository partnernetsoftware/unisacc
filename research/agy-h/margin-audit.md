# 单批冷态余量审计结论 (2026-11-24)

## 1. 数据口径与覆盖盘点

以 [tests/batches.tsv](../../tests/batches.tsv) 全量 2014 批(`1986 IMPLEMENTED` + `28 PLANNED`)为基准,合并五类只读计时源(`~/.cache/unisacc/batch-times.tsv` 10213 行 / 1001 个已登记批、[tests/commit-tier-timings.tsv](../../tests/commit-tier-timings.tsv) 286 批、[tests/tier-runtime-estimates.tsv](../../tests/tier-runtime-estimates.tsv)、[build/phase-times.tsv](../../build/phase-times.tsv) 与 [build/speed-baseline.tsv](../../build/speed-baseline.tsv)、以及 `research/` 与 `~/.local/share/unisacc/research/` 非副本日志中的 98 条 `BATCH_TIMING` 行与 `G24`/`G25`/`G26`/`G29` 复核记录),不跑重命令、不碰 `.claude/worktrees/v0.0.106`。完整逐行清单见 [margin-audit.tsv](margin-audit.tsv) (共 1031 行,按 S1..S7 严重度降序排列)。

- (1) 全量 2014 批分布:
  - 实测破线 (`S1 + S2 + S3 + S4`): **220** 批 (`commit` 93、`daily` 126、`release` 1)。
  - 全源零实测 (`S5 + S6 + S7`): **811** 批,其中已实现零记录 **783** 批(`commit` **459**、`daily` **262**、`release` **62**),`PLANNED` 未实现 **28** 批。
  - 现有记录均不超过 27.5 秒且不超过半限 (`<= budget/2`): **983** 批 (未入明细表)。注意其中仅 **11** 批在 `research/` 有明确冷态证实 `<= 27.5s` (`devsvc-core`、`devsvc-core-jobs`、`devsvc-core-cancel`、`devsvc-git-c-background` 4.5s、`devsvc-git-c-operations` 17.9s、`affected-gitnode` 22-26s、`c99-*-incomplete` 等);另 **578** 批最近与峰值均 `< 5s`、**225** 批在 `5..27.5s` (均来自未关闭 `run-cache` 的 `run-batch.cx`,非纯冷态证实)、**169** 批仅来自历史 `commit-tier-timings.tsv`。

## 2. 严重度分级与关键发现

| 档位 | 定义 | 批数 | 分布 (commit / daily / release) |
| --- | --- | --- | --- |
| `S1_OVER_BUDGET_OR_TIMEOUT` | 最近非热/冷测/看门狗超时已触及限时 | 38 | 16 / 21 / 1 |
| `S2_LATEST_OVER_27_5S` | 最近非热实测 > 27.5s 且 < 限时(破 R22 50% 余量) | 66 | 37 / 29 / 0 |
| `S3_COLD_OR_HIST_OVER_27_5S` | 最近一次 <= 27.5s(热跑或早杀),但冷测或历史峰值 > 27.5s | 112 | 39 / 73 / 0 |
| `S4_OVER_HALF_SHORT_BUDGET` | 短限批(`< 55s`)实测 <= 27.5s 但已超过自身限时一半 | 4 | 1 / 3 / 0 |
| `S5_NO_RECORD_COMMIT` | 已实现 `commit` 层无任何计时记录 | 459 | 459 / 0 / 0 |
| `S6_NO_RECORD_DAILY_REL` | 已实现 `daily`/`release` 层无任何计时记录 | 324 | 0 / 262 / 62 |
| `S7_PLANNED_UNMEASURED` | `PLANNED` 未实现批 | 28 | 0 / 0 / 28 |

重点风险点:

- (1) **S1 最急批(38 批)**:
  - `commit` 层 16 批: `precommit-gates-na` (53.08s, kill=5)、`push-replay-commit-push` (53.13s FAIL, kill=3)、`layer-api` (冷测 `53.117s` 超时,热态仅 24.7s)、`precommit-gates-toolsreg-a2` (53.13s UNKNOWN)、`batch-timing` (首冷 `55.453s`)、`premerge-selfcheck-main` (53.23s FAIL)、`precommit-gates-toolsreg-d` (52.12s)、`devsvc-remain` (51.18s)、`precommit-gates-reuse-commit` (50.96s)、`devsvc-core-b` (50.60s)、`devsvc-core-c` (48.60s)、`devsvc-compile-context` (48.42s)、`commit-own-reg-a` (首冷 47.25s, 热 5.35s)、`precommit-gates-reuse-key` (46.39s KILLED)、`land-patch` (46.35s KILLED)、`devsvc-run-b2` (43.64s, kill=2)。
  - `daily`/`release` 层 22 批: `precommit-toolsloc` / `-source` / `-symlink` (45s 限下 46.1-46.3s KILLED, kill=8)、`ratchets-tool` (46.2s KILLED / 峰值 53.12s, kill=9)、`cx-npfault` (44.65s KILLED / 53.95s, kill=8)、`s8c-bind` (55.78s, kill=7)、`install-ape` (55.61s, kill=7)、`module-units` (54.09s, kill=7)、`wt-gate-clean` (54.02s, kill=7)、`tsv-lint` (55.83s KILLED)、`cx-booter-ape` (`release`, 45.69s KILLED) 及 11 批 `commit-tier-timings.tsv` `UNKNOWN` 历史超时项。
- (2) **S2 `commit` 贴线批(部分代表)**: `check-register-layout-gate` (50.48s)、`devsvc-git-unstage` (`G29` 冷 48.575s, 较 G26 58s 降但仍 88%)、`lane-key-probes` (46.97s)、`dvqualify` (46.48s / 峰 53.15s)、`real-commit` (45.11s)、`derive` (45.08s)、`stream-pipe-parts` (44.16s)、`devsvc-freeze` (43.85s / 拆前 54.01s)、`selfcheck-item12-flip` (41.28s)、`premerge-selfcheck-rd` (40.42s)、`check-prerun-drive` (`G24` 冷 31.1s / 峰 53.13s)。
- (3) **热缓存与连带早杀掩盖 (S3, 112 批)**: `batch-times.tsv` 只记 `total_ms` 不区分 `UNISACC_RUN_CACHE=0`, 导致 `ctor-debt` (冷 53.11s vs 早杀 0.23s)、`object-source-replay-identity` (冷 53.11s vs 早杀 0.16s)、`chain-batchkeys` (冷 47.16s vs 0.10s)、`s9-shards-identity` (43.60s vs 0.19s)、`tools-compile-000..011` (峰值 46.9-57.5s kill=4-5 vs 最近 0.2-0.5s) 等 112 批在最近行呈现低耗时假象;另有 361 批 `batch-times.tsv` 最近为 `affected` 外窗连带 `KILLED` (`< 27.5s` 且无 >27.5s 峰值),未误列入超线但说明波窗排队有截断。
- (4) **拆分补丁核查 (`research/` + `tests/split-parents.tsv`)**:
  - 待合入拆批补丁 **1 组**: `module-units` -> `~/.local/share/unisacc/research/ompgpt/r35-module-units.patch` (另见 `r35-module-units-ac48.patch`),拆为 `module-units` / `module-units-forward` / `module-units-limits`,三片冷态实测 7.1-7.7s (可清 S1 头号 `daily` 阻塞)。
  - 已合拆但子片仍超 27.5s **3 组**: `premerge-selfcheck` (`premerge-selfcheck-main` 53.23s、`premerge-selfcheck-rd` 40.42s)、`devsvc-freeze` (`devsvc-freeze` 43.85s、`devsvc-freeze-owner` 24.17s/冷 53.15s、`devsvc-freeze-scope` 25.02s/冷 41.81s)、`devsvc-git-c` (`c1a` 冷 27.385s 压 27.5s 线)。
  - 其余 S1/S2/S3 批在 `research/` **均无现成拆分补丁** (`否`)。
- (5) **表外补充**: `~/.cache/unisacc/batch-times.tsv` 中 35 个未登记 ID 含 `layer-api-private-bump` (53.124s FAIL, `g35-private-bump-auto.log:4`);[build/phase-times.tsv](../../build/phase-times.tsv) 第 254 行 `near-limit-pcg-stale-lock-e2e` (38.07s) 属 `check-precommit-gates.cx` 子用例。

## 3. 建议动作(解冻后)

- (1) 优先合入 `ompgpt/r35-module-units.patch` 并复核三片冷态 `< 10s`;
- (2) 为 `S1` 其余 `commit` 超限批 (`precommit-gates-*`, `layer-api`, `devsvc-remain/core-b/c`, `devsvc-compile-context`, `premerge-selfcheck-main`, `push-replay-commit-push`, `batch-timing`) 立项拆批或预备原生校验器;
- (3) 对 `S5` 459 个零记录 `commit` 批以及未确认纯冷态的 `batch-times.tsv <= 27.5s` 批,分波用 `UNISACC_RUN_CACHE=0 UNISACC_UNIT_CACHE=0` 补一次冷态普查并扩展 `batch-times.tsv` 增加 `cold=0|1` 列。
