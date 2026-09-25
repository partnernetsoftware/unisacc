# UJS: Exact-by-Construction Table Networks for a Closed JavaScript Subset on the Web

**Status:** workshop / short-paper draft (companion to Paper A)  
**Language:** English body; Chinese abstract optional  
**Repo artifact:** `ujs/` (layout: `seed/` · `weights/` · `iterate/` · `practice/` — see `ujs/ARCHITECTURE.md`; legacy `construct`/`core`/`uxe` paths are compatibility symlinks)  
**Gates (layered; see `ujs/prd.md` #4):**  
- **Language / construct / path-B:** `./tests/ujs.sh` (includes `ujs2wasm` → step)  
- **M3 / P0 product compile:** `./tests/ujs2wasm_compiler.sh`  
- **Ship-js contract:** `./tests/uxe_ship_js.sh`  
- **UXE unmanned:** `npm run test:uxe:*` — **separate** from language; flaky UXE must not block language green  
**Method proofs:** live in Paper A [cite A] (`research/formalization-roadmap.md`, Lean L0–L3)—this paper does not re-prove IntNet existence, and **ships no UJS-specific Lean**.

---

## Abstract

We present **UJS**, a closed JavaScript subset (**UJS-1**) that *transfers* the construct-don't-train discipline of UNISA SH [cite A]: finite gold tables become an integer IntNet; shipping accuracy is exact by construction and checked by full key-space enumeration. **Method-kernel obligations (P-8 / P-3 / P-5 / P-1, and a P-2 sample) are discharged in [cite A]** (enumeration + Lean theorems such as `naive_exact`, `decision_list_exact`, `cong_of_pointwise`, `asks_subset_K`); Paper B only *instantiates* them on a dynamic-language / Web pipeline and adds product gates.

Where Paper A targets a C self-hosting shell, UJS applies the same oracle discipline on the **construct / jtape spine** (lex…irsel [+ IC], wasm lower when tables ship)—and ships browser/Node/Bun surfaces whose **product compile / step path** no longer requires Python. Separately, the **M3 spine** ships a *hand-written* UJS subset compiler (`compiler.ujs` → `compiler_core.wasm`) with stage2 ≡ stage1 and game bodies ≡ stage0. M3 is twin-test evidence for Python-free ship, **not** a claim that the shipping compiler is itself an IntNet. Claims are suite names already in the repository; we invent no throughput numbers.

**摘要（可选）**  
UJS 把 UNISA SH [cite A] 的构造法迁到闭合 JS：方法核在 A 卸责；B 只证明**迁移 + 可命名产品门禁**。验收有两条脊——**构造/jtape**（表→IntNet）与 **M3 出货编译器**（手写子集自举、字节一致）；后者不是「Web 编译器 = 表网络」。出货默认 `compiler_core.wasm`；Python 限于构造臂，非 Pages/ship 必经。语言子集写 **UJS-1_ship（M3 已交付面）**，勿与全表 UJS-1 规格等同。

---

## 1. Introduction

Neural components in compilers and runtimes are usually *trained* heuristics with classical fallbacks. A parallel line *constructs* networks from discrete objects so equivalence is a finite check (Omlin & Giles 1996; Tracr; Jia & Rinard). UNISA SH [cite A] takes that path for C: **Shell = inferencer + executor + model data**, with hard contracts P-1/P-2/P-3 and a now machine-checked method kernel (Lean L0–L3 in the companion repo).

**UJS** asks whether the same method transfers to a language that *looks* dynamic—finite shapes, tag-guarded operators, Web delivery—without opening ECMAScript. We define **UJS-1**: a closed subset (full language table in `ujs/prd.md` / archive). Outside the tables, rejection is permanent. **What ships today under M3** is a *growing* subset we call **UJS-1_ship** (§5.2); it is not yet the full UJS-1 table.

**Companion framing.** Paper A [cite A] owns construction algebra, IntNet soundness, enumeration-as-decision-procedure, and C self-host. This paper reuses that kernel by citation, then evaluates (i) closed JS + IC + wasm lower on the construct spine, (ii) jtape/fold and front parity where applicable, (iii) **M3 self-host of a UJS→wasm *product* compiler** and Python-free ship builders. Framing for Paper C: B is a **reproducible, gate-backed transfer instance**—not a claim that every UJS walker is Lean-closed or that UJS-1 is fully emitted by core.

---

## 2. Contributions

1. **Method transfer (not re-proof).** Same Shell proposition and P-discipline as [cite A], applied to UJS-1 *table* stages including **IC** (`shape × op × guard`) and wasm `isel`/`enc`/`reloc` when those tables ship. Novel writing is the *transfer* and product packaging; IntNet proofs stay in A.
2. **Two spines, one discipline.** (a) **jtape / fold / acc / icfold** — construct path; method acceptance. (b) **M3** — `compiler.ujs` → `compiler_core.wasm`, stage2 ≡ stage1, sim/drone body ≡ stage0 — product compile without Python emit. Spines share P-style honesty; they are **not** the same artifact.
3. **Web product surface**: `wasm_run` / UXE Pages; default compile path **`compiler_core.wasm`** (P0); Python construct is not a ship dependency (`web-build` remains a *dev / engine* path, not Pages emit).
4. **Cross-host front parity** where dual fronts still exist; parity reduces drift, it does not replace [cite A].
5. **Layered ship gates** without selling compression over tables [cite A; Boniol et al.].

Non-claims (aligned with [cite A] / `prior-art.md`): no novelty of table→MLP construction; no novelty of enumeration verification; UJS-1 ≠ full ES; **UJS-1_ship ≠ full UJS-1**; no CompCert-level claim that gold ≡ JS semantics (external referees / fold only); **M3 compiler ≠ constructed IntNet**.

---

## 3. Related Work

We share adversarial positioning with [cite A] / `research/prior-art.md`; here only what matters for the *transfer* story.

**Constructed nets, not trained heuristics.** Omlin & Giles (1996) and Tracr (Lindner et al., 2023) already show finite discrete maps can be *compiled* into network weights. Paper A restates existence (P-8) as folklore and discharges soundness/enumeration/congruence in Lean. UJS does **not** re-claim that folklore; it asks whether the *same contracts* survive a closed dynamic-looking language and a Web ship path.

**Verification style.** Reluplex-style SMT on continuous nets is the wrong tool once keys are finite products: P-3 is a decision procedure by enumeration [cite A; Jia & Rinard 2021]. Boniol et al. (2026) compress exact decision procedures with BDDs; we explicitly **do not** sell size≪table as the thesis.

**Learned compiler components.** MLGO / learned indexes keep classical fallbacks or optimize among legal choices. Our *table* stages are total functions on closed $K_s$ with P-1 (opaque `ask`). The M3 product compiler is classical structure code under twin-tests—closer to ordinary self-hosting than to MLGO.

**Self-hosting and Web runtimes.** Stage2 ≡ stage1 is the packaging discipline of self-hosting compilers applied to UJS→wasm *product* emit. It does **not** discharge A's L0–L3 for UJS walkers, and it does **not** mean the shipping compiler is an IntNet. Browser/Node surfaces (`wasm_run`, UXE Host) are product packaging around the same theory split as [cite A]'s shell vs Web non-goal for C.

Full survey and negative evidence: `research/prior-art.md`.

---

## 4. System Overview

The living tree mirrors Paper C’s seed / reusable-data / iterate split (`ujs/ARCHITECTURE.md`):

| Panel | Path | Role |
|---|---|---|
| **Seed** | `ujs/seed/` | Python construct (`seed/construct/`), C stage0 (`seed/stage0/`), path-A VM sources |
| **Weights** | `ujs/weights/` | *Artifacts only* (e.g. `ujs_ic_net.c`); generators stay in seed |
| **Iterate** | `ujs/iterate/` | M3: `compiler.ujs`, `compiler_core.wasm`, `compile.mjs` |
| **Practice** | `ujs/practice/` | `core/` (`wasm_run`, path-A), `uxe/` (Host/Pages), `web/` |

```
src ──► walk (lex…irsel [+ IC]) ──► Fn(jtape, meta)     [construct / bytecode spine]
              │ oracle.ask(stage, key)
              ▼
         constructed IntNet  [cite A: same kernel]
              │
              ▼  (optional) emit_wasm / path-B tools — method & full-surface emit

src ──► iterate/compiler.ujs (M3) ──► compiler_core.wasm
              │                        ──► splice RT ──► \0asm
              ▼                        [product compile spine; not IntNet]
         stage2 ≡ stage1 · game body ≡ stage0
```

- **Classic code:** seed walkers / stage0; **and** iterate M3 sources.
- **Network:** last-mile table selection on the construct spine only; proofs in [cite A]; weight *bytes* land in `weights/` (and shared root `weights/` for A).
- **Product compile:** `ujs/iterate/compile.mjs` (symlink `ujs/compile.mjs`) defaults to `compiler_core.wasm`; stage0 `compiler.wasm` is bootstrap and body≡ oracle. **Do not read M3 as “the Web compiler is driven by table nets.”**
- **API:** `practice/core` `wasm_run` / `practice/uxe` Host; Pages load game wasm + host, not a Python emit at request time.
- **Dev residual:** `python3 -m ujs web-build` still builds path-A `ujs_full` under `practice/core` when forced; it is **out of** the P0 ship path.

Stages that remain table-shaped when present: `lex`, `parse`, `scope`, `type`, `shape`, `irsel`, `ic`, `isel`, `enc`, `reloc` (exact at ship when that table ships). M3 expands a **classical** AST→wasm subset independently of baking those golds.

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

UJS inherits these by citation. **B’s extra P-2 obligation:** each new *table* stage (especially **IC** and wasm `isel`/`enc`/`reloc`) must eventually supply an A-style domain-closure sample or keep the runtime assert + suite pressure. **M3’s classical compiler does not inherit a Lean P-2 proof**; its honesty is stage2≡ / body≡ / fold suites.

### 5.2 UJS-1 language (closed) vs UJS-1_ship

**UJS-1 (spec):** values `null | bool | i64 | f64 | str | list | dict | fn | tup`; locals → lexical outer → globals; permanent reject outside tables (no `eval`, prototypes, `this`/`new`/`class`, async, RegExp engine). Full clause table: `ujs/prd.md` / `archive/prd-v1.0.md`.

**UJS-1_ship (M3 / Pages path, as of compiler v17):** growing classical subset sufficient for Asteroid + drone and the self-hosting compiler—control (`if` / `else if` / `while` / …), i64/f64 arith, list/dict/index/`setidx`, unary `-`/`!`, `&&`/`||` (i64 short-circuit), **short str lit + concat + return**, globals inject / `host_*` / `run_step`. **Still out of M3 ship:** general long `str` / `fn`, baked gold/catalog, byte-identical module vs full Python `emit_wasm` (see `ujs/prd.md` M2 residual).

Papers and release notes must say **which** face is meant.

### 5.3 jtape, fold, and M3 self-host

**jtape** remains the semantic spine for the bytecode VM / construct path; fold equates jtape VM ↔ WasmProgram (host wasm when exercised).

**M3** is the second spine: `compiler.ujs` compiled by stage0 yields `compiler_core.wasm`; recompilation yields stage2 ≡ stage1; game `sim.ujs` / `drone.ujs` main bodies ≡ stage0. Twin-test for **Python-free product compile**, not a re-proof of IntNet, and not a claim that stage0 C and stage1 UJS are table-constructed.

### 5.4 Inline cache stage

Hot sites: `ask(ic, (shape, op, guard)) → stub_kind` from a finite stub catalog. Deopt is classical control flow. **icfold:** IC on/off agree. IC keys are another finite product—same P-2/P-3 shape as [cite A]; stub choice is not learned by SGD. Lives on the **construct** spine.

### 5.5 Wasm lower and product

`isel` / `enc` / `reloc` go through the oracle when those tables are on the construct path [cite A]. Ship builders (`build-*-pages.mjs`) call `compile.mjs` → core; they must not shell out to `python3` emit on the product path (gate: `ujs2wasm_compiler.sh`).

### 5.6 Front parity

Where a JS front and a Python front both exist, packed images must match on shared probes. Parity is anti-drift, not a substitute for [cite A].

---

## 6. Evaluation Claims (suite-backed, qualitative)

**No fabricated latencies or accuracy decimals** beyond contractual 1.000 where `acc` applies. Fold “want” values are the checked oracles in `tests/ujs2wasm/expect.json`—suite contracts, not marketing scores.

### 6.1 Layered gates

| Claim | Suite / gate | Pass means |
|---|---|---|
| Method kernel | [cite A] Lean + `unisa acc` | Not re-run as B’s novelty |
| UJS table exactness (when tables ship) | `acc` / construct via `ujs.sh` | FULL gold; SHIP_ACC |
| Construct fold / icfold / front parity | `tests/ujs.sh` (where enabled) | agree / identical images |
| Full path-B emit corpus | `tests/ujs2wasm.sh` (under `ujs.sh`) | includes probes **outside** UJS-1_ship (e.g. `str`, `arrow`, …) via Python/`emit` tools |
| M3 compiler self-host + P0 | `ujs2wasm_compiler.sh` | stage2≡stage1; sim/drone body≡stage0; default bridge=`compiler_core.wasm`; no ship `emit_wasm` / no A-core copy |
| UJS-1_ship fold on **core** | same | §6.2 corpus + inject step |
| Ship-js contract | `uxe_ship_js.sh` | `sim.wasm` present; Pages load path B |
| UXE unmanned | `npm run test:uxe:*` | **separate gate**; see `ujs/prd.md` #4 |

### 6.2 UJS-1_ship fold corpus (on `compiler_core`)

Exercised by `ujs2wasm_compiler.sh` with bridge forced to core (names ⊂ `tests/ujs2wasm/corpus/`):

| Probe | Exercises (abbrev.) | `expect.json` |
|---|---|---|
| `arith` | i64 ops | 7 |
| `fact` | while / locals | 120 |
| `branch` | if/else | 10 |
| `f64_arith` | f64 | 25 |
| `list` / `setidx` / `list_f64` | list + index write | 23 / 13 / 20 |
| `dict` | dict / dot | 3 |
| `globals_fold` | globals | 11 |
| `unary_minus` | unary `-` | 3 |
| `elseif` | else-if | 20 |
| `logic` | `!` · `&&` · `||` (i64) | 7 |

Plus `setidx_globals` (host inject + `run_step`) and Asteroid/drone ship emit without `python3`.  
**Not** on the core gate today (still construct / full emit): e.g. `str`, `arrow`, `forof`, `ternary`, `nullish`, `blockarrow`, `sum`, `switch`, `setidx_loop`—presence in `expect.json` must not be read as UJS-1_ship coverage.

**Relation to Paper A.** IntNet / enumeration / congruence / P-2 sample live in [cite A]. B emphasizes language self-host *gates*, Web ship path, IC packaging on the construct spine, and honest subset bounds.

---

## 7. Discussion and Limitations

- **Closed language.** Opening to full ES breaks finite $K_s$ and P-3 on the construct spine.
- **Two spines.** Overclaiming “the compiler is a table network” collapses product twin-tests with method proofs; keep them named separately.
- **UJS-1_ship is incomplete.** Expanding M3 (short str, fn, …) is engineering schedule, not a silent widening of the paper’s ship claim.
- **IC is not learning.** Another constructed table under the same oracle (construct spine).
- **P-2 remains the hard transfer debt.** A’s Lean sample is a template; UJS *table* walkers still rely on asserts + suites. M3 adds suite/byte obligations, not Lean.
- **Python constructor.** Allowed to shrink only; product compile must not grow new Python-only ship edges. P1 (construct → unisacc) is intentionally deferred (`ujs/prd.md`).
- **Web vs A’s “no Web” for the C product.** Intentional split: A = C shell; B = Web/JS practice of the *same theory*.
- **Layout is part of the claim hygiene.** Keeping generators in `seed/` and weight *bytes* in `weights/` (A/C rule) prevents rereading `gold.py` as “the reusable net.”
- **Execution hosts are not the next IntNet.** Browser/Node already run `\0asm`; a portable wasm interpreter (e.g. companion `tinyvm`) would be another *practice* executor / twin oracle—not a third language spine and not a substitute for M3 or for construct tables.

---

## 8. Conclusion

UJS shows that the UNISA SH method [cite A] extends to a closed JavaScript *spec* with IC and wasm lowering on the construct spine, and that the **product compile path can self-host in UJS** (`compiler_core.wasm`) under named twin-tests—without re-deriving IntNet theory and without equating that compiler to an IntNet. Method proofs stay in A; B’s claims are transfer, layered gates, UJS-1_ship honesty, and Web delivery—operationalized as suite names, not statistical scores.

---

## References

- [cite A] Companion: *UNISA SH / 表即网络* — `research/unisacc-paper.md` §3.4 · `research/formalization-roadmap.md` (Lean L0–L3).
- Omlin & Giles, *Constructing deterministic finite-state automata in recurrent neural networks*, JACM 1996.
- Lindner et al., Tracr (2023); Jia & Rinard (SAS 2021); Boniol et al. (2026); Kraska et al. (2018); Trofin et al., MLGO (2021); Shalev-Shwartz et al. (P-8b family)—detail in `research/prior-art.md`.
- Living product / gates: `ujs/prd.md` · layout: `ujs/ARCHITECTURE.md` · Host: `ujs/practice/uxe/HOST_ABI.md` (symlink `ujs/uxe/`) · outline: `research/ujs-paper-outline.md`.
- Suite oracles (not marketing scores): `tests/ujs2wasm/expect.json` · core fold list in §6.2.

---

## Appendix A — Suite name checklist

**Language / construct:** `acc` · `fold` · `icfold` · `difftest` · `determinism` · front parity · in-page `wasm_run` · `ujs.sh` · `ujs2wasm.sh` (full corpus ≠ ship; optional tinyvm validate)  
**Product / M3 / P0:** **`ujs2wasm_compiler`** · ship builders without `emit_wasm` · §6.2 core probes · optional **tinyvm `module validate`** on fold+sim/drone (`UJS_REQUIRE_TINYVM=1` to require)  
**Ship-js:** `uxe_ship_js`  
**UXE (separate):** `test:uxe:*`

## Appendix B — Non-goals

Full ES · equating UJS-1_ship with full UJS-1 · equating M3 with IntNet · reading full `expect.json` as core coverage · in-page training · claiming size≪table as main result · inventing latency tables · re-proving P-8/P-3 in B · CompCert-level gold≡JS · UJS-specific Lean milestone.

## Appendix C — Warm-review (温故) map

| Reader trap | Correction |
|---|---|
| “Web compiler = table network” | M3 is classical subset + twin-tests; IntNet lives on construct spine |
| “language gate = only `ujs2wasm_compiler`” | Layered: `ujs.sh` + compiler + `uxe_ship_js`; UXE separate |
| “UJS-1 done because v17 green” | UJS-1_ship ⊂ UJS-1; long `str`/`fn`/… still construct-side |
| “B re-proves IntNet” | [cite A] only; B has no UJS Lean |
| “`web-build` is the ship path” | path-A residual; P0 ship uses `compiler_core` + `sim.wasm` |
| “HOST_ABI / prd still say engine.wasm ships” | Was stale; living docs now: ship = `sim.wasm` + Host; `ujs_full` optional |
| “`construct/` holds the reusable weights” | Generators are `seed/`; artifacts are `weights/` (A/C rule) |
| “need a new UJS bytecode VM next” | Product path already emits `\0asm`; companion `tinyvm` is validate/execute *oracle*, not a third IR |

---

*Draft. Cross-cite [cite A] at every shared-method claim. Numbers only from green suites. Keep two spines and subset bounds in every abstract rewrite.*
