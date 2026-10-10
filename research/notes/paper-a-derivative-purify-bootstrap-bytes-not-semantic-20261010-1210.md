# Paper A · 衍生净化登记：逐字节自举 ≠ 语义自举（BootstrapHost）

- **拍点：** 2026-10-10 ~12:10 Asia/Shanghai（论文心跳）
- **本拍切口：** **E. 衍生净化**
- **仓库 tip（只读核对后）：** `e1a8c5c3`（full `e1a8c5c31d8ee8fdf037e5dbef86a32949c3d947`；= `origin/main`；相对上一拍 tip 同 `e1a8c5c3`，+0 于 Paper A；相对 Paper A 锚 tip `04a5087b`：四 blob 未漂）
- **四 blob（相对 `04a5087b` / 期望）：** CN `000b7ef1` · EN `93fdf052` · TeX `8122353c` · abstract `9e883892` — **全部 SAME**
- **状态：** 登记稿（UNAPPLIED）；**不**改正文 tip；**不**改键数/产品/投稿方向；**不**叠 ~03:44 决策卡

---

## 主张一句

把「逐字节自举 N1=N2=N3 不是语义自举/C99完备/表≡C」从 A 摘要与 §5.3 的防御长句升为挂回 A 的短衍生课题，使 A 只留最短实证句，少堆否定清单，从而更纯粹、更难被「你们自举了所以语义完备」随便挑战。

---

## 切什么（A 中哪段 / 哪类句子）

| 位置 | 现况（tip `e1a8c5c3` / blob 未漂；原文 verbatim） | 切出后去向 |
| --- | --- | --- |
| CN 摘要（`unisacc-paper.md` L13） | 「它不经 Python 在五个目标上实现了逐字节自举（N1 = N2 = N3）；**该判据仅断言未签名连续代产物在具名检查下字节相同，不是语义自举定理，也不是 C99 完备或「表≡C 语义」的证明，表语义仍靠外部裁判与已列边界。**」 | 否定清单迁入本衍生；A 摘要只留最短实证句 + 指针 |
| CN §5.3（同文件 L252） | 「逐字节自举（N1 = N2 = N3）仅断言未签名连续代产物在具名检查下字节相同；不是语义自举定理，也不是 C99 完备或「表≡C 语义」的证明；表语义仍靠外部裁判与已列边界。」 | 整段防御升为本衍生主体；§5.3 保留三层自举事实（L246–250） |
| EN 摘要（`unisacc-paper.en.md` L13） | 「…self-hosts byte-for-byte (N1 = N2 = N3) on five targets without Python; **that criterion asserts only unsigned successive-generation binary identity under the named checks, not a semantic self-hosting theorem, C99 completeness, or a proof that tables ≡ C semantics (table semantics still rest on external referees and the stated boundaries).**」 | 同 CN |
| EN §5.3（同文件 L250） | 「Byte-identical bootstrap (N1 = N2 = N3) asserts only unsigned successive-generation binary identity under the named checks; it is not a semantic self-hosting theorem, not C99 completeness, and not a proof that tables ≡ C semantics (table semantics still rest on external referees and the stated boundaries).」 | 同 CN |
| TeX 摘要（`arxiv-paper-a/main.tex` L76） | 「…self-hosts byte-for-byte (N1 = N2 = N3) on five targets---**that criterion asserts only unsigned successive-generation binary identity under the named checks, not a semantic self-hosting theorem, C99 completeness, or a proof that tables agree with C semantics (table semantics still rest on external referees and the stated boundaries).**」 | 同 CN/EN；三联同改 |
| TeX §Self-hosting（同文件 L565） | 「Byte-identical bootstrap (N1 = N2 = N3) asserts only unsigned successive-generation binary identity under the named checks; it is not a semantic self-hosting theorem, not C99 completeness, and not a proof that tables agree with C semantics (table semantics still rest on external referees and the stated boundaries).」 | 同 §5.3 |
| `abstract.txt`（blob `9e883892`） | 现况**已无**上述否定长句，仅有实证：「…and the compiler self-hosts byte-for-byte on five targets.」 | **本拍不强迫加长**；日后若三联缩锚，可任选加最短指针或不改 |

**明确不切：** Softguess / P0 粘贴；8509/8769 双身份免责；DeployForm（DENSE≠直播推理，已另登记）；A2 构造 vs 训练；定义 3 偏函数一行；§5.3 三层自举事实本身（前端不动点 / 后端闭环 / N1=N2=N3 五目标）——那些是 A 实证，不是否定清单。

**已有 seal（升格来源，非新发明）：** `research/seal-bootstrap-bytes-not-semantic-20261009.md` — 已 pin 同句于摘要 + §5.3（CN/EN/TeX），但尚未写成「可独立开题的衍生课题登记」。本拍补登记，不重复粘 seal，也不改正文。

---

## 为何不挡 A 投稿

