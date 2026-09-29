# USLSIG3 ordered source facts implementation contract

2026-09-29; foundation for full R10 ABI support, not ABI certification itself. Preserve exact USLSIG1/2 decoding and absent-resource V2 output. No host ABI classifier or executor primitive.

Wire: record/header/count/flags/mode and callback schema are V2 unchanged except magic USLSIG3\n. Every descriptor has seven V2 LE64 words, then fp_rank:u8, fp_format:u8, natural_alignment:LE64, layout_flags:u8, layout_known_mask:u8, layout_origin:u8, then V2 tag:u8 / payload_length:LE64 / payload. rank0=unknown or non-FP; ranks1/2/3=float/double/long_double. format0=unknown or non-FP;1=IEEE32,2=IEEE64,3=x87extended80/padded16,4=IEEE128. FP formats1/2 widths4/8, formats3/4 width16. Semantic rank and storage format are distinct. Non-FP rank/format must0. Layout flags bits0packed,1explicit alignment; known_mask bits0packingknown,1alignmentknown; no flag outside knownmask. origin0=parser projection,1=complete source facts,2=explicit external layout facts. natural_alignment0=unknown; otherwise power of2<=65536. Unknown source facts MUST NOT be upgraded to complete. The standalone token source producer emits origin0/known0/flags0/natural0 and retained declaration rank1/2/3 for float/double/long double, including typedefs, arrays and callback declarations; fp_format1/2 describes actual F32/F64 storage. Rank3 with format2 retains long-double semantic identity but does not provide native wide-FP support. Concrete call-site descriptors retain semantic value ranks through literals, variables, members, arrays, dereferences, casts, arithmetic/conditional expressions and call results; fixed argument conversion uses the declared parameter rank, and variadic F32 promotes to rank2/F64 while rank3 remains rank3/F64. Opaque pointer descriptors do not yet encode pointee semantic identity. V3 source supported flag remains0, including nested callback definitions; format acceptance is not runtime certification.

V3 struct/union payload: entry_count LE64 <=128, then each ordered entry: ordinal LE64 (must equal0..count-1), kind:u8 (0ordinary,1namedbitfield,2anonymousbitfield,3zero-width barrier,4anonymous aggregate), effective_alignment LE64 (power2<=65536), V2 layout words offset/bit_offset/bit_width/storage LE64, recursive V3 child descriptor. Ordinary/anonymous aggregate bitwords0; storage equals child extent; child fits parent. Named/anonymous bitfields: positive width, contained in storage bits, child kind1/depth0/tag0 with storage==child width, and storage fits parent extent. Barrier: width0/bit_offset0, storage equals declared integer child width, offset <= parent width; it occupies no bytes so offset+storage may exceed parent extent. Anonymous aggregate child tag1/2. Ordinals are declaration order, not lookup flatten order. V3 arrays/callbacks recursively use V3 descriptor. Atomic decode and bounded recursive ownership remain.

Capability: E3 reads u64 resource \0library/signatureversion. Absent or2 outputs byte-identical V2;3 enables V3 in all top-level/nested emission sites; anything else or wrong resource length rejects. V3 source uses actual LF banks600..616 and array shape; no guesses from SMEM. Old source facts stays defaultoff; library symbols already enables LF capture. Public contexts default to V2; explicit us_set_signature_version(ctx,3) selects the source-evidence chain described below.

Model framing: MS.canonical preserves wireversion and every new fact, still normalizes base/shape as V2; MG.equal indexes all new fields/layout entryfacts and compares them including version/completeness. V2 index layout must remain exact because nativeabi consumes MG directly. Use fresh separate banks only after parent coordination; reserve620..639 for V3-only descriptor/model metadata if needed, assert collision. Nativeabi indexes V3 metadata separately and certifies only complete natural facts within its declared domain; incomplete origin0 remains a named rejection. Do not parse V3 using V2 offsets.

## Certification boundary

