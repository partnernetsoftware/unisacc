# sizeof indexed postfix: private fast lane

State: private NN construction and product-level positive/refusal/regression
runs completed; formal construction/admission/installation remain open. Do not alter the
frozen `unisacc-epoch-aggregate-13d8cfe478` source or replace its ace3 inputs.
This change grants no installed-product or matrix PASS.

Next-run base (read-only update after the formal mixed build): use candidate
`f3705dfa3308b13fd7a5408236d8d19261d37ed03fc07bf1a07410578f8e23fa`,
source `ace3b44700ce593e99084edf299011be5730e4dac1f157f0b97e816f16ee5916`,
and its formal mixed PC `out/c43-f9-ace3-mixed-pc-01/compiler.pkg` SHA
`661c2b02bb9fce9f6d6939a3ee7b97242462daf5ce70b7604cea832d5fe53d6f`.
ACE_ROOT's installed product and GEN product already match that raw SHA by
read-only hashing. Shared-root migration is separately owned: confirm its
installed product matches f3705 after that operation before executing these
commands. This preparation does not claim the migration or any sizeof test
passed. The old d65 red remains historical evidence for d65 only.

The actual red is preserved at
`out/c43-typed-fixture-01/out/session-01/wave-3/typed-wave.stderr`: Root d65
rejects `sizeof material[nmaterial].name` used as an argument with
`not covered: argument list`. The minimal pending probe expects default-run
exit 7. It merges the previously deferred C44-CX-SIZEOF-POSTFIX-01 into this
version's work; the old deferred entry is historical, not authorization to
leave the defect open.

`exec/parse2/control-result.tsv` is the graph-rule fact input consumed directly
by `seed/gen.c:parse2_gen2_control_ex` through `control-manifest.tsv`,
`control-fresh.tsv` and `exec/facts/k2-control.tsv`. It is not a generated
kernel/weight table; the removed historical Python translator is not a current
generator entry. The facts' existing five postfix token classes now send
`SZ.sub` to `SZ.expr`, which rewinds `szpos` and enters the existing unevaluated
UNARY path. This preserves dimension metadata and the existing bitfield
rejection. The old shortcut consumed `[index]` but returned before `.member`,
leaving the caller to reject the remaining dot. No register/address allocation,
seed generator, finite constants, old assertions or timeouts changed.

The original preparation at shared HEAD
`6d4a04f74babdb7458cf7d35eec715064eb447c0` used d65/e154. Those two values
remain historical; they are superseded for the next run by f3705/661c above.
The rule/test and native constructor inputs are still independently pinned:

| Input | SHA256 |
|---|---|
| historical installed `unisacc.com` | d65c14744390932885660cf62c0ce77db7eeb4cefa46094909f1e05b0ba21b9f |
| historical base PC `compiler.pkg` | e154c747dbdfacc4c638d020f55fcd8998e0e6124d2b968c86cab4bf28a1d4e6 |
| `exec/parse2/control-result.tsv` | f2958ab86d246e30dc7aa3705d3b54fcc0c021e292a16939e16e4ec74475a350 |
| pending indexed postfix controls | 7b3f1a89119480544c6d5ef63d664ef8ab05c0e046aaf45c198472c37bdd1a7e |
| `tests/sizeof-postfix.cx` | 31ff98b5ad786db7823d8cdd81f9e09fd6602ff71fa86a4351c517c2c116d473 |
| native tool-gen | 5145075e08da56a5e4e25cf90d4eecf0366b322b49938b1cfd8889f04496818b |
| native tool-tbl | 12c360d41850513962d97fffcbbdbca3b16b1ae007bbc0e224b34c71c10b21a1 |
| native tool-net | 978b2abe9deeead07170ba72bfc16523f5b522325a883b47ef5ce2f926fdb664 |
| native tool-pack | d7aa065a8c6b3c753c7c0a76f04b2aebde73c3126fa18a1874b22063745ff38a |
| `tools/pp-fullname-pack.cx` | df79ea6b469fbecb8757598d202c2f109331d25cb615c7589ca080e90ad7802f |

Recheck hashes before the granted run. If this source changes, re-freeze its
identity and use new output paths; do not relabel old evidence. The controller
and test hashes must be re-measured if further edits occur. Read-only cached
native constructors are construction material, never cached test results.

Run these commands individually, sequentially, from the shared root after the
window is explicitly released. Each command and child stays bounded; inspect
the actual rc and checkpoint bytes before the next phase. The variable
declarations only abbreviate exact paths:

```sh
ROOT=/home/devali/repos/unisacc
J=/home/devali/repos/unisacc-epoch-aggregate-13d8cfe478/out/epoch-context-01/jobs
B=$ROOT/out/c43-f9-ace3-mixed-pc-01
M=$ROOT/out/c43-sizeof-postfix-f370-models-01
P=$ROOT/out/c43-sizeof-postfix-f370-private-01
```

