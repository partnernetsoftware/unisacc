# Fixed typed ownership: independent model component

Model input is USLPAR1 and the same address-free canonical declaration records as
librarypartition. Mode byte 11 is a bitset: bit0 migrates data as before; bit1
opts fixed typed V3 into the function physical primitive. Values outside 0..3
refuse. Modes 0/1 retain the original outputs and refusals.

Output stays USLSEL1, containing physical owner1 (function), owner4 (data) and
owner2 (legacy). Fixed typed is a distinct logical consumer, NOT new owner8.
For kind0/callkind1/signature-version3 with bit1 enabled, physical owner is 1;
ordinary kind0/callkind0 also physically uses 1. Typed V2, callbacks and variadics
remain legacy. No provider address, dlsym result or native plan enters this rule.

Audit byte10=0 classifies; 1 audits physical claims exactly as before; 2 audits
logical consumer masks: 1 ordinary, 8 fixed typed, 4 migrated data, 2 legacy.
The logical mask8 is observation input only and is never an owner in USLSEL1.
Missing, duplicate and wrong claims refuse with empty stdout and named reasons.
Ordinary claiming typed (or typed claiming ordinary) is wrong-owner even though
both use physical1; combined mask9 is duplicate-owner. This is the partition
proof, not yet proof that actual E3/lower consumers submitted the right claims.

The manifest reuses the existing common byte/result tables with explicit skip
lists and small typed overlays. Original shared tables and the generic executor
are unchanged; no copied common grammar or new large stage is introduced.
Opaque USLSIG3 payload framing is checked, not ABI compatibility. Native plan
instantiation, provider cross-signature refusal and main-only E3/lower outputs
belong to subsequent independent components and integration.

Warehouse owner8 experiments were superseded before integration and are not
product contract evidence. The actual unchanged-framework path uses owner1.

Actual warehouse evidence: C/Python construction JSON byte-identical, 194 states;
195 network banks / 198 hidden units / 7,833 bytes. Table/network actions and
strings identical on 50,054 observations. Four dynamic-C slices cover 2,899
fixed cases (166 accept / 2,733 refuse), including every byte-valued audit
claim for ordinary/typed/data, all mode byte values, explicit mixed ownership,
and both crossed logical claims. Legacy-mode reference comparison covers 1,047
observations. No ABI or deferred-plan integration is claimed by these counts.
Existing librarydata model: 54 live-reference fixtures, full/ARM/object C/Python
construction and spans unchanged (all rc0).
