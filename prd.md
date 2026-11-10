# unisacc —— 产品与项目总览

本文回答五个问题：unisacc.com **是什么**、**有什么功能**、**结构组成**、**测试套件怎么设计**、**项目目录与文件结构**。术语歧义以 §0 为准；历史流水与逐版收口在 [archive/plans/2026-11-01/prd-c43-full-2026-11-01.md](archive/plans/2026-11-01/prd-c43-full-2026-11-01.md)（完整旧版），不在这里重复。

## 0. 概念与术语（先读；歧义以本节为准）

「流 / 网 / 段 / 阶段」四个词各只有一个意思。详细版与逐处核实见 [docs/glossary.md](docs/glossary.md)。

| 层 | 唯一名 | 含义 | 数量 | 真源 |
|---|---|---|---|---|
| L1 | **Stream**（别名 Flow） | 字节管道阶段：输入格式 -> 输出格式 | unisacc **7**（e2/e1/e3/e4/prune/lower/elf） | `exec/pipeline/image-stages.tsv` |
| L2 | **Net** | ask 事实网：一张有限表 + 全键穷举 exact | **18**（8770 键） | `weights/gold/*.tsv`（权威；规则形式 `seed/gold_<阶段>_label.c`，由 `tools/gold-table.cx --check` 核对） |
| L3 | **Segment** | 训练合并段（多网合一的 masked 多任务模型） | 3（s1/s2/s3） | 负结果证据已归档 |
| L4 | **Book phase** | 一次推导中 ask 的记账阶段 | 由 contract 声明 | `flownet/contract.cx` |
| L5 | **Release phase** | 发布流程阶段 | 10 | 已归档 |

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

**RI 的固定含义**：本文 RI = **ResourceInput（具名资源输入）**，含名称、字节和长度、存在性/空值及跨流身份绑定，不是 "runtime integrity"。

## 1. unisacc.com 是什么

**一句话**：unisacc.com 是 C99 编译器（带部分 POSIX 库子集），打包成单个类 APE 文件——核心需求是六个 ISA 切片（lnx/osx/win × x86_64/arm64，每个目标同候选原生证据；现状缺口见下），能为 {Linux, macOS, Windows} × {x86-64, arm64} 六目标直接写出可执行文件，并以默认 `-run` 直接运行 `.c` 和 `.cx`。cx 是参照 Rust 设计的模块化方言（`#pragma unisa module/use`），所有权与边界目前为**运行期**检查，不等于编译期借用检查。编译内部是 7 个字节流阶段，表状决策交给 18 张事实网（16 张出货，isel/combo 为对照），网络权重由真值表构造并穷举验证、不经训练。通用执行器执行构造出的选择器与动作程序；这是决策表示的改变，尚无语义决策覆盖率证明，不能由模型体积推出“以网换逻辑”。

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

