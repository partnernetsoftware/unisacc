# 0.0.38 裁后落地清单（2026-10-10；只整理，不代裁、不改验收措辞、不起全量重跑）

状态（2026-10-10 更新）：一次裁已落地（full038c 出口表 53 行，c860b4b2）；二次裁已落地（ver038 9 行 H1，7d6220a0）；union16 两项已按授权修片，单项真实 gate 各 27 s rc0（7d6220a0，未入基线，cc 回执、非独立复现）；SC1 三项定位义务仍在；历史 solo 起跑提案另议未裁。原账 rc1 均保留。证据包：仓外 `~/.unisacc/evidence/full038c-final`、`~/.unisacc/evidence/ver038-final`（各含 SHA256SUMS）。

## 1. 裁定到达后的落地动作（逐项，单写者 cc）

| 裁定结果 | 落点 | 消费方式 | 边界 |
|---|---|---|---|
| 某套件记 H1 云机基线 | `release/rulings.tsv` 追加一行：套件 / `TIMEOUT` 或 `FAILED` / 首错签名 / `H1` / 裁定出处；`archive/plans/v0.0.38.md` H1 行具名 | exittable 归 RULED_BASELINE，不计 PASS | 只覆盖该套件、该形态、该签名；同套件新形态或新签名回 NEEDS_RULING；限时不变 |
| 某套件记 H2 宿主基线 | 同上（`H2`）；H2 行具名，并在 H2 切口给出宿主需求声明 | 同上 | 宿主不适用子项最终应由套件声明并退 77（P7），不以 skip 放过 |
| 允许 H2 切口修一轮 | 修片单笔提交（不动产品闭包，需动先报）；单项 bound≤55 真实 gate 复测 | 修通即原 rulings 行不再匹配（套件转 PASS）；修不动维持该行 | 只一轮；不重试到绿 |
| 覆盖缺口（COV1） | plans COV1 行；不进 rulings 宿主基线 | exittable 仍 NEEDS_RULING 直至补覆盖或保留具名拒收的裁定行 | 不改 PASS、不改判据 |
| 先定位再结（SC1） | plans SC1 行；定位结论写 prd | 定位前不入任何基线、也不挡收口 | 定位后按真实归因另行报裁 |
| lib-union16-callback（已裁并已修，7d6220a0） | 内部阶段超时如实退 142、普通失败 rc1；受控假 CC 挂起→142 / 失败→1 | 截断后由调度按规则延期/重试 | 未入基线；ver038 原账 rc1 保留 |
| 历史 solo 起跑提案 | 仅在单独裁定后实施 | 若裁“纯调度”：保留非独占 142 后 solo 重试；若裁“减少新鲜尝试”：另立契约改动并刷新身份 | 不得藏入 SCHEDULING 掩码 |

每批裁定落地后：一次 `make gatedeps`（P4 稳定点）、`exittable.py` 对 ver038 冻结 state 复核 NEEDS_RULING 归零或只余明确待定项、`ledgercheck --final`、push；不开新全量。

## 2. 可度量收口证据（保持，不重写结论）

- 前基线 full038c：649/649 rc1，四段，175 窗 / 8533.5 s，中途失效 39+124 项。
- 后验证 ver038：650/650 rc1，单段，153 窗 / 7301.0 s，中途失效 0；tail 延期 87 次 / 1595.7 作业秒、tail 成功 273 次 / 1926.2 作业秒。
- 口径：共同 suite stamp 全不同，非同身份对照，公共身份分量的逐项解释仍缺；墙钟差只作描述；成功 tail 273 次 / 1926.2 作业秒是作业秒、不是净墙钟收益；可确认的事实是中途失效消失、单段完成、tail 成功与延期可分账。23 项终 rc 变化已按尝试链定位（`research/c38-ruling-request.md`）。

## 3. 进入 Draft 的前提（全部满足前不进 Draft）

1. 两次裁定均已落地：出口表除明确“待定位不挡收口”（SC1）外无 NEEDS_RULING。
2. 产品身份未变：`provenance.source_digest()` 仍为 689a91de，且候选 740007ef 未重造；若 0.0.38 有产品闭包改动，则须按 §24/§27 重冻结、重造同源候选与定点，前述证据不可复用。
3. 一次完整 queue 在当前 main 以同候选真实出口，且其 RULED/H1/H2/UNKNOWN/77 均有出处、无未裁红。**口径待董秘明确**：既有 ver038（81c4827f）出口 + 其后具名补验（gate-infra-38 修 6b655220、union16 修 7d6220a0 单项 PASS）能否满足本条，抑或须当前 main 新一次完整运行（须单独授权）；不自行免验、不默认开新全量。
4. `ledgercheck --final` 0 未结、carry 0；`freezecheck`、`subtract-safety`、`gate-layers`、`script-inventory`、`gate-infra(-38)` 绿。
5. CI release-check 对封存提交绿；封存（seal_candidate.sh）与 rc 标签按 RELEASE-PIPELINE §9/§28 顺序；公开与签名另按政委/董秘授权。
6. 不改验收措辞、不抬限时、不删测试；所有 H1/H2 条目随版本说明列明为宿主基线而非通过。

