# E3 typed aggregate value-return cut

This source/model cut fixes local member aggregate returns. The installed
Rootd65 remains unchanged; the new network evidence uses an explicit private,
non-admitted package. Source SID observed after the final model changes:
`ace3b44700ce593e99084edf299011be5730e4dac1f157f0b97e816f16ee5916`
(`out/c43-aggregate-return-live-source-02.stdout`).

## Semantic change

The original `tests/pending/struct-return-member.cx` expects exit 7. Installed
Rootd65 rejects it with `not covered: struct return`, retained in
`out/c43-struct-member-baseline-01.stderr`. The old local-return fast path
compares the root variable's type against the function return type before
walking members: it compares `parent` with `child` in `return p.item`.

The same tracked `.cx` minimal case returns 7 with final private04 and empty
stdout/stderr (`out/c43-aggregate-return-original-final-01.*`).

`exec/parse2/structreturnexpr-template.tsv` now routes the local case through
the existing typed expression/copy path. That path parses the complete
expression, checks zero indirection and the final nominal aggregate identity
against the return descriptor, requires the semicolon, and copies the complete
aggregate value into the existing return storage. The returned value is
independent of later writes to the source object. Existing scalar returns
and the internal aggregate return calling convention are retained.

The two type-mismatch edges in `libraryimports-result.tsv` now reach the
source-owned `aggregate-return.tsv` diagnostic. A wrong scalar, pointer,
decayed array or different nominal aggregate reports a located
`incompatible aggregate return type` error. `uc_kind=0` makes this an invalid
input diagnostic, including under `-Wall`; it is not a feature-coverage gap or
a host-library binding error. The unchanged parser still owns other syntax
and unsupported-expression refusals.

This advances E3's native type/value ABI behavior. CXIF remains a textual
compatibility interface, and compile-time ownership/borrowing remains open.
Copies of ordinary C values do not establish unique ownership of pointer
fields or handles. No kernel or classic-reference C implementation was edited.

## Actual model construction and package boundary

The actual native gen/tbl/net tools from the admitted f9fff160 epoch rebuilt
these three recipes, all nine commands rc0 under separate outer45 bounds:

| Recipe | Generator flags |
|---|---|
| shared-e3 | none |
| warnparse | `--warnings --errors` |
| errorparse | `--errors` |

Final material is `out/c43-aggregate-return-models-03/{RECIPE}.{json,tbl,net}`.
Commands use the corresponding `tool-{gen,tbl,net}/result.exe` under
`/home/devali/repos/unisacc-epoch-fullname-778fc68ebc/out/epoch-context-01/jobs`.
Generation is `tool-gen parse2 OUTPUT.json FLAGS`, followed by
`tool-tbl INPUT.json OUTPUT.tbl` and `tool-net INPUT.tbl OUTPUT.net`.
Raw output is `out/c43-aggregate-return-final03-RECIPE-PHASE.{stdout,stderr}`.

`tools/pp-fullname-pack.cx --aggregate-return` reuses its existing byte-level
package comparison for this E3 family. The base package's declaration actually
routes `errorparse` and `warnparse`; `shared-e3` is constructed but not routed
there. Accordingly the new mode replaces exactly two compressed M records,
preserves the other 30 M records and the complete F resource suffix, and checks
all route/index bytes. Its original PP mode still replaces exactly three;
replaying it against Root94 reproduced the historical private package SHA
`7241237d...` exactly (`out/c43-aggregate-return-pp-control-01.stdout`).

Final package: `out/c43-aggregate-return-private-04/compiler.pkg`, SHA-256
`661c2b02bb9fce9f6d6939a3ee7b97242462daf5ce70b7604cea832d5fe53d6f`.
Its base is the actual installed Rootd65 terminal package, verified equal to
`/home/devali/repos/unisacc-epoch-fullname-778fc68ebc/out/epoch-pc-01/compiler.pkg`
(`e154c747...`). The native pack is that epoch's actual `tool-pack/result.exe`.
The packer never installs or writes admission. Failed construction attempts
and earlier private packages remain under their original `out/` paths.

