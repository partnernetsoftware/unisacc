# devsvc:仓开发服务(智能体伴侣)设计稿

状态:基础版已入库,在 main 上开发(`tools/devsvc.cx`、`tools/check-devsvc.cx`、配置表 `tools/devsvc-repo.tsv`),批次 `devsvc`。

## 1 定位

- 一个仓一个实例的开发辅助服务。各 agent 经同一个 CLI 与套接字 API 互动,把分散的工具聚合到服务里,并提供事件。
- 长期目标(项目所有者 2026-11-15):成熟后成为独立产品,角色是"智能体伴侣",服务任意代码仓的多个智能体。因此从基础版起分两层。

## 2 分层

- 核心层(不含任何仓专有知识):实例生命周期、命名锁、run(原生执行体缓存)、events,以及将来的 flow 与关系 DAG。
- 本仓配置层:产品入口、构造命令、缓存键涉及的源目录、事件来源工具、在途进程匹配串、登记表名、门清单。核心只经配置读取。
- 基础版的做法:不做插件系统;专有知识集中在手写配置表 `tools/devsvc-repo.tsv`(产品入口、服务源、三个库源目录、事件来源工具与行首词、在途进程匹配串、实例目录前缀、实例目录环境变量名、约定锁名),核心启动时经 `cfg_load` 读进 `RepoCfg`;环境变量 `DEVSVC_REPO_TABLE` 可指向另一张表。没有内置缺省:表缺失、缺键、重复键即 rc 3。`devsvc config` 打印所读内容。核心里仅剩的仓专有字面量是表的缺省相对路径。
- 判据:核心代码里出现 `unisacc`、`tools/`、`cx/`、`batches.tsv` 等字面量即越界,后续可加一条棘轮检查。

## 3 进程模型

- 实例目录:`$UNISACC_DEVSVC_DIR`,否则 `$HOME/.cache/unisacc/devsvc/<sha256(git 公共目录绝对路径)前 16 位>/`。所有版本工作目录共用一个实例。
- 目录内:`sock`(unix 域套接字)、`pid`、`locks.tsv`(锁落盘)、`native/<键>/<工具名>`(原生执行体)。
- 有界常驻:ttl 默认 1800 s、硬上限 7200 s;连续 idle(默认 300 s)无请求即自退并删 sock。没有无界常驻。
- 嵌套继承规则(2026-11-17):devsvc 给子进程导出 `UNISACC_FLOW_ROOT` 时同时导出标记 `UNISACC_FLOW_ROOT_JOB=<该根>`;嵌套的 devsvc 若本次根(入口取 git 顶层,核心取 cwd)等于标记根或在其下,即同仓嵌套,继承照旧;不在其下(临时根、夹具根)就丢弃继承的 `DEVSVC_REPO_TABLE`、`DEVSVC_REPO_TABLE_PIN`、`DEVSVC_SELF_BIN` 与等于标记的 `UNISACC_FLOW_ROOT`,按本根重新解析;未带标记的 `UNISACC_FLOW_ROOT` 视为调用者显式给的,始终保留。实现:tools/devsvc.cx 入口与 tools/devsvc-core.cx 的 flow_root_env;断言:批次 devsvc-compile-context(nested-* 与 unmarked-explicit-root-kept)。调用者不必再自己 `env -u`。
- 单实例:listen 遇 EADDRINUSE 先试连,连得上即拒绝再起,连不上视为陈旧 sock 清掉重听。
- 无实例时:lock、run、events 自动拉起(等待上限 40 s);status、ping、stop 不拉起;run 可 `--local` 或在拉起失败时本地执行。

## 4 CLI 与 API

- CLI:`sh ./unisacc.com tools/devsvc.cx -- <子命令>`,子命令见文件头注。
- API:一连接一请求。请求一行 TSV `verb<TAB>arg...<LF>`;响应首行 `OK<TAB>...` 或 `ERR<TAB>code<TAB>...`,其后载荷行,服务端关连接。
- 现有 verb:ping、stop、lock-acquire、lock-release、lock-list、native、emit、events。`.cx` 客户端用 `net.unix` 加 `net.tcp` 的带截止收发即可。
- 退出码:0 OK;1 ERR(如锁被他人持有);2 用法错;3 连不上实例;run 为工具自身退出码。

## 5 基础版服务

- (1) status:各工作目录的已暂存、未暂存、未跟踪计数(`--paths` 列路径)、在途进程、实例与锁。
- (2) lock:命名写窗口锁,带属主与截止,自动过期,同属主再拿即续期,落盘跨实例重启。约定名:`registry`(tests/batches.tsv、tests/coverage.tsv 写窗口)、`derive`(派生表写窗口)。锁是协作约定,当前不强制拦截写入。
- (3) run:服务构造并缓存工具的原生执行体,客户端直接执行,stdout 与退出码即工具自己的。缓存键含工具源摘要、产品身份、库源目录的树号与未提交差异摘要;库源目录有未跟踪文件时不缓存。
- (4) events:容量 256 的环形缓冲,`--since N` 增量轮询。来源:锁的获取、释放、过期,实例启动,`events emit`,`events on-change PATH...`(只读跑 on-change --dry-run 后入环,不改钩子)。

- (5) flow、f、x(批次 `devsvc-flow`,`tools/check-devsvc-flow.cx`):引擎 `cxlib.flowexec` 与 file 算子 `cxlib.flowfile` 在客户端进程里跑,服务提供写锁与事件环;无实例时照跑(via=local,不取锁、不入环)。写节点开跑前取锁(锁名为 lock 键或 path),跑完即放;节点事件在流结束后以 `flow.node` 入环。

## 5a 给 agent 的用法:用 devsvc 代替原生工具

前缀一律 `sh ./unisacc.com tools/devsvc.cx --`(下表省略)。热启动一次调用约 0.3 s;实测 `flow -` 并发 3 个文件读加 3 条命令(其中两条各睡 1 s)墙钟 1.3 s。

| 原生工具 | devsvc 写法 | 说明 |
|---|---|---|
| Read | `f read PATH [OFF LIM]` | 内容到 stdout;stderr 结果行带整文件 sha256 与总行数,sha 可接着喂给 `--expect-sha` |
| Write | `f write PATH --from SRC [--expect-sha X] [--mkdir]` | 原子替换;新内容先写到 `/tmp` 或 `$CLAUDE_JOB_DIR/tmp` 的 SRC;有实例时自动取路径锁 |
| Edit | `f edit PATH --old-file A --new-file B [--expect-sha X] [--all] [--chomp]` | old/new 从文件读,免命令行转义;old 须恰好出现一次(`--all` 至少一次);`--chomp` 去掉两文件末尾各一个换行 |
| Edits(同一文件多处) | `f edits PATH --stdin [--sep S] [--sep-next S2] [--expect-sha X]` | stdin 为若干组 `OLD`、独占一行 `@@DEVSVC-NEW@@`、`NEW`,组间独占一行 `@@DEVSVC-NEXT@@`;各组依次在内存里套用,每组 old 须在当时内容里恰好出现一次(ENOMATCH/EMULTI 指明第几组),全过才经一次 file.write 原子落盘,.cx 只试编一次,试编失败(ECOMPILE)整批不落盘;相对 PATH 按写根(UNISACC_FLOW_ROOT,缺省仓根)解析。同一文件改三处:逐次 f edit 约 6960 ms,一次 f edits 约 2390 ms(2026-11-17 实测,检查 check-devsvc-flow 的 f-edits-* 例) |
| Bash | `x [--timeout-ms N] [--cwd D] -- ARGV...` | 不经 shell(要管道就 `-- sh -c '...'`);独立进程组,超时整组杀,rc 124;退出码即命令自己的 |
| 多步并行 | `flow - [--cat] [--jobs N]`(stdin 给节点表)或 `flow run FILE.tsv` | 每行 `id<TAB>op<TAB>deps<TAB>k=v;k=v`,op 为 exec、file.read、file.write、file.edit、compare、search;无依赖的节点并发,有依赖的按序;`--cat` 直接带回各节点输出 |
| Grep、git grep | 节点 `search`:`pattern=固定串;paths=空格分隔的文件、目录或 glob;max=N` | 输出 `path:line:text`;只做固定串(cx 无正则库,`regex=1` 拒绝);跳过 `.git`、链接与二进制;超出 max 末行 `SEARCH_TRUNC` |
| cmp、diff、comm | 节点 `compare`:`a=;b=`(路径或 `node:ID`,ID 须在 deps 里)`;mode=bytes\|lines\|set` | 输出 `same=0\|1`、`diffs=N` 与前 20 条差异;不同即节点 rc 1(`fail_on_diff=0` 关掉) |
| 管道、重定向 | exec 键 `stdin_node=ID`(依赖节点的 out 作 stdin)、`stdin_from=PATH`、`stdout_to=PATH` | 不经 shell;`stdout_to` 走 file.write 的 root 越界检查,拒绝即节点 rc 125 |
| 大段值免转义 | 任意键写成 `k@=PATH` | 值取文件全文:`a2@=`、`content@=`、`old@=`、`new@=`;文件读不到即拒绝 |

节点表例(`printf` 生成 TAB;值里的 `;` 写 `%3B`,换行 `%0A`,TAB `%09`,`%` 写 `%25`):

```
r1	file.read	-	path=plans/devsvc.md
c1	exec	-	a0=git;a1=log;a2=--oneline;a3=-3
t1	exec	c1	a0=sh;a1=-c;a2=sh ./unisacc.com tools/check-devsvc-flow.cx;timeout_ms=58000
```

约定:

- (1) 读后改:先 `f read` 取 sha,再 `f edit ... --expect-sha SHA`;别人中途改过即 ESHA 拒绝,重读再改,不覆盖他人内容。
- (2) 写共享表(`tests/batches.tsv` 等)前先 `lock acquire registry --owner 自己`,调用 flow 或 f 时带同一个 `--owner`(只续期、不代放),写完自己 release。
- (3) 锁被他人持有时默认立即拒绝(rc 1);愿意排队就加 `--lock-wait-ms N`(上限 60000)。
- (4) 结果看输出行:`NODE id 状态 rc= ms= 摘要 [sha=]`、`F op ok|fail|refused ...`、`X 状态 rc= ...`;节点完整输出在 `RUN/out/<id>.out|.err`。

仍用原生工具的情形(devsvc 尚未覆盖):

- (1) 搜索:固定串用 `search` 节点;正则搜索与只列文件名(Glob)仍用原生工具或 `x -- git grep -E ...`。
- (2) 图片、PDF、notebook 的读取:`f read` 只处理文本字节。
- (3) 单次超过 300 s 的节点(`timeout_ms` 上限)与整链等正规构造工具:照旧用后台作业加监视器。
- (4) 需要交互或保持 shell 状态(cd、环境变量跨调用)的操作:x 每次是独立进程,用 `--cwd` 与节点 `env` 键显式给。
- (5) file.* 节点在引擎进程里内联执行,彼此串行;并发收益来自 exec 节点。大量文件读写想并发就拆成多个客户端调用。
- (6) 版本工作目录里先按规则放 `unisacc.com` 符号链接;写根缺省为当前目录,跨目录写须设 `UNISACC_FLOW_ROOT`。
- 只读 git 用 `devsvc git log|diff|show|status ARGS|blame|ls-files ...` 代替原生 git(不取锁,stdout 上限 1 MB,超出打 DEVSVC_TRUNCATED);事件同时落盘实例目录 events.tsv(超 4 MB 轮转 events.tsv.1,$DEVSVC_EV_ROTATE 可调),每条命令记 <命令>.<子命令> owner=($DEVSVC_OWNER,缺省 auto-<pid>) rc ms,供 tools/devsvc-usage.cx 计数;请设 DEVSVC_OWNER=<窗口名>。
后台作业(--bg,2026-11-15):x|flow|git commit|register|derive 加 --bg 立即返回 `JOB<TAB>id<TAB>run=DIR`,作业在独立会话(setsid)里跑,各自 deadline 与锁照旧,锁在作业结束时释放;元数据在实例目录 jobs/<id>/(pid、meta、stdin、out、rc),结束记事件 job.done(id、rc、ms、owner)。
- `devsvc wait JOB [--timeout-ms N]`:默认 50000,上限 55000;结束打输出尾部(至多 8 KB)与 `JOB_DONE id rc= ms=`,rc 即作业的;超时打 WAIT_TIMEOUT 并 rc 124,不杀作业,可重复 wait;作业进程消失又无 rc 文件打 JOB_LOST rc 125。
- `devsvc jobs`:列最近 30 个作业(running|done|lost)。
- `devsvc events --wait KIND [--since SEQ] [--timeout-ms N]`:阻塞等某类事件,缺省只等新事件,超时 rc 124。
- 用法:长命令一律 `--bg` 再 `wait`,不写 sleep 轮询;宿主 30/60 s 上限内反复 wait 即可。

- 并行强制参数(2026-11-15):`flow`、`x` 与 `tools/flowrun.cx run` 一律要求显式 `--par N`(1..32,单节点也写 `--par 1`),缺省 rc 2 并打 `REFUSE missing --par`;flow 节点表可写一行 `par N` 声明,命令行优先;`--jobs N` 仅作旧写法别名。上表 x、flow 两行的写法均应补 `--par N`。`git commit --whole PATH` 可提交已 `git rm` 的删除(HEAD 有、工作区无,打 `DELETE PATH`)。
### 提交门:新增 tlib 模块与分相派生

新增库按一个完整登记集提交:(1) 实跑库检查,再用 `devsvc register --tool tools/cxlib/NAME.cx --tool tools/check-NAME.cx` 同步工具清单与 `tools-compile-NNN` 分片,不要只手加清单两行;用 `devsvc register ID CHECK MARKER COVERAGE FILES --args ARGS` 登记批次。(2) 用 `devsvc git add` 只暂存本次包源;同一包的并行改动交集成者统一跑 `tools/layer-api.cx -- --bump tools.cxlib`,接口清单、api_sha、frozen 与版本一并更新,已覆盖全部本次源且检查通过的 bump 不重复执行。(3) 跑新批次、对应工具分片与 `layer-api --check`,再用 `devsvc git commit --derive --whole 本次源... 各自manifest... --keyed tests/tools-compile-sources.tsv 本次路径键 --keyed tests/batches.tsv 批次ID与分片ID --keyed tests/coverage.tsv 覆盖ID --push`。若同次新增工具分片,不要混用 `--registered` 与该表的额外 `--keyed`:当前 `--registered` 会独占注册表选行并丢弃额外键;只含完整注册集时可用 `--registered`。包版本与接口表由 `--derive` 在 HEAD 加本次包源的私有树生成,不把别的在途源混入 bump;共享表只带本次行,ujs 暂存保留。

