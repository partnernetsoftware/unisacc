# unisacc —— 当前状态、架构与路线

> prd 只写**当前与将来**（0.0.15 R15-0 重写，2026-09-30）。条款全文在 [spec.md](spec.md)（代码注释引用的 `[S-9]`、`[W-16]` 等编号都在那里，只增不改）；构想在 [plans/ideas.md](plans/ideas.md)；已发布版本的计划、叙事与证据在 [archive/](archive/)；目录归属见 [ARCHITECTURE.md](ARCHITECTURE.md)。新知识和决定先写进这里再做别的。

## 1. 命题与产品

- **产品**：一个文件 `unisacc.com`（APE），在 {Linux, macOS, Windows} × {x86-64, arm64} 上运行，自己写出六个目标的可执行文件（ELF、Mach-O、PE；不用汇编器、链接器），并能自举：宿主构建的 `unisacc-seed.com` → 种子自建 → 再自建，三个 sha256 相等（N22）。两代产品只叫 `unisacc-seed.com` 与 `unisacc.com`。
- **语言**：C99 = ISO/IEC 9899:1999 + TC1–TC3（WG14 N1256）。覆盖度来自逐条款账本 [tests/c99/clauses.tsv](tests/c99/clauses.tsv)（门禁 `c99-ledger`，README 表由账本生成），不作主张；缺口在 README 限制表逐条点名。
- **方法**：编译器里每个表状决策（词法类、预处理指令、优先级、类型、指令选择、ABI、窥孔……）由**构造**出来的小整数网络回答，权重从真值表派生、在表的全部定义域上**穷举验证**（不是训练）；结构性粘合是经典代码。两条路线：C 参考编译器（`unisacc.c` + `src/`，行为裁判与回退）与模型产品（`exec/` 各阶段网络 + 通用执行器），逐字节对拍（`exec-chain`、`closure`、`stages`、difftest 两侧）。
- **交付物**：签名的 `unisacc.com` 与公证的 macOS dmg（公开资产只有这两个，主人 2026-09-30 裁定）；中间形式 `.tape`/`.tapebin`（[docs/tapebin-v1.md](docs/tapebin-v1.md)，内容寻址、C/Python/产品三方编码逐字节同）。

## 2. 当前版本

### 已发布版本身份（README 只放用户入口；本表按版本一行，数字以各版验收记录为准）

| 版本 | 身份 / 证据 |
|---|---|
| v0.0.12 | Version 0.0.12; product closure sealed at `c4d667e` (candidate GHCR digest in `release/candidate.json`); published `unisacc.com` is Authenticode-signed: 1,170,368 B, SHA-256 `fb607af59388aa20cbd9f536d6b781f3cf83a6a0294c0e4e69e844faba2736cb` (unsigned gate candidate 1,154,589 B, SHA-256 `df8cc9b4a3d997ea9e33bbdbaefeecb1a11d10dd98f529a4fb8d87f2e21e7f7a`); macOS app/dmg Developer ID signed and notarized; the same candidate bytes ran the demo suite on six hosted runners (lnx/osx/win × x86_64/arm64) and full suites on four native architectures; receipt [archive/research/r12/r12-release-acceptance.json](archive/research/r12/r12-release-acceptance.json) |
| v0.0.11 | Version 0.0.11; product closure sealed at `8b5abc9` (candidate GHCR digest in `release/candidate.json`); published `unisacc.com` is Authenticode-signed: 1,170,384 B, SHA-256 `e86cc61c2d9ee8abd511f5d6b5c0f114a01a34dbe6146bb411e3f204c65d792a` (unsigned gate candidate 1,154,605 B, SHA-256 `6a3dfce28aa05ca474442ebe9e6fc4d07f4da7c15d1d3b5a6c21e91290f920c8`); macOS app/dmg Developer ID signed and notarized; library bodies on demand by default (`-fno-trim-libc` opts out); receipt [archive/research/r11/r11-release-acceptance.json](archive/research/r11/r11-release-acceptance.json) |
| v0.0.10 | Version 0.0.10; product closure sealed at `fdff9c5`, release source `ae6d512`; published `unisacc.com` is Authenticode-signed: 1,168,488 B, SHA-256 `f6e8e090a5288583389bdbbb5674f7e0fbb717baf13fa600f8074c77d1acdb2e` (unsigned gate candidate 1,152,711 B, SHA-256 `4ba24140a1307a34216efd0f2e7c892a92990ee2729a8774a58e0a4fe2315002`; model package identical); 24 deployed networks each `network = table` over the whole domain; in-process `libunisacc` (contexts, symbol injection, typed V2/V3 signatures with model-certified carriers, callbacks, USLCALL3 source-origin aliases); local release gate 332 suites rc 0, Linux arm64 guest and Windows/x86_64 guest smoke recorded in the [release receipt](archive/research/r10/r10-release-acceptance.json) |
| v0.0.9 与更早 | 见 §7.1 版本沿革 |


