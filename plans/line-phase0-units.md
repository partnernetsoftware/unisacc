# 产线化第 0 阶段派工清单(2026-11-20,主调度决定,文档写者落库)

上位:plans/devsvc.md "产线化(2026-11-20 决定)"(盘上合同 (2) 是唯一权威,所有单元引用同一列头常量);挂 plans/v0.0.87.md L 线。来源:fable 产线化报告第 9 节(~/.claude/jobs/2798e440/tmp/devsvc-pipeline.md)。

## 通用要求(每单元都适用)

- 工作目录:仓内 .claude/worktrees/<单元分支名>,根放 unisacc.com 与 unisacc.com.build.json 两个符号链接;新文件改动只在写域内;提交前确认 build/lane-inputs.tsv 对所有改动路径判定非 A。
- 验收命令前缀 `sh ./unisacc.com`,单条 55 秒内;检查末行 `failed=0`。
- 先红后绿:红在未实现时必然成立(在 origin/main 工具下跑验收命令即得红),绿在本单元补丁下得到;红绿两次输出都随回执交。
- 变异要求:对本补丁的产品代码(非检查本身)至少施加 3 个变异,自带检查逐个变红,回执行 `MUT killed=K total=N`,K 必须等于 N;检查不得重写被测函数的副本,必须调真实入口;有端到端断言的不得改成桩。无变异回执不开复核单。
- 列头与事件名一律引用 tools/cxlib/lineorder.cx 导出的常量(U1 先落该常量;U1 未落库前其它单元按合同页字面写并在 U1 落库后改为引用,检查里加一条"与常量逐字相等")。
- 新工具带 check-*.cx 并登记批次(U10 收尾时统一核);禁区:不改写域外,不删断言,不放宽时限,不提交他人路径,不往仓库 out/ 写。

## 文件名历史核对(2026-11-20,git log --all --oneline -- 路径,均 0 行且不存在)

plans/line-phase0-units.md、tools/line.cx、tools/art.cx、tools/winstate.cx、tools/cxlib/artstore.cx、tools/cxlib/lineorder.cx、tools/line-stations.tsv、tools/line-release.tsv、tools/check-lineorder.cx、tools/check-line-lease.cx、tools/check-winstate.cx、tools/check-art.cx、tools/check-line-tick.cx、tools/cxlib/stationev.cx、tools/check-stationev.cx。无占用,未换名。

## 单元表

### U1 工单字段与状态机(甲组首段)

- 写域:tools/cxlib/lineorder.cx、tools/check-lineorder.cx。
- 内容:合同页 (2) 的 contract.tsv、state.tsv 读写;状态迁移表全部合法迁移与非法拒绝(rc 2 不写盘);wo.state 事件;deps 与 in 就绪判定的纯函数;导出 `LINE_HDR_*`、`LINE_EV_*` 常量。
- 验收:`sh ./unisacc.com tools/check-lineorder.cx`。红:OPEN 直接到 DONE 被接受(未实现时无此拒绝,文件都不存在即红)。绿:12 条非法迁移负控全拒;deps 未 DONE 不得 READY;kill -9 写入进程后重读 state.tsv 与最后一条 wo.state 一致;末行 failed=0。
- 变异:去掉迁移表一行、把 attempt 加一删掉、把原子改名换成直写,三者各令检查变红。
- 估时 1.5 单位;依赖 U0;建议负责人 cdx。

### U2 租约(甲组次段)

- 写域:tools/line.cx(pull、done、handoff、status 子命令)、tools/check-line-lease.cx。
- 内容:line pull --win W(能力匹配、写域与已租工单不相交、独占资源可得;记 LEASED 与 task.ack;打印合同与 contract.sha);line done ID --sha 摘要;line handoff ID --note FILE(四段固定:已改路径、已跑验收与结果、未完成项、下一步命令);ack 超时出 task.unacked 并置 EXPIRED。
- 验收:`sh ./unisacc.com tools/check-line-lease.cx`。红五种:没 pull 就 done 被拒、摘要不符被拒、写域相交的第二次 pull 被拒、ack 超时出 task.unacked、写域外有改动 done 返回 rc 1(未实现时 tools/line.cx 不存在,五项全红)。绿:五项全过,且两进程并发 pull 同一单只一方得到;末行 failed=0。
- 变异:去掉写域相交判定、摘要比对改为恒真、去掉 flock,三者各令检查变红。
- 估时 2 单位;依赖 U1;建议负责人 cdx。

