# UJS: Exact-by-Construction Table Networks for a Closed JavaScript Subset on the Web

**Status:** workshop / short-paper draft (companion to Paper A)  
**Language:** English body; Chinese abstract optional  
**Repo artifact:** `ujs/` · language gate: `./tests/ujs2wasm_compiler.sh` · product: `./tests/ujs.sh`  
**Method proofs:** live in Paper A [cite A] (`research/formalization-roadmap.md`, Lean L0–L3)—this paper does not re-prove IntNet existence.

---

## Abstract

We present **UJS**, a closed JavaScript subset (**UJS-1**) that *transfers* the construct-don't-train discipline of UNISA SH [cite A]: finite gold tables become an integer IntNet; shipping accuracy is exact by construction and checked by full key-space enumeration. **Method-kernel obligations (P-8 / P-3 / P-5 / P-1, and a P-2 sample) are discharged in [cite A]** (enumeration + Lean theorems such as `naive_exact`, `decision_list_exact`, `cong_of_pointwise`, `asks_subset_K`); Paper B only *instantiates* them on a dynamic-language / Web pipeline and adds product gates.

Where Paper A targets a C self-hosting shell, UJS applies the same oracle discipline to lex/parse/scope/type/shape, IR selection, optional **inline-cache (IC)** stubs, and WebAssembly lowering—and ships browser/Node/Bun surfaces whose stepping path no longer requires Python at product time. The semantic spine remains **jtape** where the bytecode VM is exercised; the **M3** line additionally ships a UJS-written compiler (`compiler.ujs` → `compiler_core.wasm`) with stage2 ≡ stage1 and fold bodies ≡ stage0. We do not invent throughput numbers; claims are suite names already in the repository.

**摘要（可选）**  
UJS 把 UNISA SH [cite A] 的构造法迁到闭合 JS 子集：方法核（存在性、决策列表声音性、`ask` 同余、P-2 样板）在 A 用枚举与 Lean 卸责；B 只证明**迁移 + Web/自举产品义务**。出货编译默认 `compiler_core.wasm`（M3）；Python 限于构造臂，非 Pages/ship 必经。验收绑套件名，不虚构数字。

---

## 1. Introduction

Neural components in compilers and runtimes are usually *trained* heuristics with classical fallbacks. A parallel line *constructs* networks from discrete objects so equivalence is a finite check (Omlin & Giles 1996; Tracr; Jia & Rinard). UNISA SH [cite A] takes that path for C: **Shell = inferencer + executor + model data**, with hard contracts P-1/P-2/P-3 and a now machine-checked method kernel (Lean L0–L3 in the companion repo).

**UJS** asks whether the same method transfers to a language that *looks* dynamic—finite shapes, tag-guarded operators, Web delivery—without opening ECMAScript. We define **UJS-1**: a closed subset. Outside the tables, rejection is permanent.

**Companion framing.** Paper A [cite A] owns construction algebra, IntNet soundness, enumeration-as-decision-procedure, and C self-host. This paper reuses that kernel by citation, then evaluates (i) closed JS + IC + wasm lower, (ii) jtape/fold and front parity where applicable, (iii) **M3 self-host of the UJS→wasm compiler** and Python-free ship builders.

---

## 2. Contributions

1. **Method transfer (not re-proof).** Same Shell proposition and P-discipline as [cite A], applied to UJS-1 stages including **IC** (`shape × op × guard`) and wasm `isel`/`enc`/`reloc`. Novel writing is the *transfer* and product packaging; IntNet proofs stay in A.
2. **jtape / fold** (bytecode path) and **M3 compiler self-host** (`compiler.ujs`, stage2 ≡ stage1, sim/drone body ≡ stage0) as language-line obligations A does not cover.
3. **Web product surface**: `wasm_run` / UXE Pages; default compile path **`compiler_core.wasm`** (P0); Python construct is not a ship dependency.
4. **Cross-host front parity** where dual fronts still exist; parity reduces drift, it does not replace [cite A].
5. **Ship gates** without selling compression over tables [cite A; Boniol et al.].