### 计划索引（正文在 plans/，prd 只放索引）

| 版本 | 文件 | 状态 |
|---|---|---|
| v0.0.12 | [archive/plans/v0.0.12.md](archive/plans/v0.0.12.md) — 计划树 R12-0..R12-7 与逐项回执（已发布 2026-09-30，归档） | 已发布 |
| v0.0.13 | [archive/plans/v0.0.13.md](archive/plans/v0.0.13.md) — 计划树 L0–L5/N0–N22 与逐项回执（R13-0 六例、R13-0b 25 条、N22 产物谱系、门禁队列方法）；状态叙事全文见 [archive/prd-history-20260930.md](archive/prd-history-20260930.md) | 已发布 2026-09-30（tag 3af8b36；公开 unisacc.com 75c698ec… 与 dmg；回执 archive/research/r13/r13-release-acceptance.json） |
| v0.0.14 | [archive/plans/v0.0.14.md](archive/plans/v0.0.14.md) — #21 callm、8 个 L1 库体、#line、tapebin v1 端到端、C99 条款账本（语言 96%）、流水线方法债、R14-8 6/8 | 已发布 2026-09-30（tag 75ec4de；公开 unisacc.com c229cebf… 与 dmg；回执 research/r14-release-acceptance.json） |
| v0.0.15 | [archive/plans/v0.0.15.md](archive/plans/v0.0.15.md) — 整理版（主人 2026-09-30）：prd 重写（≤500 行、只写当前与将来）、文件梳理与归档、封装/抽象/复用、论文 A 梳理、剩余问题总结进 0.0.16 | 完成，不发布（全量队列 370/370，产品字节同 0.0.14） |
| v0.0.16 | [plans/v0.0.16.md](plans/v0.0.16.md) — 原定 0.0.15 的功能项：R14-8 余项、续行 __LINE__、复数/三字符组决定、狗粮启动器（POSIX 最小面）、库面、CI 预热 | 草案 |

**产品命名（主人 2026-09-30 提醒，硬规则）**：两代产品只有两个名字——宿主构建的第一代叫 **`unisacc-seed.com`**，由它自举出来的最终产品叫 **`unisacc.com`**（发布物、GHCR 候选、README 与 N22 三阶段的文件名都按此；`unisacc-next.com` 只是构建目录里的中间名，不出仓）。
| v0.1.x | [plans/v0.1.x.md](plans/v0.1.x.md) — 证明侧路线（T2 机器证明、P-2 全走查器、T3、.o、wasm）；v0.1.3 内存安全检查节点（模型推断标注 + 确定性检查器 + 分级编译，论文 D）；v0.2.x 包市场 + agent 友好主线（tapebin 包格式、发布即证明、权限沙箱、官方包） | 草案 |
| v0.0.11 及更早 | 见 §7.1 版本沿革与 §7.2 归档索引 | 已发布 |


<a id="pipeline-design"></a>
## 3. 当前架构：模型流水线（结构、功能与边界）

本节以 `68cdd50` 时的构造器、路由声明和执行器源码为依据；出货版本与本地候选身份见上面的 R9 清单。**本节描述当前设计；条款全文（含早期条款与经典实现规格）在 [spec.md](spec.md)。** 旧的“14 个决策点”不等于当前整条流水线的网络数，旧的“走查器不许模型化”不是当前迁移约束。

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

源码先到 lower，再给 memory 入口传实际 OS 预留地址、容量、argc/argv 和适用的动态导入资源。模型据真实代码/导入长度计算对齐与数据位置，**只生成一次最终绑定的 UNIMEM1**（magic、text/extent/stored/entry 四个 u64 及代码/保存数据）。模型负责布局；宿主 reserve/commit、校验、复制、设置权限、清缓存、进入入口，不解释 C/tape/指令。

预留约 2GB 虚拟区由 OS 选地址，不用 MAP_FIXED；Windows reserve 与原地址 commit 分开。失败和错误地址显式拒绝。`UNISA_MEMORY_TWOPASS=1` 只作同驱动比较基线，不能当默认路线。memorycheck 比同基址镜像、资源/范围失败、loader 边界和 Windows API mock；mock 不是客机实测。当前同身份 calc 五次暖中位为 P2/双遍 205.382ms、P3/双遍 207.770ms、P3/单遍 172.749ms；它是该输入的测量，不外推所有程序。证据见 [集成测量](archive/research/20260928/memory-once-integrated-bench-20260928.json)。