签名交接成员清单：`sh ./unisacc.com tools/release-members.cx -- DIR NEW_MANIFEST APE OSX_ARM OSX_X64 CACHE_ARM CACHE_X64`生成无header五角色清单；`--check DIR MANIFEST`重核实际SHA、P4/footer与四thin MachO边界。配套`tools/check-release-members.cx ROOT`独立夹具实际21/0（会话69831），只证明字节/结构，不认证签名或最终候选资格。
登记补齐：`release-members-interface`注册器实际BATCH PASS（92725），显式Mac宿主daily接口；库存经`tests/tools-compile.cx --register`原生重算并仅加入两新源，从652到654，仍82片。此处仅库存登记，不称654源全编通过。
收尾实证：最终Mac daily批62131实际21/0+BATCH PASS；清单59661 metadata_valid1/757批，分片9196实测82；首次棘轮12/1的两处未检查拷贝改真容量mm_copy，最终90679实际13/13，无扩基线或删覆盖。

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
| 候选链相位预检(隐性前提) | `sh ./unisacc.com tools/preflight.cx -- build/preflight-rules.tsv ROOT=DIR [BUILD=DIR] [SNAP=DIR] [REV=SHA40] [PKG=FILE] [PHASE=entry\|post-snap\|pre-p3\|post-p3] [--strict]`；检查 `tools/check-preflight.cx`(批次 preflight-tool) | 逐条执行规则表 assert 列(规则表行数以表为准;hook 列标挂点;断言 absent/present/exec-if-present/epoch-canonical/release-rev/src-has/src-lacks/pkg-headers/env-set/receipt-if-present),输出 PASS/FAIL/SKIP 与 error_text,汇总 `PREFLIGHT<TAB>failed=N`;PHASE= 只跑该挂点规则,且该挂点应已产出的变量缺失记 FAIL;变量未给的规则记 SKIP 不记 PASS,`--strict` 下有 SKIP 即退出 1;失败退出 1 |
| 改一个文件看爆炸半径（一条命令） | `sh ./unisacc.com tools/flow.cx -- check FILE... [--run]` | 串起符号图受影响文件 + 受影响测试批次；`flow -- graph FILE` 出局部图 |
| 模块关系图（全图/子图/差分） | `sh ./unisacc.com tools/symindex.cx -- graph [FILE...]` / `-- delta FILE...` | 节点=文件、边=use/call；给 FILE 只出该文件的正向 use 闭包 + 反向 users |
| 文档断链 | `sh ./unisacc.com tools/doc-links.cx -- --tracked` | 归档后修链 |
| 只提交自己的行 | `sh ./unisacc.com tools/commit-own.cx -- -m MSG --keyed PATH KEY` | 多人同改文件（prd/batches/coverage） |
| 提交器出补丁 | `sh ./unisacc.com tools/submit-patch.cx -- OUT.patch [OWN_ID]`；检查 `tools/check-submit-patch.cx` | 私有克隆 git add -A 后 full-index 补丁(序号后缀不覆盖)、sha、tools/*.cx 逐个 -c、新增 .cx 走 newcx-check、batches/coverage 只含 OWN_ID 且 IMPLEMENTED 的 check 存在、须含 index 行；一行 SUBMIT_PATCH |
| 研发耗时账本 | `sh ./unisacc.com tools/devtime.cx -- append ...` / `-- report [FILE]`；表 `agentwork/devtime.tsv`；检查 `tools/check-devtime.cx` | 列 task_id/窗口/开始/交付/落地/类别/备注；类别六种；report 按类总秒与占比，未记 UNMEASURED |
| s9 夜跑编排 | `sh ./unisacc.com tools/s9-nightly.cx -- ROOT RUN --tier commit\|daily [--date D] [--fake BIN]`；假 s9 `tests/fixtures/s9-nightly-fake.cx`；检查 `tools/check-s9-nightly.cx` | 按手册 build/init/prepare 六步/step 至非 PENDING/aggregate；失败具名停止留报告；夹具不跑真批次 |
| 产品全景表 | `sh ./unisacc.com tools/product-map.cx -- --write\| --check`；轻检查 `tools/check-product-map.cx`；重夹具 `tools/check-product-map-fixtures.cx -- unrelated\|prd-edit\|input-change` | 层/片/流/网字节与 prd 块对齐；默认检查只读 tsv+prd，不跑 --write |
| agentwork分片读侧、迁移与账本追加 | `sh ./unisacc.com tools/aw.cx -- ledger/items/check/migrate [筛选] [--root ROOT]`；`-- append --owner OWNER --id ID --event EVENT --evidence TEXT [--commit C] [--date D]`；检查 `tools/check-aw.cx -- pure/parallel/conflict` | 按月/owner账本、按ID看板，历史5/7列及重复/行尾位无损容器；新布局严格校验，真实并发夹具零冲突，同项双方状态可读；append 为 owner 串行化原子追加（O_APPEND+fsync，路径/IO 校验失败 rc2）；工具交付不代表真实数据已迁移，迁移/删旧/引用切换仅由cc静默执行 |
| 装前判产品能不能用 | `sh ./unisacc.com tools/check-dev-product.cx -- ROOT PRODUCT` | 六条：shell 头 / `sh --version` / 不带 `UNISA_CONTAINER` 能编译 / 写入产物 / 内嵌包是新头 |
| 失控与超数量`.cx`进程守卫 | `sh ./unisacc.com tools/procguard.cx --once`（默认dry）；`--enforce`才回收；`--only-parent PID`收窄受控扫描；检查`tools/check-procguard.cx`（批`cx-procguard`） | 每10s扫描同uid/.cx；保留同文件数>`PG_MAX`(100)规则，另将累计CPU>=90%且存活>10分钟、cwd已删、ppid1且>30分钟非tmux树列候选。只日志不默认信号；enforce逐PID先STOP后KILL并复核starttime/uid/保护，绝不按pgid/session杀。`CLK_TCK`真实读取，读失败不启CPU/orphan判定；沿用本身/祖先/原名单及tmux/pane保护，orphan判定排除tmux子树。日志`~/.cache/procguard/log`，旧组回收后的fresh-scan `residue=KEY:N`合同保留。 |
| 超时作业的整棵树清理 | `#pragma unisa use sys.process` -> `sy_child_subreaper()`、`sy_kill_tree(root, ms, keep, nkeep, rep, cap)` | 先冻结根、反复扫 `/proc` 把新后代与同组进程也 SIGSTOP（被停进程不能再 fork），到不动点后全杀，返回残留数（0 才算干净）；starttime 防 pid 复用；subreaper 让双 fork 的孤儿仍可见；非 Linux 返回 -ENOSYS。检查：`sh ./unisacc.com tools/check-reaper.cx` |
| 子进程超时契约（run/capture + residue） | `sh ./unisacc.com tools/check-netpack.cx` | 双管EOF后仍按原deadline非阻塞等待，超时冻结整树后击杀并有界定向回收直接子，不抢调用者的其他子进程；`np_result.residue` 0才算干净。孤儿可见性仍需调用方先`np_subreaper()`。`UNISACC_NP_FAULT=residue`为tools与run-batch的显式注入缝，默认关，其它值具名拒绝；sysx产品单次回收疑点另列0.0.47，不混同此测试库修复。 |
| e2stream 包装契约 | `sh ./unisacc.com tools/check-e2stream.cx` | 自建进程组 + 整树击杀（含 setsid 逃逸）+ `TimeoutExpired` 文案；干净超时不带 `survived)` 后缀 |
| 督导观察 agent 面板 | `sh ./unisacc.com tools/observe-agents.cx -- [--panes a,b,c] [--no-git] [--ratchets] [--fixture FILE]` | 逐个 tmux 面板读 `PANE`（state 取 mind 块首行 `▶`=busy / `○ 空闲`=idle，busy_for 取底部栏相对时长）/`PLAN`/`TOPIC`/`TASK`，并按 `agentwork/items.tsv` 第 9 列标 `STALE`（DONE、CLOSED…、SUPERSEDED）与 `PARTIAL`（DONE-\*，尾巴未收）；`--fixture` 用文本面板，`tests/check-observe-agents.cx` 即其批次；输出 ≤30 行 |

#### 2.2 测试（棘轮、门禁、批次）

T1合同登记（登记不等于通过）：`generator-location`批次自建真实GCC hostgen，并保留 `ROOT ABS_GEN FRESH_OUT` 独立入口；`unipp2-reference`必须绑定 `newpp-run/newpp-net`；30个`netregen-CASE-OS-ARCH`批次必须绑定 `old-product/new-product/new-models`，各自prepare后执行A/B镜像比较。PREPARED不作测试结果；镜像字节同不代表外机执行资格。`opt-frame4-o1-*`与`opt-frame4-o2-*`共8片各显式绑定对应o1-run/o1-net/o1-table或o2-run/o2-net/o2-table，分别执行positive/negative-label/negative-end/negative-eof；两算法路径分别登记，覆盖语义投影、有序元数据、唯一end、真实dot标签与精确截断拒绝，不升级为产品全链覆盖。


| 动作 | 现成用法 | 说明 |
| 核对队友报告里的事实 | `sh ./unisacc.com tools/report-verify.cx -- CLAIMS.tsv`；校验 `tools/check-report-verify.cx` | commit/tsvcount/receipts/sha256/ratchets 五类断言逐条核实，输出 OK/MISMATCH/MISSING，有 MISMATCH 退出 1 |
|---|---|---|
| 查模块层向与包边界 | `sh ./unisacc.com tools/check-cx-layers.cx [PRODUCT]`（`-- --fixtures` 跑夹具） | 编号层 L0–L3 直接 `use`，向上越层即红；`ext/` 是包层：产品层（`src/`、`cx/core`、`cx/runtime`、`cx/compiler`、`cx/driver`、`cx/cx0`、`cx/workspace` 与 `tests/cx-module-freeze.tsv` 的 core 模块）`use` 到 `ext/` 即红；core 模块文件集合按冻结表只许减（新增行即红、行无文件亦红） |
| core 导出面棘轮 | `sh ./unisacc.com tests/coreapi.cx -- --check`（`--write` 下调，`--fixtures` 跑夹具） | 26 个冻结 core 模块的每个非 static 定义各占一行：新符号红、行缺失红、签名变动红、`DEPRECATED-ALIAS` 的调用方文件数超上限红；`core_run` 一行标 `DUAL-IMPL linked=cx/sys/kernel.cx`，两条链接由 `cx-core-run-diff` 逐字节比对；当前一次构造native symindex后逐查询执行，26模块/194导出实核13.874s、原8夹具14.322s，原55秒不扩，查询结果不缓存 |
| core_run 双执行器差分 | `sh ./unisacc.com tools/check-core-run-diff.cx -- ROOT KERNELS` | 同一模型与三组固定输入分别喂 `cx/core/execute.cx`（CX 参考实现）与 `cx/sys/kernel.cx`（产品链接的汇编内核），两行整体逐字节比较，末行 `CORE_RUN_DIFF`；KERNELS 是含 `x86_64.kernel` 的目录 |
| 拒新的模块 `#include` | `sh ./unisacc.com -run tests/cxinclude.cx -- --check` | 新模块必须 0 |
| 上层禁 libc/POSIX | `sh ./unisacc.com tests/cxlibc.cx -- --check` | 名单不手写，按前导分类 |
| 看某域还差什么 | `sh ./unisacc.com tests/full-domain.cx -- . DOMAIN` | 逐格读矩阵，印 OPEN 行 |
| 记录已知缺口 | `sh ./unisacc.com tools/matrix-waive.cx -- . tests/waivers-0.0.44.tsv --check` | R 行理由 + W 行豁免 |
| 构造图检查（G1） | `sh ./unisacc.com tools/graph-check.cx [-- --graph F --chain F --hardwire F --no-hardwire]` | 读 `build/graph.tsv`（203 节点事实表）：无环/依赖存在/kind 枚举/budget≤60；六目标次序与 `tools/chain-step.cx` 三份常量比对（不一致 ORDER_MISMATCH+rc1）；`build/hardwire.tsv` 32 条第二真相源模式逐行扫描只计数；末行 `GRAPH_CHECK\tnodes=\tedges=\tcycles=\torder_mismatch=\thardwire=`；夹具 `tools/check-graph-check.cx` |
| 根独立/帧规范化（0.0.50） | `sh ./unisacc.com tools/root-canon.cx -- --map PREFIX=PLACE... [--diff A B \| --scan F \| F]`；`tools/root-independent.cx -- --a DIRA --b DIRB --list F --map ...` | 多前缀最长优先替换，按 formats.md 重算 phase/单元/UNIPP2 LE32 长度；末行 `ROOT_CANON_DIFF\traw_equal\traw_len_delta\tprefix_bytes\tnonprefix_bytes\tcanon_equal`；独立性 list 行 `node\tproduct\|frame\trelpath`，product 逐字节且不含根、frame 规范化相等；`build/graph.tsv` 第 12 列 root_dependent 由 graph-check 校验；夹具 `tools/check-root-canon.cx`、`tools/check-root-independent.cx` |
| 链看门狗（0.0.51 W/W2） | `sh ./unisacc.com tools/chain-watch.cx -- DIR [--idle-min N] [--final PHASE] [--resume-cmd 'CMD'] [--max-resumes K] [--now EPOCH_S]` | 读 `DIR/checkpoint.tsv`（candidate-chain 的 `DONE\tphase\t...` 行）取 last_phase，进展时间取 checkpoint/DIR 顶层/`DIR/logs` 最新 mtime；N 分钟（默认 10）无进展判 STALLED，含 final 相位（默认 pack）判 DONE；STALLED 前核 `DIR/heartbeat.tsv` 末行（`epoch\tphase\tpid\tpgid[\tstart\|end]`，由链每相位写）的 pid/pgid 存活：liveness=HUNG（存活）\|DEAD（不存活）\|UNKNOWN（无心跳），三者都续跑；一行 `CHAIN_WATCH\tstatus=RUNNING\|STALLED\|DONE\tlast_phase=\tidle_s=\tliveness=`，现场写 `DIR/state.md`；STALLED 且给 `--resume-cmd` 时经 `sh -c` 执行一次（55s 上界），次数记 `DIR/chain-watch.tsv` 封顶 K（默认 3，耗尽 rc3 不再重启）；退出码 0 RUNNING/DONE、1 STALLED、3 耗尽、2 用法；夹具 `tools/check-chain-watch.cx`（临时目录伪造三态、HUNG/DEAD 与封顶，19 项） |
| 仓级棘轮（13 项） | `sh ./unisacc.com tools/ratchets.cx` | cxraw/cxinclude/cxapis/cxtrunc/cxlibc/cxplatform/srcshape/coreapi/verwriter/aluiwidth/outdep/check-literal/net-deps；toolsloc退役，新增net-deps沿计划保留第14号门称呼；旧通过记录不迁移 |
| tools 统计与候选清理 | `sh ./unisacc.com tools/tool-inventory.cx -- --recent 10` | 总行数、按目录/文件及近N次提交增量；无调用者/无批次/疑似重复只报告；不设总量或逐文件上限。tests/toolsloc.tsv退役保留；precommit只对新增tool缺check或批次登记拦截 |
| osx X1旧项复核（2026-11-08） | `tests/driver-target-object.cx ROOT cli`：9/0；平台宏最小源见`tests/cases/driver-default-platform.cx` | Darwin arm64 26.5.1、产品SHA f069d4c92b7f5ede508809aa3abd21e326cbecadb2fb9eccf0bd1ac7004c3e82。默认-run原生、裸-c产宿主Mach-O对象并由产品链接执行，NATIVE-RUN-HOST-TARGET核销；不承诺系统链接器兼容。裸-S仍用lnx/x86_64预定义：平台源bare/显式lnx tape SHA同为d7bb734dae19…，显式osx为e02cac44f622…；DEFAULT-TARGET-S-C保留READY，只剩-S语义裁定/修复，不重复-c修复，也不迁移六宿主CI资格 |
| 真实提交工具配套与源码分类闸 | `sh ./unisacc.com tools/check-precommit-toolsloc.cx -- --loc`、`-- --source`、`-- --symlink`（保留三批历史ID）、`-- --lint-r2-sep`/`--lint-r1-report` | 独立真实临时git仓运行生产hook；历史行数超额允许，shebang伪源码拒绝，新增tool缺check或缺IMPLEMENTED批次拒绝；staged .cx 经 cx-lint：R2 拦、R1 只打印；旧工具无调用者/无批次/重复仅列候选 |
| .cx token 层 lint（第二真相源/子进程） | `sh ./unisacc.com tools/cx-lint.cx -- [--strict] [--rules R2,R1-version] [PATH...]` | R1-version（版本字面量；tests/、check-* 的拒绝数据与文件名豁免）、R1-product（产品路径字面量经名字数据流进 argv 槽/run 首参才报）、R2-timeout（无界子进程）、R2-sep（.cx 工具 --opt 前缺 `--`）；`/* lint:allow R1 原因 */` 行尾显式豁免 R1（必须带原因）；精确计数夹具 `tools/check-cx-lint.cx`；precommit 对 staged .cx 跑 `--rules R2 --strict` 拦截 |
| 预冻结演练(SPEED-049-E) | `sh ./unisacc.com tools/freeze-plan.cx -- plan ROOT out/`；`-- check --ident-list FILE --draft FILE`；`-- boot-list LIST`；夹具 `tools/check-freeze-plan.cx` | ident list-current 导出 freeze.tsv 草案到 out/（不写 agentwork/）；覆盖/过宽非零；启动清单缺文件启动前拒；符号链接真身须覆盖。不覆盖现行 L0-CORE-FREEZE-049 |
| SPEED-049-F 版本数据解耦 | `sh ./unisacc.com tests/release-version.cx`；打包与身份验收见 `tests/README.md` 的 SPEED-049-F 配方 | 唯一版本列为 `release/candidate.tsv`；源固定32字节槽，打包注入。解析测试已登记，未经本轮执行不得转授旧材料资格；L1–L4 非版本格式断言保持。 |
| SPEED-049-A 两代抽样 | `sh ./unisacc.com tools/fixedpoint-sample.cx -- sample --seed N UNITS GEN2 GEN3`；`-- release` 拒 `--sample`；`-- wave PART OUT`；夹具 `tools/check-fixedpoint-sample.cx`；链内开关 `tools/chain-step.cx fixedpoint … --sample --non-release`（979b56e4d9） | 非发布候选确定性抽样(收据 SAMPLED);发布候选强制 FULL;拆波拼接=一次跑。chain-step 默认完整两代，`--sample` 不配 `--non-release` 按发布候选拒绝（check `chain-step-release-forbids-sample`）；0.0.49 发布候选按 FULL 跑，g2==g3（发布说明）。gen1–gen3 整链并行端到端墙钟 UNMEASURED（`agentwork/phase-times.tsv` 第 11 行） |
| C49 X全闭包分相构造 | `tools/whole-front.cx` prepare/front/merge/stub/span；固定点由 `tools/chain-step.cx fixedpoint --span-driver/--span-prepared/--span-index` 逐步接纳 | 真实85单元32/32/21，Mac真实桩后units..e3与prune..elf，Win三段；子相总45秒，来源与checkpoint实际哈希重核；当代Ctor与P4显式绑定，旧env-B路径不用，原生CI资格独立 |
| 分相结构合并及独立边界 | `tools/check-whole-front.cx`；`tools/check-wholefrontmerge.cx`，批次whole-front/wholefrontmerge | 原9项合同保留；另实测3项多属性顺序/opaque字节、非相邻重复拒绝无输出、零单位双空编码；模块检查不冒充产品等价或候选资格 |
| 分片测试：显式绑定 slice/pkg 启动 | `tools/test-loader.cx`（`--slice ABS --models ABS --identity ABS`、`--net STAGE=ABS --swap-tool ABS` 单网替换、`--e2-adapter`）；检查 `tools/check-test-loader.cx`（批 test-loader-com，需 5 个 `TEST_LOADER_*` 绝对路径环境绑定） | 开发证据，不是发布资格；缺绑定非零，不回落读 `./unisacc.com`；规格见 plans/test-loader-com.md |
| 波次哪些批不能同跑 | `sh ./unisacc.com tests/check-wave-domains.cx`（批次 `wave-domains`） | `tests/wave-domains.tsv` 每行 `id<TAB>domains<TAB>reason<TAB>evidence`；domains 取 `serial`（独占一波）/`wt`（写共享路径）/`hs`（写 harness 三树）/`mi:<材料名>`，空=无约束；all-waves 按掩码两两不相交放置 |
| 单阶段过滤器（分段编译管道） | `./unisacc.com stream STAGE [--route R] [--res NAME[=FILE]] [--models PKG] [--raw] < in > out`；`stream frame --in L=F...`；`stream sidecar` | 同一 st_span/包/`\0cli/*` 跑一个阶段；属性表随流（USLATTRT）；`e2\|e1\|e3\|sidecar\|prune\|lower\|elf` 与 `-o` 逐字节同；校验 `tests/check-stream-pipe.cx`；改表 -> `tools/lab-loader/p3-swap.cx --model` -> `--models` 实测 ≈8s |
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
| 切片 NN 迁移清单 | `sh ./unisacc.com tools/slice-migration-gen.cx -- --write\|--check`；校验 `tools/check-slice-migration-gen.cx` | 由 `plans/unit-text.tsv` + `plans/slice-units.tsv` 生成 `plans/slice-nn-migration.md`；`--check` 核两表键 1:1、源文件存在、文档与重生成文本一致 |
|---|---|---|
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
| 模型输入影响面投影（遗留 Python） | `python3 tools/affected-model-inputs.py CONTEXT_OR_TAR [--check]`；数据 `tools/affected-model-inputs.tsv` | 不运行生成器，仅由 source-package gen 读记录推影响的模型输入；遗留 Python 依赖，待 cx 化 |
|---|---|---|
| 普查红批次 | `sh ./unisacc.com tools/census.cx -- $PWD out/census` | 无材料批次逐个实跑找回归 |
| 看组成结构树 | `sh ./unisacc.com tools/com-tree.cx` | 每段占文件百分比 |
| 模型解码产物的逐模型 sha(解码器改写的 oracle) | `sh ./unisacc.com tools/model-decode-sha.cx -- EXPLODED_DIR [--reps N]`（批次 `model-decode-sha`） | EXPLODED_DIR 由 `tools/p3-explode.cx -- PKG OUT` 产出;每个模型 inflate 后 `cxmd_decode`,把 CxModel 逐字段渲染后取 sha,打 `MDSHA` 行与总 `all=`;`--reps` 只改计时(`decode_ns`,不含 inflate);改 `cx/runtime/model_decode.cx` 前后 `all=` 必须相同;校验 `tools/check-model-decode-sha.cx` |
| 切片单元只读盘点 | `sh ./unisacc.com tools/slice-inventory.cx -- --slice DIR [--tools-root ROOT]`；`sh ./unisacc.com tools/check-slice-inventory.cx` | 核对象/构造器/容器SHA与同构造器顺序plan映射，输出字节/物理行/use反向闭包/保守标签与top15；字节降序 -> 标签 -> 用户数升序 -> 名称。`identity=VERIFIED`不证明源码与对象历史等价或产品资格；未知纯度归MIXED，批次`slice-inventory` |
| 单元分流 fan + 材料准备 | `tools/bound 55 sh ./unisacc.com tools/units-prepare.cx -- ROOT ABS_OUT SLICE MODELS VERSION CARRIERS STAGE`，STAGE 依次 `loader`、`swap`、`seal`；`sh ./unisacc.com tools/fan.cx -- -j N --env ABS_OUT/loader.env` | T2.a clean cutover：不再提供 M8 复制式选项或幂等复用；loader 创建新 OUT 实编译 test-loader，swap 实编译 p3-swap，seal 记录真实源/部件/六 carrier.N SHA，原冻结 carrier pin 仍由消费者检查。仅 loader 六批可将 CARRIERS 设为 `-` 并以 `seal-loader` 输出 loader6.env，不得冒作 fan 材料。精确 VERSION 与提取来源由调用者绑定；PREPARED 非 PASS、非同候选发布资格。RI 入口为 `tools/materials-prepare.cx -- ROOT ABS_OUT native\|chain\|seal`，每阶段外包55秒；历史16项就绪/7项未备及原始Root归属见 `plans/m7-material-spec.md`、`plans/l-tests-materials-prepared.tsv`。 |
| 切片镜像归属度量（不改产品） | `sh ./unisacc.com tools/image-contrib.cx -- --objects a.o... [--image FILE] [--src KEY=PATH]`；`sh ./unisacc.com tools/check-image-contrib.cx` | 合并tape的`tl_unit` emit按单元计数；最终镜像列恒UNATTRIBUTED（prune/lower/elf丢来源）。分类TRANSFORM/FINITE/UNBOUNDED/SYS-IO来自源码扫描不是对象体积。批次`image-contrib`。试点设计`plans/slice-shrink.md`，fable评审前不动产品 |
| 生成代码运行时性能三档对照 | `sh ./unisacc.com tools/check-bench-gencode.cx -- ROOT [N] [1\|8]` | 同一 `tools/bench-gencode.c` 用 unisacc / gcc -O0 / gcc -O2 各编，1MB+8MB 两档，每项 N 次取中位与 cv；三档 RESULT 校验值必须逐字节相同，否则该项 INVALID；cv>10% 标 NOISY。批次 `cx-bench-gencode` |
| libc99posix 缺口扫描 | `sh ./unisacc.com tools/libc99posix-audit.cx`；校验 `tools/check-libc99posix-audit.cx` | 扫 include/*.h 的 static 函数名对照契约表；GAP 按头文件；UCRT? 为下划线导出候选(未读 PE) |
| 未初始化局部扫描 | `sh ./unisacc.com tools/uninit-scan.cx [-- FILE...]`；校验 `tools/check-uninit-scan.cx` | 流敏感 UNINIT/SUSPECT；正例 `int n=0, owner;`（e91eb31bcf^ kernel.cx）；第一版清单不进棘轮 |
| 改表验证闭环（A12） | `sh ./unisacc.com tools/a12-cycle.cx -- [--dry-run] [--only STEP] [--build B] [--flags F] [--src C [--reject]] [--neg43 EXE]` | gen -> tbl -> net -> UNINETB1(bin) -> e2/e1/sim 探针 -> decide CTX/RTC -> check-decide -> ratchets -> 冻结43例；G=B/tools 由 main 构造，缺则具名 MISSING；每步 `STEP name rc s receipt`，首败即停；UNISACC_WAVE_REUSE=1 仅子进程；检查 tools/check-a12-cycle.cx（批次 a12-cycle） |



纯 cx 密码学（无宿主库，均过向量）：`crypto.sha256`、`crypto.b64`、`crypto.hkdf`、`crypto.chacha20poly1305`、`crypto.gcm`（AES-128/256）、`crypto.ct`、`crypto.x25519`、`crypto.bn`、`crypto.der`、`crypto.p256`、`crypto.rsa`、`crypto.x509`、`crypto.tls13`、`crypto.record`——用 `#pragma unisa use crypto.<模块>`。

完整入口清单（含每条的说明、既有依据与覆盖边界）见 [archive/plans/2026-11-01/prd-c43-full-2026-11-01.md](archive/plans/2026-11-01/prd-c43-full-2026-11-01.md) 的「已实现能力速查」；这里只列常用入口，具体输入结果与覆盖边界以实际核对为准。

<a id="model-function-bytes"></a>
## 3. 结构组成（unisacc.com 的内部）

用 `sh ./unisacc.com tools/com-tree.cx` 看实时结构。当前公开发布是 0.0.49（APE `2d3852a8…`，6,501,348 字节，§9.1），其 com-tree 取数未在本文重跑（未测）；下面是 0.0.47 APE `fde2db18…`（6,215,778 字节）的实跑取数，只作结构示例；pkg.bin 1,665,282 字节（P4：40 模型、1823 阶段路由行、77 资源）。win/arm64 不在 APE 的 gzip 切片段里，作为独立发布资产 `slice-win-arm64`（3,367,424 字节）随发，见 [plans/release-notes-0.0.47.md](plans/release-notes-0.0.47.md)。

```
unisacc.com  6,215,778 B  sha fde2db18…
├─ head: PE 镜像 + shell 启动器      2,663,424 (42.8%)  win/x86_64 原样存；DOS stub 放 sh 启动器
├─ slice lnx/x86_64                    372,017 ( 5.9%)  gzip -> 解出 1,880,766 B
├─ slice lnx/arm64                     351,487 ( 5.6%)  gzip -> 解出 1,884,814 B
├─ slice osx/x86_64                    383,660 ( 6.1%)  gzip -> 解出 2,410,978 B
├─ slice osx/arm64                     379,590 ( 6.1%)  gzip -> 解出 3,071,458 B
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

| 流名 | 输入 -> 输出 | 功能 | 主要网 |
|---|---|---|---|
| **e2** | `src.c` -> `pp.text` | 预处理：续行/注释/指令/宏/条件/include | pp |
| **e1** | `pp.text` -> `tokens.typed` | 词法：关键字/标识/算符/字面量最长匹配 | lex |
| **e3** | `tokens.typed` -> `tape.text` | 语法+类型等合成段 -> tape（parse/type 等物理嵌此） | parse, type, scope, irsel, tyinfo, pfconv, binsel, prec |
| **e4** | `tape.text` -> `tape.text` | opt：peephole / 栈 -> 寄存器类改写 | opinfo, peep |
| **prune** | `tape.text` -> `tape.text` | 按根/导出剪不可达整函数 | （无 18 网，政策在代码） |
| **lower** | `tape.text` -> `target.text` | ABI/regmap/syscall/布局/reloc 降到目标 ops | abi, regmap, reloc |
| **elf** | `target.text` -> `image.bytes` | 编码 + ELF/Mach-O/PE 写出 | enc |

### 4.1 十八网（ask 事实表）

真源 `weights/gold/*.tsv`（`tools/gold-table.cx --check` 核对）：pp、lex、parse、type、scope、irsel、enc、reloc、tyinfo、pfconv、regmap、isel、abi、combo、binsel、prec、opinfo、peep。每张是「有限表 + 全键穷举 exact」，由 `seed/gold_<阶段>_label.c` 规则形式构造。**对照臂**（七流的 42 张网不读，变异实验改一键 0 网变化，2026-11-05 plans/gold-mutation-2026-11-05.md）：combo、isel、parse、pfconv、scope——仍进 weights/built.json 与 kernel/*.inc，供 18 网经典批次与 gold 对照工具使用，不删；parse 的语句分派已由 weights/stmt/stmt.tsv 决策表替代。其余 13 张（pp lex type irsel enc reloc tyinfo regmap abi binsel prec opinfo peep）及 weights/stmt、weights/arity 是七流出货主脊（改一键必改对应网）。

### 4.2 构造链

```
有限表（.tsv）-> 生成器（tools/*-gen.cx，带 --check）-> 网权重 -> 通用执行器（cx/core/kernel.cx 的 UNIKERN1）
```

- 有限表只经「事实表 -> 生成器（--check）-> 表」修改，不手改。
- 通用执行器：定宽运算、栈/存储、字节/帧、资源与接受/拒绝。语言、ABI 与布局语义仍由阶段规则拥有。
- 模型包 P3 = 32 模型 + 阶段路由 + 资源，装载由 `cx/runtime/model_decode.cx` / `packagemodel` 处理。

### 4.3 分层:booter / 基座 / 阶段权重 / 资源(架构方向,2026-11-02 主人定)

```
L0 booter(APE com,极小)   识别 OS/架构 -> 按 manifest 找到并校验基座与阶段权重 -> 交给基座执行
L1 基座(每架构一个文件)    通用执行器("权重换逻辑"的机器,即 kernel);外部文件,按内容哈希寻址
L2 阶段权重(7 流/18 网)    e2/e1/e3/e4/prune/lower/elf 各一个模型文件;外部文件,按内容哈希寻址
L3 资源                    头文件、事实表等
```

- **一切按内容哈希寻址;内置 pack 只是 resolver 的另一个来源**,与外部目录行为完全一致。
- `manifest.tsv`:`name kind(base|stage|resource) arch sha256 size order`。来源顺序:`UNISA_HOME` -> com 旁目录 -> 用户缓存 -> 内嵌 pack(UNIPKG1 footer)。命中必校验 sha256,不符具名拒绝,**不静默回退**到别的来源。
- **产品身份 = manifest 的 sha256(Merkle 根)**。取代 `pp_product_digest` / `ape-receipt` / `build.json` artifact 段等多处各算一份的口径(见 `plans/identity-inventory-2026-11-02.md`、`plans/identity-unify-2026-11-02.md`)。kernel context 仍独立于它(判断 kernel 可否复用,不随全树变),两代 GEN 互证仍保留。
- 目的:每一层单独固化、单独开发、单独测试;阶段测试=输入流字节 -> 输出流字节的 golden;换基座不动权重、换权重不动基座;慢车道缩成"只重算被改文件及其下游哈希"。
- 单文件发布(pack 内嵌)保留;外部目录形态用于开发。启动开销是主要风险(现热 `-run` ≈10ms),每步测量。
- 进度与分步:见 `agentwork/items.tsv`(B0 P3 explode/implode -> B1 resolver -> B2 阶段测试台 -> B3 基座外置 -> B4 booter APE 头 -> B5 身份收敛到 manifest)。
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

**已核事实（2026-11-08 更正）**：`ext/` 目录本身不在 `seed/ident.c` 的目录集内（`exec src kernel include weights seed cx` + `iterate/kernel/typekw.tsv` + `tools/` 10 文件，共 1586 文件），**但 `cx/mem`、`cx/text` 里有 8 个符号链接指向 `ext/memx`、`ext/textx`（反向亦有互指），`ident` 跟随文件链接，故这两个 ext 目录的内容经链接参与源 SID**；`ext/sysx`、`ext/crypto` 不参与。`ext/` 不随 APE 发布（产品二进制无其符号）。W3（`plans/w3-memtext-2026-11-02.md`）判定 arena/slice/grow/num/build/fmt/deflate 七模块产品链路引用为 0、归 ext、原路径留别名，当前状态即该终态；`cxroot.tsv` 只映射 `sysx`、`crypto`，`memx/textx` 经别名使用（`use mem.*`/`use text.*`，`use text.fmt` 169 处），**不是孤儿**。`sysx`/`crypto` 由 75/14 个检查守住。**风险**：互指符号链接让同一文件有两个身份、Windows 无符号链接不安全；冻结路径须含 `ext/memx ext/textx`（已补）。**待办**：0.0.49 评估把真身/别名收敛为单向并去掉互指（先读 W3 阶段二、三终态）。内部包接口摘要与依赖设计见 `plans/v0.0.48-unisacc.md` §12–§13（C0 分治身份）。
- `cx/core/`（L1）：执行器（kernel/execute/stack/output/bytes/memory）。
- `cx/driver/`（L1）：path/compat/args/output/deps/cli/route。
- `cx/runtime/`（L1）：model_decode/package_*/stream_*/source_frame 等。
- `cx/compiler/`（L1）：cxmod（模块解析）/asmtext（汇编文本）/streamname/netname 等名表。

