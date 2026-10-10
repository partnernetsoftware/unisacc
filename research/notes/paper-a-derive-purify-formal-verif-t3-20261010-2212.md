# Paper A 衍生净化：形式化／CompCert 级 T3／Lean 路线表收口不得搭乘 A

- **拍点：** 2026-10-10 ~22:12 Asia/Shanghai（UTC+8）
- **切口：** **E. 衍生净化**
- **仓库 tip（只读核对 / before）：** `6026621bf294634b9d91c056461fb28efed2257a`
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1＝网络≡表；unisacc POSIX C99 跨架构实证）→ 未来 **形式化／机器证明旁支**（CompCert 级 T3、全走查器 P-2、T2a 实现连接、形式化路线表收口）→ 最终仍回 **A**
- **状态：** **note only**（登记未来独立形式化课题；**不起稿正文**；**不改** A 根主张、键数身份、CN/EN/TeX/abstract、product/kernel/weights/facts；**不重写** [`../formalization-roadmap.md`](../formalization-roadmap.md) 指针／`archive/research/formalization-roadmap.md`；**不重写** `research/lean/`）
- **Softguess：** tip 四文件 **NONE×4**（本拍机扫：`Softguess` / `权重就是` / `weights are the logic` / `逻辑本身` / `权重即逻辑` 均 0 hit）
- **测量身份：** 8509/8769 **仍开** — 仅引用 R87/R74（~20:52）与 Ο1/Ο2 正交钉（~20:45）；本拍**不重钉、不叠**；**不叠** ~17:08 决策卡
- **相邻已封／已切（本拍不重做）：**
  - [`paper-a-derive-purify-verif-discipline-beyond-nn-20261010-2030.md`](paper-a-derive-purify-verif-discipline-beyond-nn-20261010-2030.md)（验证纪律 beyond NN → C；**正交**：经验构造+枚举+逐字节差分推广 ≠ 机器证明／T3）
  - [`paper-a-derive-purify-bootstrap-fixedpoint-20261010-1440.md`](paper-a-derive-purify-bootstrap-fixedpoint-20261010-1440.md)（语义自举不动点定理 OUT；**正交**：字节 N1=N2=N3 留 A）
  - tip §2.3／§3.4：T1 定理性；T2a 实现连接、T2b、T3 **明确开放**；「刻意不做」自举不动点／六 ABI／表≡C99／最少单元复杂度机器证明
  - tip §9：CompCert／CakeML = 本文明确不声称的 T3
  - [`../formalization-roadmap.md`](../formalization-roadmap.md) → `archive/research/formalization-roadmap.md`（L0–L3 索引；全 walker P-2 仍开放）
  - 今晚 E：optim volume/speed（~21:53／PR#65）、memsafe D（~21:40／PR#64）；C：平台矩阵 Latest v0.0.39（~21:15）— **不重做**

---

## 主张一句（本页唯一）

**把「必须先完成 CompCert／CakeML 级整机语义保持（T3）、或收口 `formalization-roadmap` 全 walker P-2／T2a 实现连接／自举与六 ABI 机器证明，才算 Paper A 可投／构造法才成立」——或把 A 读成「又一篇验证编译器（verified compiler）论文，缺机器证明则根主张空」——升成 A 投稿前提或否决条件，不得搭乘 A。A 只需构造 + T1（枚举／发布检查）+ 诚实开放义务披露（T2a/T2b/T3 与 §3.4「刻意不做」已写明）+ unisacc 实证；机器形式化收口属未来旁支，挂回 A，不是第二套编译器理论，也不挡 A 投稿。**

---

## 切什么（从 A 的主张负担里切出）

切出对象是 **把 tip 已标明开放／刻意不做的形式化义务，升成投稿闸门或根主张否决条件**，从而在「尚无 T3／全 walker Lean／表≡C99 机器证明」时被挑战者用来否定构造法或 T1：

