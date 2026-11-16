# devsvc 协同提速方案(2026-11-16)
作者:fable-review(主调度 cc 派工),2026-11-16 08:05 至 09:00,只读分析。本文件不含任何新的通过结论;带"实测"的数字来自本轮命令或所引表行,带"估计"或"推算"的没有跑过。与 plans/release-speedup-plan.md(方案 A 至 G 与裁决 D1 至 D7)和 plans/devsvc.md 第 6 节(P1 至 P9、R1 至 R7、S1 至 S3、索引两阶段)不重复:那两处已排或已做的只引用并标状态。按规则不该再写带日期的 plans 文档,本文件是派工明确指定的落点,主调度可在吸收进 plans/devsvc.md 后归档。

## 1 结论

(1) 本轮最大的可回收时间不在某个慢命令,而在三类"税":派生表补提交(v0.0.68 近 9 小时 75 个提交里 18 个是重算,占 24%)、登记与清单漂移(23 个提交提到登记,12 个提到 tools-compile)、批次贴 55 秒后的拆组(7 个提交)。三类都能改成提交流内自动完成或由生成器维护。
(2) 后期才暴露的确定性缺陷有共同结构:HEAD 不保证派生物干净、tools-compile 用"每片八源"定片使加一个工具就可能改批次表、建冻结根之前没有静态就绪门。对应三项改动 P1、P3、P4。
(3) devsvc 现在有两处协同底座缺口比功能缺口更要紧:实例空闲 300 秒自退后 flow、f、x 静默转 via=local 且 locks=0(本轮 08:07 实测 svc=down,同时有 12 个在途进程);全仓没有任何按负载或可用内存的准入(loadavg、MemAvailable 在 tools 下检索 0 处),dcheck 缺省并发可到 32 而机器 16 核 31 GB。
(4) 按铁律判"设计需要改"的有五项:整链 62.5 分且四个对象相各 696 至 734 秒、daily 一轮 60.9 分且 86% 不在测、冻结到公开 0.0.57 实测 190.6 分、register 单次贴 55 秒、批次预算列 96.6% 填 55 不携带信息。
(5) 写域租约、验收隔离、合规核对都可以在现有工单文件与 f 事件(path 加写后 sha)上实现,不需要私有工作区;只有"验收在私有求值根里跑"一项触到项目所有者 2026-11-15 的约束,需要裁决。
(6) daily 与发布序列改为 devsvc flow DAG 是既有方案 D 的细化;各相已是幂等可续,引擎不用加恢复语义,只缺生成节点表的工具与一个有界续推作业。
(7) 建议落地顺序:P7(锁不失效,半天内)-> P2(准入)-> P1(提交即派生)-> P3、P4(与 0.0.69 慢车道前的门改造同窗)-> P5、P6 -> 其余。
(8) 全部方案都不缓存测试结论、不改任何时限、不删断言;凡是"少跑"的地方都记 NOT_AFFECTED、NOT_RUN 或 DEFERRED,不记 PASS。

## 2 第一手事实的核实

| 号 | 简报所述 | 核实结果 | 证据入口 |
|---|---|---|---|
| F1 | tools-compile 登记表两次被新增工具打破,整族必败 | 已核实。c7b09090fe(04:19)新增 check-devtask-help 未登记,9ace1a8cca(05:55)补,间隔 96 分;7b42d15453(06:14)新增 libneed-native 两工具,193ab4112d(06:28)补,间隔 14 分。另有 e43b97ecbf 一次补 133 个源、54a62b656a 片数 131 对 130 漂移、426171e5c2 补 3 个 | git log v0.0.68 |
| F2 | 登记门缺 toolsreg 触发,新门又在夹具仓硬拒 | 已核实。3721a123cc(06:43)加 toolsreg 门,5f1ee866f3(07:33)修夹具仓判定,90c8808d2b 拆三批。main 的 tests/precommit-gates.tsv 仍无 toolsreg 行(v0.0.68 尚未合入 main,git merge-base 实测) | 两处 precommit-gates.tsv |
| F3 | 版本工作树里因 .git 是文件而失败 | 已核实修复提交 a36d8bbab8、6652cf045e、5f1ee866f3。存量:tools 与 tests 下仍有 14 处 "/.git/" 字面量,分布在 check-precommit-flow.cx:80、check-derive-flow.cx:57 至 58、check-integrate-patch.cx:169 与 260、status.cx:73(回退分支)、check-status.cx 等 | devsvc flow search 节点 |
| F4 | check 预期过期 | 已核实 62be1c9736、8babd6695f、12b236e1d2(precommit-preview 把新信息行当门行) | git log |
| F5 | 派生表反复漂移 | 已核实。近 9 小时 main 6 个、v0.0.68 18 个重算提交;近 24 小时全仓 328 个提交里 31 个;tools.cxlib 一夜从 0.3.13 升到 0.3.17 | git log --grep |
| F6 | s9 片窗口公式、注册表扫描、材料生产者、record、wrapper | 部分核实:1667117aa0(need 公式)、87e8040cc1(单遍扫描)、6958aa2df5(assembler)、3e10187265(kernels)、7b42d15453(libneed-native)、d71e9aa5fd(RI)、e8741a7b77(读最后一条终态)有提交。source-origin 与 P53 原始 record 未找到对应提交或文件,未复核 | git log v0.0.68 |
| F7 | 子代理互相干扰、affected 压到负载 48 引发 OOM | 机制已核实,事件未复现:dcheck 头注释写明缺省并发为批数与 32 的较小值;affected、dcheck、flowexec、devsvc-core、register 中 loadavg 与 MemAvailable 检索均为 0 处;裁决 D6 的 heavy 锁检索 0 处(未实现)。负载 48 与 OOM 本身没有留存证据,记未复核 | search 节点 |
| F8 | 子代理绕开 devsvc | 已核实并量化:hook-warn.tsv 近 9 小时 2300 条,pipe-trim 1706、direct-cx 259、git-read 228、sleep-poll 73、git-write 34。告警行只有会话号,子代理与父会话同号,无法归到具体代理 | 一次性脚本 /tmp/rv/hw.cx |
| F9 | devsvc 自身缺口 | task 子命令(c7b09090fe)、x 与 flow 非零 rc 误报(16045f8009、c00bff644a)已修;--keyed 同文件合并、git merge、git add、提交回执跟随(55 秒外层超时)在主检出有未提交改动,属在途;guard-hook 的 /tmp 误拦同属在途。x --bg 已有(c2b183cf14)。devsvc ps 未实现 | git diff main 工作区 |
| F10 | 批次预算 55 而 affected 54 秒杀 | 已核实且是结构问题:main 的 1195 批里 1154 批预算填 55(96.6%),30 至 49 的 27 批,小于 30 的 13 批,另有 chain-batchkeys 填 90(commit 档,超过 55 秒节点上限)。x.affected 近 9 小时 73 次里 12 次不低于 50 秒 | /tmp/rv/bt.cx、ev.cx |
| F11 | 整链约 62 分 | 已核实:build/phase-times.tsv 的 real6 为 3752 秒,real5 为 3538 秒,g3fix8 为 4625 秒 | phase-times 209、211、228 行 |

