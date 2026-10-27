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
| 用系统 GCC／Clang 把 `.cx` 编成普通可执行文件 | `sh ./gcc_cx.com FILE.cx -o OUT`；Clang 用 `sh ./clang_cx.com` | 仓库根六目标 APE，由 `./unisacc.com -run tools/build-cxcc-ape.cx` 写出，不入库。抹掉首行 `#!`。无 `use` 时一次执行系统 `gcc` 或 `clang -std=c99 -x c -include include/cx.h`。有 `#pragma unisa use` 时用本仓库 `unisacc.com -cx-plan` 展开，按每个单元自己的头列表编译再链接。不授予已检查身份。见 §5.3。 |

新动态逻辑优先由 `unisacc.com + .cx` 完成，减少 `.sh`，不新增临时 `.py`。旧脚本尚未迁移的部分如实记为依赖，不把 `.cx` 转调脚本记成迁移完成。经典 C 参考与模型产品是不同实现；对拍前确认实际两端身份，不能笼统判成“对自身验证”。入口已实现不等于任意输入已验证，继续保留原断言、拒绝和预算。

本机开发控制器可用 `tools/dev-run tests/run-batch.cx -- ROOT ABS_OUT BATCH_ID` 复用原生 ELF 代码；它每次仍调用已装产品实际运行所选 SOURCE 批，不能把它产生的开发速度当作产品默认 `-run` 提速或发布资格。新检出先按 `plans/v0.0.43.md` 重建忽略的启动 ELF；当前仅 Linux/x86_64 可用。

## 0. 概念与术语（先读；歧义以本节为准）

> 本节只做一件事：让「流 / 网 / 段 / 阶段」四个词各自只有一个意思。详细版与逐处核实见 [docs/glossary.md](docs/glossary.md)。

### 0.1 五层，每层只有一个名字

| 层 | 唯一名 | 含义 | 数量 | 真源 |
|---|---|---|---|---|
| L1 | **Stream**（别名 Flow） | 字节管道阶段：输入格式 → 输出格式 | unisacc **7**（e2/e1/e3/e4/prune/lower/elf）· m4/ujs **4**（lex/front/select/encode）· tdesc **3**（declare/context/size） | `exec/pipeline/image-stages.tsv` · `m4/contract.py FLOWS` · `flownet/tdesc/contract.tsv` |
| L2 | **Net** | ask 事实网：一张有限表 + 全键穷举 exact | **18**（8770 键）· **10**（3677 键）· **3** | `unisa/gold.py` · `ujs/seed/construct/gold.py` |
| L3 | **Segment** | 训练合并段（多网合一的 masked 多任务模型） | 3（s1/s2/s3） | `unisa/control/segment.py` |
| L4 | **Book phase** | 一次推导中 ask 的记账阶段 | 由 contract 声明 | `flownet/contract.cx` 的 `phase` 行 |
| L5 | **Release phase** | 发布流程阶段 | 10 | `release/tools/stagelog.py` |

### 0.2 六条常见误解（逐条对照，不要按直觉读）

| 如果你这样理解 | 正确的是 | 为什么容易错 |
|---|---|---|
| 「7 流」与「18 网」是同一批东西的两种叫法 | 流是**字节管道阶段**，网是 **ask 事实表** | 旧句「勿混」只否定了互换，没说明二者各是什么 |
| `class Stage`（`unisa/gold.py`）表示流 | 它表示**网**（18 张的规格定义） | 类名抢占了「流」的词 |
| `unisa/control/net.py` 的 `Net` 是事实网 | 它是**神经网络模型**（TableNet / StageNet / UnisaNet） | `net` 一词被两处占用 |
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

<a id="work-tree"></a>
### 思维树

**2026-10-27 cdx 主刀计划（主人本轮改派；重构造仍串行，互不抢文件的轻切片由 cc／Codex 辅助）**

完成门（`plans/v0.0.43.md:42`）：19 条 NOT_READY 宽行、完整 N22、五外宿主同候选实证全部闭合后，`--all` 才可 rc0。

**当前矩阵账（2026-10-27 对 19 个 `tests/*-domain-matrix.tsv` 数据行实算；旧 `out/c43-cc-plan-matrix-counts.txt` 已过期）：** 822 个必选格中，245 个标 `COVERED_LOCAL`、3 个 `OBSERVED`、408 个 `MISSING`、38 个 `NEEDS_PRODUCT_CHANGE`、79 个 `NEEDS_FOREIGN_HOST`、33 个一般 `BLOCKED`、6 个具名 `BLOCKED:*`、10 个 `DECLARED_DEPENDENCY`；另有 9 个非必选格。这些覆盖值仍是 Root7a 账，新候选须重跑，不能直接转授。N22 现状：E7 已由产品内汇编合同闭合（`0764fa8861`）；E6 已有成员交叉核对；E2–E5、E9–E11 属声明的宿主/工具依赖；E8 的权重仍由 Python 发射器生成；E12 即外宿主。声明项并未被当作完整 N22 PASS。

**P0 完成边界与待决项：** 外宿主需要同候选真实工件及五宿主原生执行；当前不得 push、上传或派发，授权或真实宿主通道未到前只做本地准备。822 格是当前矩阵账，未经主人裁定不删必选格、不把依赖声明当作 N22 实证。词法三项按 C99 与产品实际输出继续核，具名缺陷不改贴 PASS。

**P1 纪元1候选（主人已在 A4 前暂停）：** A2 已封源，SID `df8f29fd`；A3 的 107/107 作业收据和 context 复验完成（`out/c43-cc-epoch1-build-01.md`）。A4 PC、A5 GEN、A6 准入／安装均未开始，不把 PREPARED 升格成候选。先解决 `.cx` 开发反馈墙钟，再决定沿用此 SID 或将已知产品修复合入下一纪元；cdx/cc/grk 均不擅自重开该构造闸。

**P1a `.cx` 提速第一优先（已交开发代码缓存）：** 本机无模块 `.cx` 默认约0.09s，`use netpack` 默认约5.45s、`use allsession` 默认约10.37s 且二次约10.55s；产品默认 `-run` 重复模块编译未复用。`5a0dd533f4` 将 `tools/dev-run` 改为按已装产品/包/源与模块闭包/选项/环境内容寻址的原生 ELF 代码缓存，`7e37405e21` 只排除12个精确会话变量，冷/热分别为 netpack 8.86/0.099s、allsession 17.05/0.133s；坏 ELF、产品/模块/源/选项/包变更均 miss。cdx 独立用它缓存 `tests/run-batch.cx` 控制器跑 `core-compat`：当前版冷19.77s、热7.72s，默认控制器15.57s；三次子批均真实执行16/0。仅 DEV_ONLY，不缓存测试结果，不把正式 SOURCE 批的受测默认 `-run` 改成 ELF。grk 的 std/模块路径工作仍限隔离原型；A3 身份不漂移。

**P2 下一产品源码纪元（cdx 串行收源、重建）：** 38 个需改产品格，加已知缺陷：E2 五项、E1 errno、`?:` 返回 struct、A-40 三诊断、自引用 struct 措辞、分相计时戳。新红例 `tests/pending/sizeof-global-member.cx`（`68c3adfb72`）：静态结构体数组的 `char op[24]` 得 `sizeof(rows[0].op)=8`，嵌套元素为24、整结构为336、整数组为29232；Root7a 默认 `-run` rc1。只读定位：参考 `src/front_parse.c` 的成员尺寸路径正确，产品走 `exec/parse2/control-result.tsv` 的 `SZ.expr` 通用分支，末端成员数组衰变后 `ELSZ` 给出指针宽 8；`((Row*)0)->op` 也红。修复须保留 `sizeof` 操作数最终成员的数组类型及维度、普通指针仍为8，并测静态／自动对象、数组索引、箭头、指针成员与嵌套成员。改 `exec/` 表会换 SID，纳入下一纪元一起重建。`lower` 的 ENOTDIR/EACCES 与 ENOENT 诊断未分开，输入域 29 断言中 2 红；修源码后必须重测，不删红项。

**P3 本机格逐行闭合（独立轻切片可并行，父行仍按完整域判）：** `9a44ce9869` 为 lower 登记四个 SOURCE 批，Root7a 的正式 run-batch 分别 51/0、778/0、1516/0、115/0，A-41 的 762 个本机布局输入已穷举；ABI 可观察格 144/0 但六项无 tape 拼写记 UNVERIFIED，输入拒绝 29/2 保持红，六目标／五外宿主／完整 lower 父行仍开。新候选必须重跑；其余宽行按矩阵逐格收据推进，不以局部绿替代完整域。

**P3a 六目标流材料及有限 oracle：** `61ba5c621c` 给 `stream-pack.cx` 增加六个 `image/O2` 目标的显式选择；默认 `lnx/x86_64` 七件与原材料逐字相同。PC04 包六路各生成七件，非法目标在建目录前 rc2。`611a8c62d3` 登记五个外国输出目标的本机 `stream6` 四夹具（ret、八寄存器、三重定位、稀疏数据），五批各 17/0、20 次流执行；`4ef850b1c7` 把材料目录改为 `run-batch` 显式输入，复跑 lnx/arm64 17/0。只授 Root7a 本机有限输出目标行为；新候选须重包、重钉、重跑，五外宿主原生与宽父行仍开。

**P4 `--all` 跨命令收据（B1b）：** `dabc5a0efb` 已提交 start → run → finish；run 相由执行器自身 waitpid 封存 bound rc，另钉末尾 RUN SHA。外层 `bound 58` 即硬限 60 秒；隔离根 12/12 控制、smoke PASS，聚合仍 rc3（132 批未跑、19 宽行未解）。尚须在最终同候选上执行完整分波，不能缓存 PASS。

**P5 本机全量：** 在最终候选上分波跑完全部批次，矩阵中本机格全部 COVERED_LOCAL。

**P6 外宿主（依 D1）：** 同一产物、同一身份在五宿主实跑，然后 `--all` 聚合。

**P7 发布：** 签名、Draft、Latest、公开另需授权，未授权不动。

**已提交的本轮辅助成果：** 外宿主 CI 草稿核 producer run／artifact 的 head SHA 互相一致（`b40487eadd` 后经 `5b4616387f` 修正，允许较新的验证提交；静态核，未派发）；B1b `dabc5a0efb`；lower 四批 `9a44ce9869`。先前 B1/E7/C1/S3/S5 收据保留。待办债：L1 草稿使用宿主 GCC，需改产品自身 `.cx`；lower 的 ABI 六项、输入 errno 与五目标路线仍未关。

**2026-10-27 cc 节点·宽合同已全量规格化：** 19 条 NOT_READY 宽行此前只有一行 oracle 文本、无法诚实关闭；现每行有 `tests/<row>-domain-matrix.tsv` 逐格规格（统一列 `host cell required status evidence requirement citation oracle note`；COVERED_LOCAL 仅限 Root7a 上实际 run-batch PASS 且恰断言该组，新候选须全部重跑）。提交 `e4d7a95bb8`（negative/optimization/formats/phase-timing/bootstrap/candidate 231 格）、`175a9642ff`（ri/resources/library/properties 212 格）、`97a3f4472a`（七流＋interfaces 365 格）；报告 `out/c43-cc-domain-specs-{contract,runtime,streams}-01.md` 给出约 70 个批量级切片。同提交修共享运行器：`np_run` 子进程自成进程组、超时杀整组、有无输出均计 1ms 拍；`run-batch` 自身超时记 `status=UNKNOWN` 退 3（非 PASS、需重跑），smoke/qprefix 复跑 PASS。纪元裁定：本纪元封源点＝流控制表 F1/F3 修复落地；A-40 三诊断缺口（触及 `exec/parse2`/`exec/pp` 位置传递）与产品内分相计时戳排入下一纪元，不挡 SID 重建。`@relocs` 槽值按参考 `relocate` 维持不设值域（C 允许 one-past-end），F3 只拒缺头/空码/静态 `@sym` 与指令数据引用越界。外宿主（push/派发/产物暴露）待主人授权。

**2026-10-27 cc 节点·本纪元产品源修复（下一步已由上段覆盖）：** 本纪元已提交四处真实产品缺陷修复，均为具名拒绝而非放宽：`5fd85c0130` 库 late-prepare 吞 selection panic；`9df74fd151` `as -b` 与 `# unisacc -S` 头冲突时静默产出异 ISA 对象（开发 ELF 复验：冲突 rc1 无产物，无 `-b` 仍由头定目标）；`48b5be2a4c` `attr_strip` 未核 USLATTR1 token_len（f1sidecarcheck 钉其等于记录后字节）及 image 路由缺 elf 阶段时把 lower 文本写成映像（开发 ELF＋PC04：改名 elf 行 rc2 `run: unknown package route or stage boundary` 无产物，干净 image/-run/ctor 不变）。`ebe891507c` 登记 negative-diagnostics 193/0、negative-model-loader 99/0、negative-model-qprefix 4/0（Root7a 正式 PASS），negative-model-loader-terminal 按设计红（待新候选）；严格清单 133 批／144 覆盖／19 未解。E3 `eof` 后尾随与 ELF 阶段缺头/越界数据写崩溃映像两缺陷属模型控制表（`exec/parse2`、`exec/enc`），按设计在表中修并重建受影响模型（`out/c43-cc-stream-findings-design-01.md`）。E7 按主人裁定改为产品内两 ISA 汇编/链接字节合同：产品自带 `unisacc as` 已对 22 个 `.s` 逐字重现 GEN-C 对象；规范 oracle 须逐指令手册引用、重定位、对象边界、拒绝恢复与 11 单元覆盖分立，GCC 重演只作 Linux 本机诊断（`out/c43-cc-kernel-hostcc-probe-01/`，不登记、不作六宿主前置）。ctx05（SID9147）已因后续源修复作废并停跑（12 作业已执行，tool-pack 缺 zlib 头 rc2，被打断作业 UNKNOWN），待本纪元修复齐后一次性新建 context/PC/GEN 并核 Root。interfaces-lnx-x86_64 批（271 断言）待流修复后登记。禁签/Draft/Latest/公开/push。

**2026-10-27 cc 主刀接管节点（其“下一步”已由上段覆盖）：** cdx 已把 0.0.43 /goal 交 cc（Opus）主刀，cdx/CSR 停写主树。`5fd85c0130` 修复库产品真实缺陷：`us_late_prepare`（`exec/c/librarylatebind.h`）与合同查询（`exec/c/libunisacc.c`）在第一路由成功后 `rc` 已为 0，setjmp 失败分支又无 `else rc=1`，故 selection 模型装载 panic（如 M31 正文损坏）被报告为成功，随后误报 `library selection: missing-owner`；现与 `us_compile` 一样失败并保留具名原因。结构事实：已装 APE 源（asmcompiler→compiler/bind）**不存在** `*/nativeabi`、`library-contract`、`library-selection` 路由，M28/M30/M31 唯一消费者是 `libunisacc`，“APE 全32路消费”在结构上不可达，只能在库产品登记。新正式 SOURCE 批 `p3-library-members`（GCC11+系统 libffi 编公开 API 夹具 `tests/cases/p3_library_members.c`）对 PC04 包、未引用对照成员 M2 及 M28/M30/M31 单字节翻转逐格精确比对：M30 三相、M31 compile/carrier、M28 union 导出 carrier 均具名 `invalid compressed model`，登记运行器 PASS 27/0（`out/c43-cc-p3-library-members-registry-01.log`）；修复前私有副本 m31 两格失败（`out/c43-cc-p3-library-members-negative-01/run.log`）。严格 `--list` 129 批／140 覆盖／19 未解。**`exec/` 在 SID 内，源码身份已变：** GCC11 新编 `seed/ident.c` 实测 HEAD SID `9147bd1346795cd2d81cf3dfdd256849b5b33aada5b63f1b1606bbbb86ed3dac`（`out/c43-cc-ident-01/`）；已装 Root7a 产品未重建，SID4fe 的 ctx04/PC04/hostseed/七流等收据只作上一源码基线，不转授新 SID，绑定活 SID 的批将按设计拒绝，直至新 context/PC/代际重建。三份只读分析在 `out/c43-cc-{wide-gap-matrix,n22-gap,foreign-host-readiness}-01.md`：19 宽行本机无一能诚实翻绿（多数缺行规格、需逐宿主批，`tests/run.cx:33` 的 `--all` 写死 rc3）；N22 最大实缺为 ASM 内核字节全由上代产品产出（E7），GCC 构建汇编器/链接器独立重演为首选刀；外宿主连授权派发也缺 producer 产物路径与各宿主格（仓库公开，产物暴露、push/派发需主人授权）。下一步：攒齐本纪元产品源修复后一次性重建 SID9147 的 context/PC/GEN 并核 Root；并行推进 E7 独立重演与 interfaces/negative 本机格；禁签/Draft/Latest/公开/push。

**2026-10-27 本轮新证据：** 未改产品源的 native-first 控制器由本机 GCC11 直接编成 `out/c43-hostseed-nativefirst-hostcc-direct-02.elf`，不经已装 APE 启动构造控制器；新 context01 下 HOST→三代五切片与 wrap 均逐命令 bounded 完成，三代 APE 与 Root7a 逐字同 SHA `7a0cf28b…`，直接 `--verify-chain` VERIFIED，默认产品链批 41/0、活源绑定 58/0（`out/c43-hostseed-nativefirst-hostcc-{chain,bind}-test-01.log`）。另由宿主 GCC 新编 prior helper，与旧件同 SHA `72c2ddb9…`，context02 与 HOST 已 PREPARED，但未在 context02 重建三代；context01 仍借 ctx04 的旧 helper。CSR 已启用 `/goal` 并在独立域交 P3 NN/ASM 溯源正式批、同候选本机宿主格及 stream.e2 Linux 金表格；P3 批经双方核对主要重做 PC04/kind2 材料绑定，不足以改写 hostseed `payload-trust`。私有探针从 Root7a footer 取到 32 个去重 M（34 阶段引用），32/32 解压正文与 `binary_encode(compact_q(PC04 模型及 ctx04 net-job))` 相等；M3 在 -E、M13/M29 在普通编译翻字节具名拒绝，互换为未用成员则同路线仍通，差分负控 8/0。其余五个产品入口仍 NOT_RUN。该交叉须先转成独立 `.cx` 正式批，不授完整 N22。本机同候选六项输入已在 commit 38b32f 快照重算并落 `out/c43-foreign-host-inputs-local-20261027.tsv`；producer run/artifact、远端可检出 commit 和五外宿主真机仍缺。19 宽合同和 `--all` rc3 不变。

