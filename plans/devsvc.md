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

## 6 已知限制与下一单

- (1) 服务单线程:native 构造期间其它请求排队。改为构造放子进程,主循环继续收请求。
- (2) 事件环不落盘、无阻塞订阅。加 `events wait --since N --timeout S`(长轮询)与落盘尾段。
- (3) status 不标路径归属。接 `tools/domain.cx` 与 `tools/own-diff.cx`。
- (4) 锁不强制。让 `tools/commit-own.cx`、`tools/register.cx`、`tools/derive.cx --write` 在写共享表前查锁,钩子接入另议(本轮禁改 `.claude/hooks`)。
- (5) 本仓配置表落地(见第 2 节),并登记批次、补 prd.md 能力速查、重算 tool-index。
- (6) 两个 start 同时清理陈旧 sock 有窄竞态,改用锁目录原子创建。

## 7 路线

- lint:把十三个棘轮与 precommit 预演做成服务内的增量检查,按变更路径触发。
- 关系 DAG:常驻持有 `build/links.tsv` 与 use 边,答"改这个路径影响谁",取代 affected 与 on-change 各自冷算。
- 记忆宫殿视图:把 `prd_mempalace.md` 的房间解析成可查询的表,经 API 读写单个房间。
- lsp:符号索引(`symindex`)常驻,提供定义与引用查询。
- flow:把"改 -> 派生 -> 选批 -> 提交门"串成事件驱动的流。