本轮新增发现(简报未列):

(1) 实例不在时写锁静默失效。08:07 devsvc status 末行 svc=down locks=0,同时 PROC 12 个(含 s9-nightly map 相、check-devsvc-git、derive --check)。本轮我发的每条 flow 汇总行都是 via=local locks=0。头注释写明 flow、f、x 不拉起实例,无实例时照跑不取锁。实例 idle 缺省 300 秒自退,所以两次写之间只要空 5 分钟,后面的写就没有锁。
(2) 实例口径不一致:devsvc-core.cx:172 的注释是"一个工作树一个实例"(0ef0c026f8),同文件头注与 plans/devsvc.md 第 3 节写"所有版本工作目录共用一个实例"。实例目录下有事件的是三个(045c5a38 有 21140 行、9837e2ae 有 993 行、968568b4 有工单 319 个文件)。按工作树分实例后,锁只在本工作树内互斥,机器级资源(核、内存、tlib 套件缓存)没有任何跨实例的协调点。
(3) devsvc status 一次 24.5 秒(本轮单次实测,负载 5.6),不适合当开工前置检查;分布未测。
(4) 工单收工率已从 3/9 升到近 9 小时 43/45,S3 的验收目标已达到,不再需要追加措施。
(5) git.commit 经 devsvc 的失败率近 9 小时两实例合计 46 次里 4 次(8.7%),对比前一日 36%;但 --bg 提交的事件 ms 是起作业耗时,不是提交耗时,devsvc-budget 的 git.commit 分位因此偏小,需要在 job.done 上按命令分类才准。

## 3 A:按铁律属于"设计需要改"的慢项

| 项 | 铁律 | 量化 | 状态 |
|---|---|---|---|
| A1 整链 candidate-chain | 单操作 300 秒 | 整链 real6 3752 秒(实测);g2、g3 四个 linux 对象相 696 至 734 秒,obj-N 360 秒,closure 348 秒(实测);豁免 G2-OBJ-SPLIT 到期 2026-11-30 | 超限,有豁免无实现 |
| A2 daily 一轮 | 单操作 300 秒,整版 2 小时 | 5a071a43df 根 60.9 分(实测):准备 32 分、137 片 24.8 分、聚合 2.7 分;真正跑测试约 8.4 分(14%)。降并发后的全量 1.5 至 2 小时为推算,UNMEASURED | 方案 C 已裁决未实现;87e8040cc1 与 1667117aa0 做了其中两小块 |
| A3 冻结到公开 | 整版 7200 秒 | 0.0.57 为 11436 秒(实测,190.6 分);0.0.47 为 26400 秒;0.0.68 本轮 UNMEASURED,计划 2.2 节推算关键路径 3.5 至 4 小时 | 超限 |
| A4 register 单次 | 55 秒 | 事件 register.devsvc-run p50 50.0 秒、max 55.1 秒;近 9 小时 2 次合计 80.1 秒;phase-times 该行 UNMEASURED 且豁免 D1.register | 贴线 |
| A5 批次预算 | 55 秒 | 96.6% 的批填 55,无实测耗时依据;job.done 822 条里 160 条贴线、40 条超时(全历史);flow.node 近 9 小时 5993 个里失败 379、超时 56 | 预算列不携带信息 |
| A6 派生检查 | 55 秒 | derive 整体 --check 首次 55 秒超时(release-gates 文档);近 9 小时 20 次合计 364.5 秒,x.derive 历史 max 83.7 秒 | 已有并发 derive-flow,仍无增量 |
| A7 工具自检批 | 55 秒 | check-devtask p50 41.1 秒 max 60.0;check-devsvc max 58.0 且 3 次超时;check-precommit-flow p50 32.9 秒。冷编译 edit-span 2.6 秒、ratchets 5.0 至 5.1 秒(speed-baseline 实测),批入口链接 7 至 9 秒对运行 0.2 秒(release-speedup-plan 1.5) | 靠拆组维持 |
| A8 状态查询 | 无明文 | devsvc status 24.5 秒(单次实测) | 新发现 |

结构性方案(只列前两份文档没有的部分):

A1 方案:对象相按键分段。目标:四个对象相每次调用不超过 300 秒,整链墙钟不升。做法:44 键的对象批按 11 键一段拆成 4 个 lane 步,每段自带 intent、done 与收据,段间沿用现有 --resume 的重核;段内并发度不变。写域:tools/candidate-chain.cx 的对象相步骤表与其 check(v0.0.69 或 main,不进 0.0.68)。验收:`sh ./unisacc.com tools/phase-times-calc.cx -- CHAIN/chain-run.tsv` 的单次最大值不超过 300;compare 仍为 objects、kernels、slices、product 四类 same。收益:合规,墙钟持平(估计);中途失败的重做量从 12 分降到 3 分(估计)。风险:段间重核开销,0.0.57 曾因续跑复核每次 167 秒而并相(phase-times 183 行),须先确认 T11 备忘后的重入是 35 秒量级。

