# unisacc —— 产品与项目总览

本文回答五个问题：unisacc.com **是什么**、**有什么功能**、**结构组成**、**测试套件怎么设计**、**项目目录与文件结构**。术语歧义以 §0 为准；历史流水与逐版收口在 [archive/plans/2026-11-01/prd-c43-full-2026-11-01.md](archive/plans/2026-11-01/prd-c43-full-2026-11-01.md)（完整旧版），不在这里重复。

## 0. 概念与术语（先读；歧义以本节为准）

「流 / 网 / 段 / 阶段」四个词各只有一个意思。详细版与逐处核实见 [docs/glossary.md](docs/glossary.md)。

| 层 | 唯一名 | 含义 | 数量 | 真源 |
|---|---|---|---|---|
| L1 | **Stream**（别名 Flow） | 字节管道阶段：输入格式 → 输出格式 | unisacc **7**（e2/e1/e3/e4/prune/lower/elf） | `exec/pipeline/image-stages.tsv` |
| L2 | **Net** | ask 事实网：一张有限表 + 全键穷举 exact | **18**（8770 键） | `weights/gold/*.tsv`（权威；规则形式 `seed/gold_<阶段>_label.c`，由 `tools/gold-table.cx --check` 核对） |
| L3 | **Segment** | 训练合并段（多网合一的 masked 多任务模型） | 3（s1/s2/s3） | 负结果证据已归档 |
| L4 | **Book phase** | 一次推导中 ask 的记账阶段 | 由 contract 声明 | `flownet/contract.cx` |
| L5 | **Release phase** | 发布流程阶段 | 10 | 已归档 |

**六条常见误解**（不要按直觉读）：

1. 「7 流」与「18 网」不是同一批东西的两种叫法——流是**字节管道阶段**，网是 **ask 事实表**。
2. `class Stage`（旧 `unisa/gold.py`）表示**网**，不是流。
3. 旧 `unisa/control/net.py` 的 `Net` 是**神经网络模型**，不是事实网。
4. m4 的「4 流」是**四个字节段**（lex/front/select/encode），不是 4 张决策网。
5. `phase` 三义：book phase（记账）/ release phase（发布）/ 历史 compiler phase（实为 segment）。
6. `STAGE_NETS` 只是 isel、abi 两张不在 `TABLES` 里的网（15+2+1=18）。

**禁用写法**：裸 `stage` 一律写 `stream-stage`（流）或 `net`（网）；用 `net` 指神经网络一律写 `model`。

**RI 的固定含义**：本文 RI = **ResourceInput（具名资源输入）**，含名称、字节和长度、存在性/空值及跨流身份绑定，不是 "runtime integrity"。

## 1. unisacc.com 是什么

**一句话**：unisacc.com 是 C99 编译器（带部分 POSIX 库子集），打包成单个类 APE 文件——核心需求是六个 ISA 切片（lnx/osx/win × x86_64/arm64，每个目标同候选原生证据；现状缺口见下），能为 {Linux, macOS, Windows} × {x86-64, arm64} 六目标直接写出可执行文件，并以默认 `-run` 直接运行 `.c` 和 `.cx`。cx 是参照 Rust 设计的模块化方言（`#pragma unisa module/use`），所有权与边界目前为**运行期**检查，不等于编译期借用检查。编译内部是 7 个字节流阶段，表状决策交给 18 张事实网（16 张出货，isel/combo 为对照），网络权重由真值表构造并穷举验证、不经训练，由一个通用执行器推理——以网换逻辑。

**一个文件** `unisacc.com`（APE，即 αcτµαlly pδrταblε εxεcµταblε），一个文件同时是「编译器」和「程序」，但「它能写出的目标」和「它自己运行在哪些原生宿主」是两回事：

- **写出六目标程序**（真）：能编出 {Linux, macOS, Windows} × {x86-64, arm64} 六个目标的可执行文件（ELF / Mach-O / PE），**映像不经汇编器和链接器**（模型路由直接编码写出）。
- **六 ISA 切片是核心需求，不可降级**（主人 2026-11-06 两次重申）：lnx/x86_64、lnx/arm64、osx/x86_64、osx/arm64、win/x86_64、win/arm64，每个目标都要同候选原生证据；当前候选的原生 CI 结果与 Windows 两格仅 portable 子集的限制见 §9.1。
- 参考编译器可写三种格式的可重定位 `.o`，产品自带链接器与 `ar`；
- **自举**：宿主构建 `unisacc-seed.com` → 种子自建 → 再自建，三个 sha256 相等（N22）。

**语言**：C99 = ISO/IEC 9899:1999 + TC1–TC3（WG14 N1256）。覆盖度按逐条款账本 [archive/testing-c43-legacy/tests/c99/clauses.tsv](archive/testing-c43-legacy/tests/c99/clauses.tsv)，缺口在 [README.md](README.md) 限制表逐条点名。

**方法**（论文 A 的范式）：编译器里每个**表状决策**（词法类、预处理指令、优先级、类型、指令选择、ABI、窥孔……）由**构造**出来的小整数网络回答——权重从真值表派生，在表的全部定义域上**穷举验证**（不是训练）；结构性粘合是经典代码。范式一句话：

```
事实表 → 生成器 → 网 → 一个通用执行器 + 按包跑阶段
```

四条范式支柱：**有限表表达决策**、**共用执行器提供机制**、**显式流接口连接阶段**、**晚绑定合同隔离声明需求与宿主供给**。

**动态逻辑进表的范式（2026-11-05，实例：arity、type、stmt）**：运行值先**分类**成有限键（机制行），再用**决策表**选标签（`weights/**/*.tsv` → `tools/decide-gen.cx` → 网行，逐键 T1，批次 `check-decide`），最后由机制把标签落成动作；值放 `exec/facts`，动作只写 `["constant",NAME]`（棘轮 `check-literal`，跨处一致 `check-limits`）。能进表：一步决策（选标签、选后继）。留在机制或生成器：循环与递归结构、带副作用的序列。T1 只证“网 = 表”，表是否对另靠产物比对和外部裁判。第四例（E4 窥孔，`weights/gold/peep.tsv` 1632 键）证明范式跨流成立：两级都动态时用 `decide-gen --each a=... --select REG`，第一级 `RLD pa_` 选态、第二级 `RLD b*12+rel` 选标签，START 的 1632 条 STX 池写入归零；opt 的生成器是手写清单解释器（`seed/gen.c construct_opt`），接入新 rows 要在白名单加 stem。

**两条实现路线**：C 参考编译器（`archive/reference-c/`，行为裁判与回退）与模型产品（`exec/` 各阶段网络 + 通用执行器），逐字节对拍。

## 2. 有什么功能（已实现能力速查）

#### 2.1 日常（运行、改写、查询）

