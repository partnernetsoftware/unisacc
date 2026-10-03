# K2 slice C/block 5 (lower, prune, opt, nativeabi, pp, lex entries) — handoff (2026-10-03 17:25, branch head e53ab61f + this note)

Owner of block 5: pp and lex entries (and nativeabi). Rules: exec/k2-boundary.md; manifest ops in exec/assemble.py doc;
generic entry driver exec/build/gen.py (`exec/build/gen.py STAGE OUT [--FLAG...]`, OUT/STAGE order either way; header
lines `#! base PATH`, `#! flags ...`, `#! start NAME`). Verify every step byte-for-byte (old vs new output, cmp) and
with `GRAPHHASH_JOBS=1 <scratchpad>/slot.sh python3 tests/graphhash.py [--only DIR]`; gates one per slot.sh call:
finite-template, decision-ledger, exec-lower, exec-lexloc, exec-pploc, exec-chain, lib-callable-model.

## State (all on main after e53ab61f merges)
- Done as manifests, old .py deleted, callers switched: lower code (code-manifest.tsv; per-target env = facts/lower-code
  `codeenv`), prune, opt (`--o2`), nativeabi (`#! start NC.START`, calls ../modelgraphequality landed by the decoder agent;
  facts/nativeabi from export.py nativeabi()). py op removed from assemble.py.
- Generic primitives: exec/build/graph.py (G, Delta), exec/build/procs.py (make_P; parse/gen.py subclasses it).
  exec/.gitignore = `build/*/` (files directly in exec/build are tracked). models.py closure includes exec/build/gen.py.
  tests/gatedeps.json compilercheck excludes nothing now (its exec/.gitignore guard hash is stale -> `make gatedeps` at
  release time only).
- parse/gen.py is a library, not an entry (no parse manifest needed).
- pp: exec/pp/gen.py (257 lines) is still the entry, but build() is: G(); assemble.run(body-manifest.tsv, flags
  {locations, shared, autoinc}, env {target}); g.finish(). body-manifest calls autoinc-manifest, locations-manifest,
  sourcefacts-manifest. Facts: facts/pp-gen (export.py ppgen(): init acts, predef.{target} chains, cases/dswkeys,
  dactions, esc/esckeys, xelayout, objname, location_line), facts/pp-autoinc-gen (ppautoinc()). Both producers import
  exec/pp/gen.py for its module-level facts (PREDEF, XOPS, DIRV/PPT from weights/gold/pp.tsv, autoinc_map, sbconst,
  xe_init) -> keep those or move them into export.py before deleting gen.py.
- lex: not started (exec/lex/gen.py 497 lines + lexsourcefacts.py + locations.py; Delta already in exec/build/graph.py;
  decisionledger keep line: 3 sites = the unreachable-state sweep after the completeness check, gen.py:~411-417).

