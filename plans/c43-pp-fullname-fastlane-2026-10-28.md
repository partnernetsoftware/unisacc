# E2 full semantic header names: private evidence and formal installation

The historical candidate94 truncates header `__FILE__` values to the 62-byte
UNIPP1 diagnostic-name limit. Its frozen-source header batch remains FAIL; the
matrix at that point was `NEEDS_PRODUCT_CHANGE`. This source fix was verified
initially with an external **PRIVATE / NON_ADMITTED** package, without installation,
SID/context/PC/GEN rebuilding or transfer of private results to candidate94.
The later full-source build and installed fix are recorded below. The old
private package retains its original status; it was not relabelled as admitted.

## Change and reproducibility

`IRNAME` still stores the first 62 name bytes for the existing locations wire
format. New `IRFULLNAME=4398046511104` (4×2^40) stores a separate full name blob.
`INC.record` saves `NM..NME` there. Semantic `LF.r3` and the line-directive
inherited-name/error path `LD.st2` read it. `location-byte.tsv` and the public
UNIPP1 layout are unchanged.

An E2-wide address search found no previous 4T allocation. Global searches did
find 4T constants in independent parse2/encoder models; their W stores are
separate. Existing E2 regions remain IRNAME 1T, XUB 2T, IRPATH 3T, PMHEAD 16T and
PMSTACK 32T; record indices do not approach the disjoint region stride.

Facts are `exec/facts/pp-layout.tsv` and `exec/facts/pp-fullname.tsv`.
`tools/pp-fullname-gen.cx` applies seven exact row/fragment rules to four table/
manifest files, refusing unmatched or ambiguous rules before writing output.
Its byte-exact `--check` passed. Pristine input is preserved at
`out/c43-pp-fullname-pristine-01`; the same four old files are available at
commit `ca988fbcbd`. To reproduce, supply those old files as PRISTINE and the
current facts as FACT_ROOT:

```
tools/bound 30 sh ./unisacc.com tools/pp-fullname-gen.cx -- gen FACT_ROOT PRISTINE TARGET
tools/bound 30 sh ./unisacc.com tools/pp-fullname-gen.cx -- --check FACT_ROOT PRISTINE TARGET
```

The frozen epoch's native gen/tbl/net tools actually reconstructed these three
PP recipes, all nine phases rc0, under per-command45-second watchdogs:

| Model | Exact generator flags |
|---|---|
| shared-e2 | `--shared-predefines` |
| warnpp-shared | `--locations --shared-predefines` |
| tokenpp | `--shared-predefines --no-autoinc` |

Models are in `out/c43-pp-fullname-models-01`; each phase's stdout/stderr is
`out/c43-pp-fullname-MODEL-{gen,tbl,net}.{stdout,stderr}`.
All three deltas pass `tools/table-lint.cx` with zero errors. The shared-e2
baseline and changed graph have the same 4241 states/9217 edges and 141 warnings;
registers increase 326→328 for the full blob/address temporaries. Warnings are
existing conservative dataflow/SB warnings, not waived errors.

## Actual private package

`tools/pp-fullname-pack.cx` verifies that the base PC package equals the actual
installed product's terminal package. It redirects only the three PP paths in
the frozen PC's declaration, invokes its actual native `tool-pack`, then checks
that all route/index bytes, all 29 other compressed M records and the complete F
resource suffix remain byte-identical. Exactly 3 M records changed. It never
installs, writes admission or changes the installed product.

- Base product: `94b549d1fbbbb29f9068492c17866aa8ea072f950fe4c7519d86cd989d0bb7f1`.
- Base source SID: `d4ada505a117028b5e35685d7407077278b28c29a410bf8a47d709021a674f73`.
- Base PC: `/home/devali/repos/unisacc-epoch-c590410c68/out/epoch-pc-01`.
- Base package: `66c797a038876106ca3948c79e84772536ba7b552df8912cbae25626573deb66`.
- Private package: `out/c43-pp-fullname-private-01/compiler.pkg`, 1,630,410 bytes.
- Private SHA: `7241237d0ac0e735b60a80f1670fa4008a407f51f21005574eaa41655efc2b16`.

The packer's identity/command output and byte-comparison result are preserved
under `out/c43-pp-fullname-private-01/` and
`out/c43-pp-fullname-private-01.stdout`.
`out/c43-pp-fullname-identities.tsv` records the selected source and all nine
JSON/table/network hashes. No new complete source SID was measured. In
particular, the new layout hash is
`1daa91ff2392baf76333d5b78d54d9b9ae10699a4798fc54bcadab10679c3d2d`
and the generation-rule facts hash is
`ac0f9a4cd73a4bb08b6b2ba5db22f489827249d60750027dbede3c03c381686a`.

