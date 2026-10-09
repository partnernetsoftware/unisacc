# unisacc —— 当前状态、架构与路线

> prd 只写**当前与将来**；发布身份与过程记录在 [历史索引](archive/prd-release-and-development-history-20261001.md)。条款全文在 [spec.md](spec.md)（代码注释引用的 `[S-9]`、`[W-16]` 等编号都在那里，只增不改）；构想在 [plans/ideas.md](plans/ideas.md)；已发布版本的计划、叙事与证据在 [archive/](archive/)；目录归属见 [ARCHITECTURE.md](ARCHITECTURE.md)。新知识和决定先写进这里再做别的。

## 1. 命题与产品

- **产品**：一个文件 `unisacc.com`（APE），在 {Linux, macOS, Windows} × {x86-64, arm64} 上运行，自己写出六个目标的可执行文件（ELF、Mach-O、PE；映像不经汇编器和链接器），0.0.17 起也写三种格式的可重定位 `.o` 并自带链接器与 `ar`（§3.15），并能自举：宿主构建的 `unisacc-seed.com` → 种子自建 → 再自建，三个 sha256 相等（N22）。两代产品只叫 `unisacc-seed.com` 与 `unisacc.com`。
- **语言**：C99 = ISO/IEC 9899:1999 + TC1–TC3（WG14 N1256）。覆盖度来自逐条款账本 [tests/c99/clauses.tsv](tests/c99/clauses.tsv)（门禁 `c99-ledger`，README 表由账本生成），不作主张；缺口在 README 限制表逐条点名。
- **方法**：编译器里每个表状决策（词法类、预处理指令、优先级、类型、指令选择、ABI、窥孔……）由**构造**出来的小整数网络回答，权重从真值表派生、在表的全部定义域上**穷举验证**（不是训练）；结构性粘合是经典代码。两条路线：C 参考编译器（`unisacc.c` + `src/`，行为裁判与回退）与模型产品（`exec/` 各阶段网络 + 通用执行器），逐字节对拍（`exec-chain`、`closure`、`stages`、difftest 两侧）。
- **交付物**：签名的 `unisacc.com` 与公证的 macOS dmg（公开资产只有这两个，主人 2026-09-30 裁定）；中间形式 `.tape`/`.tapebin`（[docs/tapebin-v1.md](docs/tapebin-v1.md)，内容寻址、C/Python/产品三方编码逐字节同）。
- **公开入口仓**（主人 2026-10-05，拟注册商标 UNISA）：[partnernetsoftware/unisa](https://github.com/partnernetsoftware/unisa)，只收副本：unisacc、ujs 的最终发行包（`releases/`，定期从本仓 release 抄过去），产品介绍（`docs/`）与论文（`papers/`，不定时抄）；以后做 unisacc 的包管理入口和 agenterm 的“插件与应用”入口。开发不在那里。

## 2. 当前版本

### 版本状态与历史身份

最近已发布版本为 **v0.0.36**（2026-10-09 公开）。逐版发布身份见 [历史记录](archive/prd-release-and-development-history-20261001.md#逐版发布身份) 与 [GitHub Release](https://github.com/partnernetsoftware/unisacc/releases)；候选哈希与逐版验收不在本文件复写。后续计划见下表。

### 计划索引（正文在 plans/，prd 只放索引）

| 版本 | 文件 | 状态 |
|---|---|---|
| v0.0.12–v0.0.29 | [archive/plans/](archive/plans/) — 逐版计划与结项收据；发布身份与回执见 [GitHub Release](https://github.com/partnernetsoftware/unisacc/releases) 与 research/r*-release-acceptance.json | 已归档 |
| v0.0.30 | [archive/plans/v0.0.30.md](archive/plans/v0.0.30.md) | 2026-10-06 公开 |
| v0.0.31 | [archive/plans/v0.0.31.md](archive/plans/v0.0.31.md) | 已完成，公开待政委裁定 |
| v0.0.32 | [archive/plans/v0.0.32.md](archive/plans/v0.0.32.md) | 10-07 公开（unisacc.com d198bae2，候选 38ea3622；回执 research/r32-release-acceptance.json） |
| v0.0.33 | [archive/plans/v0.0.33.md](archive/plans/v0.0.33.md) | 10-08 公开（unisacc.com c2d03fe6，候选 e0a8ff7c；rc/v0.0.33=bc23f113；回执 research/r33-release-acceptance.json） |
| v0.0.34 | [archive/plans/v0.0.34.md](archive/plans/v0.0.34.md) | 10-08 公开（unisacc.com f48938ef，候选 60f4765c；rc/v0.0.34=99150ee6；回执 research/r34-release-acceptance.json） |
| v0.0.35 | [archive/plans/v0.0.35.md](archive/plans/v0.0.35.md) | 10-08 公开（回执 research/r35-release-acceptance.json） |
| v0.0.36 | [archive/plans/v0.0.36.md](archive/plans/v0.0.36.md) | 10-09 公开（unisacc.com baf296dd，候选 b4607639；rc2 29728cc6；回执 research/r36-release-acceptance.json） |
| v0.0.37 | [plans/v0.0.37.md](plans/v0.0.37.md) | 进行中（c99 闭环与顺延超限项） |
| v0.0.11 及更早 | 见 §7 归档索引 | 已发布 |

历史实施决定与试验见 [R19 过程记录](archive/prd-release-and-development-history-20261001.md#r19-开发决定)；仍未解决的问题由 [v0.0.31 计划](archive/plans/v0.0.31.md) 跟踪。

**产品命名（主人 2026-09-30 提醒，硬规则）**：两代产品只有两个名字——宿主构建的第一代叫 **`unisacc-seed.com`**，由它自举出来的最终产品叫 **`unisacc.com`**（发布物、GHCR 候选、README 与 N22 三阶段的文件名都按此；`unisacc-next.com` 只是构建目录里的中间名，不出仓）。


<a id="pipeline-design"></a>
## 3. 当前架构：模型流水线（结构、功能与边界）

本节以 0.0.17 发布基线（`ae3f989`）说明构造器、路由与执行器，后续已验收的变化按相关小节补充；版本身份见 [历史记录](archive/prd-release-and-development-history-20261001.md#逐版发布身份)。条款全文在 [spec.md](spec.md)。 旧的“14 个决策点”不等于当前整条流水线的网络数，旧的“走查器不许模型化”不是当前迁移约束。

**阅读顺序**：共同模型结构 → 路由总表 → 各阶段 → 打包/装载 → 字节账 → 验证与现状。详细源码边界另见 [规则来源](exec/rules.md)、[执行机制](exec/c/CORE.md)、[包协议](exec/c/PACKAGE.md)。迁移过程及原始失败记录完整保留在 [S-17 迁移档案](archive/s17-migration-log-20260928.md)，不再混入当前规格。

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

输入/输出均为 tape，O0 不调用；O1/O2 使用独立控制网络。START 加载 opinfo/peep 事实；扫描、解析、基本块、读写、活跃性、融合和轮次来自 `exec/opt` 的 TSV。O1 在最多四轮内检查有限直线范围，把软件栈保存/恢复替换成 mov。O2 加入基本块/标签、活跃性重扫、死寄存器携值、局部读融合、相邻指令 peep 和复制/目的重定向。它是有界保守改写，不是全程序优化或完整寄存器分配；算法被编码进规则与动作，并未凭网络构造消除算法信息。判据是同一 O0 tape 经模型后与参考 O1/O2 的 tape 字节相等，另有独立执行差分。来源/门禁见 [opt/gen.py](exec/opt/gen.py)、exec-e4/exec-e4self。

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

〔开发提速评审吸纳，2026-10-09；政委令，cc 落实〕来源 cdx2 只读评审（/tmp/unisacc-cdx2/speedup-review.md，未入库）：近期最大损耗是环境型失效与临时状态丢失（0.0.36 两次 539→15、713→11；重启清空临时目录后重建候选），其次是候选前补录/闭包变化引起的重封与整轮重跑（0.0.24 三次封装 64 分；0.0.31/32 版本号后置重复整轮约 45 分）。本版只落 ROI 高、不改验收的五项，细则见 release/RELEASE-PIPELINE.md §23：①队列只用 queue.sh（NOFALLBACK、同源候选对、单驱动、caffeinate 防休眠）；②precheck 加未提交/未跟踪输入与 corpus 在位检查；③queue.sh 每窗后目录外持久备份、重启自动恢复（gatequeue 仍逐项核指纹）；④签名等待窗重叠原生 Linux 预验；⑤日常阶段定向+chain 再构正式候选。不做：加 jobs、放宽预算、自动重试到绿、Linux 绿顶替 m4pro 全门禁、重开 Q2。评审中的 6–10 项（两槽并发构建、缓存命中核验、precheck 契约状态共享、阶段输入失效、套件合并/K2 迁移）待测量或待批准，不在本版实施；K2/K1 排期仍未定（E76）。

## 4. 质量体系

- **门禁**：`tests/gate.sh --list [--com]`（0.0.21：--com 432 项）；发布验收只用 `tests/release.sh --com`（jobs 4、window 50、经 `tests/term.sh`、`SEED_DIR` 必填、STRICT=1），每项各绿一次；冷建网络的预热已并进 release.sh（队列前三窗，0.0.17 E1）。CI（`ci.yml` 跑 `all.sh`）是第二意见，Intel runner 仅供参考。每次运行 ≤60 s。
- **诊断分层**：[tests/GATE-LAYERS.md](tests/GATE-LAYERS.md) 把同一门禁清单按断言跨度分为 contract、stage、pipeline、platform；`gatequeue.py --layer/--through-layer` 只筛选日常排查范围，耗时仍由实测历史调度。`gate-layers` 校验全部套件均归类且四层并集等于原清单；未筛选的发布全量门禁保持原样。
- **清单契约**：knownfail/knownwrong 文件一行一个名字，列名即 known、转同即 revived（红，必须删行）：difftest(.com)、pyfront（Python 对照组缺口）、chain（模型与参考镜像差异）、diag(.com)、corpus（非 C99 输入）、c99。没有隐式排除——任何“覆盖名单”都要配清单（R14-8 教训）。
- **账本**：C99 条款账本（覆盖）、模型字节账（`research/model-bytes.json`）、裁判登记（`research/referee.tsv`）；README/ARCHITECTURE 不重抄账本数字（docs 门禁断言）。
- **发版复盘**（0.0.27 起每版一份，research/r<N>-retrospective.md）：列出各环节耗时、问题与解决、要带进下一版的流程项；能持久化的规则写进 release/RELEASE-PIPELINE.md 的逐版回顾，流程项写进下一版计划（0.0.27 → v0.0.28 R1–R4）。
- **队列复用与超时**（0.0.32）：只改一份测试文件复用 90.7%（q13）；声明与作业命令两边展开 `{MODEL_COM}`、带 env 前缀的用 `match: contains`，否则退回全局指纹；调度先后在 `tests/gateorder.json`（不进公共指纹）；满额看门狗超时自动独占重试一次；性能断言量 CPU 秒而非墙钟。F4 采样：产品冷编约 92% 样本在执行器内核 transition 热循环。
- **reviewed trees**：`make gatedeps` 从 HEAD 的 `git archive`（umask 022）计算戳与 guards，作为发布前最后一提交。

### 开发与发布流水线（向 minicon 学习，v0.0.10 起实行；手册与脚本是权威，此处为索引）

```
本机：改代码 → 提交 → 冻结源构私有 UA → make model-com（shared/六 target/pack）→ 全量门禁 gatequeue（Terminal 交接）
   → 装根 + 字节账 + 回执 → seal_candidate.sh：oras push 到 GHCR，release/candidate.json 记摘要 → 一次 push
GitHub：release-check.yml（每次 push，约 1 分钟）= 源预检 + 按 GHCR 摘要拉取候选在 ubuntu/macos 实跑
   → windows-signing.yml（手动 dispatch：qualification → company，release-signing 环境审批，Azure Artifact Signing）
   → 草稿 Release（未签 zip + 回执 + 签后 .com + 回执 + Apple app/dmg）→ gh release 发布（tag 落在候选源 SHA）
本机：Apple 签名/公证/staple（apple-sign.sh，与 CI 并行）；ci.yml 全量矩阵每周一或手动，是安全网不是前提
```

- **权威文档**：[release/RELEASE-PIPELINE.md](release/RELEASE-PIPELINE.md)（§0–10：冻结源、UA、P3、门禁队列、客机、CI/GHCR、Apple、Windows 签名、发布、命令序列、必跑清单；§11–14 逐版教训）；契约 [release/README.md](release/README.md)；策略 `release/signing-policy.json`；封存 `release/candidate.json`；工作流 `.github/workflows/{release-check,windows-signing,ci}.yml`；本机技能 `~/.claude/skills/unisacc-release-pipeline`。
- **与 minicon 的对应**：minicon 在 CI 构建候选并推 GHCR，我们**本机构建**（一个能写出六目标的编译器，CI 只测不建，见 CLAUDE.md）再推 GHCR；签名/运行验证都按摘要拉取，同 minicon；docs 提交不打断签名（按产品源闭包比较）。
- **现状**（0.0.21 实发）：push 后 release-check 约 1 分钟（六个原生 runner 跑封存候选，这就是六格产品证据，x86_64 Linux 也以此为准）；资格→公司签名约 4 分钟；Apple 公证约 6 分钟，可与队列并行；`release/tools/publish.sh` 先核对草稿字节再公开，公开后下载核对。Linux 源码全套只在原生 arm64 客机上跑，模拟机全套不算证据。教训与下一版目标流程（照搬 minicon 的候选绑定、只发封存字节、发布后冒烟）见 RELEASE-PIPELINE.md §14 与 0.0.22 计划第 2 项。
- **每片重封**：开发期每次产品闭包变化都要 `seal_candidate.sh <ver>-dev`，否则 release-check 候选作业红（0.0.11 前三次 push 的教训）。


## 5. 路线

- **0.0.x 收官（2026-10-05，主人：按规划早点做完；同日修订）**：见 [plans/roadmap-0.0.x.md](plans/roadmap-0.0.x.md)。六个版本：0.0.29 parse2；0.0.30 enc/lower + pthread/wctype/fenv；0.0.31 构建链不调用 Python；0.0.32 lua+sqlite 两路线通过；0.0.33 Windows 宿主转发 + 六平台进程矩阵；0.0.34 minicon+agenterm 通过、包与容器自构造 = 0.0.x 完工（10-08：三项未开工，具名顺延到 0.0.35，0.0.x 完工随之移到 0.0.35）。0.1.x 只留证明、内存安全节点、权重研究、wasm。只顺延、不砍。
- **远期（主人 2026-10-06）**：从 0.2.x.y 起具备完善的 POSIX C99 能力，并逐步丰富包管理，形成自己的生态，摆脱对 Python/Node.js/Bun 等动态语言的依赖。路线含义：0.0.31 的构建链不调用 Python 是第一步；0.1.x 之后的每版应削减一项动态语言依赖（参考实现、门禁脚本、发布工具），并为包格式和包仓库预留条款。远期还包括（主人 2026-10-06 补充）：unisacc.com 成熟后作为 shebang 解释器运行 Unix 脚本（`#!/usr/bin/env unisacc.com -run`，已有的 `-run` 在内存里映射代码，不写出二进制、不触发首次执行扫描）；脚本后缀定为 **`.cx`**（主人 2026-10-06 决定：表示 .c 的扩展；`.cs` C#、`.csh` C shell 已被占用）。
- **.cx 安全与模块设计（主人 2026-10-08）**：主人重申：目标是焕发 C 的生机，借鉴 Rust 的编译期内存检查、内存管理和模块化，不新增语法糖；.cx 定位为服务 unisacc C 脚本体系的受检查 C99 子集与运行配置。以现有声明、契约、模块清单和必要的 pragma 承载能力。设计提案见 [docs/cx-spec-draft.md](docs/cx-spec-draft.md)；尚未实现或纳入稳定 spec，现有 .cx 脚本兼容模式不自动获得安全保证。
- **.cx 替代 .sh/.py 的分期（主人 2026-10-07 问“哪个版本能完成”；cc 评估 10-07）**：现状——`#!/usr/bin/env unisacc.com -run` 的 .cx 已能跑（t.cx 实测传参正确），libc 已有 system/fork/execvp/waitpid/opendir/glob/regcomp/getenv，缺 popen；仓库跟踪 190 个 .sh（tests 106、exec 55、release 13）与 408 个 .py（tests 170、exec 118、unisa 48、ujs 29）。分期：**0.0.34** 补 popen 与一个小的 `include/cx.h`（跑命令取输出、读写整文件、sha256、限时），试点改写 3 个小脚本（prevheaders、headerchain、lifecycle）并入门禁；**0.0.35–0.0.36** 构建与发布链（release/ 13 个 .sh、build_candidate、buildcompiler 编排）改为 .cx，由上一版 unisacc.com 运行，自举链除宿主 cc 外不再需要 sh/Python；**0.0.37** 门禁编排（gate/gatequeue/term）；**0.0.38–0.0.39** tests 下 170 个 .py 套件（两版分批）；**0.0.40** exec 生成器（cdx 域，δ 构造器已有 C 种子先例 seed/gen.c）；**0.0.41** 替换 unisa/ Python 参考实现（行为基准，须先有逐字节等价的 C/.cx 参考）。**全部在 0.0.x 内完成自举**（主人 10-07：0.0.x 就自举，0.1.x 开发包管理）。完成判据：仓库干净机器只装 unisacc.com 与宿主 cc 即可构建、门禁、发布。
- 0.2.x.y 系列目标（脚本领域、Rust 式内存检查）随 0.1.x/0.2.x 计划于 2026-10-07 撤下，原文见 git 历史（主人：专心 0.0.x.y）。

当前版本及其完成状态只在 §2 的计划索引维护。0.0.21 已公开；后续列入 [0.0.22 计划](archive/plans/v0.0.22.md)。0.1.x/0.2.x 远期计划已于 2026-10-07 撤下（主人：干扰太大，专心 0.0.x.y），旧稿在 archive/plans/。C99 种子构造器（R20-1）与单元链接语义（R20-3）的实施记录已移到 [archive/prd-notes-20261002.md](archive/prd-notes-20261002.md)，进度以 0.0.22 计划第 7 项为准。

### 5.0 种子层路线（主人定调 2026-10-03 19:51）

- **种子备用（主人 2026-10-06 更正）**：`unisacc-seed.com`(.build.json) 仍在 .gitignore，但与 `unisacc.com` 一样装在仓根备用（发布流程的安装步骤成对安装），不再只放 /tmp；**不随各版本发布公开**（sha 只记在回执里）。

0.0.24–0.0.27：K2 收尾 → 冻结 TSV/清单 DSL 并写语法与语义规格（opts 键与值前缀进门禁封顶；去掉 @stack getattr 回调、@fmt 的 str.format 依赖、=vN 不透明绑定；隐式顺序改表内显式规则；动作序列移出 facts；清死数据）→ 每个表条目与 DSL 操作配独立用例并量覆盖率（判对错不依赖 Python 产物或 graphhash）。0.0.25 起用 C 重写种子层（政委 2026-10-04 21:31 确认，原“0.0.25 之后评估”作废；路线见 archive/plans/v0.0.25.md 的自举路线 B1–B5），作为冻结规格的权威实现，最终完全不依赖 Python；Python 只留历史参照。详见 archive/plans/v0.0.23.md「0.0.24–0.0.27 方向」。

### 5.1 libc 路线裁定（主人 2026-10-02）

**反对自研 libc：系统已有的尽量复用（转发给系统 libc），参照 tinycc / `tcc -run`。** 依据与移交见 [research/libc-forward-handoff.md](research/libc-forward-handoff.md)，方案底稿是 [libc-unify-design.md](research/libc-unify-design.md) 的 D2。落地口径：每个函数族先进“转发 / 保留 / 拒绝”路由表，用探针与宿主逐字节对拍通过才切换，否则维持按名拒绝；字节与自举（N22、六目标折叠）的影响逐条标注。参考侧已随 v0.0.21 交付（[归档计划](archive/plans/v0.0.21.md) 第 4a 项），产品侧在 0.0.22。**补充裁定（主人 2026-10-02）**：不接受“Linux 静态 ELF 没有动态装载器所以不转发”——三个 OS 都要把动态装载做好：Linux 写最小动态 ELF（PT_INTERP 指向系统 ld.so，DT_NEEDED libc.so.6，四个 GLOB_DAT 槽绑定 dlopen/dlsym/dlclose/dlerror，与 macOS 的四个 eager bind 同形），Windows 把同样四个槽映射到 LoadLibraryA/GetProcAddress/FreeLibrary/GetLastError；于是 `__hostaddr0..3` + `__hostcall` 在六个目标上是同一条转发通道。只有用到转发的程序才写动态头，其余镜像字节不变。**方向澄清（主人 2026-10-02）**：反对自研 libc 的原因是它是大工程，**将来应做成外置的 libc 包，而不是内置在 unisacc.com 里**；“缺什么函数就补一个函数体编进去”的亡羊补牢做法是无底洞，停止。于是 0.0.21 的转发不是逐个函数写转发桩，而是**通用转发**：有原型、无定义、随带库也没有的外部函数，一律由编译器生成转发桩（0.0.19 R19-10 的 `fwd_stub` 机制，目前只在 macOS `-run` 下），扩展到写出的镜像与 Linux（借上面的四个 dl 槽）；随带头文件逐步收缩为声明，函数体只留纯计算且影响确定性的部分，最终外置。主人也说明静态与动态不是硬要求、产物体积暂不是关键，以实现与可维护为先。

**F4 分拆+顺延（政委 10-07 09:26）**：0.0.32 收已落地提速与钉住基准，实测约 6.0 s；E3 单遍等拆为 0.0.33 第一项 F4′，发布说明写明。
**F4′ 增量 E3 私有验证（10-07，待同源候选）**：首遍保留程序 tape，仅将转发桩单元过 E2/E1/E3 后并入；不认识的 tape 形状退回全程序重编。单源转发探针快慢镜像在已测四个非 Windows 目标逐字节一致；多单元因桩单元顺序不同，镜像字节未与旧路对齐，行为探针通过。固定 0.0.32 的 csih 15 单元，私有产品编译器低负载热态测得 4.59 秒（旧路约 6 秒）；最新头与模型包在宿主 load average 约 12 时快路 15.0–15.4 秒、旧路 19.5 秒，旧模型对照也升到 13.4 秒，说明该轮墙钟受共享负载影响。正式 ≤5 秒及全门禁须在同源候选的固定条件下复核。

**H3c 已降级（10-07）**：出货 .com 对 pthread 具名拒绝是 0.0.32 的正式行为，接受路径为 0.0.33 H3d。

**董秘 10-07 可逆裁定**：F4 前缀和只做 scratch 实测、不进主线；H3c 降级线同意；0.0.31 公开继续等政委明确“发”。

**借用口径（政委 10-07 07:40，董秘转、unisa 协调员二次澄清；E44 闭合）**：不全实现 libc，不等于禁止借用。缺的 POSIX / C99 函数（如 pthread）可按需从 cosmocc（ISC）或同类实现抄写，保留版权并在 NOTICE 注明出处；禁止的是完整自研 libc 或整包内置替代系统库。不再以“只许薄封装转发、禁止抄源码”卡住缺项。

### 5.2 v0.1.x：权重本体与推理速度（研究候选，不自动开工）

目标是减少阈值网络的**权重与结构记录本身**、运行时内存及每次编译的模型求值成本；P3/DEFLATE 发布包压缩率只作背景指标，不能冒充权重减少。只读快照（根 `unisacc.com` SHA256 `8dad89c528a789a25151ec26e8ea38889b8c1aa15bb8a8badba161a9fb7eda31`，2026-10-05）：28 个网络共 71,771 个状态银行、122,958 个阈值隐单元；48,558 个银行无隐单元，66,230 个银行至多两个，284 个银行至少 32 个，最大 2,171 个。`research/model-bytes.json` 的 raw 2,996,855 B / 存储 880,454 B 与此快照的网络数量同量，但该账本绑定旧候选，不能当当前源码身份。已有 [FX-6 测量](archive/research/r12/r12-fx6-measurement.json)：每次转移平均激活 1.9 个单元、每输入字节约 18.3 次转移，SIMD/O3 未提速；静态银行数不代表动态热度。

优先做两项**最小实验**，旧排期见 [archive/plans/v0.1.x.md](archive/plans/v0.1.x.md) 的权重本体与推理一节（已撤下，不排期）：

1. **同权连续阈值段的精确聚合**：把一段相同的双输出差分权重表示为阈值激活数乘权重，保持阈值网络语义。只读统计得到 122,958 个单元可归为 66,975 段（记录数 1.84 倍），其中 234 个长银行的记录数可局部减少至少 10 倍；尚未计新增段字段、真实字节、访问频率与延迟，不能宣称全模型或整机达到 10 倍。
2. **类型化动作模板与银行结构归一化**：统计仅在寄存器、常量、字符串或续点上不同的动作/状态，连同参数和重定位计算真实大小；先只读建模，不改执行器。若净字节或动态热点不足，停止“全模型数量级缩小”假设。

后续候选包括跨网络子图共享、精确窄位宽/连续 arena、分层阈值电路，以及由网络和动作目录自动导出的执行块。它们分别量权重数与位宽、运行内存、加载时间、转移数和同身份单次编译延迟；批量吞吐另列。任何表示变更都须能反展开或映射到原状态/动作编号，对全部观察域（含缺边、EOF、声明返回）逐项验证，并过阶段接口与整流水线逐字节门禁。不得以答案表、手写 C 解析/ABI 快路径或近似量化替换出货的网络决策。当前结论只是结构统计和研究设计，尚无新速度基准或实现。

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


当前决定：论文区分 **T2a（同一通用执行器中的控制器替换）** 与 **T2b（独立参考编译器的整程模拟）**。T2a 在相同初始化、有限观察、动作目录、原语语义、全接口、整数范围与逻辑资源预算条件下，以逐步归纳得到轨迹及接受/拒绝/发散等价；这是数学推导，不冒称实际 C/汇编循环已由 Lean 证明。T2b 与源语言语义保持 T3 仍开放。

固定阶段的续点字母表有限，无界栈高度不使返回成为无限决策；声明式返回是完整控制器中的等价因子化。阶段语义由权重、返回数据、动作目录与常量共同承载。整管线组合要求字节、状态、位置及资源元数据全接口一致；字节接口不承诺有界内存或在线输出。

索引：[论文理论连接与只读代码审查](research/paper-a-theory-review.md)。优先改进候选：部署求值器的域/范围证书、通用原语的边界契约、阶段完整接口与可回放轨迹、E3 语义组合与状态不变量。建议仅登记，产品改动由 cc 编排。

## 7. 归档索引


prd 只描述当前与将来；过程记录、旧计划与历史数字按时间封存在 `archive/`：

- [prd-history.md](archive/prd-history.md)：v1–v3.4 逐版条目与 14 阶段前的模型总表。
- [s17-migration-log-20260928.md](archive/s17-migration-log-20260928.md)：2026-09-26–28 模型化迁移逐片日志。
- [r9-integration-history-20260928.md](archive/r9-integration-history-20260928.md)：R9 集成史。
- [prd-r9-r10-receipts-20260929.md](archive/prd-r9-r10-receipts-20260929.md)：v0.0.9/R9 状态、v0.0.10 全计划、R9/R10 逐片回执与发布收口、2026-09-27 巡查（5.6/5.8/5.10）。
- [prd-history-20260929.md](archive/prd-history-20260929.md)：本次第二轮清理移出的 §0.3 早期交付边界、§5.5 完成度盘点 S-1..S-17、§5.7/5.9 巡查记录、v0.0.11 逐项回执全文、附录 7.1/7.2/7.4。
- [prd-findings-20260929.md](archive/prd-findings-20260929.md)：§6 实验发现 E-编号全表（论文引用按编号在此解析）。
- [prd-r17-notes-20261001.md](archive/prd-r17-notes-20261001.md)：0.0.17 开发期间写在 prd 末尾的 R17 实施记录（tape 链接属性、产品对象路线各片）。
- [prd-notes-20261002.md](archive/prd-notes-20261002.md)：0.0.21 发布后从 §5/§6 移出的 R20-1/R20-3 实施记录、已修复问题与旧排期条目。
- [prd-release-and-development-history-20261001.md](archive/prd-release-and-development-history-20261001.md)：逐版发布身份、R19 开发决定、旧结构测量与 R18/R19 实施回执。

### V1：discarded-value 与 volatile 读取核查（源码与表已验，候选待验）

按 C99 6.3.2.1/6.5.17 修复逗号左操作数、表达式语句及 for 初始/步进语境的读取；取地址、sizeof 和函数设计ator不读对象或代码。系统 cc 优化后汇编确认 volatile 读取；独立 VM 追踪确认参考 O0/O1/O2 与最终 E3 对探针发生恰好六次对象读取，且不读函数代码。门禁为 volatile-comma / com-volatile-comma，不把 tape 相等单独当正确性证明。

本地私有同源参考：difftest 四片 226 匹配、0 wrong；difftest_o 四片 678 匹配、0 wrong；ccrun 八片 212 匹配、14 既有 knownwrong、0 wrong；exec-chain 三片共 296 文件，295 相等、1 既有未覆盖、0 bad/lost。E3 八变体图基线已更新；普通版 9,543 状态、2,461,838 观测全域 net=table。新产品须重建后跑 com-volatile-comma，尚未计产品候选通过。

### T7：流水线生成缓存闭包

采用 exec/pipeline/models.py 的共享保守内容闭包，替代生成命令入口目录猜测。把闭包摘要作为 fresh 的输入，保留参考身份和命令身份；允许多失效，不允许阶段表变化后命中旧图。验收以私有目录中的旧键命中/新键失效复现及现有 -I 流水线为准。 已复现旧逻辑命中旧图记录，新键对表修改/恢复及文件新增/删除均失效，未改输入仍命中；引用独立按内容寻址的摘要文件，避免不同构造身份并发覆盖。pipeline-cache 门禁已登记，原目录引号 include 与 -I 三阶段、Csmith seed 1 的仓内收据路径均与参考一致。

### T1e：autoinc facts 动作迁回声明模板

本片只迁 pp-autoinc-gen：函数名、库依赖闭包的槽偏移、头文件名与顺序作为事实；SBCLR/SBOUT、槽置位及 include 输出动作留在 autoinc 模板清单。状态名与续点格式由清单绑定，保持展开次序及 pp 全变体图字节不变，不扩充 DSL。 本片已验：动作序列 1096 → 0，13 个 pp 图哈希不变，export --check 28 表/0 differ，DSL 9 op/上限 12、无新增 opts 键或值前缀。

### T1e 第二片：lex 输出与入口动作迁回模板

lex-gen 只保留字符类、词法分类、token 与拼写属性、前缀树等领域数据。输出动作及入口/NSTART 的控制行由 lexer 声明模板提供，维持原输出、状态与动作序列编号。复用既有 mapseq 的动作展开规则，不增加 DSL op、opts 键或值前缀；以五种 lex 图哈希不变验收。

### T1e 第三片：桥接、诊断与值栈动作归位

将 hostbridge 两架构 guards、k2-errors、k2-gen2、k2-stack 的动作序列迁回声明模板；facts 只保留测试操作数、诊断属性、布局常量与槽位数据。保持相关整图哈希，使用既有 mapseq/模板机制，不增加 DSL op、opts 键或值前缀。

### D2 拒绝边界核验（2026-10-04）
八个最小构造三方核验见 research/d2-product-refusals-20261004.md；区分产品独有拒绝、两侧共同缺口、头文件缺口与已接受。T1c 验证期间不改产品图；低成本 ttyname 需要头文件/生成物授权窗口，不能靠只改参考源宣称产品修复。

### csih 辅导观察（2026-10-08）

主人授权辅助 csih1、csih2；保持 file/exec 极简路线，以真实任务的故障和证据指导，不接管正在修改的源码。静态检查发现当前 `apps/csih/shell.c` 先阻塞 read/drain，后在 waitpid 循环计时，因此现有超时无法覆盖静默不退出或持续持有输出管道的命令；只 kill 直接子进程也不足以证明后代清理。此结论仅为源码审查，未跑超时探针。csih2 屏幕显示看客投递被拒后多次重复相同操作，其先前“没有 timeout”的报告已不对应当前磁盘源码；应更新证据，并明确区分磁盘源码与运行中旧进程。指导顺序：完成当前超时小件，再单独修复看客通信闭环，禁止同时改树与跑套件。

csih 管理恢复（2026-10-08）：两窗已消费指导，但写手被失败 slice 拒绝 answer/go:stop，看客被旧 peer `0:csih-tui` 的发信要求拒绝结束，造成重复无效回合；父会话取消两轮，不重启未验收源码。写手回执 `/tmp/csih-card-1-reply.md` 已实际落盘，确认阻塞 read 在超时之前；看客大段读取被中间截断，应改成短窗口。下一步将超时机制、短探针与通信恢复分件推进。

csih 小件 A 实跑（2026-10-08）：csih1 自己修改 shell.c/shell_cli.c，宿主 `env CSIH_EXEC_TIMEOUT_SEC=1` 下经 bound15 的 shell selftest 日志 `/tmp/csih-shell-A.log` 为 `selftest ok / EXIT=0`；csih2 已只读复核日志及主 read 前 poll。未证明后代清理、TUI 编译或全门禁通过。CLI 内 setenv 的测试曾失败；include/stdlib.h 使用每翻译单元 static override table，宿主传预算后通过，支持跨单元覆写可见性为失败原因。两窗仍在运行旧 harness，门禁拦停止导致重复回合，父会话取消；后续先修验证契约和通信，不重启未验收源。

csih 协调来源更正：投递工具先前继承了 `cdx-unisacc` 的来源标识，可能使回信进入另一个开发窗口；后续用 `csih-cdx` 来源标识，仅以 /tmp 回执及父会话抓屏交接，不要求两窗向其他开发窗口发信。

csih 轮末宏观死循环根因审查（2026-10-08，主人要求优先）：`agent.c:1703` 的红旗拒绝 answer/stop 与 `1737` 的看客未发信拒绝 answer 均在 `AT.action++` 之前 return PH_GO，既不消耗动作预算，也不终止；HTTP_END 的非 continue 响应被 judge 兜底当成 stop，而 continue 在每轮可重置 action。因此单靠 MAX_ACTIONS/MAX_ROUNDS/MAX_JUDGE 不能约束拒绝分支无限模型调用，屏幕已有重复红旗/stop与重复发信要求实证。优先补覆盖全部模型回复和早返回的硬预算，预算耗尽应报告未完成/阻塞，不能伪装完成；同时将“禁止宣布通过”与“允许结束失败回合”分开。冻结 shell 后续改动，交 csih1 实现及打桩验证；父会话不改 csih 代码。

### 优先件 B：轮末宏观死循环的有界退出与小件提示词校正（2026-10-08）

**冻结**：`shell.c` / `shell_cli.c`。只改 `apps/csih/agent.c` 与其测试/打桩，不改编译器，不重启。

**缺陷**：`agent.c` 中红旗拒绝分支（约 1703）与未发信看客拒绝分支（约 1737）在 `AT.phase = PH_GO; return 1;` 前未做 `AT.action++`，绕过 1813 的动作预算，故可无限请求模型；`HTTP_END` 非 continue 路径也不严格校验造成重复判定。

**方案（最小改动）**：给整个用户回合加统一的模型回复硬预算 `MAX_MODEL_REPLIES`，计数在任何解析/角色/红旗/看客分支之前进行，`HTTP_ACT` 与 `HTTP_END` 均计一次。预算耗尽时以明确的未完成/阻塞原因置 `PH_DONE` 结束，不再发起请求，且不置 `ok=1`。保留现有 JSON 字段兼容，仅收紧语义与状态转移：`continue`=确有下一步可执行修复；`stop`=结束本轮（可未完成/阻塞，但必须保留原因，不等同验收绿）；`answer`=报告结果，禁止“准备继续”冒充最终答复。轮末只接受当前阶段合法 go，其他内容按协议错误做有界纠错，不默认当 stop。移除“测试红永不停”“前两次 stop 不结束”这类无条件指令；测试红不得宣称通过，但可交付阻塞回执。未知/拒绝反馈需指出可执行下一步；重复同一拒绝最终结束未完成。

**验证（实际打桩，无真实模型）**：回归覆盖①红旗+重复 stop、②未发信看客+重复 answer、③正常工具/answer/stop 三路；断言请求次数有限且原因字段正确；外层与内层看门狗均 ≤60 秒。失败收尾与成功验收分开断言。

csih B1 独立验收（2026-10-08）：当前 agent.c sha256 `c8cfb19cba067f9c19c972b2aed94db3c3c2813c5649cc5daf7d0c6c981dee44` 在本地 HTTP 桩中正常 exec/answer/stop 用 3 请求 rc0；watch 未发信反复 answer 在128请求止于unfinished、rc1，源码测试期间未变化，证据 `/tmp/csih-review-B-results.json` 与对应日志。首次验证0请求/HTTP503为宿主代理路由，去掉代理并对localhost设NO_PROXY后通过，不能隐去此归因。B1仅证明硬预算兜底，不证明低重复收尾、红旗阻塞退出和两窗当前运行版本更新。下一小件B2由csih1修红旗stop的失败收尾，保留失败原因，不放宽通过判据；董秘建议的其余工作流改造仍为待审阅方案。

csih B2独立实跑：agent.c sha256 `770074ad9e9b0a862517567891d0d006e31fd9a38f51cf8c42069f3d02260b29` 正常收尾3请求rc0；临时工作目录内造源码写入、缺suite.c触发真实slice rows红旗，再stop，2请求rc1且保留原始失败原因；watch重复answer仍128请求rc1，故低重复角色退出未完成。证据 `/tmp/csih-review-B2-results.json`，测试期间源码未变化。下一片B3指导csih1让看客重复拒答低次数结束失败，stop也保留未发信原因；不要求父会话代写源。

csih TUI 编译拒绝归因更正（2026-10-08）：父会话复制 apps/csih 到私有临时目录，将 shell.c/agent.c/shell_cli.c 恢复为当前HEAD版本，在相同当前 unisacc.com 下 bound15 编译并运行完整15单元TUI selftest，仍 rc1 `reject: not covered: ARM64 operand or instruction / arm64: __ccw_::`；日志 `/tmp/csih-tui-baseline.log`。因此不能将此拒绝归因于本轮csih超时或B1/B2新增代码；对照只隔离这三文件，未证明整个编译器/头文件发行基线。现有旧TUI进程不重启；核心编译器问题只记录，不擅动其他开发者路径/窗口。

E59：for 步进字符串池按实际发出顺序（体先、步进后）递归遍历；表内记录 token 边界，不截断 token 流。规则与回归见 [pool-order](exec/parse2/pool-order.md)、`tests/c/b_forstep_strings.c`。

csih B3独立实跑（2026-10-08）：当前source身份见 `/tmp/csih-review-B3-results.json`。五例正常3请求rc0、真实slice rows红旗stop2请求rc1、watch未发信重复answer2请求rc1、watch未发信stop1请求rc1、watch用户不用工具answer/stop2请求rc0，全通过且测试间源码未变。B3由csih1修改，父会话仅调度/只读审查/本地桩验证。永久回归探针与现有两窗运行版本更新未完成，TUI当前编译拒绝仍单列，不宣称完整harness恢复。

csih 指令意图误判（2026-10-08）：agent_user_forbids_tools只strstr匹配三句禁用工具用语，不区分引用/测试场景/当前真实命令；管理任务内包含watch_no_tools测试用语时也触发AT.no_tools，解析失败后agent_take_prose会把整段拟议file JSON当answer。窗口已出现写文件JSON作为answer、探针未落盘的症状；源匹配条件与当次指令包含字面串已核对，模型具体意图未独立证明。先改管理任务表述消除触发，禁用意图识别修复另立小件，不弱化用户真实禁用工具的优先级。

多单元 static 声明：units 预扫描需保留指针限定符 const/volatile/restrict，不能把合法指针声明当未覆盖；修复与三方回归由 cdx 跟进。`.cx` 的可移植方向是契约检查/模块转换前端输出普通 C99，再接外部 C 编译器；仅忽略 pragma 不构成安全保证。

csih H1热更新需求（主人2026-10-08）：当前/reload仅在空闲时清密钥缓存，mind页逐帧读；运行中的C源码不更新，既有self-iter只安排门禁绿后人工重新拉起。主人认为代码热更新有助快速迭代，新增H1待实现：显式/reload-code先构建验证候选，再在回合边界保存会话/目标/cwd/角色/peer，自动换版并恢复；失败保留旧运行版本与原因，显示运行源码身份以免旧进程冒称新代码。优先用受控进程换版实现体验，不先做原生内存热补；未来可考虑稳定TUI+可换agent执行进程。配置/reload含义保持明确。当前TUI编译拒绝是启用前阻塞，不能绕门禁或宣称已支持。H1先由csih1完成最小设计与状态清单，再实现并实跑保持状态/候选失败/忙时行为/重复换版探针。

csih H1设计交付核对：csih1已实际写出 `apps/csih/docs/code-reload.md`，明确未实现/未实跑；B3错误回执已追加更正，旧记录保留。设计当前覆盖用户目标、回合边界、状态清单、失败回退、首次部署和验收，但尚无具体状态格式/版本校验、候选ready与TTY接管握手、冻结候选的启动路径及看门狗，不能当可直接实现的完整方案；下一步由csih1补可执行机制，再由csih2独立审查。

开发效率观察（cdx）：tests/graphhash.py 的 --only parse2/units 当前选中0项且退出0，exec/parse2仅选8张E3图不含units；应支持明确阶段键并拒绝空选择，避免误验与依赖全局分片编号。候选构建期间仅报告，不修改门禁输入。

csih 阶段验收收口（2026-10-08）：永久探针 `apps/csih/probes/agent_loop_bound.py` 已由csih1写出并实跑，父会话复跑五例全PASS，测试前后app全部C/H、探针及unisacc.com哈希未变；证据 `/tmp/csih-loop-final-review.json`、`.log`。H1 canonical `apps/csih/docs/code-reload.md` 已补session/handoff/candidate身份、私有状态文件、最终快照边界与I/O所有权、ACK不明暂停及独立诊断、首次部署与验收，明确仅设计未实现/未实跑；误写的H1.md加旧稿提示，原内容留存。csih2独立审查指出共享状态/锁、日志身份和过时快照缺口，已交写手修订；监督恢复和完整热换版仍需实现与验证，export/resume只是阶段，不代表H1完成。两窗仍旧版本，父会话取消收尾空转，没有擅重启或改csih代码。

csih H1-S1安排（2026-10-08）：当前unisacc.com sha256仍为e0a8ff7c1be9165eb29509b630c820a642d7683a13a7c51c5e36649d855cc36c，完整TUI换版未解除。先由csih1实现独立的状态v1校验库/CLI并在unisacc实跑，随后推进私有原子状态文件、恢复接入与候选交接，S1不代表H1完成。状态须保留TUI最多8条pending消息队列（每条4095字节）以及当前4095字节input/goal；只保存input会丢忙时已提交但未执行的消息。candidate/session/handoff身份和journal路径/整数偏移属于校验契约；非法/未知/重复字段拒绝，不借此改核心编译器或替换目标运行路线。

csih 大写入协议边界诊断（2026-10-08）：agent.c的AGENT_CONTENT_MAX=4096，content/stripped/one均4096，动作text/nw字段也4096；at_after_http用snprintf把完整模型content拷入4KiB后再解析，超长JSON会变成不可解析，但反馈只让模型重发而未说明截断。H1-S1一次性大C写入连续无法解析，符合此确定性容量缺口；具体每次原始HTTP内容长度尚未独立抓取，不能声称所有解析失败都源于容量。管理指令改为每步UTF8 JSON≤3000字节，代码片段≤1500字节，通过单一NEXT标记精确edit分块；后续需明确工具协议容量与有界超长反馈，而非任意扩大缓冲。

csih H1-S1静态审查（2026-10-08）：reload_state.c已落盘但未测试，发现goal/input错误复用身份白名单，带空格或换行的真实文本被拒；root journal/pending_queue未检查重复；身份校验放行所有非ASCII字节；offset上限2^53导致2^53+1经double舍入后可能误收。先交csih1在该库修正四处，再补独立CLI与实际边界测试；父会话不代写csih源，落盘不等于通过。

csih H1-S1四处修订已只读核对落盘：任意goal/input文本改用rs_text_field，所有必填根字段检查唯一，身份拒非ASCII，上限改2^53-1。本轮csih1耗尽16动作（11读5edit）无回执，不能因此假定失败或重做；下一件限定单个小CLI check FILE，独立验收提供JSON输入，不再为确认读满预算。仍未实际测试。

csih H1-S1独立实跑（2026-10-08）：csih1编写reload_state_cli.c check FILE后，父会话使用当前unisacc.com运行json.c+reload_state.c+CLI，19例全通过：中文空格换行、队列8/9、文本4095/4096、未知/缺失/重复journal与pending_queue、版本、身份/hash/绝对路径、负数/小数/2^53-1与2^53及2^53+1。输入哈希前后不变，逐例返回码和原因见 /tmp/csih-state-S1-review.json。只证明状态校验，不证明文件持久化/恢复/热换版；下一交付永久探针，再推进私有原子状态文件。

csih S1永久探针首次实跑失败（2026-10-08）：reload_state.py 17例均失败，cwd=ROOT且相对json.c导致cannot open json.c；静态核对base还把version写字符串且缺handoff_id/candidate_hash/cwd/role/peer。此为探针构造/调用错误，不撤销先前独立19例库验收；探针非零且检查FAIL前缀避免空验成功。交csih1修绝对compiler+cwd APP、完整合法fixture并补2^53+1/hash/cwd边界。

csih S1永久探针父会话复跑20/20通过（2026-10-08），包含原因断言与超时失败；S1格式验证收口，H1未完成。csih2两次收到S2审查安排后仍反复answer不发/要求发信，没有审查产物，取消该空转；不能报告为正在审查。S2先由csih1交一份具体API与失败语义契约，随后实现私有文件save/load/consume，consume不能在恢复提交之前自动删除。现有头文件提供open/O_NOFOLLOW/fstat/fsync/unlink，具体实现仍需当前unisacc实跑验证。

csih S2契约审查（2026-10-08）：reload-io.md纯设计已落盘，save/load/consume语义方向正确但签名误把why写const、cap写char*，不符合既有why缓冲+size_t容量；load尚未返回同一文件证明给consume，单靠JSON三身份不足以证明同一inode。先纠正类型和读取凭据，再实施save单件。目录fsync后失败须区分已rename但持久性未确认；恢复已提交与清理失败须分别报告，不允许将清理失败解释为恢复可重试。

csih S2-save实现授权（2026-10-08）：设计类型修正已核对；load签名仍漏token显式参数，留待load单件补，不影响save。csih1下一唯一源码reload_io.c，仅实现save，私有父目录校验、O_EXCL/O_NOFOLLOW创建0600临时、短写/EINTR、fsync/close/rename与目录同步、rename后-2单列；不实现load/consume或声称热更新。可信祖先/同session单写者是明确前提，尚无锁/监督证明。

主人输入键位决定（2026-10-08）：上下键应调出用户输入历史供修改再发送，消息翻页另设快捷键。当前tui.c上下键仅在空input时滚日志，不满足；拟改Up/Down输入历史，PageUp/PageDown滚消息并保留鼠标滚动，组合键依终端支持核对。H1仍未实现：S1状态校验20例实跑，S2-save刚落盘未验收。静态审查save发现将validate成功1当失败拒绝、缺stdio声明、rename后目录close失败返回-1错误暗示未提交，需写手修正后CLI实跑；不得宣传热更新已可用。

csih S2-save静态修正已核对，父会话将reload_io.c与json/reload_state/现有CLI一起在当前unisacc编译执行合法格式例rc0，日志/tmp/csih-save-compile.log。该调用未执行save，不算保存验收；下一csih1交save CLI，随后独立临时目录验证内容/0600/失败保旧文件。

csih S2-save独立实跑六例通过（2026-10-08）：当前unisacc+saveCLI在临时目录验证保存中文input/队列和0600、坏JSON保旧内容、父目录0755拒绝保旧、父目录symlink拒绝保旧、目标目录造成rename失败且清己临时、重复保存替换成功。源/编译器哈希未变，证据/tmp/csih-save-review.json。未注入短写/同步/close失败，-2分支尚无实跑证据；load/consume/TUI换版未实现。下一先兑现主人Up/Down输入历史键位，随后回到H1恢复接管。

csih 输入历史任务定位纠偏（2026-10-08）：csih1已执行ls/find寻找并不存在的tui.h，源码尚无history改动；父会话取消该定位并提供tui.c内struct/enqueue/submit/key/selftest定点行号，将第一片限定状态+helpers，后续接入/测试分片。不将已发任务当实现进展。

csih 输入历史首片实际落盘：tui_state含16条history和draft/cursor，add/move helpers已只读审查，尚未接入。发现move返回末尾未退出browsing会使下一次Up恢复过时草稿；连续去重早return未重置浏览。交写手修正并通过手工提交wrapper接Enter/paste，不放到自动tui_submit路径重复记录系统/dequeue/loop。

主人内容高度需求（2026-10-08）：内容区相对高度适当增加。当前TUI_LOG_VIEW=20固定上限，在高终端留下额外空白；frame最多63行，日志ring40。拟在不越frame/终端限制下将内容预算随终端高度增长，避免只改渲染不改滚动预算；展开system10/mind12仍须给输入和正文保底。先完成正在进行的输入历史接入，再做高度单件和24/40/60行布局验证，不擅改其它窗口。

csih 输入历史键位接入已只读核对：Enter/PASTE_OFF调manual wrapper，Up/Down调history_move，尚未测试/部署。Up/Down在pasting分支额外插入换行未经需求授权，应改忽略箭头不篡改粘贴文本。下一件补历史实质测试与内容上限40（现ring40/frame63，实际room仍按窗口预算）；原20行滚动测试用27行窗口保留原条件，另检24/40/60窗口内容增长和frame边界，不用改上限导致旧断言误红。

csih 高度件进展核对（2026-10-08）：TUI_LOG_VIEW40和paste箭头忽略已落盘；csih1再次16动作耗尽，多为读取/grep定位，历史与24/40/60新测试尚未交付。编译器哈希仍原e0a8，完整TUI阻塞未解除，不做重复无效构建/窗口重启。下一件进一步限定仅一个历史selftest块，明确插入锚点，减少定位。

csih 历史测试再次受容量限制（2026-10-08）：窗口出现无法解析且测试尚未落盘，先前AGENT_CONTENT_MAX4096问题未修；父会话进一步限定首次测试仅A/B手工提交断言<900字节、HIST_NEXT标记后续分片，避免重复大edit。当前仍不能声称历史/高度已验收。

csih 历史测试回执与盘不符（2026-10-08）：csih1称113579字节含A/B、中文草稿、满队列测试；父会话立即读取实际tui.c为112612字节，全文件检索无tui_state h/HIST_NEXT/相关FAIL，selftest声明后直接init st。不能验收回执所称块，原因尚未证明（可能错误回忆或未落盘），要求保留原回执追加更正，再用真实int failures=0, cols=40锚点精确补写，禁止改归因。

csih 历史测试A/B实际落盘核对（2026-10-08）：selftest声明后新块含busy1手工Enter A/B，断言nhistory2/npending2和history原文，唯一HIST_NEXT。此前不符回执不作为证据，此次真实源码已证明小块存在，尚未实跑。继续按标记小片补草稿及拒绝路径。

csih 测试去重安排（2026-10-08）：queued锚点更正与旧盘不符任务先后被执行，导致A/B块两份与HIST_NEXT两处；当前源码证实重复，csih1主动回报且未删除。父会话授权只删除第二份该agent自己的完整重复块，再补草稿片；不应把“不要清他人修改”扩成无法清自己重复插入。

csih 历史测试去重及草稿片盘面验收：父会话读取selftest确认仅一份A/B块，中文草稿Up/Down、新草稿X和log_skip断言实际存在（未实跑）。HIST_NEXT位于草稿片之前，下一拒绝测试需恢复npending以不改变已有A/B草稿前提；旧滚屏rows54尚需调整，不能称测试完整。

csih 满队列拒绝测试已核对落盘；旧滚屏高度54→27的edit匹配失败后未完成，父会话提供含goal_at唯一五行精确上下文再次交写手。此处不能用单行匹配两处wide.rows54；未获执行证据仍不宣称全套测试完整。

csih 旧滚屏测试修复回执收到：csih1报唯一上下文54→27成功，下一安排selftest布局3例24/40/60对应room17/33/40，检查frame<=rows/63、宽度和HEIGHT_DRAFT输入可见。布局块只落盘，完整TUI测试仍需实际跑，不能把回执当通过。

csih 布局块初次插入存在struct tui_state与裸HEIGHT_DRAFT错误，随后当前盘面块已移除，未交有效测试。下一缩为单一24行用例，明确typedef和字符串字面量，先正确落盘后扩40/60，不以拟议测试称完成。

csih 当前完整UI冻结副本验收（2026-10-08）：15源当前unisacc selftest rc1，ARM64拒绝 .frame1011688，未进入测试，证据/tmp/csih-ui-current-review.json。与先前__ccw_不同，本次显示约1MB函数栈；history16x4096增加各tui_state大小，selftest多局部state累计可能诱发该帧（推断未隔离证明）。下一由写手只调整selftest状态为各自static+原init，缩栈后重验，不改编译器或运行窗口。csih1标题编译通过但正文cc语法仍报mkdir冲突，不能当完整编译绿。

csih selftest缩栈独立验证（2026-10-08）：写手只将selftest16局部state改static，父会话冻结副本15源重跑，frame1011688拒绝消失，仍rc1 ARM64 __ccw_::，证据/tmp/csih-ui-static-review.json；未进入测试，不能宣称历史/高度通过或部署。当前UI停留已有编译阻塞，继续H1可独立验证的load/consume，不改核心。load API须明确返回token，用dev/ino+三身份，可信祖先/单写者前提仍有效。

csih load接口头实际审查：reload_io.h已落盘含dev/ino+三身份token，save why错误const再次出现，交写手改为char*。下一独立reload_load.c实现私有父目录/目标权限属主、完整读取EOF、格式三身份校验、成功才输出token/len，失败不删状态；无恢复或接管副作用。仍未实现/未测试。

csih load首片API静态审查：初始reload_load.c把validator/json_parse省略why/cap参数、jstr误声明int+out参数，与实际源不符。先给准确原型并允许定点读取接口；禁止读取过多不应变成省略必要接口核对，继续使用伪API会导致ABI/调用错误。尚未测试。

csih load声明修正已盘面核对；部分body已至out_cap检查，read_all含短读/EINTR/EOF，资源尚待统一done。静态发现缺绝对路径/预期三身份非空参数检查，父目录根路径处理为点不符合契约。下一写手补guard并完成parse/身份/token/清理尾段，复用真实API，仍未测试。

csih load尾段静态审查（2026-10-08）：已落盘，但JSON取值使用session/handoff/hash而schema为session_id/handoff_id/candidate_hash，token同样引用不存在短名成员；合法状态会缺字段或编译失败，需修既定名字，不扩白名单迎合错误。目录close失败清输出但why残留ok，也交修。其余读取/EOF/关闭/格式校验/失败不删语义已核对，未实跑。

csih load字段恢复已核对；父会话将json/reload_state/reload_load/既有校验CLI在当前unisacc编译并跑合法格式例rc0，仅证明模块可编译，未调用load。下一独立loadCLI输出载荷与失败清零标志，临时私有目录边界验收，不引入恢复/接管副作用。

csih loadCLI落盘静态审查：调用/失败哨兵检查与载荷输出已在，但static struct reload_io_token错误使用匿名typedef的不存在tag，交写手一行修为static reload_io_token。未编译/实跑，不修改公共schema迎合错误。

csih S2-load独立实跑10例通过（2026-10-08）：合法中文/草稿/队列载荷逐字节相等且token dev/ino对真实stat；三身份各错、cap1/0、目标0644、symlink、缺字段JSON、父目录0755均rc-1/cleared1，全部状态文件保留。源与编译器哈希未变，证据/tmp/csih-load-review.json。只证明load不恢复会话、不接管TTY。下一consume在恢复已提交后校验load token同dev/ino再unlink；无外部替换/同session单写者为明确前提，删除后同步失败需单列已删除未确认持久，不假称可回滚或重做恢复。

csih consume任务停在读取/静态helper复用讨论，文件尚未创建；需要独立新模块调用公共reload_io_load，不改已有static helper。父目录检查/why可在新模块小幅重复，不应把不要改load理解为无法实现consume。下一限定先新文件prolog和函数骨架，不重复查位置。

csih consume骨架已只读核对落盘1435字节，真实typedef/token/签名和私有cwhy，尚未body。下一在独立文件内完成load验证、dev/ino比较、可信父目录再检查、unlink和fsync/close，删除后失败-2。输入token三数组需要有界NUL检查，避免不合法调用者token让strlen越界；仍假定可信祖先/同session单写者无外部替换。

csih consume body已盘面审查完成，检查token NUL、复用load、dev/ino、私有父目录、unlink/fsync/close返回边界；父会话组合json/state/load/consume/loadCLI在当前unisacc编译并合法load rc0，尚未调用consume。下一consumeCLI实跑direct、旧inode凭据、错身份、无NUL token、加载后原路径实际替换，全部仅临时目录。getuid/geteuid差异无本机普通用户影响，setuid运行契约未验证。

csih consumeCLI静态审查：调用误扩成9参数而公共consume仅(path,token,why,cap)，缺argv1 consume检查，非replace仍允许argc8，bad-id memcpy不显式NUL。先交写手修签名与参数门禁，不扩库API迎合测试错误，尚未实跑。

csih S2-consume冻结副本6例通过：direct真实删除；stale inode/bad-id/无NUL token拒且原文件保留；load后rename实际换同身份新inode拒且新文件保留；换坏JSON拒且坏文件保留。证据/tmp/csih-consume-review.json含冻结输入哈希。该冻结CLI可能早于最后argc门禁修复，六例均完整合法argc，无用法门禁验收宣称。save/load/consume独立验证完成初步，不代表恢复或热换版。下一真实状态映射必须包含历史新字段及pending队列、journal持久偏移，恢复只能在空闲边界，候选不能读TTY/写journal直到所有权确定。

csih 会话映射只读事实：tui_state匿名typedef仍在tui.c122起，transcript const char*约144，goal175，input/pending/history在125..133；不存在tui_state.h。session.c78 session_append fwrite/fputc/fclose，没有fsync或已提交偏移getter，注释提fflush但实际无显式fflush，fclose会flush但不等于持久化。真实恢复需补日志边界证明，不能凭路径/文件大小宣称已提交。交设计写手以此为准。

csih reload-session.md设计实际落盘，字段及缺口基本真实；version写成字符串2，与既有数字version契约不符，首节非2拒与v1可降级相矛盾，导出步骤漏input/pending/history需补齐。下一先实现独立journal_checkpoint(path,offset,why,cap)，空闲/单写者前提下打开实际日志fsync后fstat给安全范围offset，失败不提供边界，不用调用者随意填写偏移替代同步证明。仅checkpoint不等于读日志恢复。

csih journal_checkpoint盘面审查：sync后fstat/close才写offset实现已在；NULL offset仍会返回成功，需按有效检查点输出契约拒绝，初始NEXT注释需收口。下一写手小CLI，临时空/多字节日志偏移与相对/缺失/链接/目录拒绝，未进行日志重放恢复。

csih checkpoint首轮独立验收6例均未执行：当前unisacc reject unexpected character，源码顶注释第一行提前*/，后两行星号文字暴露为C代码，非checkpoint运行失败。证据/tmp/csih-journal-checkpoint-review.json，源哈希未变。交写手只修第一行注释闭合位置，随后复跑。

csih journal_checkpoint重跑6例通过：空日志offset0、中文18字节offset18；symlink/目录/不存在/相对路径拒offset0，原日志不变、源哈希未变。证据/tmp/csih-journal-checkpoint-review-2.json，首轮词法拒绝日志保留。下一v2状态15字段（v1共11+history/history_pos/history_browsing/history_draft），公共v1校验语义保留，独立v2严格入口；history数组<=16、文本4095，pos/browsing约束符合真实cursor。IO支持v2尚待随后接入，不能称v2支持或恢复完成。

csih v2首轮16动作耗尽，实际partial重构已在：按版本白名单/必填表及history helpers，共用rs_validate_version；公共reload_state_validate wrapper尚无、版本判断仍固定1、history校验尚未接入。当前partial不能当v1可用，需要下件优先恢复wrapper+完成分支，再v1永久回归。history_pos helper大数先转long long可能越界，须先限制<=history_len再转换。


### csih v2 验收盘面更正（2026-10-08）
父会话读回 reload_state.c：v1/v2 公共入口与版本分支已在；history_pos 仍先转 long long 后检查上界，与 csih1 回执不符，待其精确修正。checkpoint 修正后独立六例通过，证据 /tmp/csih-journal-checkpoint-review-2.json；不代表热更新已实现。

csih v2 冻结验收：csih1 已正确将 history_pos double 上界检查前移；私有快照 v1 20/20，v2 十例全部通过（含 1e300、布尔类型、历史长度与浏览位置约束），源码前后相同。证据 /tmp/csih-v2-review.json。下一件只扩展现有 CLI 的 check-v2 入口；IO 仍仅接 v1，热换版未完成。

csih CLI 冻结三例通过，证据 /tmp/csih-v2-cli-review.json。决定新增严格版本派发 reload_state_validate_any，仅使存储层识别完整 v1/v2，不自动将 v1 当作完整 v2 恢复；恢复迁移须显式选择。下一交付先只在 reload_state.c 添加公共派发函数，随后独立验收与 IO 接入。

csih 派发函数审查发现真实 API 不匹配：json.c 的 jnum(jvalue*,double) 返回 double，新增函数却传 &ver 并当布尔返回。要求只纠正为 J_NUM 后读取 v->n，double 严格比较1/2再确定整数分支，暂不接IO。

csih reload_state_validate_any 修正后冻结十例全通过，证据 /tmp/csih-any-review.json，含 v1/v2、未知/巨数/小数/字符串/缺失/重复版本、v2缺历史、v1多历史。下一件仅将 save/load 声明与调用切到该入口；消费经 load 间接支持，原文件规则不变；不声称完成会话恢复。

csih v2 IO 冻结四项通过：0600保存、完整中文历史/草稿/队列逐字节加载且不删、坏版本保存保旧、消费清除；证据 /tmp/csih-v2-io-review.json，源码未变。下一步会话 codec 的结构与接口，最终须接真实 TUI export/resume 与交接，JSON往返不是完成条件。

csih reload_session.h 已读回，字段覆盖v2全部会话快照；接口仅声明，尚无codec。下一交付只做decode：先完整v2验证，再解析映射全部字段，失败清零，JSON树每路径释放，大状态不放局部栈。

csih 状态新边界缺陷（独立实跑 /tmp/csih-nul-review.json）：v2 goal 含 JSON \u0000 的完整文本仍被接受，但会话结构C字符串无法保留NUL后内容；codec验收必须包含此例，不能宣称无损恢复。待decode交付后处理，仅修csih状态入口，不擅改共享json库。

csih decode 首版已完整落盘但未验收：MAX_ACTIONS16后停止；发现违规栈上 reload_session_state tmp，以及jget/jstr声明缺const。安排直接写已清零out（完整验证和第二次parse成功后才开始赋值），移除大栈临时对象，不改JSON库。

csih decode 冻结四例独立通过：全字段中文映射、三类失败输出全零，证据 /tmp/csih-decode-review.json，源码未变。下一件在csih状态公共验证入口拒绝原始NUL和真实 JSON NUL escape，保留双反斜杠的字面文字，不改共享json库；避免codec silently截断。

csih NUL 修正冻结九例通过，证据 /tmp/csih-nul-regression.json：身份/goal/input/draft/history/queue实际NUL拒，原始NUL拒，字面反斜杠u0000与中文保留。下一唯一交付 encode_v2，复用真实json_escape并严格检查数组终止、计数和输出容量，生成后v2校验；不把部分JSON当成功。

csih encode 调度纠偏：首次大动作无法解析，随后重复读取而无产物。保留完整encode目标，停止该回合，先交付同文件小型builder基础，后接字符串转义及全字段主函数；每片显式内容上限。

csih encode builder首片1375字节读回已在，无完整公共函数。审查真实json_escape发现非法UTF8会跳过字节且in_used仍前进，单查in_used不足以保证无损；下一字符串helper先严格UTF8验证，再转义并核容量，继续同一编码交付。

csih encode 字符串helper读回已在：memchr终止、严格UTF8、json_escape完整消费、容量累计；仍无主编码函数。下一件同文件补全部15字段和失败清零，再冻结roundtrip验收。

csih codec 冻结八项及结构→编码→save→load→decode→consume 全链独立通过，证据 /tmp/csih-codec-review.json、/tmp/csih-session-chain-review.json；大结构全字节一致，源码未变。实际热换版仍缺TUI接线和可启动候选。调度csih2只读定位现有完整TUI ARM64未覆盖，不改编译器/他窗；csih1补可独立复用的codec CLI以留永久验收入口。

csih CLI 回执拒收：父会话stat/读文件返回不存在，窗口仅有发回执exec，无write；未把宣称计成交付。csih2仍不发循环，无诊断产物。取消两轮空转，重派具体首步并要求事实更正。

csih1 CLI首片实际588字节已读回，旧“完整已建”回执仍无效。csih2明确说明原窗口用户规则只允许对0:csih-tui capture/envelope，因此源码诊断不在其权限范围；撤回该任务，保持其受限职责，TUI诊断转csih1，不覆盖窗口原规则。

csih session CLI 首次实跑全轮无效：undefined decode_v2/encode_v2，未进入main；两拒绝例退出码碰巧1不能算通过。保留日志 /tmp/csih-session-cli-review.json，要求只恢复真实函数全名后重验，失败例须检查FAIL原因避免编译错误假绿。

csih session CLI 修正完整函数名后冻结四例通过，拒绝例检查FAIL+原因且无编译stderr，证据 /tmp/csih-session-cli-review-2.json。下一交付csih1只读冻结完整TUI编译诊断报告，解决可启动候选的真实阻塞；csih2源码任务已撤回。

父会话独立冻结TUI复现rc1 __ccw_::：快照 /tmp/csih-tui-verified-tdv57l6q/app，编译器在上一层；全命令/hash/stderr证据 /tmp/csih-tui-verified.json。诊断任务转到该明确快照，避免缺符号试验与路径猜测。当前src/main.c确认-S为写出汇编选项，下一试验用冻结源导出查看异常标签。

csih TUI报告已读回 /tmp/csih-tui-build-blocker.md：-S缺14外部符号未产汇编属真实限制；“只有15源/14源不触发”与删clock/json/plugin仍ARM64拒绝矛盾，缺符号早拒不证明无后续bug。要求追加更正保留原文。只读src/front_parse.c cca_emit显示__ccw_后缀由en(i)生成，异常冒号是否为计数命名问题尚属假设，下一隔离wrapper数量实验。

csih TUI 阻塞取得最小边界复现：同冻结compiler，单C文件getaddrinfo host调用+函数地址数组，1/9/10项rc0无stderr，11/12项rc1 __ccw_::。证据 /tmp/csih-wrapper-count-review.json，探针 /tmp/csih-wrapper-count-836hdnpt。无需15源、非csih新codec；结合cca_emit第0起计数，异常在索引10，具体生成算术根因尚待证明。安排csih1独立复核并核对TUI实际函数地址集合，不擅改编译器/规避门禁。

csih 管理纠偏：csih1未产复核收据，又重复寻找报告，需重给绝对路径并首步落盘。只读工具表发现七项fn指针（sh/cd/edit/read/write/append/ls），可在私有快照实验改整数id+直接switch，保留全部工具及语义以减少生成wrapper；仅实验，不修改仓库或降低门禁，未声称可用。

父独立私有等价分派复跑：源码仅tools.c不同，__ccw_::消失、进入selftest，rc1，仅FAIL log home skip20（无log view失败），证据 /tmp/csih-tools-dispatch-review.json。盘面fixture保留40条日志、rows27实际viewport20，HOME clamp最大20，断言40错误；PGDN20回尾成立。下一只在私有tui自测修fixture预期20，不改生产滚屏逻辑，重验。

私有TUI完整selftest父独立通过，rc0/noFAIL/selftest ok/stderr空，源码前后不变，证据 /tmp/csih-tools-dispatch-review-2.json。已验证仅两项变化：tools整数id直接分派保全部工具，以及selftest HOME fixture预期40→20。下一让csih1主树精确落同两项（检查tools基线hash），不整文件覆盖；主树仍待重新冻结验收，未部署。

csih1 主树落盘任务暂未执行：主树仍fn表/HOME40，pane停旧任务；PID38793活着，状态Ss+、0%CPU，cwd及stdin/out仍原TTY。Ctrl-C与解除可能终端流控后无可见进展，屏幕出现NFS localhost not responding/is alive again系统消息（不证明具体阻塞根因）。未杀/重启、未重复覆盖源码；待确认恢复后续同一落盘任务。私有selftest全绿证据仍有效但不能冒充主树验收。

csih1仍未恢复，PID38793采样 /tmp/csih1-stack.txt 为JIT未知地址，不能据此认定NFS根因。按AGENTS其余任务可用code-review代理及主人避免父会话直接改源码的要求，委派专职代理落实已私验两路径小补丁；仅本补丁一交付≤10分钟，精确匹配并检查并发相同修改，父独立验收，不提交/部署。

主树两项经专职code-review代理落盘并父独立冻结完整TUI自测通过：rc0/noFAIL/selftest ok/stderr空，主树前后未变，证据 /tmp/csih-main-selftest-review.json；快照 /tmp/csih-main-verified-f6vvj92w。候选可编译已恢复，但运行旧窗仍未更新，export/resume/交接尚未接TUI。

csih TUI恢复接线发现loop_on缺口：当前v2十五字段goal文字不等于持续执行开关，必须保留loop_on才能忠实恢复。v2尚未部署，决定增加严格必填JSON bool loop_on成为16字段；v1入口保持不变，旧缺字段v2明确拒。codec结构新增int loop_on、decode/encode全映射（值只0/1）；先专职代理补接口与语义、独立往返，再接TUI，不把丢执行开关当恢复。mode由启动命令明确保留，不能默默从role推断。

同一恢复语义补充：tui.loop_left为剩余自动重提交预算，真实上限TUI_LOOP_MAX8。v2同时必填loop_left数字精确整数0..8，总17键；bool loop_on+left0是末次目标已排队的合法状态，不额外拒。codec完整保存两字段以免换版重置轮次预算。已实时转专职代理，尚待验收。

自动执行状态父独立冻结验收：v1 20/20、session17字段12/12、完整TUIselftest rc0，全源码前后不变，证据 /tmp/csih-loop-state-review.json。下一TUI helper接线：保留旧15源启动argv，csih.c以私有support include聚合三个codec库，JSON restated类型用共享guard防同TU重复；capture带显式metadata，apply先验证再原子更新UI选定字段，保存goal/input/pending/history/loop_on/loop_left及拥有journal路径。cwd/role/peer/身份启动上下文与TTY/journal所有权不由helper偷偷改变，命令和换版仍后续交付。

TUI capture/apply与私有codec接线专职交付已完成：旧15源argv不变，代理完整selftest、v1 20/20、session 12/12通过，收据 /tmp/csih-tui-reload-helpers-review.json 与 /tmp/csih-tui-reload-probes-review.json。父读回helper确认先验证后更新、journal路径由state拥有，并用本树原15源命令、15秒看门狗独立重跑rc0/selftest ok。首条父命令因cwd下tests路径错误rc2，修正../../tests/bound.py后通过。此仅UI状态capture/apply，不是export/resume命令、启动上下文恢复或热换版；未重启原窗口。

下一交付为TUI状态文件桥：保留十五源argv，将现有save/load/consume/checkpoint纳入私有support；export显式身份metadata+真实日志checkpoint后capture/encode/save；prepare-resume仅load/decode并核对日志当前长度与offset一致，返回状态+消费token，不apply、不consume、不碰cwd/env/TTY。为后续命令和所有权交接提供真实文件链，不能称热更新完成。原csih1 pane仍原请求画面、PID存在，不以画面静止判进程终止、不重启。

状态文件桥父独立冻结实跑通过：/tmp/csih-file-bridge-parent-review.json，完整旧十五源selftest rc0/selftest ok/stderr空，源码前后hash一致。export复用真实journal_checkpoint、capture/encode/save，prepare复用load/decode/checkpoint要求当前日志长度严格等于offset，失败清out/token、不消费；prepare会fsync日志，未append/截断。新增测试真实私有0700目录覆盖中文/队列/预算往返、busy保文件、错身份保文件、日志变化拒恢复。代理第一轮fixture mkstemp undefined已保失败收据，改mkdir夹具后绿；v1全20例15秒超时尚待拆批，不算全绿。命令入口和所有权交接仍未实现。

文件桥v1拆批回归已实跑20/20：/tmp/csih-tui-file-bridge-v1-batches-review.json，完整20首次15秒超时保留。进入公开恢复入口前必须补真实跨进程独占所有权，否则两个新旧进程都可读TTY/写journal。下一单件reload_owner库用POSIX fcntl进程记录锁，私有可信0700会话目录与0600普通本人锁文件、不跟随链接、不自动删锁/清陈旧状态；完整fork实测竞争/释放/进程退出释放。该锁只约束参与新协议的进程，不能证明旧未接入窗口已停；首次部署仍需单独受控bootstrap。

原csih2当前pane现场：rounds=110/actions=1/judge ok，末尾显示envelope目标0:csih-tui不存在(exit1)；因此这是旧运行体错误验收的直接证据，不能作为邮件送达。当前已空闲而非请求中，不追加超出原角色权限源码任务；源码B3门禁的绿不能冒充该旧运行体已升级。

watch邮件验收当前源码确证漏洞（非仅旧窗）：agent.c agent_watch_note在agent_exec之前仅strstr(cmd,"envelope")及peer就置mailed。父本地HTTPstub执行无发信命令 false # envelope 0:csih-x，exec exit1后answer/stop仍rc0，证据 /tmp/csih-watch-false-mail-review.json。原五例只覆盖没发送动作，不能证明执行失败不得过门禁。下一优先修复须以真实成功回执+准确单条envelope目标验证，不可只把note挪后/仅看shell复合命令退出0（false;true、echo/comment都可能伪造）。保留失败探针证据，未发任何真实邮件。

邮件修复单件决策：送达标记必须在exec_run拿到真实shell_result后，要求ok/exited/status0/无timeout/无err，再匹配可信绝对/Users/wjc/repos/moltbaby/bin/envelope的单条无shell组合调用、准确peer参数和真实envelope成功回执行。公共旧agent_watch_note(cmd)不得仅凭命令置位；CLI自测改真实结果语义。不因echo/注释/false;true/错目标/exit1/缺回执等放行。不修改外部envelope。正例先本地真实结构单元测试（不能冒充实际投递），负例HTTPstub无真实邮件；后续再验真实投递。

reload_owner父独立复跑永久探针通过：python3 tests/bound.py 15 python3 apps/csih/probes/reload_owner.py rc0，8个CLI场景/3个真实fork阶段，源/compiler hash不变。代理原证据 /tmp/csih-reload-owner-review.json。尚未接TUI、不证明旧窗受保护/跨exec交接/close故障；正派发邮件验收补漏修复。

watch邮件门禁修复专职交付11例拆批全部实跑通过（/tmp/csih-watch-mail-regression-review.json），CLI结构单元通过（/tmp/csih-watch-result-unit-review.json）。父独立复跑原漏洞false注释/echo伪造回执/false;true三例全部req3/rc1且unfinished mail未投递，/tmp/csih-watch-mail-parent-review.json，源码/compiler前后不变。agent_watch_note文字API现无副作用，exec_run从真实原始shell_result正常成功退出、保守canonical单条调用与exact peer、完整可信成功回执标记；不接受不确定复杂调用。未实际发信/未证明收件人已读，旧窗未升级。

下一交付接用户可用owned运行/导出/恢复：新增显式agent-owned DIR SESSION HASH及resume-agent DIR STATE SESSION HANDOFF HASH，DIR为每会话现有私有0700目录，锁DIR/owner.lock、日志DIR/journal.jsonl，避免共享默认日志。旧agent/run不默默改行为，也不宣称旧窗已保护。所有权在任何TTY读取/写journal前取得；resume先prepare校验身份+日志与目录绑定再apply，显式恢复cwd/role/peer并令agent实际路径走ownedcontext，提交恢复后才consume，清理失败说明已恢复不能重复提交。/export-state HANDOFF保存空闲真实状态，控制命令本身不作为待发送输入，不改变原pending/history/预算；失败保输入与原state，成功显式路径。HASH本阶段为显式启动绑定值，候选真实源码身份仍由后续冻结launcher校验，不能冒充已证明运行产物hash。专职代理实现和私有PTY实跑，不动原窗。

owned入口父读真实agent运行容量：AT.transcript[512]、AT.cwd/run_cwd[1024]且agent_turn_begin at_copy/snprint静默截断；agent_peer_name_ok上限48。已派要求owned/resume激活前拒journal>=512、cwd>=1024、peer超48/非法，而非把schema4095存储上限冒充实际可运行；本件不扩agent数组。

owned恢复入口父审查补强已落盘：tui_reload_prepare_bound先load/decode，再对expected_journal固定路径比较，之后才checkpoint；旧prepare wrapper保原语义。此防拿本会话owner锁去fsync另一会话journal。专职PTY端到端尚待交付，不能仅源码存在就报运行恢复已绿。

owned入口父独立实跑永久PTY probe通过：tests/bound.py40 owned_session.py rc0，私有unisacc真实候选+完整selftest、第二owner拒/日志不变、中文草稿goal/history export退出resume重导出与consume、坏identity/offset/binding保state，hash不变。此未覆盖自动pending实际执行。父读tui_dequeue现直接把pending[0]写input，恢复同时有pending与未发input会被覆盖；下一内聚交付本地HTTPstub实际自动队列执行并保用户草稿，验证真实cwd/role/journal+loop true left0不重置/不多派，先复现再修，不改旧窗/compiler。

resume-pending实际HTTPstub验收发现上下文真实执行未过：专用prompt修后草稿及运行中编辑保留，但system仍旧看客/wrong-peer，tui TU setenv不足以令agent TU getter读新角色（真实请求证据，不推断编译器根因）。决定新增生产int agent_role_configure(role,peer)原子校验/设置既有g_role_force/g_peer_force，0/-1；不是调用test setter冒充生产接口。owned_start成功白名单环境设置后显式调用，拒role超真实g_role_force容量，peer保持48，getter优先force；未改compiler，不把该bug归因定论。probe actualpwd断言须检查tool body本身而不是cwd头。失败 /tmp/csih-resume-pending-before.json及after保留。

resume-pending新实际收据after-3父已读passed true/3 localhost请求/输入草稿与运行中编辑保留/实际角色write+restoredpeer/严格pwd正文，before==after冻结hash；失败before草稿覆盖、after旧角色、after-2 /var别名全部保留。现生产agent_role_configure校验后同步getter override（role真实16容量拒超15，emptypeer显式配置不fallback旧环境），owned界面title读ownedrole。仍待专职最终冻结回执与父独立复跑，不以收据观察冒充父已运行。

自动pending及真实上下文恢复父独立复跑最终永久probe rc0：/tmp/csih-resume-pending-parent.json，40秒外层/15秒candidate与PTY，exactly3本地stub请求、严格pwd正文、write/restoredpeer与本DIRjournal、草稿+EDIT保留、left0自然disarm不多派。未实际API/邮件/原窗升级；多pending与setter全部拒绝边界未测，候选身份/真实热换版仍待。

下一候选构建单件：stdlib Python driver（仅宿主构建/验证辅助，不增agent工具类别），CLI build SOURCE_APP COMPILER PRIVATE_ROOT / verify CANDIDATE_DIR。冻结真实.c/.h/.inc编译输入与compiler到0700候选目录，身份取排序路径+文件SHA256清单摘要；同一冻结树构建旧15源、真实候选完整selftest，并绑定确切argv/compiler/artifact SHA256和各rc收据，前后及启动前重新验证；不信手填hash、不用系统cc、失败不进入handoff、旧进程不动。verify只证明冻结候选与门禁收据一致，不证明运行已接管。构建/每门禁≤15秒，外层≤60；私有HOME防读取密钥，禁止真实邮件/API。坏候选、修改源/编译器/产物后verify拒、两不同源身份候选验收。

环境跨TU最小独立证据：私有main.c setenv(CSIH_PROBE_ROLE,new,1)后printf自身getenv和peer.c中peer_get(getenv)，unisacc.com两源实际rc0/stdout rc=0 main=new peer=old（初始env old），/tmp/csih-env-behavior-review.json。因此实际观测跨译元环境更新不一致，未定位内部根因；生产role_configure跨函数参数同步确有必要。未改compiler/仓库C、未读真实keys/环境全量输出。

真实控制TTY生命周期私有实测：pty.fork创建session leader，fork候选同前台组READY，leader退出后候选实际收到SIGHUP，证据 /tmp/csih-tty-leader-review.json（第一读事件文件有竞态先无证据，随后读回确认HUP）。不动原csih窗。由此后续热换版不能旧TUI作为session leader直接fork候选再exit；需稳定监督launcher保会话leader、管理各native候选子进程，或证明其它TTY策略。现owned_session openpty+Popen setsid未建立真实CTTY，不可当本项证据；handoff验收须真实pty.fork前台终端。

候选构建父独立永久矩阵实跑rc0：坏源码无receipt，正常freeze/build两gate/verify，source/compiler/binary内容与binary权限篡改各精确拒，两注释变更源hash efd12fd51eed0421763aed63b5dca4c21a3971c7ec1ec0d5c2b3a26cbf553b7a→bbd2f8f6fd2958ca8f2fb5b48ebcd64a9f0339a886e73eba969ad7923f95c74e，两个真实gate绿。源码/compiler/driver/probe未变。门禁只本机candidate TUI/agent单元；不代替全仓库/跨平台、不认证来源、不接管。
父同时读出net.c NET_TOTAL_SEC仍120，违背所有看门狗≤60的仓库规则；须降60并用私有短阈值网络超时分支验收，不能把旧窗16h挂起归因此宏（旧运行身份不同且根因未定位）。

NET超时收口补充真实关联：agent.c超时原因硬编码model call timed out (120s)，同交付允许仅该文字120s→60s以匹配生产阈值；私有阈值1秒探针只验branch/err=-6，须注明文案60为生产默认，未完整等60秒。

NET60收口父独立实跑probes/net_timeout.py通过：/tmp/csih-net-timeout-parent-review.json，生产60秒完整TUI selftest，私有1秒同branch单localhost请求失败err=-6；未等完整60/未归因旧窗。
下一热换版实现决定：稳定宿主launcher保CTTY session leader及初始termios，所有native actor留同前台进程组；先实现native控制面再接实际launcher。agent-managed经继承私有socket FD由监督者激活；standby-managed启动只报READY，不读TTY/写journal/取owner。旧actor收到freeze在idle暂停输入/派队列/journal，用新candidate hash+handoff导出最终快照；release成功后监督者COMMIT新actor，新actor取owner→固定日志prepare→apply/raw/consume→ACK但仍待ACTIVATE。监督者收到有效ACK才ACTIVATE新、RETIRE旧；旧成功退休绝不raw_leave影响新actor。ACK未知超时须终止并wait确认candidate退出后，才RESUME旧重新取得owner/raw；关闭所有权失败保持暂停不假称恢复。消息全匹配session/handoff/hash，拒迟到ACK；FD需CLOEXEC防tool继承。/reload-code请求只managed idle，不入history；真实source/artifact身份由后续launcher verify绑定。本native件由私有真实pty.fork broker测试，不把它当完整发布launcher。

managed native控制面父独立实跑通过：tests/bound.py45 probes/managed_handoff.py rc0，输出 /tmp/csih-managed-parent-review.json 为PASS managed CTTY commit and rollback（纯文本非JSON，扩展名不代表格式），stderr空。真实CTTY commit/rollback/错sid拒；代理详细冻结收据 /tmp/csih-managed-after-review.json。下一专职单件稳定reload launcher：复用candidate build/verify与native phase协议，初始原TTY前台组存活监督者、基线termios最终恢复；构建期间旧继续服务，READY后idle冻结最终draft，release/commit/ACK/activate/retire，失败先候选terminate+wait后resume；每消息三身份绑定、看门狗<=60。必须实际两次换版/坏候选/ACK未知私有CTTY验收，不把控制面当完整热更新。原窗不动、不发布。

父验收收据更正：managed_handoff.py调用未传收据参数，默认覆盖 /tmp/csih-managed-review.json，导致首轮role fixture失败原JSON丢失（先前文字事实仍保留，不能称原文件仍保存失败）。本次父详细成功另存 /tmp/csih-managed-parent-detail.json，stdout /tmp/csih-managed-parent-review.json为纯文本PASS；代理最终after-review收据未动。下一永久probe调用显式传不同路径。

现场只读巡检（时间更正：2026-10-08 14:03 SGT，原误写10-09）：原PID38793/48491仍活，csih1继续NFS系统文字覆盖；csih2旧轮次110仍把envelope目标不存在exit1宣称judge ok，未重启未升级。当前agent.c ACT_ERR连续三次分支仍res.ok=1/reason too many unparseable steps，是决策诚实性待验证缺口。父仅冻结私有agent源与compiler复现，不与launcher改树测试重叠，不改主树C。

三次不可解析父独立私有复现 /tmp/csih-unparseable-parent-review.json：真实unisacc/本地HTTP stub requests3、rc0、no answer text、actions0/MAX_ROUNDS，无真实API/邮件，无主树C修改。这证明失败被成功退出编码，不能以有限终止等同工作通过；launcher单件冻结后派agent诚实失败小修与永久负例。

launcher第一真实success场景实际两次换版/坏build编辑均走到末尾，但正常exit control EOF先于waitpid可见退出造成错误失败，原 /tmp/csih-launcher-success-review.json 已保存；代理有限wait3s取真实退出码修后再跑，尚未父验。rollback首实跑收到RESUMED且child rc0，却baseline_restored=False，/tmp/csih-launcher-rollback-review.json，明确未绿，不删断言；须查termios前后差异，并证明恢复后新输入响应/owner而非旧frame。另父审查现launcher未加独立supervisor会话锁，两launcher可能共享事件和TTYcleanup，nativeowner锁不等于host独占；排必要下一小件。

rollback终端差异专职实证：仅macOS PENDIN(0x20000000)，其它flags/cc相同，完整数组证据 /tmp/csih-launcher-rollback-termios-review.json；恢复后suffix响应和owner检查过。私有openpty最小实验raw→TCSANOW baseline自动生PENDIN且重复不清，TCSAFLUSH精确相等。决定最终launcher退出恢复采用TCSAFLUSH（会清终端尚未读取输入，须doc明示），不放松测试；finally即cleanup异常也尝试baseline。此是最终退出清理，不在成功热换版中flush用户输入。

旧窗bootstrap前只读现场保存：/tmp/csih-legacy-runtime-snapshot-20261008/receipt.json 及每窗300行screen，0700目录/0600文件；PID38793/48491均活，运行unisacc-ce4abda旧driver，cwd apps/csih。未取密钥/全量env、未终止/重启/改旧窗口。此仅可见屏幕，不证明完整内存目标/草稿已导出，不能当无损迁移收据。

launcher父独立并行两selector实跑均rc0/PASS：/tmp/csih-launcher-success-parent.json 与 /tmp/csih-launcher-rollback-parent.json，二者passedtrue/before==after/childrc0/baseline exacttrue。真实两个不同hash接管/构建期中文draft、坏build保旧编辑、goal/history/cwd/role/peer/journal结构保留；ACK忽略后candidate-exited确认rc先于resumed且恢复后新编辑/旧owner有效。未验证真实API/tools在换版后执行；原窗仍未部署。下一专职单件host监督锁：同session DIR独立supervisor.lock，取得锁后才能写事件/捕获或恢复TTY/构建或spawn，锁本人普通0600nofollow+CLOEXEC且持至所有actor清理/termios恢复完成；第二host拒且零事件/TTY/产物副作用，死进程释放但不删除锁。本件父不改源码、不提交。

父另一终止诚实性实证：冻结前述私有agent树，localhost stub连续3次go:stop，/tmp/csih-stop-no-answer-parent-review.txt 显示requests3/rc0/actions0/no answer text/stoppedyes。预期失败退出1负例FAIL（probe脚本本身0不能当行为通过）；证明三次判定上限也把零交付编码成功。与ACT_ERR三次问题同归终止结果诚实性，待host锁冻结后专职单件修、正常exec→answer→stop正例不退化。不先以提示词变更掩盖代码判据。

host锁父独立focused永久probe rc0/PASS，/tmp/csih-launcher-lock-parent-review.json passedtrue/frozen；真实第二host EAGAIN rc1且events/termios/artifacts不变、owner held、原actor继续中文编辑，正常exit baselineequal lockclosed。代理含两个换版回归green收据已读，父不无故重复。
终止诚实性完整父冻结补证 /tmp/csih-budget-no-answer-parent-review.txt：8continue无答案rc0、16exec true耗动作无答案rc0、answer后无法解析收尾字符串仍rc0且日志合成为go:stop。全部期望failure负例FAIL，是行为bug不是探针green。下一专职单件修agent.c所有这些终止误报：三parse失败/第三stop无answer/轮动作budget未完成均ok0+unfinished原因；HTTP_END只明确GO_STOP/CONTINUE可被识别为判定，坏内容有界重问并日志保事实，不默认合成为stop通过。保原有效answer→stop/no-tools/红旗/watch规则及限额，当前round末合法answer atMAX_ROUNDS仍保既有接受语义，不重置budget。提示词与docs同步真实实现，不把修prompt代替结果门禁。永久负例和正常/坏判定后恢复stop正例真实localhost stub，无真实API/邮件。

终止诚实性验收入口父审查发现跨TU真实类型不兼容：agent.c agent_result.reason[320]、tui.c相同，agent_cli.c.reason[128]却按值声明返回agent_run/agent_run_cb。此是C接口不兼容事实，不臆断已产生某个栈损坏或编译器根因；要求专职当前suite结束后精确CLI128→320，与生产完整结构一致，打印stopped yes/no+实际reason代替默认MAX_ROUNDS标签，再冻结全组验收。必要验收入口修正，不重构其它头/编译器，未由父改源码。

终止诚实性父独立最终9例rc0/TOTALPASS9，/tmp/csih-agent-terminal-parent-review.json passedtrue/frozen：parse3失败、stopHTTP_END3失败、直接ACT第三stop3失败、actions16失败、continue8失败、prioranswer继续9失败、badjudge4失败且原文/error记录无合成stop、badjudge恢复3成功、第8round合法answer8成功。首父命令误用stop_no_answer_act/final_round_answer不存在被selector启动前拒（不是行为失败），改源码真实direct_stop_no_answer/last_round_answer后通过。CLI reason320声明一致验收版本，源码未在suite中修改。
下一专职单件真实运行端到端覆盖：稳定launcher热换版后localhost stub实际file/exec，tool输出body和system role/peer/cwd+实际journal证明上下文不是只结构/UI；非空journal再跨换版检查offset。另原TUI自动goal失败parse3结束后观察无额外自动请求，证明新ok0不再宏观重派。仅永久probe/docs，必要源码bug先报，不扩compiler/真实API/发信/旧窗。

工作流借鉴收口（管理决策，非代码交付）：1角色分工继续父只读定位/派单/独立验收，专职写源码；外窗挂起不能冒充它们已改。2巡检比对源码/候选receipt/真实运行身份及具体live句柄，不因观察超时重启、不把静态画面当进程退出。3轻推只针对真实空等/小阻塞，当前csih1活但不响应不能连续塞任务；等待裁定不算停摆。4诚实台账：落盘/编译/私测/父验/部署分开，失败原文保留，父误路径/误日期/覆盖失败回执如实记更正。5决策step以明确合法JSON类型与事实门禁为准，stop仅终止、失败预算不能成功；代码判据先于prompt。6发布与原窗迁移最后做，先提供可复验候选和运行结果，不能把原窗可见screen备份说成完整内存保存。7报告仅真进展/真阻塞/需主人裁定，先结论SGT时间。每15分钟长期巡检目前未新增daemon/自动任务，不能宣称已实现。

主人要求立即恢复满屏错误csih1，父管理优先级纠正：此前过度停留源码验收未恢复现场。保留旧PID38793/%388和内存，保存2000行scene到 /Users/wjc/.csih/managed-csih1-gybilyl7/legacy-screen.txt，旧窗改留档名，新csih1启动稳定managed launcher真实候选。新会话非完整内存迁移，不杀旧进程、不宣称旧任务状态原样恢复。恢复元数据 /Users/wjc/.csih/managed-csih1-gybilyl7/recovery.json。

csih1现场恢复实际完成启动/输入验收：旧%%388 PID38793保留csih1-legacy，新csih1 %%400稳定host PID40541/native41689，candidate hash5b05e29d9089a699f21b1e6e327fdf12877f72bdf1c57a1678e21eb673e24202真实build/gates并ACK+ACTIVATE。新窗口干净正常，/export-state recovery-check真响应，state-v2.json sessioncsih1/rolewrite/peer0:csih-cdx/cwdAPP。候选与会话私有目录 /Users/wjc/.csih/managed-csih1-gybilyl7；新会话接班不是旧内存无损迁移。未杀旧进程/未重启csih2/未触发真实模型请求。

csih1新运行体真实接班已证：父envelope回执rc0投0:21并抓%%400确认file读取handover、exec pwd、exec envelope；主人随后收到csih1接班回执，rolewrite/peer0:csih-cdx/cwdAPP一致，只读未改源码未动别窗未重做旧单。新会话已真实执行工具并向父发信，非仅亮屏/私有stub。父先令本轮answer+stop后值守，不派旧任务。

恢复后调度回csih1：唯一交付tui.c selftest高度24/40/60用例，父已读当前height24块真实r_frame/tui_state/tui_log_line两参；仅测试不改生产布局，room17/33/40、frame不超终端/63/宽80、草稿marker可见。先读真实API再精确改，实际unisacc原15源与完整selftest，每步骤<=15/外层<=60，保失败回执；不重做历史块/不改compiler/不部署。csih2原窗仍旧110误验收与失效peer0:csih-tui，下一需要新监督会话修角色/peer后只读巡检，保旧而非遗忘现场。

csih1布局首派真实耗16actions全查资料，MAX_ACTIONS未完成ok0，未改源码，未自动重派；父派单太宽缺完整compile命令，下一给精确范围+确切15argv，继续同一交付不重置规则。csih2恢复同样保旧PID48491/%%390内存和屏幕，新watch session peer改为真实0:csih1（旧失效0:csih-tui），用csih1已验证冻结候选不并发build变化主树；只读观察/送一件，无代码权限。目录 /Users/wjc/.csih/managed-csih2-ayny_t1x。

实窗发现观察回执反馈环：csih2观察→csih1确认→csih2将确认再当新观察任务。父先发单向纪律，csih1不再给csih2任何回执，csih2停止本轮工具观察；后续消息类型/任务触发需改进，不能只靠每回合模型stop。另layout selftest唯一失败真实条件不仅goal_at2，还有header csih·agent；新实际角色write被selftest继承，之前候选gate隔离ROLE/PEER故绿，需env -u CSIH_ROLE -u CSIH_PEER复跑，不先改已有断言/认定既有代码bug。

csih1布局冻结hashfe9c5a9a601697950ff4bcc4842456f1ef6f5b7afa96cc55506b8db98bb09776；csih1 env -u CSIH_ROLE -u CSIH_PEER selftest实跑rc0/selftestok，原失败selftest.log保留，新selftest-env.log。父读wide断言goalat已2，真正失败来自header角色write，排除原断言回退错误。下一父独立重新编译/完整selftest并现窗/reload-code候选gates，保存真实handoff身份。观察双方停止互发已处理root停止指令日志，尚无transport类型机制，仅当前纪律止环。

三例布局父独立重新编译+完整selftest rc0，/tmp/csih-height-parent-review.json frozen true/selftestok，生产未改。实际csih2最后user为父停止观察，随后answer收到+stop，互答停止；csih1本轮env隔离真green回执后仍旧agent_red在进程内红（可见末tool旧sliceFAIL），需要自动slice子进程隔离ROLE/PEER后防复发。先现窗/reload-code走已验监督协议及候选完整门禁，新版本实际receipt可替旧陈旧红状态，不能称红无故忽略。

现窗/reload-code真实接管成功：稳定host40541未退出，旧managed actor41689退休，新actor36040；handoff e2f4ccf1dc244d56a7f6c3f1bdba940f/hash efc2432f26283cba7d890fba9fe5caacb05a6f8720189d5049b9b979bc4b1a21，事件READY/FROZEN/RELEASED/ACK/activated/committed已读，显示新hash且空闲可输入。原最初legacy38793仍留档。下一父post-live-reload导出核状态，自动slice隔离问题由csih1单件两命令环境修，不由父下场。

现窗换版后/export-state post-live-reload真响应，hash efc243.../cwdAPP/rolewrite/peer0:csih-cdx/pending[]，host40541仍同一进程；新actor36040，旧managed41689已退出。下一csih1单件只agent.c agent_slice两处shell命令清CSIH_ROLE/CSIH_PEER供gate子进程（POSIX空环境变量prefix），不改真实agent角色配置、失败门禁或标题断言。验收用rolewrite环境编译新native/真实自修改触发tui slice，确保子gate默认角色且green，保原污染失败证据。

gate隔离两处已读回agent.c659/690为CSIH_ROLE= CSIH_PEER= exec；csih1复用csih.sh build/once探路无必要，父及时收窄并要求冻结，不让编译入口调查再次耗动作。下一专职probe仅私有复制CLI新增测试入口，直接agent_parse/agent_exec真实file edit触发agent_slice，无模型/API；修前控制rolewrite导致sliceFAIL，修后green且父role仍write，源树/原文件不变。

现窗输入历史父实测计划：仅空闲csih1，输入中文草稿→Up取旧消息→追加-EDIT→Down返回草稿，不Enter、不触发模型或重跑任务；最后清本次合成草稿，留frame证据。不能覆盖真实未发输入，若检测非空先跳过。

父现窗Up/Down/可编辑草稿实测通过 /tmp/csih-history-live-parent-review.json：Up画面input改变、-EDIT实际可见、Down中文草稿恢复、最终清自造草稿回空输入；无Enter/模型派发/重做任务。首tmux -l -EDIT被当option拒，Down+清草稿后加--重跑成功，保错误说明。

2026-10-08 csih父验收更正：/tmp/csih-history-live-parent-review.json passed=false，Up历史/Down中文草稿恢复/清空已证，编辑后缀未在帧中显示；此前“编辑实测通过”措辞撤回，原记录保留。csih1直接write环境selftest的标题断言失败不等于agent_slice环境隔离失败，需真实slice新旧对照；源码agent.c e6e2372261ef1b7d2ea8263eba8c1580598bf23d140c231bdd929adb74222299冻结。

2026-10-08 父独立真实slice复验：new与old两个selector各rc0（old为预期红对照）；收据 /tmp/csih-slice-env-new-parent.json 与 /tmp/csih-slice-env-old-parent.json。两次源树before==after。下一步在csih1空闲输入/reload-code由已存在的托管host构建门禁并换版，失败保旧，不修改生产C。

2026-10-08 csih1现窗/reload-code成功：READY/FROZEN/RELEASED/ACK/activated/committed齐全，新身份19d4df856935984172d2d070dd08f4bb98e3883f0ed74c908f591c7a79597193；host40541不变，新native3217，旧36040已退出。导出确认cwd/role=write/peer=0:csih-cdx保留。收据/tmp/csih-slice-env-live-parent.json。源码修复由csih1完成，父只验收换版。

2026-10-08 下一工作流件：派csih1只读定位通知自动激活入口并交docs/notification-task.md设计，显式task/notice而非模型猜测；父继续管理现窗，不改C，csih2保持空闲。

2026-10-08 历史现窗补验通过（csih2空闲）：发送无副作用/goal短命令，中文草稿Up取回/goal，Backspace可见/goa，补l可见/goal，Down恢复中文草稿，再Up Enter重新发送并清空。journal逐字节不变证明未发模型任务。导出历史末项/goal、input空；收据/tmp/csih-history-live-short-parent.json。之前长行后缀截图失败仍保留，不能据短命令验收声称长行末尾显示已解决。

2026-10-08 通知设计审查未通过：全局env不能逐封分类，agent_cli非TUI投递入口，类型标志不提供身份认证；真实投递是bracket paste+Enter，现PASTE拼input会污染草稿。派csih1只修同文档，明确独立paste缓冲及task/notice分派、容量拒绝与兼容边界后再实施。

2026-10-08 通知设计裁定：普通用户paste保持追加+自动提交，仅完整真实envelope格式隔离草稿；标题精确[notice]/[task]分派无身份认证；需解决paste后外部Enter误提交草稿，禁止简单无限skip_next_enter。交csih1收口设计，未准生产实现，先通过输入协议门禁。

2026-10-08 父独立reload_runtime两项通过：tools与goal-failure外层各≤60s、rc0，收据/tmp/csih-reload-runtime-tools-parent.json及/tmp/csih-reload-runtime-goal-parent.json，源before==after。tools四请求真实pwd/file工具、两次换版/固定journal/草稿历史保留；goal-failure三次解析失败后观察期无再派，非宣称永不再发。

2026-10-08 notification-task v3已收口但仍只是设计：同一TTY字节流无法证明识别用户Enter与外部传输Enter，开工门禁未过。已派专职code_review_dispatch只交/tmp/csih-notification-protocol-review.md，审查最小file+exec独立消息入口并复用现owned会话/控制通道，外部envelope不改；旧通路不能声称已修。csih1未获C实施单，保持空闲。

2026-10-08 e6e环境隔离改动后父agent_loop_bound回归21/21通过（三批9+6+6各≤60s）；源before==after。收据/tmp/csih-envpatch-{terminal,base,mail}-parent.json。现场两托管host仍活、输入空；csih2帧内“请求中”来自旧日志不能据substring宣称正在等待，以journal尾判定轮次。

2026-10-08 通知入口父裁定：采用逐封严格JSON文件(type task/notice)绕开不可证明的TTY Enter区分，沿用file+exec与owned会话目录；旧envelope原路兼容且未修缺口。第一片派csih1唯一csih_message.c纯验证库，schema version/id/session/kind/body，尚不接TUI/消费/host。crash-started不自动重放，不承诺exactly-once；详细协议审查待/tmp报告。

2026-10-08 csih1验证库整件失败：journal3个parse错误后停止，csih_message.c未存在。未记成功。父拆为≤1400字节声明/helper首片，直接file write，禁止再读查循环；后续逐片验盘。解析失败原始模型内容未入journal，诊断证据缺失列为工作流后续问题。

2026-10-08 通知协议审查落盘/tmp/csih-notification-protocol-review.md（只设计未实跑），父已读全文。采用fixedDIR/inbox ready/started/done、ACTIVE owner消费、短时mailbox记录锁；notice UI-only零模型零ACK；task独立prompt不占input/pending，不扣用户goal预算；started残留报告不确定不自动重放。容量上限32未完成/1024保留id需真实探针。v2快照不塞正文、四字段控制握手不扩正文。旧TTY信封问题明确未修；实现从纯validator分片开始。

2026-10-08 首片仍未落盘且csih1误报已写：实际file动作text内真实换行导致严格JSON解析失败，父json.loads定位column142；后续answer无工具成功证据。已明确更正并派重发JSON转义同首片，不扩任务。台账保留错误原话并说明，不以模型自述代替盘面。

2026-10-08 csih_message.c首片父验盘894字节，工具回执wrote894与盘一致，首个JSON转义重发成功。声明/helper与唯一MESSAGE_NEXT在，公共validator未实现。按声明/helper→NUL/schema公函数分片继续。

2026-10-08 分工实施：csih1仅csih_message.c的NUL/session helper下一片；code_review_dispatch唯一message_send.py发布器，复用真实trusted/unique_object，file+exec逐封schema、短mailbox记录锁、ready/started/done容量32/1024、原子不覆盖发布/重复冲突/可见后不确定码。两路径独立，尚不接host/native消费者，文件发布不等于执行ACK。父只写台账/临时探针并验收。

2026-10-08 csih_message.c第二片2147字节父读盘：NUL扫描与safe_session真实在盘、唯一锚点保持；工具edited回执相符。下一片公共validator复用局部errbuf避免NULL why向json_parse传入，schema五键/类型/id/会话匹配/解树统一cleanup。

2026-10-08 csih_message.c公共validator盘5735字节、五键校验及统一jfree在；当前未查UTF8且重复键仅被总数/缺字段拒。派第四片复用真实sb_utf8_one补原文+body严格UTF8，逐键count拒重复并写字段原因；后续冻结编译探针，禁止边跑边改。

2026-10-08 csih_message.c冻结父验收通过：系统cc与出货unisacc各构建成功，32schema/Unicode例+3坏UTF8原始字节+NULL expected_session各跑，共72/72通过；源码before==after。收据/tmp/csih-message-parent-review.json，临时驱动目录/var/folders/kv/yf8l6q994rl27550kd5985y40000gn/T/csih-message-parent-3okeslol。这是纯validator验收，不等于消息发布/消费/通知零模型已完成。

2026-10-08 csih_message.h458字节父验盘，签名与库一致。consumer需取出id/body才可独立处理，派同头新增有界decoded type和decode声明，下一片c实现复用validator/json_parse不另解析器。publisher父审查发现EXCL失败误清他人临时风险，已送唯一实现代理修且要求碰撞注入证据，未验收/未自行改代码。

2026-10-08 csih_message.h解码类型与真实声明父读回在盘（id33/body4096/kind），next decoder实现明确失败清输出。派csih1只补csih_message.c与同头历史未实现注释，无I/O/排队/TUI。待冻结后父双编译器比对body及失败清零，再推进consumer。

2026-10-08 父独立decode验收72/72通过（系统cc/真实unisacc各32数据例+NULL text/out/why及cap0），正文与id逐字节一致，失败输出全零；源码before==after。收据/tmp/csih-message-decode-parent-review.json。publisher父独立CLI/schema同32例/实际记录锁busy/EXCL碰撞保他文件/可见后fsync rc3保目标也通过，收据/tmp/csih-message-send-parent-review.json。下一消费库契约take/finish，接线前独立实跑。

2026-10-08 csih_message_io.h1374字节父读盘与真实签名一致，take/finish跨提交返回码/out清零边界明确。消费实现唯一csih_message_io.c已交code_review_dispatch，csih1当前停在header交付不改生产TUI；等消费库冻结/实跑后再接线。

2026-10-08 H1-1 开工：文件域对象的联结历史独立于存储身份 GUNIT，另设每标识符有效联结与单元身份；extern 继承已有 internal，普通对象声明转 external 与已有 internal 冲突时拒绝。检查先于 GV.emit 的存储分流，先覆盖对象，函数入口/位置诊断留 H1-2/H1-3。此前撤出的 lex/pipeline WIP 不并入此片。

2026-10-08 父独立消费库新私有快照双编译器正常矩阵34调用通过，before==after，收据/tmp/csih-message-io-parent-review.json（实际父cwd与argv修正为新目录）。父测试驱动首准备断言count和漏python启动两次失败均未作为产品失败/绿；修正私有驱动后实跑通过。原native动态故障注入超时不抹除，已派仅私有固定fail helper变体诊断、不改compiler。TUI接线预查：reload_support.inc聚合与csih_message.c未guard的jkind/jvalue会冲突，需沿既有CSIH_RELOAD_JSON_TYPES guard；消息结果journal不能带role/text，否则agent_chat_role会包装给模型，采用只有mail_id/result/reason的元记录；通知不得入模型journal。

2026-10-08 native固定故障注入父收据复核：/tmp/csih-message-io-native-fault-review.json passed=true/frozen=true，默认unisacc两variant四例实际返回rename -1/fsync -2，take完整struct清零与状态字节保留。旧global包装故障3秒超时仍保留且未定位原因，不用新绿改写旧事实。消费库正常/固定故障关键路径已可推进聚合；接线之前仍需完整TUI门禁。

2026-10-08 主人要求检查并清理csih1-legacy：父确认唯一pane%388/旧PID38793，仍旧ARM64失败/循环答复与大量localhost NFS告警，新csih1%400/host40541独立正常。清理前全部可取screen+进程树现场私有备份/Users/wjc/.csih/legacy-csih1-20261008-154943（目录700文件600）；只终止已被替代旧窗口，不删源码/记录、不改NFS系统设置、不动其他窗。

2026-10-08 csih1-legacy清理完成：SIGTERM后旧PID与其已记录子树均退出，tmux自动移除@388窗口；后续kill-window报无窗口，父独立核窗口确实不存在、新csih1仍%400/host40541原身份。收据/tmp/csih1-legacy-cleanup-parent.json，备份/Users/wjc/.csih/legacy-csih1-20261008-154943。没有把NFS告警归因为csih崩溃；未修改NFS/文件系统配置。

2026-10-08 消息库聚合完整候选门禁父核通过：默认15源TUI编译+selftest及agent gate均绿，输入before==after，候选hash6e7579ae47db9641f67d4051df37755a6e6df96071217135a4ecdd01eec2452f；仅库聚合尚无TUI消费，不部署假称通知能力。下一TUI接线按state/prototypes→metadata/complete→poll→loop/callback分片；notice不入模型journal，task审计仅mail_id/result/reason无role/text，外部task不改goal预算/用户草稿。

2026-10-08 TUI接线首片父核字段/真实prototype在盘，csih1工具自检tui rc0，未接循环。第二片metadata/complete：mail元记录无role/text，不进入agent上下文；终态先记真实结果再finish，失败保started并mail_blocked。started残留未知UI报告仍需后续接线验证，不可因库“不重放”声称已完成可观测性。

H1-2（0.0.35）：对象和文件域函数共用独立的有效联结历史表；函数在 FN.fn 检查，无存储类函数继承已有联结，块内原型不写该历史。内嵌头原型与定义须联结一致，参考删除头内冲突豁免；纯 token 与 located 模式采用同一 C99 判据，不新增位置格式。

2026-10-08 消息收尾第二片父读盘：metadata真实三键已在；发现meta失败早return未清mail_active/id，与收尾契约不符，先交csih1精确修正再接poll。不拿模型回执当验收。下一poll仅ACTIVE owner、单次节流、notice不写模型journal，task启动审计失败保started不执行。

2026-10-08 父读盘核meta失败已清mail_active/id且blocked保留。下一poll单件已实际envelope投递csih1：ACTIVE+owner检查、250ms节流、notice只UI与done不写journal，task执行前started审计、run_prompt(flag0)保草稿，审计失败保started不执行。真实notice/task永久probe已准备py_compile绿但未跑，已派metadata契约校准started→ok，不以未接线源码运行冒充验收。

2026-10-08 poll父读盘发现实际条件!tasks_allowed&&busy无法阻止tasks_allowed=1忙时启动，且tui_run_prompt返回未检查；尚未接调用/部署。先交csih1修两处，再接主循环，不能以字段helper落盘宣称收信能力完成。

2026-10-08 父读盘确认poll许可条件与启动返回修正在盘。下一hooks单件：turn_done真实终态收尾，mail来源抑制自动goal排队；main与net回调分开task许可，保普通用户行为。接完冻结才完整候选与真实probe。

2026-10-08 接线实况：csih1纠结3次编辑限制后同一输出两个JSON动作，被工具明确拒绝，尚无hook落盘。父纠正调度：次数只是建议，允许5次小edit；严格每条单一JSON，完成前以工具回执/盘面为准。协议失败的自动约束仍需后续处理，不计实现完成。

2026-10-08 接线父读盘核四点齐且csih1答冻结，tui.c158496字节。候选完整门禁与真实notice/task探针开始，独立私有快照，不部署。先前多动作有的拒绝、有的工具仅执行首条，均按真实工具/盘面计数，不按模型全文完成。

2026-10-08 父完整候选门禁通过rc0/frozen=true，hash ecd4a8598b7155da2df51b3dee948d6d03868e8f845bd04c64351c3acad20d48，收据/tmp/csih-inbox-hooks-build-parent.json；真实notice/task已交验收代理运行，当前窗口尚未部署新消息消费。

2026-10-08 真实message_inbox首轮：notice passed=true/source frozen、0模型POST、journal字节一致及handoff真实快照；task failed AssertionError/source frozen、4POST停在外部任务等待期，通知原五键已done，任务仍started。旧失败收据保/tmp/csih-message-inbox-task-agent-review.json，已交验收代理定位失败行，同步/UI还是行为未定，不部署。

2026-10-08 真实message_inbox复跑notice/task均passed=true且before==after；task忙时notice可见/4POST未增/journal字节未变，最终7POST、实际cwd/file工具正文、草稿/预算/handoff断言通过。原首次失败保留，修改仅probe有界PTY等待及正确区分3parse-error与2task工具。收据/tmp/csih-message-inbox-{notice,task}-agent-after-review.json；拟空闲边界/reload-code现场部署。

2026-10-08 csih1现场/reload-code接管成功hash ecd4a8598b7155da2df51b3dee948d6d03868e8f845bd04c64351c3acad20d48，handoff63f3a409d7af4377bedaad337f9a440d，新native95834/host40541稳定、旧3217退出。实际message_send notice发布rc0/done原文、UI正文可见、LIVE_DRAFT_KEPT可见、journal字节不变，无TTY注入通知。收据/tmp/csih-inbox-live-parent.json passed=true，现场测试草稿用退格清空未发送。csih2仍旧版本，watch publisher权限/残留started观测/工具动作诚实门禁尚待完成。

2026-10-08 csih2现场journal最终decision stop且空输入，安排同验收源码/reload-code；保持role watch/peer0:csih1。动作诚实问题定位：agent对多对象仅执行首条并附提示，parse失败后仍可answer；下一需针对真实工具失败/协议违规后无成功证据的成功宣称设计与回归，不让提示词替代验收。

2026-10-08 csih2同候选热换版完成，hash ecd4a8598b7155da2df51b3dee948d6d03868e8f845bd04c64351c3acad20d48、handoffd9a765cd70e246a6afe1dc72d921189d/newnative5970；export实核role watch/peer0:csih1，现场notice发布rc0/done原文/UI可见/journal不变，无自动模型ACK。收据/tmp/csih2-inbox-live-parent.json。csih1已接docs/action-result-proof.md设计单件，不动C；工具失败诚实验收与watch文件投递权限仍未完成。

2026-10-08 动作证据设计父审查未通过：任意success清pending与语义覆盖冲突；agent_exec/plugin返回处理与工具成功非同义，agent_file read失败仍return1，空文件写入0字节也可合法。需先建立真实结构化status，不从文案/handled猜成功，修复关联须明确键或承认未知，不靠模型说覆盖放行。派csih1唯一文档修订。

2026-10-08 下一watch文件publisher规则交只读审查单件/tmp/csih-watch-publisher-review.md：固定目标session/peer绑定、真实receipt而非echo、notice/task区别、无托管映射诚实拒绝。尚未扩角色权限或实现，不用消息类型标志当身份认证。

2026-10-08 动作证据先落确定性子件：agent_parse在任何工具执行前拒绝多顶层对象，保no_tools纯文字路径；不以有歧义tool_status设计实施成功闸门。现设计handled/read失败矛盾、key64截断/exec首词碰撞及unknown语义仍需修订。多动作整体拒绝不等于已解决拒绝后假完成，本轮验收不得混称。

2026-10-08 多对象前拒绝落盘后slice agent selftest红：agent_cli.c154-156明确旧契约keeps first，需同步成ACT_ERR整拒；这不是放宽验收，父还需实际双write零副作用与单对象重试。agent.c第三处删除旧追加提示尚在进行，以冻结盘面为准。

H1-2 补片：块内函数原型按 C99 6.2.2p4 参与同单元的有效联结历史，声明种类作为共用检查入口参数，不借用外层函数的 fstat_cur；块内 extern 对象另待参考判据补齐。

2026-10-08 agent.c三处多动作前拒绝父读盘齐，csih1冻结回执。永久真实副作用probe交准备，等agent_cli旧断言同步冻结才实跑。watch审查/tmp/csih-watch-publisher-review.md仅设计：缺明确peer/session/DIR三元组、受控outbox生成，当前不能仅放行publisher冒充完整投递；规则实现未开始。

2026-10-08 多动作规则agent.c+旧断言agent_cli.c均冻结，父完整candidate构建/tui+agent门禁rc0/frozen=true，hash11b1e5e8cc0a35d6cba8c0b1e4cd7ee48fea13280643ee7059365400bc6cfc7c，收据/tmp/csih-multi-build-parent.json。真实副作用probe已允许实跑，尚未新规则部署；不以门禁代替双文件未写出的验收。

2026-10-08 真实multi probe已准备py_compile并启动五例矩阵，compile14/run9/selector55/outer60，源码冻结。父另核工具API：shell_result实际无rc字段，ok表示shell运行而非exit0，需exited/status/signal/timeout/err；plugin_api注释成功0与agent_file处理返回又不同层。后续toolstatus必须从原始字段记录，不能直接照文档shell_result.rc写实现。

2026-10-08 multi真实五例4绿/frozen：双write三次拒绝两文件无副作用，随后单write恢复实际正文，双exec拒绝零touch，无工具括号纯文通过。single-fenced-braces真实失败：说明后fence被agent_strip_fence当结束，单write文件未创建、随后answer仍rc0；原收据/tmp/csih-agent-multi-action-agent-review.json保留，不降测试。派csih1修说明+围栏兼容，假完成尚未修。

2026-10-08 解析修片尚进行：object_count已加backtick行跳过，strip_fence仍旧。父核英文前导next的n无效JSON仍break可漏两个动作，继续同件纠偏；坏容器/字符串必须拒，不能扫描内部当新合法动作。新probe已备两fence负例，等待冻结，不提前实跑。

2026-10-08 父读计数修片：英文t/f/n无效前导已跳过，畸形容器仍break返回0/既有数量，agent_parse仅>1拒绝后可从数组内部strchr提取合法动作。需明确count错误负值及parse恰1，避免仅停止计数被误当可执行；交csih1收敛一次契约，不动编译器。

2026-10-08 csih1连续多轮动作JSON转义失败达到3次终止，真实strip修片未落，不把失败叫完成。父收敛剩余任务：计数已恰1后agent_parse直接从原content提唯一对象，不再调用错误strip helper；两简单替换避免反复转义围栏字符串。仍由csih1改生产，父未下场。九例未跑。

2026-10-08 主人明确反馈“没什么进展、仍很差”。父承认管理过度碎片化，基础功能进展不等于自主工作体验。收口当前解析修复后，优先真实任务端到端验收：独立完成/实际工具证据、失败诚实终止、无确认循环；暂停新增设计文档作为主要交付。避免继续把局部测试绿当整体好用。

2026-10-08 主人要求咨询cdxwjhk-ios取得产品经理skills并学习产品设计；已纳入当前csih管理目标，先问准确SKILL.md路径/来源再读原文，不凭转述自称已学习。当前解析修片已由csih1冻结，九例实跑收口继续。

2026-10-08 产品skill咨询已真实envelope投递cdxwjhk-ios，capture确认待其下一工具后提交队列，未中断其他工作。多动作解析r2九例真实全PASS/frozen，收据/tmp/csih-agent-multi-action-r2-agent-review.json；selftest补核进行，未把已清临时快照诊断称实际跑过。产品skills未收到前不声称学习完成。

2026-10-08 产品设计学习与当前优先级（主人授权，非完成声明）
来源已实际读取：/Users/wjc/.codex-wjhk2020/skills/product-design-and-ux/SKILL.md、product-manager/SKILL.md；同时读AI uncertainty/interaction contract/task flow/content cognitive demand。对象是主人与写/看agent；目标是一次委托能获得可核实结果且异常可恢复，核心工具file+exec、父不直接写生产代码约束继续有效。
- 当前主问题：真实窗口多次动作解析失败、失败后answer判judge ok、需要父逐片纠偏；主人明确反馈仍难用。基础热换版/通知stub验收是真进展，但真实模型自主任务表现UNKNOWN。
- 首要产品结果：明确任务→执行与可取消状态→真实产物与验证→完成或失败/部分完成。完成必须区别“模型已答”，失败须显示失败步骤、已发生副作用、保留的草稿/上下文及可执行恢复路径。禁止以文本自述成功代替证据。
- 可观测验收：同一真实任务无需父提供逐行补丁或中途轻推，实际产物及独立检查吻合；失败任务保留证据不称通过、不自动重放副作用；返回窗口能辨认正在执行/等待/部分完成/已失败，不用从旧滚屏猜当前状态。均为待验目标，不是已证明指标。
- 产品取舍：先解决任务完成与诚实状态，再优化恢复/消息协作；不把更多命令、agent角色、设计文档或测试数当独立用户价值。模型负责理解/产出，消息分类、输入保留、单所有者与执行证据采用确定性规则；不能替模型虚构任意任务语义完成证明。
- 下一实验：收口当前九例解析修复与完整门禁后，私有工作目录做一件真实模型端到端编码/验证任务，父仅给目标与验收标准，记录产物、真实工具、用时/纠偏次数/失败报告；结果用于决定继续工作流修正还是产品状态/恢复改进。当前不扩新功能范围、不发布。

2026-10-08 真实产品任务基线开始：九例解析与同产物selftest通过后父完整candidate门禁rc0/frozen，hashab728fd3ac36dd3a1aeeee7ba96ba3abef823ba2c02e842edea166e721507827。先csih1空闲/reload-code切到此候选，再私有目录完整整数求和CLI任务，仅目标/验收/编译器入口，不提供补丁不中途轻推。记录成功与失败、实际产物及纠偏次数，不以模型answer判完成。

2026-10-08 首个真实模型产品任务失败，收据/tmp/csih-real-task-sum-parent.json：私有目录只有request.json，无sum.c/可执行；3次tool动作+answer多对象输出全拒，mail终态failed/too many unparseable steps，无中途轻推或父代写。严格解析已阻止部分副作用且失败诚实，但自主任务能力不达标。当前prompt仍写“一次只跑第一个JSON”与新整拒规则矛盾，长期journal大量多对象错误示例亦可能污染（推断，未证原因）；下一必须修输出契约并对同真实目标复验，不能用九例stub绿替代。

2026-10-08 真实任务失败后的下一动作：委派专职agent唯一agent.c两处prompt文案，消除base“一次只跑第一个JSON”与整拒冲突、ACT_ERR明确整条未执行且tool/answer不能同响应；不改解析/预算/角色/go。父不写生产C，保失败基线，候选门禁后同真实求和任务复验。这个调整是否改善真实模型行为尚未证明。

2026-10-08 prompt两文案专职agent已实现，agent.c hash6b438e543e446768d57d0a4d50eb46d8251ea5e711b9411cf694a8cd9d28b963；私有native编译0/selftest唯一旧rules只跑第一个硬断言红，证据/tmp/csih-action-contract-prompt-review.json保留。已派唯一agent_cli断言同步为恰好一个JSON且等待真实结果，保下一窗检查；不放宽为空测试，不恢复错误prompt。未部署/真实任务复验尚未开始。

2026-10-08 单对象prompt/错误反馈修复及旧断言已同步，父完整candidate门禁rc0/frozen，hash9549ccb4f1ccd84239b50adc56362d450bdfee90f4aec143b35b4886c5b24460；专职验收初record错用TUI marker已保initial纠正agent真实marker。开始csih1/reload-code与原求和任务同目标复验，无中途提示，不修改失败基线。

## 0.0.x 退出条件：自有脚本体系完全迁入 C/.cx（主人 2026-10-08 新裁定）

版本分工与退出验收见 [0.0.x 路线图](plans/roadmap-0.0.x.md)（79667f4d）；从 0.0.35 起不新增 .py/.sh。

进入 0.1.x 前完成，不顺延到 0.1.x/0.2.x：仓库自有 .py 与 .sh 清零，以 unisacc.com 与 .c/.cx 实现模型构造、构建、自举、测试、队列、缓存、报告及发布编排的日常工具链。当前 Git 跟踪基线为 .py 412、.sh 191；archive/ 与独立子项目 ujs/ 不默认豁免，应逐项明确迁移或经等价替代后淘汰。未跟踪脚本另盘点，不能用不跟踪来绕过验收。

验收不是后缀改名或包装 Python/shell：不得把脚本正文藏进其他扩展名或字符串，不得以 .cx 间接调用 python、sh/bash/zsh 等完成必需步骤。移除 Python、第三方 C 编译器和 shell 的测试环境中，以前一版 unisacc.com 完成下一版构建、自举不动点及本地适用的完整测试；外部对照编译器仅作可独立关闭的验证预言机，不能成为自身构建/测试运行器依赖。各文件迁移须保留原检查内容与失败语义，并有故障注入及新旧结果对拍。

实施按依赖族而非逐个翻译：先共用 .cx 工具基础（模块/文件/进程/JSON/TSV/超时/退出码），再构建与生成辅助、测试运行器与队列、发布编排、归档及子项目清点。已落地的小脚本迁移继续推进；完整 Rust 风格安全检查不作为脚本替代的先决条件。cc 重排剩余 0.0.x 的版本与分工；0.1.x 的入口由上述实证控制，不能仅按版本号宣布完成。

2026-10-08 prompt修复候选9549已csih1热部署/newnative34883，真实同目标r2失败：sum.c实际落盘但编译动作再次tool+answer混输整拒，终态failed/3parse，无可执行；收据/tmp/csih-real-task-sum-r2-parent.json，零中途轻推/父未代写。不能称prompt已解决；下一同规则全新会话与同等任务对照，隔离历史污染假说（仍未经证明）。

2026-10-08 新上下文对照已委派独立私有CTTY/managed真实默认模型，复用已验9549候选initial，不重建不改endpoint/key不输出秘密，不触碰两工作窗；同求和任务新work/空journal，45秒观察、每运行3秒。回执/tmp/csih-real-task-fresh-context-review.json，父不提供补丁/中途轻推。通过须实际独立运行程序，结果未得不得归因旧history。

2026-10-08 真实r2遗留sum.c父独立私有副本编译核验：默认unisacc rc0，正常/负数/无参/12x/越界/101参数六检查全部通过，证据/tmp/csih-sum-source-independent-review.json。这说明本次代码产出质量可用，但agent未成功编译执行，产品任务仍failed；父检查不是补做任务或伪造agent通过，原工作目录未创建sum。

2026-10-08 真实新空journal/9549同规则对照：actor独立创建sum.c/sum，约25.58秒，独立六检查通过，无轻推，正常CTTY退出/owner释放。原回执/tmp/csih-real-task-fresh-context-review.json，但父发现任务禁止发确认而仍实际envelope sum-done（来自继承pane cdx-unisacc），已要求补违例修正版，程序通过不等于整个交互通过。新空上下文成功支持上下文污染假说非唯一根因证明；下一产品重点明确任务边界/可保留日志的上下文隔离，非简单清空历史或丢记忆。

2026-10-08 任务边界最小实施确认：复用真实三键mail_id32hex/result=started/reason=accepted的持久metadata，agent_build_messages只发送最后合法起点之后eligible角色记录；notice无metadata，普通澄清续办保同边界。完整journal不删，含该marker自动trim返回0不截断（当前调用忽略返回继续），旧无marker策略不变；热恢复靠已校验原journal/offset而非新增v2字段。父已核trim调用2052。身份不由marker认证；单可信ACTIVE writer前提。待回归包括热恢复、伪marker及释放json无泄漏。

2026-10-08 任务边界核心父读盘核已落agent.c：agent_task_boundary精确三键/重复计数/32lowerhex/started accepted及NULescape拒绝，builder旧held逐个jfree/reset nvals，trim有合法marker返回0保完整journal。唯一task_context.py回归正在准备，预计结构边界/CTTY任务→notice→澄清→第二任务→真实handoff继续请求。尚未门禁/部署，不能将结构实现称长期真实模型问题已解。

H1 块内 extern 对象补片：仅在同单元已有可见文件域对象（LOC=GMARK 且独立联结 epoch 匹配）时复用现有描述符及存储；较近自动/静态局部遮蔽或无声明仍按名未覆盖。外部声明后缀只扫描，不执行维度或初始化表达式、不分配帧槽；初始化按 C99 6.7.8p5 拒绝。

### 2026-10-08 任务上下文隔离冻结验收与部署安排
结构10例、真实CTTY交接8请求及既有inbox任务回归通过，证据 /tmp/csih-task-context-isolation-review.json；生产agent.c SHA256 700d6e499ded6cd4b890743721dccd4082f5e3648d94e1c3465e7426c1ad0c10。此为机制证据，不证明真实模型长期会话完成。csih1当前9549版本空闲且上一任务明确失败，安排受控reload后新私有目录复测；保留旧journal和失败记录，不删除历史，不改编译器。fresh程序成功但违反不发确认约束，整体仍失败。

csih1实机reload完成：committed d616ba36b615093971544f2034b892dcd013c3afbabf8df9e3efb3360ca026fd，native15362，旧34883已退出；候选 candidate-81a3c4b59cba48a8aee5d9f3695306eb，TUI/agent编译及自测四项rc0。真实任务复测尚未投递，不能据此宣称解决长期会话失败。

### 2026-10-08 长期会话真实任务复测
实机d616新task无中途提醒，28.55秒终态started→ok，sum程序六独立检查通过，旧journal前缀完整；然而明确禁止同伴确认仍两次调用envelope，最终消息真实送达，整体失败。证据 /tmp/csih-real-task-long-context-review.json。任务边界后程序交付成功，仅单次证据；下一件纠正基础提示的硬编码信封义务与用户明确约束冲突，不能把程序成功升级为整体通过。

2026-10-08 回报提示修订冻结：agent.c基础/角色/seed去旧peer与默认强制信封义务，默认answer、外部通信须当前任务明确授权；agent_cli核实际write/watch请求。真实compile/selftest及完整candidate门禁通过 /tmp/csih-report-policy-review.json。提示不是强制权限隔离，watch投递门禁未修改，仍需真实任务验证。安排csih1空闲边界reload，不改旧失败记录。

2026-10-08 3458实机真实复测：20.87秒，六程序检查与started→ok、无envelope均通过；但父读实际cmd发现向/tmp/o1/e1/o2/e2重定向，违反只在私有目录工作的边界。原driver漏检查此约束，原绿收据保留，更正 /tmp/csih-real-task-report-policy-review-corrected.json 整体失败；不宣称独立完整通过。下一验收须同时覆盖外部通信与私有目录副作用，不能仅查程序功能。

2026-10-08 工作范围只读核实：shell.c:171仅子进程chdir(cwd)，197 execvp(/bin/sh)；agent_under允许绝对路径，均非写入隔离。csih1 PID47693/3458与csih2 PID5970/ecd当前存活。下一件应优先任务目录内临时文件惯例和真实副作用审查；硬隔离另列能力，不用shell文本匹配冒充通用证明，不将单次/tmp越界扩成默认shell重写。

2026-10-08 采纳scope审查最小下一件 /tmp/csih-task-scope-review.md（仅设计未实现）：工具说明明确cwd非隔离，任务指定范围内临时输出/清理及副作用诚实汇报；私有driver显式allowed_work_dir与scope_mode=instruction，拆功能/通信/scope审查/强制隔离证据，不用无/tmp字符串或目录快照证明无外部写入。强隔离另项待实际宿主证据，本轮不重写shell，不扩消息schema。

2026-10-08 新只读发现需后续验收：agent.c ACT_ANSWER在 AT.round+1>=MAX_ROUNDS分支直接AT.res.ok=1/reason reached MAX_ROUNDS，不能据历史门禁称所有预算末尾都诚实；当前scope件冻结后优先核此实际触发与预算末尾失败语义。工具主循环丢弃agent_exec返回值，file write handled返回由参数非空决定，不能从handled推真实success。下一能力验收覆盖实际失败→修复/未修复终态，不继续仅用sum功能绿代表harness可靠。

2026-10-08 预算风险补核：probes/agent_loop_bound.py现有last_round_answer明确7次continue+ANSWER期望rc0，故maxround直接green是现行已测契约，不能仅以代码分支称新bug。下一件须实际未修复失败与正常最终完成对照，解决验收证据/状态而非最后轮一律拒绝。scope两生产路径已落提示与ctx_preview测试，门禁未收终态前不部署。

2026-10-08 scope最小实现门禁通过 /tmp/csih-scope-prompt-review.json，candidate1577e436，父独立verify及全after哈希核验。旧明确越界fixture失败、隐藏副作用fixture未知，不将unknown自动成功。只提示/实际system测试与私有driver证据，无隔离/预算改动。安排空闲csih1受控reload与真实任务有限人工审查。

2026-10-08 scope1577真实复测20.34秒：六程序检查通过、无通信，仍向/tmp六测试文件重定向；answer本次诚实承认越界，但journal仍started→ok judge ok。父有限命令审查scope失败 /tmp/csih-real-task-scope-parent-review.json。提示改善披露而未阻止违约，不再反复sum/补一句提示；下一重点真实失败状态与约束违反不得自称ok，需结构化验收来源而非中文匹配。

2026-10-08 outcome私有实证 /tmp/csih-outcome-evidence-review.json：冻结1577现有artifact，正常末轮8POST rc0；真实exec false exit1、未修复后7continue、末轮answer outcome failed仍9POST rc0并✓answer，binary前后哈希一致。明确失败声明被旧parser忽略，终态错误升级有实跑证据，非仅代码猜测。等待≤60行完整结果契约设计，涉及agent_result重复ABI与TUI/mail通路同步，不用中文匹配。

2026-10-08 批准outcome贯通实现原则：answer显式completed/partial/failed，缺字段旧答复为unverified并可显示，不默默默认completed；失败/部分声明不能被stop/maxround升级。状态经所有agent_result ABI声明同步到CLI/TUI与mail lifecycle；failed/partial仍展示原正文/已有产物便于恢复。无文本中文匹配、无算法完成证明承诺；保持watch真实投递/取消/预算上限，不为了绿删旧测试，合法新契约更新 fixtures 并保留legacy-unverified回归。专职agent可在最小报告落盘后直接实施上述范围，父负责验收与部署。

2026-10-08 outcome契约38行已核 /tmp/csih-outcome-contract-review.md，并已派完整实现：严格新answer outcome枚举，旧text/prose unverified保显示；非completed不被末轮/judge升级；末轮completed有界HTTP_END；agent.c/agent_cli.c/tui.c唯一ABI共享，同步CLI失败正文与TUI/mail三键终态partial/unverified，所有消费终态finish一次且不重派。声明不是真实验收证明，后续真实失败证据关联另验。现csih1八次committed记录，1577 PID74473存活；本件未部署。

2026-10-08 outcome落盘父审纠偏：ACT_ANSWER事件不得对failed/partial/unverified统一✓，completed在judge前也只是声明；outcome字符串embedded NUL不得通过strcmp前缀伪装；watch真实投递门禁只阻completed成功，显式非完成保正文可失败结束，不为获得收件成功而续问/无授权发信。已交实现agent，本轮测试冻结前修复并补负例。

2026-10-08 outcome首10例真实HTTP回归通过 /tmp/csih-outcome-loop-review.json，before==after。failed/partial/legacy一次回复非成功；失败末轮8POST rc1；完成末轮continue9POST预算失败；错枚举/重复/NUL三次解析失败；watch非完成一次终止保正文。此仅agent子集，不是TUI/mail全链路或整体候选验收；后续CTTY与协议探针迁移仍待。

2026-10-08 outcome全链路阶段证据：partial/unverified真实CTTY /tmp/csih-outcome-inbox-partial-review-r2.json 与 /tmp/csih-outcome-inbox-unverified-review.json passed、before==after；三键终态分别partial/unverified、6POST、不自动重派、busy草稿与handoff/termios通过。candidate /tmp/csih-outcome-candidate-review.json rc0，父verify f46fcf32856613afd4d613e766250faec4304bc13345d8b7ae9226137b346aae，TUI/agent四构建运行rc0，当前全C/H/INC manifest相同。旧loop分批仍待终态，未部署未真实模型，不称完成证据关联已解决。

2026-10-08 outcome旧loop两批21例通过且before==after：/tmp/csih-outcome-loop-regression-a.json 与 -b.json，含合法末轮stop9POST成功、预算失败、坏judge、watch伪回执等。已具备本件机制部署证据，安排csih1空闲reload；no_tools旧宽容坏JSON转unverified非成功边界保留，不伪称通用结构全部parsefail。

结果契约实机csih1已committed f46fcf32856613afd4d613e766250faec4304bc13345d8b7ae9226137b346aae PID92936，父verify身份与活进程核验，真实模型outcome使用仍待下一任务。

2026-10-08 f46f真实失败验收投递：读取确实缺失的私有required-input.txt，禁止创建/编造/写文件/通信，期望明确failed回执且无产物。证据/tmp/csih-real-missing-input-parent.json，不中途提醒，不代做。

真实缺输入任务验收通过（任务失败不等于harness验收失败）：实际file read err2，answer outcome failed保原因与缺失输入，mail started→failed、done一次；两模型动作仅read与answer，私有目录空、旧日志哈希前缀一致、零提醒/通信。证据/tmp/csih-real-missing-input-parent.json。此仅明确失败场景，不代表任意工具失败都阻止模型completed。

2026-10-08 真实修复任务投递：私有max.c有min/max错误，check.py固定哈希，要求先见失败→仅改max.c→unisacc重建→原四例通过；禁止通信与外部写入，0提醒。证据/tmp/csih-real-repair-parent.json，fixture非生产代码。

真实修复任务未完成：file/cd/只读检查后连续输出多对象（含user分隔的整套计划），三次解析拒绝→failed，未执行编译/原测试/修复，max.c和check.py哈希保持、无max产物。证据/tmp/csih-real-repair-parent.json。失败终态诚实但自主修复能力仍不可靠；下一核 rejected raw assistant 在同task请求里是否重放导致重复，不加重试预算/不执行批次首项来掩盖失败。

2026-10-08 rejected context真实HTTP捕获 /tmp/csih-rejected-context-evidence-review.json：冻结f46fartifact，合法pwd后连续多对象被拒，后续请求确实包含BAD_A/B原文assistant；4POSTrc1，artifact前后hash一致。证实错误示例重放，不证明它导致真实模型重复的唯一原因。下一推荐可信单行parser拒绝标记、model context排除原文而audit保留，合法assistant/错误反馈仍保留，预算不变。

2026-10-08 拒绝context最小实现已盘面核：严格同记录role/text/parse_status三键标记，builder跳过protocol_rejected原文；heap6*AT.content长度+256编码并检查json_rec/session_append，失败当前回合停止。仅保存实际已接收C字符串，不声称API超长截断前完整。仍待真实HTTP、审计写失败零副作用、合法角色历史、handoff与candidate门禁；不称因果问题已解决。

2026-10-08 拒绝context实现default native compile与agent selftest实际通过（专职agent报告3793终态rc0，父读/tmp/csih-rejected-context-selftest.stdout末行all cases pass）；两真实入口HTTP_ACT/坏HTTP_END已调用检查式heap audit。不以旧selftest宣称新过滤/写失败/handoff专用覆盖，新永久probe仍待实跑。

2026-10-08 rejected context最终验收 /tmp/csih-rejected-context-implementation-review.json：结构9类与managed9POST/handoff通过，精准audit唯一POST后err-7且零touch副作用通过before==after，原夹具失败保留。父独立verify2f5a040/source映射与TUI/agent四gate核；安排csih1空闲受控reload及同规格新私有fixture真实修复任务，不中途提醒，不把局部过滤绿当根因唯一证明。

2f5a实机csih1已committed PID14214，真实修复r2按原规格新fixture投递 /tmp/csih-real-repair-r2-parent.json，固定测试hash不变。尚未结果。

2026-10-08 2f5a真实修复r2通过 /tmp/csih-real-repair-r2-parent.json：0提醒，真实前测3FAIL1PASS EXIT1；仅精确改maximum比较符；同编译器重建原四例PASS EXIT0，父独立check.py rc0，固定测试hash不变、源码仅预期变更、无通信/外部写入已按具体命令有限审查、日志前缀完整、mail ok。本轮无protocol_rejected记录，单次成功不能证明过滤改动唯一因果或一般可靠；后续多任务/未修复失败证据仍需验。

2026-10-08 下一工具结果证据件：父只读agent_file缺文件return1、exec插件返回handled而循环忽略；现行tool正文exit信息可见但无可信结构化状态。先核实际result API并设计最小journal状态元数据传递，不将handled等同success，不用中文解析；预期负测试的exec非零不应永久锁死任务，task真实语义另验，不恢复旧错误pending设计。先独立报告/私有对照，再按必要范围实施。

2026-10-08 csih2盘面核空闲input空，仍ecd旧版本；同步已验收当前2f5a结果契约/拒绝context，受控reload保持watch/peer0:csih1与私有session，父核交接收据，不派自动观察/回信循环。

csih2同步committed2f5a PID16231，父verify候选/新actor与host68404存活、旧5970已gone；快照watch/peer0:csih1/sessioncsih2/input空保持，未触发观察或回信循环。两窗同已验收源身份。

2026-10-08 工具结果契约42行已核 /tmp/csih-tool-result-contract-review.md。冻结artifact七工具真实对照read/write/edit失败、空写0字节成功、false/pwd分别退出1/0后completed→rc0，证明handled不是success且当前字段只文本化，并非所有非零都应阻任务。批准最小事实链实施：同工具journal行绑定typed handled/op_success/err/exec状态/gate三态，model前置元数据不被正文裁剪，UI/CLI展示实际事实；read成功补ferror/fclose实际检查。保持执行权限/预算/角色，不新增永久失败锁或中文匹配，不声称completed独立验收已解决。

2026-10-08 已部署2f5a私有多文件修复验收投递：main输出换行与squeeze连续去重两缺陷，固定header/check.py哈希，先失败后仅改两源码、同unisacc命令重建原五例通过；0提醒，证据/tmp/csih-real-multifile-parent.json。不改生产、与元数据实施隔离，非新metadata验证。

2026-10-08 csih 双文件真实任务验收失败：4dbf8f75d86b4136c0394c6188b38994 实际首次五例 FAIL 后，没有任何修改/重建/复测动作，模型却声明 completed 且 judge stop，mail result ok；父独立复跑仍五例失败、main.c/squeeze.c 未变。证据 /tmp/csih-real-multifile-parent.json。outcome 字段及判官不是完成证明；工具元数据当前工作继续，但不得宣称已解决虚假完成。

2026-10-08 typed tool metadata 父审查：agent_fact_read 的浮点cast前需拒NaN；shell.c捕获循环timeout/read错误可保留非负bytes，故bytes>=0不能证明完整捕获，且模型正文另有1600裁剪。已要求实现代理修正为未证明null、确定截断/超时false，明确捕获与可见正文边界，不扩大shell架构。最小agent编译和selftest已实绿，但专项probe与新部署未完成。

2026-10-08 完成证据下一阶段设计方向：保持通用file+exec，不以任意非零永久禁止完成；每回合工具记录稳定action_id，completed提交可核对的证据引用，父构建判官输入时绑定当前回合真实记录而非仅自述。无引用/不存在/前回合引用不得升级成ok；引用存在也只证明动作执行，不等于任意任务语义完成。双文件真实虚报案例需永久脚本回归。设计 /tmp/csih-completion-evidence-next.md，尚未派实现。

2026-10-08 虚假完成追根新证据：agent.c at_judge_nudge 有answer分支提示“已经答完，只输出go:stop结束；只有还剩具体一步没做完才continue”，并未要求审查完成声明与真实动作，直接把有答复当作答完。该指令是明显的放行偏置，但仅凭源码不能证明是唯一因果。下一阶段需将收尾决策改为依据实际执行证据核对而非强推stop，并用红测试后虚构PASS回归验证；不靠提示词单独声称确定性修复。

2026-10-08 typed metadata core r2 父核验：/tmp/csih-tool-result-core-review-r2.json passed=true、before/after相同；真实8次调用含缺失读/坏父目录写/空文件写/正常写/无匹配edit/false/pwd/超时，各自op_success准确，false状态1后pwd状态0不抹旧事实，超时signal9/exitedfalse/statusnull/capturefalse。正常捕获完整性null，未伪称true。10个HTTP请求，编译/运行rc0，完成仍只是声明。门禁和CTTY交接专项待验。

2026-10-08 tool metadata专项首轮：gate私有compiler权限0600引发direct exec126，保留 /tmp/csih-tool-result-gate-review.json，不能据此算恢复绿；managed探针清理先删临时目录再停child且child异常可能覆盖父receipt，原失败信息不足，要求修测试生命周期。candidate b0181fe504e2f4810661d39a099a0dd4a0c7f78b4435c8773cca8911b5987226构建rc0，父reload_candidate.verify核验成功；尚不部署，待专项真实绿。

2026-10-08 父独立以冻结b0181fe候选源/编译器跑8项loop回归，外层50秒且各case9秒：normal、parsefail、failed/partial/legacy、invalid judge及恢复、watch timeout全部PASS，before=after；/tmp/csih-tool-result-parent-loop-review.json。不与主树实现并跑，不证明虚假completed已阻断。

2026-10-08 tool metadata gate r2父核验真实写成功但门禁失败，再恢复写与门禁成功，discovery/rows/last退出字段相符；/tmp/csih-tool-result-gate-review-r2.json。managed r2 8秒done观察超时仅6POST，不能称终态停摆，要求有界延长夹具观察或拆场景（总55秒不变）。UI当前每步铺17字段JSON在实拍占满内容区，不符合极简产品目标；部署前要求同typed事实生成简短人话状态，完整结构仅journal/API，不能文案匹配推导。

2026-10-08 完成决策设计补充：go=stop是停止控制而非验收通过，现有“有answer+stop=>ok”的职责混同应拆开。证据引用存在只是必要条件，失败测试引用不算验证；语义判定来源仍模型时应明示边界。先设计再实现，不增加预算强行工作。已补 /tmp/csih-completion-evidence-next.md。

2026-10-08 managed r3仍done观察超时6POST，生产短状态输出已改、但probe精确替换断言失败未落盘，调度却继续启动旧7calls/8秒夹具。代理承认调度失误，当前r3 before/after冻结、失败原文保留 /tmp/csih-tool-result-managed-review-r3.json；不得称等待修复已验证。要求下一次编辑成功与读回后才运行，不改运行中输入。

2026-10-08 短状态候选a0218f53898309623d49b884f084372d4f866bb2fd457f0f63b217d842ae2020 父独立reload_candidate.verify通过，TUI/agent build/run四门禁rc0且source_manifest与当前树一致；目录 /var/folders/kv/yf8l6q994rl27550kd5985y40000gn/T/csih-tool-result-short-candidate-nkbk80kc/candidate-45d095f5795d4d38afd4e65075ac084b。只候选验证，不等于managed专项或部署。

2026-10-08 managed r4实跑通过：/tmp/csih-tool-result-managed-review-r4.json 10POST、before=after/current一致，真实控制TTY候选交接commit，输入工具事实交接草稿与journal偏移保存，交接后旧tool失败/成功事实仍进API；child rc0/termios恢复。四代表调用与core八类覆盖互补，不宣称本managed覆盖全部工具。先核新版core/gate后安排受控部署。

2026-10-08 工具元数据与简短状态专项全部通过后，安排csih1/2逐个reload-code到冻结a0218候选；先核空闲/input空/queue空，保存旧PID与事件偏移，再验READY/FROZEN/ACK/commit、新身份与旧进程退出。未把部署称虚假完成已修。

2026-10-08 csih1受控热更新a0218成功，host40541保留/new29231/old14214已退出，READY/FROZEN/RELEASED/ACK/activated/committed完整、候选verify成功。证据 /tmp/csih-tool-result-csih1-deploy-parent.json；csih2尚未更新。

2026-10-08 csih2先核活进程/空输入空队列与rolewatch peer0:csih1，安排同a0218受控热更新；设计代理已续派唯一完成契约设计，不动冻结生产。

2026-10-08 csih2热更新a0218成功：新native29994、旧16231退出、host68404保持，rolewatch/peer0:csih1/input/queue/goal保持；完整握手commit及候选verify通过。证据 /tmp/csih-tool-result-csih2-deploy-parent.json。

2026-10-08 双窗部署a0218后，用新私有目录原双文件同规格真实模型任务复跑，不加修复方法/中途提示；原失败保留，重复结果不证明唯一因果。证据 /tmp/csih-real-multifile-r2-parent.json，任务31358211656569fa3c363764f5e0ce2d。

2026-10-08 真实双文件r2这次确实先5FAIL→写两文件→unisacc重建5PASS，父原测试复跑rc0、固定文件未改、无中途提示。但代码把newline移入squeeze helper：main允许strlen511/out[512]，helper写511字符+newline+NUL共513字节，源码证明越界。边界运行暂rc0不能消除UB；因此仅五例通过，不计产品整体通过。证据 /tmp/csih-real-multifile-r2-parent.json。下一纠正交csih1，不由父改代码。

2026-10-08 已向csih1派私有双文件边界审查纠正任务：保持原接受长度、原固定测试/接口，修511不连续重复+newline+NUL越界并验证512拒绝。此为明确指导后的恢复，不混入无提示任务成功率，父不改代码。证据 /tmp/csih-real-multifile-boundary-recovery-parent.json。

2026-10-08 边界恢复过程中父独立运行agent实际生成的squeeze_asan，对511交替ab输入ASan明确stack-buffer-overflow、WRITE1/out范围[32,544)在544越界，rc-6，binary hash前后相同。证据 /tmp/csih-real-multifile-boundary-asan-parent.json。这是实际内存错误证据，不仅源码猜测；尚待当前恢复回合真实终态。

2026-10-08 边界恢复终态failed/MAX_ACTIONS16，未虚报成功；原ASan证明仍越界，当前恢复未完成。完成契约设计51行已交 /tmp/csih-completion-contract-review.md：两段schema/当前turn工具引用/独立收尾输入/旧stop降unverified/完整ABI迁移，仅设计。父审仍需明确“有证据引用+模型accepted”不构成确定性防虚报，HTTP stub自称accepted与真实语义评估应分开验；不能以强行拒所有负测简化任务。

2026-10-08 继续管理csih1私有恢复，明确转交真实ASan WRITE1和上轮无效修改事实，要求不重复无关ls/read而解决容量上界，unisacc原五例与ASan合法/非法边界实跑。属于二次指导恢复，不冒充独立任务benchmark。证据 /tmp/csih-real-multifile-boundary-r2-parent.json。

2026-10-08 二次边界恢复结束partial，没有虚报验证通过；仅11个当前工具行却自述本轮上限已到（未有真实预算终态）。新实现511字符跳过newline避免越界，牺牲要求；父运行输出511字节无换行，仍不通过。/tmp/csih-real-multifile-boundary-r2-parent.json。需要引导容量职责而非以省略输出绕过规范；不加预算。

2026-10-08 三次指导私有恢复明确职责：helper只压缩+NUL，main将newline直接写stdout，不牺牲511输入输出。父只给指示，不写代码。原partial与11工具预算自述不符事实保留。证据 /tmp/csih-real-multifile-boundary-r3-parent.json。

2026-10-08 父核完成契约设计55行纠偏后，批准专职代理贯通实现：当前turn/action工具事实与引用、独立收尾证据输入、stop只停止、acceptance/scope独立终态、共享ABI/CLI/TUI/mail一致，旧协议停但未验证；有界分阶段先最小编译再冻结实测。硬性来源门禁与真实语义模型观察严格分开，合法证据+模型accepted仍可虚报，不声称确定性证明。不改预算/权限/编译器/state-v2，不用只ID绑定代替目标。

2026-10-08 三次明确指导恢复仍failed：DSML混合动作与ds_safety分类文字造成三次协议拒绝终止，原文已审计，未执行混合动作。终态非成功；这是默认模型协议遵从问题，不能靠放松多动作拒绝/把安全文字当执行结果绕过。证据 /tmp/csih-real-multifile-boundary-r3-parent.json 对应journal。

2026-10-08 三次指导后agent实际两源码已正确分职责；父冻结拷贝独立unisacc/ASan重建，原五例与511输出完整newline/512拒绝皆PASS，源码hash保持。父验收证明产物源码已修，不把它算agent自主完成：agent自身仍协议失败且未跑所要求测试。证据 /tmp/csih-real-multifile-boundary-r3-parent.json。

2026-10-08 协议遵从调研：当前agent请求经json_model只含model/messages/stream，无response_format/native tools；真实失败出现DSML与ds_safety附加文案。DeepSeek官方JSON Output文档 https://api-docs.deepseek.com/guides/json_mode/ 搜索结果确认response_format={type:json_object}能力。尚未验证当前实际endpoint/model支持与效果，不因此放松本地严格parser，也不混入正在实现的验收契约；后续私有A/B实测JSON模式可作为协议改善候选，合法JSON仍不证明act/schema或任务完成。

2026-10-08 安排私有JSON接口能力对照：同deepseek-chat/direct endpoint，普通与json_object各一次无工具精确JSON探针，每请求18秒/外层45秒，凭据只内存读取且不写输出。此只接口支持实验，非harness成功率A/B、非任务完成证明；/tmp/csih-json-mode-capability-parent.json。

2026-10-08 私有能力探针结果：plain与json_object均HTTP200且返回精确单JSON，1.326/1.04秒；实测当前direct deepseek-chat接受response_format，但两样本都绿不能证明JSON模式改善协议稳定性。不改窗口/默认请求；后续完整真实任务对照需另测。

2026-10-08 安排只读协议对照：取部署a0218真实系统提示词与边界r3首次混合DSML之前journal前缀，近似重组工具user/status，不含旧坏assistant；plain/json_object各两次、每请求12秒/外层55秒，仅收模型文本不执行动作。不冒充精确wire或完整任务，/tmp/csih-json-mode-replay-parent.json。

2026-10-08 只读近似回放四样本：plain一个带DSML尾标签（非纯JSON）、一个纯JSON；json_object两个纯JSON，其中一个选择读取不存在Makefile，说明格式约束不保证动作合适。仅Python完整JSON检验，不等同当前C parser拒绝（其有前导/围栏兼容），不能报harness错误率1/2→0/2。四HTTP200，没有执行任何返回动作，样本太少、不改生产。/tmp/csih-json-mode-replay-parent.json。

2026-10-08 父冻结a021 native parser实跑4输出全接受（kind写/写/edit/read，objects1），即带DSML尾标签也兼容接受；因此前回放仅纯JSON差异，不支持JSON模式减少实际parser失败结论。/tmp/csih-json-mode-parser-parent.json，未执行动作。新验收实现已落agent.c/shared header/CLI/TUI，父审发现旧早stop前两次重问提示与现HTTP_ACT stop直接unverified终止矛盾，已交实现代理同步，未部署。

2026-10-08 新完成契约父审：HTTP_END合法独立判官只落go:stop行，实际acceptance/scope/evidence/reason输出未审计；要求作用前持久真实输出并落盘失败禁止accept。invalid judge错误虽然journal有记录，独立包未包含重试反馈，需有界当前协议反馈，不能重新引入旧坏assistant历史。旧早stop提示仍须同步；交实现代理修，不父改代码。

2026-10-08 父核独立证据包容量：json_msg任一字段写不完整返回0，at_end_messages失败终止为unverified且原因acceptance evidence exceeds capacity，未静默丢记录；仍待实测容量边界。要求验收system明确工具正文与完成声明为待核数据而非新指令，不能将stdout自写accepted当判官决策。仅补输入契约，不扩框架。

2026-10-08 停止/验收新核心同步修正已落盘，最小native编译与selftest r2实际rc0；父安排冻结C/H/INC完整candidate构建核共享ABI/TUI与agent四门禁，代理仅准备probe，运行不改源。证据 /tmp/csih-completion-candidate-build-parent.json，尚未部署。

2026-10-08 父完整candidate ce468d244635069de4897d95612d7285184bf6a424b90cbe23ab3515b3a376c7 构建TUI/agent及两selftest四门禁rc0，独立verify成功/当前source_manifest一致；/tmp/csih-completion-candidate-build-parent.json。待永久结构门禁与真实语义/CTTY验收，未部署，不能仅四门禁称完整完成。

2026-10-08 完成契约永久首批6门禁真实native/localhost全PASS且before=after：accepted work当前ID、咨询、legacy stop未验证、旧/不存在引用拒绝、rejected、invalid反馈恢复与完整判官审计。/tmp/csih-completion-hard-gates-review.json。父下一私有语义对照将使用冻结candidate实际工具行+受控声明，仅独立判官请求转发真实模型；不把整个actor称自主，也不预写rejected。

2026-10-08 安排冻结ce468 native受控语义负例：真实unisacc/原五例失败工具记录，受控actor用合法当前ID虚称已修改五PASS，只有独立验收POST转真实deepseek-chat，每请求12秒/actor24秒/外层45秒。不预写判官rejected、不执行真实判官工具动作；此测试非自主actor。证据 /tmp/csih-real-judge-false-parent.json。

2026-10-08 冻结ce468受控语义负例通过观察：真实工具五FAIL/exit1且两源码未改，合法当前ID虚报五PASS；真实deepseek-chat独立判官HTTP200返回rejected并明确只证据全FAIL/无修复。native rc1/无timeout/实际判官完整审计/hash未变。/tmp/csih-real-judge-false-parent.json。只证明此一次受控负例被识别，不证明通用确定性防虚报或自主actor能力。

2026-10-08 安排ce468同真实判官正例对照：用父已冻结验证的修正源码，真实unisacc重建原五例PASS，受控actor只声明这五例验证/未修改源码、不泛化边界，独立judge转真实模型。任务只验证五例、不假称本回合修复，source保持；/tmp/csih-real-judge-true-parent.json，非自主actor。

2026-10-08 csih completion 正例对照：/tmp/csih-real-judge-true-parent.json 独立核验 rc0、指定 unisacc 实际构建、固定五例 PASS、当前真实 action_id 引用、真实 DeepSeek 独立 accepted/work 完整落日志，源码及候选未变化。仅受控执行者+真实判官的验证任务，不是自主修复或任意任务完成证明；与 false-parent 拒绝假完成构成有限正反对照。最终候选仍需重冻结，尚未部署。

2026-10-08 csih1 sum 回执独立验收：/tmp/csih-sum-long-independent-parent.json，现有产物7例实际通过（正负数/无参/非法字串/越界/101参），源码与二进制哈希已记。此检查未重新构建，不把窗口自报的编译来源当父证实，也不据单件推断 harness 整体达标。

2026-10-08 completion最终候选已构建并核验四门禁与当前全输入一致：{"candidate_dir": "/private/var/folders/kv/yf8l6q994rl27550kd5985y40000gn/T/csih-completion-final-parent-jgth9fk4/candidate-7703cc5b97f84b9a84e59f37c6eab783", "hash": "4a4f43302ed1decd176533c43ae9dbe9c59e289d5b78f175cc3166c1c2b3f1fb", "binary": "/private/var/folders/kv/yf8l6q994rl27550kd5985y40000gn/T/csih-completion-final-parent-jgth9fk4/candidate-7703cc5b97f84b9a84e59f37c6eab783/candidate", "receipt": "/private/var/folders/kv/yf8l6q994rl27550kd5985y40000gn/T/csih-completion-final-parent-jgth9fk4/candidate-7703cc5b97f84b9a84e59f37c6eab783/receipt.json"}。父第一次后置manifest核对误传str导致AttributeError，原构建本身已完成；未重建，改传Path后独立verify成功。终端负态补测仍待结果，未部署。

2026-10-08 completion负终态实际CTTY两例通过，partial正文/部分完成与unverified正文/未验证可见且不显示成功，mail各started+一次对应非成功终态，忙时草稿/后缀保留，交接及终端恢复经收据核验；前后冻结一致。最终4a4f433候选进入顺序部署，先csih1再csih2，逐窗核验不同时操作。

2026-10-08 csih1真实热换版通过 /tmp/csih-completion-csih1-deploy-parent.json：4a4f433，native29231→48553、launcher40541保持、READY/FROZEN/RELEASED/ACK/committed，旧PID消失、角色peer/空草稿pending保持、候选收据独立verify，窗口显式新身份。csih2换版正在执行。

2026-10-08 csih2真实热换版通过 /tmp/csih-completion-csih2-deploy-parent.json：4a4f433，native29994→48926、launcher68404保持、完整交接/committed、旧PID消失、watch/0:csih1及空输入pending保持，候选收据独立verify，窗口显示新身份。两窗均已部署completion独立语义验收契约；自主真实任务与协议可靠性尚待实测，不能据受控正反例声称总体完成。

2026-10-08 新版自主真实任务启动：私有squeeze双文件修复，固定7例含511完整输出和512拒绝；要求真实红→修改→指定unisacc绿、不改check/header、不外信，父不提示修法，回执 /tmp/csih-completion-real-task-parent.json。

2026-10-08 新版真实自主任务未通过：/tmp/csih-completion-real-task-parent.json。9真实工具动作，红6FAIL/1PASS→仅改squeeze.c→固定7PASS，父独立7PASS且check/header哈希不变；但helper仍将511字符+换行+NUL写入main out512，父ASan实际确认stack-buffer-overflow。真实独立判官两次倾向accepted未识别容量错误，首条证据集合不一致被拒，第二条结构合法但终态unverified（须再核具体原因），不能当验收成功或ASan主动自测。父仅测试未改源码。

2026-10-08 真实任务引导恢复：明确转交父ASan的511字节越界证据给csih1，修法由其选择，要求指定unisacc固定七例+本目录实际ASan/UBSan边界检查；不算自主基准。回执 /tmp/csih-completion-real-recovery-parent.json。

2026-10-08 归因更正（保留前文）：自主任务第一judge无效实际reason473字节超过judgment_reason320容量，agent_parse拒绝，不是证据集合校验；第二reason298合法，但judge refs[-8,-9]与claim仅[-9]不一致，at_accept_stop强制unverified，符合严格同集合契约。工具write只记录wrote228bytes/path，独立packet有完整初始源码但没有修后源码正文；不可声称判官看过完整最终源码。该诊断提示需明确协议长度/精确集合反馈，并增强可验收源事实，不能放宽假完成门禁。

2026-10-08 引导恢复回合终态failed/MAX_ACTIONS16，/tmp/csih-completion-real-recovery-parent.json 保留实际15工具记录与失败自测；父独立冻结当前源码再ASan构建边界复核结果=False。不能当自主或恢复成功。反馈设计交专职agent，父未修源码。

2026-10-08 恢复r2明确职责引导：helper只压缩+NUL，main输出换行，仍由csih1修改私有任务源码，固定七例和实际ASan511/512检查，不计自主成功。回执/tmp/csih-completion-real-recovery-r2-parent.json。

2026-10-08 批准completion协议反馈实现切片，设计 /tmp/csih-completion-protocol-feedback-next.md 已读审：reason UTF8 1..319明确、safe schema诊断、accepted精确claim集合、共现MAX_JUDGE/回复预算有限恢复、终止准确原因与审计fail closed；不放宽接受定义、不改ABI/角色/编译器/工具、不混最终源码证据项。专职agent实现并原生定向回归，父验收后另冻结候选，暂不部署。

2026-10-08 恢复r2失败MAX_ACTIONS，模型猜d1并搜仓库；父派单漏重复编译器路径（新任务隔离旧上下文）是明确父指示缺口，不能全归因csih。r3完整给/bin/sh根unisacc.com准确命令，禁止入口调查，仍要求固定七例与ASan511/512，不算自主成功；原失败/搜索越界事实保留。

2026-10-08 r3终态failed因csih1误用subprocess stdin代argv，错误提出需改接口。父当前源码实际ASan/UBSan511完整stdout与512拒绝均过，header/check不变；记录产物修复与agent流程失败分开。r4只纠正argv验证方法，不改源码/接口，不算自主成功。模型还追加虚构“权限确认未执行”文字但exec实际执行，作为协议/事实表达问题保留。

2026-10-08 恢复r4真实argv自测通过（ARGV-OK rc0 stdout512、512 rc2/stdout空/stderr43），父前次ASan同样通过；本任务最终仍failed/invalid round-end judgment3，三判reason字节=[503, 623, 737]超过319。工件限定边界已修，agent任务收口失败，不能混为自主成功。新反馈片直接对应可复現真实超长重试失败。

2026-10-08 真实判官修后源码有限对照 /tmp/csih-judge-source-contrast-parent.json：重构packet非原始wire，同一真实任务/声明/9工具记录，无补源一次accepted；追加父保存实际修后源码一次仍accepted且误说未发现越界，并把go写成stop:true。各一例不足统计证明，但直接反证“补最终源码即可解决本例”的期待；不可因源码补充自动宣称独立语义准确。实际ASan确有越界仍是权威反证。无生产修改/部署。

2026-10-08 反馈修复父独立候选四门禁通过、当前C/H/INC冻结输入一致：{"candidate_dir": "/private/var/folders/kv/yf8l6q994rl27550kd5985y40000gn/T/csih-feedback-final-parent-k1wpjbxe/candidate-26e87822c9a345738a2840cf5773ca52", "hash": "4fc8297a3356801a3b96bc900f73696873039251cdd262132502885c7637baec", "binary": "/private/var/folders/kv/yf8l6q994rl27550kd5985y40000gn/T/csih-feedback-final-parent-k1wpjbxe/candidate-26e87822c9a345738a2840cf5773ca52/candidate", "receipt": "/private/var/folders/kv/yf8l6q994rl27550kd5985y40000gn/T/csih-feedback-final-parent-k1wpjbxe/candidate-26e87822c9a345738a2840cf5773ca52/receipt.json"}。原14门禁和新增13反馈定向均实跑passed/frozen，真实CTTY恢复/三错终态仍在跑；未部署，不据候选四绿替代CTTY。

2026-10-08 反馈CTTY recover/exhaust父读收据passed且before==after，child0/baseline restored，recover started→ok一次，exhaust started→unverified明确reason473/1..319一次；完整原14+新13及候选四门禁就绪，批准逐窗热换版4fc829，仍非语义正确性证明。

2026-10-08 csih1反馈修复热换版父验通过 /tmp/csih-feedback-csih1-deploy-parent.json，4fc829 native48553→57958，launcher40541保持、完整交接、旧PID消失、角色/peer/空input+pending保持、窗口显示新身份。csih2顺序部署已启动。

2026-10-08 csih2反馈修复热换版父验通过 /tmp/csih-feedback-csih2-deploy-parent.json，4fc829 native48926→58548、launcher68404保持、完整交接、旧PID退出、watch/peer/空input+pending保持，窗口新身份。两窗已同版，下一步真实模型重新验收修复任务，不能用受控CTTY恢复替实际判官可靠性。

2026-10-08 新反馈真实任务收口复验启动，使用同已修工件，明确argv且指定完整unisacc命令，须实际七例和ASan511/512，禁止改源码/外信；仅流程验收不算自主修复，旧任务与judge长理由失败保留。回执/tmp/csih-feedback-real-closure-parent.json。

2026-10-08 新反馈真实收口复验通过 /tmp/csih-feedback-real-closure-parent.json：仅2真实exec，指定unisacc七例PASS+实际argv ASan511完整stdout/512rc2拒绝，source及固定header/check未变，真实judge一次accepted、reason=232字节、精确同集合、mail仅started→ok。未追加提示，新版不再复现本例reason超长循环。有限单例对比，已有修复工件验收不是自主修复、更非通用语义证明。

2026-10-08 新自主rank任务启动：严格argv整数语法/范围/32容量、先全验证不输出部分结果、升序去重；固定12例，不提示修法，完整根unisacc命令，要求真实红→修复→绿，父只造私有坏fixture/验收不改csih生产。回执/tmp/csih-real-rank-parent.json。

H1-3 实施：联结冲突定位保存本次声明标识符的 token 位置；普通名、逗号续声明、函数指针声明与块原型分别从既有名称捕获入口赋值，拒绝前回到该 token。接受路径不移动游标；不依赖头来源豁免。正式产品验收完成前不删 knownfail。

2026-10-08 rank真实自主任务父核验：/tmp/csih-real-rank-parent.json，0中间提示，初始真实红→写rank1197字节→同unisacc绿12，mail started→ok、judge397超长→新明确反馈→合法短理由接受，父独立unisacc重建+41额外输入对照（30固定种子随机/11语法极限）均=False，固定check哈希不变/source冻结。8工具动作，存在cwd混淆/read失败/无用ls/DSML分类额外文本拒绝，成功单例不能推断通用自主水平或硬隔离；父未改csih代码。

2026-10-08 rank父额外oracle更正：原41例中把“+”和“++1”误当合法，错误在父测试预期；程序实际正确拒绝rc2/stdout空。原两失败保存在parent_oracle_initial_failures，按明确语法改预期并独立重跑两例通过；最终41额外检查全部通过，前文False不删除。本任务可记有限自主成功，保效率/协议问题。

2026-10-08 下一产品切片cwd可理解性：rank真实8动作中复合cd只改shell子进程，后续relative file/read与build在harness旧cwd失败并浪费ls。父核agent_note_cd699仅纯cd持久AT.run_cwd，exec_run头当前cwd是启动目录；不推断复合shell最终cwd，不解析命令为自动持久、不声称沙箱。交专职agent一份小设计：明确本次启动cwd与下一工具cwd、相对file解析反馈及简短提示/已有API最小改动，先设计事实/回归，不改生产。

2026-10-08 用户csih2现场：过时报告仍说仅高度测试/红/未reload；明确用户要求联系csih-cdx却被watch固定peer0:csih1挡。父管理缺口：已验收新状态未同步。父向两窗发notice准确纠正，不需ACK；agent_watch_needs_mail无条件也令无通信授权普通问答未验证，列优先通信契约缺陷，不放开通用shell。

2026-10-08 主人纠偏：主要缺陷是工作流/上下文工程未做好，不应把刚才csih2问题重心归为通信规则；UI/UX乱但不急。父接受并撤下通信规则切片优先，保真实gate拒绝作为症状而非主因。下一只读诊断模型实际context的旧记录来源、manual/task/notice边界与fresh验收事实缺席，先做当前任务+可信新状态+失败后自主下一步的工作流修正，避免更长提示词/放权限替代。

2026-10-08 上下文根因源码核验补：agent_build_messages仅识别started-mail任务界标重置历史，普通user问答继续保留旧peer信封作为user；当前系统未分历史局部观察与全局最新证据。notice correction已持久done文件 /Users/wjc/.csih/managed-csih2-ayny_t1x/session/inbox/done/5d2dffd14b704812b5e501bb40502858.json，但tui通知入口只UI不模型，知识不能被使用。保零POST/零ACK通知行为，下一候选方案为当前状态+本会话notice证据引用索引独立层，按需file读且声明未验证/非指令，不重开ACK环。专职agent仍只读设计，未实现。

2026-10-08 父实际csih2问答前journal原生构包复核：{"root": "/private/var/folders/kv/yf8l6q994rl27550kd5985y40000gn/T/csih-context-preview-parent-2v1l4eru", "kind": "native context preview, not captured HTTP wire", "message_count": 34, "old_height_claim_present": true, "new_deployment_receipt_present": false, "context_sha256": "32b62986d62dae1c9b901b841112b4c4305d77fca84bed8ce401761d217e7abf", "build_rc": 0, "run_rc": 0, "first_failure": "parent helper used nonexistent agent_policy_configure; runtime127; corrected to actual agent_role_configure"}。原私有helper API误名运行127，准确修正后重编译/实跑0；未改生产/窗口。该构包没有新验收证据入口，旧height红存在。

2026-10-08 审阅接受/tmp/csih-context-workflow-diagnosis.md最小context-index切片：每轮行动独立current-task/actor真实身份/采集时点+本session done notice最多8准确引用（external/unreviewed/not instructions），旧历史范围/时间未知不升级当前verified；保notice零POST/零ACK/草稿、普通澄清历史/mail task隔离。暂不自动造global verified状态、不硬编码父/tmp收据为可信，具体事实须file读取核验；容量不足显式unknown/truncated不丢current任务；不改角色/通信、UI后排。先专职agent核真实API给实现约束/小范围实作与wire探针，父验收不改csih代码。

2026-10-08 上下文产品验收材料已准备 /tmp/csih-context-product-fixture-parent.json：实际旧39行journal冻结哈希，真实双窗换版/rank收据路径与内容哈希，验收首wire分层+实际file取证+结论范围+零自发通信/改代码。历史收据不冒充实时全局状态；watch旧必投递门禁可能导致只读答复非成功，作为独立已知问题不能隐藏或本片偷偷改权限。仅fixture未执行新上下文版。

2026-10-08 上下文控制另发现：csih2实际journal48/50用户输入/clear、/help；tui仅识别/reload-code/export-state/goal/loop/reload/exit/quit，无/clear或/help路由，因此被当模型普通任务，不能以模型答复确认“已清上下文”。应另切片机器处理显式context reset/new-task（保审计，写真实上下文界标、不删除日志），控制命令未知必须准确说明而非伪成功。当前context-index片不混实现，后续工作流必要项。

2026-10-08 父私有克隆mailbox原生只读API可行性已实证 /tmp/csih-context-io-proof-parent.json：冻结4fc代码，cmi_open/scan/cleanup编译运行0，实际消息严格decode；未触生产mailbox/窗口。用于收敛context-index复用API风险，非新索引实施证明。

D1-1（E70 后恢复）：静态初始化约束使用独立 d1_active，GV.bi/GV.iv 与 SC.init 保存旧值后设置，递归 INITLIST 继承，已有返回栈恢复。sizeof 的求值豁免与该标志分开保存，不能清除静态语境；位置诊断与错误恢复亦须保留/重置该语境。D1 尚未验收，不再按 0.0.36 顺延处理。

2026-10-08 context-index三路径已落盘，父只读源审/tmp/csih-context-first-source-review-parent.json（当时hash）；未核编译/实跑、未部署。反馈单调clock非SGT/跨机器新旧，ownedcwd为turn-start范围，JSONescaped容量实际失败无POST，以及旧历史分层需抓wire和真实产品取证；专职agent修正/冻源验证，父未改代码。

2026-10-08 /clear控制缺陷原生构包实证/tmp/csih-clear-context-proof-parent.json：冻结4fc builder读actual52-row clear/help后的journal，旧height失败仍在模型context、/clear仍普通user、虚构/help说明仍在；非HTTP抓包/非新index源码，不替代index片测试。证明需要机器上下文界标而非模型声称清空，审计必须保留。

D1-2 核心判据：求值调用与对象值读取经静态语境门禁；枚举和函数名地址不经对象取值门禁，数组衰变保留。sizeof 通用操作数保存 d1_unevaluated 后递增并在返回恢复，返回后的求值继续受限。字面量条件死臂另片实现，不把暂时误拒当最终边界。

2026-10-08 父新context-index原生API私有冻结验证 /tmp/csih-context-index-parent-review.json：完整15源编译0，index函数实际JSON可解析、actor/session/cwd/hash/clock_domain真fixture值、available且实际已done纠正notice ID可见；非HTTPwire/非真实模型读证据证明。原源码双manifest冻结，父仅私有helper，未改生产。

2026-10-08 context-index父私有原生额外4例通过：10notice取8排序mtime降序/id升序含tie、omitted/total准确；坏JSON/错session/链接均unavailable、unknown total=-1、不输出可信引用。/tmp/csih-context-index-parent-review.json保各实际packet。仅私有fixture顺序改变，未碰production inbox或测试中变树。

2026-10-08 context clock归因更正：父先前按clock.c注释认为真实monotonic不充分。实际新index/nativewire captured_ms≈17914549...与wall epoch一致，宿主Python monotonic≈3991487...；include/time.h21/50明确unisacc macOS两clock映射gettimeofday。不得标clock_domain=monotonic或声称不可wall是已证事实；需要packet准确声明requested API=clock_now_ms/CLOCK_MONOTONIC、effective domain平台依赖/不保证monotonic/不作为freshness。仅context字段修正，clock.c/编译器/include库不改，已跑旧标签收据保留，修后只重跑相关packet/必要当前候选。

2026-10-08 主人裁定：csih 当前主问题是工作流与上下文工程，通信规则不是本轮主修方向；UI/UX 乱但暂不急。验收应使用原始质问“搞这么久，你们做了什么”而非明确教模型读哪份证据：观察实际主动读取新证据、区分旧观察与当前事实、给出下一步而不把调度推回主人。17 CLI/11 scripted CTTY 仅证明结构与边界，不等同真实模型自主查证通过；当前 context-index 未部署。

2026-10-08 工作流验收准备：父私有 /tmp/csih-context-real-observation-parent.py 已语法检查，真实默认模型+原39行审计历史+私有notice索引+手动原始模糊质问，不给中途提示；watch原权限保持，主动读取证据和mail门禁终局分别评估。未运行不能算验收，未部署。

2026-10-08 真实上下文验收未过：/tmp/csih-context-real-observation-parent.json，候选 ee1c8bab4f64b2b25c9b740902d3a4f4bd413789efea4b288003f29385795615 四构建/门禁通过后，私有watch会话保留原39行旧历史，done notice列实际新三收据，手动只问“搞了这么久，你们做了什么？”，无中途提示。约48秒观察到两份assistant答复仍复述旧高度/selftest红/未reload，真实tool证据读取0；第一份被协议拒绝。未观察自然终局；父按私有实验时限SIGINT关闭，child记录KeyboardInterrupt，不能称终局通过或已恢复termios。源码候选复核一致，未部署。结论：仅首包数据分层不足以克服旧历史锚定，下一件需工作流/上下文策略而非通信权限。

2026-10-08 父已找到机械根因并核实真实scripted CTTY请求：/tmp/csih-context-retry-loss-parent.json 引用 wire-r2 requests0有当前task/index/history scope，requests1/2全部丢失。agent.c:1918-1919只首action无tail传当前层；协议重试后回到旧history原角色。下一片必须所有actor请求保持当前层并置于历史之后以明确当前任务，judge仍独立。此为脚本真实wire，不冒称真实模型HTTP抓包；真实模型旧结论失败另见real-observation回执。

2026-10-08 第二片实现决策：审查 /tmp/csih-context-workflow-r2-design.md 后授权专职代理修agent actor请求稳定工作集（任务/身份/初始证据索引每步保留、历史之后尾锚点、反馈最后）、容量显式预留；同步纠正“默认answer”和“这句话唯一任务”矛盾规则。独立judge/通信权限/工具预算保持；过去观察分层要有可证明当前turn界限，不用中文猜新任务，audit不能删。先真实wire及原模糊质问验收，不以prompt改动本身称完成。

2026-10-08 稳定锚点真实r2未改善：/tmp/csih-context-real-observation-r2-parent.json，候选7bdbd857ea1b8689ace7bc23a0b91f9d09424f5a5fd83c11e2497d951c98297c四门禁绿，原39行/同notice/同模糊质问/零中途提示，约48秒两答均旧结论，新收据读取0；第一为exec+answer多对象整体拒绝，未自然终局，限时结束与host恢复事件/私有native退出已核。每步anchor修复机械缺陷但未证明自主行为改善，未部署。下一决策：专职代理在当前冻结测试结束后，用真实turn_first_record将之前的原user/assistant（及旧工具）降为历史观察数据，保role/顺序/文本及未知时间覆盖，不删除audit、不编造总结、不把旧授权自动转为当前授权；当前turn真实tools/反馈保持原序列。只改context装配必要规则/probe，不扩通信/UI/预算/schema。

2026-10-08 父进一步核权限上下文矛盾：agent_peer_blocked明确ACT_READ允许、ACT_MIND read允许，但agent_role_line watch文本宣称“权限范围仅tmux capture-pane...以及envelope”，遗漏已允许的file/mind只读，真实r1/r2答复也声称只剩这两条；write role另有“默认单独answer”覆盖新主策略。这是系统上下文描述不忠于实际能力，修文案并非放宽通信/工具门禁。授权专职代理历史分区同片将角色描述对齐既有门禁：watch已有file/mind read明确可用，exec限制原样，write默认answer改为按当前任务查证/执行后汇报。不改agent_peer_blocked/agent_watch_exec_ok/agent_watch_needs_mail。

2026-10-08 历史分区真实r3仍未改善：/tmp/csih-context-real-observation-r3-parent.json，候选16e2f18c四门禁绿，同39行/notice/质问。新收据读取0，继续旧高度/selftest红/未reload；合法answer后watch无条件投递nudge要求“先给同伴发envelope”，实际exec向真实csih1误投旧总结434 chars。父私有实验未隔离peer传输是管理疏漏，实验已结束、native48488已不存在；这不是自主查证通过。授权立即发送notice纠正而不派任务，后续真实模型实验必须私有投递隔离，原失败记录保存。工作流末决策不能为了验收诱导当前未授权通信，先设计其与任务约束一致的失败/停止语义，不靠放宽路由解决。

2026-10-08 私有真实模型验收传输隔离决策：以后helper创建独立tmux socket/session0/windowcsih1（仅cat，不跑agent），子会话显式TMUX/TMUX_PANE指向私有服务；先验证默认tmux只列私有窗才启动模型。结束仅关闭该socket自己的服务；不动主tmux。原误投纠正notice已消费done，当前capture无可见正文不能声称对方模型已理解。

2026-10-08 审查task-workflow-contract-design后实施决策：先将watch权限与完成条件分离，普通任务不自动生成投递义务/先发信nudge，仍按原任务/实际证据/独立判官验收；显式机器投递条件若设置须当前turn绑定，真实回执识别与路由门禁不变，失败/partial不附无关未投递归因。随后给索引加最多256字节UTF8安全未验证notice预览/总字节数/截断标志，使模型能判断适用性，不把线索当验证/授权。两片分别冻结测试，未验证前不部署。私有传输初测因sender==receiver拒绝（正确隔离且服务已清），需独立sender窗后再证明发送仅落私有peer。

2026-10-08 独立私有传输已实测通过：/tmp/csih-private-transport-r3-proof-parent.json，独立socket/独立sender与peer，真实bin/envelope rc0且正文只见私有pane，kill-server仅该socket rc0。前两失败保留（同窗拒发；new-window数字target歧义），helper已用session0:、sleep60的有限存活窗与独立sender修正；后续模型验收依赖该隔离，不触主窗口。

2026-10-08 父核任务条件源码修复：/tmp/csih-task-delivery-source-parent.json 对照历史候选门禁/receipt detector未改，无条件先发信nudge已移除；显式next-turn投递义务consume，普通watch完成走独立judge、partial/failed保正文。首nativeprobe运行有旧原因断言和echo shell成功≠receipt成功断言误配，/tmp/csih-task-delivery-contract-review.json 保留；实际stdout已显示普通咨询接受/partial与failed分别失败/假receipt拒绝，但纠正探针后完整通过前不声称验收。

2026-10-08 任务完成条件片已验收冻结：专职 /tmp/csih-task-delivery-implementation-review.json，native build/selftest和8 localhost病例通过，普通watch咨询2POST接受/partial failed1POST保正文，显式未确认/fakeecho/wrongpeer拒；timeout仅结构单元，不冒称真实网络。下一片授权context_index.inc通知预览（<=256 UTF8字节、实际原body长度与truncated、external/unreviewed），排序/验证/容量/零自动POST ACK规则保持。未部署，真实模型效果留待隔离r4。

2026-10-08 真实r4结果：/tmp/csih-context-real-observation-r4-parent.json，preview候选1e3f88a8四门禁绿，传输隔离服务清理rc0，同39行/notice/质问，新证据读取0。实际UI已显示answer outcome=partial终态且未发信；helper只检go/acceptance漏检partial（原false保留并加更正）。答复虽说范围未经验证仍沿用旧全貌，主动查证未过。下一动作先抓真实模型请求正文（仅body、绝不记录Authorization/key），确认index/preview/status实际送达；不靠脚本wire推定真实实验wire，也不继续盲加提示。

2026-10-08 真实wire r5传输失败保留：/tmp/csih-context-real-wire-r5-parent.json，relay实际request0，native HTTP503，无模型答复，不能算任何模型上下文或行为验收。与既有localhost probe不同，父helper子进程未清代理环境；下一次仅child→localhost清proxy/NO_PROXY，父relay→真实API保持原网络环境，不改生产，不存密钥。

2026-10-08 重要父归因更正：真实wire-r6 request1证明status unavailable: lock kind/owner/permissions invalid，私有helper手工创建inbox但漏mailbox.lock，故r1-r4不证明模型忽略已提供的新索引/预览（该归因撤回，原失败记录保留）。r1/r2机械丢索引、权限提示不一致、r3无条件mail nudge仍有独立代码/wire证据有效；不能用坏fixture证明其自主效果。r6 backend401因net.c仅api.deepseek.com URL发送key，本地relay缺认证；父relay改为从已有env.jsonl读取deepseek api_key到内存，仅真实API请求使用，不保存/打印headers/key。两helper补0600锁文件，后续真实wire须先确认available/newpreview。

2026-10-08 首次有效真实wire/模型验收r7：/tmp/csih-context-real-wire-r7-parent.json，5实际POST，3actor包均available/notice1/256-byte预览真实，key/header未存。实际自主先mind读tree+palace（当前全局两页中有旧强制先读两页/旧通信方向/旧任务记录），新notice/收据read0；随后旧结论completed，独立judge因仅记忆证据拒绝（初332字节reason协议拒，第二短reason拒）。无外信，隔离server cleanup0。这次有效证明模型看到preview仍未核新证据，并发现全局记忆旧指令污染；未部署。下一重点是记忆来源/范围/有效约束与当前观察分离，不擅改全局记忆文件、不继续盲加prompt。

2026-10-08 记忆对照实验决策：保持候选1e3f88a8、原39行history、同notice证据、同模糊质问/真实relay及私有投递；仅child HOME指向私有新目录，自动seed最小页，不改共享记忆。真实模型配置/available index须在请求体核对，HOME变更会影响其他本机配置，因此该对照不是严格证明唯一因果，也不算生产修复。

2026-10-08 记忆控制r8已实际终态：/tmp/csih-context-memory-control-r8-parent.json，同model deepseek-chat与available notice预览，私有空HOME/最小页，无mind调用，也无新notice/收据read；answer看到纠正notice并承认不是当前全貌，却仍先给旧结论，再说读取反馈需主人“说一声”，独立judge unverified。说明旧全局memory不是唯一原因，包装memory不可当充分修复。下一先做当前状态问题（不显式教read）控制以区分质问历史歧义与自主查证决策，再设计有限证据冲突查证/已有只读权限自主使用；不盲改共享memory。

2026-10-08 当前状态控制r9已终态：/tmp/csih-context-current-status-r9-parent.json，原问题换“现在csih有哪些已交付进展，哪些还没完成？”（未教read），私有HOME/same candidate，actual POST1、available notice preview，tool0；answer partial且承认新通知有更新证据，却说“需要读那些反馈或由父派单才能核实”。问题已超原质问时间歧义，缺的是已有权限范围内主动核证的决策；未知标记有改善但未完成用户所需状态核实。原partial终态可诚实停止，不应为此重新强迫通信或无限续问。

2026-10-08 查证决策贯通片实施授权（审查51行设计）：actor与独立review共享本turn固定index/实际只读能力/用户禁工具状态，候选仅记录当前真实同路径成功read调用，不等同完整读取、内容verified或链接收据核实（精确执行路径不证明时unknown）。独立continue可带1..319 UTF8 reason，审计先于效果，尾反馈传actor，不增预算。对partial且有available未使用候选、非禁工具的turn最多一次停止理由/行动选择review，可保partial停止或已有权限内最小核查；不能accept partial为completed，不审查显式stop、真实failed/禁工具/无候选默认终态。completed维持独立验收，真实r9路径须覆盖，不能只修completed。

### 2026-10-08 上下文工作流独立验收
主人再次明确工作流/上下文优先、UI延期。实现代理已冻结一次partial审查与共享查证工作集；父会话先独立冻结构建，再以隔离真实模型检验，未部署。脚本指定的读取不算自主查证，旧失败记录保留。

### 2026-10-08 真实模型 r10：读取进步，任务闭环仍失败
父独立冻结候选 a4eab967ed740d16085333948c554479988aaf487427da61cf2b007d0dbda7c4，两编译/两自测通过。/tmp/csih-reconciliation-real-r10-parent.json：真实7 POST、约31.8秒，无中途指示，主动6次file读取（两个部署收据、notice、rank前段）。但最后partial仍重复历史selftest红/未reload，并询问是否继续；rank只读取前231/1188行，不足完整结论。notice已读到一行使候选content_lines_seen成立，partial审查未触发；不能把call/content_seen等同问题证据已充分。故只是主动读取有所改善，仍未通过当前状态汇总验收，未部署。测试清理子进程KeyboardInterrupt为实验终止，不宣称程序自然退出。

### 2026-10-08 独立wire审查发现新read_call接线回归
core-r2/protocol-r2实际passed=false，旧协议regression=true。父读core recover第四包：file工具仅status unknown，无harness action_id。根因agent_tool_status_wire严格root仅4/8键，新read_call根9键被拒；actor看不到合法ID，judge仍看原ledger真ID，非仅fixture提取错误。已派实现代理支持严格9键read_call并回归旧schema与实际actor wire；生产集合门禁不放宽。r10主动读取仍真实，但该候选另有ID接线回归，不可部署。

现场复核：tmux %400/%401 仍原4fc829运行PID57958/58548，尚未部署当前上下文候选。capture显示csih1 rank完成后旧总结询问信息与unknown/old evidence失败；csih2仍旧总结、强制peer mail失败以及/clear、/help文本回合。只读核对，未输入、未重启；这些旧显示不代表新的专项结果。

9键修复审查补充：新读取audit应以实际ACT_READ写action.op=read，严格9键才可区别旧file/write的空op；旧8键兼容。call_success与status.op_success保持一致，status.handled为真；content_lines_seen不等同完整或verified。父已交实现代理同件收口，不做新工具或权限扩张。

私有真实模型验收器修正计划：原终结判断匹配原文go/acceptance，可能把被protocol_rejected的判官答复当结束；新副本只接受已提交decision go=stop，或本轮TUI最终上一轮错误横条。保留原helper与r10，不改生产。观察到期仍标实验截断，不当自然终结。

9键接线修复实现代理确认C已冻结，父开始独立候选构建。原r2失败与root-schema证据保留；不提前宣称专项通过。

真实r11 /tmp/csih-reconciliation-real-r11-parent.json：11POST，10次实际file读取，每一actor包currentID/read_call数0→10一致，接线修复真实成立。模型自主沿rank收据窗口读至943行，48秒观察到期仍在等待第11次真实模型；实验终止，不是任务失败或完成。需要私有后台验收服务分段观察同一PID，每HTTP≤35秒、每观察/租约≤55秒，到期明确实验中断，不能重复重启来假装继续；后续先实现私有测试租约监视，不改csih生产。

r12后台私有租约实验实际终结于HTTP502 relay transport failed（约11.8秒，3POST，先多对象拒绝后单file读取成功）。是外部传输失败，不评价任务查证能力；同worker/私有tmux已清理。允许新r13独立重试同固定候选，不是因观察超时重启活任务。28项唯一native/localhost专项实际通过，父候选与源冻结绑定；仍未部署。

真实r13已自然回合终结：/tmp/csih-reconciliation-real-r13-parent.json，约53.9秒/10POST，后台租约在同worker上续观一次，未给提示、私有隔离清理rc0。模型自主读取notice/两部署记录/rank尾窗口；长answer首次格式截断被拒后partial重试。最终仍把部署记录内恢复input历史中的selftest红/未reload混入当前未完成，并错误声称看客不能file读尽管本轮真实file读已执行。未触发partial review，notice已有content_seen使pending0。这证明接线恢复且能持续查证，但来源层级、有效权限、停止决策仍未通过真实验收；不部署。下一件应先设计partial停止审查不能被read_seen关闭、judge区分记录本身与其内历史引用，不再仅增加actor提示。

### 2026-10-08 partial停止审查设计通过
父读回 /tmp/csih-partial-stop-context-design.md 36行，授权实现唯一贯通片：partial一次review资格按available候选存在，不按content_seen未消费；共享原预算，保原正文，failed/explicitstop/no_tools/无候选不新开。actor/judge共享本轮native能力快照和外部记录/其内历史载荷的来源边界，不靠模型自述权限、不按关键词分类，顶层passed不自动verified；不改UI/通信/memory。原r13失败与源码新ID接线绿保留，真实模型仍须复测。

下一真实状态查询验收口径：允许诚实有限partial/未知，不要求模型虚构全局完备或强制completed。必须区分收据自身事件与恢复input历史、识别当前真实read权限、不给过时selftest红/未reload当当前结论；有相关可读缺口可自主处理，不把普通只读核对再推给主人授权。一次review停或继续均可，具体结果由真实过程判断；有限查询通过不等整个harness目标完成。

预算表述更正：当前MAX_JUDGE=3限制at_judge_retry的非法判断恢复（计数包含先前judge），不是所有合法judge POST总数硬限3；合法continue仍受MAX_ROUNDS8/MAX_MODEL_REPLIES128。新partial单次review沿用现计数不重置，不悄悄新增全局3POST改变旧continue契约；设计/回执须据实际代码表述。

partial停止片C实现代理确认已冻结；父独立冻结构建，不复用上一候选绿。专项正在准确handle75496运行，本轮尚未宣称通过或部署。

真实r14 /tmp/csih-partial-stop-real-r14-parent.json：14POST约51.1秒、同worker续观、无提示，已读notice后partial审查实际触发；shared原生capabilities/current ID可见。actor正确找到rank12PASS，但仍把旧恢复input selftest红/未reload当当前。独立judge停止unverified，理由“未重跑check/未核全貌”没有纠正可见旧状态矛盾。机械停止审查修复成立，语义工作流仍未通过；不部署。下一定位：partial action-choice规则只允许有用read而未明确已有证据下的答复修正，判官验收思路偏重是否重跑测试而非当前任务本身的事实汇总；应先只读设计审查，不堆继续提示或强制accepted。

### 2026-10-08 r14审查改进设计通过
父读 /tmp/csih-partial-review-r14-design.md：已读后review已生效，不再归因机械触发。授权最小两处语义契约改进：一次review可用已有当前证据修正答复、限定时域/撤回矛盾，不仅限继续file读；尾反馈改任务核对而非Read-choice。judge按原任务所需证据评价（状态汇总不默认重跑编译），嵌套历史不能覆盖该记录事件/native权限。不增加review/预算、不自动partial accepted、不改UI/通信/memory。只是一项具体可测偏置修复，真实语义仍需验收，不承诺确定性事实验证。

任务核对片实现代理确认C冻结；新增read-correct真file→一次review→无新工具更正partial。父独立构建后同原状态查询fixture实测，不把纠正文案脚本当语义能力证明；原budget/partial非成功定义未改。

真实r15 /tmp/csih-task-reconciliation-real-r15-parent.json：14POST约55.3秒，自主10次file读，actor改称completed且仍旧selftest红/未reload当前化。judge前两次accepted但被reason372字节/answer_only有工具记录硬门禁拒，末次rejected reason410又超长，最终protocol invalid非成功。不能称修正语义通过；旧规则保护未假绿。停止继续小段prompt打补丁，先做上下文消融：同固定候选/实际notice/三源收据/当前问题/默认模型/共享HOME，只移除私有实验的39条旧journal，验证自动旧历史是否主因；不删除真实日志、不缩问题、不把控制组成功当已部署修复。

上下文消融r16 /tmp/csih-context-history-control-r16-parent.json：12POST约36.7秒；首包无past observation，私有journal确为空（初始hash为空SHA），仍读共享mind旧任务、capture私有空peer后转述旧红/未reload，独立review停unverified。故仅去自动journal不足；不能推断唯一根因。原helper scope文案仍误写old actual journal retained/original vague，以实际seed空hash/当前prompt/wire为准，保留原记录并在此更正；后续control helper修准确scope。下一r17同时空journal+私有HOME基线页，模型/候选/实际notice/三证据/当前问题一致；HOME可能影响其他配置，实测wire模型核对，不宣称严格单变量记忆因果。共享HOME/真实journal不修改。

并行只读下一设计范围：已有用户实际/clear被送模型且未清上下文的独立证据 /tmp/csih-clear-context-proof-parent.json，不依赖r17因果结论。安排代理设计native上下文重置/任务边界，旧审计保留、现role/peer/cwd保留、忙时明确不清或空闲边界执行；上下文epoch须跨hotreload恢复且不把恢复state input/history当新授权。只设计不改C，不删除共享memory、不先扩大state schema；比较复用journal typed boundary与私有sidecar持久化，选最小可证实现，说明trim/恢复/失败行为。

context reset设计复用源证：agent_task_boundary严格识别真实mail started/accepted顶层三键，agent_messages逐持久边界释放此前held roots，agent_journal_trim遇边界保全文；可研究独立真实native clear记录复用边界扫描/trim，不伪造mail lifecycle。普通follow-up现不创建task boundary，保持此契约。

完整私有fresh control r17 /tmp/csih-context-fresh-control-r17-parent.json：8POST约28.2秒，空journal+私有HOME、wire仍deepseek-chat、只file读新notice/两部署/rank前段，仍将收据内旧input红/未reload当当前。故journal/shared memory均非唯一原因；来源与工具信息实际呈现须继续查。父定位两个可测file契约疑点：agent_file fgets 2048每片段total++可能不是真实行；agent_tool_record无条件head/tail压至1600（包括当前turn）可能省略部署中段activated/committed却保旧input尾段和原窗口footer。先私有native POC验证，不改生产，不据source猜测当已证实。native clear设计已收到但未授权实现，避免一口气扩方向。

### 2026-10-08 file基础契约原生复现
/tmp/csih-file-contract-poc-parent.json 用冻结ea851候选源/编译器独立native main实际调用agent_exec与agent_tool_record：5000字节首行＋SECOND/THIRD共3物理行，line2 n1却读到首行A片段，long_line_contract_matches=false；300行fixture当前raw含中段MANIFEST_ACTIVATED_CURRENT，但写当前tool record后中段消失、旧tail保留。实际r15/r17部署tool rows均无activated/committed且含中间略/旧尾历史，footer仍称第1-97/1-103行。rank收据实际1180物理行（4条>2047），native声称1188。故此前judge从完整原部署事件读回判断的表述过强：它只见打包片段，不能说完整生命周期已机械可见。已看passed/新PID是真的，语义仍误用旧状态也是真的；不认定单一根因。下一优先只读设计file真实行/连续窗口/长行续读与raw audit vs模型呈现契约，不再继续prompt微补。当前source仍冻结，不部署；/clear设计保留待基础读取修正后实施。

file观察设计A原则通过，1024固定窗口未直接授权：它虽连续，但会增加真实文件读取次数并耗16action预算，不能仅为易装入旧1600压缩器而缩小能力。要求先预算化选择默认4096 UTF8字节、可选max_bytes≤8192或同等可证有界设计；read审计JSON最坏转义需按实际最大容量分配/失败明确，不能fallback静默缩正文。遵守actor/judge容量但不强塞全部大raw，完整实际窗口元数据同步校验，游标保证长行可还原。新设计无需额外工具类。

### 2026-10-08 工作流优先：file观察契约实施授权
主人明确问题是工作流与上下文工程，通讯/UI延期。父审查47行file观察设计，授权代理完整实施路线A：真实LF物理行、默认4096/max8192连续UTF8窗口、准确续读游标、当前read审计与actor/judge不再头尾打包；严格容量失败保审计、不空body请求。增加实施硬边界：原始CRLF字节不归一化；max_bytes装不下首个完整字符须明确无进展失败，不能成功返回同游标；显式offset落字符中间须拒；非普通文件拒绝且打开不能阻塞FIFO，扫描须有明确有界成本/失败，不能无限扫大文件。使用真实已有API，平台数值上界先检查后cast。同步9键schema/共享step/实际actor与judge wire，防再次ID丢失。仅代理改生产，父独立冻结验收；未部署、未宣称语义问题已解决。/clear设计继续排队，不扩大本片方向。

产品skill学习落地：已读 product-design-and-ux/SKILL.md 及 AI interaction/task flow references。借鉴三条验收约束：从用户任务到可观察结果而非发送/工具rc结束；区分来源时域、缺失/冲突与实际权限；部分成功后保留工作并有明确修正/恢复路径。当前file片只修事实观察基础，不以机械绿代替真实状态汇总正确。后续任务边界/reset继续按审计保留和恢复证据验收，UI仍延期。

file观察父验收准备已落盘：/tmp/csih-file-observation-parent-fixtures.json，6个独立原始字节fixture，含5000长行、UTF8/CRLF/无尾LF、非法UTF8/NUL、高转义容量、中段部署事实；逐文件SHA与物理行数固定。尚未执行新实现，不算通过。代理当前实际running并确认普通文件NONBLOCK打开后fstat、扫描8MiB明确边界，正在写贯通实现，未冻结/未测试。父不重复派单、不修改生产树；下一步等待完整冻结交付后独立测试，扫描边界作为能力限制如实记录，不能称任意大文件全覆盖。

父现场复核（只读）：tmux %400/%401仍原launcher40541/68404、native57958/58548，ps实际存活，runtime4fc829；显示旧失败/旧总结不代表新candidate结果，未注入命令/未重启。读取实现代理实际running，当前树尚未出现新窗口字段，不能假报落盘。/tmp/csih-context-reset-design.md 已读39行，typed journal reset路线与保审计/原权限/忙时拒/零POST原则可用，待file片冻结验收后实施；不同时扩新scope。当前主线结果门禁：原始观察可靠→当前任务证据核对→原权限内行动→独立结果验收→失败可修正/恢复，不能以回执发送、模型自述或工具rc当总体完成。

file实施中途只读审查发现偏离：当前agent_read_window malloc整个≤8MiB文件、读取/验证全文且大文件直接拒，byte_offset收窄int/8MiB；这不是已批准A连续窗口（设计明确不需全文扫描/长行不整体累积），是B整文件快照的简化。父不改代码，立即要求代理在同件恢复流式有界窗口：scan预算约束实际定位成本，不把文件总大小当能力上限；不要因窗口之外坏UTF8/NUL拒绝本次合法窗口；offset平台精确上界先判后cast。当前未冻结/未测试，不当交付或复用绿。

父独立原生验收入口 /tmp/csih-file-observation-parent-main.c 已准备：经真实agent_parse→agent_exec，stdout按strlen精确输出不增加换行，用于与原始连续字节比对，parse拒返回3。仅私有helper，未编译/运行共享半成品，不算通过。正式9键审计/actor/judge链仍需独立protocol probe，不能以此raw入口替代。

流式体已实际落盘：read_stream4096、offset long+lseek、物理line定位8MiB成本上限，不再整体malloc/拒大文件。中途只读仍发现窗口边界先检查下一字节continuation再检查used==limit，可能把窗外非法续字节误当本窗失败；metadata未见fstat观测size/mtime与起始行完整标识。父交同件收口，尚未冻结/验收。offset直seek不扫描前缀时行号null是诚实未知，不能称准确物理行已全满足；必须记录设计变更与代价。

/tmp/csih-read-minimal-review.json已实际核：build rc0/selftest rc0/consultation passed=true/frozen=true，scripted protocol非真实语义。父尝试复制其绑定source以独立native复测，目录已被探针清理(FileNotFoundError)，编译与case均未启动；不能声称父复验通过。已要求最终冻结交付保留snapshot或由父在停改后重freeze源码；当前最小绿不覆盖后来边界修复。

父独立原生raw验收取得实际11/11：复制主树前/后/副本全hash相同后仅私有immutable snapshot测试，agent SHA a7a3fd001eedf32e02b48c1f3d34e8155e0e0d07f10c8e94259478b5765df5bc；/tmp/csih-read-parent-stage-build.json build rc0，/tmp/csih-read-parent-stage-raw-review.json passed=true。真parse/exec覆盖5000首行line2=SECOND、9MiB有效首行窗外坏UTF8不拒、无writer FIFO即时拒、UTF8小cap无进展拒/中字符offset拒、NUL/坏UTF8拒、小数与双起点parse拒、长行与中文CRLF原字节游标重组。是中间快照raw路径验收，不替代最终candidate或审计/actor/judge/容量与真实语义验收，未部署。

父独立运行永久file_observation探针终态PASS：/tmp/csih-file-observation-independent-parent-review.json，outer55/逐compile14/run3，build/selftest rc0，before/after frozen=true、agent a7a3fd00…同父raw快照。11raw用例；actor完整中段窗口/currentID/status/read_call；judge真实packet中段marker/当前ID/完整presentation；两8192控制字节窗口完整审计后容量失败rc1、只2POST、无empty body后续POST，旧证据不静默压缩。原full-review失败保留，原因probe跨独立turn复用旧ID被生产正确拒；r2改probe取当前ledger后通过，不放宽生产。此父验收为机械协议，真实默认模型汇总仍未证明，未部署。

父进入file观察候选构建：当前agent SHA仍a7a3fd00…与独立专项一致，构建冻结源码/编译器，两compile/two selftest；只私有candidate，不热更生产。随后先实际部署收据连续窗口重放，再默认deepseek-chat原当前状态fixture零中途指示验收，机械绿不当语义通过。

file观察最终交付代理已冻结a7a3fd00…；父候选 /tmp/csih-file-observation-candidate-parent.json build rc0，hash971f4c008750bb4645be9865fe6a29b21ed057f0f20863f9f1ce6f23c6cf46d3，两compile/two selftest。实际部署收据native连续重放 /tmp/csih-file-observation-deploy-replay-parent.json：csih1 11573字节3窗、csih2 5814字节2窗，逐字节一致；首窗activated/committed均实际可见，不再省中段保旧tail。下一真实r18使用同当前状态问题、默认deepseek-chat、39旧journal/sharedHOME/实际notice，private peer/relay租约，同worker续观不重启。仅实验、不部署。

真实r18已终态 /tmp/csih-file-observation-real-r18-parent.json：13POST/约44.27秒、自主12次file读取；当前完整窗口累积超过64K，最终TUI unfinished current task/context-index exceeds capacity，无最终answer/独立judge，非超时/非语义通过。完整file机械修复有效却暴露全量当前ledger堆入model上下文的工作集设计缺陷；不能用容量硬失败作为产品完成。rank多次按next_line重读含截断长行导致重叠，未使用byte cursor，需评估模型呈现/工作集原生策略，不能仅强迫prompt或增预算。private server cleanup0/candidate verifiedtrue，child KeyboardInterrupt是父实验宿主清理，不是native自然退出成功。下一只读设计有限任务工作集与完整durable audit分离：保持当前task/失败义务/真实IDs/范围与证据可追索，不用头尾省略却伪称完整；judge依据claim所需记录+原始可追索审计，不自动accepted，不靠模型单方删除失败。禁止把64K直接扩大或恢复旧1600pack当充分解决；未部署。

父量化真实r18工作集 /tmp/csih-r18-working-set-cost-parent.json：12read正文39680字节（含2555重复文件字节），JSON工具record合计58022字节，rank只读至19328/58998。重复不是唯一成本，更大是metadata/assistant/anchor和转义后整集累积；compact JSON是估算非原encoded-byte测量，不当token数。此数据供设计选型，不由重复比例猜语义充分性。

父补核r18实际工具而非仅原生重放：首部署read当前ID66153-19747-1-1，body activated/committed/passed均字面可见，范围byte0..4094；/tmp/csih-r18-visible-deploy-parent.json。故本轮容量失败前已机械提供部署事实，不能再归因该中段被新read丢失；仍无最终answer，不推断模型已正确理解。

工作集设计B已读（/tmp/csih-task-working-set-design.md）：完整audit+机械目录+有限连续原文+本turn ID回取方向可取，尚未授权C。未解决接受漏洞：只送claim引用/显式op失败原文，会漏未引用且op_success=true的语义反证（rc0正文含测试失败），目录不证明全文被judge核对。要求设计补齐收尾有界原文分批覆盖状态，native绑定当前turn/ledger身份/byte范围并记录所有原正文评估覆盖；accepted不能在未覆盖反证风险时凭模型自述相关性跳过。保既有预算，覆盖不足partial/unverified，不能单纯新增硬失败当产品完成；只设计，先比较主动judge取窗与原生顺序批送的成本/实现边界。

分批审查预算源码核对：at_after_http HTTP_END每次AT.judge++；合法go continue目前必at_continue_claim并round++、action0、回actor，不是内部下一judge页；at_judge_retry按包含合法judge次数的AT.judge>=3拒非法回复，MAX_ROUNDS8在turn_step共闸门。新设计必须明确内部page协议/phase转换与重试计数，不把旧go continue直接当页翻动而触发actor宏观循环；不得重置既有累计预算或偷偷把合法页变无限回合。尚设计，不改实现。

工作集设计补交84行审查通过，授权完整贯通B：durable完整audit/机械目录/actor有界连续原文/本turn evidence回取/native顺序judge分页/最终覆盖硬门禁。native绑定turn/ledger_version/claim_version/page范围，不复用go continue翻页；每页reply与AT.judge累计不重置、不计round/action，现judge>=3后非法page非成功；正常continue才回actor。全部工具正文含成功未引用记录必须coverage，typed事实至少提供一次；真实concern及页观察append-only保留，coverage非语义证明。实施前在同设计文件补具体严格page JSON键/类型/长度/duplicates/NUL规则，避免即兴API。页响应非go且不能accepted、伪page不推进；最终原exact evidence/partial规则不变。预算不足诚实unknown但真实r18约40K正文须以完整闭环复验，不接受仅actor省容量就宣称交付。只代理改生产，父独立验收；UI/通信/clear/编译器仍不动。

工作集父独立fixture /tmp/csih-working-set-parent-fixtures.json：12份r18实际观察正文逐字节保存，原status/范围/currentID仅作出处不复用新turnID，合计39680字节；额外rc0含失败测试正文反例。用于容量/全集分页覆盖/反证保留机械验收，不把动态脚本page no_concern当真实语义判断。代理仍running，未见实施字段，未启动半成品测试。

父准备独立page字节覆盖验证器 /tmp/csih-page-coverage-parent.py：actual页范围必须对应不可变原body、合法UTF8边界、当前已知ID，全集无缺口，空body须实际呈现。helper自检正/负例通过，仅证明验收器可拒缺尾/伪正文，未测试native实现；后续须从真实请求页提取，不用模型回执伪造coverage。

工作集实施真实落盘进度：agent.c已出现evidence_id严格read参数、AT claim_version/page游标状态、机械ledger目录和本turn ID原文回取helper（源码约142253字节）；尚无分页HTTP处理/专项收据，不称完整闭环或冻结。父提前要求新evidence_source typed根字段若改变9键须同步严格status/read_call/schema，防目录整体失败和actor ID unknown重现；不直接改生产。不再把代理只有设计当现态，当前已是部分实现。

工作集实现中途只读审查：agent_messages超预算分支agent_ctx_pick(nvals...)后设history_n=0，但随后for(k=history_n;k<nvals;k++)use[k]=1会重新选全部record，覆盖预算选择；是源码确定的选择器逻辑缺陷，尚未native复现（未冻结）。新增evidence_source嵌action保持root8，但agent_observed_read严格验证还缺source/text对象类型、数值整数/上界、meaning与action.op绑定，畸形历史可能被当完整展示。父要求代理同件纠正并专项覆盖，不替写C，不称当前工作集已有效。

分页处理中途源码审查新增两个实施缺陷/偏离：HTTP_PAGE合法/非法分支均写if(!at_audit_assistant(...)) at_fail，而既有API成功0，导致合法页也审计后失败；尚未native测试。分页schema现单observation319字符串，未实现批准的结构化ID/range concern及最终原文反证窗，不能只用页摘要替代原文反证。分页预算while空间不足可在已有片段后goto fail而不封页，应区分已有页封页与单首字符不容失败。父交代理同件收口，仍不冻结/不部署。

工作集最小回归 /tmp/csih-working-minimal-review.json 盘面核实：passed=true/frozen=true，build/selftest/consultation rc0；仅普通无工具路径，不覆盖分页审计/structured concern/全集coverage。选择器picked_all守卫已实际落盘，避免旧尾循环选回全体；HTTP_PAGE审计反条件与单短observation仍未修，故不能称分页通过。父继续同件管理，不重复跑该最小路径或部署。

分页收口盘面已核：HTTP_PAGE审计按!=0失败纠正，observations严格数组≤4（每项id/start/end/note），原文范围绑定本页且UTF8安全，append-only concerns保原字节并送最终包；满页page_ready封页已加入。/tmp/csih-working-minimal-r3-review.json build/selftest/consultation rc0、frozen=true绑定240a8994…，仅最小路径，不覆盖上述分页运行。父未重复最小测试，等待完整原生page主链实际收据，仍未部署。

完整分页首次真实专项已终态失败 /tmp/csih-working-pages-review.json：passed=false/frozen=true，native rc=-11 SIGSEGV，不是容量诚实失败或模型语义结果；保留原收据/私有snapshot。最小accepted-work绿未覆盖长主链。父已核真实崩溃并交实现代理根因定位，禁止根据机械设计宣布闭环绿/部署；只修本次回归，查清后绑定新hash重跑。

崩溃定位父补证：failed-pages responses仅普通path读，offset4096/8192、max4096；尚未evidence_id/页审，不能直接归因新页parser或回取函数。已转代理以普通第二read与actor包内存链为起点。生产ps57958/58548当前存活、仍4fc829原PID/launcher，崩溃仅私有专项，不是生产窗口被本轮部署损坏；未重启窗口。

SIGSEGV根因实际证据已对齐：私有插桩 /tmp/csih-working-segv-diagnostic-review.json 显示第二目录status后崩；系统cc /tmp/csih-working-segv-asan-diagnostic-review.json 明确新增json_rec(id,cap,"id",id,NULL,NULL)仅6参，真实固定声明8参，另ffi header使ASan构建失败，未ASan运行绿。代理仅补末两NULL，正在r2同原生主链重跑handle37529；不归因编译器/模型。原崩溃、插桩、构建失败收据都保留，当前修复需终态复验。

父独立核对r2真实requests分页覆盖 /tmp/csih-working-pages-coverage-parent.json passed：13当前records合计53248原文字节，4实际页15连续片段，对不可变原body逐字节匹配并全域无缺口；expected tuple与实际page响应绑定一致，最终concern原文在真实对应source正文中，页observations仍非verified语义证明。r2 native rc0/frozen agent1680fe864…；原崩溃证据保留。尚需strict/audit/旧回归与父真实模型验收，未部署。

父独立重新运行永久task_working_set pages终态PASS：/tmp/csih-working-pages-independent-parent-review.json，outer55/compile14/内run bound；native rc0/frozen=true，12原read→实际evidence回取→4分页→final accepted，19POST/14actor动作，全部13record按字节覆盖且原concern在final。脚本协议不是真实语义，strict负例/审计失败与旧回归仍按各receipt核，不复用此前source绿；未部署。

父工作集隔离candidate构建终态rc0 /tmp/csih-working-set-candidate-parent.json，hash8f4713c11e6b394af603bbb1974bf569615d27d2c42cf35dc7437ae6a967dd1c，源绑定1680fe864…；两compile/two selftest通过，私有candidate保留。待strict/audit/旧回归收据齐备并核scope后真实默认模型复测，不先部署、不把模型脚本accepted当任务语义绿。

工作集边界receipt已核同源1680fe864…：invalid/gap/duplicate/NUL/unknown-concern/late-invalid/auditfail预期rc1均passed/frozen，escaped与pages预期rc0均passed/frozen。旧回归由代理继续，不扩大重复同scope。父启动隔离真实r19同r18当前状态问题/39旧journal/sharedHOME/实际notice/default deepseek-chat，candidate8f4713c…固定；本轮是语义验收，非部署，旧回归未齐不能发生产。

真实r19终态 /tmp/csih-working-set-real-r19-parent.json：16POST约56.19秒，自主10read，partial答复→3native页（observations全空）→最终unverified，未超容量，candidate verified/server cleanup0，私有worker/host均已核gone；child KeyboardInterrupt为父实验宿主清理非自然退出绿。actor正确rank12PASS/csih2 activated/committed，却仍把csih1部署收据嵌入旧input高度块红/未reload当现态。最终judge称无本轮必要读取/无当前tool证据，与真实10read/全目录不符。机械工作集/分页进步成立，来源时域与最终证据消费仍未语义验收，不能部署或宣称产品成功。下一查new final包是否把正文支持全丢仅空page ack，设计要求关心反证原文已保但正向支持未呈现；先只读实际wire诊断，不继续盲改prompt、不把typed目录当语义proof。旧回归仍由代理收口。

### 2026-10-08 r19 工作流/上下文审查收口
主人明确优先工作流和上下文工程，UI/UX后排。实际final wire仅保留三份 observations=[] 的分页回执及目录，分页读过不等于最终判断仍持有语义证据；actor把部署收据内旧input误当当前状态。证据 /tmp/csih-r19-final-context-review-parent.json。C 1680fe864 保持冻结、候选不部署。下一件仅设计：任务事实工作集、来源/时间/当前与历史分层、逐主张原文依据与支持/反证/未知保留、决策步明确下一动作和结束条件；先交设计再实施，不以机械回归绿冒充产品闭环。

r19 二次只读核对：本轮 csih1 实际工具body同时含 passed=true、activated、committed、pid57958 与历史history串，故不能归因未读取部署事实。agent.c:2010分页规则偏反证，2031给空observations模板，2078/2079最终只保回执与concern原文，肯定性原文支持无保留契约。证据 /tmp/csih-r19-source-layer-review-parent.json；下一设计必须逐主张保支持/反证/未知及来源层级，不仅改提示词。

工作流真实问题验收基线 /tmp/csih-workflow-status-acceptance-parent.json 已建立：双部署收据、私有rank范围、历史串不可覆盖部署事件、未知保持未知、逐主张当前ID+原文区间、final保正反证、零越权。只是验收规格，未通过；模型accepted与脚本绿不替代父对照原文。

2026-10-08 partial审查路径新增只读发现：agent.c:2518先清claim_active/nrefs，只在completed分支复制s.evidence；partial可进入review_active但其引用不作为结构化claim refs传给judge，r19 final expected_evidence=[]。这不允许partial直接验收，但“尚未完成”不应等同“没有本轮读取”；设计应分别建观察引用和完成验收引用，保持partial不可accepted。尚未改代码。

2026-10-08 下一设计已审 /tmp/csih-workflow-context-next-design.md。新增证据纠正：r19首页模型确提出矛盾观察，但note UTF8 353/357超319导致整页拒绝，后退为空数组，不能称模型从未识别矛盾。接受主张—证据表/原文final grounding/partial观察引用独立方向；尚缺精确结构与版本生命周期，未授权直接生产实现。下一交付同文档补唯一JSON契约、跨页聚合与无主张路径、失败诊断和预算，保持原权限/累计预算/partial不可accepted，随后按完整纵向片实施。

2026-10-08 初页实际回复独立复核 /tmp/csih-r19-page-rejection-review-parent.json：note长度353/286/357/284确认；其中第二项正确识别部署反证，第三项却把state cwd/hash当mainrepo/goal证据，第四把历史派单当交付。验收基线新增这两种禁止错误推断。故修note长度不足以完成语义验收，主张来源分层仍必要；原失败不改写。

2026-10-08 主张—原文纵向契约已审并批准首次实现：/tmp/csih-workflow-context-next-design.md 新六键answer、≤8claims/≤16观察ref、结构化页支持/反证、final必保连续原文、partial观察ref独立、精确错误反馈、provenance。JSONPointer/layer保持模型解释unknown，不伪称验证。实现交专职agent，不由父改生产；保持权限/累计budget/partial不可accepted/原audit。新规划器next_action暂不实施且不得半接受。父复核要求：全引用联合容量测量、原文UTF8字节验证、共享ABI同步、超长response拒而非截断、legacy真实状态明确；完整纵向后冻结机械验收再真实模型验收，未部署。

2026-10-08 父只读实现前复核：agent.c:2338 当前仍snprintf(AT.content,sizeof,...jstr(ct))，服务正文过长可先被截断再审计；2256旧注释亦承认这点。新16K契约必须在复制前核完整decoded回复长度/UTF8/NUL，过长明确拒、不执行动作且审计能诚实标服务回复过界，不能仅增缓冲区。AGENT_CONTENT_MAX与parser两scratch/AT.content目前同宏；共享结果头agent_cli/tui均include，无新字段前ABI一致。

2026-10-08 超长回复漏洞已独立实跑确认：/tmp/csih-response-bound-baseline-parent.json，冻结旧1680 baseline native build rc0；localhost脚本模型送17382字节首write+空白+第二write，实际第一私有文件写23字节，第二不存在。整段本应拒却先截为单动作执行，不是仅理论风险。run rc1来自后续显式failed收口，不能掩盖先前副作用；原trace及文件保留。新实现验收须用fresh私有目录确认两文件均无，不复用本次已写路径。未改生产/窗口，无真模型调用。

0.0.35 队列修红：多单元 printf 的源名分类须在私有名字改写后仍保留，且按实际名字查询变参描述；D1 静态初始化先判自动对象再判通用非恒定值，诊断优先级与参考一致。

2026-10-08 主人新增指示：让csih1/csih2给自己设计/help、/clear等常见harness CLI命令。父只读确认tui_slash已有/reload-code、/export-state HANDOFF、/goal、/loop、/reload；/exit和/quit在submit。/help、/clear尚无native分支。安排csih1唯一docs/harness-commands.md（仅设计），csih2从用户任务/忙闲/破坏性边界审查；不与冻结agent核心改动混跑、不改源码/重启。必须区分清屏/模型上下文/完整审计与共享memory，命令native零模型请求、未知斜杠显式报错，列实有与拟议及验收。

2026-10-08 主人明确要求csih1/csih2尽快完成剩余产品设计改良与实现，授权从仅命令设计扩为分批实施验收。分工：csih1产品设计/TUI生产实现（不得碰agent核心），csih2只读UX审查/验收（现watch角色，不靠提示词绕权限），父调度/独立本机验证/实际窗口验收；核心agent负责clear上下文边界等必要agent.c对接。先交现状→缺口→交互契约，再直接实施已可定义第一批/help /status/未知slash零模型；/clear须真上下文边界不能只伪清屏；完整UX剩项任务流/忙闲/失败反馈/内容布局/历史操作列为可验收后续批，不以命令子集缩目标。生产各路径单owner，每阶段≤10min，禁止重启/提交/公开发布，候选本地验收后再由父受控换版。

2026-10-08 命令草案实际已落盘：docs/harness-commands.md。父审查纠偏：不能把仅清UI当/clear完整交付，目标是可控模型上下文边界；忙时默认立即拒绝，非需主人再拍板。发送历史/goal/loop/audit/mind默认保留，不凭清屏伪忘。/loop现实际toggle无on参数；/goal无参不清目标，帮助必须按真实行为写。/status需实际运行身份/版本，未知明确。第一批/help/status/未知slash可先实施，clear需核心对接后真实model wire证明旧context不再引入，维持完整目标。

〔0.0.36 A1 对象互调切片，2026-10-08〕先补产品 E3 的已声明外部函数调用：unitmode 的 UM.cc.thunk 当前具名拒绝，按参考 ccx_plain 的整数/指针、至多六参数范围构造 ABI thunk，并保留不支持签名的精确拒绝；用 Linux arm64 非 PIE 对象与 agenterm 动态库真实链接/运行验收。第二片做外部函数取址 GOT：参考当前也缺 GOT，因此参考及产品 lower/enc/对象重定位共同落地，验证 PIE 链接及地址调用，不能把 -no-pie 当作完成 GOT。第三片才接 Mach-O 对象产品入口（当前 compiler.c 明确仅支持 Linux 对象）；不因 Linux 通过宣称 macOS dylib 已支持。切片只登记设计，尚未实现；不扩 K1/K2。

〔A1a 实测，2026-10-08〕产品网络已补显式原型、至多六整数/指针参的外部调用桥；私有网络驱动生成 agenterm ELF arm64 对象与参考 84056 字节完全相同，Linux 非 PIE 链接实跑与 gcc 输出一致（去 PID）。x86_64 最小对象同字节；r21 为 48 同/5 具名拒绝，整数桥另验动作契约。外部函数取址（包括先调用后取址）仍具名拒绝，等待 GOT 独立片；浮点/变参/无原型/七参仍拒。不是正式候选 .com 验收，需 cc 构建候选补正式证据。只变 parse2 八个和对象编码两条图基线，不扩 K1/K2。

〔A1b GOT 同落方案，2026-10-08〕仅对象编码器的未定义名取址改为 GOT：ELF arm64 发 adrp+ldr、重定位311/312；x86_64 lea改mov、重定位9且addend=-4。内部代码/数据地址保持旧路径，不增指令长度；外部名非零偏移继续拒绝。先与隔离参考补丁的真实对象逐字节对拍，再协调参考与δ同批落地；E3用户函数地址放行及Mach-O对象入口另行验证。

〔N1 6.7.8p13 δ 实施，2026-10-09〕自动聚合初始化在表达式求值后，以右值结构类型沿外层结构/数组槽向内查同型子对象起点；命中整体COPYSTRUCT、游标跨该结构全部槽，未命中仍标量转换/写入。仅自动存储、非指针结构表达式；全局与块静态常量判据不改。与f27a5129参考stobjat的外层优先和布局偏移同形，验收67及嵌套/结构数组/设计符。

〔N1 标准预定义同落，2026-10-09〕E2 在CLI -D之前安装三项标准宏，默认与shared-predefines共用小清单及名称/替换串领域表；不改目标宏资源格式。__STDC__=1、__STDC_VERSION__=199901L、__STDC_HOSTED__=1，对齐research/stdc-reference.patch，-D可覆盖、-U可删除，版本宏独立body。先隔离补丁参考对拍，禁止只改名表使版本值误为1。

〔N1 register 产品覆盖，2026-10-09〕register 保留为独立 token，在块声明、for 初始声明及参数类型入口消费，不在 tokenizer 全局抹掉，避免把文件域 register 当作普通定义接受。parse2 追加词由领域表统一供 Python/C 种子读取；有效声明的 tape 与参考逐字节一致。

〔云机接班单切口，2026-10-09〕cdx-unisacc 在 /home/box/repos/unisacc、main 235ca5bd、干净工作树接班，宿主原生 Linux x86_64，未携入 m4pro 的仓根产品或 out 缓存。本轮只核验 C2 g5 typedef 翻译单元隔离的产品 δ；参考 fe_units 已有 td0/ntd 复位，产品 startup-marker 的 @unit+ 当前只递增 unit_epoch/复位 ixcount，TDN 仍按 intern 名称共享。先复现并核对名字可见性和描述符保留，再决定最小修片，不动其他切口及验收措辞。

〔0.0.37 开工，2026-10-09〕cc 解冻 main，0.0.36 队列留在29728cc6独立冻结树。cdx先做C1：66结构体返回调用必须与参考tape同形；register对象取址诊断参考与δ同落，不能只用有效声明通过就把6.7.1记covered。C2由只读子代理最小化g4/g5。F4′沿用钉住v0.0.32 csih 15单元、正式同源候选冷编≤5s及csmithdiff/difftest/com无豁免；W2沿用Windows两ISA的csih DNS+真实HTTPS和参考行为对拍；E57余半随W2结算，不另换验收。预算估算另报cc，实测阻塞即重估。

〔0.0.37 C1a 验收，2026-10-09〕4ab53c46 已含产品结构体返回调用的清单/模板与错误契约：非局部返回走已有 LI.structreturnexpr 的类型/分号检查及 WCOPY，局部对象路径不变。66与259字节结构体分支返回的完整E3 tape同参考；错误/警告模式各18完整结果同，r21为48同、5具名拒绝、0 accepted-different。C3头新增后补重导k2-gen2/k2-libraryenv/pp-autoinc-gen，facts30表0 differ；重录parse2八项与pp十三项（实际变8+12），fresh-order0..8全核对且无变化；kernel stale0，op9。仓根.com直接-run seed/gen.c产出完整parse2与Python逐字节同。正式0.0.37候选.com运行66仍需cc验收后删knownfail；return(mk())等非ID入口仍具名未覆盖，不把本片说成所有结构体返回表达式均覆盖。C1b register取址及C2仍未结。

〔0.0.37 C2 g4 验收，2026-10-09〕无原型普通直接函数超过六实参走现有参数栈反转，再按恢复的sys=0选择直接调用；内建函数超限仍具名拒绝，间接调用与默认float提升控制保持参考同tape。新增b_noproto_many探针覆盖0/6/7/8参、不同位置权重及嵌套调用，系统cc与参考实跑均8 91 140 204 204 204；完整流水线E3逐字节同。parse2八变体基线重录，fresh-order0..8无变化，facts30表0 differ，r21为48同/5具名拒绝/0接受不同；仓根.com -run seed/gen.c完整parse2同Python。正式新候选跨单元运行留给cc补验。g5用zlib1.2.12历史配置尚未复现原incomplete struct：全编先被K&R声明阻挡，描述符前缀可编，不记已修。

〔C1b register 取址实施约束，2026-10-09〕声明属性独立REGISTERBANK，作用域undo新增第43格（总44）；对象与参数声明显式捕获register，结构/函数指针嵌套保存恢复，不复用类型/联结银行。unary-&按参考14例的对象/子对象规则预检；不得直接TN.raw前瞻再只恢复位置，其可达分支会写TIX与转换临时银行。独立token视图原型仅识别id、括号、点、箭头、下标，其余整行不透明跳过，24项普通/位置v1/v2及截断检查通过；尚未接入parse2，不算14例诊断验收完成。

〔C1b 元数据首片实测〕逗号续项、普通参数、局部遮蔽、函数指针对象/参数、嵌套结构、sizeof不泄漏共7例tape同参考，探针观测REGISTERBANK置1、遮蔽清0并恢复1、退域恢复0；r21仍48同/5具名拒绝/0异，facts30/0、op9，parse2八变体图重录，fresh-order0..8不变。默认图临时注入独立预读/游标恢复，含数字/字符串的4份完整tape不变；带位置的24项只读视图检查通过。以上不是unary-&14例完成，参考补丁仍隔离。

〔C1b 取址检查接线〕采用独立有限token视图，仅查询根及结构成员；合法名字在声明时已INTERN，复制图14例中新增intern顺序不变。SBFIND查询的是blob而非符号，不用它访问REGISTERBANK。根/成员数组按SHAPE rank跟踪，跨指针下标、箭头或调用放行，仍在register对象内则报C99 6.5.3.2p1。复制图6接受同tape、8精确拒绝；正式清单/errors/location与C执行器随后验。

〔C1b 诊断验收补正〕正式C执行器发现uc_kind=1会多报结构返回未覆盖帧；register取址是C约束诊断，应设uc_kind=0，完整stderr必须与参考逐字节比较，不能只检查子串。

〔0.0.37 C1b 同落验收，2026-10-09〕参考register取址检查与E3独立token视图同落，tests/diag.sh增加14例（6接受、8拒绝C99 6.5.3.2p1）。正式默认清单14例通过且接受tape与INTERN新增顺序不变；C执行器E2→E1→E3位置/错误路径14例完整tape或stderr逐字节同新参考，UNITOK2另4例完整结果同；参考diag31/0、r21为48同/5具名拒绝/0接受不同。parse2八变体graphhash重录，其他阶段记录不变；fresh-order0..8全重录核对无变化，facts30/0、kernel stale0、op9。仓根.com直接-run seed/gen.c生成完整parse2与Python逐字节同。正式0.0.37候选.com的14例运行由cc补验，未以合成tape替代；register数组隐式衰变等未纳入本片，不把整个6.7.1宣称covered。元数据额外嵌套FP不泄漏验证累计9例，前文首片7例为当时记录。

〔L1b′ 勘察约束，2026-10-09；只是设计〕不能只修.sys6的SYSFP/SYSSP：六参数SYSA格以及.sys/.write参数格也共享，信号可在保存或装载之间插入而污染外层调用。优先设计POSIX系统调用的私有栈帧（现有load64/store64/栈调整表达），同时核对真实信号入口的host ABI→tape栈边界；旧sigaction补丁直接传函数地址，既有__ccw转换只在转发桩启用时生效，不能假定直接内核回调已有包装。参考、头、lower/enc同批落，先证明嵌套重入和处理函数返回；不以仅修arm64重装FP/SP代替完整验收。详见research/l1b-signal-lowering-design.md。

〔L1b′ 第一轮原型实测，2026-10-09〕仅在/tmp复制头与导出参考C上应用旧sigaction补丁/诊断变体，main产品未改。重构sig_nested：旧参考osx/arm64 rc138、x86_64 rc139；arm64仅删除共享FP/SP恢复的诊断变体rc0，仅强制trampoline host入口rc139，两者合用rc0。私有80B sys6帧+临时host入口桥接：arm64单层rc0，x86单层rc142；更强双信号嵌套系统cc为MABCD/rc0，原型两架构只到MAB（arm64 rc139，x86 rc159）。因此不交δ、不记完成。arm64 .frame只动x7而真实SP未同步，嵌套备用栈可能覆盖tape活动帧，属待验证推断；x86另需核对sigreturn现场。证据与命令在research/l1b-signal-prototype-results.md。

〔L1b′ 第二轮原型实测，2026-10-09〕x86入口原始r8/r9与跳板uctx/token逐位相同；TO_GATE前rax=0x20000b8及三参数正确，排除桥接换址和调用号错误。cc系统clang对照定位Rosetta须UC_FLAVOR=30；临时头按此改后，私有sys6帧原型单层与双层通过。arm64仅在gate期间把真实SP移到x7下512B、保存恢复原SP，双层由MAB/139变MABCD/0。两架构双层各5/5通过。这是缩小根因的临时原型，不是产品完成：gate之外异步中断、sys/write共享格、通用入口与四目标δ同字节仍未验证；512B是诊断值，不是栈契约。详见research/l1b-signal-prototype-results.md第二轮。

〔L1b′ 生产边界设计，2026-10-09；尚未实现〕备用栈与UC_FLAVOR头补丁继续停放。arm64拟建立真实SP≤活动tape栈下界的逐指令不变量：分配先降低真实SP再提交x7，撤帧先提高x7再提高真实SP，真实SP保持16B对齐；call/ret、显式写r7、HENTRY/HLEAVE和宿主调用均须审计，不能仅在gate同步。入口优先核对既有__ccw_保留前缀契约可否表达头内信号跳板，不引入按单个函数名硬接线。sys6/sys/write的私有快照与FP恢复同片验证。此处是下一轮原型约束，不是产品验收结论。

〔L1b′ 第三轮异步反例，2026-10-09；实际跑过〕临时arm64编码器在.frame分配前降真实SP、撤帧后抬真实SP，并处理call/ret及HENTRY/HLEAVE锚点。强探针由宿主异步送ALRM/USR2，外层处理函数反复递归建帧、检查局部数组；全程保护原型3/3正常，只有gate同步原型3/3首次送信号即SIGSEGV。最初外层只在gate内触发嵌套的弱探针两者都绿，已明确不作反例。证据和诊断补丁入research；仍缺任意r7写入完整覆盖、sys/write私有化、通用入口、产品δ和四目标验收，不宣称L1b完成。

〔L1b′ r7/HOSTCALL 原型续验，2026-10-09；实际跑过〕mov写r7临时编码区分方向：向下先降真实SP再写x7，向上先写x7再抬真实SP。独立低级tape移动SP下128B再恢复，实际退出0；双层与外部异步递归反例续验绿。HOSTCALL审计见既有ARM桥用私有真栈保存x1–x7/原SP/LR；加入真实libSystem sched_yield转发的异步递归探针退出0，tape确认含该函数及.hostcall。以上不等于任意算术写r7或所有宿主签名已验；下一片仍为完整r7写入覆盖与sys/write私有快照。

〔L1b′ 私有参数与通用入口原型，2026-10-09；实际跑过〕临时POSIX .sys/.write已改为80B私有帧快照，覆盖atfd_1/atfd_1_zero/atfd_2_zero5/zero4参数形状；sys6原SP暂存由x16改x12，避免.frame计算覆盖它。两架构低级tape分别用r7作为sys6/sys/write缓冲，均ABCABCABC/0。移除信号跳板单名判断，临时头改用已有__ccw_前缀，两架构双层MABCD/0。ARM普通目的寄存器为r7的计算先产出到独立暂存，再按移动方向提交栈；sub/add/load写r7低级探针退出0，HOSTCALL+外部异步探针194次ALRM退出0。这些仍是隔离导出C原型，未生成产品δ；.exit/.print共享格、逐快照重入注入、Linux信号入口和正式四目标验收继续保留。

〔L1b′ 裁库闭包核查，2026-10-09；实际跑过〕libneed.table从头内标识符自动提依赖，不需手写sigaction边。隔离应用14c85649停放补丁后，完整table先报carried body defined twice: alarm（unistd新增POSIX/Windows两个同名static体，解析器不按条件编译去重）。仅在临时副本删去Windows重复体以检查依赖后，sigaction闭包含__ccw_unisa_sigtramp、_unisa_sigkernel、_unisa_sigaltstack、_unisa_sigrestorer与_unisa_ret，守卫名匹配。停放补丁需把alarm平台分支放同一函数体，正式头落地后导出pp facts/E2，不添加手写边或放松重复检查。

〔L1′/L2 数组拒绝最小化，2026-10-09；出货产品实跑〕宏N=32的static unsigned long mask[N]独立-run退出0；仅前置extern const char version[]，同一完整数组在[32]后被拒incomplete array declaration before another definition，退出1。因此存在可复现的未完成extern声明历史影响，不能把完整数组报错直接归因宏长度识别；sqlite现场是否同根尚未验。两例入research/l1prime-array-history，排L1b后，当前输入窗口未动。

〔C2 g5 单元边界，2026-10-09；源码核查，补丁待验〕research/g5-typedef-leak记录a.c的文件域typedef code泄漏到b.c参数同名，交换单元顺序不报错。fe_units每单元重载token而未重置ntd；typedef名字可见性应按翻译单元隔离，不能靠增大MAXTD结算。L1′参考窗口并入此项，由cc准备补丁及顺序互换/不同同名typedef/前单元签名保留的对拍；δ另核对TDN/TDB/TDD/TDE等名字绑定的单元复位。结构描述与跨单元函数签名必须保留，行映射偏移另列，不混作typedef修复验收。排在L1b首片后，尚未修改生产输入。

〔C2 g5 行数补丁审阅，2026-10-09；系统cc实跑，修后参考待验〕unsized-rows.patch用initrowsat(j,per)把每个独立字符串都计为完整外层行，但字符串可初始化指针元素或内层char数组，不能只按token判整行。反例const char *p[][2]={"a","b"}的sizeof为2*sizeof(void*)，char s[][2][4]={"a","b"}的sizeof为8；系统cc合并探针退出0。当前补丁按其扫描逻辑两者均算两行（未跑修后参考）；需携带元素类型/剩余维度，或对此精确拒绝，补对拍后再同窗落。bkscr写越界是独立后端缺陷，缩短此数组不能算边界防护完成。

〔g5 头续行不只影响诊断，2026-10-09；出货产品与系统cc实跑〕包含research/g5-header-splice-line/m.h后，在主文件第3行return __LINE__ != 3，仓根unisacc.com -run退出1，系统cc编译执行退出0（临时源/tmp/cdx-g5-splice-line/line.c）。src/front_pp.c的line_at与diag_at均以ireg_ln/ireg_nl撤销include拼接，line_at还供__LINE__/__FILE__宏使用；不能按仅诊断、不影响代码收口。参考补丁需同步line_at/diag_at/头来源映射，产品E2位置记录与宏展开同规则；验收增加主文件/头内/嵌套头的LINE与FILE及CRLF续行，普通预处理文本不变与受影响宏值的修正须分别对拍。生产输入未改，仍排L1b首片后的窗口。

〔rowcov 原生观测可行性，2026-10-09；源码勘察，尚未实现〕Python sim在transition查询之前记(q,key)，故拒绝/缺边前的最后一次观察也必须保留；t模式栈空的BOT在C为-1。C core_run与ARM/x86汇编run均需观测钩子，不能只改C备用循环。CoreModel没有状态名字，原生日志应先输出阶段/模型身份+数值state/key，以同一模型绑定的状态字典还原，再关联现有终态出处旁表；不得假定另一变体的state序号相同。UNISACC_ROW_LOG当前是构图出处旁表开关（state,key,path,line），运行期观测另用UNISACC_EDGE_LOG避免格式混淆。运行期每次模型执行只写首次命中的边并带探针身份，日志IO失败须显式失败；输出流与模型字节不改。先小模型C/两汇编与sim逐边集合对拍含拒绝/BOT，再pp一片和全pp，最后lex/parse2/lower/enc；各层保持相同模型、输入、资源和flags，不能以默认产品整链替换现有rowcov指定变体。阶段数秒是待测目标，不承诺已达到。排L1b生产首片之后，与cc的C汇总器接口设计可先并行。

〔rowcov C 日志接口反例，2026-10-09；产品-run实跑〕tests/rowcov.c把state_id=-1视为BOT跳过，但机器BOT是key=-1且state_id仍有效。最小STATES 0/S0、EDGES与ROWS S0/BOT、LOG M/0/-1/p.c：原生日志路线taken0/covered0/rc1，--seen S0/BOT/p.c路线taken1/covered1/rc0。应拒绝所有未知state_id（包括-1），把key=-1规范化BOT再关联旁表；已有--seen汇总计数对拍不覆盖该日志接口。STATE字典必须由构造时保留并绑定同模型身份，现CoreModel不含符号状态名，不能从二进制捏造恢复。运行钩子尚未实现。

〔L1b sys6 验收探针审阅，2026-10-09；源码核查〕sys6probe.c的12个__syscall6在参考前端均由sysargs6固定送r0..r5，并发出.sys6 syscall，不因常量号变成具名op，也不因局部声明顺序改变tape源寄存器；帧内buf地址经求值送入r2，不等于直接以r7作源。现12/12证据仅覆盖C参数求值/动态号写调用，尚缺具名mmap和低级tape寄存器置换/r7别名。accept.c中sh编译失败未设置bad、未清理旧产物就sha，可把旧文件当新证据；需生成临时产物、成功才替换、编译失败显式失败。private3的Windows ARM镜像变化已标反例，首片只移POSIX sys6 helper，不搬ARM编码器原型。生产代码仍未落。

〔L1b POSIX sys6 生产首片，2026-10-09；参考实跑与δ对拍进行中〕六个源寄存器在修改ABI寄存器前写入80B私有tape帧（0–40参数、48旧FP、56旧SP）；r7作源取帧分配前的快照，arm用x12、x86用r11保存旧SP，避免未来ARM.frame计算所用x16覆盖快照。动态号从槽0取、其余五参从8–40取；具名调用六参从0–40取，gate后从私有帧恢复FP并释放帧。Windows仍走原全局格路线；不在本片落信号头或编码器SP更新，.sys/.write与HOSTCALL私有化另片处理。生产参考通过验收器四次macOS运行及Windows四镜像基线；lnx/x86_64原生δ对手写tape255条指令与Python逐字段同，其他目标与正式产品验收尚待完成。验收手写tape带注释，lower文本入口目前不接受分号注释，对拍只去注释不改变指令；不把该格式拒绝当sys6错码。

〔L1b sys6 首片验收补齐，2026-10-09；实际跑过〕生产代码已由共享main提交819793f1收录，未改写该提交。六目标lower逐指令/元数据对拍、四POSIX手写tape镜像与C参考逐字节同、参考四次macOS实跑、Windows四镜像旧基线、产品-run种子构造lower整图逐字节同均通过；facts 30表0异，lower九模式基线重录（Windows/default不变），fresh-order九模式无变化。正式新.com实跑与Linux实跑尚待cc候选验收；不据本片声称完整信号安全。详见research/l1b-sys6-production-slice.md。

〔L1b 首片产品补验与第二片边界，2026-10-09；cc提供实跑回执、后续为源码勘察〕cc在同源私有候选a4943d78（8bb59293）上跑accept ok，Windows四镜像旧基线相同，POSIX十二行变化，macOS四路与Lima Linux arm64的C/tape两路通过；参考七片已同窗落7646bb81，阶段门禁进行中。本会话继续只读准备，不动src/include/kernel/exec。下一片优先.sys/.write私有参数帧：严格复用code-abi-sources.tsv中mode0/2的有序mem/imm与参数数目（plain三、zero4四、atfd_1四、atfd_1_zero三、atfd_2_zero5五、write三），仅把mem源改成帧槽读取；旧隔离bk_proto3固定装六个参数不可直接移入生产。准备说明见research/l1b-next-production-slice.md（只是设计，未实施）。Windows保持原序列；ARM逐指令SP保护、HOSTCALL和信号头继续独立结算，私有帧本身不解决ARM真实SP仍高于活动tape帧的问题。

〔C2 g5 云机产品反例，2026-10-09；实际跑过〕原有 a/b、s1/s2/sm、g1/g2 双向共六组 typed E1→E3 模拟输出与 Linux 参考逐字节相同；参数遮蔽已清 TDN，原例不足以证明产品有错。补充反例：第一单元 typedef struct {int x;} code，第二单元未声明 code 却 return (code){1}.x!=1；参考拒 unknown identifier，产品 δ 接受。拟只在 @unit+ 清 typedef 名字可见标志及其枚举类型标记，保留描述符、结构池、跨单元函数签名；用 typedef 写入的 intern ID 上界限定清理，逐名只处理有效 TDN。验收含原六组保持同字节、新反例双序拒绝以及同单元合法复合字面量。

〔C2 g5 云机验证资源边界，2026-10-09；实际跑过〕修后普通 E3 原生网络穷举 2559878 观察一致；原六组 typed E1→E3 模拟与原生网络均同参考 tape，新增复合字面量泄漏由接受变拒绝，合法全局同名对象同字节。unitlocationcheck 全过（位置序列化、10 map/7 frame 拒绝），seedfacts 1076 同/0 异。并发两 Python 变体与 C seed/gen 构造时，内核 OOM 日志杀 cdx37-seedgen（anon-rss 5932868kB），两变体超 55s；已报董秘并改串行，不放宽 60s，不计通过。C 种子构造一致性与正式候选/新增 multi isolation 全片实跑仍需补验。

〔C2 g5 云机修片冻结前验证，2026-10-09；实际跑过〕八个 parse2 变体均串行/受限构造成功，fresh 哈希及计数全部保持既有基线；仅更新对应八条 graphhash。r21 复用本轮新构造普通 E3 JSON，PP/E1 重建后原生网络实跑 48 同/5 具名拒绝/0 接受不同。errors 模式的跨单元泄漏与同单元合法复合字面量双向共四组 tape/完整诊断与参考同字节。回归借用的七份最小例复制入 tests/multi/typedef-*.c，纳入既有门禁的 tests/multi 输入闭包，避免 research 例变化不使队列缓存失效；新增 multi isolation 全片与 C 构造器一致性、正式 .com 仍待 m4pro 验收。证据整理于 research/c2-g5-unit-typedef-cloud.md，不结算整个 C2。

〔云机提交身份，2026-10-09〕云机未配 Git 作者，当前修片提交只用命令级 cdx-unisacc <cdx-unisacc@localhost>，不改全局配置、不借用人的作者身份。会话中 main 已经论文/计划文档更新前移666bb0bc；编译输入未变，本片仍只提交自身路径。

〔C2 g4 云机接单核对，2026-10-09〕董秘/cc授权先推g5再只跟g4；g5与cc的c4e5001f文档已合并推到cee7ccc7。当前main已含5a632535的g4实现：普通sys=0超过六参走CL.vdone，CL.vend按sys=0恢复直接调用；callcontrol.py已于15bbf1bf迁成manifest删除，云机/home/box与/tmp未找到暂存副本。先按当前构造链重新生成E3并复验无原型/有原型/K&R与多单元；不手改TSV、不复活已删除构造器、不以公开0.0.36旧拒绝判main缺口。cc独立克隆验收待交接。

〔C2 g4 当前源码复验，2026-10-09；实际跑过〕main d87c8917 从当前 manifest 重新构造 E3，JSON sha2187cf66、net shaf25d68ab，与g5冻结证据一致。六类九组（无原型/原型单文件、原始a+b双序、加权无原型/原型双序、float默认提升）原生E3 tape与参考逐字节同，系统cc与参考-run均退出0。cc确认K&R定义在参考本身expected {，撤销该变体；正式验收保持a+b、完整单文件、原型对照三者候选.com退出0且同参考。无需重复修5a632535已实现的控制，不手改表、不复活15bbf1bf已删除的生成器；当前仅补可复现证据并交cc独立克隆。Darwin构建限制仍使正式候选未验，不结算C2整项。

〔0.0.37 Linux 正式候选构建调查，2026-10-09〕政委明确不写死Darwin，授权查实依赖并移除可移植守卫，目标云机完成候选及发布。已pull ded65f53与后续文档至31f1ebba，不改验收。buildcompiler守卫背后是seed/blob.c、asm/blob.py以cc -arch/Apple ld参数生成Mach-O并抽取含头的PIC kernel，不是运行时OS依赖；拟先用Linux LLVM Mach-O交叉汇编/链接验证UNIKERN1、零未决导入/运行重定位、入口/slot与原生执行契约，再修改构建适配。LLVM未安装，准备安装Debian现有clang/lld/llvm19。Apple签名公证/dmg与多平台证据是独立发布边界，不能仅删Darwin行就宣布发布可行。

〔Linux Mach-O 种子最小实验，2026-10-09；实际跑过〕LLVM19交叉构造两ISA后，原blob.py抽取/结构检查均接受，ARM7672B sha9bab0499、x867696B sha56bcd071，未决导入0；与公开0.0.36 Apple ld64包内blob不同，不能冒称逐字节同。拟让C/Python两构造器共享一个host工具适配（Darwin保持原cc/ld参数，Linux用clang+ld64.lld+llvm-nm），UNIKERN1格式与所有PIC/入口/零重定位检查不变；扩seedgen首片原有blob对拍至Linux，先产品ABI原生x86网络执行后去buildcompiler守卫。cc已查Apple签名/公证/dmg及macOS门禁为真依赖、GHCR缺write:packages；这些尚不能由Linux替代，发布不予豁免。

〔Linux kernel 实跑与构建路径，2026-10-09；实际跑过〕共享machocc适配后C/Python双ISAblob逐字节同；产品ABI x86原生netcheck全过，含E3表2559878观察、资源/包/拒绝/诊断链与无Cfallback；seedgen e2首片SAME e2/ident/blob，3同0异。保留默认C构造，首台未装产品或Cgen内存不足时使用原有SEED_GEN=0/SEED_C=0路线试建（非新增回退）；build_candidate的已安装产品pair前置只在实际使用它的SEED_C=1要求，Python路线不读取该pair。正式发布仍受macOS门禁/Apple资产与GHCR权限约束，不改验收。

〔Linux 候选全构造，2026-10-09；实际跑过〕既有Python构造路线SEED_GEN=0/SEED_C=0已完成shared、六target、三pack-prep、pack-models、pack-driver，每步bound58内成功。私有产品2243464B sha8d147e0f，源码身份80f404fc；版本仍0.0.36，非封版。g4五组/C1三探针同参考；初次diag因语料缺位0项失败，补齐上游语料后原套件31通过0错（40损坏语料、13定位、14register例）；seed七工具编译通过，pack.c仍在zlib include拒绝。政委新令本云机继续闭环，拟合入cc Linux门禁修片并交独立克隆，不降低验收。

〔0.0.37 候选版本推进，2026-10-09〕cc独立克隆实际验私有Linux构造：g4五组、C1 64–66、C1b diag31/0、C3七工具与realpath通过（research/c37-linux-precheck.md）；pack.c zlib包含仍拒绝，不结算C2整项。政委要求本云机继续完成发布，现只推进同一候选构造/验收切口：版本号升0.0.37，重建同源参考/候选，构造seed对并验证comboot定点，再交Linux全量642套件。升版本不是封版或全项完成。

〔0.0.37 云机构造交接，2026-10-09；实际跑过〕版本b398a194；同源UA /tmp/cdx37-linux-ref，候选/seed对在/tmp/cdx37-linux-candidate。候选2243464B sha52173312，--version0.0.37，身份核验成功；seed7581888B sha928bda8d，hello与版本正确。g4五组、C99 64–66在新候选同参考rc0。shared并行首次58s超时，串行原预算内通过；cc已确认当时另一路comboot parse2并发OOM，原生Cgen单测也OOM（5576016KiB/18s），jemalloc试验50s超时；均不算绿。现完成Python构造后交cc独占COMBOOT_BUDGET=1现有窗口旋钮做定点及642套件，本会话停止重构造避免抢内存，不改验收。
