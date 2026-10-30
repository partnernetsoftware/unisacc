# unisacc —— 当前状态、架构与路线

> prd 只写**当前与将来**；发布身份与过程记录在 [历史索引](archive/prd-release-and-development-history-20261001.md)。条款全文在 [spec.md](spec.md)（代码注释引用的 `[S-9]`、`[W-16]` 等编号都在那里，只增不改）；未排期构想见§5，旧稿在 [archive/plans/ideas.md](archive/plans/ideas.md)；已发布版本的计划、叙事与证据在 [archive/](archive/)；目录归属见 [ARCHITECTURE.md](ARCHITECTURE.md)。新知识和决定先写进这里再做别的。

## 当前导航

- **[概念与术语](#0-概念与术语先读歧义以本节为准)**：§0，流 / 网 / 段 / 阶段四个词各只有一个意思；六条常见误解逐条对照。
- **[流/网合同三表与可核验进度](#flow-net-contracts)**：七流、十八网、关系表及是否封存；有封不等于已冻。
- **[思维树](#work-tree) → [记忆宫殿](#memory-palace)**：先取目标、当前节点、事实与下一动作。
- **[已实现能力速查](#implemented-usage)**：默认动态执行、tape 输入、六目标镜像、`.cx` 依赖计划。
- [当前版本与有效计划](#current-plans) · [当前架构](#pipeline-design)
- [路线](#5-路线) · [未解决问题](#6-未解决问题) · [归档索引](#7-归档索引)
- [0.1.x：其他 C 编译器接入 `.cx`](#53-01x其他-c-编译器接入-cx)
- **[cx 模式](docs/cx/prd.md)**：`gcc_cx.com` 与 `clang_cx.com` 是 APE 六格。现行阅读含宿主 `-o`、模块 lint、包清单与 CXIF v0。
- **[论文五线](research/README.md)**：A 流网换逻辑；B＝`ujs/`；C 管道方法；**D＝`.cx` 模块化＋内存安全**；E 系统构表。CX 叙事归 D，不塞进 A。

<a id="implemented-usage"></a>

## 已实现能力速查

下表是现有产品入口，不是待研究路线。开始工具开发、替代脚本或派发子代理前，先按这里选择现成用法；需要核对的是具体输入的结果与覆盖边界，不重复研究这些入口是否存在。

| 动作 | 现成用法 | 说明与既有依据 |
|---|---|---|
| 动态执行 C／`.cx` 工具 | `./unisacc.com FILE.cx -- ARGS` | 默认就是 `-run`；`.c` 同样适用。见本篇§3.11、[README 用法](README.md)及[`.cx` 草案](docs/cx-spec-draft.md)。 |
| 显式动态执行 | `./unisacc.com -run FILE.cx -- ARGS` | 保留的显式写法；日常推进直接执行源码。 |
| 从 tape／tapebin 写目标镜像 | `./unisacc.com INPUT.tape -b OS/ARCH -o OUTPUT` | `.tapebin` 也有现成入口。支持域及具名拒绝仍生效；见 [README](README.md)、[驱动](exec/c/compiler.c)及[当前优化Tape合同](tests/optimization.cx)。 |
| 写六目标程序 | `./unisacc.com FILE.c -b OS/ARCH -o OUTPUT` | 目标为 `lnx`／`osx`／`win` × `x86_64`／`arm64`；跨目标写出与目标机器原生执行分别记证。 |
| 查看 `.cx` 模块依赖计划 | `./unisacc.com -cx-plan FILE.cx` | 复用现有模块入口与[当前模块ABI用例](tests/library-abi.cx)，不另造模块扫描器。 |
| 用系统 GCC／Clang 把 `.cx` 编成普通可执行文件 | `sh ./gcc_cx.com FILE.cx -o OUT`；Clang 用 `sh ./clang_cx.com`。Windows 上是 `gcc_cx.com FILE.cx -o OUT` | PATH 上的 gcc 或 clang 由 `tools/build-cxcc-host.cx` 在该机链出驱动，不入库。Linux 与 OSX 用 `sh ./gcc_cx.com`；Windows 的 `gcc_cx.com` 是本机程序。本机只跑过 Linux。不调用 `unisacc.com`。抹掉首行 `#!`。无 `use` 时一次执行系统 `gcc` 或 `clang -std=c99 -x c -include include/cx.h`；没有后续非选项输入时不追加 `-x none`。有 `#pragma unisa use` 时在驱动进程里按 `src/cxmod.c` 展开，再按每个单元编译链接。`-run` 尚未做。`gcc_cx.com` 1606288 字节，`clang_cx.com` 1606304 字节，是六格 APE。本机这次构建跑过 Linux x86-64 那一格；其余四份切片只写出。进程 API 是按需模块 `tools/cxproc.cx`。不授予已检查身份。见 §5.3。 |
| 拒新的模块 `#include` | `sh ./unisacc.com -run tests/cxinclude.cx -- --check` | 模块 `.cx` 新文件必须为 0；`tests/cxinclude.tsv` 旧债只降不升。`--write` 遇到新行或更高计数就拒绝落盘。`include/` 与 `tools/cxcc-hostinc/` 仍可用 `#include`。见 [plans/cx-include-lint.md](plans/cx-include-lint.md)。 |
| 包清单与导入身份 | `sh ./unisacc.com -run tests/cxpack.cx -- --fixtures` | `cxpack.tsv` 将模块名钉到路径。已装 e955 的 CXIF 在存在匹配文件行时只导出公开列表中的非 static 函数；清单缺席或无匹配行仍保留全部非 static 函数的兼容行为。清单超限、目录或其他读取错误具名拒绝，不扩大公开接口。类型声明仍沿用文本 CXIF v0，尚非结构化 ABI 或编译期所有权。见 [plans/cx-pack.md](plans/cx-pack.md) 与 [缺陷证据](plans/defects-c43.md)。 |
| 核对 CXIF v0 文本 | `sh ./unisacc.com -run tests/cxif.cx -- --fixtures` | 现场 `-cx-interface` 必须是 banner、守卫、类型、原型和 `#endif`。函数体、`static`、缺 `#endif`、版本不是 v0 都退出 1。效果、布局和所有权字段不在 v0 里。见 [plans/cxif-v0.md](plans/cxif-v0.md)。 |

新动态逻辑优先由 `unisacc.com + .cx` 完成，减少 `.sh`，不新增临时 `.py`。旧脚本尚未迁移的部分如实记为依赖，不把 `.cx` 转调脚本记成迁移完成。经典 C 参考与模型产品是不同实现；对拍前确认实际两端身份，不能笼统判成“对自身验证”。入口已实现不等于任意输入已验证，继续保留原断言、拒绝和预算。

本机开发控制器可用 `tools/dev-run tests/run-batch.cx -- ROOT ABS_OUT BATCH_ID` 复用原生 ELF 代码；它每次仍调用已装产品实际运行所选 SOURCE 批，不能把它产生的开发速度当作产品默认 `-run` 提速或发布资格。新检出先按 `plans/v0.0.43.md` 重建忽略的启动 ELF；当前仅 Linux/x86_64 可用。

## 0. 概念与术语（先读；歧义以本节为准）

> 本节只做一件事：让「流 / 网 / 段 / 阶段」四个词各自只有一个意思。详细版与逐处核实见 [docs/glossary.md](docs/glossary.md)。

### 0.1 五层，每层只有一个名字

| 层 | 唯一名 | 含义 | 数量 | 真源 |
|---|---|---|---|---|
| L1 | **Stream**（别名 Flow） | 字节管道阶段：输入格式 → 输出格式 | unisacc **7**（e2/e1/e3/e4/prune/lower/elf）· m4/ujs **4**（lex/front/select/encode）· tdesc **3**（declare/context/size） | `exec/pipeline/image-stages.tsv` · `adapter/ujswasm/tables/contract.tsv`（原 `m4/contract.py FLOWS`，已归档） · `flownet/tdesc/contract.tsv` |
| L2 | **Net** | ask 事实网：一张有限表 + 全键穷举 exact | **18**（8770 键）· **10**（3677 键）· **3** | `weights/gold/*.tsv`（权威；规则形式 `seed/gold_<阶段>_label.c`，由 `tools/gold-table.cx --check` 核对）· `adapter/ujswasm/ujsgold.cx` → `adapter/ujswasm/tables/*.gold.tsv`（原 `unisa/gold.py`、`ujs/seed/construct/gold.py`，均已归档） |
| L3 | **Segment** | 训练合并段（多网合一的 masked 多任务模型） | 3（s1/s2/s3） | `archive/reference-py/unisa/control/segment.py`（负结果证据，已归档） |
| L4 | **Book phase** | 一次推导中 ask 的记账阶段 | 由 contract 声明 | `flownet/contract.cx` 的 `phase` 行 |
| L5 | **Release phase** | 发布流程阶段 | 10 | `archive/reference-py/release/tools/stagelog.py`（2026-10-30 随发布队列链归档） |

### 0.2 六条常见误解（逐条对照，不要按直觉读）

| 如果你这样理解 | 正确的是 | 为什么容易错 |
|---|---|---|
| 「7 流」与「18 网」是同一批东西的两种叫法 | 流是**字节管道阶段**，网是 **ask 事实表** | 旧句「勿混」只否定了互换，没说明二者各是什么 |
| `class Stage`（`unisa/gold.py`，已归档到 `archive/reference-py/`）表示流 | 它表示**网**（18 张的规格定义） | 类名抢占了「流」的词 |
| `unisa/control/net.py`（已归档）的 `Net` 是事实网 | 它是**神经网络模型**（TableNet / StageNet / UnisaNet） | `net` 一词被两处占用 |
| m4 的「4 流」指 4 张决策网（lex/parse/isel/enc） | 4 流是**四个字节段**：lex / front / select / encode；决策网恰好也是 4 张，是数字巧合 | 两个「4」属不同维度 |
| 所有 `phase` 都是编译阶段 | 三义：**book phase**（记账）/ **release phase**（发布）/ 历史 `compiler phase`（实为 segment） | 同一个词三次入场 |
| `STAGE_NETS` 是「所有与流相关的网」 | 仅 **isel、abi** 两张不在 `TABLES` 里的网（15 + 2 + 1 = 18） | 名字里的 stage 是历史包袱 |

### 0.3 禁用写法

- 裸 `stage`：一律写 `stream-stage`（指流）或 `net`（指网）
- 用 `net` 指神经网络：一律写 `model` / `neural net`
- 用裸 `PHASES` 指发布阶段：写 `release phase`（代码侧待改 `RELEASE_PHASES`）
- 单说「4 流」：必须带上下文——「m4 的四个字节段」或「4 张决策网」

### 0.4 与 §3 的关系

§3.0a 七流表、§3.0b 十八网表、§3.0c 关系总表都以本节定义为前提；三表与本节冲突时，**以本节为准**，并同步修表。

### 0.5 一句话

**流 = stream，网 = net，训练合并 = segment，记账 = book phase，发布 = release phase。**

### 0.6 RI 的固定含义

本文的 **RI 是 ResourceInput（具名资源输入）**，对应exec/c/run.c中的ResourceInput及RI/NRI，包含名称、字节和长度、存在性／空值及跨流身份绑定。不是“runtime integrity”的缩写。模型加载器的坏头／越界／非法动作拒绝另属负例与运行完整性合同，仍须测试，不能拿资源接口测试代替。新清单原ri行的英文说明误写为runtime integrity，现纠正；完整覆盖义务不减少。

## 1. 命题与产品

- **产品**：一个文件 `unisacc.com`（APE），在 {Linux, macOS, Windows} × {x86-64, arm64} 上运行，自己写出六个目标的可执行文件（ELF、Mach-O、PE；映像不经汇编器和链接器），0.0.17 起参考编译器可写三种格式的可重定位 `.o`，产品自带链接器与 `ar`；当前模型产品对象输出只支持Linux两目标，Mach-O/COFF对象逐项具名拒绝（§3.15及README边界），并能自举：宿主构建的 `unisacc-seed.com` → 种子自建 → 再自建，三个 sha256 相等（N22）。两代产品只叫 `unisacc-seed.com` 与 `unisacc.com`。
- **语言**：C99 = ISO/IEC 9899:1999 + TC1–TC3（WG14 N1256）。覆盖度来自逐条款账本 [归档条款账本](archive/testing-c43-legacy/tests/c99/clauses.tsv)（仅历史覆盖义务，不再调旧gate；当前入口见[新清单](tests/coverage.tsv)），不作主张；缺口在 README 限制表逐条点名。
- **方法**：编译器里每个表状决策（词法类、预处理指令、优先级、类型、指令选择、ABI、窥孔……）由**构造**出来的小整数网络回答，权重从真值表派生、在表的全部定义域上**穷举验证**（不是训练）；结构性粘合是经典代码。两条路线：C 参考编译器（`unisacc.c` + `src/`，行为裁判与回退）与模型产品（`exec/` 各阶段网络 + 通用执行器），逐字节对拍（`exec-chain`、`closure`、`stages`、difftest 两侧）。
- **零 Python 依赖（硬目标；主人 2026-10-24 再次重申）**：构建、闸、验证链中 **100% 不依赖 Python**，牢固使用自己的 `unisacc.com + .cx` 体系；新增与修复路径优先原生实现，不新增 Python 算法或调用点。历史候选曾完成N22三SHA相等；当前重构候选完整三阶段链仍待验，依赖清零也尚未完成：归档依赖审计 `archive/testing-c43-legacy/tests/zeropython.tsv` 记直接调用声明 **1771**（真实执行标记1739、数据32；解析入口复用使静态标记少4，原外部参考依赖未消除）、`import unisa` 文件 **92** 个、`.py` **409** 个。仅用 `.cx` 分批调用原 Python 只算调度改进，不算消除运行依赖；原生替换须实际自举运行，并保留完整行为与拒绝、恢复合同。三者同时清零后，`unisa/` 才移入 `archive/`（不删，仅作人工参考，不参与任何链）。
- **交付物**：签名的 `unisacc.com` 与公证的 macOS dmg（公开资产只有这两个，主人 2026-09-30 裁定）；中间形式 `.tape`/`.tapebin`（[docs/tapebin-v1.md](docs/tapebin-v1.md)，内容寻址、C/Python/产品三方编码逐字节同）。
- **公开入口仓**（主人 2026-10-05，拟注册商标 UNISA）：[partnernetsoftware/unisa](https://github.com/partnernetsoftware/unisa)，只收副本：unisacc、ujs 的最终发行包（`releases/`，定期从本仓 release 抄过去），产品介绍（`docs/`）与论文（`papers/`，不定时抄）；以后做 unisacc 的包管理入口和 agenterm 的“插件与应用”入口。开发不在那里。

### 范式层：表、执行器、流推导与晚绑定接线

unisacc 的设计应按可组合的范式组织，而不只按编译器特例记账：**表表达决策，共用执行器提供机制，显式流接口连接阶段，晚绑定合同隔离声明需求与宿主供给。** 各刀优先补强这四者之间可核验、可复用的边界；具体语法或平台修复仍须完成，但应说明它修复了哪类共同规则，避免复制分支替代抽象。

```text
范式与复用
├─ 有限表：阶段的决策、控制、目标规则与拒绝定义
│  └─ 构造与对拍：有限决策域可穷举，不等于完整语言域已穷举
├─ 共用执行器：定宽运算、栈/存储、字节/帧、资源与接受/拒绝
│  └─ 机制共用，语言、ABI与布局语义仍由阶段规则拥有
├─ 显式流接口：主字节流 + 位置/属性 + 具名资源 + 状态/诊断
│  └─ 串联 / 验域后变换 / 有序pass链；版本、状态作用域与提交点明确
└─ 晚绑定接线：声明需求 → 稳定ID/未绑定片段 → 供给核验与实例化
   └─ 可复用需求合同；地址、handle和运行上下文按实例管理
```

**流推导的复用单位是阶段合同，不只是统一读写函数。** 合同应说明输入/输出格式与版本、目标/模式/能力、位置与属性、资源依赖、状态生命周期及失败出口。接线检查格式和前置不变量相容；上游接受不保证下游成功，Tape验域接受后仍可由lower按目标能力拒绝。通道共用不合并语义所有者，非法输入、合法未覆盖、绑定失败、机制故障与预算耗尽应保留各自归属。流式接口也不要求立即外发字节；先沿用接受后提交输出的规则，避免末段失败时已承诺成功前缀。

**晚绑定是可固化的接线层。** 名字、kind、类型/签名、extent、权限、ABI和生命周期需求属于可复用合同；provider地址及计划handle属于绑定实例。argv、mapping等运行上下文另由运行适配器显式提供。宿主负责资源取得、内存与平台机制，不能替核心隐式选择语言或ABI语义；将地址藏进资源或清零后保留供给派生状态，都不构成解耦。

**判定不变量（接线检查依此判定，2026-10-15 cc 据 cc3 对照补）。**
1. 终态穷尽：每个阶段对所有合法输入都有已声明的终态——接受、合法未覆盖（not-covered）、语义拒绝三者之一；机制故障、预算耗尽、宿主超时另列，不算终态。
2. 未覆盖不是通过：not-covered 不得记为 PASS，不得当成非法输入，也不得静默回退到另一实现。
3. 链成功是合取：一条链成功 = 每个单元都接受 且 末端提交完成；中间单元的接受只是该单元的事实。
4. 见证不授资格：一次见证只说明一次观察；派生哈希、生成清单或单次资源观测不授予审阅、测量或全覆盖的资格。
5. 拒绝归属：语言级拒绝归声明该终态的阶段；运行层失败（provider 缺失、IO、预算、宿主超时）归绑定层或运行适配器，不得改记为阶段拒绝，反之亦然。
6. 有限表 ≠ 有限处理器：表的决策域有限可穷举，但栈、索引存储与输入帧不受界，表穷举不推出处理器对全部输入已覆盖。
7. 目标显式：target 是显式输入字段并由表声明能力；不得由宿主环境（uname、路径、环境变量）隐式选择。

**非目标。** 不做万能前端、统一错误码、自动回退到另一实现或并发调度；“流”不意味着立即逐字节外发；不以统一接口或新增总表本身计进展。

**刀口服务范式复用。** 选刀先问：是否消除一类隐式输入或重复接线，能否复用现有表/原语/协议，拒绝和所有权是否更清楚，有没有跨形状正例及边界反例。只有名字相似、却有不同语义或生命周期的路径不强行合并。优先小切面见证，再提公共合同，再逐路迁移；不以统一接口或新增总表本身计进展。

**现状与证据分账。** 本节提升设计方向，不宣称所有路径已纯化、可重入或具备同一完整合同。表、执行器和部分帧协议已有实现；晚绑定按类别迁移，剩余typed/variadic/callback与运行桥等以主线账本为准。相同完整输入的阶段重放、有序pass组合及合同分层仍须逐项取得证据；单次资源观测只证明该次执行，哈希/生成清单不替代审阅、ABI核验或产品测试。具体路由和实现边界见§3，条款以spec为准。

### 0.0.43 拒绝域裁定（机房主任代裁，2026-10-14）

选择方案 **A：前置 Tape 验域**。raw 字节先经显式验域，lower 只接与同一输入绑定的已验流；目标 syscall/ABI/外部能力拒绝仍属于 lower。raw `.tape` 是产品承诺；NEG-001 `?\n` 在 `-S`/`-c` 下 rc0 原样输出已裁为产品缺陷，须经前置验域返回 rc1 与具名诊断，不能以末端编码器拒绝或上游保证改记通过。

Windows `.hostcall`/`.hostaddr` 是有意的**库模式合同**，须显式输入模式/目标/调用能力，并使参考承认同一合同；该一致性证据未齐前，Windows 接受不得记产品绿。条款全文见 [spec.md 的 TP-7／TP-8／L-4](spec.md#c43-refusal-ruling)。归属与承诺已裁，实现与补验仍未完成；E3及lower接受样本不扩为完整拒绝域资格。历史待裁提案与观测保留在 [裁定摘要](research/c43-refusal-ruling-summary.md)。本裁定不授签名、Latest或公开，不改变7000历史结果或env-pin。

机房主任追加代裁：Tape 整数字面量为 64 位字，范围 `[-2^63, 2^64−1]`；超出由前置验域具名拒绝（integer 与行号），模型与 Python 参考同步，不复制 Python 的 4300 位宿主转换上限。负号先于进制前缀，适用于十进制/十六进制；保留既有 Python/模型二进制、八进制拼写同样的字范围。此前4301位分歧在实现与对拍后应消除，不能靠调宿主环境计绿。下划线整数当前仍为已登记未覆盖，固定动态 C 用例单列，不冒充产品接受。

实施接线采用独立 `exec/tapecheck` 有限转移模型：成功才透传整条原始输入，失败不输出字节、诊断标明原因与行号；包内普通/warning/multi/object 路由在 E3 后插入该阶段；forwarding 的 E3 side-car 先由现有适配器抽出/合并 Tape 再验域；公共 raw Tape/Tapebin 在任何输出之前单独调用同一模型。缺少包内 tapecheck 阶段时 driver 显式拒绝，不复用旧包或信任源后缀；构造与打包两实现须同拍；Windows 库模式参考暂列待证，不顺手扩为产品绿。

### 0.0.43 晚绑定首刀授权（机房主任代裁，2026-10-14）

已授开工：先将 lower 的外部合同符号流与宿主 provider 解析分离，并落绑定负例。首刀不以未声明签名冒充已验证 ABI；缺 provider 的具名拒绝与签名不匹配证据分账。设计入口见 [晚绑定首刀](research/c43-late-binding-first-cut.md)。驱动准备/执行与 Tapebin 超时结构债可并行拆提案，但内外步骤仍不超过 60 秒，缓存准备不算测试通过。开发树与封存候选闭包不同须按 CI 工作流解释，不当作产品回归；此授权不授重启 7000、签名、Latest 或公开。

### 0.0.43 晚绑定与前级冻结边界（现行摘要）

- 普通函数归属由 librarypartition / USLSEL1 决定；宿主层只解帧、按span绑定。provider只在绑定时查询，缺失不降级旧路；无地址片段不冒称整流无地址。
- 合同/归属/片段与编译态一起提交或回滚，禁止新合同配旧Tape。getter只观测已产出的代际收据，不惰性启动lower；缺provider不等于已有可调用镜像。
- 数据需求的稳定ID、类型/extent/权限显式入流；只读写拒绝归未绑定lower，供给资格和missing-provider归绑定层。typed/data/callback及运行桥迁移范围各自记账，完整条款见[spec.md](spec.md)的LB系列。
- 库实现与provider应独立于声明头；前级P/M不随库实现变更，禁止无pp表/模型变更实证而随库刀reseal，头内实现扩张暂缓。单标量非变参boundary-r1只获具名范围，不能扩成全libc迁移或发布绿。
- 字节流阶段各自保留语义与拒绝归属，资源和target显式；地址/provider/宿主路径不进入核心流。当前W/2c2/S的过程收据见归档，是否完成按具名实际证据核，不按历史窗口“已授”字样推定。

<a id="current-plans"></a>
## 2. 当前版本与有效计划

**2026-10-25 主人重新裁定：0.0.43 现在重构。** 旧1333项测试套件整体归档，退出开发和发布的主验收路径；新套件按产品合同重新设计，直接由 `unisacc.com + .cx` 驱动。此项属于本版，不等待0.0.44。保留现有产品、源码、夹具及原始结果作为历史和迁移依据，不把旧结果自动计入新验收。

**Python 归档（2026-10-30，cc，`a755ce969c`）：** 191 个已无活调用方的 `.py` 及会执行 `python3` 的旧 `.sh` 原路径挪入 `archive/reference-py/`（清单与依据见其 README）。活路径剩 94 个 `.py`：事实表生成器 `exec/facts/export.py` 与 `python3 -m unisa emit-kernel` 的实测闭包 42 个（原生替代前不能归档），`release/` 18 属其域；`apps/csih` 原有 34 个，2026-10-30 分两批归档 33 个到 `archive/reference-py/apps/csih/`；探针改用 `.cx` 桩和私有 tmux 见证，`check.cx` probes-net 6/0、loop 加 probes-e2e 8/0。只剩 `deepseek-proxy.py`，调用方是仓外的 rawcsih。另有 7 个缺失的系统接口和 `signal()`，在 0.0.43 上实测仍然缺，要走产品慢车道（`apps/csih/docs/python-debt.md`）。归档前后 `export.py --check` 行为不变、emit-kernel 产物逐字节相同、`tests/run.cx --list` 计数不变。第二批 `59948f29c8`、`af185e7069` 归档依赖已归档代码的旧 shell、`plans/pending` 与 `exec/pipeline/stages.tsv`。`42d03b6c0a` 让事实表生成器改读 `exec/facts/statics.tsv` 的 MAXTOK、不再读已归档的 `src/front_pp.c`，并重写 30 张表：29 张只变摘要头，`pp-autoinc-gen.tsv` 有 4 个 cx.h 函数依赖槽此前未随 cx.h 更新；现 `--check` 30/0。`kernel/` 里 40 个文件仍由 `python3 -m unisa emit-kernel` 生成，`unisa_headers.inc` 已改由原生 `tools/carried-header-export.cx` 生成。`.cx` 旧检查暂不归档：`tests/cxinclude|cxapis|cxtrunc` 三份棘轮在 HEAD 已红（51/71/167 行超基线），挪动会再改路径键。**更新（同日）：** 事实表生成器已全部移植为 `tools/facts-gen.cx`（30 张，加 bank 归属检查，`--check` 31/0），`export.py` 归档于 `07432e3ae3`，其导入闭包 7 个 `.py` 归档于 `e0bee5e42d`。仍在活路径执行的 Python 只剩 `python3 -m unisa emit-kernel` 的闭包，由 `tools/net-built-private.cx` 和 `tools/pp-built-private.cx` 调用；`4b9cc0ea95` 让它跳过生成物 `include/cx-flat.h`。

**当前入口（2026-10-30）：** `src/executor.cx` 改用 `core.kernel`，`core_run` 走包里的 UNIKERN1 汇编核心。`tools/entry-build.cx` 编 33 只 `-funit` 对象，unit tape 接成文本后链成 `entry.elf`，再接已装产品尾部的 P3。第一代 `out/grk-entry-build-12` 由已装产品经 `sh` 构造，候选 2631060 字节，sha256 `11b7a23ad760970590a28350da189f8391cde3649c8053f936b3756b7715747d`。第二代 `out/grk-entry-build-13` 直接执行这只候选，33 只对象、`entry.elf` 和候选与第一代逐字节相同。候选侧最慢对象是 `driveface.o`，ns=11794761865，最慢相位 e4 推理 ns=5386000902。上一只 CX 核心候选编同一对象是 ns=34415356768。`dup.c -run` 在这只候选和已装产品上都退出 3。`hello.c -run` 两边都退出 0，stdout 是 `from hello.c`。build-13 上 148 格是 143 SAME、5 DIFF、0 TIMEOUT，记在 [plans/cx-modules.md](plans/cx-modules.md) 第 160 节。转发那一刀的候选是 `out/grk-entry-build-14/candidate`（sha256 `0989b4fdda853103b1f2a45e991654ca1b17d6e9b424d5b53192d989f4679c13`）。它的 56 格 SAME 记在第 161 节，不能挪到现在这只上。上一只运行缓存候选是 build-15（sha256 `5a9cb1b4bbf188119a48b80b59910f40a36b7c4cced45a71f0576827254aa02a`），单源命中 506 ms，当时没有宿主调用的程序只跳过 e3。运行缓存那只是 `out/grk-entry-build-17/candidate`，2717972 字节，权限 0755，sha256 `a051997fdf52304bfffd285cca02474b1f4743012abaf26e1a7b1f2439f3f2b9`。单源且 side-car 没有宿主槽时也存转发 ELF，命中后直接执行。候选整文件（含 P3）里 64 KiB 及以上的哈希走内核 AF_ALG，失败再回退软件实现。`tests/runcache.cx` 对这只是 12/0，首次 546 ms，命中 40 ms。细节在第 163 节。这只混合候选是显式 `--o1-drivers` 编出来的。148 格全 SAME 之后，`tools/entry-build.cx` 不带旗标也把 drive、host、driveface、driveroute、cxmod、tapegate 编成 `-O1`，`--o1-drivers` 仍接受。无旗标只重编了 tapegate.o 和 path.o（`out/grk-opt2-default`）：tapegate.o 与这只候选逐字节相同，path.o 与全 `-O2` 候选逐字节相同。整只候选没有按新默认重编。两次构造是同一份 `tools/entry-build.cx`（sha256 `f7231608fb6d2f0dc5fef01e00dc9aa84ab246be915e5ad259fe56ecae0e98a3`），各拆成三批。对象加链接 61448 ms → 52768 ms。混合候选是 `out/grk-opt2-o1/candidate`，2959636 字节，sha256 `e477a4c70be28c11b86f9fce129fae2acf4e6c6813aa7d8436a8881d75160970`。`tests/runcache.cx` 12/0，首次 515 ms，命中 41 ms。`tools/dev-run.cx` 不带参数的冷 `-run` 编译加上一次用法拒绝，6112 ms，退出 2。这不是 dev-run 自己的运行时间。和全 `-O2` 候选对同一份小程序的 `-run`，stdout 和 stderr 逐字节相同。数字在 [plans/perf-bottlenecks-2026-10-30.md](plans/perf-bottlenecks-2026-10-30.md) 的 B5。已装产品仍是 3035300 字节，sha256 `79fb84e65a10b55ce40be5f68746748acbd38f441944046c0a5dfa5718bc0c32`。这不是产品安装。`76085cb99d` 给 entry-build 加了 `--target`，省略仍是 lnx/x86_64。不带该参数重编的 path.o、tapegate.o 与改前逐字节相同。四目标只编对象的结果在 [plans/entry-target-probe-2026-10-30.md](plans/entry-target-probe-2026-10-30.md)：lnx/arm64 的 33 个对象出了 ELF；osx 两个目标和 win/x86_64 各 33 个被已装产品拒绝。没有链接，没有安装。

<a id="work-tree"></a>
### 思维树

当前节点（2026-10-30）。逐刀记录在 [plans/cx-modules.md](plans/cx-modules.md)；10-30 之前的思维树与记忆宫殿原文在 [归档](archive/prd-c43-history-20261030.md)。

```text
0.0.43
├─ 测试体系重建（主人 10-25）：tests/run.cx --list（2026-10-30）289 批 / 270 已实现；覆盖 269 / 已实现 250；未准备 176；19 宽合同未解；--all 从未在一个候选上完整执行
├─ CX 结构重写 unisacc.com（主人 10-29）
│  ├─ 旧 C 已归档 archive/reference-c/（只供对照）；新入口 src/unisacc.cx 六层：host / packmodel / executor / stages / drive / bind
│  ├─ 候选：tools/entry-build.cx 33 只对象 → out/grk-entry-build-12/candidate（sha 11b7a23a…，自带已装 P3）；13 与 12 逐字节相同
│  ├─ 对拍：build-14 sha 0989b4fd 的 56 格 SAME 只属于那只。运行缓存候选 build-17 sha a051997f；tests/runcache.cx 12/0（首次 546 ms，命中 40 ms）。out/grk-opt2-o1 sha e477a4c7 上 148 格全 SAME，表在 out/grk-opt2-mx（不提交），见第 164 节。正式构造不带旗标也是这六只 -O1
│  ├─ 自举：第二代 13 已编完 33 只对象并链上，最慢 driveface.o 11.8s，低于 55s
│  ├─ 未切换：旧构造脚本 exec/c/buildcompiler.sh 等仍指向已归档 C；已装产品未换
│  ├─ 四目标对象探针（2026-10-30，已装产品，不算通过）：lnx/arm64 33 个对象都是 ELF；osx/x86_64、osx/arm64、win/x86_64 各 33 个编译失败，第一条诊断都是 model driver 还没把 -c -b 接到非 Linux。没有链接，没有 wrap。见 plans/entry-target-probe-2026-10-30.md
│  └─ 注册表里仍读旧 C 的批次见 plans/archived-c-batches-2026-10-30.md：比对类 6 个已改指 archive/reference-c，-I exec/c 加 source_frame/codecgate 对象，实跑全 PASS；p3-library-* 待 p3-pack-ctx 材料；旧构造路线（hostseed-*、ri-material-*、package-controls 等）归档还是标不可运行，待主人定
├─ 性能（plans/perf-bottlenecks-2026-10-30.md）
│  ├─ B1 E2 每个 #include 整缓冲重扫：前导部分已解（4f4a2ae2b3，include/cx.h → 生成的单文件 cx-flat.h，tools/cx-flatten.cx --check；空 main E2 1.74→0.09 s，候选冷编 facts-gen 40.5→16.8 s）；用户自己的 #include 仍待规则方案
│  ├─ B2 执行器自编 -O2 代码质量：主人定走汇编。cx/core/kernel.cx（f03c5a940d）接包内 UNIKERN1；入口已切 src/executor.cx use core.kernel。候选编 driveface.o 34.4s→11.8s，e4 推理 14.8s→5.39s
│  ├─ B3 运行缓存：单源也存转发 ELF，大文件哈希走 AF_ALG。tests/runcache.cx 对 build-17 是 12/0（首次 546 ms，命中 40 ms）。包解析和解压还在
│  └─ B5 六个驱动对象用 -O1：同一份 entry-build.cx，对象加链接 61.4 s → 52.8 s。混合候选 runcache 12/0（首次 515 ms，命中 41 ms），dev-run.cx 冷 -run 编译加用法拒绝 6.11 s。148 格全 SAME 后，不带旗标也是这六只 -O1
├─ 去 Python：已归档的在 archive/reference-py/；事实表 30 张由 tools/facts-gen.cx 生成并 --check，kernel/ 由 tools/kernel-emit.cx 生成，unisa 包已归档；活 .py 只剩 3 个：apps/csih/deepseek-proxy.py（仓外 rawcsih 拉起，待主人定改 rawcsih 直连）、release/apeinspect.py 与 release/tools/draftfetch.py（CI 用；release/apeinspect.cx 输出一致，工作流补丁在 out/ci-draft/，待授权）
├─ 分层：plans/cx-layers.md；L0 包 cx/text、cx/mem（arena）、cx/sys（env/fs/proc/time/net/sha，检查 tools/check-cx-sys.cx，仓库根这次 PASS ok=90 fail=0），棘轮 tests/cxraw.cx（上层直接 libc/POSIX 调用只降不升）；include/cx.h 与 cx_read/cx_write/cx_run* 收编归 cc
├─ 卫生：cx 四棘轮不计 archive/ 与已跟踪符号链接（f638c861e5，原把 31 个链接重复计数）；cxraw 这次是 628 个 .cx、19639 次调用，基线 19639，超过 0（src/drive.cx 79、src/driveroute.cx 22）。src/drive.cx、src/driveroute.cx、src/host.cx 里那 4 处 memcpy 已留下返回值。cxtrunc 整次仍失败，超基线的是 csih 新文件和过期行，不是这 4 处。cx/runtime、cx/compiler 的 5 处已标（71c45c265c）
├─ 新测试入口注册表 289 批次、270 已实现：本轮新登 24 个——21 个原本未登记的模块检查（其中 3 个解码检查原先就坏，缺 use，已修），加 tests/core-stack-empty.cx、tests/driver-environment.cx 两个包装和 tests/runcache.cx；还有 10 个 check 工具要材料或参数，未登记
└─ 分层棘轮：tests/cxraw.cx（plans/cx-layers.md）——上层 .cx 直接 libc/POSIX 只降不升、新文件须 0；csih 下层封装已入 tests/cxraw-layers.tsv（netwit/tmuxwit/retry_stub/tclock），apps/csih/tests/run.cx 已迁 tx_* 至 0
```

已装产品：`unisacc.com` 3,035,300 字节，sha `79fb84e65a10…`。旧候选资格、矩阵与安装收据不转授新入口。

### 本版重构合同

- **自身体系现在使用。** 日常编辑、构造、调度、校验和数据逻辑直接 `sh ./unisacc.com FILE.cx -- ARGS`；新代码不引入Python或新shell算法。旧独立裁判可作为归档参考；转调旧Python不算原生迁移。
- **按合同设计，不按旧脚本数量设计。** 用例明确输入、预期输出/拒绝、来源和覆盖范围；材料准备只记PREPARED。分层为网、流、流间接口、产品端到端、库/ABI和跨平台。完整覆盖清单独立于执行调度。
- **先复用已实现能力。** `.cx` 模块、内容摘要、通用执行器、库API及现成原生构造器先查再用。保留核心NN推理、明确拒绝域、完整RI和既有内存/ABI合同，不能为提速删断言或跳过推理。
- **先改循环成本。** 同一批用例尽量一次编译/启动；模型构造与用例执行分开，源码/表未变就不再转换权重。材料缓存命中仍核身份，测试结果不能冒充材料缓存。
- **按相计量。** 分开记录工具启动、构造、装载、推理、输出和校验；冷路径、热路径、准备成功与功能通过分别报告。只对已定位热点动刀，失败实验停止，不将研究数量算进度。
- **保留完整产品验收。** 新套件必须覆盖七流十八网及其组合、正负例、六目标、实际原生宿主和自举。归档不是通过；旧缺陷、未覆盖项、资源未知和未实跑平台必须迁入新合同缺口表。
- **归档不打断构建。** 测试树内被产品构建实际依赖的工具迁入独立tools目录；必要兼容入口仅转向原生工具并明确用途，不继续触发旧1333项套件。

### csr/grk 的经验在本版如何采用

| 已有证据 | 本版决定 |
|---|---|
| ujs反复构网和外部assembler启动曾占绝大多数时间 | 构造与执行解耦；批处理/常驻复用明确身份的材料和执行上下文 |
| 去死询问后可只构造实际使用的网 | 核对真实查询与合同再精简；不得裸砍必须存在的模型或跳完整性检查 |
| 并行与常驻优化有逐字节对照，部分memo无墙钟收益 | 新方案先测实际耗时和正确性；不为并行而并行，不保留无收益缓存 |
| unisacc无每文件fork外部assembler；本地大输入权重装载合计约0.01秒，E4执行约28秒 | 不移植wabt.js；按七流实测，优化实际重复工作和热点 |
| 旧C整包约40秒；未修改C执行器改用现成Clang热测38.617773秒且字节同；原生准备22.510912秒0 | 仅作旧链诊断和迁移参考，不把Clang变成日常迭代前提，也不把材料准备当功能通过 |

旧F4在window082停调度：327/1333结证，其中314rc0、6资源UNKNOWN、7原失败，1006待办。完整目录 `/tmp/unisacc-c43-final-e4-20261025` 原样保留；这些数字只描述封存状态，不再代表新0.0.43计划的完成率。现有P7f709产品仍可用作新工具的启动编译器；重构涉及产品输入时须重建并重新核验，不能借用旧候选资格。

<a id="memory-palace"></a>
### 记忆宫殿

| 房间 | 当前事实 | 入口 |
|---|---|---|
| 新入口 | `src/unisacc.cx` 六层；`src/core`、`src/runtime` 为指向 `cx/` 的符号链接 | plans/cx-modules.md 顶部产品面 |
| 候选构造 | 全只候选仍是显式 `--o1-drivers` 的 `out/grk-opt2-o1/candidate`，sha `e477a4c7…`，2959636 字节。entry-build 不带旗标也编那六只 `-O1`；整只还没按新默认重编。无旗标探针的 tapegate.o、path.o 与已知对象逐字节相同 | plans/perf-bottlenecks-2026-10-30.md 的 B5；plans/cx-modules.md 第 164 节 |
| 四目标探针 | 已装产品只编对象：lnx/arm64 33 只 ELF；osx/x86_64、osx/arm64、win/x86_64 各 33 只编译失败，诊断同一句。不记通过。`76085cb99d` 之后不带 `--target` 的 path.o、tapegate.o 与改前逐字节相同 | plans/entry-target-probe-2026-10-30.md |
| 对拍 | `e477a4c7` 上 148 格全 SAME（`out/grk-opt2-mx`）。build-14 的 56 格和 build-13 的 143 格不转到这只。build-17 的运行缓存合同是 `tests/runcache.cx` 12/0 | plans/cx-modules.md 第 164、161、163 节 |
| 性能 | 对象加链接 61.4 s → 52.8 s。混合候选 runcache 首次 515 ms、命中 41 ms。dev-run.cx 冷 -run 编译加用法拒绝 6.11 s。runcache 的运行时间没有变快 | plans/perf-bottlenecks-2026-10-30.md 的 B5 |
| 事实表 | 只经 `sh ./unisacc.com tools/facts-gen.cx -- --write\|--check .`（30 张 + bank 归属，`--check` 31/0） | exec/facts/ |
| 归档 | 参考 C、参考 Python、旧检查与旧 shell | archive/reference-c/、archive/reference-py/README.md |
| 已装产品 | 3,035,300 B，sha 79fb84e6…；新测试入口 `sh ./unisacc.com tests/run.cx -- --list` | tests/batches.tsv、tests/coverage.tsv |
| CX 分层 | L0=`cx/{sys,mem,text}`+layers 表白名单；棘轮 `tests/cxraw.cx`；示范 `tools/gold-table.cx` | plans/cx-layers.md、tests/cxraw-layers.tsv |
| 旧纪元证据 | 10-30 前的构造链、安装、矩阵收据 | archive/prd-c43-history-20261030.md |

**动态维护规则**：进度、身份、阻塞、决定或下一动作变化时，同步原位更新思维树节点及对应房间；只把当前事实留在这里。每次恢复先读两者，派发任务引用当前节点和确切缺口。原始过程与旧失败保留在证据或Git历史中，不冒充当前状态。

**产品命名**：宿主构建第一代为`unisacc-seed.com`，自举最终产品为`unisacc.com`；`unisacc-next.com`只是构建目录中间名。

<a id="pipeline-design"></a>
## 3. 当前架构：模型流水线（结构、功能与边界）

本节以 0.0.17 发布基线（`ae3f989`）说明构造器、路由与执行器，后续已验收的变化按相关小节补充；版本身份见 [历史记录](archive/prd-release-and-development-history-20261001.md#逐版发布身份)。条款全文在 [spec.md](spec.md)。 旧的“14 个决策点”不等于当前整条流水线的网络数，旧的“走查器不许模型化”不是当前迁移约束。

**阅读顺序**：共同模型结构 → 路由总表 → 各阶段 → 打包/装载 → 字节账 → 验证与现状。详细源码边界另见 [规则来源](exec/rules.md)、[执行机制](exec/c/CORE.md)、[包协议](exec/c/PACKAGE.md)。迁移过程及原始失败记录完整保留在 [S-17 迁移档案](archive/s17-migration-log-20260928.md)，不再混入当前规格。

### 流推导与模块复用口径（政委参考，2026-10-14）

函数调用表达为调模型做流式推导，IO 统一为显式字节流接口（通道形态可类比 iostream，不要求采用 C++ 或依赖其库）。可复用模块由**共用执行器 + 有限表 + 显式流接口**组成；通道共用不合并阶段语义，输入定义域、目标合同、具名拒绝及拒绝归属仍由各阶段保留。流格式、版本、资源依赖与失败出口须可核验；宿主解析/观测与核心流身份分账，不能靠不可复用特例或隐式宿主状态替代合同。本段固化设计方向，不宣称现有路径已全部迁移或获准复用旧测试结果。

<a id="flow-net-contracts"></a>
### 3.0 流/网合同三表与进度

七流真源：[exec/pipeline/image-stages.tsv](exec/pipeline/image-stages.tsv)；十八网真源：金表 `weights/gold/*.tsv` 本身（由 `tools/gold-table.cx --check` 核对，规则形式在 `seed/gold_<阶段>_label.c`）；原 `unisa/gold.py` 的 STAGES 已归档到 `archive/reference-py/unisa/gold.py`。逻辑完整输入与拆分方案保留在[归档身份义务](archive/testing-c43-legacy/tests/STAGE-IDENTITIES.md)，不作为当前执行入口，本节汇总现有物理管道及观测。

**逻辑边界不等于物理嵌套。** parse/type等逻辑阶段目前集成于e3，enc/image集成于elf，不声称已有独立产物。七流模型并非十八张ask事实网的简单一对一替换；下表消费关系含经典ask定位与产品归属，不能只因同名pp就把有限金表的叶证据当完整E2验证。流级封不推出内嵌每张网有独立封存。出货闸模块口径：**网格** `tests/netseal.cx`，**流格** `tests/streamseal.cx` + 叶内 `cell.tsv`（详见 `tests/SEAL-MODULES.md`）；e1/e4/prune/elf 已挂 streamseal，e2/e3/lower/pp 见该文「暂不合形状」。聚合测套 `./tests/seal-modules.sh`（闸 job `seal-modules`，contract 层，~25s）：manifest+x流×y网适用性检查 + 4 流封 + 17 网封回放；登记表 `tests/seal-modules.manifest.tsv`。

以下三表更新到 tip `25f720ab`（2026-10-21 17:33 AWST 观测）；近窗提交数量是当时记录，不是持续更新仪表。**有封≠已冻**：本次整理重验了被引封件闸（pp-leaf-seal(-negative)、exec-pp-literals、stage-contract-e1e3、lex-e1-seal、opt-e4-seal、elf-seal、prune-seal、middle-flow-e3/lower-accept、17×net-*-seal 各两遍，全部 rc 0），但过闸只证封件自身一致，不证封件范围覆盖了流的全部行为。

**归档边界：** 以下是2026-10-21的历史三表；其中的`tests/seals/`、旧seal模块及闸路径已迁入`archive/testing-c43-legacy/`，不在当前根路径，也不作为0.0.43活动闸。当前以§2、`tests/run.cx`和新清单为准。`exec-net-lex`的144键是独立网格内核×金表证据；不能用它或字节流中一次EOF观测冒充E1流内144个独立(c,peek)观测。当前E1矩阵保留此完整要求为MISSING/oracle缺口；历史“有封”不授本代资格。

<a id="seven-stream-contracts"></a>
### 3.0a 七流合同表

- tip `25f720ab` · 观测 2026-10-21 17:33 AWST（封件闸当日重验）
- 真源：`exec/pipeline/image-stages.tsv`
- **是否封存** = `tests/seals/` 有点名该流的冻结夹具/叶封原件；可标 STALE风险。**有封 ≠ 已冻**。

| 流名 | 输入→输出 | 产品网 | 功能 | 是否封存 | 备注 |
|------|-----------|--------|------|----------|------|
| **e2** | `src.c` → `pp.text` | `e2.net` | 预处理：续行/注释/指令/宏/条件/include | **有** · `tests/seals/exec-pp-literals/`（measured leaf；STALE风险：头/资源闭包持续进封件） | 近 tip 常被库头→autoinc→叶封拖动；纯转发原型刀可守冻哨 |
| **e1** | `pp.text` → `tokens.typed` | `e1.net` | 词法：关键字/标识/算符/字面量最长匹配 | **有** · `tests/seals/exec-lex-e1/` + `cell.tsv`；闸 `lex-e1-seal` 走 **`tests/streamseal.cx`**（gate + reseal 均 `streamseal.cx`） | 流封只证录制样本；另有网级叶 `exec-net-lex/`（内核×金表，非流内 ask） |
| **e3** | `tokens.typed` → `tape.text` | `e3.net` | 语法+类型等合成段 → tape（parse/type 等物理嵌此） | **有** · `stagecontract-e1-e3/` + `middleflow-step0/e3/` + `librarycallbacke3/`（librarytyped* 历史≠PASS→STALE风险） | STAGE-IDENTITIES：parse/type **未拆**仍嵌 e3 |
| **e4** | `tape.text` → `tape.text` | `e4.net` | opt：tape 上 peephole / 栈→寄存器类改写 | **有** · `tests/seals/exec-opt-e4/` + `cell.tsv`；闸 `opt-e4-seal` 走 **`tests/streamseal.cx`**（gate + reseal 均 `streamseal.cx`） | 前端是输入；另有网级叶 `exec-net-opinfo/`、`exec-net-peep/` |
| **prune** | `tape.text` → `tape.text` | `prune.net` | 按根/导出剪不可达整函数（公开 tape 路径可跳） | **有** · `tests/seals/exec-prune/` + `cell.tsv`（referee 冻字节；闸 `prune-seal` 走 **`streamseal.cx`**） | 仍**不消费** 18 金网 |
| **lower** | `tape.text` → `target.text` | `lower.net` | ABI/regmap/syscall/布局/reloc 降到目标 ops | **有** · `stagecontract-tape-lower/` + `middleflow-step0/lower/` + `librarytypedlower/` + `librarycallbacklower/`（历史≠PASS→STALE风险） | tip≈99-commit 窗 **0** 笔改 lower 封/构造；有封且近窗不拖 |
| **elf** | `target.text` → `image.bytes` | `elf.net` | 编码 + ELF/Mach-O/PE（或内存镜像）写出 | **有** · `tests/seals/exec-elf/` + `cell.tsv`（**仅 lnx/x86_64**；闸 `elf-seal` 走 **`streamseal.cx`**） | Mach-O/PE/内存镜像未封；STAGE-IDENTITIES：enc/image **集成于 elf**；另有网级叶 `exec-net-enc/` |

**汇总**：有封存原件 **7/7**，但范围各异：e1/e4/prune/elf 是对录制输入的点名流封（elf 仅一目标），e2/e3/lower 为此前的叶封/阶段合同封。**有封≠已冻**，更不等于该流全行为已覆盖。勿与「在调 6/7」混读——在调口径含未冻/耦合/未拆，见观测 `seal-count-7x18-1049awst.md`。

<a id="eighteen-net-contracts"></a>
### 3.0b 十八网合同表

- tip `25f720ab` · 观测 2026-10-21 17:33 AWST
- 真源：金表 `weights/gold/*.tsv`（`tools/gold-table.cx --check`）；README 的 stages 表是冻结文本；原 `unisa/gold.py` → `STAGES` 已归档
- **是否封存** = `tests/seals/` 有点名该网的叶封/夹具原件；可标 STALE风险。**有封 ≠ 已冻**。金表 tsv ≠ seal 夹具。

| 网名 | 功能 | 主要落流 | 是否封存 | 备注（出货/对照） |
|------|------|----------|----------|-------------------|
| **pp** | dir×defined → take/skip/pop/macro（指令条件） | **e2** | **有** · `tests/seals/exec-pp-literals/`（STALE风险：autoinc/头资源耦合） | 出货主脊；唯一有点名叶封的金网；tip 窗重封多为库存货身份，非 `pp.tsv` 语义行 |
| **lex** | c×peek → token 类 | **e1** | **有** · `tests/seals/exec-net-lex/`（per-net cell，144 键 144 判定，0 wrong） | 出货主脊；earlystageseal 不验证 E1/lex |
| **parse** | nt×tok → 归约/动作类 | **e3**（嵌） | **有** · `tests/seals/exec-net-parse/`（per-net cell，340 键 340 判定，0 wrong） | 出货；STAGE-IDENTITIES：无单独产物，集成 e3 |
| **type** | t1×op×t2 → 结果类型 | **e3**（嵌） | **有** · `tests/seals/exec-net-type/`（per-net cell，4275 键 4275 判定，0 wrong） | 出货；当前集成 e3 |
| **scope** | ctx×kind → 作用域动作 | **e3**（嵌） | **有** · `tests/seals/exec-net-scope/`（per-net cell，30 键 30 判定，0 wrong） | 出货 |
| **irsel** | family×flavor → IR 形 | **e3** | **有** · `tests/seals/exec-net-irsel/`（per-net cell，420 键 420 判定，0 wrong） | 出货（ask 在 e3） |
| **enc** | op×os×arch → 编码类 | **elf**（主嵌）；lower 次要 ask | **有** · `tests/seals/exec-net-enc/`（per-net cell，522 键 522 判定，0 wrong） | 出货；身份合同钉 elf；经典 lowering 仍问 form |
| **reloc** | kind×arch → 重定位类 | **lower** | **有** · `tests/seals/exec-net-reloc/`（per-net cell，6 键 6 判定，0 wrong） | 出货；决策在 lower，image 只落记录 |
| **tyinfo** | t → size/uns/narrow | **e3** | **有** · `tests/seals/exec-net-tyinfo/`（per-net cell，48 判定，0 wrong） | 出货（类型旁路） |
| **pfconv** | conv → 指针/函数转换类 | **e3** | **有** · `tests/seals/exec-net-pfconv/`（per-net cell，9 判定，0 wrong） | 出货 |
| **regmap** | treg×arch → 机器寄存器 | **lower** | **有** · `tests/seals/exec-net-regmap/`（per-net cell，16 判定，0 wrong） | 出货 |
| **isel** | op×arch → form/symbol | （构造有；主脊不问） | **有** · `tests/seals/exec-net-isel/`（per-net cell，174 键 348 判定，0 wrong） | **对照**：金表仍构造；lowering **不 ask**（form 由 enc 覆盖） |
| **abi** | op×os×arch → syscall/形/门… | **lower** | **有** · `tests/seals/exec-net-abi/`（per-net cell，522 键 6786 判定，0 wrong） | 出货；`abi.tsv` 是金表非封存夹具 |
| **combo** | isel∪abi 对照臂 | 对照 / 非出货主脊 | **有** · `tests/seals/exec-net-combo/`（per-net cell，522 键 7830 判定，0 wrong） | **不出货主脊**；`drive=="combo"` 实验路径；research `seal-combo-*` ≠ 金网夹具 |
| **binsel** | op×sign → ALU 味 | **e3** | **有** · `tests/seals/exec-net-binsel/`（per-net cell，32 判定，0 wrong） | 出货 |
| **prec** | op → 优先级 | **e3** | **有** · `tests/seals/exec-net-prec/`（per-net cell，19 判定，0 wrong） | 出货（表达式） |
| **opinfo** | op → 优化用属性 | **e4** | **有** · `tests/seals/exec-net-opinfo/`（per-net cell，73 键 219 判定，0 wrong） | 出货 |
| **peep** | a×b×rel → peephole 动作 | **e4** | **有** · `tests/seals/exec-net-peep/`（per-net cell，1632 键 1632 判定，0 wrong） | 出货 |

**汇总**：有点名封存原件 **18/18**（pp 叶封 + 17 个 per-net kernel cell，80eca415/2ebf3f7c/25f720ab，闸 net-<网>-seal）。**范围要读清**：per-net cell 封的是整数内核对该网金表**全域判定**的冻结回放（闭包只有 unisa_core.c + 该网权重字面量 + 该网金表，零 pp 路径），**不**覆盖该网在产品流内的运行时 ask、不覆盖 dense.*.inc、不覆盖权重构造。`weights/gold/*.tsv` / `unisa/gold.py` 本窗仍**无语义行提交**。

<a id="flow-net-relation"></a>
### 3.0c 流×网关系与边级夹具

- tip `25f720ab` · 观测 2026-10-21 17:33 AWST
- 真源：`image-stages.tsv`、`gold.STAGES`、`tests/STAGE-IDENTITIES.md`、经典 ask/`inf` 位点、`compilerpack` 路由
- **勿混**：7 流 = 字节管道（**stream**）；18 网 = ask 事实表（**net**）；名词定义以 §0 为准，同名不可互换。

#### 图例

| 符号 | 含义 |
|------|------|
| **✓** | 主消费（出货主脊该段主要承载/查询） |
| **◐** | 次要/旁路（跨段 ask、构造旁路、或物理未拆的次归属） |
| **·** | 不相关 |
| **⊘** | 有金表但不在出货主脊 ask（对照臂） |

#### 1) 关系总表（行=7流，列=18网）

| 流＼网 | pp | lex | parse | type | scope | irsel | enc | reloc | tyinfo | pfconv | regmap | isel | abi | combo | binsel | prec | opinfo | peep |
|--------|:--:|:---:|:-----:|:----:|:-----:|:-----:|:---:|:-----:|:------:|:------:|:------:|:----:|:---:|:-----:|:------:|:----:|:------:|:----:|
| **e2** | ✓ | · | · | · | · | · | · | · | · | · | · | ⊘ | · | ⊘ | · | · | · | · |
| **e1** | · | ✓ | · | · | · | · | · | · | · | · | · | ⊘ | · | ⊘ | · | · | · | · |
| **e3** | · | · | ✓ | ✓ | ✓ | ✓ | · | · | ✓ | ✓ | · | ⊘ | · | ⊘ | ✓ | ✓ | · | · |
| **e4** | · | · | · | · | · | · | · | · | · | · | · | ⊘ | · | ⊘ | · | · | ✓ | ✓ |
| **prune** | · | · | · | · | · | · | · | · | · | · | · | ⊘ | · | ⊘ | · | · | · | · |
| **lower** | · | · | · | · | · | · | ◐ | ✓ | · | · | ✓ | ⊘ | ✓ | ⊘ | · | · | · | · |
| **elf** | · | · | · | · | · | · | ✓ | · | · | · | · | ⊘ | · | ⊘ | · | · | · | · |

#### 2) 白话：每流主要网

| 流 | 主要网 | 可能旁路 | 白话 |
|----|--------|----------|------|
| **e2** | pp | — | 预处理管道；指令/条件决策查 pp 表。 |
| **e1** | lex | 构造期旁看 parse.tsv 的 tok 域（非运行时消费 parse 网） | 词法管道；最长匹配/类别查 lex。 |
| **e3** | parse, type, scope, irsel, tyinfo, pfconv, binsel, prec | — | **一大段未拆**：tokens→tape；语法/类型/作用域/IR/优先级等 ask 全嵌此物理网。 |
| **e4** | opinfo, peep | — | tape 上 peephole / 属性驱动改写。 |
| **prune** | （无 18 网） | — | 按导出/根剪函数；**不查金表**，政策在代码。 |
| **lower** | abi, regmap, reloc | **enc**（经典 lowering 问 form；产品编码段在 elf） | ABI/寄存器/重定位；enc form 经典侧顺带问。 |
| **elf** | enc（+ image 写出，无独立金网名） | — | 编码+容器；STAGE-IDENTITIES 把 enc/image 钉在 elf。 |

#### 3) 边级「有无夹具」（流×网 主边）

口径：夹具 = `tests/seals/` 点名覆盖该**流段**或该**网**的原件；流级封≠网级叶封。有封 ≠ 已冻；可标 STALE。机器登记表 `tests/seal-edges.tsv`（`sealmodulescheck --edges-only` 校验主边全集/夹具/点名闸，随 `seal-modules` 闸跑）。

| 边（流→网） | 关系 | 有无夹具 | 路径简述 |
|-------------|------|----------|----------|
| e2 → pp | ✓ | **有（网级叶 + 流侧）** | `tests/seals/exec-pp-literals/`；STALE风险（头/autoinc） |
| e1 → lex | ✓ | **有（流侧 + 网级叶）** | `exec-lex-e1/`（录制输入流封）+ `exec-net-lex/`（内核×金表）；边上运行时 ask 仍无点名夹具 |
| e3 → parse/type/scope/irsel/tyinfo/pfconv/binsel/prec | ✓（嵌） | **有（流级 + 八网网级叶）** | `stagecontract-e1-e3/`、`middleflow-step0/e3/`、`librarycallbacke3/`；八网各有 `exec-net-*`；嵌在 e3 内的运行时 ask 仍无边级点名 |
| e4 → opinfo, peep | ✓ | **有（流侧 + 网级叶）** | `exec-opt-e4/` + `exec-net-opinfo/`、`exec-net-peep/`；边级 ask 无点名 |
| prune → ∅ | · | **有（流侧）** | `exec-prune/`（录制 -O2 tape，referee 冻字节）；无金网可封 |
| lower → abi, regmap, reloc | ✓ | **有（流级 + 网级叶）** | `stagecontract-tape-lower/`、`middleflow-step0/lower/`、`library*lower/` + `exec-net-abi/regmap/reloc`；边级 ask 无点名 |
| lower → enc | ◐ | **有（网级叶，非边级）** | `exec-net-enc/` 点名 enc 网本身；经典 lowering ask 这条边仍无夹具 |
| elf → enc | ✓ | **有（流侧 + 网级叶）** | `exec-elf/`（仅 lnx/x86_64）+ `exec-net-enc/`；其余容器目标未封 |
| * → isel / combo | ⊘ | **有（网级叶）** | `exec-net-isel/`、`exec-net-combo/`（内核×金表）；仍是对照臂，不在出货主脊 ask |

##### 脚注

1. **流级封 ≠ 网级封**：e3/lower 有阶段合同封，只证明该段管道原件存在，**不**等于嵌在内的每张金网已点名冻结。
2. **网级点名原件 18/18**（25f720ab）：pp 的 `exec-pp-literals` 叶封 + 17 个 `exec-net-*` per-net kernel cell。cell 只冻内核×金表全域判定；**不**替代边级（流内运行时 ask）夹具，该层仍空。
3. **isel/combo**：矩阵列上对所有流出货主脊标 ⊘（对照/不问或 off ship）；不单独为每行再写夹具行。

#### 4) 物理嵌套（勿读成路由扇出）

- **e3 ⊃** {parse, type, scope, irsel, tyinfo, pfconv, binsel, prec} — 一段流嵌多张网。
- **elf ⊃** {enc, image} — image 不是 18 金网之一。
- **e4 ⊃** {opinfo, peep} — 一段两网，仍未拆独立 image-stage。
- **近 1:1**：e2↔pp，e1↔lex；**prune → ∅**；**lower** 对 abi/regmap/reloc 为一对多路由式。

<a id="flow-net-progress"></a>
### 3.0d 可核验进度


封存清点（tip `9722183c`，2026-10-21 18:30 AWST）：**流 7/7、网 18/18 有点名原件**——这是「有原件」计数，不是处理器完成比例，更不是覆盖率。**有封≠已冻**；余缺照列：① per-net cell 不覆盖该网在流内的运行时 ask（边级原件 **17/17 = 15 键答映射 + 2 产品侧见证**（`tests/edgeseal.py` 参考端规范化键答映射叶 + `tests/edgewitness.sh` 产品侧见证件——后者为 e3→pfconv（唯 `-nostdinc` 可达，参考侧永不问、stages 对它空真）与 elf→enc（elf 流 enc ask 产品侧独有）两条，强度弱于键答映射件、分类计数不混同；各叶双录/双观测判等、三重 sha 钉扎、负例必红；设计/考古/实测收据 research/c43-edge-freeze-design-r1.md），计数入闸 `tests/seal-edges.tsv`；但链条 gold≡weights（net-*-seal）≡DENSE（oracle-cache 闸，22694 问双向，负例实测一字节翻转即红）已闭，且 stages-1*–3* 闸比对 10 网「Python 问过则 C 必问过」（前端 8 网 + prec/binsel，-c -v 打印已扩），后端六边（opinfo/peep/abi/regmap/reloc/enc）由 stages-back 闸以全程路线比对（Python compile_file+optimise -O2+lower vs C 全构建 -v 的 askedline，两探针，负例实测旧源二进制即红）——16 张出货网的「问过」证据至此全部入闸；仍缺的是按探针冻结的边级封存原件（比对式≠冻结回放））；② e1/e4/prune/elf 流封只回放录制输入，elf 仅 lnx/x86_64 一个目标；③ 权重/DENSE 的语义链已闭（net-*-seal：weights≡gold；oracle-cache：DENSE≡weights；kernel 闸：kernel/* 全部 59 件对 emit-kernel 再生逐字节比对），构造工具 build-weights 在 CI 两处跑（ci.yml）；余下的只是 gold→built.json 构造没有本机闸、built.json 无点名封存原件——语义错误会被上述三闸抓住，此条降级为低风险；④ parse/type 等物理仍嵌 e3 未拆；⑤ pp 叶封的 STALE 风险（头/autoinc 耦合）未消。本次封件闸全量重验见 §3.0 开头。

| 发布链三格（2026-10-25动态状态） | 状态 | 证据边界 |
|---|---|---|
| ① 源码与发布身份 | 候选SHA／S及不可变封存不变；RC精确绑定cd30109850 | 后续测试／发布修复提交另记新HEAD，不冒充产品来源或旧RC |
| ② 本轮候选有效闸与收口收据 | **a877旧089停窗；k2b修复后新图1317／954已核，待新正式结证** | 两叶原40秒冷回归、四元数据闸通过；旧源部分结果及失败保留，缺测仍OPEN／非PASS，不迁未知闭包绿 |
| ③ 发行资产及正式发布 | 本轮未完成；签名／最终字节／公开仍待办 | Latest最后核对为v0.0.42；源码push与签前CI成功不等于0.0.43已发布 |

**根对现状（2026-10-22）**：开发已迁到新 Linux x86_64 宿主（16 核，内存充足，旧机 2.3G 内存压力导致的 seed-gen 卡死不再出现）。根部 `unisacc.com` 为工作树候选（本机由 `make model-com` 各步构建；宿主须有 clang/ld64.lld/llvm-nm 与 zlib 头，exec/c/BUILDING.md 未列，待补），每次表刀后按 `release/derived-map.tsv` + `release/tools/derivedcheck.py` 刷派生物、经 `release/tools/commitgate.sh` 提交。旧机第一至第七次根对重建/归位收据与宿主坑位已归档：[2026-10-22 过程归档](archive/prd-process-notes-20261022.md)。

三格分别记录源码身份、验收及公开资产，不折算为完成率或剩余时间。当前版本见[src/version.h](src/version.h)；Latest于2026-10-24实际只读核对仍为v0.0.42。

查看：[七流表](#seven-stream-contracts) · [十八网表](#eighteen-net-contracts) · [流×网关系及边级夹具](#flow-net-relation)。本节更新状态口径，不改三表原观测数据。

### 3.1 从声明到网络：到底替换了什么

```text
事实表 weights/gold/*.tsv + 各阶段控制/动作 TSV + 模板/资源
    → 离线构造器装配有限转移 δ（JSON）
    → tbl.py 编号化（.tbl，参考与检查格式）
    → net.py 构造整数阈值网络（.net）
    → compilerpack/pack 压缩、去重、声明路线（P3）
    → 通用执行器：字节输入 → 网络选转移 → 执行动作 → 字节输出
```

**事实表与控制规则必须同时记账。** `weights/gold` 提供词类、类型、优化、ABI、编码等有限决策事实；`exec/*` 邻接 TSV 描述扫描、归约、调用、布局和输出等控制过程。Python 构造器仍负责 trie、名字/状态分配、目标绑定和规则装配，部分编排仍是手写代码。不能说仅靠原有 18 张 gold 表已经自动推导完整 C99 编译器，也不能把构造器的代码量隐藏在“权重”一词里。它们是离线规则来源，不在用户编译路径中执行。

每个控制状态选择一个观察银行，观察值是**输入字节/EOF、续接栈顶或比较结果**。网络输出两个整数：下一控制状态与动作序列编号；并非对任意源码直接作一次分类。数据寄存器、索引内存、intern 字符串、输入帧、续接栈与输出缓冲承载跨步状态；它们不作为一个无限宽向量输入网络。

对一个有限银行的两个输出分别构造：`f(x) = f(lo) + Σ[t≤x](f(t)−f(t−1))`。隐单元是整数阈值 `[x≥t]`，权重是相邻答案差；构造精确，不训练。缺转移也编码为 `(-1,0)`。升序阈值允许只累加激活前缀，仍求同一个网络函数，不转换成答案查表。

**续接返回是声明的通用控制原语。** 栈键银行中共享同一动作序列的 `k→k` 返回由声明返回集合表示，执行核验证成员后恢复续点；其余转移仍由阈值网络回答。字节账包含这些声明，不冒称它们全是数值神经权重。

**执行器边界**：执行核只有定宽整数算术/比较、寄存器与索引内存读写、字节跨度/缓冲、栈和输入帧、intern/blob、资源获取、接受/拒绝等原语。宿主负责文件、OS 分配、平台 API 绑定和进入已生成代码；不调用宿主 C 解析器、优化器或参考镜像 writer。语言相关判定、扫描顺序、ABI 选择和机器字节布局应在阶段规则/动作中。资源模板、Windows argv 机器代码模板及离线手写装配仍明确保留，不能宣称“所有语义均已模型化”。

### 3.2 路由、数据协议与分支

| 逻辑步骤 / 实际阶段名 | 输入 → 输出 | 模型组织 | 路由条件 |
|---|---|---|---|
| E2 / `e2` | 源字节 → 预处理文本及位置封装 | 六目标预定义；普通/位置变体 | `-E` 在此结束；宏与头资源来自声明 |
| E1 / `e1` | 预处理封装 → 带拼写/位置的 token 流 | 共享词法控制网络 | 普通编译保留 E2 映射 |
| 多单元 / `units` | 独立单元的 token 帧 → 合并 token 与单位映射 | 一个共享隔离/合并网络 | 多文件在 E3 前插入；宏状态不跨单位 |
| E3 / `e3` | token 流 → tape；另输出诊断 | 普通错误版与错误+告警版 | 正常产品路线也是 located/errors，不只告警才有位置 |
| E4 / `e4` | tape → tape | O1、O2 两个网络 | O0 省略；优化级别不等于另一种前端 |
| `prune` | tape → 保守剪枝tape | 共享闭包扫描网络；数据/代码地址根保留 | 镜像/-run在lower前调用，公开-S/-c不调用；超域原文保持 |
| E5 前段 / `lower` | tape → 目标指令/符号/稀疏数据文本 | 六目标网络，ABI 事实与布局动作 | 所有目标共用 tape 协议 |
| E5 编码 + E6 / `elf` | 目标文本 → ELF/Mach-O/PE 字节 | 六目标编码及镜像网络 | `elf` 是历史路由名，包含三种镜像格式 |
| 内存路线 / `target/memory` | 目标文本 + 实际基址资源 → UNIMEM1 | 重用末端网络的内存入口 | `-run` 不落盘执行镜像；一次绑定 |

实际路线来自 [image-stages.tsv](exec/pipeline/image-stages.tsv) 与 [compilerpack.py](exec/c/compilerpack.py)，驱动执行见 [compiler.c](exec/c/compiler.c)。公开 token dump 另有 `tokenpp/tokenlex`，不等于普通编译的 located E2/E1。`-c` 当前输出 tape，不是原生 `.o`；多个 C 文件一次编译成一个程序，没有普通系统链接器的 `-l/-L` 契约。模型拒绝不能悄悄改走经典编译；经典路线是显式参考/回退构建。

### 3.3 E2：预处理模型

- **输入/输出**：源文件字节；普通输出 `pp.text`，产品输出带文本长度、强制/自动包含信息、续行与包含位置的 `UNIPP1` 封装。`-D/-U/-include/-nostdinc` 由 CLI 资源传入，不由 driver 改写源码。
- **结构**：先处理 shebang、续行、注释与字面量保护，再进行自动头选择、指令处理和宏重扫。宏记录存名字、正文、参数、可见区间、历史链和隐藏标记；输入帧承载参数/替换正文，显式栈承载条件表达式归约。包含文件拼入后重新规范化。
- **规则来源**：gold `pp.tsv` 的指令与 defined 判定；[exec/pp](exec/pp/rules.md) 的 text/autoinc/macro/directive/expression/reduce/hash/rescan/pragma/location 声明；目标 `predefines.tsv`；头文本为命名资源。gold 的小表只是指令判定的一部分，不等于整个预处理器。
- **功能/限制**：对象/函数/变参宏、参数展开、字符串化、覆盖的拼接、条件表达式、包含搜索、macro-stack pragma、位置跟踪。参数上限 MAXP=12（exec/facts/pp-layout.tsv；2026-10-27 cc 裁定，旧值 8 已过时）；一般标点/字面量拼接等尚有限制，E2 自身不生成完整 file:line:col 诊断。
- **判据**：macro/literal/pragma/location 专项、分片参考字节对照、坏封装与失败路径；位置传输还须经 E1/E3 核对。构造器读取自动头导出名单并装配控制，不能写成这些编排已消失。

### 3.4 E1：词法模型

- **输入/输出**：预处理字节及映射 → typed token 行流，保留标识符/数字/字符串原始拼写。located 输出 `UNITOK1`，携带原始预处理封装与每 token 的源偏移；EOF 是显式观察。
- **结构**：字节类/前瞻类分派，标识符与运算符 trie、数字/字面量控制状态；MARK/JUMP/跨度动作实现最长前缀，栈处理声明中的平衡跳过。数值求值不是词法阶段的职责。
- **来源**：gold lex、parse token 词汇，lexcls/lexword/typekw，以及 [词法规则](exec/lex/rules.md) 的 number/literal/ident/entry/output/spelling/count/location。trie 和连接由构造器生成，不运行经典 lexer 补答案。
- **功能/限制**：关键字、类型词、普通标识符、数字拼写、字符/字符串前缀、注释、最长匹配标点及声明中的 UCN 路径；规则存在不等于完整 Unicode/C99 保证。非法字节和坏封装明确拒绝。
- **判据**：compare、positioncheck、locationcheck；同时比实际网络与通用动作参考，不把旧独立 lex 实验误认为生产构造路线。

### 3.5 多单元：隔离与合并模型

每个源文件用全新的 E2/E1 机器状态运行，宏与 include guard 不跨单元。driver 只读文件、封装长度/文件名；模型验证帧、扫描并隔离文件级 static 与标签，合并 token，输出 `UNITOK2` 的单位/位置目录。最多 64 帧。E3 在程序范围共享符号、签名、字符串池，并保留声明的单位归属。不是把源文件简单拼接，也不是系统 `.o` 链接。多源 `-E`/token CLI 当前拒绝；部分 aggregate static 声明形式有命名拒绝。判据见 multicheck、multiwarningcheck 与 unitlocationcheck，检查隔离、输入顺序及错误归属。

### 3.6 E3：解析、类型、存储与 tape 模型

- **输入/输出**：UNITOK1/UNITOK2 → `tape.text` 与独立诊断缓冲。tape 包含标签、指令、字符串/BSS、初始化与入口段；是中间协议，不是宿主机器代码。
- **控制结构**：构造器用状态 label、边上动作、压续点的 call 和栈顶 RET 组织子过程。token-reader 把行流解码成 token 类、intern 名字、跨度/数值；索引内存存符号、作用域撤销、维度、成员、函数签名、标签和字符串池。共享优先级梯子进行表达式调度，不是一次匹配有限个完整程序。
- **事实与控制来源**：prec/binsel/irsel/type/tyinfo/pfconv 提供优先级、ALU 拼写、共同/结果类型、宽度及转换。`exec/parse2` 的 declaration/scope/operator/ladder/control/member/call/initializer/statics/vla/constexpr 等 TSV 提供控制；tape/text 模板负责输出。`exec/parse/gen.py` 仍被导入作共享组装器；intrinsic 名单来自种子声明。手写构造编排仍在，不称已有通用文法编译器。
- **功能**：声明与作用域、typedef、整数/浮点/指针、数组、函数指针/签名、struct/union/enum、成员/位域/柔性数组、常量与 VLA 维度、初始化、表达式/转换/更新、sizeof、语句控制/标签/调用及 intrinsic。每项以保留探针中的具体形状为支持范围，不能从名字存在推断任意组合都支持。
- **浮点字面量**：数字解码通过 32 位 limb 动作形成精确比例并最近偶数舍入，包含次正规数；不是宿主浮点解析回调。160-limb 上限等构造容量单列。
- **位置、错误、告警**：模型将 token 偏移关联到原文件/行列/上下文；普通版启用 errors，告警版再启用 return/int-conversion/unused/format 规则。部分已映射语法错误允许顶层平衡恢复；未覆盖形式定位后停止。错误非零不得发布 tape。告警规则不称完整控制流/printf 规范分析。
- **限制**：token 字节跨度 <2^26，数组 rank≤8、struct 池≤128，签名/shape/enum 池有限；部分声明符、类型形状、初始化、struct 返回和异型条件分支仍有拒绝。静态容量与理论无界存储分开。
- **判据**：固定 probe/keep、floatconst 的系统 cc/表/网络核对、scope/enum/location/diagnostic/error/warning 专项和完整 self-source tape。`parse2/selfcheck.sh` 使用 `.tbl`，其成功不能单独冒称网络自源证明；产品网络与完整路线另有门禁。

### 3.7 E4：优化模型

输入/输出均为 tape，O0 不调用；O1/O2 使用独立控制网络。START 加载 opinfo/peep 事实；扫描、解析、基本块、读写、活跃性、融合和轮次来自 `exec/opt` 的 TSV。O1 在最多四轮内检查有限直线范围，把软件栈保存/恢复替换成 mov。O2 加入基本块/标签、活跃性重扫、死寄存器携值、局部读融合、相邻指令 peep 和复制/目的重定向。它是有界保守改写，不是全程序优化或完整寄存器分配；算法被编码进规则与动作，并未凭网络构造消除算法信息。判据是同一 O0 tape 经模型后与参考 O1/O2 的 tape 字节相等，另有独立执行差分。来源/门禁见 [opt规则清单](exec/opt/gen-manifest.tsv)、exec-e4/exec-e4self。

### 3.7a prune：函数可达闭包模型

输入/输出为tape原文。模型识别受支持的函数prologue，建立调用、跳转及可能贯穿边，从入口、main/__init和代码取址根求闭包；保留所有数据语句和可达原始字节跨度。一个共享阈值网络由 `exec/prune/gen.py` 构造，执行核没有剪枝原语。host ABI、可达间接调用、超2MiB或目录容量、未知/歧义与非LF等输入完整保留。参考分别为 `src/tapeprune.c` 与 `unisa/prune.py`；四个PRUNE_PART门禁对拍，不把保守fallback说成完成全程序分析。

### 3.8 lower：ABI、目标指令与稀疏数据模型

- **输入/输出**：tape → `target.text`，含目标、实际保存的数据、符号、逻辑数据长/BSS/重定位，再接目标寄存器指令、标签、元数据。
- **结构**：数据扫描与 code 扫描分开。zero-last 排布和 mod-8 对齐由模型动作执行；虚拟长度与保存前缀分离，BSS/对齐推进逻辑长度，不逐字节存零。宽索引命名空间避免大数据偏移撞上其他表。
- **来源/功能**：regmap/enc/abi/reloc 事实、tape SHAPE、scratch/WinAPI 常量与 code-abi-sources/code-syscall 声明。模型完成入口、参数、软件栈、寄存器映射、POSIX syscall、Darwin carry 与 hostcall/hostaddr、Windows 保存/恢复与返回转换。ARM 窄化/立即数融合及有界 dead-after 扫描也在声明动作中。
- **限制/判据**：约 2GB 数据/相对寻址边界，不支持的参数形状拒绝，none WinAPI 导入不能当正常导入。普通/ARM/Windows lowering 与 sparsecheck 分开验，稀疏 610MB 布局成功不等于编译 610MB 数组源码已经成功。

### 3.9 E5：指令编码模型

编码与镜像写出在实际 `elf` 阶段同一网络中连接；逻辑区分不代表磁盘有额外一份模型。x86 解析寄存器/操作数/元数据，执行 REX/ModRM/SIB/位移、整数/FP/syscall/Windows 设置与地址打包；先长分支布局再迭代缩短，call 保持 rel32。ARM 两遍测量/解析标签，无分支缩短，call 位移从 BL 实际位置算；执行 MOVZ/MOVK、整数/FP、存取、软件栈、地址和各 OS gate。

opcode、NUM、FP/存取/relocation 字段来自 catalog/emit 的声明；扫描、位打包、分支和地址算法来自 `exec/enc` 控制 TSV。**WINARGS_BODY 是明确保留的机器代码模板**，其中命令行解析还不是 δ；模板大小计入产物。重复/未定义标签、非法元数据、范围越界和破坏 scratch 的别名必须拒绝。check/armcheck/x86wincheck/armwincheck 既比参考字节，也保留手算/独立解码及边界探针；全域转移等价不等于所有 FP/整数指令语义已形式证明。

### 3.10 E6：镜像与内部完整性模型

共享动作解码 payload、应用重定位和裁掉文件零尾；实际内存范围仍保留。ELF 模型计算 ELF64 头、入口、两个 PT_LOAD，区分 p_filesz/p_memsz；Mach-O 模型生成段/section/dyld/LINKEDIT，通过 SHA256 δ 计算页哈希和 ad-hoc CodeDirectory/SuperBlob；PE 模型生成 PE32+、导入、cookie/load-config 与 DIR64 排序去重/页分组。不在运行时调用 Python image writer。

格式常量/模板来自 `unisa.image` 与声明文件；格式控制也有 TSV。Mach-O **ad-hoc 签名只满足镜像运行格式要求，不是 Developer ID、企业发行签名或公证**。PE 重定位的二次插入排序仍是规模限制。imagecheck/armimagecheck/machocheck/shacheck/pecheck 验格式与字节；客机运行、自举及签后运行是另一组证据，不能用“能写六目标”代替“本轮实跑六目标”。

### 3.11 `-run`：一次绑定与宿主装载

**驱动默认模式（主人裁定 2026-10-01）：不带 `-run` 也不带 `-o` 时，`unisacc FILE.c [args]` 就是运行模式**——与 cc 的“静默写 a.out”刻意不同，这是产品的特点，不是兼容缺口。`-o` 才写可执行文件，`-run` 保留为显式写法。0.0.16 及之前的产品按 cc 语义写 a.out；**0.0.17 R17-10 已在两条路线落地**（src/main.c、exec/c/compiler.c：命令行没有任何模式/输出旗标即 `-run`），README 用法、用法行、ccparity（唯一登记的与 cc 差异）与 cli 套件同步。

源码先到 lower，再给 memory 入口传实际 OS 预留地址、容量、argc/argv 和适用的动态导入资源。模型据真实代码/导入长度计算对齐与数据位置，**只生成一次最终绑定的 UNIMEM1**（magic、text/extent/stored/entry 四个 u64 及代码/保存数据）。模型负责布局；宿主 reserve/commit、校验、复制、设置权限、清缓存、进入入口，不解释 C/tape/指令。

预留约 2GB 虚拟区由 OS 选地址，不用 MAP_FIXED；Windows reserve 与原地址 commit 分开。失败和错误地址显式拒绝。`UNISA_MEMORY_TWOPASS=1` 只作同驱动比较基线，不能当默认路线。memorycheck 比同基址镜像、资源/范围失败、loader 边界和 Windows API mock；mock 不是客机实测。当前同身份 calc 五次暖中位为 P2/双遍 205.382ms、P3/双遍 207.770ms、P3/单遍 172.749ms；它是该输入的测量，不外推所有程序。证据见 [集成测量](archive/research/20260928/memory-once-integrated-bench-20260928.json)。

### 3.12 模型包、库与发布边界

P3 在动作共享前缀 `compact_q` 后，将网络编码为 `UNINETB1`：整数用规范 LEB128/zigzag，字符串按长度保留字节，再逐网络 raw-DEFLATE + 原长/CRC32。按路由只解需要的网络；校验与模型解析完成后释放解压缓冲。构建期保留 SHA256 与全域 check-net，CRC32 仅防损坏，不是企业信任。旧 P1/P2 的兼容路径和未知版本拒绝必须保留测试。

共享索引让同一模型内容只保存一份；Q/C 动作、H 参数和 S 字节声明分别计账，解压后逻辑量不与压缩体相加。头文件库源码是另计资源，当前没有全库 AOT 或全系统 libc 转发；macOS FFI 是有边界的宿主能力，不把它说成六平台都可任意调用系统库。执行器、驱动/OS 适配、模板、库和网络均是产品组成，不能只报纯推理核几 KB。

**产品边界：转发与 cc 互调只走参考路线**（0.0.24 收口 0.0.23 A1/A2，两次顺延已满按原计划兜底砍掉）：产品对 Windows 目标的 `.hostcall/.hostaddr`（libc 转发）和 `__ccx_`/`__ccw_`（cc 互调）保持带位置的按名拒绝，这是正式行为，不再排期；参考编译器两者都支持，forward 与 ccinterop 门禁只测参考路线，所以不设 com-forward、com-ccinterop。

**macOS 发布方案（0.0.13 起实行，每版公证并公开 dmg）**：采用 `unisacc.dmg → Unisacc.app → 签名的原生入口 → 同包封存 unisacc.com`，实际编译仍由 `.com` 完成，参数/退出状态应原样传递。先验证 quarantine/Gatekeeper、内层 APE 运行与解包缓存、`-run`/FFI，以及公证扫描能否接受整个 bundle。minicon 实际是先签内层 Mach-O，再签 app、公证并 staple app/dmg；不是给任意载荷加壳便完成信任。本方案若未过，不宣称 `.com` 已获得 Apple 签名，回报具体阻挡并评估同源原生内层，不静默更换编译路线。微软 Authenticode 与 Apple 分发链分别验，参见 R9-6。

### 七流开发工具的外置模型方向

设计约束（2026-10-24，尚未实现）：通用执行器`.com`与流配置、模型及资源分离。编译语义由模型数据表达；壳负责通用操作集、装载校验、输入输出协议和宿主适配。模型包包含转移、动作及权重，不只是一组数值。编译规则改变时独立重建受影响的模型包；操作码、数据格式、执行器ABI或宿主能力改变时才更新壳。当前七个开发`.com`仍附加各自模型，尚未达到物理分离。

先复用P3的模型／资源身份与范围检查，增加显式外部包入口；流配置声明输入输出格式、模型及资源依赖。权重构造与测试可按模块并行演进：候选包和测试版本各自保持不可变身份，测试独立维护输入／断言／参考，合并时固定壳＋模型＋资源＋测试的配对。按依赖共享资源，缺包、错配或不兼容必须具名拒绝。测试绑定壳SHA、配置／模型／资源SHA及实际输入与环境，只复测受影响闭包，保持独立对照；模型数据命中不等于旧测试PASS可迁用。开发外置与正式单文件封装共用协议。完整外部共享能力在[0.0.45计划](plans/v0.0.45.md)细化，不把重做发行架构变成本版发布前置。

### 3.13 验证分层与当前证据

| 要回答的问题 | 证据 | 不能推出什么 |
|---|---|---|
| 网络是否精确实现声明转移 | check-net 全部有限观察，包含缺转移/声明返回 | 声明自身就是 C99 语义 |
| 阶段是否保持参考行为 | 固定 keep、输出/退出/诊断、边界与失败探针 | 任意程序/未测组合全域等价 |
| 产品是否完成实际编译/运行 | `.com` 用户命令、cc 可观察差分、真实应用与自举 | 编译器与平台 cc 所有实现定义相同 |
| 包/宿主是否可靠 | codec、损坏包、资源、内存范围、API 失败 | CRC 已提供企业签名信任 |
| 发布物是否可分发 | 精确 SHA、来源、签后运行、平台和信任回执 | 旧候选门禁适用于新源码/重签字节 |
| 真实程序能否编译运行 | `realprog`（kilo/jsmn/cJSON/lua/sqlite 与 cc 同源构建同参运行，棘轮）、`tools`、`corpus` | 未登记的程序/库面 |
| 目标文件能否被系统工具接受 | `elfobj`（ELF/Mach-O/COFF `.o`：节与重定位集合、与映像的不变量、GNU ld/lld/ld64/lld-link 链接并在 Lima/本机/Windows 虚拟机运行同映像；读 cc `.o` 的数据符号；产品 Linux 对象与参考逐字节同） | 产品的 Mach-O/COFF 对象（0.0.18）、与 cc 的函数互调（0.0.18） |
| 分开编译与链接是否正确 | `linkunits`（单元单独编译再由 unisacc 链接 = cc = 一步编译；`.a` 只拉所需成员；产品链接与产品 ar 同参考） | 系统链接器链多个单元对象（不支持：初始化链只有 unisacc 会做） |
| 减法是否安全 | `subtract-safety`（归档文件无外部引用、指令文件可解析） | 行为层面的回归（由各功能门禁负责） |

出货身份以历史发布身份与各版回执为准；旧候选的队列、平台与 Apple 资格不移植到新源码或重签字节。T1是有限转移/网络等价；全程T2和C语义T3仍为独立证明义务。

<a id="model-function-bytes"></a>
### 3.14 当前模型功能与物理字节账

下账对应 0.0.43 本机开发候选 `75a52ae7f898a6db7ea32b810f250deb9ffda9a2fad2b1d8a6ec81efba4f8972`，不是发布字节。`research/model-bytes.json` 中 SHA `518601e5…` 属 0.0.24 历史候选，不作当前账。只按实际封装内容记物理字节；同一网络被多条阶段行引用时不重复相加。

<!-- model-bytes:begin -->
快照 SHA-256：`75a52ae7f898a6db7ea32b810f250deb9ffda9a2fad2b1d8a6ec81efba4f8972`；总计 **2,934,337 B**。源码 SID `718017ab98ae13963d0b928496724c0e661990b3972ba452909742ccfa6f6a81`；包 SHA `4d9b8a3a526f340709316f7fb67189603bbd15bcd38dbab5b7997617c505a83e`。

| 物理内容 | 文件内字节 |
|---|---:|
| Windows x86_64 PE，含脚本/版本资源；Windows ARM 入口共用 | 687,616 |
| Linux x86_64 gzip（展开 724,187 B） | 166,772 |
| Linux arm64 gzip（展开 752,796 B） | 153,761 |
| macOS x86_64 gzip（展开 726,754 B） | 171,586 |
| macOS arm64 gzip（展开 759,778 B） | 159,121 |
| 五片间及尾部对齐 | 56 |
| P3 `compiler.pkg` | 1,595,409 |
| `UNIPKG1` 尾 | 16 |
| **总计** | **2,934,337** |

包内 32 个网络压缩体共 1,088,123 B，74 个资源内容共 420,643 B；其余目录、记录头与键共 86,643 B。七流压缩体共 996,753 B：e3 342,284、lower 320,870、elf 185,908、e2 94,739、e1 28,340、e4 15,481、prune 9,131。阶段行引用网络内容，不另占一份网络体。四个 gzip 成员实际解压 SHA 与本轮四个 ELF/Mach-O 切片逐一相同；当前打包由宿主 GCC + zlib 完成，编译器本身未链接 zlib。五目标写出和本机 smoke 已验，外国宿主原生运行仍待。
<!-- model-bytes:end -->

旧快照、纠错和计数来源见 [结构测量史](archive/prd-release-and-development-history-20261001.md#结构测量与纠错)。

### 3.15 目标文件、分开编译与链接（0.0.17）

- **对象**：`-c -b os/arch` 写可重定位对象，格式随目标（ELF/Mach-O/COFF）。参考侧三格式都有；产品的 Linux ELF 由模型路由写出（对象专用 lower + ELF 写出 δ，与参考逐字节同），Mach-O/COFF 产品路由在 0.0.18（驱动按名拒绝）。整程序对象可被系统链接器单独链接，并可读 cc 定义的数据符号；与 cc 的函数互调需要调用约定适配（0.0.18）。设计与事实表见 [docs/toolchain.md](docs/toolchain.md)。
- **单元与链接**：`-funit` 写单元对象（tape 带 `.global/.extern` 链接属性；对象里另有不装载的节存单元 tape）。`unisacc a.o b.o lib.a [-o prog]` 在 tape 级合并（src/tapelink.c：私有名改 `__u<k>_`、全局按名相会、同名全局对象按 C 公共定义合一、各单元 `__init_u` 串成 `__init`），再走普通后端，六目标都可出；`unisacc ar rcs|t|x` 读写系统 `ar` 也认的归档（2026-10-28 cc：archive.product-ar 以宿主 ar 为 oracle 实跑 17/0 @f3705dfa；未定义外部函数的链接拒绝另立格 link.undef-function-refusal，当前为已知缺陷，见 plans/defects-c43.md）。产品驱动复用同一合并核心，编译决策仍全在模型路由。
- **驱动默认模式**：见 §3.11（裸 `unisacc FILE.c` 即运行）。

## 4. 质量体系

- 新合同按网、流、接口、产品、库/ABI和平台分账，当前覆盖见[tests/coverage.tsv](tests/coverage.tsv)，批次见[tests/batches.tsv](tests/batches.tsv)；[归档层次](archive/testing-c43-legacy/tests/GATE-LAYERS.md)只保留历史义务。局部绿不代全链，未覆盖不计通过。
- 清单、来源身份、实际受试字节与原始结果分别可核；旧C99条款与原失败见[归档条款账本](archive/testing-c43-legacy/tests/c99/clauses.tsv)，当前入口是 `sh ./unisacc.com tests/run.cx -- --smoke|--all|--list`。[旧清单](archive/testing-c43-legacy/tests/INVENTORY.md)不恢复调度。
- 当前开发按具名差集及必要合同门推进，准备/构造与测量单列预算；不得因历史流水线描述自动全量打包、重封或恢复旧队列。
- 发布流程材料见[release/RELEASE-PIPELINE.md](release/RELEASE-PIPELINE.md)、[release/README.md](release/README.md)。当前明确禁止签名、Draft、Latest及公开；历史授权不沿用。先完成新清单与同候选平台证据，再按明确新授权处理发布；旧队列不恢复（见§2）。

## 5. 路线

### 5.0 当前优先与0.0.x退出条件

当前先收口前级/库边界与晚绑定阶段合同，具体刀序和验收只在[0.0.43计划](plans/v0.0.43.md)维护。旧0.0.29–0.0.42时间表已归档，不因版本号推进而宣称义务完成。

**0.0.44 启动提速优先需求（主人2026-10-25）**：`-run`启动偏慢，同一程序第二次启动应有透明复用和实测加速机制，可顺延0.0.44；先拆相测量，缓存须绑定源码/传递依赖/候选/目标/选项，正确失效，每次仍真实执行程序及重新绑定运行资源，不缓存输出或PASS。需求与原C44-RUN-STARTUP-01合并排程；当前仅登记，未实现或宣称提速，详见[启动性能需求](plans/v0.0.44.md#优先启动性能需求主人2026-10-25)。

**0.0.44 已登记需求（主人2026-10-24）**：改进 `.cx` 模块化与内存安全设计，参考Rust等经验；持续把 `.py` 和大部分 `.sh` 改造为 `.cx`，形成自有工具链；改进测试套件结构及端到端速度。当前仅登记，尚未设计或实现，不插入0.0.43收口；范围及验收方向统一见[0.0.44计划](plans/v0.0.44.md#registered-requirements)。

**0.0.45 已登记需求（主人2026-10-24）**：继续改良流＋网范式的模块边界，使模块成为外部可调用、可共享的工具能力；支持不同`.com`程序动态共用这些模块和权重，构建更丰富的测试及应用功能。当前仅登记，具体接口和共享机制待设计；见[0.0.45计划](plans/v0.0.45.md#registered-requirements)，不加入0.0.43发布前置。

进入0.1.x前，自有构造、构建、自举、测试、队列、缓存、报告及发布工具链迁到C/.cx；在没有Python、外部cc和shell的环境中，由上一版unisacc.com构建下一版并达到自举不动点、完成本地适用测试。外部对照编译器仅作可关闭的预言机。不得改后缀、藏脚本或由.cx调用Python/shell冒充完成；按依赖族迁移，保留原检查、失败语义、故障注入及新旧对拍。归档不等于删除未端口义务，archive/ujs等范围逐项清点，不默认豁免。

种子仍按两个代际命名：unisacc-seed.com与unisacc.com；C种子构造器和声明规格的完整性必须有同源实证，Python保留为对照不冒称已退役。

### 5.1 libc与前级冻结

优先复用系统libc，按函数族明确“转发/保留/拒绝”，不无限扩头内库体；方向是独立provider/外置运行库与声明头分离。单函数边界刀只换实现时P/M必须不变，头资源输入变化与模型语义变化分账，禁止无pp表/模型实证随库刀reseal。

通用转发/目标装载与ABI资格分别核验；声明有名不等于签名可调用。宿主地址只在绑定层进入。静态/动态不是硬性二选一：纯计算且影响确定性的保留项须具名，其他库实现可独立静态链接；不把静态链接等同于头内实现。允许按需借用合规实现并保留版权/NOTICE，禁止整包内置替代系统libc或无界自研。来源与细账见[libc移交](research/libc-forward-handoff.md)、[历史过程归档](archive/prd-process-notes-20261021.md)。

### 5.2 未排期研究与构想（不自动开工）

- 权重表示及推理成本：优先研究同权阈值段精确聚合、类型化动作模板/银行归一化；分别量权重、运行内存与同身份延迟。包压缩不冒称权重减少，结构统计不冒称真实提速；网络语义及完整接口保持是前提。
- 可移植Tape包、嵌入式库/API与包管理、wasm第七目标；表解释性/语义完备性与证明；向量化、工具链去脚本及队列改进。已有实施部分归当前计划，不重复排队。
- DSL 批量重构工具（主人 2026-10-29，仅登记）：设计并制作代码重构工具，范围含迁移。它是 file 工具的增强版，能对指定 DSL 文件批量并发修改，用来提高代码进化速度。当前不设计、不实现，也不插入进行中的 CX 粗迁。
- **csih C->CX 导出的工具／脚手架缺口（2026-10-29，政委收口；后面再探索改进方案，不自动开工）：** 经验与测据见短结 [apps/csih/docs/c-to-cx-report.md](apps/csih/docs/c-to-cx-report.md)，宫殿已指该文。P0：CX `-run` 跨进程内容寻址缓存（键勿绑 `/tmp/ua-cxif-*`）；正式 `CSIH_BIN`／源摘要变则重建与缺产物具名拒绝。P1：`.c`→`.cx`／`#include`→`use`／残留 `.c` 扫描／下层 include 白名单一条命令；`check.cx` 滚动分桶门禁（输入哈希、嵌套超时与直跑分账）；cc fuzz「库 TU+cx.h／驱动 TU+API.h」配方入文档。P2：CXIF 导出面 diff、`use` 闭包计数与超限诊断、禁上层 include 静态闸；e2e 减 Python（pty 见证 CX 化或最小宿主助手）；`-cx-plan` 可读摘要。P3：测用 native 缓存与产品默认 `-run` 合同分离；`self-iter.md` 等旧 `.c` 路径随手清。与上文 0.0.44 启动复用登记相关但不合并开工；不授 Draft/Latest/签/公开。
- 原FX-1–FX-8细稿及旧测量完整留在[构想归档](archive/plans/ideas.md)，旧收官排期在[路线图归档](archive/plans/roadmap-0.0.x.md)。构想不构成产品/发布授权。

### 5.3 0.1.x：其他 C 编译器接入 `.cx`

**规划决定（主人 2026-10-24）**：在 0.1.x 制作共用 `.cx` 前端、构建驱动器及必要的编译器插件/适配，使 Clang、GCC、TinyCC 等工具链支持 `.cx`。具体安全语义以 [`.cx` 草案](docs/cx-spec-draft.md) 后续形成的稳定规范为准。`.cx` 继续采用 C99 语法，通过模块接口、契约和受检查的编译配置表达能力，不增加 Rust 式源码关键字。

**已落地的宿主驱动（2026-10-26）**：源码是 `tools/gcc_cx.cx` 与 `tools/clang_cx.cx`（实现在 `tools/cxcc.cx`）。六目标 APE 由 `./unisacc.com -run tools/build-cxcc-ape.cx` 写出，放到仓库根的 `gcc_cx.com` 与 `clang_cx.com`，不入库。五个切片是 `win/x86_64`、`lnx/x86_64`、`lnx/arm64`、`osx/x86_64`、`osx/arm64`；`win/arm64` 走同一份 x86_64 PE。Unix 上用 `sh ./gcc_cx.com` 启动，shell 按 `uname` 解出切片，缓存名是 `~/.cache/gcc_cx` 或 `~/.cache/clang_cx`，所以从 argv[0] 找不到仓库时要在仓库根启动，或设置 `CX_ROOT`。驱动把首行 `#!` 涂成空格。没有 `#pragma unisa use` 的单元仍一次交给系统 `gcc` 或 `clang`，参数是 `-std=c99 -x c -include <仓库>/include/cx.h`。有 `use` 时，驱动执行本仓库 `unisacc.com -cx-plan`，按每个单元自己的 include 列表分别编译再链接。解析器只有这一份。产物是普通 C，不授予「已通过 `.cx` 检查」的身份，所有权检查仍未实现。Windows 切片用 `posix_spawnp` 启动系统编译器，也用它抓取 `-cx-plan` 的标准输出；临时文件先看 `TMPDIR`，否则用 `GetTempPathA`。本机没有 Windows，该切片只编译进 APE，没有执行。本机有 `/usr/bin/gcc` 和 Ubuntu clang 15.0.7。OSX 切片只写出、未执行。草案中的版本化 CXIF 对象和编译器插件仍未实现；这一步用的是 unisacc 已有的 CXIF v0 文本头。

**宿主驱动（2026-10-27）**：本机 gcc 与 clang 在 `-std=c99` 下碰到 `#pragma unisa use` 会丢掉这一行并退出 0；`-Wall` 才给出 `-Wunknown-pragmas`，编译仍然成功。gcc 要在 `-fplugin` 的 `PLUGIN_PRAGMAS` 回调里 `c_register_pragma("unisa", "use", ...)` 才会执行它；clang 要注册 `PragmaHandler` 到 `unisa` 名字空间。两套插件 ABI 不通用，也不能代替跨文件的模块图。当时的驱动因此仍调用本仓库 `unisacc.com -cx-plan`。未声明的函数现在是错误，跨单元调用文件内函数会被拒绝，导出函数仍可链接。单元目标文件按前奏和源字节缓存在 `out/cxcc-cache/`，不写入 `include/`。产物仍是普通 C，不授予已检查身份。

**宿主驱动（2026-10-28，-o）**：`tools/build-cxcc-host.cx` 用系统 gcc 把 `tools/cxcc.cx`、`src/cxmod.c` 与 `tools/gcc_cx.cx`（clang 同理）链成 `out/cxcc-host/` 里的程序。仓库根 `gcc_cx.com` 与 `clang_cx.com` 是六格 APE，运行时不调用 `unisacc.com`。有 `#pragma unisa use` 时，模块图在驱动进程内展开，再交给系统编译器写出 `-o` 的可执行文件。没有后续非选项输入时不追加 `-x none`；本机 clang 对 `tests/cxcc_one.cx -o OUT` 标准错误为空，程序退出码 5。后面还有目标文件时仍追加 `-x none` 并完成链接。`-run` 尚未做。本机跑过 Linux x86-64 那一格；其余四份切片只写出。`sh ./gcc_cx.com tests/cxhost_main.cx -o OUT` 与 clang 同样得到退出码 42；缺模块、循环和 `std.absent` 在编译器启动前拒绝。`strace` 没有执行 `unisacc.com`。小套件是 `tests/cxcc/units.tsv`，由 `sh ./gcc_cx.com tests/cxcc/suite.cx -o out/cxcc-suite/suite` 编出后执行；gcc 与 clang 各跑一遍。对象缓存除直接 `-include` 外，还按编译器 `-MMD` 列出的头文件字节失效：只改被包含的 `nested.h` 时，两边都从退出码 8 变为 12。行首 `#pragma  unisa use`（pragma 后两个空格）会展开。编译副本前插 `#line 1`，`__FILE__` 与错误行都是原来的 `.cx` 路径和行号，例如 `tests/cxpriv_bad.cx:3`。热 `-o` 三次中位 gcc 38 ms、clang 69 ms，上限 268 ms 和 227 ms。cdx 用当前源码另建 `out/c43-grk-review-{gcc,clang}-02.elf`，日志 `out/c43-grk-review-02.stdout` 为 `REVIEW findings=0 harness_errors=0`：两边 canonical 与双空格 pragma 均为 7，只改 `nested.h` 为 cold 8、warm 12、forced 12，`__FILE__` 为原始 `.cx` 绝对路径。这只关本机这三项，不授其他平台或 `-run`。同日补 win/osx 的构建方式：假定 PATH 上有对应的 gcc 或 clang，`tools/build-cxcc-host.cx` 在那台机器上用该编译器链出驱动，不调用 `unisacc.com`。同机 `tools/build-cxcc-host.cx` 仍可把驱动链到 `out/cxcc-host/`。仓库根的 `.com` 是六格 APE。驱动调用 POSIX 的 `posix_spawnp`、`waitpid` 和 `mkstemp`。Linux 与 OSX 走系统 libc，环境表传 `environ`。Windows 的 `gcc_cx.com` 是本机程序；同一组调用的函数体在 `tools/cxcc-hostinc/spawn.h`，按本仓库 `include/spawn.h` 与 `mkstemp` 的形状用 `CreateProcessA` 和 `open` 实现，不走 `_ux_sym`。`pid_t` 放不下 HANDLE 时用一张小表，空环境在 Windows 上表示继承。编译行加 `-I tools/cxcc-hostinc`，这样 `<spawn.h>` 和 `<sys/wait.h>` 用这一目录，而不是产品头。同一目录还有 POSIX 的 `mkdir` 与 `realpath`。本机用 zig 0.13 交叉编过并链出 `x86_64-windows-gnu` 的 `gcc_cx.exe`、`clang_cx.exe`、`build.exe`，以及 `aarch64-macos` 的驱动和构建器、`x86_64-macos` 的驱动。这些二进制没有在 Windows 或 OSX 上执行。本机 Linux 套件 `fails=0`，热 `-o` 中位 gcc 36 ms、clang 72 ms。`strace` 仍由 PATH 找到 `/usr/bin/gcc`，没有 `-I tools/cxcc-hostinc`，也没有执行 `unisacc.com`，程序退出码 5。`-run` 仍未做。六格 APE 已重打，只跑过 Linux x86-64。

**构建收据（2026-10-26，模块展开之后重打）**：`./unisacc.com -run tools/build-cxcc-ape.cx` 实际 rc 0。`gcc_cx.com` 617424 字节，`clang_cx.com` 617456 字节，`file` 均为 PE32+ console x86-64。构建里 gcc 编出的 `ujs/seed/construct/lex.cx` 对 `ujs/tests/corpus/arith.ujs` 与 `./unisacc.com -run` 逐字节相同；`tests/cxhost_main.cx` 展开 `cxhost_mod` 后退出码 42。同一次构建里 Ubuntu clang 15 编过该扫描器，并编过同一夹具、退出码 42。`-run` 另测：用 gcc 编出的 `gcc_cx` 再编 `tests/cxhost_main.cx`，退出码仍是 42；缺模块 `no_such_mod_zz` 由 unisacc 具名拒绝（rc 1），没有交给 gcc。字符串里的 `#pragma unisa use` 不当作导入。`win/x86_64` 的 `tools/gcc_cx.cx` 只编译通过，没有执行。OSX 切片只写出、未执行。活树棘轮 `tests/cxinclude.cx --check` 与 `tests/cxapis.cx --check` 实际 rc 0：436 个 `.cx`，111 行 include，724 处无界调用。

**Windows POSIX／C99（主人 2026-10-26，对照 Cosmopolitan 的目录模板、临时目录和最终路径；未在 Windows 上执行）**：`include/stdlib.h` 的 `mkstemp` 在 Windows 上用 `CreateFileA` 的 `CREATE_NEW`。`realpath` 能打开的路径先问 `GetFinalPathNameByHandleA`（`VOLUME_NAME_DOS`），再去掉 `\\?\`，并把 `\\?\UNC\` 收成 `\\`。目录打不开，因为 `__open` 传不进 `FILE_FLAG_BACKUP_SEMANTICS`；这一路和最终路径失败都退回 `GetFileAttributesA` 加 `GetFullPathNameA`，退回路不解析符号链接。`mkdtemp` 在 Windows 上用 `CreateDirectoryA`，模式忽略；模板必须以 `XXXXXX` 结尾，`EEXIST` 会重试，其它错误和重试耗尽会把 `XXXXXX` 写回去。这些调用展成宿主通道宏，这样当前编译器的库依赖表还没记下新函数时，被调函数也不会被裁掉。本机 Linux 探针实际跑通 `mkstemp`、`mkdtemp` 和 `realpath`；`win/x86_64` 探针只编译通过。`fork`／`execvp` 仍不映射，也不移植 Cosmopolitan 的 Windows `fork`。`include/cx.h` 的 `cx_run`、`cx_run_timeout` 与 `cx_runv_timeout` 在 Windows 上调用 `posix_spawnp`（标准输出经 `mkstemp`），`cx_walk` 调用 `opendir`，`cx_read` 对目录返回 NULL。系统 gcc／clang 没有 `_ux_sym`，所以 `tools/cxcc-hostinc` 提供 `dirent.h`、`spawn.h`（文件动作、`kill`、`usleep`，`open` 加 `O_BINARY`）和 `sys/wait.h`；Windows 编译行在 `-include include/cx.h` 之前加 `-I tools/cxcc-hostinc`。`cx_remove`／`cx_rmtree`／`cx_trash` 仍由 `unisa_safedel` 在 Windows 上拒绝。这些 Windows 路径只做交叉编译，没有在 Windows 上执行。

#### 5.3.1 共用前端与后端分工

优先建立以下接入路径：

```text
.cx 源码 + 包清单/锁文件 + CXIF 接口与契约
    → 共用 .cx 前端：模块解析、类型/安全分析、必要的运行期检查插入
    → 确定性生成的普通 C 编译单元
    → Clang / GCC / TinyCC 等后端编译与链接
```

共用前端拥有 `.cx` 规则的权威实现，读取 CXIF 的类型、布局、公开符号、效果契约和来源身份。后端接收显式声明与函数体；从受检程序降为普通 C 必须保持相应语义，不能在降级时丢掉检查或改变模块可见性。自动前奏、模块依赖与构建顺序可先由驱动器实现；所有权、生命周期、初始化、范围及可写别名检查需要类型与控制流分析，并按规范插入必要的运行期检查，不能由后缀、宏或头文件约定代替。

正式模块消费 CXIF，不重放提供者头文件或源码 token。为普通 C 互操作生成的声明头属于显式 C ABI 边界，不能当作 CXIF 的等价替代。未经验证的 C 实现、FFI 与受信任模块须单列契约和信任范围；不能因为后端能编译就把它们算入 `.cx` 安全保证。

#### 5.3.2 编译器插件与适配职责

| 工具链 | 计划接入方式 | 职责边界 |
|---|---|---|
| Clang | 先用独立驱动器/检查工具，评估 LibTooling；再按需要提供 pragma、属性、诊断和构建集成插件 | 复用共用契约与检查规则；Clang AST/控制流适配的等价性须验证 |
| GCC | 共用前端生成 C；后续提供 pragma/属性及分析流程的插件适配 | 插件只承担 GCC 特定的导入、诊断或编译流程接线，不维护另一套独立安全语义 |
| TinyCC | 共用前端生成 C，由驱动器调用 tcc，或通过 libtcc 嵌入编译执行 | 优先复用其 C 编译后端；若需要内部前端扩展，须独立评估维护成本及接口稳定性 |
| 其他 C 编译器 | 先提供普通 C 输出与目标 ABI 适配，再决定是否需要专用插件 | 逐个声明支持的版本、目标、C 能力和运行库条件，未验证组合明确拒绝 |

这些入口有现成基础：[Clang 插件](https://clang.llvm.org/docs/ClangPlugins.html)可注册 pragma/属性，[LibTooling](https://clang.llvm.org/docs/LibTooling.html)可构建独立工具；[GCC 插件](https://gcc.gnu.org/onlinedocs/gccint/Plugins-attr.html)可注册 pragma/属性；[libtcc](https://bellard.org/tcc/tcc-doc.html#SEC19)可接收 C 代码作为编译后端。存在扩展入口不代表已经实现 `.cx`，各版本适配及检查能力都须实际验证。

#### 5.3.3 编译约定与检查身份

包清单固定模块名、语言版本、公开集合、依赖、契约和目标；源码中的 `#pragma unisa cx/module/use` 与清单相互核验。严格模式中的未知 pragma、非法 include、缺失依赖、接口不匹配和未支持语义必须产生具名错误并阻止出货。

检查与后端编译必须绑定同一份宏展开后的程序、接口、目标 ABI、编译配置及受信任运行库。生成 C、检查结果和构建收据记录来源映射与内容摘要；缓存键覆盖 CXIF/契约、前端版本、后端工具版本及影响语义的选项。原生编译器可能忽略未知 pragma；绕过驱动器直接把 `.cx` 当普通 C 编译，不能获得“已通过 `.cx` 检查”的身份。仅识别后缀或注入前奏也不授予该身份。

#### 5.3.4 分阶段交付与验收

1. **公共接入基线**：先固化最小语言/模块配置、CXIF 版本与后端协议；实现驱动器、来源映射和普通 C 输出，以小型真实模块程序验证接入。
2. **检查能力逐片落地**：按草案把初始化、范围、所有权/生命周期、别名及运行期检查拆成可验证交付；每片明确保证域和拒绝域，不从一片通过推定完整内存安全或线程数据竞争排除。
3. **Clang 插件与工具集成**：复用同一规则和测试语料，验证 pragma/契约接入、诊断位置、IDE 与构建流程；固定首批支持版本。
4. **GCC、TinyCC 与扩展后端**：完成对应适配和安装包，验证模块、C ABI/FFI、优化与运行库兼容；再扩大版本和目标矩阵。

共同验收要求：合法真实程序在相同目标上的可观测行为及 ABI 一致；共用前端对非法程序的接受/拒绝和具名原因一致；前端输出确定，但不要求不同编译器生成相同机器码。测试须覆盖缺依赖即使碰巧链接也拒绝、接口/目标/宏配置变动使缓存失效、实现变更且接口不变时消费者无需重编、绕过检查不能冒领收据，以及干净机器构建运行。安全结论须同时有规则依据、正反例和降级保持证据，不能只凭编译成功或 sanitizer 没报错。

## 6. 未解决问题

- **论文 A 口径**：表 2/表 4 与 v0.0.9 的数字是绑定旧候选的历史实验；§8 当前能力边界须与 §3.15 对齐。目标文件/分开编译已实现的范围不可再写成“没有 `.o` 工作流”，未完成的是产品 Mach-O/COFF 对象与外部函数 ABI 互调。

- **正确性与诊断**：产品 5 类拒绝缺位置（B1）与 `c_struct_ret` 在 0.0.22 第 4 项；全局复合字面量（A1）已于 0.0.20 闭合；续行前 token 的 `__LINE__`（A2）见 0.1.x 附录。
- **C99**：复数与三字符组决定不做（账本写明理由）；库函数按“转发 / 保留 / 拒绝”路由表处理（§5.1），不再逐个补函数体。
- **工具链**：产品侧的 cc 互调、Windows 转发和产品 Mach-O/COFF 对象未闭合（0.0.22 第 1、11 项）。
- **反复顺延的架构项**：通用 BANK/nativeabi、Windows SEH、us_eval/us_reload 已写入 0.1.x（D1–D3）。
- **流程**：换工作树续跑整轮作废（指纹含路径）、模拟机误报、候选绑定 main HEAD 导致重封——0.0.22 第 2 项；Linux 客机固定的环境红（4 GiB 内存不够、没有 clang）照旧点名登记。

- **种子层迁移倒退（2026-10-03 实测，主人强烈不满）**：exec/*.py 中直接造转移的手写控制，v0.0.17 为 1,650 处，v0.0.19 1,680，v0.0.20 1,692，v0.0.21 1,725，v0.0.22 1,732，现在 1,735；同期 exec/ 下的 .tsv 声明从 462 个只增加到 464 个。也就是说，最近几轮的产品追平都是在 Python 里加控制，没有写成表。处理：decision-ledger 门禁（只降不升，07dc885）防止继续增长；0.0.23 K 项要求本版迁完（总数 ≤ 200，M3 前不发版）；论文 A 必须如实写明这一点（0.0.23 J）。
  - **现状（0.0.24，2026-10-04）**：K2 之后阶段专用 .py 为 0；decision-ledger 实测直接造转移的位置只剩 9 处（基线 10，只降不升）；剩下的债是 facts 里的 385 处动作序列（T1e，顺延到 0.0.25）。上面的 1,735 是 0.0.22 之前的历史数字。

- **gettimeofday 在 fork+wait 后跳涨 2^32 微秒（cx-lab 实验副产物，2026-10-09，未修）**：本机 unisacc.com 0.0.35，同一进程里背靠背两次 `gettimeofday()` 相差 0.000000s；但只要中间跑过一次 `fork()+waitpid()`（哪怕子进程只是 `true`），下一次 `gettimeofday()` 立刻比预期多出约 4294.967296s（≈2^32 微秒），像是返回值在 fork 之后被错位多算了一轮 2^32。触发路径：`include/cx.h` 的 `cx_run_timeout`/`cx_runv_timeout`（内部 fork+waitpid）之后，调用方进程自己再调一次 `gettimeofday`。影响：任何“fork 子进程前后各自测一次 gettimeofday 算耗时”的写法都会算出垂圾数字——`examples/cx-lab/time_run.cx` 最初版本就中招，六行计时全变成 ~8589s；已在该 lab 脚本里绕开（让计时发生在被 fork 出来的 bash 子进程内部，用 `TIMEFORMAT=%R` 把耗时当字符串读回来，不再跨 fork 调用自己的 gettimeofday），没有碰 src/ 或运行时实现，产品代码未改。未验证范围：是否只在这台机器/这个版本/`-run` 内触发，是否也影响 `clock_gettime`（include/time.h 两者都映射到同一个宿主 gettimeofday），是否波及任何产品内计时相关功能或套件。

- **`include/cx.h` 缺独立 stderr 捕获（cx-lab 实验副产物，2026-10-09，未修）**：`cx_run`/`cx_run_timeout`/`cx_runv_timeout` 只把子进程 stdout 接进调用方给的缓冲区（`dup2(fd,1)`），stderr 原样继承自父进程，没有单独通道。把 `tests/runtimeiocheck.py` port 成 `examples/cx-lab/runtimeiocheck.cx` 时需要同时断言“stdout 等于某行”和“stderr 为空”（原 Python 版本分别拿 `subprocess.run` 的 `stdout`/`stderr` 两个字段判断），cx.h 现状做不到分别拿到两路输出，只能退一步在命令字符串里 `2>&1` 合流后整体比对期望字符串——如果 stderr 真的非空，合流后的内容就不会精确等于期望行，等价地抓住了错误，但拿不到 stderr 本身的内容用于诊断信息。影响：任何要迁的 `.py` 测试如果像这样既要分别看 stdout 又要分别看 stderr（不少 tests/*.py 用 `subprocess.run(capture_output=True)` 正是这种模式），都会撞上同一个缺口。没有改 include/cx.h；如果 0.0.38–0.0.39 真要批量迁 tests/ 下的 170 个 `.py`（§5 路线），这里需要先补一个 `cx_run_timeout` 的变体（例如把 stderr 另 dup2 到第二个临时文件）。

- **signal() 不向内核注册处理函数（2026-10-29，未修、有复现、csih 已兜底）**：rcg2 报告，unisacc 运行时的 `signal()` 不调用 `rt_sigaction`。`signal(SIGTERM, h)` 返回成功；本进程 `raise(SIGTERM)` 会调用 `h`；外部信号（`kill -TERM`、关 pane 的 SIGHUP）直接杀死进程，`h` 不执行。`-run` 和 `-o` 都这样。复现目录 `out/csih-signal-repro/`（`sigprobe.cx`、`README.txt`）。`strace -e trace=rt_sigaction` 跑 `-o` 编出的二进制，报告里没有 `rt_sigaction`。这次登记没有重跑 strace。影响：csih 被杀掉或崩溃时停在 raw 模式、鼠标上报还开着；csih 自己的 SIGINT 处理也没被外部信号真正调用。csih 已在 `csih.sh` 外层用子进程兜底，信号退出时由 shell 还原 termios。代码在 `apps/csih/term.cx` 的 `term_install_exit_handler`。运行时修好后这条兜底不用改就会走到真正的处理函数。另外 unisacc 头文件没有定义 `SIGBUS`。这次不改运行时。

### 6.1 研究问题立项：表的可解释性与完备性（主人 2026-10-03，立项不打断 K2）

主人原问：① 构造出来的表数据（即使由 LLM 起草）如何具备**可解释性**；② 如何**计算并证明完备性**——现在只靠 TDD 检验，没有机制说明编译器还有多少不完备，“很没底气”；现有推论只挽回一点信心，缺严谨理论推导，需研究补上。

背景（事实）：表→网络是确定性构造，网络=表已全域对拍（E3 全域穷举）；风险在**表本身**是否对——表由 LLM 起草、靠测试逼出，测试没覆盖的组合错了不会被发现。

董秘推论（供研究出发，非定论）：
- **可解释性三层**：(a) 出处——每行标注 C 标准条款 / 语法构造 / 逼出它的反例，LLM 起草必须填；(b) 见证——每行至少一个最小 C 程序走到它，删掉该行就有输出变错，无见证 = 死数据或盲区；(c) 可反查——编译任意程序能列出走过的表行，输出指令回指表行。
- **完备性三层**：(a) 语法层——对照 C 标准文法逐条产生式，可算出 N/M；(b) 表层——统计每行 / 状态×输入组合的覆盖率，未走到即待补清单；(c) 语义层——只能逼近：Csmith + 按文法系统生成探针持续与系统 cc 对拍，以每万程序新增缺陷率趋近零为证据。
- **unisacc 独有条件**：表是有限状态机，局部可穷举——可把 E3 的穷举从“网络 = 表”推广到“表符合文法 / 语义规格”，在局部给出真证明。完整证明 C 编译器完备做不到（CompCert 只证正确、不证完备），需研究的是理论框架。
- **建议顺序**：先做表行覆盖率 + 见证（便宜，两问共用）→ 文法产生式对照得第一个完备性百分比 → 语义统计常跑。

状态：已立项；0.0.24 只做了逐条目契约（六个阶段，T2）。表行覆盖率加见证排在 0.0.25，与 T2 余项（每个表条目、每个 DSL 操作配独立用例）合并成一项；文法产生式对照与语义统计排在它之后，具体次序等政委定。

### 6.2 论文 A 的条件性理论连接与实现审查

**核心定位与构造方法。** Paper A 的核心是基于神经网络的编译器：编译阶段的控制与决策由网络及通用执行器推进。TSV 声明和展开后的有限转移表，是本文采用的网络构造、审查与验证工具，不是这一类编译器的定义性要求。由规则、程序或 IR 直接构造网络也可以是候选路线，但本文尚未实现或验证这些路线；现有精确构造与穷举验证结论仍限定于本文给定的有限表及执行条件。

论文分工决定：系统构造编译器控制表的方法作为独立衍生论题 **Paper E**（见 [意向书](research/paper-e-intent.md)）。Paper A 聚焦基于神经网络的编译器，本文以给定有限控制表到阈值网络的精确构造及工程实例支持这一定位，不承担完整 C99 控制表的自动合成或语义正确性证明。unisacc 的构造方法继续在实践中改进，成熟结果再支持 Paper E；不以新论文计划冒充已完成能力。

**Paper D 与 `.cx`（2026-10-28 主人裁定）：** 模块化与内存安全的论文叙事归 **Paper D**（[意向书](research/paper-d-intent.md)），以 `.cx` 架构为主载体；流水线上「不改一行 C99」的可选安全节点为对照面。A 正文只可点到工程载体，不写 CX 细则；不另起第六篇。五线总表见 [research/README.md](research/README.md)。工程阅读仍从 [docs/cx/prd.md](docs/cx/prd.md)／[cx/README.md](cx/README.md) 进入。

**Paper A arXiv 主类（2026-10-16，政委）：** 投稿身份主类 **cs.AI**；可选交叉 **cs.PL**。账号默认曾为 cs.PL；CN/EN 与 `research/arxiv-paper-a/` 已对齐。未投稿。


当前决定：论文区分 **T2a（同一通用执行器中的控制器替换）** 与 **T2b（独立参考编译器的整程模拟）**。T2a 在相同初始化、有限观察、动作目录、原语语义、全接口、整数范围与逻辑资源预算条件下，以逐步归纳得到轨迹及接受/拒绝/发散等价；这是数学推导，不冒称实际 C/汇编循环已由 Lean 证明。T2b 与源语言语义保持 T3 仍开放。

固定阶段的续点字母表有限，无界栈高度不使返回成为无限决策；声明式返回是完整控制器中的等价因子化。阶段语义由权重、返回数据、动作目录与常量共同承载。整管线组合要求字节、状态、位置及资源元数据全接口一致；字节接口不承诺有界内存或在线输出。

索引：[论文理论连接与只读代码审查](research/paper-a-theory-review.md)。优先改进候选：部署求值器的域/范围证书、通用原语的边界契约、阶段完整接口与可回放轨迹、E3 语义组合与状态不变量。建议仅登记，产品改动由 cc 编排。

## 7. 归档索引

- [2026-10-22过程归档](archive/prd-process-notes-20261022.md)：旧宿主第一至第七次根对重建/归位收据与坑位；仅历史。
- [2026-10-21过程归档](archive/prd-process-notes-20261021.md)：原PRD完整快照、csih长会话、W44/W45窗、逐日细账及原未提交追加；仅历史，不授恢复或验收。
- [发布与开发历史](archive/prd-release-and-development-history-20261001.md)：逐版发布身份及旧验收。
- [早期PRD](archive/prd-history.md)、[迁移日志](archive/s17-migration-log-20260928.md)、[R9/R10回执](archive/prd-r9-r10-receipts-20260929.md)、[问题发现编号](archive/prd-findings-20260929.md)。
- [R17过程](archive/prd-r17-notes-20261001.md)、[0.0.21后旧条目](archive/prd-notes-20261002.md)、[旧计划](archive/plans/)。

F1 落地补充：现有 glossarycheck 闸扫描全部受跟踪的新文件，须在 gatedeps 中声明受跟踪顶层目录和根文件；只补输入声明，不改规则或豁免。

## 8. 历史收口记录

已移至 [archive/prd-c43-history-20261030.md](archive/prd-c43-history-20261030.md)。
