# FFI_CARRIER prototype stage

`python3 exec/nativeabi/gen.py OUT.json` constructs a finite delta accepting raw
single-record USLSIG2 bytes and the explicit `\0cli/target` ASCII resource.
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
signature IDs, epoch marks and 16-register frames; recursion is bounded at64.
