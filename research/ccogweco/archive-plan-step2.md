# research 归档第二批(0.0.107 夜页)范围与引用清单(2026-11-25 ccogweco)

本页只出清单,不执行迁移。第二批在 P0 出口后做;`research/agy-h/research-archive-plan.md` 的第二步范围以本页回填
(agy-h 合并,cc 复核)。目标:`archive/research-2026-11-25/<窗口>/`,与第一批同规则:先改引、再迁、再重算 doc-links。

## 0 范围

- 迁:0.0.107 发布夜产生的窗口页(ccogweco/agy-h/agy-flow 中的 106/107 页),含相应补丁与记录。
- 不迁:0.1.0 起车前后的活页(notes-0.1.0-*、v010-*、loss/risk/incident 的 0.1.0 版本、chain-fill 等),留原地至 0.1.1 公开后。
- 不迁:prd_mempalace.md 点名且未改引的决定依据页;论文区与 lean/。

## 1 ccogweco 候选(24 页;引用数为七处范围内 `git grep -nF` 实测)

| 页 | 引用行数 | 处置 |
|---|---|---|
| notes-0.0.107-v2.md | 0 | 可迁;但公开说明留存建议留到 0.1.1 后 |
| notes-107-factcheck.md | 0 | 可迁 |
| w0150-history.md | 3 | 先改引 |
| w0191-history.md | 1 | 先改引 |
| post-106-ledger.md | 3 | 先改引 |
| post-106-landing-plan.md | 8 | 先改引 |
| handoff-2326.md | 0 | 可迁 |
| land106-dryrun.md / land106-batch2.md | 0 / 0 | 可迁 |
| loss-guards-107.md | 3 | 先改引 |
| incident-playbook-107.md | 1 | 先改引 |
| lessons-106-night.md / lessons-107-prewarm.md | 0 / 0 | 可迁 |
| lessons-w0201-newbatch-gate.md / lessons-w0000-priority.md | 0 / 0 | 可迁 |
| lessons-model-patch-gate.md | 1 | 先改引 |
| precommit-gap-affected.md | 3 | 先改引 |
| precommit-run-tier-design.md | 0 | 可迁(工具线设计,0.1.x 可能还要引用,建议留) |
| rerun-provenance-templates.md / env-semantics-acceptance.md / pathc-0030-decision.md | 0 | 可迁 |
| chain-day-drill.md / chain-day-roster.md / cc-script-audit.md | 0 / 1 / 0 | roster 先改引;三者属 107 值守记录,可迁 |

- 合计:24 页,其中 8 页有引用(21 行),16 页零引用。建议保留到 0.1.1 后的:notes-0.0.107-v2、precommit-run-tier-design。

## 2 待补清单(其他窗口,由其自行出)

- research/agy-h/:106/107 值守、审计、时间线与补丁页(50 页中约 38 页),由 agy-h 列。
- research/agy-flow/:car2 早期页(17 页中约 12 页),由 agy-flow 列。
- 0.0.107 相关 .patch(历史补丁)按第一批裁定并入迁移、不改内容。

## 3 改引与执行序

1. 对第 1 节引用行先改指 `archive/research-2026-11-25/ccogweco/<file>`(devsvc f write 带 --expect-sha),单独提交;
2. agy-h 合并范围后,用 archive-move 清单迁移;doc-links-gen --write 重算随迁移提交;
3. 迁后 check-archive-refs 必须 PASS(含 patch 历史路径;必要时用检查自身豁免机制,不改逻辑);
4. 回滚=archive-mv 逆操作 + git revert 改引提交。

## 4 本窗口自有草稿清理(全体第四条)

- research/ccogweco 下无 `.cx` 草稿副本(实测 `ls research/ccogweco/*.cx` 空);无过期私有目录需移走。
- 本窗口在 /tmp 的临时脚本(repls/winsum/covcross 等)不在仓内,按临时目录纪律由 cc 统一清。

## 5 备注

- 引用数按七处范围现查(2026-11-25);迁移前重查一次,防新增引用。
- 本页不替代 agy-h 的预案;请 agy-h 把第 1 节并入 plan 第二步,cc 复核后再排执行。
