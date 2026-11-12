# unisacc 0.0.57 -- 发布说明(未实跑一律写未跑)

发布提交 X `3d04b5eb063a1d6f25593eaf66833be147c3141b`。构造源(候选记录 commit 列)`60acc817b86b0605016f38604ec0dbea5fac4d8d`,源 SID 25e41517(`25e41517592e5544b864241ad37d568fd459518bd49cd8bd77637a112bcef58d`),引导产品为 0.0.56 公开 APE 088a31c6。本版产品输入有改动,整链真实重建一轮;构造源之后 main 上只有账本、工具组 A2(factscount)、chainfix、REG-PREP 与候选表提交,都是工具与测试侧,不触产品输入,沿 0.0.53、0.0.55、0.0.56 先例「构造源与发布提交分离」。git 范围:`v0.0.56..3d04b5eb06`,`git log` 现算 14 条(含提交 X 与 0.0.56 公开后的两条收尾提交)。tag 所指提交见 git(提交 Y)。

## 是什么

面向 C99 的编译器,自己写出 ELF、Mach-O、PE,可编译自身;产品形态为单文件 `unisacc.com`。0.0.57 是一个工具面为主的版本:唯一一类产品改动是 basered(两处基线红转绿的卫生修复,不改编译行为);其余是第一波构造链、发布与 s9 工具(gen1split、g2g3split、chain-run、factscount、wfcheck、k7exec、s9split、t57-1、ib55-1)。本版首次用真实整链检验这些工具,结果有好有坏,见「整链、阻塞与人工介入全表」。上一发布版见 GitHub Release 0.0.56 页。

## 身份与证据

| 对象 | 身份 | 说明 |
|---|---|---|
| APE `unisacc.com` | c116a52ff62a427929334d728bcb7946860b120d3faebbb6ee0fdb2c3fa2d985,6648004 字节 | `--version` 实测 `unisacc 0.0.57`;release-ape smoke ok=5 failed=0;链 pack 相产出的 APE 为 088a31c6,6648004 字节,与 0.0.56 公开 APE 同 sha(见下一行说明) |
| 与 0.0.56 的字节关系 | 链产出的六片与 P4 包的 sha 与 0.0.56 逐项相同(lnx/x86_64 ef0e0a1a、lnx/arm64 e6e0174b、osx/x86_64 e4675046、osx/arm64 917ef911、win/x86_64 897b5823、win/arm64 44f51001、P4 包 56efea53) | basered 是模块搬移与常量引用,没有改变任何一片的产品字节;本版 APE 与 0.0.56 公开 APE 的差别只来自注入的版本号。也就是说,这一版在产品字节上等于 0.0.56 换了版本号,整链重建证明的是「这次源码改动不改变产物」 |
| 记录 `unisacc.com.build.json` | ddd4c5bcf66f9de415982f4bca68f5206fe31a53eee21ed85fb10bf354752e6e | SOURCES_SID VERIFIED 25e41517(identity-helper 对冻结快照现算) |
| `SHA256SUMS` | 48a91c8fb79033cca00f6c147e663de84e168365c1a94fd0cfd16ae63708dd20 | 本地自校 6/6 OK,草稿读回 6/6 OK,读回七件 sha 与本地逐项相同 |
| 其余资产 | pkg.bin 56efea53(与 0.0.55、0.0.56 同字节),slice-win-arm64 bc1c4e32,zip 8a16ed22 | 见 release/candidate.tsv |
| 链产物 | 本版整链重建一轮,构造源 60acc817b8;链内 compare:objects=same count=86,kernels=same count=2,slices=same count=6,product=same | 三个阻塞、人工介入见下节;不是无人值守 |
| s9 commit 档 | 公开时运行中,公开后(04:26 聚合)补记:候选 APE c116a52f 加提交 X,selected 154,PASS 150,FAIL 2,UNKNOWN 2,DEFERRED 0;qualification NOT_FULL(12 个 osx 批本机 NOT_APPLICABLE,含 osx-sentinel 六批)。FAIL 两批仍是 0.0.56 的那两批、同样的检查项:s9-nightly-run-pass(fake-run-pass、index-row)与 s9-nightly-run-fail(fail-run-fail、fail-index-report-not-dash);s9split 提交称已修,真实会话里没有转绿,原因未查明。UNKNOWN 两批是 55 秒看门狗超时:register-metadata-rollback 与 candidate-chain(后者在 0.0.56 的门内同样 killed-at-deadline);时限未放宽,不记 PASS,也未重跑。分相耗时:prep 23、init 219、steps 四次 174、202、173、14、aggregate(4 片加 merge 加一次读回)124 秒,每次调用都在 5 分钟内 | 新的分相调用(init、steps 重复、aggregate 分 4 片)在真实会话上本版首次使用;s9 不在公开的关键路径上;不称 QUALIFIED |
| s9 release / daily 档 | 本版未跑 | 0.0.54 release 档首跑 15 批全域矩阵 OPEN 格 FAIL 未变 |
| 原生 CI | 同候选运行 43094459755(检出提交 X 3d04b5eb06,ape_sha256 c116a52f,release_tag v0.0.57):QUALIFIED pass=42 fail=0 not_run=0 invalid=0,04:13 触发,04:16 结束。此前的首次运行 43093841811 为 FAIL pass=36 fail=1 not_run=0 invalid=5:原因是发布尾部在草稿里只传完 unisacc.com 就触发了 CI,fetch 作业早于 slice-win-arm64 上传,win/arm64 格报 `win/arm64 slice asset missing`;其余五个目标该次全过。七资产传齐后以同一提交、同一 APE sha、同一 tag 重新触发,得到上面的结论。两次运行的日志都留存(cc-work/rel057/ci-run1.log、ci-run.log) | 六目标原生单元格实跑;公开依据是第二次运行 |
| release-decide | 七步 PASS(1 freeze-sid、2 construct、3 ape-pack、5 layout-check、6 draft-assets、7 ci-qualified、8 readback),步 4 s9 UNKNOWN;QUALIFIED 行为 NOT_QUALIFIED unknown=1 fail=0(公开时);s9 出结果后步 4 改记 FAIL,重裁输出存 cc-work/rel057/decide2.txt | 八步原样输出存 cc-work/rel057/decide.txt |
| 六个按目标独立可执行片 | 本版未产出 | 候选表仍是七资产 |

