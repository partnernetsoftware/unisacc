# 预跑补跑 provenance 文字模板(三型;2026-11-24 ccogweco)

用途:预跑 strict 表出现 FAIL 后,补跑或修复后回填证据时,统一按本页三型写 strict 注释头、INDEX 行与发布说明一句,
并附"为什么不违反不为过线放宽"的说明。所有原行必须来自真实日志(路径与行号随行),未跑不写 PASS。

## 0 公共格式

strict 表的注释头(fail 行保留在原 strict 表,新证据写在同一文件顶部或同目录的补跑表,不覆盖原行):

    # RERUN-PROVENANCE<TAB>type=TYPE<TAB>batch=ID<TAB>original=FAIL<TAB>kept=1<TAB>window=WNNNN<TAB>
    #   source=FULL_SHA<TAB>ape_sha256=HEX64<TAB>sid=HEX64<TAB>reason=...<TAB>evidence=PATH
    #   rerun=<n>/<n> PASS<TAB>method=idle|after-fix<TAB>author=WINDOW<TAB>at=ISO8601

INDEX.tsv 五列(`item<TAB>time<TAB>verdict<TAB>path_or_ref<TAB>note`,与 release-exec.cx:1763 同格式):

    <type>-<batch>	<UTC 时间>	WAIVED|SUPERSEDED|PASS	<证据路径或提交>	<一句理由;原 FAIL 行保留>

发布说明一句:写清"原 FAIL 行仍在、补跑条件、结论",不写"重跑后全绿"这种笼统句。

## 1 W0087 型:车上 tools-only 修复后补跑

条件:主链头 F 之后加一个 tools-only 提交(如 R39 路径 B 的 c698ef0091),只动 tools/ 检查夹具,不动产品闭包;
冻结路径 diff 为零,链内 helper 重算的 SOURCES_SID 与链上下文逐字相等。

strict 注释头示例(原行取自 ~/.local/share/unisacc/research/cdx/path-b-proof-c698ef0091.md):

    # RERUN-PROVENANCE	type=W0087	batch=check-gen-all-drift	original=FAIL	kept=1	window=W0087
    #   source=2e8a19a342251910d53010f51002b14a4ca7f916	head=c698ef0091ea11b9bcf7f29e46d213ab69b7ba57
    #   sid=c6eeca9f10d66039ce49f5f74ee1ec63727bd85e552f342965d8445136bd812d
    #   reason=R39 路径 B:仅修 W0087 私有检查夹具;单文件差异、冻结路径 0、SID 与链头逐字一致
    #   evidence=release/evidence/0.0.106/path-b-proof.md	method=after-fix	author=cdx

证明原行(该页原文,打补丁前必须重跑核对):

    PATH_B_DIFF	PASS	chain=2e8a19a3...	head=c698ef0091...	files=1	frozen_paths=0
    PATH_B_INPUTS	PASS	file=tools/check-gen-all-check.cx	in_sid=0	support_diff=0
    PATH_B_SID	PASS	head=c698ef0091...	sid=c6eeca9f10d66039ce49f5f74ee1ec63727bd85e552f342965d8445136bd812d

INDEX 行(预期;时间由执行器填,见原页):

    exec-1-post-chain-merge	<UTC>	WAIVED	c698ef0091ea11b9bcf7f29e46d213ab69b7ba57	HEAD=c698ef0091... chain_rev=2e8a19a3... waived: R39 路径 B,仅修 W0087 私有检查夹具。单文件差异、冻结路径和 SOURCES_SID 已核。证据 path-b-proof.md

notes 一句:0.0.106 整链后仅在 `c698ef0091` 修正 W0087 的私有检查夹具,冻结路径零变化且重算 SOURCES_SID 与链头
`2e8a19a342` 逐字一致;发布闸带 `--post-chain-merge` 与上句为理由。

为什么不违反"不为过线放宽":原 FAIL 行留在 strict 表(kept=1),没有删断言、没有改断言文本、没有延长时限;
修复对象是检查夹具本身,不是被测产品;冻结路径 diff 为零且有 SID 重算原行;若补跑仍红,仍按 FAIL 记,不转 WAIVED。
依据:R39 与 R36 路径 B;证据:path-b-proof-c698ef0091.md。

## 2 W0150 型:同源码同 APE 的空闲复跑(一次性并发红)

条件:原 FAIL 出现在并发预跑窗口,同 source、同 ape_sha256;空闲单批复跑(至少 3 次)均 PASS;原 FAIL 行保留,
类型记 flaky 的一次性,不记"已修"。

strict 注释头示例(数据取自今晚预跑日志 ~/.cache/unisacc/prerun/v0.0.106-2e8a19a342/,当前仍 FAIL):

    # RERUN-PROVENANCE	type=W0150	batch=release-exec-c	original=FAIL	kept=1	window=W0150
    #   source=2e8a19a342251910d53010f51002b14a4ca7f916	ape_sha256=5f83ac93d201b252ea0b56098475be9e30045875cdf1ce692451cda095ceeb3c
    #   reason=原 FAIL 发生在并发窗口(jobs=1,wall_ms=25313/24242,批次 ms=22216);同源码同 APE 空闲复跑 3/3 PASS
    #   rerun=3/3 PASS	method=idle	author=WINDOW	at=ISO8601	evidence=<补跑日志路径>

