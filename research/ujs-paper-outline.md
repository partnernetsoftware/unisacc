# UJS 论文（Paper B）—— 作者一页提纲

> 规格真源：`ujs/prd.md` · 目录：`ujs/ARCHITECTURE.md`（seed / weights / iterate / practice）  
> 伴生：Paper A = 方法核 + C 自举（Lean L0–L3 / §3.4）；本文 = **同一方法的迁移** × 闭合 JS × Web / M3 自举。

---

## 标题备选

1. **UJS: Exact-by-Construction Table Networks for a Closed JavaScript Subset on the Web**（主推）
2. *Shipping `compiler_core`: Method Transfer, Twin-Test Self-Host, and Honest Subsets*（避免「IntNet = 出货编译器」误读）
3. 中文：《UJS：引用 UNISA 方法核的闭合 JS 子集与 Web 自举》

---

## 贡献点（对外可说的）

- **方法迁移（不重证）**：P-8/3/5/1 与 P-2 样板在 A；B 只 [cite A] + 定理名；**无 UJS 专用 Lean**；不暗示 A 卸责自动覆盖 UJS walker / M3。
- **两条脊**：构造/jtape（表→IntNet）∥ **M3 出货编译器**（手写子集、**mainBody** stage2≡ / body≡）——同纪律、不同产物；勿写成「Web 编译器 = 表网络」。
- **UJS-1（规格）** vs **UJS-1_ship（M3 已交付面，v18）**：ASCII 字面量 0–255、无转义（拼接可更长）；缺 `fn`/转义/UTF-8 等须明说。
- **产品面**：Pages/ship · `wasm_run` / UXE；默认 `compiler_core.wasm`；Python 非 ship 必经。
- **门禁分层**（对齐 prd #4）：`ujs.sh` · `ujs2wasm_compiler.sh` · `uxe_ship_js` · UXE 另门；tinyvm 可选 **validate** 非执行孪生。
- **验收**：套件名 + 写明比较对象；**不编造数字**。

---

## 与 Paper A 的分工

| | Paper A | Paper B |
|---|---|---|
| 证明 | 存在性、决策列表、同余、reloc P-2 样板（Lean+枚举） | **引用**；IC/wasm **表阶段**交 P-2 模板义务；M3 用字节/fold 门禁 |
| 语言 | C 子集 / 自举 | UJS-1 规格 + UJS-1_ship 出货面 |
| 交付 | CLI shell | npm / Pages / `compiler_core` |
| 不抢功 | IntNet 构造与枚举验证本身 | 同；指 prior-art |

---

## 写作红线

- 正文校准（2026-10）：UJS **10** 表阶段 vs UNISA **18** 事实决策网（≠18 管线阶段）；unisacc 另有 **7** 段字节流契约（两层见 A / README）；`intnet.build(..., stages=ujs.STAGES)`；`irsel` 仅 `acc`、非全线 `ask`；|K| 例与 wasm 出货对比见 [`ujs-paper.md`](ujs-paper.md) §4。
- 不把 A 的 Lean 证明抄进 B 当新定理。
- 不写假 latency；精确性 = 契约 + 绿套件；fold want 只引自 `expect.json`。
- 出货路径写 **core**，勿仍把 Python `web-build` 写成唯一真源（它是 path-A 开发/引擎残线）。
- 不把 M3 自举说成 IntNet 实例；不把 UJS-1_ship 说成全表 UJS-1。
- 不把全量 `ujs2wasm` corpus（含 `str`/`arrow`/…）写成 core 已覆盖。
- 规格与门禁以 `ujs/prd.md` 为准。
- 对 Paper C：B = **可复现门禁下的迁移实例**，非「全 walker Lean 闭合」。
- **unisacc↔UJS 分层横向对照**（七流×18 网 vs M3×10 网、构造臂不进出货口诀、管线按*角色*对齐非同名等同）：见 `research/ujs-paper.md` **Appendix D**；§4 仅一句口诀+指针。
- **Paper B 初心（2026-10-18）**：见 [`paper-b-intent-decision-20261018.md`](paper-b-intent-decision-20261018.md)；**暂定 B+m4（A 种子）**；M3 / 出货面不是 B 的主张中心，勿写成主贡献；`ujs-paper.md` 主张措辞待 m4 最小合同草案后再改。

## 温故检查（改摘要前过一遍）

- [ ] 两脊命名是否仍分清？
- [ ] §4 板块表是否仍对齐 `ujs/ARCHITECTURE.md`？
- [ ] §6.2 探针表是否与 `ujs2wasm_compiler.sh` 循环一致？
- [ ] Appendix C 陷阱是否仍成立？
- [ ] UXE 是否仍标「另门」？
- [ ] 是否误把「下一刀」写成新 bytecode VM，而非 UJS-1_ship / 可移植 wasm 宿主？

---

## 一句话定位

**A 钉死方法核；B 证明同一纪律能支撑闭合动态语言的构造脊 + Web 出货自举脊，并开箱交付——在诚实子集与分层门禁之内。**