## 行为变化(升级前请读)

产品输入只有一类改动 basered(`c2ef7067c6`),预期没有可见的编译行为变化:
(1) 通用字节帧模块 `frames`、`frameparts` 由 `cx/compiler/` 移到 `cx/runtime/`(frameparts 内容不变,frames 只改一行 use),`cx/runtime/holdframe.cx`、`src/drivestream.cx`、`src/driveunits.cx` 改 `use runtime.frames`;消掉 runtime -> compiler 的上行依赖边。
(2) `src/runcache.cx` 的源单位上限由裸写 128 改为引用 `compiler.cxmod` 的 `DV_MAXU`(唯一声明处不变,数值不变)。
(3) 0.0.56 的单元前端缓存行为不变:仍只在 lnx/x86_64 启用,`UNISACC_RUN_CACHE=0`、`UNISACC_UNIT_CACHE=0` 开关不变。
「无行为变化」有字节证据:整链产出的六片与 P4 包的 sha 与 0.0.56 逐项相同,本版 APE 与 0.0.56 的差别只有注入的版本号(见上表)。升级 0.0.56 到 0.0.57 得到的是同一个编译器加新版本号。

## 本版已落地(`v0.0.56..3d04b5eb06`)

产品输入(一类改动):
- `c2ef7067c6` basered 基线红清零:见上节。改前干净 main 实跑 cx-layers FAIL(`UP cx/runtime/holdframe.cx:5 use compiler.frames`)、module-units FAIL(`src/runcache.cx` 裸上限);改后门克隆实跑 cx-layers PASS(33.6s)、module-units PASS(37.3s,首次冷跑 53.9s 被杀未计,热跑通过),shared-frames PASS,棘轮 13/0。这两批是 0.0.56 说明里登记的两处基线红,本版转绿。

