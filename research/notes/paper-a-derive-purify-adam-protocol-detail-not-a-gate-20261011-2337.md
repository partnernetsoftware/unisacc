# Paper A 衍生净化：§7.3 Adam 实验协议细节不得搭乘 A

- **拍点：** 2026-10-11 ~23:37 Asia/Singapore（UTC+8）
- **切口：** **E. 衍生净化**
- **仓库 tip（只读核对 / before）：** `46cf167cf1ac5a8be3b3fa0400f5abf1ee1fb0ee`（= Merge #161；父链经 #160 K6-12 E、#159 C tip-refresh、#154 T1→\(K_s\)）
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1＝网络≡表于声明域 \(K_s\)；unisacc 为 POSIX C99 子集跨架构实证载体）→ **预注册旁支 A2**（系统构造 vs 训练对照；RQ1–RQ4；冻结 [`../../archive/research/a2-preregistration.md`](../../archive/research/a2-preregistration.md)）→ 仍回 **A**
- **状态：** **note only**（登记「§7.3 Adam 超参／冻结协议／11-of-18／θ 计数／合成目标／无冻结稳定性 30/99 等实验协议细节 ≠ A 必交可复现方法叙事／≠ seal-before 协议」升格；**不起稿**；**不改** A 根主张、键数身份、CN/EN/TeX/abstract、product/kernel/weights/facts、投稿方向、`archive/research/a2-preregistration.md` 冻结判据；**不重做** [`paper-a-derive-purify-a2-train-contrast-not-a-gate-20261011-0114.md`](paper-a-derive-purify-a2-train-contrast-not-a-gate-20261011-0114.md)（整包 A2 完稿闸门）、[`paper-a-derive-purify-width-complexity-wstar-20261010-1615.md`](paper-a-derive-purify-width-complexity-wstar-20261010-1615.md)（宽度竞赛）、[`paper-a-derive-purify-trapdoor-unreachability-20261010-1915.md`](paper-a-derive-purify-trapdoor-unreachability-20261010-1915.md)（不可达性标签）、[`../seal-adam-width-not-theory-20261009.md`](../seal-adam-width-not-theory-20261009.md)；**不代裁** Ο1/Ο2（仍荐 Ο1）；**不重钉** Softguess／8509·8769／§8.1#3）
- **Softguess：** tip 四文件 **NONE×4**（机扫 `Softguess`/`softguess`/`权重就是`/`weights are the logic`/`逻辑本身`/`权重即逻辑`/`权重≡逻辑` 均 0 hit；§3「不含 softmax」／「no softmax」否认句不计；**NONE≠seal-before 债**）
- **测量身份：** 8509/8769 **仍开** — 本拍不重钉、不叠；**不叠** Ο1/Ο2
- **Paper A 正文漂移：** tip 四 blob 相对 #154 `bdec4bcb` **SAME**（本拍零正文 diff）

---

## 主张一句（本页唯一）

**把 §7.3 里仍残留的 Adam 实验协议细节（β₁/β₂、batch 16、学习率阶梯、首次 1.000 即冻结、仅 11/18 阶段与 21,925 θ、超出六出货平台的合成目标、以及「无冻结」稳定性 30/99 另一协议）升成 Paper A 必交可复现方法叙事／seal-before 协议／「缺协议复现则构造≠训练边界空」——不得搭乘 A。** 这些是 A2 预注册对照实验的材料；A 只保留「产品与主张仅用构造；§7.3 仅为历史示意；系统性对照属 A2」边界。A seal-before 仍只是 §8.1#3、8509·8769、Ο1|Ο2（荐Ο1）。

**DISTINCT：** 本拍切的是 **协议细节负担**（hyperparam / freeze / 子集阶段 / 合成目标 / 稳定性另一协议），不是 #0114「整包 A2 须先跑完才许投 A」闸门，也不是 width-W\*「Adam 行＝宽度优势定理」、也不是 trapdoor「＝不可达性定理」。

---

## 切什么

| 对象 | 位点（tip `46cf167c`） | 分类 |
| --- | --- | --- |
| Adam 超参与冻结协议 | CN/EN/TeX §7.3：Adam（β₁=0.9，β₂=0.999），batch 16，lr 0.032→…→0.0025，**首次 1.000 即冻结** | **migrate → A2**（实验协议宿主）；A **keep**「曾跑一次历史示意」一句 |
| 子集参与 / θ | 仅 **11 of 18** 阶段；21,925 θ | **migrate → A2**；非 A 方法必交口径 |
| 合成目标 | 合并／扩表／>24 目标行用**超出六个出货平台的合成目标** | **migrate → A2**（与预注册 RQ1 (T) 合成目标同族）；A **keep**「前六才是真实产品目标」诚实披露边界 |
| 稳定性另一协议 | 同超参**不**采用首次冻结时 30/99 曾触及 1.000 后回落 | **migrate → A2**；A **keep**「不是同一实验、不可合并解读」卫生句（不升成 A 必复现协议） |
| 历史示意表本身 | Adam vs 构造对照表（精度跌落等） | **keep as illustrative history in A**（seal-adam-width 已钉非优势定理）；切出「须按该协议复现才许封 A」 |
| A 根主张／T1 on \(K_s\) | 构造网络；声明域逐键全等 | **keep in A** |
| §8.1#3／8509·8769／Ο1\|Ο2 | 仍开硬缺口 | **keep seal-blocking**；本拍不重钉、不叠 |

**不切：** 根主张、键数数字、§8.1#3、Ο1/Ο2、product/kernel/weights/facts、CN/EN/TeX/abstract 正文（本拍零 diff）、A2 冻结预注册、K6-\* 产品战役、投稿方向。

