# unisacc 0.0.58 -- 发布说明草稿(未公开;未实跑一律写未跑)

状态:**未公开**。候选在同候选原生 CI 上 FAIL(pass=40 fail=2),两个 Windows 目标的 stream-pipe 格失败,原因指向本版唯一的产品改动 cx.stream。按「CI QUALIFIED 之后才裁决与公开」的流程,发布尾部停在草稿:没有提交 Y,没有 tag,没有公开,本机根仍是 0.0.57。本文件是这次候选的如实记录,供修复后重新冻结的下一轮候选使用;不是一份已发布版本的说明。

候选提交 X `931fa623b54c0f32cdf31b044ea5b1c63aebf9d4`(只含 release/candidate.tsv)。构造源 `9a34cb97b4128187a60aaad339f319aadee65de7`,源 SID 4a8ccef4(`4a8ccef4c235b51441f6e9aa08e2bc0cda029d8f3c6ee438920263f22365344f`),引导产品为 0.0.57 公开 APE c116a52f。CI 失败后,X 的候选表已由 `90856efe238f96324d7b35218c557826669428c6` 还原为 0.0.57 公开记录(原因见「发布尾部」)。git 范围 `v0.0.57..931fa623b5` 现算 68 条,其中构造源之后 27 条。

## 是什么

面向 C99 的编译器,自己写出 ELF、Mach-O、PE,可编译自身;产品形态为单文件 `unisacc.com`。0.0.58 计划是「一类产品改动 cx.stream 加一批构造链、tlib 与门工具」的版本。上一发布版见 `plans/release-notes-0.0.57.md`。

## 身份与证据

| 对象 | 身份 | 说明 |
|---|---|---|
| 链内 APE(pack 相) | 27cc6d82467a194723eb9d9d1f2ad7cfd4a80fbb394b915f7aaff9cb31becd79,6750580 字节 | `bg-candidate promote --check` 输出 PROMOTE_CHECK OK phases=498/498 inputs=11,自带 not=PUBLISHED not=PASS |
| 候选 APE `unisacc.com` | 8c8cd94f77d63d72322628c76f772379c468aedadcbbb093f6c1c4c424b6868e,6750580 字节 | `release-run --pack` 由同一批链输入重打包并注入版本号;本机 `--version` 实测 `unisacc 0.0.58`;SOURCES_SID VERIFIED 4a8ccef4(identity-helper 对冻结快照现算) |
| 与 0.0.57 的字节关系 | 六片 sha 全部不同(lnx/x86_64 2ad31df6、lnx/arm64 0b0c7bf3、osx/x86_64 2c4dd7dd、osx/arm64 47b316c9、win/x86_64 e5c34202、win/arm64 d25c1448);P4 包 56efea53 与 0.0.55 至 0.0.57 同字节 | 本版产品字节确实变了,不是只换版本号 |
| 其余资产 | pkg.bin 56efea53,slice-win-arm64 392dbbd6,unisacc.com.build.json 与 build.json e0d7c4b0,zip a49e7808,SHA256SUMS 77c45a2c | 候选表存 out/rel058/rr/candidate.tsv 与提交 X |
| 链内 compare | objects=same count=90,kernels=same count=2,slices=same count=6,product=same | 第 286 次尝试的输出;chain-run 终态自报 PREPARED not=PASS |
| 草稿与资产齐全检查 | v0.0.58 草稿,target 为提交 X,七资产 state 全为 uploaded、字节数与本地相同;草稿读回七件与本地逐件 cmp 相同,SHA256SUMS 6/6 OK,release-layout --check PASS | 在触发 CI 之前完成。用的是 gh 查询加读回的手工等价检查,不是 releaseexec 里的 assetguard 代码路径(那条路径只在 `release-oneshot --execute` 内) |
| 原生 CI | 运行 43219651302(检出提交 X,ape_sha256 8c8cd94f,release_tag v0.0.58):**FAIL** pass=40 fail=2 not_run=0 invalid=0,20:37:47 触发,20:40:44 出结论 | 见下节;只跑了这一次,没有重新触发 |
| release-decide | 步 1、2、3、5、6、8 PASS,步 4 s9 UNKNOWN,步 7 ci-qualified FAIL;NOT_QUALIFIED unknown=1 fail=1 | 原样输出存 out/rel058/decide.txt;步 8 是草稿读回,不是公开读回 |
| s9 commit 档 | 候选 APE 8c8cd94f 加提交 X,本机 lnx/x86_64,21:07 聚合:selected 191,PASS 187,FAIL 3,UNKNOWN 1,pending 0;qualification NOT_FULL(12 个 osx 批本机 NOT_APPLICABLE)。FAIL:chain-phases-gen(checks=10 failed=1)、productpkg-dir-identity(assertions=18 failed=3,含 receipt-copy-ok)、batch-packages(table-fresh)。UNKNOWN:chain-batchkeys(55 秒看门狗超时)。四批都未查明、未修、未重跑,时限未放宽。0.0.57 的两个 FAIL(s9-nightly-run-pass、s9-nightly-run-fail)本次 PASS。分相耗时:init 159、steps 四次 196、198、208、18、aggregate(4 片加 merge)120 秒,每次调用都在 5 分钟内 | 这是第二轮的结果;第一轮因克隆准备不全作废,见「发布尾部」。s9 不在公开的关键路径上;不称 QUALIFIED |
| s9 release / daily 档 | 本版未跑 | |
| 公开、tag、提交 Y、公开读回、本机根安装 | 全部未做 | 本机根仍是 0.0.57 c116a52f |

