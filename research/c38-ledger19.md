# 0.0.38 ledger 19 行一次汇总（2026-10-10；只汇总，不代裁、不改验收措辞、不起候选/重活）

依据：`python3 tests/ledgercheck.py --final plans/v0.0.38.md` → 19 行未结，carry 0。结语只认 已完成/顺延/跨版进行/砍掉。
P1–P7（流水线提效）已完成并不在 19 行内；本表与版本冻结分开：账裁清后再一次版本提交冻结。

## 表一 已完成证据（可直接落结语）

| 行 | 证据 | 建议结语 |
|---|---|---|
| — | 19 行中无整项完成证据。P1–P7 已结；ver038+具名补验已作流水线出口（§3 裁） | — |

## 表二 建议顺延（附去向，均不改原验收）

| 行 | 现状 | 建议去向 |
|---|---|---|
| K2a | 调度器 C 化未起；本版 Python 调度已加固 | 顺延 0.0.39（与 K1 同排） |
| K1 | 随 K2 排期 | 顺延 0.0.39 |
| F4″ | F4′ FAILED，冷编≤5 s 未达；流水线化为下一刀候选 | 顺延 0.0.39 |
| COV1 | fb12-31 明确拒收（wrong 0），已裁不挡收口 | 顺延 0.0.39：补覆盖或保留具名拒收 |
| SC1 | **当前：已定位、未修**（行内“待定位/静态未跑”为历史记录）；已裁不挡收口；修片草案 research/c38-sc1-fix.patch（未应用：改 seed 生成器，须授权并核冻结身份；修后三项仍须对独立参考逐字节对照）；请董秘一次裁“本版修”或“后版承接”；ver038 日志首错一致：`seed-gen inspect-parse2-{startup-control,unary-head,unary-part10}-graph` 均 `seed-gen: unknown constant binding`（seed/gen.c:211，静态定位（未跑）：fe90d6f8（10-09 typedef 跨单元隔离）在 control-result.tsv 的 startup-marker 行加入 `["constant","TDE"]`/`["constant","TDN"]`；E3/Python 侧从 exec/facts/k2-control.tsv 的 consts（TDE=12094627905536、TDN=8000000）绑定，seed/gen.c `parse2_startup_control` 传空 `bindings`。三项静态调用链均先经此函数（unary-part10 → inspect_parse2_printfallback_bodies_graph 内含 parse2_startup_control），最早失败节点静态同一；cdx 复现确认（current main e6a6afe2 真实 gate bound55，12 s rc1，未遇 rc2；私有副本仅加诊断、已删）：三条 C 路线均最先缺 TDN，TDN/TDE 皆 missing，无图产物；Python 经 control-manifest.tsv:3-4 bindmap→assemble._bindings 合入 k2-control consts；C parse2_startup_control 传空 bindings。独立参考三项均 rc0（startup 1388473 B sha 8418fa0d…、unary-head 3794697 B sha b17fcf7b…、unary-part10 6129169 B sha 9ed0d14a…），字节对照 N/A（C 无产物，不记不等/通过）；共同 assert 行不等于共同根因，三项义务分列。修法属 seed 生成器改动，按 SC1 裁定另报，不自行修） | 顺延 0.0.39：定位后另报裁 |
| W2、E57 | 需 Windows 双 ISA 真宿主 | 顺延至有真宿主的版本 |
| X3、L2、L1′、L1b′、A1、N1 | 仅部分证据，无整项冻结证据 | 顺延 0.0.39（各行原阻塞照录） |

## 表三 须政委裁

| 行 | 待裁点 | cc 建议 |
|---|---|---|
| H1、H2 | 宿主基线（已两次裁入具名套件）结语用“跨版进行”还是本版“已完成（记为基线、非通过）” | 跨版进行：基线随版本说明列明，修片在后续 H1/H2 切口 |
| K2b | 阶段范围失效需政委批准 | 未批则顺延 0.0.39 |
| K2c | 重叠清单合并需政委/董秘确认 | 顺延 0.0.39 |
| H37 | 提案未批（realpath feature macro，改正式源码=动产品闭包） | 若批本版实现则须入本版冻结输入；否则顺延 |
| （顺延）占位 | 0.0.37 未结项已逐行填入，占位行本身 | 砍掉（占位行无内容） |

冻结影响（限定）：本表若只落结语、不在本版实现，则 19 行均不改冻结输入。若本版实现：H37 改产品闭包；K2b/K2c 改失效/门禁契约；H1/H2 修片须核身份——均须先定范围再冻结。建议裁回明确每行“仅结语”或“本版实现”。结语落账后版本号提交一次冻结，再造同源候选与定点、出口（须单独授权）。
