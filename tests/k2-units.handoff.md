# K2 block 3 (units/unitmode/parenfold/errors/valueranks/layoutprovenance) — handoff

## Round 1 (2026-10-03)
- valueranks: install translated by tests/k2translate.py (one gen2 mode recorded; all args are
  mode invariant) -> exec/parse2/valueranks-manifest.tsv + exec/facts/k2-valueranks{,-f1}.tsv.
  The `g.labels.update` is a label *write* and becomes a `label` row (recorder captures it).
  gen2.py calls assemble.run(valueranks-manifest.tsv). install() removed from valueranks.py.
  Dropped: the owner-collision assert (VALUEBANK.. vs other modules' banks) — a python sanity check,
  not graph construction; move to a facts check if wanted.
  valueranks.py NOT deleted yet: it still exports VALUEBANK/LCSITERANK/slots/RANKOF read by
  gen2.py (slots, VALUEBANK) and librarycallables.py (LCSITERANK). Next: those callers read
  exec/facts/valueranks.tsv directly (facts('valueranks')), slots() moves into gen2 helpers; delete.

## Plan for the rest (not started)
- errors.py: live reads are `g.st[tag]` assert (drop: the template returns tags) and
  `sorted(g.labels)` as the stack domain. Needed: the label set at that point as a named result.
  Since g.labels is the global return alphabet built by every stage, that is a generic pass
  candidate ("domain = all labels") used by errors-stack and others -> whitelist with justification,
  or assemble opts.domain="labels" (one marked change). Decide with coordinator.
- parenfold.py: asserts on g.st ("NEXT", "DL.read" presence) select the reader by mode: replace with
  `when` on flags (warnings|errors -> DL.read else TN.checked) and drop the seq asserts.
- unitmode.py: header relocation finds the unique state whose seq contains HEADER, and END.ok's
  init tail/target: both must become named results exported by the producing templates
  (start/END manifests) -> needs those producers to `result=`/@out the state names.
- layoutprovenance.accept_gates: walks all ACCEPT edges in the whole units graph: a generic graph pass
  (redirect every ACCEPT edge through a gate) — only one stage uses it, so per the rule the ACCEPT
  producers must export names instead.
- units.py -> exec/build/gen.py units + units/gen-manifest; callers: tests/graphhash.py lines 34, prepare.sh,
  exec/c/*, checks; re-record graphhash keys.

## Round 2 (2026-10-03)
- Gates finite-template + decision-ledger green (run with UNISACC_FFI_X86_PROVIDER=x via slot.sh).
- @labels survey: the only users of "all labels as return domain" are G.finish's RET (exec/pp/gen.py:233,
  generic graph primitive, parse/gen.py shares it) and errors-stack. So >=2 -> drafted assemble opts
  `"domain": "@labels"` = sorted(g.labels)+["BOT"] (5 lines, script .k2tmp/vr/asm.py); reverted until
  errors' manifest uses it (needs tests/decisionledger.allow line: converter-level label enumeration).
- errors record (modes errors+locations, errors+warnings+locations; zsh: pass flags unquoted literally,
  "$m" does not word-split): emit refuses "template fresh callback invoked (ER_b)": errors.py's
  fresh(kind) splits OWNER_H into holder cur + kind. Fix: rewrite the errors template fresh kinds
  as plain kinds per owner holder (`U:OWNER` fresh column per section) or make translator accept
  `S:` split callbacks; also `groups` (reasons template return dict) drives the per-tag loop -> needs
  result=+foreach over the returned tags. Also drop the g.st[tag] assert and the load_rules overlap assert.
- Not started: valueranks importers -> facts (gen2.py:61,64,702 slots/VALUEBANK; librarycallables.py:13),
  parenfold, unitmode, layoutprovenance, units entry.
