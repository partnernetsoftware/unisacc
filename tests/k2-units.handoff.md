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

## Round 3 (2026-10-03)
- valueranks.py deleted: gen2 reads exec/facts/valueranks.tsv (_VR banks, _RANKOF, local _slots);
  librarycallables/libraryvariadic read LCSITERANK/LVSITERANK from facts; libraryvariadic's bank
  namespace for `valueranks.` comes from facts. parse2 graphhash 10/10 identical (one 142 rerun).
- errors next: the `reasons` section is a `group-tail` template kind that returns {reason: tag}; the per-tag
  message loop needs result=+foreach over that return. Fresh kinds `ER_b`/`DP_b`/`UC_r` (owner_kind) need
  a per-owner holder form in assemble/translator. Then @labels (approved) with the errors-stack table row
  + decisionledger.allow line.

## Round 4 (2026-10-03)
- parenfold.py deleted -> parenfold-manifest.tsv (+ k2-parenfold{,-f1}.tsv). Live reads removed: the
  NEXT/TN.checked asserts and the DL.read seq asserts dropped; reader chosen by flags (DL.read exactly when
  tokenlocations records tokens: warnings or errors). The translator refuses disjunctions, so the DL.read row
  is hand-split into `warnings` and `errors&!warnings` (recorded with 3 modes; gen2 implies locations).
  parse2 graphhash 10/10 identical. Recorder script: .k2tmp/vr/rec.sh MODULE STEM (8 modes, sequential).

## Round 5 (2026-10-03)
- Gates on 70d634a2 tree: exec-unitparse, exec-errors, exec-r21-e3, exec-chain-1/2/3 all rc=0
  (chain: 97/97, 96 equal + 1 known, 95 equal + 1 not-covered; 0 bad).
- errors design (not landed): the `reasons` section is finite_rules kind `group-tail` (an existing,
  stage-blind graph pass: every edge whose last act is REJECT "not covered: ..." -> label ER.message<n>
  in discovery order) returning {reason: label}. Needed in assemble: (1) `result=groups` already stores the
  dict; (2) foreach over an env dict (`over: "$groups"`, rows {key, value}) with a facts join
  messages[key] (message, identifier; default reason/0/recover 0) to drive the `message` rows +
  `unknown.move/unknown` vs `plain` (split by a messages column, not a python if); (3) export one entry by
  key: lm_nomain_msg = groups["not covered: <librarymodule no-main reason>"] into env/E.results
  (global-results.tsv pattern) for librarymodule (coordinator request). Fresh: ER_b/DP_b/UC_r are
  OWNER_KIND -> per-owner holders `=ER`,`=DP`,`=UC` (holder op). Stack: errors-stack via `table` with
  `"domain":"@labels"` (approved; .k2tmp/vr/asm.py) + decisionledger.allow line.

## Round 6 (2026-10-03)
- errors.py deleted -> errors-manifest.tsv + exec/facts/k2-errors.tsv (messages joined by reason, texts, position
  acts; former errors-messages/-text/-position.tsv removed). assemble marked changes (approved): domain "@labels";
  foreach over `$NAME` (dict from an earlier result=) with join {over,on,default}; fresh `split` (OWNER_KIND).
  Exports lm_nomain_msg = groups["not covered: no main"] (ER.message61 in --errors) -> gen2 puts it in E.results.
  parse2 graphhash 10/10; finite-template, decision-ledger, exec-errors green. exec-errors-warn printed no rc line
  (not re-run). Allow justification appended to tests/decisionledger.allow.
- Remaining: unitmode, layoutprovenance, units entry.

## Round 7 (2026-10-03)
- exec-errors-warn rc=0. unitmode.py deleted -> unitmode-manifest.tsv + exec/facts/k2-unitmode.tsv (DECL banks).
  Dropped live reads: `assert name not in g.st`, the unique-HEADER-state assert (result unused), the END.ok scan:
  init_target is the fixed state INITS, init_tail = JUMP x0, LDI dep 0, PUSH $lm_initret (gen2 global3 named
  result). DEFS re-keying dropped (only feeds the "defined twice" count, unchanged). Caller env from gen2: start,
  FND, TK_extern, TK_assign, header_json, lm_initret. parse2 graphhash 10/10.
