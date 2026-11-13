# unisacc 0.0.59 -- 发布说明

状态:定稿于公开之前。本文件随提交 Y 入库,并作为发布页正文;因此本文件不能包含 Y 自身的哈希,也不能包含公开之后才发生的读回结果,这两项记在账本 C4-PUBLISH-RESULT。未实跑的项一律写未跑,未证实的项不写 PASS。

- 冻结提交 `fbdc99ca8921514fca927781a85ec93fbb5a82ec`(标签 v0.0.59-src2;父提交 v0.0.58-src `2a46e1a427`,加六个 P59R 文件与 release/candidate.tsv 的版本列;账本 C3-START5-059、C3-C-UNIT-COMPAT)。
- 提交 X:`7668b32d52b78d4180ddf3c09ee4ba2a37ebbf35`(父 b74bc339f8,只改 release/candidate.tsv 一个文件;同候选 CI 的被测提交)。
- 提交 X2:`dfb9b5102ebc978e47cb304f0f592562b460f18a`(父 X,只改 build/tlib-packages.tsv 两行:cx.compile 0.2.0->0.2.1、tools.cxlib 0.1.11->0.1.12 的冻结摘要;账本 C4-X2-RESULT)。
- tag v0.0.59 所指提交:冻结提交 `fbdc99ca8921514fca927781a85ec93fbb5a82ec`(工具写死 `--target build_rev`,由 GitHub 在公开时建在草稿 target 上),不是提交 Y,与 0.0.58 不同。
- 提交 Y:main 上的记录提交,父为 X2,主题为 `0.0.59 Y: ci_run_id=43379357079, release notes`,带本文件与 release/candidate.tsv、release/evidence.tsv、release/rollback.txt、release/decision.txt。Y 的哈希见账本 C4-PUBLISH-RESULT 与 `git log origin/main`。Y 不是 tag 所指。
- 候选 APE `unisacc.com`:sha256 `e43ef36832939c7f4132937564ce50c04e50e7910112f4ca2ac0e26df649ea44`,6856916 字节,`--version` 实测 `unisacc 0.0.59`。
- git 范围 `v0.0.58..Y`。

## 是什么

面向 C99 的编译器,自己写出 ELF、Mach-O、PE,可编译自身;产品形态为单文件 `unisacc.com`。0.0.59 的产品改动类别是「路线上行」(route upstream),且在本候选内缩小了范围,见下。上一发布版见 `plans/release-notes-0.0.58.md`。

## 行为变化(产品输入)

本候选包含:

(1) 链接路线经 cx.stream 管道(routepipe)。
(2) tl_dce:链接期丢弃不可达函数。
(3) fwdsig 共享。

本候选撤下,推迟到 0.0.60:

- 对象路线上行,即 run-forward 前段给单元对象加前推标记(fk_mark)。
- 撤下原因(账本 C3-GEN1-FIX、C3-C-UNIT-COMPAT):首个候选的 gen1 编译器拒编 cx/stream/item.cx,报 `not covered: library signature resource or duplicate definition`;修正后 C 单元调用别的单元里签名含按值结构体的函数仍被拒,0.0.58 同命令返回 0。取缩范围方案,只改 cx/compiler/routepipe.cx(c70f2c8002)。

行为后果:

- 分离编译、之后在链接期才由宿主前推的情形,与 0.0.58 一样仍被拒,典型报错 `link: undefined reference to pread`。本版不修复。
- 其余 stream 命令行为设计预期不变。实测依据只有两项:同候选原生 CI run 43379357079 的逐字判定 QUALIFIED pass=42 fail=0 not_run=0 invalid=0;候选专属批 route-equiv(候选对已装 0.0.58 基线)PASS rc=0 found 145/145。除此之外没有另做独立的全量字节等价检查。

## 随发布带出的工具与规则变化(非产品输入)

