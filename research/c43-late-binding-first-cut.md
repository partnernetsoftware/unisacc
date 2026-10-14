# 0.0.43 晚绑定第一刀：lower 符号流 + 一条绑定负例（只读设计稿）

性质：设计稿。**全部证据为“读源码”**，未执行任何套件或候选；不改产品、不改 gatedeps、不碰 exec/tapecheck、exec/c/compiler.c（cdx 第二刀）、tests/tapedomaincheck.cx、plans/。

## 0. 结论先行
- 今天的晚绑定**已经存在一半**：`.hostaddr r, N` 的 N 是宿主槽位下标（0..3），地址在加载时才由宿主填入（`unisa/lower.py:328–329`；`src/host_dl.h:13–29`）。问题不在时机，而在**契约没有显式化**：签名不在 tape 里，符号 ID 只是裸下标。
- 第一刀建议：把 lower 对 `.hostaddr`/`.hostcall` 的输出改成“字节流 + 符号 ID 表 + reloc 记录”，装配/绑定单独成步，只做一条负例。`.extern`/`.global` 留到第二刀（它们在对象路径，不在 lower 的单遍流里）。
- 首刀只做 osx 与 lnx（参考已接受，`lower.py:326`），**不碰 Windows**，避免 L-4 的参考一致性之争。

## 1. 现状：各阶段输入里的外部信息（逐阶段）
| 阶段 / 位置 | 输入里出现的外部信息 | 性质 | 证据（读源码） |
|---|---|---|---|
| E2 预处理 | `cli/include-dir`（宿主路径字符串）、目标 predefines | 宿主态（路径）+ 合同（目标） | `exec/c/libunisacc.c:608–612`；prd §3.3 |
| E1/units/E3 | 无宿主地址；E3 的符号表是阶段内部态 | 合同（语法） | prd §3.4–3.6 |
| 签名 `signatures` | `library/signatures` 资源，`signature_version==3` 时注入 | **合同，但走的是资源而非流** | `libunisacc.c:41`、`:983`（资源 `library/signatures` 在 `c->signatures` 非空时注入） |
| lower | `.hostcall`/`.hostaddr` 作为 tape 操作；`.hostaddr` 的 N∈0..3；目标限 osx/lnx | 合同（操作码 + 槽位下标） | `unisa/lower.py:325–330`；`exec/lower/libraryexitcheck.py:63–68` |
| lower-objectfacts（对象路径） | `.extern`/`.global` 记录，按出现顺序记 linkage | 合同（对象事实），位于对象/合并路径 | `exec/lower/objectfacts-manifest.tsv:4`；`objectfacts-result.tsv:13–14,34`；`exec/c/fwdmerge.c:146–177` |
| 库绑定（内存路线） | `library/bindings` = `c->binding_blob`，由 `us_bindings_add_*` 写入**宿主函数地址** | **宿主解析结果进入模型输入** | `libunisacc.c:423`（`(uintptr_t)address`）、`:587`、`:619`、`:987` |
| 库分发 | `library_native_dispatch` / `library_variadic_dispatch` 的地址在冻结时取得 | 宿主地址进入冻结 | `libunisacc.c:579` |
| -run 转发 | `dlopen/dlsym/...` 槽位（Linux/macOS） | 宿主态，**不在模型里** | `src/host_dl.h:13–29`；`exec/c/compiler.c:440–456` |

**读法**：今天的混入有两处关键：(a) 库绑定资源里直接是宿主函数地址（`libunisacc.c:587`），它随模型输入走；(b) 签名是独立资源，不在 tape 的 `.hostcall` 操作里。前者违反“宿主态不进核心身份”，后者使契约分裂在两处。

## 2. 最小第一刀：`.hostaddr`/`.hostcall` 的符号流
**选择理由**：这条路径已有下标式晚绑定，改动最小；且 osx/lnx 参考已接受，不涉及 L-4 的争议。

**lower 输出（字节流，草案）**
```
UNISYM1 magic            8B
target  len+bytes        (osx/x86_64 | lnx/...)
mode    u8               0 = program, 1 = library(explicit)     ← L-4 的显式模式输入
symtab  count u32, then per entry (sorted by name, 无指针、无地址):
          id u32 | kind u8 (1=slot-fn 2=slot-addr) | slot u8 (0..3) | sig[32] (签名哈希; 首刀可为全零=未声明) | name len+bytes
text    原 target.text（.hostaddr 的 N 改为 id）
reloc   count u32, then per entry: off u32 | id u32 | addend i64
```
- 确定性：symtab 按 name 排序，无宿主地址、无时间戳、无路径（路径只允许在 `target` 之外的诊断里）。
- 首刀 `sig` 全零也合法：它表示“签名未声明，待前置契约检查”；这样不引入新的签名语义，只把它显式化。

