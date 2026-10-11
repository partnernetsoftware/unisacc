# Paper A 衍生净化：§5.8／§8 Windows POSIX 层完稿不得搭乘 A

- **拍点：** 2026-10-11 ~09:11 Asia/Shanghai（UTC+8）
- **切口：** **E. 衍生净化**
- **仓库 tip（只读核对 / before）：** `a4d85631c47f881c76ed4ded0dbf9f0d18180c85`（相对 PR #102 tip `de565baf…` 后另有产品／research stamp `882a0dce`／`a4d85631`；Paper A 四 blob 未漂）
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1＝网络≡表（声明域枚举）；unisacc 为 POSIX C99 子集跨架构实证载体）→ 未来 **Windows POSIX／仿 cosmopolitan POSIX 层战役旁支**（计划层完稿；与 macOS+Linux 通用转发 DISTINCT）→ 仍回 **A**
- **状态：** **note only**（登记「必须先完成 Windows POSIX／仿 cosmopolitan 风格 POSIX 层完稿（§5.8／§8 已写明 Windows 上仿 cosmopolitan 的 POSIX 层只有计划、尚未实现），才许投 A／构造+T1 才成立／缺之则实证或根主张不完整」升格 ≠ 理论／根主张闸门；**不起稿**独立 Windows POSIX 完稿程序正文；**不改** A 根主张、键数身份、CN/EN/TeX/abstract、product/kernel/weights/facts、投稿方向；**不发明**完稿数字；**不声称** Windows POSIX 层已实现；**不代裁** Ο1/Ο2／投稿产物身份；**不降级** §8.1／8509-8769／Ο1/Ο2；**不重写** [`paper-a-derive-purify-cc-interop-not-a-gate-20261011-0829.md`](paper-a-derive-purify-cc-interop-not-a-gate-20261011-0829.md)／[`paper-a-derive-purify-libc-boundary-not-a-gate-20261011-0615.md`](paper-a-derive-purify-libc-boundary-not-a-gate-20261011-0615.md)／[`paper-a-derive-purify-asm-text-assembler-not-a-gate-20261011-0856.md`](paper-a-derive-purify-asm-text-assembler-not-a-gate-20261011-0856.md)／[`paper-a-derive-purify-macho-coff-object-not-a-gate-20261011-0623.md`](paper-a-derive-purify-macho-coff-object-not-a-gate-20261011-0623.md)／[`paper-a-derive-purify-nativeabi-union16-not-a-gate-20261011-0811.md`](paper-a-derive-purify-nativeabi-union16-not-a-gate-20261011-0811.md)／[`paper-a-derive-purify-five-vs-six-selfhost-campaign-not-a-gate-20261011-0159.md`](paper-a-derive-purify-five-vs-six-selfhost-campaign-not-a-gate-20261011-0159.md)／[`paper-a-derive-purify-platform-matrix-empirical-program-not-a-gate-20261011-0127.md`](paper-a-derive-purify-platform-matrix-empirical-program-not-a-gate-20261011-0127.md)／[`paper-a-derive-purify-product-maturity-not-theory-20261010-1801.md`](paper-a-derive-purify-product-maturity-not-theory-20261010-1801.md)／[`paper-a-derive-purify-named-diff-ledger-closure-not-a-gate-20261011-0553.md`](paper-a-derive-purify-named-diff-ledger-closure-not-a-gate-20261011-0553.md)／[`paper-a-derive-purify-construction-combo-fail-not-a-gate-20261011-0752.md`](paper-a-derive-purify-construction-combo-fail-not-a-gate-20261011-0752.md)；**不重做** cc-interop／libc-boundary／asm-text-assembler／macho-coff／nativeabi／five-vs-six／platform-matrix／product-maturity／named-diff／construction-combo）
- **Softguess：** tip 四文件 **NONE×4**（本拍机扫：CN `74893e3d…` · EN `95657738…` · TeX `3830ede1…` · abs `43d7ea7e…`；`Softguess`／`权重就是`／`weights are the logic`／`逻辑本身`／`权重即逻辑` 均 0 hit）
- **测量身份：** 8509/8769 **仍开** — 仅引用既有钉；本拍**不重钉、不叠**；**不叠** Ο1/Ο2；Windows POSIX 开放工程债**仍保留**，本拍只切「升成理论／根主张闸门」读法
- **Paper A 正文漂移：** tip 四 blob **SAME**（相对本拍 tip before；本拍零正文 diff）
- **搭乘风险源（本拍钉）：** tip §5.8「通用转发」：macOS 与 Linux 上成立；**「Windows 上仿 cosmopolitan 的 POSIX 层只有计划（[设计](win-posix-plan.md)），尚未实现。」** tip §8「语言与产品范围」：v0.0.21 开发树把转发推广为通用转发（写出镜像与 `-run`，macOS 与 Linux，§5.8），**「Windows POSIX 层仍是计划。」** tip 树另有 `archive/research/win-posix-plan.md`（2026-10-02 计划稿；legend 标明 [P]=proposal, not built）。——诚实披露「macOS+Linux 通用转发开发树可用；Windows POSIX 层计划／未实现」≠ 「须先完稿 Windows POSIX／仿 cosmopolitan 层才许谈构造+T1／缺之则根主张空」理论闸门。**DISTINCT：** §5.8 cc 互调 ≠ Windows POSIX 层；libc-boundary／转发边界 ≠ Windows POSIX 完稿闸门。
- **相邻已封／已切（本拍不重做；正交声明）：**
  - [`paper-a-derive-purify-cc-interop-not-a-gate-20261011-0829.md`](paper-a-derive-purify-cc-interop-not-a-gate-20261011-0829.md)（§5.8 cc 互调；**相邻 DISTINCT**：彼＝对象↔cc 互调；**本拍钉**＝「Windows POSIX／仿 cosmopolitan 层完稿＝理论／根主张闸门」）
  - [`paper-a-derive-purify-libc-boundary-not-a-gate-20261011-0615.md`](paper-a-derive-purify-libc-boundary-not-a-gate-20261011-0615.md)（§8.1#5 libc 边界／外置包与转发；**正交**：转发／外置口径 ≠ Windows POSIX 层完稿）
  - [`paper-a-derive-purify-asm-text-assembler-not-a-gate-20261011-0856.md`](paper-a-derive-purify-asm-text-assembler-not-a-gate-20261011-0856.md)（§5.6 汇编；**正交**）
  - [`paper-a-derive-purify-macho-coff-object-not-a-gate-20261011-0623.md`](paper-a-derive-purify-macho-coff-object-not-a-gate-20261011-0623.md)／[`paper-a-derive-purify-nativeabi-union16-not-a-gate-20261011-0811.md`](paper-a-derive-purify-nativeabi-union16-not-a-gate-20261011-0811.md)／[`paper-a-derive-purify-five-vs-six-selfhost-campaign-not-a-gate-20261011-0159.md`](paper-a-derive-purify-five-vs-six-selfhost-campaign-not-a-gate-20261011-0159.md)／[`paper-a-derive-purify-platform-matrix-empirical-program-not-a-gate-20261011-0127.md`](paper-a-derive-purify-platform-matrix-empirical-program-not-a-gate-20261011-0127.md)／[`paper-a-derive-purify-product-maturity-not-theory-20261010-1801.md`](paper-a-derive-purify-product-maturity-not-theory-20261010-1801.md)／[`paper-a-derive-purify-named-diff-ledger-closure-not-a-gate-20261011-0553.md`](paper-a-derive-purify-named-diff-ledger-closure-not-a-gate-20261011-0553.md)／[`paper-a-derive-purify-construction-combo-fail-not-a-gate-20261011-0752.md`](paper-a-derive-purify-construction-combo-fail-not-a-gate-20261011-0752.md)（对象文件／nativeabi／自举五六／同身份矩阵／成熟度／具名差分／组合；**正交**）
  - 平台矩阵 §8.1／8509-8769／Ο1/Ο2 — **仍开硬缺口**；本拍**不混读**为「缺 Windows POSIX 层完稿」即否定构造+T1

