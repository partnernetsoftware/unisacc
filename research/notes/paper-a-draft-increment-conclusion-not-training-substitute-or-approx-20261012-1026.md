# Paper A 成篇定稿增量：§10 结论对齐 §7.4「不是训练的替代或逼近」三联对齐

- **拍点：** 2026-10-12 ~10:26 Asia/Singapore（UTC+8）
- **切口：** **A. 成篇定稿增量**
- **仓库 tip（before）：** `f55371c24f5ea2c351b70758b76ba73c97296da0`（短 `f55371c2`；= #241 after A #240 §7.4 not-training-substitute；相对任务书 tip `f55371c2` **无漂**）
- **tip after：** `39a7dbf08a65c1c65e033e591a742f51e803e704`（短 `39a7dbf0`；= squash Merge #242 本 A）
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1＝网络≡表于声明域 \(K_s\)；unisacc 为 POSIX C99 子集跨架构实证）
- **状态：** research-only CN+EN+TeX §10 结论同义收紧，对齐已硬的 §7.4；**不改** abstract（摘要无该软结论句）；**不改**键数身份、product/kernel/weights/facts、投稿方向；**不叠** Ο1（仍荐）；**不重钉** Softguess／§8.1#3／8509·8769
- **Softguess：** tip 四文件 **NONE×4**（机扫 `Softguess`/`softguess`/`SoftGuess`/`权重就是`/`weights are the logic`/`逻辑本身`/`权重即逻辑`/`权重≡逻辑` 均 0 hit；§3「不含 softmax」／「no softmax」否认句不计；**未**粘 Softguess 口号）
- **Latest：** **v0.0.40**；冻结重测身份仍 **v0.0.19**；tip version **0.0.41**（本拍不 bump）

---

## 主张一句（本页唯一）

**§10 结论把「构造+枚举+逐字节差分作为一种验证纪律，其价值不限于…」收紧为「不是训练的替代品或逼近，而是与训练不同类的验证纪律（权重由表构造而非训练）／not as a substitute for—or approximation of—training, but as a distinct verification discipline … (weights constructed from tables, not trained)」——与已硬的 §7.4 同质；堵住结论段仍可被读成「训练替代路线的价值陈述」的攻击面；同一实质改动同步进 CN/EN/TeX；不引入 Softguess 口号字面。**

---

## Before / after 句段（精确）

### CN `research/unisacc-paper.md` §10

- **Before：** `我们认为，“构造 + 枚举 + 逐字节差分”作为一种验证纪律，其价值不限于神经网络：它同样能在手写代码中找到抽样测试找不到的缺陷。`
- **After：** `我们认为，“构造 + 枚举 + 逐字节差分”不是训练的替代品或逼近，而是一种与训练不同类的验证纪律（权重由表构造而非训练），其价值不限于神经网络：它同样能在手写代码中找到抽样测试找不到的缺陷。`

### EN `research/unisacc-paper.en.md` §10

- **Before：** `We believe that construction plus enumeration plus byte-level differential testing, as a verification discipline, is valuable beyond neural networks: it also finds defects in hand-written code that sampled testing cannot.`
- **After：** `We take construction + enumeration + byte-for-byte differencing not as a substitute for—or approximation of—training, but as a distinct verification discipline (weights constructed from tables, not trained), whose value is not limited to neural networks: it also finds defects in hand-written code that sampled testing cannot.`

### TeX `research/arxiv-paper-a/main.tex` §10 Conclusion

- **Before：** `We believe that construction plus enumeration plus byte-level differential testing, as a verification discipline, is valuable beyond neural networks: it also finds defects in hand-written code that sampled testing cannot.`
- **After：** `We take construction + enumeration + byte-for-byte differencing not as a substitute for---or approximation of---training, but as a distinct verification discipline (weights constructed from tables, not trained), whose value is not limited to neural networks: it also finds defects in hand-written code that sampled testing cannot.`

### abstract `research/arxiv-paper-a/abstract.txt`

- **本拍不改**（摘要无「as a verification discipline, is valuable」软结论句；核心主张句已含 constructed / rather than trained / key-by-key / not learned from data）。

---

## 为何更难挑战

| 攻击读法 | Before 软点 | After 挡法 |
| --- | --- | --- |
| 结论只陈述「验证纪律有价值」未排除训练替代范畴 | 「作为一种验证纪律，其价值不限于」未显式否定 substitute/approx | 显式「不是…替代品或逼近」／「not as a substitute for—or approximation of—training」 |
| §10 与 §7.4 软硬不一，审稿人可挑软结论 | §7.4 已硬（#240），§10 仍软 | 同质措辞对齐 §7.4（不同类 + 权重由表构造而非训练） |
| 构造≈另一种训练／逼近训练 | 「作为一种验证纪律」可被读成训练工作流的补充 | 「与训练不同类」+ 括注锚定 construction≠training |
| Softguess／破平近似合法出货 | 不涉及；义务仍在 Def2/3·Alg1.8·§3 | **未**粘 Softguess 口号；Softguess NONE×4 保持 |

---

## tip / blob 表

| 项 | before (`f55371c2`) | after（本拍；merge 后回填 SHA） |
| --- | --- | --- |
| tip | `f55371c24f5ea2c351b70758b76ba73c97296da0` | `39a7dbf08a65c1c65e033e591a742f51e803e704` |
| CN `research/unisacc-paper.md` | `4d49887755a7aeb79ce46dd4756d0e9a1db6579f` | **CHANGED** → `d00fd2507fde66253a12efd2f3b3048282b7acb7` |
| EN `research/unisacc-paper.en.md` | `5ef967c3d4eea723b101697e8c789c8ef7790194` | **CHANGED** → `5ed764f4ffdd9d23f2ea2aaf39de4098b4e489ff` |
| TeX `research/arxiv-paper-a/main.tex` | `c4ad1d88739bd2d5b14e9e2dc3284ba8abaa1f4a` | **CHANGED** → `bf62f9e90f67957cf2242153a910fc1420a9aaf0` |
| abs `research/arxiv-paper-a/abstract.txt` | `d2eb48f7814b7b578b5644c5e19ff9a76ffe3d3b` | **SAME**（未改） |
| Softguess | **NONE×4** | **NONE×4** |

相对 #168 基线：本拍起 Paper A 正文相对 1011 tip 再变 CN/EN/TeX；abs 仍 #168。

---

## Softguess 机扫 @ 2026-10-12 10:26 Asia/Singapore（工作区，merge 前 tip 四文件 + 改后）

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
