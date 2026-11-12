# unisacc —— 产品与项目总览

本文回答五个问题：unisacc.com **是什么**、**有什么功能**、**结构组成**、**测试套件怎么设计**、**项目目录与文件结构**。术语歧义以 §0 为准；**结构点（层、片、流、网、模型、执行器、格式、事实表、构造链、测试）只在 §3 定义一次，其余各节只引用**；历史流水与逐版收口在 [archive/plans/2026-11-01/prd-c43-full-2026-11-01.md](archive/plans/2026-11-01/prd-c43-full-2026-11-01.md)（完整旧版），不在这里重复。

## 0. 概念与术语（先读；歧义以本节为准）

「流 / 网 / 段 / 阶段」四个词各只有一个意思。详细版与逐处核实见 [docs/glossary.md](docs/glossary.md)。

| 术语层 | 唯一名 | 含义 | 数量 | 真源 |
|---|---|---|---|---|
| T1 | **Stream**（别名 Flow） | 字节管道阶段：输入格式 -> 输出格式 | unisacc **7**（e2/e1/e3/e4/prune/lower/elf）+ 1（cx 侧 CX0，尚非模型流，§3.3、[prd_cx.md](prd_cx.md)） | `exec/pipeline/image-stages.tsv` |
| T2 | **Net** | ask 事实网：一张有限表 + 全键穷举 exact | **18**（另有 2 张声明表与 42 张构造网，口径见 §3.4） | `weights/gold/*.tsv`（权威；规则形式 `seed/gold_<阶段>_label.c`，由 `tools/gold-table.cx --check` 核对） |
| T3 | **Segment** | 训练合并段（多网合一的 masked 多任务模型） | 3（s1/s2/s3） | 负结果证据已归档 |
| T4 | **Book phase** | 一次推导中 ask 的记账阶段 | 由 contract 声明 | `flownet/contract.cx` |
| T5 | **Release phase** | 发布流程阶段 | 10 | 已归档 |

**三套「层」编号不要混**（同是「五层」「L 几」，指的不是一回事）：

| 编号 | 指什么 | 成员 | 权威定义 |
|---|---|---|---|
| 术语层 T1-T5 | 五个易混词各自的唯一含义（上表；glossary 原文写 L1-L5） | Stream / Net / Segment / Book phase / Release phase | 本节、[docs/glossary.md](docs/glossary.md) §3 |
| 结构层 L0-L4 | unisacc.com 这个产品由哪五部分组成（必须带名字写） | L0 loader / L1 base（六片）/ L2 program / L3 nets / L4 runtime | §3.1 |
| 模块层 cx-L0..cx-L3 | `.cx` 源码的 use 方向（AGENTS.md 与棘轮里裸写的「L0 才对接 libc」指这一套） | cx-L0 `cx/sys mem text`、cx-L1 `cx/core runtime`、cx-L2 `cx/driver compiler` 与 `src/`、cx-L3 `tools tests apps` | `tools/check-cx-layers.cx` 头注、§5 |

对应关系：术语层与结构层只在两处相交——T1 Stream 是结构 L3 nets 的执行次序（每流一组模型），T2 Net 是结构 L3 nets 的内容；T3-T5 与产品结构无关。模块层到结构层的对齐见 §5。

**两类 `.tsv` 不要混**：`exec/`、`weights/` 里的 `.tsv` 是**产品事实表**，由生成器构造成网（TSV -> NN），参与源 SID；`tests/`（51 个：批次表、覆盖表、棘轮基线、域矩阵、冻结/分档表）和 `agentwork/`（任务与账）里的 `.tsv` 是**管理表**，只被测试/流程工具按列读写，不构造成网、不进源 SID。读者按目录判断。

**六条常见误解**（不要按直觉读）：

1. 「7 流」与「18 网」不是同一批东西的两种叫法——流是**字节管道阶段**，网是**由 ask 事实表构造的 NN**（定义见 §0.1）。
2. `class Stage`（旧 `unisa/gold.py`）表示**网**，不是流。
3. 旧 `unisa/control/net.py` 的 `Net` 是**神经网络模型**，不是事实网。
4. m4 的「4 流」是**四个字节段**（lex/front/select/encode），不是 4 张决策网。
5. `phase` 三义：book phase（记账）/ release phase（发布）/ 历史 compiler phase（实为 segment）。
6. `STAGE_NETS` 只是 isel、abi 两张不在 `TABLES` 里的网（15+2+1=18）。

**禁用写法**：裸 `stage` 一律写 `stream-stage`（流）或 `net`（网）；「网」专指 §0.1 定义的构造 NN；包里的执行单元写 `model`（模型 ≠ 网，一个模型可含网+生成程序+常量）；旧 `unisa/control/net.py` 那类训练用神经网络写「训练模型」。

### 0.1 范式与核心概念（定义；全文以此为准）

**范式一句话**：用 NN 替代逻辑——编译器中的表状决策先写成**事实表**，再由生成器**确定性构造**成 NN，由一个通用执行器运行。「以网换逻辑」「NN 替代逻辑」「TSV -> NN」是同一命题的三种说法，不存在冲突。

| 概念 | 定义 | 不是什么 |
|---|---|---|
| **事实表**（tsv） | 有限定义域上的决策映射，人可审、可 diff；范式的**源** | 不是生成物，不手改生成结果 |
| **生成器** | 表 -> 网/程序 的确定性构造程序（`tools/*-gen.cx`、`seed/gen.c`），带 `--check` | 不是训练 |
| **网**（NN） | 由事实表构造的小整数阈值神经网络；在表的**全定义域逐键验证 = 表** | 不是训练所得，不声称泛化；验证只证「网 = 表」，不证「表 = C99」 |
| **生成程序** | 生成器产出的转移/动作序列，由执行器运行（承担网之外的动作落地） | 不是手写代码 |
| **模型**（kind N） | 包里的执行单元 = 选择器（网）+ 生成程序 + 常量；P4 包 40 个 | 不等于「网」的个数 |
| **执行器** | 通用机制（定宽运算、栈、字节/帧、资源），语义全来自模型 | 不含语言/ABI 特判 |
| **机制代码** | 尚未进表的经典代码：循环/递归/带副作用序列等 | 不是范式失败，是范式**尚未覆盖**的剩余空间 |

**两套计数不混用**：经典参考 **18 张金表网**（`weights/gold`，其中 5 张为对照臂）；默认产品阶段包 **42 张构造阈值网**（分布在 40 个模型里）。

**替代发生在哪一步**：逻辑 -> 事实表 这一步把决策从代码里拿出来；表 -> 网 是等价的执行形式转换。覆盖度的度量看「决策进表的比例」，现状见 [prd_hist.md](prd_hist.md)「0.0.47 逻辑分布普查结论」（源码中机制代码约 49%，是后续推进空间）。

构造链 Linux g2/g3 根键与动态 closure 的待编预算由 `build/g2-budget-facts.tsv` 经 `tools/g2-budget-gen.cx --write|--check` 生成 `build/g2-budget.tsv`。证据冻结rev `60acc817b86b0605016f38604ec0dbea5fac4d8d`，首键含准备44.4秒=44400ms，后续host.o完整检查点wall_ms=8116；不是对象行的ns。严格小于55秒公式 `1+(55000-44400-1)/8116` 推导最多2个待编键；首键单独运行并核整体phase身份，剩余已完成键不重算。首键达到预算、零/负测量具名拒绝。逐键日志绑定root/rev/源/pkg/constructor/target及artifact路径，全部备份SHA核完才恢复该键对象/cache/rr与追加收据、检查点；备份保留到原phase全树身份通过，恢复可幂等重入，已提交seal不回滚。轻夹具 `tools/check-g2-budget-gen.cx ROOT OUT COMPILER` 覆盖预算、漂移、根/closure SIGKILL、异输入、备份坏SHA与陈旧对象拒绝，不替代真链。

G2日志身份计算不能先于原有目录状态拒绝：没有phase记录却已有slice目录时，先报 `REFUSE unrecorded/existing phase`，不抢先读取尚未提供的pkg/constructor。`check-chain-batchkeys.cx`原七条拒绝/无记录断言保持，夹具仅操作本次自有文件与空目录，不删除已有现场。

G2的 `fp_call` 仍属于 `chainargv` 现有局部char指针数组、n++追加、空终止与step包装支持域；真实正控核19处调用。负控必须锚定真实构造：保留bare-helper检测，并覆盖G2位置互换、helper缺值和未知包装拒绝；不通过新增白名单或改guard顺序跳过检测。

G2预算夹具的成功行遵循 `tests/run-batch.cx` 标记合同：`CHECK_G2_BUDGET_GEN` 后必须是行尾或TAB，不能用空格代替。真实批次验收同时核子进程rc、唯一标记和产品身份不变；负控的预期REFUSE日志完整保留，不据直接夹具rc0冒充批次PASS。

**RI 的固定含义**：本文 RI = **ResourceInput（具名资源输入）**，含名称、字节和长度、存在性/空值及跨流身份绑定，不是 "runtime integrity"。

## 1. unisacc.com 是什么

**一句话**：unisacc.com 是 C99 编译器（带部分 POSIX 库子集），打包成单个类 APE 文件——核心需求是六个 ISA 切片（lnx/osx/win × x86_64/arm64，每个目标同候选原生证据；现状缺口见下），能为 {Linux, macOS, Windows} × {x86-64, arm64} 六目标直接写出可执行文件，并以默认 `-run` 直接运行 `.c` 和 `.cx`。cx 是参照 Rust 设计的模块化方言（`#pragma unisa module/use`），所有权与边界目前为**运行期**检查，不等于编译期借用检查；cx 是编译器无关的语言 / 模块层，定位与结构见 [prd_cx.md](prd_cx.md)。编译内部是 7 个字节流阶段，表状决策交给 18 张事实网（13 张为出货主脊，5 张为对照臂，见 §3.4），网络权重由真值表构造并穷举验证、不经训练。通用执行器执行构造出的选择器与动作程序；这是决策表示的改变，尚无语义决策覆盖率证明，不能由模型体积推出“以网换逻辑”。

**一个文件** `unisacc.com`（APE，即 αcτµαlly pδrταblε εxεcµταblε），一个文件同时是「编译器」和「程序」，但「它能写出的目标」和「它自己运行在哪些原生宿主」是两回事：

- **写出六目标程序**（真）：能编出 {Linux, macOS, Windows} × {x86-64, arm64} 六个目标的可执行文件（ELF / Mach-O / PE），**映像不经汇编器和链接器**（模型路由直接编码写出）。
- **六 ISA 切片是核心需求，不可降级**（主人 2026-11-06 两次重申）：lnx/x86_64、lnx/arm64、osx/x86_64、osx/arm64、win/x86_64、win/arm64，每个目标都要同候选原生证据；当前候选的原生 CI 结果与 Windows 两格仅 portable 子集的限制见 §9.1。
- 参考编译器可写三种格式的可重定位 `.o`，产品自带链接器与 `ar`；
- **自举**：宿主构建 `unisacc-seed.com` -> 种子自建 -> 再自建，三个 sha256 相等（N22）。

**语言**：C99 = ISO/IEC 9899:1999 + TC1–TC3（WG14 N1256）。覆盖度按逐条款账本 [archive/testing-c43-legacy/tests/c99/clauses.tsv](archive/testing-c43-legacy/tests/c99/clauses.tsv)，缺口在 [README.md](README.md) 限制表逐条点名。

**方法**（论文 A 的范式）：编译器里每个**表状决策**（词法类、预处理指令、优先级、类型、指令选择、ABI、窥孔……）由**构造**出来的小整数网络回答——权重从真值表派生，在表的全部定义域上**穷举验证**（不是训练）；结构性粘合是经典代码。范式一句话：

```
事实表 -> 生成器 -> 网 -> 一个通用执行器 + 按包跑阶段
```

四条范式支柱：**有限表表达决策**、**共用执行器提供机制**、**显式流接口连接阶段**、**晚绑定合同隔离声明需求与宿主供给**。

**动态逻辑进表的范式（2026-11-05，实例：arity、type、stmt）**：运行值先**分类**成有限键（机制行），再用**决策表**选标签（`weights/**/*.tsv` -> `tools/decide-gen.cx` -> 网行，逐键 T1，批次 `check-decide`），最后由机制把标签落成动作；值放 `exec/facts`，动作只写 `["constant",NAME]`（棘轮 `check-literal`，跨处一致 `check-limits`）。能进表：一步决策（选标签、选后继）。留在机制或生成器：循环与递归结构、带副作用的序列。T1 只证“网 = 表”，表是否对另靠产物比对和外部裁判。第四例（E4 窥孔，`weights/gold/peep.tsv` 1632 键）证明范式跨流成立：两级都动态时用 `decide-gen --each a=... --select REG`，第一级 `RLD pa_` 选态、第二级 `RLD b*12+rel` 选标签，START 的 1632 条 STX 池写入归零；opt 的生成器是手写清单解释器（`seed/gen.c construct_opt`），接入新 rows 要在白名单加 stem。

**两条实现路线**：C 参考编译器（`archive/reference-c/`，行为裁判与回退）与模型产品（`exec/` 各阶段网络 + 通用执行器），逐字节对拍。

## 2. 有什么功能（已实现能力速查）

