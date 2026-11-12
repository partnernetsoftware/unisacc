# Paper A 衍生净化：局部性＝构造法独有编辑定理 / edit locality as construction-unique theorem

- **拍点：** 2026-10-10 ~15:43 Asia/Shanghai（UTC+8）
- **切口：** **E. 衍生净化**
- **仓库 tip（只读核对 / before）：** `eec82d5a0f40abac937a4ee0825d6968214fe99d`
- **父节点：** **A2 RQ2**（单条规则编辑的权重局部性与无关键零错；归档预注册 [`../../archive/research/a2-preregistration.md`](../../archive/research/a2-preregistration.md)）；最终回 **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；实证载体 unisacc POSIX C99 跨架构）
- **状态：** **note only**（登记未来独立编辑定理 / 构造法「独有」主张负荷；**不起稿正文**；**不改** A 根主张、键数身份、摘要 T1 句、product/kernel/weights/facts；**不重做** seal-rebuild-vs-locality）
- **Softguess：** tip 四文件 **NONE×4**（本拍机扫：`research/unisacc-paper.md` / `unisacc-paper.en.md` / `arxiv-paper-a/main.tex` / `abstract.txt` 均 0 hit）
- **测量身份：** 8509/8769 **仍开** — 引用既有钉 [`paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md`](paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md)；本拍**不重钉、不叠** ~03:44 路线Ι 卡
- **已封口（本拍不重做）：** [`../seal-rebuild-vs-locality-20261009.md`](../seal-rebuild-vs-locality-20261009.md)（PR #16：摘要「精确重建」= T1 全表重跑构造器，≠ A2/RQ2 单条编辑局部性）

---

## 主张一句（本页唯一）

**单条规则编辑下的权重局部性与无关键零错，若升成「构造法独有编辑定理」，不得搭乘 A 投稿；A 只保留 T1（更新全表后确定性全量重建仍全域精确），局部性实验与「独有」主张留给 A2 RQ2 / 未来衍生。**

---

## 切什么（从 A 的主张负担里切出）

切出对象是 **把「改一条规则 → 权重只动相关 bank、原有键零错」升成须证的构造法独有编辑定理**，以及任何把摘要/§7.3「精确重建」误读成局部性定理的投稿级负荷；登记为挂回 **A2 RQ2** 的未来独立课题，而不是 A 的投稿级定理：

| 稿内位点（tip `eec82d5a`） | 摘句要点 | 分类 |
| --- | --- | --- |
| CN/EN/TeX 摘要；`abstract.txt` | 表上增删规则后，对更新后全表做确定性全量重建仍逐键精确（T1）；**不**主张单条编辑局部性或无关键零错（A2 RQ2） | **keep T1 in A**（已由 seal-rebuild 澄清）；**「独有编辑定理」migrate** |
| CN/EN/TeX §7.3 | 「精确重建」= 自更新表全量重跑构造器后仍全域精确（T1）；单条规则编辑局部性属 A2/RQ2 | **keep T1 vs RQ2 指针 in A**；**系统性局部性实验 migrate → A2** |
| 先验封口 | [`seal-rebuild-vs-locality-20261009.md`](../seal-rebuild-vs-locality-20261009.md)：文案对齐，非新定理 | **evidence parent**；本衍生不重写 seal |
| A2 预注册 RQ2 | 改一条规则要动多少权重、会弄坏多少原有键；含「构造法独有局部性」推翻条件 | **正式实验宿主**；本衍生不改预注册正文 |
| `paper-notes`「局部性」意向段 | 已写「若成立搬出去是编辑定理；若不成立 A 去掉『局部性为构造法独有』」 | **本拍升格登记** → 正式 derive-purify 笔记 |

**不切：** 根主张（构造非训练、T1、神经编译器命名）、表 1 键数、§7.3/A2 训练对照其余三问、表 4/表 5 历史耗时（已登记）、自举不动点（已登记）、偏函数定义边界（已登记）、目标参数化（已登记）、摘要与 §7.3 已封口的 T1≠局部性句。

---

## 为何不挡 A 投稿

1. **A 根主张不依赖编辑局部性定理。** 核心是表→构造网络→声明域穷举精确（T1）；「改一条只动一小块」是 A2 RQ2 的实验假设，不是存在性定理。
2. **T1≠局部性已在正文与 seal 对齐。** seal-rebuild（PR #16）已把摘要与 §7.3 钉死为全量重建；审稿人仍可能把「独有局部性」读成 A 的须证定理——故升格登记衍生以卸负担。
3. **A2 RQ2 已冻结判据与推翻条件。** 局部性实验与「独有」主张的宿主是 A2；本衍生只搬走投稿搭乘风险，**不替代、不改写** A2 预注册。
4. **本拍零正文 diff。** 不改正文、不改键数、不重做 seal-rebuild；只登记。