## Executed tests on final private04

`tests/preprocess/aggregate-return.cx` constructs explicit fixtures and invokes
the actual product with `--models PACKAGE`. Child bound5/transport6 and outer55
are fixed. No verdict cache, host compiler or classical fallback supplies the
fixture answers; the existing executor/watchdog host-C bootstrap dependencies
remain. Its four modes passed:

| Mode | Assertions | Scope |
|---|---:|---|
| positive | 15/0 | Original installed red retained; local/member/nested/pointer/array/alias/global/chained returns; single evaluation; independent and full 24-byte value copies; identities |
| refusal | 12/0 | Five type mismatches, fresh recovery after each, identities |
| refusal-warnings | 12/0 | The same rejects/recoveries through the warning model; no `UNCOVERED` classification of the invalid types |
| module | 4/0 | Real `.cx` imports share the public aggregate type; private module state changes do not alter an already returned copy; identities |

Logs and per-fixture argv/stdout/stderr are
`out/c43-aggregate-return-MODE-02{.stdout,.stderr,/}`. A fresh-mode invocation is:

```sh
tools/bound 55 sh ./unisacc.com tests/preprocess/aggregate-return.cx -- positive "$PWD" "$PWD/unisacc.com" "$PWD/out/c43-aggregate-return-private-04/compiler.pkg" "$PWD/out/aggregate-new"
```

On the same final package, the existing `cx/check_memory.cx` batch passed
138/0 and `tests/library-abi.cx -- ROOT` passed 55/0, not_run=0, via actual
default-run compilations. Logs are `out/c43-aggregate-return-memory-02.*` and
`out/c43-aggregate-return-library-abi-02.*`. Their original runtime ownership,
borrow, native-host and incomplete-library scope statements are retained.

`tools/stream-pack.cx` extracted the private package into
`out/c43-aggregate-return-streams-02`, using the existing product-built bare
stream runner. Its E3 container SHA is
`d388ba640665e5495287ecae198fd38d21673e121992970effc5d398542118d7`.
`tests/stream-e3.cx` now permits a validated explicit regression pin while
retaining its exact independent Tape oracle and all original assertions.
That standalone stream passed 17/0 (`out/c43-aggregate-return-stream-check-02.*`):

```sh
tools/bound 55 sh ./unisacc.com tests/stream-e3.cx -- "$PWD" "$PWD/out/c43-aggregate-return-streams-02" "$PWD/out/aggregate-stream-new" d388ba640665e5495287ecae198fd38d21673e121992970effc5d398542118d7
```

The explicit pin records a private stream regression; it does not grant
installed-product model binding or change the registered default material pin.

## Lint and remaining integration

A freshly product-built `tools/table-lint.cx` checked the routed graphs:

| Recipe | Baseline | Final |
|---|---|---|
| warnparse | 11003 states, 42355 edges, 1550 registers; 0 ERROR, 1128 WARN | 11004 states; same edges/registers, 0 ERROR, 1128 WARN |
| errorparse | 10756 states, 41913 edges, 1498 registers; 0 ERROR, 1105 WARN | 10757 states; same edges/registers, 0 ERROR, 1105 WARN |

Final logs are `out/c43-aggregate-return-lint-{warnparse,errorparse}-02.*`;
baseline logs end in `-baseline.*`. The plain shared-e3 baseline and changed
graph both hit the loader's pre-existing `UD.errpos -> DIAG.report` unknown
target refusal; those rc2 results are retained, not counted as lint PASS.
Earlier private03 semantic/lint evidence is separate from the final04 results
above; private03 exposed the coverage classification issue under `-Wall`.

The installed product still rejects the original pending positive, and its
matrix qualification is unchanged. Next is formal source-bound construction,
admission and integration of the two routed E3 models, followed by structured
CX interface records and compile-time ownership work. This cut does not close
full C99 aggregate expressions, cross-platform ABI, N22 or publication gates.
