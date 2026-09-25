# UJS 论文（Paper B）—— 作者一页提纲

> 对应正文：`research/ujs-paper.md`  
> 伴生：Paper A = 方法核 + C 自举（Lean L0–L3 / §3.4）；本文 = **同一方法的迁移** × 闭合 JS × Web / M3 自举。  
> 规格真源：`ujs/prd.md`（门禁分层、M3 子集、P0/P1）。

---

## 标题备选

1. **UJS: Exact-by-Construction Table Networks for a Closed JavaScript Subset on the Web**（主推）
2. *Shipping `compiler_core`: Method Transfer, Twin-Test Self-Host, and Honest Subsets*（避免「IntNet = 出货编译器」误读）
3. 中文：《UJS：引用 UNISA 方法核的闭合 JS 子集与 Web 自举》

---

## 贡献点（对外可说的）

- **方法迁移（不重证）**：P-8/3/5/1 与 P-2 样板在 A；B 只 [cite A] + 定理名；**无 UJS 专用 Lean**。
- **两条脊**：构造/jtape（表→IntNet）∥ **M3 出货编译器**（手写子集、stage2≡ / body≡）——同纪律、不同产物；勿写成「Web 编译器 = 表网络」。
- **UJS-1（规格）** vs **UJS-1_ship（M3 已交付面，v16）**：后者渐扩，缺通用长 str/fn 等须明说。
- **产品面**：Pages/ship · `wasm_run` / UXE；默认 `compiler_core.wasm`；Python 非 ship 必经。
- **门禁分层**（对齐 prd #4）：`ujs.sh` · `ujs2wasm_compiler.sh` · `uxe_ship_js` · UXE 另门。
- **验收**：套件名；**不编造数字**。

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

- 不把 A 的 Lean 证明抄进 B 当新定理。
- 不写假 latency；精确性 = 契约 + 绿套件；fold want 只引自 `expect.json`。
- 出货路径写 **core**，勿仍把 Python `web-build` 写成唯一真源（它是 path-A 开发/引擎残线）。
- 不把 M3 自举说成 IntNet 实例；不把 UJS-1_ship 说成全表 UJS-1。
- 不把全量 `ujs2wasm` corpus（含 `str`/`arrow`/…）写成 core 已覆盖。
- 规格与门禁以 `ujs/prd.md` 为准。
- 对 Paper C：B = **可复现门禁下的迁移实例**，非「全 walker Lean 闭合」。

## 温故检查（改摘要前过一遍）

- [ ] 两脊命名是否仍分清？
- [ ] §6.2 探针表是否与 `ujs2wasm_compiler.sh` 循环一致？
- [ ] Appendix C 陷阱是否仍成立？
- [ ] HOST_ABI §6 / prd 随身卡是否仍写 ship=`engine.wasm`？
- [ ] `uxe_ship_js` / `ujs2wasm.sh` 头注释是否标明脊？

---

## 一句话定位

**A 钉死方法核；B 证明同一纪律能支撑闭合动态语言的构造脊 + Web 出货自举脊，并开箱交付——在诚实子集与分层门禁之内。**
