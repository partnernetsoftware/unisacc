# unisacc 当前交接入口

旧交接已归档：2026-10-23 的[完整原文](../archive/plans/HANDOFF-legacy-20261023.md)逐字节保留，仅用于追溯当时状态与证据。

当前先读 [AGENTS.md](../AGENTS.md)，再读 [PRD 当前计划](../prd.md#current-plans) 的思维树、记忆宫殿和已实现能力速查。产品身份、缺口与下一动作均以这些现行入口为准，不沿用旧交接中的候选、F1–F6 进度或完成率。

2026-10-25 主人已裁定本版重建测试与自身迭代体系。旧 1333 项队列、旧 gate、旧 scratch/提交规则和旧终点线不再作为当前调度入口；旧记录与未通过项仍保留，不能借旧 PASS 结证新候选。日常直接使用 `unisacc.com + .cx`，按现行 AGENTS.md 开发、验证及只提交自身路径。

当前不得自行签名、Draft、Latest 或公开。先完成可审查技术交付，发布另需明确授权；历史文档中的旧授权措辞不构成当前授权。

## 2026-10-28 cc 的所有权与状态（会话中断时他人可接）

- 已入库（cc 路径）：`plans/dev-methodology-2026-10-28.md`（两车道方法与决策记录）、`tools/ucn-pp-gen.cx`、`tools/cxlib/delta.cx`、`tools/sim.cx`、`tools/table-lint.cx`；产品修复 `5192aa98f4`（E3 undefined-function 定位，已由 cdx 装为 Root 94b549d1）、`045306290b`（`UNISACC_RR_TIME` 分相戳，待下一纪元装入；`tests/phase-timing-stamps.cx` 已能读戳，已装产品上 54/0，shim 上 57/0）。
- 正在做/下一步：(1) 前奏 E2 缓存（方法学 §9，A1/A2 二选一，需先与 cdx 对齐 E2 资源消费改动）；(2) `e4.gold.peep-1632` 已落地为 tools/peep-cases-gen.cx（方法学 §10：1632 键 = 277 PASS / 0 FAIL / 1339 UNREALIZABLE / 16 TABLE_SUSPECT；此前报给 cdx 的 copy_into→fold_copy 差异撤回，是 r1 在 ret 处为活所致）；`e4.gold.opinfo-73` 已落地为 tools/opinfo-cases-gen.cx（方法学 §11：87 PASS / 0 FAIL）；待 cdx 提交 tests/batches.tsv 后登记批 `stream-e4-peep`、`stream-e4-opinfo` 并把两格 MISSING→PASS；(3) 其余 MISSING 格的事实生成用例（`e4.pos.o2-*` 需参考 O2 tape oracle，暂无机械路线）。
- 不属 cc：cx/、cx0、include/cx.h、tools/cxcc*（cdx/grk）；cdx 工作树里未提交的 tests/工具文件未动。
- 证据目录：`out/c43-cc-e3diag-01`（E3 修复快车道）、`out/c43-e3diag-epoch-01` + 仓外 `/home/devali/repos/unisacc-epoch-c590410c68`（纪元）、`out/c43-cc-phase-01`（分相戳）、`out/c43-cc-da4e-matrix-review-01`（矩阵迁移）。
