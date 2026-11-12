# Paper A 衍生净化：伴生 Paper B（UJS）完稿／出货不得搭乘 A

- **拍点：** 2026-10-11 ~00:26 Asia/Shanghai（UTC+8）
- **切口：** **E. 衍生净化**
- **仓库 tip（只读核对 / before）：** `3a46c5c6382bf6ff42cc327ec677e034f0fba76f`
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1；unisacc 为 POSIX C99 跨架构实证载体）→ 已有草稿的 **伴生应用 Paper B（UJS／闭合 JS 子集）** → 方法核仍回 **A**
- **状态：** **note only**（登记「B 完稿／出货 ≠ A 投稿闸门」；**不起稿、不改写** [`../ujs-paper.md`](../ujs-paper.md)；**不改** A 根主张、键数身份、CN/EN/TeX/abstract、product/kernel/weights/facts；**不重做** [`paper-a-derive-purify-wasm-app-20261010-2231.md`](paper-a-derive-purify-wasm-app-20261010-2231.md)）
- **Softguess：** tip 四文件 **NONE×4**（本拍机扫：CN `74893e3d…` · EN `95657738…` · TeX `3830ede1…` · abs `43d7ea7e…`）
- **测量身份：** 8509/8769 **仍开** — 仅引用既有钉；本拍**不重钉、不叠**；**不叠** ~17:08 决策卡 Ο1/Ο2
- **Paper A 正文漂移：** tip 相对 PR #73 合入后纸面 tip，四 blob **SAME**（CN/EN/TeX/abs）；本拍零正文 diff
- **相邻已封／已切（本拍不重做）：**
  - [`paper-a-derive-purify-wasm-app-20261010-2231.md`](paper-a-derive-purify-wasm-app-20261010-2231.md)（**独立** wasm 应用文升格 ≠ A 闸门；**正交**：未起稿 wasm ≠ 「已有草稿的 B 必须先完稿才可投 A」）
  - tip 文末伴生句：「同一方法在 JavaScript 与 WebAssembly 上的迁移见伴生文 Paper B」——**keep 伴生指针**；本拍切出「伴生指针＝A 已承诺 B 必先完稿／出货才可投」
  - [`../ujs-paper.md`](../ujs-paper.md)：Status＝workshop／short-paper draft；**Paper A owns the method kernel**；B cites A — 本拍**不重写**
  - 今晚已切 TDD 方法文／Softguess／ABI／refusal／C／LLM／wasm／optim／memsafe D／T3 — **不重做**
  - product-line「论文不要跟着产品名各写一篇」（paper-notes）— **相邻意向**；本拍只钉 **B 这一支伴生应用** 不得当闸门

---

## 主张一句（本页唯一）

**把「必须先完稿／出货伴生 Paper B（UJS／闭合 JS 子集），或把 tip『方法迁移见 Paper B』读成 A 已承诺交齐 Web／JS 迁移实例才算可投」——或主张「B 仍是 workshop 草稿／门禁未齐则 A 根主张／方法迁移不完整」——不得搭乘 A。A 只需构造 + T1 + unisacc（POSIX C99）实证；B 是挂回 A 的应用伴生文（已有草稿、可独立推进），不是第二套编译器理论，也不挡 A 投稿。**

---

## 切什么（从 A 的主张负担里切出）

切出对象是 **把「伴生 B 尚未 workshop→camera-ready／尚未 Web 出货」升成 A 投稿闸门或根主张否决条件**，以及把 tip 伴生指针读成 **强制先交齐 B**：

| 位点 | 摘句／现状要点 | 分类 |
| --- | --- | --- |
| tip 伴生句（CN/EN/TeX 文末） | 「JavaScript 与 WebAssembly … Paper B」 | **keep 伴生指针**（本拍不改正文）；切出「A 承诺 B 必先完稿／出货才可投」 |
| [`../ujs-paper.md`](../ujs-paper.md) | workshop draft；method kernel 在 A；B cites | **keep in B**；切出「B 草稿未齐＝A 方法迁移债／seal 前提」 |
| DAG／paper-notes | B＝Web 闭合 JS 子集应用，父节点 A；投稿顺序 A 先独立能投、B 不跟 A 抢 | **keep 登记**；本拍升格为正式 derive-purify：**B 完稿 ≠ A 闸门** |
| wasm-app ~22:31 | 独立 wasm 文 ≠ A 闸门 | **evidence parent／正交**；本拍钉的是**已有草稿的 B**，不是未起稿 wasm |
| A 根主张／C99 实证 | 构造网络；unisacc POSIX C99 | **keep in A** |

**不切：** 根主张、键数 8509/8769/9174、§8.1 同身份矩阵、Ο1/Ο2、Softguess、T1、B 已有草稿与 UJS 产品证据、product/kernel/weights/facts、wasm-app 正文、tip 伴生句措辞（只登记闸门边界；择机收窄并进语气非本拍义务）。

---