| 动作 | 现成用法 | 说明 |
|---|---|---|
| 动态执行 C／`.cx` 工具 | `./unisacc.com FILE.cx -- ARGS` | 默认即 `-run`；`.c` 同样适用 |
| 显式动态执行 | `./unisacc.com -run FILE.cx -- ARGS` | 保留的显式写法 |
| 从 tape 写目标镜像 | `./unisacc.com INPUT.tape -b OS/ARCH -o OUTPUT` | `.tapebin` 也有现成入口 |
| 写六目标程序 | `./unisacc.com FILE.c -b OS/ARCH -o OUTPUT` | 目标 `lnx/osx/win` × `x86_64/arm64` |
| 查 `.cx` 模块依赖计划 | `./unisacc.com -cx-plan FILE.cx` | 模块循环硬拒绝，报 `module cycle` |
| 用系统 GCC/Clang 编 `.cx` | `sh ./gcc_cx.com FILE.cx -o OUT` / `sh ./clang_cx.com` | 六格 APE 宿主驱动，不调 unisacc.com |
| 查调用点 | `sh ./unisacc.com tools/refactor.cx -- sites NAME` | 全仓一次，跳过注释/字面量 |
| 符号索引 | `sh ./unisacc.com tools/symindex.cx -- def NAME` | 冷建 ~2.4s，查询 ~250ms |
| `use` 的包根解析 | 仓根 `cxroot.tsv`（`R<TAB>PREFIX<TAB>DIR`）；工具侧 `use cxlib.cxroot` 调 `cxr_path`；`sh ./unisacc.com tools/check-cxroot.cx` | 旁边找不到时查最近祖先的 `cxroot.tsv`，最长前缀映射（cx/spec.md CX-U-7）；编译器侧已改 `plan.cx`/`cxmod.cx`，**已装产品要重建安装后才生效**，软链届时逐目录淘汰 |
| 源码所有权检查（CX-W-4 路线 B） | `sh ./unisacc.com tools/cx-own.cx -- FILE...` 或 `-- --tracked`；夹具 `tools/check-cx-own.cx` | 函数内局部 owner 的 DOUBLE-FREE / USE-AFTER-FREE / LEAK；保守（转给未知函数视作可能转出）。2026-11-03 全仓：9043 函数，LEAK 220（抽 10 全真，多为工具错误路径），DOUBLE-FREE 1（误报，互斥整数条件），产品目录只有 60 个局部 owner——产品所有权多在结构体字段与包装分配器里，函数内文本分析覆盖不到 |
| 批量机械改写 | `refactor.cx -- find/apply/sweep` | 每文件改前改后验证，不过即还原 |
| 归档过期 ledger/items 行 | `sh ./unisacc.com tools/archive-records.cx -- [--cutoff YYYY-MM-DD] [--write]`；检查 `tools/check-archive-records.cx` | 默认干跑；`--write` 把早于 cutoff 的 ledger 行与无存活引用的 DONE/CLOSED items 追加到 `archive/work/`；SLICE-CHANGE 不动；写前写后核 sha |
| 改了文件重跑哪些批 | `sh ./unisacc.com tools/affected.cx -- --files F` | 按 use 闭包算受影响批次 |
| 改一个文件看爆炸半径（一条命令） | `sh ./unisacc.com tools/flow.cx -- check FILE... [--run]` | 串起符号图受影响文件 + 受影响测试批次；`flow -- graph FILE` 出局部图 |
| 模块关系图（全图/子图/差分） | `sh ./unisacc.com tools/symindex.cx -- graph [FILE...]` / `-- delta FILE...` | 节点=文件、边=use/call；给 FILE 只出该文件的正向 use 闭包 + 反向 users |
| 文档断链 | `sh ./unisacc.com tools/doc-links.cx -- --tracked` | 归档后修链 |
| 只提交自己的行 | `sh ./unisacc.com tools/commit-own.cx -- -m MSG --keyed PATH KEY` | 多人同改文件（prd/batches/coverage） |
| 装前判产品能不能用 | `sh ./unisacc.com tools/check-dev-product.cx -- ROOT PRODUCT` | 六条：shell 头 / `sh --version` / 不带 `UNISA_CONTAINER` 能编译 / 写入产物 / 内嵌包是新头 |
| 失控与超数量`.cx`进程守卫 | `sh ./unisacc.com tools/procguard.cx --once`（默认dry）；`--enforce`才回收；`--only-parent PID`收窄受控扫描；检查`tools/check-procguard.cx`（批`cx-procguard`） | 每10s扫描同uid/.cx；保留同文件数>`PG_MAX`(100)规则，另将累计CPU>=90%且存活>10分钟、cwd已删、ppid1且>30分钟非tmux树列候选。只日志不默认信号；enforce逐PID先STOP后KILL并复核starttime/uid/保护，绝不按pgid/session杀。`CLK_TCK`真实读取，读失败不启CPU/orphan判定；沿用本身/祖先/原名单及tmux/pane保护，orphan判定排除tmux子树。日志`~/.cache/procguard/log`，旧组回收后的fresh-scan `residue=KEY:N`合同保留。 |
| 超时作业的整棵树清理 | `#pragma unisa use sys.process` → `sy_child_subreaper()`、`sy_kill_tree(root, ms, keep, nkeep, rep, cap)` | 先冻结根、反复扫 `/proc` 把新后代与同组进程也 SIGSTOP（被停进程不能再 fork），到不动点后全杀，返回残留数（0 才算干净）；starttime 防 pid 复用；subreaper 让双 fork 的孤儿仍可见；非 Linux 返回 -ENOSYS。检查：`sh ./unisacc.com tools/check-reaper.cx` |
| 子进程超时契约（run/capture + residue） | `sh ./unisacc.com tools/check-netpack.cx` | 双管EOF后仍按原deadline非阻塞等待，超时冻结整树后击杀并有界定向回收直接子，不抢调用者的其他子进程；`np_result.residue` 0才算干净。孤儿可见性仍需调用方先`np_subreaper()`。`UNISACC_NP_FAULT=residue`为tools与run-batch的显式注入缝，默认关，其它值具名拒绝；sysx产品单次回收疑点另列0.0.47，不混同此测试库修复。 |
| e2stream 包装契约 | `sh ./unisacc.com tools/check-e2stream.cx` | 自建进程组 + 整树击杀（含 setsid 逃逸）+ `TimeoutExpired` 文案；干净超时不带 `survived)` 后缀 |
| 督导观察 agent 面板 | `sh ./unisacc.com tools/observe-agents.cx -- [--panes a,b,c] [--no-git] [--ratchets] [--fixture FILE]` | 逐个 tmux 面板读 `PANE`（state 取 mind 块首行 `▶`=busy / `○ 空闲`=idle，busy_for 取底部栏相对时长）/`PLAN`/`TOPIC`/`TASK`，并按 `agentwork/items.tsv` 第 9 列标 `STALE`（DONE、CLOSED…、SUPERSEDED）与 `PARTIAL`（DONE-\*，尾巴未收）；`--fixture` 用文本面板，`tests/check-observe-agents.cx` 即其批次；输出 ≤30 行 |

#### 2.2 测试（棘轮、门禁、批次）