## 4. §3 裁定落地与收口措辞（2026-10-10；实际跑过）

董秘代裁 §3 选 1：认 ver038（650/650 FINAL）+ 具名补验为完整出口，不开新完整全量。具名补验（current main，同候选 740007ef，真实 gate，各 rc0，cc 回执）：gate-infra-38；lib-union16-callback-1..5、lib-union16-direct-1..5（libraryunion16check.py 共用者）；publish-order、com-auditnet、lib-ffi-provider（变更闭包表其余现役门禁）。exec-container、ffi-bridge、lib-source-longdouble-import-rosetta 为 gatedeps 旧声明、非现役门禁，无可跑。变更闭包表：`research/c38-s3-change-closure.md`。

**收口措辞**：0.0.38 流水线提效可度量收口——前基线 full038c 与后验证 ver038 均为完整出口；同输入 468 项末次执行成本持平（−1.0%），整轮 175→153 窗、8533.5→7301.0 s、中途失效 163→0、四段→单段、tail 延期 175→87，与返工减少和调度改变一致、贡献未拆分、本机单样本；红项全部按两次裁定与 SC1 记账，产品验收措辞未改。

## 5. 进入 0.0.38 Draft 的剩余前提（报告，未执行）

1. `src/version.h` 仍为 0.0.37，候选 740007ef 即已公开的 v0.0.37 产物。0.0.38 发布须先做版本号提交——它改变产品闭包，按 §24/§27 与本清单 §3-2 须重新冻结、构造同源候选与 stage2=stage3 定点，并对新候选重做出口（本轮 ver038 证据只证流水线与套件，不能签给新二进制）。
2. `ledgercheck --final archive/plans/v0.0.38.md` 仍有 19 行未结（K2a–c、K1、H37、F4″、H1、H2、COV1、SC1、W2、E57、X3、L2、L1′、L1b′、A1、N1、顺延占位），须逐行结算或顺延。
3. 其余：freezecheck 待版本提交后进入冻结窗；subtract-safety 0；gate-layers/script-inventory/gate-infra 绿；CI release-check、封存与 rc 按 §9/§28；公开另由政委/董秘授权。

## 6. ledger 一次裁落地与 SC1 修片（2026-10-10；实际跑过）

- 董秘一次裁 ledger19 已落 archive/plans/v0.0.38.md（335b9312）：13 行 `顺延 N → 0.0.39`（计数加一，承接入草案 plans/v0.0.39.md，原验收不变）；W2/E57 顺延至有 Windows 双 ISA 真宿主的版本；H1/H2 跨版进行（基线非通过）；占位行砍掉；SC1 已完成。`ledgercheck --final archive/plans/v0.0.38.md` 0 未结、carry 0。
- SC1 修片 72120ea8：seed/gen.c `parse2_startup_control` 绑定 k2-control consts；seed-construct-parse2/-2/-3 真实 gate bound55 各 rc0（43/37/13 s），图对 Python 参考逐字节一致。
- **身份变化**：`provenance.source_digest()` 689a91de → c7e75006（seed/gen.c 在产品闭包内）；seed-gen 二进制随之变化，种子内存证据（绑定生成器 sha 2d1696da）须对新生成器重测，否则准入 rc2。候选 740007ef 与 ver038 出口不能签给新闭包——本就须随 0.0.38 版本冻结重造候选、定点、出口；仍等董秘授权，不进 Draft。
- 种子内存证据键（只读 tests/seedmemory.py 核对）：绑定 `C_SHA`=seed/gen.c 字节 sha（8ba73354…，72120ea8 后已变 → gen/parse2 族现判 rc2 'C source differs from measured route'）、`REFERENCE_KEYS`（exec/assemble.py、exec/build、exec/parse2、exec/facts 等，未变）、cc 实体 sha/版本/flags/并发/启动器。**不含 src/version.h**：版本号提交不使证据失效，故可在冻结授权后、版本提交前后任一时点对当前 gen.c 一次采证，无需二次重测；com 族本就 UNKNOWN（未测路线）。