## CI 失败(候选不公开的原因)

六目标七格共 42 格,40 格 PASS。失败的两格是 win/x86_64 与 win/arm64 的 stream-pipe:
- 子检查 B plain-image-pipe-cmp、C hello-stdio-image-pipe-cmp、D two-source-image-pipe-cmp、E 65-source-tape-pipe-cmp(asis 与 lf)全部是参照路径 rcA=0 有输出,管道路径 rcB=1、输出长度 0、标准错误为空;E 的定位行是 `step=e2:m0.c pipe_rc=1 frame_argc=3 pipe_err=-`。npass=2/6。
- 同一运行里 lnx 两格、osx 两格的 stream-pipe 都 PASS;两个 Windows 目标的 identity、version、hello、run、stdio-crossunit、smoke 都 PASS。
- 归因:`.github/`、`tests/candidate-qualify.cx`、`tests/win-portable-cell.ps1` 自 0.0.57 的提交 Y 以来没有改动(git diff 为空),0.0.57 同一套检查是 42/42;v0.0.57 到构造源的产品输入差异只有 cx/stream(新增)、ext/memx 两个文件的 use 改名、src/drivestream.cx(改为两阶段管道)。因此判为 cx.stream 这次产品改动在 Windows 上的回归。两个不同架构的 Windows 运行器结果一致,不是偶发,所以没有重跑。
- 根因有一条线索,未在 Windows 上验证(本机没有 Windows 宿主):`src/drivestream.cx` 用 `xs_open(&h, 0x7fffffffffffffffL)` 开管道,`cx/stream/pipe.cx` 的 `xs_open(int *h, long bound)` 在 `bound <= 0` 时返回 4 且不打印;win 目标预定义 `_WIN32` 而没有 `__LP64__`(候选 APE `-b win/x86_64 -E` 实测),long 若为 32 位,这个上限截断成负数,管道在任何输入之前就被拒,正好是「rc=1、无输出、无标准错误、所有管道子检查一起失败」的现象。接口里其余以 long 传字节数的地方同样要查。XSTREAM-058 冻结前的证据只覆盖 lnx/x86_64(71 条 stream 命令与已装产品同字节),它自己写明未跑六目标;这次回归正落在那个未跑范围里。
- 日志:out/rel058/ci-run1.log。

## 行为变化

产品输入只有一类改动 cx.stream(`2afa486d68`,加 `9a34cb97b4` 的 whole-front 修正):
(1) 新增核心包 cx.stream@0.1.0:`cx/stream/item.cx`、`cx/stream/pipe.cx`,12 个公开函数 xs_*,只依赖 mem.grow、mem.slice、mem.bytes、text.text;接口清单 build/layer-api/cx.stream.tsv。
(2) 首个消费者 `src/drivestream.cx` 改为两阶段管道(剥属性表,再进模型阶段或 sidecar)。
(3) `9a34cb97b4`:ext/memx 的跨单元 use 改为包根限定名,消掉 whole-front 的重复导出。
设计预期是 stream 命令行为不变;在 lnx/x86_64 上有同字节证据,在 Windows 上被上面的 CI 结果否定。

## 本版范围内的工具改动(`v0.0.57..931fa623b5`,非产品输入)

