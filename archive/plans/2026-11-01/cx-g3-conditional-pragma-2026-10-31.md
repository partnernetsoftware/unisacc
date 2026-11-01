# CX-G-3 对齐草案：条件内 `#pragma unisa` 一律拒绝（2026-10-31）

状态：**草案，不是生效条款**。`cx/spec.md` 未改；三源未改；不授权实施；不以此文开测。
本页只做一件事：把 `cx-arch-next-slices-2026-10-31.md` §2 倾向的**选项 A** 写成可冻结的措辞，
并列出它与现行 spec 文本的差异、证据与冻结前置次序。

---

## 1. 现行两套说法

| 来源 | 文本 | 判据 |
|---|---|---|
| `cx/spec.md:90`（**现行条款**） | 「`#pragma unisa` 不得写在 `#if` **条件为真**的分支内，否则拒绝 `unisa pragma inside conditional`。」 | 需要求值 `#if` 条件 |
| `cx/spec.md:75`（表行） | 「不得在 `#if` 为真的分支内 \| **CX-G-3**」 | 同上 |
| 三源实现 | `if (*cond > 0) { … "unisa pragma inside conditional" … }` | 只计嵌套深度，**不求真假** |

- `cx/cx0/plan.cx:441`（权威文法区）
- `cx/compiler/cxmod.cx:412`（产品模块）
- `src/cxmod.c:382`（逐字投影）

三源此处语句一致。实现里没有任何宏求值器：`#if` 后的表达式被整体跳过，只做 `+1/-1`。

⇒ 现状是**合同超前于实现**（spec 要求求值，实现不评估）。选项 A 的方向是合同向实现对齐。

## 2. 选项 A 的措辞（建议冻结文本）

> **CX-G-3**：`#pragma unisa` 不得出现在任何预处理条件构造内部，无论该条件是否成立、也无论位于哪个分支；
> 否则拒绝 `unisa pragma inside conditional`。预处理条件构造指 `#if`、`#ifdef`、`#ifndef` 及其
> `#elif` / `#else` 分支，由配对的 `#endif` 结束，允许嵌套。

要点（每条都对应可核对的事实）：

1. **不求真假**：`#if 0` 内的 pragma 与 `#if 1` 内的 pragma 同罪。诊断文本不变，仍是 `unisa pragma inside conditional`。
2. **所有分支同罪**：`#else` / `#elif` 分支不因"另一支为假"而豁免。
   实现侧依据：`cond` 只在 `#if`/`#ifdef`/`#ifndef` 加一、`#endif` 减一
   （`plan.cx:431-432` / `cxmod.cx:413-414` / `src/cxmod.c:376-377`），`#else`、`#elif` **完全不参与**；
   故按深度判据，两支等价。
   另注：`#define` 的名字被记录进 `cxm_mac`（`plan.cx:433-436`），但只用于
   "这个词是不是宏调用"（`cxm_ismac`），**不参与** `#if` 谓词求值。因此 `#ifdef X` 的成立与否对判据无影响。
3. **嵌套无豁免**：进入任一深度即拒；深度只用于配对，不用于放宽。
4. **与 CX-G-2 的次序**：同一 pragma 既在条件内又在首个声明之后时，先报 **CX-G-2**
   （`unisa pragma after first declaration`）。依据：三源的 `seen` 判定位于 `*cond` 判定之前
   （`plan.cx:440` 在 `:441` 之前；`cxmod.cx:411`/`:412`；`src/cxmod.c:381`/`:382`）。该次序是**可观测行为**，
   冻结时一并写明，避免下游按诊断前缀分支的实现被顺序变化打断。
5. **诊断位置**：取该 pragma 所在物理行；`line` 计数在含续行的指令中不增加，见 §5（`OPEN-LEX-LINE`），
   差额 1 属独立未决项，不由本条款吸收。

## 3. 与同一语法区的既有先例一致

`cxm_exportok`（`plan.cx` 内）对处在 `#if` 内的导出项直接拒绝：