**2026-10-27 properties 并行有限叶：** 新 `tests/pending/properties.cx` 已于 `0334cfb2e8` 提交；本机 Root7a 在单次 outer55 下 7.59 秒通过 41/0（`out/c43-full-properties-local-04.log`）。测叶含循环边界 0／1／255／256 的字面金值、前后向和 O0／O2／重编的输出对照、精确 rc1 语法拒绝与无产物、运行参数拒绝及恢复、产品／源码摘要不变。完整语言、stream/net/RI、ABI、内存生命周期域及五外宿主仍缺；`full-properties` 保持 PLANNED、`properties` 保持 NOT_READY，不以有限绿批改父行。

**2026-10-27 P3 独立 CX 对照：** 现有 `exec/c/qprefixcheck.cx` 对 Root7a 整包在 outer55 内 rc0，实际读 32 个已压紧模型，Q 展开与运行结果相同，八类畸形前缀拒绝；日志 `out/c43-root7a-qprefix-artifact-01.log`，stderr 空、产品 SHA 不变。这与 CSR 私有 32/32 成员交叉是不同读取器的补充证据，但它自身不核 ctx04 job、不关 N22 或外宿主。

**2026-10-27 P3 包成员正式交叉：** `a598fb6bfb` 关闭首版预编 ELF 假标记洞；`b9c65f4836` 利用现成 `-dump-tokens` 触发 M24/M25 损坏具名拒绝。`8d5e023d04` 在同一正式批里对当前 PC04 配方的 32 个成员逐一重算：ctx04 `net-*` 作业文本等于 PC 模型，且 `inflate(M)` 与 `networkformat.encode(netpack.compact_q(job.result.net))` **逐字相等**。Root7a/ctx04/PC04 登记批外层 11/0、内部 88/0（`out/c43-p3-member-all-registry-02.log`）；私有负控把 M28 作业与 PC 文本同步改一字节，文本门仍过、`member-binary-28` 失败（`out/c43-p3-member-all-negative-01/run.log`）。假 ELF 仍具名拒绝。分账为 34 阶段引用、32 包成员、27 项早期探索 ICM、正式动态 M3/M13/M24/M25 四项；M28 nativeabi、M30 librarycontract、M31 librarypartition 的产品运行入口仍 NOT_RUN。`--list` 128 批／139 覆盖／19 未解；不改 hostseed `payload-trust`，不授全 32 路运行消费、完整 N22 或外宿主。

**2026-10-27 native-first wrap-link 与活源端点：** 工具已于 `e3076abd21` 提交，SHA `101ffcae…`；grk 将四条 hostseed 路径交由主线单写。并发移动造成的旧 context01/02 继续记 UNKNOWN。最终源码下新 context03 的 HOST→三代 18 步均在各自 outer55 内 PREPARED，三代 APE=Root7a；三份 trace 各有唯一 `calling init` 从本代 `wrap-link/libz.so` 装入，SHA 与钉定 libz 内容进入 artifact-v2。默认产品 `--verify-chain` VERIFIED，正式 `hostseed-nativefirst-p3` SOURCE 批 41/0（`out/c43-hostseed-nativefirst-libz-{verify,batch}-03.log`），csr 只读复核 ACCEPT。现行 `hostseed-source-bind` 已改为同窗先核 native-first 链、后核活 ctx04/PC04 的 107 作业／34 模型，显式核两 context 的 SID、HOST、helper、CC 版本、三代切片／APE及收据和 loader trace 前后内容观察，最终正式批 58/0（`out/c43-hostseed-nativefirst-live-bind-batch-08.log`）；错混旧代 stage 具名 `receipt-bytes-mismatch` 拒绝，错 SID 具名 `host-and-source-context-SID-mismatch` 拒绝。此轮收住本机 libz 实际装载路径及新链的活源端点观察；`-lz` 名转发、宿主 CC/头/loader、同 UID 竞争、历史执行、P3 NN/ASM 二进制信任、完整 N22 与五外宿主仍开。19 宽合同和 `--all` rc3 不变，禁签/Draft/Latest/公开。

**2026-10-27 收口调度：** 开发完成度粗估约 72%，不按有限 PASS 批数量上调。真正待关是 19 条宽覆盖、完整 N22 和同候选五外宿主；`--all` 必须维持 rc3 直至实际执行。主线以现行 SID4fe/Root7a/ctx04/PC04 为基线，串行负责产品来源闭包与正式准入；任何产品源码改动必须新 SID、新 context/PC/代际收据。CSR 的外部 harness 消息拥塞后，主线已直接接手并提交 P3 成员正式门；协作者暂存文件各自保留，不依赖消息排队作资格。后续可并行只读审查或独立文件域，热文件/PC/闸继续串行。外宿主仍需 Linux arm64、macOS 双架构、Windows x86_64 真机；WinARM 当前只执行 x86_64 PE 模拟，ARM 原生另需产品能力。七流／接口、RI／资源、优化、库等按域矩阵关父合同，不以新增有限绿行替代。顺序见 `plans/v0.0.43.md`；禁签/Draft/Latest/公开。

**2026-10-27 当前 native-first 宿主种子节点：** SID4fe／Root7a 的新 `hostseed-ident-v3` context03 在编译 fresh helper 前，先用内容钉住的 prior native helper 测全树来源；编后 fresh native、已装 APE、prior native 三方同 SID，并在后续 context check 重测。HOST→三代共 18 条独立切片／真 deflate wrap 命令均于各自 outer55 内收条；三代 APE 逐字等于 Root7a。默认产品路径 `--verify-chain` 已返回 VERIFIED；新正式 SOURCE 批 `hostseed-nativefirst-p3` 41/0（`out/c43-hostseed-nativefirst-registry-01.log`）。第三代慢步骤使用同源码 `-O2` 本机 ELF 代码缓存执行，未缓存 PASS；旧 v2/context02 和两次第三代超时部分产物不得记资格。严格清单 124 批／135 覆盖／19 宽缺口，`--all` 仍 rc3、子批0。这仅收紧本机初测顺序与固定 P3 链；prior helper 自身诞生、已装产品编排、宿主 CC/libz、P3 NN/ASM 二进制输入、五外宿主与完整 N22 仍开。下一步查先验 helper 来源与完整 N22/外宿主，禁签/Draft/Latest/公开。

**2026-10-27 本代独立身份算法对照：** SID4fe／Root7a 的 ctx04 `source-job-union` 经正式单批重验 37/0。新 `hostseed-independent-ident` 独立批用 hostseed 冻结的 GCC11 从 `seed/ident.c` 另编 helper，helper 字节与冻结件相同，GCC 版本字节亦同冻结收据；批内先用原生 helper 测来源，再调用产品身份路径，两者及 ctx 来源均为 SID4fe。正式批 14/0、stderr 空（`out/c43-classic18-source-union-batch-01/`、`out/c43-hostseed-independent-ident-batch-05/`）。严格清单 123 批／134 覆盖／19 宽缺口。**批本身由已装产品启动**，因此只证明批内首次身份计算由独立 helper 完成；历史 hostseed 初测顺序、宿主编译器／系统库来源、P3 NN/ASM 输入、完整 N22、外宿主仍开。下一步优先同候选外国原生观察和 P3 输入溯源；禁签/Draft/Latest/公开。

**2026-10-27 本代七流复验：** SID4fe／Root7a 以当前 PC04 同字节 P3 包，复用上期 e252 制备且七容器摘要未变的流材料，在八个独立 outer55 正式批重跑：整链 40/0；E2/E1/E3/E4/prune/lower/ELF 分别 36/25/17/17/19/12/11，八份子 stderr 空（`out/c43-classic18-stream-batches-01/`）。这是当前产品对有限流合同的实际消费，不是本代重新构造流材料；七条完整金表域、RI、六宿主仍开。下一刀仍是完整 N22 的 NN/ASM 来源与同候选外宿主；禁签/Draft/Latest/公开。

**2026-10-27 P3→hostseed 当前端点绑定：** 当前 SID4fe/Root7a 下，PC04 钉住的 native tool-pack 从原输入在新目录重封 1,595,409 B 包，逐字节等于 PC04；活 ctx04/PC04 `--verify` 和三代 hostseed `--verify-chain` 均 rc0、stderr 空（`out/c43-classic18-p3-pack-replay-01/`）。csr 的私有一字节负例证明单独 hostseed 链不看活 net job／PC 模型。新独立正式批 `hostseed-source-bind` 要求 payload 路径为活 PC04 包，在同一 outer55 内先验三代链、再全验活 ctx04/PC04 的 107 作业／34 模型，14/0、stderr 空；错包路径 rc2 且 stdout 空（`out/c43-hostseed-source-bound-formal-01/`）。原 41 断言批保留；严格清单现 122 批／133 覆盖，19 个宽合同仍未关闭。此为单写者前提下的**当前端点**绑定，不追证历史源码执行；经典18 MODEL 尚未进 P3、完整 N22／外国宿主仍开。禁签/Draft/Latest/公开。

**2026-10-27 最新行动（覆盖下方旧下一刀）：** SID4fe5a4f1/Root7a/e2ab9233 的本机同源两代 P3 固定点已核；当前已装产品实际运行全部 24 条零材料 IMPLEMENTED 批，24 个独立 `BATCH_RUN PASS`、子 stderr 空。`out/c43-hostseed-classic18-context-01/` 测得 SID4fe，GCC11 构造 HOST，HOST→三代各自五片加真 deflate wrap 的 18 个命令均在 outer55 内完成；三代 APE 与 Root7a 逐字节相同。`--verify-chain` 通过，正式 `hostseed-fixed-p3` 41/0，八个具名拒绝控制批各自 PASS。本轮又从已选经典十八网的活 `kernel/` 生成并复验材料；正式 `nets` 在当前 Root 上通过全18域、8770键、22694决策、28868字段拒绝及恢复、72适配拒绝，`net-controls` 3/0，日志在 `out/c43-classic18-net-batches-02/`，stderr 空。此为**当前 SID 的本机固定 P3 宿主种子链与 deploy-kernel 行为**；经典十八网 MODEL 尚未进入 P3 产品包，链仍信任 P3 NN/ASM 二进制输入与 Root 辅助初测，宿主 CC/libz 非 hermetic、外国原生宿主未跑；完整 N22、RI/流域等 19 宽合同仍开，`--all` 仍应 rc3。下一步直攻 P3 输入溯源、完整合同和同候选外宿主；禁签/Draft/Latest/公开。

**2026-10-27 同源 ASM/身份补证：** GEN-C 的本代材料又经正式 `run-batch` 分三窗实跑：`assembler` 189/0、`assembler-cache` 篡改拒绝 2/0、`identity` 15/0，日志与空 stderr 均在 `out/c43-classic18-asm-batches-01/`。此前 e252/GEN29 的同类结果不再充当前 SID。现有 `source-package-pack` seal 已验 107 job、34 组模型副本，kind2 准入已核 ASM 源/对象收据；新只读门应针对真正缺的包内 M 正文与来源绑定，不重复旧收据。完整 N22/外宿主仍开。

**〔历史快照，非现行〕2026-10-27 SID4fe 期补记（覆盖下方两段的下一动作）：** 已装开发根仍为 SID `4fe5a4f1…`、产品 `7a0cf28b…`、record `e2ab9233…`。换源后新建 `out/c43-source-classic18-context-04/`，冻结当前 root record 并完成全部 107 个正式作业；PC02/03/04 各自 34 模型、六路、declare、pack、seal、verify 均完成且包逐字节同。GEN-B/PC03 与 GEN-C/PC04 各自完成双 ISA 核、五切片、真 deflate APE，核／切片／整包逐字相同且同已装根；两端分别通过 kind2 准入（107/34）、smoke 16/0。旧 context03/PC01 在新 live record 下拒绝，旧 GEN-A 收据与新 PC02 配对也拒绝，均保留为预期负例。这只关闭本机同源 P3 两代固定点；经典18 deploy MODEL 未进入 P3，完整 N22、NN/ASM 二进制输入、宿主 CC/libz、RI/流域及五外宿主仍开放。下一刀对准完整合同与同候选外宿主，不签名、不 Draft/Latest/公开。

**〔历史快照，非现行〕2026-10-27 SID4fe 节点（覆盖下方 e252 开头的旧节点）：** 当前工作 SID `4fe5a4f101a4960adf8d160937c413a05c60e05951921b288b877f6cbf2ca541`；已装开发产品仍为 `7a0cf28bba67f2394abaf783b886baeafe0687f4590c392dbd8611212a0f5f77`、2,954,593 B，build record 已更新为新 SID（SHA `e2ab9233…`）。经典十八网的私有构造材料经 18 次 verify、22,694/0 自测及 evidence replay 后写入 `weights/built.json` 并由 legacy emitter 再生 `kernel/`；这套 deploy MODEL 尚未进入产品 P3 包，不能把产品固定点当作新网产品功能 PASS。新 SID 的双 ISA 核、source context03 的 107 作业、full PC01 的 34 模型／六路／封包／seal 均已实跑核验；同源 `-O2` ELF 使 `tbl-shared-e3 finish-job` 在 55 秒内完成，不吸 memo。GEN `out/c43-native-kernel-classic18-01/` 的五切片和真 deflate APE 已完成 kind2 独立准入（107/34），整包逐字节等于旧 Root7a；开发安装成功，安装后 smoke 16/0。旧 context03 冻结的是安装前 e252 root record，不能跨新 SID 转授；下一代须新建 source context。完整 RI、流域、N22、同候选五外宿主仍开放，`--all` 不能记 PASS；不签名、不 Draft/Latest/公开。

**〔历史快照，非现行；现行身份见 §思维树 cc 计划树与记忆宫殿首行〕2026-10-27 早期节点：** 工作 SID `e252e3fe9970e396649f554d1caa625246fea0edd3beec51ab746f3e12f6f0eb`，已装开发 Root `7a0cf28bba67f2394abaf783b886baeafe0687f4590c392dbd8611212a0f5f77`（2,954,593 B）。新 Root 的 ctx01 已冻结当前 epoch，49/49 作业有收据；GEN28/PC01 与 GEN29/PC02 分别完成双 kernel、五切片及真 deflate APE，包和产品逐字节相同。GEN28 两段式开发安装后，ctx01/PC01 复验仍绿；mixed 五个正式批已 PASS（比较64/0）。当前 smoke16/0、formats14/0、formats-objects21/0、对象材料九批198/0、identity15/0、GEN29 assembler189/0、image RI 单TU29/0、多TU15/0、错路由10/0、SC2 image PEM17/0；严格清单121批/132覆盖/19缺口；固定P3宿主种子链41/0、八拒绝批71/0，`--all`仍 rc3、子批0。开发专用 `tools/dev-run` 已有独立命令热缓存实测，代码缓存不授产品资格。下一刀是完整 RI/流域、N22 和同候选外国宿主，旧 P75 结果不转授；不签/Draft/Latest/公开。以下 P75 段为上期状态，以本段和记忆宫殿首行为准。

**当前 Root 七流实跑补证：** 用 Root7a 重建流运行体并从当前包打出 `out/c43-streams-riroot-01/`；七流链40/0，独立 E2/E1/E3/E4/prune/lower/ELF 分别36/25/17/17/19/12/11断言、均0失败。七个测试材料钉已换为本代容器实际 SHA，旧 E2 钉的首轮拒绝留作代际证据。收据在 `out/c43-riroot-stream-batches-{01,02}/`；SID 复核仍 e252。只授本机有限流用例，不关闭七个完整流或六宿主 interfaces 合同。

**当前 Root 零材料批：** 清单中19个 IMPLEMENTED、`materials=-` 批已各自实际执行并在 `out/c43-riroot-zero-batches-01/` 留 `BATCH_RUN PASS`，收据均钉 Root7a。同期 `out/c43-riroot-all-afterstream-01.log` 的 `--all` 实际 rc3，115批与92个已实现覆盖行 NOT_RUN、子进程批0、19完整缺口仍在；单批收据不回填聚合。

**当前 Root 既有材料实跑：** `out/c43-riroot-material-02/` 下 `nets` 全18域 8770键/22694判定、28868字段拒绝与恢复、72适配拒绝实跑 PASS；`libneed-attributes` 12/0。旧网络与扫描器材料只经本候选实际消费，不称 SID e252 新构造。

**当前 Root source-job union：** 注册批已修正材料 SID 文件入口和执行模块源身份核验，在 ctx01 实跑37/0（`out/c43-riroot-source-job-batch-01/`）；旧硬钉直接调用的三项失败原样保留。该有限材料并集不是完整包、NN 推理或候选资格。新清单 `--list` 仍有效，产品 SID 不变。

**当前 Root RI image：** CSR 单／多 TU 私有探针已改接已装产品并由主树收为两条 SOURCE 批；`out/c43-riroot-ri-image-batches-01/` 分别29/0、15/0，含 source:image 前闸、E3 实际 tokens/ATTRS 副身份、严格封存回放、改/删活头仍同镜像、改源／篡改／缺副收据拒绝。清单118批/129覆盖/19缺口；`out/c43-riroot-all-riimage-01.log` 实际 rc3，117批与94个已实现覆盖行 NOT_RUN、子批0。仍不授完整 RI、外国宿主或发布资格。

**当前 Root mixed 固定点正式批：** GEN28/PC01、GEN29/PC02 的四个独立 `selfhost-mixed-*` 端点在 `out/c43-riroot-selfhost-registry-01/` 均实际 `BATCH_RUN PASS`；比较批以这四份同 run-id 日志在 `out/c43-riroot-selfhost-registry-02/compare.log` 实际64/0。首轮错传旧 run-id 日志的比较4项失败保留在 `...registry-01/compare.log`，不改写为绿。此为同候选本机 mixed 两代固定点，非宿主种子 N22／外国宿主资格。

**当前 Root 对象格式有限批：** `formats-objects` 直接经已装 Root7a 编 Linux x86_64/arm64 ET_REL，核 section/symbol/RELA 有限界，链接 x86_64 ET_EXEC 并原生返回7；四个未支持目标与混目标链接均具名拒绝且无输出。`out/c43-riroot-formats-objects-batch-01/` 实际21/0；`--list` 119批/130覆盖/19完整缺口。arm64 原生、Mach-O/COFF 单元对象与完整 formats 仍未授资格。

**当前 Root 九条对象材料批：** 上期 MAT02 在 Root7a 下具名拒绝；新 `out/c43-riroot-object-material-01/` 由已装产品准备并验证。`out/c43-riroot-objects-batches-01/` 九条 `objects-*` 正式批各自复核材料并全部 PASS，合计198/0（链接返回7、混目标／定义／版本和四未支持目标拒绝）。材料准备不替代功能执行；外国宿主与完整 formats 仍开放。

**当前 Root RI 错路由拒绝：** `ri-route-refuse` 正式批在 `out/c43-riroot-ri-route-refuse-batch-01/` 实际10/0：tape 捕获正端；object/run 捕获在账本／输出前精确拒绝；有效 tape 账本也不能授权 object 严格回放。清单120批/131覆盖/19完整缺口；`out/c43-riroot-all-riroute-01.log` 实际 rc3、119批与96个实现覆盖行 NOT_RUN、子批0。仍不授完整 RI。

