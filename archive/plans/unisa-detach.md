# unisa/ detachment plan (zero-Python hard goal, prd.md §1)

> 2026-10-23 · planning agent output (read-only analysis on HEAD 2a6d2403), adopted by cc as the wave plan; per-file decisions in the session scratch (decisions.tsv). Waves W1-W4 at most 8 agents each; the move `git mv unisa archive/unisa` is the LAST step, after the seedfree gate is green. Until then unisa/ is a live dependency, not "archived".

```text
UNISA DETACH PLAN -- archive the Python seed unisa/ after its last live dependency is gone
==========================================================================================
Date: 2026-10-23.  Read-only planning against HEAD 2a6d2403 (scratch export of HEAD; live checkout untouched;
note the live checkout has uncommitted edits in m4/contract.py, m4/nets.py, docs/glossary.md and
research/c43-roadmap-0043-and-refactor.md -- coordinate with whoever owns them before W1-H/W3-F/W4-C).
Owner ruling: bootstrap complete (N22, three sha256 equal) => unisa/ goes to archive/unisa (manual reference only),
LAST, after its live dependencies are zero.  Commissar rule: no new or extended Python; replacements are .cx/.c,
the product judging itself, or a committed frozen baseline; exec/facts/export.py runs only for the R3 cut.
Interpretation used throughout: an existing .py may only SHRINK (delete code, retarget a literal path); anything
that would add logic to a .py is a .cx port instead.

Method (reproducible): `git archive HEAD` into the scratch dir; references found with
  git grep -P '\bunisa\b(?!cc)'   (815 files; word "unisa" not followed by "cc")
then classified per line (python import / `-m unisa` / `python3 -c "from unisa..."` / runtime read of unisa/*.py /
closure-or-cache dir list / generated-artifact provenance header / `#pragma unisa` keyword / prose), plus a Python
AST import graph for transitive imports (3 files import unisa only transitively: exec/lower/sparsecheck.py,
exec/lower/largecodecheck.py via tests/enc/tins.py; tests/modelsourceimportcompatcheck.py via
tests/modelsignature3check.py), plus the m4/ujs chain (adapter + flownet jobs spawn `python3 -m m4` / `-m ujs`,
which import unisa.intnet).  Gate jobs: every `job NAME CMD` in tests/gate.sh (811 names incl. the --com block),
reachability = files named in CMD plus up to 3 reference hops, excluding registry hubs (gate.sh, all.sh, lib.sh,
release.sh, refreshgatedeps.cx, export.py, seed/gen.c, apps/csih/suite.c) and registry-only jobs (gate-infra,
gate-layers, gate-select, commitgate, script-inventory, ...).  Job lists are therefore "jobs that execute or
reference the file"; hub files (src/*.c, include/*.h) show many jobs because every compile reads them.
Scratch artifacts: .../scratchpad/unisadetach/{decisions.tsv,expanded.json,inventory.txt,jobsclean.tsv,graph.json}.

Why the count is larger than "~43 tests / ~160 refs": those numbers count Python imports.  The live surface also
includes `python3 -m unisa ...` invoked from sh/.cx/Makefile/CI (154 files mention it), `python3 -c "from unisa..."`
embedded in .cx checks, runtime reads of unisa/*.py source (ast extraction, sha pins, awk mutation), the model
identity / cache-key directory lists that name `unisa`, and generated artifacts whose generator is in unisa/.

DECISION CLASSES
  (a) port to .cx; facts read from committed tables (weights/gold/*.tsv, exec/facts/*.tsv, kernel/*.inc,
      iterate/kernel/*.tsv, docs/tapebin-v1.*.tsv) and/or referee = UA (/tmp/ua_ref, the classic C reference built
      from unisacc.c by host cc, whose back end tests/closure.sh proves byte-identical to unisa's)
  (b) the product judges itself: driver = UA / unisacc.com (-run, -b), expected = host cc or a frozen expected
  (c) frozen baseline + .cx checker: capture the Python output ONCE while unisa is still live, commit it as data
  (d) obsolete -> delete with its gate job (justified per row)
  (e) generator / hard core: must become a .c/.cx generator or an explicitly frozen artifact before archive
  (f) [added] mechanical: drop the `unisa` token from a closure / cache-key / tree / registry list (no logic)
  (g) [added] inert: prose, product names (`.unisa.tape`, "unisa C subset", `#pragma unisa`), or provenance
      text inside frozen bytes -- reword or deliberately keep

COUNTS (files; 478 tracked files outside archive/ and unisa/ carry a seed reference; research/ excluded)
                              (a)  (b)  (c)  (d)  (e)  (f)  (g)   total
  code / scripts / docs-in-code 39   25  112   33    8   48   46    311
  generated / frozen data        -    -   48    -   84    -   35    167
  total                         39   25  160   33   92   48   81    478
  of which .py                  37    2   22   18    2   14    -     95  (=> 0 .py added; 18 deleted (d), 60 replaced
                                                                        by .cx (a+c + tsvbuild_check.py), 17 shrink-
                                                                        only edits (f, strconvertroute/winsuite, export.py))
  Live dependencies (a-f): 397 files (265 code + 132 data).  (c) code includes 29 of the 31 seed/unisa_*.c K6
  references, which are KEPT and re-judged (libneed, tapebin_shape are promoted under (e)); (e) data = kernel/ 41 + weights/gold 18 + weights/ 24 + src/tapebin_shape.inc.

HARD CORE (class e; removed last, wave 3, before the identity cut-over and the move)
  H1 emit-kernel   -> kernel/* (41 files; also frozen copies in tests/seals/exec-net-*/model-*.inc)
                      job `kernel` (tests/kernel.sh) regenerates and byte-compares; iterate/kernel/check.sh;
                      tests/headerchain.{sh,cx}; generator unisa/ckernel.py + gold.py + catalog.py + front/lex.py +
                      include/*.h + weights/built.*
  H2 gold-export   -> weights/gold/*.tsv (18), checked by the same `kernel` job
  H3 build-weights -> weights/built.json, built.uns2, *.f32.unisa, *.i8.unisa (24); Makefile weights/quick/acc,
                      CI ci.yml + release-check.yml, release/tools/precheck.sh, tests/tsvbuild_check.py
                      (tsv-build-account), iterate/construct/tools/*.py
  H4 facts export  -> exec/facts/export.py (R3 cut) + 44 exec/facts/*.tsv whose producers import unisa;
                      job facts-export (`export.py --check`)
  H5 tapebin shape -> src/tapebin_shape.inc, docs/tapebin-v1.shape.tsv, docs/tapebin-v1.records.tsv,
                      exec/c/tapebin.h include line; job tapebin-shape (`python3 -m unisa.tapebin_shape --check`);
                      replacement C already exists: seed/unisa_tapebin_shape.c (K6-11b)
  H6 libneed guard -> tests/libneed.sh + headerchain step run `python3 unisa/libneed.py`;
                      replacement C already exists: seed/unisa_libneed.c (K6-6)
  H7 docs table    -> tests/docs.sh (`python3 -m unisa docs --check`, README stage table + referee ledger via
                      unisa.gold.ALL); replacement: .cx reading weights/gold + built.uns2 + iterate/kernel/order.tsv
  H8 APE container -> `python3 -m unisa ape` in exec/c/apecache.py (exec-driver-* jobs), exec/c/embeddedcheck.py
                      (exec-embedded), Makefile classic-com, buildcompiler.sh SEED_GEN=0 fallback, comboot.py
                      (release/tools/candidate_chain.sh); replacement already default: UA -b + seed/ape.c
                      (buildcompiler SEED_GEN=1)  -- soft hard core, wave 2
  H9 ujs/m4 seeds  -> ujs/seed/construct/* and m4/nets.py import unisa.intnet.build; flownet/adapter jobs spawn
                      `python3 -m m4` / `-m ujs build-weights`; outputs already committed (tests/flownet-baseline.tsv,
                      adapter/ujswasm/tables/*.net.tsv) => freeze (W1-H) then archive m4/ + ujs/seed/ with unisa (W3-F)
  H10 identity     -> seed/ident.c + exec/pipeline/models.py closure() hash the `unisa` tree into the product
                      source identity recorded in unisacc.com.build.json; dropping it changes the identity once =>
                      one controlled rebuild/re-record (make com + comboot) in W4-A, BEFORE the git mv, so the move
                      itself is identity-neutral

FROZEN-vs-REGENERATE TRADE-OFF (recommendation: freeze, with drift pins; no Python regeneration path survives)
  Already committed and therefore freezable as-is: kernel/* (41), weights/gold/*.tsv (18), weights/built.*, the
  24 weights dumps, the 44 unisa-derived exec/facts/*.tsv, src/tapebin_shape.inc + docs/tapebin-v1.*.tsv,
  tests/seals/exec-net-*/model-*.inc (17 frozen kernel copies already guarded by net-*-seal), tests/flownet-
  baseline.tsv, adapter/ujswasm/tables/*.net.tsv.  The `kernel` job today proves committed == regenerated, so at
  the moment of the cut the frozen bytes ARE the generator's output.
  Cost of freezing: a future change to a gold table, a catalog row, the kernel template or include/*.h can no
  longer be propagated by running Python.  Mitigation by input class:
   - include/*.h (changes often, and kernel/unisa_headers.inc embeds them -- the original reason kernel.sh exists):
     NOT frozen.  New .cx generator for unisa_headers.inc (pure text embedding) in W3-A; the kernel job keeps
     regenerating and byte-comparing that one file.
   - weights/gold/*.tsv: become the authority (they already are for construct.c, export.py, seed tools, flownet).
     A table edit is then a data edit; weights/built.uns2 is re-derivable in C (iterate/construct/construct.c -u,
     already byte-checked against the shipped blob) and the kernel weight/dense fragments by iterate/kernel/
     genmodel.c (J10, partial: provenance header, TYPEV, ENC_* still Python).  Recommendation: freeze now; finish
     genmodel.c as ordinary product work only when the first table edit needs it.  The new kernel job is a .cx
     "provenance pin": every `# input PATH sha256:PREFIX` / per-block source line in a frozen header whose PATH
     still exists must still match; a mismatch is red with "frozen generator archived: regenerate in C
     (iterate/construct, iterate/kernel) or unfreeze deliberately".  Inputs that live in unisa/ are pinned by the
     hashes recorded in the frozen headers themselves (archive/unisa keeps the same bytes).
   - exec/facts (44): frozen + exec/facts/FROZEN.tsv (path, sha256) checked by .cx; export.py keeps only its
     non-unisa producers until its own py2cx (roadmap item 4, stategen.cx).
  Do NOT rewrite provenance headers inside frozen artifacts (e.g. "generated by python3 -m unisa gold-export"):
  changing those bytes cascades into the sha prefixes recorded in kernel/* headers, facts headers, seals and the
  model identity.  They are explicitly allowlisted by path in the done-check (below).

DONE CRITERION (make it a gate job, tests/seedfree.cx, so it stays at zero)
  The literal `git grep -l unisa` can never be empty: the product is unisacc, the .cx module language has
  `#pragma unisa`, ELF/Mach-O carry `.unisa.tape`/"unisa", include/*.h say "unisa C subset".  The precise check:
    git grep -n -P '(?<![\w/.-])unisa/|-m[\x27", ]+unisa\b(?!cc)|\b(from|import)\s+unisa\b(?!cc)|[\x27"]unisa[\x27"]|(?<![\w.-])unisa\.[a-z_]|\bunisa\b(?!cc)(?=\s+(seed|include|kernel|src|tests|weights|examples|exec)\b)|\b(exec|seed|include|find)\s+unisa\b(?!cc)' \
      -- . ':!archive' ':!research' ':!release/evidence' ':!kernel' ':!weights' ':!tests/seals' \
           ':!adapter/ujswasm/tables' ':!exec/facts/*.tsv'
  must print nothing except an explicit allowlist kept in the checker: src/cxmod.c and
  tests/carrierimportprepared.cx (`"unisa"` = pragma keyword).  Today it hits 308 files.
  `python3 -c 'import unisa'` must fail from the repo root, and `UA=... tests/gate.sh --list` must not name any
  `unisa-*` suite.

1. INVENTORY (per file: [class|wave] path / what unisa is used for / gate jobs / action)
-----------------------------------------------------------------------------------------
Jobs: suffix -N = numbered shards; "-" = no gate job (run by tests/all.sh, Makefile, CI, release tools, or ungated).
Registry-only jobs (gate-infra, gate-layers, gate-select, commitgate, script-inventory, ...) are omitted; hub files
(src/*.c, include/*.h) list many jobs because every compile reads them.

[b|W3-B] .github/workflows/ci.yml
    uses : `-m unisa build-weights/acc/compile/run` (CI host suites: construct weights, verify, Python-route compile+run of examples)
    jobs : -
    do   : drop build-weights/acc; compile/run examples with UA (build_ref.sh) or unisacc.com -run vs examples/*.expected
[b|W3-B] .github/workflows/release-check.yml
    uses : same as ci.yml (build-weights, acc, compile, run)
    jobs : -
    do   : same as ci.yml
[f|W4-A] .github/workflows/windows-signing.yml
    uses : closure dir list ('exec','unisa',...) for identity recomputation
    jobs : -
    do   : drop 'unisa' in lockstep with seed/ident.c
[b|W3-B] Makefile
    uses : `quick`/`acc`: `-m unisa acc`; `weights`: build-weights; `classic-com`: `-m unisa ape`; comment on N22 ape step
    jobs : -
    do   : quick/acc/weights targets removed (frozen weights); classic-com -> UA -b slices + seed/ape.c (same as buildcompiler SEED_GEN=1)
[c|W1-H] adapter/ujswasm/tables.cx
    uses : --check spawns `python3 -m ujs build-weights` (-> unisa.intnet)
    jobs : flownet-w2-accepts flownet-w2-d1 flownet-w2-negative flownet-w2-parity flownet-w2-refuse flownet-w2-tables flownet-w2-tables-gold flownet-w3-d3-N flownet-w3-d4-adapter flownet-w3-d4-tdesc flownet-w3-flow-N
    do   : --check uses committed adapter/ujswasm/tables/*.net.tsv (frozen); keep --check-gold
[g|-] adapter/ujswasm/tables/*.net.tsv (10)  (10 files)
    files: adapter/ujswasm/tables/enc.net.tsv adapter/ujswasm/tables/ic.net.tsv adapter/ujswasm/tables/irsel.net.tsv adapter/ujswasm/tables/isel.net.tsv adapter/ujswasm/tables/lex.net.tsv adapter/ujswasm/tables/parse.net.tsv adapter/ujswasm/tables/reloc.net.tsv adapter/ujswasm/tables/scope.net.tsv adapter/ujswasm/tables/shape.net.tsv adapter/ujswasm/tables/type.net.tsv
    uses : header: GENERATED via ujs build-weights (unisa/intnet.build)
    jobs : -
    do   : frozen bytes
[c|W1-H] adapter/ujswasm/ujswasm.cx
    uses : accepts mode spawns `python3 -m m4 build` (-> ujs.seed -> unisa.intnet)
    jobs : com-tdescprobe-N flownet-w2-accepts flownet-w2-d1 flownet-w2-negative flownet-w2-parity flownet-w2-refuse flownet-w2-tables flownet-w2-tables-gold flownet-w3-d3-N flownet-w3-d4-adapter flownet-w3-d4-tdesc flownet-w3-flow-N tdescprobe-classic tdescprobe-oracle
    do   : freeze m4 bytes for the 7 accepts inputs
[a|W2-F] exec/c/apecache.py
    uses : `-m unisa ape exec/c/compiler.c` container build for driver suites
    jobs : exec-driver-core-N exec-driver-core-prep-N exec-driver-core-prep-f1json exec-driver-language-N exec-driver-resources-headers exec-driver-resources-isolated exec-driver-resources-macros exec-driver-resources-printf
    do   : port to .cx: UA -O2 -b per target + seed/ape.c (seedtool ape), same cache key minus unisa
[d|W2-F] exec/c/buildcompiler.sh
    uses : SEED_GEN=0 fallback `-m unisa ape` (default SEED_GEN=1 already uses seed/ape.c)
    jobs : com-auditnet com-comboot-fixedpoint com-comboot-seed com-comboot-stage2 com-comboot-stage3 exec-bindprep-N exec-bindprep-first-N exec-bindprep-second-N exec-bindverify-N exec-container exec-driver-core-N exec-driver-core-prep-N exec-driver-core-prep-f1json exec-driver-language-N ...(+18)
    do   : delete the SEED_GEN=0 branch (dead fallback); SEED_GEN knob removed
[d|W2-F] exec/c/comboot.py
    uses : documents/implements Python comboot incl. `-m unisa ape`; Makefile says comboot.sh is the no-Python path, comboot.py "kept as reference"
    jobs : com-comboot-fixedpoint com-comboot-seed com-comboot-stage2 com-comboot-stage3
    do   : delete after release/tools/candidate_chain.sh (lines 19-20 run comboot.py) is switched to exec/c/comboot.sh; comboot.sh is already the gate/Makefile path
[f|W4-B] exec/c/compilerpack.py
    uses : cache-key source list includes 'unisa' (rglob *.py)
    jobs : com-auditnet com-comboot-fixedpoint com-comboot-seed com-comboot-stage2 com-comboot-stage3 com-seedgen exec-bindprep-N exec-bindprep-first-N exec-bindprep-second-N exec-bindverify-N exec-container exec-driver-core-N exec-driver-core-prep-N exec-driver-core-prep-f1json ...(+21)
    do   : drop 'unisa' from the key list (deletion only)
[a|W2-F] exec/c/embeddedcheck.py
    uses : `-m unisa ape exec/c/run.c --payload` to build the embedded runner
    jobs : exec-embedded
    do   : port to .cx; container via seed/ape.c
[a|W2-D3] exec/c/librarycallcheck.py
    uses : reference machine bytes: unisa.tape.parse + lower + assemble + image.macho.HDRS + __main__._oracle
    jobs : hostcheck lib-stack-arm lib-stack-x86
    do   : port to .cx; reference bytes from UA (-t/-b of the same SOURCE) -- UA back end is closure-proven byte-equal to the Python one
[a|W2-D3] exec/c/librarystackcheck.py
    uses : same reference pipeline as librarycallcheck (parse/lower/assemble/macho)
    jobs : hostcheck lib-stack-arm lib-stack-x86
    do   : merge with librarycallcheck port (one .cx, two modes)
[f|W4-B] exec/c/memorycheck.sh
    uses : cache key: git ls-files/status over 'exec unisa include ...'
    jobs : exec-memory-N-N exec-memx86-N-N
    do   : drop 'unisa' token
[a|W2-D2] exec/c/winmemorycheck.py
    uses : unisa.image.pe IMPORTS/DLL + assemble to compute expected Windows memory bytes
    jobs : exec-memwinarm exec-memwinx86
    do   : port to .cx; IMPORTS/DLL from exec/facts/enc-memorylayout-imports.tsv (committed), bytes from UA -b win/*
[a|W2-D1] exec/enc/armbranchcheck.py
    uses : unisa.assemble reference bytes for section-local branches
    jobs : exec-arm
    do   : port to .cx; reference bytes = UA -b of the same tape, or frozen exec/enc/armbranch-result rows
[c|W2-D1] exec/enc/armcheck.py
    uses : unisa.emit_arm.encode as the arm64 encoder referee
    jobs : exec-arm
    do   : freeze encode() outputs over the checked domain into exec/enc/arm-ref.tsv (capture while live); .cx compares net vs frozen
[c|W2-D1] exec/enc/armfpcheck.py
    uses : unisa.emit_arm FP_OPS/FARITH/FCMP_INV + encode
    jobs : exec-arm
    do   : tables already in exec/facts/enc-armfp-specs.tsv; freeze encode() bytes; .cx
[a|W2-D1] exec/enc/arminputcheck.py
    uses : unisa.assemble reference
    jobs : exec-arm
    do   : port to .cx, referee UA
[c|W2-D1] exec/enc/armintcheck.py
    uses : unisa.emit_arm.encode referee
    jobs : exec-arm
    do   : frozen encodings + .cx (shared runner with armcheck)
[a|W2-D1] exec/enc/armlayoutcheck.py
    uses : unisa.assemble reference
    jobs : exec-arm
    do   : port to .cx, referee UA
[g|W4-C] exec/enc/armmem-result.tsv
    uses : comment names unisa.emit_arm as provenance
    jobs : -
    do   : data; leave bytes (seed-memory seals hash it)
[c|W2-D1] exec/enc/armmemcheck.py
    uses : unisa.emit_arm.encode memory-op referee
    jobs : exec-arm
    do   : frozen encodings + .cx (shared runner)
[a|W2-D1] exec/enc/armwincheck.py
    uses : reference: parse + lower + assemble + _oracle (Windows arm64)
    jobs : exec-armwin
    do   : port to .cx, referee UA -b win/arm64
[a|W2-D2] exec/enc/check.sh
    uses : runs tests/enc/ref.py and exec/enc/roundtrip.py (exec-e5)
    jobs : exec-e5
    do   : port the whole exec-e5 driver to .cx, referee UA
[a|W2-D2] exec/enc/dynelfcheck.py
    uses : reference ELF (dynamic): parse/lower/assemble/image, DATA_BASE
    jobs : exec-elf
    do   : port to .cx, referee UA; DATA_BASE from exec/facts/enc-address-bindings.tsv
[a|W2-D2] exec/enc/imagecheck.py
    uses : reference ELF via driver.compile_file + lower + assemble + image
    jobs : exec-armelf exec-elf
    do   : port to .cx, referee UA
[d|W1-B] exec/enc/libraryexitcheck.py
    uses : unisa.hostabi WIN_*_BODY as expected bridge bytes (ungated helper)
    jobs : -
    do   : ungated, called only by exec/lower/libraryexitcheck.py; delete pair + ungated row (bridge covered by windowshostbridge port)
[a|W2-D2] exec/enc/librarysymbolscheck.py
    uses : unisa.assemble reference
    jobs : exec-elfdata-bridge
    do   : port to .cx, referee UA
[a|W2-D2] exec/enc/machocheck.py
    uses : reference Mach-O via driver/lower/assemble/image
    jobs : exec-macharm exec-machx86
    do   : port to .cx, referee UA -b osx/*
[a|W2-D2] exec/enc/pecheck.py
    uses : reference PE via assemble/image
    jobs : exec-pearm exec-pex86
    do   : port to .cx, referee UA -b win/*
[a|W2-D2] exec/enc/realcheck.py
    uses : reference images (driver, emit_arm.size, image)
    jobs : exec-arm exec-armwin exec-x86win
    do   : port to .cx, referee UA
[a|W2-D2] exec/enc/roundtrip.py
    uses : reference driver.compile_file + lower round trip (run by exec/enc/check.sh)
    jobs : -
    do   : port into the exec/enc/check.sh .cx port, referee UA
[a|W2-D3] exec/enc/windowshostbridgecheck.py
    uses : unisa.hostabi WIN_ARM_BODY/WIN_X86_BODY expected guard chain
    jobs : hostcheck lib-windows-gp lib-windows-gp-native
    do   : port to .cx; bodies from exec/facts/enc-hostbridge-*-winbody.tsv (already exported from hostabi)
[a|W2-D1] exec/enc/x86wincheck.py
    uses : reference: parse/lower/assemble/_oracle (Windows x86_64)
    jobs : exec-x86win
    do   : port to .cx, referee UA -b win/x86_64
[c|W3-C] exec/facts/*.tsv (44 with '# input/source: unisa/...')  (44 files)
    files: exec/facts/enc-address-bindings.tsv exec/facts/enc-address-layout-lnx.tsv exec/facts/enc-address-layout-lnx_dyn.tsv exec/facts/enc-address-layout-osx.tsv exec/facts/enc-address-layout-win.tsv exec/facts/enc-arm.tsv exec/facts/enc-armbranch-bindings.tsv exec/facts/enc-armfp-bindings.tsv exec/facts/enc-armfp-specs.tsv exec/facts/enc-armlayout-bindings.tsv exec/facts/enc-armmem-bindings.tsv exec/facts/enc-armwin-bindings.tsv exec/facts/enc-armwin-winargs.tsv exec/facts/enc-elf.tsv exec/facts/enc-fp-ops.tsv exec/facts/enc-fp.tsv exec/facts/enc-hostbridge-arm-body.tsv exec/facts/enc-hostbridge-arm-winbody.tsv exec/facts/enc-hostbridge-guards-arm64.tsv exec/facts/enc-hostbridge-guards-x86_64.tsv exec/facts/enc-hostbridge-library.tsv exec/facts/enc-hostbridge-x86-body.tsv exec/facts/enc-hostbridge-x86-winbody.tsv exec/facts/enc-hostbridge.tsv exec/facts/enc-macho.tsv exec/facts/enc-memorylayout-imports.tsv exec/facts/enc-memorylayout.tsv exec/facts/enc-pe-forward.tsv exec/facts/enc-pe.tsv exec/facts/enc-x86.tsv exec/facts/enc-x86win-arity.tsv exec/facts/enc-x86win-fields.tsv exec/facts/enc-x86win-metarows.tsv exec/facts/enc-x86win-oprows.tsv exec/facts/enc-x86win-regs.tsv exec/facts/enc-x86win-stdh.tsv exec/facts/enc-x86win-winargs.tsv exec/facts/enc-x86win.tsv exec/facts/lex-consts.tsv exec/facts/lower-abi.tsv exec/facts/lower-armfuse.tsv exec/facts/lower-code.tsv exec/facts/pp-autoinc-gen.tsv exec/facts/pp-gen.tsv ...
    uses : generated facts (catalog, emit_x86/arm, image layouts, hostabi, tape DATA_BASE, lower consts, lex/pp)
    jobs : facts-export (export.py --check); read by exec builders (seed/gen.c, exec/build) in exec-* model jobs
    do   : frozen baseline; headers left byte-identical (provenance text); listed in exec/facts/FROZEN.tsv with sha256
[e|W3-C] exec/facts/export.py
    uses : PRODUCER of ~44 facts tables from unisa.catalog/emit_*/image/tape/lower/front/libneed/tsvgold (R3 cut)
    jobs : decision-ledger e3-seal elf-seal exec-diag exec-e4-N exec-e4self exec-elfobject facts-export finite-template host-triple-N host-triple-u0-prepare kernel lex-e1-seal lib-callback-graph-equality ...(+47)
    do   : HARD CORE: after R3, delete unisa producers (deletion only), freeze those tables, sha manifest checked by .cx; export.py keeps only non-unisa producers until its own py2cx
