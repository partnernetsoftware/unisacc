# 落库第二批:研究页与记分表(纯文档里程碑收尾;2026-11-25 ccogweco)

现状:共享检出 HEAD = origin/main = 6eec0f4fdd(记忆宫殿三笔已落)。本批只列可提交的纯文档路径,
不提交、不 push。检查项:/home/ 绝对路径、Unicode 箭头与 emoji(grep -P 三个区间)、sha256 前 12。

## 0 候选盘面(git status:80 条)

- research/ccogweco 57、research/agy-h 17、research/2026-11-24 5、skills/unisacc-orch-supervise 1。
- 其中 8 个是 .patch(工具线补丁,随各自工具提交落,不入本批)。
- 最近 10 分钟有改动:research/agy-h/notes-0.0.107-skeleton.md(1 分钟,他人正在写,排除);
  本窗口自己的 land106-dryrun.md(3 分钟)、post-106-ledger.md(6 分钟)不属于他人,可入。
- 含 /home/ 或被 Unicode 符号污染而排除的:
  - archive/research-2026-11-25/2026-11-24/G53-review.txt(/home/=1,他人件,不入,待其作者改);
  - research/agy-h/evidence-path-style.md(/home/=1,新页,不入);
  - research/agy-h/wp7-material-a.patch(Unicode 箭头,属补丁,不入本批);
  - research/ccogweco/archive-106-steps.md、land106-dryrun.md、after-seal-roadmap.md、path-c-rechain-plan.md
    原先各有 /home/ 或箭头,已按规则改净(archive 的"/home/ 明文"改"家目录明文";dry-run 的 cd 路径改 ~;
    两处箭头改"到"),现四文件 /home/=0、uni=0,纳入本批。
- skills/unisacc-orch-supervise/team-scoreboard.md 为 MM(暂存+工作区改),取工作区内容;属本批。

## 1 本批入选 33 条(≤40),全部 /home/=0、uni=0

research/ccogweco(28):after-seal-roadmap.md、archive-106-steps.md、doclinks-81-investigation.md、
e01-closure.md、e01-exec-steps.md、e02-closure.md、file-macro-scan.md、fp-106-extraction.md、
notes-gaps-draft.md、notes-0.0.106-final.md、notes-0.1.0-draft.md、notes-106-disclosure.md、
path-c-rechain-plan.md、post-106-ledger.md、post-106-landing-plan.md、precommit-gap-affected.md、
precommit-run-tier-design.md、rerun-provenance-templates.md、seal-0.1.0-evidence-map.md、
u30-capture-checklist.md、u30-e01-dryread.md、u33-callsite-audit.md、unverified-classification.md、
unlanded-inventory.md、usable-list-draft.md、w0150-history.md、w0191-history.md、y2-root-backup.md;
research/2026-11-24(4):R37-seal.md、R38-ruling.md、R39-ruling.md、R40-ruling.md;
记分表(1):skills/unisacc-orch-supervise/team-scoreboard.md。

逐文件 sha256 前 12 与检查结果见 /tmp/land106/batch2-inv.tsv(80 行全表;本批 33 行可用
`grep -Ff /tmp/land106/selected.txt /tmp/land106/batch2-inv.tsv` 复核)。

## 2 本批排除(下一批/他人处理)

- 8 个 .patch(mempalace-decisions.patch、mutation-gate-static、wp6 x2、wp8、wp7-material-a 等):
  随对应工具/记忆宫殿提交落,不作为纯文档批。
- 其余研究页(car2-next、handoff-2326、mutation-gate-handoff、mempalace-decisions-verify、mempalace-refs-remaining、
  notes-107-factcheck、pgid-cleanup、release-day-107、r35-b1、unverified-extra-rows、v0103 两页、v0108-remaining、
  wp-write-domain-crosscheck、wp6-rest-notes、claimed-needs.tsv、p0-ledger.tsv 等):留待第三批或随实质提交;
  计数为不超 40 的取舍,不是质量降级。
