# Paper A 衍生净化：验证纪律「不限于神经网络」≠ A 已证方法定理（挂 C）— csih 后 tip 重登记

- **拍点：** 2026-10-10 ~20:30 Asia/Shanghai（CST / UTC+8）
- **切口：** **E. 衍生净化**
- **仓库 tip（只读核对 / before）：** `4fe2ca4c0b77942a46184b31ba1dd43b20ddd34e`
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；§7.4 实证发现 + 结论 §10）→ 意向 **Paper C**（管道方法：确定性推断／构造+枚举+逐字节差分推广）→ 最终仍回 **A**
- **状态：** **note only**（登记未来独立「验证纪律可推广到手写代码／非 NN 管道」命题；**不起稿正文**；**不改** A 根主张、键数身份、§7.4 单元格／案例数字、product/kernel/weights/facts；**不重写** [`../paper-c-intent.md`](../paper-c-intent.md)）
- **Softguess：** tip 四文件 **NONE×4**（`research/unisacc-paper.md` / `unisacc-paper.en.md` / `arxiv-paper-a/main.tex` / `arxiv-paper-a/abstract.txt`；本拍机扫 0 hit）
- **测量身份：** 8509/8769 **仍开** — 引用 [`paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md`](paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md)；本拍**不重钉、不叠**；**不叠** Ο1/Ο2 决策卡
- **同身份矩阵：** **仍开** — 引用 [`paper-a-gap-nail-platform-matrix-same-identity-20261010-1645.md`](paper-a-gap-nail-platform-matrix-same-identity-20261010-1645.md)；本拍不触
- **已封口／已切（本拍不重做）：**
  - [`../paper-c-intent.md`](../paper-c-intent.md)（**父意向**：种子机／迭代脱离／组合精确性；A、B 作迁移实例）
  - [`paper-a-derive-purify-verif-discipline-beyond-nn-20261010-1845.md`](paper-a-derive-purify-verif-discipline-beyond-nn-20261010-1845.md)（下午正式升格；本拍与之**并存**，不改正文、不重写 paper-c-intent）
  - [`paper-a-derive-purify-combo-merge-theorem-20261010-1552.md`](paper-a-derive-purify-combo-merge-theorem-20261010-1552.md)（阶段合并定理 → A2 RQ1／C；**正交**）
  - [`paper-a-derive-purify-product-maturity-not-theory-20261010-1801.md`](paper-a-derive-purify-product-maturity-not-theory-20261010-1801.md)（RQ1/门禁成熟度；**正交**）
  - [`../seal-combo-fail-not-t1-20261009.md`](../seal-combo-fail-not-t1-20261009.md)（组合失败不否定 T1；**正交**）
  - [`paper-a-derive-purify-bdiff-ledger-vs-tape-20261010-1826.md`](paper-a-derive-purify-bdiff-ledger-vs-tape-20261010-1826.md)／[`-1945`](paper-a-derive-purify-bdiff-ledger-vs-tape-20261010-1945.md)（tape≠台账；**正交**）
- **tip 核对说明：** 开拍 tip `4fe2ca4c`（相对 PR #59 DENSE merge `5cedc4b9` 已前进；其后含 plans/rulingwait 等）。tip 上已有 afternoon `-1845`；本拍仍按今夜重登记序列（Algo1 #56／bdiff #57／maint #58／DENSE #59 之后）**续切 verification**，新 slug `-2030`（与 `-1845` 并存；不改正文；不重写 paper-c-intent）。**本拍关闭丢失集 overnight 系列中的 verification 项。**

---

## 主张一句（本页唯一）

**把「构造 + 枚举 + 逐字节差分作为一种验证纪律，其价值不限于神经网络、同样能在手写代码中找到抽样测不到的缺陷」升成 Paper A 必须证明（或已证）的一般方法定理——从而要求 A 先完成管道推广／多场景迁移——不得搭乘 A 投稿。A 只需保留构造 + T1 + unisacc 上 §7.4 的实证发现；「纪律可推广到非 NN／整条管道／新手写场景」留给意向 Paper C（csih 后 tip 重登记；与 afternoon `-1845` 并存）。**

---