| 位点 | 摘句／现状要点 | 分类 |
| --- | --- | --- |
| §2.3 证明档位 | T1＝定理性结论；T2a 实现连接、T2b、T3＝明确开放义务 | **keep 档位表**；切出的是「未完成 T2/T3 ⇒ A 不可投／T1 空」 |
| §3.4 形式化义务 | P-8/P-3/P-5 等 Lean 玩具／子情形；「刻意不做：自举不动点与六目标 ABI 的机器证明、真值表 ≡ C99、最少单元数复杂度」 | **keep 义务索引与刻意不做**；切出「必须先做完刻意不做项才能投 A」 |
| §9 CompCert／CakeML | 「证明了整机语义保持，这是本文明确不声称的 T3」 | **keep 对照**；切出「A = verified-compiler 同族、缺 T3 则主张失败」 |
| `formalization-roadmap` | L0–L3；全 walker P-2 仍开放；Lean 证算法声音性 ≠ 替代 CI | **keep 路线索引**；切出「路线表收口＝A seal-before」 |
| 发布检查／枚举 | 出货权重逐键整数网络 + 严格 argmax；错误或并列拒绝 | **keep in A**（T1 工程证据） |
| A 根主张／T1 | 构造网络与权重；声明域精确；unisacc 实证 | **keep in A** |

**不切：** 根主张、键数 8509/8769/9174、§8.1 同身份矩阵、Ο1/Ο2、Softguess、T1 本身、§3.4 已有 Lean 子情形披露、验证纪律 beyond-NN（已切→C）、语义自举不动点（已切）、product/kernel/weights/facts、`formalization-roadmap`／`research/lean/` 正文（本拍只交叉引用）。

---

## 为何不挡 A 投稿

1. **A 根主张不依赖 T3。** 核心是 TSV 表 → 构造网络 → 声明域穷举精确（T1）+ unisacc 实证。tip 已写明 T3 未证、CompCert 级语义保持明确不声称。
2. **开放义务 ≠ 投稿闸门。** §2.3／§8 把 T2a/T2b/T3、全走查器 P-2 列为开放研究／工程义务；诚实披露开放项正是 A 的成本诚实，不是「先证完再投」。
3. **与「验证纪律→C」正交。** ~20:30 切的是经验纪律（构造+枚举+逐字节）推广到手写／非 NN；本拍切的是**机器证明／verified-compiler 同族义务**。二者都挂回 A，但命题不同。
4. **与自举不动点已切正交。** ~14:40 切语义不动点定理；本拍覆盖更广的 T3／全形式化路线收口，不重做字节自举边界。
5. **本拍零 Paper A 正文 diff。** 只 notes + registry。

---

## 父节点如何回 A

- **理论回 A：** 任何未来机器证明若仍关于「构造网络 ≡ 声明表（T1）／控制器替换同余（T2a）」等，其对象仍是 A 的构造法；T3 若成立，是在固定语言子集上加强可观察行为保持，不能改写根主张为「须先 verified」。
- **旁支挂回：** 形式化收口论文／里程碑挂 **A**（可引用 `formalization-roadmap` 与 tip §3.4）；结果可回来收窄 A 对开放义务的语气，不能把「未完成 Lean／T3」写成构造法失败。
- **DAG：** A（construction + T1；enumeration evidence；honest open obligations）→ 未来形式化旁支（T3／全 walker P-2／T2 实现连接）；**不是**与 A 并列的第二套编译器理论，也**不是** CompCert 同族「缺机器证明则无论文」。

---

## 切后 A 哪一句更硬

切出后，A 可把形式化边界收成一句硬边界（**本拍不改正文；仅登记主张**）：

> **Paper A 只主张基于神经网络（构造网络与权重）的编译器与 T1，并以枚举／发布检查与 unisacc 实证为证据；CompCert／CakeML 级 T3、全走查器 P-2 收口、自举／六 ABI／表≡C99 的机器证明属明确开放或刻意不做项，不是 A 投稿前提，也不是「未完成形式化则构造法／T1 不成立」的否决条件——A 也不是又一篇 verified-compiler 故事。**

攻击面从「你们还没有机器证明／还没做成 CompCert，神经编译器主张是空的？」缩回 **「A = 构造 + T1 + 诚实开放义务 + C99 子集实证；形式化收口另挂」**。

---

