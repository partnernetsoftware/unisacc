# Paper A 衍生净化：C99 条款账本／规范性条款覆盖完稿不得搭乘 A

- **拍点：** 2026-10-11 ~02:58 Asia/Shanghai（UTC+8）
- **切口：** **E. 衍生净化**
- **仓库 tip（只读核对 / before）：** `7a064de576c079fa2d3d804005b6390f50b91eb0`
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1＝网络≡表；unisacc 为 POSIX C99 **子集**跨架构实证载体）→ 未来 **C99 条款账本／规范性覆盖战役旁支** → 仍回 **A**
- **状态：** **note only**（登记「须先把 `tests/c99/clauses.tsv` 规范性子条款做到全 covered／清掉 unsupported／或交齐条款覆盖战役程序文，才算 A 可投／构造+T1 才成立」升格 ≠ 理论闸门；**不起稿**独立条款完备正文；**不改** A 根主张、键数身份、CN/EN/TeX/abstract、product/kernel/weights/facts、clauses.tsv；**不删、不改写** §5.5 条款分母诚实披露；**不叠** Ο1/Ο2；**不重做** product-maturity、external-referee、五 vs 六、C99 种子／Python TCB、cx-lab、平台矩阵实证程序等已切项）
- **Softguess：** tip 四文件 **NONE×4 产品路径**（本拍机扫：CN `ceb394aa…` · EN `cc58115d…` · TeX `3826ec96…` · abs `da956706…`；正文仅「不含／no softmax」否定句，无 Softguess 产品路径升格）
- **测量身份：** 8509/8769 **仍开** — 仅引用既有钉；本拍**不重钉、不叠**；**不叠** Ο1/Ο2
- **Paper A 正文漂移：** tip 四 blob **SAME**（相对 ~02:38 封口机扫）；本拍零正文 diff
- **相邻已封／已切（本拍不重做）：**
  - [`paper-a-derive-purify-product-maturity-not-theory-20261010-1801.md`](paper-a-derive-purify-product-maturity-not-theory-20261010-1801.md)／[`paper-a-derive-purify-tdd-product-engineering-methods-20261011-0013.md`](paper-a-derive-purify-tdd-product-engineering-methods-20261011-0013.md)（RQ1·Lua·门禁成熟度／TDD 方法文；**正交**：工程成熟度 ≠ 「须先交齐规范性条款账本全 covered」）
  - [`paper-a-derive-purify-external-referee-coverage-20261011-0052.md`](paper-a-derive-purify-external-referee-coverage-20261011-0052.md)（阶段外部裁判 8/18→18/18；**正交**：表语义裁判 ≠ C99 标准条款分母台账）
  - [`paper-a-derive-purify-c99-seed-constructor-python-tcb-not-a-gate-20261011-0238.md`](paper-a-derive-purify-c99-seed-constructor-python-tcb-not-a-gate-20261011-0238.md)／[`paper-a-derive-purify-five-vs-six-selfhost-campaign-not-a-gate-20261011-0159.md`](paper-a-derive-purify-five-vs-six-selfhost-campaign-not-a-gate-20261011-0159.md)（今晚已切种子／五六；**不重做**）
  - [`seal-bootstrap-bytes-not-semantic-20261009.md`](../seal-bootstrap-bytes-not-semantic-20261009.md)（字节自举 ≠ C99 完备／表≡C；**支撑**本拍，不重写 seal）
  - 平台矩阵 §8.1／8509-8769／Ο1/Ο2 — **仍开硬缺口**；本拍**不混读**为「缺条款全覆盖」理论否决

---

## 主张一句（本页唯一）

**把「必须先把 §5.5 的 C99 条款账本（`tests/c99/clauses.tsv`）做到语言／环境／库规范性子条款全 covered、清掉 unsupported／partial，或先交齐条款覆盖战役完稿程序文，才算 Paper A 可投／构造+T1 才成立」——或主张「库条款大量 unsupported（如 191）则子集实证空／根主张不完整」——不得搭乘 A。A 只需构造 + T1 + unisacc（POSIX C99 **子集**）实证，并如实披露条款分母与探针分母不可互换（§5.5）。条款覆盖完稿是挂回 A 的工程／标准对齐旁支，不是第二套编译器理论，也不是封口闸门。**

---

## 切什么（从 A 的主张负担里切出）

切出对象是 **把「C99 条款账本未全 covered／库条款大量 unsupported」升成 A 理论完备或投稿闸门**，从而在「条款台账仍开 partial/unsupported、或未另交条款战役程序文」时被挑战者用来否定构造法／子集实证：

| 位点 | 摘句／现状要点 | 分类 |
| --- | --- | --- |
| §5.5 条款账本 | `clauses.tsv`：语言 98→covered 95／partial 1／unsupported 2；环境 16→13／2／1；库 361→127／43／191；并声明 `c99.sh` 59/59 **不能替代**条款覆盖分母 | **keep 诚实披露／分母纪律 in A**；切出「须先全 covered＝理论／投稿闸门」 |
| 摘要／贡献 | unisacc＝C99 **子集**编译器实证；字节自举 ≠ C99 完备 | **keep 子集边界 in A**；切出「缺条款全覆盖 ⇒ 子集主张空」 |
| A 根主张／T1 | 构造网络；声明域精确；unisacc 实证 | **keep in A** |
| 仍开封口债 | §8.1 矩阵；8509/8769；Ο1/Ο2 | **keep seal-blocking**；**≠** 「缺条款全覆盖」理论否决 |

