# unisacc 0.1.0 发布说明(定稿骨架;2026-11-25 ccogweco)

状态:同一候选通过六格原生 CI,结论为 QUALIFIED(run <CI_RUN_ID>)。本版未执行全套 s9,未取得全套 s9 资格;
同候选六格原生 CI 与全套 s9 是两件事,不得混称。

## 真实身份

| 对象 | 身份 |
|---|---|
| 冻结提交(F5=起链提交) | a11d28fc0a61939ee71345d732a7dcf205b261f1 |
| 发布 APE SHA256 | b7cae17b1ae557a4907247e3168db56925504819f98d15f1b75a2da2de8fa97f |
| 源快照 SID | <SID5_64HEX> |
| CI run | <CI_RUN_ID> |
| pkg.bin / slice-win-arm64 / 两个 build.json / SHA256SUMS | 按候选表与 layout 核(与 0.0.107 的未注版对象相同) |

回填:`sha256sum <pack>/layout/unisacc.com`;pack 的 `RELEASE_RUN SOURCES_SID VERIFIED` 原行;
`gh run list` 按 headSha;其余资产按候选表 sha256 列。

## 零改动封存口径(三条硬断言)

(1) 产品闭包零差异:`git diff --stat <V107_FREEZE> <V010_FREEZE> -- src cx include exec kernel seed weights
iterate/kernel/typekw.tsv` 与 tools 支撑文件清单无输出。
(2) SOURCES_SID 相等:两快照的 ident helper 结果逐字相同;链产物 `NEED source_sid` 与 pack 的
`SOURCES_SID VERIFIED` 同值。
(3) 未注版切片与 pkg.bin 相等:六目标 `fixedpoint/g2` 与 `p3.pkg` 逐字节相同;`pkg.bin` 与 0.0.107 相同;
(3) 未注版切片与 pkg.bin 相等:六目标 `fixedpoint/g2` 与 `p3.pkg` 逐字节相同;产品闭包、六切片、compiler.p4.pkg 与 `pkg.bin` 字节相同。候选 APE 由同一正式输入加不同版本槽经重封得到(版本槽 32 字节经压缩、PE 资源与 Mach-O 重签传播,裸文件字节差异远大于 32 字节),重封两次私有复现一致;`--version` 输出 0.1.0。
`--version` 输出 0.1.0。

三条在链止后立即跑(1)(2),pack 前后各跑(3)对应部分;任一失败取消封版车,按 R37 重排。

## 整链实测(0.1.0 第二轮封版链)

- T1=2026-11-25 08:59:43 +0800(1795568383);T2=09:26:30 +0800(1795569990,exit.tsv rc=0,why=exited),用时 26.8 分钟;state 文件 DONE 时刻 09:26:43。
- fixedpoint:`objects=same count=102`、`kernels=same count=2`、`slices=same count=6`、`product=same`、`package=present sha=c4120564b66c2dbc540644c98fce10ee0665cc996197de5b4c72aebdb15f8a24`。
- 六目标 g2/g3 逐对 sha 相同(前 16 位):lnx-x86_64 entry.elf 20f97a98248e28b5;lnx-arm64 entry.elf dab47fbe4848d459;osx-x86_64 602d66940c3e8771;osx-arm64 bc4069bbdfcea5f6;win-x86_64 8dab50897163c901;win-arm64 52fa4467270df0c9。
- APE5=sha256sum ape-final/unisacc-next.com = b7cae17b1ae557a4907247e3168db56925504819f98d15f1b75a2da2de8fa97f;ape-receipt.txt 与 p3-package.body 已在链目录。
- SID5 与 pkg.bin 待 pack 后填;不提前用 107 的 SID。

## 与 0.0.107 的差异

- tools/ 下发布形态修复与 `tools.cxlib` 包版本(0.4.12 之后的实际 bump);
- 版本槽:`release/candidate.tsv` 版本槽、APE 的版本槽重封(非裸 32 字节差异)、win-arm64 切片注入版本。
- 除上述外无 `src/cx/include/exec/kernel/seed/weights` 字节差异;出现即零改动断言失败。

