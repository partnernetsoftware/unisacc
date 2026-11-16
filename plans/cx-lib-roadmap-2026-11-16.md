# cx 核心库与扩展库丰富完善路线(2026-11-16)

日期 2026-11-16。性质:只读分析与规划,未改任何源码与表,未实现项一律不得记 PASS。取数产品:共享检出已装 unisacc.com(0.0.67 线),main 头 acd8c633df,lnx/x86_64 本机;osx 与 win 行为本文未测,一律以 `build/std-surface.tsv` 单元格为准。

取数方法(均经 `devsvc x --par 1` 实跑,可重跑):
- 重复函数普查与片段计数:一次性脚本 `$CLAUDE_JOB_DIR/tmp/libroad/scan.cx`(作业目录,未入库;`fn` 模式按行首 `static` 取函数定义名、函数体行数与去空白 FNV 指纹,按名聚合出定义文件数与不同函数体数;`pat` 模式对固定串计出现次数与文件数)。口径:按函数名聚合,同名不同义会高估,异名同义会低估;字符串内花括号已跳过,注释内花括号未处理。
- 面表聚合:`$CLAUDE_JOB_DIR/tmp/libroad/surf.cx`(用 tsvx.read 逐行读 `build/std-surface.tsv`)。
- 稳定判据:`sh ./unisacc.com tools/stable-eval.cx` 实跑,末行 `CHECK_STABLE NOT-READY packages=19`。
- 未跑:`tools/tape-gap.cx`(需要目标带子,本次没有冻结候选的带子)、任何批次、任何六目标验证。相关结论均标 UNVERIFIED。

版本号口径:以 `build/version-seq.tsv` 为准(0.0.69 编译覆盖缺口一期、0.0.70 prune 前置、0.0.71 内存安全乙期、0.0.72 链接单元一期、0.0.73 netraw、0.0.74 标准核心面封口、0.0.75 门面重导出与包根、0.0.76 锁文件校验、0.0.77 DNS 与 inflate 及 G0)。`prd_mempalace.md` 地图与派工文字里的"0.0.70 内存安全乙期、0.0.73 至 0.0.76 生态门"是顺延前的旧号,与版本序表差一号,需主调度在下一次里程碑改图。

## 1 结论

- (1) 库不是主要缺口,采用才是。已有的 `mem.vec`、`mem.map`、`path.path`、`text.iter`、`text.edit`、`text.codec`、`stream.rw`、`tsvx.read`、`sysx.exec`(np_exec)在 tools 非检查文件里的使用单元数全部为 0(各自只有自己的 check 在用),而同类手写片段在 tools 里有几百到上千处。原因是 API 形状不贴合写工具的人:偏移加长度的迭代器、没有带格式的拼装、map 是线性扫描、路径拼接要自己管缓冲。
- (2) tools 与 tools/cxlib 共 1028 个 .cx、180403 行,静态函数 8433 个;同名定义于 2 个以上文件的函数名 755 个、定义 4442 处、合计 33787 行,其中超出"每名一份"的冗余 26213 行(占 14.5%,按名聚合的上限)。生成器族 74 文件 20273 行里冗余 6249 行(31%);检查族 485 文件 65116 行里冗余 7448 行(11%)。
- (3) 按价值排序先做六个不触产品输入的包:进程统一 `ext.sysx` 的 px 面、检查骨架 `ext.testx`、表装载 `ext.tsvx` 的 tt 面、生成器套件 `cxlib.genkit`、文件系统便捷 `ext.fsx`、参数表 `ext.argx`;全部可立即在 ext/ 与 tools/cxlib 孵化,用已装产品编译,不等任何版本。
- (4) 必须进产品输入的只有 L0 的六项(子进程等待集、rusage、PDEATHSIG、mtime 纳秒、带 cwd 与 stderr 分离的同步运行、mm_map 改哈希),归 0.0.74 封口版一次改完;封口后新函数只能先进 ext。
- (5) API 设计受当前编译器三条硬限制约束:跨单元函数指针形参不能带结构体指针(0.0.69 修)、`use as alias` 与 export 门面不可用(0.0.71、0.0.75)、每单元 use 上限。所以首版一律用游标式迭代代替回调、回调上下文只用 `void *`、一包一至两个模块、前缀全局唯一。
- (6) 不做:线程与事件循环、TOML、SQLite、正则全集、时区。工具代码里这几类需求的出现次数为 0 或只有一条缺口记录,做了没有消费者。
- (7) 内存安全口径不变:新包只提供运行期句柄代次检查、带界接口与可断言的泄漏计数,不声称编译期所有权;0.0.71 的 owns 标注落地后把新包的创建函数作为首批标注对象。

## 2 A 现状盘点

### 2.1 包按层列表(真源 `build/tlib-packages.tsv`、`build/layer-api/`、`build/std-surface.tsv`)

| 环 | 包 | 版本 | 接口数 | 面表行数 | test 列为 - 的行 | win 拒绝格 | win 未核格 | 非检查工具使用单元数(按模块) |
|---|---|---|---|---|---|---|---|---|
| S | cx.mem | 0.1.2 | 61 | 59 | 10 | 0 | 0 | heap 113、bytes 156、grow 4、arena 10、slice 0、vec 0、map 0 |
| S | cx.text | 0.1.2 | 44 | 41 | 21 | 0 | 0 | text 309、fmt 45、conv 2、iter 0、edit 0、utf8 0、codec 0 |
| S | cx.sys | 0.1.11 | 88 | 58 | 47 | 25 | 20 | io 144、sha 56、sig 26、sigwatch 1、errkind 0、cap 0、reslive 0 |
| S | cx.os | 0.1.3 | 135 | 115 | 115 | 26 | 33 | fs 234、time 96、env 94、proc 93、posix 65、osx 4、utc 0 |
| S | cx.path | 0.1.0 | 27 | 26 | 2 | 0 | 0 | path 0 |
| S | cx.stream | 0.1.2 | 32 | 29 | 13 | 0 | 0 | item 0、pipe 0、rw 0 |
| X | ext.memx | 0.1.1 | 5 | 4 | 4 | 0 | 0 | (arena,经 mem.arena) |
| X | ext.textx | 0.1.3 | 44 | 25 | 25 | 0 | 0 | build 8、json 5、deflate 3、num 11 |
| X | ext.sysx | 0.1.3 | 32 | 10(记在 ext.procx 名下) | 10 | 2 | 7 | process 18、poll 14、reaper 7、trace 2、exec 0 |
| X | ext.net | 0.1.9 | 37 | 33 | 33 | 27 | 0 | tcp 1、unix 1、sock 1、其余 0 |
| X | ext.crypto | 0.1.1 | 132 | 118 | 8 | 0 | 0 | sha256 2、tls13 1,合计 3 |
| X | ext.tsvx | 0.1.1 | 11 | 9 | 9 | 0 | 0 | merge3 1、read 0 |
| K | tools.cxlib | 0.3.13 | 规范记 467,现清单 570 行(67 个模块文件) | 无行 | - | - | - | 任一 cxlib 模块 217 |
| K | cx.base、cx.plan、cx.runtime、cx.compile、cx.driver | 各见真源 | 93、14、349、524、34 | 无行 | - | - | - | 产品内部环,不在本规划范围 |

补充事实:
- `unisa/` 目录在仓内不存在(`git ls-files unisa` 为 0);pkg-gaps 里写"建议归属 unisa/graph、unisa/algo、unisa/build"的五行没有落点。本规划的裁定:不新开第三棵树,纯计算算法进 `ext/algox`,待 G1 后再按 0.9 节外置。
- 面表 new 行已清零(stable-eval 各包 S2 全 ok),0.2 节里"new 108"是 11 月 12 日旧数。稳定判据仍 19 包全 NOT-READY,S6 至 S9、S12 恒 UNKNOWN(证据未接)。
- test 列为 - 不等于没有检查(cx.os 的 115 行全为 -,但 check-cx-sys 等批次存在),说明面表到批次的映射没填,S6 无法机械求值。这是表缺口,不是测试缺口,但负控是否逐行覆盖因此无法从表判定。
- `cx/mem/arena.cx`、`cx/text/build.cx`、`deflate.cx`、`json.cx`、`num.cx` 是指向 ext 的符号链接,同一文件两个模块身份(改进 E,0.0.73)。
- 文档不一致两处:`prd_cx_spec.md` 5.2 写"没有 JSON 解析库",4.4 已列 js_ 系列(ext/textx/json.cx 336 行,check-json 批次 textx-json 在);5.2 写 pkg-gaps 记录方式已退役,CLAUDE.md 仍要求记 `build/pkg-gaps.tsv`。

### 2.2 对照 Rust std 与 Bun:缺口与"能力在但不好用"

