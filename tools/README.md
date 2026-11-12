# Development and construction support

Current source, candidate and installed identities are in prd.md §2 (work tree). Identity narratives before 2026-10-30 (Source329/P53/S447 epochs) are in archive/prd-c43-history-20261030.md and do not transfer to the CX-rewritten entry.

## release-oneshot：隔离发布演练与独立片

`sh ./unisacc.com tools/release-oneshot.cx -- --dry-run CANDIDATE LAYOUT NEW_STATE` 现算七资产SHA，写私有候选表并生成17相发布演练记录；`--stop-after PHASE` 可暂停，`--resume STATE` 续跑且输入变化或断点损坏拒绝。草稿先于CI，CI计划携带release_tag、提交X与APE摘要，release-decide证据使用数字1到8，演练证据全部UNKNOWN。MANUAL=0仅表示演练无人工步骤；此入口不执行网络、提交、上传、CI或公开，PLANNED不等于实跑PASS。

`--pack APE NEW_DIR` 从APE导出六个 `unisacc-{lnx,osx,win}64-{x86,arm}[.exe]`，逐份附同一P4及UNIPKG1尾；`--check-pack APE DIR` 逐字节核验。win/x86取APE外层PE至最后raw section，保留dispatcher与版本资源，不声称还原原plain输入；其余五片保持实际解压字节。两检查批次release-oneshot、release-oneshot-pack只证隔离演练与格式字节合同，不认证发布或六宿主原生资格。

编排夹具要求显式预编材料：先独立有界运行 `sh ./unisacc.com tools/release-oneshot.cx -o ABS_TOOL`，再运行 `sh ./unisacc.com tools/check-release-oneshot.cx -- ROOT NEW_OUT ABS_TOOL`。daily批次材料为 `release-oneshot-tool:path`；无材料记DEFERRED，直接缺参rc77，不把冷编超过12秒伪装成合同失败，也不放宽原时限。pack检查仍为无材料commit批。库存752源、94片，新增4源不跨片界。

## edit-span：断言替换与原子可见边界

```sh
sh ./unisacc.com tools/edit-span.cx -- FILE FIRST LAST FIRST_TEXT LAST_TEXT REPLACEMENT
sh ./unisacc.com tools/edit-span.cx -- FILE --text OLD NEW [OLD NEW ...]
sh ./unisacc.com tools/check-edit-span.cx -- [EDIT_SPAN_BINARY]
```

旧行模式保留1基闭区间及首末行前缀断言；`REPLACEMENT`是替换文件。`--text`的每个非空`OLD`必须在原始文件字节中恰好出现一次，自重叠出现也计数；各匹配须互不重叠，按原位置顺序一次替换，不在`NEW`里继续搜索。`NEW`可为空，可删除全部内容；UTF-8按字节精确匹配。成功rc0并输出`REPLACED`；参数不成对、空OLD、缺失、重复或重叠匹配rc2且不写原件，诊断不回显OLD内容。仅接受普通文件，拒绝符号链接，保留文件权限。

两模式均使用同目录独占临时文件，完整写入、设置权限、fsync并关闭后rename发布；这是**单文件原子可见性，不是并发CAS，也不是多文件事务**，不提供并发写者冲突检测或目录fsync持久性保证。独立批`check-edit-span`原预算55秒、daily、无材料依赖，以真实文件及子进程退出码检验合同；可选`EDIT_SPAN_BINARY`用于复用已编译子工具。父已观测8.63秒、14项PASS及`CHECK_EDIT_SPAN failed=0`；该历史观测不冒认当前候选或已安装产品资格，本次登记未重跑。

新增源库存只通过既有显式候选入口生成；下列命令由父统一运行，输出目录必须尚不存在，生成后仍须人工审阅，不自动安装。本次未生成、未运行库存：

```sh
python3 tests/update-tools-compile.py /tmp/omp-cdx2-editspan-text-20261109 /tmp/omp-cdx2-editspan-text-20261109-tools-compile-review
```

`agentwork/pkg-gaps.tsv`当前缺cc13–15记录，本次不补造、不改动；登记不等于库存或包缺口已闭合。

Darwin 时钟常量以 `exec/facts/clock-targets.tsv` 为唯一事实，
`tools/clock-targets-gen.cx` 维护 `include/time.h` 的标记块。
先生成并检查常量，再展平及导出携带头；`tools/check-clock-targets.cx`
只验证六目标常量与生成漂移；宿主行为和错误 ABI 由 Mac 原生探针验证。Darwin 使用 libSystem
`clock_gettime`，不再以 `gettimeofday` 代替单调时钟；Linux 保留系统调用，
Windows 保留签名适配 shim。`exec/facts/libc99posix.tsv` 的逐格绑定同时约束
编译器与 Mac 候选 smoke。两个 Mac 架构都须真实运行 50ms nanosleep，
并得到 [50, 500]ms 内非负单调增量；源码检查及 Linux 执行不算 Mac 收据。
没有现有原生流水线收据，不宣称执行或候选资格通过。

```sh
sh ./unisacc.com tools/clock-targets-gen.cx
sh ./unisacc.com tools/clock-targets-gen.cx -- --check
sh ./unisacc.com tools/libc99posix-gen.cx -- --write
sh ./unisacc.com tools/libc99posix-gen.cx -- --check
sh ./unisacc.com tools/cx-flatten.cx -- --write .
sh ./unisacc.com tools/cx-flatten.cx -- --check .
sh ./unisacc.com tools/facts-gen.cx -- --write . pp-autoinc-gen
sh ./unisacc.com tools/facts-gen.cx -- --check . pp-autoinc-gen
```

`kernel/unisa_headers.inc` 仅通过现有
`tools/carried-header-export.cx prepare ROOT NEW_MAT LIBNEED_NATIVE`、
`--verify ROOT MAT`、`install ROOT MAT` 接口重建；物料收据不等于产品资格，
禁止手改生成的展平头或携带头。

`clock-targets-gen` 是独立生成入口，不在旧 NN 作业的 `command.reads.tsv` 自动映射内。
完整变更清单仍须保留原始 clock 事实、time 头和 libc 契约，分别记录上述生成检查、clock 批次与携带头物料收据；不得删去未映射源文件冒称完整覆盖。
NN 作业预检只声明实际读取的派生表覆盖（本项为 `exec/facts/pp-autoinc-gen.tsv`）；携带头及其它 C/CX 实现通过新包探针、当前源单片构造及同候选原生验证另行验收。

### Context legality facts

`exec/facts/type-context.tsv` owns the four context rules, independent of the old
binary `weights/gold/type.tsv` answers. `facts-gen` reuses that table's `t1` order
(the TAX `ax` domain) and `gen2-literals.tsv`'s `tyint` integer class. It writes
`weights/type/context.tsv`: 4 contexts × 15 source types, with labels `legal`,
`illegal`, and `needs-null`. Arrays and functions are pointer-class after decay.
`ret-pointer` integers need a fresh null-pointer proof; this table alone does not
grant one. Only `ret-arith` and `ret-bool` are consumed by the return path;
pointer-target compatibility and null-pointer proof remain on the existing path.

```sh
sh ./unisacc.com tools/facts-gen.cx -- --write . type-context
sh ./unisacc.com tools/facts-gen.cx -- --check . type-context
sh ./unisacc.com tools/decide-gen.cx -- --write --each ctx=index weights/type/context.tsv CTX CTX.ask exec/parse2/context-result.tsv
sh ./unisacc.com tools/decide-gen.cx -- --check --each ctx=index weights/type/context.tsv CTX CTX.ask exec/parse2/context-result.tsv
sh ./unisacc.com tools/decide-gen.cx -- --write --each ctx=ret-arith,ret-bool weights/type/context.tsv RTC RTC.ask exec/parse2/returncontext-result.tsv
sh ./unisacc.com tools/decide-gen.cx -- --check --each ctx=ret-arith,ret-bool weights/type/context.tsv RTC RTC.ask exec/parse2/returncontext-result.tsv
```

`context-manifest.tsv` installs the independent `CTX.ask.<ctx>` rows once. The
caller uses TAX then `RLD ax` to enter the chosen state, and owns preservation of
`vt`/`vb` and the `CTX.legal`, `CTX.illegal`, `CTX.needs-null`, and
`CTX.out-of-domain` destinations. No RESD state or old type/kernel answer changes.
`returncontext-manifest.tsv` separately installs `RTC.ask.ret-arith` and
`RTC.ask.ret-bool`. Before target conversion, the return consumer classifies the
actual target with TAX (including encoded function pointers), then classifies
the original source. Non-Boolean arithmetic targets reject pointer, array and
function addresses and void/aggregate sources with a return-type diagnostic;
Boolean targets allow pointer-class sources. Legal arithmetic conversions and
explicit casts retain the existing ASSIGNCV/NARROW path. This is a finite
category constraint, not complete C99 scalar-assignment compatibility.


`sourceflat.cx` is the native complete-source exporter:

```sh
sh ./unisacc.com tools/sourceflat.cx -- out/unisacc-flat.c
sh ./unisacc.com tools/sourceflat.cx -- --verify out/unisacc-flat.c
sh ./unisacc.com tools/sourceflat.cx -- --root archive/reference-c out/reference-flat.c
```

`--root DIR` reads the canonical source tree from DIR (the retired reference C
lives in `archive/reference-c`); the tool's own identity and the shipped
`include/` still come from this checkout, so `source.json` names which tree
every byte came from.

The export and provenance verification use the product directly. `export_ref.sh`
is only a compatibility loader for existing construction callers.

`object-material.cx` prepares three real product-generated Linux unit objects
from two header-free CX sources. Preparation and verification are separate
commands; preparation requires a fresh absolute directory:

```sh
tools/bound 55 sh ./unisacc.com tools/object-material.cx -- prepare ROOT NEW_ABS_MATERIAL
tools/bound 20 sh ./unisacc.com tools/object-material.cx -- --verify ROOT MATERIAL
```

The literal sources are `extern int object_value; int main(void){return object_value;}`
and `int object_value=7;`. Actual product invocations use
`-O2 -c -funit -b TARGET INPUT -o OUTPUT`, producing `main-x86_64.o`,
`value-x86_64.o` and `value-arm64.o` for Linux x86_64/x86_64/arm64 respectively.
Each of the three compilation children is bounded at 8 seconds and must return
0 with empty stdout/stderr. Preparation also makes two fresh full-source
identity observations, each bounded at 8 seconds: explicit child budgets total
40 seconds within the original 55-second parent. It records actual child
arguments, rc and output streams as construction evidence.

The exact `material.tsv` record binds the full source identity, raw product,
tool and netpack module SHA, fixed flags/targets, both literal source files and
all three object byte hashes. Preparation rechecks source identity and input
content before recording PREPARED. Pure verification never compiles objects:
it checks exact literal bytes and receipt/content identity, independently
observes full source before and after, rereads the content record, and finally
rereads the receipt itself. The two source children retain 8-second bounds
within the original 20-second verify command. Neither repeated reads nor
matching endpoints establish an atomic snapshot or detect every transient
concurrent mutation; inputs must remain frozen during execution.

`out/c43-object-material-prepare-02.log` and
`out/c43-object-material-verify-02.log` record successful bounded MAT02
preparation and verification, with exactly
`PREPARED objects-material no-test-result` and
`VERIFIED objects-material no-test-result` status lines (tab-separated).
The tool SHA changed with the endpoint checks, so MAT01 evidence is not
transferred to MAT02. These are material PREPARED/VERIFIED observations only.
They establish no linked-program semantics, ARM execution, object refusal
contract or functional PASS; no unexecuted parent CASE is registered here.
Internal phase and cold/hot timing remain UNMEASURED.

`run-startup-bench.cx` measures first-observed and repeated default-run
end-to-end wall time on Linux/x86_64 LP64. It generates one header-free minimal
CX program and starts four fresh processes with different argv; each must
return0, have empty stderr and match its exact distinct stdout:

```sh
tools/bound 55 sh ./unisacc.com tools/run-startup-bench.cx -- "$PWD" "$PWD/out/run-startup-new"
```

NEW_OUT must be a new absolute directory. Each target process has an8-second
watchdog; full raw-source identity is separately measured before/after with
two8-second controls. All child budgets total48 seconds under the55-second
parent bound. The tool also checks product, identity-source, benchmark-source
and generated-fixture SHA before/after, and saves each sample's stdout, stderr,
rc and duration plus `samples.tsv`. Failed semantic or identity checks invalidate
the aggregate observation; there is no speed threshold or cached test verdict.

The current P53e2cd1f/S447f794a run,
`out/c43-run-startup-bench-01.log`, completed9 assertions with zero failures:

| Sample | End-to-end ns |
|---|---:|
| first | 2008658900 |
| repeat1 | 2007829223 |
| repeat2 | 2076205842 |
| repeat3 | 2026560291 |
| repeat median3 | 2026560291 |

The existing Linux `CLOCK_MONOTONIC` syscall supplies the timestamps; ns is a
representation unit, with clock resolution UNMEASURED. The measured interval
includes watchdog/spawn, compilation, execution and pipe capture, while output
checks and evidence writes occur afterward. Source-identity controls can warm
product/OS state, so first is not an OS-cold or cache-cleared observation.
Startup, preprocessing/modules, construction, loading, binding, inference,
user execution, output, validation and cleanup are not measured separately;
all remain UNMEASURED. Four samples do not estimate p95. This supplies the
current baseline for [C44-RUN-REUSE-01](../plans/v0.0.44.md), not a cache-hit,
phase-cost or acceleration claim. The run-reuse mechanism remains unimplemented.

`run-watchdog-bench.cx` independently compares a guarded resolved `/bin/true`
with one guarded fresh default-run of its own header-free CX fixture. The
fixture receives a fixed argv and must print the exact distinct `WATCHDOG_ARG`
marker; no-op must return0 with both streams empty. Both samples are real
executions, not reused results:

```sh
tools/bound 55 sh ./unisacc.com tools/run-watchdog-bench.cx -- "$PWD" "$PWD/out/run-watchdog-new"
```

Each target and each of the two full-source identity controls has an8-second
child bound:32 explicit child-budget seconds under the unchanged55-second
parent. Before/after hashes bind the full product, controller, netpack, watchdog
wrapper/C source, resolved `/bin/true` binary, generated fixture and identity
source; `/bin/true` realpath and complete raw-source identity are rechecked.
Each sample retains argv/environment, stdout/stderr, rc, duration, and its own
private `noop.trace.raw` or `cx.trace.raw`, explicitly passed through `np_run`
environment additions. Trace bytes and their SHA are retained without parsing
trace events into PASS or internal phase timings. Transport failures record rc
as UNAVAILABLE. Tracing overhead is included in these end-to-end intervals.

On P53e2cd1f/S447f794a, `out/c43-run-watchdog-bench-01.log` completed12/0 under
the original55-second bound. `samples.tsv` records no-op28015979ns and
CX1932617292ns, both with exact outputs MATCH. The raw trace timestamps are:

| Sample | bound_start | child wait returned | exit normal |
|---|---:|---:|---:|
| noop | 19394254.108306 | 19394254.118615 | 19394254.123554 |
| cx | 19394254.137433 | 19394256.053985 | 19394256.058899 |

This observation does not support a fixed two-second watchdog exit wait.
The traces expose child completion and cleanup event order, but do not measure
product startup, compilation, loading or inference separately; those phases
remain UNMEASURED. No no-op subtraction, speed threshold, statistical estimate,
cache result or acceleration implementation is claimed. The original four
startup samples above remain unchanged, and these single traced controls are
not an OS-cold or phase-cost benchmark.

`stream-pack.cx` prepares seven isolated stream artifacts from a candidate's
existing model bytes. Its shared runtime and the new partial contracts are
documented in [streams.md](streams.md). Preparation never records a test PASS.