| 动作 | 现成用法 | 说明 |
| 核对队友报告里的事实 | `sh ./unisacc.com tools/report-verify.cx -- CLAIMS.tsv`；校验 `tools/check-report-verify.cx` | commit/tsvcount/receipts/sha256/ratchets 五类断言逐条核实，输出 OK/MISMATCH/MISSING，有 MISMATCH 退出 1 |
|---|---|---|
| 查模块层向与包边界 | `sh ./unisacc.com tools/check-cx-layers.cx [PRODUCT]`（`-- --fixtures` 跑夹具） | 编号层 L0–L3 直接 `use`，向上越层即红；`ext/` 是包层：产品层（`src/`、`cx/core`、`cx/runtime`、`cx/compiler`、`cx/driver`、`cx/cx0`、`cx/workspace` 与 `tests/cx-module-freeze.tsv` 的 core 模块）`use` 到 `ext/` 即红；core 模块文件集合按冻结表只许减（新增行即红、行无文件亦红） |
| core 导出面棘轮 | `sh ./unisacc.com tests/coreapi.cx -- --check`（`--write` 下调，`--fixtures` 跑夹具） | 26 个冻结 core 模块的每个非 static 定义各占一行：新符号红、行缺失红、签名变动红、`DEPRECATED-ALIAS` 的调用方文件数超上限红；`core_run` 一行标 `DUAL-IMPL linked=cx/sys/kernel.cx`，两条链接由 `cx-core-run-diff` 逐字节比对 |
| core_run 双执行器差分 | `sh ./unisacc.com tools/check-core-run-diff.cx -- ROOT KERNELS` | 同一模型与三组固定输入分别喂 `cx/core/execute.cx`（CX 参考实现）与 `cx/sys/kernel.cx`（产品链接的汇编内核），两行整体逐字节比较，末行 `CORE_RUN_DIFF`；KERNELS 是含 `x86_64.kernel` 的目录 |
| 拒新的模块 `#include` | `sh ./unisacc.com -run tests/cxinclude.cx -- --check` | 新模块必须 0 |
| 上层禁 libc/POSIX | `sh ./unisacc.com tests/cxlibc.cx -- --check` | 名单不手写，按前导分类 |
| 看某域还差什么 | `sh ./unisacc.com tests/full-domain.cx -- . DOMAIN` | 逐格读矩阵，印 OPEN 行 |
| 记录已知缺口 | `sh ./unisacc.com tools/matrix-waive.cx -- . tests/waivers-0.0.44.tsv --check` | R 行理由 + W 行豁免 |
| 仓级棘轮（13 项） | `sh ./unisacc.com tools/ratchets.cx` | cxraw/cxinclude/cxapis/cxtrunc/cxlibc/cxplatform/srcshape/coreapi/toolsloc/verwriter/aluiwidth/outdep/check-literal（2026-11-06 主树 `RATCHETS ok=13 fail=0`，见 plans/rc1-gates-2026-11-06.md 门槛 2） |
| tools 行数只降不升 | `sh ./unisacc.com tests/toolsloc.cx -- --check`（`--write` 只下调） | tracked regular `tools/*.cx` 总行数，基线 `tests/toolsloc.tsv`；新增 tool 必须先改基线并经 cc 批准 |
| 真实提交 tools LOC 与源码分类闸 | `sh ./unisacc.com tools/check-precommit-toolsloc.cx -- --loc`、`-- --source`、`-- --symlink`（三批全部执行；批次 `precommit-toolsloc`、`precommit-toolsloc-source`、`precommit-toolsloc-symlink`，各 45 秒） | 各批独立真实临时 git 仓，实际编译并运行生产 hook 与完整 ratchets；保留超额 rc/理由、未跟踪排除、shebang 分类、符号链接不重复计行、无尾 LF 计行、未暂存增长排除和新增常规行拒绝；无参拒绝，不提供隐式串行全批入口；耗时须实测，不表示已通过 |
| 分片测试：显式绑定 slice/pkg 启动 | `tools/test-loader.cx`（`--slice ABS --models ABS --identity ABS`、`--net STAGE=ABS --swap-tool ABS` 单网替换、`--e2-adapter`）；检查 `tools/check-test-loader.cx`（批 test-loader-com，需 5 个 `TEST_LOADER_*` 绝对路径环境绑定） | 开发证据，不是发布资格；缺绑定非零，不回落读 `./unisacc.com`；规格见 plans/test-loader-com.md |
| 波次哪些批不能同跑 | `sh ./unisacc.com tests/check-wave-domains.cx`（批次 `wave-domains`） | `tests/wave-domains.tsv` 每行 `id<TAB>domains<TAB>reason<TAB>evidence`；domains 取 `serial`（独占一波）/`wt`（写共享路径）/`hs`（写 harness 三树）/`mi:<材料名>`，空=无约束；all-waves 按掩码两两不相交放置 |
| 单阶段过滤器（分段编译管道） | `./unisacc.com stream STAGE [--route R] [--res NAME[=FILE]] [--models PKG] [--raw] < in > out`；`stream frame --in L=F...`；`stream sidecar` | 同一 st_span/包/`\0cli/*` 跑一个阶段；属性表随流（USLATTRT）；`e2\|e1\|e3\|sidecar\|prune\|lower\|elf` 与 `-o` 逐字节同；校验 `tests/check-stream-pipe.cx`；改表→`tools/lab-loader/p3-swap.cx --model`→`--models` 实测 ≈8s |
| stream 路由目标一致性 | `sh ./unisacc.com tools/check-stream-target.cx`（批次 `stream-target`）；开发可指定 `PRODUCT/ROOT/BASELINE_PRODUCT` | route-only 从事实表 cell 前缀导出目标，显式 target 冲突一行拒绝；六 cell 宏与 direct 相等且互异、E3 long 字宽、显式一致及默认宿主逐次实跑；旧公开预发布的 route-only 曾错误沿用宿主宏，新候选重建后生效，未供旧产品伪报通过 |
| 分段单阶段用例（逐段字节 golden） | `sh ./unisacc.com tests/check-stages.cx [-- PRODUCT ROOT]`（批次 `stages`） | `tests/stages/<段>/` 每段 `args/in/expect`，可选 `bad/bad.err/bad.args`，喂 `unisacc stream STAGE`；无拒绝用例的段记 GAP。tape 入口校验撤回，本批如实红（见 CHANGELOG 已知限制） |
| 矩阵 BLOCKED 格归属 | `sh ./unisacc.com tests/check-roll-up-open.cx`（批次 `rollup-open`，输出 `ROLLUP_OPEN domains=19 failed=0`） | 19 张域矩阵 roll-up；lnx/x86_64 必需格不得 MISSING，每个 BLOCKED 格必须引用 `agentwork/items.tsv` 里存在的条目；`--self-test` 含 unowned-blocked-refuse |
| 已知 FAIL 必须登记 | `sh ./unisacc.com tests/check-known-fail.cx -- AGGREGATE.out WAIVERS.tsv`（`-- --self-test`） | rc1 门槛 1：聚合输出里非 PASS 的批次都得是 `tests/waivers-0.0.44.tsv` 的 `F` 行；F 行只说"已知有主有计划"，不把 FAIL 变 PASS |
| 懒分配表有没有漏守卫 | `sh ./unisacc.com tools/check-asmtext-tables.cx -- ROOT` | 碰表的 asmtext 入口必须调 `at_tables()`；删掉守卫即红 |
| 标准流写失败可见吗 | `sh ./unisacc.com tools/check-stdio.cx -- ROOT` | `/dev/full` 下 `puts`/`fflush`/`ferror` 必须全报，退出码 15 |
| 携带头与 glibc 的转换语义是否一致 | `sh ./unisacc.com tools/check-libc-probes.cx -- ROOT` | 七个自校验 C 程序，期望值是 glibc 字面量（运行时不靠宿主 gcc）：`putc` / `SIGUSR1,SIGUSR2` / `NAN` 符号 / scanf `%[...]`（含 16000 字符字段与宽度边界）/ printf `%n`（含截断不越界）/ scanf `%n`（含 `"%n%s"` 不跳空白）/ `strtod` 的 inf、infinity、nan。每条以 `.c` 与 `.cx` 两种输入各跑一次；`putc` 一行要产品重建（名字表在 `p3-package.bin`）才转绿 |
| 切片冻结门（改薄接口层必须带回归用例+批准+字节对比;advisory,6 格 CI 原生通过后转 enforce） | `sh ./unisacc.com tools/slice-freeze.cx -- check BASE [HEAD] --receipts DIR [--baseline DIR] [--mode advisory\|enforce]`；`-- list` 列出冻结面×批次 | 冻结面清单 `tests/slice-freeze.tsv`（唯一写者 omp-cc-opus）;缺收据报 UNMEASURED,永不 ok;批次 `slice-freeze-tool` |
| 分片本机启动与外域不声明 | `sh ./unisacc.com tools/check-loader-isa.cx`；批次 `loader-isa-start` | 必需显式 `TEST_LOADER_ROOT/LOADER/SLICE/MODELS/TARGET/VERSION`，路径绝对；只核真实本机精确版本，其他五格 `NOT_CLAIMED`，缺绑定具名拒绝，开发证据不是六目标发布资格 |
| 部署模型单网替换回归 | `sh ./unisacc.com tools/check-loader-nets.cx`；批次 `loader-net-replacement` | 显式 `TEST_LOADER_ROOT/LOADER/SLICE/MODELS/SWAP`；八个真实D/M模板完整输出等价及变异实际拒绝；`test-loader --net STAGE或数值INDEX=ABS --swap-tool ABS`，精确route选M；18事实网另轨，跨目标只生成产物不冒认原生 |
| 共享部署模型逐mode消费 | `sh ./unisacc.com tools/check-loader-common.cx`；批次 `loader-common-models` | 五绑定加显式 `TEST_LOADER_COMMON_MODE=pp/tapecheck/o2/units/tokens-pp/tokens-lex/warn`；每mode10项真实自M等价、变异产品拒绝及SHA不变，缺模式非零；覆盖M3/5/7/32/33/34/35，18事实网另轨 |
| 特殊部署接口模型消费 | `sh ./unisacc.com tools/check-loader-special.cx`；批次 `loader-special` | 五绑定；M36 nativeabi、M37 symbols、M38 contract、M39 ownership完整合法协议帧、自M全输出相等、变异产品具名拒绝及部件SHA不变，共21项；非归档libunisacc发布资格 |
| 六目标部署模型逐格消费 | `sh ./unisacc.com tools/check-loader-targets.cx`；批次 `loader-target-models` | 五绑定加显式 `TEST_LOADER_TARGET_MODE=image-或object-目标`；12格各14项，覆盖24个lower/elf模型，自M全输出相等、变异必须产品拒绝（仅launcher拒绝不算）；跨目标镜像不执行，非原生资格 |
| RI原生材料生产与缺头负控 | `sh ./unisacc.com tools/ri-material.cx -- --build ROOT NEW_OUT tests/cases/ri-cli.cx`（或`ri-consumers.cx`）；`--verify-native`只核验 | 完整fixture前缀、`-I ROOT/exec/c`及既有close-fail链接叶SHA入native收据；缺-I真编译负控批`ri-material-native-missing-include`必须rc2具名，NULL0 hook保留；仅PREPARED，绝对ROOT不能搬，P4 flow-data兼容仍另项 |
| Linux默认运行命名回归 | `sh ./unisacc.com tools/check-run-comm.cx -- ROOT NEW_PRODUCT OLD_PRODUCT`；正式批 `run-comm` 绑定 `run-comm-new,run-comm-old` | 实跑旧c46-2固定SHA负控 `comm=63` 与新程序 `comm=rcprobe.c`，同时核memfd、argv0、空参数/带空格参数、fd9、stdout及rc37；无参需三项显式RUN_COMM绑定，缺失NOT_RUN/rc2，不猜候选 |

#### 2.3 发布（资格、会话、安装）

