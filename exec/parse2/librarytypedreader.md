# Typed USLCON reader (2c2, reader commit only)

The default parse2 entry is unchanged. `parse2/librarytyped --librarytyped`
constructs an opt-in overlay at the existing USLCON reader installation point.
Without the constructor flag, no typed nodes are installed: the complete JSON,
table and threshold network must match an isolated copy with the original reader
installation restored. The existing byte/result tables and W runtime are not edited.

The enabled reader requires the explicit eight-byte LE `library/typed-mode=1`,
`library/data-contract=1`, selection owner1, and a kind0/callkind1/V3/fixed record.
Legacy owner2 V2/variadic records are handed back before new-class validation;
owner1 old categories and owner2 fixed V3 refuse, so there is no downgrade.
It reads the signature through MS.canonical, checks name and argument count, and
retains the existing LX.signature / MG.compatible source-prototype check.
Provider addresses are absent. PLAN and DISPATCH are zero, and the stable ID uses
an independent bank; the ordinary-function and data side streams stay untouched.

This commit does NOT install the tail overlay. Its old tail still emits dispatch
literal zero. The self-test executes only the model and observes this unconnected
metadata; its output must never be executed or described as a usable typed closure.
The later overlay will replace the literal with libraryfn; the later binding step
must construct the native plan and closure. No P1–P6 or full ABI proof is claimed.

## Reproduction (each command separately bounded to 58 seconds)

```
tests/bound 58 sh exec/parse2/librarytypedreadercheck.sh --prepare-tools
tests/bound 58 sh exec/parse2/librarytypedreadercheck.sh --prepare-base-c
tests/bound 58 sh exec/parse2/librarytypedreadercheck.sh --prepare-base-off
tests/bound 58 sh exec/parse2/librarytypedreadercheck.sh --prepare-base-reference
tests/bound 58 sh exec/parse2/librarytypedreadercheck.sh --prepare-base-tables-base
tests/bound 58 sh exec/parse2/librarytypedreadercheck.sh --prepare-base-tables-off
tests/bound 58 sh exec/parse2/librarytypedreadercheck.sh --prepare-base-net-base
tests/bound 58 sh exec/parse2/librarytypedreadercheck.sh --prepare-base-net-off
tests/bound 58 sh exec/parse2/librarytypedreadercheck.sh --prepare-typed
tests/bound 58 ./tests/gate.sh --suite exec-librarytypedreader-model
```

Preparation is split rather than hiding cold construction in the test window.
The former combined --prepare-base is refused (rc2): cold review exceeded its
58-second window. Each new step validates its prerequisite content hashes; only
the final network comparison publishes base.sha256. No timeout was increased.
Every cached artifact is hashed and rechecked; a missing preparation returns rc2,
not PASS. Root unisacc.com is only the dynamic-C observation tool, not a fresh
product under test. The native generic executor is likewise observation tooling.

The dynamic-C verifier owns all verdicts: 5 accepts and 10 named refusals, with
60 table/network/A/B executions. Frozen complete E1 input frames include USLFACT1
and UNITOK, with explicit attributes; locations, sourcefacts and signature version
must agree. The reference oracle is intentionally only a fixed two-int witness:
it consumes explicit used/defined/source-compatible observations and does not parse
C, certify arbitrary graphs or claim native ABI qualification. The rejected naked
token experiment lacked layout provenance; it is not converted to a positive.

The parse2base C constructor also enforces the base check hook's repeated-P
rejection. The negative manifest defines the same P twice; both constructors must
return rc1 with `defined twice` and no stdout. Manifest label rows merely update a
label set, as in Python; they do not call P.label. The fixed token base is reused,
not reimplemented. Existing seedgen and all eight seedparse2 flag variants are
separately checked, with newly measured admission evidence for the six formerly
unmeasured variants. Cold Python references and unknown flags remain UNKNOWN.
