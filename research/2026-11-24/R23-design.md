来源:会话临时目录 /home/devali/.claude/jobs/7f46e032/tmp/R23/design.md 拷贝于 2026-11-24
# R23 结构诊断与裁定:dev-product 未含输入改动、表改快车道、register PLANNED

日期 2026-11-24。只读取证加一次私有构建(产物在 ~/.claude/jobs/7f46e032/tmp/R23/fw3,未动仓内文件与在途进程)。

## 0. 最重要判断

来题的定因不成立。dev-product 认工作树:cx-dev reload 把工作树与已装修订的 base-tree 逐文件比对(tools/cx-dev.cx:606-625),对 exec/parse2/forward-result.tsv 的改动重建了 errorparse 与 warnparse 两张网。sha 相同是另一个缺陷:同一个 OUT 上两次调用重叠,后一次的 `cx-dev --build` 把已重建的 pkg.bin 重置回已装包,dev-product 随后把别人的 reload.rc 当成自己的并封印。

单次干净调用在 149 秒内给出含改动的候选(f4bf1e28,不等于 459f790e)。但这个候选跑 check-forward-count 仍然红(8193 未被拒)。所以真正未决的是 S0 修复本身或其生效路径,不是车道缺失。

## 1. 证据

(1) 修前 ~/../tmp/unisacc-session-fwpre(即 /tmp/unisacc-session-fwpre):reload.log `CXDEV_KEYS files=0`,nets_pkg=a6bd7c13(等于 pkg.installed),product=459f790e。正常基线。

(2) 修后第一次 /tmp/unisacc-session-fwpost(15:52-15:54):reload.log `files=1`、`CXDEV_RECIPE warnparse built index=34`、`errorparse built index=4`、`local_members=2`,nets_pkg=864521ea,product=f4bf1e28。改动进了候选。

(3) 修后第二次 /tmp/unisacc-session-fwpost2(16:02-16:04,即报告"sha 相同"的那次):
- reload.log 同样写 `built=2 pkg=864521ea`;
- 但 cxdev/state.tsv 是 `fp not-compared-yet`、`scope LOCAL-BASELINE`、`pkgsha a6bd7c13`,sha256(cxdev/pkg.bin)=a6bd7c13=pkg.installed;
- 时间戳:state/reload.pid 16:03:04.013(reload 已启动),cxdev/ape.id 与 origin.tsv 16:02:55.245(另一次 --build 开始),log/reload.log 16:03:44.985(reload 结束),cxdev/pkg.bin 16:03:45.021 与 state.tsv 16:03:45.069(--build 末尾的 no_diff reload 写回已装包,tools/cx-dev.cx:920),state/build.rc 16:03:45.081,state/reload.rc 16:03:45.169(第二次调用的 seal_step)。
- 结论:两次 dev-product 调用重叠在同一 OUT。第一次调用何以被重发未核(推断:外层 60 秒命令上限或代理重发)。

(4) 本次私有复现 ~/.claude/jobs/7f46e032/tmp/R23/fw3.log:分支 HEAD b7449bb131,单次 `--until-done`,build 51.4 s、reload 40.7 s、hdr 4.0 s、entry 56.6 s,wall_ms=149028,product=f4bf1e28,与 (2) 逐位相同。

(5) `tests/check-forward-count.cx` 对 /tmp/unisacc-session-fwpost/unisacc.com(f4bf1e28):n8191-ok、n8192-ok、n8-ok 过,n8193-rejected FAIL,`ok=3 failed=1`。对 fwpost2(459f790e)结果相同。

(6) reload.log 有一行 `CXDEV_RECIPE shared-e3 NOT_DEPLOYED`:受影响的配方 shared-e3(parse2 --locations,tools/cxlib/sourcepackage.cx:69)在 cx-dev 里不重建(tools/cx-dev.cx:296-301,名单取自 tools/p3-member-consume.cx:385 的 DEDUPE 行),而 scope 仍报 LOCAL,dev-product 的标准输出与 receipt.tsv 都不带这行。

## 2. 时间与信心耗在哪里,以及结构性原因

- 耗在"候选与修前相同"的误定因上,进而得出"只能等整链"的错误结论,并在分支上手写了一行 PLANNED 覆盖行(b7449bb131)。
- 结构性原因(同类问题会反复出现):
  (a) dev-product 的步骤状态只靠 OUT/state 下的文件,没有 OUT 级互斥;bg() 只按 runid 相同加 rc 文件存在就认结果(tools/dev-product.cx:110-111),不核该 rc 是否属于本次调用启动的进程。
  (b) build 步无条件把 pkg.bin 重置为已装包,而 reload 的完成判据只是"pkg.bin 的 sha 等于当时记下的 sha",没有"工作树有输入差 -> 包必须不同于已装包"这条不变量。
  (c) 候选相对已装修订的差异(改了几个文件、重建了哪些配方、哪些受影响却未部署)只落在后台步骤日志里,不进标准输出和收据。使用者只能拿最终 sha 猜。
  (d) "受影响但未重建"的配方(NOT_DEPLOYED、NOT_CLAIMED)不降级 scope,与 CXDEV_STALE 的"绝不静默"口径不一致。