| 动作 | 现成用法 | 说明 |
|---|---|---|
| 同候选原生资格（六目标证据表） | `sh ./unisacc.com tests/candidate-qualify.cx -- cell|aggregate ...` | 由 `.github/workflows/candidate-native.yml` 调度；缺证据或身份不符 rc1，有 NOT_RUN rc3，全 PASS 才 rc0 |
| streams 旧合同壳材料 (unisacc-stream1..7.com) | `sh ./unisacc.com tools/streams-material.cx -- ROOT NEW_OUTDIR [LEVEL [TARGET]]`;校验 `tests/check-streams-material.cx`(批次 streams-material) | 7 个 <=40B 薄壳转调 `unisacc stream STAGE`,分派器 `s` 烘入 ROOT;e1 经 `tools/streams-e1-frame.cx` 把 USLATTRT 转 USLATTR1;收据 streams.json scope=product-stream-shim + NOT_CLAIMED,不是独立 runner,与 streampack 8 容器无关;钉子由 tests/cases/stream_pin.cx `stream_receipt_pin` 取自收据。lower 阶段产品会追加运行库尾部,依赖精确 lower 输出的用例仍红 |
| 两代不动点（读 chain-step 产物） | `sh ./unisacc.com tests/chain-fixed-point.cx`（批次 `chain-fixed-point`，材料 `chain-gen1`、`chain-fixedpoint`） | F1 gen2 sha==gen3 sha；F2/F3 receipt 的 constructor_sha 依次指向上一代，防复制同一文件冒充；不重跑构造。材料必须绑到候选文件而非其父目录（RC1-MATERIAL-BIND） |
| 全量会话（all-waves） | `sh ./unisacc.com tools/all-waves.cx -- init/material/wave/pending`（每条外包 `tools/bound 55`） | 会话是 ROOT/out 下新目录，候选必须是 ROOT/unisacc.com；light 波最多 4 个无材料批并发，heavy 波一次 1 批；要点：init 前取最终 MAP_READY 材料，init 后 diff `materials.tsv`，按批次参数形态（文件/目录）写 map——会话内材料绑错会造出假 FAIL，单跑才是 HEAD 事实（ledger RC1-MATERIAL-BIND） |
| 会话图内材料一键绑定 | `sh ./unisacc.com tools/s8c-bind.cx -- ROOT SESSION CANDIDATE MAP`（薄壳 `tools/s8c-bind.sh`；批次 `s8c-bind`） | init 后对 MAP 每名独立 `all-waves material`（各 bound 55）。PREPARED≠PASS。scratch 14 名后 heavy yes 0→84 |
| 候选 P3 自指钉 | `sh ./unisacc.com tools/s9-p3-pins.cx -- PKG MAP`（批次 `s9-p3-pins`） | 同一 pkg/APE 现算 file/p3 sha 写 MAP；c46-3 空跑 p3=14a69316…；篡改一字节必红。禁止旧候选拷钉 |
| 身份派生校对 | `sh ./unisacc.com tools/identity-derive.cx -- ROOT --check|--write`（批次 `identity-derive`） | 编排 `check-root-identity` emit/check；--check 不一致打印 DIFF。尚未接 ape-receipt/build.json/pins |
| 发布目录布局与校验 | `sh ./unisacc.com tools/release-layout.cx -- --candidate release/candidate.tsv SOURCE NEW_OUT`；`-- --candidate release/candidate.tsv --check OUT`（批次 `release-layout`；旧 `--version VERSION SOURCE NEW_OUT` 形式保留） | 候选身份只来自 `release/candidate.tsv` 唯一一行；生成六文件及 SHA256SUMS 与 WinARM ZIP，`--check` 重算全部 sha 并拒非法 ZIP；见 `release/README.md` |
| 只读发布资产完整读回 | `sh ./unisacc.com release/tools/c47-1-readback.cx -- ROOT TABLE NEW_OUT OLD_APE true\|false` | 身份与资产 sha 均取自 `release/candidate.tsv`；新目录从 draft（true）或正式 release（false）下载，核元数据/sha/版本/冒烟/进程名/ZIP；`tools/release-dispatch.cx -- --dry-run TABLE ASSETS` 只打印将创建的 draft、上传项与 workflow 输入，不做任何外部操作 |
| 固定220例外部语料与期望配对 | `sh ./unisacc.com tools/ctestsuite.cx -- ROOT MATERIAL NEW_OUT START COUNT`，每片≤20；批次`ctestsuite` | 上游rev/归档SHA/每例源码与期望SHA在`tests/corpus-ctestsuite/`；源码不重分发。真实c46-3首轮216符合、4拒绝、0错值、0未知；仅1个拒绝最小例为严格C99，另3个为GNU/C11扩展。输出接受/拒绝/错值/未知和原件，不删拒绝例换绿。 |
| 三方编译差分与百例整型值差分 | 旧`opt-corpus-gen`的`ccdiff[-2/-3/-4] HOST_CC [TCC]`追加第二oracle；`sh ./unisacc.com tools/random-expr.cx -- ROOT NEW_OUT GCC TCC [SEED]`；批次`random-expr` | 原产品O0/O2及HOST_CC字面期望不变；TCC无优化flag，编译警告和三方不一致单列原件。随机每批100个unsigned表达式，固定seed、非零除数、界内移位，逐值比较两个真实oracle。缺编译器非零；默认check读取HOST_CC/TCC，tcc私有安装不入库。 |
| 产品改动链预检 | `sh ./unisacc.com tools/chain-precheck.cx -- ROOT CHAIN_OUT NEW_OUT init --files LIST`（或`--range A..B`），随后`reuse`、逐项`job`、`identity/routes/p3`、四`probe`、`slice all`、`finish` | 原链只读、副本执行与身份/路径隔离合同不变；job reads命中优先，显式非产品事实零gen，已审计无消费者资产精确路径+SHA单列dead-asset，未知仍拒绝；产品域不得伪标非产品。69/0含hash变化和同目录假文件负控。真实44182b4059..含A1的私有干净HEAD只选gen-shared-e3/gen-warnparse/gen-errorparse；control-reject仅历史镜像无live消费者，不伪造目录映射。变更分类账与审计证据可读，不缩range冒称全覆盖。 |
| 仓根换成真 APE（旧脚本留为 unisacc-dev,可回退） | `sh ./unisacc.com tools/install-ape.cx -- ROOT APE SHA256 [--write]`；回退 `-- ROOT --rollback [--write]`；检查 `tools/check-install-ape.cx` | 校验整文件 sha、UNIPKG1 尾、`--version`；同目录临时文件复核后原子 rename；旧启动脚本存 `unisacc-dev`、`.dev/` 让位为 `.dev.split-prev`、记录备份 `.split-prev`。build.json 同步 `ape1`/digest/**实际 bytes**；`ALREADY --write` 修陈旧记录，干净记录不重写，不带 `--write` 不改。两源包生产入口共享 `productpkg.pp_execution_record_error` 规范校验；更新生产者自钉后旧代材料必须重新准备；批次 `install-ape` |
| win/arm64分片固定点证据检查 | `sh ./unisacc.com tools/check-win-arm64-fixedpoint.cx -- MANIFEST [--test-loader LOADER PROBE]` | 必需 `root` 为 win 实际源根、`native_root` 为 Linux 构造器实际源根；双方收据均核同一 rev，另核候选/包/两代构造器与收据SHA、AA64、独立产物字节；缺代际rc3，坏PE/身份rc1；负控批 `win-arm64-fixedpoint-controls`，不冒认Windows原生自举 |

#### 2.4 生成器（事实表→生成器→表）

| 动作 | 现成用法 | 说明 |
| 切片 NN 迁移清单 | `sh ./unisacc.com tools/slice-migration-gen.cx -- --write\|--check`；校验 `tools/check-slice-migration-gen.cx` | 由 `plans/unit-text.tsv` + `plans/slice-units.tsv` 生成 `plans/slice-nn-migration.md`；`--check` 核两表键 1:1、源文件存在、文档与重生成文本一致 |
|---|---|---|
| 表单元里的 SBOUT 字节反解成文本 | `sh ./unisacc.com tools/decode-sbout.cx [-- FILE] [--min N]`（批次 `decode-sbout`） | 只读；把一段 `["SBOUT",N]` 还原成 `@bytes` 标签要携带的字符串，含 `\0`/`\xNN` 转义；默认扫 `exec/pp/*.tsv`，`--min` 按段长过滤；校验 `tools/check-decode-sbout.cx` |
| 网表裸立即数换成常量引用 | `sh ./unisacc.com tools/lit-const.cx -- FILE OLD NEW [--apply]`（批次 `lit-const`） | OLD/NEW 是整条动作原文，只换完全相同的那条，找不到退出 1；配合 `tools/facts-gen.cx` 派生常量 + manifest 绑定，改后用现装 `out/rtl3/ctx/jobs/tool-{gen,tbl,net}` 重打相关网，JSON 与网 `cmp` 全等才算行为不变；校验 `tools/check-lit-const.cx` |
| 运行库函数只出一份(RT-DEDUP) | `sh ./unisacc.com tools/rt-linkage.cx -- HEADER [--apply] [--skip NAME,...]`（批次 `rt-linkage`） | 把头文件里守卫中恰好一个单行签名的运行库函数改成三态:默认 `static`(字节不变)、`RT_UNIT` 出全局定义、`RT_EXTERN` 只留 extern 声明;变参/多函数/数据/`--skip` 的守卫原样保留并列出 `LEFT`;改后用 `tools/cx-flatten.cx -- --write ROOT` 重生成 cx-flat.h;校验 `tools/check-rt-linkage.cx` |
| Windows argv容量与布局 | `sh ./unisacc.com tools/win-argv-gen.cx -- --check . enc-x86win-winargs enc-armwin-winargs lower-data`；`PRODUCT=新包产品 sh ./unisacc.com tools/check-win-argv.cx` | `WIN_ARGVA_BYTES=131080`唯一事实推导16384项加NULL；双ISA边界/向量模式/显式NULL、PE虚拟尾与漂移负控；批次`win-argv`，Linux交叉检查不等于Windows原生通过 |
| 表闭包静态预检 | `sh ./unisacc.com tools/tsv-lint.cx -- --root . --errors-only --limit 10`；`sh ./unisacc.com tools/check-tsv-lint.cx` | 查模式冲突、未定义引用、缺兜底及精确SB入口清理合同；支持根内call、有限facts/with/let/模板及确定flags。动态宏、fresh、条件和图编辑前置条件未证明者单列`UNRESOLVED`，`status=PARTIAL`且rc0仅表示无确定错误，不代替gen/新包/slice预检；批次`tsv-lint`保留三段历史失败回放与修后负控 |
| 构造前DELTA.json审计（旧工具已恢复） | `sh ./unisacc.com tools/table-lint.cx -- DELTA.json [--strict]` | 保留5596cb4310以来的total/reach/stale-r/t-keys/unwritten/maybe-unwritten等合同；`tests/delta-loader.cx`负责接受/拒绝消费回归。与新`tools/tsv-lint.cx`是不同功能，不能互相替代或共用入口 |
| arity 表与规则对拍 | `sh ./unisacc.com tools/arity-gen.cx -- --check PWD`（`--write` 重生成，`call P N M` 查单键） | `weights/arity/arity.tsv` 243 键＝proto×npar×nargs，与 `seed/gold_arity_label.c` 的 `table` 输出逐字节相等；不计入 18 网，产品 parse 尚未读它 |
| libc99posix 契约表 | `sh ./unisacc.com tools/libc99posix-gen.cx -- --check`（`--write` 生成 `cx/compiler/libc99bind.cx`）；校验 `tools/check-libc99posix.cx` | 保留原八项及 P0 十二项语义，新增 clock_gettime；bind=per-cell 时六格为 KIND:SYMBOL，Darwin 两格 host-export:clock_gettime，Linux/Windows 为 cx-own:-（原系统调用头/签名适配 shim，不直绑 QPC）。前转按完整目标名选格，不以参数数目混同 Linux/Mac；回归检查旧绑定及 clock 六格行为，不钉死总行数。win P0 导出仍按 run42196947660 双架构 UCRT 实物及 ABI/errno 约定，cx-own 核 static 体，两 win 目标 `-c -funit` 实编 |
| 目标信号宏事实 | `sh ./unisacc.com tools/signal-targets-gen.cx -- --check`（默认生成）；检查器 `tools/check-signal-targets.cx -- ROOT`（批次 `signal-targets`） | `exec/facts/signal-targets.tsv` 派生头宏，补 SIGPIPE，Darwin SIGUSR1/2 为 30/31；六目标编译核值，Windows 数字只属进程内模拟兼容空间。未改变 signal/raise 的模拟语义，也不宣称能忽略内核投递 |
| 目标时钟事实与Darwin真单调 | `sh ./unisacc.com tools/clock-targets-gen.cx -- --check`；`sh ./unisacc.com tools/check-clock-targets.cx`（批次`clock-targets`） | CLOCK_REALTIME=0合法，MONOTONIC按目标派生；Darwin宿主libSystem clock_gettime经逐格libc契约，Linux/Windows原路径不变。六格仅编译核常量；50ms单调增量及非法clockid/errno须同c46候选两Mac原生probe，不以Linux或源码检查冒认 |
| include 驱动器号绝对路径 | `sh ./unisacc.com tools/inc-abspath-gen.cx -- --check`；校验 `tools/check-inc-abspath.cx` | `exec/facts/inc-abspath.tsv` 六目标 drive_letter；INC2 字母+':' + '/'或'\\' 且事实为真才绝对 SBFIND；win=1 lnx/osx=0。改 exec/pp 图表须先跑 seed-gen pp --shared-predefines（及 tokenpp/warnpp-shared），已装 -E 不经 seed-gen |
| syscall 分派表（lower） | `sh ./unisacc.com tools/abidispatch-gen.cx -- --check`（`--write` 重生成）；网行 `tools/decide-gen.cx -- --check --each os=lnx,osx,win --each arch=x86_64,arm64 --sections weights/abi/dispatch.tsv SC SC.ask exec/lower/abidecide-result.tsv` | `weights/abi/dispatch.tsv` 522 键＝op×os×arch，是 `weights/gold/abi.tsv` 的投影（有 sysno／win 有 winimp 的 op 标自身，否则 `fail`）；lower 的 `SYSCALL.base` 不再逐 op CMP 链，而是 prelude 写 `mem[SCIX+sysid_op]=序号+1`，`SC.ask.{os}.{arch}` 一次 RLD 选 `SC.<op>`；每个 lower 网只装自己目标那一节（section os.arch）。批次 `check-decide` |

#### 2.5 诊断（度量、盘点、普查）

| 动作 | 现成用法 | 说明 |
| 模型输入影响面投影（遗留 Python） | `python3 tools/affected-model-inputs.py CONTEXT_OR_TAR [--check]`；数据 `tools/affected-model-inputs.tsv` | 不运行生成器，仅由 source-package gen 读记录推影响的模型输入；遗留 Python 依赖，待 cx 化 |
|---|---|---|
| 普查红批次 | `sh ./unisacc.com tools/census.cx -- $PWD out/census` | 无材料批次逐个实跑找回归 |
| 看组成结构树 | `sh ./unisacc.com tools/com-tree.cx` | 每段占文件百分比 |
| 模型解码产物的逐模型 sha(解码器改写的 oracle) | `sh ./unisacc.com tools/model-decode-sha.cx -- EXPLODED_DIR [--reps N]`（批次 `model-decode-sha`） | EXPLODED_DIR 由 `tools/p3-explode.cx -- PKG OUT` 产出;每个模型 inflate 后 `cxmd_decode`,把 CxModel 逐字段渲染后取 sha,打 `MDSHA` 行与总 `all=`;`--reps` 只改计时(`decode_ns`,不含 inflate);改 `cx/runtime/model_decode.cx` 前后 `all=` 必须相同;校验 `tools/check-model-decode-sha.cx` |
| 切片单元只读盘点 | `sh ./unisacc.com tools/slice-inventory.cx -- --slice DIR [--tools-root ROOT]`；`sh ./unisacc.com tools/check-slice-inventory.cx` | 核对象/构造器/容器SHA与同构造器顺序plan映射，输出字节/物理行/use反向闭包/保守标签与top15；字节降序→标签→用户数升序→名称。`identity=VERIFIED`不证明源码与对象历史等价或产品资格；未知纯度归MIXED，批次`slice-inventory` |
| 单元分流 fan + 材料准备 | `sh ./unisacc.com tools/units-prepare.cx -- --out ABS`（幂等 PREPARED）；`sh ./unisacc.com tools/fan.cx -- -j N --env ABS/loader.env` | units.tsv 无绑定为 UNBOUND 不假绿；`--env` 必须绝对路径，缺则 rc2。开发证据非发布资格。生成器 `tools/units-gen.cx -- --check` |
| 切片镜像归属度量（不改产品） | `sh ./unisacc.com tools/image-contrib.cx -- --objects a.o... [--image FILE] [--src KEY=PATH]`；`sh ./unisacc.com tools/check-image-contrib.cx` | 合并tape的`tl_unit` emit按单元计数；最终镜像列恒UNATTRIBUTED（prune/lower/elf丢来源）。分类TRANSFORM/FINITE/UNBOUNDED/SYS-IO来自源码扫描不是对象体积。批次`image-contrib`。试点设计`plans/slice-shrink.md`，fable评审前不动产品 |
| 生成代码运行时性能三档对照 | `sh ./unisacc.com tools/check-bench-gencode.cx -- ROOT [N] [1\|8]` | 同一 `tools/bench-gencode.c` 用 unisacc / gcc -O0 / gcc -O2 各编，1MB+8MB 两档，每项 N 次取中位与 cv；三档 RESULT 校验值必须逐字节相同，否则该项 INVALID；cv>10% 标 NOISY。批次 `cx-bench-gencode` |
| libc99posix 缺口扫描 | `sh ./unisacc.com tools/libc99posix-audit.cx`；校验 `tools/check-libc99posix-audit.cx` | 扫 include/*.h 的 static 函数名对照契约表；GAP 按头文件；UCRT? 为下划线导出候选(未读 PE) |
| 未初始化局部扫描 | `sh ./unisacc.com tools/uninit-scan.cx [-- FILE...]`；校验 `tools/check-uninit-scan.cx` | 流敏感 UNINIT/SUSPECT；正例 `int n=0, owner;`（e91eb31bcf^ kernel.cx）；第一版清单不进棘轮 |
| 改表验证闭环（A12） | `sh ./unisacc.com tools/a12-cycle.cx -- [--dry-run] [--only STEP] [--build B] [--flags F] [--src C [--reject]] [--neg43 EXE]` | gen→tbl→net→UNINETB1(bin) → e2/e1/sim 探针 → decide CTX/RTC → check-decide → ratchets → 冻结43例；G=B/tools 由 main 构造，缺则具名 MISSING；每步 `STEP name rc s receipt`，首败即停；UNISACC_WAVE_REUSE=1 仅子进程；检查 tools/check-a12-cycle.cx（批次 a12-cycle） |



纯 cx 密码学（无宿主库，均过向量）：`crypto.sha256`、`crypto.b64`、`crypto.hkdf`、`crypto.chacha20poly1305`、`crypto.gcm`（AES-128/256）、`crypto.ct`、`crypto.x25519`、`crypto.bn`、`crypto.der`、`crypto.p256`、`crypto.rsa`、`crypto.x509`、`crypto.tls13`、`crypto.record`——用 `#pragma unisa use crypto.<模块>`。

完整入口清单（含每条的说明、既有依据与覆盖边界）见 [archive/plans/2026-11-01/prd-c43-full-2026-11-01.md](archive/plans/2026-11-01/prd-c43-full-2026-11-01.md) 的「已实现能力速查」；这里只列常用入口，具体输入结果与覆盖边界以实际核对为准。

<a id="model-function-bytes"></a>
## 3. 结构组成（unisacc.com 的内部）

用 `sh ./unisacc.com tools/com-tree.cx` 看实时结构。当前发布 0.0.47 = APE `fde2db18…`（6,215,778 字节），实跑取数如下；pkg.bin 1,665,282 字节（P4：40 模型、1823 阶段路由行、77 资源）。win/arm64 不在 APE 的 gzip 切片段里，作为独立发布资产 `slice-win-arm64`（3,367,424 字节）随发，见 [plans/release-notes-0.0.47.md](plans/release-notes-0.0.47.md)。

```
unisacc.com  6,215,778 B  sha fde2db18…
├─ head: PE 镜像 + shell 启动器      2,663,424 (42.8%)  win/x86_64 原样存；DOS stub 放 sh 启动器
├─ slice lnx/x86_64                    372,017 ( 5.9%)  gzip → 解出 1,880,766 B
├─ slice lnx/arm64                     351,487 ( 5.6%)  gzip → 解出 1,884,814 B
├─ slice osx/x86_64                    383,660 ( 6.1%)  gzip → 解出 2,410,978 B
├─ slice osx/arm64                     379,590 ( 6.1%)  gzip → 解出 3,071,458 B
├─ (padding 合计)                      400,318 ( 6.4%)
└─ package (P4)                      1,665,282 (26.7%)  40 模型 + 1823 阶段路由行 + 77 资源
   ├─ stage routing (D rows)            104,052
   ├─ models                          1,316,990        40 模型（deflate）
   └─ resources                         244,209        77 资源（@hdr/* 69 个头文件 227,391；最大 cx-flat.h 101,204）
```

六片 = 四片 gzip 的 Unix 编译器 + 外层 win/x86_64 PE 头 + 独立资产 win/arm64 切片（`slice-win-arm64` 3,367,424 B，不计入上述 APE 字节），共用一份 P4 模型包。

<a id="pipeline-design"></a>
## 4. 架构：模型流水线

<a id="flow-net-contracts"></a>
### 4.0 七流（字节管道）

真源 `exec/pipeline/image-stages.tsv`：

| 流名 | 输入 → 输出 | 功能 | 主要网 |
|---|---|---|---|
| **e2** | `src.c` → `pp.text` | 预处理：续行/注释/指令/宏/条件/include | pp |
| **e1** | `pp.text` → `tokens.typed` | 词法：关键字/标识/算符/字面量最长匹配 | lex |
| **e3** | `tokens.typed` → `tape.text` | 语法+类型等合成段 → tape（parse/type 等物理嵌此） | parse, type, scope, irsel, tyinfo, pfconv, binsel, prec |
| **e4** | `tape.text` → `tape.text` | opt：peephole / 栈→寄存器类改写 | opinfo, peep |
| **prune** | `tape.text` → `tape.text` | 按根/导出剪不可达整函数 | （无 18 网，政策在代码） |
| **lower** | `tape.text` → `target.text` | ABI/regmap/syscall/布局/reloc 降到目标 ops | abi, regmap, reloc |
| **elf** | `target.text` → `image.bytes` | 编码 + ELF/Mach-O/PE 写出 | enc |

### 4.1 十八网（ask 事实表）

真源 `weights/gold/*.tsv`（`tools/gold-table.cx --check` 核对）：pp、lex、parse、type、scope、irsel、enc、reloc、tyinfo、pfconv、regmap、isel、abi、combo、binsel、prec、opinfo、peep。每张是「有限表 + 全键穷举 exact」，由 `seed/gold_<阶段>_label.c` 规则形式构造。**对照臂**（七流的 42 张网不读，变异实验改一键 0 网变化，2026-11-05 plans/gold-mutation-2026-11-05.md）：combo、isel、parse、pfconv、scope——仍进 weights/built.json 与 kernel/*.inc，供 18 网经典批次与 gold 对照工具使用，不删；parse 的语句分派已由 weights/stmt/stmt.tsv 决策表替代。其余 13 张（pp lex type irsel enc reloc tyinfo regmap abi binsel prec opinfo peep）及 weights/stmt、weights/arity 是七流出货主脊（改一键必改对应网）。

### 4.2 构造链

```
有限表（.tsv）→ 生成器（tools/*-gen.cx，带 --check）→ 网权重 → 通用执行器（cx/core/kernel.cx 的 UNIKERN1）
```

- 有限表只经「事实表 → 生成器（--check）→ 表」修改，不手改。
- 通用执行器：定宽运算、栈/存储、字节/帧、资源与接受/拒绝。语言、ABI 与布局语义仍由阶段规则拥有。
- 模型包 P3 = 32 模型 + 阶段路由 + 资源，装载由 `cx/runtime/model_decode.cx` / `packagemodel` 处理。

### 4.3 分层:booter / 基座 / 阶段权重 / 资源(架构方向,2026-11-02 主人定)

```
L0 booter(APE com,极小)   识别 OS/架构 → 按 manifest 找到并校验基座与阶段权重 → 交给基座执行
L1 基座(每架构一个文件)    通用执行器("权重换逻辑"的机器,即 kernel);外部文件,按内容哈希寻址
L2 阶段权重(7 流/18 网)    e2/e1/e3/e4/prune/lower/elf 各一个模型文件;外部文件,按内容哈希寻址
L3 资源                    头文件、事实表等
```

- **一切按内容哈希寻址;内置 pack 只是 resolver 的另一个来源**,与外部目录行为完全一致。
- `manifest.tsv`:`name kind(base|stage|resource) arch sha256 size order`。来源顺序:`UNISA_HOME` → com 旁目录 → 用户缓存 → 内嵌 pack(UNIPKG1 footer)。命中必校验 sha256,不符具名拒绝,**不静默回退**到别的来源。
- **产品身份 = manifest 的 sha256(Merkle 根)**。取代 `pp_product_digest` / `ape-receipt` / `build.json` artifact 段等多处各算一份的口径(见 `plans/identity-inventory-2026-11-02.md`、`plans/identity-unify-2026-11-02.md`)。kernel context 仍独立于它(判断 kernel 可否复用,不随全树变),两代 GEN 互证仍保留。
- 目的:每一层单独固化、单独开发、单独测试;阶段测试=输入流字节→输出流字节的 golden;换基座不动权重、换权重不动基座;慢车道缩成"只重算被改文件及其下游哈希"。
- 单文件发布(pack 内嵌)保留;外部目录形态用于开发。启动开销是主要风险(现热 `-run` ≈10ms),每步测量。
- 进度与分步:见 `agentwork/items.tsv`(B0 P3 explode/implode → B1 resolver → B2 阶段测试台 → B3 基座外置 → B4 booter APE 头 → B5 身份收敛到 manifest)。
- 现状(2026-11-06):L1/L2/L3 的外置与 booter 分离([B3][B4][B5c])未完成,发布形态仍是单文件 APE(尾部一份包);已落地的是 resolver/explode 工具与 `unisacc stream STAGE` 阶段过滤器。三层口径(loader / ISA 切片 / 模型)与 CORE-MIN 见 §9.1。

## 5. 源码结构（src/ 六层 + cx/ 树）

`src/unisacc.cx` 是入口，`use drive`。六层（文件数与行数以 `tests/srcshape.tsv` 为准）：

| 层 | 文件 | 职责 |
|---|---|---|
| host | `host.cx`、`hostnative.cx` | 平台胶水（mmap/dlopen/进程），只在 L0 `cx/sys` 之下 |
| packmodel | `packmodel.cx` | 装载 P3 包、解压模型 |
| executor | `executor.cx` | 调 `core.kernel` 的 `core_run`（UNIKERN1 汇编核心） |
| stages | `stages.cx` | 按包里的阶段装模型，交给 `core_run` |
| drive | `drive.cx`、`driveroute.cx`、`driveface.cx`、`runcache.cx` | 驱动、路由、缓存 |
| bind | `bind.cx`、`entrybind.cx`、`entryexec.cx` | 入口绑定、晚绑定 |

`src/core`、`src/runtime` 是指向 `cx/` 的符号链接。

**cx/ 树**（`.cx` 模块体系，`#pragma unisa module/use`）：

- `cx/sys/`（L0）：对接 libc/POSIX 的唯一层（env/fs/proc/time/net/poll/tty/posix/sha/mem/dl/sig…）。
- `cx/mem/`（L0）：所有者缓冲、arena、slice（Rust 式 Vec/String/所有权）。
- `cx/text/`（L0）：带长度字节串比较/格式化/拼接（`tx_*`）。
- `ext/crypto/`（包层 EXT）：纯 cx 密码学（sha256/b64/hkdf/aes-gcm/chacha20poly1305/x25519/bn/der/p256/rsa/x509/tls13）。
- `cx/core/`（L1）：执行器（kernel/execute/stack/output/bytes/memory）。
- `cx/driver/`（L1）：path/compat/args/output/deps/cli/route。
- `cx/runtime/`（L1）：model_decode/package_*/stream_*/source_frame 等。
- `cx/compiler/`（L1）：cxmod（模块解析）/asmtext（汇编文本）/streamname/netname 等名表。

