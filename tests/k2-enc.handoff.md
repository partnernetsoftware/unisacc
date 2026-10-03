# K2 round 3 slice B (enc + entry drivers) — handoff

## Done (round 1, graphhash `--only exec/enc` 10/10 identical after each)
- tools moved to tests/enc: ref.py, pereal.py, tins.py (TIns text). tins META -> fact enc-tins-meta;
  check importers insert tests/enc into sys.path (one line before `from tins import`).
- memorylayout.py -> memorylayout-manifest.tsv (env in: fail, code_size; foreach+chain over
  enc-memorylayout-resources / -dl / -imports; u64 = rows ../modelinput u64; key/pending via mapseq).
- armlayout.py -> armlayout-manifest.tsv (bindmap enc-armlayout-bindings!, freshrows over enc-armlayout-names,
  accumulate; call memorylayout). init inlined in arm.py START; SYM/PRESENT read from facts.
- address.py -> address-manifest.tsv (one row per former instance; layouts split to enc-address-layout-<tag>).
- fp.py -> fp-manifest.tsv (foreach ops as op, nested foreach op.regs as r; FP_IDS from enc-fp-ops).

## Done (round 2)
- x86win.py -> x86win-manifest.tsv (foreach over enc-x86win-{regs,stdh,arity,metarows,oprows,rcs}; section chaining via
  opts export {"wx_prev": last fresh}; init/reset inlined in gen.py START).
- hostbridge.py -> hostbridge-manifest.tsv (flag arm64; guard chain = facts enc-hostbridge-guards-<arch>; guard template
  test label now {fresh:t:b} with fresh column P:EMIT / P:HB chosen by `when fact:own_X`).
- gates after round 2 (each one slot.sh call, UNISACC_FFI_X86_PROVIDER=unused needed by gate.sh): finite-template,
  decision-ledger, exec-arm, exec-elf, exec-pex86, exec-machx86, exec-armwin all rc=0 (arm/elf/machx86/armwin ran
  before the hostbridge change; finite-template, decision-ledger, pex86 re-ran after).

## Remaining
- elfimage.py (calls pedelta/machodelta installs), machodelta.py, pedelta.py.
- header-form facts gotcha: a first data row starting with TAB (empty first cell) is read as the old =/@ format.
- entries gen.py, arm.py -> exec/enc/gen-manifest.tsv, arm-manifest.tsv run by exec/build/gen.py; switch
  tests/graphhash.py keys, exec/pipeline/prepare.sh, exec/c/*, check scripts (pecheck.sh uses `exec/enc/$encoder`).
- gates at end (one per slot.sh): finite-template, decision-ledger, exec-arm, exec-elf, exec-pex86, exec-machx86, exec-armwin.

## Patterns that worked
- byte(p,v) callback == [["LDI","ob",v],["OUTW","ob"]]: build via mapseq (literal or "$name" from bind).
- a fresh whose owner is another fresh: `.let bind x=fresh:P:{entry}:r` then `fresh:P:{x}:b` in the next row.
- names TSVs (section/name/owner/kind) -> exec/facts table + opts freshrows where {"section": ...}.
- python generators for manifests were scratch (.k2enc/, uncommitted).

## Round 3 (rebased: already on origin/main 9ecfe45e)
- all 7 gates re-run after the last change (hostbridge 48ed2aa8): rc=0.
- not converted: pedelta/machodelta/elfimage. They are field-emission chains: P.call(proc) = goto proc with the
  pending acts + PUSH of a fresh ret label (P counter, owner prefix PE/MH), literal bytes are LDI ob/OUTW ob appended
  to the pending acts. Plan: field schema as facts (rows: kind field|literal, width, value int|register name, big),
  one template block per chain: `rule {prev:cur} * {fresh:ret:r}`-style per field row with the literal bytes folded
  into the row's actions; first state = section entry, last label exported (result) for the following install
  (header-end/imports-end/signature-end take state + pending). Verify fresh order against the P counter by graphhash.
  load_rules(...)['actions'][0][1] uses (entry-value, directory-values, reloc-align, import-values, name-value,
  signature-length) become literal act facts with bindings resolved via mapseq "$name".
- then gen.py/arm.py entries -> manifests (touch only enc lines in shared caller scripts).

## Round 4 (rebased on origin/main)
- pedelta.py -> pedelta-manifest.tsv + pedelta-template.tsv: field chains recorded into facts
  enc-pedelta-{header,imports}-<arch> (proc, pre = JSON text of the actions before the call); template block
  `each tl` (one-element list holding the tail text, since template vars come only from each/over scope):
  fresh row seeds prev:ret (literal or $entry via bindings), loop rule `{prev:ret} * {f.proc} {f.pre}["PUSH","{fresh@row:ret:r}"]]`,
  then the old header-end/imports-end rules as `-` rows. pedelta-result.tsv sections entry-value/directory-values/
  reloc-align/import-values/name-value/header-end/imports-end are now read only by the scratch generator (can be pruned).
- next: machodelta (same pattern; its field() calls MB.big/MB.little, p.call(ret=label5) once), elfimage
  (iat/dlslots chains + header field chain), then entries.

## P4 rework of pedelta (coordinator 2026-10-03): open
- current enc-pedelta-{header,imports}-<arch> rows hold proc labels + action-list text: recorded transitions, violates P4.
- target: facts = domain fields from unisa.image.pe via exec/facts/export.py: per field (name, width, endian kind,
  value const|register), literal byte runs (hex) and named computations (entry-value, directory-values, reloc-align,
  import-values, name-value(offset)) as separate items; template maps kind -> writer proc (EI.bytes / MB.little / MB.big).
- blocker: one graph transition = all literal bytes + computations since the previous field + that field. A template
  loop cannot concatenate a variable number of items into one action list; one rule per item changes the graph
  (graphhash red). Needs either (a) a generic assemble opt: mapseq part with a formatted key per element
  (e.g. "lead_{i}" over a field list, acts built from the item's hex bytes and from named result-tsv sections), with the
  template splicing ["@","lead_{f.i}"]; or (b) accepting extra states (graph changes, new hashes). Decision: (a).

## Round 5: P4 rework done for pedelta (f2a54b75)
- facts exec/facts/enc-pe.tsv written by exec/facts/export.py `pefields` (from unisa.image.pe): constants, tables
  header_<arch>/imports (i, width, src const|reg, value, endian, lead = item names byteN / entry-value / name-valueN),
  *_tail, bytes, names. No labels or action lists in facts.
- exec/assemble.py block "enc field chains": mapseq key with {col} + {"over", "parts": [{"splice": COL} | {"where", "acts"}]}.
- machodelta plan: same; writer MB.{endian}; copy section rows move into the template with $labelN -> {fresh:lN:k},
  allocation order forced by `fresh {fresh:lN:k} -` rows (owner MH); p.call('MH.pad', ret=label5) -> rule to MH.pad
  pushing {fresh:l5:r}; signature chain starts at {fresh:l6:r} (big-endian); signature-length becomes a mapseq computation.
- editing exec/facts/export.py changes the input sha of every table: rerun `python3 exec/facts/export.py`.
