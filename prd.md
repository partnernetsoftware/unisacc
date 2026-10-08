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

最近已发布版本为 **v0.0.30**（2026-10-06 公开）。逐版发布身份见 [历史记录](archive/prd-release-and-development-history-20261001.md#逐版发布身份) 与 [GitHub Release](https://github.com/partnernetsoftware/unisacc/releases)；候选哈希与逐版验收不在本文件复写。后续计划见下表。

### 计划索引（正文在 plans/，prd 只放索引）

| 版本 | 文件 | 状态 |
|---|---|---|
| v0.0.12–v0.0.29 | [archive/plans/](archive/plans/) — 逐版计划与结项收据；发布身份与回执见 [GitHub Release](https://github.com/partnernetsoftware/unisacc/releases) 与 research/r*-release-acceptance.json | 已归档 |
| v0.0.30 | [archive/plans/v0.0.30.md](archive/plans/v0.0.30.md) | 2026-10-06 公开 |
| v0.0.31 | [plans/v0.0.31.md](plans/v0.0.31.md) | 已完成，公开待政委裁定 |
| v0.0.32 | [archive/plans/v0.0.32.md](archive/plans/v0.0.32.md) | 10-07 公开（unisacc.com d198bae2，候选 38ea3622；回执 research/r32-release-acceptance.json） |
| v0.0.33 | [archive/plans/v0.0.33.md](archive/plans/v0.0.33.md) | 10-08 公开（unisacc.com c2d03fe6，候选 e0a8ff7c；rc/v0.0.33=bc23f113；回执 research/r33-release-acceptance.json） |
| v0.0.34 | [archive/plans/v0.0.34.md](archive/plans/v0.0.34.md) | 10-08 公开（unisacc.com f48938ef，候选 60f4765c；rc/v0.0.34=99150ee6；回执 research/r34-release-acceptance.json） |
| v0.0.34 | [archive/plans/v0.0.34.md](archive/plans/v0.0.34.md) | 候选已封（78ae645f，rc/v0.0.34=d70ba891），本地验收中；主题项 minicon/agenterm/包与容器顺延 0.0.35；公开须政委说「发」 |
| v0.0.35 | [archive/plans/v0.0.35.md](archive/plans/v0.0.35.md) | 草案（承接 0.0.34 顺延项） |
| v0.0.11 及更早 | 见 §7 归档索引 | 已发布 |

历史实施决定与试验见 [R19 过程记录](archive/prd-release-and-development-history-20261001.md#r19-开发决定)；仍未解决的问题由 [v0.0.31 计划](plans/v0.0.31.md) 跟踪。

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
- **.cx 替代 .sh/.py 的分期（主人 2026-10-07 问“哪个版本能完成”；cc 评估 10-07）**：现状——`#!/usr/bin/env unisacc.com -run` 的 .cx 已能跑（t.cx 实测传参正确），libc 已有 system/fork/execvp/waitpid/opendir/glob/regcomp/getenv，缺 popen；仓库跟踪 190 个 .sh（tests 106、exec 55、release 13）与 408 个 .py（tests 170、exec 118、unisa 48、ujs 29）。分期：**0.0.34** 补 popen 与一个小的 `include/cx.h`（跑命令取输出、读写整文件、sha256、限时），试点改写 3 个小脚本（prevheaders、headerchain、lifecycle）并入门禁；**0.0.35–0.0.36** 构建与发布链（release/ 13 个 .sh、build_candidate、buildcompiler 编排）改为 .cx，由上一版 unisacc.com 运行，自举链除宿主 cc 外不再需要 sh/Python；**0.0.37** 门禁编排（gate/gatequeue/term）；**0.0.38–0.0.39** tests 下 170 个 .py 套件（两版分批）；**0.0.40** exec 生成器（cdx 域，δ 构造器已有 C 种子先例 seed/gen.c）；**0.0.41** 替换 unisa/ Python 参考实现（行为基准，须先有逐字节等价的 C/.cx 参考）。**全部在 0.0.x 内完成自举**（主人 10-07：0.0.x 就自举，0.1.x 开发包管理）。完成判据：仓库干净机器只装 unisacc.com 与宿主 cc 即可构建、门禁、发布。
- 0.2.x.y 系列目标（脚本领域、Rust 式内存检查）随 0.1.x/0.2.x 计划于 2026-10-07 撤下，原文见 git 历史（主人：专心 0.0.x.y）。

当前版本及其完成状态只在 §2 的计划索引维护。0.0.21 已公开；后续列入 [0.0.22 计划](archive/plans/v0.0.22.md)。0.1.x/0.2.x 远期计划已于 2026-10-07 撤下（主人：干扰太大，专心 0.0.x.y），旧稿在 archive/plans/。C99 种子构造器（R20-1）与单元链接语义（R20-3）的实施记录已移到 [archive/prd-notes-20261002.md](archive/prd-notes-20261002.md)，进度以 0.0.22 计划第 7 项为准。

### 5.0 种子层路线（主人定调 2026-10-03 19:51）

- **种子备用（主人 2026-10-06 更正）**：`unisacc-seed.com`(.build.json) 仍在 .gitignore，但与 `unisacc.com` 一样装在仓根备用（发布流程的安装步骤成对安装），不再只放 /tmp；**不随各版本发布公开**（sha 只记在回执里）。

0.0.24–0.0.27：K2 收尾 → 冻结 TSV/清单 DSL 并写语法与语义规格（opts 键与值前缀进门禁封顶；去掉 @stack getattr 回调、@fmt 的 str.format 依赖、=vN 不透明绑定；隐式顺序改表内显式规则；动作序列移出 facts；清死数据）→ 每个表条目与 DSL 操作配独立用例并量覆盖率（判对错不依赖 Python 产物或 graphhash）。0.0.25 起用 C 重写种子层（政委 2026-10-04 21:31 确认，原“0.0.25 之后评估”作废；路线见 archive/plans/v0.0.25.md 的自举路线 B1–B5），作为冻结规格的权威实现，最终完全不依赖 Python；Python 只留历史参照。详见 archive/plans/v0.0.23.md「0.0.24–0.0.27 方向」。

### 5.1 libc 路线裁定（主人 2026-10-02）

**反对自研 libc：系统已有的尽量复用（转发给系统 libc），参照 tinycc / `tcc -run`。** 依据与移交见 [research/libc-forward-handoff.md](research/libc-forward-handoff.md)，方案底稿是 [libc-unify-design.md](research/libc-unify-design.md) 的 D2。落地口径：每个函数族先进“转发 / 保留 / 拒绝”路由表，用探针与宿主逐字节对拍通过才切换，否则维持按名拒绝；字节与自举（N22、六目标折叠）的影响逐条标注。参考侧已随 v0.0.21 交付（[归档计划](archive/plans/v0.0.21.md) 第 4a 项），产品侧在 0.0.22。**补充裁定（主人 2026-10-02）**：不接受“Linux 静态 ELF 没有动态装载器所以不转发”——三个 OS 都要把动态装载做好：Linux 写最小动态 ELF（PT_INTERP 指向系统 ld.so，DT_NEEDED libc.so.6，四个 GLOB_DAT 槽绑定 dlopen/dlsym/dlclose/dlerror，与 macOS 的四个 eager bind 同形），Windows 把同样四个槽映射到 LoadLibraryA/GetProcAddress/FreeLibrary/GetLastError；于是 `__hostaddr0..3` + `__hostcall` 在六个目标上是同一条转发通道。只有用到转发的程序才写动态头，其余镜像字节不变。**方向澄清（主人 2026-10-02）**：反对自研 libc 的原因是它是大工程，**将来应做成外置的 libc 包，而不是内置在 unisacc.com 里**；“缺什么函数就补一个函数体编进去”的亡羊补牢做法是无底洞，停止。于是 0.0.21 的转发不是逐个函数写转发桩，而是**通用转发**：有原型、无定义、随带库也没有的外部函数，一律由编译器生成转发桩（0.0.19 R19-10 的 `fwd_stub` 机制，目前只在 macOS `-run` 下），扩展到写出的镜像与 Linux（借上面的四个 dl 槽）；随带头文件逐步收缩为声明，函数体只留纯计算且影响确定性的部分，最终外置。主人也说明静态与动态不是硬要求、产物体积暂不是关键，以实现与可维护为先。

**F4 分拆+顺延（政委 10-07 09:26）**：0.0.32 收已落地提速与钉住基准，实测约 6.0 s；E3 单遍等拆为 0.0.33 第一项 F4′，发布说明写明。

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
