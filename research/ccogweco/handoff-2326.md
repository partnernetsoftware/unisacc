# ccogweco 交接要点(2026-11-24 23:26,压缩前)

## 1 已交产物(全部在 research/ccogweco/,均未推 main)

关键补丁(sha256):
- mempalace-decisions.patch 54c5ecb1a84a1e8cfe9bc98f6feef48224d686680f906d9fbbda8d0ed6e37d7e(基于 a95623fb29 的 prd_mempalace.md,只追加九条决定;解冻后落)
- wp8-slice-inventory.patch fdfb175b3f7865e3e272a418f00e75a2bacd6ada0ea59794035ddf16c5c9e7e2(0.0.108/0.1.x)
- wp6-rest.patch 3c8d890fe7f97daf862b17ced406c287dca6586510209d576057790ab998fb49(cx-npfault 原生复用)
- wp6-wt-gate-clean-b.patch 5c7eae900e5719c835528e77a4c223bd4d64f5e045db95682d26f5c1a9a9624f
- cxm-maxu-256.patch 6e12837f83a870933c21f8803b5e853b0f798352d4c722f27baa4be0a55b4349(已随 9199f8dbbf 入 v0.0.107 分支,不再单独落)
- 旧 mutation-gate 两份(65463fad…、1adfdaa9…)已判假绿,被 ompgpt 的 v2 取代,不落。

README/证据页(主要):
- unlanded-inventory.md(99 行;五组:本窗口 40 个文件、待落补丁、已落/不落、其他窗口今晚涉及补丁路径与 sha、落库顺序四步)
- file-macro-scan.md(材料闭包内 9 个 __FILE__ 命中;affected.cx:1203 注释与 devsvc-core.cx:538 检测字面是最高收益)
- u33-callsite-audit.md(发布链 P0:chain-step.cx:486-487 回收后再杀)
- p0-ledger.tsv(含 P0-chain-step)、unverified-classification.md(23 数据行、声称类 10)、claimed-needs.tsv、seal-0.1.0-evidence-map.md(条 4 三值 3/3-4/8)、notes-gaps-draft.md、release-day-107.md、u30-capture-checklist.md、e01-exec-steps.md、archive-106-steps.md、fp-106-extraction.md、u08-evidence.md、u11-review.md、e02-closure.md、v0108-remaining.md、v0103-*、wp6-wp8-plan.md、car2-next.md、doclinks-81-investigation.md、mempalace-refs-remaining.md、r35-b1-assets-fixedpoint.md、usable-list-draft.md(29/30 就绪)等,约 40 个文件。

## 2 状态与阻塞

- 冻结持续中,main 不提交不推;所有产物待解冻由 cc 一次性落。
- 106 证据四项:ape-receipt 与 fixedpoint-6targets 已备(agy-flow 暂存目录),INDEX 与 notes 待 agy-flow 按 archive-106-steps 补。
- E01 待 106 预跑完成低载窗口由 agy-h 执行;U08 待 agyneon 按 A 加负控;U30 待 107 发布时捕获五步原行。
- 条 4 预期 3 到 4 行,可能不满足,按 R38 如实记;E02 已知缺口不改 kind。
- plans/seal-precheck.md 与工具目前只在 cdx 的 seal-precheck.patch 里,未落库;107/封版 runbook 的第 3、4 节依赖它先落。

## 3 待办(压缩后)

1 下一件(cc 已派):只读核对 cdx 的 research/cdx/notes-0.0.107-draft.md 事实(提交号、补丁 sha、批次数、实测原行),出 research/ccogweco/notes-107-factcheck.md。
2 解冻后按 unlanded-inventory.md 的 E 组顺序落库。
3 余下机制:条 4 计数、E01/U08/U30 关闭证据回填、107 追加集补丁由各窗口落。
