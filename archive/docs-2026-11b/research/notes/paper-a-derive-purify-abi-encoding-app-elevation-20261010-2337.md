# Paper A 衍生净化：ABI／编码／指令选择／重定位表升成独立应用论文不得作 A 投稿闸门

- **拍点：** 2026-10-10 ~23:37 Asia/Shanghai（UTC+8）
- **切口：** **E. 衍生净化**
- **仓库 tip（只读核对 / before）：** `6e62544fd14d079fd49aa28e6e8fc1315dc3930b`
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1；仓内 abi／isel／reloc／encoding 表仍作 A 实证证据）→（当某一类表面对另一种语言／目标时升成应用，与 B／wasm 同类）→ 未来 **ABI／encoding／calling-convention 应用旁支** → 最终仍回 **A**
- **状态：** **SEALED**（本 locus 登记完成；**note only**；**不起稿正文**；**不改** A 根主张、键数身份、CN/EN/TeX/abstract、product/kernel/weights/facts；**不重做** [`paper-a-derive-purify-wasm-app-20261010-2231.md`](paper-a-derive-purify-wasm-app-20261010-2231.md)／[`paper-a-derive-target-parameterization-20261010-1256.md`](paper-a-derive-target-parameterization-20261010-1256.md)）
- **Softguess：** tip 四文件 **NONE×4**（本拍核对 Paper A 四 blob SAME：CN `74893e3d…` · EN `95657738…` · TeX `3830ede1…` · abs `43d7ea7e…`；相对 prior heartbeat 同 tip 链）
- **测量身份：** 8509/8769 **仍开** — 仅引用既有钉；本拍**不重钉、不叠**；**不叠** ~17:08 决策卡
- **相邻已封／已切（本拍不重做）：**
  - [`paper-a-derive-purify-wasm-app-20261010-2231.md`](paper-a-derive-purify-wasm-app-20261010-2231.md)（独立 wasm 应用文；**相关但已切**：wasm 是「表类面对另一语言／目标」的升法实例；本拍切的是 **把仓内 abi／isel／reloc／encoding 升成独立应用论文／投稿闸门**，不是再切 wasm）
  - [`paper-a-derive-target-parameterization-20261010-1256.md`](paper-a-derive-target-parameterization-20261010-1256.md)（目标参数化／跨架构不变式；**正交**：跨架构不变式定理 ≠ ABI 表升成应用论文闸门）
  - [`paper-a-derive-purify-refusal-product-surface-20261010-2327.md`](paper-a-derive-purify-refusal-product-surface-20261010-2327.md)（诊断／拒绝产品表面；**正交**）
  - [`paper-a-derive-purify-pipeline-method-paper-c-20261010-2302.md`](paper-a-derive-purify-pipeline-method-paper-c-20261010-2302.md)（完整管道方法 C；**正交**）
  - [`paper-a-derive-purify-llm-coevolve-outside-pipeline-20261010-2242.md`](paper-a-derive-purify-llm-coevolve-outside-pipeline-20261010-2242.md)（管道外 LLM；**正交**）
  - [`paper-a-derive-purify-optim-volume-speed-20261010-2153.md`](paper-a-derive-purify-optim-volume-speed-20261010-2153.md)（优化；**正交**）
  - [`paper-a-derive-purify-memsafe-node-paper-d-20261010-2140.md`](paper-a-derive-purify-memsafe-node-paper-d-20261010-2140.md)（memsafe D；**正交**）
  - [`paper-a-derive-purify-formal-verif-t3-20261010-2212.md`](paper-a-derive-purify-formal-verif-t3-20261010-2212.md)（T3；**正交**）
  - paper-notes §应用：「编码和调用约定可以单独被看成应用…论文上它们仍是 A 的证据。只有当某一类表要面对另一种语言或另一种目标时，才升成应用节点，wasm 就是这种升法。」——本拍升格 derive-purify（**禁止把该升法读成 A 投稿闸门／第二根／方法不完整否决**）

