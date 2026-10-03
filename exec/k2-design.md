# 只是设计: moving constructor glue into per-stage manifests

This design is based on reading `exec/finite_rules.py`, `exec/parse2/gen2.py` (only partly), `structreturnexpr.py`, `valueranks.py` and `tests/decisionledger.py`. I did not open `librarytypesv3.py`, `libraryimports.py`, `lex/gen.py`, `pp/gen.py`, or any `enc/` or `lower/` constructor. Each slice below starts with a survey for that reason.

Counts from a grep:

| Directory | `.py` files | Files that call install |
|---|---|---|
| parse2 | 56 | 46 |
| enc | 49 | 26 |
| lower | 16 | 10 |
| pp | 12 | 3 |
| lex | 11 | 3 |
| parse | 3 | 1 |
| nativeabi | 2 | 2 |
| opt | 1 | 1 |
| top-level `exec/*.py` | 7 | not counted |

`gen2.py` contains 111 lines that mention "install".

## 1. Manifest TSV: `exec/<stage>/<gen>-manifest.tsv`

There is one manifest per generator entry point, for example `parse2/gen2-manifest.tsv`, `lex/gen-manifest.tsv`, `enc/armobject-manifest.tsv`. Rows run in the order they are written. There is no separate order column; the line number is the order, which avoids renumbering.

Columns: `op  stem  section  when  facts  fresh  seq  bind  opts`

- **op** is one of:
  - `rows`: becomes `install(g, root, stem, ...)`.
  - `table`: becomes `install_rows(path)`.
  - `template`: becomes `install_template`.
  - `call`: runs a nested manifest, for a sub-constructor such as `floatconst`. Its arguments are `stem=<sub-manifest>` and a `bind` map.
  - `label`: adds names to `g.labels`.
  - `assert-absent`: covers guards such as `assert "TN.raw" not in g.st`.
  - `holder`: declares a named fresh scope (see `fresh`).
  - `foreach`: repeats the following indented block once per fact element. Depth is shown by leading `.` on `op`, the same convention as template `over`. This covers `for section in ('h1'..'h7')` and the width-dispatch per-row loops.
- **section** is a literal, or a format string over mode flags and facts, for example `fail{errors?.errors}`. That syntax expresses `"fail.errors" if errors else "fail"` without an if-statement.
- **when** is a mode predicate: `-`, `errors`, `!warnings`, `locations&errors`, or `fact:NAME` (true when the fact NAME is non-empty, as for `float-select` only when floats exist). It is a pure boolean over the build flags `{locations, warnings, errors}` plus the truthiness of named facts. There is no expression language.
- **facts** names a fact function, `module:func_facts`, or `-`. Several can be joined with `+` to merge dicts. The returned dict supplies template facts, and also `sequences` and `bindings` through the reserved keys `_seq` and `_bind` when computed values are needed.
- **fresh** is the fresh-holder scope:
  - `-` means None.
  - `P:NAME` means `P(NAME).fresh`, with DEFS registered.
  - `U:NAME` means an unregistered holder, like `valueranks`' `VR.fresh`.
  - `S:NAME` means a scope with `cur=NAME` (the `structreturnexpr` `_Scope` hack).
  - `none` means `lambda k: None`.
  - `=NAME` reuses a holder declared earlier with `op=holder`. This matters because `valueranks` shares one holder across 9 installs. Fresh-label order depends on holder identity.
- **seq** and **bind** are comma lists. Each item is `k=factref`, `k=@rej:TEXT` (`E.rej(TEXT)`), `k=@bytes:TEXT` (`SBOUT` byte sequence), `k=$NAME` (a stage-global binding from the stage facts), or `k=fresh:SCOPE:KIND`, which allocates through the named holder at this row. That last form replaces the `P(x).fresh("b")` calls inside binding dicts. Allocation order is the left-to-right order in the cell, which reproduces Python dict-literal evaluation order.
- **opts** is JSON for `mode`, `domain`, `classes` (by fact reference), `overlay`, and `result=NAME`, which stores a return value such as `group-tail`'s `{S: label}` into the fact environment for later rows.