1. A 投稿所需的根主张仍是：TSV 表构造网络与权重 + T1；逐字节自举是**实证载体**上的未签名连续代字节相等，**不是**根主张的前提条件，也不是语义保持定理。
2. 切出的是**「自举不是什么」的否定清单与审稿防御**，不是测量身份封口、不是表 4/5、不是平台矩阵——那些仍是 A 硬缺口，本登记不替代。
3. A 保留最短实证句（五目标 N1=N2=N3）后，审稿人仍能在 A 内看到事实；「不是语义自举 / 不是 C99 完备 / 不是表≡C」的展开可后置到衍生，故**不挡** A 先投。
4. 本拍只写登记稿，**默认不改仓内正文**，零风险于 tip。

---

## 父节点如何回 A（DAG 边）

```
A (根：基于神经网络的编译器；构造≠训练；unisacc 实证)
 └─ [本节点暂名] BootstrapHost / 「逐字节自举 ≠ 语义自举」
      边类型：实证边界澄清（非第二套编译器理论）
      默认单父 A；不另起「自举语义学」根
```

- **禁止：** 把本节点写成与 A 并列的「语义自举编译器」根；禁止把 N1=N2=N3 抬成 T3 / 全 C99 / 表≡C。
- **与 DeployForm：** 同为 E 切口衍生净化兄弟节点；DeployForm 钉部署形态，本节点钉自举判据边界；互不吞并。
- **与 A2 / C / D / E(造表)：** 无关或不替代；本节点不讨论训练对照、管道组合、内存安全或造表方法。

---

## 切后 A 更纯在哪

- **更硬的主张句：** A 的自举主张 = **未签名连续代产物在具名检查下字节相同（N1=N2=N3）**，与语义自举定理 / C99 完备 / 表≡C **正交**。
- **少掉的可攻击面：** 审稿人用「你们都自举了，所以语义完备 / 表就是 C」一击把实证字节相等抬成定理——防御否定清单从 A 摘要与 §5.3 主叙事移出后，A 不再用半页「不是什么」冲淡根主张与实证句；攻击须转到衍生短文（那里可系统列：unsigned SHA pin、签名 `.com` 不进不动点、clauses.tsv 覆盖分母、§8 边界、外部裁判）。
- **完备性：** A 实证仍如实写五目标逐字节自举；完备的是**判据语义**，不是隐瞒自举事实。

---

## A 正文建议保留的最小锚句（UNAPPLIED 草案 · 本拍不粘 tip）

**CN（摘要从句 + §5.3，替换现防御长句为）：**  
决策网络实例不经 Python 在五个目标上实现逐字节自举（N1 = N2 = N3）；该判据仅断言未签名连续代产物在具名检查下字节相同（展开见衍生课题 BootstrapHost）。

**EN：**  
The decision-network instance self-hosts byte-for-byte (N1 = N2 = N3) on five targets without Python; that criterion asserts only unsigned successive-generation binary identity under the named checks (details in derivative note BootstrapHost).

**TeX：**  
The decision-network instance self-hosts byte-for-byte (N1 = N2 = N3) on five targets without Python; that criterion asserts only unsigned successive-generation binary identity under the named checks (derivative note BootstrapHost).

> 粘贴条件（非本拍）：政委批「§5.3/摘要缩锚」或自审零风险 research-only PR；须 CN/EN/TeX（及若改 abstract.txt 则四处）三联同改；**禁止**顺手改 8509/8769 或粘 Softguess；**禁止**删 §5.3 三层自举事实。

---

## 与已有 seal / DeployForm / A2 / B / C / D / E 的关系

| 节点 | 关系 |
| --- | --- |
| `seal-bootstrap-bytes-not-semantic-20261009` | **直接升格来源**：seal=正文 pin；本文件=可独立开题登记 |
| DeployForm（20261010-1140） | 同 genre 兄弟衍生；切 DENSE 部署辩护；本拍不切、不叠 |
| A2 | 无关（构造 vs 训练对照）；不抢 A2 |
| B (UJS) | 应用换语言；若 UJS 也有字节自举边界可引用本节点，父仍 A |
| C | 管道方法；本节点不替代 |
| D | 内存安全节点；无关 |
| E (有限控制表系统构造) | 造表方法；本节点是**已造产物的自举判据边界**，不混 |
| `seal-refusal-partial-fn` / 定义 3 | 偏函数/拒绝；**不迁 Softguess**；本拍不切定义 3 |
| `seal-dense-path-not-withdraw-nn` | DeployForm 升格来源；本拍不重复 |

**反第二套理论检查：** 本节点不引入新「编译器是什么」定义；只澄清 A 已有实证自举判据的否定边界。通过。

---

## Softguess / 双身份状态一行

Softguess/权重就是/are the logic/逻辑本身/权重即逻辑 **仍 NONE×4**（四文件未粘）；双身份 8,509/8,769 **免责仍在**；P0 merged + 路线Ι 待批板（~03:44）**仍有效，本拍不叠卡、不催同口号**。

---

## 本拍交付与非交付

**已做：** tip/blob 只读核对；本登记稿；机核 JSON；短报证据。  
**未做：** 不粘正文；不改键数；不开产品 PR；不叠决策卡；不 push。  
**镜像：** `research/notes/` 同名文件；**默认不 commit tip**。
