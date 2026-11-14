# unisacc 0.0.63 -- 发布说明

状态:草稿,定稿于公开之前。沿用 plans/release-notes-0.0.62.md 的口径:只写已入库事实;未实跑的项写未跑,未证实的项标 UNVERIFIED,不写 PASS;本文件不宣称 QUALIFIED。候选提交、CI run、sha256、公开时间、裁决等字段尚无事实,标"待填",由发布流程填写,不得臆测。

- 冻结提交(F):9f3d367e44d7e6e89aca9279272f0b0472e31166;候选表提交(X,仅改 release/candidate.tsv):86e85e60c8686e50c3330b6496505811465216f1。
- 候选 APE(unisacc.com)sha256:04b85e6e4ec3ddcb076aa5af02185ee8f3f8f1e3e9c5808776ab0a5e21b2c9a5。
- 同候选 CI:candidate-native.yml run 43600577542,completed success,10 个作业全 success,含 6 个平台候选作业。
- 草稿 release v0.0.63(未公开)资产 6 件:unisacc.com、pkg.bin、slice-win-arm64、unisacc.com.build.json、build.json、SHA256SUMS;无 zip。
- 公开时间:待填(公开后由读回填);release/decision.txt 裁决(公开前,tools/release-decide.cx 实跑):NOT_QUALIFIED unknown=2 fail=0(第 4 步 s9 与第 8 步读回为 UNKNOWN;读回在公开后补)。
- 速度数字只引 build/speed-baseline.tsv 已有行,下文逐条标出处;未测不写数。
- 规划与口径依据:prd_mempalace.md "下一动作(0.0.63,fable 第五轮规划 2026-11-14)"节的卖点四条与未达项。

## 是什么

面向 C99 的编译器,自己写出 ELF、Mach-O、PE,可编译自身;产品形态为单文件 `unisacc.com`。0.0.63 的主题是驱动与运行缓存的一批修正(63A 最小集:E3TRIM2/E3TRIM3、C2a、N15)与发布资产合同摘去一件冗余包装。上一发布版见 `plans/release-notes-0.0.62.md`。

## 卖点(全部带口径)

### (1) 大缓存目录与 E3TRIM2、E3TRIM3

E3TRIM2:模型缓存目录的修剪由每次发布全量扫描改为摊销账。口径:专用 UNISACC_E3CACHE 目录预填假项,first_e2e,n=3 取中位,load1 5 以上,全部 noisy;候选为 entry-build 自含入口形态(30ef6915),0 项基线差约 18 ms 含入口差异未隔离。出处:build/speed-baseline.tsv 2026-11-14 的 E3TRIM2 曲线各行。

| 目录项数 | 账状态 | 0.0.62 中位 | 63A 候选中位 |
| --- | --- | --- | --- |
| 0 | 账已在 | 244 ms | 225 ms |
| 2000 | 账缺失 | 492 ms | 286 ms |
| 2000 | 账已在 | 286 ms | 222 ms |
| 4000 | 账缺失 | 704 ms | 329 ms |
| 4000 | 账已在 | 330 ms | 223 ms |

曲线近乎拉平:4000 项账缺失 704 -> 329 ms,账已在 330 -> 223 ms。

顶满预算(MAX_BYTES=380000,1900 项):E3TRIM2 单独无收益,候选 531/415 ms 对 0.0.62 的 489/396 ms(账缺失/账已在,中位),差在噪声内,顶满仍全扫。出处:speed-baseline.tsv 满预算两行。

E3TRIM3(低水位淘汰,淘汰到预算的 90%、记账门槛 100%,入库提交 657f2113ca):满预算实测,专用 0700 缓存预填 1900 项(380155648 B)、限额 400000000,n=5 中位,每样本两个新内容程序,load1 2.2 至 2.4。出处:speed-baseline.tsv 指标列为 run1_first_publish、run2_second_publish 的两行(三产品对照,E3TRIM3 列的候选为 ca36d2eb)。

| 口径 | 0.0.62 | 63A | E3TRIM3 |
| --- | --- | --- | --- |
| 首次发布 | 463 ms | 347 ms | 280 ms |
| 第二次发布 | 284 ms | 240 ms | 224 ms |

