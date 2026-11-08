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

1. 「7 流」与「18 网」不是同一批东西的两种叫法——流是**字节管道阶段**，网是**由 ask 事实表构造的 NN**（定义见 §0.1）。
2. `class Stage`（旧 `unisa/gold.py`）表示**网**，不是流。
3. 旧 `unisa/control/net.py` 的 `Net` 是**神经网络模型**，不是事实网。
4. m4 的「4 流」是**四个字节段**（lex/front/select/encode），不是 4 张决策网。
5. `phase` 三义：book phase（记账）/ release phase（发布）/ 历史 compiler phase（实为 segment）。
6. `STAGE_NETS` 只是 isel、abi 两张不在 `TABLES` 里的网（15+2+1=18）。

**禁用写法**：裸 `stage` 一律写 `stream-stage`（流）或 `net`（网）；「网」专指 §0.1 定义的构造 NN；包里的执行单元写 `model`（模型 ≠ 网，一个模型可含网+生成程序+常量）；旧 `unisa/control/net.py` 那类训练用神经网络写「训练模型」。

### 0.1 范式与核心概念（定义；全文以此为准）

**范式一句话**：用 NN 替代逻辑——编译器中的表状决策先写成**事实表**，再由生成器**确定性构造**成 NN，由一个通用执行器运行。「以网换逻辑」「NN 替代逻辑」「TSV→NN」是同一命题的三种说法，不存在冲突。

| 概念 | 定义 | 不是什么 |
|---|---|---|
| **事实表**（tsv） | 有限定义域上的决策映射，人可审、可 diff；范式的**源** | 不是生成物，不手改生成结果 |
| **生成器** | 表 → 网/程序 的确定性构造程序（`tools/*-gen.cx`、`seed/gen.c`），带 `--check` | 不是训练 |
| **网**（NN） | 由事实表构造的小整数阈值神经网络；在表的**全定义域逐键验证 = 表** | 不是训练所得，不声称泛化；验证只证「网 = 表」，不证「表 = C99」 |
| **生成程序** | 生成器产出的转移/动作序列，由执行器运行（承担网之外的动作落地） | 不是手写代码 |
| **模型**（kind N） | 包里的执行单元 = 选择器（网）+ 生成程序 + 常量；P4 包 40 个 | 不等于「网」的个数 |
| **执行器** | 通用机制（定宽运算、栈、字节/帧、资源），语义全来自模型 | 不含语言/ABI 特判 |
| **机制代码** | 尚未进表的经典代码：循环/递归/带副作用序列等 | 不是范式失败，是范式**尚未覆盖**的剩余空间 |

**两套计数不混用**：经典参考 **18 张金表网**（`weights/gold`，其中 5 张为对照臂）；默认产品阶段包 **42 张构造阈值网**（分布在 40 个模型里）。

**替代发生在哪一步**：逻辑 → 事实表 这一步把决策从代码里拿出来；表 → 网 是等价的执行形式转换。覆盖度的度量看「决策进表的比例」，现状见 §9.6（源码中机制代码约 49%，是后续推进空间）。

**RI 的固定含义**：本文 RI = **ResourceInput（具名资源输入）**，含名称、字节和长度、存在性/空值及跨流身份绑定，不是 "runtime integrity"。

## 1. unisacc.com 是什么

**一句话**：unisacc.com 是 C99 编译器（带部分 POSIX 库子集），打包成单个类 APE 文件——核心需求是六个 ISA 切片（lnx/osx/win × x86_64/arm64，每个目标同候选原生证据；现状缺口见下），能为 {Linux, macOS, Windows} × {x86-64, arm64} 六目标直接写出可执行文件，并以默认 `-run` 直接运行 `.c` 和 `.cx`。cx 是参照 Rust 设计的模块化方言（`#pragma unisa module/use`），所有权与边界目前为**运行期**检查，不等于编译期借用检查。编译内部是 7 个字节流阶段，表状决策交给 18 张事实网（16 张出货，isel/combo 为对照），网络权重由真值表构造并穷举验证、不经训练。通用执行器执行构造出的选择器与动作程序；这是决策表示的改变，尚无语义决策覆盖率证明，不能由模型体积推出“以网换逻辑”。

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
| 网级依赖图与第14号波及面门 | `sh ./unisacc.com tools/net-deps.cx -- [--summary\|--touch FILE...\|--oracle]`；`tests/net-deps.cx --check` | 路径别名统一规范化；原工具25/0、棘轮行为7/0；42配方静态上界实测pp3/lower12/generator42/facts30，无预留。批次net-deps及net-deps-ratchet；不是运行读集完备或语义覆盖证明 |
| 0.0.48底子执行（cc续派） | BASE-1..6与C0.2登记于items；父集成BASE-1/BASE-6，独立子任务C0.2摘要、BASE-2耗时、BASE-5函数矩阵；BASE-4复用I2原型 | 先落实依赖/摘要及单网实测，再做真实pp扰动增量验证；仅冻结输入运行，不改产品输入。逐项提交同步并在计划§14填实际数字，全部完成后接X2；超过5分钟先分解，已有60秒子步骤看门狗不放宽 |
| 单网闭包内容摘要（C0.2） | `sh ./unisacc.com tools/net-digest.cx -- --root DIR [--closure TSV] --net NAME\|--all [--export-closure NEW]`；正式net-digest批15/0 | 三真实pp配方端到端；长度分帧绑定recipe参数、排序canonical路径/kind/内容与工具身份；闭包内变/外不变。缺输入、符号链接、权限、坏图、重复、逃逸、未知网均拒绝且不出摘要。旧tree_digest未找到定义，本次复用现SHA库实落地；显式闭包只适用于已冻结图，不证明运行读集完备或构造/全键验证通过 |
| 逐网构造耗时（BASE-2） | `tools/net-build-profile.cx -- --root ROOT --gen GEN --tbl TBL --net NET --out NEW --start N --count K`；`--summary PROFILE...` | 官方42配方三阶段实际构造累计26559ms，p50=483/p90=807/p95=2466ms，最长warnparse3138ms；7批全rc0，原始命令/输入/产物SHA见research/c48-base-build-profile/。检查9/0含重复配方/混身份拒绝；现将阶段子时限钳入45秒剩余预算，实测仍按原调用保存。不含工具编译/输入哈希等未分相位开销，不冒称全键验证 |
| whole-o时限整改（X2进行中） | `tools/release-build.cx` 当前完整44根、osx/win四目标统一whole-o | 旧70秒属于旧候选，C47已有25–32秒收据但不能迁移资格；将真实默认watchdog改60000，并在固定ROOT/PKG/CTOR/REV下四目标各新构造一次，不设override。保留全部根/资源/字节合同；另用短预算夹具验证超时清理，四片通过后撤AGENTS例外 |
| 增量骨架实测（BASE-6） | `tools/net-incremental-rehearsal.cx` prepare/snapshot/mutate/plan/build/validate/compare/report；正式批11/0 | 隔离副本只改pp/location-byte末REJECT，42摘要选3重建/39复用；与独立全量42网的JSON/TBL/NET实际memcmp及双SHA全同，45次全键实际rc0。构造阶段累计2774 vs33467ms；含身份开销build12067 vs175355ms，validate2573 vs21571ms。42分片实际比对后同冻结窗口聚合，小元数据重hash、大产物stat防变化，非跨run缓存；并发墙钟/端到端收益未测。research/c48-base-incremental/；不改产品输入 |
| 三网全状态全键验证（BASE-3） | `tools/lab-loader/net-check.cx TABLE NET`；research/c48-base-single-net/ | 原始shared-e2 1105532键/4286态、tokenpp 247682/961、warnpp-shared 1107338/4293，三次实际rc0；1548输入内容仍EXACT，三摘要与C0.2一致。可解码畸变网首差state0/key0 rc1，交叉配对rc2。比较声明全域next/seq、动作/字符串/维度，不证明表语言语义或执行动作副作用；BASE-6对改后模型另验 |
| 符号索引 | `sh ./unisacc.com tools/symindex.cx -- def NAME` | 冷建 ~2.4s，查询 ~250ms |
| `use` 的包根解析 | 仓根 `cxroot.tsv`（`R<TAB>PREFIX<TAB>DIR`）；工具侧 `use cxlib.cxroot` 调 `cxr_path`；`sh ./unisacc.com tools/check-cxroot.cx` | 旁边找不到时查最近祖先的 `cxroot.tsv`，最长前缀映射（cx/spec.md CX-U-7）；编译器侧已改 `plan.cx`/`cxmod.cx`，**已装产品要重建安装后才生效**，软链届时逐目录淘汰 |
| 源码所有权检查（CX-W-4 路线 B） | `sh ./unisacc.com tools/cx-own.cx -- FILE...` 或 `-- --tracked`；夹具 `tools/check-cx-own.cx` | 函数内局部 owner 的 DOUBLE-FREE / USE-AFTER-FREE / LEAK；保守（转给未知函数视作可能转出）。2026-11-03 全仓：9043 函数，LEAK 220（抽 10 全真，多为工具错误路径），DOUBLE-FREE 1（误报，互斥整数条件），产品目录只有 60 个局部 owner——产品所有权多在结构体字段与包装分配器里，函数内文本分析覆盖不到 |
| 批量机械改写 | `refactor.cx -- find/apply/sweep` | 每文件改前改后验证，不过即还原 |
| C47 whole函数来源 | `tools/whole-function-origin.cx -- MATERIAL UNITS FRAMES CONFIG WATCH OUTPUT NEWDIR [--sidecar]`；`tools/module-byte-map.cx -- --whole-context …` | 校验normalized帧地图及同位置快照；后改写、区间外与跨上下文保持UNKNOWN，stub单列SYNTHETIC；非排他所有权 |
| C47双段观察消费 | `lower-row-origin.cx`、`encoder-record-origin.cx`、`arm-encoder-record-origin.cx` 的显式 `--watch-bundle`；共享 `cxlib.watchbundle`，登记 `register.cx … --args '分号分隔参数'` | 两段最多2M条/128MiB，旧单文件预算不变；manifest/execution/每段SHA及连续序号全核，新证书绑定实际manifest；有界拼接，非恒定内存 |
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
| 仓级棘轮（13 项） | `sh ./unisacc.com tools/ratchets.cx` | cxraw/cxinclude/cxapis/cxtrunc/cxlibc/cxplatform/srcshape/coreapi/verwriter/aluiwidth/outdep/check-literal/net-deps；toolsloc退役，新增net-deps沿计划保留第14号门称呼；旧通过记录不迁移 |
| tools 统计与候选清理 | `sh ./unisacc.com tools/tool-inventory.cx -- --recent 10` | 总行数、按目录/文件及近N次提交增量；无调用者/无批次/疑似重复只报告；不设总量或逐文件上限。tests/toolsloc.tsv退役保留；precommit只对新增tool缺check或批次登记拦截 |
| osx X1旧项复核（2026-11-08） | `tests/driver-target-object.cx ROOT cli`：9/0；平台宏最小源见`tests/cases/driver-default-platform.cx` | Darwin arm64 26.5.1、产品SHA f069d4c92b7f5ede508809aa3abd21e326cbecadb2fb9eccf0bd1ac7004c3e82。默认-run原生、裸-c产宿主Mach-O对象并由产品链接执行，NATIVE-RUN-HOST-TARGET核销；不承诺系统链接器兼容。裸-S仍用lnx/x86_64预定义：平台源bare/显式lnx tape SHA同为d7bb734dae19…，显式osx为e02cac44f622…；DEFAULT-TARGET-S-C保留READY，只剩-S语义裁定/修复，不重复-c修复，也不迁移六宿主CI资格 |
| 真实提交工具配套与源码分类闸 | `sh ./unisacc.com tools/check-precommit-toolsloc.cx -- --loc`、`-- --source`、`-- --symlink`（保留三批历史ID） | 独立真实临时git仓运行生产hook；历史行数超额允许，shebang伪源码拒绝，新增tool缺check或缺IMPLEMENTED批次拒绝；旧工具无调用者/无批次/重复仅列候选 |
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
- 其余见下表「包层」。

