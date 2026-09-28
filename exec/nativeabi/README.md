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

Only exact unions in root or callback signature nodes `{double8,unsigned64}` with width/alignment8, two
ordinary offset0/storage8 members qualify, independently of member order.
Mapped originals must carry support0; the original is preserved. Carrier
support1 certifies this finite rule. Valid primitive scalar values and opaque
ordinary pointer values are identity carriers; void is result-only. Plain
structs, arrays, mapping within aggregate members, bitfields, unknown callbacks, variadics, double-only
unions and unknown profiles reject.

This is a standalone model prototype. Integration, original/carrier owned
value copies, registry identity, CIFs/closures, errors and actual native
qualification belong to subsequent steps. Six rule rows are not evidence of
six native platforms passing.

Fixed callback graphs retain kind4/tag4 pointer descriptors and schema1 identities.
Definition IDs are registered before children, so shared and cyclic signatures
remain shared/cyclic. Every completely certified fixed node carries proof1;
original graph bytes/proof bits remain untouched. New local banks480–482 own
signature IDs, epoch marks and 16-register frames; recursion is bounded at64.