| 能力域 | Rust 或 Bun 的对应 | cx 现状 | 判定 |
|---|---|---|---|
| 进程:同步跑并取输出 | Command::output、Bun.spawnSync | 五个入口并存:隐式 cx_runv_timeout(253 处 135 文件)、np_run(581 处 294 文件)、sy_run(89 处 35 文件)、sy_spawn_pipe(84 处 56 文件)、cxlib pr_run(39 处 16 文件);另有裸 fork 51 处、waitpid 68 处、system 34 处、popen 19 处;np_exec 是设计上的正解但使用者只有它的检查 | 能力在,入口分裂;缺 cwd、环境增量、stdin 数据、rusage、等待集 |
| 进程:异步与等待多个 | Child、Bun.spawn | sy_proc_spawn 加 try_wait 轮询(flowexec 2 毫秒睡眠轮询) | 缺等待集(pidfd 或 SIGCHLD)、PDEATHSIG、kill-tree 未接超时路径 |
| 文件整读整写 | fs::read、Bun.file | sy_read_file(127 处 46 文件)对隐式 cx_read(1439 处 480 文件)、cx_write(867 处 290 文件);fopen 245 处 | 能力在;隐式运行库的 cx_read 失败只返回空指针、无错误码,是事实上的标准入口 |
| 路径 | Path::join | pt_join 使用 0 处;`"%s/%s"` 格式拼接 947 处 399 文件 | 能力在但不好用:要自备缓冲、返回长度、无"拼完即用"的形态 |
| 目录遍历与 glob | read_dir、walkdir、Bun.Glob | sy_dir_next 38 处、裸 opendir 64 处、cx_walk 9 处(回调式);glob 只在 flowfile 内部私有 | 缺排序遍历、缺公开 glob、缺树哈希 |
| 临时目录与清理 | tempfile、mkdtemp | mkdtemp 257 处 201 文件、sy_tmpdir 276 处、cx_rmtree 147 处、`rm -rf` 子进程 75 处、写死 `"/tmp/` 196 处 128 文件 | 缺"作用域临时目录"(建、用、必删);写死路径是并发检查互扰来源(例:check-abab-bench 写固定 /tmp/abab-check.out) |
| 字符串构建与格式化 | String、format! | sb_ 系列 10 文件;snprintf 1755 处 256 文件;同名手写 put 125 文件、app 34 文件 | 能力在但不好用:sb 没有带格式追加,所以大家用定长栈缓冲加 snprintf |
| 行与字段迭代 | lines、split | tx_line_next 3 处、tx_split_next 5 处;手写 `'\n'` 2049 处 524 文件、`'\t'` 1187 处 365 文件、strstr 1741 处 | 能力在但不好用:迭代器给偏移加长度,取成 C 串还要自己拷 |
| TSV | csv crate、Bun 内置 | tv_ 四个函数使用 0;tx_field 147 处 35 文件;手写 field、row、split、parse_tsv、take_row 共 133 个文件各一份 | 缺表对象:表头(含注释行表头)、按列名取值、键索引、重复键拒绝、原子追加 |
| 集合 | Vec、HashMap、BTreeMap | mm_vec 使用 0、mm_map 使用 0(实现为线性扫描,cx/mem/map.cx 第 9 行自述 O(n));malloc 423 处、realloc 160 处、calloc 121 处、`#define MAX` 定长表 114 处 67 文件 | 能力在但不好用加性能不足;缺字符串驻留表、位集 |
| 排序与二分 | sort、binary_search | 无库;libc qsort 53 处 41 文件;手写 FNV 28 处 20 文件 | 缺 |
| 参数解析 | 无(clap)、util.parseArgs | 无库;`strcmp(argv[` 821 处 183 文件;`usage:` 325 处 256 文件 | 缺 |
| JSON | serde_json、内置 | js_ 解析树 6 个使用者;写出侧手写转义,`\":` 887 处 58 文件(facts-gen 一个文件 214 处) | 读有写无 |
| 哈希 | sha2、Bun.hash | 三个入口:cx_sha256_hex 622 处 219 文件、sy_sha256_hex 157 处 68 文件、`sha256sum` 子进程 54 处 18 文件;手写 digest(文件) 46 文件 | 缺"文件一步哈希"与树哈希的公开入口 |
| 压缩 | flate2、Bun.gzipSync | deflate、gzip、crc32 有;inflate 无 | 已排 0.0.77 |
| 测试 | #[test]、bun:test | 无库;485 个检查各自手写 ck(293 文件 244 种写法)、has(130)、run(138)、ok(63) | 缺 |
| 错误退出 | panic、process.exit | cxlib.die 13 个使用者;手写 die 276 文件 274 种写法、fail 55 文件 | 能力在但签名四参数不顺手 |
| 网络与 HTTP | std::net、Bun.serve | TCP、UDP、Unix 域有;DNS、HTTP、URL 无 | 已排 0.0.77(G0) |
| 时间 | Instant、Duration | sy_now_ms 720 处、deadline 一族齐;usleep 36 处应迁 sy_sleep_ms | 够用 |
| 线程、TOML、SQLite、正则 | std::thread、内置 | 无 | 工具代码零信号(线程一条缺口记录),本规划不做 |

## 3 B 需求证据

### 3.1 `build/pkg-gaps.tsv`(49 行)按主题归并

| 序 | 主题 | 行数 | 代表行(source_task) | 现状核对 |
|---|---|---|---|---|
| 1 | 进程:stderr 分离、cwd、环境增量、rusage、等待集、kill-tree、PDEATHSIG、带界捕获、线程 | 9 | P1 func-callgraph、0.0.58 tlib 接线、CPU-METER-RUSAGE、EVIDENCE-COVER-V1、B2f-NPEXEC、DEVSVC-FLOWEXEC-WAIT、KILLTREE、THREADS、CLIENT-PDEATHSIG | np_exec 已在 ext/sysx/exec.cx,B2f-NPEXEC 行仍 OPEN(过期);其余未做 |
| 2 | TSV:行迭代、列拆分、schema、重复键、条件改写、原子追加、三方合并 | 9 | C0.2/BASE-1、G1 graph-check、0.0.50 构造图、W1 std-surface、PATCH-QUEUE-V1、ITER-8、LEDGER-ADD、batch-cost、cc 合并 | merge3 已验证;tv_ 读取器已有但零采用,0.0.50 行仍 OPEN(部分过期) |
| 3 | 文件系统:目录列举、glob、树差异、树哈希、mtime 纳秒、原子追加、自身路径 | 8 | 0.0.58 tlib、DOC-CHECKS-V1、EVIDENCE-COVER-V1、RECEIPT-GLOB-TREEHASH、batch-cost、C49-131、LEDGER-ADD、OSX-TOOL-SELF-PATH | 均未做;mtime 纳秒受编译器 member access 缺口(第二期)与 sy_stat_t 签名双重限制 |
| 4 | 编译器与驱动(非库):仓外模块搜索路径 2 行、函数指针形参、运行缓存按内容取键、阶段化 API、入口可达集、分相测速 2 行 | 8 | 0.0.50 构造图、W1、0.0.58 cx.stream、DEVSVC-DERIVE-FLOW-RUNCACHE、C49-X、R2-HOTFAIL、A12、X7 | 不在库规划内;其中两项直接限制库 API,见第 7 节 |
| 5 | 容器与算法:字符串到整数映射、位集与可达闭包、排序、DAG 环检测、拓扑与最长路 | 5 | P1 func-callgraph 三行、G1 graph-check、0.0.50 构造图 | 均未做 |
| 6 | C 词法与源码扫描:跳注释与字符串的扫描器、调用实参抽取 | 4 | P1 func-callgraph、0.0.50 lint 原型、PRECHAIN-SRC-SCAN、ITER-8 | cx-lint 内有原型,cxlib.chainargv 另有一份 token 化(ca_tokens),本次 scan.cx 又手写第三份 |
| 7 | 文本:行索引与偏移转行号、markdown 锚点 3 行、键值扫描、大缓冲 `%.*s` 慢 | 6 | P1、README-ANCHOR、README-PAIR、DOC-CHECKS-V1、batch-cost、CX-PRINTF-PREC-SLOW | 未做 |
| 8 | 其余:DNS、sy_res_live 登记点、mm_copy 容量宏、配方枚举、gh 回放、ELF 逐节对拍、账本追加与精确替换(已验证 3 行) | 10 | B2e-NB、B2f-RES、cx-mm_copy-dcap 等 | sy_res_live 已有 cx/sys/reslive.cx(行过期);精确替换与账本追加已 VERIFIED |

### 3.2 工具重复代码统计 Top 20(tools 与 tools/cxlib,1028 文件;列:同名静态函数的定义文件数、不同函数体数、合计行数)