- 限值表 build/limits.tsv 与生成器 limits-gen;链运行身份不含限值。
- 宽度回升(C1):chain-run 降宽规则改为可回升。0.0.58 链诊断:264 次尝试,墙钟 7985 秒,同时只有 1 次尝试在跑的占 92.5%(账本 C1-PARALLEL)。本链(第 8 次)由 tools/phase-times-calc.cx 对 chain-run.tsv 实测:257 次尝试,墙钟 5124 秒,忙 5117.6 秒,其中同时 1 次 3337.9 秒(占忙时 65.2%)、2 次 818.4 秒、3 次 318.6 秒、4 次 642.6 秒,均值 1.66;按生效宽度分摊的尝试耗时:宽 4 下 6147.5 秒、宽 3 下 1279.1 秒、宽 2 下 659.2 秒、宽 1 下 415.3 秒;5 个 KILL 行,宽度 4->1->3,有回升。
- 表驱动与钉住:whole-front、preflight、release-build、release-ape 按输出目录钉住(CHAIN-PIN-REST);续跑与残留计数改为表驱动规则(RESUME-RULE,build/chain-residue.tsv);release-oneshot 的身份绑定。
- s9 重型域宽度 2,分片归属 v2;预提交加登记配对门;tlib 批拆分;check-reaper 的 nproc 上限修正。
- 链调用统一由 cxlib 的 chaincall 单元决定并校验(账本 C3-CALL-PLAN),起因是本轮前三次起链都停在两边不一致上。
- cursor-fleet 派工器(只是工具,不进产品)。
- tlib 包的预稳定策略:build/tlib-policy.tsv 中 stable 为 no,版本升级上限到 Z 位。std-surface v2 与 stable-eval v0 已有;没有任何包被宣布稳定。

## 整链与预演经过(如实)

本版共起链 8 次,前 7 次失败或被终止,第 8 次完成:

(1) 第 1 次:snap 相 preflight 规则钉了字面量 55000,与限值表改动冲突,链 0/498 停(修正 b9da262839,账本 C3-PREFLIGHT-FIX)。
(2) 第 2 次:分组相的段内限时被拒,10/498 停(修正 65b5ce2565,账本 C3-STOP-059)。
(3) 第 3 次:分组相段尾调用被拒,10/498 停(chaincall.cx,0223ddf096、9ffe37c948,账本 C3-STOP3-059)。其后先用真表干跑再起链(C3-DRY-059)。
(4) 第 4 次:到 p3(329/498)后,在 gen2 的 468/498 因产品缺陷停(g2-lnx-x86_64-closure first-failure,账本 C3-PRODUCT-STOP-059),即上文的 gen1 拒编问题。
(5) 第 5 次:起于 fbdc99ca8921;已记录到 p3(332/498,无 KILL、WIDEN、RESUME、BLOCKER)。该次停于 whole-front 外层 45 秒限(同第 6 次一类,见下)。
(6) 第 6 次:3/498 时被终止(账本 C3-STOP6-059,更正 C3-STOP6-CORRECTION):exit.tsv rc=143,实为协调者按真实 PGID 1310096 主动终止,原因是 candidate-chain.cx 多处 bound_sh_secs 用 cmd.front=45 秒外层限,gen1 前端子步骤并行负载下需 45 至 90 秒,预计在约 377/498 重蹈第 5 次。这是一次人工介入,且换了输出目录:失败目录移至 out/b1-old/fbdc-cmdfront-45s。
(7) 第 7 次:账本 C3-START7-059、C3-STOP7-059,同冻结提交,新输出目录。374/498 停,exit rc=2,BLOCKER phase=gen1-osx-x86_64-front0 reason=first-failure,stderr 为 `whole-front: REFUSE prepared input SHA mismatch`。根因:按 rev 前 10 位命名的共享快照里残留第 5 次链写下的 wf-prep 收据,被当作已完成复用,不核收据与本次输入是否一致;确定性工具缺陷,非瞬态、非产品。修正 bcd96c8f47(C3-WFPREP-REUSE-FIX)。失败目录移至 out/b1-old/fbdc-wfprep-reuse。
(8) 第 8 次:账本 C3-START8-059、C3-CHAIN8-DONE,同冻结提交,pgid 1937858。终态 state=DONE phases=498/498 last=pack rc=0;chain-run.tsv 无 RESUME、无 BLOCKER,5 个 KILL(均为可重试的相位界杀,不耗续跑);g2、g3 closure 的 rc=77 为已验证让出,非失败;日志出现两行 WF_PREP_STALE,即上条修正对残留收据生效(C3-WFPREP-STALE-OBSERVED)。

7 次失败的根因类别:链工具与限值表不一致 3 次(第 1 至 3 次);产品缺陷 1 次(第 4 次);链工具外层 45 秒限 1 次(第 5 次)及由此预判的人工终止 1 次(第 6 次);共享快照残留收据被复用 1 次(第 7 次)。

