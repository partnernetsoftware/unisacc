# 0.0.68 发布核对

## 核对清单
输入身份: v0.0.68 的 f77ebf7b714af8547b0c73277fa5e692e6eacd80。以下开发树证据不迁作 fable 新冻结根或公开候选的资格证据。

### 公开硬门:方案 A 第4步

(1) tools/cxlib/releaseprepare.cx:206 已把 chain_out 作为 --chain-out 传入 release-run --pack；不是 tools/releaseprepare.cx。
(2) tools/release-run.cx:143-190 在 pack 前要求 checkpoint.tsv 的 compare 为 DONE；chain-run.tsv 最后 compare ATTEMPT 为 rc=0、residue=0、memo=off 且 bound 正数；读取该 attempt 绑定的 chain-run-N.stdout，不猜 compare.log。
(3) 日志要求 objects、kernels、slices、product 四类 same；kernels count=2、slices count=6。再对 g2/g3 六目标非空实际切片逐个 SHA256 相等。
(4) 当前 release/candidate.tsv、evidence.tsv、decision.txt 仍绑定 0.0.67，其中 s9 为 UNKNOWN，裁决 NOT_QUALIFIED。不得把这些旧收据改称 0.0.68 的资格。0.0.68 的真实整链目录、compare 成功尝试与六目标 SHA 证据尚缺；不能公开宣称硬门已实跑通过。

### 冻结门复跑清单

(1) 等 fable 四文件提交及新根完成后固定新提交、candidate/SID/harness、材料映射、工具映像 SHA；不复用旧根成功收据。
(2) main 集成后复核 1-rev/1-branch；版本分支的 SKIPPED-branch 不是通过。
(3) derive 按 exec/facts/derive.tsv 全10项分片只读检查；单次55秒，只有全部同输入无漂移/拒绝才记完成。
(4) 13项 ratchets、cxmod-tri、cx0-extract、完整P1扫描、candidate-chain、受影响合同分别保留原判据；P1 --fast 的 SKIPPED 不计通过。
(5) tlib-frozen 和 bootstrap-0.0.59 多单元编译/链接/执行；不能仅已装产品 -c。
(6) 真实在途进程及环境卫生；链输入不设 UNISACC_RUN_CACHE/UNISACC_CACHE_DIR；真实配方与开链记录由主调度确认。
(7) 注意现有 freeze-gate/chain-ready 内部声明95/80秒子闸，不能直接整体调用后宣称符合55秒操作上限；应经 devsvc 独立有界节点复核并报告超时，不放宽时限。
(8) fable 的137片 flow --par8 每节点55秒，后台作业终态、每片实际结果与既有聚合裁决都必须取回；STEP_FINISHED/节点启动成功不等于资格通过。

### ext.net 与派生状态
(1) ext.net 表版本为0.1.9；layer-api 本轮实跑 OK。
(2) derive 首次整体55秒超时；拆片后十项中九项 OK，tool-index 为 DRIFT。当前索引仍记 nightly736行/check526行，而所读源码为1021/621行。fable 稳定提交后重算派生索引并在新根复核；不抢改其四文件。
(3) 首轮六网络批 dcheck 外层59.006秒超时，无 CHECK 总结，未记PASS；确认无遗留检查进程后，仅补跑尚无结果项。resolve 单批 pass=1，另五批并行 pass=5、fail=0、deferred=0、not_run=0，合计六批实跑PASS。addr3207ms、tcp3301ms、sock2911ms、unix3471ms、udp2927ms、resolve4091ms。
(4) resolve证据:/home/devali/.cache/unisacc/devsvc/045c5a383068df3b/jobs/21174432513-4128839/out；另五批证据:/home/devali/.cache/unisacc/devsvc/045c5a383068df3b/jobs/21174544530-4157587/out。这是f77开发树本机lnx/x86_64合同核对，不替代新根或跨平台资格。
(5) 主调度已将ext-net-sock预算由58秒收紧为55秒；本轮只读核对tests/batches.tsv:1082及tests/manifests/ext-net-sock.tsv:1均为55，与flow节点上限一致，该声明不相容项关闭。manifest中的58 asserts是断言数量，不是预算，不应改动。主调度报告reg-manifest 144项无问题，此处注明来源，不冒称本窗口重跑。既有六批真实通过证据保留，不迁作新候选资格。

### 两个冻结根保留判断

(1) /tmp/v68-s9-daily-0087af5c9e 为170M；/tmp/v68-s9-daily-f77ebf7b71 为174M。
(2) 本轮 pgrep 仅命中并行du及其服务包装，没有冻结候选作业。当前两根均保留，f77保留真实137片PENDING与NOT_QUALIFIED现场。
(3) fable 新根通过初始化/身份核对后，保留新根及 f77；删除最旧0087前重新核真实进程与du，并先提取必要失败证据。当前未删除任何根。

### 本轮证据入口

(1) 原daily aggregate: /tmp/v68-s9-daily-f77ebf7b71/out/daily/aggregate.out。
(2) 派生中段: /tmp/devsvc-flow-21174278693-4101593/full.out。
(3) product-map: /tmp/devsvc-flow-21174278693-4101594/full.out。
(4) tool-index/cx0: /tmp/devsvc-flow-21174278693-4101597/full.out。
(5) 首轮derive: /tmp/devsvc-flow-21174156531-4077180/full.out。
(6) 网络验收: /home/devali/.cache/unisacc/devsvc/045c5a383068df3b/jobs/21174201509-4087176/out。

## daily 首次真跑聚合分类

来源:fable-agg,2026-11-15 23:14 至 23:40。只读分析,未改产品源、未改冻结输入。输入身份:ROOT=/tmp/v68-s9-daily-5a071a43df,RUN=ROOT/out/daily,candidate=a99ea71d,registry=8afd1928,sid=5e012a24,harness=e38bb18b;聚合全文 /tmp/devsvc-flow-21183022418-1125699/full.out。逐批归并表(1159 行,含原因列)在 $CLAUDE_JOB_DIR/tmp/fagg/per-batch.tsv,由 join.cx 从聚合行、plan.tsv、各批 receipt、片日志、片 .done 归并得出;逐片墙钟表 ov.tsv 由 ov.cx 得出。

### 结论

(1) NOT_QUALIFIED 的主因不是产品缺陷,而是 harness 的时间窗:743 个 UNKNOWN 里 540 个(73%)是时间窗类(含身份测量失败 18 个),175 个是材料未备,28 个是整片被单个批连带拒绝。
(2) 124 个 FAIL 里 94 个同一根因(tools-compile 审核清单缺 107 个新工具),17 个是 full-* 域闭合检查按设计报 OPEN,其余 13 个里 2 个是争抢假失败,2 个是冻结根缺伴随文件,9 个需要逐个修或定因。
(3) UNKNOWN 掩盖着真实 FAIL:在冻结根外输出目录单独复跑 38 个 UNKNOWN 批,32 个 PASS,6 个 FAIL。所以"单跑能过"只对约八成成立,重跑后 FAIL 数会上升,不能按 124 估工作量。
(4) 本 RUN 无法靠重试转绿:FAIL 在会话内粘滞(allsession.cx:595 注释),含 FAIL 的片 retry 会被拒(s9-shards.cx:155 sticky-fail),且修复会改变 sid,必须新冻结根、新 RUN。

### 终态定义(源码行)

| 终态 | 定义 | 来源 |
|---|---|---|
| PASS | 末次有效 receipt 为 PASS:bound_rc 等于子进程退出码 0,末行为期望的 BATCH_RUN 通过行 | tests/cxlib/allsession.cx:372、551 |
| FAIL | 任一 receipt 判 FAIL(退出码 1 且非超时,或通过行不符、输入或材料在批内被改);会话内粘滞 | allsession.cx:347-379、595 |
| UNKNOWN | 有尝试但无 PASS 也无 FAIL:bound_rc 142 记 watchdog-deadline;身份在波内变化;退出码 2 runner-refused;退出码 3 批自身超时;波未收尾;片被拒未起波 | allsession.cx:345-378、589-590、551 |
| REFUSED | 聚合复核 receipt 失败(as_check_receipt 非零),会话内粘滞 | allsession.cx:592、551 |
| MISSING | 计划内但零次尝试(本次为 0) | allsession.cx:551 |
| NOT_APPLICABLE | 批的 platform 不属本机(lnx/x86_64)可跑范围,聚合时单列不入计数 | allsession.cx:555,s9-shards.cx:88 |

片 .done 四列:起始纳秒、结束纳秒、退出码、摘要;只有退出码 0 或 1 且墙钟不超 s9.slice(300 秒)才算封存(s9-shards.cx:171)。本次 137 片退出码分布:0 有 17 片,1 有 72 片,2 有 27 片,3 有 21 片。片日志里的 S9 NOT_AVAIL 表示该批声明的材料不在映射里,被移出本片(s9-shards.cx:160-178)。

### 时间窗机制(主因的证据)

(1) 每片是一条 tools/bound 55 的 all-waves 命令(build/limits.tsv s9.cmd=55)。波窗 = 截止 - 当前 - 12 秒测后保留(tools/all-waves.cx:376),不足 3 秒即拒 insufficient-wave-window。批的实际可用时间是 min(budget_seconds, 波窗),不是登记的 55 秒。
(2) 实测 96 片有波:片墙钟 18.7 至 46.7 秒(1 片 79.5),波前后固定开销 15.0 至 37.0 秒,留给波的只有 0.9 至 28.2 秒(slice-0-17 例外,45.2 秒)。336 个 watchdog-deadline 批的 child_seconds 分布在 3 至 28 秒(预算 55 的批占 321 个;另有 6 批在墙钟 79.5 秒的 slice-0-17 里为 44 秒),除 2 个预算 20 的批跑满 20 秒外,都不是跑满自身预算被杀。
(3) 代表:slice-1-93 十批(ctestsuite、random-expr、tools-compile-000 等)片墙钟 45.9 秒,波只有 14.7 秒,十批全部 child_seconds=12 被杀;slice-0-0 单批 ucn-identities 开销 26.1 秒,16 秒被杀,而单独复跑 10.4 秒 PASS。
(4) 开销最小也有 15 秒(与 prd.md 候选池 Q 的"每片固定开销 15.0 秒"一致),所以即使零争抢,波窗上限约 28 秒:登记预算 55 而实际需要超过约 28 秒的批在现结构下永远过不了,这是 prd.md 候选池 S 的 F5(贴线批拆分)。

### UNKNOWN 743 归因

| 类 | 批数 | 原因首行 | 代表 | 性质 |
|---|---|---|---|---|
| U1 波窗被挤,批被看门狗杀 | 336 | receipt reason=watchdog-deadline,bound_rc=142 | ctestsuite、ucn-identities、modelcache-budget-lru | 时间窗(harness 结构加并发) |
| U2 波未起:窗口不足 | 91 | REFUSE insufficient-wave-window | slice-1-53 十批(cx-driver-route、cx0-unit 等) | 时间窗 |
| U3 片超时或残留 | 55 | S9 UNKNOWN reason=operation-deadline(51)、descendant-residue(4) | slice-1-54(identity、pp-cli-*)| 时间窗 |
| U4 all-waves 被 bound 杀 | 40 | bound: timeout after 55 seconds(28)、hard deadline reached unreaped child(12) | slice-0-1 cx-bounded | 时间窗 |
| U5 身份测量失败 | 18 | REFUSE live-identity sid-measurement(10)、identity-changed-during-wave(8) | slice-1-103 | 时间窗(sid 测量子进程失败,未单独定因) |
| U6 一批拖垮整片:cx-plan | 10 | REFUSE cx-plan tools/lab-loader/check-utf8.cx | slice-1-84 | 确定性缺陷(一个批的入口计划被拒,同片 9 批连带) |
| U7 一批拖垮整片:需他机 | 18 | REFUSE batch-needs-another-native-host(runner-platform、macos-signing-interface、osx-lane-queue、prune-route-placement-oracle) | 4 片 | 确定性缺陷(候选池 S 的 F3:平台不适用批不应入计划) |
| U8 材料缺失,有 NOT_AVAIL 行 | 119 | S9 NOT_AVAIL material=old-product(30)、release-ctor-location(28)、ri-flow-runtime(11)、ref(10)、kernel-route(10)、ri-cli(8)、ri-chain(7)、其余 15 种各 1 至 2 | host-cell-binding 等 | 材料未备(候选池 S 的 F4) |
| U9 材料缺失,片内静默移出 | 56 | 片退出码 0 或 1,片日志无该批任何行;登记材料 loader6(9)、streams(8)、p3-pack-ctx(6)、ri-flow-runtime(4)、ref(4)、o1-run(4)、o2-run(3)等 | loader-target-image-osx-arm64、ri-capture | 材料未备;另有可观测性缺口(无行可查) |

合计 336+91+55+40+18+10+18+119+56=743。按层:U1 为 interfaces 246、contract 55、negative 15、product 9、platform 7、properties 4;U2 至 U7(不含 8 个有 receipt 的身份变化批)为 interfaces 124、contract 80、negative 10、library 7、product 2、properties 1;U8 加 U9 为 contract 114、negative 30、interfaces 20、properties 9、library 2。按档:commit 239、daily 423、release 81。

### REFUSED 49 归因

全部落在 6 片(slice-0-16、1-88、1-92、1-103、1-104、1-118),这些片的 ALL_WAVE 行为 identity=CHANGED changed=after-measurement-failed,片日志首行 all-waves: identity changed during wave: after-measurement-failed sid-measurement。35 批有 receipt(reason=identity-changed-during-wave),14 批无 receipt(host-cell、illegal-action、loader-* 6 批、run-comm、slice-inventory-* 5 批)。性质:波后身份测量子进程失败(allsession.cx:154、434),不是候选真的变了(candidate_before 与 candidate_after 在同会话其他片一致);未单独复现该测量失败,定为时间窗类、待证。按层:interfaces 39、product 5、contract 4、negative 1;全部 daily 档。

### FAIL 124 归因

| 类 | 批数 | 证据行 | 性质 | 归属 |
|---|---|---|---|---|
| F1 tools-compile-NNN | 94 | tools-compile: REFUSE inventory differs from reviewed registry;每批 107 行 REFUSE added tools/...(devsvc、flow、rel、s9-root 等新工具);tests/tools-compile.cx:268 | 真实检查缺陷:tests/tools-compile-sources.tsv 未登记新工具;主干 HEAD 8e280aaf83 同样缺 | devsvc 车道加工具登记 |
| F2 full-* 域闭合 | 17 | FULL_DOMAIN ... OPEN,如 stream-e2 required=69 satisfied=64 open=5,hosts required=15 satisfied=3 open=12;OPEN 项含 NEEDS_FOREIGN_HOST 与 BLOCKED 条目 | 按设计的未闭合清单,release 档;不是回归 | 各域 |
| F3 冻结根缺伴随文件 | 2 | cx-root-identity: FAIL build-record-present;no-out-snap: product companion missing .../unisacc.com.build.json(共享检出有此文件,冻结根没有) | 夹具问题:建冻结根时未带 unisacc.com.build.json;单跑仍 FAIL | s9 根构造 |
| F4 争抢假失败 | 2 | encoder-osx-bundle、arm-encoder-osx-dynamic-bundle: bundle fixture preparation ... TimeoutExpired | 单跑 encoder-osx-bundle 1.1 秒 PASS | 无需修批;随并发整改消失 |
| F5 pins | 1 | FAIL UNDECLARED_PIN tools/check-devsvc-flow.cx:118、tools/check-relstore.cx:65、66 | 真实检查缺陷:新检查里的占位哈希未申报 | devsvc 车道 |
| F6 chain-argv-check 两批(commit 档) | 2 | REFUSE chain-argv-check line=3001、3230 unsupported array construction or missing terminator;real-source-positive FAIL | 真实检查缺陷:检查器认不得被检源的新写法 | 工具 |
| F7 graph-anchor 两批(commit 档) | 2 | route-real want route_mismatch=0 got 17;MUTATION_UNSUPPORTED_OR_INVALID | 真实检查漂移,待定因 | 工具 |
| F8 run-reserve | 1 | FAIL run-reserve.low-named-refusal(期望 stderr 含 run: out of memory,tests/run-reserve.cx:54);单跑仍 FAIL | 产品或环境,待定因;单跑的 compile-as-limit 同名断言也 FAIL | 产品 |
| F9 capsule-export | 1 | CHECK_CAPSULE_ZIP failed=115;cx: module sys.fs has two lexical entries: release/ci/sys/fs.cx and cx/sys/fs.cx | 待定因(release/ci/sys 是指向 cx/sys 的符号链接,模块解析把它当成第二入口) | 工具或 cx 解析 |
| F10 watch-bundle | 1 | PermissionError: [Errno 13] Permission denied: ROOT/tools/check-watch-bundle.cx(冻结根文件权限 0600) | 夹具或权限,待定因 | 工具 |
| F11 stream-prune-cases | 1 | prune-cases: REFUSE classic sources errno=2(tools/prune-cases-gen.cx:35 读 src/tapeprune.c 与 src/tapescan.h) | 真实检查缺陷:src/tapeprune.c 在冻结根与主干都不存在,版本树里只有 archive/reference-c/src/tapeprune.c;检查读的是已归档的旧路径 | 流 |

合计 94+17+2+2+1+2+2+1+1+1+1=124。tools-compile-093 与 stream-prune-cases 的 receipt 在 wave-2,其余在 wave-1。按档:daily 103、release 17、commit 4。

### 单独复跑记录(摸底,非资格证据)

做法:tools/bound 55 加本 RUN 的 executor/run-batch,ROOT 不变,输出父目录 /tmp/unisacc-affected-fagg-a 与 -b,并行不超过 4,机器负载约 2 至 3。原计划对 383 个去重来源的 UNKNOWN 批摸底,跑到 31 个时发现副作用后停止(见下)。

| 批 | 聚合态与原因 | 单跑结果 | 单跑墙钟 |
|---|---|---|---|
| ucn-identities(commit) | UNKNOWN watchdog 16 秒 | PASS | 10.4 秒 |
| ctestsuite | UNKNOWN watchdog 12 秒 | PASS | 11.4 秒 |
| modelcache-budget-lru | UNKNOWN watchdog 22 秒 | PASS | 8.0 秒 |
| cx-driver-route | UNKNOWN 窗口不足未起 | PASS | 2.4 秒 |
| cx-bounded(commit) | UNKNOWN all-waves 被杀 | PASS | 1.5 秒 |
| random-expr | UNKNOWN watchdog 12 秒 | FAIL(CHECK_RANDOM_EXPR status=FAIL) | 2.0 秒 |
| slice-migration-gen | UNKNOWN watchdog 12 秒 | FAIL(check-passes-81-units) | 7.1 秒 |
| tools-compile-000 | UNKNOWN watchdog 12 秒 | FAIL(同 F1) | 0.6 秒 |
| source-bom | UNKNOWN watchdog 6 秒 | FAIL(C-BOM/tape/byte-equal 等) | 未记 |
| compile-as-limit | UNKNOWN watchdog 21 秒 | FAIL(c/S/b-low-named-refusal) | 未记 |
| whole-o-deadline | UNKNOWN watchdog 5 秒 | FAIL(同 F3:缺 unisacc.com.build.json) | 未记 |
| 另 27 批(RESULTS.tsv) | UNKNOWN 各时间窗类 | PASS | 见 RESULTS.tsv |
| run-reserve、cx-root-identity、tools-compile-007 | FAIL | FAIL(复现) | 9.6、7.9、0.7 秒 |
| encoder-osx-bundle | FAIL | PASS | 1.1 秒 |

小结:38 个 UNKNOWN 批单跑 32 PASS、6 FAIL;4 个 FAIL 批单跑 3 个复现、1 个转 PASS。逐批退出码与起止时间在 /tmp/unisacc-affected-fagg-b/RESULTS.tsv(31 行)。

副作用如实记录:批 streams-material(tests/check-streams-material.cx)会往 ROOT/out/streams-material-PID 写目录且正常退出也不清。门跑自己留下了 out/streams-material-983116,我的摸底又留下 out/streams-material-1184114(23:22)。该目录在 out/ 下,不属冻结输入(git status 仍只有 ?? out/),不在 RUN 目录内;我没有删除它。发现后已按真实 PID(1178652、1178653)停掉摸底循环,在途一组在 57 秒上限内自然收尾。删除与否由集成者决定;这同时是一条批次缺陷(测试往根内写且不清)。

### 修复与重跑计划

阻塞 0.0.68(daily 档必须跑通):

| 序 | 修复 | 收益(批) | 估时 | 说明 |
|---|---|---|---|---|
| 1 | 降片内并发与片间并发:S9_WIDTH 由 10 降到 3 至 4(s9-shards.cx:28,init 时生效),flow --par 由 8 降到 4,s9.heavy.width 维持表值 | U1 至 U5 共 540,加 REFUSED 49、F4 的 2 | 改参数 10 分钟;重跑另计 | 不改任何限时。片数约变 3 倍,需重估总时长;候选池 Q 的每片 15 秒固定开销决定下限 |
| 2 | tools-compile 审核清单补登 107 个工具:tests/tools-compile.cx --register ROOT NEW_REGISTRY 生成后人工核对再替换 tests/tools-compile-sources.tsv | F1 的 94,加 tools-compile 其余 15 批(本次为 UNKNOWN,单跑 tools-compile-000 同样 FAIL) | 30 分钟 | 是审核表更新,不是放宽;分片数若变要同步 batches.tsv |
| 3 | F3 平台批不入计划(候选池 S 已定):runner-platform 等 4 批记 NOT_APPLICABLE,不再拖垮同片 | U7 的 18 | 1 小时 | 按现有 F3 定义,不是新增口径 |
| 4 | U6:tools/lab-loader/check-utf8.cx 的 cx-plan 拒绝定因并修;同时让单批计划被拒只影响该批 | 10 | 1 小时 | 先读 all-waves 发出 REFUSE cx-plan 的源码行 |
| 5 | 冻结根构造带上 unisacc.com.build.json | F3 的 2,加 whole-o-deadline | 30 分钟 | s9 根构造工具 |
| 6 | 材料 F4a、F4b(候选池 S 已排):补生产者与材料依赖材料 | U8、U9 中约 100 | 半天以上,需拆 | kernel-route、ri-*、loader6、ref、o1/o2、streams 等 |
| 7 | 逐个修真实缺陷:pins(1)、chain-argv-check(2)、graph-anchor(2)、run-reserve 与 compile-as-limit 的 low-named-refusal(2)、source-bom(1)、random-expr(1)、slice-migration-gen(1)、capsule-export(1)、watch-bundle(1)、stream-prune-cases(1) | 约 14,且重跑后还会冒出新的 | 每项 20 至 60 分钟,可并行派 | 先各自单跑复现再修 |
| 8 | F5 贴线批拆分:单跑超过约 28 秒的批拆小 | 重跑后才知道数量 | 待测 | 不许靠加大 s9.cmd 解决 |

可如实写进发布说明、不放宽口径的部分:

(1) full-* 17 批(F2)是 release 档的域闭合清单,按设计在有 OPEN 项时 FAIL。它们会让 daily 的 FULL 选择集永远 NOT_QUALIFIED,除非按候选池 S 的 F2 给 release 档单独聚合分支。在该分支实现前,只能如实写"域闭合 17 项 OPEN,明细见各 full-* 输出",不能改标。
(2) 材料 F4c 类(old-product 30、release-ctor-location 28、candidate-profile、release-fixture 等约 60 批):候选池 S 已定"移出 daily 改 release 或绑最近一轮慢车道链产物,无候选报 NOT_APPLICABLE"。这条定义存在但未实现,本次它们是 UNKNOWN。要么实现 F4c 后重跑,要么发布说明如实写 UNKNOWN 及原因;不得手工改标。
(3) NOT_APPLICABLE 41 批是现行定义(平台不属本机),无需处理。

重跑办法:

(1) 本 RUN 只能做一次性验证,不能出资格:RUN/driver retry SLICE 只接受无 FAIL 的终态片,每片只能重试一层(s9-shards.cx:149-157 的 retry-limit、sticky-fail)。137 片里含 FAIL 的 23 片不可重试。可用它验证第 1 项:挑 U2、U3 类的片(如 slice-1-53、slice-1-54)登记 retry 后,以 s9-nightly ROOT --resume RUN --phase steps 推进,flow --par 4。这一步我没有实跑,因为它会往 RUN 写新片,超出只读范围。
(2) 出资格必须新冻结根:第 1 至 5 项合入 main 后,s9-root 建新根,s9-nightly --phase init,再 --phase steps 循环到非 PENDING,--phase aggregate --agg-parts 4。flow 侧 --par 4,S9_WIDTH=3 或 4。
(3) 分类重跑:新 RUN 里先单独推进 commit 档片(tier 0)看时间窗是否消失,再放 daily 档;材料类批在 F4 落地前照旧 NOT_AVAIL,不必等它们。
(4) 预计:第 1 至 5 项合计约 3 至 4 小时人工(可并行派 4 路),之后一轮 daily 重跑。按片数约 3 倍、par 4 粗估 60 至 90 分钟,超过分治铁律的单相位上限,需按片分步续跑。这个时长是推算,未实测。