### 3.12 模型包、库与发布边界

P3 在动作共享前缀 `compact_q` 后，将网络编码为 `UNINETB1`：整数用规范 LEB128/zigzag，字符串按长度保留字节，再逐网络 raw-DEFLATE + 原长/CRC32。按路由只解需要的网络；校验与模型解析完成后释放解压缓冲。构建期保留 SHA256 与全域 check-net，CRC32 仅防损坏，不是企业信任。旧 P1/P2 的兼容路径和未知版本拒绝必须保留测试。

共享索引让同一模型内容只保存一份；Q/C 动作、H 参数和 S 字节声明分别计账，解压后逻辑量不与压缩体相加。头文件库源码是另计资源，当前没有全库 AOT 或全系统 libc 转发；macOS FFI 是有边界的宿主能力，不把它说成六平台都可任意调用系统库。执行器、驱动/OS 适配、模板、库和网络均是产品组成，不能只报纯推理核几 KB。

**macOS 发布方案（资格已通过，最终分发验收待做）**：采用 `unisacc.dmg → Unisacc.app → 签名的原生入口 → 同包封存 unisacc.com`，实际编译仍由 `.com` 完成，参数/退出状态应原样传递。先验证 quarantine/Gatekeeper、内层 APE 运行与解包缓存、`-run`/FFI，以及公证扫描能否接受整个 bundle。minicon 实际是先签内层 Mach-O，再签 app、公证并 staple app/dmg；不是给任意载荷加壳便完成信任。本方案若未过，不宣称 `.com` 已获得 Apple 签名，回报具体阻挡并评估同源原生内层，不静默更换编译路线。微软 Authenticode 与 Apple 分发链分别验，参见 R9-6。

### 3.13 验证分层与当前证据

| 要回答的问题 | 证据 | 不能推出什么 |
|---|---|---|
| 网络是否精确实现声明转移 | check-net 全部有限观察，包含缺转移/声明返回 | 声明自身就是 C99 语义 |
| 阶段是否保持参考行为 | 固定 keep、输出/退出/诊断、边界与失败探针 | 任意程序/未测组合全域等价 |
| 产品是否完成实际编译/运行 | `.com` 用户命令、cc 可观察差分、真实应用与自举 | 编译器与平台 cc 所有实现定义相同 |
| 包/宿主是否可靠 | codec、损坏包、资源、内存范围、API 失败 | CRC 已提供企业签名信任 |
| 发布物是否可分发 | 精确 SHA、来源、签后运行、平台和信任回执 | 旧候选门禁适用于新源码/重签字节 |

当前集成候选身份及已验/待验范围以本页R9清单为准；旧候选的完整队列、平台与Apple资格不移植。完整610MB数组C源码仍是未解决规模限制。T1是有限转移/网络等价；全程T2和C语义T3仍为独立证明义务。

<a id="model-function-bytes"></a>
### 3.14 当前模型功能与物理字节账

下面从 `research/model-bytes.json` 生成，按实际封存 `.com` 计。只对阶段/模式作可证的功能归属，不把跨阶段动作任意分摊成“指针占多少字节”。

<!-- model-bytes:begin -->
快照 SHA-256：`df8cc9b4a3d997ea9e33bbdbaefeecb1a11d10dd98f529a4fb8d87f2e21e7f7a`；总计 **1,154,589 B**。

| 物理内容 | 字节 | 占整个 .com |
|---|---:|---:|
| 24 个共享网络体 | 700,980 | 60.71% |
| 平台驱动、APE 启动/加载与对齐（混合账） | 251,360 | 21.77% |
| 21 份 C 头文件/库实现源码 | 123,856 | 10.73% |
| 两 ISA 通用推理执行核资源 | 15,520 | 1.34% |
| 目标预定义宏声明资源 | 321 | 0.03% |
| 目录、记录头与资源键 | 62,536 | 5.42% |
| 尾部 | 16 | 0.00% |