[g|-] exec/facts/pp-layout.tsv
    uses : comment only ("not derived from unisa")
    jobs : -
    do   : none
[a|W2-D3] exec/ffi/bridgecheck.py
    uses : reference images: assemble/catalog REGMAP/emit_arm/emit_x86/image/lower/TIns
    jobs : ffi-bridge
    do   : port to .cx; REGMAP from exec/facts (enc-fp.tsv / lower-abi.tsv); referee UA
[a|W2-E] exec/lex/declcheck.py
    uses : reference TOKS (gold.py), TYPEKW (front/lex.py), tsvgold.load_table
    jobs : -
    do   : port to .cx; tokens from weights/gold/lex.tsv + exec/facts/lex-consts.tsv, TYPEKW from iterate/kernel/typekw.tsv
[d|W1-B] exec/lex/net.py
    uses : older UNS2/DENSE experiment (rules.md says so): gold.Stage, intnet, uns2
    jobs : decision-ledger seedpy
    do   : delete with exec/lex/run.sh `net` branch and its decisionledger/seedpy entries
[d|W1-B] exec/lex/stage.py
    uses : construct.build_net, gold.Stage for the same experiment
    jobs : decision-ledger seedpy
    do   : delete with net.py
[a|W2-D3] exec/lower/check.py
    uses : unisa.lower SCRATCH/PRINTMAX/WIN_*; tape.parse
    jobs : exec-lowdata
    do   : port to .cx; constants from exec/facts/lower-data.tsv / lower-code.tsv
[a|W2-D3] exec/lower/fullcheck.py
    uses : typed lowering referee: tape.parse + lower + _oracle + catalog.WINAPI/winimp
    jobs : exec-armlower exec-lower exec-winlower
    do   : port to .cx; referee = UA -t lowering dump (or frozen per-probe lowering rows, captured live)
[a|W2-D3] exec/lower/largecodecheck.py
    uses : via tests/enc/tins.py (unisa.lower.TargetProgram)
    jobs : exec-armlower exec-lower exec-winlower
    do   : follows tins port
[d|W1-B] exec/lower/librarycallback-oracle.py
    uses : unisa.libraryfnlower/librarytypedreader/tape (ungated reference)
    jobs : -
    do   : ungated observation tool; delete + ungated row
[c|W1-F] exec/lower/librarycallsite-oracle.py
    uses : unisa.librarycontract/libraryfnlower/tape as referee for callsite lowering model
    jobs : exec-librarycallsitelower-model-N
    do   : freeze reference outputs per case; .cx compares model vs frozen
[f|W4-B] exec/lower/librarydatacheck.sh
    uses : cache key `rg --files exec unisa ...`
    jobs : exec-librarydata-model
    do   : drop token
[f|W4-B] exec/lower/libraryfncheck.sh
    uses : cache key `rg --files exec unisa ...`
    jobs : exec-libraryfn-model
    do   : drop token
[c|W1-F] exec/lower/librarytyped-oracle.py
    uses : unisa.librarycontract/libraryfnlower referee for typed lowering model
    jobs : exec-librarycallback-reader-model exec-librarycallbacklower-model-N exec-librarycallsitelower-model-N exec-librarytypedlower-model-N lib-callback-candidate-auth
    do   : freeze reference outputs per case; .cx
[f|W4-B] exec/lower/librarytypedcheck.sh
    uses : cache key `rg --files exec seed unisa ...`
    jobs : exec-librarycallback-reader-model exec-librarycallbacklower-model-N exec-librarycallsitelower-model-N exec-librarytypedlower-model-N lib-callback-candidate-auth
    do   : drop token
[a|W2-D3] exec/lower/sparsecheck.py
    uses : via tests/enc/tins.py
    jobs : com-extent-neg exec-sparse
    do   : follows tins port
[c|W1-F] exec/parse2/librarycallsitecheck.cx
    uses : `python3 -m unisa.librarytypedreader ... --librarycallsite` reference
    jobs : exec-librarycallsite-reader-model
    do   : frozen reference outputs
[f|W4-B] exec/parse2/libraryfunctionimportscheck.sh
    uses : cache key list
    jobs : exec-libraryfunctionimports-model
    do   : drop token
[f|W4-B] exec/parse2/librarytypedreadercheck.sh
    uses : cache key list
    jobs : exec-librarycallback-reader-model exec-librarycallbacklower-model-N exec-librarycallsite-reader-model exec-librarytypedreader-model lib-callback-candidate-auth
    do   : drop token
