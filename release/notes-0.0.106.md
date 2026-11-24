# unisacc 0.0.106 发布说明


状态：同一候选通过六格原生 CI，结论为 QUALIFIED（run 45446325698）。本版未执行全套 s9，未取得全套 s9 资格。

## 真实身份

| 对象 | 身份 |
|---|---|
| 冻结提交(构造输入、发布标签目标) | 2e8a19a342251910d53010f51002b14a4ca7f916 |
| 发布 APE SHA256(unisacc.com) | 5f83ac93d201b252ea0b56098475be9e30045875cdf1ce692451cda095ceeb3c |
| CI run | 45446325698 |

## 产品改动

(1) 转发桩显式分相与隔离:桩文本、sidecar 源与收据改由 cx/driver/fwdreplay.cx 在 A/B 两相生成,共享 TMPDIR 下并发回放互不覆盖,损坏或缺失的收据按具名拒绝。证据:cx/driver/fwdreplay.cx、cx/compiler/fwdstub.cx、src/drive.cx、src/driveroute.cx(661de08935)。

(2) FFI 头拆分:include/unisacc_ffi.h 的转发声明移入新头 include/sys/_ffi_forward.h,公开头只保留薄壳;Windows 上桥接函数定义以 `#ifndef _WIN32` 收口(b150aa37e2)。证据:include/unisacc_ffi.h、include/sys/_ffi_forward.h。

(3) 运行缓存跨目录路径漏钉修复:缓存键纳入路径身份(cx/runtime/rcident.cx 与新增 cx/text/pinscan.cx、src/runcache.cx),不同目录的同名输入不再互相命中(c8d03bb33f)。

(4) parse2 转发结果表:exec/parse2/forward-result.tsv 随引擎行更新,转发调用的结果分类与前端一致(578c617de6,引擎行取自 daae75b8)。

(5) 环境变量活表:env-semantics 的活表自本版起为 tests/env-semantics.tsv;本区间 exec/facts/env-semantics.tsv 只随 b150aa37e2 作为旧副本同步,读者与检查以 tests/ 表为准。

(6) 起链前修补一批:lane-inputs、dup-census、unstage、preflight 第 19 节点、no-home-path、core-a 与 ownernorm 一次合入(b150aa37e2);kernelsha pin 过期修复 r2 与 preflight 第 9 节点随冻结提交刷新(2e8a19a342)。

(7) 预合并分支选择:premerge-union 按方案 B 选择分支,避免两侧同改表时的误合(6faff97eed)。

(8) 覆盖声明:本版整链全绿(REV=2e8a19a342);预跑原 merge 为 503 批 PASS=495 FAIL=6 UNKNOWN=2,同候选后补七条原 FAIL 行后合并为 PASS=503 FAIL=0 UNKNOWN=2(见严格证据与链后补跑节);此段是覆盖事实,不构成全批资格声明。

## 严格证据

证据 release/evidence/v0.0.106-strict-2026-11-24.tsv:rows=526,expected_fail=0,superseded=7;预跑原 merge 为 503 批;合并补跑 PASS 行后 SUMMARY 为 batches=505、PASS=503、FAIL=0、UNKNOWN=2、INVALID=0。七条被取代的原始 FAIL 行均保留在原表并标 SUPERSEDED,原行不改写、不删除。FAIL 批与上一版对照见 release/evidence/0.0.106/expected-fail.tsv,标 NEW 的不自动放行。

## 已知缺口与未验证范围