**包层（2026-11-08 实测，`git ls-files` 行数）**：

| 层 | 目录 | 文件/行 | 在源 SID 内 | 说明 |
|---|---|---|---|---|
| L0 | `cx/sys` `cx/mem` `cx/text` | 27/3544、6/521、7/837 | 是 | 对接 libc/POSIX 的唯一层 |
| L1 | `cx/core` `cx/driver` `cx/runtime` `cx/compiler` | 11/719、14/2389、89/6521、77/8234 | 是 | 执行器、驱动、运行库、编译器模块 |
| 构建图 | `cx/cx0` `cx/workspace` | 7/1594、6/220 | 是 | 解析与包图；`cx/workspace/packages.tsv`（CXWORK1：P/I/C/D/T/G 记录）定义 6 个构建包与依赖边 |
| EXT | `ext/sysx` `ext/memx` `ext/textx` `ext/crypto` | 13/1271、5/488、5/610、15/3432 | **否** | 上层扩展包；`use sysx.*`/`crypto.*` 经根目录 `cxroot.tsv`（CX-U-7）按前缀映射到目录 |
| 工具库 | `tools/cxlib` | — | 部分 | `cxroot.tsv` 也映射 `cxlib`；仅 10 个工具支撑文件进源 SID |

**已核事实**：`ext/` 不在 `seed/ident.c` 的目录集内（`exec src kernel include weights seed cx` + `iterate/kernel/typekw.tsv` + `tools/` 10 文件，共 1586 文件）；`cxroot.tsv` 只映射 `sysx`、`crypto`，**未映射 `memx`、`textx`**（其解析方式未核）。**使用者（`git grep` 实测，除自身目录、archive、plans、research）**：`sysx`、`crypto` 有仓内引用（测试、工具、技能示例）；**`memx`、`textx` 仓内零引用**（不在 `cxroot.tsv`），疑为未接线或待删，须裁定。**未核/待办**：EXT 包的变更由哪个身份覆盖（PC/GEN/收据）、是否随 APE 发布、`memx/textx` 与 `cxroot.tsv` 的关系；内部包接口摘要与依赖设计见 `plans/v0.0.48-unisacc.md` §12–§13（C0 分治身份）。
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
| 棘轮 | 13项，以`tools/ratchets.cx`末行为准；新增第14号net-deps，toolsloc已退役；工具行数只统计 | `tools/ratchets.cx`、§2.2 |
| 事实表 | 事实→生成器（须带 `--check`）→产物 | `exec/facts/`、§2.4 |
| 静态预检 | 模式/引用/兜底/SB 合同先查；动态及未证明性质单列 PARTIAL，不能替代真实 gen/新包/slice | `tools/tsv-lint.cx`、`tools/chain-precheck.cx` |
| 结构盘点与普查 | 源码/模型分母与六片模块账全部实核守恒；最终模式全部登记实跑；C47已DONE，行数门按主人新裁定退役，不称全局棘轮或CI绿 | §9.6、`tools/module-byte-map.cx`、[六片账本与材料身份](research/c47-logic-distribution-20261108/)、[plans/slice-shrink.md](plans/slice-shrink.md) |
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

### 9.6 0.0.47 逻辑分布普查结论（2026-11-08，本机实测）

产品身份仍为 `f069d4c92b7f5ede508809aa3abd21e326cbecadb2fb9eccf0bd1ac7004c3e82`，5,818,178 B、0.0.47、NOT_RELEASE。driver 四个正式批次20/0，六目标最小镜像格式、裸-c、链接并执行23、默认-run23均有实际证据；本次统计工具变化未重建或刷新产品。公开版本同名不迁移CI资格。

源码测量冻结于 `ce710d583`（合入远程 net-deps 后；随后 `99e24cd8a` 仅 PRD 变化）。`logic-distribution --sources` 重新实跑：2320个已跟踪普通源码，根为src/cx/exec/seed/weights/tools，总14,096,270 B。表5,771,926 B（40.9464%）、生成器源码1,450,565 B（10.2904%）、经典机制源码6,873,779 B（48.7631%）；该口径的已跟踪网络表示源文件为0，不等于产品没有网络。工具也在此分母，不能把源码字节或branch token冒充语义决策占比。

当前产品包另实跑40个kind N模型，raw总4,833,789 B：生成动作程序2,886,663 B（59.7184%）、选择器1,757,294 B（36.3544%）、常量189,035 B（3.9107%）、头797 B；精确守恒。压缩模型1,315,916 B、资源241,070 B、元数据108,280 B合为包1,665,266 B，未按raw比例虚摊压缩段。源码/模型/压缩包是三个分母，不相加；生成器源码也不等于生成动作程序。

六片逐模块明细和SHA链见[普查结果](research/c47-logic-distribution-20261108/summary.md)。下表为字节；MODULE是发射上下文来源，非排他数据所有权；各片SHARED均为0，只代表本次没有核到多源融合区间。

| 目标 | 解压映像 | MODULE | UNKNOWN代码 | SYNTHETIC | HEADER | DATA/ALIGN | RESOURCE |
|---|---:|---:|---:|---:|---:|---:|---:|
| lnx/x86_64 | 1880862 | 1803567 | 36795 | 411 | 176 | 39913 | 0 |
| lnx/arm64 | 1889006 | 1793808 | 52860 | 1324 | 176 | 40838 | 0 |
| osx/x86_64 | 2410978 | 2177228 | 148248 | 6941 | 1020 | 77541 | 0 |
| osx/arm64 | 3071458 | 2811856 | 169396 | 10160 | 1020 | 79026 | 0 |
| win/x86_64 外PE | 2664960 | 2379225 | 235958 | 0 | 2048 | 47217 | 512 |
| win/arm64 | 3369472 | 3074840 | 246224 | 0 | 1024 | 47384 | 0 |