[nets.md](nets.md) describes the native full-gold integer-kernel batch and its
identity/anti-vacuity controls. [assembly.md](assembly.md) tracks the native .S
construction path, its real encoding checks and the remaining gaps.
[kernels.md](kernels.md) specifies the new bounded construction steps and native
ELF-to-UNIKERN1 linker contracts; it does not claim a completed product rebuild.
[packages.md](packages.md) describes the pinned, source-checked migration of
unchanged P3 models to freshly built native kernels and a rebuilt driver.

The following support was separated from the archived test suite so that
construction is not broken by its removal: `bound`, `bound.c`, `cxbin`,
`safedel.cx`, `lib.sh`, `build_ref.sh`, `refshim.h` and `reffoot.h`.
These are transitional construction adapters, not the new test framework.

Remaining dependencies are explicit: the native watchdog cold bootstrap and
independent reference compiler still use a host C compiler; `cxbin` has an
existing C-oracle bootstrap for the deletion guard. The old watchdog's Python
cold path has been removed. A direct self-hosted run of the old watchdog source
currently fails on the uncarried `sys/prctl.h`; this is an active migration gap.
Existing model coordinators still containing Python are not described as native.

现行源身份以 `seed/ident.c` 的 `source-packages-v2` 为唯一权威；归档 Python
和平铺 path-NUL/raw32 算法不作现行实现。默认输出仍严格为64个小写hex加换行
（65 bytes），不增加构造provenance字段。新组合SID不继承旧SID的候选资格。
`packages` 输出标题 `source-packages-v2` 和固定顺序12行包名/SHA：
`cx/sys,cx/mem,cx/text,cx/core,cx/driver,cx/runtime,cx/compiler,src,exec,seed,weights,include`。
kernel及旧十三个tools构造支持归seed，iterate/typekw归exec，cx根其余源码归cx/core。
规范真身全字节只计一次；alias拓扑独立哈希，不夹带目标content digest。
`package-inputs` 列owner/类别/规范真身路径/SHA，`list-aliases` 列owner/logicalalias/真身，
`list-current` 保持真身路径/SHA两列，`changes OLD_TABLE` 严格校验旧表后报告变更包及前后SID。
seed/include整体保守纳入，只排除明确README.md与.DS_Store；新分片、私有头、未知后缀、无后缀及有序组装清单不能被basename白名单漏掉。清单原始字节受哈希，同集合换序也变seed摘要；H组装过程本身不在F实现。真实include逐边闭包延期0.0.50 G1，不把目录树哈希称为逐边证明。
版本注入和平台资源生成规则仍属于源码语义，只有release/candidate.tsv发布数据排除；
limits、drive、version声明文件仍整文件计入，不靠排除整文件隐藏语义。
`table-kernel-export.cx` 直接消费权威 `list-current`，仅排除两个生成math目标，
另绑定producer支持及产品，不维护第二份语义选择器；NN及产物bytes合同不变。
`identity-material.cx` 保留完整核心算法；其固定base pin必须绑定最终实际核心SHA，
历史P53 bootstrap pin仅说明材料来源，不授予当前候选资格。
nativefirst v4桥标记 `source-packages-v2`，保留显式历史legacy-projection，
不宣称旧新SID相等。v1包表与缺少新schema/身份字段的旧缓存不能补字段重标；物理#line保持，物理根参与执行身份，不引入source-map。构造身份工具后的回归入口：

```
tools/bound 40 sh ./unisacc.com -O2 -fno-trim-libc seed/ident.c -o out/ident
make identity IDENT_BINARY=out/ident IDENT_OUT=out/identity-fresh
```

This regression checks support-source binding, not the complete build pipeline or
Python runtime parity. Changed source identities invalidate old build manifests.

`cxlib/` retains five native support modules still imported by exec/ callers.
Eight exec/ module symlinks now point here instead of the archived tests tree.
The host watchdog and reference-export paths inside the modules use tools/.
These compatibility callers do not become the new test gate, and their historical
PASS records are not imported. Four caller-family dependency plans and a six-check
default-run support probe passed after relocation; complete historical suites
were not run. Existing external-reference dependencies remain explicit debt.

New `.cx` tools use module interfaces and the implicit runtime without manual
`#include`. Compiled-artifact caching is reserved for measured setup hotspots;
it must not be confused with caching test results.

`pp-gold-material.cx` is a local pp-rule-to-deploy-NN material constructor,
separate from the existing weight-reuse `net-material.cx` path:

```sh
tools/bound 55 sh ./unisacc.com tools/pp-gold-material.cx -- prepare ROOT NEW_ABS_MATERIAL
tools/bound 20 sh ./unisacc.com tools/pp-gold-material.cx -- --verify ROOT MATERIAL
```

Preparation derives six disjoint cubes for9 directives x2 defined states from
the fixed pp rule source, with sparse W2=1, and emits `pp.weights`,
`pp-observe.cx` and `material.tsv`. The142-byte NN uses the actual deploy
kernel's little-endian masks/sparse-entry representation. Its C mathematical
body is extracted exactly from the raw `KERNEL_BODY` in
`unisa/ckernel.py`, stopping before main. No Python interpreter or
`np_convert` runs, and no old weight value supplies the new cube weights.

Source447 MAT01's weight SHA is
`2fd7181593523ce306cbcf0825b2d6ebd5dde3f69cf17762e8beeb6ebf25e609`;
the actual C math-body SHA is
`463b02ddd20c31182cbf90458b7cf308992f903b931bbdd8d355d38edee52eb2`.
The receipt binds recipe, ABI, root/source, product/tool/source-text/support
hashes and actual material bytes. Both paths observe fresh source endpoints and
repeat applicable content reads; verify finally rereads the receipt. Verify
admits the fixed142-byte recipe field by field (12 masks, six ordered sparse
unit/head/class entries, W2=1), without calling construction or rebuilding NN.
An altered weight plus a synchronized unsigned receipt therefore remains
ineligible as the baseline.

The actual prepare55/verify20 commands completed rc0 in
`out/c43-pp-gold-material-{prepare,verify}-01.log`. Their statuses are only
PREPARED/VERIFIED no-test-result. Separate tests documented in
[tests/README.md](../tests/README.md) actually completed92/0 for new-weight
inference and23/0 for synchronized-receipt recipe refusal, each within55.
The former runs one real observation child8 between verify20 calls, checking
all18 keys/four logits, boundary refusals/recovery and a private valid W2=2
candidate's actual score change. The latter uses verify20+20 and final source8;
it performs no inference or reconstruction. Neither test verdict is cached.

The `.py` files remain C-source-text/rule dependencies. Full SID observations
also hash old weight files, and the bootstrap product is trusted; this is local
new PP weight-value independence, not complete Python independence or a claim
that the whole execution closure never reads existing model bytes. Current
P3/product/generated sources remain unchanged. No equality with the old NN blob,
all18-net reconstruction, atomic snapshot or foreign-host qualification is
claimed. The19 broad qualification gaps and unmeasured internal/cold/hot phases
remain.

The unsigned classic table-math repair is exported by
`table-kernel-export.cx`, independently of NN weight construction:

```sh
tools/bound 55 sh ./unisacc.com tools/table-kernel-export.cx -- prepare ROOT NEW_MATERIAL
tools/bound 55 sh ./unisacc.com tools/table-kernel-export.cx -- --verify ROOT MATERIAL
tools/bound 55 sh ./unisacc.com tools/table-kernel-export.cx -- install ROOT MATERIAL
```

All three actual01 commands completed0. The tool extracts the current raw C
KERNEL_BODY, emits only core/self math with its own producer/template hashes,
and preserves39 model/cases/header/weight/dense files byte-for-byte with their
old producer provenance. Install admits exact old generated targets, records
the actual before/after source identities and uses per-file exclusive sibling
temporaries/fsync/rename. Two renames are not one atomic pair; partial failures
retain explicit recovery evidence. It does not forge a new candidate or bypass
the package constructor-input comparison.

`out/c43-table-kernel-math-01/installed.tsv` records Source329 and new
core/self hashes. The saved P53 ran all18 actual tables over8770 keys/22694
decisions, with28868 field refusals/recoveries and72 adapter refusals, all0,
in `out/c43-nets-u64-bootstrap-all-01.log`. Host C99/O2/shift sanitizer
ran the generated self-test with exactly22694 decisions/0 wrong. Permanent
`tests/table-mask.cx ROOT SAVED_BOOTSTRAP ABS_CC NEW_OUT words|all-nets`
uses separate55-second commands: words9/0 and all-nets8/0, in the corresponding
`out/c43-table-mask-*-01.log` records. The words mode extracts actual
functions without forcing unsigned types; sanitizer errors or NOT_RUN cannot
pass. These are classic-math mechanism checks on Source329 using trusted P53
and a nonhermetic host C foundation. They neither reconstruct18 NN parameters
nor qualify a new complete product. The old signed sanitizer failure remains.

The source-P3 pipeline has two distinct controllers:
`source-package.cx` produces native seed tools and model jobs;
`source-package-pack.cx` consumes their verified receipts and a separately
prepared native kernel generation. The commands below document the available
CLI and required order. They are not an execution-success report or a claim
that the complete package/new product is qualified.

Use canonical absolute paths for ROOT and all existing inputs; each NEW path
must be a fresh private absolute directory. Keep source and construction inputs
frozen while executing phases. Each command owns its own55-second watchdog;
do not aggregate the107 jobs under one watchdog or relax individual budgets.

```sh
tools/bound 55 sh ./unisacc.com tools/source-package.cx -- prepare-context ROOT SAVED_BOOTSTRAP ABS_CC NEW_SOURCE_CTX
tools/bound 55 sh ./unisacc.com tools/source-package.cx -- --verify-context ROOT SOURCE_CTX
tools/bound 55 sh ./unisacc.com tools/source-package.cx -- --list ROOT SOURCE_CTX
tools/bound 55 sh ./unisacc.com tools/source-package.cx -- job ROOT SOURCE_CTX JOB_ID
```

There are107 construction jobs: five `tool-gen`, `tool-tbl`,
`tool-net`, `tool-compilerpack`, `tool-pack` jobs, then34 model recipes
with three jobs each. After the corresponding native tool jobs, run
`gen-RECIPE`, `tbl-RECIPE`, `net-RECIPE` in dependency order;
`--list` supplies the exact34 recipe IDs. For example, shared-e2 uses
`gen-shared-e2`, `tbl-shared-e2` and `net-shared-e2`.
Generation invokes the actual C API as STAGE OUT.json FLAG..., table conversion
as INPUT.json OUT.tbl, and threshold construction as INPUT.tbl OUT.net.
Each job creates an exclusive job directory with its actual output, command
argv/streams/rc and canonical dependency receipt. Normal generator/net diagnostic
output is retained. Prior or partial outputs are refused and retained rather
than overwritten. PREPARED jobs do not contain a saved functional verdict.

The context binds source, saved bootstrap and its optional build record,
current root product and its optional build record, CC path/binary/version,
native identity helper, controller and support hashes. Its fixed raw-environ
policy removes SEED_GEN_GENERIC, SEED_GEN_TRACE, UNISACC_SEED_TRACE,
UNISACC_RF_CAPTURE and UNISA_ATTRIBUTES before child execution; both context
and job receipts carry that policy. Presence of an empty environment value is
also removed. A job retains a40-second native command plus two6-second source
observations:52 explicit child seconds, with all controller/IO work still
required to fit the unchanged55-second parent.

The host CC is an explicit construction dependency. Native pack compilation
requires the zlib development headers and library for its `-lz` argument.
The host CC, system headers, linker, compiler subtools, runtime libraries and
zlib remain declared nonhermetic host inputs. The local out/ CX CC adapter and
downloaded/unpacked development package used during development are private
host preparation artifacts; they are not an installed compiler feature,
automatic dependency resolver or portable product capability.

The pack consumer currently pins foundation source
`9c2e71ef2094773bcada4c27a39871f3d0f5947555eb3ebe338aaddf203427ba`.
A foundation change needs explicit consumer compatibility review and fresh
identity material. Prepare KERNEL_GEN separately via
[the native kernel steps](kernels.md), under the same current full source and
root build epoch as SOURCE_CTX. It must contain digest-verified identity/helper
and assembler material, both ISA source/object receipts and arm64/x86_64
UNIKERN1 blobs. Old source-generation receipts are not renewed by renaming them.

```sh
tools/bound 55 sh ./unisacc.com tools/source-package-pack.cx -- prepare ROOT SOURCE_CTX KERNEL_GEN NEW_PACK_CTX
tools/bound 55 sh ./unisacc.com tools/source-package-pack.cx -- model ROOT SOURCE_CTX PACK_CTX MODEL_RECIPE_ID
tools/bound 55 sh ./unisacc.com tools/source-package-pack.cx -- route ROOT SOURCE_CTX PACK_CTX TARGET
tools/bound 55 sh ./unisacc.com tools/source-package-pack.cx -- declare ROOT SOURCE_CTX PACK_CTX
tools/bound 55 sh ./unisacc.com tools/source-package-pack.cx -- pack ROOT SOURCE_CTX PACK_CTX
tools/bound 55 sh ./unisacc.com tools/source-package-pack.cx -- seal ROOT SOURCE_CTX PACK_CTX
tools/bound 55 sh ./unisacc.com tools/source-package-pack.cx -- --verify ROOT SOURCE_CTX PACK_CTX
```

After pack-context preparation, run the model command once for each of the34
recipe IDs, then the route command once for each of six TARGET values:
`lnx-arm64`, `lnx-x86_64`, `osx-arm64`, `osx-x86_64`,
`win-arm64`, `win-x86_64`. These CLI target IDs use hyphens;
the resulting route rows use OS/ARCH names. Each model phase verifies its
tool/gen/tbl/net ancestry and copies the actual paired table/network bytes into
PACK_CTX/models without reconstructing weights. Route phases bind the actual
image-stages declaration and seven verified stage models. The complete
inventory requires exactly34 model pairs, six route files and two private
kernel files, including directory/mode/full-content observations.

Declare then executes native seed/compilerpack with the verified routes,
shared O1/E2/nativeabi, model directory and kernels, writing declaration and
audit material. Pack executes native seed/pack with compression and explicit
header/kernel/predefine mounts, writing PACK_CTX/compiler.pkg. Each command
has one40-second native child plus source6 before/after; prepare additionally
uses the existing kernel-context check bound15. These bounds leave no promise
that arbitrary inputs or maximum ancestry fit55 seconds. Each phase must
actually complete under that same limit.

Seal rereads all107 job dependencies,34 pairs, six routes, kernel construction
receipts, actual declaration/pack command evidence, carried headers and full
package bytes. It writes exclusive PACK_CTX/model-inputs.tsv and package.tsv
with source-package-v1 identity. The consumer's final `--verify` is pure read,
rechecking content/source endpoints without generating NN/package bytes.
The foundation `--verify-context` checks context only; its own pack and
package-level `--verify` entries remain NOT_READY. Use the consumer CLI for
these later stages. This source-package path is separate from the pinned
P7 package-refresh migration and does not invoke archived aggregation gates.

The v1 root build epoch binds ROOT/unisacc.com and the presence/content of its
.build.json as well as the saved bootstrap. Keep those bytes fixed through
context, kernel-generation and pack verification. A changed Root/record cannot
silently satisfy the original epoch, even with equal artifact bytes. Current
sourcepackage explicitly resolves that original epoch to the saved checkpoint
only after checking BOTH original raw and record hashes. Installing P53/S329
does not relabel PkgCTX03's S447/e084 epoch; its original bytes remain available.
Without that exact provider observation, prepare new matching material. Saved P53/S447 can bootstrap current Source329 while their
identities remain separate. That use alone does not create a Source329 product.

