# Paper A 成篇定稿增量：P-3「全域枚举」／Full-domain enumeration 钉为 T1／非训练精度三联对齐

- **拍点：** 2026-10-12 ~13:01 Asia/Singapore（UTC+8）
- **切口：** **A. 成篇定稿增量**
- **仓库 tip（before）：** `0795db950c15cf79e303bab16394ae96b9b2c7eb`（短 `0795db95`；= #256 env-pin on main；paper 父 `#261/#262` tip `6467ab6c` 之后）
- **tip after：** `TIP_AFTER_PLACEHOLDER`（merge 后由 follow-up 填）
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1＝网络≡表于声明域 \(K_s\)；unisacc 为 POSIX C99 子集跨架构实证）
- **状态：** research-only CN+EN+TeX §3.4 P-3 判据列「全域枚举」／「Full-domain enumeration」同义收紧为 T1／定义 2；**不改** abstract.txt（无 P-3 短语）；**不改**键数身份、表 1／表 3／§6.1／Def2／附录 CLI／摘要已钉句、product/kernel/weights/facts、投稿方向；**不叠** Ο1；**不重钉** Softguess／§8.1#3／8509·8769
- **Softguess：** tip 四文件 **NONE×4**（机扫 `Softguess`/`softguess`/`soft-guess`/`Softmax-as-product`／「权重就是」／“weights are the logic”／「逻辑本身」／「权重即逻辑」／「权重≡逻辑」均 0 hit；§3「不含 softmax」／「no softmax」否认句不计；**未**粘 Softguess 口号）
- **Latest：** **v0.0.40**；冻结重测身份仍 **v0.0.19**；tip version **0.0.41**（本拍不 bump）
- **同族链（本拍续）：** 0956 → 1011 §7.4 → 1026 §10 → 1046 abstract → 1056 contrib4 → 1109 contrib1 Method → 1130 Def2 exactness → 1141 精度 1.000＝T1 → 1156 表 1 表注精度＝T1 → 1209 表 3 阶段精度判据＝T1 → 1225 §6.1 全域枚举开篇＝T1 → 1243 附录 CLI 全域精度＝T1 → **本拍 P-3 判据「全域枚举」／Full-domain enumeration＝T1 非训练精度**

---

## 主张一句（本页唯一）

**表 1 表注、表 3、§6.1、附录 CLI 已钉「全域枚举／精度」为 T1／定义 2（#254／#257／#259／#261）之后，§3.4 形式化义务表 P-3 判据列仍只写「对出货整数网络做全域枚举」／「Full-domain enumeration of the released integer network」，可软读为样本拟合或训练准确率；本拍在保留 `decision_list_exact` 尾句的同时，括注其为声明域穷举逐键相等（T1／模型检验／定义 2），并排除相对样本的拟合精度或训练精度；CN/EN/TeX 三联；abs SAME；不改键数、不引入 Softguess 口号字面。**

---

## Before / after 句段（精确）

### CN `research/unisacc-paper.md` §3.4 P-3

- **Before：** `| **P-3** | 发布网络 ≡ 真值表 | 对出货整数网络做全域枚举；\`decision_list_exact\` 仅证明全局单色的首次命中子情形 |`
- **After：** `| **P-3** | 发布网络 ≡ 真值表 | 对出货整数网络做全域枚举（声明域穷举逐键相等，即 T1／模型检验／定义 2，不是相对样本的拟合精度或训练精度）；\`decision_list_exact\` 仅证明全局单色的首次命中子情形 |`

### EN `research/unisacc-paper.en.md` §3.4 P-3

- **Before：** `| **P-3** | Released network ≡ truth table | Full-domain enumeration of the released integer network; \`decision_list_exact\` proves only the globally monochrome first-hit subcase |`
- **After：** `| **P-3** | Released network ≡ truth table | Full-domain enumeration of the released integer network (= T1 / exhaustive key-by-key equality on the declared domain / Definition 2, not sample fit or training accuracy); \`decision_list_exact\` proves only the globally monochrome first-hit subcase |`

### TeX `research/arxiv-paper-a/main.tex` P-3 row

- **Before：** `\textbf{P-3} & Released network ≡ truth table & Full-domain enumeration of the released integer network; \texttt{decision\_list\_exact} proves only the globally monochrome first-hit subcase \\`
- **After：** `\textbf{P-3} & Released network ≡ truth table & Full-domain enumeration of the released integer network (= T1 / exhaustive key-by-key equality on the declared domain / Definition~2, not sample fit or training accuracy); \texttt{decision\_list\_exact} proves only the globally monochrome first-hit subcase \\`

### abstract `research/arxiv-paper-a/abstract.txt`

- **本拍不改**（无 P-3／「全域枚举」义务表短语；摘要「all exact」已于 #252 钉 T1）。

---

## 为何更难挑战

| 攻击读法 | Before 软点 | After 挡法 |
| --- | --- | --- |
| P-3「全域枚举」＝样本准确率／训练 accuracy | 判据列未钉 T1≠训练精度 | 括注「声明域穷举逐键相等（T1／模型检验／定义 2）」／「= T1 / … Definition 2」 |
| 形式化义务表「枚举」族可软读为 ML metric | 表 1／表 3／§6.1／附录已钉，P-3 未钉 | 在 P-3 判据入口显式排除「相对样本的拟合精度或训练精度」 |
| Softguess／破平近似合法出货 | 不涉及 | **未**粘 Softguess 口号；Softguess NONE×4 保持 |

---

## tip / blob 表

| 项 | before (`0795db95`) | after（本拍；merge 后填 tip） |
| --- | --- | --- |
| tip | `0795db950c15cf79e303bab16394ae96b9b2c7eb` | `TIP_AFTER_PLACEHOLDER` |
| CN `research/unisacc-paper.md` | `4f5da1c5af011500fd27542155901b81c5778380` | **CHANGED** → `eda0b09521c35220014d7333357929779d041673` |
| EN `research/unisacc-paper.en.md` | `eb01072afc097b3dcbd54120c72cb284c13937ad` | **CHANGED** → `61e7999a213837b0e3a6424b39ccbd728c637101` |
| TeX `research/arxiv-paper-a/main.tex` | `6df0c6a4e23e4b1e80551b3c047377c02e7190c7` | **CHANGED** → `7d315a722d493b018cb6764386f2e9e277f6e91a` |
| abs `research/arxiv-paper-a/abstract.txt` | `980f670df7734b49a5d90bb3c305cd2b565d0b8b` | **SAME**（未改；`980f670df7734b49a5d90bb3c305cd2b565d0b8b`） |
| Softguess | **NONE×4** | **NONE×4** |

---

## Softguess 机扫 @ 2026-10-12 13:01 Asia/Singapore（工作区，merge 前 tip 四文件 + 改后）

| 文件 | Softguess | softguess | soft-guess | Softmax-as-product | 权重就是 | weights are the logic | 逻辑本身 | 权重即逻辑 | 权重≡逻辑 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| CN | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| EN | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| TeX | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| abstract | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |

- **Softguess_NONE_x4 = true**

---

## 挡粘点（仍开，本拍未动）

- §8.1#3 同身份平台矩阵
- 8509/8769 双身份
- Ο1（荐冻结 v0.0.19 vs Latest）— **不叠新决策卡**；仍荐 Ο1