限定:该曲线下总量未超限额,故没有走到淘汰;淘汰到 90% 的路径耗时未测(UNMEASURED,见未达项)。modelcache-budget-lru 与 modelcache-budget-concurrent 两批 PASS(2026-11-14 在 main 2bd89d4ead 用已装 0.0.62 实跑 `tools/affected.cx --ids ... --run`,六批 pass=6 fail=0;冻结提交上须重跑)。

### (2) C2a:缓存根与 TMPDIR 解耦(UNISACC_CACHE_DIR)

缓存根与 TMPDIR 解耦,默认根改到家目录下。实测:同一新内容程序换 TMPDIR,候选在 UNISACC_CACHE_DIR 同目录(0700)下第二 TMPDIR 命中 3 ms、0 行 inference;0.0.62 忽略 UNISACC_CACHE_DIR,同设置第二次 308 ms 未命中。默认缓存根下两者换 TMPDIR 都命中(候选 4 ms,0.0.62 8 ms);load1 6.0 至 6.8 noisy,n=1。出处:speed-baseline.tsv C2a 行。

迁移与注意:
(1) 旧的 `$TMPDIR/unisacc-e3cache-<uid>` 根不再使用,也不自动清理,需手工删除(0.0.62 阶段该目录曾约 1 GiB)。
(2) 缓存目录须为 0700,否则缓存被静默不用,且无任何输出。
(3) 默认根改为全机共享后,并发作业共用一个根;需要隔离时设专用 UNISACC_CACHE_DIR(0700)。
(4) 默认根口径下"两个不同 TMPDIR、同源第二次 RR_TIME lookup_hit=2"的冻结门尚未在冻结提交上跑(UNVERIFIED)。

### (3) N15:-c 对象复用

同命令、同 -o 的 `-c` 编译,第二次起 0 行 inference。键含 argv,换 -o 文件名则键不同、重跑,不命中(有意,换名复用未做);改源后回到 7 行。证据:修复提交 2049e14b8a;副本重建候选(c0c4b24f,专用 0700 缓存根)三次 inference 为 7/0/0,对象 sha 三次相同(29227a42857f),出自 prd_mempalace.md N15 定因条。注意:build/speed-baseline.tsv 的 N15 行是修复前的 63A 候选(30ef6915)探针,三次均 7 行(落空);修复后的 7/0/0 目前只在 prd_mempalace.md 有记载,未入该表。第二份独立实跑(prd_mempalace.md Z6 行):origin/main c39a890651 的 git archive 副本经 entry-build 得候选 227e63f5998e,同命令同 -o 三连 inference 为 7/0/0,对象 sha 三次均 e2df35b00e46,与 `UNISACC_RUN_CACHE=0` 所得对象 cmp 相同;只测了一个极小源,换 -o 名与改源在该次未测。

测试:tests/phase-timing-stamps 新断言(K3,提交 d59e4b01a0),旧产品 70 PASS、6 UNVERIFIED,候选 74 PASS(含版本槽未写导致的 ver.cold/ver.hot 两条版本串 FAIL 说明以 prd_mempalace.md K3 行为准);装新产品后须以 `UNISACC_REQUIRE_N15C=1` 重跑,结果待填。

限定:对真实构建系统,若每次换 -o 文件名,命中率为 0,本卖点可能落空。自举链内对象相是否命中缓存(固定点比较变缓存对缓存)链内未实测,UNVERIFIED;起链前须固定 UNISACC_RUN_CACHE=0 或断言对象相 artifact_hit 为 0。

### (4) 发布资产 6 件,不再提供 win-arm64.zip

0.0.63 发布上传清单:unisacc.com、pkg.bin、slice-win-arm64、unisacc.com.build.json、build.json、SHA256SUMS 六件。原因:0.0.62 及以前的七件合同里,win-arm64.zip 的四个成员与单独资产重复,其中 run.cmd 从未在 Windows 原生运行验收(原生 CI 只运行 slice-win-arm64 本身),故摘除;已公开的 0.0.62 资产不改动。Windows arm64 用户直接用 slice-win-arm64 加 pkg.bin。依据与摘除实现:prd_mempalace.md "win-arm64.zip 去留"小节;工具侧 check-release-layout 17 项断言 failed=0(该节记载)。