六个mapper均真实exit0；独立默认-run探针逐行重算、核原映像SHA也exit0。Win x64先核bare 2663424 B，再验证正文整体平移1024 B、HEADER增加1024 B及版本资源512 B，得到外PE 2664960 B；整个APE余尾不算该切片。Mach83单元中的forward stub单列SYNTHETIC。后改写、函数区间外和未覆盖lower规则保留UNKNOWN，不把守恒等同100%归因。

D1数据结论：模型大半raw字节是生成动作程序，选择器只是其中一部分；这些表示计数没有证明“以网消除逻辑”。机制代码、跨目标合同与独立于参考C的语义oracle仍须分别验证；本次逐字节对拍和来源一致性不是独立语义oracle。whole观察的快核目前依赖out私有HOSTCC/ASM诊断原型及实际execution身份，不声称已迁入共享sim或形成可移植产品能力。

全部新增工具及最终参数模式均已正式登记实跑，包含8个encoder双段profile；静态和动态错配拒绝保留。C47最终测量585文件83,279行；2026-11-08主人最终裁定工具行数门退役，C47据已有功能证据DONE。先前13项实测9绿、cxapis/cxtrunc/outdep红及coreapi ERROR仍保留，统计制度改变不使其余门变绿。局部功能和登记通过不称全仓棘轮/--all/六宿主CI通过；首次超时、静态profile拒绝与未知状态均保留。

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

〔C47-LOGIC-DISTRIBUTION 事件MVP冻结验证〕事件增加 `dst_lo/dst_hi`，OFILL取真实保留位置与宽度（非零起点1、同长覆盖[1,2)），追加取旧尾至新尾，截断/清空/丢尾/SWAP取被移除范围；文本key固定为本次edge选择前的观测值，不随同seq后续PUSH变化。帧实例递增、内容SHA缓存只用于诊断。最终已装产品f069d4c9…默认-run检查 `out/sim-write-events/check-final-bound.log`：SIM_WRITE_EVENTS_CHECK checks=14 failed=0，rc0；OFILL范围断言限制在OFILL同一事件行，防SPAN相同区间误绿。新批 `sim-write-events` 已写注册与覆盖行；14/0为直接检查证据，注册表全批执行由父集成，尚不借此宣称最终六片模块分账完成。无工具进程在途。

〔C47 两项同候选批次收据〕已装f069d4c9…下，driver-target-object-lnx/osx/win/cli、migrate-record-json、logic-distribution、sim-write-events七批真实run-batch均PASS/rc0/transport0，断言分别5/3/3/9/8/6/14。首轮父未先建ABS_OUT_PARENT，七次均canonical-root/output-parent errno2拒绝、未执行批次，原*.batch.log保留；创建目录后新*.registered.log绑定真实执行，未复用PASS。①回退修复的本机目标已验；②最终机器码模块分账仍UNKNOWN，事件MVP不是来源证书。tools行数扩额尚未收到答复，基线保持原值；不把七批局部通过称全量资格。

〔C47-LOGIC-DISTRIBUTION 合并边界下一步〕先复用产品compiler.tapelink的真实tl_unit emitter，保存有序对象SHA、各次调用的合并输出区间及合成__init范围；标记emitter-context而非最终机器码所有权，共享全局去重不凭首个emit调用归独占。新工具tape-origin与check只取独立对象（archives显式拒绝），保持名字改写/重复定义/目标不一致/缺入口/undefined等真实拒绝。后续须与编码器blob/记录传播补边组合；不把本层区间当六片最终模块账。

〔C47-LOGIC-DISTRIBUTION 新口径：生成程序与选择器分开〕现有GENERATOR仅统计构造器源码，未度量生成出的有限程序；不能据其份额替代产品逻辑分布。新增只读普查将内嵌模型先经正式decoder完整验证，再以现成model_number定位UNINETB1四段：header、S常量、Q/C action程序、T/N state/selector余段；kindT决策表与kindN网络选择器分别报告。raw四段字节精确守恒，压缩成员只报整体stored，不按比例摊section；actions区分编码的显式动作与C前缀展开动作。该统计仍为表示体积/结构代理，语义决策比例UNKNOWN。

〔C47-LOGIC-DISTRIBUTION 生成程序普查实测，2026-11-08〕新增 `sh ./unisacc.com tools/model-program-census.cx -- --package unisacc.com`（也支持 `--model FILE`，`--arity PATH` 默认core-ops事实表）。使用正式package/model decoder完整验证，codec解压并核CRC；另以现成model_number仅定位二进制跨度，Q/C与decoded nq/qlen/qa核一致，opcode arity从 `exec/facts/core-ops.tsv` 读取。40成员均kindN（kindT=0），**这层分母仅为解压模型4,833,789B，不能与上面的13,760,638B源码分母合并**：header797B、S常量189,035B、Q/C生成动作程序2,886,663B、state/网络选择器1,757,294B，四段精确守恒。总states116,728、显式rows143,590、sequences79,038、动作显式/前缀展开均774,497；rows是编码行/阈值数，不是语义决策覆盖率或完整输入域。Q/C动作程序与N选择器是不同的产品组成，“生成器源码”不能代表“生成程序”，模型也不全是选择器。压缩stored只逐成员原样报告，绝不按raw比例分摊四段。

本次输入APE sha `f069d4c92b7f5ede508809aa3abd21e326cbecadb2fb9eccf0bd1ac7004c3e82`，arity事实表sha `7527789f1322b67e5f6b3fe66f64a994ae5e5b42ce24b5ea69779c5ce92ad458`，结果 `out/model-program-census-final.tsv` sha `2b445ea26f3629616dac245bdc9de24be36f453209512ca20770f5cf3a0797d3`，真实40成员命令rc0。独立最小UNINETB1 T/N、空S、Q/C前缀复用、截断/未知opcode/错prefix/错arity拒绝检查默认-run 9/0，证据 `out/model-program-census-check-final.log`；批次 `model-program-census` 注册实跑另列，不借直接check冒充注册门禁。此工具仅回答表示体积与结构数量，模块所有权与独立语义oracle仍未补齐。

model-program-census注册验收实跑终态：`out/model-program-census-register.log` 输出 `BATCH_RUN id=model-program-census status=PASS rc=0 marker=MODEL_PROGRAM_CENSUS_CHECK candidate=f069d4c9… scope=actual-one-batch-no-cached-result` 与 `REGISTER status=REGISTERED`；真实会话91311 rc0，无在途进程。仅本批通过，不表示整个普查/模块归因目标完成。

〔C47-LOGIC-DISTRIBUTION 合并来源账本实证〕`tools/tape-origin.cx --entry INVENTORY FROZEN_BUILDER OBJECT_DIR --out NEW_OUTDIR` 从已验证完整的slice-inventory与冻结entry-build编号重建真实链接顺序，不依赖收据完成顺序或文件名排序；复用compiler.tapelink，不复制链接算法。82对象（44根+38闭包）生成8,123,582B合并tape，SHA `d97a2288606b7012f13cf879420e41482ee4dcad814d2275d3beeb4f1ac7b1a3`；emitter上下文8,122,102B+合成初始化1,480B精确守恒。仅记planned_source，去重共享所有权仍UNPROVEN。输入收据复制到origin-material，P3路径仅重绑定到相同fde2db18…字节备份，原始收据未改；不是新构造证据。真实已装f069d4c9…重放该tape所得ELF SHA `2343d81333db0359fbba3336b157785b1485c0d0c798be97d62db466d42c9818` 与本轮lnx/x86_64/entry.elf cmp0，证明合并路径一致，不证明最终模块边界。

新增entry-order四项后检查与正式批次均实际16/0，最终批次日志 `out/c47-driver-fix/tape-origin-final.registered.log` 为PASS/rc0/transport0，绑定f069d4c9…；覆盖反序inventory、重复builder key与未验证inventory拒绝，保留原12项真实对象/同名局部/共享BSS及拒绝断言。最终元数据版 `origin-lnxx64-v2/merged.tape` 与前版cmp0；账本在同目录origin.tsv。能力速查：`sh ./unisacc.com tools/tape-origin.cx -- NEW_OUTDIR ORDERED_OBJECT...` 或上述--entry接口；`sh ./unisacc.com tools/model-program-census.cx -- --package unisacc.com` 分开报告生成动作与选择器。六片最终机器码模块归因仍未完成；下一步补blob创建/使用、缓冲代次与属性读写版本，连接编码器重排至最终文件。

〔C47普查同候选更新与D1结论〕已装f069d4c9…的六片/包普查实际rc0，`out/c47-driver-fix/current-candidate-slices.tsv`：lnx/x86_64=1,880,862B，lnx/arm64=1,889,006B，osx/x86_64=2,410,978B，osx/arm64=3,071,458B，win/x86_64外层PE=2,664,960B（含APE stub），独立win/arm64=3,369,472B。包内模型/资源/元数据仍1,315,916/241,070/108,280B；不得用旧fde2db18…两片Linux尺寸或旧win/arm64身份替代本轮。就解压模型分母而言，动作程序59.7184%、选择器36.3544%、常量3.9107%，只说明存储表示；生成器源码、已生成程序、表与网须分层记账，存在信息重叠。D1不能由这些份额声称逻辑消失或语义正确：经典机制仍需独立oracle与跨目标合同，第一项已补六目标格式回归，而完整同候选原生资格与最终模块贡献另需证据。

