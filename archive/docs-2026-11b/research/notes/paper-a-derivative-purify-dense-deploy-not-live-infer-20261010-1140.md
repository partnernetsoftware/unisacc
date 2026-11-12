# Paper A · 衍生净化登记：部署形态 ≠ 直播推理（DENSE）

- **拍点：** 2026-10-10 ~11:40 Asia/Shanghai（论文心跳）
- **本拍切口：** **E. 衍生净化**
- **仓库 tip（只读核对后）：** `e1a8c5c3`（相对上一拍 tip 同 `e1a8c5c3`，+0 于 Paper A；相对 Paper A 锚 tip `04a5087b`：四 blob 未漂）
- **四 blob（相对 `04a5087b` / 期望）：** CN `000b7ef1` · EN `93fdf052` · TeX `8122353c` · abstract `9e883892` — **全部 SAME**
- **状态：** 登记稿（UNAPPLIED）；**不**改正文 tip；**不**改键数/产品/投稿方向；**不**叠 ~03:44 决策卡

---

## 主张一句

把「决策网络实例的 DENSE 产品路径 ≠ 每次现场权重推理」从 A 正文的防御段升为挂回 A 的短衍生课题，使 A 只留一句 T1 命名锚，少堆产品辩护，从而更纯粹、更难被「你们只是查表」随便挑战。

---

## 切什么（A 中哪段 / 哪类句子）

| 位置 | 现况（tip `e1a8c5c3` / blob 未漂） | 切出后去向 |
| --- | --- | --- |
| §4.4 / *What is not the model* | `front_parse.c::inf` 读 DENSE 之后的长句：「命名锚定在构造与 T1…DENSE 是部署形态…不收回命名、不削弱 T1」（CN L210 / EN L208 / TeX ~L436） | 迁入本衍生短文主体 |
| 摘要决策网络实例句 | 「命名同样锚定在构造与 T1，而不要求每次产品询问都现场算权重」类从句 | A 摘要只留最短 T1 锚；展开论证进衍生 |
| §3.1 附近 | 「运行时查 DENSE 表是不同部署形态，但 T1 判据不变」类一句 | 可并入衍生「部署谱系」小节；A 留事实一句即可 |
| 表 5 / 速度讨论里的 DENSE 标注 | 实证测量标签（历史） | **不切**：仍属 A 实证载体说明 |

**明确不切：** Softguess / REFUSE 对偶名（仍属待粘 P0）；8509/8769 双身份；定义 3 偏函数一行；A2 构造 vs 训练。

**已有 seal（升格来源，非新发明）：** `research/seal-dense-path-not-withdraw-nn-20261009.md` — 已 pin 同句，但尚未写成「可独立开题的衍生课题登记」。本拍补登记，不重复粘 seal。

---

## 为何不挡 A 投稿

1. A 投稿所需的根主张仍是：TSV 表构造网络与权重 + T1（声明域上网络=表）；DENSE 是同构造的部署形态，**不是**根主张的前提条件。
2. 切出的是**产品辩护与部署谱系讨论**，不是测量身份封口、不是表 4/5 历史降级、不是平台矩阵——那些仍是 A 硬缺口，本登记不替代。
3. A 保留最小锚句后，审稿人仍能在 A 内看到「命名不因 DENSE 撤回」；展开证明与对照实验可后置，故**不挡** A 先投。
4. 本拍只写登记稿，**默认不改仓内正文**，零风险于 tip。

---

## 父节点如何回 A（DAG 边）

```
A (根：基于神经网络的编译器；构造≠训练；unisacc 实证)
 └─ [本节点暂名] DeployForm / 「部署形态 ≠ 直播推理」
      边类型：方法/产品表面澄清（非第二套编译器理论）
      可选第二上游：C（管道方法里「表状决策 vs 结构粘合 / 部署核」）——仅当讨论推广到非编译器管道时；默认单父 A
```

- **禁止：** 把本节点写成与 A 并列的「查表编译器」根；禁止把 DENSE 抬成新定义来替代 T1。
- **与 C：** C 讲管道组合与种子脱离；本节点只钉「同一构造网络的部署形态谱系」。可引用 C，不吞并 C。

---

## 切后 A 更纯在哪

- **更硬的主张句：** 「基于神经网络的编译器」= **构造权 + T1**，与产品 `ask` 是否现场 gemv **正交**。
- **少掉的可攻击面：** 审稿人用「你们产品路径在查 DENSE 表」一击把整篇降成「只是查表、不配叫神经网络编译器」——防御长段从 A 主叙事移出后，A 不再用半页产品辩护冲淡根主张；攻击须转到衍生短文（那里可系统列：验证路径 / 部署核 / DENSE 枚举表 / 七阶段逐步推理，同一 T1）。
- **完备性：** A 实证仍可如实写「决策网络实例读 DENSE」；完备的是**命名语义**，不是隐瞒产品路径。

---

## A 正文建议保留的最小锚句（UNAPPLIED 草案 · 本拍不粘 tip）

**CN（§4.4，替换现防御长句为）：**  
决策网络实例的产品路径可读对构造网络穷举所得的 DENSE 表；「基于神经网络的编译器」命名锚定在构造与 T1，DENSE 仅为同网络的部署形态（展开见衍生课题 DeployForm）。

**EN：**  
The decision-network product path may read the enumerated DENSE table; the neural-network-based naming is anchored on construction and T1, with DENSE only a deployment form of the same network (details in derivative note DeployForm).

**TeX：**  
The decision-network product path may read the enumerated DENSE table; the neural-network-based naming is anchored on construction and T1---DENSE is only a deployment form of the same network (derivative note DeployForm).

> 粘贴条件（非本拍）：政委批「§4.4 缩锚」或自审零风险 research-only PR；须 CN/EN/TeX/abstract 三联同改；**禁止**顺手改 8509/8769 或粘 Softguess。

---

## 与已有 seal / intent / A2 / B / C / D / E 的关系

| 节点 | 关系 |
| --- | --- |
| `seal-dense-path-not-withdraw-nn-20261009` | **直接升格来源**：seal=正文 pin；本文件=可独立开题登记 |
| A2 | 无关（构造 vs 训练对照）；不抢 A2 |
| B (UJS) | 应用换语言；若 UJS 也有「表部署 vs 现场推理」可引用本节点，父仍 A |
| C | 可选第二上游；本节点不替代管道方法 |
| D | 内存安全节点；无关 |
| E (有限控制表系统构造) | 造表方法；本节点是**已造网络的部署形态**，不混 |
| `seal-refusal-partial-fn` | 偏函数/拒绝；**不迁 Softguess**；本拍不切定义 3 |
| 目标参数化 / 跨架构不变式 | 另一候选；本拍不选（实证仍留 A；不变式主张未起稿） |

**反第二套理论检查：** 本节点不引入新「编译器是什么」定义；只澄清 A 已有构造网络的部署谱系。通过。

---

## Softguess / 双身份状态一行

Softguess/权重就是 **仍 NONE**（四文件未粘）；双身份 8509/8769 **免责仍在**；P0 merged + 路线Ι 待批板 **仍有效，本拍不叠卡、不催同口号**。

---

## 本拍交付与非交付

**已做：** tip/blob 只读核对；本登记稿；短报证据。  
**未做：** 不粘正文；不改键数；不开产品 PR；不叠决策卡；不 push。  
**可选镜像：** `research/notes/` 同名文件（若可写）；**默认不 commit tip**。
