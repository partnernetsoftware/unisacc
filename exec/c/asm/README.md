# Assembly kernel migration

This directory implements **`core_transition` and the 32/64-bit arithmetic
primitives, byte-buffer append, sparse memory, byte-string interning, blob
copies, resource caching, decimal field rendering and control/input stacks**
by hand for AArch64 and x86-64 System V. AArch64 also implements all 56 action
handlers and their dispatch in assembly. x86-64 action dispatch remains C.
The outer transition loop, initialization and cleanup remain C on both ISAs. Allocation remains libc. The shipped product and default runtime still
select C. This is not a completed assembly kernel or product switch.

Both transition routines evaluate the threshold network directly: initialize the two
signed 64-bit outputs, visit every threshold, activate `key >= threshold`,
accumulate its signed weights, and validate the result. Neither materializes
answers. The retained table-control path has the same sparse-stack/direct-byte
contract as C. Missing transitions and both error strings are preserved.

`layout.h` names the 64-bit-pointer/32-bit-int CoreModel offsets. Building
layoutcheck.c asserts each used offset and the struct size against core.h;
a changed ABI fails compilation. The transition assembly has no imported functions; arithmetic imports only
the non-returning core_host_panic hook for an invalid operation. Buffer append
uses realloc and the same panic hook; it preserves all callee-saved registers.
The transition uses caller-saved x0–x17; all routines leave x18 untouched. x86-64 preserves
rbx/r12–r15 on every return. Windows object/calling-convention bindings are
explicitly not implemented. Mach-O native arm64 and Rosetta x86-64 were run;
the ELF assembler spelling is present but has not yet been run on Linux.

`cc.sh` builds an explicit development runtime: it omits the C transition
arithmetic, buffer-append, sparse-memory intern, blob, resource, field-rendering and stack implementations and links their assembly symbols. It is a build adapter, not a
runtime fallback. `transitioncheck.c` separately retains the actual C body
under a different name for 537,620 comparisons per ISA, including independent
missing/domain/output expectations, signed limits and sums that would wrap a
32-bit accumulator into a wrongly valid result.

Run `../asmcheck.sh`, or `CORE_ASM_ARCH=x86_64 ../asmcheck.sh` on this ARM Mac.
Each job builds fresh models, runs the network/resource/error checks and all
six full-domain checks, produces six complete images through assembly
inference, compares them to the reference, and executes five against system
cc. The sixth image is the C runtime; executing it does not turn it into an
assembly action engine. Test Python constructs models and compares results;
no Python stage handles source at runtime.

Measured uncompressed __text on macOS (object section, no subtraction):

| ISA | transition | arithmetic | buffer | sparse memory | intern/hash | blobs/resources | decimal/fill | stacks | action dispatch | remaining C (`cc -Os`) | sum |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| arm64 | 332 B | 436 B | 208 B | 500 B | 508 B | 548 B | 272 B | 288 B | 1,968 B | 980 B | 6,040 B |
| x86_64 | 334 B | 450 B | 170 B | 473 B | 479 B | 513 B | 225 B | 275 B | C | 3,855 B | 6,774 B |

Error strings are 58 B per transition, 24 B per arithmetic object, and 39 B
each for buffer, sparse-memory and intern objects; blob/resource strings add 70 B and field rendering 26 B; stacks add 84 B; arm64 action diagnostics add 41 B. Host/library/model costs
remain outside this object sum, accounted separately in ../CORE.md. The C-only
baseline with the capacity guards and explicit memory/intern state is
6,396/7,106 B. These are migration measurements, not a performance claim.

## Word arithmetic contract

32-bit inputs are truncated to their low 32 bits; each result is sign extended
to 64 bits. Add/sub/multiply wrap, shifts mask the count to 5 bits, div/rem by
zero return 0, and MIN/-1 returns MIN or zero. 64-bit add/sub/multiply wrap,
shifts mask to 6 bits, and signed/unsigned division by zero returns 0 with
`*z=1`; all other operations clear that flag. Signed MIN/-1 is again defined.
These are machine primitives, not a claim that C source division by zero is
well-defined. x86 IDIV traps are explicitly avoided. Unknown operation codes
reach the same non-returning host panic as C.

