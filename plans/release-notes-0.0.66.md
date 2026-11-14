# unisacc 0.0.66 -- 发布说明

状态:已公开(GitHub Latest)。沿用 plans/release-notes-0.0.65.md 的口径:只写已入库事实;未实跑的项写未跑,未证实的项标 UNVERIFIED,不写 PASS;本文件不宣称 QUALIFIED。标"待填"的字段尚无事实,由发布流程填写,不得臆测。

- 冻结提交(REV):24bfc44c705f7253095aa2990c641533ae348b52;源标签 v0.0.66-src 指向 F:e96881d77d915e3600a39ac922e46886a1dfbf06(树取 REV,只差 release/candidate.tsv,父为 fbdc99ca 与 REV);候选表提交(X,仅改 release/candidate.tsv):9722945843bcae498f1cea27d7c7e5dbe92fbd63。整链所用私有提交:0f7d4c83fc。
- 候选 APE(unisacc.com)sha256:75c4e940af61d9956d9337c96d843ef0d37202b63c9b72e5446fa1cdbf047aed(7091252 字节)。pkg.bin sha256:c7cb3a8c3a6d88779bafc7aa0686ce6fe413c4477be502db4fbddf2cb092beae(与 0.0.65、0.0.63 的 pkg.bin 相同)。
- 整链 real9:rc=0,墙钟 3676 s,compare 通过,fixedpoint package sha 前缀 c7cb3a8c(出自整链日志与派工记载)。源身份 sources_sha256:aec51b74284201337b48d1b8482b96ef0eaa6da5114f36bbec321a69b0d4d57b(链内 identity-helper 对整链源快照实算;release-run --pack 对它的核对结果为 UNVERIFIED,因 F 无对应快照目录)。
- 本地打包(tools/release-run.cx --pack):步 4 至 9 全 DONE,结果 PACK_OK;tools/release-ape.cx smoke 对打包件 ok=5 failed=0(本机 lnx/x86_64)。
- 同候选 CI:candidate-native.yml run 43648734577,2026-11-14T22:33:41Z 触发,输入 commit 为 X、release_tag 为 v0.0.66;结果:completed success,10 个作业全 success(fetch、cell-tools、ffi-lnx、六个平台 candidate 作业、report),2026-11-14T22:36:53Z 结束。osx 与 win 的证据范围以该 run 作业为准,不扩大解读。
- release v0.0.66 资产 6 件:unisacc.com、pkg.bin、slice-win-arm64、unisacc.com.build.json、build.json、SHA256SUMS;无 zip。草稿上传后下载读回,5 件 sha256 与 SHA256SUMS 逐件一致,SHA256SUMS 与本地打包件逐字节相同(sha256 a6dfd03a55dfe5ed006a8c48a7ab76aafa93951e34e9f722a7f23ffdf7e507ff)。
- 公开时间:2026-11-14T23:12:18Z(gh release view 的 publishedAt);标签 v0.0.66 与 v0.0.66-src 均指 F。release/decision.txt 裁决(tools/release-decide.cx 实跑):公开前 NOT_QUALIFIED unknown=2 fail=0(第 4 步 s9 与第 8 步读回为 UNKNOWN),公开读回后 NOT_QUALIFIED unknown=1 fail=0(只剩第 4 步 s9 为 UNKNOWN)。本版不宣称 QUALIFIED。
- 公开后读回:已做,下载 6 件,5 件 sha256 与 SHA256SUMS 逐件一致并与 release/candidate.tsv 各列一致,6 件与本地打包件逐字节相同(APE 75c4e940af61d995...)。本机装机:本文件定稿时未做。
- 速度数字只引 build/speed-baseline.tsv 已有行。0.0.66 的启动、冷编译、生成代码对 0.0.65 的对比:公开时未测;装机后实测(已装 0.0.66 对 0.0.65,两边同走包装根,ABAB 交错 n=9,load1 0.60 至 1.27,build/speed-baseline.tsv 2026-11-15 的 0.0.66 行):首次启动 665.1 对 664.0 ms、重复启动 28.5 对 28.5 ms(run-startup-bench)、冷编译 edit-span 2634 对 2625 ms、ratchets 4989 对 4993 ms(coldcompile),差值均在噪声内;gencode 8MB 两边各只 1 轮,sha 976.2 对 987.6 ms、sort 1077.5 对 1094.1 ms、memcpy 12.02 对 12.00 ms。无变慢项,也无可声称的提升;speed-probe 未测。
- 装后门实测(已装 0.0.66,APE 75c4e940af61d995):tests/cx-namespace.cx checks=18 failed=0 unverified=0;K3 硬模式(UNISACC_REQUIRE_N15C=1,tests/phase-timing-stamps.cx)assertions=76 failed=0;tools/check-export.cx passed=13 failed=0 unverified=0。