**当前 Root 构造支持正式批：** GEN29 材料经 `out/c43-riroot-identity-assembler-batches-01/` 的 `run-batch` 实际消费：identity15/0、assembler189/0、assembler-cache 私有单字节篡改拒绝2/0（生成任何对象前拒绝）。三条均钉已装 Root7a，不把有限构造／ISA 合同升格为完整 N22 或外国宿主。

**当前 Root 宿主种子三代：** 新 `out/c43-hostseed-riroot-zlib-context-01/` 用本代 SID e252 和已装 Root7a 测源并由 GCC11 构造 HOST；HOST→第一代→第二代→第三代各自实际跑五片与自运行 zlib 封装，18 个步骤均独立 outer55 返回0。三代整包都是 2,954,593 B、SHA `7a0cf28b…`，五切片与 GEN29 同字节；正式 `hostseed-fixed-p3` 41/0、八项拒绝批合计71/0（`out/c43-hostseed-riroot-zlib-{registry,control}-01*`）。旧 stored 封装 5,316,769 B 与根不等，保留作差异证据。此链仍信任既有 NN/ASM 二进制输入，宿主 GCC/libz 非 hermetic，初始测源借已装 Root，六宿主未齐，故仅授本机固定 P3 三代字节固定点，不关闭完整 N22。

**2026-10-27 当前树（覆盖本节下方 Source03a5 历史快照）：** 工作源码 SID `718017ab98ae13963d0b928496724c0e661990b3972ba452909742ccfa6f6a81`；开发 Root `75a52ae7f898a6db7ea32b810f250deb9ffda9a2fad2b1d8a6ec81efba4f8972`、2,934,337 B、版本0.0.43，真 deflate APE。ctx03 冻结 root-epoch 记录、49/49 作业正式收据；PC04/PC05 与 GEN26/GEN27 已各自构造和验证，两代 44 个 ASM/对象、双 kernel、五片及整包逐字节相同。双阶段开发安装 rc0，安装换活记录后 ctx03/PC04 复验 rc0。四个 mixed-candidate 端点及比较现已登记为五个独立批次，并在 `out/c43-selfhost-mixed-registry-01/` 实跑五个 PASS、比较 64/0；源码驱动协调器最初两次外层超时仍记 UNKNOWN，实际 PASS 使用测得热点的私有 native 代码缓存，未放宽单命令 55 秒。P75 上独立 E2/E1/E3/prune 有限 oracle 在新清单分别实跑 36/0、25/0、17/0、19/0；四个完整流合同仍 NOT_READY。Root smoke16/0、18零材料批、相位外墙钟5/0及部分有材料批另有收据；严格清单现 113 批、124 覆盖、19 缺口，`--all` 仍 rc3/子批执行0。这是本机混合包两代固定点，不是完整 N22 或外国宿主资格。下一刀：补19完整合同与同候选外国宿主证据；不签名、不 Draft/Latest/公开。以下以 Source03a5、P53 为“当前”的旧树及长叙事是未整理的历史，不得作为现行身份或资格；以本段和本轮原始收据为准。

**开发反馈波次：** `tools/zero-wave.cx` 在 P75 下以一条 outer55 命令并行执行四个独立零材料批次，smoke/core-compat/library-abi/owned 各自经原 `run-batch.cx` 实际 PASS、独占日志和输出叶，产品摘要前后不变；重复 ID 在派生前拒绝，缺材料不能被合计成 PASS。它只缩短合格开发批的反馈，不改变 `--all` 的 rc3、19 个完整缺口或发布资格。

**RI 产品路由缺口：** P75 安装产品对有效 tape SC2 走严格 image/O0 仍 rc2 `resource-route-unsupported`。C44 私有 B 原型的 `RF_CAPTURE_HOLD` 曾让 e2/e1 资源读取不入捕获；`out/c43-ri-b-front-01/` 实际收据没有 include 头行。CSR 私有 B2 已改成 source:image 前闸、e2/e1 资源事件及 E3 tokens+attrs 第二身份；副身份作为有序 SC2 行封存，NUL key 探针改为显式长度，单 TU 27/0（`unisacc-c44-opt/out/c44-ri-image-B2-02.log`）。主树 `exec/c` 未吸纳。多 TU 首次补闸后在 `out/c43-ri-b3-multitu-01/` 实跑 rc1 `tape domain: unknown opcode`；包声明表明须先过 `units` 再核 E3 实际输入。B2/B3 私有稿还有严格回放读取活宿主头的错误接线，须撤。严格 SC2 回放应只用收据封存的头字节，活头改动后同输出属于正确行为，篡改收据或 E3 身份才应拒绝；不得以私有有限绿宣称完整 RI。

**外国宿主候选准备：** 本地 CI 草稿的 Windows 分支现编排直接启动原始 PE 的 `--version` 单进程观察，并在前后核产品摘要；`actionlint` 与本地 PowerShell 解析绿。没有 Windows runner 实跑，功能批、来源闭包、JobObject 后代清理与原生路径适配仍 NOT_RUN，不算六宿主资格。

**目标：开发unisacc.com 0.0.43直到完成。** 日常直接用unisacc.com＋.cx迭代；源码身份、bootstrap身份和同候选资格分别记账。新候选未构建前，不把旧产品局部结果转授给新源码。发布仍须明确授权。

```text
0.0.43：完成开发、源码构造闭环与新验收（发布须另授）
├─ 当前主刀：工作源码已变更，SOURCE_FIXED/PRODUCT_NOT_REBUILT；上一冻结工作SID0d5923...已fresh核0；新forward-result表SID03a5...已fresh核0；scanner12/头oracle及导出prepare/verify/install均0，cf2270仅中间header身份；Root仍P53/record21704/Source329；GEN15仅PREPARED，origin37/0、NI104/0、采用单端永久合同15/0；采用包prepare/pack/seal/verify/snapshot材料均0，union新SOURCE批strict-list实际100批/115覆盖且执行0（此前99批/114覆盖保留历史）；S03a5 partialCTX/三工具及三NN材料已准备，union probe及永久路径各原55实际36/0，全部34gen实际0且31JSON逐字same/3diff，生产者两net外层142仍UNKNOWN；RB3与混合采用schema/installer及新候选待完成，未新slice
├─ R0 已归档：旧1333队列停止；Source447当前段/记忆原文另存archive，原红/UNKNOWN/局部绿不改写
├─ R1 待新候选：P53的smoke/RI/库/属性/优化等仍是旧候选证据，不能自动授给变更中的工作源
├─ R2 新P3生产链：CTX03全部107已执行，106项outer55 rc0/PREPARED；net-warnparse outer142/native0缺终态，仍不计完成
│  └─ gen13九phase、当前dd6/407的PC04全材料流水与full-SP双端0，新pin gate67/0；材料仅PREPARED/VERIFIED；旧ctx候选A已归档；当前B新ctx重建/smoke16/candidate58/schema79全0，installer实际outer55 rc0/INSTALLED development-only，Root329记录已独立核0；GEN13/14新ctx重wrap全0，Source329原30 selfhost03实际58/0仅保留基线；fclose工作头-I include338/0，新产品未构造
├─ R3 已通过Source329历史机制：C数学核三处unsigned修复，经独立CX exporter再生两C，39个NN输入字节/provenance不变
│  └─ 保存P53执行18网8770键/22694判定、28868字段拒绝及恢复、72适配拒绝全0；宿主shiftSan22694/0；永久words9/0、all-nets8/0
├─ R4 范围保留：经典18数学机制仍复用旧NN数值；当前B仅version/smoke16/candidate58/schema79实证，旧RI/P3消费者不转授新ctx，Windows/完整预算仍待
├─ R5 待新来源链：旧11/12固定点58、CTX02固定P3链41/拒绝71均属Source447；Source329候选A/包及当前B已构造，新ctx58/79已实跑，本机原30两代selfhost03已58/0；完整N22和外宿主仍待
├─ R6 pp局部来源：Source447低mask域构造92/配方拒绝23有效但历史；不计Source329重验或全18参数源码重建
├─ R7 19完整缺口、完整NN/ASM源码构造及同候选跨平台资格仍未收口；签名/Draft/Latest/公开仍禁
└─ C44旁路：已审修 live record 入 witness 洞——sp_load_ephemeral 严读活记录不入 sp_files；witness 仅冻副本全文SHA+artifact/source投影。补丁 C44 out/.../fix-live-ephemeral-vs-main.diff（lib->6d0a4296…）；origin 107仍绿。
```

**当前源码与已安装Root分离：** 当前最终工作SID为0d59237f3250b59514a55ff529c77b665033e5feb71061f6fcaded0c7ef925c0，已含scanner精确修复及工具正规头再生并fresh helper核0；cf2270b2e103ead1d13859489a34d6eb9e57a127eb0bef5f347b17822cb6c16e仅保留header-only中间实测身份；当前无新源码候选，以下Source329构造/安装/58证据保留为基线，不转授工作源。旧Source329cdb7077891074f638634dc7abdcd59ee39c368ad6ba1c2fcf867c3c63de2e下，完整候选A已由该源码实际构造并归档，当前新tool context候选B已完整重建并获smoke16/candidate58/schema79实证，B已实际本地安装、本机原30两代selfhost03已58/0；完整N22和外宿主仍待。根unisacc.com现是P53e2cd1f669c92fda2c16dd93a9021cd4db97f05c57ab2d1acf9a835cbac9857（4911193B）/Source329，当前record21704e...由真实安装生成；旧S447/source记录完整保留在GEN13/bootstrap-before-install.com及out/c43-u64-checkpoint-01/bootstrap.com。完整重构与实际安装均有证据，旧epoch与旧资格不改贴为Source329。

**修复与生成物来源：** 原KERNEL_BODY的signed long拼装能遇到合法parse mask bit63。旧宿主probe编译0/run1，out/c43-mask64-highbit-host-run-01.stderr保留实际signed-left-shift UB；旧默认-run位型匹配只记旧行为。修复的是unisa/ckernel.py中mask64返回类型、累加器v和接收lit三处unsigned。tools/table-kernel-export.cx只抽取实际raw C数学体，精确生成core/main前缀及self全文，未执行Python、未重构NN数值；prepare/verify/install各原55秒实际0。两次逐文件原子rename不称整对原子，installed.tsv记录真实前后S，不造candidate。39个model/cases/header/weight/dense依赖的旧bytes和producer provenance保留。当前生成core SHA=c9219cd3e201b1e1b65a4296d9375d44eb766ce5a6c415a951de27a86c31dcbd，self SHA=00bd6d26d639f7c210ee9f99526955bd46dfab27073f5dbbcdd2c561168e2989；收据out/c43-table-kernel-math-01/installed.tsv。旧package-refresh的完整构造输入比较不能因此被删除、跳过或泛化兼容；下一包必须有自己的源码构造与验证闭环。

**Source329已实际验证的机制：** 保存P53对新数学核的18网批次out/c43-nets-u64-bootstrap-all-01.log完成18模型/8770键/22694个真实infer判定，28868字段边界拒绝及等量恢复、72适配拒绝均0，词表/标签/唯一最大logit照核；材料仍复用旧NN参数，不是18网从gold重新构造。宿主C99/O2/shift sanitizer编译并执行当前kernel/unisa_self.c，out/c43-table-kernel-u64-host-run-01.stdout精确22694 decisions, 0 wrong，无san诊断。固定probe的bootstrap位型和宿主san运行也实际0。永久tests/table-mask.cx两独立模式原55秒words9/0、all-nets8/0（out/c43-table-mask-{words,all-nets}-01.log），函数从实际core原样抽取而非自写decoder；前后source/全部列明依赖/CC/root/保存bootstrap身份均核。宿主系统stdio、CC子工具和sanitizer/libc仍是声明Cfoundation依赖，不称hermetic或新Source329产品资格。

**Source329基线P3生产准备与逐job执行（不转授工作源）：** tools/source-package.cx已固定为9c2e71ef2094773bcada4c27a39871f3d0f5947555eb3ebe338aaddf203427ba，raw-environ逐次移除SEED_GEN_GENERIC、SEED_GEN_TRACE、UNISACC_SEED_TRACE、UNISACC_RF_CAPTURE、UNISA_ATTRIBUTES五键，并将固定environment-policy写入context与job收据；按存在性变支及空字符串绕过的旧缺口已处理，不再是待修阻塞。Source329、保存的P53 bootstrap和宿主CC仍分列身份；新CTX02 prepare成功，native runner02构建成功，外部SEED_GEN_GENERIC=1、SEED_GEN_TRACE=1、UNISACC_SEED_TRACE=1条件下--verify-context实际VERIFIED/rc0，见out/c43-source-package-context-{prepare,verify}-02.log与out/c43-source-package-runner-build-02.log。107个job（5个native seed工具＋34组gen/tbl/net）现以新CTX03逐job实跑：tool-gen、tool-tbl、tool-net、tool-compilerpack、tool-pack五工具均PREPARED/实际rc0；CTX03全部107项均已实际执行，其中106项outer55实际rc0并输出PREPARED；唯一net-warnparse的outer55实际rc142，外围UNKNOWN/timeout保留，仍不计完成。全部34组recipe的102个native gen/tbl/net步骤都已实际执行，实际NN数值来自C源码构造器；这不等于外围107/107完成或完整包通过，也不等于经典18张gold表网参数源码重构。该项native command.status实际rc0，result.net和record.tsv已写，但没有最终PREPARED输出；out/c43-source-job-net-warnparse-03.log及超时原状态保留。gen-warnparse/result.json为67615824B（约65MiB），base末尾重复recursive校验造成预算问题；Source329内核gen13全部9个phase已实际rc0，新context与两份blob均fresh生成。旧consumer 5c7的PC01 prepare、全部34组modelcopy和6条route均实际rc0；warnparse材料在consumer中双端复验与复制也实际rc0。这是独立材料消费证据，原producer outer142及缺PREPARED终态不改绿，不重造NN或延时。旧CTX02四工具成功与tool-pack缺zlib.h实际rc2的stderr保留为历史，不用新绿抹旧失败。Ubuntu jammy zlib1g-dev 1:1.2.11.dfsg-2ubuntu9.2已下载至out/c43-zlib-dev-01，并仅用dpkg-deb在本地解包，没有系统权限/安装修改。显式CC CX adapter源码out/c43-cc-zlib-driver-01.cx的SHA为91b3a50ce40cab8000cbffe4957051ccb095e686db8f5f06b553fdd0a28ce275，ELF SHA为699ee495860448b975c28f3046a393700987f0b5178223778fed6139ffcf69b4；每次调用核GCC、两头文件和静态libz.a SHA，用明确-I与绝对libz.a替换-lz。新CTX03 prepare实际0，复用同base9c2e71...的native runner02代码执行当前job；不需改贴Source329，也不缓存任务结果。旧CTX01/runner01与唯一tool-gen01 PREPARED保留为旧工具历史，不迁作新CTX03的已执行job。独立consumer tools/source-package-pack.cx的源码SHA为5c7ad479cdedb0bc726219825d1856da733e37a944c2d85c5ec41dea61bfa8b7，native -O2 -b lnx/x86_64编译实际0；该旧工具PC01的prepare/34modelcopy/6routes已实际0，declare01却在完整输入核验时outer55 rc142，phases/declare均未创建，尚未到native命令；旧5c7阶段的pack/seal/最终verify未跑。上一冻结consumer tools/source-package-pack.cx SHA56a0be5e2d321456c825d3f8997fcd598303295b62e095af714a2f12dfc12117已-O2 native编译实际0；新PC03的prepare、全部34modelcopy、6routes、declare、pack、seal和--verify各自均在原outer55内实际rc0。完整source-package-v1材料compiler.pkg为1588745B、SHA1c6a503090ee8d4189d6326139fa4a18b5db14b9682f1db0949e8ee79519db3e；model-inputs.tsv SHA为fbabeffcc27d9ca0a677ce793aea7645aa8d30a97893e483755a3aa14c950b8b。只记PREPARED/VERIFIED，不是功能PASS。PC01 declare142与PC02 pack142原记录保留；PC02虽留下同SHA包，也不借其未获终态的外层结论。 新tools/source-job-verify基于SP模块a8aebdd73000fc607bfe4f5353788af81cdbadb457585f197788e1d69c7660bf实际编译0，对net-warnparse两端纯读复验实际0，每端53个物理文件、6个祖先job、1个模型、约0.58s，见out/c43-source-job-verify-warnparse-01.log；没有重造NN，原producer142不改绿。旧56a/a8ae下tests/source-job-snapshot.cx已native O2编译实际0，原outer55 gate实际rc0、67/0，见out/c43-source-job-snapshot-01.log；两端各53file/6job/1model，具名拒绝、整输出清零和恢复均通过。编译artifact仅复用代码，任何调用仍实际观测/校验，不缓存PASS。已提交66d8646a99只是构造器/消费者/用法的可审查里程碑，不是包通过，不称P3包、新Source329产品或完整N22通过。宿主CC/header/linker/zlib、初始root辅助测S、完整SID旧weights与受信任bootstrap边界不变；P3源生产链不等于经典18张gold表网参数源码重建。

**内核SHA机制与预算：** tools/cxlib/kernelsha.cx显式依赖Linux/x86_64-LP64的kernel AF_ALG hash/sha256；每次新socket、无fallback、无结果缓存，失败直接返回负Linux errno，调用方errno不变。旧ce019正式probe的empty/abc/65MiB固定摘要均实际匹配，load 51028690ns、hash 380484652ns；这是分开的装载/内存hash耗时，不直接等同stream耗时或-run启动成本。当前8ad60正式module也已重新native编译并实跑，out/c43-kernelsha-module-probe-run-02.log的empty/abc/65MiB摘要均匹配，load 55760174ns、hash 386130270ns，同一输入两相合计441890444ns；软件先前4501887772ns仅为单点对照，非OS-cold或统计加速结论。fresh helper在out/c43-source-after-afalg-01.sha256仍观察完整Source329。跨模块errno传播首红out/c43-kernelsha-regression-01.log的142断言2失败保留；新8ad60f9004f98b6bd68397668e1c6feabe468bb5be4b5ec9aedc4494aa541e31修显式错误返回ABI，永久tests/kernelsha.cx在原outer55内实际rc0、144/0，见out/c43-kernelsha-regression-02.log。仅bootstrap下kernel机制回归，不是新产品或P3包通过；上一冻结consumer56a/SPa8ae已有PC03完整包材料与warning双端复验实证，仍非新产品资格，不称-run编译复用/启动缓存已实现。

