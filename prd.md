# unisacc —— 当前状态、架构与路线

> prd 只写**当前与将来**；发布身份与过程记录在 [历史索引](archive/prd-release-and-development-history-20261001.md)。条款全文在 [spec.md](spec.md)（代码注释引用的 `[S-9]`、`[W-16]` 等编号都在那里，只增不改）；未排期构想见§5，旧稿在 [archive/plans/ideas.md](archive/plans/ideas.md)；已发布版本的计划、叙事与证据在 [archive/](archive/)；目录归属见 [ARCHITECTURE.md](ARCHITECTURE.md)。新知识和决定先写进这里再做别的。

## 当前导航

- **[概念与术语](#0-概念与术语先读歧义以本节为准)**：§0，流 / 网 / 段 / 阶段四个词各只有一个意思；六条常见误解逐条对照。
- **[流/网合同三表与可核验进度](#flow-net-contracts)**：七流、十八网、关系表及是否封存；有封不等于已冻。
- [当前版本与有效计划](#current-plans) · [当前架构](#pipeline-design)
- [路线](#5-路线) · [未解决问题](#6-未解决问题) · [归档索引](#7-归档索引)

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
- **零 Python 依赖（硬目标；主人 2026-10-23 重申）**：构建、闸、验证链中 **100% 不依赖 Python**。这不是“保留 oracle”——自举已完成（N22，三 sha256 相等），但当前基线仍差：`python3` 调用点 **2253** 处（`tests/gate.sh` 占 183）、`import unisa` 的文件 **86** 个、剩余 `.py` **452** 个。三者同时清零后，`unisa/` 才移入 `archive/`（不删，仅作人工参考，不参与任何链）。
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

2026-10-23 F1 重封前置修复：隔离短路径触发 strace 对齐空白，PP 采集器原固定 `) = ` 匹配遗漏头流 EOF。已改空格/制表符解析并补三类自测；scratch 正式重封四步与内容变异负例均通过（详见 research/c43-ppcapture-padding.md）。本次仅落采集器修复，F1 产品批与重封件尚未落地，不计 F1 完成。

F1 落地执行口径：每刀在 archive 导出的独立 main 中经 landlock、commitgate --index 创建自洽提交，逐刀补齐受影响派生和 PP 测量，再持锁快进活检出；活检出不运行工具或测试。cx_read 刀置最后，与最终根产品及 sidecar 同锁安装；中间刀不重建 model-com。每次创建及提升提交均 bundle 备份。此为执行顺序，不是全闸通过声明。

F1 第1刀 vsnprintf/vprintf：scratch 已重生成 kernel/facts（30表无差异）、ABI manifest 523项；gen/parse2 键未变。正式 PP 重采四步全 rc0，最大30.66秒。根产品仍为旧根，仅作工具载体；本刀尚非最终产品验收。

F1 第2刀 dup2/fopen 修剪：kernel/facts/ABI 已重生成，PP 四步重采全 rc0（最大31.76秒）；gen 键变为624c68f6d0891b83、parse2未变，同步两实现常量。旧根的内嵌修剪图仍旧，直接跑专项返回1，不能记该旧根为新刀产品；本刀重建经典参考后两项返回0，最终模型专项亦已0。根只在第7刀安装。

第2刀首轮提交被声明覆盖检查正确拒绝：libneed 不覆盖 exec/facts/pp-autoinc-gen.tsv，facts-export/pp-leaf-seal 属合同检查，不能代替产品套件声明。补点 exec-pp-literals（其树声明覆盖 exec/include/seed），保留所有原有派生闸，不修改覆盖表。

路径纪律冲突待主人澄清：本轮禁止任何工具接收点或仓库根作路径，而指定 ident/seal 接口及现行 gate/seed-com 内有 ROOT/点参数。已询问是否仅写入/删除目标禁用；未获答复前不启动后续依赖该接口的验收。第2刀已结证并 bundle 备份，第三刀仅推进源码及不依赖该接口的准备。

第3刀准备发现旧根已无法编译新环境头上的 ABI 工具（undefined _unisa_envfind），不能沿用“直到最后都以旧根载工具”的设想。决定仅在隔离 scratch 内提前采用已验最终根作工具载体；中间树不作该产品的 ident/PASS 声明，活检出继续保留原根，最终仍由 cx_read 刀与新根一起提升。旧工具根及 sidecar 保存在 out，未再重建 model-com。

第3刀经典参考专项实测：environ bad=0，system/popen/execvp 子环境 bad=0；用最终根作工具载体后 ABI writer 523项成功。测得 gen/parse2 键为9d231357170a6825/74bdabd1919010b1，同步常量。PP 重采与提交门禁尚未启动，等待 ROOT 参数纪律口径，未计第3刀落地。

政委新指示解堵（2026-10-23）：HANDOFF §3 路径禁项针对活检出；scratch 导出根作为 ident/seal/--check-core 的 ROOT 是规定做法，允许，保留接口不做无ROOT改造。恢复 F1→F6，Draft/Latest/签名/公开仍禁。另授权本次推送当前 main 的 ahead；实际 git push origin main:main 返回 Everything up-to-date，main与origin/main均d59cecd99，后续新提交不据此自动推送。

第三刀首轮 PP 四步虽均 rc0，但 HANDOFF 口径补录与末步检查重叠；按套件运行不改树纪律，不计此轮最终验收。文档与源码固定后，以 cut3-pp2 新目录完整重采；不覆盖或删除原始记录。

第3刀固定树 PP 重采 cut3-pp2 四步全 rc0，最大31.85秒，封件目录摘要746fabe6daa656c16608ddf8c1e155427e81d1e1936023446acf5aa7172295c7；采用这轮进入逐刀提交门禁。此前经典参考 environ/子环境专项均 bad=0，ABI 523项及两实现键同步已完成。F1 仍待其余四补丁与最终整体落地。

逐刀工具缓存按独立 CXBIN_CACHE 目录运行：scratch 工具根提前采用最终根，后续隐式头仍会变化，因此不跨刀复用旧前奏编译的工具；缓存保留，不修改 cxbin 或门禁。第4刀只新增长值/多名字回归，不重生成未变化的头、facts、ABI 或封件。

第4刀长值/多名字探针在本刀经典参考与模型工具载体上均 rc0、输出逐字节相等（bad=0）；现只提交该回归与实测说明，未改产品实现。完整终点线仍以最终新根的 F3/F4 为准。

恢复后的范围收束：政委当前目标只有 F1→F6，W2-D1 等长期预检保留原始记录，不再占本轮调度。第4刀已提交1fb8967c并bundle备份；接续第5刀fileno，仍使用独立工具缓存与scratch隔离。

第5刀 fileno：经典参考与模型探针均 rc0、fileno bad=0，输出相等；头/facts重生成、ABI writer 523项成功。测得 gen/parse2 键0312a3f7528020ac/5ee7ddf710199918，同步两个实现，随后固定树重采 PP，不复用第3刀封件。

第5刀 PP 正式重采四步全 rc0，最大31.54秒，封件目录摘要7062d665e66d0ddaccf069935eb2f7460e8014e9b1a65b06dd95986fc7b717fb；保留旧目录及README，现进入逐刀commitgate。

第6刀采用原0007 enum E; 表修复并补 tests/c/b_enum_bare.c；除裸/重复裸标签外，同时直接验证原 R11 enum_forward.c 的 typedef、函数指针、局部枚举、位域结果。不修改 Tape/枚举域的拒绝边界；PP输入不变，保留第5刀有效封件。

第6刀实际验证：b_enum_bare 在模型与经典参考均rc0；原 enum_forward.c 在模型、经典参考、宿主cc三路rc0且输出完全相同（7 7 7 7 4 / 3 7 4 / 8 7 -2 4）。ABI 523项成功；gen/parse2 键7fb1eaf41cfcdf89/289b42a4ad9b85db，同步后进入寄存器、单元解析与派生门禁。

第6刀组合闸首轮外层142，未提交。保留日志定位到 exec-unitparse 冷准备：unitparse-ref 已按fresh协议构建并有stamp，parse2 JSON仍在生成，其他步骤未结证。按原脚本的fresh命令在同一index导出逐步限时准备JSON与dump，再重跑全部命名闸；不抬看门狗、不伪造stamp、不裁掉该套件。

第6刀按原fresh协议分步准备已成功：parse2 10209 states、unitparse-ref及token dump均有正常产物与stamp，每步独立≤58秒；保留首轮外层142证据。现在在同一index导出重跑原全部命名套件，没有修改套件、输入或缓存协议。

F1 最后刀：应用原0006后，以已实测的精确补修替换 cx_read POSIX 路径，直接检查新开描述符 read 返回值，覆盖8路stdio状态占满边界；Windows原路径保留。同步已验证的K0容量回归，重生成派生并以最终候选源指纹/字节身份核对，再整体提升 main 与根产品，不再混入其他轨道。

最后刀重生成已成功：gen/parse2键为最终1aba99c3d52243aa/289b42a4ad9b85db，源指纹c4618639cc3fcfac4f8ce52a11df3bdab93b2c9376eecee63fd70e6550adc57f与已验证候选完全相同，ident确认4f0b493c7a83d73a4ae1b348c8400ea3b7eda0ea190051b54a84c712a6730eda为fresh，K0 bad=0 files=18。同步精确同输入的最终双封及657份差分证据，接续封件负例与提交闸，不重新训练或改动其他轨道。

F1 最终材料转移后检查已通过：E3 gate含负例、PP core与负例均rc0；原657份证据独立重算再次通过（628 SAME/29 DIFF，双产品ident与全部输入pin通过），落地树产品输入与该证据源指纹完全相同。进入最后刀index提交闸；main与根产品整体提升成功之前仍不计F1正式完成。

最后刀首次提交被输入声明分类器正确拒绝：已有 e3-seal gate 未登记到 gatedeps。补齐 refreshgatedeps.cx 中该已有套件的命令与依赖（334条实测依赖均落在7个产品源树，另声明封件树、检查器、bound及根产品对和cc/sh工具），通过landlock --derive生成仅本项及本刀快照的JSON变化；不移除E3检查、不改seal判据。

### 版本状态与可核验进度

| 项 | 当前口径 | 核验来源/限制 |
|---|---|---|
| 开发版本 | **0.0.43** | [src/version.h](src/version.h) 的 UNISACC_VERSION |
| Latest | **v0.0.42** | 2026-10-21 GitHub `/releases/latest` API 只读核对；[Release](https://github.com/partnernetsoftware/unisacc/releases/latest) |
| 本轮闸 | **ABSENT** | 本次协调裁定；未附本轮有效闸收据，不能引用旧绿顶替 |
| 发布链 | **约1/3，仅源码已推** | 三格口径见[可核验进度](#flow-net-progress)；非产品完成率 |

### 当前有效计划（唯一索引）

| 计划 | 当前有效范围 | 不代表 |
|---|---|---|
| [v0.0.43执行计划](plans/v0.0.43.md) | 前级/库边界、晚绑定类别复用、Tape验域；具名范围与验收条件 | 已授事项不等于已实现，旧候选绿不等于本轮闸 |
| [路线与退出条件（§5）](#5-路线) | 0.0.x自举工具链、有限库面、后续研究 | 旧版本时间表已实现，或自动开工 |

0.0.37–0.0.42旧计划、旧路线图、门禁重叠清单和构想原稿按原字节归档（旧文内路径按原位置解读），统一见[archive/plans/](archive/plans/)；历史0.0.43基线亦在其中。已发布不等于旧计划每项完成，遗留义务须在本版具名核对，不默删。发布及逐日过程见§7。

**产品命名（主人 2026-09-30 提醒，硬规则）**：两代产品只有两个名字——宿主构建的第一代叫 **`unisacc-seed.com`**，由它自举出来的最终产品叫 **`unisacc.com`**（发布物、GHCR 候选、README 与 N22 三阶段的文件名都按此；`unisacc-next.com` 只是构建目录里的中间名，不出仓）。


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

| 发布链三格（2026-10-21协调口径） | 状态 | 证据边界 |
|---|---|---|
| ① 文档/源码tip已推 | 已有 | Git远端分支可核；不等于产品发布 |
| ② 本轮候选有效闸与收口收据 | **ABSENT** | 不用旧门绿、旧包或封存数量顶替 |
| ③ 发行资产及正式发布 | 本轮未完成 | Latest仍v0.0.42；本轮0.0.43未因源码push而发布 |

**根对现状（2026-10-22）**：开发已迁到新 Linux x86_64 宿主（16 核，内存充足，旧机 2.3G 内存压力导致的 seed-gen 卡死不再出现）。根部 `unisacc.com` 为工作树候选（本机由 `make model-com` 各步构建；宿主须有 clang/ld64.lld/llvm-nm 与 zlib 头，exec/c/BUILDING.md 未列，待补），每次表刀后按 `release/derived-map.tsv` + `release/tools/derivedcheck.py` 刷派生物、经 `release/tools/commitgate.sh` 提交。旧机第一至第七次根对重建/归位收据与宿主坑位已归档：[2026-10-22 过程归档](archive/prd-process-notes-20261022.md)。

“约1/3”仅指上述三格已有第一格，不是模型覆盖率、时间完成率或发布授权。当前版本声明见[src/version.h](src/version.h)，Latest于2026-10-21通过GitHub releases/latest API只读核对。

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
- 发布流程权威入口：[release/RELEASE-PIPELINE.md](release/RELEASE-PIPELINE.md)、[release/README.md](release/README.md)及工作流。流程文档不等于执行授权；本轮闸仍ABSENT。

## 5. 路线

### 5.0 当前优先与0.0.x退出条件

当前先收口前级/库边界与晚绑定阶段合同，具体刀序和验收只在[0.0.43计划](plans/v0.0.43.md)维护。旧0.0.29–0.0.42时间表已归档，不因版本号推进而宣称义务完成。

进入0.1.x前，自有构造、构建、自举、测试、队列、缓存、报告及发布工具链迁到C/.cx；在没有Python、外部cc和shell的环境中，由上一版unisacc.com构建下一版并达到自举不动点、完成本地适用测试。外部对照编译器仅作可关闭的预言机。不得改后缀、藏脚本或由.cx调用Python/shell冒充完成；按依赖族迁移，保留原检查、失败语义、故障注入及新旧对拍。归档不等于删除未端口义务，archive/ujs等范围逐项清点，不默认豁免。

种子仍按两个代际命名：unisacc-seed.com与unisacc.com；C种子构造器和声明规格的完整性必须有同源实证，Python保留为对照不冒称已退役。

### 5.1 libc与前级冻结

优先复用系统libc，按函数族明确“转发/保留/拒绝”，不无限扩头内库体；方向是独立provider/外置运行库与声明头分离。单函数边界刀只换实现时P/M必须不变，头资源输入变化与模型语义变化分账，禁止无pp表/模型实证随库刀reseal。

通用转发/目标装载与ABI资格分别核验；声明有名不等于签名可调用。宿主地址只在绑定层进入。静态/动态不是硬性二选一：纯计算且影响确定性的保留项须具名，其他库实现可独立静态链接；不把静态链接等同于头内实现。允许按需借用合规实现并保留版权/NOTICE，禁止整包内置替代系统libc或无界自研。来源与细账见[libc移交](research/libc-forward-handoff.md)、[历史过程归档](archive/prd-process-notes-20261021.md)。

### 5.2 未排期研究与构想（不自动开工）

- 权重表示及推理成本：优先研究同权阈值段精确聚合、类型化动作模板/银行归一化；分别量权重、运行内存与同身份延迟。包压缩不冒称权重减少，结构统计不冒称真实提速；网络语义及完整接口保持是前提。
- 可移植Tape包、嵌入式库/API与包管理、wasm第七目标；表解释性/语义完备性与证明；向量化、工具链去脚本及队列改进。已有实施部分归当前计划，不重复排队。
- 原FX-1–FX-8细稿及旧测量完整留在[构想归档](archive/plans/ideas.md)，旧收官排期在[路线图归档](archive/plans/roadmap-0.0.x.md)。构想不构成产品/发布授权。

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