[a|W2-D2] exec/pipeline/check-elf.py
    uses : referee ELF: parse/lower/assemble/image/_oracle, DATA_BASE
    jobs : exec-srcelf-N
    do   : port to .cx, referee UA -O2 -b lnx/*
[f|W4-A] exec/pipeline/models.py
    uses : models.closure() dir list includes 'unisa' (model identity)
    jobs : com-auditnet com-comboot-fixedpoint com-comboot-seed com-comboot-stage2 com-comboot-stage3 com-declmatrix-N declmatrix-N exec-armself exec-armself-package exec-asm exec-asmx86 exec-bindprep-N exec-bindprep-first-N exec-bindprep-second-N ...(+97)
    do   : drop 'unisa' (identity changes -> rebuild/re-record)
[g|W4-A] exec/pp/escape-template.tsv
    uses : declared tables whose header comments say "equals unisa.front.lex.ESC / unisa.tape.SHAPE / gold.ALL (transition)"
    jobs : -
    do   : these become the authorities: reword "transition: equals unisa/..." to "authority (frozen from archive/unisa at <commit>)"; exec/ and typekw.tsv are in the identity closure, so bundle with the W4-A rebuild
[g|W4-A] exec/tapecheck/shape.tsv
    uses : declared tables whose header comments say "equals unisa.front.lex.ESC / unisa.tape.SHAPE / gold.ALL (transition)"
    jobs : tapecheck-model-N
    do   : these become the authorities: reword "transition: equals unisa/..." to "authority (frozen from archive/unisa at <commit>)"; exec/ and typekw.tsv are in the identity closure, so bundle with the W4-A rebuild
[g|W4-A] iterate/kernel/order.tsv
    uses : declared tables whose header comments say "equals unisa.front.lex.ESC / unisa.tape.SHAPE / gold.ALL (transition)"
    jobs : -
    do   : these become the authorities: reword "transition: equals unisa/..." to "authority (frozen from archive/unisa at <commit>)"; exec/ and typekw.tsv are in the identity closure, so bundle with the W4-A rebuild
[g|W4-A] iterate/kernel/typekw.tsv
    uses : declared tables whose header comments say "equals unisa.front.lex.ESC / unisa.tape.SHAPE / gold.ALL (transition)"
    jobs : com-auditnet com-comboot-fixedpoint com-comboot-seed com-comboot-stage2 com-comboot-stage3 exec-armself exec-armself-package exec-asm exec-asmx86 exec-bindprep-N exec-bindprep-first-N exec-bindprep-second-N exec-bootstrap-osxarm exec-bootstrap-osxx86 ...(+102)
    do   : these become the authorities: reword "transition: equals unisa/..." to "authority (frozen from archive/unisa at <commit>)"; exec/ and typekw.tsv are in the identity closure, so bundle with the W4-A rebuild
[g|W4-A] iterate/kernel/vocab.tsv
    uses : declared tables whose header comments say "equals unisa.front.lex.ESC / unisa.tape.SHAPE / gold.ALL (transition)"
    jobs : -
    do   : these become the authorities: reword "transition: equals unisa/..." to "authority (frozen from archive/unisa at <commit>)"; exec/ and typekw.tsv are in the identity closure, so bundle with the W4-A rebuild
[c|W2-E] exec/pp/literalcheck.py
    uses : unisa.front.pp._pieces reference tokenisation of literals
    jobs : exec-pp-literals pp-leaf-capture-selftest
    do   : freeze _pieces outputs for the checked literal set; .cx; reseal tests/seals/exec-pp-literals (closure lists unisa/__init__.py)
[c|W2-E] exec/pp/macrocheck.py
    uses : unisa.front.pp._pieces
    jobs : exec-macros
    do   : same frozen _pieces table + .cx
[c|W2-E] exec/pp/pragmacheck.py
    uses : unisa.front.pp._pieces
    jobs : exec-pp-pragmas
    do   : same
[c|W2-F] exec/prune/check.cx
    uses : reads unisa/tape.py SHAPE/REGS + unisa/fp.py OPS at start; sha of unisa/prune.py for referee-port staleness
    jobs : exec-prune-N
    do   : read SHAPE/REGS from docs/tapebin-v1.shape.tsv (+ frozen REGS row); drop staleness check (referee archived => port is frozen); receipt fields renamed
[g|-] exec/prune/gen-manifest.tsv
    uses : comment mention
    jobs : -
    do   : none
[d|W1-B] exec/prune/libraryrootscheck.py
    uses : unisa.prune.prune_text referee (ungated)
    jobs : -
    do   : ungated; delete + row
[f|W4-B] exec/stamp.sh
    uses : PYSRC=`find unisa -name *.py -o *.tsv` in stamp identity
    jobs : exec-bindprep-N exec-bindprep-first-N exec-bindprep-second-N exec-bindverify-N exec-chain-N exec-formats exec-formats-f1 exec-unitparse include-key pipeline-cache rowcov-N-N rowcov-N-build rowcov-N-union rowcov-lex ...(+25)
    do   : drop PYSRC term
[c|W3-D] exec/tapecheck/check.sh
    uses : `python3 -c from unisa.tape import SHAPE` -> oracle-shape.tsv vs exec/tapecheck/shape.tsv
    jobs : tapecheck-model-N
    do   : compare against docs/tapebin-v1.shape.tsv (frozen authority) instead
[g|-] include/*.h (18 others)  (18 files)
    files: include/assert.h include/ctype.h include/cx.h include/errno.h include/float.h include/inttypes.h include/limits.h include/locale.h include/math.h include/pthread.h include/setjmp.h include/signal.h include/stdarg.h include/stdbool.h include/stdint.h include/string.h include/sys/types.h include/time.h
    uses : "for the unisa C subset" product prose
    jobs : ...(+14) ...(+26) ...(+27) ...(+28) ...(+29) ...(+31) c99-ledger com-memalign cxinclude cxmodule-classic cxmodule-neg cxmodule-pos ...
    do   : keep: product language name, not the seed
[g|W4-C] include/stdio.h
    uses : comments cite unisa/vm.py, unisa/libneed.py ROOTS
    jobs : cxinclude cxmodule-classic cxmodule-neg cxmodule-pos e3-seal elf-seal hosthdr identity-include-consumer-negative identity-include-negative identity-prep-negative identity-product-negative lex-e1-seal lower-seal middle-flow-e3 ...(+29)
    do   : reword only when a header is edited for another reason (headers are embedded in kernel/unisa_headers.inc and sealed in exec-pp-literals)
[g|W4-C] include/stdlib.h
    uses : comments cite unisa/vm.py, unisa/libneed.py ROOTS
    jobs : cxinclude cxmodule-classic cxmodule-neg cxmodule-pos e3-seal elf-seal hosthdr identity-include-consumer-negative identity-include-negative identity-prep-negative identity-product-negative lex-e1-seal lower-seal middle-flow-e3 ...(+29)
    do   : reword only when a header is edited for another reason (headers are embedded in kernel/unisa_headers.inc and sealed in exec-pp-literals)
[g|W4-C] iterate/construct/construct.c
    uses : comments: port of unisa/construct.py, uns2.py, tsvgold.py
    jobs : -
    do   : reword to archive/unisa/...
[c|W3-B] iterate/construct/tools/netdump.py
    uses : unisa.construct/intnet/tsvgold reference net text
    jobs : -
    do   : freeze netdump text per stage (iterate/construct/expected/*.net.txt); check.sh compares construct.c vs frozen; delete .py
[c|W3-B] iterate/construct/tools/uns2round.py
    uses : unisa.uns2/tsvgold round-trip referee
    jobs : -
    do   : replace by `construct -u` cmp weights/built.uns2 slices (C only); delete .py
[c|W3-B] iterate/construct/tools/uns2slice.py
    uses : unisa.intnet/uns2/tsvgold slice of shipped built.uns2
    jobs : -
    do   : same; delete .py (uns2pack.py references it: update)
[c|W3-A] iterate/kernel/check.sh
    uses : `-m unisa emit-kernel` reference + awk-mutated unisa/ckernel.py, front/lex.py negatives
    jobs : -
    do   : compare genmodel output with committed kernel/unisa_model.inc (== emit-kernel output by the kernel job until archive); drop Python-mutation negatives, keep TSV-mutation negatives
[g|W4-C] iterate/kernel/genmodel.c
    uses : comment cites unisa/ckernel.py _blob
    jobs : -
    do   : reword
[c|W2-E] iterate/kernel/order_check.py
    uses : order.tsv vs gold.ALL/ckernel.HEADS_MAX
    jobs : -
    do   : order.tsv becomes the authority; check vs weights/gold/*.tsv '# stage' set + built.uns2 header in .cx; delete .py
[d|W2-E] iterate/kernel/permoracle.py
    uses : ckernel/gold/intnet permutation oracle
    jobs : -
    do   : transition tool for J10; delete (genmodel vs committed kernel suffices)
[c|W2-E] iterate/kernel/typekw_check.py
    uses : typekw.tsv vs unisa/front/lex.py TYPEKW (ast)
    jobs : -
    do   : typekw.tsv is the authority; check vs kernel/unisa_model.inc TYPEV (committed) in .cx
[c|W2-E] iterate/kernel/vocab_check.py
    uses : vocab.tsv vs gold/intnet/front.lex/ckernel
    jobs : -
    do   : check vs committed kernel/unisa_model.inc vocab arrays in .cx
[e|W3-A] kernel/*.inc, kernel/unisa_core.c, kernel/unisa_self.c (41)  (41 files)
    files: kernel/dense.abi.inc kernel/dense.binsel.inc kernel/dense.combo.inc kernel/dense.enc.inc kernel/dense.irsel.inc kernel/dense.isel.inc kernel/dense.lex.inc kernel/dense.opinfo.inc kernel/dense.parse.inc kernel/dense.peep.inc kernel/dense.pfconv.inc kernel/dense.pp.inc kernel/dense.prec.inc kernel/dense.regmap.inc kernel/dense.reloc.inc kernel/dense.scope.inc kernel/dense.tyinfo.inc kernel/dense.type.inc kernel/unisa_cases.inc kernel/unisa_core.c kernel/unisa_headers.inc kernel/unisa_model.inc kernel/unisa_self.c kernel/weight.abi.inc kernel/weight.binsel.inc kernel/weight.combo.inc kernel/weight.enc.inc kernel/weight.irsel.inc kernel/weight.isel.inc kernel/weight.lex.inc kernel/weight.opinfo.inc kernel/weight.parse.inc kernel/weight.peep.inc kernel/weight.pfconv.inc kernel/weight.pp.inc kernel/weight.prec.inc kernel/weight.regmap.inc kernel/weight.reloc.inc kernel/weight.scope.inc kernel/weight.tyinfo.inc kernel/weight.type.inc
    uses : emit-kernel output (gold.py, catalog.py, front/lex.py, ckernel.py, include/*.h, weights/built.*)
    jobs : kernel (regenerate + cmp); net-<stage>-seal x17 (frozen copies); every UA build (unisacc.c includes kernel/), i.e. most of the gate
    do   : freeze 40; regenerate unisa_headers.inc by a .cx generator; headers left byte-identical
[d|W3-F] m4/nets.py
    uses : unisa.intnet.build (constructs m4's IntNets); m4 is the flownet oracle
    jobs : -
    do   : archive m4/ with unisa after W1-H froze its outputs (flownet .cx is the replacement); NOTE live checkout has uncommitted m4/ edits -- coordinate
[f|W4-B] release/derived-map.tsv
    uses : rows unisa/*.py -> facts-export/tapebin-shape/kernel/docs/unisa-*/reviewed-trees/pp-leaf-seal; seed/unisa_*.c rows
    jobs : -
    do   : delete unisa rows; rows for seed/unisa_*.c follow W1-C/D outcome
[f|W4-A] release/tools/model-reseal.py
    uses : closure dir list incl. 'unisa'
    jobs : exec-librarycallback-reader-model exec-librarycallbacklower-model-N lib-callback-candidate-auth seedmemory
    do   : drop token
[d|W3-B] release/tools/precheck.sh
    uses : `-m unisa build-weights` timing precheck
    jobs : -
    do   : drop the build-weights step (weights frozen)
[f|W4-A] release/tools/whyrerun.py
    uses : PRODUCT prefixes incl. 'unisa/'
    jobs : -
    do   : drop token
[g|W4-C] seed/ape.c
    uses : comments: same layout as unisa/ape.py, image/pe.py, peversion.py
    jobs : com-auditnet com-comboot-fixedpoint com-comboot-seed com-comboot-stage2 com-comboot-stage3 exec-bindprep-N exec-bindprep-first-N exec-bindprep-second-N exec-container exec-driver-core-N exec-driver-core-prep-N exec-driver-core-prep-f1json exec-driver-language-N exec-driver-resources-headers ...(+17)
    do   : reword
[g|-] seed/blob.c
    uses : tmp prefix "/tmp/unisa-kernel-XXXXXX"
    jobs : com-auditnet com-comboot-fixedpoint com-comboot-seed com-comboot-stage2 com-comboot-stage3 com-seedgen exec-bindprep-N exec-bindprep-first-N exec-bindprep-second-N exec-container exec-driver-core-N exec-driver-core-prep-N exec-driver-core-prep-f1json exec-driver-language-N ...(+19)
    do   : none (name only)
[f|W4-A] seed/ident.c
    uses : identity dir list {"exec","unisa",...}
    jobs : com-auditnet com-comboot-fixedpoint com-comboot-seed com-comboot-stage2 com-comboot-stage3 com-seedgen exec-bindprep-N exec-bindprep-first-N exec-bindprep-second-N exec-container exec-driver-core-N exec-driver-core-prep-N exec-driver-core-prep-f1json exec-driver-language-N ...(+20)
    do   : drop "unisa" (identity changes; one rebuild in W4-A)
[c|W1-C] seed/unisa_gold_*_label.c, seed/unisa_gold_prec.c (11)  (11 files)
    files: seed/unisa_gold_binsel_label.c seed/unisa_gold_irsel_label.c seed/unisa_gold_lex_label.c seed/unisa_gold_opinfo_label.c seed/unisa_gold_parse_label.c seed/unisa_gold_peep_label.c seed/unisa_gold_pp_label.c seed/unisa_gold_prec.c seed/unisa_gold_reloc_label.c seed/unisa_gold_scope_label.c seed/unisa_gold_type_label.c
    uses : C refs of gold.py label functions (K6-18..29)
    jobs : unisa-gold-{prec,lex,type,binsel,opinfo,peep,scope,pp,reloc,irsel,parse}(-label)
    do   : re-point to weights/gold/<stage>.tsv: `X table` output must equal the TSV rows (second derivation of the frozen authority); rename seed/gold_<stage>_label.c
[e|W3-E] seed/unisa_libneed.c
    uses : C ref of unisa.libneed (consts/guard/bodies/table/roots)
    jobs : unisa-libneed
    do   : promote to the -ftrim-libc guard checker replacing `python3 unisa/libneed.py` (libneed.sh, headerchain)
[e|W3-D] seed/unisa_tapebin_shape.c
    uses : C ref of unisa.tapebin_shape (render/records/c)
    jobs : unisa-tapebin-shape
    do   : promote to generator seed/tapebin_shape.c reading docs/tapebin-v1.shape.tsv; replaces `-m unisa.tapebin_shape --check`
[c|W1-D] seed/unisa_{ape,bits,catalog,emit_arm_nwmem,fp,hostabi,image_elf,image_fat,image_macho,image_pe,linalg,padded,peversion,tape,tapebin,uns1,vm,zdata}.c (18)  (18 files)
    files: seed/unisa_ape.c seed/unisa_bits.c seed/unisa_catalog.c seed/unisa_emit_arm_nwmem.c seed/unisa_fp.c seed/unisa_hostabi.c seed/unisa_image_elf.c seed/unisa_image_fat.c seed/unisa_image_macho.c seed/unisa_image_pe.c seed/unisa_linalg.c seed/unisa_padded.c seed/unisa_peversion.c seed/unisa_tape.c seed/unisa_tapebin.c seed/unisa_uns1.c seed/unisa_vm.c seed/unisa_zdata.c
    uses : C refs of the matching unisa modules (K6), "not a product closure member"
    jobs : unisa-{ape,bits,catalog,emit-arm-nwmem,fp,hostabi,image-elf,image-fat,image-macho,image-pe,linalg,padded,peversion,tape,tapebin,uns1,vm,zdata}
    do   : keep; judged by one generic .cx point-sample runner against frozen tests/k6/<name>.tsv (inputs+expected captured from the Python while live)
[g|-] src/asmtext.c
    uses : `.unisa.tape` section name, Mach-O identifier "unisa"
    jobs : abi-manifest abi-manifest-selftest ape-version asmtext bigclosure-lnx-arm64 bigclosure-lnx-x86_64 bigclosure-osx-arm64 bigclosure-osx-x86_64 bigclosure-win-arm64 bigclosure-win-x86_64 ccparity ccrun-N ccrun-malloc com-corpus-N ...(+150)
    do   : product format names, keep