〔C47编码来源blob边实证〕显式write-events新增EDGE：OCUT/BLOBSAVE/SBSAVE记录新blob身份、内容SHA和源缓冲/代次/范围，INPUSH连接blob_id到独立frame实例，SWAP连接旧output代次到新frame，SBCLR递增scratch代次；相同内容的不同blob不靠SHA合并身份。所有EDGE与EVENT共用预算，仍仅ACCEPT后独占原子发布。独立小delta默认与开启日志均输出BQ，解析字段核BLOBSAVE的B子串、SBSAVE的Q、OCUT的BQ及INPUSH/SWAP连接；原14项断言保留，预算因新增两条真实边改为精确12/超界11，不放宽资源硬上限。直接检查20/0（真实会话7701退出0）与正式sim-write-events批次20/0/transport0（真实会话17974退出0）均绑定已装f069d4c9…；日志out/c47-driver-fix/blob-edges-check-final.log、blob-edges-final.registered.log。STX/LDX地址及最后写版本、SBBLOB等边仍缺，不能据本次称因果完整或最终模块已归属。

〔C47真实编码模型内容绑定〕冻结3b41cc224源的seed-gen `enc --elf` 实际生成JSON→seed-tbl→seed-net→现成nw_binary_net（不compactQ），64920B SHA `411198b71aa84eb74d48cdfdf5790181acb746fc24a7eee58e0e32f755b79be7` 与已装f069d4c9…包模型12解压内容逐字节相同，CRC/正式decoder通过；包route实际绑定lnx/x86_64/image/O0等elf阶段。前网络图 `out/c47-driver-fix/model-binding/elf.json` 可用于带状态名的真实编码诊断，不借旧out/model-com缓存。此小构造明确依赖宿主Apple clang21及系统头/链接器，不冒充完整context/PC/GEN或零宿主依赖；lower/model13与其他38模型未重造。报告out/c47-driver-fix/model-binding-report.txt（sha ac9793b5e367db3e5ebf13935f7f7ea9c11ee2d38ee881d344f138e246fdae15），全部真实会话已退出。下一增量在私有诊断补STX/LDX地址和值的最后写版本，避免BLB属性槽重用时追错旧blob。

〔C47属性版本及真实编码事件实证〕显式write-events的MEMORY行记录STX/LDX实际地址、值、frame/cursor与最后写版本；STX每次含零值都分配新版本并记previous_version，未写槽读取version0；诊断关闭时不构造版本map。独立delta先读未写槽，再B→Q→零覆盖同地址，LDX必须连接Q/零的实际最后写：默认/开启输出均Q。直接check及正式sim-write-events批次均25/0（会话68776、64531均exit0，日志memory-edges-check.log与memory-edges-final.registered.log），原20项保留。

冻结lower --full另已生成并绑定当前包model13 raw189100B SHA `8604703258355a176f850ec63ff55f4b78f43e09a2b1cf784e1b72e49e441b50`；产品-S真实main return7的tape经冻结lower得到minimal.target（24191B），当前带来源日志enc实际rc0产生8192B ELF SHA `ad5feb951792a7a8b4e2783061a5f2f9eb913bd82fc5fc5ec0bdd39e7d94643f`，40945个完整事件，日志out/c47-driver-fix/model-binding/minimal-current-events.tsv。手写ret.target曾因DEAD.reg拒绝，两次off/on rc1材料保留；换成真实lower输出有具体新输入理由。真实链仍待规则证书及合并单元到lower输出的来源证明，不认领模块字节。最新toolsloc实际红81046>80680，基线未改，扩额未获答复，不报全仓棘轮通过。

〔C47真实编码小例对照〕冻结sim的ELF、当前带事件sim的ELF、已装产品以同minimal.cx `--target lnx/x86_64 -o`写出的ELF三者实际cmp0，均8192B/SHA ad5feb95…；同源默认-run退出7是在Darwin宿主，未运行该Linux ELF，不冒称Linux原生证据。完整命令、终态与lower绑定见out/c47-driver-fix/lower-binding-report.txt（sha47757b8653b12ec5a72b654e7d1b57ac3319235d847b7b9c386321a0e1367c62）。三个相等证明日志未改变该例输出，模块归属仍需单独证明。

〔C47首个编码事件到最终字节的实证链〕只读核查固定事件2230的E.ret seq190=LDI195/OUTW→2231 OCUT/blob63→2233 BLB[68]版本437→24443 LDX同版本→24444 INPUSH/frame58→24445 COPY写重排text[336,337)→32745 LIB.imagebegin OCUT/text_blob995→32929 INPUSH/frame994→33266 EI.copy写file[512,513)，实读最终byte195，沿途无覆盖/截断失效。真实图被librarysymbols包装，OCUT状态是LIB.imagebegin，不能只据原始ELF.begin模板猜测。报告out/c47-driver-fix/ret-lineage-report.txt（sha b42a8bd09db3c85e0fa40668f4c7f3baf5336e603cb3bcd677e8f5ba329e700c）。这是特定编码事件到一个文件字节的链；输入ret完整行到事件尚缺INTERN/row_id显式边，原单元经prune/lower到target行也未证，模块仍UNKNOWN。下一步须补这两层并采用稀疏记录量真实切片，不能逐个手认字节冒充模块普查。

〔C47输入词身份下一增量〕私有诊断为INTERN/SBINTERN记录源帧/缓冲代次、精确半开子串范围、内容SHA及返回的intern身份；相同词在不同位置保留不同来源事件，不凭共享id合并模块归属。先以两行相同词和scratch复用的独立delta核位置与身份，再接真实图中的OPC属性读及E.ret规则；输入词所在完整行只能由同SHA不可变输入的行界核对，仍不把行位置直接当模块名。并行只读调查prune/lower实体来源合同及稀疏日志规模，暂不改产品。

〔C47 INTERN实际验证〕直接check29/0/rc0（会话59482），独立重复ret两行分别绑定[0,3)/[4,7)且scratch同id保持独立源事件；原25项保留。首轮29/1因新夹具留下ret输入使旧拒绝用例读到114而非65，修复为恢复其原始A输入，原失败日志intern-edges-check.log保留，不改拒绝期望。真实同输入/模型仅新增诊断，minimal-intern.elf与前版cmp0；44859事件/11405402B，INTERN在LW.i返回id59且精确子串[2224,2227)，内容SHA 85e4aea1…；并不靠cursor猜该词。注册批次尚待父本轮执行。

〔C47稀疏观察决定〕全写事件小例已约10MB，不能直接扩到8MB/439366行真实merged tape。新增仅私有sim的显式状态观察模式：配置列出实际状态与只读寄存器/指定地址槽，状态入口快照带frame SHA/实例/cursor、输出代次/长度及配置SHA；模式明确STATE_OBSERVATION_ONLY，未选择动作不称完整因果日志。现有完整write-events默认合同及1M事件/64MiB硬界保留。先用独立delta验证状态取值、坏配置拒绝与默认产物cmp，再为prune整行、lower实体/episode制定同模型内容SHA绑定的profile；不能按名字相似泛用证书。此模式是量测原语，来源解释另由固定规则合同检验。

〔C47 source/object身份缺口〕slice-inventory的VERIFIED只核字节/顺序计划，不独立证明源码等于对象。已实际用原冻结构造器fde2db18…、同snapshot cwd/source、原-O2 -c -b lnx/x86_64 -funit重编routestep.o，0.20s/rc0，输出SHA a5be9626…与候选对象cmp0。下一步用分批有界重放核82对象的原始优化参数及源码SHA；逐对象相等后才能把planned_source升级为该构造输入的实证来源，仍不替代模块到最终镜像的传播证明。不重建权重、不重跑全套测试。

〔C47 lower整行实体量测决定〕先以LINE入口前后输出长度及原输入完整行范围复原CL复制出的code缓冲，必须核复原字节SHA等于C.init所观测真实帧，不能靠词或标签相似。C.next/C.label将nc绑定该code帧完整行，再把GENERIC至C.advance的输出episode、PF/PL两行消耗分别绑定实体集合，其他规则/合成/数据仍UNKNOWN。实现独立消费检查，拒绝错input/delta/配置SHA、错帧、反序或重复实体以及不守恒复制；它只证明所登记规则的行来源，不把nc或intern身份当对象模块。当前真实小例watch已rc0/6264条，输入与原lower输出的cmp由消费校验接续。

〔C47稀疏观察实际交付〕`--watch-states CONFIG --write-events NEW.tsv`严格配置为STATE TAB REG,REG（当前只支持直接寄存器，不支持间接地址槽），最多64状态/每状态16不同寄存器/64KiB；未知状态/寄存器、重复/空字段及坏形均拒绝。仅选中状态入口记录frame实例/SHA/cursor/end、output/scratch代次和长度、指定寄存器值；关闭完整动作日志及属性版本map，header明确STATE_OBSERVATION_ONLY与config_sha，ACCEPT/预算/独占原子发布合同保留。新check13/0、原check29/0真实终态；sim-state-watch实际注册PASS/13/0/transport0，会话13804退出0。真实lower小例6,264条/1,632,524B，输出与原minimal.target cmp0；该规模不是全切片60秒资格。

