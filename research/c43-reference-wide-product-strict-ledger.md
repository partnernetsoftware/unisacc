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

## 3. 共同事项

- 两叶已把现状写死：裁定任一项落地时，对应叶的 ref 断言与注释须同一提交内改，不允许叶与裁定口径分叉。
- 本文不改产品、不改参考、不改模型表；win 口径（TP-7/8 待证）不在本账内。