## 准入七条

| # | 条件 | 本版状态 |
|---|---|---|
| 1 | 六目标不动点连续 3 版 | 106/107 已归档;0.1.0 链止后归档第三套(链上四同与六目标 g2/g3 逐对 sha 相同) |
| 2 | CI 六格 QUALIFIED 且 strict FAIL/UNKNOWN=0 | CI 按实际 run 回填;strict 以当轮 SUMMARY 为准,UNKNOWN 只允许已登记无生产者项 |
| 3 | 零已知 P0 | P0 账本 5 行,F1 与 chain-step 已在 0.0.107 关闭;U33 为 P1,随 0.1.x |
| 4 | state-unverified 声称类 <=3 | **未满足**,预期 3 到 4 行,按 R38 口径如实记 |
| 5 | 前端覆盖三期 | **未满足**,按 R37 豁免;B2 顺延 0.1.2(原 0.0.109) |
| 6 | 起链到公开 <=120 分钟 | **未满足**;参照 0.0.107 实测(链 26.4 分钟 + 预跑 80.9 分钟 + CI 9.1 分钟)与起车前预估,T1 到 T4 预计 125.4 到 139.4 分钟,链止到 X 亦超 45 分钟;两口径并列,如实写 |
| 7 | 能用清单 30 过 25 | 清单已冻结、29 项就绪;唯一保留缺口是 release-chain 批本身,登记 0.1.x |

## 已知缺口

- (1) list-cap 设计性贴限:0.0.107 strict 1 UNKNOWN(`precommit-list-cap` 多次 55486/55492/55530/55557 ms);
  不放宽 55 秒、不缩覆盖,0.1.1 拆批。0.1.0 若再现同类,按同口径披露。
- (2) 三批 PASS 但余量不足:premerge-selfcheck-main 50251 ms(余 2749)、premerge-selfcheck-rd 49898 ms(余 3102)、
  slow-split 51144 ms(余 1856),距 53 秒限不足 4 秒;披露沿用并引用 research/ompgpt/margin-disclosure-107.md。
- (3) U05(provenance)未测;全套 s9 未跑;严格证据范围外 UNMEASURED。
- (4) 条件 4/5/6 未满足如实写;B2 顺延 0.1.2。

## 起车中发现并处置的缺陷(2026-11-25)

- batch-domains 漏检:新批次/派生表更新后未被域索引选中;按当轮窗口页的记录处置,细节与命令原行以该页为准。
- worktree 形态:0.1.0 工作树启动件形态不合规(需正式副本);按 107 的同类教训处置,具体原行见当轮页。
- version-seq 读回按行序:读回取版本时依赖行序;按版本键取值,细节见当轮页。
- 以上三项若不改候选字节与 SID,只作流程记录;若影响身份,须在本节与证据 INDEX 同步披露(以当轮页结论为准)。

## 顺延与 0.1.x

0.1.1 诊断(108 原范围 + 103 判据 + list-cap 拆批);0.1.2 前端 B2(109 原范围);0.1.3 进程生命周期
(125 原范围 + U33);0.1.4 转发桩余项(126);工具线 T1-T11 见 research/ccogweco/roadmap-0.1.x.md。

## 占位与回填

| 占位 | 回填来源 |
|---|---|
| <CI_RUN_ID> | `gh run list` 按 headSha;失败/重派各留一行 |
| <APE5_SHA256> / <SID5_64HEX> | pack 输出与 `sha256sum <layout>/unisacc.com` |
| 公开时刻 | release/evidence/0.1.0/INDEX.tsv 的 `exec-5-publish` |

其余值(T1/T2/T3_R6/T4、Y1/Y2、strict 计数)在链止与各步回执后写入正文对应句,不预写。

## 落盘规则

正式 release/notes-0.1.0.md 只保留成文章节;三个占位未回填前不得过 Y1;不把未测写成通过;不夸大余量。