First capture installed-product behavior without invoking a reference compiler
or leaving an ELF. The minimal semantic expectation is exit 7; the installed
red is expected to be exit 1, never a PASS. The complete positive/refusal
contract uses the same original 12-second child bound and records actual argv,
stdout, stderr, rc, and before/after product/package SHA:

```sh
tools/bound 12 sh ./unisacc.com tests/pending/sizeof-indexed-member-arg.cx > out/c43-sizeof-postfix-f370-baseline-min.stdout 2> out/c43-sizeof-postfix-f370-baseline-min.stderr
tools/bound 55 sh ./unisacc.com tests/sizeof-postfix.cx -- "$ROOT" - "$ROOT/out/c43-sizeof-postfix-f370-baseline-positive-01" positive > out/c43-sizeof-postfix-f370-baseline-positive-01.log 2>&1
tools/bound 55 sh ./unisacc.com tests/sizeof-postfix.cx -- "$ROOT" - "$ROOT/out/c43-sizeof-postfix-f370-baseline-refusal-01" refusal > out/c43-sizeof-postfix-f370-baseline-refusal-01.log 2>&1
```

Create the fresh output directory as a filesystem preparation step, then build
only the two actually routed models. The base declaration contains 120
`errorparse.net` rows, 120 `warnparse.net` rows, and zero `shared-e3.net` rows.
`shared-e3` uses the same parse2 rule source but has no M in this product route
set; it may be generated/linted as JSON if a specific rule-consistency question
requires it, and must not acquire an unnecessary tbl/net construction here.

```sh
mkdir "$M"
tools/bound 55 "$J/tool-gen/result.exe" parse2 "$M/errorparse.json" --errors
tools/bound 55 "$J/tool-gen/result.exe" parse2 "$M/warnparse.json" --warnings --errors
tools/bound 55 sh ./unisacc.com tools/table-lint.cx -- "$M/errorparse.json"
tools/bound 55 sh ./unisacc.com tools/table-lint.cx -- "$M/warnparse.json"
tools/bound 55 "$J/tool-tbl/result.exe" "$M/errorparse.json" "$M/errorparse.tbl"
tools/bound 55 "$J/tool-tbl/result.exe" "$M/warnparse.json" "$M/warnparse.tbl"
tools/bound 55 "$J/tool-net/result.exe" "$M/errorparse.tbl" "$M/errorparse.net"
tools/bound 55 "$J/tool-net/result.exe" "$M/warnparse.tbl" "$M/warnparse.net"
tools/bound 55 sh ./unisacc.com tools/pp-fullname-pack.cx -- "$ROOT/unisacc.com" "$B" "$J/tool-pack/result.exe" "$M" "$P" --aggregate-return
```

The reused packer mode's historical name is `--aggregate-return`; its actual
contract selects exactly warnparse/errorparse. Check its result says 2 changed
M, 30 preserved M, and all F resource bytes identical. It also rejects a base
PC that differs from the installed product's embedded package. If current
headers or another resource changed, retain its refusal and investigate the
specific input; do not relax the byte check. This private package stays
NON_ADMITTED and does not replace installed f3705 or formal mixed package 661c.
Read-only material checks confirm the mixed base has the required
`declaration/routes.tsv`, `declaration/predefines`, and `kernels/{arm64,x86_64}`.
Its 120 errorparse and 120 warnparse rows point to this same base's
`models/errorparse.net` and `models/warnparse.net`; no shared-e3 row exists.
Thus the existing packer accepts its directory layout and exact route path
selection without modifying the packer. Actual pack execution/byte comparisons
remain pending. Because the new parse2 facts retain the aggregate-return
change already present in 661c, preserve it with the existing member-return
probe and module/refusal regressions below; do not silently reset E3 to d65.

```sh
tools/bound 55 sh ./unisacc.com tests/sizeof-postfix.cx -- "$ROOT" "$P/compiler.pkg" "$ROOT/out/c43-sizeof-postfix-f370-new-positive-01" positive > out/c43-sizeof-postfix-f370-new-positive-01.log 2>&1
tools/bound 55 sh ./unisacc.com tests/sizeof-postfix.cx -- "$ROOT" "$P/compiler.pkg" "$ROOT/out/c43-sizeof-postfix-f370-new-refusal-01" refusal > out/c43-sizeof-postfix-f370-new-refusal-01.log 2>&1
tools/bound 12 sh ./unisacc.com --models "$P/compiler.pkg" tests/pending/struct-return-member.cx > out/c43-sizeof-postfix-aggregate-retained.stdout 2> out/c43-sizeof-postfix-aggregate-retained.stderr
tools/bound 55 sh ./unisacc.com tests/preprocess/aggregate-return.cx -- module "$ROOT" "$ROOT/unisacc.com" "$P/compiler.pkg" "$ROOT/out/c43-sizeof-postfix-aggregate-module-01" > out/c43-sizeof-postfix-aggregate-module-01.log 2>&1
tools/bound 55 sh ./unisacc.com tests/preprocess/aggregate-return.cx -- refusal "$ROOT" "$ROOT/unisacc.com" "$P/compiler.pkg" "$ROOT/out/c43-sizeof-postfix-aggregate-refusal-01" > out/c43-sizeof-postfix-aggregate-refusal-01.log 2>&1
```

