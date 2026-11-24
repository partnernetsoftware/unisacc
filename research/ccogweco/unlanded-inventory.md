# 未落库产物清单 v3(0.0.106 发布时点;2026-11-25 ccogweco)

规则:冻结期共享检出只收集成者提交;下列均未推 main;纯文档随下一实质提交或里程碑收尾带上。
本版按发布时点更新:车上已含六笔路径 B/B-t 事实,路径 C 草案作废待命,107 追加集按 cc 记十二份(与
agyneon 页现列十份有差额,落前以实页为准),第三车栈见第 4 节。

## 0 第一车车上已含(发布即随 origin/main 出)

- c698ef0091(R39 W0087 夹具修复,tools-only)
- d12d2087bd(R39/R36 W0148 check-ln-noderef 四处 ln 改 -sT,tools-only)
- a46b98d713(R40 B-t W0191 check-fwdstub-replay 用 sh 启动候选 APE,tests-only)
- 879e446640(R40 B-t W0189 刷新树内 build/freeze.tsv deadline 至 1795546800,tools/freeze)
- 8ca2f5e4b8(W0206 tests/env-semantics.tsv 追加四行,tests-only)
- 路径 B 事实(path-b-proof.md R40 B-t 六文件节):最终补跑头 8ca2f5e4b8,`PATH_B_DIFF PASS files=6
  frozen_paths=2 freeze_deadline=1795546800`,冻结路径内只有 G51 放行的 tests/check-fwdstub-replay.cx 与
  G52 放行的 tests/env-semantics.tsv;`PATH_B_SID PASS sid=c6eeca9f10d66039ce49f5f74ee1ec63727bd85e552f342965d8445136bd812d`;
  REASON 句见 path-b-proof.md。
- 不再列待落:cxm-maxu(9199f8dbbf 入 107)、module-units/cxmod-tri/F1/不动点 v4(107 分支持续)、c698/d12/a46/879/8ca(本车)。

## 1 P1:发布后立即落 main(文档、记忆宫殿、工具线小件)

### P1-a 记忆宫殿(同文件串行,先 refs-fix 再决定补丁)

| 项 | 内容 | 依据/证据 | 顺序 |
|---|---|---|---|
| mempalace-refs-fix.patch | agy-h 的失效引用修复 | 6ab5adc4… | 1 |
| mempalace-decisions.patch v2 | 16 条决定追加到 prd_mempalace.md 末尾(基线 a95623fb29,已私有 apply 与逐字节核) | sha 8b8915ba… | 2 |
| mempalace-decisions-verify.md | 补 v2 七条核对后随行 | — | 2 |
| prd:43 笔误 | "链止到 F,0.0.105 为 161 分钟"改"链止到 X 161.0 分钟" | research/agy-h/chain-to-public.tsv 0.0.105 行;R27:40 | 3 |

### P1-b 发布证据与说明(随 0.0.106 证据归档)

| 项 | 内容 | 前置 |
|---|---|---|
| archive-106-steps.md | 106 证据四项归档步骤(ape-receipt 12 行、fixedpoint-6targets 六行、INDEX 两行、notes 原句) | 106 Y1 前,交 agy-flow |
| fp-106-extraction.md | 六目标 g2==g3 与回执原行 | 随归档引用 |
| notes-106-disclosure.md | 链后补跑与已知缺口披露段(sha 3273ad30) | 发布说明定稿时并入 |
| path-b-proof.md 归档 | 复制到 release/evidence/0.0.106/path-b-proof.md | 发布时 |
| rerun-provenance-templates.md | 三型补跑 provenance 模板(sha 34cb4de3) | 补跑证据引用 |

### P1-c 研究页与后续基线(纯文档)

u30-capture-checklist.md(8e51799c)、e01-exec-steps.md(ad7ea466)、e01-closure.md(9e4efc21)、
u30-e01-dryread.md(774342ef)、precommit-gap-affected.md(b927ddb9)、precommit-run-tier-design.md(7a95ed82)、
after-seal-roadmap.md(ebea76fc)、notes-0.1.0-draft.md(ccabbea)、w0150-history.md(f6429cbe)、
w0191-history.md(b7ec6875)、env-semantics-acceptance.md(fc174cb8)、pathc-0030-decision.md(dfbb0ac8)、
path-c-rechain-plan.md(d2f552e5,备而不用)、unlanded-inventory.md(本页)、
notes-gaps-draft(394593ec)、seal map(81a868c0)、claimed-needs(29c58a6e)、unverified-classification(a61eaad5)、
p0-ledger(0737f4ff)、u33-callsite-audit(be858251)、u11-review(abae617f)、e02-closure(d77d49f7)、
u08-evidence(92042063)、usable-list-draft(1cdc029c)、r35-b1-assets(796cff2a)、doclinks-81(aa74529b)、
mempalace-refs-remaining(f834c779)、pgid-cleanup(91cdda83)、car2-next(affea030)、v0103 两页、wp6-wp8-plan。

