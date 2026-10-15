# Callback ownership overlay (partition proof only)

`gen` reuses the librarypartition, librarytypedpartition and librarycallsitepartition
finite tables and the existing parsebase/shared executor. It copies no framing,
name/signature/count loop or runtime mechanism. `--librarycallback` explicitly
installs only the callback-owned seams; without it the complete construction is
byte-identical to librarycallsitepartition, including mode8..15 malformed refusals.
No shipping capability or runtime selection is changed in this model-only step.
Future wiring will advertise ownership-callback; this step does not claim it exists.

USLPAR1 byte11 retains data/fixed/variadic bits0/1/2; explicit callback adds bit3.
Wire kind0, callkind3, signature_version3 with bit3 set consumes logical mask32
(kind4); physical USLSEL1 still owner1. All prior classes stay on their own path.
Masks1/8/16/32 share physical owner1 without sharing logical ownership. Wrong,
duplicate and missing consumer claims have named refusals. Addresses/provider
success do not choose ownership; no plan/closure/lifecycle is instantiated here.
Nested USLSIG3 bytes are opaque validated observations: no ABI qualification,
nested-graph matching, E3 consumption, lower span or callback execution is claimed.

Compatibility is explicitly split (cc ruling): predecessor models and their full
unchanged fixtures run as before; this overlay agrees with its predecessor on
mode0..7; mode8..15 is a deliberate capability-gated extension, not a rewrite of
16 old reserved-mode negative fixtures. New positive callback signatures are
476-byte product output from the real C definition under probes/, with source,
generator, product and signature hashes in signature-provenance.json. Old fixture
bytes remain compatibility/framing observations, never retroactive ABI proofs.

The dynamic C judge checks exact accepted bytes and both model/reference named
refusal reasons, with the existing cx.h I/O/bound mechanisms. Python is only the
batched observation oracle; its ledger binds the complete fixture SHA. Timeout,
signal and missing artifacts are mechanism/host failures, never semantic refusal.
`check.sh 1..16` constructs C/P independently, compares full JSON, builds table/net,
checks the complete transition observation domain, then runs one bounded shard.
Every preparation is <=15s; observer <=53s; each whole invocation <=58s.

This is a warehouse model selftest run by an old product as observation tool;
it is not fresh-product or callback ABI/runtime acceptance. No W/public header/
original tables/2c3 wiring source changed. Registration is requested separately.