构造链工具(第一波,非产品输入):
- `16ba637b0c` gen1split:GEN1 Linux 对象相与链接相分开(`entry-build --objects-only` 后接同输入 `--link-only`,新增 `gen1-lnx-*-objects` 检查点)。真实链结果:gen1-lnx 两目标四相 61、50、61、50 秒,均一次通过,不再需要 0.0.56 那样的手工 `--link-only`(0.0.56 两轮共 4 次)。
- 同提交 g2g3split:G2、G3 的 Linux 对象改为批量键(44 根一次调用、动态闭包每批至多 256 键)加独立链接。真实链结果:不可用,见下节 BLOCKER-3。
- 同提交 chain-run:新工具 `tools/chain-run.cx`,按 `build/chain-phases.tsv` 分块调用 candidate-chain,只对超时或被杀续跑。真实链结果:零接触 FAIL,见下节 BLOCKER-1、BLOCKER-2。
- `c27fea81ed` wfcheck:whole-front 整程序展开预检 `tools/check-whole-front-expand.cx`(0.0.56 BLOCKER-1 那类跨单元重复定义与分片漏单元的门内左移检查)。冻结前在 main 60acc817b8 实跑 units=86 partition=PASS;链内 gen1-osx 与 gen1-win 四目标的 front0、front1、front2、merge 均一次通过。
- `67fb47417b` factscount(组 A2):链事实表 `build/chain-facts.tsv` 生成 `cxlib.chainfacts`,根数、相数单源,front 分片按 part.tsv total 推导。冻结之后才落 main,不在构造源里,本版整链没有用到它;在提交 X 的树里。
- `8ff22b3446` chainfix(起链之后才落 main,不在本候选构造源内,本版整链没有用到):snap 的半成品快照改为私有归档加锁并记录 owner 身份,被杀后可续跑并清理同身份残留;外来或旧的无 owner 半成品一律拒绝,不盲删;chain-run 前置检查拒绝仓外 OUT 与逃逸符号链接;55 秒时限与白名单不放宽。对应本版 RESUME-1、MANUAL-1、BLOCKER-1 三处;8 个恢复夹具与 19 项 chain-run 检查实跑 PASS,真实链未验。BLOCKER-2(逐相 55s 界偏紧)与 BLOCKER-3(g2g3split 批量键)不在这次修复范围内。
- `40f5185de1` REG-PREP(同样在构造源之外):cx-register、slice-freeze-tool、slice-inventory、chain-phases-gen 四组检查迁到 toolprep/runnerprep 原生准备,显式输出目录,不再用 /tmp 夹具,54/55/45 秒时限不变;slice-freeze-tool 的根因是 `pp_copy_product` 在复制完全部成员之前就发布了构造记录,改为先校验 SHA。提交说明记实跑 35/0、22/0、9/0 与 inventory 各片全过。集成事实如实记:前一提交 `8ff22b3446` 误带入了本补丁的 batches.tsv、coverage、docs 与 tools-compile-sources 行(regprep-dir 材料声明),由 `40f5185de1` 补齐对应的检查与工具源;两个提交之间的 main 处于表行先于实现的状态。

发布与 s9 工具:
- `9a163d2c17` k7exec:`release-oneshot` 的明确授权执行入口(`--execute STATE CONFIG --authorize-external-operations`)与发布前置准备(`--prepare`);修 release-run 的 `--resume` 与 release-layout 派生摘要生成。本版发布尾部没有用执行入口,仍按 0.0.56 的办法逐步执行;`release-run --pack` 用的是修后的版本。
- `9486998577` s9split:`s9-nightly` 拆 `--phase init|steps|aggregate`,`s9-shards aggregate` 新增 `--part K N` 与 `--merge N` 分片聚合;check-s9-nightly-run 并行预热,针对 0.0.56 s9 提交档的两个 FAIL(s9-nightly-run-pass、s9-nightly-run-fail)。
- `0feddae3be` t57-1:运行器原生预编译库 `tools/cxlib/runnerprep.cx`(尚未接线 affected)与冷编译量表 `tools/coldcompile.cx`;ib55-1:`tools/cxlib/toolprep.cx`(检查内层时限前先把工具编成原生映像),`tests/snapshots-cx.cx` 改用它。