arithcheck.c compiles the retained core.c helpers under test-only names. Both
ISAs pass 108,919 value/flag checks (edge pairs, deterministic random pairs,
independent expectations), plus 16 C/ASM invalid-operation invocations. The
real six-stage route uses assembly arithmetic as well as assembly inference;
the C-only baseline remains the default build and is tested separately.

## Byte buffer contract

Append stores the low byte plus its exact 64-bit attribute, and advances n.
Valid input has 0 <= n <= cap and arrays of cap elements. Empty buffers grow
to 256; subsequent growth doubles both capacities. Byte realloc precedes
attribute realloc, and either failure terminates through core_host_panic.
Capacity above INT32_MAX/2 is rejected before doubling; the same guard was
added to C to avoid signed overflow. There is no new configurable limit.

buffercheck.c compiles the actual C helper under a test-only name. A moving
allocator checks 70,000 appends across growth boundaries, all stored bytes and
attributes, truncation, reset and reuse. Six C/ASM fault invocations check
first/second allocation failure and capacity overflow, explicitly SIMULATED.
Both ISAs also pass the full existing network checks and four-image route.
Allocation remains libc; later sections cover the other migrated helpers.

## Sparse memory contract

CoreMemory carries keys, values, occupancy bytes, capacity and entry count.
The hash multiplies a 64-bit key modulo 2^64, shifts by 20 and masks by cap-1;
linear probing wraps. Absent keys read zero, including before allocation.
Writing zero still creates an occupied entry. Growth happens before insertion
when (n+1)*2 exceeds capacity, even for an overwrite; the first capacity is
65536 and later capacities double. All occupied entries are reinserted before
the old arrays are freed. Cleanup/reset remains in core_run, in C.

Both versions reject doubling whose eight-byte array would overflow the
64-bit storage extent. This is an explicit representation guard, not a new
model limit. The C implementation now receives one state pointer; both
assembly routines use its build-asserted layout. The selected C object imports
core_memory_get/set, whose definitions are in assembly, not C fallbacks.

memorycheck.c checks 140,000 keys through multiple growths, zero overwrites,
64 deliberately colliding keys wrapping at the last slot, signed/extreme
addresses, absent reads, full backing-array equality, and cleanup/reuse.
Eight C/ASM fault runs simulate each of three allocation failures and extent
overflow, checking the panic, allocation count and resulting state. Actual
execution remains macOS arm64 and Rosetta x86-64. libc calloc/free remain
external generic primitives, not model or compiler rules.

## Binary string interning contract

CoreIntern owns a power-of-two table of (byte pointer, int length, 64-bit ID).
Strings are compared by length and memcmp, not as NUL-terminated text. The
empty string has an allocated byte and is a normal occupied entry. IDs start
at one, are assigned only to a new string, and remain stable during rehash.
The first capacity is 1024; growth precedes lookup at half load, including
when the incoming string already exists. Rehash moves entry pointers; it does
not reallocate their bytes or change IDs. Callers supply nonnegative lengths.

The hash keeps the existing seed 1469598103934665603 and multiplier
1099511628211, with modulo-64-bit arithmetic. This is deliberately not labeled
as the standard FNV seed. Both implementations check the 24-byte-entry growth
extent, and convert the string length to size_t before adding the spare byte.
calloc, realloc, memcpy, memcmp and free remain library dependencies.

interncheck.c checks five fixed hash values, 20,000 distinct binary strings,
empty/prefix/NUL distinctions, owned copies, 16 colliding keys wrapping the
table, growth on a duplicate, reverse lookups after rehash and reset IDs.
Eight C/ASM fault runs explicitly simulate table/byte allocation failure,
rehash allocation failure and capacity overflow. The selected C object imports
core_string_intern; the assembly implementation calls its own core_hash_bytes.
The full route also checks prefix_members.c against cc and the independent
exit value 40. This E3 path was needed by the new core's ++t->n expression;
it reuses model member/index addressing, not a language-specific C primitive.

## Blob and resource ownership

CoreBlobs owns copies with stable zero-based IDs. Capacity starts at 64 and
doubles; doubling is checked before int overflow. The copied length is
nonnegative, and the spare allocated byte is not a promised NUL terminator.
core_run reserves ID zero for a missing resource before looking anything up.
CoreResources caches byte keys by length and content, including absent results.
On a miss it calls the host once: borrowed bytes are copied, malloc-owned bytes
are copied and freed exactly once, and absence is cached as zero. Empty but
present bytes get a nonzero blob ID. Cleanup/reset remains in C.

