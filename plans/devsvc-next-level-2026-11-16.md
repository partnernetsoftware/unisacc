# devsvc 下一层设计:从工具箱到开发进展的内核
作者:fable-nextlevel(主调度 cc 派工),2026-11-16 08:26 起,只读分析。本文件不含任何新的通过结论;标"实测"的数字来自本轮命令或所引文件,标"估计"的没有跑过,标"简报"的只见于派工简报、本轮未复核。与 plans/devsvc-speedup-2026-11-16.md(下称提速方案,P1 至 P12、A1、A3)和 plans/devsvc.md 第 6 节不重复:那两处的条目只引用。本文回答的是它们共同缺什么结构。按规则不该再写带日期的 plans 文档,本文件是派工指定的落点,吸收进 plans/devsvc.md 后可归档。

## 1 结论

(1) P1 至 P12 每条都在修一类症状,它们共同缺的是两张表和一个循环:一张"门与义务表"(哪些检查存在、读什么、属于哪个目标),一张"观测账本"(哪条命令在哪个输入身份上得到什么结果),以及一个"按目标求陈旧集 -> 按机器预算排程 -> 实跑 -> 记观测"的推进循环。没有这三样,每个 P 都要各自发明输入身份、各自写一份门清单、各自由人串接。
(2) 输入身份现在至少有七种互不相通的私有定义(第 2 节 E1),发布证据表 release/evidence.tsv 的行不带任何身份。所以"换 HEAD 后哪些结论失效"只能靠人记。
(3) 门的定义散在至少九处(第 2 节 E2),其中 tools/flows/daily-ready.tsv 直接抄了 verify-tree.tsv 的五个节点。新增一道门或一个工具要人去补多处登记,这是登记类提交占 32% 的直接原因。
(4) 窗口一次一件的结构原因不是 cdx 本身,而是共享登记表与派生表进了工单写域:91 张工单里 22 张的写域含这些表(实测),这些工单两两相交,只能串行。派生物改由提交流产出之后,写域才能真正不相交。
(5) 建议 devsvc 长出五个内核:身份与观测账本、门注册表、目标规划器、工单合同与写域租约、机器预算与持久编排。合规与简报是工单内核加账本的一个视图,不单列内核。
(6) 账本只记观测、不产生通过结论:状态词只有 FRESH、STALE、NEVER、RUNNING 四个,PASS 只出现在观测行里;发布目标只认输入身份精确等于候选身份的观测;重放一律真实执行。这与现有三处"同输入不重跑"(commit-own 门复用、devtask 验收备忘、evidence-cover)是同一条规则,只是把身份定义统一成一处。
(7) 0.0.68 公开前只做三件小步(第 6 节):观测行落盘、门清单只读汇总、健康指标基线。结构件一律在公开后做,不进冻结窗。
(8) 最大风险是读集声明不全导致错判 FRESH。对策是未声明读集按整棵树算、抽样真实复跑对拍、发布目标不使用读集级身份。

## 2 第一手事实的核实与扩展

| 号 | 简报所述 | 核实结果 | 证据入口 |
|---|---|---|---|
| N1 | 冲刺时间用在修检查与登记漂移 | 实测。v0.0.68 在 11-15 23:00 至 11-16 08:30 有 71 个非合并提交加 4 个合并。说明含"重算"15 个(21%)、"登记"23 个(32%)、"tools-compile"12 个、"拆"4 个、"漂移"8 个、"夹具"12 个、"工作树"10 个。含"重算、登记、拆、漂移、升版"任一词的 39 个(55%)。按关键词计是上界,功能提交也会提到登记。简报的 24%、31%、9% 与此同量级,窗口口径不同 | git rev-list --count --grep |
| N2 | 一次功能改动被拆成多次提交 | 实测。libneed-native 生产者一项:7b42d15453(06:14)、3376c49adf、32779fbaff、193ab4112d(06:28)四个提交,源码 4 个文件,登记与派生 9 个文件(三份 manifest、batches、coverage、batch-packages、links、tool-index、tools-compile-sources) | git show --stat |
| N3 | cdx 一次一件,每单 25 至 30 分 | 部分核实。cdx 在 11-15 16:28 至 22:10(UTC)约 5.7 小时内 21 张工单,严格串行(只有两对重叠),单张 2 至 30 分,多数 13 至 23 分;上一张 done 到下一张 open 间隔约 1 分。25 至 30 分是上沿,不是中位 | 实例 968568b4c968e332 的 task-events.tsv |
| N4 | 写域靠文字约定仍串扰 | 实测结构原因。91 张工单里 22 张(24%)写域含共享登记表或派生表(登记表 19、派生表 20);当前 cdx 在途工单写域列了 7 张共享表。v0.0.68 工作树此刻有 6 张共享表未提交(batch-packages、links、tool-index、batches、coverage、tools-compile-sources) | 一次性脚本 /tmp/nl/stat.cx;git -C 工作树 status |
| N5 | 验收是合同 | 偏弱,实测。91 张工单共 109 条验收命令,平均 1.2 条,其中 18 条只是跑 ratchets。验收命令由开单人手写,不由写域推出 | 同上 |
| N6 | 工具箱可绕开 | 实测。hook-warn.tsv 现有 4811 行:pipe-trim 3699、direct-cx 535、git-read 403、sleep-poll 121、git-write 55。只告警不拒绝 | $HOME/.cache/unisacc/devsvc/hook-warn.tsv |
| N7 | 事实散落,没有发布状态图 | 实测。release/evidence.tsv 八行,列为步骤、结论、路径,路径指向 /tmp 与作业目录,没有 HEAD、树号或候选身份列。chain-ready 九项、verify-tree 六节点、daily-ready 十节点、release-decide 八步各是一张写死的平清单,互不引用 | f read release/evidence.tsv 等 |
| N8 | 负载 48 与 OOM | 未复核,只见于简报。本轮机器 16 核、31 GB、负载 3.8。全仓仍没有按负载或内存的准入 | /proc;git grep |