分层铁律：L0 才对接 libc/POSIX，上层禁 `#include` 和裸 libc（`tests/cxinclude.cx`、`tests/cxlibc.cx`、`tests/cxraw.cx` 三个棘轮，只降不升）。

## 6. 事实表与权重（exec/ 与 weights/ 与 seed/）

- `exec/facts/`：编译各阶段的**事实表**（pp-targets、lex-consts、lower-abi、image-elf/macho/pe…），经 `tools/facts-gen.cx` 生成，只经「事实表 → 生成器 → 表」修改。
- `exec/`（enc/lex/lower/opt/parse/pp/prune/tapecheck…）：各阶段网络的事实与构造（含 `exec/c/` 旧 C 参考，只供对照）。
- `weights/gold/`：十八网金表；`weights/*.f32/i8.unisa`：构造出的权重。
- `seed/`：C 种子构造器（`gold_<阶段>_label.c` 规则形式、`ident.c` 身份、`ape.c` 打包）。

## 7. 测试套件设计

### 7.1 入口

```
sh ./unisacc.com tests/run.cx -- --smoke | --all | --list
```

- `--smoke`：本地冒烟（product 层 smoke 批次，条数以 `--list` 为准）。
- `--list`：列全部批次 + 覆盖行 + 计划计数。
- `--all ABS_SESSION`：全量资格判定（`tools/all-waves.cx` 会话 + `tests/all-aggregate.cx` 聚合）。