| 序 | 缺口 | 证据(函数名 文件数/不同体/行数) | 例子文件 | 拟归包 |
|---|---|---|---|---|
| 1 | 带时限跑子进程并取输出 | run 138/130/959;spawn_pipe_bounded 27/1/1053(逐字相同);spool_open 27/1/216;bounded_run 26/5/472;sh 37/26/229;call 32/28/114;runv 14/10/116;child 9/9/561 | tools/check-archive-refs.cx、tools/check-census.cx、tools/affected.cx | ext.sysx px 面 |
| 2 | 检查断言骨架 | ck 293/244/754;has 130/95/489;ok 63/26/153;check 38/28/131;same 33/28/79;ckv 11/11/44;note 15/8/84 | tools/check-abab-bench.cx、tools/check-affected.cx | ext.testx |
| 3 | 生成器骨架(事实表到生成物) | emit_check 26/26/1185;emit_mod 25/25/704;take_row 18/18/463;parse_tsv 14/14/332;one_file 26/1/312(逐字相同);same_bytes 30/2/180;copy_field 29/2/145;app 34/5/348;app_n 18/1/162;ident_ok 11/3/139 | tools/axis-gen.cx、tools/cell-gen.cx、tools/code-gen.cx | cxlib.genkit |
| 4 | 文件读写与存在性 | load 67/56/452;slurp 38/32/303;save 53/46/99;saved 31/5/56;readfile 13/9/36;exists 12/12/28;absent 21/16/30;write_new 9/7/9 | tools/bg-candidate.cx、tools/ape-pack.cx、tools/carried-header-export.cx | ext.fsx |
| 5 | 退出与报错 | die 276/274/336;fail 55/54/162;refuse 18/17/47;bad 13/13/34 | tools/a12-cycle.cx、tools/axis-gen.cx | ext.argx 的 ax_die |
| 6 | 字符串拼装 | put 125/97/451;append 8/8/43;join 18/16/29;snprintf 1755 处 256 文件 | tools/a12-cycle.cx、tools/abidispatch-gen.cx | ext.textx 的 sb_fmt |
| 7 | 路径拼接 | path 118/53/147;`"%s/%s"` 947 处 399 文件;pt_join 0 处 | tools/archive-ar-cases.cx、tools/chain-step.cx | ext.fsx 的 fsx_join |
| 8 | TSV 行列 | field 38/34/193;row 42/28/100;split 21/16/127;fields 9/9/39;line 13/12/63;`'\t'` 1187 处 365 文件 | tools/graph-check.cx、tools/precommit.cx、tools/devsvc-core.cx | ext.tsvx tt 面 |
| 9 | 参数解析 | `strcmp(argv[` 821 处 183 文件;`, "--` 2822 处 399 文件;usage 8/8/47;`usage:` 325 处 | tools/devsvc-core.cx(83 处)、tools/s9-shards.cx(40 处) | ext.argx |
| 10 | 文件摘要 | digest 46/29/77;hash 30/25/35;`sha256sum` 子进程 54 处 18 文件 | tools/cache-retain.cx、tools/check-fixedpoint-sample.cx | ext.fsx 的 fsx_sha256 |
| 11 | 临时目录与清理 | mkdtemp 257 处 201 文件;写死 `"/tmp/` 196 处 128 文件;`rm -rf` 75 处;cleanup 10/10/28;fixture 12/12/163;setup 9/9/66 | tools/check-candidate-chain.cx、tools/build-cxcc-ape.cx | ext.fsx 的 scratch |
| 12 | 文本查找 | find 13/12/51;starts 8/7/22;count 9/9/52;lines 12/10/50;strstr 1741 处 270 文件 | tools/check-devtask.cx(86 处) | ext.textx 行索引与 tx 包装 |
| 13 | 动态数组与定长表 | malloc 423 处 187 文件;realloc 160 处 65 文件;`#define MAX` 114 处 67 文件;add 16/15/92;mm_vec 0 处 | tools/sim.cx、tools/cxlib/e2stream.cx | ext.algox、cx.mem |
| 14 | 排序 | qsort 53 处 41 文件;手写插入排序(pkg-gaps 两行) | tools/cxlib/sourcepackage.cx、tools/batch-cost.cx | ext.algox |
| 15 | 目录遍历 | walk 12/11/218;opendir 64 处 51 文件;`"find"` 子进程 11 处 | tools/chain-precheck.cx、tools/cx-driver.cx | ext.fsx 的 walk |
| 16 | git 子进程 | `"git"` 388 处 89 文件 | tools/check-integrate-patch.cx(39)、tools/devsvc-core.cx(24) | cxlib.git |
| 17 | JSON 写出 | `\":` 887 处 58 文件;js_esc 5 处 2 文件 | tools/facts-gen.cx(214)、tools/check-guard-hook.cx(93) | ext.textx 的 jw 面 |
| 18 | 锁与独占创建 | O_EXCL 149 处 109 文件;sy_open_new 51 处;flock 35 处 18 文件 | tools/whole-front.cx、tools/cache-retain.cx | ext.fsx 的 lock |
| 19 | 数字解析 | atoi 81 处、atol 97 处、strtol 139 处、sscanf 124 处;tx_to_i64 27 处 3 文件;num 19/18/107;number 25/25/60 | tools/facts-gen.cx、tools/devsvc-core.cx | 迁 tx_to_i64(加 lint) |
| 20 | 手写哈希表 | FNV 常数 28 处 20 文件、djb2 3 处;mm_map 0 处 | tools/rel.cx、tools/symindex.cx、tools/tsv-lint.cx | ext.algox 的 hm 与 intern |

近 100 条提交里的缺陷信号(同一类错反复修,说明应沉到库):参数解析类 8 条(48ad2dbf7a 选项位置无关、481452ee45 严格解析、38d49e211d 接受并忽略 --par、16045f8009 与 c00bff644a 非零 rc 误报 bad arguments、b94d1a83a2 --help rc 0、c7b09090fe 缺必填项逐个具名、e1a162aab0 --args 规范化);检查夹具(克隆、固定副本、路径)类修复至少 4 条(4da17f021c、5d7312a846、8babd6695f、62be1c9736)。

## 4 C 设计

### 4.1 共同约定(每个包都遵守,不再逐包重复)

- 错误模型:沿用 0.4 节,返回 0 或负的 errno 码,计数与长度走返回值;不做 Result 对象,不用全局 errno。面向工具的便捷层(testx、genkit、ax_die)允许"失败即带文案退出",但只包在返回码函数之外,不替代它。
- 所有权:拥有资源的对象一律是 int 句柄加代次(仿 cx.stream:关闭后使用与重复关闭确定返回 `-EBADF`,同槽重开代次加 1);从句柄取出的文本是"借到下一次改动或关闭为止"的指针加长度,合同写进面表 own 列并配一条关闭后取值被拒的负控。句柄表容量是固定上限,满了返回 `-EMFILE`,不静默扩张。
- 边界:所有写出口带目标容量,放不下返回 `-ERANGE` 或 `-EMSGSIZE`;整读一律带 max;大缓冲走堆(自动数组达 1000000 字节当前被拒)。
- 与 L0 的关系:纯计算包只依赖 cx.mem 与 cx.text;系统包只调 `sy_`,不写平台 `#if`、不直调 libc;平台差异只经 cx.sys、cx.os 的 `_os` 帮手与 `sy_cap` 查询。包内不写 `#include`,不 dock。
- 六目标:纯计算包三列全 impl,用同一份 gold 表在各目标跑;系统包的 osx、win 列在实跑前记 unverified,按能力拒绝的写 `refuse:cap=NAME`。新增任何 `.sys` 操作前先跑 `tools/tape-gap.cx -- TAPE TARGET` 确认 gaps=0(已知 win/x86_64 的 poll 与 syscall 行 selected 0)。
- 表先行:每个公开函数先在 `build/std-surface.tsv` 加行(含 analogue、errors、own、test 列,test 列必须填批次号,不留 -),再 `tools/layer-api.cx -- --write` 判类升版、`tools/surface-doc-gen.cx` 重生文档。
- gold 与负控:纯计算包的用例放 `tests/cases/<包>/cases.tsv`(列:id、输入、期望输出或期望错误码、来源),一个检查驱动读表逐行求值,这是"事实表 -> 检查"的形态;每个公开函数至少一行成功、一行失败、一行拒绝。有独立参照的做差分(排序对 libc qsort、新 map 对现线性 mm_map、JSON 写出再经 js_parse 读回)。
- 受编译器限制的写法:回调上下文只用 `void *`;优先游标式迭代;不用 `use as`;一个包对外一至两个模块;不在文件作用域用 `sizeof 对象` 作常量;函数表用哨兵结尾。
- 内存安全的如实清单。已有:带界接口、句柄代次检查(cx.stream 已实测 4096 次重开)、arena 整体释放、`sy_res_live` 计数、工具 `tools/cx-own.cx`(过程内所有权检查,只报告)。没有:借用检查与生命周期、裸指针释放后使用的拒绝、跨函数所有权转移的编译期验证、整数溢出的系统性检查。已知风险照录:`xs_item_set` 参数非法时仍释放调用方数据(双重释放隐患,未修)。新包不得在文档里使用"内存安全"一词。

### 4.2 P1 进程统一:`ext/sysx/run.cx`(模块 sysx.run,前缀 px_)