本轮新增发现:

- E1 输入身份有七种私有定义:(a) 启动器固定键(HEAD 的 self 与库树号加产品);(b) 原生执行体缓存键(产品字节 sha 加 use 闭包内容 sha);(c) toolprep 的 tq_key;(d) commit-own 门复用键(暂存树 sha、门名、工具 blob sha、argv);(e) devtask 验收备忘(HEAD 号加写域内脏路径的 sha);(f) evidence-cover(两棵树求差后问 affected);(g) s9 与整链的会话根、材料身份与收据。其中 (e) 不含写域外的脏文件:别人在验收闭包内留的未提交改动不会使备忘失效,这与提速方案 P6 的串扰检测是同一个洞。
- E2 门定义至少九处:tests/precommit-gates.tsv(6 行)、exec/facts/derive.tsv(9 行)、ratchets.cx(代码内 13 项)、chain-ready.cx(代码内 9 项)、tools/flows/verify-tree.tsv、tools/flows/daily-ready.tsv、build/freeze.tsv、release-decide.cx(代码内 8 步)、tests/batches.tsv(1234 批)。另有 prechain-gate 与 materials-path-gate 两个独立工具。
- E3 "改了什么会影响什么"有六个并行来源:affected 的 use 与字面量闭包、rel impact(读 affected 的闭包转储)、derive 的 inputs_glob、precommit-gates 的 pathspec、links.tsv 的 derive 边(on-change 读)、s9 的材料族映射。选批用一套,判派生用一套,判门用一套。
- E4 检查覆盖:tool-index 里 383 个工具有 197 个带 check(51%);485 个 check 有 446 个登记了批次(92%)。实测。
- E5 提速方案之后已落地或在途的部分:commit-own --derive(P1,已入库,缺省关闭)、verify-tree.tsv(已入库)、写节点无实例即拒(P7,在 devsvc-core.cx 工作区未提交)、daily-ready(P4,暂存未提交)、batch-timing 与 run-batch 写耗时表(P9 第一步,暂存未提交)。P2 准入与 P6 租约在源码里检索为 0。
- E6 实例目录下有约 156 个子目录,事件、工单、作业分在三个实例目录里(045c5a38 有事件与作业,968568b4 有工单,9837e2ae 有事件与作业)。账本与租约若跟着实例走,会同样碎成几份。

## 3 A:根因抽象,六个缺失的结构

| 号 | 结构 | 定义 | 现状差距 | 它解释的现象 |
|---|---|---|---|---|
| S1 | 统一输入身份 | 对任一门,其读集内全部文件的(路径、内容 sha)排序后取摘要;读集未声明时等于整棵树 | 七种私有定义,互不可比;发布证据不带身份 | 换 HEAD 后靠人判哪些要重跑;同输入重复构造 |
| S2 | 观测账本 | 只增的行:命令、输入身份、结果、耗时、属主、工单、日志摘要 | 结果在 stdout、/tmp 日志、子代理报告里;events.tsv 有 rc 与耗时但没有身份与门号 | 没有发布状态图;陈旧结论数不可测 |
| S3 | 门注册表 | 一张事实表列出每道门的命令、读集、所属目标、预算、仓形态、生效方式 | 九处定义,两处互抄;新门直接对全员生效 | 新门在夹具仓硬拒 8 批;漏登使整族必败 |
| S4 | 受影响分析单一来源 | 选批、判派生、判门、判陈旧都读同一份读集 | 六个并行来源 | derive 反复漂移到 daily 才发现;选批与门判不一致 |
| S5 | 工单即合同 | 写域是被强制的租约;验收由写域推出;工单进队列可被任何合格代理领取 | 写域只告警;验收手写;派单靠消息 | 串扰;cdx 串行;换人接手要读转录 |
| S6 | 目标驱动的持久编排 | 给定目标与身份,算出未满足的义务,按机器预算排程,可中断可续 | 平清单写死在各工具里;长作业靠子代理串接;无准入 | 逐项刷屏、空等、并发压垮机器 |