**Source329基线阈值调优与材料实证：** 旧SPa8ae对PC03完整包的独立双端纯读复验已实际rc0，见out/c43-sp-package-probe-run-01.log；每端1004个物理文件、107job/34model，耗时7744849441ns与7753816590ns。四端仅SP即约31s，已超selfhost原30s，候选构造前因此把hybrid阈值从1MiB改为64KiB/provider-v3，不放宽30/55、不减任何读取、不缓存PASS。新consumer冻结dd6f5e6068289d04346f202035fb1ae3c84a10e254630c7ba5a650c7a06deb46，SP冻结407f5ef7bd8cf0b65d1086db12b36ab6e19627dc8a0ed5f2e4002df8eae22442；KS8ad、Source329和base9c2不变。PC04已以原NN字节重装配，prepare、全部34modelcopy、6routes、declare、pack、seal和--verify各自均原outer55实际rc0。完整compiler.pkg为1588745B、SHA256=1c6a503090ee8d4189d6326139fa4a18b5db14b9682f1db0949e8ee79519db3e（同PC03），当前model-inputs.tsv SHA=b10f49023fbcd78b813ec2adc1df05e42cfa6cd068bf2e383b4a80b38131f822，package.tsv SHA=431639124a24de143a82dbb455fceb3fd8fa815b48085d5199853138952b6e3f。新407/dd6独立full-SP probe02实际rc0，两端各1004file/107job/34model，耗时5110674321ns与5088791923ns；仍全部真实内容读取，不计SELFHOST通过。新严格pin gate native02编译0、原outer55实际67/0，见out/c43-source-job-snapshot-02.log，两端53file约.55/.52s；旧PC03/旧67日志仍归旧身份，不抹或转授。安装前root为P53/S447 bootstrap；当前已实际本地安装P53/S329，见下文当前安装身份；Source329完整候选A已构造，version/smoke16/candidate58实际0后归档。新rbce7/installer e37改变tool context后的候选B五slice/wrap及smoke16/candidate58/schema79已实际0，installer协调器编译及旧P7拒绝/无副作用已验，随后实际本地安装rc0，Root329 record已独立核0，本机原30两代selfhost03已58/0，完整N22/外宿主仍未完成。材料只PREPARED/VERIFIED，不是功能PASS。

**完整候选A与当前重验：** 旧rb813166/product2872/source-candidate0e7已在9abe0e86cf提交。GEN13旧product context 699ccb6169f39f77868f0835cd5dd13b9ee1bd7c773fc938cc968cad19a0b00d的5个slice及wrap均实际rc0，完整候选A为4911193B、SHA256=53e2cd1f669c92fda2c16dd93a9021cd4db97f05c57ab2d1acf9a835cbac9857，与旧Root同字节。这是完整Source329实际重构观测，不是二进制时间戳刷新。候选version0.0.43、直接candidate smoke16/0与historical-bootstrap准入58/0均实际0，后者见out/c43-source-candidate-13-01.log；不等于完整功能或selfhost通过。候选A阶段Root当时未安装，保留S447原record，record SHA=e0843c0c025f05e728c8584a7fd5db51965345316b65c6316815bd57787dbe45。

随后发现旧installer的legacy头路径只核kc与嵌包，漏完整P7准入。当前rbce7f/installer e37草稿改为显式schema合同：mode2只legacy kind1并完整走P7 BOM/corecompat/v1，mode1只source kind2，mode0原契约不变；相关候选B的source准入58及schema拒绝/失效旧BOM79已实际0；这是API合同实证，不是legacy正资格或installer执行通过。根已自身CX核旧ctx699与SHA后，将12个旧product/native receipt归档至out/c43-native-kernel-13/product-context-699ccb61-before-legacy-admission，见out/c43-product-13-a-archive.log；9phase ASM和NN未改。当前同kernel13的新候选B在context14954ce18cff8a1b087c22a60b56546b19a3fb149005240ff8333fb108b98cba下5slice及wrap各原55秒全部实际rc0，完整4911193B/SHA53e2cd1f669c92fda2c16dd93a9021cd4db97f05c57ab2d1acf9a835cbac9857仍与旧Root同raw；B version命令实际0、直接smoke16/0，新rbce7下source-candidate native02与outer55实际58/0，见out/c43-source-candidate-13b-02.log。candidate-schema首跑out/c43-candidate-schema-13b-01.log为72/1，是测试预设旧BOM顺序错误：unisa/ckernel.py在row1042，旧db184ffec27565ad0a6d725d49710e7818af7d8e686fb642b67c862b88503aa3、当前d3400bfb0d8dab72c8851eec67fba9c732815cb46ac90d9466791be027e5b9cc，早于kernel/unisa_core.c row1115；生产精确model-input-digest拒绝正确。首红保留；测试保留原72义务并加7个独立旧行唯一/行序/固定hash/当前模板不变前提，native02及原outer55实际79/0，见out/c43-candidate-schema-13b-02.log。三项精确schema/BOM拒绝、A5全清零、恢复和材料不变均0，仅invalid-old-BOM探针，不是legacy正资格。正向安装前Root为S447/e084，installer e37协调器native build已实际rc0，见out/c43-candidate-install-runner-build-01.log。真实installer对old-known-P7 schema及旧BOM负控的child rc2，精确model-input-digest path=unisa/ckernel.py；根自有CX薄调用核验外层rc0，见out/c43-installer-reject-probe-run-01.log。Root/candidate完整SHA均未变，candidate metadata/backup/staged共六路径均仅因ENOENT确认不存在，未安装；这不是legacy正资格或安装成功。随后installer已实际完成原55秒双端准入并本地安装，Root329记录独立核0；随后GEN14九ASM已完成，新RB/product协调器重建两代产品上下文，原30 selfhost03实际58/0；接下文当前完整固定点和fclose未修缺陷。

**当前已安装Root身份（不同于工作源）：** 10d2a688a4已提交rbce7/installer e37/schema79完整schema准入。真实installer在原outer55实际rc0，输出INSTALLED development-only-no-release，见out/c43-candidate-13b-install-01.log；Root现为P53/S329，不再是仅供启动的旧S447身份。完整raw仍为53e2cd1f669c92fda2c16dd93a9021cd4db97f05c57ab2d1acf9a835cbac9857、4911193B，是同字节真实Source329构造，不是更新时间戳。当前unisacc.com.build.json SHA21704e50abc28930d9a0792fdb9f895aee49b13cd163dc783451784d3bb73ef5，record.commit=10d2a688a491d6b4740377cbe46d9eaabec00ce1，source=329cdb7077891074f638634dc7abdcd59ee39c368ad6ba1c2fcf867c3c63de2e，nativeSHA=3d75d3557afa43d441a8bdfe6a7cbc992161da90326bfd0d751e9e3d0a115a7b。独立helper对ROOT/record检查实际0，见out/c43-installed-329-record-check-01.log。GEN13/bootstrap-before-install.com的raw53、旧record e084/source447完整保留，与原checkpoint相同；PkgCTX03旧ROOT epoch继续由显式checkpoint raw+record双hash provider核，不改贴旧epoch。先前GEN14九ASM与旧c275/149产品上下文构造曾实际0，selfhost01原30超时142，日志保留；当前通过事实与未修缺陷如下。

**已通过Source329原30秒固定点基线（不转授变更中的源码）：** 新RB冻结699b0a2cc5ffb1a791c024aa0b2cb0c21a9e2ffe213e77f69c4535132ceebd92、product协调器68d55d5548922930d1223445098d7e8418539e6d5d84e1e08ddab85fdbacd15c。GEN13/14各五slice＋wrap共12phase独立原55秒全部实际0，未重造PkgPC04、NN或九phase ASM。当前GEN13 v2 context1660a13221c0d3034b396af62d4621290ba8bb61deb173128582629595df091d绑定saved447/e084 builder；GEN14 v2 context5cf8787fec970a934c9d50caa2e9ead3116ca19ccdff0d3e9e24d756c080c437绑定当前Root329/21704 builder；完整raw同P53。tests/selfhost在原30秒实际rc0、58/0，见out/c43-selfhost-source329-03.log；bash内建time（含watchdog）wall26.414s/user7.701s/sys18.999s。四个SP端点均实际107jobs/34models/1004物理读，分别5072600419/5155174425/5069025671/5094686385ns；after-check11项Root/record/原builder和冻结文件均OK。本轮runner源码/flags/context profile保存在out；所有断言、四端真实读取、后验与原30秒保持，不缓存PASS。早先01（旧GEN14 c275/GEN13 149）超时142保留；02仅/usr/bin/time缺失wrapper127，未启动测试，不当测试失败或PASS。该结果是Source329本机两代完整固定点，不是HOST-seed三阶段N22、外国native资格或全版本完成。

**Source329旧库缺陷历史：** Root默认-run out/c43-fclose-devfull-probe.cx实际rc1，见out/c43-fclose-devfull-product-01.log：buffered fwrite1/ferror0/errno0后，fclose错误返回rc0/errno0，但FD已关闭（EBADF9）。同literal宿主系统header参考用/usr/bin/cc（POSIX200809L/O2/隐式声明视为错误）build02/run02实际0，正确EOF/-1/ENOSPC28及关闭行为，见out/c43-fclose-devfull-host-run-02.log；host01隐式fileno警告保留，仅clean02作参考。Root/record/CC/两份source/stdio前后SHA均未变，Source329当时include/stdio.h为22ede652a598774465af4d8ecf8cd3195b7fe948a1b9492ab717a39cab179b9e旧字节。**当前状态：SOURCE_FIXED / PRODUCT_NOT_REBUILT。** 最终工作SID已fresh核为0d59237f3250b59514a55ff529c77b665033e5feb71061f6fcaded0c7ef925c0（含scanner修复/正规头再生）；cf2270b2e103ead1d13859489a34d6eb9e57a127eb0bef5f347b17822cb6c16e仅是两header修复阶段的实测身份（out/c43-fclose-source-identity-01.log，原helper bound8 rc0），不能代表后续含scanner修改的源码。Root仍P53/record21704/Source329，未据新源码重建或安装；已通过的Source329原30 selfhost58仅保留基线/历史，不转授工作源。

stdio源码SHA=a6f5cc020e04e547f2668f162f736b5eac423bf1ec1e241406a745d06e2b01e0，include/sys/_exit.h SHA=1b35e59300e2120420d3261c7239159c08332c43d2efb87bde9ad2594ed88701。永久tests/stdio-close.cx仍为7106b12654dec939d6657a71b95ad379cb0247c585273d5c119d9f9cf3ecc8b5，未改338断言。旧默认-run out/c43-stdio-close-before-01.log实际rc1/338断言48失败，仅24轮EOF/ENOSPC两项；显式sh root -I include运行新工作头，out/c43-stdio-close-source-01.log原bound55实际rc0/338/0、stderr空。最小probe同样以-I include核新工作头，out/c43-fclose-devfull-source-01.log原bound12实际0，EOF/-1、ENOSPC28且closed EBADF9。此为工作源码header合同通过，既非新候选资格，也不说明默认嵌入库头已更新；旧Source329原默认头的真实红与宿主clean02参考保持历史。

scanner已精确修复seed/unisa_libneed.c，SHA5f0469bcb6268bf979d8bb872424ae238c8f991b51543337b6f4b2750b2fdbb1；宿主CC以POSIX200809/c99/O2/Wall/Wextra/implicit-declaration-error实际compile0，ELF out/c43-libneed-attributes-01.elf SHA5c6a7756fa4de143a3e74ec14f392fc8ee736755ad55043cb976ace0fb2bb011。out/c43-libneed-attributes-contract-run-01.log原55实际rc0/12断言0失败，覆盖flat attrs exactspan/closure/拒绝/稳定；永久tests/libneed-attributes.cx已落地，SHAfd7fa200469f1f5aecb3f7567ba0ab7c77759e48d6f417a562295b53988c8679，与原out合同实现相同、仅注释调整；新-I include默认-run原55实际rc0/12/0，见out/c43-libneed-attributes-permanent-run-01.log。construction.libneed-attributes与library.stdio-close现均登记独立SOURCE55 coverage/batch，IMPLEMENTED仅表示入口实现，不等于本轮PASS，broad library仍NOT_READY。

tools/carried-header-export.cx SHA432bf38606d945a1acb8508e04ab784127842593c3ac2a51013019ac1c00acc4已以-I include/O2 native编译实际0；prepare/--verify/install各独立原55实际0、stderr空，见out/c43-carried-header-{prepare,verify,install}-01.log。完整旧生成头的header names/text/routing/LIBKEY/OFF/DEP/BODY/ROOT canonical oracle已全部实际通过；新kernel/unisa_headers.inc由该工具正规安装，SHA482d376cc8eb2c5152512a4ead9841f47b32e23b948024a10718eb82b5806d9f，未手改生成物。旧e63生成头原字节保存在capsule及新MAT previous文件。新旧libneed table完整实际SHA均aea1293b96da78c6a0cfaf0354e47e840d414de41bdea735bead30d736c8c853，autoinc投影探针out/c43-autoinc-projection-probe-01.cx SHA6363a2235de1781dbeb150a34f5694b60cb02814bdb292557a3781054e3cd6a3已显式-I include默认-run原55实际rc0/3/0，见out/c43-autoinc-projection-run-01.log；按实际6头顺序及Latin1行规则，包含printf和剔除printf两个完整canonical投影逐字一致，两个include完整树及全部support四端fresh稳定。结合完整libneed表同aea129...只形成NN采用的必要条件证据；NN adoption仍未实现，不称完整pp事实等价或复用完成，新产品仍未重建。

上一轮完整fresh工作SID为0d59237f3250b59514a55ff529c77b665033e5feb71061f6fcaded0c7ef925c0（out/c43-fclose-source-identity-02.log，原helper bound8实际0）；cf2270仅header-only中间实测身份。Root仍P53/record21704/Source329，新工作源PRODUCT_NOT_REBUILT、未安装。origin capsule现已完整706MiB/19分片及finalverify各原55全部实际0，SEALED SHA87f7d1fd1469d170556f8a0cc2861d27e14de05f687e97e4805db135268c3273，原helper两次实际重扫完整329，见out/c43-origin-capsule-final-verify-01.log；不改旧收据/旧epoch。GEN15按先前Source0d实际九phase各原55全部0，context67fb6c9b0dacf0c385bf05e49b49a67493d34bdb489afd5f91589a6d3471cd3f；两blob同旧4d6e(7096B（含40B头，body7056B）)/32ce(7144B（含40B头，body7104B）)，仅PREPARED、非功能PASS，Root/record/runner/builder源后验OK。

新origin SP API98848f9f99ef35cf63504c942818b550f865db1527f055deca61cab86802606b与生产wrapper bf9a331c30eeef30c016debf487eb1319c7d5cc8f2c1dc822f3f5dd93b8b68f7已真实用CAP/root/unisacc.com及绝对-I当前include/O2/native编译0；ELF de8511f0501a1bafb1e2038b3efa89352022d19dccaa0130b0442346f1d8588c，独立保存bootstrap53/原record21704/source329，不绑live Root raw。--write-profile实际0，profile4793f9d98c489b224f020cbc58d0f217c33500cb496147474d4fe73ce8a54bba；首次--snapshot原55实际0，见out/c43-source-origin-snapshot-01.log，来源329/107jobs/34models/1004物理读、execution_reads148/wrapper6，elapsed6397320828ns。当前执行SP988与origin规格407分列，只证明旧来源DAG新鲜核验，非当前Source0d构造或测试PASS。该首点保留历史；当前索引SP a832永久origin gate实际37/0，详见下文。

历史NI module8becdff327ca5bfed12bbee94367cf59cd3dd91ab68933d0fae66394a4627fac已native重编0，out/c43-nninputs-probe-run-02.log原55实际6/0，7791 reads=current3888/origin3888/support15，包含全部四树四端/10Cdeps/目录mode；完整include和origin DAG另核，2个native调用的内部读取仍UNMEASURED。elapsed3627775504ns、reader3626988686ns；native_current1503902949ns/native_origin1455617847ns，四树各约0.157-0.173s。旧NI cd50/3.4738s首点保留历史，不作OS-cold/统计提速、完整PP等价或NN采用完成宣称。

当前索引SP a832、新verifier ELF02 52fc/profile02 6acb已真实构建；永久origin gate原55实际37/0，两正例各107jobs/34models/1004物理读，5082765804/5145743979ns。旧98837日志及out/c43-sourcepackage-before-index-988.cx完整保留。执行新SP与来源规格407分列，非Source0d重新构造。fast生成器221119 prepare0、derivedCX6b48/nativeE4编译0；两include完整表8/0、39810B/SHAaea1293b96da78c6a0cfaf0354e47e840d414de41bdea735bead30d736c8c853精确相同；旧native1.452/1.473s与CX.161/.159s仅单点观测，非OS-cold统计。fast-attrs01保留旧fd7逻辑12/0；报告标签改REFERENCE_C_SOURCE后同逻辑新版尚待重跑，不冒称CX是HOSTCC历史编译。NI a95/profilewriter954实际编译/创建0，E4.cache.tsv dfab.../3318B绑定compiled header NI1ed1cb...、原MAT/bootstrap/source/module；永久NI9217 native02编译0/原55实际104/0，两完整positive约1.14s。旧93/1日志保留：Root.com非ELF的精确reason预期修正，原93义务全留并新增11项shape/只读不执行私有ELFclone/未知artifact拒绝前提。机制已提交02bb4c614d与789e7d4c6f。

新adoption producer a546/native03 9457的prepare03/pack01/seal01/verify01各原55实际0，PC=out/c43-adopted-package-01；新P3为1589556B/SHAe486e9523c30c6c4985050f12c5e6a4323f8f6d8d84b9088709083f83b3e52a5，manifest20e5ac180c3e88a0fd415ca670a59e666731fa65d044a57948ade77657075a47、receipt24ee9ffc4fb6e1b2d757b7f55e8dec03e424cf63804454fb6ff180e229adbbc3、ctx2b1a53bd337b1899a76271139171372b16c71dcd11e913dec8b600c2173f45f4。34NN与declaration明确ADOPTED Source329，107完整freshOriginDAG+NI+当前G15核验证全保留，仅pack以当前headers实际EXECUTED。prepare01/02 rc2原日志保留；*.kernel到P3私有kernels/arm64,x86_64映射修正，未删验证或重贴旧绿。raw kernel7096/7144B=body7056/7104B+40B头。

