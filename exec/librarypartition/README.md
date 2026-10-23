# 2b declaration ownership model (first independently verifiable component)

No E3, lower, provider, or library consumer has been connected in this commit.
The existing product is only the dynamic-C observer. The reference is the frozen
expected-bytes column of tests/librarypartition-cases.tsv (captured once from the
Python seed, W1-F, 2026-10-23).
Core input never contains a provider address or availability decision.

Input `USLPAR1\n` followed by the canonical USLDEC1 declaration layout (see
exec/librarycontract/README.md), with header byte 10 selecting classify=0 or
audit=1. Header byte 11 selects data migration: 0 retains legacy data ownership,
1 assigns kind=1 data declarations owner bit 4. Unknown modes reject. Records
retain zero reservations. All declaration bounds, sorted names,
IDs, signatures and exact lengths are checked by the finite model, not an adapter.
In audit mode exactly count route-mask bytes follow the records. Each must equal
the model-derived owner: 1=new iff kind=0 and callkind=0, 4=data iff kind=1
and header byte 11 is 1, otherwise 2=legacy. These are independent owner bits.
0 rejects missing-owner; 3 always rejects duplicate-owner. In data migration
mode 5, 6 and 7 also reject duplicate-owner. Claims 1/2/4 must match the exact
model owner or reject wrong-owner; notably claim 4 in old mode is wrong-owner.
In old mode claims 5/6/7 remain malformed. No declaration may have two owners. Unknown kind/callkind/version rejects; no provider fallback exists.
Classify mode permits no claims or trailing bytes. Audit claims are observations
of consumers' actual routing, not authoritative classification inputs.

Output `USLSEL1\n,count:u32` then ordered `id:u32,owner:u8` records, little endian.
Nothing is emitted before validation and audit are complete. Refusals have rc1,
empty stdout and named library partition diagnostics. Output is only ownership,
not ABI qualification, provider validation or proof that old bridges are migrated.

Actual local construction: seed/gen C and Python JSON byte-identical; 172 states,
173 banks, 175 hidden units, 7040 network bytes; table/network actions identical
on 44378 observations. Dynamic-C selftest: 489 cases, 84 accept / 405 refuse,
exact reference output. Includes empty and shortest nonempty frames, inconsistent
count, truncation, unknown declarations, and missing/duplicate/wrong route masks.
Initial new comparison branches were wrongly installed in the byte-observation
section, causing an empty-frame loop; moved to result-observation rows. Repeated
all cases with the original 3-second per-execution bound. A fixture conversion
initially erased corrupted magic; repaired it by preserving each mutation delta.
No criterion or timeout was relaxed.


2c1 actual model-only extension: seed C/Python JSON identical; 176 states,
177 banks, 181 hidden units, 7197 network bytes; table/network action equivalence
on 45410 observations. All original 489 cases remain green without changing
expected results. Added 54 fixed cases (16 accept/38 refuse), including empty
migration frames, mixed ordinary/data/typed ownership, classify and audit,
missing/wrong/duplicate claims, unknown modes and truncations. New accepted
fixtures assert explicitly composed owner bytes as well as exact reference
output; diagnostic reasons are checked on new negative fixtures. No product
consumer is migrated by this model-only change.
