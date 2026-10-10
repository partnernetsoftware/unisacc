# §3 决策用：ver038（81c4827f）之后的变更闭包表（2026-10-10；只用现存 diff 与回执，未跑 fingerprint，不进 Draft）

产品闭包：`provenance.source_digest()` 仍为 689a91de，候选 740007ef 不变；本段变更全部在测试/发布记录侧。

| 变更文件（81c4827f→HEAD，不含 research/plans/prd） | 声明为输入/guard 的套件数 | 其中已在本机具名补验（真实 gate） | 尚无补验（按契约须重验或待裁） |
|---|---|---|---|
| `release/rulings.tsv` | 3 | gate-infra, script-inventory | publish-order |
| `tests/gatedeps.json` | 8 | gate-infra, script-inventory | com-auditnet, exec-container, ffi-bridge, lib-ffi-provider, lib-source-longdouble-import-rosetta, publish-order |
| `tests/libraryunion16check.py` | 28 | gate-infra, lib-union16-callback-2, lib-union16-callback-3, script-inventory | com-auditnet, exec-container, ffi-bridge, lib-ffi-provider, lib-source-longdouble-import-rosetta, lib-union16-callback-1, lib-union16-callback-1-rosetta, lib-union16-callback-2-rosetta, lib-union16-callback-3-rosetta, lib-union16-callback-4, lib-union16-callback-4-rosetta, lib-union16-callback-5, lib-union16-callback-5-rosetta, lib-union16-direct-1, lib-union16-direct-1-rosetta, lib-union16-direct-2, lib-union16-direct-2-rosetta, lib-union16-direct-3, lib-union16-direct-3-rosetta, lib-union16-direct-4, lib-union16-direct-4-rosetta, lib-union16-direct-5, lib-union16-direct-5-rosetta, publish-order |
| `tests/queuecheck38.py` | 8 | gate-infra, script-inventory | com-auditnet, exec-container, ffi-bridge, lib-ffi-provider, lib-source-longdouble-import-rosetta, publish-order |

已具名补验（cc 回执，非独立复现）：gate-infra、gate-infra-38、script-inventory、gate-layers（各 rc0）；lib-union16-callback-2/3（真实 gate 各 27 s rc0，修片 7d6220a0）。

显式 UNKNOWN：队列契约（gatequeue/queue.sh 未变，但 queuecheck38 为其自检）、guard 回退、共享输入与 family 工具分量——未跑 fingerprint，不推断复用资格。

参照（非预测）：ver038 窗并集 7301 s（约 121.7 min）仅作资源量级；full→solo 2757.8 作业秒不是可省墙钟；历史 solo 起跑提案不在本表、未裁。

供董秘按 §3 二选一：(1) 以 ver038 出口 + 上表“已具名补验”+ 对“尚无补验”列逐项单项补验（不跑全量）认定满足；或 (2) 当前 main 新一次完整 queue（单独授权）。