bytescheck.c uses the real C functions under alternate names and a moving
allocator: 300 blocks, 256 binary resource keys, reverse cached lookups,
borrowed/owned/empty/absent responses, input mutation and reset. Ten explicit
SIMULATED C/ASM failures cover blob-array/blob-byte and cache-array/key
allocations plus blob capacity overflow. The resource count overflow guard
requires INT32_MAX cache entries and was not exercised. The host contract
remains trusted for valid lengths and ownership flags.

Both actual ISA runs pass the full network checks and six-image route.
member_index_address.c independently expects exit 25: E3 must load a pointer
member before evaluating its subscript under address-of. Native C network
self-reconstruction and C ASan/UBSan network checks also pass. These are
migration checks, not a claim of smaller code or a completed assembly kernel.

## Signed decimal and reserved fields

core_decimal writes the signed 64-bit number without a trailing NUL and
returns its byte count. INT64_MIN is converted through its unsigned magnitude.
core_field_fill right-aligns those bytes with spaces inside a reserved output
span, retaining output length, capacity and every attribute. A narrow or
negative width returns field-overflow before destination access. Otherwise
negative/out-of-range offsets or a width larger than the remaining output
extent fail through core_host_panic before writing. The bounds use subtraction
after validating the offset, avoiding overflow of at + width.

formatcheck.c compares the actual C and assembly routines against snprintf
for 10,013 values and 250,325 fields; untouched bytes and attributes are checked.
Ten C/ASM bad-offset/width runs check failure before writes, including INT64_MAX.
Both real ISA jobs retain all network and six-image checks. C ASan/UBSan and
network-built C self-reconstruction also pass. The outer loop and cleanup remain C.

## Control stack and input-frame stack

CoreStack owns 32-bit symbols; capacity begins at 1024 and doubles. Empty pop
is fatal. CoreFrames owns 32-byte records, starting at 16 and doubling. A frame
borrows its byte and optional attribute arrays and stores signed 64-bit cursor
and end positions; pushing copies all four fields. The push source is external
to the backing array, because realloc may move that array. Frame pop preserves
the bottom record and is a no-op for zero or one record. Both capacity doublings
are checked before signed int overflow. Allocator failure is fatal; cleanup
of backing storage still belongs to the C run lifecycle.

stackcheck.c compares the real C helpers and assembly using an always-moving
allocator: 20,000 symbols and frames, all retained fields, full reverse pops,
bottom-frame preservation and reuse without allocation. Fourteen explicit
SIMULATED failure runs cover first/growth allocation failure for both stacks,
each capacity guard, and empty control pop. Both real ISA jobs preserve the
six-image route and five native comparisons. Native C network self-rebuild
and ASan/UBSan checks pass. No parser/compiler-specific primitive was added.

## Explicit action state and ARM action dispatch

CoreMachine holds registers, comparison result, buffers, both stacks, indexed
memory, blobs, interned strings, resource cache and borrowed model/result/input
references. Its 280-byte layout is asserted field by field. Working state is
now invocation-local rather than stored in C globals. The host concurrency
contract is unchanged. Decoded action numbers are asserted against core.h.

On arm64 cc.sh selects core_action and every primitive in assembly. A compile
error prevents selecting this action engine while silently keeping C primitive
implementations. The only remaining C work is the transition/step-limit loop,
initial ownership setup and final ownership transfer/free. On x86-64 cc.sh
explicitly keeps the C action engine, with the same state API.

actioncheck.c runs the actual retained C action body and ARM assembly on
independent states, checking all registers, byte/attribute buffers, frames,
control stack, memory, intern/blob/cache entries and halt/error results after
each action. Its 1,050 comparisons cover all 56 actions, signed/unsigned edges,
selector and missing-attribute cases, clipping, cached lookups and repeated
SWAP. Bad action rejects are checked independently. Primitive-specific tests
retain their independent expected values and allocation-failure controls.
A test caught a missing address-add instruction caused by a semicolon comment
in a preprocessor macro; the source now uses separate assembly lines.

Both ISA integration jobs pass six-image/five-native comparisons. C network
self-reconstruction and ASan/UBSan pass. This is not an assembly self-compiler:
the generated run.c image is still C, and the shipped product route is unchanged.
SWAP allocation failures are not directly injected by the action suite.
