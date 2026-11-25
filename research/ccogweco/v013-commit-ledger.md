# v0.1.3 提交台账(main..v0.1.3)

生成:HEAD=8b3727031;git log c500c0db8..HEAD(--reverse);verify 列对每个提交按其触及的批次入口给出 `sh ./unisacc.com tests/run-batch.cx ROOT OUT <id>` 模板,多批次用分号分隔;`-` 表示该提交不触批次入口(文档/研究/补丁)。

| commit | subject | verify |
|---|---|---|
| 7eb0d06a47b95fdd3f8dba78c3e0dcebc6de7401 | v0.1.3 prep: land pending research and skills from main checkout | - |
| 387184426b03960fc5eac4c681dca7f4313405c2 | v0.1.3: repoint 290 moved doc links, record 346 unresolved in broken-links-residue.tsv | - |
| 36ed4bc63046c587026ad15ea82e55a1196de24d | v0.1.3: scrub 64 home-path lines in research and plans (before files=4 lines=64, after 0) | - |
| b540dd7660589829252ebed6360c41451e1d77bc | v0.1.3: classify 181 check-less tools: 28 in batches, 1 archive candidate (stream-pack.cx), 0 byte-identical duplicates | - |
| ffdb8fd3cf18e569ee97152bf790bebf640bf9e1 | v0.1.3: merge origin/main (v0.1.2 post-publish: thaw 0.1.2-FREEZE, version-seq public) | - |
| becfe3430b33bc31e2f0adc6a3ceed1749dd693d | v0.1.3: stream-pack.cx is not archivable (executed by tools/check-streampack.cx:82,123 and listed in tests/coverage.tsv:358); classify 96 tools as kept-on-evidence and record the evidence column | - |
| af0276d0550ad57e5001c10175c89a56ba31a882 | v0.1.3: restore cx-layers history text and point the migration sources at fixed commits (3aae1b769 kernel/process/lnx/winprocess, fab2451e9 deflate) | - |
| fb01a0714e7d7e27cfe7203de549fb5d923c840d | v0.1.3: restore post-publish-verify-106 historical worktree paths and cite 2e8a19a342:release/candidate.tsv as the fixed reference | - |
| 80d0778b9156eb709a778cc7921bf84a0b93d9ee | v0.1.3: keep historical text in research and release-note docs, add fixed-commit refs for 127 moved paths instead of current-path links | - |
| fc0b3555b97112956e441a47417baed171a0d9bd | v0.1.3 T3.6-1: add --nn-dir and --no-external-nn in cx/driver/cli.cx (keep CxcState==144) | - |
| 11db56715396b0918b5ffd2a9653203455f6bf97 | v0.1.3: audit stale index lines (table-index purpose names /tmp/sa-graph/hardwire.md, gone; prd 能力速查 15 tokens clean) | - |
| 0abae1ff66c21393c439aca66811af8407e4e478 | v0.1.3: audit skills refs (3 missing: examples/hello.cx, examples/kv.cx, kernels/contracts.cx; 101 tokens checked) | - |
| 3aa8ee25f2c06a56277acb9cf7d1b4b0db1dfe06 | v0.1.3: list research TSV/patch with zero ledger refs (44); git first-commit is 2026-11-24/25 for all, so no 60-day-age row exists | - |
| f83be407f6e0c19beef146a136477c69877d6446 | v0.1.3: refresh broken-links-residue to the post-rework tree (630 rows: 420 history, 155 absent, 44 notpath, 7 ambiguous, 2 notoken, 2 version-skew) | - |
| c15e22078a74c0de3fbce6c75b702d74ce1549c3 | v0.1.3 T3.6-2: external-first nn loader (cx/driver/nnext.cx, src wiring, check-nn-external) | sh ./unisacc.com tests/run-batch.cx ROOT OUT check-nn-external |
| 8c984145b0db4a12870dc34eb78171914dd926b8 | v0.1.3: list bare fork/waitpid call sites under tools/cxlib, ext, cx for the S5 ratchet (40 calls; cx/sys/proc.cx excluded; 2 in cx/sys helpers) | - |
| 86090bebb607a3482f71677e62d90c88ad4711de | v0.1.3: S2 patch for cx/sys/time.cx EINTR deadline loops plus tools/check-time-eintr.cx (private tree; red=compile fail, green=5 PASS, mutation=hang timeout) | - |
| 31544ffad04bf2830dbd2d84bb63f7c12b124b46 | v0.1.3 T3.6-3: fix external nn cache bypass, L0 self-path/regular guards, formal chain builtin & E2E | sh ./unisacc.com tests/run-batch.cx ROOT OUT check-nn-external |
| c807f40cf0cbc7a9394b646295d04e4b4b8b13e0 | v0.1.3: registration audit of branch-new tools vs v0.1.2 (1 new check-nn-external.cx, all four tables present; batches 2045/2048) | - |
| d3b97da5ae767325e558025500a2a343fa5531ce | v0.1.3: S5 bare fork/waitpid ratchet patch (tools/check-bare-proc.cx + fixture + baseline + ratchets wiring; base 19/0, patched 20/0, mutation 19/1) | - |
| f0b85694735f66d5e7bb739017556002f2df2ba2 | v0.1.3: audit 20 branch commit messages against their diffs (numbers recomputed; 9 rows carry notes: unit/scope/evidence gaps) | - |
| bc3070168fa4900f15f0cf8e7d82369897605e38 | v0.1.3 C3-S1a: L0 process ownership table with (slot,gen) binding (gen verified; full table coverage UNMEASURED, table-driven fixture by agy-h) | - |
| e90e9337ef32e015efbf94bb1a0d0cf7a504469b | v0.1.3 C3-S1b-1: tools/check-proc-own-gen.cx + proc-own-transitions.tsv (10 assertions, cold 10.3s) | sh ./unisacc.com tests/run-batch.cx ROOT OUT check-proc-own-gen |
| 1d0c1a2fad877e1817afe7dcc35c7e3cfc187415 | v0.1.3: S5 ratchet patch v2 (lexical scan, monotonic baseline, red failure paths, fixture batch; private tree only) | - |
| 958e1ba0560e9e0c666a4d9842c54b0662c8a572 | C3-S1c: fix row 67 LOST_LIVE close->DETACHED and add test-only observability/injection hooks | - |
| 5e8c0e5278c4917f0d2b1816c17f7a7904c9f161 | C3-S1d: split SYP_SLOT_LOST_LIVE(4)/SYP_SLOT_LOST_ABANDONED(5) and gate test event log | - |
| d45a45894a7ea90656795f03140ab69dbbac156b | C3-S1e: gate env fault injection behind syp_test_armed and require exact PID filter (B4) | - |
| ec82c2e90464828fb49661e5ce42fc5c7d959c8b | v0.1.3: land S5 ratchet patch v3, its self-check table and the known-gaps collection in research/ccogweco | - |
| e7fc6397609437411b5c019ed9e37dabad1fb908 | v0.1.3: release-note known-gaps draft for 0.1.3 (defects and UNCOVERED/UNOBSERVED only; pending-evidence rows flagged) | - |
| 77774d7b23a54734c4e21247bce96717c93fb140 | v0.1.3: re-audit of commits after f0b856947 (S1a-S1e): 2 notes, 0 false recomputable numbers | - |
| aed1f0361a0562a35d68f3b7b24386321cb929b1 | v0.1.3: merge origin/main (batch capacity 4096 landed) | - |
| 4bc212f701fafe1f2137e0ee96cf5e559c93d84e | C3-S1b-2: add core process ownership check (21 assertions, table fixture by cdx) | sh ./unisacc.com tests/run-batch.cx ROOT OUT check-proc-own |
| ec78ceeb70ca589d68edb418028bb495e4c1e6f6 | v0.1.3: S2 patch v2 (first poll on ms=0, EINTR remaining-time re-entry, exact-PID seam) with registration and fixture evidence | - |
| 43b11e5ce17aa19acf2ee7552318422f61061515 | C3-S1b-3: add 64-slot saturation & end-timeout check (8 assertions, 30+table reconciled) | sh ./unisacc.com tests/run-batch.cx ROOT OUT check-proc-own-sat |
| d51481c6b75eb37d0902b0a6437187068bcce8b2 | C3 S1: register the process ownership transition table check | sh ./unisacc.com tests/run-batch.cx ROOT OUT check-proc-own-table |
| e4da05da84a6eb48efa76e3e428f99ead315db0b | v0.1.3: fill the 27-cell gap source from d51481c6b (tools/proc-own-unmeasured.tsv, 63 PASS / 27 UNMEASURED) | - |
| 8b3727031b70481eed09f8bc60f5217c96d8ad69 | v0.1.3: merge origin/main (tape carrier tooling and mutex safe subset landed) | - |