P1 至 P12 与这六个结构的对应:P1、P3、P12 是 S3 与 S4 的三个特例(登记完整性都是"树里的集合与表里的集合应当相等");P4、verify-tree 是 S3 的两张手写视图;P6、P11 是 S5 的两半;P2、P5、P10、A3 是 S6 的三个零件;P9 是 S2 只记耗时的一个子集;P7、P8 是底座修补,不属于这六个结构。

## 4 B:五个内核

### 4.1 K1 身份与观测账本

职责:给每次经 devsvc 执行的门、批次、派生检查、验收命令记一行观测;提供"在当前树上,这条观测是否仍绑定"的判定。不产生任何通过结论。

数据模型:

- 账本文件在 git 公共目录键下的机器本地目录(与实例目录同级,不随实例自退而消失,所有工作树共用),按日分段的只增 TSV。列:seq、ts、kind(gate、batch、derive、accept、phase)、subject(门号或批号)、ident(读集摘要)、tree(HEAD 号,脏则加 dirty 标记)、candidate(候选身份,没有为 -)、argv_sha、rc、word(工具自己打印的状态词原样)、ms、load1、owner、task、log、log_sha。
- 身份由库 tools/cxlib/ident.cx 一处计算:输入是门注册表的读集与一棵树,输出摘要。脏树的身份带 dirty 前缀,只能用于开发视图。
- 派生表:release/evidence.tsv、build/phase-times.tsv 的新增行由生成器从账本加义务表产出,带 --check;过渡期与手写行对拍。

接口:flow 引擎的 exec 节点带键 obs=门号 时,节点结束追加一行(devsvc x 加同名选项);dcheck、task done、derive、verify-tree 的节点表由生成器自动带上该键。读者是新工具 tools/obs.cx:obs list、obs state(逐义务一行 FRESH、STALE、NEVER、RUNNING)、obs replay(真实重跑并追加新行)。

最小可用版本:只做写入与 list,不做身份判定(ident 列先填树号)。验收:`sh ./unisacc.com tools/check-obs.cx` 末行 failed=0;夹具里跑一个带 obs= 的节点后账本增一行且 rc、ms 与 NODE 行相同;把账本设成只读时节点结果不变并打 OBS_WRITE_FAILED(记账失败不改变命令结果)。

预估收益:冻结前"哪些门在这个 HEAD 上跑过"由读八行手工表变成一条命令(估计每次冻结省 20 至 40 分人工核对);陈旧结论数第一次可测;同输入重复构造可被指出(简报与提速方案里一轮 daily 61 分,实测)。

### 4.2 K2 门注册表

职责:门只在一处声明;各目标的节点表、提交门读集、用例矩阵都由它生成;新门先影子运行再生效;登记完整性用表驱动的集合不变量表达。

数据模型:

- 事实表 tools/gates.tsv,列:id、kind(ratchet、derive、static、check、batch-family、phase)、argv、reads(pathspec 列表,或 closure:入口,或 tree)、goals(commit、verify-tree、daily-ready、chain-ready、freeze、release 的子集)、bound_s、weight、shapes(main、worktree、fixture)、mode(shadow、enforce、retired)、since。1234 个批次仍在 tests/batches.tsv,注册表用一行 batch-family 引用,读集取 affected 的闭包。
- 事实表 tools/reg-invariants.tsv,列:name、左集合命令、右集合命令、关系(相等或包含)、修复命令。例:tools 下全部 .cx 等于 tools-compile-sources 第一列;全部 check 源包含于 batches 的 source 列;gates.tsv 每行在用例矩阵里有三种形态。提速方案的 P3、P12、toolsreg、new_tools 各是其中一行,不再各写一段代码。
- 生成器 tools/gates-gen.cx(--check 与 --write)产出 tools/flows/verify-tree.tsv、tools/flows/daily-ready.tsv、tests/precommit-gates.tsv,并登记进 exec/facts/derive.tsv。