限定:全仓归属已做(prd_mempalace.md F26 行;2026-11-14 复核 `git grep -n 'win-arm64.zip'`):命中只剩按版本分支的发布工具、0.0.62 及更早的候选表与历史夹具,.github/workflows 无该 zip 的取件,README 与安装说明无该 zip 的下载链接,现行 release/rollback.txt(0.0.62 版)不含件数;check-release-layout 17 项断言 failed=0(同日实跑,含 0.0.62 判七件、0.0.63 判六件、多出 zip 即拒绝)。仍 UNVERIFIED:0.0.63 真实草稿与公开后的读回件数(应为 6)未跑;release-oneshot、releaseexec、releaseprepare、release-run 的 0.0.63 分支没有用例;对 0.0.62 release 只核了资产数为 7(`gh release view v0.0.62`),未用读回工具预演。

## 未达项(必须如实写)

(1) first_e2e 500 ms 门未过。63A 候选(30ef6915)hello 口径 n=3:676.7/684.8/683.8 ms,中位 683.8 ms;已装 0.0.62 同口径 675.2/675.7/655.1,中位 675.2 ms;load1 3.5 至 5.8 noisy,9/9 断言过。出处:speed-baseline.tsv "63A 重建候选 30ef6915 / tools/run-startup-bench.cx --product"行。E3TRIM3 候选同工具 n=5(同表 startup_first 两行):ca36d2eb 中位 687 ms 对 0.0.62 的 691 ms;227e63f5998e 中位 664 ms 对 678 ms。即 hello 口径冷启动本版无改善,持平。冷编译与生成代码同样持平,本版不宣称变化(同表 227e63f5998e 各行,n=5):tools/edit-span.cx 冷编译 2743 ms 对 0.0.62 的 2674 ms,五次区间重叠(更早一行的 2745 对 3523 未复现,不采用);tools/ratchets.cx 冷编译 5061 对 5239 ms;gencode sha 8MB 单轮 1013 对 970 ms,经 ABAB 交错 n=15 复测为 997.1 对 995.4 ms,两编译器产出的可执行文件逐字节相同,判噪声。L1 方案 A 与 N7 未带(决定:收益为推算、同字节合同风险未测,见 prd_mempalace.md),500 ms 门顺延,不放宽。
(2) E3TRIM3 淘汰路径耗时未测:现有满预算实测总量未超限额,淘汰到 90% 的一次性开销 UNMEASURED。若冻结时 E3TRIM3 新旧断言或满预算曲线不满足带入规则,本节(1)将改写为"顶满预算的目录无收益"。
(3) s9 daily 与 release 档未跑:按决定只有 commit 档计划卡发布,daily 与 release 档夜跑、结果如实入说明、不卡发布。0.0.62 上 commit 档实测 289 批通过、11 失败(8 转绿、3 已修,提交 d892ef38b9;该三批 chain-phases-gen、cx-driver-deps、dock-audit 于 2026-11-14 在 main 2bd89d4ead 单独复跑 PASS)、4 个 watchdog UNKNOWN 未复测;0.0.63 候选的 s9 commit 档(在冻结 F 的快照根上实跑,主机范围 lnx/x86_64):选中 321 批,313 PASS、6 FAIL、2 UNKNOWN,工具裁决 `S9_VERDICT 4 UNKNOWN reason=tier-commit-not-full`(commit 档按设计不给全批资格),decision.txt STEP 4 记 UNKNOWN,不写 PASS。6 个 FAIL 的定因:derive、tlib-frozen 为 0.0.63 真缺陷,属检查与登记,不进产品(check-derive 断言未随第九个生成器入册更新;tools.cxlib 改动后未补版本号致冻结摘要不符),已在 main 提交 41a15ffc7f 修复;发布树(F)内含这两项检查的旧版,即在 F 上重跑仍会红。release-usage 推断为真缺陷,未单跑复核(UNVERIFIED)。chain-batchkeys、preflight-tool、report-verify-tool 定因为冻结快照根的环境所致,未在其他根复跑证实(UNVERIFIED)。2 个 UNKNOWN 为 fixedpoint-sample、prechain-gate(watchdog-deadline,既有,未复测)。daily 档预测约 16111 s 为条件值,墙钟 UNMEASURED。
(4) 候选级证据:
- 整链 real7:共三段。前两段因设置 UNISACC_RUN_CACHE=0,在 g2 阶段超时失败;第三段通过(rc=0,compare 通过,产物 unisacc-next.com sha 8312372d7af1e23d,冻结 F 为 9f3d367e44d7)。前两段失败原始记录保留,不抹去。
- 同候选 CI:run 43600577542,10 个作业全 success,含 6 个平台候选作业。osx 与 win 平台证据范围以该 run 作业为准,不扩大解读,见未达项 (5)。
- 本机全批资格:未取得(s9 commit 档 313 PASS、6 FAIL、2 UNKNOWN,见 (3));仍待填:候选速度基线:build/speed-baseline.tsv 中暂无含 9f3d367e44d7 的行(grep 计数 0),待填,未测不写数(candidate 自含入口与发布件入口的差异未隔离)。
(5) osx 与 win 平台证据:沿用 0.0.62 缺口,0.0.63 未新增平台实跑,UNVERIFIED。