原 FAIL 原行(保留,不改):

    PRERUN_KEEP_GOING	batch=release-exec-c	retries=2	window=W0150	raw=release-exec-c	tool-selfcheck	FAIL	1	5f83ac93...	22216	5.15	0	-

补跑段(三次,逐次一行,用 run-batch 原行格式;以下为模板,跑后填真值):

    RERUN	<n>/3	release-exec-c	PASS	rc=0	ms=<实测>	candidate=5f83ac93...	assertions=<实测>
    RERUN	<n>/3	release-exec-c	PASS	rc=0	ms=<实测>	candidate=5f83ac93...	assertions=<实测>
    RERUN	<n>/3	release-exec-c	PASS	rc=0	ms=<实测>	candidate=5f83ac93...	assertions=<实测>

INDEX 行:

    W0150-release-exec-c	<UTC>	SUPERSEDED	<补跑日志路径>	原 FAIL 行保留;同源码同 APE 空闲复跑 3/3 PASS,按一次性并发红记

notes 一句:W0150 的 `release-exec-c` 原 FAIL 在并发预跑窗口一次性出现,同源码同候选 APE 空闲复跑 3 次全过;
原 FAIL 行保留,类型记 flaky,不改批次判据与时限。

为什么不违反"不为过线放宽":判据、时限、断言都没动;FAIL 行原样保留并公布;只有"同源码同 APE 下可复现性"被
补测,补测次数与每一行原样入表;若 3 次里任意一次红,本型不成立,仍记 FAIL 并转入定因,不允许只挑绿的一次。

## 3 W0148 型:ln 修复四处(三个 tools-only 文件,SID 不变)

条件:原 FAIL 批次 check-ln-noderef,修复为三类 tools-only 检查夹具的四个链接点;修复提交的父提交为前一笔修复
(c698ef0091),链头仍是 F;三文件 diff、冻结路径零差异、SID 重算一致(与 W0087 型同构,差别是文件数与修复点)。

strict 注释头示例(依据 ~/.local/share/unisacc/research/cdx/path-b-proof.md 的三文件增量):

    # RERUN-PROVENANCE	type=W0148	batch=check-ln-noderef	original=FAIL	kept=1	window=W0148
    #   source=2e8a19a342251910d53010f51002b14a4ca7f916	head=<第二笔修复提交>
    #   paths=tools/check-chain-ready-freeze.cx,tools/check-gen-all-check.cx,tools/check-prepush.cx
    #   sid=<重算 SID>	frozen_paths=0	reason=ln 修复四处,三个 tools-only 夹具文件,产品闭包未动
    #   evidence=release/evidence/0.0.106/path-b-proof.md	method=after-fix	author=<窗口>

证明原行(该页模板;打补丁前重跑):

    PATH_B_DIFF	PASS	chain=2e8a19a3...	head=<第二笔>	files=3	frozen_paths=0
    PATH_B_INPUT	PASS	file=tools/check-chain-ready-freeze.cx	in_sid=0
    PATH_B_INPUT	PASS	file=tools/check-gen-all-check.cx	in_sid=0
    PATH_B_INPUT	PASS	file=tools/check-prepush.cx	in_sid=0
    PATH_B_SID	PASS	head=<第二笔>	sid=<重算值>

原 FAIL 原行(今晚预跑,保留):

    PRERUN_KEEP_GOING	batch=check-ln-noderef	retries=2	window=W0148	raw=check-ln-noderef	tool-selfcheck	FAIL	1	5f83ac93...	3763	5.08	0	-

INDEX 行:

    exec-2-post-chain-merge	<UTC>	WAIVED	<第二笔提交>	HEAD=<第二笔> chain_rev=2e8a19a3... waived: W0148 ln 修复四处,三文件 tools-only,冻结路径 0、SID 一致。证据 path-b-proof.md

notes 一句:0.0.106 整链后在 `c698ef0091` 之上补一笔 ln 修复(三个 tools-only 夹具文件的四个链接点),冻结路径
零变化,重算 SOURCES_SID 与链头一致;原 W0148 FAIL 行保留。

为什么不违反"不为过线放宽":修复只碰检查夹具(三个文件),产品闭包与冻结路径为零;原 FAIL 保留;SID 重算原行入档;
若补跑仍红,仍记 FAIL。三文件列表与 diff 原行是这条的硬前提,缺一行不得写 WAIVED。

## 4 三种型的共同底线

1. 原 FAIL 行一律保留(kept=1),不改写、不删除、不登记为预期失败。
2. 不放宽任何时限或断言;补跑用同一候选 APE 与同一源码身份(除非本型明确是 after-fix,并附 diff/SID 证明)。
3. 每次补跑给原行;没有原行就不算证据。
4. 若补跑仍红,该型不成立,回 FAIL 并进入定因,不换说法重试。
