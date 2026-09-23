# UJS: Exact-by-Construction Table Networks for a Closed JavaScript Subset on the Web

**Status:** workshop / short-paper draft (companion to Paper A)  
**Language:** English body; Chinese abstract optional  
**Repo artifact:** `ujs/` · acceptance: `./tests/ujs.sh`

---

## Abstract

We present **UJS**, a closed JavaScript subset (**UJS-1**) whose compile-time decisions are realized by the same *construct-don't-train* discipline as UNISA SH [cite A]: finite gold tables are compiled into an integer IntNet kernel; shipping accuracy is exact by construction and verified by full key-space enumeration (P-1 / P-2 / P-3). Where Paper A targets a C self-hosting shell, UJS applies the method to a *dynamic* language pipeline—lex, parse, scope/type/shape, IR selection, **inline-cache (IC)** stubs, and WebAssembly lowering—and ships a browser/Node/Bun product whose public API is `wasm_run(code, G, L)`. The semantic spine is **jtape**; fold checks equate the jtape VM with a WasmProgram interpreter (and, when present, a host wasm instance). An IC stage (`shape × op × guard → stub_kind`) is optional for performance but must preserve results (`icfold`). We do not report invented throughput numbers; we claim qualitative properties already gated by the repository suite `tests/ujs.sh` (acc, fold, icfold, front parity, ship, in-page `wasm_run`).

**摘要（可选）**  
UJS 将 UNISA SH [cite A] 的「由 gold 表构造整数网络、全域枚举保证精确、禁止低置信回退」方法迁到闭合 JS 子集 UJS-1，并交付 Web 产品 API `wasm_run`。语义真源为 jtape；IC 与 wasm lower 为经典控制流上的有限决策表。验收绑定既有套件名称，不虚构实验数字。

---

## 1. Introduction

Neural components in compilers and runtimes are usually *trained* heuristics: they may speed up common cases but correctness rests on a classical fallback (learned indexes [Kraska et al. 2018], MLGO-style guidance [Trofin et al. 2021]). A parallel line *constructs* networks from discrete objects so that equivalence is a theorem or a finite check (Omlin & Giles 1996; Tracr [Lindner et al. 2023]; enumeration verification [Jia & Rinard 2021]). UNISA SH [cite A] takes the constructive path for a C shell: **Shell = inferencer + executor + model data**, weights built from gold tables, shared integer IntNet, and P-1/P-2/P-3 as hard contracts (oracle returns class names only; every ask key is in a closed set $K_s$; $\forall k: \mathrm{argmax}(N)=G$).

**UJS** asks whether the same method transfers to a language that *looks* dynamic—objects with finite shapes, operators guarded by runtime tags, and a web delivery surface—without opening the ECMAScript universe. We define **UJS-1**: a closed subset (literals, `let`/`const`, control including `switch`, functions/arrows, rest and call-site spread, list/dict values, lexical capture of an explicit name set). Outside the tables, rejection is permanent: no `eval`, prototypes, `this`/`new`/`class`, async, or RegExp engine.

The product is not a research REPL alone: `ujs/package.json` exports ESM `bootRuntime` / `wasm_run` for browser, Node, and Bun; the shipped runtime blob is **`ujs_full.wasm`**. Python under `ujs/construct/` is the **constructor** (gold, walk, build, ship)—not an application dependency.

**Companion framing.** Paper A [cite A] develops the method and the C self-host story. This paper (Paper B) reuses the construction algebra and IntNet kernel, separates gold/jtape/IC/wasm corpora, and evaluates web-facing fold and parity claims.

---

## 2. Contributions

1. **Method transfer to a closed dynamic subset.** Same Shell proposition and P-discipline as [cite A], applied to UJS-1 stages including **IC** (`shape × op × guard`) and wasm instruction selection/encoding/relocation.
2. **jtape as semantic source of truth**, with **fold**: jtape VM ≡ WasmProgram interpretation (host `.wasm` as a third side when exercised).
3. **Web product surface**: `wasm_run(code|Fn, G, L)` with explicit globals/locals; no in-page training; construct via `python3 -m ujs web-build`.
4. **Cross-host front parity**: Python front image ≡ JS front image on a fixed probe set (suite: front parity).
5. **Ship gate**: kit includes MANIFEST with per-stage `exact`, weights, native VM sources, `web/ujs_full.wasm`, and package entry (suite: ship)—without claiming compression wins over tables [cite A; cf. Boniol et al. 2026 on ACAS Xu].

