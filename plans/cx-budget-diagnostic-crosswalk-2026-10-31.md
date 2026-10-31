# 附加页：身份与导入的预算量纲 / 诊断前缀对照（2026-10-31）

状态：**补充页，不改任何原件**。`plans/cx-identity-handoff-2026-10-31.md` 属 cdx 移交范围，本页不替换、不修订它；
`plans/cx-use-sym-multi-2026-10-31.md` 是同一批的合同草案。本页只做一件派单要求的事：
把两笔账里的**预算量纲**与**诊断前缀**并到一张可核对的表上，避免实施方把不同量纲压成一个上限，
或把两类不一致混用同一前缀。本文档不含实测数值；所有限额除注明为现行源码常量外均为 UNKNOWN。

---

## 1. 现行源码里已经存在的量纲（可核对，非提议）

| 量纲 | 位置 | 现行值 | 超限诊断 | 作用域 |
|---|---|---|---|---|
| 图节点 / 单元 | `CXM_MAXU` | 64 | `more than 64 units in one program` | 一次规划 |
| 每单元导入声明 | `CXM_MAXUSE` | **32**（`plan.cx:78`、`src/cxmod.c:28`）/ **48**（`cxmod.cx:29`） | `more than 32 uses in one unit` / `… 48 …` | 每使用方单元 |
| 接口文本字节 | `CXM_IFMAX` | 262144 | `interface too large` | 每单元 |
| 导出名总数 | `CXM_SYMS` | 2048 | `too many names for the module check` | 一次规划 |
| 文件域项数 | `CXM_ITEMS` | 8192（`plan.cx:83`、`cxmod.cx:34`） | `too many file-scope items for the module scan` | 每单元 |
| 宏名缓冲 | `CXM_MACROS` | 1024（`plan.cx:86`、`cxmod.cx:37`） | 超限即静默不再记录（**无诊断**） | 每单元 |
| 改名/投影池 | `CXM_MAXU * CXM_MAXUSE` | 2048 / 3072（随 `CXM_MAXUSE` 分歧） | 无独立名称（越界风险） | 一次规划 |
| 构造闭包 | `EB_MAXC`（`tools/entry-build.cx:333`） | 128 | `closure-cap` | 每构造实例 |
| 真实路径宽 | `EB_REALW`（`entry-build.cx:334`） | 1024 | `plan-path` | 每路径 |

两处需要单列的事实：

- **`CXM_MACROS` 超限无诊断**。`cxm_directive` 的 `#define` 记录是 `if (… && cxm_nmac < CXM_MACROS)`（`plan.cx:435`），
  满了就静默丢弃。后果不是中性：`cxm_ismac`（`plan.cx:525` 附近）查不到该名字，`cxm_scanreg` 会把对它的调用
  当作普通函数调用，进而可能落到 `cxm_sees`/`cxm_owner` 并报 `call to undeclared function`——
  **超限表现为下游误诊，而非具名拒绝**。这违反"超限具名拒绝"的一般要求，是现存缺陷，不在本刀修，
  但两张预算表都必须标它。
- **`cxm_renpool` 无独立诊断、无边界检查**。池索引直接使用 `cxm_renpool[cxm_nren]`
  （`plan.cx:1026`、`cxmod.cx:1146`、`src/cxmod.c:973`），使用前**没有** `cxm_nren < 容量` 判定，
  `cxm_nren` 随后无条件递增。上限只是数组维度 `CXM_MAXU * CXM_MAXUSE`（2048 / 3072，随 `CXM_MAXUSE` 分歧）。
  多声明一旦放开，这个隐含关系失效（见 `cx-use-sym-multi` §C6），且越界是**写越界**而非具名拒绝。

## 2. 提议量纲（两笔账合并视图）

identity 刀 §2 提出四种，`cx-use-sym-multi` §C6 提出四种。它们的**单位互不相同**，因此不可互相顶替：

| # | 量纲 | 定义 | 出处 | 作用域 | 上限 | 超限诊断（提议） |
|---|---|---|---|---|---|---|
| B1 | **上下文数** | 共享同一 realpath 的可达词法入口个数 | identity §2 | 每 realpath | UNKNOWN | `module context budget exceeded` + `contexts` |
| B2 | **核验对数** | 需两两比较的上下文对（有序与否须冻结） | identity §2 | 每 realpath 或每构造 | UNKNOWN | 同上 + `pairs` |
| B3 | **边检查数** | 递归核验中比较的直接 `use` 边总数 | identity §2 | 每 realpath 闭包 | UNKNOWN | 同上 + `edges` |
| B4 | **路径字节数** | 参与比较的路径串总字节（含两侧词法目标与真实目标） | identity §2 | 每次核验 | UNKNOWN | 同上 + `bytes` |
| B5 | **图节点数** | 一次规划内单元数 | 本页 §1 | 一次规划 | 64（现行） | `more than 64 units in one program` |
| B6 | **导入声明数** | 每使用方单元的 `use` 出现条数 | sym §C4 | 每单元 | 32（现行） | `more than 32 uses in one unit` |
| B7 | **投影条目数** | 每单元的 `(sym, alias)` 投影条数 | sym §C5/C6 | 每单元 | 隐含于 renpool | 需新增具名诊断 |
| B8 | **改名源数** | 每 `(物理模块, 符号选择集合)` 一份 | sym §C6 | 一次规划 | 现行 1/模块 | 需新增具名诊断 |
| B9 | **构造闭包数** | 跨计划合并后的对象数 | §1 | 每构造实例 | 128（现行） | `closure-cap` |

