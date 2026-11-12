# Paper A 衍生净化：完整管道方法（Paper C）不得作 A 投稿闸门

- **拍点：** 2026-10-10 ~23:02 Asia/Shanghai（UTC+8）
- **切口：** **E. 衍生净化**
- **仓库 tip（只读核对 / before）：** `bb5795679bf9fb3eb6d41f5631e4fb5a61ef3687`
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1；unisacc 管线实证）→ 意向 **Paper C**（管道方法：种子机／迭代脱离／组合精确性／新场景迁移）→ 最终仍回 **A**
- **状态：** **note only**（登记未来独立课题；**不起稿正文**；**不改** A 根主张、键数身份、CN/EN/TeX/abstract、product/kernel/weights/facts；**不重写** [`../paper-c-intent.md`](../paper-c-intent.md)）
- **Softguess：** tip 四文件 **NONE×4**（本拍机扫：`Softguess` / `权重就是` / `weights are the logic` / `逻辑本身` / `权重即逻辑` 均 0 hit）
- **测量身份：** 8509/8769 **仍开** — 仅引用 R87/R74（~20:52）与 Ο1/Ο2 正交钉（~20:45）；本拍**不重钉、不叠**；**不叠** ~17:08 决策卡
- **相邻已封／已切（本拍不重做）：**
  - [`paper-a-derive-purify-verif-discipline-beyond-nn-20261010-2030.md`](paper-a-derive-purify-verif-discipline-beyond-nn-20261010-2030.md)（验证纪律 beyond NN → C；**正交**：经验纪律推广 ≠ 完整管道方法／种子脱离／≥3 非编译器场景收口）
  - [`paper-a-derive-purify-combo-merge-theorem-20261010-1552.md`](paper-a-derive-purify-combo-merge-theorem-20261010-1552.md)（阶段合并定理 → A2 RQ1／C；**正交**：合并可达性 ≠ C 成文／投稿闸门）
  - [`paper-a-derive-purify-finiteness-control-tables-20261010-1629.md`](paper-a-derive-purify-finiteness-control-tables-20261010-1629.md)（有限性 → 意向 E；C 的相邻上游，非本拍）
  - [`paper-a-derive-purify-formal-verif-t3-20261010-2212.md`](paper-a-derive-purify-formal-verif-t3-20261010-2212.md)（T3／形式化；**正交**：机器证明 ≠ 管道方法推广）
  - [`paper-a-derive-purify-llm-coevolve-outside-pipeline-20261010-2242.md`](paper-a-derive-purify-llm-coevolve-outside-pipeline-20261010-2242.md)（管道外 LLM；**正交**：管道外工作流 ≠ C 管道内方法）
  - [`../paper-c-intent.md`](../paper-c-intent.md)（意向书 2026-09-25：种子机／迭代脱离／组合精确性／≥3 新场景；**不**改本拍）
  - 今晚其他 E：wasm／T3／optim／memsafe D — **不重做**

---

## 主张一句（本页唯一）

**把「必须先完成意向 Paper C 的完整管道方法（种子机→迭代脱离→定义域闭包／组合精确性形式化→≥3 个非编译器迁移场景收口）才算 Paper A 可投／构造法才完整」——或把 paper-c-intent 里「A、B、C 三条并行主线」读成 A 不可独立封口、须与 C 同捆提交——不得搭乘 A。A 只需构造 + T1 + unisacc（POSIX C99）管线实证；完整管道方法属意向 Paper C，挂回 A，现在不起稿，不是 A 投稿前提。**

---

## 切什么（从 A 的主张负担里切出）

切出对象是 **把「完整管道方法／Paper C 成文」升成投稿闸门或根主张否决条件**，以及把 A 正文里「方法推广到整条编译管线」的**实证叙述**读成「须先交齐 C」：

| 位点 | 摘句／现状要点 | 分类 |
| --- | --- | --- |
| tip CN §10／结论 | 「沿着『有限控制 + 通用存储』的分解，这一方法从单个决策推广到整条编译管线」——unisacc 实证叙述 | **keep 实证叙述 in A**；切出「整条管线推广＝已证一般管道方法定理＝须先写完 C」 |
| tip CN 贡献／RQ4／§7.4 | 验证纪律可迁移经验；已切 → C（`-2030`） | **已切**；本拍**不重做**纪律推广；本拍切的是**整份 C 方法包**（种子／脱离／新场景） |
| `paper-c-intent.md` | 「A、B、C 是三条并行主线」；C 回答「为什么能推广、条件、步骤能否机械化」；A/B 只作迁移实例 | **keep 意向**；切出「并行主线＝A 封口须捆绑 C」／「缺 C 则方法不完整」 |
| C §5 实践计划 | ≥3 新场景（汇编器／正则／协议／构建求解…） | **OUT → C**；**不得**作 A seal-before |
| C §4 理论债 | 组合精确性、定义域闭包、Thompson／多样双编译、裁判形式化 | **OUT → C**；A 只保留 unisacc 上已披露的实证与开放义务 |
| 投稿顺序（paper-notes） | A 先独立能投；C 在 A 投出前不升主线 | **keep 顺序纪律**；本拍升格 derive-purify 钉死「C≠闸门」 |

