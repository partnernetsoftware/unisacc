# Model library export roots

The host supplies complete E3 `USLSIG1` or `USLSIG2` bytes as resource `\0library/signatures`.
It does not filter linkage or build a root-name list. The prune delta validates
all records before processing tape, interns each name, and keeps every external
(linkage 0), defined (1) function as an extra root regardless of supported ABI.
Internal records do not become export roots. Existing main/init, address-taken
and data handling remain unchanged. A missing public code label rejects.

The fixed wire declarations are libraryroots-schema.tsv. Complete fields follow
parse2/libraryexports.md: header/count, bounded nonempty ASCII name, linkage,
defined, variadic, total count, six-field return descriptor, stored count,
stored parameter descriptors, supported. Count is at most existing NAMEMAX;
stored count must equal min(total,8). Boolean fields are validated, defined must
be 1, type class is 0..6, width is 0/1/2/4/8, unsigned is 0/1. Depth/base/shape
are opaque u64 facts rather than independently reclassified C semantics. Name
uniqueness and exact resource exhaustion are checked. Every LE64 read checks
remaining bytes using unsigned comparison; no truncated read or length addition
overflow can succeed. Bad input rejects, never invokes conservative FALLBACK.

Without the resource, original prune output is unchanged. libraryrootscheck.py
requires private prepared JSON/TBL/NET/core inputs and compares C network with
Python simulator, checks the complete finite net domain, keeps unsupported
public functions, removes an internal dead function, preserves init/address
roots, and checks all resource prefix truncations plus malformed fields.
This is export retention, not a claim that raw addresses support native calls.


## V2 framing and full parameter lists

`exec/c/librarysignature-v2.md` defines the versioned wire protocol. V1 remains
unchanged. V2 adds the explicit REGISTER/ALL_STACK mode, caps total parameters
at 1,024, and stores exactly that many descriptors (no eight-parameter truncation).
The mode agrees with the declared variadic bit and count; the delta validates
this relation rather than guessing an ABI at invocation time. V2 resources and
aggregate width/payload are bounded by 16 MiB.

Seven-word descriptor framing includes class, primitive width, unsigned bit,
and alignment (zero only for void/unknown, otherwise power of two <=65,536).
Primitives/pointers have empty payload/tag0; aggregate tags1/2/3 require a bounded
nonempty payload; reserved function tag4 accepts only an empty payload. After
checking the payload extent against remaining resource bytes with unsigned
comparison, prune skips it mechanically. **Prune does not validate recursive
members or claim a supported native ABI**: the typed host decoder validates
those layouts before execution. Unsupported public functions are still roots.

The independent checker includes a nine-parameter mixed FP/pointer/struct V2
record, array/union outer layouts, unsupported public retention, an empty V2
record set, 1,024-parameter and true-variadic positive framing, void/unknown/opaque
function-pointer declarations, V1 complete-prefix controls, V2 small-record complete-prefix and
large-record boundary truncations, invalid mode/count/class/width/alignment/tag,
forbidden primitive/function payload and payload-extent controls. Simulator and
constructed-network C executor must agree on every fixture. Without a metadata
resource the original manual prune fixtures are byte-identical to the reference.


### Actual V2 qualification (2026-09-29)

Private frozen source `/tmp/r10-sig2-prune-qktdly8l/src`: generated 497 states,
258 action sequences and 451 threshold units (32,576-byte network). The actual
C executor `--check-net` checked 127,970 observations: network equals table and
actions/strings are identical. `libraryrootscheck.py` completed with rc0 in
3.82 s: simulator/C agree, five no-resource manual fixtures equal the reference,
366 V1 and 289 V2 malformed resources reject, full nine/1,024-parameter metadata
and unsupported aggregate/function declarations retain public roots. Build and
qualification ran with outer 55 s and child <=20 s limits. Machine-readable
private receipt: `/tmp/r10-sig2-prune-qktdly8l/evidence.json`. No shared compiler,
root product, index, VM or committed reference was changed. This qualifies prune
framing/retention only, not the typed native call bridge or full recursive ABI.