A precompiled native controller may avoid repeated controller compilation.
Its code-cache identity must bind actual ELF bytes, compiler/options/bootstrap,
controller/modules/source and the applicable context; revalidate this identity
when reusing it. Every job/phase still executes and freshly validates its own
inputs, output and dependency receipts. A listing, context verification or
prior verdict cannot substitute for construction or functional execution.
Endpoint-local digest observations are discarded before the next fresh
endpoint; no PASS cache is provided.

PREPARED/VERIFIED package status is content/source consistency evidence.
Independent consumer behavior, source-to-model semantics, same-candidate
product construction and platform qualification remain separate. The34
delta-model recipes are distinct from the classic18 gold-table NN parameters,
whose existing numeric artifacts and .py source files remain full-SID inputs.
Trusted initial bootstrap, root-assisted initial measurement, unsigned
receipts, host dependencies and remaining Python-source/construction debts
must stay explicit; no complete N22, historical process proof, zero-Python
chain, foreign-host execution or whole-source atomic snapshot is claimed.

The retained Source329 material milestone has gen13's nine kernel phases at
actual rc0, with a fresh context and both blobs. The old `5c7ad479...` consumer's
PC01 prepare, all34 model copies and six routes completed at actual rc0,
including fresh two-endpoint verification and copying of warnparse material.
The original producer's outer142/no-PREPARED outcome remains unchanged.
PC01 declare01 instead reached outer55 rc142 during full input verification:
no phases/declare output was created and the native command was not reached.
The prior frozen consumer `56a0be5e2d321456c825d3f8997fcd598303295b62e095af714a2f12dfc12117`
was actually compiled native with -O2. New PC03 prepare, all 34 copies, six
routes, declare, pack, seal and --verify each completed rc0 under original
outer55 limits. The source-package-v1 compiler.pkg is 1588745B, SHA256
`1c6a503090ee8d4189d6326139fa4a18b5db14b9682f1db0949e8ee79519db3e`;
model-inputs.tsv SHA256 is
`fbabeffcc27d9ca0a677ce793aea7645aa8d30a97893e483755a3aa14c950b8b`.
PC01 declare142 and PC02 pack142 remain retained failures. PC02's same-SHA
package does not convert its incomplete outer run into success. PC03 material
is PREPARED/VERIFIED only, with no functional-test conclusion. The independent
old-SP full-package observation and current PC04 results are recorded below;
product-v2/installer qualification, a new candidate and same-candidate
selfhost03 completed58/0, with further function checks pending. Root is now locally installed P53/S329; the prior S447 bootstrap remains saved. Source329 candidate A was fully constructed under ctx699
and archived; current candidate B's new-context reconstruction and58/79 gates
completed. Local installation and GEN14 construction are complete; Source329 selfhost
now completed58/0 in run03 within the unchanged original30-second gate;
run01 timeout remains historical; fclose is now SOURCE_FIXED with explicit
working headers, while the new-source product is NOT_REBUILT.

The compiled `tools/source-job-verify.cx`, using the prior frozen SP module a8aebdd,
actually verified net-warnparse twice in
`out/c43-source-job-verify-warnparse-01.log`: each endpoint physically read 53
files with six ancestry jobs/one model, at approximately 0.58s per snapshot.
This pure-read material observation reconstructs no NN and does not rewrite
the original producer's142 outcome. The independent `tests/source-job-snapshot.cx` API gate was actually compiled
native -O2 and completed outer55 rc0 with 67 assertions/0 failures in
`out/c43-source-job-snapshot-01.log`. Both endpoints observed 53 files/six jobs/
one model; exact negative reasons, complete failed-output clearing and recovery
all passed. A compiled controller caches code only: every invocation still
physically reads/verifies the applicable inputs, never reuses a PASS. All
original limits and 19 broad gaps remain.

Current `cxlib.sourcepackage`407f5ef7 retains the same independent pure-read API
`sp_verify_job_snapshot(ROOT, SOURCE_CTX, JOB_ID, FRESH_SID, SECONDS, &result, why, cap)`.
FRESH_SID must come from an actual current identity-helper execution, not a saved
receipt. Success returns0 with sp_result material identities, witness, physical
read count and measured elapsed_ns; failure returns-1 with a bounded reason,
clearing every byte of a supplied result. SECONDS must be1..55 and does not add
time to the caller's outer limit. Each call uses a fresh forked child/view;
retained per-endpoint bytes do not survive as a future verification result.
The companion package snapshot API verifies a complete prepared package.
Both APIs prove unsigned observed consistency, not historical construction,
atomic filesystem snapshots, NN behavior or new-candidate qualification.

The prior a8aebdd SP module's independent full-package double observation
actually completed rc0 on PC03 in `out/c43-sp-package-probe-run-01.log`:
1004 physical files,107 jobs/34 models per endpoint,7744849441ns and
7753816590ns. Four SP endpoints alone take approximately31s, beyond the
original selfhost30 limit. Before candidate construction, the hybrid threshold
was therefore changed from1MiB to64KiB/provider-v3. No30/55-second limit,
physical input read or assertion is removed; no PASS is cached.

New frozen consumer SHA is
`dd6f5e6068289d04346f202035fb1ae3c84a10e254630c7ba5a650c7a06deb46`,
and sourcepackage SHA is
`407f5ef7bd8cf0b65d1086db12b36ab6e19627dc8a0ed5f2e4002df8eae22442`.
KS8ad, Source329 and foundation9c2 are unchanged. Current PC04 prepare,
all 34 model copies, six routes, declare, pack, seal and --verify each completed
actual rc0 under original outer55 limits, reusing verified NN bytes without
reconstructing weights. compiler.pkg is 1588745B with the same1c6a503... SHA
as PC03; current model-inputs.tsv SHA256 is
`b10f49023fbcd78b813ec2adc1df05e42cfa6cd068bf2e383b4a80b38131f822`,
and package.tsv SHA256 is
`431639124a24de143a82dbb455fceb3fd8fa815b48085d5199853138952b6e3f`.
New407/dd6 full-SP probe02 actually completed rc0:1004 physical files,
107 jobs/34 models per endpoint,5110674321ns and5088791923ns. Every content
read remains real; these observations do not count as SELFHOST passing.
The strict new-pin gate's native02 build completed rc0, then original outer55
execution recorded67/0 in `out/c43-source-job-snapshot-02.log`, with53 files
per endpoint at approximately0.55/0.52s. Old PC03 and prior67/0 logs remain
bound to their original identities. All material is only PREPARED/VERIFIED,
not functional PASS. Root is now locally installed P53/S329, while historical Source329
candidate A was fully constructed and archived. Current new-context candidate
B construction and58/79 gates completed, followed by actual local installation;
current GEN13/14 construction and original30 selfhost03 completed58/0,
including complete after-validation, retained as Source329 baseline only.
Current fclose is SOURCE_FIXED/PRODUCT_NOT_REBUILT, with338/0 using -I include.

Source329 candidate A was fully reconstructed under GEN13 product context
`699ccb6169f39f77868f0835cd5dd13b9ee1bd7c773fc938cc968cad19a0b00d`:
all five slices and wrap actually completed rc0. Its4911193B complete artifact
SHA256 is `53e2cd1f669c92fda2c16dd93a9021cd4db97f05c57ab2d1acf9a835cbac9857`,
identical to old Root bytes. This is a full source reconstruction observation,
not a timestamp refresh. Candidate version0.0.43, direct smoke16/0 and
historical-bootstrap candidate admission58/0 actually completed; see
`out/c43-source-candidate-13-01.log`. Old rb813/product2872/test0e7 were
committed as9abe0e86cf. Before B installation, Root retained its original
S447 bootstrap record, SHA256
`e0843c0c025f05e728c8584a7fd5db51965345316b65c6316815bd57787dbe45`.

The old installer legacy-header branch checked kernel context/embedded bytes
without complete P7 admission. New rbce7f/installer e37 drafts use explicit
schema contracts:mode2 accepts only legacy kind1 and runs full P7 BOM,
corecompat and v1 verification; mode1 accepts only source kind2; mode0 retains
its original contract. The relevant source58 and schema/invalid-BOM79 gates
now passed for candidate B; these are API controls, not positive legacy
qualification or successful installer execution.
After checking ctx699 and actual hashes, Root's CX archived12 old product/native
receipt files in
`out/c43-native-kernel-13/product-context-699ccb61-before-legacy-admission`
(`out/c43-product-13-a-archive.log`), preserving the nine ASM phases and NN.
Current GEN13 candidate B's five slices and wrap each actually completed
rc0 under original55-second limits, with context
`14954ce18cff8a1b087c22a60b56546b19a3fb149005240ff8333fb108b98cba`.
Its complete4911193B artifact retains P53's original raw SHA53e2cd1f...
Candidate B's version command completed rc0, direct smoke16/0 passed, and new
rbce7 source-candidate native02/outer55 completed58/0 in
`out/c43-source-candidate-13b-02.log`; this is independent new-context evidence.

The initial schema gate's72/1 in `out/c43-candidate-schema-13b-01.log` was a
test expectation error:the old BOM's unisa/ckernel.py row1042 has old SHA
`db184ffec27565ad0a6d725d49710e7818af7d8e686fb642b67c862b88503aa3`,
while the current template is
`d3400bfb0d8dab72c8851eec67fba9c732815cb46ac90d9466791be027e5b9cc`;
it precedes kernel/unisa_core.c row1115. Production correctly rejected the
first changed input with exact model-input-digest. The original red log remains.
The test retains all72 obligations, adding seven independent unique-old-row,
order, fixed-hash and current-template-stability prerequisites. Native02 and
original outer55 then actually completed79/0 in
`out/c43-candidate-schema-13b-02.log`:all three exact schema/BOM refusals,
A5 output clearing, recovery and unchanged-material checks passed. This is an
invalid-old-BOM probe, not positive legacy qualification or selfhost.

Before positive installation, Root retained its original S447/e084 record. Installer
e37's native coordinator actually built rc0 in
`out/c43-candidate-install-runner-build-01.log`. A real installer negative
control on the old known-P7 schema/BOM returned child rc2 with exact
model-input-digest path=unisa/ckernel.py. Root's own thin CX caller checked
that refusal and completed outer rc0 in
`out/c43-installer-reject-probe-run-01.log`. Complete Root/candidate hashes
were unchanged; all six candidate metadata/backup/staged paths were absent
with ENOENT specifically. No installation occurred in that negative control. This is neither positive
legacy qualification nor successful installation.
Current installation:commit10d2a688a4 contains full-schema rbce7/installer e37/
schema79 admission. The real installer completed original outer55 rc0 with
INSTALLED development-only-no-release in
`out/c43-candidate-13b-install-01.log`. Root is now P53/S329, rather than the
historical S447 bootstrap-only identity. Its full raw SHA remains
`53e2cd1f669c92fda2c16dd93a9021cd4db97f05c57ab2d1acf9a835cbac9857`,4911193B:
identical bytes were actually reconstructed, not timestamp-refreshed.
Current Root.build.json SHA is
`21704e50abc28930d9a0792fdb9f895aee49b13cd163dc783451784d3bb73ef5`,
record.commit is `10d2a688a491d6b4740377cbe46d9eaabec00ce1`, source is329,
and native SHA is
`3d75d3557afa43d441a8bdfe6a7cbc992161da90326bfd0d751e9e3d0a115a7b`.
An independent helper/Root-record check actually completed rc0 in
`out/c43-installed-329-record-check-01.log`.
GEN13/bootstrap-before-install.com preserves raw53 and the original e084/S447
record, identical to the checkpoint originals. PkgCTX03's original Root epoch
continues through the explicit checkpoint provider's two raw/record hashes;
its old epoch is not relabeled. Retained Source329 fixed-point baseline (not transferred to changed source):RB SHA
`699b0a2cc5ffb1a791c024aa0b2cb0c21a9e2ffe213e77f69c4535132ceebd92`,
product coordinator SHA
`68d55d5548922930d1223445098d7e8418539e6d5d84e1e08ddab85fdbacd15c`.
GEN13/14 each actually completed five slices/wrap (12 independent original55
phases), without reconstructing PC04/NN/ASM. Current GEN13 v2 context is
`1660a13221c0d3034b396af62d4621290ba8bb61deb173128582629595df091d`,
with saved447/e084 builder. GEN14 context is
`5cf8787fec970a934c9d50caa2e9ead3116ca19ccdff0d3e9e24d756c080c437`,
with Root329/21704 builder. Full raw remains P53.
Actual original30 selfhost completed rc0,58/0 in
`out/c43-selfhost-source329-03.log`; bash builtin time, including watchdog,
recorded wall26.414s/user7.701s/sys18.999s. All four SP endpoints physically
read107 jobs/34 models/1004 files each, with5072600419/5155174425/5069025671/
5094686385ns elapsed. Eleven after-checks of Root/record/original builders and
frozen files were OK. Runner source/flags/context profiles are preserved in out/. Assertions, all
four physical reads, after-validation
and the original30 bound remain intact; no PASS cache is used.
Original01 timeout142 (older c275/149 contexts) remains. Run02 only failed to
find /usr/bin/time (wrapper127), so never started the test; it is neither a
test failure nor PASS. This is a local native two-generation fixed point, not
HOST-seed three-stage N22, foreign native qualification or version completion.

Current state:SOURCE_FIXED / PRODUCT_NOT_REBUILT. Final measured working
source is `0d59237f3250b59514a55ff529c77b665033e5feb71061f6fcaded0c7ef925c0`
after precise scanner repair and legitimate carried-header regeneration.
Header-only cf2270b2e103ead1d13859489a34d6eb9e57a127eb0bef5f347b17822cb6c16e
remains intermediate source-header evidence (identity01/helper8 rc0). Installed Root
remains P53/record21704/Source329; no new-source product has been rebuilt or
installed. Passing Source329 selfhost58 is retained as a baseline/history,
never transferred to the changed source.

Current stdio source SHA is
`a6f5cc020e04e547f2668f162f736b5eac423bf1ec1e241406a745d06e2b01e0`;
include/sys/_exit.h SHA is
`1b35e59300e2120420d3261c7239159c08332c43d2efb87bde9ad2594ed88701`.
Permanent tests/stdio-close.cx is unchanged at
`7106b12654dec939d6657a71b95ad379cb0247c585273d5c119d9f9cf3ecc8b5`.
Old default-run actually returned1 with338 assertions/48 failures (24 rounds'
EOF/ENOSPC checks) in `out/c43-stdio-close-before-01.log`. Explicit
`sh ./unisacc.com -I include tests/stdio-close.cx -- ABS_NEW_OUT` uses the
working headers:original bound55 actually returned0,338/0, empty stderr in
`out/c43-stdio-close-source-01.log`. The minimal -I include probe also returned0
under original bound12 in `out/c43-fclose-devfull-source-01.log`, with EOF/-1,
ENOSPC28 and closed FD/EBADF9. These are source-header observations, not a new
candidate or evidence that Root's default embedded library headers changed.
The old Source329 red and clean host02 reference remain historical evidence.

The scanner is now precisely repaired:seed/unisa_libneed.c SHA
`5f0469bcb6268bf979d8bb872424ae238c8f991b51543337b6f4b2750b2fdbb1`.
Host CC compiled it rc0 with POSIX200809/c99/O2/Wall/Wextra and implicit
declarations treated as errors. out/c43-libneed-attributes-01.elf SHA is
`5c6a7756fa4de143a3e74ec14f392fc8ee736755ad55043cb976ace0fb2bb011`.
Actual original55 `out/c43-libneed-attributes-contract-run-01.log` completed
rc0,12 assertions/0 failures for flat-attribute exact spans/closure/refusal/
stability. The same implementation is now permanent tests/libneed-attributes.cx,
SHA `fd7fa200469f1f5aecb3f7567ba0ab7c77759e48d6f417a562295b53988c8679`
(only comments differ from the original out/ contract). New -I include
bootstrap default-run actually completed original55 rc0,12/0 in
`out/c43-libneed-attributes-permanent-run-01.log`:

