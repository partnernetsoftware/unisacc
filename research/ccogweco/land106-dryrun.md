# 0.0.106 落库三笔 dry-run(2026-11-25 ccogweco)

基线:origin/main = 9d08f985fa 的 prd_mempalace.md(blob e2859e7,260 行)。全部操作只在 /tmp/land106/ 副本,
未写仓。仓内落库由 cc 在 release-post 与解冻信号后照第 4 节执行。

## 1 输入文件(/tmp/land106/)

| 文件 | 内容 | 来源 |
|---|---|---|
| prd_pristine.md | 9d08f985fa 原文 260 行 | `git show 9d08f985fa:prd_mempalace.md` |
| refs-fixed.md | 第 1 笔:agy-h 引用修复后的全文,260 行 | mempalace-refs-fix.patch 在副本上 `git apply` 通过(基线 blob 相同) |
| tail-old.txt | 第 2 笔的锚点:原文最后一行(509 字节) | 原文第 260 行 |
| tail-new.txt | 锚点行 + 22 条决定(16 条 v2 + 6 条 106 账本),23 行 | v2block 的 16 个 `- ` 行 + ledger-bullets.txt |
| l43-old.txt | 第 3 笔的锚点:原文第 43 行(1163 字节) | 原文第 43 行 |
| l43-new2.txt | 同长度替换:度量改为"链止到 X"(F 改 X,25 字节对 25 字节) | 由 replacer 生成 |
| v2bullets.txt / ledger-bullets.txt | 两组决定条目原文件 | mempalace-decisions.patch 与 post-106-ledger.md |

## 2 三笔在副本上的干跑结果

命令(私有 replacer /tmp/bisect/rep.cx,复制到 tree-b150 的 tools 下运行):

    rep prd_mempalace.md tail-old.txt tail-new.txt prd_step2.md      -> REPLACED old=509 new=12853
    rep prd_step2.md l43-old.txt l43-new2.txt prd_final.md           -> REPLACED old=1163 new=1163

`git diff --stat`(在 /tmp/land106 的临时 git 仓,基线为原文):

    prd_mempalace.md | 36 +++++++++++++++++++++++++++++-------
    1 file changed, 29 insertions(+), 7 deletions(-)

分解:引用修复 6 行改(6+/6-);追加 22 条决定(+22/-0,末行保留);第 43 行 1 行改(1+/1-)。

## 3 doc-links-gen 对副本的结果

在完整 106 树副本(/tmp/bisect/tree-b150aa37e2,git 仓)上换入 prd_final 后:

    # 未重算表:预期 STALE
    doc-links-gen --check --root <tree>
    DOC_LINKS_GEN rows=5146 inline=691 tick=4455 bare=0 broken=80 baseline=80 anchor_miss=1 orphans=112 result=FAIL

    # --write 后:OK
    doc-links-gen --write --root <tree>
    doc-links-gen --check --root <tree>
    DOC_LINKS_GEN rows=5146 inline=691 tick=4455 bare=0 broken=80 baseline=80 anchor_miss=1 orphans=112 result=OK

结论:三笔不改断链集合(broken=80 接基线 80),只让 build/doc-links.tsv 的 tick 行变化,必须随提交重算。
注意:纯 md-only 树(rst archive '*.md')会把对非 md 目标的链接误报为 broken(实测 broken=163),不要用它做判据。

## 4 可直接执行的串行命令清单(解冻信号后由 cc 执行)

    # 0 前置:origin/main 已含 9d08f985fa;工作树干净
    cd ~/repos/unisacc && git status --porcelain && git log -1 --oneline

    # 1 agy-h 引用修复
    sh ./unisacc.com tools/devsvc.cx -- f write prd_mempalace.md --from /tmp/land106/refs-fixed.md

    # 2 追加 16+6 条决定(锚点为原文最后一行,refs 修复不动该行)
    sh ./unisacc.com tools/devsvc.cx -- f edit prd_mempalace.md --old-file /tmp/land106/tail-old.txt --new-file /tmp/land106/tail-new.txt

    # 3 第 43 行口径订正(链止到 X)
    sh ./unisacc.com tools/devsvc.cx -- f edit prd_mempalace.md --old-file /tmp/land106/l43-old.txt --new-file /tmp/land106/l43-new2.txt

    # 4 重算 doc-links 生成表(必须与 prd 同提交)
    sh ./unisacc.com tools/doc-links-gen.cx -- --write
    sh ./unisacc.com tools/doc-links-gen.cx -- --check      # 期望 result=OK

    # 5 核对与提交(共享路径,cc 署名;只带这两个文件)
    git diff --stat -- prd_mempalace.md build/doc-links.tsv
    sh ./unisacc.com tools/commit-own.cx -- -m "记忆宫殿:106 账本与 16+6 条决定落库、161 分钟口径订正;doc-links 重算" --whole prd_mempalace.md build/doc-links.tsv

    # 6 推送(解冻后)
    sh ./unisacc.com tools/devsvc.cx -- git push "$(git rev-parse HEAD)" --cwd .

## 5 风险与回滚

- 若 refs 修复与 9d08 的现状不一致(blob 现为 e2859e7,与补丁基线相同,当前一致):先重出 refs-fixed,再重跑第 2/3 步。
- 若第 2 步 ENOMATCH:说明末行在信号到达前被别的提交改过;重新取末行做锚点,不整体覆盖。
- doc-links 若出现 NEW_BROKEN(非本改动的):停手,先定因,不得改基线。
- 回滚:git revert 该提交;doc-links.tsv 随 revert 复原。