冻结seed-gen prune→tbl→net另实际绑定包model8：raw23014B SHA33f2a9daec27e4f90d42781d7d01f1c76a975c1b9a04b18656b7e850591a98d7，完整memcmp=1/CRC通过；JSON sha048d415d42047197afddc2d1e1a3e8f80947efd753487e317680a58626134f48，可接真实整行prune量测，不信旧stamp。证据out/c47-driver-fix/model-binding/prune-binding.tsv，真实会话97920退出0。INTERN正式sim-write-events批亦29/0/transport0，会话96410退出0，前述“待执行”已由本收据闭合。

〔C47真实Linux切片诊断规模〕冻结8,123,582B/439366行merged经已绑定模型8→13→12的sim真实重放，各步rc0：prune2.83s、lower17.20s、enc17.93s；最终ELF SHA2343d813…与候选entry.elf完整cmp0。此轮不含全动作日志，未扩硬时限；prune输出逐字节等输入，不能冒称存在删行覆盖。来源观察下一步新增显式compact-v1：配置序号与数值列、每frame一次SHA/end声明，保留同样观测值、预算与限定claim，降低约89万状态观察的重复元数据，默认长格式不变；消费工具须同时验证帧表/序号/配置列，不可仅省掉身份字段。完整六片模块归因仍未达到。

〔C47冻结源码到82对象全部重放〕`tools/object-source-replay.cx` 将原构造器/receipt/inventory及各源SHA先记PREPARED seal，再在新输出目录按原目标/优化参数逐个实际重编并完整memcmp；每child8s、每批49s截止，不缓存PASS、不改原物。index0..81全覆盖：首批2，start2/18/34/50/66各16，六批真实exit0，82个全部BYTE_IDENTICAL；输入是冻结3b41cc224根和fde2db18…构造器，source-replay-seal.tsv只证明本次冻结，不冒充历史签名。源码/constructor编译后复核，receipt/inventory批末复核。独立真实最小源→对象→slice-inventory→seal→重编及篡改/路径/范围负控10/0，实际注册object-source-replay亦PASS/rc0/transport0，会话80794退出0。原首轮静态dup撞POSIX名、以及继承UNISA_CONTAINER拒绝均保留；后者修为子构造前明确清理override。可据本次确定性相等将该批对象与冻结源码关联，再与合并emitter上下文连接；共享数据独占性、优化传播和最终模块贡献仍不能从此直接推出。

能力速查：`sh ./unisacc.com tools/object-source-replay.cx -- --inventory INV --slice SLICE --sources NEW_SEAL --prepare` 只准备身份；随后同输入 `--sources SEAL --out NEWDIR --start N --count K` 实际重编（K<=32，父本轮用16），末行RECOMPILED_BYTE_IDENTICAL才是该批字节等价终态。证据out/c47-driver-fix/source-replay-{first-two-v2,batch-02,batch-18,batch-34,batch-50,batch-66}.log及register-object-source-replay.log。

〔C47紧凑观察与全片实体实际规模〕compact-v1独立check19/0、原完整check29/0，实际sim-state-watch批19/0；lower-row-origin长短统一解析检查21/0、实际注册PASS。真实全片前端观察27.20s/rc0，901415个FRAME+WATCHC记录、46,102,138B，未增加1M/64MiB硬上限，输出target与无观察full-lowered.target cmp0。全片实体消费已核完输入并开始输出，原逐行printf在20秒外限被杀，仅留下部分输出且无总结，**不能记全片消费PASS**；日志full-lower-rows.stderr保留。下一窄修将完整ENTITY输出按64KiB缓冲写出，保持全部行及原检查，不靠缩覆盖/summary模式规避规模。

〔C47完整实体账已生成〕64KiB缓冲输出后，原21项实际全绿（会话63129退出0）；同一完整901415记录的消费实际15.29s/rc0（会话69179退出0），生成48,248,191B完整ENTITY账，433975实体/7,956,487B code，code SHA d77c652bda755fc8761ceace35dc81c0864f9a8e00534c7d2c362a9326e215dc 与真实code帧相等，末行LOWER_ROW_ORIGIN rows=entities=433975闭合。未重跑原27秒观察、未删行、未改预算或验证合同；初次不完整full-lower-rows.tsv留存，正式新结果为full-lower-rows-buffered.tsv。独立消费工具接口 `tools/lower-row-origin.cx SOURCE DELTA CONFIG WATCH OUTPUT`，长/compact-v1共用SHA/帧/整行复制/nc检查；source-row-lineage-only仍不自动等于模块到最终字节。

本片prune另观察到FALLBACK在根frame1/cursor0、zero=0/size=8123582执行，输入/输出SHA都为d97a2288…；冻结图该规则明确OCLR→INPUSHXE(0,size)→SPAN2(0,size)→ACCEPT，因此本片是整帧身份传递，不是凭两份同字节猜内部行重排。实际日志full-prune-fallback-watch.tsv（1观察记录）及输出cmp0，4.65s/rc0；其它输入的prune删行路径仍须单独证。

〔C47 lower/encoder区间连接登记〕完整Linux lower episode真实消费20.08s/rc0：371917条已闭合GENERIC/PF/PL episode，1076644个target字节仍UNKNOWN，不能按最后模块兜底。独立check保留全部30断言，原合跑55s超时未记通过，拆--rows 21/0与--episodes 9/0实际通过。编码器新增encoder-record-origin/check，限定已内容绑定的model12静态Linux x86_64，将LW完整target行与WR.nx累计输出接成独占文件字节区间；最小实际1155记录/text4878B/image8192B，独立11/0。下一工具module-byte-map/check只连接经核SHA的origin→ROW→EPISODE→RECORD，模块贡献是发射上下文代码来源；多模块融合单记SHARED、合成初始化单列、其余lower规则/头/数据/对齐保持UNKNOWN。不能以lnx/x86_64证书外推其它五目标，六片仍DOING。

编码消费者全片首轮2351被50s外限终止（exit142），虽校验阶段已完成，输出只到q333668且无总结，full-encoder-origin.tsv不是PASS。与前序lower相同，setvbuf不足以降低运行时逐printf开销；下一窄修只改显式64KiB拼接/fwrite，所有RECORD保留，原超时材料保留，新文件full-encoder-origin-buffered.tsv再核。producer完整759328观察已23.98s/rc0，输出image与原候选相等；不能把producer成功替代consumer成功。

〔C47区间链消费闭合实证〕编码器显式缓冲窄修后，独立check11/0（21431退出0），完整consumer54329在原50s限内退出0；out/c47-driver-fix/full-encoder-origin-buffered.tsv SHA b74c57265d5652bf41edb549d0a25509747e756fc34fc8d8a18e23f9b606dd64，378536条RECORD、text1840773B，header[0,176)、code[176,1840949)、DATA_OR_ALIGN[1840949,1880862)，image SHA2343d813…与本轮候选一致。绑定target cf021fbf…/watch a1bcb30…，不能由同一SHA替代事务闭合校验。lower-row-origin注册批21/0、lower-row-episodes注册9/0、encoder-record-origin注册11/0均本机实际PASS/rc0/transport0，日志out/c47-driver-fix/{lower-row-origin-episodes-final.registered,lower-row-episodes-final.registered,encoder-record-origin.registered}.log；全部同候选f069d4c9…，不等于全仓门禁。下游module-byte-map尚在独立实现/核验，当前不报模块覆盖率，更不报六片完成。

〔C47其余目标来源前置〕lnx/arm64同为82对象entry-build路线，但现有slice-inventory/object-source-replay硬限lnx/x86_64，不能借该目标PASS贴到arm64。下一步在现有工具明确接收并绑定两个Linux目标，默认行为不变；receipt目标参与seal身份且重放命令使用该目标，source-plan仍经实际对象重编核。osx/win原whole-o路线需从真实单位发射阶段补源实体，不将另编独立对象的不同优化/链接结果冒充原片。六片分账目标不缩为两片。

〔C47五目标模型内容前置〕只读合同已定位真实路由：lnxARM lower/enc=9/2、osxARM17/16、osxx8621/20、winARM25/24、winx8629/28；ARM编码为两遍扫描且lnum未重置，不能沿用WR.nx证书。现先从冻结snapshot以既有native seed-gen/tbl/net重造这10模型，逐个与已装包raw完整memcmp/CRC核一致后才用其JSON制定观察；仍明确宿主工具依赖，不称产品重建或独立oracle。各相位内层有界，新路径在model-binding各目标子目录，所有原产物保留。whole-o sourceframe到tape传播尚待单独补证。