```sh
tools/bound 55 sh ./unisacc.com -I include tests/libneed-attributes.cx -- ROOT LIBNEED_NATIVE NEW_WORK
```

Both construction.libneed-attributes and library.stdio-close are registered
as independent SOURCE55 batches/coverage. IMPLEMENTED records an implemented
entry, not execution/PASS; broad library remains NOT_READY.

New tools/carried-header-export.cx SHA is
`432bf38606d945a1acb8508e04ab784127842593c3ac2a51013019ac1c00acc4`.
Its -I include/O2 native build actually completed rc0. prepare/--verify/install
each independently completed rc0 under original55, with empty stderr in
`out/c43-carried-header-{prepare,verify,install}-01.log`. The complete old
header names/text/routing/LIBKEY/OFF/DEP/BODY/ROOT canonical oracle actually
passed. The tool legitimately installed generated kernel/unisa_headers.inc,
SHA `482d376cc8eb2c5152512a4ead9841f47b32e23b948024a10718eb82b5806d9f`.
Old e63 generated-header bytes remain in the capsule and new material's
previous files. Old/new complete libneed tables actually share SHA
`aea1293b96da78c6a0cfaf0354e47e840d414de41bdea735bead30d736c8c853`;
autoinc projection probe out/c43-autoinc-projection-probe-01.cx SHA
`6363a2235de1781dbeb150a34f5694b60cb02814bdb292557a3781054e3cd6a3`
actually completed -I include default-run original55 rc0,3/0 in
`out/c43-autoinc-projection-run-01.log`. Actual six-header ordering and Latin1
line rules produced byte-identical complete canonical projections both
including printf and excluding printf. Both complete include trees and all
support inputs were freshly stable at four endpoints. Together with the equal
complete aea129... libneed table, this is necessary-condition evidence for NN
adoption. Adoption remains unimplemented; neither complete PP-fact equivalence
nor completed reuse/new-product construction is claimed.

Final fresh working SID is
`0d59237f3250b59514a55ff529c77b665033e5feb71061f6fcaded0c7ef925c0`
(`out/c43-fclose-source-identity-02.log`, original helper bound8 rc0).
cf2270 remains the header-only intermediate measured identity. Root remains
P53/record21704/Source329; the new-source product is NOT_REBUILT/not installed.
At `out/c43-source329-origin-01`, all seven source trees/typekw/ten support
files and old Root raw/record are actually sealed and checked. The original
CTX03 helper rescanned capsule/root and returned the complete329 SID, rc0 in
`out/c43-origin-capsule-snapshot-sid-01.log` and capsule snapshot-sid.stdout.
The origin capsule is now complete:706MiB,19 parts plus final verification,
each under original55 with actual rc0. SEALED SHA is
`87f7d1fd1469d170556f8a0cc2861d27e14de05f687e97e4805db135268c3273`;
the original helper twice actually rescanned the full329 SID. See
`out/c43-origin-capsule-final-verify-01.log`. Old receipts/epoch remain intact.
Current Source0d GEN15 actually completed all nine phases under original55,
context `67fb6c9b0dacf0c385bf05e49b49a67493d34bdb489afd5f91589a6d3471cd3f`.
The two blobs match old4d6e(7096B（含40B头，body7056B）)/32ce(7144B（含40B头，body7104B）); this is PREPARED, not functional
PASS. Root/record/runner/builder-source after-checks were OK.

Historical origin SP API98848f9f... and wrapperbf9a331c... were actually compiled native
with saved CAP/root/unisacc.com plus absolute-I current include/O2. ELF SHA is
`de8511f0501a1bafb1e2038b3efa89352022d19dccaa0130b0442346f1d8588c`.
Its independent saved compiler is53/record21704/source329, not live Root raw.
--write-profile completed0, profile SHA
`4793f9d98c489b224f020cbc58d0f217c33500cb496147474d4fe73ce8a54bba`.
The first --snapshot completed original55 rc0 in
`out/c43-source-origin-snapshot-01.log`:origin329,107 jobs/34 models/1004
physical reads, execution_reads148 and wrapper6,elapsed6397320828ns. Current
executing SP988 is separate from origin specification407. This is fresh old
DAG verification, not current Source0d construction or test PASS. That initial point remains historical; the later indexed37-check gate is recorded below.

Earlier NI module SHA was
`8becdff327ca5bfed12bbee94367cf59cd3dd91ab68933d0fae66394a4627fac`.
Its native rebuild completed0; original55
`out/c43-nninputs-probe-run-02.log` completed6/0,7791 reads=current3888/
origin3888/support15. This covers all four trees at four endpoints,ten Cdeps,
directory modes; complete includes and origin DAG have separate domains.
Two native calls' internal reads remain UNMEASURED. elapsed3627775504ns,
reader3626988686ns,native_current1503902949ns,native_origin1455617847ns;
four tree observations each took approximately0.157-0.173s. Old NIcd50's
3.4738s first observation remains historical. These are not OS-cold/statistical
acceleration, complete PP equivalence or completed NN adoption.

Current indexed SP a832 uses new verifier ELF02 52fc/profile02 6acb.
The permanent origin gate completed original55 with37/0; its two positive
snapshots read107 jobs/34 models/1004 files each in5082765804/5145743979ns.
The old98837 logs and out/c43-sourcepackage-before-index-988.cx remain
historical. Current execution code and old origin specification407 are
separate identities; this verifies Source329 material, not new Source0d
construction.

tools/libneed-material.cx221119 actually prepared material; derived CX6b48
and nativeE4 actually built. Both complete include-table controls completed8/0:
39810B, exact SHA
aea1293b96da78c6a0cfaf0354e47e840d414de41bdea735bead30d736c8c853.
Native observations1.452/1.473s versus CX0.161/0.159s are individual timings,
not OS-cold statistics. fast-attrs01 retained12/0 from old fd7 test logic.
The later report-label correction to REFERENCE_C_SOURCE avoids claiming a
host-C compilation history for CX; that labelled version has not yet rerun.
No seed/header/Source0d/KGEN15 input changed for this optimization.

NI a95 and profile writer954 actually built and created the reader profile.
E4.cache.tsv (dfab...,3318B) binds compiled headers NI1ed1cb..., original
material, saved bootstrap, source and module identities. tests/nn-inputs.cx9217
native02 built and completed original55 with104/0, including two complete
positive comparisons at approximately1.14s each. The original93/1 log remains:
Root.com was non-ELF, so its exact expected shape refusal was corrected.
All93 obligations remain, plus11 shape/private non-executable ELF-clone and
unknown-artifact refusal premises. The clone is only read, never executed.
SP/wrapper/origin-test commit02bb4c614d and NI/material/profile/test commit
789e7d4c6f record these mechanisms.

#### F：source-packages-v2 必要输入缓存迁移

上述 `E4.cache.tsv` 与 104/0 是历史证据，不转授新身份资格。
活动读写路径为 `tools/libneed-native-profile.cx` →
`ni_fast_profile_text` → `FAST_ELF.source-packages-v2.cache.tsv` →
`nninputs.ni_profile_check`。旧 `.cache.tsv` 不读取、不补字段、不回退；
放进新路径的 v1、错误 schema、缺字段或非规范字节均拒绝。
新档案强制 `identity-schema=source-packages-v2`、规范物理 root/origin、
两端实际必要输入摘要与 `execution-key-sha256`。执行键覆盖键之前全部规范
档案字节，包含实际 reader ELF、原材料、编译 include、模块源码、flags 和物理路径；
相同源码字节换物理根不能复用执行身份。保留原物理 `#line`，不做 source-map。
必要输入摘要不是完整 SID：fresh current/origin SID 与原 SP DAG 仍由调用者另证，
没有 NN PASS、重构 NN 或历史编译证明。所有原 material/compiler/artifact pin 保持原值。

迁移须用当前 writer/模块重新编译调用者并 EXCL 新建新命名空间档案；不得给旧档案
追加字段。当前改动尚未实跑。父代理运行原 `tests/nn-inputs.cx ROOT ORIGIN READER`
外，再运行 `tools/bound 55 sh ./unisacc.com tests/nn-inputs.cx -- --cache-v2 ROOT ORIGIN MATERIAL FAST_ELF LEGACY_REFERENCE_ELF`。
新模式在私有临时 ELF 副本旁依次放入 v1、缺 identity-schema、实际重建 v2，
通过真实 `ni_compare` 消费链断言前两者拒绝、后者执行两次 native table 调用。


The new adoption producer a546/native03 9457 actually completed prepare03,
pack01, seal01 and verify01, each within original55, for
out/c43-adopted-package-01. P3 is1589556B, SHA
e486e9523c30c6c4985050f12c5e6a4323f8f6d8d84b9088709083f83b3e52a5;
manifest20e5ac180c3e88a0fd415ca670a59e666731fa65d044a57948ade77657075a47,
receipt24ee9ffc4fb6e1b2d757b7f55e8dec03e424cf63804454fb6ff180e229adbbc3,
context2b1a53bd337b1899a76271139171372b16c71dcd11e913dec8b600c2173f45f4.
All34 NN and the declaration are explicitly ADOPTED from Source329.
All107 origin jobs receive fresh full-DAG verification, plus NI and current
GEN15 kernels; only pack is EXECUTED with current headers. Earlier prepare01/02
rc2 logs remain: the kernel suffix mapping was corrected from KGEN *.kernel
to private P3 kernels/arm64 and kernels/x86_64 without removing checks.
Raw kernels are7096/7144B, including40B headers (bodies7056/7104B).

out/c43-adopted-package-snapshot-01.log completed original55 rc0:
single-endpoint8.8639s, origin5.1836s, NI1.1473s, body9.0065s.
Current working source is S03a5: the fixed forward-result.tsv is frozen
as014e1ac4.... The actual G15 identity helper completed bound8 rc0 and measured
03a5e1b77b24c7fcfe92d289c53d772088bb1b07a94bb3d419a34c75e2bd2334
in out/c43-forward-decimal-source-identity-01.log; the SID is no longer pending.
Installed Root remains P53/record21704/Source329. The Source0d adopted package,
GEN15 and15/37/104 observations are earlier material/mechanism evidence;
they do not qualify the changed source or a new candidate.
RB3 source is implemented, but native/default-run compilation through the
old product remains blocked by its front-end encoding defect; no new slice
exists. Old forward-result.tsv emitted ccw_i+48 as one byte, turning ordinal10
(the11th result) into a colon. The badargs default-run returned1 at line400482
in out/c43-adopted-rb-default-repro-run-01.stderr; original Tape evidence is
out/c43-driver-tape-debug-01/stage-2243329-000.txt:400482.
The24576-byte struct-copy probe ran0, so this is not evidence of a large-struct
failure. source-package-forward-runner01 actually compiled native0 using
unchanged9c2 tool source. The new partialCTX is not prepared, and shared-e3,
warnparse and errorparse NN are not reconstructed. The other31 require a new
mixed-origin schema. Current NI full-equality must refuse this actual change;
do not exclude the table or reduce coverage. No new candidate exists.
identity-material a052 actually compiled native02 rc0, but MAT02 prepare,
verify and derived native compile/run remain NOT_RUN. The old bb7 source,
MAT01 prepare/verify0 and module-missing derived compile01 rc1 remain;
no acceleration is claimed. Original30/55, four full fresh endpoints,
19 gaps and publication restrictions remain.

These earlier timers have different scopes and must not be added. Four such fresh
endpoints are projected to exceed original selfhost30; a complete kind3
selfhost has not run, so this is not a measured selfhost failure. Next:
measure and reduce helper/controller overhead while retaining30, all four
full endpoints and all assertions. The forward-result source fix and S03a5 measurement are complete; prepare
the new partialCTX, reconstruct the affected three NN (not yet built),
and implement mixed-origin adoption for31. Then compile and validate the
already-written RB3 source, integrate the installer, and construct and qualify
a new candidate. Earlier adapter evidence remains scoped to Source0d. Package PREPARED/VERIFIED/snapshot
results are material evidence, not functional PASS. Installed Root remains
P53/Source329; current S03a5 has no rebuilt candidate. Source0d is historical material. The19 gaps,
foreign hosts, classic18 source construction and signing/publication
restrictions remain. The separately authorized csr private C44 v6 report of
approximately0.07s is not main43 qualification; its size+mtime shortcut is
not accepted into43, and -run reuse is not implemented in main.

Before registration of the adopted-package gate below, the origin and NI
SOURCE/lnx_x86_64/55 registrations were strictly parsed as IMPLEMENTED,
not PASS. Strict --list actually returned0 in
out/c43-adoption-testplan-list-01.log: metadata_valid1,98 batches,
79 implemented/19 planned/63 unprepared,113 coverage,
94 implemented/19 unresolved,executed0/NOT_EXECUTED. This parses metadata;
it does not execute batches. The corresponding latest actual original20
--all returned3 in out/c43-adoption-testplan-all-01.log:
113 coverage/94 implemented/19 unresolved,97 NOT_RUN batches and78 NOT_RUN
coverage,SMOKE16/0,executed_subprocess_batches0. Those counts precede the
new adopted-package registration. Before the union registration, strict --list actually
returned0 in out/c43-adopted-package-testplan-list-01.log: metadata_valid1,
99 batches/80 implemented/19 planned/64 unprepared,114 coverage/95
implemented/19 unresolved,executed0/NOT_EXECUTED. These are historical inventory
counts before the union registration, not batch execution or qualification.
The union registration was strictly parsed rc0 in
out/c43-source-job-union-testplan-list-01.log: metadata_valid1,
100 batches/81 implemented/19 planned/65 unprepared,115 coverage/
96 implemented/19 unresolved,executed0/NOT_EXECUTED. Metadata parsing
does not execute batches or provide an updated --all qualification. Older111/92 and109/90 evidence
below remains historical.

* construction.origin-package: finite37-check origin gate, with materials
  source-origin, source-origin-verifier, source-origin-profile and
  logical-source-package. Actual CLI:
  sh ./unisacc.com tests/origin-package.cx -- ROOT CAPROOT LOGICAL_PC VERIFIER_ELF VERIFIER_PROFILE NEW_WORK
* construction.nn-input-applicability: finite104-check NI gate, with
  source-origin and libneed-reader. Actual CLI:
  sh ./unisacc.com tests/nn-inputs.cx -- ROOT SNAPSHOT_ROOT TRUSTED_READER

A measured native -O2 controller may reuse compiled code; it never caches
a verdict. These gates do not cover DAG-leaf/status/extra-directory/
execute-bit/parent-symlink/normal-v1-alias mutations or NI data/Cdep/domain
mutations, OS-cold behavior, foreign hosts or full PP equivalence.

The adopted-snapshot adapter now has a separate permanent gate,
`construction.adopted-package`, registered SOURCE/lnx_x86_64/55 IMPLEMENTED,
not broad candidate qualification. `tests/adopted-package.cx` takes exactly
`ROOT PC FRESH_SID`; registry materials are `adopted-package` (sealed PC)
and `current-source-id` (the actual independently measured64-hex current SID
value, not a source record or filename). Expectation: `rc0:ADOPTED_SNAPSHOT`.
Actual invocation in this checkout:

