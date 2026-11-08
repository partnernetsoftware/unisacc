# Rule-source migration and completion boundary

Source inventory refreshed after `11a0d56` (2026-09-28). This is a remaining-work
list, not a completion percentage. The inventory describes offline construction sources; it does not declare
product adoption incomplete. The authoritative [pipeline design](../prd.md#pipeline-design)
and [S-17 migration archive](../archive/s17-migration-log-20260928.md) separate
current behavior from historical acceptance evidence.

The route is **declarations → finite transitions/actions → constructed threshold
networks → generic execution over stage byte streams**. Moving a rule to TSV
removes its Python control implementation; it does not erase the rule or imply
smaller weights. Shared action sequences and dynamic gold/catalog values must
remain shared inputs, not copied answers.

E4 元记录零宽约定：`opt/scans-*` 的 `META` 只识别严格输入校验已认可的
`.unit ` 记录（含两字段版本头）及精确 `.end` 行，不把其它点前缀标签视为元记录。
`NEXTSEM` 仅移动输入游标，不输出、不改变定位上下文；所有投机窗口共用它。
O2 的 `LSS/LEE` 与活跃性数组仅索引语义行，因此原来的 `pi+1`、窗口上限和
寄存器扫描仍按去元记录后的旧流解释。`METACOPY` 仅在确认提交后按输入顺序
输出被合并区间的元记录；`SEMCOPY` 输出同区间的非元记录，避免重复。
替换输出前保留该窗口内最后的逻辑文件上下文；窗口结束后的元记录仍留给后续行。
每个内部 round 透传末尾 `.end`，不把它当作屏障、标签或符号引用，也不另造终止行。
独立正负材料见 `tests/opt-frame-transparency/cases.tsv`；材料为手写，不从新实现取 oracle。
既有 `tests/check-netregen-bytes.cx` 的
`opt-frames ROOT RUN NET TABLE ABS_OUT CASE` 模式按 cases.tsv 的一行分片运行；
CASE 为 `positive`、`negative-label`、`negative-end` 或 `negative-eof`。
NET 与 TABLE 分别作为 MODEL 以 `RUN MODEL INPUT SRCPATH` 调用，绝不把表当作输入；
两路均检查严格投影、原序元记录、末尾唯一 end，并比较实际输出字节。
负例只接受 rc=1、空 stdout 及精确 `reject: tape truncated` 诊断，不接受 usage rc=2。
同一检查器的 `prepare|case|summary ROOT OLD_PRODUCT NEW_PRODUCT ABS_OUT [CASE TARGET]`
可统一追加 `--new-models ABS_PKG`；该包的规范绝对路径及 SHA256 进入 material 身份，
后续 case/summary 必须传相同参数且包内容不变。仅 NEW_PRODUCT 显式收到 `--models PKG`，
OLD_PRODUCT 继续用 embedded 旧包。产品前四字节为 ELF magic 时直接 exec，否则保留 APE 的
`sh PRODUCT` 路径；实际 argv 原样存证。每产品仍限 25 秒，五案例乘六目标共 30 片，
只比较跨产品输出图像，不能把该比较当成目标平台原生执行资格。
参考旧 0.0.48 产品的独立片为
`reference ROOT A B OUT CASE TARGET --reference-product ABS_LEGACY [--new-models SAME_B_PKG]`。
它要求同版本检查器 prepare 的正式 material 及既有 A/B 片，复用同一固定源码绝对路径、
目标和编译参数，只执行第三 legacy 通道一次（25 秒），不重编 A/B；没有历史 harness 例外。
每片记录 legacy 产品 path/SHA、argv/stdout/stderr/status/image 和独立绑定/观察；真实拒绝
保留为 REJECT，图像不存在时明确记 ABSENT，不填伪造 SHA。传输失败不作语义拒绝。
`reference-summary ROOT A B OUT --reference-product ABS_LEGACY [--new-models SAME_B_PKG]`
只读真实 30 片，输出每案例/六目标的 legacy/A/B SHA 及 legacy 对 A 的 SAME/DIFF/REJECT。
同源 B 与 A 的逐字节相同始终是强断言；旧 0.0.48 仅作参考，不能迁移候选资格。

| Stage | Declaration sources already used | Remaining handwritten construction |
|---|---|---|
| E1 | `lex/` declarations | Data assembly and bindings; runtime model is separate from the classic reference lexer |
| E2 | [pp/rules.md](pp/rules.md) | Header-name extraction, resource ordering, initialization and assembly bindings |
| E3 | `parse2/tape-*`, `scope-*`, `declaration-*`, `width-*`, `type-tape.tsv`, `control-*`, `ladder-*`, `initializers-*`, `strings-*`, `constexpr-*`, `statics-*`, `vla-*`, `unresolved-*`, `printfallback-*`, `type-entry/default/follow`, `operator-*`, `shape-*`, `conversion-*`; gold type/tyinfo/prec and other existing facts | Dynamic type/gold/template bindings and initialization; fixed expression, declaration, literal, diagnostic, warning, unit and location controls now have declarations |
| E4 | `opt/scans-*`, `local-*`, `stfuse-*`, `peep-*`, `analysis-*`, `parsers-*`, `rounds-*`; gold opinfo/peep | LEVEL selection, dynamic data initialization/dispatch and generic assembly in `opt/gen.py` |
| E5 lowering | `lower/data-*` sparse layout, `lower/code-scan-*`, `lower/code-print-*`, `lower/code-entry-*`, `lower/code-sysprep-*`, `code-abi-sources.tsv`, `code-syscall-*`, `armfuse-*`; existing gold/catalog facts | Dynamic ABI fact selection and template bindings in `lower/code.py` (fixed syscall/termination control now uses `code-syscall-*` and `code-shell-*`); target/escape/layout bindings in `data.py`; ARM destination-shape policy and shared optional u64 reader are declared |
| E5 encoding | `enc/armint-*`, `armmem-*`, `armbranch-*`, `armfp-*`, `arminput-*`, `armlayout-*`, `armwin-*`, `armbase-*`, `armcontract-*`, `x86-operand-*`, `x86-line-*`, `x86-procs-*` (including relaxation); catalog opcode facts | Dynamic instruction/shape and format-field bindings; machine-code templates including Windows command-line setup remain shared sources; see [enc/CONTROL_AUDIT.md](../archive/reference-c/exec/enc/CONTROL_AUDIT.md) |
| E6 images | `enc/elfimage-byte/result.tsv` shared image control and `enc/pedelta-byte/result.tsv` PE control, `enc/machodelta-*` Mach-O layout/signature-page control, `enc/sha256-*` shared digest control; existing target layout facts | Format header/section/CD fields and import/string enumeration, child installation and dynamic layout/hash-constant bindings in `enc/` |

`finite_rules.py` expands declared observations, bindings and action sequences;
it must not acquire compiler-specific predicates. The retired grammar in
`parse/gen.py` was deleted. Its fixed numeric/character token reader, numeric
formatting/conversion and automatic-header scanner now use `parse/tokenread-*`,
`numeric-*` and `autoscan-*` declarations. PRN and PRNW instantiate one rule with
different widths. The module retains the generic assembler, dynamic word-trie
assembly and vocabulary/qualifier selection. `autonames()` still extracts header
resources. `optext()` binds binsel/irsel results to shared tape templates.
Any remaining compiler-specific control must stay visible in this inventory;
calling the file “shared support” does not remove that obligation. Keep a single implementation of each shared
routine when migrating it; E3, the unit reader, E4 and lowering use this module.
`src/` and `unisa/` remain behavior references after the default switch;
the explicit classic fallback is not an automatic retry for model refusals.

## Current validation boundary

Each generated δ is checked against its table over the finite observation domain; the packaged product is then tested on source, tape, image, diagnostics and native execution. The current adopted route, coverage limits and release identity are tracked in [prd.md](../prd.md), [the package contract](c/PACKAGE.md) and the release receipts. Old candidate identities and fixed-control milestones are preserved in [the rule history](../archive/exec/rules-history-20261002.md).

## 残余直接改图与理由（0.0.23 K）

**口径（2026-10-03 扩大）**：tests/decisionledger.py 现在统计 exec/*.py（finite_rules.py 除外）里所有直接改图的写法：`.on(` `.branch(` `.goto(`、lex 的 `Delta.put(`，以及对图对象的 `st`/`els`/`r`/`seqs`/`states`/`rows` 的写入（赋值、删除、pop/append/update 等）——改名、挂钩、别名这类 Python 图编辑都算在内，不能换个写法绕过。基线见 research/decision-ledger.json，只降不升。

**理由**：每个仍被计数的文件都必须在 tests/decisionledger.allow 里有一行 `keep FILE CATEGORY reason`，门禁检查。类别：test-tool（检查脚本内部的探针图）、generic-assembly（通用组装：汇编器 P 的原语等，所有表都经它展开）、pending-migration（0.0.23 K 的欠账，用参数化模板迁移，不是被接受的残余）、cdx-pending。主人规则：“表行目标列不能由事实生成”不算理由，参数化模板（finite_rules.install_template）能表达事实生成的控制；保留必须说明为什么连模板也表达不了。

## 构建期 import 的 unisa/ 模块：事实来源与可信基

exec/ 的构造器在构建时 import 以下 unisa/ 模块（19 个）：`unisa/__main__`、`unisa/assemble`、`unisa/catalog`、`unisa/construct`、`unisa/driver`、`unisa/emit_arm`、`unisa/emit_x`、`unisa/front`、`unisa/front.pp`、`unisa/gold`、`unisa/hostabi`、`unisa/image`、`unisa/intnet`、`unisa/libneed`、`unisa/lower`、`unisa/prune`、`unisa/tape`、`unisa/tsvgold`、`unisa/uns`。它们不在决策代码账本的范围内，因为它们不直接改图，作用是：①**事实来源**：gold 真值表、目录（catalog）、编码规格、类型与目标事实等，通过绑定和事实列表进入声明表；②**参考实现的一部分**：经典 Python 路线，作为逐字节对拍的裁判之一；③**可信基**：它们给出的事实若有错，网络与表会“一致地错”，全域枚举发现不了——这一层由外部裁判、参考/产品/cc 三方对拍（declmatrix、csmithdiff）提供证据。论文 A（J 项）应把它们列入可信基，与 Python 构造器、枚举器并列。

## 旧口径下的残余清单（历史，2026-10-03，仅统计 .on/.branch/.goto；现行口径与理由见上一节和 tests/decisionledger.allow）

迁移方法：记录原 install 在真实生成器进程中发出的每条转移和新标签分配，机械写成声明表（STEM-result/byte/fresh.tsv），动态事实以绑定、序列、类传入；每片都证明迁移前后各生成器的转移图逐字节相同。下表是剩下的直接转移调用点：

| 调用点 | 文件 | 理由 |
|---|---|---|
| 11 | `exec/parse/gen.py` | 未迁：经典 parse 阶段的 token 读取/格式化控制，待下一片 |
| 10 | `exec/enc/hostbridge.py` | 依赖事实的改写：把已安装状态改名为 *.original.* 并挂接钩子，或按事实列表（整数类型、平台前缀、recipes）生成状态；表行的目标列不能由事实生成，所以留在 Python |
| 9 | `exec/parse2/membercontrol.py` | 依赖事实的改写：把已安装状态改名为 *.original.* 并挂接钩子，或按事实列表（整数类型、平台前缀、recipes）生成状态；表行的目标列不能由事实生成，所以留在 Python |
| 8 | `exec/parse2/diagnosticcheck.py` | 测试工具：在检查脚本内自建探针图，不进入产品构造 |
| 7 | `exec/parse2/valueranks.py` | 依赖事实的改写：把已安装状态改名为 *.original.* 并挂接钩子，或按事实列表（整数类型、平台前缀、recipes）生成状态；表行的目标列不能由事实生成，所以留在 Python |
| 7 | `exec/parse2/callcontrol.py` | cdx 暂存区中，待迁 |
| 7 | `exec/nativeabi/gen.py` | 依赖事实的改写：把已安装状态改名为 *.original.* 并挂接钩子，或按事实列表（整数类型、平台前缀、recipes）生成状态；表行的目标列不能由事实生成，所以留在 Python |
| 6 | `exec/parse2/errors.py` | 依赖事实的改写：把已安装状态改名为 *.original.* 并挂接钩子，或按事实列表（整数类型、平台前缀、recipes）生成状态；表行的目标列不能由事实生成，所以留在 Python |
| 6 | `exec/modelbindings.py` | 依赖事实的改写：把已安装状态改名为 *.original.* 并挂接钩子，或按事实列表（整数类型、平台前缀、recipes）生成状态；表行的目标列不能由事实生成，所以留在 Python |
| 5 | `exec/parse2/librarydata.py` | 依赖事实的改写：把已安装状态改名为 *.original.* 并挂接钩子，或按事实列表（整数类型、平台前缀、recipes）生成状态；表行的目标列不能由事实生成，所以留在 Python |
| 3 | `exec/pp/gen-manifest.tsv` (was `exec/pp/gen.py`) | 依赖事实的改写：把已安装状态改名为 *.original.* 并挂接钩子，或按事实列表（整数类型、平台前缀、recipes）生成状态；表行的目标列不能由事实生成，所以留在 Python |
| 3 | `exec/parse2/structreturnexpr.py` | 依赖事实的改写：把已安装状态改名为 *.original.* 并挂接钩子，或按事实列表（整数类型、平台前缀、recipes）生成状态；表行的目标列不能由事实生成，所以留在 Python |
| 3 | `exec/parse2/libraryexports.py` | 依赖事实的改写：把已安装状态改名为 *.original.* 并挂接钩子，或按事实列表（整数类型、平台前缀、recipes）生成状态；表行的目标列不能由事实生成，所以留在 Python |
| 3 | `exec/lower/code.py` | 依赖事实的改写：把已安装状态改名为 *.original.* 并挂接钩子，或按事实列表（整数类型、平台前缀、recipes）生成状态；表行的目标列不能由事实生成，所以留在 Python |
| 3 | `exec/enc/elfimage.py` | 依赖事实的改写：把已安装状态改名为 *.original.* 并挂接钩子，或按事实列表（整数类型、平台前缀、recipes）生成状态；表行的目标列不能由事实生成，所以留在 Python |
| 2 | `exec/parse2/printfcontrol.py` | 依赖事实的改写：把已安装状态改名为 *.original.* 并挂接钩子，或按事实列表（整数类型、平台前缀、recipes）生成状态；表行的目标列不能由事实生成，所以留在 Python |
| 2 | `exec/parse2/libraryvariadic.py` | 依赖事实的改写：把已安装状态改名为 *.original.* 并挂接钩子，或按事实列表（整数类型、平台前缀、recipes）生成状态；表行的目标列不能由事实生成，所以留在 Python |
| 2 | `exec/parse2/librarytypes.py` | 依赖事实的改写：把已安装状态改名为 *.original.* 并挂接钩子，或按事实列表（整数类型、平台前缀、recipes）生成状态；表行的目标列不能由事实生成，所以留在 Python |
| 2 | `exec/parse2/librarymodule-manifest.tsv` (was `exec/parse2/librarymodule.py`) | 依赖事实的改写：把已安装状态改名为 *.original.* 并挂接钩子，或按事实列表（整数类型、平台前缀、recipes）生成状态；表行的目标列不能由事实生成，所以留在 Python |
| 2 | `exec/parse2/libraryimports.py` | 依赖事实的改写：把已安装状态改名为 *.original.* 并挂接钩子，或按事实列表（整数类型、平台前缀、recipes）生成状态；表行的目标列不能由事实生成，所以留在 Python |
| 2 | `exec/parse2/constexpr.py` | 依赖事实的改写：把已安装状态改名为 *.original.* 并挂接钩子，或按事实列表（整数类型、平台前缀、recipes）生成状态；表行的目标列不能由事实生成，所以留在 Python |
| 2 | `exec/nativeabi/ordered.py` | 依赖事实的改写：把已安装状态改名为 *.original.* 并挂接钩子，或按事实列表（整数类型、平台前缀、recipes）生成状态；表行的目标列不能由事实生成，所以留在 Python |
| 0 | `exec/layoutprovenance.py` | 已删除（K2 round 55）：plain/located 帧与 gates 由 exec/parse2/units-manifest.tsv 的 rows/template 行装配 |
| 1 | `exec/parse2/unitmode.py` | 依赖事实的改写：把已安装状态改名为 *.original.* 并挂接钩子，或按事实列表（整数类型、平台前缀、recipes）生成状态；表行的目标列不能由事实生成，所以留在 Python |
| 1 | `exec/parse2/printfallback.py` | 依赖事实的改写：把已安装状态改名为 *.original.* 并挂接钩子，或按事实列表（整数类型、平台前缀、recipes）生成状态；表行的目标列不能由事实生成，所以留在 Python |
| 1 | `exec/parse2/librarytypesv3.py` | 依赖事实的改写：把已安装状态改名为 *.original.* 并挂接钩子，或按事实列表（整数类型、平台前缀、recipes）生成状态；表行的目标列不能由事实生成，所以留在 Python |
| 1 | `exec/parse2/librarycallables.py` | 依赖事实的改写：把已安装状态改名为 *.original.* 并挂接钩子，或按事实列表（整数类型、平台前缀、recipes）生成状态；表行的目标列不能由事实生成，所以留在 Python |
| 1 | `exec/parse2/formatwarnings.py` | 依赖事实的改写：把已安装状态改名为 *.original.* 并挂接钩子，或按事实列表（整数类型、平台前缀、recipes）生成状态；表行的目标列不能由事实生成，所以留在 Python |
| 1 | `exec/parse2/floatconst.py` | 依赖事实的改写：把已安装状态改名为 *.original.* 并挂接钩子，或按事实列表（整数类型、平台前缀、recipes）生成状态；表行的目标列不能由事实生成，所以留在 Python |
| 1 | `exec/opt/gen.py` | 依赖事实的改写：把已安装状态改名为 *.original.* 并挂接钩子，或按事实列表（整数类型、平台前缀、recipes）生成状态；表行的目标列不能由事实生成，所以留在 Python |
| 1 | `exec/modelsignature.py` | 依赖事实的改写：把已安装状态改名为 *.original.* 并挂接钩子，或按事实列表（整数类型、平台前缀、recipes）生成状态；表行的目标列不能由事实生成，所以留在 Python |
| 1 | `exec/enc/shacheck.py` | 测试工具：检查脚本内的探针图 |
| 1 | `exec/enc/machodelta.py` | 依赖事实的改写：把已安装状态改名为 *.original.* 并挂接钩子，或按事实列表（整数类型、平台前缀、recipes）生成状态；表行的目标列不能由事实生成，所以留在 Python |

enc 入口（x86_64 与 ARM64）已是清单：`exec/build/gen.py enc|enc/arm OUT [--elf|--macho|--pe|--object]`，分别运行 exec/enc/gen-manifest.tsv 与 exec/enc/arm-manifest.tsv。
