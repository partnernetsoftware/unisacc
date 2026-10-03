# K2 block 2 (library* + layoutfacts) — handoff (current state, 2026-10-03 18:28)

## Done (each: parse2 graphhash 10/10 identical, .py deleted, committed)
librarycallbackgraph, librarytypesv3, librarytypes, libraryvariadic, librarydata (install -> manifest; global_address
lives in exec/parse2/addr-manifest.tsv), libraryimports, layoutfacts. 5 gates green on the rebased HEAD
(finite-template, decision-ledger, exec-parse2-libimports, lib-callable-model, exec-r21-e3).

## Handed over (coordinator 18:30): librarycallables + libraryexports -> gen2-driver agent
- librarycallables.py: one live-graph read — the FS.CALLTYPE continuation, produced by exec/parse2/callcontrol-result.tsv
  (part2.plain / part2.warnings, `PUSH f_part2_1430_FPCALL_r_1`). The callcontrol owner exports it as a named result
  (`fpcont`); librarycallables-manifest binds `$fpcont`. Rest is mechanical (libraryvariadic pattern: move/hookcall/copy
  templates need the $name/$proc/$hook/$original rewrite; `out:TEXT` sequences -> str facts or an outseq-like option;
  u64 x3 -> `call ../modelinput` with json NUL keys as in layoutfacts-manifest.tsv). Its `owned` bank assert vs all
  modules -> extend exec/facts/export.py BANK_OWNERS.
- libraryexports.py: after librarycallables. u64 x2 -> `call ../modelinput` (key fact `"\u0000library/symbols"` json,
  `key=@bytes:=FACT`); hooks/templates per libraryexports-template.tsv; result label 'LX.version'; env for the
  librarytypes/libraryimports/libraryvariadic calls as now built in libraryexports.py.
- librarymodule.py: with the gen2-driver work (END.* producers).
- Bank ownership: exec/facts/export.py --check now runs `_bank_owners` (BANK_OWNERS: layoutfacts.RESERVED_BANKS,
  libraryexports.RANK_BANKS) over the stage constant tables; replaces the runtime asserts. Note: `--check` already
  reports 14 table diffs on HEAD before this change (not from block 2).

## Rules / tools learned
- NUL-prefixed resource keys: json-typed fact `"\u0000library/..."` + `@bytes:=FACT` (no loader change needed).
- Translator (tests/k2translate.py): `_partial` refuses template parameters in partial use; it cannot record modules whose
  install runs nested manifests (flattened) — hand-write those. Binding names with '=' and sequence names with
  newlines break manifest cells.
- assemble opts.outseq: `@ O:TEXT` names in the stem's tables become OUT bytes of TEXT.
- Debug a graphhash DIFF by `python3 exec/parse2/gen2.py OUT.json` old vs new and diffing states/seqs.
- Frames (STX/LDX register save/restore) are mapseq over register tables in exec/facts/k2-libraryframes.tsv.
- Scratch: .k2lib/ (untracked; generators mk*.py, wip/).