> `module <m> exports <name> under #if (conditional interface)`

判据同样是 `cxm_icond[k]`（记录该项起始时 `cond > 0`），也是**深度判据、不求真假**。
即"条件内的模块级事实不被接受"在本语法区已经是既成规则；CX-G-3 的选项 A 只是把同一判据
用在 pragma 行上，不引入新语义族。这条先例是选项 A 最有力的对齐理由。

## 4. 夹具矩阵（冻结后必须逐项有结论）

"现状"列写三源**静态判据**给出的结果；"实测"列只填已有记录，标 `UNMEASURED` 者本轮未跑。

| id | 形状 | 选项 A | 现行 spec 文本（求真假） | 现状（静态） | 实测 |
|---|---|---|---|---|---|
| g3-1 | `#if 1` 内 `use m` | 拒 | 拒（条件为真） | 拒 | UNMEASURED |
| g3-2 | `#if 0` 内 `use m` | 拒 | **放行**（条件为假） | 拒 | **已实测拒绝**（inventory §7.1 `dbl`，拒 `:4`） |
| g3-3 | `#ifdef X`（X 未定义）内 `use m` | 拒 | 放行 | 拒 | UNMEASURED |
| g3-4 | `#ifndef X` 内 `use m` | 拒 | 放行（当 X 已定义） | 拒 | UNMEASURED |
| g3-5 | `#if 0` … `#else` 内 `use m` | 拒 | 拒（该支为真） | 拒（`#else` 不改 `cond`） | UNMEASURED |
| g3-6 | `#if 1` … `#else` 内 `use m` | 拒 | 放行（该支为假） | 拒 | UNMEASURED |
| g3-7 | 两层 `#if` 内 `use m` | 拒 | 视两层为真 | 拒 | UNMEASURED |
| g3-8 | `#elif` 分支内 `use m` | 拒 | 视前面条件真假 | 拒（`#elif` 不改 `cond`） | UNMEASURED |
| g3-9 | `#pragma unisa` 在注释里（被注释掉） | 不适用（不构成 pragma） | 同 | 不拒（`cxm_load` 把注释整体置空白，`#if` 文本同样被抹掉） | 静态确认 |
| g3-10 | 先声明，后在 `#if 1` 内 `use m` | 拒，且诊断是 **CX-G-2** 文本 | 同 | 报 `unisa pragma after first declaration` | UNMEASURED |
| g3-11 | `#if \` 换行 `0` 内 `use m` | 拒 | — | 拒，但报行号比物理行小 1 | 已实测行号差（`OPEN-LEX-LINE`） |
| g3-12 | 条件外的 `use m`（对照） | 放行 | 放行 | 放行 | 已实测（`out/cdx-use-sym-mO1QrV1h/*.cx`） |

注：g3-9 的"注释内 `#if`"之所以无害，是因为 `cxm_load` 在扫描前把 `/*…*/` 与 `//` 内容整体置换为空格
（换行保留），`cxm_directive` 只识别行首 `#`。此条是静态结论，不依赖实测。

## 5. 与 `OPEN-LEX-LINE` 分账

| | CX-G-3 | `OPEN-LEX-LINE` |
|---|---|---|
| 争的什么 | 条件内 pragma **是否**拒绝 | 拒绝时报告的**行号**是拼接前还是物理行 |
| 涉及代码 | `cxm_directive` 的 `*cond > 0` 分支 | 同一函数内 `line` 不随续行递增 |
| 三源全同？ | 是 | 是（同一缺陷面） |
| 本草案处置 | 给措辞，等冻结 | **不处置**；不修三源，差值 1 继续记录 |
| 是否互相吞并 | 否 | 否 |

`OPEN-LEX-LINE` 的现行处置见 `plans/cx-layers-inventory-2026-10-31.md` §8.5：消费行号者必须知道差 1。
G-3 的诊断文本冻结不因行号未决而延后；但 G-3 的"诊断位置"一句必须写明它继承该未决项。

## 6. 冻结前置次序（CX-G-4）