---

## 主张一句（本页唯一）

**把「必须先单独发表 ABI／指令选择／重定位／编码（calling-convention）应用论文」——或把仓内 abi／isel／reloc／encoding 表读成第二根、或主张「未另起应用文则构造方法不完整」——升成 Paper A 投稿闸门或根主张否决，不得搭乘 A。A 在构造 + T1 下继续把这些表当作实证证据；未来升格（面对另一语言／目标时）挂回 A（与 B／wasm 同类），现在不起稿，不挡 A。**

---

## 切什么（从 A 的主张负担里切出）

切出对象是 **把「ABI／encoding／isel／reloc／calling-convention 升成独立应用论文」升成投稿闸门、根主张否决、或封口前必须先交齐的应用文义务**，与已切 wasm 升法实例及目标参数化不变式正交：

| 位点 | 摘句／现状要点 | 分类 |
| --- | --- | --- |
| paper-notes §应用 | 「编码和调用约定可以单独被看成应用。仓里的实证本来就有 abi、指令选择、重定位这类表。论文上它们仍是 A 的证据。只有当某一类表要面对另一种语言或另一种目标时，才升成应用节点，wasm 就是这种升法。」 | **keep 意向句 + 「仍是 A 的证据」**；切出「须先写成独立应用论文／须先交齐 ABI·isel·reloc 应用文才可投 A／缺则方法不完整」 |
| 仓内 abi／isel／reloc／encoding 表 | unisacc POSIX C99 实证载体上的构造表 | **keep 作 A 实证证据**；切出「升成第二根或独立投稿义务」 |
| wasm-app `-2231` | 独立 wasm 应用文不得作 A 闸门 | **已切升法实例**；本拍**不重做** wasm；本拍切的是 **仓内表类本身被升成闸门** |
| target-parameterization `-1256` | 跨架构若只换编码表、网络仍同一构造＝目标参数化；A 不写已证不变式 | **正交**：不变式定理负荷 ≠ 本拍应用论文闸门 |

**不切：** 根主张、键数 8509/8769、§8.1 同身份矩阵、Ο1/Ο2、T1、仓内表作为 A 证据的地位、wasm 已切项、目标参数化已切项、product/kernel/weights/facts、本拍不改正文。

---

## 为何不挡 A 投稿

1. **paper-notes 已写明：论文上它们仍是 A 的证据。** 升成应用节点的条件是「面对另一种语言或另一种目标」——那是未来挂回 A 的旁支，不是 A 封口前义务。
2. **A 根主张不依赖单独 ABI 应用文。** 核心是表→构造网络→T1；abi／isel／reloc／encoding 是同一构造法下的实证表类。
3. **与 wasm 升法一致、但不重做 wasm。** wasm 已登记「独立迁移文不得作闸门」；本拍堵住对称攻击面：「你们还没单独发 ABI／isel／reloc 应用文，方法不完整？」
4. **政委投稿纪律。** A 先独立能投；应用旁支不得升成闸门——本拍执行该纪律的登记面。
5. **本拍零 Paper A 正文 diff。** 只 notes + registry。

---

## 父节点如何回 A

- **证据回 A：** 仓内 abi／isel／reloc／encoding 表仍是 A（construction + T1）下 unisacc 实证证据，不是第二根。
- **升格回 A：** 当某一类表面对另一种语言／目标时，升成应用节点的挂法与 B／wasm 同类，父节点仍是 A。
- **DAG：** A（construction + T1 + in-repo abi/isel/reloc/encoding as evidence）→（未来，面对另一语言／目标时）ABI／encoding／calling-convention application（undrafted，与 B／wasm 同类）；**不是**第二根，也不是 wasm／target-parameterization／optim／C／D 重切。

---

## 切后 A 哪一句更硬

切出后，A 可把边界收成一句硬边界（**本拍不改正文；仅登记主张**）：