[g|-] src/back_image.c
    uses : `.unisa.tape` section name, Mach-O identifier "unisa"
    jobs : abi-manifest abi-manifest-selftest ape-version bigclosure-lnx-arm64 bigclosure-lnx-x86_64 bigclosure-osx-arm64 bigclosure-osx-x86_64 bigclosure-win-arm64 bigclosure-win-x86_64 ccrun-N ccrun-malloc com-difftest-N com-lifecycle com-tapebin-cover ...(+120)
    do   : product format names, keep
[g|W4-A] src/back_lower.c
    uses : comments: "port of unisa/lower.py", "same as unisa/front/pp.py", schema sha of unisa/tape.py
    jobs : abi-manifest abi-manifest-selftest ape-version bigclosure-lnx-arm64 bigclosure-lnx-x86_64 bigclosure-osx-arm64 bigclosure-osx-x86_64 bigclosure-win-arm64 bigclosure-win-x86_64 ccrun-N ccrun-malloc com-difftest-N com-lifecycle com-tapebin-cover ...(+120)
    do   : reword in the W4-A rebuild commit (src edits change identity anyway)
[g|W4-A] src/front_pp.c
    uses : comments: "port of unisa/lower.py", "same as unisa/front/pp.py", schema sha of unisa/tape.py
    jobs : ape-version bigclosure-lnx-arm64 bigclosure-lnx-x86_64 bigclosure-osx-arm64 bigclosure-osx-x86_64 bigclosure-win-arm64 bigclosure-win-x86_64 ccrun-N ccrun-malloc com-csmithdiff-N com-difftest-N com-lifecycle com-seedgen com-tapebin-cover ...(+138)
    do   : reword in the W4-A rebuild commit (src edits change identity anyway)
[g|W4-A] src/opt.c
    uses : comments: "port of unisa/lower.py", "same as unisa/front/pp.py", schema sha of unisa/tape.py
    jobs : ape-version bigclosure-lnx-arm64 bigclosure-lnx-x86_64 bigclosure-osx-arm64 bigclosure-osx-x86_64 bigclosure-win-arm64 bigclosure-win-x86_64 ccrun-N ccrun-malloc com-difftest-N com-lifecycle com-tapebin-cover com-tapebin-ref com-tapebin-six ...(+118)
    do   : reword in the W4-A rebuild commit (src edits change identity anyway)
[g|W4-A] src/tapebin.c
    uses : comments: "port of unisa/lower.py", "same as unisa/front/pp.py", schema sha of unisa/tape.py
    jobs : ape-version bigclosure-lnx-arm64 bigclosure-lnx-x86_64 bigclosure-osx-arm64 bigclosure-osx-x86_64 bigclosure-win-arm64 bigclosure-win-x86_64 ccrun-N ccrun-malloc com-difftest-N com-lifecycle com-tapebin-cover com-tapebin-ref com-tapebin-six ...(+118)
    do   : reword in the W4-A rebuild commit (src edits change identity anyway)
[g|W4-A] src/tapeprune.c
    uses : comments: "port of unisa/lower.py", "same as unisa/front/pp.py", schema sha of unisa/tape.py
    jobs : ape-version bigclosure-lnx-arm64 bigclosure-lnx-x86_64 bigclosure-osx-arm64 bigclosure-osx-x86_64 bigclosure-win-arm64 bigclosure-win-x86_64 ccrun-N ccrun-malloc com-difftest-N com-lifecycle com-tapebin-cover com-tapebin-ref com-tapebin-six ...(+118)
    do   : reword in the W4-A rebuild commit (src edits change identity anyway)
[g|-] src/cxmod.c
    uses : the `#pragma unisa` keyword (product .cx module language), not the seed
    jobs : abi-manifest abi-manifest-selftest ape-version bigclosure-lnx-arm64 bigclosure-lnx-x86_64 bigclosure-osx-arm64 bigclosure-osx-x86_64 bigclosure-win-arm64 bigclosure-win-x86_64 ccparity ccrun-N ccrun-malloc com-corpus-N com-difftest-N ...(+152)
    do   : keep; allowlisted in the done-check
[g|-] tests/carrierimportprepared.cx
    uses : the `#pragma unisa` keyword (product .cx module language), not the seed
    jobs : lib-carrier-import-model-import lib-carrier-import-model-package lib-carrier-import-model-prepare
    do   : keep; allowlisted in the done-check
[e|W3-D] src/tapebin_shape.inc
    uses : "Generated from unisa/tape.py SHAPE"
    jobs : exec-bindprep-N exec-bindprep-first-N exec-bindprep-second-N exec-bindverify-N exec-driver-core-N exec-driver-core-prep-N exec-driver-core-prep-f1json exec-driver-language-N exec-driver-resources-headers exec-driver-resources-isolated exec-driver-resources-macros exec-driver-resources-printf exec-e4-N exec-e4self ...(+27)
    do   : regenerated by seed/tapebin_shape.c from docs/tapebin-v1.shape.tsv; header text changes once
[a|W1-E] tests/abi_audit.py
    uses : unisa.catalog syscall numbers vs host headers (external referee)
    jobs : -
    do   : .cx reading weights/gold/abi.tsv (+ exec/facts lower-abi.tsv)
[d|W2-C] tests/ablate.sh
    uses : `-m unisa compile` with UNISA_ABLATE (unisa/oracle.py) per stage; ungated
    jobs : -
    do   : ungated Python-route ablation; edgeablate.cx (product side, frozen) is the gate form; delete + row
[d|W1-A] tests/acceptance.sh
    uses : Python-seed acceptance (build-weights/acc/run/ship/quant, *.unisa dumps)
    jobs : -
    do   : all.sh only; tests the archived seed; delete (+ opt_partition_check.py reference)
[f|W4-B] tests/affected.py
    uses : PRODUCT/GLOBAL include 'unisa'; model-identity pp list cites unisa/front/pp.py
    jobs : -
    do   : drop tokens (ungated)
[d|W4-B] tests/all.sh
    uses : `run acc python3 -m unisa acc`; runs acceptance/vm/optpy/etc.
    jobs : c99precheck ccrun-N ccrun-malloc cli closure-c1a closure-c1b closure-c1c closure-c2a closure-c2b closure-c2c closure-c3a-lnx-arm64 closure-c3a-lnx-x86_64 closure-c3a-osx-arm64 closure-c3a-osx-x86_64 ...(+55)
    do   : remove acc + rows of deleted suites
[d|W1-A] tests/ape.sh
    uses : `-m unisa ape unisacc.c` + ape_stage.py
    jobs : -
    do   : all.sh only; container is seed/ape.c (seedape/comboot); delete
[d|W1-A] tests/ape_stage.py
    uses : ape/assemble/image/lower/padded/tape internals
    jobs : -
    do   : delete with ape.sh
[a|W2-F] tests/apeversioncheck.py
    uses : unisa.peversion.add_version_info to rebuild the expected VERSIONINFO
    jobs : ape-version
    do   : .cx: parse product VERSIONINFO, compare to frozen field table (name/version) + llvm-readobj
[d|W1-A] tests/artifacts.sh
    uses : __main__._built, gold, uns2; `-m unisa compile/ship`
    jobs : -
    do   : all.sh only; seed artifacts; delete
[c|W2-C] tests/backstages.sh
    uses : Python oracle/driver/lower ask-set for the back end (A-33)
    jobs : edge-e3-pfconv-witness edge-elf-enc-witness stages-back
    do   : freeze the Python asked-set per probe (tests/backstages.expected) + .cx compares product -v asked line
[d|W1-A] tests/baseline.sh
    uses : `-m unisa train/acc/compile` SGD control arm
    jobs : -
    do   : control arm of the seed; delete
[c|W2-A] tests/bigclosure.sh
    uses : `-m unisa compile --from-tape` reference images of the compiler source
    jobs : bigclosure-lnx-arm64 bigclosure-lnx-x86_64 bigclosure-osx-arm64 bigclosure-osx-x86_64 bigclosure-win-arm64 bigclosure-win-x86_64
    do   : freeze sha256 of reference images per target (captured live) + host self-compile run; or UA-vs-product only
[b|W1-G] tests/bindingsnegcheck.cx
    uses : `-m unisa compile --from-tape` control image
    jobs : com-bindings-neg
    do   : control = UA (classic C) on the same tape
[a|W1-E] tests/binsel_audit.py
    uses : gold.BINSEL_OPS/_BIN_U/binsel_label vs host cc (external referee)
    jobs : referee-binsel
    do   : .cx reading weights/gold/binsel.tsv
[b|W2-B] tests/bootstrap.sh
    uses : U branch: `-m unisa compile unisacc.c` (Python front end) = B = C (all.sh only)
    jobs : ccrun-N ccrun-malloc host-triple-N host-triple-u0-prepare nativeboot-N
    do   : drop U branch; B = C fixed point (UA) stays; comboot is the N22 proof
[c|W2-B] tests/ccrun.sh
    uses : expected = `-m unisa run` (Python compiler), VM = `-m unisa vm` of the UA tape
    jobs : ccrun-N ccrun-malloc cli closure-c1a closure-c1b closure-c1c closure-c2a closure-c2b closure-c2c closure-c3a-lnx-arm64 closure-c3a-lnx-x86_64 closure-c3a-osx-arm64 closure-c3a-osx-x86_64 closure-c3a-prepare ...(+47)
    do   : freeze Python run (stdout,rc) per probe into tests/ccrun.expected.tsv while live; tape run via tests/cxlib/tapevm.cx; ccrun.baseline/knownwrong unchanged
[f|W4-B] tests/chainfirstredcheck.sh
    uses : PATHS list incl. unisa
    jobs : -
    do   : drop token
[c|W2-A] tests/closure.sh
    uses : `-m unisa compile --from-tape` reference image + `-m unisa vm` run
    jobs : c99-ledger c99precheck ccrun-N ccrun-malloc cli closure-c1a closure-c1b closure-c1c closure-c2a closure-c2b closure-c2c closure-c3a-lnx-arm64 closure-c3a-lnx-x86_64 closure-c3a-osx-arm64 ...(+79)
    do   : frozen per (probe,target) reference-image sha table captured live (tests/closure.expected.tsv); run via tapevm.cx; REPLAY path kept
[f|W4-B] tests/commitgatecheck.sh
    uses : scratch-repo tree list incl. unisa
    jobs : -
    do   : drop token
[a|W1-E] tests/consts_check.py
    uses : unisa.lower consts + image elf/macho/pe + DATA_BASE vs src/back_*.c
    jobs : -
    do   : .cx: src constants vs exec/facts/lower-data.tsv, enc-address-*.tsv (already exported)
[b|W2-B] tests/corpus.sh
    uses : default driver `-m unisa compile/run` when CORPUS_UA unset (gate corpus-N)
    jobs : c99-ledger com-corpus-N com-lifecycle com-luatests com-luatests-build com-tools corpus-N diag luatests realprog sqlspeed tools-N-N tools-N
    do   : default driver = UA; expected from host cc (as com-corpus)
[b|W2-B] tests/crossnative.sh
    uses : `-m unisa compile/run` want values
    jobs : com-lifecycle winposix
    do   : want from host cc/frozen .want; images by UA
[f|W4-A] tests/cxlib/seedprep.cx
    uses : identity dir list
    jobs : seedgen-ppchain-N-default seedgen-ppchain-N-inherited seedgen-ppchain-N-prepare seedgen-ppconsumer-N-osx-arm64 seedgen-ppconsumer-N-prepare seedgen-ppconsumer-N-win-arm64 seedgen-ppconsumer-N-win-x86_64
    do   : drop "unisa"
[g|W4-C] tests/cxlib/tapevm.cx
    uses : comment: port of unisa/tape.py parse + vm.py
    jobs : com-declmatrix-N com-volatile-comma declmatrix-N lib-callable-variadic-native lib-callable-variadic-native-rosetta lib-callback-forward lib-callback-forward-rosetta lib-callback-native lib-callback-native-rosetta lib-variadic-native lib-variadic-native-rosetta volatile-comma
    do   : becomes THE tape VM (replaces `-m unisa vm`)
[b|W2-B] tests/diag.sh
    uses : `-m unisa tape/run` expected diagnostics
    jobs : c99-ledger diag
    do   : UA/product diagnostics vs frozen expected (com-diag already product-side)
[d|W2-B] tests/difftest-py.knownfail
    uses : Python-route knownfail list
    jobs : -
    do   : delete with difftest-py-N jobs
[g|W2-B] tests/difftest.knownfail
    uses : text mentions Python route
    jobs : -
    do   : reword
[b|W2-B] tests/difftest.sh
    uses : Python branch (`-m unisa run`) when UA unset
    jobs : c99-ledger c99precheck com-corpus-N com-difftest-N com-difftest_o-N com-fb12-multi com-lifecycle com-luatests com-luatests-build com-tools corpus-N diag difftest-N difftest-py-N ...(+7)
    do   : delete Python branch; UA-only