### U3 win state 与心跳(乙组)

- 写域:tools/winstate.cx、tools/check-winstate.cx。
- 内容:合同页 win/NAME.tsv 九列;state 四值,其它拒绝;waiting 无 blocker 拒绝;同状态重复只刷 heartbeat 不发事件;对账时心跳超 10 分钟记 window.state blocker=heartbeat-lost;status --tsv 的窗口行;时钟经与 $DEVTASK_NOW 同类的环境变量注入。
- 验收:`sh ./unisacc.com tools/check-winstate.cx`。红:非法 state 值被接受(未实现时工具不存在即红)。绿:四状态往返、重复上报不增事件、心跳超时出 heartbeat-lost、waiting 无 blocker 被拒;末行 failed=0。
- 变异:去掉 state 白名单、去掉重复上报去重、超时阈值判定反向,三者各令检查变红。
- 估时 1.5 单位;依赖 U0;建议负责人 agy-flow(T3 之后;钩子接线在 moltbaby 仓,归其维护者)。

### U4 产物库(丙组)

- 写域:tools/cxlib/artstore.cx、tools/art.cx、tools/check-art.cx。
- 内容:art put KIND --in ID... --file F、art get、art verify、art gc --plan;只开 candidate-ape;KEY 与 meta.tsv 按合同页;get 命中必重算 payload 摘要,不符即删条目报 miss;库内无结论类。
- 验收:`sh ./unisacc.com tools/check-art.cx`。红:改 payload 一字节后 get 仍命中(未实现时工具不存在即红)。绿:该情形 miss 且条目被删;换一项输入身份 miss;并发两次 put 同键只一份;被 release/evidence 引用的键 gc --plan 不列;末行 failed=0。
- 变异:get 跳过摘要重算、KEY 不含 recipe、gc 跳过 evidence 引用检查,三者各令检查变红。
- 估时 1.5 单位;依赖 U0;建议负责人 agyneon(S5a 之后)。键的完备性(构造工具身份含哪些路径)须由 S1 六类负控证明,本单元不替定。

### U5 工位表、产线定义表与 tick 只打印(丁组)

- 写域:tools/line-stations.tsv、tools/line-release.tsv、tools/line.cx 的 open、tick、next 子命令、tools/check-line-tick.cx。与 U2 同文件不同子命令:U5 在 U2 落库后基于其版本加子命令,若并行开工则 U5 先把 open、tick、next 放在 tools/line.cx 的独立段并在合入时由集成者按段合并(写域按子命令划分,不交叉改同一函数)。
- 内容:line open VERSION 按 line-release.tsv 开工单;line tick(flock tick.lock,拿不到立即退;auto=shadow 只打印并记事件);line next --print 打印 READY 机器工单的完整手工命令;工位 preflight 不过则工单进 BLOCKED 并点名缺项。
- 验收:`sh ./unisacc.com tools/check-line-tick.cx`(夹具仓里开一版产线)。红:输入未就绪的工位被列为可起(未实现时工具不存在即红)。绿:按 0.0.85 INDEX 里程碑逐步喂入身份,tick 打印的下一步序列等于 W5 到 W11 的顺序;并发两个 tick 只一个动作;preflight 不过进 BLOCKED 并点名;末行 failed=0。
- 变异:就绪判定忽略 deps、去掉 tick.lock、preflight 结果忽略,三者各令检查变红。
- 估时 2 单位;依赖 U1、U4 的盘上格式(不依赖其实现);建议负责人 opus 执行者(编排者验收工位表)。