〔C47首片模块字节表实际接通〕module-byte-map/check已以SHA链连接真实origin→433975 ENTITY→371917 lower EPISODE/UNKNOWN→378536编码RECORD→候选image；独立一致性fixture8/0含2模块、融合SHARED单计、合成、UNKNOWN/zero、label/metadata空隙及错hash/重叠/ENTITY/count/缺尾拒绝。真实v3退出0，lnx/x86_64的1880862B分为82模块上下文代码1803567B、未覆盖代码36795B、合成初始化411B、HEADER176B、DATA_OR_ALIGN39913B，加总精确相等；SHARED0B只是本片命中情况，不证明融合永不同源。原v1把RECORD target跨度误要求连续的拒绝保留，最终仅要求target单调且file/text严格连续，因label/@sym行本来无编码RECORD。planned_source发射上下文贡献在82对象实际重编证据支持下可追冻结输入，仍不宣称共享数据独占。真实证据out/c47-driver-fix/module-byte-map-real-v3.log，当前仅首片，六片要求未完成。

已完成其余五目标10模型的gen/tbl/net→正式wire/decoder→包raw核对，各child10s、整个阶段55s内终态0；10个bytes_equal=1，模型索引与真实路由一致。为避免只留out临时材料，本轮把首片完整模块摘要及10模型身份表存research/c47-logic-distribution-20261108/，包含全部输入链SHA；这只是来源/体积证据，不是六片或语义验收。

module-byte-map正式注册实际PASS/8/0/transport0，证据out/c47-driver-fix/module-byte-map-final.registered.log；首次注册父写错marker MODULE_BYTE_CHECK，虽check8/0但门禁正确判FAIL并撤回两行，日志保留，未据此记通过。实际marker为MODULE_BYTE_MAP_CHECK，最终新调用重新执行而非复用缓存。首片贡献前五为asmtext223070B、cxmod119477B、source_frame92079B、host90856B、tapegate77950B；完整82模块与链身份见research/c47-logic-distribution-20261108/lnx-x86_64-module-bytes.tsv，其它五目标内容绑定表见同目录five-target-model-bindings.tsv。

〔C47 ARM编码事务实现登记〕新增arm-encoder-record-origin/check（名称全git历史查空），先只做已绑定lnx/arm64 model2静态ELF。依真实pass1完整行及OP.end输出事务建区间，pass0/REWIND OCUT严格隔离，lnum不假定重置；布局176B/EM_AARCH64核实际ELF。先独立fixture和真实最小target，再接全片；不是把x86 WR.nx改机器号。其它ARM格式与whole-o来源仍另需合同。

〔C47 ARM来源前置实证与whole-o捕获决定〕Linux ARM实际inventory=82/82、44roots+38closure、VERIFIED/COMPLETE；合并tape8122531B SHA4f565b8b…，prune根FALLBACK完整帧传递输出同SHA，未借x86材料替代。首次重编调用给相对out，child冻结cwd下正确拒cannot open output，终态2保留；使用绝对新目录重跑前32对象已实memcmp相等，不改工具放宽。另实比snapshot与原release-snap-3b41cc224f的src/cx/exec/include/seed共1890文件零差异。下一仅重放win/x86_64原whole-o诊断构造，保留真实绝对source路径，捕获原holdframe/资源，并与原bare片memcmp；各步<=50s，不修改产品或发布资产。RF_CAPTURE不自带stage全文，后续仍需实际阶段重放，不能以捕获目录存在认完成。

〔C47 x86其余格式编码证书登记〕已有encoder-record-origin/check保留Linux固定profile，下一增显式osx/x86_64与win/x86_64 profile，只接受各自已绑定delta与确切配置。共用LW/WRITE/WR.nx事务，分别核Mach-O1020B文本复制及二次整文件复制、PE1024B文本复制及同名copy的重定位用途；不得只替换header机器号。先独立格式fixture与产品真实最小链，完整whole-o来源仍另证。

〔C47两个Linux源码重编链实证〕arm64完整82对象六批0/16/32/48/64各16、80起2均真实exit0和BYTE_IDENTICAL；源seal/receipt/构造器批前后复核，原合并内容身份4f565b8b…不变。lower-front观察与433924实体消费也真实exit0，full-front.target现已生成，最终ARM编码镜像仍未闭合，不能提前记第二片贡献。slice-inventory/check泛化两个Linux目标，拒冲突/重复/非Linux，replay核inventory与receipt目标并实际传-b；seal-v1由receiptSHA绑定目标继续兼容。独立最小ARM和x64重编链各13/0；inventory原无参超55s，不删断言，拆--part1/2/3/4分别18/5/4/5，32全部实际通过，注册同样拆分，旧超时保留。

whole-o首轮捕获在编译前因默认UNISA_JOBS=8被资源严格合同拒绝（rc2，非超时）；原stderr保留。下一明确UNISA_JOBS=1并用已有PHASE A/B断点把单位准备与最终输出分开，各自50s；不提高时限、不把A成功当镜像成功。ARM编码窄profile以START及两pass OP.end取事务，省掉每字节/行重复LINE观察而保留全部指令记录；标签/元数据/空行无代码追加的源规则需消费检查核，1M事件硬限不变。

〔C47复制边观察的窄扩展决定〕Mach-O/PE纯COPY循环若逐字节观察，整片必超既有1M记录；新增watch配置可选第三列frame-edges，仅对明确登记状态在当前frame cursor=0或end时记录，默认两列观察不变。配置SHA绑定筛选策略，header仍STATE_OBSERVATION_ONLY，消费证书必须验证首/尾frame SHA/end及cursor/output变化，再以已内容绑定且固定COPY/ADV无分支改写的规则核完整复制；不以省掉动作审查换低记录数。profile未登记的状态不受此筛选。保留1M/64MiB硬限，扩展独立正负控后才用于整片。

正式inventory mapping批53515/95213并发中，target部分4/0通过，但mapping注册外55s终止，未记PASS（直接part4的5/0不冒充注册通过）。进一步按拒绝阶段拆原part4为unsafe/dupkey两项、claimed/missing两项、root外一项，各自保持原真实fixture和断言；旧part1/2/3不变，合计仍32，不加时限。

〔C47第二片区间与覆盖扩展决定〕ARM完整producer761808条观察、consumer379288条RECORD均真实exit0，最终1889006B ELF与原ARM候选entry.elf cmp0。当前已核GENERIC/PF/PL的模块账亦守恒，但代码UNKNOWN132612B（AF/AI等尚未纳入），不能用x86较高覆盖率替代。下一保留旧profile/schema，另增已审AF消费4、AI消费2的显式profile与每实体source字段；mapper按规则确切count逐一核nc及所有source0..3，跨模块只记SHARED一次，AI lookahead不计消费。独立正负控后再新观察ARM完整发射，禁止把原UNKNOWN直接按相邻模块填账。

〔C47 PE真实路径不能缩为static〕已装产品Win最小输入的host_dyn实际为1，现static消费者正确拒绝，不能换不含运行库的静态玩具当当前Win全片证据。新增明确forward profile将分别核PE forward头/导入布局与观察分支，文本仍需证明PE.text→PE.copy完整且逐q闭合；额外合成代码若存在单列，绝不贴给最近q。Mach-O最小1155 records/text4909/image33250已实际consumer通过，完整whole-o来源仍独立未闭合。

object-source-replay两目标的直接13/0仍有效，但注册并发三批时两份replay均在55s外限终止，不能把直接检查替代该两份注册PASS。下一保留默认13项，新增--part1/2/3，每份都实际准备源/对象/inventory/seal/重编，再分别覆盖目标错配、对象/构造器/新目录、源码/路径/范围三组；唯一断言集合不减，只重复必要准备以限制单批。不放宽55s。

frame-edges最初登记helper没有入口args参数，实际运行了默认19项，不能记新增policy通过；已把该batch args明确更正--frame-edges，正式新调用须核11项。新增独立edge11/0、原watch19/0、完整事件29/0均实际exit0；原edge夹具用非法'*'byte键的9项失败保留，换为真实97/98/99字节键后通过，不改生产拒绝逻辑。AF/AI新check7/0、mapper3/0与旧21/9/8亦均实际通过，注册以独立新批覆盖。

外格式最终独立检查Linux11/0、MachO5/0、PE static5/0、PE forward9/0均实际exit0；真实PE forward1479记录/text7892/image11776、imports[9216,10752)，consumer两次真实终态0。新profile配置SHA与首尾COPY帧内容绑定，两OTHER保持完整非text尾守恒桶，ID附已核imports区间。sim-edge和mapper-ARM独立check的marker与默认模式不同，首注册虽rc0仍按不匹配marker正确判FAIL；metadata改用SIM_STATE_WATCH_EDGES_CHECK与MODULE_BYTE_MAP_ARM_CHECK后重新执行，不以错门禁回执记通过。

〔C47两片模块最终本地账〕ARM新AF/AI完整观察774105记录、target与原front cmp0；消费372588 episode实际退出0，最后mapper亦退出0：1889006B=82模块上下文代码1793808B+未覆盖代码52860B+合成1324B+HEADER176B+DATA_OR_ALIGN40838B，SHARED0。新账research/c47-logic-distribution-20261108/lnx-arm64-module-bytes.tsv保留完整SHA链；旧未含AF/AI的132612B UNKNOWN账保留在out，不拿其覆盖新证据。Linux两片分别守恒且与本轮候选镜像cmp0；仍不是其它四片完成，亦不证明共享全局数据的排他所有权。