接口:precommit、dcheck、chain-ready、daily-ready 读生成的表,判定代码不动。shadow 状态的门照常实跑并记观测,但结果只进账本与 PRECOMMIT_GATES 信息行,不拦提交;在账本里连续 N 次与预期一致(N 写在表里)后由人改成 enforce。门工具自身的改动同样先进 shadow。

最小可用版本:只做 gates.tsv 加 gates-gen --check(对拍现有三张手写表,不切换读者)。验收:`sh ./unisacc.com tools/gates-gen.cx -- --check` 末行 drift=0;临时删掉 gates.tsv 的 pins 行后报 verify-tree.tsv 与 daily-ready.tsv 两处漂移并具名。

预估收益:toolsreg 一道门的夹具返工约 50 分(实测间隔,提速方案 P12)在 shadow 下不发生;两次整族必败的修复间隔 14 与 96 分(实测)变成提交时被不变量具名拒绝;登记类提交占比 32%(实测上界)的下降幅度是估计,目标降到 10% 以下。

### 4.3 K3 目标规划器

职责:把"冻结 0.0.NN""这次提交""这张工单收工"都表达成目标;对给定树算出义务集合,与账本连接后给出状态图与待办。

数据模型:义务不落盘,是 gates.tsv 按 goals 列展开再乘以身份的计算结果。输出行:goal、gate、ident、state、last_seq、last_word、age_s。目标之间的依赖(冻结依赖 verify-tree 与 daily-ready,发布依赖冻结与整链各相)写在 tools/goals.tsv,列:goal、requires、identity_scope(tree 或 candidate)。

接口:`devsvc goal state NAME [--tree REF]` 打印状态图;`devsvc goal todo NAME` 只列 STALE 与 NEVER 的义务并给出节点表;dcheck 与 task done 是 goal=commit 与 goal=task 的两个视图。chain-ready、status、release-decide 的输出格式保留,数据来源逐个改读规划器,先对拍。

受影响分析的单一来源在这里落地:选批、判派生、判门都问同一个函数"这组改动路径命中哪些门的读集"。affected 的闭包是其中一类读集,不被替换。

最小可用版本:goal state 只支持 verify-tree 一个目标,身份取整棵树。验收:在干净 HEAD 上先跑 verify-tree,`goal state verify-tree` 六行全 FRESH;提交任一文件后全部 STALE;账本为空时全部 NEVER 且 rc 3。

预估收益:冻结前缺陷暴露周期 61 分与 70 至 120 分(实测,提速方案)的来源是"到 daily 才第一次跑";规划器在每次提交后给出 STALE 集,把首次暴露前移到提交后几分钟(估计)。换 HEAD 后要重跑什么不再靠主调度记忆。

### 4.4 K4 工单合同与写域租约

职责:工单是唯一的派工载体;写域是强制租约;验收由写域推出;工单进队列,任何合格代理可领;收工时核合规。

数据模型:沿用实例 work 目录的工单文件,新增字段 state(queued、claimed、accept-pending、done、expired)、lane、requires(前置工单)、goal;租约表放 git 公共目录键下(跨工作树可见)。验收命令缺省由工具生成为"对写域内已改路径跑 dcheck",手写命令只作追加。

接口:租约、越域写拒绝、提交路径核对、串扰标记四条与提速方案 P6 的 (1) 至 (4) 相同,不重复。本文增加三条:`task queue` 与 `task next --owner O --lane L`(领取队列中写域可授予的第一张);派生表与登记表不得出现在写域里(由提交流的 --derive 产出,前提是 P1 缺省开启);`task show ID --ledger` 把该单的观测行、已改路径与 sha 一并打印,作为交接材料。

最小可用版本:task queue 与 task next,加"写域含 gates.tsv 标为 derived 的输出时开单拒绝"的检查(先告警)。验收:夹具里排三张写域不相交的工单,两个属主各 next 一次拿到不同的单;第三张与在途写域相交时 next 跳过它并打 SKIP lease-held;写域含 build/links.tsv 时打 WARN derived-in-domain。

预估收益:24% 的工单因含共享表而两两相交(实测)这一项归零后,可并行的工单数由代理数与机器预算决定;cdx 5.7 小时 21 单的串行队列(实测)可由两到三个领取者分担,估计墙钟降到一半左右,未测。

### 4.5 K5 机器预算与持久编排

职责:全机一个预算(核、内存、三类互斥域:工作树、候选身份、harness);长作业是可续的运行对象;推进循环无人值守。

