# K2 round 3 slice C (lower + stage entries) — handoff (2026-10-03, after round 1)

## Done
- 7fffaf14 Part 1: exec/lower/code.py deleted; exec/lower/code-manifest.tsv = one `let` with
  `{"bindmap": "codeenv.{target}"}` (assemble.py: bindmap string path is now `{NAME}`-interpolated)
  + `call` of codeinit/armfuse/codedispatch/codeprep/codeabi/codelib/codeend.
  Per-target env (reg_*, reloc_*, form_load64/callr, ids/idmap/idmapu, scratch, s0/s1, process_flag,
  decoder bank constants IDS/ADDRESS/DESC/BKIND/SUPPORTED/WRITABLE/EXTENT/STRIDE, hosted=true) is the
  json scalar `codeenv` in exec/facts/lower-code.tsv, written by exec/facts/export.py lowercode()
  (scratch-alias asserts moved there). `hosted` was a private flag of code.py's sub-runs: now
  `fact:hosted` in codeend/codelib/libraryexit/librarydata/libraryimports manifests.
  gen-manifest `py code` -> `call code`. graphhash --only lower: 9 entries, 0 bad.
  Gates: exec-lower ok, finite-template ok, decision-ledger RED (expected, see below).
- decisionledger.py default run now also runs ops() (gate decision-ledger enforces --ops).

## Open
- Decoder: exec/modelbindings.py was NOT yet a manifest on origin/main (16:25). code-manifest row 12 is
  `py ../modelbindings` and assemble.py keeps a py branch marked TRANSITIONAL, decoder only. When the
  decoder manifest lands: replace row 12 with `call` of it (stem relative to exec/lower, e.g. a
  `../modelbindings` manifest needs call to accept a path or a lower-local wrapper manifest), delete
  the py branch, decision-ledger goes green.
- Part 2 started: exec/prune/gen-manifest.tsv (base parse/gen.py; rows prune main; label CLASS.r8).
  `exec/build/gen.py prune OUT` == `exec/prune/gen.py OUT` byte-for-byte except the old trailing
  newline. Still to do for prune: move the SCHEMA_SHA tape check into exec/prune/check.py (or a
  facts digest), switch callers (tests/graphhash.py key + graphhash.tsv hash, exec/pipeline/prepare.sh,
  exec/c/buildcompiler.sh, exec/prune/check.py, tests/seedconstruct{check,matrix}.py,
  tests/modelobjectcheck.py, tests/modelcallbackgraphcheck.py uses PE.construct()), then delete gen.py.
  Callers that read the prune stdout JSON summary: none found (check.py only checks rc).
- Remaining entries untouched: lex (497 lines + lexsourcefacts/locations), pp (464), parse (334),
  opt (184), nativeabi (89 + ordered.py). parse/gen.py is also the base module for prune/lower.

## Round 2 (16:25-16:31)
- 5d4b4e7b decision-ledger: `allowop py exec/lower/code-manifest.tsv ../modelbindings` in decisionledger.allow
  (matched by manifest+stem); gate green, strict once the row is removed.
- d13885d3 prune done: callers on `exec/build/gen.py prune`, exec/prune/gen.py deleted, SCHEMA_SHA tape guard in
  exec/prune/check.py schema() (only runs with check.py now, not on every build), graphhash key renamed (068a6a14...).
- uncommitted->committed: exec/parse/{prn,numout,fconv}-manifest.tsv + exec/facts/numeric-instances.tsv = the
  parse/gen.py prn()/numout()/fconv() primitives as manifests (states identical to the Python versions,
  .k2tmp/prntest.py). Reach them from another stage with `call ../parse/prn` (call resolves stem against the
  caller's dir; the sub-run root becomes exec/parse).
- 12602e22 opt done: exec/opt/gen-manifest.tsv (`#! flags o2`; callers `exec/build/gen.py opt OUT [--o2]`),
  facts/opt-gen (start1/start2 acts, peepidx, Y) from export.py optgen(), name tables moved to
  facts/opt-{analysis,peep,rounds}-names.tsv (header first) used by freshrows; rounds bindings carried with
  `accumulate`. graphhash --only opt 0 bad; gate exec-chain ok (287 equal, 1 not-covered, 0 bad).
- Next: nativeabi (+ordered.py), parse (base module for prune/lower/opt; its P class / tokenizer are the
  generic primitives -> exec/build/), pp, lex; then allow/ledger update and full graphhash + gates.

## Gotchas
- gate.sh needs UNISACC_FFI_X86_PROVIDER set even for unrelated suites (any value works for these).
- Worktree guard rejects commands that call a shell variable as the command (`$S ...`); write the
  slot.sh path literally.