**装配/绑定（步骤与输入输出）**
- 输入：UNISYM1 流 + provider 表（名字、kind、slot 或地址、签名哈希）。
- 输出：绑定后的镜像（memory route）或 `-o` 时的 import 记录（交给宿主动态加载器，即 host_dl 所做的事）。
- 位置：**不放进 compiler.c**（cdx 第二刀），新建一个 `exec/c/bind.c` 或 `unisa/bind.py`，由 memory route 调用。`libunisacc.c:587` 的 `binding_blob` 在首刀保留为 provider 的来源，但不再进入 lower 的输入。

## 3. 一条绑定负例：形状与拒绝位置
**形状**：合同声明 `id=1, kind=slot-fn, slot=2`（`dlclose` 槽）；provider 表里缺该槽，或签名哈希与合同不符。

**拒绝位置**：**装配/绑定步**，在写镜像之前，memory route 入口处（`compiler.c` 的 memory 分支只调用 bind，不自检）。
**具名诊断（建议文本）**：`unisacc: bind: symbol 1 (slot 2, sig <hex8>) not provided by host` 或 `... signature differs from contract`。
**期望**：rc1、stdout 为空、不写镜像、不执行程序；与参考绑定器（同一 provider 表）对拍 rc 与诊断前缀。

**为什么不应由 lower 拒绝**：lower 是有限模型，其身份不能包含宿主 provider 的内容（否则每台机器一个候选）。宿主提供哪些槽位是运行时事实，lower 无从知道。lower 能做的只是**合同级**检查（slot 下标是否在 0..3、目标是否允许）——这正是 TP-7 说的“显式目标/外部合同”，保留在 lower（`lower.py:327–329` 已经如此）。

## 4. 与 TP-7 / TP-8 / L-4 的关系
- **TP-7**（`spec.md:825`）：lower 只消费已验流及显式目标/外部合同。UNISYM1 的 `mode`、`target`、symtab 正是“显式外部合同”的数据形态，前置验域（TP-7 的方案 A）可以校验它的格式。
- **TP-8**（`spec.md:827`）：与本刀无直接关系，但 UNISYM1 的解析失败必须走具名拒绝，不能以末端 encoder 拒绝充数。
- **L-4**（`spec.md:829`）：本刀可以把“模式/目标/能力”落到同一份 UNISYM1 头部（`mode`、`target`、symtab 的 kind 即能力）。**但**首刀的参考一致性只在 osx/lnx 成立；Windows 的 `.hostcall/.hostaddr` 仍被 `lower.py:327` 拒绝，所以 L-4 的 Windows 接受**不**由本刀关闭，须另行裁定参考侧。

## 5. 诱人做法与约束冲突
| 诱人做法 | 冲突 | 处理 |
|---|---|---|
| 在 lower 里直接查 provider 并填地址 | 宿主态进核心身份；每台机器一个候选 | 禁止；绑定只在装配步 |
| 把 `library/bindings` 地址直接写进 UNISYM1 的 symtab | 非确定性；与 `libunisacc.c:587` 同一问题 | symtab 只存 id/kind/slot/sig；地址只在 bind 输出 |
| 首刀顺带支持 `.extern` 的占位地址 | 布局需要大小（lua `initcount` 类问题，f265108e）；占位不能替代布局 | 留第二刀；首刀只做 hostaddr/hostcall |
| 用 lower 拒绝“宿主没提供”的符号 | 运行时事实进编译期；与参考对拍不可比 | 负例只在绑定步；lower 只查 slot 合同范围 |
| 直接改 `compiler.c` 加 bind 分支 | 与 cdx 第二刀冲突 | 新建 bind 模块，compiler.c 仅一行调用，由 cdx 合并 |
| 首刀同时做 Windows | 参考拒绝（`lower.py:327`）；L-4 未裁 | 不做；先 osx/lnx |

**与 prune 的关系（风险）**：`prune`（prd §3.7a）的根是入口、main/__init 与取址代码；`hostcall` 引用的槽位不在 tape 内的函数里，不影响闭包，但 UNISYM1 若引入 `.extern` 的未解析引用，必须把它列为根，否则误剪。首刀不引入，记为第二刀前提。

## 6. 验收（首刀，不跑整闸）
1. 合法控制：`.hostaddr r0, 2` 的 UNISYM1 输出，绑定后与现有镜像**逐字节相等**（同一 provider）。
2. 负例：缺槽 / 签名不符各一条，rc1 + 具名诊断 + 无镜像写出。
3. 确定性：同一输入两次生成 UNISYM1 逐字节相同；不同宿主 provider 只改变绑定输出。
4. 参考对拍：参考绑定器与模型绑定器对负例给出相同 rc 与诊断前缀。

**证据说明**：以上全部为读源码得出的设计；未执行。验收用的 provider 与参考绑定器都还不存在，首刀要先写出它们。