分层铁律：L0 才对接 libc/POSIX，上层禁 `#include` 和裸 libc（`tests/cxinclude.cx`、`tests/cxlibc.cx`、`tests/cxraw.cx` 三个棘轮，只降不升）。

## 6. 事实表与权重（exec/ 与 weights/ 与 seed/）

- `exec/facts/`：编译各阶段的**事实表**（pp-targets、lex-consts、lower-abi、image-elf/macho/pe…），经 `tools/facts-gen.cx` 生成，只经「事实表 -> 生成器 -> 表」修改。
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
| `tools/` | 开发工具（约 565 个 .cx，个数以 `ls tools/*.cx` 为准：生成器/棘轮/refactor/symindex/all-waves…）；`tools/cxlib/` 为工具共享库（rechunk、netpack、resolver 等，不进源 SID） |
| `ext/` | 扩展包：sysx、memx、textx、crypto、tsvx（TSV 读写与 merge3）；不进源 SID，memx/textx 经 cx/mem、cx/text 符号链接间接参与 |
| `build/` | 构造图与前提规则表：graph.tsv（203 节点构造图）、hardwire.tsv（第二真值硬连线模式）、preflight-rules.tsv（0.0.50 撞到的隐性前提，0.0.51 PF 预检输入）；构造链维护者所有，不进源 SID |
| `plans/` | 计划与设计（约 120 个）：现行 v0.0.50/51/52-unisacc.md、y-design-050.md、rechunk-design.md、standing-goal.md、release-notes-0.0.45 至 0.0.50；已完成的归档到 archive/plans |
| `docs/` | 规格/术语与目录地图（docs/REPO-MAP.md、glossary.md、toolchain.md、tapebin-v1*、docs/cx/；spec.md 在根） |
| `research/` | 论文五线（A 流网换逻辑、B ujs、C 管道、D cx 模块化、E 系统构表） |
| `archive/` | 归档：reference-c/reference-py（旧实现）、testing-c43-legacy（旧 1333 项套件）、plans/（历史计划）、prd 历史 |
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

