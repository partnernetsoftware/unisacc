# Typed USLCON reader and terminal wrapper overlay (2c2)

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

The reader-only commit left the old dispatcher literal zero. The subsequent
terminal overlay now replaces that literal with `.libraryfn r1, ID` and bypasses
only the old alias route for the new typed ID. It reuses the existing frame and
tail, preserving PLAN=0; it does not instantiate a native plan or closure. Its
main-only typed wrapper is not executable before the later lower/binding work.
No P1–P6 or full ABI proof is claimed.

`librarytypedoverlaycheck.cx` extracts actual symbol256 references from the E3
output: a typed dispatcher site claims logical consumer8; an ordinary side
fragment claims consumer1 and must exactly mirror its main wrapper. That mirror
is one consumer, not two. Each observed claim is fed to the partition model in
audit2 mode and checked against its reference; crossed claims and mask9 refuse.
These are one typed and one ordinary emitted-symbol witnesses, not a proof for
arbitrary declarations or lower consumers. The ordinary control uses a captured
complete E1 UNITOK/sourcefacts frame; its side matches the existing reference,
and A/B observations must preserve the full E3 output.

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
tests/bound 58 sh exec/parse2/librarytypedreadercheck.sh --prepare-audit
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
