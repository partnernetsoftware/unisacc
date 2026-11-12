# Paper A 衍生净化：编译网络是偏函数 / partial function

- **拍点：** 2026-10-10 ~15:12 Asia/Shanghai（UTC+8）
- **切口：** **E. 衍生净化**
- **仓库 tip（只读核对 / before）：** `b19427380e9ab58f5ecb12f878cbb48545ff5ba4`
- **父节点：** Paper A（基于神经网络的编译器；TSV 表构造网络与权重，非训练；实证载体 unisacc POSIX C99 跨架构）
- **状态：** **note only**（登记未来独立理论命题 / 诊断-拒绝产品表面；**不起稿正文**；**不改** A 根主张、键数身份、定义 3 正文、product/kernel/weights/facts）
- **Softguess：** tip 四文件 **NONE×4**（本拍机扫：`filename:unisacc-paper.md` / `unisacc-paper.en.md` / `path:research/arxiv-paper-a` 均 0 hit）
- **测量身份：** 8509/8769 **仍开** — 引用既有钉 [`paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md`](paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md)；本拍**不重钉、不叠** ~03:44 路线Ι 卡
- **已封口（本拍不重做）：** [`../seal-refusal-partial-fn-20261009.md`](../seal-refusal-partial-fn-20261009.md)（摘要拒绝措辞 + 定义 3 偏函数澄清句）

---

## 主张一句（本页唯一）

**偏函数/未覆盖拒绝作为可独立开题的理论（及诊断产品表面）不得搭乘 A 投稿；A 只保留「覆盖键精确、未覆盖拒绝」的定义边界，不把「编译网络≡偏函数」升成须证定理，也不把拒绝 vs 错误输出的价值判断写成 A 的定理。**

---

## 切什么（从 A 的主张负担里切出）

切出对象是 **把「编译网络 ≡ 偏函数」升成须证独立定理、以及「拒绝代价高于错误输出」的价值判断 / 诊断-拒绝产品表面**，登记为未来独立的 **theory / product-surface side note**，而不是 A 的投稿级定理或产品论文：

| 稿内位点（tip `b1942738`） | 摘句要点 | 分类 |
| --- | --- | --- |
| CN/EN/TeX **定义 3**（询问纪律） | 部署 `ask` 相对更大观察/键空间是偏函数：在声明 \(K_s\) 上有定义且精确，域外报错；设计选择，不是留出泛化；泛化实验属 A2 | **keep definition boundary in A**（已由 seal-refusal 澄清）；**≡偏函数独立定理 migrate** |
| CN/EN 摘要；TeX abstract；`abstract.txt` | 构造网络对域外或未覆盖键拒绝即报错；从不回退经典编译器（arXiv 摘要经 seal-refusal 已对齐） | **keep refusal≠guess fact in A**；**诊断产品表面 migrate** |
| CN/EN/TeX 网络编译器实例叙述 | Rejection on out-of-domain or uncovered keys is an error | **keep product fact in A**；**代价比较价值判断 migrate** |
| 先验封口 | [`seal-refusal-partial-fn-20261009.md`](../seal-refusal-partial-fn-20261009.md)：文案对齐，非新定理 | **evidence parent**；本衍生不重写 seal |
| `paper-notes`「部分函数」意向段 | 已写「A 只需留下定义；价值判断留讨论、不升定理」 | **本拍升格登记** → 正式 derive-purify 笔记 |

**不切：** 根主张（构造非训练、T1、神经编译器命名）、表 1 键数、§7.3/A2 训练对照（含 A2 第三问留出泛化对照实验本身）、表 4/表 5 历史耗时（已登记）、自举不动点（已登记）、目标参数化（已登记）、定义 3 已封口的拒绝≠猜边界句。

---

## 为何不挡 A 投稿

1. **A 根主张不依赖「编译网络≡偏函数」定理。** 核心是表→构造网络→声明域穷举精确（T1）；偏函数说法是定义 3 相对 ambient 空间的阅读卫生，不是存在性定理。
2. **拒绝≠猜已在正文与 seal 对齐。** 定义 3 + 摘要拒绝句已固定产品事实；审稿人不能把未覆盖当成「该泛化却失败」，但**可以**把「偏函数」读成须证定理或把代价比较读成 A 的价值主张——故登记衍生以卸负担。
3. **A2 第三问仍挂训练对照。** 留出泛化实验属 A2，不是本衍生的正文；本衍生搬走的是「独立偏函数定理 / 诊断产品表面」负荷，**不替代** A2。
4. **本拍零正文 diff。** 不改正文、不改键数、不重做 seal-refusal；只登记。

