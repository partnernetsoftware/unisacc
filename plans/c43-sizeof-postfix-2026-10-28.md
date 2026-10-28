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

For coverage, first capture actual E1 tokens from the minimal probe with the
installed product's `-dump-tokens`, preserving its observed framing. Feed that
input to the new errorparse JSON with `tools/sim.cx --cov` under a 55-second
bound; do not feed raw C to an E3 token automaton. Check the trace enters the
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