批次档位变化(如实):四个批次由 commit 档改为 daily 档,断言、材料与 55 秒界不变,但不再计入 commit 档覆盖:release-exec、release-prepare(`9a163d2c17`,需要登记的原生工具材料)、chain-facts、chain-partition(`67fb47417b`)。

记录:
- `f6f56097a2`、`41c71d52e4` 0.0.56 公开后的收尾与 s9 补记;`60acc817b8` 第一波账本 11 行;`7d27bb2053` FREEZE-057。两条账本提交未过集成门(账本行破坏门的表模式,由集成者从干净克隆直接提交),如实记。

## 整链、阻塞与人工介入全表

本版整链一轮,构造源始终是 60acc817b8,快照与产品输入未动。链 01:07:33 起,04:07:04 pack 完成,墙钟约 180 分钟(其中 BLOCKER-1 之前的首段约 7 分钟作废重起)。证据:`cc-work/evidence-057/MANUAL.tsv`、`steps.tsv`、`chain-run-*.log`。

| 编号 | 相位 | 原因 | 做法与核验 |
|---|---|---|---|
| RESUME-1 | snap | chain-run 首次调用在 snap 相 55s 子调用超时(rc=124)且有残留进程,报 BLOCKER surviving-processes,不自动续跑。0.0.56 snap 实测 58s,本就贴 55s 界 | 核实残留进程已全部退出后,人工重新调用同一条 chain-run 命令 |
| MANUAL-1 | snap | 续跑被拒:snapshot exists without a matching rev marker。snap 被杀后留下无 rev 标记的半成品快照,工具不自清 | 半成品目录移到 cc-work/trash,BUILD_REV 不动,重调 |
| BLOCKER-1 / MANUAL-2 | jobs-tool-0-finish | chain-run 逐相调用下,续跑触发 candidate-chain 清理,清理函数拒绝仓外 OUT(不在白名单根之下),首败即停。0.0.56 同样的 OUT 位置没触发,是因为 start 与 finish 在同一次调用里 | 不改工具、不放宽白名单;链输出改到白名单内的 `r057/out/chain057` 从头重起(已有带 rev 标记的快照复用) |
| BLOCKER-2 / MANUAL-3 | jobs-gen-16-start | chain-run 的 55s 逐相子调用界比 candidate-chain 实际单相耗时紧,超时带残留即拒续 | 按预案第三级后备,退回 0.0.56 的逐相 `--until` 办法(每次调用外层 300 秒保险);chain-run 零接触记 FAIL。此后每段调用都算人工推进 |
| (无) | gen1 六目标 | 无人工 | 01:52 至 02:06 一次通过;gen1-lnx 不再需要手工链接 |
| BLOCKER-3 | g2-lnx-x86_64-obj-3 | g2g3split 把 `--until` 截到的全部根键放进一个 55s 的 chain-step 子调用里逐键串行编:首键含一次性准备 44.4s,其后每键约 8s,第 3 键写到一半被杀。原命令重调,被 `REFUSE resume identity` 拒:键中被杀后目录比相位记录多出半成品,工具的续跑对「键中被杀」不成立。0.0.56 是每键一次子调用,没有这个问题。按这个设计,44 根、闭包每批 256 键、g3 的 44 根都必然超界 | 定性为构造工具缺陷,非产品(已完成对象正常,每键耗时与 0.0.56 相当) |
| MANUAL-4 | g2-lnx-x86_64-obj-3 | BLOCKER-3 的状态修复 | 半成品 stages.o 移到 cc-work/trash,receipt.txt 去掉末行,回到上一键完成态;按工具的目录身份规则现算,与相位记录的 artifact_sha 相同(73eca3d6);续跑时工具自己核验并接受。修前副本留证 |
| MANUAL-5 | g2-lnx-x86_64-obj-3 起 | BLOCKER-3 的构造工具修复,非产品绕行 | 只改冻结克隆里的 `tools/candidate-chain.cx` 两处:待编的根键与闭包键每个子调用最多 4 个,已完成的键仍一批复核;每个子调用的 55s 界不变,键序、覆盖、逐键检查点不变。修前副本与补丁留证(`cc-work/rel057/manual5-candidate-chain.patch`);正式修复尚未过门落 main |
| MANUAL-6 | g3-lnx-arm64 根对象相(预防) | 无失败。g3-lnx-x86_64 根对象相实测 369 秒,每次调用的续跑复核开销已涨到 75 至 115 秒;估 arm64 会碰到原定 540 秒外层保险,外层一杀就是键中被杀 | 只结束推进脚本外壳,当前子进程自然结束后重起,该相外层保险改 900 秒(子调用仍 55s 界);实测 418 秒 |
| MANUAL-7、MANUAL-8 | g3-lnx-arm64 闭包起、kernels-g2 起(调度调整) | 无失败。每次 candidate-chain 调用的续跑复核开销涨到 100 至 170 秒(链接本身只有约 20 秒),按每相一次调用,pack 会贴 04:30 止损线 | 同办法重起两次,最终把 kernels-g2 之后的全部相位(kernels-g3、osx 与 win 的 g2、g3 八相、compare、pack)并成一次调用;各相位自身的子进程界、检查点与首败即停不变。代价是这次调用 1075 秒,steps.tsv 没有这些相位的逐相行,逐相耗时以 checkpoint.tsv 为准 |
| MANUAL-8a | kernels-g2(操作失误) | 发布尾部代理的错误:MANUAL-8 的重起清单里带了已完成的 kernels-g2,用默认 300 秒外层保险;工具对已完成相位不在 `--until` 处退出,继续跑到 g2-osx-x86_64 中途被外层杀(rc=124) | 无残留进程,已写检查点保留;直接续跑,工具接受,没有留下需要修的现场。不是链或产品失败 |