## Validation and retained failures

The frozen Root94 with `--models PRIVATE_PACKAGE` passes all 11 original
`tests/pp-header-locations.cx` cases, including long-name forced/nested LF/CRLF
and continuation-capacity 4096/4097. The original assertions and child 5/transport 6
limits were unchanged. Transcript:
`/home/devali/repos/unisacc-epoch-c590410c68/out/c43-pp-fullname-header-locations-01.stdout`.
The original installed-product wave27 FAIL remains intact.

New `tests/preprocess/pp-fullname.cx` passes 14 assertions in
`out/c43-pp-fullname-probe-02.stdout`: installed long-name failure retained;
private full-name and main-file `#line` semantics; eight short-name comparisons
of stdout/stderr/rc; the existing named refusal of header-local `#line`; and
candidate/private-package hash stability. The first probe attempt incorrectly
assumed header-local `#line` acceptance; its four failures remain under
`out/c43-pp-fullname-probe-01.*`. The corrected probe preserves explicit
rejection assertions and adds the valid main-file positive case. Header-local
`#line` support is not claimed.

The current simulator cannot supply the shared target-predefinitions resource:
the direct shared-delta attempt remains a recorded
`missing target predefinition selector` rejection in
`out/c43-pp-fullname-sim-01.stderr`. Auxiliary simulation therefore uses the
same changed PP body with inline Linux x86_64 predefinitions. It actually reads
the long header through its file RI and emits both full semantic names.
`out/c43-pp-fullname-sim-02.cov.tsv` records `INC.record`, `LF.r3` and main-file
`LD.st2`; the expected header-`#line` rejection trace
`out/c43-pp-fullname-sim-refusal.cov.tsv` reaches `LD.st2` observation 1 and prints
the complete header name. These traces cover the modified body paths, not the
unserved shared-selector boundary. Actual shared-route NN semantics come from
the private package's11+14 assertions above.

No matrix qualification changed because of that private green. The following
installation and new executions supply separate evidence.

<a id="formal-installation"></a>
## Full-source construction, admission and installation

On 2026-10-28, main commit `778fc68ebc51c96ac69dbf0e0d4c0f35c0c61cc8`
was independently cloned at
`/home/devali/repos/unisacc-epoch-fullname-778fc68ebc` (called EPOCH below).
Only the ignored bootstrap/product and `out/` artifacts were added there.
Source SID is `f9fff160a091ac0004db69fe8b2713a4c448c22c963b8db89e3a4e2624540ec2`.
The actual source-ID helper measured the same SID in the shared checkout after
installation (`out/c43-fullname-live-source-01.stdout`).

| Material | Actual identity/result |
|---|---|
| Saved bootstrap | Root94 `94b549d1fbbbb29f9068492c17866aa8ea072f950fe4c7519d86cd989d0bb7f1` |
| Saved bootstrap record | `fe5d0aec94bdf46fd3816806a0afc412105494684415816af30f7b810fa52f75` |
| Source context | `EPOCH/out/epoch-context-01`; 107/107 actual start/finish/command rc0; final context verified |
| Kernel generation | `EPOCH/out/epoch-gen-a-01`; nine steps, both kernels equal the preceding generation |
| Full package | `EPOCH/out/epoch-pc-01/compiler.pkg`, 1,632,605 B, `e154c747dbdfacc4c638d020f55fcd8998e0e6124d2b968c86cab4bf28a1d4e6` |
| APE | 3,007,277 B, `d65c14744390932885660cf62c0ce77db7eeb4cefa46094909f1e05b0ba21b9f` |
| Installed record | `49465750c2bef8b3a5421bc82f3480034a2e776389db25b6ddf65aba67349cb8` |
| Native construction receipt | `889f4bc983cb1cd14d63d0b792471594ffd61d3b312ae09be8ee096e739e2773` |
| Installation output | `EPOCH/out/epoch-build-01/install.stdout`, `bdb1b638af4c9cb49686880e2888d810eb6bbc4471e49df4ca217929ea09e20f` |

