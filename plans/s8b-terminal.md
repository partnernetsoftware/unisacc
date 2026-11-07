# s8b 中段（非终态）

会话 `/tmp/grk-s8b-c463/out/s8b` candidate `884adfef1815e87750c86c7036d3074552a66ce18dfa855588ab279c2aa99fcf` harness `8a134c18…` 2026-11-07T17:50 END n=58 后同会话续（跳 wt-gate/cx-resolver）。s8 PASS 不沿用。

## 计数

| 类 | 数 | 注 |
|---|---|---|
| PASS | 204 | 末次收据 |
| FAIL | 17 | 下表 |
| UNKNOWN | 6 | wave-39 bound 超时 INFRA 待复跑 |
| NOT-AVAIL | 162 | 按下表材料分组 |
| KNOWN-GAP | 20 | full-* |
| 未决 light | 41 | 续跑中 |
| resolver / wt-gate | 各1 | 抽干后单独串行，bound 数分钟 |

## NOT-AVAIL 按材料

能当场造=否（s8b 冻输入；PREPARED≠PASS）。

| n | 材料 | 能否现在生成 | 代表批 |
|---|---|---|---|
| 28 | streams | 否：s8b 图已有 streams 但这些批 materials_registered≠yes | interfaces-lnx-x86_64, e2-input-binding |
| 12 | ref | 否：P4/冻结参考 ELF，无当场造 | optimization-corpus-* |
| 10 | streams,ref | 否 | stream-e4-o2, properties-cases-* |
| 9 | ri-flow-runtime | 图有路径，未绑入这些批 | ri-flow-profile-* |
| 9 | objects | 图有 objects-routes | objects-link |
| 9 | kernel-route | 图有 | kernel-route-* |
| 8 | ri-cli | 增量 ROOT 另会话；s8b 不迁 | ri-material-* |
| 7 | ri-chain | 图有目录，批未 registered | ri-chain-* |
| 7 | hostcc | 图有 gcc 路径 | optimization-ccdiff |
| 6 | streams+stream-lower-sha | 否：sha 绑定 | stream-lower-* |
| 6 | ri-flow-runtime,ri-flow-data | 否：ri-flow-data 未图 | ri-flow-e3-* |
| 6 | p3-pack-ctx | 图有 | p3-library-* |
| 其余 <5 | pp-gold, nets, kernels, origin, baseline, adopted-package, identity-helper, e2model, p3-gen-* | 否：P53/baseline/adopted/foreign | 见 pending |

## FAIL 17（stdout 末行归类；无编译器真缺陷，不报 c46-4）

| 批 | 归类 | stdout/stderr 末 |
|---|---|---|
| pins | 记录滞后 | UNDECLARED_PIN ×7；主树补充 CHECK_PINS failed=0 |
| cx-root-identity | 记录滞后 | doc ae0b12 vs APE 884adfef |
| cx-kernel-context | 工具合同/cdx | matches-recorded-v3 GOT22308622… |
| illegal-action | INFRA 缺 .dev | 缺 side ELF |
| cx-manifest | 记录滞后 | RECEIPT_CONSISTENCY sha=ae0b12；FAIL manifest-sha-equals-ape-receipt |
| cx-toolsloc | 快照工具行 | 非产品 |
| commit-own | INFRA 无 git | `check-commit-own: REFUSE cannot prepare the clone` rc2 |
| affected-tool | 工具/快照 | `FAIL affected.users-reverse` CHECK_AFFECTED failed=1 |
| stale-tool | 工具/快照 | `FAIL stale.orphans-listed-referenced-not` failed=2 |
| snapshot-run-tool | INFRA | stdout 仅 BATCH_RUN rc=2 无探针行 |
| doc-links-tool | 记录滞后 | `FAIL doc-links.fix-moved-two-on-one-line` failed=2（44182 链） |
| cxraw-dock | INFRA 无 git | `dock-map: git ls-files failed` rc2 |
| cx-npfault | 工具/INFRA | `FAIL D: a batch that never times out is still PASS` checks=13 failed=1 |
| cx-reaper | 工具（sysx 回收） | CX_REAPER ok=6 failed=14；非编译器 |
| candidate-qualify | INFRA | 十行 PASS 后 rc=2 无 CHECK 汇总行 |
| host-cell | 记录滞后 | `host-cell: REFUSE no-argument and no-recorded-product-digest errno=2` |
| stages | KNOWN-GAP STAGE-REJECT | FAIL e4/prune/lower reject-bad{,.trunc,.rand} failed=9 known_gap=3 |
| UNKNOWN×6 | SESSION-INFRA 55s | wave-39 |

## 材料接线（图有路径但 registered=no）

`all-waves pending` 的 `materials_registered=yes` **不是** init 的 map 文件，而是 session 里 `MATERIAL` 块：`as_find_material`（`all-waves.cx:122`）。init 只把 map 拷到 `session/materials.tsv`。s7 曾对 14 名逐个 `all-waves material ROOT SESSION NAME`（测指纹、append MATERIAL）。s8b **未跑 material**，故 ri-flow-runtime/objects/kernel-route/ri-chain/hostcc/p3-pack-ctx 等虽在 map 仍 no。

s8c：init 后对 map 每一名执行 `tools/bound 55 sh ./unisacc.com tools/all-waves.cx -- material ROOT SESSION NAME`，再 heavy 波。不挡本版发布。

## 会话基础设施

s8 作废因 cdx ROOT 未跟踪 `illegal-action-work/`（测试写仓根）。s8b：`tests/illegal-action.cx` mkdtemp（c05aa85998 / 34f7e2db），harness 8a134c18。