**当前工作源S03a5与构造阻塞：** exec/parse2/forward-result.tsv修复已冻结014e1ac4...；G15实际helper bound8 rc0 fresh测得完整SID 03a5e1b77b24c7fcfe92d289c53d772088bb1b07a94bb3d419a34c75e2bd2334（out/c43-forward-decimal-source-identity-01.log），不再PENDING。Root仍P53/record21704/Source329，旧S0d采用包/GEN15/15/37/104仅保留先前材料和机制证据，不转授新源码或候选。RB3源码已实现，但native/default-run编译所用旧产品仍受旧前端编号编码缺陷阻塞，无新slice；旧表以单字节ccw_i+48输出，第11个编号10变冒号，badargs默认-run实际rc1，out/c43-adopted-rb-default-repro-run-01.stderr line400482，原Tape=out/c43-driver-tape-debug-01/stage-2243329-000.txt:400482。24576字节struct-copy probe run0，不归因为大struct。source-package-forward-runner01 native已实际编译0，9c2构造器源码未改；新partialCTX已prepare0，tool-gen/tbl/net实际原55各0且raw与原CAP构造工具逐字同身份；shared-e3/warnparse/errorparse三NN材料已准备，其中两net生产者outer142终态保留UNKNOWN，不能借独立材料验证改写；全部34个current gen已实际原55 rc0，与旧origin全JSON逐字比较，仅shared-e3/warnparse/errorparse三项diff，其余31项same；三net native均0，其中warn/error outer142保留UNKNOWN；混合schema构造与新候选仍待完成；当前NI完整相等必须拒绝真实变更，不排除或缩覆盖绕过。新候选未构造。identity-material a052仅native02编译0，MAT02 prepare/verify及派生native编跑仍NOT_RUN；旧bb7/MAT01准备复验0及派生compile01缺模块rc1保留，不称加速。19缺口、原30/55、四fresh端点与签公开禁令不变。

先前Source0d材料--snapshot原55实际rc0（out/c43-adopted-package-snapshot-01.log），singleendpoint8.8639s、origin5.1836s/NI1.1473s/body9.0065s，各计时范围不同不相加。四fresh端点据此推算超原30，但kind3完整selfhost尚未实跑，不记实跑红；下一步实测压缩helper/controller开销、保原30/四完整端点/全部断言，修复旧产品forward-result编码缺陷并fresh测新SID，重构三NN及实现其余31混合来源schema，再完成已写RB3的实际编译/installer集成与新候选；原snapshot adapter15/0仅旧Source0d合同。当前采用包仅PREPARED/VERIFIED材料证据，非功能PASS；Root仍P53/Source329，Source0d新产品未重建。两精确机制construction.origin-package与construction.nn-input-applicability已登记独立SOURCE/本机/55 IMPLEMENTED批，37与104有限合同不抵19 broad gaps；严格--list已实际rc0：98batch/79实现/19planned/63unprepared，113coverage/94实现/19缺口，executed0/NOT_EXECUTED；非批次执行器资格。snapshot adapter默认-run原55实际15/0（out/c43-adoptedsnapshot-probe-run-01.log）：7项非突变负控+1完整正端、own51reads/9.355677247s，仅模块开发验证，非RB3或新候选资格。csr旁路私有v6约0.07s及新增tools/c44-rebuild-private.cx一键私有slice+wrap仅其报告，非main43资格；已确认size+mtime捷径不吸纳43。签公开禁令及-run主线未实现不变。

**采用单端永久合同与当前聚合实测：** tests/adopted-package.cx（47d338b73d1a87d4f1986173f176591016ec8549503a8b383019a6b5183f50fd）已提交0c6171bef9；原probe out/c43-adoptedsnapshot-probe-run-01.log与永久路径out/c43-adopted-package-permanent-run-01.log各在原55实际rc0、15/0。七项非突变参数/SID/规范路径负控、存在out时整块A5清零及一次完整正端均保留；前者own51read/9.355677247s，后者own51read/8.958785555s，只是两次具名有限合同，不是RBkind3、NN功能或新候选资格。现登记construction.adopted-package独立SOURCE、lnx/x86_64、55秒批，真实CLI为`ROOT PC FRESH_SID`，材料adopted-package给sealed PC、current-source-id给独立fresh helper实际测得的64hex值（不是路径/旧record）；args为`{root};{adopted-package};{current-source-id}`，终态`rc0:ADOPTED_SNAPSHOT`。本机实际入口：`tools/bound 55 sh ./unisacc.com -I /home/devali/repos/unisacc/include tests/adopted-package.cx -- /home/devali/repos/unisacc /home/devali/repos/unisacc/out/c43-adopted-package-01 0d59237f3250b59514a55ff529c77b665033e5feb71061f6fcaded0c7ef925c0`。controller bootstrap P53/21704/Source329不改贴为旧NN生产epoch447/e084。最近实际聚合out/c43-adoption-testplan-all-01.log原20 rc3：metadata_valid1、113coverage/94implemented/19unresolved、97not_run_batches/78not_run_coverage、仅现有Root smoke16/0、executed_subprocess_batches0。该--all及旧98batch/113coverage的--list均发生在本次单端批新登记前。登记后严格--list已实际rc0（out/c43-adopted-package-testplan-list-01.log）：metadata_valid1，99批/80实现/19planned/64unprepared，114coverage/95实现/19unresolved，executed0/NOT_EXECUTED；仅解析元数据，不等于批次执行。下方111/92、旧109/90及原红/UNKNOWN均保留历史；原30秒、四fresh端点和19完整缺口不变，未以独立15日志给--all填绿。

**历史证据不转授：** P53/S447的RI30/15/8/17、capture/consumer/snapshot/库、P3四CASE、profile133/目录30、objects198/formats14、固定点58、固定P3宿主链41/拒绝71、属性100/优化241/193/16，以及pp构造92/配方控制23均保留原事实和原限时。pp低mask域不因本次高bit缺陷被抹成失败，但旧MAT的完整SID与数学体身份不再代表Source329；旧RI/P3/自举和旧CI也不自动适用新候选。原文与身份/范围见[Source447证据快照](archive/prd-c43-source447-before-unsigned-math-20261026.md)。

**当前S03a5有限union合同：** 新增tests/source-job-union.cx，精确保留out/c43-source-job-union-probe-01.cx的36义务与CLI `ROOT CTX FRESH_SID`，无copylive；construction.source-job-union登记单独SOURCE/lnx/x86_64/55批，材料source-job-context/current-source-id，终态rc0:SOURCE_JOB_UNION。原probe out/c43-source-job-union-probe-run-01.log实际原55 rc0、36/0、stderr空：四项非突变参数拒绝各核rc/why/整块清零，两fresh三net闭包各12jobs/3models/89reads，1.446987410s与1.462053267s，换序canonical摘要及data witness相同；独立三工具集3jobs/0model/35reads、0.083057602s，父进程存活。永久路径out/c43-source-job-union-permanent-run-02.log已实际原55 rc0、36/0，执行模块SP5cbc（永久test仅相应pin更新，断言不变）；两fresh三net端仍各12jobs/3models/89reads，1.449938355s与1.385322946s，独立三工具3/0/35reads、0.053068219s。仅验证材料，非NN推理、当前完整107包、混合采用或新候选资格；原生产者两net outer142仍UNKNOWN。新origin03实际实编profile且完整正端0：ELF dfc1fbd76be4da34a3e9a4cfe7823a94a3498d47918ed7e0013a427fd7d9cac7，profile aeb85359e6d9ff85a3563ac58aa879e283bee254c4d0906f7e3136833eb7f068，当前执行SP058a与origin_spec407分列，107jobs/34models/1004reads、5.174877257s。新origin04实编/profile/full-old快照各原55 rc0：ELF ad550418ff24462c2c2a017790b85e1c4a70f67d9b9f93adbfe75fde614d5a13、profile24daf95e0f16f404f4ad1737203abb4a923add5e118cbef2d25c0e1506b9de05，执行SP5cbceaa8a5bd424b5a40005c8b29c7db4f4ea70c982892a3bdeed9416c915aff，origin_spec407不变；完整107/34/1004耗时4.910414267s。实际执行provider-v4阈值1024、旧material声明provider-v3阈值65536分列，旧canonical材料未重写；与origin03的5.174877257s均是单点，不称统计性能结论。旧Source0d的15/37/104与Root329/原30 selfhost58仍仅原身份历史；当前SID03a5新候选未构造，19缺口/原30/四fresh端点不减。

**清单与下一动作：** union独立SOURCE批登记后严格--list实际rc0（out/c43-source-job-union-testplan-list-01.log）：metadata_valid1、100batch（81实现/19planned/65unprepared）、115coverage（96实现/19unresolved）、executed0/NOT_EXECUTED；仅严格解析，不给--all填绿。此前严格--list实际rc0（out/c43-adopted-package-testplan-list-01.log）：metadata_valid1、99batch（80实现/19planned/64unprepared）、114coverage（95实现/19unresolved）、executed0/NOT_EXECUTED。采用单端批登记前--list的98batch/113coverage及79/94实现、19planned/63unprepared保留历史（out/c43-adoption-testplan-list-01.log）；该阶段--all原20实际rc3，113coverage/94实现/19缺口、97批/78coverage NOT_RUN、smoke16/0、executed_subprocess_batches0（out/c43-adoption-testplan-all-01.log），不改贴为登记后聚合执行。新增两机制前最后实测清单--list原20 rc0（out/c43-fclose-testplan-list-01.log）：metadata_valid1、96batch（77实现/19planned/61unprepared）、111coverage（92实现/19unresolved）、executed0/NOT_EXECUTED。随后--all原20实际rc3、stderr空（out/c43-fclose-testplan-all-01.log），metadata_valid1/111覆盖/92实现/19缺口；仅SMOKE16/0，95非smoke batch及76 coverage NOT_RUN，executed_subprocess_batches0，未知或未跑未记PASS。旧109覆盖/94批/90实现为历史，不冒充本次新增两机制后的计数；新增元数据现已严格重解析通过，但未执行任何批次。 Source447的out/c43-all-pp-source-installed-11.log原20秒rc3只含smoke16，其余聚合NOT_RUN，不计Source329资格。Source329 gen13九phase、当前PC04完整包材料和新full-SP双端均已实际完成，新pin gate67/0；候选A在旧ctx699下完整构造并获得version/smoke16/candidate58实证后已归档；当前候选B新context14954ce...的五slice/wrap、version/smoke16、candidate58与修正前提后的schema79均实际0；installer已实际双端完整准入并本地安装rc0，当前Root329 record独立核0；GEN13/14新ctx五slice/wrap12phase全0，Source329原30 selfhost03已实际58/0、四端物理读取与11项后验完整；01超时保留、02wrapper127未启动测试。fclose两header现为SOURCE_FIXED，header-only cf身份下显式-I include338/0；Root默认头仍旧329，新源码PRODUCT_NOT_REBUILT。scanner12与完整旧header oracle、导出prepare/verify/install已实际0，前序工作SID0d5923...已fresh核0、当前修表SID03a5已fresh核0；capsule706MiB/19片+finalverify全0且SEALED87f...，GEN15九phase全0/PREPARED；索引origin SP a832永久37/0、NI a95永久104/0，fast reader实际完整表8/0；新adoption包prepare/pack/seal/verify/snapshot各55实际0，仅采用Source329材料而非新NN构造。新kind3四端点据单端观测推算超30，完整selfhost尚未跑；当前forward-result表已修、SID03a5已fresh核0、9c2 forward-runner01 native编译0；新partialCTX/三工具与三NN材料已准备，union probe36/0，生产者两net outer142保留；全部34gen原55实际0且31JSON逐字same/3diff；永久union实际36/0、新strict-list实际100批/115覆盖且执行0；下一步G16九phase与动态混合材料构造，候选未建；RB3已写但受旧产品编译缺陷阻塞，保30/四fresh端点/全部断言后再实际RB/installer与新候选重验，不改旧329收据冒fresh。旧ctx58保留为历史，不替代B已独立获得的新58；installer native编译已0，旧P7负控child2/外层校验0、无副作用已验；随后source候选已实际安装成功（development-only-no-release），非完整发布资格。PC01 declare142、PC02 pack142与原producer142均保留，原55秒预算不变；PC03包PREPARED/VERIFIED不当功能PASS或新产品资格。classic十八网从gold的通用构造及ASM从源重演、完整RI/属性/资源/库和六宿主资格继续保留。C44-RUN-REUSE-01在主检出未回并；csr旁路旧v3约0.17秒保留历史，最新私有v6约0.07秒仅其报告，非43资格；Root已读v4确认size+mtime捷径缺口，不吸纳43；旧启动/对照测量归P53历史，内部相位/OS-cold未测，不宣称启动提速。当前不得签名、Draft、Latest或公开。

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

**本轮入图（2026-10-27，cdx 主刀）：** cc 的纪元1 A3 已有 107/107 收据、SID `df8f29fd…`，主人在 A4 前暂停，PC/GEN/准入/安装未做；产品仍是旧 Root7a。产品默认 `-run` 的模块 `.cx` 二次运行仍约5–11s，独立开发代码缓存已先收反馈墙钟；cc 已按当前产品交 `dev-run` 内容寻址代码缓存，模块热约0.10–0.13s；cdx 独立核 `run-batch` 控制器热7.72s/16/0，默认15.57s/16/0。缓存仅 DEV_ONLY，不替代 SOURCE 默认 `-run`；grk 的标准库／模块路径工作仍隔离，A3 不重开。Codex 辅助已交外宿主草稿 producer/artifact head 一致检查（`b40487eadd`，cdx 以 `5b4616387f` 保留较新验证提交）与跨命令 B1b 收据 `dabc5a0efb`，未派发 CI。lower 四个旧 Root7a 正式批 `9a44ce9869` 为 51/0、778/0、1516/0、115/0；ABI 六项 UNVERIFIED、输入 errno 29/2 红。六目标流材料 `61ba5c621c`，五目标有限 oracle `611a8c62d3` 各 17/0，材料显式绑定补丁 `4ef850b1c7` 的 lnx/arm64 正式复跑 17/0；外国目标只在本机验证输出，不授外宿主资格。`sizeof` 红例定位在产品 `exec/parse2` 末端成员数组衰变路径，须下一 SID 修。当前矩阵为 822 必选／245 本机覆盖／3 观察／408 缺测，旧 257/421 摘要作废；完整 lower 父行和其余 18 宽行、N22、五外宿主仍开，`--all` rc3。

**现行入口：** 下一表第一行是当前身份与行动；其余旧行保留来源历史，凡与第一行冲突的“当前”字样均已失效。

