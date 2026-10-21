# 两条「参考宽／产品严」分账（ledger 行，待裁；本文不代裁、不动参考）

2026-10-22 02:5x AWST。来源：com-extent-neg（e79ea3ab）与 com-bindings-neg（41fb49bb）两叶采证中发现，现状已被两叶按登记纪律写死（参考改口即叶红），本文供 ledger 线裁定走向。

## 1. extent ≥ 2^31（族 2）

**实测**（target lnx/x86_64，全部可复现）：
- 产品：单项 `.bss a 2147483648` 与累计 `2000000000+1000000000` 均 rc1、stdout 空、不写输出文件、具名 `not covered: tape data directive`。
- 模型两执行器（`exec/lower/sparsecheck.py` 在册断言）：同拒，诊断含 `tape data directive`。
- 参考（`python3 -m unisa compile --from-tape`）：**接受** rc0，镜像结构自洽——RW 段 `filesz 0 / memsz 2147483816`（累计件 `3000000168`），与 6.1 亿控制件同构；控制件产品/参考逐字节同（4097 B）。

**定性**：参考能真实表达 >2^31 虚区，产品/模型的 2^31 是**决策表上界**；按「不能精确匹配的必须拒绝并报未覆盖」，`not covered` 是诚实答案，不是缺陷。与 underscore（参考语法漂移）**不同类**。

**选项**：
- (B·现状) 保持登记式分账：产品 not covered，参考接受由叶写死。零改动。
- (C) 扩模型决策域到 64 位 extent 上界（与 Tape 整数 64 位字裁定同口径）：动 gold/表，属构造路径，需另授与穷举证据。
- (A) 收紧参考到 2^31：**不建议**——参考能力真实，收紧只是掩盖表的上界，还会使现有大额合法区间失去对照。

**建议：(B) 维持，(C) 列为可另授的扩表刀；不采 (A)。**

**追加（2026-10-22 03:4x，载体路由归属分叉）**：同一 extent 程序在两条路由上拒绝**归属不同**——文本路 `.tape` 归 lower 表（`not covered: tape data directive`），Tapebin 载体路在 ≥2^31 时被产品**解码器**先拒（驱动层 `invalid tapebin or origin target mismatch`），而参考 codec（LEB128 无界）与参考编译都接受该载体。可表示边界 2^31−1 两路同归 `tape data directive`（归属平价成立）。对照 hostaddr/bindings 两类 NEG：载体路归属、具名、行号与文本路完全一致——分叉只在 extent 的解码器界限上。prd 拒绝归属条（语言级拒绝归声明终态的阶段，不得改记）之下，裁向有二：(i) 产品解码器放行 LEB128 全域、让 lower 表以同一具名拒绝（归属两路合一，动 src/ 须另授）；(ii) 登记解码器 32 位界为载体格式承诺（参考 codec 向产品收紧，动 unisa/ 须另授）。现状已由 com-extent-neg 的 `neg-single-bin`/`neg-max-bin`/`neg-max-text` 三断言写死，com-hostaddr-neg 的 `neg-high-bin` 写死了归属平价的对照面。

## 2. 缺 bindings 的 `.librarycall`（族 4）

**实测**：
- 产品：`f:\n.librarycall r1, r0\n…` 无任何 bindings 输入时 rc1、stdout 空、不写输出文件、具名 `not covered: library import binding or signature`（绑定层）。
- 模型（`exec/lower/libraryimportscheck.py` bundle 路由）：缺/坏 `\0library/bindings` 一律 reject（在册断言）。
- 参考：**接受** rc0，无 bindings 通道照样下放出 4096 B 镜像（text 203 B、RW memsz 168；`.librarycall` 被下放成了什么语义本文未验，仅记录其存在）。

**定性**：spec LB 系列要求库需求走显式合同/bindings 通道；参考在通道完全缺席时静默产像，疑属**参考漂移**（与 underscore 同类：参考比合同宽）。

**选项**：
- (A) 照 underscore 先例把参考向合同对齐：无 bindings 输入时具名拒绝（动 `unisa/`，需另授；落地时 com-bindings-neg 的 `neg-no-bindings-ref` 断言同步由「登记接受」改「具名拒绝」）。
- (B·现状) 保持登记式分账。
- (C) 放宽产品——与模型在册拒绝冲突，不可取。

**建议：(A)，另授后实施；未授期间 (B) 由叶守住。**

## 2c. CRLF 行尾（第四条「参考宽／产品严」，2026-10-22 05:4x）

**实测**：`_start:\r\n  ret\r\n` 这类 CRLF 行尾的 tape——产品 tape 验域**接受**（`-S` 原样回显含 `\r`），但镜像路（`-b`/`-run`）拒绝 `not covered: an op outside the first encoder slice`（`ret\r` 作为 op 记号不可分类）；参考全路接受并建像。产品内部还有域/镜像两层口径不一。发射器从不写 `\r`。对照组：无尾换行、5000 字符符号名、UTF-8 BOM（双方都当普通标签字节）三者双实现接受且镜像逐字节同。

**选项**：(A) 域收紧——tapecheck 对 `\r` 给具名拒绝（动模型+参考同步，域/镜像口径合一）；(B) 双方剥 `\r`（CRLF 透明，动 src/+unisa/）；(C·现状) 登记分叉。**建议 (A)**：发射器不产 `\r`，域内合法性没有 CRLF 的根据，且能顺带消掉产品内两层不一致。现状由 com-tape-struct 的 `crlf-domain/identity/build/ref-accepts` 四断言写死。

## 2b. 另一类观察：双实现同宽于 C99（登记，不入前两类）

2026-10-22 04:4x 采证 com-cerr-hygiene 时发现：`int *p = 1.5;`（double 初始化指针，C99 约束违例，gcc error）被**产品与 Python 参考双双接受**（两边 rc0）。产品与参考互相一致（对拍纪律无违例），但共同宽于 C99；`int *p = 3;` 的整数→指针则是常见扩展（gcc 仅警告），不算。该类不属「参考宽/产品严」，单列为 conformance 面待裁：是否照 R13 缺陷账或 difftest 负域另立登记、以及双实现同步收紧的授权归属。本文不代裁，未入任何叶（叶里语义负例改用 unknown identifier 规避未裁面）。

## 3. 共同事项

- 两叶已把现状写死：裁定任一项落地时，对应叶的 ref 断言与注释须同一提交内改，不允许叶与裁定口径分叉。
- 本文不改产品、不改参考、不改模型表；win 口径（TP-7/8 待证）不在本账内。
