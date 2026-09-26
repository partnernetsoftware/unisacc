# Assembly kernel migration

This directory implements **`core_transition` and the 32/64-bit arithmetic
primitives and byte-buffer append** by hand for AArch64 and x86-64 System V. Action dispatch, working
storage, allocation and cleanup are still the generic C kernel. The shipped product and default runtime still
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
arithmetic and buffer-append implementations and links their assembly symbols. It is a build adapter, not a
runtime fallback. `transitioncheck.c` separately retains the actual C body
under a different name for 537,620 comparisons per ISA, including independent
missing/domain/output expectations, signed limits and sums that would wrap a
32-bit accumulator into a wrongly valid result.

Run `../asmcheck.sh`, or `CORE_ASM_ARCH=x86_64 ../asmcheck.sh` on this ARM Mac.
Each job builds fresh models, runs the network/resource/error checks and all
six full-domain checks, produces four complete images through assembly
inference, compares them to the reference, and executes three against system
cc. The fourth image is the C runtime; executing it does not turn it into an
assembly action engine. Test Python constructs models and compares results;
no Python stage handles source at runtime.

Measured uncompressed __text on macOS (object section, no subtraction):

| ISA | transition | arithmetic | buffer append | remaining C (`cc -Os`) | sum |
|---|---:|---:|---:|---:|---:|
| arm64 | 332 B | 436 B | 208 B | 5,232 B | 6,208 B |
| x86_64 | 334 B | 450 B | 170 B | 6,562 B | 7,516 B |

Error strings are 58 B per transition, 24 B per arithmetic object and 39 B
per buffer object. Host/library/model costs remain outside this object sum,
accounted separately in ../CORE.md. The C-only baseline with the buffer
capacity guard is 6,508/7,530 B. The mixed sum grew from 6,128/7,439 B in
the previous slice; this is implementation migration, not a size win.

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
Allocation, other storage helpers and action dispatch remain C/libc.
