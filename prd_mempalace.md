# 记忆宫殿(地图)

本文件是一张图加少量规则,不是账本。主体是下面的流程图:节点是房间(现状一句),连线是依赖与去向,入口写在节点里;过程、取证、重跑记录不进来,放提交说明、链日志目录或 archive/。2026-11-14 之前的全文在 archive/prd_mempalace-2026-11-14.md。

## 写法规则

- 改图只改节点文字或连线,一个节点一句现状加一个入口;过程不入图。
- 只记三类:别人必须遵守的决定、不可逆的对外动作、阻塞发布的未验证项。
- 子代理不提交本文件;主调度只在里程碑(整链结束、公开、装机完成)改一次。
- 版本序在 build/version-seq.tsv,各版内容与验收见对应 plans/v0.0.NN.md(0.0.65 至 0.0.80 的旧版本表已迁入 prd_hist.md「prd.md 9.4 版本表」),未验证项真源在 build/state-unverified.tsv(tools/state-gen.cx 与下方"仍未验证"逐条对拍)。
- 任务表行 `| 部分落地 | ID、ID |` 与 `| 已落地 | ID、ID |` 供 tools/observe-agents.cx 读取。

## 地图

本图只留指针,过期流程图已归档 archive/prd_mempalace-2026-11-23.md(2026-11-23 移出)。

- 版本序与当前状态:build/version-seq.tsv(唯一权威;发布证据索引入口 release/evidence/)。
- 所有者需求与未告知断点:build/owner-reqs.tsv(只降不升)。
- 起草快照(2026-11-24 解冻口):0.0.105 已公开(v0.0.105 Latest,冻结 F=414a024ea5,X=edff8d214d,Y1=63d91a9744,Y2=7f1d244075,release-post 772daa328e,解冻合入 323b70985c;详见 release/notes-0.0.105.md、release/evidence/0.0.105/、plans/v0.0.105.md);0.0.101 已公开(F=8f0a0af2c4,Y2=27ca1178a8,严格证据豁免到期 0.0.102,见 release/notes-0.0.101.md);后续版本序已按路线图重排(e524638bfa):下一版 0.0.103,随后 0.0.102、0.0.122、0.0.124、0.0.107、0.0.106、0.0.109、0.0.108、0.0.112、0.0.123、0.0.125,延后 0.0.104、0.0.110、0.0.111、0.0.113、0.0.114;各版主题只看 build/version-seq.tsv 与 plans/v0.0.NN.md。


## 决定(别人必须遵守)

### 所有者需求登记

- 所有者需求不得静默砍掉:砍掉或顺延所有者提出的需求,必须在本节记一行(需求、原排期、新排期或砍掉、理由、日期),并在给所有者的回复里写明;只写进路线图"不做"行或风险节不算告知。依据:内存安全线曾被静默砍掉。入口:plans/memory-safety.md 第 0 节。
- 登记:向 Rust 学习内存安全(所有者 2026-10-23 提出,曾在 roadmap 与 v0.0.85 计划里被列为不做):已重新编排,路线权威 plans/memory-safety.md(五期、六项采用门指标);甲、乙期为快车道工具期不占版本槽;丙一期条件并入 0.0.95,入口门不过则不并入;丙二期起在 0.0.102 之后新增版号。回滚:恢复 roadmap-0.0.83-0.0.99.md:122 修订注之前口径并在此记一行。
- 登记:四道并行、每版足量(所有者 2026-11-19)不撤销,按"一个候选只含一类大改"执行:只有功能道一类 A 类改动进候选,速度、资格、用户三道在 main 持续落库且不计 planned_units;足量以封顶内完成率衡量。入口:plans/roadmap-0.0.83-0.0.99.md(含 0.0.100 起汇编级精修入口准则)。回滚:所有者一句话即恢复原口径。

### 材料身份(闭包摘要)

- 一次性失效(2026-11-24,3c04bf835b):tools/affected.cx 的 closure_sha 不再计 .git/ 前缀节点(per-checkout 路径,不是源码依赖;版本工作树里 .git 是文件)。main 检出实测四个闭包摘要全变:release-run、precommit、chain-ready 同为 da37137e022a... -> 606d63e227c2...,affected.cx 的 019185220d2d... -> 74d283b679c7...。结果:此提交之后所有按闭包摘要键的材料身份(tools/affected.cx --prepare-materials 的 release-oneshot、release-run 等)第一次需要冷建一次,旧缓存条目失效不再命中。预跑(agy-flow)首轮冷建属预期,不是回归。入口:tools/check-affected-gitnode.cx。回滚:恢复 3e54e46d70 版 closure_sha(计 .git/ 节点,版本工作树里需 git_path_abs 解析)。

### 版本与发布

- 版本序唯一入口:build/version-seq.tsv;各版内容、顺延与对调只在该表改,本节不再记逐版重排。跳号:0.0.64、0.0.69、0.0.70(后两号并入 0.0.71 一次发布)。
- 发布节奏:不设发布频率上限,内容就绪即发;提速靠产线(起链前置门、预跑、提交门只评估提交树),不靠少发版;工具、测试、文档在 main 持续落库,不必触发整链与公开。发布说明如实写已知缺口(未跑 s9 全套就写 NOT_QUALIFIED)。依据:所有者"高速迭代进化"。
- 起链即内容截止:整链起点之后产品输入一律冻结,链后只收阻塞修复(产品输入变更须重起链);工具、测试、文档可并行但不进候选键。依据:plans/speed-first-rev4.md。
- 发布期间 main 冻结(2026-11-24 起,直到分支直发补丁拆小重审落库):现行 release-exec gate_main 要求 origin/main 为发布 HEAD 祖先,main 一前移在途候选即发不出(0.0.99 因此记为未公开,内容并入 0.0.100);候选起链后至 release-post 完成,除集成者与发布动作外不得推 origin/main,版本分支不受影响。依据:gatekeeper 两轮 BLOCK 分支直发补丁,fable 回退裁定。回滚:分支直发补丁落库并经一次发布实证后撤销本条。0.0.105 冻结期出现两次不明推 main(14:31:18、15:02:08),推送者未查出;已在共享仓 .git/hooks/pre-push 加推送审计(日志 ~/.cache/unisacc/push-audit.log,原钩子备份 ~/.local/share/unisacc/research/cc/parked/pre-push.orig),过期冻结行已删(79de13e3c3,原因:devsvc 判过期为解冻而 pre-push 只看 PUSH-FREEZE,冻结门全失效);新 pre-push 冻结门原型 D7(过期不当解冻)在审,落库前冻结仍靠本条人工遵守。回滚审计:用备份覆盖 .git/hooks/pre-push。
- 0.0.105 零重编保 SID(2026-11-24):exec/facts/env-semantics.tsv 恢复为 0bca 版 blob 3a2305921a 以保持 SID 不变,新版表迁到 tests/env-semantics.tsv 作单一读点;exec 下旧副本是待删遗留,须由下一产品版(进候选键的版)删除并同步读点,任何工具不得再读 exec 下旧副本。依据:避免为表迁移触发全量重编。入口:release/notes-0.0.105.md、tests/env-semantics.tsv。回滚:前向提交把 tests/env-semantics.tsv 内容写回 exec/facts 并重起链。
- 开版本分支前置(2026-11-24):没有临时分支;开 v0.0.NN 分支前该号须已在 build/version-seq.tsv 有行且 plans/v0.0.NN.md 已写;0.0.122 至 0.0.125 已立项(cbc9469b03)。版本分支上游设为同名 origin/v0.0.NN,不得设为 main(今日已纠正一例)。入口:tools/verseq-gen.cx(--reorder 只在公开后用,写后须立即提交)。回滚:删本条。

