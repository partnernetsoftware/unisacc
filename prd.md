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
| 从 tape／tapebin 写目标镜像 | `./unisacc.com INPUT.tape -b OS/ARCH -o OUTPUT` | `.tapebin` 也有现成入口。支持域及具名拒绝仍生效；见 [README](README.md)、[驱动](exec/c/compiler.c)及[现有 tape 对拍](tests/bindingsnegcheck.cx)。 |
| 写六目标程序 | `./unisacc.com FILE.c -b OS/ARCH -o OUTPUT` | 目标为 `lnx`／`osx`／`win` × `x86_64`／`arm64`；跨目标写出与目标机器原生执行分别记证。 |
| 查看 `.cx` 模块依赖计划 | `./unisacc.com -cx-plan FILE.cx` | 复用现有模块入口与[模块用例](tests/cxmodcheck.cx)，不另造模块扫描器。 |

新动态逻辑优先由 `unisacc.com + .cx` 完成，减少 `.sh`，不新增临时 `.py`。旧脚本尚未迁移的部分如实记为依赖，不把 `.cx` 转调脚本记成迁移完成。经典 C 参考与模型产品是不同实现；对拍前确认实际两端身份，不能笼统判成“对自身验证”。入口已实现不等于任意输入已验证，继续保留原断言、拒绝和预算。

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

## 1. 命题与产品

- **产品**：一个文件 `unisacc.com`（APE），在 {Linux, macOS, Windows} × {x86-64, arm64} 上运行，自己写出六个目标的可执行文件（ELF、Mach-O、PE；映像不经汇编器和链接器），0.0.17 起也写三种格式的可重定位 `.o` 并自带链接器与 `ar`（§3.15），并能自举：宿主构建的 `unisacc-seed.com` → 种子自建 → 再自建，三个 sha256 相等（N22）。两代产品只叫 `unisacc-seed.com` 与 `unisacc.com`。
- **语言**：C99 = ISO/IEC 9899:1999 + TC1–TC3（WG14 N1256）。覆盖度来自逐条款账本 [tests/c99/clauses.tsv](tests/c99/clauses.tsv)（门禁 `c99-ledger`，README 表由账本生成），不作主张；缺口在 README 限制表逐条点名。
- **方法**：编译器里每个表状决策（词法类、预处理指令、优先级、类型、指令选择、ABI、窥孔……）由**构造**出来的小整数网络回答，权重从真值表派生、在表的全部定义域上**穷举验证**（不是训练）；结构性粘合是经典代码。两条路线：C 参考编译器（`unisacc.c` + `src/`，行为裁判与回退）与模型产品（`exec/` 各阶段网络 + 通用执行器），逐字节对拍（`exec-chain`、`closure`、`stages`、difftest 两侧）。
- **零 Python 依赖（硬目标；主人 2026-10-24 再次重申）**：构建、闸、验证链中 **100% 不依赖 Python**，牢固使用自己的 `unisacc.com + .cx` 体系；新增与修复路径优先原生实现，不新增 Python 算法或调用点。自举已完成（N22，三 sha256 相等），但依赖清零尚未完成：本轮 `tests/zeropython.tsv` 记调用点 **1779**（真实执行 1747、数据 32）、`import unisa` 文件 **92** 个、`.py` **409** 个。仅用 `.cx` 分批调用原 Python 只算调度改进，不算消除运行依赖；原生替换须实际自举运行，并保留完整行为与拒绝、恢复合同。三者同时清零后，`unisa/` 才移入 `archive/`（不删，仅作人工参考，不参与任何链）。
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

<a id="work-tree"></a>
### 思维树

**目标：完成 unisacc 0.0.43 并公开发布。** 本节按实际进展原位动态更新，替换过时节点，不把过程不断追加到文末。当前用户目标已包含发布授权，跨仓 Apple 服务接入另待具体确认。2026-10-24 实际核对 Latest 仍为 v0.0.42，本版尚未公开。