数据模型:令牌与准入同提速方案 P2,不重复。本文增加:权重与内存需求不手填,由账本的实测耗时与(若可得)峰值内存推出,gates.tsv 的 weight 列只作缺省;运行对象是 RUN 目录里的节点表加状态文件,`devsvc goal advance NAME --rounds N --deadline-s S` 每轮做"规划器求待办 -> 准入 -> 实跑 -> 记观测",到目标全 FRESH、轮数用尽或截止为止,结束只发一条事件。单节点 55 秒与流总截止 3600 秒不变。

接口:daily 与发布序列的节点表(提速方案 P5、A3)成为 goals.tsv 里的两个目标,不再各写一个 emit 工具。

最小可用版本:P2 的令牌准入先落;advance 只支持 verify-tree。验收:P2 的原验收;`goal advance verify-tree --rounds 2 --deadline-s 300` 在一个节点被夹具置败时 rc 1 并只发一条 goal.done 事件,期间 events 里没有逐节点推送给调用方的行。

预估收益:整链并发度 1.12(简报)与 daily 一轮 86% 不在测(实测,提速方案)说明排程空间大,但具体提升取决于对象相拆段(A1),本文不估数字;逐项推送与空等(sleep-poll 告警 121 条,实测)归零。

### 4.6 合规与简报(K4 加 K1 的视图)

简报由 `devtask brief ID` 从工单生成(提速方案 P11),本文只补一条:合规不靠事后核查裸写,而靠"没有观测行就没有状态"。裸跑的检查不进账本,规划器里对应义务仍是 NEVER,task done 不放行。绕开因此不需要被逐条抓住,它只是不产生任何效果。

### 4.7 设计总表

| 内核 | 职责一句 | 事实表与生成物 | 接到现有接口 | 最小版本验收 | 收益(实测或估计) |
|---|---|---|---|---|---|
| K1 账本 | 记观测,判绑定 | 账本 TSV;cxlib/ident;evidence 与 phase-times 由其生成 | flow 与 x 的 obs 键;dcheck、task done、derive | check-obs failed=0;记账失败不改命令结果 | 冻结核对省 20 至 40 分(估计);陈旧数可测 |
| K2 门注册 | 门一处声明,影子生效,集合不变量 | tools/gates.tsv、tools/reg-invariants.tsv -> flows 两表与 precommit-gates | precommit、dcheck、chain-ready、daily-ready | gates-gen --check drift=0;删行报两处漂移 | 50 分一次的门返工(实测)不发生;登记类提交目标低于 10%(估计) |
| K3 规划器 | 目标 -> 义务 -> 待办 | tools/goals.tsv;义务为计算结果 | devsvc goal state、todo;status、release-decide 改读 | 干净 HEAD 六行 FRESH,提交后全 STALE | 缺陷暴露由 61 至 120 分(实测)前移到提交后(估计) |
| K4 工单合同 | 租约强制,验收推出,队列领取 | 工单字段扩展;租约表在公共目录键下 | devtask、f、git commit | 两属主 next 拿到不同单;相交单被跳过 | 24% 相交工单(实测)归零;cdx 队列墙钟约减半(估计) |
| K5 编排 | 机器预算,持久推进 | 令牌;RUN 状态文件;权重取自账本 | flow 引擎、dcheck、affected | 两个满并发流令牌和不超过 16;advance 只发一条事件 | 空等告警 121 条(实测)归零;并发提升不估 |

## 5 C:迁移路径

并入关系:

| 现有工具 | 并入 | 方式 |
|---|---|---|
| events.tsv、batch-times.tsv、devtask 的 .pass 备忘、commit-own 门复用记录 | K1 | 先双写对拍,再改读账本;.pass 的身份改用 cxlib/ident |
| evidence-cover.cx | K1 与 K3 | 保留命令行合同,内部改问 goal state 的两棵树版本 |
| precommit-gates.tsv、verify-tree.tsv、daily-ready.tsv、ratchets 与 chain-ready 的项目清单 | K2 | 先由 gates-gen --check 对拍,再切成生成物 |
| toolsreg、new_tools、tools-compile 清单对拍、P12 矩阵 | K2 的 reg-invariants | 一行一个不变量 |
| chain-ready、daily-ready、status、release-decide、dcheck | K3 的视图 | 输出格式不变,数据来源逐个切换 |
| devtask、domain.cx、guard-hook 的写域占用 | K4 | 租约表唯一,钩子读它 |
| s9-nightly 的相循环、candidate-chain 的步骤表 | K5 的目标 | 相本身不改,只换编排者 |