- Remaining: layoutprovenance (units graph ACCEPT walk), units entry.

## Round 8 (2026-10-03) — layoutprovenance findings (nothing landed)
- Probe (.k2tmp/vr/probe3.py wraps accept_gates): exactly one ACCEPT producer per mode, all 257 keys, identical acts:
  plain -> DONE (units-result.tsv main12: [@text5, ACCEPT]); --locations -> LS.tokens (unitlocations-result.tsv:24,
  DONE no longer accepts). Gate index = key (0..256) in both modes.
- Plan without a graph walk and without new assemble opts: rewrite layoutprovenance-template.tsv `gates` to iterate a
  key table (rows key 0..256, a domain fact) with scalar template facts acc_state / acc_acts: {G.index}->{G.key},
  {G.name}->{acc_state}, {G.acts}->{acc_acts}. The producers name the state: units manifest exports acc_state=DONE
  (plain) / LS.tokens (unitlocations); acc_acts = the producer row's actions minus ACCEPT (text5 OUT bytes; LS.tokens
  row acts) — exported by the producing row (result=/@out), not copied into facts. Open question for coordinator:
  whether exporting a row's action list as a named result is allowed (else facts duplicate the producer acts).
- units entry (exec/build/gen.py units + units/gen-manifest) not started: units.py also writes g.st directly for the
  qualifier newline rows (load_rules + g.st[state][1][key]=...; use `table` op with units-byte.tsv@qualifier inside a
  foreach over units-qualifiers) and calls E.tokenizer/prn/fconv, strings.token_span, unitlocations.install.

## Round 9 (2026-10-03) — layoutprovenance landed; units entry handed to a fresh agent
- accept_gates no longer walks the graph: units.py / unitlocations.install return named results
  acc_state (DONE | LS.tokens) and acc_acts (the producing row's actions before ACCEPT, read from that row of
  units-result.tsv main12 / unitlocations-result.tsv with the same bindings; nothing copied into facts).
  layoutprovenance.units(E,P,locations,accept) builds one gate per key 0..256. units graphhash identical (both
  modes, hashes checked against tests/graphhash.tsv); gen2 8/8 identical in the same run.
- layoutprovenance.py still exists: parser() (gen2:851, u64 + _rules 'parser') and units() (units.py). Both become
  manifest rows when their callers do (gen2 driver; units entry).