A3 方案:冻结门与整链并行,发布 12 步成 flow。目标:S0 到 S13 的关键路径去掉 S1 的 30 分与步间人工等待。做法:(a) 合入并固定 REV 后立即起整链(它的输入是 REV 快照,属独立状态域),冻结门十几项同时作为只读 flow 节点跑;任一门失败即按真实 PGID 停链并保留现场;(b) 步 6 至 13 写成一张节点表 tools/release-flow.cx emit,每步一个节点,前置是上一步的收据文件,公开一步之前插入一个必须人工或主调度显式放行的 gate 节点(读 release/rollback.txt 已按本版重写才放行)。写域:新工具 tools/release-flow.cx 与 check、release/ 下收据(仍由 cc 写)。验收:夹具里门失败时链进程组在 10 秒内消失且 checkpoint 保留;`devsvc flow run /tmp/rel.tsv --par 4 --summary` 每步一行 SUM;phase-times 新增"冻结到可发布(0.0.69)"行不超过 7200。收益:每次发布约 30 分(S1 并行,估计)加步间等待 10 至 20 分(估计)。风险:门失败时浪费最多 30 分整链 CPU;整链 --parallel 12 与门并跑会抢核,须先有 P2 的准入。铁律:门一项不少,只是并行;整链未过门前不产生任何资格。

A4、A5、A6、A7 的方案分别并入 P1(提交即派生)、P9(实测耗时表)与 P2(准入),见第 5 节。

## 4 B:确定性缺陷前移

### P1 提交即派生:HEAD 恒为派生干净

目标:消掉"重算"补提交与由此产生的漂移窗口;derive --check 在任何提交上恒为 drift=0。
做法:devsvc git commit(commit-own 私有根路径)在门之前加一步:用 rel impact 的 IMPACT table 行(G6 已落地)算出本提交路径命中的派生生成器,在私有根里对这些生成器跑 write_argv,把生成物的变化并入同一棵树,再跑各自 check_argv 与原有门。私有根等于 HEAD 加本请求路径,所以生成结果只反映本提交,不含他人半截改动。tools.cxlib 接口摘要变化时的升版(排期里的"tlib bump 经 devsvc git 内部暂存")作为其中一个生成器处理。未命中的生成器记 NOT_AFFECTED。write_argv 为 - 的(layer-api)只跑 check,失败时具名拒绝并提示手工步骤。
写域:tools/commit-own.cx、tools/devsvc-core.cx 的 cmd_git、tools/derive.cx(加 --only NAME 与 --root DIR)、exec/facts/derive.tsv(补 inputs_glob 精度)、check-commit-own 新组、check-derive。
验收:(1) 夹具:提交一个新增批次的登记集,不手工重算,`git show --stat HEAD` 含 build/links.tsv 与 build/tool-index.tsv 的对应行,`sh ./unisacc.com tools/derive.cx -- --check` 末行 drift=0;(2) 夹具:他人工作区有未提交的派生漂移,本提交的树不含其行;(3) 真实仓连续 20 个提交后 `git log --grep=重算 --since=...` 为 0;(4) 提交相位行 COMMIT_OWN_PHASE 新增 derive_ms,p90 不超过 20000。
收益:每 9 小时窗约 18 至 24 次补提交,每次含发现、重算、键选提交约 4 至 8 分代理时间,合计约 70 至 190 分每窗(估计);另消掉"漂移到 daily 才发现"的整轮(实测一轮 61 分)。
依赖与风险:rel impact 的 table 行要覆盖全部 10 个生成器(现状未逐个核);生成器最长 55 秒(layer-api、product-map),提交关键路径可能从 11 秒升到 30 秒以上,所以必须只跑命中的并与门并发;commit-own --registered 的"同键新旧成对"键选缺陷(plans/devsvc.md 第 6 节)须先修。回滚:环境变量关掉该步,行为回到现状。铁律:生成物仍由生成器产出并过 --check;不缓存结论。

### P3 tools-compile 定片与清单

目标:新增、删除、改名工具不再改动批次表,漏登只败一个批而不是整族。
做法:(a) 片归属由"清单行序除以 8"改为"路径哈希对固定片数 K 取模"(K 取现值 130 附近并写进表头,改 K 才需要动批次行);(b) 全表对拍(inventory differs、shard range)从每片都做改为只在单独的 tools-compile-inventory 批里做,各片只编自己的源;(c) tests/tools-compile-sources.tsv 进 exec/facts/derive.tsv 成第 11 项,check_argv 用现有 `tests/tools-compile.cx --register` 的对拍模式,随 P1 在提交流里自动补行。审核语义保留:自动补的行在提交说明里由 commit-own 打印 TOOLSREG added=路径,供集成者审。
写域:tests/tools-compile.cx、tests/tools-compile-sources.tsv 表头、tests/batches.tsv 的 tools-compile 行(一次性)、tests/manifests 对应清单、exec/facts/derive.tsv、check-precommit-gates 的 toolsreg 组。须在 v0.0.69 或合入后的 main 上做,不进 0.0.68 冻结窗。
验收:夹具里新增一个 tools/x.cx 不登记:`affected --ids tools-compile-inventory --run` 为 FAIL 且首行具名该路径,其余 tools-compile 片 PASS;登记后全过且 `git diff --stat tests/batches.tsv` 为空;删除 9 个工具后批次表仍不变。
收益:本轮两次整族必败各拖一轮(61 分每轮,实测)加修复间隔 14 与 96 分(实测);片数漂移类缺陷归零。
风险:哈希定片使各片大小不均,最大片可能超时,须在生成时打印每片源数并对最大片设上限检查;改判据属于改检查结构,断言总量要逐条对照不减。

### P4 daily-ready:建冻结根之前的静态就绪门

目标:把只能在一轮 daily 里发现的结构缺陷前移到建根前 5 分钟内。
做法:新工具 tools/daily-ready.cx(仿 chain-ready 的 READY=yes 或 no 末行),全部是只读静态核对,每项一个 flow 节点:(1) derive --check 十项;(2) tools-compile-inventory;(3) 材料生产者闭合:batches.tsv 的 materials 列每个材料名在 s9-nightly 的材料族映射里有生产者,缺的逐个具名;(4) 批次平台与计划:make_plan 干跑,不属本机的批全部落到 NOT_APPLICABLE 而不进片;(5) 入口计划:对每个批入口跑候选的 -cx-plan(现在在 all-waves.cx:160 片内才跑,一个被拒拖垮同片 9 批),建根前并发跑完;(6) 冻结根伴随文件清单(unisacc.com.build.json 等)表驱动核对;(7) 预算核对:预算超过 55 的批(现有 chain-batchkeys 为 90)与实测 p90 超过 28 秒的批具名;(8) 限时不变量:build/limits.tsv 注释里的不变量与片 need 公式写成可执行行,遍历全部读取点核对。
写域:新 tools/daily-ready.cx、tools/check-daily-ready.cx、批次登记;不改 s9 核验合同。
验收:在 42caaf6115 的归档树上跑应报 tools-compile 片数漂移一项;在 5a071a43df 上应报 utf8-net 的 cx-plan 拒绝与缺 build.json 两项(这两个根的缺陷已知,正好作回归样本);当前 v0.0.68 HEAD 上 READY=yes 或逐项具名。整条不超过 300 秒,单节点不超过 55 秒。
收益:每个被前移的结构缺陷省一轮,周期 70 至 120 分(release-speedup-plan 1.2 实测);本轮简报列的材料生产者缺失 5 类、cx-plan 1 类、伴随文件 1 类都在覆盖内。
风险:静态核对通过不等于门内通过,末行必须带 note=static-only-not-a-verdict;材料族映射若只在代码里而不在表里,第 (3) 项要先把映射抽成事实表。

