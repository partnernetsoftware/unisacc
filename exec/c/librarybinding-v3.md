# USBIND3 and typed native call protocol (R10, implementation decision)

This extends fixed-type native imports; full variadic/callback/union/bitfield and wider floating obligations remain R10, not removed.
USBIND1/2 existing public structs and bytes remain readable. USBIND2 already means candidate origin+ordinal. New APIs do not change the old us_signature ABI.

## APIs and shared typed declaration
`us_add_symbol_typed(ctx, name, address, signature_bytes, signature_length)` and
`us_declare_import_typed(ctx,name,signature_bytes,signature_length)` accept a complete USLSIG2 blob with exactly ONE external defined function record, matching name. The TypeGraph is exactly librarysignature-v2.md; count<=1024. Host deep-owns declaration/ffi layout, does not parse C or pick a name winner. Fixed FP, GP, pointers, naturally laid out plain structs/arrays supported when verified by libffi. Unsupported layouts retained, referenced calls must reject. Existing old API stays exact.

## Wire
Header `USBIND3\n` (8), LE64 candidate_count. Each LE64 record_length then:
LE64 name_len, ASCII name, u8 kind, u8 origin, LE64 ordinal, u8 abi, u8 variadic, LE64 address, LE64 count, u8 format.
- format=0: old (result + count args) six-word descriptors; old optional data extent/writable; u8 supported. This permits mixed old/new registries.
- format=1: kind=0 only; LE64 dispatcher_address, LE64 plan_handle, LE64 signature_length, complete USLSIG2 single-record blob; u8 supported. Signature name/count/variadic must match enclosing record. Native-system ABI remains abi=0, valid only for ctx host-native relocation; target check already belongs to API.
Origins and rank exactly USBIND2: 0 injected ordinal0; 1 process ordinal0; 2 owned ordinal1..64. No C winner selection. V3 normalized output remains USBIND3 with unique names and origin/ordinal both0; retains format/payload/length unchanged. V1/2 normalization remains USBIND1 byte-identical.
Raw target and dispatcher are distinct fields. Host prepares one context-owned ffi plan PER candidate address, including unselected candidates, without picking a winner. plan_handle is an opaque stable owned address, never a C struct written into the model. Unsupported plans use handle0/support0. The dispatcher validates active context and exact handle membership before dereferencing it.

## Shared model signature canonicalisation
Signature comparison belongs to delta. A bounded reusable signature canonicalizer takes a USLSIG2 single record and returns canonical ABI bytes: variadic byte + count LE64 + result/all parameter TypeGraph descriptors, with each recursive descriptor's parser-local base and shape zeroed. It preserves depth/kind/width/unsigned/alignment/tag/member offsets/bit facts/array count-stride/children, checks full framing and bounds (librarysignature-v2.md), and reports result kind/width and support. Name/linkage/script_mode/defined/support flags are checked structurally, but name and parser-local IDs are not ABI keys. E3 serializes the actual source prototype through the SAME librarytypes serializer and full parameter capture, canonicalizes both sides, and compares all ABI bytes. Capture needs a per-signature epoch, not one global current definition epoch. No host source parser, no third type classifier. Source definitions win before unsupported import checks, as before.

## Wrapper and generic dispatcher
Reuse `.librarycall target_register, control_pointer_register` as the existing explicit module-only GP call capability. Its native target for format1 is ONLY dispatcher_address, never the arbitrary typed target. E3 declares the selected plan and typed layout by emitting the wrapper, lower gates the format1 resource and capability without reclassifying C.
Control pointer addresses SIX uint64 words (48 bytes):
0 plan_handle; 8 argument_slots_pointer; 16 result_pointer; 24 argument_count; 32 reserved0; 40 reserved0.
The dispatcher signature is uint64 dispatch(uint64 plan, uint64 slots, uint64 result, uint64 count, uint64 reserved0, uint64 reserved0). It returns0 on success; failure returns through ScriptFrame error/exit mechanism, not a fabricated successful result.
Argument slots are count raw uint64 values. FP raw bits, GP sign/zero-extended by source semantics; aggregate slot points to the by-value object. Script wrapper uses standard FP prologue, so ALL_STACK parameter k is FP+16+8k; REGISTER fixed<=6 uses r0..r5. Wrapper frame reserves control48+slots8*count with correct alignment. Preserve/save r6 standard script ABI.
Scalar result buffer is8 bytes, void none. Aggregate result buffer is declared size/alignment, emitted in script image using the existing __rv_<name> return-buffer convention. Dispatcher copies exact declared bytes; wrapper returns its address for aggregate, raw word for FP/scalar, and void0. Do not return an expired stack buffer.
Host ffi_call converts raw slots to native scalar objects and private aggregate copies using the model-verified explicit graph. All per-call allocations register with the current ScriptFrame BEFORE ffi_call; normal return frees them, explicit nested script exit cleanup frees them on unwind. Context invalidation destroys old plans after code/closures are quiescent; failed registration preserves old generation.

## First actual test
Host us_sym(script_exchange9) -> model-compiled script -> injected real host_exchange9 -> Pair result17.00/40, O0/O1/O2,100 repeats, original caller objects unchanged. Also fixed FP9 and integer17 imports; wrong referenced graph rejected, unused/source-shadowed unsupported candidate retains previous priority behaviour. Actual two macOS ISAs first, other four targets still explicit required follow-up; unit libffi smoke alone does not close this slice.

## File domains
Host agent: librarybindings.h/libraryresolver.h and NEW librarynative.h + host unit tests. Root owns libunisacc.h/.c integration and ScriptFrame cleanup/public API, PRD/gate/candidate.
Candidate agent: modelcandidates.py/modelbindings.py, NEW shared typed signature canonicalizer, lower/libraryimports.py + own tests. Expose reusable delta canonicalization helper to E3, communicate its exact registers/entry.
E3 agent: parse2/libraryimports.py and necessary capture/serializer hooks in libraryexports/librarytypes/gen2; import existing/new shared canonicalizer; own probes. No edits to candidate/lower/host files.
