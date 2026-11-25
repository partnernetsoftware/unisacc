# doc-links 81 对 80 差 1 的核查(只读 + 私有树实测)

## (1) +1 来自哪里

- 同一构造的私有树(HEAD 的 git archive + git init,没有再放补丁)跑 `tools/doc-links-gen.cx -- --check`,结果 rows=5248 inline=692 tick=4556 broken=81 baseline=80 result=FAIL;把 mempalace 决定补丁应用后再跑,数字完全相同(rows=5248/692/4556/81)。=> +1 与决定补丁无关,也不是私有树多塞了文件。
- 真实主检出上同样 broken=81;`--check` 的具名行只有一条:
      NEW_BROKEN	plans/v0.0.126.md	plans/cx/compiler/fwdstub.cx:14-16
  根因:plans/v0.0.126.md 第 20 行把路径写成 Markdown 行内链接 `(cx/compiler/fwdstub.cx:14-16)`,该写法按 src 所在目录解析,变成 plans/cx/compiler/fwdstub.cx:14-16,不存在。这条随 0.0.126 计划正文引入,不是本次决定补丁。

## (2) 项目的 doc-links 口径

- tools/ratchets.cx 的 19 项(fsync、cxraw、cxinclude、cxapis、cxtrunc、cxlibc、cxplatform、srcshape、coreapi、verwriter、aluiwidth、outdep、check-literal、net-deps、notcovered、dup-census、no-home-path、ref-job-paths、tsv-lint、reserved-names)里没有 doc-links;第一车 19/19 全绿不覆盖这一项。
- doc-links 是 derive 的 pathset 行(tools/derive-inputs.tsv:20):`--check` 比的是 build/doc-links.tsv 生成表字节;而 tools/doc-links-gen.cx 自己的 `--check` 另外把断链与 tools/doc-links-baseline.tsv 做多重集比较(NEW_BROKEN 与 STALE_BASELINE 都算失败,基线只许缩)。当前真实树之所以红,就是上面那条 NEW_BROKEN,不是表漂移。

## (3) 修法与验证(私有树,未改共享检出)

- 把第 20 行的行内链接改成反引号路径:`fwdsrc[131072](cx/compiler/fwdstub.cx:14-16)` 改为 `fwdsrc[131072] 见 `cx/compiler/fwdstub.cx` 第 14 到 16 行`。
- 随后必须重算生成表:`tools/doc-links-gen.cx -- --write`,否则会报 STALE build/doc-links.tsv。
- 私有树实测(改行 + --write 后):
      DOC_LINKS_GEN	rows=5248	inline=691	tick=4557	bare=0	broken=80	baseline=80	anchor_miss=1	orphans=111	result=OK
- 建议:该修正由 0.0.126 计划的维护者在解冻后落到 plans/v0.0.126.md,并随同 --write 的 build/doc-links.tsv;不属于 mempalace 决定补丁的范围。

## 附:no-home-path

- 私有树 RATCHETS 的 no-home-path 仅剩 `archive/research-2026-11-25/omp-sonnet/handoff.md has 1 lines`(既有),决定补丁与本次修法都没有新增;RATCHETS fail=2 的另一项与本改动无关(dup-census 等既有)。

## 工具线缺口登记(cc 2026-11-24 决定)

- 缺口:doc-links 的 NEW_BROKEN/STALE_BASELINE 不在 tools/ratchets.cx 的 19 项棘轮里,也不在 chain-ready 的门内;本次 81 对 80 的差 1 是靠人发现才定因(plans/v0.0.126.md 行内链接)。建议把 tools/doc-links-gen.cx 的 NEW_BROKEN 检查纳入 chain-ready 或 tools/ratchets.cx,否则同类断链下次仍会靠人工发现。证据:本页上文、tools/ratchets.cx 19 项名单、research/ccogweco/mempalace-decisions-verify.md。回滚:删本行。