### P8 版本工作树一等支持与 git 目录棘轮

目标:任何工具与检查在主检出与版本工作树里行为一致,不再逐个修 .git 是文件的假设。
做法:(a) cxlib 加一个帮手(工作树根、git-dir、git-common-dir 三个取值,内部用 git rev-parse),现有十余处各写各的 rev-parse(devsvc-core、devsvc、devtask、cxlib/devtask、layer-api、precommit)收敛到它;(b) 新棘轮 gitdir-literal:tools 与 tests 下 .cx 的字符串字面量出现 "/.git/" 或 "-d .git" 即拦,存量 14 处进基线并逐个清;(c) 启动器加全局选项 `devsvc --wt v0.0.NN 子命令...`:一次解析出工作树绝对路径,chdir 过去,设 flow 写根,缺 unisacc.com 符号链接时自动补,x、f、flow、doc、git、derive、dcheck、task 全部继承,不再各自实现 --cwd;(d) 夹具帮手提供"普通仓、无 tests/run.cx 的夹具仓、工作树"三种形态,P12 的门用例矩阵用它。
写域:tools/cxlib 新模块、tools/ratchets.cx 加一项与基线表、tools/devsvc.cx 启动器、相关 check。
验收:`sh ./unisacc.com tools/ratchets.cx` 新项存量等于基线 14 且新增为 0;`devsvc --wt v0.0.68 x --par 1 -- git rev-parse --show-toplevel` 打印工作树路径;在工作树里 `dcheck -- tools/precommit.cx` 选出的批与主检出逐行相同且全部可跑。
收益:本轮工作树类修复至少 3 个提交加 8 批硬拒,估计 60 至 90 分;以后每个新版本工作树首日不再重演(估计每版 30 分)。
风险:启动器按工作副本即编即跑,改坏会断全员入口,须先落排期里的"启动器写入保护"。

### P9 批次实测耗时表与自动拆组

目标:预算列有依据,贴线批在登记时而不是在 daily 里暴露。
做法:run-batch 已打印 BATCH_TIMING(prep、link、run、total 与 kit 状态)。devsvc 在实例目录外的共享位置维护一张只增的耗时表(批 id、时间、total_ms、kit、负载),只记耗时不记结论;tools/batch-cost.cx 已是读者。三个消费者:(1) register 与 reg-manifest 对实测 p90 超过 28 秒(s9 波窗上限,release-gates 文档时间窗机制一节)的批打 NEAR 行,超过 45 秒拒绝登记并提示拆组;(2) 新工具 tools/batch-split.cx 读检查的 --list-cases 与各用例耗时,按不超过 25 秒热态装箱,生成清单行草稿(本轮 7 次手工拆组的模板化);(3) dcheck 与 P2 按实测耗时排程。
写域:tests/run-batch.cx(追加一行写表)、tools/register.cx、新 tools/batch-split.cx 与 check。
验收:夹具批睡 30 秒,register 打 NEAR;睡 46 秒被拒;batch-split 对 check-commit-own 给出的分组每组实测不超过 25 秒且用例名并集与原全集逐名相同。
收益:拆组类每次 20 至 40 分,本轮 7 次,约 140 至 280 分(估计);消掉 F5 贴线批在 daily 里的假 UNKNOWN。
风险:耗时随负载波动,阈值判定取低负载样本(记录负载列并只用 load 低于核数一半的样本);表只是调度与告警依据,不得被任何门当作通过证据。

## 5 C:devsvc 协同的下一层

### P7 实例常在,写锁不静默失效

目标:任何写节点要么持锁,要么具名拒绝,不存在 via=local locks=0 的写。
做法:f write、f edit、f edits、doc 写、带写节点或 lock 键的 flow、git commit 在无实例时自动拉起(与 lock、run、events 同一条路径,等待上限 40 秒);拉不起时写节点拒绝并打 REFUSE no-instance,只读节点照旧本地跑。idle 自退前检查实例目录下是否有未完成工单或在途作业,有则续到 ttl。
写域:tools/devsvc-core.cx(flow 与 f 入口)、check-devsvc-flow 加两例。
验收:`devsvc stop` 后立刻 `devsvc f write /tmp 外的夹具路径 ...`,结果行 via=svc 且 locks=1;把实例目录设成不可写再试,rc 1 且首行 REFUSE no-instance,文件字节不变。
收益:正确性项,不直接省时;它是 P6、P2 的前提。估计避免每窗 1 至 2 次并发写覆盖的排查(每次 15 至 30 分)。
风险:自动拉起的冷构造最长 22 秒;服务单线程的排队问题(第 6 节限制 1)会更显眼。

### P2 机器级准入:全局并发与内存预算