### 未核实与局限

(1) U5 与 REFUSED 的 sid 测量失败只看到拒绝文案,未复现,未读到子进程的 stderr。
(2) 14 个无 receipt 的 REFUSED 批为何是 REFUSED 而非 UNKNOWN,未在聚合分片输出里找到 RECEIPT_REFUSE 原文。
(3) F8 至 F11 与单跑新冒出的 4 个 FAIL 都只有失败行,未定因。
(4) 单跑摸底只覆盖 38 个 UNKNOWN 批,"约八成能过"是小样本,不能外推为精确比例。
(5) 第 1 项降并发后时间窗是否足够,没有实测;若每片固定开销在低并发下仍接近 15 秒,预算 55 的慢批仍会被杀,要靠第 8 项。

补充核实(23:27 至 23:31,均为只读命令实测):

(6) 冻结根取自分支 v0.0.68 的提交 5a071a43df,不是 main 的 HEAD(8e280aaf83);5a071a43df 不是 main HEAD 的祖先。上文凡写"主干同样如此"的两处(tools-compile 审核清单缺新工具、src/tapeprune.c 不存在)已在两边分别核对,结论对两边都成立。
(7) U5 与 REFUSED 的机制已读到源码:身份测量是 tools/bound 8 sh ROOT/unisacc.com ROOT/seed/ident.c -- ROOT,自带 8 秒看门狗,退出码非 0 或有任何 stderr 即记 sid-measurement 失败(tests/cxlib/allsession.cx:151-154)。空载单独实测 2.0 秒,输出 5e012a24...与会话 sid 一致。争抢下超过 8 秒即整波判身份变化。定性为时间窗类成立;争抢下的失败本身仍未复现。
(8) U6 定因:all-waves.cx:160 对每个批入口跑 sh candidate -cx-plan ENTRY(10 秒上限),退出码非 0 即 die cx-plan,整片不起。批 utf8-net(入口 tools/lab-loader/check-utf8.cx)在冻结根实测退出码 1,stderr 为 cx: tools/lab-loader/runtime/model_decode.cx:3: module modelcache: no file tools/lab-loader/runtime/modelcache.cx。原因:v0.0.68 分支上 tools/lab-loader/runtime/model_decode.cx 是指向 cx/runtime/model_decode.cx 的符号链接,0.0.68 的 model_decode 新增了 use modelcache,而 tools/lab-loader/runtime/ 下没有对应的 modelcache.cx 链接。这是 0.0.68 本版改动带出的真实缺陷,属阻塞项(修法待该车道定:补链接或改限定名);main 工作树上同一命令退出码 0,因为 main 的 model_decode 尚无该 use。
(9) 修复清单第 4 项据此改为:修 utf8-net 的模块解析(1 批),并让单批 cx-plan 被拒只影响该批而不是整片(同片 9 批)。
(10) F8 已复现并收窄:在仓外目录按 tests/run-reserve.cx 的做法执行 ulimit -v 40000 后 sh ROOT/unisacc.com -run t.c,候选产品退出码 2、stdout 为空,但 stderr 也为空,缺少断言要求的 run: out of memory 文案(run-reserve.cx:54)。确定性,与争抢无关;单跑 FAIL 的 compile-as-limit 三条 low-named-refusal 断言同类。定为真实缺陷(产品在低虚存下的具名拒绝文案丢失,或拒绝发生在打印之前),阻塞项,归产品车道定因。
(11) F10 收窄:tools/check-watch-bundle.cx:38 用 argv[0] --read 重新执行自己;在 run-batch 下 argv[0] 是 .cx 源路径,该文件在版本树里是 100644,执行即 Permission denied。与冻结根的 0600 权限无关。定为检查缺陷(假设 argv[0] 可执行),未另行复跑。
(12) 按车道线索的分布(来源路径):tests/ 下 UNKNOWN 254、FAIL 112、PASS 33、REFUSED 12;tools/ 下 UNKNOWN 489、PASS 167、REFUSED 37、FAIL 12。lab-loader 相关 38 批全未过(UNKNOWN 32、REFUSED 6);ri-* 40 批 PASS 2、UNKNOWN 38(多为 ri-cli、ri-chain、ri-flow-runtime 材料缺失)。apps/csih 与 ujs 车道在本计划内没有批次。
(13) 阻塞清单因此增补:第 4 项(utf8-net 模块解析,0.0.68 改动带出)与第 7 项里的 low-named-refusal 属本版必须定因的两条;其余第 7 项各条在单跑复现后再定是否阻塞。
(14) 单跑新冒出的 FAIL 逐个定性(只读核查):random-expr 是环境问题,证据目录 /tmp/check-random-expr-lptshf/run/batch-0 里 gcc 三步 rc=0,tcc.identity.status 为 transport=-1,本机没有 tcc(which 只找到 /usr/bin/gcc);不是产品缺陷,但该批在本机永远过不了,需按现行定义处理宿主工具依赖(声明材料或前置,不得改标)。slice-migration-gen 是真实检查漂移:在冻结根直接跑 tools/slice-migration-gen.cx -- --check 得 FAIL source-missing cx/compiler/frames.cx,该文件在 v0.0.68 与 main 都已不存在,生成器的输入表过期。source-bom 三条 tape/byte-equal 断言 FAIL(C-BOM、cx-BOM、C-BOM-CRLF),其余 32 条 PASS,未定因,疑为产品行为,需优先看。
(15) F9 capsule-export 收窄:release/ci 下 sys、mem、runtime、sysx、text 是指向 cx/ 与 ext/ 的符号链接目录;编译 release/ci 单元时 cx 报 module sys.fs has two lexical entries: release/ci/sys/fs.cx and cx/sys/fs.cx。与第 (8) 条 lab-loader 同属"经符号链接目录到达模块"一族,两者是否同一根因未证实;若是,修一处可同时清掉 capsule-export 与 utf8-net 所在片。
(16) 至此真实缺陷清单(需修,按是否 0.0.68 带出):本版带出或疑似带出 3 项(utf8-net 模块解析、capsule-export 双入口、low-named-refusal 文案丢失,共影响 run-reserve、compile-as-limit、capsule-export、utf8-net 及其同片 9 批);既有漂移 7 项(tools-compile 清单 109 批、pins 1、chain-argv-check 2、graph-anchor 2、slice-migration-gen 1、stream-prune-cases 1、watch-bundle 1);待定因 1 项(source-bom);环境 2 项(random-expr 缺 tcc、冻结根缺 unisacc.com.build.json 影响 3 批);争抢假失败 2 批。
(17) U8 的 119 批按候选池 S 的 F4 三步对照(按片日志里实际缺的材料名;归类依据 prd.md 第 682 行的列举,未列举的记未归类):F4c 类 60 批(old-product 30、release-ctor-location 28、candidate-profile 1、release-fixture 1);F4b 类 39 批(ri-flow-runtime 11、ref 10、ri-cli 8、ri-chain 7、old-checker 1、before-checker 1、o2-run 1);F4a 类 11 批(kernel-route 10、regprep-dir 1);未归类 9 批(source-origin 2、source-job-context 2、libneed-native、kernels、e2reader、assembler、adopted-package 各 1)。U9 的 56 批片日志没有缺失材料名,只知道登记的首个材料,不能可靠归类。
(18) F6 定位:REFUSE chain-argv-check line=3001、3230 指向 tools/candidate-chain.cx(3308 行,另两份被检源 chain-step.cx 1835 行、whole-front.cx 288 行都不到 3001 行);检查器认不得该文件这两处的数组构造写法。
(19) 第 (2) 条补证:slice-1-88 计划 9 批,其中 host-cell、illegal-action 有 S9 NOT_AVAIL 行(材料 candidate-sha、illegal-action-package)未进波,其余 7 批进波后因身份测量失败记 identity-changed-during-wave;聚合把该片 9 批全部记 REFUSED。即 14 个无 receipt 的 REFUSED 是与身份变化的波同片、自身未跑的批(已核 2 个,其余 12 个按同一模式推断)。
## 发布步骤与差距(cc 接手后)

来源:relprep(cc 调度),2026-11-16 00:18 起,只读核对。依据:本文件上文、prd_mempalace.md 决定节、release/decision.txt 与 release/rollback.txt(均仍为 0.0.67)、archive/prd_mempalace-2026-11-14.md 的起链配方 1 至 5 与 0.0.62 发布 12 步实际做法、tools/release-run.cx:143-190、prd.md 候选池 Q 与 S。未跑 daily、未起整链,本节不含任何新的通过结论。

### 现状实测(00:19 命令实测)

(1) 分支未合入 main:origin/v0.0.68 与工作树 HEAD 均为 42caaf6115,main 与 origin/main 为 8e280aaf83;git merge-base --is-ancestor 42caaf6115 origin/main 为否,origin/main..v0.0.68 有 61 个提交。派工背景里"已合入 main"与实测不符,以实测为准:冻结前必须先合入。
(2) release/candidate.tsv 的 tag 列仍为 v0.0.67(commit e02a8f8099);decision.txt 仍是 0.0.67 的 NOT_QUALIFIED(s9 UNKNOWN);rollback.txt 仍是 0.0.67 的回滚。三者都要按 0.0.68 重写,旧收据不得改名沿用。
(3) daily 第二次真跑在 /tmp/v68-s9-daily-42caaf6115 进行中(本窗口未碰);00:19 时 out/daily/aggregate.out 尚不存在,结论待该子代理回报。
(4) 产品源相对 v0.0.67 只改 3 个文件:cx/runtime/model_decode.cx、cx/runtime/modelcache.cx、src/executor.cx(26 增 4 删);exec/ 只改 exec/facts/derive.tsv 一行,kernel/ 无改动。

### 步骤清单与差距

| 序 | 步骤 | 前置 | 现状 | 缺什么 | 谁来做 |
|---|---|---|---|---|---|
| 0 | 合入 main | 版本分支稳定、daily 第二次真跑结论已出(决定是否再修) | 未合入(见现状 1) | 合入与推送 main;合入后复核 1-rev/1-branch | 主调度 cc(集成者) |
| 1 | 冻结门复跑 | 步 0 完成,冻结提交固定 | 上文冻结门复跑清单 (1)(8) 均未在冻结提交上执行 | derive 十项分片、13 项 ratchets、cxmod-tri、cx0-extract、P1 完整扫描、candidate-chain 合同、tlib-frozen 与 bootstrap-0.0.59 多单元编译,经 devsvc 有界节点逐项跑,不整体调用 freeze-gate/chain-ready 的 95/80 秒子闸 | 待派(建议 relgate 子代理) |
| 2 | 整链 E2 | 步 1 全过;起链配方 1 至 5(私有根、bootstrap 0.0.59 与其 build.json、cxroot.tsv 指向本 rev 快照、只设 UNISACC_IDENT_HELPER、不设 UNISACC_RUN_CACHE 与 UNISACC_CACHE_DIR);账本行写 REV、PID/PGID、截止、输出目录 | 0.0.68 没有任何整链目录 | candidate-chain --rev REV --out 全新目录 --parallel 12,产出 checkpoint.tsv、chain-run.tsv、fixedpoint g2/g3 | 主调度 cc 起链,后台有界监视 |
| 3 | s9 commit 档 | 冻结后 | 不挡公开(prd_mempalace 决定) | 与整链并行跑,结果补记 release/evidence.tsv;新增 FAIL 走补丁版 | 随步 2 并行,待派 |
| 4 | s9 daily 档 | 新冻结根 | 第一次真跑 NOT_QUALIFIED(上文);第二次在跑 | 第二次结论;若仍 NOT_QUALIFIED,按 FAIL/UNKNOWN 分类决定修或如实入说明。现有定义下 daily 档 NOT_QUALIFIED 时 decision 的 s9 步不能记 PASS | 在跑的子代理出结论,cc 裁定 |
| 5 | s9 release 档 | 同上 | full-* 17 项按设计 OPEN(F2) | 候选池 S 的 F2 release 档单独聚合分支未实现;实现前只能如实写 OPEN | 无人负责,见下节 |
| 6 | 公开硬门(方案 A 第 4 步) | 步 2 产出 | 代码在 tools/release-run.cx:143-190,releaseprepare 经 tools/cxlib/releaseprepare.cx:206 传 --chain-out;无 0.0.68 整链,未实跑 | compare DONE、最后 compare attempt rc=0 residue=0 memo=off、四类 same(kernels 2、slices 6)、g2/g3 六目标切片 SHA256 逐个相等 | 随 release-run --pack 自动执行,cc 核输出 |
| 7 | 造 F 树与 v0.0.68-src 标签 | 步 2 的 REV | 未做 | git archive 解到全新目录,git add -f -A,commit-tree -p fbdc99ca -p REV(0.0.62 做法) | cc |
| 8 | pack-srcdir 与 release-run --pack | 步 6、7 | 未做 | 全新 W 目录;ROOT 为含 unisacc.com 的完整树;candidate 表 commit 非空、version 0.0.68 | cc |
| 9 | X 提交(只改 release/candidate.tsv) | 步 8 | 未做 | 21 列表,tag v0.0.68 | cc |
| 10 | 草稿 release 与资产 | 步 9 | 未做 | 6 资产(unisacc.com、pkg.bin、slice-win-arm64、unisacc.com.build.json、build.json、SHA256SUMS) | cc |
| 11 | 同候选 CI | 步 10 | 未做 | candidate-native.yml 新 run 全作业 success;旧 run 43672079447 不授权 | cc |
| 12 | Y1(公开前):发布说明、decision、evidence、rollback | 步 11 | 说明草稿见 plans/release-notes-0.0.68.md(本轮起草,含待填) | 填实测数;rollback.txt 按 0.0.68 重写(回到 0.0.67 Latest、草稿化 v0.0.68、前向回滚 X/Y1/Y2、本机装机回滚到 0.0.67 APE a99ea71d) | cc |
| 13 | 公开、读回、装机、Y2 | 步 12 | 未做 | gh release edit v0.0.68 --draft=false --latest;读回 sha 与 --version;install-ape --write | cc |
| 14 | 公开后大扫除 | 步 13 | 未做 | 冻结根 0087、f77、5a07、42ca 的去留按真实进程与 du 核后串行删 | cc |

### 判定要点

(1) 公开阻塞(现有定义):步 0、1、2、6、11 未做;daily 档结论未出。commit 档不挡公开。
(2) decision.txt 的规则:任一步 UNKNOWN 即 NOT_QUALIFIED。0.0.67 是在 s9 UNKNOWN 下公开的(按当时授权与说明如实写缺口);0.0.68 若 daily 仍非全绿,必须同样如实写 NOT_QUALIFIED 与缺口,不得宣称 QUALIFIED。
(3) 时长:整链 real6 实测 3752 秒,超分治 5 分钟单操作线,只能作为正规构造链工具一次跑完一个相位;冻结到公开推算约 88 分钟(旧值),本版未实测。

### 未有人负责的发布前缺口与建议负责单

| 缺口 | 现状 | 建议负责 | 是否挡公开 |
|---|---|---|---|
| 合入 main(现状 1) | 未合 | cc 集成者 | 挡 |
| F2 release 档域闭合单独聚合分支 | 未实现,17 项 OPEN | s9 harness 子代理(写域 tests/cxlib/allsession.cx、tools/s9-*) | 不挡,说明如实写 |
| F4b 材料生产者(ri-flow-runtime、ref、ri-cli、ri-chain 等 39 批) | 未实现 | 材料子代理(affected 生产者规则) | 不挡,写 UNKNOWN |
| F4c 无候选材料报 NOT_APPLICABLE(old-product、release-ctor-location 等约 60 批) | tier 已部分移 release(198ba2e64f、fa77aa97d9、a52b57f67e),报 NOT_APPLICABLE 的逻辑未核实是否落地 | s9 harness 子代理先核再补 | 不挡,写 UNKNOWN |
| F5 贴线批拆分(单跑超约 28 秒) | 需第二次真跑数据 | 各批属主 | 视第二次结果 |
| run-reserve、compile-as-limit low-named-refusal | 6929ca4d91 修了 ape-pack 壳头补文案;待新壳头产品(整链产物)上复测 | 整链后由 cc 复测 | 若复测仍 FAIL 则挡 daily |
| random-expr 缺宿主 tcc | 66505c70d7 改为 HOST_TOOL_MISSING rc 3 | 需裁定按材料或前置声明;建议环境子代理 | 不挡,写缺口 |
| source-bom 三条 tape/byte-equal | 66505c70d7 改对比方式,未见新根复测结论 | 等第二次真跑 | 待定 |
| slice-migration-gen 输入表过期 | 未见修复提交 | 工具属主 | 不挡 |
| streams-material 往根内写不清 | 未修 | 测试属主 | 不挡 |
| 0.0.68 rollback.txt、decision、evidence 重写 | 未做 | cc(步 12) | 挡 |
| U5 sid 测量 8 秒窗争抢 | 降并发 21f95007be 后待第二次真跑验证 | s9 子代理 | 视结果 |

补充核实(00:20 至 00:22,只读):
(1) F4c"无候选材料报 NOT_APPLICABLE"未实现:tools/s9-shards.cx:139-140 与 tests/cxlib/allsession.cx:550 只按登记 platform 判 NOT_APPLICABLE,没有按材料缺失判的分支。上表 F4c 一行由"未核实"改为"已核未实现"。
(2) slice-migration-gen 与 streams-material 自 v0.0.67 以来无修复提交(git log 对 tools/slice-migration-gen.cx、tests/check-streams-material.cx 为空);check-streams-material.cx:1 仍声明工作目录 out/streams-material-PID。

## 逐批归类表

cdx只读分诊，2026-11-16。旧根5a071a43df，聚合为 /tmp/v68-s9-daily-5a071a43df/out/daily/aggregate.out。main的s9-triage实跑复核:1159批，PASS202、FAIL124、UNKNOWN743、REFUSED49、NOT_APPLICABLE41。本表逐一覆盖916个FAIL/UNKNOWN/REFUSED；41个NOT_APPLICABLE为范围外，不算失败。旧flow全文现不可读，未以其缺失冒充内容；根内聚合、137片计划、片日志、会话与收据仍可读。未执行旧根批次，未改冻结输入、代码或表。优先插单只在版本树申报tools/pins.tsv两行，并独立提交43378d1a91，不改变本次分诊输入。

修复身份核对基准:main 84d3a23d3f8d20773ae748b4b1b3083c528ffe25，版本分支43378d1a91cba3df920ea6ba69d943f0ec04f027(含426171e5c2补三工具清单及c7795c4171的F2)。合并尚在进行，以下提交以git可见内容和说明核实，不声称已进入尚未生成的新根。

类别互斥:具名修复列已修待复核，旧因保留；其余按材料F4a/b/c、时间窗、真缺陷分类。候选池S未明确列举的材料单列材料待归类。直接超时文案与测量失败是事实，争抢因果、静默移出因果及新根转绿均标推测。full-* OPEN按设计未闭合，放在真缺陷类表示真实未通过，不等于产品回归。没有新根收据的行均不记PASS。

固定输入SHA-256:aggregate.out=6db4725c852b24c8cf821cd98e25fca49f148630a5ba6558735f61fe270410c0；plan.tsv=3ca32c3cd9c50d508bae149f0c644c918f954d9375472b10a10e953c1812f206；materials.tsv=f7c495ad50db14315582c86952d75493ddba01f5f5489ad62fdddc112b42377a。可用main入口复核状态计数:sh ./unisacc.com tools/s9-triage.cx -- /tmp/v68-s9-daily-5a071a43df/out/daily/aggregate.out。--help当前返回unknown-option；实际用法已读工具文件头，未绕开入口。

| 类别 | 批数 |
|---|---|
| 时间窗 | 559 |
| 材料 F4a | 16 |
| 材料 F4b | 82 |
| 材料 F4c | 60 |
| 材料待归类 | 30 |
| 真缺陷 | 17 |
| 已修待复核 | 152 |
| 合计 | 916 |

证据审计:读1232份文件、697份具名receipt，全部697份与session.tsv所记receipt SHA匹配；这是既有产物整理，不重授资格。相对路径均相对旧RUN；日志因果不足的行保守注明推测。

已修待复核不表示该行旧时间窗已消失:若旧因为超时/REFUSED，具名提交只解决该批已知清单、入口或检查缺陷；时间窗仍需新根验证。REFUSED优先按波/收据复核拒绝归类，另缺材料只作第二阻塞保留，不用F4c追溯改标。

与fable汇总的口径差异:本节材料类188批均找到本批具名NOT_AVAIL，直接缺映射优先于同片后来出现的cx-plan/异平台/窗口错误；已实测其中8批缺loader6且同片又有异平台拒绝。本节同时读片.log与片/wave.log，不沿用旧U9的静默假设，也不凭两表差额倒推其它原因；旧119+56=175的汇总原位保留。本表是当前证据重算并按具名修复去重，不把数字变化当新根通过或修复收益。

7f0ca7c7db确实修复materials分组索引性能(提交实跑13.197秒)，但旧affected-tool登记跑的是list、预算25秒，旧证据是窗口不足，不能据此标已修；故它仍列时间窗。该性能修复不补任何材料生产者。

修复索引:e43b97ecbf补清单，4d7c7a48a9同步删旧链接，54a62b656a修130片边界，426171e5c2再补check-s9-f4c/check-s9-triage/s9-triage三个工具(1038源，仍130片)；1c52ddf1aa排异平台；6929ca4d91修模块入口和壳头文案(须重打包产品)；61e730f9d8修pins/argv/graph/frames漂移，43378d1a91补F4c两处占位申报；66505c70d7修watch/prune/BOM及缺tcc诊断；7cf4705e22登记hosttcc；21f95007be补伴随记录并降推荐并发(不证明波窗已消失)；a4990c8a7e仅显式daily无候选且白名单缺映射时NA；d729fae9af、b997444202拆schedule/phases；c7795c4171只增release聚合，不消灭OPEN。