The retained member-return default-run expects exit 7. Do not invoke the
aggregate-return harness's legacy `positive` mode on fixed f3705: that mode
intentionally includes an old installed-product red and must not have its
assertion removed to fit this new base. The candidate-only embedded test mode
is also distinct from this private-package check and must keep its no-model-
override contract. Test-source hashes and candidate artifact hashes identify
different inputs; never compare a product's MATERIAL hash to a test-source SHA.

Positive cases preserve the existing 12-member and 4-global-member controls,
plus indexed argument, 18 explicit ABI/dimension values, nested member arrays,
pointer-to-row sizes, callback return type and arithmetic/relational contexts.
Both increment and decrement, side-effecting subscripts and function calls must
remain unevaluated. All fixed-width oracles explicitly target Linux/x86_64
LP64. Negative cases require the existing named missing-member/bitfield
diagnostics with actual source path, rc1 and empty stdout; a valid default-run
probe after each rejection must return 7 again.

For coverage, obtain the actual E1-to-E3 UNITOK wire from a separately
identified raw stream route; that acquisition remains unimplemented here.
Do not use `-dump-tokens`: its human-readable token names are not that wire.
Only feed a correctly framed input to the new errorparse JSON with
`tools/sim.cx --cov` under a 55-second bound; do not feed raw C or the readable
token dump to an E3 token automaton. Check the trace enters the
new SZ.sub continuation and the unevaluated member/shape path. Sim is auxiliary
and grants no NN execution evidence by itself. Actual private-package runs
above must be the semantic evidence. Record any input-framing obstacle before
claiming coverage, and compare unchanged-case output/tape bytes against the
installed baseline where both accept. No full SID/PC/GEN, installation, broad
matrix migration, host qualification or unrelated batch rerun is authorized by
this fast-lane plan.

## Actual private verification (2026-10-28)

Shared product remained f3705dfa/record63e7 throughout. The fresh source helper
observed SID `353cb1189bbdff746a11918584c13c86f74fd4e2a13314a79bf2fb016aadbe88`
(root session20143, rc0). Installed baseline retained rc1 for the indexed
argument and failed 3/9 positive, 4/9 refusal checks; those are preserved red
observations, not expected-PASS relabels.

Both routed recipes completed gen/tbl/net with reported terminal0. Lint:
errorparse 10757 states/0 errors/1105 warnings; warnparse 11004/0/1128.
The packer actually reported changed2/preserved30/resources-byte-identical.
Private package SHA `ff696253808d017b4ac81e353729c918eb5c96a7a78bf375f564baa2e40ef955`;
it remains NON_ADMITTED. The new positive and refusal runs each passed10/0,
including the 18-width/unevaluated-postfix fixture and rejection recovery.
Existing aggregate module4/0 and refusal12/0 remained green, and the retained
member-return probe reported its expected rc7. Logs are
`out/c43-sizeof-postfix-f370-new-{positive,refusal}-01.log` and
`out/c43-sizeof-postfix-aggregate-{module,refusal}-01.log`; per-child argv,
status and output are retained beside them. No installed product was replaced.

The auxiliary simulator probe did NOT cover the new states. Its input came
from `-dump-tokens`, a human-readable token-name stream beginning `typedef`,
not the E3 UNITOK1 wire. Actual stderr is `reject: not covered: malformed token
locations`; coverage contains only UC.start/UC.magic0/UC.bad. Preserve
`out/c43-sizeof-postfix-min.{tokens,cov.tsv,sim.stderr,sim.stdout}` and report
state coverage UNMEASURED until a correctly framed input is supplied. The
worker hit service capacity afterward; root did not infer its simulator exit
code from the log or restart it. This auxiliary fixture error does not replace
the actual private-package product evidence above.

