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

## FAIL 17

| 批 | 归类 | 补充证据 |
|---|---|---|
| pins | 记录滞后（七 carrier 针） | 主树 1d57fa479d 后 CHECK_PINS failed=0（非同会话） |
| cx-root-identity | 记录滞后（文档/APE） | 主树文档已 884adfef；主树已装产品仍 ae0b12 故单跑 failed=1 |
| cx-kernel-context | 工具合同，归 cdx | 不钉 GOT |
| illegal-action | INFRA 缺 .dev side ELF | cdx：缺 fixture 不算产品已测坏 opcode |
| cx-manifest | 记录滞后 receipt ae0b12 vs APE 884adfef | 同 root-identity |
| cx-toolsloc | 快照 44182 工具行 vs 后继主树 | 非产品 |
| commit-own / affected-tool / stale-tool / snapshot-run-tool / doc-links-tool | 待 INFRA/工具，stdout 未逐条钉 | s8b 冻 |
| cxraw-dock / cx-npfault / cx-reaper | 待 | |
| candidate-qualify / host-cell / stages | 待材料/资格格 | |
| UNKNOWN×6 | SESSION-INFRA 55s | skill-cx unit-boundary flow-tool ratchets-tool cx-core-interface cx-layers |

## 会话基础设施

s8 作废因 cdx ROOT 未跟踪 `illegal-action-work/`（测试写仓根）。s8b：`tests/illegal-action.cx` mkdtemp（c05aa85998 / 34f7e2db），harness 8a134c18。