```sh
tools/bound 55 sh ./unisacc.com -I include tests/adopted-package.cx -- . out/c43-adopted-package-01 0d59237f3250b59514a55ff529c77b665033e5feb71061f6fcaded0c7ef925c0
```

The probe out/c43-adoptedsnapshot-probe-run-01.log and permanent
out/c43-adopted-package-permanent-run-01.log each actually completed
original55 rc0/15/0. All15 obligations are retained: seven non-mutating
argument/SID/canonical-path controls and one actual positive single endpoint,
with full prefilled-output clearing where present. The probe's own51 reads
took9.355677247s; the permanent run's own51 took8.958785555s. Both retain the
actual pinned controller/source and separately named controller-bootstrap
P53/21704 role, which does not relabel NN epoch447/e084.
Neither run is RB kind3 integration, source/body mutation coverage, NN
functional qualification or a four-endpoint fixed point. Original30 and
the19 broad gaps remain. Before this union registration, Root's strict --list returned0
with the actual99-batch/114-coverage historical inventory above; completed batch
scheduling or qualification is not claimed.

The separate `construction.source-job-union` SOURCE/lnx_x86_64/55 batch
uses `tests/source-job-union.cx`, CLI `ROOT CTX FRESH_SID`; materials are
`source-job-context` (the prepared current context directory) and
`current-source-id` (an independently measured fresh64-hex value).
Expectation: `rc0:SOURCE_JOB_UNION`. Example for the actual current inputs:

```sh
tools/bound 55 sh ./unisacc.com -I /ABS/ROOT/include tests/source-job-union.cx -- /ABS/ROOT /ABS/ROOT/out/c43-source-package-forward-context-01 03a5e1b77b24c7fcfe92d289c53d772088bb1b07a94bb3d419a34c75e2bd2334
```

The permanent source retains all36 probe obligations without copylive.
The permanent path actually completed original55 rc0/36/0 in
out/c43-source-job-union-permanent-run-02.log using executing SP5cbc;
only its module pin changed from the probe, with all36 obligations retained.
Its two fresh net unions each observed12 jobs/3 models/89 reads in
1.449938355s and1.385322946s; the tool union observed3/0/35 reads in
0.053068219s. The actual original55 probe
out/c43-source-job-union-probe-run-01.log completed rc0/36/0 with empty
stderr: four nonmutating argument controls each check refusal, exact reason
and whole-result clearing; two fresh reordered three-net unions each verify
12 jobs/3 models/89 physical reads in1.446987410s and1.462053267s, with the
same canonical identities and data witness; the independent three-tool
union verifies3 jobs/0 models/35 reads in0.083057602s; the parent survives.
Two original net producer outer142 outcomes remain UNKNOWN. Material
verification does not change those outcomes or claim NN inference,
a current full107 package, mixed adoption or candidate qualification.

Current S03a5 partial context, three native tools and three affected NN
materials are prepared. All34 current gen jobs actually completed
original55 rc0; full JSON byte comparisons against origin found exactly
shared-e3/warnparse/errorparse different and the other31 identical. All
three new net native statuses are0; warnparse/errorparse outer142 remain
UNKNOWN. G16 construction and mixed material assembly are next. No
Source03a5 candidate has been built. Old Source0d15/37/104 and Root329
results remain historical. The new origin03 reader/profile was actually
compiled and its full positive snapshot completed0: ELF
`dfc1fbd76be4da34a3e9a4cfe7823a94a3498d47918ed7e0013a427fd7d9cac7`,
profile `aeb85359e6d9ff85a3563ac58aa879e283bee254c4d0906f7e3136833eb7f068`,
107 jobs/34 models/1004 reads in5.174877257s. Executing SP058a and old
origin_spec407 are separate roles. New origin04 actual native compilation,
profile and full-old snapshot each completed original55 rc0: ELF
`ad550418ff24462c2c2a017790b85e1c4a70f67d9b9f93adbfe75fde614d5a13`,
profile `24daf95e0f16f404f4ad1737203abb4a923add5e118cbef2d25c0e1506b9de05`,
executing SP `5cbceaa8a5bd424b5a40005c8b29c7db4f4ea70c982892a3bdeed9416c915aff`,
with origin_spec407 unchanged,107/34/1004 and4.910414267s.
Runtime provider-v4 uses1024 bytes, while material provider-v3 retains the
historical65536-byte declaration; old canonical material is unchanged.
The origin03/04 times are single observations, not a statistical speedup.
The original30-second budget, all four fresh endpoints and19 broad gaps
remain unchanged. The new100-batch/115-coverage inventory above is actual
strict parsing with executed0, not --all execution or candidate qualification.

Historical registry evidence before the two new registrations: original20 --list completed rc0 in
`out/c43-fclose-testplan-list-01.log`, metadata_valid1,96 batches (77
implemented,19 planned,61 unprepared),111 coverage (92 implemented,19
unresolved),executed0/NOT_EXECUTED. Subsequent original20 --all actually
returned3 with empty stderr in `out/c43-fclose-testplan-all-01.log`:
metadata_valid1/111 coverage/92 implemented/19 gaps; SMOKE16/0 only,
95 non-smoke batches and76 coverage NOT_RUN,executed_subprocess_batches0.
Unknown/unrun entries are not PASS. Historical109 coverage/94 batches are not
an updated count after these two registrations. Implementation metadata does not replace actual tests,
NN adoption or new-candidate qualification. Local installation is not complete qualification.
The19 gaps, foreign hosts, classic18 weight-source construction and-run44
remain unfinished; signing/publication restrictions are unchanged. Complete
classic18 source work,19 broad gaps and foreign-host qualification remain open;
no signing/Draft/Latest/publication or implemented `-run` reuse is claimed.

`cxlib.kernelsha` provides `ks_sha256_hex(input, length, hex65)`:0 succeeds;
a negative Linux errno reports failure (`-22` invalid arguments, `-95`
unsupported platform/layout). Caller errno is unchanged; module-local errno
is not a cross-module error ABI. Failure clears the first output byte when the
output pointer is non-NULL. It requires Linux/x86_64-LP64 AF_ALG hash/sha256,
creates fresh transform/operation sockets for every call, and has no fallback
or digest/result cache. Each sendmsg update is at most256KiB; socket and message
layouts are checked, interrupted/short sends handled, and all owned FDs closed.

The old ce019 module's actual formal probe matched empty/abc/65MiB fixed
SHA256:load51028690ns and memory-hash380484652ns, measured separately.
The current8ad60 module corrects the explicit error ABI. Its own fresh native
probe in `out/c43-kernelsha-module-probe-run-02.log` also matched all three
vectors:load55760174ns plus hash386130270ns (441890444ns total for this input).
The earlier software4501887772ns is a single-point comparison, not OS-cold or
statistical acceleration evidence. Fresh source observation still yields329,
recorded in `out/c43-source-after-afalg-01.sha256`. Its permanent gate
recorded144 assertions/0 failures within outer55 in
`out/c43-kernelsha-regression-02.log`; the original142/2 first-run failure is
retained in `out/c43-kernelsha-regression-01.log`. This establishes a bootstrap
kernel-crypto mechanism, not new-product/P3 qualification or a measured
implementation of `-run` compilation reuse. Producer integration and PC02
still require their own actual execution; the19 broad gaps remain open.

## 0.0.48 开发车道提速 T1/T4

`tools/dev-run` 的普通 `.cx` 入口保持不变；GCC 生成器必须显式启用，没有自动回退。以下构造入口每次真实编译 `seed/gen.c`，输出路径相对调用目录解释：

```sh
sh tools/dev-run --seed-gen out/seed-gen-product
sh tools/dev-run --seed-gen=product out/seed-gen-product
sh tools/dev-run --seed-gen=gcc-O2 out/seed-gen-gcc
```

未指定 GCC 开关的生成器构造使用当前产品版编译器 `-O0`；显式 GCC 车道使用 `gcc -std=c99 -O2`。发布构造的既有工具选择、原 40 秒限时和产品源码不变。`check-dev-product.cx` 保留安装检查，新增两版 `hostsymbols` 真实生成及严格字节相等冒烟；冒烟不是全 42 配方的等价声明。

全量对照是 Linux/GCC/GNU `date`、`timeout` 环境下的离线命令，不登记批次，工作目录必须尚不存在：

```sh
mkdir -p out && sh tools/dev-run --check-seed-gen out/t1-seed-delta
```

该命令只编译一次对照工具、分别真实构造产品版及 GCC-O2 版生成器，再运行当前规范配方的 42 个模型 × 两版生成，最后逐对检查成功标记、非空输出及长度和 `memcmp` 全同。配方参数来自 `cxlib.sourcepackage`；缺任何配方或执行失败都拒绝。仅此新增离线对照的单配方观察窗为 120 秒，编译/构造与原注册检查仍为 55 秒；发布和 s9 的限时不变。失败不重试、不删除现场，也不把部分完成称为全量通过。

离线目录的 `recipes.tsv` 保存配方清单，`timings.tsv` 记录每步 `step/model/lane/elapsed_ns/rc`；lane 0 为产品版，lane 1 为 GCC-O2。产品版超过 40 秒的配方须单列慢生成证据。这是单次墙钟差记录，不是稳定性能倍率或跨平台资格。

本次离线实测 84 次生成均 rc0，42 对 JSON 全部严格字节相等；总流程 425.01 秒。以下两项为「产品版生成器慢」证据，超过原 40 秒而未放宽任何发布或批次门：

| 配方 | 产品版秒数 | GCC-O2 秒数 |
|---|---:|---:|
| errorparse | 47.629704 | 8.317670 |
| warnparse | 48.056603 | 8.357375 |

首次产品版 `errorparse` 的 40 秒超时及首次误用通用限时器的拒绝现场均保留，不能替换为成功记录；全量通过仅指后续获批的独立 120 秒离线对照。

`affected.cx` 的表类别现输出 `via=table`，从原生静态保守表闭包关联精确模型/job/artifact消费者，不再拿旧观测快照冒充新表的完整依赖。新主树 `exec/tapeframe/frame-result.tsv` 曾在旧投影下漏选为零；当前真实CLI能选择 `source-job-union` 等消费者，且不把28个网硬等同28个批次。

```sh
sh ./unisacc.com tools/affected.cx -- --files exec/parse2/control-result.tsv --list-only
sh ./unisacc.com tools/net-deps.cx -- --table-deps
sh ./unisacc.com tools/net-deps.cx -- --check-table-deps
python3 tools/affected-model-inputs.py CONTEXT_OR_TAR --check
```

两份映射语义独立，旧观测投影及遗留Python投影器原字节保留，只作独立oracle，不与静态映射互相冒名。原生导出先核缺引用；只在rc0时安装新文件，失败不能覆盖现有映射。静态图按MODEL/路径排序去重，`--check-table-deps` 对漂移、漏边、多边、重复边及改边返回1，IO/用法返回2。

| 映射 | 当前边数 | 来源与用途 |
|---|---:|---|
| `exec/pipeline/table-deps.tsv` | 5210 | 原生net-deps的42配方静态保守闭包；含manifest/facts，排除共享生成器，仅exec下TSV；供affected及漂移检查 |
| `tools/affected-model-inputs.tsv` | 3970 | 历史source-package实际读取记录；仍由原投影器复算并供独立oracle，不能宣称当前完整运行读集 |

模型消费者只通过精确模型/job/artifact 字面量及现有 use/literal 工具闭包关联；不是整个 `exec/` 的粗 guard。包含通用配方模块的批次会保守扩散，计算所得的动态模型选择仍未完整建模；本映射不证明生成字节等价，也不改变发布资格。

## 0.0.48 M8：原生实测分档及逐文件棘轮

第十四列 `tier` 为 commit/daily/release。提交档是实测选择子集，daily累计commit与daily，release及默认无过滤仍覆盖全部登记；旧十二/十三列兼容，不减少原断言、40/45/55秒预算或发布条件。子集PASS不是全量发布资格。

两算法原生 `.cx`，不调用宿主算法。本轮误增的两份Python算法已撤回并从开发树删除，当前补丁不新增`.py`。现有`tests/update-tools-compile.py`仅作为明确登记的遗留库存入口保留，本轮适配十四列，不据此允许新增Python算法。

```sh
sh ./unisacc.com tools/select-commit-tier.cx -- "$ROOT"
sh ./unisacc.com tools/forecast-tier-runtime.cx -- "$ROOT"
```

显式接口：选择器`ROOT TIMINGS OUTPUT [JOBS]`，估时器`ROOT TIMINGS ESTIMATES SUMMARY [JOBS]`。只消费冻结289唯一批次的多快照实测TSV；原型extract/observe未保留，不自动用验收结果改档。输入保存原日志行号与历史观测，完整日志和SHA随包交付，不将旧快照测量冒作当前源码资格。

规则为完整PASS耗时升序、ID平局、最小增量关键路径成本，最多16任务；域互斥，serial/材料重项独窗。启动47.464787754秒、每波预留十秒，整数纳秒乘5/4，预测帽240秒。非PASS/未测/未选归daily，既有release不降级。旧冻结输入82项七波预测222.090062702秒曾逐字复现；按4102实测为cx-bounded补串行约束后，原生重算仍82项七波，预测212.885888647秒。当前新计划尚未完整重跑，不是通过声明。

当前单包基点`4102b514115b3bd23b35e6af5f6b00fc5165ade6`，保留所有tip新增行，649登记82/528/39，daily累计610、release/default649（637已实现、12PLANNED不虚构执行）。本tip整跑外墙210.836秒但只有81PASS/1UNKNOWN：cx-bounded在16任务波原20秒预算超时；失败保留，不标提交档全绿。同预算独立窗实际1.706842662秒PASS，身份STABLE，按cc授权登记serial域，冷启动不并行、不放宽20秒或删断言。串行约束原生重算后ucn-identities进入commit、cx-childdepth移daily（非名单特判）；全量覆盖不减，新计划资格待完整实测。旧78b4的82/82外墙209.492秒只属历史支持，不能替代本tip验收。

估时两表列完整清单、平台、材料与完成/预算来源；lnx/x86_64 daily累计16443.712749917秒，release/default16968.614173520秒。非PASS用原预算，材料构造与外平台等待未估；登记顺序估时不替代实际选择波，重叠scope/pool不可相加。

工具行数只作统计报告：TOTAL、逐文件、新文件、增长比例均无上限或批准门槛。`tests/toolsloc.cx --check` 保留报告兼容入口，拒绝写入或初始化基线；`tests/toolsloc.tsv` 保留全部历史行并标RETIRED。新工具仍须真实配套check与批次登记，非法.cx脚本及未登记别名仍拒绝；M8选择、估时、材料门与新增检查登记保持。

完整库存626源/79片，含18 symlink，与608普通文件指标分开。全部三补丁的有界拷贝、CLI选择查询均已合入；上层检查不再include工具源码。支持性行为原日志附各自基点，不冒充4102提交档全绿。WT原八断言保留在daily问题`M8-WT-NEG-CONSTRUCT`，不扩大缓存框架；T2.c十INFRA和T2.a材料另循真实证据交付，不回填历史s9资格。

既有遗留`tests/update-tools-compile.py`本轮只改十四列兼容：`batch(i,tier=None)`无tier仍输出原十三列、有tier才追加；仅当表头末列为tier才读取既有编译片档位；要求档位唯一且合法，混合档位真实拒绝；再生编译片沿用该既有档位，不隐式改档。原索引regular/symlink发现、每8源向上取整、独立候选输出及coverage字节复制流程均未改，具体diff随包列明。

## 0.0.48 T5.3：有限事实证明与未解析单调门

`tsv-lint` 默认对真实根启用未解析棘轮，`--max-unresolved N` 只能收紧；最终上限按父实测固化为11050，不能回调旧11281。fixture默认不继承整仓额度，显式传上限时同样受门约束。

