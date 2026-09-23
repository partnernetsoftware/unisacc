# UJS 论文（Paper B）—— 作者一页提纲

> 对应正文草稿：`research/ujs-paper.md`  
> 伴生：Paper A = UNISA / unisacc（方法 + C 自举）；本文 = 同一方法 × 动态语言子集 × Web 产品。

---

## 标题备选

1. **UJS: Exact-by-Construction Table Networks for a Closed JavaScript Subset on the Web**（主推；与 A 对称）
2. *Shipping `wasm_run`: Constructed IntNets for UJS-1 (Companion to UNISA SH)*
3. *From Gold Tables to Browser Wasm: A Closed JS Pipeline without Ask-Time Fallback*
4. 中文备选：《UJS：由 gold 表构造的闭合 JS 子集与 Web 交付》

---

## 贡献点（对外可说的）

- **方法迁移**：与 A 共享构造代数、IntNet、P-1/P-2/P-3、SHIP_ACC=1.000；不训练出货。
- **UJS-1 闭合语言**：表内全功能、表外永久拒绝；显式 `G`/`L`；无原型/`eval`/async。
- **jtape + fold**：语义真源；jtape VM ≡ WasmProgram（+ 可选宿主 wasm）。
- **IC 阶段**：`shape×op×guard→stub_kind`；有限 stub 库；`icfold` 保证开关同答。
- **产品面**：`wasm_run` + `ujs_full.wasm`（browser/Node/Bun）；Python 仅在 `construct/`；二进制走 Release + `BUILD.json` 指纹。
- **验收叙事**：定性绑定 `tests/ujs.sh`——acc / fold / icfold / front parity / ship / in-page wasm_run；**不编造数字**。
- **可选产物指针**（非数字表）：playground；**`ujs/web/engine/demo/`**（Host ABI）；`ujs/web/game/`（Three 对照）。

---

## 与 Paper A 的分工（审稿人会问）

| | Paper A (UNISA) | Paper B (UJS) |
|---|---|---|
| 语言 | C 子集 / 自举叙事 | 闭合 JS（UJS-1） |
| 交付 | CLI shell；不交付 Web | npm/`wasm_run`/playground |
| 特有层 | tape→多 ISA、自举不动点等 | jtape、IC、wasm lower、前后端 image 奇偶 |
| 共享 | Shell 命题、gold→权重、枚举、IntNet、禁回退 | 同左；文中一律 **[cite A]** |
| 不抢功 | 表→MLP 构造、枚举验证本身 | 同；指 prior-art / Tracr / Jia&Rinard |

A 可未发表：正文写 *companion manuscript / technical report*，占位 **[cite A]**。

---

## 建议投稿方向（非正式）

- **短文 / workshop**：WASM / PL / neuro-symbolic 交叉（强调 *product + contracts*，非 SOTA 速度）。
- **系统演示轨**：playground + `ship` kit + 套件名即 artifact。
- **不宜硬闯**：纯 ML 顶会（无训练故事）；纯验证顶会（P-3 细节在 A）；「完整 JS 引擎」叙事。
- 若与 A 同投：B 作 companion / tool paper；交叉引用，避免重复 IntNet 证明篇幅。

---

## 写作时红线

- 不写假 latency / 假准确率小数；精确性只说契约与套件通过。
- 不跑 `unisa train` / 不暗示页内训练。
- 体积优势勿当主论（prior-art 已警示 ACAS/BDD 线）。
- 改产品代码非本稿范围；规格以 `ujs/prd.md` 为准。

---

## 一句话定位

**A 证明方法在 C shell 上成立；B 证明同一方法能支撑闭合动态语言流水线并开箱交付 Web 上的 `wasm_run`。**
