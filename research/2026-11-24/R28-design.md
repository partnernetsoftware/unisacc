来源:会话临时目录 /home/devali/.claude/jobs/7f46e032/tmp/R28/design.md 拷贝于 2026-11-24
# R28 设计裁定:affected 闭包按边分级收窄

日期 2026-11-24。只读取证,未改仓内文件。证据目录 ~/.claude/jobs/7f46e032/tmp/R28/(dump.txt 为默认档逐批闭包,dumpc.txt 为 --tier commit 逐批闭包,均由 `tools/affected.cx -- --closure-dump` 现产,各约 13 秒)。

## 0 立场(一句)

把现行 commit 档的"字面量工具只再跟一层"定为所有档位与材料身份的唯一规则,更深的 spawn 链只经声明表进入;不做"识别 spawn 上下文"的启发式,不要求 1972 个批次逐个声明 spawn 读集;用"实测 spawn 集必须是静态闭包子集"作为漏选的可执行判据。

## 1 实测事实(本次)

(1) 不动点确实存在,但只在默认档与 daily/release 档。默认档 1972 个批次根中 507 个闭包落在同一档(1600 至 1699 节点,含 table 节点);commit 档最大只到 1200 余,不存在 1600 档。
(2) literal-use 边:默认档 380178 条,commit 档 37239 条,差 10.2 倍;table 边 519894 对 142764。use 边两档相同(31546 对 31574),source 相同。
(3) 单文件触达批次数(literal + literal-use),默认档 -> commit 档:tools/edit-span.cx 521 -> 57;tools/install-ape.cx 512 -> 62;tools/ape-pack.cx 565 -> 35;tools/devsvc.cx 507 -> 169;tools/affected.cx 507 -> 183。tools/cxlib/sessionroot.cx 因 use 边 342 条基本不变(730 -> 695),这是真依赖,不该收。
(4) 读码(tools/affected.cx:365-388,716-735):closure_sha 走 mark_root -> mark,而 mark 读全局 lit_unlimited;该全局只在 --tier commit 时置 0。推断:同一材料的身份随调用是否带 --tier commit 而不同(未实跑核对,见第 7 节实验 E1)。
(5) 作者自己已不信字面量闭包当身份:PK_STREAMS 在 closure_sha 之外手工并入 tools/streams-e1-frame.cx 与 tools/isolated-stream.cx 的闭包(affected.cx:1192-1200)。

修正来报:提交门选批(on-change 与 precommit 走 --tier commit)已经是收窄后的;没解决的是三处:材料与原生运行器身份(closure_sha 恒用全传递)、默认档与 daily 选批、evidence-cover 的 COVERED 判定(闭包取 B 树默认档,prd.md:173)。

## 2 结构性原因

"字面量即依赖"把三种不同的东西压成一种边:数据文件读、spawn 工具、仅在提示文案与索引里提到的工具名。dispatcher 型工具(devsvc、release-exec、precommit、tool-index 一类)天然点名几十个工具,传递一跟就收敛到全体。11 月初为堵 ape-pack 两跳漏选(check-release-ape 夹具红数天无人发现)把 daily/release 改成无限传递,是用"语法上完整"换"有区分度",而完整是空的:任一工具改动选中四分之一批次,等于没选。

## 3 裁定一:闭包跟哪些边

三级,所有档位与 closure_sha 一致,不再随 --tier 变深度(--tier 只做批次档位过滤):

(1) 强边,全传递:source、use、include,以及任何被触达的 .cx 节点的 use 闭包。
(2) 数据边,只计不展开:根与被触达工具里的非 .cx 字面量文件(tsv、夹具、.com、.sh),目录规则不变(仅根一层,两段以上)。
(3) spawn 边(.cx 字面量),深度 2:根 -> T1 -> T2,T1 与 T2 各带完整 use 闭包与数据边;T2 的 .cx 字面量不再跟。这正是现行 commit 档语义,ape-pack 两跳案例保持命中。
(4) 深链声明表 tests/affected-deep.tsv(新,列 from、to、why、evidence):确有三跳以上真实执行依赖的,按"工具 -> 工具"声明一条边,声明边不计深度。来源只能是第 4 节的实测 spawn 判据或一次真实漏选事故,不凭猜。
(5) table 边沿用 batch-product-inputs 现有 near/declared 规则,不动。

