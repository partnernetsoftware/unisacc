# 107 冻结行预备:106 行处置与 0.0.107 新行(2026-11-25 ccogweco)

给 cc 的精确文本与命令。所有行号取当前 main 的 tools/ 源码;deadline 取 agyneon 建议的 05:30 +0800 =
**1795555800**。build/freeze.tsv 的写入只经 devsvc(共享路径,提交归 cc)。

## 0 结论

- 106 行**不能只等过期**:prepush 对过期行照样拒绝,并提示 FREEZE_EXPIRED,必须显式删行或就地改写
  (prepush.cx:10-12)。所以公开后由 cc 删 106 行(或改成注释),再在 107 起链前写 107 行。
- 107 行:第一列 sid 用 `0.0.107-FREEZE`;base 用 106 公开后 origin/main 的 40 位头;owner=cc;
  deadline=1795555800;paths 与 106 行逐字相同。
- 两行不能同时存在并同时 active:106 行未删时,105/106 的 owner 能推 main,107 链的 F 门会被 106 行挡住。

## 1 106 行处置(公开后,由 cc 执行)

现状(build/freeze.tsv 唯一数据行):

    0.0.106-FREEZE	f23c7b56fd5c526aa6559a70762e7aad8fc949d6	cc	1795546800	src cx ext exec include seed kernel tests release

删除(推荐,改成注释留痕,避免空文件与"删行后重写"的审计争议):

    printf '0.0.106-FREEZE\tf23c7b56fd5c526aa6559a70762e7aad8fc949d6\tcc\t1795546800\tsrc cx ext exec include seed kernel tests release\n' > /tmp/freeze-106-row.txt
    printf '# 0.0.106 已公开;冻结行于公开后删除,107 行在起链前写入\n' > /tmp/freeze-106-new.txt
    sh ./unisacc.com tools/devsvc.cx -- f edit build/freeze.tsv --old-file /tmp/freeze-106-row.txt --new-file /tmp/freeze-106-new.txt
    sh ./unisacc.com tools/commit-own.cx -- -m "解冻:0.0.106 公开,删除冻结行(留注释)" --whole build/freeze.tsv

期望:prepush 不再冻结(无 active 行);`git show --stat` 只有 build/freeze.tsv。

## 2 0.0.107 新行(107 起链前,由 cc 写)

先取 base(106 公开后的 origin/main 头):

    git -C .claude/worktrees/v0.0.107 fetch origin
    git -C .claude/worktrees/v0.0.107 rev-parse HEAD
    git -C .claude/worktrees/v0.0.107 merge-base --is-ancestor origin/main HEAD && echo MAIN-ANCESTOR

新行文本(BASE 换 40 位):

    0.0.107-FREEZE	<BASE_40_HEX>	cc	1795555800	src cx ext exec include seed kernel tests release

写入命令(把注释行替换成 107 行,保留注释亦可):

    printf '# 0.0.106 已公开;冻结行于公开后删除,107 行在起链前写入\n' > /tmp/freeze-note.txt
    printf '0.0.107-FREEZE\t<BASE_40_HEX>\tcc\t1795555800\tsrc cx ext exec include seed kernel tests release\n' > /tmp/freeze-107-row.txt
    sh ./unisacc.com tools/devsvc.cx -- f edit build/freeze.tsv --old-file /tmp/freeze-note.txt --new-file /tmp/freeze-107-row.txt
    sh ./unisacc.com tools/commit-own.cx -- -m "0.0.107: 写冻结行(base=<BASE>,owner=cc,deadline=1795555800)" --whole build/freeze.tsv

注意:该提交必须发生在"107 合 origin/main、编号文件冻结、起链"之前;提交后再有任何编号文件改动,
23-freeze-armed 会以 FREEZE_TLIB_AFTER_FREEZE 拒绝。

## 3 23-freeze-armed 判据(chain-ready.cx:1522-1638)

- 表行解析(:1569-1586):非注释行四段以上(sid、base、owner、deadline_epoch_s、paths);deadline 必须正整数;
  过期行跳过;active 行 owner 必须与 DEVSVC_OWNER 或 UNISACC_INTEGRATOR 大小写不敏感相等,否则
  FREEZE_OWNER_MISMATCH;无 active 行且无 PUSH-FREEZE 标记则 FREEZE_NOT_ARMED。
- 远端(:1600-1619):除 UNISACC_FREEZE_NO_FETCH=1 外先 `git fetch origin`;失败 FREEZE_ORIGIN_MAIN_UNKNOWN;
  `git rev-list --count HEAD..origin/main` 必须为 0,否则 FREEZE_BEHIND_MAIN。
- 编号文件(:1621-1626):build/tlib-packages.tsv、build/layer-api、build/iface-hash.tsv 不得有未提交改动,
  否则 FREEZE_TLIB_AFTER_FREEZE。
- 提交顺序(:1627-1638):freeze.tsv 已提交时,最后一次编号提交必须是冻结行提交本身或其祖先,
  否则 FREEZE_TLIB_AFTER_FREEZE;只有 PUSH-FREEZE 标记时按标记 mtime 比较提交时间。
- 期望 PASS 证据行(:1641):
      armed: freeze.tsv sid=0.0.107-FREEZE owner=cc deadline=1795555800; HEAD behind origin/main=0; numbering files unchanged after the freeze

## 4 prepush 对新行的读法(prepush.cx:1-20、:102-130)

- 任一条非注释行即冻结;未过期行 -> FROZEN;过期行不因过期而解冻,拒绝并提示 FREEZE_EXPIRED,
  要求续期或显式删行(:10-12,:116-121)。
- owner pass:所有 active 来源都带 owner,且 DEVSVC_OWNER 或 UNISACC_INTEGRATOR 与它相等,main 可推(:17,:167-172)。
- 表不可读/坏行:fail closed,任何人不能推(:114)。
- 所以 106 删除后到 107 行写入前,main 推送不受冻;107 行写入后,只有 cc 能推 main。

## 5 release-exec 的 F 门(release-exec.cx:934-966)

- release-exec 不读 freeze.tsv 文字,它按候选表 commit 判 F:HEAD 必须等于候选 commit(链 rev),
  否则 F_NOT_CHAIN_REV;只有 `--post-chain-merge REASON` 才放行并写 INDEX `exec-1-post-chain-merge`(WAIVED)。
- 与本行的关系:107 行 base 若等于 107 链 F,则 F 门天然通过;若 107 车头另有 tools-only 提交,
  只能用 post-chain-merge 豁免(与 106 同一形态),且 SOURCES_SID 必须不变。
- --dry 下不匹配打印 DEFERRED,不拒;实跑才拒。

## 6 失败处置

- FREEZE_NOT_ARMED:行未写、已过期或 owner 与 env 不符;按第 2 节写入并 `DEVSVC_OWNER=cc` 重跑 `--only 23`。
- FREEZE_BEHIND_MAIN:先 `git merge origin/main`,再重写 base 并重跑判据;不得跳过判据起链。
- FREEZE_TLIB_AFTER_FREEZE:把编号改动合进冻结提交或在冻结前完成;已冻结后取号要重写冻结行并重跑 23。
- FREEZE_EXPIRED(prepush):改 deadline 或显式删行;过期行不能靠 owner 绕过。