Non-claims (aligned with [cite A] / `prior-art.md`): no novelty of table→MLP construction; no novelty of enumeration verification; UJS-1 ≠ full ES; no CompCert-level claim that gold ≡ JS semantics (external referees / fold only).

---

## 3. Related Work (sketch)

| Line | Relation to UJS |
|---|---|
| **Tracr** | Same *kind* of finite lookup→MLP; not a shipping JS→wasm product. |
| **ACAS Xu / Reluplex / Jia&Rinard** | Continuous nets need heavy verification; our $K_s$ are discrete products, so P-3 is a decision procedure [cite A]. |
| **Boniol et al. 2026** | Exact BDD compression; we do **not** sell size as the thesis. |
| **Learned indexes / Bloom** | Backup filters; P-1 forbids gold/logit fallback at ask time. |
| **MLGO** | Heuristics among legal choices; our stages are total functions on closed keys. |
| **UNISA SH** [cite A] | Method kernel + C self-host; UJS is the dynamic/web companion. P-8b SGD unreachability is literature + A's instance, not B's open problem. |

Full survey: `research/prior-art.md`.

---

## 4. System Overview

```
src ──► walk (lex…irsel [+ IC]) ──► Fn(jtape, meta)     [bytecode / construct path]
              │ oracle.ask(stage, key)
              ▼
         constructed IntNet  [cite A: same kernel]

src ──► compiler.ujs (M3) ──► compiler_core.wasm ──► splice RT ──► \0asm   [product compile path]
```

- **Classic code:** walker / M3 compiler source, environments, wasm layout.
- **Network:** last-mile table selection only; proofs in [cite A].
- **Product compile:** `ujs/compile.mjs` defaults to `compiler_core.wasm`; stage0 `compiler.wasm` is bootstrap and body≡ oracle.
- **API:** `wasm_run` / UXE Host; Pages load game wasm + host, not a Python emit at request time.

Stages that remain table-shaped when present: `lex`, `parse`, `scope`, `type`, `shape`, `irsel`, `ic`, `isel`, `enc`, `reloc` (exact at ship when that table ships).

---

## 5. Method

### 5.1 Construction and contracts (owned by Paper A)

Weights from FULL gold via `build-weights`—not training. Discipline [cite A] §3.4:

| ID | Content | Discharged in A by |
|---|---|---|
| **P-8** | Exact net exists for any finite $G:K\to Y$ | Lean L0 `naive_exact` (folklore restated) |
| **P-3** | $\forall k:\arg\max N(k)=G(k)$ | Enumeration + Lean L1 `decision_list_exact` |
| **P-5** | Net-driven ≡ gold-driven under P-1 | Lean L2 `cong_of_pointwise` / `p5_of_p3_trace` |
| **P-1** | Opaque `ask` (class names only) | Engineering + L2 model hypothesis |
| **P-2** | Every ask key ∈ $K_s$ | Runtime assert; Lean L3 **sample** `asks_subset_K` (`reloc`); **full walker still open** |
| **P-6** | gold ≡ language semantics | External referee—not claimed in A or B |

UJS inherits these by citation. **B’s extra P-2 obligation:** each new UJS table stage (especially **IC** and wasm `isel`/`enc`/`reloc`) must eventually supply an A-style domain-closure sample or keep the runtime assert + suite pressure; we do not pretend IC is exempt.

### 5.2 UJS-1 language (closed)

Values: `null | bool | i64 | f64 | str | list | dict | fn | tup` (subset actually emitted grows with M3: today ship/game path emphasizes i64/f64/list/dict/control). Name resolution: locals → lexical outer → globals. Outside tables / subset: permanent reject (no `eval`, prototypes, `this`/`new`/`class`, async, RegExp engine).

### 5.3 jtape, fold, and M3 self-host

**jtape** remains the semantic spine for the bytecode VM path; fold equates jtape VM ↔ WasmProgram (host wasm when exercised).

**M3** adds a second spine: `compiler.ujs` compiled by stage0 yields `compiler_core.wasm`; recompilation yields stage2 ≡ stage1; game `sim.ujs` / `drone.ujs` main bodies ≡ stage0. This is the UJS analogue of A’s twin-test discipline, aimed at **Python-free product compile**, not at re-proving IntNet.