## Next: pp entry (exact steps)
1. exec/pp/gen-manifest.tsv: `#! base pp/gen.py`? NO — the base must provide E.g/E.P; build/gen.py does `E.g.finish()` and
   writes compact JSON. Make a tiny generic base: exec/build/gbase.py (g = G(); P = None) or give build/gen.py a
   `#! base build/graph.py` mode where E.g = G(). Then gen-manifest rows: lets for target from flags (like
   exec/lower/gen-manifest.tsv: `#! flags locations shared-predefines osx win arm64 no-autoinc`; target lnx/x86_64
   default, `osx`/`win` exclusive, `arm64`), `let` autoinc (flag `no-autoinc` -> body flag), then `call body`.
   body-manifest uses flags `shared` / `autoinc`: rename to `shared-predefines` and `!no-autoinc` (call passes the
   entry's flags), and env `target`.
   Only targets in facts/pp-targets are valid (old gen.py raised); the predef.{target} lookup fails otherwise.
2. Output differences: old gen.py json.dump default separators and printed sizes; build/gen.py writes compact JSON
   -> compare old vs new by parsed JSON (json.load equal, incl. key order) for every graphhash mode, then
   `tests/graphhash.py --write --only pp` after renaming keys in tests/graphhash.py (target matrix x flags:
   ('',) + lnx/x86_64, osx/arm64, win/x86_64 x --locations/--shared-predefines) to `exec/build/gen.py pp ...`.
3. E2_AUTOINC=0 env -> `--no-autoinc` flag: callers exec/c/compilerpack.py (tokenpp built_model env E2_AUTOINC='0',
   line ~183), tests/seedconstructmatrix.py tokenpp entry, any `E2_AUTOINC` grep hit.
4. compilerpack imports exec/pp/gen.py for predefine_resources() (line ~165): export the resources
   (b"\0predefines/OS/ARCH" -> NUL-joined names, from PREDEF via target_predefines) into a facts table and read it.
5. Switch the ~54 callers (grep `pp/gen.py` in tests/ exec/ release/): positional target `exec/pp/gen.py OUT osx/arm64`
   -> `exec/build/gen.py pp OUT --osx --arm64`; e.g. exec/pipeline/prepare.sh (`"$TARGET"`: map with a case like its
   OSFLAG/ARCHFLAG lines), exec/pipeline/stages.tsv, exec/pp/*check.py, exec/parse2/*check.py (other owner's dir:
   only the generator path/args), exec/c/{chain,buildcompiler}.sh, tests/modelsourcelayoutprovenancecheck.sh,
   tests/queuecheck.py fixture name (actual-generator edit). Some callers read gen.py's stdout sizes: check.
6. Move the module-level fact code still needed by export.py out of pp/gen.py (into export.py helpers), delete
   exec/pp/gen.py, run export.py, full graphhash, gates exec-pploc, exec-chain, decision-ledger (drop the
   `keep pp/gen.py` line in tests/decisionledger.allow; `python3 tests/decisionledger.py --update` lowers baseline).

## Next: lex (design)
- argv flags parsed at import (--typed/--positions/--locations imply downwards, --sourcefacts, --check-declarations);
  graphhash covers (), --typed, --positions, --locations, --sourcefacts, --check-declarations. Manifest flags the same;
  implications as `let`/when rows or flag-conjunctions in `when`.
- Delta is not G: no RET/DEAD finish, its own seq tuple rule, output written by lex/gen.py (check its JSON shape and
  start state). build/gen.py assumes E.g with st/seqs/finish(): give Delta a finish() that does the existing
  completeness check + unreachable sweep (that moves the 3 remaining ledger sites into exec/build/graph.py, honest only
  if the sweep is stage-independent: it removes states unreachable from START — generic), or a `#! finish` header.
- Rules come from exec/lex/*.tsv via install_section/install_rules/install_group (lex's own loader with domain(),
  skip, in_domain_order) — check whether finite_rules.install covers them (domain lists via opts domain_keys) or
  whether lex rows need a template. charclass/_check_decl is a build-time check of declared byte classes: move it to a
  check script or export.py asserts. lexsourcefacts.py / locations.py are small installers -> sub-manifests.
- Same procedure: record old outputs for all 6 modes first (PYTHONHASHSEED=0), convert piecewise with assemble.run
  sub-manifests called from lex/gen.py (byte-equal at each step), then the entry + callers (grep `lex/gen.py`).

## Gotchas
- Do not edit the tree while graphhash/gates run (they import the generators). Commit before testing (pathspec commits).
- gate.sh needs UNISACC_FFI_X86_PROVIDER set (any value) even for these suites.
- Worktree guard: no `$VAR` as a command, no python heredoc text naming git, no compound git+loops; put Python in
  .k2tmp/*.py (untracked scratch) and run it.
- exec/facts/*.tsv headers carry input sha prefixes: after a rebase, conflicts in generated facts are resolved by
  re-running `python3 exec/facts/export.py`; `export.py --check` must report 0 differ.
- assemble template cells substitute str(value): JSON-valued template fields must be pre-serialised (pp dsw `acts`).
  Templates demand complete states unless `domain_keys` limits the domain; foreach `pre` belongs on the foreach row.
- Header-form facts (`# name<TAB>value`) give names directly and `STEM!` as a dict; bank tables now store value = bank<<40.
- Set iteration order (hash-seed dependent) in old generators: record it under PYTHONHASHSEED=0 in the exporter
  (see export.py _NATIVE_TRIE) rather than re-deriving.