Win/x86_64正常whole-o原44roots路线实际重放成功，bare2663424B SHA76248e1fdcb152e260b9bb530179bb44295530ad448ee1c050475bdf720b2fec，与原片完整cmp0；原holdframe37029416B/82源帧/742B属性，SHA6aa39be72d1c0f8cf0c5183edc387e9df06b0a9d6cd5dd8b9c209c5e9a2e12b4。分段PHASE B确实在__ccw_0处lower拒绝，未冒充原片证据；存在固定临时forward源码名且多agent共享TMPDIR，后续捕获应先隔离TMPDIR再核，不先断言根因。旧RF_CAPTURE e3 companion的SHA字段空白不能作有效输入身份，后续用独立内容hash和最终image cmp补证，绝不把空身份填成PASS。

下一正常Win重放将TMPDIR设为本任务独占目录，保持原构造器/容器/44输入和正常whole-o路线，不用PHASE B、不启RF_CAPTURE；同时留存hf原帧及真实forward stub。须再次最终image cmp原片，才能把隔离捕获材料加入来源链；不会用不同输入产物冒充原片。

〔C47 whole-o关键分支已实测收窄〕正常重放的独占TMPDIR下image仍cmp原Win片0，且无forward stub文件；实际product stream units/e3/sidecar已全部退出0，USLFW1为hostslot0/ccw46。按dv_run_lower的cnt0真实分支可直接剥tape→prune/image，不需要第二e3；前述假设stub的计划被实证撤回。原PHASE A在cnt0也可能完成整片，不得只按环境名认“停在e3”。e3-first.tape13472921B/787224行，SHA cb9860332f975e901a6cbfb5da2f53db788e6bb421db502cbdb443561d61f583，82帧与742B属性保留。

模型4实际绑定的是parse2 --errors图（隐含locations），JSON SHA a985b1706b96659bd3951a44495e774d4eb3c969163845c7a63d9c225e6b1811、raw434447B SHA09ca74c5…，正式raw cmp=1；--locations-only的409740B不匹配图保留拒绝，不用它作来源。模型35 --locations已raw cmp=1。现source-package配方的shared-e3 --locations与本包事实不符，只记录缺口，不顺手修改构造链。

工具预算实测574文件82081行，对新合入cc基线81824多257；没有改基线，已异步提交具体本刀扩额问题。工具功能/来源验证继续，不把该棘轮红表述成全仓绿；另误选precommit自身夹具--loc整体55s超时保留，真正当前计量是tests/toolsloc.cx --check明确exit1。

〔C47 ARM其它格式证书决定〕保留现Linux ARM两pass/OP.end合同和默认11断言，新增显式osx/arm64与win/arm64-forward profile，各自绑定model16/24及新配置SHA；格式布局及完整复制边沿用已审x86格式事实，但机器码事务仍按ARM两pass验证。先独立格式正负控与真实最小target，完整whole-o分账未闭合前不记其它片完成。

〔C47 whole-o阶段链实证〕父真实product stream lower与elf均已终态exit0，win-stage-replayed.exe为2663424B，与正常whole-o原bare全字节cmp0；因此已捕获的82帧→units→model4首e3→cnt0 strip→prune→lower→elf链确为该原片材料。尚无逐函数来源证书，不能把镜像相等替代来源。下一对osx两架构及winARM各用同一原44绝对输入、冻结旧构造器/容器、独占TMPDIR正常whole-o捕获，各外限50秒，分别与自己的原片cmp；三路不共享输出或状态。

〔C47四片whole-o原材料闭合〕osx/x86_64、osx/arm64、win/arm64三份正常whole-o捕获会话62586/66780/24640均真实exit0，各自image与原候选slice cmp0；字节/镜像SHA分别2410978/42ce67ed…、3071458/e5f80fac…、3369472/fcfcc413…；原holdframe SHA分别6b80eb1a…、43ecf905…、1e9598aa…。首次WinARM cmp路径误指whole下不存在文件，保留拒绝，随后对正确win-arm64/slice-win-arm64实际cmp0；不把不存在路径当通过。所有材料为相同44原绝对输入及冻结构造器，四目标源帧各自捕获不互代。

〔C47 whole观察性能未闭合〕minimal真实e3输入198B上默认与-O2 sim均exit0，623B sidecar及940B完整12记录观察账逐字节相等；据此才单次完整-O2重试，仍在同50s退出142、无COMPLETE/sidecar0B，不记全片PASS。下一只做10秒私有诊断核新frame重复哈希是否热点，不凭猜测改性能或再完整重试；共享sim输入已移交写窗口。

〔C47 holdframe材料提取工具决定〕四目标whole-o已捕获，但原一次性Python提取有明确债，不继续复制算法。新增可复用.cx工具严格解析holdframe LE32 frn/atn、82源帧记录与原属性，验证路径/长度/重复单元/完整尾后输出USLATTRT units载体、attrs及逐帧SHA身份；仅准备材料，不代表功能或来源已证。配独立check并登记，原Python材料保留且新工具输出须cmp0。

〔C47 ARM外格式实际门禁〕新MachO ARM独立4/0、PE ARM forward5/0，原Linux11/0保留；真实最小producer/consumer均exit0（1349records/text6524/image33250及1765records/text9820/image13824）。两个新注册批最终均PASS/rc0/transport0/ARM_FOREIGN_CHECK，绑定现候选f069d4c9…。register无args，初次默认11只算注册准备，父改精确args/marker后正式新跑；另首次手动run-batch漏ROOT/OUT参数正确usage拒绝，已保留原日志，正确完整参数调用才记PASS。能力速查：arm-encoder-record-origin --profile osx/arm64|win/arm64-forward TARGET CONFIG WATCH IMAGE；配置SHA d64f933c…/edc2f841…。四目标whole-o材料身份已固化research/c47-logic-distribution-20261108/whole-o-material-identities.tsv，但函数来源/四片最终模块表仍未完成。

〔C47 holdframe原生材料能力已实跑〕新增whole-stage-material/check默认14独立断言，显式--real额外Win原Python载体cmp为15/0；实际工具Win提取82frames/82attrs且carrier/frames/attrs均cmp原0，正式注册批PASS/rc0/transport0。父对其余三片各自原holdframe调用也均exit0，全部只记PREPARED。能力速查：`sh ./unisacc.com tools/whole-stage-material.cx -- PHASE NEW_OUTDIR`，全校验后独占发布frames.bin/attrs.bin/units-input.carrier/material.tsv，ledger有完整SHA与原序路径，不覆盖旧目录。默认check不隐读out；旧Python提取债在这条新材料入口已替代，但不冒称全构造链Python清零。

〔C47函数来源下一窄合同〕采用已审输出快照方案作独立诊断原型：CT.afterfn output-mark保存同generation起点，FN.nx output-span对当时真实[A,B)哈希；最终bare tape同offset逐快照核，后改写者UNKNOWN、区间外UNKNOWN，不搜索平移，不称最后写者或排他所有权。单active mark、嵌套/无mark/未闭合/换代拒绝；配置SHA绑定新policy且新record显式区分旧WATCHC。先私有sim副本+独立check，HOSTCC在途观察继续冻结共享sim；原型通过后再明确集成。工具预算最终577文件82310行，比基线81824多486；先前异步请求441为快照前实测，新增快照运行时18行及跟踪后的新check27行合计再增45。当前toolsloc实际FAIL，尚无本刀扩额答复，不改基线。

〔C47目标分支继续收窄〕其余三片units/e3均真实exit0。Mach两架构sidecar各明确rc1报告5 hostslot，不能套Win cnt0路线；正常捕获的独占TMPDIR均留有2534B forwarding stub，后续需核真实增量merge来源。WinARM sidecar/prune/lower/elf均exit0，最终3369472B SHAfcfcc413…与原候选slice cmp0；这闭合第二条whole-o kernel材料链，不替代函数上下文账。共享sim尚f5810a66，新增snapshot只在私有原型验证中，尚未记生产能力。

〔C47输出快照诊断已集成〕私有原型首夹具参数错误rc1保留，最终14/0后父按原sim f5810a66与私有9c90c332双SHA断言、精确diff集成。共享源实跑旧watch19/0、完整事件29/0、frame-edges11/0均exit0，新snapshot正式注册批14/0/PASS/rc0/transport0；原1M/64MiB上限不变。第三列output-mark/output-span须各恰一状态，单active，记录OUTPUT_MARK/OUTPUT_SPAN与同generation/full-line[A,B)SHA；嵌套/无mark/未闭合/换代/换帧/边界/预算拒绝，后改写快照不等不冒充最终来源。能力速查：sim --watch-states CONFIG --watch-compact --write-events NEW；snapshot配置SHA与header snapshot_policy=single-active-output-v1显式绑定。当前只是观察器能力，还没全片producer/consumer，不报四片模块完成。