- 目标:把五个同步入口与 27 份逐字复制的 spawn_pipe_bounded 收成一个;补 cwd、环境增量、stdin 数据、两路分别带界、整树清理。
- 做法:在已有 np_exec 之上加一层便捷面,不重写底层;np_run、pr_run 保留为调用 px_run 的薄壳(合同不变,检查原样通过)。
- API 草案:
```c
typedef struct { char **argv; const char *cwd; char **env_add; const void *stdin_data; long stdin_n;
                 long deadline_ms; long out_cap, err_cap; int merge_err; int kill_tree; } px_opts;
typedef struct { int code; int timed_out; int sig; int residue; long wall_ms; mg_buf out, err; } px_result;
void px_opts_init(px_opts *o);                       /* deadline_ms 默认 55000,out_cap 默认 16 MiB */
int  px_run(const px_opts *o, px_result *r);          /* 0 表示已起并已回收;code 为归一退出码,超时 124 */
void px_result_free(px_result *r);
int  px_set_open(int *set);                            /* 等待集句柄 */
int  px_set_add(int set, const px_opts *o, int *slot);
int  px_set_wait(int set, sy_deadline_t until, int *slot, px_result *r);   /* 1 有结束,0 到期 */
int  px_set_close(int set);                            /* 未结束的子进程按 kill_tree 清理后回收 */
```
- 所有权:px_result 的两个缓冲归调用方,px_result_free 配对;等待集是句柄,关闭即清场,关闭后使用 `-EBADF`。未实现的性质:调用方忘记 free 不会被编译期发现,只能靠检查里的 `sy_res_live` 断言。
- 与 L0:首版只用现有 sy_proc_spawn、sy_proc_try_wait、sy_poll、sy_kill_tree,等待集内部仍轮询;0.0.74 的 L0 等待集(第 5 节 L1)落地后只换内部实现,签名不动。
- 六目标:lnx、osx 走 poll;win 走面表已记的 PeekNamedPipe 轮询路径;osx、win 列记 unverified 直到候选 CI 实跑。
- 负控:超时得 124 且进程组无残留(按真实 PID 查);自行 setsid 的孙进程在 kill_tree 为 1 时被清;输出超 cap 得 `-EMSGSIZE` 且子进程已杀;可执行不存在 `-ENOENT`;cwd 不存在 `-ENOENT`;stdin 数据大于管道容量不死锁;两路同时大量输出不死锁;等待集关闭后再 add 得 `-EBADF`。
- 写域:`ext/sysx/run.cx`、`tools/check-ext-sysx-run.cx`、`tests/cases/sysx-run/`、`build/std-surface.tsv` 与 `build/layer-api/ext.sysx.tsv` 自己的行、批次登记行。
- 验收:`sh ./unisacc.com tools/check-ext-sysx-run.cx` 末行 `failed=0`(预计 24 例);`sh ./unisacc.com tools/layer-api.cx -- --check --pkg ext.sysx` rc 0;`sh ./unisacc.com tools/check-std-surface.cx` 通过;重复普查中 spawn_pipe_bounded 定义数 27 -> 0、bounded_run 26 -> 0。
- 预估收益:可删约 3000 行(spawn_pipe_bounded 加 spool_open 1269 行为精确值,其余 run、sh、runv、call 约 1400 行按一半可替换估);关掉 pkg-gaps 5 行。
- 依赖与风险:无前置。风险是 135 个 cx_runv_timeout 调用方的语义差(它合并两路且以秒计),迁移必须分批并逐批跑 affected;不得借迁移改任何检查的时限。

### 4.3 P2 检查骨架:`ext/testx/check.cx`(模块 testx.check,前缀 tk_)

- 目标:485 个检查共用一套断言、子进程调用、夹具目录与汇总行。
- API 草案:
```c
void tk_begin(const char *name);
void tk_ok(int cond, const char *what);                              /* 取代 ck */
void tk_eq_str(const char *got, const char *want, const char *what);
void tk_eq_int(long got, long want, const char *what);
int  tk_has(const char *hay, const char *needle);
int  tk_run(char **argv, const char *cwd, long deadline_ms);          /* 经 px_run;结果留在模块内 */
int  tk_rc(void); const char *tk_out(void); const char *tk_err(void); /* 借到下一次 tk_run */
int  tk_scratch(char *dir, long cap);                                /* 唯一临时目录,tk_end 时必删 */
int  tk_write(const char *dir, const char *rel, const char *text);
int  tk_end(void);                                                   /* 打印 failed=N total=M,返回 0 或 1 */
```
- 所有权:模块私有状态持有最近一次运行的输出与所有 scratch 目录;tk_end 统一释放并删除。检查写的是进程,泄漏到进程结束为止,不另设句柄。
- 负控:tk_end 在有失败时返回 1 且末行计数与失败数一致;tk_scratch 的目录在 tk_end 后不存在;tk_run 超时记失败并带 124;两个检查并发各自 scratch 不相交。
- 写域:`ext/testx/check.cx`、`cxroot.tsv` 加一行 `R testx ext/testx`(共享表,须持锁并只带自己的行)、`build/tlib-packages.tsv` 新包行、`tools/check-ext-testx.cx`、登记行。
- 验收:`sh ./unisacc.com tools/check-ext-testx.cx` 末行 `failed=0`;试点迁移 20 个检查后各自批次 PASS 且断言总数逐文件不变(迁移工具输出 `ASSERTS before=N after=N`);重复普查 ck 定义数 293 单调下降。
- 预估收益:可删约 2300 行(ck 754、has 489、ok 153、check 131、same 79、put 在检查内的份额等);写死 `/tmp` 路径 196 处随迁移清零,消除并发检查互扰一类缺陷。
- 依赖与风险:依赖 P1。风险:迁移即改检查,必须保证断言不删不弱;做法是用 `tools/refactor.cx` 机械改写加每文件前后断言数对拍,不过就还原;批量迁移属"同类第二遍"须造迁移小工具并登记。

### 4.4 P3 表装载:`ext/tsvx/table.cx`(模块 tsvx.table,前缀 tt_)

- 目标:一次装载、按列名取值、键索引、重复键拒绝、列数校验、原子追加;覆盖仓内两种表头(首行表头、`# a<TAB>b` 注释行表头)。
- API 草案:
```c
enum { TT_HEADER_ROW = 1, TT_HEADER_COMMENT = 2, TT_NO_HEADER = 4, TT_STRICT_COLS = 8 };
int  tt_load(int *t, const char *path, long max, int flags);        /* 句柄;内容与行索引归表 */
int  tt_parse(int *t, const char *text, long n, int flags);          /* 拷入一份 */
long tt_rows(int t); int tt_cols(int t);
int  tt_col(int t, const char *name);                               /* 列号或 -ENOENT */
int  tt_get(int t, long row, int col, const char **s, long *n);      /* 借用到 tt_close */
int  tt_getz(int t, long row, int col, char *dst, long cap);         /* 拷成 C 串,放不下 -ERANGE */
int  tt_index(int t, int col);                                      /* 建键索引;重复键 -EEXIST 并可取冲突行号 */
long tt_find(int t, int col, const char *key, long kn);              /* 行号或 -ENOENT */
long tt_line(int t, long row);                                      /* 源文件行号,报错用 */
int  tt_close(int t);
int  tt_append_row(const char *path, const char *const *fields, int n, int expect_cols);  /* 持锁加原子发布 */
```
- 负控(gold 表 `tests/cases/tsvx-table/cases.tsv`):短行与长行在 TT_STRICT_COLS 下 `-EINVAL` 并给行号;重复键 `-EEXIST`;行内 NUL `-EINVAL`;CRLF 按 LF 处理并保留可查;末行无换行照收;空文件 0 行;超过 max `-EFBIG`;关闭后 tt_get `-EBADF`;tt_append_row 列数不符拒绝且文件字节不变;两个进程并发追加各 100 行后行数 200 无交错。
- 与现有:tv_ 四函数保留(流式、零分配场景),tt_ 内部用它;merge3 不动。
- 写域:`ext/tsvx/table.cx`、`tools/check-tsvx-table.cx`、`tests/cases/tsvx-table/`、面表与 layer-api 自己的行、登记行。
- 验收:`sh ./unisacc.com tools/check-tsvx-table.cx` 末行 `failed=0`;试点把 `tools/graph-check.cx`(tx_field 18 处)、`tools/precommit.cx`(17 处)改用 tt_ 后各自批次 PASS、输出逐字节不变。
- 预估收益:手写 field、row、split、fields、line 约 520 行可删;`'\t'` 1187 处里按表读取的部分(估一半)改为按列名,列序变动不再是静默错位;关掉 pkg-gaps 6 行。
- 依赖与风险:依赖 P6 的哈希索引(首版可用排序加二分,不等 P6)。风险:各表方言(空字段、`-` 占位、分号子列表)不统一,首版只管制表符一层,子列表另给 `tt_sub_next` 迭代,不猜语义。

### 4.5 P4 生成器套件:`tools/cxlib/genkit.cx`(模块 cxlib.genkit,前缀 gk_)