人工介入合计(MANUAL.tsv):RESUME-1、MANUAL-1 至 MANUAL-8a 共 10 行,其中阻塞 3 个(BLOCKER-1、2、3),状态修复 2 次(MANUAL-1、MANUAL-4),冻结克隆内改构造工具 1 次(MANUAL-5),调度调整 3 次,操作失误 1 次。

如实说明:
- chain-run 的零接触目标本版记 FAIL:真实链上它在第 2 个和第 4 个调用就各阻塞一次,之后整条链是用 0.0.56 的逐相脚本推过去的。
- 链产物是在同一冻结克隆里先后用两个版本的 `tools/candidate-chain.cx` 产出的(原版到 gen1 结束;MANUAL-5 版用于 g2 之后)。更换只改构造工具的调用分批,不改快照与产品输入,但这不是「一条命令无人值守」的链。
- g3 的两个 Linux 目标的根对象相没有 `--until` 截断点,一次 candidate-chain 调用要串行编 44 键,外层保险放到 540 秒(每个 chain-step 子调用仍是 55s 界);实测 x86_64 369 秒、arm64 418 秒。加上最后并成一次的 1075 秒调用,本链有三次单次操作超 5 分钟,如实记。
- MANUAL-5 改的分批不是免费的:g3 的 Linux 对象由 0.0.56 的整片一次调用(每目标约 110 秒)变成逐键串行,g3 两个 Linux 目标本版用了约 50 分钟(02:50 至 03:40);这是 g2g3split 的设计代价,不是 MANUAL-5 引入的,MANUAL-5 只是让它能跑完。
- 冻结前的预演没有跑:`chain-run --rehearse` 记 NOT_RUN(工具以缺少显式的已备构造器、对象与包而拒,rc=2;所需的 0.0.56 链产物此前被清走)。替代证据只有 whole-front-expand(units=86 partition=PASS)与两处基线红转绿的实跑,不覆盖 e2/e3、对象与 front0 实编。BLOCKER-1、2、3 都是预演本可以提前暴露的问题。为此本版起新增一条规则:发布后不清理本版的冻结克隆与链输出(`cc-work/r057`、`r057/out/chain057`),留给下一版预演用。

