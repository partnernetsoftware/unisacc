# unisacc —— 当前产品规格与计划

> **配套**：[`prd.tree.md`](prd.tree.md) 树+DAG，开工用 · [`prd.map.md`](prd.map.md) 记忆宫殿，记全局用 · [`research/prior-art.md`](research/prior-art.md) 先行研究，写论文前必读 · `archive/` 历史版本
>
> **结构**：§0 的发布清单与模型流水线是当前规格；§1–§7 保留稳定条款和经典/种子阶段的设计与实验。日期、模式和冻结身份决定证据适用范围；S-17 长篇迁移记录已归档。

## 0. 导读

先读[当前模型流水线设计](#pipeline-design)：每步的输入/输出、网络控制结构、规则来源、功能、限制和验证。当前本地功能/权重字节账见[模型功能与物理字节账](#model-function-bytes)。


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

### v0.0.12（2026-09-30 发布，源 `c4d667e`）

- **发布**：https://github.com/partnernetsoftware/unisacc/releases/tag/v0.0.12 ；签后 `unisacc.com` fb607af5…（1,170,368 B），未签候选 df8cc9b4…（1,154,589 B）；最终树全量 350/350；验收 `research/r12-release-acceptance.json`。
- **多架构结果**：六托管 runner（lnx/osx/win × x86_64/arm64）对同一候选跑演示套件六格全绿（`research/r12-demo-matrix-c4d667e.json`）；全套件 ci 矩阵 lnx/x86_64、lnx/arm64、osx/arm64 全绿，osx/x86_64（Intel）163/181（18 项为 Python 参考路径的时间预算）；本机六目标 8/8 与 Windows 双自举。
- **[v]**：R12-0 测试债（清点/入门禁/usage/exec-formats/linuxbridgecheck/ua_ref 竞态/--list/bindingcheck 拆分）、R12-1 ① BANK 可执行部分（两 ISA 19 夹具）、R12-3 ③ 矩阵、R12-4 ③④（external 8/18、计数单一来源）、R12-5 ④ FX-6 量化负结果。**[-] 顺延 0.0.13**：BANK 表接线与宿主、R12-2、Windows 全套件 runner、网络裁判登记、R12-6 文档/规格/论文余项、FX-5。回执全文 `plans/v0.0.12.md`。

### 已发布版本身份（README 只放用户入口；本表按版本一行，数字以各版验收记录为准）

| 版本 | 身份 / 证据 |
|---|---|
| v0.0.12 | Version 0.0.12; product closure sealed at `c4d667e` (candidate GHCR digest in `release/candidate.json`); published `unisacc.com` is Authenticode-signed: 1,170,368 B, SHA-256 `fb607af59388aa20cbd9f536d6b781f3cf83a6a0294c0e4e69e844faba2736cb` (unsigned gate candidate 1,154,589 B, SHA-256 `df8cc9b4a3d997ea9e33bbdbaefeecb1a11d10dd98f529a4fb8d87f2e21e7f7a`); macOS app/dmg Developer ID signed and notarized; the same candidate bytes ran the demo suite on six hosted runners (lnx/osx/win × x86_64/arm64) and full suites on four native architectures; receipt [research/r12-release-acceptance.json](research/r12-release-acceptance.json) |
| v0.0.11 | Version 0.0.11; product closure sealed at `8b5abc9` (candidate GHCR digest in `release/candidate.json`); published `unisacc.com` is Authenticode-signed: 1,170,384 B, SHA-256 `e86cc61c2d9ee8abd511f5d6b5c0f114a01a34dbe6146bb411e3f204c65d792a` (unsigned gate candidate 1,154,605 B, SHA-256 `6a3dfce28aa05ca474442ebe9e6fc4d07f4da7c15d1d3b5a6c21e91290f920c8`); macOS app/dmg Developer ID signed and notarized; library bodies on demand by default (`-fno-trim-libc` opts out); receipt [research/r11-release-acceptance.json](research/r11-release-acceptance.json) |
| v0.0.10 | Version 0.0.10; product closure sealed at `fdff9c5`, release source `ae6d512`; published `unisacc.com` is Authenticode-signed: 1,168,488 B, SHA-256 `f6e8e090a5288583389bdbbb5674f7e0fbb717baf13fa600f8074c77d1acdb2e` (unsigned gate candidate 1,152,711 B, SHA-256 `4ba24140a1307a34216efd0f2e7c892a92990ee2729a8774a58e0a4fe2315002`; model package identical); 24 deployed networks each `network = table` over the whole domain; in-process `libunisacc` (contexts, symbol injection, typed V2/V3 signatures with model-certified carriers, callbacks, USLCALL3 source-origin aliases); local release gate 332 suites rc 0, Linux arm64 guest and Windows/x86_64 guest smoke recorded in the [release receipt](research/r10-release-acceptance.json) |
| v0.0.9 与更早 | 见 §7.1 版本沿革 |

### 计划索引（正文在 plans/，prd 只放索引）

| 版本 | 文件 | 状态 |
|---|---|---|
| v0.0.12 | [archive/plans/v0.0.12.md](archive/plans/v0.0.12.md) — 计划树 R12-0..R12-7 与逐项回执（已发布 2026-09-30，归档） | 已发布 |
| v0.0.13 | [plans/v0.0.13.md](plans/v0.0.13.md) — 草案：决策完备性主图（R13-1）、六目标零 #ifdef 门禁化（R13-2）、FX-6 结论进论文（R13-3）、0.0.12 顺延承接（BANK 表接线、R12-2、Windows 全套件 runner、网络裁判登记、文档/规格、FX-5） | 现行；2026-09-30 L0 五个 P0 与 N7/N10/N16 的 20 余条外部缺陷两侧修完，候选 0.0.13-dev7（b0ac8366…）六 runner 六格绿、miniz deflate/inflate 往返（23/30/34）在产品上通过，com 清单 29→8 行、fb12.knownfail 剩 27/28；N14 决定：FX-5 L1 自研库体；N15 收完（dsh ea1f1cf/2bd1865 + fd5f759 补修）；N22 定点在 dev9 上成立（三阶段 sha 全等 c09d32bc… = 候选本身）；候选 0.0.13-dev13 构建 92731d72…（a28c5bb，未封；dev12 ef445536… 已封推）：#24/#27/#28/#25/#11、01/02/06、N17a `__LINE__` 在产品上转绿（com 清单 2 行：14（E1 折叠待做）/21（0.0.14）），fb12.knownfail 清空，产品 difftest/difftest_o 四片 wrong 0，demo 18 通过；multi 门禁修好（dsh ed02425）且参考侧 #31 可达性线性（c23f12b）；N17b `__FILE__` 两侧已做、随 dev12 上产品；dev12 上 07/08b/29/31/n17-file 转绿删行；首轮全量队列在 dev12 上暴露并修掉三处回归（static 导出链接、x86 地址阶段零默认范围、fat 的 Python 路线清单）；第二轮全量队列（359/359）归因 16 红并修掉 4 处回归（参考 -I 目录 60 字节截断、E2 autoinc 缺头负行号、layout-facts 的 BASE 常量、cdx parenfold 的 --warnings 变体）；候选 0.0.13-dev14 构建 94ea9c76…（8632a40 同源，wt25 干净工作树；fb12-multi 10/10、demo 18、驱动/E2/e4/layout 8 套件绿）作为最终候选，最终全量队列（显式 UA=同源、MODEL_COM=dev14、SEED_DIR=dev14 种子）在 /tmp/wt26 上跑中，其后 N22 三阶段、封版、签名、发布；dsh 已退出 0.0.13（主人裁定）；剩：#14 产品侧（cdx）、comboot 片契约（dsh）、N22 定点复跑（种子 sha = 候选 sha 92731d72 已证）、签名/发布链 |

**产品命名（主人 2026-09-30 提醒，硬规则）**：两代产品只有两个名字——宿主构建的第一代叫 **`unisacc-seed.com`**，由它自举出来的最终产品叫 **`unisacc.com`**（发布物、GHCR 候选、README 与 N22 三阶段的文件名都按此；`unisacc-next.com` 只是构建目录里的中间名，不出仓）。
| v0.1.x | [plans/v0.1.x.md](plans/v0.1.x.md) — 证明侧路线（T2 机器证明、P-2 全走查器、T3、.o、wasm） | 草案 |
| v0.0.11 及更早 | 见 §7.1 版本沿革与 §7.2 归档索引 | 已发布 |

### 0.1 条款编号

每条规范性条款带稳定 ID，供实现、形式化证明、验收脚本三方引用。**ID 一旦分配不再改动**，废弃条款保留编号并标记。

| 前缀 | 域 | 前缀 | 域 |
|---|---|---|---|
| `T` | 命题 | `O` | Oracle 接缝 |
| `D` | 确定性 | `W` | 前端走查 |
| `F` | 超级拟合 | `TP` | Tape 与 VM |
| `P` | 证明义务 | `C` | Catalog |
| `U` | CLI | `L` | Lowering |
| `K` | kernel | `X` | 目标机执行 |
| `N` | TableNet | `I` | 镜像 |
| `S` | StageNet/combo | `A` | 验收 |
| `V` | 词表 | `B` | 预算 |
| `G` | Gold | `E` | **实验发现（§6）** |
| `TR` | 训练 | | |
| `Q` | UNS1/量化 | | |

### 0.2 术语

| 术语 | 定义 |
|---|---|
| **stage（阶段）** | 当前产品的字节流变换步骤，见 §0.3 路由表；经典 gold 的有限决策点清单另见 §3.0，二者不是同一模型粒度 |
| **key** | 某阶段的一个输入元组，取自该阶段声明的字段词表 |
| **K_s** | 阶段 s 的 key 全域 = 各字段词表的完整笛卡尔积 |
| **gold** | 阶段 s 上的参考标签函数 `G_s : K_s → Class_s`，全函数 |
| **FULL gold** | 在整个 K_s 上评估，而非在批次或子集上 |
| **Oracle** | 网络与 gold 之间的唯一分发点，见 §4.1 |
| **tape** | 目标无关的通用指令流，§4.3 |
| **TargetProgram** | tape 经 lowering 后得到的、携带目标事实的指令流，§4.5 |
| **fold** | 把同一份 tape 铺到 6 个目标各自执行并比对，§5.1 |
| **kit** | `unisa ship` 产出的交付包 |

### 0.3 当前模型流水线：结构、功能与边界

本节以 `68cdd50` 时的构造器、路由声明和执行器源码为依据；出货版本与本地候选身份见上面的 R9 清单。**本节描述当前设计；§1–§7 保留的早期条款、训练实验和经典实现规格按其注明的历史范围阅读。** 旧的“14 个决策点”不等于当前整条流水线的网络数，旧的“走查器不许模型化”不是当前迁移约束。

**阅读顺序**：共同模型结构 → 路由总表 → 各阶段 → 打包/装载 → 字节账 → 验证与现状。详细源码边界另见 [规则来源](exec/rules.md)、[执行机制](exec/c/CORE.md)、[包协议](exec/c/PACKAGE.md)。迁移过程及原始失败记录完整保留在 [S-17 迁移档案](archive/s17-migration-log-20260928.md)，不再混入当前规格。

#### 0.3.1 从声明到网络：到底替换了什么

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

#### 0.3.2 路由、数据协议与分支

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

#### 0.3.3 E2：预处理模型

- **输入/输出**：源文件字节；普通输出 `pp.text`，产品输出带文本长度、强制/自动包含信息、续行与包含位置的 `UNIPP1` 封装。`-D/-U/-include/-nostdinc` 由 CLI 资源传入，不由 driver 改写源码。
- **结构**：先处理 shebang、续行、注释与字面量保护，再进行自动头选择、指令处理和宏重扫。宏记录存名字、正文、参数、可见区间、历史链和隐藏标记；输入帧承载参数/替换正文，显式栈承载条件表达式归约。包含文件拼入后重新规范化。
- **规则来源**：gold `pp.tsv` 的指令与 defined 判定；[exec/pp](exec/pp/rules.md) 的 text/autoinc/macro/directive/expression/reduce/hash/rescan/pragma/location 声明；目标 `predefines.tsv`；头文本为命名资源。gold 的小表只是指令判定的一部分，不等于整个预处理器。
- **功能/限制**：对象/函数/变参宏、参数展开、字符串化、覆盖的拼接、条件表达式、包含搜索、macro-stack pragma、位置跟踪。参数上限 MAXP=8；一般标点/字面量拼接等尚有限制，E2 自身不生成完整 file:line:col 诊断。
- **判据**：macro/literal/pragma/location 专项、分片参考字节对照、坏封装与失败路径；位置传输还须经 E1/E3 核对。构造器读取自动头导出名单并装配控制，不能写成这些编排已消失。

#### 0.3.4 E1：词法模型

- **输入/输出**：预处理字节及映射 → typed token 行流，保留标识符/数字/字符串原始拼写。located 输出 `UNITOK1`，携带原始预处理封装与每 token 的源偏移；EOF 是显式观察。
- **结构**：字节类/前瞻类分派，标识符与运算符 trie、数字/字面量控制状态；MARK/JUMP/跨度动作实现最长前缀，栈处理声明中的平衡跳过。数值求值不是词法阶段的职责。
- **来源**：gold lex、parse token 词汇，lexcls/lexword/typekw，以及 [词法规则](exec/lex/rules.md) 的 number/literal/ident/entry/output/spelling/count/location。trie 和连接由构造器生成，不运行经典 lexer 补答案。
- **功能/限制**：关键字、类型词、普通标识符、数字拼写、字符/字符串前缀、注释、最长匹配标点及声明中的 UCN 路径；规则存在不等于完整 Unicode/C99 保证。非法字节和坏封装明确拒绝。
- **判据**：compare、positioncheck、locationcheck；同时比实际网络与通用动作参考，不把旧独立 lex 实验误认为生产构造路线。

#### 0.3.5 多单元：隔离与合并模型

每个源文件用全新的 E2/E1 机器状态运行，宏与 include guard 不跨单元。driver 只读文件、封装长度/文件名；模型验证帧、扫描并隔离文件级 static 与标签，合并 token，输出 `UNITOK2` 的单位/位置目录。最多 64 帧。E3 在程序范围共享符号、签名、字符串池，并保留声明的单位归属。不是把源文件简单拼接，也不是系统 `.o` 链接。多源 `-E`/token CLI 当前拒绝；部分 aggregate static 声明形式有命名拒绝。判据见 multicheck、multiwarningcheck 与 unitlocationcheck，检查隔离、输入顺序及错误归属。

#### 0.3.6 E3：解析、类型、存储与 tape 模型

- **输入/输出**：UNITOK1/UNITOK2 → `tape.text` 与独立诊断缓冲。tape 包含标签、指令、字符串/BSS、初始化与入口段；是中间协议，不是宿主机器代码。
- **控制结构**：构造器用状态 label、边上动作、压续点的 call 和栈顶 RET 组织子过程。token-reader 把行流解码成 token 类、intern 名字、跨度/数值；索引内存存符号、作用域撤销、维度、成员、函数签名、标签和字符串池。共享优先级梯子进行表达式调度，不是一次匹配有限个完整程序。
- **事实与控制来源**：prec/binsel/irsel/type/tyinfo/pfconv 提供优先级、ALU 拼写、共同/结果类型、宽度及转换。`exec/parse2` 的 declaration/scope/operator/ladder/control/member/call/initializer/statics/vla/constexpr 等 TSV 提供控制；tape/text 模板负责输出。`exec/parse/gen.py` 仍被导入作共享组装器；intrinsic 名单来自种子声明。手写构造编排仍在，不称已有通用文法编译器。
- **功能**：声明与作用域、typedef、整数/浮点/指针、数组、函数指针/签名、struct/union/enum、成员/位域/柔性数组、常量与 VLA 维度、初始化、表达式/转换/更新、sizeof、语句控制/标签/调用及 intrinsic。每项以保留探针中的具体形状为支持范围，不能从名字存在推断任意组合都支持。
- **浮点字面量**：数字解码通过 32 位 limb 动作形成精确比例并最近偶数舍入，包含次正规数；不是宿主浮点解析回调。160-limb 上限等构造容量单列。
- **位置、错误、告警**：模型将 token 偏移关联到原文件/行列/上下文；普通版启用 errors，告警版再启用 return/int-conversion/unused/format 规则。部分已映射语法错误允许顶层平衡恢复；未覆盖形式定位后停止。错误非零不得发布 tape。告警规则不称完整控制流/printf 规范分析。
- **限制**：token 字节跨度 <2^26，数组 rank≤8、struct 池≤128，签名/shape/enum 池有限；部分声明符、类型形状、初始化、struct 返回和异型条件分支仍有拒绝。静态容量与理论无界存储分开。
- **判据**：固定 probe/keep、floatconst 的系统 cc/表/网络核对、scope/enum/location/diagnostic/error/warning 专项和完整 self-source tape。`parse2/selfcheck.sh` 使用 `.tbl`，其成功不能单独冒称网络自源证明；产品网络与完整路线另有门禁。

#### 0.3.7 E4：优化模型

输入/输出均为 tape，O0 不调用；O1/O2 使用独立控制网络。START 加载 opinfo/peep 事实；扫描、解析、基本块、读写、活跃性、融合和轮次来自 `exec/opt` 的 TSV。O1 在最多四轮内检查有限直线范围，把软件栈保存/恢复替换成 mov。O2 加入基本块/标签、活跃性重扫、死寄存器携值、局部读融合、相邻指令 peep 和复制/目的重定向。它是有界保守改写，不是全程序优化或完整寄存器分配；算法被编码进规则与动作，并未凭网络构造消除算法信息。判据是同一 O0 tape 经模型后与参考 O1/O2 的 tape 字节相等，另有独立执行差分。来源/门禁见 [opt/gen.py](exec/opt/gen.py)、exec-e4/exec-e4self。

#### 0.3.7a prune：函数可达闭包模型

输入/输出为tape原文。模型识别受支持的函数prologue，建立调用、跳转及可能贯穿边，从入口、main/__init和代码取址根求闭包；保留所有数据语句和可达原始字节跨度。一个共享阈值网络由 `exec/prune/gen.py` 构造，执行核没有剪枝原语。host ABI、可达间接调用、超2MiB或目录容量、未知/歧义与非LF等输入完整保留。参考分别为 `src/tapeprune.c` 与 `unisa/prune.py`；四个PRUNE_PART门禁对拍，不把保守fallback说成完成全程序分析。

#### 0.3.8 lower：ABI、目标指令与稀疏数据模型

- **输入/输出**：tape → `target.text`，含目标、实际保存的数据、符号、逻辑数据长/BSS/重定位，再接目标寄存器指令、标签、元数据。
- **结构**：数据扫描与 code 扫描分开。zero-last 排布和 mod-8 对齐由模型动作执行；虚拟长度与保存前缀分离，BSS/对齐推进逻辑长度，不逐字节存零。宽索引命名空间避免大数据偏移撞上其他表。
- **来源/功能**：regmap/enc/abi/reloc 事实、tape SHAPE、scratch/WinAPI 常量与 code-abi-sources/code-syscall 声明。模型完成入口、参数、软件栈、寄存器映射、POSIX syscall、Darwin carry 与 hostcall/hostaddr、Windows 保存/恢复与返回转换。ARM 窄化/立即数融合及有界 dead-after 扫描也在声明动作中。
- **限制/判据**：约 2GB 数据/相对寻址边界，不支持的参数形状拒绝，none WinAPI 导入不能当正常导入。普通/ARM/Windows lowering 与 sparsecheck 分开验，稀疏 610MB 布局成功不等于编译 610MB 数组源码已经成功。

#### 0.3.9 E5：指令编码模型

编码与镜像写出在实际 `elf` 阶段同一网络中连接；逻辑区分不代表磁盘有额外一份模型。x86 解析寄存器/操作数/元数据，执行 REX/ModRM/SIB/位移、整数/FP/syscall/Windows 设置与地址打包；先长分支布局再迭代缩短，call 保持 rel32。ARM 两遍测量/解析标签，无分支缩短，call 位移从 BL 实际位置算；执行 MOVZ/MOVK、整数/FP、存取、软件栈、地址和各 OS gate。

opcode、NUM、FP/存取/relocation 字段来自 catalog/emit 的声明；扫描、位打包、分支和地址算法来自 `exec/enc` 控制 TSV。**WINARGS_BODY 是明确保留的机器代码模板**，其中命令行解析还不是 δ；模板大小计入产物。重复/未定义标签、非法元数据、范围越界和破坏 scratch 的别名必须拒绝。check/armcheck/x86wincheck/armwincheck 既比参考字节，也保留手算/独立解码及边界探针；全域转移等价不等于所有 FP/整数指令语义已形式证明。

#### 0.3.10 E6：镜像与内部完整性模型

共享动作解码 payload、应用重定位和裁掉文件零尾；实际内存范围仍保留。ELF 模型计算 ELF64 头、入口、两个 PT_LOAD，区分 p_filesz/p_memsz；Mach-O 模型生成段/section/dyld/LINKEDIT，通过 SHA256 δ 计算页哈希和 ad-hoc CodeDirectory/SuperBlob；PE 模型生成 PE32+、导入、cookie/load-config 与 DIR64 排序去重/页分组。不在运行时调用 Python image writer。

格式常量/模板来自 `unisa.image` 与声明文件；格式控制也有 TSV。Mach-O **ad-hoc 签名只满足镜像运行格式要求，不是 Developer ID、企业发行签名或公证**。PE 重定位的二次插入排序仍是规模限制。imagecheck/armimagecheck/machocheck/shacheck/pecheck 验格式与字节；客机运行、自举及签后运行是另一组证据，不能用“能写六目标”代替“本轮实跑六目标”。

#### 0.3.11 `-run`：一次绑定与宿主装载

源码先到 lower，再给 memory 入口传实际 OS 预留地址、容量、argc/argv 和适用的动态导入资源。模型据真实代码/导入长度计算对齐与数据位置，**只生成一次最终绑定的 UNIMEM1**（magic、text/extent/stored/entry 四个 u64 及代码/保存数据）。模型负责布局；宿主 reserve/commit、校验、复制、设置权限、清缓存、进入入口，不解释 C/tape/指令。

预留约 2GB 虚拟区由 OS 选地址，不用 MAP_FIXED；Windows reserve 与原地址 commit 分开。失败和错误地址显式拒绝。`UNISA_MEMORY_TWOPASS=1` 只作同驱动比较基线，不能当默认路线。memorycheck 比同基址镜像、资源/范围失败、loader 边界和 Windows API mock；mock 不是客机实测。当前同身份 calc 五次暖中位为 P2/双遍 205.382ms、P3/双遍 207.770ms、P3/单遍 172.749ms；它是该输入的测量，不外推所有程序。证据见 [集成测量](research/memory-once-integrated-bench-20260928.json)。

#### 0.3.12 模型包、库与发布边界

P3 在动作共享前缀 `compact_q` 后，将网络编码为 `UNINETB1`：整数用规范 LEB128/zigzag，字符串按长度保留字节，再逐网络 raw-DEFLATE + 原长/CRC32。按路由只解需要的网络；校验与模型解析完成后释放解压缓冲。构建期保留 SHA256 与全域 check-net，CRC32 仅防损坏，不是企业信任。旧 P1/P2 的兼容路径和未知版本拒绝必须保留测试。

共享索引让同一模型内容只保存一份；Q/C 动作、H 参数和 S 字节声明分别计账，解压后逻辑量不与压缩体相加。头文件库源码是另计资源，当前没有全库 AOT 或全系统 libc 转发；macOS FFI 是有边界的宿主能力，不把它说成六平台都可任意调用系统库。执行器、驱动/OS 适配、模板、库和网络均是产品组成，不能只报纯推理核几 KB。

**macOS 发布方案（资格已通过，最终分发验收待做）**：采用 `unisacc.dmg → Unisacc.app → 签名的原生入口 → 同包封存 unisacc.com`，实际编译仍由 `.com` 完成，参数/退出状态应原样传递。先验证 quarantine/Gatekeeper、内层 APE 运行与解包缓存、`-run`/FFI，以及公证扫描能否接受整个 bundle。minicon 实际是先签内层 Mach-O，再签 app、公证并 staple app/dmg；不是给任意载荷加壳便完成信任。本方案若未过，不宣称 `.com` 已获得 Apple 签名，回报具体阻挡并评估同源原生内层，不静默更换编译路线。微软 Authenticode 与 Apple 分发链分别验，参见 R9-6。

#### 0.3.13 验证分层与当前证据

| 要回答的问题 | 证据 | 不能推出什么 |
|---|---|---|
| 网络是否精确实现声明转移 | check-net 全部有限观察，包含缺转移/声明返回 | 声明自身就是 C99 语义 |
| 阶段是否保持参考行为 | 固定 keep、输出/退出/诊断、边界与失败探针 | 任意程序/未测组合全域等价 |
| 产品是否完成实际编译/运行 | `.com` 用户命令、cc 可观察差分、真实应用与自举 | 编译器与平台 cc 所有实现定义相同 |
| 包/宿主是否可靠 | codec、损坏包、资源、内存范围、API 失败 | CRC 已提供企业签名信任 |
| 发布物是否可分发 | 精确 SHA、来源、签后运行、平台和信任回执 | 旧候选门禁适用于新源码/重签字节 |

当前集成候选身份及已验/待验范围以本页R9清单为准；旧候选的完整队列、平台与Apple资格不移植。完整610MB数组C源码仍是未解决规模限制。T1是有限转移/网络等价；全程T2和C语义T3仍为独立证明义务。

<a id="model-function-bytes"></a>
#### 0.3.14 当前模型功能与物理字节账

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

**逐阶段结构计数口径**：论文 §1.3 的结构快照见 [pipeline-structure-20260928.json](research/pipeline-structure-20260928.json)，绑定旧01e5c1d9测量基线；当前d4f7d302静态结构见[r9-pipeline-structure-prune.json](research/r9-pipeline-structure-prune.json)，旧calc动态计数不得外推。静态动作数按每个序列展开 Q/C 前缀后的动作次数求和；另记包中实际保存的后缀动作数。它们不等于一次程序运行的动态动作数，也不是动作操作码种类数。声明返回按银行数与键成员数另记；O1/O2 不能共用一组结构数字。结构审计另附 calc 动态计数：cc-unisacc 提供插桩/原始输出，cdx 用同一二进制独立复跑，五阶段计数一致；输入、模型与运行时 SHA、O0/run 与 memory 路由及日志归档到 research/pipeline-counts-20260928/。它不是出货汇编内核的性能测量，结构表中的 O2 网络未参与该次 O0 运行。

---

# 第一部分 · 契约

## 1. 命题与契约

### 1.1 命题 [T]

Shell = **推理器 + 执行器 + 模型数据**。

- **[T-1]** 走查器 / 符号表 / 重定位算术 / ELF-MachO-PE 头 = **经典代码**（代数），**不得神经化**。
- **[T-2]** 网络只做**最后一公里选表**：离散 key → 1 个类。每一个表形状的阶段**都要**神经化。
- **[T-3]** Gold 表同时是标注、兜底与验证器。
- **[T-4]** 到处只有一个 kernel：`embed → gemv → ReLU → gemv → argmax`。换权重即换能力，kernel 不变。
- **[T-5]** 6 目标，仅 64 位：`lnx|osx|win` × `x86_64|arm64`。同一条 tape，六份镜像，stdout / exit 必须一致。

**这个实验在证明什么**：一个决策层由确定性神经策略驱动的 C99 编译器（对标 tinycc / `tcc -run`），支持 6 个目标，体积小到离谱。不是"AI 写代码"，恰恰相反——编译器的决策表是**构造/学出来的产物**：可热插拔、形式统一、可穷举验证。**主张是接口统一性，不是体积**（[R-2]；体积主张已被 E-3′ 与 Boniol et al. NFM'26 打掉）。让论点成立的交付物是 §5.3 那张表——旁边摆 `tcc`、裸表、BDD 三条基线，并诚实报告我们输在哪里。

### 1.2 确定性契约 [D]

不可字节复现的编译器不算编译器。这一节压过所有 ML 习惯。

| ID | 条款 |
|---|---|
| **D-1** | 无 dropout / 无权重噪声 / 无 label smoothing / 无数据增广 / **推理期无 RNG** |
| **D-2** | `argmax` 平局取**最小类下标** |
| **D-3** | 训练期随机性仅存在于初始化与 batch 置换，且只来自 mulberry32；置换种子 `seed ^ epoch` |
| **D-4** | `gemv` 累加顺序固定（下标升序，禁 pairwise / 禁多线程重排），跨构建位稳定 |
| **D-5** | 同源码 + 同权重 → 镜像**逐字节相同**；任何头部不得含时间戳、路径、build ID |
| **D-6** | 相同种子 → 权重**逐字节相同**，跨运行、跨机器 |
| **D-7** | 网络与 gold 对同一 key 的决定必须完全相同；不一致 = 其中之一有 bug，不是"模型方差" |

### 1.3 超级拟合 [F]

每个阶段都是**有限离散全函数**，`K_s` **就是**定义域，不存在要泛化过去的留出分布。**记忆即规格。**

| ID | 条款 |
|---|---|
| **F-1** | `NET_READY = 0.85` —— 运行时接管门槛，保证训练中途编译器可用 |
| **F-2** | `NET_HOT = 0.995` —— 训练热跳过门槛 |
| **F-3** | `SHIP_ACC = 1.000` —— 出货门槛；`unisa ship` 与验收拒绝低于此值 |
| **F-4** | `--holdout` 仅作诊断（网络是学到结构还是纯记忆？），出货运行**禁止**开启；holdout 精度作实验发现记录，门禁一律以 FULL gold 为准 |
| **F-5** | 某阶段到不了 1.000 = **gold 的 key 编码错了**。去修编码。只有编码被证明正确后才允许加宽，且必须记入 `MANIFEST.json` |

### 1.4 证明义务与接口约束 [P]

「C99 可用模型等价替代」不是一个定理，是**五层义务**，难度差几个量级。分层列明，避免把可判定的当成难题、把难题当成已解决。

#### 交给实现的硬约束

| ID | 条款 |
|---|---|
| **P-1** | **Oracle 不透明性**：走查器与 lowering 对任何阶段的依赖，**只经由 `ask` 返回的类名**。禁止读取 logits / 概率 / margin / 任何网络内部状态来改变编译输出。不得做 ensemble，不得"低置信度回退 gold" |
| **P-1a** | Oracle 的 net/gold 用量统计仅用于报告，**不得反馈进任何编译决策** |
| **P-1b** | `NET_READY` 门禁在**权重加载时一次性解析**，不是逐决策解析；给定一份权重快照，编译器是一个固定函数 |
| **P-2** | **key 全域性**：`ask` 收到的 key 必须 ∈ `K_s`。实现上**无条件断言**；「该断言不可达」是交给形式化的义务 |

#### 证明义务分层

| ID | 命题 | 性质 | 归属 |
|---|---|---|---|
| **P-3** | 逐阶段等价：`∀k ∈ K_s : argmax(N_s(k)) = G_s(k)` | `K_s` 有限、闭合、极小（6–960 行），**穷举即完全判定过程** | 已由 `acc = 1.000` 完整证明，无剩余义务 |
| **P-4** | 量化等价：量化网络 ≡ f32 网络 | 同上，在同一有限域上穷举 `argmax` 不变性 | 由 `unisa quant` 证明。logit margin 只用来**预测**阶梯落点，不承担证明责任 |
| **P-5** | 组合等价：net 驱动的编译器 ≡ gold 驱动的编译器 | 由 P-3 经同余性直接得到，**不需要对走查器做归纳** | **前提是 P-1**。一旦读了 logits，P-5 当场失效 |
| **P-6** | 语义保持：gold 驱动的编译器 ≡ C99 语义 | CompCert 级别工作量 | **与神经化正交，本实验不声称**。它是经典编译器本来就有的负担 |
| **P-7** | 目标等价：`∀t : exec_t(lower_t(tape)) ≃ vm(tape)`（可观测行为上的模拟关系） | 每目标一个模拟关系 | 真正需要干活的部分，见 §5.1 |

#### P-8　存在性定理（构造式，已验证）

> 设 `f : K → C` 是有限积 `K = V₁×…×V_m` 上的全函数。则**存在**权重，使 kernel
> `embed → gemv → ReLU → gemv → argmax` 在 K 的**每一点**上精确计算 f。

构造：`embed` 取 one-hot，故 `x ∈ {0,1}^Σ|Vᵢ|` 恰有 m 个 1；每个 key `k` 配一个隐单元
`u_k`，令 `W1[coord(i,kᵢ), u_k] = 1`、`b1[u_k] = −(m − 0.5)`，则 ReLU 仅在 key 完全匹配时
激活（值 0.5），否则恰为 0；令 `W2[u_k, f(k)] = 1`。任意输入恰好一个隐单元激活，logit
在 `f(k)` 位取 0.5、其余为 0，argmax 必然正确。∎　上界 `h = |K|`。

**推论**：「能不能达到 100%」不是开放问题。开放的是**最小性**：

```
h_min(f)（未知）  ≤  h_训练(f)（已知实例，精确）  <  h_构造(f)（已知实例，精确）
```

尚待形式化的三条：

| ID | 命题 | 备注 |
|---|---|---|
| **P-8a** | `h_min(f)` 的组合刻画 | **小表已精确关闭**（E-26：reloc/enc/pp/scope/lex 的 `h_min` 上下界吻合；`h_min(parse) ≥ 6`）。大表、三字段表、多头表仍开放 |
| **P-8b** | **可达性分离**：存在 f 使精确解集非空，但从随机初始化出发的 SGD 在任意预算内不可达 | 已有实例（E-18 的 s2：12.8× 参数仍 0.8143）**且已有机理**（E-25 的活板门：`W1` 方向上小步只会杀死单元，改规则需 `(W1,b1)` 同时离散跳变）。论文里最硬的一块 |
| **P-8c** | 是否存在确定性算法在 `O(h_min)` 内构造 | 先验注意：**最小 DNF 是 NP-hard**，目标应为「好启发式 + 精确性作为不变量」而非最优 |

> **给形式化的提示**：P-3 / P-4 不需要任何逼近论证——不需要 PAC bound、Lipschitz 常数、鲁棒性半径。超级拟合的意义正是把 ML 泛化问题变成了**模型检验问题**。唯一穷举帮不上忙、需要对走查器做真推理的地方是 **P-2**，那也是唯一真正藏 bug 的地方。

---

## 2. CLI [U]

```
unisa train [--epochs N] [--holdout none|random15|osx/arch] [--out weights/]
unisa acc
unisa compile in.c -o out --target lnx/x86_64
unisa run in.c [--target T] [--fold] [--drive gold|spec|combo] [--fault NAME]
unisa tape in.c
unisa lower OP --target T
unisa ship [--dtype q2|q4|i8|f16|f32] [--out kit.zip]
unisa dump-weights [--dtype q2|q4|i8|f16|f32] --out weights/
unisa quant [--stage S]        # 量化阶梯报告 §3.7
unisa bench [--n N]            # 逐阶段决策吞吐 §5.3
unisa build-weights [--out weights/built.json] [--pack weights/built.uns2]
                               # 构造精确整数权重，无训练无随机 [K-5] [E-22]
unisa emit-kernel [--out kernel/]
                               # 把决策层发成 C：模型 blob + 整数 kernel [E-28]
unisa vm FILE.tape [args...]   # 直接运行一条 tape —— 基准机器 [TP-4]
unisa compile ... --from-tape  # 输入是 .tape 而非 C（只做 lowering + 汇编）
```

**出货的编译器 `unisacc`（`unisacc.com`）的契约** [S-11] [S-15 C1]——上面是 Python 驱动，这才是用户拿到的东西。每个 flag 与 `tests/cli.sh` 里的一条用例对应；行为对标 cc/tcc，凡有出入处写明：

```
unisacc [flags] FILE.c [FILE.c ...] [-- args...]
  -run            编译后直接在内存里运行，不落盘；后面的 .c 是输入，第一个非 .c 起是程序的 argv，`--` 显式分界
  -b os/arch      写该目标的可执行镜像（六个目标任选，与宿主无关）
  -S              写 tape——本编译器汇编层的 IR，与 gcc -S 同级；-c 是它的同义词（没有目标文件）
  -E              只预处理，输出文本
  -o FILE         输出位置；`-o -` 为 stdout
  -               作为输入文件名时表示 stdin
  -I DIR  -D N[=v]  -U N     与 cc 同义；-U 在全部预定义之后生效，能撤销 __linux__ 之类
  -include FILE   如同源文件第一行是 #include "FILE"；报错行号仍是用户文件的
  -nostdinc       不查内建的头文件副本（只查源文件目录与 -I）
  -l* -L* -x c    接受并忽略：库在头文件里，没有链接器，只有一种语言
  -W* -w -g -f* -std=* -pipe -m*   接受并忽略：一种方言、无独立调试信息
  -O0 / -O / -O1 / -O2 / -O3 / -Os   tape→tape 优化档位（H1–H4；-O0 与缺省 walker tape 逐字节相同；出货构建用 -O2）
  -v              编译后打印每个阶段被问了多少次（仪表，不是 verbose）
  -version / --version   版本串
  --check-oracle  枚举全部问题，核对缓存与网络逐个一致 [A-49]
FILE.tape         输入是 tape 时直接进后端
```

与 cc 明确不同之处：`-c` 不产目标文件（与 `-S` 同义，写出 tape）；`-g` 不产调试信息；找不到的 `#include` 是错误（C99 6.10.2p4），以前是静默跳过。`-O*` **会**改写 tape（见 H1–H4），不是接受并忽略。

### 2.1 测试套件

`tests/all.sh` 是唯一入口，按**退出码**判定每一套（不靠匹配末行）：

| 套件 | 查什么 | 条款 |
|---|---|---|
| `acceptance.sh` | 规格自己的验收清单 | [A-*] |
| `vm.sh` | tape 解释器，手写夹具（不经前端） | [TP-4] |
| `difftest.sh` | 与系统 `cc` 对拍 —— **唯一能看见 gold 缺陷的仪器** | [P-6] [A-17] |
| `native.sh` | 发出的镜像在本机**真实执行** | [A-18] |
| `artifacts.sh` | 出货产物**可用**而非仅"格式正确"：UNS2 往返、六目标镜像被平台工具识别、两次 ship 字节相同、体积预算 | [A-24] |
| `ccrun.sh` | unisacc 编译 C，基准 VM 运行，**逐个探针与 Python 前端对答案**；`wrong` 必须为 0，一致数只能升，已知分歧记在 `ccrun.knownwrong` | [A-21] |
| `selfhost.sh` | 两种构建的 unisacc 与 Python 前端逐 token 一致 | [A-20] |
| `bootstrap.sh` | `B = C = U` 自举不动点 | [A-23] |
| `baseline.sh` | **SGD 对照组**，手动触发：E-18 / E-31 / E-37 要的那几个数 | U-5 |
| `corpus.sh` | **c-testsuite 的 220 个程序** —— 别人写的、为别的编译器写的 | [A-25] |
| `crossnative.sh` | **非本机目标**真实执行：两个 Linux 目标进本地虚机，osx/x86_64 走 Rosetta 2，win/arm64 与 win/x86_64 进 UTM 的 Windows 11 真机（虚机没开就**跳过并点名**，`STRICT=1` 时跳过即失败） | [A-27] |
| `fat.sh` | **一个文件两条 ISA**，两个 slice 都真跑（arm64 原生 + x86_64 经 Rosetta） | [A-28] |
| `closure.sh` | `unisacc -b` 写出的镜像与 Python 后端**逐字节相同**，六个目标；宿主目标的镜像真跑 | [A-34] |
| `nativeboot.sh` | N1 = N2 = N3，全程无 Python | [A-35] |
| `bigclosure.sh` | 同 closure，但对象是**编译器自己**（707 KB、1,874 个数据符号） | [A-42] |
| `layout.sh` | 数据布局**枚举**：762 条 tape × 6 目标 = 4,572 次逐字节比对 | [A-41] [P-3] |
| `datashape.sh` | 生成声明序 ≠ 地址序的病态程序，比对字节前先断言形状确实出现 | [A-41] |
| `consts_check.py` | `lower.py` 的常量链与 `unisacc_back.c` 的字面量必须相等 | [A-43] |
| `ablate.sh` | 把某阶段的答案旋成错的：必须有镜像变了或编译被拒 | [A-36] |
| `run.sh` | `unisacc -run` 在内存里编译并运行 | [A-37] |
| `cli.sh` | 空目录里把编译器当工具用：自带头文件、`-I`/`-D`/`-o`/`-E`/`--version`、shebang、argv | [A-39] |
| `diag.sh` | 报错要给用户的文件、行、列与源码行 | [A-40] |
| `ape.sh` | `unisacc.com` 一个文件、每个目标 | [A-38] |
| `multi.sh` | 多翻译单元一个程序，对 `cc a.c b.c` | [A-30] |
| `tools.sh` | 别人的库代码，多文件，自带已知答案测试 | [A-31] |
| `selfgap.sh` | 自举差距本身：unisacc 拒绝而 Python 前端接受的程序数，必须为 0 | [A-29] |
| `stages.sh` | Python 前端问过的每个阶段，C 前端也问过 | [A-33] |
| `linux.sh` | 整套跑进本地 Linux 虚机；客机架构与宿主不同则自动放大看门狗 | [A-32] |
| `release.sh` | 发布前的门：树已提交、版本串在、`STRICT=1` 全绿、没有套件被跳过 | [S-14] |

（这张表以前只列 12 套，落后了十几套。唯一权威是 `tests/all.sh` 的 `run` 行。）

| ID | 条款 |
|---|---|
| **U-1** | `unisa run file.c --fold` 是产品本体；权重缺失时首次运行自动训练 |
| **U-2** | `--drive`（默认 `built`）：`gold` 强制全部走表；`spec` = isel∘abi 两个 StageNet；`combo` = 单个 UnisaNet 供 12 头；**`built` = 构造出的整数网络**（K-5，精确性由构造保证，无 readiness 门禁）。前端表不受 spec/combo 影响，但 `built` 覆盖全部阶段 |
| **U-3** | `--fault`：`--fold` 的反向对照（§5.1），正常运行不出现 |
| **U-4** | 退出码：`0` 成功 / `1` 编译错误 / `2` fold 未达 6/6 |

---

# 第二部分 · 模型层

## 3. 模型层

### 3.0 决策点清单 —— 工作流 × 模型结构

**每个决策点一个独立的小模型**，不按编译阶段合并。合并已实测为有害（见 [E-18]）：
`parse+type+scope+irsel` 合成一段后无论加宽到多少都到不了 1.000，因为段内各表的
key 空间互不相干，共享 trunk 没有结构可共享、只有互相挤占。分立还带来**变更局部性**
——改一张 gold 只需重训那一张（约 0.3 秒）。

全部模型共用同一个 kernel（[K-1]），只有形状和权重不同。

决策点表**不再手抄在这里**：原先这里的 14 行手写快照（合计 6,650 行、413 单元）自称“生成”，实际已经与真表不符。bdy-ds4flash 审阅（2026-09-26）指出了这一点。生成的表见 `README.md`、`prd.tree.md` 和 `prd.map.md`，由 `unisa/docgen.py` 写出，`tests/docs.sh` 检查。按 `unisa/gold.py` 的 `ALL`，共 **18 张，8,484 行，513 单元**；C 编译器实际询问其中 16 张（isel 和 combo 不在路径上）。

> 生成的表**由 `unisa/gold.py` 与发布的构造权重生成**（`python3 -m unisa acc` 是同一批
> 数字）。手抄快照漂过四次，所以 prd 不再抄；训练 θ 属于对照臂，见
> [E-1]，不在发布路径上。`isel` 仍是一个阶段，但 **lowering 不再询问它**：它的
> `form` 由 os 感知的 `enc` 取代，`symbol` 没有任何编码器读（`tests/ablate.sh`）。

**装配方式**（`--drive`，[U-2]）：

| 模式 | 使用的模型 |
|---|---|
| `built`（默认，发布路径） | 全部 14 个的**构造整数权重** |
| `spec` | 训练臂：pp lex parse type scope irsel enc reloc **isel abi**（历史口径） |
| `combo` | 训练臂：前端各表 + **combo** 替代 isel+abi |
| `gold` | 不用模型，全部查表（见 [O-1]） |

`combo` 是 `isel`+`abi` 的**替代**而非追加：同一 key 空间 `op×os×arch` 上的头合一。
两条路径在完整 gold 上逐 key 同类（[D-7]），所以 `--drive` 不改变编译结果，只改变由谁作答。

**前端 8 个** 把源码走到通用 tape，**后端 4 个** 把 tape 铺到 6 个目标（`isel` 与 `combo` 只在对照臂里）。
形状细则见 [N]（TableNet）与 [S]（StageNet / combo）；每个 key 空间的标签函数见 [G]。

### 3.1 唯一 kernel [K]

| ID | 条款 |
|---|---|
| **K-1** | `embed → gemv → ReLU → gemv → argmax`，仅此一条路径 |
| **K-2** | 部署期**无 softmax、无 libm、热路径无 malloc** |
| **K-3** | 整数 dtype 用 `int32` 累加器，scale 每个输出只乘一次——**内层循环无浮点** |
| **K-5** | **整数构造后端（E-21 / E-22）**：`W1 ∈ {0,1}`、`b1 ∈ {0,−1,−2}` 且**无需存储**（由字面量数恢复）、`W2 ∈ {1,2,4,8,16}`。输入是恰含 m 个 1 的 one-hot ⇒ 第一层是 **m 次加法 + bias**，不是矩阵乘；隐激活恒为 0 或 1 ⇒ 第二层是**条件整数加**，连移位都不需要。全链路**零乘法、零移位、零浮点**。最小覆盖下 max\|pre\| = 2、max logit = 19 ⇒ **int8 足够** |
| **K-4** | Python 参考实现 `unisa/linalg.py`，C 部署实现 `kernel/unisa_core.c`，两者在 FULL gold 上必须逐 key 同类 |

### 3.2 TableNet [N]

```
embed keys (d=8) → gemv W1[h0,16]+b1 → ReLU → gemv W2[16,nout]+b2 → argmax
h0 = d * (key 字段数)
train: softmax CE, Adam β1=0.9 β2=0.999 eps=1e-8
init:  E N(0,0.08); W1 N(0,sqrt(2/h0)); b=0; W2 N(0,sqrt(2/h))
rng:   mulberry32
```

| ID | 条款 |
|---|---|
| **N-1** | 默认 `d=8 h=16`；lex/pp 用 `d=6 h=12`；reloc 用 `d=6 h=8` |
| **N-2** | 种子：parse13 type17 scope19 pp23 enc29 lex31 reloc37 irsel11 |
| **N-3** | `fit` 热跳过：acc≥`NET_HOT` 且 `step%6≠0` 时跳过 |
| **N-4** | **永远评估 FULL gold，绝不用 batch acc 判 ready** |

**mulberry32**（逐位精确，每步 32 位截断）：

```
s = (s + 0x6D2B79F5) & M32 ; t = s
t = imul32(t ^ (t >>> 15), t | 1)
t = t ^ ((t + imul32(t ^ (t >>> 7), t | 61)) & M32)
u = (t ^ (t >>> 14)) & M32  ;  return u / 2^32
```

**N-5** 高斯 = 相邻两个均匀数做 Box-Muller。训练期可用 libm，**部署期不可**。

### 3.3 StageNet / combo [S]

| ID | 网络 | 规格 |
|---|---|---|
| **S-1** | isel | emb op12+arch8，h1=24 h2=16，heads form/symbol（gate 已移入 abi），seed 3 |
| **S-2** | abi | emb op12+os8+arch8，h1=32 h2=20，heads sysno/arg0-5/ret/gate/nrreg，bilinear 8d，seed 5 |
| **S-3** | combo | ~10.5kθ，seed 7，`E_op[38,16] E_os[3,8] E_arch[2,8]` |

**S-4** combo 的 `h0=52` 分解：

```
h0 = E_op(16) ++ E_os(8) ++ E_arch(8) ++ factor(12) ++ bilinear(8)      = 52
factor   = ReLU(W_op·E_op + W_os·E_os + W_arch·E_arch + b)              → 12d
bilinear = (P_op·E_op) ⊙ (P_os·E_os + P_arch·E_arch)                    → 8d
→ W1 52×48 → ReLU → W2 48×32 → ReLU → 9 heads
```

**S-5** 4 个寄存器头（arg0,arg1,arg2,ret）共享一个 `W_reg[32,|REGS|]`，各自独立 bias。

**S-6 9 个头**

| head | 来源 | 词表 |
|---|---|---|
| form | isel | FORMS |
| symbol | isel | SYMS |
| gate | isel | GATES |
| sysno | abi | SYSNOS |
| arg0 arg1 arg2 ret | abi | REGS（共享 W_reg） |
| tls | abi | TLS |

### 3.4 词表 [V]

**V-1** `OPS`（N=38），isel / abi / enc / combo 共用此 op 轴，**顺序写死，下标即 embedding 行号**：

```
SYSOPS(19) = exit read write open close mmap munmap mprotect getpid
             clock_gettime nanosleep futex socket connect bind listen
             accept clone execve
MOPS(19)   = add64 sub64 xor64 mul64 slt64 sle64 load64 store64 jump
             jumpz call ret nop cas64 fence syscall_gate tls_base
             cycle_counter stack_enter
OPS = SYSOPS ++ MOPS
```

**V-2** 输出词表：

```
FORMS(5)  = syscall svc winapi x86 arm
GATES(5)  = syscall svc0 svc80 winapi none
REGS(18)  = rdi rsi rdx r10 rcx r8 r9 rax x0 x1 x2 x3 x4 x5 x6 x7 x8 none
TLS(4)    = fsbase tpidr_el0 teb none
SYMS, SYSNOS = §4.4 派生函数实际吐出值的有序并集 + `none`
```

### 3.5 Gold [G]

**G-0** 每阶段声明 `[(field, vocab), ...]` 与标签函数。**gold 语料 = 完整笛卡尔积 `K_s`**——枚举代价极低，并强制网络学成全函数。这是 P-3 可判定的前提。

**G-1 parse** — NT(5) × TOKS(54) → PRODS(32)，**270 行**

```
默认: top=global  stmt=expr  unary=prim  postfix=done  after_name=var_def
覆盖: top/eof=end | top/typedef=typedef | top/struct=struct | top/enum=enum
      after_name/( = fn_sig
      stmt/type = stmt/struct = stmt/enum = stmt/typedef = decl | stmt/{ = block
      stmt/X = X     X ∈ {if,while,for,do,switch,case,default,return,break,continue}
      unary/- = neg | unary/! = not | unary/* = deref | unary/& = addr | unary/sizeof = sizeof
      postfix/[ = index | postfix/( = call | postfix/++ = inc | postfix/-- = inc
      postfix/. = field | postfix/-> = field

TOKS  = eof type id num str if else while for do switch case default return break
        continue sizeof struct typedef enum { } ( ) [ ] ; , = += -= *= /= ? : + - * / %
        == != < > <= >= && || ! & ++ -- . ->
PRODS = end fn global typedef struct enum decl if while for do switch case default
        return break continue block expr neg not deref addr sizeof prim index call
        inc field done fn_sig var_def
```

**G-2 type** — TYS(15) × TOPS(19) × TYS(15) → TYS|illegal，**4275 行**

```
TYS  = void i8 i16 i32 i64 u8 u16 u32 u64 ptr arr struct fn
                                （i16 = short，E-31；u* = unsigned，E-37）
TOPS = + - * / % < == = & [] . call sizeof , un* | ^ << >>   （un* 一元解引用，忽略 t2）
默认 illegal
数值算术 (+ - * / %) 与位运算 (| ^ << >>) → 任一为 i8|i16 则 i32，否则 i64
< 与 == → i64        , → 右操作数        = → lhs        sizeof → i64
ptr|arr ± num → ptr        ptr - ptr → i64        [] → i64
ptr un* → i64        fn call → i64        i64 & i64 → ptr        struct . → i64
TY_SIZE: void=1 i8=1 i16=2 i32=4 u8=1 u16=2 u32=4 其余=8
无符号（C99 6.3.1.8 的"惯常算术转换"）：
  先做整型提升（秩 < int 的一律变 int，含 u8/u16）
  两侧都无符号 → 无符号；一侧无符号且秩不低于另一侧 → 无符号；否则有符号
  无符号结果的**宽度按 C 取**（u32 或 u64），因为无符号运算必须在自己的宽度上回绕
  有符号的那些行与加入 unsigned 之前**逐行相同**
```

`i8`/`i16` 都当作窄于 int 的类型，参与整型提升；这是 C99 6.3.1.1 的投影，不是近似。

**G-2a** `illegal` 占定义域的大半（原 960 行版本为 611/960）。v1 要求"训练时按 1/19 下采样 illegal 行"——**实测证伪，已废止**：降权后 type 恒定卡在 0.999，唯一丢失的行是 `(i64,&,i32)→illegal`，正是那个稀疏正例 `(i64,&,i64)→ptr` 的近邻，且全表最紧的 margin 全部落在 `&` 上。类别平衡保护的是泛化，而这里没有泛化可保护、只有记忆；饿死 64% 的定义域，只会让网络在稀疏正例旁边变瞎。**权重取 1.0，全部行等权训练、等权评估。** 代码中保留 `TYPE_ILLEGAL_WEIGHT` 以复现 v1 行为。

**G-3 scope** — CTX(6) × KIND(5) → ACTS(7)，**30 行**

```
ACTS = bind_global bind_param bind_local lookup type_name fn_name field
默认 lookup
top/type_kw = top/typedef_id = type_name | top/id = bind_global | top/lparen = fn_name
param/id = bind_param | local/id = bind_local
local/type_kw = local/typedef_id = type_name
sizeof/type_kw = sizeof/typedef_id = type_name
field/id = field
```

**G-4 pp** — DIRS(9) × {0,1} → take|skip|pop|macro，**18 行**

```
默认 skip
ifdef: 1=take 0=skip   ifndef: 取反   if/elif/else: 跟随 flag
endif=pop   define=undef=include=macro
```

**G-5 lex** — CHARC(11) × peek CHARC(11) → ACT(10)，**121 行**

```
CHARC = ws nl A d q sq slash star punct eof other
ACT   = skip nl ident num str charlit cmt linecmt op bad
ws→skip  nl→nl  A→ident  d→num  q→str  sq→charlit
slash|star|punct→op  eof→skip  other→bad
slash×slash = linecmt   slash×star = cmt
```

**G-6 enc** — OPS(38) × os(3) × arch(2) → FORMS，**228 行**

```
win 且 op ∉ MOPS                  → winapi
arm64 且 op ∉ MOPS                → svc
op ∈ SYSOPS 且 (x86_64, lnx|osx)  → syscall
否则                              → 按 arch（x86_64→x86，arm64→arm）
```

**G-7 reloc** — jmpkind(3) × arch(2)，**6 行**

```
x86_64 → rel32 ; arm64 且 jz → arm19 ; 否则 → arm26
```

**G-8 irsel** — family(5) × flavor(27) → recipe|bad，**135 行**（27 有效）

```
alu : add sub mul lt le gt ge eq ne neg → add64 sub64 mul64 slt64 sle64 slt64 sle64 eq ne sub64
mem : load store lea ld st zero         → load64 store64 lea ld st zero
ctrl: jump jumpz ret                    → jump jumpz ret
call: call push arg frame               → call callpush arg frame
lit : imm print write exit              → imm print write exit
默认 bad
```

`gt`/`ge` 映射到 `slt64`/`sle64`——由走查器交换操作数。这种不对称正是最后一公里的表该承载的事实。

**G-9 isel / abi / combo 的 gold 由 §4.4 的 `(op, os, arch)` 函数派生，禁止手工标注。**

### 3.6 训练 [TR]

| ID | 条款 |
|---|---|
| **TR-1** | Epoch 循环：combo + isel + abi 按 batch 16；前端网络；每 epoch 评估一次 FULL gold |
| **TR-2** | **LR 是 epoch 衰减表，不是按参数量分档**：`epoch<18 → 0.032；<50 → 0.014；<90 → 0.006；else 0.0025`。所有网络共用 |
| **TR-3** | **停机**：所有网络达 1.000，或到 `--epochs`（**默认 200**，E-14 实测 90 不够） |
| **TR-3a** | **1.000 必须是吸收态**：某网络首次达到 `SHIP_ACC` 的那个 epoch 立即**快照并冻结**，此后不再训练它。实测（E-14）99 次运行中 30 次先命中 1.000 又掉下来，含三个出货种子——`[N-3]` 热跳过会带着陈旧的 Adam 动量恢复满 LR 步，把边缘 key 踢掉。**`[F-3]` 判定的是快照，不是最后一个 epoch 的状态** |
| **TR-4** | v1 的 `combo ≥ 0.985 && 所有 table ≥ 0.85 && epoch > 30` 保留为**最低可用**状态而非停机点；停在该状态报 `UNDERFIT`，被 `unisa ship` 拒绝 |
| **TR-5** | `--holdout`：`none`；`random15` 扣 15% 行不训练但全部行仍评估；`osx/arm64` 扣掉所有 os=osx ∧ arch=arm64 的行（仅 enc/isel/abi/combo；前端表无 os/arch 轴，回落 `none`） |

### 3.7 UNS1 与量化 [Q]

**Q-1** 头部 16B：`UNS1` | dtype u8 | flags u8 | nTensors u16 | nParams u32 | acc f32
**Q-2** 张量：`name[16]` | rows u16 | cols u16 | scale f32 | payload | 补齐到 4

**Q-3** dtype：

```
dtype: 0=i8  1=f16  2=f32  3=q4  4=q2
i8 : 整张量  scale = maxabs/127        1 值 / 字节
q4 : 按行    scale = maxabs/7          取值 -7..7，    2 值 / 字节
q2 : 按行    scale = maxabs            取值 {-1,0,1}， 4 值 / 字节
```

**Q-4** 按行 scale 存为 payload 前的 `f32[rows]` 段；张量头的 `scale` 取该段最大值（忽略行 scale 的读取方仍能拿到合理上界）。
**Q-5** 亚字节 payload 按**低半字节 / 低 2 位在先**打包，行主序，每行从字节边界起。

#### 量化阶梯 —— 真正的实验

超级拟合换来极大的 logit margin，所以问题不是"q2 损失多少精度"，而是 **"q2 会不会改变任何一个决策"**。

**Q-6 唯一判据 —— argmax 不变性**：对 `K_s` 的每一个 key，量化网络必须选出与 f32 **完全相同的类下标**（平局按 D-2）。

**Q-7** `unisa quant` 沿 `f32 → f16 → i8 → q4 → q2` 逐级下探，报告每阶段能保持不变的最低 dtype，写入 `MANIFEST.json`：

```
stage    θ      f32     i8     q4     q2    min-margin   ship
parse    1642   6568B   1642B  821B   411B  ...          q?
...
TOTAL           ...
```

**Q-8 [修正]** 每阶段按**各自体积最小的不变 dtype** 出货，**不是**按"最低 dtype"。二者不等价：
按行 scale 的 `f32[rows]` 块（Q-4）在小张量上比半字节打包省下的还多，实测 `pp` 的 q4 是
452 B 而 i8 只要 436 B，`reloc` 是 360 B vs 324 B。**以实测字节为准，不以 dtype 序为准。**
混合 dtype 的 kit 是预期结果；某阶段扛不住更低 dtype 是关于这张表结构的**发现**，不是失败。
**Q-9 [修正]** 逐阶段记录最小 logit margin。**实测它并不能预测阶梯落点**（见 E-6′）：
更好的预测量是**头数**与**参数量**——真正压垮 q4 的是逐行误差在多少个独立决策上复利，
而不是最坏的那一个有多紧。margin 仅作弱排序提示，且不承担任何证明责任（P-4）。

**Q-10** kit = `weights/*.unisa` + `MANIFEST.json` + `kernel/unisa_core.c` + 镜像。

---

# 第三部分 · 经典层

## 4. 经典层

### 4.1 Oracle —— 唯一接缝 [O]

```
oracle.ask(stage, key_tuple) -> class_name
```

| ID | 条款 |
|---|---|
| **O-1** | 网络存在**且** `net.acc >= NET_READY`（在 FULL gold 上测得）→ 网络 argmax；否则 → 查 gold 表 |
| **O-2** | 逐阶段统计 net / gold 用量，`unisa run` 末尾打印 `nets: k/14 driven` |
| **O-3** | 无条件断言 `key ∈ K_s`（P-2） |
| **O-4** | 遵守 P-1 / P-1a / P-1b：只回传类名，不泄露任何内部状态 |

这让"gold 是验证器"成为**运行时事实**：编译器在训练曲线的任何一点都正确，网络渐进接管。

### 4.2 前端走查 [W]

```
src → pp → lex → parse → type → scope → irsel → tape
```

| ID | 阶段 | key | 输出 |
|---|---|---|---|
| **W-1** | pp | dir × defined | take\|skip\|pop\|macro |
| **W-2** | lex | charclass × peekclass | act |
| **W-3** | parse | NT × TOK | production |
| **W-4** | type | t1 × op × t2 | ty |
| **W-5** | scope | ctx × kind | action |
| **W-6** | irsel | family × flavor | recipe |

**W-7** 走查器本身是经典递归下降代码（T-1），只有选表那一下问 Oracle。
**W-8** C99 子集覆盖：`#if` 家族、函数 / 指针 / 数组 / struct / typedef、if/for/while/do/switch、算术、`printf` → `.print`/`.write`。
**W-9**（2026-09-26 修订：printf **一律是对 `<stdio.h>` 里 printf 的普通调用**。C 前端见 991d337，调用时自动带入该头；Python 前端见 18c8f22，由 driver 补头，脱糖代码已删。C 前端的 `do_printf` 脱糖只在单元里**没有** printf 声明时兜底，也就是 `-nostdinc`。两个前端的 tape 不要求一致：closure 比的是同一条 C tape 过两个后端，ccrun 只比运行结果。下面是修订前的原文。）`printf` 在走查期按**静态格式串**脱糖（`%d %s %c %u %%`），这是快路径也是常见路径。**格式串不是字面量就根本脱不了糖**，此时放行到 `<stdio.h>` 里真正的变参 `printf`（它和 `fprintf`/`sprintf`/`snprintf` 共用同一个运行期格式化器 `_u_vfmt`）。一个坑：调用在**走查末尾**才解析，所以调用可以先于定义——对普通函数无害，但这几个函数的**调用约定不同**（参数全压 tape 栈，[W-16]），后到的定义救不回已经按另一种约定发出去的调用，所以它们的变参性写死在 `VARIADIC_LIBC` 里。`%d` 经发射的 `__itoa` 助手（纯 tape op，故可原生编码），`%s` 经 `__strlen`。
**W-10** C 字符串字面量**必须 NUL 结尾**。`write_literal` 传显式长度，故字面量块不会输出该字节；但 `%s` 走 `__strlen`，无结尾符会一路扫进相邻字面量。
**W-12** 相邻字符串字面量按 C 语义**拼接**（`"a" "b"` == `"ab"`）；扫描器逐个字面量出 token，在 token 层折叠。
**W-11** `static` 局部变量取**静态存储**（数据段、零初始化），不是栈槽。
**W-15** **libc 地板**：`include/` 带 `<ctype.h>` `<limits.h>` `<assert.h>`，以及 `exit`/`abort`。这三个头加 `exit` 几乎每个真实 C 程序都要，之前一个都没有。两条实现上的决定：① **`exit` 是唯一不能用 C 写的库函数**——它不能返回——所以它是 `__exit` 内建（`.sys` gate）的一行包装；② **`assert` 只打印表达式，不打印文件与行号**：这个预处理器没有 `__LINE__`/`__FILE__`，因为 include 是**原地展开且不插行标记**的，第一个 `#include` 之后两者都会是错的，而**错的行号比没有行号更坏**。`<limits.h>` 给的是**类型的极限**，不是本编译器的求值宽度——[G-2] 让 `int` 表达式在 64 位里算，这不改变 `INT_MAX` 是多少。
**W-13** **全局指针的启动期初始化**：文件作用域的指针变量若以地址常量（字符串字面量、全局/函数地址）初始化，其值在程序启动时由启动序列写入数据段，而不是编译期绝对地址（镜像加载地址不固定）。（编号早已分配并在 archive/prd-findings-20260929.md 的“补齐的 C 特性”表中使用，正文 2026-09-30 补入。）
**W-16** **unisacc 生成代码之间的私有调用约定**（不是 SysV/AAPCS64；跨 ABI 的进出由 lower 的桥与库调用桥承担，见 §4.5/§I）：调用者把全部实参**依次压 tape 栈**（第一个参数在最低地址、紧挨返回地址，自左向右递增），`r9` 为帧指针，返回值在 `rax`（arm64 对应寄存器由 regmap 表给出），调用者清栈。该约定**不稳定**：随 regmap/abi 表变化，外部代码不得依赖；需要与外部代码互调时走 libunisacc 的载体/计划路线。`examples/apps/xgui.c` 的 syscall 桩只是对现状的经验描述。
**W-14** **多翻译单元，无链接器**：`unisa compile a.c b.c` 对每个文件**各自预处理与词法**（include guard、`#define` 状态是 per file 的），再由**同一个 walker** 依次走查，最后统一解析调用 —— 因为调用本来就是在单元结束时才解析的（`called`），所以跨文件调用与跨行调用走的是同一条路。代价两条，都记在这里：① **文件作用域 `static` 必须重命名**（`name_u<k>`），否则两个文件的同名 helper 会互相覆盖；单文件编译后缀为空，于是 tape **逐字节不变** —— 这很重要，[A-23] 要拿 Python 前端的 tape 和 unisacc 自己的 tape 逐字节比。② **作用域不分文件**：前一个文件的 typedef 与 struct tag 在后一个文件里仍然可见。这是错的 C，是真实程序绊倒时第一个要修的地方。

### 4.3 Tape 与 VM [TP]

按行文本。标签 `L:`。

| ID | 条款 |
|---|---|
| **TP-1** | 寄存器 `r0–r7`，`r7` 为 SP，初值 `0x10000`；内存 64 KB 小端 |
| **TP-2** | 算术按 2^64 取模、有符号二补码 |
| **TP-3** | `.print` / `.write` / `.exit` 是仅有的三个有外部副作用的 op，lowering 后**都必须变成真实 syscall / WinAPI 调用** |
| **TP-4** | **tape 解释器是 `--fold` 的基准真值** |

```
imm    rd, K              rd = K
mov    rd, rs
add64  rd, ra, rb         sub64 mul64 xor64 同理
.div   rd, ra, rb         有符号截断除；除零/模零 → exit 136
.mod   rd, ra, rb
slt64  rd, ra, rb         sle64 eq ne 同理 → 0|1
load64 rd, [rb+K]         store64 [rb+K], rs
.ld    rd, [rb+K], W      W ∈ {1,2,4,8}，符号扩展
.st    [rb+K], rs, W
.lea   rd, sym|K
.zero  [rb+K], N
jump   L                  jumpz ra, L
call   L                  ret
.frame N                  r7 -= N
.arg   i, rs              为下一次 call / builtin 就位第 i 个参数
.print ra                 int64 十进制写 stdout，不带换行
.write ptr, len           原始字节写 stdout（fd 1）
.exit  ra
nop
```

### 4.4 Catalog [C]

**C-1** 系统调用目录（lnx-x64 / lnx-arm / osx / win）：

exit 60/93/1 ExitProcess；read 0/63/3 ReadFile；write 1/64/4 WriteFile；open 2/56/5 CreateFileW（lnx-arm 为 openat）；close 3/57/6 CloseHandle；mmap 9/222/197 VirtualAlloc；munmap 11/215/73 VirtualFree；mprotect 10/226/74 VirtualProtect；getpid 39/172/20 GetCurrentProcessId；clock_gettime 228/113/116 QPC（osx 为 gettimeofday）；nanosleep 35/101/240 Sleep；futex 202/98/515 WaitOnAddress（osx 为 ulock_wait）；socket 41/198/97 WSASocketW；connect 42/203/98 connect；bind 49/200/104 bind；listen 50/201/106 listen；accept 43/202/30 accept；clone 56/220/360 CreateThread（osx 为 bsdthread_create）；execve 59/221/59 CreateProcessW。

**C-2** **osx sysno = `0x02000000 | nr`。** 这个 class bit 承重——§5.1 用抽掉它作反向对照。

**C-3** ABI：lnx/osx x64 参数 rdi rsi rdx r10，返回 rax，gate `syscall`；lnx arm 用 x0..，gate `svc #0`；osx arm gate `svc #0x80`；win x64 参数 rcx rdx r8 r9，form/gate 均 `winapi`。

**C-4** 字节：`add64` = `48 01 f0` / `00 00 01 8b`；`ret` = `c3` / `c0 03 5f d6`；`syscall` = `0f 05` / `01 00 00 d4`。

**C-5** MOPS 寄存器映射 r0–r7 → `rax rdi rsi rdx rcx r8 r9 r10`(x86_64) / `x0..x7`(arm64)。
**C-6** 除 `tls_base` 外，MOPS 的 arg0-2/ret/tls 一律 `none`；`tls_base` 的 tls 按 os 取 `fsbase|tpidr_el0|teb`。
**C-7** win 下 `sysno = none`，WinAPI 名字落在 `symbol`。

**C-8** 本节是 isel / abi / enc / combo 的**唯一真源**；多头 gold 必须由 `(op, os, arch)` 上的函数派生（G-9），`SYMS`/`SYSNOS` 即它吐出的值。

### 4.5 Lowering [L]

**L-1** `tape → lower(isel, abi, enc, reloc) → TargetProgram`，每个目标独立。
**L-2** TargetProgram 的每条指令携带：`form, symbol, gate, sysno, arg0-2, ret, tls, 字节, reloc kind`。
**L-3** `--fault` 的注入点在此：`osx_class_bit`、`win_argregs`、`arm_gate`。

### 4.6 目标机执行 [X]

| ID | 条款 |
|---|---|
| **X-1** | 目标解释器持有**该 arch 的具名寄存器**：`rdi rsi rdx r10/rax` 对 `x0..x8` |
| **X-2** | 系统调用分发器**只**认 lowering 产出的 `(os, sysno)` 或 `(os, winapi_symbol)`，**绝不回看通用 tape op** |
| **X-3 [修订]** | 解释器是 `--fold` 的基准真值 [TP-4]，六目标比对一律在解释器上进行。**但对宿主目标，镜像是真程序**：`tests/native.sh` 在 macOS/arm64 上实际执行并比对（E-27）。原条款的"不 execve"是范围约定，已达成后解除 |

X-2 带来的后果全是设计意图：

- abi 给 lnx/x86_64 的 write 吐 sysno 4（osx 的号）→ Linux 分发器拒绝 → stdout 不匹配
- osx sysno 丢掉 `0x02000000` class bit → 不匹配
- win 路径用 `rdi` 而非 `rcx` 传 arg0 → 不匹配

**X-4** **fold 若不是目标感知的，它就不是测试。禁止把通用 tape 跑六遍再和自己比。**

### 4.7 镜像 [I]

| ID | 格式 | 结构 | 魔数 |
|---|---|---|---|
| **I-1** | ELF64 | 单 `PT_LOAD`，vaddr `0x400000` | `7f454c46` |
| **I-2** | Mach-O 64 | `mach_header_64` + `LC_SEGMENT_64(__TEXT)` + `LC_UNIXTHREAD` | `cffaedfe` |
| **I-3** | PE32+ | MZ stub + `PE\0\0` + optional header `0x20b` + 单 `.text` | `4d5a` |

**I-4** 镜像字节可复现（D-5）。

### 4.7.1 宿主平台契约 [I-5..I-19]

以下不是我们的设计选择，是**平台强制要求**。违反其中任何一条，失败方式都不是报错而是 SIGILL / SIGKILL / 进程挂死。实测代价见 E-27。

| ID | 契约 | 违反后果 |
|---|---|---|
| **I-5** | Apple Silicon **不支持静态可执行文件**。arm64 Mach-O 必须带 `LC_LOAD_DYLINKER` + `LC_LOAD_DYLIB` + `LC_MAIN`，经 dyld 引导 | dyld 拒载，`Bad executable` |
| **I-6** | 头区必须留余量（本实现 `SLACK = 256`）：`codesign` 会**追加** `LC_CODE_SIGNATURE` | 覆写 `__text` 前 16 字节 → `udf` → **SIGILL** |
| **I-7** | arm64 macOS **强制 `MH_PIE`**，镜像会滑动 ⇒ 代码必须位置无关：arm64 用 `adrp+add`，x86_64 用 rip-relative | 绝对地址全部失效 |
| **I-8** | 可写数据（scratch / globals）必须在独立的 rw `__DATA` 段，不能放进 r-x 的 `__TEXT`；且需 `__PAGEZERO` 与 `__LINKEDIT`（后者是 codesign 写签名的地方） | 写入即 SIGBUS；无 `__LINKEDIT` 则 `codesign` 报 *failed strict validation* |
| **I-9** | **系统调用号寄存器是 OS 事实**：Darwin/arm64 用 **`x16`**，Linux/arm64 用 `x8`，x86_64 用 `rax`。因此 arm64 的地址合成临时寄存器必须避开 x16（本实现用 IP1 = `x17`） | 调用号被冲掉，**进程挂死而非报错** |
| **I-10** | `call`/`ret` 必须实现 **tape 的栈语义**（在 tape SP 上压弹返回地址），不能用 `bl`/`ret` 的 lr | 递归第二层即崩 |

**I-12** **ELF 同样要分段**：text 是 `PF_R|PF_X`，data 必须是独立的 `PF_R|PF_W` PT_LOAD，且 `p_offset ≡ p_vaddr (mod 0x1000)`。这条与 I-8 是同一条物理事实的两个平台写法；Mach-O 被 macOS 当场逼出来，ELF 因为只在解释器里跑过而藏了很久（E-32）。违反后果：写 scratch 即 **SIGSEGV**，且本地解释器与 `readelf` 都看不出来。

**K-5a** **C kernel 的字面掩码按阶段定宽**：每个字段的掩码是 `ceil(|vocab|/64)` 个 u64。一个字就够用，直到 TOKS 越过 64（`~` 与其余复合赋值把它推到 67），blob 写入直接溢出。宽度是**每阶段**的而非全局的——只有 `parse` 需要两个字，全局加宽要多花 5.7 KB。

**I-16** **PE 的节 RVA 必须按 SectionAlignment 对齐**，且要分 `.text` / `.rdata` / `.data`。第一版把 text+data 塞进一个 r-x 节、节 RVA 取 0x200，Windows 直接拒载（`Access is denied`，exit 5）。**而 `MajorSubsystemVersion` 是一个开关：声明 10.0 把加载器拨到严格路径，声明 4.0 走宽松路径**。两个 arch 都如此，包括 arm64 —— “arm64 不存在于 Windows 10 之前所以必须声明 10” 是错的，它正是我们每一张镜像都被 `STATUS_INVALID_IMAGE_FORMAT` 拒掉的原因。详见 E-35。

**I-17** **在严格路径上，arm64 Windows 确实强制可重定位，而且查得很细**（拿定主自己的 arm64 exe 逐字段拆出来的，那些 exe 声明的子系统版本都 ≥ 6）：把 dir[5] 的条目换成 ABSOLUTE 填充、目录原样保留，它立刻不加载；只把 dir[10] load config 的 `SecurityCookie`（+0x58）清零，它也不加载。**但这两条并不是 PE 的普遍要求** —— 我们曾从它们推出“自己的镜像也必须带 .reloc 与 load config”，那是错的：把 `MajorSubsystemVersion` 改成 4.0，一个 **无 .reloc、无 load config** 的位置无关镜像直接跑起来。教训是方法学的：**从一个能跑的样本里拆掉某字段 → 它不跑了**，只证明该字段在**那个样本所在的模式**下是必需的，不证明它在别的模式下也必需。详见 E-35。

**I-18** **WinAPI 调用是真调用**：它按 AAPCS64 / Win64 破坏全部 volatile 寄存器，而我们八个 tape 寄存器**全都**是 volatile，**tape 栈指针 r7 也在内**。所以 win 的 gate 必须前后夹一个保存区，tape 必须有**自己的栈**（不能像别处那样把 SP 绑到进程栈），并且要把 tape 说的 POSIX 形状翻译成 kernel32 的形状（`fd → HANDLE`、`WriteFile` 的第四个出参、返回写入字节数而非 BOOL）。

**I-20** **`open` 是最不可移植的系统调用，三个 OS 三个样子**：① `O_CREAT/O_TRUNC/O_APPEND` 的**位值** Linux 与 BSD 不同（64/512/1024 vs 512/1024/8），我们曾把 Linux 的硬编进 `include/stdio.h`，于是 macOS 上 `fopen("w")` 静默失败；② **Linux/arm64 根本没有 `open`**，目录里的号是 `openat`，第一个参数是目录 fd（`AT_FDCWD = -100`），**所有参数往后移一位**，mode 落在第四个寄存器——而 abi 网只有三个参数头 [C-1]，所以这一移位是**结构性代码**，在 lowering 里；③ **Windows 根本没有 open(2)**，gate 是 `CreateFileA`，它要的是 dwDesiredAccess 与 dwCreationDisposition，而且 disposition 是**第五个**参数（得从 r8/x2 抖到影子空间）。第三条的翻译放在 **C 库里**（`#ifdef _WIN32`）而不是编码器里，因为只有它知道在为哪个平台编译；代价是这类源码像 `host.c` 一样**每个目标一份 tape**，解释器因此要记住 tape 是为哪个 OS 编的（`src_os`）。

**I-21** **gate 有返回值**，而目标机解释器曾经把它丢掉：结果是返回寄存器里留着**系统调用号**。`write` 与 `exit` 不用返回值，所以这个 bug 活了很久，直到一个程序用 `open()` 的结果去 `write`，它就把 0x2000005 当成了 fd。与 [TP-6] 同类：**解释器比真机宽容的每一处，都是一个迟早会爆的雷**。

**I-22** **arm64 的局部变量寻址有两条窄路，越界是"改符号"而不是"截断"**：缩放正偏移 `imm12`（`[fp, #off]`，按宽度缩放）够不到负偏移；非缩放形式 `LDUR/STUR` 的立即数是**有符号 9 位**，范围只有 **−256..255**。我们直接把偏移 `& 0x1FF` 塞进那个字段，于是 `[fp, #-260]` 编成了 `[fp, #+252]` —— **不是读到截断的地址，是读到另一个局部变量**，没有异常、没有诊断，只是答案不对。于是**任何帧超过 256 字节的函数在三个 arm64 目标上都是错的**，而我们所有探针的帧都太小，六目标 `--fold` 也看不见（解释器不建模寻址模式）。真实代码一天之内撞到三次（`char buf[1024]` 是 C 里最常见的局部变量之一）。办法：两条范围都不够时，把地址算进一个 scratch 寄存器再访存。见 E-45。

**I-19** **一个文件带多条 ISA**：macOS 的标准容器是 Mach-O universal（fat）——大端的 slice 描述表 + 各自按页对齐的普通镜像，内核挑 slice，`codesign -f -s -` 会把每个 slice 都签掉。这是多 ISA 主张**诚实的前半**：它是**一个 OS 之内**的多架构；cosmopolitan 那种同时是 ELF / Mach-O / PE 的文件是另一个问题，我们还没做。

**I-15** **PIE 的 Mach-O 必须告诉 dyld 怎么 rebase**：既无 `LC_DYLD_INFO` 也无 chained fixups 时，dyld 走**旧的重定位路径**，去解引用我们从未发射的 `LC_DYSYMTAB` —— 在我们第一条指令之前**就在 dyld 里面崩了**（`forEachRebase_Relocations`，EXC_BAD_ACCESS at 0x48，即空 dysymtab 上的 `locreloff`）。**Darwin 25 容忍这个缺省，Darwin 23/24 不容忍**。办法是把 `LC_DYLD_INFO_ONLY` / `LC_SYMTAB` / `LC_DYSYMTAB` 三条**全零地**发出来，dyld 于是走 opcode 路径、发现无事可做。字符串表给 8 个 NUL（字符串表不能为空）。

**I-13** **x86_64 的 `push`/`pop` 不能用**：`spinit` 把 tape SP 绑到真 `rsp`，于是 tape 栈的第一个槽正是 `push` 要写的地址，两者互相覆盖。`idiv` 当年就是这么保存 rax/rdx 的，症状是返回地址被踩、**每一个打印整数的 x86_64 程序都崩**。一切寄存器保存都必须走 **tape 栈**（这正是 I-10 的含义）。

**I-14** **x86 ALU 是两操作数**：`dst = s1 op s2` 要展开成 `mov dst,s1; op dst,s2`，当 `dst` 就是 `s2` 时，那条 `mov` 先把右操作数毁了 —— `17 - 5` 算成 `17 - 17`。同理移位的计数必须在 `cl`，而 **`rcx` 是 tape 寄存器 r4**，必须存回。arm64 是三操作数指令，解释器也不建模这些，所以两条都只能被真机看见（E-33）。

**I-11** `ld`/`st` 必须尊重宽度与偏移符号：局部变量在 `[fp − off]`，**负偏移用不了 scaled imm12**，arm64 需 `LDUR/STUR` 系列；`int` 是 4 字节，按 64 位取会读进相邻变量。

---

# 第四部分 · 验收与度量

## 5. 验收与度量

### 5.1 fold [A]

**A-1** 对 6 个目标各自独立走：

```
tape → lower → TargetProgram → 镜像 + 目标机解释执行
```

**A-2** 六份 `(stdout, exit)` 必须全同，并与 `vm(tape)` 对拍一致（P-7）。

**A-3 反向对照（必做）**：`unisa run examples/hello.c --fold --fault osx_class_bit` 必须打印 **4/6**。这证明 fold 有牙齿。若仍 6/6，说明测试是假的。

### 5.2 样例 [A-4]

| 样例 | stdout | 说明 |
|---|---|---|
| hello.c | `hello from C99\n` | |
| fact.c | `120\n` | 5! |
| switch.c | `6\n` | |
| do.c | `3\n` | |
| fib.c | `55\n` | fib(10) |
| ptr.c | `7\n` | |
| struct.c | `9\n` | |
| host.c | 随 OS 变 | **允许跨 OS 不同，绝不允许跨 arch 不同** |

前七个必须 **6/6**；`host.c` 按 OS 分三组，每组 2/2。`main` 返回值即退出码，也必须全 fold 一致。

### 5.2.1 差分测试 [A-17] —— P-6 的唯一仪器

`unisa acc` 只证明 **net ≡ gold**（P-3）。它对 **gold ≡ C99**（P-6）**完全沉默**，而 gold 已被实现打脸六次，每次 acc 都是满分：

| | 缺陷 | 被什么发现 |
|---|---|---|
| E-2 | `type` 的 illegal 行被 1/19 降权 | acc 卡 0.999 |
| E-15 | `parse` 的 stmt 行漏 struct/enum/typedef | 编译报错 |
| E-16 | `gate` 挂在 `isel` 上，而 gate 是 OS 事实 | lowering 发错 gate |
| — | `type` 缺 `ptr − num → ptr` | **差分测试**（静默错答案）|
| — | `,` 判成 illegal；TOKS 缺 goto/union/位运算符 | 编译报错 |
| — | 字符串字面量无 NUL 结尾 | **差分测试**（静默错答案）|

**[A-17]** `tests/difftest.sh`：同一份源码交给参考编译器（`cc`）与 `unisa`，比对 stdout 与退出码。探针集在 `tests/c/`，分两类：`a_*` 已知应支持、`b_*` 边界探针。**每次改动 gold 或走查器后必须跑。** 它同时把"C99 覆盖是子集"从定性描述变成可测数字。

> 参考编译器需要 `printf` 的声明而我们尚无 `#include`，所以对拍只给**参考**那一侧补 `#include <stdio.h>`，其余一字不改。

**[A-18]** `tests/native.sh`：在宿主平台**真实执行**镜像并与解释器比对（见 E-27）。
**原生执行是比解释器更强的 oracle** —— 解释器内存**清零**，会掩盖未初始化读；真机栈是垃圾，会把它们抖出来（`b_static` 即如此暴露）。

**[A-25]** `tests/corpus.sh`：外部语料 [c-testsuite](https://github.com/c-testsuite/c-testsuite)，220 个单文件 C 程序，**不是我们写的，也不是为我们写的**。三类判定：

| 类 | 含义 | 是否失败 |
|---|---|---|
| `pass` | 编译并跑出期望输出（**编成本机镜像真跑**，宿主无匹配目标时才退回解释器） | — |
| `unsupported` | 前端**拒绝**该程序 | 否，这是诚实的覆盖缺口 |
| `wrong` | 编译通过但输出不符 | **是**，这是误编译 |
| `knownfail` | 列在 `tests/corpus.knownfail` 的非 C99 扩展 | 否；但它**开始通过**时判失败，防止清单腐烂 |
| `slow` | 超过每程序时限（默认 10 秒） | 否；记账用。纯 Python 的 tape VM 上八皇后要十几分钟，这正是改走原生执行的原因 |

`tests/corpus.baseline` 是棘轮：`pass` 只能升不能降。

> 自有探针度量的是**我们想得到的东西**；外部语料度量的是**我们实际覆盖了什么**。两者的差距就是 E-30 里那六个缺陷。

### 5.3 验收清单

| ID | 断言 | 依赖条款 |
|---|---|---|
| **A-5** | `unisa run examples/hello.c --fold` → 6/6，`hello from C99\n` | X-1..4, L-1 |
| **A-6** | fact→120、switch→6、do→3、fib→55、ptr→7、struct→9，全部 6/6 | A-4 |
| **A-7** | `host.c` → 每个 OS 组内 2/2 | A-4 |
| **A-8** | `--fault osx_class_bit` → **4/6** | A-3, C-2 |
| **A-9** | `unisa compile` → 三魔数正确 | I-1..3 |
| **A-10** | 同输入编译两次 → 字节相同 | D-5 |
| **A-11** | `weights/parse.i8.unisa` 以 `554e5331` 开头 | Q-1 |
| **A-12** | `unisa acc` → **每阶段 = 1.000**，验的是**出货的构造权重**（8,484 key 全枚举，18 个阶段；2026-09-26 实测）；`--trained` 才看 SGD 对照组，且**不作门槛** | F-3, P-3, U-5 |
| **A-13** | `unisa quant` → 每个出货阶段在其记录 dtype 下 argmax 不变 | Q-6, P-4 |
| **A-14** | 构造两次 → `built.uns2` 字节相同。**套件不再训练**：训练是分钟级满核工作、不在出货路径上，曾把套件变成两小时的活 | D-3, D-6, U-5 |
| **A-15** | `unisa ship` → kit 四件套齐全 | Q-10 |
| **A-16** | Python kernel 与 `unisa_core.c` 在 FULL gold 上逐 key 同类 | K-4 |
| **A-25** | `tests/corpus.sh` → `wrong = 0`，且 `pass` 不低于 `tests/corpus.baseline` | P-6 |
| **A-26** | 在**真 Linux 内核**上执行发出的 ELF（不是解释它）。默认由本机 Lima 虚机承担（[A-32]），GitHub 的 runner 只在把目录挪回来之后作为无尘室复核 | I-1, I-12, X-3 |
| **A-27** | `tests/crossnative.sh` → lnx/x86_64、lnx/arm64、osx/x86_64、win/arm64、win/x86_64 在真机上与解释器逐例一致；**虚机没开就跳过并点名**，`STRICT=1` 时跳过算失败 | I-12..14, X-3 |
| **A-28** | `unisa fat` 产出 Mach-O universal，两个 slice 都执行且与解释器逐例一致 | I-19 |
| **A-32** | `tests/linux.sh` → 整套套件在**本机虚机的真 Linux 内核**上跑过（2026-09-21 实测：`difftest 78/0`、`tools 11/11`、`corpus 209`；2026-09-23 复测 `run 11/11`、`ape 3/3`）。虚机把仓库**只读**挂载，而套件要写（`build_ref.sh` 在源码旁边生成 `unisacc.c`），所以先把树拷进客机再跑；`corpus/` 软链回挂载点，因为套件只读它。**Linux 走 Lima 而非 UTM**：UTM 里那两台 Linux 没装 QEMU guest agent，`utmctl exec/file/ip-address` 一律失败，网络 Shared 无端口转发、MAC 不进宿主 ARP 表，所以也没有现成 SSH 路；Windows 那台**装了** agent，所以 UTM 管 Windows。**CI 只做测试，不做构建**（2026-09-23 定）：交付物在本机一个环境里交叉编译出全部目标，再经 GitHub release（在途时用草稿）搬运；Actions 只是干净机器上的第二意见。仓库转公开后 runner 免费，`.github/workflows/ci.yml` 已恢复在推送时触发，但它仍然不是产物的来源。当初把 workflow 挪走是因为：runner 是计费的，macOS 那两个按 10 倍计价，而这台机器上 macOS（原生）、Linux（Lima）、Windows（UTM）**三个平台都有**，六个目标全都能真跑。**按下推送就烧一次额度**，等于把钱花在这台机器不花钱就能做的事情上 | A-26, A-27 |
| **A-33** | `tests/stages.sh` → 每个探针上，Python 前端问过的每个决策阶段，C 前端也问过。**所有别的套件量的都是答案**，而一条与表一致的手写规则答案与表完全相同 —— 只有这一条能看见「决定是不是网络做的」 | T-2, E-52 |
| **A-34** | `tests/closure.sh` → 每个探针、每个目标，`unisacc FILE -b os/arch` 写出的镜像与 Python 后端从同一条 tape 写出的**逐字节相同**；宿主目标的镜像真跑，输出与 VM 一致。一个头字段、一个位移、一个 REX 前缀错了都会在这里现形 | S-6, E-55 |
| **A-35** | `tests/nativeboot.sh` → cc 编出的 unisacc `-b` 造出 unisacc（N1），N1 造 N2，N2 造 N3：**N1 = N2 = N3 逐字节**，且 N2 交叉写出的其余五个目标与 cc 编出的 unisacc 写出的相同；UTM 的 Windows 机器开着时，win/arm64 与 win/x86_64 的 unisacc 在 Windows 上各自重建自己，逐字节相同。**全程没有 Python** | S-6, E-55 |
| **A-36** | `tests/ablate.sh` → 把某个阶段（或某个输出头）的答案**旋转成错的**，重编全部探针的六个镜像：必须有镜像变了，或者编译被拒。问过一个网络不等于听它的——[A-33] 只能证明"问过" | S-6, E-57 |
| **A-37** | `tests/run.sh` → `unisacc -run FILE.c` 编译并在内存里运行，stdout 与退出码都要与系统 cc 编出的二进制一致。不写镜像，也就没有代码签名这一步 | S-9, E-56 |
| **A-39** | `tests/cli.sh` → 在一个与本仓库无关的空目录里，把编译器当**工具**用：自带头文件、`-I`、`-D`、shebang、argv、退出码，以及"文件不存在要被诊断"。一个能用的编译器，是在它是你手上唯一一个文件时也能用的 | S-11, E-59 |
| **A-38** | `tests/ape.sh` → 构建 `unisacc.com` 并在本机运行它：检验头部（shell 必须接受第一行）、脚本里的偏移（BSD tail 的八进制陷阱）与切片本身。其余平台由 `linux.sh` 与 `crossnative.sh` 的 Windows 机器承担 | S-10, E-56 |
| **A-41** | `tests/layout.sh` + `tests/datashape.sh` → 数据布局的**枚举**验收：`lower.zero_last` 是纯函数，又有 Python 参考实现与 C 移植两套，于是枚举它的定义域——所有长度 ≤3 的数据定义序列（字母表 `{bss 1, bss 8, str 1, str 7, str 8, 全 NUL 的 str}`，长度取 1/7/8 才看得见 mod 8 规则），加上"同一符号定义两次"的变体，762 条 tape × 6 个目标 = 4,572 次逐字节比对，12 秒。`datashape` 则**生成**声明序 ≠ 地址序的 C 程序，并在比对字节之前先断言"这种形状确实出现了"——否则生成器退化后套件会在什么都没测的情况下继续绿 | A-34, E-61 |
| **A-42** | `tests/bigclosure.sh` → 对**编译器自己**做闭环：707 KB、1,874 个数据符号，六个目标的镜像与 Python 后端逐字节相同，且宿主那份镜像必须是一个能工作的编译器。90 个探针全绿而它六个目标全崩过——抽样对"只在大程序里自然出现的形状"是系统性失明的 | A-34, E-61 |
| **A-43** | `tests/consts_check.py` → `unisa/lower.py` 的常量链（`SCRATCH`、`PRINTMAX`、`WIN_HSTD`…`WIN_EXTRA`）与 `src/unisacc_back.c` 里对应的**字面量**逐个相等，共 17 个数。C 后端是手抄的，它只带结果不带推导：改一个 Python 常量会一次作废六个 C 数字，而唯一会发现的是 closure 报两张镜像在某个字节不同——那是真的，但它说不出为什么。这一条说得出 | A-34, E-61 |
| **A-44** | `tests/c99.sh` + `tests/c99/` → 每个 C99 特性一个探针，与系统 `cc` 对拍**输出**（编译通过不算通过）。分母写自标准自己的变更清单，**不写自我们的支持范围**——分母跟着分子动就什么都没量到。`corpus`（基线 216 通过、4 个登记的已知失败）太宽容：那批程序又短又重叠 | A-25, S-10 #9 |
| **A-45** | `tests/bench.sh` → 编译耗时作棘轮：编译自己（707 KB）、一个小探针、以及**出货二进制与 cc -O2 构建之比**。速度从来没被量过，所以可以随便烂掉——一个每 token 调用一次、每次从头走表的查找，把 57% 的自编译时间放进了 strlen，而没有任何仪表会发现。基线按机器分文件，容差 30%。**一次耗时为零的测量等于什么都没跑**，所以有下限检查：这个套件的第一版把 "command not found" 的 15 ms 当基线记了下来 | S-10 #10, #11 |
| **A-46** | `tests/fuzz.sh` + `tests/gen_prog.py` → 从种子生成随机 C 程序，与系统 `cc` 对拍**输出**。别的套件测的都是**有人想到的东西**；活下来的缺陷在**没人组合过的组合**里。生成规则必须让每个程序行为**有定义**，否则两边有权不同、套件就只是噪声——其中一条规则是用一个 bug 换来的：**循环计数器在循环体内只读**。第一版允许循环体给它赋值，种子 2 生成的程序在 cc 下也跑了二十秒以上；一个会写出不终止程序的生成器，量的是超时不是编译器 | A-17, S-10 |
| **A-47** | `tests/hostile.sh` → 编译器遇到**意料之外的输入**时必须**退出**：截断的文件、未闭合的注释与字符串、include 循环、5000 字符的标识符、嵌套一千层的括号、二进制垃圾、嵌入的 NUL、自我展开的宏、一个目录当输入。判据分两类：(1) 不许崩、不许挂；(2) C 说无效的东西**必须被诊断**——静默接受非法 C，等于产出一个作者从没写过的程序。当天就抓到两个：未闭合注释被静默吞掉，`struct S { struct S inner; }` 被静默接受（`sizeof` 是半成品表项里碰巧的值）。两个都已修 | A-40, S-12 |
| **A-48** | `tests/docs.sh` → `prd.tree.md`、`prd.map.md`、`README.md` 里的阶段表由 `python3 -m unisa docs` 从 `unisa/gold.py` 与构造权重**生成**（标记区 `<!-- stages:begin -->`…`<!-- stages:end -->`），过期或标记丢失即失败。由来：阶段表在四个文件里各手抄一份，11→14 重构后其中两份照旧描述 11 阶段的编译器整整一天（`type 960`，实为 4,275），没有任何机制发现 | S-10 #5 |
| **A-49** | `unisacc --check-oracle` → 模型能被问到的**每一个**问题（2026-09-26 实测 20,184 个问题，0 分歧）经缓存问一遍，与网络直接作答逐个比对，正序、逆序各一遍（驱逐与碰撞取决于顺序）。由来：oracle 缓存的第一版存的是问题的**哈希**、比的也是哈希；全定义域上有两个问题撞同一个哈希，命中时就静默交出另一个问题的答案。哈希计算还有有符号溢出（UB），clang 与 gcc 在 -O2 下算出不同的值，于是错答案在两个编译器之间**挪位置**——Linux CI（gcc）拒绝了一个 Mac（clang）能编的 C99 程序。把比较退化回"只比部分字段"时，这条报出 6,646 个错答案 | A-33, P-3 |
| **A-50** | `tests/gold_audit.py` → `type` 表对照系统 cc 逐键审计：10 个数值类型两两 × 14 个二元运算（1,400 键），cc 用 C11 `_Generic` 答结果类型、用编译错误答"非法"，逐键与 gold 比对。**枚举只能证明网络等于表，这一条问表是不是 C**——论文 §8 所述局限的第一个仪表。首跑即发现 200 个键错（比较运算的结果类型写成了 long，与 [G-2] 同源），已在 gold 中改正；唯一的有意偏离（`i64 & i64` 这一行编码的是取地址）记在 `tests/gold.knownfail` 并写明理由 | P-3, S-15 A1 |
| **A-51** | `tests/abi_audit.py` → `abi` 表的系统调用号对照**本机**的 `<sys/syscall.h>`：每台机器只担保自己那一格（本机 osx、Lima lnx/arm64、CI 的 Linux runner lnx/x86_64）。某平台没有对应系统调用的 op，catalog 必须写 `none`、由 lowering **拒绝**，而不是借一个号码——首跑发现 macOS 的 `nanosleep` 映射到 240，即 `SYS_listxattr`；`clock_gettime` 映射到参数完全不同的 `gettimeofday`。两个后端现在都拒绝这种 op | C-1, S-15 A2 |
| **A-31** | `tests/tools.sh` → 别人的库代码（crypto-algorithms，八个算法，每个多文件、自带已知答案测试）与 `cc` 同输出，且 `pass` 不低于 `tests/tools.baseline`。**语料清零只说明前端不拒绝，不说明跑对** | W-14, I-22 |
| **A-30** | `tests/multi.sh` → 两个翻译单元编译成一个程序，与 `cc a.c b.c` 同输出，`--fold` 6/6，且本机镜像真跑。语料构造成**共享会被看见**：两个单元各有同名不同值的 `static` | W-14 |
| **A-29** | `tests/selfgap.sh` → unisacc 接受的程序数不低于 `tests/selfgap.baseline`（自举差距只能缩小）。**A-23 的不动点不是覆盖率**：unisacc.c 只需接受它自己用到的子集，于是三十次提交里前端特性单边堆在 Python 侧而套件量不到 —— `selfhost.sh` 只比词法器，`ccrun.sh` 遇到拒绝就打印 `UNS` 走人。这条把那个数变成棘轮 | A-20, A-23 |

### 5.4 体积与速度预算 [B]

头号主张是体积，就把它量出来，摆到现有方案旁边。

| ID | 指标 | 怎么测 | 目标 |
|---|---|---|---|
| **B-1** | 总 θ | 所有网络求和 | 记录 |
| **B-2** | kit 字节 | `unisa ship` 按逐阶段最低 dtype | 权重 **≤ 32 KB**；kit ≤ 64 KB |
| **B-2a** | **对比基线必须写明** | **训练**权重比裸表大（E-3），故其体积主张只能对"手写算法代码"成立；**构造**权重比裸表小 2.19×（E-22），无此限制 | 报告时并列四列：裸表 / 训练权重 / 构造权重 / tcc |
| **B-3** | 对比 tinycc | `size $(which tcc)` 或 tcc 发布二进制 | 记录比值 |
| **B-4** | 决策吞吐 | `unisa bench`，逐阶段，冷 | Python ≥ 50k/s；`unisa_core.c` ≥ 5M/s |
| **B-5** | 端到端编译 | `unisa run examples/fact.c` | ≤ 1 s |

**B-6** `unisa bench` 必须报**冷**数据。在确定性全函数上加 memo 缓存是正当工程手段，`run` 可开；但 `bench`/`acc`/`quant` 必须关——否则测的是 dict，不是 kernel。

### 5.5 完成度（四层判据，当前）

盘点分四层，因为**难的部分和多的部分不是同一部分**：论点层基本做完了，产品层才走了一半。每层的"100%"是一条可测的判据，不是一个百分比。

| 层 | 目标 | 完成判据 | 实测（2026-09-21 起，2026-09-25 复核） |
|---|---|---|---|
| **S-1 命题** | 每个表形状的决策点都是网络，结构性代码不神经化 | 全 stage `acc = 1.000`（FULL gold 穷举）+ [P-1] 不透明性 | **已达**。18 个 stage，8,484 key 全枚举，零分歧（2026-09-25 复核） |
| **S-2 目标** | 一条 tape → 六份镜像，stdout/exit 一致 | 六个目标**在真机上**逐例一致 | **已达**。`unisa fat` 是**一个 OS 内**的多架构；cosmo 式一文件多目标（MZ + shell 自选切片）**已达**，见 S-7 #9；真正同时合法为 ELF/Mach-O/PE 的**单一字节序列**仍未开工 |
| **S-3 语言** | C99 子集覆盖别人写的代码 | 外部语料 `unsupported 0`、`wrong 0` | **已达当前语料，含浮点**（2026-09-22）。`corpus 220 pass 214 wrong 0 unsupported 0 knownfail 6`；余下 6 个全是 C99 之外的扩展（GCC 语句表达式、空结构体、`push_macro`、C11 `_Generic`）与 [G-2] 的 64 位整数求值，见 E-54 |

S-5..S-17（历史推理、0.0.7/0.0.8 计划、S-17 迁移决定）已归档：[prd-history-20260929.md](archive/prd-history-20260929.md#5-5)。当前版本状态见 §0 的 v0.0.11 摘要与 v0.0.12 计划树。

### 5.6 覆盖限制登记（当前）

- 三字符组（trigraphs）未实现；C99 24 个标准头缺 6 个（complex/fenv/locale/setjmp 之外的清单见归档 5.7）；`<setjmp.h>` 是评估过的永久非目标（tape 无间接跳转）。
- `fork`/`exec`/`popen` 未暴露给用户代码（目录列举已补）→ R12-2 ⑥。
- 位域 sizeof（2026-09-27 巡查）已修复并复验。

原巡查记录见 [归档](archive/prd-history-20260929.md#5-7-5-9)。

## 6. 实验发现 [E] —— 面向论文（已封存）

E-编号的已证实/待验证/已证伪/开放问题/复现/先行研究全表移至 [archive/prd-findings-20260929.md](archive/prd-findings-20260929.md)；论文按 E-编号引用时在该文件解析。新的实验结论直接写进对应版本的回执与 `research/*.json`，不再在 prd 累积叙事。

## 未来猜想与探索区 [FX]

这里只记猜想，不排期、不验收。一条猜想要进入实施，须先改写成带编号的条款，放进前面的章节。前缀用 `FX`，因为 `X` 已是 §4.6“目标机执行”的条款前缀（X-1…X-3）。

### FX-1 可移植 tape 包（主人 2026-09-27 提出）

**动机**：应用内嵌 `libunisacc`（或提供 `unisaccrun()`），分发的是一个跨架构的中间文件，而不是 `.c`；这个文件本身就是一个包，将来也用作包升级的单位。类比 `.class` 与 wasm。

**已有基础**：
- 同一条 tape 在 6 个目标上的可观测行为必须一致（P-7，由 fold 与 closure 测试强制）。
- `--from-tape` 只走后端，从 tape 直接得到本机代码。
- `-run` 在内存里映射代码执行，不落盘，也不触发 macOS 首启扫描。
- `libunisacc` 实质上就是把“后端 + `-run`”暴露成 API。

**缺口**：
1. **目标中立**：tape 在预处理之后生成，`_WIN32` 这类目标宏的分支已经定死。两种办法：用一个“中立目标”生成（约定不写目标分支），或者一个包里放 6 份 tape、加载时挑选。
2. **格式冻结**：现在没有文件头、版本号和 op 集版本。需要冻结一个版本化的 op 子集，文件头包含魔数、版本、目标和内容哈希。
3. **加载前校验**：加载 tape 等于执行任意代码。需要校验器：跳转目标合法、栈与 `.frame` 平衡、副作用只经由 `.print`/`.write`/`.exit` 及声明过的调用。之后才谈签名。
4. **保密预期**：tape 接近汇编，标签里保留函数名。去掉符号能挡住随手翻看，挡不住认真的逆向，大致与 `.class` 或 wasm 同级。
5. **包与升级**：tape 是整个程序连同内嵌的 libc，天然是单文件包，整文件替换加哈希即可升级。增量补丁和按模块加载需要链接层，而多个翻译单元目前在编译期就合并了（W-14）。

6. **二进制编码 `.tapebin`**：`.tape` 是汇编型文本，分发单位应当是它的二进制编码。编码方式：1 字节 opcode；两个寄存器号合成 1 字节（r0–r7 各占 4 位）；立即数用变长整数；标签与符号进符号表，指令里只存索引；字符串和数据进常量池（与 wasm 或 `.class` 同类）。文件头（魔数、格式版本、op 集版本、目标、内容哈希）成为格式的一部分。好处是体积小、加载时不用解析文本，第 3 点的校验也只需检查编码合法、索引在界内。这一层只是编码，不改变第 1 点的跨架构前提，也不改变第 4 点的保密预期。验收：`tape → tapebin → tape` 逐字节往返一致；fold 测试直接跑 `.tapebin`，6 个目标结果不变。

**前置条件**：S-17 完成，E7 切换到 `.com` 之后。

### FX-2 libunisacc 与 crate：给别的应用当脚本引擎（主人 2026-09-27 提出）

**动机**：让其他应用把 unisacc 当作嵌入式脚本引擎，执行动态 C、`.tape`，将来还有 `.tapebin`（FX-1）。它比解释型脚本引擎快，因为输出的是本机代码；一个引擎覆盖六个目标；自带头文件，不依赖系统工具链。

**现状（2026-09-27 核对）**：没有库形态。`-run` 已并入本体（E-59），产物只有单文件 `unisacc.com`；`src/main.c` 是进程入口，编译器自带 `_start` 和 syscall，不依赖 libc。

**两级路线**：
1. **子进程封装（现在就能做）**：crate 捆绑 `unisacc.com`（或只带目标平台对应的切片），API 形如 `run_c(src, args, timeout)`、`run_tape(tape, args, timeout)`，内部调用 `unisacc -run` 或 `--from-tape`，拿回 stdout 和退出码。子进程天然隔离，崩溃、死循环、越权 syscall 都不会伤到宿主，超时直接杀掉（注意 alarm 只约束它 exec 的那个进程）。
2. **进程内库 `libunisacc.a`（需要改造）**，缺口有五个：
   - 符号冲突：本体自带一份 libc 实现（printf、malloc 等，x86 上约 27.7 KB 代码），与宿主的 libc 或 Rust std 链接时会重名，需要加前缀、不导出；
   - 全局状态：全是静态大数组，bss 约 610 MB 虚拟空间，不可重入、非线程安全，需要收进一个堆上的上下文结构；
   - 错误处理：出错路径直接 `__exit`，会把宿主一起结束，需要改为返回错误码；
   - 输出形态：API 返回内存里的机器码或镜像，由宿主决定是映射执行还是交给子进程；macOS 进程内执行要处理 hardened runtime 下的 `MAP_JIT` 权限；
   - 宿主交互：生成的代码只会直接发 syscall，没有调用宿主函数的机制。脚本引擎需要一套宿主函数导入约定（tape 层声明、lowering 层绑定），并配上 FX-1 的加载前校验，否则就只是“运行一个外部程序”。

**安全前提**：在进程内执行动态代码等于把宿主的全部权限交给脚本。没有校验器（FX-1 第 3 点）或沙箱之前，只推荐子进程这一级。

**前置条件**：第 1 级随时可做，而且不改编译器；第 2 级排在 S-17 与 E7 之后，并与 FX-1 的格式冻结一起设计。

### FX-3 tape → wasm：第 7 输出目标（主人 2026-09-27 提出）

**定位**：保留 tape 作为内部 IR，**新增**一条从 tape 到 wasm 的输出后端，不是把 tape 换成 wasm。它服务的是没有我们运行时的外部环境（浏览器、wasmtime、WASI）；六个原生目标和 S-17 都不受影响。与 FX-1 的 `.tapebin` 互补：`.tapebin` 是无损序列化，由我们自己的运行时加载；wasm 是翻译，控制流结构上有损，交给别人的运行时。

**为什么不整体替换**（2026-09-27 评估）：
- tape 是寄存器形态（r0–r7），降级几乎一对一，所以能查表、能做成 delta；wasm 是栈机，六个目标的下沉层都要改为做寄存器分配。
- tape 用任意标签跳转；wasm 只有结构化控制流，需要 relooper 或 stackifier。
- r7 软件栈可以取地址；wasm 需要在线性内存里另设影子栈。
- 标准 wasm 是 32 位地址，要守住 LP64 契约就得用 memory64。

整体替换粗估要重写现有 C 源码的 50–65%（front_parse 的生成部分、opt 的 H1–H4、back_lower，以及 Python 孪生实现、以 tape op 为键的表网络、exec/ 的 E3/E4/lower），对六个原生目标没有收益，本体代码估计还会增大 10–25%。

**第 7 目标需要做的事**：
- 控制流结构化（只在这个后端里做）；
- 影子栈与 memory64（或者明确声明为 32 位子集）；
- `.print`/`.write`/`.exit` 映射为 WASI 导入；
- 等价验收：wasm 的运行结果与 `vm(tape)` 的可观测行为一致，并入 fold 测试。

规模估计是一个新后端，约 3,000–5,000 行 C 加上 Python 孪生实现。

**边界**：js/wasm 的研究线归 Paper B / `ujs/`（csr，见第 5 条“js / wasm 这条线”）。FX-3 只是 unisacc 后端的一个输出目标，不碰 `ujs/`，也不以 ujs 的编译器为前提；立项时要与 csr 对齐，避免重叠。

**前置条件**：S-17 与 E7 之后；与 FX-1、FX-2 共用文件头和校验器的设计。

### FX-4 规格优先：规则化表规格、分层覆盖、δ 最小化（主人 2026-09-27 提出问题）

**问题**：构造路线的方向是对的，但灵活性不够。这里说的不是统计泛化，而是工程上的：改动的局部性、可组合、可复用、新增构造与目标的成本。

**诊断**（两位独立研究员的结论一致，2026-09-27）：瓶颈是**规格缺位，只剩外延**。表与 δ 是从手写参考实现转写出来的结果，规则本身（内涵）丢了，所以任何改动都只能整张重铺、全量穷举。具体是：
- 解析 δ 约 5,230 个状态、约 130 万个条目，是文法、属性、模板预先相乘的结果，相当于一次没有保留源码的部分求值（Futamura 第一投影）；
- 表之间没有组合运算（没有覆盖、积、串联）；
- 验证是全域的，没有“这次改动影响哪些 key”的依赖信息。

字节流接口**不是**瓶颈：它保证阶段能按变换器组合，应当保留并加 schema，不要换成共享内存。“多表合并有害”的实测结论也说明：共享应发生在构造期，不该发生在推理期。

**总原则**：规则是源头，表是可重新生成的产物；参考实现降为逐字节对拍的对照。这不违反确定性与穷举验证，也符合“旧编译器留作行为参考”的口径。

**候选思路**（都在构造期，不改执行器；实验设计已有，未运行）：
1. **规则化表规格**：每张表写成有序守卫规则，首条命中；生成器枚举 key 域得到表，同时输出“每个 key 由哪条规则产出”的覆盖图。验收：对全域 rules(k) 等于参考表。
2. **分层覆盖（默认加例外）**：表等于基表加有序补丁，补丁是（key 模式到类别）；生成期合并成一张扁平全函数表，运行时不变。
3. **δ 最小化**：把解析 δ 看成带输出的变换器，先做输出规范化，再做 Hopcroft 划分精化，得到行为逐字节相同的商机器，并保留反向映射。一次测量就能量出 5,230 个状态里有多少是冗余，决定下一步的收益上限。
4. **因子化（Ashenhurst–Curtis）**：找划分 {A,B} 使 f = h(g(A), B)，g 的像域小。
5. **文法机、属性机、模板变换器的同步积**：新增构造只加产生式与一条模板，规模相加而不是相乘。E3 的结构化设计（research/e3-structured.md）已经走了这一步。
6. **类型化的阶段接口**：仍是字节流，但每个格式有 schema，清单连接时检查，并从 schema 生成两端。
7. **增量验证**：由覆盖图得到“规则到 key 区域”的依赖，只重验受影响区域，其余靠哈希证书；定期全量验证兜底。
8. **受限等价饱和**：只用于优化阶段，只在生成期、限定窗口和步数，抽取结果必须与参考逐字节一致。

**推荐先试**：1 与 2（只改生成层，秒级实验，且是其余思路的地基），加上 3（一次测量）。

**粗测（2026-09-27，对 `weights/gold/*.tsv` 已导出的真值表，只读、纯 Python；方法粗糙，仅供定方向）**：

| 表 | 行数 | 结果 |
|---|---|---|
| type | 4,275 | 可分解为 h(g(t1,op), t2)，g 只有 39 类；存储约 870 项，压缩约 5 倍 |
| peep | 1,632 | 91.5% 是默认值，约 118 条规则可表达，压缩约 14 倍 |
| enc | 438 | lnx 与 osx 在两个架构上各自完全相同，只有 win 不同；覆盖层成立，win 只需补丁 |
| abi、combo | 438 | 六个目标两两之间有 30% 到 100% 的项不同，是真数据；跨目标覆盖帮不上。**新增目标靠覆盖只对 enc 成立** |

限定：这些表合计只有约 8,900 行，本来就不大；真正的体积在解析 δ，其三源结构化（E3）正在进行，所以思路 5、3 有一半已在路上，缺口是其余阶段和覆盖层。

**与硬约束冲突的方向**（不做）：
- 在推理层组合、共享或“软”补全：把逐字节精确变成近似，还破坏“改一张表只重建一张”；
- 运行期的等价饱和或特化：结果依赖搜索预算，执行器也会远超几 KB；
- 用共享内存的 IR 取代字节流：丢掉阶段隔离与“未覆盖即拒绝”的边界。

**前置条件与归属**：不阻塞 S-17。δ 最小化的测量需要读取 exec/ 的生成物，建议由 cdx 顺手做一次；思路 1、2 可在 E3 结构化收尾之后，先拿 peep 与 enc 两张表试点。


### FX-5 分层 libc：直接系统调用 / 自研纯计算 / 声明并运行时导入系统真身（主人 2026-09-27 提出，两位研究员核对）

**动机**：unisacc 自己从零写 C 库（不用系统 libc），今天已经给 execve、getdents64 这两个纯系统调用接上了真实实现（隔离分支，未合并）。主人提出更大的问题：要不要学 tinycc，做一套"只声明、运行时接目标机器真正的系统 libc"的机制，一次性拿到 fork/exec/opendir/sysctl/pthread/dlopen/locale 这一整片目前完全没有的能力？同时指出两个具体难点：用户程序自己提供同名实现时怎么处理；tinycc 这种做法本身"兼容性不太好"，要弄清楚为什么。

**建议的三层，只新增一层**：
- **L0 直接系统调用**（现状不变，已验证稳定）：open/read/write/close/execve/getdents64 这类很薄的内核包装，参数只有整数和指针，不依赖 libc 内部状态。
- **L1 自研纯计算**：string.h、printf、malloc（基于 mmap）等——这是自研库的核心价值（确定性、自包含），不外包。
- **L2（新增）声明 + 运行时导入系统真身**：只用于 L0/L1 做不到、也不该自己做的部分：`dlopen`/`pthread`/`locale`/DNS 解析等。Windows 现在已经有一个范围很窄的同类机制（一张"符号名→导入源"的固定表，`weights/gold/abi.tsv` 的 `winimp` 列），可以推广成跨平台的 `libspec` 表（字段：name、layer、各平台的 import_symbol、version、family、owner_allocator、layout_id）。

**判定某个函数该进哪一层，看两条**：
1. 调用者要不要**直接读**它返回的结构体内部字段？只经函数访问、从不自己拆开看的（不透明句柄，如目录句柄本身）适合进 L2；调用者要直接读偏移的（透明布局，如目录项里的文件名字段、`struct stat`）必须继续留在 L0/L1 自己填，否则声明的布局和目标机器真实库不一致时会**静默读错**，不报错。
2. 是否依赖进程级隐式状态（TLS、线程列表、动态加载器的内部表、locale 数据库）？依赖的只能进 L2，自己实现一套会和系统真正接入的库互相打架。

**用户自己提供同名实现时怎么办**：单遍编译没有传统链接阶段，但可以在"整份源码读完、生成代码之前"设一个统一的符号终结点，按顺序解析：用户定义 > 编译器自研实现 > 声明并导入系统真身 > 报错未定义。自研实现之间必须通过同一套符号槎互相调用（不能内联死），这样用户重写 `malloc` 时，自研的 `calloc`/`strdup` 会自动跟着换成用户版本。另留一个逃生口（类似 `__real_opendir`）给"既想覆盖又想调用系统原版"的场景。一致性检查：覆盖了一个资源族的一员（如 `malloc`）却不管其它成员（`free`/`realloc`），要报错或警告，不能静默放过——这是 tinycc 类工具常见的崩溃根源。

**tinycc"兼容性不好"的六个具体根子**（不是空泛的"ABI 不稳定"）：
1. 透明结构体跨版本/跨架构布局漂移（`struct stat` 在 x86_64 与 arm64 上字节数都不同）；
2. glibc 符号版本（同一个函数名绑定到不同版本，行为可能不同）；
3. 很多"函数"其实是宏或内联，真正的符号名完全不同（`errno` 在三个平台是三个不同的东西）；
4. 一旦引入系统线程，自研的全局状态（自己的 `errno`、`malloc` 锁）不再线程安全，且不在系统 TLS 里；
5. 两套 `stdio`/两套分配器混用会导致输出错序或跨分配器释放崩溃；
6. Windows 的 CRT 本身就有多个互不兼容的版本（msvcrt/ucrtbase/api-ms-win-crt-*）。

每一条都有对应的缓解方法（按平台固化透明布局、显式绑定符号版本基线、给每个符号记录"直接符号/调用 getter/数据重定位"三种访问方式、一旦用 L2 就整族切到系统的程序启动流程、stdio 与 exit 整族切换不混用、Windows 只导入 kernel32/ntdll 与固定的 ucrtbase 集合）。

**最小验证实验**（≤60 秒）：12 个探针程序分四组——透明布局（`readdir`/`stat`/`localtime_r`）、不透明句柄（`opendir`/`getaddrinfo`）、隐式状态（`pthread`/`dlopen`）、用户覆盖（自定义 `opendir`/`malloc` 配 `__real_` 逃生口），各自编译 L0 版和 L2 版，在 macOS、glibc Linux、musl 三种环境跑，比对与系统 cc 的逐字节差异、崩溃、超时。

**结论：值得做，但范围要窄**——L2 只当"L0/L1 做不到的出口"，不是拿来替换现有的自研库。确定性和单文件自包含仍是主线卖点，L1 必须保留；透明布局类（`readdir`/`stat`）继续留在 L0，除非实验证明 L2 更好。

**第一小步（未排期）**：把 Windows 现有的固定导入表推广成跨平台 `libspec` 结构；先在 Linux 上只接一个完全不涉及透明结构体的功能族（建议 `dlopen`/`dlsym`/`dlclose`），落地"符号终结点解析 + `__real_` 逃生口"，用上面的 C、D 组探针验证。做 `pthread` 之前必须先解决"一旦用了 L2 就要走系统的程序启动流程"这个前置问题。

**前置条件与归属**：不阻塞 S-17 与当前正在做的门禁验收；不影响已经验证过的 L0（execve、getdents64，隔离分支）。留给 cdx 判断何时排期。

#### FX-5 补充：混合 libc 的速度与体积评估（主人 2026-09-28，后续思考，未排期）

主人提出“绝大部分转发系统 libc、少量保留内置实现”的混合方案；本轮只记录，不实施、不阻塞当前功能与性能收尾。它是对上面 L1 保留范围的进一步候选，不把“L1 全部保留”或“绝大部分转发”提前当成已定架构。当前内置库是携带的实现，不是默认系统 libc 转发。

**体积依据**：冻结 `9a0ae470` 候选共 6,279,167 B，其中共享模型体 5,922,890 B（94.33%）；19 份头文件/库实现源码 103,636 B（1.65%）。因此仅移除内嵌库源码最多影响这一项，不能据此承诺大幅缩小整个 `.com`。驱动已链接的库代码尚无独立 link-map，节省量待测；系统导入声明、绑定、加载与回退也有体积成本。模型中的动作序列不是全都属于 libc，不能把 94.33% 当成可由系统库替代的部分。完整账见[模型功能与物理字节账](#model-function-bytes)。

**更可能的收益**：用户程序每次包含 stdio 等头文件时，当前路线要处理/编译库函数体；改为声明和系统符号绑定，有望减少预处理、解析与后端工作及用户输出中的库代码。收益未实测，不能直接套用内核提速比例；也不能直接改为解析庞大的系统头文件而假定会更快。

**后续最小评估**：先分账“编译器驱动链接库代码 / 携带库源码 / 用户程序生成的库代码”，再选一个完整库族做私有实验。比较同源、同目标、同优化级别的实际 `.com` 与用户产物大小、包含 stdio 的编译延时、运行行为及启动成本；性能独占计时，功能检查使用有界并发队列。只记录测过的平台。六目标的 ABI/导入方式、`-run` 与原生执行、跨平台自包含与缺库回退、用户同名覆盖、FILE/errno/分配器所有权须一并定义；不得混用两套 FILE 或跨分配器释放。以功能 TDD 与净收益决定采用范围，不开启通用链接器工程。




### FX-6 推理提速猜想：int32 权重 + SIMD 向量化（用户直觉，2026-09-29 提出并澄清为整型；已排入 R12-5 ④ 作为有界可行性片）

**用户假设**：权重统一为 32 位整数（int32）、推理改用高级 CPU 向量指令（NEON / AVX2 / AVX-512）做整数向量运算，也许能让推理速度提升一个数量级。（最初记为 f32，用户已澄清是 int32；不涉及浮点。）

**现状事实**（评估依据，不是结论）：出货内核已是整数阈值网络——第一层收成 64 位掩码的合取，第二层是整数加法与严格 argmax（Paper A §3、`exec/c/run.c`），没有浮点、乘法或 libm；可复现性依赖“整数推理在约定宽度内结果唯一”（Paper A §3.4）。int32 权重与这一前提**兼容**：只要累加范围仍在 32 位内（构造时可逐网证明），结果保持逐位相同。速度剖面上，网络编译器比经典慢 4–17 倍，Paper A §7.1 归因于通用执行器**逐步解释**，而非单次网络求值。

**方向评估**：
- 与现有路线一致：int32 是当前整数内核的自然载体，改动只在**权重布局与内层循环**，不碰规则→构造→网络的语义；全域 net=table 检查可原样复用作为对拍裁判。
- 向量化有机会但幅度存疑：单次推理只触及几十到几百个隐单元；掩码合取已 64 位并行，第二层 int32 累加与 argmax 可用 NEON/AVX2 一次 4–16 路，单步推理理论上快 2–4 倍；端到端能否可见取决于推理段在 `-run` 总时间中的占比（执行器动作、内存搬运、系统调用可能占大头）。“一个数量级”需要推理段占比 > 90% 且向量化收益 ≥ 10×，当前结构下不太可能，但应由剖面而不是推理判定。
- 代价：单文件六目标产物需按 ISA 运行时分发内核（arm64 有 NEON 基线；x86-64 的 AVX2/AVX-512 不能假定存在，需 SSE2 基线 + 运行时检测），内核多出向量版本与自检；与“执行器不加语义原语”的边界无冲突，与体积目标有张力。权重从紧凑记录扩成 int32 会增大包体，除非只在装载后展开。
- **最小实验（≤1 天）**：1) 剖面：`calc.c` 与 `unisacc.c` 自源在 `-run` 下拆出“网络求值 / 动作执行 / 系统调用”三段占比；2) 只对第二层 int32 累加与 argmax 写 NEON 版（arm64），装载时展开为 int32 数组，全域 net=table 不变，同机五次中位对比；3) 若推理段占比 < 30%，此猜想在当前结构下上限约 1.4 倍，记录后关闭；若 > 60%，再评估 x86 AVX2/SSE2 分发与包体代价。

**结论状态**：用户直觉假设，方向合理（int32 权重 + SIMD 与现有整数内核同向，值得一测）；进入 0.0.11 探索区，按最小实验决定是否立项。

## 7. 附录

### 7.1 版本沿革

v1 到 v3.4 的逐版钉死条目，连同 14 阶段之前的模型总表，已归档到
[`archive/prd-history.md`](archive/prd-history.md)：那些数字记录的是当时的口径
（`OPS` 38、9 头、TYS 8→9、selfgap 31/73 等），与今天的代码不符，留在正文里只会
被当成现状读。

**此后的变化**按实验条目记在 §6：浮点 [E-54]、自举闭环 [E-55]、编译即运行与单文件
打包 [E-56]，以及表形决策的对齐（`regmap`/`tyinfo`/`pfconv` 三个新阶段、`isel`
退出 lowering、`abi` 去 `tls` 加 `nrreg`/`arg3..5`）。


**v0.0.10（2026-09-29 发布，tag ae6d512）**
- **发布**：https://github.com/partnernetsoftware/unisacc/releases/tag/v0.0.10 ；签后 `unisacc.com` f6e8e090…（1,168,488 B），未签候选 4ba24140…（1,152,711 B）；本地 332/332；验收 `research/r10-release-acceptance.json`。
- **闭合项**：干净机器 CI（后改为 release-check 快链 + 每周全量）、Windows 企业签名（qualification→company）、Apple 公证、共享 E2/24 网、源码/权重 include 分离、六单目标私有构建、进程内库主 API/线程/生命周期/回调/USLCALL3 别名、`-ftrim-libc` 改名、GHCR 候选封存。
- **顺延到 0.0.11**（已在 v0.0.11 节逐项回执）：V3 variadic 跨来源、pointee 身份、packed / 宽 FP / BANK、Windows SEH、六平台生命周期矩阵、公共 origin0 refinement、Paper A 定稿。
- v0.0.9（`a606ff4`）及 R9 状态、v0.0.10 全计划见归档索引。


**v0.0.11（2026-09-29 发布，tag 8b5abc9）**
- **发布**：https://github.com/partnernetsoftware/unisacc/releases/tag/v0.0.11 ；签后 `unisacc.com` e86cc61c…（1,170,384 B），未签候选 6a3dfce2…（1,154,605 B）；本地 338/338；验收 `research/r11-release-acceptance.json`。
- **[v]**：V3 variadic 跨来源、一层 pointee 身份、long double（IEEE64 profile）、packed 外部布局（AAPCS64/Win64）—— `research/r11-{variadic-import,pointee-identity,longdouble,packed}-evidence.json`；`-ftrim-libc` 默认（住在 E2 网络，`-fno-trim-libc` 退出；calc -run 188.7→142.7 ms）—— `r11-trim-default-evidence.json`；关系账 —— `r11-package-relations.json`；Windows 双目标编译器自举与六目标 crossnative；GHCR 每片重封。
- **[-] 顺延 0.0.12（理由在 R12 计划树对应项）**：general BANK（设计 `research/r11-bank-design.md`）、Windows SEH/六平台生命周期矩阵、us_eval/us_reload/us_opt_verify、网络裁判登记、目录职责梳理、论文定稿。
- 逐项回执全文见 [归档](archive/prd-history-20260929.md#v0-0-11)。

**v0.0.9 / v0.0.8 身份行（自 README 迁入的快照表）**

| 版本 | 身份 |
|---|---|
| v0.0.9 artifact | Synchronized reference/network prune; the build sidecar records the actual source-content closure separately from its build-time base HEAD; `.com` 1,233,236 B; SHA-256 `d4f7d3022a373fb71ad46dde23c72fe450beefad045727122383c85da17c678b` |
| Published v0.0.8 | `10672e3`; unsigned `.com` 5,388,402 B; SHA-256 `948232f00028170d2090983375fbca2a3829ef8f73235baada5deb9db174d737` |

v0.0.9 时代的说明（单次绑定计时、平台范围 d4f7d302、Windows 签名顺延句、有限域检查边界）已随 README 迁入块移至 [archive/prd-history-20260929.md](archive/prd-history-20260929.md#readme-migrated)。

### 7.2 归档索引

prd 只描述当前与将来；过程记录、旧计划与历史数字按时间封存在 `archive/`：

- [prd-history.md](archive/prd-history.md)：v1–v3.4 逐版条目与 14 阶段前的模型总表。
- [s17-migration-log-20260928.md](archive/s17-migration-log-20260928.md)：2026-09-26–28 模型化迁移逐片日志。
- [r9-integration-history-20260928.md](archive/r9-integration-history-20260928.md)：R9 集成史。
- [prd-r9-r10-receipts-20260929.md](archive/prd-r9-r10-receipts-20260929.md)：v0.0.9/R9 状态、v0.0.10 全计划、R9/R10 逐片回执与发布收口、2026-09-27 巡查（5.6/5.8/5.10）。
- [prd-history-20260929.md](archive/prd-history-20260929.md)：本次第二轮清理移出的 §0.3 早期交付边界、§5.5 完成度盘点 S-1..S-17、§5.7/5.9 巡查记录、v0.0.11 逐项回执全文、附录 7.1/7.2/7.4。
- [prd-findings-20260929.md](archive/prd-findings-20260929.md)：§6 实验发现 E-编号全表（论文引用按编号在此解析）。