### 5.4 Inline cache stage

Hot sites: `ask(ic, (shape, op, guard)) → stub_kind` from a finite stub catalog. Deopt is classical control flow. **icfold:** IC on/off agree. IC keys are another finite product—same P-2/P-3 shape as [cite A]; stub choice is not learned by SGD.

### 5.5 Wasm lower and product

`isel` / `enc` / `reloc` go through the oracle when those tables are on the path [cite A]. Ship builders (`build-*-pages.mjs`) call `compile.mjs` → core; they must not shell out to `python3` emit on the product path (gate: `ujs2wasm_compiler.sh`).

### 5.6 Front parity

Where a JS front and a Python front both exist, packed images must match on shared probes. Parity is anti-drift, not a substitute for [cite A].

---

## 6. Evaluation Claims (suite-backed, qualitative)

**No fabricated latencies or accuracy decimals** beyond contractual 1.000 where `acc` applies.

| Claim | Suite / gate | Pass means |
|---|---|---|
| Method kernel | [cite A] Lean + `unisa acc` | Not re-run as B’s novelty |
| UJS table exactness (when tables ship) | `acc` / construct gates | FULL gold; SHIP_ACC |
| M3 compiler self-host | `ujs2wasm_compiler.sh` | stage2≡stage1; sim/drone body≡stage0; default bridge=`compiler_core.wasm` |
| Subset fold (arith…elseif, setidx_globals, …) | same | fold values + inject step |
| Ship builders Python-free | same | no `emit_wasm` / no A-core copy for Pages |
| jtape fold / icfold / front parity | `tests/ujs.sh` (where enabled) | agree / identical images |
| UXE unmanned | `npm run test:uxe:*` | **separate gate** from language (see `ujs/prd.md` #4) |

**Relation to Paper A.** IntNet / enumeration / congruence / P-2 sample live in [cite A]. B emphasizes language self-host, Web ship path, and IC packaging.

---

## 7. Discussion and Limitations

- **Closed language.** Opening to full ES breaks finite $K_s$ and P-3.
- **IC is not learning.** Another constructed table under the same oracle.
- **P-2 remains the hard transfer debt.** A’s Lean sample is a template; UJS walkers (IC, wasm lower, M3 compiler) still rely on asserts + suites until each stage gets a closure argument.
- **Python constructor.** Allowed to shrink only; product compile must not grow new Python-only ship edges.
- **Web vs A’s “no Web” for the C product.** Intentional split: A = C shell; B = Web/JS practice of the *same theory*.

---

## 8. Conclusion

UJS shows that the UNISA SH method [cite A] extends to a closed JavaScript subset with IC and wasm lowering, and that the **product compile path can self-host in UJS** (`compiler_core.wasm`) without re-deriving IntNet theory. Method proofs stay in A; B’s claims are transfer, self-host gates, and Web delivery—operationalized as named suites, not statistical scores.

---

## References (placeholders)

- [cite A] Companion: *UNISA SH / 表即网络* — construction, enumeration, Lean L0–L3, C self-host. See `research/unisacc-paper.md` §3.4 and `research/formalization-roadmap.md`.
- Omlin & Giles 1996; Lindner et al. 2023 (Tracr); Jia & Rinard 2021; Boniol et al. 2026; Kraska et al. 2018; Trofin et al. 2021 (MLGO); Shalev-Shwartz et al. (P-8b family)—as in `research/prior-art.md`.

---

## Appendix A — Suite name checklist

`acc` · `fold` · `icfold` · `difftest` · `determinism` · `ship` · front parity · in-page `wasm_run` · **`ujs2wasm_compiler`** (M2/M3/P0) · UXE probes (separate).

## Appendix B — Non-goals

Full ES · in-page training · claiming size≪table as main result · inventing latency tables · re-proving P-8/P-3 in B · CompCert-level gold≡JS.

---

*Draft. Cross-cite [cite A] at every shared-method claim. Numbers only from green suites.*
