# UJS: Exact-by-Construction Table Networks for a Closed JavaScript Subset on the Web

**Status:** workshop / short-paper draft (companion to Paper A)  
**Language:** English body; Chinese abstract optional  
**Repo artifact:** `ujs/` (layout: `seed/` · `weights/` · `iterate/` · `practice/` — see `ujs/ARCHITECTURE.md`; legacy `construct`/`core`/`uxe` paths are compatibility symlinks)  
**Gates (layered; see `ujs/prd.md` #4):**  
- **Language / construct / path-B:** `./tests/ujs.sh` (includes `ujs2wasm` → step)  
- **M3 / P0 product compile:** `./tests/ujs2wasm_compiler.sh`  
- **Ship-js contract:** `./tests/uxe_ship_js.sh`  
- **UXE unmanned:** `npm run test:uxe:*` — **separate** from language; flaky UXE must not block language green  
**Method proofs:** live in Paper A [cite A] (`archive/research/formalization-roadmap.md`, Lean L0–L3)—this paper does not re-prove IntNet existence, and **ships no UJS-specific Lean**.

---

## Abstract

We present **UJS**, a closed JavaScript subset (**UJS-1**) that *transfers* the construct-don't-train discipline of UNISA SH [cite A]: finite gold tables become an integer IntNet; exactness on table nets is checked by full key-space enumeration (`acc`). **Paper B’s claim center** is the **constructed IntNet / table nets / (target) stream-derivation pipeline** for closed JS → **`\0asm`** (gold → IntNet → flow → bytes; provisional **m4** contract in `research/m4-flow-net-contract-draft.md`). **Paper A owns the method kernel** (enumeration and Lean samples such as `naive_exact`, `decision_list_exact`, `cong_of_pointwise`, `asks_subset_K`). Paper B **cites** that kernel; it does **not** re-prove IntNet existence and does **not** claim that A’s discharges automatically transfer to every UJS walker.

Where Paper A targets a C self-hosting shell, UJS applies the same oracle discipline on the **construct / jtape spine** (lex…irsel [+ IC], wasm lower when tables ship). The **M3 spine** (`compiler.ujs` → `compiler_core.wasm`, twin-tests in §5.2) is **product scaffolding / 旁路** for Pages ship—Python-free delivery evidence, **not** Paper B’s main contribution and **not** a claim that the shipping compiler is an IntNet. **现状出货 ≠ 主张中心** (Appendix D). Claims are suite names already in the repository; we invent no throughput numbers.

**摘要（可选）**  
UJS 把 UNISA SH [cite A] 的构造法迁到闭合 JS：**主张中心** = 构造 IntNet / 表阶段 /（目标）流推导管线（闭合 JS→`\0asm`；见 `research/m4-flow-net-contract-draft.md`）。方法核在 A；B **引用**，**不**声称 A 卸责自动覆盖每张 UJS 表。**M3 出货**（`compiler_core`、**mainBody** 字节对照）= 工程旁路/脚手架，**不是**主贡献，也不是「Web 编译器 = 表网络」。**现状出货 ≠ 主张中心**。语言子集写 **UJS-1_ship**，勿与全表 UJS-1 等同。

---

## 1. Introduction

Neural components in compilers and runtimes are usually *trained* heuristics with classical fallbacks. A parallel line *constructs* networks from discrete objects so equivalence is a finite check (Omlin & Giles 1996; Tracr; Jia & Rinard). UNISA SH [cite A] takes that path for C: **Shell = inferencer + executor + model data**, with hard contracts P-1/P-2/P-3 and a now machine-checked method kernel (Lean L0–L3 in the companion repo).

**UJS** asks whether the same method transfers to a language that *looks* dynamic—finite shapes, tag-guarded operators, Web delivery—without opening ECMAScript. We define **UJS-1**: a closed subset (full language table in `ujs/prd.md` / archive). Outside the tables, rejection is permanent. **What ships today under M3** is a *growing* subset we call **UJS-1_ship** (§5.2); it is not yet the full UJS-1 table.

**Companion framing.** Paper A [cite A] owns construction algebra, IntNet soundness, enumeration-as-decision-procedure, and C self-host. This paper reuses that kernel by citation, then evaluates (i) closed JS + IC + wasm lower on the construct spine, (ii) jtape/fold and front parity where applicable, (iii) **M3 self-host of a UJS→wasm *product* compiler** and Python-free ship builders. Framing for Paper C: B is a **reproducible, gate-backed transfer instance**—not a claim that every UJS walker is Lean-closed or that UJS-1 is fully emitted by core.

---

## 2. Contributions

1. **Claim center (construct spine).** **Constructed IntNet / UJS table nets / (target) stream pipeline** for a closed JS subset → **`\0asm`**, with gold → IntNet → flow → bytes as the paper figure (provisional **4 × 10 → `\0asm`** mnemonic; `research/m4-flow-net-contract-draft.md`). Method transfer (not re-proof): same Shell proposition and P-discipline as [cite A], including **IC** and wasm `isel`/`enc`/`reloc` when tables ship; IntNet proofs stay in A.
2. **M3 as scaffolding, not thesis.** **M3** (`compiler.ujs` → `compiler_core.wasm`; main-body twin-tests) documents **现状出货** and Python-free ship—it is **旁路 / historical scaffolding**, not the main Paper B contribution and not an IntNet instance. Construct path (**jtape / fold / acc / icfold**) carries method acceptance.
3. **Web product surface**: `wasm_run` / UXE Pages; default compile path **`compiler_core.wasm`** (P0); Python construct is not a ship dependency (`web-build` remains a *dev / engine* path, not Pages emit).
4. **Cross-host front parity** where dual fronts still exist; parity reduces drift, it does not replace [cite A].
5. **Layered ship gates** without selling compression over tables [cite A; Boniol et al.].

Non-claims (aligned with [cite A] / `prior-art.md`): no novelty of table→MLP construction; no novelty of enumeration verification; UJS-1 ≠ full ES; **UJS-1_ship ≠ full UJS-1**; no CompCert-level claim that gold ≡ JS semantics (external referees / fold only); **M3 compiler ≠ constructed IntNet**.

---

## 3. Related Work

We share adversarial positioning with [cite A] / `research/prior-art.md`; here only what matters for the *transfer* story.

**Constructed nets, not trained heuristics.** Omlin & Giles (1996) and Tracr (Lindner et al., 2023) already show finite discrete maps can be *compiled* into network weights. Paper A restates existence (P-8) as folklore and discharges soundness/enumeration/congruence in Lean. UJS does **not** re-claim that folklore; it asks whether the *same contracts* survive a closed dynamic-looking language and a Web ship path.

**Verification style.** Reluplex-style SMT on continuous nets is the wrong tool once keys are finite products: P-3 is a decision procedure by enumeration [cite A; Jia & Rinard 2021]. Boniol et al. (2026) compress exact decision procedures with BDDs; we explicitly **do not** sell size≪table as the thesis.

**Learned compiler components.** MLGO / learned indexes keep classical fallbacks or optimize among legal choices. Our *table* nets are total functions on closed $K_s$ with P-1 (opaque `ask`). The M3 product compiler is classical structure code under twin-tests—closer to ordinary self-hosting than to MLGO.

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
src ──► walk (front + jtape [+ IC]) ──► Fn(jtape, meta)   [construct / bytecode spine]
              │ runtime oracle.ask on a subset only (see below)
              │ irsel: gold → acc exact; no ask("irsel")
              ▼
         unisa.intnet.build(..., stages=ujs.STAGES)  [cite A: same kernel; UJS keys]
              │
              ▼  (optional) emit_wasm / path-B tools — method & full-surface emit

src ──► iterate/compiler.ujs (M3) ──► compiler_core.wasm
              │                        ──► splice RT ──► \0asm
              ▼                        [product compile spine; not IntNet]
         stage2 ≡ stage1 · game body ≡ stage0
```

- **Classic code:** seed walkers / stage0; **and** iterate M3 sources.
- **Network:** last-mile table selection on the construct spine only; proofs in [cite A]; weight *bytes* land in `weights/` (and shared root `weights/` for A). Construction calls `unisa.intnet.build(name, stages=ujs.STAGES)` with UJS’s explicit `STAGES` registration (`ujs/seed/construct/gold.py`) — **not** UNISA’s default eighteen-network map.
- **Product compile:** `ujs/iterate/compile.mjs` (symlink `ujs/compile.mjs`) defaults to `compiler_core.wasm`; stage0 `compiler.wasm` is bootstrap and body≡ oracle. **Do not read M3 as “the Web compiler is driven by table nets.”**
- **API:** `practice/core` `wasm_run` / `practice/uxe` Host; Pages load game wasm + host, not a Python emit at request time.
- **Dev residual:** `python3 -m ujs web-build` still builds path-A `ujs_full` under `practice/core` when forced; it is **out of** the P0 ship path.

Nets that remain table-shaped when present: `lex`, `parse`, `scope`, `type`, `shape`, `irsel`, `ic`, `isel`, `enc`, `reloc` (exact at ship when that table ships). M3 expands a **classical** AST→wasm subset independently of baking those golds.

**Ten UJS nets vs eighteen UNISA fact networks.** UJS carries **ten** table nets (listed above; one gold table = one net). Paper A’s C product is **two layers** [cite A]: a **seven-stream-stage byte pipeline** (`preprocess` → `lex` → `parse` → `opt` → `prune` → `lowering` → `encode`+image; `README.md`, `exec/pipeline/image-stages.tsv`) and, orthogonally, **eighteen** fact/decision networks in `unisa/gold.py` (`pp`, `lex`, `parse`, `type`, `scope`, `irsel`, `enc`, `reloc`, `regmap`, `tyinfo`, `pfconv`, `peep`, `opinfo`, `prec`, `binsel`, `isel`, `abi`, `combo`)—the eighteen are **ask nets**, not eighteen stream-stages. Shared *names* across UJS, the stream-stages, and those nets (e.g. `type`, `irsel`, `isel`, `lex`) are **not** the same gold tables: field vocabularies and backend targets differ (UJS `isel` keys on `JOPS` / wasm forms; UNISA `isel` adds `arch`; UNISA adds C/multi-target nets such as `peep`, `abi`, `combo` that UJS does not register).

**Key-space scale (FULL gold; counts from `ujs/seed/construct/gold.py` and `unisa/gold.py`).** Illustrative |K|: UJS `irsel` 117, `type` 1863, `isel` 42; same-named UNISA nets are larger (`irsel` ≈ 420, `type` ≈ 4275, `isel` ≈ 174), before counting UNISA-only nets.

**Runtime `ask` vs enumeration (`acc`).** Do **not** claim that all ten nets drive compilation via `oracle.ask`. On the construct path, walkers call `ask` on `lex`, `parse`, `scope`, `type`, `shape`, and `ic` (`ujs/seed/construct/front/`, `vm.py`); wasm emit/lower call `isel`, `enc`, and `reloc` when enabled (`emit_wasm.py`, `lower_wasm.py`). **`irsel` is checked only by full-key `acc` against gold** — there is no runtime `ask("irsel", …)` in the UJS tree.

**Wasm ship surface (UJS vs UNISACC).** UJS **ships** WebAssembly today: **`compiler_core.wasm`** (M3 product compile) and **`sim.wasm`** (Pages step). Python **`ujs2wasm`** remains the **method / control factory** for full-table emit, fold probes, and path-B tooling — not the P0 ship bridge (`compile.mjs` defaults to core). **UNISACC** wasm (tape → wasm as a **seventh output target**; FX-3 in `archive/plans/ideas.md`) is a **roadmap** item for the C product; it currently has **no ship surface** — the public compiler still delivers native objects and `.tape`, not a browser-facing wasm artifact.

**unisacc ↔ UJS (spine mnemonic, not identity).** [cite A]’s shipped C product is **seven stream-stages × eighteen fact/decision nets → native** (six platforms). **Paper B claim center (provisional m4 method arc):** **four stream segments × ten UJS table nets (`ujs.STAGES`) → `\0asm`**—stream count **N may differ from seven**; nets are the closed-JS table surface, not UNISA’s eighteen fact nets. **现状出货（≠ 主张中心）：** UJS’s current Web product remains **one M3 ship spine × ten table nets → `\0asm`** (`compiler_core`); cite it only as product scaffolding (footnote), not as the thesis numerator. Construct/jtape is the method arm feeding the m4 figure. Layered horizontal map (overview, pipeline *role* alignment, net surface, name lists): **Appendix D**.

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

UJS cites these by reference. **B’s extra P-2 obligation:** each new *table* net (especially **IC** and wasm `isel`/`enc`/`reloc`) must eventually supply an A-style domain-closure sample or keep the runtime assert + suite pressure. **A’s reloc L3 sample is not a proof of any UJS walker.** **M3’s classical compiler does not inherit a Lean P-2 proof**; its honesty is mainBody twin-tests / fold suites.

### 5.2 UJS-1 language (closed) vs UJS-1_ship

**UJS-1 (spec):** values `null | bool | i64 | f64 | str | list | dict | fn | tup`; locals → lexical outer → globals; permanent reject outside tables (no `eval`, prototypes, `this`/`new`/`class`, async, RegExp engine). Full clause table: `ujs/prd.md` / `archive/prd-v1.0.md`.

**UJS-1_ship (M3 / Pages path, as of compiler v18):** growing classical subset sufficient for Asteroid + drone and the self-hosting compiler—control (`if` / `else if` / `while` / …), i64/f64 arith, list/dict/index/`setidx`, unary `-`/`!`, `&&`/`||` (i64 short-circuit), **ASCII string literals 0–255 + concat + return**, globals inject / `host_*` / `run_step`.

**Str literal bound (v18):** each `"…"` literal is **0–255 bytes of non-NUL ASCII** with **no escapes** (`\` / NUL / bytes >127 / length ≥256 rejected). **Dict/dot keys ≤8** ASCII (runtime `dict_get` compares an 8-byte prefix; longer keys are rejected—no silent collision). **Still out of M3 ship:** `fn`, escapes/UTF-8, full-length dict key identity, baked gold/catalog, whole-module ≡ full Python `emit_wasm`.

Papers and release notes must say **which** face is meant.

### 5.3 jtape, fold, and M3 self-host

**jtape** remains the semantic spine for the bytecode VM / construct path; fold equates jtape VM ↔ WasmProgram (host wasm when exercised).

**M3** is the second spine: `compiler.ujs` compiled by stage0 yields `compiler_core.wasm`; recompilation yields stage2 ≡ stage1; game `sim.ujs` / `drone.ujs` bodies ≡ stage0. **Compare object in the gate:** the **main function body** from the wasm code section (`fi===1`, locals+ops). That equality does **not** by itself prove whole-module identity, `*.meta.json` identity, RT-stub identity, or source-level semantic equivalence—claim those only with separate checks. Twin-test for **Python-free product compile**, not a re-proof of IntNet, and not a claim that stage0 C and stage1 UJS are table-constructed.

### 5.4 Inline cache net

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
| Full path-B emit corpus | `tests/ujs2wasm.sh` (under `ujs.sh`) | includes probes **outside** UJS-1_ship (e.g. `arrow`, long-form `str` via Python tools, …) |
| M3 compiler self-host + P0 | `ujs2wasm_compiler.sh` | **mainBody** stage2≡stage1; sim/drone **mainBody**≡stage0; default bridge=`compiler_core.wasm`; no ship `emit_wasm` / no A-core copy; missing `compiler.wasm` = FAIL |
| UJS-1_ship fold on **core** | same | §6.2 corpus + inject step |
| Optional tinyvm validate | same (when binary found / `UJS_REQUIRE_TINYVM=1`) | `module validate` only—**not** execution twin |
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
| `str` | lit 0–255 ASCII, no escapes + concat + return | `hello ujs` |

Plus `setidx_globals` (host inject + `run_step`) and Asteroid/drone ship emit without `python3`.  
**Not** on the core gate today (still construct / full emit): e.g. `arrow`, `forof`, `ternary`, `nullish`, `blockarrow`, `sum`, `switch`, `setidx_loop`—presence in `expect.json` must not be read as UJS-1_ship coverage. Short-str **literals** are on core; **general long literals / `fn`** are not.

**Relation to Paper A.** IntNet / enumeration / congruence / P-2 sample live in [cite A]. B emphasizes language self-host *gates*, Web ship path, IC packaging on the construct spine, and honest subset bounds.

---

## 7. Discussion and Limitations

- **Closed language.** Full ES is **not** UJS-1: current table keys, schemas, and walker coverage are finite by construction for the closed subset. Opening to unrestricted ES would require **new** key/schema/coverage work and re-validation; we do **not** claim a theorem that “any infinite program set forces infinite local decision domains.”
- **Two spines.** Overclaiming “the compiler is a table network” collapses product twin-tests with method proofs; keep them named separately.
- **UJS-1_ship is incomplete.** Expanding M3 (long str, fn, …) is engineering schedule, not a silent widening of the paper’s ship claim.
- **IC is not learning.** Another constructed table under the same oracle (construct spine).
- **P-2 remains the hard transfer debt.** A’s Lean sample is a template; each UJS *table* walker must carry its own domain evidence (assert + suite, eventually A-style sample). M3 adds mainBody/fold obligations, not Lean.
- **Python constructor.** Allowed to shrink only; product compile must not grow new Python-only ship edges. P1 (construct → unisacc) is intentionally deferred (`ujs/prd.md`).
- **Web vs A’s “no Web” for the C product.** Intentional split: A = C shell; B = Web/JS practice of the *same theory*.
- **Layout is part of the claim hygiene.** Keeping generators in `seed/` and weight *bytes* in `weights/` (A/C rule) prevents rereading `gold.py` as “the reusable net.”
- **Execution hosts are not the next IntNet.** Browser/Node already run `\0asm`. Companion `tinyvm` may **validate** (and later, if budgeted, execute) ship modules as a practice oracle—**validate ≠ semantic twin**; not a third language spine and not a substitute for M3 or construct tables.

---

## 8. Conclusion

UJS shows that the UNISA SH method [cite A] extends to a closed JavaScript *spec* with IC and wasm lowering on the construct spine, and that the **product compile path can self-host in UJS** (`compiler_core.wasm`) under named twin-tests—without re-deriving IntNet theory and without equating that compiler to an IntNet. Method proofs stay in A; B’s claims are transfer, layered gates, UJS-1_ship honesty, and Web delivery—operationalized as suite names, not statistical scores.

---

## References

- [cite A] Companion: *UNISA SH / 表即网络* — `research/unisacc-paper.md` §3.4 · `archive/research/formalization-roadmap.md` (Lean L0–L3).
- Omlin & Giles, *Constructing deterministic finite-state automata in recurrent neural networks*, JACM 1996.
- Lindner et al., Tracr (2023); Jia & Rinard (SAS 2021); Boniol et al. (2026); Kraska et al. (2018); Trofin et al., MLGO (2021); Shalev-Shwartz et al. (P-8b family)—detail in `research/prior-art.md`.
- Living product / gates: `ujs/prd.md` · layout: `ujs/ARCHITECTURE.md` · Host: `ujs/practice/uxe/HOST_ABI.md` (symlink `ujs/uxe/`) · outline: `research/ujs-paper-outline.md`.
- Suite oracles (not marketing scores): `tests/ujs2wasm/expect.json` · core fold list in §6.2.

---

## Appendix A — Suite name checklist

**Language / construct:** `acc` · `fold` · `icfold` · `difftest` · `determinism` · front parity · in-page `wasm_run` · `ujs.sh` · `ujs2wasm.sh` (full corpus ≠ ship; optional tinyvm validate)  
**Product / M3 / P0:** **`ujs2wasm_compiler`** · ship builders without `emit_wasm` · §6.2 core probes (incl. short `str`) · optional **tinyvm `module validate`** (not execute) on fold+sim/drone (`UJS_REQUIRE_TINYVM=1` to require)  
**Ship-js:** `uxe_ship_js`  
**UXE (separate):** `test:uxe:*`

## Appendix B — Non-goals

Full ES · equating UJS-1_ship with full UJS-1 · equating M3 with IntNet · reading full `expect.json` as core coverage · in-page training · claiming size≪table as main result · inventing latency tables · re-proving P-8/P-3 in B · CompCert-level gold≡JS · UJS-specific Lean milestone.

## Appendix C — Warm-review (温故) map

| Reader trap | Correction |
|---|---|
| “Web compiler = table network” | M3 is classical subset + twin-tests; IntNet lives on construct spine |
| “language gate = only `ujs2wasm_compiler`” | Layered: `ujs.sh` + compiler + `uxe_ship_js`; UXE separate |
| “UJS-1 done because v18 green” | UJS-1_ship ⊂ UJS-1; `fn`/escapes/UTF-8/… still construct-side |
| “B re-proves IntNet” | [cite A] only; B has no UJS Lean |
| “`web-build` is the ship path” | path-A residual; P0 ship uses `compiler_core` + `sim.wasm` |
| “HOST_ABI / prd still say engine.wasm ships” | Was stale; living docs now: ship = `sim.wasm` + Host; `ujs_full` optional |
| “`construct/` holds the reusable weights” | Generators are `seed/`; artifacts are `weights/` (A/C rule) |
| “need a new UJS bytecode VM next” | Product path already emits `\0asm`; companion `tinyvm` validate is optional hygiene—not a third IR or finished execute twin |
| “UJS and UNISA share the same ten tables” | UJS has 10 table nets; UNISA has 18 fact nets **and** a separate 7-stream-stage byte pipeline ([cite A] two layers); same *names* ≠ same K / vocab |
| “UNISA / unisacc = 18 compile stages” | 18 = fact/decision nets at `ask` points; product byte-stream compile is 7 stream-stages—not the same layer |
| “all ten stages use oracle.ask at compile time” | `irsel` is `acc` only; runtime asks are lex/parse/scope/type/shape/ic plus isel/enc/reloc on wasm lower |
| “UNISACC already ships wasm like UJS” | UJS ships `compiler_core` / `sim`; UNISACC wasm (FX-3 / target #7) is roadmap; product IR is still `.tape` |
| “UJS ship = construct/jtape in the mnemonic” | Ship spine is M3 → `\0asm`; construct is method对照—do not fold it into the product numerator (Appendix D) |
| “unisacc stage *i* = UJS stage *i*” | Appendix D aligns *roles* across pipelines; same short name (e.g. `lex`) ≠ same byte-stream position or ask wiring |
| “Appendix D M3×10→`\0asm` = Paper B thesis” | That row is **现状出货** mnemonic only; thesis center is **4×10 stream×nets → `\0asm`** (m4 contract)—do not equate ship spine with claim center |

---

## Appendix D — unisacc↔UJS layered horizontal map（分层横向对照）

*Draft appendix for warm review. **Role alignment**, not same-name or same-index equivalence. Construct/jtape is the **method** arm; it is **not** part of the UJS **ship** mnemonic below.*

### D.1 Mnemonic (construct excluded from ship numerator)

| | unisacc ([cite A] C product) | UJS (this paper) |
|---|---|---|
| **Claim center (provisional m4)** | — | **4 stream × 10 table nets → `\0asm`** (method proof arc; `research/m4-flow-net-contract-draft.md`) |
| **Compressed spine (现状出货 ≠ 主张中心)** | 7 stream-stages × 18 fact/decision nets → **native** (six platforms) | 1 **M3 ship** spine × 10 table nets → **`\0asm`** — **product footnote only** |
| **Method arm (not ship numerator)** | exec / table-driven reference (Paper C) | **construct / jtape** → emit / `ujs2wasm` (when tables ship); seeds **m4** figure |

### D.2 Overview

| Layer | unisacc | UJS |
|---|---|---|
| **Language** | C99 (closed subset) | UJS (UJS-1 spec; **UJS-1_ship** on M3 path) |
| **Pipeline** | Seven stream-stages (generic executor + δ) | **M3** single product spine (`compiler.ujs` → `compiler_core.wasm`) |
| **Nets (ask points)** | 18 fact/decision nets | 10 `STAGES` in `ujs.STAGES` |
| **IR** | Compiler **tape** (byte-stream contract) | **jtape** / M3 wasm opcode lowering (classical on ship spine) |
| **Ship** | Six-platform **native** object / image | **`\0asm`** (+ Host/Pages packaging) |
| **Wasm** | **Not shipped** (roadmap / tape-side lower) | **Shipped** (`compiler_core`, sim/drone, construct `ujs2wasm` when enabled) |
| **Kernel** | Shared **IntNet** construction kernel [cite A] | Same intnet kernel; walker passes **`stages=ujs.STAGES`** (10), not UNISA default 18 |

### D.3 Pipeline (horizontal roles)

**unisacc byte-stream (seven stream-stages, [cite A]):**

`preprocess (e2)` → `lex (e1)` → `parse (e3)` → `opt (e4)` → `prune` → `lower` → `elf/encode + image`

**UJS product ship (collapses to):**

`compiler_core` (M3 classical subset) → **`\0asm`**

**UJS method arm (construct, not M3 numerator):**

`front + ask` → **jtape** → `emit` / **`ujs2wasm`**

*Alignment rule:* e.g. unisacc `lex` and UJS `lex` share vocabulary and gold-shaped tables on the construct path, but UJS **ship** does not replay unisacc’s seven-stream schedule; M3 is a separate classical compiler under twin-tests (§5.3).

### D.4 Net-layer surface

| Topic | unisacc | UJS |
|---|---|---|
| **Shared net names** (vocab / backends differ) | `lex`, `parse`, `type`, `scope`, `irsel`, `isel`, `enc`, `reloc` | same set where table-backed on construct path |
| **UJS-only** | — | `shape`, `ic` |
| **unisacc-only** | `pp`, `abi`, `combo`, `regmap`, `tyinfo`, `pfconv`, `peep`, `opinfo`, `prec`, `binsel`, … | — |
| **`irsel`** | Wired for real **`ask("irsel", …)`** on the C pipeline | Table / **`acc`** on construct path; **no** `ask("irsel")` on the shipping M3 spine |

Eighteen ≠ seven: decision nets are **ask points**; stream-stages are the **compiler executor contract** (two-layer framing in [cite A]). UJS’s ten nets are the closed JS/wasm table surface for the construct spine; M3 may compile without baking every gold.

### D.5 Name lists (stable checklist)

**unisacc — 18 decision nets:**  
`pp` · `lex` · `parse` · `type` · `scope` · `irsel` · `enc` · `reloc` · `regmap` · `tyinfo` · `pfconv` · `peep` · `opinfo` · `prec` · `binsel` · `isel` · `abi` · `combo`

**UJS — 10 table nets (`ujs.STAGES`):**  
`lex` · `parse` · `type` · `scope` · `shape` · `irsel` · `ic` · `isel` · `enc` · `reloc`

---

*Draft. Cross-cite [cite A] at every shared-method claim. Numbers only from green suites. Keep two spines and subset bounds in every abstract rewrite.*