Non-claims (aligned with [cite A] and `research/prior-art.md`): we do not claim novelty of table→MLP construction (Tracr / Omlin–Giles folklore); we do not claim enumeration verification itself is new (Jia & Rinard); we do not claim UJS-1 is ECMAScript.

---

## 3. Related Work (sketch)

| Line | Relation to UJS |
|---|---|
| **Tracr** [Lindner et al. 2023] | Finite-domain lookup → MLP: same *kind* of construction as our table nets; goal is interpretability lab, not a shipping JS→wasm product. |
| **ACAS Xu + Reluplex + Jia&Rinard** | Networks approximating continuous tables need verification effort; enumeration after quantization is prior art. Our $K_s$ are discrete finite products by design, so P-3 is a decision procedure on the closed key set [cite A]. |
| **Boniol et al. 2026 (BDD compression)** | Exact alternatives to approximate NN compression; we do **not** sell size reduction as the thesis. |
| **Learned indexes / Bloom** | Correctness via last-mile or backup filters; our contracts forbid gold/logit fallback at ask time (P-1). |
| **MLGO / production ML in compilers** | Heuristic replacement where any choice may be legal; our stages are *total functions on closed keys* with SHIP_ACC = 1.000. |
| **UNISA SH** [cite A] | Method + C self-host; UJS is the dynamic-language / web companion. |

Full adversarial survey: repository `research/prior-art.md`.

---

## 4. System Overview

```
src ──► walk (lex…irsel [+ IC]) ──► Fn(jtape, meta)
              │ oracle.ask(stage, key)     │
              ▼                            ▼
         constructed IntNet          run / wasm_run
         (embed→gemv→ReLU→gemv→argmax)     │
                                           ├─ jtape VM
                                           ├─ WasmProgram interpret
                                           └─ host wasm (optional)
```

- **Classic code:** walker, environments, stacks, stub assembly, wasm section layout [T-1].
- **Network:** last-mile table selection only [T-2]; gold is annotation and verifier; no low-confidence ship path [T-3].
- **Kernel:** identical integer path to unisa [T-4][K-1] [cite A].
- **API anchor:** `wasm_run` [T-5][A-1]; `compile` → cacheable `Fn`; execution binds explicit `G`/`L`.

Stages (all must be exact at ship): `lex`, `parse`, `scope`, `type`, `shape`, `irsel`, `ic`, `isel`, `enc`, `reloc`.

---

## 5. Method

### 5.1 Construction and contracts (shared with Paper A)

Weights are produced by `build-weights` from FULL gold—not by training. Determinism: no RNG at inference; argmax ties take the least class index; same source + weights ⇒ byte-identical Fn/wasm blob [D-*]. Proof discipline [cite A]:

- **P-1** Oracle returns class names only; no logits; no gold fallback in the ask path.
- **P-2** Every `ask` key ∈ $K_s$.
- **P-3** Per-stage enumeration: $\forall k \in K_s:\ \mathrm{argmax}(N(k))=G(k)$.
- **P-5** Net-driven ≡ gold-driven under P-1.
- **F-3** SHIP_ACC = 1.000 or refuse ship.

### 5.2 UJS-1 language (closed)

Values: `null | bool | i64 | f64 | str | list | dict | fn | tup`. Name resolution: locals → lexical outer → globals; miss is hard error. Closures capture a compile-time closed set of names from $G$ ∪ outer bindings. Dict shapes are finite `shape_id`s—no prototype chain.

### 5.3 jtape and fold

**jtape** is the target-independent instruction stream (analogue of unisa tape) and the semantic truth [TP-1]. Fold [X-4][LW-2][LW-3]: disagreement between jtape VM and WasmProgram (or host instance) is a bug, not a confidence band.

### 5.4 Inline cache stage

Hot sites query `ask(ic, (shape, op, guard)) → stub_kind` from a finite stub catalog [IC-1][IC-2]. Deopt back to the interpreter is classical control flow [IC-4]. **icfold** [IC-3][X-3]: IC on/off must agree on the same `(fn, G, L)`.

### 5.5 Wasm lower and product

`isel` / `enc` / `reloc` go only through the oracle [LW-4]. Constructor CLI builds `ujs_full.wasm` and demos; `js2wasm` emits standalone UJS-1 programs. Page/Node loads ESM + wasm; training never runs in the page.

### 5.6 Front parity

The in-browser/JS `compiler.js` front must emit the same packed image as the Python `compile_src` path on shared probes—bridging constructor and product without dual semantics.

---

## 6. Evaluation Claims (suite-backed, qualitative)

We claim properties checked by `./tests/ujs.sh` (and the CLI commands it wraps). **No fabricated latencies, sizes, or accuracy decimals beyond the contractual 1.000 ship gate.**