当前已发布：**0.0.51 已于 2026-11-10 公开**（tag `v0.0.51` 指向发布提交 Y `cbef4eca53`，Latest）。APE sha256 `d5d6d0023898ea007fbb975f3118c8cf7e0fcd4c735fab50c78bdc0d56594b4a`（6529460 字节），构造源 `bc5b04bf14`、源 SID `a811885ea6f94dd59cae599a40a45e85dedec1755251245a3e56dbb70e5d5c0c`；**Y-0 产品候选（e3 阶段检查点）首次进入发布**，产品源自 bc5b04bf14 起 0 个提交变动。链证据：六片 g1==g2==g3、kernels g2==g3、fp-compare slices/objects/kernels/product 全 same（osx/win 经分相镜像，slice-osx-* 为 staged-from-image 字节拷贝，待根治）；cache-manager 由 cache-retain 构造，与 0.0.49 借用件同 sha；H8 `release-run --pack` 复现 APE 逐字节相同。六目标原生 CI **QUALIFIED 42/0**（运行 42809902396，测试提交 X' `30d1c3cd67`）；**s9 三档未跑，全批资格 NOT_QUALIFIED**。发布前拦下一次身份错误（打包输入误带 0.0.50 的旧源 SID e764a816，已按 identity-helper 复算的 a811885e 更正）。发布说明：[plans/release-notes-0.0.51.md](plans/release-notes-0.0.51.md)。

