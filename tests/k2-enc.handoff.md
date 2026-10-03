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
- Entries done (see rounds below). Optional cleanup: remaining *-result.tsv sections no longer read; .k2enc/ scratch.

## Round 2026-10-03 17:36 (be572e50)
- exec/enc/gen-manifest.tsv done: `exec/build/gen.py enc OUT [--elf|--macho|--pe|--object]` byte-identical to gen.py in all 5 modes
  (.k2enc/cmp.sh OLD STAGE MODES). Facts enc-x86 (export.py x86entry). assemble.py: call opts flags, domain_at, seqenv.
- Next: arm-manifest (stage dir clash: put it in exec/enc/arm/ or add a manifest-path arg to build/gen.py), callers, delete
  gen.py/arm.py and the now-unused x86-*-names/instances/sequences/bytes/reject/dispatch tsvs, graphhash full, 8 gates.

## Round 2026-10-03 17:45 (after rebase onto 97d3bf2a)
- exec/enc/arm-manifest.tsv done; driver form `exec/build/gen.py enc/arm OUT [--elf|--macho|--pe|--object]`
  (build/gen.py: STAGE/SUB -> exec/STAGE/SUB-manifest.tsv). Facts enc-arm (export.py armentry: specs with per-char
  check data first/ps/isv/code/signed). 5 modes byte-identical to arm.py before deletion.
- assemble (coordinator-accepted generic opts): call flags (literal or list = any of these flags), domain_at, seqenv.
- Callers switched: tests/graphhash.py + graphhash.tsv keys (hashes kept), exec/pipeline/prepare.sh, exec/c/{buildcompiler.sh,
  winmemorycheck.sh,compilerpack.py}, exec/enc/*check.sh + librarysymbolscheck.py, exec/ffi/bridgecheck.py,
  exec/lower/sparsecheck.sh, tests/{objectplancheck,objectpackcheck,modelobjectcheck,seedconstructmatrix}.py.
  gen.py/arm.py and their private tsvs (x86-emit-instances/sequences/bytes/reject, x86-line-names/reject,
  x86-operand-names, x86-shell-dispatch, armcontract-names) deleted.
- Note: fresh labels are named by the owner's first dotted part only (parse/gen.py P.fresh), so fresh:P:CHECK:b equals
  P("CHECK.x.y").fresh("b"); manifests use that.
- exec/rules.md still lists exec/enc/gen.py and arm.py rows (doc table, not edited here).