## 切什么（从 A 的主张负担里切出）

切出对象是 **把 §7.4／§10／贡献点 4 读成「已证：验证纪律普遍优于抽样、且不限于神经网络」的方法定理**，从而在「尚无 Paper C 多场景迁移／组合精确性证明」时被挑战者用来否定 A 的投稿完备性：

| 稿内位点（tip `4fe2ca4c`） | 摘句要点 | 分类 |
| --- | --- | --- |
| 结论 §10（CN） | 「我们认为，“构造 + 枚举 + 逐字节差分”作为一种验证纪律，其价值不限于神经网络：它同样能在手写代码中找到抽样测试找不到的缺陷。」 | **keep 信念／展望语气 as soft discussion**；**「已证方法定理／投稿前提」migrate → C** |
| 结论 §10（EN/TeX） | `We believe that construction plus enumeration plus byte-level differential testing, as a verification discipline, is valuable beyond neural networks: it also finds defects in hand-written code that sampled testing cannot.` | 同上 |
| §7.4 两课（EN） | `construction plus enumeration is not merely a substitute for training but a verification discipline that can run through an entire implementation` | **keep unisacc 实证课**；**「整机／任意实现上已证可跑通的方法定理」migrate → C** |
| 引言贡献点 4 | 「验证纪律及其发现……包括枚举找到、抽样找不到的手写代码缺陷」 | **keep 指向 §7.4 发现**；**「纪律已推广为一般方法」migrate → C** |
| RQ4 设问 | 「这套验证纪律能否发现传统测试发现不了的缺陷？」 | **keep 作为 unisacc 实证 RQ**；**「肯定答案 = 一般方法定理」migrate** |

**不切：** 根主张（构造非训练、T1）、§7.4 里 data-layout 枚举案例的具体数字与 segfault 发现（实证保留）、组合失败≠T1 seal、product/kernel/weights/facts、键数、表 1/4/5 单元格、paper-c-intent 正文（本拍只交叉引用）。

---

## 为何不挡 A 投稿

1. **A 根主张不依赖「纪律可推广到非 NN」。** 核心是构造法 + T1 + unisacc 实证；§7.4 只需报告在本载体上枚举／差分找到了什么，不必先证一般管道方法。
2. **意向 C 早已承接推广。** [`paper-c-intent.md`](../paper-c-intent.md) 明确：C 回答「为什么能推广、条件是什么、步骤能否机械化」；A、B 只作实例被引用。本拍是 derive-purify **csih 后 tip 重登记**，把「beyond NN／整机纪律定理」从 A 理论负担卸下（与 afternoon `-1845` 同主张、新 tip）。
3. **§10 的 “We believe / 我们认为” 本是软讨论。** 挑战者若把它读成硬定理，会要求多场景证明；切出后 A 可明确：那是 C 的命题，不是 A 投稿闸门。
4. **本拍零正文 diff。** 不改正文 CN/EN/TeX/abstract、不改键数；只登记（与今夜 #56 Algo1／#57 bdiff／#58 maint／#59 DENSE 同模式）。

---

## 父节点如何回 A

- **理论句回 A：** 构造 + T1 + §7.4 在 unisacc 上的分层验证发现仍是 A；「纪律」在 A 里是实证叙述，不是第二根。
- **方法旁支 C：** 「构造+枚举+逐字节差分」推广到非编译器管道、手写纯函数场景的可证明条件与迁移协议，挂意向 C；父边仍回 A（A 提供第一个钉死的迁移实例）。
- **DAG：** A（construction + T1；§7.4 empirical findings）→ C（pipeline / verification-discipline generalization）；与 combo-merge（可达性）、product-maturity（门禁成熟度）、Algo1／bdiff／maint／DENSE 正交。

---

## 切后 A 哪一句更硬

切出后，A 可把 §10／贡献点 4 收成一句硬边界（**本拍不改正文；仅登记主张**）：

> **§7.4 报告的是 unisacc 载体上「构造 + 枚举 + 逐字节差分」找到的具体缺陷与两课；结论里「价值不限于神经网络／同样适用于手写代码」是讨论性展望，不是 Paper A 已证的一般方法定理，也不构成投稿前必须完成的管道推广义务。系统性推广、组合精确性与多场景迁移属意向 Paper C；未完成 C，不自动否定构造法或 T1。**