不采纳:(a) 静态识别 argv 上下文区分 spawn 与文案,C 风格代码里 argv 可任意拼,误判方向是漏选;(b) 每批声明 spawn 读集,1972 行人工表必然腐烂,现表只有 111 行 declared 且靠占位符强制才活着。

## 4 裁定二:防漏选的可执行判据

(1) 历史回放(合入前置,必须过):新增只读开关 --lit-depth full|2(与 --tier 解耦)。对近 50 个触及 tools/、tests/、cx/ 的提交,以 HEAD 树为冻结登记表,逐个取 git diff --name-only,比较两种深度的 AFFECTED 集。输出 DROPPED<TAB>batch<TAB>file<TAB>chain。判据:DROPPED 中 chain 长度不超过 2 的为 0(否则实现有错);其余每个 (file, batch 组) 必须落入下面 (2) 的子集判据或逐条人工裁定并留行。
(2) 实测 spawn 子集判据(核心):在 git archive 私有布局里把 unisacc.com 换成记录 argv 中 FILE.cx 后再 exec 真体的薄壳,把默认档 1600 档的 507 个批次各跑一次(本就在日档范围),得 observed_spawn(B)。判据:observed_spawn(B) 必须是收窄闭包(B) 的子集;每个违例要么补一条 affected-deep 行,要么否决收窄。成本是批次数的一次方,不是批次乘文件。
(3) 变异补盲:(2) 看不到失败分支上的 spawn。对 DROPPED 涉及的前 13 个枢纽工具,各做一次"入口立即 rc 97"变异,只跑被丢弃且 (2) 未观测到的批次的抽样(每工具不少于 20 批,-j 8),任何翻红即真依赖。
(4) 常驻金丝雀 check-affected-narrow.cx 并登记批次:ape-pack 两跳仍选中;一条声明的三跳边选中;负控(改 tools/edit-span.cx 不选中 modelcache-budget-lru);同一节点集的批次数设只降不升的棘轮(今 507,收窄后的实测值为基线)。
(5) 兜底事实(推断,未核):整链与发布资格跑全批不经 affected 选批,所以漏选最坏是推迟到下一轮慢车道发现,不会漏出发布。S0 须核实这一点,不成立则本方案降级为只改身份、不改 daily 选批。

## 5 裁定三:迁移与发布窗口

(1) 候选键不受影响:工具不进候选键(prd_mempalace.md:38)。变的是材料身份、PK_TOOL 类 id、原生运行器 native_key(affected.cx:742)。
(2) 双身份并存:先加 --closure-sha-v2 与 --lit-depth,默认不变,零身份变化,可随时落 main。缓存按身份分目录,新旧互不覆盖。
(3) 翻转是单独一个提交(只改默认深度与 closure_sha 取 v2)。时机:chain-ready 第九条显示无在途链、且刚公开解冻之后;起链到公开之间一律不落。当前快照 0.0.105 已公开并解冻,下一条链起之前即是窗口;若 0.0.103 或任何链在途则顺延到其公开后。
(4) 翻转前预热:用 v2 身份后台跑一遍 --prep,使翻转当刻材料已命中;冷建耗时本次 UNMEASURED,预热时顺带测并记入 build/phase-times.tsv。
(5) 回退:revert 翻转提交;旧身份缓存保留一个版本周期不清,回退后即命中。

## 6 裁定四:切片、负责人、验收、风险

负责人按角色给,具体窗口由主调度按记分表定(本次未核人选)。全部在 main 上做,属纯工具。