[a|W3-D] tests/docs.sh
    uses : `-m unisa docs --check` (README stage table) + unisa.gold.ALL for referee ledger
    jobs : docs
    do   : .cx: stage table from weights/gold/*.tsv + built.uns2 H; stage list from iterate/kernel/order.tsv
[c|W2-C] tests/edgeablate.cx
    uses : `--produce` uses `-m unisa compile` with UNISA_ABLATE
    jobs : edge-e2-pp-ablate edge-e3-parse-ablate
    do   : gate modes already --gate/--check-frozen; delete --produce Python route (frozen leaves stay)
[a|W2-D2] tests/elfobj.py
    uses : unisa.image.pe.IMPORTS
    jobs : com-elfobj elfobj
    do   : IMPORTS from exec/facts/enc-memorylayout-imports.tsv; port to .cx with elfobj.sh
[a|W2-D2] tests/enc/pereal.py
    uses : reference PE via driver/lower/assemble/image
    jobs : exec-pearm exec-pex86
    do   : port to .cx, referee UA
[a|W2-D2] tests/enc/ref.py
    uses : unisa.assemble reference + reads unisa/assemble.py
    jobs : exec-e5
    do   : port to .cx, referee UA (exec-e5)
[a|W2-D1] tests/enc/tins.py
    uses : unisa.lower.TargetProgram (TIns text tool)
    jobs : com-extent-neg exec-armlower exec-e5 exec-lowdata exec-lower exec-pearm exec-pex86 exec-sparse exec-winlower
    do   : port TIns text tool to tests/cxlib/tins.cx (pure text format, no reference semantics)
[c|W1-G] tests/extentnegcheck.cx
    uses : `-m unisa compile` control + `unisa.tapebin.encode` inputs
    jobs : com-extent-neg
    do   : freeze encoded .tapebin fixtures (tests/fixtures/tapebin/*.bytes); control = UA
[b|W2-B] tests/fat.sh
    uses : `-m unisa fat/run` (Python-route fat binaries)
    jobs : ccrun-N ccrun-malloc cli closure-c1a closure-c1b closure-c1c closure-c2a closure-c2b closure-c2c closure-c3a-lnx-arm64 closure-c3a-lnx-x86_64 closure-c3a-osx-arm64 closure-c3a-osx-x86_64 closure-c3a-prepare ...(+47)
    do   : UA/product fat (-b fat) + host run vs frozen .want; or (d) if product fat is covered by com-* jobs
[c|W1-H] tests/flownetbaseline.cx
    uses : --check acc/out1-4/check/selftest spawn `python3 -m m4` (-> unisa.intnet)
    jobs : flownet-baseline-acc flownet-baseline-check flownet-baseline-file flownet-baseline-out1 flownet-baseline-out2 flownet-baseline-out3 flownet-baseline-out4 flownet-baseline-selftest
    do   : keep --check-file (frozen tests/flownet-baseline.tsv); delete the 7 m4-respawn jobs
[f|W4-A] tests/freezecheck.cx
    uses : CLOSURE prefixes incl. "unisa/"
    jobs : freezecheck freezecheck-selftest
    do   : drop token
[b|W2-C] tests/fwrite/README
    uses : `-m unisa run` third route (ungated)
    jobs : -
    do   : drop Python route; UA + product only
[b|W2-C] tests/fwrite/real.sh
    uses : `-m unisa run` third route (ungated)
    jobs : -
    do   : drop Python route; UA + product only
[b|W2-C] tests/fwrite/sim.sh
    uses : `-m unisa run` third route (ungated)
    jobs : -
    do   : drop Python route; UA + product only
[f|W4-B] tests/gate.sh
    uses : K6 job table (29 unisa-* jobs), tapebin-shape/tapebin-roundtrip jobs, difftest-py, comments
    jobs : "seed-construct-N" c99-ledger com-csmithdiff-N com-declmatrix-N com-fb12-multi com-forceinclude com-luatests com-luatests-build com-seedgen com-tools csmithdiff-N declmatrix-N diag exec-armself ...(+49)
    do   : remove deleted jobs; each wave edits its own jobs; final sweep here
[f|W4-B] tests/gatedecl.py
    uses : PRODUCT prefixes incl. 'unisa/'
    jobs : -
    do   : drop token
[f|W4-B] tests/gatedeclcheck.py
    uses : PRODUCT prefixes incl. 'unisa/'
    jobs : -
    do   : drop token
[f|W4-B] tests/gatedeps.json
    uses : 740 suites list tree 'unisa'; 49 list unisa/*.py files
    jobs : -
    do   : regenerate with tests/refreshgatedeps.cx after its lists drop unisa
[f|W4-B] tests/gatelayers.py
    uses : unisa-* job names
    jobs : -
    do   : drop deleted job names / rename re-pointed K6 jobs
[f|W4-B] tests/gateselect.tsv
    uses : unisa-* job names
    jobs : -
    do   : drop deleted job names / rename re-pointed K6 jobs
[f|W4-B] tests/gateselectcheck.py
    uses : unisa-* job names
    jobs : -
    do   : drop deleted job names / rename re-pointed K6 jobs
[f|W4-B] tests/stagemap.c
    uses : unisa-* job names
    jobs : -
    do   : drop deleted job names / rename re-pointed K6 jobs
[f|W4-B] tests/gatequeue.py
    uses : git status paths incl. 'unisa'
    jobs : -
    do   : drop token
[a|W1-E] tests/gold_audit.py
    uses : unisa.gold type table vs host cc (A-50 external referee)
    jobs : -
    do   : .cx reading weights/gold/type.tsv
[e|W3-E] tests/headerchain.cx
    uses : steps: `-m unisa emit-kernel`, `python3 unisa/libneed.py`
    jobs : -
    do   : steps become the .cx headers generator (W3-A) and seed libneed checker (W3-E)
[e|W3-E] tests/headerchain.sh
    uses : steps: `-m unisa emit-kernel`, `python3 unisa/libneed.py`
    jobs : -
    do   : steps become the .cx headers generator (W3-A) and seed libneed checker (W3-E)
[c|W1-G] tests/hostaddrnegcheck.cx
    uses : `-m unisa compile` control + tapebin.encode inputs
    jobs : com-hostaddr-neg
    do   : frozen fixtures + UA control
[c|W1-F] tests/hostsymbolscheck.cx
    uses : unisa.hostsymbols.produce / hostbind.bind reference
    jobs : exec-hostsymbols-model
    do   : frozen reference outputs per case
[b|W1-G] tests/inputcontractcheck.cx
    uses : `-m unisa compile` control on empty tape
    jobs : com-input-contract
    do   : control = UA
[f|W4-B] tests/k2bcheck.py
    uses : mutates unisa/vm.py, unisa/fp.py, adds unisa/k2bnew.py as whitelist fixtures
    jobs : -
    do   : retarget fixture paths to another product-closure tree (literal change only)
[e|W3-A] tests/kernel.sh
    uses : `-m unisa emit-kernel` + `gold-export`, byte-compare kernel/*, weights/gold/*
    jobs : exec-e4-N exec-e4self host-triple-N host-triple-u0-prepare kernel nativeboot-N net-abi-seal net-binsel-seal net-combo-seal net-enc-seal net-irsel-seal net-isel-seal net-lex-seal net-opinfo-seal ...(+22)
    do   : HARD CORE: replace with .cx: regenerate unisa_headers.inc from include/ (new .cx generator), provenance-pin the frozen rest
[c|W2-A] tests/layout_check.py
    uses : both back ends (Python reference via assemble/image/lower/_oracle) over enumerated tapes
    jobs : -
    do   : freeze reference image sha per enumerated tape (captured live); .cx (layout.sh: Makefile quick/all.sh)
[d|W1-A] tests/lexdiff.sh
    uses : Python lexer (front.pp/lex, oracle) vs C lexer
    jobs : ccrun-N ccrun-malloc
    do   : reached via selfhost/ccrun; the C lexer is the reference now; delete
[e|W3-E] tests/libneed.sh
    uses : `python3 unisa/libneed.py` guard/table check
    jobs : cxinclude cxmodule-classic cxmodule-neg cxmodule-pos e3-seal elf-seal hosthdr identity-include-consumer-negative identity-include-negative identity-prep-negative identity-product-negative lex-e1-seal libneed lower-seal ...(+27)
    do   : seed/libneed (promoted from seed/unisa_libneed.c)
[c|W1-F] tests/librarycallbackpartitioncheck.cx
    uses : `-m unisa.librarycallbackpartition --oracle`
    jobs : exec-librarycallbackpartition-model-N
    do   : frozen reference ledger per case
[c|W1-F] tests/librarycallsitepartitioncheck.cx
    uses : `-m unisa.librarycallsitepartition --oracle`
    jobs : exec-librarycallsitepartition-model-N
    do   : frozen
[c|W1-F] tests/librarycontractcheck.cx
    uses : unisa.librarycontract.produce
    jobs : exec-librarycontract-model
    do   : frozen
[c|W1-F] tests/librarydatacheck.cx
    uses : unisa.librarydatalower.fragment
    jobs : exec-librarydata-model
    do   : frozen
[c|W1-F] tests/libraryfncheck.cx
    uses : unisa.libraryfnlower.fragment
    jobs : exec-libraryfn-model
    do   : frozen
[c|W1-F] tests/libraryfunctionimportscheck.cx
    uses : unisa.libraryfunctionimports.produce
    jobs : exec-libraryfunctionimports-model
    do   : frozen
[c|W1-F] tests/librarypartitioncheck.cx
    uses : unisa.librarypartition.produce
    jobs : exec-librarypartition-model
    do   : frozen
[c|W1-F] tests/librarytypedoverlaycheck.cx
    uses : unisa.librarytypedpartition/libraryfunctionimports.produce
    jobs : exec-librarycallsite-reader-model exec-librarytypedreader-model
    do   : frozen
[c|W1-F] tests/librarytypedpartitioncheck.cx
    uses : unisa.librarytypedpartition.produce
    jobs : exec-librarytypedpartition-model-N
    do   : frozen
[c|W1-F] tests/librarytypedreadercheck.cx
    uses : `-m unisa.librarytypedreader` reference
    jobs : exec-librarycallsite-reader-model exec-librarytypedreader-model
    do   : frozen
[f|W4-A] tests/libraryunioncompositecheck.cx
    uses : identity dir list; "unisa" token scan of #pragma
    jobs : lib-union-composite-callback-N lib-union-composite-callback-N-fp9 lib-union-composite-callback-N-gp9 lib-union-composite-callback-N-plain lib-union-composite-callback-N-rosetta lib-union-composite-direct-N lib-union-composite-direct-N-a lib-union-composite-direct-N-b lib-union-composite-direct-N-rosetta lib-union-composite-direct-N-req lib-union-composite-prepare
    do   : drop "unisa" from dir list (pragma scan stays: product language)
[f|W4-A] tests/libraryunionscalarcheck.cx
    uses : identity dir list; "unisa" token scan of #pragma
    jobs : lib-union-scalar-N lib-union-scalar-N-rosetta lib-union-scalar-double8-fp9 lib-union-scalar-double8-gp9 lib-union-scalar-double8-plain lib-union-scalar-float4-fp9 lib-union-scalar-float4-gp9 lib-union-scalar-float4-plain lib-union-scalar-int8-fp9 lib-union-scalar-int8-gp9 lib-union-scalar-int8-plain lib-union-scalar-mixed-reordered-fp9 lib-union-scalar-mixed-reordered-gp9 lib-union-scalar-mixed-reordered-plain ...(+8)
    do   : drop "unisa" from dir list (pragma scan stays: product language)
[c|W2-D3] tests/libunisacccheck.py
    uses : unisa.opt.optimise reference optimiser (lib-context)
    jobs : lib-context
    do   : freeze optimise() outputs for its fixtures; .cx
[d|W1-B] tests/libunisaccsymbolcheck.py
    uses : reference pipeline (ungated)
    jobs : -
    do   : ungated; delete + row
[g|-] tests/linux.sh
    uses : host dir names ~/unisa-linux
    jobs : apps-real boundnested-N boundreason boundstall-r7g com-declmatrix-N com-lifecycle combo declmatrix-N docs exec-librarycallback-reader-model exec-librarycallbacklower-model-N exec-warningdriver-N exec-warningdriver-ua-N exec-warnprep-N ...(+71)
    do   : none
[c|W2-D3] tests/modelsignature3check.py
    uses : unisa.signature3.oracle/equal
    jobs : lib-sig3-canonical lib-sig3-equality lib-source-import-compat
    do   : freeze oracle verdicts for its hand-built graphs; .cx (modelsourceimportcompatcheck.py follows)
[c|W2-D3] tests/modelsourceimportcompatcheck.py
    uses : via modelsignature3check.py
    jobs : lib-source-import-compat
    do   : follows
[b|W2-B] tests/multi.sh
    uses : `-m unisa compile/run` multi-TU
    jobs : multi
    do   : UA (com-multi already product)
[b|W2-B] tests/native.sh
    uses : `-m unisa run/compile` native host run
    jobs : c99-ledger ccrun-N ccrun-malloc cli closure-c1a closure-c1b closure-c1c closure-c2a closure-c2b closure-c2c closure-c3a-lnx-arm64 closure-c3a-lnx-x86_64 closure-c3a-osx-arm64 closure-c3a-osx-x86_64 ...(+59)
    do   : UA/product + host cc expected
[c|W2-A] tests/opt.sh
    uses : OPT_PART closure: Python back end lowers C -O2 tape (`-m unisa compile --from-tape`)
    jobs : opt-run-N
    do   : frozen image sha (as closure); run part unaffected
[d|W1-A] tests/optpy.sh
    uses : unisa.opt.optimise vs C -O1/-O2 tape (all.sh only)
    jobs : -
    do   : the Python optimiser is archived; C opt is the reference; delete
[a|W1-E] tests/pfconv_audit.py
    uses : unisa.gold pfconv table vs host printf
    jobs : referee-pfconv
    do   : .cx reading weights/gold/pfconv.tsv
[f|W4-B] tests/pipelinecache.py
    uses : cache tree list incl. 'unisa'
    jobs : pipeline-cache
    do   : drop token
[f|W2-E] tests/ppleafcapture.cx
    uses : role classifier treats paths starting "unisa/" (and literalcheck.py) as harness deps
    jobs : pp-leaf-capture-selftest
    do   : drop the unisa/ branch after literalcheck port + pp-leaf reseal
[c|W2-E] tests/pptruthcheck.py
    uses : unisa.front.pp #if evaluator as truth
    jobs : pptruth
    do   : freeze pp truth rows; .cx
[f|W4-A] tests/provenancecheck.py
    uses : identity dir fixture incl. 'unisa'
    jobs : -
    do   : drop token
[d|W2-B] tests/pyfront.knownfail
    uses : Python front-end knownfail list
    jobs : -
    do   : delete (fat/native/ccrun/lexdiff/precheck stop reading it)
[f|W4-B] tests/queuecheck.py
    uses : fixture dirs incl. 'unisa'
    jobs : -
    do   : drop token
[f|W4-B] tests/refreshgatedeps.cx
    uses : generator lists tree 'unisa' and unisa/*.py files
    jobs : -
    do   : drop; regenerate gatedeps.json
[f|W4-B] tests/release.sh
    uses : DECLARED tree list incl. unisa
    jobs : -
    do   : drop token
[d|W2-F] tests/reseal-prune.sh
    uses : unisa.prune.prune_text referee for resealing exec-prune (ungated)
    jobs : -
    do   : seal frozen; reseal from product-side capture only; delete referee step
[g|W4-C] tests/reviewedtreescheck.cx
    uses : comment: derived map routes unisa/ changes here
    jobs : -
    do   : reword
[b|W2-C] tests/revivedscan.cx
    uses : Python-route lists run `-m unisa run`; --changed watches unisa/front
    jobs : revivedscan
    do   : drop Python-route lists (pyfront/difftest-py); watch src/front_* only
[g|-] tests/seals/exec-net-*/model-*.inc (17)  (17 files)
    files: tests/seals/exec-net-abi/model-b89cba9f4d9758d327cb0d854932668e93994ec4f1c5e0a80619713e1468dcbc.inc tests/seals/exec-net-binsel/model-6b455f6da918a2e0f28ef04312d9ebfb2cfea6fe06c25067406364627b9cbd48.inc tests/seals/exec-net-combo/model-4c8fd4d4b4105ae98876eed58b7d421c74a9745f907fd316aefad919a08e0022.inc tests/seals/exec-net-enc/model-156c750b0556d03d1ff3887047177579ebddf5c19cc9efb98b9e4e9d8d7baf36.inc tests/seals/exec-net-irsel/model-b876034a8f4bfb519defa91e84812d6c1dfab698516ab411a87a38a5655e8b40.inc tests/seals/exec-net-isel/model-c1a0ef29f8312acb3318e07642cf236427b4f853d5f59aa9c9a846a0d9c25c35.inc tests/seals/exec-net-lex/model-0f81a675f3cf22f7f234df5e1a240ad11468fc25cf8f7d30496433056a9911c6.inc tests/seals/exec-net-opinfo/model-f2f9196ad8ecfe5869d04f8a138276fbe3af50781b4067aa1b44f051ee41998a.inc tests/seals/exec-net-parse/model-999402fd28c53fde797d8bc8bbad9e198396b7e72c3505c4afee927b21c41c43.inc tests/seals/exec-net-peep/model-b8db677586800735a942c9ed585f651c229d76990d5128562752b5921359488d.inc tests/seals/exec-net-pfconv/model-39309bf75d9dd521a19906a35007015062692bd3b21a38bc5cdb2516af4841c6.inc tests/seals/exec-net-prec/model-43fff91b24dd27eb7c1b9db8fb9f144c09149854008f87f6f71f83d032d1e5d1.inc tests/seals/exec-net-regmap/model-17f28a23cd0d00da29f21e295924a3b66731c3038bdd4bc673c138a149d8152a.inc tests/seals/exec-net-reloc/model-d3a709aeaf753859831f51c42af543995700667a24118704b9c3f407ead5f6d3.inc tests/seals/exec-net-scope/model-dc01446031d21f117237bcc10ba4ad484d9d2d59ba061256ed6e0e2422c5e06e.inc tests/seals/exec-net-tyinfo/model-fb4abf4218ef53c5bce645b91a1ab20db25e2262f7a7b66dadbcad09f3c43399.inc tests/seals/exec-net-type/model-e2e400d0b63b9c654eadce0a9b4c8536a428fd3c457422ac1d214fa4ac847de8.inc ...
    uses : frozen copies of kernel/weight.*.inc incl. emit-kernel header
    jobs : -
    do   : frozen bytes; leave