目标:任何组合的 flow、dcheck、affected、整链、daily 同时跑,负载不超过核数、不触发 OOM;排不上的如实 DEFERRED。
做法:令牌放在机器级目录(HOME 下 devsvc/machine/slots,文件锁实现,跨实例跨工作树),总数取配置(缺省等于 nproc 16)。flow 引擎起每个 exec 节点前取 weight 个令牌(缺省 1,节点键 weight=N;affected -j N 记 N;整链按其 --parallel 记;裁决 D6 的 heavy 名额用同一机制表达为大权重)。起节点前读 /proc/meminfo 的 MemAvailable,低于配置阈值(缺省 4 GB)不起新节点。等待计入 flow 总截止,不计入节点自己的 55 秒;到截止仍没起的节点记 NOT_RUN reason=admission。dcheck 缺省并发改为批数与当前空闲令牌的较小值。DEVSVC_FLOW 汇总行加 admit_wait_ms 与 peak_tokens。
写域:tools/cxlib/flowexec.cx、tools/devsvc-core.cx、tools/dcheck.cx、tools/affected.cx(-j 取令牌)、tools/devsvc-repo.tsv(两行配置)、check-flowexec 新组。
验收:两个各 --par 16 的 flow(每节点睡 5 秒)同时跑,期间 `devsvc status` 显示占用令牌不超过 16,两者末行 peak_tokens 之和不超过 16;把阈值设成高于当前可用内存,节点全部 NOT_RUN reason=admission 且 rc 3;单节点 55 秒超时行为不变(check-flowrun 原例全过)。
收益:近 9 小时 flow 节点失败加超时 435 个、job.done 非零 80 个里有多少属争抢未分类(未测);按 release-speedup-plan 的 R7(假 UNKNOWN 与假 REFUSED 49 批)与简报的 OOM 事件估计每窗 30 至 60 分,并让 A3 的并行化成为可能。
风险:令牌泄漏(持有者被杀),用文件锁随进程退出自动释放;准入让墙钟变长但不改任何判定;不得把 admission 的 NOT_RUN 当通过。

### P6 写域租约、串扰检测与验收隔离

目标:两个代理不会同时改同一写域;验收与提交不读别人的中间态,读到了也能被指出来。
做法:(1) 租约:task open --write 的前缀写进实例的租约表;与他人未完成工单的写域相交时拒绝并具名持有者与剩余秒数,--share 显式放行并记事件。(2) 写拦截:f 写与事务化 flow 对落在他人租约内的路径拒绝 ELEASE(现在只有写期间的路径锁)。(3) 提交核对:devsvc git commit 的路径集必须落在属主未完成工单的写域内,越域具名拒绝 class=out-of-domain;无工单的提交只允许主调度属主。(4) 串扰检测:task done 与 dcheck 开跑前取"他人脏路径集"(工作树脏路径减去本属主 f 事件写过的),与本次验收所跑批次的输入闭包(rel who-uses 反查)求交;相交时结果行标 CONTAMINATED path= owner=,验收不计通过,rc 3。(5) 验收隔离(需裁决):task done 的验收改在 commit-own 现成的私有求值根(HEAD 加本属主路径,约 1.3 秒、155 MB)里跑,彻底不读他人改动。
写域:tools/devtask.cx、tools/cxlib/devtask.cx、tools/devsvc-core.cx、tools/dcheck.cx、check-devtask 新组。
验收:夹具两个属主开相交写域,后者 rc 1 并打印前者工单号;属主甲对乙的写域 f edit 被拒 ELEASE 且文件字节不变;乙在甲的验收闭包内留一个未提交改动,甲 task done 打 CONTAMINATED 并 rc 3;清掉后 rc 0。
收益:plans/devsvc.md 取证里"被他人内容牵连"的提交失败占 12.5% 至 32.5%;简报所述验收读到中间态的返工估计每窗 2 至 4 次,每次 15 至 30 分。
依赖与风险:第 (5) 步触到"不采用私有工作区或覆盖层"的约束;它是一次性求值根不是工作区,与索引第一阶段同一机制,但仍应由主调度裁决后再做,(1) 至 (4) 不需要裁决。按工作树分实例后租约表要放在 git 公共目录键下,否则跨工作树看不见。

### P5 daily 与冻结根的 flow 化(既有方案 D 的细化)

目标:一轮 daily 一条命令起、无人值守走完、任意相中断后同一条命令续上;各相状态可查。
做法:新工具 tools/daily-flow.cx emit ROOT RUN 生成节点表:daily-ready(P4)-> 建根 -> build-shards 与 build-waves 并行 -> 各材料族 map --map-family 并行(按 P2 权重)-> map 封口 -> init-session -> prepare 1 至 6 -> 片(沿用 s9-shards-flow 的节点)-> aggregate 各 part 并行 -> merge -> s9-triage 与上一轮 diff。每个节点就是现有的一个有界相(各自 50 或 55 秒预算不变),相本身已是 intent、done、收据三段幂等,所以"恢复"等于整张表重跑、已完成的相自己 SKIP,引擎不需要新语义。续推用一个有界作业 `devsvc flow run --bg --rounds N --deadline-s S`:一轮结束若有 PENDING 节点且未到轮数与截止就再发一轮,每轮之间核真实进程,结束只发一条 job.done。状态查询 `devsvc flow status RUN目录` 逐节点一行(状态、rc、ms、输入身份)。
写域:新 tools/daily-flow.cx 与 check(假 driver 夹具,仿 check-s9-shards-flow)、tools/devsvc-core.cx 的 --rounds;不改 s9 核验合同与任何限时。
验收:夹具假 driver 下杀掉第 2 轮的客户端,再发同一命令,已完成相全部 SKIP 且最终节点集与不中断时逐行相同;真实根上各相 wall_ms 之和与墙钟之比(并发收益)写进报告;无人值守期间相间空转为 0(对比 f77 根 52 分停摆)。
收益:停摆 52 分(实测一次)归零;相间约 9 秒空转乘 43 相加调用方轮询,每轮 5 至 10 分(推算,同 release-speedup-plan 方案 D)。
风险:flow 缺陷放大到整轮,所以第一版只编排、不裁决,裁决仍由 s9-verdict 与 aggregate 出;flow 总截止上限 3600 秒,一轮超过时由 --rounds 接续,不放宽单轮上限。

### P10 长作业静默等待与进程查询

目标:主调度对一个长作业或一组子代理只被唤醒一次;不再用 pgrep -f 与 sleep 轮询(近 9 小时 sleep-poll 告警 73 条)。
做法:(1) `devsvc wait JOB... --until-done --deadline-s N --quiet`:供宿主后台任务使用,终态或截止前不输出,结束时每个作业一行 TERMINAL id rc ms first=;到截止打 WAIT_DEADLINE 并附作业真实 PID、PGID 是否存活,不宣称终态。有总截止与轮询次数上限。(2) `devsvc events --wait task.done --ids a,b,c --all --deadline-s N`:等齐一组工单(完成或过期)后一次输出每单一行,代替逐单事件唤醒。(3) `devsvc ps [--owner O] [--job ID]`(排期 P5 未做):按作业元数据与进程组列真实 PID、PGID、已运行秒、命令前 80 字节;`devsvc kill JOB` 只按自己属主的作业进程组杀。(4) 简报里的监控模板固定为这两条命令。
写域:tools/devsvc-core.cx、check-devsvc 新组。
验收:起三个各睡 3、6、9 秒的后台作业,wait --until-done 只在约 9 秒处输出三行 TERMINAL;把其中一个设成超过截止,输出 WAIT_DEADLINE 且 alive=1;ps 的 PID 与 /proc 一致。
收益:主调度每窗被子代理事件唤醒的次数从每单多次降到每组一次,估计省 20 至 40 分上下文与等待;pgrep 空等消失。
风险:后台等待命令自身必须有界(截止上限 3600 秒)且不得在超时后重启任何东西。