人工介入与换目录登记(退出条件 (3),只列账本可核者):第 1 至 7 次每次停后都有人工修复再起链(各次修复提交见上);第 6 次为人工主动终止;换输出目录账本可核的有 2 次(第 6 次到第 7 次、第 7 次到第 8 次),第 1 至 5 次之间是否换目录未逐次核对,不作断言;第 8 次链 BLOCKER 条数 0。「零接触」未达成。

链工具取自工作检出,冻结的是产品输入,不是链工具。

## 身份与证据

| 对象 | 结果 | 证据 |
|---|---|---|
| 整链(第 8 次) | state=DONE phases=498/498 last=pack rc=0,ape_sha=e43ef368...49ea44;promote --check OK(账本 C4-PREPARE-START 所记) | out/b1/fbdc99ca8921514fca927781a85ec93fbb5a82ec |
| 链墙钟与各相 | 墙钟 5124 秒;尝试合计 8501.2 秒(257 次),并发均值 1.66;各域合计秒数:target-tail 1706.0、p3 946.0、final-net 624.3、jobs-tbl-net-tool 481.1、kernels 122.5、pack 97.4、other 474.1(其余域见工具输出) | 同上 chain-run.tsv,tools/phase-times-calc.cx |
| kernels-g2、kernels-g3 | kernels-g2 80.0 秒,kernels-g3 42.5 秒;现行 kernels 相位时限 120 秒(临时放宽,见下),两者均在 90 秒内 | 同上 chain-run.tsv 的 ATTEMPT_TIME |
| 打包 | release-oneshot --prepare:第一次 rc=3(pack 子命令到 52.5 秒上限被杀,可重入),同参数重入 rc=0;PREPARED;source_sid d200be95...e5e52db4(账本 C4-PREPARE-RESULT 所记缩写) | out/c4/prepare-1.out、out/c4/prepare-2.out |
| 十四件资产(候选表七件加 `--prepare` 导出的六个独立切片与 standalone-sha256.tsv) | 草稿 id 443884706 上 14 件;assetguard REMOTE_COMPLETE;execute 的 draft-readback DONE rc=0;release-layout --check rc=0;摘要见下表 | out/c4/state/readback、out/c4/state/assetguard.tsv、out/c4/layout-check.out |
| 同候选原生 CI | run 43379357079,被测提交 X,绑定 ape_sha256 e43ef368...49ea44;制品 table.md 逐字 verdict: QUALIFIED pass=42 fail=0 not_run=0 invalid=0;tools/ci-verdict.cx --sha X 得 CI_VERDICT PASS rc=0 | https://github.com/partnernetsoftware/unisacc/actions/runs/43379357079;out/c4/state/ci-evidence.dir |
| s9 commit 档 | 单树一次的结论是 FAIL:b74bc339f8 树上 selected=256 status=FAIL failed_or_unknown=2 qualification=NOT_FULL;两条 FAIL 在后继树上逐批复验,属三棵树的证据拼合,详见下节 | out/c4/s9/evidence/summary-r2.txt、out/c4/s9x |
| release-decide | 工具内置(s9 档位为 external,步 4 记 UNKNOWN):QUALIFIED NOT_QUALIFIED unknown=1 fail=0;手工证据表步 4 填单树实况 FAIL 时:QUALIFIED NOT_QUALIFIED unknown=0 fail=1。公开经 s9 裁定文件 ALLOW_S9_GAP 放行,不是 release-decide 的 unknown=0 fail=0 | release/decision.txt、out/c4/manual-decide.out、out/c4/s9-gap-ruling.txt |
| 公开后读回 | 本文件定稿时尚未发生;结果记账本 C4-PUBLISH-RESULT | 账本 |

十四件资产 sha256:

| 资产 | sha256 |
|---|---|
| unisacc.com | e43ef36832939c7f4132937564ce50c04e50e7910112f4ca2ac0e26df649ea44 |
| pkg.bin | 56efea531e104b4114b314d5bdb4e5efc2a6debb96f974fa5d6e32675a637db8 |
| slice-win-arm64 | 99bffaebb9ae554eca1b1baabfe54ed791c2532db945f24b9cf405dce271a4ee |
| unisacc.com.build.json | 38e64b9c55c7a370d98ec36cb6ac9631e0fdc026be04c9c2afc9f09dbb3bfd55 |
| build.json | 38e64b9c55c7a370d98ec36cb6ac9631e0fdc026be04c9c2afc9f09dbb3bfd55 |
| unisacc-0.0.59-win-arm64.zip | 802a74dc83a10fa307564fb4c6ea4cb7af6cf6e8eea9a57483d6b8f5377547a5 |
| SHA256SUMS | fbbc04d7ecc14eddbf3822932542e75dd28ccf0ff8d13ae3280f08198ed2d280 |
| unisacc-lnx64-x86 | df00d3d8267ded5205f12c1f8b8413cc92fb707eb5e114306761042923908dcf |
| unisacc-lnx64-arm | a74371f0f6e54a746d8f8e34652fe552f57fcd0e3d22b4f0d0da779a3cbc2208 |
| unisacc-osx64-x86 | b7d9ec2dbed33644e8a047813b1760096ee0add542279670fb8ff7756a4ec9e2 |
| unisacc-osx64-arm | 6117f1c2af2b4c0bf28a95faf59eacbee4620f3e8f18702d05727e42988a89ff |
| unisacc-win64-x86.exe | dbb5a24c15e72362051973fdfde0a6cd710ab72ac346f25601e3dde8517ec6b9 |
| unisacc-win64-arm.exe | 1a0c3b1489a5c78b5fc6dd2bce23ac1a951afb45b191197adc57fa380e2d9d14 |
| standalone-sha256.tsv | ffc44fd2eddf3834e1a2805929c94e2f7ffdeca4d6d4bbfdce56bafe0b8c271f |

win 与 osx 的真实可执行体只由同候选 CI 判定,本机为 linux 宿主。QUALIFIED 一词在本文件中只引用同候选 CI 的逐字判定,不指 s9,也不指 release-decide。

## s9 证据的实际形态(三棵树拼合,不是单树 PASS)

- 树一,b74bc339f8(X 的父,候选 APE e43ef368):s9 提交档整档实跑,原样 `S9 COMMIT_ONLY tier=commit host_scope=lnx/x86_64 selected=256 status=FAIL failed_or_unknown=2 pending=0 qualification=NOT_FULL`;not_applicable=12,全为 osx/arm64 批。全程 2923 秒。首跑有 6 批因 watchdog-deadline 记 UNKNOWN(register-metadata-rollback、fixedpoint-sample、candidate-chain、chain-run、tlib-prep、prechain-gate),在封存重试片全 PASS,时限断言未改;该轮 load1 峰值 13.02,超时与负载的因果未证实。证据 out/c4/s9/evidence(账本 C5-S9-RESULT2)。
- 两条 FAIL:(1) productpkg-dir-identity,树内 release/candidate.tsv 版本列仍为 0.0.58 而候选输出 0.0.59;(2) tlib-frozen,cx.compile 与 tools.cxlib 两包冻结摘要与内容不符。均为树内输入问题。
- 树二,X(7668b32d52):productpkg-dir-identity 单批复验 PASS(out/c4/s9x/pp.out);tlib-frozen 单批仍 FAIL(out/c4/s9x/tf.out)。账本 S9-FAIL-DIAG。
- 树三,X2(dfb9b5102e):tlib-frozen 单批 PASS(out/c4/s9x/x2-tlib-frozen.out),另 layer-api、tlibpkg、tlib、tlib-kit、tlib-link、tlib-refuse 六批单跑 PASS。账本 C4-X2-RESULT。
- 没有在任何一棵树上得到过一次 failed_or_unknown=0 的整档 s9;X 与 X2 上未重跑整档。s9 步因此不记 PASS,公开依据裁定文件 out/c4/s9-gap-ruling.txt(decision ALLOW_S9_GAP)放行,裁定内容与本节一致。

## 已跑与未跑

已跑:

- 整链第 8 次 498/498(见上)。
- s9 提交档一次整档(结论 FAIL,见上节)及其后在 X、X2 上的单批复验。
- 候选专属五批,对链产出的候选 APE 实跑(账本 C4B-RESULT):cand-unit-compile PASS rc=0 23/23,23.7 秒;check-link-dce PASS rc=0 37/37,15.8 秒;check-route-fwd PASS rc=0 26/26,12.2 秒;route-equiv PASS rc=0 145/145,48.2 秒;xs-bound-width PASS rc=0 13/13,3.2 秒。首轮出现的偏差定因为检查工具缺陷(相对路径与标记名),修工具后重跑得上述结果,被测批次未改。证据 out/c4/cand-evidence。
- 同候选原生 CI(见上表)。