### 7.2 注册表（两份 TSV）

- `tests/batches.tsv`（行数以 `tests/run.cx -- --list` 为准）：每个测试批次一行——id、implementation、layer、source、mode、platform、budget、materials、entry、args、coverage、expect。`IMPLEMENTED` 才进 QUALIFIED，`PLANNED` 不进。
- `tests/coverage.tsv`（行数以 `--list` 为准）：每个覆盖行一行——id、layer、implementation、expected_or_gap、source。一行被至少一个 IMPLEMENTED 批次 PASS 即 satisfied。

### 7.3 分层（coverage.tsv 的 layer 列）

| 层 | 覆盖行数 | 内容 |
|---|---|---|
| contract | 169 | 语言/流/网合同（正反例、拒绝） |
| interfaces | 199 | 流间接口、模块接口、L0 接口 |
| negative | 32 | 具名拒绝、坏输入、语义级坏包 |
| product | 32 | 产品端到端（编译/运行/镜像字节） |
| library | 15 | 库/ABI（libunisacc、头布局） |
| platform | 6 | 跨平台（六目标写程序；六 ISA 切片需求与现状缺口见 §1） |
| properties | 9 | 优化/property 性质 |

以上为 2026-11-08 在克隆实跑 `sh ./unisacc.com tests/run.cx -- --list` 的取数，覆盖行合计 462；`PLAN` 行：metadata_valid=1、batches=550（implemented 538 / planned 12）、implemented_coverage=451、unresolved=6、unprepared_batches=167、`qualification=NOT_EXECUTED`（`--list` 不执行批次）。数字随注册表变化，以 `--list` 为准。

### 7.4 19 张域矩阵（tests/*-domain-matrix.tsv）

每张矩阵是一个「域」的逐格合同：`bootstrap`、`candidate`、`formats`、`hosts`、`interfaces`、`library`、`negative`、`optimization`、`phase-timing`、`properties`、`resources`、`ri`、`stream-e1/e2/e3/e4`、`stream-elf`、`stream-lower`、`stream-prune`。

格状态：`COVERED_LOCAL`（本机批次断言了）/ `OBSERVED`（原生宿主观测）/ `DECLARED_DEPENDENCY`（有意声明的宿主依赖）= 满足；`MISSING` / `NEEDS_PRODUCT_CHANGE` / `NEEDS_FOREIGN_HOST` / `BLOCKED` = 开着的。

