# Paper A 成篇定稿增量：§7.4「不只是训练的替代品」→「不是训练的替代或逼近（与训练不同类）」三联对齐

- **拍点：** 2026-10-12 ~10:11 Asia/Singapore（UTC+8）
- **切口：** **A. 成篇定稿增量**
- **仓库 tip（before）：** `bfdbdebfd4bcd8f1caed057da640829028f43b2e`（短 `bfdbdebf`；= #239 after A #238 exact-executable-form；相对任务书 tip ~bfdbdebf **无漂**）
- **tip after：** `81ad1cc73f6c6ac8341739c0eeabf6bed62514aa`（短 `81ad1cc7`；= squash Merge #240 本 A）
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1＝网络≡表于声明域 \(K_s\)；unisacc 为 POSIX C99 子集跨架构实证）
- **状态：** research-only CN+EN+TeX §7.4 同义收紧；**不改** abstract（该句不在摘要）；**不改**键数身份、product/kernel/weights/facts、投稿方向；**不叠** Ο1/Ο2（仍荐 Ο1）；**不重钉** Softguess／§8.1#3／8509·8769
- **Softguess：** tip 四文件 **NONE×4**（机扫 `Softguess`/`softguess`/`SoftGuess`/`权重就是`/`weights are the logic`/`逻辑本身`/`权重即逻辑`/`权重≡逻辑` 均 0 hit；§3「不含 softmax」／「no softmax」否认句不计；**未**粘 Softguess 口号）
- **Latest：** **v0.0.40**；冻结重测身份仍 **v0.0.19**；tip version **0.0.41**（本拍不 bump）

---

## 主张一句（本页唯一）

**§7.4 把「构造—枚举不只是训练的替代品 / not merely a substitute for training」收紧为「不是训练的替代品或逼近，而是与训练不同类的验证纪律（权重由表构造而非训练）/ not a substitute for—or approximation of—training, but a distinct verification discipline … (weights are constructed from tables, not trained)」——堵住「not merely」仍承认「是训练替代」的攻击面；同一实质改动同步进 CN/EN/TeX；不引入 Softguess 口号字面。**

---

## Before / after 句段（精确）

### CN `research/unisacc-paper.md` §7.4

- **Before：** `两条可迁移的经验：构造—枚举不只是训练的替代品，而是一种可以贯穿整个实现的验证纪律；…`
- **After：** `两条可迁移的经验：构造—枚举不是训练的替代品或逼近，而是一种与训练不同类、可以贯穿整个实现的验证纪律（权重由表构造而非训练）；…`

### EN `research/unisacc-paper.en.md` §7.4

- **Before：** `Two lessons transfer: construction plus enumeration is not merely a substitute for training but a verification discipline that can run through an entire implementation; …`
- **After：** `Two lessons transfer: construction plus enumeration is not a substitute for—or approximation of—training, but a distinct verification discipline that can run through an entire implementation (weights are constructed from tables, not trained); …`

### TeX `research/arxiv-paper-a/main.tex` §7.4

- **Before：** `Two lessons transfer: construction plus enumeration is not merely a substitute for training but a verification discipline that can run through an entire implementation; …`
- **After：** `Two lessons transfer: construction plus enumeration is not a substitute for---or approximation of---training, but a distinct verification discipline that can run through an entire implementation (weights are constructed from tables, not trained); …`

### abstract `research/arxiv-paper-a/abstract.txt`

- **本拍不改**（摘要无「substitute for training」句；核心主张句已含 constructed / rather than trained / key-by-key / not learned from data）。

---

## 为何更难挑战

| 攻击读法 | Before 软点 | After 挡法 |
| --- | --- | --- |
| 「not merely X but Y」仍承认 X | 「不只是训练的替代品」逻辑上仍把构造—枚举放进「训练替代」范畴 | 显式「不是…替代品或逼近」／「not a substitute for—or approximation of—training」 |
| 构造≈另一种训练／逼近训练 | 「替代品」可被读成功能等价的训练路线 | 「与训练不同类」+ 括注「权重由表构造而非训练」锚定 construction≠training |
| Softguess／破平近似合法出货 | 不涉及；义务仍在 Def2/3·Alg1.8·§3 | **未**粘 Softguess 口号；Softguess NONE×4 保持 |

---

## tip / blob 表

| 项 | before (`bfdbdebf`) | after（本拍；merge 后回填 SHA） |
| --- | --- | --- |
| tip | `bfdbdebfd4bcd8f1caed057da640829028f43b2e` | `81ad1cc73f6c6ac8341739c0eeabf6bed62514aa` |
| CN `research/unisacc-paper.md` | `0e358fdb035ec43da65661f86e0e940b944b5361` | **CHANGED** → `4d49887755a7aeb79ce46dd4756d0e9a1db6579f` |
| EN `research/unisacc-paper.en.md` | `d2e5aa7ddc3b6ff81589b9ec9a36db6b368e1001` | **CHANGED** → `5ef967c3d4eea723b101697e8c789c8ef7790194` |
| TeX `research/arxiv-paper-a/main.tex` | `b57e0079bd9b1696289382a947fc84805adbdcdf` | **CHANGED** → `c4ad1d88739bd2d5b14e9e2dc3284ba8abaa1f4a` |
| abs `research/arxiv-paper-a/abstract.txt` | `d2eb48f7814b7b578b5644c5e19ff9a76ffe3d3b` | **SAME**（未改） |
| Softguess | **NONE×4** | **NONE×4** |

相对 #168 基线：本拍起 Paper A 正文相对 0956 tip 再变 CN/EN/TeX；abs 仍 #168。

---

## Softguess 机扫 @ 2026-10-12 10:11 Asia/Singapore（工作区，merge 前 tip 四文件 + 改后）

| 文件 | Softguess | softguess | SoftGuess | 权重就是 | weights are the logic | 逻辑本身 | 权重即逻辑 | 权重≡逻辑 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| CN | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| EN | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| TeX | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| abstract | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |

- **Softguess_NONE_x4 = true**
- **weights_slogan_NONE_x4 = true**

---

## 挡粘点（本拍不处理）

1. §8.1#3 同一测量身份六目标原生矩阵仍开
2. 8509/8769 测量身份并存仍 seal-blocking
3. Ο1 已荐、本拍**不叠**新决策卡

---

## 待批板

- **Ο1：** 仍荐；本拍无新信息 → **禁止叠卡**