- 发布提速规则(2026-11-24,fable R25 与 R27,0.0.103 起试行):(1) 冻结提交 F 定为起链提交本身,链后不再合任何 main,阻塞修复例外则重起链;度量改为"链止到 X",0.0.105 为 161 分钟,验收线 30 分钟、硬线 45 分钟,起链到公开目标 90 分钟(硬线 120);(2) 发布一律在 .claude/worktrees/v0.0.NN 里跑 release-exec 并推 HEAD:main,不在共享检出合本地未推积压(agy-flow 已证 0.0.105 的 F 不需要合那 13 笔本地积压,仅 c1709e5506 与 72c02fc130 是硬需求);(3) 起链前落后 main 的提交数必须为 0,冻结行已写、分支已合 origin/main 之后才取 tlib 号,冻结期不得取号;(4) 整链三条串行(0.0.103 -> 0.0.124 -> 0.0.122),不并行,整链期间暂停变异门、预检整表、--all、宽并发 affected 与他版开发版重建;(5) 预跑的已审未过期豁免红不应停机(S1,agy-flow)。证据已归档:archive/research-2026-11-25/2026-11-24/R25-plan.md、archive/research-2026-11-25/2026-11-24/R27-design.md。- planned_units 只计进候选键的工作项且必须是数字;类标签与手改 A 类文件封顶由起链前置强制。入口:build/version-seq.tsv、tools/chain-ready.cx。
- 0.1.0 边界与版本归类决定(2026-11-24,集成者据 fable R30 决定,有条件试行;回滚:revert 本条与 version-seq 的归类改动,恢复原行序):(1) xx 上限 129,0.0.129 之后不再开 0.0.x;0.1.0 是从现在起第 7 列车、最迟第 10 列车,时限:ASAP,自所有者 2026-11-24 17:50 的指令起 24 小时内完成 0.0.x 系列(所有者明令,AI 团队不休息,不用日历日期与以天计的目标;逐趟条件触发与压缩手段见 fable R31);(2) 0.1.0 准入七条:六目标自举不动点连续 3 版;CI 六格 QUALIFIED 且严格证据 FAIL/UNKNOWN 为 0;零已知 P0;state-unverified 不超过 3 行;前端覆盖三期完成;整链起到公开连续 3 版不超过 120 分钟;能用清单 30 过 25;(3) 0.1.0 之前必做:103 -> 124 -> 122 并 126 -> 107 -> 109 并 106 -> 125 -> 0.1.0 封板,127 到 129 留给 P0 缺陷批;(4) 0.1.x 再做:123 首位,108 并 114,102,104 并 111,113,110,112,汇编线 115 到 121(含 A1b-A4、pthread、原子与 TLS 顺延,需求仍有效只是排期后移);砍:0.0.96 死行、104 批次 3 与 4 预登记、A1-A4 两段拆法;(5) 试行规则:一版一主类可搭一个字节等价的结构项,拆分阈值由 3 单位提到 6 单位,仅用于 122 并 126、109 并 106 两次同类合并,各自先核写域不相交、起链失败 3 次以上即判无效回到单类;(6) 登记预算:建议行上限 9(0.1.x 期间 8),进一行先出一行;预算满、连续两版起链到公开超 120 分钟、已合入未公开版本超过 2 个,三者任一即停止新增登记;0.1.0 公开前无槽位的新发现不登记,只记未验证项。证据 R30(已归档:archive/research-2026-11-25/2026-11-24/R30-boundary.md;R31 见 archive/research-2026-11-25/2026-11-24/R31-plan24h.md)。- 已发布冻结链 0.0.105 的 errorparse、warnparse、shared-e3 原始 JSON 里,PUSH $f0.r 的返回边目标是空状态(RET 三元组目标 R 0 0 -1 0,真实 f0.r 在别处),由 aa70fc41dc(0.0.92)引入;f0 是 MS.canonical 调用 MS.callablecap 后的内部续点,typed library 签名路径使用。网级证据(cdx 2026-11-24,research/cdx/r24-l2,待归档):把该边目标改为 f0.r 后覆盖实见 RET $f0.r 与 f0.r,随后在 MS.fail 具名拒空签名,说明一旦进入签名子图会走错边;已装产品 -run 最小 struct 返回与函数指针调用 struct 返回都 rc0,普通已装 CLI 无该入口(cdx 2026-11-24 normal-entry.md:LI.start 需非空宿主 library/bindings,typed 分支另需 --librarytyped、typed-mode/data-contract;类型化调用语法、struct 返回、#pragma use、tlib 链接都不产生这些宿主资源;冻结 CLI 源无 mr_extra 调用者,无 contract/typed-mode 生产点,旧 libunisacc 归档才显式注入),保留为显式 typed 宿主潜在影响,非全局死边;未修,不并入在途版本。- 新增拒绝两步走:使原先被接受的程序变为被拒的改动,第 N 版只以告警进入并用候选前端清扫全仓;告警命中连续两版为 0 后,才可在单独一版把默认翻成拒绝,该版不带别的 A 类改动。依据:0.0.86 的 D2 直接默认拒绝导致两次失败链。回滚:删本条。
- 线程与原子的分层决定(2026-11-24,集成者定,0.1.x 兑现时按此落,回滚为改本条):(1) 原子(__atomic/__sync 内建、最小 _Atomic)与 TLS 放核心:是语言与编译器能力,每目标各有降级(x86 锁前缀、arm 独占/LSE、段寄存器或 TPIDR),不是库,只能在 cx/compiler 与 L0 平台表里做;(2) 线程原语(sy_thread 创建/join、mutex、cond、once、tls key,经 futex/CreateThread 等)放核心 L0 cx/sys,平台差异只放 L0 的 _os 静态帮手或表,符合上层不写平台 #if 的铁律;(3) pthread_* 的 POSIX 门面分两层:C 程序可见的最小子集(create/join/mutex/cond/once/key)作为 include/pthread.h 随 C 库面,薄映射到 (2),受 FTRIM 门裁剪;cx 原生封装 API 与可选的重语义部分(rwlock、barrier、spin、attr 全集、取消语义)放扩展层 ext(新包 ext/threadx 或并入 ext/sysx),不进编译闭包、不进候选键。理由:编译器与运行库不得依赖 ext,而 C 程序经产品编译时必须直接拿到 pthread.h;重语义留 ext 可选,避免核心膨胀与每版整链变慢。- s9 commit 档不挡公开(自 0.0.68):冻结后与整链并行跑,结果补记 release/evidence.tsv,新增 FAIL 走补丁版。入口:prd.md 候选池 Q。
- 集成车 v0.0.106 决定(所有者 2026-11-24 指令,集成者执行;回滚:删 .claude/worktrees/v0.0.106 工作树与分支 v0.0.106,版本序与计划行恢复原样(origin/main ce7c2b483e 为基点)):0.0.105 之后下一个公开版定为 0.0.106,作为集成车,把已就绪的未发布分支合到同一个分支 v0.0.106 上、整链一次、公开一次,取代 103 -> 124 -> 122 串行三趟。此决定对本车覆盖"一版一类"铁律(所有者明令),各被并入件仍须各自过 gatekeeper 与自己的检查,合并按写域串行,每并入一件即在 v0.0.106 工作树跑 premerge-union 与 affected 受影响批次。入车条件(满足即并,不满足顺延到 0.0.107 起):已过 gatekeeper 无未清必改、自身检查与变异门过线、对 origin/main 能无冲突或仅号冲突合并。当前入车名单与顺序:0.0.124(运行缓存)-> 0.0.122(转发桩)-> 0.0.108 的 S0 表改与测试(只含已过审部分)-> 0.0.103(内联;2026-11-24 18:55 集成者定:不入第一车——五夹具判据 UNMEASURED,s1..s5 与 one.cx 源文件已丢,只有合成微基准(K=1..16 平均快 32.8%、无一档回归,替代数据不能作入车依据),按降级线第一车不含 103,待找回夹具并做 e3 加汇编内核的真实基准后再入第二车或 0.1.x,证据 B2/report.txt);原 0.0.106 的 CXM_MAXU 256 内容改入第二车 0.0.107(2026-11-24 集成者定,依据 G31:它在 -o 链接层仍是 128,抬高整族要改磁带序数预算与模型网身份,保守方案只做具名拒绝,属半成品功能;其产品级验证项 CHECK_MAXU、UNIT_BOUNDARY 只能在新候选上真跑,放进第一车会让一趟整链因它变红而返工;补丁已备在 archive/research-2026-11-25/ccogweco/cxm-maxu-256.patch 与证据文件,等第二车入)。暂不入车:0.0.102(7 项必改未清)、0.0.125(落后 main 207 提交,先合 main)、0.0.123、0.0.126(未实现)。冻结包与派生表的号只在最后一次合 main 之后、冻结行已写时现取。- decision.txt 必须从当次证据重算并写出,未修不得公开。依据:release-exec 第 7 步曾用停在 0.0.74 的陈旧文件比对恒等。入口:tools/release-exec.cx(现有 check_decision_current 只核引用当前版本号,是否已满足"当次重算"待核)。
- 24 小时节拍(所有者 2026-11-24 17:50 令,fable R31 时间账与集成车合并):单趟整链按保守 240 分钟、目标线 110 分钟估;S2(冻结提交即起链提交、链后不合 main)今天人工执行,S1(预跑已审未过期豁免红继续)首趟起链前落库;两车方案:第一车 0.0.106(124、122、108 的 S0 与测试、103 若五夹具数字达标、CXM_MAXU)、第二车 0.0.107(107、109 并 106 之外的就绪件、125、126 若已过预审)、随后 0.1.0 封板;降级线(T 为本决定时刻):T+1 若 103 无验收数字则 0.0.106 不含 103;第一车起链到公开超 150 分钟则停叠加推测链、砍 125;T+8 已公开不足两车则只保最小集;T+20 无论状态起 0.1.0 封板链;T+24 若 126 仍未修则当趟按 0.0.127 发、不叫 0.1.0;0.1.0 的最小集是 124、122、107、106 与 126 的超限具名拒绝已公开,且六目标自举不动点连续 3 版与零已知 P0 两条为真、不可豁免。砍序:125 -> 103 -> 109,所有者需求顺延须按记忆宫殿登记。- devsvc 工具线不在冻结到公开期间向 main 落库(会触发 MAIN_ADVANCED);列车工具(chain-ready、candidate-chain、bg-candidate、prerun-drive、commit-own、register、release-exec、release-chain)链在跑期间冻结,只修彩排暴露的缺陷。待核:常态冻结是否仍有效。
- demo 发布位置为 docs/uxe/<id>/(GitHub Pages 指 main 的 docs/);docs 改动经 ujs 分支由集成者合入 main,不在共享 main 直接改。回滚:恢复 docs/index.html 原文。
- 整链工具取自 `--rev` 快照与分支合并规则(2026-11-27 fable 裁定):正式链工具一律取自 `--rev` 快照(工具身份 = 候选身份),仅显式 `--tools-root` 可逃生且须写入收据;`main -> 分支` 只经 merge,`分支 -> main` 只经冻结 merge,禁止把分支基点的提交 cherry-pick 到 main;纯编排工具(devsvc、flow、guard)仍在 main 改。依据:避免分支工具与候选脱节及跨基点 cherry-pick 污染。入口:tools/check-chain-tool-identity.cx。回滚:删除本条并恢复按工作区取工具。
- 收据 v2 与人工复核前两项固定执行变异(2026-11-27 fable 裁定):扩展 tools/cxlib/receipt.cx v2(rc_write2 含 run_id/candidate_sha/inputs_sha/output_sha/producer_sha/rc/binding_sha,rc_need(rec,key,kind) 缺字段或格式非法一律拒 rc 2 无默认读,rc_fresh(rec,run_id) 核验新鲜度与绑定摘要),新增或修改的检查/复用点在人工复核前须过 mutation-gate execution 两项固定算子(delete-run-step、feed-previous-output 均 KILLED);首批迁移 check-v0110-exit.cx 与 tape-tldce-shard.cx。依据:cdx 回放证明 M2 可拦 3/3 历史缺陷交付。入口:tools/cxlib/receipt.cx、tools/mutation-gate.cx、w-cdx/.work/gate-falsifier-replay.md。回滚:撤回 6e6644530/bb7cd5287/c47093915 并移出两项固定算子门。
- v0.1.9 定义能力 = 消除上层产品单元静态 libc 副本(2026-11-27 fable 裁定):v0.1.9 主能力定为上层产品单元移除每单元静态 libc/stdio 副本(printf 族迁至 L0 sy_*/mm_*/tx_*),src/ 与 cx/ 守 cxlibc 硬门;磁带迁移不再以体积为判据(改按 tl_dce 活逻辑字节 + golden 全量对拍判定,体积仅作记录)。依据:nm 实测 v0.1.7 g2 中 21 个单元含静态 stdio 核心共 322,888 B .text,首批 3 单元实降 99,193 B、次批 8 单元实降 169,542 B PT_LOAD R E。入口:w/agyneon2 6d6bc885b、w/agyneon bf5be06b2、tools/check-stdio-copies.cx。回滚:按版本分支回退对应提交并恢复原 tests/cxlibc.tsv 基线。
- 统一回执校验(2026-11-27 晚 fable 裁定,cc 执行):tools/cxlib/receipt.cx 增 rc_write3(binding_sha 覆盖全部字段含 rc/verdict/ts)与 rc_verify(消费方必填 rev/unit/candidate_sha/inputs_sha 期望值,拒绝码 10..24 一码一因,v2 回执判 11 不降级),mutation-gate 增 execution --tamper-set receipt 固定 8 项篡改;库落地前各窗口不得自造回执解析器,接口落地前任何单元不得在 main 记 ADMIT。依据:手写伪造回执(全零 sha、rc=1)可令 check-v0111-exit --judge-receipt rc0;cdx 今晚 6 件复核 5 件因回执未绑定身份 REJECT。执行:库 agyneon3,消费方 agy-flow(chain-ready/freeze-cut)、agyneon(admission/golden-m53)、agyneon2(v0111-exit 先行试点)。回滚:rc_fresh 保留,单文件 revert。
- 回执库延期预案(2026-11-27 晚 cdx 冷复核,cc 采纳):若 rc_write3/rc_verify 当晚未落,先停 f75571b29 的 freeze-cut --stage-receipts 消费者(保留实时九阶段),再撤 1ca56d751 的收据发布;f4163f7d3 保留数值校验但代码强制拒绝所有 ADMIT(不单独回退,单独回退会退回手填放行)。证据:w-cdx/.work/review-main-receipts-cold.md(合成 product 正例可放行;stage 与 freeze-cut 冷测各 58 秒超时)。
- 单一编排者(2026-11-27 20:11 fable 裁定 A,cc 执行):编排者 cc 现为会话 b167683f;旧会话 7f46e032(sonnet 后台,PID 977984 PGID 977984,pty host 977978,转录 218400015 字节)终止,其无上限轮询子进程 PGID 2325592 一并终止;旧会话 11-27 的 'RULE: only cc lands on main' 广播与对 1ca56d751、f4163f7d3 的追查作废,两提交有效。依据:双 cc 并行重复派工与并发写记分表、prd_mempalace.md 不可回退。回退:claude --resume 7f46e032 转录(未删)。
- v0.1.9 定义改定(2026-11-27 21:10 所有者令加 fable 裁定,cc 执行;取代第 51 行 v0.1.9 去 libc 定义,去 libc 顺延 v0.1.10):v0.1.9 = 六切片(lnx/osx/win 各 x86_64 与 arm64)同时开工的切片逻辑迁移,一类大改。共用层先行:单一 .cx 可移植执行器(与 lnx/x86_64 汇编执行器字节/rc/步数对拍)、切片身份去目标化与执行器选择事实表、rc_verify 落 main 加 target 字段、准入生成器加目标列。必达:lnx/x86_64 driveargs/m25/bind 到 S3;lnx/arm64 m53 S2 加批 A S1;osx 与 win 四片 m53 S1(默认关,CI 对拍回执)。覆盖 plans/version-roadmap.md:28"跨目标进下一版"。stream_s1_m1.cx 在执行器接口冻结后只由 ccogweco 改。链提速 A1-A5 为 main 工具线,不算版本内容。降级:某片 CI 红则选择表该片回 native 并如实写说明。回滚:选择表全回 native。
- 磁带桥接接口边界(2026-11-27 fable 裁定选项 c):v0.1.9 不引入宿主句柄或回调桥,统一采用 driveargs 形态(宿主薄包装保留指针遍历、出参、回调与跨调用状态,仅拷贝有界字节跨界,guest 无状态且每次调用清 arena),仅暴露 call_u64、fetch_str、call_out 三个桥接原语(call_out 写固定输出区并校验 len <= caller cap 且 <= area,超限具名拒绝);按纯决策核心字节排序迁移,尺寸只记录,不作为回退判据(2026-11-27 迁移加速裁定撤回 2,000B 门槛)。依据:m25 证伪显示每单元定制桥开销约 5.8 KB 反增体积。入口:cx/driver/stream_s1_m1.cx、w/agyneon3 f137e7d01。回滚:改回本条并另立新版本设计有状态 guest 桥。
- TapeRuntime 启动不设校验缓存(2026-11-27 裁定):TapeRuntime 启动加载内置磁带切片时不得设任何校验缓存跳过核验,每次启动均按真实字节与内容身份实核。依据:缓存校验结论违背不冒充执行铁律且会掩盖离线篡改。入口:cx/driver/stream_s1_m1.cx。回滚:仅在所有者修订铁律后经新版本引入有界验证缓存。
- tier 64/102 VM 单元判定 stay-native(2026-11-27 fable 裁定):tier 64 真实 416.2M 步实测所有 [r6-OFF] 栈槽 ld/st 仅占动态步数 14.58%(加 [r7+OFF] 仅 24.3% < 25%),编译器级槽提升无法将 18.33x 压至 15x 以内,tier 64/102 VM 单元判定 stay-native,v0.1.x 退出门以该实测替代 tier 64/102 15x 要求(应用预定义回退,非放宽);事后 regalloc 原型停止,仅留 f5d1ab783 作静态统计工具。依据:tier 64 动态步数实测(load1 2.43)触发路线图预定义回退。入口:tools/check-tape-tldce-speed.cx。回滚:若未来出现可削减超 25% 动态步数的新架构,另开新版本重测 15x 门。

### 不可逆对外动作

- v0.0.105 已公开(2026-11-24,tag v0.0.105 Latest,F=414a024ea5,X=edff8d214d,Y1=63d91a9744,Y2=7f1d244075,release-post 772daa328e)。回滚:按 release/evidence/0.0.105/rollback.txt,release 改回 draft、Latest 回指 v0.0.101 并读回、前向 git revert Y2 Y1 X、Root 回装用 0.0.101 备份(仓根 .unisacc-root-backups/bb715a8bd3.../ 由 install-ape 于 root-upgrade 时写入并登记 index.tsv,镜像 ~/.unisacc-root-backups/0.0.101-public/;回滚命令 sh ./unisacc.com tools/install-ape.cx -- ROOT --rollback 干跑核验后加 --write)。- v0.0.88 已公开(2026-11-21,tag v0.0.88,冻结 F=3038dc3e73,发布提交 X=60c5a4dba3,Y1=0d63ceccd9,Y2=4cbf2a5a53,CI 44902485264 QUALIFIED 42/42)。入口:release/evidence/0.0.88/INDEX.tsv。
- v0.0.88 回滚:按 release/evidence/0.0.88/rollback.txt,release 改回 draft、前向 git revert Y2 Y1 X、Root 从 ~/.unisacc-root-backups/ 经 tools/install-ape.cx 写回;不删 tag、不 force push。待核:其后 0.0.89、0.0.90 已公开(Latest 现为 v0.0.90),Latest 回指步骤需按现状重写。
- 已公开不可撤回的路径泄露:v0.0.85 树内 227 个文件含 /home/ 路径,只泄露系统用户名,无凭据,不重写历史。风险低。

### 证据与门

- 发布证据每候选全套实跑,只复用构造产物,不复用测试结论;"工具契约批次绑定执行编译器身份、跨候选不重跑"未采纳,只做影子核算。依据:1712 行批次里只有 71 行显式接收候选。
- 证据按字节寻址(2026-11-23 fable 裁定):0.0.99 只做影子核算(全批照常实跑,另记镜像/材料/运行库 sha 命中与结论一致性),不跳过执行,不记 REUSED 替代实跑;连续两版零不等且可省不低于 30% 再议改铁律,改铁律须所有者裁定。前置:agyneon 50 工具镜像相同率实验,低于 60% 或编译占比 80% 以上即结案不做。回滚:删影子表并停用生成步骤。
- 提交门类与共用发布路径(dvqualify、release-exec、release-prep、register)的变更验收:夹具绿不算,必须在共享检出同款条件下真实提交一次成功;补丁须独立复核并做原版与补丁版真实发车对拍。依据:三次假漂移都靠真实提交抓到。
- 改产品源的验收必须含引导编译器(bootstrap-0.0.59)多单元编译,不只已装产品 -c。
- cx-dev(tools/cx-dev、tools/cx-dev-root)的结果只是 LOCAL 证据,不是候选结果;基座(src、cx、include)改动要重装产品。
- ref 类判据:"镜像字节等于冻结经典参考"口径已废(X3 起指令选择有意变化),跨版本字节防线用替代判据与 netregen 优化档。
- 整链会话根在链运行与发布期间勿删,清理只由主调度在公开后做。
- 起整链的环境只设 UNISACC_IDENT_HELPER,不设 UNISACC_RUN_CACHE 与 UNISACC_CACHE_DIR。
- 起链前置:版本工作树的 unisacc.com 与 unisacc.com.build.json 必须是真实文件副本,不是符号链接;Root 升级后同步各工作树并移走 ~/.cache/unisacc/chains/snap/ 下旧快照。入口:tools/chain-ready.cx 前置 12 到 14。日常跑工具可放指向共享检出的符号链接(CLAUDE.md),起链前必须换成真实副本,以前置 12 到 14 的检查为准。
- 第二车 0.0.107 起链前:在冻结工作树后台有界预热模块单元材料。G46 的驱动材料冷编约 41 秒,单作业有效上限 50 秒,余量约 9 秒;冷编 `affected` 不得与材料构造塞进同一个 60 秒窗口。先用 `sh ./unisacc.com tools/devsvc.cx -- x --bg --par 1 --timeout-ms 55000 -- sh ./unisacc.com tools/affected.cx -o "$JOB_TMP/affected"` 生成执行体,成功后再用 `sh ./unisacc.com tools/devsvc.cx -- x --bg --par 1 --timeout-ms 55000 -- "$JOB_TMP/affected" --materials-dump --ids module-units,module-units-forward,module-units-limits --prepare-materials -j 1`;`JOB_TMP` 为已建的私有临时目录,两步分别收成功回执。核对 `PREBUILD failed=0` 及两份材料的 SHA-256,只记 PREPARED,三片照常实跑。
  私有验证显式设置私有 `UNISACC_PREP_CACHE`;冻结工作树的正式预热使用后续起链接着消费的缓存。
- 废除 load1 负载门(所有者 2026-11-20 明令):不得按 load1 放行、等待、降并发或拒跑,只用固定显式并发上限(--jobs、--max-jobs、--par、S9_WIDTH),load1 只作记录字段;新工具不得再引入。回滚:git revert ecb8cb246f de000a79d6 39cba77ba6。
- 整链预制:事件触发预制真实命中 0 次,不作依赖;改走推测链加并行确认,采纳内容键待定;推测 APE 与正规 APE sha256 不等则证据自动 INVALID。入口:plans/reviews/2026-11-20-chain-incremental.md 第 9 节。回滚:devsvc react stop,正规链冷起照旧。
- 第二车三源门: `chain-ready --only 3a-cxmod-tri` 先独立编两份会话材料,再实跑核心及导出两片。登记批次可用 `affected --materials-dump --ids cxmod-tri,cxmod-tri-exports --prepare-materials -j 1` 预备内容键材料,核对 `PREBUILD failed=0`;只复用执行体,不复用断言结论。

### 分支与提交

- 分支规则:本地与远端只允许 main、v0.0.NN 与论文 B 车道的 ujs,不建单元分支与短命分支;main 只做集成与发布,一分支一类大改;多窗口共用 .claude/worktrees/v0.0.NN,各自只提交自己的路径;已公开版本的分支在下一版公开后删,只留标签。回滚:关 GUARD-BRANCH-V1。待核:tools/guard-hook.cx 内未见该规则名,且现存 v0.0.89-old。
- 提交不带任何署名行(所有者 2026-11-20 明令):agent 不得在提交说明里写 Co-Authored-By、Signed-off-by 或工具归属行;devsvc git commit、commit-own、dvgitx 与 commit-msg 钩子落库前剔除。入口:tools/cxlib/trailer.cx、`tools/commit-msg.cx -- --check-installed`。回滚:git revert 0566008dfb 并删 .git/hooks/commit-msg。
- 绝对家目录路径(隐私):证据入口与文档一律写 ~/ 或仓内相对路径,不写 /home/<用户名>/、/Users/<用户名>/。入口:棘轮 no-home-path(基线 build/no-home-path-baseline.tsv 只降不升)、tools/no-home-path.cx --staged、改写用 tools/scrub-home-paths.cx --write。回滚:删 ratchets.cx 的 no-home-path 项并还原 check-ratchets 计数。
- 家目录路径豁免(历史证据不改):release/evidence/、research/、archive/、apps/csih、ujs、tests/stream-e1/e2-domain-matrix.tsv、tests/real-inputs.tsv、ape-receipt.txt、tools/check-preflight.cx、build/preflight-rules.tsv、tools/libneed-material.cx、tests/check-guard-edit.cx;后几项为工具生成或断言字面,后续在生成处规范化。
- 写仓内文件只用 Edit/Write 或 .cx 工具。入口:CLAUDE.md。
- tools.cxlib 版本号只在落 main 时按当时 HEAD 现取号,版本分支上的号一律视为临时,合入时重取;layer-api 与 iface-hash 同提交。当前 main 为 0.4.8(件一 0e839333a0 取 0.4.7,0.0.103 分支合入后重取 0.4.8,分支那两次号作废)。依据:分支并行取号撞号。回滚:删本条。
- premerge-union 的 release/candidate.tsv 归版本分支(2026-11-24):版本槽行主分支带上一已发布版、版本分支带自己那一行,两侧必改同一行,三方合并无法自解;tools/premerge-union.cx 的 merge_one_both 把 release/candidate.tsv 与 plans/v0.0.* 同列固定取分支侧(rule=branch-side-version-slot),tests/merge-rules.tsv 的 conflict 行不被该函数消费,故另记 candidate-version-slot 与代码一致。此前 0.0.103 起链 22-premerge-union 因此项红。变异证明:删该 arm 后 check-premerge-union 26/5(含 FAIL version-slot-takes-branch-side),原状 31/0。落地 9b57647531。回滚:撤该 commit。
- 整链 derive 高负载缺口(2026-11-24):chain-ready 的 2a-derive 与 22-premerge-union 在高并发下会把 batch-packages-gen 压过 derive 的 30 秒界(TIMEOUT rc=142),单跑 18.6 秒 ok=1;19-chain-preflight 的 sixtarget 节点同理(整链 49.8 秒 rc=1,单跑 44 秒 PASS)。验收用例归 agy-flow 的冲突域限流件:整链中 derive 节点需按冲突域限并发或让 batch-packages-gen 串行。这不是放宽时效,是限并发。入口:tools/derive.cx 第 138 行附近的原生生成器须知。回滚:删本条。
- git push --no-verify 可绕过全部本地钩子(含推送审计与冻结门);GitHub 分支保护未开,因发布流程直推 main。冻结与审计在服务端不可强制,属已知缺口,改发布流程为分支直发后再开保护。

### devsvc 与协同方式

- devsvc 是最高优先工具线与协同唯一接口(所有者 2026-11-15):写、提交、拿锁、跑工具、查关系只经 devsvc 的 flow 及其子算子;不采用每个 agent 私有工作区或覆盖层;核心不写死本仓路径,成熟后独立成产品。入口:plans/devsvc.md、prd.md 候选池 T。
- 读路径不经 devsvc:读仓内文件直接用 Read 或只读命令。依据:读不需要锁。回滚:恢复 guard-hook 读拦截。待核:guard-hook.cx 的 DEVSVC-ONLY 注释仍列 Read。
- devsvc 源码归属:agyneon 实现、cc 集成(opus 做夹具);omp-go、omp-cdx 只作为使用者提缺口,不改 devsvc 源码。依据:plans/devsvc.md 2026-11-20 演进评审。待核:提交门线 0.0.88 起由 agy-flow 接手是否仍是现状。
- 门迁入 devsvc:precommit、register、derive、ratchets、affected、tlib bump 逐步成为 flow 节点,判定强度不降,先对拍再切换;build/ 只留构建 unisacc.com 用的表,开发元数据逐表迁出(先等价对拍、再切读者、再移除)。入口:prd.md 候选池 T。
- prd*.md 与 plans/*.md 在 devsvc doc 子命令与 flow 的 doc.* 算子落地后只经接口改,不直接编辑。待核:接口是否已落地。
- 派生查询:工具索引与产品全景经 devsvc tool-index、devsvc product-map 现查;两个 build/ 导出不再跟踪,快照仅显式导出,--check 仍现算并逐字节对拍。入口:plans/devsvc.md R1 节。回滚:恢复 derive.tsv 物化行并重新跟踪。
- 分而治之(所有者 2026-11-20 方向):按模块、环节与泳道分割,每个环节写清输入身份、输出身份、等待时做什么;并发只是结果。入口:plans/devsvc.md"分布式演进(2026-11-20)"。回滚:git revert 该决定所在提交。
- devsvc 设计原则(2026-11-24 fable R5 路线与 R4 结构评审):(1) 门核真跑结果与字节身份,不核登记表;(2) 工具失败大声报,过期不当解冻、失败不当零、缺行不当 0;(3) 单一提交入口,私有索引,不卷共享暂存;(4) devsvc 默认按 HEAD 构建,核心改动须加 --dev 自测工作区版本。入口:源已佚(2026-11-24 复核,R5 路线、R4 结构评审原临时文件已无)。回滚:删本条。
- agentwork 已退役到 archive/agentwork-retire-2026-11/:协同决定只进本文件,不追加过程账本;ledger-add 默认拒绝,仅显式 --ledger 可用。回滚:前向提交恢复原路径。

### 编排与工作方式

- 战略层两张表:build/north-star.tsv 与 build/portfolio.tsv(在途上限 7 条,新项进入须先移出一项),编排者只在整链终态、公开完成、每天 21:00 更新;复盘只按这三类时点触发,每次至多一停一启一调;督导用事件驱动,例行巡检 60 分钟。入口:archive/plans-pre090/v0.0.87.md"编排运行模型"。回滚:回到固定轮询,删两表。
- 在建项以 build/portfolio.tsv 为准;重开 T3(推测链)的条件:一次过的版本槽到公开仍超过 200 分钟。回滚:对应行改回 active 并先移出一项。
- 分析禁令到 2026-11-27:不开方案类分析报告,只保留顾问每次不超过 300 字的触发式复盘、守门人复核与失败定因。依据:一天 10 份分析加 22 份评审无人核对执行。回滚:删本条。
- 造工具护栏(试行到 2026-12-04):(1) 优先并入现有工具的子命令,不新开文件;(2) 一次性分析脚本仍用 .cx,留会话临时目录不入库;(3) 每天度量 tools 顶层 .cx 净增数,只度量不拦。依据:每个新工具同时改四到五张共享表,是提交冲突主源。
- 派工:执行类工作由主调度经 devsvc flow 并发自己做,子代理只开 fable 级(设计、难题);给常驻窗口派整块任务,不派一次一个小样例;不因上下文占用高要求 cdx 交接或改派。待核:"omp-cdx 停止派工"(2026-11-16)与其后给 cdx 派版本项不一致。
- 三层分车道(已采纳):产品输入口径以 SID 的 list-current 为唯一事实源生成 build/lane-inputs.tsv,其余口径都读它;promote 按 A 键相同即复用候选;ext 不是纯 B(ext/memx/arena.cx、ext/textx 四文件、ext/sysx 九文件实为 A);B 车道交付不做 submodule 不发 tar。入口:plans/reviews/2026-11-20-layer-lanes.md。
- 三层分车道条文未生效:CLAUDE.md 改写稿在 archive/plans-pre094/claude-md-layer-lanes-draft.md,待 lane-inputs 生成器与包门工具就位后由主调度改 CLAUDE.md 并记决定,此前版本分支规则不变。入口:plans/layer-orchestration.md。待核:build/pkg-release.tsv 尚不存在。

### 仍未验证或未关闭

- freeze-scope 的三个批次行手工登记，以 `tests/run.cx --list` 的 `metadata_valid=1` 和 `run-batch` 实跑为准。依据为 `tools/register.cx:1458` 固定写入无材料列 `-`，而 `tools/cxlib/testplan.cx:107` 的 `tp_args` 要求占位符在材料列声明。普通 register 试跑因 `batch-args` 拒绝并回滚，登记门未过，33 秒结果记 UNMEASURED。将 register 支持材料列的缺口列入 devsvc 迭代，支持后重新走登记校验。证据入口为本机 `~/.local/share/unisacc/research/freeze-scope/supplement/`。回退通过撤销该提交恢复原批与冻结门。

- 会话与记忆被 moltbaby 清理脚本整目录删除(2026-11-23 编排者查明,待所有者裁定清理策略):~/repos/moltbaby/systemd/scripts/system-cleanup.sh 第 147 到 154 行,每小时(system-cleanup.timer)对 ~/.claude/projects/* 超 100M 即整目录 rm -rf,含全部会话转录与 memory/;近 7 天 16 次(日志 journalctl --user -u system-cleanup.service,00:00:12 一行为证)。unisacc 侧不改他车道:已建镜像兜底(硬链接转录加拷贝 memory,见 tools 下镜像工具)并把记分表权威副本进仓;修正补丁 system-cleanup.patch 源已佚(2026-11-24 复核,原临时作业目录已无;若所有者还要须从 moltbaby 侧或旧备份另寻),且原补丁已知存在两处缺陷(会话子目录 mtime 不随深层追加刷新、管道缺 || true 在 pipefail 下会中止后续清理),不可直接应用。入口:所有者裁定后经 moltbaby 车道维护者落。

- 分车道未核项:APE 字节对 ext 改动不变只是推断;14 个链接目标文件是否进隐式运行库前缀;四个 sysx 别名是否有非 use 形式使用者(去别名前必须先查)。入口:plans/reviews/2026-11-20-layer-lanes.md。
- 缺口 OBJABI-ARM64-X7-STACK-BELOW-SP-SIGNAL(arm64 -funit 单元 x7 栈在 sp 之下,异步信号可覆盖):build/pkg-gaps.tsv 仍记 OPEN。待核:0.0.82 版本行写有 x7 修复,是否可关。
- release-flow stage-a 未支持 expected-fail.tsv 的 image=OLD->NEW 指纹豁免(2026-11-24 0.0.101 发布暴露):`tools/release-flow.cx:84` 仅解析 `count=` 未解析 `image=`,第 232 行因 `!b->count` 拒绝全部 `image=` 豁免行;底层正式链(`release-run --pack`、`release-exec --warm`、`release-prep`、`release-exec --execute`)原生支持 `image=` 已全过机检并完成 0.0.101 发布。待发布工具线补齐 `release-flow.cx` 的 `image=` 解析与自测。
- 提交门不跑受影响批次(E4):precommit 只派生与评估提交树,不实跑 affected 选出的批次,改被多处 use 的模块可绿进 main。入口:源已佚(2026-11-24 复核,E4 评审原临时文件已无)。清零:提交门接 affected --run 有界子集并带负控。
- affected 曾因 tests/batch-product-inputs 漏行全线 rc2,已修 c1709e5506;需补"表缺行即具名拒绝"负控,防再次静默。
- run-batch 断言计数盲区(E9):零断言或计数不进结论的批次可记 PASS,候选清单源已佚(2026-11-24 复核,E9 zero-candidates 原临时文件已无);不据此删批,逐批补断言或标 UNKNOWN。
- v0.1.3/C0f 的外置 NN 启动边界待 v0.1.4 收口。候选批若从一个 APE 启动工具、再执行裸切片，继承的同值外壳标记可把旁置解析到启动工具的 APE。安装根旁放置 `nn` 之前，候选批须显式设 `UNISACC_NN_MODE=builtin`；下一版补候选入口的回归。裸切片默认旁置沿用旧规则；下一版验证并统一双信任规则。证据：C0f 真打包 E2E 与 `~/.local/share/unisacc/research/cc/gk-c0f-r2.md` 的 S4、N2。回退：候选入口继续显式强制内置，不用外置旁置。
- tls13-get 证书检查曾失效(已修 7f277e7344);修复前经该工具取得的外部内容不视为已验真。
- old-product-assets:0.0.101 将登记为旧产物基线,由 agy-flow 执行;登记前旧产物对比无基线,结论记 UNKNOWN。

### 仍未验证(缺什么 / 清零动作)

真源是 build/state-unverified.tsv(逐条带缺什么、清零动作、写域、证据);本节只留阻塞发布或带约束的项,其余只列编号。

- 全套资格:118 材料族仅 47 项登记,无生产者与外部主机证据未补齐,全套 NOT_QUALIFIED;入口分片聚合在新候选上端到端未验收。入口:state-unverified.tsv U21 U22。
- 负例静默接受:c-negative 与 pp-negative 共 17 项产品约束缺陷须补具名拒绝并逐项保留负控。入口:U26、release/s9-assert-0.0.71-summary.txt。待核(0.0.87 严格证据称 expected-fail 已清零,U26 未同步)。
- 保留断言与失败的 oracle 缺口:stages 五项字节差异的独立金样、interfaces-lnx-x86_64 既有 oracle 两轮 FAIL,均不删断言不改判。入口:U23 U25。
- core-compat 参考源转换:P4 解析与 SBLOAD 未经独立核验,旧 helper 只批准空 copy guard 转换,不扩大批准域。入口:U20。
- release-chain 端到端 --dry:未用真实证据形态跑全五步,仅夹具与分项检查绿。入口:U30。
- 发布裁决文件陈旧:release/decision.txt 与 release/evidence.tsv 停在 0.0.74,release-exec 第 4 步与第 7 步用同一份旧输入互比恒等,不证明裁决对应本版。清零:第 4 步断言两文件含本候选版本号,否则具名拒绝。入口:tools/release-exec.cx。未入 state-unverified.tsv,待补行。
- ujs compiler_min.cx cxraw 实测 96:不把 96 当合格,不并 G1,不升基线冒充。依据:d473474f14 与 ujs/prd.md C4。入口:U28。回滚:到期未压回则拒绝合入 main 与公开。待核(原文限期写作 0.0.9,疑为笔误)。
- ujs/product 九文件棘轮基线:缺行按实测补,不把缺行当 0;各行 calls 须至少减半。入口:U29。回滚:删这九行棘轮即 FAIL;到期未压回拒绝公开升格。待核(限期 0.0.80 已过,是否已压回未查)。
- 来源未核的改动:共享 tests 下大批纯执行位改动来源未核,提交须排除;额度中断留下的三张小单未验收,隔离树半成品不算已交付。入口:U27、archive/plans/local-product-0.0.71.md。
- 模块接口摘要与缓存键:单元缓存与预构建对象的键是否覆盖所用模块接口摘要未查(结构体扩字段后旧布局对象被复用属同类风险)。未入 state-unverified.tsv,待补行。
- ext/sysx/linuxns.cx:lnx_arm64 实跑 UNMEASURED;私有 lns_sys 带 dock 直调,L0 有 sy_syscall6 后须改一行去 dock。入口:build/pkg-gaps.tsv L0-LACKS-SY-SYSCALL6。待核(0.0.88 已含 sy_syscall6,是否已改未查)。
- 其余不阻塞发布,只见真源:U02 U05 U06 U07 U08 U09 U11 U12 U13 U14 U18 U19。
- forward 调用上限 8192 在产品内不可达,先触发的是更重的缺陷(fable R26,2026-11-24 只读实跑):已装 0.0.105 在约 350 个不同 host-forwarded 符号处无名失败(rc1,只有 model driver: cli),约 450 起 rc139 段错误;其后才是签名池 8064(parse-constants.tsv:2)与 CXM_ITEMS 8192(cxmod.cx:1383)。S0(1-2 相等即拒)仅有表级证据(tests/check-forward-limit-table.cx),0.0.108 按收口不改限值;崩溃另立 0.0.126 单元(定位崩溃点、统一宿主容量与表限值事实源、具名拒绝)。证据:R26 已归档 archive/research-2026-11-25/2026-11-24/R26-ruling.md;R24 源已佚(2026-11-24 复核,原临时文件已无)。未入 state-unverified.tsv,待补行。
### 里程碑与跨窗口约定

哪一版何时公开、冻结提交、资产哈希、CI 编号一律查 build/version-seq.tsv、release/notes-X.md 与 release/evidence/X/,本节不重复。

#### 发布与回滚

- 发布撤回(对已公开的每一版通用):转回草稿、Latest 移回上一公开版、前向 revert,不删标签。依据:公开不可逆。入口:release/rollback.txt。
- Root 回退:用 tools/install-ape.cx 重装上一版公开 APE;公开安装不留候选备份,--rollback 会拒绝(REFUSED no-candidate-backup)。旧件备份在 .unisacc-root-backups/<版本>-public/。
old-product-assets 0.0.101 登记与回滚(c4a154dfea 2026-11-24):tools/old-product-assets.tsv 已登记 0.0.101(8827155B; sha256 bb715a8bd3ed...eac82) 并预置本地缓存 ~/.cache/unisacc/prep-cache/old-product-cache/<sha256>/unisacc.com。回滚:删除 tools/old-product-assets.tsv 中 0.0.101 登记行即回退至 0.0.83; ~/.cache/unisacc/prep-cache/old-product-cache/<sha256>/ 按内容哈希寻址无副作用。
- 资格口径:0.0.71 至 0.0.87 各公开版整体裁决均为 NOT_QUALIFIED(全套 s9 未过或未跑,原生 CI 只覆盖冒烟子集);发布说明须如实写已知缺口,不得称全批资格。入口:release/notes-X.md。
- CI 单格超时处置:先读发码源码行,再用上一公开件与候选在本机各跑三次对照,gh run rerun --failed 只一次,然后 release-exec --from-step 3;第二次失败按确定性缺陷停手;发布说明写明非一次通过。依据:0.0.83 定因。
- stream-pipe 55 秒单部件界限不放宽,余量靠拆部件取得。入口:build/pkg-gaps.tsv STREAM-PIPE-OSX-X86_64-55S-BOUND-THIN-MARGIN。
- netregen 字节栅栏:判据文本不改;有意改变 .cx 镜像的版本只能在 G1 expected-fail 登记 image=旧SHA256->新SHA256 精确指纹并带到期,逐字相等才放行,过期拒绝,下版不同须定因不得续期。回滚:撤 G1 指纹分支与登记行。
- 冻结行带截止,发布若越过截止须在截止前续期或改行;过期冻结行在新 pre-push 门(D7)落库前会被 devsvc 当解冻、被 pre-push 忽略,两端口径不一。依据:0.0.94-FREEZE 过期致冻结门全失效(79de13e3c3)。入口:build/freeze.tsv;D7 原报告源已佚(2026-11-24 复核),后续修订版已归档 archive/research-2026-11-25/2026-11-24/D8-report.txt。
- 发布节奏:日常装机投放加每周一到两次正式公开,公开最多每天一次;仓根 Root 升级与公开脱钩。入口:plans/roadmap-0.0.83-0.0.99.md 末节修订注、archive/plans-pre090/v0.0.87.md S1 S4。
- 内容截止:planned 封顶,超出另开新版;起链即内容截止;计划外满 3 项停收。范围变更由主调度先写回计划文件(四道表与 planned 键值)再派工,内容截止、冻结、公开后三点对账。依据:0.0.83 计划 0/10 而计划外 12 项。
- 无生产者挂账:cx-booter-ape 与 tools-default-regression 已连续 11 版 UNKNOWN,须补材料生产者。入口:build/owner-reqs.tsv R56。
- 0.0.85 公开 release 正文为空且未改。待核(是否已补)。

#### 证据与验收

- 发布证据与资格账本落仓内 release/evidence/,不得只放 /tmp。依据:0.0.71 登记的 8 个证据全部丢失。入口:release/evidence/0.0.71/INDEX.tsv。
- 被发布证据或打包引用的产物须有持久副本(~/.cache/unisacc/release-vNNN、~/.cache/unisacc/candidates/X/);整链会话目录发布后留 7 天;任何清理前先核 release/evidence 与 build/pkg-gaps.tsv 是否引用该路径,有引用先入仓或迁走。入口:tools/cache-retain.cx。
- 改默认行为(默认档、路由、默认输出)的提交,须按断言名全仓搜同组检查的其它平台副本并同提交修齐。入口:tests/mirror-pairs.tsv、tools/check-mirror-pairs.cx。
- 改被多处 use 的模块,验收须覆盖 default-run、-c -funit、-S 三种入口的受影响运行批次,不止编译分片。依据:7026ecaa4d 的 setpgid 回归。
- 扩结构体或改接口须遍历全部消费者;不得手写私有 sy_stat 类结构与原型。依据:0.0.88 首候选因 56 与 88 字节不一致栈溢出作废。入口:tools/check-sy-stat-struct.cx。
- 包格式变更须附工具侧消费者清扫清单并全部实跑。
- 新增批次先在 dvqualify 单提交资格布局自测,再登记。入口:build/pkg-gaps.tsv NEW-BATCH-QUALIFY-LAYOUT-SELFTEST-GATE。
- 测试夹具创建新工具一律经 check-precommit-gates 的共用夹具助手,门加校验只改助手一处。
- PREPARED 与 PASS 严格区分,超窗记 UNKNOWN(窗口)不记 PASS;同一失败连续两次即停手读发码源码行。
- 产品表生成器总闸:tools/gen-all-check.cx 管全部产品表生成器,接 chain-ready 前置与提交门,未落库不起链;未覆盖项须显式列 GEN_UNCOVERED 并带到期,不静默跳过。入口:tools/check-gen-all-check.cx。回滚:git revert。待核(源码内 rt-shared-gen 与 ucn-pp-gen 两项到期写 0.0.87,已过期)。
- R1 release-flow 与 R2 增量预跑两个开关独立、默认关;R2 一例漏选即停用复用。待核(现行开关状态)。
- 已知限制:sysx.process 与 netpack 共用 processcore,不能同单元共导入。入口:build/pkg-gaps.tsv。

#### 派生表与提交门

- 派生表不入 git:tool-index、product-map 等按内容身份现算现查(devsvc tool-index、devsvc product-map,本地需要时 derive --write --only);读集缓存须带清单哈希并有失效负控。回滚:恢复 derive.tsv 对应行与文件跟踪。
- 测试分六域(product-qualification、repository-governance、tool-selfcheck、devsvc-selfcheck、release-matrix、external-product),affected --domain 过滤;新批次域由规则表推断,显式例外不增。
- 提交门方向:只派生与改动相交的表,棘轮按改动归因加落库后全树实跑;收据绑读集内容身份,只限同一事务内自己实跑过的门。红线:不复用批次结论、不删断言、不缩全树覆盖、不放宽 55 秒窗口、不抬基线换绿。
- 不做按负载伸缩的门(失败率与负载档无关,793 次提交实测);单条命令仍不超 60 秒。判据:提交失败率低于 15%、用法错低于 2%,两版不动即回退该项。入口:plans/reviews/2026-11-20-devsvc-evolution.md。
- ujs 车道:全部在 ujs 分支做完并自验收后才合 main(合并前 ratchets 全绿,基线行同并),main 上不中途窄合入。

#### 迭代方式

- 现阶段只聚焦小版本快速迭代:分而治之、事件触发预制(只记 PREPARED,验证仍实跑)、前移、删除工作、内容寻址不重做、度量驱动、快败具名;每版至少收敛一项产品缺口。目标:冻结到公开 30 分钟级,提交门固定税秒级。依据:项目所有者 2026-11-18。
- 预跑并行度可按核数提高,但不改任何单命令时限;按 CPU 秒计量须项目所有者裁定,未替定。入口:archive/plans-pre090/v0.0.87.md S2。

#### devsvc 与产线

- 编排者对接:概念采纳并用 cx 重实现(任务合同、窗口状态机、升级、按身份引用证据),加窗口上报与拉取;不依赖 bun 或 TypeScript;moltbaby 侧钩子由其维护者改。入口:plans/devsvc.md (C)(7)。回滚:子命令按开关停用,派工回退信封。
- 产线是磁盘状态机加有界对账命令,不新增常驻,不做调度器;产线逻辑不进 devsvc-core,走独立小工具;盘上合同只以 plans/devsvc.md 产线化节 (2) 为准;每单元交付必带变异回执;整链期间不动 main 上 A 类输入,提交前核 build/lane-inputs.tsv。
- 产线证伪条件:0.0.88 截止到公开仍高于 100 分钟或列车手敲命令数没降,即停第 2、3 阶段投入。回滚:不接即退回手工。入口:plans/line-phase0-units.md。待核(tools/winstate.cx 在,tools/line.cx 与 tools/art.cx 不存在;判据是否已评)。
- 编排者角色升级只列前置不立项:租约队列与状态总线实跑超一版且有送达率数据、合同摘要防篡改有负控、多实例下截止自退与持锁有检查等。入口:plans/devsvc.md (C)(7) 第 (6) 条。

#### 路线与版本序

- 版本序与各版主题只以 build/version-seq.tsv 为准;改序须同步 plans/vX.md 与路线图修订注,只在公开后用 tools/verseq-gen.cx --reorder,写后立即提交(最近一次 e524638bfa)。回滚:按提交还原这些文件。入口:plans/roadmap-0.0.83-0.0.99.md。
- 所有者需求(2026-11-20,不得静默砍):汇编支持是 C99 加 POSIX 基本需求,内联 asm、.s/.S 与 gcc .o 混编、汇编标签、原子内建不延到 0.0.100 之后;核心层对 asm 的支持应更完善。入口:build/owner-reqs.tsv R20。
- 汇编范围:接受 A1 到 A4,达标线为 lnx 两 ISA 对 gcc 对拍通过;SIMD 内建与 -S 输出 GAS 文本不进基本需求;A4 在 A1 到 A3 对拍通过后重过采用门。入口:plans/reviews/2026-11-20-asm-acceleration.md。
- U4 asm 标签只做 lnx ELF 函数改名,变量改名砍,osx 与 win 具名拒绝;A1 与 K2 不同版(一个候选只含一类大改,产品大改超过 3 单位拆两版)。入口:archive/plans-pre094/v0.0.90.md。
- 核心层强制:层列放 cxroot.tsv 并由编译器强制(dock、upper、tool、非模块脚本四档),不用换后缀;sys.dl、sys.ffi、sys.dlcall 标特权接口;K2 须排在 sy_syscall6 之后;裸 libc 编译期拒绝先出设计。入口:plans/reviews/2026-11-20-core-layer-marking.md。
- 核心层范围:只含 cx 语言支持、libc 对接、系统调用与宿主 ABI 对接、posix 封装,其余归扩展层。依据:核心越小 A 类输入越窄。入口:plans/layer-orchestration.md。
- 核心层需求流程:需求单复用 build/pkg-gaps.tsv,每版开槽前对每条需求四选一响应;接口变更前用 layer-api 判类并列受影响使用者;仓外使用者登记 build/external-users.tsv。入口:plans/reviews/2026-11-20-core-ext-matrix.md。
- 战略:脚本运行时优先;执行器内层循环实验出结论前冻结一切逻辑转网;__int128 砍。入口:plans/roadmap-0.0.83-0.0.99.md 末节修订注。待核(实验是否已出结论)。
- 0.0.91 身份与构建边界:严格先合 L.1 取 SID 快照,再另起提交做 Z5,不可反向;B3 收回 dock 授权的名单待核。入口:build/version-seq.tsv 第 26 行。
- 删 cx/text 下 build、deflate、json、num 四链接:同批提交加 cxroot.tsv 整名行与删链接,绝不先删后加;前后跑 symindex、check-cx-layers、product-links、core-closure;仓外消费者先在 build/external-users.tsv 逐一确认。现状:四文件仍在。
- 冷跑缓存:0.0.94 只做 L5(a) 前导预制;L1 闭包复用本期不开(e4 原字节同率 60%,另有三项挡路点);固定前缀预制曾判 NO-GO,重启须重做 P0。入口:plans/v0.0.94.md、build/owner-reqs.tsv R60。
- 风险登记(均为推断或未核):引导编译器换基与新关键字相撞;SHA256SUMS 无独立签名;缓存目录同用户篡改威胁模型未成文;Mach-O 与 PE 的 TLS 未设计;前端硬上限具名拒绝未普查;主调度单点与磁盘余量低于 20G 预警;用户来源需求为 0。

#### 预跑封口门时序与工具自检迁出(omp-go 备料,cc 2026-11-23 裁定)

- 封口门两端都等 0.0.98 公开后再落:0.0.98 从 main 发,main 上若先有消费侧则 stage-a 必拒。已实测:把 0.0.97 的 strict-final.tsv 按 segment/strict-sha256 写进 inputs 喂新 stage-a,输出 RELEASE_FLOW REFUSE SEAL_MISSING;0.0.97 与 0.0.94-r3 的 strict-final.tsv 里封口行计数都是 0。时序:公开后同一班先落写端,0.0.99 预跑用新 merge 出带封口行的 strict,0.0.99 公开前再落消费侧。回滚:撤两份补丁即回旧行为,旧 strict 重新可发布。入口:research/lane-jobs2/SEAL-GATE-NOTE.md。
- 工具自检迁出选 c:只迁 tool-selfcheck 中 tier=commit 的 578 批,去处 commit-gate;另 330 批(264 daily 加 66 release)留在预跑;加 5 条候选执行冒烟。a 方案(把 264 daily 批改 commit)单独一件,须附提交门耗时实测再定。依据:0.0.97 预跑 797 批实测耗时归属,tool-selfcheck 占 20.4%,其中有去处的 commit 档为 14.2%,daily 档 6.2% 无去处。入口:research/lane-jobs2/SELFCHECK-MIGRATION.md。

#### 清理留痕

- 归档找回:archive/plans-2026-11g/README.tsv 记原路径与原因,git mv 回去或 git log --follow。清理候选(仓根 .o、tools 疑似无用清单、未逐份审的 plans)只列不动。待核(旧版本工作树 v0.0.67 至 v0.0.70 是否仍在)。


#### pp-equiv 对拍证据包(2026-11-23)

全部证据(两 oracle 包身份、全量 5128 项与步数补充)已移到 archive/research-2026-11-25/2026-11-23/pp-equiv-evidence.md。入口保留:tools/check-pp-equiv.cx 靠环境变量 UNISACC_PPEQ_PKG_ORIGINAL 与 UNISACC_PPEQ_PKG_PATCHED 指向两包才跑对拍,未设置或文件不在时打印 UNMEASURED 不算 PASS。

## 编排稳定约定与近两版发布事实(原"编排交接 2026-11-21 21:10")

当前版本状态不在本节维护:以 `build/version-seq.tsv`、`plans/v0.0.NN.md`、`build/owner-reqs.tsv` 为准。

### (1) 窗口职责与合入
- 回信地址:各窗口回报统一发给主调度窗口;主调度停摆时由接班者在 `mux` 声明新地址。 依据:编排约定 入口:`mux`
- 角色分工:主调度管派工与裁决;流程窗口管提交门、合并工作流、预跑工具;发布窗口管发布列车、文档台账、CI 工具、清扫;库窗口管 `ext/` 库;分析窗口管深度审计与补丁;`ujs` 为论文 B 独立车道。 依据:编排约定 入口:`plans/devsvc.md`
- `ujs` 合入:收到合入请求后在共享 `main` 经 `devsvc git merge origin/ujs` 合入,再 `devsvc git push <完整40位SHA>`;绝不直接改 `ujs/` 文件。 依据:车道归属 入口:`tools/devsvc.cx`
- `apps/csih` 待认领:探针中仓根相对层数疑似差一层(`deepseek_proxy.cx`、`libc_cover.cx`),核心仓不动,等 csih 维护者核实。待核(原行号已漂移)。 依据:只读扫描 入口:`apps/csih/probes/`

### (2) 不变式
- 验收:未实跑不记 PASS,未证实不宣称 QUALIFIED;不删断言、不缩覆盖、不放宽时限。 依据:项目铁律 入口:`CLAUDE.md`
- git 写操作:全部经 `devsvc`;共享暂存区只提交自己的明确路径(`--whole`/`--keyed`);严禁 `git add -A`、`git add .`、`git commit -a`。 依据:防卷走他人暂存 入口:`tools/commit-own.cx`
- 发布冻结:冻结期间 `main` 冻结,仅发布提交例外;一个候选只含一类大改。 依据:项目铁律 入口:`plans/v0.0.NN.md`
- 预算口径:由推断得出的"可缩短"数字一律对折后计入时间预算。 依据:编排约定 入口:`build/phase-times.tsv`
- 起链前置:`chain-ready` 必须 20/20,版本槽 `release/candidate.tsv` 须先切到本版;不接受"冻结时再切"。 依据:R80(0.0.90 首链自报旧版号作废) 入口:`tools/bg-candidate.cx`
- 整链续跑:只按原参数续跑,不增删开关;续跑前核前段清单与 `--base/src` 干净树仍在,前置流程不得清空在途链的快照目录。 依据:0.0.89 首链四次 rc=2 入口:`tools/chain-run.cx`
- 证据留存:链与预跑证据产生后立即备份到仓外 `~/.unisacc-release-evidence-backups/<版本>/`,清单入 `release/evidence/<版本>/backup-evidence.tsv`;作废产物保留并标作废,不作发布证据。 依据:R71 入口:`tools/release-chain.cx`
- 重复代码:重复是测量,合并是决定;契约不同或合并净亏的重复组登记白名单,不为降数字合并。 依据:ccogw 盘点 入口:`tools/dup-census.cx`
- 待补用例: `tests/module/boundary.tsv` 的 64 单元记录翻转 `expected` 不会让规划分片失败;旧版也忽略该列,实际仍按 `DV_MAXU` 核计数、展开和拒绝。G46 已实证。另开小项令规划分片断言 `expected`,本次拆片不改判据。

### (3) 已公开版本(不可逆动作)

0.0.89、0.0.90 的公开事实(候选/冻结/Y2/CI/预跑数)与后续版本要点已归档 archive/prd_mempalace-versions-2026-11-23.md。回滚办法与 Root 备份留原位(下两条,不可清扫)。
- 0.0.90 回滚:(1) `gh release edit v0.0.90 --draft=true` 并把 `v0.0.89` 置 Latest 后读回;(2) `main` 从新到旧 `git revert` 后推送;(3) `tools/install-ape.cx` 用 `~/.unisacc-root-backups/0.0.89-public/unisacc.com` 回装并核版本。 入口:`release/evidence/0.0.90/rollback.txt`
- 分支与 Root 备份:分支 `v0.0.90` 保留到 0.0.91 公开;旧 Root 备份在 `~/.unisacc-root-backups/{0.0.87,0.0.88,0.0.89}-public/`,回滚依赖它们,不得清扫。 依据:版本分支规则 入口:`git branch`

### (4) 0.0.90 已知缺口与未验证项

R75/R78/R69/R69.6/R79/R81 等仍跟踪项已归档全文于 archive/prd_mempalace-versions-2026-11-23.md;跟踪真源为 build/owner-reqs.tsv(只降不升),本文件不再逐条维护。

### (5) 后续版本与评审要点

0.0.91 及以后范围与评审要点同上归档;当前版本范围以 build/version-seq.tsv 与对应 plans/v0.0.NN.md 为准。

### (6) 待所有者裁定或告知
- R20 汇编拆版:U4 拆为 0.0.90、其后版本顺延一号,尚未告知所有者;`build/owner-reqs.tsv` 的 `told_owner` 列 72 行全空,统一告知后才可填。待核。 入口:`build/owner-reqs.tsv`
- `sync-company` 等脱敏源码问题:归项目所有者裁定,agent 不自行处置。待核。 入口:无
- `lz-unisacc` 窗口:须提醒改用明确路径或 `devsvc` 提交。待核是否已提醒。 入口:`mux`

### (7) 研究产物入口
- 归档现状:`archive/research-2026-11-25/2026-11-22/` 已入仓收录 34 份设计、评审、变异与盘点底稿(索引见 `archive/research-2026-11-25/2026-11-22/README.md`,记忆宫殿瘦身映射见 `archive/research-2026-11-25/2026-11-22/mempalace-rewrite/dropped.tsv`);历史已丢失的 38 处临时目录引用已清。
- 仓内持久入口:A1a 与 U4 设计 `plans/a1a-design/` 及 `archive/research-2026-11-25/2026-11-22/u4-slices/`;`cxpkg` 设计 `plans/cxpkg.md`;`devsvc` 迁 Windows 路线 `plans/devsvc.md`;速度候选池与热启动定因 `plans/v0.0.94.md` 及 `archive/research-2026-11-25/2026-11-22/r70-review/review.md`;R76 HTTP 服务端设计与二审 `archive/research-2026-11-25/2026-11-22/r76-design/design.md`、`archive/research-2026-11-25/2026-11-22/r76-review2/review.md`;R69.6 零断言守卫 `archive/research-2026-11-25/2026-11-22/zero-assert/phase2-design-v2.md`;发布证据 `release/evidence/`。
- R34 B4 采纳与工具线不冻结:采纳 B4 列车工具五件草案(pre-push、ready、release、devsvc、precommit)作为后续工具的保全基线;工具线不随版本冻结一起停,列车工具在链跑期间仍冻结。依据:五份草案对基线索引 git apply --cached --check rc=0 与各自私有实跑记录。证据入口:~/.local/share/unisacc/research/ompgpt/r34-train-draft-evidence.md。回滚:删本条并撤销 B4 采纳标记,五份草案退回未采纳状态。
- R35 版本重排:最小集缩为三车 0.0.106、0.0.107 与 0.1.0 封版,中途设爬坡检查点(前置门通过即起链、失败只报失败节点原行不重试);0.0.108 等其余件移出封版关键路径。依据:R35 时间账与 24 小时节拍。证据入口:archive/research-2026-11-25/2026-11-24/R31-plan24h.md 与 24 小时节拍条。回滚:恢复原版本序与 version-seq 对应行,删除本条。
- R36 链后发现工具线缺陷的规则:车头不动;修复走路径 A(在下一车前置补丁)或 B(两车之间的独立补丁)或 C(封版后 0.1.x);F 门的自证补丁排在两车之间,不并入首车发布入口。依据:R36 草案与 F 门豁免补丁的静态应用记录。证据入口:~/.local/share/unisacc/research/ompgpt/r36-f-waiver-evidence.md。回滚:删本条,按原"链中发现即停链修"的老规则处理。
- R37 封版读法与两车路线:走 0.0.107 到 0.1.0 两车,0.0.108 移出封版关键路径;封版车为零代码生成变化的版本槽,材料类拆批补丁栈不进封版车;准入条 1 与条 3 不可豁免;条 1 的三版含 0.1.0 自身公开前判定;条 2 的 FAIL 须为 0,UNKNOWN 仅按已登记无生产者项且不超 2;条 4 只计声称类 kind、缺口与债转已知缺口不删行;条 5 的 B2 顺延并在封版说明写未满足;条 6 第三样本取封版车到 READY_TO_EXECUTE。依据:R37 裁定的逐条读法。证据入口:archive/research-2026-11-25/2026-11-24/R37-seal.md。回滚:恢复七条字面读法与 0.0.108 回关键路径,删除本条。
- R38 三项裁定:U33 的 sy_proc_kill 回收后 pid 复用竞态判 P1(非静默错编或损坏自身输出,且需复用窗口);声称类只包含对已发布行为缺实跑的 kind,禁止为过线改 kind;声称类基数按当前表加两条补行与 U21 明示归类为 10(U05、U06、U08、U09、U11、U30、U33、U21、E01、E02)。依据:R38 对每条 kind 的判定与计数。证据入口:archive/research-2026-11-25/2026-11-24/R38-ruling.md。回滚:恢复按全表计数的旧口径,删除本条。
- 0.0.107 追加集:只收 WP4 全部、两个无生产者项(booter-raw-elf、old-checker)、选择器复用、提交门 list-cap,以及 R38 升级的 chain-step P0 修复(第七笔);WP1、WP2、WP3、WP6、WP7、WP8 不进本车。依据:各件是否已有过审补丁、是否触碰产品零变化封版前提。证据入口:~/.local/share/unisacc/research/ompgpt 下的 WP4 补丁页与 P0 账本。回滚:从追加集撤回对应补丁并重排 107 判据,删除本条。
- P0-chain-step 记录:tools/chain-step.cx:486-487 先 wait 回收,失败分支仍 sy_kill(pid),属回收后再杀,发布链失败分支必现;按 R38"发布链路径一处即 P0"升级 P0,由 0.0.107 追加集第七笔修复,关闭证据为补丁、夹具与 107 候选实跑。依据与调用点审读:research/ccogweco/u33-callsite-audit.md。回滚:若补丁被撤回,本条改回 P1 并把 U33 重新覆盖 chain-step 路径。
- 预跑 2 车道规则:预跑并发固定为 2 车道;曾以 3 车道跑 heavy 窗,三路全 rc=124,判定为负载不可行的实测,不作为放宽时限的理由。依据:预跑日志的 rc=124 记录。证据入口:release/evidence/0.0.105/prerun-supervise.log。回滚:删本条,恢复旧并发说明。
- E02 已知缺口:单元缓存键不覆盖导入模块接口,结构体扩字段类改动可能复用旧布局对象;决定不进 0.0.107、不改产品键、不改 kind,只加检查例作已知红复现,按已知缺口写入 107 与封版说明。依据:cx/runtime/unitcache.cx 的键说明与 tools/check-cx-unitcache.cx 的现有覆盖面。证据入口:research/ccogweco/e02-closure.md。回滚:删本条,恢复为未分类声称行。
- 封版后路线基线(after-seal,cc 2026-11-24 采纳):0.1.0 之后按三线并行——版本车继续"一车一类"(2026-11-25 更正(fable 采纳,见下一节):0.1.1 诊断车(0.0.108 收窄为 c15/c16/c17/-o 警告与 diag-contract 基线下调);0.1.2 前端 B2(0.0.109 硬上限事实表化);0.1.3 进程生命周期(0.0.125);0.1.4 转发桩容量(0.0.126);工具线先过 P0 预备窗口再开车);工具线与材料类不占版本车,按 devsvc 例外直接落 main,带 check 批与登记、`--keyed` 只带自己行;材料栈 B1-B9(WP1 到 WP8 与 S5,先测量后优化,登记表同刻单窗口串行);工具线 C1-C5(提交钩子按层跑批、doc-links 纳入棘轮/chain-ready、precommit list-cap 后续、check-split-parents 贴限、预期失败表与两个无生产者项)。依据:research/ccogweco/after-seal-roadmap.md 的件表与依赖图;R37 把 0.0.108 移出封版关键路径。证据入口:research/ccogweco/after-seal-roadmap.md、research/agy-h/margin-packages.md、research/ccogweco/wp6-wp8-plan.md。回滚:删本条,路线回退到 research/ccogweco/car2-next.md 的旧排序。
- R39 路径 B 记录(第一车 W0087 红):0.0.106 车树上加一个 tools-only 提交 c698ef0091("R39 修复 W0087 check-gen-all-drift 最小仓缺 tests/ 触发 affected-canary"),只改 tools/check-gen-all-check.cx 的迷你仓 rm 清单;预跑 strict 表按折叠规则记录同一红指纹(同批号加同断言文本)的多行,修复后以候选 APE 的复跑行替换,原行保留不删,不准登记预期失败;provenance 由提交只动 tools/ 证明:改动不在 SOURCES_SID 路径集,候选 APE 字节与源快照 SID 不变,以 `git diff --name-only 2e8a19a342 HEAD` 只有该一个文件为核。依据:R39 裁决页与提交 c698ef0091 的实际内容。证据入口:archive/research-2026-11-25/2026-11-24/R39-ruling.md;git show c698ef0091。回滚:撤回该 tools-only 提交与折叠行,按路径 C 重链;不把修复改成放宽真实门。
- 0.0.107 追加集十份与顺序:按顺序为 (1) chain-step 回收后再杀 P0 修复,(2) WP4 batch-timing,(3) WP4 tsv-lint,(4) WP4 ratchets-tool,(5) WP4 install-ape,(6) cx-booter-ape 的 booter-raw-elf 生产者,(7) tools-default-regression 的 old-checker 生产者,(8) 选择器复用(候选与闭包和容器身份入键),(9) 提交门 list-cap(大输出按上限具名拒绝),(10) 夹具修复 c698ef0091(R39 路径 B)。顺序原则:先 P0 与夹具修复(它们在预跑链路上),再材料生产者与门上限,最后选择器复用;每笔单独 tools-only 提交,`--keyed` 只带自己行。材料宏检查(新增 -tool 材料前的 __FILE__ 八字节扫描)依赖第一车已合的 pinscan(`cx/text/pinscan.cx`,0.0.124 件);扫描件本身不进 107 追加集。依据:usable-list-draft 的追加集归属、R39 路径 B、file-macro-scan 的依赖说明。证据入口:research/ccogweco/usable-list-draft.md:54-66、research/ccogweco/file-macro-scan.md、research/ccogweco/unlanded-inventory.md。回滚:从追加集撤回对应补丁并重排 107 判据;材料宏检查若 pinscan 未在树上则不得启用。
- 0.1.0 封版读法(R37 加 R38):七条逐条按 R37 读法判定——条 1(六目标不动点连续 3 版)与条 3(零已知 P0)不可豁免;条 2 的 FAIL 须为 0,UNKNOWN 只按已登记无生产者项且不超 2;条 4 只计声称类 kind(基数 10:U05、U06、U08、U09、U11、U30、U33、U21、E01、E02),缺口与债 15 行转已知缺口不删行,三值最好 3、预期 3 到 4、最差 8;条 5 的 B2 顺延并明写未满足;条 6 按 R6 重述只取封版车"起链到 READY_TO_EXECUTE"为第三样本;条 7 清单 30 项先冻结再绑 strict 批次行。未测一律 UNMEASURED,不得写成满足。依据:R37/R38 原文与三值重算。证据入口:archive/research-2026-11-25/2026-11-24/R37-seal.md、archive/research-2026-11-25/2026-11-24/R38-ruling.md、research/ccogweco/notes-0.1.0-draft.md、research/ccogweco/seal-0.1.0-evidence-map.md。回滚:恢复七条字面读法与全表计数,删除本条。
- 批次余量缺口 check-split-parents:最近实测 54947 ms(rc=0 PASS,紧贴 55000 ms 限,余 53 ms);0.0.105 为 38490 ms,差 16457 ms,冷热状态与根因未定(run-cache 命中、负载或 W0080 内核编译工作量三者之一);登记 0.1.x 余量包,处置为拆 --cases 或材料相,不得放宽时限。依据:margin-audit.tsv 第 125 行与两版实测。证据入口:research/agy-h/margin-audit.tsv、research/agy-h/margin-packages.md。回滚:删本条,该批回普通超限观察。
- 预跑 DVQUALIFY_NATIVE_RUN_BATCH 表述更正:该开关是 opt-in,默认 0(tools/dvqualify.cx:624 `use_native=!strcmp(sy_env_or("DVQUALIFY_NATIVE_RUN_BATCH","0"),"1")`;dvledger.cx:167 同口径),预跑与日常按 sh 运行器记录;此前把 native 运行器写成预跑默认的表述作废,不得据此推断预跑用了原生执行体。依据:两处源码行与 dvqualify.cx 文件头说明。证据入口:tools/dvqualify.cx:4、:624;tools/dvledger.cx:167。回滚:恢复旧表述并在证据页附源码行。
- doc-links 与 no-home-path 零新增(私有核对原行):私有树(HEAD git archive + git init)跑 `sh ./unisacc.com tools/doc-links-gen.cx -- --check` 得 `DOC_LINKS_GEN rows=5248 inline=692 tick=4556 bare=0 broken=81 baseline=80 result=FAIL`,具名行只有 `NEW_BROKEN plans/v0.0.126.md cx/compiler/fwdstub.cx:14-16`;把该行内链接改反引号并 `--write` 后得 `DOC_LINKS_GEN rows=5248 inline=691 tick=4557 bare=0 broken=80 baseline=80 anchor_miss=1 orphans=111 result=OK`,即断链回到基线 80,本决定补丁零新增;no-home-path 侧仅剩既有 `archive/research-2026-11-25/omp-sonnet/handoff.md has 1 lines`,基线既有项不计,本补丁零新增。依据:私有树两次 --check 原行与 ratchets 输出。证据入口:research/ccogweco/doclinks-81-investigation.md(含原行与 no-home-path 附录)。回滚:删本条;doc-links 修复与 --write 的生成表归 0.0.126 计划维护者。
- 0.0.106 公开与重公开(2026-11-25):01:42:35 重新公开;首次 01:36:31 因读回 smoke 冷编超 45 秒自动回滚 Draft,6 分钟后重公开,两次都记录。依据:同候选 APE 5f83ac93 与 SOURCES_SID c6eeca9f 未变。证据入口:release/evidence/0.0.106/INDEX.tsv;Y1 32f26cd024、Y2 9d08f985fa。回滚:按 release/rollback.txt 转回 Draft 并读回。
- CI 两次 run 记录口径:45442318936 败于 windows-11-arm fetch 对新建草稿 gh release download 的瞬态 404(被软失败吞掉,slice 缺载荷);同候选重派 45446325698 全绿 42/0,两次 run 都留,不以后一次覆盖前一次。依据:同候选身份不变。证据入口:release/evidence/0.0.106/ci-45442318936 与 ci-45446325698;notes-0.0.106 已知缺口 (192)。回滚:若重派 run 被判无效,重派第三次并保留全部原行。
- 读回事故处置:冷缓存下读回 smoke 冷编超 45 秒,处置为读回前精确 argv 预热,实测 1452 ms;首次公开按失败自动回滚,不改时限、不跳过读回。依据:预热为同 argv 重复执行,不改变被验证字节。证据入口:release/evidence/0.0.106 的 warm 收据与 INDEX 的 warm/readback 行。回滚:删预热步骤回到冷编路径。
- 107 追加集再增三条:读回前自动预热(精确 argv,失败具名);warm_note 收据写 release/evidence/<V>/warm-<key>.tsv 且先 scrub 家路径;rollback 动作写 INDEX 行。依据:本车读回回滚暴露三处无自动记录;三件 tools-only,不触 SID。证据入口:INDEX 的 warm/readback/rollback 行。回滚:任一被证伪即从追加集撤回并在 107 说明披露。
- 准入条件 4/5/6 截至 106 公开的诚实状态:条 4 未满足(声称类基数 10,预期 3 到 4,E02/U05/U33 保留);条 5 未满足并按 R37 豁免(B1 部分入 107,B2 顺延 0.0.109);条 6 原文不满足(0.0.106 链起 21:31:05 到公开 01:42:35 共 251.5 分钟,链止到公开 223.7 分钟;R6 三样本仍缺)。依据:build/state-unverified.tsv、R37 第 5/6 条、chain-to-public.tsv。证据入口:research/ccogweco/post-106-ledger.md、notes-0.1.0-draft.md。回滚:删本条并回退到 106 公开前记法。
- 0.0.106 账本写入位置:本节作为 106 账本随公开落记忆宫殿(由 cc 落),107 与 0.1.0 各起新节不追改;条件 6 分钟数在各自公开后补。依据:记忆宫殿只记决定与证据入口。证据入口:research/ccogweco/post-106-ledger.md。回滚:删本节并回退条件状态记法。

## 决定:24 小时裁定落地(2026-11-25 02:23,fable 裁定,cc 执行)

## 决定

107 公开后**直接起 0.1.0 零改动封版车**(只收封版证明与版本号收口,不进新功能、材料补丁栈或工具线);
0.0.108 + 126、0.0.109、0.0.125 全部顺延到 0.1.x,按 0.1.1 到 0.1.4 分批,每车只装一类大改;

- 0.1.1 诊断(108 原范围 + 103 判据若就绪);
- 0.1.2 前端 B2(109 原范围:k2-caps 17 上限、生成器、cap-probe);
- 0.1.3 进程生命周期(125 原范围:reaper/proc-lifecycle、变异门组杀核实、U33);
- 0.1.4 转发桩余项(126 余项)。
工具线与测试材料(顺延十项、预跑提速)不占版本车,单独落 main 或随 0.1.x 分步。

## 依据

- 四车合计 **9.7 小时**;0.0.106 单车实测 **251 分钟**(链起 21:31:05 到公开 01:42:35);
- 历史公开间隔均值 **6.8 小时**;
- 自 17:50 起 24 小时窗口内,只够一车余量 **7 小时以上**,最迟起链 **13:40**;
- 107 基准公开 04:50-05:10、保守 06:13-06:32;0.1.0 基准约 08:00、保守约 10:30(均在 17:50 前);
- 106 预跑 122.6 分钟里约 **70 分钟**是红停、重试与等人,净跑约 **52 分钟**(分位数推断);
- 预跑提速 S1 到 S4 与结构点**不改源码**即可先用的部分先做(S1 值守、S2 预热),不改调度核心;
- 准入第 6 条(连续三版起链到公开 <=120 分钟)**字面不可能**在本窗口成立,按原文如实记未满足;
  第 5 条(前端三期)按 R37 豁免,B2 顺延并写未满足。

## 证据入口

- fable 24 小时裁定报告摘要(要点由 cc 转述:四车 9.7 小时、106 单车 251 分钟、公开间隔均值 6.8 小时、
  17:50 前余量 7 小时以上、最迟起链 13:40;107 基准 04:50-05:10 / 保守 06:13-06:32;
  0.1.0 基准约 08:00 / 保守约 10:30;106 预跑 70 分钟红停重试等人、净跑 52 分钟为分位数推断);
- research/agy-h/car2-timeline-v2.md(106 实测分相与 107 估计);
- research/ccogweco/roadmap-0.1.x.md(0.1.x 批次与顺延十项);
- research/ccogweco/prerun-speedup-0.1.x.md(S1-S4 与结构点);
- archive/research-2026-11-25/2026-11-24/R37-seal.md(第 5/6 条读法);
- 0.0.109 的 plans/v0.0.109.md 与 0.0.125 的 plans/v0.0.125.md(原始范围)。
报告原页落库时补路径;在补上前,证据入口以本页列出的摘要为准。

## 证伪点(触发即改档)

- 107 预跑在**第 30 分钟**完成窗数**小于 110**:按保守档执行(公开预计 06:13-06:32,0.1.0 预算按 10:30 上限),
  并重新核 13:40 最迟起链线;不得用基准档继续排。
- 若 107 链止到 READY 超过 45 分钟硬线,或 CI 需两次以上重派:同样改保守档。

## 回滚

- 若 107 公开后、0.1.0 起链前发现"零变化"断言不成立(候选 APE 与 107 除版本槽注入外还有任何字节差异,
  或 SOURCES_SID 不同):**取消 0.1.0 封版车**,不重试、不改口径;按 R37 重排版本序,当趟用 0.0.127 或
  按当时下一车处理,并把顺延项重新排入。
- 若 0.1.0 起链后失败:保留链目录与 exit.tsv 原行,按 R37 记失败节点,不改写 0.1.0 的 strict 表。
- 本决定块回滚:删本条并恢复 108/109/125 原版本序;已公开的 107 证据不动。

## 账本:0.0.107 起链(2026-11-25)

- 链起 02:49:02 +0800(epoch 1795546142):REV=28b6a518fe9fac6b117f31adeb25e1c8ab51701f(车树 F,v0.0.107 已推 origin);guard=1984485 pid=1984486 pgid=1984486;deadline_s=5400 hard_s=6000;输出 ~/.cache/unisacc/chains/bg-b1/28b6a518fe9fac6b117f31adeb25e1c8ab51701f。
- 冻结:main 上 0.0.107-FREEZE 提交 6e4f8d9a9c(base=1fa88fbf46,deadline 07:30);车树 F 重写冻结行续期到 08:00(门 23 要求最后编号提交 e2f0539e2a 为冻结文件提交祖先)。
- 起链前红两处:门 20(导出已核 identity helper,sha 00aa0909a219)与门 23(上条);首次 bg-candidate start 被外层 58 秒限在严格 preflight 阶段中断(链未起),重起放后台限 300 秒;chain-ready 全量第二轮 READY yes failed=none。
- 首窗预期 rc=124 各一次,按 S1 在 2 分钟内续跑;第 30 分钟完成窗数小于 110 改保守档。

## 决定:0.0.xx 系列停工,旧分支改名素材(2026-11-25 13:16,cc 执行)
- 0.0.xx 版本系列已停,不再接受提交;旧未合并分支 v0.0.102、103、104、108、123、125 已改名为 src/v0.0.NN(哈希不变),只作重放素材来源;新开发只在 v0.1.x 工作树(.claude/worktrees/v0.1.N,分支 v0.1.N)。0.0.108 的 9 个提交由 0.1.1 重放(见 plans/v0.1.1 与 research/agyneon/v011-impl-prep.md);0.0.109 前端 B2 补丁对应 0.1.2,0.0.125 对应 0.1.3,0.0.126 对应 0.1.4。分支去向与状态台账见 research/ccogweco/branch-ledger.md。回滚:git branch -m src/v0.0.NN v0.0.NN。

## 决定:0.1.0 公开与 v0.1.x 开发流改进(2026-11-25 12:05,cc 执行,fable 方案采纳)

- 事实:v0.1.0 于 2026-11-25 11:55:05 +0800 公开(Latest),APE b7cae17b1ae557a4907247e3168db56925504819f98d15f1b75a2da2de8fa97f,SOURCES_SID 37ff46cd(与 0.0.107 相同);Y1=7375d05e33、Y2=7dc318425d、release-post 三笔 177b6326f5/c3e39dc4cf/d1e0616c31;T1=08:59:43 到公开 175.4 分钟,准入线 T1+120 与 T2+45 未满足(如实披露)。0.1.0 不再是零 B 提交纯封存车:B-t 四文件(三 tools 夹具、一 tests/env-semantics.tsv 到期表),零触 SID 与产品闭包,证明 research/cdx/path-b-proof-010.md。
- 决定(依据 fable 两页 v01x-flow-proposal.md 与 v011-v012-task-tree.md):(1)0.1.1 之前先开预备窗口 P0(目标 150 分钟,只落 main,出口=以 0.1.1 版号离线彩排全绿);(2)四趟产品车不合并:0.1.1 诊断定位(收窄 c15/c16/c17/-o 警告加 diag-contract 基线下调,目标 130 分钟)、0.1.2 前端 B2(120 分钟线起)、0.1.3 进程生命周期、0.1.4 转发桩容量;(3)入车条件=P0 出口彩排全绿;(4)硬停手线:同车第 2 次产品红、尾段(预跑止到 X)超 20 分钟;120/130 分钟为目标线,超线记未达标并复盘,不单独停车;(5)车前 60 分钟门工具路径零变化(静默窗);(6)人工补救计数:0.1.0=5,0.1.1 目标不超 1,0.1.2 目标 0。
- 依据与证据:0.1.0 起车缺陷(worktree 与 release-exec 不认三段版本、version-seq 读回按行序、batch-domains 被 precommit 跳过 query-only、cxtrunc 与头模式误登记)、预跑三次人工补救与四次工具红停(W0000 冷首窗、keep-going 漏传、W0212 env-semantics 到期日、W0217 xunit-fnptr 版本行硬编码,W0088/W0111 夹具缺 version-seq)见 research/agyneon/v010-car-structural-lessons.md、research/agy-h/chain-to-public-010.tsv、research/ccogweco/prd-decision-block-fable-flow.md。
- 回滚:P0 失败则按旧流程发 0.1.1 并复盘;本决定块可由下一次决定块取代。

## 账本:0.1.0 起链(2026-11-25)

- 链起 08:59:43 +0800(epoch 1795568383):REV=a11d28fc0a61939ee71345d732a7dcf205b261f1(车树 F,v0.1.0 零改动封板车,SOURCES_SID=37ff46cd02171c8ad2d3ba04dd084c296f5290a992077f8935e8b853e67d3238);guard=570322 pid=570323 pgid=570323;deadline_s=9000 hard_s=9600;输出 ~/.cache/unisacc/chains/bg-b1/a11d28fc0a61939ee71345d732a7dcf205b261f1。
- 冻结与前置:main 上 0.1.0-FREEZE 提交 c26d9f47e8(base=eb57262f30,deadline 12:00 epoch 1795579200),车树 F=a11d28fc0a;ompgpt 三项封板硬断言(SOURCES_SID、CLOSURE、FREEZE_METADATA added=1 deleted=0)全 PASS;env -i 下 chain-ready --version 0.1.0 全 23 门 PASS(READY yes failed=none 133666)。
- 链止 09:26:30 +0800(epoch 1795569990,耗时 26.8m = 1607s):EXIT rc=0 why=exited;APE=b7cae17b1ae557a4907247e3168db56925504819f98d15f1b75a2da2de8fa97f(8991230 B,unisacc 0.1.0);fixedpoint 四同 objects=same(102) kernels=same(2) slices=same(6) product=same package=c4120564b66c2dbc540644c98fce10ee0665cc996197de5b4c72aebdb15f8a24,六目标 g2/g3 6/6 SAME(FIXEDPOINT_EVIDENCE PASS compare_attempt=245)。
- 预跑起 09:29:39 +0800(epoch 1795570179,旧 pid=817099 因 W0000=module-units-limits 冷 kit/材料连续两次 58041ms rc=124 于 09:32:30 PRERUN_STOP 退出);按 cc 放行将 W0000 改为 module-units-forward(module-units-limits 移入 W0244 normal 窗)并在同 frozen-root 与同布局 tree-6cve4rtr 手动预热 PASS(elapsed_ms=25332)后,于 09:37:31 +0800(epoch 1795570651,pid=872413)以 --resume 重起,W0000..W0021 全 PASS(rc=0);W0022(precommit-list-cap,设计性贴限批)两次 58037/58034ms rc=124 因未传 --keep-going 于 09:47:30 PRERUN_STOP,按 cc 放行保留原 443B strict-W0022.tsv、在 load1=1.43 同布局外层 timeout 75s 单跑产出 strict-W9922-precommit-list-cap.tsv(UNKNOWN ms=55587 transport=-1 assertions=UNMEASURED)与 done.W0022(fail-kept=1)后,于 09:52:02 +0800(epoch 1795571522,pid=964623)带 --keep-going --resume 从 W0023 续跑至 W0212;W0212(env-semantics)因 tests/env-semantics.tsv:22-29 八行 expires=0.0.110 被 0.1.0 版号跨过红停,写 done.W0212(fail-kept=1)于 10:44:04(epoch 1795574644,pid=1303501)从 W0213 续跑至 W0217;W0217(xunit-fnptr)因 tools/check-xunit-fnptr.cx:97 硬编码 tx_find("unisacc 0.0.",12) 红停(rc=2),经 cc 裁定写 done.W0217(fail-kept=1,保留 strict-W0217.tsv 原件)并并入四文件路径 B-t 补丁后,于 10:47:13 +0800(epoch 1795574833,pid=1335474)从 W0218 续跑余 27 窗并于 10:57:40 达 245/245 DONE(PRERUN_RUN status=DONE ran=27 skipped=218);S2 补跑 tsv-lint-root:旧件 strict-W9923-tsv-lint-root.tsv(UNKNOWN ms=55557 load1=1.68,因外层 75s 超时截断于次轮重试无 SUMMARY 行,误清后从 cat 日志还原 restored-from-report,sha256=58683c81163a8a7c18eddb314b23f34b7b8cf67e533adc05e87b6a5811c4e1ab)由同布局热态单跑 strict-W9923b-tsv-lint-root.tsv(PASS ms=11733 load1=1.83 assertions=2,sha256=d99db704c32f0dca8b5f2bf6c78763cc06f47a622b9885f58f4aae407467b0a9)折叠为 SUPERSEDED;输出 ~/.cache/unisacc/prerun/v0.1.0-a11d28fc0a。

# v0.1.3 记忆宫殿留痕终稿(已公开;未入仓)

状态:终稿(2026-11-26),供 `prd_mempalace.md` 合并;发布前版本另存 `.draft`。数字只引已有文件原行并写来源;命令一律 `cx`(受控发布链起跑例外)。

## 1. 决定与依据

- 冻结:`0.1.3-FREEZE` 的 base 提交为 `37f197cbfc6a510c9cd787327ce4647bedab95c4`(来源:`build/freeze.tsv` 原行);起链 REV 与候选绑定为 `f485ec9a2bf02a7204b0d40b4b048ebd8a652a7a`(来源:整链账本 `chain-ledger-0.1.3.txt` 的 `chain#5` 行)。
- 候选身份:tag `v0.1.3`,commit `f485ec9a2b…`,APE `unisacc.com` sha256 `eb6741afe2d54c4a7e867b229aea903f1650a797508623869f858c0b197d3575`(来源:`release/candidate.tsv` 0.1.3 行;字节 `9232894` 来源:`notes-evidence.txt` 的 `APE_SUMMARY`)。
- C0 系列修复(来源:版本树 `git log` 原行):
  - `d07d9640c` C0c:check-unit-frontcache 隔离并预热 CXIF 暂存,修正收割后读 deadline;
  - `b5968b6c6` C0d:不再断言 CXIF 预备命令退出码(夹具可合法编译失败);x/c/d 腿与 2 秒等待不变;
  - `a0c4fc828` C0e:冻结期限 +12h,base 不变;
  - `37f197cbf` C0f:真实 APE 的外部 nn 默认旁置(启动头配对 `UNISA_CONTAINER` 与 `UNISACC_APE_SELF`,父目录 fd 校验属主/权限/链接,保留 chain-forced 与 `--models` 禁用);
  - `f485ec9a2b` C0f-freeze:重挂 `0.1.3-FREEZE`(base C0f,期限 +12h)。
- 预跑补跑 W9901-W9906(来源:`release/evidence/0.1.3/windows/` 同名 strict 文件原行):
  - W9901 tsv-lint-root PASS 11187 ms load1=4.42;
  - W9902 bpidecl PASS 2469 ms load1=4.18;
  - W9903 lane-key-probes PASS 20755 ms load1=3.92;
  - W9904 slow-split 两行:UNKNOWN 55512 ms load1=3.93,FAIL 55635 ms load1=4.14 retries=1;
  - W9905 slow-split PASS 21690 ms load1=2.53;
  - W9906 slow-split PASS 20897 ms load1=5.76。
  - 合并口径:549 个唯一批折叠后 `549/549 PASS`,原行保留(来源:`INDEX.tsv` 的 `strict` 行原行 `PRERUN_MERGE … SUPERSEDED=1`)。
- `windows/` 布局:298 个窗口文件(含 W9901-W9906)由证据根移入 `windows/`;原因:release-exec 的 Y1/Y2 会把证据根顶层全部普通文件拼进单条 commit-own 调用,上限 64 路径(来源:`INDEX.tsv` 的 `layout-change` 行;搬移提交见 `0c28bce04` 等 15 个 part)。逐文件 sha 与分组摘要 sha 不变。

## 2. 证据入口

- 索引:`release/evidence/0.1.3/INDEX.tsv`,2026-11-26 16:37 +0800(发布 Y2 后)现算 sha256 `8eefbb8a55221fe7316dc6c924d775d5ddee3fef8d8448ff029d1effabf311ac`;cdx r4 签收快照 `9e75d208…` 与中间态 `02989d6d…` 为更早时点,以本次现算为终态。
- 聚合算法:`release/evidence/0.1.3/AGGREGATION.md`,sha256 `31cd9b6cb4dd7acddc2a0996fadf89efcef995940bcb3ac94b6f028ca235f039`。
- 已知缺口:`release/evidence/0.1.3/known-gaps.md`,sha256 `992753336cce77cf2f8fc05adeeef12ab374916d6bf1e782b0aeaaa8f363928f`。
- 清单:`file-manifest.tsv`(归档时 944 条数据行 = 树内 946 文件减 INDEX 与 file-manifest;物理行数含表头为 945);`evidence-rows.tsv` 在 release-exec Step 4/7 会同步改写,清单记归档提交时点原行(来源:`AGGREGATION.md` 收录范围段)。

## 3. 发布终态事实(2026-11-26)

- 候选 X:`427a53281`;Y1:`0a6809420`;Y2:`6f9780fe9`;post-publish:`bc952cf3d`;thaw:`88e7e3210`;主检出合入:`24e7f4d6a`。
- 公开读回:发布资产 `ape_sha256` 读回与 `eb6741afe2d54c4a7e867b229aea903f1650a797508623869f858c0b197d3575` 一致;release `isDraft=false`,标记 `Latest`。
- 装根:根已装 `0.1.3`(`cx --version` 输出 `unisacc 0.1.3`,`sha256sum unisacc.com` = `eb6741af…`);装前备份为 `0.1.2`(`e31c413a…`);`root-identity` 装前单份另存 `~/.cache/unisacc/prerun-backups/root-identity.pre-0.1.3.md`。
- CI 终态见第 4 节。

## 4. 阻塞与未验证(承接 known-gaps 第 5 节)

- C3 S3 59 格(59A/59B、M1-M6)未完成;S1 `syp_reap` 的 `deadline_at_ms` 字段复用留 v0.1.4。
- `prerun-drive --rebase-rev --plan`(P01-P33)未落。
- devsvc 提交互斥 v5.3 余项(30 秒单片归一、b 片与真实 commit 证据)仍被复核阻断。
- CXIF 读探测续龄:本版以测试补偿绕过,根治留 v0.1.4。
- 外部 NN 的 E2E 全矩阵 `UNMEASURED`;Darwin/Windows 与其它交叉目标的外部权重旁置 `UNCOVERED`(来源:`known-gaps.md` 第 1 节)。该 NN 矩阵通过也不销 C0f 遗留:`S4`(候选批次须显式 `UNISACC_NN_MODE=builtin`)与 `N2`(普通资格 APE 自容器环境与裸切片默认旁置两套信任规则)仍未验证,单列于 v0.1.4。
- CI:首轮 run `45779825447` 为 `FAIL`,其中 `osx/x86_64` 单格报 55 秒子进程超时(超时数绑定该首轮 job 原行);同候选重派 run `45781798885`(X 提交 `427a532816fde450cd7f87da90df66e77ef3ea16`)最终全六格 `success`,裁决 `QUALIFIED`;资格只采同一 attempt 的最终六格。
- `osx/x86_64` 贴限根因(agy-flow 取证,来源:`research/agy-flow/v014/osx-x64-cell-timing.md`):该格所用 osx 与 lnx-arm64 缓存全关,netpack 闭包冷编膨胀 `+32%` 到 `+39%`;v0.1.4 根治方向为 `STREAM_PREPARE` 一次 `-o` 编译复用,不放宽 55 秒限。
- tape:16 档 `94.037` 倍、64 档 `114.316` 倍为旧逐步入口证据,102 未完成,15 倍门未过;M1X 未启动(来源:`known-gaps.md` 第 4 节)。

## 5. 事故:缓存清理定时器误删

- 事实:2026-11-26 `13:00:11` 前后,`chains/bg-b1.bak-pre-C0c` 与两个 `prerun/v0.1.3-*.superseded-*` 目录消失,含三份红停原件 `evidence-red-*` 与 `bfdd1448b`、`d07d9640c` 两条旧链。
- 归因:用户 journal 显示 `system-cleanup.service` 于 `13:00:03` 到 `13:00:13` 运行;`13:00:11` 报 `/tmp and 2h caches cleaned` 与 `unisacc stale artifacts cleaned`;unit 的 `ExecStart` 指向 moltbaby 脚本,脚本对 `~/.cache/unisacc/{prerun,chains/snap,chains,顶层}` 与 `~/.claude/jobs/*/tmp` 的子目录按 120 分钟 atime/mtime 规则 `rm -rf`(排除 `.*`/`*.lock`/`*.sock`)。journal 无逐目录原行,归因为据目录 mtime 与源码规则的推定。
- 处置与终态:2026-11-26 15:48 曾加 `prerun/`、`chains/` 父级与 `chains/snap/` 钉住标记;发布后 `16:35` 已撤除,三处现均不存在;keepalive 已停(无 `keepalive-0.1.3` 进程)。保留的钉住标记(当前实数五枚,均 0 字节):`release-v0.1.3-f485ec9a2b/.no-purge`、`evidence-staging-0.1.3/.no-purge`、`evidence-vault-0.1.3/.no-purge`、`prerun-backups/.no-purge`、本会话 job 目录下 tmp 的 `.no-purge`;父级 `prerun/`、`chains/`、`chains/snap/` 三枚已于 16:35 撤除。moltbaby 侧提交 `02837a44e`(提交说明为尊重 `.no-purge`、豁免 `evidence-*` 并记录清理路径)。顶层 `~/.cache/unisacc/.no-purge` 不存在,不声称已加。
- 证据:缺件记录 `incident-red/ABSENT.md`;重建引用 `reconstruct-chain-ledger-0.1.3*.txt` 与 `reconstruct-prerun4-supervise.out`,均标非原件;保护库 `~/.cache/unisacc/evidence-vault-0.1.3/`。

## 6. 回滚办法

- 证据(仅限尚未公开且只撤己方提交):按提交逆序撤销 `e1771a6e9`(八步 evidence-rows 与 windows 路径前缀)及 `windows/` 搬移的 15 个 part 提交;恢复 `release/evidence/0.1.3/` 到上一固定提交。已公开后不删原件,只追加更正记录并保留撤回说明。
- 候选:撤销 `427a53281`(candidate 表 X 行)即回到未登记状态;不触已生成资产。
- 装根:工作目录为仓根,先 `cx tools/install-ape.cx -- . --rollback` 看 `ROLLBACK_DRYRUN`,再 `cx tools/install-ape.cx -- . --rollback --write` 恢复装前备份(按清单逐文件核验后复制,见 `tools/install-ape.cx` 用法);装前另存一份根身份文件(`root-identity` 单份留档);回滚后实跑 `cx --version` 与 `sha256sum unisacc.com` 读回版本与根 APE SHA,核等于装前值。版本树当前无 `check-root-current` 工具,不以不存在的入口作为回滚步骤。
- 缓存钉住:逐枚撤销上述五枚 `.no-purge` 即可恢复清理行为(`release-v0.1.3-f485ec9a2b`、`evidence-staging-0.1.3`、`evidence-vault-0.1.3`、`prerun-backups`、本会话 tmp);该动作只影响缓存保留,不影响产品与证据。
- 不涉及 `SOURCES_SID` 的回滚;任何证据撤销都不得改写已公开资产或已发送的发布说明。

## 决定:v0.1.5 发布终态与 v0.1.6 起步(2026-11-27)

### 当前版本

- 0.1.5 已公开:`isDraft=false`、`Latest`、tag `v0.1.5` 指向 `ba12b08dfddefea5a5e9241f095ede4e7f83473d`;发布 APE `sha=824e3f05f72a485293bf0c33ea877c3f905ca3ef7c31a447473931f6ffa1fb1c`(`9384540`B);CI run `45902924617` 六目标通过;共享检出根已装 0.1.5;`origin/main=f69ad26fc073458de5d064957d3efab3f26add3e`。
- 整体裁决 `NOT_QUALIFIED`:全套 s9 未跑;严格证据末行胜合并 `593 通过 / 0 失败 / 0 未覆盖 / 7 条被取代`;G2/G3 `UNMEASURED`。

### 发布步骤与时间(INDEX.tsv 原行)

- 06:08:58 `exec-1-post-chain-merge WAIVED`(`bfdb8c447`;REV..HEAD 只触 release 证据、说明与冻结域外 5 个 tools 文件);06:09:13 X=`2a06f6718`;06:09:22 `exec-2-draft-readback PASS`;06:09:31 CI 派发 `45902924617`;06:14:21 CI PASS 六目标;06:14:30 Y1=`b3e358eb2`;06:14:38 publish;06:14:53 `exec-6-public-readback PASS`;06:14:54/06:15:01 Y2=`ee829606c`;06:14:57 版本序标已公开;post=`5558ff432`、`6f4ac7b4f`。
- 公开体:cdx 读回后定为五条被哈希实体加 `SHA256SUMS`,共六个上传资产;`public-readback.txt` 记 metadata/tag/download/asset-sha/checksums 全 PASS,`C47_1_READBACK PASS`。
- 回滚入口:`release/rollback.txt`(draft、Latest 回 0.1.3、Y2/Y1/X 前向 revert、`install-ape --rollback`);m53 只清 `UNISACC_M53_S1` 回原生,源码撤回未经验证。

### 证据与工具修复

- 三次严格合并保留:`v0.1.5-strict-20261127.tsv` 585/7/1、`…b.tsv` 591/2/0、`…c.tsv` 593/0/0(`SUPERSEDED=7`)。
- 首轮 7 失败与 1 未覆盖的定因:缺 `UNISACC_BOOTER_CHAIN_OUT`;`TMPDIR` 在仓内造成外层仓穿透;继承 `UNISACC_*` 环境;`check-ln-noderef` 真实缺陷修于 `bd023bab4`;`retcheck` 真实基线违例修于 `1c23d0393` 且未抬基线。
- 重跑 X9019/X9020 用同一 APE,但源树在更新的 main HEAD `bd023bab4`,冻结提交为 `ba12b08df`;源不同、APE 字节相同,如实保留。
- 预跑 3 lanes,04:16 后 5 lanes,58 秒超时重试至通过;`tools-compile` 计划覆盖 179 个唯一 id,另有候选 APE 全 202 分片单跑 201 通过、shard 137 在 load 36 超时后单独通过,不计入严格证据。

### v0.1.6 波浪(已落 1/2/4/5)

- Wave1 `d68f73d0f`:M1X-A slot-12 raw syscall lowering、负控与 S2/slot12-RO 验证计划。
- Wave2 `d07a46b7b`:解释器 A1(48 项 PIC 跳表、专用三寄存器 handler、预算在 rbp、单一 init_budget 槽);作者树 22/22 明文差分、6/6 变异,cdx 两轮复核;速度 `UNMEASURED`。
- Wave4 `e195a860d`:root-current `--expect-ref main|HEAD` 单源与严格解冻边界,cases 32-35;含 freeze/devsvc 缺口分析、门 24 OID 钉死设计与重冻结手册。
- Wave5 `f69ad26fc`:root-current `--controller-canary` 与 `--expect-ref` 合并,要求真实对象产物;check-root-current 42/0、check-tapesidecar 42/0。
- 开放:Wave3 gate19 安全子集(`b87930f83` 为暂存源;`c186529a9` 为 v0.1.7 6-affected,待复核后落);GV.compat 产品补丁(计划 `cd9cd0719`,实际补丁未落)。

### 队列与未验证

- `plans/v0.1.6-v017-queue.tsv` 15 行为当前优先级队列;Wave3 与 GV.compat 产品补丁仍开放。
- 未验证:全套 s9、G2/G3、配对改名中断恢复、product-map 读集、公开根编译 25 controller 文件的前置门尚待 Wave 落地后实测;未跑项一律 `UNMEASURED`。


## 决定:v0.1.6 预写(2026-11-27,待发布后并入)

### 当前版本

- 版本序槽 70,版本 0.1.6,主题 tape 载体公开分发;已公开(`isDraft=false`、`Latest`,2026-11-27 12:17:03)。tag/F=`c9275e7756acad801ff2f206b3aa6a891e8115a2`;APE `ed50199fd085be4da2fa692db8010057be6674f8bfc609b101e8d18daf23583f`;X=`2da9a60df`、Y1=`bd02939a5`、Y2=`e04bb033b`、post=`07395b519` 与 `507b2b608`;CI `45938522235` 六目标成功;公开读回 rc=0,`REMOTE_ASSET_COUNT=9`。九资产:`tapemode.so` `e08947f2...`、`tape-manifest.tsv` `098e3041...`、`m53.tapeslice` `9f9294c4...`,其余六件同 0.1.5 布局。共享根已升 0.1.6 且 `root-current` PASS(含 `--controller-canary`);严格证据 593/593,含 5 次重放。
- 再冻结事件:预跑在 `check-gen-all-check` 红停(确定性:`a1f3b8d00` 手工改了 k2-gen2 却没走生成器);修复 `84737d935`(`tools/facts-gen.cx` 加 37 行重生成头),重做 C0a/C0b 并推送;新正式链自 09:36 在 0.0.105 引导根上运行,随后全量预跑。旧预跑证据作废;机器保持安静,其他窗口只做 docs/静态/本树提交。

### 决定与依据

- **九资产 sidecar**:在 0.1.5 六资产(五件被校验文件加 `SHA256SUMS`)之上,公开侧只面向 `lnx/x86_64` 增加 GitHub 顶层文件 `tapemode.so`、`tape-manifest.tsv`、`m53.tapeslice`;安装目录对应 `tape/tapemode.so`、`tape/manifest.tsv`、`tape/m53.tapeslice`。八条清单,正式上传与回读未完成前不得称已分发。六资产仅作私下稿读回连续失败两次时的回退块。回滚:转草稿、恢复 v0.1.5 Latest、清本机 sidecar 并验证。
- **GV.compat 产品补丁**:`a1f3b8d00`(合入 `be9902be0`)把 `GV.compat.base` 收窄到函数指针区间 `[FPS_FIRST, SBB)`,新增 7 个多单元回归;已进入 v0.1.6 候选。公开 0.1.6 根 `root-current` PASS(含 `--controller-canary`);此前公开 0.1.5 根 canary 为 RED,该文案也覆盖临时目录等原因,不单独定因。
- **引导根换入与备份**:正式链前先把共享检出当前公开 0.1.5 根与 build record 完整备份,再换入 0.0.105 开发根(`934c7c0bc`)起链;`release-post Step 5 --root-upgrade` 前必须先还原 0.1.5 根,禁止把 0.0.105 写入备份目录;解冻 gate 24 只在共享根升级为正式 0.1.6 后运行。
- **env-semantics 到期推迟**:多条 `UNISA_*` 弃用名的 expires 从 0.1.6 推到 0.1.7(`tests/env-semantics.tsv` 对应行),到期前须改名或再明确推迟,不得静默越过。
- **干净环境规则**:正式链/预跑的 `env -i` 白名单 PATH 必须含 `$HOME/.local/bin`;缺它会让工具或包装器不可见。runner 还须显式提供材料变量、`TMPDIR` 在仓外、git 探测失败闭合、分片与登记表齐平、保留首轮失败原行。
- **冻结纪律**:v0.1.6 冻结期共享 main、v0.1.6 树与冻结路径只由集成者写;其他窗口只在自己树做 docs/静态/补丁。

### 证据入口

- 版本与冻结:`build/version-seq.tsv` 槽 70;`build/freeze.tsv`;`release/evidence/0.1.6/INDEX.tsv`;`release/evidence/0.1.6/public-readback.txt`(9 资产、tag、version)。
- 说明草稿:`plans/notes-0.1.6-final.patch`(本窗口提交 `29b2862590`);`release/notes-0.1.6.md` 待集成者落。
- GV.compat:分支 `a1f3b8d00`、合入 `be9902be0`;复核 `w-cdx/.work/review-v016-release-docs.md`。
- 引导根备份顺序:`w-agy-h/.work/freeze-arm-commands-015.md:181`、`freeze-runbook-v015.md:205`。
- env-semantics:`tests/env-semantics.tsv` 的 `UNISA_*` 到期行。
- 队列:`plans/v0.1.6-v017-queue.tsv`。

### 阻塞与 UNVERIFIED

- 发布链、CI、九资产上传/回读、装根、公开读回与解冻已有回执;整体资格措辞以 `release/decision.txt` 与公开说明为准,本段不另立。
- GV.compat 的 phase 4a 之外用例、嵌套类型/签名等价仍未关闭;M1/M2 私有 APE 仍为 LOCAL,不是 PASS。
- 全套 s9、G2/G3、配对改名中断恢复、product-map 读集仍未关闭。
- 六资产回退只在九资产私下读回连续失败两次时启用,当前不启用。

### 下一步

- v0.1.8(计划 `plans/v0.1.8.md`)与 0.1.6 后续队列见 `plans/v0.1.6-v017-queue.tsv` 及 `plans/version-roadmap.md`;M1 五步与 module-evolution 逐单元循环优先。

## 决定:迁移加速(2026-11-27,所有者指令 + fable 裁定)

- 撤回桥接条款里的单单元体积回退条款:尺寸只记录,从不作为准入或回退判据;已写入桥接条款的该句由本条取代。
- stay-native 不是终态,迁移继续;准入三元组:(1) 正确性 = golden 全枚举或字节对拍;(2) 速度 = `calls_per_run x 单次成本` 实测,启动预算 `<=2,000us`(含 0.91ms boot),热启动 `<=1.10x`;(3) 可维护性 = 源为表或生成器且带 `--check`。
- 顺序按成本升序:A m53(已完成)、driveargs、modelnumber、m25;B route、m20、m54(route 若按 token 则先做批入口);C 9 个纯核;D 冷 VM;E 有状态(tapelink 两回调原型、m16、m9、cxmod)。
- 宿主开销三层:共享 TapeRuntime(M1 已付)、薄宿主调用表(TapeUnitSpec ~32B + 16-24B stub)、按签名 shape 复用宿主桩;每单元 5.8-9KB 降至 200-600B(估计)。
- 版本:v0.1.12 薄宿主表+批 A;v0.1.13 批 B+拆分 2;v0.1.14 冷 VM+剩余拆分(tape wall-clock share 新判据在此进入,不追溯;stay-native 行须写 re-entry 条件与复测日期);v0.1.15 有状态 guest;v0.1.16 六目标+出口门(最早)。
- falsifiers:批 A 任一单元 `calls_per_run>200`;shape 扫描 `>16` 或 top 8 `<80%`;第三单元边际宿主成本 `>600B`;v0.1.11 私有候选热启动 `>1.10x` 则不进入 v0.1.12。
