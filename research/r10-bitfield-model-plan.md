# R10 named bitfield classifier / carrier plan

2026-09-29. **Read-only design; no implementation or executable tests in this slice.** Repository untouched. The existing natural scalar/composite/16B model remains frozen. The separate x86_64 mixed-return system-libffi first difference is independently reproduced; do not use bitfield work to hide that limitation.

## 1. Current facts and the first necessary boundary

USLSIG2 V2 member layout words are exactly `(offset_bytes, bit_offset, bit_width, storage_bytes)`, followed by the declared child descriptor. `MG.LAYOUT` retains those four words, `MG.EDGES` retains the child, and `MG.FIELDS` retains extent/alignment/kind/unsigned/tag. `librarytypes.py:71–79` serializes MOF/BFO/BFW/MSZ and marks any positive BFW unsupported. Child kind1 width/unsigned/alignment supplies the resolved integer type. Named fields sharing a container have repeated container offsets/storage but distinct bit intervals. MS.canonical validates graph framing; it is not a target layout allocator.

Producer facts are incomplete: `BF.anonymous`, `BF.zero`, `BF.padding` in `bitfields-result.tsv` affect soff/umax/alignment but do not enter named SMEM. Therefore a V2 graph cannot certify that anonymous declarations, zero-width barriers, attributes or pragma packing were absent. It must not be described as a complete original declaration. Equal named fields plus extent/alignment are a transport projection, not proof of complete declaration equality.

Two domains should be separated explicitly:

1. **V2 explicit-layout certification:** accept only named ordinary integral bitfield facts whose positive widths, contained bit intervals, natural container constraints and surrounding named ordinary members satisfy a target's declared finite layout policy. Certify the supplied projected graph, not omitted modifiers. If transport depends on omitted facts or placement cannot be proven, reject with `not covered: incomplete bitfield layout facts`. Existing V2 source graphs remain support0 declarations.
2. **Complete source declaration certification:** requires V3 ordered layout entries/completeness flags. Do not enable it by guessing from natural-looking V2 offsets. In particular, do not claim all named source bitfields supported while the serializer drops anonymous/barrier facts.

A layout attribute that happens to produce the same layout is not automatically an ABI difference. Conversely, a matching size/alignment alone never proves that an unreported field cannot change a lane class or HFA eligibility. A V2 acceptance theorem must show the target transport is invariant over the omitted-fact equivalence class, or require a separately trusted producer completeness fact. The latter is the cleaner product seam.

## 2. Unified recursive representation

Extend NL.walk's result rather than add recognizers for individual structs:

- exact extent/alignment; natural-layout validity and target policy ID;
- bit occupancy (not only current 16-bit byte mask), declared padding and unknown coverage separately;
- N/S/I transport classes per eightbyte, with byte/bit offsets shifted before joins;
- HFA base/count/validity;
- contains_callable_object flag;
- compact storage intervals and callback conversion edges, when needed.

For <=16B, keep bit occupancy in two64-bit words. Use existing generic A64 actions for bit masks and comparisons; special-case width64 to avoid shifting by64. Larger-object MEMORY/indirect certification is a separate finite recipe policy, not silently implemented by a two-lane classifier. No executor action or C ABI classifier is required.

An ordinary scalar contributes its occupied bytes and existing I/S class. A named integer bitfield contributes **I only to lanes intersecting its actual declared bit interval**, HFA invalid, and a physical container interval used for natural storage bounds/alignment. Padding and unused container bits contribute N, not I or S. Natural aligned unit sizes1/2/4/8 cannot themselves straddle an eightbyte, but keep actual bit offsets so later packed/cross-boundary support does not rely on that accident.

Maintain distinct cursors:

- allocated bit end: `offset*8 + bit_offset + bit_width`;
- physical storage high-water mark: `offset + storage`;
- next ordinary member address computed from the target's bit-to-byte placement rule;
- record alignment contribution from the container.

