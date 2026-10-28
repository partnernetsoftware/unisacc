# Candidate 94: frozen-source E1/E2 refresh

Fourteen registered SOURCE batches were executed afresh on Linux x86_64:
13 PASS, one FAIL, zero UNKNOWN. The failed header-location batch remains a
sticky failure; neither its assertions nor its time limits were changed.

## Identity and material

- Product: `94b549d1fbbbb29f9068492c17866aa8ea072f950fe4c7519d86cd989d0bb7f1`, 3,001,840 bytes.
- Build record: `fe5d0aec94bdf46fd3816806a0afc412105494684415816af30f7b810fa52f75`.
- Frozen test root: `/home/devali/repos/unisacc-epoch-c590410c68`.
- Measured test-source SID: `d4ada505a117028b5e35685d7407077278b28c29a410bf8a47d709021a674f73`.
- Registry: `b7cd6eea7cb7f11e242e8fbaf130a7ff85fa1b95941a66db280ecd30d7303eed`.
- Harness: `25f12250f2e4e6bc96e84896b9036229a68cac2c7db529c211cadddf1bc76021`.

The shared checkout's live SID `88fe6313…` is different. These are explicitly
frozen-source observations, not a claim that the shared live tree has the
installed product's source identity. No Git metadata or test inputs were changed
after session initialization.

The frozen installed product rebuilt `tools/stream-runtime.c` under the original
55-second bound, then `tools/stream-pack.cx` extracted its actual installed P3
into `out/c43-94-streams-01`. Package SHA is
`66c797a038876106ca3948c79e84772536ba7b552df8912cbae25626573deb66`.
The runtime base `3f38810c…`, resource section `5c087fe6…`, E2 container
`4f3c87fd…` and E1 container `68f08deb…` equal the previous bytes. Therefore the
existing E1/E2 test pins were left unchanged. The new E3 container is
`b4dc37c0bd99b2c9bd8f44ac2b69c266c471cba315d56dbf3327fbf917d2434a`;
old E3 passes are not transferred. The fresh input-binding batch checked the
selected members against the actual candidate.

## Execution and receipts

Session: `out/c43-94-stream-validation-01` below the frozen root.
The existing `tools/all-waves.cx` initialized it, hashed the `streams` material,
and built its registered native coordinator/runner. Every wave ran exactly one
registered batch; the unchanged `tests/run-batch.cx` invoked the frozen
`unisacc.com` on that batch's SOURCE entry. Native coordinator artifacts are
startup material, not cached test results.

Each command was bounded at 55 seconds. Each heavy wave allocated 42 seconds to
its child and retained the existing batch and inner-child limits. All receipts
record unchanged candidate, source/registry/harness identity, inputs and
materials; all child stderr files are empty.

| Batch | Result/count | Wave |
|---|---:|---:|
| e2-input-binding | PASS 24/0 | 3 |
| stream-e1-lex-tokens | PASS 188/0 | 5 |
| stream-e1-lex-bytes | PASS 501/0 | 7 |
| stream-e1-lex-refusals | PASS 624/0 | 9 |
| stream-e1-independent | PASS 25/0 | 11 |
| stream-e2-domain-positive | PASS 103/0 | 13 |
| stream-e2-domain-refusal | PASS 187/0 | 15 |
| stream-e2-independent | PASS 36/0 | 17 |
| stream-e2-pp-gold-linux | PASS 21/0 | 19 |
| pp-cli-targets | PASS 20/0 | 21 |
| pp-cli-includes | PASS 20/0 | 23 |
| pp-cli-macros | PASS 23/0 | 25 |
| pp-header-locations | FAIL 11 cases / 8 failures | 27 |
| negative-diagnostics | PASS 211/0, known_gaps=1 | 29 |

Each leaf is `wave-N/BATCH.{receipt,stdout,stderr,status}`. For example,
`wave-13/stream-e2-domain-positive.receipt` binds the new candidate and frozen SID.
Top-level commands/transcripts are `out/c43-94-wave-BATCH-01.{stdout,stderr}`.

Independent `tests/all-aggregate.cx` re-read and verified the session and returned
rc1, `NOT_QUALIFIED`: 149 implemented batches, pass13/fail1/unknown0/missing135;
177 coverage rows, satisfied13/unsatisfied145/unresolved19; refused0, waves14.
Its transcript is `out/c43-94-stream-aggregate-01.stdout`. This partial session
does not establish the complete suite or any foreign-host qualification.
Detailed startup/construction/loading/inference/output/validation timings remain
UNMEASURED.

## Preserved header failure

The frozen root makes the `plain.h` fixture's absolute name 65 bytes. Both the
old `da4e7ef1…` product and new `94b549d1…` product produce the same 62-byte
semantic `hfile` string, ending in `/plai` instead of `/plain.h`. `hline` remains
2. The new product's `-E` output already contains the truncated string, locating
the issue before E3. The `LF` case, whose name is within the bound, and both
continuation-capacity cases still pass.

Evidence below the frozen root:

- `out/c43-{94,da4}-header-path-repro.{stdout,stderr}`.
- `out/c43-{94,da4}-header-path-length.{stdout,stderr}` records actual62/expected65.
- `out/c43-94-header-path-preprocess.stdout` records the semantic truncation.
- Original fixtures remain in `out/hl-crrwxn/`; wave27 remains FAIL.

The existing UNIPP1 include-record protocol intentionally truncates diagnostic
names to 62 bytes (`exec/pp/include-location-{byte,result}.tsv`). That limit
does not justify silently truncating the `__FILE__` semantic value required by
the original product-route test. No shorter-path rerun was substituted for this
failure. A separate full-name state for semantic rescanning is the next fix;
the old diagnostic record format must remain unchanged.

## Conservative matrix update

The two matrices were clean before editing. An exact-hash CX editor validated
each mapped fresh receipt's PASS, candidate before/after and frozen SID before
replacing evidence: 38 E1 cells and 58 E2 cells now point to actual new receipts.
Status, requirement, citation and oracle columns were retained. The three
header-location/capacity E2 cells and local E2 host aggregate keep their old
historical evidence, explicitly marked not granted to candidate94. No
MISSING/foreign-host obligation was closed, and the old Rootda4 E1 NN boundary
293/0 supplement was not transferred.