| 批次 | 旧态 | 类别 | 旧因与证据 | 已核修复提交 | 新根预期状态(无实跑不授PASS) |
|---|---|---|---|---|---|
| abab-bench | UNKNOWN | 时间窗 | slice-0-46.log/wave.log: insufficient-wave-window；slice-0-46.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| adopted-package | UNKNOWN | 材料待归类 | slice-2-127.log/wave.log:缺映射 adopted-package(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| adoption-capture-path | UNKNOWN | 时间窗 | slice-0-31/wave-1/adoption-capture-path.receipt reason=watchdog-deadline; child_seconds=19; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| affected-drain | UNKNOWN | 时间窗 | slice-0-47/wave-1/affected-drain.receipt reason=watchdog-deadline; child_seconds=21; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| affected-iface | UNKNOWN | 时间窗 | slice-0-42/wave-1/affected-iface.receipt reason=watchdog-deadline; child_seconds=28; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| affected-iface2 | UNKNOWN | 时间窗 | slice-0-42/wave-1/affected-iface2.receipt reason=watchdog-deadline; child_seconds=28; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| affected-list | UNKNOWN | 时间窗 | slice-0-37/wave-1/affected-list.receipt reason=watchdog-deadline; child_seconds=22; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| affected-list2 | UNKNOWN | 时间窗 | slice-0-37/wave-1/affected-list2.receipt reason=watchdog-deadline; child_seconds=22; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| affected-run | UNKNOWN | 时间窗 | slice-0-37/wave-1/affected-run.receipt reason=watchdog-deadline; child_seconds=22; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| affected-tiers | UNKNOWN | 时间窗 | slice-0-37/wave-1/affected-tiers.receipt reason=watchdog-deadline; child_seconds=22; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| affected-tool | UNKNOWN | 时间窗 | slice-1-72.log/wave.log: insufficient-wave-window；slice-1-72.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| alloc-errno | UNKNOWN | 时间窗 | slice-0-31/wave-1/alloc-errno.receipt reason=watchdog-deadline; child_seconds=19; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| appendmerge | UNKNOWN | 已修待复核 | slice-1-116/wave.log:异平台批拒绝连带本批 | 1c52ddf1aa | 本机批不再受该异平台批拖累(推测)，仍须实跑；异平台独立NOT_APPLICABLE |
| archive-mv | UNKNOWN | 时间窗 | slice-0-45/wave-1/archive-mv.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| archive-refs | UNKNOWN | 时间窗 | slice-1-82/wave-1/archive-refs.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| arity-controls | UNKNOWN | 已修待复核 | slice-1-84/wave.log:cx-plan拒check-utf8.cx，连带本批 | 6929ca4d91 | 旧utf8模块入口阻塞已修，仍须新根实跑，非现成PASS |
| arm-encoder-osx-dynamic-bundle | FAIL | 时间窗 | slice-1-107/wave-1/arm-encoder-osx-dynamic-bundle.receipt reason=batch-fail; child_seconds=12; 收据SHA=匹配 | 无专门修复；21f95007be仅缓解 | encoder单跑PASS是旧摸底证据；争抢因果推测，另一批及新根均待复核 |
| arm-encoder-record-origin | REFUSED | 时间窗 | slice-1-104/wave-1/arm-encoder-record-origin.receipt reason=identity-changed-during-wave; child_seconds=16; 收据SHA=匹配 | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| assembler | UNKNOWN | 材料待归类 | slice-1-55.log/wave.log:缺映射 assembler(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| assembler-cache | UNKNOWN | 材料待归类 | slice-1-55.log/wave.log:缺映射 kernels(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| auto-continue | UNKNOWN | 时间窗 | slice-0-50/wave-1/auto-continue.receipt reason=watchdog-deadline; child_seconds=22; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| autoscan-decl | UNKNOWN | 时间窗 | slice-0-31/wave-1/autoscan-decl.receipt reason=watchdog-deadline; child_seconds=19; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| bad-package | UNKNOWN | 时间窗 | slice-1-87/wave-1/bad-package.receipt reason=watchdog-deadline; child_seconds=18; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| batch-cost | UNKNOWN | 时间窗 | slice-0-40/wave-1/batch-cost.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| batch-packages | UNKNOWN | 时间窗 | slice-0-24/wave-1/batch-packages.receipt reason=watchdog-deadline; child_seconds=19; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| bg-candidate | UNKNOWN | 时间窗 | slice-0-24/wave-1/bg-candidate.receipt reason=watchdog-deadline; child_seconds=19; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| boot-head-check | UNKNOWN | 已修待复核 | slice-1-84/wave.log:cx-plan拒check-utf8.cx，连带本批 | 6929ca4d91 | 旧utf8模块入口阻塞已修，仍须新根实跑，非现成PASS |
| bound-headroom | UNKNOWN | 时间窗 | slice-0-38/wave-1/bound-headroom.receipt reason=watchdog-deadline; child_seconds=17; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| bound-rc142 | UNKNOWN | 时间窗 | slice-1-76/wave-1/bound-rc142.receipt reason=watchdog-deadline; child_seconds=21; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| boundenv-tool | UNKNOWN | 时间窗 | slice-0-47/wave-1/boundenv-tool.receipt reason=watchdog-deadline; child_seconds=21; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| c-negative | UNKNOWN | 时间窗 | slice-1-68/wave-2/c-negative.receipt reason=watchdog-deadline; child_seconds=10; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cache-manager-determinism | UNKNOWN | 时间窗 | slice-0-15.log/wave.log: operation-deadline；slice-0-15.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| callback-wrapper-index | UNKNOWN | 时间窗 | slice-1-54.log/wave.log: operation-deadline；slice-1-54.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| candidate-chain | UNKNOWN | 时间窗 | slice-0-14.log/wave.log: sid-measurement；slice-0-14.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| candidate-chain-lanes | UNKNOWN | 时间窗 | slice-1-124.log/wave.log: operation-deadline；slice-1-124.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| candidate-chain-objects | UNKNOWN | 时间窗 | slice-0-46.log/wave.log: insufficient-wave-window；slice-0-46.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| candidate-chain-watch | UNKNOWN | 时间窗 | slice-0-46.log/wave.log: insufficient-wave-window；slice-0-46.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| candidate-identity-lnx-x86_64 | UNKNOWN | 材料 F4c | slice-2-130.log/wave.log:缺映射 candidate-profile(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| candidate-profile | UNKNOWN | 时间窗 | slice-0-32/wave-1/candidate-profile.receipt reason=watchdog-deadline; child_seconds=23; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| candidate-qualify | UNKNOWN | 时间窗 | slice-1-87/wave-1/candidate-qualify.receipt reason=watchdog-deadline; child_seconds=18; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cap-gen | UNKNOWN | 时间窗 | slice-0-40/wave-1/cap-gen.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| capsule-export | FAIL | 已修待复核 | slice-1-71/wave-1/capsule-export.receipt reason=batch-fail; child_seconds=24; 收据SHA=匹配 | 6929ca4d91 | 提交记录该批单跑PASS；新根仍须复核模块入口和产物 |
| census | UNKNOWN | 时间窗 | slice-1-75.log/wave.log: insufficient-wave-window；slice-1-75.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| chain-argv-check | FAIL | 已修待复核 | slice-0-19/wave-1/chain-argv-check.receipt reason=batch-fail; child_seconds=24; 收据SHA=匹配 | 61e730f9d8 | 已修检查/路径漂移，提交称相关批单跑过；新候选资格仍待新根实跑 |
| chain-argv-check-b | FAIL | 已修待复核 | slice-0-19/wave-1/chain-argv-check-b.receipt reason=batch-fail; child_seconds=24; 收据SHA=匹配 | 61e730f9d8 | 已修检查/路径漂移，提交称相关批单跑过；新候选资格仍待新根实跑 |
| chain-batchkeys | UNKNOWN | 时间窗 | slice-0-17/wave-1/chain-batchkeys.receipt reason=watchdog-deadline; child_seconds=44; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| chain-facts | UNKNOWN | 时间窗 | slice-1-124.log/wave.log: operation-deadline；slice-1-124.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| chain-fixed-point | UNKNOWN | 材料待归类 | slice-2-130.log/wave.log:缺映射 chain-gen1(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| chain-fresh-out | UNKNOWN | 时间窗 | slice-1-125.log/wave.log: hard deadline；slice-1-125.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| chain-pace | UNKNOWN | 时间窗 | slice-0-46.log/wave.log: insufficient-wave-window；slice-0-46.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| chain-partition | UNKNOWN | 时间窗 | slice-1-124.log/wave.log: operation-deadline；slice-1-124.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| chain-phases-gen | UNKNOWN | 时间窗 | slice-0-17/wave-1/chain-phases-gen.receipt reason=watchdog-deadline; child_seconds=44; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| chain-precheck | UNKNOWN | 时间窗 | slice-1-91/wave-1/chain-precheck.receipt reason=watchdog-deadline; child_seconds=6; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| chain-ready | UNKNOWN | 时间窗 | slice-0-44/wave-1/chain-ready.receipt reason=watchdog-deadline; child_seconds=10; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| chain-run | UNKNOWN | 时间窗 | slice-0-18/wave-1/chain-run.receipt reason=watchdog-deadline; child_seconds=23; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| chain-run-segments | UNKNOWN | 时间窗 | slice-0-19/wave-1/chain-run-segments.receipt reason=watchdog-deadline; child_seconds=24; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| chain-run-supervise | UNKNOWN | 时间窗 | slice-0-19/wave-1/chain-run-supervise.receipt reason=watchdog-deadline; child_seconds=24; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| chain-smoke | UNKNOWN | 时间窗 | slice-0-16.log/wave.log: after-measurement-failed sid-measurement；测量失败实测，争抢/超时因果推测；slice-0-16.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| chain-tool-pin | UNKNOWN | 时间窗 | slice-1-125.log/wave.log: hard deadline；slice-1-125.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| chain-watch | UNKNOWN | 时间窗 | slice-0-15.log/wave.log: operation-deadline；slice-0-15.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| chainargv | UNKNOWN | 时间窗 | slice-0-19/wave-1/chainargv.receipt reason=watchdog-deadline; child_seconds=24; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| chainresume | UNKNOWN | 时间窗 | slice-1-124.log/wave.log: operation-deadline；slice-1-124.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| chainroot | UNKNOWN | 时间窗 | slice-0-41/wave-1/chainroot.receipt reason=watchdog-deadline; child_seconds=21; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| check-decide | UNKNOWN | 时间窗 | slice-1-85/wave-1/check-decide.receipt reason=watchdog-deadline; child_seconds=14; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| check-edit-span | UNKNOWN | 时间窗 | slice-1-52/wave-1/check-edit-span.receipt reason=watchdog-deadline; child_seconds=22; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| check-export | UNKNOWN | 时间窗 | slice-0-44/wave-1/check-export.receipt reason=watchdog-deadline; child_seconds=10; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| check-limits | UNKNOWN | 时间窗 | slice-0-9/wave-1/check-limits.receipt reason=watchdog-deadline; child_seconds=13; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| check-win-ffi | UNKNOWN | 时间窗 | slice-0-31/wave-1/check-win-ffi.receipt reason=watchdog-deadline; child_seconds=19; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| ci-collect | UNKNOWN | 时间窗 | slice-0-44/wave-1/ci-collect.receipt reason=watchdog-deadline; child_seconds=10; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| clock-targets | UNKNOWN | 时间窗 | slice-1-91/wave-1/clock-targets.receipt reason=watchdog-deadline; child_seconds=6; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| com-tree | UNKNOWN | 时间窗 | slice-1-73.log/wave.log: bound timeout；slice-1-73.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| commit-own | UNKNOWN | 时间窗 | slice-1-73.log/wave.log: bound timeout；slice-1-73.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| commit-own-nolock | UNKNOWN | 时间窗 | slice-1-126.log/wave.log: hard deadline；slice-1-126.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| compile-as-limit | UNKNOWN | 已修待复核 | slice-1-76/wave-1/compile-as-limit.receipt reason=watchdog-deadline; child_seconds=21; 收据SHA=匹配 | 6929ca4d91(壳头源码) | 只换源不换a99ea71d产品仍FAIL；须用新ape-pack实际重打包，不能预测PASS |
| cpu-meter | UNKNOWN | 时间窗 | slice-0-39/wave-1/cpu-meter.receipt reason=watchdog-deadline; child_seconds=23; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cross-targets-lnx | REFUSED | 时间窗 | slice-1-88/wave-1/cross-targets-lnx.receipt reason=identity-changed-during-wave; child_seconds=19; 收据SHA=匹配 | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| cross-targets-osx | REFUSED | 时间窗 | slice-1-88/wave-1/cross-targets-osx.receipt reason=identity-changed-during-wave; child_seconds=19; 收据SHA=匹配 | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| cross-targets-win | REFUSED | 时间窗 | slice-1-88/wave-1/cross-targets-win.receipt reason=identity-changed-during-wave; child_seconds=19; 收据SHA=匹配 | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| ctestsuite | UNKNOWN | 时间窗 | slice-1-93/wave-1/ctestsuite.receipt reason=watchdog-deadline; child_seconds=12; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| ctor-debt | UNKNOWN | 时间窗 | slice-0-14.log/wave.log: sid-measurement；slice-0-14.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx-base-external | UNKNOWN | 已修待复核 | slice-1-84/wave.log:cx-plan拒check-utf8.cx，连带本批 | 6929ca4d91 | 旧utf8模块入口阻塞已修，仍须新根实跑，非现成PASS |
| cx-bench-gencode | UNKNOWN | 已修待复核 | slice-1-84/wave.log:cx-plan拒check-utf8.cx，连带本批 | 6929ca4d91 | 旧utf8模块入口阻塞已修，仍须新根实跑，非现成PASS |
| cx-booter-ape | UNKNOWN | 材料待归类 | slice-2-130.log/wave.log:缺映射 booter-raw-elf(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| cx-bounded | UNKNOWN | 时间窗 | slice-0-1.log/wave.log: insufficient-wave-window；slice-0-1.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx-codec | UNKNOWN | 时间窗 | slice-1-53.log/wave.log: insufficient-wave-window；slice-1-53.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx-core-interface | UNKNOWN | 时间窗 | slice-1-70/wave-1/cx-core-interface.receipt reason=watchdog-deadline; child_seconds=24; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx-core-run-diff | UNKNOWN | 材料待归类 | slice-1-70.log/wave.log:缺映射 kernels(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| cx-driver-route | UNKNOWN | 时间窗 | slice-1-53.log/wave.log: insufficient-wave-window；slice-1-53.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx-e2stream | UNKNOWN | 时间窗 | slice-1-81/wave-1/cx-e2stream.receipt reason=watchdog-deadline; child_seconds=23; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx-elf-text | UNKNOWN | 时间窗 | slice-1-81/wave-1/cx-elf-text.receipt reason=watchdog-deadline; child_seconds=23; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx-ffi-bytes | UNKNOWN | 时间窗 | slice-1-81/wave-1/cx-ffi-bytes.receipt reason=watchdog-deadline; child_seconds=23; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx-funit-host | UNKNOWN | 时间窗 | slice-1-74/wave-1/cx-funit-host.receipt reason=watchdog-deadline; child_seconds=8; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx-host-run | UNKNOWN | 时间窗 | slice-1-74/wave-1/cx-host-run.receipt reason=watchdog-deadline; child_seconds=8; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx-l0-crypto-tls13 | UNKNOWN | 时间窗 | slice-1-74/wave-1/cx-l0-crypto-tls13.receipt reason=watchdog-deadline; child_seconds=8; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx-l0-crypto-x25519 | UNKNOWN | 时间窗 | slice-1-74/wave-1/cx-l0-crypto-x25519.receipt reason=watchdog-deadline; child_seconds=8; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx-l0-fmt | UNKNOWN | 时间窗 | slice-1-71/wave-1/cx-l0-fmt.receipt reason=watchdog-deadline; child_seconds=24; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx-l0-sys | UNKNOWN | 时间窗 | slice-1-71/wave-1/cx-l0-sys.receipt reason=watchdog-deadline; child_seconds=24; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx-l0-sys-net | UNKNOWN | 时间窗 | slice-1-71/wave-1/cx-l0-sys-net.receipt reason=watchdog-deadline; child_seconds=24; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx-l0-sys-new | UNKNOWN | 时间窗 | slice-0-41/wave-1/cx-l0-sys-new.receipt reason=watchdog-deadline; child_seconds=21; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx-l0-sys-sig | UNKNOWN | 时间窗 | slice-1-73.log/wave.log: bound timeout；slice-1-73.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx-l0-text-fmt | UNKNOWN | 时间窗 | slice-0-40/wave-1/cx-l0-text-fmt.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx-layers | UNKNOWN | 时间窗 | slice-1-74/wave-1/cx-layers.receipt reason=watchdog-deadline; child_seconds=8; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx-library-cache | UNKNOWN | 材料待归类 | slice-1-70.log/wave.log:缺映射 p3-pack-ctx(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| cx-library-payload | UNKNOWN | 时间窗 | slice-1-70/wave-1/cx-library-payload.receipt reason=watchdog-deadline; child_seconds=24; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx-lint | UNKNOWN | 时间窗 | slice-0-13.log/wave.log: operation-deadline；slice-0-13.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx-manifest | UNKNOWN | 已修待复核 | slice-1-84/wave.log:cx-plan拒check-utf8.cx，连带本批 | 6929ca4d91 | 旧utf8模块入口阻塞已修，仍须新根实跑，非现成PASS |
| cx-model-decode | UNKNOWN | 材料待归类 | slice-1-70.log/wave.log:缺映射 p3model-material(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| cx-namespace | UNKNOWN | 时间窗 | slice-0-32/wave-1/cx-namespace.receipt reason=watchdog-deadline; child_seconds=23; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx-npfault | UNKNOWN | 时间窗 | slice-1-81/wave-1/cx-npfault.receipt reason=watchdog-deadline; child_seconds=23; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx-os-new | UNKNOWN | 时间窗 | slice-0-41/wave-1/cx-os-new.receipt reason=watchdog-deadline; child_seconds=21; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx-own-check | UNKNOWN | 时间窗 | slice-0-9/wave-1/cx-own-check.receipt reason=watchdog-deadline; child_seconds=13; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx-own-escapes | UNKNOWN | 时间窗 | slice-0-45/wave-1/cx-own-escapes.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx-p3-extract | UNKNOWN | 材料待归类 | slice-1-70.log/wave.log:缺映射 p3-pack-ctx(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| cx-p3-roundtrip | UNKNOWN | 时间窗 | slice-1-82/wave-1/cx-p3-roundtrip.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx-packagefooter | UNKNOWN | 时间窗 | slice-1-53.log/wave.log: insufficient-wave-window；slice-1-53.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx-procguard | UNKNOWN | 时间窗 | slice-1-78/wave-1/cx-procguard.receipt reason=watchdog-deadline; child_seconds=26; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx-register | UNKNOWN | 材料 F4a | slice-1-80.log/wave.log:缺映射 regprep-dir(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| cx-resolver | UNKNOWN | 时间窗 | slice-1-83/wave-1/cx-resolver.receipt reason=watchdog-deadline; child_seconds=26; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx-root-identity | FAIL | 已修待复核 | slice-1-81/wave-1/cx-root-identity.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | 21f95007be | 新根须带真实产品build.json并核内容；旧缺伴随文件断言预期恢复(推测)，须实跑 |
| cx-sbload | UNKNOWN | 时间窗 | slice-0-9/wave-1/cx-sbload.receipt reason=watchdog-deadline; child_seconds=13; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx-stage-lower | UNKNOWN | 已修待复核 | slice-1-84/wave.log:cx-plan拒check-utf8.cx，连带本批 | 6929ca4d91 | 旧utf8模块入口阻塞已修，仍须新根实跑，非现成PASS |
| cx-streamapi | UNKNOWN | 时间窗 | slice-0-41/wave-1/cx-streamapi.receipt reason=watchdog-deadline; child_seconds=20; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx-streamown | UNKNOWN | 时间窗 | slice-0-44/wave-1/cx-streamown.receipt reason=watchdog-deadline; child_seconds=10; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx-streampack | UNKNOWN | 时间窗 | slice-1-81/wave-1/cx-streampack.receipt reason=watchdog-deadline; child_seconds=23; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx-sys-netnb | UNKNOWN | 时间窗 | slice-0-44/wave-1/cx-sys-netnb.receipt reason=watchdog-deadline; child_seconds=10; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx-sys-netraw | UNKNOWN | 时间窗 | slice-0-41/wave-1/cx-sys-netraw.receipt reason=watchdog-deadline; child_seconds=21; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx-sys-sigwatch | UNKNOWN | 时间窗 | slice-0-44/wave-1/cx-sys-sigwatch.receipt reason=watchdog-deadline; child_seconds=10; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx-toolsloc | UNKNOWN | 时间窗 | slice-1-81/wave-1/cx-toolsloc.receipt reason=watchdog-deadline; child_seconds=23; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx-unit-graph | UNKNOWN | 时间窗 | slice-1-53.log/wave.log: insufficient-wave-window；slice-1-53.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx-unitkey | UNKNOWN | 时间窗 | slice-0-16.log/wave.log: after-measurement-failed sid-measurement；测量失败实测，争抢/超时因果推测；slice-0-16.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx0-bundle | UNKNOWN | 时间窗 | slice-1-53.log/wave.log: insufficient-wave-window；slice-1-53.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx0-diff | UNKNOWN | 时间窗 | slice-0-43/wave-1/cx0-diff.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cx0-e2 | UNKNOWN | 材料待归类 | slice-1-53.log/wave.log:缺映射 e2reader(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| cx0-unit | UNKNOWN | 时间窗 | slice-1-53.log/wave.log: insufficient-wave-window；slice-1-53.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cxmod-tri | UNKNOWN | 时间窗 | slice-0-43/wave-1/cxmod-tri.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| cxraw-dock | UNKNOWN | 时间窗 | slice-1-85/wave-1/cxraw-dock.receipt reason=watchdog-deadline; child_seconds=14; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| decode-sbout | UNKNOWN | 时间窗 | slice-1-90.log/wave.log: insufficient-wave-window；slice-1-90.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| delta-loader | UNKNOWN | 时间窗 | slice-1-53.log/wave.log: insufficient-wave-window；slice-1-53.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| dep-split-probe | UNKNOWN | 已修待复核 | slice-0-11/wave.log:异平台批拒绝连带本批 | 1c52ddf1aa | 本机批不再受该异平台批拖累(推测)，仍须实跑；异平台独立NOT_APPLICABLE |
| derive | UNKNOWN | 时间窗 | slice-0-43/wave-1/derive.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| dev-product | UNKNOWN | 时间窗 | slice-1-87/wave-1/dev-product.receipt reason=watchdog-deadline; child_seconds=18; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| devsvc | UNKNOWN | 时间窗 | slice-0-49/wave-1/devsvc.receipt reason=watchdog-deadline; child_seconds=20; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| devsvc-core | UNKNOWN | 时间窗 | slice-0-50/wave-1/devsvc-core.receipt reason=watchdog-deadline; child_seconds=22; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| devsvc-flow | UNKNOWN | 时间窗 | slice-0-49/wave-1/devsvc-flow.receipt reason=watchdog-deadline; child_seconds=20; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| devsvc-git | UNKNOWN | 时间窗 | slice-0-50/wave-1/devsvc-git.receipt reason=watchdog-deadline; child_seconds=22; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| devsvc-usage | UNKNOWN | 时间窗 | slice-0-51/wave-1/devsvc-usage.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| devtask | UNKNOWN | 时间窗 | slice-0-50/wave-1/devtask.receipt reason=watchdog-deadline; child_seconds=22; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| die-lib | UNKNOWN | 时间窗 | slice-0-15.log/wave.log: operation-deadline；slice-0-15.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| doc-links-tool | UNKNOWN | 时间窗 | slice-1-73.log/wave.log: bound timeout；slice-1-73.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| doc-pair | UNKNOWN | 时间窗 | slice-0-37/wave-1/doc-pair.receipt reason=watchdog-deadline; child_seconds=22; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| dock-audit | UNKNOWN | 时间窗 | slice-0-43/wave-1/dock-audit.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| dock-map | UNKNOWN | 已修待复核 | slice-1-84/wave.log:cx-plan拒check-utf8.cx，连带本批 | 6929ca4d91 | 旧utf8模块入口阻塞已修，仍须新根实跑，非现成PASS |
| domain | UNKNOWN | 时间窗 | slice-0-45/wave-1/domain.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| driver-diagnostics | UNKNOWN | 时间窗 | slice-0-2/wave-1/driver-diagnostics.receipt reason=watchdog-deadline; child_seconds=14; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| driver-token-target | UNKNOWN | 时间窗 | slice-1-53.log/wave.log: insufficient-wave-window；slice-1-53.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| e2-input-binding | UNKNOWN | 时间窗 | slice-1-54.log/wave.log: operation-deadline；slice-1-54.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| e3-warnings | UNKNOWN | 时间窗 | slice-1-76/wave-1/e3-warnings.receipt reason=watchdog-deadline; child_seconds=21; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| elf-secdiff | UNKNOWN | 时间窗 | slice-0-46.log/wave.log: insufficient-wave-window；slice-0-46.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| elf-x86-image | UNKNOWN | 时间窗 | slice-1-76/wave-1/elf-x86-image.receipt reason=watchdog-deadline; child_seconds=21; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| encoder-osx-bundle | FAIL | 时间窗 | slice-1-107/wave-1/encoder-osx-bundle.receipt reason=batch-fail; child_seconds=12; 收据SHA=匹配 | 无专门修复；21f95007be仅缓解 | encoder单跑PASS是旧摸底证据；争抢因果推测，另一批及新根均待复核 |
| encoder-record-origin | REFUSED | 时间窗 | slice-1-103/wave-1/encoder-record-origin.receipt reason=identity-changed-during-wave; child_seconds=13; 收据SHA=匹配 | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| encoder-record-osx | REFUSED | 时间窗 | slice-1-104/wave-1/encoder-record-osx.receipt reason=identity-changed-during-wave; child_seconds=16; 收据SHA=匹配 | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| enosys-sites | UNKNOWN | 时间窗 | slice-0-42/wave-1/enosys-sites.receipt reason=watchdog-deadline; child_seconds=28; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| entry-receipt-head | UNKNOWN | 时间窗 | slice-0-17/wave-1/entry-receipt-head.receipt reason=watchdog-deadline; child_seconds=44; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| evidence-cover | UNKNOWN | 时间窗 | slice-0-39/wave-1/evidence-cover.receipt reason=watchdog-deadline; child_seconds=23; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| examples-smoke | UNKNOWN | 时间窗 | slice-0-45/wave-1/examples-smoke.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| exitclass | UNKNOWN | 时间窗 | slice-0-38/wave-1/exitclass.receipt reason=watchdog-deadline; child_seconds=17; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| exitclass-gen | UNKNOWN | 时间窗 | slice-0-38/wave-1/exitclass-gen.receipt reason=watchdog-deadline; child_seconds=17; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| ext-net-tcp | UNKNOWN | 时间窗 | slice-0-41/wave-1/ext-net-tcp.receipt reason=watchdog-deadline; child_seconds=21; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| ext-net-unix | UNKNOWN | 时间窗 | slice-0-43/wave-1/ext-net-unix.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| ext-procx-exec | UNKNOWN | 时间窗 | slice-0-46.log/wave.log: insufficient-wave-window；slice-0-46.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| facts-current | UNKNOWN | 时间窗 | slice-1-87/wave-1/facts-current.receipt reason=watchdog-deadline; child_seconds=18; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| fake-gh-contract | UNKNOWN | 时间窗 | slice-0-40/wave-1/fake-gh-contract.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| fixedpoint-sample | UNKNOWN | 时间窗 | slice-0-13.log/wave.log: operation-deadline；slice-0-13.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| fixedpoint-sample-v4 | UNKNOWN | 时间窗 | slice-0-16.log/wave.log: after-measurement-failed sid-measurement；测量失败实测，争抢/超时因果推测；slice-0-16.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| flow-gen | UNKNOWN | 时间窗 | slice-0-50/wave-1/flow-gen.receipt reason=watchdog-deadline; child_seconds=22; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| flow-tool | UNKNOWN | 时间窗 | slice-1-76/wave-1/flow-tool.receipt reason=watchdog-deadline; child_seconds=21; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| flowdoc | UNKNOWN | 时间窗 | slice-0-50/wave-1/flowdoc.receipt reason=watchdog-deadline; child_seconds=22; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| flowexec | UNKNOWN | 时间窗 | slice-0-49/wave-1/flowexec.receipt reason=watchdog-deadline; child_seconds=20; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| flowfile | UNKNOWN | 时间窗 | slice-0-49/wave-1/flowfile.receipt reason=watchdog-deadline; child_seconds=20; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| flowrun | UNKNOWN | 时间窗 | slice-0-49/wave-1/flowrun.receipt reason=watchdog-deadline; child_seconds=20; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| formats | UNKNOWN | 时间窗 | slice-2-127/wave-2/formats.receipt reason=watchdog-deadline; child_seconds=9; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| formats-archive-ar | UNKNOWN | 时间窗 | slice-2-130/wave-3/formats-archive-ar.receipt reason=watchdog-deadline; child_seconds=3; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| formats-elf-ape | UNKNOWN | 时间窗 | slice-2-127/wave-2/formats-elf-ape.receipt reason=watchdog-deadline; child_seconds=9; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| formats-macho-pe | UNKNOWN | 时间窗 | slice-2-130/wave-3/formats-macho-pe.receipt reason=watchdog-deadline; child_seconds=3; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| formats-object-ld | UNKNOWN | 时间窗 | slice-2-130/wave-3/formats-object-ld.receipt reason=watchdog-deadline; child_seconds=3; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| formats-objects | UNKNOWN | 时间窗 | slice-2-127/wave-2/formats-objects.receipt reason=watchdog-deadline; child_seconds=9; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| freeze-gate | UNKNOWN | 时间窗 | slice-0-49/wave-1/freeze-gate.receipt reason=watchdog-deadline; child_seconds=20; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| freeze-plan | UNKNOWN | 时间窗 | slice-0-13.log/wave.log: operation-deadline；slice-0-13.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| full-bootstrap | FAIL | 真缺陷 | slice-2-129/wave-1/full-bootstrap.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | c7795c4171仅修release聚合，未修OPEN | 域闭合OPEN仍FAIL；release独立RELEASE_ONLY不得改成daily QUALIFIED |
| full-candidate | UNKNOWN | 时间窗 | slice-2-130/wave-3/full-candidate.receipt reason=watchdog-deadline; child_seconds=3; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| full-domain-check | UNKNOWN | 时间窗 | slice-1-75.log/wave.log: insufficient-wave-window；slice-1-75.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| full-formats | FAIL | 真缺陷 | slice-2-129/wave-1/full-formats.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | c7795c4171仅修release聚合，未修OPEN | 域闭合OPEN仍FAIL；release独立RELEASE_ONLY不得改成daily QUALIFIED |
| full-hosts | FAIL | 真缺陷 | slice-2-129/wave-1/full-hosts.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | c7795c4171仅修release聚合，未修OPEN | 域闭合OPEN仍FAIL；release独立RELEASE_ONLY不得改成daily QUALIFIED |
| full-interfaces | FAIL | 真缺陷 | slice-2-129/wave-1/full-interfaces.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | c7795c4171仅修release聚合，未修OPEN | 域闭合OPEN仍FAIL；release独立RELEASE_ONLY不得改成daily QUALIFIED |
| full-library | FAIL | 真缺陷 | slice-2-129/wave-1/full-library.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | c7795c4171仅修release聚合，未修OPEN | 域闭合OPEN仍FAIL；release独立RELEASE_ONLY不得改成daily QUALIFIED |
| full-negative | FAIL | 真缺陷 | slice-2-129/wave-1/full-negative.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | c7795c4171仅修release聚合，未修OPEN | 域闭合OPEN仍FAIL；release独立RELEASE_ONLY不得改成daily QUALIFIED |
| full-optimization | FAIL | 真缺陷 | slice-2-129/wave-1/full-optimization.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | c7795c4171仅修release聚合，未修OPEN | 域闭合OPEN仍FAIL；release独立RELEASE_ONLY不得改成daily QUALIFIED |
| full-phase-timing | UNKNOWN | 时间窗 | slice-2-130/wave-3/full-phase-timing.receipt reason=watchdog-deadline; child_seconds=3; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| full-properties | FAIL | 真缺陷 | slice-2-129/wave-1/full-properties.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | c7795c4171仅修release聚合，未修OPEN | 域闭合OPEN仍FAIL；release独立RELEASE_ONLY不得改成daily QUALIFIED |
| full-resources | FAIL | 真缺陷 | slice-2-129/wave-1/full-resources.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | c7795c4171仅修release聚合，未修OPEN | 域闭合OPEN仍FAIL；release独立RELEASE_ONLY不得改成daily QUALIFIED |
| full-ri | FAIL | 真缺陷 | slice-2-129/wave-1/full-ri.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | c7795c4171仅修release聚合，未修OPEN | 域闭合OPEN仍FAIL；release独立RELEASE_ONLY不得改成daily QUALIFIED |
| full-stream-e1 | FAIL | 真缺陷 | slice-2-128/wave-1/full-stream-e1.receipt reason=batch-fail; child_seconds=21; 收据SHA=匹配 | c7795c4171仅修release聚合，未修OPEN | 域闭合OPEN仍FAIL；release独立RELEASE_ONLY不得改成daily QUALIFIED |
| full-stream-e2 | FAIL | 真缺陷 | slice-2-128/wave-1/full-stream-e2.receipt reason=batch-fail; child_seconds=21; 收据SHA=匹配 | c7795c4171仅修release聚合，未修OPEN | 域闭合OPEN仍FAIL；release独立RELEASE_ONLY不得改成daily QUALIFIED |
| full-stream-e3 | FAIL | 真缺陷 | slice-2-128/wave-1/full-stream-e3.receipt reason=batch-fail; child_seconds=21; 收据SHA=匹配 | c7795c4171仅修release聚合，未修OPEN | 域闭合OPEN仍FAIL；release独立RELEASE_ONLY不得改成daily QUALIFIED |
| full-stream-e4 | FAIL | 真缺陷 | slice-2-128/wave-1/full-stream-e4.receipt reason=batch-fail; child_seconds=21; 收据SHA=匹配 | c7795c4171仅修release聚合，未修OPEN | 域闭合OPEN仍FAIL；release独立RELEASE_ONLY不得改成daily QUALIFIED |
| full-stream-elf | FAIL | 真缺陷 | slice-2-128/wave-1/full-stream-elf.receipt reason=batch-fail; child_seconds=21; 收据SHA=匹配 | c7795c4171仅修release聚合，未修OPEN | 域闭合OPEN仍FAIL；release独立RELEASE_ONLY不得改成daily QUALIFIED |
| full-stream-lower | FAIL | 真缺陷 | slice-2-128/wave-1/full-stream-lower.receipt reason=batch-fail; child_seconds=21; 收据SHA=匹配 | c7795c4171仅修release聚合，未修OPEN | 域闭合OPEN仍FAIL；release独立RELEASE_ONLY不得改成daily QUALIFIED |
| full-stream-prune | FAIL | 真缺陷 | slice-2-128/wave-1/full-stream-prune.receipt reason=batch-fail; child_seconds=21; 收据SHA=匹配 | c7795c4171仅修release聚合，未修OPEN | 域闭合OPEN仍FAIL；release独立RELEASE_ONLY不得改成daily QUALIFIED |
| func-callgraph | UNKNOWN | 时间窗 | slice-1-107/wave-1/func-callgraph.receipt reason=watchdog-deadline; child_seconds=12; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| g2-budget-gen | UNKNOWN | 时间窗 | slice-1-124.log/wave.log: operation-deadline；slice-1-124.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| gate-prepare | UNKNOWN | 时间窗 | slice-0-24/wave-1/gate-prepare.receipt reason=watchdog-deadline; child_seconds=19; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| gh-record | UNKNOWN | 时间窗 | slice-0-39/wave-1/gh-record.receipt reason=watchdog-deadline; child_seconds=23; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| graph-anchor-route | FAIL | 已修待复核 | slice-0-20/wave-1/graph-anchor-route.receipt reason=batch-fail; child_seconds=14; 收据SHA=匹配 | 61e730f9d8 | 已修检查/路径漂移，提交称相关批单跑过；新候选资格仍待新根实跑 |
| graph-anchor-structure | FAIL | 已修待复核 | slice-0-20/wave-1/graph-anchor-structure.receipt reason=batch-fail; child_seconds=14; 收据SHA=匹配 | 61e730f9d8 | 已修检查/路径漂移，提交称相关批单跑过；新候选资格仍待新根实跑 |
| graph-check | UNKNOWN | 时间窗 | slice-0-13.log/wave.log: operation-deadline；slice-0-13.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| guard-edit | UNKNOWN | 时间窗 | slice-0-43/wave-1/guard-edit.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| harness-contracts | UNKNOWN | 时间窗 | slice-0-49/wave-1/harness-contracts.receipt reason=watchdog-deadline; child_seconds=20; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| host-cell | REFUSED | 时间窗 | slice-1-88/wave.log:after-measurement-failed sid-measurement；本批无收据，波级拒绝连带；另缺映射candidate-sha，不能覆盖旧REFUSED | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| host-cell-binding | UNKNOWN | 已修待复核 | slice-0-11/wave.log:异平台批拒绝连带本批 | 1c52ddf1aa | 本机批不再受该异平台批拖累(推测)，仍须实跑；异平台独立NOT_APPLICABLE |
| hosts-same-candidate-lnx-x86_64 | UNKNOWN | 材料待归类 | slice-2-128.log/wave.log:缺映射 hosts-domain-matrix(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| identity | UNKNOWN | 时间窗 | slice-1-54.log/wave.log: operation-deadline；slice-1-54.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| identity-derive | REFUSED | 时间窗 | slice-1-92/wave-1/identity-derive.receipt reason=identity-changed-during-wave; child_seconds=19; 收据SHA=匹配 | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| iface-gap | UNKNOWN | 时间窗 | slice-0-31/wave-1/iface-gap.receipt reason=watchdog-deadline; child_seconds=19; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| iface-hash | UNKNOWN | 时间窗 | slice-0-42/wave-1/iface-hash.receipt reason=watchdog-deadline; child_seconds=28; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| illegal-action | REFUSED | 时间窗 | slice-1-88/wave.log:after-measurement-failed sid-measurement；本批无收据，波级拒绝连带；另缺映射illegal-action-package，不能覆盖旧REFUSED | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| image-contrib | UNKNOWN | 时间窗 | slice-1-91/wave-1/image-contrib.receipt reason=watchdog-deadline; child_seconds=6; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| inc-abspath | UNKNOWN | 时间窗 | slice-1-90.log/wave.log: insufficient-wave-window；slice-1-90.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| include-shadow | UNKNOWN | 时间窗 | slice-0-30/wave-1/include-shadow.receipt reason=watchdog-deadline; child_seconds=24; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| input-refuse | UNKNOWN | 时间窗 | slice-1-75.log/wave.log: insufficient-wave-window；slice-1-75.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| install-ape | UNKNOWN | 时间窗 | slice-1-90.log/wave.log: insufficient-wave-window；slice-1-90.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| install-fixes | UNKNOWN | 时间窗 | slice-0-30/wave-1/install-fixes.receipt reason=watchdog-deadline; child_seconds=24; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| integrate-patch | UNKNOWN | 时间窗 | slice-1-117/wave-1/integrate-patch.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| integrate-patch-affected | UNKNOWN | 时间窗 | slice-1-119/wave-1/integrate-patch-affected.receipt reason=watchdog-deadline; child_seconds=18; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| integrate-patch-land | REFUSED | 时间窗 | slice-1-118/wave-1/integrate-patch-land.receipt reason=identity-changed-during-wave; child_seconds=10; 收据SHA=匹配 | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| integrate-patch-preflight-resume | UNKNOWN | 时间窗 | slice-1-120/wave-1/integrate-patch-preflight-resume.receipt reason=watchdog-deadline; child_seconds=9; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| integrate-patch-preflight-stop | UNKNOWN | 时间窗 | slice-1-120/wave-1/integrate-patch-preflight-stop.receipt reason=watchdog-deadline; child_seconds=9; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| integrate-patch-prepare-resume | UNKNOWN | 时间窗 | slice-1-120/wave-1/integrate-patch-prepare-resume.receipt reason=watchdog-deadline; child_seconds=9; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| integrate-ratchet-cold | UNKNOWN | 时间窗 | slice-1-119/wave-1/integrate-ratchet-cold.receipt reason=watchdog-deadline; child_seconds=18; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| integrate-ratchet-corrupt | UNKNOWN | 时间窗 | slice-1-120/wave-1/integrate-ratchet-corrupt.receipt reason=watchdog-deadline; child_seconds=9; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| integrate-ratchet-hit | UNKNOWN | 时间窗 | slice-1-120/wave-1/integrate-ratchet-hit.receipt reason=watchdog-deadline; child_seconds=9; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| integrate-ratchet-input | UNKNOWN | 时间窗 | slice-1-120/wave-1/integrate-ratchet-input.receipt reason=watchdog-deadline; child_seconds=9; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| integrate-ratchet-prepare | UNKNOWN | 时间窗 | slice-1-119/wave-1/integrate-ratchet-prepare.receipt reason=watchdog-deadline; child_seconds=18; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| integrate-ratchet-product | UNKNOWN | 时间窗 | slice-1-120/wave-1/integrate-ratchet-product.receipt reason=watchdog-deadline; child_seconds=9; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| integrate-ratchet-tool | UNKNOWN | 时间窗 | slice-1-120/wave-1/integrate-ratchet-tool.receipt reason=watchdog-deadline; child_seconds=9; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| interface-digest | UNKNOWN | 时间窗 | slice-1-107/wave-1/interface-digest.receipt reason=watchdog-deadline; child_seconds=12; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| interfaces-lnx-x86_64 | UNKNOWN | 时间窗 | slice-1-53.log/wave.log: insufficient-wave-window；slice-1-53.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| kernel-base | UNKNOWN | 时间窗 | slice-1-87/wave-1/kernel-base.receipt reason=watchdog-deadline; child_seconds=18; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| kernel-route-coverage | UNKNOWN | 材料 F4a | slice-1-86.log/wave.log:缺映射 kernel-route(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| kernel-route-encoding | UNKNOWN | 材料 F4a | slice-1-85.log/wave.log:缺映射 kernel-route(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| kernel-route-encoding-arm | UNKNOWN | 材料 F4a | slice-1-86.log/wave.log:缺映射 kernel-route(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| kernel-route-encoding-x86 | UNKNOWN | 材料 F4a | slice-1-85.log/wave.log:缺映射 kernel-route(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| kernel-route-encoding-x86-a | UNKNOWN | 材料 F4a | slice-1-86.log/wave.log:缺映射 kernel-route(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| kernel-route-encoding-x86-b | UNKNOWN | 材料 F4a | slice-1-86.log/wave.log:缺映射 kernel-route(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| kernel-route-object | UNKNOWN | 材料 F4a | slice-1-86.log/wave.log:缺映射 kernel-route(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| kernel-route-pin-mutation | UNKNOWN | 材料 F4a | slice-1-86.log/wave.log:缺映射 kernel-route(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| kernel-route-pins | UNKNOWN | 材料 F4a | slice-1-87.log/wave.log:缺映射 kernel-route(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| kernel-route-refusal | UNKNOWN | 材料 F4a | slice-1-86.log/wave.log:缺映射 kernel-route(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| kernel-route-relocation | UNKNOWN | 材料 F4a | slice-1-86.log/wave.log:缺映射 kernel-route(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| kernel-route-route-recovery | UNKNOWN | 材料 F4a | slice-1-86.log/wave.log:缺映射 kernel-route(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| kernel-route-route-refusal | UNKNOWN | 材料 F4a | slice-1-86.log/wave.log:缺映射 kernel-route(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| layer-api | UNKNOWN | 时间窗 | slice-0-20/wave-1/layer-api.receipt reason=watchdog-deadline; child_seconds=14; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| ledger-add | UNKNOWN | 时间窗 | slice-0-40/wave-1/ledger-add.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| ledger-parser-compare | UNKNOWN | 时间窗 | slice-0-3/wave-1/ledger-parser-compare.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| libc-decl-scan | UNKNOWN | 时间窗 | slice-0-42/wave-1/libc-decl-scan.receipt reason=watchdog-deadline; child_seconds=28; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| libc-probes | UNKNOWN | 时间窗 | slice-0-31/wave-1/libc-probes.receipt reason=watchdog-deadline; child_seconds=19; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| libc99posix | UNKNOWN | 时间窗 | slice-1-90.log/wave.log: insufficient-wave-window；slice-1-90.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| libneed-attributes | UNKNOWN | 材料待归类 | slice-1-54.log/wave.log:缺映射 libneed-native(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| library-cells | UNKNOWN | 时间窗 | slice-1-63.log/wave.log: insufficient-wave-window；slice-1-63.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| limits-gen | UNKNOWN | 时间窗 | slice-0-30/wave-1/limits-gen.receipt reason=watchdog-deadline; child_seconds=24; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| link-edge-cases | UNKNOWN | 时间窗 | slice-1-68/wave-2/link-edge-cases.receipt reason=watchdog-deadline; child_seconds=10; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| link-malformed-input | UNKNOWN | 时间窗 | slice-1-68/wave-2/link-malformed-input.receipt reason=watchdog-deadline; child_seconds=10; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| link-sourceless-tu | UNKNOWN | 材料待归类 | slice-1-114.log/wave.log:缺映射 sourceless-product(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| link-undefined-function | UNKNOWN | 时间窗 | slice-1-68/wave-2/link-undefined-function.receipt reason=watchdog-deadline; child_seconds=10; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| links-gen | UNKNOWN | 时间窗 | slice-0-45/wave-1/links-gen.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| lit-const | UNKNOWN | 时间窗 | slice-1-90.log/wave.log: insufficient-wave-window；slice-1-90.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| loader-common-models | REFUSED | 时间窗 | slice-1-92/wave.log:after-measurement-failed sid-measurement；本批无收据，波级拒绝连带；另缺映射loader6，不能覆盖旧REFUSED | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| loader-common-o2 | UNKNOWN | 材料 F4b | slice-1-112.log/wave.log:缺映射 loader6(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| loader-common-tapecheck | UNKNOWN | 材料 F4b | slice-1-112.log/wave.log:缺映射 loader6(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| loader-common-tokens-lex | UNKNOWN | 材料 F4b | slice-1-112.log/wave.log:缺映射 loader6(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| loader-common-tokens-pp | UNKNOWN | 材料 F4b | slice-1-112.log/wave.log:缺映射 loader6(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| loader-common-units | UNKNOWN | 材料 F4b | slice-1-112.log/wave.log:缺映射 loader6(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| loader-common-warn | UNKNOWN | 材料 F4b | slice-1-112.log/wave.log:缺映射 loader6(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| loader-deployment-binding | UNKNOWN | 已修待复核 | slice-0-11/wave.log:异平台批拒绝连带本批 | 1c52ddf1aa | 本机批不再受该异平台批拖累(推测)，仍须实跑；异平台独立NOT_APPLICABLE |
| loader-faults | UNKNOWN | 时间窗 | slice-1-75.log/wave.log: insufficient-wave-window；slice-1-75.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| loader-isa-start | REFUSED | 时间窗 | slice-1-92/wave.log:after-measurement-failed sid-measurement；本批无收据，波级拒绝连带；另缺映射loader6，不能覆盖旧REFUSED | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| loader-net-replacement | REFUSED | 时间窗 | slice-1-92/wave.log:after-measurement-failed sid-measurement；本批无收据，波级拒绝连带；另缺映射loader6，不能覆盖旧REFUSED | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| loader-special | REFUSED | 时间窗 | slice-1-92/wave.log:after-measurement-failed sid-measurement；本批无收据，波级拒绝连带；另缺映射loader6，不能覆盖旧REFUSED | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| loader-target-image-lnx-arm64 | UNKNOWN | 材料 F4b | slice-1-112.log/wave.log:缺映射 loader6(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| loader-target-image-osx-arm64 | UNKNOWN | 材料 F4b | slice-1-113.log/wave.log:缺映射 loader6(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| loader-target-image-osx-x86_64 | UNKNOWN | 材料 F4b | slice-1-112.log/wave.log:缺映射 loader6(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| loader-target-image-win-arm64 | UNKNOWN | 材料 F4b | slice-1-113.log/wave.log:缺映射 loader6(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| loader-target-image-win-x86_64 | UNKNOWN | 材料 F4b | slice-1-113.log/wave.log:缺映射 loader6(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| loader-target-models | REFUSED | 时间窗 | slice-1-92/wave.log:after-measurement-failed sid-measurement；本批无收据，波级拒绝连带；另缺映射loader6，不能覆盖旧REFUSED | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| loader-target-object-lnx-arm64 | UNKNOWN | 材料 F4b | slice-1-113.log/wave.log:缺映射 loader6(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| loader-target-object-lnx-x86_64 | UNKNOWN | 材料 F4b | slice-1-113.log/wave.log:缺映射 loader6(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| loader-target-object-osx-arm64 | UNKNOWN | 材料 F4b | slice-1-113.log/wave.log:缺映射 loader6(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| loader-target-object-osx-x86_64 | UNKNOWN | 材料 F4b | slice-1-113.log/wave.log:缺映射 loader6(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| loader-target-object-win-arm64 | UNKNOWN | 材料 F4b | slice-1-113.log/wave.log:缺映射 loader6(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| loader-target-object-win-x86_64 | UNKNOWN | 材料 F4b | slice-1-113.log/wave.log:缺映射 loader6(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| logic-distribution | UNKNOWN | 时间窗 | slice-1-102/wave-1/logic-distribution.receipt reason=watchdog-deadline; child_seconds=21; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| lower-arm-episodes | REFUSED | 时间窗 | slice-1-104/wave-1/lower-arm-episodes.receipt reason=identity-changed-during-wave; child_seconds=16; 收据SHA=匹配 | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| lower-fnreuse | UNKNOWN | 时间窗 | slice-0-46.log/wave.log: insufficient-wave-window；slice-0-46.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| lower-prefix | UNKNOWN | 时间窗 | slice-0-47/wave-1/lower-prefix.receipt reason=watchdog-deadline; child_seconds=21; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| lower-prefix-b | UNKNOWN | 时间窗 | slice-0-47/wave-1/lower-prefix-b.receipt reason=watchdog-deadline; child_seconds=21; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| lower-prefix-base0 | UNKNOWN | 时间窗 | slice-0-48/wave-1/lower-prefix-base0.receipt reason=watchdog-deadline; child_seconds=24; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| lower-prefix-base1 | UNKNOWN | 时间窗 | slice-0-48/wave-1/lower-prefix-base1.receipt reason=watchdog-deadline; child_seconds=24; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| lower-prefix-base2 | UNKNOWN | 时间窗 | slice-0-48/wave-1/lower-prefix-base2.receipt reason=watchdog-deadline; child_seconds=24; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| lower-prefix-base3 | UNKNOWN | 时间窗 | slice-0-48/wave-1/lower-prefix-base3.receipt reason=watchdog-deadline; child_seconds=24; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| lower-prefix-base4 | UNKNOWN | 时间窗 | slice-0-48/wave-1/lower-prefix-base4.receipt reason=watchdog-deadline; child_seconds=24; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| lower-prefix-base5 | UNKNOWN | 时间窗 | slice-0-48/wave-1/lower-prefix-base5.receipt reason=watchdog-deadline; child_seconds=24; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| lower-prefix-base6 | UNKNOWN | 时间窗 | slice-0-48/wave-1/lower-prefix-base6.receipt reason=watchdog-deadline; child_seconds=24; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| lower-prefix-c | UNKNOWN | 时间窗 | slice-0-48/wave-1/lower-prefix-c.receipt reason=watchdog-deadline; child_seconds=24; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| lower-row-bundle | UNKNOWN | 时间窗 | slice-1-106/wave-1/lower-row-bundle.receipt reason=watchdog-deadline; child_seconds=15; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| lower-row-episodes | REFUSED | 时间窗 | slice-1-103/wave-1/lower-row-episodes.receipt reason=identity-changed-during-wave; child_seconds=13; 收据SHA=匹配 | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| lower-row-origin | REFUSED | 时间窗 | slice-1-103/wave-1/lower-row-origin.receipt reason=identity-changed-during-wave; child_seconds=13; 收据SHA=匹配 | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| materials-prepare-controls | UNKNOWN | 已修待复核 | slice-1-112/wave.log:异平台批拒绝连带本批 | 1c52ddf1aa | 本机批不再受该异平台批拖累(推测)，仍须实跑；异平台独立NOT_APPLICABLE |
| matrix-stale | UNKNOWN | 时间窗 | slice-1-75.log/wave.log: insufficient-wave-window；slice-1-75.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| matrix-waive | UNKNOWN | 时间窗 | slice-1-74/wave-1/matrix-waive.receipt reason=watchdog-deadline; child_seconds=8; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| mem-recovery | UNKNOWN | 时间窗 | slice-1-76/wave-1/mem-recovery.receipt reason=watchdog-deadline; child_seconds=21; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| model-decode-sha | UNKNOWN | 时间窗 | slice-1-90.log/wave.log: insufficient-wave-window；slice-1-90.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| model-image | UNKNOWN | 时间窗 | slice-0-47/wave-1/model-image.receipt reason=watchdog-deadline; child_seconds=21; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| model-image-cache | UNKNOWN | 时间窗 | slice-0-47/wave-1/model-image-cache.receipt reason=watchdog-deadline; child_seconds=21; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| model-program-census | UNKNOWN | 时间窗 | slice-1-102/wave-1/model-program-census.receipt reason=watchdog-deadline; child_seconds=21; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| modelcache-budget-concurrent | UNKNOWN | 时间窗 | slice-1-52/wave-1/modelcache-budget-concurrent.receipt reason=watchdog-deadline; child_seconds=22; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| modelcache-budget-lru | UNKNOWN | 时间窗 | slice-1-52/wave-1/modelcache-budget-lru.receipt reason=watchdog-deadline; child_seconds=22; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| modroot-lib | REFUSED | 时间窗 | slice-1-118/wave-1/modroot-lib.receipt reason=identity-changed-during-wave; child_seconds=10; 收据SHA=匹配 | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| module-byte-map | REFUSED | 时间窗 | slice-1-103/wave-1/module-byte-map.receipt reason=identity-changed-during-wave; child_seconds=13; 收据SHA=匹配 | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| module-byte-map-arm | REFUSED | 时间窗 | slice-1-104/wave-1/module-byte-map-arm.receipt reason=identity-changed-during-wave; child_seconds=16; 收据SHA=匹配 | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| module-units | UNKNOWN | 时间窗 | slice-1-85/wave-1/module-units.receipt reason=watchdog-deadline; child_seconds=14; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| negative-diagnostics | UNKNOWN | 时间窗 | slice-1-62/wave-2/negative-diagnostics.receipt reason=watchdog-deadline; child_seconds=16; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| negative-hostile | UNKNOWN | 时间窗 | slice-1-63.log/wave.log: insufficient-wave-window；slice-1-63.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| negative-model-loader | UNKNOWN | 时间窗 | slice-1-62/wave-2/negative-model-loader.receipt reason=watchdog-deadline; child_seconds=16; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| negative-model-loader-terminal | UNKNOWN | 时间窗 | slice-1-63.log/wave.log: insufficient-wave-window；slice-1-63.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| net-build-profile | UNKNOWN | 时间窗 | slice-1-108/wave-1/net-build-profile.receipt reason=watchdog-deadline; child_seconds=5; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| net-check | UNKNOWN | 已修待复核 | slice-1-84/wave.log:cx-plan拒check-utf8.cx，连带本批 | 6929ca4d91 | 旧utf8模块入口阻塞已修，仍须新根实跑，非现成PASS |
| net-controls | UNKNOWN | 时间窗 | slice-1-55.log/wave.log: bound timeout；slice-1-55.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| net-deps | UNKNOWN | 时间窗 | slice-1-106/wave-1/net-deps.receipt reason=watchdog-deadline; child_seconds=15; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| net-deps-ratchet | UNKNOWN | 时间窗 | slice-1-108/wave-1/net-deps-ratchet.receipt reason=watchdog-deadline; child_seconds=5; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| net-digest | UNKNOWN | 时间窗 | slice-1-108/wave-1/net-digest.receipt reason=watchdog-deadline; child_seconds=5; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| net-dump | UNKNOWN | 时间窗 | slice-1-85/wave-1/net-dump.receipt reason=watchdog-deadline; child_seconds=14; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| net-incremental-rehearsal | UNKNOWN | 时间窗 | slice-1-108/wave-1/net-incremental-rehearsal.receipt reason=watchdog-deadline; child_seconds=5; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| netregen-boundary-lnx-arm64 | UNKNOWN | 材料 F4c | slice-2-133.log/wave.log:缺映射 old-product(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| netregen-boundary-lnx-x86_64 | UNKNOWN | 材料 F4c | slice-2-133.log/wave.log:缺映射 old-product(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| netregen-boundary-osx-arm64 | UNKNOWN | 材料 F4c | slice-2-133.log/wave.log:缺映射 old-product(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| netregen-boundary-osx-x86_64 | UNKNOWN | 材料 F4c | slice-2-133.log/wave.log:缺映射 old-product(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| netregen-boundary-win-arm64 | UNKNOWN | 材料 F4c | slice-2-133.log/wave.log:缺映射 old-product(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| netregen-boundary-win-x86_64 | UNKNOWN | 材料 F4c | slice-2-133.log/wave.log:缺映射 old-product(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| netregen-module-lnx-arm64 | UNKNOWN | 材料 F4c | slice-2-132.log/wave.log:缺映射 old-product(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| netregen-module-lnx-x86_64 | UNKNOWN | 材料 F4c | slice-2-132.log/wave.log:缺映射 old-product(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| netregen-module-osx-arm64 | UNKNOWN | 材料 F4c | slice-2-133.log/wave.log:缺映射 old-product(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| netregen-module-osx-x86_64 | UNKNOWN | 材料 F4c | slice-2-133.log/wave.log:缺映射 old-product(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| netregen-module-win-arm64 | UNKNOWN | 材料 F4c | slice-2-133.log/wave.log:缺映射 old-product(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| netregen-module-win-x86_64 | UNKNOWN | 材料 F4c | slice-2-133.log/wave.log:缺映射 old-product(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| netregen-multi-lnx-arm64 | UNKNOWN | 材料 F4c | slice-2-132.log/wave.log:缺映射 old-product(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| netregen-multi-lnx-x86_64 | UNKNOWN | 材料 F4c | slice-2-132.log/wave.log:缺映射 old-product(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| netregen-multi-osx-arm64 | UNKNOWN | 材料 F4c | slice-2-132.log/wave.log:缺映射 old-product(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| netregen-multi-osx-x86_64 | UNKNOWN | 材料 F4c | slice-2-132.log/wave.log:缺映射 old-product(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| netregen-multi-win-arm64 | UNKNOWN | 材料 F4c | slice-2-132.log/wave.log:缺映射 old-product(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| netregen-multi-win-x86_64 | UNKNOWN | 材料 F4c | slice-2-132.log/wave.log:缺映射 old-product(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| netregen-nested-lnx-arm64 | UNKNOWN | 材料 F4c | slice-2-131.log/wave.log:缺映射 old-product(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| netregen-nested-lnx-x86_64 | UNKNOWN | 材料 F4c | slice-2-131.log/wave.log:缺映射 old-product(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| netregen-nested-osx-arm64 | UNKNOWN | 材料 F4c | slice-2-131.log/wave.log:缺映射 old-product(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| netregen-nested-osx-x86_64 | UNKNOWN | 材料 F4c | slice-2-131.log/wave.log:缺映射 old-product(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| netregen-nested-win-arm64 | UNKNOWN | 材料 F4c | slice-2-132.log/wave.log:缺映射 old-product(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| netregen-nested-win-x86_64 | UNKNOWN | 材料 F4c | slice-2-132.log/wave.log:缺映射 old-product(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| netregen-small-lnx-arm64 | UNKNOWN | 材料 F4c | slice-2-131.log/wave.log:缺映射 old-product(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| netregen-small-lnx-x86_64 | UNKNOWN | 材料 F4c | slice-2-131.log/wave.log:缺映射 old-product(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| netregen-small-osx-arm64 | UNKNOWN | 材料 F4c | slice-2-131.log/wave.log:缺映射 old-product(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| netregen-small-osx-x86_64 | UNKNOWN | 材料 F4c | slice-2-131.log/wave.log:缺映射 old-product(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| netregen-small-win-arm64 | UNKNOWN | 材料 F4c | slice-2-131.log/wave.log:缺映射 old-product(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| netregen-small-win-x86_64 | UNKNOWN | 材料 F4c | slice-2-131.log/wave.log:缺映射 old-product(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| nets | UNKNOWN | 时间窗 | slice-1-54.log/wave.log: operation-deadline；slice-1-54.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| newcx-check | UNKNOWN | 时间窗 | slice-0-16.log/wave.log: after-measurement-failed sid-measurement；测量失败实测，争抢/超时因果推测；slice-0-16.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| nn-inputs | UNKNOWN | 材料待归类 | slice-1-54.log/wave.log:缺映射 source-origin(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| nn-nf-c-byte-report | UNKNOWN | 材料待归类 | slice-2-130.log/wave.log:缺映射 nnc-nets(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| no-out-snap | FAIL | 已修待复核 | slice-1-119/wave-1/no-out-snap.receipt reason=batch-fail; child_seconds=18; 收据SHA=匹配 | 21f95007be | 新根须带真实产品build.json并核内容；旧缺伴随文件断言预期恢复(推测)，须实跑 |
| notes-fill | UNKNOWN | 时间窗 | slice-0-46.log/wave.log: insufficient-wave-window；slice-0-46.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| object-route-directory | UNKNOWN | 已修待复核 | slice-0-12/wave.log:异平台批拒绝连带本批 | 1c52ddf1aa | 本机批不再受该异平台批拖累(推测)，仍须实跑；异平台独立NOT_APPLICABLE |
| object-source-replay | REFUSED | 时间窗 | slice-1-103/wave-1/object-source-replay.receipt reason=identity-changed-during-wave; child_seconds=13; 收据SHA=匹配 | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| object-source-replay-arm64 | REFUSED | 时间窗 | slice-1-103/wave-1/object-source-replay-arm64.receipt reason=identity-changed-during-wave; child_seconds=13; 收据SHA=匹配 | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| object-source-replay-arm64-bounds | REFUSED | 时间窗 | slice-1-104/wave-1/object-source-replay-arm64-bounds.receipt reason=identity-changed-during-wave; child_seconds=16; 收据SHA=匹配 | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| object-source-replay-arm64-identity | REFUSED | 时间窗 | slice-1-104/wave-1/object-source-replay-arm64-identity.receipt reason=identity-changed-during-wave; child_seconds=16; 收据SHA=匹配 | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| object-source-replay-bounds | REFUSED | 时间窗 | slice-1-104/wave-1/object-source-replay-bounds.receipt reason=identity-changed-during-wave; child_seconds=16; 收据SHA=匹配 | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| object-source-replay-identity | REFUSED | 时间窗 | slice-1-104/wave-1/object-source-replay-identity.receipt reason=identity-changed-during-wave; child_seconds=16; 收据SHA=匹配 | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| objects-duplicate-definition | UNKNOWN | 时间窗 | slice-1-62/wave-2/objects-duplicate-definition.receipt reason=watchdog-deadline; child_seconds=16; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| objects-link | UNKNOWN | 时间窗 | slice-2-127/wave-2/objects-link.receipt reason=watchdog-deadline; child_seconds=9; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| objects-missing-definition | UNKNOWN | 时间窗 | slice-1-62/wave-2/objects-missing-definition.receipt reason=watchdog-deadline; child_seconds=16; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| objects-mixed-target | UNKNOWN | 时间窗 | slice-1-61/wave-2/objects-mixed-target.receipt reason=watchdog-deadline; child_seconds=17; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| objects-old-unit-version | UNKNOWN | 时间窗 | slice-1-62/wave-2/objects-old-unit-version.receipt reason=watchdog-deadline; child_seconds=16; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| objects-unsupported-win-arm64 | UNKNOWN | 时间窗 | slice-1-62/wave-2/objects-unsupported-win-arm64.receipt reason=watchdog-deadline; child_seconds=16; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| objects-unsupported-win-x86_64 | UNKNOWN | 时间窗 | slice-1-62/wave-2/objects-unsupported-win-x86_64.receipt reason=watchdog-deadline; child_seconds=16; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| observe-agents | UNKNOWN | 时间窗 | slice-1-90.log/wave.log: insufficient-wave-window；slice-1-90.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| on-change | UNKNOWN | 时间窗 | slice-0-45/wave-1/on-change.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| opt-frame4-o1-negative-end | UNKNOWN | 材料 F4b | slice-1-114.log/wave.log:缺映射 o1-run(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| opt-frame4-o1-negative-eof | UNKNOWN | 材料 F4b | slice-1-114.log/wave.log:缺映射 o1-run(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| opt-frame4-o1-negative-label | UNKNOWN | 材料 F4b | slice-1-114.log/wave.log:缺映射 o1-run(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| opt-frame4-o1-positive | UNKNOWN | 材料 F4b | slice-1-114.log/wave.log:缺映射 o1-run(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| opt-frame4-o2-negative-end | UNKNOWN | 材料 F4b | slice-1-114.log/wave.log:缺映射 o2-run(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| opt-frame4-o2-negative-eof | UNKNOWN | 材料 F4b | slice-1-115.log/wave.log:缺映射 o2-run(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| opt-frame4-o2-negative-label | UNKNOWN | 材料 F4b | slice-1-114.log/wave.log:缺映射 o2-run(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| opt-frame4-o2-positive | UNKNOWN | 材料 F4b | slice-1-114.log/wave.log:缺映射 o2-run(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| optimization-ccdiff | UNKNOWN | 时间窗 | slice-1-68/wave-2/optimization-ccdiff.receipt reason=watchdog-deadline; child_seconds=10; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| optimization-ccdiff-2 | UNKNOWN | 时间窗 | slice-1-68/wave-2/optimization-ccdiff-2.receipt reason=watchdog-deadline; child_seconds=10; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| optimization-ccdiff-3 | UNKNOWN | 时间窗 | slice-1-68/wave-2/optimization-ccdiff-3.receipt reason=watchdog-deadline; child_seconds=10; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| optimization-ccdiff-4 | UNKNOWN | 时间窗 | slice-1-68/wave-2/optimization-ccdiff-4.receipt reason=watchdog-deadline; child_seconds=10; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| optimization-cli | UNKNOWN | 材料 F4b | slice-1-66.log/wave.log:缺映射 ref(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| optimization-closure | UNKNOWN | 材料 F4b | slice-1-66.log/wave.log:缺映射 ref(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| optimization-corpus-0 | UNKNOWN | 材料 F4b | slice-1-65.log/wave.log:缺映射 ref(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| optimization-corpus-1 | UNKNOWN | 材料 F4b | slice-1-66.log/wave.log:缺映射 ref(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| optimization-corpus-2 | UNKNOWN | 材料 F4b | slice-1-66.log/wave.log:缺映射 ref(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| optimization-corpus-3 | UNKNOWN | 材料 F4b | slice-1-66.log/wave.log:缺映射 ref(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| optimization-corpus-4 | UNKNOWN | 材料 F4b | slice-1-66.log/wave.log:缺映射 ref(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| optimization-corpus-5 | UNKNOWN | 材料 F4b | slice-1-66.log/wave.log:缺映射 ref(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| optimization-corpus-6 | UNKNOWN | 材料 F4b | slice-1-66.log/wave.log:缺映射 ref(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| optimization-corpus-7 | UNKNOWN | 材料 F4b | slice-1-67.log/wave.log:缺映射 ref(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| optimization-corpus-8 | UNKNOWN | 材料 F4b | slice-1-67.log/wave.log:缺映射 ref(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| optimization-corpus-9 | UNKNOWN | 材料 F4b | slice-1-67.log/wave.log:缺映射 ref(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| optimization-native | UNKNOWN | 时间窗 | slice-1-61/wave-2/optimization-native.receipt reason=watchdog-deadline; child_seconds=17; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| optimization-run | UNKNOWN | 时间窗 | slice-1-61/wave-2/optimization-run.receipt reason=watchdog-deadline; child_seconds=17; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| optimization-tape | UNKNOWN | 时间窗 | slice-1-61/wave-2/optimization-tape.receipt reason=watchdog-deadline; child_seconds=17; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| origin-package | UNKNOWN | 材料待归类 | slice-2-127.log/wave.log:缺映射 source-origin(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| out-gc | UNKNOWN | 时间窗 | slice-1-74/wave-1/out-gc.receipt reason=watchdog-deadline; child_seconds=8; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| out-writers | UNKNOWN | 时间窗 | slice-0-45/wave-1/out-writers.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| own-diff-tool | UNKNOWN | 时间窗 | slice-1-73.log/wave.log: bound timeout；slice-1-73.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| own-escapes | UNKNOWN | 时间窗 | slice-0-46.log/wave.log: insufficient-wave-window；slice-0-46.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| p3-library-api | UNKNOWN | 材料待归类 | slice-1-62.log/wave.log:缺映射 p3-pack-ctx(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| p3-library-members | UNKNOWN | 材料待归类 | slice-1-62.log/wave.log:缺映射 p3-pack-ctx(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| p3-library-members-cxcodec | UNKNOWN | 材料待归类 | slice-1-62.log/wave.log:缺映射 p3-pack-ctx(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| p3-library-members-cxruntime | UNKNOWN | 材料待归类 | slice-1-69.log/wave.log:缺映射 p3-pack-ctx(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| p3-member-consume | UNKNOWN | 材料待归类 | slice-2-128.log/wave.log:缺映射 p3-source-ctx(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| p3-nn-asm-provenance | UNKNOWN | 材料待归类 | slice-2-128.log/wave.log:缺映射 p3-source-ctx(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| p3-swap | UNKNOWN | 已修待复核 | slice-1-84/wave.log:cx-plan拒check-utf8.cx，连带本批 | 6929ca4d91 | 旧utf8模块入口阻塞已修，仍须新根实跑，非现成PASS |
| pack-srcdir | UNKNOWN | 时间窗 | slice-0-44/wave-1/pack-srcdir.receipt reason=watchdog-deadline; child_seconds=10; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| package-controls | UNKNOWN | 材料待归类 | slice-1-85.log/wave.log:缺映射 baseline(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| phase-graph | UNKNOWN | 时间窗 | slice-0-42/wave-1/phase-graph.receipt reason=watchdog-deadline; child_seconds=28; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| phase-times-calc | UNKNOWN | 时间窗 | slice-0-35/wave-1/phase-times-calc.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| phase-timing-external | UNKNOWN | 时间窗 | slice-1-63.log/wave.log: insufficient-wave-window；slice-1-63.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| phase-timing-stamps | UNKNOWN | 时间窗 | slice-1-63.log/wave.log: insufficient-wave-window；slice-1-63.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| pins | FAIL | 已修待复核 | slice-1-81/wave-1/pins.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | 61e730f9d8；43378d1a91(新增F4c占位) | 旧三处及新增F4c两处均申报；pins已实跑failed=0 stale=29，新根仍须复核 |
| pkg-compile | UNKNOWN | 时间窗 | slice-0-25.log/wave.log: operation-deadline；slice-0-25.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| pkg-driver | UNKNOWN | 时间窗 | slice-0-25.log/wave.log: operation-deadline；slice-0-25.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| pkg-plan | UNKNOWN | 时间窗 | slice-0-25.log/wave.log: operation-deadline；slice-0-25.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| pkg-runtime | UNKNOWN | 时间窗 | slice-0-25.log/wave.log: operation-deadline；slice-0-25.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| pkg-stream | UNKNOWN | 时间窗 | slice-0-23/wave-1/pkg-stream.receipt reason=watchdog-deadline; child_seconds=26; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| pp-cli-includes | UNKNOWN | 时间窗 | slice-1-54.log/wave.log: operation-deadline；slice-1-54.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| pp-cli-macros | UNKNOWN | 时间窗 | slice-1-54.log/wave.log: operation-deadline；slice-1-54.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| pp-cli-targets | UNKNOWN | 时间窗 | slice-1-54.log/wave.log: operation-deadline；slice-1-54.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| pp-differential | UNKNOWN | 时间窗 | slice-1-68/wave-2/pp-differential.receipt reason=watchdog-deadline; child_seconds=10; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| pp-gold-controls | UNKNOWN | 时间窗 | slice-1-55.log/wave.log: bound timeout；slice-1-55.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| pp-gold-material | UNKNOWN | 时间窗 | slice-0-49/wave-1/pp-gold-material.receipt reason=watchdog-deadline; child_seconds=20; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| pp-header-locations | UNKNOWN | 时间窗 | slice-1-54.log/wave.log: operation-deadline；slice-1-54.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| pp-negative | UNKNOWN | 时间窗 | slice-1-68/wave-2/pp-negative.receipt reason=watchdog-deadline; child_seconds=10; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| pp-source-constructed | UNKNOWN | 时间窗 | slice-1-55.log/wave.log: bound timeout；slice-1-55.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| prechain-gate | UNKNOWN | 时间窗 | slice-0-37/wave-1/prechain-gate.receipt reason=watchdog-deadline; child_seconds=22; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| precommit-coverage-block | UNKNOWN | 时间窗 | slice-0-37/wave-1/precommit-coverage-block.receipt reason=watchdog-deadline; child_seconds=22; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| precommit-coverage-pass | UNKNOWN | 时间窗 | slice-0-37/wave-1/precommit-coverage-pass.receipt reason=watchdog-deadline; child_seconds=22; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| precommit-lint-r1-report | UNKNOWN | 时间窗 | slice-0-14.log/wave.log: sid-measurement；slice-0-14.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| precommit-lint-r2-sep | UNKNOWN | 时间窗 | slice-0-14.log/wave.log: sid-measurement；slice-0-14.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| precommit-lint-r3-die | UNKNOWN | 时间窗 | slice-0-15.log/wave.log: operation-deadline；slice-0-15.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| precommit-preview | UNKNOWN | 时间窗 | slice-0-47/wave-1/precommit-preview.receipt reason=watchdog-deadline; child_seconds=21; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| precommit-toolsloc | UNKNOWN | 时间窗 | slice-1-52/wave-1/precommit-toolsloc.receipt reason=watchdog-deadline; child_seconds=22; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| precommit-toolsloc-baseline-launder | UNKNOWN | 时间窗 | slice-1-109.log/wave.log: insufficient-wave-window；slice-1-109.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| precommit-toolsloc-broken-baseline | UNKNOWN | 时间窗 | slice-1-109.log/wave.log: insufficient-wave-window；slice-1-109.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| precommit-toolsloc-indexed-repair | UNKNOWN | 时间窗 | slice-1-108/wave-1/precommit-toolsloc-indexed-repair.receipt reason=watchdog-deadline; child_seconds=5; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| precommit-toolsloc-missing-baseline | UNKNOWN | 时间窗 | slice-1-109.log/wave.log: insufficient-wave-window；slice-1-109.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| precommit-toolsloc-new-601 | UNKNOWN | 时间窗 | slice-1-108/wave-1/precommit-toolsloc-new-601.receipt reason=watchdog-deadline; child_seconds=5; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| precommit-toolsloc-new-check | UNKNOWN | 时间窗 | slice-1-109.log/wave.log: insufficient-wave-window；slice-1-109.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| precommit-toolsloc-per-file | UNKNOWN | 时间窗 | slice-1-108/wave-1/precommit-toolsloc-per-file.receipt reason=watchdog-deadline; child_seconds=5; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| precommit-toolsloc-source | UNKNOWN | 时间窗 | slice-1-52/wave-1/precommit-toolsloc-source.receipt reason=watchdog-deadline; child_seconds=22; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| precommit-toolsloc-source-bash | UNKNOWN | 时间窗 | slice-1-109.log/wave.log: insufficient-wave-window；slice-1-109.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| precommit-toolsloc-source-entry-registration | UNKNOWN | 时间窗 | slice-1-110/wave-1/precommit-toolsloc-source-entry-registration.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| precommit-toolsloc-source-new-cli | UNKNOWN | 时间窗 | slice-1-109.log/wave.log: insufficient-wave-window；slice-1-109.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| precommit-toolsloc-source-shebang | UNKNOWN | 时间窗 | slice-1-109.log/wave.log: insufficient-wave-window；slice-1-109.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| precommit-toolsloc-source-source-registration | UNKNOWN | 时间窗 | slice-1-109.log/wave.log: insufficient-wave-window；slice-1-109.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| precommit-toolsloc-source-working-batches | UNKNOWN | 时间窗 | slice-1-109.log/wave.log: insufficient-wave-window；slice-1-109.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| precommit-toolsloc-source-working-check | UNKNOWN | 时间窗 | slice-1-109.log/wave.log: insufficient-wave-window；slice-1-109.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| precommit-toolsloc-symlink | UNKNOWN | 时间窗 | slice-1-52/wave-1/precommit-toolsloc-symlink.receipt reason=watchdog-deadline; child_seconds=22; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| precommit-toolsloc-symlink-new-regular | UNKNOWN | 时间窗 | slice-1-110/wave-1/precommit-toolsloc-symlink-new-regular.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| precommit-toolsloc-total-report | UNKNOWN | 时间窗 | slice-1-108/wave-1/precommit-toolsloc-total-report.receipt reason=watchdog-deadline; child_seconds=5; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| preflight-tool | UNKNOWN | 时间窗 | slice-0-15.log/wave.log: operation-deadline；slice-0-15.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| procrun-tool | UNKNOWN | 时间窗 | slice-0-15.log/wave.log: operation-deadline；slice-0-15.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| product-map | UNKNOWN | 时间窗 | slice-0-15.log/wave.log: operation-deadline；slice-0-15.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| product-map-fx-input | REFUSED | 时间窗 | slice-1-118/wave-1/product-map-fx-input.receipt reason=identity-changed-during-wave; child_seconds=10; 收据SHA=匹配 | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| product-map-fx-prd-red | REFUSED | 时间窗 | slice-1-118/wave-1/product-map-fx-prd-red.receipt reason=identity-changed-during-wave; child_seconds=10; 收据SHA=匹配 | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| product-map-fx-prd-restore | REFUSED | 时间窗 | slice-1-118/wave-1/product-map-fx-prd-restore.receipt reason=identity-changed-during-wave; child_seconds=10; 收据SHA=匹配 | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| product-map-fx-unrelated | REFUSED | 时间窗 | slice-1-118/wave-1/product-map-fx-unrelated.receipt reason=identity-changed-during-wave; child_seconds=10; 收据SHA=匹配 | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| productpkg-dir-identity | UNKNOWN | 时间窗 | slice-0-24/wave-1/productpkg-dir-identity.receipt reason=watchdog-deadline; child_seconds=19; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| properties | UNKNOWN | 时间窗 | slice-1-63.log/wave.log: insufficient-wave-window；slice-1-63.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| properties-cases-bound | UNKNOWN | 时间窗 | slice-1-65/wave-2/properties-cases-bound.receipt reason=watchdog-deadline; child_seconds=11; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| properties-cases-det | UNKNOWN | 时间窗 | slice-1-65/wave-2/properties-cases-det.receipt reason=watchdog-deadline; child_seconds=11; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| properties-cases-layout-len12 | UNKNOWN | 材料 F4b | slice-1-66.log/wave.log:缺映射 ref(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| properties-cases-layout-len3-0 | UNKNOWN | 材料 F4b | slice-1-67.log/wave.log:缺映射 ref(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| properties-cases-layout-len3-1 | UNKNOWN | 材料 F4b | slice-1-67.log/wave.log:缺映射 ref(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| properties-cases-layout-len3-2 | UNKNOWN | 材料 F4b | slice-1-67.log/wave.log:缺映射 ref(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| properties-cases-layout-len3-3 | UNKNOWN | 材料 F4b | slice-1-67.log/wave.log:缺映射 ref(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| properties-cases-layout-len3-4 | UNKNOWN | 材料 F4b | slice-1-67.log/wave.log:缺映射 ref(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| properties-cases-layout-len3-5 | UNKNOWN | 材料 F4b | slice-1-67.log/wave.log:缺映射 ref(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| properties-cases-lower | UNKNOWN | 材料 F4b | slice-1-65.log/wave.log:缺映射 ref(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| properties-cases-meta | UNKNOWN | 时间窗 | slice-1-65/wave-2/properties-cases-meta.receipt reason=watchdog-deadline; child_seconds=11; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| properties-cases-random | UNKNOWN | 材料 F4b | slice-1-66.log/wave.log:缺映射 ref(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| properties-cases-tu | UNKNOWN | 时间窗 | slice-1-65/wave-2/properties-cases-tu.receipt reason=watchdog-deadline; child_seconds=11; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| prune-identity | UNKNOWN | 时间窗 | slice-0-48/wave-1/prune-identity.receipt reason=watchdog-deadline; child_seconds=24; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| qsort-intro | UNKNOWN | 时间窗 | slice-0-41/wave-1/qsort-intro.receipt reason=watchdog-deadline; child_seconds=21; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| random-expr | UNKNOWN | 已修待复核 | slice-1-93/wave-1/random-expr.receipt reason=watchdog-deadline; child_seconds=12; 收据SHA=匹配 | 66505c70d7 + 7cf4705e22 | 无tcc则材料准备HOST_TOOL_MISSING/DEFERRED；s9未备材料仍UNKNOWN；不得NA或替换gcc/clang |
| ratchet-read-audit | UNKNOWN | 时间窗 | slice-1-120/wave-1/ratchet-read-audit.receipt reason=watchdog-deadline; child_seconds=9; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| ratchets-tool | UNKNOWN | 时间窗 | slice-1-71/wave-1/ratchets-tool.receipt reason=watchdog-deadline; child_seconds=24; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| rb-opguard | UNKNOWN | 时间窗 | slice-0-45/wave-1/rb-opguard.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| real-input-check | UNKNOWN | 时间窗 | slice-0-15.log/wave.log: operation-deadline；slice-0-15.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| rechunk | UNKNOWN | 时间窗 | slice-0-14.log/wave.log: sid-measurement；slice-0-14.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| refactor-inject | UNKNOWN | 时间窗 | slice-1-85/wave-1/refactor-inject.receipt reason=watchdog-deadline; child_seconds=14; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| refusals-gen | UNKNOWN | 时间窗 | slice-0-42/wave-1/refusals-gen.receipt reason=watchdog-deadline; child_seconds=28; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| reg-manifest | UNKNOWN | 时间窗 | slice-0-40/wave-1/reg-manifest.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| register-args | UNKNOWN | 时间窗 | slice-1-106/wave-1/register-args.receipt reason=watchdog-deadline; child_seconds=15; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| register-metadata-rollback | UNKNOWN | 已修待复核 | slice-0-11/wave.log:异平台批拒绝连带本批 | 1c52ddf1aa | 本机批不再受该异平台批拖累(推测)，仍须实跑；异平台独立NOT_APPLICABLE |
| register-root-preflight | UNKNOWN | 时间窗 | slice-0-15.log/wave.log: operation-deadline；slice-0-15.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| reject-locate | UNKNOWN | 时间窗 | slice-1-85/wave-1/reject-locate.receipt reason=watchdog-deadline; child_seconds=14; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| rel | UNKNOWN | 时间窗 | slice-0-50/wave-1/rel.receipt reason=watchdog-deadline; child_seconds=22; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| release-assetguard | UNKNOWN | 时间窗 | slice-1-125.log/wave.log: hard deadline；slice-1-125.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| release-assetguard-export | UNKNOWN | 时间窗 | slice-1-125.log/wave.log: hard deadline；slice-1-125.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| release-assetguard-export-missing | UNKNOWN | 时间窗 | slice-1-126.log/wave.log: hard deadline；slice-1-126.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| release-assetguard-refuse | UNKNOWN | 时间窗 | slice-1-125.log/wave.log: hard deadline；slice-1-125.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| release-assetguard-resume | UNKNOWN | 时间窗 | slice-1-125.log/wave.log: hard deadline；slice-1-125.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| release-ctor | UNKNOWN | 材料 F4c | slice-2-134.log/wave.log:缺映射 release-ctor-location(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| release-ctor-ctor-invalid | UNKNOWN | 材料 F4c | slice-2-134.log/wave.log:缺映射 release-ctor-location(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| release-ctor-ctor-missing | UNKNOWN | 材料 F4c | slice-2-134.log/wave.log:缺映射 release-ctor-location(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| release-ctor-ctor-positive | UNKNOWN | 材料 F4c | slice-2-134.log/wave.log:缺映射 release-ctor-location(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| release-ctor-duplicate-init | UNKNOWN | 材料 F4c | slice-2-136.log/wave.log:缺映射 release-ctor-location(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| release-ctor-missing-binding | UNKNOWN | 材料 F4c | slice-2-134.log/wave.log:缺映射 release-ctor-location(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| release-ctor-missing-binding-proof | UNKNOWN | 材料 F4c | slice-2-135.log/wave.log:缺映射 release-ctor-location(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| release-ctor-missing-checker | UNKNOWN | 材料 F4c | slice-2-135.log/wave.log:缺映射 release-ctor-location(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| release-ctor-missing-epoch | UNKNOWN | 材料 F4c | slice-2-134.log/wave.log:缺映射 release-ctor-location(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| release-ctor-missing-guard-proof | UNKNOWN | 材料 F4c | slice-2-135.log/wave.log:缺映射 release-ctor-location(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| release-ctor-missing-models | UNKNOWN | 材料 F4c | slice-2-135.log/wave.log:缺映射 release-ctor-location(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| release-ctor-missing-models-proof | UNKNOWN | 材料 F4c | slice-2-135.log/wave.log:缺映射 release-ctor-location(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| release-ctor-missing-resources | UNKNOWN | 材料 F4c | slice-2-136.log/wave.log:缺映射 release-ctor-location(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| release-ctor-missing-resources-proof | UNKNOWN | 材料 F4c | slice-2-136.log/wave.log:缺映射 release-ctor-location(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| release-ctor-nets-alias | UNKNOWN | 材料 F4c | slice-2-136.log/wave.log:缺映射 release-ctor-location(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| release-ctor-out-alias | UNKNOWN | 材料 F4c | slice-2-136.log/wave.log:缺映射 release-ctor-location(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| release-ctor-root-alias | UNKNOWN | 材料 F4c | slice-2-136.log/wave.log:缺映射 release-ctor-location(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| release-ctor-run-alias | UNKNOWN | 材料 F4c | slice-2-136.log/wave.log:缺映射 release-ctor-location(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| release-ctor-source-changed | UNKNOWN | 材料 F4c | slice-2-136.log/wave.log:缺映射 release-ctor-location(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| release-ctor-tamper-binding | UNKNOWN | 材料 F4c | slice-2-134.log/wave.log:缺映射 release-ctor-location(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| release-ctor-tamper-binding-proof | UNKNOWN | 材料 F4c | slice-2-135.log/wave.log:缺映射 release-ctor-location(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| release-ctor-tamper-checker | UNKNOWN | 材料 F4c | slice-2-135.log/wave.log:缺映射 release-ctor-location(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| release-ctor-tamper-epoch | UNKNOWN | 材料 F4c | slice-2-134.log/wave.log:缺映射 release-ctor-location(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| release-ctor-tamper-guard-proof | UNKNOWN | 材料 F4c | slice-2-135.log/wave.log:缺映射 release-ctor-location(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| release-ctor-tamper-models | UNKNOWN | 材料 F4c | slice-2-135.log/wave.log:缺映射 release-ctor-location(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| release-ctor-tamper-models-proof | UNKNOWN | 材料 F4c | slice-2-135.log/wave.log:缺映射 release-ctor-location(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| release-ctor-tamper-resources | UNKNOWN | 材料 F4c | slice-2-136.log/wave.log:缺映射 release-ctor-location(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| release-ctor-tamper-resources-proof | UNKNOWN | 材料 F4c | slice-2-136.log/wave.log:缺映射 release-ctor-location(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| release-decide | UNKNOWN | 时间窗 | slice-0-13.log/wave.log: operation-deadline；slice-0-13.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| release-dispatch | UNKNOWN | 时间窗 | slice-0-30/wave-1/release-dispatch.receipt reason=watchdog-deadline; child_seconds=24; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| release-exec | UNKNOWN | 时间窗 | slice-1-122.log/wave.log: bound timeout；slice-1-122.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| release-exec-ystep | UNKNOWN | 时间窗 | slice-1-125.log/wave.log: hard deadline；slice-1-125.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| release-layout | REFUSED | 时间窗 | slice-1-92/wave-1/release-layout.receipt reason=identity-changed-during-wave; child_seconds=19; 收据SHA=匹配 | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| release-oneshot | UNKNOWN | 时间窗 | slice-1-122.log/wave.log: bound timeout；slice-1-122.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| release-prepare | UNKNOWN | 时间窗 | slice-1-122.log/wave.log: bound timeout；slice-1-122.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| release-run | UNKNOWN | 材料 F4c | slice-2-134.log/wave.log:缺映射 release-fixture(具名NOT_AVAIL) | a4990c8a7e(条件处理) | 有效封存daily-no-candidate声明和材料表、无候选且白名单材料未映射才NA；有候选/release/缺表仍UNKNOWN或REFUSED |
| release-sweep | UNKNOWN | 时间窗 | slice-0-20/wave-1/release-sweep.receipt reason=watchdog-deadline; child_seconds=14; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| release-usage | UNKNOWN | 时间窗 | slice-0-14.log/wave.log: sid-measurement；slice-0-14.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| release-version | UNKNOWN | 时间窗 | slice-0-13.log/wave.log: operation-deadline；slice-0-13.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| release-version-pack | UNKNOWN | 时间窗 | slice-1-117/wave-1/release-version-pack.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| releaseversion | UNKNOWN | 时间窗 | slice-0-13.log/wave.log: operation-deadline；slice-0-13.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| relload | UNKNOWN | 时间窗 | slice-0-49/wave-1/relload.receipt reason=watchdog-deadline; child_seconds=20; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| relscan | UNKNOWN | 时间窗 | slice-0-50/wave-1/relscan.receipt reason=watchdog-deadline; child_seconds=22; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| relsha-lib | REFUSED | 时间窗 | slice-1-118/wave-1/relsha-lib.receipt reason=identity-changed-during-wave; child_seconds=10; 收据SHA=匹配 | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| relstore | UNKNOWN | 时间窗 | slice-0-50/wave-1/relstore.receipt reason=watchdog-deadline; child_seconds=22; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| report-verify-tool | UNKNOWN | 已修待复核 | slice-0-11/wave.log:异平台批拒绝连带本批 | 1c52ddf1aa | 本机批不再受该异平台批拖累(推测)，仍须实跑；异平台独立NOT_APPLICABLE |
| resources | UNKNOWN | 时间窗 | slice-1-57.log/wave.log: insufficient-wave-window；slice-1-57.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| resources-limits | UNKNOWN | 时间窗 | slice-1-75.log/wave.log: insufficient-wave-window；slice-1-75.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| ri-capture | UNKNOWN | 材料待归类 | slice-1-58.log/wave.log:缺映射 ri-capture(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| ri-chain-fiforeceipt | UNKNOWN | 材料 F4b | slice-1-61.log/wave.log:缺映射 ri-chain(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| ri-chain-missingnet | UNKNOWN | 材料 F4b | slice-1-61.log/wave.log:缺映射 ri-chain(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| ri-chain-nnflip | UNKNOWN | 材料 F4b | slice-1-61.log/wave.log:缺映射 ri-chain(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| ri-chain-receiptflip | UNKNOWN | 材料 F4b | slice-1-61.log/wave.log:缺映射 ri-chain(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| ri-chain-symlinknet | UNKNOWN | 材料 F4b | slice-1-61.log/wave.log:缺映射 ri-chain(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| ri-chain-tableflip | UNKNOWN | 材料 F4b | slice-1-61.log/wave.log:缺映射 ri-chain(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| ri-chain-valid | UNKNOWN | 材料 F4b | slice-1-60.log/wave.log:缺映射 ri-chain(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| ri-consumers | UNKNOWN | 材料待归类 | slice-1-58.log/wave.log:缺映射 ri-consumers(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| ri-flow-data-declared-empty-remove | UNKNOWN | 材料 F4b | slice-1-60.log/wave.log:缺映射 ri-flow-runtime(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| ri-flow-data-extra-empty | UNKNOWN | 材料 F4b | slice-1-59.log/wave.log:缺映射 ri-flow-runtime(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| ri-flow-e3-absent | UNKNOWN | 材料 F4b | slice-1-58.log/wave.log:缺映射 ri-flow-runtime(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| ri-flow-e3-empty | UNKNOWN | 材料 F4b | slice-1-58.log/wave.log:缺映射 ri-flow-runtime(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| ri-flow-e3-missing | UNKNOWN | 材料 F4b | slice-1-58.log/wave.log:缺映射 ri-flow-runtime(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| ri-flow-e3-to-lower | UNKNOWN | 材料 F4b | slice-1-58.log/wave.log:缺映射 ri-flow-runtime(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| ri-flow-profile-closure-field | UNKNOWN | 材料 F4b | slice-1-59.log/wave.log:缺映射 ri-flow-runtime(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| ri-flow-profile-elf-flip | UNKNOWN | 材料 F4b | slice-1-59.log/wave.log:缺映射 ri-flow-runtime(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| ri-flow-profile-final-flip | UNKNOWN | 材料 F4b | slice-1-59.log/wave.log:缺映射 ri-flow-runtime(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| ri-flow-profile-old-v1 | UNKNOWN | 材料 F4b | slice-1-59.log/wave.log:缺映射 ri-flow-runtime(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| ri-flow-profile-pending-flip | UNKNOWN | 材料 F4b | slice-1-59.log/wave.log:缺映射 ri-flow-runtime(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| ri-flow-profile-recompile | UNKNOWN | 材料 F4b | slice-1-59.log/wave.log:缺映射 ri-flow-runtime(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| ri-flow-profile-reseal | UNKNOWN | 材料 F4b | slice-1-59.log/wave.log:缺映射 ri-flow-runtime(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| ri-flow-profile-unsealed | UNKNOWN | 材料 F4b | slice-1-59.log/wave.log:缺映射 ri-flow-runtime(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| ri-flow-profile-valid | UNKNOWN | 材料 F4b | slice-1-59.log/wave.log:缺映射 ri-flow-runtime(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| ri-image-multi | UNKNOWN | 时间窗 | slice-1-57.log/wave.log: insufficient-wave-window；slice-1-57.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| ri-image-single | UNKNOWN | 时间窗 | slice-1-57.log/wave.log: insufficient-wave-window；slice-1-57.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| ri-material-control-valid | UNKNOWN | 材料 F4b | slice-1-60.log/wave.log:缺映射 ri-cli(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| ri-material-corrupt-flags-receipt | UNKNOWN | 材料 F4b | slice-1-60.log/wave.log:缺映射 ri-cli(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| ri-material-corrupt-runtime | UNKNOWN | 材料 F4b | slice-1-60.log/wave.log:缺映射 ri-cli(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| ri-material-fifo-receipt | UNKNOWN | 材料 F4b | slice-1-60.log/wave.log:缺映射 ri-cli(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| ri-material-missing-native-receipt | UNKNOWN | 材料 F4b | slice-1-60.log/wave.log:缺映射 ri-cli(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| ri-material-native-byte-flip | UNKNOWN | 材料 F4b | slice-1-60.log/wave.log:缺映射 ri-cli(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| ri-material-native-missing-include | UNKNOWN | 材料 F4b | slice-1-60.log/wave.log:缺映射 ri-cli(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| ri-material-symlink-native | UNKNOWN | 材料 F4b | slice-1-60.log/wave.log:缺映射 ri-cli(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| ri-multi | UNKNOWN | 时间窗 | slice-1-57.log/wave.log: insufficient-wave-window；slice-1-57.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| ri-standalone | UNKNOWN | 材料 F4b | slice-1-58.log/wave.log:缺映射 ri-cli(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| ri-trace | UNKNOWN | 时间窗 | slice-1-57.log/wave.log: insufficient-wave-window；slice-1-57.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| ri-trace-output | UNKNOWN | 时间窗 | slice-1-57.log/wave.log: insufficient-wave-window；slice-1-57.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| rollup-open | UNKNOWN | 时间窗 | slice-1-87/wave-1/rollup-open.receipt reason=watchdog-deadline; child_seconds=18; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| root-canon | UNKNOWN | 时间窗 | slice-0-14.log/wave.log: sid-measurement；slice-0-14.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| root-independent | UNKNOWN | 时间窗 | slice-0-14.log/wave.log: sid-measurement；slice-0-14.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| route-chains | UNKNOWN | 时间窗 | slice-1-73.log/wave.log: bound timeout；slice-1-73.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| rt-linkage | UNKNOWN | 时间窗 | slice-1-90.log/wave.log: insufficient-wave-window；slice-1-90.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| run-comm | REFUSED | 时间窗 | slice-1-92/wave.log:after-measurement-failed sid-measurement；本批无收据，波级拒绝连带；另缺映射run-comm-new，不能覆盖旧REFUSED | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| run-reserve | FAIL | 已修待复核 | slice-1-76/wave-1/run-reserve.receipt reason=batch-fail; child_seconds=21; 收据SHA=匹配 | 6929ca4d91(壳头源码) | 只换源不换a99ea71d产品仍FAIL；须用新ape-pack实际重打包，不能预测PASS |
| run-startup-bench | UNKNOWN | 时间窗 | slice-0-45/wave-1/run-startup-bench.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| run-tape | UNKNOWN | 时间窗 | slice-0-31/wave-1/run-tape.receipt reason=watchdog-deadline; child_seconds=19; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| run-tmpfile | REFUSED | 时间窗 | slice-1-88/wave-1/run-tmpfile.receipt reason=identity-changed-during-wave; child_seconds=19; 收据SHA=匹配 | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| runner-faults | UNKNOWN | 时间窗 | slice-1-75.log/wave.log: insufficient-wave-window；slice-1-75.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| runneridentity | UNKNOWN | 时间窗 | slice-0-47/wave-1/runneridentity.receipt reason=watchdog-deadline; child_seconds=21; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| runnerprep | UNKNOWN | 时间窗 | slice-0-18/wave-1/runnerprep.receipt reason=watchdog-deadline; child_seconds=23; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| s8c-bind | REFUSED | 时间窗 | slice-1-92/wave-1/s8c-bind.receipt reason=identity-changed-during-wave; child_seconds=19; 收据SHA=匹配 | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| s9-agg-parts | UNKNOWN | 时间窗 | slice-0-18/wave-1/s9-agg-parts.receipt reason=watchdog-deadline; child_seconds=23; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| s9-native | UNKNOWN | 时间窗 | slice-0-15.log/wave.log: operation-deadline；slice-0-15.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| s9-nightly | UNKNOWN | 时间窗 | slice-0-16.log/wave.log: after-measurement-failed sid-measurement；测量失败实测，争抢/超时因果推测；slice-0-16.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| s9-nightly-limits | UNKNOWN | 时间窗 | slice-0-17/wave-1/s9-nightly-limits.receipt reason=watchdog-deadline; child_seconds=44; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| s9-nightly-materials | UNKNOWN | 时间窗 | slice-0-47/wave-1/s9-nightly-materials.receipt reason=watchdog-deadline; child_seconds=21; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| s9-nightly-phases | UNKNOWN | 已修待复核 | slice-0-18/wave-1/s9-nightly-phases.receipt reason=watchdog-deadline; child_seconds=23; 收据SHA=匹配 | d729fae9af + b997444202 | 11组注册实跑最长33.863秒；shared/platform须真实all-waves-tool材料及SHA，缺失拒绝 |
| s9-nightly-resume | UNKNOWN | 时间窗 | slice-0-18/wave-1/s9-nightly-resume.receipt reason=watchdog-deadline; child_seconds=23; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| s9-nightly-run | UNKNOWN | 时间窗 | slice-0-16.log/wave.log: after-measurement-failed sid-measurement；测量失败实测，争抢/超时因果推测；slice-0-16.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| s9-nightly-run-fail | UNKNOWN | 时间窗 | slice-0-16.log/wave.log: after-measurement-failed sid-measurement；测量失败实测，争抢/超时因果推测；slice-0-16.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| s9-nightly-run-pass | UNKNOWN | 时间窗 | slice-0-18/wave-1/s9-nightly-run-pass.receipt reason=watchdog-deadline; child_seconds=23; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| s9-nightly-tiers | UNKNOWN | 时间窗 | slice-0-17/wave-1/s9-nightly-tiers.receipt reason=watchdog-deadline; child_seconds=44; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| s9-profile | UNKNOWN | 时间窗 | slice-0-39/wave-1/s9-profile.receipt reason=watchdog-deadline; child_seconds=23; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| s9-release-tier | UNKNOWN | 时间窗 | slice-1-119/wave-1/s9-release-tier.receipt reason=watchdog-deadline; child_seconds=18; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| s9-root | UNKNOWN | 时间窗 | slice-0-49/wave-1/s9-root.receipt reason=watchdog-deadline; child_seconds=20; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| s9-shards-flow | UNKNOWN | 时间窗 | slice-0-51/wave-1/s9-shards-flow.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| s9-shards-identity | UNKNOWN | 时间窗 | slice-0-16.log/wave.log: after-measurement-failed sid-measurement；测量失败实测，争抢/超时因果推测；slice-0-16.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| s9-shards-identity-sid | UNKNOWN | 时间窗 | slice-0-16.log/wave.log: after-measurement-failed sid-measurement；测量失败实测，争抢/超时因果推测；slice-0-16.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| s9-shards-inputs | UNKNOWN | 已修待复核 | slice-0-12/wave.log:异平台批拒绝连带本批 | 1c52ddf1aa | 本机批不再受该异平台批拖累(推测)，仍须实跑；异平台独立NOT_APPLICABLE |
| s9-shards-schedule | UNKNOWN | 已修待复核 | slice-1-117/wave-1/s9-shards-schedule.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | d729fae9af + b997444202 | 11组注册实跑最长33.863秒；shared/platform须真实all-waves-tool材料及SHA，缺失拒绝 |
| s9-verdict | UNKNOWN | 时间窗 | slice-0-44/wave-1/s9-verdict.receipt reason=watchdog-deadline; child_seconds=10; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| s9-worker-smoke | UNKNOWN | 时间窗 | slice-1-119/wave-1/s9-worker-smoke.receipt reason=watchdog-deadline; child_seconds=18; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| safedel-chains | UNKNOWN | 时间窗 | slice-0-41/wave-1/safedel-chains.receipt reason=watchdog-deadline; child_seconds=20; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| sessionroot | UNKNOWN | 时间窗 | slice-0-40/wave-1/sessionroot.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| sha-memo-chain | UNKNOWN | 时间窗 | slice-1-125.log/wave.log: hard deadline；slice-1-125.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| shared-frames | UNKNOWN | 时间窗 | slice-0-13.log/wave.log: operation-deadline；slice-0-13.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| signal-targets | UNKNOWN | 时间窗 | slice-1-91/wave-1/signal-targets.receipt reason=watchdog-deadline; child_seconds=6; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| sim-state-frame-edges | REFUSED | 时间窗 | slice-1-104/wave-1/sim-state-frame-edges.receipt reason=identity-changed-during-wave; child_seconds=16; 收据SHA=匹配 | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| sizeof-postfix-positive | UNKNOWN | 时间窗 | slice-0-32/wave-1/sizeof-postfix-positive.receipt reason=watchdog-deadline; child_seconds=23; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| sizeof-postfix-refusal | UNKNOWN | 时间窗 | slice-0-32/wave-1/sizeof-postfix-refusal.receipt reason=watchdog-deadline; child_seconds=23; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| skill-cx | REFUSED | 时间窗 | slice-1-88/wave-1/skill-cx.receipt reason=identity-changed-during-wave; child_seconds=19; 收据SHA=匹配 | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| slice-freeze-tool | UNKNOWN | 材料 F4a | slice-1-90.log/wave.log:缺映射 regprep-dir(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| slice-inventory | UNKNOWN | 材料 F4a | slice-1-91.log/wave.log:缺映射 regprep-dir(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| slice-inventory-identity | REFUSED | 时间窗 | slice-1-103/wave.log:after-measurement-failed sid-measurement；本批无收据，波级拒绝连带；另缺映射regprep-dir，不能覆盖旧REFUSED | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| slice-inventory-mapping | REFUSED | 时间窗 | slice-1-103/wave.log:after-measurement-failed sid-measurement；本批无收据，波级拒绝连带；另缺映射regprep-dir，不能覆盖旧REFUSED | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| slice-inventory-root | REFUSED | 时间窗 | slice-1-104/wave.log:after-measurement-failed sid-measurement；本批无收据，波级拒绝连带；另缺映射regprep-dir，不能覆盖旧REFUSED | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| slice-inventory-source | REFUSED | 时间窗 | slice-1-103/wave.log:after-measurement-failed sid-measurement；本批无收据，波级拒绝连带；另缺映射regprep-dir，不能覆盖旧REFUSED | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| slice-inventory-target | REFUSED | 时间窗 | slice-1-103/wave.log:after-measurement-failed sid-measurement；本批无收据，波级拒绝连带；另缺映射regprep-dir，不能覆盖旧REFUSED | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| slice-migration-gen | UNKNOWN | 已修待复核 | slice-1-93/wave-1/slice-migration-gen.receipt reason=watchdog-deadline; child_seconds=12; 收据SHA=匹配 | 61e730f9d8 | 已修检查/路径漂移，提交称相关批单跑过；新候选资格仍待新根实跑 |
| snap-residue | UNKNOWN | 时间窗 | slice-0-38/wave-1/snap-residue.receipt reason=watchdog-deadline; child_seconds=17; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| snap-tool-drift | UNKNOWN | 时间窗 | slice-0-38/wave-1/snap-tool-drift.receipt reason=watchdog-deadline; child_seconds=17; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| snapshot-run-tool | UNKNOWN | 时间窗 | slice-1-73.log/wave.log: bound timeout；slice-1-73.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| snapshots | UNKNOWN | 时间窗 | slice-1-63.log/wave.log: insufficient-wave-window；slice-1-63.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| snapshots-cx | UNKNOWN | 时间窗 | slice-1-52/wave-1/snapshots-cx.receipt reason=watchdog-deadline; child_seconds=22; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| source-anchor-controls | UNKNOWN | 时间窗 | slice-1-125.log/wave.log: hard deadline；slice-1-125.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| source-anchor-legacy-g2 | UNKNOWN | 时间窗 | slice-1-125.log/wave.log: hard deadline；slice-1-125.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| source-bom | UNKNOWN | 已修待复核 | slice-1-91/wave-1/source-bom.receipt reason=watchdog-deadline; child_seconds=6; 收据SHA=匹配 | 66505c70d7 | 已修自重入/归档路径/tape同路径对比；新根须实跑 |
| source-ident-helper | UNKNOWN | 时间窗 | slice-0-14.log/wave.log: sid-measurement；slice-0-14.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| source-ident-helper-missing | UNKNOWN | 时间窗 | slice-0-17/wave-1/source-ident-helper-missing.receipt reason=watchdog-deadline; child_seconds=44; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| source-job-count | UNKNOWN | 时间窗 | slice-0-30/wave-1/source-job-count.receipt reason=watchdog-deadline; child_seconds=24; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| source-job-union | UNKNOWN | 材料待归类 | slice-2-127.log/wave.log:缺映射 source-job-context(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| source-package-mixed-profiles | UNKNOWN | 时间窗 | slice-0-32/wave-1/source-package-mixed-profiles.receipt reason=watchdog-deadline; child_seconds=23; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| source-recorded-jobs | UNKNOWN | 材料待归类 | slice-2-127.log/wave.log:缺映射 source-job-context(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| speed-probe | UNKNOWN | 时间窗 | slice-1-81/wave-1/speed-probe.receipt reason=watchdog-deadline; child_seconds=23; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| st-gap-gen | UNKNOWN | 时间窗 | slice-0-44/wave-1/st-gap-gen.receipt reason=watchdog-deadline; child_seconds=10; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stable-eval | UNKNOWN | 时间窗 | slice-0-31/wave-1/stable-eval.receipt reason=watchdog-deadline; child_seconds=19; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stable-gaps | UNKNOWN | 时间窗 | slice-0-38/wave-1/stable-gaps.receipt reason=watchdog-deadline; child_seconds=17; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stages | UNKNOWN | 时间窗 | slice-1-87/wave-1/stages.receipt reason=watchdog-deadline; child_seconds=18; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stale-tool | UNKNOWN | 时间窗 | slice-1-73.log/wave.log: bound timeout；slice-1-73.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| state-gen | UNKNOWN | 时间窗 | slice-0-45/wave-1/state-gen.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| status | UNKNOWN | 时间窗 | slice-0-43/wave-1/status.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| std-surface-v2 | UNKNOWN | 时间窗 | slice-0-32/wave-1/std-surface-v2.receipt reason=watchdog-deadline; child_seconds=23; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stdio | UNKNOWN | 时间窗 | slice-1-77/wave-1/stdio.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stdio-buffer | UNKNOWN | 时间窗 | slice-0-41/wave-1/stdio-buffer.receipt reason=watchdog-deadline; child_seconds=21; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stdio-crossunit | UNKNOWN | 时间窗 | slice-1-87/wave-1/stdio-crossunit.receipt reason=watchdog-deadline; child_seconds=18; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| step-budget | UNKNOWN | 时间窗 | slice-0-31/wave-1/step-budget.receipt reason=watchdog-deadline; child_seconds=19; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-cells | REFUSED | 时间窗 | slice-1-88/wave-1/stream-cells.receipt reason=identity-changed-during-wave; child_seconds=19; 收据SHA=匹配 | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| stream-e1-independent | UNKNOWN | 时间窗 | slice-1-55.log/wave.log: bound timeout；slice-1-55.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-e1-lex-bytes | UNKNOWN | 时间窗 | slice-1-64.log/wave.log: insufficient-wave-window；slice-1-64.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-e1-lex-refusals | UNKNOWN | 时间窗 | slice-1-64.log/wave.log: insufficient-wave-window；slice-1-64.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-e1-lex-tokens | UNKNOWN | 时间窗 | slice-1-63.log/wave.log: insufficient-wave-window；slice-1-63.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-e2-domain-positive | UNKNOWN | 时间窗 | slice-1-63.log/wave.log: insufficient-wave-window；slice-1-63.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-e2-domain-refusal | UNKNOWN | 时间窗 | slice-1-63.log/wave.log: insufficient-wave-window；slice-1-63.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-e2-independent | UNKNOWN | 时间窗 | slice-1-55.log/wave.log: bound timeout；slice-1-55.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-e2-pp-gold-linux | UNKNOWN | 时间窗 | slice-1-55.log/wave.log: bound timeout；slice-1-55.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-e3-gold | UNKNOWN | 时间窗 | slice-1-74/wave-1/stream-e3-gold.receipt reason=watchdog-deadline; child_seconds=8; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-e3-independent | UNKNOWN | 时间窗 | slice-1-55.log/wave.log: bound timeout；slice-1-55.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-e3-pos | UNKNOWN | 时间窗 | slice-1-74/wave-1/stream-e3-pos.receipt reason=watchdog-deadline; child_seconds=8; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-e3-ref | UNKNOWN | 时间窗 | slice-1-75.log/wave.log: insufficient-wave-window；slice-1-75.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-e3-scope | UNKNOWN | 时间窗 | slice-1-75.log/wave.log: insufficient-wave-window；slice-1-75.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-e3-unmap | UNKNOWN | 时间窗 | slice-1-75.log/wave.log: insufficient-wave-window；slice-1-75.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-e4-independent | UNKNOWN | 时间窗 | slice-1-56.log/wave.log: insufficient-wave-window；slice-1-56.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-e4-io | UNKNOWN | 时间窗 | slice-1-69/wave-2/stream-e4-io.receipt reason=watchdog-deadline; child_seconds=12; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-e4-io-cc | UNKNOWN | 材料 F4b | slice-1-69.log/wave.log:缺映射 streams-cc(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| stream-e4-o1 | UNKNOWN | 材料待归类 | slice-1-69.log/wave.log:缺映射 streams-o1(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| stream-e4-o2 | UNKNOWN | 材料 F4b | slice-1-65.log/wave.log:缺映射 ref(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| stream-e4-opinfo-0 | UNKNOWN | 时间窗 | slice-1-65/wave-2/stream-e4-opinfo-0.receipt reason=watchdog-deadline; child_seconds=11; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-e4-opinfo-1 | UNKNOWN | 时间窗 | slice-1-65/wave-2/stream-e4-opinfo-1.receipt reason=watchdog-deadline; child_seconds=11; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-e4-peep-0 | UNKNOWN | 时间窗 | slice-1-64.log/wave.log: insufficient-wave-window；slice-1-64.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-e4-peep-1 | UNKNOWN | 时间窗 | slice-1-64.log/wave.log: insufficient-wave-window；slice-1-64.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-e4-peep-2 | UNKNOWN | 时间窗 | slice-1-64.log/wave.log: insufficient-wave-window；slice-1-64.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-e4-peep-3 | UNKNOWN | 时间窗 | slice-1-64.log/wave.log: insufficient-wave-window；slice-1-64.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-e4-peep-4 | UNKNOWN | 时间窗 | slice-1-64.log/wave.log: insufficient-wave-window；slice-1-64.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-e4-peep-5 | UNKNOWN | 时间窗 | slice-1-64.log/wave.log: insufficient-wave-window；slice-1-64.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-e4-peep-6 | UNKNOWN | 时间窗 | slice-1-64.log/wave.log: insufficient-wave-window；slice-1-64.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-e4-peep-7 | UNKNOWN | 时间窗 | slice-1-64.log/wave.log: insufficient-wave-window；slice-1-64.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-e4-semantics | UNKNOWN | 时间窗 | slice-1-69/wave-2/stream-e4-semantics.receipt reason=watchdog-deadline; child_seconds=12; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-elf-independent | UNKNOWN | 时间窗 | slice-1-57.log/wave.log: insufficient-wave-window；slice-1-57.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-lower-domain-format | UNKNOWN | 时间窗 | slice-1-56.log/wave.log: insufficient-wave-window；slice-1-56.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-lower-domain-input | UNKNOWN | 时间窗 | slice-1-56.log/wave.log: insufficient-wave-window；slice-1-56.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-lower-domain-layout-plain | UNKNOWN | 时间窗 | slice-1-56.log/wave.log: insufficient-wave-window；slice-1-56.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-lower-domain-layout-redef | UNKNOWN | 时间窗 | slice-1-56.log/wave.log: insufficient-wave-window；slice-1-56.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-lower-domain-refusals | UNKNOWN | 时间窗 | slice-1-56.log/wave.log: insufficient-wave-window；slice-1-56.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-lower-independent | UNKNOWN | 时间窗 | slice-1-56.log/wave.log: insufficient-wave-window；slice-1-56.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-lower-target-lnx-arm64 | UNKNOWN | 时间窗 | slice-1-56.log/wave.log: insufficient-wave-window；slice-1-56.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-lower-target-osx-arm64 | UNKNOWN | 时间窗 | slice-1-56.log/wave.log: insufficient-wave-window；slice-1-56.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-lower-target-osx-x86_64 | UNKNOWN | 时间窗 | slice-1-56.log/wave.log: insufficient-wave-window；slice-1-56.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-lower-target-win-arm64 | UNKNOWN | 时间窗 | slice-1-57.log/wave.log: insufficient-wave-window；slice-1-57.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-lower-target-win-x86_64 | UNKNOWN | 时间窗 | slice-1-57.log/wave.log: insufficient-wave-window；slice-1-57.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-model-identity | UNKNOWN | 时间窗 | slice-1-67/wave-2/stream-model-identity.receipt reason=watchdog-deadline; child_seconds=14; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-pipe | UNKNOWN | 时间窗 | slice-1-87/wave-1/stream-pipe.receipt reason=watchdog-deadline; child_seconds=18; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-prune-cases | FAIL | 已修待复核 | slice-1-65/wave-2/stream-prune-cases.receipt reason=batch-fail; child_seconds=11; 收据SHA=匹配 | 66505c70d7 | 已修自重入/归档路径/tape同路径对比；新根须实跑 |
| stream-prune-independent | UNKNOWN | 时间窗 | slice-1-57.log/wave.log: insufficient-wave-window；slice-1-57.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-prune-io | UNKNOWN | 时间窗 | slice-1-69/wave-2/stream-prune-io.receipt reason=watchdog-deadline; child_seconds=12; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-prune-io-cc | UNKNOWN | 材料 F4b | slice-1-69.log/wave.log:缺映射 streams-cc(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| stream-prune-library | UNKNOWN | 材料待归类 | slice-1-69.log/wave.log:缺映射 streams-cc2(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| stream-prune-route | UNKNOWN | 时间窗 | slice-0-2/wave-1/stream-prune-route.receipt reason=watchdog-deadline; child_seconds=14; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-prune-semantics | UNKNOWN | 时间窗 | slice-1-69/wave-2/stream-prune-semantics.receipt reason=watchdog-deadline; child_seconds=12; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| stream-target | UNKNOWN | 时间窗 | slice-1-91/wave-1/stream-target.receipt reason=watchdog-deadline; child_seconds=6; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| streams | UNKNOWN | 时间窗 | slice-1-55.log/wave.log: bound timeout；slice-1-55.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| streams-material | REFUSED | 时间窗 | slice-1-88/wave-1/streams-material.receipt reason=identity-changed-during-wave; child_seconds=19; 收据SHA=匹配 | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| submit-patch | UNKNOWN | 时间窗 | slice-0-16.log/wave.log: after-measurement-failed sid-measurement；测量失败实测，争抢/超时因果推测；slice-0-16.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| surface-doc-gen | UNKNOWN | 时间窗 | slice-0-38/wave-1/surface-doc-gen.receipt reason=watchdog-deadline; child_seconds=17; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| sweep-plan | UNKNOWN | 时间窗 | slice-0-44/wave-1/sweep-plan.receipt reason=watchdog-deadline; child_seconds=10; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| sy-tmpdir | UNKNOWN | 时间窗 | slice-1-90.log/wave.log: insufficient-wave-window；slice-1-90.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| symindex-tool | UNKNOWN | 时间窗 | slice-1-73.log/wave.log: bound timeout；slice-1-73.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| sys-reslive | UNKNOWN | 时间窗 | slice-0-46.log/wave.log: insufficient-wave-window；slice-0-46.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| tables-gen | UNKNOWN | 时间窗 | slice-0-40/wave-1/tables-gen.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| tape-neg | UNKNOWN | 时间窗 | slice-1-73.log/wave.log: bound timeout；slice-1-73.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| tapebin-consumer | UNKNOWN | 材料 F4b | slice-2-134.log/wave.log:缺映射 before-checker(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| tbin-wiring | UNKNOWN | 时间窗 | slice-1-77/wave-1/tbin-wiring.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| test-loader-com | REFUSED | 时间窗 | slice-1-92/wave.log:after-measurement-failed sid-measurement；本批无收据，波级拒绝连带；另缺映射loader6，不能覆盖旧REFUSED | 21f95007be(仅缓解) | 旧REFUSED不追溯改标；测量失败因果与新根终态待复核，降并发收益为推测 |
| testplan-controls | UNKNOWN | 时间窗 | slice-0-32/wave-1/testplan-controls.receipt reason=watchdog-deadline; child_seconds=23; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| tlib | UNKNOWN | 时间窗 | slice-0-21.log/wave.log: operation-deadline；slice-0-21.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| tlib-assert-bag | UNKNOWN | 时间窗 | slice-0-33/wave-1/tlib-assert-bag.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| tlib-base | UNKNOWN | 时间窗 | slice-0-21.log/wave.log: operation-deadline；slice-0-21.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| tlib-batch | UNKNOWN | 时间窗 | slice-0-27/wave-1/tlib-batch.receipt reason=watchdog-deadline; child_seconds=26; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| tlib-ext-memx | UNKNOWN | 时间窗 | slice-0-21.log/wave.log: operation-deadline；slice-0-21.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| tlib-ext-sysx | UNKNOWN | 时间窗 | slice-0-21.log/wave.log: operation-deadline；slice-0-21.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| tlib-ext-textx | UNKNOWN | 时间窗 | slice-0-21.log/wave.log: operation-deadline；slice-0-21.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| tlib-frozen | UNKNOWN | 时间窗 | slice-0-29/wave-1/tlib-frozen.receipt reason=watchdog-deadline; child_seconds=26; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| tlib-kit | UNKNOWN | 时间窗 | slice-0-25.log/wave.log: operation-deadline；slice-0-25.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| tlib-prep | UNKNOWN | 时间窗 | slice-0-21.log/wave.log: operation-deadline；slice-0-21.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| tlibpkg | UNKNOWN | 时间窗 | slice-0-20/wave-1/tlibpkg.receipt reason=watchdog-deadline; child_seconds=14; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| tool-index | UNKNOWN | 时间窗 | slice-0-42/wave-1/tool-index.receipt reason=watchdog-deadline; child_seconds=28; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| tool-inventory | UNKNOWN | 时间窗 | slice-1-108/wave-1/tool-inventory.receipt reason=watchdog-deadline; child_seconds=5; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| tools-compile-000 | UNKNOWN | 已修待复核 | slice-1-93/wave-1/tools-compile-000.receipt reason=watchdog-deadline; child_seconds=12; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-001 | UNKNOWN | 已修待复核 | slice-1-93/wave-1/tools-compile-001.receipt reason=watchdog-deadline; child_seconds=12; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-002 | UNKNOWN | 已修待复核 | slice-1-93/wave-1/tools-compile-002.receipt reason=watchdog-deadline; child_seconds=12; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-003 | UNKNOWN | 已修待复核 | slice-1-93/wave-1/tools-compile-003.receipt reason=watchdog-deadline; child_seconds=12; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-004 | UNKNOWN | 已修待复核 | slice-1-93/wave-1/tools-compile-004.receipt reason=watchdog-deadline; child_seconds=12; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-005 | UNKNOWN | 已修待复核 | slice-1-93/wave-1/tools-compile-005.receipt reason=watchdog-deadline; child_seconds=12; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-006 | UNKNOWN | 已修待复核 | slice-1-93/wave-1/tools-compile-006.receipt reason=watchdog-deadline; child_seconds=12; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-007 | FAIL | 已修待复核 | slice-1-94/wave-1/tools-compile-007.receipt reason=batch-fail; child_seconds=25; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-008 | FAIL | 已修待复核 | slice-1-94/wave-1/tools-compile-008.receipt reason=batch-fail; child_seconds=25; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-009 | FAIL | 已修待复核 | slice-1-94/wave-1/tools-compile-009.receipt reason=batch-fail; child_seconds=25; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-010 | FAIL | 已修待复核 | slice-1-94/wave-1/tools-compile-010.receipt reason=batch-fail; child_seconds=25; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-011 | FAIL | 已修待复核 | slice-1-94/wave-1/tools-compile-011.receipt reason=batch-fail; child_seconds=25; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-012 | FAIL | 已修待复核 | slice-1-94/wave-1/tools-compile-012.receipt reason=batch-fail; child_seconds=25; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-013 | FAIL | 已修待复核 | slice-1-94/wave-1/tools-compile-013.receipt reason=batch-fail; child_seconds=25; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-014 | FAIL | 已修待复核 | slice-1-94/wave-1/tools-compile-014.receipt reason=batch-fail; child_seconds=25; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-015 | FAIL | 已修待复核 | slice-1-94/wave-1/tools-compile-015.receipt reason=batch-fail; child_seconds=25; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-016 | FAIL | 已修待复核 | slice-1-94/wave-1/tools-compile-016.receipt reason=batch-fail; child_seconds=25; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-017 | FAIL | 已修待复核 | slice-1-95/wave-1/tools-compile-017.receipt reason=batch-fail; child_seconds=22; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-018 | FAIL | 已修待复核 | slice-1-95/wave-1/tools-compile-018.receipt reason=batch-fail; child_seconds=22; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-019 | FAIL | 已修待复核 | slice-1-95/wave-1/tools-compile-019.receipt reason=batch-fail; child_seconds=22; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-020 | FAIL | 已修待复核 | slice-1-95/wave-1/tools-compile-020.receipt reason=batch-fail; child_seconds=22; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-021 | FAIL | 已修待复核 | slice-1-95/wave-1/tools-compile-021.receipt reason=batch-fail; child_seconds=22; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-022 | FAIL | 已修待复核 | slice-1-95/wave-1/tools-compile-022.receipt reason=batch-fail; child_seconds=22; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-023 | FAIL | 已修待复核 | slice-1-95/wave-1/tools-compile-023.receipt reason=batch-fail; child_seconds=22; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-024 | FAIL | 已修待复核 | slice-1-95/wave-1/tools-compile-024.receipt reason=batch-fail; child_seconds=22; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-025 | FAIL | 已修待复核 | slice-1-95/wave-1/tools-compile-025.receipt reason=batch-fail; child_seconds=22; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-026 | FAIL | 已修待复核 | slice-1-95/wave-1/tools-compile-026.receipt reason=batch-fail; child_seconds=22; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-027 | FAIL | 已修待复核 | slice-1-96/wave-1/tools-compile-027.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-028 | FAIL | 已修待复核 | slice-1-96/wave-1/tools-compile-028.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-029 | FAIL | 已修待复核 | slice-1-96/wave-1/tools-compile-029.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-030 | FAIL | 已修待复核 | slice-1-96/wave-1/tools-compile-030.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-031 | FAIL | 已修待复核 | slice-1-96/wave-1/tools-compile-031.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-032 | FAIL | 已修待复核 | slice-1-96/wave-1/tools-compile-032.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-033 | FAIL | 已修待复核 | slice-1-96/wave-1/tools-compile-033.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-034 | FAIL | 已修待复核 | slice-1-96/wave-1/tools-compile-034.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-035 | FAIL | 已修待复核 | slice-1-96/wave-1/tools-compile-035.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-036 | FAIL | 已修待复核 | slice-1-96/wave-1/tools-compile-036.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-037 | FAIL | 已修待复核 | slice-1-97/wave-1/tools-compile-037.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-038 | FAIL | 已修待复核 | slice-1-97/wave-1/tools-compile-038.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-039 | FAIL | 已修待复核 | slice-1-97/wave-1/tools-compile-039.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-040 | FAIL | 已修待复核 | slice-1-97/wave-1/tools-compile-040.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-041 | FAIL | 已修待复核 | slice-1-97/wave-1/tools-compile-041.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-042 | FAIL | 已修待复核 | slice-1-97/wave-1/tools-compile-042.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-043 | FAIL | 已修待复核 | slice-1-97/wave-1/tools-compile-043.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-044 | FAIL | 已修待复核 | slice-1-97/wave-1/tools-compile-044.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-045 | FAIL | 已修待复核 | slice-1-97/wave-1/tools-compile-045.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-046 | FAIL | 已修待复核 | slice-1-97/wave-1/tools-compile-046.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-047 | FAIL | 已修待复核 | slice-1-98/wave-1/tools-compile-047.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-048 | FAIL | 已修待复核 | slice-1-98/wave-1/tools-compile-048.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-049 | FAIL | 已修待复核 | slice-1-98/wave-1/tools-compile-049.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-050 | FAIL | 已修待复核 | slice-1-98/wave-1/tools-compile-050.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-051 | FAIL | 已修待复核 | slice-1-98/wave-1/tools-compile-051.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-052 | FAIL | 已修待复核 | slice-1-98/wave-1/tools-compile-052.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-053 | FAIL | 已修待复核 | slice-1-98/wave-1/tools-compile-053.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-054 | FAIL | 已修待复核 | slice-1-98/wave-1/tools-compile-054.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-055 | FAIL | 已修待复核 | slice-1-98/wave-1/tools-compile-055.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-056 | FAIL | 已修待复核 | slice-1-98/wave-1/tools-compile-056.receipt reason=batch-fail; child_seconds=23; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-057 | FAIL | 已修待复核 | slice-1-99/wave-1/tools-compile-057.receipt reason=batch-fail; child_seconds=24; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-058 | FAIL | 已修待复核 | slice-1-99/wave-1/tools-compile-058.receipt reason=batch-fail; child_seconds=24; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-059 | FAIL | 已修待复核 | slice-1-99/wave-1/tools-compile-059.receipt reason=batch-fail; child_seconds=24; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-060 | FAIL | 已修待复核 | slice-1-99/wave-1/tools-compile-060.receipt reason=batch-fail; child_seconds=24; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-061 | FAIL | 已修待复核 | slice-1-99/wave-1/tools-compile-061.receipt reason=batch-fail; child_seconds=24; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-062 | FAIL | 已修待复核 | slice-1-99/wave-1/tools-compile-062.receipt reason=batch-fail; child_seconds=24; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-063 | FAIL | 已修待复核 | slice-1-99/wave-1/tools-compile-063.receipt reason=batch-fail; child_seconds=24; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-064 | FAIL | 已修待复核 | slice-1-99/wave-1/tools-compile-064.receipt reason=batch-fail; child_seconds=24; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-065 | FAIL | 已修待复核 | slice-1-99/wave-1/tools-compile-065.receipt reason=batch-fail; child_seconds=24; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-066 | FAIL | 已修待复核 | slice-1-99/wave-1/tools-compile-066.receipt reason=batch-fail; child_seconds=24; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-067 | FAIL | 已修待复核 | slice-1-100/wave-1/tools-compile-067.receipt reason=batch-fail; child_seconds=21; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-068 | FAIL | 已修待复核 | slice-1-100/wave-1/tools-compile-068.receipt reason=batch-fail; child_seconds=21; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-069 | FAIL | 已修待复核 | slice-1-100/wave-1/tools-compile-069.receipt reason=batch-fail; child_seconds=21; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-070 | FAIL | 已修待复核 | slice-1-100/wave-1/tools-compile-070.receipt reason=batch-fail; child_seconds=21; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-071 | FAIL | 已修待复核 | slice-1-100/wave-1/tools-compile-071.receipt reason=batch-fail; child_seconds=21; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-072 | FAIL | 已修待复核 | slice-1-100/wave-1/tools-compile-072.receipt reason=batch-fail; child_seconds=21; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-073 | FAIL | 已修待复核 | slice-1-100/wave-1/tools-compile-073.receipt reason=batch-fail; child_seconds=21; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-074 | FAIL | 已修待复核 | slice-1-100/wave-1/tools-compile-074.receipt reason=batch-fail; child_seconds=21; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-075 | FAIL | 已修待复核 | slice-1-100/wave-1/tools-compile-075.receipt reason=batch-fail; child_seconds=21; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-076 | FAIL | 已修待复核 | slice-1-100/wave-1/tools-compile-076.receipt reason=batch-fail; child_seconds=21; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-077 | FAIL | 已修待复核 | slice-1-101/wave-1/tools-compile-077.receipt reason=batch-fail; child_seconds=22; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-078 | FAIL | 已修待复核 | slice-1-101/wave-1/tools-compile-078.receipt reason=batch-fail; child_seconds=22; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-079 | FAIL | 已修待复核 | slice-1-101/wave-1/tools-compile-079.receipt reason=batch-fail; child_seconds=22; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-080 | FAIL | 已修待复核 | slice-1-101/wave-1/tools-compile-080.receipt reason=batch-fail; child_seconds=22; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-081 | FAIL | 已修待复核 | slice-1-101/wave-1/tools-compile-081.receipt reason=batch-fail; child_seconds=22; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-082 | FAIL | 已修待复核 | slice-1-101/wave-1/tools-compile-082.receipt reason=batch-fail; child_seconds=22; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-083 | FAIL | 已修待复核 | slice-1-117/wave-1/tools-compile-083.receipt reason=batch-fail; child_seconds=25; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-084 | FAIL | 已修待复核 | slice-1-117/wave-1/tools-compile-084.receipt reason=batch-fail; child_seconds=25; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-085 | FAIL | 已修待复核 | slice-1-117/wave-1/tools-compile-085.receipt reason=batch-fail; child_seconds=25; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-086 | FAIL | 已修待复核 | slice-1-117/wave-1/tools-compile-086.receipt reason=batch-fail; child_seconds=25; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-087 | FAIL | 已修待复核 | slice-1-117/wave-1/tools-compile-087.receipt reason=batch-fail; child_seconds=25; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-088 | REFUSED | 已修待复核 | slice-1-118/wave-1/tools-compile-088.receipt reason=identity-changed-during-wave; child_seconds=10; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-089 | REFUSED | 已修待复核 | slice-1-118/wave-1/tools-compile-089.receipt reason=identity-changed-during-wave; child_seconds=10; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-090 | REFUSED | 已修待复核 | slice-1-118/wave-1/tools-compile-090.receipt reason=identity-changed-during-wave; child_seconds=10; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-091 | FAIL | 已修待复核 | slice-1-119/wave-1/tools-compile-091.receipt reason=batch-fail; child_seconds=18; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-092 | FAIL | 已修待复核 | slice-1-120/wave-1/tools-compile-092.receipt reason=batch-fail; child_seconds=9; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-093 | FAIL | 已修待复核 | slice-1-121/wave-2/tools-compile-093.receipt reason=batch-fail; child_seconds=5; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-094 | UNKNOWN | 已修待复核 | slice-1-122.log/wave.log: bound timeout；slice-1-122.done rc=2 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-095 | UNKNOWN | 已修待复核 | slice-1-122.log/wave.log: bound timeout；slice-1-122.done rc=2 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-096 | UNKNOWN | 已修待复核 | slice-1-122.log/wave.log: bound timeout；slice-1-122.done rc=2 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-097 | UNKNOWN | 已修待复核 | slice-1-122.log/wave.log: bound timeout；slice-1-122.done rc=2 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-098 | UNKNOWN | 已修待复核 | slice-1-122.log/wave.log: bound timeout；slice-1-122.done rc=2 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-099 | FAIL | 已修待复核 | slice-1-123/wave-1/tools-compile-099.receipt reason=batch-fail; child_seconds=25; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-100 | FAIL | 已修待复核 | slice-1-123/wave-1/tools-compile-100.receipt reason=batch-fail; child_seconds=25; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-101 | FAIL | 已修待复核 | slice-1-123/wave-1/tools-compile-101.receipt reason=batch-fail; child_seconds=25; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-102 | FAIL | 已修待复核 | slice-1-123/wave-1/tools-compile-102.receipt reason=batch-fail; child_seconds=25; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-103 | FAIL | 已修待复核 | slice-1-123/wave-1/tools-compile-103.receipt reason=batch-fail; child_seconds=25; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-104 | FAIL | 已修待复核 | slice-1-123/wave-1/tools-compile-104.receipt reason=batch-fail; child_seconds=25; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-105 | FAIL | 已修待复核 | slice-1-123/wave-1/tools-compile-105.receipt reason=batch-fail; child_seconds=25; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-106 | FAIL | 已修待复核 | slice-1-123/wave-1/tools-compile-106.receipt reason=batch-fail; child_seconds=25; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-107 | FAIL | 已修待复核 | slice-1-123/wave-1/tools-compile-107.receipt reason=batch-fail; child_seconds=25; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-108 | FAIL | 已修待复核 | slice-1-123/wave-1/tools-compile-108.receipt reason=batch-fail; child_seconds=25; 收据SHA=匹配 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-109 | UNKNOWN | 已修待复核 | slice-1-124.log/wave.log: operation-deadline；slice-1-124.done rc=3 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-110 | UNKNOWN | 已修待复核 | slice-1-124.log/wave.log: operation-deadline；slice-1-124.done rc=3 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-111 | UNKNOWN | 已修待复核 | slice-1-124.log/wave.log: operation-deadline；slice-1-124.done rc=3 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-112 | UNKNOWN | 已修待复核 | slice-1-124.log/wave.log: operation-deadline；slice-1-124.done rc=3 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-compile-113 | UNKNOWN | 已修待复核 | slice-1-124.log/wave.log: operation-deadline；slice-1-124.done rc=3 | e43b97ecbf + 4d7c7a48a9 + 54a62b656a + 426171e5c2 | 清单补齐至1038源、130片边界对齐；须逐片重跑，不能把三片样本外推全绿 |
| tools-default-regression | UNKNOWN | 材料 F4b | slice-1-115.log/wave.log:缺映射 old-checker(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| tools-inventory-boundaries | UNKNOWN | 时间窗 | slice-1-101/wave-1/tools-inventory-boundaries.receipt reason=watchdog-deadline; child_seconds=22; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| tsv-lint | UNKNOWN | 时间窗 | slice-1-91/wave-1/tsv-lint.receipt reason=watchdog-deadline; child_seconds=6; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| tsvx-merge3 | UNKNOWN | 已修待复核 | slice-0-12/wave.log:异平台批拒绝连带本批 | 1c52ddf1aa | 本机批不再受该异平台批拖累(推测)，仍须实跑；异平台独立NOT_APPLICABLE |
| ucn-identities | UNKNOWN | 时间窗 | slice-0-0/wave-1/ucn-identities.receipt reason=watchdog-deadline; child_seconds=16; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| uninit-scan | UNKNOWN | 时间窗 | slice-1-91/wave-1/uninit-scan.receipt reason=watchdog-deadline; child_seconds=6; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| unipp2-reference | UNKNOWN | 材料待归类 | slice-1-114.log/wave.log:缺映射 newpp-run(具名NOT_AVAIL) | 未核到对应修复 | 若新映射仍缺此材料则UNKNOWN；生产者和实际准备须逐族确认，不能记PASS |
| unit-boundary | UNKNOWN | 时间窗 | slice-1-85/wave-1/unit-boundary.receipt reason=watchdog-deadline; child_seconds=14; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| unit-frontcache-budget | UNKNOWN | 时间窗 | slice-1-122.log/wave.log: bound timeout；slice-1-122.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| unit-frontcache-concurrent | UNKNOWN | 时间窗 | slice-1-122.log/wave.log: bound timeout；slice-1-122.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| unit-frontcache-equiv0 | UNKNOWN | 时间窗 | slice-1-121/wave-2/unit-frontcache-equiv0.receipt reason=watchdog-deadline; child_seconds=5; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| unit-frontcache-equiv1 | UNKNOWN | 时间窗 | slice-1-121/wave-2/unit-frontcache-equiv1.receipt reason=watchdog-deadline; child_seconds=5; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| unit-frontcache-equiv2 | UNKNOWN | 时间窗 | slice-1-121/wave-2/unit-frontcache-equiv2.receipt reason=watchdog-deadline; child_seconds=5; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| unit-frontcache-equiv3 | UNKNOWN | 时间窗 | slice-1-121/wave-2/unit-frontcache-equiv3.receipt reason=watchdog-deadline; child_seconds=5; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| unit-frontcache-equiv4 | UNKNOWN | 时间窗 | slice-1-121/wave-2/unit-frontcache-equiv4.receipt reason=watchdog-deadline; child_seconds=5; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| unit-frontcache-neg | UNKNOWN | 时间窗 | slice-1-121/wave-2/unit-frontcache-neg.receipt reason=watchdog-deadline; child_seconds=5; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| unit-frontcache-race | UNKNOWN | 时间窗 | slice-1-121/wave-2/unit-frontcache-race.receipt reason=watchdog-deadline; child_seconds=5; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| unit-frontcache-stderr | UNKNOWN | 时间窗 | slice-1-121/wave-2/unit-frontcache-stderr.receipt reason=watchdog-deadline; child_seconds=5; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| unit-frontcache-switch | UNKNOWN | 时间窗 | slice-1-121/wave-2/unit-frontcache-switch.receipt reason=watchdog-deadline; child_seconds=5; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| use-path | UNKNOWN | 时间窗 | slice-1-74/wave-1/use-path.receipt reason=watchdog-deadline; child_seconds=8; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| user-malloc-null | UNKNOWN | 时间窗 | slice-0-33/wave-1/user-malloc-null.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| utf8-net | UNKNOWN | 已修待复核 | slice-1-84/wave.log:cx-plan拒check-utf8.cx，连带本批 | 6929ca4d91 | 旧utf8模块入口阻塞已修，仍须新根实跑，非现成PASS |
| version-inject | UNKNOWN | 时间窗 | slice-0-14.log/wave.log: sid-measurement；slice-0-14.done rc=2 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| version-metrics | UNKNOWN | 时间窗 | slice-0-19/wave-1/version-metrics.receipt reason=watchdog-deadline; child_seconds=24; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| watch-bundle | FAIL | 已修待复核 | slice-1-106/wave-1/watch-bundle.receipt reason=batch-fail; child_seconds=15; 收据SHA=匹配 | 66505c70d7 | 已修自重入/归档路径/tape同路径对比；新根须实跑 |
| whole-front | UNKNOWN | 时间窗 | slice-0-13.log/wave.log: operation-deadline；slice-0-13.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| whole-front-wiring | UNKNOWN | 时间窗 | slice-0-20/wave-1/whole-front-wiring.receipt reason=watchdog-deadline; child_seconds=14; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| whole-o-deadline | UNKNOWN | 已修待复核 | slice-1-108/wave-1/whole-o-deadline.receipt reason=watchdog-deadline; child_seconds=5; 收据SHA=匹配 | 21f95007be | 新根须带真实产品build.json并核内容；旧缺伴随文件断言预期恢复(推测)，须实跑 |
| wholefrontmerge | UNKNOWN | 时间窗 | slice-0-13.log/wave.log: operation-deadline；slice-0-13.done rc=3 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| win-argv | UNKNOWN | 时间窗 | slice-1-91/wave-1/win-argv.receipt reason=watchdog-deadline; child_seconds=6; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| win-arm64-fixedpoint-controls | UNKNOWN | 时间窗 | slice-2-130/wave-3/win-arm64-fixedpoint-controls.receipt reason=watchdog-deadline; child_seconds=3; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| workspace-plan | UNKNOWN | 时间窗 | slice-0-33/wave-1/workspace-plan.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| wt-gate-clean | UNKNOWN | 时间窗 | slice-1-89/wave-1/wt-gate-clean.receipt reason=watchdog-deadline; child_seconds=26; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |
| y-oracle | UNKNOWN | 时间窗 | slice-1-117/wave-1/y-oracle.receipt reason=watchdog-deadline; child_seconds=25; 收据SHA=匹配 | 21f95007be(仅缓解) | 降并发可能改善(推测)，没有整轮证据；仍可能UNKNOWN/REFUSED或暴露FAIL |

范围外41批另列，不计入916个未通过项；新根继续按平台判NOT_APPLICABLE，不冒充PASS。1c52ddf1aa补的是计划和波选择，避免再拖垮同片本机批。

| 批次 | 旧态 | 登记平台 | 新根预期 |
|---|---|---|---|
| ape-cache-arm64 | NOT_APPLICABLE | osx/arm64 | NOT_APPLICABLE，仍非PASS |
| ape-cache-x86_64 | NOT_APPLICABLE | osx/arm64 | NOT_APPLICABLE，仍非PASS |
| arm64-cx-sys-netmsg | NOT_APPLICABLE | lnx/arm64 | NOT_APPLICABLE，仍非PASS |
| arm64-cx-sys-netnb | NOT_APPLICABLE | lnx/arm64 | NOT_APPLICABLE，仍非PASS |
| arm64-cx-sys-sigwatch | NOT_APPLICABLE | lnx/arm64 | NOT_APPLICABLE，仍非PASS |
| arm64-ext-net-unix | NOT_APPLICABLE | lnx/arm64 | NOT_APPLICABLE，仍非PASS |
| driver-default-tape-osx | NOT_APPLICABLE | osx/arm64 | NOT_APPLICABLE，仍非PASS |
| driver-default-tape-osx-multi | NOT_APPLICABLE | osx/arm64 | NOT_APPLICABLE，仍非PASS |
| macos-signing-interface | NOT_APPLICABLE | osx/arm64 | NOT_APPLICABLE，仍非PASS |
| objects-osx-arm64-mixed-target | NOT_APPLICABLE | osx/arm64 | NOT_APPLICABLE，仍非PASS |
| objects-osx-arm64-old-unit-version | NOT_APPLICABLE | osx/arm64 | NOT_APPLICABLE，仍非PASS |
| objects-osx-x86_64-mixed-target | NOT_APPLICABLE | osx/arm64 | NOT_APPLICABLE，仍非PASS |
| objects-osx-x86_64-old-unit-version | NOT_APPLICABLE | osx/arm64 | NOT_APPLICABLE，仍非PASS |
| objects-unsupported-osx-arm64 | NOT_APPLICABLE | osx/arm64 | NOT_APPLICABLE，仍非PASS |
| objects-unsupported-osx-x86_64 | NOT_APPLICABLE | osx/arm64 | NOT_APPLICABLE，仍非PASS |
| osx-chain-interface | NOT_APPLICABLE | osx/arm64 | NOT_APPLICABLE，仍非PASS |
| osx-lane-interface | NOT_APPLICABLE | osx/arm64 | NOT_APPLICABLE，仍非PASS |
| osx-lane-queue | NOT_APPLICABLE | osx/arm64 | NOT_APPLICABLE，仍非PASS |
| osx-lane-scheduler | NOT_APPLICABLE | osx/arm64 | NOT_APPLICABLE，仍非PASS |
| osx-launch-arm64 | NOT_APPLICABLE | osx/arm64 | NOT_APPLICABLE，仍非PASS |
| osx-launch-profile-interface | NOT_APPLICABLE | osx/arm64 | NOT_APPLICABLE，仍非PASS |
| osx-launch-x86_64 | NOT_APPLICABLE | osx/arm64 | NOT_APPLICABLE，仍非PASS |
| osx-sentinel-baseline-red | NOT_APPLICABLE | osx/arm64 | NOT_APPLICABLE，仍非PASS |
| osx-sentinel-bisect | NOT_APPLICABLE | osx/arm64 | NOT_APPLICABLE，仍非PASS |
| osx-sentinel-execution | NOT_APPLICABLE | osx/arm64 | NOT_APPLICABLE，仍非PASS |
| osx-sentinel-forged-wait | NOT_APPLICABLE | osx/arm64 | NOT_APPLICABLE，仍非PASS |
| osx-sentinel-prepare | NOT_APPLICABLE | osx/arm64 | NOT_APPLICABLE，仍非PASS |
| osx-sentinel-unknown | NOT_APPLICABLE | osx/arm64 | NOT_APPLICABLE，仍非PASS |
| prune-object-external-bare-osx | NOT_APPLICABLE | osx/arm64 | NOT_APPLICABLE，仍非PASS |
| prune-object-external-lnx-arm64 | NOT_APPLICABLE | osx/arm64 | NOT_APPLICABLE，仍非PASS |
| prune-object-external-lnx-x86_64 | NOT_APPLICABLE | osx/arm64 | NOT_APPLICABLE，仍非PASS |
| prune-object-external-osx-arm64 | NOT_APPLICABLE | osx/arm64 | NOT_APPLICABLE，仍非PASS |
| prune-object-external-osx-x86_64 | NOT_APPLICABLE | osx/arm64 | NOT_APPLICABLE，仍非PASS |
| prune-object-external-win-arm64 | NOT_APPLICABLE | osx/arm64 | NOT_APPLICABLE，仍非PASS |
| prune-object-external-win-x86_64 | NOT_APPLICABLE | osx/arm64 | NOT_APPLICABLE，仍非PASS |
| prune-route-placement-oracle | NOT_APPLICABLE | osx/arm64 | NOT_APPLICABLE，仍非PASS |
| release-ape-replacements | NOT_APPLICABLE | osx/arm64 | NOT_APPLICABLE，仍非PASS |
| release-members-interface | NOT_APPLICABLE | osx/arm64 | NOT_APPLICABLE，仍非PASS |
| runner-platform | NOT_APPLICABLE | osx/arm64 | NOT_APPLICABLE，仍非PASS |
| tool-self-path | NOT_APPLICABLE | osx/arm64 | NOT_APPLICABLE，仍非PASS |
| wt-gate-clean-win | NOT_APPLICABLE | win/x86_64 | NOT_APPLICABLE，仍非PASS |