构造链:
- T1 G2 批量键(`c9db1471c7`)、T2 逐相预算与进程所有权 K10(`64ec0e64d6`)、T3 按目标并行 K9(`a84ef9695b`)、T11 分段与监督器及持久备忘(`7149a8ea3b`)。
- S1 作业续跑摊销与新鲜输出目录检查(`bbf44e0863`):jobs-tool/gen/tbl/net 在同一次调用里推进多个 start/finish,段首全量核已完成作业的身份与字节。这个提交的说明只有 `;;;`,没有文字,内容以 prd 能力速查「作业续跑摊销修复」一行为准。bofix(build-objects 走不通时退回一步构建)没有找到独立提交,只在 C58-BUILDSPLIT 的说明里被提到,具体落在哪个提交未核实。
- p3 拆相(`f9811804a6`):内核基底核对拆成独立相 p3-opguard。opguard 按 ISA 拆相加 ctor ctime 修正(`ea2de83739`):续跑不再无条件 chmod 构造器,翻译缓存不再每相失效。
- build 相拆成 build-front 与 build(`c68fa3ee35`,C58-BUILDSPLIT),相表 497 变 498。
- seed-helper 回执系列:每个输出目录钉一份 chain-step(`20bc155730`,CHAIN-STEP-PIN);构造器步看门狗秒数按快照 bound 的冻结范围取值(`1cda50ca06`,CTOR-WATCHDOG-RANGE,即 bound 修正);osx stub 步被杀后可重试(`72445dbac4`,WF-STUB-RESUME);进程表释放已消失的身份(`59ae54ef06`,CHAINRUN-PROC-TABLE);多次有界调用组成的相在时限前让出(`d0d7356507`,PHASE-YIELD)。
- B1 后台候选线 `tools/bg-candidate.cx`(`2036e661d4`、`c9fd3ab364`):start、status、promote --check、prune。本版整链全程经它启动。

tlib 与测试:
- tlib 包:接口清单与 cx.base(`513cc4c72d`、`2a1eeeec72`),ext.memx、ext.textx、ext.sysx(`e2caeb698a`、`29ad99f53e`、`93faf65dc9`),准备路径接线(`aa6344191f`),kit 增量(`fb0236a379`),核心环 cx.plan、cx.runtime、cx.compile、cx.driver(`8119efc34e`),私有 stdio 表修正(`162c543a2c`),覆盖计数(`31026005a2`)。
- tlib 批次入口(`b54284d3c8`、`e7eff69038`):run-batch 对准入表所列入口用套件链接后运行,失败退回源码默认运行。
- 测试分治粗分草稿(`f23cac02bb`)。

门与登记:
- S2 门前快检 `integrate-patch --stop-after`(`e0e2b49379`);DAG 任务图 `agentwork/dag.tsv` 与 `tools/dag.cx`(`8bd3f51e97`)。
- 门准备相 `affected.cx --prepare-materials`(`effc773243`、`a41dae9843`、`193e6b76b6`)。
- S7 草稿 `tools/reg-gen.cx`(`39b522d29b`、`219057333c`)。
- streams 7 项假红修正(`62591e9599`,STREAMS-058);productpkg 目录身份修正(`b9e3b8e350`,PPDIR-058)。

发布与其他:
- T5 发布尾部接线 c58-t5-adopt(`cca78515f0`),T6 assetguard(在 `7149a8ea3b` 内),T8 release-sweep、T9 version-metrics、T4 anchorban、s9red(`4d49176bc7`)。

## LIMIT-90-TEMP(时限临时放宽,如实)

主人 2026-11-12 授权把链相位与批子进程时限由 55 秒放宽到 90 秒(`554daec122`、`47ce9da692`),明确是临时覆盖「不放宽时限」铁律,只为解除迭代阻塞。tools/bound 外层上限 60 变 95。不变的:分段 270 秒、300 秒规则、各批 budget_seconds(仍 55)、build/g2-budget.tsv 的 55000 毫秒。这次放宽发生在整链进行中(17:44 落工作检出),本候选的链有相当一部分相是在 90 秒界下跑的;最终输出目录里 rc=124 的行有 8 条。回滚办法见账本 LIMIT-90-TEMP。

## 整链、重启与人工介入全表

构造源始终是 9a34cb97b4,快照 out/release-snap-9a34cb97b4 与产品输入未动。首次起链 16:37:09,pack 完成 20:34:41,墙钟约 237 分钟。链工具取自工作检出而不是冻结 rev(candidate-chain 以当前目录为 root),所以修链工具不需要重新冻结,但也意味着这条链先后用了多个版本的链工具。

