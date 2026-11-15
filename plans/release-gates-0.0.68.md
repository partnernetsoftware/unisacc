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






