# 工具清单（tools/*.cx 全量盘点，omp-ds 2026-11-02）

范围：`tools/` 顶层 351 个 `.cx`（53099 行）。明细见同目录 `tools-inventory-2026-11-02.tsv`（351 行 + 表头，11 列）。
只读盘点，**本清单不删除、不移动任何文件**；进批次的工具一律不动，归档/合并只作候选提交给人裁定。

## 列说明（TSV）

`tool`（文件名，去掉 `tools/`）｜`lines`｜`last_commit`（该文件最近提交日）｜`class`（7 类）｜`batches`（`tests/batches.tsv` 里以它为实现或入口的批次 id，逗号分隔，`-` 为不在批次）｜`code_refs`｜`code_date`｜`code_sample`（`.cx/.sh/.com` 中的引用数、最新引用日、样例）｜`doc_refs`｜`doc_date`｜`doc_sample`（`.md` 语料同理）。
统计口径：引用语料排除 `archive/`、`apps/`、`tools/compiler/`；`tests/cx*.tsv` 是棘轮量计表，不计入引用。

## 一、七类占比

| 类 | 文件 | 行 | 行占比 | 进批次 | 无任何引用 |
|---|---|---|---|---|---|
| 构造与发布 | 103 | 23854 | 44.9% | 11 | 11 |
| 测试执行与资格 | 164 | 18744 | 35.3% | 103 | 57 |
| 取证/材料/普查 | 41 | 5538 | 10.4% | 19 | 10 |
| 重构与提交 | 13 | 2217 | 4.2% | 0 | 1 |
| 审计与棘轮 | 11 | 1368 | 2.6% | 1 | 1 |
| 库消费者与样例 | 10 | 860 | 1.6% | 0 | 2 |
| 一次性遗留 | 8 | 490 | 0.9% | 1 | 1 |
| 未分类 | 1 | 28 | 0.1% | 0 | 1 |
| **合计** | **351** | **53099** | 100% | **135** | **84** |

读数三条：
1. 构造与发布一类占 45% 行数，只有 11 个进批次——这一片是「构造链」而非「检查」，判定依据是 receipt/产物身份，不是批次号。
2. 测试执行与资格 103/164 进批次，是当前批次网的主体；其 57 个无引用项多为被更高层批次间接调用或已退役。
3. 重构与提交、库消费者、一次性遗留三类合计 31 文件 / 3567 行，进批次 1 个，是可压缩空间。

## 二、引用分布

| 引用情况 | 文件 | 行 |
|---|---|---|
| 代码 0 + 文档 0（完全无引用） | 84 | 9659 |
| 仅文档引用（`code_refs==0, doc_refs>0`） | 106 | 12053 |
| 仅代码引用 | 51 | 3185 |
| 代码与文档均有 | 110 | 28202 |

「仅文档引用」不自动等于可删：`prd.md`、`plans/` 里的复现命令和回归台账都靠这些文件存在（如 `check-cx-layers.cx`、`check-cx-entry.cx`、`c-negative.cx`）。

## 三、完全无引用且不在批次（25 文件 / 1464 行）——归档候选

| 文件（行） | 说明 |
|---|---|
| `p3-member-full.cx`(421) | 一次性 P3 成员提取，结论已在 plans |
| `cxproc_ask.cx`(92) | 早期进程询问探针，被 `sy_*`/`np_run` 取代 |
| `precommit.cx`(68) | 旧提交流程，现用 `commit-own.cx` |
| `check-cx-sys-fs.cx`(67) | 已被 `check-cx-sys-*.cx` 新一代覆盖，未登记 |
| `source-job-wave.cx`(66) | 构造链旧入口，现走 `entry-build` |
| `output-file-cases.cx`(51) / `runtime-budget-cases.cx`(43) / `check-cx-unit-graph.cx`(33) / `check-cx-separate-use.cx`(32) / `pp-built-private.cx`(34) / `pp-fullname-gen.cx`(23) / `pp-fullname-pack.cx`(28) / `pp-private-install.cx`(34) | 同上：被后续实现取代的一次性脚本 |
| `driverdiag-/fifo-/tapelink-candidate-install.cx`(34×3) / `check-driver-target-long.cx`(28) | 见第四节合并候选 |
| `practice-slice.cx`(32) / `deadloop-guard.cx`(33) / `fsize-exec.cx`(14) / `check-core-execute-real.cx`(19) / `check-core-transition-real.cx`(19) / `check-cx-cross-units.cx`(29) / `check-cx-driver-target-unit.cx`(20) / `ucn-identity-tables.cx`(82) | 早期探针/一次性表提取 |

裁定建议：先归档 `p3-member-full.cx`、`cxproc_ask.cx`、`precommit.cx`、`source-job-wave.cx`（占该组 43% 行数），其余按引用复查后再动；归档动作必须与 `prd.md` 对应行同批提交。

## 四、合并候选

1. **5 组 `<name>-candidate-{check,install}.cx`**（`cxpack`/`driverdiag`/`fifo`/`link`/`tapelink`，各 34–39 行，均 2026-10-30 同日）：结构同构，可共用一个「候选检查/安装」驱动 + 数据行。仅 `cxpack`、`link` 一组有活引用。
2. **25 个 `check-cx-*.cx` 是 `*-gen.cx` 的产物**（约 1200 行），未进批次：应作为生成物落到 `out/` 或忽略目录，不与被检源同列。
3. `practice-{build,grow,grow-perf,vec}.cx` 只被 `plans/cx-layers.md` 与 `tests/cxlibc.tsv` 提及——若 `cx-layers.md` 已定稿，这组可随文档一起归档。
4. `tls13-get.cx`(244) 仅被 `plans/cx-l0-tls.md` 引用：属 TLS 设计物料，保留在 `tools/` 之外更合适。

## 五、不变量

- 135 个进批次的工具**一个都不动**：批次网是完成率与资格的载体，归档必须连带改 `tests/batches.tsv`、`prd.md` 能力速查与本文三处。
- `tests/cx*.tsv` 七个棘轮表继续按现口径计数，不受本清单影响。
- 本清单只作盘点与候选，删除/归档需单独提交，且每项附引用复查证据。
