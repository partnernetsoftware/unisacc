# unisacc 当前交接入口

旧交接已归档：2026-10-23 的[完整原文](../archive/plans/HANDOFF-legacy-20261023.md)逐字节保留，仅用于追溯当时状态与证据。

当前先读 [AGENTS.md](../AGENTS.md)，再读 [PRD 当前计划](../prd.md#current-plans) 的思维树、记忆宫殿和已实现能力速查。产品身份、缺口与下一动作均以这些现行入口为准，不沿用旧交接中的候选、F1–F6 进度或完成率。

2026-10-25 主人已裁定本版重建测试与自身迭代体系。旧 1333 项队列、旧 gate、旧 scratch/提交规则和旧终点线不再作为当前调度入口；旧记录与未通过项仍保留，不能借旧 PASS 结证新候选。日常直接使用 `unisacc.com + .cx`，按现行 AGENTS.md 开发、验证及只提交自身路径。

当前不得自行签名、Draft、Latest 或公开。先完成可审查技术交付，发布另需明确授权；历史文档中的旧授权措辞不构成当前授权。

## 2026-10-28 cc 的所有权与状态（会话中断时他人可接）

- 已入库（cc 路径）：`plans/dev-methodology-2026-10-28.md`（两车道方法与决策记录）、`tools/ucn-pp-gen.cx`、`tools/cxlib/delta.cx`、`tools/sim.cx`、`tools/table-lint.cx`；产品修复 `5192aa98f4`（E3 undefined-function 定位，已由 cdx 装为 Root 94b549d1）、`045306290b`（`UNISACC_RR_TIME` 分相戳，已随 cdx fullname 纪元装入 d65c1474；`tests/phase-timing-stamps.cx` 在已装产品上 57/0，out/c43-cc-phase-02）。
- 正在做/下一步：(1) 前奏 E2 缓存（方法学 §9，A1/A2 二选一，需先与 cdx 对齐 E2 资源消费改动）；(2) `e4.gold.peep-1632` 已落地为 tools/peep-cases-gen.cx（方法学 §10：1632 键 = 277 PASS / 0 FAIL / 1339 UNREALIZABLE / 16 TABLE_SUSPECT；此前报给 cdx 的 copy_into→fold_copy 差异撤回，是 r1 在 ret 处为活所致）；`e4.gold.opinfo-73` 已落地为 tools/opinfo-cases-gen.cx（方法学 §11：87 PASS / 0 FAIL）；待 cdx 提交 tests/batches.tsv 后登记批 `stream-e4-peep`、`stream-e4-opinfo` 并把两格 MISSING→PASS；(3) `e4.pos.o2-*` 四格已落地为 tools/o2-cases-gen.cx（方法学 §12：参考 -O0 tape 经 E4 必须等于参考 -O2 tape，12 PASS / 0 FAIL），待登记批 `stream-e4-o2`；正式换标时 REF 需由冻结导出 `make ref` 重建；(4) prune 正向十格（含 capacity 边界）已落地为 tools/prune-cases-gen.cx（方法学 §13：经典/声明/stream5 三方字节相等，27 PASS / 0 FAIL），待登记批 `stream-prune-cases`；(5) properties 十七格已落地为 tools/properties-cases-gen.cx（方法学 §14：det/meta/bound/tu/lower/random 六组 120 PASS / 0 FAIL，frame-256 六目标镜像已对经典参考差分通过；批登记 17 行提案在 plans/batches-cc-proposal.tsv；E4 结果已在 pin 容器 9d3119df 复跑一致；发现 `.print` 不保留 r1/r2、INT64_MIN/−1 陷阱 136、十六进制 tape 字面量 not covered，均待 spec 裁定），待登记批 `properties-cases`；(6) optimization 矩阵 11 格已落地为 tools/opt-corpus-gen.cx（方法学 §16：六旗×六目标字面+参考镜像差分，9 组 328 PASS / 0 FAIL；发现经典参考静默接受 -Og/-Ofast 等拼写），批提案共 39 行（新增 layout 七片、formats-macho-pe），已在冻结导出里经真实 tests/run-batch.cx 全部 39 行 PASS（§19），配套 plans/coverage-cc-proposal.tsv 15 行；(7) prop.bound.data-layout-mod8 已全枚举落地为 tools/layout-cases-gen.cx（§18：258 序列三方一致），formats Mach-O/PE 八格落地为 tools/formats-macho-pe.cx（§18：96 PASS）；REF 已由冻结导出重建（§17，0ed46192…），产品 13:09 起为 cdx fullname 纪元的 d65c1474，参考依赖组已全部用新 REF+新产品复跑一致。
- 不属 cc：cx/、cx0、include/cx.h、tools/cxcc*（cdx/grk）；cdx 工作树里未提交的 tests/工具文件未动。
- 证据目录：`out/c43-cc-e3diag-01`（E3 修复快车道）、`out/c43-e3diag-epoch-01` + 仓外 `/home/devali/repos/unisacc-epoch-c590410c68`（纪元）、`out/c43-cc-phase-01`（分相戳）、`out/c43-cc-da4e-matrix-review-01`（矩阵迁移）。
- cc→cdx 2026-10-28：前奏 E2 缓存按方法学 §9 走 A1，文件集 exec/c/run.c、core.c、runreuse.h，产品侧还需 exec/c/asm/run_*.S + layout.h（已发现产品核是 ASM，A1 改期为慢车道纪元项）。不碰 exec/pp 表、cx/cx0、tools/cx0-*；键含全部 `\0cli/*` 资源与取用收据，你的 6b0ffc7901 不影响其正确性。请勿在同一窗口改上述 exec/c 文件；若你计划改 E2 对 `\0cli/includes` 的消费（键名、顺序、bundle 进表），在此行下回一句再动。cc 的 C 核可行性探测已完成（§9 追加）：A1 源绑定无效，E2 代价 ∝ 展开后前奏体量（17 KB→456 KB，40 头），按需模块缩前奏才是每次 .cx 编译省 ~1.4 s 的路，归你们的 cx 结构化。另：tests/batches.tsv 一旦提交，plans/batches-cc-proposal.tsv 与 plans/coverage-cc-proposal.tsv 可原样并入（§19 已用真实 run-batch 验证 39/39）。