攻击面从「你们还没证明纪律能推广，A 是不是吹大了？」缩回 **「A = 构造 + T1 + 本载体发现；推广另立 C」**；挑战者用未完成的方法推广债攻击根主张时，A 不必把 beyond-NN 定理当投稿前提来防守。

---

## 衍生课题提案（不起稿）

| 项 | 内容 |
| --- | --- |
| **暂定题** | Verification Discipline Beyond Neural Networks: When Construct–Enumerate–Byte-Diff Generalizes (Paper C; hangs on A) |
| **一行主张** | Elevating “construction + enumeration + byte-level differential testing is valuable beyond neural networks / finds defects sampling cannot in hand-written code” into a proved general method theorem required of Paper A is an independent pipeline-method proposition (Paper C), not required to submit Paper A; Paper A only needs construction + T1 + honest §7.4 findings on the unisacc vehicle. |
| **上游** | Paper A（§7.4 / §10 / 贡献点 4；[`paper-c-intent.md`](../paper-c-intent.md)）；afternoon [`-1845`](paper-a-derive-purify-verif-discipline-beyond-nn-20261010-1845.md) |
| **协议指针** | 不起稿不得改 A 键数或 T1；不得删 §7.4 实证数字冒充「已证推广」；不得与 8509-8769 身份钉或 Ο1/Ο2 混读；成文前仍按 C 意向：A/B 只作实例 |
| **状态** | **note only** — 无独立 TeX/中英草稿；不改正文；**不重写** paper-c-intent；与 `-1845` **并存** |

---

## 与既有衍生／硬缺口的正交

| 主题 | 关系 |
| --- | --- |
| 测量身份 8509/8769 | **仍开硬缺口**；键数身份 ≠ 验证纪律推广 |
| 同身份平台矩阵 | **仍开**；平台执行矩阵 ≠ 方法推广 |
| 重测身份决策卡 Ο1/Ο2 | **正交**；本拍**不叠、不代裁** |
| Algo1（~19:30 E／PR #56） | 正交 |
| bdiff（~19:45 E／PR #57） | 正交 |
| maint（~19:52 E／PR #58） | 正交 |
| DENSE（~20:07 E／PR #59） | 正交（部署命名 vs 方法推广） |
| afternoon `-1845` | **并存**；本拍不删、不改名 |
| Softguess | **NONE×4**；非硬缺口 |
| 丢失集 overnight（Algo1／bdiff／maint／DENSE／verification） | **本拍关闭 verification 项** |

---

## 明确不做

- 不改正文 CN/EN/TeX/abstract（含**不删** §7.4 案例、**不重写** §10 展望句为硬定理或硬否决）
- 不改键数身份 8509/8769/9174；不改 product/kernel/weights/facts
- 不起稿；不叠决策卡 Ο1/Ο2；不重钉 8509/8769
- **不重写** paper-c-intent；不发明「必须先完成 C 才能投 A」新句塞回 A
- **不删** afternoon `-1845`；不与 Algo1／bdiff／maint／DENSE 合并

---

## 证据

- tip（before）：`4fe2ca4c0b77942a46184b31ba1dd43b20ddd34e`
- Paper A blobs：CN `74893e3df1f9663b5d11f5bad6dc3cf0ca59e884` · EN `95657738e0b447ee4e40ea9ca3dd961329aebffc` · TeX `3830ede13556e8c9673f994fd6d843937476ecf5` · abs `43d7ea7e7297ed8be9a96e3950e014e359467973`（本 tip；本拍零正文 diff → **SAME**）
- Softguess：**NONE×4**
- 先验：[`../paper-c-intent.md`](../paper-c-intent.md)（仅引用）；[`-1845`](paper-a-derive-purify-verif-discipline-beyond-nn-20261010-1845.md)；CN/EN/TeX §7.4／§10
- 短报：[`paper-a-heartbeat-report-20261010-2030.md`](paper-a-heartbeat-report-20261010-2030.md)
- 父节点：**A** → **C**（推广）