## 是什么

面向 C99 的编译器,自己写出 ELF、Mach-O、PE,可编译自身;产品形态为单文件 `unisacc.com`。0.0.66 的主题是库包源四份补丁(sx、r2、rl2、npx),另带起链与测试工具的若干修正。上一发布版见 plans/release-notes-0.0.65.md。

## 本版内容(按入库提交,范围 b7520b8e81..24bfc44c70,共 26 个提交,其中非合并提交 22 个)

### (1) 库包源四补丁

入库提交 354a116990:按 sx、r2、rl2、npx 叠贴;包版本 ext.net 0.1.9、cx.stream 0.1.2、cx.sys 0.1.11、ext.sysx 0.1.3;新增 cx/sys/reslive.cx,改动 cx/stream 的 item、pipe、rw 与 ext/net 的 tcp、unix 等。该提交记载的检查:14 个 .cx 用已装 0.0.65 逐个 -c 实编 rc 0;sys-reslive 与 ext-procx-exec 两批登记并实跑 PASS;derive ok=9 drift=0;ratchets ok=13 fail=0;tlib-frozen failed=0。同提交如实记载:affected 全量枚举 60 秒超时,未记 PASS。

后续两提交:2333b3713c 重生 build/enosys-sites.tsv(enosys-sites pass=1 fail=0;流 API、所有权、一致性直接合同 pass=3 fail=0;网络 unix、tcp、udp、sock 及 np_exec 直接合同 PASS);12a3f19228 修正 build/std-surface.tsv 的三行事实(np_exec 改为 exists 状态,两条已删除的 deadline 接口按 removed 注释约定移出活动行),std-surface 实跑 PASS。

### (2) 冻结表与接口基线补登

提交 a7734cfcee:冻结表补登 0.0.65 T8 引入的 10 个 cx/cx0/proj 投影片段,coreapi 基线随之补 10 行 cxm_* 公开接口(均为既有 cxmod 驱动接口,非新增语义)。

### (3) 起链与测试工具

(a) candidate-chain 批拆为 core、objects、watch 三组(提交 2854ce4362,104 条断言 = 85 + 16 + 3,commit 覆盖保持)。限定:S9 未接线,commit 档三组在 S9 中报 DEFERRED needs-materials;冷 HOME 验证 UNMEASURED。
(b) 看门狗:默认选择最近完成的兄弟链作基线(2b972c5ee1);准备阶段复用链内源码钉住以隔离 tlib 依赖(5cd04ebf75);准备失败不挡整链,记 WATCH-UNAVAILABLE(6a9465439a)。
(c) s9-nightly 前置门:根下候选 unisacc.com 无可执行位则 REFUSE 并点名路径与权限,rc 2,不自动 chmod(06decafea4)。
(d) status 与 affected 的 .git 路径经 git rev-parse --git-path 解析,版本工作目录下可用(3510d7c60b);check-tlib-base 环包探针补链模块目录(5d9bf5373b);tools.cxlib 0.3.1 -> 0.3.2 补 bump(83f0215bdd)。

其余提交为 0.0.65 的公开记录、装后实测与派生物重生,不属本版产品改动。

## 未达项与已知缺口(必须如实写)