| 模型阶段 / 具体功能 | 物理模型数 | 模型体 B | 占 .com | 阶段行引用数 |
|---|---:|---:|---:|---:|
| `e2`：预处理、目标预定义宏与位置模式 | 2 | 49,208 | 4.26% | 126 |
| `e1`：词法与 token/位置输出 | 1 | 27,764 | 2.40% | 120 |
| `e3`：解析、类型/作用域、tape、错误与警告 | 2 | 283,279 | 24.54% | 216 |
| `e4`：O1/O2 优化 | 2 | 14,404 | 1.25% | 144 |
| `nativeabi`：宿主ABI carrier认证（原始类型图→目标载体证书） | 1 | 15,976 | 1.38% | 6 |
| `prune`：函数可达闭包与保守原文剪枝（数据/地址根保留） | 1 | 9,103 | 0.79% | 144 |
| `lower`：ABI、调用、目标指令 lowering 与数据布局 | 6 | 132,839 | 11.51% | 144 |
| `elf`：目标指令编码及 ELF/Mach-O/PE 镜像写出 | 6 | 118,347 | 10.25% | 78 |
| `tokenpp`：公开 token 路线的预处理 | 1 | 11,443 | 0.99% | 1 |
| `tokenlex`：公开 token 路线的词法输出 | 1 | 4,477 | 0.39% | 1 |
| `units`：多文件分帧与文件级 static 隔离 | 1 | 34,140 | 2.96% | 108 |

下表是二进制网络的压缩前拆分；与物理压缩体不可相加，百分比为相对整包大小而非物理占比：

| 网络记录 / 含义 | 字节 | 占整个 .com |
|---|---:|---:|
| `H`：阈值/选择网络参数记录 | 984,396 | 85.26% |
| `Q`：动作序列声明（包含编译模板动作） | 954,809 | 82.70% |
| `S`：字节字符串声明 | 18,805 | 1.63% |
| `N`：网络头记录 | 485 | 0.04% |
| `C`：动作序列共享前缀声明 | 453,436 | 39.27% |
<!-- model-bytes:end -->

两个 e3 分别是错误与错误+告警变体；十二个 e2 是六目标的普通/位置变体。共享物理模型数与阶段行数以本节的生成账本为准（见上方 `model-bytes` 区：24 个共享网络体、1,088 条阶段行），本段不重述；此处曾写「33 个共享物理模型被 1,082 条阶段行引用」，与同一文件上方的生成区不符。引用次数不是物理份数。压缩前 Q/C 是动作与共享声明，H 是阈值/选择参数；三个数字不与压缩模型体相加。21 份头/库源码与两 ISA 核资源另计；混合平台驱动缺独立 link-map，不虚构其 libc/启动/OS 子项比例。当前产物大小相对已发布 v0.0.8 的 5,388,402 B 小约 77.11%，签名后字节变化另记；产物与包的当前字节同样以生成账本为准（本段曾写 1,233,236 B 与 985,172 B，那是 v0.0.9 的值）。

---

**逐阶段结构计数口径**：论文 §1.3 的结构快照见 [pipeline-structure-20260928.json](archive/research/20260928/pipeline-structure-20260928.json)，绑定旧01e5c1d9测量基线；当前d4f7d302静态结构见[r9-pipeline-structure-prune.json](archive/research/r9/r9-pipeline-structure-prune.json)，旧calc动态计数不得外推。静态动作数按每个序列展开 Q/C 前缀后的动作次数求和；另记包中实际保存的后缀动作数。它们不等于一次程序运行的动态动作数，也不是动作操作码种类数。声明返回按银行数与键成员数另记；O1/O2 不能共用一组结构数字。结构审计另附 calc 动态计数：cc-unisacc 提供插桩/原始输出，cdx 用同一二进制独立复跑，五阶段计数一致；输入、模型与运行时 SHA、O0/run 与 memory 路由及日志归档到 research/pipeline-counts-20260928/。它不是出货汇编内核的性能测量，结构表中的 O2 网络未参与该次 O0 运行。


## 4. 质量体系

- **门禁**：`tests/gate.sh --list [--com]`（约 365 项）；发布验收只用 `tests/release.sh --com`（jobs 4、window 50、经 `tests/term.sh`、`SEED_DIR` 必填、STRICT=1），每项各绿一次；冷建网络的两项先预热（R16-5 自动化）。CI（`ci.yml` 跑 `all.sh`）是第二意见，Intel runner 仅供参考。每次运行 ≤60 s。
- **清单契约**：knownfail/knownwrong 文件一行一个名字，列名即 known、转同即 revived（红，必须删行）：difftest(.com)、pyfront（Python 对照组缺口）、chain（模型与参考镜像差异）、diag(.com)、corpus（非 C99 输入）、c99。没有隐式排除——任何“覆盖名单”都要配清单（R14-8 教训）。
- **账本**：C99 条款账本（覆盖）、模型字节账（`research/model-bytes.json`）、裁判登记（`research/referee.tsv`）；README/ARCHITECTURE 不重抄账本数字（docs 门禁断言）。
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