---

## 为何不挡 A 投稿

1. **A 根主张是构造＋T1，不是「已按 §7.3 历史 Adam 协议复现对照」。** tip 已写明产品路径不用训练；§7.3 仅为历史示意；系统性对照明确外包给 A2。
2. **协议细节是 A2 的实验材料，不是 A 的方法定义。** 预注册已冻结调参网格、优化器集合、种子与推翻条件；把旧示意里的 β／lr／freeze／11-of-18 读成 A 必交复现清单，会把 A 绑死在一次未预注册的历史跑法上，反而削弱「示意 vs 系统对照」边界。
3. **与已切切片正交、命题 DISTINCT。** #0114 切「整包 A2 完稿闸门」；width-W\* 切「宽度竞赛／优势定理」；trapdoor 切「不可达性标签」。本拍切「协议细节＝A 方法／seal-before」。
4. **与仍开硬缺口正交。** §8.1#3／8509·8769／Ο1|Ο2 仍是 A 封口债；复现 Adam 协议**不能替代**也不能**冒充**这些测量债已闭。
5. **本拍零 Paper A 正文 diff。** 只 notes；四 blob 不改。日后若政委点头，可把 §7.3 协议句软成「see A2」前向引用——本拍不改正文。

---

## 父节点如何回 A

- **旁支回 A：** A（构造 + T1 + 「产品不用训练／§7.3 历史示意」边界）→ A2（系统训练对照；协议／网格／种子／推翻条件宿主）→ 仍回 A。
- 结果可回来收窄 A 是否保留任何「Adam 未达 1.000」软读法或是否把示意表再压缩，不能长成第二套编译器理论，也不能把「须按该历史协议复现」回写成投稿前提。
- **DAG：** 切出的是协议负担升格，不是删掉 §7.3 历史示意表，也不是启跑 SGD。

---

## 切后 A 哪一句更硬

切出后，A 可把训练对照边界收成一句更硬的主张（**本拍不改正文；仅登记**）：

> **Paper A 只主张基于神经网络（构造网络与权重）的编译器与 T1（网络≡表于声明域 \(K_s\)），并诚实披露产品路径不用训练、§7.3 仅为历史 Adam 示意；Adam 超参／冻结协议／子集阶段／合成目标／稳定性另一协议等实验细节属于预注册 A2，不是 A 的必交可复现方法叙事，也不是 seal-before。**

攻击面从「你们没按 β/lr/freeze/11-of-18 复现 Adam，主张空／不能投？」缩回 **「A = 构造 + T1 + 示意边界 + C99 实证（含仍开测量债）；系统训练协议另挂 A2」**。

---

## tip / blob 表（本拍核对）

| 项 | 值 |
| --- | --- |
| tip before | `46cf167cf1ac5a8be3b3fa0400f5abf1ee1fb0ee` |
| tip after（本 note 合入后填） | _(merge 后由 PR 填)_ |
| Paper A 末次正文 | #154 `bdec4bcbf823d0b527a15c40afe61f69942063fc`（T1→\(K_s\)） |
| CN `research/unisacc-paper.md` | `5231dd3bf3e60804c678d48015f1e337edd9dc56` **SAME** vs #154 |
| EN `research/unisacc-paper.en.md` | `0a707d704079bfef3b2b5193bfa8dc2b5ec90303` **SAME** |
| TeX `research/arxiv-paper-a/main.tex` | `0ef6027ccc73b20b1d7d4371f89c47b069885bb7` **SAME** |
| abs `research/arxiv-paper-a/abstract.txt` | `43d7ea7e7297ed8be9a96e3950e014e359467973` **SAME** |
| Softguess | **NONE×4**（预期态；不升 seal-before） |

---

## OUT（本拍明确不做）

- 改正文 CN/EN/TeX/abstract；粘 Softguess／近似推断
- 代裁／叠卡 Ο1/Ο2；重钉 Softguess tip-refresh 或 8509·8769／§8.1#3 tip-refresh
- 改 product/kernel/weights/facts／键数／投稿方向；bump Latest；启跑训练
- 重做 #0114 A2 完稿闸门、width-W\*、trapdoor、seal-adam-width、K6-\* E、#161 C tip-refresh
- 把 Adam 协议细节加为第四 seal-before／Softguess 触发／测量身份闭合证明

---

## 证据索引

- tip before：`46cf167c`（Merge #161）
- 相邻已切：[`paper-a-derive-purify-a2-train-contrast-not-a-gate-20261011-0114.md`](paper-a-derive-purify-a2-train-contrast-not-a-gate-20261011-0114.md)；[`paper-a-derive-purify-width-complexity-wstar-20261010-1615.md`](paper-a-derive-purify-width-complexity-wstar-20261010-1615.md)；[`paper-a-derive-purify-trapdoor-unreachability-20261010-1915.md`](paper-a-derive-purify-trapdoor-unreachability-20261010-1915.md)；[`../seal-adam-width-not-theory-20261009.md`](../seal-adam-width-not-theory-20261009.md)
- 上一拍 C：[`paper-a-gap-nail-platform-matrix-tip-refresh-t1-k612-20261011-2323.md`](paper-a-gap-nail-platform-matrix-tip-refresh-t1-k612-20261011-2323.md)
- 上一拍 E：[`paper-a-derive-purify-k6-12-image-fat-not-a-gate-20261011-2316.md`](paper-a-derive-purify-k6-12-image-fat-not-a-gate-20261011-2316.md)
- 短报：[`paper-a-heartbeat-report-20261011-2337.md`](paper-a-heartbeat-report-20261011-2337.md)
