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
