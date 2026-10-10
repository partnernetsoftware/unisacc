# 0.0.38/0.0.39 待裁汇总（单一裁定请求，WF4 实例；request 2026-10-10 13:08 +0800）

每项回一个选项即可；decision/applied 时刻回执后补。cc 不代裁，等待期间不重复问。

| # | 事项 | 选项 | cc 建议 | 依据 | 裁后动作 |
|---|---|---|---|---|---|
| R1 | gate-infra 归因（r38 pending） | A 认机房主任 12:31 具名裁定；B 记 NEEDS_RULING，指定审核人真实重审 compilercheck 闭包（自 5d9a549b 起只变 src/version.h 版本行与 prd.md 状态行） | B（一次真审后裁定，消除争议） | research/c38-freeze-receipt.md「gate-infra 待裁材料」；/tmp/c38-inbox-evidence-20261010T124818 | r38 pending 改 RESOLVED 并写 decision；publish 闸的 pending 校验不再因 R1 拒绝（R2 未裁前无 dmg 仍拒，不等于入口全过） |
| R2 | 无 Apple 资产版本的资产策略 | A publish.sh 按回执声明的资产集合要求（dmg 仅在回执列出时必需）；B 维持硬要求 dmg，无 Apple 版本不得公开 | A | 0.0.37/0.0.38 均无 dmg，硬要求逼旁路（0.0.38 绕开 publish.sh） | 改 publish.sh + 负例 |
| R3 | court 前置取件 | A Draft 固定签后工件上跑六 cell/Defender 作为 court，公开后另跑用户视角 smoke；B 维持公开后 smoke 为 court（接受“先公开后 court”） | A | release-smoke 只能下公开资产，与“court 先于公开”矛盾 | 改 release-smoke/defender-scan 支持 Draft 取件 + 负例 |
| R4 | seedmemorycheck Darwin 复跑 | m4pro FF 到含 0b831b64 后 `python3 tests/seedmemorycheck.py` | — | Linux 干净 checkout 13/13 已过 | 结果写 r38 unverified |
| R5 | 0.0.38 收口认定 | 认 research/c38-freeze-receipt.md + research/r38-release-acceptance.json 为最终可度量收口报告（R1 未决处照列；认定 R5 不等于裁清 R1 争议） | 认 | — | prd/计划标结 |
| R6 | 0.0.39 WF1/WF2/WF4 排期 | WF1 首个具名 probe 组（cdx 建议）；WF2 填尾受控验证；WF4 本表即首例 | 先 WF1 | plans/v0.0.39.md | 开刀 |