| 输入及工具视图 | 未解析数 | 结果 |
|---|---:|---|
| 6a前一工具树原始实测 | 11281 | 原始PARTIAL，不冒称f66同树前后对照 |
| f66有限稀疏域证明 | 11272 | 原始PARTIAL，未知条件仍保留 |
| 最新产品输入、已兼容事实前导注释、尚无闭合section证明 | 11281 | 原11272门实际拒绝，失败保留，未抬上限 |
| 最新产品输入、完整两类证明 | 11050 | errors=0，仍为PARTIAL，不称全图已证明 |

有限域仅接受实际环境中的整数键集合，严格核EOF/越界/类型、多调用并集与未知局部遮蔽。闭合section排除仅证明manifest安装视图内某普通4/5列行不可能选中；动态call/stem/section（含@表达式）、缺失调用与通配选择保守保留。排除行不制造状态、引用或可能定义，已选行引用仅排除定义时仍报错。此证明不涵盖seed/gen.c额外手工装配、实际NN可达性或语言语义。

事实读取接受生成说明/来源注释/空行后真实schema，数据行宽仍严格拒绝，错误保留文件与行号。真实根盘点仍为560文件、28193行、5216状态。父行为检查54/0覆盖稀疏域、模式/引用、闭合选择、动态遮蔽及单调门；状态仍PARTIAL不是隐藏未解析项。

```sh
tools/bound 55 sh ./unisacc.com tools/tsv-lint.cx -- --root . --audit --limit 0
tools/bound 55 sh ./unisacc.com tools/check-tsv-lint.cx
```

收口仅交T5.1/T5.3；strict原型、生产图strict零告警、native20秒拆分与T2余项按 `agentwork/items.tsv` 排0.0.49，不注册生产常红批、不修改产品实现或退役LOC表。


## agentwork 分片读侧与受控迁移

`aw.cx` 默认根为当前目录，所有命令支持子命令之后的 `--root ROOT`。不生成 `out/aw/`，读命令不写文件；尚未迁移时拒绝，不回退旧表。

```sh
tools/bound 55 sh ./unisacc.com tools/aw.cx -- ledger --owner agent-a --since 2026-11-01 --root out/aw-fixture
tools/bound 55 sh ./unisacc.com tools/aw.cx -- items --state open --owner agent-a --root out/aw-fixture
tools/bound 55 sh ./unisacc.com tools/aw.cx -- check --root out/aw-fixture
tools/bound 55 sh ./unisacc.com tools/aw.cx -- migrate --root out/aw-fixture
```

账本规范分片带原六列表头，分片键为时间前七字节及第六列可安全文件名的角色/模型；不猜测作者。非规范历史行进入 `ledger/legacy.tsv`：严格双列表头 `aw-legacy-v1-hex<TAB>newline`，每行严格两列：原行字节的小写十六进制、换行存在位 `0|1`。存储及迁移等价保留原行内容和行尾位；排序/过滤视图在相邻记录之间始终输出LF，仅最后匹配记录保留原EOF位，避免无LF旧末行被排序到中间后粘连。正常六列分片仍严格校验日期、ID、owner及分片键。账本按整行字节排序（即时间、ID及其余字段），重复事件保留。看板每项严格为旧十一列表头加单行，文件名等于ID，按整行字节排序。筛选只使用能解释的实际字段，`since` 排除无法识别日期的历史行。

`check` 有旧表时分别逐行比较多重集（包含重复及行尾换行存在位）；缺任一旧表明确报告 `equivalence=unavailable`，仅剩结构检查，不作迁移验收声明。仅物理行起点的Git冲突标记触发非零拒绝，字段内普通标记文字可读。诊断仅输出相对文件及双方安全ID/state/owner，不输出goal/context/check、分支标签或URL查询；diff3基段不作为一方。pure模式包含实际stderr隔离及保留域查询脱漏回归。

`migrate` 先验证旧表与看板唯一ID，再在私有 `.aw-migrate-stage/` 写入、fsync并验证逐行等价，然后依次rename两个目录。**不是跨目录原子事务**。旧文件不变；收据保留于暂存目录。目标存在（包括空目录）或暂存现场存在均拒绝，不覆盖、不清理、不自动修复部分发布；读侧检测未完成现场并拒绝。没有每次写者更新的共享manifest。真实迁移只由cc静默窗口执行，本工具线仅使用自建夹具。

真实CLI检查分三项，各自55秒总预算（包含原生编译），不能将三项包装为同一55秒批：

```sh
tools/bound 55 sh ./unisacc.com tools/check-aw.cx -- pure
tools/bound 55 sh ./unisacc.com tools/check-aw.cx -- parallel
tools/bound 55 sh ./unisacc.com tools/check-aw.cx -- conflict
```

每模式创建唯一合成夹具，编译原生aw后真实执行；parallel另编译原生写者，两个独立main克隆的两个进程各追加20条自己的月/owner账本并改不同项，真实提交合并零冲突；conflict验证同ID冲突仅一个文件及双方状态。父最终实测pure 10.85秒、parallel 16.01秒、conflict 10.40秒，各原55秒不变，三者 `CHECK_AW failed=0`。源码仓旧表原字节及新目录存在性/身份均未变；无LF末行排序粘连、合法空owner误拒绝均已先复现再实测修复。旧items空owner及自由文本owner/state原样迁移，筛选允许空值与干净UTF8按字段相等。真实数据从未迁移；正式迁移、删旧及25处读写引用切换仅由cc静默窗口执行。

### 原生追加单条事件

```sh
tools/bound 55 sh ./unisacc.com tools/aw.cx -- append --owner agent-a --id ITEM-1 --event verified --evidence fixture --root out/aw-fixture
tools/bound 55 sh ./unisacc.com tools/check-aw.cx -- append
tools/bound 55 sh ./unisacc.com tools/check-aw.cx -- append-concurrent
```

`append` 只接受已迁移布局，默认真实UTC日期及commit `-`，可显式指定 `--date YYYY-MM-DD`、`--commit COMMIT`。校验六列、日期与分片键后写入 `ledger/YYYY-MM/OWNER.tsv`；完整六列事件相同为 `ALREADY_PRESENT rows_added=0`，同ID的不同事件仍追加。输出仅状态及相对分片路径，不回显证据。

同owner使用稳定 `.aw-append-OWNER.lock` 和最多10秒的原生flock等待；同owner串行检查重复并发布，不同owner独立。独占 `.aw-append-OWNER.tmp` 经完整写入、fsync、关闭、rename发布；未知已有暂存拒绝且保留，写失败保留原目标。锁与暂存仅窄范围忽略，不忽略业务分片。不写items、旧表或其他owner；不承诺跨文件事务。

父实测append 12.99秒、append-concurrent 13.15秒，均failed=0；pure回归6.76秒failed=0。原并发检查单次读取即关闭输出管道存在SIGPIPE风险，已改为读至EOF并保存逐轮逐子完整输出/退出状态；原失败现场保留，未观测原signal，不将风险推断写成既证根因。

## TSV 三方合并与补丁集成

泛型库由 `use tsvx.merge3` 引入，不提供独立 merge CLI。API 为：

```c
int tsm_merge(const char *header, int columns, int keymode,
              const char *base, size_t bn, const char *ours, size_t on,
              const char *theirs, size_t tn, tsm_buf *out, tsm_buf *conflicts);
int tsm_merge_schema(const char *header, int columns, int keymode,
                     const char *base, size_t bn, const char *ours, size_t on,
                     const char *theirs, size_t tn, tsm_buf *out,
                     tsm_buf *conflicts, int inherited_min_columns);
```

`keymode=0` 按整行键集合并（相同行幂等），`keymode=1` 按首列键合并并检查重复键；表头和列数按声明检查。返回 `0` 为干净合并，`1` 为冲突，`2` 为输入/schema/key或资源错误。冲突材料使用可读 `base/ours/theirs` 内容，不以十六进制代替；失败不向 `out` 发布部分结果。`tsm_buf` 含 `char *p; size_t n,cap; int failed;`，调用方零初始化并最终 `free(p)`。`tsm_merge` 保持严格默认；`tsm_merge_schema` 的 `inherited_min_columns=0` 同样严格，正值仅允许base行宽不少于该值、且侧异常行完整等于同key的base行。领域历史兼容政策留在 `cxlib.appendmerge` 上层：ledger选择5，其余选择0，不作为泛型默认放宽。

三个真实原生检查各自独立55秒（包括编译），登记批次分别为 `tsvx-merge3`（commit）、`appendmerge`（daily）、`integrate-patch`（daily），不将三个检查塞入同一55秒窗：

```sh
tools/bound 55 sh ./unisacc.com tools/check-tsvx-merge3.cx
tools/bound 55 sh ./unisacc.com tools/check-appendmerge.cx
tools/bound 55 sh ./unisacc.com tools/check-integrate-patch.cx
```

成功标记分别为 `TSVX_MERGE3_CHECK`、`APPENDMERGE_CHECK`、`INTEGRATE_PATCH_CHECK`，均带 `checks=N`、`failed=0`。integrator检查无参会编译真实工具，也支持 `--native ABS_INTEGRATE ROOT` 绑定已有原生工具。集成CLI为：

```sh
sh ./unisacc.com tools/integrate-patch.cx -- ROOT PATCH ~/unisacc-integration [--base REV] [--prepare-only]
sh ./unisacc.com tools/integrate-patch.cx -- --resume ~/unisacc-integration
```

`--stop-after N`（N为1至5，也可 `--resume CLONE --stop-after N`）为门前快检：跑完第N个gate阶段即打印 `INTEGRATE_PREFLIGHT status=PREFLIGHT_OK` 并退出，不写 ready.tsv，不报READY；N越界为 `stop-after-range`；不带该参数时行为与120秒/55秒限额不变。

`--prepare-only` 只产生 `PREPARED`、`qualification=NOT_QUALIFIED`，不是PASS。只有真实gate完成才报告 `READY_TO_COMMIT`、`qualification=GATES_ONLY`，不是全产品或全批资格；不自动commit/push。每次调用原120秒总限额、子进程最多55秒保持不变，预算未完阶段可报告PENDING，不冒称成功。

父已观测的**首轮历史版本**保留：泛型17/0、3.36秒；领域28/0、10.09秒；integrator真实夹具17/0、47.16秒。后续函数指针schema导出曾被当前编译器拒绝，已改为纯值schema参数；去除平方级路径及不必要hex拷贝并加入checked-copy后的**最终版本**由父真实验收：泛型17/0、3.33秒；领域33/0、10.36秒；integrator真实夹具17/0、43.06秒，均在各自55秒预算内。这些仅证明对应工具/夹具检查，不授全产品或全批资格。

## 0.0.49 S9：同 RUN 原生分片与工作树差分

这是普通 main 候选的工具线，不修改产品源码或 SID。当前 `--tier commit` 仅为 Linux/x64（`lnx/x86_64`）的已实现提交档，输出 `COMMIT_ONLY`、`qualification=NOT_FULL`；其他平台明确 NOT_APPLICABLE，未实现项明确 NOT_AVAIL，不作为通过。省略选择参数才是完整登记范围；`--files`/`--since` 是回归选择，不能冒充完整资格。

```sh
tools/bound 55 sh ./unisacc.com tools/s9-shards.cx -o out/s9-shards
out/s9-shards init ROOT RUN - --tier commit
out/s9-shards prepare ROOT RUN checker
out/s9-shards prepare ROOT RUN executor-all-waves
out/s9-shards prepare ROOT RUN executor-run-batch
out/s9-shards prepare ROOT RUN executor
out/s9-shards prepare ROOT RUN driver
out/s9-shards prepare ROOT RUN selection
RUN/driver step ROOT RUN
RUN/driver aggregate ROOT RUN
```

`ROOT`、`RUN` 是参数占位，须在运行时替换为规范化绝对路径；`.` 会被 canonical-root 闸拒绝，不能照旧示例直接传入。

`init ROOT NEW_RUN MAP|-` 要求新目录；需材料的映射批次另逐项执行 `out/s9-shards prepare ROOT RUN material NAME`。checker及 native executor在同 RUN 按当前身份构造、认证并封存，不能复制其他 RUN 的工具、材料收据或 PASS 结果充数。init/prepare由原生监督器分相位；step/aggregate使用 RUN 内 driver。原 batch 55 秒及 native 整 step 300 秒不变，不能用扩大的 `tools/bound` 参数代替监督器；bound接口仅接受1..60秒。PENDING返回3，不是PASS；中断未封存保持UNKNOWN，不静默重跑。

并发上限15 slots，三档共享总量；writerexclusive须跨commit/daily/release排空并独占，不能只在本档互斥。source/tree/SID、输入和材料在执行前后重新观察并核对，不缓存内容摘要。工作树账本例外仅为 `agentwork/ledger.tsv`、`agentwork/ledger/` 及精确 namespace 规则；不是忽略整个 `agentwork/`，items、注册表、选择表与其他测试输入仍受闸约束。

调度维持3个slice、每slice最多5个batch。首个被活动nonwriter阻挡的writer获得固定预约；仅按预约时空槽数一次回填最多2个独立nonwriter，不补充耗尽的quota，不跨过各档下一writer。quota耗尽后强制drain，活动清零时预约writer优先；无预约时按各档忙槽数择最少者，同值轮转。原75秒准入余量、batch预算及整step监督限额不变。真实运行时间线与逻辑planner夹具分开取证；writer独占期间保留的空槽不是可消除的HOL浪费。

### wt_diff 的平方级证据与保序修复

原实现对每行反复全表查找，造成平方级差分。真实1101803字节相同快照的同单元实测：原 `wt_diff` 51.301455538秒，修后5.962208毫秒；前后全内容扫描仍各约6.5秒，未靠缓存摘要规避扫描。修法借用原快照字节，按路径/整行排序索引、分组集合比较和路径去重，再按原序号排序输出，保持旧算法“after再before”的首遇顺序及已有输出路径去重；相同字节可直接返回。真实规模仅一个摘要改变的非快路328.659817毫秒、moved=1，不能只用相同快照快路作证。

`out/wtdiff-evidence/original-profile.tsv`、`fixed-profile.tsv`、`changed-profile.tsv` 保存同单元分相位证据。另一组真实pair在 `profile-before.wt`、`profile-after.wt`，完整结果见 `real-snapshot-equivalence.tsv`：旧oracle diff为65.084956897秒，新 `native_process` 为16.699770毫秒（含文件读取及启动），完整输出双方SHA256均为 `b8b0ff1c9e48a6fa1bd7b27dbb44fcab06e261f1bff6bbc5fbc2f0ea295cc6c4`。该pair的进程时间与前述同单元diff计时口径不同，不混为一个倍率。旧oracle仅留在check作独立对照，不进生产执行路径。

父已实测inputs的11个对抗及17个动态场景，新旧输出全部逐字节一致；11项完整对照在 `out/wtdiff-evidence/adversarial-results.tsv`。首轮误写改名输出顺序断言的原失败保存在 `inputs-first-failure.tsv`，已修的是断言；当时旧oracle输出仍一致，不抹去失败或改称实现回归。

正式完整89项功能收据为 **89 PASS、0 bad、0 pending**，来自同RUN两次step：255.77秒 + 165.62秒 = **421.39秒**；这不是单step≤300秒完成，SPEED-C、EFF1仍为 **DOING**。旧两个RUN的255.79秒全未决、295.09秒仅7有效/16pending均保留，不借历史RUN补绿。上述差分证据不等于整step或发布资格。H候选2仍只是静态方案，cc同意等F后再冻结。

## 0.0.49：六片 fixedpoint 逐相位恢复

