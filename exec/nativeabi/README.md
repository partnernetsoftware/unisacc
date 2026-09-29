# FFI_CARRIER prototype stage

`python3 exec/nativeabi/gen.py OUT.json` constructs a finite delta accepting raw
single-record USLSIG2 or complete ordered USLSIG3 bytes and the explicit `\0cli/target` ASCII resource.
The six exact profiles and qualified mixed-union8 rule are in `rules.tsv`.

Output is `USLNCAR1\n`, little-endian u64 target length, target bytes, u64 original
length, untouched original bytes, u64 carrier length, carrier single USLSIG2.
Parameter count and top-level fixed calling mode remain unchanged. No native
address is contained in the certificate. The initial mapping is one direct
union object to one unsigned64 carrier, copied as object bytes by a future
consumer. It is never a numerical conversion or a flattening of parameters.

The shared `MS.canonical` validates the full original graph. Installing
`MG` also installs its graph equality procedures; only `MG.root` indexing and
FIELDS/EDGES/LAYOUT are used here. The standalone generator's ordinary finish
retains those states; no new shared parser or executor operation was added.
Stage scratch is `nc_` registers; existing MS/MG banks are borrowed, and
banks480–482 own callback graph emission scratch. Scalar descriptors are reconstructed with zero parser IDs.

Natural scalar unions in root or callback signature nodes qualify when every
member is an ordinary offset0 scalar, with zero bit offset/width and storage
width equal to its natural width/alignment. The maximum member width must equal
the union width/alignment. Member count is bounded 1..1024; reorder and repeated
alternatives do not change the recipe. No member arrays, nested aggregates,
callbacks, packed or over-aligned layouts qualify. Mapped originals carry
support0 and remain byte-for-byte untouched; complete carrier graphs carry proof1.

| Profile | Integer-only | Homogeneous FP-only | Integer+FP |
| --- | --- | --- | --- |
| osx/x86_64, lnx/x86_64 | u8/u16/u32/u64 | float4/double8 | width8 -> u64 |
| win/x86_64 | u8/u16/u32/u64 | width4/8 -> u32/u64 | width8 -> u64 |
| osx/arm64, lnx/arm64, win/arm64 | width8 -> u64 | float4/double8 | width8 -> u64 |

ARM integer-only widths1/2/4 remain rejected: non-HFA aggregate arguments can
be rounded to a 64-bit carrier, while scalar narrow integer extension and
Darwin stack packing follow different rules. A same-width scalar certificate
has not established that complete boundary. FP-only unions mixing float4 and
double8 alternatives remain rejected in this slice, even when an ABI
might permit another recipe. Mixed integer+FP8 allows arbitrary natural integer
widths and both FP formats, since the integer alternative prevents HFA/SSE-only classification. The former double8+unsigned64 mapping
is unchanged on all six profiles.

Primary classification references (read before implementing these finite rules):