---

## 父节点如何回 A

- **实证回 A：** 未覆盖拒绝与无经典回退仍是 A 实证载体（unisacc）上的产品事实；seal-refusal 三联对齐仍服务 A 摘要/定义 3。
- **理论句回 A：** 「覆盖键精确、未覆盖拒绝」仍是根主张的定义边界；衍生只搬走「须证 ≡偏函数」与「拒绝 vs 错误输出哪个代价更高」的升格读法。
- **DAG：** A →（未来）partial-function theory / diagnostic-refusal product surface；**不是**第二根，也不是 A2（训练对照）、C（管道方法）或表 4/5 测量衍生。

---

## 切后 A 哪一句更硬

切出后，A 可把拒绝叙事收成一句硬边界：

> **在声明域上网络与表逐键精确；域外或未覆盖键按设计拒绝（报错），从不猜、不回退经典路径。本文不把「编译网络≡偏函数」升成须证定理，也不把拒绝相对错误输出的代价比较写成定理。**

攻击面从「是不是在证明偏函数 / 是不是在主张拒绝更优？」缩回 **定义级边界**（拒绝≠猜 + T1），与根主张正交。

---

## 衍生课题提案（不起稿）

| 项 | 内容 |
| --- | --- |
| **暂定题** | Compiler Networks as Partial Functions: Exact Coverage, Designed Refusal, and Diagnostic Product Surfaces |
| **一行主张** | Relative to a larger ambient key space, a constructed compiler network is a partial function (exact on declared \(K_s\), reject outside); elevating that reading to a stand-alone theorem, and comparing the cost of refusal vs wrong output as a product thesis, are independent of Paper A's T1 existence claim. |
| **上游** | Paper A（定义 3；摘要拒绝句；[`seal-refusal-partial-fn-20261009.md`](../seal-refusal-partial-fn-20261009.md)）；旁支对照指针 A2 RQ3（留出泛化，不并进本稿） |
| **协议指针** | 文案边界已封于 seal-refusal；本课题若起稿须另开独立 TeX/中英，不得改 A 键数或 T1 表述 |
| **状态** | **note only** — 无独立 TeX/中英草稿；无新预注册文件（本拍不做） |

---

## 与既有衍生/硬缺口的正交

| 主题 | 关系 |
| --- | --- |
| 测量身份 8509/8769 | **仍开硬缺口**；本拍只引用 12:39 钉，不重论 |
| 表 4 速度区间（~14:25 E） | 正交（测量带宽 ≠ 偏函数定理） |
| 自举不动点（~14:40 E） | 正交 |
| 表 5 历史耗时（~14:57 E） | 正交 |
| 目标参数化（~12:56 E） | 正交 |
| seal-refusal（2026-10-09） | **父证据**：文案已封；本拍是 derive-purify 登记，不重做 seal |
| A2 RQ3 留出泛化 | 正交指针：实验属 A2；本衍生是理论/产品表面升格负荷 |
| Softguess | **NONE×4**；非硬缺口 |

---

## 明确不做

- 不改正文 CN/EN/TeX/abstract（含不改定义 3、不重写 seal-refusal）
- 不改键数身份；不改 product/kernel/weights/facts
- 不起稿；不叠 ~03:44 决策卡；不重钉 8509/8769
- 不把 A2 RQ3 并进本衍生；不发明新定理句塞回 A
- 不重做已封口的拒绝措辞三联对齐

---

## 证据

- tip（before）：`b19427380e9ab58f5ecb12f878cbb48545ff5ba4`
- Paper A blobs：CN `f4007083…` · EN `7878d807…` · TeX `eacae1a1…` · abs `43d7ea7e…`（相对 ~14:57 心跳 **SAME**）
- Softguess：**NONE×4**（四正文文件无 Softguess 字面）
- 先验：`research/seal-refusal-partial-fn-20261009.md`；`research/paper-notes-20261009.md`「部分函数」意向段
- 短报：[`paper-a-heartbeat-report-20261010-1512.md`](paper-a-heartbeat-report-20261010-1512.md)
- 父节点：**A**