上一发布（0.0.50）：**0.0.50 已于 2026-11-10 14:58 公开**（tag `v0.0.50` 指向发布提交 Y `31c5886469`）。APE sha256 `76ed4200aaaf71b2ac9dd13e6dde6ad31632016ddea8fe43893a4ffe32484967`（6501348 字节）；**后备候选**：产品字节由 0.0.49 冻结源 `979b56e4` 的同一组 g3 六片 + P4 `56efea53` 以 version=0.0.50 的候选表重打包（候选表 commit 列=构造源 979b56e4，真值不改写），唯一变更输入是版本串；六目标原生 CI **QUALIFIED 42/0**（运行 42767415154，提交 X `114ef751be`，约 10 分钟）；s9 三档未跑。产品候选 Y-0（`77b5e559b3`，e3 阶段检查点）在候选链中暴露 worker.o 的 `setpgid` 降级为 libc 符号的产品回归（`out/c050-y0/y0-setpgid-regression.md`，items `Y0-SETPGID-REGRESSION`），**不进 0.0.50**，留 0.0.51。发布说明：[plans/release-notes-0.0.50.md](plans/release-notes-0.0.50.md)。独立切片版本注入新工具 `tools/version-inject.cx`。

上一发布：**0.0.49 已于 2026-11-10 公开**（tag `v0.0.49` 指向提交 `35cfdd1e`，非 draft 非 prerelease）。APE `unisacc.com` sha256 `2d3852a8404e1ba372de0fd28e69d6da7d9412d7b8b259d0d2e36593dac8e44d`（6501348 字节），冻结源 `979b56e4`、源 SID `e764a816…`（版本串已移出 SID，提交 c66fa850e6）、构造器 `8224ff13…`；同候选六目标原生 CI **QUALIFIED 42/0**（运行 42661106290，测试提交 e5b26c5c；首轮 42659892210 因资格工具未迁移版本源四格 rc2）；s9 **仅提交档 96/96**，daily/release 档未跑。发布链实际走法：提交 X `e3fc485793`（候选表换 0.0.49，ci_run_id=0）触发 CI，提交 Y `35cfdd1e3a` 回填 run id。发布说明：[plans/release-notes-0.0.49.md](plans/release-notes-0.0.49.md)；发布手册与 0.0.49 教训：[prd_release.md](prd_release.md)；0.0.49 收尾的逐条记录：[prd_hist.md](prd_hist.md)「0.0.49 收尾」。