**不切：** 根主张、键数 8509/8769、§8.1 同身份矩阵、Ο1/Ο2、Softguess、T1、unisacc 管线实证句、验证纪律→C 已切项、product/kernel/weights/facts、本拍不改正文。

---

## 为何不挡 A 投稿

1. **DAG 已定：A 是唯一的根。** C 挂回 A；「并行主线」是工作编排语言，不是「三根同捆才能投」。
2. **A 根主张不依赖 C 成文。** 核心是 TSV→构造网络→T1 + unisacc 实证；种子脱离／新场景迁移是**推广方法**，不是根。
3. **与已切 verif→C 正交。** `-2030` 切的是「验证纪律 beyond NN＝已证方法定理」；本拍切的是「整份管道方法包（种子／脱离／≥3 场景／组合闭包形式化）＝A 投稿闸门」。
4. **政委投稿顺序。** A 先独立能投；E 和 C 在 A 投出前不升主线——本拍执行该纪律的登记面。
5. **本拍零 Paper A 正文 diff。** 只 notes + registry。

---

## 父节点如何回 A

- **理论回 A：** 任何未来管道方法文若仍用「表构造 → 声明域精确（T1）」作阶段核，其方法核仍是 A；结果可回来展示迁移条件，不能改写根主张为「须先有 C」。
- **实例上游：** A（与 B）是 C 的迁移实例，不是 C 的投稿人质。
- **DAG：** A（construction + T1；unisacc 管线实证）→（意向）Paper C pipeline-method（undrafted）；C 不与 A 并列成第二根。

---

## 切后 A 哪一句更硬

切出后，A 可把边界收成一句硬边界（**本拍不改正文；仅登记主张**）：

> **Paper A 只主张基于神经网络（构造网络与权重、非训练）的编译器与 T1，并以 unisacc 整条编译管线为实证载体；把该方法推广为一般管道方法（种子机、迭代脱离、组合精确性／定义域闭包、≥3 非编译器场景）属意向 Paper C，挂回 A，现在不起稿，不是 A 投稿前提，也不是「尚未交齐 C 则构造法不完整」的否决条件——A 正文中的管线推广句是 unisacc 实证叙述，不是 C 已成文的证明义务。**

攻击面从「你们还没有一般管道方法论文／三个新场景，方法不完整？」缩回 **「A = 构造 + T1 + unisacc 管线实证；C 另挂、不起稿不挡投」**。

---

## 衍生课题提案（不起稿）

| 项 | 内容 |
| --- | --- |
| **暂定题** | Pipeline Method: Seed Machine, Iterative Detachment, and Migration Conditions (Paper C; hanging on A; undrafted) |
| **一行主张** | Elevating completion of the full Paper C pipeline-method package—seed machine, iterative detachment, domain-closure / combination-exactness formalization, and ≥3 non-compiler migration scenes—into a Paper A submission gate or root-claim veto is an independent future topic hanging on A, not required to submit A. |
| **上游** | Paper A；相邻 B 作第二实例；相邻 E（造表）与 verif-discipline 切片；**不**吞并 A2 |
| **协议指针** | 不起稿不得改 A 键数或 T1；不得把「并行主线」读成同捆闸门；不得与 Softguess／8509-8769／Ο1/Ο2／平台矩阵／verif→C／combo-merge／T3／LLM-outside／wasm／memsafe D 混读；**不重写** paper-c-intent |
| **状态** | **note only** — 无独立 TeX/中英草稿；不改正文 |

---

## 证据（本拍）

- tip before：`bb5795679bf9fb3eb6d41f5631e4fb5a61ef3687`
- Paper A 四 blob SAME：CN `74893e3d…` · EN `95657738…` · TeX `3830ede1…` · abs `43d7ea7e…`
- Softguess NONE×4；Latest 仍 v0.0.39
- 挡粘仍开：平台矩阵 §8.1、8509/8769、Ο1/Ο2
- 机读：`research/paper-c-intent.md` 存在；既有 derive-purify 无「pipeline-method-paper-c／C＝闸门」slug（仅 verif-discipline→C）