(1) 严格证据批次 cx-booter-ape 状态 UNKNOWN(无 RAW BATCH_RUN 行),未覆盖,不记 PASS;0.0.105 同形,属预期 UNKNOWN,不是 INVALID。
(2) 严格证据批次 tools-default-regression 状态 UNKNOWN(无 RAW BATCH_RUN 行),未覆盖,不记 PASS;0.0.105 同形,属预期 UNKNOWN,不是 INVALID。
(3) s9 全套本版未跑;严格证据所列 505 批之外的批次 UNMEASURED。
(4) COMMIT-REGISTERED-SERIAL-OUTER-WINDOW:commit-own --registered 串行登记一致性后再进真实预提交门，A5 regm 24057ms 与门共用 60 秒外窗(build/pkg-gaps.tsv,OPEN)
(5) D2-FUNCTION-TYPEDEF-RETURN:经 typedef int F(int) 的 F *g 调用并使用 g 的返回值仍拒绝，文案 not covered: invalid return-type source category；0.0.86 同样，void 返回的对照可编(build/pkg-gaps.tsv,OPEN)
(6) D2-FUNCTION-TYPEDEF-EXPLICIT-PROTOTYPE:函数 typedef 形参对等的显式函数指针定义仍有拒绝，例如 typedef int F(int); void h(F); void h(int (*q)(int)){(void)q;}；无原型 typedef int F() 配 (int) 同样(build/pkg-gaps.tsv,OPEN)
(7) D2-DOUBLE-PARENTHESIZED-PARAMETER-NAME:双层括号形参名 void f(int ((a))); 与 void f(void ((cb))(int)); 仍具名拒绝，第九链与 0.0.87 单元 2 均可复现(build/pkg-gaps.tsv,OPEN)
(8) D2-PROTOTYPE-DEFINITION-EDGE-CASES:K&R 定义前置有原型或无原型时有合法形态仍拒；空形参表定义与显式参数原型两序误收；函数 typedef 直接声明后冲突定义、函数返回函数、函数元素数组和结构成员误收，独立样本见 ~/.claude/jobs/2798e440/tmp/gk-d2p2.SyDm9o/keep/gkx6.cx(build/pkg-gaps.tsv,OPEN)
(9) D2-VOID-ARRAY-DECLARATOR-EDGES:0.0.86 第九链与 0.0.87 单元 1 终版均误收 typedef void V; void f(V []);、typedef void V; void f(int, V [3]);、void f(int, void (*)[3]);，均误拒 void f(int, void (*[])(void));，见 ~/.claude/jobs/2798e440/tmp/gatekeeper-d2-p1-r2.md(build/pkg-gaps.tsv,OPEN)
(10) D2-VOID-POINTER-ARRAY-PARAM-RUNTIME:守门人核 0.0.86 第九链正式件：具名 void *v[] 形参的定义取元素出现段错误，修复抽象形参分派前已存在；较小的索引和解引用样本未复现，见 ~/.claude/jobs/2798e440/tmp/gatekeeper-d2-p1.md(build/pkg-gaps.tsv,OPEN)
(11) AFFECTED-PRODUCT-INPUT-TOO-COARSE:D2 discards-const 的 9 个变更路径经 affected 选出 514 批,外层 59 秒超时;诊断表的改动经 exec/ 等前缀引入宽选(build/pkg-gaps.tsv,OPEN)
(12) DVQUALIFY-RESUME-MULTI-SUMMARY-VS-G1:dvqualify 多次续跑可在同一表头下追加多个 SUMMARY，G1 的 UNKNOWN 计数与这些分窗汇总不一致(build/pkg-gaps.tsv,OPEN)
(13) REAL-COMMIT-COLD-PREBUILD-NOT-AUTOMATIC:新候选单提交布局首次运行 real-commit 时，devsvc 固定版本冷构造与真实提交门叠加，T3 外窗 60050ms 超时(build/pkg-gaps.tsv,OPEN)
(14) SCRIPT-BENCH-JSON-SERIALIZE:已解析 JSON 树缺少通用紧凑序列化接口(build/pkg-gaps.tsv,OPEN)
(15) NP-PROCESS-UNIFY:模块重导出或同名符号合并(build/pkg-gaps.tsv,OPEN)
(16) OSX-TOOL-SELF-PATH:Darwin sy_self_path实际返回-2，L0仍读/proc/self/exe；需要OS实际进程映像规范路径与容量合同(build/pkg-gaps.tsv,OPEN-L0-TOOL-BRIDGE-VERIFIED)
(17) P1:func-callgraph.cx:字符串->整数的哈希映射(按名字+长度查函数序号)(build/pkg-gaps.tsv,OPEN)
(18) P1:func-callgraph.cx:位集/稠密邻接矩阵+图可达闭包(DFS)(build/pkg-gaps.tsv,OPEN)
(19) P1:func-callgraph.cx:文本按行索引(行首偏移数组)与偏移->行号二分(build/pkg-gaps.tsv,OPEN)
(20) P1:func-callgraph.cx:C 词法扫描器库(跳过注释/字符串/字符常量/预处理续行,吐 token 或平衡括号事件)(build/pkg-gaps.tsv,OPEN)
(21) P1:func-callgraph.cx:通用排序(qsort 式比较回调)(build/pkg-gaps.tsv,OPEN)
(22) P1:func-callgraph.cx:子进程捕获 stdout 与 stderr 分离(build/pkg-gaps.tsv,OPEN)
(23) C0.2/BASE-1:带schema/重复键拒绝的流式TSV闭包解析与规范路径文件内容摘要(build/pkg-gaps.tsv,OPEN)
(24) BASE-2/BASE-6:配方ID/目录/flags公开只读枚举与统一执行记录(build/pkg-gaps.tsv,OPEN)
(25) cx-mm_copy-dcap-real-capacity:R1补丁约90处mm_copy目标容量参数取拷贝长度n,边界检查形同虚设(build/pkg-gaps.tsv,OPEN)
(26) G1:graph-check:TSV 表装载(注释行/表头核对/按列取字段)与 DAG 工具(邻接表、DFS 环检测、前缀通配依赖展开)没有库包;一次性草案转表(graph-finalize)也要手写子串替换(build/pkg-gaps.tsv,OPEN)
(27) cdx:C49-131总墙钟取证:.cx读取文件mtime纳秒；struct stat.st_mtim嵌套成员已装产品报not covered: member access，sy_stat_t仅秒级mtime(build/pkg-gaps.tsv,OPEN)
(28) cdx:C49-X分相位编排:稳定公开的前端批处理及首遍e3转发桩API缺失(build/pkg-gaps.tsv,OPEN)
(29) cc:0.0.50 构造图分析:仓外 .cx 脚本解析不到 #pragma unisa use 的模块(报 no file /tmp/.../sys/fs.cx)(build/pkg-gaps.tsv,OPEN(复核 2026-11-20 已装 0.0.85 仍成立:$CLAUDE_JOB_DIR 下 t.cx use sys.fs 报 no file )
(30) cc:0.0.50 构造图分析:DAG 拓扑排序与最长路(关键路径)(build/pkg-gaps.tsv,OPEN)
(31) cc:0.0.58 tlib 接线测量:列举目录下的 *.cx(一次性测量脚本扫调用点)与关运行缓存跑子进程并计时(build/pkg-gaps.tsv,OPEN)
(32) cc:0.0.58 cx.stream:导入接口里函数指针形参带结构类型(typedef 结构指针、struct 指针、const 结构指针)被拒:error: not covered: incompatible semantic declaration type;最小复现:模块 m 写 typedef struct { int a; } tt; int f(int (*fn)(tt *, int), void *c) { return fn(0, 1); },另一单元 #pragma unisa use m 后默认 -run 即报;只含标量与 void * 的函数指针形参正常,模块单独 -c 也正常(build/pkg-gaps.tsv,OPEN)
(33) W1:std-surface:TSV 按行条件改写(给字段数不足的行补占位列)与整表列数校验;仓外脚本用不了 #pragma unisa use(模块根按脚本目录解析)(build/pkg-gaps.tsv,OPEN)
(34) README-ANCHOR:markdown 标题锚点检查器(tools/doc-links.cx 只核路径不核 #anchor)(build/pkg-gaps.tsv,OPEN)
(35) README-PAIR:入库的中英 README 对检查器:两文件 <a id> 序列、表内链接目标序列、顶部相对互链、表行数与链接数一致,目标文件与 #anchor 存在(build/pkg-gaps.tsv,OPEN)
(36) cc:DOC-CHECKS-V1:文档目录扫描(按后缀列文件)与 md 锚点/链接抽取(build/pkg-gaps.tsv,OPEN)
(37) PRECHAIN-SRC-SCAN:C 源码级调用参数/超时分支静态提取(调用图、参数切分、timed_out 控制流)(build/pkg-gaps.tsv,OPEN)
(38) CPU-METER-RUSAGE:子进程 CPU 时间(getrusage/wait4/times 的 L0 包装,用户态加内核态,单位毫秒)(build/pkg-gaps.tsv,OPEN(复核 2026-11-20 已装 0.0.85 仍成立:无 getrusage 包装))
(39) EVIDENCE-COVER-V1:两树目录差异比较与 git archive 取树库包(不依赖外部 git/tar/diff 子进程;含符号链接按链接比较)与带 cwd 的 sy_run(build/pkg-gaps.tsv,OPEN)
(40) RECEIPT-GLOB-TREEHASH:glob 展开(src:DIR/**)与目录/git 树内容哈希库函数,以及 git rev-parse 取树哈希不经 shell(build/pkg-gaps.tsv,OPEN)
(41) -:按前缀匹配目录并逐目录扫 .log 的 glob/walk 小库(batch-cost 手写 sy_dir_next 两层循环)与按行/键值(key=N)扫描文本的库函数(build/pkg-gaps.tsv,OPEN)
(42) PATCH-QUEUE-V1:TSV 行/列拆分与文件 sha256 的一步库函数(patch-queue 手写按行 strchr 拆列, 复用 cx_sha256_hex)(build/pkg-gaps.tsv,OPEN)
(43) ITER-8:C 源码调用点字面量参数抽取(np_run/pr_run/eb_run 第 N 参数)与 tsv 行拆列;prechain-gate 无导出可复用(build/pkg-gaps.tsv,OPEN)
(44) REH-2:录像按 argv 精确匹配无法与夹具用的假仓库名共用,需 gh-record 的占位符/规范化回放(repo,tag,id 参数化)(build/pkg-gaps.tsv,OPEN)
(45) LEDGER-ADD:TSV 逐行按列数扫描、原子追加一行(临时文件+rename+flock)无库包(build/pkg-gaps.tsv,OPEN)
(46) A12:tools/speed-probe.cx 只做每对象编译计时且需 JOBS.tsv(默认 recipe 的 out/c44-five-lnx 不存在),无首次运行分相;run-startup-bench 分相全 UNMEASURED(build/pkg-gaps.tsv,OPEN)
(47) B2e-NB:sy_tcp_connect_to 与 sy_tcp_dial 需非阻塞 connect 后恢复阻塞,cx.sys 无 sy_sock_nonblock(fcntl O_NONBLOCK)与 SOCK_NONBLOCK 名;另 sy_resolve 无 DNS 客户端(UDP 53 报文编解码与 resolv.conf 读取),/etc/hosts 未读(build/pkg-gaps.tsv,OPEN-DNS-ONLY)
(48) B2f-RES:sy_res_live 需要句柄型资源的统一登记表(fd/进程/映射的开闭计数钩子),cx.sys 各单元无共用登记点,需先定 kind 枚举与各 open/close 的计数合同(build/pkg-gaps.tsv,OPEN)
(49) B2f-NPEXEC:np_exec 需带界捕获 stdout/stderr:cx.sys 有 sy_spawn_pipe 与 poll,但 ext.procx 无包目录(np_run 在 ext.sysx),需双管道轮询读+cap 超限杀子进程+mg_buf 追加,单窗口 12 分钟内无法做对(build/pkg-gaps.tsv,OPEN)
(50) X7-RRSTEPS:RR_TIME inference 行只有 stage、rc、ns,无每流执行步数字段(cx/runtime/routetime.cx:90 rr_infer),无法换算每步成本(build/pkg-gaps.tsv,OPEN)
(51) R2-HOTFAIL:取被执行函数集合的入口(给定入口集合算出可达函数闭包,供惰性 lower 与 elf 只处理被执行的函数)(build/pkg-gaps.tsv,OPEN)
(52) R3-ELF-SECDIFF:ELF 逐节对拍(两个 .o 或两个候选二进制按节比较:节名、大小、sha256、首个不同偏移,并能把差异归到 .strtab/.unisa.tape/.text/.data/.rela.text)(build/pkg-gaps.tsv,OPEN)
(53) DEVSVC-FLOWEXEC-WAIT:子进程结束的事件式等待(SIGCHLD 或 pidfd 可 poll),一次等多个子进程并带截止(build/pkg-gaps.tsv,OPEN)
(54) DEVSVC-FLOWEXEC-KILLTREE:超时杀整棵后代树(含自行 setsid 脱离进程组的后代)(build/pkg-gaps.tsv,OPEN)
(55) DEVSVC-FLOWEXEC-THREADS:进程内并行执行(线程或等价的工作者),供引擎内联算子并发(build/pkg-gaps.tsv,OPEN)
(56) DEVSVC-CLIENT-PDEATHSIG:客户端被 SIGKILL 时节点进程组随之终止(build/pkg-gaps.tsv,OPEN)
(57) DEVSVC-DERIVE-FLOW-RUNCACHE:产品 -run 编译缓存按内容取键(内容相同换路径不冷编)(build/pkg-gaps.tsv,OPEN)
(58) CX-PNG-INFLATE:.cx 缺 PNG 解码(inflate)(build/pkg-gaps.tsv,OPEN(复核 2026-11-20 已装 0.0.85 仍成立:ext/textx/deflate.cx 只有压缩无 inflate))
(59) D2D3D4-REF-FRAMES:本机无 ffmpeg/yt-dlp,无法按秒精确抽帧(build/pkg-gaps.tsv,OPEN)
(60) RUNTIME-FOPEN-WX:运行库 fopen 的 "wx" 模式应拒绝已存在文件(独占创建)(build/pkg-gaps.tsv,OPEN(复核 2026-11-20 已装 0.0.85 仍成立:fopen(已存在,"wx") 返回非空))
(61) LINT-R4-OUTPUT-ASSERT:cx-lint R4 需要给"对产品输出文本做包含断言"一个正式写法(不是源码锚点)(build/pkg-gaps.tsv,OPEN)
(62) DEVSVC-DERIVE-CHECK-ALL-WINDOW:derive --check 全量逐表合计约 45 秒,加上 git merge 或 commit 的收据复核装不进单个 55 秒窗口(R1 前超窗,R1 后 28 到 45 秒视冷热)(build/pkg-gaps.tsv,OPEN)
(63) FRONTEND-STRUCT-MEMBER-CAP-64:exec/parse2/control-result.tsv:568-570 的 reject18 判据:当前结构体已有成员数不小于 64 就拒绝(not covered: structure member capacity),匿名 struct/union 拍平计入外层;步长 64 写死在 control-result、initializers-result:15、librarytypes-result 三张生成表,基址在 exec/facts/k2-gen2.tsv(build/pkg-gaps.tsv,OPEN)
(64) DIAG-LINE-OFFSET:诊断行号比源文件行号偏大且偏移不固定(sqlite3.c 实测 +147 与 +105),来源未查(build/pkg-gaps.tsv,OPEN)
(65) SY-RANDOM-MISSING:运行库没有随机数或熵源接口(sy_*)(build/pkg-gaps.tsv,OPEN(复核 2026-11-20 已装 0.0.85 仍成立:cx/sys ext tools/cxlib 无 random/getrandom))
(66) EXPLICIT-CACHE-MANAGER-CHECKPOINT-IDENTITY:显式管理器的 DONE 行仅标相名，未绑定 x64 与 arm64 的内容身份(build/pkg-gaps.tsv,OPEN)
(67) CHAIN-PACK-VERSION-GATE:整链打包后 APE 未实跑 --version 对照目标版本;v0.0.72 链产物 0c66aba7 自报 0.0.71(build/pkg-gaps.tsv,OPEN)
(68) VERSION-SLOT-WRITER:版本槽行无生成或登记工具:v0.0.71 与 v0.0.73 都是手写一行(commit 绑分支基点、ci_run_id 0、六件摘要全零)(build/pkg-gaps.tsv,OPEN)
(69) DIAG-CENSUS-TYPES:S0 普查对类型或作用域相关的 8 类(assign-to-const、implicit-function、discards-const 等)只能记 UNMEASURED:没有可供 .cx 调用的 C 语义查询(声明作用域、限定符、指针类型)(build/pkg-gaps.tsv,OPEN)
(70) DEVSVC-WORKTREE:devsvc git 不提供 worktree add,建版本分支工作树只能直调 git worktree add(build/pkg-gaps.tsv,OPEN(复核 2026-11-20 已装 0.0.85 仍成立:devsvc help 无 worktree add))
(71) S1-AFFECTED-PP-PRECISION:affected 对 exec/pp 表改动给出 405 批(interfaces 253、contract 160),却不含 pp-cli-*、pp-header-locations、pp-differential、pp-targets-lines、c99-whitespace 等真正跑预处理的产品批;全量一窗 5 批,估 81 分钟(build/pkg-gaps.tsv,OPEN)
(72) S1-E-DIAG-TEXT:-E 入口遇预处理诊断只打印 unisacc: model driver: e2(rc 2),丢掉 file:line: error: 文案;-c/-S/default-run 有完整文案(build/pkg-gaps.tsv,OPEN)
(73) S1-SIM-PROBE-WRAPPER:快车道 sim.cx 跑 pp delta 需手给 --resource cli/target=FILE 与 predefines/TARGET=FILE(NUL 分隔名单),无文档无一条命令的探针入口(build/pkg-gaps.tsv,OPEN)
(74) S1-DOMAIN-INDEX-DRIFT:v0.0.73 分支上 affected --domain 报 index-drift(batch-domain-index 与 batch-domains-gen 不一致),域过滤不可用(build/pkg-gaps.tsv,OPEN)
(75) S2-INLINE-REJECT-NO-LOCATION:parse2 内联 REJECT 动作的拒绝只打 reject: 文案,无 file:line;HL.static 用 JUMP hl_pos 加 MARK tpos 才带位置。S2 六项均无位置(c-negative 只把位置记 INFO)(build/pkg-gaps.tsv,OPEN)
(76) S2-RUNBATCH-CANDIDATE-ROOT:run-batch --candidate 对 c-negative、pp-negative 无效:批次参数 {root} 把已装启动器交给工具,candidate= 字段非空但实测的是已装产品(44/15、8/2)(build/pkg-gaps.tsv,OPEN)
(77) S2-PARSE2-SEEDGEN-SLOW:parse2 delta 76 MB,产品版 -O0 seed-gen 每次 42 s,table-lint 43 s,四次重生成占快车道大半(build/pkg-gaps.tsv,OPEN)
(78) S2-TEMPLATE-NAMED-STATE-TRAP:update-result 的 compound-body 按运算符模板多次实例化,具名(非 $)状态全局共享,插入具名状态会把各运算符副本合流(-= *= 被改成 add);table-lint 不报(build/pkg-gaps.tsv,OPEN)
(79) FREEZE-VERSION-SLOT-PREP:冻结前缺版本槽准备:冻结提交里的 release/candidate.tsv 仍是上一版行,整链打包按快照取版本(build/pkg-gaps.tsv,OPEN)
(80) REACT-CANDIDATE-KEY-MISSED-INPUTS:产品键排除构造执行器，工具行为或版本配方改变时需独立构造证据；身份槽沿用 readset:product-map 名称(build/pkg-gaps.tsv,OPEN-MANUAL-REBUILD)
(81) DEVSVC-MERGE-RETRACKS-UNTRACKED:devsvc git merge 自动解决 modify/delete 冲突时把 main 已停止跟踪的 build/batch-packages.tsv 又纳入跟踪(v0.0.73 合并时发生,用 devsvc git commit --untrack 纠正 6292aca241)(build/pkg-gaps.tsv,OPEN)
(82) RELEASE-RUN-NO-COMMIT-BACKFILL:tools/release-run.cx --pack 不回填 release/candidate.tsv 的 commit 字段(版本槽行里 commit 是基点占位),按原样打包时 build.json 绑定占位提交(v0.0.73 发布单在私有副本里手改成冻结提交再打包)(build/pkg-gaps.tsv,OPEN)
(83) EVIDENCE-DEPENDS-ON-TMP-CLEANUP:发布打包依赖的 cache-manager 在 /tmp/unisacc-session-v072-chain2 下,被别的窗口清理后打包被拒(v0.0.73 发布单用 cache-retain.cx 重建,逐字节相同)(build/pkg-gaps.tsv,OPEN)
(84) V74-SIM-FANOUT:parse2 快车道 sim 探针一次跑 19 份(base 与 new 各一遍)无一条命令的入口:单个 devsvc x 内 4 路循环 58 s 超时,子进程在 x 超时后残留需按 PID 手杀(build/pkg-gaps.tsv,OPEN)
(85) V74-BASE-DELTA-CACHE:字节等价需要 base 侧 parse2 delta,每次重生成 41 s;同一表内容身份的 base delta 未按身份复用(build/pkg-gaps.tsv,OPEN)
(86) V74-INSTALLED-ONLY-BATCHES:formats-objects、negative-diagnostics、negative-hostile、ri-image-single、ri-image-multi、ri-route-refuse 硬要求 argv 为 ROOT/unisacc.com(installed-product-required),run-batch --candidate 开发产品时 rc 2,产品输入改动在安装前无法用这些批次验证(build/pkg-gaps.tsv,OPEN)
(87) QUALIFY-NO-AUTO-SEGMENT:devsvc qualify 的 product-qualification 全量(574 批)按当前速度超过总上限 1800 秒会被 devsvc 杀掉,只能 --list 加 --ids 手工分段(build/pkg-gaps.tsv,OPEN)
(88) CHECK-DVQUALIFY-COLD-COMPILE:tools/check-dvqualify.cx 第 4 项实跑 devsvc --dev qualify,devsvc 核心无编译缓存且机器高载时可能超 55 秒窗口(想改成直接查源码被 R4-source-anchor 拦下)(build/pkg-gaps.tsv,OPEN)
(89) QUALIFY-FIRST-TWO-SPAWN-FAIL:devsvc qualify 每轮最先起跑的两批 60 秒 UNKNOWN(strict2/3/4/5 四轮均如此:struct-member-capacity 与 check-struct-member-capacity 记 rc=124 ms=60022 note=spawn-failed;retried),同一批在同一布局 /tmp/unisacc-qualify-63334e18 手跑 affected --ids check-struct-member-capacity --run 仅 6.9 秒 PASS;重跑一次后多数转实值(build/pkg-gaps.tsv,OPEN)
(90) RUN-BATCH-COLD-VS-DIRECT-3X:同一产品 63334e18 同一工具 struct-member-capacity:直跑(命中产品跨进程缓存)reject 全 15 项 19.0 秒(load 4.7)、accept 10.0 秒;经 run-batch(冷缓存或全新目录)reject 54725 ms 未完成 transport=-1(load 2.3),structs2040 的 -c -funit 单步 31 秒(16:39:03 -> 16:39:34),accept 52249 ms;关缓存直跑 UNISACC_RUN_CACHE=0 UNISACC_UNIT_CACHE=0 每夹具 12 到 23 秒(含工具冷编约 10 秒)(build/pkg-gaps.tsv,OPEN)
(91) CI-TIMEOUT-DROPS-OUTPUT:候选 CI 子检查超时时保留已捕获的 stdout、stderr 各末 60 行(build/pkg-gaps.tsv,OPEN)
(92) CI-OSX-X64-STDIO-CROSSUNIT-MARGIN:osx/x86_64 runner 的 stdio-crossunit 用时 44.3 秒,占 55 秒窗口约 81%(build/pkg-gaps.tsv,OPEN)
(93) tools/register.cx 登记批次时只用已装产品跑一次,失败即 ROLLED_BACK;在私有副本里把 unisacc.com 指向开�:缺 --runner ABS_DEV_PRODUCT_DIR(透传 tests/run-batch.cx --candidate)(build/pkg-gaps.tsv,OPEN)
(94) REGISTER-NO-PRODUCT-INPUT-DECL:register.cx 登记参数含 {candidate} 等 require 占位符的批次时不写 tests/batch-product-inputs.tsv 的 id 行,precommit 与 register 也不核该声明,缺口直到 affected 运行才以 BATCH_PRODUCT_INPUT_UNDECLARED 暴露(funit-noprologue 等七批)(build/pkg-gaps.tsv,OPEN)
(95) NN-VERIFY-ELF-INIT:-c -funit 对象初值与const入.bss无.data/.rodata/.init_array,gcc ld 链接读0(v0.0.76候选仍在)(build/pkg-gaps.tsv,OPEN)
(96) NN-VERIFY-FUNIT-SYSV:-funit对外函数首参取%rax非SysV %rdi,gcc main调add_one(0)得17(v0.0.75序言已修,v0.0.76仍在)(build/pkg-gaps.tsv,OPEN)
(97) UNIT-CACHE-PARALLEL-FIRST-COMPILE-LOSES-STORES:全新缓存目录(XDG_CACHE_HOME 新建)里并行首次编译 5 个工具丢 2 个运行缓存存储(第二次调用 lookup_hit=0 仍冷编),产品缓存并发首写缺陷,UNISACC_UNIT_CACHE 开启口径下 2026-11-19 实测(build/pkg-gaps.tsv,OPEN)
(98) RELEASE-EXEC-WARM-CANDIDATE-APE-NOT-PREWARMED:公开读回里候选 APE 自己的 smoke 编译(srcshape、hostc)未预热:读回下载路径每次新建而运行缓存按产品路径区分(实测同字节两路径互不命中);要预热需固定下载路径,会改 c47-1-readback 合同,留后续(build/pkg-gaps.tsv,OPEN)
(99) V077-BATCH-DOMAIN-INDEX-STALE:T2 ff3a87fd35 的 tests/batch-domain-index.tsv 缺 6 个已登记批次(commit-merge net-health objectplan-pair own-flow-product table-cover tools-compile-158),derive batch-domains REFUSE index-drift,qualify --set product-qualification 因此漏批(build/pkg-gaps.tsv,OPEN)
(100) V077-DERIVE-CHECK-DRIFT-T2:T2 布局 derive --check 两次同结果 rc 2:drift=6(iface-hash links libc-decls doc-links table-index cx0-three-source) refused=3 含两项 30 秒 TIMEOUT(batch-packages-gen chain-phases,load 11 时)(build/pkg-gaps.tsv,OPEN)
(101) V077-QUALIFY-TLIBKIT-COLD-TIMEOUT:UNISACC_UNIT_CACHE=0 下 affected --prepare-materials 的 tlib-kit 准备 40 秒 bound 超时(rc 142),每批 60 秒外窗被吃尽,T2 预跑 16/16 UNKNOWN(含 notcovered 这类轻批)(build/pkg-gaps.tsv,OPEN)
(102) DVQUALIFY-TLIB-KIT-COLD-40S:新候选的 dvqualify 布局里 affected.cx kit_prepare 以 40 秒 bound 跑 tlib kit,tools.cxlib 84 单元冷编单跑 41 秒(整 kit 52.7 秒,load 7.4),被杀后对象不留(obj_reused=0),三路并发每批重来,首段全批 rc 3/124 UNKNOWN;kit 落 ~/.cache/unisacc/tlib/kit 后 0.6 秒命中(build/pkg-gaps.tsv,OPEN)
(103) OWN-FLOW-SHADOW-RESETS-OUTER-FREED:safe 单元内层块声明同名变量遮蔽外层后,出块时外层已释放状态被清零(cxf_block 的 vd>depth 清零),char *p=malloc(4); free(p); { char *p=malloc(2); free(p);} return *p; 编过无诊断(0.0.77 独立复核 a2)(build/pkg-gaps.tsv,OPEN)
(104) OWN-FLOW-LEX-FAIL-SILENT-SKIP:safe 单元词法失败(括号不配对、token 超 65536、括号嵌套超 4096,#if 分支里括号不平衡很常见)时 cxf_unit 首行 cxf_lex 失败整单元静默放过,一个发现也不报(0.0.77 独立复核 a4)(build/pkg-gaps.tsv,OPEN)
(105) OWN-FLOW-CAPS-SILENT-MISS:safe 单元容量上限超出时静默漏判:第 65 个变量起不跟踪、第 33 个 goto 标签起忽略汇合、goto 只跑 2 遍非不动点、循环嵌套超 64 层跳过循环体可越块吞下一语句(读代码得出,未逐一复现)(build/pkg-gaps.tsv,OPEN)
(106) OWN-FLOW-FALSE-REJECT-SIZEOF-AND-CONDFREE:safe 单元误拒两类:free(p); n = sizeof *p;(无括号 sizeof 只看前一 token);if (c) free(p); return c ? 0 : *p;(路径不敏感,任一路径已释放即算已释放,设计结果应写进说明)(build/pkg-gaps.tsv,OPEN)
(107) COMMIT-MERGE-BYTE-EQUAL-NOT-ASSERTED:check-commit-merge 的 plain-add/modify/delete 三项只断言 rc、单父、说明、树,未对新旧 commit-own 比较提交号(提交说明里的逐字节相同只是手工对拍);release-exec gate_main 浅克隆下 merge-base 返回 1 会误标 MAIN_ADVANCED 而非 MAIN_UNKNOWN(0.0.77 独立复核)(build/pkg-gaps.tsv,OPEN)
(108) DEVSVC-LOADGATE-STARTS-AFTER-WAIT-TIMEOUT:cdx2 自述(ccapi 转,未核):验收批次在 load1 小于 4 时启动,devsvc 批内等候负载超时(dvqualify 发车门有界等待 120 秒)后仍启动后续节点,与冻结窗"load1 高于 4 不起批"让路约定相悖;读 tools/dvqualify.cx 等候超时后的分支核实是否应改为返回 UNKNOWN(load-wait-timeout)而不是继续发车(build/pkg-gaps.tsv,OPEN)
(109) TLIB-KIT-UNIT-CACHE-SPLIT:tools.cxlib 冷建 84 个单元时一个 worker 在 60 秒内未产出 kit，超时前已完成的对象尚未逐个发布(build/pkg-gaps.tsv,OPEN)
(110) REGISTER-TOOL-LOCK-DOMAIN:UNISACC_TABLES 移动 batches、coverage 与登记锁，编译清单及 pins 仍相对当前仓根。两个不同表目录的写者可持不同锁写同一份清单。devsvc 是否持同一锁尚未实测。CRLF、空表、表头和无末尾换行的回滚缺少边界夹具；并发删除已移除的旧行与内存分配失败的恢复也未验证。(build/pkg-gaps.tsv,OPEN)
(111) OWN-FLOW-SHADOW-BREAK-CONTINUE-MERGE:safe 单元内层遮蔽块经 break/continue 出块时,内层同名变量的已释放状态按同一变量槽并回外层同名变量,造成误拒(for 循环头声明、switch case 内声明同样);0.0.77 候选同样拒绝,不是回归,方向安全(只误拒不漏判),独立复核夹具 s3 s4 s6 s7(build/pkg-gaps.tsv,OPEN)
(112) OWN-FLOW-BOUNDARY-FIXTURES-MISSING:0.0.78 静默漏判修复的夹具只覆盖超限一侧(65 变量、33 标签、65 层),缺恰 64 变量、恰 32 标签、恰 64 层的放行侧;遮蔽声明超 256、token 超 65536、括号嵌套超 4096 三项可达但无夹具(独立复核:用 .cx 生成器各补一个 LIMIT 夹具);宏体内 sizeof(SZ(*p))偏保守误拒(build/pkg-gaps.tsv,OPEN)
(113) DEVSVC-FWRITE-LOCK-NAME-TOO-LONG:devsvc f write 对约 60 字符路径(release/evidence/v0.0.78-p1-silent-miss-2026-11-19.tsv)报 lock refused usage lock-acquire NAME OWNER TTL(1..3600),改短文件名后通过;锁名不应受路径长度限制,或应具名报 name-too-long(build/pkg-gaps.tsv,OPEN)
(114) OWN-E1-MACRO-RETURN-AND-KR-MISS:E1 看不到宏展开后的 return(#define RET(x) return x 再 RET(p) 不拒,m8),K&R 签名不被当函数(漏判),owns 指向返回 int 0 的函数被接受没有反向检查(m15)(build/pkg-gaps.tsv,OPEN)
(115) OWN-NAMES-TABLE-HARDCODED-TWICE:分配名 9 个在 cx/cx0/plan.cx 的 cxf_isalloc 与 tools/cx-own.cx 第 65 到 67 行硬编码两处,build/own-names.tsv 尚不存在,无检查守漂移(独立复核建议)(build/pkg-gaps.tsv,OPEN)
(116) OWN-ESCAPE-B-PARALLEL-SAFE-FIXTURE:own-escape 夹具 (b) 不含 safe 故仍 PENDING 6/7;转正做法:给 (b) 加一份带 safe 的平行夹具作为新行(原夹具继续作不声明 safe 的阴性对照),不改原合同(build/pkg-gaps.tsv,OPEN)
(117) OWN-FLOW-E2-ATTRIBUTE-MISNAMES:safe 单元的流检查 cxf_unit(plan.cx 取 nm = cxf_mt[k - 1] - 1 处)与 E1 修复前同样把尾部 __attribute__((...)) 认成函数名:unsafe 名豁免与诊断里的函数名会错,E1 修复未覆盖此处(build/pkg-gaps.tsv,OPEN)
(118) RELEASE-EXEC-G1-FAIL-ROW-NOT-SUPERSEDABLE:G1 STRICT 拒绝任何不在 expected-fail.tsv 的 FAIL 行,而 UNKNOWN 已改按批次末态判定:同一批先 FAIL 后经低载单批重跑 PASS 时,FAIL 行仍挡门;0.0.78 只能把首轮 FAIL 的 2 行(unit-frontcache-equiv3,FAIL 行与 RAW)从整合严格证据 v0.0.78-strict-2026-11-19.tsv 中去掉并在文件头与发布说明写明,原始行保留在 release/evidence/0.0.78/strict-prefreeze.tsv(人工删行是不应有的手工编辑证据)(build/pkg-gaps.tsv,OPEN)
(119) UNIT-FRONTCACHE-EQUIV3-LOAD-DEPS-DIFF:unit-frontcache-equiv3(CHECK_UNIT_FRONTCACHE)在 load1 约 6、jobs 3 并发下 tests/check-limits.cx 一例 rc=0/0/0 hit=16 miss=0 deps=DIFF(两次带缓存运行的 run 缓存快照 Ds 3 项 Dt 1 项,产物与 rc 一致),低载单批重跑 2 次 PASS,0.0.77 同候选同检查 PASS,原因未复现;与 UNIT-CACHE-PARALLEL-FIRST-COMPILE-LOSES-STORES 同族(产品 run 缓存并发写丢存储)疑似(build/pkg-gaps.tsv,OPEN)
(120) OBJECTPLAN-PAIR-CHECK-RESULT-TABLE-ONLY:check-objectplan-pair 只覆盖 objectplan-result.tsv:T2 共用的 25 对里 pair 检查只数到 shared=12,另外 13 对只存在于 byte 表;判定共用只看 manifest 有没有 rows objectplan d0.sN 行,不核 byte 表;共用的 d0 节改动会同时改 arm64 输出而检查计为 same 不报警(独立复核;耦合风险已写进检查头注释 COUPLING,入库检查本身无 byte 表保护)(build/pkg-gaps.tsv,OPEN)
(121) OBJECTPLAN-NET-SIM-COV-NO-RECIPE:对象网的 FIX2、ENT3、sim --cov 没有现成跑法(tools/sim.cx 支持 --cov 但对象网输入与资源无现成配方,0.0.76 w2a3 同样记过),T2 第二步等价验收里记 UNKNOWN/UNMEASURED,靠 17 份 -o、对象产出 68/68 加复核补测 15/15、六个对象网 delta 逐字节相同代替(build/pkg-gaps.tsv,OPEN)
(122) LEX-CF-FINISH-R-UNSET-ON-POP:exec/lex/f1-result.tsv 第 10 行 CF.finish 只是用 CMPI 设 r 的过渡行却放在 r 表里,`*` 只展开 0..19,经 f1-stack-template.tsv:2 的 ATTCHK BOT 弹栈进入时 r 是上一次留下的值;lex 各表共有 28 处 RLD OLAST A64 DIVMOD10,只要某条路径设了 r 后不经 r 态派发就到达这里,r>=20 时执行器会死(N1 复核:体检剩余 2 个 ERROR 即此处,可达性未证实也未证否)(build/pkg-gaps.tsv,OPEN)
(123) TABLE-LINT-NO-DEDICATED-CHECK:tools/table-lint.cx 值域推导(设 r 的 9 条指令 CMP CMPI C64 C64U A64 A64I RLD OLAST DIVMOD10 与可达 r 值域)没有专门检查文件,0.0.79 N1 的复核负例 neg1 neg2 neg3 与正例 pos1 只在复核临时目录;表网体检基线只验证整体计数(build/pkg-gaps.tsv,OPEN)
(124) RELEASE-EXEC-G1-UNWITNESSED-HISTORICAL-FAIL:结构化九列 FAIL 缺本次 RAW，随后同批有 RAW 的 PASS 可通过 G1，历史 FAIL 的实跑身份未核(build/pkg-gaps.tsv,OPEN)
(125) DEVSVC-X-TIMEOUT-ORPHANS-DVQUALIFY:外层 timeout(如 590 秒)只杀掉 devsvc 外壳,内层 dvqualify 子进程仍继续运行并写证据文件(0.0.79 预跑中 PID 676792 成孤儿);之后 12 次 --resume 都被证据写锁拒绝 evidence-writer-busy rc 2,没有并发写,写锁兜住了;孤儿进程不会被 devsvc 或外层清理(违背每次命令及其内层子进程最多 60 秒、超时清理所属后代的铁律)(build/pkg-gaps.tsv,OPEN)
(126) CI-STREAM-PIPE-OSX-X86-TIMING-MARGIN:0.0.79 CI 首次 run 44429827605 在 macos-15-intel(osx/x86_64)stream-pipe part=e rc=-1 ms=55018(撞 55 秒分段上限,总 ms=106861)失败,其余五目标成功,report FAIL 41 PASS 1 FAIL;重派 run 44430678546 六目标全成功(--from-step 3 --redispatch,旧 run 记 exec-3-ci-prior FAIL);0.0.78 同批同目标 PASS 总 ms=75494 已贴近上限;候选 -o 与 0.0.78 逐字节相同,无产品行为变化,判为慢 runner 计时敏感,不是产品回归(build/pkg-gaps.tsv,OPEN)
(127) NET-HEALTH-NO-CROSSNET-INPUT-DECL:table-lint 与 net-health 按单网计算,跨网输入寄存器(一网写另一网读)在读方网里被报成 maybe-unwritten:eo_nzend 已在基线,0.0.80 F1b 新增 eo_initsym(objectplan-result 的 OBJ.relstart 用 LDX 写入,elfobject-result 读)使 x86_64 与 arm64 两个对象网 maybe_unwritten 与 warnings 各 +1,基线各加 1(F1b 独立复核认可,依据已记);没有声明网输入的机制,每新增一个跨网输入就得抬基线,与基线只降冲突(build/pkg-gaps.tsv,OPEN)
(128) OBJABI-INIT-U-SYMBOL-TYPE-NOTYPE:F1b 后 -funit 对象的 __init_u 符号类型从 FUNC 变成 NOTYPE(LOCAL),调试器与 objdump 不再把它识别为函数(F1b 独立复核低优先发现)(build/pkg-gaps.tsv,OPEN)
(129) OBJABI-APPS-LINK-DUP-START:用 -c -funit 对象的真实 apps(calc dijkstra wordfreq life)与 gcc 加 crt 链接失败:对象自带 _start 与 main 并有 UND __init,报 multiple definition of _start;旧候选同样(原有缺陷,0.0.80 F1b 复核实测),属于 -funit 单元对象语义(F4)与入口约定(F2)范围,不是 F1 引入(build/pkg-gaps.tsv,OPEN)
(130) ENV-PATH-VARS-ZERO-RELATIVE:F6 事实表取证:UNISACC_PHASE_OUT、UNISACC_PHASE_IN、UNISACC_RF_CAPTURE 取值 0 或相对路径时按 cwd 相对文件处理(PHASE_OUT=0 会在 cwd 写文件 0),runexec.cx:1025 在 RR_TIME 开时把 UNISACC_E3CACHE 原值当探测路径不经 mc_cache_dir 判定;与缓存根的 off/rel-refused 口径不一致;临时做法:只在事实表 exec/facts/env-semantics.tsv 如实登记现语义(build/pkg-gaps.tsv,OPEN)
(131) ENV-EMPTY-STRING-INCONSISTENT:UNISACC_RR_TIME 空串:多数读点 e[0] 非空才开,modelcache mc_phase_begin 只看已设(空串也计 modelphase);UNISACC_RR_DEBUG 同类(runcache 要非空,unitcache 已设即开);事实表记为 mixed(build/pkg-gaps.tsv,OPEN)
(132) FUNIT-SYSV-ARGS-OVER-6-FLOAT-FNPTR-SILENT:-funit 对象(lnx/x86_64)的 SysV 桩与外调搬运只覆盖前 6 个整型参数:第 7 个起栈参数、浮点参数(xmm)、单元内取址外传给 C 的 .global 函数(如 qsort 回调)静默算错或崩溃(check-objabi XL-args7 XL-float XL-qsortcb 固定现状);建议 -funit 下对这三类具名拒绝,拒绝点需前端诊断,归 0.0.82 前端诊断二期;临时做法:无(build/pkg-gaps.tsv,OPEN)
(133) DVQUALIFY-TRANSPORT-NEG1:qualify 写出 rc 非失败但 transport=-1 的 UNKNOWN RAW 行(0.0.80 预跑 notcovered 首窗),根因是批次进程在外层 60 秒窗内被杀或传输失败而 BATCH_RUN 仍落了行;需要判定 transport=-1 是否应写成无 RAW 行或带具名原因(build/pkg-gaps.tsv,OPEN)
(134) RELEASE-PREP-STRICT-NEEDS-FLAG:release-prep 找不到未提交的严格文件时要显式传 --strict(0.0.80 阶段 A 实测),默认不会读 release/evidence/vV-strict-*.tsv(build/pkg-gaps.tsv,OPEN)
(135) DEVSVC-COMMIT-ARG-ORDER:devsvc git commit 要求 -m 写在 --whole 之前(0.0.80 阶段 A 实测),反序时行为不具名(build/pkg-gaps.tsv,OPEN)
(136) DVQUALIFY-KIT-WINDOW-COLD:冷 kit 预热超过 55 秒外窗时第一段只写 CHECKPOINT remaining=N 且没有批次行(0.0.80 预跑实测,tools/dvqualify.cx 359 到 364 行),要手动再 --resume 一次;已有 kit-window CHECKPOINT 可 resume,但没有自动重试(build/pkg-gaps.tsv,OPEN)
(137) RELEASE-GATE-EVIDENCE-SET-VS-DOMAIN:0.0.80 治理域批次 release-prep 因 release/evidence/0.0.80/ 新文件被选中,但不在产品资格域,冻结后补差统计的 1 批治理批与产品域 0 批需分别记账;发布单应区分选批全集与产品资格域交集(预跑 223 批=选 653 批与域 609 批交集)(build/pkg-gaps.tsv,OPEN)
(138) OBJABI-ARM64-INIT-U-NO-AAPCS-ENTRY:0.0.80 公开件(110ab298)的 lnx/arm64 -c -funit 对象带 .init_array 与 R_AARCH64_ABS64 指向 __init_u,但 arm64 的 __init_u 代码是 ldr x17,[x7]; add x7,x7,#8; ret x17(按内部约定从 x7 栈取返回地址,没有 AAPCS64 入口包装),被 libc 经 .init_array 以 AAPCS64 调入时 x7 无定义而崩溃:链接这类对象的 arm64 程序启动即段错误 rc 139(qemu-aarch64-static 实跑,p03 与三对象同链都崩,main 为空也崩;证据 release/evidence/v0.0.81-f1barm.tsv);F2 arm64 桩同类缺失,最简单 int f7(void) 由 C 调用同样 rc 139;改前(0.0.79)arm64 -funit 对象无 .init_array 只在被调用时崩,F1b 把崩溃提前到启动,0.0.80 发布说明已于公开后更正(build/pkg-gaps.tsv,OPEN)
(139) CXDEV-ARM64-RECIPES-NOT-CLAIMED:tools/cx-dev.cx:210 把配方 id 含 arm64、osx、win- 或阶段 enc/arm、unitcontainer 的配方一律记 native=0,597 行起只重建受影响且 native 的配方,状态行写死 targets=lnx/x86_64:改 arm64 网后开发产品里的 arm64 对象仍用已装网,arm64 的先红后绿测不出来,只能等整链(约 30 分钟);本机有 qemu-aarch64-static 与 aarch64-linux-gnu-gcc 可真实运行 arm64 对象(build/pkg-gaps.tsv,OPEN)
(140) OBJABI-ARM64-X7-STACK-BELOW-SP-SIGNAL:lnx/arm64 -funit 单元的内部栈由 hentry 取 x7=sp(sub sp,16 之后)并向下增长,放在 sp 之下没有预留;单元代码执行期间若来带处理函数的异步信号(SIGALRM、SIGPROF 等,未设 sigaltstack),内核在 sp 下建信号帧会覆盖 x7 栈里的保存值,x86_64 内部栈就是 rsp 所以没有此问题(独立复核发现,机制确定,严重度未实测);另 hentry 不保存 x19-x28 与 d8-d15,当前单元代码经 17 份反汇编只用 x0-x17 但没有守卫,寄存器映射扩到 x19 以上会静默违约(build/pkg-gaps.tsv,OPEN)
(141) INSTALL-APE-RECORD-STALE-COMMIT:tools/install-ape.cx 写 unisacc.com.build.json 只同步 bytes 与 product_digest,commit 与 sources_sha256 保留旧值并打印 RECORD_STALE(0.0.71 -> 0.0.80 升级后记录仍写 0571f88,实际应为 a08294deab),导致依赖该记录定位原包配方的 cx-dev 在 main 上红(独立复核发现:object-enc 配方 recipe-not-located LOCAL_FAIL rc 2);本次手工用公开件的 unisacc.com.build.json 覆盖修正(记录文件被忽略不入库)(build/pkg-gaps.tsv,OPEN)
(142) WORKTREE-ROOT-FILES-MUST-BE-REGULAR:版本工作树(.claude/worktrees/v0.0.NN)的 unisacc.com 与 unisacc.com.build.json 必须是真实文件副本而不是指向共享检出的符号链接:第一次起链缺 build.json 时预检 epoch-canonical 报 no record(phase=snap rc=2);补成符号链接后第二次 phase=ctx 报 source-package REFUSE root-epoch-product-stat(lstat(product) 要求常规文件,快照里是符号链接);改成真实副本后第三次起链仍报同样错误,原因是整链快照缓存 ~/.cache/unisacc/chains/snap/<rev10>-<根路径哈希8> 在第一次起链时固化了符号链接并被后续起链复用,新基目录也不改变快照身份,必须把该快照目录与 .partial.lock 移走再起链(第五次起链才通过,0.0.81 共耗约 35 分钟);手工创建工作树与 Root 升级步骤都没有这一项,快照身份没有包含 unisacc.com 与 build.json 的文件类型,且复用时只核 sha256 不核常规文件(build/pkg-gaps.tsv,OPEN)
(143) RELEASE-EXEC-X-COMMIT-EMPTY:阶段 A 把 release/candidate.tsv 提交并推送(F2)后,release-exec --execute 步骤 1 X-commit 再执行 devsvc git commit --whole release/candidate.tsv 无变更而 rc=1,门 RELEASE_EXEC_GATE_X_PUSHED 报 devsvc git commit X failed;0.0.80 与 0.0.81 连续出现,手工补 exec-1-X-commit 的 INDEX 行(commit 指向 F2)后 --from-step 2 续跑(build/pkg-gaps.tsv,OPEN)
(144) SPEED-LEDGER-CHAIN-ATTEMPTS-NOT-COUNTED:tools/speed-ledger.cx 的 chains 与 chain_eff 列只按整链基目录里的最后一个会话目录计(0.0.81 为 chain3 的 1/1 100%),前四次起链失败(chain 与 chain2 目录与被覆盖的会话)没计入,数值偏乐观(实际有效占比约 43%,fable 诊断数字 25/58)(build/pkg-gaps.tsv,OPEN)
(145) SPEED-LEDGER-SELECTED-PARSE:tools/speed-ledger.cx 对 0.0.81 的 selected 列给出 4/614 与严格证据 263 批不符,疑似 batch-domain-index 解析口径与严格证据文件的 BATCH_RUN 去重口径不一致(build/pkg-gaps.tsv,OPEN)
(146) C99WARN-LOCALREDEF-STATIC-MISS:0.0.82 -Wc99-local-redefinition 对前一绑定是 static 局部对象时漏报(int x; static int x; 与 static int x; int x; 都不告警):WU.local 的 ELIGIBLE 对 static 不为 1(独立复核实测)(build/pkg-gaps.tsv,OPEN(复核 2026-11-20 已装 0.0.85:int x; static int x; 默认 rc=0 无诊断,-Wc99-local-red)
(147) C99-PARAM-LOCAL-SAME-NAME-NO-DIAG:参数与函数体最外层块同名(int f(int x){int x=2;})新旧产品都没有任何诊断(C99 6.2.1p4 应视为重定义),设计稿误称有 redefinition of parameter 先例;规则需单独做(build/pkg-gaps.tsv,OPEN(复核 2026-11-20 已装 0.0.85 仍成立:int f(int x){int x=2;} rc=0 无诊断))
(148) POSIX-PTHREAD-ATOMICS-TLS-MISSING:<pthread.h> 存在但链接期 undefined function pthread_create;__sync_* 与 __atomic_* 与 _Atomic 与 __thread 与 _Thread_local 编译期全拒;线程同步 0/3(真实程序阻碍);依赖链:原子原语与 TLS 是 pthread 的前提(build/pkg-gaps.tsv,OPEN(复核 2026-11-20 已装 0.0.85 仍成立:pthread_create 与 __atomic_fetch_add 报 undefi)
(149) ASM-EXTENDED-INLINE-BUILTINS-INT128-SIMD-MISSING:扩展内联汇编(带约束与 clobber)与顶层 asm 拒绝 not covered: argument list;__builtin_clz/ctz/popcount/bswap 与 __int128 缺失;SIMD 头文件(immintrin.h、arm_neon.h)与 vector_size 缺失;驱动不接受 .s 与 gcc .o 混编(build/pkg-gaps.tsv,OPEN(复核 2026-11-20 已装 0.0.85 仍成立:__builtin_clz undefined function))
(150) REGISTER-NO-BATCH-PRODUCT-INPUTS-ROW:新登记的候选依赖批次(register 用 {candidate} 参数的批次,如 c99-warn 的 --args {candidate};{root}/unisacc.com;{out})没有同时写 tests/batch-product-inputs.tsv 的 id 行,affected 对它报 BATCH_PRODUCT_INPUT_UNDECLARED rc=2 拒绝选批(0.0.82 预跑选批时才暴露,预跑代理在副本里临时补行);登记表工具应在登记含 {candidate} 参数的批次时提示或自动写该表行(build/pkg-gaps.tsv,OPEN)
(151) PRODUCT-TABLE-GENERATORS-NOT-IN-DERIVE-CHECK:产品表生成器(tools/win-argv-gen.cx 管 enc-x86win-winargs 与 enc-armwin-winargs 与 lower-data 三表,另有其它 facts-gen 外的生成器)不在 exec/facts/derive.tsv 的统一 derive --check 里;0.0.82 arm64 单元栈改 enc-literals.tsv 后没人重算 win-argv-gen,表仅来源戳注释不同,但批次 win-argv 稳定失败 rc2,在整链完成后的冻结前预跑才暴露,重起整链一次(约 25 分钟)(build/pkg-gaps.tsv,OPEN)
(152) NP-TIMEOUT-PYTHON-COMPAT-DIAGNOSTIC:np_run 超时文案沿用 Python subprocess.TimeoutExpired，无法从日志区分实际执行器(build/pkg-gaps.tsv,OPEN(复核 2026-11-20 已装 0.0.85 仍成立:ext/sysx/processcore.cx:150 仍打 subprocess.Ti)
(153) STREAM-PIPE-OSX-X86_64-55S-BOUND-THIN-MARGIN:osx/x86_64 CI 格 stream-pipe 总耗时 0.0.81 为 61.7 秒、0.0.82 为 79.9 秒(parts=main,e),逼近 55 秒单部件界限;0.0.83 run 44589130353 首次运行部件 e 撞界限失败(本机对照 SP_PART=e 在 0.0.82 与候选上各约 4.8 秒,无产品回归),失败格重跑后通过;同因已见 0.0.79(CI-STREAM-PIPE-OSX-X86-TIMING-MARGIN)(build/pkg-gaps.tsv,OPEN)
(154) DERIVE-FLOW-BATCH-UNMEASURED-AT-5MIN-BOUND:tests/run.cx --commit derive-flow 在 55 秒命令窗口下超窗 rc=124,0.0.83 严格证据无该批行(未实跑不记 PASS)(build/pkg-gaps.tsv,OPEN)
(155) DVQUALIFY-IDS-BUFFER-8192:dvqualify --ids 参数缓冲 8192 字节,约 400 个 id 溢出后报 empty batch set(误导文案)(build/pkg-gaps.tsv,OPEN(复核 2026-11-20 已装 0.0.85 仍成立:tools/dvqualify.cx:624 char t[8192]))
(156) RT-SHARED-LBIN-NOT-REGISTERED:0.0.86 候选执行 rt-shared-gen --check 得 rc=1，include/stdlib.h 与 cx-flat.h 的 _unisa_lbin、_unisa_lheld、_unisa_lev 均未列入状态事实表(build/pkg-gaps.tsv,OPEN)
(157) UCN-PP-GEN-NEEDS-PRISTINE-ROOT:0.0.86 候选执行 ucn-pp-gen --check ROOT ROOT 得 rc=3，directive-scan-byte.tsv 的旧 DWS 锚点命中 0 次，当前产物树无法作 pristine 输入(build/pkg-gaps.tsv,OPEN)
(158) RELEASE-EXEC-STEP3-REUSES-FAILED-RUN:release-exec --from-step 3 会复用已失败的同一 CI run 而不重派,须先人工 gh run rerun --failed 再续(0.0.83 run 44589130353)(build/pkg-gaps.tsv,OPEN)
(159) REGISTER-COLD-SHARD-OVER-33S-ON-0083:0.0.83 公开件在 UNISACC_RUN_CACHE=0 UNISACC_UNIT_CACHE=0 下跑 register --tool --sync 冷预检新分片 tools-compile-164 稳定耗时 wall_ms=33036..34841(四次实测,run_ms=15674..16236),略超 register.cx:739 硬编码 33000ms(不放宽时限)(build/pkg-gaps.tsv,OPEN)
(160) L0-LACKS-SY-SYSCALL6:cx/sys 尚缺通用 sy_syscall6(n,a,b,c,d,e,f) 入口;ext/sysx/linuxns.cx 暂以 #pragma unisa dock 在私有 lns_sys() 内按 lns_targets[] 表调用 __syscall6(_UNISA_SC(n),...)(build/pkg-gaps.tsv,OPEN(复核 2026-11-20 已装 0.0.85 仍成立:无 sy_syscall6))
(161) STREAM-PREPARE-OSX-FALLBACK:0.0.84 CI run 44647659602 的 stream-pipe 预编译 STREAM_PREPARE 在 osx/x86_64(rc=1 26129ms)、osx/arm64(rc=1 15100ms)与 lnx/arm64(rc=1 6530ms)均 FALLBACK,11 个部件各自冷编译,osx/x86_64 总 292372ms、osx/arm64 总 178677ms;lnx/x86_64 PREPARED 3854ms 总 7080ms(build/pkg-gaps.tsv,OPEN)
(162) CX-DEV-A-E-AND-LOADER-BATCHES-UNQUALIFIED-IN-LAYOUT:cx-dev-a 到 cx-dev-e、test-loader-com、loader-net-replacement 在单提交资格布局缺 bundle、installed-build-record 与 loader6 材料,不在 0.0.84 严格域,未测(build/pkg-gaps.tsv,OPEN)
(163) CHECK-PACKAGE-DECODE-REAL-REF-BUILD:check-package-decode-real 的归档 C 参照构建缺 core.h,参照侧无法编译(build/pkg-gaps.tsv,OPEN)
(164) DEVSVC-GIT-COMMIT-CANNOT-CONCLUDE-MERGE:devsvc 只提供 commit 与只读 git,带冲突的合并(版本分支合入 main)无法经 devsvc 完成(build/pkg-gaps.tsv,OPEN)
(165) DERIVE-FULL-CHECK-54S-NEAR-BOUND:tools/derive.cx 全量 --check 约 54 秒,贴 55 秒命令界限(build/pkg-gaps.tsv,OPEN)
(166) DEVSVC-GATE-REJECT-NO-GATE-NAME:近 58 小时主实例 125 次门拒绝在 git.commit 事件里不记门名与原因码,无法分真拦截与假停手(依据 ~/.claude/jobs/2798e440/tmp/devsvc-evolution.md;来源:内部)(build/pkg-gaps.tsv,OPEN)
(167) DEVSVC-COMMIT-NO-SELF-HEAL-DERIVED:文档提交几乎每次撞 doc-links 漂移被拒,需手工 derive --write 再提交(依据 ~/.claude/jobs/2798e440/tmp/devsvc-evolution.md;来源:内部)(build/pkg-gaps.tsv,OPEN)
(168) DEVSVC-JOB-LOST-NOT-EVENTED:JOB_LOST 只打到 stdout 不入事件,次数无法统计且输出为空(依据 ~/.claude/jobs/2798e440/tmp/devsvc-evolution.md;来源:内部)(build/pkg-gaps.tsv,OPEN)
(169) DEVSVC-OWN-GARBAGE-NO-CLEANUP:devsvc 自造垃圾无清理:实例目录 425 个里 379 个空,主实例下 commit-derive 残留 50 个,两个检查夹具缓存合计 5G 无上限(依据 ~/.claude/jobs/2798e440/tmp/devsvc-evolution.md;来源:内部)(build/pkg-gaps.tsv,OPEN)
(170) GATE-INNER-DEADLINE-NOT-LOAD-AWARE:此前叙述把提交门超窗归因于负载;补注:数据不支持负载归因(三档提交失败率 29.9%、37.7%、31.0%,命令超时率 2.0%、1.4%、1.8%,高负载档样本 29,不支持而非已证伪),见 devsvc-evolution.md;来源:内部(build/pkg-gaps.tsv,OPEN)
(171) DEVSVC-GATE-READS-SHARED-WORKTREE:提交门读工作树而非提交的私有索引视图,同树他人未提交改动导致误拒(2026-11-20 agyneon 在共享 v0.0.86 工作树提交被 cdx 未提交 exec/parse2 改动触发 batch-domains 与 k2-gen2 DIFF 拦下;来源:内部)(build/pkg-gaps.tsv,OPEN)
(172) PREP-OUTSIDE-SCRIPT-USE-NEEDS-CXROOT-LINK:仓外 .cx(作业临时目录下)解析 #pragma unisa use 只找脚本旁的 sys/fs.cx 等,失败报 module sys.fs: no file;脚本旁放 cx 或各前缀符号链接都不行(被引模块按其自身目录再解析),A1a 与 L0 预备 2026-11-20 两次实证,是 M1 仓外包根门槛的又一个实证(build/pkg-gaps.tsv,OPEN)
(173) SY-RUN-NO-DEADLINE-ARG:sy_run(argv, buf, cap, &cut) 没有截止参数,捕获输出的子进程卡住会让调用者一起卡住;A1a 预备驱动 baseline.cx 只能在外层套 timeout 命令(build/pkg-gaps.tsv,OPEN)
(174) CTOR-PRIORITY-SILENTLY-IGNORED:__attribute__((constructor(N)))/destructor(N) 优先级被静默忽略,执行顺序与 gcc 相反(tests/a1a/cases/t18_ctor_prio.c);constructor("x") 非法写法被接受(t20_ctor_prio_bad_rej.c);A1a 基线发现,已装 0.0.85 lnx x86_64 实测(build/pkg-gaps.tsv,OPEN)
(175) SIXTARGET-M4-NO-UNTRIMMED-HEADER-SCAN:M4 sixtarget-precompile 只编模板与链单元(默认 ftrim,未用到的头函数体不发射),不扫 include/ 全部头在 win 分支的非裁剪闭包;unistd.h unlinkat 的 _WIN32 分支前置调用 rmdir 在 D2 后成拒绝,只在 win 原生 smoke 用到 unlinkat 时暴露(CI 44772879607)(build/pkg-gaps.tsv,OPEN)
(176) D2-FUNCTION-TYPEDEF-PARAM-DECAY:0.0.86 D2 对函数类型 typedef 形参未做 C99 6.7.5.3p8 衰减:typedef void cb_t(void*); f(cb_t) 与 f(cb_t *c) 被拒 conflicting parameter types(守门人 a07),0.0.85 收,属 D2 相对公开版的回归(build/pkg-gaps.tsv,OPEN)
(177) D2-NONFIRST-UNNAMED-VOID:0.0.86 D2 新拒绝类对非首位无名 void 形参有 5 例假拒绝(发布说明已知缺口)(build/pkg-gaps.tsv,OPEN)
(178) D2-CALLBACK-POINTEE-FALSE-ACCEPT:回调形参内不兼容仍假接受(0.0.85 同样接受,既有 3 例):r03 void(*)(const char*) 对 void(*)(char*)、r17 struct a* 对 struct b*、r02 long 对 long long(build/pkg-gaps.tsv,OPEN)
(179) JSON-INVALID-UTF8-ACCEPTED:ext/textx/json.cx 接受原始串中的非法 UTF-8 字节(0xFF 0xFE、过长编码 0xC0 0x80、截断三字节序列),与严格 RFC 8259 自述不符(已登记 tests/json-conform-known.tsv,到期 0.0.89 身份收窄版之后的库改进版)(build/pkg-gaps.tsv,OPEN)
(180) JSON-I64-PRECISION-LOSS:ext/textx/json.cx 仅以 double 存数字,整数超 2^53(如 9007199254740993 读成 9007199254740992)静默丢精度(已登记 tests/json-conform-known.tsv,到期 0.0.89 身份收窄版之后的库改进版)(build/pkg-gaps.tsv,OPEN)
(181) JSON-OVERFLOW-INF-ACCEPTED:ext/textx/json.cx 接受超范围数字(如 1e400)并得到无穷大,无法写回合法 JSON(已登记 tests/json-conform-known.tsv,到期 0.0.89 身份收窄版之后的库改进版)(build/pkg-gaps.tsv,OPEN)
(182) R79-C-FUNCTION-BOUNDARY-SPLIT:无现成的 C 函数边界切分库(给定源文本切出每个函数定义的起止行/名字,供 R79 释放后使用静态粗检和 dup-census 类工具共用);R79 设计稿已提该需求(build/pkg-gaps.tsv,OPEN)
(183) CCOGW-102-LINE-ACCUMULATOR:无通用的行累加器/流式行切分库:按块读入的输出里 PASS 行可能跨块边界,运行器数 PASS 行需要跨块累加半行(ccogw 第 102 节的跨块丢行缺陷)(build/pkg-gaps.tsv,OPEN)
(184) R76-SY-FORK-SESSION-WIN-ENOSYS:sy_fork_session 在 _WIN32 返回 -ENOSYS(cx/sys/proc.cx syp_session_os),R76 一期(ext/net/httpd 每连接一子会话)仅 Linux 可用;同镜像 fork 在 Windows 无直接对应(build/pkg-gaps.tsv,OPEN)
(185) R70-COLD-HOT-START-GROWS-WITH-USE-COUNT:cx 热启动耗时随被 use 的模块数增长(R70 测得),模块多的工具即使命中 run-cache 也比少 use 的慢(build/pkg-gaps.tsv,OPEN)
(186) R60-COLD-COMPILE-LINEAR-IN-CLOSURE-LINES:cx 冷编译耗时随闭包行数线性增长,约 2ms/行(R60 测得),大闭包工具首次编译过长(build/pkg-gaps.tsv,OPEN)
(187) SY-SPAWN-ENV-REPLACE-NO-ENVADD:sy_spawn_t 的 env 字段(cx/sys/proc.cx:223,316)为 execve 整表替换语义(设非 NULL 即丢掉 PATH/HOME/TMPDIR 等继承环境,无类似 ext/sysx/process.cx np_run 的 envadd 增量叠加能力或环境合并帮手)(build/pkg-gaps.tsv,OPEN)
(188) SY-SPAWN-EXTRA-FD:sy_spawn_t 无任意额外 fd 映射(cx/sys/proc.cx:222-225 只有 in_fd/out_fd/err_fd 三个标准流;来源 R69.6 SOURCE 运输实测:子进程读 fd=3 返回 EBADF,cdx 核对原行)(build/pkg-gaps.tsv,OPEN)
(189) TEAM-PULSE-HARDLINK-TREES:nlink(build/pkg-gaps.tsv,OPEN)
(190) PRELUDE-SIGNAL-NO-KERNEL-HANDLER:prelude 的 signal()/sigaction() 仅填进程内表而不调内核 rt_sigaction, 导致工具用 signal(SIGTERM/SIGINT, ...) 做被杀清理时静默失效(如 prerun-drive.cx:1510); 需求: signal() 应当真注册内核 handler 或在文档/诊断里明确仅进程内语义(build/pkg-gaps.tsv,OPEN)
(191) SIZEOF-TYPE-NAME-ARRAY-PTR:sizeof(int (*)[3]) 等类型名作 sizeof 操作数时产品报 "expected ')'" (gcc 给 8); 需求: sizeof(类型名) 接受完整类型名并给出尺寸(build/pkg-gaps.tsv,OPEN)

(192) CI 首个 run 45442318936 的 windows-11-arm 格红:fetch 任务对新建草稿下载 win/arm64 slice 时撞 GitHub 最终一致性瞬态 404(release not found),被 workflow 的 `|| echo warning` 软失败吞掉(:81 与 :223-228 两处),随后该格因 slice 资产缺载荷判红;保持候选不变显式重派发一次,run 45446325698 六格通过(pass=42 fail=0 not_run=0 invalid=0)。(release/evidence/0.0.106/ci-45442318936.log、release/evidence/0.0.106/ci-45446325698.log)
(193) U05(provenance)在本车 workflow 无取证步骤,记未测(UNMEASURED):0.0.105 的 cache-manager.tsv 只有 sha 与备份路径,无 macOS 实跑;关闭条件为 CI osx 格出现一次带 sha 对照的实跑行,本车未做。(research/ccogweco/unverified-classification.md U05 行)

## 链后补跑与后补证据(R40 B-t / R36 路径 B)

整链完成于 chain_rev=2e8a19a342;随后合入六个非 SID 文件到 post_chain_rev=8ca2f5e4b8(build/freeze.tsv、tests/check-fwdstub-replay.cx、tests/env-semantics.tsv、tools/check-chain-ready-freeze.cx、tools/check-gen-all-check.cx、tools/check-prepush.cx;两个 tests 路径经 G51/G52 放行)。重算 SOURCES_SID=c6eeca9f10d66039ce49f5f74ee1ec63727bd85e552f342965d8445136bd812d 与原链逐字一致,候选 APE=5f83ac93d201b252ea0b56098475be9e30045875cdf1ce692451cda095ceeb3c 不变。证据:release/evidence/0.0.106/path-b-proof.md 与 post-chain-merge-provenance.txt。

六个批、七条原始 FAIL 行被同候选后补 PASS 取代(SUPERSEDED=7):

(1) W0087 check-gen-all-drift:最小仓夹具缺 tests/ 时 affected-canary 读不到 tests/batches.tsv,affected 以 rc=2 拒绝(tools/affected.cx:534-535);修复 c698ef0091 在 check-gen-all-check.cx:61-73 的 rm 清单加 tools/affected.cx,precommit.cx:3762-3768 的夹具仓分支打印 AFFECTED_CANARY skipped=1。证据:c698ef0091。

(2) W0148 check-ln-noderef:四处 ln -s 未加 -n/-T,目标已是目录或链接时落错位置;修复 d12d2087bd 只改 tools/check-chain-ready-freeze.cx 与 tools/check-prepush.cx 的四处写法,不删断言。

(3) W0150 release-exec-c:sha_file 用 char o[200] 捕获 `sha256sum -- <path>` 整行,长 TMPDIR 车道路径超 200 字节时截断缓冲、哈希取空(tools/release-exec.cx:233-243);修复在 0.0.107 的 r39c-release-exec-sha.patch,本版只披露。

(4) W0191 fwdstub-replay:批直接 execve 候选 APE,未注册 binfmt 的主机得 ENOEXEC(stderr 四次 OSError Errno 8);该批 0.0.105 不存在;修复 a46b98d713 用 sh 启动候选 APE(tests-only)。

(5) W0189 与 W9998 devsvc-git-c-operations:两条原始 FAIL 行分属两种原因。W0189 是树内 build/freeze.tsv 冻结行过期(树内行在 22:31 过期),修复 879e446640 只刷新 deadline 到 1795546800,owner 与冻结路径列不变;W9998 是该批固定执行体 prebuild-self 冷编约 43 秒被 55 秒窗口杀掉(rc=0 transport=-1),属并发窗贴限。

(6) W0206 env-semantics:cx/driver/fwdreplay.cx 新增四个环境变量(UNISACC_PHASE、UNISACC_PHASE_OUT、UNISACC_PHASE_IN、UNISA_FWD_REPLAY)而活表 tests/env-semantics.tsv 未登记;修复 8ca2f5e4b8 只追加四行(八列,UNISA_FWD_REPLAY 的 expires=0.0.110 不动)。

计数口径:批=6;原始 FAIL 行=7(W0087、W0148、W0150、W0189、W0191、W0206 各一行,加 W9998 同批一行);SUPERSEDED=7。预跑原 merge 为 503 批 PASS=495 FAIL=6 UNKNOWN=2(不含后补,不是 503/0)。补跑过程如实:W9997 十批与 W9998 首次 20 批为多批并发运行;W9998 首次的 10 条红含 PATH 遮蔽(moltbaby/bin/cc)与并发贴限两类原因,之后改为逐批串行单跑(W9998b/c、W9999a..c),原 FAIL 行保留。

## 不动点证据

fixedpoint-6targets.tsv 含六目标行与原链 chain-run-249.stdout:161-164 的四条 compare 原行(agy-flow 补录);链 compare 相位 slices=same count=6、objects=same count=102、kernels=same count=2、product=same,六目标 g2/g3 终片 6/6 逐对 sha256 相同。证据:release/evidence/0.0.106/fixedpoint-6targets.tsv 与 ape-receipt.txt。

## 证据入口

release/evidence/0.0.106/INDEX.tsv;release/evidence/v0.0.106-strict-2026-11-24.tsv;release/evidence/0.0.106/expected-fail.tsv。撤回方案:release/rollback.txt(回退 Latest 到 v0.0.105,不删标签与 release)。
- 六目标自举不动点: 链 compare 相位 `slices=same count=6`、`objects=same count=102`、`kernels=same count=2`、`product=same`, 六目标 `g2`/`g3` 终片(`lnx/x86_64`、`lnx/arm64`、`osx/x86_64`、`osx/arm64`、`win/x86_64`、`win/arm64`) 6/6 逐对 `sha256` 完全相同(`SAME`); 脱敏回执与逐目标摘要见 `release/evidence/0.0.106/ape-receipt.txt` 与 `release/evidence/0.0.106/fixedpoint-6targets.tsv`, 链 `REV=2e8a19a342251910d53010f51002b14a4ca7f916`、`APE=5f83ac93d201b252ea0b56098475be9e30045875cdf1ce692451cda095ceeb3c` (`8932113` B)。
- Post-chain-merge 溯源 (R40 B-t / R36-Path-B 六文件, `superseded=7`): 整链完成于 `chain_rev=2e8a19a342251910d53010f51002b14a4ca7f916`, 随后合入 6 个非 SID 文件至 `post_chain_rev=8ca2f5e4b89b38d9b5dff074c127989572f45833` (`build/freeze.tsv`、`tests/check-fwdstub-replay.cx`、`tests/env-semantics.tsv`、`tools/check-chain-ready-freeze.cx`、`tools/check-gen-all-check.cx`、`tools/check-prepush.cx`, 两个 `tests/` 路径经 `G51`/`G52` 放行, 六路径均不在 `SOURCES_SID` 内; 重算 `SOURCES_SID=c6eeca9f10d66039ce49f5f74ee1ec63727bd85e552f342965d8445136bd812d` 与原链逐字一致, 候选 `APE=5f83ac93d201b252ea0b56098475be9e30045875cdf1ce692451cda095ceeb3c` 不变)。已用同候选 APE 在 `8ca2f5e4b8` 的 `frozen-root` 复跑 20 批(`strict-W9997-r40-bt1.tsv` 10/10 PASS、`strict-W9998a-r40-bt2.tsv` 9/10 PASS、`strict-W9998b-devsvc-git-c-run1.tsv` 空闲单批 1/1 PASS `ms=16748 assertions=22`、`strict-W9998c-devsvc-git-c-run2.tsv` 预热后单批 1/1 PASS `ms=6518 assertions=22`)并在默认短 `TMPDIR` 下连续独立复跑 3 次 `release-exec-c` (`strict-W9999a..c-w0150-run1..3.tsv`, 3/3 PASS `assertions=55`), 保留原 238 窗 6 条历史 `FAIL` 行及 `W9998a` 中 `devsvc-git-c-operations` 首次固定执行体 `prebuild-self` 冷编约 43.5 秒触 55 秒限的第 7 条 `RAW FAIL` (`rc=0 transport=-1`) 原行, 六个唯一批共七条原始 `FAIL` 行按 `expected_fail=0 superseded=7` 折叠并入 `release/evidence/v0.0.106-strict-2026-11-24.tsv` (`PASS=503 FAIL=0 UNKNOWN=2`), 详见 `release/evidence/0.0.106/post-chain-merge-provenance.txt` 与 `release/evidence/0.0.106/path-b-proof.md`。
