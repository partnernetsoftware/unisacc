# K2 enc slice (block 4) — handoff (current as of 2026-10-03 17:25, commit after 554a0b5d)

## Rules (from coordinator / exec/k2-boundary.md)
- Manifests use only ops rows/table/template/call/foreach(<=2)/holder/label/assert-absent/let (+ bind/seq/mapseq/result/export).
  No `py` op, no `state:` / live-graph reads, no callbacks.
- P4: facts are domain data only (fields, widths, constants, names). No state labels or action lists in facts. Facts come
  from exec/facts/export.py producers (or hand header-form tables with a `# source:` line). After editing export.py run
  `python3 exec/facts/export.py` (every table's input sha changes) and commit the re-rendered tables.
- Verify after each file: `GRAPHHASH_JOBS=1 SLOT python3 tests/graphhash.py --only exec/enc` must say 10 entries, 0 bad.
  SLOT = /private/tmp/claude-501/-Users-wjc-repos-unisacc/a4100558-c3da-4eaf-85bb-27b1ebf587d1/scratchpad/slot.sh
- Gates (SLOT ./tests/gate.sh --suite ...; batches of <=60 s): finite-template, decision-ledger, exec-arm (25 s),
  exec-elf, exec-pex86, exec-machx86, exec-armwin. decision-ledger counts `.rows/.st/.seqs... .append(...)` in any exec/*.py
  as a graph edit: avoid those attribute names in export.py helpers.
- Commit per file with pathspec; no push; temp files only inside the worktree (scratch generators in .k2enc/, not committed).

## Done (all graphhash-identical, py deleted)
- tools moved to tests/enc: ref.py, pereal.py, tins.py (META -> fact enc-tins-meta).
- memorylayout, armlayout, address, fp, x86win, hostbridge -> *-manifest.tsv (+ facts enc-*).
- pedelta, machodelta, elfimage -> *-manifest.tsv + *-template.tsv with field-schema facts enc-pe / enc-macho / enc-elf
  (export.py producers pefields / machofields / elffields; helper _fieldchain/_rows).
  Pattern: fact table rows (i, width, src const|reg, value, endian, lead = names of preceding items: byteN or a
  computation name). Manifest mapseq builds byte{v}, computation sequences (actions with $CONST cells), and per-field
  lead{i} = splice(lead) + field load (where src). Template block: `fresh START ret`, loop rule
  `{prev:ret} * WRITER [["@","lead{f.i}"],["PUSH","{fresh@row:ret:r}"]]`, then end rules; extra labels used by later rules
  are allocated in the original order with `fresh {fresh:lN:k} -` rows. Template vars come only from each/over scope,
  so lists go in through opts let `@ref:...`; template $NAME cells resolve through the row's bind.
- exec/assemble.py: approved generic block "enc field chains" (mapseq key with {col}: {"over", "parts": [{"splice"} |
  {"where", "acts"}]}).
- 7 gates green after machodelta (0b1859b6) and after elfimage (554a0b5d).

## Remaining
1. Entries: exec/enc/gen.py and exec/enc/arm.py still Python. Convert to exec/enc/gen-manifest.tsv / arm-manifest.tsv run
   by exec/build/gen.py (see exec/lower/gen-manifest.tsv: `#! base`, `#! flags`, let rows). Flags: elf macho pe object.
   gen.py pieces: START symbol interning (classes/ops/META/x86win keys), procs(), emit_rules pre/division/post,
   x86-shell dispatch (install_rows per class), relax(), done-write/done-image/done-raw; arm.py: specs, contract
   (armcontract sections put/check/signed/finish loops), armbase, manifests. START interning loops become foreach over
   fact tables (ops, META, keys); check loops over specs become foreach over a spec fact table (shape string -> rows).
2. Switch callers (enc lines only; R3-C edits the same scripts for other stages): tests/graphhash.py keys
   ('exec/enc/gen.py', 'exec/enc/arm.py' -> 'exec/build/gen.py enc' / 'exec/build/gen.py arm'?), exec/pipeline/prepare.sh,
   exec/c/*, exec/enc/*check.sh (pecheck.sh uses `exec/enc/$encoder`), tests/gate.sh rows.
3. Re-run 7 gates after each change.
4. Optional cleanup: remaining *-result.tsv sections no longer read; .k2enc/ scratch can be deleted.