S0 只读核实(0.5 天,取证型窗口):E1 身份随档位漂移;整链是否绕过 affected;evidence-cover 现状 MUST_RERUN 占比。验收:三条各一行 TSV 结论带命令。
S1 加 --lit-depth 与 --closure-sha-v2,默认不变(1 天,affected 现维护者)。验收:`affected -- --closure-dump` 与改前逐字节相同;`--lit-depth 2 --closure-dump` 与 `--tier commit --closure-dump` 的 C 行集相同;check-affected-gitnode、check-affected-product-inputs、check-batch-packages 绿。
S2 回放与实测 spawn 工具 tools/affected-narrow-audit.cx 加 check 加批次(1.5 天,另一窗口,与 S1 写域不交)。验收:50 提交回放末行 NARROW_AUDIT dropped=N short_chain=0 unexplained=0;spawn 子集末行 SPAWN_SUBSET violations=0 或每个违例对应一条 deep 行。
S3 gatekeeper 独立复核 S2 证据与变异(0.5 天,unisacc-gatekeeper)。
S4 翻转提交加预热加金丝雀棘轮(0.5 天,集成者,窗口见第 5 节)。验收:edit-span 默认档选中数由 521 降到 60 上下;modal 节点集批次数棘轮落库;翻转后首轮 --prep 的 cache=hit 占比记录。
S5 evidence-cover 改用同一规则并重测 COVERED 占比(0.5 天)。

收益估计:默认档单工具改动选中批次降 3 至 16 倍(实测上表);材料与原生运行器身份只在两跳内的工具改动时变,推断日常 tools/ 提交引起的冷建次数降一个量级(未测,S4 后用 batch-cost 复核)。提交门 affected 7 秒不会因此变快:commit 档早已收窄,7 秒主要是解析 3668 个节点,要靠 devsvc 关系 DAG 增量(候选池 T 的 G 单),不属本单。

砍与延后:砍"spawn 上下文静态识别";砍"逐批 spawn 读集声明表";延后"枢纽停点表"(深度 2 后 devsvc 仍触达 169 批,若 S2 显示其中多数未实测 spawn 再做);延后把 affected 自扫换成 devsvc 关系 DAG。

风险:(1) 三跳真依赖在失败分支上,(2)(3) 都没观测到,漏选推迟到慢车道,缓解是 deep 表加事故即补行;(2) 薄壳记录不到经 devsvc run 原生执行体或 run-cache 直起的工具,S2 须在布局内关掉这两条路或在 px_spawn 层记录;(3) 回放用 HEAD 登记表近似历史登记表,旧提交里已删批次不可比,列为 UNMEASURED 不记零漏选;(4) 翻转当日所有材料冷建一次,若预热不足会顶到 5 分钟分治上限,须分片预热。

## 7 我最可能错在哪里,最便宜的证伪

E1(5 分钟):同一 src 分别在带与不带 --tier commit 的 --prep --plan 下比较 PREP_PLAN 的 id。若相同,则第 1 节 (4) 的身份漂移推断为错,但收窄结论不变。
E2(30 分钟):不写任何代码,拿 dump.txt 与 dumpc.txt 求每批节点集差,取差集中出现频次最高的 20 个工具,人工看 5 个批次是否真会执行它们。若多数真执行,则深度 2 不够,我的方案错,应改为枢纽停点表而非统一深度。
E3:若 S0 发现整链或发布资格的某一步按 affected 选批,则 daily/release 收窄有出发布的漏选风险,方案须收缩为"只改身份与 evidence-cover,选批不动"。
E4:若 S2 的 SPAWN_SUBSET 违例超过 30 条,说明三跳以上真依赖是常态,deep 表会变成第二张腐烂表,此时改走"运行时实测 spawn 集直接入生成表"(由运行器落事实,生成器带 --check),放弃静态深度规则。
E5:507 个同档批次"节点集完全相同"我只按桶推断,未逐集比对;omp-sonnet 对三个根的实测是相同。