---

## 主张一句（本页唯一）

**把「必须先完成 Windows POSIX／仿 cosmopolitan 风格 POSIX 层完稿（§5.8／§8 已写明该层只有计划、尚未实现），才许投 A／构造+T1 才成立／缺之则实证或根主张不完整」——不得搭乘 A。A 只保留构造 + T1 + 诚实披露：开发树通用转发在 macOS 与 Linux 上成立；Windows 上仿 cosmopolitan 的 POSIX 层仍是计划、尚未实现；该项完稿是挂回 A 的工程／产品旁支，不是理论／根主张闸门。开放工程债仍保留；不降级 §8.1／8509-8769／Ο1/Ο2；不发明完稿数字；不声称 Windows POSIX 已实现；与刚切的 cc-interop／libc-boundary／asm-text-assembler 正交 DISTINCT。**

---

## 切什么（从 A 的主张负担里切出）

切出对象是 **把「Windows POSIX／仿 cosmopolitan POSIX 层」开放工程债升成 A 理论完备或根主张闸门**，从而在「尚未交出该层完稿」时被挑战者用来否定构造法／T1／实证完整性：

| 位点 | 摘句／现状要点 | 分类 |
| --- | --- | --- |
| tip §5.8 通用转发 | macOS 与 Linux 上成立；「Windows 上仿 cosmopolitan 的 POSIX 层只有计划（设计 win-posix-plan），尚未实现。」 | **keep 开发树证据＋诚实披露**；切出「须先 Windows POSIX 完稿＝理论／根主张闸门」 |
| tip §8 语言与产品范围 | v0.0.21 通用转发（macOS 与 Linux，§5.8），「Windows POSIX 层仍是计划。」 | **keep 诚实披露**；切出「未实现＝须先完稿才许投」 |
| archive `win-posix-plan.md` | 2026-10-02 计划稿；[P]=not built | **keep 计划指针**；切出「计划存在＝必须先完稿才许投」 |
| cc-interop／libc-boundary／asm／macho／nativeabi | 互调／转发边界／汇编／对象文件／ABI | **正交已切** |
| 8509/8769／§8.1 矩阵／Ο1/Ο2 | 仍开硬缺口 | **keep seal-blocking**；本拍不代裁、不替代 |
| A 根主张／T1 | 构造网络；声明域精确；unisacc 实证 | **keep in A** |