| 房间 | 当前事实／决定 | 证据与行动入口 |
|---|---|---|
| 0.0.43 properties 有限域（2026-10-27） | 独立新测叶 41/0、7.59 秒，本机循环边界／变形／拒绝／恢复绿；完整性质域及五外宿主未测，不改宽父行。 | `tests/pending/properties.cx`、`out/c43-full-properties-local-04.log`；下一步先补完整域矩阵与同候选外宿主。 |
| 0.0.43 P3 独立前缀读取器（2026-10-27） | 对现装 APE 内 32 个模型用既有 CX oracle 实跑 rc0，Q 语义与运行结果相同、八拒绝；只授包内压紧格式/运行对照，不授每模型 ctx04 job、N22 或外宿主。 | `out/c43-root7a-qprefix-artifact-01.log`；下一步接 CSR 独立成员↔job 正式批。 |
| 0.0.43 P3 包成员正式交叉（2026-10-27） | `8d5e023d04`：32/32 M 解压正文与现行 ctx04 job 经 compact_q＋二进制编码逐字相等，job 文本又等于 PC 模型；私有 M28 双文本同改负控仅二进制门失败。正式批11/0、内部88/0，M3/M13/M24/M25 四路动态差分；34 阶段引用及27早期探索分账，三入口 NOT_RUN。完整 N22 未关。 | `out/c43-p3-member-all-registry-02.log`、`out/c43-p3-member-all-negative-01/run.log`；下一步审库三成员真实入口与 hostseed 历史消费边，不把静态交叉冒充全 32 运行。 |
| 0.0.43 直接原生控制器与 P3 消费缺边（2026-10-27） | GCC11 直编 native-first 控制器产生新 context01、HOST 与三代固定 P3 链；三代 APE=Root7a，chain 41/0、live bind 58/0。另新编 prior helper 同 SHA72c，context02/HOST 仅 PREPARED，未重建三代；完整独立初测和 N22 未关。CSR `/goal` 活跃；私有 P3 包内 32/32 成员与 PC04/job 经 compact_q+编码逐字交叉，M3／M13／M29 对应路线拒绝且未用成员同路线仍通，差分负控8/0；五入口仍 NOT_RUN。正式 `.cx` 测叶待审，不改 `payload-trust` 或关 all-six 父行。本机同候选产品／记录／来源／计划／commit 已重算，远端 producer 工件仍缺。 | `out/c43-hostseed-nativefirst-hostcc-context-{01,02}/`、`out/c43-hostseed-nativefirst-hostcc-stage{1,2,3}-01/`、`out/c43-hostseed-nativefirst-hostcc-{chain,bind}-test-01.log`、CSR `out/c44-p3-member-cross-01/`、`out/c44-p3-route-member-neg-01/`、`out/c43-foreign-host-inputs-local-20261027.tsv`；下一步限域正式批与同候选远程工件。 |
| 0.0.43 wrap-link 与活源正式验证（2026-10-27） | 工具 SHA101ffc 已提交；最终源码的 context03、HOST 和三代18步完成，APE 三代均为 Root7a；三份 loader trace 各唯一指向本代私有 libz，收据钉内容与 trace。默认产品整链 VERIFIED，正式批41/0，csr 只读 ACCEPT。活 source-package 同窗绑定已改接此链，两 context SID 同值且 HOST／18镜像、收据和三代 trace 前后未变，58/0；混旧代 stage 与错 SID 均拒绝。旧 context01/02 因 tool-changed 仍 UNKNOWN。只收本机装载路径和当前端点，非 hermetic/N22。 | `out/c43-hostseed-nativefirst-libz-context-03/`、`out/c43-hostseed-nativefirst-libz-stage{1,2,3}-03/`、`out/c43-hostseed-nativefirst-libz-{verify,batch}-03.log`、`out/c43-hostseed-nativefirst-live-bind-batch-08.log`；下一步 prior 出生链、宿主依赖和完整 N22。19 宽缺口、五外宿主仍开；禁签/Draft/Latest/公开。 |
| 0.0.43 收口调度（2026-10-27） | 粗估约72%，有限 PASS 不推高完成度；19 宽合同、完整 N22、同候选五外宿主仍开。CSR harness 消息拥塞后主线直接接手 P3 成员门，正式 32/32 静态交叉已提交；不以消息或私有探针授资格。外宿主草稿仅元数据/版本观察，Windows 功能仍缺 JobObject/原生路径，WinARM x86_64 模拟不算 ARM 原生。 | `plans/v0.0.43.md`、`docs/c43-candidate-ci.md`；下一步库三成员真实入口、N22 历史消费与同候选真机。`--all` rc3，禁签/Draft/Latest/公开。 |
| 0.0.43 native-first 宿主种子（2026-10-27） | v3 context03 先由 prior native helper 测 SID4fe，再编 fresh helper，三方同 SID；HOST→三代 18 步收条，三代 APE=Root7a，默认产品 `--verify-chain` VERIFIED，正式批 41/0。旧 v2/context02、超时残片不计资格。完整 N22、P3 NN/ASM、宿主 CC/libz、五外宿主仍开；清单 124 批／135 覆盖／19 宽缺口，`--all` rc3、子批0。 | `out/c43-hostseed-nativefirst-context-03/`、`out/c43-hostseed-nativefirst-stage1-03/`、`out/c43-hostseed-nativefirst-stage2-03/`、`out/c43-hostseed-nativefirst-stage3-05/`、`out/c43-hostseed-nativefirst-registry-01.log`；下一步 prior helper 来源。禁签/Draft/Latest/公开。 |
| 0.0.43 本代独立身份算法对照（2026-10-27） | 当前 ctx04 `source-job-union` 正式 37/0；新 `hostseed-independent-ident` 用冻结 GCC11 自 `seed/ident.c` 编出与冻结 helper 同字节的新 helper，GCC 版本收据匹配；批内先测 SID4fe 后测 Root7a，两者与 ctx 来源一致；正式批 14/0、stderr 空。清单 123 批／134 覆盖／19 宽缺口。批本身由产品启动；只证明批内首次身份计算独立，历史 hostseed 顺序、宿主 CC/库、P3 NN/ASM、完整 N22 和外国宿主仍开。 | `out/c43-classic18-source-union-batch-01/`、`out/c43-hostseed-independent-ident-batch-05/`；下一步同候选外国原生观察／P3 输入来源。禁签/Draft/Latest/公开。 |
| 0.0.43 本代七流复验（2026-10-27） | SID4fe／Root7a、当前 PC04 同字节包；上期 e252 构造的七流容器摘要仍钉住，八个正式 outer55 批当前实跑 PASS：整链40/0、独立 E2/E1/E3/E4/prune/lower/ELF 为36/25/17/17/19/12/11，stderr 全空。只授本机有限流实际消费，不称本代重新构造材料或七条完整流、RI、外宿主。 | `out/c43-classic18-stream-batches-01/`；下一步完整 NN/ASM 来源和同候选外宿主，禁签/Draft/Latest/公开。 |
| 0.0.43 P3→宿主链当前端点（2026-10-27） | SID4fe/Root7a/e2ab；原输入重包与 PC04 同字。私有一字节负例说明单独 hostseed 不核活 job/model。独立正式 `hostseed-source-bind` 同一 outer55 内验三代链及活 ctx04/PC04 107 作业／34 模型，14/0、stderr 空；错 payload 路径 rc2。原 41 断言批不变；清单 122 批／133 覆盖／19 宽缺口。仅单写者前提下当前端点绑定，历史源码执行、完整 N22、经典18→P3、外宿主仍开。 | `out/c43-classic18-p3-pack-replay-01/`、`out/c43-hostseed-source-bound-formal-01/`；下一步实际历史构造来源／P3 NN-ASM 输入闭包及外宿主，禁签/Draft/Latest/公开。 |
| 0.0.43 本代 ASM/身份补证（2026-10-27） | 当前 SID4fe5a4f1、产品 Root7a0cf28b、record e2ab9233；GEN-C 材料正式实跑 assembler189/0、assembler-cache2/0、identity15/0，每批 outer55、同候选、stderr 空。现有 seal/kind2 已核作业与 ASM 收据，尚须核包内模型正文消费及完整 N22；经典18 MODEL 未进 P3。 | `out/c43-classic18-asm-batches-01/`；继续 P3 真缺边，不重复已核收据。其余当前功能与宿主链见下一行；禁签/Draft/Latest/公开。 |
| 0.0.43 当前 SID 功能与宿主链（2026-10-27） | SID4fe5a4f1/Root7a/e2ab9233；24 条零材料正式批各自 PASS。HOST→三代的 18 个切片/wrap 阶段已执行，三代真 deflate APE=Root7a；链级 verify、正式批41/0、八拒绝批各自 PASS。活经典18 deploy-kernel 新材料在当前 Root 全域实跑：8770键／22694决策，拒绝及恢复28868、适配拒绝72，负控3/0。只授本机固定 P3 链和新权重 deploy-kernel 行为；经典18 MODEL 未进 P3，P3 NN/ASM 二进制输入、CC/libz、外国宿主、完整 N22/RI/流域仍开。 | `out/c43-classic18-zero-batches-01/`、`out/c43-hostseed-classic18-{context,stage1,stage2,stage3}-01/`、`out/c43-hostseed-classic18-registry-01/`、`out/c43-classic18-net-material-01/`、`out/c43-classic18-net-batches-02/`；下一步 P3 输入溯源、完整合同与同候选外宿主。禁签/Draft/Latest/公开。 |
| 0.0.43 当前同源两代（2026-10-27） | SID4fe5a4f1/Root7a/e2ab9233；context04 新 epoch 107/107。PC02/03/04 各自 seal/verify，GEN-B/PC03 与 GEN-C/PC04 的两核、五片、完整 APE 逐字相同，各自 kind2 准入 107/34、smoke16/0。旧 ctx03/PC01 和跨 epoch 配对具名拒绝。本机 P3 两代固定点成立；经典18 MODEL 未进包，完整 N22/RI/流域/五外宿主仍开。 | `out/c43-source-classic18-context-04/`、`out/c43-source-classic18-full-pc-{02,03,04}/`、`out/c43-native-kernel-classic18-{02,03}/`；下一步完整合同与同候选外宿主。禁签/Draft/Latest/公开。 |
| 0.0.43 最新开发根（2026-10-27） | SID4fe5a4f1 已开发安装，产品仍 Root7a0cf28b/2,954,593 B，build record SHA e2ab9233；同源优化 ELF 化解 `tbl-shared-e3 finish-job` 超时，未改验证合同。新 context03 107/107、full PC01 34 模型与六路/封包/复验、GEN 五片与真 deflate 包、kind2 候选准入均完成；安装后 smoke16/0。经典18 deploy MODEL 未进 P3 包，旧 e252 功能收据不转授新 SID；完整 RI/流域/N22/五外宿主未完。 | `out/c43-source-classic18-context-03/`、`out/c43-source-classic18-full-pc-01/`、`out/c43-native-kernel-classic18-01/`；下一步新 root epoch 的 source context、同候选功能批与域缺口。禁签/Draft/Latest/公开。 |
| 0.0.43 现行根与下一刀（2026-10-27） | SID e252e3fe、Root7a0cf28b/2,954,593 B 已本机开发安装。ctx01 的49作业、GEN28/PC01 与 GEN29/PC02 各自完成并验证；GEN28安装后 ctx01/PC01 复验仍绿；mixed 四端点及比较正式批全 PASS、比较64/0。新 Root smoke16/0、formats14/0、formats-objects21/0、对象九批198/0、identity15/0；image RI 单TU29/0、多TU15/0、错路由10/0、SC2 image PEM17/0。严格清单121批/132覆盖/19缺口；固定P3宿主种子链41/0、八拒绝批71/0；`--all` rc3、子批0。完整 RI/N22/流域及外国宿主资格未完成。开发专用 `tools/dev-run` 跨独立命令热缓存已验。 | 新根 `out/c43-native-kernel-riimage-winfix-27/`；两代 `out/c43-native-kernel-riroot-{28,29}/`、`out/c43-source-riroot-mixed-pc-{01,02}/`；正式五批 `out/c43-riroot-selfhost-registry-{01,02}/`、对象九批 `out/c43-riroot-objects-batches-01/`、RI 错路由 `out/c43-riroot-ri-route-refuse-batch-01/`、PEM `out/c43-riroot-ri-image-pem-batch-02/`。下一步补完整合同和同候选外国宿主；不签名、不 Draft/Latest/公开。 |
| 上期开发根快照 | SID718017ab、Root75a52ae7/2,934,337 B；kernel23、49作业、PC02、GEN25/PC03及双构建者安装记录保留为上期证据。smoke16/0、callback冷热23/0、18零材料批PASS；相位外墙钟5/0，identity15/0、assembler189/0、nets十八网全域、libneed-attributes12/0 分别PASS，P75 七流材料 streams40/0。其时清单104批/119覆盖/19缺口，`--all` rc3/子批0；现行状态以上一行及思维树首段为准。 | `out/c43-native-kernel-runreuse-23/`、`out/c43-native-kernel-runreuse-25/`、`out/c43-source-7180-mixed-pc-03/`、`out/c43-root-7180-validation-01/`、`out/c43-phase-timing-75a5-01/`、`out/c43-material-batches-75a5-01/`、`out/c43-streams-75a5-01/`。 |
| 源码／产品身份室 | 当前工作SID03a5e1b7...fresh核0（forward-result表修复014e1ac4...）；旧SID0d5923...是先前材料身份，cf2270仅header-only中间实测；scanner12/正规头再生实际0，PRODUCT_NOT_REBUILT。根仍已安装P53/S329，当前record21704e/native3d75；旧S447/e084保存在GEN13及checkpoint，原pkg epoch继续双hash核。B149构造/58/79实际0，当前GEN13 ctx1660/GEN14 ctx5cf8重wrap全0，原30 selfhost03实际58/0；旧01超时保留。 | 保存P53在out/c43-u64-checkpoint-01；不改贴旧root build record，不转授旧绿。 |
| 数学核修复室 | mask64返回/v/lit三处unsigned；独立CX exporter再生两C，39 NN输入及旧provenance不变，非NN参数重构。 | out/c43-table-kernel-math-01/installed.tsv记录实际Source329与两artifact SHA；单文件原子rename，非整对事务。 |
| 十八网机制室 | 新数学核由保存P53实际执行8770键/22694判定、28868字段拒绝及恢复、72适配拒绝全0；宿主shiftSan22694/0。 | out/c43-nets-u64-bootstrap-all-01.log、out/c43-table-kernel-u64-host-run-01.stdout；旧NN参数复用，不称新产品或全18源码重建。 |
| 高位永久回归室 | words9/0、all-nets8/0各原55秒；真实core函数原样抽取，独立四位型，宿主san无诊断。 | tests/table-mask.cx、out/c43-table-mask-{words,all-nets}-01.log；Source329经典C机制资格，宿主Cfoundation非hermetic。旧signed宿主run1保留。 |
| 新P3生产准备室 | CTX03全部107已执行：106 outer55 rc0/PREPARED；net-warnparse outer142/native0，result/record已写但缺终态，UNKNOWN/timeout仍不计完成。34组102个native步骤已实际执行，NN数值来自C源构造器，非经典18参数重构。 | gen13九phase全0；consumer56a的PC03 prepare/34copy/6routes/declare/pack/seal/verify全0，1588745B包SHA1c6a503...及完整manifest只PREPARED/VERIFIED。SPa8ae warning两端复验各53file/6job/1model约.58s，原producer142不改绿；67gate native O2编译0、outer55实际67/0（out/c43-source-job-snapshot-01.log），负控具名拒绝/清零/恢复0；仅代码缓存不缓存PASS。PC01declare142/PC02pack142保留；旧SP fullpkg双端0，每端1004file/107job34model约7.75s，四端约31s超selfhost30；当前64KiB/provider-v3 dd6/407的PC04 prepare/34copy/6routes/declare/pack/seal/verify全0，manifest b10f490...、receipt431639...；full-SP02两端1004file/107job34model约5.11/5.09s全0，新pin gate02实际67/0。旧ctx699候选A完整4911193B/SHA53e2...与Root同字节，version/smoke16/candidate58全0后归档12旧产品/收据；当前B context14954ce...五slice/wrap与smoke16/candidate58实际0，schema首72/1因测试BOM顺序预期错，独立前提增7后79/0；installer编译/旧P7拒绝无副作用0，实际安装55 rc0，Root329/record21704e已独立核0；GEN14九ASM及新RB699b/product68d5下两代五slice/wrap12phase全0，ctx1660 saved447/e084与ctx5cf8 Root329/21704；原30 selfhost03实际58/0，四SP107/34/1004与11后验完整。01超时/02未启动保留；fclose SOURCE_FIXED/PRODUCT_NOT_REBUILT：不变测试旧默认338/48失败，新-I include338/0；origin七源码树及Root/record已封存，typekw/10support封存、helper复算完整329实际0；capsule706MiB/19片及finalverify全0、SEALED87f...、helper两次329；GEN15九phase0/PREPARED、origin首snapshot0/107/34/1004；NI8bec 6/0为历史，新索引origin37/0、NI a95永久104/0；采用包prepare/pack/seal/verify/snapshot均0仅材料，RB3源码已写但forward-result表缺陷阻塞编译，未新slice；修表SID03a5已fresh核0、forward-runner01 native0；partialCTX/三工具/三NN材料已准备、34gen0且31JSONsame/3diff、union probe及永久36/0（生产者两net outer142仍UNKNOWN）/动态混合采用待做，installer与新候选未完；scanner12/完整旧头oracle/导出prepare/verify/install均0、最终SID0d5923已核，新产品未重建；autoinc双投影真实3/0、两include树/support四端稳定，完整libneed表同值仅必要条件；NN材料显式ADOPTED Source329，非Source0d新构造或新产品；原30/55不放宽，只材料PREPARED/VERIFIED、非功能PASS。 |
| 采用单端合同室 | tests/adopted-package.cx永久路径及原probe各原55实际15/0；七非突变负控和一次正端，controller bootstrap329/21704与旧NN epoch447/e084分列。construction.adopted-package登记SOURCE55有限批，非RBkind3或新候选资格；新登记后strict-list实际99批/114覆盖、执行0。 | out/c43-adoptedsnapshot-probe-run-01.log、out/c43-adopted-package-permanent-run-01.log；CLI ROOT PC FRESH_SID，旧30秒和四完整端点不减。 |
| pp独立参数室 | S447下六cube新142B构造92/0、配方控制23/0仍是有效低mask历史，未授Source329。 | out/c43-pp-{constructed,gold-controls}-01.log；.py文本依赖、SID旧weights与受信任bootstrap边界保留，全18/P3独立参数构造未完成。 |
| RI／库／属性／对象室 | P53/S447局部RI/P3消费者/库ABI/属性优化/objects等结果原样保留，不转授新源码。 | [Source447快照](archive/prd-c43-source447-before-unsigned-math-20261026.md)含原始范围、限时和日志；完整合同仍待新候选。 |
| 自举／原生构造室 | 旧P53的11/12固定点58、CTX02链41/拒绝71与旧ASM生成物是历史，不当Source329新来源链。 | 新源码兼容闭包不得靠删1241项或改贴SID绕过；新P3包与产品构建后再实际校验。宿主依赖、未签收据、root辅助来源边界保留。 |
| 清单与旧套件室 | 当前严格--list实际rc0（adopted-package-testplan-list-01）：99批/80实现/19planned/64unprepared，114coverage/95实现/19缺口，executed0/NOT_EXECUTED。最近实际--all原20 rc3为登记前113coverage/94实现/19缺口，97批/78coverage NOT_RUN、smoke16/0、子批执行0；旧98/113 list、96/111、95批/76coverage NOT_RUN及109/94/旧1333均保留历史。 | out/c43-adopted-package-testplan-list-01.log、out/c43-adoption-testplan-all-01.log；只有P53 smoke16，独立15日志不替代聚合执行；新源码不借旧绿。 |
| ujs 子项目 | 独立规格与进度只在ujs/prd.md及ujs/ARCHITECTURE.md；根不改写其代际或语料。 | [ujs/prd.md](ujs/prd.md)；底座.bootstrap使用与产品资格分列。 |
| 宿主 .cx 驱动 | `gcc_cx.com` 617424 字节，`clang_cx.com` 617456 字节，六目标 APE，不入库。模块展开走 `unisacc.com -cx-plan`，不另写解析器。本机 gcc 与 clang 15 对 lex/arith 逐字节相同，对 `tests/cxhost_main.cx` 退出码 42。产物不是已检查的 `.cx`。OSX 切片未实跑。Windows 切片含 `posix_spawnp`，只编译未跑。 | §5.3。`tests/cxinclude.cx`、`tests/cxapis.cx`。 |
| 资格与发布室 | Source329候选A已归档、B新ctx58/79已实跑，本地安装与GEN13/14构造及原30完整selfhost58实际0；fclose工作header修复已338/0，但新产品未重建，329证据不转授；19缺口、完整NN/ASM源码构造与同候选外宿主未收口。 | 不签名/Draft/Latest/公开；C44启动复用仅csr旁路私有APE原型，尚未回并或获新43资格，旧P53测量不报新提速。 |
| C44旁路协作室 | Root-epoch C44原型：非origin=v2+冻副本+sp_execution_record；origin=v1兼容；PC03 origin正端107绿；pin链清单已交cdx。 | C44 `out/c44-root-epoch-proto-01/`（含PIN-CHECKLIST与diff-vs-HEAD）；不改主热路径吸纳前；不签不公开。 |

Source447的原思维树、当前证据段与记忆表已原样存入[历史快照](archive/prd-c43-source447-before-unsigned-math-20261026.md)。此处仅维护当前源码、待办和证据入口。

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

七流真源：[exec/pipeline/image-stages.tsv](exec/pipeline/image-stages.tsv)；十八网真源：[unisa/gold.py 的 STAGES](unisa/gold.py)，金表位于 weights/gold/。逻辑完整输入与拆分方案保留在[归档身份义务](archive/testing-c43-legacy/tests/STAGE-IDENTITIES.md)，不作为当前执行入口，本节汇总现有物理管道及观测。

**逻辑边界不等于物理嵌套。** parse/type等逻辑阶段目前集成于e3，enc/image集成于elf，不声称已有独立产物。七流模型并非十八张ask事实网的简单一对一替换；下表消费关系含经典ask定位与产品归属，不能只因同名pp就把有限金表的叶证据当完整E2验证。流级封不推出内嵌每张网有独立封存。出货闸模块口径：**网格** `tests/netseal.cx`，**流格** `tests/streamseal.cx` + 叶内 `cell.tsv`（详见 `tests/SEAL-MODULES.md`）；e1/e4/prune/elf 已挂 streamseal，e2/e3/lower/pp 见该文「暂不合形状」。聚合测套 `./tests/seal-modules.sh`（闸 job `seal-modules`，contract 层，~25s）：manifest+x流×y网适用性检查 + 4 流封 + 17 网封回放；登记表 `tests/seal-modules.manifest.tsv`。

以下三表更新到 tip `25f720ab`（2026-10-21 17:33 AWST 观测）；近窗提交数量是当时记录，不是持续更新仪表。**有封≠已冻**：本次整理重验了被引封件闸（pp-leaf-seal(-negative)、exec-pp-literals、stage-contract-e1e3、lex-e1-seal、opt-e4-seal、elf-seal、prune-seal、middle-flow-e3/lower-accept、17×net-*-seal 各两遍，全部 rc 0），但过闸只证封件自身一致，不证封件范围覆盖了流的全部行为。

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
- 真源：`unisa/gold.py` → `STAGES`；README stages 表；金表在 `weights/gold/*.tsv`
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
- **单元与链接**：`-funit` 写单元对象（tape 带 `.global/.extern` 链接属性；对象里另有不装载的节存单元 tape）。`unisacc a.o b.o lib.a [-o prog]` 在 tape 级合并（src/tapelink.c：私有名改 `__u<k>_`、全局按名相会、同名全局对象按 C 公共定义合一、各单元 `__init_u` 串成 `__init`），再走普通后端，六目标都可出；`unisacc ar rcs|t|x` 读写系统 `ar` 也认的归档。产品驱动复用同一合并核心，编译决策仍全在模型路由。
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
- 原FX-1–FX-8细稿及旧测量完整留在[构想归档](archive/plans/ideas.md)，旧收官排期在[路线图归档](archive/plans/roadmap-0.0.x.md)。构想不构成产品/发布授权。

