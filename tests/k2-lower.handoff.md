# K2 slice C/block 5 (pp, lex entries) — handoff (2026-10-03 17:45)

Rules: exec/k2-boundary.md. Entry driver: `exec/build/gen.py STAGE OUT [--FLAG...]` (OUT STAGE order too).
Header lines: `#! base PATH`, `#! graph CLASS` (E.g = CLASS() from the base, e.g. build/graph.py G/Delta),
`#! flags ...`, `#! start NAME|$ENV`, `#! domains STEM` (Delta.finish(start, {mode: values})),
`#! extra KEY STEM` (output KEY = fact KEY of facts/STEM).

## Done (this block; worktree branch, merged block-5 branch e53ab61f/14463ed1 first)
- pp: exec/pp/gen-manifest.tsv (flags locations shared-predefines osx win arm64 no-autoinc; lnx/x86_64 default)
  -> call body-manifest (flags renamed shared-predefines / no-autoinc). E2_AUTOINC env is gone: `--no-autoinc`.
  Predefine resources = fact `predefres` in facts/pp-gen (compilerpack, sharedcheck read it).
  export.py `_ppsrc()` replaces importing pp/gen.py. parse/gen.py takes G from build/graph.py and keeps the
  ROOT + exec/ sys.path inserts (enc/parse2 relied on that side effect). pp/gen.py deleted.
  Parsed JSON equal to old pp/gen.py in 14 modes; graphhash pp keys re-recorded (13 entries incl --no-autoinc).
- lex: exec/lex/gen-manifest.tsv over facts/lex-gen (export.py lexgen(): HANDLE/PEEKCLASS/tries/classes,
  seq_plain/seq_typed/seq_positions, tok_names). Delta.finish = totality + reachability (generic, domains passed).
  finite_rules: install_rows skip/ordered; observation substitution yields signed i64 bits (only lex uses <<56).
  sourcefacts-*.tsv `@lexer` -> `$lexer` binding. --check-declarations -> exec/lex/declcheck.py (lex/run.sh).
  Output byte-identical to old lex/gen.py in 5 modes; graphhash keys renamed, hashes unchanged.
  lex/gen.py, locations.py, lexsourcefacts.py deleted. decisionledger keep lines for pp/lex dropped; baseline 10.

## Verify state
- See the round report; full graphhash + gates finite-template, decision-ledger, exec-pploc, exec-pp-directives,
  exec-macros, exec-lexloc, exec-lexpos, exec-lex-sourcefacts, exec-chain.

## Left / notes
- Remaining old entries: parse/gen.py is a library (prune/lower/opt base); enc/arm.py, enc/gen.py, parse2/gen2.py,
  parse2/units.py are other owners'.
- docs/exec/e2-pp-delta.md, e1 docs still name the old scripts (historical text).
- tests/gatedeps.json closure hashes are stale until `make gatedeps` at release time.

## Gotchas
- gate.sh needs UNISACC_FFI_X86_PROVIDER (slot.sh sets it). Worktree guard: no `$VAR` command, no compound
  git commands, heredocs with conflict markers or git words refused -> write scripts to .k2tmp/ with Write.
- Grep for callers that build paths (`R/'exec'/script/'gen.py'`, `('pp','pp/gen.py',...)`) — plain grep for
  `exec/pp/gen.py` misses them.
- After a rebase, regenerate facts with `python3 exec/facts/export.py` (headers carry input sha prefixes).