## 3. 裁定 (1):dev-product 如何大声报

立场:不采用来题的最小修"构建修订与工作树的产品输入差非空则 REFUSE"。差非空正是快车道的正常用法,那样拒会把车道本身关掉。正确的不变量是"差非空而包未变则拒"。

切片 D1(dev-product 自身,改 tools/dev-product.cx 与 tools/check-dev-product-build.cx,在 main 上做,属工具):
- OUT 互斥:进入时 O_EXCL 建 OUT/state/lock(写 pid),存在且 pid 存活则 `dev-product: REFUSE out-busy pid=N`,rc 2;pid 已死则接管。--status 不拿锁。
- reload 步结束后读 cxdev/state.tsv 与 log/reload.log,打印一行并写进 receipt.tsv:
  `DEV_DELTA<TAB>changed=N<TAB>local_members=M<TAB>built=recipe,..<TAB>not_rebuilt=recipe:NOT_DEPLOYED,..<TAB>stale=K<TAB>base_stale=B<TAB>scope=..`
- 三条拒绝(均 rc 2,具名):
  - `REFUSE reload-not-compared`:state.tsv 的 fp 仍是 not-compared-yet;
  - `REFUSE input-change-not-in-candidate first=PATH`:changed>0 且 sha(cxdev/pkg.bin) 等于 sha(cxdev/pkg.installed) 且 base_stale=0;
  - `REFUSE package-changed-under-us`:seal 前 sha(cxdev/pkg.bin) 不等于 state.tsv 的 pkgsha。
- not_rebuilt 非空时不拒,但末行 scope 用 cx-dev 的 scope 且降为 LOCAL-PARTIAL,并打印 `DEV_WARN<TAB>affected-recipe-not-rebuilt<TAB>recipe=..`。
- 谁做:cc(dev-product 与 cx-dev 的原作者线,未核具体窗口,由主调度按记分表派)。
- 验收:`sh ./unisacc.com tools/check-dev-product-build.cx`,新增四例:两次并发调用后者得 out-busy;手工把 pkg.bin 换回 pkg.installed 后得 input-change-not-in-candidate;fp 未比对得 reload-not-compared;改一张 parse2 表得 DEV_DELTA changed=1 且 not_rebuilt 含 shared-e3。先红后绿:用 /tmp/unisacc-session-fwpost2 的状态目录副本作红样本。
- 收益:本次约 20 分钟以上的误判加一次错误升级到慢车道(推断)直接变成一行拒绝。代价:约 60 行加四例检查,半天。
- 风险:REFUSE 误伤"改动确实不影响任何网"的情形(例如只改注释行的表)。缓解:该情形 cx-dev 的 local_members 仍会大于 0(重建后 M 记录字节相同与否未核);若出现误伤,把该条降为 DEV_WARN 即回退。
- 回退:三条拒绝各自独立,删对应分支即可,DEV_DELTA 行保留。

切片 D2(cx-dev,改 tools/cx-dev.cx 与 tools/check-cx-dev.cx):state.tsv 增 `changed<TAB>N<TAB>first` 与 `not_rebuilt<TAB>list`,D1 改读 state 而不解析日志;受影响配方未重建时 scope 记 LOCAL-PARTIAL。D1 可先靠解析 reload.log 落地,D2 随后。

## 4. 裁定 (2):表改 -> 可测候选的快车道

立场:这类改动不必走慢车道,现成路径已满足"1 小时内、不触 SID/PC/GEN"。实测 149 秒(证据 4);reload 只跑 gen -> lint -> tbl -> net -> M 记录替换,不碰 context、PC、两代 GEN。不采用"dev-product --rev HEAD"(工作树已是输入,无此缺口),不新建相位。

切片 A1(a12-cycle 复用 cx-dev 的构造器,改 tools/a12-cycle.cx 与其 check):
- 现状:G=B/tools 须由 main 预建,分支上没有;而每个 cx-dev 包里已有 cache/ctor/KEY/{genp,tbl,net} 与 tools/{table-lint,net-deps}(实见 /tmp/unisacc-session-fwpost2/cxdev/cache/ctor/90d0714d22aae623)。
- 改法:加 `--ctor DIR`,gen/tbl/net 三步改从 DIR 取(genp 对应 gen);缺省仍是 B/tools;MISSING 文案改为指向 `cx-dev --build`。neg43 仍要求显式 --neg43,缺就 MISSING 具名,不默认跳过。
- 验收:`sh ./unisacc.com tools/a12-cycle.cx -- --ctor <cxdev>/cache/ctor/KEY --only sim --src <8193 夹具> --reject`,预期 `STEP sim 0`(sim 以 rc 1 加具名 error 拒)。这一步几十秒,且直接回答 S0 表改在 errorparse 网上是否生效。
- 代价:约 20 行。风险:genp 与 seed 的 gen 参数不完全等价(未核,cx-dev.cx:325 提到 genp 的分配器差异)。证伪:同一表两条路径产出的 errorparse.json 做字节比对,不同则本切片改为"a12-cycle 调 cx-dev 的 ensure_ctor"。