| Claim | Suite / gate | What “pass” means |
|---|---|---|
| Per-stage exact nets | `acc` [X-1] | FULL gold; SHIP_ACC discipline |
| API / language probes | `wasm_run` probes [X-6] | arith, strings, list/dict, while, switch, functions, rest |
| Semantic fold | `fold` [X-4] | jtape ≡ WasmProgram on exercised programs |
| IC preserves semantics | `icfold` [X-3] | IC on ≡ IC off |
| Diff vs reference | `difftest` [X-2] | agree with net-free reference interpreter |
| Deterministic compile | `determinism` [X-5] | two compiles → identical bytes |
| Ship kit integrity | `ship` [X-7] | MANIFEST `exact` stages; includes `ujs_full.wasm`, `wasm_run.js`, package.json, samples |
| Constructor ≡ page front | **front parity** | Python packed image ≡ JS `compile` image on probe set |
| Product path | **in-page `wasm_run`** | Node loads `ujs_full.wasm` via `bootRuntime`; probes incl. G/L binding, spread, arrows |
| Package contract | npm entry check | exports `bootRuntime` / `wasm_run` |

Optional CI/context: `js2wasm` and full demo wasm instantiate under Node when available—still pass/fail, not benchmark tables.

**Optional qualitative artifact (not a metric).** The repository ships a static playground (`npm run demo`), an UXE Host-ABI demo under `ujs/web/engine/demo/` (UJS-1 `sim.ujs` via `wasm_run`, binary render packet to the browser host), and a Three.js contrast page under `ujs/web/game/`. These illustrate the product loop without contributing latency or accuracy claims beyond the suite table above.

**Relation to Paper A.** Enumeration and IntNet proofs live primarily in [cite A]; this paper’s evaluation emphasizes *language/product* obligations (IC, fold, front parity, in-page API) that C self-host does not cover.

---

## 7. Discussion and Limitations

- **Closed language.** UJS-1 is not a migration path to full ES; openness would break finite $K_s$ and P-3.
- **IC is not learning.** Stub choice is another constructed table; novelty is packaging IC into the same oracle discipline, not discovering stubs by gradient descent.
- **Python constructor vs JS product.** Parity tests reduce dual-implementation risk; they do not replace proofs in [cite A].
- **Web without self-host thesis.** Paper A’s bootstrap/self-host contributions are out of scope here; UJS parallel-delivers web while C-line PRD retains “no Web” for that product [prd §14].

---

## 8. Conclusion

UJS shows that the UNISA SH method [cite A]—constructed integer table networks, closed keys, enumeration, no ask-time fallback—extends to a closed JavaScript subset with IC and wasm lowering, and ships as `wasm_run` for browser and server JS runtimes. Correctness obligations are operationalized as named suites (`acc`, `fold`, `icfold`, front parity, `ship`, in-page `wasm_run`) rather than statistical test scores.

---

## References (incomplete draft list)

- [cite A] Companion manuscript / technical report: *UNISA SH* (exact-by-construction table networks; C self-host). Unpublished or under submission; cite as companion to this work.
- Boniol et al. Compressing ACAS-Xu Lookup Tables with Binary Decision Diagrams. NFM 2026.
- Jia & Rinard. Verifying Low-dimensional Input Neural Networks via Input Quantization. SAS 2021.
- Julian et al. Policy Compression for Aircraft Collision Avoidance Systems. DASC 2016.
- Katz et al. Reluplex. CAV 2017.
- Kraska et al. The Case for Learned Index Structures. SIGMOD 2018.
- Lindner et al. Tracr: Compiled Transformers as a Laboratory for Interpretability. NeurIPS 2023.
- Omlin & Giles. Constructing Deterministic Finite-State Automata in Recurrent Neural Networks. JACM 1996.
- Trofin et al. MLGO. 2021.
- Repository: `research/prior-art.md`, `ujs/prd.md`, `tests/ujs.sh`.

---

## Outline map (for expansion)

| § | Expand with |
|---|---|
| 4 | Diagram of stages ↔ gold columns; Fn serialization |
| 5.3 | jtape opcode families (pointer only; no full ISA dump in short paper) |
| 5.4 | One stub_kind example (shape×load) without claiming perf |
| 6 | Screenshot/pointer to playground + `ujs/web/engine/demo/` (Host ABI artifact, not number) |
| App. | Mapping UJS clause IDs (T/L/IC/X) ↔ test names |

---

*Draft only. Do not invent metrics. Cross-cite Paper A at every shared-method claim marked [cite A].*