`chain-step.cx fixedpoint` 每次只执行一个目标的一个对象、链接、切片接纳、kernel阶段或重新比较。分相位工具限定Linux宿主；交叉构造六目标不等于六宿主原生CI。以下是**单条原生命令模板和参数表**，不是shell算法流水脚本；逐项调度实际使用 `.cx` 工具（`whole-front.cx`、`chain-step.cx`），不依赖忽略的 `out/` 协调器作为唯一实现。

`REV` 为40位小写revision；`S` 为当前工作根下 `out/release-snap-${REV前10位}` 的规范绝对路径，含匹配 `.release-rev`；`PKG` 为同一冻结包；`GEN1` 为第一代Linux/x86_64 candidate；`FP` 为新私有绝对目录；`HELPER` 为实际F-v2源身份原生helper。`PRIVATE_ROOT` 为本私有补丁树，`CHAIN`、`WF` 为从其绝对源路径实编的原生映像。所有路径占位必须先绑定真实材料，不用快照旧工具或default-run临时映像替代。

```sh
tools/bound 45 sh ./unisacc.com -O2 "$PRIVATE_ROOT/tools/chain-step.cx" -o "$CHAIN"
tools/bound 45 sh ./unisacc.com -O2 "$PRIVATE_ROOT/tools/whole-front.cx" -o "$WF"
tools/bound 45 "$CHAIN" fixedpoint "$REV" "$S" "$PKG" "$GEN1" "$FP" --identity-helper "$HELPER" --phase "$G" --target "$T" --object "$KEY" --resume
tools/bound 45 "$CHAIN" fixedpoint "$REV" "$S" "$PKG" "$GEN1" "$FP" --identity-helper "$HELPER" --phase "$G" --target "$T" --link-only --resume
```

Linux两代对象/链接入口保持：依次 `G=g2,g3`，各代先 `T=lnx/x86_64` 后 `lnx/arm64`，逐个完成下列44个 `KEY` 后才 `--link-only`。不是用一次整段watchdog包住全部工作。

```text
drive.o host.o stages.o routestep.o stagehandoff.o runexec.o attributeframe.o bind.o cli.o compat.o route.o deps.o input.o args.o path.o entrybind.o memlayout.o driveface.o driveroute.o cxmod.o tapegate.o fwdstub.o executor.o transition.o codec.o modeldecode.o modelnumber.o packmodel.o modelload.o bindstage.o entryexec.o hostnative.o tapelink.o drivelink.o drivetapebin.o driveargs.o driveplan.o driveutil.o driveunits.o driverun.o hostout.o hostres.o rescache.o drivestream.o
```

### 四Mac/Win本版生产入口：front分片与通用span

每代Linux两片完成后，四片 `T=osx/x86_64,osx/arm64,win/x86_64,win/arm64` **必须**用以下入口；原来直接无对象参数的四个whole-o整段循环不是本版生产入口。每次原生调用独立45秒**总预算**，包括其身份观察、哈希、子进程与收据验证，不为子步骤重置预算。

|参数|绑定与顺序|
|---|---|
|`G`、`CTOR`、`OBJECTS`|g2用 `FP/g2/lnx-x86_64/candidate`、`FP/g2/lnx-x86_64`；g3用对应 `FP/g3/...`。四片使用**当代**Linux/x86_64 product与对象收据，不用GEN1或上一代对象|
|`D`|每代每目标独立新目录；Mac两ISA各有独立 `D/front-prepared`、`D/span-prepared` 及它们自己的 `tmp`，不得共用或覆写真实FWD桩|
|`F`、`B`|分别为 `D/front-prepared`、`D/span-prepared`；front模板与通用span模板分别prepare，均绑定同一当代CTOR/OBJECTS|
|`ROOT_1 ... ROOT_44`|逐个独立argv，严格取 `F/roots.txt` 与 `B/roots.txt` 的相同44行顺序；保持逻辑路径原文（如src/runtime别名），不能排序、realpath改写或合成单字符串|
|front0..2|FIRST/COUNT依次 `0/32`、`32/32`、`64/21`，共85输入；merge严格按front0、front1、front2顺序|

以下每一行都是独立调用模板；`UNISACC_IDENTITY_HELPER` 显式绑定实际helper。prepare会创建并绑定自身 `TMPDIR=PREPARED/tmp`，后续原生调用重验路径/dev/ino及字节。

```sh
UNISACC_IDENTITY_HELPER="$HELPER" tools/bound 45 "$WF" prepare "$S" "$PKG" "$CTOR" "$OBJECTS" "$F" --template "$PRIVATE_ROOT/tools/templates/whole-front.cx.in"
UNISACC_IDENTITY_HELPER="$HELPER" tools/bound 45 "$WF" prepare "$S" "$PKG" "$CTOR" "$OBJECTS" "$B" --template "$PRIVATE_ROOT/tools/templates/whole-win-span.cx.in"
tools/bound 45 "$WF" front "$F" "$T" "$D/front0" 0 32
tools/bound 45 "$WF" front "$F" "$T" "$D/front1" 32 32
tools/bound 45 "$WF" front "$F" "$T" "$D/front2" 64 21
tools/bound 45 "$WF" merge "$F" "$T" "$D/A.phase" "$D/front0" "$D/front1" "$D/front2"
```

Mac两片先执行真实stub生成。`STUB` 必须是 `D/stub/stub-path.txt` 记录的 **F/tmp/unisacc-forward-entry.c** 绝对路径，其真实TMPDIR文件字节须与 `D/stub/stub.c` 相同；不能用stub.c副本路径、空桩或另一ISA的桩冒充。两个span都传同一真实路径及重核SHA。

```sh
tools/bound 45 "$WF" stub "$F" "$T" "$D/A.phase" "$D/stub"
tools/bound 45 "$WF" span "$B" "$T" "$D/A.phase" "$D/units-e3" units e3 ROOT_1 ... ROOT_44 --forward-stub "$STUB"
tools/bound 45 "$WF" span "$B" "$T" "$D/units-e3" "$D/prune-elf" prune elf ROOT_1 ... ROOT_44 --forward-stub "$STUB"
```

Win两片无forward-stub，必须三相，而非把units..e3塞进一次调用：

```sh
tools/bound 45 "$WF" span "$B" "$T" "$D/A.phase" "$D/units" units units ROOT_1 ... ROOT_44
tools/bound 45 "$WF" span "$B" "$T" "$D/units" "$D/e3" e3 e3 ROOT_1 ... ROOT_44
tools/bound 45 "$WF" span "$B" "$T" "$D/e3" "$D/prune-elf" prune elf ROOT_1 ... ROOT_44
```

`ROOT_1 ... ROOT_44` 是参数表占位，不是可直接执行的字面参数。恢复已存在材料时在对应front/merge/stub/span原命令末尾加 `--resume`，由工具重新验完整上下文；不能删除收据绕过拒绝。Mac切片不使用旧 `UNISACC_PHASE=B` 双相整段路径或image-build路径。

### 有序接纳、旧producer与kernel结果

接纳桥每次只对一个索引发起新的真实原生 `span --resume` 验证，不伪造constructor历史。Mac命令重复完整同序span列表，依次 `I=1,2`；Win重复完整同序列表，依次 `I=1,2,3`。不得省略前项、换序或并发同片索引；失败尝试保留，不能覆写历史。

```sh
tools/bound 45 "$CHAIN" fixedpoint "$REV" "$S" "$PKG" "$GEN1" "$FP" --identity-helper "$HELPER" --phase "$G" --target "$T" --span-driver "$WF" --span-prepared "$B" --span "$D/units-e3" --span "$D/prune-elf" --span-index "$I"
tools/bound 45 "$CHAIN" fixedpoint "$REV" "$S" "$PKG" "$GEN1" "$FP" --identity-helper "$HELPER" --phase "$G" --target "$T" --span-driver "$WF" --span-prepared "$B" --span "$D/units" --span "$D/e3" --span "$D/prune-elf" --span-index "$I"
tools/bound 45 "$CHAIN" fixedpoint "$REV" "$S" "$PKG" "$GEN1" "$FP" --identity-helper "$HELPER" --phase "kernels-$G" --kernel-result "$REAL_KERNEL_ROUTE"
tools/bound 45 "$CHAIN" fixedpoint "$REV" "$S" "$PKG" "$GEN1" "$FP" --identity-helper "$HELPER" --phase compare
sh ./unisacc.com tests/chain-fixed-point.cx -- "$GEN1" "$FP"
sh ./unisacc.com tests/chain-fixed-point.cx -- --self-test
```

若依赖收据由旧chain映像产生，在上述相关fixedpoint调用显式追加 `--receipt-producer "$OLD_CHAIN_NATIVE"`：必须提供仍存在的真实旧原生映像，现场重核其实际SHA并与收据producer匹配；不接受手写SHA、源文件SHA或把新映像重命名为旧producer。新收据始终绑定当前 `/proc/self/exe`，该参数仅允许验证匹配旧producer的依赖，不改写历史。

`REAL_KERNEL_ROUTE` 是相应代真实已完成kernel-route旧输出目录，不是占位收据或新造空树。`--kernel-result` 接纳重新核对其constructor/package、完整34 inputs、22 sources、22 objects、2 kernels及复制前后树字节；记录的是验证接纳，不能宣称当前调用重新构造。没有旧真实输出则使用原 `--phase kernels-g2|kernels-g3 --resume` 构造入口，不能用接纳跳过构造。每代先完成Linux两片、四切片接纳及本代kernels再进入下一代；最终compare新验完整依赖，仍不等于独立第二次构造或六平台原生CI。

### FWD中等夹具与已废弃证据

`tests/fixtures/whole-front-medium/main_fwd.cx` 来自已验证真实FWD根源码，复用同目录records/hash/sort/graph/report五模块，保留256记录、双向稳定排序、排列/袋哈希及独立图算法断言；通过 `sys.dlcall` 的实际 `sy_dlopen(NULL,2)` / `sy_dlclose` 触发dock，不造extern假接口、桩或缩规模。原main.cx及五模块不变。

决定性历史证据是 `out/c49-slow-cdx-979b56e4/mac-medium-forward-proof/sys-dlcall/`：正常whole未设置PHASE，与真实A捕获后units..e3/prune..elf输出cmp rc=0；真实桩1693B，图像115810B，SHA256 `525fb0c9372af2588aa8bebc1a74b3a8a2d71fc7637a404b49b9e6cab603529b`。这是osx/x86_64对应夹具证据，不外推其他ISA或原生CI。旧envB `st_nunit1` 证据标为 **SUPERSEDED / 0.0.50Y已知问题**，保留失败记录但不作为本版Mac切片入口或成功证据。

父代理最窄FWD fixture实编入口（本段不宣称已运行；CTOR绑定真实product，输出用新绝对路径）：

```sh
tools/bound 45 "$CTOR" --target=osx/x86_64 "$PRIVATE_ROOT/tests/fixtures/whole-front-medium/main_fwd.cx" -o "$FWD_IMAGE"
```

上述对象键对应当前 `entry-build.cx` 的44个显式入口对象；源闭包对象仍由实际entry-build规则处理，不用伪造空对象。目标参数使用 `OS/ARCH`，目录使用连字符。每代六片的真实路径分别是 `lnx-x86_64/candidate`、`lnx-arm64/candidate`、`osx-x86_64/slice-osx-x86_64`、`osx-arm64/slice-osx-arm64`、`win-x86_64/slice-win-x86_64`、`win-arm64/slice-win-arm64`，均在 `FP/g2` 或 `FP/g3` 下。对应收据是代目录下 `phase-0.tsv` 至 `phase-5.tsv`；`phase-6.tsv` 绑定 `kernel-route`。不能读取旧的 `g2/candidate` 或 `g2/receipt.txt` 代替它们。

`--resume` 可用于第一次fixedpoint调用；已有阶段必须重新验证源身份、包、constructor、工具/helper及产物树字节和wall证据，且只跳过已记录的对应步骤，不缓存PASS。`chain_sha` 绑定 `/proc/self/exe` 的实际执行映像SHA，不是可能来自另一棵树的 `__FILE__` 源字节；不要default-run改变映像后期待旧收据可resume。无收据的已有产物、身份改变或已链接片再增改对象均拒绝；恢复时保持全部输入冻结，不删除收据来绕过拒绝。Linux两片由上一代product构造，四切片由本代Linux/x86_64 product构造，保持上述代际/目标顺序。`compare` 每次新验完整六片与kernel收据，再实际比较objects/kernels/slices/product/package。

`tests/chain-fixed-point.cx` 保留F1–F4代际、包与根断言，并对两代所有六片检查存在性、字节相等和各片constructor/包/根绑定。其自测保留原有异字节、断代和异包负控，另对两代每片逐一执行缺失与异字节拒绝（含Windows/arm64）。这些命令是验证入口，不是本文宣称已通过的运行收据；同一运行的固定点也不代表独立第二次构造。

### wholefrontmerge：独立模块检查

| 能力 | 入口 | 边界 |
|---|---|---|
| 多属性记录保留全局单位编号、输入记录顺序与opaque字节；同单位不同属性合法；非相邻相同记录具名拒绝且无输出；零单位的空attrs与四字节零计数合并为精确12字节phase | `tools/check-wholefrontmerge.cx` / `CHECK_WHOLEFRONTMERGE` | 直接调用 `wf_merge_parts(dirs,count,new_output,why,cap)`，三个确定性文件夹具检查；不替代 `check-whole-front.cx` 原九项，不代表产品阶段等价、resume或候选资格 |

新入口basename对应 `tools/cxlib/wholefrontmerge.cx`。当前已在最终已装2d3852候选实际3项/0失败；注册器实际BATCH PASS并REGISTERED（其隔离根产品559仅为工具启动材料，不转授候选资格）。没有增加baseline。以下为可复用登记命令；会登记后实际运行batch，不缓存PASS，生成wholefrontmerge和interfaces.wholefrontmerge两行。

```sh
sh ./unisacc.com tools/register.cx -- wholefrontmerge tools/check-wholefrontmerge.cx CHECK_WHOLEFRONTMERGE "多属性全局单位编号、顺序与opaque字节；非相邻重复具名拒绝无输出；零单位双空属性编码精确phase；模块检查，非产品资格" "tools/check-wholefrontmerge.cx + tools/cxlib/wholefrontmerge.cx" interfaces product
```

精确登记行（由上述工具生成，不手工追加）：

```tsv
wholefrontmerge	IMPLEMENTED	interfaces	tools/check-wholefrontmerge.cx	SOURCE	lnx/x86_64	55	-	tools/check-wholefrontmerge.cx	-	interfaces.wholefrontmerge	rc0:CHECK_WHOLEFRONTMERGE	product	commit
interfaces.wholefrontmerge	interfaces	IMPLEMENTED_SEPARATE_BATCH	多属性全局单位编号、顺序与opaque字节；非相邻重复具名拒绝无输出；零单位双空属性编码精确phase；模块检查，非产品资格	tools/check-wholefrontmerge.cx + tools/cxlib/wholefrontmerge.cx
```


## 0.0.57：chain-run 有界全链编排