顺序与依赖:阶段 0(底座)-> 阶段 1(K1 写入)-> 阶段 2(K2 对拍)-> 阶段 3(K3 单目标,依赖 K1 与 K2)-> 阶段 4(K4 队列与租约,依赖提速方案 P1 缺省开启)-> 阶段 5(K5 推进循环,依赖 K3 与 P2)。

| 阶段 | 内容 | 单独价值(可停点) | 与 0.0.68 的关系 |
|---|---|---|---|
| 0 | 提速方案 P7 提交入库;账本与租约的存放目录定在公共目录键下 | 写不再无锁;存储不再随实例碎裂 | P7 已在途,可在公开前入库 |
| 1 | 观测行落盘与 obs list | 有了可查的执行历史,健康指标可算 | 公开前可做,只加不改 |
| 2 | gates.tsv 与 gates-gen --check、reg-invariants 的只读核对 | 门清单有了唯一全貌,互抄的两表被对拍 | 公开前只做只读汇总;切生成物在公开后 |
| 3 | cxlib/ident 与 goal state verify-tree | 换 HEAD 后的陈旧集可机判 | 公开后 |
| 4 | 影子门、task queue 与 next、租约强制 | 新门不再直接伤人;工单可并行领取 | 公开后,0.0.69 冻结前 |
| 5 | goal advance、daily 与发布目标 | 长作业无人值守 | 公开后,与 0.0.69 慢车道同窗 |

铁律兼容:

- 不缓存结论:账本行是观测,不是结论。规划器的 FRESH 表示"该义务在同一输入身份上有过观测",报告里连同观测的时间与序号一起显示;任何输出不为 FRESH 新造 PASS 行。提交门照旧实跑。发布与冻结目标的 identity_scope 固定为 candidate,读集级身份不参与,矩阵证据不跨候选迁移的规则不变。obs replay 永远真实执行。
- 同输入不重复构造:这是现有规则,账本让它可被机器核对,而不是放宽。
- 不放宽时限:单节点 55 秒、流 3600 秒、advance 有轮数与截止上限;准入等待计入流截止。
- 未实跑不记 PASS:NEVER 与 STALE 使目标 rc 3,不计通过;shadow 门的结果不计入任何目标。
- 事实表 -> 生成器 -> 表:gates.tsv、goals.tsv、reg-invariants.tsv 是手写事实表,flows 两表、precommit-gates、evidence、phase-times 新行是带 --check 的生成物。
- devsvc 自守铁律:账本与租约是文件,不要求实例常驻;实例的 ttl 与 idle 自退不变;写账本持文件锁。

## 6 立刻可开工的首个任务

发布前两小时内能做完的小步(都只加不改,不进版本分支,在 main 上):

| 号 | 内核 | 任务 | 写域 | 验收命令与预期 |
|---|---|---|---|---|
| T1 | K1 | flow 引擎 exec 节点认 obs= 键,结束追加一行观测;新工具 obs.cx 只做 list | tools/cxlib/flowexec.cx、tools/obs.cx、tools/check-obs.cx | `sh ./unisacc.com tools/check-obs.cx` 末行 CHECK_OBS failed=0;check-flowrun 原例全过 |
| T2 | K2 | 手写 tools/gates.tsv(先收 verify-tree 六节点、daily-ready 五个静态项、precommit 六门);gates-gen 只做 --check 对拍 | tools/gates.tsv、tools/gates-gen.cx、tools/check-gates-gen.cx | `sh ./unisacc.com tools/gates-gen.cx -- --check` 末行 drift=0 |
| T3 | 度量 | flow-health.cx 出第 8 节八项指标的基线 | tools/flow-health.cx、tools/check-flow-health.cx | 对 v0.0.68 窗口输出 tax_commits=39 of 71、orders_shared_domain=22 of 91 两行与本文一致 |

三项的写域互不相交,可同时派。T1 碰 flowexec.cx,须先确认 devsvc-core.cx 的在途改动已提交。

发布后的结构件(每个内核至多三条首任务):

