# Paper A 成篇定稿增量：§1「可执行编码」→「精确可执行形式（非学得统计替代）」三联对齐

- **拍点：** 2026-10-12 ~09:56 Asia/Singapore（UTC+8）
- **切口：** **A. 成篇定稿增量**
- **仓库 tip（before）：** `4d645c71cea3224ac523ef4a0cddaf4889b0af0c`（短 `4d645c7`；= #237 after E #236 c41 Draft-create；相对任务书 tip ~4d645c7 **无漂**）
- **tip after：** （merge 后回填）
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1＝网络≡表于声明域 \(K_s\)；unisacc 为 POSIX C99 子集跨架构实证）
- **状态：** research-only CN+EN+TeX §1 同义收紧；**不改** abstract（该句不在摘要）；**不改**键数身份、product/kernel/weights/facts、投稿方向；**不叠** Ο1/Ο2（仍荐 Ο1）；**不重钉** Softguess／§8.1#3／8509·8769
- **Softguess：** tip 四文件 **NONE×4**（机扫 `Softguess`/`softguess`/`SoftGuess`/`权重就是`/`weights are the logic`/`逻辑本身`/`权重即逻辑`/`权重≡逻辑` 均 0 hit；§3「不含 softmax」／「no softmax」否认句不计；**未**粘 Softguess 口号）
- **Latest：** **v0.0.40**；冻结重测身份仍 **v0.0.19**；tip version **0.0.41**（本拍不 bump）

---

## 主张一句（本页唯一）

**§1 把「阈值网络是表的可执行编码 / executable encoding of the table」收紧为「声明域上的精确可执行形式（与表逐键同一，不是学得的统计替代）/ exact executable form … identical key-by-key … not a learned statistical surrogate」——同一实质改动同步进 CN/EN/TeX；堵住把构造网络读成「表的近似／编码代理」的攻击面，且不引入 Softguess 口号字面。**

---

## Before / after 句段（精确）

### CN `research/unisacc-paper.md` §1

- **Before：** `在表覆盖的域上，阈值网络是表的可执行编码，权重由构造器从表导出。`
- **After：** `在表覆盖的域上，阈值网络是表在声明域上的精确可执行形式（与表逐键同一，不是学得的统计替代），权重由构造器从表导出。`

### EN `research/unisacc-paper.en.md` §1

- **Before：** `on the table-covered domain, the threshold network is an executable encoding of the table, with weights exported by the constructor.`
- **After：** `on the table-covered domain, the threshold network is an exact executable form of the table (identical key-by-key on the declared domain, not a learned statistical surrogate), with weights exported by the constructor.`

### TeX `research/arxiv-paper-a/main.tex` §1

- **Before：** `on the table-covered domain, the threshold network is an executable encoding of the table, with weights exported by the constructor.`
- **After：** `on the table-covered domain, the threshold network is an exact executable form of the table (identical key-by-key on the declared domain, not a learned statistical surrogate), with weights exported by the constructor.`

### abstract `research/arxiv-paper-a/abstract.txt`

- **本拍不改**（摘要无「executable encoding」句；核心主张句已含 constructed weights / key-by-key / not learned from data）。

---

## 为何更难挑战

| 攻击读法 | Before 软点 | After 挡法 |
| --- | --- | --- |
| 「encoding」= 近似表示／有损编码 | 「可执行编码／executable encoding」可被读成代理或近似 | 显式「精确可执行形式」+「逐键同一／identical key-by-key」锚定 T1 身份 |
| 构造网络仍是学得的统计替代 | 未在该句否定 surrogate | 显式「不是学得的统计替代／not a learned statistical surrogate」 |
| Softguess／破平近似合法出货 | 不涉及；义务仍在 Def2/3·Alg1.8·§3 | **未**粘 Softguess 口号；Softguess NONE×4 保持 |

---

## tip / blob 表

| 项 | before (`4d645c7` / #168 正文基线) | after（本拍） |
| --- | --- | --- |
| tip | `4d645c71cea3224ac523ef4a0cddaf4889b0af0c` | （merge 后回填） |
| CN `research/unisacc-paper.md` | `5231dd3bf3e60804c678d48015f1e337edd9dc56` SAME vs #168 | **CHANGED** → `0e358fdb035ec43da65661f86e0e940b944b5361` |
| EN `research/unisacc-paper.en.md` | `0a707d704079bfef3b2b5193bfa8dc2b5ec90303` SAME vs #168 | **CHANGED** → `d2e5aa7ddc3b6ff81589b9ec9a36db6b368e1001` |
| TeX `research/arxiv-paper-a/main.tex` | `aab7bd9b8e3c72d08aa2672a559513bb70e02049` SAME vs #168 | **CHANGED** → `b57e0079bd9b1696289382a947fc84805adbdcdf` |
| abs `research/arxiv-paper-a/abstract.txt` | `d2eb48f7814b7b578b5644c5e19ff9a76ffe3d3b` SAME vs #168 | **SAME**（未改） |
| Softguess | **NONE×4** | **NONE×4** |

#168 基线 merge `ae7f7424585f60fc0184e86e3dbd3b32686125bf`（image field layout）。本拍起 Paper A 正文基线迁至本 tip（CN/EN/TeX 变；abs 仍 #168）。

---

## Softguess 机扫 @ 2026-10-12 09:56 Asia/Singapore（工作区，merge 前）

| 文件 | Softguess | softguess | SoftGuess | 权重就是 | weights are the logic | 逻辑本身 | 权重即逻辑 | 权重≡逻辑 | softmax（对照） |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| CN | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1（「不含 softmax」） |
| EN | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1（「no softmax」） |
| TeX | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1（「no softmax」） |
| abstract | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |

- **Softguess_NONE_x4 = true**
- **weights_slogan_NONE_x4 = true**

---

## 本拍明确未改

- 键数／测量身份：8509 / 8769 / 9174 / op-\* / 517 / 569 / unit-test 数字
- product / kernel / weights / facts
- §8.1#3 矩阵、Ο1 决策卡（仍荐 Ο1，不叠卡）
- Softguess 字面／口号粘贴
- abstract.txt
- 投稿方向／arXiv 推送

---

## 挡粘仍开（本拍不重钉、不叠卡）

1. §8.1#3 六目标同身份原生矩阵
2. 摘要／表口径 8509 vs 表 1 合计 8769（class-(a) 双身份）
3. Ο1（仍荐；不叠新卡）

---

## 切后 A 更硬在哪

「可执行编码」可被读成近似／代理的攻击面收窄为：声明域上与表逐键同一的精确可执行形式，并显式否定学得统计替代——构造 vs 训练边界在 §1 命名句更硬，且三联对齐。