要点：

1. B1–B4 是**核验成本**；B5–B9 是**产物规模**。前者可以只读、可以只在需要时算；后者决定内存与落盘。混用会让
   "核验超预算"被误报成"程序太大"。
2. B6 与 B7 **不是**同一件事：一条 `use m` 可以产生零个投影条目，一条 `use m::x as a` 产生一个；
   受同一物理模块的多条声明影响，B6 增而 B7 可能不增。所以两条限额不可合并写成一个数。
3. B8 是目前唯一会因"多声明"而增加的**产物**量；它没有现行上限，属新增约束（sym §C6、OPEN 项 SYM-2）。
4. 全部 B1–B4、B7、B8 的具体数值 **UNKNOWN**。派单明确禁止在本轮测预算；此表只给量纲归属与诊断槽位。

## 3. 诊断前缀对照（防吞并）

| 前缀（提议） | 归属 | 不得吞并 | 备注 |
|---|---|---|---|
| `module context mismatch: dependency` | identity 依赖不一致 | 公开集不一致、预算耗尽、清单错误 | 两段都必须断言，仅匹配共同部分不算覆盖 |
| `module context mismatch: public set` | identity 公开集不一致 | 同上 | 声明级集合，不是运行可达集 |
| `module context budget exceeded` | identity B1–B4 | 不一致类；也不与 `more than N uses` 混用 | 须附所触量纲名 |
| 清单错误具名拒绝 | identity 清单读取/非法/超限 | 不得降级为默认全集 | EACCES/ENOTDIR ≠ ENOENT |
| `module cycle: …`（现行） | 现行图合同 | 不得改报为身份不一致 | 保持原诊断 |
| `more than 32 uses in one unit`（现行） | sym B6 | 不得改报为 `budget exceeded` | 三源文本当前 32/48 不一致，见 §1 |
| 投影/改名源超限（新增） | sym B7/B8 | 不得降级为首入者胜、不得静默截断 | 文本待冻结 |
| `unisa pragma inside conditional`（现行） | CX-G-3 | 不与 `OPEN-LEX-LINE` 行号差混写 | 见 `cx-g3-conditional-pragma-2026-10-31.md` |
| `unisa pragma after first declaration`（现行） | CX-G-2 | 优先级高于 G-3（三源同序） | 冻结时须写明次序 |

规则：**一个前缀只对应一个类别，一个类别只对应一个前缀**。若实施方需要合并输出以提高可读性，只能在同一行里附加
子句（如 `… budget exceeded: contexts=… bytes=…`），不得复用另一类的前缀主词。

## 4. `eb_real` 回退键 ⇒ 构造级一致性 UNKNOWN

`tools/entry-build.cx:346-349`：

```c
static void eb_real(const char *p, char *dst, int cap) {
    if (sy_realpath(p, dst, cap) >= 0) return;
    eb_copy_tok(dst, cap, p);
}
```

去重判定（`:558-570`）以这个函数产出的字符串为键，与 `eb_rootreal[]`、`eb_creal[]` 比较后 `continue` 丢弃重复项。
后果：

- realpath 失败时键退化为**词法路径**；两条本应合并的路径可能因一侧失败而各占一槽，或一条词法路径与另一条
  真实路径的别名比较出不相等。
- 因此**含回退键的比较集合不能用于宣称"身份一致"**；后续路径解析成功也不追认前面的比较（identity §3 已写，此处给出源码依据）。
- 该键误差还影响 B9（闭包数）与产物槽位分配；同一构造实例里若出现回退键，B9 的计数与对象数为 UNKNOWN。

正面结论：`eb_real` 的失败是**静默**的（无诊断），所以构造器层面今日无法区分"合并成功"与"回退后误判不同"。
这一条应进 identity 刀的拒绝类清单（提议：回退即 UNKNOWN 非零，或至少具名），本页不改原草案，只标记。

## 5. 仍 OPEN

| id | 内容 | 归属 |
|---|---|---|
| BD-1 | B1–B4 的具体数值与是否按构造实例重算 | identity 实施方 |
| BD-2 | B2 的"对"是否有序（影响是否反序两次即可） | identity 实施方 |
| BD-3 | B7/B8 是否新增上限，取值依据 | sym 实施方 |
| BD-4 | `CXM_MACROS` 静默丢弃是否本版修 | 裁定方 |
| BD-5 | `CXM_MAXUSE` 32/48 三源分歧的裁定 | 发布方 |
| BD-6 | `eb_real` 回退是否升格为具名拒绝 | identity + cdx |

## 6. 证据索引

| 项 | 位置 |
|---|---|
| `CXM_MAXUSE` 三源值与文本 | `cx/cx0/plan.cx:78,403`；`cx/compiler/cxmod.cx:29,385`；`src/cxmod.c:28,348` |
| 无诊断的宏名缓冲 | `plan.cx:435`（`cxm_nmac < CXM_MACROS` 条件） |
| renpool 使用点 | `cxmod.cx` 体投影循环内 `cxm_renpool[cxm_nren]` |
| 构造闭包上限与真实路径宽 | `tools/entry-build.cx:333-334` |
| `eb_real` 回退 | `tools/entry-build.cx:346-349` |
| 去重比较 | `tools/entry-build.cx:558-570` |
| four-dimension 原文 | `plans/cx-identity-handoff-2026-10-31.md` §2 |
| 五类拒绝原文 | 同上 §4 |
| sym 侧量纲 | `plans/cx-use-sym-multi-2026-10-31.md` §C4/§C5/§C6 |
