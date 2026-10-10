# Paper A 衍生净化：管道外大模型协同进化／自进化工作流不得搭乘 A

- **拍点：** 2026-10-10 ~22:42 Asia/Shanghai（UTC+8）
- **切口：** **E. 衍生净化**
- **仓库 tip（只读核对 / before）：** `249d2f3d7a9d2af6d7ddb5a3d414e5cbf4e9de1d`（PR#67 wasm 合入后）
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1＝网络≡表；字节流管道实证）＋未来 **稳定后的 unisa+unisacc** → 未来 **管道外大模型协同进化／自进化工作流** 旁支 → 最终仍回 **A**
- **状态：** **note only**（登记未来独立课题；**不起稿正文**；**不改** A 根主张、键数身份、CN/EN/TeX/abstract、product/kernel/weights/facts、正在封的 0.0.x；**不重写** `examples/cx-lab/`；**不把训练写回**构造编译器权重）
- **Softguess：** tip 四文件 **NONE×4**（本拍机扫：`Softguess` / `权重就是` / `weights are the logic` / `逻辑本身` / `权重即逻辑` 均 0 hit）
- **测量身份：** 8509/8769 **仍开** — 仅引用；本拍**不重钉、不叠** Ο1/Ο2／~17:08 决策卡
- **相邻已封／已切（本拍不重做）：**
  - 今晚 E：wasm 应用（~22:31／PR#67）、formal-verif/T3（~22:12／PR#66）、optim volume/speed（~21:53／PR#65）、memsafe D（~21:40／PR#64）
  - A2＝构造 vs 训练对照（预注册；SGD 只作对照，产品仍只用构造）
  - verif-discipline → C（~20:30）：经验验证纪律推广 ≠ 管道外 LLM 协同进化
  - cx-lab：站在编译器**外面**收表现有二进制已印文本；是「外面出表、不改核心」的雏形，**不是**本课题正文，也**不是** A 投稿前提
  - 政委 2026-10-09 远期意向（记忆／公司笔记）：unisa+unisacc 稳定后，大模型在管道**外面**产候选，构造网络确定性检查／拒绝；**不**把训练写回构造权重；**不进** Paper A，**不写进**正在封的 0.0.x

---

## 主张一句（本页唯一）

**把「必须先完成管道外大模型协同进化／自进化工作流（LLM 在管道外产候选、构造网络确定性检查／拒绝；或把 cx-lab 外面出表读成 A 已承诺自进化／LLM 共训闭环）才算 Paper A 可投／构造法才成立」——或把「尚未起稿的协同进化／把训练写回构造权重」升成否决 A 根主张／构造≠训练边界的条件——不得搭乘 A。A 只需构造 + T1 + unisacc 实证；该课题属未来衍生，挂回 A（字节流管道）与稳定后的 unisa+unisacc，现在不起稿，不进正在封的 0.0.x，也不改正文。**

---

## 切什么（从 A 的主张负担里切出）

切出对象是 **把「管道外 LLM 协同进化／自进化工作流」升成投稿闸门或根主张否决条件**，以及把远期意向／cx-lab 雏形读成 A 已承担交齐「自进化／LLM 共训」面：

| 位点 | 摘句／现状要点 | 分类 |
| --- | --- | --- |
| 政委 2026-10-09 远期意向 | unisa+unisacc 稳定后，LLM 管道外产候选；构造网检查／拒绝；不写回权重；不进 A／0.0.x | **keep 远期意向**；本拍升格 derive-purify：**不得**升成 seal 前提 |
| cx-lab | 外面出表、不改 kernel/weights/facts | **keep 实验室雏形**；切出「cx-lab＝自进化论文已起步＝A 完备义务」 |
| A 根主张 | 构造网络与权重，**非训练**；T1；unisacc 实证 | **keep in A** |
| A2 | 构造 vs 训练对照；SGD 只对照 | **keep in A2**；切出「协同进化＝把训练写回构造权重／第二套学习编译器」 |
| 产品分层 unisa／tinyvm／ujs | 产品进化路线 | **keep 产品意向**；切出「论文须先交齐 LLM 协同闭环才封口 A」 |

**不切：** 根主张、键数 8509/8769、§8.1 同身份矩阵、Ο1/Ο2、Softguess、T1、A2 预注册、cx-lab 实验代码、product/kernel/weights/facts、本拍不改正文。

---

## 为何不挡 A 投稿