### P1-d 工具线小件(不占车,落 main 需带 check 与登记)

- plans/v0.0.126.md 断链已由第一车修掉,doc-links 只剩"纳入棘轮/chain-ready"决定项(C2,107 后)。
- 其余工具线补丁(seal-precheck、list-cap 等)属 107 追加集,不在 P1。

## 2 Q1:107 冻结前(cc 记十二份;agyneon 页现列十份,落前按实页重取)

| # | 件 | 状态 |
|---|---|---|
| 1 | WP4 材料(wp4-material.patch v2,8958b5bf…) | 待合入 |
| 2 | WP4 install-ape v4(页 sha 为 v3 a2f6e863…) | 版本与 sha 待核 |
| 3 | 两个生产者(cx-booter-ape、old-checker;affected.cx:876 两方并集) | 待合入 |
| 4 | 选择器复用 v3 | 待终版 |
| 5 | list-cap v7(页 sha 为 v4 1f0b7459…) | 版本与 sha 待核 |
| 6 | U08 v3(32affee7…) | cdx 二审可入 |
| 7 | 夹具修复(ompgpt 页) | 待落盘 |
| 8 | seal-precheck(9162fe44…,chain-step 之前) | 待合入 |
| 9 | P0 chain-step(1271d5c8…) | 本段最后一笔 |
| 10 | 材料宏检查 v3(第一车合入后,前置 pinscan) | 待合入 |
| 11-12 | cc 记十二份中的差额两份 | 待 agyneon 页补明;不以本页为准 |

- 文档随同批提交:notes-gaps-draft、usable-list-draft、seal map、p0-ledger、本页、u30/e01 v2 两页。
- 版本与页 sha 不一致处(install-ape、list-cap、选择器)落前必须重取,不得直接 apply。

## 3 Q2:107 公开后(工具线、0.0.125、0.1.x 产品车)

- 材料栈 B1-B9(WP1-WP8、S5 先测量后优化)、C1-C5(list-cap 后续、doc-links 入棘轮、check-split-parents
  54947 ms 贴 55000 ms 限、提交钩子按层跑批 MVP、预期失败表与两个无生产者项)。
- 0.0.125:U33 与变异门 v2(mutation-gate-timeout-v2.patch 50f89249…)。
- 0.1.x 产品车:0.1.1 诊断(c15/c16/c17/-o 警告/diag-contract 基线、103 判据)、0.1.2 E02 键、
  0.1.3 链接层/CXM、0.1.4 B2;与 after-seal-roadmap.md 一致。
- 其他窗口补丁(WP1/WP3/WP6/WP7/WP8、s5-measure-tool、c15-c16-redo、mempalace-refs 余项、no-home-path、
  old-product-0101、release-branch-direct-r2、expfail-shared-v1):按各窗口交付信与 gatekeeper 记录再核。

## 4 第三车栈(封版后第一批装车)

- 第三车栈 = 0.1.x 系列:先 0.0.125(既有版槽,U33 与变异门),再 0.1.1 诊断车,随后 0.1.2/0.1.3/0.1.4;
  材料与工具线不占版本车,直接落 main。完整件表、依赖图、预计冷态与回滚见 research/ccogweco/after-seal-roadmap.md。
- 作废/待命:路径 C 草案(path-c-rechain-plan.md 的 v2 整链表)保留为回退;pathc-head-draft 与
  car1-pathc-head-plan 中"以 exec/facts 补登记换车头"的方案作废(改 tests 活表后不再需要)。

## 5 落库顺序

1. 发布完成 -> P1-a(refs-fix、决定补丁 v2、verify、prd:43)。
2. 随 0.0.106 证据归档:P1-b 五项。
3. 其余 P1-c 研究页随下一实质提交或里程碑带上。
4. 107 冻结前:Q1(按实页十二份,逐笔 `--keyed`);107 公开后:Q2。
5. 0.1.0 封版前:notes-0.1.0-draft 填数、seal-precheck 必落、usable-list 冻结。

注:其他窗口 179 份补丁只列今晚交付信与决定直接涉及的;其余落前按各窗口交付信与 gatekeeper 记录再核。