**不切：** 根主张、键数 8509/8769/9174、§5.5 条款分母披露句本身（本拍不改数字、不改措辞）、§8.1 同身份矩阵硬缺口、Ο1/Ο2、product-maturity／external-referee／种子 TCB／五 vs 六／cx-lab／平台程序已切项、product/kernel/weights/facts、CN/EN/TeX/abstract 正文、`tests/c99/clauses.tsv`。

---

## 为何不挡 A 投稿（切出项本身）

1. **A 根主张是构造＋T1，不是「已对齐全部 C99 规范性子条款」。** 实证载体明确是 C99 **子集**；条款台账是诚实范围图，不是 T1 的充要条件。
2. **披露分母 ≠ 完稿闸门。** §5.5 把 covered/partial/unsupported 摊开，并禁止用 `c99.sh` 探针冒充条款分母——这是测量纪律；不能反向读成「必须先清零 unsupported 才许谈根主张」。
3. **与外部裁判／产品成熟度正交。** 外部裁判钉的是**阶段表语义**具名裁判覆盖；product-maturity 钉的是 RQ1·Lua·门禁；本拍对象是 **ISO C99 规范性条款台账分母**。
4. **与仍开硬缺口正交。** §8.1 矩阵／8509-8769／Ο1/Ο2 仍是 A 封口债；条款全覆盖**不能替代**也不能**冒充**这些测量债已闭。
5. **本拍零 Paper A 正文 diff。** 只 notes + registry；四 blob 不改；**不**改写 §5.5 条款数字；**不**声称条款已全 covered。

---

## 父节点如何回 A

- **实证回 A：** 任何未来「C99 条款覆盖战役／标准对齐程序文」仍服务 A 已声明的子集边界——构造确定性；表语义靠外部裁判与已列边界；条款台账是范围诚实，不是完备证明。
- **旁支回 A：** 父边 **A（construction + T1 + C99 子集实证 + §5.5 条款分母诚实披露）→ 条款覆盖战役旁支**；结果可回来收紧范围措辞与验收入口，不能长成与 A 并列的第二套编译器理论，也不能把「缺条款全 covered」回写成理论投稿前提。
- **DAG：** A 在主张与测量债收口、政委点头后可独立投／公开；条款战役可并行推进；**切出的是完稿升格闸门，不是删掉 §5.5 披露，也不是降级仍开测量债**。

---

## 切后 A 哪一句更硬

切出后，A 可把条款台账收成一句硬边界（**本拍不改正文；仅登记主张**）：

> **Paper A 只主张基于神经网络（构造网络与权重）的编译器与 T1，并以 unisacc 为 POSIX C99 **子集**跨架构实证；§5.5 的 `clauses.tsv` 条款分母是范围诚实与探针／条款分母不可互换的纪律，不是「必须先清零 unsupported／交齐条款全覆盖战役（或缺之则根主张不完整）」的理论／投稿闸门——该旁支若独立成题，挂回 A，不起稿挡粘。**

攻击面从「你们库条款 191 unsupported，C99 编译器主张空？」缩回 **「A = 构造 + T1 + 子集实证（含条款分母诚实披露 + 仍开测量债）；条款全覆盖另挂」**。

---

## 衍生课题提案（不起稿）

| 项 | 内容 |
| --- | --- |
| 题名意向 | C99 clause-ledger coverage campaign / normative-subclause evidence completion (hangs on A) |
| 父节点 | A（经 construction + T1 + C99 子集实证 + §5.5 条款分母披露） |
| 与 A 关系 | 可回来收紧范围与验收措辞；不能另起第二根；不能把「缺条款全 covered」写成理论闸门；**不**替代 §8.1 矩阵／8509-8769／Ο1/Ο2；**不**改正文 §5.5 条款数字 |
| 本拍动作 | **不起稿**；不改正文；不叠 Ο1/Ο2；不改 `clauses.tsv`；不发明新覆盖百分比 |

---

## 明确非目标

- 不解决、不重钉、不降级 §8.1／8509-8769／Ο1/Ο2；**不改** §5.5 条款分母数字／措辞
- 不改正文键数、表 1/4/5、摘要主张方向；不改 product/kernel/weights/facts
- 不重做 product-maturity／external-referee／种子 TCB／五 vs 六／cx-lab／平台程序／TDD／T3／bootstrap-FP／A2／裁判／GitHub-ORCID／B／Softguess／wasm／C／D
- 不启跑条款覆盖战役；不改 `tests/c99/clauses.tsv`
- 不把 Softguess／近似推断升成产品路径；本拍确认 tip **NONE×4**
