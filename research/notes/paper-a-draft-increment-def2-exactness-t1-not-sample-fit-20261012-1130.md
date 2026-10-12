# Paper A 成篇定稿增量：定义 2（精确）对齐「T1／模型检验、非样本拟合」三联对齐

- **拍点：** 2026-10-12 ~11:30 Asia/Singapore（UTC+8）
- **切口：** **A. 成篇定稿增量**
- **仓库 tip（before）：** `50dfff99b0a82ac1a9b6195edb5fb1cc388d9e7c`（短 `50dfff99`；= #249 tip-after fill for #248 contrib1 Method；相对任务书 tip `50dfff99` **无漂**）
- **tip after：** `PENDING_MERGE`（squash Merge 本拍 A PR 后由 notes-only follow-up 填入）
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1＝网络≡表于声明域 \(K_s\)；unisacc 为 POSIX C99 子集跨架构实证）
- **状态：** research-only CN+EN+TeX **定义 2（精确）** 同义收紧；**不改** abstract（定义 2 字面不在摘要）；**不改**键数身份、product/kernel/weights/facts、投稿方向；**不叠** Ο1（仍荐冻结 v0.0.19 vs Latest）；**不重钉** Softguess／§8.1#3／8509·8769；**不改**定义 3／T1 表行／贡献列表
- **Softguess：** tip 四文件 **NONE×4**（机扫 `Softguess`/`softguess`/`soft-guess`/`Softmax-as-product`／「权重就是」／“weights are the logic”／「逻辑本身」／「权重即逻辑」／「权重≡逻辑」均 0 hit；§3「不含 softmax」／「no softmax」否认句不计；**未**粘 Softguess 口号）
- **Latest：** **v0.0.40**；冻结重测身份仍 **v0.0.19**；tip version **0.0.41**（本拍不 bump）
- **同族链（本拍续）：** 0956 → 1011 §7.4 → 1026 §10 → 1046 abstract → 1056 contrib4 → 1109 contrib1 Method → **本拍 Def2 exactness**

---

## 主张一句（本页唯一）

**定义 2（精确）在贡献 #1 Method 已钉「发布判据＝T1／模型检验」之后，仍只写「∀k∈K_s 唯一 argmax＝G_s(k)」而未显式排除样本拟合／训练精度读法；本拍把精确性钉成声明域上穷举判定的逐键相等（T1／模型检验），并显式排除相对样本的拟合精度或训练精度；堵住 Def2 单独攻击面；不引入 Softguess 口号字面；不改键数、不改定义 3／T1 表行。**

---

## Before / after 句段（精确）

### CN `research/unisacc-paper.md` §2.1 定义 2

- **Before：** `**定义 2（精确）。** 网络 $N$ 对阶段 $s$ 精确，当且仅当对每个 $k\in K_s$，$\operatorname{argmax}N(k)$ 唯一且等于 $G_s(k)$。`
- **After：** `**定义 2（精确）。** 网络 $N$ 对阶段 $s$ 精确，当且仅当对每个 $k\in K_s$，$\operatorname{argmax}N(k)$ 唯一且等于 $G_s(k)$。该判据是声明域上穷举判定的逐键相等（T1／模型检验），不是相对样本的拟合精度或训练精度。`

### EN `research/unisacc-paper.en.md` §2.1 Definition 2

- **Before：** `**Definition 2 (exactness).** A network $N$ is exact for stage $s$ iff for every $k\in K_s$, $\operatorname{argmax}N(k)$ is unique and equals $G_s(k)$.`
- **After：** `**Definition 2 (exactness).** A network $N$ is exact for stage $s$ iff for every $k\in K_s$, $\operatorname{argmax}N(k)$ is unique and equals $G_s(k)$. This criterion is key-by-key equality decided by exhaustion on the declared domain (T1 / model checking), not sample fit or training accuracy.`

### TeX `research/arxiv-paper-a/main.tex` Definition 2 (exactness)

- **Before：** `\textbf{Definition 2 (exactness).} A network \(N\) is exact for stage \(s\) iff for every \(k\in K_s\), \(\operatorname{argmax}N(k)\) is unique and equals \(G_s(k)\).`
- **After：** `\textbf{Definition 2 (exactness).} A network \(N\) is exact for stage \(s\) iff for every \(k\in K_s\), \(\operatorname{argmax}N(k)\) is unique and equals \(G_s(k)\). This criterion is key-by-key equality decided by exhaustion on the declared domain (T1 / model checking), not sample fit or training accuracy.`

### abstract `research/arxiv-paper-a/abstract.txt`

- **本拍不改**（定义 2 字面不在摘要；摘要已有 constructed/not-trained 与穷举／T1 相关语言）。

### 邻近句（可选 follow-through）

- **本拍不改**：定义 2 下立即接定义 3（询问纪律）与「精确性可以被完全判定，无需任何统计论证」——已含穷举／非统计读法，不软读为「accuracy without T1」；不扩改定义 3／T1 表／贡献列表。

---

## 为何更难挑战

| 攻击读法 | Before 软点 | After 挡法 |
| --- | --- | --- |
| 只读 Def2，未见 T1 表／贡献 #1 | 「∀k argmax＝G」可被读成样本/训练集上的精度或训练准确率 | 显式「不是相对样本的拟合精度或训练精度」／「not sample fit or training accuracy」 |
| 精确＝拟合度量 | 未点名 T1／模型检验 | 括注「声明域上穷举判定的逐键相等（T1／模型检验）」／「T1 / model checking」 |
| Softguess／破平近似合法出货 | 不涉及；义务仍在 Def2/3·Alg1.8·§3 | **未**粘 Softguess 口号；Softguess NONE×4 保持 |

---

## tip / blob 表

| 项 | before (`50dfff99`) | after（本拍；PENDING_MERGE） |
| --- | --- | --- |
| tip | `50dfff99b0a82ac1a9b6195edb5fb1cc388d9e7c` | `PENDING_MERGE` |
| CN `research/unisacc-paper.md` | `7e02f4656b5215aa19be0b272f24d4489465eec7` | **CHANGED** → `482661ec2954ae995a9841145c2b44db2cac95f3` |
| EN `research/unisacc-paper.en.md` | `69ab3777c9c5553105f5228996ec7f0c236fd83b` | **CHANGED** → `a7761815fffff16ffa31e1837d68ec46bcf5b396` |
| TeX `research/arxiv-paper-a/main.tex` | `7749c4e1cd4287f636fe7282863e898af6ef8e20` | **CHANGED** → `9ee7cd263691f961538737885fe0cd9332e6e5c1` |
| abs `research/arxiv-paper-a/abstract.txt` | `980f670df7734b49a5d90bb3c305cd2b565d0b8b` | **SAME**（未改） |
| Softguess | **NONE×4** | **NONE×4** |

---

## Softguess 机扫 @ 2026-10-12 11:30 Asia/Singapore（工作区，merge 前 tip 四文件 + 改后）

| 文件 | Softguess | softguess | soft-guess | Softmax-as-product | 权重就是 | weights are the logic | 逻辑本身 | 权重即逻辑 | 权重≡逻辑 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| CN | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| EN | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| TeX | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| abstract | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |

- **Softguess_NONE_x4 = true**