- 目标:74 个 `-gen.cx` 共用"读事实表、拼生成物、`--check` 比对或 `--write` 发布"的骨架;这是"事实表 -> 生成器 -> 表"在工具侧的公共件。
- API 草案:
```c
int  gk_args(int argc, char **argv, int *dowrite, const char **root);   /* 统一 --check|--write|--facts [ROOT] */
int  gk_out_new(int *o); int gk_puts(int o, const char *s); int gk_putn(int o, const char *s, long n);
int  gk_puti(int o, long v); int gk_fmt(int o, const char *fmt, ...);
int  gk_emit(int dowrite, const char *path, int o);     /* check:字节相同 0,漂移 1 并打印首个不同偏移;write:原子发布 */
int  gk_ident_ok(const char *s, long n);
int  gk_summary(const char *tool);                      /* 末行 TOOL files=N drift=M */
```
- 负控:生成物改一字节后 `--check` rc 1 并报偏移;`--write` 后 `--check` rc 0;输出目录不存在具名拒绝;gk_fmt 截断时整次生成失败而不是写出残件。
- 写域:`tools/cxlib/genkit.cx`、`tools/check-genkit.cx`、layer-api 的 tools.cxlib 行(加法升 Y 位)、登记行。在 main 上做(工具类)。
- 验收:`sh ./unisacc.com tools/check-genkit.cx` 末行 `failed=0`;迁移每个生成器后 `sh ./unisacc.com tools/derive.cx -- --check` 漂移为 0 且该生成器 `--check` rc 0(生成物逐字节不变是硬门,因为其中有产品输入的生成物);重复普查 one_file 26 -> 0、same_bytes 30 -> 0、copy_field 29 -> 0、app 34 -> 0。
- 预估收益:生成器族冗余 6249 行为上限,其中逐字相同或两种写法的 one_file、same_bytes、copy_field、app、app_n 合计 1147 行可直接删;emit_check、emit_mod、take_row、parse_tsv 共 2684 行需改写,按六成计,合计约 2700 行。
- 依赖与风险:依赖 P3(parse_tsv、take_row 换 tt_)。风险:产品输入的生成器改动后生成物必须字节不变,否则即产品输入变更,须出补丁给集成者核;首批只迁生成物不进产品的生成器。

### 4.6 P5 文件系统便捷:`ext/fsx/`(模块 fsx.fs,前缀 fsx_)

- 目标:路径拼接、整读整写、作用域临时目录、排序遍历、glob、文件与树哈希、锁文件、原子追加,各一个入口。
- API 草案:
```c
long fsx_join(char *dst, long cap, const char *a, const char *b);      /* 同 pt_join,另保证 NUL 结尾;放不下 -ERANGE */
int  fsx_read(const char *path, mg_buf *out, long max);                 /* 0 或 -errno;不存在 -ENOENT */
int  fsx_write(const char *path, const void *p, long n, int mode);      /* 原子发布 */
int  fsx_exists(const char *path);                                      /* 1、0;ENOENT 以外的错误返回负码 */
int  fsx_scratch(int *h, const char *prefix); const char *fsx_scratch_dir(int h); int fsx_scratch_drop(int h);
int  fsx_walk_open(int *w, const char *root, int flags);                /* 排序、可递归、可跳 .git 与链接 */
int  fsx_walk_next(int w, const char **rel, int *kind);                 /* 1 有项,0 结束 */
int  fsx_walk_close(int w);
int  fsx_glob_match(const char *pat, const char *rel);                  /* *、?、** 三种,纯计算 */
int  fsx_sha256(const char *path, char hex[65]);                        /* 流式,不整读 */
int  fsx_tree_sha256(const char *root, const char *glob, char hex[65]); /* 排序路径加逐文件摘要的规范拼接 */
int  fsx_lock(int *h, const char *path, long deadline_ms);              /* 锁文件记属主 PID,属主已死可回收 */
int  fsx_unlock(int h);
int  fsx_append_line(const char *path, const char *line, long n);       /* 持锁、校验无内嵌换行、fsync */
```
- 所有权:scratch、walk、lock 是句柄;fsx_scratch_drop 删整棵目录且只删自己建的(记 dev 与 ino,目录被换成链接则拒绝)。
- 负控:fsx_exists 对 ENOTDIR、EACCES 返回负码不返回 0;scratch 目录被换成符号链接后 drop 拒绝;walk 遇目录环不死循环(深度上限);glob 的 `**` 不跨出根;tree_sha256 对文件名顺序与并发新增文件有确定结果或具名拒绝;lock 到期 `-ETIMEDOUT`,属主 PID 不存在时可回收;append_line 含换行 `-EINVAL`。
- 与 L0:只用 sys.fs、sys.posix、sys.env、sys.sha;win 的目录遍历在面表是 `refuse:ENOSYS`(sy_dir_next 的 win 列),故 fsx_walk 在 win 上如实返回 `-ENOSYS`,不模拟。
- 写域:`ext/fsx/fs.cx`、`cxroot.tsv` 一行、`build/tlib-packages.tsv` 新包行、`tools/check-ext-fsx.cx`、`tests/cases/fsx/`、面表行、登记行。
- 验收:`sh ./unisacc.com tools/check-ext-fsx.cx` 末行 `failed=0`(预计 40 例);`fsx_tree_sha256` 与 `tools/cxlib/receipt.cx` 现算法对同一目录结果逐字相同(差分);试点把 receipt.cx 的 cx_walk 加逐文件拼接换成 fsx_tree_sha256 后 check-receipt 批次 PASS。
- 预估收益:手写 path、load、slurp、save、saved、readfile、exists、absent、walk、digest、cleanup 等约 1500 行可删;`rm -rf`、`mkdir`、`find`、`sha256sum` 四类子进程 231 处可改为进程内调用(每处省一次 fork 加 exec);关掉 pkg-gaps 6 行。
- 依赖与风险:无前置。风险:cx_read 是隐式运行库且 480 文件在用,不强迁;只对新代码立 lint(见第 6 节 E7),存量按文件自然迁移。

### 4.7 P6 参数表:`ext/argx/args.cx`(模块 argx.args,前缀 ax_)

- 目标:用一张静态选项表描述工具的参数,解析、报错与 usage 文案由库生成;子命令是表的嵌套。
- API 草案:
```c
enum { AX_FLAG, AX_STR, AX_INT, AX_MULTI, AX_POS, AX_REST };
typedef struct { const char *name; int kind; const char *meta; const char *help; long lo, hi; int required; } ax_opt;
int  ax_parse(int *h, const ax_opt *spec, int argc, char **argv);   /* spec 以 name 为 0 的行结尾;选项位置无关;-- 之后原样 */
int  ax_flag(int h, const char *name); const char *ax_str(int h, const char *name);
long ax_int(int h, const char *name, long dflt);
int  ax_multi(int h, const char *name, int i, const char **v);       /* 第 i 个取值 */
int  ax_rest(int h, char ***argv, int *argc);
int  ax_usage(const ax_opt *spec, const char *tool, char *dst, long cap);   /* 由表生成 */
const char *ax_why(int h);                                           /* 具名原因:unknown option X、missing value for X、X out of range */
void ax_die(const char *tool, int rc, const char *fmt, ...);         /* 取代 276 份 die */
int  ax_close(int h);
```
- 约定(来自近 100 条提交反复修的行为,定成合同):未识别选项 rc 2 并具名;`--help` rc 0 且输出等于 ax_usage;缺必填项逐个具名;`--opt=value` 与 `--opt value` 等价;`--` 之后不解析。
- 事实表适用处:选项表就是事实;`build/tool-index.tsv` 的 usage 列现由头注释抽取,迁移后可由 `TOOL --help` 的输出生成,头注释与实际解析不再漂移。
- 负控(gold 表):未知选项、重复的单值选项、整数越界、缺值、必填缺失、`--` 后的 `--x` 原样、空 argv、选项名前缀歧义一律按精确匹配拒绝。
- 写域:`ext/argx/args.cx`、`cxroot.tsv` 一行、新包行、`tools/check-ext-argx.cx`、`tests/cases/argx/`、面表行、登记行。
- 验收:`sh ./unisacc.com tools/check-ext-argx.cx` 末行 `failed=0`;试点 devsvc-core 的一个子命令族(doc)改用后 `tools/check-devsvc.cx` 相应组断言全过且 `devsvc doc --help` rc 0(现状 rc 2 并报 bad doc arguments,本次实测)。
- 预估收益:821 处 `strcmp(argv[` 改成表行,行数净减有限(估 800 行),主要收益是缺陷类归零:近 100 条提交中 8 条是参数解析修复。
- 依赖与风险:选项表里不放函数指针(避开跨单元函数指针限制)。风险:各工具现有的容错行为不一,迁移可能改变外部可见的 rc 与文案,须逐工具以其 check 为准,不一致处先补断言再迁。

### 4.8 P7 文本补件:`ext/textx/build.cx` 加法、`ext/textx/lines.cx`、`ext/clex/lex.cx`