### 5.3 0.1.x：其他 C 编译器接入 `.cx`

**规划决定（主人 2026-10-24）**：在 0.1.x 制作共用 `.cx` 前端、构建驱动器及必要的编译器插件/适配，使 Clang、GCC、TinyCC 等工具链支持 `.cx`。具体安全语义以 [`.cx` 草案](docs/cx-spec-draft.md) 后续形成的稳定规范为准。`.cx` 继续采用 C99 语法，通过模块接口、契约和受检查的编译配置表达能力，不增加 Rust 式源码关键字。

**已落地的宿主驱动（2026-10-26）**：源码是 `tools/gcc_cx.cx` 与 `tools/clang_cx.cx`（实现在 `tools/cxcc.cx`）。六目标 APE 由 `./unisacc.com -run tools/build-cxcc-ape.cx` 写出，放到仓库根的 `gcc_cx.com` 与 `clang_cx.com`，不入库。五个切片是 `win/x86_64`、`lnx/x86_64`、`lnx/arm64`、`osx/x86_64`、`osx/arm64`；`win/arm64` 走同一份 x86_64 PE。Unix 上用 `sh ./gcc_cx.com` 启动，shell 按 `uname` 解出切片，缓存名是 `~/.cache/gcc_cx` 或 `~/.cache/clang_cx`，所以从 argv[0] 找不到仓库时要在仓库根启动，或设置 `CX_ROOT`。驱动把首行 `#!` 涂成空格。没有 `#pragma unisa use` 的单元仍一次交给系统 `gcc` 或 `clang`，参数是 `-std=c99 -x c -include <仓库>/include/cx.h`。有 `use` 时，驱动执行本仓库 `unisacc.com -cx-plan`，按每个单元自己的 include 列表分别编译再链接。解析器只有这一份。产物是普通 C，不授予「已通过 `.cx` 检查」的身份，所有权检查仍未实现。Windows 切片用 `posix_spawnp` 启动系统编译器，也用它抓取 `-cx-plan` 的标准输出；临时文件先看 `TMPDIR`，否则用 `GetTempPathA`。本机没有 Windows，该切片只编译进 APE，没有执行。本机有 `/usr/bin/gcc` 和 Ubuntu clang 15.0.7。OSX 切片只写出、未执行。草案中的版本化 CXIF 对象和编译器插件仍未实现；这一步用的是 unisacc 已有的 CXIF v0 文本头。

**构建收据（2026-10-26，模块展开之后重打）**：`./unisacc.com -run tools/build-cxcc-ape.cx` 实际 rc 0。`gcc_cx.com` 617424 字节，`clang_cx.com` 617456 字节，`file` 均为 PE32+ console x86-64。构建里 gcc 编出的 `ujs/seed/construct/lex.cx` 对 `ujs/tests/corpus/arith.ujs` 与 `./unisacc.com -run` 逐字节相同；`tests/cxhost_main.cx` 展开 `cxhost_mod` 后退出码 42。同一次构建里 Ubuntu clang 15 编过该扫描器，并编过同一夹具、退出码 42。`-run` 另测：用 gcc 编出的 `gcc_cx` 再编 `tests/cxhost_main.cx`，退出码仍是 42；缺模块 `no_such_mod_zz` 由 unisacc 具名拒绝（rc 1），没有交给 gcc。字符串里的 `#pragma unisa use` 不当作导入。`win/x86_64` 的 `tools/gcc_cx.cx` 只编译通过，没有执行。OSX 切片只写出、未执行。活树棘轮 `tests/cxinclude.cx --check` 与 `tests/cxapis.cx --check` 实际 rc 0：436 个 `.cx`，111 行 include，724 处无界调用。