## 衍生课题提案（不起稿）

| 项 | 内容 |
| --- | --- |
| **暂定题** | Machine Formalisation of Constructed Compiler Networks: From T1 Enumeration to T3-Class Obligations (hangs on A) |
| **一行主张** | Elevating CompCert/CakeML-class T3, full-walker P-2 closure, or completing the formalization roadmap into a Paper A submission gate—or reading A as another verified-compiler paper that fails without machine proofs—is an independent formalisation proposition hanging on A, not required to submit A; A only needs construction + T1 + honest open-obligation disclosure + unisacc empirics. |
| **上游** | Paper A（§2.3／§3.4／§9；T1；[`../formalization-roadmap.md`](../formalization-roadmap.md)）；相邻 [`paper-a-derive-purify-verif-discipline-beyond-nn-20261010-2030.md`](paper-a-derive-purify-verif-discipline-beyond-nn-20261010-2030.md)（正交→C）；[`paper-a-derive-purify-bootstrap-fixedpoint-20261010-1440.md`](paper-a-derive-purify-bootstrap-fixedpoint-20261010-1440.md) |
| **协议指针** | 不起稿不得改 A 键数或 T1；不得把「刻意不做」项偷偷升成 seal-before；不得与 Softguess／8509-8769／Ο1/Ο2／平台矩阵／验证纪律→C／optim／memsafe D 混读；不得写成第二套编译器理论；**不重写** formalization-roadmap／lean 树 |
| **状态** | **note only** — 无独立 TeX/中英草稿；不改正文 |

---

## 与既有衍生／硬缺口的正交

| 主题 | 关系 |
| --- | --- |
| 测量身份 8509/8769（R74/R87） | **仍开硬缺口**；词表轴 ≠ 形式化收口 |
| 同身份平台矩阵 §8.1（Latest v0.0.39） | **仍开 · seal-blocking**；矩阵 ≠ T3 |
| 产物轴 Ο1/Ο2（~17:08 卡） | **正交**；本拍**不叠、不代裁** |
| Softguess | **NONE×4**；封口地位仍 OUT-OF-SCOPE（~12:30）；不粘 |
| verif-discipline → C（~20:30） | **正交**：经验纪律推广 ≠ 机器证明 |
| bootstrap-fixedpoint（~14:40） | **相邻**：语义不动点已切；本拍覆盖 T3／路线表收口更广一档 |
| optim volume/speed（~21:53）／memsafe D（~21:40） | **正交** |
| formalization-roadmap／lean | **引用、不重写** |

---

## 明确不做

- 不改正文 CN/EN/TeX/abstract（零 Paper A prose diff）
- 不改键数身份 8509/8769/9174；不改 product/kernel/weights/facts
- 不起稿形式化论文；不重写 `formalization-roadmap.md`／`research/lean/`；不叠 Ο1/Ο2；不重钉 8509/8769；不刷新平台矩阵
- 不把形式化写成第二套编译器理论或「缺 T3 则 T1 空」；不发明 Softguess 证据；不删 §3.4 已有 Lean 披露

---

## 证据指针（本拍）

- tip before：`6026621bf294634b9d91c056461fb28efed2257a`
- Latest 产品标签事实：`v0.0.39`（公开 SHA-256 `6b2b9680…`；本拍不刷新矩阵）
- 四 blob（本拍零正文 diff）：CN `74893e3df1f9663b5d11f5bad6dc3cf0ca59e884` · EN `95657738e0b447ee4e40ea9ca3dd961329aebffc` · TeX `3830ede13556e8c9673f994fd6d843937476ecf5` · abs `43d7ea7e7297ed8be9a96e3950e014e359467973`
- Softguess 机扫：**NONE×4**
- 路线指针：[`../formalization-roadmap.md`](../formalization-roadmap.md)；DAG 笔记 [`../paper-notes-20261009.md`](../paper-notes-20261009.md)
- 短报：[`paper-a-heartbeat-report-20261010-2212.md`](paper-a-heartbeat-report-20261010-2212.md)
- 父节点：**A → 形式化旁支（T3／Lean 收口）→ A**