```text
0.0.43 公开发布
├─ 已完成：F1 产品修复；F2 文档与探针收口
├─ 已完成：F3 历史 CONTRACT 基线（187项：186 PASS、1宿主未核）
├─ 已完成：新F6三实际SHA相同且官方固定点通过；stage3复用同身份模型包，五代码切片均由stage2实际重编，原50秒界通过
├─ 正在执行：F4／M7 测试调度重构与受影响单元验证 ← 当前节点
│  ├─ 当前产品冻结d09bd1decb：1309节点／944依赖，计划SHA b6c80dbd…；Windows完整消费仍待原预算通过
│  ├─ 已实跑：七流独立工具链；native六叶；functionimports六准备＋20案例；callback12目标＋四片消费
│  ├─ 已修复：typed/callsite完整材料命中及无关检查脚本触发失效；普通七流／native资源缓存拆层仍待做
│  ├─ E4块摘要修复与新候选固定点已通过；完整M7仍待完成
│  ├─ J9最小缓存实验输出一致但ISA慢20.27%，不合入；当前刀为原候选E4重封与Windows摘要批处理实测
│  ├─ 旧1142轮已停窗；原失败与未执行项保留，准备仅计PREPARED，不迁旧PASS
│  └─ 完成判据：最终模块清单完整、PENDING=0、NEEDS_RULING=0、product_red=0、处置齐全
├─ 待完成：据终态结果收口发布说明 → 候选封存 → RC
├─ 待完成：匹配候选的六平台 CI、Windows 原生与 Darwin 证据
├─ 待完成：Apple／Windows签名、公证及最终字节验证
│  └─ 独立外部待答事项：minicon Apple 服务具体接入授权
└─ 待完成：Draft资产核对 → Latest／公开 → 下载SHA与运行复核
```

**下一动作：J9私有缓存实验负收益，不合入；重封当前E4 seal，实测Windows控制器摘要批处理，再推进同身份M7。** 普通七流模型和native附加资源缓存拆层仍待做。旧`.net`在固定8,446,684字节输入上触41秒上限；私有采样约9秒记录1.41亿网络决策／1.74亿动作，热点为重复SCAN。d09bd1decb已提交块摘要复用：同输入通用C执行器约30.5秒完成，5,227,535字节与独立C输出逐字节相同；八个具名案例对拍通过，实际ISA内核同输入约33.3秒通过。新候选构建、hello／O2回归、三阶段实际SHA与官方固定点均通过；完整M7仍待完成，不迁旧M7绿。不得以表参考超时推断纯NN速度。控制器-O2草案保存在仓外，暂停推进；NN、表参考、准备／校验各自记时与失败归属。新`.cx`控制缓存、依赖和状态，旧独立对照留在原入口；不新增解释器依赖或绕过门禁。已通过的模块及证据见下表；尚无通用同身份续跑，不能把模块选择称为完整结果缓存。未知闭包禁用缓存，RESOURCE_UNKNOWN保留OPEN，H1超时不算PASS。完整M7后仍须候选封存、匹配候选的六平台CI、签名及公开下载验证。文档清理首批3份旧稿已独立提交；过时过程留在Git与原始证据，不再堆入当前上下文。


<a id="memory-palace"></a>
### 记忆宫殿