当前版本：**0.0.52，主题「兑现增量与自含，模型层度量先行」**（计划 [plans/v0.0.52-unisacc.md](plans/v0.0.52-unisacc.md)，经 fable 咨询调整：A 段兑现 0.0.51 欠账为门槛，A1 G2 增量 + A3 chain_sha 收窄 + A7 s9 夜跑三项过门后 B 段模型层 M1-M4 才开工，否则顺延 0.0.53；A0 产品全景表为第一优先，主人关切「每一层/切片/流/网的功能与大小可追踪并与 prd 对齐」）。0.0.51 尾部在途：链收尾（fresh-start 已落、from-image 命名、D2）、不间断从零复现并计量墙钟、noborrow、s9 提交档用 native 缓存真跑、新 .cx 本地自查工具、产品全景表阶段一。任务状态以 `agentwork/items.tsv`、进度以 `agentwork/ledger.tsv`、相位耗时以 `agentwork/phase-times.tsv` 为准，本节只写当前节点。

已上 main（`git log 35cfdd1e..origin/main`）：
- L 两步：`87016b122d`（`tools/cx-lint.cx` 原型，check 19/0）、`8a07e8e119`（R1 数据流判据、`lint:allow`、R2 接入 precommit 拦截，check 41）；顺带修出潜伏缺陷 `bf395ec282`（check-utf8 子进程缺 `--`）。
- G1 第一步：`84df77f4b2`（`build/graph.tsv` 203 节点/833 边、`build/hardwire.tsv` 32 条、`tools/graph-check.cx` check 9/0）。
- 0.0.49 工具总补丁 `ba945f3238`（whole-front 分相、chain-step 不截 argv、release-build 容量 API）；phase-times 回写 `022caf53b1`（131 作业波 276.89s、42 网全键 7.18s、S9 255.13s+61.94s）。