spec §2 末段规定：权威是 `cx/cx0/plan.cx` 的 `CX0_COMPAT_BODY` 区；`src/cxmod.c` 必须与该区逐字一致；
`cx/compiler/cxmod.cx` 实现同一文法；**改文法先改 `plan.cx`**。

因此本条款若被批准，落地次序固定为：

1. `cx/cx0/plan.cx`（权威区）——若选项 A 只是措辞对齐，则**源码零改动**，只改注释与 spec；
2. `src/cxmod.c`——同样零改动，仅需证明与 `plan.cx` 逐字一致；
3. `cx/compiler/cxmod.cx`——同上；
4. 最后改 `cx/spec.md:75` 表行与 `:90` 条款文本；
5. 冻结前 spec 的"条件为真"仍是现行条款；草案不影响现行效力。

⇒ **选项 A 是"改合同"而不是"改实现"的少数情形**：实现已满足 A，spec 未描述 A。
这一点必须在冻结记录里写明，防止后续有人以"实现不符合 spec"为由回改实现。

## 7. 影响面（静态）

| 消费者 | 影响 |
|---|---|
| `tools/symindex.cx` / 分层索引工具 | inventory §7.1 已按 `cond>0` 判，与选项 A 一致；无新增实现 |
| 模块扫描（`cxm_scan` mode 0） | 已是选项 A；无改动 |
| 模块扫描（mode 1，投影/接口生成） | `cxm_directive` 只在 `mode == 0` 时解析 pragma，故 G-3 拒绝总发生在 mode 0；mode 1 仍跟踪 `cond` 但不再判 pragma。冻结时须写明"拒绝发生在首次扫描"，避免实现侧误加第二处判定 |
| 用户代码 | `#if 0` 里藏 `use` 的写法（常见于临时开关）将确定被拒；这是选项 A 的实际代价，须在发布说明出现 |

## 8. 仍 OPEN

| id | 内容 |
|---|---|
| G3-1 | 选项 A 是否被裁定方正式选择（本页按派单 §2 倾向写，未获冻结） |
| G3-2 | spec 措辞是否同步加入"所有分支同罪"一句（§2 第 2 点），还是仅删"为真" |
| G3-3 | CX-G-2 与 CX-G-3 的诊断优先级是否写进 spec（现仅隐含在代码次序里） |
| G3-4 | `OPEN-LEX-LINE` 的行号差是否在同一版修正（本草案不处置） |
| G3-5 | 发布说明是否需要单列"`#if 0` 内 `use` 现在被拒" |

## 9. 证据索引

| 项 | 位置 |
|---|---|
| 判据语句（三源同形） | `plan.cx:441` / `cxmod.cx:412` / `src/cxmod.c:382` |
| `cond` 增减规则 | `plan.cx:431-432`（`if`/`ifdef`/`ifndef` +1；`endif` 且 `>0` 时 −1；`else`/`elif` 不参与） |
| `#define` 只记名字不参与求值 | `plan.cx:433-436` → `cxm_mac[]`，仅供 `cxm_ismac` |
| `seen` 判定在前 | `plan.cx:440` 先于 `:441` |
| mode 门 | `plan.cx:439` 的 `mode == 0` 条件；`cxm_scan` 调用点 `:973`(mode 0) / `:1002`(mode 1) |
| 注释预置换 | `plan.cx:214-245`（`cxm_load`：注释置空格、保留换行、拼接成 `\001`） |
| 条件导出先例 | `cxm_exportok`：`module <m> exports <name> under #if (conditional interface)` |
| 现行 spec 文本 | `cx/spec.md:75`（表行）、`:90`（条款）；CX-G-4 在 `:91` |
| `#if 0` 实测拒绝 | `plans/cx-layers-inventory-2026-10-31.md` §7.1（`dbl` 夹具，拒 `:4`） |
| 行号未决 | 同上 §8.5（`OPEN-LEX-LINE`） |
| 节点位置（不再重跑） | 本轮不执行任何夹具；所有"已实测"均为已记录结果的引用 |
