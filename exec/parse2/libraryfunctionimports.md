# 2b ordinary-function E3 component (model witness, no consumer migration yet)

The canonical `\0library/contract` USLCON1 resource must already pass the
librarycontract validator and ownership partition. Only kind=0/callkind=0
records enter this new branch. Existing USBIND3 owns the other classes; supplying
a same-name ordinary record to both streams rejects duplicate-owner. Absence of
the new resource preserves the old branch. Unknown or malformed contracts do
not acquire a fallback provider. Stable ID and intern name ID are distinct banks.
The ordinary path reads no ADDRESS, dispatcher, provider plan or availability.

The model appends imported declarations to the old metadata tables, retaining
source-defined priority and USED filtering. It checks source prototype against
canonical flat descriptor facts before emitting `.libraryfn r1, ID` instead of
the old address immediate. Existing typed/variadic/data wrappers stay old.
Capability, lower, relocation and the three new library APIs are NOT connected
by this component. Existing library API consumers still pass only old resources.

The model marks each new wrapper start with OLEN, captures its complete output
with OCUT after the return, restores the main stream and concatenates the captured
bytes. Output with a new contract is an internal USTFLOW1 envelope: magic8,
mainN:u64,fragmentN:u64,main,fragment. All fields little endian. The main may
retain the old USLTAPE1/2/3 envelope; no old signature/callsite format is changed.
Without a new contract the old output is byte-identical. C observers only decode
these lengths: there is no C scan of Tape or C selection of snippet boundaries.

Actual default-model construction: seed C JSON equals Python JSON, 10048 states,
10048 banks, 14760 hidden units, 1067079 network bytes. Dynamic C seven-fixture
contract passed: used wrapper exactly equals the narrow Python reference; unused
and source-defined names produce no new wrapper/fragment; legacy wrapper remains
unchanged; duplicate/truncation reject empty stdout with named causes.
Two native observation providers have different function addresses and environment
labels but are held OUTSIDE ResourceInput; replay output is identical. This is a
stage boundary witness, not the pending two-context library-API blackbox proof.
The reference only covers the explicit flat 64-bit witness, not all C/ABI shapes.

`libraryfunctionimportscheck.sh --prepare` prepares the C/Python comparison,
network and observation tool in a separate <=55s window. A conservative source
content key and artifact SHA256 prevent stale preparation reuse; default check
prepares on a miss and always reruns the dynamic C contract (results are not
cached). Every tool/contract subprocess has its own bound. No full gate/fresh
candidate is run. Initial observer load() and stdint include errors were tool
construction faults; corrected without relaxing contract criteria or limits.


## 2c1 data model witness (not library API integration)

Explicit `library/data-contract` LE64(1) enables address-free kind=1 contracts.
Selection must be canonical USLSEL1 with data owner4 and ordinary owner1; cli/target
must agree with the POSIX contract target. Missing/wrong/duplicate ownership and
ordinary/data same-name claims refuse with named diagnostics. No provider address
or provider-derived supported flag enters this path. Readonly address-taking and
reading pass E3; write refusal belongs to unbound lower, not this component.

The six-operand .libraryaddr is retained. The finite model marks each emitted
reference and accumulates a separate data fragment. Enabled output is USTFLOW2:
magic8, mainN:u64, ordinaryN:u64, dataN:u64, then main/ordinary/data bytes.
With the new resource absent, existing USTFLOW1 behavior remains unchanged.
This model-only commit does not connect us_library_data_fragment, provider
resolution, or the lower write-permission contract.

Actual dynamic-C named selftest: old seven plus thirteen data fixtures, 20 total;
positive realtime reference == frozen independent source expectations == model
fragment; negative model/reference both rc1 with named reasons and no stdout.
Table and network outputs match. A/B independent data object addresses are only
observations in stderr, never model resource inputs; direct LE/BE/decimal/hex
windows are absent from data fragments. Source/E1 UNITOK/package hashes are
recorded in fixture comments. The typed projection is a narrow witness, not a
complete located-input identity or result-reuse authorization.

Construction and readset/RSS evidence: research/c43-e3-data-receipt.md.