| 内核 | 任务 | 写域 | 验收命令与预期 |
|---|---|---|---|
| K1 | cxlib/ident:读集加树 -> 摘要 | tools/cxlib/ident.cx 与 check | 同树两次摘要相同;改读集内一个文件后不同;改读集外文件后相同 |
| K1 | devtask 验收备忘改用 ident,闭包内他人脏文件使备忘失效 | tools/cxlib/devtask.cx、check-devtask 新组 | 夹具:他人改闭包内文件后 task done 不再打 SKIP_PASSED |
| K1 | evidence-gen:由账本生成 release/evidence.tsv 新行并与手写行对拍 | tools/evidence-gen.cx 与 check | --check 对 0.0.67 的八行 drift=0 或逐行具名差异 |
| K2 | 三张手写表切成生成物,进 derive.tsv | exec/facts/derive.tsv、tools/flows 两表、tests/precommit-gates.tsv | derive --check 含 gates 行且 drift=0;precommit --gates 输出与切换前逐行相同 |
| K2 | reg-invariants.tsv 与执行器,先收 tools-compile 清单与 new_tools 两行 | tools/reg-invariants.tsv、tools/reg-invariants.cx 与 check | 夹具新增未登记工具:rc 1 且具名该路径与修复命令 |
| K2 | mode 列与影子门 | tools/precommit.cx、check-precommit-gates | shadow 门失败时提交 rc 0 且 PRECOMMIT_GATES 行含 shadow_fail=门名 |
| K3 | goal state verify-tree | tools/goals.tsv、tools/goal.cx 与 check | 见 4.3 最小版本验收 |
| K3 | chain-ready 的 2a、2b 两项改读规划器并对拍 | tools/chain-ready.cx | 两种来源下 READY 行逐字相同 |
| K4 | task queue 与 task next | tools/devtask.cx、tools/cxlib/devtask.cx、check-devtask 新组 | 见 4.4 最小版本验收 |
| K4 | commit-own --derive 缺省开启后,开单拒绝写域含派生输出 | tools/devtask.cx | 夹具:写域含 build/links.tsv 时 rc 2 并提示由提交流产出 |
| K5 | 提速方案 P2 令牌准入 | 见该方案 | 见该方案 |
| K5 | goal advance verify-tree | tools/goal.cx、tools/devsvc-core.cx | 见 4.5 最小版本验收 |

新工具名 obs、gates-gen、goal、flow-health、reg-invariants、evidence-gen 与 cxlib 下的 ident、obs、gates 已用 `git log --all --oneline -- tools/名字` 核过,历史为 0 条(实测)。

## 7 D:多 agent 协同

- 共享状态只有三样:门注册表(仓内)、账本与工单队列(机器本地,公共目录键下)。cc、cdx、子代理都经 devsvc 读写这三样,不再用消息传状态。消息只剩"有新工单"这一类通知,可由 `devsvc events --wait task.queued` 代替。
- 派工由"cc 逐张写简报发信封"改成"cc 或规划器把目标拆成写域不相交的工单入队"。入队时就做租约可授予性检查,相交的工单用 requires 串起来,不相交的并行。
- cdx 窗口一次一件的限制不去改它,而是让它不再是瓶颈:cdx 自己 `task next --lane product` 领单,cc 的子代理领 devsvc 车道的单,同一时刻在跑的工单数由领取者数量与 K5 的机器预算决定。车道规则(devsvc 源只归 cc 及其子代理)写成工单的 lane 字段,由 next 强制。
- 读并行写串行:读类工单(审查、取证、只读检查)不占租约,满并发;写类工单按租约互斥;同一工作树、同一候选、同一 harness 三类互斥域由 K5 的令牌表达。所有代理仍在共享检出与版本工作树上工作,没有私有工作区。
- 可替换性:一张工单的全部状态是工单文件、它名下的 f 事件(已改路径与写后 sha)和账本行。工单过期或属主放弃时回到 queued,`task show ID --ledger` 打印的内容就是下一个代理的全部交接材料,不需要读前任的转录。
- 交接环节数的目标:一项改动从提出到入库,人写的消息从现在的"简报、汇报、验收转述、补登记通知"降到只有开单一次。

## 8 E:度量

新工具 tools/flow-health.cx(只读,输出 TSV 到 stdout,带 check),数据来源只有 git log、账本、task-events、hook-warn、build/phase-times.tsv。

| 号 | 指标 | 定义 | 来源 | 现基线 |
|---|---|---|---|---|
| H1 | 漂移类提交占比 | 窗口内说明含重算、登记、拆、漂移、升版任一词的非合并提交数除以总数 | git log | 39 除以 71,55%(实测,关键词上界) |
| H2 | 提交到全部门绿的中位时间 | 每个提交之后,goal=verify-tree 首次全 FRESH 的时间差 | 账本 | 不可测(无账本) |
| H3 | 登记破损修复间隔 | 引入 reg-invariants 违例的提交到修复提交的分钟数 | git log 加不变量回放 | 14 与 96 分(实测,两次) |
| H4 | 冻结后暴露的确定性缺陷数 | 冻结提交之后、公开之前的修复提交数 | git log 加 build/freeze.tsv | 0.0.68 未冻结,不可测;简报称周期 61 至 120 分 |
| H5 | 陈旧结论数 | 发布目标下 STALE 与 NEVER 的义务数 | 规划器 | 不可测(无账本) |
| H6 | 门覆盖率 | 带 check 的工具占比;登记批次的 check 占比 | tool-index | 197 除以 383,51%;446 除以 485,92%(实测) |
| H7 | 并发度 | 各节点耗时之和除以墙钟,按目标分 | 账本 | 整链 1.12(简报);其余未测 |
| H8 | 工单健康 | 收工率;写域含共享表的占比;每单验收命令数;绕开告警数 | task-events、工单文件、hook-warn | 76 除以 93;22 除以 91;1.2 条;4811 行(实测) |