`git commit --derive` 先在提交专属树执行独立有界派生阶段,落内容回执,再由提交阶段核对基准提交、输入树、产物树与回执摘要;身份变化就拒绝复用。回执的 `phase=derive|commit` 与 `derived=completed|incomplete` 区分失败位置;先用 `devsvc git commit --status JOB` 确认真实终态,不要重复已落地的提交。修正输入后重试原命令会重新派生。直跑工具可用 `commit-own --derive-prepare FILE` 与同参数的 `--derive-receipt FILE --derive-id SHA` 分相,后者只核身份并过原提交门。

`derive --write` 只按生成器版本、输入内容和当前产物内容复用构建身份,输出 `IDENTICAL`,不复用测试结论;未知输入闭包仍全量执行。补充读集在 `tools/derive-inputs.tsv`,`derive --check` 始终全量重算,`--write --full` 是强制重建对照。新增派生项同时补读集与逐表篡改负控,完整检查通过后才算派生清零。

### 工单作业化 task(R5,2026-11-15)

子代理与窗口的每一单都开成工单,协调者只认 `task.done` 事件(`devsvc events --wait task.done` 或实例目录 `task-events.tsv` 的 `task.done` 行),不认"已完成"的口头汇报。工单落在 devsvc 实例目录的 `work/`,不入库。

- 开单:`sh ./unisacc.com tools/devtask.cx -- task open --id ID --title T --write 路径前缀... --accept '验收命令' [--accept '...']... --deadline-min N`。记下写域、验收命令和当时工作树脏路径的内容哈希快照;同名工单未完成时再开 rc 1。
- 看进度:`task status ID`。打印已用秒 `used_s`、剩余秒 `left_s`(过期为负且 `overdue=1`)、写域内已改文件 `CHANGED`、写域外被改文件 `OUTSIDE_WARN`。还有剩余秒就继续干,不得以"到截止"收工。
- 收工:`task done ID`。逐条实跑验收命令(经 `devsvc x`,在仓根,各 55 s 上限),并核写域外无改动;全过才发 `task.done` 事件、记 `DONE` 行,rc 0;任一不过打 `TASK_NOT_DONE` 与失败命令输出尾部,rc 1,工单保持打开,修完可随时重试。超过 55 s 的验收拆成多条,或先 `devsvc x --bg` 起作业、验收命令写成 `devsvc wait JOB`。
- 列表:`task list`(open、overdue、done)。
- 已知边界:共享检出里别的窗口在本单期间改了写域外文件,也会计入 `OUTSIDE_WARN` 并挡住 done(工具分不清是谁改的);此时在汇报里写明路径与归属,由协调者裁决,不绕过。开单时已脏且内容未变的路径不计。

简报模板(固定五段之外加一行):

```
目标:...
写域:...(与 task open --write 一致)
验收命令:...(与 task open --accept 一致,含预期值)
截止:N 分钟(task open --deadline-min N)
禁区:...
工单:开工先 task open,收工前 task done;未见 DONE 行不得报完成,TASK_NOT_DONE 时原样贴出失败尾部并继续修,或在截止时如实报未完成。
```
- 输出控制(P2,2026-11-15):x、flow(含 --bg 后的 wait)统一支持 `--tail N`、`--head N`、`--grep S`(固定串,可多次,行级或)、`--summary`(每节点一行 `SUM id rc= ms= lines= first=`,first 为首个含 FAIL/REFUSE/error 的行)。任一给出即合并 stdout 与 stderr 过滤,先 grep 再 head 再 tail;全量落 run 目录 `full.out`(wait 为作业目录 `out`),有行略去时打 `DEVSVC_TRIMMED shown=N total=M`,末行 `OUT=路径`。不给时行为不变。代替 `2>&1 | tail -N` 一类管道截短,例:`devsvc x --tail 20 -- sh ./unisacc.com tools/check-x.cx`、`devsvc flow run F --summary`、`devsvc wait JOB --grep FAIL`。
- 工单时钟(S3 第一步,2026-11-15,批次 devsvc-remain):开工 `export DEVSVC_OWNER=名字` 并 `task open` 之后,每条 `devsvc flow|f|x|doc|git` 命令结束都会在 stderr 末尾带 `TASK_LEFT id=ID left_s=N used_s=M`(过期为 `TASK_OVERDUE id=ID over_s=N used_s=M`),`task done` 通过后不再出现。代理按这行判断剩余时间,不按工具调用次数估;left_s 还大而验收未过时继续做,不以"到截止"收工。
- 速记(2026-11-15):不必手拼节点表。`sh ./unisacc.com tools/flowsh.cx -- sh 'CMD1' 'CMD2' | sh ./unisacc.com tools/devsvc.cx -- flow - --cat` 每个参数一个并发 exec 节点;`read PATH:A-B ...` 每个参数一个 file.read 节点;`map V1 V2 -- ARGV 含 {}` 按值展开;`--chain` 串行、`--after ID` 接到已有表、`--timeout-ms N`。值由工具转义。失败或超时的 exec 节点,NODE 行末尾自带 `first=`(stderr 首个非空行),先看它再决定要不要 --cat。
- 事务化 flow(S1,2026-11-15):一次改动跨多个文件(库加调用方、batches.tsv 加 coverage.tsv)时写进一个 flow,加 `--txn` 或节点表里一行 `txn 1`:`devsvc flow - --txn`。全部 file.write、file.edit、doc.* 节点先暂存不落盘(同 flow 后续节点读到暂存内容,exec 节点仍读磁盘),锁按名排序一次取齐(含写路径所属共享表锁),然后整组校验:被写 .cx 加直接消费者试编(use 边,缺省至多 8 个,`txn_consumers N` 行或 $UNISACC_TXN_CONSUMERS 可调;写前就编不过的消费者记 prebroken 不拦)、tests/batches.tsv 与 tests/coverage.tsv 被写时在暂存视图上跑 tests/run.cx -- --list 须 metadata_valid=1、被写文件磁盘内容未被他人改动;全过才逐文件原子落盘,任一不过整组不落盘,落盘中途失败回滚已写文件。结果看 `NODE TXN` 行:`FLOW_TXN committed paths=N` 或 `TXN_ABORT reason=ECOMPILE|ECONSUMER|EREGISTRY|ESHA|ENODE|EPUBLISH node=ID path=P`,明细(TXN_CHECK 行与编译输出)在 RUN/out/TXN.out;被拒时各写节点补一行 fail rc=125。不加 txn 时行为不变。检查 tools/check-flow-txn.cx(批次 flow-txn)。
- 改完就验 dcheck(P4,2026-11-15,批次 dcheck):改完文件不再各自直跑 `sh ./unisacc.com tools/check-*.cx`,统一跑 `sh ./unisacc.com tools/dcheck.cx -- [路径...]`。不给路径取 git 工作区改动(暂存、未暂存、未跟踪),`--owner O` 改取该 owner 经 devsvc 写事件 path= 记录的路径;用 tools/affected.cx 按 `--tier`(缺省 commit)选批,`--only ID,ID` 取交集(未被选中的 ID 标 NOT_RUN,不盲跑);生成一张节点表经一次 devsvc flow 并行跑:kit 准备节点在前,每批一个 exec 节点(`affected --ids ID --run -j 1`,声明材料的批加 `--prepare-materials`,各 55 s 上限),`--par N` 缺省取批次数与 32 的较小值。输出每批一行 `CHECK id PASS|FAIL|DEFERRED|NOT_RUN ms first=首条失败行`,再 `DCHECK_FLOW peak= par= select_ms= run=`,末行 `DCHECK total= pass= fail= deferred= not_run= wall_ms= serial_ms=`;PASS 须节点 rc 0 且有该批自己的 BATCH_RUN status=PASS 行,超时与延后标 DEFERRED,被跳过或无结果标 NOT_RUN;rc 0 全过或无批可跑,1 有 FAIL,3 无 FAIL 但有 DEFERRED 或 NOT_RUN,2 用法或运行器错。`--bg` 立即返回 JOB(接 `devsvc wait JOB`),`--plan` 只打印选批与节点表。不缓存任何结论;非夹具运行结束发一条 `dcheck.done` 事件。整条命令窗口 53 s:选批冷启动实测 15 s、热 1.2 至 2.1 s,窗口剩余给 flow,排不上的批如实 NOT_RUN,用 `--only` 分次补跑。已知缺口:affected 没有独立的材料准备入口,材料准备仍在各批节点内完成,只有 kit 是前置节点;批数超过窗口时要分次。
- 简报模板加一行:`检查:改完跑 sh ./unisacc.com tools/dcheck.cx -- 改动路径(或 --owner 自己),按 CHECK 行处理 FAIL 与 DEFERRED,不直跑 check-*.cx。`
- 外部根与工作树:`task open` 可加 `--write-ext ABSDIR`(可多次;须已存在的绝对目录,拒绝 / 与 HOME 本身及上两级;目录内文件按内容 sha 做快照,done 时比对,此时 `--write` 可省)和 `--cwd DIR`(验收命令在该目录执行;须在仓内 `.claude/worktrees/` 下或某个 `--write-ext` 目录下,缺省仍是主检出根)。
- `--write` 里的前缀写成 `.claude/worktrees/v0.0.NN` 时,status 与 done 会另对该工作树做 `git -C` 脏文件快照,路径以 `.claude/worktrees/v0.0.NN/` 开头;验收放在工作树里的检查用 `--cwd 该工作树绝对路径`。检查 `tools/check-devtask.cx` 的 ext-、worktree-、cwd- 开头用例。
- 新建文档:`devsvc doc create FILE 标题`(FILE 须匹配配置表 doc 行,已存在 rc 1 拒绝,内容为一行一级标题,内部取锁 doc:FILE);其后再用 doc append/set。服务空闲巡检另会回收自己的 /tmp/devsvc-flow-* 运行目录(超 2 小时且无进程引用,每次至多 200 个,记 flowdir.gc 事件)。dcheck 遇批次登记预算超过 55 的,在该批 CHECK 行前打 DCHECK_WARN budget-above-node-cap。
- git commit 参数约定(2026-11-16):`devsvc git commit -m 说明 (--whole 路径... | --keyed 路径 KEY | --registered ID...) [--push] [--bg]` 严格解析。必须有 -m 与至少一个路径来源;出现未知选项或 --help 一律打印用法、rc 2、不提交(HEAD 与索引不变)。说明首行不得为空,不得以 Co-Authored-By: 或 Signed-off-by: 开头,违反 rc 2 并提示"说明首行必须是正文,署名放最后空行后"。检查 check-devsvc 的 gitargs- 用例(夹具仓,环境变量 DEVSVC_GIT_PARSE_ONLY 只过解析)。
- task open 路径约定(2026-11-16):`--cwd` 可写绝对路径或相对仓根的路径(如 .claude/worktrees/v0.0.68),须落在 .claude/worktrees/ 或某个 --write-ext 目录下,否则 rc 2 并给示例;`--write` 写仓内相对前缀,也接受以仓根为前缀的绝对路径(内部转为相对,登记与输出均为相对),仓外绝对路径 rc 2。检查 check-devtask 的 write-abs- 用例。
- dcheck 选批与跑批分两作业:先 `dcheck --select-only --out /tmp/m.tsv [路径...] --bg` 选批(devsvc wait 得 DCHECK_SELECT 与 DCHECK_MANIFEST,清单落盘含每批预算),再 `dcheck --from /tmp/m.tsv --par 8 --bg` 只跑批,每批各自 55 s 节点,不共用选批的整窗;不带这两个选项仍一次完成。清单损坏(头、字段、id 字符、重复、计数)rc 2。







### tlib 包版本预留 bump(K3,2026-11-17)
- 用途:多个并行任务改同一 tlib 包(如 tools.cxlib)时,各自跑 `layer-api --bump` 会得到互相覆盖的版本。改为先经 devsvc 预留版本:`sh ./unisacc.com tools/devsvc.cx -- bump PKG [--owner O] [--kind patch|minor] [--ttl-s N]`,输出 `BUMP PKG=.. VERSION=.. OWNER=.. EXPIRES=.. REUSED=0|1`;释放 `bump --release PKG VERSION [--owner O]`;查询 `bump --list [PKG]`(每行 BUMP_ROW 带 STATE=reserved|released|expired|consumed 与 HEAD 版本)。启动器把 bump 直接转 tools/bump.cx,不经核心。
- 语义:V 取 `git show HEAD:build/tlib-packages.tsv` 里该包版本(不读工作副本);新预留 = max(V, 该包全部有效预留) 的下一 patch(或 minor);同属主同包再预留返回原值(REUSED=1,不递增);属主缺省 $DEVSVC_OWNER,再缺省 auto-<pid>;ttl 缺省 7200 s,上限 14400 s;到期转 expired 并记 bump.expire,其版本可再分配;版本不大于 HEAD 版本的预留在每次调用时按 HEAD 判为 consumed,不再占用,无需写回。
- 状态:实例目录(同 leases/locks,dt_instdir;检查用 --dir 或 $BUMP_DIR)下 bump-reservations.tsv、bump-events.tsv 与 bump.lock(flock,等待 $BUMP_LOCK_WAIT_MS 缺省 5000、上限 20000 ms,超时 rc 1 `BUMP_REFUSE lock busy`)。不进 git。事件 bump.reserve/bump.release/bump.expire 另经 `devsvc events emit` 进事件日志($BUMP_EMIT=0 关闭)。
- 与 layer-api 的关系(K4 起):提交前先 `sh ./unisacc.com tools/devsvc.cx -- bump PKG` 预留版本 V,再 `sh ./unisacc.com tools/layer-api.cx -- --bump PKG --version V`(只在 V 是调用者自己的有效预留或 HEAD 的下一 patch 时写入,否则 rc 2 REFUSE version-not-reserved);`bump --query PKG [VERSION]` 给出 VERDICT=head|next|reserved|reserved-other|unreserved。提交门 precommit 的 tlib 门对每个涉及的包查询一次:暂存版本既非下一 patch 也非任何有效预留(VERDICT=unreserved)时打印 `TLIB_BUMP_UNRESERVED pkg= version= expected=` 并附 `devsvc bump` 用法,含义是这个版本号没经预留、可能与并行任务撞号;只提示,不计入 blocked,查询失败时不出声;TLIB_BUMP_NEEDED、TLIB_API_STALE 等原有拦截不变。检查 tools/check-bump.cx(批次 devsvc-bump)、tools/check-precommit-tlibhint.cx(批次 precommit-tlibhint)。

### 进程捕获:单执行器与兼容门面

sysx.processcore 是 np_buf、np_result 与捕获执行逻辑的唯一来源;sysx.process 与 cxlib.netpack 的 np_run、np_free 只转发。模块尚不支持重导出或同名导出合并,同单元不能同时 use 两个门面;只需捕获的工具任选一个,需要 gitsync 的服务使用 netpack。缺口登记在 build/pkg-gaps.tsv 的 NP-PROCESS-UNIFY。373 个仅依赖 netpack 的真实调用文件无需补 use。