Do **not** advance the ordinary cursor by storage for each bitfield. AAPCS permits a following ordinary member to reuse unallocated bytes of a container. For example named `u32 a:24; u8 b` can place b at3 while the bitfield's physical container spans0..4. Shared unit fields may overlap container intervals; their occupied bit intervals must not overlap in a struct. Union alternatives intentionally overlap and are joined independently.

Struct: iterate ordered entries, validate allocation/placement, merge shifted child masks/classes, preserve ordinary leaf recursion and callback edges. Array: validate stride/count/element extent and replicate shifted result, including bitfield-bearing struct elements. Union: validate each alternative layout, join N<S<I classes, merge HFA eligibility independently, and reject any callback object alternative. Any bitfield makes an ARM aggregate non-HFA even when adjacent FP fields exist. Overlapping alternatives do not add counts.

## 3. Concrete state seams and data

Existing gen.py locations:

- NL.memberedge / NL.bitwidth: currently reject any positive bit_offset/bit_width. Replace with a four-word reader followed by `NL.memberform` (ordinary versus named-positive-bitfield; barrier requires V3).
- New `NL.bfkind`, `.bfunit`, `.bfwidth`, `.bfrange`, `.bfnatural`, `.bfplacement`, `.bfoccupied`, `.bfclass`, `.bfnext` validate kind1/depth0/tag0, units1/2/4/8, positive contained width, target placement, then merge I/bit masks without NL.walk on the declared integer's full bytes.
- NL.naturaloffset / NL.childend: use explicit bit cursor plus storage high-water for records containing fields; retain current ordinary path unchanged.
- NL.occupied: current full-byte occupancy rejects many legitimate bitfields. Replace only in the bitfield domain by an explicit recipe-coverage predicate. Never fill an N-only eightbyte with u8/u64 and thereby change it to I.
- NL.result: return the extra occupancy/HFA/contains-bitfield facts. Existing NC callback epoch/IDs remain unchanged; classifier return fields must stay outside saved scratch.
- NC.aggregatekind / NC.structlayout: a bitfield-containing record cannot be emitted member-for-member as ordinary libffi fields. Route its whole transport projection to a finite natural carrier recipe; recurse outer ordinary arrays/structs only after full-layout and class revalidation.
- NC.unionwidth / NC.union16choose: reuse class/profile recipe dispatch; preserve one logical object. Additional <=8 aggregate recipes must stay aggregate roots, particularly ARM narrow composites, instead of reusing the old rejected narrow scalar-union recipe.

Current NL frame uses bank482 offset100000+depth*32; NC emission uses the original lower32-slot region. Current NL saved tuple has22 fields, leaving10 slots for a carefully documented expansion. Two-word bit occupancy plus placement state may exceed that; measure first and move NL to64-slot stride in the already isolated region if needed, retaining NC32-slot frames and collision assertions. No new bank should be chosen without parent coordination. Budgets remain graph nodes16384/signatures1024; classifier recursion32, source members64; exhaustion is not covered.

Declare in rules.tsv separate sections for:

- six profile layout policy IDs, little-endian bit numbering, allowed resolved units/types;
- named-field unit/straddle/reuse/ordinary-placement policy;
- bitfield leaf class I, HFA invalidation and N/S/I join;
- transport projection keys `(family, extent, alignment, lane classes, aggregate identity)`;
- recipe elements including offsets/storage, plus no-N-lane guard;
- reject policies for unknown modifier/barrier/anonymous completeness.

Actual field offsets remain input facts. The TSV policy checks their consistency; it must not silently overwrite original layout with another target's allocator. Windows/MS unit allocation must have its own policy and independent gold; the current source BF placement procedure is one generic algorithm and is not evidence of MS-compatible mixed declared-unit layout.

## 4. Carrier projection and host mechanical gaps

Carrier retains original extent/alignment, logical signature count/mode/name and callback signature topology. It can coalesce several shared bitfields into one ordinary integer recipe element or use two/four natural transport elements; bitfields are not flattened into extra logical parameters. Signedness remains in untouched original graph; copying is object storage, not numeric field conversion.

