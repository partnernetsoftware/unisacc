# USLSIG2 declaration protocol (R10, 2026-09-29)

This is model output, not a host source parser. Outer USLTAPE1 framing is unchanged.
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
Static linkage retains flag0. True variadic export is still flag0 pending typed invocation API.

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
These layout facts come from SSZ/SAL/SMEM/MOF/MPT/MBS/MAR/BFW/BFO/BFS.
Tag3 array payload: element_count, stride_bytes, child_descriptor; width=count*stride.
Bitfields, unions, flexible arrays and unresolved types stay represented but unsupported
until a model NativePlan/typed callback path is implemented, never faked as byte arrays.
Tag4 reserved for nested function signature; version2 readers reject a nonempty tag4
until its schema is introduced together with the typed callback implementation.
Each descriptor payload is bounded by its declared length and must be consumed exactly.
Limits: descriptor recursion32, nodes16384 per record, byte extent<=16MiB.

Host call frame contract (mechanical):
`us_export_frame {slots,count,mode,result_kind,result_bytes,result}`.
slots are raw uint64 words. GP values sign/zero extended according to descriptor;
FP copied as raw bits; aggregate slots point to per-invocation owned native objects.
Result buffer stores a uint64 raw word for scalar, declared-width bytes for aggregate,
no object for void. Aggregate copied before releasing the script call frame.
`us_export_invoke_frame(owner,raw,&frame)` returns status, with no signature inference.
Original us_exports_symbol/invoke and V1 unit tests remain supported. New public library
integration calls us_exports_symbol_frame and chooses explicit REGISTER/ALL_STACK mode.

Prune consumes V1 and V2 framing, retaining public definitions regardless support flag.
It validates all scalar framing/bounds before skipping a payload; the host typed decoder
validates the complete recursive layout before any execution. No public roots are picked in C.

This protocol alone is not complete R10 FFI. Typed variadic import callsites, union/bitfield
NativePlan, function-pointer ownership, wider long double and all target native tests remain.