### U6 持久作业记录(戊组)

- 写域:devsvc-core 的作业部分(演进评审第三项写域,agyneon 现行车道)及其 check;具体文件由实现者开工前用 devsvc 关系查询列出并写回本行。
- 内容:jobs/ID/meta 落 PGID、boot_id、截止、输入身份;JOB_LOST 三态(活、已结束有 rc、确认丢失);判丢必须 PGID 与 boot_id 都核不到且无 rc 文件。
- 验收:kill -9 客户端与服务各一次。红:wait 得 JOB_LOST 空输出(现状)。绿:得真实 rc 与尾部;自行 setsid 的孙进程超时后 pgrep 为空;末行 failed=0。
- 变异:去掉 boot_id 比对、rc 文件写入删掉、超时清理只杀主进程,三者各令检查变红。
- 估时 2 单位;依赖无;建议负责人 agyneon 或 opus。

### U7 列车发事件(己组)

- 写域:tools/cxlib/stationev.cx、tools/check-stationev.cx,以及 tools/bg-candidate.cx、tools/prerun-drive.cx、tools/release-flow.cx 各一处调用。
- 内容:bg-candidate 守护进程写 EXIT 行时、prerun-drive 每窗结束时、release-flow 各相结束时各发一条 station.done(有 DEVSVC_SOCK 才发,无则照旧)。
- 验收:`sh ./unisacc.com tools/check-stationev.cx`。红:跑完夹具链后事件流无 station 行(现状)。绿:每相一行且 input= 等于链身份;不设 DEVSVC_SOCK 时三工具输出逐字节不变;末行 failed=0。
- 变异:发事件时 input 字段写死、DEVSVC_SOCK 判定反向、漏发一相,三者各令检查变红。
- 估时 1 单位;依赖无;注意 tools/prerun-drive.cx 在共享检出有他人未提交改动(2026-11-20 git status 为 MM),开工前先与其作者对齐;建议负责人 opus。

### U8 起链前置补齐(庚组,已单独派出,本清单不重派)

- 写域:tools/chain-ready.cx 与其 check。
- 验收:红为复现 0.0.86 17:31 现场(链接换成真实文件)时 READY=yes;绿为 READY=no 并点名 unisacc.com.build.json。
- 估时 0.5 单位;建议负责人 opus 执行者(已派出,结果由其窗口回报)。

### U9 变异自检门(辛组,原型已完成,落库中,不要重做)

- 内容:给补丁与检查名,输出 MUT killed= total=。红样本 T1 第六版补丁(应报 3 个变异存活),绿样本为已知好的检查。
- 状态:原型已完成,由其作者落库;本清单只记位置,工具名以其落库提交为准。第 1 阶段接为质检口机器工位。

### U10 登记与收尾(串行,集成者)

- 写域:tests/manifests/ 各单元清单、prd.md 能力速查(keyed 提交)、pins 与派生表重算。
- 验收:`sh ./unisacc.com tools/ratchets.cx` 全过;`sh ./unisacc.com tests/run.cx -- --list` 的 metadata_valid=1;各单元批次经 register 登记且资格布局自测 PASS。
- 估时 0.5 单位;依赖 U1 到 U9 各自完成;负责人集成者 cc。

## 并行组与落库顺序

甲 U1 接 U2、乙 U3、丙 U4、丁 U5、戊 U6、己 U7、庚 U8、辛 U9,写域互不相交(U2 与 U5 同文件按子命令分段,见 U5),U0 落库后同时开。落库顺序建议:庚 -> 丙 -> 乙 -> 甲 -> 丁,戊己辛随完成随落;每个合入后真实提交一次并跑自己的检查,不攒成一次大合并。整链在途时不动 main 上 A 类输入。

## 合计

U1 到 U8 约 12 单位(U8 0.5 已派出);U9 不重计;U10 0.5。今天到明天现实可落约 9 单位(甲乙丙丁加庚),戊己跟进到 0.0.87 版内。