### P11 简报生成与合规核对

目标:简报与工单单一来源;绕开 devsvc 的写与提交在收工时被机器指出。
做法:(1) `devtask brief ID` 从工单记录打印固定五段加工单、检查、监控三行,协调者直接粘贴,不再手写。(2) task done 加合规节点:写域内每个已改文件的当前 sha 必须等于该属主最近一条 f 事件记录的写后 sha(f9dcd1bdbc 已记 path 与 sha),不等即 NONCOMPLIANT bare-write path=;工单期间落在写域内的提交,其 sha 必须出现在 git.commit 事件里(事件需补 sha= 字段),否则 NONCOMPLIANT bare-commit sha=。第一阶段只告警并记事件,采用率稳定后改为挡 done。(3) 钩子对直接 git commit 由告警升为拒绝,前提是在途的提交回执修复已合入。(4) hook-warn.tsv 增一列代理标识(若宿主钩子输入里有子代理字段;未核实,没有就用 DEVSVC_OWNER 写进命令前缀的约定)。
写域:tools/devtask.cx、tools/cxlib/devtask.cx、tools/devsvc-core.cx(事件字段)、tools/guard-hook.cx 与 check。
验收:夹具里用 shell 重定向改写域内文件后 task done 打 NONCOMPLIANT bare-write;经 f edit 改则不打;裸 git commit 一次后打 bare-commit;`devtask brief ID` 输出含 task open 的全部 --write 与 --accept 原文。
收益:本轮 git-write 34 次、direct-cx 259 次里与事故相关的比例未测;估计每窗少 1 至 2 次串扰排查。
风险:生成物被工具写入(derive --write、register)会被误报为裸写,这些工具要经 devsvc 子命令发 f 类事件或列入白名单表。

### P12 门用例矩阵表驱动

目标:新增一道提交门时自动获得三种仓形态的用例,不再出现"新门在夹具仓硬拒 8 批"。
做法:check-precommit-gates 按 tests/precommit-gates.tsv 的每一行自动展开三例:真实形态相交时照跑、读集不相交时 NOT_AFFECTED、夹具仓(无 tests/run.cx 或无 tests/tools-compile.cx)与工作树形态下不硬拒。表里新增一行而检查没有对应三例时检查自身失败。未入表的 freeze、tlib、new_tools 三道门先补读集行(索引第二阶段 b 已要求)。
写域:tools/check-precommit-gates.cx、tests/precommit-gates.tsv、tools/precommit.cx(三道门入表)。
验收:临时往表里加一行假门,check 报 missing-matrix gate=名字;v0.0.68 现有 7 行(main 为 6 行,缺 toolsreg)每行三例共 21 例全过。
收益:本轮 toolsreg 一道门的夹具返工约 50 分(06:43 到 07:33,实测间隔)。

## 6 D:可模板化的手工修复清单

| 序 | 本轮手工修复(提交) | 归类 | 落到 | 优先级 |
|---|---|---|---|---|
| 1 | 派生表重算 18 加 6 个提交;tools.cxlib 四次升版 | 生成器进提交流 | P1 | 高 |
| 2 | tools-compile 漏登 4 次、片数漂移 1 次 | 生成器加结构改造 | P3 | 高 |
| 3 | 材料生产者补 5 族;冻结根缺伴随文件;入口 cx-plan 被拒拖垮整片 | 静态就绪检查 | P4 | 高 |
| 4 | 批次与检查拆组 7 次(d4b68937e5、082ef34279、41b04ea8db、01d984c634、90c8808d2b、b2aa9c41d1、d729fae9af) | 生成器 batch-split 加登记告警 | P9 | 高 |
| 5 | .git 是文件 3 次;夹具仓判定 2 次 | 棘轮加共用帮手 | P8 | 中 |
| 6 | 新门夹具硬拒、门用例缺形态 | 表驱动用例矩阵 | P12 | 中 |
| 7 | 检查预期过期 3 次(解析别的工具输出行:整行相等、取末行、取首个 status=) | 输出行合同表:生产者工具、行首词、消费者清单;cxlib 提供"取最后一条带词行"的帮手;lint 拦 tail -1 与整行相等 | 新表 tests/line-contracts.tsv 与 cx-lint 规则 | 中 |
| 8 | pins 未申报 3 次(61e730f9d8、43378d1a91、77661df5a5) | 提交门:对暂存的 .cx 跑 pins 的增量核对 | precommit cx 门内加一项,读集 *.cx | 中 |
| 9 | 检查读已归档旧路径(stream-prune-cases、slice-migration-gen) | 棘轮:tools 与 tests 源里形如仓内路径而不存在的字面量;derive.tsv 的 inputs_glob 至少命中一个文件 | 新棘轮 dangling-path 与 derive --check 加一条 | 中 |
| 10 | 预算 58 改 55;chain-batchkeys 仍为 90 | reg-manifest 规则:预算不超过节点上限,超出须有 limits 覆盖行 | reg-manifest | 低 |
| 11 | 片 need 公式漏算子进程固定开销 | limits.tsv 不变量可执行化并遍历全部消费者 | limits-gen --check(并入 P4 第 8 项) | 中 |
| 12 | devsvc 非零 rc 误报、task 缺失、--keyed 覆盖、回执丢失、/tmp 误拦 | 已修或在途,不重复 | - | - |
| 13 | 检查器认不得被检源新写法(chain-argv-check、graph-anchor) | 改工具时其反向依赖的检查批没跑:提交流对暂存工具的直接检查批强制实跑(每批 55 秒并发,超时记 DEFERRED 并进"欠验批次"清单,不记通过) | devsvc git commit 加 owned-checks 节点 | 中,待 P9 的耗时表先落地 |
| 14 | 测试往根内写且不清(streams-material) | 既有 outdep 棘轮是否覆盖该写法未核;补一条"批结束后根内无新文件"的 run-batch 后置核对 | tests/run-batch.cx | 低 |

