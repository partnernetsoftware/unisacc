# Variadic ownership overlay (partition evidence only)

USLPAR1 mode byte11 retains bit0=data and bit1=fixed typed; bit2 explicitly
enables V3 variadic declarations. Values0..7 are accepted; other bits refuse.
Logical variadic consumer mask16 is distinct from fixed typed8 and ordinary1;
all three physically use function owner1 in USLSEL1. Data remains4, legacy2.
Only wire kind0/callkind2/signature-version3 with bit2 set migrates. Nothing
checks provider addresses, dlsym success or native plans to classify ownership.

This manifest references the existing librarypartition byte/result body and
librarytypedpartition overlay. Fourteen category byte rows and sixteen result
rows override only mode/variadic classification and audit edges. It copies no
framing/name/signature/record loop, introduces no owner/stage/executor, and does
not alter the shared source tables. The new model is206 states; C/Python JSON
are byte-identical. Table/network agree on53,150 observations and all actions.

Audit byte10=2 compares logical consumer masks:1 ordinary,8 fixed typed,
16 variadic,4 migrated data,2 legacy. With variadic disabled the previous
claim-byte domain is unchanged. With it enabled, combined masks involving16
are duplicate-owner; single crossed claims are wrong-owner; zero is missing.
Physical audit byte10=1 never accepts logical8/16 as physical owners.

The new reference validates opaque USLSIG framing, NOT ABI compatibility.
New typed signature payloads are actual product outputs from the real C sources
listed in signature-provenance.json (V2 andV3 separately generated; no magic
rewrite to impersonate another wire version). Existing old fixtures are retained
as compatibility/framing observations, not retroactively claimed native ABI
qualification. Framing/header mutations likewise are not callable API proofs.

12,194 fixed cases include 2,627 original-mode0..3 exact reference comparisons,
all mode bytes, all claim bytes on migrating mixed classes, crossed/missing/
duplicate consumers, count/truncation/extra-byte cases and 3-class mixed frames.
The expanded TSV is6.1MB (~133KB deflated); repeated canonical signed frames
are intentionally explicit so every expected refusal/output is reviewable.

Eight fresh selftests construct C and Python models independently, compare
JSON, make table/network, check the complete transition observation domain,
then run the dynamic-C verdict checker against the frozen expected rc, bytes and
named refusal columns of the cases TSV (they equalled the Python seed's ledger at
HEAD e3c4b123; that ledger no longer runs, W1-F).  C checks every selected model
rc, exact output and the named refusal;
crash/signal/timeout is never refusal. Batched reference startup does not drop
cases. Every setup command is bounded15s, observer53s, whole named job58s.

No E3 actual-consumer/audit linkage, callsite frame, promotion, lower span,
provider, ABI, plan lifetime or V0–V7 evidence is claimed here. No W/runtime,
shipping package selection, original lower/E3 tables or public API changes.
