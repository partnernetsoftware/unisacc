# UJS 论文（Paper B）—— 作者一页提纲

> 对应正文：`research/ujs-paper.md`  
> 伴生：Paper A = 方法核 + C 自举（Lean L0–L3 / §3.4）；本文 = **同一方法的迁移** × 闭合 JS × Web / M3 自举。

---

## 标题备选

1. **UJS: Exact-by-Construction Table Networks for a Closed JavaScript Subset on the Web**（主推）
2. *Shipping `compiler_core`: Constructed IntNets Meet a Self-Hosted UJS→Wasm Path*
3. 中文：《UJS：引用 UNISA 方法核的闭合 JS 子集与 Web 自举》

---

## 贡献点（对外可说的）

- **方法迁移（不重证）**：P-8/3/5/1 与 P-2 样板在 A；B 只 [cite A] + 定理名。
- **UJS-1 闭合语言** + **M3**：`compiler.ujs` → `compiler_core.wasm`；stage2≡stage1；默认产品编译无 Python。
- **jtape / fold**（字节码路径）与 **IC**（另一张有限表；交同类 P-2 义务）。
- **产品面**：Pages/ship · `wasm_run` / UXE；语言门与 UXE 门分离（`ujs/prd.md` #4）。
- **验收**：套件名；**不编造数字**。

---

## 与 Paper A 的分工

| | Paper A | Paper B |
|---|---|---|
| 证明 | 存在性、决策列表、同余、reloc P-2 样板（Lean+枚举） | **引用**；IC/wasm/M3 交 P-2 模板义务 |
| 语言 | C 子集 / 自举 | 闭合 JS（UJS-1）+ M3 编译器自举 |
| 交付 | CLI shell | npm / Pages / `compiler_core` |
| 不抢功 | IntNet 构造与枚举验证本身 | 同；指 prior-art |

---

## 写作红线

- 不把 A 的 Lean 证明抄进 B 当新定理。
- 不写假 latency；精确性 = 契约 + 绿套件。
- 出货路径写 **core**，勿仍把 Python `web-build` 写成唯一真源。
- 规格以 `ujs/prd.md` 为准。

---

## 一句话定位

**A 钉死方法核；B 证明同一核能支撑闭合动态语言流水线、UJS 编译器自举，并开箱交付 Web。**