- **权威文档**：[release/RELEASE-PIPELINE.md](release/RELEASE-PIPELINE.md)（§0–8：冻结源、UA、P3、门禁队列、客机、CI/GHCR、Apple、Windows 签名、发布；每步坑与判据）；契约 [release/README.md](release/README.md)；策略 `release/signing-policy.json`；封存 `release/candidate.json`；工作流 `.github/workflows/{release-check,windows-signing,ci}.yml`；本机技能 `~/.claude/skills/unisacc-release-pipeline`。
- **与 minicon 的对应**：minicon 在 CI 构建候选并推 GHCR，我们**本机构建**（一个能写出六目标的编译器，CI 只测不建，见 CLAUDE.md）再推 GHCR；签名/运行验证都按摘要拉取，同 minicon；docs 提交不打断签名（按产品源闭包比较）。
- **已落地**（0.0.10/0.0.11 两次实发）：push→签完约 4–5 分钟；GHCR 封存与 release-check 实跑；qualification→company；Apple 公证。**未落地**（R12-0/R12-7）：发布本身仍是本机 `gh release edit` 而非工作流；qualification 的本地演练脚本；单架构原生 runner 跑完整套件（R12-3）。
- **每片重封**：开发期每次产品闭包变化都要 `seal_candidate.sh <ver>-dev`，否则 release-check 候选作业红（0.0.11 前三次 push 的教训）。


## 5. 路线

| 版本 | 主线 |
|---|---|
| **v0.0.15**（完成，不发布） | 整理：prd 重写、文件清单与归档、封装/复用、论文 A、剩余问题总结 → [archive/plans/v0.0.15.md](archive/plans/v0.0.15.md) |
| v0.0.16 | 功能承接与问题总表；**工具链起步（R16-7，主人严肃需求）：`-c` 目标文件 → 链接器 → 汇编器 → ar/nm**，与系统工具互操作 → [plans/v0.0.16.md](plans/v0.0.16.md) |
| v0.1.x | 证明侧（T2、P-2、T3、.o、wasm）、tapebin 平台化、内存安全检查节点（论文 D）→ [plans/v0.1.x.md](plans/v0.1.x.md) |
| v0.2.x | 包市场 + agent 友好（tapebin 包、发布即证明、权限沙箱、官方包）→ 同上 |

## 6. 未解决问题

全表（正确性 / 诊断 / C99 覆盖 / 反复顺延的架构项 / 流程 / 论文，每条带来源与优先级）在 [plans/v0.0.16.md §剩余问题总表](plans/v0.0.16.md)。摘要：
- 正确性：全局初始化器里的复合字面量镜像不同（A1）、续行前 token 的 `__LINE__` 多 1（A2）。
- 诊断：产品 9 类、参考 3 类拒绝仍无位置（B1/B2）。
- C99：复数、三字符组未定；库条款 191 条 unsupported；POSIX 最小面缺（狗粮启动器前提）。
- 自 0.0.11 起每版顺延：通用 BANK/nativeabi、Windows SEH、us_eval/us_reload、网络裁判登记、Windows 全套件 runner、发布工作流化——0.0.16 须逐项决定“做一片”或降级。

## 7. 归档索引


prd 只描述当前与将来；过程记录、旧计划与历史数字按时间封存在 `archive/`：

- [prd-history.md](archive/prd-history.md)：v1–v3.4 逐版条目与 14 阶段前的模型总表。
- [s17-migration-log-20260928.md](archive/s17-migration-log-20260928.md)：2026-09-26–28 模型化迁移逐片日志。
- [r9-integration-history-20260928.md](archive/r9-integration-history-20260928.md)：R9 集成史。
- [prd-r9-r10-receipts-20260929.md](archive/prd-r9-r10-receipts-20260929.md)：v0.0.9/R9 状态、v0.0.10 全计划、R9/R10 逐片回执与发布收口、2026-09-27 巡查（5.6/5.8/5.10）。
- [prd-history-20260929.md](archive/prd-history-20260929.md)：本次第二轮清理移出的 §0.3 早期交付边界、§5.5 完成度盘点 S-1..S-17、§5.7/5.9 巡查记录、v0.0.11 逐项回执全文、附录 7.1/7.2/7.4。
- [prd-findings-20260929.md](archive/prd-findings-20260929.md)：§6 实验发现 E-编号全表（论文引用按编号在此解析）。