- sb 加法:`int sb_fmt(sb *b, const char *fmt, ...)`、`sb_put_i64`、`sb_put_tsv(sb *b, const char *const *fields, int n)`(字段含制表符或换行 `-EINVAL`)、`sb_put_json_str`。目的是让"拼一段文本"不再需要定长栈缓冲。验收:check-cx-build 增例后 `failed=0`;重复普查 put 定义数 125 单调下降。
- 行索引 `li_`:`li_build(int *h, const char *s, long n)`、`li_count`、`li_line(int h, long i, long *off, long *len)`、`li_of_offset(int h, long off)`(二分)。关掉 pkg-gaps 的行索引行;验收为差分(对逐字节数换行的朴素实现)。
- 键值扫描:`tx_kv_get(const char *line, long n, const char *key, long *off, long *len)`(`key=value` 以空白或制表符分隔)。
- C 词法 `cl_`:从 `tools/cx-lint.cx` 的原型与 `cxlib.chainargv` 的 token 化抽成一个包,游标式:`cl_open(int *h, const char *src, long n)`、`cl_next(int h, int *kind, long *off, long *len, int *line)`、`cl_skip_balanced(int h)`、`cl_call_arg(int h, int n, long *off, long *len)`。gold:注释、字符串、字符常量、续行、预处理行、嵌套括号各一组;负控:未闭合字符串与注释给行号不越界。可关 pkg-gaps 4 行,并让 func-callgraph、prechain-gate、limits-audit、cx-lint、chainargv 五处合一。
- 写域:`ext/textx/` 两个文件、`ext/clex/`、对应 check 与 cases。验收命令:`tools/check-cx-build.cx`、`tools/check-textx-lines.cx`、`tools/check-ext-clex.cx` 各 `failed=0`;`tools/check-cx-lint.cx` 与 chain-argv 三批在换库后原样 PASS。
- 预估收益:put、append、join、find、starts、count、lines 约 700 行;词法扫描五份合一约 400 行。
- 风险:sb_fmt 是变参,需确认跨单元变参调用在已装产品上可用(tx_snprintf 已是先例);ext.textx 与 cx/text 的符号链接双身份在 0.0.73 去重前,新文件只放 ext/textx 且不建反向链接。

### 4.9 P8 容器与算法:`ext/algox/`(模块 algox.map、algox.sort、algox.bits、algox.graph)

- API 草案:
```c
int  hm_new(int *m); int hm_set(int m, const char *k, long kn, long v, long *old);
int  hm_get(int m, const char *k, long kn, long *v); int hm_del(int m, const char *k, long kn);
long hm_len(int m); int hm_next(int m, long *cursor, const char **k, long *kn, long *v); int hm_free(int m);
int  it_new(int *t); long it_id(int t, const char *s, long n);        /* 驻留:名字到稠密序号 */
int  it_str(int t, long id, const char **s, long *n); int it_free(int t);
void so_sort(void *base, long n, long elem, int (*cmp)(const void *, const void *, void *), void *ctx);   /* 稳定归并 */
long so_lower(const void *base, long n, long elem, const void *key, int (*cmp)(const void *, const void *, void *), void *ctx);
int  so_strs(const char **v, long n);                                  /* 常用特例:C 串数组按字节序 */
int  bs_new(int *b, long nbits); int bs_set(int b, long i); int bs_test(int b, long i); int bs_free(int b);
int  gr_new(int *g, long nodes); int gr_edge(int g, long from, long to);
int  gr_topo(int g, long *order, long cap);                            /* 有环 -ELOOP 并可取一条环 */
int  gr_reach(int g, long from, int bits); long gr_longest(int g, const long *w, long *path, long cap);
```
- 值类型取 long(可放序号或指针宽整数),不做泛型;需要结构体值的场景用"驻留序号加并行数组"。
- 差分与性质:hm 对现 mm_map 做随机操作序列差分(十万步,结果逐步相同);so_sort 对 libc qsort 加稳定性性质;gr_topo 对输出做"每条边前驱在前"的校验;bs 对朴素字节数组。负控:句柄关闭后使用、容量溢出、gr_edge 越界节点。
- 写域:`ext/algox/` 四个文件、`cxroot.tsv` 一行、新包行、`tools/check-ext-algox.cx`、`tests/cases/algox/`。
- 验收:`sh ./unisacc.com tools/check-ext-algox.cx` 末行 `failed=0`;hm 在 10 万键上单次查找均摊耗时不随键数线性增长(同一检查内 1 万键与 10 万键每操作耗时比小于 3,负控是对线性 mm_map 同测比值大于 5);试点 `tools/func-callgraph.cx`、`tools/graph-check.cx`、`tools/rel.cx` 改用后各批次 PASS。
- 预估收益:20 个文件的手写哈希、41 个文件的 qsort 比较器样板、67 个文件的定长表(触顶即静默或拒绝)可逐步替换;行数估 600;关掉 pkg-gaps 5 行。
- 依赖与风险:比较器回调只用 `void *`,当前产品可编;0.0.69 后可加带类型的便捷宏。mm_map 自身改哈希属产品输入,见第 5 节 L6。

### 4.10 P9 JSON 写出与行式 JSON:`ext/textx/jsonw.cx`(前缀 jw_)

- API:`jw_new(int *w, sb *out)`、`jw_obj_begin`、`jw_key`、`jw_str`、`jw_int`、`jw_bool`、`jw_null`、`jw_arr_begin`、`jw_end`、`jw_close`(未配平 `-EINVAL`);`jl_next(tx_iter_t *it, js_val **v, char *err, long cap)` 逐行解析。
- gold:写出再经 js_parse 读回逐值相同(往返);控制字符、代理对、深度 128 上限;负控:键重复由调用方负责(写明不查)、未配平关闭被拒、非 UTF-8 输入 `-EILSEQ`。
- 写域:`ext/textx/jsonw.cx`、`tools/check-jsonw.cx`、cases。验收:`failed=0`;试点 `tools/guard-hook.cx`(钩子 JSON 输出)与 `tools/facts-gen.cx` 的一段改用后批次 PASS、输出逐字节不变。
- 预估收益:`\":` 887 处里约 600 处在 3 个文件,试点即可消去大半;收益主要是转义正确性。
- 归属:G0(0.0.77)要求 json、url、http 三包按外部包形态发布,jw_ 届时与 js_ 一起迁出为 A 环包;本步只是先孵化。

### 4.11 P10 git 调用:`tools/cxlib/git.cx`(前缀 gx_,只给工具)

- API:`gx_head(char hex[41])`、`gx_tree_of(const char *rev, const char *path, char hex[41])`、`gx_ls_files(const char *pathspec, sb *out)`、`gx_show(const char *rev, const char *path, mg_buf *out, long max)`、`gx_archive_to(const char *rev, const char *dir)`、`gx_dirty(const char *pathspec, sb *out)`;全部经 px_run、不经 shell、带 55 秒时限、剥离 `GIT_` 与 `DEVSVC_` 环境变量(后者 4da17f021c 在检查里修过一次)。
- 验收:`tools/check-cxlib-git.cx` 在夹具仓上 `failed=0`;负控:非仓目录具名拒绝、路径含换行拒绝、rev 以 `-` 开头拒绝(防选项注入)。
- 预估收益:89 个文件 388 处里重复的 argv 组装与输出修剪,估 500 行;统一环境剥离后夹具类检查不再各修一次。

## 5 D 实现顺序与版本归属

### 5.1 不触产品输入、可立即孵化(工具类,不占版本槽)

落点规则:tools/cxlib 的在 main 上做(devsvc 例外规则已覆盖其库);ext/ 新包按 0.7 节 C9"ext 不随产品,不触产品输入"处理,在当期版本分支随行。此条有一处未定:0.0.66 曾把 ext.sysx 的 npx 补丁算作"库包源补丁版"占了一版,ext 新增是否占槽没有明文,须由集成者在首个包入库前裁定;若判占槽,则 W1 至 W8 整体并入 0.0.73。

| 波次 | 内容 | 前置 | 截止判据 |
|---|---|---|---|
| W0 | 把本次 scan.cx 入库为 `tools/dup-census.cx` 加 `check-dup-census.cx` 与底表 `tests/dupfn.tsv`(每名定义数),并入棘轮:定义数只减不增 | 先 `git log --all --oneline -- tools/dup-census.cx` 查名 | `--check` 末行 `FN_SUMMARY ... excess_lines=26213` 与底表一致;往任一工具加一份 `static void ck(` 后 rc 1 |
| W1 | P1 px_run 与等待集(轮询版) | 无 | 4.2 验收 |
| W2 | P2 testx,试点 20 个检查 | W1 | 4.3 验收 |
| W3 | P3 tt_ 表装载,试点 graph-check、precommit | 无 | 4.4 验收 |
| W4 | P5 fsx | 无 | 4.6 验收 |
| W5 | P6 argx,试点 devsvc doc 子命令 | 无 | 4.7 验收 |
| W6 | P4 genkit,先迁生成物不进产品的生成器 | W3 | 4.5 验收 |
| W7 | P7 文本补件与 clex、P8 algox | 无 | 4.8、4.9 验收 |
| W8 | P9 jsonw、P10 cxlib.git | W1、W7 | 4.10、4.11 验收 |

W1、W3、W4、W5、W7 写域互不相交,可并行派五个子代理,每个 15 分钟内只交"面表行加模块骨架加 gold 表加检查",实现分第二轮。