[g|-] tests/seals/exec-pp-literals/resource-{252daeb934c1e5553d7530a5e29c525d8ace601838ce6b388d98b812978bc065,e941dc8a7a3f98c2c964494c2ef4a0e4bb62565044d1c12babd80fb58441236f,99b04c4d5d015289b1ac0336d08678603c84cd55564caaa84f6061a9e2a6a57e,5e92adaa7ccc3d5a6d6bd8888dc7bfb3109ebd9450ecffeb5d8a643752b1e15b,4ad3dd31b0be601673cb02dbc4058ae3e092a249bdc67ede24a898e2a209a5cc}.bytes (5)  (5 files)
    files: tests/seals/exec-pp-literals/resource-252daeb934c1e5553d7530a5e29c525d8ace601838ce6b388d98b812978bc065.bytes tests/seals/exec-pp-literals/resource-4ad3dd31b0be601673cb02dbc4058ae3e092a249bdc67ede24a898e2a209a5cc.bytes tests/seals/exec-pp-literals/resource-5e92adaa7ccc3d5a6d6bd8888dc7bfb3109ebd9450ecffeb5d8a643752b1e15b.bytes tests/seals/exec-pp-literals/resource-99b04c4d5d015289b1ac0336d08678603c84cd55564caaa84f6061a9e2a6a57e.bytes tests/seals/exec-pp-literals/resource-e941dc8a7a3f98c2c964494c2ef4a0e4bb62565044d1c12babd80fb58441236f.bytes
    uses : copies of include/*.h prose
    jobs : -
    do   : frozen bytes
[c|W2-E] tests/seals/exec-pp-literals/closure.expected.tsv
    uses : builder/harness rows unisa/__init__.py, unisa/front/__init__.py
    jobs : -
    do   : reseal after literalcheck port
[c|W2-E] tests/seals/exec-pp-literals/dependencies.tsv
    uses : builder/harness rows unisa/__init__.py, unisa/front/__init__.py
    jobs : -
    do   : reseal after literalcheck port
[g|W4-C] tests/seals/exec-prune/cell.tsv
    uses : text: Python referee frozen at reseal
    jobs : -
    do   : reword optional; frozen
[g|W4-C] tests/seals/exec-prune/measurement.tsv
    uses : text: Python referee frozen at reseal
    jobs : -
    do   : reword optional; frozen
[c|W1-F] tests/seals/librarytypedreader/overlay.json
    uses : sha of unisa/librarytypedreader.py
    jobs : -
    do   : reseal without the Python reference entry
[c|W1-F] tests/seals/librarytypedreader/receipt.json
    uses : sha of unisa/librarytypedreader.py
    jobs : -
    do   : reseal without the Python reference entry
[d|W2-F] tests/seedapecheck.sh
    uses : seed/ape.c container == `-m unisa ape` (byte)
    jobs : com-auditnet com-comboot-fixedpoint com-comboot-seed com-comboot-stage2 com-comboot-stage3 exec-bindprep-N exec-bindprep-first-N exec-bindprep-second-N exec-container exec-driver-core-N exec-driver-core-prep-N exec-driver-core-prep-f1json exec-driver-language-N exec-driver-resources-headers ...(+16)
    do   : seed/ape.c is the only container writer; comboot fixed point + ape-version cover it; delete
[f|W4-A] tests/seedmatrixprepared.cx
    uses : identity dir list
    jobs : seed-matrix-features-N seed-matrix-lnx-arm64 seed-matrix-lnx-x86_64 seed-matrix-object seed-matrix-osx-arm64 seed-matrix-osx-x86_64 seed-matrix-shared-e1 seed-matrix-shared-e2 seed-matrix-shared-e3 seed-matrix-shared-e4 seed-matrix-shared-nativeabi seed-matrix-shared-o1 seed-matrix-shared-prepare seed-matrix-shared-prune ...(+9)
    do   : drop token
[f|W4-B] tests/seedoptcheck.sh
    uses : `git archive ... exec seed unisa ...` scratch export lists
    jobs : seedgen-opt seedgen-pp-N
    do   : drop 'unisa' (verify nothing in the export imports it after W1-W3)
[f|W4-B] tests/seedoptconsumercheck.sh
    uses : `git archive ... exec seed unisa ...` scratch export lists
    jobs : seedgen-optconsumer-N
    do   : drop 'unisa' (verify nothing in the export imports it after W1-W3)
[f|W4-B] tests/seedparse2consumercheck.sh
    uses : `git archive ... exec seed unisa ...` scratch export lists
    jobs : seedgen-parse2consumer-N seedgen-parse2consumer-N-prepare seedgen-pruneconsumer-N
    do   : drop 'unisa' (verify nothing in the export imports it after W1-W3)
[f|W4-B] tests/seedppchaincheck.sh
    uses : `git archive ... exec seed unisa ...` scratch export lists
    jobs : seedgen-ppchain-N-default seedgen-ppchain-N-inherited seedgen-ppchain-N-prepare seedgen-ppchain-N seedgen-ppconsumer-N-osx-arm64 seedgen-ppconsumer-N-prepare seedgen-ppconsumer-N-win-arm64 seedgen-ppconsumer-N-win-x86_64
    do   : drop 'unisa' (verify nothing in the export imports it after W1-W3)
[f|W4-B] tests/seedppcheck.sh
    uses : `git archive ... exec seed unisa ...` scratch export lists
    jobs : seedgen-pp-N
    do   : drop 'unisa' (verify nothing in the export imports it after W1-W3)
[f|W4-B] tests/seedppconsumercheck.sh
    uses : `git archive ... exec seed unisa ...` scratch export lists
    jobs : seedgen-ppchain-N-default seedgen-ppchain-N-inherited seedgen-ppchain-N-prepare seedgen-ppconsumer-N-lnx-arm64 seedgen-ppconsumer-N-lnx-x86_64 seedgen-ppconsumer-N-negative seedgen-ppconsumer-N-osx-x86_64 seedgen-ppconsumer-N-osx-arm64 seedgen-ppconsumer-N-prepare seedgen-ppconsumer-N-win-arm64 seedgen-ppconsumer-N-win-x86_64
    do   : drop 'unisa' (verify nothing in the export imports it after W1-W3)
[f|W4-B] tests/seedpprun.sh
    uses : `git archive ... exec seed unisa ...` scratch export lists
    jobs : seedgen-pprun
    do   : drop 'unisa' (verify nothing in the export imports it after W1-W3)
[f|W4-B] tests/seedpruneconsumercheck.sh
    uses : `git archive ... exec seed unisa ...` scratch export lists
    jobs : seedgen-pruneconsumer-N
    do   : drop 'unisa' (verify nothing in the export imports it after W1-W3)
[b|W2-B] tests/selfgap.sh
    uses : `-m unisa tape` (Python front end) acceptance ratchet
    jobs : ccrun-N ccrun-malloc closure-c1a closure-c1b closure-c1c closure-c2a closure-c2b closure-c2c closure-c3a-lnx-arm64 closure-c3a-lnx-x86_64 closure-c3a-osx-arm64 closure-c3a-osx-x86_64 closure-c3a-prepare closure-c3a-win-arm64 ...(+40)
    do   : ratchet against host cc acceptance instead; or (d)
[d|W1-A] tests/selfhost.sh
    uses : `-m unisa compile` Python-built unisacc + lexdiff (all.sh selfhost-prepare-*)
    jobs : ccrun-N ccrun-malloc
    do   : bootstrap/comboot supersede; delete + all.sh rows
[d|W1-A] tests/selfhost_prepare.py
    uses : `-m unisa tape/compile`
    jobs : ccrun-N ccrun-malloc
    do   : delete with selfhost.sh
[f|W4-A] tests/shipclassify.cx
    uses : CLOSURE prefixes incl. "unisa/"
    jobs : -
    do   : drop token
[a|W1-E] tests/source_layout_check.py
    uses : unisa ckernel/gold/intnet (source/data boundary)
    jobs : source-layout
    do   : .cx reading kernel/ and weights/gold
[c|W2-C] tests/stages.sh
    uses : Python oracle/driver asked-set per probe (A-33)
    jobs : edge-e3-pfconv-witness edge-elf-enc-witness stages-1a stages-1b stages-1c stages-2a stages-2b stages-2c stages-3a stages-3b stages-3c stages-back
    do   : freeze Python asked-set per probe; .cx compares product -v asked line
[b|W2-C] tests/staticunits.sh
    uses : `-m unisa run` third route
    jobs : staticunits
    do   : cc vs UA vs product only (com-staticunits)
[b|W2-C] tests/strconvertroute.py
    uses : seed route `-m unisa run`
    jobs : strconvert-N
    do   : drop seed route (classic+model remain)
[b|W2-C] tests/tagforward.sh
    uses : `-m unisa tape` route
    jobs : tagforward
    do   : drop Python route
[d|W1-G] tests/tapebin.py
    uses : unisa.tape/tapebin codec self-test + C/Python byte identity (tapebin-roundtrip-*, unisa-tapebin)
    jobs : tapebin-roundtrip-codec tapebin-roundtrip-structure-N unisa-tapebin
    do   : structure-N test the archived codec (d); codec parity folded into frozen tapebin fixtures (c)
[c|W1-G] tests/tapebinnegcheck.cx
    uses : unisa.tapebin.encode/target_id inputs + `-m unisa compile` control
    jobs : com-tapebin-neg
    do   : frozen fixtures + UA control
[c|W1-G] tests/tapebinproduct.cx
    uses : comment: shard part not ported (compares vs Python codec)
    jobs : com-tapebin-cover com-tapebin-six
    do   : port shard part against frozen per-file tapebin sha (tests/tapebin-expected.tsv)
[c|W1-G] tests/tapebinproduct.py
    uses : unisa.tapebin encode/decode/target_id parity (com-tapebin-N)
    jobs : com-tapebin-N com-tapebin-cover com-tapebin-ref com-tapebin-six
    do   : superseded by the .cx shard part; delete
[g|W1-F] tests/tapecheck-cases.tsv
    uses : header: decisions obtained from unisa.tape.parse
    jobs : tapecheck-model-N
    do   : reword when tapecheckmodel.cx gets its frozen verdict column
[c|W1-F] tests/tapecheckmodel.cx
    uses : unisa.tape.parse as oracle
    jobs : tapecheck-model-N
    do   : freeze accept/refuse verdicts per case; .cx
[c|W1-G] tests/tapereadercheck.py
    uses : unisa.tapebin.encode to build inputs (tape-reader)
    jobs : tape-reader
    do   : frozen tapebin fixtures; .cx
[b|W1-G] tests/tapestructcheck.cx
    uses : `-m unisa compile` controls
    jobs : com-tape-struct
    do   : control = UA
[b|W2-B] tests/tools.sh
    uses : default TOOLS_UA = `-m unisa compile`; sha list over unisa
    jobs : com-luatests com-luatests-build com-tools luatests realprog sqlspeed tools-N-N tools-N
    do   : default driver UA; drop unisa from sha list
[a|W1-E] tests/tsv_contract.py
    uses : unisa.tsvgold reader must reject damaged prec.tsv
    jobs : -
    do   : .cx: iterate/construct/construct.c reader (same contract, J10) must reject the damaged tables
[e|W3-B] tests/tsvbuild_check.py
    uses : gold.ALL, intnet, tsvgold, uns2: fresh construction vs shipped built.*
    jobs : tsv-build-account
    do   : construct.c -u all stages == weights/built.uns2 (C only); built.json frozen
[f|W4-B] tests/ungated-checks.tsv
    uses : rows for unisa/*.py units and deleted tools
    jobs : com-source-name
    do   : delete rows
[c|W1-C] tests/unisagold{binsel,irsel,lex,opinfo,parse,peep,pp,reloc,scope,type}labelcheck.sh, tests/unisagoldpreccheck.sh (11)  (11 files)
    files: tests/unisagoldbinsellabelcheck.sh tests/unisagoldirsellabelcheck.sh tests/unisagoldlexlabelcheck.sh tests/unisagoldopinfolabelcheck.sh tests/unisagoldparselabelcheck.sh tests/unisagoldpeeplabelcheck.sh tests/unisagoldpplabelcheck.sh tests/unisagoldpreccheck.sh tests/unisagoldreloclabelcheck.sh tests/unisagoldscopelabelcheck.sh tests/unisagoldtypelabelcheck.sh
    uses : K6: C label ref vs gold.py source (ast extraction)
    jobs : unisa-gold-binsel-label unisa-gold-irsel-label unisa-gold-lex-label unisa-gold-opinfo-label unisa-gold-parse-label unisa-gold-peep-label unisa-gold-pp-label unisa-gold-prec unisa-gold-reloc-label unisa-gold-scope-label unisa-gold-type-label
    do   : replace by one .cx comparing `seed/gold_*_label table` with weights/gold/<stage>.tsv rows
[c|W1-D] tests/unisa{ape,bits,catalog,emitarmnwmem,fp,hostabi,imageelf,imagefat,imagemacho,imagepe,libneed,linalg,padded,peversion,tape,tapebin,tapebinshape,uns1,vm,zdata}check.sh (20)  (20 files)
    files: tests/unisaapecheck.sh tests/unisabitscheck.sh tests/unisacatalogcheck.sh tests/unisaemitarmnwmemcheck.sh tests/unisafpcheck.sh tests/unisahostabicheck.sh tests/unisaimageelfcheck.sh tests/unisaimagefatcheck.sh tests/unisaimagemachocheck.sh tests/unisaimagepecheck.sh tests/unisalibneedcheck.sh tests/unisalinalgcheck.sh tests/unisapaddedcheck.sh tests/unisapeversioncheck.sh tests/unisatapebincheck.sh tests/unisatapebinshapecheck.sh tests/unisatapecheck.sh tests/unisauns1check.sh tests/unisavmcheck.sh tests/unisazdatacheck.sh
    uses : K6: C ref vs unisa module (python side imports module, sha of unisa/*.py)
    jobs : unisa-ape unisa-bits unisa-catalog unisa-emit-arm-nwmem unisa-fp unisa-hostabi unisa-image-elf unisa-image-fat unisa-image-macho unisa-image-pe unisa-libneed unisa-linalg ...
    do   : replace all 20 by one tests/k6frozen.cx over tests/k6/<name>.tsv (libneed and tapebin-shape move to W3-E/W3-D generators)
[b|W2-B] tests/vm.sh
    uses : `-m unisa vm tests/tapes/*.tape`
    jobs : -
    do   : tapevm.cx (all.sh only)
[g|W4-C] tests/volatilecomma.cx
    uses : comment: tapevm.cx = unisa tape/vm port
    jobs : com-declmatrix-N com-volatile-comma declmatrix-N lib-callable-variadic-native lib-callable-variadic-native-rosetta lib-callback-forward lib-callback-forward-rosetta lib-callback-native lib-callback-native-rosetta lib-variadic-native lib-variadic-native-rosetta volatile-comma
    do   : reword
[d|W1-A] tests/volatilecomma.py
    uses : unisa.tape/vm (superseded by volatilecomma.cx)
    jobs : com-declmatrix-N com-volatile-comma declmatrix-N lib-callable-variadic-native lib-callable-variadic-native-rosetta lib-callback-forward lib-callback-forward-rosetta lib-callback-native lib-callback-native-rosetta lib-variadic-native lib-variadic-native-rosetta volatile-comma
    do   : delete (gate job already uses the .cx; k2bcheck fixture reference retarget)
[b|W2-C] tests/winsuite.py
    uses : expected = `-m unisa run --target`, builds via `-m unisa compile`
    jobs : winposix
    do   : build with UA -b win/*, expected from host cc / frozen manifest
[d|W3-F] ujs/seed/construct/api.py
    uses : unisa.intnet.build/save_all (UJS weight construction)
    jobs : -
    do   : archive ujs/seed with unisa (UJS line's Python seed; flownet .cx + frozen adapter tables replace it) -- see H8
[d|W3-F] ujs/seed/construct/build/browser.py
    uses : unisa.intnet.build/save_all (UJS weight construction)
    jobs : -
    do   : archive ujs/seed with unisa (UJS line's Python seed; flownet .cx + frozen adapter tables replace it) -- see H8
[d|W3-F] ujs/seed/construct/build/ic_c.py
    uses : unisa.intnet.build/save_all (UJS weight construction)
    jobs : -
    do   : archive ujs/seed with unisa (UJS line's Python seed; flownet .cx + frozen adapter tables replace it) -- see H8
[d|W3-F] ujs/seed/construct/build/web.py
    uses : unisa.intnet.build/save_all (UJS weight construction)
    jobs : -
    do   : archive ujs/seed with unisa (UJS line's Python seed; flownet .cx + frozen adapter tables replace it) -- see H8
[d|W3-F] ujs/seed/construct/cli.py
    uses : unisa.intnet.build/save_all (UJS weight construction)
    jobs : -
    do   : archive ujs/seed with unisa (UJS line's Python seed; flownet .cx + frozen adapter tables replace it) -- see H8
[e|W3-B] weights/built.json, weights/built.uns2, weights/*.f32.unisa, weights/*.i8.unisa  (24 files)
    files: weights/abi.f32.unisa weights/abi.i8.unisa weights/built.json weights/built.uns2 weights/combo.f32.unisa weights/combo.i8.unisa weights/enc.f32.unisa weights/enc.i8.unisa weights/irsel.f32.unisa weights/irsel.i8.unisa weights/isel.f32.unisa weights/isel.i8.unisa weights/lex.f32.unisa weights/lex.i8.unisa weights/parse.f32.unisa weights/parse.i8.unisa weights/pp.f32.unisa weights/pp.i8.unisa weights/reloc.f32.unisa weights/reloc.i8.unisa weights/scope.f32.unisa weights/scope.i8.unisa weights/type.f32.unisa weights/type.i8.unisa
    uses : build-weights / dump-weights output
    jobs : acc (all.sh), CI build-weights; read by emit-kernel (unisa_model.inc), tsv-build-account, iterate/*
    do   : freeze; built.uns2 re-derivable by construct.c (C); *.unisa dumps only read by acceptance.sh -> keep frozen or delete with it
[e|W3-B] weights/gold/{tyinfo,abi,binsel,combo,enc,irsel,isel,lex,scope,reloc,type,pp,pfconv,parse,prec,peep,opinfo,regmap}.tsv (18)  (18 files)
    files: weights/gold/abi.tsv weights/gold/binsel.tsv weights/gold/combo.tsv weights/gold/enc.tsv weights/gold/irsel.tsv weights/gold/isel.tsv weights/gold/lex.tsv weights/gold/opinfo.tsv weights/gold/parse.tsv weights/gold/peep.tsv weights/gold/pfconv.tsv weights/gold/pp.tsv weights/gold/prec.tsv weights/gold/regmap.tsv weights/gold/reloc.tsv weights/gold/scope.tsv weights/gold/tyinfo.tsv weights/gold/type.tsv
    uses : gold-export output of unisa/gold.py
    jobs : kernel (gold-export + cmp); read by construct.c, export.py, seed tools, flownet, audits
    do   : freeze: TSVs become the authority (already read by construct.c, export.py, seed tools, flownet)

2. DECISIONS BY FAMILY (rationale; per-file rows are in section 1)
--------------------------------------------------------------------
F1 Python-seed self-tests (d, W1-A): acceptance.sh, baseline.sh, artifacts.sh, ape.sh+ape_stage.py, lexdiff.sh,
   optpy.sh, selfhost.sh+selfhost_prepare.py, volatilecomma.py.  They test the archived seed itself (its CLI, its
   SGD control arm, its packaging, its lexer/optimiser) and run only from tests/all.sh / Makefile; the gate already
   has product-side forms (comboot, seedape->comboot, volatile-comma .cx, difftest/difftest_o, com-*).  Delete,
   plus their all.sh rows and the referrers tests/opt_partition_check.py, tests/ciplancheck.py.
F2 Ungated Python referees (d, W1-B): exec/enc/libraryexitcheck.py (+exec/lower/libraryexitcheck.py caller),
   exec/lower/librarycallback-oracle.py, exec/prune/libraryrootscheck.py, tests/libunisaccsymbolcheck.py, and the
   exec/lex/net.py + stage.py UNS2 experiment (exec/lex/rules.md calls it "older, separate"; run.sh `net` branch,
   decisionledger.py list).  tests/ungated-checks.tsv itself says the gated .cx checks own the verdicts.
F3 K6 C references (W1-C, W1-D; class c): 31 jobs `unisa-*` (gate.sh 112-142 req table, 976-1006 jobs) compare
   seed/unisa_*.c against the Python module ("keep Python").  With the Python archived the reference side must be
   frozen, not dropped, because the C refs are the only executable copy of that knowledge outside the archive.
   - 11 gold label refs (prec, lex, type, binsel, opinfo, peep, scope, pp, reloc, irsel, parse): re-point to the
     frozen authority -- `seed/gold_<stage>_label table` must reproduce weights/gold/<stage>.tsv rows.  That is a
     genuine second derivation of the frozen gold (rule form vs table form), not a copy.  One .cx, 11 jobs or 1.
   - 18 others: ONE generic runner tests/k6frozen.cx over tests/k6/<name>.tsv (argv, stdin-hex, expected stdout,
     rc) captured from the Python side of each current harness while unisa is live.  Paradigm-level: one checker,
     data per module (owner's abstraction-first rule).
   - K6-6 libneed and K6-11b tapebin_shape are promoted to generators/checkers (H6, H5) instead.
F4 External referees (a, W1-E): gold_audit (type vs host cc), abi_audit (syscall numbers vs host headers),
   binsel_audit, pfconv_audit, consts_check, source_layout_check, tsv_contract.  unisa is only their table
   enumerator; the referee is the host.  Read weights/gold/*.tsv / exec/facts / kernel/ instead.  tsv_contract's
   "damaged table must be rejected" moves to the C reader (iterate/construct/construct.c), which has the same
   contract (J10).
F5 Library-model checks (c, W1-F): 12 tests/*check.cx + exec/parse2/librarycallsitecheck.cx +
   exec/lower/library{callsite,typed}-oracle.py + tapecheckmodel.cx spawn `python3 -c "from unisa.libraryX import
   produce"` per case as the reference.  Each already iterates a committed cases TSV; add the reference output
   (hex) + rc per case, captured live, and compare the model against it.  Reseal tests/seals/librarytypedreader.
F6 Tape codec + refusal controls (W1-G): tests/tapebin.py structure-N test the archived codec (d); the C/Python
   parity of `tapebin-roundtrip-codec`, com-tapebin-N (tapebinproduct.py) and tape-reader become frozen per-file
   tapebin sha/fixtures (c) checked by tapebinproduct.cx (its shard part is the stub "not ported") and a .cx
   reader check.  com-*-neg / com-input-contract / com-tape-struct use `python3 -m unisa compile --from-tape` as a
   CONTROL: control = UA (b) and fixtures previously produced by unisa.tapebin.encode become committed bytes (c).
F7 flownet / m4 / ujs (c, W1-H; then H9): keep flownet-baseline-file (already "no Python"); delete the 7 jobs that
   respawn `python3 -m m4` (acc, out1-4, check, selftest) -- their content is the frozen tests/flownet-baseline.tsv;
   flownet-w2-accepts compares to frozen m4 bytes; flownet-w2-tables uses the committed tables/*.net.tsv.
F8 Image-equality suites (c, W2-A): closure.sh (+ com-closure stays product-side), bigclosure.sh, opt.sh
   OPT_PART=closure, layout_check.py.  The Python back end is the image oracle.  Capture sha256 of every
   (probe, target) Python image while live (tests/closure.expected.tsv, bigclosure.expected.tsv, layout.expected)
   and compare UA images to it; the run half uses tests/cxlib/tapevm.cx (already a .cx port of tape.py parse +
   vm.py run_reference).  closure.sh's existing REPLAY mode is the natural hook.
F9 Run-route suites (b, W2-B): corpus, crossnative, fat, native, multi, diag, tools, selfgap, bootstrap (U
   branch), difftest (Python branch; delete difftest-py-1..4 and difftest-py.knownfail, pyfront.knownfail), vm.sh.
   Their default driver is `python3 -m unisa`; com-corpus/-difftest/-closure/-tools/-multi/-diag already run the
   product.  Default driver becomes UA; expected values from host cc (as difftest) or frozen .want.  ccrun is (c):
   its expected IS the Python run (ccrun.knownwrong encodes Python-specific outcomes), so freeze it.
F10 Misc Python routes (W2-C): staticunits, tagforward, strconvertroute (seed route), winsuite, fwrite, revivedscan
   (drop Python-route lists) are (b).  stages.sh/backstages.sh (A-33 "no stage decided in code") compare the C
   asked-set with the Python oracle's asked-set: freeze the Python asked-sets (c).  edgeablate.cx --produce uses
   UNISA_ABLATE through Python: the gate modes are already frozen; drop --produce (c).  ablate.sh (ungated) (d).
F11 Encoder / image / lowering referees (a, W2-D1..D3): 34 files (32 .py) import unisa.assemble,
   emit_arm/emit_x86, image.*, lower, __main__._oracle to compute reference bytes for the delta nets.  Referee
   becomes UA (`-b TARGET`, `-t`), which closure proves byte-identical to the archived Python back end, plus
   committed exec/facts tables for named constants; emit_arm.encode domain sweeps (armcheck/armint/armmem/armfp)
   are frozen encodings (c).  tests/enc/tins.py is a pure text tool -> tests/cxlib/tins.cx.
F12 Front-end / J10 tables (W2-E): exec/pp/{literal,macro,pragma}check.py freeze unisa.front.pp._pieces outputs
   (c) and reseal pp-leaf (tests/seals/exec-pp-literals lists unisa/__init__.py as builder); exec/lex/declcheck.py
   reads weights/gold/lex.tsv + iterate/kernel/typekw.tsv (a); iterate/kernel/{order,typekw,vocab}_check.py
   compare the declared TSVs with committed kernel/unisa_model.inc instead of unisa (c); permoracle.py (d);
   tests/pptruthcheck.py (c).
F13 Container / prune (W2-F): see H8; tests/seedapecheck.sh (d: seed/ape.c is the only container writer; comboot's
   fixed point and ape-version cover it); tests/apeversioncheck.py -> .cx with a frozen VERSIONINFO field table
   (a); exec/prune/check.cx reads SHAPE/REGS from unisa/tape.py at start and pins unisa/prune.py sha ->
   read docs/tapebin-v1.shape.tsv + a frozen REGS row, drop the referee-staleness pin (c); tests/reseal-prune.sh
   loses its Python referee step (d).
F14 Lists and registries (f, W4-A/W4-B): identity lists (seed/ident.c, exec/pipeline/models.py,
   tests/cxlib/seedprep.cx, tests/seedmatrixprepared.cx, tests/libraryunion{composite,scalar}check.cx,
   tests/provenancecheck.py, release/tools/model-reseal.py, .github/workflows/windows-signing.yml), product-closure
   prefixes (tests/freezecheck.cx, tests/shipclassify.cx, tests/gatedecl{,check}.py, release/tools/whyrerun.py,
   tests/affected.py), cache keys (exec/c/compilerpack.py, exec/c/memorycheck.sh, exec/lower/*check.sh,
   exec/parse2/*check.sh, exec/stamp.sh, tests/pipelinecache.py, tests/tools.sh), scratch exports
   (tests/seed*check.sh git archive lists), registries (tests/gate.sh, gatedeps.json via refreshgatedeps.cx,
   gatelayers.py, gateselect.tsv, gateselectcheck.py, stagemap.c, ungated-checks.tsv, release/derived-map.tsv,
   tests/INVENTORY.md via inventory-derived), fixtures (tests/k2bcheck.py mutates unisa/vm.py; queuecheck.py).

3. WAVES (py2cx; max 8 agents per wave; related files grouped; hard core last)
-------------------------------------------------------------------------------
Rules for every batch (each agent):
  R1 CAPTURE FIRST: any (c) baseline is captured from the live unisa in a scratch export, committed as data
     (TSV/hex/sha), and the capture command recorded in the data file header.  No capture after W3 ends.
  R2 No .py grows.  A replaced check becomes .cx (run via `sh ./unisacc.com -run` or tests/cxbin); the old .py is
     deleted in the same commit; parity is proven by running old and new side by side once (rc and verdict lines)
     plus one mutation that makes the new check red (commitgate/derivedcheck as usual).
  R3 Each batch updates its own registries in the same commit: tests/gate.sh job line, tests/gatedeps.json (via
     tests/refreshgatedeps.cx), tests/INVENTORY.md (inventory-derived is byte-exact), tests/ungated-checks.tsv,
     tests/gatelayers.py / gateselect.tsv / stagemap.c when a job name changes or disappears,
     release/derived-map.tsv rows for the files it touched.
  R4 Batches never touch the identity lists (W4-A) or include/*.h / src/*.c prose (bundled into W4-A).

WAVE 1 -- independent, no hard core (8 agents)                                          files
  W1-A seed self-tests obsolete (d): acceptance, baseline, artifacts, ape(+ape_stage), lexdiff, optpy,
       selfhost(+selfhost_prepare), volatilecomma.py; all.sh rows; opt_partition_check/ciplancheck referrers     10
  W1-B ungated referees + lex net experiment (d): exec/enc/libraryexitcheck.py (+exec/lower caller),
       librarycallback-oracle.py, libraryrootscheck.py, libunisaccsymbolcheck.py, exec/lex/{net,stage}.py           6
  W1-C K6 gold labels (c): 11 seed/unisa_gold_*.c renamed seed/gold_*_label.c, 11 harnesses -> one .cx vs
       weights/gold/<stage>.tsv; jobs unisa-gold-* renamed gold-label-*                                           22
  W1-D K6 generic (c): 18 seed C refs kept, 18 harnesses -> tests/k6frozen.cx + tests/k6/<name>.tsv captured live;
       jobs unisa-{bits,...,zdata} -> k6-<name> (tapebin-shape and libneed harnesses deleted here, generators W3)  38
  W1-E external referees (a): gold/abi/binsel/pfconv audits, consts_check, source_layout_check, tsv_contract       7
  W1-F library model checks (c): 12 .cx + 3 exec checks + tapecheckmodel.cx + librarytypedreader reseal           18
  W1-G tape codec + refusal controls (b/c/d): tapebin.py, tapebinproduct.{py,cx}, tapereadercheck.py,
       {tapebin,extent,hostaddr,bindings}negcheck.cx, inputcontractcheck.cx, tapestructcheck.cx                   10
  W1-H flownet/m4/ujs oracle jobs (c): tests/flownetbaseline.cx (drop 7 m4-respawn jobs),
       adapter/ujswasm/ujswasm.cx accepts, adapter/ujswasm/tables.cx --check                                        3

WAVE 2 -- Python routes and referees (8 agents); needs UA only
  W2-A image equality (c): closure.sh, bigclosure.sh, opt.sh closure part, layout_check.py (+layout.sh)           4
  W2-B run-route suites (b; ccrun c): corpus, crossnative, fat, native, multi, diag, tools, selfgap, bootstrap,
       difftest (+delete difftest-py-1..4), ccrun, vm.sh, difftest-py.knownfail, pyfront.knownfail               15
  W2-C misc routes + ask-sets: staticunits, tagforward, strconvertroute, winsuite, fwrite/*, revivedscan,
       stages.sh, backstages.sh, edgeablate.cx --produce, ablate.sh                                               12
  W2-D1 arm/x86 encoder referees: exec/enc/arm*check.py (7), armwincheck, x86wincheck, tests/enc/tins.py -> tins.cx  10
  W2-D2 image-writer referees: exec/enc/{imagecheck,dynelfcheck,machocheck,pecheck,realcheck,librarysymbolscheck,
       roundtrip}.py + exec/enc/check.sh (exec-e5) + tests/enc/{ref,pereal}.py + exec/pipeline/check-elf.py +
       exec/c/winmemorycheck.py + tests/elfobj.py                                                                13
  W2-D3 lowering / ABI referees: exec/lower/{check,fullcheck,largecodecheck,sparsecheck}.py, exec/c/library{call,
       stack}check.py, exec/enc/windowshostbridgecheck.py, exec/ffi/bridgecheck.py, tests/libunisacccheck.py,
       tests/modelsignature3check.py (+modelsourceimportcompatcheck.py)                                          11
  W2-E front end / J10: exec/pp/{literal,macro,pragma}check.py (+pp-leaf reseal, ppleafcapture.cx branch),
       exec/lex/declcheck.py, iterate/kernel/{order,typekw,vocab}_check.py, permoracle.py, tests/pptruthcheck.py   12
  W2-F container + prune (H8): apecache.py, embeddedcheck.py, buildcompiler.sh SEED_GEN=0, comboot.py (+
       release/tools/candidate_chain.sh -> comboot.sh), seedapecheck.sh, apeversioncheck.py, exec/prune/check.cx,
       reseal-prune.sh; Makefile classic-com -> seed/ape.c                                                          8+

WAVE 3 -- hard core (6 agents); W3-A after W3-B (A pins B's frozen inputs)
  W3-B H2+H3 gold/weights freeze: weights/gold + weights/built.* + dumps frozen; tsvbuild_check.py -> construct.c
       -u == weights/built.uns2 (C); iterate/construct tools/*.py -> frozen expected + C cmp; Makefile quick/acc/
       weights; ci.yml + release-check.yml (UA/unisacc.com -run of examples); release/tools/precheck.sh             51
  W3-A H1 emit-kernel: new .cx generator for kernel/unisa_headers.inc (include/*.h embedding); tests/kernel.sh ->
       .cx (regenerate headers.inc + provenance pins for the 40 frozen files and weights/gold); iterate/kernel/
       check.sh compares genmodel with committed kernel/ (drop Python-mutation negatives); headerchain.{sh,cx}
       steps; AGENTS.md/CLAUDE.md kernel section reworded in the same commit                                       43
  W3-C H4 facts (R3 end): after the R3 cut lands, delete unisa producers from export.py (shrink only), add
       exec/facts/FROZEN.tsv + .cx sha checker; facts-export keeps --check for the remaining producers             45
  W3-D H5+H7: seed/unisa_tapebin_shape.c -> seed/tapebin_shape.c generating src/tapebin_shape.inc and
       docs/tapebin-v1.{shape,records}.tsv from the shape TSV (authority moves to docs/tapebin-v1.shape.tsv;
       docs/tapebin-v1.md sentence); exec/tapecheck/check.sh; tests/docs.sh -> .cx                                  4
  W3-E H6: seed/unisa_libneed.c -> seed/libneed.c guard/table checker; tests/libneed.sh; headerchain step           4
  W3-F H9: m4/ + ujs/seed/ prepared for archive (no gate job imports them after W1-H; ujs README/ARCHITECTURE
       note); coordinate with the uncommitted m4/ work in the live checkout                                         6

WAVE 4 -- cut-over and move (sequential: A and B in parallel, then C)
  W4-A H10 identity cut-over (one agent, one commit): drop `unisa` from seed/ident.c, exec/pipeline/models.py,
       tests/cxlib/seedprep.cx, tests/seedmatrixprepared.cx, tests/libraryunion{composite,scalar}check.cx,
       tests/provenancecheck.py, release/tools/model-reseal.py, windows-signing.yml, tests/freezecheck.cx,
       tests/shipclassify.cx, release/tools/whyrerun.py; bundle the prose rewording of src/*.c comments and of the
       declared tables exec/pp/escape-template.tsv, exec/tapecheck/shape.tsv, iterate/kernel/*.tsv; then
       `make com` (bounded steps) + `make comboot` to re-record unisacc.com(.build.json) and reseal what hashes
       the identity.  unisa/ still present but no longer hashed.                                                   22
  W4-B registries and keys (one or two agents): compilerpack.py, memorycheck.sh, exec/lower|parse2 *check.sh,
       stamp.sh, pipelinecache.py, tools.sh, chainfirstredcheck.sh, commitgatecheck.sh, release.sh DECLARED,
       tests/seed*check.sh git-archive lists (verify each export no longer needs unisa), k2bcheck.py fixture
       retarget, queuecheck.py, affected.py, gatedecl{,check}.py, gatequeue.py, refreshgatedeps.cx ->
       regenerate gatedeps.json, gate.sh/gatelayers.py/gateselect*/stagemap.c leftovers, ungated-checks.tsv,
       derived-map.tsv, all.sh `acc` row; add tests/seedfree.cx (done criterion) as a gate job                      36
  W4-C final (one agent): run tests/seedfree.cx in a scratch export with unisa/ removed (proof), then
       `git mv unisa archive/unisa` (and `git mv m4 archive/m4`, `git mv ujs/seed archive/ujs-seed` per H9),
       reword the docs of section 4, update README layout table and ARCHITECTURE.md, AGENTS.md/CLAUDE.md;
       full gate (`tests/release.sh --com` queue) green; tests/seedfree.cx green.                                  11+docs