(1) 同候选 CI run 43648734577 已结束,10 作业全 success;osx、win 的平台证据仅限该 run 各作业实际执行的范围。
(2) s9 commit 档(主机范围 lnx/x86_64,根为 F 树快照加候选件,候选权限 0755):选中 329 批,310 PASS、7 FAIL、12 UNKNOWN,工具裁决 `S9_VERDICT 4 UNKNOWN reason=tier-commit-not-full`(commit 档按设计不给全批资格),decision.txt STEP 4 记 UNKNOWN,不写 PASS。定性:未发现编译器产品回归;有两项为本版树内派生物漂移(见下)。s9 内的 FAIL 与 UNKNOWN 记录保留不改,未重跑整档。逐项:
- surface-doc-gen FAIL(本版新增):断言 check-tree 红,`tools/surface-doc-gen.cx --check` 报 docs/cx/api/ext.procx.md 与 docs/cx/api/cx.stream.md 两份生成文档与 build/std-surface.tsv 不一致(diff=2);库包四补丁后未重生,属树内文档漂移,不在产品件内,未修。
- dock-audit FAIL(本版新增):3 条断言红(A1-rows-equal-docked-count、A1b-summary-docked-equals-rows、cx/sys/reslive.cx);本版新增的 cx/sys/reslive.cx 未在 build/dock-audit.tsv 播种行,属审计表漂移,未修。
- release-sweep FAIL(24 条):工具对 /tmp 下路径拒绝(tools/release-sweep.cx 第 940 行 `refusing /tmp path`),本次 s9 根放在 /tmp/s966/root 所致,属 s9 根位置的环境原因;未在非 /tmp 根复跑证实,UNVERIFIED。
- release-usage(6 条)、preflight-tool(`check-preflight: git rev-parse`)、chain-batchkeys(no-out-dir-in-checkout)、report-verify-tool(positive-all-ok)4 项 FAIL:0.0.63、0.0.65 已有,冻结根环境所致,照旧,未在其他根复跑证实,UNVERIFIED。
- fixedpoint-sample、prechain-gate UNKNOWN:55 秒预算超时,0.0.63 已有,照旧,未复测。
- s9-nightly-run-fail UNKNOWN(本版新增):s9 内 37 秒超时;在同一根单跑通过(单跑证据,非 s9 整批证据)。
- candidate-chain、candidate-chain-objects、candidate-chain-watch UNKNOWN:S9 未接线材料 candidate-chain-tool(`S9 NOT_AVAIL material=candidate-chain-tool`),整片未执行,与下文 (7) 一致。
- root-canon、root-independent、ctor-debt、rechunk、sys-reslive、ext-procx-exec 共 6 项 UNKNOWN(本版新增):与上述 candidate-chain 三组同片(slice-0-23、slice-0-76),整片因材料不可得未执行而被连带;在同一根逐项单跑 6 项全过(单跑证据,非 s9 整批证据)。
- 0.0.65 的 tlib-prep-refuse FAIL 与 tlib-prep UNKNOWN 本次在 s9 内为 PASS;0.0.65 因候选缺可执行位失败的 7 项本次在 s9 内为 PASS。
daily 与 release 档未跑。本机全批资格:未取得。
(3) s9 工具缺陷(本版发现,未修):tools/s9-native.cx 的缓存键只取 s9-shards.cx 或 all-waves.cx 自身字节的 sha256,不含它们依赖的模块(如 tools/cxlib/testplan.cx)。本版 testplan.cx 改了而这两个文件没改,首两次 s9 init 命中 0.0.65 时期编出的旧 ELF,被 `REFUSE registry-metadata commit-materials row=780 id=candidate-chain` 挡住;把两个旧缓存项改名让位后第三次才跑通。上面的计数出自第三次。
(3b) 本机装机:本文件定稿时未做。F 树内 release/candidate.tsv 的 ape_sha256 列为 0.0.65 的值(889dfe85...),X 才带本版值;s9 根取 F 树,各批输出中未见该旧值,BATCH_RUN 记录的候选 sha 均为 75c4e940...,未发现受影响的批次。
(4) first_e2e 500 ms 启动门未过:装机后实测 0.0.66 首次启动 665.1 ms 对 0.0.65 的 664.0 ms(run-startup-bench,两边包装根,ABAB n=9),差在噪声内;0.0.65 装机后以 abab-bench 口径为 678.0 ms(build/speed-baseline.tsv,工具不同,不可直接比)。本版不宣称启动变化。
(5) 已知编译缺口,未修:已装编译器对大型二维自动数组报 `not covered: an op outside the first encoder slice`(0.0.63 上发现)。0.0.66 是否同样复现:UNVERIFIED。
(6) 库包四补丁的限定:affected 全量枚举当时 60 秒超时未记 PASS;提交 2333b3713c 记载 affected 分片当时有 cx-layers 一项 FAIL(十个 cx0/proj 模块 FREEZE-NEW),后由 a7734cfcee 补登冻结表,补登后的全量复跑结果本文件起草时未核,UNVERIFIED。
(7) candidate-chain 三组在 S9 中为 DEFERRED needs-materials,不是 PASS。
(8) D1a 在仓内模块的落地、export 对既有模块的影响:仍未做(沿用 0.0.65 未达项 (7))。
(9) sources_sha256 在打包步的核对为 UNVERIFIED(见文首)。