### 5.2 与版本序的对应

| 版本 | 一类大改 | 本规划随行或归入的内容 | 验收命令与预期 |
|---|---|---|---|
| 0.0.68(进行中) | 驱动与运行缓存二期 | 随行:W0 至 W5;库包改进 A 至 D 已在本版 | 各包 check `failed=0`;`tools/layer-api.cx -- --check` rc 0 |
| 0.0.69 | 编译覆盖缺口一期 | 无库源改动;公开后做"带类型回调"第二步:so_sort 带类型宏、fsx_walk 回调形态、cx.stream 阶段回调去强转 | notcovered s01 输出 5;`tools/check-ext-algox.cx` 增带类型回调例后 `failed=0` |
| 0.0.71 | 内存安全乙期 | 随行:新包的创建与关闭函数作为 `#pragma unisa owns` 首批标注对象;`use as alias` 可用后评估缩短前缀(不改已发名字,只加别名示例) | `tools/check-own-escapes.cx` failed=0;`tools/cx-own.cx -- ext/fsx/fs.cx ext/tsvx/table.cx ext/sysx/run.cx` 无新增告警 |
| 0.0.73 | netraw(库包源类);随行改进 E | 归入:ext.memx、ext.textx 与 cx/mem、cx/text 去重时,把 sb_fmt 一族一并定位;px 等待集的 osx 路径实跑 | `tools/check-std-surface.cx` 通过;osx 列由 unverified 转 impl 的格数如实登记 |
| 0.0.74 | 标准核心面封口(库包源类) | 归入 L1 至 L6(见 5.3),全部是产品输入;同时裁定哪些孵化包满足"3 个调用单元"提升判据进 S 环(预计只有 tt_ 的只读部分与 fsx_join,其余留 X 环) | `tools/stable-eval.cx` 六包 S2、S4 ok;L1 至 L6 各自 check `failed=0`;bootstrap-0.0.59 多单元编译通过;七源 `-o` sha 变化逐源说明 |
| 0.0.75 | 门面重导出与包根 | 归入:工具门面 `tools.std`(一行 use 带入 fsx、tsvx.table、argx、sysx.run、testx),仅限 tools、tests、apps;仓外零配置 use(T5)落地后删除规范 3.3 节的软链做法 | `tools/check-cxmod-tri.cx` failed=0 gaps=0;空目录 hello use fsx.fs rc 0 |
| 0.0.76 | 锁文件校验 | 归入:fsx_tree_sha256 作为包树身份的唯一算法(与 CXWORKTREE1 对拍后二选一,不留两套) | `tools/check-cxpkg.cx` failed=0;篡改一字节负控被拒 |
| 0.0.77 | DNS、inflate、G0 | 归入:jw_ 与 js_ 合成 json 包按 A 环形态发布;url、http 在 px、fsx、algox 之上写,暴露出的缺口回填 | E4 清单带包栏 3 过 3 |

### 5.3 必须进产品输入的 L0 项(全部归 0.0.74,出补丁给集成者核,用 bootstrap-0.0.59 多单元编译验证)

| 号 | 内容 | 写域 | 验收 | 关掉的缺口 |
|---|---|---|---|---|
| L1 | 子进程等待集:`sy_proc_wait_any(const sy_child_t *v, int n, sy_deadline_t until, int *which, sy_exit_t *st)`;lnx 用 pidfd 接 sy_poll,osx 用 kqueue 的进程事件,win 用等待多对象,差异在 `_os` 帮手 | `cx/sys/proc.cx`(冻结前缀内,走解除流程)、面表行 | `tools/check-cx-sys-spawn.cx` 增 6 例 `failed=0`;8 个子进程同时结束 50 毫秒内全部收到;`tools/tape-gap.cx` 对三目标带子 gaps=0 | DEVSVC-FLOWEXEC-WAIT |
| L2 | `sy_exit_t` 增 `utime_ms`、`stime_ms`(wait4 的 rusage) | 同上;破坏性签名,并入已列的 11 行一次改 | cpu-meter 改用后与 /proc 读数差小于 20 毫秒 | CPU-METER-RUSAGE |
| L3 | `sy_spawn_t.flags` 增 `SY_SPAWN_PDEATHSIG`(lnx prctl;osx、win 按能力拒绝 `cap=pdeathsig`) | 同上 | 父进程被 SIGKILL 后 1 秒内子进程组不存在(按真实 PID 查);osx 上返回 `-ENOSYS` 且 `sy_cap` 报无 | DEVSVC-CLIENT-PDEATHSIG |
| L4 | `sy_stat_t` 增 `mtime_ns`(已在破坏性签名清单内) | `cx/sys/fs.cx` | 写文件后两次 stat 的纳秒值单调 | C49-131 |
| L5 | `sy_run` 标 legacy,面表指向 `sy_proc_run` 加 np_exec;`sy_kill_tree` 接进 `sy_child_t` 的超时路径 | `cx/sys/proc.cx`、`ext/sysx/process.cx` | 自行 setsid 的孙进程在超时后不存在 | KILLTREE、P1 stderr 分离 |
| L6 | `mm_map` 内部改开放寻址哈希,签名与迭代顺序合同不变(现合同若保证插入序则保留一条顺序链) | `cx/mem/map.cx` | check-cx-buffer 原有断言原样通过;差分对旧实现十万步相同;产品若使用 mm_map 则七源 sha 逐源说明 | P1 strmap |

每项的冻结解除、同候选改完全部调用者(`tools/refactor.cx sites` 加 `find` 补查类型名用法)、csih 车道受影响处先发信封,沿用 0.7 节 C5 的清单做法。

## 6 E 与 devsvc 及工具链的联动

| 号 | 用到哪里 | 现状(实测) | 改后 | 可测量收益 |
|---|---|---|---|---|
| E1 | `tools/cxlib/flowexec.cx` 的节点执行 | 每节点 try_wait 加 2 毫秒睡眠轮询;超时只杀进程组;客户端被 SIGKILL 靠锁租约兜底 | 换 px 等待集;L1、L3、L5 落地后换内核等待 | 空转唤醒归零;结束时刻误差由至多 2 毫秒到事件驱动;关 pkg-gaps 4 行;`tools/devsvc-budget.cx` 的 p50 可对比 |
| E2 | `tools/devsvc-core.cx`(1787 行)与 devtask、flowrun、flowsh 的参数解析 | `strcmp(argv[` 83 处、`, "--` 118 处;`devsvc doc --help` 报 bad doc arguments | 各子命令一张 ax_opt 表,usage 由表生成 | 近 100 条提交里 8 条同类修复不再发生;devsvc-core 估减 150 行;`--help` 对全部子命令 rc 0 可成断言 |
| E3 | devsvc 的 TSV 行协议、配置表 `tools/devsvc-repo.tsv`、precommit 门读集表、graph-check、register | `'\t'` 手拆:devsvc-core 21 处、precommit 19 处、graph-check 19 处;tx_field 按列号取 | tt_load 加按列名取值 | 列序调整不再静默错位;三个文件估减 200 行 |
| E4 | 485 个 `tools/check-*.cx` | ck 293 份、run 138 份、27 份逐字相同的 spawn_pipe_bounded;写死 /tmp 路径 196 处 | testx 加 px | 可删约 5300 行(P1 加 P2);dcheck 并发跑批时固定路径互扰类失败归零(以 tsv-lint 同日 PASS、FAIL、PASS 这类不稳批为观察对象,未定因,不预设结论) |
| E5 | 74 个生成器与 `tools/derive.cx` | one_file 26 份逐字相同;`--check` 与 `--write` 语义各写一遍 | genkit;derive 的 PLAN 行可由 gk_args 统一给出 | 约 2700 行;新生成器样板由约 120 行降到约 30 行(估) |
| E6 | `tools/cxlib/receipt.cx`、relsha、shamemo、toolprep 的闭包内容键 | 三处各自实现"遍历加逐文件 sha 拼接" | fsx_tree_sha256 单源 | 缓存键算法单源(候选池 M 第六条要求);约 150 行 |
| E7 | cx-lint 与棘轮 | 无"禁止新增手写轮子"规则 | W0 的 dup-census 棘轮;cx-lint 加规则:新文件定义 `ck`、`slurp`、`spawn_pipe_bounded`、`one_file` 等底表内名字即告警,并提示对应库函数 | 冗余行数 26213 成为只减不增的读数,每版公开后读一次 |
| E8 | flow 的 search 与 glob 算子(`tools/cxlib/flowfile.cx` 内私有 glob 匹配) | glob 匹配器私有;O6 glob 算子待做 | 匹配器移到 fsx_glob_match,flowfile 反过来用它 | plans/devsvc.md O6 的落地成本降为加一个算子入口 |
| E9 | `build/tool-index.tsv` 的 usage 列 | 从头注释抽取,多行为空 | 由 `TOOL --help`(ax_usage)生成 | usage 为空的行数成为可清零的读数 |
| E10 | `prd_cx_spec.md` 5.1 对照表与 4 节速查 | 推荐 np_run、tx_iter 加 tv_row_next | 每个包入库时把对照表对应行改成新入口,并删 5.2 里已不成立的限制 | 规范与库一致;修掉 2.1 节列的两处文档不一致 |

