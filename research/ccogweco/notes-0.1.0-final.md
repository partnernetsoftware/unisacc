# unisacc 0.1.0 发布说明(定稿骨架;2026-11-25 ccogweco)

状态:同一候选通过六格原生 CI,结论为 QUALIFIED(run 45541660048,status=completed、conclusion=success,6/6 候选目标全绿、42/0)。本版未执行全套 s9,未取得全套 s9 资格;同候选六格原生 CI 与全套 s9 是两件事,不得混称。
同候选六格原生 CI 与全套 s9 是两件事,不得混称。

## 真实身份

| 对象 | 身份 |
|---|---|
| 冻结提交(F5=起链提交) | a11d28fc0a61939ee71345d732a7dcf205b261f1 |
| 发布 APE SHA256 | b7cae17b1ae557a4907247e3168db56925504819f98d15f1b75a2da2de8fa97f |
| 源快照 SID | 37ff46cd02171c8ad2d3ba04dd084c296f5290a992077f8935e8b853e67d3238(与 107 相同) |
| CI run | 45541660048(QUALIFIED,6/6 全绿 42/0;ci-45541660048.json sha=220e3f3b…,log sha=e1248d7f…) |
| pkg.bin / slice-win-arm64 / 两个 build.json / SHA256SUMS | pkg.bin 4d13b970…;slice-win-arm64 44b59b8c…;unisacc.com.build.json 与 build.json 同为 e60aed84…;SHA256SUMS 122be4a2…(全值见 pack 候选表) |

回填:`sha256sum <pack>/layout/unisacc.com`;pack 的 `RELEASE_RUN SOURCES_SID VERIFIED` 原行;
`gh run list` 按 headSha;其余资产按候选表 sha256 列。

## 零改动封存口径(三条硬断言与路径 B-t 说明)

(1) 产品闭包零差异:`git diff --stat <V107_FREEZE> <V010_FREEZE> -- src cx include exec kernel seed weights
iterate/kernel/typekw.tsv` 与 21 条 tools 支撑文件清单无输出(链止后至发布通过路径 B-t 合入的 4 文件不触及产品闭包与该 21 路径清单)。
(2) SOURCES_SID 相等:两快照的 ident helper 结果逐字相同;链产物 `NEED source_sid` 与 pack 的
`SOURCES_SID VERIFIED` 同值。
(3) 未注版切片与 pkg.bin 相等:六目标 `fixedpoint/g2` 与 `p3.pkg` 逐字节相同;产品闭包、六切片、compiler.p4.pkg 与 `pkg.bin` 字节相同。候选 APE 由同一正式输入加不同版本槽经重封得到(版本槽 32 字节经压缩、PE 资源与 Mach-O 重签传播,裸文件字节差异远大于 32 字节),重封两次私有复现一致;`--version` 输出 0.1.0。

说明(agy-h 代并入): 0.1.0 不再是零 B 提交纯封存车(起链后合入路径 B-t 4 文件: `tools/check-split-parents.cx`、`tools/check-netregen-expected-fail.cx`、`tools/check-xunit-fnptr.cx` 三个 tools 校验文件与 `tests/env-semantics.tsv` 一个表), 但产品闭包、SOURCES_SID 与 21 条支撑路径零差异仍严格成立, 候选 APE (`b7cae17b1ae557a4907247e3168db56925504819f98d15f1b75a2da2de8fa97f`) 字节不变。
三条硬断言在链止后立即跑(1)(2), pack 前后各跑(3)对应部分, 并于 B-t 后复核 diff 只限上述 4 文件; 任一失败取消封版车, 按 R37 重排。

## 整链实测(0.1.0 第二轮封版链)

- T1=2026-11-25 08:59:43 +0800(1795568383);T2=09:26:30 +0800(1795569990,exit.tsv rc=0,why=exited),用时 26.8 分钟;state 文件 DONE 时刻 09:26:43。
- fixedpoint:`objects=same count=102`、`kernels=same count=2`、`slices=same count=6`、`product=same`、`package=present sha=c4120564b66c2dbc540644c98fce10ee0665cc996197de5b4c72aebdb15f8a24`。
- 六目标 g2/g3 逐对 sha 相同(前 16 位):lnx-x86_64 entry.elf 20f97a98248e28b5;lnx-arm64 entry.elf dab47fbe4848d459;osx-x86_64 602d66940c3e8771;osx-arm64 bc4069bbdfcea5f6;win-x86_64 8dab50897163c901;win-arm64 52fa4467270df0c9。
- APE5=sha256sum ape-final/unisacc-next.com = b7cae17b1ae557a4907247e3168db56925504819f98d15f1b75a2da2de8fa97f;ape-receipt.txt 与 p3-package.body 已在链目录。
- SID5 与 pkg.bin 已在 pack 后回填(见身份表与 pack 节),不再使用 107 的 SID。