迁移明确改变两项旧行为:(1) netpack 按循环累计 tick 改为单调墙钟,调用进程被暂停时仍消耗同一秒数预算;(2) 旧阻塞 exec 握手改为与捕获共用非阻塞截止,执行前迟滞不再绕过预算。tools/check-process-deadline.cx 各有一条回归断言:旧实现加同一执行前延迟测试接缝后分别为 2610 ms、2502 ms 且两条失败;共享执行器为 1619 ms、1017 ms 且两条通过,均用 1 秒调用预算。日志入口 /tmp/npbuf-cdx 与 /tmp/devsvc-flow-21356743325-2369480。没有放宽原批次时限或修改既有断言。

另有两项兼容边界必须保持:(1) np_result 的 timed_out 尾字段在开始与释放时清零,仅预算耗尽时置位;(2) UNISACC_NP_FAULT 只由 netpack 门面启用,sysx.process 仍忽略该工具测试变量。首次合并时第二项曾使 check-npfault 的 D 两条原断言失败;保留断言、修正转发标志后 13 条全过。直接子进程在超时返回前有界收尸,不偷取调用者其它子进程,由原 check-netpack 与 check-reaper 合同覆盖。UNISACC_NP_TEST_PREEXEC_MS 仅在子进程中作最多 5000 ms 的回归接缝,默认关闭;未改变调用的截止值。

共享模块变更验收清单:(1) 逐个列出全部 use 消费者及其登记运行批次,用 affected --files 模块取集合,编译分片不能替代运行批次;(2) 共享检出上用当前已装产品分别做 default-run 与 -S/-c 静态编译,明确编译方式和头文件/模块输入身份,不把宿主运行成功当作静态生成成功;(3) 同输入跑受影响运行批次,每批原时限不变,并列保留失败、重跑与 UNKNOWN 收据;(4) 进程原语至少复验 lower-prefix-base3/4、cx-reaper、process-deadline、procrun-tool、cx-netpack、cx-npfault。7026ecaa4d 漏了 processcore 的裸 setpgid 声明,现改走 sys.posix 的 sy_setpgid;processcore-compile 永久覆盖 tests/bound-rc142.cx 与 tests/all-aggregate.cx 的 -S 路径,affected-processcore 覆盖传递 use 的选批正例与无关源负控。

## 6 已知限制与下一单