合计可删量(估算口径:逐字相同的按精确值,其余按可替换比例折算):P1 加 P2 约 5300 行、P4 约 2700 行、P5 约 1500 行、P3 约 520 行、P6 约 800 行、P7 约 1100 行、P8 约 600 行、P10 约 500 行,合计约 13000 行,约为冗余上限 26213 行的一半、tools 总量的 7%。这是推算,不是实测;实测读数由 W0 的棘轮给出。

## 7 F 风险

| 号 | 风险 | 证据 | 对策 |
|---|---|---|---|
| R1 | API 稳定性与升版成本:tools.cxlib 一次接口变更约 6 条命令加 1 处手改、7 个以上文件;加法升 Y、破坏升 X(预稳定期只走 Z 并打印本应的位) | prd_cx.md 0.2 节审视;tools.cxlib 已到 0.3.13 | 新包首版一次把面表行写全再实现,避免逐函数升版;依赖改进 A(派生表一条命令闭环)先落;ext 新包从 0.1.0 起,孵化期不承诺,但迁移了消费者之后的破坏性改动必须同提交改完全部调用点 |
| R2 | 提升窗口:0.0.74 封口后新函数只能先进 ext;S 环升 1.0 后破坏性签名改不动 | prd_cx.md 0.5、0.9 | L1 至 L6 必须赶上 0.0.74;赶不上的宁可留在 ext,不带病进 S 环 |
| R3 | 编译器未覆盖的构造限制包实现 | `tests/notcovered.tsv` 267 条文案,确认被拒 17 条 | 逐条对应:(a) 跨单元函数指针形参带结构体指针被拒(s01),0.0.69 前接口回调只用 `void *`;(b) 文件作用域 `sizeof 对象` 与 offsetof 作常量被拒(y01、r09),表长用哨兵或显式计数;(c) 自动数组达 1000000 字节被拒且 900000 字节点状拒绝(r01、t08),大缓冲走堆;(d) `_Noreturn` 被拒(y09),ax_die 不标注,靠 cx-lint 的 R3-die 规则;(e) 宿主头嵌套成员被拒(t09),纳秒时间只能经 L4;(f) `use as alias` 调用期报 no host function,前缀必须全局唯一;(g) 每单元 use 上限(已装产品与候选不同),一包一至两个模块,门面等 0.0.75;(h) 大缓冲上 `%.*s` 极慢,库的输出函数先拷片段再打印 |
| R4 | 引导约束:产品源改动须能被 bootstrap-0.0.59 多单元编译 | 记忆宫殿决定第 5 条 | L1 至 L6 的实现只用 0.0.59 已接受的写法;ext 与 tools 不受此限,但须用已装产品实编 |
| R5 | 六目标 lowering 缺口 | `tools/tape-gap.cx` 头注:win/x86_64 的 poll 与 syscall 行 selected 0;面表 cx.os win 拒绝 26 格、未核 33 格,cx.sys 拒绝 25 格、未核 20 格,ext.net osx 拒绝 25 格 | 系统包的 win、osx 列如实记 refuse 或 unverified;本文未跑 tape-gap(无带子),L1 与 L3 的三目标 gaps=0 是待取证的验收项,不是结论 |
| R6 | 迁移即改检查,可能削弱断言 | 铁律:不删断言、不缩覆盖 | 迁移用机械改写工具,逐文件输出改前改后断言数并要求相等;每批跑 `tools/affected.cx --files ... --run`;任何时限参数原值带入 px_opts,不取默认 |
| R7 | 按名聚合的统计偏差 | scan.cx 口径;例如 run 138 个文件 130 种函数体,同名不同义的比例未知 | 收益数字一律标推算;以 W0 棘轮的实测读数为准;逐字相同的三组(spawn_pipe_bounded、spool_open、one_file)是确定可删的下限 1600 行左右 |
| R8 | 车道与共享表 | `cxroot.tsv`、`build/tlib-packages.tsv`、`build/std-surface.tsv`、`tests/batches.tsv` 多人同改;apps/csih 归 csih 车道 | 共享表经 devsvc 持锁、`commit-own --keyed` 只带自己的行;不迁 apps/csih 的任何调用点,先发信封 |
| R9 | 隐式运行库与新包并存造成第三套入口 | cx_read 480 文件、cx_runv_timeout 135 文件 | 不新增隐式函数;规范只推荐一个入口;存量靠 E7 的 lint 提示自然迁移,不设强迁期限 |
| R10 | ext 新包是否占版本槽未定 | 0.7 节 C9 写不触产品输入,0.0.66 却把 npx 补丁算一版 | 首包入库前由集成者裁定并写进 prd.md 版本表;本规划两种结果都给了落点(5.1 节) |

## 8 按价值排序的包清单

| 序 | 包与模块 | 一句话 | 证据强度(文件数) | 触产品输入 | 最早落点 | 预估可删行 |
|---|---|---|---|---|---|---|
| 1 | ext.sysx 的 sysx.run(px_) | 带时限、带界、清整树的子进程唯一入口与等待集 | 294 加 135 加 56 加 35 | 否 | 现在 | 3000 |
| 2 | ext.testx(tk_) | 检查的断言、运行、夹具、汇总 | 293 | 否 | 现在,依赖序 1 | 2300 |
| 3 | cxlib.genkit(gk_) | 事实表到生成物的 check 与 write 骨架 | 74 个生成器里 26 至 34 | 否(生成物须字节不变) | 现在,依赖序 4 | 2700 |
| 4 | ext.tsvx 的 tsvx.table(tt_) | 表对象:表头、列名、键索引、原子追加 | 365 | 否 | 现在 | 520 |
| 5 | ext.fsx(fsx_) | 路径、整读写、作用域临时目录、遍历、glob、树哈希、锁 | 399 加 201 加 128 | 否 | 现在 | 1500 |
| 6 | ext.argx(ax_) | 选项表驱动的解析、usage 与具名拒绝 | 183 | 否 | 现在 | 800 |
| 7 | ext.textx 补件与 ext.clex(sb_fmt、li_、cl_) | 带格式拼装、行索引、C 词法 | 256 加 5 处重复词法 | 否 | 现在 | 1100 |
| 8 | ext.algox(hm_、it_、so_、bs_、gr_) | 哈希映射、驻留、排序二分、位集、图 | 41 加 20 加 67 | 否 | 现在;带类型回调等 0.0.69 | 600 |
| 9 | ext.textx 的 jsonw(jw_、jl_) | JSON 写出与行式 JSON | 58 | 否 | 现在;0.0.77 外置 | 约 600 处转义 |
| 10 | cxlib.git(gx_) | 不经 shell 的 git 调用 | 89 | 否 | 现在,依赖序 1 | 500 |
| 11 | L0 六项(L1 至 L6) | 等待集、rusage、PDEATHSIG、mtime 纳秒、sy_run 退役、mm_map 哈希 | pkg-gaps 7 行 | 是 | 0.0.74 | 不以行数计 |
| 12 | 工具门面 tools.std | 一行 use 带入常用包 | use 行 5103 条 935 文件 | 是(cxmod) | 0.0.75 | 约 2000 条 use 行 |
| 13 | json、url、http、DNS、inflate | G0 自用三包 | 生态门 T10 | 部分 | 0.0.77 | 不适用 |

不做清单与理由:线程与事件循环(核心 PRD 非目标,一条缺口记录,devsvc 以多进程加等待集满足)、TOML(仓内配置全是 TSV,零信号)、SQLite(零信号)、正则全集(flow search 只需固定串加 glob;若要做先做 glob 与字面量前缀后缀,不引入回溯引擎)、时区(核心 PRD 非目标)、shell 语言(Bun 的 `$` 模板与"只收 argv 不经 shell"的既定合同相反;"sh -c" 150 处应随 px 与 fsx 减少而不是包装)。

## 9 遗留与待核

- 本文所有"预估可删行"为推算;确定下限只有三组逐字相同函数约 1600 行。
- 未跑:任何批次、tape-gap、六目标;面表 own 列多数为 none(cx.os 115 行里 110 行),是否如实未核。
- pkg-gaps 过期行 3 条(B2f-NPEXEC、B2f-RES、0.0.50 的 tsvx reader)须由持表者改状态;本工单不改表。
- 记忆宫殿地图的版本号比版本序表早一号,须主调度改图。
- 分析脚本在作业目录,作业目录回收后即失;W0 入库是保留这套读数的唯一办法。

### 更正记录

- 2026-11-16:第 7 节 R7 与第 9 节的"约 2600 行"改为"约 1600 行"。依据 tools/dup-census.cx --print 实测:spawn_pipe_bounded 27 份 x 39 行 = 1053、spool_open 27 x 8 = 216、one_file 26 x 12 = 312,合计 1581 行(三组逐字相同函数的确定可删下限,含保留的一份)。其余内容未改。
