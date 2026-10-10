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

## decision/applied
- R6：机房主任 13:12 SGT 代裁“先 WF1”（decision）；WF1 首刀 begin 13:1x，applied 见 plans/v0.0.39.md WF1 首刀。R1–R5 仍待政委裁。
- R4：已验证（消息入口回执：干净检出 af12c118，不设 UNISACC_FFI_X86_PROVIDER，seedmemorycheck 13/13 OK），按消息入口指示关闭；回执未注明该检出所在宿主。
- R7（机房主任 13:40 SGT 原文）：“WF1–WF5 首刀已合 tip（4d253833）并经 cdx/cdx2 复核闭合。R1/R2/R3/R5 仍待政委（~32min），勿空等 Goal paused。下一刀（不碰归因/验收措辞/删测/方向大改）：A) COV1 产品覆盖：fb12-31-unused-static-refs-undefined（-O0/-O1/-O2）补覆盖或保留具名拒收（difftest wrong 0）；plans/v0.0.39.md COV1；WF1 动态路已证明本路线暴露不了该拒收，产品义务仍在。B) 或 F4″……脏树 examples 删文件仍无人认领——勿代恢复；需要干净树再跑用 stash/干净检出。选 A 或 B 开刀并写回执；R 裁到了再接。” cc 选 A，2ac9a998（knownfail）→ 严格化为 tests/difftest.com.refuse（具名签名、三级正常拒收），knownfail 行按消息入口保留。
- R8（政委授权机房主任研判代裁，14:1x 原文）：“COV1：选「补覆盖」，不选「保留具名拒收当通过」。台账写明选择人=机房主任（政委授权自决）。覆盖完成前勿删 fb12-31 knownfail；refuse 不得计 PASS。按消息入口要求落台账可追溯 + REVIVED 证明测。”状态 DECIDED；applied 待补覆盖提交。REVIVED 证明：tests/namedrefusecheck.sh 的全一致情形判 REVIVED 且 FAIL；具名拒收计入 named、断言不计 agree。
- 政委授权机房主任代裁（2026-10-10T14:26:00+08:00）原文：“R1=B（gate-infra 记 NEEDS_RULING，真审后再裁定）；R2=A（无 dmg 不硬要，按回执资产集合）；R3=A（Draft 上跑六格+Defender 再公开）；R5=认（freeze-receipt+acceptance 为最终收口报告，R1 未决照列）。R4/R6/COV1 已定。请落台账并解闸推进 0.0.39。”另：commitgate 默认集选 B——默认只保证基建，产品改动须 --suite；src/ 与 unisacc.c 无产品套件则拒绝；勿改 gate.sh required。
- R1 结案（2026-10-10T14:48:47+08:00，机房主任代裁（政委授权真审后由其裁））原文要点：“gate-infra：基于 cdx2 独立内容重审 research/c39-r1-gate-infra-review.md，七套件命令/断言/预算未改；src 直接输入仅 version.h 版本行。记 RESOLVED：独立内容重审确认契约未改；版本变更改变字节与源身份，须同源重建；未本窗复跑七套件→UNKNOWN 照列。撤回/覆盖先前「仅刷哈希代重审」争议点；旧「具名裁定通过」与 R1=B 不一致处，GitHub release notes 改为与本裁定一致的短句。reviewed_trees/exec 9ae3c358：仅有原作者自审（2ca3d8d7），不得记已审、不回退哈希、不删其他 gatedeps；已派 cdx2 独立二审，通过前 stamp 维持「hash refresh 待真审」。R3：helper/负例≠court APPLIED；须真 Draft 上六格+Defender 才 APPLIED；下一刀版本冻结候选链→开 Draft→实跑 court，勿公开。”
- 机房主任代裁（unisacc10m）：exec 9ae3c358 据 cdx2 独立二审通过、记已审（不宣称六平台/对象 ABI 已证）；两 court 工作流 contents:write 仅限 release-smoke.yml、defender-scan.yml，先单 job 预检后扇出，Draft 不公开，R3 仍 DECIDED 至签后字节 court 绿。