未跑或未验:

- s9 的 daily 档与 release 档未跑。
- 12 个 osx/arm64 批在 linux 宿主上 NOT_APPLICABLE,未跑。
- macos-signing-interface(osx 批)在 linux 宿主 RUN_REFUSED,未跑,UNVERIFIED。
- release-ape-replacements 未验,UNVERIFIED。
- 整档 s9 未在 X、X2 或 Y 上重跑。
- 发布树与 CI 被测树不是同一棵:CI 在 X 上跑;X2 比 X 多改一个非 release 文件 build/tlib-packages.tsv(两行),Y 再加发布记录文件。CI 未在 X2 与 Y 上跑(candidate-native.yml 不引用 tlib 表,此为读码结论,不是实跑结论)。发布资产与候选 APE 不受这些提交影响,其身份由 sha256 绑定。
- 公开后的本机根安装(install-ape)与清扫不在本次范围,未做。

## 临时放宽(如实)

- LIMIT-90-TEMP:链相位与批子进程时限 55 秒放宽到 90 秒,由项目所有者 2026-11-12 授权,临时覆盖「不放宽时限」铁律,尚未回收。
- LIMIT-120-KERNELS-TEMP:仅 kernels-* 相位 90 秒放宽到 120 秒,到期 2026-12-31;依据 0.0.58 中 kernels-g2、g3 实测 80 至 88 秒逼近 90 秒限。到期前须回收或重新裁定。

## 已知缺口(如实)

- s9:没有单树整档 PASS,见上节。
- DEBT-C-STATIC-ASSERT:前端对含 `__builtin_offsetof` 的 `_Static_assert` 与数组界报 `not covered: nonconstant bound`,未修。
- 18 个参考 C 夹具需要外部 -I 目录,不是回归,未处理。
- check-model-decode-real 自 C 参考归档(e16028ffa1)起损坏,已在 bf596b4484 修通(实跑 models=32 failed=0);但仍未登记批次。
- 链工具中仍未钉住的:entry-build、source-job-wave、net-deps。
- 对象路线上行与 C 单元按值结构体兼容推迟到 0.0.60。
- 文档整并不属于本版。
- 整版超 2 小时铁律:未达成。第 8 次链单次墙钟 5124 秒(约 85 分钟),在 7200 秒内;但同一冻结提交 fbdc99ca89 自第 5 次起链到可发布,经 4 次起链加 prepare、execute、s9 外跑 2923 秒与复验,整体超过 2 小时。按铁律视为设计需要改,不是赶工问题。
- 发布尾链工具缺陷在本次发布中现场修补(账本 C4-EXEC1-RESULT、C4-EXEC1-FIX-RESULT):assetguard 对草稿取 release id 的端点返回 404,execute 第一次因此停在 ci-dispatch;修补后重入到 decide。修补提交在共享检出本地,其中改了 tools/cxlib/releaseexec.cx,推送后 tools.cxlib 冻结摘要须再次 bump。
- 沿 0.0.58 未处理项:BASE6 字节 oracle、p3 release-ctor 材料、全域矩阵 release 档红格。

## 撤回与回滚

撤回公开:`gh release edit v0.0.59 --draft=true -R partnernetsoftware/unisacc`,并 `gh release edit v0.0.58 --latest -R partnernetsoftware/unisacc`;彻底撤回再删 tag:`git push origin :refs/tags/v0.0.59` 与 `git tag -d v0.0.59`,依运行手册。main 上的提交 Y 只用 `git revert` 撤回。本机根回退到 0.0.58:`sh ./unisacc.com tools/install-ape.cx -- ROOT <v0.0.58 公开 APE> 3744eacfa6b2472de8b18ef3d64ffad6f6a47f956d62e4a43760dee6e768632e --write`。候选表回退到 0.0.58 记录:提交 3700ef64a897050b2c592c0913daa80d8ed8a933(git show 该提交:release/candidate.tsv,已核其第 2 行为 v0.0.58)。

规则:发布页正文必须与本文件逐字相同,公开后读回核对(DEBT-RELEASE-BODY-READBACK);读回结果记账本 C4-PUBLISH-RESULT。