| 次 | 起止 | 停点 | 原因与处置 |
|---|---|---|---|
| try1 | 16:37:09 起 | 未核实 | FREEZE-058 记了这次起链;现场不在 out/b1-old,结局未核实 |
| try2 | 16:50:12 至 16:54:39 | 2/497,resumes-exhausted | build 相四次约 53 秒被 55 秒上限杀。处置:build 相拆成 build-front 与 build(C58-BUILDSPLIT),相表变 498 相,换新输出目录 |
| try3 | 17:31:47 至 17:45:04 | 278/498,final-net-2 首败 | seed helper receipt mismatch:另一代理 17:44 在工作检出改了 chain-step.cx(55 改 90),续跑时 producer sha 变了,回执如实拒绝。处置:CHAIN-STEP-PIN |
| try4 | 17:49:41 至 17:53:36 | 6/498,ctx 首败 | 冻结 rev 的 source-package 里 8 秒初始源测量预算在高负载下超时。冻结输入不改,工具侧只补 ctx 可续跑;换目录重起 |
| try5 第 1 段 | 18:04:29 至 18:46:11 | 368/498,gen1-lnx-arm64-objects 首败 rc=2 | LIMIT-90-TEMP 把传给快照 bound 的秒数改成 90,冻结的 bound 只收 1..60。处置:CTOR-WATCHDOG-RANGE,同目录续跑 |
| try5 第 2 段 | 18:53:56 至 19:01:53 | 395/498,gen1-osx-x86_64-stub 首败 | 两个 osx stub 相并行抢 wf-prep 锁,后一个在时限处被杀,重试时被残留目录拒。处置:WF-STUB-RESUME,同目录续跑 |
| try5 第 3 段 | 19:04:56 至 19:07:55 | 404/498,g2-lnx-x86_64-closure-prepare,surviving-processes | 实查无存活进程;chainrunproc 身份表只增不减,满 1024 后登记失败。处置:CHAINRUN-PROC-TABLE,同目录续跑 |
| try5 第 4 段 | 19:12:48 至 19:16:29 | 406/498,resumes-exhausted | g2 两个 lnx 目标的 closure-prepare 每次都跑到 90 秒被杀,3 次 resume 用完。处置:PHASE-YIELD(让出合同,不加 resume 次数),同目录再起 |
| try5 第 5 段 | 19:30:27 至 20:34:41 | 498/498,pack DONE | 38 个分段、19 次监督迭代,rc=0 |

如实说明:
- 这不是一条无人值守的链:至少 9 次起链(try1 至 try4 各一次,try5 目录五次),换输出目录至少 3 次,最终目录的 chain-run.tsv 里有 4 条 BLOCKER。每次停点都是先修链工具再续,修的都是工具,没有改快照或产品输入。
- 最终产物是在同一输出目录里先后用至少五个版本的链工具产出的;try5 目录的 resume 计数在第 4 段已到 3/3,第 5 段是新的一次显式启动。
- 冻结前的整链预演 R3 取消,由这条 0.0.58 整链兼作新工具预演(FREEZE-058);上面的停点都是预演本可以提前暴露的。
- 本文作者是发布尾部代理,接手时链已在 try5 第 5 段的 compare 之后;之前的经过取自账本(FREEZE-058、C58-BUILDSPLIT、CHAIN-STEP-PIN、CTOR-WATCHDOG-RANGE、WF-STUB-RESUME、CHAINRUN-PROC-TABLE、PHASE-YIELD)与各目录的 owner.tsv、exit.tsv、chain-run.tsv,没有亲见。

## 发布尾部(实际做了什么)