## 7 E:铁律兼容性与回滚

| 方案 | 不缓存测试结论 | 不放宽时限 | 读并行写串行 | 未实跑不记 PASS | 回滚 |
|---|---|---|---|---|---|
| P1 提交即派生 | 只生成派生物并过各自 --check | 生成器各自上限不变 | 在私有根里写,落 ref 仍比较并交换 | 未命中记 NOT_AFFECTED | 环境变量关闭该步 |
| P2 准入 | 无关 | 等待不计入节点 55 秒,计入 flow 总截止 | 只限制并发总量 | 排不上记 NOT_RUN reason=admission | 令牌数设为 0 表示不限 |
| P3 定片 | 无关 | 各片 55 秒不变;最大片设源数上限 | 无关 | 全表对拍集中到单批,断言逐条对照不减 | 表头 K 与旧算法并存一版,对拍后再切 |
| P4 daily-ready | 静态核对,不写收据 | 每节点 55 秒 | 全只读并行 | 末行 static-only-not-a-verdict | 不接入任何门即回滚 |
| P5 daily flow | 只编排 | 各相预算与 s9.cmd 不变 | 相内写由相自己的 intent 保护 | 裁决仍由 aggregate | 退回调用方循环 |
| P6 租约与串扰 | 无关 | 无关 | 写域互斥是写串行的强化 | CONTAMINATED 不计通过 | --share 与关闭开关 |
| P7 锁不失效 | 无关 | 拉起等待上限 40 秒 | 强化 | 无关 | 环境变量允许本地无锁(仅检查用) |
| P8 工作树与棘轮 | 无关 | 无关 | 无关 | 无关 | 棘轮基线可回退 |
| P9 耗时表 | 只记耗时,任何门不得读它判通过 | 用于提前告警贴线,不改预算 | 追加写,单写者 | 无关 | 删除消费者即可 |
| P10 静默等待 | 无关 | 等待命令有总截止与次数上限 | 只读 | 截止不等于终态,附真实 PID | 无状态 |
| P11 合规 | 无关 | 无关 | 无关 | 先告警后挡 | 降回告警 |
| P12 门矩阵 | 无关 | 无关 | 无关 | 无关 | 无状态 |
| A1 对象相分段 | 段收据绑定内容身份 | 把 696 至 734 秒拆到 300 秒内 | 不变 | compare 四类 same 不变 | 步骤表回退 |
| A3 门与整链并行 | 无关 | 无关 | 整链是独立状态域 | 门未全过不得进步 6 | 改回串行 |

需要主调度裁决的三件:(1) P6 第 (5) 步验收在私有求值根里跑是否违背"不采用私有工作区";(2) 实例口径取"每工作树一个"还是"每 git 公共目录一个"(影响租约与准入的作用域,建议后者加锁名带工作树前缀);(3) P3 改 tools-compile 判据的时间点(建议 0.0.68 公开后、0.0.69 冻结前)。

## 8 改动总表(按预估收益排序)

| 序 | 改动 | 主要写域 | 验收命令与预期 | 预估收益 | 依赖 |
|---|---|---|---|---|---|
| 1 | P1 提交即派生 | commit-own.cx、devsvc-core.cx、derive.cx、derive.tsv | 新增批次提交后 `derive.cx -- --check` drift=0;20 个提交内无"重算"提交 | 70 至 190 分每 9 小时窗(估计) | rel impact 表行覆盖;--registered 键选缺陷先修 |
| 2 | P4 daily-ready | 新 daily-ready.cx | 在 42caaf6115、5a071a43df 归档树上分别报出已知缺陷;整条不超过 300 秒 | 每个结构缺陷 70 至 120 分(实测周期) | 材料族映射成表 |
| 3 | P3 tools-compile 定片 | tests/tools-compile.cx、清单与批次行 | 漏登只败 inventory 一批;增删工具后 batches.tsv 无差异 | 每次漏登省一轮 61 分(实测)加 14 至 96 分修复间隔(实测) | 0.0.68 公开后 |
| 4 | P2 机器级准入 | flowexec.cx、devsvc-core.cx、dcheck.cx、affected.cx | 两个 --par 16 的 flow 并跑,peak_tokens 之和不超过 16 | 30 至 60 分每窗(估计);A3 的前提 | P7 |
| 5 | P9 耗时表与自动拆组 | run-batch.cx、register.cx、新 batch-split.cx | 46 秒批被拒登记;拆组后每组不超过 25 秒 | 140 至 280 分每窗(估计,按本轮 7 次) | 无 |
| 6 | P5 daily flow 与续推 | 新 daily-flow.cx、devsvc-core.cx | 中断后同命令续上,节点集逐行相同 | 停摆 52 分(实测一次)加 5 至 10 分每轮(推算) | P2、P4 |
| 7 | A3 门与整链并行、发布 flow | 新 release-flow.cx | 冻结到可发布不超过 7200 秒 | 40 至 50 分每次发布(估计) | P2 |
| 8 | P6 写域租约与串扰检测 | devtask.cx、devsvc-core.cx、dcheck.cx | 相交写域被拒;闭包内他人脏文件打 CONTAMINATED | 30 至 120 分每窗(估计) | P7;第 (5) 步待裁决 |
| 9 | P8 工作树一等与 gitdir 棘轮 | cxlib 帮手、ratchets.cx、devsvc.cx | 棘轮新增 0;`devsvc --wt` 下 dcheck 选批与主检出相同 | 60 至 90 分本轮同类(估计),每版 30 分 | 启动器写入保护 |
| 10 | P7 锁不失效 | devsvc-core.cx | stop 后 f write 结果行 via=svc locks=1 | 正确性;15 至 60 分每窗(估计) | 无,最先做 |
| 11 | P10 静默等待与 ps | devsvc-core.cx | 三作业只输出一次三行 TERMINAL | 20 至 40 分每窗(估计) | 无 |
| 12 | P12 门用例矩阵 | check-precommit-gates.cx、precommit-gates.tsv | 7 行 21 例全过;假门报 missing-matrix | 每道新门约 50 分(实测一次) | P8 的夹具帮手 |
| 13 | P11 简报生成与合规 | devtask.cx、guard-hook.cx | 裸写与裸提交各打一行 NONCOMPLIANT | 15 至 60 分每窗(估计) | 在途的提交回执修复 |
| 14 | 第 6 节序 7、8、9、13 四条检查 | cx-lint、precommit、ratchets | 各自夹具负例被拦 | 每类缺陷一次 20 至 60 分(估计) | P9(序 13) |
| 15 | A1 对象相分段 | candidate-chain.cx | 单次最大值不超过 300;compare 四类 same | 合规;失败重做 12 分降到 3 分(估计) | 豁免 2026-11-30 到期前 |

