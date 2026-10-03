# K2 block 2 (library* + layoutfacts) — handoff (current state, 2026-10-03 18:28)

## Done (each: parse2 graphhash 10/10 identical, .py deleted, committed)
librarycallbackgraph, librarytypesv3, librarytypes, libraryvariadic, librarydata (install -> manifest; global_address
lives in exec/parse2/addr-manifest.tsv), libraryimports, layoutfacts. 5 gates green on the rebased HEAD
(finite-template, decision-ledger, exec-parse2-libimports, lib-callable-model, exec-r21-e3).

## Remaining
- librarycallables.py: blocked on one live-graph read — the FS.CALLTYPE continuation (`continuations = {PUSH arg of
  every row into FS.CALLTYPE}`), produced by exec/parse2/callcontrol-result.tsv (part2.plain / part2.warnings,
  `PUSH f_part2_1430_FPCALL_r_1`). callcontrol is not a block-2 file: its owner must export that label as a named
  result (e.g. `fpcont`), then librarycallables-manifest binds it with `$fpcont`. Rest is mechanical (like libraryvariadic:
  move/hookcall/copy templates need the $name/$proc/$hook/$original rewrite; out:TEXT sequences -> an `outseq`-like
  option for `out:` names or str facts; u64 x3 -> call ../modelinput with json NUL keys as in layoutfacts).
- libraryexports.py: after librarycallables. Its u64 x2 -> `call ../modelinput` rows (key fact
  `"\u0000library/symbols"` json + `key=@bytes:=fact`), hooks/templates per libraryexports-template.tsv; returns 'LX.version'.
  Bank-ownership asserts (RANK_BANKS vs other modules) -> exec/facts/export.py --check (not yet added).
- librarymodule.py: skipped by coordinator decision (goes with the gen2-driver work owning END.* producers).

## Rules / tools learned
- NUL-prefixed resource keys: json-typed fact `"\u0000library/..."` + `@bytes:=FACT` (no loader change needed).
- Translator (tests/k2translate.py): `_partial` refuses template parameters in partial use; it cannot record modules whose
  install runs nested manifests (flattened) — hand-write those. Binding names with '=' and sequence names with
  newlines break manifest cells.
- assemble opts.outseq: `@ O:TEXT` names in the stem's tables become OUT bytes of TEXT.
- Debug a graphhash DIFF by `python3 exec/parse2/gen2.py OUT.json` old vs new and diffing states/seqs.
- Frames (STX/LDX register save/restore) are mapseq over register tables in exec/facts/k2-libraryframes.tsv.
- Scratch: .k2lib/ (untracked; generators mk*.py, wip/).