The existing `*-fresh.tsv` tables (`control-fresh`, `ordinary-fresh`, `return-fresh`, `update-fresh`, `shape-fresh`, `<ns>-fresh`) already have the form "part, owner, kind, key". They become a generic `fresh=tape:control-fresh.tsv@part`. The assembler then does what `structured_control`, `return_control` and the others do today, so those helpers are deleted rather than translated.

## 2. Assembler: new `exec/assemble.py`

`finite_rules.py` stays "expansion only", so the assembler lives in its own file.

```
run(manifest_path, E, P, flags: dict, env: dict) -> env
register(module_name) -> resolved by importlib; facts fn = getattr(mod, name); must end in _facts
```

How it works:
- Each fact function has the signature `f(ctx) -> dict`. `ctx` is a read-only view `{E, b (bindings env), flags, consts}`.
- Results are memoized per `(fn, flags)` within one build. This matters for performance, because the same facts are reused across sections.
- The assembler is the only caller of `install*`, of `P(...)`, of `.fresh`, and of `g.labels`. The `E.WORDS.append`/`E.TK` token registrations at the top of `build()` become an op `token` with `stem=type=extern`.
- `gen2.build()` collapses to `assemble.run(here/'gen2-manifest.tsv', E, P, dict(locations=.., warnings=.., errors=..), stage_facts())`.

## 3. The gen2 driver: what counts as generic assembly and what must move

**Generic (it moves into `assemble.py` or stays a primitive):**
- The `P` subclass with its `DEFS` bookkeeping and `vpush`/`vpop` calling `valueranks.slots`. This is runtime machinery of `P`, not a per-stage decision. Move `class P` into `exec/parse2/pclass.py`. The ledger classifies it as `keep pclass primitive`. The `RANKOF` mapping becomes a table, `valueranks-slots.tsv`, and `slots()` becomes a generic "companion expansion" read from that table.
- `tape_rows`, the fresh-tape loops, `addr`/`emit` binding builders, and `width_dispatch`'s per-row loop. All of these become `foreach` plus `fresh=tape:` rows.

**Must move:**
- **Into tables:** every `if warnings/errors/locations`, every section choice, every literal binding dict, and the call order of the roughly 40 sub-constructors in `build()`. The `from X import install; X_install(E, P, ...)` calls become `call` rows.
- **Into the facts layer:** the magic constants at module level (`ETAG`, `SHAPE_IDS`, `TYINFO`, `FPS_*`). They become `gen2_facts.consts_facts()` returning a dict, which feeds `$NAME` bindings.
- **Into `*_facts` functions:** `valueranks`' ownership asserts on the `1<<40` banks. Asserts that only check data are allowed in fact functions; they produce no graph effect.
- **Into ops:** the `ladder`/`dispatch` recursion over operator levels becomes `foreach ctx:levels` with a `template` op.

## 4. Ledger extension in `tests/decisionledger.py`, new metric `glue`

For each `exec/**.py` file, excluding `finite_rules.py`, `assemble.py`, `pclass.py` and `build/`, count violations:

1. A module-level statement other than an import, a docstring, a `def`, or an `Assign` whose value is a literal or constant expression (checked with `ast.literal_eval`, or BinOp/Tuple over literals).
2. A `def` whose name does not match `*_facts` or `_*` (private helpers are allowed), or a `class` defined anywhere.
3. A call inside any def to `install`, `install_rows`, `install_template`, `load`, `run`, `P(`, `.fresh`, `.vpush`/`.vpop`, or `.labels.*`, or any of the existing `GRAPH_ATTRS` mutations.
4. A `*_facts` def that does not end in a `return` of a Dict, Name or Call, or that contains a `yield`, or that has a parameter other than `ctx`.
5. An `if` or `IfExp` that tests a name in `{errors, warnings, locations, mode}`. Mode branching is only allowed in manifests.

The report gives `glue` per stage. The baseline should reach 0 except for allowlisted `keep` primitives. Also add a check that every `facts` name in every `*-manifest.tsv` resolves, and that every `*_facts` function is referenced by at least one manifest, so no dead facts remain. Both run as `python3 tests/decisionledger.py --glue`.