- [Microsoft x64 calling convention](https://learn.microsoft.com/en-us/cpp/build/x64-calling-convention?view=msvc-170): small unions use integer passing and return registers, unlike FP scalars.
- [AAPCS64](https://github.com/ARM-software/abi-aa/blob/main/aapcs64/aapcs64.rst): homogeneous FP aggregates use the FP register class; overlapping union alternatives count uniquely addressable members.
- [Microsoft ARM64 conventions](https://learn.microsoft.com/en-us/cpp/build/arm64-windows-abi-conventions?view=msvc-170): fixed HFA and ordinary composite rules.
- [LLVM AArch64 classifier](https://github.com/llvm/llvm-project/blob/main/clang/lib/CodeGen/Targets/AArch64.cpp): Darwin, AAPCS and Win64 fixed HFA paths, and composite argument width rounding.
- [LLVM X86 classifier](https://github.com/llvm/llvm-project/blob/main/clang/lib/CodeGen/Targets/X86.cpp): SysV class merging gives INTEGER priority over SSE.
- [LLVM homogeneous aggregate implementation](https://clang.llvm.org/doxygen/ABIInfo_8cpp_source.html): union member count is the maximum of overlapping alternatives, and the fundamental FP type must match.

Primitive scalars and ordinary opaque pointers remain identity carriers; void
is result-only. Variadics, unknown profiles and malformed graphs reject without
an accepted partial certificate. These rules describe model classification;
they do not claim native qualification on all six targets.

The stage is integrated into the development package and paired registry.
Actual public native qualification of these natural scalar union rules is
recorded separately in research/r10-natural-union-public-evidence.json.
Six rule rows are not evidence of six native platforms passing; general
aggregate/bitfield/wide-FP and BANK support remain unfinished.

Fixed callback graphs retain kind4/tag4 pointer descriptors and schema1 identities.
Definition IDs are registered before children, so shared and cyclic signatures
remain shared/cyclic. Every completely certified fixed node carries proof1;
original graph bytes/proof bits remain untouched. New local banks480–482 own
signature IDs, epoch marks and 32-slot frames; recursion is bounded at64.

## Natural recursive aggregates

Ordinary struct(tag1) and array(tag3) descriptors retain exact size/alignment,
member offset/bitfield/storage words, count and stride. Struct offsets must equal
sequential natural align-up placement; aggregate alignment is the maximum member
alignment and its extent is the rounded final member end. Arrays require stride
equal to element extent, extent equal to count*stride, and element alignment.
Counts1..1024, alignment1/2/4/8 and extent<=1MiB bound this slice; void children,
nonzero bitfields, packed/extra-padding/over-aligned layouts and unsupported
children reject. Existing bank482 uses 32-slot frames for recursive emission.

The local replacement argument is limited to these natural layouts. On SysV
x64 each certified union preserves its eightbyte INTEGER/SSE class and byte
extent; enclosing natural structs/arrays therefore merge the same classes,
while larger aggregates remain MEMORY. On ARM, homogeneous same-format FP unions
contribute one uniquely addressable FP element both before and after mapping;
arrays/structs preserve HFA multiplicity. Integer/mixed unions remain non-HFA
with unchanged extent/alignment. Narrow integer ARM unions remain rejected.
On Windows x64 small aggregates use extent-based integer passing and larger
aggregates remain indirect; original and carrier extents are identical.
Fixed callback descriptors remain pointer-sized and their signature graphs are
certified recursively. These arguments do not cover arbitrary packed layouts,
bitfields, vector types or variadics. Natural fully occupied16-byte unions are
qualified in the following section; that does not establish arbitrary padded,
packed or over-aligned16-byte unions.
They are model-domain proofs, not all-platform native qualification.

## Natural16-byte union domain

`NL.walk` recursively validates ordinary int/data-pointer/float leaves, natural
struct/array placement and overlapping union alternatives. Returned occupied,
INTEGER and FP byte masks cover exact relative offsets, then parent offsets shift
the masks before merging. Every alternative must occupy its entire extent; holes
and padding are explicitly outside this first domain. A union alternative must
have the union extent; all alternatives together establish its natural maximum
alignment. Callback object leaves reject even if overlapping integer alternatives
would dominate their lane class. Existing callback signature topology is unchanged.
Classifier recursion<=32 uses bank482 keys100000+depth*32, isolated from existing
NC 32-slot emission frames; its return masks are outside saved scratch.

`rules.tsv` declares six profile families, leaf classes, N<S<I joins, and eight
natural recipes. SysV16B union alignment8 emits two64-bit elements with the
computed II/IS/SI/SS classes; alignment4 emits four32-bit elements, repeating each
lane class twice. ARM homogeneous FP base8/base4 emits double2/float4 respectively;
any integer leaf or differing FP bases makes the object non-HFA, using GP II
aggregate recipes. In particular `{double[2],float[4]}` is SysVSS but ARMII.
Windows x64 retains an extent16 aggregate (II recipe), preserving indirect
parameter and sret identity. All recipes retain root alignment4/8, width16 and
one logical object; no numeric conversions, artificial ffi layout fields,
new executor primitives or C ABI classifier are introduced. Broader16B layouts,
SSEUP/x87/vector classes, holes and general union extents remain rejected.

## Complete ordered V3 facts

Complete natural origin1/2 facts (known-mask3, no modifiers, known natural alignment) can produce a V2 struct carrier while retaining the untouched V3 original. Actual occupied bit intervals, zero-width barriers, container reuse and ordered anonymous aggregates are retained. SysV anonymous lanes qualify only where the two supported policies agree. Incomplete parser origin0, packed/over-aligned layouts, native long-double and general BANK remain outside this certified domain. See [the wire/certification contract](../c/librarysignature-v3.md), `tests/modelnativebitfieldcheck.py` and the measured two-ISA native/closure checks in `tests/modelorderedbitfieldnativecheck.py`. These explicit external-layout fixtures do not prove public source refinement or all six platforms.