〔C47最后短采样诊断〕HOSTCC完整仍50s超时；一次15秒轻采样实际rc142，JSON解析281ms、712ms开始执行、约2.94秒完成UC前扫描、唯一39MB framehash141ms，至33M steps未再hash。3.17至14.905秒主要在RET/NX/IX/AU/DL.tokenunit/DL.offset路径且cursor持续前进、out0；排除了本样本JSON卡住/重复framehash说法，未泛化全程，不再无变更重试完整观察。另unit上下文前置报告已纠正：UC.finish可达SWAP，CT的hl_pos不能直接索引原units.payload；须取得实际normalized immutable frame并核其map/prefix才归属，否则UNKNOWN。

〔C47本轮材料里程碑及下一合同〕最后实质提交28249b986已pull --rebase/push，ls-remote与HEAD完全相等；工作树当时干净，本段仅补本轮最终事实。Mach两个sidecar实解5slots/ccw50，names为dlopen/dlsym/dlclose/dlerror/utimes；正常独占新TMPDIR的2534B stub仅可由dv_fwd_merge fast0回退写出，具体fast0拒绝原因仍OPEN。最小后继是只补stub e2/e1，按原生产splice插frame0之后并重标attrs成83帧，再正式units/e3 --res ccw且不含run-forward，捕获最终tape与原image对拍；无需重跑原82源前端。现尚无直接phase+stub splice CLI，不以设计当完成。六片机器码模块目标仍仅Linux两片闭合；本轮四片whole-o材料、两Win阶段镜像、ARM外格式最小证书、strict材料提取及snapshot观察能力均有真实局部证据，完整来源/跨平台CI不外推。
〔C47本轮决定与分工〕上轮有实质代码/证据进展，非阻塞等待。当前四片缺最终来源：一刀扩whole-stage-material严格83帧stub splice，复用原82帧；一刀给snapshot可选导出实际immutable frame，解决UC SWAP后hl_pos的来源边界；完整VM观察的5次不同诊断仍未过50s，符合两次自查未解/设计决策条件，另交自包含抽象简报做只读性能设计评审，禁止加时/减观察。三刀分域，父持有prd/注册/集成；不改产品模型或资产，原事件/属性/拒绝合同保留。
〔C47有界VM设计收窄〕只读强模型设计评审未跑实验，推荐诊断侧复用现fast integer核心，次选分块原生动作专门化。父实读可在私有run_arm64.S的Lrloop以x19=machine/w24=q/x23=steps作mask订阅hook，不改生产ASM；先out-only普通HOSTCC原型核模型解码/最小字节对拍/状态计数，再完整输入50秒一次，尚不视为命名全图映射已证。关键预算身份：生产ex_budget_now默认400000000000 steps，旧sim默认200000000；新引擎明确按生产预算执行并记录实际steps，绝不把旧模式默改后报绿。另从私有seed-tbl副本提取完整state/register名字，原表必须cmp已绑定tbl0，修正dl 1based reg和整数核0based reg的映射风险。所有构造仅PREPARED、HOSTCC依赖明确。
〔C47函数上下文消费决定〕下一可复用consumer将核material ledger/真实units地图→ACCEPT观察与导出的实际normalized frame→CT.afterfn入口unit_epoch/hl_pos前缀→mark/span快照→最终同offset tape SHA；仅同内容稳定上下文可归模块，改写/区间外UNKNOWN，stub独立SYNTHETIC，不凭标签猜归属。因UNITOK2经过UC SWAP，original与normalized两份地图/前缀须分别核并显式连接，禁止直接拿原offset。输出非连续CONTEXT与独立UNIT定义，后续mapper另扩显式格式，旧Linux连续UNIT协议不改。先独立fixture，真实全片另实核。
〔C47快核来源观察实际突破〕私有counter完整e3在生产4e11步预算下ACCEPT，382608138步/4.63秒，输出13472952B与生产sidecar cmp0。完整稀疏snapshot+frame observer随后真实9.97秒/exit0，11121事件、2780 mark/span对、1实际frame；output仍cmp0，导出frame与完整39268516B units.payload也cmp0，故本输入UC SWAP后内容相等由实际字节证实而非假设。minimal事件/manifest/frame与当前sim逐字节核等，动作wrapper保留SWAP/SBCLR/INPUSH/POP代次。数据仍out私有原型，Win4资源身份单列尚未header绑定；不是来源消费已绿。实际consumer接入rc2揭示新consumer把USLFW1 magic误写为NUL结尾，生产实际LF；由其owner修精确合同并补sidecar正负控，原拒绝保留。
〔C47 Mach最终镜像反证〕两片83帧splice、units/e3/prune/lower/elf均真实exit0，但stage-replayed.image对原slice均char217不同，未闭合最终identity。保留新材料作为失败对照，不把83帧守恒外推到原像；正在核生产fallback资源/阶段差异，不盲重跑whole。
〔C47函数消费与Mach窄修复〕真实whole-function消费已exit0：82单元、2780上下文全部survived=1，裸tape与原e3-first.tape cmp0，尾[13015177,13472921)保留UNKNOWN；sidecar LF magic正负控4/0、旧夹具7/0。Mach差异已收窄到stub独立stream漏生产RT_EXTERN：14项私有状态按实际D.align合计33320B，恰等两片BSS增量；补RT_EXTERN后stub e2/e1均exit0且声明变extern。下一仅用新stub接83帧后续链核原镜像，未闭合前仍记FAIL。另将私有快核参数化并测lower/encoder真实观察量，决定有界事件账设计；原1M/64MiB模式上限不改。
〔C47 Mach新资源链实证〕补RT_EXTERN后两Mach的splice/units/e3/prune/lower/elf链各步均exit0，最终image对各自原候选slice实际cmp0：x64 2410978B/42ce67ed…，ARM 3071458B/e5f80fac…。新产物分目录保留原失败对照，未重跑82源front。此证明阶段材料链身份，不替代未完成的函数与机器码模块来源证书。
〔C47显式双段账决定〕Win lower实际1598104选态、encoder选态3821476（含PE.copy字节循环，尚非有效事件数），原1M预算不足。后继采用独立watch-bundle-v1，最多两段、每段1M/64MiB、全部文件128MiB；旧单账预算不变，不自动迁移、不删观测。先共享严格容器reader与独立check；为了保持现有业务校验循环与不引入跨段游标错误，新模式首版在总128MiB硬界内重建规范compact视图，单次分配与峰值须实测，不能冒称流式常量内存。新证书watch_sha明确为manifest SHA；运行引擎/步预算/帧策略/资源身份保留独立execution.tsv并由manifest核SHA，不伪装旧日志。原frame/sequence/episode/两遍encoder业务状态一律贯穿双段。
〔C47双段真实规模与调度〕Win完整lower1598106事件/83.37MB、encoder1206286/66.15MB，两producer均exit0且输出cmp0；encoder原子发布窄修后独立复验2.91s、RSS470466560B，不称恒定内存。新reader默认14/0且两完整bundle均核SHA/序号/全尾exit0。12路consumer夹具与另4路lower夹具并发后，x64旧11项与ARM新5项在50s超时，均不记绿；其它10路已exit0。下一仅两失败作业改为受控少并发重核，不加时、不删断言，检查输入保持同SHA。
〔C47带参批次登记窄修〕现register.cx只登记无参数检查，迫使每个新带参模式先重复不相干默认检查再改行。为本刀双段/旧覆盖分批登记，补可选末尾--args（按现注册表分号编码）直接写入并实跑正确模式；原无参入口和事务回滚保持。独立私有表核两参数实传、空值/控制字符拒绝及表不变，不改原锁/并发负控。两失败夹具改少并发后分别旧11/0、新5/0终态exit0，原超时记录保留。
〔C47四片e3上下文闭合〕其余WinARM/Machx64/MachARM真实root观察及whole-function消费均exit0，前者82单元2780上下文、Mach各83单元2597上下文（含stub）；全部快照survived=1，e3输出各cmp对应产品0。资源capture残缺回执保留，实际文件与pack/生产st_pub覆盖逐项核后才观察；命名图未选部分仍不声称独立同构/语义oracle。Win x64完整ROW消费已781127实体/exit0、encoder消费603136记录/exit0；其它三片lower/encoder观察与完整证书仍DOING。
〔C47 Mach encoder实值与合同分叉〕两Mach完整观察的WRITE/LIB.imagebegin实际host_dyn=1，原最小静态profile限定0所以消费明确rc2，拒绝有效保留。新增独立osx/x86_64-forward与osx/arm64-forward profile，仅该显式模式要求host_dyn=1；原osx静态profile仍要求0，原PE forward元数据校验不泛用到Mach。模型/配置SHA与完整复制事务合同不变，另加静态/动态错配负控，不扩大旧profile接受域。Win x64 bare模块账已守恒2663424B；实际wrapper正文delta1024、头增长1024和版本资源512核后，outer PE模块账守恒2664960B，82模块及代码未知项逐行保持，未把整个APE尾算到该slice。
