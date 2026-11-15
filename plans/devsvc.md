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