在途（未上 main，证据在各窗口 out/ 与账本）：
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
layer	src.drive	src	drive	39724	source-bytes	-	file=src/drive.cx	stat src/drive.cx	77b5e559b3822b0a5244b6305a05b26b0e0a3202	UNMEASURED	UNMEASURED	UNMEASURED
layer	src.bind	src	bind	205	source-bytes	-	file=src/bind.cx	stat src/bind.cx	d4189306a6e4872dfc274518b062342cb4a2780c	UNMEASURED	UNMEASURED	UNMEASURED
slice	lnx/x86_64	ape	embedded target image	1943158	target-image-bytes	18993fd7d1e39f873cceb6f0414daff4a14dcbafa843d4bc40dace4597f56d7b	format=ELF;unattributed_bytes=1943158;attributed_bytes=0;unattributed_frac=1;note=module-byte-map-not-exclusive	sh ./unisacc.com tools/logic-distribution.cx -- --ape unisacc.com	18993fd7d1e39f873cceb6f0414daff4a14dcbafa843d4bc40dace4597f56d7b	UNMEASURED	UNMEASURED	UNMEASURED
slice	lnx/arm64	ape	embedded target image	1951302	target-image-bytes	5e33829af1aed1c085612ebef3457c7fa77cc2a483561f7d68a9aa6fc26f0dfc	format=ELF;unattributed_bytes=1951302;attributed_bytes=0;unattributed_frac=1;note=module-byte-map-not-exclusive	sh ./unisacc.com tools/logic-distribution.cx -- --ape unisacc.com	5e33829af1aed1c085612ebef3457c7fa77cc2a483561f7d68a9aa6fc26f0dfc	UNMEASURED	UNMEASURED	UNMEASURED
slice	osx/x86_64	ape	embedded target image	2493538	target-image-bytes	2f570f2ce5f7e6bfa9ae068d0d49496d5eb4021e5bc406f4a698801af2276d25	format=MACHO;unattributed_bytes=2493538;attributed_bytes=0;unattributed_frac=1;note=module-byte-map-not-exclusive	sh ./unisacc.com tools/logic-distribution.cx -- --ape unisacc.com	2f570f2ce5f7e6bfa9ae068d0d49496d5eb4021e5bc406f4a698801af2276d25	UNMEASURED	UNMEASURED	UNMEASURED
slice	osx/arm64	ape	embedded target image	3170530	target-image-bytes	d569482af4dca12059b6a3cf7cbc2249549fde79dec9630dade471616371120c	format=MACHO;unattributed_bytes=3170530;attributed_bytes=0;unattributed_frac=1;note=module-byte-map-not-exclusive	sh ./unisacc.com tools/logic-distribution.cx -- --ape unisacc.com	d569482af4dca12059b6a3cf7cbc2249549fde79dec9630dade471616371120c	UNMEASURED	UNMEASURED	UNMEASURED
slice	win/x86_64	ape	embedded target image	2760192	target-image-bytes	88d7a715e7bb7188f02652118fa479465b9051feb876a6a9584025ff0a2c6f7a	format=PE;unattributed_bytes=2760192;attributed_bytes=0;unattributed_frac=1;note=module-byte-map-not-exclusive	sh ./unisacc.com tools/logic-distribution.cx -- --ape unisacc.com	88d7a715e7bb7188f02652118fa479465b9051feb876a6a9584025ff0a2c6f7a	UNMEASURED	UNMEASURED	UNMEASURED
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
- G0 一条命令复现：六片 g1 已全部在私有根逐字节复现（lnx/x86_64 8224ff13、lnx/arm64 f7ddb573、osx/x86_64 eddda34a、osx/arm64 e1087d14、win/x86_64 64782d36、win/arm64 2f520450；Linux 两片与 `agentwork/phase-times.tsv` 第 51/53 行一致，其余四片以派工回执为准），g2/g3 在跑。
- Y（e3 产品原生阶段化）设计结论（fable，/tmp/y-design.md，已写入计划 §2 Y 行）：「由 e2 推导未定义符号」不成立（侧车需 e3 的 UD 可达性/类型 kind/ccw 计数）；改为首遍 e3 tape 作检查点 + 重放只编桩单元 + fwm_merge，拆 Y-0 -> Y-A -> Y-B -> Y-C。
- D（遗留债清零）、R（一键发布）、S（S9 单 step 剩余项）、自动再分块（rechunk）、根无关性检查工具均在途；`candidate-chain`、`root-canon`、`rechunk`、`release-run` 尚不在 main，不写入 §2。

三个最大阻塞（对应计划 §0 目标与 §5 风险）：
- **冻结 -> 可发布墙钟**：0.0.49 实际十余小时（计划 §0；端到端计算之和约 31 分钟，见发布说明），目标 ≤20 分钟；取决于 G0 持久检查点 + R 一键发布落地。风险：六片并行 CPU 争用撞 60 秒闸，预案按实测降并行度、贴线相位串行。
- **Mac/Win 相位贴线与方案 X 的工具层依赖**：0.0.49 Mac Intel B 相 43.1s 贴 45s 闸（发布说明「已知限制」）；根治靠产品候选 Y。风险：Y-A tape 合并不等价（取址函数、static 隔离、ccw 顺序），预案同源中等程序 cmp0 卡死、不等则保留 X。
- **全批资格与第二真相源**：s9 daily（约 544 项）/release 档自 0.0.47 起未跑，全批仍 **NOT_QUALIFIED**（计划 §1b 要求 0.0.50 至少跑一次 daily）；版本/构造器/相位次序硬写 0.0.49 返工 ≥5 次（计划 §0），已由 `build/hardwire.tsv` 32 条计数、`cx-lint` R1 只报告，归零未达成。风险：图描述与工具实际读集不一致，预案 `graph-check` 进 commit 档。

历史完成记录与证据不再追加到本文件：见 [prd_hist.md](prd_hist.md)；新记录写 `agentwork/ledger.tsv`。
<a id="memory-palace"></a>
### 9.2 记忆宫殿（索引；此处是唯一索引，正文各节只指向本表）

