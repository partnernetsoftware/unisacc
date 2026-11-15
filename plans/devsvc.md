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
- 单实例:listen 遇 EADDRINUSE 先试连,连得上即拒绝再起,连不上视为陈旧 sock 清掉重听。
- 无实例时:lock、run、events 自动拉起(等待上限 40 s);status、ping、stop 不拉起;run 可 `--local` 或在拉起失败时本地执行。

## 4 CLI 与 API

- CLI:`sh ./unisacc.com tools/devsvc.cx -- <子命令>`,子命令见文件头注。
- API:一连接一请求。请求一行 TSV `verb<TAB>arg...<LF>`;响应首行 `OK<TAB>...` 或 `ERR<TAB>code<TAB>...`,其后载荷行,服务端关连接。
- 现有 verb:ping、stop、lock-acquire、lock-release、lock-list、native、emit、events。`.cx` 客户端用 `net.unix` 加 `net.tcp` 的带截止收发即可。
- 退出码:0 OK;1 ERR(如锁被他人持有);2 用法错;3 连不上实例;run 为工具自身退出码。

## 5 基础版服务

- (1) status:各工作目录的已暂存、未暂存、未跟踪计数(`--paths` 列路径)、在途进程、实例与锁。
- (2) lock:命名写窗口锁,带属主与截止,自动过期,同属主再拿即续期,落盘跨实例重启。约定名:`registry`(tests/batches.tsv、tests/coverage.tsv、build/batch-packages.tsv 写窗口)、`derive`(派生表写窗口)。锁是协作约定,当前不强制拦截写入。
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
- P6 precheck --files:一次列出全部门拦截原因(blocked 96 次、REFUSE 73 次)。
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

- 库 `tools/cxlib/flowdoc.cx`(`fd_run(op, args, ff_out*)`):节路径 `A/B/C` 按 Markdown 标题逐层嵌套匹配(标题去首尾空白,代码块内 `#` 行不算标题),找不到 ENOSEC,歧义 EMULTISEC。
  - doc.get 取节(含标题行,sha 为全文件摘要);doc.list 列节树 `层级<TAB>路径`。
  - doc.set 整节正文替换(标题保留);doc.append 节尾(尾随空行之前)追加一行;doc.remove 按唯一子串删节内一行。
  - doc.mermaid 在节内 mermaid 块里:old 唯一则替换为 new(改节点文字);old 空则在首块闭合前加一行 new(加连线);new 空则删含 old 的行(删连线)。
  - 写入一律经 file.write:原子写、根越界 EOUTSIDE;带读入时的 sha 写,期间他人改过即 ESHA;调用者给的 expect_sha 不符同样 ESHA。
- 锁:库不取锁,只做 sha 兜底;flow 的 doc.* 写节点默认 lock=doc:<path>(同文件串行,不同文件并行);CLI 写操作内部取 devsvc 锁 doc:<文件>。调用者不感知。
- CLI:`devsvc doc get|set|append|remove|list|mermaid FILE SECTION [k=v | k@=PATH ...] [--from F] [--expect-sha X]`;set 的 --from 为 content,其余为 line。
- 允许范围:只作用于 `tools/devsvc-repo.tsv` 的 doc 行(glob,* 不跨 /):prd.md、prd_*.md、plans/*.md;核心不写死路径。cfg_load 对不认识的行类型只告警一次不判坏表(向前兼容旧执行体)。
- 检查:`tools/check-flowdoc.cx`(批次 flowdoc,26 项)。
- 未做:CLI 与 flow doc 节点各自的专项检查项;入口按 HEAD 固定核心,doc 子命令在新核心入库后才对 `devsvc.cx` 生效。