1. **政委已明令延期。** 2026-10-09：该题不进 Paper A，也不写进正在封的 0.0.x；本拍只登记「不得搭乘」。
2. **A 根主张不依赖自进化闭环。** 核心是 TSV 表 → 构造网络 → T1 + unisacc 实证；管道外 LLM 是**以后**的工作流，不是根。
3. **构造≠训练必须保持硬。** 大模型站在管道**外面**；构造网络只做确定性检查／拒绝；**禁止**把训练／梯度写回构造编译器权重——这正是 A 与「学出编译器」故事的分界，切出后更不易被混读。
4. **cx-lab ≠ 本课题正文。** cx 是外面收表的雏形，服务测量／实验，不是 A 的自进化主张，也不挡投。
5. **与今晚已切项正交。** wasm／T3／optim／memsafe 切的是应用／形式化／优化／管道节点；本拍切的是**管道外协同进化工作流**。
6. **本拍零 Paper A 正文 diff。** 只 notes + registry。

---

## 父节点如何回 A

- **理论回 A：** 任何未来协同进化若仍用「表构造 → 声明域精确（T1）」作检查核，其方法核仍是 A 的字节流管道；结果可回来展示「外面产候选、里面确定性拒绝」，不能改写根主张为「须先有自进化／LLM 共训」。
- **产品上游：** 稳定后的 **unisa+unisacc** 是工作流载体，不是第二套编译器理论。
- **旁支挂回：** 管道外 LLM 协同进化挂 **A**（经字节流管道）＋稳定 unisa+unisacc；**不是**与 A 并列的第二根，也**不是**把训练写回权重的学习编译器论文。
- **DAG：** A（construction + T1；管道实证）→（未来，稳定后）LLM-outside co-evolution（undrafted）；A2 仍只管构造 vs 训练对照，不吞并本旁支。

---

## 切后 A 哪一句更硬

切出后，A 可把边界收成一句硬边界（**本拍不改正文；仅登记主张**）：

> **Paper A 只主张基于神经网络（构造网络与权重、非训练）的编译器与 T1，并以 unisacc 为实证载体；管道外大模型协同进化／自进化工作流（LLM 产候选、构造网检查／拒绝、不写回权重）属未来衍生，挂回 A 与稳定后的 unisa+unisacc，现在不起稿、不进正在封的 0.0.x，不是 A 投稿前提，也不是「尚未交齐自进化／LLM 共训则构造法不成立」的否决条件——cx-lab 外面出表亦不强制这支文。**

攻击面从「你们没有自进化／和 LLM 共训，方法不完整？」缩回 **「A = 构造 + T1 + 实证；协同进化另挂、不起稿不挡投；构造≠训练仍硬」**。

---

## 衍生课题提案（不起稿）

| 项 | 内容 |
| --- | --- |
| **暂定题** | Outside-Pipeline LLM Co-Evolution with Constructed Checker Networks (hanging on Paper A; undrafted) |
| **一行主张** | Elevating a deferred outside-pipeline LLM co-evolution / self-evolution workflow—LLM proposes candidates; constructed networks deterministically check/refuse; no training write-back into constructed compiler weights; cx-lab external-table as precursor—into a Paper A submission gate or root-claim veto is an independent future topic hanging on A (byte-stream pipeline) plus stable unisa+unisacc, not required to submit A. |
| **上游** | Paper A（字节流管道）；未来稳定 unisa+unisacc；相邻 A2（构造 vs 训练对照，不吞并）；cx-lab 雏形 |
| **协议指针** | 不起稿不得改 A 键数或 T1；不得把训练写回构造权重；不得把 cx-lab／产品分层偷偷升成 seal-before；不得与 Softguess／8509-8769／Ο1/Ο2／平台矩阵／wasm／T3／optim／memsafe D／verif→C 混读；不得写成第二套「学出的编译器」理论；**不重写** cx-lab |
| **状态** | **note only** — 无独立 TeX/中英草稿；不改正文；不进 0.0.x |

---

## 证据（本拍）

- tip before：`249d2f3d7a9d2af6d7ddb5a3d414e5cbf4e9de1d`
- Paper A 四 blob SAME：CN `74893e3d…` · EN `95657738…` · TeX `3830ede1…` · abs `43d7ea7e…`
- Softguess NONE×4；Latest 仍 v0.0.39
- 挡粘仍开：平台矩阵 §8.1、8509/8769、Ο1/Ο2