| 房间 | 当前事实 | 入口 |
|---|---|---|
| 任务队列 | `agentwork/items.tsv`（owner 列=tmux 窗口名；只有 READY 项被 omp-workflow 自动续跑）；进度追加在 `agentwork/ledger.tsv` | [agentwork/WORKFLOW.md](agentwork/WORKFLOW.md) |
| 当前发布 | 0.0.50 已公开（见 §9.1；APE 76ed4200、CI 42767415154 QUALIFIED 42/0、产品字节同 0.0.49 源只换版本、s9 未跑）；旧候选资格不迁移 | [0.0.50 发布说明](plans/release-notes-0.0.50.md)、[prd_release.md](prd_release.md) |
| 当前版本 | 0.0.51「增量、自含、一键」；第零批 W（fable）/PF（opus）/RA（sonnet）/LD（zai）并行中；Y-0 setpgid 回归（grk 定位）为产品候选前置 | [计划](plans/v0.0.51-unisacc.md)、`agentwork/items.tsv` |
| 新入口 | `src/unisacc.cx` 六层（host/packmodel/executor/stages/drive/bind）；`src/core`、`src/runtime` 指向 `cx/` | [plans/cx-modules.md](plans/cx-modules.md)、§5 |
| 流/网 | 7 流 + 18 网 | §0、§4 |
| 测试入口 | `sh ./unisacc.com tests/run.cx -- --smoke/--all/--list` | §7.1、`tests/batches.tsv`、`tests/coverage.tsv` |
| 分层 | L0 `cx/sys`·`cx/mem`·`cx/text`；包层 `ext/crypto`；L1 `cx/core`·`cx/driver`·`cx/runtime`·`cx/compiler` | [plans/cx-layers.md](plans/cx-layers.md)、§8 |
| 棘轮 | 13项，以`tools/ratchets.cx`末行为准；新增第14号net-deps，toolsloc已退役；工具行数只统计 | `tools/ratchets.cx`、§2.2 |
| 事实表 | 事实 -> 生成器（须带 `--check`）-> 产物 | `exec/facts/`、§2.4 |
| 构造图 | `build/graph.tsv` 203 节点/833 边 + `build/hardwire.tsv` 32 条第二真相源模式；`graph-check` 9/0（84df77f4b2） | §2.2、[plans/v0.0.50-unisacc.md](plans/v0.0.50-unisacc.md) G1 |
| .cx lint | `tools/cx-lint.cx` R1（第二真相源，只报告）/R2（子进程时限与 `--`，precommit 拦截）；check 41（8a07e8e119） | §2.2 |
| 静态预检 | 模式/引用/兜底/SB 合同先查；动态及未证明性质单列 PARTIAL，不能替代真实 gen/新包/slice | `tools/tsv-lint.cx`、`tools/chain-precheck.cx` |
| 结构盘点与普查 | 源码/模型分母与六片模块账全部实核守恒；最终模式全部登记实跑；C47已DONE，行数门按主人新裁定退役，不称全局棘轮或CI绿 | §9.6、`tools/module-byte-map.cx`、[六片账本与材料身份](research/c47-logic-distribution-20261108/)、[plans/slice-shrink.md](plans/slice-shrink.md) |
| 归档 | 旧 C/Python 实现、旧 1333 项套件、历史计划与 PRD | §10、`archive/` |

### 9.3 往期发布与历史（索引）

逐版发布历史见 [prd_release.md](prd_release.md) §往期发布；0.0.43 见 [plans/release-0.0.43.md](plans/release-0.0.43.md)，0.0.45 见 [plans/release-notes-0.0.45.md](plans/release-notes-0.0.45.md)，0.0.48 见 [plans/release-notes-0.0.48.md](plans/release-notes-0.0.48.md)，0.0.49 见 [plans/release-notes-0.0.49.md](plans/release-notes-0.0.49.md)。0.0.47 逻辑分布普查结论与 0.0.48.1/0.0.49 收尾记录已迁入 [prd_hist.md](prd_hist.md)。

### 9.4 路线
- **0.0.43 – 0.0.49**：已收口并发布（计划见 `plans/v0.0.4x*.md`，当前发布 [plans/release-notes-0.0.49.md](plans/release-notes-0.0.49.md)）；agent harness（cx-csih）仍归 csih 车道。
- **0.0.50**：主题「构造即图，阶段原生」，见 [plans/v0.0.50-unisacc.md](plans/v0.0.50-unisacc.md)；产品候选顺序 Y -> B2 -> H，工具包 G0 -> G0b -> G1 -> G2。
- **0.1.x**：Clang/GCC/TinyCC 接入 `.cx`（共用前端 + 后端插件）。

<a id="optimization-leads"></a>
### 9.5 优化线索

已登记到各版计划与 `agentwork/items.tsv`，入口见 [plans/v0.0.50-unisacc.md](plans/v0.0.50-unisacc.md)（§1 设计判断、§1b 预见）。

## 10. 子文档与归档索引（树）

```
prd.md                 当前状态（本文件，只写当前事实）
├─ prd_release.md      发布与 CI 手册、0.0.48/0.0.49 教训、往期发布身份
├─ prd_hist.md         历史完成记录与证据（逐字迁出，只读；新记录写 agentwork/ledger.tsv）
├─ plans/              各版计划与发布说明
│  ├─ v0.0.50-unisacc.md         当前版本计划（§0 目标、§2 工作包、§5 风险）
│  └─ release-notes-0.0.49.md    当前已发布版本说明
├─ build/graph.tsv     构造图事实表（G1；graph-check 读）
├─ agentwork/          任务看板 items.tsv、账本 ledger.tsv、phase-times.tsv、WORKFLOW.md
└─ archive/            旧实现与旧文档（见下）
```

子文档存在性核对（2026-11-10 私有克隆 `ls`）：[plans/release-notes-0.0.49.md](plans/release-notes-0.0.49.md)、[plans/v0.0.50-unisacc.md](plans/v0.0.50-unisacc.md)、[prd_release.md](prd_release.md)、[prd_hist.md](prd_hist.md)、[build/graph.tsv](build/graph.tsv)、[docs/REPO-MAP.md](docs/REPO-MAP.md) 均存在。


- [archive/plans/2026-11-01/prd-c43-full-2026-11-01.md](archive/plans/2026-11-01/prd-c43-full-2026-11-01.md) —— 本文件重写前的完整旧版（含思维树/记忆宫殿全流水、能力速查全文、§3 架构全文、§5 路线全文、§6 未解决问题）。
- [archive/prd-c43-history-20261030.md](archive/prd-c43-history-20261030.md) —— 0.0.43 历史收口。
- [archive/prd-release-and-development-history-20261001.md](archive/prd-release-and-development-history-20261001.md) —— 逐版发布身份。
- [archive/testing-c43-legacy/](archive/testing-c43-legacy/) —— 旧 1333 项测试套件（已停调度）。
- [archive/reference-c/](archive/reference-c/)、[archive/reference-py/](archive/reference-py/) —— 旧 C/Python 参考实现。
- [archive/plans/](archive/plans/) —— 历史计划与设计探针。


0.0.48.1 OSX-LAUNCH-SLOW / OSX-SIGN-ROSETTA 与 0.0.49 收尾的逐条完成记录已逐字迁入 [prd_hist.md](prd_hist.md)「0.0.49 收尾」；本文件不再保留。