> **Paper A 只主张基于神经网络（构造网络与权重、非训练）的编译器与 T1，并以仓内 abi／指令选择／重定位／编码表作为该主张的实证证据；把这些表升成必须先单独发表的应用论文（或主张「未另起应用文则方法不完整／构成第二根」）属未来应用旁支，仅在面对另一种语言或另一种目标时挂回 A（与 B／wasm 同类），现在不起稿，不是 A 投稿前提，也不是根主张否决条件。**

攻击面从「你们还没有独立的 ABI／isel／reloc／encoding 应用论文，方法对目标／调用约定不完整？」缩回 **「A = 构造 + T1 + 仓内表作证据；应用升格另挂、不起稿不挡投」**。

---

## 衍生课题提案（不起稿）

| 项 | 内容 |
| --- | --- |
| **暂定题** | Encoding / ABI / Instruction-Selection / Relocation Tables as Application Papers (hanging on A; same class as B/wasm when facing another language/target; undrafted) |
| **一行主张** | Elevating "must first publish standalone ABI/isel/reloc/encoding application papers" (or treating those in-repo tables as a second root / as incomplete method unless separate app papers exist) into a Paper A submission gate or root-claim veto is forbidden; future elevation hangs on A; undrafted now; does not block A. |
| **上游** | Paper A（构造 + T1；仓内表作证据）；升格条件对齐 B／wasm；**不**吞并 wasm 已切项、target-parameterization、optim／C／D／T3 |
| **协议指针** | 不起稿不得改 A 键数或 T1；不得把「可看成应用」读成封口前应用文义务；不得与 Softguess／8509-8769／Ο1/Ο2／平台矩阵／wasm-app／target-parameterization／refusal／C／LLM／optim／memsafe D／T3 混读；**不重做** wasm／target-parameterization |
| **状态** | **SEALED** for this locus — note only；无独立 TeX/中英草稿；不改正文 |

---

## 与既有衍生／硬缺口的正交

| 主题 | 关系 |
| --- | --- |
| 测量身份 8509/8769 | **仍开硬缺口**；本拍只引用，不重论 |
| 平台矩阵 §8.1 | **仍开**；正交 |
| Ο1/Ο2 | **仍开**；不叠决策卡 |
| wasm-app `-2231` | **相关已切**：升法实例；本拍切闸门读法，不重做 wasm |
| target-parameterization `-1256` | **正交**：跨架构不变式定理 ≠ ABI 应用论文闸门 |
| refusal-product-surface `-2327` | **正交**：产品表面／UX ≠ 编码／ABI 表升格 |
| Softguess | **NONE×4**；非硬缺口 |

---

## 明确不做

- 不改正文 CN/EN/TeX/abstract
- 不改键数身份；不改 product/kernel/weights/facts
- 不起稿；不叠 ~17:08 决策卡；不重钉 8509/8769
- 不重做 wasm／target-parameterization／refusal／C／LLM／optim／memsafe D／T3／finiteness
- 不把仓内 abi／isel／reloc 表移出 A 证据位；不发明第二根

---

## 证据（本拍）

- tip before：`6e62544fd14d079fd49aa28e6e8fc1315dc3930b`
- Paper A 四 blob SAME：CN `74893e3d…` · EN `95657738…` · TeX `3830ede1…` · abs `43d7ea7e…`
- Softguess NONE×4；Latest 仍 v0.0.39
- 挡粘仍开：平台矩阵 §8.1、8509/8769、Ο1/Ο2
- 机读：paper-notes §应用「编码和调用约定可以单独被看成应用」意向句存在；既有 derive-purify 无 `abi-encoding-app-elevation` slug（wasm 仅升法实例；target-parameterization 仅不变式面）
- 短报：[`paper-a-heartbeat-report-20261010-2337.md`](paper-a-heartbeat-report-20261010-2337.md)