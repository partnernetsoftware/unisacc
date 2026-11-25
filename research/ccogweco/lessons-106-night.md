# 0.0.106 之夜流程教训(草稿,待并入 skills/unisacc-orch-supervise;2026-11-25 ccogweco)

每条四段:现象、根因、固化动作、落点。只写草稿,技能文件的合并由 cc 做。

## (1) 发布第 6 步读回超时回滚

- 现象:0.0.106 首次公开 01:36:31 后,第 6 步读回 smoke 冷编超 45 秒失败,自动回滚 Draft,01:42:35 重公开,
  多耗 6.1 分钟,首次公开已对外。
- 根因:读回走的调用链在冷缓存下要现编工具;45 秒窗口对冷编不够;流程把"公开"放在读回之前,
  导致失败时已对外。
- 固化动作:公开前用与第 6 步完全相同的 argv 预热一次(warm 收据 `WARM ... READY key=`),
  并把预热写成公开动作前的硬门(107 的 #14 v2);回滚路径保留两次 INDEX 行。
- 落点:skills/unisacc-orch-supervise 发布流程节;runtime/release-exec 门;证据 release/evidence/0.0.106/INDEX.tsv
  与 research/ccogweco/post-106-ledger.md。

## (2) 门 5 余量 1.3 到 2.3 秒与预备写法两处错

- 现象:门 5 热复跑 50.773 秒,对 55 秒只剩约 4.2 秒(对 54 秒口径 1.3-2.3 秒);手册初稿把预备命令写成
  `OUT CACHE --prepare`,与现行源码不符。
- 根因:门 5 的贴限主因是准备相未命中(冷 58.002 秒 rc124 两次),不是某片构造慢;手册复制的旧写法
  未经源码核对。
- 固化动作:预备固定为同一 TMPDIR 两次 `--prepare ... tool` 与 `... generator`(各一次),要求出现
  `reused=1 reason=identity-hit`;门 22 的 stamp 只省预合树生成,prep 仍重查生成器,不计零。
- 落点:skills 发布/起链节;tools/check-candidate-chain.cx 用法;research/ccogweco/gate5-margin.md、
  pre-chain-crosscheck.md、cc-script-audit.md。

## (3) TLIB_BUMP_NEEDED 只报不拦

- 现象:预提交门对工具包内容漂移只打印 TLIB_BUMP_NEEDED 提示,提交照过;需人工去 bump。
- 根因:tlib-frozen 门是 report-only;内容摘要变了但版本未升,不阻断。
- 固化动作:0.1.x 的第 17 份把 tlib-frozen 升为拒绝(tlib-bump),并先清旧漂移、依赖 list-cap;
  在升级前,手册把"看到 TLIB_FAST bad=0 且版本号已升"列为起链前事实。
- 落点:skills 提交门节;research/cdx/car2-17-tlib-bump-gate.md;roadmap-0.1.x.md 的 T5。

## (4) 四车序列不成立与 fable 改法

- 现象:原排程按四车走 24 小时,但 0.0.106 单车 251 分钟,四车合计 9.7 小时,加公开间隔均值 6.8 小时,
  17:50 前只剩一车余量 7 小时以上。
- 根因:单车时长被红停、重试、等人与 CI 重派拉长;四车串行不现实。
- 固化动作:fable 裁定 107 后直接 0.1.0,108/109/125 顺延 0.1.x;条 6 字面不可能如实记未满足,
  条 5 按 R37 豁免;B2 顺延写未满足。
- 落点:prd_mempalace.md 决定块(research/ccogweco/decision-24h-fable.md);skills 排程节。

## (5) 窗口勘误流程

- 现象:cdx 读源码行发现手册里"门 5 预备写法"与"门 22 prep 算零"两处不符,随即把更正要求发给
  agy-flow、agy-h、ccogweco 三个窗口,各页按源码原行更正并回报 sha。
- 根因:跨窗口手册抄写缺少"以现行源码为准"的核对步。
- 固化动作:任何手册里的命令必须附源码行号,并在执行前由第二窗口按源码复核;发现不符,
  发出"已更正+sha"回执,禁止口头'已改'。
- 落点:skills 协作/勘误节;research/ccogweco/cc-script-audit.md 为样例。

## (6) 复用类证据被 fable 否决

- 现象:曾拟复用 107 的 CI 资格与 strict 表给 0.1.0 以压 120 分钟线。
- 根因:版本槽注入使 0.1.0 的 APE 字节与 107 必不相同,同候选前提不成立;复用会绕开条件 2 的独立证据。
- 固化动作:fable 裁否;0.1.0 必须自跑 CI 与 strict;压时间改用合规手段(前置并行、warm 预热、
  构造缓存、跨批次调度),不改时限。
- 落点:skills 发布证据节;research/ccogweco/seal-clock-margin.md 与 seal-pre-run.md 的"已裁否"节。

## 备注

- 本页只列流程教训,不重复产品缺陷(W0087/W0148/W0150/W0191/W0206 已在 notes-0.0.106-final 披露)。
- 落点中的技能文件合并由 cc 做;本页不直接改 skills/。