The installed package is a full current-source reconstruction, not adoption of
the old private package. The unchanged-source controllers were bootstrapped by
Root94 with `-O2`. `tools/source-package.cx prepare-context` established the
fresh context. `tools/source-job-wave.cx` actually ran all five tool jobs and
34 recipes through gen/tbl/net start/finish waves (at most four per wave),
followed by `final`. Raw commands, environment, output and status files are in
`EPOCH/out/epoch-build-01/outer/`; the aggregate is `job-outer-statuses.tsv`.
The explicit host zlib/header inputs remain declared nonhermetic dependencies.

`tools/kernel-build.cx` ran bootstrap, prepare, prepare-assembler and both
ISA preprocess/assemble/link triples. `tools/source-package-pack.cx` ran
prepare, all 34 model steps, six route steps, declare, actual pack, seal and
independent `--verify`. `tools/product-build.cx` rebuilt all five container
slices and the real deflate APE. Every command had its own outer bound no
larger than 58 seconds; existing child budgets were retained. Raw controller
compilations and each phase's stdout/stderr are in `epoch-build-01`.

Before installation, the actual APE passed header locations 11/0 and default
`-run` smoke 16/0. The product-built `candidate-installer-O2.elf` executed
`tools/candidate-install.cx EPOCH GEN GEN/unisacc-next.com PC/compiler.pkg`.
Its two `rb_verify_candidate` calls require kind 2 and equal admission results
before replacement. This completed successfully; the previous bootstrap and
record remain in GEN. `tools/candidate-promote.cx` then verified the installed
origin, fresh SID, record/native receipt and terminal package bytes and copied
the exact product/record into the shared checkout. Backups and transfer
evidence are `out/c43-fullname-promote-01/` and its adjacent stdout/stderr.

## Installed-product executions

`tools/stream-runtime.c` was rebuilt by installed Rootd65 with
`-O2 -fno-trim-libc`, then `tools/stream-pack.cx` extracted that candidate's
actual models into `out/c43-fullname-streams-01`. Stream1 SHA is
`2e3111884b31b0b263616f1ea6a62feda999807c669210d1106f782740db4360`;
stream2 SHA is
`d23b7b4105146fb4ba67c084423984ee7186e3ef396b5bf6c973d447a57a8d1c`.
The three E2 test pins were changed only to the freshly bound stream1 identity.

`out/c43-fullname-run-batch-01.elf` is the product-built unchanged
`tests/run-batch.cx` controller. Each row below was actually invoked once with
`ROOT ABS_OUT_PARENT BATCH_ID [streams=ABS_STREAMS]` under outer58; tests compile
and execute through the installed product. Each has a PASS `BATCH_RUN` line
identifying full product SHA d65 above, with empty stderr and no UNKNOWN.
Logs are `out/c43-fullname-validation-01/BATCH_ID.registry.{stdout,stderr}`.

| Batch | Assertions/cases passed |
|---|---:|
| e2-input-binding | 44 |
| pp-header-locations | 11 |
| negative-diagnostics | 211 (known_gaps=1 retained) |
| pp-cli-targets / includes / macros | 20 / 20 / 23 |
| stream-e2-domain-positive / refusal | 103 / 187 |
| stream-e2-independent / pp-gold-linux | 36 / 21 |
| smoke | 16 |

The 44-assertion binding execution used the collaborator's then-staged extension
of `tests/e2-input-binding.cx`; this change does not commit that path on their
behalf. All seven model records physically matched the candidate members and
the extra single-byte negatives and before/after identity checks passed.

The full-name probe now also accepts `--installed`. It requires the actual
`ROOT/unisacc.com`, retains all 14 assertions, and expects the no-override long
name/`#line` inheritance case to succeed. Its default mode still requires the
old-product failure. Reproduce the installed check with a fresh output path:

Both modes passed 14/0 with empty stderr: installed output is
`out/c43-fullname-installed-probe-02.stdout`; default-mode comparison using
`out/c43-fullname-promote-01/previous.com` is
`out/c43-fullname-retained-red-probe-01.stdout`. The latter still asserts the
original Root94 long-name failure. A final current-source/installed-product
identity check passed in `out/c43-fullname-installed-identity-01.stdout`.

```sh
tools/bound 55 sh ./unisacc.com tests/preprocess/pp-fullname.cx -- "$PWD" "$PWD/unisacc.com" /home/devali/repos/unisacc-epoch-fullname-778fc68ebc/out/epoch-pc-01/compiler.pkg "$PWD/out/fullname-new" --installed
```

Only the three affected header semantic/capacity matrix cells are reissued
from the new product's 11/0; other matrix stamps remain historical. Full N22,
all other domains, same-source second-generation fixed point and foreign-host
qualification are not inferred from these local executions.