- `full-domain.cx -- . DOMAIN` 逐格判域 PASS（开着的格为 0 才 rc0），19 个 `full-<域>` 批次就是它。
- **外平台格**（`NEEDS_FOREIGN_HOST`）只能在**该原生宿主**上跑同一候选关掉——模拟/容器/缺席客机一律 BLOCKED/NOT_RUN，永远不算 PASS。逐格 oracle 见 [plans/foreign-host-runbook-cc.md](plans/foreign-host-runbook-cc.md)，原生 runner 见 [.github/workflows/candidate-native.yml](.github/workflows/candidate-native.yml)（ubuntu-24.04 / ubuntu-24.04-arm / macos-15-intel / macos-15 / windows-2025 / windows-11-arm）。
- **已知缺口豁免**：`tests/waivers-0.0.44.tsv`（R 行理由 + W 行豁免 + F 行已知 FAIL 批次，F 行不改结论），`matrix-waive.cx --check` 保证记录和矩阵一致。

### 7.5 QUALIFIED 判定

`--all` 跑 `tests/all-aggregate.cx`，QUALIFIED = **每个 IMPLEMENTED 批次有同候选 PASS 收据 + 0 unsatisfied + 0 unresolved 覆盖行**。材料身份绑定候选（换候选须重构造材料）。FAIL/REFUSED 粘住会话。

## 8. 目录与文件结构（顶层）

| 路径 | 内容 |
|---|---|
| `src/` | 产品六层入口（规模以 `tests/srcshape.tsv` 为准，srcshape 棘轮只降不升） |
| `cx/` | `.cx` 模块树（sys/mem/text = L0；core/driver/runtime/compiler = L1）；包层在 `ext/<package>/`（crypto 在此） |
| `exec/` | 各阶段事实与构造（facts/enc/lex/lower/opt/parse/pp/prune/tapecheck；c/ 旧 C 参考） |
| `weights/` | 十八网金表 + 构造权重 |
| `seed/` | C 种子构造器（gold label 规则、ident.c 身份、ape 打包） |
| `include/` | 头文件（cx.h 前导 + 69 个 std 头） |
| `tests/` | 测试套件（batches/coverage 注册表 + 19 域矩阵 + 各层测试 .cx） |
| `tools/` | 开发工具（个数以 `ls tools/*.cx` 为准：生成器/棘轮/refactor/symindex/all-waves…） |
| `plans/` | 活跃计划（v0.0.4x 路线 + 架构/分层/缺陷；已完成的归档到 archive/plans） |
| `docs/` | 规格/术语（spec.md 在根、docs/glossary.md、docs/cx/） |
| `research/` | 论文五线（A 流网换逻辑、B ujs、C 管道、D cx 模块化、E 系统构表） |
| `archive/` | 归档：reference-c/reference-py（旧实现）、testing-c43-legacy（旧 1333 项套件）、plans/（历史计划）、prd 历史 |
| `apps/csih/` | csih 车道（归 csih 主人） |
| `ujs/` | 论文 B（归论文 B 主人，unisacc 侧不碰） |
| `adapter/`、`m4/`、`flownet/` | 流×网范式的适配/m4/通用流水线 |
| `out/` | 临时产物（gitignore，日志/候选/材料/会话） |
| `kernel/` | 生成物（禁止手改；权重由 gold 构造） |
| `agentwork/`、`release/`、`agentwork/roles/`、`skills/`、`bench/`、`scripts/`、`examples/`、`iterate/` | 任务看板与账（agentwork/，管理账不是事实表）、发布（release/candidate.tsv、SHA256SUMS.template）、代理角色、技能、基准、脚本、示例、迭代记录；完整四组地图见 [docs/REPO-MAP.md](docs/REPO-MAP.md) |

<a id="current-plans"></a>
## 9. 当前状态与路线

<a id="work-tree"></a>
### 9.1 思维树（当前节点；每轮先读，事实/阻塞/下一动作原位更新）

当前版本：**0.0.47 已于 2026-11-08 正式发布**（tag `v0.0.47`，提交 `e8309f0488`；GitHub Release 非 draft 非 prerelease；APE sha256 `fde2db1812533a9a56a66d8e0d1870c310fdcc77d444a2d33210ca96e9ef98fe`，6,215,778 字节；冻结产品源 `7b98ec2c06`；同候选六目标原生 CI 运行 `42408207540` 为 42/0）。全批 s9：445 批 = 380 PASS + 0 真 FAIL + 32 KNOWN-GAP + 10 INFRA + 23 NOT-AVAIL，**NOT_QUALIFIED**。上一版 0.0.46 同日早些时候发布。事实见 [plans/release-notes-0.0.47.md](plans/release-notes-0.0.47.md)。

下一版：**0.0.48**，计划见 [plans/v0.0.48-unisacc.md](plans/v0.0.48-unisacc.md)；任务状态以 `agentwork/items.tsv`、进度以 `agentwork/ledger.tsv` 为准，本节不复述条目状态。

三个最大阻塞：
- 全批资格：32 KNOWN-GAP / 10 INFRA / 23 NOT-AVAIL 未收敛，全批仍 NOT_QUALIFIED。
- Windows 两格 stdio/stream 仅 portable 子集，完整 POSIX 与六目标全部原生验证未声称。
- 已披露的既有缺陷与推迟项（A5 conflicting-function-types、TAPE-END、对象编译合同债等）：限制清单见 [plans/release-notes-0.0.47.md](plans/release-notes-0.0.47.md) 的「已知限制」与 [README.md](README.md) 的 Known limitations 表，此处不复写。
<a id="memory-palace"></a>
### 9.2 记忆宫殿（索引；此处是唯一索引，正文各节只指向本表）

| 房间 | 当前事实 | 入口 |
|---|---|---|
| 任务队列 | `agentwork/items.tsv`（owner 列=tmux 窗口名；只有 READY 项被 omp-workflow 自动续跑）；进度追加在 `agentwork/ledger.tsv` | [agentwork/WORKFLOW.md](agentwork/WORKFLOW.md) |
| 当前发布 | 0.0.47 已发布（见 §9.1）；0.0.46 同日早些时候发布；旧候选资格不迁移 | [0.0.47 发布说明](plans/release-notes-0.0.47.md) |
| 下一版 | 0.0.48 | [计划](plans/v0.0.48-unisacc.md)、`agentwork/items.tsv` |
| 新入口 | `src/unisacc.cx` 六层（host/packmodel/executor/stages/drive/bind）；`src/core`、`src/runtime` 指向 `cx/` | [plans/cx-modules.md](plans/cx-modules.md)、§5 |
| 流/网 | 7 流 + 18 网 | §0、§4 |
| 测试入口 | `sh ./unisacc.com tests/run.cx -- --smoke/--all/--list` | §7.1、`tests/batches.tsv`、`tests/coverage.tsv` |
| 分层 | L0 `cx/sys`·`cx/mem`·`cx/text`；包层 `ext/crypto`；L1 `cx/core`·`cx/driver`·`cx/runtime`·`cx/compiler` | [plans/cx-layers.md](plans/cx-layers.md)、§8 |
| 棘轮 | 13 项，以 `tools/ratchets.cx` 末行为准；默认只降不升，功能性工具行数扩额须授权、实测并分清既有偏差 | `tools/ratchets.cx`、§2.2 |
| 事实表 | 事实→生成器（须带 `--check`）→产物 | `exec/facts/`、§2.4 |
| 静态预检 | 模式/引用/兜底/SB 合同先查；动态及未证明性质单列 PARTIAL，不能替代真实 gen/新包/slice | `tools/tsv-lint.cx`、`tools/chain-precheck.cx` |
| 结构盘点与试点 | 对象清单已交付，镜像单元贡献仍 UNATTRIBUTED；link-map 边车方案保留不实施 | `tools/image-contrib.cx`、[plans/slice-shrink.md](plans/slice-shrink.md)、[plans/link-map-sidecar.md](plans/link-map-sidecar.md) |
| 归档 | 旧 C/Python 实现、旧 1333 项套件、历史计划与 PRD | §10、`archive/` |

### 9.3 往期发布（历史）

0.0.43 历史见 [plans/release-0.0.43.md](plans/release-0.0.43.md)；0.0.45 公开预发布见 [plans/release-notes-0.0.45.md](plans/release-notes-0.0.45.md)；逐版身份见 §10 归档索引。

### 9.4 路线
- **0.0.43 – 0.0.47**：已收口（历史计划见 `plans/v0.0.4x*.md`，当前发布见 [plans/release-notes-0.0.47.md](plans/release-notes-0.0.47.md)）；agent harness（cx-csih）仍归 csih 车道。
- **0.0.48**：见 [plans/v0.0.48-unisacc.md](plans/v0.0.48-unisacc.md)。
- **0.1.x**：Clang/GCC/TinyCC 接入 `.cx`（共用前端 + 后端插件）。
- **0.1.x**：Clang/GCC/TinyCC 接入 `.cx`（共用前端 + 后端插件）。

<a id="optimization-leads"></a>
### 9.5 优化线索

2026-11-03 实测的优化线索表（libc 闭包、整树 SID、生成代码性能、资源段、执行内核/loader 体积等）已登记到各版计划与 `agentwork/items.tsv`，此处不复写；入口见 [plans/v0.0.48-unisacc.md](plans/v0.0.48-unisacc.md)，旧表全文见 §10 的完整旧版。

## 10. 归档索引

历史 PRD 全文、逐版发布身份、旧测试套件、旧 C/Python 实现、历史计划与过程归档的入口：