**不切：** 根主张、键数 8509/8769/9174、CN/EN/TeX/abstract、开放 Windows POSIX 工程债本身（可做，非理论闸门）、§5.8／§8 已有「计划／尚未实现」诚实句、macOS+Linux 通用转发开发树证据、§8.1 同身份矩阵硬缺口、Ο1/Ο2、已切旁支、product/kernel/weights/facts、投稿方向。

---

## 为何不挡 A 投稿（切出项本身）

1. **A 根主张是构造＋T1，不是「已交出 Windows POSIX／仿 cosmopolitan 层完稿」。** 缺该层不否定网络≡表；正文已诚实写「只有计划、尚未实现」。
2. **开放工程债清单 ≠ 理论／根主张闸门。** §5.8／§8 登记的是工程／产品债；不能反向读成「必须先完稿才许谈构造+T1」。
3. **诚实披露「macOS+Linux 通用转发可用；Windows POSIX 仍计划」≠ 完稿闸门（本拍钉）。** tip 已标明计划／未实现；把「须先把已标明开放的义务升成投稿前提」升成理论闸门，是把工程战役塞进根主张。
4. **与 cc-interop／libc-boundary／asm-text-assembler／macho-coff／nativeabi 正交（收窄）。** cc-interop＝对象↔cc 互调；libc＝外置／转发口径；本拍单钉「Windows POSIX 层完稿＝A 闸门」——**DISTINCT**。
5. **与仍开硬缺口正交、不互替。** §8.1 矩阵／8509-8769／Ο1/Ο2 仍是 A 封口债；「Windows POSIX 战役」**不能替代**也不能**冒充**这些测量债已闭，也**不得**把「缺 Windows POSIX 完稿」混读成对构造+T1 的理论否决。
6. **本拍零 Paper A 正文 diff。** 只 notes + registry。

---

## 父节点如何回 A

- **工程／产品回 A：** 若未来 Windows POSIX／仿 cosmopolitan 层闭合，可回来收紧 §5.8／§8 披露边界；不能另起第二根，也不能把未完成写成对 T1 的反例。
- **实证回 A：** 「macOS+Linux 通用转发＋Windows POSIX 计划／未实现」已在 A 的诚实披露内；该层完稿是工程／产品栈的旁支义务，父边仍是 A 的构造+T1+§5.8／§8 诚实，不是新编译器理论。
- **DAG：** A（construction + T1 + §5.8／§8 诚实披露：generic forwarding on macOS+Linux; Windows POSIX layer planned/not implemented）→ win-posix-layer campaign adjunct（开放）→ 仍回 A；**不是**与 A 并列的第二套理论。