- research/agy-h 其余 12 页与 tsV:属 agy-h 窗口,由其交付信决定批次;本批不代提交。
- build/links.tsv 的 M:派生表,不属纯文档批,随下一实质提交或单独 derive 提交。

## 3 doc-links 重算的副本干跑(前后 --check)

在完整 106 树副本(/tmp/bisect/tree-b150aa37e2,git 仓)放入上述 33 个文件后:

    # 放入前(基线树)
    DOC_LINKS_GEN rows=5146 inline=691 tick=4455 bare=0 broken=80 baseline=80 anchor_miss=1 orphans=112 result=OK
    # 放入后,未重算表:预期 STALE
    doc-links-gen --check --root <tree>
    DOC_LINKS_GEN rows=5145 inline=691 tick=4454 bare=0 broken=80 baseline=80 anchor_miss=1 orphans=112 result=FAIL
    # --write 后
    doc-links-gen --write --root <tree>; doc-links-gen --check --root <tree>
    DOC_LINKS_GEN rows=5145 inline=691 tick=4454 bare=0 broken=80 baseline=80 anchor_miss=1 orphans=112 result=OK

结论:本批不新增断链(broken=80 接基线 80);表需随提交重算(行数会因这批 md 的链接而变)。
说明:副本树是 106 树,行数与真实 main 的 5248 口径不同;真实提交前在共享检出重跑 --write/--check 以现树为准。

## 4 可直接执行的单条提交命令(解冻后由 cc 执行;不超 40)

    sh ./unisacc.com tools/commit-own.cx -- -m "0.0.106 里程碑收尾:研究页 33 件与记分表" --whole \
      research/ccogweco/after-seal-roadmap.md research/ccogweco/archive-106-steps.md \
      research/ccogweco/doclinks-81-investigation.md research/ccogweco/e01-closure.md \
      research/ccogweco/e01-exec-steps.md research/ccogweco/e02-closure.md \
      research/ccogweco/file-macro-scan.md research/ccogweco/fp-106-extraction.md \
      research/ccogweco/notes-gaps-draft.md research/ccogweco/notes-0.0.106-final.md \
      research/ccogweco/notes-0.1.0-draft.md research/ccogweco/notes-106-disclosure.md \
      research/ccogweco/path-c-rechain-plan.md research/ccogweco/post-106-ledger.md \
      research/ccogweco/post-106-landing-plan.md research/ccogweco/precommit-gap-affected.md \
      research/ccogweco/precommit-run-tier-design.md research/ccogweco/rerun-provenance-templates.md \
      research/ccogweco/seal-0.1.0-evidence-map.md research/ccogweco/u30-capture-checklist.md \
      research/ccogweco/u30-e01-dryread.md research/ccogweco/u33-callsite-audit.md \
      research/ccogweco/unverified-classification.md research/ccogweco/unlanded-inventory.md \
      research/ccogweco/usable-list-draft.md research/ccogweco/w0150-history.md \
      research/ccogweco/w0191-history.md research/ccogweco/y2-root-backup.md \
      archive/research-2026-11-25/2026-11-24/R37-seal.md archive/research-2026-11-25/2026-11-24/R38-ruling.md \
      archive/research-2026-11-25/2026-11-24/R39-ruling.md archive/research-2026-11-25/2026-11-24/R40-ruling.md \
      skills/unisacc-orch-supervise/team-scoreboard.md

提交后补:若本批 md 改变了 build/doc-links.tsv,须同提交带上或紧接一次
`tools/doc-links-gen.cx -- --write` 提交(见第 3 节);纯文档提交后不重跑棘轮。

## 5 风险

- 若 file-macro-scan/fp-106 等页在信号前被同作者再改,sha 会变;提交前重跑 /tmp/land106/batch2-inv.tsv 的
  grep 校验,只提交当时 /home/=0、uni=0 的版本。
- 他人页(G53-review、evidence-path-style)待其作者清家路径后再入下一批,不代为改写。