- `release-oneshot --prepare/--execute` 没有用:`rp_freeze` 要求 ROOT 的 HEAD 等于 build_rev,共享检出的 HEAD 早已过了冻结点(当时是 e7eff69038),而本轮规定不用私有克隆;从源码读出必拒,没有实跑验证这个拒绝。因此 k7exec 执行入口与 c58-t5-adopt 的接链路径在真实发布上仍然没有被用过。
- `release-run --pack` 实跑一次通过(约 54 秒,贴着 60 秒命令界):输入是 promote --check 列出的 11 件,汇集到 out/rel058/src。它要求 ROOT/release/candidate.tsv 的版本与候选表一致并提示 ROOT 应是私有克隆;本轮直接在共享检出里把 release/candidate.tsv 写成 0.0.58 初始表再打包。打包用的 release-run、release-ape、version-inject、release-layout 取自工作检出(HEAD e7eff69038)。
- 提交 X 推 main,建草稿,逐件上传七资产,资产齐全检查,然后才触发 CI(0.0.57 的「资产没传完就触发」没有重演)。
- CI FAIL 之后的处置:不公开。提交 X 让读候选表的两个 daily 批次对已装的 0.0.57 根转红(实跑:cx-root-identity 12 项里 3 项 FAIL,dev-product 的 sh-version-text FAIL),所以用 `90856efe23` 把 release/candidate.tsv 还原成 0.0.57 公开记录,还原后 cx-root-identity 实跑 failed=0。v0.0.58 草稿与七资产保留,没有删。
- s9 用了一个隔离克隆 out/rel058/s9-clone(检出提交 X,install-ape 装入候选 APE,克隆里 plans/root-identity.md 与 ape-receipt.txt 因此被改写)。这与「本轮不用私有克隆」的说法有出入:s9 的结论必须绑定候选 APE 加提交 X,在共享检出里跑就得提前把未公开的候选装成本机根。init 相前两次失败是调用方式问题(克隆里没有 out 目录;RUN 目录必须在 ROOT/out 之下),不是 s9 结果。第一轮跑完 27 片后末步被 `REFUSE frozen-input-changed` 拒:克隆根下没有放候选的 unisacc.com.build.json(发布尾部代理的准备疏漏),check-release-usage 的 freeze 用例因此失败并把 o2、o3 两个输出文件留在克隆根目录,s9 如实判冻结输入已变。第一轮整轮作废不计;补上构造记录、把 o2、o3 移到 out/rel058/trash 后用新的 RUN 目录重跑第二轮,期间克隆工作区没有再出现新文件。check-release-usage 在失败路径上往 ROOT 写文件这一点本身是测试卫生缺口,未修。
- 发布后清理(release-sweep)没有做:没有发布,链输出与证据原样保留在 out/b1、out/b1-old、out/rel058、out/release-snap-9a34cb97b4。

## 已知缺口(如实)

- 候选未过 CI,未公开;Windows 两个目标的 stream 管道路径在这个候选上不可用,根因只有一条未验证的线索(long 宽度)。
- s9 commit 档 187/191,3 FAIL 加 1 UNKNOWN,四批未查明;release-decide 步 4 按此应记 FAIL(存档的 decide.txt 是 s9 出结果之前跑的,步 4 为 UNKNOWN)。
- ROUTES(`9b107888db`,对象与链接路线改走 cx.stream)与 link-DCE(`f9a79c9dbd`,链接期不可达函数剔除)是 0.0.59 的产品草稿,已在 main 上、也在提交 X 的树里,但不在构造源里,这个候选的 APE 不含它们。ROUTES 建在 cx.stream 之上,Windows 回归对它同样有效,未验。
- module-units 仍慢:40 单元映像整体冷编 48.8 秒,带缓存的 -o 也是 50.4 秒(单元缓存对多根整体编译无效);tlib 链接路线要等 link-DCE 与归档次序修正,不在本版。
- 整版超 2 小时铁律:首次起链到 pack 约 237 分钟,还没有算公开。
- LIMIT-90-TEMP 仍在生效,没有回收。
- release-oneshot 真实执行入口、assetguard 的代码路径、release-sweep 在真实发布上都还没有用过。
- g2、g3 两个 lnx 目标的 closure-prepare 整相耗时没有缩短,PHASE-YIELD 只是让它不再消耗 resume。
- 沿 0.0.57 说明、本版未处理也未复核:BASE6 字节 oracle 未重建;p3 的 release-ctor case 材料未重建;s9 release 档与 daily 档未跑;全域矩阵 release 档红格未回填;六个按目标独立可执行片未产出。
- 表中写未跑、未核实或 FAIL 的项,不称 QUALIFIED。

## 撤回与回滚

没有公开,所以没有要撤回的公开动作。现状的清理办法:`gh -R partnernetsoftware/unisacc release delete v0.0.58 --cleanup-tag` 删掉草稿(草稿没有 tag,--cleanup-tag 无副作用);候选表已还原;本机根没有动,不需要回滚。若将来公开后要撤回:`gh release edit v0.0.58 --draft=true`,彻底撤回再 `gh release delete v0.0.58 --cleanup-tag`,本机根用 install-ape 装回 v0.0.57 公开 APE c116a52ff62a427929334d728bcb7946860b120d3faebbb6ee0fdb2c3fa2d985。