---

## 父节点如何回 A

- **实证/实验回 A2→A：** 单条编辑局部性实验属 A2 RQ2；结果可收窄 A 是否保留「局部性为构造法独有」这类软句，但不能长成第二套编译器理论。
- **理论句回 A：** 「更新全表后确定性全量重建仍全域精确（T1）」仍是根主张；衍生只搬走「须证独有编辑定理」升格读法。
- **DAG：** A → A2（RQ2）→（未来）edit-locality / construction-unique editing theorem；**不是**第二根，也不是偏函数衍生、C（管道）或表 4/5 测量衍生。

---

## 切后 A 哪一句更硬

切出后，A 可把重建叙事收成一句硬边界：

> **表上增删规则后，对更新后的全表做一次确定性全量重建，仍得到与表逐键一致的网络（T1）。本文不主张、亦不证明单条规则编辑下的权重局部性或无关键零错，更不把「局部性＝构造法独有」写成须证定理；该实验与「独有」主张留给 A2 RQ2 / 未来衍生。**

攻击面从「是不是在证明编辑局部性 / 是不是在主张构造法独有？」缩回 **T1 全量重建**，与根主张正交；与既有 seal-rebuild 文案封口同向。

---

## 衍生课题提案（不起稿）

| 项 | 内容 |
| --- | --- |
| **暂定题** | Edit Locality as a Construction-Unique Theorem: Single-Rule Weight Banks, Unrelated-Key Zero Error, and Falsification via A2 RQ2 |
| **一行主张** | Under single-rule edits, whether constructed compiler networks change only related weight banks with zero error on unrelated keys—and whether that locality is unique to construction vs fine-tuning / model editing—is an independent theorem/experiment owned by A2 RQ2, not a Paper A T1 claim. |
| **上游** | Paper A（T1；[`seal-rebuild-vs-locality-20261009.md`](../seal-rebuild-vs-locality-20261009.md)）；**直接父节点** A2 RQ2（[`../../archive/research/a2-preregistration.md`](../../archive/research/a2-preregistration.md)） |
| **协议指针** | T1≠局部性文案已封于 seal-rebuild；本课题若起稿须沿 A2 预注册判据另开独立 TeX/中英，不得改 A 键数或 T1 表述 |
| **状态** | **note only** — 无独立 TeX/中英草稿；不改 A2 预注册文件（本拍不做） |

---

## 与既有衍生/硬缺口的正交

| 主题 | 关系 |
| --- | --- |
| 测量身份 8509/8769 | **仍开硬缺口**；本拍只引用 12:39 钉，不重论 |
| 表 4 速度区间（~14:25 E） | 正交（测量带宽 ≠ 编辑局部性） |
| 自举不动点（~14:40 E） | 正交 |
| 表 5 历史耗时（~14:57 E） | 正交 |
| 偏函数／partial-fn（~15:12 E） | 正交（定义边界 ≠ 编辑局部性） |
| 目标参数化（~12:56 E） | 正交 |
| seal-rebuild-vs-locality（2026-10-09，PR #16） | **父证据**：T1≠RQ2 文案已封；本拍是 derive-purify 升格登记，不重做 seal |
| A2 RQ2 规则局部性 | **直接父节点 / 实验宿主**；本衍生是「独有编辑定理」投稿负荷卸除 |
| Softguess | **NONE×4**；非硬缺口 |

---

## 明确不做

- 不改正文 CN/EN/TeX/abstract（含不改摘要 T1 句、不重写 seal-rebuild）
- 不改键数身份；不改 product/kernel/weights/facts
- 不起稿；不叠 ~03:44 决策卡；不重钉 8509/8769
- 不改写 A2 预注册；不发明新定理句塞回 A
- 不重做已封口的 T1≠局部性三联对齐
- 不与 ~15:12 偏函数衍生合并（正交）

---

## 证据

- tip（before）：`eec82d5a0f40abac937a4ee0825d6968214fe99d`
- Paper A blobs：CN `f40070830bc44538ac15c3734dcc8257a8e3d7ee` · EN `7878d8072a35438db9653cb21c257b362c7a0665` · TeX `eacae1a1dafa8b4d81eaaa4283d73e1792636a48` · abs `43d7ea7e7297ed8be9a96e3950e014e359467973`（相对 ~15:12 心跳 **SAME**）
- Softguess：**NONE×4**（四正文文件无 Softguess 字面）
- 先验：`research/seal-rebuild-vs-locality-20261009.md`；`archive/research/a2-preregistration.md` RQ2；`research/paper-notes-20261009.md`「局部性」意向段
- 短报：[`paper-a-heartbeat-report-20261010-1543.md`](paper-a-heartbeat-report-20261010-1543.md)
- 父节点：**A2 RQ2 → A**