---

## 切后 A 哪一句更硬

> **Paper A 只主张基于神经网络（构造网络与权重）的编译器与 T1，并保留诚实披露：§5.8／§8 记录的是开发树通用转发在 macOS 与 Linux 上成立，以及 Windows 上仿 cosmopolitan 的 POSIX 层只有计划、尚未实现；该层完稿属挂回 A 的工程／产品旁支，不是 A 投稿前提，也不是「未完稿则构造法／T1／实证不成立」的否决条件。**

攻击面从「你们还没把 Windows POSIX／仿 cosmopolitan 层做完，根主张／实证是空的？」缩回 **「A = 构造 + T1 + §5.8／§8 诚实披露；Windows POSIX 战役另挂」**。

---

## 与既有衍生／硬缺口的正交

| 主题 | 关系 |
| --- | --- |
| cc-interop | **正交（相邻 DISTINCT）**：§5.8 cc 互调 ≠ Windows POSIX 层完稿 |
| libc-boundary | **正交**：外置包／转发口径 ≠ Windows POSIX 层完稿闸门 |
| asm-text-assembler／macho-coff／nativeabi | **正交** |
| five-vs-six／platform-matrix／product-maturity／named-diff／construction-combo | **正交** |
| 8509/8769 | **仍开硬缺口**；词表轴 ≠ Windows POSIX 战役轴 |
| Ο1/Ο2 | **仍开**；本拍不代裁 |
| Softguess | **NONE×4**；不粘 |

---

## 衍生课题提案（不起稿）

| 项 | 内容 |
| --- | --- |
| **暂定题** | §5.8／§8 Windows POSIX / cosmopolitan-style POSIX layer campaign completion (hangs on A) |
| **一行主张** | Elevating “must finish the Windows POSIX / cosmopolitan-style POSIX layer (tip §5.8/§8: planned, not implemented; macOS+Linux generic forwarding works on the dev tree) before Paper A may be submitted or construction+T1 stand” into a theory/root gate is an independent engineering/product adjunct hanging on A, not required for A’s root claim; A only needs construction + T1 + honest disclosure that the Windows POSIX layer remains plan-only. |
| **上游** | Paper A（T1；§5.8／§8 诚实登记） |
| **协议指针** | 不起稿不得改 A 键数或 T1；不得把「Windows POSIX 仍开」写成已实现；不得与 cc-interop／libc／asm／8509-8769／Ο1/Ο2 混读为「已切＝可删开放义务」；不发明完稿数字；不声称 Windows POSIX 已实现；不重写相邻已切注 |
| **状态** | **note only** — 无独立 TeX/中英草稿；不改正文 |

---

## 明确不做

- 不改正文 CN/EN/TeX/abstract（零 Paper A prose diff）
- 不改键数身份 8509/8769/9174；不改 product/kernel/weights/facts
- 不起稿 Windows POSIX 完稿程序文；不声称该层已实现；不叠 Ο1/Ο2；不重钉 8509/8769；不刷新平台矩阵；不重写 cc-interop／libc／asm／macho／nativeabi／five-vs-six／platform-matrix／product-maturity／named-diff／construction-combo
- 不发明完稿数字；不代裁公开／arXiv／Ο1 vs Ο2
- 不把 Softguess／近似推断升成产品路径；本拍确认 tip **NONE×4**
- 不发明假「Windows POSIX 层已完稿」bullet

---

## 证据指针（本拍）

- tip before：`a4d85631c47f881c76ed4ded0dbf9f0d18180c85`（PR #102 后另有 stamp；Paper A 未漂）
- Latest 产品标签事实：不刷新；本拍不改派冻结
- 四 blob（本拍零正文 diff）：CN `74893e3df1f9663b5d11f5bad6dc3cf0ca59e884` · EN `95657738e0b447ee4e40ea9ca3dd961329aebffc` · TeX `3830ede13556e8c9673f994fd6d843937476ecf5` · abs `43d7ea7e7297ed8be9a96e3950e014e359467973`
- Softguess 机扫：**NONE×4**
- 风险源：tip §5.8「Windows 上仿 cosmopolitan 的 POSIX 层只有计划…尚未实现」；tip §8「Windows POSIX 层仍是计划」；`archive/research/win-posix-plan.md`；无既有 `research/notes/*win-posix*GATE*` 切注（本拍新建）