## pack 与 warm(2026-11-25)

- candidate.tsv sha256=456cb99e5ca4499f37941bd0cdd6ec1abab7e17d05f50d3583fc8dcc483bc076;APE=b7cae17b…,SID=37ff46cd…。
- SHA256SUMS 五件:unisacc.com b7cae17b…、pkg.bin 4d13b970…、slice-win-arm64 44b59b8c…、unisacc.com.build.json 与 build.json e60aed84…;ape-receipt.txt sha256=cf3d1b57…。
- warm receipt:WARM RESULT READY key=3b9f8d7feea11300a15f08d69c14534a4cd3c9e4ea437f3c1c0d52daf21f8025 targets=5;S7 dry 全绿 DRY_GATES_OK steps=7。
- X 提交:X_SHA=336618e100c2ad5d32a8e7e12dded1f8fa6cc3d0(短号 336618e100,已推 origin/v0.1.0);candidate.tsv 七哈希同 pack 表。
- X 时间 2026-11-25 11:39:21 +0800;T2 到 X = 132.9 分钟,T_done(10:57:16)到 X = 42.1 分钟(45 分钟硬线按 T2 起算仍超,如实记)。
- Step 2 draft:2026-11-25 11:40:53 +0800 建草稿(target=a11d28fc0a),回读通过 `GATE 2 DRAFT_READBACK PASS five assets match downloaded SHA256SUMS`;Step 3 CI 于 11:41:01 +0800 起跑(`candidate-native.yml`,run 45541660048,headSha=336618e100,headBranch=v0.1.0),**已于本轮结单:run completed/conclusion success,6/6 全绿,QUALIFIED 42/0**;ci-45541660048.json sha256=220e3f3b1c5c5f34023462cd5560dd6e99635b3f66368b98fc77316ee210cd09,log sha256=e1248d7fbeb11bd48d5871b8c9960d6bd8ee6708affaf3b333b7d9e764947fc5。
- CI 耗时:2026-11-25T03:41:01Z -> 03:50:30Z = **9 分 29 秒(9.48 分钟)**,0.0.107 为 9.06 分钟(本车多约 0.42 分钟);六条 CANDIDATE_QUALIFY 原行:`CANDIDATE_QUALIFY\tverdict=QUALIFIED\tpass=42\tfail=0\tnot_run=0\tinvalid=0`。
- Step 3 四行回执:`GATE 3 CI_SUCCESS PASS run success, six candidate targets success`;`GATE 3 CI_VERDICT PASS verdict=QUALIFIED fail=0 invalid=0`;`INDEX exec-3-ci PASS 45541660048 six targets success, CANDIDATE_QUALIFY no fail/invalid`;`STEP 3 ci DONE`。

## 严格预跑 S5(0.1.0)

- 预跑终态:245 窗 / 521 批;T_prerun 09:37:31 -> 10:57:16(79.8 分钟;含 W0000 重起跨度 87.6 分钟)。
- strict:S5 计数 PASS=520、FAIL=0、UNKNOWN=1(strict 527 RAW;superseded=6);ratchets 19/0。
- 与 107 对比:107 为 241 窗/519 批/80.9 分钟;0.1.0 为 245 窗/521 批、79.8 分钟,窗多 4、批多 2,时长少约 1.1 分钟(以本轮实测为准)。

## 与 0.0.107 的差异

- tools/ 下发布形态修复与 `tools.cxlib` 包版本(0.4.12 之后的实际 bump);
- 版本槽:`release/candidate.tsv` 版本槽、APE 的版本槽重封(非裸 32 字节差异)、win-arm64 切片注入版本。
- 除上述外无 `src/cx/include/exec/kernel/seed/weights` 字节差异;出现即零改动断言失败。

## 准入七条