## 0.0.62 已知缺口里仍开的项

以下沿用 plans/release-notes-0.0.62.md 编号;本版未修或未验证修复,不得写成已解决。

(1) DNS 未做(功能边界,既定):sy_resolve/sy_resolve4 只认数字字面量与 localhost。
(2) 0.0.62 缺口 (2):std-surface 仍有 2 行 new(sy_res_live、np_exec);本版未重算,UNVERIFIED。
(3) 0.0.62 缺口 (3)(4)(sy_unix_connect 越过截止时间;sy_tcp_accept_from 对 unix 监听返回 -EAFNOSUPPORT):补丁草稿在 patches-0.0.63 的 sx 补丁,对 main 2bd89d4ead 仅 `git apply --check` 通过(2026-11-14 复核);未入库即视为仍开,UNVERIFIED。
(4) 0.0.62 缺口 (5)(6)(7)(8):poll 后阻塞调用不保证截止时间、EINTR 上抛、netmsg salen 为负、sy_sig_taken 非原子,均为读码推断,未修。
(5) 0.0.62 缺口 (9):cxm_checkcalls 误拒短名函数指针调用,已复现,既有缺陷;修复补丁(checkcalls)排 0.0.65,本版不含。
(6) 0.0.62 缺口 (10)(12):cx 模块软链被当两个模块等问题,以及 osx/win 的 -ENOSYS 点未全进 exec/facts/refusals.tsv,未修。
(7) 0.0.62 缺口 (11):-c 对象路线无最终产物复用,已由本版 N15 处理,但限定见卖点 (3)(键含 -o)。artifact_hit 戳丢失的提交未二分,不影响本条结论。
(8) 0.0.62 缺口 (13)(14)(15)(16):arm64 msghdr 布局未实跑;osx/win 无主机实跑证据;osx cache-manager 自产件在 macOS 上的运行与签名未验证;wt-gate-probe 为 PLANNED。本版未新增证据。
(9) 0.0.62 缺口 (18)(19):批次全集复验面不完整,s9 daily 与 release 层未跑通一次(见未达项 (3))。
(10) 0.0.62 缺口 (22)(23):check-stdio-buffer 覆盖范围、dock-audit 全部 UNAUDITED、derive 总闸登记项偏少,未变。

## 撤回与回滚

按运行手册(release/rollback.txt 形态,0.0.63 版已写,首行含 0.0.63,随 Y1 入库):撤回公开用 `gh release edit v0.0.63 --draft=true` 并把 v0.0.62 重设为 Latest;main 上提交只用 `git revert`,不 force push;本机根回退用 `tools/install-ape.cx --rollback`。回滚核对项:资产件数按 6 件,不得按七件核对。

规则:发布页正文必须与本文件逐字相同,公开后读回核对;读回结果记账本。

## 待核对

(1) 卖点 (1) 中 E3TRIM3 数字取自 speed-baseline.tsv 的 run1_first_publish、run2_second_publish 两行,"跑后"目录项数与淘汰行为以冻结提交重测为准。
(2) 卖点 (3) 的修复后 7/0/0 只在 prd_mempalace.md,未入 speed-baseline.tsv,冻结后装新产品重测并入表。
(3) 卖点 (4) 的 F26 全仓归属已填;0.0.63 草稿与公开后的读回件数、0.0.63 版 release/rollback.txt 在发布步骤中补,公开前必须写好。
(4) 0.0.62 缺口 (3)(4)(9) 的补丁是否入 0.0.63 或 0.0.65,以冻结时 main 为准。