Ordering constraints: W1-* and W2-* captures must finish while unisa is importable (all of W1/W2/W3 precede W4).
W3-A depends on W3-B (pins). W3-C waits for the R3 cut. W4-A needs every batch that edits exec/, src/, seed/,
include/, kernel/, weights/ landed (identity is rebuilt once). W4-C is the only commit that moves unisa/.

4. DOCS: sentences that argue "keep unisa as oracle / reference / fallback" -> reword to
   "archived at archive/unisa (manual reference only), not a live dependency"
---------------------------------------------------------------------------------------------
Search: git grep -n -i oracle (+context) over prd.md spec.md research/ docs/ plans/ AGENTS.md CLAUDE.md
ARCHITECTURE.md README.md, plus "keep Python / 参考实现 / 回退 / 保留" phrasings.  Most of the 135 "oracle" hits are
the Oracle seam of the spec (P-1, §4.1 `ask`) or "system cc oracle" -- those stay.  The ones about the seed:

A. Normative / live docs -- MUST be reworded in W4-C (or in the wave named):
  1. research/c43-roadmap-0043-and-refactor.md:239  "**保留**：Python seed（`unisa/`）继续当 oracle。tdesc 审计实测它在所有探针上
     都正确。重构不新增 Python，但 R3 改 k2 常量时仍会运行现有的 `export.py`。…" -> replace with the owner ruling:
     bootstrap complete (N22), unisa archived, export.py only for the R3 cut; and :232 (export.py row, item 4) -> W3-C status.
     (Live checkout has an uncommitted edit to this file -- merge, do not overwrite.)
  2. ARCHITECTURE.md:8   table row "种子与参考实现 | `unisa/`、`src/`、`kernel/`、`unisacc.c` | Python 种子、经典 C 参考/回退、
     生成静态事实与容器打包；与模型路线对照" -> split: unisa/ archived; src/ kernel/ unisacc.c = classic C reference (UA).
  3. ARCHITECTURE.md:92-94  "## 2 种子（第 0 代）：`unisa/`，全部是 Python … 之后作为参考实现与回退手段：C 编译器的输出要与它逐字节
     一致。它的每个表状决策都经 `oracle.ask`。" -> past tense: generation 0, archived after N22; byte-equality is now
     frozen (closure.expected etc.) and the referee is UA.
  4. ARCHITECTURE.md:96-118  module table (oracle.py "唯一的询问入口", linalg.py "构造器与 oracle 共用", vm.py "tape 参考解释器",
     ape.py, control/ "保留作负结果的证据", __main__/driver/docgen) -> one line pointing at archive/unisa; the live tape VM
     is tests/cxlib/tapevm.cx, the container writer seed/ape.c, the doc table generator the W3-D .cx.
  5. ARCHITECTURE.md:23, :134  classic-com described via the Python container path -> seed/ape.c.
  6. docs/glossary.md:62  "长期充当 oracle 的 `.py`（`unisa/gold.py`）| 不重命名 … 等金表全走 weights/gold/*.tsv 后自然退役" ->
     "retired: weights/gold/*.tsv is the authority; gold.py archived".  docs/glossary.md:90 "长期 oracle（`unisa/gold.py`）只加
     顶部术语注释" -> delete the clause.  Lines 24-42 cite unisa/gold.py, control/net.py, control/segment.py as term
     sources -> prefix archive/.  (Uncommitted edit in the live checkout -- merge.)
  7. iterate/kernel/README.md:74  check row "region | the Python **oracle**: `python3 -m unisa emit-kernel …`", :81 perm row
     (permoracle.py), :26 "**Transition:** `gold.ALL` is still what the Python side uses …", :143 "vocab.tsv equals
     ckernel.py's tuples (transition)" -> committed kernel/ is the comparison; order/typekw/vocab TSVs are the
     authorities (W2-E / W3-A).  iterate/kernel/check.sh:146 comment "Python as ORACLE only" goes with W3-A.
  8. iterate/construct/README.md (netdump.py / uns2slice.py / uns2round.py usage block; "prints the net that
     unisa/construct.py build_net builds") -> frozen expected files + C-only commands (W3-B).
  9. docs/tapebin-v1.md:17  "`unisa/tape.py:SHAPE` 是唯一声明，导出 `docs/tapebin-v1.shape.tsv` …" -> "docs/tapebin-v1.shape.tsv
     is the only declaration; seed/tapebin_shape.c generates src/tapebin_shape.inc" (W3-D).
 10. docs/toolchain.md:20  "后端有三份实现必须逐字节一致：Python（`unisa/image/`，构造源头）、C 参考（`src/back_image.c`）、产品 δ …；
     `tests/closure.sh` 对映像逐字节对拍" -> two live implementations (C reference, product delta) + frozen Python images.
 11. spec.md:264 K-4 "Python 参考实现 `unisa/linalg.py`，C 部署实现 `kernel/unisa_core.c`，两者在 FULL gold 上必须逐 key 同类" ->
     the C kernel against the frozen gold; spec.md:236 "生成的表由 `unisa/gold.py` 与发布的构造权重生成（`python3 -m unisa acc` …）";
     spec.md:130-147 the `unisa …` CLI block, :214 U-1, :96 P-4 (`unisa quant`), :482 Q-7, :499 Q-10, :516 O-2, :546 W-14,
     :686 A-3, :705/:716 (`unisa acc`, difftest "unisa vs cc"), :741-:747 A-5/A-9/A-11 -> mark as the archived seed's
     CLI/acceptance (historical), and name the live equivalents (unisacc -run/-b, --check-oracle A-49, difftest UA).
 12. prd.md:101  "模型与 Python 参考同步" (tape integer range) -> "模型与 C 参考同步"; other prd.md seed mentions (9 lines).
 13. README.md:222 "*Generated by `python3 -m unisa docs` from `unisa/gold.py` …*" (W3-D generator), :239-250 "## Develop it
     (the Python seed) … is a second, independent front and back end that the C compiler is checked against byte for
     byte" + the build-weights/acc/run/emit-kernel command block, :266, :274 ("difftest | unisa vs the system
     compiler"), :312 layout row "unisa/ the Python seed …" -> archived seed paragraph + C-only commands.
 14. AGENTS.md:94-108 and CLAUDE.md:94-108 (identical)  "`kernel/` 里的一切都由 `python3 -m unisa emit-kernel` 写出" + table rows
     (unisa_model.inc from built.json "由 `unisa build-weights` 从 `unisa/gold.py` 构造", unisa_cases.inc from gold.py,
     unisa_core.c from ckernel.py KERNEL_BODY, weights/gold "由 `unisa gold-export` 写出") + :108 "`unisa build-weights` …
     `unisa train` 只是对照组" -> frozen artifacts, headers.inc generator, provenance pins (W3-A, same commit).
 15. research/c43-derived-consistency.md:12  map row "weights/gold/*.tsv | `unisa gold-export`（`unisa/gold.py` 构造）| …
     `unisa-gold-*`（K6-18…28）只对拍 C 参考与 `gold.py` 源" -> frozen authority + gold-label second derivation (W1-C).
 16. research/c43-typed-reader-receipt.md:28  "The fixed-two-int Python oracle is observation only" -> frozen reference rows.
 17. research/c43-stream-interface-S-step0.md:82 "参考Python若使用只作可关闭…" and S-step1.md:53 "参考工具仅作独立oracle" ->
     reference = frozen data / UA.
 18. docs/tape-vs-wasm.md, docs/toolchain.md (3 lines), exec/README.md:12 ("`unisa build-weights`, packaging"),
     exec/rules.md (3), exec/lex/rules.md:6, exec/tapecheck/README.md (3), exec/k2-design.md, exec/c/PACKAGE.md,
     exec/lower/hostsymbols.md, exec/librarycontract/README.md, exec/enc/CONTROL_AUDIT.md, examples/win/README.md,
     release/RELEASE-PIPELINE.md (4), tests/STAGE-IDENTITIES.md (3), tests/SEAL-MODULES.md, tests/seals/exec-net-*/
     README.md (17 x 2: "regenerate with python3 -m unisa emit-kernel"), tests/seals/exec-prune/README.md,
     tests/GATEDEPS-LAYERS-PROPOSAL.md (unisa-* job list), tests/INVENTORY.md (generated: regenerate), m4/README.md:44,
     ujs/ARCHITECTURE.md, ujs/README.md, plans/flownet-consumer-configs.md, plans/v0.0.43.md:45 ("Python 参考
     `unisa/tape.py` … 属于参考漂移") -> path prefix archive/ or "frozen"; plans/ entries are history: one-line note.
 19. Code comments that say "keep Python" / "Python referee": tests/gate.sh comments on the 31 K6 jobs and on
     tapebin-roundtrip-*/tapebin-shape (lines 909-911, 976-1006), tests/ungated-checks.tsv rows 80-101 ("reference
     oracle only"), tests/seals/exec-prune/{cell,measurement}.tsv referee_note -- they disappear or are rewritten
     with their jobs (W1-C/D/G, W2-F).

B. Historical records -- leave text, no edit (docs history; add nothing): research/c41-*.md rulings (K6 inventory,
   freeze rulings listing "全 unisa 无专用 seed C 模块"), research/notes/*, research/unisacc-paper*.md, research/arxiv-
   paper-a/*, research/lean/*, release/evidence/*, ujs/archive/*.  The papers describe the system as of their
   date; if they are revised later, cite archive/unisa.

5. RISKS AND CHECKS
-------------------
 - Identity: dropping `unisa` from models.closure()/seed/ident.c changes the source identity once; every
   artifact that records it (unisacc.com.build.json, seeds/receipts that embed the identity, tests/seals/*
   identity rows) must be re-recorded in W4-A, with comboot proving the fixed point again.  Do it once, after all
   exec/src/seed/include edits.
 - Frozen-header bytes: do not "clean up" provenance lines inside kernel/, weights/, exec/facts/*.tsv, tests/seals/,
   adapter/ujswasm/tables/; their sha prefixes are referenced elsewhere.  include/*.h prose edits cascade into
   kernel/unisa_headers.inc and the exec-pp-literals seal -- only with a real header change.
 - Capture completeness: a (c) baseline captured on one host must not hide host-dependent outputs (image bytes
   per target are host-independent; run outputs are not) -- capture image sha for all six targets, run outputs on
   the gate host only, as the current suites do.
 - Coverage loss to accept knowingly: the Python front end as an independent second implementation (selfhost,
   lexdiff, stages' Python asked-set, difftest-py) is gone; what remains independent is host cc (difftest,
   referees), the classic C reference UA vs the model product, and frozen Python outputs.
 - Scope creep: m4/ and ujs/seed/ are Python seeds of the UJS line that import unisa.intnet.build; archiving them
   with unisa (H9) is the recorded decision (vendoring intnet.build into ujs would extend Python, which the
   commissar rule forbids).  The live checkout has uncommitted m4/ edits; W3-F must coordinate.
 - Verification of the end state: in a scratch export, `rm -rf unisa m4 ujs/seed` and run the full gate list
   (tests/gate.sh --list) plus `tests/release.sh --com`; any import error or missing file is a missed dependency.
```
