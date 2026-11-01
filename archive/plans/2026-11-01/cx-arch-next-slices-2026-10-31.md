# CX 架构下一刀切片（csr 设计台账，2026-10-31）

状态：**设计调度**。不改 `cx/spec.md`；不授权安装/发布；不派 grk；不把 PASS 当未覆盖路径已验。
施工按车道所有权经 `moltbaby/bin/envelope` 下达；本文件冻结本轮优先级与合同边界。

## 0. 本轮裁定与关闭项

| 项 | 状态 | 证据 / 门 |
|---|---|---|
| `mg_append` 自重叠扩容 | **源码刀关闭** | commit `93e8959c13`（仅 `cx/mem/grow.cx` + `tools/check-cx-grow.cx`）；缺陷锚 `836d041883`；旧失败探针不复跑。产品 sha 仍须**所有者核验**后才能写进发布账。 |
| CX-G-1 / W-7 四故障注入 | 另账未关 | 不并入本轮；不 rerun 已报失败。 |
| CX-U-2 隐式调用后果 | **OPEN** | 不得用入口分叉冒充条款已满。 |
| 身份四项（identity handoff） | **门未开** | 未完成书面细化前禁止改 `cxmod` / `entry-build` / spec。 |
| preflight | go 所有；csr **只读** | 不授权新测根/工具/measure。 |
| 未跟踪根文件 `a` | 不动 | — |

## 1. 优先级（本轮）

1. **A 刀缺口推进**（omp-go；`5568` 文案 + `ce35` lex12 + `c45b977d2a` U/manifest12 已收，checks=40/0；下一步余 6 + edges 差解释）  
2. **`::sym` 多声明合同草案**（omp-ds 已交 `914a784959` → `plans/cx-use-sym-multi-2026-10-31.md`；**csr 审阅收为冻结候选**；SYM-2/3/4/5/6 仍 OPEN → **禁止三源施工**）  
3. **CX-G-3 合同对齐草案**（omp-ds 已交同提交 → `plans/cx-g3-conditional-pragma-2026-10-31.md`；**选项 A 措辞 csr 预批为冻结候选**；**仍不改 spec/三源**，待书面冻结令）  
4. 身份四项继续细化（预算对照页已交 → `plans/cx-budget-diagnostic-crosswalk-2026-10-31.md`；仍草案；无三源施工）  
5. 其余（W-7 余项、G-3 真求值、lexical 共享扫描器、sy_sha L0、eb_real）单列，不抢刀。

### 1.1 ds 三草案身份（`914a784959`）

| 文件 | 行 | sha256 前缀 | csr 审阅 |
|---|---|---|---|
| `plans/cx-use-sym-multi-2026-10-31.md` | 208 | `2f854a9e…` | 对齐 §3；L-G/L-D、S1+S4 同改、C1–C8、分账齐全 → **冻结候选** |
| `plans/cx-g3-conditional-pragma-2026-10-31.md` | 142 | `8453be0c…` | 对齐 §2 选项 A；exportok 先例、G-2≻G-3、OPEN-LEX-LINE 分账、零实现改 → **冻结候选** |
| `plans/cx-budget-diagnostic-crosswalk-2026-10-31.md` | 123 | `68981324…` | 对齐 §4；B1–B9 分量纲、前缀防吞、MACROS/renpool/eb_real 已标 → **补充页收** |

门：**冻结令未发** ⇒ 三源 / `cx/spec.md` / 夹具开测 / 工具新建 **一律禁止**。

## 2. CX-G-3 设计倾向（csr 预裁定，未生效）

实证：三源只计 `#if/#ifdef/#ifndef` **嵌套深度**、不求真假；现有 `#if 0` 夹具要求拒绝。  
规格现写「条件为真的分支」。

**倾向选项 A（合同对齐实现）**：未来修订规格措辞为——在任意条件预处理嵌套内禁止 `#pragma unisa …`（不论该分支活跃与否）。理由：与现行三源及夹具一致；真活跃分支求值成本高且会改语义面。  
**选项 B**（实现真活跃分支）本轮不做；若将来要做，须单独兼容性评估与夹具重写。

门：ds 写出一页草案（诊断前缀、夹具矩阵、与 OPEN-LEX-LINE / `#` 后空格分账）；**在 cdx/csr 书面冻结并授权前，三源与 `cx/spec.md` 均不动**。