签名交接成员清单：`sh ./unisacc.com tools/release-members.cx -- DIR NEW_MANIFEST APE OSX_ARM OSX_X64 CACHE_ARM CACHE_X64`生成无header五角色清单；`--check DIR MANIFEST`重核实际SHA、P4/footer与四thin MachO边界；检查 `tools/check-release-members.cx ROOT`，只证明字节/结构，不认证签名或最终候选资格。

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
| 真实输入验收（0.0.51 RA） | `sh ./unisacc.com tools/real-input-check.cx -- [--table FILE] [--only ID] [--list]`；清单 `tests/real-inputs.tsv`（七列：id/args/expect_rc/expect_text/timeout_s/requires/post_sha）；检查 `tools/check-real-input-check.cx`（批 `real-input-check`） | 用已装产品对真实输入逐项实跑，核rc、输出片段、产物sha前缀，输出 `REAL_INPUT failed=N`；requires 缺失记FAIL不跳过；首批5项：release-build p3/slice缺参rc1、release-ape smoke、version-inject对win-arm64 B2 image(38b8ab0e)、release-layout --check。只读真实输入，单条<=60s；不证明链整体可用 |
| 网级依赖图与第14号波及面门 | `sh ./unisacc.com tools/net-deps.cx -- [--summary\|--touch FILE...\|--oracle]`；`tests/net-deps.cx --check` | 路径别名统一规范化；原工具25/0、棘轮行为7/0；42配方静态上界实测pp3/lower12/generator42/facts30，无预留。批次net-deps及net-deps-ratchet；不是运行读集完备或语义覆盖证明 |
| whole-o默认60秒（X2 DONE） | `tools/release-build.cx` 完整44根；正式whole-o-deadline批16/0、slice-freeze-tool22/0、check-limits通过 | 四目标新实测：osx/x64 29.572228s、osx/arm64 30.085927s、win/x64 22.826450s、win/arm64 23.330840s；外bound60均rc0，整调用31.00/30.29/23.02/23.56s。新工具默认60000且未设override，全部根/资源保留，四片SHA同旧C47；旧70秒属另一候选，本次实核已有提速并撤AGENTS120秒例外。短预算真实杀组/半片清理+完整argv/容器/收据已核；快照夹具不再依赖不存在的/bin/true，仅ENOENT复用实编材料；research/c48-whole-o-speed/。不迁移原生CI资格 |
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
| 测试分治粗分：按层选批（0.0.58 草稿） | `sh ./unisacc.com tools/affected.cx -- --layer base\|ext\|stream\|compiler\|driver\|chain\|release\|all [--tier T] [--list-only\|--run]`；表 `build/batch-packages.tsv` 由 `sh ./unisacc.com tools/batch-packages-gen.cx -- --write\|--check` 生成；检查 `tools/check-batch-packages.cx`（批 `batch-packages`） | 层只按批次 id 与源路径前缀切（不做依赖推理），无规则命中的批次归 `all` 层；`--layer L` 选 L 层加 `all` 层加表中未列的批次，拿不准的不丢。2026-11-12 实测 946 批：all 690、release 73、compiler 70、stream 60、chain 20、driver 18、base 12、ext 3。登记新批次后须 `--write` 重新生成。`affected -- --closure-dump` 逐批导出闭包事实；生成器 `--packages` 另填所达包与包层档（conformance/integration/chain），为依赖精化草稿，未入库、affected 尚不读。只是选择事实，不是测试结论 |
| 候选链相位预检(隐性前提) | `sh ./unisacc.com tools/preflight.cx -- build/preflight-rules.tsv ROOT=DIR [BUILD=DIR] [SNAP=DIR] [REV=SHA40] [PKG=FILE] [PHASE=entry\|post-snap\|pre-p3\|post-p3] [--strict]`；检查 `tools/check-preflight.cx`(批次 preflight-tool) | 逐条执行规则表 assert 列(规则表行数以表为准;hook 列标挂点;断言 absent/present/exec-if-present/epoch-canonical/release-rev/src-has/src-lacks/pkg-headers/env-set/receipt-if-present),输出 PASS/FAIL/SKIP 与 error_text,汇总 `PREFLIGHT<TAB>failed=N`;PHASE= 只跑该挂点规则,且该挂点应已产出的变量缺失记 FAIL;变量未给的规则记 SKIP 不记 PASS,`--strict` 下有 SKIP 即退出 1;失败退出 1 |
| 改一个文件看爆炸半径（一条命令） | `sh ./unisacc.com tools/flow.cx -- check FILE... [--run]` | 串起符号图受影响文件 + 受影响测试批次；`flow -- graph FILE` 出局部图 |
| 模块关系图（全图/子图/差分） | `sh ./unisacc.com tools/symindex.cx -- graph [FILE...]` / `-- delta FILE...` | 节点=文件、边=use/call；给 FILE 只出该文件的正向 use 闭包 + 反向 users |
| 文档断链 | `sh ./unisacc.com tools/doc-links.cx -- --tracked` | 归档后修链 |
| 只提交自己的行 | `sh ./unisacc.com tools/commit-own.cx -- -m MSG --keyed PATH KEY` | 多人同改文件（prd/batches/coverage） |
| 任务图并发推进 | `sh ./unisacc.com tools/dag.cx -- ready`、`plan --max N`、`check`、`mark ID STATUS [SHA]` | 图在 `agentwork/dag.tsv`；plan 选出写域两两不相交的就绪节点（RUNNING 占域并计入 N），共享登记表在 `#shared` 闸节点（S7）DONE 前合为一个冲突域；两个带通配的写域按字面前缀保守判相交；批次 `dag`（`tools/check-dag.cx`） |
| 后台候选线 B1（0.0.58） | `sh ./unisacc.com tools/bg-candidate.cx -- start --rev HEX40 [-- 链选项]`、`status [REV]`、`promote --check --rev HEX40`、`prune [--keep N] [--dry-run]` | 候选在 `out/b1/<rev>/`，由 chain-run 带分段与监督器从源冷构造，与发布链同一条命令；start 落 OWNER 行（守护 pid、链 pid/pgid），守护进程在 deadline 加余量处击杀并落 EXIT 行，rc0 后写封条；promote --check 只读核对修订、完整性与收据封条，打印发布尾所需的 APE 路径与 sha、源 SID、打包输入，不发布；版本槽由 `release-run --pack` 用这些片重打包写入，不重跑链；prune 只改名进 `~/cc-work/trash`，无该目录则不动，绝不删除；尚无自动触发（提交钩子或定时）；批次 `bg-candidate`（`tools/check-bg-candidate.cx`） |
| 提交器出补丁 | `sh ./unisacc.com tools/submit-patch.cx -- OUT.patch [OWN_ID]`；检查 `tools/check-submit-patch.cx` | 私有克隆 git add -A 后 full-index 补丁(序号后缀不覆盖)、sha、tools/*.cx 逐个 -c、新增 .cx 走 newcx-check、batches/coverage 只含 OWN_ID 且 IMPLEMENTED 的 check 存在、须含 index 行；一行 SUBMIT_PATCH |
| 研发耗时账本 | `sh ./unisacc.com tools/devtime.cx -- append ...` / `-- report [FILE]`；表 `agentwork/devtime.tsv`；检查 `tools/check-devtime.cx` | 列 task_id/窗口/开始/交付/落地/类别/备注；类别六种；report 按类总秒与占比，未记 UNMEASURED |
| 版本度量表（0.0.58 T9） | `sh ./unisacc.com tools/version-metrics.cx -- --write\|--check [--root DIR] [--table FILE]`；表 `agentwork/version-metrics.tsv`；检查 `tools/check-version-metrics.cx`（批 `version-metrics`） | 从 `steps.tsv`/`chain-run.tsv`/`MANUAL.tsv`/`ledger.tsv` 派生每版 15 列指标（0.0.54..0.0.57 回填），不对 checkpoint `wall_ms` 逐行求和；`--check` 守漂移 |
| tlib 接口清单（0.0.58） | `sh ./unisacc.com tools/layer-api.cx -- --write\|--check [--pkg PKG] [--root ROOT]`；包表 `build/tlib-packages.tsv`；清单 `build/layer-api/PKG.tsv`；检查 `tools/check-layer-api.cx`（批 `layer-api`） | 由产品 `-cx-interface` 生成每包公开函数与类型（名字、签名、签名 sha、文件）；`--check` 守清单等于源码、表 `api_sha` 一致、单元闭包不出包及其声明依赖；`--write` 报升版类别 none/minor/major，不自动改表 |
| 磁带库 tlib（0.0.58） | `sh ./unisacc.com tools/tlib.cx -- build PKG [--out DIR] [--cache DIR] [--jobs N]` / `check FILE [--fresh]` / `inspect FILE [--api\|--receipt]` / `link TOOL.cx -o OUT --tlib FILE... [--require PKG@X.Y.Z]`；检查 `tools/check-tlib.cx`（批 `tlib`，夹具）、`tools/check-tlib-base.cx`（批 `tlib-base`，真实 cx.base）、`tools/check-tlibpkg.cx`（批 `tlibpkg`） | tlib 即产品 ar 归档 `PKG@X.Y.Z.tlib`：`__.S.receipt`（源 sha、产品 sha、标志、目标、依赖内容 sha、回执 sha）、`__.S.api`（等于 layer-api 清单）与各单元 `-c -funit` 对象；链接器跳过 `__.S` 成员，经 `.a` 符号链接别名喂给产品。缓存 `~/.cache/unisacc/tlib/PKG/<content-sha>.tlib` 只在输入 sha 精确相同时复用且先验结构。退出码 3 篡改、4 过期、5 版本不兼容、6 闭包需要被排除单元。发布链与 gen1/g2/g3 不读 tlib，始终从源码冷建。现有包（lnx/x86_64）：cx.base 27 单元、ext.memx 3、ext.textx 4、ext.sysx 9（批 `tlib-ext-memx`/`tlib-ext-textx`/`tlib-ext-sysx`）、cx.stream 2（批 `pkg-stream`），以及核心环 cx.plan 2（cx/cx0）、cx.runtime 87（cx/runtime 及 image/net/stream）、cx.compile 78（cx/compiler 及 image/net/stream，依赖 cx.base、ext.memx、cx.stream、cx.runtime）、cx.driver 10（批 `pkg-plan`/`pkg-runtime`/`pkg-compile`/`pkg-driver`，行内 `dep=PKG` 表示依赖只作材料）；cx/core 已在 cx.base 内，不另拆 cx.core；九包均 0.1.0 且各有冻结清单；`kit` 为增量（只重建回执身份变了的包，9 包空转实测 0.49 秒，可在每次门或批前直接跑）；实测根单元改动后 `tools/release-sweep.cx` 运行路径 7.4 秒、tlib 链接 5.8 秒（省约 21%）；准备路径的接线见下一行，产品默认运行路径 `sh ./unisacc.com TOOL.cx -- ARGS` 仍未接入 |
| tlib 接线：准备路径自动走磁带库（0.0.58） | `sh ./unisacc.com tools/tlib.cx -- kit [--out DIR] [--jobs N]` 建套件（各包 tlib、`tlib-prep` 原生映像、`kit.tsv`，默认 `out/tlib`）；之后 `cxlib.toolprep` 的 `tq_native` 与 `cxlib.runnerprep` 的 `rn_prepare` 不改调用方式，编译前先在 `tools/bound` 20 秒内跑 `out/tlib/tlib-prep prep TOOL -o BIN`（两个模块不新增 `use`：实测多一个模块让每个用它的检查冷编多约 2 秒）；环境守卫 `cxlib.tlibpkg` 的 `tp_prep_forbidden` 由映像自己调用；准入表 `build/tlib-prep.tsv`；检查 `tools/check-tlib-prep.cx`（批 `tlib-prep`） | 只有准入表列出的工具走链接（现列 `tools/release-sweep.cx`、`tools/version-metrics.cx`，入表须带本机实测且该工具自己的检查用链接映像跑过）；`prep` 先验 `kit.tsv` 自身 sha、产品 sha、`tools/tlib.cx` 闭包各单元 sha 与映像 sha，再走与 `link`/`check --fresh` 相同的结构、版本、新鲜度校验；任何不满足（无套件、未列、过期、篡改、链接失败）都回退到原来的 `-o` 编译，退出码 7 表示不适用。成功时 `PREPARED` 行多一个 `via=tlib` 字段，只是准备，不存不读任何结论。发布链与 gen1/g2/g3 不走此路：环境里有 `UNISACC_CHAIN_*`、`UNISACC_CTOR`、`UNISACC_IDENTITY_HELPER` 任一项或 `UNISACC_TLIB_PREP=0` 即拒绝，发布快照根下也没有套件。套件不会自动重建：产品、包源码或 `tools/tlib.cx` 闭包一变就整体过期并回退，需再跑一次 `kit`；链接映像带产品的"多对象各有私有 stdio 表"告警，入表工具须逐个验证输出一致。实测（2026-11-12，关运行缓存逐批冷跑 run-batch，一次一个编译）：批 `release-sweep` 26.4 -> 22.6/21.6 秒，`version-metrics` 24.7 -> 24.6/24.8 秒（无可测收益），未列工具的 `toolprep` 17.8 -> 17.3、`runnerprep` 27.7 -> 26.4 秒（噪声内）；收益只来自闭包大半在包内且根单元小的工具。 |
| s9 夜跑编排 | `sh ./unisacc.com tools/s9-nightly.cx -- ROOT RUN --tier commit\|daily [--date D] [--fake BIN]`；假 s9 `tests/fixtures/s9-nightly-fake.cx`；检查 `tools/check-s9-nightly.cx` | 按手册 build/init/prepare 六步/step 至非 PENDING/aggregate；失败具名停止留报告；夹具不跑真批次 |
| 产品全景表 | `sh ./unisacc.com tools/product-map.cx -- --write\| --check`；轻检查 `tools/check-product-map.cx`；重夹具 `tools/check-product-map-fixtures.cx -- unrelated\|prd-edit\|input-change` | 层/片/流/网字节与 prd 块对齐；默认检查只读 tsv+prd，不跑 --write |
| agentwork分片读侧、迁移与账本追加 | `sh ./unisacc.com tools/aw.cx -- ledger/items/check/migrate [筛选] [--root ROOT]`；`-- append --owner OWNER --id ID --event EVENT --evidence TEXT [--commit C] [--date D]`；检查 `tools/check-aw.cx -- pure/parallel/conflict` | 按月/owner账本、按ID看板，历史5/7列及重复/行尾位无损容器；新布局严格校验，真实并发夹具零冲突，同项双方状态可读；append 为 owner 串行化原子追加（O_APPEND+fsync，路径/IO 校验失败 rc2）；工具交付不代表真实数据已迁移，迁移/删旧/引用切换仅由cc静默执行 |
| 单元前端缓存回归 | `sh ./unisacc.com tools/check-unit-frontcache.cx -- OUT equiv0..equiv4\|neg\|race\|stderr\|switch\|concurrent\|budget [--product CMD] [--base CMD]` | 产品内 e2/e1 单元缓存(cx/runtime/unitcache.cx):26 例字节等价分五片、九类负例、编译中改头、stderr 回放、两个开关与非宿主目标、并发写同键、预算淘汰;产品无该特性时 DEFERRED 返回 3;批次 unit-frontcache-*(材料 unitcache-product=带特性的候选,无材料时 affected 报 DEFERRED needs-materials);设计见 plans/unit-frontcache.md |
| 装前判产品能不能用 | `sh ./unisacc.com tools/check-dev-product.cx -- ROOT PRODUCT` | 六条：shell 头 / `sh --version` / 不带 `UNISA_CONTAINER` 能编译 / 写入产物 / 内嵌包是新头 |
| 失控与超数量`.cx`进程守卫 | `sh ./unisacc.com tools/procguard.cx --once`（默认dry）；`--enforce`才回收；`--only-parent PID`收窄受控扫描；检查`tools/check-procguard.cx`（批`cx-procguard`） | 每10s扫描同uid/.cx；保留同文件数>`PG_MAX`(100)规则，另将累计CPU>=90%且存活>10分钟、cwd已删、ppid1且>30分钟非tmux树列候选。只日志不默认信号；enforce逐PID先STOP后KILL并复核starttime/uid/保护，绝不按pgid/session杀。`CLK_TCK`真实读取，读失败不启CPU/orphan判定；沿用本身/祖先/原名单及tmux/pane保护，orphan判定排除tmux子树。日志`~/.cache/procguard/log`，旧组回收后的fresh-scan `residue=KEY:N`合同保留。 |
| 超时作业的整棵树清理 | `#pragma unisa use sys.process` -> `sy_child_subreaper()`、`sy_kill_tree(root, ms, keep, nkeep, rep, cap)` | 先冻结根、反复扫 `/proc` 把新后代与同组进程也 SIGSTOP（被停进程不能再 fork），到不动点后全杀，返回残留数（0 才算干净）；starttime 防 pid 复用；subreaper 让双 fork 的孤儿仍可见；非 Linux 返回 -ENOSYS。检查：`sh ./unisacc.com tools/check-reaper.cx` |
| 子进程超时契约（run/capture + residue） | `sh ./unisacc.com tools/check-netpack.cx` | 双管EOF后仍按原deadline非阻塞等待，超时冻结整树后击杀并有界定向回收直接子，不抢调用者的其他子进程；`np_result.residue` 0才算干净。孤儿可见性仍需调用方先`np_subreaper()`。`UNISACC_NP_FAULT=residue`为tools与run-batch的显式注入缝，默认关，其它值具名拒绝；sysx产品单次回收疑点另列0.0.47，不混同此测试库修复。 |
| e2stream 包装契约 | `sh ./unisacc.com tools/check-e2stream.cx` | 自建进程组 + 整树击杀（含 setsid 逃逸）+ `TimeoutExpired` 文案；干净超时不带 `survived)` 后缀 |
| 督导观察 agent 面板 | `sh ./unisacc.com tools/observe-agents.cx -- [--panes a,b,c] [--no-git] [--ratchets] [--fixture FILE]` | 逐个 tmux 面板读 `PANE`（state 取 mind 块首行 `▶`=busy / `○ 空闲`=idle，busy_for 取底部栏相对时长）/`PLAN`/`TOPIC`/`TASK`，并按 `agentwork/items.tsv` 第 9 列标 `STALE`（DONE、CLOSED…、SUPERSEDED）与 `PARTIAL`（DONE-\*，尾巴未收）；`--fixture` 用文本面板，`tests/check-observe-agents.cx` 即其批次；输出 ≤30 行 |

#### 2.2 测试（棘轮、门禁、批次）

T1合同登记（登记不等于通过）：`generator-location`批次自建真实GCC hostgen，并保留 `ROOT ABS_GEN FRESH_OUT` 独立入口；`unipp2-reference`必须绑定 `newpp-run/newpp-net`；30个`netregen-CASE-OS-ARCH`批次必须绑定 `old-product/new-product/new-models`，各自prepare后执行A/B镜像比较。PREPARED不作测试结果；镜像字节同不代表外机执行资格。`opt-frame4-o1-*`与`opt-frame4-o2-*`共8片各显式绑定对应o1-run/o1-net/o1-table或o2-run/o2-net/o2-table，分别执行positive/negative-label/negative-end/negative-eof；两算法路径分别登记，覆盖语义投影、有序元数据、唯一end、真实dot标签与精确截断拒绝，不升级为产品全链覆盖。


| 动作 | 现成用法 | 说明 |
|---|---|---|
| 核对队友报告里的事实 | `sh ./unisacc.com tools/report-verify.cx -- CLAIMS.tsv`；校验 `tools/check-report-verify.cx` | commit/tsvcount/receipts/sha256/ratchets 五类断言逐条核实，输出 OK/MISMATCH/MISSING，有 MISMATCH 退出 1 |
| 查模块层向与包边界 | `sh ./unisacc.com tools/check-cx-layers.cx [PRODUCT]`（`-- --fixtures` 跑夹具） | 编号层 L0–L3 直接 `use`，向上越层即红；`ext/` 是包层：产品层（`src/`、`cx/core`、`cx/runtime`、`cx/compiler`、`cx/driver`、`cx/cx0`、`cx/workspace` 与 `tests/cx-module-freeze.tsv` 的 core 模块）`use` 到 `ext/` 即红；core 模块文件集合按冻结表只许减（新增行即红、行无文件亦红） |
| whole-front 整程序展开预检 | `sh ./unisacc.com tools/check-whole-front-expand.cx -- ROOT NEW_ABS_OUT [--partition FIRST COUNT ...]`；`--selftest` 为轻量负控 | 从 release-build 的实际根表调用 `cx0_plan`，拒绝跨单元定义冲突；默认按实际展开数推导 32/32/末片并核覆盖，不再固定85。保留 plan.tsv/diagnostic.txt；不跑 e2/e3、对象或图像，不代表阶段等价或候选资格。批 whole-front-expand 与 whole-front-expand-controls |
| 共享字节帧与阶段传输 | `cx/runtime/frames.cx`、`cx/runtime/frameparts.cx` | cx-L1 通用字节帧构建、精确块共享、UNIFP1 编解码；驱动及 holdframe 共同依赖，不依赖 compiler。源单位上限唯一声明仍为 `compiler.cxmod` 的 `CXM_MAXU/DV_MAXU`，运行缓存直接复用。源路径迁移不等于已重构造或 APE 字节相同。 |
| core 导出面棘轮 | `sh ./unisacc.com tests/coreapi.cx -- --check`（`--write` 下调，`--fixtures` 跑夹具） | 26 个冻结 core 模块的每个非 static 定义各占一行：新符号红、行缺失红、签名变动红、`DEPRECATED-ALIAS` 的调用方文件数超上限红；`core_run` 一行标 `DUAL-IMPL linked=cx/sys/kernel.cx`，两条链接由 `cx-core-run-diff` 逐字节比对；当前一次构造native symindex后逐查询执行，26模块/194导出实核13.874s、原8夹具14.322s，原55秒不扩，查询结果不缓存 |
| core_run 双执行器差分 | `sh ./unisacc.com tools/check-core-run-diff.cx -- ROOT KERNELS` | 同一模型与三组固定输入分别喂 `cx/core/execute.cx`（CX 参考实现）与 `cx/sys/kernel.cx`（产品链接的汇编内核），两行整体逐字节比较，末行 `CORE_RUN_DIFF`；KERNELS 是含 `x86_64.kernel` 的目录 |
| 拒新的模块 `#include` | `sh ./unisacc.com -run tests/cxinclude.cx -- --check` | 新模块必须 0 |
| 上层禁 libc/POSIX | `sh ./unisacc.com tests/cxlibc.cx -- --check` | 名单不手写，按前导分类 |
| 看某域还差什么 | `sh ./unisacc.com tests/full-domain.cx -- . DOMAIN` | 逐格读矩阵，印 OPEN 行 |
| 记录已知缺口 | `sh ./unisacc.com tools/matrix-waive.cx -- . tests/waivers-0.0.44.tsv --check` | R 行理由 + W 行豁免 |
| 全链有界续跑（0.0.57 K2） | `sh ./unisacc.com tools/chain-run.cx -- --rev HEX40 --out ABS [--deadline-s 5400] [--max-resumes 3] [--max-attempts 4096] -- --cache-manager-osx X64 ARM64` | 按 chain-phases.tsv 分块调用 candidate-chain --resume --until，子调用55秒；deadline/次数跨调用持久化，不无界轮询。非超时首败立即BLOCKER，不自动绕行；仅超时/被杀有界续跑，chain-watch核现场，活owner拒绝；输出PREPARED/not=PASS，已有pack须本工具身份及产物收据SHA匹配。`--rehearse --rehearse-ctor ABS --rehearse-objects ABS --rehearse-pkg ABS` 仅snap/build/ctx及显式材料的whole-front prepare/front0，缺材料拒绝，只记REHEARSED/not=FULL。真实整链UNMEASURED，耗时回写K8不在本工具。夹具check-chain-run不跑真链。 |
| 链分段、监督器与续跑备忘（0.0.58 T11） | `sh ./unisacc.com tools/chain-run.cx -- --rev HEX40 --out ABS --supervise --segment-s 270 --max-segments N [--no-sha-memo] [--memo-derived] -- ...`；库 `cxlib.shamemo`；检查 `tools/check-shamemo.cx`、`tools/check-chain-run.cx -- --segments\|--supervise ROOT OUT` | `--segment-s`(1..270)限定一次调用：相的整段预算放不进剩余段时不启动，段到期写SEGMENT行并以75退出(无被杀子进程)；总deadline、尝试、恢复、段数(`--max-segments`)都持久化，新段不重置；`--supervise`逐段续跑到rc非75或上限。备忘 `OUT/chain-sha.memo` 绑定本输出身份：文件 SHA 只在 dev/inode/大小/mtime/ctime 全同时命中，源码 SID 只在整树元数据清单全同时复用;verify-context 结论与闭包计划的派生备忘默认关(两工具还依赖宿主缓存与环境,前提经源码核对不成立),`--memo-derived` 才开，年轻条目与越界软链不入备忘；compare 与 pack 之前各有一次去备忘的 `--recheck-only` 完整复核(独立有界尝试，留收据，一次复核只授权一次相位调用)。备忘不是 ABA 保证；不授产品资格。 |
| 逐相预算与进程所有权（0.0.58 K10） | `tools/chain-run.cx`；`tools/cxlib/chainrunproc.cx`；真实批次 `chainresume`(daily)、`chainrunproc`/`chain-run`(commit) | 读表1..55秒，整表越界前拒；build/wave对象与链接独立SHA收据及检查点，ctx原生复用；真实PID/PGID/starttime+pidfd核所有权，预算内清整树后核终态才有限恢复，观测不确定拒续。仅假链行为检查，不授真实构造资格。 |
| 作业续跑摊销修复 | `chain-run` -> `candidate-chain --resume --until jobs-GROUP --jobs-segment-s N`（N=55..267） | jobs-tool/gen/tbl/net 同调用推进多个 start/finish。段首按权威清单经现有 source-package reuse-batch 全量核全部 DONE job 身份与字节，成功集合只限本进程；作业段关闭持久 SHA 备忘。每子作业仍<=55秒，完整预算放不进段即无在途子进程返回75；外层核本次attempt/目标及当前checkpoint SHA收据才续段，不消耗失败重试。整个段含前缀核验受<=270秒所有权监督并留3秒返回/清理。compare/pack原全量复核不变。SEGMENT_VERIFY记job数/毫秒；chainresume及chain-run覆盖进程内复用、新进程重核、篡改/伪造边界拒绝；真链资格另须新OUT实跑。 |
| 链检查证据结构 | `tools/check-chain-run.cx`（批 chain-run/chain-run-segments/chain-run-supervise） | 阶段表由显式 put_plan 写入，不从路径子串猜类型；证据经现有 tx_fields/tx_field 拆记录/字段，限定协议字母表与标识符组件边界后匹配，未知字节拒绝。全部原行为断言保留，另测组件前缀误命中及未知协议字节负控；不读取源码文字作判据，不改执行链资格。 |
| 构造图检查（G1） | `sh ./unisacc.com tools/graph-check.cx [-- --graph F --chain F --hardwire F --no-hardwire]` | 读 `build/graph.tsv`（203 节点事实表）：无环/依赖存在/kind 枚举/budget≤60；六目标次序与 `tools/chain-step.cx` 三份常量比对（不一致 ORDER_MISMATCH+rc1）；`build/hardwire.tsv` 32 条第二真相源模式逐行扫描只计数；末行 `GRAPH_CHECK\tnodes=\tedges=\tcycles=\torder_mismatch=\thardwire=`；夹具 `tools/check-graph-check.cx` |
| 根独立/帧规范化（0.0.50） | `sh ./unisacc.com tools/root-canon.cx -- --map PREFIX=PLACE... [--diff A B \| --scan F \| F]`；`tools/root-independent.cx -- --a DIRA --b DIRB --list F --map ...` | 多前缀最长优先替换，按 formats.md 重算 phase/单元/UNIPP2 LE32 长度；末行 `ROOT_CANON_DIFF\traw_equal\traw_len_delta\tprefix_bytes\tnonprefix_bytes\tcanon_equal`；独立性 list 行 `node\tproduct\|frame\trelpath`，product 逐字节且不含根、frame 规范化相等；`build/graph.tsv` 第 12 列 root_dependent 由 graph-check 校验；夹具 `tools/check-root-canon.cx`、`tools/check-root-independent.cx` |
| 链看门狗（0.0.51 W/W2） | `sh ./unisacc.com tools/chain-watch.cx -- DIR [--idle-min N] [--final PHASE] [--resume-cmd 'CMD'] [--max-resumes K] [--now EPOCH_S]` | 读 `DIR/checkpoint.tsv`（candidate-chain 的 `DONE\tphase\t...` 行）取 last_phase，进展时间取 checkpoint/DIR 顶层/`DIR/logs` 最新 mtime；N 分钟（默认 10）无进展判 STALLED，含 final 相位（默认 pack）判 DONE；STALLED 前核 `DIR/heartbeat.tsv` 末行（`epoch\tphase\tpid\tpgid[\tstart\|end]`，由链每相位写）的 pid/pgid 存活：liveness=HUNG（存活）\|DEAD（不存活）\|UNKNOWN（无心跳），三者都续跑；一行 `CHAIN_WATCH\tstatus=RUNNING\|STALLED\|DONE\tlast_phase=\tidle_s=\tliveness=`，现场写 `DIR/state.md`；STALLED 且给 `--resume-cmd` 时经 `sh -c` 执行一次（55s 上界），次数记 `DIR/chain-watch.tsv` 封顶 K（默认 3，耗尽 rc3 不再重启）；退出码 0 RUNNING/DONE、1 STALLED、3 耗尽、2 用法；夹具 `tools/check-chain-watch.cx`（临时目录伪造三态、HUNG/DEAD 与封顶，19 项） |
| 仓级棘轮（13 项） | `sh ./unisacc.com tools/ratchets.cx` | cxraw/cxinclude/cxapis/cxtrunc/cxlibc/cxplatform/srcshape/coreapi/verwriter/aluiwidth/outdep/check-literal/net-deps；toolsloc退役，新增net-deps沿计划保留第14号门称呼；旧通过记录不迁移 |
| tools 统计与候选清理 | `sh ./unisacc.com tools/tool-inventory.cx -- --recent 10` | 总行数、按目录/文件及近N次提交增量；无调用者/无批次/疑似重复只报告；不设总量或逐文件上限。tests/toolsloc.tsv退役保留；precommit只对新增tool缺check或批次登记拦截 |
| 检查源码锚禁止合同（0.0.58） | `tools/cx-lint.cx --strict --rules R4`；提交门同规则强制执行；配套 `tools/check-cx-lint.cx` | 禁止把源码原始字节中的实现语句、格式或替换锚当作通过/拒绝依据；事实表、运行结果与结构化 token/调用参数分析不是源码字面锚。WFE、limits、graph-check、whole-o、chain-argv 同样适用，不设文件豁免。结构分析的未知构造须具名拒绝，不能当作检查通过；原拒绝覆盖及时间预算不变。 |
| 禁锚支持域与迁移边界 | 既有 `refactor find` 调用事实；`cxlib.chainargv` 种类化 token/平衡跨度/唯一函数/调用参数范围 | 仓级棘轮及 contract/interfaces/negative 分层不变；禁锚门约束证据取得方式，不禁止静态分析。不得将全文压平后匹配。调用参数内部注释超出 refactor 支持域时具名拒绝，不报行为回归；运行输出、收据、事实表仍按自身字节格式校验，不因含源码词语变成源码锚。 |
| 检查用共享结构查询 | `cxlib.chainargv` 的 `ca_tokens/free`、`ca_token_count/text/kind/is/end`、`ca_function`、`ca_call_arg`；负控 `check-chain-argv-check --tokens` | 复用既有词法器，字面量种类与语法 token 分离；唯一函数定义、平衡范围及嵌套实参；非法词法/括号、函数歧义、空实参具名拒绝。句柄独占并由调用者释放；不是通用 C 类型或别名分析器。 |
| 禁锚来源分析的有限支持域 | 同文件函数作用域、具结构证据的首参 read/path/runner 包装摘要；实际源码/事实/运行结果来源 | 支持 cx_read 及 fopen/fread 返回缓冲包装、别名/直接调用、cx0_plan 与 np_run 返回结果、cx_run 输出参数；私有路径还须生产者 argv 消费或运行输出写入证据，已知源码扩展及源码复制优先。未知读取用于字面搜索仍拒绝；不宣称宏、任意间接调用/指针字段/跨文件数据流或任意子进程文件 effects 完备。 |
| whole-o 实际收据结构消费 | `tests/check-whole-o-deadline.cx` 解码真实子进程生成的 `receipt.txt`；原批 `whole-o-deadline` | 以唯一 key/value 记录核 route/target/bytes/sha/rc/tool_sha/tool_run_sha/tool_src；收据使用只读视图，不复制内容；重复键、NUL、截断及状态前缀冒充拒绝。保留实际 dispatcher 的超时杀组、完整有序 argv/cwd/container 与三种源码身份状态，不为局部 receipt 变量布局扩全 C 路径引擎。 |
| 禁锚最终父与真实批次（0.0.58 T4） | 落地主线 `a84ef9695b4fb34632081791c7716abdce1a300a`（与K9-r5树逐字相同）-> anchorban-r3 | 仅保留其余十一文件，已落图blob一致且不覆盖；保全生产安全格式/显式OUT、G2负控及表键唯一。R4每文件预建括号、作用域名、函数摘要和有序多定义索引，缓存语法事实不缓存判定、不删传播/未知拒绝/2048容量界；旧module扫描超时须以实际0发现消缺，不放宽55秒。普通apply与字节继承有明示来源，正规integrate-patch自门必须真实READY后交付；本地提交末尾LEDGER及out信封供S9取精确父。 |
| 参数检查的有限队列与包络结构 | `tools/cxlib/chainargv.cx`；真实批次 `chain-argv-check`、`chain-argv-tokens` | 常量字符串数组经从0递增且界限等于元素数的for及直接const别名进入argv命令位时，逐元素核被调parser数量/角色/转发，不跳过调用；表/别名/索引改写或未知命令拒绝。bound_sh/step/rest包络核唯一函数、调用实参、复制循环与55秒转发，不再核实现文本片段。保留原十一与G2三拒绝并加队列缺终止/错序/第二命令未知/越界控制。 |
| osx X1旧项复核（2026-11-08） | `tests/driver-target-object.cx ROOT cli`：9/0；平台宏最小源见`tests/cases/driver-default-platform.cx` | Darwin arm64 26.5.1、产品SHA f069d4c92b7f5ede508809aa3abd21e326cbecadb2fb9eccf0bd1ac7004c3e82。默认-run原生、裸-c产宿主Mach-O对象并由产品链接执行，NATIVE-RUN-HOST-TARGET核销；不承诺系统链接器兼容。裸-S仍用lnx/x86_64预定义：平台源bare/显式lnx tape SHA同为d7bb734dae19…，显式osx为e02cac44f622…；DEFAULT-TARGET-S-C保留READY，只剩-S语义裁定/修复，不重复-c修复，也不迁移六宿主CI资格 |
| 真实提交工具配套与源码分类闸 | `sh ./unisacc.com tools/check-precommit-toolsloc.cx -- --loc`、`-- --source`、`-- --symlink`（保留三批历史ID）、`-- --lint-r2-sep`/`--lint-r1-report`；源码锚负控 `tools/check-cx-lint.cx --anchor-controls --lint-tool ABS_NATIVE_LINT` | 独立真实临时git仓运行生产hook；历史行数超额允许，shebang伪源码拒绝，新增tool缺check或缺IMPLEMENTED批次拒绝；staged .cx 经 cx-lint：R2/R3/R4 强制拦，R1 只打印；旧工具无调用者/无批次/重复仅列候选；禁锚负控不授产品或发布资格。 |
| .cx token 层 lint（第二真相源/子进程/源码锚） | `sh ./unisacc.com tools/cx-lint.cx -- [--strict] [--rules R2,R3,R4,R1-version] [PATH...]` | R1-version/R1-product 只报告；R2-timeout/R2-sep 为子进程合同，R3-die 为失败助手终止合同，R4-source-anchor 为来源感知的源码字面锚合同；precommit 对 staged .cx 跑 `--rules R2,R3,R4 --strict`。R4 与 R2 不接受 lint:allow；运行输出/事实读取及 token/AST 分析不等于字面锚。精确计数与来源夹具 `tools/check-cx-lint.cx`。 |
| 预冻结演练(SPEED-049-E) | `sh ./unisacc.com tools/freeze-plan.cx -- plan ROOT out/`；`-- check --ident-list FILE --draft FILE`；`-- boot-list LIST`；夹具 `tools/check-freeze-plan.cx` | ident list-current 导出 freeze.tsv 草案到 out/（不写 agentwork/）；覆盖/过宽非零；启动清单缺文件启动前拒；符号链接真身须覆盖。不覆盖现行 L0-CORE-FREEZE-049 |
| SPEED-049-F 版本数据解耦 | `sh ./unisacc.com tests/release-version.cx`；打包与身份验收见 `tests/README.md` 的 SPEED-049-F 配方 | 唯一版本列为 `release/candidate.tsv`；源固定32字节槽，打包注入。解析测试已登记，未经本轮执行不得转授旧材料资格；L1–L4 非版本格式断言保持。 |
| SPEED-049-A 两代抽样 | `sh ./unisacc.com tools/fixedpoint-sample.cx -- sample --seed N UNITS GEN2 GEN3`；`-- release` 拒 `--sample`；`-- wave PART OUT`；夹具 `tools/check-fixedpoint-sample.cx`；链内开关 `tools/chain-step.cx fixedpoint … --sample --non-release`（979b56e4d9） | 非发布候选确定性抽样(收据 SAMPLED);发布候选强制 FULL;拆波拼接=一次跑。chain-step 默认完整两代，`--sample` 不配 `--non-release` 按发布候选拒绝（check `chain-step-release-forbids-sample`）；0.0.49 发布候选按 FULL 跑，g2==g3（发布说明）。gen1–gen3 整链并行端到端墙钟 UNMEASURED（`agentwork/phase-times.tsv` 第 11 行） |
| C49 X全闭包分相构造 | `tools/whole-front.cx` prepare/front/merge/stub/span；固定点由 `tools/chain-step.cx fixedpoint --span-driver/--span-prepared/--span-index` 逐步接纳 | 真实85单元32/32/21，Mac真实桩后units..e3与prune..elf，Win三段；子相总45秒，来源与checkpoint实际哈希重核；当代Ctor与P4显式绑定，旧env-B路径不用，原生CI资格独立 |
| GEN1 Linux 对象/链接分相 | `entry-build.cx --objects-only` 后接同输入/目录的 `--link-only`；候选链 `--until gen1-lnx-{x86_64,arm64}-objects` 后 `--resume` | 两次子调用各55秒；对象相覆盖全部根与动态闭包；链接仍核全部对象缓存身份，缺失/陈旧拒绝，不回退编译。原 `gen1-lnx-*` 检查点代表链接完成；轻夹具 `check-candidate-chain.cx -- --gen1-split NEW_PARENT ABS_PRODUCT`，不跑真实构造 |
| G2-BATCH 与 G3 Linux 对象/链接分相 | 候选链沿 `chain-step.cx --objects KEY,...`，根键和动态闭包均经实测预算拆调用；`build/g2-budget-facts.tsv` -> `g2-budget-gen.cx --write|--check` -> `build/g2-budget.tsv` | 首键隔离核phase seal并恢复同身份未提交键，剩余仅投递待编键，当前测量推导最多2待编键/子调用；逐键与动态closure检查点、完整44根覆盖及独立link-only不变。写前日志完整SHA验证后恢复，原phase全树断言与55秒界不变；夹具 `check-g2-budget-gen.cx ROOT OUT COMPILER`，不认证真链或六目标资格 |
| 分相结构合并及独立边界 | `tools/check-whole-front.cx`；`tools/check-wholefrontmerge.cx`，批次whole-front/wholefrontmerge | 原9项合同保留；另实测3项多属性顺序/opaque字节、非相邻重复拒绝无输出、零单位双空编码；模块检查不冒充产品等价或候选资格 |
| 分片测试：显式绑定 slice/pkg 启动 | `tools/test-loader.cx`（`--slice ABS --models ABS --identity ABS`、`--net STAGE=ABS --swap-tool ABS` 单网替换、`--e2-adapter`）；检查 `tools/check-test-loader.cx`（批 test-loader-com，需 5 个 `TEST_LOADER_*` 绝对路径环境绑定） | 开发证据，不是发布资格；缺绑定非零，不回落读 `./unisacc.com`；规格见 plans/test-loader-com.md |
| 波次哪些批不能同跑 | `sh ./unisacc.com tests/check-wave-domains.cx`（批次 `wave-domains`） | `tests/wave-domains.tsv` 每行 `id<TAB>domains<TAB>reason<TAB>evidence`；domains 取 `serial`（独占一波）/`wt`（写共享路径）/`hs`（写 harness 三树）/`mi:<材料名>`，空=无约束；all-waves 按掩码两两不相交放置 |
| 单阶段过滤器（分段编译管道） | `./unisacc.com stream STAGE [--route R] [--res NAME[=FILE]] [--models PKG] [--raw] < in > out`；`stream frame --in L=F...`；`stream sidecar` | 同一 st_span/包/`\0cli/*` 跑一个阶段；属性表随流（USLATTRT）；`e2\|e1\|e3\|sidecar\|prune\|lower\|elf` 与 `-o` 逐字节同；校验 `tests/check-stream-pipe.cx`；改表 -> `tools/lab-loader/p3-swap.cx --model` -> `--models` 实测 ≈8s |
| cx.stream 核心包（0.0.58） | `#pragma unisa use stream.item` / `stream.pipe`（cxroot 行 `R stream cx/stream`）；12 个公开函数 `xs_item_init/set/bytes/drop`、`xs_open/add/tap/run/receipt_count/receipt/receipt_id/close`；一致性 `sh ./unisacc.com tests/stream/xs-conform.cx`（批 `xs-conform`），包检查批 `pkg-stream`，清单 `build/layer-api/cx.stream.tsv` | 有界字节管道：装配时拒形状不符；所有权移入管道，失败即释放清零；返回码 1-4 同 st_span，5 为超界具名拒绝不截断；回执身份为输入、阶段号、输出摘要上的 sha256（自带实现，不依赖 Linux 专属 sys.sha）。只依赖 mem.grow/mem.slice/mem.bytes/text.text。首个消费者 `src/drivestream.cx`（剥属性表 -> 模型阶段或 sidecar 两阶段）；阶段函数以 void * 传 item（函数指针形参带结构类型在导入接口中未覆盖，见 pkg-gaps） |
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
| 检查里跑 `.cx` 工具：编译不占断言步时限（0.0.55 T55-INNERBOUND） | `#pragma unisa use cxlib.toolprep` -> `tq_native(root, tool.cx, NATIVE, why, wl)`，断言步改跑 `bound N NATIVE ARG...`；工具内部再跑的 `sh PRODUCT x -- ARG` 用 `tq_warm_start/tq_warm_join` 后台预热；检查 `tools/check-toolprep.cx`（批次 `toolprep`） | 内层 `bound N sh PRODUCT tool.cx -- ARG` 把工具冷编译（静机 9 秒、负载下 17-18 秒）算进断言步，源一变就 rc=142，且被杀后运行缓存不落盘。`tq_native` 用本 ROOT 的产品把工具编成原生映像（自带 40 秒 `tools/bound`，只印 `PREPARED`，不跨检查复用、不缓存结论），断言步时限和断言不变。模块只用 sys.proc 与 sys.io、不带 netpack：同一检查冷编译实测只用这两个模块 4.3 秒，带 netpack 8.0 秒。批次整体 55 秒预算不因此变宽：预编译仍计入批次总时长。 |
| 运行器原生预编译、按内容身份复用（0.0.57 T57 方案 A 的库，尚未接线 affected） | `#pragma unisa use cxlib.runnerprep` -> `rn_prepare(root, "tests/run-batch.cx", CACHE_DIR, bin, cap, &reused, why, wl)`、`rn_identity(...)`；检查 `tools/check-runnerprep.cx`（批次 `runnerprep`） | 身份 = sha256(产品身份 + 目标 + 编译器自己的 `-cx-plan` 单元清单逐个路径与字节 sha)；映像放 `CACHE_DIR/<身份>/`，复用前现算映像 sha 与构造时写下的证书比对，不符或缺证书即重建；构造后复算身份，输入变了就拒绝；只印 `PREPARED`，失败具名、不回退到 `sh PRODUCT SOURCE`；不存不读任何批次结论。身份不含 `#include "..."` 文件与环境。不 use netpack/sysx.process，两类调用方都能用。 |
| 检查的冷编译耗时与 use 闭包量表（0.0.57 T57 方案 B） | `sh ./unisacc.com tools/coldcompile.cx -- OUT.tsv SCRATCH FILE...`；`-- --plan-only FILE...` 只出闭包；数据 `plans/check-coldcompile.tsv`；检查 `tools/check-coldcompile.cx`（批次 `coldcompile`） | 逐文件串行：全新 TMPDIR 下 `PRODUCT FILE -o IMAGE` 的墙钟（tools/bound 40，只编不跑），另记单元数、闭包字节、起测时 load1 与 use 列表。是测量不是测试结论；耗时随机器负载变，跨行比较要看 load1。 |

#### 2.3 发布（资格、会话、安装）

| 动作 | 现成用法 | 说明 |
|---|---|---|
| 发布后清理与证据归档（0.0.58 T8） | `sh ./unisacc.com tools/release-sweep.cx -- --manifest FILE --trash DIR --out-manifest OUT.tsv [--latest VER] [--archive-dir DIR] [--refs FILE...] [--dry-run\|--apply]`；检查 `tools/check-release-sweep.cx`（批次 `release-sweep`） | 按清单执行 move/archive/keep：只 mv 到 `/home/devali/cc-work/trash` 不删，拒绝 `/tmp`；保留最近一版冻结克隆（`r057`）与链输出（`chain057`/`release-snap-*`）及 `--refs` 引用路径；`<=1MiB` 证据小表复制到 `evidence-archive/<版本>/`；`pgrep` 与 `/proc` 排除自身跳过占用目录（`SKIPPED_BUSY`）；写出前后字节清单 TSV 并汇总 `RELEASE_SWEEP` |
| 发布隔离演练与六片导出 | `tools/release-oneshot.cx -- --dry-run CANDIDATE LAYOUT NEW_STATE`、`--resume STATE`；`--pack APE NEW_DIR`、`--check-pack APE DIR` | 现算SHA后规划X、先草稿后带release_tag的CI、数字裁决证据；仅PLANNED与UNKNOWN，不触网。六片附同P4/尾并逐字节核；win/x86取APE外层PE。两独立夹具批次，不授发布资格 |
| 发布远端资产门禁 | `release-oneshot --execute STATE CONFIG --authorize-external-operations`（原 release-exec 批次） | 每次dispatch前REST核全部候选资产大小/digest/state与完整集合；七角色及六片沿既有事实表，缺失/陈旧/重复/分页/API异常拒CI，不回退真实联网补救；假夹具不授产品资格 |
| 同候选原生资格（六目标证据表） | `sh ./unisacc.com tests/candidate-qualify.cx -- cell|aggregate ...` | 由 `.github/workflows/candidate-native.yml` 调度；缺证据或身份不符 rc1，有 NOT_RUN rc3，全 PASS 才 rc0 |
| streams 旧合同壳材料 (unisacc-stream1..7.com) | `sh ./unisacc.com tools/streams-material.cx -- ROOT NEW_OUTDIR [LEVEL [TARGET]]`;校验 `tests/check-streams-material.cx`(批次 streams-material) | 7 个 <=40B 薄壳转调 `unisacc stream STAGE`,分派器 `s` 烘入 ROOT;e1 经 `tools/streams-e1-frame.cx` 把 USLATTRT 转 USLATTR1;收据 streams.json scope=product-stream-shim + NOT_CLAIMED,不是独立 runner,与 streampack 8 容器无关;钉子由 tests/cases/stream_pin.cx `stream_receipt_pin` 取自收据。lower 阶段产品会追加运行库尾部,依赖精确 lower 输出的用例仍红 |
| 两代不动点（读 chain-step 产物） | `sh ./unisacc.com tests/chain-fixed-point.cx`（批次 `chain-fixed-point`，材料 `chain-gen1`、`chain-fixedpoint`） | F1 gen2 sha==gen3 sha；F2/F3 receipt 的 constructor_sha 依次指向上一代，防复制同一文件冒充；不重跑构造。材料必须绑到候选文件而非其父目录（RC1-MATERIAL-BIND） |
| 全量会话（all-waves） | `sh ./unisacc.com tools/all-waves.cx -- init/material/wave/pending`（每条外包 `tools/bound 55`） | 会话是 ROOT/out 下新目录，候选必须是 ROOT/unisacc.com；light 波最多 4 个无材料批并发，heavy 波一次 1 批；要点：init 前取最终 MAP_READY 材料，init 后 diff `materials.tsv`，按批次参数形态（文件/目录）写 map——会话内材料绑错会造出假 FAIL，单跑才是 HEAD 事实（ledger RC1-MATERIAL-BIND） |
| 会话图内材料一键绑定 | `sh ./unisacc.com tools/s8c-bind.cx -- ROOT SESSION CANDIDATE MAP`（薄壳 `tools/s8c-bind.sh`；批次 `s8c-bind`） | init 后对 MAP 每名独立 `all-waves material`（各 bound 55）。PREPARED≠PASS。scratch 14 名后 heavy yes 0 -> 84 |
| S9 三项提速（0.0.49，已上 main） | `tools/all-waves.cx` child：(1) 会话内预编译 native run-batch 执行器（27c1b76f86，核 SHA==manifest.runner_sha，缺失或不符拒绝）；(2) 身份继承（34c8a6865a，child 复制父会话 ident 含 harness_sha，wave-before 实测 SID 与父逐字比较）；(3) `--shared-wt`（6fccc10255，仅 `--s9-supervised` 下接受，wave 级共享工作树扫描，aggregate 新增 `wt_epoch_ok`） | child init 4.68s -> 0.84s（34c8a6865a）；不再每 child 源码重编 run-batch（约 13.7s，27c1b76f86）；封条/epoch 缺失或不符使该 slice 全部批次 refused。0.0.49 提交档实测两 step 255.13s+61.94s（022caf53b1），单 step ≤300s 未达成，剩余项见计划 S 包 |
| 候选 P3 自指钉 | `sh ./unisacc.com tools/s9-p3-pins.cx -- PKG MAP`（批次 `s9-p3-pins`） | 同一 pkg/APE 现算 file/p3 sha 写 MAP；c46-3 空跑 p3=14a69316…；篡改一字节必红。禁止旧候选拷钉 |
| 身份派生校对 | `sh ./unisacc.com tools/identity-derive.cx -- ROOT --check|--write`（批次 `identity-derive`） | 编排 `check-root-identity` emit/check；--check 不一致打印 DIFF。尚未接 ape-receipt/build.json/pins |
| 独立切片版本注入(slice-win-arm64 资产) | `sh ./unisacc.com tools/version-inject.cx -- IN OUT [TABLE]`;与 ape-pack 同源 rv_inject_slot,缺槽/双槽/坏表拒绝不写 OUT;检查 `tools/check-version-inject.cx`(批次 version-inject) |
| 发布目录布局与校验 | `sh ./unisacc.com tools/release-layout.cx -- --candidate release/candidate.tsv SOURCE NEW_OUT`；`-- --candidate release/candidate.tsv --check OUT`（批次 `release-layout`；旧 `--version VERSION SOURCE NEW_OUT` 形式保留） | 候选身份只来自 `release/candidate.tsv` 唯一一行；生成六文件及 SHA256SUMS 与 WinARM ZIP，`--check` 重算全部 sha 并拒非法 ZIP；见 `release/README.md` |
| SPEED-049-G 发布裁决清单 | `sh ./unisacc.com tools/release-decide.cx -- --candidate TABLE [--evidence EVID] [--assets DIR]`；夹具 `tools/check-release-decide.cx` | 8 步 PASS/FAIL/UNKNOWN 不得猜；生成身份表与已知限制，不写行为变化。0.0.48 重放 s9 未跑 -> UNKNOWN/NOT_QUALIFIED；0.0.49 实跑 8 步 PASS、QUALIFIED YES（账本 2026-11-10 `RELEASE-0.0.49`，证据 out/c49-release-evidence） |
| 只读发布资产完整读回 | `sh ./unisacc.com release/tools/c47-1-readback.cx -- ROOT TABLE NEW_OUT OLD_APE true\|false` | 身份与资产 sha 均取自 `release/candidate.tsv`；新目录从 draft（true）或正式 release（false）下载，核元数据/sha/版本/冒烟/进程名/ZIP；`tools/release-dispatch.cx -- --dry-run TABLE ASSETS` 只打印将创建的 draft、上传项与 workflow 输入，不做任何外部操作 |
| 固定220例外部语料与期望配对 | `sh ./unisacc.com tools/ctestsuite.cx -- ROOT MATERIAL NEW_OUT START COUNT`，每片≤20；批次`ctestsuite` | 上游rev/归档SHA/每例源码与期望SHA在`tests/corpus-ctestsuite/`；源码不重分发。真实c46-3首轮216符合、4拒绝、0错值、0未知；仅1个拒绝最小例为严格C99，另3个为GNU/C11扩展。输出接受/拒绝/错值/未知和原件，不删拒绝例换绿。 |
| 三方编译差分与百例整型值差分 | 旧`opt-corpus-gen`的`ccdiff[-2/-3/-4] HOST_CC [TCC]`追加第二oracle；`sh ./unisacc.com tools/random-expr.cx -- ROOT NEW_OUT GCC TCC [SEED]`；批次`random-expr` | 原产品O0/O2及HOST_CC字面期望不变；TCC无优化flag，编译警告和三方不一致单列原件。随机每批100个unsigned表达式，固定seed、非零除数、界内移位，逐值比较两个真实oracle。缺编译器非零；默认check读取HOST_CC/TCC，tcc私有安装不入库。 |
| 产品改动链预检 | `sh ./unisacc.com tools/chain-precheck.cx -- ROOT CHAIN_OUT NEW_OUT init --files LIST`（或`--range A..B`），随后`reuse`、逐项`job`、`identity/routes/p3`、四`probe`、`slice all`、`finish` | 原链只读、副本执行与身份/路径隔离合同不变；job reads命中优先，显式非产品事实零gen，已审计无消费者资产精确路径+SHA单列dead-asset，未知仍拒绝；产品域不得伪标非产品。69/0含hash变化和同目录假文件负控。真实44182b4059..含A1的私有干净HEAD只选gen-shared-e3/gen-warnparse/gen-errorparse；control-reject仅历史镜像无live消费者，不伪造目录映射。变更分类账与审计证据可读，不缩range冒称全覆盖。 |
| 断言式单文件文本替换 | `sh ./unisacc.com tools/edit-span.cx -- FILE --text OLD NEW [OLD NEW ...]`；旧行模式`FILE FIRST LAST FIRST_TEXT LAST_TEXT REPLACEMENT`保留；独立批`check-edit-span` | 原始字节中每个非空OLD恰好一次（含自重叠计数），匹配互不重叠，按原位置替换且NEW不级联；空NEW可删除，拒绝rc2不改原件，诊断不回显OLD。普通文件保权限，同目录独占临时文件+rename只保证单文件原子可见，非并发CAS/多文件事务。55秒daily无材料批；父已观测8.63秒14项PASS、`CHECK_EDIT_SPAN failed=0`，本次登记未重跑，不冒认候选/安装资格。新增源库存由父显式运行`tests/update-tools-compile.py`生成审阅候选；pkg-gaps缺cc13–15不伪造。 |
| 仓根换成真 APE（旧脚本留为 unisacc-dev,可回退） | `sh ./unisacc.com tools/install-ape.cx -- ROOT APE SHA256 [--write]`；回退 `-- ROOT --rollback [--write]`；检查 `tools/check-install-ape.cx` | 校验整文件 sha、UNIPKG1 尾、`--version`；同目录临时文件复核后原子 rename；旧启动脚本存 `unisacc-dev`、`.dev/` 让位为 `.dev.split-prev`、记录备份 `.split-prev`。build.json 同步 `ape1`/digest/**实际 bytes**；`ALREADY --write` 修陈旧记录，干净记录不重写，不带 `--write` 不改。两源包生产入口共享 `productpkg.pp_execution_record_error` 规范校验；更新生产者自钉后旧代材料必须重新准备；批次 `install-ape` |
| win/arm64分片固定点证据检查 | `sh ./unisacc.com tools/check-win-arm64-fixedpoint.cx -- MANIFEST [--test-loader LOADER PROBE]` | 必需 `root` 为 win 实际源根、`native_root` 为 Linux 构造器实际源根；双方收据均核同一 rev，另核候选/包/两代构造器与收据SHA、AA64、独立产物字节；缺代际rc3，坏PE/身份rc1；负控批 `win-arm64-fixedpoint-controls`，不冒认Windows原生自举 |

#### 2.4 生成器（事实表 -> 生成器 -> 表）

| 动作 | 现成用法 | 说明 |
|---|---|---|
| 切片 NN 迁移清单 | `sh ./unisacc.com tools/slice-migration-gen.cx -- --write\|--check`；校验 `tools/check-slice-migration-gen.cx` | 由 `plans/unit-text.tsv` + `plans/slice-units.tsv` 生成 `plans/slice-nn-migration.md`；`--check` 核两表键 1:1、源文件存在、文档与重生成文本一致 |
| 表单元里的 SBOUT 字节反解成文本 | `sh ./unisacc.com tools/decode-sbout.cx [-- FILE] [--min N]`（批次 `decode-sbout`） | 只读；把一段 `["SBOUT",N]` 还原成 `@bytes` 标签要携带的字符串，含 `\0`/`\xNN` 转义；默认扫 `exec/pp/*.tsv`，`--min` 按段长过滤；校验 `tools/check-decode-sbout.cx` |
| 网表裸立即数换成常量引用 | `sh ./unisacc.com tools/lit-const.cx -- FILE OLD NEW [--apply]`（批次 `lit-const`） | OLD/NEW 是整条动作原文，只换完全相同的那条，找不到退出 1；配合 `tools/facts-gen.cx` 派生常量 + manifest 绑定，改后用现装 `out/rtl3/ctx/jobs/tool-{gen,tbl,net}` 重打相关网，JSON 与网 `cmp` 全等才算行为不变；校验 `tools/check-lit-const.cx` |
| 运行库函数只出一份(RT-DEDUP) | `sh ./unisacc.com tools/rt-linkage.cx -- HEADER [--apply] [--skip NAME,...]`（批次 `rt-linkage`） | 把头文件里守卫中恰好一个单行签名的运行库函数改成三态:默认 `static`(字节不变)、`RT_UNIT` 出全局定义、`RT_EXTERN` 只留 extern 声明;变参/多函数/数据/`--skip` 的守卫原样保留并列出 `LEFT`;改后用 `tools/cx-flatten.cx -- --write ROOT` 重生成 cx-flat.h;校验 `tools/check-rt-linkage.cx` |
| Windows argv容量与布局 | `sh ./unisacc.com tools/win-argv-gen.cx -- --check . enc-x86win-winargs enc-armwin-winargs lower-data`；`PRODUCT=新包产品 sh ./unisacc.com tools/check-win-argv.cx` | `WIN_ARGVA_BYTES=131080`唯一事实推导16384项加NULL；双ISA边界/向量模式/显式NULL、PE虚拟尾与漂移负控；批次`win-argv`，Linux交叉检查不等于Windows原生通过 |
| 表闭包静态预检 | `sh ./unisacc.com tools/tsv-lint.cx -- --root . --errors-only --limit 10`；`sh ./unisacc.com tools/check-tsv-lint.cx` | 查模式冲突、未定义引用、缺兜底及精确SB入口清理合同；支持根内call、有限facts/with/let/模板及确定flags。动态宏、fresh、条件和图编辑前置条件未证明者单列`UNRESOLVED`，`status=PARTIAL`且rc0仅表示无确定错误，不代替gen/新包/slice预检；批次`tsv-lint`保留三段历史失败回放与修后负控 |
| 构造前DELTA.json审计（旧工具已恢复） | `sh ./unisacc.com tools/table-lint.cx -- DELTA.json [--strict]` | 保留5596cb4310以来的total/reach/stale-r/t-keys/unwritten/maybe-unwritten等合同；`tests/delta-loader.cx`负责接受/拒绝消费回归。与新`tools/tsv-lint.cx`是不同功能，不能互相替代或共用入口 |
| 原生TSV三方合并与补丁集成 | 泛型 `use tsvx.merge3`：`tsm_merge(header,columns,keymode,base,bn,ours,on,theirs,tn,out,conflicts)`，`tsm_merge_schema` 末参数加 `int inherited_min_columns`（0严格）；`tools/integrate-patch.cx ROOT PATCH NEW_PRIVATE_CLONE [--base REV] [--prepare-only]` 或 `--resume CLONE`；详见 `tools/README.md` | 泛型严格默认：schema/列宽及首列重复key检查，整行键集合（同事件幂等）或首列键模式；0干净、1冲突、2错误，可读base/ours/theirs冲突，失败不发布部分out。历史政策只属appendmerge上层。独立55秒真实检查批次 `tsvx-merge3`（commit）、`appendmerge`、`integrate-patch`（daily）。prepare仅PREPARED非PASS，真实gate完成才GATES_ONLY，不授全产品/全批资格；原每次120秒总限额、子进程最多55秒不变。首轮历史为泛型17/0 3.36秒、领域28/0 10.09秒、integrator夹具17/0 47.16秒；函数指针schema导出编译失败后改为纯值参数，父最终纯值schema+checked-copy版本实跑泛型17/0 3.33秒、领域33/0 10.36秒、integrator夹具17/0 43.06秒 |
| arity 表与规则对拍 | `sh ./unisacc.com tools/arity-gen.cx -- --check PWD`（`--write` 重生成，`call P N M` 查单键） | `weights/arity/arity.tsv` 243 键＝proto×npar×nargs，与 `seed/gold_arity_label.c` 的 `table` 输出逐字节相等；不计入 18 网，产品 parse 尚未读它 |
| libc99posix 契约表 | `sh ./unisacc.com tools/libc99posix-gen.cx -- --check`（`--write` 生成 `cx/compiler/libc99bind.cx`）；校验 `tools/check-libc99posix.cx` | 保留原八项及 P0 十二项语义，新增 clock_gettime；bind=per-cell 时六格为 KIND:SYMBOL，Darwin 两格 host-export:clock_gettime，Linux/Windows 为 cx-own:-（原系统调用头/签名适配 shim，不直绑 QPC）。前转按完整目标名选格，不以参数数目混同 Linux/Mac；回归检查旧绑定及 clock 六格行为，不钉死总行数。win P0 导出仍按 run42196947660 双架构 UCRT 实物及 ABI/errno 约定，cx-own 核 static 体，两 win 目标 `-c -funit` 实编 |
| 目标信号宏事实 | `sh ./unisacc.com tools/signal-targets-gen.cx -- --check`（默认生成）；检查器 `tools/check-signal-targets.cx -- ROOT`（批次 `signal-targets`） | `exec/facts/signal-targets.tsv` 派生头宏，补 SIGPIPE，Darwin SIGUSR1/2 为 30/31；六目标编译核值，Windows 数字只属进程内模拟兼容空间。未改变 signal/raise 的模拟语义，也不宣称能忽略内核投递 |
| 目标时钟事实与Darwin真单调 | `sh ./unisacc.com tools/clock-targets-gen.cx -- --check`；`sh ./unisacc.com tools/check-clock-targets.cx`（批次`clock-targets`） | CLOCK_REALTIME=0合法，MONOTONIC按目标派生；Darwin宿主libSystem clock_gettime经逐格libc契约，Linux/Windows原路径不变。六格仅编译核常量；50ms单调增量及非法clockid/errno须同c46候选两Mac原生probe，不以Linux或源码检查冒认 |
| include 驱动器号绝对路径 | `sh ./unisacc.com tools/inc-abspath-gen.cx -- --check`；校验 `tools/check-inc-abspath.cx` | `exec/facts/inc-abspath.tsv` 六目标 drive_letter；INC2 字母+':' + '/'或'\\' 且事实为真才绝对 SBFIND；win=1 lnx/osx=0。改 exec/pp 图表须先跑 seed-gen pp --shared-predefines（及 tokenpp/warnpp-shared），已装 -E 不经 seed-gen |
| syscall 分派表（lower） | `sh ./unisacc.com tools/abidispatch-gen.cx -- --check`（`--write` 重生成）；网行 `tools/decide-gen.cx -- --check --each os=lnx,osx,win --each arch=x86_64,arm64 --sections weights/abi/dispatch.tsv SC SC.ask exec/lower/abidecide-result.tsv` | `weights/abi/dispatch.tsv` 522 键＝op×os×arch，是 `weights/gold/abi.tsv` 的投影（有 sysno／win 有 winimp 的 op 标自身，否则 `fail`）；lower 的 `SYSCALL.base` 不再逐 op CMP 链，而是 prelude 写 `mem[SCIX+sysid_op]=序号+1`，`SC.ask.{os}.{arch}` 一次 RLD 选 `SC.<op>`；每个 lower 网只装自己目标那一节（section os.arch）。批次 `check-decide` |

#### 2.5 诊断（度量、盘点、普查）

| 动作 | 现成用法 | 说明 |
|---|---|---|
| 模型输入影响面投影（遗留 Python） | `python3 tools/affected-model-inputs.py CONTEXT_OR_TAR [--check]`；数据 `tools/affected-model-inputs.tsv` | 不运行生成器，仅由 source-package gen 读记录推影响的模型输入；遗留 Python 依赖，待 cx 化 |
| 普查红批次 | `sh ./unisacc.com tools/census.cx -- $PWD out/census` | 无材料批次逐个实跑找回归 |
| 看组成结构树 | `sh ./unisacc.com tools/com-tree.cx` | 每段占文件百分比 |
| 模型解码产物的逐模型 sha(解码器改写的 oracle) | `sh ./unisacc.com tools/model-decode-sha.cx -- EXPLODED_DIR [--reps N]`（批次 `model-decode-sha`） | EXPLODED_DIR 由 `tools/p3-explode.cx -- PKG OUT` 产出;每个模型 inflate 后 `cxmd_decode`,把 CxModel 逐字段渲染后取 sha,打 `MDSHA` 行与总 `all=`;`--reps` 只改计时(`decode_ns`,不含 inflate);改 `cx/runtime/model_decode.cx` 前后 `all=` 必须相同;校验 `tools/check-model-decode-sha.cx` |
| 切片单元只读盘点 | `sh ./unisacc.com tools/slice-inventory.cx -- --slice DIR [--tools-root ROOT]`；`sh ./unisacc.com tools/check-slice-inventory.cx` | 核对象/构造器/容器SHA与同构造器顺序plan映射，输出字节/物理行/use反向闭包/保守标签与top15；字节降序 -> 标签 -> 用户数升序 -> 名称。`identity=VERIFIED`不证明源码与对象历史等价或产品资格；未知纯度归MIXED，批次`slice-inventory` |
| 单元分流 fan + 材料准备 | `tools/bound 55 sh ./unisacc.com tools/units-prepare.cx -- ROOT ABS_OUT SLICE MODELS VERSION CARRIERS STAGE`，STAGE 依次 `loader`、`swap`、`seal`；`sh ./unisacc.com tools/fan.cx -- -j N --env ABS_OUT/loader.env` | T2.a clean cutover：不再提供 M8 复制式选项或幂等复用；loader 创建新 OUT 实编译 test-loader，swap 实编译 p3-swap，seal 记录真实源/部件/六 carrier.N SHA，原冻结 carrier pin 仍由消费者检查。仅 loader 六批可将 CARRIERS 设为 `-` 并以 `seal-loader` 输出 loader6.env，不得冒作 fan 材料。精确 VERSION 与提取来源由调用者绑定；PREPARED 非 PASS、非同候选发布资格。RI 入口为 `tools/materials-prepare.cx -- ROOT ABS_OUT native\|chain\|seal`，每阶段外包55秒；历史16项就绪/7项未备及原始Root归属见 `plans/m7-material-spec.md`、`archive/docs-2026-11c/plans/l-tests-materials-prepared.tsv`。 |
| 切片镜像归属度量（不改产品） | `sh ./unisacc.com tools/image-contrib.cx -- --objects a.o... [--image FILE] [--src KEY=PATH]`；`sh ./unisacc.com tools/check-image-contrib.cx` | 合并tape的`tl_unit` emit按单元计数；最终镜像列恒UNATTRIBUTED（prune/lower/elf丢来源）。分类TRANSFORM/FINITE/UNBOUNDED/SYS-IO来自源码扫描不是对象体积。批次`image-contrib`。试点设计`archive/docs-2026-11c/plans/slice-shrink.md`，fable评审前不动产品 |
| 生成代码运行时性能三档对照 | `sh ./unisacc.com tools/check-bench-gencode.cx -- ROOT [N] [1\|8]` | 同一 `tools/bench-gencode.c` 用 unisacc / gcc -O0 / gcc -O2 各编，1MB+8MB 两档，每项 N 次取中位与 cv；三档 RESULT 校验值必须逐字节相同，否则该项 INVALID；cv>10% 标 NOISY。批次 `cx-bench-gencode` |
| libc99posix 缺口扫描 | `sh ./unisacc.com tools/libc99posix-audit.cx`；校验 `tools/check-libc99posix-audit.cx` | 扫 include/*.h 的 static 函数名对照契约表；GAP 按头文件；UCRT? 为下划线导出候选(未读 PE) |
| 未初始化局部扫描 | `sh ./unisacc.com tools/uninit-scan.cx [-- FILE...]`；校验 `tools/check-uninit-scan.cx` | 流敏感 UNINIT/SUSPECT；正例 `int n=0, owner;`（e91eb31bcf^ kernel.cx）；第一版清单不进棘轮 |
| 改表验证闭环（A12） | `sh ./unisacc.com tools/a12-cycle.cx -- [--dry-run] [--only STEP] [--build B] [--flags F] [--src C [--reject]] [--neg43 EXE]` | gen -> tbl -> net -> UNINETB1(bin) -> e2/e1/sim 探针 -> decide CTX/RTC -> check-decide -> ratchets -> 冻结43例；G=B/tools 由 main 构造，缺则具名 MISSING；每步 `STEP name rc s receipt`，首败即停；UNISACC_WAVE_REUSE=1 仅子进程；检查 tools/check-a12-cycle.cx（批次 a12-cycle） |
| 0.0.55 新工具:链相位表生成 | `sh ./unisacc.com tools/chain-phases-gen.cx -- --check\|--write`；校验 `tools/check-chain-phases-gen.cx` | 由事实生成 build/ 链相位元数据(编排相位不进模型事实),`--check` 比规范字节 |
| 0.0.55 新工具:R2 无界调用改有界 | `sh ./unisacc.com tools/r2-bound.cx -- FILE LINE [LINE...]`；校验 `tools/check-r2-bound.cx` | 把指定行无界 sy_run/cx_run/sy_spawn_pipe 改写为 50s 有界并按需补帮手;R2 全仓清理用 |
| 0.0.55 新工具:相位测量 | `sh ./unisacc.com tools/cx-phase.cx -- OUTDIR [--case u1\|u2\|u6\|u12] [--parse FILE]`；校验 `tools/check-cx-phase.cx` | 跑 1/2/6/12 单元用例,读 UNISACC_RR_TIME,按 stage 汇总冷/半冷/热 |
| 0.0.55 新工具:pins 锚迁移 | `sh ./unisacc.com tools/pins-reanchor.cx [--root DIR] [--apply]`；校验 `tools/check-pins-reanchor.cx` | 把 tools/pins.tsv 的 FILE:LINE 迁为 FILE#内容锚,默认只报告 |
| 0.0.55 新工具:棘轮未声明读审计 | `sh ./unisacc.com tools/ratchet-read-audit.cx -- --check [ROOT]`；校验 `tools/check-ratchet-read-audit.cx` | 对十三个棘轮各跑两次 `--check`,找出未声明的读取 |
| 0.0.55 新工具:链批量键校验 | `sh ./unisacc.com tools/check-chain-batchkeys.cx` | 核 chain-step 批量键(fixedpoint `--objects K1,K2,...`)与单键 `--object` 同一路径 |
| 0.0.55 chain-step 批量对象 | `sh ./unisacc.com tools/chain-step.cx -- fixedpoint ... --objects K1,K2,...` | 按序对每键走与单键 `--object` 同一 cs_fixedpoint,一次启动多键 |
| 0.0.55 entry-build 收据头 | `sh ./unisacc.com tools/entry-build.cx ...` | receipt.txt 头只写一次:既有头与本次逐字相同不重写,可续跑 |
| 0.0.58 集成门前快检 S2 | `sh ./unisacc.com tools/integrate-patch.cx -- ROOT PATCH DEST --stop-after N`（N为1至5；`--resume DEST --stop-after N` 续停） | 跑到第N个gate阶段后只打印 `INTEGRATE_PREFLIGHT status=PREFLIGHT_OK`，不写ready.tsv/READY；N越界报 `stop-after-range`；批次 `integrate-patch-preflight-stop/-resume` |
| 0.0.55 integrate-patch prepare 续跑 | `sh ./unisacc.com tools/integrate-patch.cx prepare ...`（中断后同命令重跑） | tree 冻结后每个工具编译完记 .git/integrate/prepare-do,续跑跳过已完成项;`--land CLONE` 落提交 |
| 0.0.55 s9-nightly 续跑 | `sh ./unisacc.com tools/s9-nightly.cx -- ROOT RUN --tier commit\|daily --resume`（在途,未入 main,以 s9-nightly-fake 夹具与 check-s9-nightly 为准,落地后核实） | 夜跑中断后按已完成批次续跑 |
| 0.0.55 selfcheck / slot | 未作为独立 tools 落地:selfcheck 为 D1 的 tests/*.cx 棘轮接口调整;slot 未见于 v0.0.54..main | 勿当现成工具调用 |



纯 cx 密码学（无宿主库，均过向量）：`crypto.sha256`、`crypto.b64`、`crypto.hkdf`、`crypto.chacha20poly1305`、`crypto.gcm`（AES-128/256）、`crypto.ct`、`crypto.x25519`、`crypto.bn`、`crypto.der`、`crypto.p256`、`crypto.rsa`、`crypto.x509`、`crypto.tls13`、`crypto.record`——用 `#pragma unisa use crypto.<模块>`。

完整入口清单（含每条的说明、既有依据与覆盖边界）见 [archive/plans/2026-11-01/prd-c43-full-2026-11-01.md](archive/plans/2026-11-01/prd-c43-full-2026-11-01.md) 的「已实现能力速查」；这里只列常用入口，具体输入结果与覆盖边界以实际核对为准。

<a id="model-function-bytes"></a>
## 3. 结构组成（unisacc.com 的内部）

本节是全部结构点的**唯一权威定义**。取数口径：2026-11-11 在 HEAD `6a4fa1fa6f` 的私有克隆、用已装 0.0.52 APE（sha `1bffabba…`，6,529,460 B；P4 包与 0.0.53 同为 `56efea53`）实跑，每个数后括号写命令或出处；带版本号的耗时取自 `agentwork/phase-times.tsv`（多为 0.0.49 候选 979b56e4 的实测，0.0.52/0.0.53 未重测处照写），「未测」即本次没有命令证据。生成块 §9.6 由 `tools/product-map.cx` 重算，与本节冲突时以生成块的字节为准。

### 3.0 APE 实测（`sh ./unisacc.com tools/com-tree.cx -- --models`，6.6s）

```
unisacc.com  6,529,460 B  sha256 1bffabba…（0.0.52）
├─ head: PE 镜像 + sh 启动器          2,760,192 (42.2%)  win/x86_64 片原样存；DOS stub 放 sh 启动器
├─ slice lnx/x86_64                     383,136 ( 5.8%)  gzip -> 1,943,158 B
├─ slice lnx/arm64                      362,265 ( 5.5%)  gzip -> 1,951,302 B
├─ slice osx/x86_64                     397,346 ( 6.0%)  gzip -> 2,493,538 B
├─ slice osx/arm64                      392,945 ( 6.0%)  gzip -> 3,170,530 B
├─ (padding 合计 7+14+469,919)          469,940 ( 7.1%)  含两份 osx cache-manager（gzip 28,090 / 28,670 B，启动器 z= 值）
└─ package (P4)                       1,763,636 (27.0%)  40 模型 + 1823 路由行 + 77 资源 + 16 B UNIPKG1 尾
   ├─ stage routing (D rows)             104,052
   ├─ models（deflate）                1,415,344         最大 model 34 / 4（e3）155,182 / 151,577
   └─ resources                          244,209         @hdr/* 69 个 227,391（cx-flat.h 101,204）；@kernel/* 2 个 14,395；@predefines/* 6 个 432
```

### 3.1 结构五层总表

| 结构层 | 功能 | 组成（真源） | 实测字节 | 构造方式 | 测试入口 | 当前耗时 | 瓶颈与提速点 |
|---|---|---|---|---|---|---|---|
| **L0 loader** | 识别 OS/架构 -> 取对应片 -> 解到用户缓存 -> exec；Windows 由 PE 头直接执行 | APE 头 DOS stub 内 sh 启动器、osx cache-manager；`seed/ape.c`、`ujs/product/ape-pack.cx` | 启动器文本未单独计（未测）；头 2,760,192 B 其实是 win/x86_64 片 | 候选链 pack 相位（`release-run --pack`） | `check-install-ape`、`release-ape` smoke、`check-dev-product` | `--version` 6-7 ms（3 次）；pack 15 s（0.0.47 自报，未重测） | 头 42.2% 是未压缩的片、padding 7.1%；S1 评估 gzip 后头约 391,110 B（0.0.47 数据），未做 |
| **L1 base（六片）** | 每目标一份原生机器码：执行器、包解码、sys 薄层、驱动 | 见 §3.2；源 = cx-L0 + cx-L1 + `src/` | 六片解压合计见 §3.2 | 候选链 gen1 -> g2 -> g3（85 对象 / 片） | 原生 CI 六宿主 42 格、`check-loader-isa`、`slice-freeze` | lnx 片 49-61 s/代；osx 片八子相合计 142.4 s/代；win 片 118.5 s/代（0.0.49，phase-times 55-57、95-118 行相加） | osx stub 相 39.5 s、win e3 相 35.7-38.1 s 贴 45 s 闸；三代 x 六片大多串行 |
| **L2 program** | 平台无关的驱动与编译器模块（cxmod/route/frames/cache/cli） | `src/` 25 个 .cx、`cx/driver`、`cx/compiler`（cx-L2） | 203,081 + 100,918 + 397,580 B 源码（`git ls-files` + `stat`） | 现状：编进每一片（未单独分发；2026-11-07 决策不列主线） | 13 棘轮、`check-cx-layers`、`coreapi`、`srcshape` | 13 棘轮 27.8 s（`tools/ratchets.cx`）；9 单元工具冷编 6.7 s、热 0.8 s（com-tree.cx） | 每单元重复展开 cx.h 前导（9 次 e2 共 1.57 s）；单元前端缓存未落地 |
| **L3 nets** | 全部表状决策与动作程序；七流按路由取模型执行 | P4 包；源 = `exec/`（1520 文件）、`weights/`、`seed/`；见 §3.3-§3.6 | 包 1,763,636 B；源 exec 6,516,118 / weights 502,143 / seed 963,731 B | 131 作业（5 tool + 42 gen + 42 tbl + 42 net）-> routes -> p3 | 42 网全键 `net-check`、`check-decide`、`check-loader-*`、`a12-cycle` | 131 作业波 276.9 s、routes 29.7 s、P4 58.8 s（0.0.49）；42 网全键并行 1.45 s / 串行 6.97 s（0.0.51） | 改一张共享 facts 波及 28-32 网；增量构造只有演练（§3.4），未接候选链 |
| **L4 runtime** | 用户程序的运行库与标准头（static 头内实现） | `include/` 69 文件 -> 包内 `@hdr/*` | 源 852,766 B；包内 227,391 B | `tools/cx-flatten.cx` 生成 cx-flat.h，随 p3 打包 | `check-libc-probes`、`check-stdio`、`libc99posix-gen --check` | 未测 | hello.c 60 B 经 e2 展成 122,037 B；每个 .o 各带一份运行库（约 264 KB，0.0.46 数据，未重测） |

### 3.2 六片（L1 base 的六个实例）

| 片 | 在 APE 的位置 | 存储 / 解出字节 | 格式 |
|---|---|---|---|
| lnx/x86_64、lnx/arm64 | gzip 成员，偏移由启动器 `o= n=` 行给出 | 383,136 / 1,943,158；362,265 / 1,951,302 | ELF |
| osx/x86_64、osx/arm64 | gzip 成员，各配一份 cache-manager | 397,346 / 2,493,538；392,945 / 3,170,530 | Mach-O |
| win/x86_64 | APE 头本身（原样存，不压缩） | 2,760,192 | PE |
| win/arm64 | **不在 APE 内**，独立资产 `slice-win-arm64` | 3,485,184（0.0.52 发布说明；本次未测） | PE |

真源：各片 sha 在 `release/candidate.tsv` 与生成块 slice 行。片内字节到模块的归属仍为 UNATTRIBUTED（module-byte-map 非排他）。构造、测试、耗时见 §3.1 L1 行与 §3.7。

### 3.3 流（7 + 1）与流接口格式

七流真源 `exec/pipeline/image-stages.tsv`（7 行，生成 `cx/compiler/streamname.cx`）。「+ 1」是 cx 侧的 **CX0**（在 e2 之前：`.cx` -> 模块规划 / 接口 -> 平铺 C），目前由手写规划器执行、不是模型流，定义与现状只在 [prd_cx.md](prd_cx.md) §1。包内另有三个不计入流数的辅助阶段：units（模型 35，只在 `multi/*` 路由，144 行，合并多单元帧）、tapecheck（288 行，tape 域校验）、sidecar / frame（流间规整与装帧）。

| 流 | 输入 -> 输出 | 包内模型（raw 字节） | 构造配方数 | 读金表 | hello.c 单流（中位，3 次） | 9 单元工具冷编内耗时 | 提速点 |
|---|---|---|---|---|---|---|---|
| e2 | `src.c` -> `pp.text` | 0、3（474,912） | pp 3 | pp | 123 ms | 1568.5 ms（9 次） | 头展开按单元重复；最大可缓存项 |
| e1 | `pp.text` -> `tokens.typed` | 1（246,470） | lex 3 | lex、lexcls、lexword | 51 ms | 259.5 ms（9 次） | 随 e2 缓存一并省 |
| （units，辅助） | 各单元 `tokens.typed` 帧 -> 合并帧 | 35（47,101） | parse2/units 1 | - | 未测（单源不走） | 242.3 ms | - |
| e3 | `tokens.typed` -> `tape.text` | 4、34（1,095,431） | parse2 3 | type、irsel、tyinfo、binsel、prec（parse、scope、pfconv 为对照臂） | 199 ms | 1881.8 ms | 单次最大相位；整程序一遍，无按单元检查点 |
| e4 | `tape.text` -> `tape.text` | 6、7（35,194） | opt 2 | opinfo、peep | 466 ms（O2 路由，单次） | 未走（O0） | 仅 O1/O2 路由 |
| prune | `tape.text` -> `tape.text` | 8（25,627） | prune 1 | 无 | 114 ms | 325.0 ms | - |
| lower | `tape.text` -> `target.text` | 12 个（2,029,794） | lower 13（含 hostsymbols） | abi、regmap、reloc | 125 ms | 620.2 ms | 每目标 image / object 各一模型 |
| elf | `target.text` -> `image.bytes` | 12 个（994,216） | enc 12 | enc | 73 ms | 355.6 ms | - |

口径：单流耗时 = `unisacc stream STAGE --route lnx/x86_64/{run,image}/O0` 逐段实跑，管道产物与 `-o` 逐字节相同（`cmp` 0）；冷编列 = `UNISACC_RUN_CACHE=0 UNISACC_RR_TIME=1` 跑 `tools/com-tree.cx` 的 inference 行求和（推理合计 5253 ms，模型 inflate 267 ms，帧材料写 256 ms，总墙钟 6.7 s）。单流测试入口：`unisacc stream STAGE`（§2.2）、`tests/stages/<段>/` + `tests/check-stages.cx`（实跑 10.7 s，`failed=13`，自 0.0.51 起红，未修）、`tests/check-stream-pipe.cx`、七张 `stream-*` 域矩阵。

**流接口格式**（真源 `exec/pipeline/formats.md` 与 image-stages.tsv 的两列；hello.c 实测字节）：`pp.text` 122,037 -> `tokens.typed` 183,537 -> `tape.text` 178,986（e3 出）-> sidecar 后 178,955 -> `target.text` 152,503 -> `image.bytes` 36,894。属性表随流（USLATTRT），多单元用 `stream frame --in L=F...` 装帧，`stream sidecar` 把 e3 输出规整成后段输入。格式由各流模型自己强制（如 E3 token trailer），没有独立的格式校验器批次（`formats-*.cx` 只核目标文件格式）。

### 3.4 网、模型与阶段路由

四个口径，互不相加、不得互相换算：

| 口径 | 数量（实测） | 真源 | 构造生成器 | 验证入口 | 构造耗时 |
|---|---|---|---|---|---|
| 金表网（ask 事实表） | **18**（`exec/pipeline/gold-stages.tsv` 18 行） | `weights/gold/*.tsv` | 规则形式 `seed/gold_<名>_label.c` | `tools/gold-table.cx --check` | 未测 |
| cx 侧网模块 | **18 + 2 = 20**（`ls cx/compiler/net`；多出的 lexcls、lexword 是声明表，无独立 ask 网）；全是 `return 0` 骨架，定位见 [prd_cx.md](prd_cx.md) §2 | `weights/gold/` 20 个文件，254,164 B，非注释行合计 9062（含表头，未剔） | `tools/surface-gen.cx`、`declared-gen.cx` | 各生成器 `--check` | 未测 |
| 金表外决策表 | 3 张：stmt 107 行、arity 244 行、abi/dispatch 523 行（非注释行） | `weights/stmt`、`weights/arity`、`weights/abi` | `tools/decide-gen.cx`、`arity-gen`、`abidispatch-gen` | 批次 `check-decide` | 未测 |
| 构造网（配方） | **42**（`tools/source-package.cx` recipes[]；`net-deps --summary`：42 网、闭包 938 文件） | `exec/**` 表 + facts | `seed/gen.c` construct_*，每网 gen -> tbl -> net 三作业 | `tools/lab-loader/net-check.cx` 全键；`tools/net-deps.cx` 波及面 | 131 作业波 276.9 s（0.0.49） |
| 模型（kind N，包内执行单元） | **40**：32 个流模型（e2 2、e1 1、e3 2、e4 2、prune 1、lower 12、elf 12）+ 8 个公共（5 tapecheck、32 tokenpp、33 tokenlex、35 units、36 nativeabi、37 symbols、38 contract、39 ownership） | P4 包 | routes + p3 相位 | `check-loader-nets / common / special / targets`、`model-decode-sha` | routes 29.7 s + P4 58.8 s（0.0.49） |
| 阶段路由 | **323 条路由 / 1823 D 行**（`com-tree --stages`）；77 资源 | P4 包 D 行 | routes 相位 | `check-stream-target` | 同上 |

关系：18 张金表里 13 张是七流出货主脊（pp lex type irsel enc reloc tyinfo regmap abi binsel prec opinfo peep，改一键必改对应构造网），5 张是对照臂（combo isel parse pfconv scope，42 张构造网不读，仍进 `weights/built.json` 与 `kernel/*.inc`）；42 配方到 40 模型不是一一对应，包内没有这张映射（生成块记 UNMEASURED）。配方按生成器阶段：pp 3、lex 3、parse2 4、opt 2、prune 1、lower 13、enc 12、nativeabi / tapecheck / librarycontract / librarypartition 各 1。

**增量构造（BASE-6）现状**：只有隔离演练 `tools/net-incremental-rehearsal.cx`（改 pp 一处 -> 重建 3 网、复用 39 网，构造 12.07 s 对全量 175.36 s，验证 2.57 s 对 21.57 s，产物逐字节同全量）；没有接进候选链，0.0.52 的 A1「改一张表到可装候选 <= 5 分钟」未过门。共享度最高的输入：`exec/facts/tokenlocations.tsv` 32 网、`parse-constants.tsv` 30 网。

### 3.5 执行器（UNIKERN1）

通用执行器只提供机制（定宽运算、栈 / 存储、字节 / 帧、资源、接受 / 拒绝），语义全在模型里。两份实现：汇编内核（包内 `@kernel/x86_64` 7,222 B、`@kernel/arm64` 7,173 B，由 `cx/sys/kernel.cx` 6,143 B 装载）与 CX 参考实现 `cx/core/execute.cx`（14,233 B）；`src/executor.cx` 调 `core_run`。经典 18 网的 `kernel/` 41 个生成文件（4,944,716 B）禁止手改。构造：候选链 kernels-g2 20.3 s、kernels-g3 9.3 s（0.0.49）。测试：`check-core-run-diff`（两实现逐字节）、`tests/coreapi.cx`、`model-decode-sha`。瓶颈：推理占冷编墙钟约 78%（5253 / 6705 ms）；每输入字节 8 B 属性，32 MiB 输入约放大到 256 MiB（items `CORE-INPUT-MEMORY-OOM`）。

### 3.6 事实表与生成器

范式定义见 §0.1；这里只写规模与入口。`exec/facts/` 240 文件 1,449,170 B，`exec/` 全部 1520 文件；改动一律「事实表 -> 生成器（带 `--check`）-> 表 / 网」。生成器：`seed/gen.c`（网）、`tools/facts-gen.cx`（常量）、`tools/decide-gen.cx`（决策表 -> 网行）、其余 `tools/*-gen.cx`（§2.4）。测试入口：`tools/tsv-lint.cx`、`tools/table-lint.cx`、`check-decide`、`check-literal`、`check-limits`、`a12-cycle`。耗时：改表 -> `p3-swap --model` -> `stream --models` 约 8 s（§2.2 记录，未重测）。提速点：`net-deps --touch FILE` 先算波及网，再只重建那几张。

### 3.7 构造链相位、身份、两代不动点

`tools/candidate-chain.cx -- --plan`（实跑 15.1 s）给出 **35 个相位**，子进程一律 55 s 上界：snap、build、ctx、host、list、warmup、jobs-tool、jobs-gen、jobs-tbl、jobs-net、final、routes、p3、gen1 x 6 片、g2 / g3 的 lnx 两片、kernels-g2、kernels-g3、g2 / g3 的 osx 与 win 四片、compare、pack。构造图事实表 `build/graph.tsv`（非注释行 207，含表头）由 `tools/graph-check.cx` 核对。

| 相位组 | 耗时（出处） | 备注 |
|---|---|---|
| snap / ctx / build(controller) / host / list | 0.1-16.8 / 41.2 / 42.8 / 8.8 / 6.2 s（0.0.49） | ctx 曾因 8 s 内界失败一次 |
| jobs-*（131 作业） | 276.9 s | 单项最大 |
| routes / p3 | 29.7 / 58.8 s | p3 贴 60 s |
| gen1、g2、g3 每片 | lnx 49-61 s；osx 142.4 s；win 118.5 s | 三代 x 六片 |
| kernels-g2 / g3、compare、pack | 20.3 / 9.3 / 6.5 / 15 s（pack 为 0.0.47 自报） | - |
| 整链墙钟（冻结 -> 可发布） | 0.0.47 为 440 分钟，0.0.49 十余小时；0.0.52 逐相位手工续跑且有 5 处手工介入，墙钟未测；0.0.53 复用链产物未重跑 | 铁律上限 2 小时 |

**身份**：源 SID（`seed/ident.c`，目录集 exec src kernel include weights seed cx，记在 build.json 的 sources_sha256）-> 作业身份 / kernel context（独立分支，判内核可否复用）-> P4 包 sha -> APE sha；派生记录有 `unisacc.com.build.json`、`ape-receipt.txt`、`plans/root-identity.md` 身份块、`release/candidate.tsv`。「产品身份 = manifest 的 sha256（Merkle 根）」是 2026-11-02 定的方向，尚未收敛（items `IDENTITY-DERIVE` 在途）；校对入口 `tools/identity-derive.cx --check`。**两代不动点**：六片 g1 == g2 == g3、kernels g2 == g3，`fp-compare` 85 对象 / 2 内核 / 6 片 / 产品逐字节相同（6.5 s）；只证不动点，不证语义。0.0.53 没有重跑两代。

### 3.8 测试分层、批次档、棘轮、集成门、原生矩阵

| 结构点 | 数量（`tests/run.cx -- --list` 6.4 s 及表列统计） | 入口 | 耗时 | 提速点 |
|---|---|---|---|---|
| 批次注册表 | 831 批（IMPLEMENTED 819 / PLANNED 12）；有材料 261，unprepared 249 | `tests/batches.tsv` | - | 材料准备前移并可复用 |
| 批次层 | interfaces 364、contract 329、negative 70、product 29、properties 15、library 13、platform 11 | layer 列 | - | - |
| 覆盖行 | 662（实现 651、unresolved 6）；层：interfaces 327、contract 227、product 42、negative 34、library 16、properties 9、platform 7 | `tests/coverage.tsv` | - | - |
| 批次档 | commit 140 / daily 650 / release 41 | tier 列；`tools/s9-shards.cx`、`s9-nightly.cx` | commit 档 3 步 252.7 / 87.3 / 12.7 s（0.0.53 夜跑工具修后）；daily、release 档未跑 | 首步贴 300 s；daily 从未跑通一次 |
| 冒烟 | 16 断言 | `tests/run.cx -- --smoke` | 0.04 s | - |
| 棘轮 | 13 项，13 / 0 | `tools/ratchets.cx` | 27.8 s（墙钟 33.7 s） | 按改动路径只跑相关项 |
| 集成门 | 私有克隆三方应用 + 棘轮 + affected 批次 | `tools/integrate-patch.cx`、`tools/affected.cx` | 每补丁 81-95 s（0.0.48 实测，未重测）；affected 批约 42 s | phase 5 与 prepare 编译步（`prepare-done.tsv`，并行波次）已可续跑；棘轮与 native 缓存可增量 |
| 域矩阵 | 19 张 | `tests/full-domain.cx`、`check-roll-up-open` | 未测 | - |
| 原生矩阵 | 六宿主 42 格，0.0.53 QUALIFIED 42 / 0（运行 42914663896） | `.github/workflows/candidate-native.yml`、`tests/candidate-qualify.cx` | 约 10 分钟（0.0.50 账本） | 不在本机关键路径 |

判定规则（QUALIFIED、域矩阵格状态、豁免）见 §7。

<a id="pipeline-design"></a>
## 4. 架构：模型流水线

<a id="flow-net-contracts"></a>
### 4.0 七流（字节管道）

定义、规模、耗时见 §3.3（唯一权威）。各流做什么：e2 预处理（续行 / 注释 / 指令 / 宏 / 条件 / include）；e1 词法（最长匹配）；e3 语法 + 类型等合成段，产出 tape；e4 opt（窥孔、栈 -> 寄存器类改写）；prune 按根 / 导出剪不可达整函数（政策在代码，不读金表）；lower 把 ABI / regmap / syscall / 布局 / reloc 降到目标 ops；elf 编码并写出 ELF / Mach-O / PE。

### 4.1 十八网（ask 事实表）

口径、数量与主脊 / 对照臂划分见 §3.4（唯一权威）。补充依据：对照臂的判定来自变异实验（改一键 0 张构造网变化，plans/gold-mutation-2026-11-05.md），它们供 18 网经典批次与 gold 对照工具使用，不删；parse 的语句分派已由 `weights/stmt/stmt.tsv` 决策表替代。

### 4.2 构造链

```
有限表（.tsv）-> 生成器（seed/gen.c、tools/*-gen.cx，带 --check）-> 网与生成程序 -> P4 包 -> 通用执行器（UNIKERN1）
```

- 有限表只经「事实表 -> 生成器（--check）-> 表」修改，不手改（§3.6）。
- 执行器、模型包与路由的规模和入口见 §3.4、§3.5；装载由 `cx/runtime/model_decode.cx`、`src/packmodel.cx` 处理。
- 相位、身份与两代不动点见 §3.7。

### 4.3 外置与内容寻址(架构方向,2026-11-02 主人定;层名已并入 §3.1 结构五层)

2026-11-02 的四分法(booter / 基座 / 阶段权重 / 资源)在 2026-11-07 改成 §3.1 的结构五层(archive/docs-2026-11c/plans/structure-5-layer.md);旧图的编号不再使用。仍然有效的方向:

- **一切按内容哈希寻址;内置 pack 只是 resolver 的另一个来源**,与外部目录行为完全一致。
- `manifest.tsv`:`name kind(base|stage|resource) arch sha256 size order`。来源顺序:`UNISA_HOME` -> com 旁目录 -> 用户缓存 -> 内嵌 pack(UNIPKG1 footer)。命中必校验 sha256,不符具名拒绝,**不静默回退**到别的来源。
- **产品身份 = manifest 的 sha256(Merkle 根)**。取代 `pp_product_digest` / `ape-receipt` / `build.json` artifact 段等多处各算一份的口径(见 `plans/identity-inventory-2026-11-02.md`、`plans/identity-unify-2026-11-02.md`)。kernel context 仍独立于它(判断 kernel 可否复用,不随全树变),两代 GEN 互证仍保留。
- 目的:每一层单独固化、单独开发、单独测试;阶段测试=输入流字节 -> 输出流字节的 golden;换基座不动权重、换权重不动基座;慢车道缩成"只重算被改文件及其下游哈希"。
- 单文件发布(pack 内嵌)保留;外部目录形态用于开发。启动开销是主要风险(现热 `-run` ≈10ms),每步测量。
- 进度与分步:见 `agentwork/items.tsv`(B0 P3 explode/implode -> B1 resolver -> B2 阶段测试台 -> B3 基座外置 -> B4 booter APE 头 -> B5 身份收敛到 manifest)。
- 现状:基座 / 模型 / 资源的外置与 loader 分离(items B3、B4、B5c 均 DONE-PARTIAL)未完成,发布形态仍是单文件 APE(尾部一份包);已落地的是 resolver / explode 工具与 `unisacc stream STAGE` 阶段过滤器。

## 5. 源码结构（src/ 六层 + cx/ 树）

`src/unisacc.cx` 是入口，`use drive`；`src/` 现有 25 个 .cx、5854 行（`tests/srcshape.tsv` TOTAL，棘轮只降不升）、203,081 B。cx 模块树、包层、层向规则的权威定义在 [prd_cx.md](prd_cx.md) §3，这里只给 src 六层及其到结构五层的对齐：

| src 层 | 文件 | 职责 | 模块层 | 结构层 |
|---|---|---|---|---|
| host | `host.cx`、`hostnative.cx`、`hostout.cx`、`hostres.cx` | 平台胶水（mmap / dlopen / 进程） | cx-L0（引导期两文件） | L1 base |
| packmodel | `packmodel.cx` | 装载 P4 包、解压模型 | cx-L2 | L1 base |
| executor | `executor.cx` | 调 `core_run`（§3.5） | cx-L2 | L1 base |
| stages | `stages.cx`、`stream.cx` | 按路由装模型交给 `core_run`；`stream STAGE` 过滤器 | cx-L2 | L2 program（执行 L3 nets） |
| drive | `drive*.cx` 11 个、`runcache.cx`、`network.cx` | 驱动、路由、多单元、运行缓存 | cx-L2 | L2 program |
| bind | `bind.cx`、`entrybind.cx`、`entryexec.cx` | 入口绑定、晚绑定 | cx-L2 | L2 program |

cx 目录到结构层：`cx/sys mem text`（cx-L0）与 `cx/core runtime`（cx-L1）属 L1 base；`cx/driver compiler`（cx-L2）属 L2 program；`exec/ weights/ seed/` 属 L3 nets；`include/` 属 L4 runtime；`cx/cx0 workspace` 是 CX0（prd_cx.md §1）；`ext/` 不随产品发布。现状是 L1 与 L2 一起编进每一片，结构层是功能划分，不是已分开的文件。

分层铁律：cx-L0 才对接 libc/POSIX，上层禁 `#include` 和裸 libc（三个只降不升的棘轮，见 prd_cx.md §3）。源 SID 与 `ext/` 符号链接的已核事实同在该节。

## 6. 事实表与权重（exec/ 与 weights/ 与 seed/）

规模、生成器与测试入口见 §3.6。目录分工：`exec/facts/` 是各阶段常量事实（经 `tools/facts-gen.cx`）；`exec/` 其余子目录（enc/lex/lower/opt/parse/pp/prune/tapecheck…）是各阶段网的表与构造，`exec/c/` 是旧 C 参考，只供对照；`weights/gold/` 是金表，`weights/*.f32/i8.unisa` 是经典 18 网构造出的权重；`seed/` 是 C 种子构造器（`gold_<名>_label.c` 规则形式、`gen.c` 网构造、`ident.c` 身份、`ape.c` 打包）。

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

七层及含义：contract（语言 / 流 / 网合同，正反例、拒绝）、interfaces（流间接口、模块接口、cx-L0 接口）、negative（具名拒绝、坏输入、语义级坏包）、product（端到端编译 / 运行 / 镜像字节）、library（库 / ABI）、platform（六目标）、properties（优化性质）。各层行数、批次数、分档与耗时见 §3.8（2026-11-11 实测）；数字随注册表变化，以 `--list` 为准，`--list` 不执行批次（`qualification=NOT_EXECUTED`）。

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
| `cx/` | `.cx` 模块树（层与包层见 [prd_cx.md](prd_cx.md) §3） |
| `exec/` | 各阶段事实与构造（facts/enc/lex/lower/opt/parse/pp/prune/tapecheck；c/ 旧 C 参考） |
| `weights/` | 十八网金表 + 构造权重 |
| `seed/` | C 种子构造器（gold label 规则、ident.c 身份、ape 打包） |
| `include/` | 头文件（cx.h 前导 + 69 个 std 头） |
| `tests/` | 测试套件（batches/coverage 注册表 + 19 域矩阵 + 各层测试 .cx） |
| `tools/` | 开发工具（733 个 .cx，`git ls-files 'tools/*.cx'` 2026-11-11：生成器/棘轮/refactor/symindex/all-waves…）；`tools/cxlib/` 为工具共享库（rechunk、netpack、resolver 等，不进源 SID） |
| `ext/` | 扩展包：sysx、memx、textx、crypto、tsvx（TSV 读写与 merge3）；不进源 SID，memx/textx 经 cx/mem、cx/text 符号链接间接参与 |
| `build/` | 构造图与前提规则表：graph.tsv（构造图，规模见 §3.7）、hardwire.tsv（第二真值硬连线模式）、preflight-rules.tsv（0.0.50 撞到的隐性前提，0.0.51 PF 预检输入）；构造链维护者所有，不进源 SID |
| `plans/` | 计划与设计：各版 v0.0.4x / 5x-unisacc.md（最新 v0.0.58）、structure-5-layer.md、standing-goal.md、release-notes-0.0.46 至 0.0.48 与 0.0.53 至 0.0.55；已完成的归档到 archive/plans，2026-11 文档清理移走的（含 release-notes-0.0.45、0.0.48.1、0.0.49 至 0.0.52）在 archive/docs-2026-11/plans |
| `docs/` | 规格/术语与目录地图（docs/REPO-MAP.md、glossary.md、toolchain.md、tapebin-v1*、docs/cx/；spec.md 在根） |
| `research/` | 论文五线（A 流网换逻辑、B ujs、C 管道、D cx 模块化、E 系统构表） |
| `archive/` | 归档：reference-c/reference-py（旧实现）、testing-c43-legacy（旧 1333 项套件）、plans/（历史计划）、docs-2026-11/（2026-11 文档清理移入的 plans 与 research，索引 INDEX.md）、prd 历史 |
| `apps/csih/` | csih 车道（归 csih 主人） |
| `ujs/` | 论文 B（归论文 B 主人，unisacc 侧不碰） |
| `adapter/`、`m4/`、`flownet/` | 流×网范式的适配/m4/通用流水线 |
| `out/` | 临时产物（gitignore，日志/候选/材料/会话）；交接单约定写 `out/<窗口>-handoff-*.md`，不入库 |
| `kernel/` | 生成物（禁止手改；权重由 gold 构造） |
| `agentwork/`、`release/`、`agentwork/roles/`、`skills/`、`bench/`、`scripts/`、`examples/`、`iterate/` | 源 SID 目录集为 exec src kernel include weights seed cx（+ iterate/kernel/typekw.tsv），其余目录均不进源 SID；任务看板与账（agentwork/：items/ledger/units/freeze/phase-times/pkg-gaps，管理账不是事实表）、发布（release/candidate.tsv、SHA256SUMS.template）、代理角色、技能、基准、脚本、示例、迭代记录；完整四组地图见 [docs/REPO-MAP.md](docs/REPO-MAP.md) |

<a id="current-plans"></a>
## 9. 当前状态与路线

<a id="work-tree"></a>
### 9.1 思维树（当前节点；每轮先读，事实/阻塞/下一动作原位更新）

**当前已发布：0.0.57**（tag `v0.0.57` -> `99fc0b48a2`，候选表 `release/candidate.tsv`，发布说明 [plans/release-notes-0.0.57.md](plans/release-notes-0.0.57.md)）。APE sha256 前缀 `c116a52f`（6,648,004 B）；构造源 `60acc817b8`，源 SID `25e41517`；整链重建一轮，链内 compare 全 same。产品字节与 0.0.56 相同，仅版本串不同；唯一产品输入改动是 basered 基线红清零。同候选六目标原生 CI 运行 43094459755 QUALIFIED 42 / 0 / 0 / 0（pass / fail / not_run / invalid）。公开无认证读回 6/6，`--version` 为 `unisacc 0.0.57`。s9 提交档实测：选 154，PASS 150、FAIL 2、UNKNOWN 2，NOT_FULL；FAIL 为 `s9-nightly-run-pass`、`s9-nightly-run-fail`（与 0.0.56 同批同检查项，s9split 的修复在真实会话未生效），UNKNOWN 为 `register-metadata-rollback`、`candidate-chain` 的 55 秒超时，四批原因未定，未修，未重跑。release-decide 步 4 记 FAIL 重裁，**全批资格 NOT_QUALIFIED**（unknown=0 fail=1）；s9 daily 713 批与 release 41 批至今没有跑通过一次。

**本机已装根**：0.0.57 APE（`c116a52f`，`install-ape` 写 `plans/root-identity.md` 身份块与 `ape-receipt.txt`）。

**当前版本：0.0.58，进行中**（计划见 `plans/v0.0.58-unisacc.md`，定稿于 2026-11-12，基线 main `1e269c53bb`）：
- 唯一产品改动 P1：规划器单源（prd_cx.md 55-1）。`cx/compiler/cxmod.cx` 由 `cx/cx0/plan.cx` 标记区经 `tools/cx0-extract.cx --target product` 生成并受逐字门约束，`CXM_MAXU` 三处同取 128 并入 `exec/facts/limits.tsv`，壳剖面表 `exec/facts/cx0-shell.tsv`。产品必然换 SID，六片不再与 0.0.57 同字节；P1 不含 55-2、55-3、55-4。
- 工具与测试：T1 G2 预算分批、T2 chain-run 逐相预算（K10）、T3 按目标 4 路并行（K9）、T4 字面锚点禁用（anchorban）、T5 发布尾部自动化、T6 资产齐全后触发 CI（assetguard）、T7 safe-ff、T8 release-tidy、T9 版本度量（metrics）、T10 本文回写、T11 续跑复核开销与分段、T12 s9 提交档四批修复（s9red）、sweep。
- 目标：冻结到公开不超过 100 分钟（硬线 120）、人工介入 0、chain-run 零接触 PASS、单次操作不超过 300 秒。即使全部达标，全批资格仍是 NOT_QUALIFIED。
- 任务以 `agentwork/items.tsv`、进度以 `agentwork/ledger.tsv`、相位耗时以 `agentwork/phase-times.tsv` 为准。

阻塞（2026-11-12）：
- **全批资格**：s9 daily / release 档未跑通过；提交档 0.0.57 为 150 / 2 / 2。
- **链零接触未验**：T1 至 T3、T11 尚未落 main，R3 整链彩排尚未做；`g2-budget-gen` 批次门内 FAIL 原因未定。
- **补丁队列冲突**：anchorban 与 g2batch 在 `tools/check-chain-argv-check.cx` 真冲突，s9red 与 k9 在 `tools/check-candidate-chain.cx` 真冲突，须按序重导。

往期发布（0.0.49 至 0.0.56）的身份与经过见各版发布说明与 [prd_release.md](prd_release.md)；2026-11-11 自本节删去的旧段落索引在 [prd_hist.md](prd_hist.md) 末节。

<a id="product-map"></a>
### 9.6 产品全景表（生成块；`tools/product-map.cx -- --write` 重算，勿手改；位置在 §9.1 与 §9.2 之间是历史原因）

<!-- product-map:begin -->
产品全景表阶段一(层+六片)与阶段二(七流+42构造网)。数字由 `tools/product-map.cx` 重算。kind 分开源码字节与目标镜像字节,禁止相加。模块字节无 origin 时 UNATTRIBUTED。

```
kind	id	parent	function	size_bytes	size_note	sha256	aux	cmd	verified_rev	cold_s	hot_s	last_s
layer	L0	-	L0 sy/mem/text	128030	source-bytes	-	dirs=cx/sys,cx/mem,cx/text	git ls-files -z -- DIR && stat bytes	1885ba30205c7e08277fd7ad4afb501d56b80972	UNMEASURED	UNMEASURED	UNMEASURED
layer	L1	-	L1 core/driver/runtime/compiler	796701	source-bytes	-	dirs=cx/core,cx/driver,cx/runtime,cx/compiler	git ls-files -z -- DIR && stat bytes	77b5e559b3822b0a5244b6305a05b26b0e0a3202	UNMEASURED	UNMEASURED	UNMEASURED
layer	src.host	src	host launch	16383	source-bytes	-	file=src/host.cx	stat src/host.cx	44182b4059d51191cb021241c666b521bf34b2ad	UNMEASURED	UNMEASURED	UNMEASURED
layer	src.packmodel	src	pack model	4925	source-bytes	-	file=src/packmodel.cx	stat src/packmodel.cx	1bf6f7e8c3af3205f5bf3127972689e7dc3f0710	UNMEASURED	UNMEASURED	UNMEASURED
layer	src.executor	src	executor	9100	source-bytes	-	file=src/executor.cx	stat src/executor.cx	292ccd4c8bc2045a2fb2754ad69b5e7cfa0df45b	UNMEASURED	UNMEASURED	UNMEASURED
layer	src.stages	src	seven flows bind	15118	source-bytes	-	file=src/stages.cx	stat src/stages.cx	fd3b9006861e5dac954ae81f28e209a6c226b032	UNMEASURED	UNMEASURED	UNMEASURED
layer	src.drive	src	drive	38842	source-bytes	-	file=src/drive.cx	stat src/drive.cx	23e6a567dd3ab5e481dbe3c98491bc87dd2a3b95	UNMEASURED	UNMEASURED	UNMEASURED
layer	src.bind	src	bind	205	source-bytes	-	file=src/bind.cx	stat src/bind.cx	d4189306a6e4872dfc274518b062342cb4a2780c	UNMEASURED	UNMEASURED	UNMEASURED
slice	lnx/x86_64	ape	embedded target image	1943158	target-image-bytes	03823301aa8984c8441b4a65ceaa5f7762c5be238991ee1af5e67d5ffb591dfc	format=ELF;unattributed_bytes=1943158;attributed_bytes=0;unattributed_frac=1;note=module-byte-map-not-exclusive	sh ./unisacc.com tools/logic-distribution.cx -- --ape unisacc.com	03823301aa8984c8441b4a65ceaa5f7762c5be238991ee1af5e67d5ffb591dfc	UNMEASURED	UNMEASURED	UNMEASURED
slice	lnx/arm64	ape	embedded target image	1951302	target-image-bytes	d8c16f40700a82fb9d50c93285caafdbb6e2b74809bf13beb19139e5c5890843	format=ELF;unattributed_bytes=1951302;attributed_bytes=0;unattributed_frac=1;note=module-byte-map-not-exclusive	sh ./unisacc.com tools/logic-distribution.cx -- --ape unisacc.com	d8c16f40700a82fb9d50c93285caafdbb6e2b74809bf13beb19139e5c5890843	UNMEASURED	UNMEASURED	UNMEASURED
slice	osx/x86_64	ape	embedded target image	2493538	target-image-bytes	54c1622f02ec77780e3a040cfe341c138d75831cae9582248d44fd5af911f808	format=MACHO;unattributed_bytes=2493538;attributed_bytes=0;unattributed_frac=1;note=module-byte-map-not-exclusive	sh ./unisacc.com tools/logic-distribution.cx -- --ape unisacc.com	54c1622f02ec77780e3a040cfe341c138d75831cae9582248d44fd5af911f808	UNMEASURED	UNMEASURED	UNMEASURED
slice	osx/arm64	ape	embedded target image	3170530	target-image-bytes	43d7dd06d40e71644ed619568a76364a89d8282a2c82d1536abc0a1de0954402	format=MACHO;unattributed_bytes=3170530;attributed_bytes=0;unattributed_frac=1;note=module-byte-map-not-exclusive	sh ./unisacc.com tools/logic-distribution.cx -- --ape unisacc.com	43d7dd06d40e71644ed619568a76364a89d8282a2c82d1536abc0a1de0954402	UNMEASURED	UNMEASURED	UNMEASURED
slice	win/x86_64	ape	embedded target image	2760192	target-image-bytes	7ba2852fa0bfd48ad196fbab3fa547a2c2f64b0dca5f78887a072f9bdfb8c68a	format=PE;unattributed_bytes=2760192;attributed_bytes=0;unattributed_frac=1;note=module-byte-map-not-exclusive	sh ./unisacc.com tools/logic-distribution.cx -- --ape unisacc.com	7ba2852fa0bfd48ad196fbab3fa547a2c2f64b0dca5f78887a072f9bdfb8c68a	UNMEASURED	UNMEASURED	UNMEASURED
slice	win/arm64	ape	embedded target image	UNMEASURED	target-image-bytes	UNMEASURED	aux=UNATTRIBUTED missing_from_ape	sh ./unisacc.com tools/logic-distribution.cx -- --ape unisacc.com	-	UNMEASURED	UNMEASURED	UNMEASURED
flow	e2	-	preprocess: continuation/comment/directive/macro/conditional/include	465	gold-table-bytes	-	in=src.c;out=pp.text;model_file=e2.net;gold_files=1;gold=pp:465	cat exec/pipeline/image-stages.tsv && stat weights/gold/*.tsv	6e50148d261c3f6df80ad7355bbb49381cba00e2	UNMEASURED	UNMEASURED	UNMEASURED
flow	e1	-	lex: keyword/identifier/operator/literal longest match	7151	gold-table-bytes	-	in=pp.text;out=tokens.typed;model_file=e1.net;gold_files=3;gold=lex:2008,lexcls:4281,lexword:862	cat exec/pipeline/image-stages.tsv && stat weights/gold/*.tsv	6e50148d261c3f6df80ad7355bbb49381cba00e2	UNMEASURED	UNMEASURED	UNMEASURED
flow	e3	-	parse+type etc. into tape	88111	gold-table-bytes	-	in=tokens.typed;out=tape.text;model_file=e3.net;gold_files=8;gold=binsel:539,irsel:6412,parse:6173,pfconv:234,prec:299,scope:897,tyinfo:429,type:73128	cat exec/pipeline/image-stages.tsv && stat weights/gold/*.tsv	6e50148d261c3f6df80ad7355bbb49381cba00e2	UNMEASURED	UNMEASURED	UNMEASURED
flow	e4	-	opt: peephole and stack-to-register rewrite	44422	gold-table-bytes	-	in=tape.text;out=tape.text;model_file=e4.net;gold_files=2;gold=opinfo:2231,peep:42191	cat exec/pipeline/image-stages.tsv && stat weights/gold/*.tsv	6e50148d261c3f6df80ad7355bbb49381cba00e2	UNMEASURED	UNMEASURED	UNMEASURED
flow	prune	-	prune unreachable whole functions by roots/exports	0	gold-table-bytes	-	in=tape.text;out=tape.text;model_file=prune.net;gold_files=0;gold=-	cat exec/pipeline/image-stages.tsv && stat weights/gold/*.tsv	6e50148d261c3f6df80ad7355bbb49381cba00e2	UNMEASURED	UNMEASURED	UNMEASURED
flow	lower	-	lowering: ABI/regmap/syscall/layout/reloc to target ops	44995	gold-table-bytes	-	in=tape.text;out=target.text;model_file=lower.net;gold_files=3;gold=abi:44254,regmap:449,reloc:292	cat exec/pipeline/image-stages.tsv && stat weights/gold/*.tsv	6e50148d261c3f6df80ad7355bbb49381cba00e2	UNMEASURED	UNMEASURED	UNMEASURED
flow	elf	-	encode and write ELF/Mach-O/PE	12491	gold-table-bytes	-	in=target.text;out=image.bytes;model_file=elf.net;gold_files=1;gold=enc:12491	cat exec/pipeline/image-stages.tsv && stat weights/gold/*.tsv	6e50148d261c3f6df80ad7355bbb49381cba00e2	UNMEASURED	UNMEASURED	UNMEASURED
gold-unassigned	combo	-	no stream in prd 4.0 flow table	50812	gold-table-bytes	-	file=weights/gold/combo.tsv	stat weights/gold/combo.tsv	6e50148d261c3f6df80ad7355bbb49381cba00e2	UNMEASURED	UNMEASURED	UNMEASURED
gold-unassigned	isel	-	no stream in prd 4.0 flow table	5717	gold-table-bytes	-	file=weights/gold/isel.tsv	stat weights/gold/isel.tsv	6e50148d261c3f6df80ad7355bbb49381cba00e2	UNMEASURED	UNMEASURED	UNMEASURED
flow-model	e2	e2	UNINETB1 models of stream e2 in package	474912	uninetb1-raw	71fc802f9a696c0d3aaac77e62dc33444674155a859fbc7d9fd647259aa9088d	models=0+3;action_bytes=259495;selector_bytes=157413;action_permille=546;selector_permille=331;tier=daily	sh ./unisacc.com tools/model-program-census.cx -- --package PKG	71fc802f9a696c0d3aaac77e62dc33444674155a	UNMEASURED	UNMEASURED	UNMEASURED
flow-model	e1	e1	UNINETB1 models of stream e1 in package	246470	uninetb1-raw	71fc802f9a696c0d3aaac77e62dc33444674155a859fbc7d9fd647259aa9088d	models=1;action_bytes=143173;selector_bytes=81571;action_permille=580;selector_permille=330;tier=daily	sh ./unisacc.com tools/model-program-census.cx -- --package PKG	71fc802f9a696c0d3aaac77e62dc33444674155a	UNMEASURED	UNMEASURED	UNMEASURED
flow-model	e3	e3	UNINETB1 models of stream e3 in package	1095431	uninetb1-raw	71fc802f9a696c0d3aaac77e62dc33444674155a859fbc7d9fd647259aa9088d	models=4+34;action_bytes=658234;selector_bytes=418015;action_permille=600;selector_permille=381;tier=daily	sh ./unisacc.com tools/model-program-census.cx -- --package PKG	71fc802f9a696c0d3aaac77e62dc33444674155a	UNMEASURED	UNMEASURED	UNMEASURED
flow-model	e4	e4	UNINETB1 models of stream e4 in package	35194	uninetb1-raw	71fc802f9a696c0d3aaac77e62dc33444674155a859fbc7d9fd647259aa9088d	models=6+7;action_bytes=12771;selector_bytes=22191;action_permille=362;selector_permille=630;tier=daily	sh ./unisacc.com tools/model-program-census.cx -- --package PKG	71fc802f9a696c0d3aaac77e62dc33444674155a	UNMEASURED	UNMEASURED	UNMEASURED
flow-model	prune	prune	UNINETB1 models of stream prune in package	25627	uninetb1-raw	71fc802f9a696c0d3aaac77e62dc33444674155a859fbc7d9fd647259aa9088d	models=8;action_bytes=12310;selector_bytes=13005;action_permille=480;selector_permille=507;tier=daily	sh ./unisacc.com tools/model-program-census.cx -- --package PKG	71fc802f9a696c0d3aaac77e62dc33444674155a	UNMEASURED	UNMEASURED	UNMEASURED
flow-model	lower	lower	UNINETB1 models of stream lower in package	2029794	uninetb1-raw	71fc802f9a696c0d3aaac77e62dc33444674155a859fbc7d9fd647259aa9088d	models=9+10+13+14+17+18+21+22+25+26+29+30;action_bytes=1356828;selector_bytes=650532;action_permille=668;selector_permille=320;tier=daily	sh ./unisacc.com tools/model-program-census.cx -- --package PKG	71fc802f9a696c0d3aaac77e62dc33444674155a	UNMEASURED	UNMEASURED	UNMEASURED
flow-model	elf	elf	UNINETB1 models of stream elf in package	994216	uninetb1-raw	71fc802f9a696c0d3aaac77e62dc33444674155a859fbc7d9fd647259aa9088d	models=2+11+12+15+16+19+20+23+24+27+28+31;action_bytes=611383;selector_bytes=367661;action_permille=614;selector_permille=369;tier=daily	sh ./unisacc.com tools/model-program-census.cx -- --package PKG	71fc802f9a696c0d3aaac77e62dc33444674155a	UNMEASURED	UNMEASURED	UNMEASURED
net	shared-e2	e2	construction net: generator pp args --shared-predefines	465	gold-table-bytes	-	stage=pp;args=--shared-predefines;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
net	shared-e1	e1	construction net: generator lex args --typed	2008	gold-table-bytes	-	stage=lex;args=--typed;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
net	shared-e3	e3	construction net: generator parse2 args --locations	-	no-direct-gold-table	-	stage=parse2;args=--locations;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
net	shared-e4	e4	construction net: generator opt args --o2	-	no-direct-gold-table	-	stage=opt;args=--o2;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
net	shared-o1	e4	construction net: generator opt args -	-	no-direct-gold-table	-	stage=opt;args=-;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
net	shared-prune	prune	construction net: generator prune args -	-	no-direct-gold-table	-	stage=prune;args=-;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
net	shared-nativeabi	lower	construction net: generator nativeabi args -	-	no-direct-gold-table	-	stage=nativeabi;args=-;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
net	lnx-arm64-lower	lower	construction net: generator lower args --full,--arm64	-	no-direct-gold-table	-	stage=lower;args=--full,--arm64;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
net	lnx-arm64-elf	elf	construction net: generator enc/arm args --elf	-	no-direct-gold-table	-	stage=enc/arm;args=--elf;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
net	lnx-x86_64-lower	lower	construction net: generator lower args --full	-	no-direct-gold-table	-	stage=lower;args=--full;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
net	lnx-x86_64-elf	elf	construction net: generator enc args --elf	12491	gold-table-bytes	-	stage=enc;args=--elf;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
net	osx-arm64-lower	lower	construction net: generator lower args --full,--osx,--arm64	-	no-direct-gold-table	-	stage=lower;args=--full,--osx,--arm64;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
net	osx-arm64-elf	elf	construction net: generator enc/arm args --macho	-	no-direct-gold-table	-	stage=enc/arm;args=--macho;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
net	osx-x86_64-lower	lower	construction net: generator lower args --full,--osx	-	no-direct-gold-table	-	stage=lower;args=--full,--osx;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
net	osx-x86_64-elf	elf	construction net: generator enc args --macho	12491	gold-table-bytes	-	stage=enc;args=--macho;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
net	win-arm64-lower	lower	construction net: generator lower args --full,--win,--arm64	-	no-direct-gold-table	-	stage=lower;args=--full,--win,--arm64;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
net	win-arm64-elf	elf	construction net: generator enc/arm args --pe	-	no-direct-gold-table	-	stage=enc/arm;args=--pe;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
net	win-x86_64-lower	lower	construction net: generator lower args --full,--win	-	no-direct-gold-table	-	stage=lower;args=--full,--win;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
net	win-x86_64-elf	elf	construction net: generator enc args --pe	12491	gold-table-bytes	-	stage=enc;args=--pe;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
net	warnparse	e3	construction net: generator parse2 args --warnings,--errors	-	no-direct-gold-table	-	stage=parse2;args=--warnings,--errors;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
net	errorparse	e3	construction net: generator parse2 args --errors	-	no-direct-gold-table	-	stage=parse2;args=--errors;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
net	warnunits	e3	construction net: generator parse2/units args --locations	-	no-direct-gold-table	-	stage=parse2/units;args=--locations;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
net	warnlex	e1	construction net: generator lex args --locations	2008	gold-table-bytes	-	stage=lex;args=--locations;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
net	tokenlex	e1	construction net: generator lex args -	2008	gold-table-bytes	-	stage=lex;args=-;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
net	tokenpp	e2	construction net: generator pp args --shared-predefines,--no-autoinc	465	gold-table-bytes	-	stage=pp;args=--shared-predefines,--no-autoinc;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
net	warnpp-shared	e2	construction net: generator pp args --locations,--shared-predefines	465	gold-table-bytes	-	stage=pp;args=--locations,--shared-predefines;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
net	object-lower-arm64	lower	construction net: generator lower args --full,--object,--arm64	-	no-direct-gold-table	-	stage=lower;args=--full,--object,--arm64;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
net	object-lower-x86_64	lower	construction net: generator lower args --full,--object	-	no-direct-gold-table	-	stage=lower;args=--full,--object;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
net	object-enc-arm64	elf	construction net: generator enc/arm args --object	-	no-direct-gold-table	-	stage=enc/arm;args=--object;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
net	object-enc-x86_64	elf	construction net: generator enc args --object	12491	gold-table-bytes	-	stage=enc;args=--object;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
net	object-lower-osx-x86_64	lower	construction net: generator lower args --full,--object,--osx	-	no-direct-gold-table	-	stage=lower;args=--full,--object,--osx;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
net	object-lower-osx-arm64	lower	construction net: generator lower args --full,--object,--osx,--arm64	-	no-direct-gold-table	-	stage=lower;args=--full,--object,--osx,--arm64;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
net	object-lower-win-x86_64	lower	construction net: generator lower args --full,--object,--win	-	no-direct-gold-table	-	stage=lower;args=--full,--object,--win;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
net	object-lower-win-arm64	lower	construction net: generator lower args --full,--object,--win,--arm64	-	no-direct-gold-table	-	stage=lower;args=--full,--object,--win,--arm64;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
net	object-enc-osx-x86_64	elf	construction net: generator enc/unitcontainer-osx-x86_64 args --object	-	no-direct-gold-table	-	stage=enc/unitcontainer-osx-x86_64;args=--object;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
net	object-enc-osx-arm64	elf	construction net: generator enc/unitcontainer-osx-arm64 args --object	-	no-direct-gold-table	-	stage=enc/unitcontainer-osx-arm64;args=--object;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
net	object-enc-win-x86_64	elf	construction net: generator enc/unitcontainer-win-x86_64 args --object	-	no-direct-gold-table	-	stage=enc/unitcontainer-win-x86_64;args=--object;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
net	object-enc-win-arm64	elf	construction net: generator enc/unitcontainer-win-arm64 args --object	-	no-direct-gold-table	-	stage=enc/unitcontainer-win-arm64;args=--object;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
net	tapecheck	-	construction net: generator tapecheck args -	-	no-direct-gold-table	-	stage=tapecheck;args=-;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
net	hostsymbols	lower	construction net: generator lower/hostsymbols args -	-	no-direct-gold-table	-	stage=lower/hostsymbols;args=-;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
net	librarycontract	-	construction net: generator librarycontract/gen args -	-	no-direct-gold-table	-	stage=librarycontract/gen;args=-;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
net	librarypartition	-	construction net: generator librarypartition/gen args -	-	no-direct-gold-table	-	stage=librarypartition/gen;args=-;model_raw=UNMEASURED;action_permille=UNMEASURED;selector_permille=UNMEASURED;note=recipe-to-model-map-not-in-package	scan recipes[] in tools/source-package.cx	bc5b04bf14806b6783778bf40cb64dfb357bc200	UNMEASURED	UNMEASURED	UNMEASURED
```

术语表(三口径,互不相加,数字由本工具重算):
(1) ask 事实表 18 张:`exec/pipeline/gold-stages.tsv` 的行数,即 prd 4.1 的十八网;每张有限表加全键穷举 exact。
(2) `weights/gold/*.tsv` 目录 20 个文件,共 254164 字节:比 (1) 多出的是 lexcls、lexword 两个词法辅助表(无独立 ask 网)。
(3) 构造网 42 张:`tools/source-package.cx` 的 recipes[],即 job list 中 gen=tbl=net=42 的来源;它是模型构造口径,与 (1)(2) 不一一对应,不得互相换算。
七流 7 个(`exec/pipeline/image-stages.tsv`)是字节管道阶段;流与网的对应只在 `flow`/`net` 行的 parent 列。isel、combo 两张 ask 表在 prd 4.0 流表中无归属,记为 gold-unassigned。逐网模型字节(recipe 到模型的映射不在包内)记 UNMEASURED;按流的 UNINETB1 普查见 flow-model 行(daily 档,需 --package)。
```
<!-- product-map:end -->
历史完成记录与证据不再追加到本文件：见 [prd_hist.md](prd_hist.md)；新记录写 `agentwork/ledger.tsv`。
<a id="memory-palace"></a>
### 9.2 记忆宫殿（索引；此处是唯一索引，正文各节只指向本表）

| 房间 | 当前事实 | 入口 |
|---|---|---|
| 任务队列 | `agentwork/items.tsv`（owner 列=tmux 窗口名；只有 READY 项被 omp-workflow 自动续跑）；进度追加在 `agentwork/ledger.tsv` | [agentwork/WORKFLOW.md](agentwork/WORKFLOW.md) |
| 当前发布 | 0.0.57（§9.1），tag v0.0.57 -> 99fc0b48a2；旧候选资格不迁移 | [0.0.57 发布说明](plans/release-notes-0.0.57.md)、[prd_release.md](prd_release.md) |
| 当前版本 | 0.0.58 进行中（P1 规划器单源 + 工具）；0.0.59 = P59 实例化 + 门批量化；0.0.60 = P60 CX0 事实表 + 列车式发布 | §9.1、§9.5、[prd_cx.md](prd_cx.md) §6、`agentwork/items.tsv` |
| 结构总表 | 五层、六片、7 + 1 流、网四口径、执行器、格式、事实表、构造链、测试 | §3（唯一权威）；cx 侧在 [prd_cx.md](prd_cx.md) |
| 新入口 | `src/unisacc.cx` 六层（host/packmodel/executor/stages/drive/bind）；`src/core`、`src/runtime` 指向 `cx/` | [plans/cx-modules.md](plans/cx-modules.md)、§5 |
| 流/网 | 7 + 1 流；18 金表网 / 20 cx 侧模块 / 42 构造网 / 40 模型 | §0、§3.3、§3.4 |
| 测试入口 | `sh ./unisacc.com tests/run.cx -- --smoke/--all/--list` | §7.1、`tests/batches.tsv`、`tests/coverage.tsv` |
| 分层 | 三套编号（术语层 T / 结构层 L0-L4 / 模块层 cx-L0..3）见 §0；模块层与包层见 prd_cx.md §3 | [plans/cx-layers.md](plans/cx-layers.md) |
| 棘轮 | 13项，以`tools/ratchets.cx`末行为准；新增第14号net-deps，toolsloc已退役；工具行数只统计 | `tools/ratchets.cx`、§2.2 |
| 事实表 | 事实 -> 生成器（须带 `--check`）-> 产物 | `exec/facts/`、§2.4 |
| 构造图与相位 | `build/graph.tsv`、`build/hardwire.tsv`、候选链 35 相位 | §3.7、§2.2 |
| .cx lint | `tools/cx-lint.cx` R1（第二真相源，只报告）/R2（子进程时限与 `--`，precommit 拦截）；check 41（8a07e8e119） | §2.2 |
| 静态预检 | 模式/引用/兜底/SB 合同先查；动态及未证明性质单列 PARTIAL，不能替代真实 gen/新包/slice | `tools/tsv-lint.cx`、`tools/chain-precheck.cx` |
| 结构盘点与普查 | 源码/模型分母与六片模块账全部实核守恒；最终模式全部登记实跑；C47已DONE，行数门按主人新裁定退役，不称全局棘轮或CI绿 | §9.6、`tools/module-byte-map.cx`、[六片账本与材料身份](research/c47-logic-distribution-20261108/)、[archive/docs-2026-11c/plans/slice-shrink.md](archive/docs-2026-11c/plans/slice-shrink.md) |
| 归档 | 旧 C/Python 实现、旧 1333 项套件、历史计划与 PRD | §10、`archive/` |

### 9.3 往期发布与历史（索引）

逐版发布历史见 [prd_release.md](prd_release.md) §往期发布；0.0.43 见 [plans/release-0.0.43.md](plans/release-0.0.43.md)，0.0.45 及 0.0.48 至 0.0.53 见 `plans/release-notes-<版本>.md`。0.0.47 逻辑分布普查结论与 0.0.48.1/0.0.49 收尾记录已迁入 [prd_hist.md](prd_hist.md)。

### 9.4 路线
- **0.0.43 – 0.0.57**：已收口并发布（计划 `plans/v0.0.4x*.md`、`v0.0.5x-unisacc.md`，发布说明 `plans/release-notes-*.md`）；agent harness（cx-csih）仍归 csih 车道。
- **0.0.58**：进行中，唯一产品改动为规划器单源 P1（55-1），其余为工具与测试（§9.1、§9.5）。
- **0.0.59**：P59 解析状态实例化（55-2）加门批量化、main 健康检查、夜跑彩排。
- **0.0.60**：P60 CX0 事实表入库与第 8 流（55-4）加列车式发布；骨架网第一批随之裁定（prd_cx.md B2 55-4..55-6）。
- **0.1.x**：Clang/GCC/TinyCC 接入 `.cx`（共用前端 + 后端插件）；gcc_cx / clang_cx 宿主现状见 prd_cx.md 开篇。

<a id="optimization-leads"></a>
### 9.5 提速线索（0.0.58；基线数字一律引 §3，不在此重复）

| 对象 | 现状 | 线索与归属 |
|---|---|---|
| 冻结到公开 | 0.0.57 实测 190.6 分钟（冻结 01:07 起链至 04:18 公开） | 目标不超过 100 分钟（硬线 120）；T1 至 T3、T11 合计压缩，以 R3 整链彩排实测为准 |
| G2 预算分批 | 0.0.57 手工续跑，BLOCKER-3 至 MANUAL-5 | T1：逐键 journal 续跑（`c57-g2batch-r3`）；先定 `g2-budget-gen` 批次门内 FAIL 的原因 |
| chain-run 预算与所有权 | 逐相预算不持久，进程所有权靠人工 | T2：逐相预算、持久 deadline、进程所有权（K10） |
| 相位并行 | 按目标串行 | T3：按目标最多 4 路并行，七列相位表（K9） |
| 续跑复核开销 | 五个观测点 13 至 169 秒，总量 UNMEASURED | T11：实测每次重入固定开销，超 15 秒则做带身份与失效规则的持久备忘；chain-run 增段秒数与监督器自动续段（每段不超过 280 秒） |
| 发布尾部 | 17 相命令已有，缺 CONFIG 与 dry-run | T5：对 rel057 材料 dry-run；T6：资产齐全后才触发 CI（assetguard） |
| s9 | 0.0.57 提交档 150 PASS / 2 FAIL / 2 UNKNOWN，NOT_QUALIFIED；daily 713 批、release 41 批至今没有跑通过一次 | T12：四批修复（s9red），冻结前应落、不卡冻结；daily / release 整档属 0.0.59 门批量化 |
| 字面锚点 | 五域共 68 处发现 | T4：cx-lint R4 禁用，计数入表只降不升 |
| 版本度量 | 无统一表 | T9：`agentwork/version-metrics.tsv` 加生成器，回填 0.0.54 至 0.0.57 |
| L0 / APE 体积 | 头 42.2% 未压缩、padding 7.1%（§3.0） | 属产品输入大改，不进 0.0.58 |

### 9.7 未来计划（路线与入口；细节见各版 plans 文件，数字未标出处者为估计）

排序依据（主人愿景）：在稳步推进功能的基础上，优先做对提升整体开发迭代速度有用的项。每版只留一项产品或功能改动，其余槽位给缩短冻结到公开、减少人工、门吞吐、左移检查、并行化与增量复用。

- **0.0.58（进行中）**：唯一产品改动 P1 规划器单源（prd_cx.md 55-1，补丁 `c58-planner`）；工具侧 T1 至 T12（G2 预算分批、chain-run 逐相预算、按目标 4 路并行、字面锚点禁用、发布尾部自动化、assetguard、s9 红批修复、版本度量、续跑复核开销）。目标：冻结到公开不超过 100 分钟（硬线 120）、人工介入 0、chain-run 零接触。详见 `plans/v0.0.58-unisacc.md`。
- **0.0.59**：P59 解析状态实例化（prd_cx.md 55-2，去掉对工作进程隔离的依赖）；门批量化（门栈批量与门脚本迁移）、main 健康检查与令牌池、夜跑彩排；items 整表清账（H4）、贴线批次（H5）。
- **0.0.60**：P60 CX0 事实表入库与第 8 流（prd_cx.md 55-4，及 cx-tri 三宿主门 55-3 视矩阵结果）；列车式发布（固定节拍，随车搭载已绿的工具与产品改动）。
- **0.0.61 及以后**：
  - 目标可外部扩展的设计（目标枚举与路由数据驱动，外部包按 manifest 追加模型、路由、预定义、头文件，与 prd_cx.md B3-B5 外置分层合并）；交叉编译结构调查（用户程序出码是否链入预编原生对象、nativeabi/symbols 模型作用、e3 类型网是否按目标区分，结论写回 §3）。
  - 外置加载器 `unisacc-loader.com` 与按目标独立可执行（量化 L0 启动器开销），原属 0.0.55 可砍线，未落地，重新排期。
  - N0 APE 头压缩（省约 36%，单独立版）。
  - 新目标 lnx-x86-32 先于 lnx-arm-32，以扩展包形式提供，依赖目标可外部扩展；交叉出码到 32 位，编译器本身不在 32 位运行；ARM32 编码为大头。
  - 链接单元 CX-W-1。
  - yb-all 待门与链提速后测量再定。

## 10. 子文档与归档索引（树）

```
prd.md                 当前状态（本文件，只写当前事实；结构点唯一定义在 §3）
├─ prd_cx.md           cx 侧结构：三宿主、CX0 流、cx 侧网模块、包层、0.0.58 至 0.0.60 路线
├─ prd_release.md      发布与 CI 手册、0.0.48/0.0.49 教训、往期发布身份
├─ prd_hist.md         历史完成记录与证据（只读；新记录写 agentwork/ledger.tsv）
├─ docs/glossary.md    术语层（T1-T5）的逐处核实
├─ plans/              各版计划与发布说明（最新 v0.0.58-unisacc.md、release-notes-0.0.57.md）
├─ build/graph.tsv     构造图事实表（graph-check 读）；build/product-map.tsv 全景表
├─ agentwork/          任务看板 items.tsv、账本 ledger.tsv、phase-times.tsv、WORKFLOW.md
└─ archive/            旧实现与旧文档（见下）
```


- [archive/plans/2026-11-01/prd-c43-full-2026-11-01.md](archive/plans/2026-11-01/prd-c43-full-2026-11-01.md) —— 本文件重写前的完整旧版（含思维树/记忆宫殿全流水、能力速查全文、§3 架构全文、§5 路线全文、§6 未解决问题）。
- [archive/prd-c43-history-20261030.md](archive/prd-c43-history-20261030.md) —— 0.0.43 历史收口。
- [archive/prd-release-and-development-history-20261001.md](archive/prd-release-and-development-history-20261001.md) —— 逐版发布身份。
- [archive/testing-c43-legacy/](archive/testing-c43-legacy/) —— 旧 1333 项测试套件（已停调度）。
- [archive/reference-c/](archive/reference-c/)、[archive/reference-py/](archive/reference-py/) —— 旧 C/Python 参考实现。
- [archive/plans/](archive/plans/) —— 历史计划与设计探针。
- [archive/docs-2026-11/](archive/docs-2026-11/INDEX.md) —— 2026-11-11 文档清理移入的 plans/ 与 research/ 文件（保留原子路径），索引见 INDEX.md。


0.0.48.1 OSX-LAUNCH-SLOW / OSX-SIGN-ROSETTA 与 0.0.49 收尾的逐条完成记录已逐字迁入 [prd_hist.md](prd_hist.md)「0.0.49 收尾」；本文件不再保留。