Complete-source origin 1 requires known-mask 3 and positive natural alignment (zero-width void/unknown excepted). FP rank 1 requires format 1, rank 2 requires format 2, and rank 3 requires format 2/3/4. Zero-extent aggregates are rejected. Canonical V3 starts with `USLSIG3\n`; V2 canonical bytes remain unchanged. The native carrier model now accepts complete origin1/2, known-mask3, no layout modifiers and matching natural alignment. Ordered natural aggregates of 1..16 bytes and alignment1/2/4/8 use actual occupied bit intervals, including zero-width barriers, container reuse, arrays and anonymous aggregates. Anonymous SysV lanes qualify only where the two supported unnamed-bitfield policies agree. FP rank1/format1 and rank2/format2 are accepted; rank3 remains rejected even with F64 storage. The original V3 graph remains in USLNCAR1 alongside a model-produced V2 carrier; the host checks ownership/storage/callback edges and does no ABI classification. This is partial bitfield certification, not complete packed, wide-FP, general BANK or public-source interoperability. Standalone source origin0 cannot enter this path; the explicit public source-evidence chain below supplies origin1 only after validation.

## Reproducible checks

- `tests/librarysignature3check.py`: independent wire fixtures, atomic ownership, truncation, ASan/UBSan.
- `tests/modelsignature3check.sh canonical|equal`: independent canonical and cyclic graph oracles against simulator and constructed network, plus full-domain table equality.
- `tests/modelsourcefacts3check.py`: actual E3 source emission, declaration-order/array/callback oracle, actual host decode, and absent/explicit V2 compatibility.
- `tests/modelnativecarriercheck.py`: six named V3 certification rejects, retained V2 native carrier recipes.

- `tests/modelfprankcheck.py`: declared FP ranks through aliases, arrays and shared callback graphs; rejects scalar/callback declaration conflicts on simulator and constructed network; old valid V2 bytes preserved.

- `tests/modelfpvaluerankcheck.py`: four actual concrete call sites / 25 argument descriptors, fixed conversion and variadic promotion, global/local scope restore, V2 baseline bytes, full-domain network/table equality and host ASan/UBSan decoding. This verifies metadata, not native wide-FP execution.

- `tests/modelnativebitfieldcheck.py`: 15 independent ordered recipes, six profile byte comparisons on simulator and actual C network, rejected incomplete/modifier/policy facts, and full-domain network/table equality.
- `tests/libraryorderedcarriercheck.py`: certificate framing/truncation, identity metadata/order, no-carrier rejection and ASan/UBSan mechanical pairing.
- `tests/modelorderedbitfieldnativecheck.py`: measured actual C B4/B8/BD16/DB16 layouts, three register-pressure shapes, exact independent carrier bytes from the actual C network, actual native and SCRIPT closure calls under ASan/UBSan; macOS ARM and separately qualified x86 libffi. Not a public source-to-call or six-platform proof.


## Public source provenance mode

`us_set_signature_version(ctx, 3)` selects V3 before compilation; the default is
2 and existing V2 wire bytes remain unchanged. The library passes the explicit
signature-version and source-facts resources through the ordinary model route.
E2 and E1 emit `USLFACT1\n`, stage/status/policy bytes and a little-endian u64
payload length; the payload is the complete old stage output. E3 validates this
chain before recording complete source origin 1. Old standalone token streams
without chain evidence remain origin 0. No comment in user source grants proof.

Policy 1 describes this compiler's existing 64-bit storage layout. Active
unrecognized pragma / `_Pragma` deletion, or E1 skip-parenthesis extensions,
make the translation unit unknown. Multi-unit framing validates every unit;
one unknown unit conservatively makes the merged source unknown. Clean source
records have known mask 3, no modifier flags, and their actual natural alignment.
This is source provenance, not a native ABI theorem. Nativeabi independently
checks the original complete graph and chooses a carrier; V3 support remains 0.
An unknown/packed graph must not become a usable native function pointer.

The public source qualification currently measures B4/B8/DB16 calls on macOS
ARM and qualified libffi 3.5.2 x86 at O0/O1/O2. It does not establish packed,
wide floating point, BANK, source/import cross-origin refinement or six-platform
qualification. Fixtures and framing checks are separate from actual calls.
