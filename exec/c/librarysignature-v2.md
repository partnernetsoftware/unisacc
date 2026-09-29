# USLSIG2 declaration protocol (R10, 2026-09-29)

This is model output, not a host source parser. Legacy USLTAPE1 framing remains readable;
explicit callable capability uses the USLTAPE3 tape/exports/direct-sites/catalog envelope.
All integers below are unsigned LE64; one-byte flags are explicitly labelled.
Reader supports old USLSIG1 unchanged. No implicit type guessing or source-name ABI mapping.

Header: `USLSIG2\n` (8 bytes), record_count (<=8192).
Record: name_length, ASCII name bytes; linkage/defined/variadic/script_mode (four bytes);
parameter_count (<=1024); result_descriptor; stored_count (=parameter_count);
all parameter descriptors; supported (one byte).
script_mode: 0 REGISTER (fixed count<=6), 1 ALL_STACK (true variadic or count>6).
A complete ABI consumer uses this mode, never infers it from count in C.
Existing support flag now covers fixed scalar FP, GP/data pointers, and plain
struct/array layouts that the libffi backend can represent; unsupported kinds remain
recorded with flag 0. Actual native size/alignment must match declared layout.
Static linkage retains flag0. True variadic prototypes retain flag0 for ordinary
unspecialized export use. `us_sym_typed` now provides a caller-declared fixed
actual-signature entry; it does not claim a universal variadic native pointer.

Descriptor: depth, base, shape, kind, width, unsigned, alignment (seven LE64 words),
layout_tag (one byte), payload_length (LE64), payload bytes.
kind: 0 void, 1 integer, 2 data pointer, 3 FP, 4 function pointer, 5 aggregate, 6 unknown.
Width is actual byte size: 0 void/unknown, integer1/2/4/8, pointer8, FP4/8,
aggregate declared SSZ (not zero). alignment: 0 only void/unknown; otherwise a
power of two <=65536. Source local base/shape ids are diagnostic, not host ABI identities.
Plain primitives and opaque pointers tag0/payload0. Tag0 kind4 stays unsupported;
no native function pointer may be treated as a script address.
Tag1 struct or tag2 union payload: member_count (<=64), then for each ordered member
`byte_offset, bit_offset, bit_width, storage_bytes, child_descriptor`.
These layout facts come from SSZ/SAL/SMEM/MOF/MPT/MBS/MAR/BFW/BFO, with storage from MSZ and signedness tracked separately by BFS. Named lookup members are a source projection: anonymous bitfields and zero-width barriers are not represented by V2.
Tag3 array payload: element_count, stride_bytes, child_descriptor; width=count*stride.
Bitfields, flexible arrays and unresolved types retain conservative support0. Natural unions can now be certified by the model NativePlan route (see librarynativecarrier.md); V2 support0 is not erased as proof of native execution. Complete bitfield source certification requires the planned versioned ordered-layout facts, not a byte-array substitute.
Tag4 payload0 is an old unresolved callback and remains unsupported. A nonempty
payload is `schema:u8=1, form:u8, signature_id:LE64`. form1 is a reference with
no additional bytes. form0 defines a signature, followed by
`variadic:u8, script_mode:u8, parameter_count:LE64, result_descriptor,
stored_count:LE64 (=parameter_count), parameter_descriptors[], supported:u8`.
IDs are record-local, 1..1024, allocated contiguously in first-definition traversal.
Register a definition before its children: backward references and self references
are legal, but forward, dangling and duplicate IDs are rejected. Definitions have
independent counts/modes; each uses the same REGISTER/ALL_STACK rules as a record.
A nonempty tag4 must be kind4/depth1/width8/alignment8. Additional indirection is
an opaque data pointer (kind2/tag0), never automatically dereferenced.
The explicit callable capability now preserves complete callback declarations for
the registry bridge. Legacy consumers without that capability retain their
conservative refusal. Fixed callback layouts require successful host graph/layout
validation and context-owned conversion; a support bit alone is not execution
proof. A true variadic signature still cannot supply an unknown-tail closure.
Public callback execution and typed concrete varargs have native evidence on
macOS ARM64/Rosetta and Linux ARM64, with their exact scopes recorded separately.
Canonicalisation zeroes parser base/shape and emits deterministic definition IDs,
preserving graph topology, layout and nested modes. Top-level mode remains separate
metadata under the existing canonical comparison contract. Canonical graph bytes
preserve sharing topology: one shared leaf and two identical leaf definitions can
have different bytes despite ABI equivalence. The fixed and variadic declaration matchers now share MG.equal, a bounded
coinductive node-pair comparison that ignores sharing topology and support metadata
while preserving nested modes and layout facts. Its 65536 distinct-pair budget
rejects exhaustion explicitly; canonical byte equality alone does not establish
all equivalent callback graphs. Source graph serialization and directional
callable conversion are integrated. SCRIPT/NATIVE handles belong to a context and
image generation; incoming native closures recover an existing SCRIPT handle
only after registry ownership and complete ABI checks.
Each descriptor payload is bounded by its declared length and must be consumed exactly.
Limits: descriptor recursion32, nodes16384 and signature definitions1024 per record,
parameter count1024 per signature, byte extent<=16MiB.

Host call frame contract (mechanical):
`us_export_frame {slots,count,mode,result_kind,result_bytes,result}`.
slots are raw uint64 words. GP values sign/zero extended according to descriptor;
FP copied as raw bits; aggregate slots point to per-invocation owned native objects.
Result buffer stores a uint64 raw word for scalar, declared-width bytes for aggregate,
no object for void. Aggregate copied before releasing the script call frame.
`us_export_invoke_frame(owner,raw,&frame)` returns status, with no signature inference.
Original us_exports_symbol/invoke and V1 unit tests remain supported. V2 public
exports use the context-owned callable registry for supported fixed declarations;
legacy adapters remain available. Script entry uses the declared REGISTER or
ALL_STACK mode, including explicitly typed fixed specializations of variadic exports.

Prune consumes V1 and V2 framing, retaining public definitions regardless support flag.
It validates complete V2 descriptor and callback graphs through the shared model
validator; the host typed decoder also validates the graph before any execution. No public roots are picked in C.

This protocol alone is not complete R10 FFI. Public typed variadic import callsites
and concrete indirect variadic sites are implemented and tested locally, including
Pair/fixed-callback tails and explicit variadic script export specialization.
Union/bitfield NativePlan, wider long double and remaining native platforms
still require completion. See librarycallable-specialization.md and the
research/r10-*-evidence.json records; no six-platform completion is implied.