For SysV <=16B, recipes must reproduce I/S lane order and whole-object spill/result behavior. Partial occupied I lanes can use integer elements only after proving the class matches; N-only lanes stay rejected until a padding-preserving recipe exists. ARM named integral fields force non-HFA; use natural aggregate GP recipes with matching extent/alignment, retaining existing HFA path for records without bitfields. Win x64 retains small-aggregate integer or16B indirect+sret identity. Exact narrow/mixed-alignment recipes require proof and native gold, not a size-based fallback. Original/carrier must be independently reclassified in the model before acceptance.

Current host blocks are mechanical, not missing classifiers:

- `libraryexports.h:244` rejects bitfields in bridge structural validation; replace with kind/unit/range/bounds checks only, leaving support0 and no original ffi type.
- `librarycallables.h:189–193` insists on identical struct member counts and zero bit words. Shared-unit coalescing therefore cannot pass the existing pairing.
- conversion_make at216–224 currently chooses opaque COPY only for original union; structs recurse by member ordinal. A callback-free bitfield aggregate can use a model-certified opaque aggregate COPY edge after extent/alignment/bounds/opaque-safe verification. C must not choose a lane or classify a bitfield to decide the recipe.
- original union containing callback object remains rejected.

For bitfield structs that also contain ordinary callable fields, a blind aggregate COPY is wrong (handles need conversion). Either retain explicit callback descriptors at the same byte offsets in the carrier and supply model-declared conversion edges, or reject this slice pending the edge schema. Do not manufacture a one-to-one member pairing: two bitfields sharing one unit and a callback naturally change carrier member count. Recommended USLNCAR2 adds bounded per-descriptor COPY/RECURSE/CALLABLE edges with original/carrier byte offsets and signature references; host validates offsets, width, graph pairs and ownership mechanically. An ordinary root callback whose signature contains callback-free bitfield records can already keep the existing signature ID graph once those records gain COPY edges.

## 5. V3 facts required for complete scope

Recommended USLSIG3 record layout payload adds ordered entries with explicit kind `ordinary / named_bitfield / anonymous_bitfield / zero_width_barrier / padding`, declared type/container width/alignment, bit offset/width, ordinal/container grouping and effective member alignment. Record facts include target/layout policy ID, complete-layout flag, packing/effective alignment attributes and whether layout is externally supplied. Preserve source extent/alignment; do not redefine V2 words in place. Ordinary fields keep recursive child descriptors; callback schema identities remain record-local and independent of external declaration keys.

Required absent facts: unnamed/zero-width entries; whether packing/alignment controls or MS layout mode applied; effective alignment distinct from declared natural alignment; flexible/anonymous aggregate flattening and zero-sized extensions; signed/plain-int/enum/bool semantic provenance where not fully resolved. Transport-only may not need every semantic fact, but full ABI signature equality must not claim these declarations equivalent without a defined normalization contract.

V3 source producer must record BF.padding events, not infer them from gaps. Old V2 fixed/capoff bytes remain unchanged. Old malformed/support0 behavior must remain tested; a new capability cannot turn missing graph facts into supported runtime plans.

## 6. Independent executable oracle plan (not executed here)

A. Python wire oracle manually builds named descriptors and ordered layout words; it must not import rules.tsv or derive expected recipes from generated output. Expected target gold is handwritten from published ABI rules and actual system-compiler layout records. Parameterized widths, offsets, declared signed/unsigned units and member orders exercise the generic algorithm, not a handful of recognizable strings.

Positive families, each six profiles when a complete target fact recipe exists:

1. Shared u32 unit: named widths3+5+24, offsets0/BFO0,3,8; one logical struct4; whole I lane.
2. Partial shared unit3+5 with declared remainder padding; verify padding-policy acceptance/rejection explicitly, never mark remaining bits occupied.
3. Successive units: u32 widths20+20, offsets0/4; struct8 align4.
4. u64 widths31+33 sharing0; struct8 align8; signed/unsigned variants same transport but original facts differ.
5. u32 a:24 plus ordinary u8 at3 (unit overlap) and the reverse ordinary-before-bitfield arrangement allowed by each target's policy.
6. Bitfield unit before double at8:16B IS; double before bitfield unit at8:16B SI; ARM non-HFA GP, Winx64 indirect.
7. Mixed u8/u16/u32/u64 declaration-unit changes: independent MS versus AAPCS/SysV gold; no assumed common layout.
8. Array of two shared-unit structs, nested struct containing array plus FP member; validate stride, natural extent and shifted lane classes.
9. Union of bitfield-bearing integer struct and FP arrays: I dominates overlapping S; alternatives reorder/repetition; no callback member accepted.
10. Callback signature self/mutual/shared nodes whose parameter/result is a bitfield record; original graph unchanged, carrier topology/proof fields checked.
11. Struct bitfields plus ordinary callable leaf: expected named not-covered until conversion edges land, then explicit make/call copy-and-convert gold.
12. Small composite widths1/2/4 versus8/16 and larger indirect forms: certify recipes individually; keep root aggregate identity on ARM.

Negative mutations: bitwidth0 misrepresented as ordinary; width>storage*8; bitoffset+width overflow; storage mismatch with integer descriptor; FP/pointer/callback bitfield; depth/tag malformed; container outside root; unaligned unit; overlapping occupied bits in struct; illegal straddle; count/stride/extent mismatch; unknown target/policy; declared-packed/overalign; missing complete-layout evidence; anonymous/barrier projection collision; N-only lane; callback union; truncated input at each four-word boundary. Keep support1 malformed controls and all existing310 model oracle cases.

B. Actual finite-table suite: private full dependency snapshot; construct JSON->tbl->net; C --check-net all observation banks/actions/strings; independent sim and C network output exact bytes; reject must not accept/publish partial certificates. Inner20s, outer55s, Terminal native execution. Do not modify tree while running; split queues, record SHA256 and terminal process status.

C. System compiler layout gold: no offsetof(bitfield), which is invalid. Use explicit named assignment patterns and memcpy to unsigned-char storage to identify bit positions and signed extraction; query sizeof/_Alignof and ordinary member offsets; corroborate compiler record-layout dumps. Compare only named fields and ordinary member bytes when padding is unspecified; initialize inputs for source-copy immutability/canary tests. Do not treat unspecified padding preservation across native by-value return as a portable oracle.

D. Native ABI: actual C prototypes plus model carrier CIF direct/closure bidirectional calls, source imports/exports/factories and pressure matrices around each GP/FP boundary with two tails. Existing independently reproduced macOS x86_64 system-libffi mixed-return bug remains a named backend failure; never downgrade it to a recipe mismatch or count that platform green. Linux/Windows remain actual-native obligations, not inferred from model equality.

## 7. Primary references and evidence limits

- [x86-64 psABI source](https://gitlab.com/x86-psABIs/x86-64-ABI/-/blob/master/x86-64-ABI/low-level-sys-info.tex): integral bitfield storage/alignment and aggregate I/S/N classification. This supports a finite transport projection, not arbitrary packed layouts.
- [AAPCS64](https://github.com/ARM-software/abi-aa/blob/main/aapcs64/aapcs64.rst),10.1.8: named container/bit addressing, allocation, and ordinary members reusing container space; layout policy must preserve these distinctions.
- [Microsoft x64 software conventions](https://learn.microsoft.com/en-us/cpp/build/x64-software-conventions?view=msvc-170): declared unit boundaries constrain MS bitfield allocation; use target-specific gold for mixed units.
- [LLVM record layout implementation](https://github.com/llvm/llvm-project/blob/main/clang/lib/AST/RecordLayoutBuilder.cpp): source attributes and ABI layout policies are more than an extent/offset projection. It is corroborating implementation evidence, not a replacement C classifier.

Recommendation: first implement the generic bit-interval classifier and callback-free opaque aggregate carrier edges for complete named-layout facts, with an explicit V2 projected-domain theorem or a new producer completeness marker. In parallel specify V3 ordered anonymous/barrier/modifier facts and USLNCAR2 conversion edges. This is a staged path to broad bitfield support, not a proposal to redefine “bitfield support” as one layout.