H2、H5 在阶段 1 与阶段 3 之后才有值,这本身是阶段验收的一部分。

## 9 F:风险与反方意见

- 过度设计的边界。不做:数据库(TSV 加文件锁够用);把 1234 个批次逐行搬进 gates.tsv(保持一行引用);自动把 shadow 改成 enforce(留给人);自动生成或拆分工单的智能(规划器只给待办,不替主调度做判断);用系统调用跟踪求读集(太重,先用声明加抽样复跑)。若阶段 1 至 2 做完后 H1 与 H3 没有下降,就停在阶段 2,不做 K3 之后的部分。
- 读集不全会错判 FRESH。这是本设计最大的正确性风险。对策:未声明即整棵树;每轮慢车道对 FRESH 观测抽样真实复跑,结果不一致记为该门的读集缺陷并把它的读集退回整棵树;发布目标不用读集级身份。
- 反方意见一:五个内核会不会成为新的漂移源(gates.tsv 也要人维护)。回答:它替换九处定义而不是加第十处,且由 reg-invariants 核自身完整性;阶段 2 只对拍不切换,对拍不过就不切。
- 反方意见二:提速方案的十二条还没做完,再抬一层会分散力量。回答:P7、P2、P1 仍先做,它们是本文阶段 0、4、5 的前提;本文在发布前只要求三件只加不改的小步。
- 反方意见三:账本会被当成缓存。回答:状态词表里没有 PASS,门工具不读账本判通过,check-obs 里加一条负例(任何门的源码出现读账本判通过的调用即拦,形式同现有棘轮)。
- 与 prd_mempalace.md 的关系:记忆宫殿是给人读的地图,只记决定、不可逆动作与阻塞发布的未验证项;账本是给机器读的观测流水。分工:宫殿节点写"现状一句加入口",入口可以是一条 goal state 命令;build/state-unverified.tsv 的条目与规划器里长期 NEVER 的义务对拍,两边不一致时报出来,由主调度裁决,谁也不自动改谁。本文不改宫殿。
- 失败模式与回滚:账本写失败不影响命令结果(只打一行);生成物切换前后都有 --check,对拍不过即回到手写表;租约强制有 --share 与总开关;规划器只读,删掉即回滚;advance 失败退回现有的手工序列。每个阶段都能单独撤回,不留迁移中间态。

## 10 未核实与局限

(1) 负载 48、OOM、整链并发度 1.12、每单 25 至 30 分取自简报;本轮只核到 cdx 工单 2 至 30 分的分布。
(2) H1 的 55% 是关键词上界,没有逐条人工分类。
(3) 各内核的省时数字全部是估计,只有引用的基线是实测。
(4) flow 引擎能否拿到子进程峰值内存没有核实(cx、ext、tools/cxlib 下检索 getrusage 与 maxrss 为 0 处),K5 的内存预算第一版只能用声明值。
(5) ratchets 与 chain-ready 的项目清单在代码里,抽成 gates.tsv 行时要逐项核对判定不变,本轮没有读这两处的判定代码。
(6) 一次性分析脚本在 /tmp/nl(stat.cx、两个取数 sh),未入库;写的时候没有遇到缺库能力。

## 远期:数据库式表达(项目所有者直觉,2026-11-16)

devsvc 的状态散在几十张 TSV 里,查询多为全表扫(affected 每次遍历约 1300 个批次约 3 秒)。待 cx 的 ext 层有 sqlite 库包后,可获得:(1) 事务:多表原子写入,替代锁加重试;(2) 索引:关系 DAG 的依赖与影响查询毫秒级;(3) 增量物化视图:派生表在写事件上增量更新,即"事件预制缓存"的通用形式,derive 不再全量重算;(4) 历史回溯:事件与状态按时间查询。与"build/*.tsv 迁出"路线合流;前置是 ext 层 sqlite 库包(先绑定 C 实现还是 cx 自写存储引擎待定)。
