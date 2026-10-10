# 0.0.38 裁后落地清单（2026-10-10；只整理，不代裁、不改验收措辞、不起全量重跑）

依据：一次裁（full038c 出口表，已落 `release/rulings.tsv` 53 行，c860b4b2）；二次裁请求（ver038 新增 12 项，`research/c38-ruling-request.md` 末节，已上报政委待裁）。证据包：仓外 `~/.unisacc/evidence/full038c-final`、`~/.unisacc/evidence/ver038-final`（各含 SHA256SUMS）。

## 1. 裁定到达后的落地动作（逐项，单写者 cc）

| 裁定结果 | 落点 | 消费方式 | 边界 |
|---|---|---|---|
| 某套件记 H1 云机基线 | `release/rulings.tsv` 追加一行：套件 / `TIMEOUT` 或 `FAILED` / 首错签名 / `H1` / 裁定出处；`plans/v0.0.38.md` H1 行具名 | exittable 归 RULED_BASELINE，不计 PASS | 只覆盖该套件、该形态、该签名；同套件新形态或新签名回 NEEDS_RULING；限时不变 |
| 某套件记 H2 宿主基线 | 同上（`H2`）；H2 行具名，并在 H2 切口给出宿主需求声明 | 同上 | 宿主不适用子项最终应由套件声明并退 77（P7），不以 skip 放过 |
| 允许 H2 切口修一轮 | 修片单笔提交（不动产品闭包，需动先报）；单项 bound≤55 真实 gate 复测 | 修通即原 rulings 行不再匹配（套件转 PASS）；修不动维持该行 | 只一轮；不重试到绿 |
| 覆盖缺口（COV1） | plans COV1 行；不进 rulings 宿主基线 | exittable 仍 NEEDS_RULING 直至补覆盖或保留具名拒收的裁定行 | 不改 PASS、不改判据 |
| 先定位再结（SC1） | plans SC1 行；定位结论写 prd | 定位前不入任何基线、也不挡收口 | 定位后按真实归因另行报裁 |
| lib-union16-callback（若裁“修套件退 142”） | H2/P7 切口：被外层限时截断时如实退 142 | 截断后由调度按规则延期/重试 | 修前不入基线 |
| union16 超时状态传递（获准后的下一刀） | 先用受控短时子进程验证：真实被外层限时截断→142、普通失败仍 rc1；再对具名受影响套件（lib-union16-callback-2/3）单项受限复测 | 不重跑 650 项；不把所有 rc1 改 142 | 获准前不改码 |
| 历史 solo 起跑提案 | 仅在单独裁定后实施 | 若裁“纯调度”：保留非独占 142 后 solo 重试；若裁“减少新鲜尝试”：另立契约改动并刷新身份 | 不得藏入 SCHEDULING 掩码 |

每批裁定落地后：一次 `make gatedeps`（P4 稳定点）、`exittable.py` 对 ver038 冻结 state 复核 NEEDS_RULING 归零或只余明确待定项、`ledgercheck --final`、push；不开新全量。

## 2. 可度量收口证据（保持，不重写结论）

- 前基线 full038c：649/649 rc1，四段，175 窗 / 8533.5 s，中途失效 39+124 项。
- 后验证 ver038：650/650 rc1，单段，153 窗 / 7301.0 s，中途失效 0；tail 延期 87 次 / 1595.7 作业秒、tail 成功 273 次 / 1926.2 作业秒。
- 口径：共同 suite stamp 全不同，非同身份对照，公共身份分量的逐项解释仍缺；墙钟差只作描述；成功 tail 273 次 / 1926.2 作业秒是作业秒、不是净墙钟收益；可确认的事实是中途失效消失、单段完成、tail 成功与延期可分账。23 项终 rc 变化已按尝试链定位（`research/c38-ruling-request.md`）。

## 3. 进入 Draft 的前提（全部满足前不进 Draft）

1. 两次裁定均已落地：出口表除明确“待定位不挡收口”（SC1）外无 NEEDS_RULING。
2. 产品身份未变：`provenance.source_digest()` 仍为 689a91de，且候选 740007ef 未重造；若 0.0.38 有产品闭包改动，则须按 §24/§27 重冻结、重造同源候选与定点，前述证据不可复用。
3. 一次完整 queue 在当前 main 以同候选真实出口，且其 RULED/H1/H2/UNKNOWN/77 均有出处、无未裁红（本条若需新全量运行，须单独授权）。
4. `ledgercheck --final` 0 未结、carry 0；`freezecheck`、`subtract-safety`、`gate-layers`、`script-inventory`、`gate-infra(-38)` 绿。
5. CI release-check 对封存提交绿；封存（seal_candidate.sh）与 rc 标签按 RELEASE-PIPELINE §9/§28 顺序；公开与签名另按政委/董秘授权。
6. 不改验收措辞、不抬限时、不删测试；所有 H1/H2 条目随版本说明列明为宿主基线而非通过。