## 为何不挡 A 投稿

1. **A 根主张不依赖 B 完稿。** 核心是 TSV 表 → 构造网络 → T1 + unisacc（C99）实证。B 是应用层迁移实例，引用 A 的方法核，不是 A 的完备条件。
2. **B 自己写明 method kernel 在 A。** ujs-paper 摘要／贡献已写 Paper A owns the method kernel；B does not re-prove IntNet existence。缺 B camera-ready 不否定 A 的构造＋T1。
3. **DAG 投稿顺序已写「A 先独立能投；B 不跟 A 抢」。** 本拍只把这句话升成「不得把 B 未齐读成 A 闸门」的净化登记。
4. **与 wasm-app 正交。** wasm 切的是**未起稿**独立 wasm 应用升格；本拍切的是**已有草稿的伴生 B** 完稿／出货义务不得搭乘 A。二者都挂回 A，但攻击句不同。
5. **本拍零 Paper A 正文 diff。** 只 notes + registry；四 blob 不改。

---

## 父节点如何回 A

- **方法回 A：** B 的任何完稿／出货，必须以 A 的构造＋T1 为上游引用，不能改写根主张为「编译器理论＝须先交齐 Web／JS 迁移文」。
- **旁支回 A：** 父边 **A（construction + T1 + C99 实证）→ B（UJS 应用伴生）**；结果可回来展示「跨语言迁移实例」，不能长成与 A 并列的第二套编译器理论，也不能回写成 A 的投稿前提。
- **DAG：** A 独立可投；B 可并行推进、不挡 A；wasm 另支（已切、不起稿）≠ B。

---

## 切后 A 哪一句更硬

切出后，A 可把伴生应用边界收成一句硬边界（**本拍不改正文；仅登记主张**）：

> **Paper A 只主张基于神经网络（构造网络与权重）的编译器与 T1，并以 unisacc（POSIX C99）为实证载体；伴生 Paper B（UJS）是挂回 A 的应用迁移文，可独立完稿与出货，不是 A 投稿前提，也不是「B 仍为 workshop 草稿则构造法／方法迁移不成立」的否决条件——tip 指向 B 的伴生指针不等于强制先交齐 B。**

攻击面从「你们连 Paper B／UJS Web 迁移都还是草稿，方法迁移不完整／不能单独投 A？」缩回 **「A = 构造 + T1 + C99 实证；B 另挂、草稿不挡投」**。

---

## 衍生课题提案（不起稿／不改写 B）

| 项 | 内容 |
| --- | --- |
| **暂定题** | Companion Paper B (UJS) Completion Is Not a Paper A Submission Gate (application side-branch; hangs on A) |
| **一行主张** | Elevating finishing or shipping companion Paper B (UJS)—or reading A's companion pointer to B as a commitment that A must first deliver a complete Web/JS transfer instance—into a Paper A submission gate is an independent application-process proposition hanging on A, not required to submit A; A only needs construction + T1 + unisacc (POSIX C99) empirics; B already drafts as a cite-A transfer and may advance on its own schedule. |
| **上游** | Paper A；[`../ujs-paper.md`](../ujs-paper.md)；[`paper-a-derive-purify-wasm-app-20261010-2231.md`](paper-a-derive-purify-wasm-app-20261010-2231.md)（正交）；DAG [`../paper-notes-20261009.md`](../paper-notes-20261009.md) |
| **协议指针** | 不改写 ujs-paper／B 主张；不得把 B 门禁红读成 A 理论债；不得与 wasm-app（未起稿独立 wasm）／TDD 方法文／8509-8769／Ο1/Ο2／平台矩阵混读；不得写成第二套编译器理论；不起新 B 正文于本拍 |
| **状态** | **note only** — 不改正文；不改 B 草稿 |

---

## 与既有衍生／硬缺口的正交

| 主题 | 关系 |
| --- | --- |
| 测量身份 8509/8769 | **仍开硬缺口**；词表轴 ≠ B 完稿 |
| 同身份平台矩阵 §8.1 | **仍开 · seal-blocking**；矩阵 ≠ B 文 |
| 产物轴 Ο1/Ο2（~17:08 卡） | **正交**；本拍**不叠、不代裁** |
| Softguess | **NONE×4**；不粘 |
| wasm-app ~22:31 | **正交**：独立未起稿 wasm ≠ 伴生 B 完稿闸门 |
| TDD 方法文 ~00:13 | **正交**：产品工程方法文 ≠ 应用伴生文完稿 |
| tip「JS 与 WebAssembly → B」 | **引用、本拍不改正文**；登记闸门边界 |

---

## 明确不做

- 不改正文 CN/EN/TeX/abstract（零 Paper A prose diff）
- 不改键数身份 8509/8769/9174；不改 product/kernel/weights/facts
- 不重写／不起稿 ujs-paper／B；不重做 wasm-app；不叠 Ο1/Ο2；不重钉 8509/8769；不刷新平台矩阵