| # | 条件 | 本版状态 |
|---|---|---|
| 1 | 六目标不动点连续 3 版 | 106/107 已归档;0.1.0 链止后归档第三套(链上四同与六目标 g2/g3 逐对 sha 相同) |
| 2 | CI 六格 QUALIFIED 且 strict FAIL/UNKNOWN=0 | CI 按实际 run 回填;strict 以当轮 SUMMARY 为准,**UNKNOWN 仅限已登记且证据可核项:107 与 0.1.0 均为 `precommit-list-cap` 的 window-timeout**(本车 S5:520 PASS、0 FAIL、1 UNKNOWN、527 RAW、superseded=6、ratchets 19/0) |
| 3 | 零已知 P0 | P0 账本 5 行,F1 与 chain-step 已在 0.0.107 关闭;U33 为 P1,随 0.1.x |
| 4 | state-unverified 声称类 <=3 | **未满足**,预期 3 到 4 行,按 R38 口径如实记 |
| 5 | 前端覆盖三期 | **未满足**,按 R37 豁免;B2 顺延 0.1.2(原 0.0.109) |
| 6 | 起链到公开 <=120 分钟 | **未满足**;本轮 T1 08:59:43 到 T4 11:55:05 = **175.4 分钟**(T2 到 T4 = 148.6 分钟),链止到 X 亦超 45 分钟;原文与 R6 重述两口径并列,如实记 |
| 7 | 能用清单 30 过 25 | 清单已冻结、29 项就绪;唯一保留缺口是 release-chain 批本身,登记 0.1.x |

## 已知缺口

- (1) list-cap 设计性贴限:0.0.107 strict 1 UNKNOWN(`precommit-list-cap` 多次 55486/55492/55530/55557 ms);
  不放宽 55 秒、不缩覆盖,0.1.1 拆批。0.1.0 `W9922` 单跑实测 `UNKNOWN rc=0 ms=55587 transport=-1` (`expected-unmeasured`), 按同口径披露。
- (2) PASS 但余量不足披露:沿用 107 既有的 premerge-selfcheck-main、premerge-selfcheck-rd、
  slow-split(0.1.0 主预跑 `W0014 slow-split` 首试 `rc=124` 后次试批耗时 `50763 ms` 距 `53000 ms` 仅余 `2237 ms`, `W0015/W0016` 为 `23006/22513 ms` PASS; 披露沿用 `research/ompgpt/margin-disclosure-107.md`), 并增列本车 B-t 补跑两批: `W9988 check-split-parents` 实测 `49056 ms`(距 `53000 ms` 内层限算术余量 `3944 ms`, 不足 4 秒) 与 `W9989 check-netregen-expected-fail` 实测 `40738 ms`(距 `53000 ms` 余 `12262 ms`), 均纳入 `0.1.1` 拆批评估, 不放宽时限。
- (3) U05(provenance)未测;全套 s9 未跑;严格证据范围外 UNMEASURED。
- (4) 条件 4/5/6 未满足如实写;B2 顺延 0.1.2。
- (5) `env-semantics` 与 `xunit-fnptr` 跳号暴露(`research/agy-h/env-semantics-disclosure.md`, 0 产品功能缺陷): 预跑在 `W0212` (`env-semantics`, `tests/env-semantics.tsv:22-29` `expires=0.0.110 < 0.1.0`, `count=125/8`) 与 `W0217` (`xunit-fnptr`, `tools/check-xunit-fnptr.cx:97-98` 硬编码 `"unisacc 0.0."`, `assertions=4`, `rc=2`) 各触发一次红停; 经路径 B-t (`1a915a3955`) 修正并在同候选 APE 上单跑一次通过(`W9990 env-semantics PASS 12830 ms count=133/0 assertions=23`; `W9991 xunit-fnptr PASS 12641 ms assertions=15`)。

## 起车与预跑中发现并处置的缺陷(2026-11-25, agy-h 代并入)