| 房间 | 当前事实／决定 | 证据与行动入口 |
|---|---|---|
| 入口厅：目标 | 公开发布0.0.43；当前节点F4/M7，不是重做F1。 | 上方思维树；[范围](plans/v0.0.43.md)、[终点线](plans/v0.0.43-finish-line.md)。旧未授描述不覆盖当前明确目标授权。 |
| 工具间：能力 | 默认-run、tape→image、六目标、`.cx`模块已实现；经典UA与模型产品是不同实现。 | [能力速查](#implemented-usage)、[README](README.md)、已有用例。只验证具体输入的证据缺口，不重新研究入口是否存在。 |
| 材料室：重复构造 | d09bd1decb修复typed/callsite完整缓存仍重建，以及无关exec检查脚本使typed缓存失效；typed实际工具和obj-win-x准备均HIT，仍执行独立比较及NN全域检查。尚存：pipeline的native_ua与七流模型共键；两个缓存原有24件逐项相同，添加原生资源仍重做七流。ROOT路径亦进入整体键，迁冻结目录会失效。 | 构造缓存按相关TSV、构造器、格式、目标及必要工具身份失效；哈希核验、加载和重新测试分开计量。拆基础模型与native资源键；保持prepared-only消费，缺失／损坏返回75；不得复用旧测试PASS。实际HIT日志：`/tmp/unisacc-c43-e4-summary-final-20261024/out/lower-material-reuse/`。 |
| 环境室：跨调用失效 | Windows材料键包含整个调用环境。批量摘要实验tools rc0后json返回75；独立key-only实测两份165617字节preimage只差CODEX_TOOL_CALL_ID一个字段，所有文件／工具／其他环境相同。固定该调用字段后两次preimage逐字节相同、KEY一致。 | 实验前缀固定`CODEX_TOOL_CALL_ID=winbatch-fixed`；最终分窗验收亦须固定／明确清除此执行工具元数据，再封env-pin。不删来源、文件、工具或全环境检查。旧未pin准备不迁用；报告`/tmp/unisacc-c43-winbatch-key-rootcause.txt`。本次证据不能泛化为忽略所有未知环境变量。 |
| 自举间：驱动聚合 | 新候选作UA时，JOBS=1将五目标asmcompiler.c编译和打包串在一项，第二阶段窗口9触60秒。现已逐目标原50秒界完成十次代码编译、原封装和三阶段固定点；stage3模型材料复用经全哈希及七manifest核验。 | [F6收据](research/c43-f6-final-evidence/)。原聚合窗口仍为失败；不重构已完成的相同权重，也不迁用测试PASS。E4 seal三项规则依赖已变，须独立重封；edge-e4两seal不因这三项改动重录。 |
| 性能室：独立实验 | 同一lnx/x86_64驱动切片，实际包ISA内核诊断总27.315秒，输出与原产品逐字节相同：E2执行3.970秒、E1 0.146、E3 6.011、E4 14.397、prune 0.097、lower 1.096、enc 0.766；两次tapecheck合0.634。包加载0.025秒，解压／CRC／模型解码仅毫秒。另一次通用C语义计数对照rc0且输出相同：E4有288,505,813次模型转移、362,348,127次动作、7次SWAP；结合三处SWAP规则，证明此输入实际4次stack＋4次peep。不能把通用C计时当ISA或阶段执行当纯NN算子计时。 | 独立报告`/tmp/unisacc-c43-slice-phase-experiment.txt`与仓外`/tmp/unisacc-c43-slice-phase/`；输出SHA=ace9f9ed…与stage2切片cmp相同。热点已定位为E4多轮算法；经典C的J9已有跨轮行事实复用，模型未见对应映射。下一刀按真实变更复用行事实的最小等价实验，保留原轮数／输出／拒绝；不预断能达到2–3秒。 |
| 实验室：J9裁决 | 2026-10-25私有输出offset＋generation缓存仅复用原样COPYL的simple行。八例经典C／原NN／新NN及大输入均字节一致；同一实际ISA、包和8,446,684字节输入：原模型32.523秒、新模型39.114秒，慢20.27%。缓存增加的银行／动作成本未被收益抵消，完整接受域不别名证明亦未完成。 | **不合入**，产品仍为2974978e…／c90f0002…；报告`/tmp/unisacc-c43-e4-linefacts-prototype.txt`，补丁仅仓外审查材料。不能将经典C的数组缓存收益直接套到模型VM，也不能称J9思路整体不可行；后续需比较实际动作与内存成本，本轮回到当前候选验收。 |
| 封存室：E4 | 新seal已写入主树：49真实依赖、21接受例、96原始对拍全部相等，与候选shared/e4.net字节相同；writer与原58秒官方gate均rc0，负例全通过。零Python棘轮／inventory／inventory-derived／gate-layers四项rc0，无新增解释器调用。 | 原trace未修改；临时材料在`/srv/tmp/e4-reseal-20261025`。OUTDIR必须在ROOT外且位于TMPDIR内；前两次路径错误的原失败保留，最终清单无临时路径，不手删依赖。只重封受影响E4，不重建产品或其他模型；完整M7仍未完成。 |
| Windows室：分片 | 批量摘要私有实验固定调用环境后五准备均rc0，x86完整消费40.682秒rc0，ARM完整消费41.079秒rc142。摘要优化不足以让ARM聚合检查通过，原失败保留。 | 下一刀为五个显式scope：两架构cases／refusals及ARM large；全部旧断言联合覆盖，材料共用、共享锁串行，原40／41秒预算。新调度与缓存仍为.cx，现有独立裁判只增加选择接口，不增加Python依赖或调用点。私有补丁尚待实测，不冒充已合入。 |
| 身份室：候选 | E4新候选、本机及三阶段实际SHA=`2974978e6deec45abf38bfac2ac24e3031969b1c65009798270491d8b1e6dd1f`；S=`c90f0002f02366d510e4c8c22d4757f934edf8c0719852e706b575a7137b47df`；配对侧车SHA=`b88e3a1c227e72e621cf810cc5975e0922ba34cf34aa9e51288142aaaf94480a`。版本／hello／O2摘要回归、ident及官方固定点rc0；完整M7未完成。 | 冻结`/tmp/unisacc-c43-e4-summary-final-20261024` @d09bd1decb；`out/f6-e4-stage{2,3}-package.log`、逐切片真实argv/rc/时间/SHA、`out/comboot/stages.json`。stage3数据复用而代码切片重编，非模型构造器自举证明。按源码、产物及配对侧车核对，不为刷新mtime重复构建；[F6证据](research/c43-f6-final-evidence/)已原位更新。 |
| 控制室：现场 | Windows单元历史冻结`/tmp/unisacc-c43-winphase-accept-20261024` @2079b31e，计划SHA `d133abaf…`，1309节点／944边；原工具准备34.334秒／JSON41秒rc142保留；cb393新摘要分支下工具9.783秒、JSON15.265秒、表10.261秒均PREPARED，完整消费已输出25拒绝但整项仍41秒rc142。控制器-O2草案已暂停；当前主线为E4新候选固定点与受影响验证，完整身份与原预算保留。700ece46冻结根`/tmp/unisacc-c43-suite-accept-20261024`保存此前结果：`out/callback-final-checks`：12次off严格检查、on检查、8种生产命中、完整消费1再次rc0；空on／截断off真实消费均拒绝75。`out/winlower-x86_64-run`保留41秒rc142；`out/winlower-x86_64-timing.*`为55秒诊断，不能迁成41秒PASS。Windows新拆分仍待验。 | 先读本轮results／summary和存活进程。ae9145四片callback通过范围见`/tmp/unisacc-c43-current-accept-20261024/out/current-callback-checks/scope.tsv`；native／functionimports原证据见旧冻结根`/tmp/unisacc-c43-pe-accept-20261024/out/{native-*-refactor-check,libraryimports-atomic-v4-run}`。旧1142轮停于083，日志及原失败不清空、不迁绿。 |
| 出口室：分类 | RESOURCE_UNKNOWN保留OPEN义务；H1超时不算PASS；DEFER不等于终态失败。 | [资源处置](research/c43-resource-disposition.md)、[裁定](release/rulings.tsv)、[分类](release/preauth.tsv)及完整退出表。不补写结果或虚构平台绿。 |
| 发布室：交付 | M7之后封存；候选CI、签名、最终字节和公开下载须真实收据。2026-10-25只读API核对：本仓release-signing环境仅三项Azure凭据名，未配置Apple凭据；minicon的macOS Court绑定自仓minicon-com成功run及其原生产物，不是可直接输入unisacc候选的通用接口。 | [发布流水线](release/RELEASE-PIPELINE.md)、[候选记录](release/candidate.json)；当前候选记录尚不是0.0.43封存证明。[已查服务接口](https://github.com/partnernetsoftware/minicon/blob/main/.github/workflows/macos-signing.yml)仍需具体适配与接入授权，不触发外仓签名或复制凭据。 |
| 暂存室：边界 | 0.0.44登记.cx模块化／内存安全、自有工具链与测试改进；0.0.45登记流＋网模块外部调用及模块／权重共享；均不另作0.0.43前置。typed/callback迁移与广泛research归档保留后续范围。 | [0.0.44计划](plans/v0.0.44.md)、[0.0.45计划](plans/v0.0.45.md)、[路线](#5-路线)。新动态推进用`unisacc.com + .cx`，减少脚本，转调不算原生化。 |

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

七流真源：[exec/pipeline/image-stages.tsv](exec/pipeline/image-stages.tsv)；十八网真源：[unisa/gold.py 的 STAGES](unisa/gold.py)，金表位于 weights/gold/。逻辑完整输入与拆分方案以[tests/STAGE-IDENTITIES.md](tests/STAGE-IDENTITIES.md)为准，本节汇总现有物理管道及观测。

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

| 发布链三格（2026-10-24动态状态） | 状态 | 证据边界 |
|---|---|---|
| ① 源码与发布身份 | 历史源码已推；本轮候选封存／RC待完成 | 当前本地提交不冒充远端发布身份；按§2主线推进 |
| ② 本轮候选有效闸与收口收据 | **RUNNING：1142项，终态待收口** | 见§2思维树及记忆宫殿现场入口；不以历史局部结果顶替 |
| ③ 发行资产及正式发布 | 本轮未完成 | Latest仍v0.0.42；本轮0.0.43未因源码push而发布 |

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
- **功能/限制**：对象/函数/变参宏、参数展开、字符串化、覆盖的拼接、条件表达式、包含搜索、macro-stack pragma、位置跟踪。参数上限 MAXP=8；一般标点/字面量拼接等尚有限制，E2 自身不生成完整 file:line:col 诊断。
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

下面从 `research/model-bytes.json` 生成，按实际封存 `.com` 计。只对阶段/模式作可证的功能归属，不把跨阶段动作任意分摊成“指针占多少字节”。

<!-- model-bytes:begin -->
快照 SHA-256：`518601e5ff6e844cc7faf4c45b91bcf379cbe5f0d77017452ff6020bfc120de7`；总计 **2,019,976 B**。

| 物理内容 | 字节 | 占整个 .com |
|---|---:|---:|
| 28 个共享网络体 | 880,454 | 43.59% |
| 平台驱动、APE 启动/加载与对齐（混合账） | 796,560 | 39.43% |
| 43 份 C 头文件/库实现源码 | 255,303 | 12.64% |
| 两 ISA 通用推理执行核资源 | 15,520 | 0.77% |
| 目标预定义宏声明资源 | 321 | 0.02% |
| 目录、记录头与资源键 | 71,802 | 3.55% |
| 尾部 | 16 | 0.00% |

| 模型阶段 / 具体功能 | 物理模型数 | 模型体 B | 占 .com | 阶段行引用数 |
|---|---:|---:|---:|---:|
| `e2`：预处理、目标预定义宏与位置模式 | 2 | 72,684 | 3.60% | 138 |
| `e1`：词法与 token/位置输出 | 1 | 27,764 | 1.37% | 132 |
| `e3`：解析、类型/作用域、tape、错误与警告 | 2 | 303,469 | 15.02% | 240 |
| `e4`：O1/O2 优化 | 2 | 14,409 | 0.71% | 160 |
| `nativeabi`：宿主ABI carrier认证（原始类型图→目标载体证书） | 1 | 15,976 | 0.79% | 6 |
| `prune`：函数可达闭包与保守原文剪枝（数据/地址根保留） | 1 | 9,110 | 0.45% | 168 |
| `lower`：ABI、调用、目标指令 lowering 与数据布局 | 8 | 205,584 | 10.18% | 168 |
| `elf`：目标指令编码及 ELF/Mach-O/PE 镜像写出 | 8 | 178,690 | 8.85% | 102 |
| `tokenpp`：公开 token 路线的预处理 | 1 | 13,880 | 0.69% | 1 |
| `tokenlex`：公开 token 路线的词法输出 | 1 | 4,477 | 0.22% | 1 |
| `units`：多文件分帧与文件级 static 隔离 | 1 | 34,411 | 1.70% | 120 |

下表是二进制网络的压缩前拆分；与物理压缩体不可相加，百分比为相对整包大小而非物理占比：

| 网络记录 / 含义 | 字节 | 占整个 .com |
|---|---:|---:|
| `H`：阈值/选择网络参数记录 | 1,221,473 | 60.47% |
| `Q`：动作序列声明（包含编译模板动作） | 1,115,159 | 55.21% |
| `S`：字节字符串声明 | 22,764 | 1.13% |
| `N`：网络头记录 | 565 | 0.03% |
| `C`：动作序列共享前缀声明 | 636,894 | 31.53% |
<!-- model-bytes:end -->

两个 e3 是错误与错误+告警变体。共享物理模型数、阶段行数和物理字节以本节的生成账本为准；压缩前 Q/C/H 不能与压缩体相加。旧快照、纠错和计数来源见 [结构测量史](archive/prd-release-and-development-history-20261001.md#结构测量与纠错)。

### 3.15 目标文件、分开编译与链接（0.0.17）

- **对象**：`-c -b os/arch` 写可重定位对象，格式随目标（ELF/Mach-O/COFF）。参考侧三格式都有；产品的 Linux ELF 由模型路由写出（对象专用 lower + ELF 写出 δ，与参考逐字节同），Mach-O/COFF 产品路由在 0.0.18（驱动按名拒绝）。整程序对象可被系统链接器单独链接，并可读 cc 定义的数据符号；与 cc 的函数互调需要调用约定适配（0.0.18）。设计与事实表见 [docs/toolchain.md](docs/toolchain.md)。
- **单元与链接**：`-funit` 写单元对象（tape 带 `.global/.extern` 链接属性；对象里另有不装载的节存单元 tape）。`unisacc a.o b.o lib.a [-o prog]` 在 tape 级合并（src/tapelink.c：私有名改 `__u<k>_`、全局按名相会、同名全局对象按 C 公共定义合一、各单元 `__init_u` 串成 `__init`），再走普通后端，六目标都可出；`unisacc ar rcs|t|x` 读写系统 `ar` 也认的归档。产品驱动复用同一合并核心，编译决策仍全在模型路由。
- **驱动默认模式**：见 §3.11（裸 `unisacc FILE.c` 即运行）。

## 4. 质量体系

- 断言按 contract / stage / pipeline / platform 分账，见 [tests/GATE-LAYERS.md](tests/GATE-LAYERS.md)；局部绿不代全链，未覆盖不计通过。
- 清单、来源身份、实际受试字节与原始结果分别可核；C99覆盖见 [tests/c99/clauses.tsv](tests/c99/clauses.tsv)，测试入口见 [tests/INVENTORY.md](tests/INVENTORY.md)。
- 当前开发按具名差集及必要合同门推进，准备/构造与测量单列预算；不得因历史流水线描述自动全量打包、重封或恢复旧队列。
- 发布流程权威入口：[release/RELEASE-PIPELINE.md](release/RELEASE-PIPELINE.md)、[release/README.md](release/README.md)及工作流。当前目标已授权公开发布；新候选固定点与M7终态待收口，旧1142项队列已停窗，不能作为新候选验收（见§2）。

## 5. 路线

### 5.0 当前优先与0.0.x退出条件

当前先收口前级/库边界与晚绑定阶段合同，具体刀序和验收只在[0.0.43计划](plans/v0.0.43.md)维护。旧0.0.29–0.0.42时间表已归档，不因版本号推进而宣称义务完成。

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

**规划决定（主人 2026-10-24）**：在 0.1.x 制作共用 `.cx` 前端、构建驱动器及必要的编译器插件/适配，使 Clang、GCC、TinyCC 等工具链支持 `.cx`。本节是未来路线，尚未实现；具体安全语义以 [`.cx` 草案](docs/cx-spec-draft.md) 后续形成的稳定规范为准。`.cx` 继续采用 C99 语法，通过模块接口、契约和受检查的编译配置表达能力，不增加 Rust 式源码关键字。

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
