# Paper A 成篇定稿增量：贡献列表 #1「方法」对齐「T1／模型检验、非样本拟合」三联对齐

- **拍点：** 2026-10-12 ~11:09 Asia/Singapore（UTC+8）
- **切口：** **A. 成篇定稿增量**
- **仓库 tip（before）：** `545cbf022dd17a4aba5ca2bdbd2ed57c9c121c03`（短 `545cbf02`；= #247 after A #246 contrib4；相对任务书 tip `545cbf02` **无漂**）
- **tip after：** PENDING_MERGE（短 PENDING；= squash Merge 本 A；follow-up 填 SHA）
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1＝网络≡表于声明域 \(K_s\)；unisacc 为 POSIX C99 子集跨架构实证）
- **状态：** research-only CN+EN+TeX 贡献列表第 1 条同义收紧；**不改** abstract（贡献列表不在摘要；abs 已有 constructed/not-trained）；**不改**键数身份、product/kernel/weights/facts、投稿方向；**不叠** Ο1/Ο2（仍荐 Ο1）；**不重钉** Softguess／§8.1#3／8509·8769
- **Softguess：** tip 四文件 **NONE×4**（机扫 `Softguess`/`softguess`/`soft-guess`/`Softmax-as-product`／「权重就是」／“weights are the logic”／「逻辑本身」／「权重即逻辑」／「权重≡逻辑」均 0 hit；§3「不含 softmax」／「no softmax」否认句不计；**未**粘 Softguess 口号）
- **Latest：** **v0.0.40**；冻结重测身份仍 **v0.0.19**；tip version **0.0.41**（本拍不 bump）
- **同族链（本拍续）：** 0956 exact-executable-form → 1011 §7.4 → 1026 §10 → 1046 abstract ¶3 → 1056 contrib4 → **本拍贡献 #1 Method**

---

## 主张一句（本页唯一）

**贡献列表第 1 条「方法」在摘要/§7.4/§10/贡献 #4 已钉「不是训练的替代或逼近」之后，仍只写「全定义域枚举下的逐键相等」而未显式排除样本拟合／训练精度读法；本拍把发布判据钉成声明域上穷举判定的逐键相等（T1／模型检验），并括注权重非学得的统计替代；堵住贡献表 Method 单独攻击面；不引入 Softguess 口号字面；不改键数。**

---

## Before / after 句段（精确）

### CN `research/unisacc-paper.md` §1 贡献 #1

- **Before：** `1. **方法。** 一种“TSV 表 DSL → 网络构造 → 枚举”的方法：决策被表示为有限定义域上的全函数；权重由表构造而非训练；发布判据是全定义域枚举下的逐键相等（§2、§3）。`
- **After：** `1. **方法。** 一种“TSV 表 DSL → 网络构造 → 枚举”的方法：决策被表示为有限定义域上的全函数；权重由表构造而非训练（不是学得的统计替代）；发布判据是声明域上穷举判定的逐键相等（T1／模型检验），不是相对样本的拟合精度或训练精度（§2、§3）。`

### EN `research/unisacc-paper.en.md` §1 Contributions #1

- **Before：** `1. **Method.** A "TSV table DSL → network construction → enumeration" method: decisions are total functions on finite domains; weights are constructed from tables rather than trained; the release criterion is key-by-key equality under full-domain enumeration (§2, §3).`
- **After：** `1. **Method.** A "TSV table DSL → network construction → enumeration" method: decisions are total functions on finite domains; weights are constructed from tables rather than trained (not a learned statistical surrogate); the release criterion is key-by-key equality decided by exhaustion on the declared domain (T1 / model checking), not sample fit or training accuracy (§2, §3).`

### TeX `research/arxiv-paper-a/main.tex` Contributions Method

- **Before：** `\textbf{Method.} A ``TSV table DSL → network construction → enumeration'' method: decisions are total functions on finite domains; weights are constructed from tables rather than trained; the release criterion is key-by-key equality under full-domain enumeration (§2, §3).`
- **After：** `\textbf{Method.} A ``TSV table DSL → network construction → enumeration'' method: decisions are total functions on finite domains; weights are constructed from tables rather than trained (not a learned statistical surrogate); the release criterion is key-by-key equality decided by exhaustion on the declared domain (T1 / model checking), not sample fit or training accuracy (§2, §3).`

### abstract `research/arxiv-paper-a/abstract.txt`

- **本拍不改**（贡献列表不在摘要；摘要已有 constructed/not-trained 语言）。

### keywords / related work

- **本拍扫描后无改**：关键词无「样本拟合／训练精度」软点；相关工作对照仍正确，不叠改。

---

## 为何更难挑战

| 攻击读法 | Before 软点 | After 挡法 |
| --- | --- | --- |
| 只读贡献表 Method，未见 T1 定义 | 「全定义域枚举下的逐键相等」可被读成样本/训练集上的精度 | 显式「不是相对样本的拟合精度或训练精度」／「not sample fit or training accuracy」 |
| 权重≈学得的统计替代 | 「由表构造而非训练」未点明非 surrogate | 括注「不是学得的统计替代」／「not a learned statistical surrogate」 |
| Softguess／破平近似合法出货 | 不涉及；义务仍在 Def2/3·Alg1.8·§3 | **未**粘 Softguess 口号；Softguess NONE×4 保持 |

---

## tip / blob 表

| 项 | before (`545cbf02`) | after（本拍；PENDING_MERGE） |
| --- | --- | --- |
| tip | `545cbf022dd17a4aba5ca2bdbd2ed57c9c121c03` | PENDING_MERGE（短 PENDING；follow-up 填） |
| CN `research/unisacc-paper.md` | `3a43650073845aba1595b9e98f51b72b5e6de2eb` | **CHANGED** → `7e02f4656b5215aa19be0b272f24d4489465eec7` |
| EN `research/unisacc-paper.en.md` | `d6c34f879680eb6f47e73b4a65e9cea9b6674093` | **CHANGED** → `69ab3777c9c5553105f5228996ec7f0c236fd83b` |
| TeX `research/arxiv-paper-a/main.tex` | `4a7cfc1981ceb99205b7805dde70f570bd5dc8ad` | **CHANGED** → `7749c4e1cd4287f636fe7282863e898af6ef8e20` |
| abs `research/arxiv-paper-a/abstract.txt` | `980f670df7734b49a5d90bb3c305cd2b565d0b8b` | **SAME**（未改） |
| Softguess | **NONE×4** | **NONE×4** |

---

## Softguess 机扫 @ 2026-10-12 11:09 Asia/Singapore（工作区，merge 前 tip 四文件 + 改后）

| 文件 | Softguess | softguess | soft-guess | Softmax-as-product | 权重就是 | weights are the logic | 逻辑本身 | 权重即逻辑 | 权重≡逻辑 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| CN | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| EN | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| TeX | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| abstract | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |

- **Softguess_NONE_x4 = true**
- **weights_slogan_NONE_x4 = true**

---

## 扫描范围（本拍 Cut A 覆盖后）

已扫贡献列表 #1 Method：发布判据样本拟合／训练精度读法为本拍唯一剩余软点；贡献 #4／摘要／§7.4／§10 同族已由 1056 及前拍钉住。若后续再有同族句则转 E 切闸门，不叠改根主张。

---

## 不叠 / 不碰

- §8.1#3 同身份平台矩阵、8509/8769 双身份、Ο1（荐冻结 v0.0.19 vs Latest）— **仍开；本拍不叠 Ο1 卡、不重钉**
- 键数、product code、投稿方向、根主张 — **不改**