- batch-domains 漏检:新批次/派生表更新后未被域索引选中;按当轮窗口页的记录处置,细节与命令原行以该页为准。
- worktree 形态:0.1.0 工作树启动件形态不合规(需正式副本);按 107 的同类教训处置,具体原行见当轮页。
- version-seq 读回按行序:读回取版本时依赖行序;按版本键取值,细节见当轮页。
- `W0000` 冷首窗两次 `rc=124` 与重起: 原将暖态 `4.2s` 批 `module-units-limits` 误作冷首窗 `W0000`, 在新 `frozen-root` 下两次 `58041ms/58040ms rc=124` 停机(`09:34`); 改 `W0000=module-units-forward` 并同布局预热后 `5297ms rc=0` 通过, `module-units-limits` 移至 `W0244` 后 `9589ms rc=0` 通过。
- `--keep-going` 漏传致 `W0022` 人工续跑: `09:37` 重起未带 `--keep-going`, 遇已登记贴限批 `precommit-list-cap` 两次 `rc=124` 硬停(`09:49`, `done=22/245`); 单跑 `strict-W9922` (`55587ms UNKNOWN`) 写 `done.W0022` 后带 `--keep-going --resume` 续跑至 `10:57:16 PRERUN_DONE 245/245`。
- 路径 B-t 四文件补丁(`1a915a3955fb3533e70164316c742b9bd03fddc3`, `11:16:31`: `W0088`/`W0111` 夹具缺 `version-seq`、`W0212` `env-semantics` 到期日、`W0217` `xunit-fnptr` 版本前缀): 合计三 `tools/` 文件(`check-split-parents.cx`, `check-netregen-expected-fail.cx`, `check-xunit-fnptr.cx`) 与一个 `tests/` 表(`tests/env-semantics.tsv:22-29` `0.0.110 -> 0.1.4`)，零触 `SOURCES_SID` 与候选 `APE` (`strict-W9923b-tsv-lint-root.tsv PASS 11733ms assertions=2 sha=d99db704`, 旧 `W9923` 记 `restored-from-report sha=58683c81`; 四补跑 `W9988 check-split-parents PASS 49056ms 19asserts sha=acc8b9e1eb7c` 距 53s 余 `3944ms`、`W9989 check-netregen-expected-fail PASS 40738ms 20asserts sha=9c0764cb78e6` 距 53s 余 `12262ms`、`W9990 env-semantics PASS 12830ms count=133/0 23asserts sha=97eb3624b317`、`W9991 xunit-fnptr PASS 12641ms 15asserts sha=8293480d5203`)。
- 以上各项不改动候选字节与 `SOURCES_SID`, 0.1.0 为含四文件 B-t 补丁的零产品差异封版车(非零 B 提交纯封存车), 细节以 `INDEX` 与 `research/agy-h/env-semantics-disclosure.md` 为准。

## 顺延与 0.1.x

0.1.1 诊断(108 原范围 + 103 判据 + list-cap 拆批);0.1.2 前端 B2(109 原范围);0.1.3 进程生命周期
(125 原范围 + U33);0.1.4 转发桩余项(126);工具线 T1-T11 见 research/ccogweco/roadmap-0.1.x.md。

## 占位与回填

| 占位 | 回填来源 |
|---|---|
| <CI_RUN_ID> | `gh run list` 按 headSha;失败/重派各留一行 |
| APE5 / SID5(已回填) | b7cae17b… / 37ff46cd02171c8a…;取自 pack 与 sha256sum |
| 公开时刻 | 2026-11-25 11:55:05 +0800(exec-5-publish,isDraft=false, Latest) |

其余值:Y1=7375d05e33ce6b3b296b375dfeb7de2b8ce56b9b、Y2=7dc318425d259087d25e0386a8239789144ce7f7(均见 INDEX);T3_R6 见 `CHAIN RESULT READY_TO_EXECUTE`;**T4=11:55:05 +0800,T1 到 T4 = 175.4 分钟,T2 到 T4 = 148.6 分钟**;strict 计数见 S5 节。

## 公开后更正(2026-11-25)

- CI 耗时原写 8.95 分钟,实为 9 分 29 秒(9.48 分钟);0.0.107 为 9.06 分钟,本车多约 0.42 分钟。
- 准入条件 2 的 UNKNOWN 描述原为模板句,实为 `precommit-list-cap` 的 window-timeout(107 与 0.1.0 同);不改变 1 UNKNOWN 计数与披露口径。
- 更正不触资产、SID、候选字节与 strict 计数。

## 落盘规则

正式 release/notes-0.1.0.md 只保留成文章节;三个占位未回填前不得过 Y1;不把未测写成通过;不夸大余量。
