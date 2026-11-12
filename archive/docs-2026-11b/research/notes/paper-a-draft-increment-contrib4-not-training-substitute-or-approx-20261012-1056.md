# Paper A 成篇定稿增量：贡献列表 #4「验证纪律」对齐「不是训练的替代或逼近」三联对齐

- **拍点：** 2026-10-12 ~10:56 Asia/Singapore（UTC+8）
- **切口：** **A. 成篇定稿增量**
- **仓库 tip（before）：** `e090ebf708b7065e4d07f02928db02589f289603`（短 `e090ebf7`；= #245 after A #244 abstract-not-training-substitute；相对任务书 tip `e090ebf7` **无漂**）
- **tip after：** `4d452126836c7cee103ae4b1849c876b08c65528`（短 `4d452126`；= squash Merge #246 本 A）
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1＝网络≡表于声明域 \(K_s\)；unisacc 为 POSIX C99 子集跨架构实证）
- **状态：** research-only CN+EN+TeX 贡献列表第 4 条同义收紧；**不改** abstract（贡献列表不在摘要）；**不改**键数身份、product/kernel/weights/facts、投稿方向；**不叠** Ο1/Ο2（仍荐 Ο1）；**不重钉** Softguess／§8.1#3／8509·8769
- **Softguess：** tip 四文件 **NONE×4**（机扫 `Softguess`/`softguess`/`soft-guess`/`Softmax-as-product`／「权重就是」／“weights are the logic”／「逻辑本身」／「权重即逻辑」／「权重≡逻辑」均 0 hit；§3「不含 softmax」／「no softmax」否认句不计；**未**粘 Softguess 口号）
- **Latest：** **v0.0.40**；冻结重测身份仍 **v0.0.19**；tip version **0.0.41**（本拍不 bump）
- **同族链（本拍续）：** 0956 exact-executable-form → 1011 §7.4 → 1026 §10 → 1046 abstract ¶3 → **本拍贡献 #4**

---

## 主张一句（本页唯一）

**贡献列表第 4 条在摘要/§7.4/§10 已收紧「不是训练的替代或逼近、与训练不同类的验证纪律」之后，仍只写「验证纪律与经验／A verification discipline」而未带反替代括注，可被单独读成「枚举+差分=训练的功能替代验证」；本拍把同一实质句补进 CN/EN/TeX 贡献 #4，堵住贡献表单独攻击面；不引入 Softguess 口号字面；不改键数。**

---

## Before / after 句段（精确）

### CN `research/unisacc-paper.md` §1 贡献 #4

- **Before：** `4. **验证纪律与经验。** 穷举枚举、逐字节差分与外部裁判的分层验证，以及把枚举用于手写纯函数时发现的、抽样无法发现的缺陷（§6、§7.4）。`
- **After：** `4. **验证纪律与经验。** 构造—枚举不是训练的替代品或逼近，而是一种与训练不同类的验证纪律（权重由表构造而非训练）；穷举枚举、逐字节差分与外部裁判的分层验证，以及把枚举用于手写纯函数时发现的、抽样无法发现的缺陷（§6、§7.4）。`

### EN `research/unisacc-paper.en.md` §1 Contributions #4

- **Before：** `4. **A verification discipline and its findings.** Layered verification by exhaustive enumeration, byte-level differential testing and external referees, including defects in hand-written code that enumeration found and sampling could not (§6, §7.4).`
- **After：** `4. **A verification discipline and its findings.** Construction plus enumeration is not a substitute for—or approximation of—training, but a distinct verification discipline (weights constructed from tables, not trained); layered verification by exhaustive enumeration, byte-level differential testing and external referees, including defects in hand-written code that enumeration found and sampling could not (§6, §7.4).`

### TeX `research/arxiv-paper-a/main.tex` Contributions #4

- **Before：** `\textbf{A verification discipline and its findings.} Layered verification by exhaustive enumeration, byte-level differential testing and external referees, including defects in hand-written code that enumeration found and sampling could not (§6, §7.4).`
- **After：** `\textbf{A verification discipline and its findings.} Construction plus enumeration is not a substitute for---or approximation of---training, but a distinct verification discipline (weights constructed from tables, not trained); layered verification by exhaustive enumeration, byte-level differential testing and external referees, including defects in hand-written code that enumeration found and sampling could not (§6, §7.4).`

### abstract `research/arxiv-paper-a/abstract.txt`

- **本拍不改**（贡献列表不在摘要；摘要 ¶3 已由 #244 对齐同一实质）。

### keywords / related work

- **本拍扫描后无改**：关键词无「替代/逼近训练」软点；相关工作「从显式程序构造权重，而非训练」已是正确对照，不叠改。

---

## 为何更难挑战

| 攻击读法 | Before 软点 | After 挡法 |
| --- | --- | --- |
| 只读贡献表，未见摘要/§7.4/§10 | 「验证纪律」可被读成训练的功能替代（用枚举交差） | 显式「不是…替代品或逼近」／「not a substitute for—or approximation of—training」 |
| 构造≈另一种训练／逼近训练 | 贡献 #4 未锚定 construction≠training | 「与训练不同类」+ 括注「权重由表构造而非训练」 |
| Softguess／破平近似合法出货 | 不涉及；义务仍在 Def2/3·Alg1.8·§3 | **未**粘 Softguess 口号；Softguess NONE×4 保持 |

---

## tip / blob 表

| 项 | before (`e090ebf7`) | after（本拍；#246 `4d452126`） |
| --- | --- | --- |
| tip | `e090ebf708b7065e4d07f02928db02589f289603` | `4d452126836c7cee103ae4b1849c876b08c65528`（短 `4d452126`；#246） |
| CN `research/unisacc-paper.md` | `dd22dc1c4dde5e0abd8da7f3b72316da4eed255d` | **CHANGED** → `3a43650073845aba1595b9e98f51b72b5e6de2eb` |
| EN `research/unisacc-paper.en.md` | `b10c1fc8e8e74bc0c2f36000b1cf5b1351982845` | **CHANGED** → `d6c34f879680eb6f47e73b4a65e9cea9b6674093` |
| TeX `research/arxiv-paper-a/main.tex` | `33b376437ea541c8a99f3b5e6787d8638cdb0156` | **CHANGED** → `4a7cfc1981ceb99205b7805dde70f570bd5dc8ad` |
| abs `research/arxiv-paper-a/abstract.txt` | `980f670df7734b49a5d90bb3c305cd2b565d0b8b` | **SAME**（未改） |
| Softguess | **NONE×4** | **NONE×4** |

---

## Softguess 机扫 @ 2026-10-12 10:56 Asia/Singapore（工作区，merge 前 tip 四文件 + 改后）

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

已扫贡献列表、§1 intro（0956 已钉 exact-executable）、keywords、§9 related work：训练-替代/逼近族在贡献 #4 为本拍唯一剩余软点；keywords/related 无同族未钉句。若后续再无同族句则转 E 切闸门，不叠改根主张。

---

## 不叠 / 不碰

- §8.1#3 同身份平台矩阵、8509/8769 双身份、Ο1（荐冻结 v0.0.19 vs Latest）— **仍开；本拍不叠 Ο1 卡、不重钉**
- 键数、product code、投稿方向、根主张 — **不改**
