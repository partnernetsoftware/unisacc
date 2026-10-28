# E2 full semantic header names: private fast-lane evidence

The installed candidate94 truncates header `__FILE__` values to the 62-byte
UNIPP1 diagnostic-name limit. Its frozen-source header batch remains FAIL and
the current matrix remains `NEEDS_PRODUCT_CHANGE`. This source fix is verified
only with an external **PRIVATE / NON_ADMITTED** package, without installation,
SID/context/PC/GEN rebuilding or transfer of private results to candidate94.

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

No matrix qualification changed because of this private green. Installation
and same-candidate release/foreign-host evidence remain separate work.