Regular construction now uses the immutable export
`/home/devali/repos/unisacc-epoch-sizeof-e5bade1851`, observed source
`353cb1189bbdff746a11918584c13c86f74fd4e2a13314a79bf2fb016aadbe88`,
and saved f3705/63e7 bootstrap and record. Context preparation exited 0.
The product-built `out/epoch-build-01/source-package-O0.elf` controller
compilation exited 0 (session24118); each subsequent job still executes and
verifies its own inputs and receipts. Tool-gen/tbl/net and all 34 gen jobs
completed; the three differing recipes also completed tbl construction.

`tools/source-model-compare.cx` ran through default `-run` (session97384,
rc0, empty stderr), comparing actual JSON bytes and binding each JSON to its
record, context and command status. The result is 31 SAME / 3 DIFF against
the original f9 context: shared-e3, warnparse and errorparse differ. Evidence:
`out/c43-sizeof-model-compare-01.stdout`. This is comparison evidence, not
complete origin/current DAG verification, reuse admission or functional PASS.
All three changed net jobs subsequently completed with actual rc0 and formal
records: current material totals 43 jobs (3 tools, 34 gen, 3 tbl, 3 net).
Private fixture mutations of output-sha and a JSON symlink were both rejected
by the comparer with actual rc2 and their expected named errors; original
contexts were not modified. Logs are `out/c43-model-compare-{badsha,symlink}-01.*`.
The new execution profile, regular package and installation remain separate
steps; the installed product is still f3705.

The finite `f9-sizeof-v1` material role now records these observed identities;
its 22-assertion role suite exited 0. This does not authorize execution with
the ace3 profile. The writer's explicit unprepared-profile refusal was run
through default `-run`: rc2, `material-profile-execution-anchors-UNPREPARED`
(`out/c43-sizeof-writer-unprepared-01.*`). The reader selects current_spec for
the current union and has the same unprepared-role boundary; its new branch
has only been reviewed, not exercised against a new package yet.

The next execution stage now has independent observed identities. The saved
f3705 bootstrap with explicit regular661c package built the new O2 origin
executor (session39547, rc0), ELF
`bdf8125488535521c258ef070ba81741a64a58714c84addf57a1b418b7c4a278`.
Its actual write-profile produced SHA
`1286c2cb65cf6c914bd177c3941b69e12969c274fc05ebf2f074c4208035c2c5`;
the snapshot then independently verified original f9's 107 jobs/34 models,
1005 physical reads (session44572, rc0). Closure is
`f4d8e92579494ff114949e3b3e1d25c5755e89c3bb9d554461f8cc9c44de1a8d`.
All raw evidence is under `out/c43-sizeof-origin-exec-01/`; old profile and
executor bytes were not changed.

Both new-source kernel pipelines completed bootstrap, material preparation,
assembler construction, preprocess/assemble/link for x86_64 and arm64,
all actual rc0. KGEN context is
`7be6f80638a13423601ab008812653dbfe5d407a3cf8bcd28fe3181d30a67270`.
The writer now selects separate finite execution identities for f9-ace3 and
f9-sizeof. Writer source `72ede138…` built O2 ELF `bf653aa5…` (session21761,
rc0). A prepare attempt with the old profile was rejected rc2 with
`reviewed-origin-profile-required`, before creating a new package directory.
The earlier UNPREPARED refusal remains historical evidence of that code state.

Regular mixed package preparation, declare/declare-finish, pack/pack-finish
and seal completed with actual rc0. The resulting package in
`out/c43-sizeof-mixed-pc-01/` is byte-identical to the earlier ff696 private
package; its newly generated material identities are context `7d69c72b…`,
manifest `cc6c5314…`, receipt `4c74d2cd…`. This equality does not transfer
private functional results into installed-product qualification.

The independent reader now selects per-invocation execution anchors by
material role, retaining the old f9 set. Default-run reader verification
(session8095) exited 0, reporting full origin107/current43, 3 new/31 adopted.
A supplied ace3 source in place of 353c was rejected rc2 with
`mixed-context-role-or-controller-mismatch`. Logs are `reader-01.*` and
`reader-wrong-source-01.*` under the execution evidence directory.
This completes package material verification, not product admission/install.

The new product-build O0 tool compiled with actual rc0 (session25383), SHA
`2d3439520d530f9a42c27ef8457952d06c518a36a48a585f7587f948867612d0`.
Its observe step (23592) verified product context
`93219be0f6ed7cc7dfd723c69f62f7559c07fbcace7fdeb6d5b16114fc2e42e4`.
The observation's jobs=150 is original107 plus current43, not 150 new jobs.
Product commands take OS/ARCH: `slice-win/x86_64` and
`finish-slice-win/x86_64` (likewise lnx/osx targets), although filenames use
hyphens. Initial hyphen-spelled commands were refused before construction or
finish; their logs are retained, not counted as successful steps.
The correctly named win/x86_64 slice (3220) and independent finish (43379)
both exited 0; four other slices, wrap, admission and installation remain.