切片 A2(prd.md 第 667 行与第 404 行的入口文字):把"改表 -> a12-cycle -> affected"改成"改表 -> dev-product(看 DEV_DELTA)-> 探针;表语义定位用 a12-cycle sim"。随 D1 提交带上。

对 S0 的直接后果(需立即处理,不等上面切片):
- 修后候选已存在(f4bf1e28,两份),check-forward-count 仍红。分支提交 daae75b8e7 的说明写"Green: build on this commit"未被任何实跑支持,b7449bb131 的"red-green rides the 0.0.108 chain"前提也错。不应带着这个假设起整链。
- 未核的三个假设:(h1) `1-2` 观测区间或 CMPI 的结果编码与预期不符,修法本身无效;(h2) -run 这条路径的计数拒绝不在 errorparse 网里(例如走 shared-e3 对应的成员,或 host 侧另有同一上限);(h3) 夹具的 N 与 fw_count 递增时点差一。最便宜的区分:A1 的 sim --reject(定 h1/h3),再查 p3 包里 parse2 的阶段引用指向哪个成员(定 h2)。

## 5. 裁定 (3):register 加 PLANNED

事实:tests 侧早已支持 PLANNED(tools/cxlib/testplan.cx:32、96-98、122;tests/run.cx:25 记 NOT_RUN reason=PLANNED;现有 26 行批次、11 行覆盖是 PLANNED),缺的只是 register 的写入口。--no-run 写 IMPLEMENTED 是把"未实跑"记成已实现,与铁律相悖。

切片 P1(改 tools/register.cx:1416-1424 的选项剥离、1464-1465 的两行模板、1493-1495 与 1541 的收尾;tools/check-register.cx 加例):
- 新增 `--planned`:批次行第二列与覆盖行第三列都写 PLANNED,不跑试跑,末行 `REGISTER<TAB>id=..<TAB>status=REGISTERED_PLANNED`;与 --no-run、--runner 互斥。仍过 --list 的元数据校验(testplan 对 PLANNED 行放宽了平台,其余列照常)。
- 新增 `--promote ID`:要求两行现为 PLANNED,在锁内改为 IMPLEMENTED 与 IMPLEMENTED_SEPARATE_BATCH,然后走与默认登记相同的试跑,失败则改回 PLANNED(不删行),末行 status=PROMOTED 或 STILL_PLANNED。
- --no-run 保留原合同不动(已有调用者,check-register.cx:775-781)。
- 谁做:register 的现维护窗口(关键路径,补丁须过 unisacc-gatekeeper 复核)。
- 验收:`sh ./unisacc.com tools/check-register.cx`,新例预期:--planned 后私有表两行均含 `\tPLANNED\t`、无 BATCH_RUN;`tests/run.cx --list` 对该行出 `NOT_RUN ... reason=PLANNED`;--promote 对必败检查得 STILL_PLANNED 且行仍在;对必过检查得 PROMOTED。
- 风险:PLANNED 行堆积成永久占位。缓解:不在本切片加期限;由 tools/status.cx 列 PLANNED 计数(现 26 加 11)即可见。回退:删两个选项分支,表内已有 PLANNED 行不受影响。

## 6. 砍项与延后

- 砍:dev-product `--rev HEAD`、a12-cycle 自建 G 的预备节点、为表改新开相位。理由见第 4 节。
- 砍:把 forward-count 的红转绿寄托于 0.0.108 整链。先用现成候选定因。
- 延后:shared-e1、shared-e3 在 cx-dev 中可重建(需要先弄清包内阶段引用到成员的映射),在 h2 被证实前不做。

## 7. 我最可能错在哪里

- 重叠调用的定因来自文件时间戳,没有第二次调用的命令记录。证伪:查 ccogweco 会话记录里 16:02:20 与 16:02:55 附近的两条 dev-product 命令;若只有一条,则是 dev-product 或外层重试自身重入,D1 的锁仍然对症,但要另查重入来源。
- "改动进了候选"只证明到 errorparse、warnparse 两张网被重建并换入,不证明运行路径用的是它们。若 h2 成立,则对"落在 shared-e3 的表改",快车道确实给不出可测候选,第 4 节立场要收窄为"仅对已部署配方成立",shared-e3 重建从延后升为第一优先。证伪实验即第 4 节末的两步,合计几分钟。
- 149 秒是负载均值约 11 时的单次测量,没有冷热分相与重复。