## 0.0.65 已知缺口里仍开的项

沿用 plans/release-notes-0.0.65.md 与 plans/release-notes-0.0.63.md 编号;本版未修或未验证修复,不得写成已解决:
(1) DNS 未做(功能边界):sy_resolve/sy_resolve4 只认数字字面量与 localhost。
(2) std-surface 的 new 行、sy_unix_connect 越过截止时间、sy_tcp_accept_from 对 unix 监听返回 -EAFNOSUPPORT、poll 后阻塞调用截止时间、EINTR 上抛、netmsg salen 为负、sy_sig_taken 非原子:本版库包补丁触及 ext/net 与 cx/sys,但逐条是否已修本文件起草时未重核,UNVERIFIED。
(3) cx 模块软链被当两个模块、osx/win 的 -ENOSYS 点未全进 exec/facts/refusals.tsv:未修。
(4) arm64 msghdr 布局未实跑;osx/win 无主机实跑证据;osx cache-manager 自产件在 macOS 上的运行与签名未验证:本版未新增证据。
(5) 批次全集复验面不完整,s9 daily 与 release 层未跑通一次。
(6) check-stdio-buffer 覆盖范围、dock-audit 全部 UNAUDITED、derive 总闸登记项偏少:未变。
(7) 0.0.65 s9 commit 档的 FAIL 与 UNKNOWN 项(tlib-prep-refuse、chain-batchkeys、preflight-tool、release-usage、report-verify-tool、fixedpoint-sample、prechain-gate 等):本版 s9 commit 档复跑结果见未达项 (2):tlib-prep-refuse 转 PASS,其余仍红或超时。

## 撤回与回滚

撤回办法以 release/rollback.txt 的 0.0.66 版为准(随 Y1 入库):`gh release edit v0.0.66 --draft=true` 并 `gh release edit v0.0.65 --latest`;main 上的 Y2、Y1、X 提交按新到旧 `git revert`,不 force push;标签不删。本机根回退件为 0.0.65(~/.cache/unisacc/ape-0.0.65,APE sha 前缀 889dfe85)。回滚核对项:资产件数按 6 件。

规则:发布页正文必须与本文件逐字相同,公开后读回核对;读回结果记账本。

## 待核对

(1) CI run 43648734577 的结论已填入文首。
(2) pkg.bin 与 0.0.65 逐字节相同(sha 同为 c7cb3a8c...)而 APE 不同:差异来自六个平台片与注入版本,公开前核一次各片 sha 与整链收据。
(3) 公开时间、裁决、读回结果已补(见文首);s9 第 4 步仍为 UNKNOWN;装机后速度对比与装后门已做(见文首),gencode 只各 1 轮、speed-probe 未测。