## 3. `::sym` 多声明：合同必须拆开的两层（给 ds）

已实证（装机产品 + 两宿主驱动局部）：同单元连续 `use m::x` / `use m::y` 后取后者失败；`use m::x` 后再 `use m` 取 `&y` 失败，反序成功。候选原因：`cxm_adduse` 目标去重早退在记录 `sym`/`alias` 之前；`plan.cx` 可见性遇同目标首项符号不匹配也早退。

合同草案须**显式分离**：

1. **图节点身份**：按 realpath（现行）去重模块对象；一只物理模块一只图节点。  
2. **导入声明记录**：每条 `#pragma unisa use …` 是独立声明；不得因图去重而丢弃后到声明的 `sym`/`alias`/整模块投影。

须写清（正例/反例表，仍不改三源）：

- 多次 `::sym`：可见集为声明并集；顺序无关。  
- 整模块与 `::sym` 混用：整模块 ⊇ 符号集；混用后有效可见集规则；与别名冲突。  
- 别名冲突、重复同 sym、预算 `CXM_MAXUSE`（超限具名拒绝，不静默丢）。  
- CXIF 投影按**声明集**而非「首条命中」；函数体别名重写若每模块一份，须说明多别名/多消费者如何计量。  
- 与路径身份刀、CX-U-2 OPEN、隐式声明入口分叉 **分账**；本刀不解释隐式调用。  
- 引用证据：架构演进文 `::sym` 段；提交/探针 `b0430c5438` / `ae1f4b8feb` / `out/cdx-use-sym-mO1QrV1h/`（若仍在）。

产出路径建议：`plans/cx-use-sym-multi-2026-10-31.md`。冻结前 **禁止** 改 `cxmod`/`plan.cx`/`cxcc`。

## 4. 身份四项（仍阻塞施工）

见 `plans/cx-identity-handoff-2026-10-31.md`。本轮若有余力，ds 可只补：预算量纲命名、诊断前缀与「构造级合并 vs 单根 plan」对照表——仍标草案。不开测；不改入口。

## 5. A 刀（omp-go）本轮硬边界

工具：`tools/check-cx-layers.cx`。文案卫生：`5568bcc1e3`（A-INCOMPLETE 仍写 `lexical-12 and named-fixture increment not implemented (U reconciliation is implemented)`）。lexical-12 具名：`ce35bbf868`。U/manifest 具名：`c45b977d2a`（u-equal-duplicate / same-count-different-name / extra / missing / prefix-match / empty / sum-total；manifest-unescape mixed/plain/too-long/unknown-kept/empty）。隔离 `--fixtures`：`checks=40 failed=0`（旧 16+12 lex+12）；证据 `out/cx-w7-isolated-1793437777/layers-lex5.log`。**A 仍 rc3 / 不得 PASS**；≠ AGGREGATE incomplete。

尚缺 **6**（勿口头改数）：digest/aba、missing-shard/orphan、escape/path-too-long 端到端、`.git/index` 前后自检（及合同所列同类）；另须同输入 edges 差可复查解释。

允许（下一刀）：

- 按 inventory 已批合同补齐剩余 **6** 条具名夹具。  
- 同输入 edges 前后差给出可复查解释（仍只记账，不 PASS）。  
- A-INCOMPLETE 文案仅在 6+edges 解释齐后、csr 书面核后才可改写；此前保持 rc3。

禁止：

- 记 A 刀 PASS；`undecided>0` 或缺口未清时不得改写为通过。  
- 动 preflight 测根/工具/measure；动 `cxmod`/entry-build/spec；动 grk L0 `sys.sha`；动未跟踪 `a`；复跑已报失败探针 / mg_append 旧探针。  
- 未经 `::sym` 合同冻结改三源导入可见性。

## 6. 车道与指挥

| 窗 | 本轮 |
|---|---|
| `1:omp-go` | §5 A 刀；架构演进 mg_append 状态一行；待命实现 `::sym`（合同冻结后另信封） |
| `1:omp-ds` | §3 合同草案；§2 G-3 对齐草案；可选 §4 身份细化；只读审计/文档 |
| omp-grk | **不派** |
| omp-csr（本角） | 设计台账、终裁、信封调度；preflight 只读 |

回复经 mux/envelope；证据落 `out/` 忽略树或既有隔离目录；共享检出外来脏文件不还原、不借工作区编译冒充隔离证据。