## 5. Migration and parallel slices

**Step 0, done serially first:**
- Land `assemble.py`, `pclass.py`, and the ledger `glue` metric in report-only mode.
- Write the golden hashes: for each generator × each mode combination (2³ flags where the generator honours them), run `PYTHONHASHSEED=0`, dump the graph canonically (state order, edges, labels, seqs) and record its sha256 in `tests/graphhash.tsv`.
- Add a `tests/graphhash.sh <slice>` script that rebuilds and compares.

**Parallel slices**, each touching disjoint directories plus its own manifests:

| Slice | Scope | Size estimate |
|---|---|---|
| A | `parse2` sub-constructors (about 45 files: structreturnexpr, valueranks, librarytypesv3, libraryimports, and the rest). Each `install(E,P,...)` becomes `X_facts` plus `X-manifest.tsv`; gen2 still calls them through `assemble.run`. | biggest; can split A1/A2 alphabetically |
| B | `parse2/gen2.py` driver: `build()` order goes into `gen2-manifest.tsv`, helpers are deleted, consts go to `gen2_facts.py`. Depends on A only through the `call` interface, so A's files can be wrapped temporarily. | |
| C | `enc` (26 install files) | |
| D | `lower` (10) plus `opt` (1) plus `nativeabi` (2) | |
| E | `lex` (3) plus `pp` (3) plus `parse` (1) plus top-level `exec/*.py` (7) | |

Every slice must keep the hash identical for every generator and mode it touches before it merges. The ledger `glue` count must reach 0 for its directories, and the existing ledger total must stay ≤30. Once all slices are merged, `glue` becomes a hard gate.

## 6. Risks and what cannot be a table

- **Fresh-label order.** Labels are allocated through holder callbacks, in Python evaluation order. Dict literals evaluate left to right, `P(x).fresh` is called inline, and generators inside `bindings.update(...)` evaluate lazily. The manifest's left-to-right `fresh:` cells plus `=NAME` shared holders must reproduce this exactly. Convert one constructor at a time and compare hashes, not end to end. The worst cases are `addr()` (6 fresh labels inside a zip, plus 3 more), `width_dispatch`, and `types()`.
- **DEFS double-definition asserts.** `P(NAME)` increments `DEFS[name,'P']`. Anywhere a holder is created more than once (`P(current).fresh` repeated in `tytail`), the manifest must create the same number of `P` instances, or the asserts silently change. Mitigation: the assembler logs a DEFS snapshot, and the hash file also records `sha(DEFS)`.
- **Performance (≤55 s per generator).** Parsing the manifest is negligible. Watch for re-running fact functions per row; memoization prevents that. `foreach` expansion should call `install_template` once per row with list facts where the template already supports `each`/`over`, not once per element. Measure with `time` per generator in the hash script.
- **What cannot be a table:**
  1. Fact computation itself: reading `unisa/` modules, gold tables, layout arithmetic, rank maxima. That is the intended Python surface.
  2. `assemble.py`, `finite_rules.py`, and `P`/`pclass` (the label allocator, DEFS, and vpush companion expansion). These are generic interpreters; a table cannot interpret itself.
  3. Python-side token-id registration (`E.TK` computing max+1) if other code reads it before the graph is built. It can become an op, but its arithmetic stays in the assembler.
  4. Any case where installation feeds back into facts, such as a `group-tail` return used by a later row's facts. This is handled by `result=` storing into the environment, but a fact function that inspects `g` would break the "facts are graph-independent" rule. Each such case should be listed under `keep` with a reason.

### Critical Files for Implementation
- /Users/wjc/repos/unisacc/exec/finite_rules.py
- /Users/wjc/repos/unisacc/exec/parse2/gen2.py
- /Users/wjc/repos/unisacc/exec/parse2/valueranks.py
- /Users/wjc/repos/unisacc/exec/parse2/structreturnexpr.py
- /Users/wjc/repos/unisacc/tests/decisionledger.py