- (1) 服务单线程:native 构造期间其它请求排队。改为构造放子进程,主循环继续收请求。
- (2) 事件环不落盘、无阻塞订阅。加 `events wait --since N --timeout S`(长轮询)与落盘尾段。
- (3) status 不标路径归属。接 `tools/domain.cx` 与 `tools/own-diff.cx`。
- (4) 锁不强制。让 `tools/commit-own.cx`、`tools/register.cx`、`tools/derive.cx --write` 在写共享表前查锁,钩子接入另议(本轮禁改 `.claude/hooks`)。
- (5) 本仓配置表落地(见第 2 节),并登记批次、补 prd.md 能力速查、重算 tool-index。
- (6) 两个 start 同时清理陈旧 sock 有窄竞态,改用锁目录原子创建。
- (7) 已完成(第二轮提升):exec 重定向键 `stdout_to`、`stdin_from`、`stdin_node`,`compare` 与 `search` 算子,`k@=PATH` 取值,均在引擎 `cxlib.flowexec` 与 `cxlib.flowfile`,检查在 check-flowexec、check-flowfile、check-flowrun。余项:search 无正则(cx 缺正则库)、目录内文件按目录序不排序。
- (8) 开发库自举缺口:devsvc 自身依赖 `cxlib.flowexec`、`cxlib.flowfile`,改这两个库的中间态编不过时 `f edit` 也随之不可用,只能退回原生编辑;应让 f 与 x 走已缓存的原生执行体或上一版可用的库。
- (9) 节点输出回显:`flowrun --echo` 已有 `--echo-max N`(字节,默认 65536,超出打 `ECHO_TRUNC` 行);devsvc `flow --cat` 尚未接同一上限。
- (10) flow 节点事件现在流结束后才入环,改为运行中实时入环,file.* 节点改为可并发。
- (2026-11-15 cc 排期)配置表随 HEAD 固定、锁释放 not-held 定因与续租:在途。
- (2026-11-15 cc 排期)devsvc-usage 使用率度量(每会话 devsvc 与原生调用占比),巡检每轮跑,原生用法排名前列者优先补能力:在途。
- (2026-11-15 cc 排期)register 提速(并行与增量,判定强度不降):在途。
- (2026-11-15 cc 排期)precommit 迁为 flow 节点图并与原门对拍:在途。
- (2026-11-15 cc 排期)待排:x --cat 上限与 --cwd(限仓内 .claude/worktrees/)、--bg 加 events wait、tools.cxlib bump 经 devsvc git 内部暂存完成、auto-continue 判断收紧、钩子对 Bash 直跑 git commit 与 unisacc.com 先告警后拒绝、关系 DAG G2 至 G12、build/*.tsv 第一批读者切换。
- (2026-11-15 痛点分析,近 12 小时 44 会话,devsvc 占比 0.42,明细 cc 作业目录 devsvc-pain.tsv)排期按每天浪费排序:
- P1 缺陷:x 事件类型取成首个参数(x.--timeout-ms 43 条、x.--bg 24 条),统计失真,先修。
- P2 x/flow 输出摘要:--tail N、--grep S、--summary,全量落 run 目录并给路径(原生 head/tail/grep/cat 超 2600 次)。
- P3 f read 行段与 who-calls 结果直接跳读(sed -n 244 次、cat 311 次)。
- P4 devsvc check:按改动选批并行跑(check 直跑 666 次),结果进事件。
- P5 devsvc ps 与按作业 kill(pgrep/ps/sleep 122 次)。
- P6 precheck --files:一次列出全部门拦截原因(blocked 96 次、REFUSE 73 次)。状态(2026-11-17):已做。`devsvc precheck --files PATH...` -> `tools/precommit.cx -- --precheck PATH...`(b7c376e9fd 模式,0bec77c73b 检查与批次 precommit-precheck-pass/-trunc,入口随本行提交):私有临时索引(HEAD 加给定路径)上以 UNISACC_PRECOMMIT_SERIAL=1 重跑同一份无参门,逐门 `PRECHECK gate=NAME verdict=PASS|BLOCKED ms=N reason=...`,末行 `PRECHECK_SUMMARY blocked=K total_ms=T slowest=NAME rc=N`;总 deadline 55 秒,超时出 PRECHECK_TIMEOUT 列已评估/未评估门;真索引不读不写,不缓存结论。已知局限:(1) runlist、facts 门失败时原逻辑直接返回,其后的门不再评估,预检同样只列到此;(2) 不打 BLOCKED/REFUSE/`precommit: blocked` 文案的失败只报 `gate=other` 并给日志路径;(3) 归门按日志出现顺序,文案出现在下一门 begin 之前才归对。评估项(未排):runlist、facts 失败后继续评估其余门是否改变原门判定,须单独论证后再动,不为"一次列全"放宽或改动原门。
- P7 按历史耗时预警超预算并建议分片;长命令自动转后台。
- P8 锁 who/wait 查询。
- P9 只读 git 已有(4ebf3b6834)但采用率低:宣传、简报模板写明;新建工具前的 git log --all 给专用入口。
- (2026-11-15 风险评审,证据入口 `sh ./unisacc.com tools/devsvc-budget.cx -- --min-ms 15000` 与 devsvc-core.cx 原生缓存键 261 至 287 行)每条:根因、方案、优先级、验收。
- R1 公共库改坏连累全员。根因:共享检出只有一份工作树,除 devsvc 启动器固定 HEAD 外,其余工具 -run 直接读工作树中间态;在途的写前试编只编被写文件本身,不编消费者(库接口变了自身能编、调用方编不过),也管不到原生 Edit/Write。方案:(a) f write/edit 落到 tools/cxlib、cx、ext 时,把 HEAD 归档加待写内容铺成临时覆盖层,按 rel 的 use 关系取消费者集并发试编,全过才原子落盘;(b) devsvc run 缺省用 HEAD 固定的库跑工具(同启动器),工作树中间态只影响显式 --dev 的作者;(c) 提交门对库改动强制跑受影响批次。优先级 P0。验收:夹具里把库函数改签名,f edit 被拒并列出编不过的消费者,工作树字节不变;另一窗口同时 run 任一工具 rc 0。
- R2 检查贴 55 s。实测(三个实例事件 835 条):rc 124 共 12 条、不小于 44 s 共 20 条;job.done p90 41 s,git.commit p50 14.4 s、p90 41.5 s、max 47.4 s。根因:原生缓存键是 cx、ext、tools/cxlib 三棵树的树号加整段 git diff 的摘要,且三处有任一未跟踪文件即 bypass 每次重建;多窗口并行时任何人的未提交改动或新文件都使全员缓存失效,夹具内冷编译因此几乎必不命中。方案:键改为工具源加其 use 传递闭包各文件内容摘要加产品身份(闭包取自 rel 链接表),与工作树其它脏文件无关;夹具经环境变量共用同一内容寻址缓存目录;-run 缓存同键。不缓存测试结论,只缓存执行体。优先级 P0。验收:工作树有无关未跟踪 .cx 时 run 两次第二次 cache=hit;改闭包内任一文件后 miss;check-devsvc-git 热跑 p90 低于 30 s(devsvc-budget 取数)。
- R3 门迁入 flow 无提速。根因:precommit-flow 各门已是无依赖并发节点,墙钟等于最慢单门加串行前后段(取计划、ratchets --from-run),并发度再加也不动关键路径;且 flow.node 事件只记数字节点号与 op,事件里算不出哪一门最慢(本轮未能逐门实测,记未测)。方案:(a) flow.node 事件带节点名,devsvc-budget 即可按门出分位;(b) 最慢门按文件分片成多节点;(c) 门按输入表与路径声明读集,只跑读集与本次暂存相交的门(选择不是缓存结论,未跑的门标 NOT_AFFECTED 不标 PASS);(d) derive 按读写表建依赖后拓扑并发;(e) R2 的缓存消掉各门冷编译。优先级 P1。验收:同一暂存下 precommit-flow 与原门判定逐行一致,p50 墙钟降到最慢单门分片后的值,事件可见各门 ms。
- R4 采用率 0.42。根因:P1 事件类型缺陷实测仍在(x.--timeout-ms 46 条、x.-- 14 条),按工具统计失真;`devsvc --` 无参只打一行 usage,子命令表只在 devsvc-core.cx 头注释里,子代理看不到入口;输出无截短筛选所以外面再套 head/tail/grep。方案:devsvc help 分组列子命令与一行例子;x/flow/git 统一 --tail N --grep S --summary;devsvc check 统一入口;devsvc ps;钩子对直跑先告警。优先级 P1。验收:devsvc-usage 比例连续两轮巡检不低于 0.7,原生 head/tail/grep 套用次数减半。
- R5 子代理 1 至 4 分钟提前收工。根因(判断,未做转录统计):截止只是简报里的文字,子代理没有可查的墙钟与剩余时间,也没有机器判定的完成条件,保守起见提前交。方案:devsvc task open ID --domain 路径 --accept 命令 --deadline-s N 落成作业(实例 tasks/ID/,记开始时刻与输入身份);task status 打已用与剩余秒数;task done 以 flow 跑全部验收命令并核改动不出写域,全过才记 task.done,否则打 TASK_NOT_DONE 加失败项与剩余秒数,rc 1;协调者只认 task.done 事件。不放宽时限,不代记 PASS。优先级 P1。验收:夹具验收命令 rc 1 时 done 被拒且事件无 task.done;写域外有改动被拒;剩余时间输出与开始时刻一致。
- R6(自查)实例目录下有三份 events.tsv(045c5a383068df3b 673 行、968568b4c968e332 385 行、9837e2ae04f553e0 11 行)。若后两者不是检查夹具而是同仓另起的实例,则锁不互斥,属高风险;待核:status 打印实例键来源并对同一 git 公共目录出现多实例时告警。优先级 P0 待证。
- R7(已做)devsvc-budget:按事件历史对每类命令与 flow 节点出 n、p50、p90、max、贴限数与 rc 124 数,--fail-near 可作巡检门;P7 第一步。批次 devsvc-budget,检查 tools/check-devsvc-budget.cx 十二例。后续:接入 x/flow 开跑前预警与分片建议。
- (2026-11-15 简单方案 S1 至 S3,项目所有者约束:不做每 agent 私有工作区、覆盖层、独立工作树;协同唯一接口是 devsvc flow 及其算子;先于 sqlite 之类方案,今天能落地)证据入口:实例 045c5a383068df3b 的 events.tsv、`tools/devsvc-budget.cx`、`tools/devsvc-usage.cx`、实例 work/*.tsv。
- S1 共享检出互相踩。证据:f.edit 试编拒绝 ECOMPILE 16 条(只拦被写文件自身);f.edit p50 11 ms、p90 3170 ms、max 10074 ms,f.edits p90 7345 ms(耗在逐文件试编与 30 s 文件锁内);事件里没有锁等待记录,简报所说排锁 20 至 30 s 未能从事件复现,记未证实。根因:(a) 一次改动跨多个文件(库加调用方、batches.tsv 加 coverage.tsv)被拆成多条互不相干的 f edit,中间态对全员可见,任一步被拒或作者中断就停在半截;(b) 试编只编被写文件,不编消费者;(c) 登记表两张分别写,无成对校验。方案(全在 flow 内):事务化 flow。一个 flow 里的全部 file.write/file.edit 节点先只算出新内容不落盘,引擎按路径排序一次取齐文件锁(共享表取配置表 lock 行的表锁),在锁内做写前校验:.cx 试编被写文件加 rel who-uses 给出的直接消费者(读盘时以待写内容替换同名文件,不建工作树);写 tests/batches.tsv 或 tests/coverage.tsv 时跑两表成对一致性校验;全过才逐文件原子改名落盘,任一失败整组不落盘,已落盘的按写前字节回滚,末行 `FLOW_TXN committed|rolled_back paths=N`。flow 文件首行 `txn 1` 开启,缺省行为不变。取舍:锁持有时间从单文件试编变成整组试编(估 3 至 10 s),换来没有半截状态;消费者只取直接一层,传递闭包留给提交门;不管原生 Edit(钩子已拦)。验收:夹具 flow 改库函数签名但不改调用方,整组被拒、工作树字节不变、列出编不过的消费者;同组补上调用方则通过;只加 batches 行不加 coverage 行被拒;两个 flow 同写一表,后者等前者提交后在新内容上校验。
- S2 一次提交太贵。证据:git.commit 事件 40 条中 rc 非 0 共 18 条(45%),失败单次 13 至 50 s;git.commit p50 14.5 s、p90 50.1 s、max 86.4 s;register 单次 37 至 55 s(register.devsvc-run p50 50 s,贴 55 s 上限)。失败原因本轮未逐条分类(d59b6a29a5 之后才有 GIT_COMMIT_ERR 尾行),记未测。根因:(a) 门在提交时才第一次跑,失败一次就整轮重来;(b) 门的输入是整个工作树与暂存区,他人半截改动进入判定;(c) register 与提交各跑一遍检查。方案(全在 flow 内):git.commit 算子作为 flow 末节点,提交路径集缺省等于本 flow 的 file.* 节点写过的路径(加 --registered 的键行),用 commit-own 的私有索引只带这些路径;precommit 各门是同一 flow 的并发节点,按读集与这组路径求交,不相交标 NOT_AFFECTED(不标 PASS);受影响批次经 affected 选出并发跑,结果只在本 flow 内使用,不跨 flow 缓存。与 S1 合起来即 `改 -> 试编 -> 受影响检查 -> 提交` 一条 flow 一次过,失败在写前或提交前暴露且整组回滚。取舍:完整门仍由慢车道与集成者全跑,flow 内只是选择不是放宽;他人路径的脏状态不再拦我,代价是他人坏状态要靠其自己的 flow 拦。验收:同一暂存下与原 precommit 判定逐行一致(未跑的门列 NOT_AFFECTED);工作树有他人无关脏文件时提交 rc 0;git.commit 失败率与 p90 由 devsvc-budget 取数,目标失败率低于 10%、p90 低于 30 s。
- S3 子代理提前收工或未验收就报完成。证据:实例 work/ 下 9 张工单,done 3 张(用时 1 s、431 s、536 s),其余开单后没有 done 记录(其中过期未 done 至少 3 张:r2b、commit-own-materials、hookwarn);本单作者实测:开工后自估已用 5 分钟以上时 `date +%s` 实际只过了 48 s。根因:代理没有时钟,按工具调用次数估时间,估值偏大数倍;`task status ID` 要主动调用且要记得单号,实际没人调。方案:剩余时间由 flow 输出携带,代理不用主动查。第一步(已做,批次 devsvc-remain,检查 tools/check-devsvc-remain.cx 十三例):启动器 tools/devsvc.cx 在 flow 族子命令(flow、f、x、doc、git)结束后读实例 work/ 下 $DEVSVC_OWNER 名下未完成工单,在 stderr 逐单打 `TASK_LEFT id=ID left_s=N used_s=M`,过期打 `TASK_OVERDUE id=ID over_s=N used_s=M`;只读、不改退出码、不进 stdout。放在启动器是因为 devsvc-core.cx 当时有他人未提交改动。第二步:移进引擎,作为 DEVSVC_FLOW 汇总行的字段 `task=ID left_s=N`,并在 left_s 大于 60 且属主无 task.done 事件时加一行 `TASK_HINT not accepted yet: devtask task done ID`;协调者只认 task.done 事件。取舍:每条命令多读几个小文件(实测无感);不设 DEVSVC_OWNER 的代理看不到,简报模板须写 export。验收:`sh ./unisacc.com tools/check-devsvc-remain.cx` 末行 `CHECK_DEVSVC_REMAIN failed=0`;一周后工单 done 率(done 数除以开单数)由 3/9 升到不低于 0.8。
- 上述三条取代 R1 方案 (a) 中的"HEAD 归档加待写内容铺成临时覆盖层"与任何私有工作树做法:消费者试编改为在共享检出上以待写内容替换同名文件读入,不落盘、不建目录树。落地顺序:S3 第一步(已做)-> S1 的 `txn 1`(只做整组校验与回滚)-> S2 的 git.commit 算子 -> S1 的消费者试编与登记表成对校验。
- commit-own --registered 的派生表键选缺陷(2026-11-15,fable-ompflow 实遇):build/tool-index.tsv 里与本批新增行相邻、且工作区已被重算改值的他行(flowrun.cx、check-flowdoc.cx,仅行数列变化),只被选中删除侧,提交 51ce77ac09 因此丢了这两行,已由 2142b3ee48 用 --keyed 补回。待修:键选对"同键新旧成对"的修改行必须成对取或成对不取,check-commit-own 加此负例;修好前 --registered 之后先看 SELECT 行里有无不属本批的 `-` 行。
- S2 失败分类(2026-11-15,窗口 fable-s2;取数:实例 045c5a383068df3b 的 events.tsv 中 git.commit rc 非 0 共 20 条,即简报时的 18 条加其后 fable-ompflow 的 2 条,总数 47;按事件 pid 对到 /tmp/devsvc-flow-起始毫秒-pid 的 flow.tsv 与 out/x.err、x.out 逐条读):(1) 参数错误 7 条 35%(rc 2,146 至 364 ms):--registered 已拥有的表又给 --whole tests/batches.tsv 5 条,--dry-run 与 --help 各 1 条;(2) precommit 门拦截 6 条 30%:cx-lint 的 R4-source-anchor 与 R2-timeout 4 条(38.7、46.7、42.7、42.6 s),cxtrunc 棘轮无基线行 2 条(12.0、13.4 s),都是本提交自己的代码问题,贵在提交末尾才第一次跑门;(3) 锁等待超时 4 条 20%(rc 1,50.06 至 50.09 s,无 run 目录,即 auto_lock 等 git-index 或 registry 锁到 50 s 上限;同期有 rc 124 的 60 s 提交作业被杀,服务侧 120 s 租约未放);(4) 门没跑成 1 条 5%(tests/run.cx --list did not run,30.8 s);(5) HEAD 竞态 1 条 5%(cannot lock ref HEAD,他人提交落在 read-tree 与更新引用之间,27.0 s);(6) --registered 选行牵连他人行 1 条 5%(build/tool-index.tsv 的派生行,1.3 s)。派生物待重算没有造成任何一次失败(只出 pending-derived 提示);他人脏文件牵连只有第 (6) 条。
- S2 第一步落地(2026-11-15,窗口 fable-s2;tools/commit-own.cx,检查 tools/check-commit-own.cx 的 --cases s2 与 s2b 共 9 例,批次 commit-own-s2、commit-own-s2b):(a) 参数类:--help rc 0;--registered 拥有的表再给 --whole 或 --keyed 时丢弃该项并打 NOTE(只取登记集的键行,比原意更窄,他人行留在工作区),不再 rc 2;新增 --dry-run(私有索引上跑到提交钩子为止,不提交、不动共享索引,末行 COMMIT_OWN_DRY rc=N),写完即可先过门,不必等到提交末尾;(b) HEAD 竞态:git 报 cannot lock ref HEAD 且本提交各路径在新 HEAD 上未变时,在新 HEAD 上重建私有索引重试一次,路径有变则不重试并报 head-moved;(c) 最慢环节:pending-derived 提示(on-change --dry-run)由串行改为与提交并行、提交结束后读取,共享检出上同一组 4 个路径的 dry-run 实测 19.4 s 降到 15.8 s(各一次,后一次负载更高),相位行 pending_ms 由 5254 降到 1;(d) 可观测:每次打 COMMIT_OWN_PHASE(select_ms、pending_ms、regm_ms、commit_ms、attempts),拒绝时 stderr 先打 COMMIT_OWN_FAIL class=args、setup、registered-foreign-row、reg-manifest、precommit、head-moved、index-lock、nothing-to-commit、git-or-hook 之一,以后分类不用读文案。夹具仓 5 次小提交对比(check-commit-own --cases bench-1 至 bench-5,带真实 precommit 钩子,每次 50 s 上限,改前改后各 5 个作业并行,负载 5 至 28):改前 47.8、44.5、超时、10.4(rc 2)、超时,失败 3 次;改后 44.9、45.9、超时、超时、超时,失败 3 次,中位数未改善;原因是夹具仓里 on-change 冷启动约 30 s,改成并行后仍要等它读完,且原先 rc 2 快拒的一次变成真跑后超时。去掉 on-change 后同类单提交夹具用例 19 至 20 s,可见提示子进程是夹具里最慢的一环。未做:precommit 各门声明读集并标 NOT_AFFECTED(现状已按暂存路径选门,staged_cx 为 0 时不跑 lint 与棘轮,但不打 NOT_AFFECTED 行);私有索引 read-tree 与逐路径 update-index 约 6 s 未优化;提示等待上限 30 s 应缩短或改为不等;锁等待超时 20% 在 devsvc-core.cx 的 auto_lock(客户端被杀后服务侧租约 120 s 不放),属事务化 flow 在途写域,接口约定:git.commit 节点结束或客户端断开即放 git-index 与 registry 锁,等待方打印持锁属主与已持有秒数。tools/devsvc-usage.cx 的 HOOKWARN 改为只计 --hours 窗口内的行(首列 ISO 时间,非 ISO 行照旧计入;新增 --now 供检查),check-devsvc-usage 加 4 例。
- S2 剩余部分:门读集与同一提交内复用(2026-11-15,窗口 fable-gates;提交 87dba71d44 与 3f8a1aad99;检查 tools/check-precommit-gates.cx,批次 precommit-gates-na 7 例、precommit-gates-reuse 5 例)。测量做法:`sh ./unisacc.com tools/check-precommit-gates.cx -- --bench before|after`,固定私有夹具 ~/.cache/unisacc/check-precommit-gates/fx-bench(HEAD 归档加工作区的 precommit.cx 与 commit-own.cx,路径固定所以 -run 编译缓存跨次复用,先做一次热身提交),五次小提交经 commit-own 加真实 pre-commit 钩子;before 用 9a69f28dbc 的 precommit.cx 与 b94d1a83a2 的 commit-own.cx。机器负载:改前一轮 load 6.5 至 9.1,改后一轮 load 6.6 至 9.8,各只跑一轮,不是多轮统计。
- 关键路径(改前,墙钟 ms,括号内是 COMMIT_OWN_PHASE 的 regm_ms 与 commit_ms):纯文本 2480(0、1320);文档 2202(0、1392);一个 .cx 10744(0、10319,钩子里 ratchets 与 cx-lint 占满);登记集加整表 35536(14291、19472);登记集 29707(8750、19287)。最慢链是登记类提交:commit-own 先跑一遍 reg-manifest 约 8.4 至 14.3 s,钩子里 precommit 再跑一遍同一判定,加 tables-gen、cov_gate 两次 run.cx --list、runlist。只动 .cx 的提交关键路径是 ratchets 整门,约 8 至 10 s。真实仓一次登记提交实测(87dba71d44 自身):regm_ms 9038、commit_ms 23114。
- 设计取舍 (a) 门读集:precommit 里按暂存路径触发的六道门(manifest、tables、runlist、facts、ledger、cx)原本各自写死 git diff 的 pathspec,现改读 tests/precommit-gates.tsv(列:gate、diff-filter、pathspec、说明),`precommit.cx -- --gates` 打印生效读集。不相交的门记 NOT_AFFECTED,不跑也不算 PASS。计数没有放进 PRECOMMIT 行,而是新增一行 PRECOMMIT_GATES(not_affected=N、na=名单、source=table 或 builtin):check-precommit-flow 与 integrate-patch 对 PRECOMMIT 行做整行相等或前缀匹配,改该行会破坏现有消费者,而那两个文件不在本单写域。表缺失(仅 ENOENT,夹具仓与旧归档)回退内置同内容表,检查用 --gates 对拍两者逐行相同;表在但缺某门的行,该门按失败处理不放行。冻结门读集在 build/freeze.tsv 自身,不入此表。这一步本身不提速:各门原来就按路径条件触发,它只是把读集变成可声明、可计数、可查。
- 设计取舍 (b) 复用:commit-own --registered 在私有索引上跑完 reg-manifest 且通过后,把退出码与输出连同键写进 sy_tmpdir 下 commit-own-PID.gate,经环境变量 UNISACC_GATE_REUSE 传给提交钩子;precommit 的 manifest 门重算键,逐字相同才用记录,否则打印 key mismatch 并重跑。键 = 暂存树 sha(git write-tree)、门名、工具源 blob sha(git hash-object)、完整 argv。记录在该次提交结束时删除,重试(HEAD 被推进后重建索引)时重写;不跨提交、不跨进程缓存结论。commit-own 走原生 reg-manifest 时 argv 不同,不写记录,钩子照常重跑。方向与简报相反(由先跑的 commit-own 产出、后跑的 precommit 复用),因为实际提交链路里 precommit-flow 不在钩子路径上,先跑的是 commit-own。precommit-flow.cx 未改:复用在 --plan 与 --from-run 两遍里同样命中,manifest 不再成为 flow 节点。
- 结果(同一夹具同五次提交,墙钟 ms):只做 (a) 时 1799、2435、9715、25906、26615,与改前差异在噪声内;(a) 加 (b) 时 1717、1995、8036、17542、17809。五次中位数 10744 -> 8036(中位数落在只动 .cx 的那次,它不经过复用路径,这 2.7 s 差应视为负载噪声而不是改进)。可归因的改进在两次登记类提交:35536 -> 17542、29707 -> 17809,钩子相位 commit_ms 19472 -> 7402、19287 -> 7725,即少跑一遍 reg-manifest 加其冷启动,每次约 12 s;regm_ms 本身不变(8438、8450)。
- 未做与未过:(1) check-commit-own 的 registered、a、lock 三组在本机当前状态下超 55 s 预算(registered 首例 FAIL 且整组 57 s;a 组单独跑、load 低于 5 时 59 s 被截,用户态 54 s,该组不经过本次改动的任何代码路径;lock 组与另两组并发时 58 s 无输出),这三组不记 PASS;同一 --registered 三集步骤在固定夹具上用改后的 commit-own 实跑 rc 0、11 s、判定各项符合该用例预期。根因判断(未证实):克隆夹具路径带 pid,编译缓存每次冷,加上他人在途的 tools/cxlib 改动使原生缓存键失效;建议该检查改用固定私有副本。s2 与 s2b 两组在改后实跑 failed=0。(2) 真实仓改后一次登记提交(3f8a1aad99)总耗时 44.4 s,相位行未留存,不据此宣称真实仓改善。(3) ratchets 13 项改前改后各实跑 ok=13。
- (c) 最慢单门的分片方案(未实现):ratchets 整门现在只要暂存了任何 .cx 就 13 项全跑。方案一,逐棘轮读集:在 tests/precommit-gates.tsv 加 rk-NAME 行,例如 rk-coreapi 的读集是 tests/cx-module-freeze.tsv 列出的冻结模块加 tests/core-interface.tsv,只动 tools/ 下 .cx 的提交对它 NOT_AFFECTED;precommit 只对暂存路径归责,所以读集外的棘轮不可能产生本次提交的拦截行,判定强度对相交部分不变。落地需要 ratchets.cx 的 --from-run 接受缺席节点并在 RATCHETS 行单列 not_affected 计数(不并入 ok),该文件不在本单写域。方案二,coreapi 内部分片:按冻结模块分组,每组一个 flow 节点各自跑 symindex 与签名比对,结果行合并后由同一份 judge 判定;6.4 s 的单门预计降到最慢一组的耗时,收益上限受模块数与每进程约 1 s 启动开销限制,须先量 coreapi 内部各阶段耗时再定是否值得。方案一覆盖最常见的只改工具类提交,优先。
- S3 提交即 flow 第一阶段(2026-11-15,窗口 fable-gates2,工单 fgates3;检查 tools/check-precommit-flow.cx 的 --cases route,批次 precommit-flow-route 8 例)。测量做法:真实仓上用私有索引(HEAD 加一个 tools 下 .cx 注释加 tests/coverage.tsv 注释行,六道门里四道相交)直接跑钩子所调的 precommit,不提交;逐门耗时取 flow 运行目录的 events.tsv。关键路径(改前,load 6 至 14):串行 precommit 23.9、23.5、23.4、22.5 s;逐门(并发下,ms)manifest 9287、rk-coreapi 6683、rk-aluiwidth 4396、rk-cxlibc 2449、tables 2196、rk-net-deps 1188、其余各小于 1000;cov_gate 的 run.cx --list 约 4 s,不是 flow 节点,在取计划与判定两遍里各跑一次。只动一个 .cx 的索引:串行 7574 ms、precommit-flow 8492 ms(ratchets 自身已并发,关键路径是 coreapi 单门,flow 无收益)。旧 precommit-flow 在登记类索引上 21.8 s(plan 4726、flow 10865、judge 5739),即 R3 所说的没提速,原因是 cov_gate 重复两遍共约 9 s 的串行段;另有一次 30.3 s 与一次 27.8 s 是 precommit-flow 源改动后的原生冷构建,约多 11 至 17 s。重复段:commit-own --registered 先跑 reg-manifest 约 9 s,钩子再跑同一判定(3f8a1aad99 已做同提交复用);cov_gate 两遍(本阶段消除)。
- S3 第一阶段做了什么:(a) cov_gate 的索引侧求值成为 flow 节点 coveval(30 s 上限不变,求值根放运行目录,判定代码与串行共用,HEAD 侧求值只在命中时由判定那一遍内联跑);(b) 钩子所调的无参数 precommit 在暂存路径命中 manifest、tables、runlist 任一读集时转交 precommit-flow,各门并发,判定仍只在 precommit 的 --from-run 一遍里;只动 .cx 或文档的提交留在串行路径。钩子文件不变(仓内没有钩子安装脚本,钩子是未跟踪的三行 sh),直接 git commit 与 commit-own 走同一入口同一批门。(c) precommit-flow 加 --verdict FILE:判定那一遍跑完才写 RC 与退出码;precommit 只在读到与退出码一致的记录时采信,否则(没编过、flow 引擎失败、被杀)打一行 flow path unusable 并在同一进程内串行重跑同一批门,不存在少跑的出口。UNISACC_PRECOMMIT_SERIAL=1 强制串行,供对拍。不缓存结论:每次都实跑,flow 运行目录只在本次调用内读。
- S3 第一阶段结果:真实仓同一私有索引交替各三次(load 5 至 8),强制串行 23523、23352、22523 ms,新主路径 11200、11047、10891 ms(plan 约 0.4 s、flow 约 10 s 即 manifest 单门、judge 约 0.3 s),PRECOMMIT 与 PRECOMMIT_GATES 行逐字相同。夹具 bench(check-precommit-gates --bench after,各一轮五次提交,不是多轮统计;改前用 UNISACC_PRECOMMIT_SERIAL=1 走同一份代码的串行路径,load 15 与 12):墙钟 ms 1700、1801、33907、39457、20040 对 1672、1753、8751、29484、19278;钩子相位 commit_ms 821、1032、33573、17937、9095 对 832、857、8434、18340、8399。如实说明:第三次(只动 .cx)两轮都走串行路径,33.9 s 对 8.8 s 是夹具重建后的冷编译,不是改进;第四次总耗时的差在 select_ms(6478 对 1339)与 regm_ms(14405 对 9177),也不归本次改动;两次登记类提交的钩子相位在夹具里没有可见改善(17937 对 18340、9095 对 8399),因为 commit-own 已先跑 reg-manifest 并被复用,钩子里剩下 tables、coveval、runlist 三个短节点,并发收益被 precommit-flow 自身启动与一次冷构建抵消。所以本阶段可归因的收益只在不经 commit-own 复用的路径(直接 git commit、commit-own 的 --whole 或 --keyed 带登记表、manifest 复用键不命中)上成立,约 23 s 降到 11 s;经 --registered 的提交本阶段不变。
- S3 发现的问题(未修,不在本单写域或未证实):(1) 真实仓上往 tests/batches.tsv 追加一行覆盖键不存在的批次,串行与 flow 两条路径都判 blocked=0,登记配对闸没有拦;check-precommit-coverage 的夹具里同类情形能拦。根因已实测:按 cov_eval 的做法搭求值根(顶层与 tests 下条目软链回工作树,只有两张表与 run.cx 是实文件)后跑 run.cx --list,rc 2,输出仅一行 REFUSE metadata-schema source-not-regular row=1 id=modelcache-budget-lru,即第一行批次的源文件经软链不是常规文件就被拒,走不到 unknown-coverage 判定;cov_eval 见到 REFUSE 而无 unknown-coverage 即返回 0 放行。这段判定逻辑本次未改,串行路径同样放行,属既有缺口:登记配对闸在真实仓实际不生效,只在夹具里生效。修法待定(求值根改用硬拷贝或给 run.cx 加只核覆盖键的入口),须排单。真实仓本次提交 6db7ca9483 自身(登记集加三个 .cx 加文档,load 17):总 24104 ms,select_ms 1147、regm_ms 9613、commit_ms 9401,钩子内 flow 段 plan 388、flow 6974、judge 312 ms(前一次被 lint 拦下的同内容尝试);对照 87dba71d44 的 regm_ms 9038、commit_ms 23114,两次提交内容不同,只作参考。(2) 在共享仓上用私有索引对拍时,他人提交推进 HEAD 会让两次 git diff --cached 结果不同;route 场景已改用钉住 HEAD 的私有裸 git 目录。(3) check-precommit-gates 的 reuse 组与另两个检查三路并发时 58 s 超时,单跑 37.6 s 通过;check-precommit-flow 原四场景三路并发时 53.8 s,贴 55 s 预算。(4) 工作期间 devsvc 因他人在途改动一度编不过(undefined function compile_to)且实例消失,约一分钟后恢复,属 R1。
- S3 后续阶段排期:第二阶段,commit-own --registered 不再在提交前串行跑 reg-manifest,改为把它作为同一 flow 的 manifest 节点,拒绝类别从 flow 判定结果取,预计每次登记类提交再省约 9 s(regm_ms 整段);须保留无钩子时的判定。第三阶段,逐棘轮读集(上文方案一):只动 tools 或 tests 下 .cx 的提交对 rk-coreapi 记 NOT_AFFECTED,关键路径从 6.7 s 降到 rk-aluiwidth 的 4.4 s;ratchets.cx 当前有他人未提交改动(notcovered 棘轮),本阶段未碰。第四阶段,devsvc git commit 把选路径、暂存、门、提交、推送表达成一个经服务的事务化 flow,门节点事件带节点名进 devsvc-budget,precommit-flow 改为常驻服务内执行以消掉每次约 0.3 s 启动与源改动后的原生冷构建;钩子读同一 flow 的判定记录(键含暂存树 sha 与工具 blob sha,仅限同一提交)。第五阶段,manifest 单门 9 s 的内部分片与 derive --write 并发。
- (2026-11-15 cc 排期)doc 缺 create:新文件 doc set 被 ENOENT 拒绝,调用方只能先用 f write 建标题空壳;加 doc create FILE TITLE(按配置表 doc 行校验路径,已存在拒绝)。
- (2026-11-15 cc 排期)dcheck 选批与跑批分成两个后台作业:选批冷启动约 17 s 占掉整窗,选批结果落盘后各批一个节点并行,不共用 53 s 整窗。
- (2026-11-15 cc 排期)devsvc 空闲巡检回收自己的流程运行目录(/tmp/devsvc-flow-*,超 2 小时且无进程引用);今日手动清了 1297 个。
- devsvc 拥有索引,现状取证(2026-11-15,窗口 idx;实例 045c5a383068df3b 的 events.tsv,16:37 起约 5 小时,更早的未留存):git.commit 108 条,rc 非 0 共 40 条(rc 1 26 条、rc 2 13 条、rc 124 1 条)。其中 5 条 rc 1 的耗时是 50048 至 50068 ms,即等 git-index 锁到上限被拒,属被他人提交占锁牵连(12.5%);COMMIT_OWN_FAIL 留痕 18 条:args 9、precommit 8、registered-foreign-row 1。事件不留 stderr 正文,precommit 类 8 条里多少是他人内容所致无法从事件再分,上界 8 条(20%),故"被他人暂存或他人未提交内容牵连"在失败里的占比是 12.5% 至 32.5%,不是精确值。hook-warn.tsv 里 git-write 15 条(直接 git add 或 commit 的绕过)。
- 交叉污染来源(读码与实跑):(1) commit-own 早已用私有索引建提交,但钩子由 git commit 在共享工作树里跑,precommit 的 cov_eval 把索引里的两张表配上共享工作树的 tests 与其余目录软链成求值根,runlist 门直接在共享工作树跑 tests/run.cx --list,ratchets 与 lint 读共享工作树,门工具自身(precommit.cx、reg-manifest.cx、tables-gen.cx)也取共享工作树的未提交版本;今晚拦人的文案 run.cx --list on the staged tables refused 只存在于 covfix 尚未提交的 precommit.cx 工作区改动里,即一个窗口未提交的门改动对全员生效。(2) 直接 git add 加 git commit 走共享索引,他人暂存整个进提交或整个被门判。(3) cmd_git 的 git-index 锁把门求值与推送一起串行,后来者等到 50 s 被拒。(4) 提交后同步共享索引只动本请求路径,这一段没有发现污染。
- devsvc 拥有索引,第一阶段(已做,tools/commit-own.cx、tools/devsvc-core.cx 的 cmd_git、检查 tools/check-commit-tree-gate.cx 批次 commit-tree-gate 5 例):每个提交请求用私有索引(HEAD 加本请求路径)建树;钩子不再由 git commit 触发,而是把私有索引检出到 sy_tmpdir 下 commit-own-PID.root(不含 archive,约 5100 个文件,git 不跟踪的顶层项如 unisacc.com、out 从共享检出软链),以该根为 cwd 与 GIT_WORK_TREE 跑同一个 pre-commit 钩子,于是全部门(含门工具自身)读私有树;门过后 write-tree、commit-tree,只在 git update-ref HEAD NEW OLD 一步做比较并交换,不持任何锁;HEAD 被他人推进且本请求路径在新 HEAD 未变则整遍重放(门重跑,不复用旧结论),最多 3 次,变了则报 COMMIT_OWN_CONFLICT path= by=,类别仍是 head-moved;cmd_git 不再取 git-index 锁,推送在提交之后不持锁;共享索引同步仍只动本请求路径。直接 git commit 仍由同一钩子在共享索引上判(检查 direct-git-blocked)。
- 第一阶段实测:夹具(钩子睡 1 s 并拒坏表行)3 个不重叠提交并发各 5 轮,15 次全部成功,每轮墙钟 10172(首轮含编译)、3672、3708、3725、3723 ms;因为重放要重跑门,第 2、3 个提交各多付一到两遍门时间,首个提交不等任何人。改前同夹具的数未取到:旧版 commit-own 拷出 tools 目录后解析不到模块,又不允许另建检出,所以改前只有上面事件历史里的 37% 失败率与 5 条等锁超时可比。真实仓对 tools/commit-own.cx 自身 dry-run 一次:私有根上 ratchets 与 lint 全跑,commit_ms 38153(load 约 40),blocked=0。既有检查:check-commit-own 的 s2b、reg-refuse、foreign、nolock、hookfail 各 failed=0;s2 组有 2 例失败(phase-line、fail-class-precommit),原因是 devsvc run 的 DEVSVC_RUN 结果行排在末行而用例取 tail -1,设 DEVSVC_RUN_QUIET=1 后整组 failed=0,与本单无关;reg-a 一例失败且整组 52.7 s,原因未查明,该组不在此前取证 PASS 的 8 批里;check-devsvc-git failed=0。
- 第一阶段已知缺口:(1) 重放重跑整套门,3 个并发提交的总 CPU 约为串行的 2 倍,墙钟收益只在首个提交;(2) 私有根每次整树检出约 1.3 s、155 MB,超时被杀时根目录残留在 sy_tmpdir;(3) commit-tree 不触发 commit-msg、post-commit 钩子,仓里目前只有 pre-commit 与 pre-push;(4) 未登记的顶层目录仍从共享检出软链,他人未跟踪的顶层新目录可被门读到;(5) --registered 的 reg-manifest 预检仍在共享工作树里起工具(只读私有索引)。
- 第一阶段落地提交 8b61e66fe1 与 54c010cedc(均经 devsvc git commit,门在私有根上跑):只含 .cx 与文档的一笔 commit_ms 37651;带 --registered 加一个新 .cx 的一笔 regm_ms 10132、commit_ms 44885,整个 x 节点 58388 ms,贴着 devsvc x 的 60 s 上限(同一组合前一次在 60007 ms 被截,rc 124,没有提交),这是阶段 2 (b)(c) 要先解决的:登记类提交在私有根上的门时间必须降到 40 s 以内。单独提交新检查源文件被 new check missing batch registration 拦下,说明门确在判本请求自己的树。
- devsvc 拥有索引,第二阶段排期:(a) 提交请求进服务内队列:devsvc git commit 把路径集、说明、登记键交给常驻实例,实例按路径集分冲突域,落 ref 的比较并交换由实例单点执行,客户端只等结果;(b) 重放不重跑的判据:按 tests/precommit-gates.tsv 的门读集,HEAD 推进的路径与本请求触发的门读集不相交时只重建树不重跑门,相交才重跑;freeze、tlib、new_tools 三道未入表的门先补读集再启用;(c) 私有根按 HEAD 树缓存一份只读底,每个请求只覆盖自己的路径,去掉每次整树检出;(d) devsvc git status 给存量暂存出归属行(按写域锁、工单写域、最近 f.edit 事件的 owner 归属),devsvc git release OWNER 经事务化 flow 只释放归属明确者(把这些路径的共享索引项还原成 HEAD 的 blob,工作树不动);(e) 钩子侧:宿主钩子拦到直接 git add 或 git commit 时打印 DEVSVC-WARN 提示改用 devsvc git commit,pre-commit 钩子在共享索引上发现非本请求路径的暂存时具名列出;(f) 推送成为独立 flow 节点,多个已落提交合并一次推送;(g) 实例拥有共享索引的唯一写权,共享索引不再是任何门的输入之后,把它降为仅供 git status 显示的派生物。
- (2026-11-15 cc 排期)启动器自身写入保护:写 tools/devsvc.cx 前强制编译加 ping 通过才落盘(中间态曾使全员入口编译失败)。



- (14) 公开接口改动后整体派生仍可能超过 55 秒:np 双门面迁移使 layer-api、iface-hash、product-map、tool-index、tables-gen、batch-packages-gen、links、doc-links 进入闭包;两次派生阶段在 55806/55654 ms 退出 rc124、stage=none,第二次低载仍复现。分相只隔开提交阶段,尚未把派生本身按表分相。临时按表在同一私有输入树实际构造并全量检查,绑定每表前后身份,再经收据复核提交;不延时限、不跳表。后续单:派生按表调度并行、接口哈希按机械读集只重算受影响消费者,失败实时指明表名与可恢复身份。证据入口 /tmp/npbuf-cdx 与 devsvc jobs/21357621022-2539683。

## 7 路线

- lint:把十三个棘轮与 precommit 预演做成服务内的增量检查,按变更路径触发。
- 关系 DAG:常驻持有 `build/links.tsv` 与 use 边,答"改这个路径影响谁",取代 affected 与 on-change 各自冷算。
- 记忆宫殿视图:把 `prd_mempalace.md` 的房间解析成可查询的表,经 API 读写单个房间。
- lsp:符号索引(`symindex`)常驻,提供定义与引用查询。
- flow:把"改 -> 派生 -> 选批 -> 提交门"串成事件驱动的流。
关系 DAG 实施单 G1 至 G12(设计 2026-11-15,原只在子代理回报中,cc 补入库;状态:G1 1eef23043a、G2 720b8c2d53、G3 91d5a298e9 已落地,G4/G5 rel who-uses/impact 已有待按验收复核,另 0f54397986 加 who-calls 文本级符号调用点不属本表):
| G1 | 抽取器(use、include、module),复用 `cxlib.cxroot` 解析;先定因探针段错误 | `tools/cxlib/relscan.cx`、`tools/check-relscan.cx` | 现行 `.cx` 的 use 边数为 5623(口径同探针);边表落盘成功 |
| G2 | 缓存存取与作废(三个文件、抽取器 sha) | `tools/cxlib/relstore.cx`、其 check | 重建两次字节相同;改抽取器 sha 后整体重建 |
| G3 | 装入已有表(links、table-index、tool-index、table-deps) | `tools/cxlib/relload.cx`、其 check | batch 边 1110、derive 边 45 |
| G4 | 反向闭包与 `who-uses FILE` | `tools/rel.cx`、`tools/check-rel.cx` | `cx/sys/fs.cx` 得 976 个使用者,与 `affected.cx --users` 逐行相同 |
| G5 | `impact`(批次部分) | `tools/rel.cx`、其 check | `cx/sys/fs.cx` 得 978 批,与 `affected --list-only` 逐行相同;commit 档 `tools/derive.cx` 得 4 批 |
| G6 | `impact` 加表与 flow 节点 | 同上 | `tests/manifests/x` 命中 tables-gen 的两张表 |
| G7 | 车道表与 `owner` | `exec/facts/lanes.tsv`、`tools/rel.cx` | `apps/csih/tui.cx` 归 csih;未登记路径 `lane=-` |
| G8 | `path A B` | `tools/rel.cx` | 已知两跳路径给出 3 行;不可达 rc 1 |
| G9 | 单文件增量更新 | `tools/cxlib/relstore.cx` | 改一个文件后增量结果与全量重建字节相同 |
| G10 | 接入 devsvc 事件与常驻内存索引 | `tools/devsvc.cx` 的 rel 子命令 | 等基础版合入后排窗口 |
| G11 | `affected.cx` 改问 rel(保留原命令行合同) | `tools/affected.cx` | `check-affected.cx` 全部批次通过;全批次逐行对照无差 |
| G12 | 登记批次与 prd 能力速查 | 清单、`prd.md` | `tests/run.cx -- --list` 含新批次 |
吸收自 omp-flow 的候选(2026-11-15,窗口 fable-ompflow;对比依据为 moltbaby 的 omp-workflow/flow-run.ts、flow-cli.ts、flow-deadline.ts、node-flow.ts 只读源码。devsvc flow 已强于 omp-flow 而不需吸收的:写算子与路径锁、就绪即起的调度(omp-flow 按层等齐)、事件落盘与 run 目录、外部取消、后台作业与 wait、输出控制、整流 deadline、stdin_node 管道、compare 与 doc 算子):
- O1 速记与 map 展开(价值最高,已落第一步):omp-flow 的 `sh c1 c2`、`read a b:3-9`、`map` 一条命令并发 N 个节点,不写图文件;devsvc 侧原先要 printf 拼 TAB 并手工写 %3B。第一步 `tools/flowsh.cx`(批次 flowsh)只生成节点表,接 `devsvc flow - --cat`;第二步在 devsvc-core.cx 加 `flow sh|read|map` 子命令内联生成(省一次进程启动,约 0.3 s),须等事务化 flow 在途单放开 devsvc-core.cx 写域。难度低。
- O2 并发收益行:omp-flow 首行给 wallMs、serialMs、savedMs。落地:flowexec.cx 的 fl_sum 加 serial_ms(各节点 ms 之和),DEVSVC_FLOW 与 FLOW_RUN 末行加 serial_ms= 与 saved_ms=,agent 据此判断拆并发是否值得。难度低;碰 flowexec.cx 与 devsvc-core.cx,排在途单之后。
- O3 失败节点自带首条诊断:omp-flow 文本行在失败节点后附 exit= 与首个非空输出行;devsvc 的 NODE 行只有 out= err= 字节数,要再跑一次 --cat 或 --summary 才知原因。落地:flowexec.cx 的 fl_finish 在 fail 与 timeout 时读 RUN/out/ID.err(空则 .out)首个非空行,截 120 字节并进事件摘要。价值:每次失败省一次往返。难度低;只碰 flowexec.cx,与 git.commit 节点单同文件,须先 devsvc git status 确认无他人未提交改动。
- O4 结构化整图报告:omp-flow 默认输出一份 JSON(每节点 id、tool、status、ms、exitCode、content)。落地:fl_run 结束时在 RUN/report.tsv 落每节点一行(id、op、state、rc、ms、out 字节、err 字节、out sha),devsvc flow 加 --report 打印它;协调者与 API 不再解析 NODE 文本行。难度中;flowexec.cx 加 devsvc-core.cx。与事务化 flow 互补(其回滚与提交结论可作为 report 的附加行),字段须与该单对齐后再做。
- O5 只读闸门:omp-flow 默认只放行可证只读的节点,--allow-write 才放行写。落地:`flow --ro`,fl_load 拒绝 file.write、file.edit、doc 写算子、stdout_to 与带 lock 的节点(exec 无法证只读,只作声明并记事件 ro=1);只读流不取任何锁。价值:审查与取证子代理可放心满并发,不会误写共享检出。难度低;flowexec.cx 加 devsvc-core.cx 选项,排在途单之后。
- O6 glob 算子:omp-flow 有进程内 glob 节点(按字典序列文件名);5a 节自认只列文件名仍要用原生工具。落地:flowfile.cx 加 `glob`(pattern=、max=,复用 search 的 glob 匹配器,跳过 .git),flowexec.cx 的 op 白名单加一项。难度低到中;flowfile.cx 不在在途写域。
- O7 跳过文案指向动作:omp-flow 的 skipped 写明"不是本调用自身的失败,先处理前驱再重发";devsvc 的 skip 摘要只有 dep=ID:state。落地:DEVSVC_FLOW 末行在有 skip 时加 first_fail=ID,让 agent 直接定位根因节点。难度低;devsvc-core.cx,排后。
- 不吸收:(1) 批内共享预算(flow-deadline.ts),devsvc 已有 --deadline-ms 与节点 timeout_ms;(2) agentbox 节点与模型侧 flow 协议(node-flow.ts、16 项上限),属 omp 宿主机制;(3) JSON 图文件输入,TSV 节点表加 flowsh 已够,cx 无 JSON 库,不为此引入。
- O3 已落第一步(2026-11-15):flowexec.cx 的 fl_finish 对 fail 或 timeout 的 exec 节点在事件摘要末尾加 ` first=TEXT`(stderr 首个非空行,fail 且 stderr 为空时取 stdout;至多 120 字节,控制字节换空格);devsvc 的 NODE 行直接带出,不必再 --cat。check-flowrun 的 fail-rc-and-stderr 收紧为整行匹配。

## 8 文档接口 doc(一期,2026-11-15)

目标:agent 不再直接改 prd*.md、plans/*.md,只经 doc 接口表达意图(取哪节、替换哪节、追加哪行),取锁、原子写、并发校验由服务做。

- 库 `tools/cxlib/flowdoc.cx`(`fd_run(op, args, ff_out*)`):节路径 `A/B/C` 按 Markdown 标题逐层嵌套匹配(标题去首尾空白,代码块内 `#` 行不算标题),找不到 ENOSEC,歧义 EMULTISEC。标题本身含 `/` 时写成 `%2F`,含 `%` 写成 `%25`(2026-11-17);doc.list 输出同样编码,list 的路径可原样喂回 get/set。
  - doc.get 取节(含标题行,sha 为全文件摘要);加 body=1(CLI `--body`)只取正文,不含标题行,且去掉 set 会补回的那一个节尾空行,于是 `doc get --body` 的输出原样 `doc set --from` 回写字节不变。doc.list 列节树 `层级<TAB>路径`。
  - doc.set 整节正文替换(标题保留);正文首个非空行就是本节标题行(把含标题的 get 全文回写)时拒绝 EDUPHEAD,不再静默生成重复标题。两者都做:get --body 给出对称形式,EDUPHEAD 堵住旧误用,两者互不替代。doc.append 节尾(尾随空行之前)追加一行;doc.remove 按唯一子串删节内一行。
  - doc.repair 节路径末段允许多处同名命中(其余段仍须唯一):取第一处,把同父范围内同层同标题的后续标题行删掉,正文并入第一处;无可合并的报 ENODUP。用于修复旧版回写造成的重复根标题(此前 get/set 一律 EMULTISEC,无入口可修)。
  - doc.mermaid 在节内 mermaid 块里:old 唯一则替换为 new(改节点文字);old 空则在首块闭合前加一行 new(加连线);new 空则删含 old 的行(删连线)。
  - 写入一律经 file.write:原子写、根越界 EOUTSIDE;带读入时的 sha 写,期间他人改过即 ESHA;调用者给的 expect_sha 不符同样 ESHA。
- 锁:库不取锁,只做 sha 兜底;flow 的 doc.* 写节点默认 lock=doc:<path>(同文件串行,不同文件并行);CLI 写操作内部取 devsvc 锁 doc:<文件>。调用者不感知。
- CLI:`devsvc doc get|set|append|remove|list|mermaid|repair FILE SECTION [k=v | k@=PATH ...] [--from F] [--body] [--expect-sha X]`;set 的 --from 为 content,其余为 line。
- 允许范围:只作用于 `tools/devsvc-repo.tsv` 的 doc 行(glob,* 不跨 /):prd.md、prd_*.md、plans/*.md;核心不写死路径。cfg_load 对不认识的行类型只告警一次不判坏表(向前兼容旧执行体)。
- 检查:`tools/check-flowdoc.cx`(批次 flowdoc,26 项)。
- 未做:CLI 与 flow doc 节点各自的专项检查项;入口按 HEAD 固定核心,doc 子命令在新核心入库后才对 `devsvc.cx` 生效。


## 9 flow 负载准入(2026-11-17)

- 规则:并发总量以机器负载不高于 10 为界(prd_mempalace.md 决定)。flow 的 exec 节点起跑前,除机器令牌(cxlib.flowadmit,容量按核数与内存,load1 超两倍核数减半)外,再过负载门:load1 高于阈值时本次运行不放行任何新 exec 节点,已在跑的不动,load1 回落到阈值及以下即放行。
- 有界:每次 flow 运行因负载推迟的累计时间有上限,到期照常放行并在回执标记;运行总 deadline 仍然有效,不会无界等待。
- 配置(`tools/devsvc-repo.tsv`,按行首键精确匹配):`admit_load_max`(阈值,缺省 10)、`admit_load_wait_ms`(每次运行推迟总上限毫秒,缺省 5000)。表来源 `$DEVSVC_REPO_TABLE`,否则当前目录下的 `tools/devsvc-repo.tsv`;测试缝 `DEVSVC_LOAD_MAX`、`DEVSVC_LOAD_WAIT_MS`、`DEVSVC_LOADAVG`。
- 留痕:每次决策在 run 目录 events.tsv 写一行 `ADMIT<TAB>defer|go|timeout<TAB>load1=L<TAB>max=M<TAB>waited_ms=W`(不占 EVT 序号);`DEVSVC_FLOW` 回执新增 `admit=ok|deferred|timeout` 与 `load_defer=N`,推迟时长计入 `admit_wait_ms`。
- 调用方不再自写临时节点记 load1。检查:`tools/check-flowadmit.cx` 的 load-* 七项(批次 flowadmit-a)。

## 10 派生表现算现查 R1 试点:tool-index 与 doc-links(2026-11-18)

目标:把 build/tool-index.tsv、build/doc-links.tsv 移出提交门负担,改由 devsvc 按内容身份现算现查;读者拿到的内容恒与当前源树一致,判定强度不降。

用法:`sh ./unisacc.com tools/devsvc.cx -- tool-index [--sha] [--key] [--cache DIR] [--timeout-ms N]`(转 `tools/derived-query.cx tool-index`)。正文到 stdout,结果行 `DQ tool-index hit|miss|recovered key= sha= bytes= ms=` 到 stderr。身份 = tools/、tools/cxlib/ 全部目录项与 tests/batches.tsv 的内容 sha 清单,外加实际执行的生成器 tools/tool-index.cx 的 sha(读集是 build_tools 读集的超集,多算只多失效)。缓存 `~/.cache/unisacc/derived/tool-index/<key>.tsv` 加 `<key>.sha`;命中时重核正文 sha,不符即删并现算(recovered);现算前后身份不同(源树在途被改)不入缓存,rc 3。生成器新增 `tool-index.cx --emit-tools PATH`,与 `--write` 同一正文,不碰 build/。检查 `tools/check-derived-query.cx`(批次 derived-query,九项;负控:把命中分支改成不核 sha 后 corrupt 与 sidecar 两项先 FAIL,还原后全 PASS)。

度量(2026-11-18,经 devsvc x 实跑,命令 `devsvc x --par 1 -- sh ./unisacc.com tools/derive.cx -- --check --only NAME`):tool-index 三次 7773/7854/7772 ms(中位 7773);doc-links 三次 1136/1137/1116 ms(中位 1136)。近 7 天提交数(`devsvc git log --since='7 days ago' --oneline -- PATH`):全仓 2010,build/ 617,tool-index.tsv 222(11.0%),doc-links.tsv 3,两者并集 222。现查:冷 4.5-7.2 s(一次),热命中 124-183 ms。

读者分类(`git grep -n 'build/tool-index.tsv\|build/doc-links.tsv'`):
- (甲) 只读查询:AGENTS.md/CLAUDE.md 与 prd.md 的索引指引(人和代理查工具);plans/notcovered-plan.md、plans/cx-lib-roadmap 的引用;tools/archive-candidates.cx(问"索引是否点名该工具")。prd.md 指引已切到 devsvc tool-index;archive-candidates 与 CLAUDE.md 未切。
- (乙) 门与检查当事实核对:tools/derive.cx 的 tool-index、doc-links 行(exec/facts/derive.tsv、build/links.tsv 的 derive 边、tools/derive-inputs.tsv),tools/commit-own.cx 的 --derive 与键选表名单,tools/check-tool-index.cx、check-doc-links-gen.cx、check-commit-own.cx(夹具根内,与仓内文件无关),tests/coverage.tsv、tests/manifests 文案。
- (丙) 必须保留文件:未发现构建 unisacc.com 或离线流程读这两张表;tools/cxlib/relload.cx 的 RL_TOOL_INDEX 只认格式,调用方传路径(仅 check-relload 夹具);build/table-index.tsv 的自述行与 .devsvc/ 双写副本属 tool-index.cx 自身。

迁移(每步可回退,回退即还原该步提交):
(1) 并行期(本次已做):新增查询接口与缓存;文件副本与 derive 门不变;(甲)读者逐个切到查询,对拍 sha 与 `tool-index.cx --emit-tools` / `--write` 一致。
(2) 停写:commit-own --derive 与 derive 的 tool-index 行改为不进提交门(derive.tsv 标记 query-only,--check 不再核文件,改核"查询输出 = 现算",由查询接口身份保证);build/tool-index.tsv 加入 .gitignore 前先 git rm --cached,本地需要文件者 `derive --write --only tool-index`。回退:恢复 derive.tsv 行与跟踪。
(3) 移除:table-index.tsv 分开处理(它也由 tool-index.cx 写,读集含 build/*,另做 table-index 查询再迁);doc-links 同法补 `devsvc doc-links`(读集 *.md 加 baseline 加 doclink.cx)。
收益估算:每次触及 tools/ 或 tests/batches.tsv 的提交门省 tool-index 约 7.8 s(并行 derive 下省的是该行占最慢行的份额,单独 --only 时全省),doc-links 约 1.1 s;build/ 提交数按近 7 天估少 222 次中只改该表的那部分(上限 222/617 = 36%,与 batches.tsv 等同改的提交仍会动 build/)。
风险:身份清单每次查询读 1100+ 文件(约 100 ms);缓存跨工作树共享(按内容键,安全);unstable 时读者须重查。

### R1 第二步:查询与物化分离

`derive.tsv` 的 `write_argv` 以 `query-only ` 开头时,该行不进入自动 `--write` 与写计划闭包,也不产生自动物化关系边。显式 `--check` 仍执行其独立现算校验,`--write --only NAME` 可生成本地导出。查询缓存只存构造物,不存检查结论。tool-index 的校验走 `derived-query.cx tool-index --check`:命中时也独立调用原生成器,逐字节比较正文,并在输出前再次核源树身份。

table-index 是独立物化项,保留本地表和元数据副本逐字节检查。tool-index 本地导出不再由 `--registered` 自动选入提交;新工具登记事实仍由原门检查。回退先恢复 derive 两行与登记表名单,再恢复 tool-index 文件跟踪。历史夹具所验证的离线生成器写入与篡改拒绝合同仍保留。

基线入口 `/tmp/r1-baseline`:低载两项检查总 34641 ms,product-map 23106 ms,tool-index 11535 ms(表索引副本漂移,该项非 PASS)。7 天全仓提交 1998,build/ 622,tool-index 225,product-map 43。旧整体派生和 reg-a/derive-reg 的超窗均保留 UNKNOWN,后测不得改写为基线 PASS。新查询模式与原模式的负控入口 `/tmp/r1-query-red` 和 `/tmp/r1-query-check`。

### product-map 查询构造与读集核验

tools/product-map-query.cx 的清单真源是 tools/product-map-inputs.tsv。缓存身份包含清单本身、目录列举与文件内容、两个根的实际编译器与生成器、完整 Git 跟踪名称集合和实际观察到的读取清单。每次命中重新核内容哈希及缓存正文摘要,不缓存检查结论;显式 --check 独立执行原生成器再逐字节对拍。仅用于 Linux 开发机,不代表六目标平台资格。

strace 只在缺失构造与独立核验路径调用,命中分支不查找也不执行它。未安装时输出 PRODUCT_MAP_READSET_NEEDS_STRACE 并非零退出;路径权限错误另行命名拒绝。检查在私有 PATH 中先构造缓存、移走 strace 链接,证明命中仍可用而冷查询命名拒绝。旧实现该负控失败,新实现通过。替代方向是统一 cx_read、stat、目录列举入口生成读取回执,并与编译器 use 闭包及子进程回执合并;只包装主进程的 cx_read 不足以覆盖 census 子进程,替代方案须保留相同未声明读取负控。

product-map --root 选择被分析的数据,分析子程序由当前已装编译器引导。目标 APE 的字节作为数据读取,不会再因外壳保留字节变化而另编同一分析程序。旧七条 flow-model 留存值已经先迁为显式事实,正常生成物前后逐字节相同。六类变更以独立现算验收,常量缓存键故障实现六项均被拦;正文、读取清单、摘要损坏与正文缺失分别保留恢复断言。热命中已测约 1.45 秒,不宣称达到百毫秒,后续先量哈希主耗时再优化。

登记与执行分相:现有 register --no-run 仅保存合同并返回 REGISTERED_NORUN,只算准备;之后经标准 affected 入口逐批实跑才记 PASS。登记尾段仍有缺口:批次耗时接近 55 秒时,试跑已经通过,60 秒外层仍可能在 packages 与 manifest 收尾前杀掉进程,且当前提前打印 REGISTERED。应拆成独立收尾阶段并在完整收尾后打印终态;不得延长时限。本轮缺失 manifest 按三张真实表逐字节投影补齐,不另造登记逻辑,并由 reg-manifest 检查闭合。

### R1 停写与验收边界

tool-index 与 product-map 已标 query-only:自动 --derive 不物化,显式 --check 走查询与独立现算逐字节对拍,显式 --write --only 保留本地导出。两个 build/ 导出均停跟踪并忽略,本地字节保留。当前产品全景经 devsvc product-map 获取,文档生成块只作显式快照。日常形状检查读查询,四个离线快照场景用归档输入显式准备后保留原断言。显式 --write --package 同步七条 flow-model 测量事实,默认生成与检查不写事实。

回退时恢复对应 derive.tsv 的普通物化行,显式生成导出并重新跟踪,再按原检查核对；查询与生成器没有各留一套计算逻辑。读取核验的 strace 依赖只属于 Linux 冷构造和独立 --check,命中不依赖它。损坏恢复会删除当前引用的失效正文及旁文件后重算,归档夹具清除继承的 Git 路由变量,不克隆开发检出。

度量入口为 release/evidence/r1-final-2026-11-18.tsv。旧整体派生 stage=none 在 55534 ms 被杀,保留 UNKNOWN。迁移后真实提交 750e63b581 的派生29516 ms、提交阶段内容身份复核41 ms、含门禁与推送总44373 ms；输入不同,只报告各自实测,不把差值当严格同输入加速比。7天 build/ 历史提交数从622变628,工具索引225变226、产品全景43变44,增加包含本次退役提交,不能宣称旧历史已减少；停写后不再由这两张表触发自动快照提交。两项夹具仍以最终独立实跑回执为准。

## 11 按域选择测试 R3-C

affected 的 --domain NAME 可重复取并集,再与 --files、--since、--ids、--layer、tier 和宿主限制取交集;也可单独列出指定域。--domain-summary 分别输出 registered、eligible、selected,最后注明 selection-only/no-test-result。完整资格继续按对应清单与实际执行证据判断,筛掉的批次保留未执行状态。

域事实仍由声明、tests/batch-domain-rules.tsv 和 tests/batch-domains.tsv 的少数例外派生到 tests/batch-domain-index.tsv。启用域选项时实际运行生成器 --check --dump,命名拒绝缺声明、冲突和陈旧索引;默认路径不读取域索引。新三批检查全部自动推断,显式例外仍为108行。默认两个典型输入与旧程序逐字节对拍,产品域结果另与全量结果按域投影逐字节核对,证据为 release/evidence/r3-domain-2026-11-18.tsv。

检查的准备相与选择运行相分别有界。真实kit、runner、域生成器与选择器按实际argv并行准备,每个子调用仍25秒,usage输出直接从进程管道捕获并持续排空,只记PREPARED;选择运行仍真实执行。无需把日志读回或用cat转换来源,无需放宽R4。取消筛选的故障实现被五条反例拒绝。整轮测试墙钟未作典型全集对照,标UNMEASURED;选择查询包含新域校验成本,本次不宣称查询加速。

两项后续准备成本需要单列:(1) 同根相对runner调用准备后,绝对runner首次仍实测16697ms;现有预热收据只含文件/产品摘要与缓存目录,后续应统一实际调用形态并给命中做负控,保留路径影响__FILE__等语义的合同。(2) register --no-run冷态仍先准备原生run与run-batch,本次49203ms;可把纯元数据登记与原生runner准备分相,登记校验继续实际执行,准备单独记PREPARED,后续批次必须实跑。

## 12 合并队列 R2:最小第一步设计

状态:降为回退方案,先做读集归因与收据绑读集内容身份(见 prd_mempalace.md 决定)。

第一步已批准,尚未实现。目标是减少同一 ref 的竞争重放和回执误归属。沿用现有私有索引、原门禁、gitsync 与 flow;调用者仍只交提交意图,不管理锁、收据或私有目录。第一步在 devsvc git commit 增加可回退的 opt-in 队列,同一 Git common-dir 加目标 ref 同时只执行一笔完整提交事务;不同 ref 与只读工作继续并行。先接受这处串行换取零自发 CAS 重放,测过吞吐再扩展,不把本提案写成已提速。

(1) 入队即冻结请求内容:owner、request-id、base、目标 ref、whole 的 blob/mode、keyed 的键和值、登记集合、说明、产品与门工具身份、deadline。只存路径名不够,排队中编辑器继续改文件不能改变已接收请求。等待返回 job-id,不占一个55秒执行窗口。入队即并行预热 tlib、kit、冷编译及私有输入,这些准备不占活动队首槽,只记 PREPARED;队首只做验证相。预热结果仍核内容及适用输入身份,过期回到准备相重做,不复用旧结论。请求、事件和恢复记录先用有界 TSV 与原子发布,不引入数据库前置。

(2) 到队首核最新 HEAD 与准备身份,只有适用时才组装验证树;身份过期就退出活动槽在队外重做准备,不能把冷准备串入队首:本请求的路径或键已冲突则具名拒绝,不覆盖别人;不相交则把冻结意图重建到最新底树。准备、逐表派生、完整提交门、落库、同步与推送分别为 flow 相,沿用各命令原时限,失败回报相名与最小反例。派生可以按内容身份复用生成物,测试与门仍真实执行;旧门读集没有闭合前,不实现 K(b) 的判定复用。

队列必须有深度上限、入队等待总 deadline 与活动队首 deadline。每次排队事件记录请求 owner、队首 owner、已等时间和剩余预算;超深度或到期具名拒绝。队首卡死时按真实 PID/PGID 有界清理并核终态,释放活动槽,不能无界等锁或只删租约而放任旧进程继续落库。

(3) 落 ref 前核产品、输入树、结果树、工具身份,并核完整路径集及变更路径集严格等于声明意图加正式派生 outputs;多出或缺少路径都具名拒绝,目录与 ignored 链接不得被 git add -A 混入。只有原门通过且 CAS 成功才记 owned-commit=NEW。外部写者抢先推进 HEAD 时,重新组树并重跑门,次数有界;不能把观察到的 HEAD 变化当成本请求已提交。本轮 derive-identity 拒绝却回报他人 OID 为 push-failed 的收据说明这项归属合同必须独立验收。

(4) CAS 前持久化 OLD、NEW 和请求身份。服务重启后核实际 ref、祖先关系与真实进程句柄,区分未落库、已落库待同步和归属不明;不能因缺末行回执重复生成提交。CAS 后 gitsync 保留原有脏工作区与暂存保护及整段55秒边界;同步或推送失败只续各自相,不重跑 commit。回执分别列 owned-commit、sync、push,不把已落库写成未提交,也不把别人的 OID 写成自己的结果。排队取消不动源码;CAS 后取消须明确已落库事实。

(5) 度量先行:实现前冻结现状基线,记录同时3笔互不相交请求到终态的总墙钟、成功/拒绝数、因 HEAD 推进而重做的收据次数及门执行次数;实现后用同一输入与口径取 p50、p95,冷/热准备分开,不能只比较成功子集。提交路径的实现先在私有树完整验证,一次原子落库,避免中间态影响 v0.0.72 合入和冻结。首单验收限定一条 ref:三笔不相交请求各实跑一次原门且队内 CAS 重放为零;入队后改工作区不改变请求 blob;同键冲突拒绝;插入声明外路径及删去声明路径分别拒绝;门失败无 CAS;外部 HEAD 推进触发新输入重验;模拟 CAS 后断电可恢复同一 OID、不重复提交;同步冲突保留新脏数据;推送失败只重试 push。以同输入同门对比排队、准备、派生、门、CAS、同步、推送耗时及门执行次数,未测标 UNMEASURED。

回退只关闭新请求入队并明确处理队列剩余项,已经 CAS 的事务完成同步或保留具名待续状态,随后回到现有 devsvc git commit。暂不做门结论跨请求复用、只读底树缓存、推送合批或多仓调度;这些属于后续独立可验的小单。

## 派生表停止跟踪路线(2026-11-18)

建议发布后按下表顺序逐张切换。以 211fc5b4b1 的生成器 strace 读集报告为测量基点,以 d2cc9f7702 之后的 main 核对调用方。F 是打开或 stat 成功的文件数,D 是目录打开数。耗时与人时分别是旧单表测量和工程估算。消费者按 tools、tests、build 下 .cx、.sh、.ps1 的完整路径字面量去重,另列间接入口。表内省略 tools/ 与 .cx 后缀。

|顺序、导出文件|消费者清单与切换点|查询与缓存键的专属输入|预计人时|
|---|---|---|---|
|1 build/doc-links.tsv|2 个命中。doc-links-gen 生成和比对,check-doc-links-gen 核内容。两者已有 --print 正文可对拍。|devsvc doc-links 取生成器 stdout。F1502,活 Markdown 正文、基线、doclink、git 跟踪路径集合和目标实际存在性。删去工作树内已跟踪目标也须失效。|2 至 3|
|2 tests/batch-domain-index.tsv|3 个命中。batch-domains-gen 产出和核表,check-batch-domains-gen 与 check-affected-domain 构造或读回夹具。affected.domain_load 间接调用 --check --dump,切此单点。|按树查询 batch-domains,保留缺声明、重复与显式例外校验。F761,批次、规则、例外、入口文件头部合同及其解析器。默认 affected 仍无此读集。|3 至 4|
|3 build/iface-hash.tsv|2 个命中。iface-hash 输出和校验,affected 两处读工作区且 git show HEAD:build/iface-hash.tsv。先迁私有输入树和 HEAD 双查询,缺任何一侧就走原保守全选。|F36 D1,build/layer-api 下成员集合和正文、生成器、所用库。按两棵树分别计算,保留 layer-api 的独立检查及 devmeta 路由。|4 至 6|
|4 build/links.tsv|10 个命中。links-gen 生成,rel 默认路径、on-change 经 awp_devmeta_read 读内容。cxlib/relload 是关系导入的解析单点,文件头示例不是额外读取。commit-own 有 keyed 路由。check-commit-own、check-relstore、check-links-gen、check-relload、check-on-change 覆盖落盘、解析与提交。|F14,derive 事实、批次表、生成器及库。默认关系查询交给按身份生成的文件,显式 --links 继续读取调用者输入。区分 .devsvc 镜像与 build 回退,防止无表时漏报派生边。|4 至 7|
|5 build/table-index.tsv|6 个命中。tool-index 的 --tables 生成,--check-tables 核表。关系加载复用 relload,另有 commit-own keyed 路由。check-relstore、check-relload、check-tool-index 分别核导入、显式文件和生成。|F1139 D4,tools 源头注释、tests/batches.tsv、build 表目录和正文。按前四项的逻辑导出清单枚举表,验证有无本地导出时索引一致,避免 fresh checkout 少行。|5 至 8|
|6 build/batch-packages.tsv|15 个命中。affected --layer、evidence-cover 的两份快照、reg-manifest、register、fixture-gate-tool 读取正文或行。register 和 batch-packages-gen 写,commit-own 选 keyed 行,precommit 选路径。check-affected-domain、check-fixture-gate-tool、check-batch-packages、check-commit-own、check-reg-manifest、check-devsvc、check-tables-gen 均需复核私有夹具。|F3129 仅为部分读集,跟踪下 affected --closure-dump 超时。先拆闭包与投影两相,包含 tools、exec、cx、tests、src 及实际读取的 apps/csih、archive、iterate、release/ci、ujs。门内 manifest 的 packages 行与独立现算逐行比对。读集未闭合前仅可无缓存现算。|8 至 16|

批次包层按当前 tests/batches.tsv 现算，用 `sh ./unisacc.com tools/batch-packages-gen.cx -- --query` 取完整投影，`--row ID` 取单行。暂存登记门用 `--index`，历史证据按各快照选择当时的生成器或已跟踪的表。新检出可直接按层选批。显式导出用 `derive --write --only batch-packages-gen`，全量 `--check` 与独立闭包对拍。回退本次迁移提交即可恢复跟踪及原读者。

六项共用第 10 节的查询合同。键纳入生成器、运行库、启动产品、请求参数、内容清单及所枚举的目录成员。新克隆和链接工作树首次查询在所请求的输入树现算,校验内容后原子发布私有导出。损坏时重算,读入或生成失败向调用者返回具名非零。显式 derive --write --only NAME 保留文件导出。实现按 CLI 调用并保留 rl_load 的显式文件接口,六项预计均无需 tools.cxlib 升版。若改为新增公开库入口,该张表另行预留版本。

六张表在 exec/facts/derive.tsv 各有活动行,因此逐张改 query-only 时,derive --check 须改验查询与独立现算的字节等价。chain-ready.cx 的 2a-derive 直接执行此检查。commit-own --derive 的私有树自动写集合逐张少一个输出,其中 links、table-index、batch-packages 还须先迁 CO_REG_TABLE 的登记和夹具。核对 chain 工具与阶段清单未见六个完整路径的直接消费,整链的间接依赖仍须以新工作树的链前置及冻结快照实跑验收。现有收据只覆盖旧入口。

每张表各一笔可回退提交。先并行比对跟踪文件与查询,再切全部默认读者,最后移除跟踪并忽略本地导出。用空缓存新工作树覆盖查询、导出、篡改输入、损坏缓存、chain-ready 的 2a-derive 与私有提交门。回退该笔提交即可同时恢复读者、derive.tsv 行、忽略规则和跟踪文件,在对应源树重算后核 --check。batch-packages 两相读集与六表迁移后的墙钟均待实测。

## 派生按读集选表(opt-in,2026-11-18)

derive.cx -- --touched FILE [--plan]:按合并读集(derive.tsv inputs_glob + derive-inputs.tsv 的全局触发 * 行、cx-runtime 行、本表行)选表,改到输出本身也选,pathset 表在增删文件时选,再做 outputs -> inputs 闭包;query-only 不选。commit-own --derive-selected 用它派生并对选中集合每张表完整 --check;--derive 默认不变,默认切换由集成者决定。真实读集由 tools/check-derive-readset.cx(strace,daily 层)守住,漏登记即 FAIL。全树 derive --check 仍保留在 chain-ready 的 2a 与慢车道,不取消全树覆盖,只换时机。证据 release/evidence/derive-selected-2026-11-18.txt。

## react 阶段一

每个工作树各有一个实例目录。反应器启动即对账，之后每 2 至 5 秒复核。事件或事件文件变化只能提前这一轮。HEAD 用 git rev-parse 取得，index-tree 用私有 index 副本执行 git write-tree，产品读集用 product-map-query --key。读取失败时保留旧记录，标 UNKNOWN；读取期间的版本变化由下一轮对账纠正。冷读集查询须独立作业，以免拖慢已运行作业的取消。

生产策略见 tools/devsvc-react.tsv。按工作树显式运行 `devsvc react start --dry` 可取一轮真实对账和 would-start 回执。`devsvc react start [--ttl 秒] [--cache-manager-osx X64 ARM64]` 启动常驻，`status` 查运行态和实例目录，`stop` 提交停止请求。产品读集稳定 1200 秒且 load1 小于 6 后调用 bg-candidate start；按整链原合同准备 UNISACC_IDENT_HELPER 和管理器。每次尝试单独分配链目录，先落 active.tsv。取消文件由仍持有子进程的 guard 消费，清理进程组及后代；清理有残留会阻止下一次启动。promote --check 与产物摘要复核成功才写 prepared.tsv。候选构造失败可从尝试目录的 bg.log 与 exit.tsv 查因。撤回时运行 `devsvc react stop` 并核 status。

每次对账先更新期望身份，再核现有作业。新身份立即撤销旧作业，稳定期从最后一次变化起算。先前 PREPARED 必须重新核对产物字节。监督者持作业锁，先记录意图，再建立独立进程组并登记 boot_id、PID、启动时刻和截止。登记完成才允许执行。取消先核身份再杀组，等进程组清空才发新作业。超载保留期望身份并延迟放行。

作业只写自己的临时目录。退出成功、产物核验通过且期望身份仍相同，才以原子改名发布产物和 prepared.tsv 的对应行。重启先取工作树单例锁，恢复未完成意图，核对存活监督者和产物，再对账。服务或反应器重启不改变输入身份。常驻 TTL 上限 7200 秒，工作截止另算，调用者可重拉；机器重启后重新计去抖。撤回时停止独立反应器即可。

|负控|可观察判据|
|---|---|
|重启服务并停事件源|一个对账周期内看到真实 HEAD 变化|
|丢弃 index 或产品改动事件|仍能分别读到新的树号或读集键|
|同键重复唤醒|一次作业，一条有效材料记录|
|杀反应器后重启|存活监督者只接管一次，已完成材料复核后复用，损坏材料重做|
|连续改变身份|旧组及后代退出，稳定期满只放行最后一项|
|负载越界再恢复|超载时零启动，恢复后重新核身份再放行|

性能度量分列身份复核、去抖、准入等待及作业墙钟。PREPARED 只表示材料可用，测试仍须实跑。

候选触发键维持产品读集口径。改变打包器、版本注入器、构造工具库的行为，或改变版本和管理器配方时，负责人用新 HEAD 手动起候选并核摘要；即使产品键相同，也要给此次变更单独的构造回执。chain4 固定材料的对照中，六份工具各改一个注释字节，APE 均与原件逐字节相同。把启动器内存下限从 53248 改为 53249 KiB，APE 长度相同、摘要改变。

生产按两步放行。先用满 24 小时的影子观测核误触发和漏触发均为零，通过取代、去重和负载负控后，允许一条有界真实候选。该候选与同冻结输入的手动整链各自通过 promote 检查、产物逐字节相同且 prepared.tsv 与磁盘一致，才放行常规自动构造。原始 dry 行是单次预览，影子统计要独立套用稳定期、准入及材料复用条件。逐项判据与复验命令见 release/evidence/react-candidate-inputs-2026-11-18.tsv。

## 整链提速(2026-11-18)

建议先拆长作业实际占用的写域。real6 的慢点是两代 Linux 对象批，chain4 的慢点已转到跨平台整目标重试。real6 从起到止 3752 秒，链内自产管理器。chain4 显式传两份管理器，OWNER 到 EXIT 为 1613 秒，499/499、rc 0；chain2 首次为 1379 秒，停在 pack；chain3 为 1346 秒，停在 cache-manager 缺 checkpoint。三者都不能替 real6 的同配方冷构造验收。

下表取各会话 `chain-run.tsv` 的 ATTEMPT_TIME 按毫秒降序，各列独立排名，单位秒，保留一次尝试的原墙钟。括号 124 表示该次超时；chain4 这五相后来均以 rc 0 收尾。代号 T 是单个 owner 驱动多步目标构造，可按产物拆；F 是已切出的前端或 stub 子步，内部单进程占比 UNMEASURED；L 是 Linux 闭包准备，J 是多作业组。原始会话在 `/tmp/unisacc-session-v072-chain{2,3,4}/REV/`，chain2/3 的 REV 前缀 ec7fbfdbaf，chain4 为 5cdbd305b1。

|名次|chain2 相 / 秒|chain3 相 / 秒|chain4 相 / 秒|
|---|---|---|---|
|1|g2-osx-arm64 T 88.078 (124)|g2-win-arm64 T 88.073 (124)|g2-win-x86_64 T 88.077 (124)|
|2|gen1-osx-x86_64 T 88.068 (124)|g3-osx-x86_64 T 88.059 (124)|g2-win-arm64 T 88.065 (124)|
|3|g3-win-x86_64 T 82.801|gen2-osx-arm64-stub F 88.055 (124)|gen1-osx-arm64 T 88.058 (124)|
|4|gen1-osx-arm64 T 82.374|gen3-osx-arm64-front1 F 85.723|g3-win-x86_64 T 88.046 (124)|
|5|jobs-gen J 71.474|gen1-osx-x86_64 T 83.577|g2-osx-arm64 T 88.042 (124)|
|6|gen1-win-arm64 T 71.212|g2-osx-x86_64 T 78.311|g2-osx-x86_64 T 84.038|
|7|gen1-osx-x86_64-stub F 66.368|g2-lnx-x86_64-closure-prepare L 77.249|gen1-osx-x86_64 T 76.839|
|8|g2-win-arm64 T 65.271|g2-lnx-arm64-closure-prepare L 76.898|gen1-win-x86_64 T 74.238|
|9|g2-lnx-arm64-closure-prepare L 64.917|g3-win-x86_64 T 72.513|gen2-osx-arm64-front1 F 71.552|
|10|g2-lnx-x86_64-closure-prepare L 64.578|gen1-osx-arm64-stub F 69.126|jobs-gen J 66.639|

计时口径影响选相。仓内 release/evidence 的 chain4 表按主心跳相邻 end 计时，kernels-g2 栏的 752 秒包含其他 lane 的工作。该相作业 ATTEMPT_TIME 为 41.724 秒，内部 DONE 为 30.268 秒。五次 88 秒也有重叠，节省量须按关键路径实测。历史 build/phase-times.tsv 的 g3 Linux x86/arm 两相分别为 734.227/733.842 秒，g2 为 725.068/727.361 秒。candidate-chain 的 run_g2_lnx 从 roots 开始计到 closures 结束，给同批各根登记累计时间，obj-43 的 359.750 秒不能归给一个编译进程。chain4 两个 g3 objects 的 DONE 仅 1.478/1.484 秒，此前闭包已有独立作业。

(1) Linux 按目标内对象组拆。run_g_lnx_pair 已支持每代两个架构同时跑，until、限次让出及 parallel=1 走串行分支；chain-run 还能分 lane，chain4 的 g3 两条 closure-prepare 有不同 owner 且执行重叠。计划序号 474 至 481 中，arm 准备依赖两条 g2 完成，x86 准备依赖 g2 x86。保留依赖与唯一链接者。在同目标内沿现有 fp_batch 冷首键、暖探针与预算划组，现行 build/g2-budget.tsv 为 55 秒、暖批最多 5 键，chain-step 暂存上限 8。先试每目标两个互斥键组、全局最多四个 worker。每组自有 stage 和临时目录；现有 OUT.batch-gG-T、目标级 receipt 和 .chain-steps 不能让两个 worker 同写。单一 owner 按原根序核完整键集、逐对象摘要及构造者身份后发布收据，再链接。组崩溃只丢该组未提交暂存。优先把切片放进相内，499 行计划已接近 candidate-chain 的 512 行容量。

(2) 跨平台先试 g2-win-x86_64。chain4 的 attempt 165 为整目标 --until 加 --lane，超时后 172 成功。gen_slice_phase 依次执行 fronts、win span、导入 image；沿已有 units/e3/image 的落盘边界逐段记开始、结束、摘要和恢复点，实测哪个段耗尽窗口，再只调度尚缺子段。macOS 保留 B 的串行依赖。对拍时同时记录作业时间与最后一个前置完成到 compare 的墙钟，区分消掉的重试和被兄弟 lane 遮住的收益。

(3) 放行与回退。建议冷链目标 3300 秒，给 3600 秒作业留 300 秒。real6 配方须净省 452 秒达到该目标，改后墙钟 UNMEASURED。固定冻结输入、工具与管理器来源，用两个空会话做原版和拆分版冷对照，记录 OWNER 到 EXIT、负载、每组 PID 和起止；逐对象、六目标 image 与 APE 字节相同，两边 promote --check 通过。另测中断一组后恢复、少键、重复键、坏摘要均不得完成父相。若计划 id、顺序、依赖或宽度改变，chain-run 的计划身份随之变化，须新开 BASE；相内方案也须绑定分片协议版本，旧 stage 或旧 DONE 不能直接采信。保留原配方和独立输出目录作回退。同配方冷跑达到预算后，才具备 react 真起的耗时证据。