## 9 未核实与局限

(1) 负载 48 与 OOM、P53 原始 record 缺失、source-origin 材料三件只见于简报,本轮没有找到留存证据。
(2) tests/tools-compile.cx 的片数算法(第 268、278、395 行)引自 release-speedup-plan 5.2,本轮没有重读源码;P3 动手前须先读。
(3) rel impact 的 table 行是否覆盖全部 10 个派生生成器没有逐个验证,P1 的"只跑命中的生成器"以此为前提。
(4) 所有"每窗省多少分"都是按本轮提交计数乘以单次估时得到的估计,不是测量;单次估时(4 至 8 分、20 至 40 分)来自提交间隔的粗看,没有转录统计。
(5) devsvc-budget 的分位是全历史口径,--bg 作业的命令事件 ms 是起作业耗时;第 3 节引用时已注明。
(6) 宿主钩子输入是否带子代理标识没有核实,P11 第 (4) 步以此为条件。
(7) 本轮的一次性分析脚本在 /tmp/rv(ev.cx、hw.cx、te.cx、bt.cx、b55.cx),未入库;写的时候没有遇到缺库能力(用了隐式运行库的 fopen、fgets、strtol、strstr)。
(8) 新工具名 daily-ready、daily-flow、release-flow、batch-split 动手前须按规则先跑 `git log --all --oneline -- tools/名字` 核历史;本轮只核了当前树里不存在。

## 10 每次调用固定开销的分段实测与启动桩原型(2026-11-16 追加,未入库代码)

口径:本机 load1 2.5 至 3.5,每项 10 至 12 次,毫秒,写作 中位/最小。

(1) 现状(HEAD 0dc640707a 的 tools/devsvc.cx):f read README.md 1 1 为 178/172;x --par 1 -- true 为 187/183;git status 为 210/199。
(2) 分段:sh 加 unisacc.com 自身启动 5;固定执行体直调 f read 5、x 11、git status 38(其中 git 自身 32);
    启动器自身逻辑(fastgit、备忘、产品 sha 备忘、执行体 1.2 MB 整读加 sha、写配置表)约 14(strace 时间线),其中执行体 sha 约 4 至 7,配置表每次原子写含一次 fsync。
    其余约 120 全在产品即编即跑命中路:启动器 main 开始之前,产品把 use 闭包(time、fs、posix、proc、env、sha、osx、sysx.process、bytes、heap 加被带入的 sysnums、reaper)全部源文件读三遍、CXIF 读两遍,再装载 52 万字节缓存映像。
(3) 闭包大小与命中开销(空 main,中位):无 use 为 7;sys.env 为 9;sys.sha 为 9;sys.fs 为 22;sys.posix 为 64(带入 29 KB 的 sysnums);sys.proc 为 91;启动器现有十条 use 为 120。
    结论:最大的一段不是 git 调用、不是与服务通信,而是启动器自己的 use 闭包在产品命中路上的重复扫描;候选 (1)(2)(4) 在启动器内已基本做完(fastgit、备忘),剩余可减不足 10。
(4) 启动桩原型(/tmp/dvx/devsvc-stub-prototype.cx,未入库):只 use time、fs、env、sha、bytes、heap;只走命中路(fastgit 读出提交号、备忘命中、产品 sha 备忘、执行体 sha 相符),
    键算法与现行逐字相同(同一段源码按行段拼入);任何不命中(--dev、DEVSVC_DEV、DEVSVC_REPO_TABLE、备忘缺、执行体缺或 sha 不符、非常规 git 布局)一律 exec 完整启动器,不自己起 git、不自己构造。
    配置表内容相同时不重写(省一次 fsync)。子进程用 fork、execv、waitpid 轮询,保留 3700 秒总上限与工单时钟。
    实测同机同负载:f read 62/60;x 70/68;git status 94/90。即三项各减约 115。
    已实跑的行为对照:退出码透传(x 内 exit 7 得 rc 7;f read 不存在路径 rc 1)、--dev 回退、DV_PIN_DIR 指向空目录时回退并由完整启动器冷构造,输出与现行一致。
(5) 没做完、因此没提交的部分:
    (a) 落库需要把现行完整启动器改名为新文件(暂名 tools/devsvc-launch.cx,动手前按规则核名字历史)、tools/devsvc.cx 换成桩,并让 check-devsvc 各组与 tools-compile 清单跟上;18 个 devsvc 批次每个上限 55 秒、并发 2,本轮时限内跑不完,未实跑不记 PASS。
    (b) tools/devsvc.cx 按工作副本即编即跑,在共享检出里改它立即影响所有窗口,必须一次原子换入并先在 /tmp 副本上把全部批次跑绿。
    (c) 原型在 fastgit 失败(环境带 GIT_DIR 等、reftable)时以 cwd 当根找完整启动器,从子目录调用会找不到;落库前要补一次有界的 git rev-parse --show-toplevel。
    (d) 原型的等待是轮询(前 20 毫秒每 0.1 毫秒,之后 2 毫秒,2 秒后 20 毫秒),超时只杀直接子进程;现行 sy_proc_run 的超时清理范围要对齐后才能替换。
    (e) 桩直接用隐式运行库的 fork、execv、waitpid、usleep、kill,落库前须过 ratchets(cxlibc、cxraw)。
(6) 150 以下的目标桩已达到(62 至 94);要再往下只能改产品:即编即跑命中路对闭包的三遍读与缓存映像装载属 src/ 与 cx/driver(X1),不在 devsvc 写域,建议排进产品迭代:命中判定只读一遍闭包或按 (dev, ino, size, mtime, ctime) 走备忘,届时所有 .cx 工具同时受益(十条 use 的工具每次约省 100)。