## 已知缺口(如实)

- 整版超 2 小时铁律:冻结 01:07:33 起链,04:07:04 才到 pack(约 180 分钟),公开在其后,整版约 3 个半小时。事先在 FREEZE-057 写的预计是 125 至 135 分钟,实际多出的约 55 分钟来自三个阻塞与 g2g3split 的逐键串行;整改任务 K9(chain-phases 依赖列与按目标并行)到期 0.0.58,登记在 agentwork/phase-times.tsv。
- chain-run 真实链不可用(BLOCKER-1、BLOCKER-2),g2g3split 的批量键真实链不可用(BLOCKER-3),kill 后续跑在 snap 相与 g2 对象相都会留下工具自己拒绝的现场。三项都只有冻结克隆内的绕行或就地修复,main 上的正式修复未落。
- s9 commit 档在公开时尚未出结果,公开不以 s9 为依据;公开后补记为 150/154,2 FAIL(两个夜跑入口自检批,与 0.0.56 相同,s9split 的修复在真实会话里未生效)加 2 UNKNOWN(55 秒超时),四批都未查明、未修、未重跑;release-decide 步 4 改记 FAIL,NOT_QUALIFIED unknown=0 fail=1。s9 克隆里 install-ape 重写了 plans/root-identity.md 的身份块,该克隆工作区与提交 X 相差这一份文档。
- 既有红未在本版处理:slice-inventory 冷跑 killed-at-deadline(53.9s,干净 main 同样被杀,热跑 24.8s 通过;时限未放宽,不记 PASS);osx-sentinel 六个批次需要 osx/arm64 原生宿主,本机 RUN_REFUSED,只能由原生 CI 覆盖的部分覆盖。
- 四个批次由 commit 档改 daily 档(release-exec、release-prepare、chain-facts、chain-partition),commit 档覆盖因此减少;release-exec、release-prepare、release-oneshot、release-run 批次因缺材料 DEFERRED,未实跑,不记 PASS。
- 组 A2(factscount)提交说明里的 74 批汇总由作者与协调者提供,门内未逐项复核:32 PASS,34 项缺材料 NOT_RUN,6 项 osx 原生 RUN_REFUSED,slice-inventory 冷跑 TIMEOUT;该提交以 LAND_SKIP_OK 显式放行落地。
- 第一波各工具在各自提交里的证据多为轻夹具或假链(提交说明逐条写了未实跑范围);本版整链只真实检验了 gen1split(可用)、wfcheck 的对象(链内 front 相通过)、chain-run 与 g2g3split(不可用)。k7exec 的执行入口、factscount、t57-1 的 runnerprep 本版没有真实使用。
- release-oneshot 的真实执行入口未使用,本版发布尾部仍按手册逐步执行。
- 六个按目标独立可执行片本版未产出、未上传;候选表仍是七资产。
- 单元前端缓存仍只在 lnx/x86_64 启用;链内强制 `UNISACC_UNIT_CACHE=0`,构造过程不使用它。
- 沿 0.0.56 说明、本版未处理也未复核:BASE6 字节 oracle 未重建;p3 的 28 个 release-ctor case 材料未重建(DEFERRED),未记 PASS;s9 release 档与 daily 档未跑;全域矩阵 release 档 15 红未回填。
- 两代不动点、跨平台原生资格、s9 与 CI 结论以上表实填为准;表中写未跑或运行中的项,不称 QUALIFIED。

## 撤回办法(公开前写定)

`gh -R partnernetsoftware/unisacc release edit v0.0.57 --draft=true` 撤为草稿;`gh -R partnernetsoftware/unisacc release edit v0.0.56 --latest` 恢复上一版 Latest;tag 不删。彻底撤回才 `gh release delete v0.0.57 --cleanup-tag` 并在账本记 REL057-WITHDRAWN。本机根回滚:install-ape 装回 v0.0.56 公开 APE 088a31c6e73a36b4e66568b76a103670ae5b40a3091c58c3aa6c3bd8ea1b832b。