- [archive/plans/2026-11-01/prd-c43-full-2026-11-01.md](archive/plans/2026-11-01/prd-c43-full-2026-11-01.md) —— 本文件重写前的完整旧版（含思维树/记忆宫殿全流水、能力速查全文、§3 架构全文、§5 路线全文、§6 未解决问题）。
- [archive/prd-c43-history-20261030.md](archive/prd-c43-history-20261030.md) —— 0.0.43 历史收口。
- [archive/prd-release-and-development-history-20261001.md](archive/prd-release-and-development-history-20261001.md) —— 逐版发布身份。
- [archive/testing-c43-legacy/](archive/testing-c43-legacy/) —— 旧 1333 项测试套件（已停调度）。
- [archive/reference-c/](archive/reference-c/)、[archive/reference-py/](archive/reference-py/) —— 旧 C/Python 参考实现。
- [archive/plans/](archive/plans/) —— 历史计划与设计探针。

〔2026-11-08 cc评审后续，C47-DRIVER-TARGET-OBJECT / C47-LOGIC-DISTRIBUTION〕先登记两项任务。修复已装0.0.47的--target拒绝与裸-c只出tape回退，保留默认-run最小复现，补六目标格式合同并重建核身份；独立普查表/生成程序/网/经典机制代码及六片解压模块字节，明确统计分母、重叠与未归因，为D1设计提供数据。当前均DOING，未借旧CI或发布资格。

〔C47-DRIVER-TARGET-OBJECT 定位/决定〕已装0.0.47默认-run最小main成功；--target lnx/x86_64 -o报option not migrated，裸-c输出_start tape（out/c47-driver-fix/*.before.log）。--target只选目标不覆盖-E/-S/-c；裸-c缺目标时取tn_self，非Linux沿现有-funit容器合同（Mach-O/COFF只载unit tape，由unisacc链接，不宣称系统链接器可用），保留-S与--tapebin显式文本路径。旧cxt_object低层拒绝合同保留，driver在展开前规范化。

〔C47-LOGIC-DISTRIBUTION，2026-11-08，实际实跑，尚未完成最终镜像模块归因〕新增 `tools/logic-distribution.cx`：`--sources ROOT` 按 tracked regular 文件、固定根 src/cx/exec/seed/weights/tools 做互斥的 TABLE(.tsv) / GENERATOR(*-gen.cx、seed/gen.c、seed/gold_*) / NETWORK_REPRESENTATION(.net/.tbl/.rows) / CLASSIC_MECHANISM(其余源码)分组，列每文件字节/物理行及分类分账；if/case/switch/? 跳过注释与字面量后只作词法代理，不等于语义决策或运行频率。当前2290文件、13,760,638B：表5,771,926B(41.9451%)，生成器1,450,565B(10.5414%)，经典机制6,538,147B(47.5134%)；tracked网表示0B仅因构造产物未跟踪，绝不表示产品不含网。事实表与其生成网有信息重叠，不能把以上字节份额当“以网换逻辑”的决策覆盖率。独立语义oracle仍未补齐。

同工具 `--ape FILE --package FILE --slice TARGET FILE` 核实际解压CRC/SHA、包内模型/资源/元数据，六片当前实测：lnx/x86_64=1,880,766B、lnx/arm64=1,884,814B、osx/x86_64=2,410,978B、osx/arm64=3,071,458B、win/x86_64外层PE=2,663,424B（含APE stub/version，不冒充原始裸片）、独立win/arm64=3,367,424B、sha889f36cd…（父提供公开候选资产）。包1,665,266B=模型存储1,315,916B+资源241,070B+目录/元数据108,280B；模型行以PACK_ROUTE绑定流与索引，资源列真实名字。证据 `out/logic-distribution-current.tsv`、`out/logic-distribution-slices.tsv`，输入为当时已装fde2db18…产品与公开win/arm64，父后续重建必须重新绑定身份。

**仍缺最终模块字节归属，目标保持未完成**：当前镜像无来源映射，经典路径tl_unit→prune→lower→elf丢单元身份且可跨单元融合；不能按对象.text比例或旧 `plans/unit-text.tsv` 分摊现候选镜像。工具每片明确 MODULE UNATTRIBUTED、归因0B并核守恒；这只验证未知账完整，不构成模块分账完成。下一步按 `plans/link-map-sidecar.md` 在构造链保留单元来源集合、融合记SHARED、最终布局输出同镜像SHA绑定的不重叠区间，再由普查消费；不能仅补节区或符号大小冒充模块。检查 `tools/check-logic-distribution.cx` 已装产品默认-run实跑6/0，覆盖词法正负例、分类、未知模块守恒、坏格式及无参拒绝；批次登记工具因既有inventory-root-layout拒绝需要父处理，不记完整批次PASS。

能力速查补充：逻辑/体积普查 `sh ./unisacc.com tools/logic-distribution.cx -- --sources . --ape unisacc.com --package unisacc.com [--slice TARGET FILE]`；检查 `sh ./unisacc.com tools/check-logic-distribution.cx`；缺最终机器码模块来源时明确UNATTRIBUTED，不按比例摊账。

〔C47-DRIVER-TARGET-OBJECT 测试入口修复，2026-11-08〕已装默认-run把argv[0]设为实际临时可执行文件，不是.cx源路径；tests/run.cx的inventory原先依赖argv[0]导致--list误拒inventory-root-layout。改为源身份__FILE__定位仓根，保留realpath失败、ROOT/tests/run.cx后缀、metadata-io/shape/schema的全部拒绝；修改前--list实际红，修改后须真实--list与smoke验，不用薄解析器代替入口资格。

〔C47-LOGIC-DISTRIBUTION 来源设计裁决〕仅设计评审建议：先做合并tape来源账本、诊断模拟器最终写出事件、绑定规则内容SHA的来源证书；只认领完整来源链已证明的模块区间，其他继续UNKNOWN。输入cursor/输出长度只证明执行关联，相邻标签差或字节cmp不能补来源证明；通用复制/切片可先证，融合需来源集合，控制依赖不得直接叫SHARED。私有测量不复制lower算法、不增加产品默认路径状态；先用两单元同名局部标签/共享BSS、缓冲覆盖、双遍布局负控证明最小接口，再量真实六片。报告在out/c47-driver-fix/provenance-design.txt，仅设计未实跑，不算模块分账已完成。

〔C47-DRIVER-TARGET-OBJECT 测试入口实证〕`tests/run.cx -- --list`修后实际metadata_valid=1（555批），smoke实际16/0，缺表仍两条metadata-io errno2；`tests/run-batch.cx`对logic-distribution实际PASS/6断言，绑定旧已装fde2db18…。既有tools/check-register.cx在55s看门狗处超时，前段断言绿但全检查未完成，原始日志保留、不记PASS。新增裸-c对象→产品链接镜像→执行返回23两条永久断言；对完整私有APE f069d4c9…实际cli 9/0（旧根产品仅作启动器，测试ROOT明确私有），不声称系统链接器兼容。证据out/driver-target-object-initial/{registry-fixed,smoke-fixed,registry-missing-tables,register-fixed,run-batch-logic-absolute,cli-final-object-link}.log。

〔C47-DRIVER-TARGET-OBJECT 身份工具缺陷〕本地新APE已安装并备份旧6215778B产品，但formal seed/ident写源记录后，migrate-build-record在原记录缺kind/digest两字段时产出缺逗号的无效JSON；已保留identity-migrate.log，不能记身份核验完成。定位为build_record的末字段判断及追加kind逗号条件均反向，须修生产者并用四种字段存在组合校验，再重生成当前记录；不手改资产摘要冒绿。

〔C47-LOGIC-DISTRIBUTION 诊断写出事件MVP，2026-11-08〕`tools/sim.cx --write-events NEW.tsv [--event-limit N] [--event-bytes N]` 只在显式启用时收集关联：state/key/action序号、frame实例/content SHA/cursor、COPY/SPAN输入范围、输出/诊断/scratch缓冲长度、SWAP前后输出代次。OFILL同长覆盖、OCUT/OCLR/ODROP显式记事件，不依赖长度差；非直接复制来源记-1/UNKNOWN，所有模块所有权仍UNKNOWN。事件上限默认100000、硬上限1000000；文件上限默认16MiB、硬上限64MiB，含身份与COMPLETE元数据。只在ACCEPT后以独占临时文件写完并原子无覆盖hard-link发布，拒绝/超预算不发布完整账本。delta/input/output绑定SHA；输入为冻结材料，资源/外部文件来源链未补齐，事件不是模块所有权证书。父安装产品sha=f069d4c92b7f5ede508809aa3abd21e326cbecadb2fb9eccf0bd1ac7004c3e82，工具默认-run实跑独立小delta开关均输出B，首轮check13/0；帧实例与SHA缓存增强后按新输入重跑，不借首轮证据。真实六片最终模块分账仍未完成，下一步消费事件连接合并tape来源与最终布局，融合/共享来源需另证。

〔C47-DRIVER-TARGET-OBJECT 本机产品实证〕冻结3b41cc224的六片均已构造，mac/win whole-o内限50s、Linux独立对象每步<=55s；完整APE f069d4c92b7f5ede508809aa3abd21e326cbecadb2fb9eccf0bd1ac7004c3e82 / 5818178B，单列win/arm64 fcfcc4135d9bd7fc71cd70e5dfae694c969047442d9594b2c0b49e0ac84792e6 / 3369472B。最终APE四批格式/对象20断言全绿（OS三批11、cli9，含对象经产品链接后实跑23），smoke5/0。本机已经install-ape安装，旧公开字节和侧车备份out/c47-driver-fix/before-install；未改远程release/tag/资产，原CI资格不迁移。身份生产者两处逗号判断已修，四存在组合及幂等8/0；源摘要9cc514d3…，新侧车配对，ROOT_IDENTITY9/0；plutil转换真实解析成功。侧车commit ac8f94dd7与冻结3b41cc224的产品输入src/cx/exec/include/seed/ext/kernel/weights零差异。完整棘轮/注册工具长检查仍未绿，tools功能扩额许可待答，不能称全仓通过。