## Units entry — instructions for the next agent
Goal: exec/parse2/units.py -> `exec/build/gen.py units` driven by exec/parse2/units-gen-manifest.tsv; delete units.py,
layoutprovenance.py (units part) once graphhash identical.
1. Callers to switch: tests/graphhash.py:34 (two entries; re-record keys with --write only after confirming the
   hashes are unchanged: 6fc8d7f5... plain, 0137a215... --locations), tests/prepare.sh, exec/c/* (grep units.py),
   gate checks (grep -rn "parse2/units" tests exec).
2. Python-only steps in units.build, in order, each needing a manifest form:
   a. qualifiers: append words to E.WORDS/E.TK, then E.tokenizer(); E.prn(); E.fconv() (see how exec/build/gen.py
      runs other generators' prologues; gen2 calls E.tokenizer(qualifiers)).
   b. qualifier newline rows: load_rules(units-byte.tsv, section qualifier, domain [10], bindings qualifier_state=NX<q>,
      qualifier_token=TK[q]) written straight into g.st (overrides key 10) -> `table` row over units-byte.tsv
      (overlay) in a foreach over units-qualifiers.
   c. strings.token_span(E,P) (python helper; check whether a manifest exists for it).
   d. rules(section, extra): freshrows units-fresh.tsv@section (P(prefix+".units_"+key) registered holder, kind),
      STATIC binding; loops: length x4 (L0..L3 / EXTENT, shift 8i) -> foreach over a facts table; counters,
      separators (units-*.tsv rows) -> foreach; trailer bytes -> foreach over the trailer json.
   e. unit-labels main (ID, GOTO, COLON tokens) -> rows with tokens/classmap.
   f. locations: unitlocations.install (python, returns accept); then layoutprovenance.units: u64 helper
      (modelinput-manifest call row, see libraryresources-manifest.tsv) + layoutprovenance-result rows
      ('plain'/'located') + gates template with G over keys (acc_state/acc_acts as named results: export them from
      the producing rows with result=/export, not facts — coordinator P4 ruling).
   g. g.finish() and the json dump are the generator driver's job (exec/build/gen.py).
3. Verify with GRAPHHASH_JOBS=1 slot.sh python3 tests/graphhash.py --only exec/parse2 (units ~10 s per mode),
   then gates exec-unitparse, exec-chain-1/2/3.

## Round 10 (2026-10-03) — units entry design (nothing landed in exec/)
- Base wait: origin/main lacked the layoutprovenance commit for the whole round (polled ~20 min).
- Qualifier newline rows: a `table` op cannot replace them (G.on is first-wins; the tokenizer already set key 10
  of NX<q>). Installing them before the tokenizer, or folding them into the trie facts as `w` rows, changes the
  seq numbering: measured 6fc8d7f5... -> 3ee68396... (493 states differ only by seq index). The existing,
  form-preserving mechanism is the template graph edit `redirect` (replace one existing edge in place, seq
  allocated at that point = exactly the old g.st write): new exec/parse2/units-template.tsv section `qualifier`,
  `Q` over a facts table of (name, tok): `redirect NX{Q.name} 10 RET [["ADV"],["LDI","tk",{Q.tok}]]`; drop the
  `qualifier` row from units-byte.tsv. No new opts.
- Blockers (assemble has no py op, so these must be manifests before units.py can go):
  1. E.tokenizer(qualifiers): no manifest. Needs a facts producer (export.py TABLES, e.g. k2-units-tokens:
     trie N for WORDS+token-prefixes+units-qualifiers, qualifier codes max(TK)+1.., builtin token list, tokread
     limit0..9) and exec/parse/tokenizer-manifest.tsv (template token mode b over N, rows tokenread); a parse2
     manifest reaches it with `call ../parse/tokenizer` (call stem is a path under the manifest dir).
     E.TK is not mutated any more: `tokens`/builtin classes come from the facts via classmap.
  2. strings.token_span: cdx draft exists only uncommitted in the main checkout
     (exec/parse2/strings-token-span-manifest.tsv, exec/facts/k2-strings-token-span.tsv). Owner cdx.
  3. unitlocations.install (+ tokenlocations.install): Python, owner cdx; --locations cannot be a manifest until
     both are. Plain mode can go first only if graphhash keeps two generators (gen.py plain, units.py located) —
     ask coordinator.
  4. prn/fconv: existing exec/parse/{prn,fconv}-manifest.tsv via `call ../parse/prn` / `../parse/fconv`.
- Coordinator rulings (round 10): (1) plain mode converts first (exec/build/gen.py parse2/units); --locations keeps
  units.py until cdx lands unitlocations/tokenlocations; graphhash keeps both entries. (2) tokenizer facts = word
  list, prefix-expanded list (finite_rules.prefix_facts form: name, last, children), token codes (qualifiers
  max(TK)+1..), builtin list, limits; NO trie nodes with next-state fields — edges come from template rows
  (NX{N.name} {c.last} -> NX{c.name}); spans (token-prefixes.tsv) as a span-prefix/reader fact pair. Key order per
  node must stay: edges asc, 10, 256, then `*`. (3) redirect for qualifier newline rows approved.

## Round 11 (2026-10-03)
- Landed 43859d2b: exec/parse/tokenizer-manifest.tsv (template tokenfacts over facts k2-units-tokens + rows
  tokenread); facts = N (prefix_facts form: name, children{name,last}, span{reader}, q, w{tok}, tail{tok}), builtin,
  qualifiers(name,tok), tokens(class,tok), limit0..9. Note: units calls E.tokenizer() with the DEFAULT skip set
  (const, volatile); the 5 units qualifiers are words (w rows) and then all 5 get the key-10 redirect
  (exec/parse2/units-template.tsv section qualifier, Q over facts qualifiers). units-byte.tsv qualifier row removed.
  units.py uses both already (interim); hashes 6fc8d7f5 / 0137a215 unchanged; export --check 0 differ.
- Next: exec/parse2/units-manifest.tsv (#! base parse/gen.py, #! flags locations, plain only; locations ->
  `let exit` until cdx's unitlocations lands): call ../parse/tokenizer, template units qualifier, call ../parse/prn,
  call ../parse/fconv, call strings-token-span (coordinator lands it after the gen2 driver merge; rebase),
  classes via classmap from facts tokens/builtin (E.TK no longer mutated), rules sections with freshrows
  units-fresh.tsv@section, foreach length/counters/separators/trailer, unit-labels, main12 export acc_state/acc_acts,
  modelinput call + layoutprovenance plain rows + gates template over a key table.

## Round 12 (2026-10-03) — units-manifest design checks (nothing new landed)
- strings-token-span not on origin/main yet (1f495b5d).
- Manifest forms confirmed against assemble.py: classes via existing opts `classes` (one json fact dict incl.
  builtin, from k2-units-tokens); rejects via `let` from facts into env then seq `reject0=@rej:{reject0}` (cdx
  token-span pattern); texts via opts `textrows units-text.tsv`; fresh via freshrows list form
  {"file":"units-fresh.tsv","where":{"section":"SEC"},"owner":"{prefix}.units_{key}","kind":"{kind}","key":"{key}"}
  (E.P(owner).fresh = old P(...).fresh; the string form uses unregistered holders -> different labels);
  length/counters/separators/trailer loops need fact tables (add to the k2-units-tokens producer from the
  units-*.tsv files); layoutprovenance plain: call ../modelinput (key=@bytes:=RESOURCE, facts
  top-layoutprovenance-const) + rows ../layoutprovenance section plain, freshrows from layoutprovenance-fresh.tsv
  (old code: one P(prefix+'.fresh') per prefix; verify per-row E.P(owner) gives the same labels).
- BLOCKER (coordinator): gates template `redirect ... {G.acts}` needs the producer's acts as JSON text. A named
  result is a list of tuples; template substitution is str(value) (python repr, not JSON) and graph edits
  (redirect) do not splice sequences. Options: (a) acc_acts json fact derived from units-text.tsv text5 (the
  producing row's text; P4-borderline); (b) marked assemble change: list-valued template scalars json-dumped
  (value-form change, no opts key); (c) redirect splices `["@", NAME]` sequences (finite_rules change).

## Round 13 (2026-10-03)
- Coordinator chose (b): finite_rules template var renders list values as JSON (marked change).
- Landed 5e0c52c4: exec/parse2/units-manifest.tsv (generator .k2tmp/mk.py was scratch, not kept): plain mode via
  `exec/build/gen.py parse2/units OUT`; `--locations` row is `let exit` (units.py keeps that mode). Order matches
  units.py: tokenizer, prn, fconv, qualifier redirect, strings-token-span (call with TK_STR from parse-constants),
  rules sections (freshrows file form, owner {prefix}.units_{key}), foreach lengths/counters/separators/trailer
  over k2-units-tokens tables, unit-labels (tk_goto/tk_colon facts), main12, let textrows (text5 = producer acts),
  modelinput call (RESOURCE from top-layoutprovenance-const), layoutprovenance plain rows (freshrows owner
  {prefix}.fresh), template gatesk (each keys 0..256, over acc=[{state DONE, acts $text5}], terminal SFU.accept).
  Hash 6fc8d7f5 identical (tested with cdx's strings-token-span draft copied, uncommitted).
- Pending: strings-token-span on main (rebase); full graphhash for the finite_rules change (running); then switch
  tests/graphhash.py plain entry to `exec/build/gen.py parse2/units`, prepare.sh/exec/c callers for plain; units.py
  stays for --locations (layoutprovenance.units()/accept_gates stay until then; layoutprovenance.parser() for gen2).