**Windows POSIX／C99（主人 2026-10-26，对照 Cosmopolitan 的目录模板、临时目录和最终路径；未在 Windows 上执行）**：`include/stdlib.h` 的 `mkstemp` 在 Windows 上用 `CreateFileA` 的 `CREATE_NEW`。`realpath` 能打开的路径先问 `GetFinalPathNameByHandleA`（`VOLUME_NAME_DOS`），再去掉 `\\?\`，并把 `\\?\UNC\` 收成 `\\`。目录打不开，因为 `__open` 传不进 `FILE_FLAG_BACKUP_SEMANTICS`；这一路和最终路径失败都退回 `GetFileAttributesA` 加 `GetFullPathNameA`，退回路不解析符号链接。`mkdtemp` 在 Windows 上用 `CreateDirectoryA`，模式忽略；模板必须以 `XXXXXX` 结尾，`EEXIST` 会重试，其它错误和重试耗尽会把 `XXXXXX` 写回去。这些调用展成宿主通道宏，这样当前编译器的库依赖表还没记下新函数时，被调函数也不会被裁掉。本机 Linux 探针实际跑通 `mkstemp`、`mkdtemp` 和 `realpath`；`win/x86_64` 探针只编译通过。`fork`／`execvp` 仍不映射，也不移植 Cosmopolitan 的 Windows `fork`。`cx_run`／`cx_runv_timeout` 在 Windows 上仍返回 -1：不把 `posix_spawnp` 写进 `include/cx.h`。驱动源码在 Windows 上直接调用 `posix_spawnp`；临时目录先看 `TMPDIR`，否则用 `GetTempPathA`。

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

## 8. 历史收口记录（不作当前调度入口）

以下保留原先各拍的事实及边界；当前状态以§2动态思维树和记忆宫殿为准。旧记录的待办、计数与授权不直接用于当前调度。

0.0.43 F1 于 ffb1fdbf 完成：七补丁逐刀提交，657 文件差分（628 相同、29 经逐项审查）、双重封、seedmemory 键及根产品身份校验通过，新根与 cx.h 同锁落地主检出。F2 按现有 TP-9 整数拒绝语义补文档，并将两个已修 P4a 探针移入正式 C 语料；只在新路径实测通过后提高 ccrun 基线，不改历史证据。

F3 实测补充：k2b 的私有 tracked-only 夹具漏掉被忽略的根产品，迁移后的 volatile-comma 通过 cxbin 调用时无编译器。保留既有全部 K2b 断言及解释器实现，采用通用 shell 夹具执行器导出已提交树并显式带入同代根产品对；原调用点不增加解释器依赖。队列原红保留，修复后重新全扫。

K2b 后续实测：补齐产品后，原 N10 注入 unisa/vm.py 已不再影响迁移后的 C VM，断言诚实报红。只替换旧夹具的路径/注入文本常量（不增加 Python 逻辑、断言或调用点）：改为未声明的 include/cx.h 编译依赖。同时修 cxbin 缓存键漏记隐式前奏头的真实缺陷，使前奏头变更必须重新编译；K2b 仍须证明真实运行改变、漏跑标红及撤销。shell 夹具清除继承的 Git 定位变量并核验同代产品。

K2b 候选已在索引导出和隔离中跑完原完整测试（rc0）；只读文件事件采证保留两边相同 stdout 摘要及结果，N10 真正编译拒绝后撤销/void 全部断言通过。冷缓存的门禁清单曾撞现有 10 秒限时，使用既有 --list 工具独立有界准备后通过，未提高任何限时。仍需 commitgate 与 F3 新状态全扫。

### 0.0.43 F3 acceptance fixture repairs (2026-10-23)

The first CONTRACT sweep on root 4f0b493c completed all 187 jobs: 162 green and 25 red, with no pending jobs. Preserve that evidence. Its Linux cleanup fixture expected the superseded guest layout; update the fixture to carry the real guard and test current owned scratch cleanup while retaining native watchdog assertions and Darwin ownership UNVERIFIED 77. The new vsnprintf and fileno probes must leave scratch removal to its caller's guard instead of using raw unlink.

The pipeline cache fixture omits the guard tools used by both gen_inputs and real stamp.sh. Copy their existing tool/header closure into the fixture through a shared shell copier; keep production ROOT behavior and every cache assertion. A HERE-only production candidate was tested and rejected because stamp.sh still failed. The corrected fixture passed all original assertions. The scheduler fixture's deadline constants can be shortened while preserving admission margin, full/solo timeout settlement and booking serialization: nine numeric replacements, no Python logic/calls added, all assertions retained or tightened. It passed within the unchanged 44-second bound (35.121 seconds).

Progress must enumerate the current complete --com job set and its CONTRACT subset; a selected queue is evidence, not the complete F4 universe. Missing declared jobs stay pending even with a stray result or waiver. No new host waiver or ROOT interface change is introduced. Raw fresh-order and edge-seal failures need separate causality proofs before recapture.

The fixture copier's path set belongs in a plain data manifest: embedding the guard filename on a Python copy line is conservatively classified as a guard call by the deletion scanner. Use a shared manifest reader instead of adding a false deletion exemption/mutation row for a copy. The original actual deletion site remains guarded and registered.

### F3 fresh-order baseline attribution (2026-10-23)

All eight parse2 flag combinations now have raw sequence evidence. The seed constructor is byte-identical to the baseline constructor at b91fdffb. A private namespace with that revision's paired update-fresh.tsv/update-result.tsv reproduces each frozen baseline SHA and row count. The current tables add exactly twenty BC/b allocations (ten compound operators times f16b/f21b), with all other owner/kind order and sequential numbering preserved. This is the expected consequence of the already accepted bit-field promotion repair 0fc2428b, not the bare-enum repair d1d9380f. Refresh only the eight parse2 rows through the existing freshordercheck.cx --write interface, retaining units and proving the mutation negative against a matching positive baseline. No product table, constructor, assertion, timeout or waiver changes.

The nine fresh-order dependency declarations still name the removed checker file. Align them with the current generic CX checker, native seed constructor, recorder, generated baseline, product pair and constructor data trees. One C declaration loop describes all modes; the old interpreter command data disappears. The independently regenerated baseline matches all eight raw captures, units is unchanged, and a matching positive followed by the original binding-swap negative passed in a private export with the manifest restored exactly.

### F3 edge and boundary reseal attribution (2026-10-23)

All twelve rejected edge leaves now have paired replay evidence on the same source/probe/gold tables: privately mounting the original 8b82f76d include tree restores every frozen body byte-for-byte. Current headers change query counts, add one sizeof parser key, one sizeof scope key and six type keys, remove no keys, and change no answer for a common key. Pointer-equality queries also cause the reference parser's existing auxiliary '+' signedness lookup; its illegal labels are retained, not replaced. Re-record these measured surfaces with the existing deterministic producer and retain every negative gate.

The stage-contract e3 model pin is exactly the old 4fbfdad2 root's extracted model ac5f7703. The new 4f0b493c root extracts 2bbfe160, byte-identical to the native packer's encoding of this candidate's errorparse.net. Re-capturing the complete accept directory from the matching package leaves every file unchanged. The lower model remains byte-identical at 3f4c80e2. Update only the e3 model/pin and its provenance; keep both boundary fixtures' positive/negative assertions and all UNKNOWN cases.

### 0.0.43 F3/F6 实证收口（2026-10-23）

构建上下文1a26da09、F1最终根4f0b493c上，完整CONTRACT187项实际执行完毕：186 PASS、0失败、1 UNVERIFIED，原队列rc4。唯一未验证为bound的Darwin Terminal归属子项；本机退出、信号、派生进程预算及Linux清理断言通过。按终点线F3宿主例外单列，不增加全局豁免、不改写为PASS。完整日志、输入pin和现有出口表见research/c43-f3-final-evidence/。

F6正常候选构建、stage2/stage3及官方fixedpoint实际完成；seed、stage2、stage3三个实际文件SHA均为4f0b493c7a83d73a4ae1b348c8400ea3b7eda0ea190051b54a84c712a6730eda，完整产品收据一致，根与候选双ident通过。证据见research/c43-f6-final-evidence/。此为构建固定点，不代表F4或公开验收。

F4仍待完成。原全量队列前11项中3 PASS、8红：三个arm64大闭包及准备步骤获25秒而超时，两项前置失败、两个seedparse2分片缺memory证据；不扩大R11或覆盖原红。真实profile定位指纹重复JSON/路径规约/哈希开销；原生原型旧哈希对照和纯数据基准通过，但UTF8拒绝和FD继承仍需补证，未接入生产。原型只留scratch out，不增加Python逻辑。当前刀只落F3/F6证据与仪表板F6行；后续正式队列按新输入重新建state，旧结果保留。

### F4 原生 stamp 接线裁决（2026-10-23）

在同一4a7be9b1输入上实测1118项身份采集：原实现12.4768秒；C原型完整函数含析构3.6886秒。去重开关两侧1118个新协议摘要逐字节相等，逐文件parts保留。历史真实JSON的旧SHA及Unicode/浮点/整数对拍、回调引用、异常传播、heap type GC均已实跑通过。

本刀只将既有stamp算法迁入C，保留原收集与K2b审计逻辑；Python仅加载模块、绑定callable，不增加Python缓存或另一套算法。数组身份统一使用版本化Merkle域，scope仍由原组件数组与parts字段表达；reviewed inventory字典保留旧哈希。对象与可达值在一次采集期间不可变，C缓存强持引用，浅字典去重必须完整比较键内容与值指针。

采用每个解释器进程新编译一次宿主适配器，不做跨进程编译缓存；实际适配器字节和固定源码闭包进入common身份，临时输出路径不入身份。额外依赖是匹配的CPython开发头与宿主cc，Linux/Darwin支持，未证明脱离Python运行时。编译输出只留在受限scratch缓存目录，不未经守卫删除。接线后需原有K2b、队列、诊断及完整F3/F4重新验证；旧红和旧state保留，不批量重盖摘要，不放宽时间或扩大R11。

N10受控漏声明负例改用自带stdio.h：新适配器真实读取cx.h，后者现为common输入，不能再把它冒充未声明依赖。隔离单文件挂载#error到自带stdio.h已实际使原volatilecomma编译具名拒绝，宿主适配器仍用系统stdio；仅替换原测试的一个文件名常量，所有漏跑RED、撤销与保留断言不变。

头目录由导入解释器的sysconfig.get_path元数据直接交给宿主构建器，不在shell猜测CPython安装布局；构建器不调用解释器。未新增或扩展Python算法。

原生门禁控制器的构建参数来自解释器安装自带的 shell 配置工具；它不执行解释器。G5 对该工具名称的三处命中以具名例外登记，零 Python 台账不变；不扩展为任何解释器调用豁免。

F3 新轮的 K2b 独占 44 秒预算超时；原断言独立 profile 全通过，主体 38.484 秒，其中 17 次纯结果比较子 CLI 占 17.442 秒，19 次实际 fingerprint 共 3.530 秒。纯 `--k2b-compare` 分支只读取和裁定已有两份 state，不构造新指纹，且全部路径在该分支退出。仅对这个直接 CLI 入口跳过原生模块构建绑定；模块导入、API 控制器与真实队列仍每进程冷编、保留实际二进制身份。无持久产物缓存、无新 Python 算法、无比较断言删减和限时调整；新提交需新队列，旧超时原样保留。

F3 完整复验发现 F6 原始步骤表的十四个临时 manifest 路径被减法安全检查当成活依赖。原始表以确定性 gzip 无损保留，解压字节及其原 SHA 不变；README 说明历史路径语义，摘要表改验压缩文件。不改检查器、不豁免悬空引用，也不改二十四个步骤证明或产品。

该步骤表原件受到删除守卫保护，删除请求已被拒绝且未删除任何内容。原文仅重命名迁入 scratch 的 out/f6-evidence-packaging/steps.original.tsv 永久保留；压缩包解压逐字节一致，不绕过守卫执行删除。

F4 ca5b3603 的 carrier import 准备在完整 41 秒预算下超时；独立原命令 strace 55 秒诊断也超时，原记录保留。跟踪及只读审查确认 consumer 调 build_ref.sh 编译了未使用的 ref，随后只读 ref.c 改成 dump.c；实际消费者使用 dump/run 八项缓存输出。为现有 build_ref.sh 增加显式 --source-only 第三参数，沿用完全相同的源码拼接并在编译前返回；consumer 只增一个该选项实参，不新增 Python 算法。保持默认构建、两次完整身份一致性核验、八输出/锁/原子安装及全部消费者断言；节省是否足以通过原预算以实测为准。

source-only 冷准备在原 55 秒看门狗内实际成功；具名 gate 的冷准备也成功（51 秒），但首次并行消费者抢在缓存安装前返回 NEED_PREPARE，保留原失败。缓存身份包含 UNISA_JOBS 等完整执行环境；gate.sh 默认注入 2，直接 CX 与正式队列须用同一冻结环境预建，不能串用不同键。采用既有发布流程允许的有界预建：先在队列外单独 bound55 完成缓存，fresh 全量 F4 仍执行原 prepare 的完整 HIT/产物验证及两个真实消费者；旧正式 41 秒超时不改写，也不声称冷建41通过。

### 历史：F4 snapshot 与优化夹具修补（2026-10-24）

正式2d冻结队列的原红保留。本批只修两处测试夹具：snapshot写失败用普通文件父路径代替只读目录，令所有uid都触发同一具名写失败；优化对拍对七个明确转发探针核完整Tape具名拒绝后，仍实际运行两优化级别，并同时要求原始子进程状态及看门狗自身退出码为零。其余语料非零比较、全部原断言、分片和限时不变。不改产品源码、资源裁定或根产物；独立archive已有正负控，落地以本次索引门禁实际结果为准，后续新正式队列不得借旧绿。

### 2026-10-24 政委：解释器退出期限与本刀边界

不新增 .py；旧 .py 在 0.0.43 推进迁到自有 .cx，最迟 0.0.44 完成退出。准备流程迁到 .cx 而仍调用旧参考生成器，只算流程迁移，不能记作完全脱离；台账1788/92/411只降不升。当前先完成 F1→F6，模型对象闸优先直接移植到 .cx，复用现有通用 Net 包装模块和数据配方，不再添加解释器消费者或缓存算法。

- 2026-10-24 exec-chain 有界预建落地决定：通用 cxlib.seedprep 执行器准备默认模型，11 项输出逐字节等于原配方；每个消费者仍执行三次网络校验和全部源比较、known/revived/keep 断言。动态看门狗截止时刻只排除缓存身份，仍传给全部子进程；保留真实环境和工具输入。无新增 Python 调用点；既有生成器迁移遵守 0.0.43 优先、最迟 0.0.44 的政委期限。索引门禁不能代替最终全量 F3/F4。

- exec-chain 索引首次九项闸八绿一红：新 .cx 的旧 stages 标识与 stage_environment 键违反术语规则。改用 nets 与 construction_environment，不加豁免；身份变化后重新冷预建和九项实跑。原 v6 日志仅证明术语修正前候选，保持原证据不冒充新源码。

- 2026-10-24 cxbin 缓存闭包缺口：原键仅绑定主 .cx 和头文件，未绑定 use 模块正文，直接或传递模块正文修改可错误复用旧二进制。以原产品 -cx-plan 枚举全部编译单元，逐个绑定实际文件完整 SHA256；计划、缺失文件和哈希失败均拒绝。保留 safedel 的系统 cc bootstrap，不依赖产品计划；不改核心或 F6 产品。

- 2026-10-24 新增 exec-chain-model-prepare 的全量枚举实际1120项、CONTRACT188项；gatelayers --check 实际红因 stagemap 漏映射。该项仅构造默认模型输入，精确归现 build 轴；五分片完整对拍仍归 chain。只增加一个精确名，不扩通配或改层级口径；原红与stderr保留，重新实际 gate-layers 验收。

- 2026-10-24 modelobject 原 Python 闸拟由自有 .cx 消费者替换，保留两架构、unit/whole 的22对象逐字节比较与六网络全域校验；八份构造输入独立预建，不缓存通过结论。只有独立原配方21项字节证明、负控和新索引实跑全部完成后才落地。原 .py 保留完整字节和SHA移入 out/retired-source 后从跟踪运行域退役，非改后缀冒充移植；原冷40超时仍保留。构造仍调用旧生成器，只记消费者/包装迁移，未宣称完全脱离解释器。

- modelobject 首次derive真实拒绝：把原配方对拍脚本复制为新增research执行器会新增18个解释器调用点。撤销该新增执行器跟踪、将副本原字节保留out，仅提交原路径与SHA、真实执行日志；不改后缀、不改扫描规则。原消费者退出本身应自然下降，以下以重新derive真实结果为准。

- modelobject 首次14项索引闸13绿，消费者内命令30s红；同index随后直接bound40热跑22对象0。实际-cx-plan与cxbin trace证实：相对准备入口与消费者绝对入口的三单元路径/缓存键不同，导致消费者重复冷编译。sp_init只设ROOT不chdir；sp_spawn子进程会chdir ROOT。最小修复显式checked chdir(ROOT)，准备源统一相对tests/modelobjectprepared.cx，保持绝对wrapper、完整路径/文件SHA闭包及30s预算；不能仅用热0当修复，重新八份独立构造/21cmp和14项闸。

- 2026-10-24 消费者监督清理补刀：modelobject cmd外sp_spawn原30会早于内bound30既有N+2清理截止杀父，挂起setsid后代存在漏清风险。本刀仅外监督32.25=命令30+既有清理2+.25调度余量，命令/整suite预算不变，通用bound/seedprep不动，须真实30秒父孙挂起负控与PID消失证明。exec-chain已有cd R，prepared源也统一相对以复用job同入口；保留完整路径和源SHA缓存键，原cxbin修复前340证据不能代替新树验收。

- 2026-10-24 unitparse 消费者迁自有 .cx：原wrapper三个fresh及b58/三参数顺序不变；保留5个unit的reference/simulator/native三方逐字节比较与whole-fib原simulator/reference比较、原10/30预算和步限，不虚构whole native新增验收。候选已实际正控、ref末尾字节扰动与坏Net拒绝/恢复、原包字节等价；原Python仅原字节移动保留out后退出运行域。仍调用旧构造器，不能宣称整条链脱Python；索引独立预建和正式具名闸另采证。

- unitparse 首索引六项5绿，消费者16s真0；唯一G5红是HANDOFF新增复制原生成器调用文本（该文档在冻结术语baseline外）。改为引用现有wrapper三个fresh原声明与次序，不再新复制解释器可执行声明；不改运行代码、扫描器或白名单，原红日志保留并重跑原六项。

- 2026-10-24 F4 R2 precommithook 实际首红：自测夹具只复制分类器与cxbin、链接同代产品，却没有include目录；原始err明确缺include/*.h，新cxbin完整SHA闭包因此正确拒绝，未到紧急跳过的日志分支。决定仅在夹具基础提交前复制真实顶层头文件，保留hook分类先于跳过、cxbin严格失败传播及全部既有断言；不以旧缓存或宿主裁定掩盖。原R2红原样保留，索引与新树验收须分别取真实证据，不能用原红或旧通过结论替代。

- 2026-10-24 F4 主检出诊断复验：precommithook rc0；unitparse 六项约束中的 zeropython、docs、cxinclude、glossarycheck、gate-layers 均rc0，exec-unitparse 经既有 fresh 协议准备后rc0（17秒）；modelobject 八份准备及消费者均rc0（消费者41秒）；exec-chain准备及五分片均rc0。这些运行发生在主检出，只作诊断，不是独立索引或正式F4验收。正式证据必须按HANDOFF在冻结scratch和隔离命名空间重新取得。旧冷路径60秒超时保留。

- 2026-10-24 恢复到真实F4现场：冻结scratch的2db2366e队列保留1128项、env-pin及前20窗。K2b首红为私有新路径冷编译seedconstructcheck的--list导致原10秒plan超时；采用已留scratch候选，将45行part/manual数据集中为TSV，轻量读取器核原始SHA及派生模块逐行相等，CLI从派生模块读取。补派生检查与具名闸后再落地。五个种子测试仍缺本宿主内存准入证据，另有优化分片超时；不以主检出诊断覆盖冻结队列结果。

- F4恢复环境核对：新会话继承en_US.UTF-8，而旧env-pin为C.UTF-8；语料排序和分片因此不同。第21/22窗的原日志保留，仅作诊断，不纳入旧队列验收；后续冻结运行显式恢复LC_ALL/LANG/LC_CTYPE/TZ及其余钉住变量。metadata冷枚举对拍只规约导出ROOT路径并扣除新增具名闸，其余原计划要求逐字节一致。

- metadata冷枚举修复已于b36d4bad29经隔离index提交闸落地：七项全rc0（含原始K2b全套，38秒），派生检查通过。完整冷计划4.82秒，原有NUL计划在规约ROOT并扣除新增seed-metadata闸后逐字节相等，原10秒限时保持；无效源数据使枚举rc2。现全量1129项、CONTRACT189项；后续正式F4按新源码建立fresh状态，既有五项内存准入UNKNOWN与旧红均保留待结证。证据见research/c43-seed-metadata-evidence/。

- 内存证据采集原型的UTF-8首红已定因：seedparse2check.sh的ROOT相对产品引用使文本actor闭包递归到unisacc.com；其PE头第797字节为0x86。隔离导出中直接调用既有Python actor_closure，仅以该shell为种子，复现同位置UnicodeDecodeError；原生seedprep按同合同解码全部actor，故同样拒绝。这是采集原型把二进制送入文本actor机制的接口错配，不是内存准入证据。生成/产品字节须另作原始文件身份绑定，文本actor与二进制叶分账的候选方案待实现；暂不改通用闭包或准入判据。

- 本宿主资源补证的更小切面：先在显式清除现有ENV_KEYS覆盖变量的隔离环境使用系统GCC11（SHA fb60c5945ce785d0cb2ef0303dac5249f25bd0d0324317a0734aae6aa24f609b）；已有Python构造器在该环境导入json/zlib实际成功。拟复用现有seedmemoryevidence.py的逐路线C/P trace与直接C RSS协议，先采小路线正控，再补全38路线的114份记录；不新增解释器算法，数据齐备前不更新准入常量。采集与正式队列分时占用重活令牌。COM构造/生成路线仍是独立未测义务，不能由这114份C路线记录推成通过。

- 清环境正控实测：GCC11在-w/非-w原两组flags下编出的生成器逐字节相等，SHA cd3f947fa601cf17c789a4c0bf63a75f66057ce12ac66792ba07b7face20fc90。E4的C直接RSS、C trace、Python trace三项均rc0且输出SHA同为fd3cac964a1eb608f76401dc45da44c0c4392d0e69519c20e7ef3f1fc7e94b7e；直接C RSS 41280 KiB（含pre-exec residency）。8GiB cgroup上限只作控制，不当峰值。三份记录只完成114份中的3份，尚不授内存准入。

- 种子缓存协议缺口已实证：当前seedgencheck.sh仍按旧glob内容的SHA1前缀写Python缓存（本树28a01f2255ab8b0d），准入器按seedmemory-reads.tsv路径与内容SHA256查找（同树1aba99c3d52243aa）。即使参考已生成，生产/消费文件名仍不相交；后续应让生产端调用现有统一gen键函数，补真实热缓存与变异拒绝验证。seedparse2现有生产端已用统一键；不通过搬改准入判据掩盖该缺口。

- gen缓存键候选已改为调用现有seedmemorykey.cx，并严格校验gen族与16位小写十六进制；隔离实测使用采集中真实E4模型作热缓存，当前键1aba99c3d52243aa命中，修改实测read-set中的finite_rules.py后键变为96368058c8762335且缓存未命中，删除该输入后rc非零且stdout为空。此为键协议验证，不计seedgen全套通过；准入常量仍保持原值，完整资源采集独立继续。

- F5材料校正：以F1/F3/F6已提交收据更新r43说明中的候选SHA、固定点、187项CONTRACT基线与已完成台账；将已落地的cx模块实现从“只是设计”改为现状，并补随候选交付的库/读文件修复。F4全量、封存、六格候选CI与Darwin验收仍待最终收据，说明继续明确标为待验，不用旧CI绿色代替本候选证据。当前用户goal明确要求完成0.0.43并公开发布；后续发布操作仍须先完成现行验收。

- 清环境资源采集首红停：81份原始记录中80份OBSERVED，gen/warnparse的Python trace在原45秒限时rc142（45.13秒），后33项未启动；不是完整114份证据，故不更新C/P准入常量。失败、已有输出和控制收据均保留于私有采集目录，需后续在同合同下补证；不提高限时、不把部分采集宣称COMPLETE。缓存键修复d44fd7f567与F5证据材料ec631233fe均已通过隔离提交闸；准备按此新源码重建F4状态，旧1129计划已完成60项仅作旧基线收据。封存工具oras1.3.5已下载并通过上游SHA256清单核验，尚未上传。

- M5/M6文档校正：保留原2026-10-23、产品3b17e191采证全文，前置当前4f0b493c候选的证据与待验边界。F2已完成两项array-pointer探针迁移及新产品ccrun/chain验证，TP-9与Tape README下划线拒绝也已对齐；F3 latebind-p6的原日志实SHA与清单及result.out_sha三方相等，证明26源/130产物的有限兼容基线，不能推成ordinary/data全部迁移或typed/callback产品迁移通过。当前F4尚在跑，具名库、平台及拒绝域重验仍待完成，不以历史GREEN补绿。libunisacc.c的Linux _GNU_SOURCE源修复已落实，旧wrapper决策归历史。

- F4准备债务的独立实证（2026-10-24）：保留原warnprep三项41秒超时及七项前置阻塞记录；在同冻结产品和环境中，未改生成器的独立PyPy参考与C构造器对warnparse/errorparse的JSON、table、net逐字节一致后，经现有compilerpack入口准备缓存。三项原warnprep在原41秒内真实HIT，七个原消费者各在41秒内执行全部断言并rc0。此为修复后独立重跑证据，不改写原F4状态，也不补成原CPython资源采集通过。E4原77探针保持顺序拆成39/38，两组原41秒实跑均rc0、equal78/76、bad0；另一原63探针组的solo41超时后，顺序拆成32/31，两组同限时实跑均rc0、equal64/62、bad0。各分组仍按完整终验复核，不以独立试跑替代F4。浮点测试的原参考已全166断言通过，C准备模型/宿主预期五文件逐字节相等，但第一版C消费者55秒超时，显式准备阶段第二版已在两个55秒准备界内成功，原41秒消费者完成166断言；与未改Python参考的166行rc/stdout/stderr逐字节一致。身份变化、缺失、腐坏、symlink四负例各返回NEED_PREPARE75，原文件恢复并核SHA。最终源首部注释整理后，已再次编译、两个55秒准备成功、原41秒消费者完整166断言rc0；派生元数据及索引提交闸与代码同批核验，不据此宣称完整F4通过。

- 索引提交闸首轮仅术语G1拒绝decimal测试中局部变量与临时目录的裸stage名称；按既有术语规则调整命名，不增白名单，保留首红并重新核验最终源的准备身份、原预算及提交闸。

- F4后续超时定位：E5原solo41与独立同限时sh-x诊断均保留；诊断已完成30正例、16拒绝例及首个运输目标，不能把静默误判为构造慢。测试侧候选固定原脚本SHA并逐字节重建原块，只按具名阶段执行原脚本；原Python调用仍存在，此为流程拆分，不计为Python运行时退出。v1正例/拒绝/真实编码与执行阶段分别41内通过（含550浮点执行用例bad0），六目标运输单块仍超时，v2五阶段均在原41秒内实际rc0，正例ok30、拒绝ok16、运输目标3+3各ok3、完整真实编码与550浮点执行ok2/bad0；旧阶段名、源SHA变化、内嵌NUL、缺失源四负例均在执行原shell前rc1拒绝，原源SHA恢复。E4其余61源组独立原41通过、equal122/bad0，保持该组原样；整源self的O1全比较成功后，在O2执行器阶段达到41秒；独立双level候选O1在41内通过、完整整源equal1/bad0，O2仍真实41超时，故该候选不落树，需定位单level执行与准备开销再决定修复。不改变原失败、不扩大H1/R11、不提高时限或切碎被比较的输入。

- F4分类与O2诊断更正：当前main与冻结树的七份分类输入逐字节相同，匹配原Python/宿主环境的现行exittable确认exec-e4self为release/rulings.tsv既有TIMEOUT/H1的RULED_BASELINE，exec-arm、exec-winlower同类；保留原41秒红，不新增裁定或记PASS。先前把self作为新未裁发布阻塞的判断遗漏了该行。对六份冻结输入再次核SHA后，原C执行器独立55秒诊断在48.48秒rc0，完整5227535字节与已生成参考逐字节一致（SHA134a71d8e6752d162edd4043e5a81b65422aca6dd86497d7570862bc6b216829）；只证明本例慢终止，不替代正式41秒验收。模型摘要化/观测方案仅保留为长期性能债务，不在本版为它改产品S。
- 旧冻结队列的新增未裁项exec-armlower已由033a45f946的两目标完整检查修复；其余六项原未裁超时已由e2837e32d0的具名修复覆盖，七个原BLOCKED消费者独立已通过，仍需新冻结树完整终验。armlower原41秒sh-x诊断已完成Linux完整检查（含125010 mov行及超过1000000 ARG slots），在Darwin完整检查阶段超时；按原固定SHA完整原字节分成两目标后，Linux/Darwin两个完整检查各在原41秒内实际rc0，均保留完整参考和大规模输入检查；四个参数/源码拒绝负例rc1且原源SHA恢复。资源组继续UNKNOWN/未执行，不改常量。

- 本轮资源身份缺口按既有preauth整组处置，见research/c43-resource-disposition.md：保留RESOURCE_UNKNOWN与未执行义务、两次原限时测量失败及不完整80/114观测，不改准入常量或分类匹配。最终完整退出表才确定组成员及计数；M7仍要求无pending、无未裁产品红及当前各项处置，后续六平台CI/签名/公开验收独立执行。此为既有分类出口记账，不宣称全量本地PASS。

- 仓库卫生纠正：已用自己的 `-run` 只读核对本轮14个证据目录，共569文件/1569021字节；其中大量为临时日志、trace、状态与生成副本。单一后台子代理已完成15目录（含本轮新快照说明）的清理：576降至67文件，移出509文件、净减1229131字节；必要结果与发布身份收据保留。完整原始材料已仓外归档逐文件核验，公开旧树可由033a45f946历史提交恢复，不另提交大过程清单。原始历史提交继续可追溯。整目录归档A批859文件的引用安全闸报告536项（含真实旧引用与模板同名误判），完整补丁及逐件备份已仓外保留；本批已恢复原布局，后续接线和工具缺陷登记到[0.0.44计划](plans/v0.0.44.md)，不再加入本版发布前置。已提交的509过程文件移出保留。

- 原生工具补刀：主人再次强调用好默认 `-run` 动态执行、动态前进，停止用临时Python编辑或编排；自身缺陷登记到[0.0.44计划](plans/v0.0.44.md)后续修复；后续默认直接用 `unisacc.com -run FILE.cx -- ARGS` 动态执行工具并推进；需要核验落盘产物字节、自举固定点或发行文件时才构建对应产物，避免把先编译可执行文件作为日常工具默认步骤。启动器测试的目录快照改由自己的 `.cx` 遍历、排序、记录模式并计算文件SHA，消除该内嵌Python调用；保留完整快照和拒绝用例，错误须显式失败，已用 `-run` 实跑完整启动器测试与独立快照格式/权限/Unicode/写失败用例（各55秒内rc0），实际减少一个Python调用点，证据见research/c43-queuesnapshot-evidence/；输出用逐次检查返回值的原生write，实测拒绝写满设备，避免仅靠stdio返回值声称拒绝。队列启动器本身的完整事务迁移尚未验证，生产入口继续如实计为Python依赖，未完成候选不切入。

- 最终本地终验冻结准备：以033a45f9467bc8b3331b51a59443c80cf4a7ab38作为release/freeze.tsv的显式冻结起点，以本版修复与清理收口后的最终干净提交作为终验源码HEAD，实际完整SHA绑定在终验输入收据中。当前完整计划1138项；产品源S仍c4618639，候选及固定点仍4f0b493c，使用候选自身7298侧车。本轮从全新状态和真实环境pin启动全量终验；旧62aa49的292/1129局部队列及原红独立保留，不迁入新绿。分类出口及资源未执行义务仍按本轮明确记录，实际终局和外部CI尚待验证。

- PE终验收口修复：旧1138项冻结队列在第44窗口停于302项已有结果（其中10项非零）、836项pending；新增未裁定exec-pex86在完整solo原41秒超时，原记录保留，不能作H1或PASS。原41秒sh-x诊断已定位到forwarding阶段。测试侧按固定原脚本SHA完整重建并分为base/forward两阶段，x86_64与arm64四个完整阶段已分别在原41秒内实际rc0，保留全部原Python调用、参考和断言、清理与继承deadline，不缓存verdict。旧结果不迁入新绿；新计划预期1140项，仍待实际枚举、派生及新冻结终验确认。产品S仍c4618639、候选H0仍4f0b493c；本轮仍未发布，资源OPEN义务及后续CI/签名/公开验收不变。