`sh ./unisacc.com tools/chain-run.cx -- --rev HEX40 --out ABS -- --cache-manager-osx X64 ARM64`
按 `build/chain-phases.tsv` 的实际顺序驱动 `candidate-chain --resume --until PHASE`。
G2兼容逐键obj-N是外部显式until边界，正常编排跳过，保留实际closure-prepare/closure/objects/link批量相。
每个子调用至多55秒，总deadline默认5400秒，最多3次异常续跑、4096次相位尝试；
`--deadline-s`、`--max-resumes`、`--max-attempts` 可显式给定，预算写入OUT，重新启动不会刷新。
这是一条带总预算的编排命令，不是把单个子命令时限放宽；外层调用者须允许编排总预算，不能误用60秒外界认证整链。
只有超时124、被杀137/143或实际SIGKILL/SIGTERM可自动续跑；其余首败立即写 `BLOCKER` 到
`OUT/chain-run.tsv` 并返回原失败码，不自动绕行。deadline或次数耗尽返回3。
每次尝试保留argv/stdout/stderr，续跑单独记行；`chain-watch` 只观察现场，活心跳owner拒绝接管，
不以其单独的DONE认定成功。身份文件绑定冻结根、revision、相表与参数；不接纳未绑定的既有pack；
完成还要核 `ape-final/ape-receipt.txt` 对 `unisacc-next.com` 的SHA与字节数，只记PREPARED/not=PASS。
非pack检查点可沿既有candidate-chain合同复用；不要跨冻结根或revision搬检查点，复用不迁移资格结论。
`--candidate ABS_EXEC`、`--watch ABS_EXEC` 可指定同argv协议的已编译原生工具（夹具也使用这个接口）；
默认先在独立55秒界内把本根watch/candidate编译为本次原生工具，再驱动相位，避免每相支付candidate冷编。
`--` 后仅传candidate普通选项，不允许覆盖rev/out/until/plan/jobs；准备步骤同样计入总deadline与尝试上限。
正规build/ctx等相的真实55秒余量未经整链测量；超限只可在上述总次数内续跑或耗尽BLOCKER，不放宽、不无限推进。

预演显式使用已有材料：加 `--rehearse --rehearse-ctor ABS --rehearse-objects ABS --rehearse-pkg ABS`。
只跑snap/build/ctx，再用上述材料跑whole-front prepare及osx/x86_64 front0一片；缺材料具名拒绝，
绝不为预演暗跑jobs/p3/gen1。预演输出与全链OUT必须分开，且只记REHEARSED/not=FULL/not=PASS。
prepare/front0每次尝试使用独占新目录，超时残留保留以便诊断，不清理或复用半成品。
耗时回写K8另属owner，本工具不生成phase-times。

轻检查：`sh ./unisacc.com tools/check-chain-run.cx`，仅含兼容边界的微型假链，不跑真实构造；
保留 `out/check-chain-run-*` 的证据。真实整链、真实预演、端到端速度和产品资格仍UNMEASURED。

## 0.0.57：release-oneshot 执行边界

规划接口 `--dry-run CANDIDATE LAYOUT NEW_STATE`、`--resume STATE` 和
`--stop-after PHASE` 仍仅产出计划；`PLANNED` 与 `MANUAL=0` 都不代表发布资格。
执行必须独立显式授权：
`release-oneshot --execute STATE CONFIG --authorize-external-operations`。
CONFIG 是逐行 TSV，命令前缀的每个参数各占一列，不作 shell 拼接或求值。
执行可能提交和推送 X/Y、触发 CI、运行 s9、上传并公开发布；不得在未经授权的根或
公共仓库上用它做测试。BUILD_REV、X、Y 与 CI run id 分开保留；输入变化拒绝续跑。
执行 checkpoint 区分 `PLANNED/RUNNING/DONE/FAILED`；命令原始结果是证据来源，
失败、deadline、未跑与 UNKNOWN 不得当作 COMPLETE。上游冻结、构造、打包资格
不得由调用者随意填写 PASS，假命令成功也不授予真实产品或发布资格。
执行 CONFIG 必填单值键 `root repo_dir build_rev prev_tag notes deadline_s`；`gh s9 decide`
为多列 ARGV 前缀，`layout_check` 可提供预编译 release-layout 前缀。
`notes` 是 repo_dir 内的说明路径；`build_rev` 必须与候选构造提交一致，不能以X/Y代替。
可选 `s9_tiers` 为 commit/release/daily 逗号表，`s9_call_s` 不超过55秒；`poll_ms` 控制观察间隔。
`deadline.tsv` 是一行 `START_MS<TAB>END_MS`（epoch毫秒），与配置跨度机检绑定。
CI只承认候选工作流 `candidate-native-evidence` 制品内 `table.md` 的规范QUALIFIED汇总、
匹配X和APE摘要；由 `gh run download` 获取，不把日志子串或自定义合格文本当资格。
s9三档命令完成还不等于FULL，须核实际候选消费者及daily完整汇总；未证明保持UNKNOWN。
可选 `s9_gap_ruling` 指向当版绝对路径裁定文件，必须逐行绑定 `release_tag`、`build_rev`、
`ape_sha256`、`decision`=`ALLOW_S9_GAP` 与 `reason`，原文保存并在恢复时逐字核对；
它仅允许明确披露的UNKNOWN缺口，不把UNKNOWN改PASS，也不豁免真实FAIL。
`tools/check-releaseexec.cx` 仅用隔离本地 git 远端及假 gh/s9；无网络调用。
批次需要调用者提供预编译 `release-oneshot-tool`、`fake-gh-tool`、`fake-s9-tool`、
`release-decide-tool`、`release-layout-tool`，避免把多个工具的冷编译计入 55 秒夹具界。

0.0.58 资产门禁在每次实际 CI dispatch 前重新查询 REST release id 和
`releases/ID/assets?per_page=100&page=1`，保留 HTTP 头及名称、大小、digest、state。
任何 Link 分页头、截断行、重复、额外、缺失或非 uploaded 项均拒绝触发；
远端大小和 sha256 必须逐项匹配本地候选，不能用 upload 返回成功或历史 checkpoint 替代。
应上传集合沿 candidate.tsv 七角色及 standalone-sha256.tsv 六目标生成，六片另与 APE
逐字节核对；无新增工具名或编译库存项，现有 release-exec 批次扩展缺 win-arm64、
错候选、错内容、错大小、重复及分页假 GH 场景，断言 workflow run trace 为零。
已有 CI 身份、RUNNING/FAILED dispatch 均不盲重触发。真实网络及产品资格不由夹具证明。
新增 `release-assetguard` 批次使用同一检查的 `--assetguard` 模式，55秒界不变；
原 `release-exec` 默认八场景及原断言保留。资产专批另覆盖缺digest、API失败、
分页截断、draft-check已DONE恢复后重查及十四资产（七角色+六片+manifest）正负控。
六片名称由releaseoneshotpack唯一权威表提供，prepare/upload/public与门禁复用。


工具编译库存以实时 Git 索引为准，每片至多八源。本增量纳入四个 tools cx 后为
767 源、96 片，范围 `tools-compile-000..095`；`096` 将是空片，不能登记。
清单已补 `tools/check-whole-front-expand.cx`，新增假CLI在 tests/cases，不计入tools库存。

`release-layout` 构造/核对 ZIP 的临时目录遵循现有规范绝对 `TMPDIR`；未设置时
在调用者布局目录下创建独占子目录，不再使用共享临时根。执行夹具全部保留在私有树。

## 0.0.57：发布前置准备 G1..G6

`release-oneshot --prepare STATE CONFIG ASSETS --authorize-external-operations`
只在调用者授权的私有冻结克隆中操作。ASSETS 必须为 `STATE/rr/layout`。
配置沿用执行配置，并提供单值 TSV 键 `root build_rev constructor identity_helper
chain_out snap version repo tag workflow workflow_ref cache_manager_x64
cache_manager_arm64 deadline_s chain_deadline_s max_resumes`。路径均使用绝对路径；
`snap` 必须为 `ROOT/out/release-snap-BUILD_REV前10位`。构造器及身份助手需已准备，
两份 cache-manager 明确记录为外借输入，不声称链内重造。不会联网或赋予发布资格。

准备器冻结 HEAD、源 SID、构造器与配置摘要；驱动有总预算的 chain-run；按现有
candidate-chain pack 的固定路径收集六片/P4/两份 manager，保留原路径与逐文件摘要；
生成 ci_run_id=0 的初表并写入私有 ROOT/release/candidate.tsv，再运行本地 pack/resume；
最后导出、检查六独立片并生成 `standalone-sha256.tsv`。`deadline.tsv` 在恢复时不刷新。
正式执行入口先用 `sh ./unisacc.com tools/release-oneshot.cx -o ABS_ONESHOT` 预编译；
该编译仅准备工具，不是发布相完成或产品资格。原生执行启动时建立共享持久总deadline，
覆盖其后的 prepare、execute、子工具冷编与 resume，恢复不得刷新截止。
直接 `sh ./unisacc.com tools/release-oneshot.cx -- ...` 的入口冷编发生在模块计时之前，
不能声称已计入；这种动态入口必须由调用者另包不超过60秒外界，或使用上述原生入口。
子命令结果保留 `prepare-*.log`；返回3表示预算/中断未完成，非PASS。测试可配置
`test_driver`：调用 `DRIVER chain|pack|standalone STATE CONFIG ASSETS`，替换昂贵命令，
但身份、固定清单及七资产摘要仍检查，且上游证据只能 UNKNOWN。

0.0.58 可选键 `chain_adopt`（值只能是 `external-chain-run`）：链已由 `tools/chain-run.cx`
在准备器之外分段跑到 pack 时使用，准备器不再启动 chain-run（其单段超过准备器 55 秒子界）。
采纳前机检 `chain_out/chain-run.identity` 含同一 `rev=BUILD_REV`、`root=ROOT`、`mode=full`、
`candidate=source`，`checkpoint.tsv` 含 `DONE compare` 且末行为 `DONE pack`；`chain-run.tsv`
的 BLOCKER 行数写入 `STATE/chain-adopt.tsv` 并打印，供发布说明披露，不隐藏、不改判。
四个链文件仍进 `prepare.seal`。采纳不重跑 compare，链内人工介入须另行披露。
初表七角色名与历版候选表一致：record 为 `unisacc.com.build.json`、别名 `build.json`、
zip 为 `unisacc-VERSION-win-arm64.zip`。

`upstream-evidence.tsv` 的1..3只来自真实完成的准备步骤；执行者必须调用
`rp_verify_upstream` 重核配置、原输入、收集材料、七资产和六独立片，不能接受任意配置PASS。
真实准备只证明freeze/construct/pack，不迁移CI或s9资格。本次已轻量实编并运行隔离执行夹具、
前置失败/身份夹具及原规划回归，均failed=0；未跑真实构造、全门、发布、CI或s9。
证据保留于私有 `out/k7-compile.log`、`out/k7-exec-fixture-02.log`、
`out/k7-prepare-fixture-01.log`、`out/k7-plan-fixture-01.log`，假结果不授发布资格。

`release-run --resume OUT` 读取 `pack-inputs.tsv` 并重算初表与十项源材料摘要，
跳过已完成相，再核七资产与校验和。初次布局使用现场四源摘要而非初表零摘要；
本地pack不再要求尚未产生的X提交，CI命令只打印 `{commit-x}`，不会执行。
`release-layout SOURCE NEW_OUT` 在验证四源后现算新生成alias/ZIP/checksums的摘要再核布局；
`--check OUT` 仍要求输入七摘要严格匹配，生成模式不再因尚未存在的派生摘要而拒绝。

## 0.0.57：FACTS-COUNT 构造事实与静态调用合同

本增量以 `23a873aaa1` 顺序叠加 gen1split、t57-1-r、ib55-1-r、wfcheck、
g2g3split、chainrun、k7exec 为基线。根清单、Linux根对象清单与动态闭包是不同域：
`unitcache` 等展开单元由现有 `closure-list` 路径纳入，不能把整程序单元总数当根数。
whole-front 的 `WF_MAX=128` 保留资源合同；32单元片预算与45秒子调用界不放宽。
分片总数必须读实际 `part.tsv total`，保持已有front检查点名，并为新增片保留独立相界。

静态调用核对只证明其公开支持域内的源码参数合同，不代替真实构造或运行资格；
未知表达式、未解析的调用或解析器必须拒绝，不得把未覆盖路径记为通过。
负控必须修改独立调用点/解析器输入，覆盖缺参、错序及未知构造，不能仅检查同源生成文本。
本工作不修改产品源、原有断言或时限，不运行真实构造链、发布、CI及s9。

新增工具命名先用 `git log --all --format= --name-only -- tools` 完整历史核查；
本增量工具名为 `chain-facts-gen`、`check-chain-facts`、`chain-argv-check`、
`check-chain-argv-check`、`check-chain-partition` 与 `cxlib.chainfacts`。
三个检查批次分别为 `chain-facts`、`chain-partition`、`chain-argv-check`；
前两者通过批次材料绑定预编译工具，调用检查在单次轻编译后进程内复用真实分析器。

运行时front子相库存位于 `OUT/chain-front-genN-SLUG.tsv`，表头绑定实际
`part.tsv total`，各行是 `PHASE<TAB>FIRST<TAB>COUNT`；merge/stub使用 `-1/0`。
candidate在恢复时逐字比较，chain-run校验total与完整片序后消费同一库存。
静态 `build/chain-phases.tsv` 仍是顶层调度表；动态front子相不以固定三片伪装成完整库存。
调用静态分析支持常数下标、数组初始化、`n++`追加及已识别包装器转发；
未知表达式或数组逃逸均属于拒绝域。源码正控与变异负控不得运行被分析的真实链。

轻夹具实测：`check-chain-facts` 的输入计数/根列表/输出漂移拒绝及确定性恢复
11项通过；`check-chain-partition` 的44/64/96/97/128单元、129拒绝、逐片until、
merge/stub边界、恢复不重复子调用及runner库存漂移拒绝均 `failed=0`。
97单元真实库存为 `0/32,32/32,64/32,96/1`，不是固定前三片或96单元硬顶。
证据在私有 `out/facts-evidence/{facts-fixture,parts-fixture}.log`；这些是假工具结果，
不证明真实构造耗时、重建或整链资格。

`build/chain-facts.tsv` 保存有序根对象/源路径及P3相名，`chain-facts-gen --write`
生成 `tools/cxlib/chainfacts.cx`；`--check` 逐字比较，不写入漂移输入或输出。
Linux根对象与span根共用映射；展开闭包仍独立由 `entry-build --closure-list` 提供。
网络/工具列表唯一保留在 `cxlib.sourcepackage`，`sp_model_count()` 与新增
`sp_tool_count()` 按真实数组长度求值，生成器拒绝事实表计数与该权威列表不符。
`chain-phases-gen --check` 在比较相表前还核事实表与已生成列表一致；本次实跑
`CHAIN_PHASES status=CHECK n=485`，不是只核旧表字面行数。

调用检查实测正控核19个调用，11个独立变异负控均拒绝，`CHECK_CHAIN_ARGV_CHECK failed=0`；
包括缺参、位置交换、裸helper、未知包装器、动态front错序、merge/root未知循环界及callee位置交换。
只有正控先通过才运行负控，负控还逐例核其诊断，其他共同失败不能冒充负控通过。
完整原始失败与修订后成功日志均保留在 `out/facts-evidence/argv-fixture*.log`。
本次索引库存实测773源、97个非空编译片，范围 `tools-compile-000..096`；
096含5源，097为空不登记。库存登记与`--shards`通过不声称97片全部实编。

K7 的 `release-exec`、`release-prepare` 需要登记的原生工具材料，按现行元数据合同由 commit 改为 daily；材料、检查断言和55秒界不变，但不再计入 commit 档覆盖。
`check-releaseprepare` 的快照仍在调用者显式输出目录内的独立夹具根中；不再内嵌仓根 out 路径。

配套检查迁移：chain-facts 批次保留，检查名改为 check-chain-facts-gen；check-chainfacts 独立核生成库计数、顺序和边界。chain-argv-check 批次保留全部原有变异断言；check-chainargv 的 chainargv 批次实测原生命令行正控、用法、缺源和 NUL 拒绝。默认 -run 的源码 argv0 不作可执行映像使用；check-chain-run 已自编原生夹具。
