# Assembly kernel migration

This directory currently implements **only `core_transition`** by hand for
AArch64 and x86-64 System V. Actions, working storage, allocation and cleanup
are still the generic C kernel. The shipped product and default runtime still
select C. This is not a completed assembly kernel or product switch.

Both routines evaluate the threshold network directly: initialize the two
signed 64-bit outputs, visit every threshold, activate `key >= threshold`,
accumulate its signed weights, and validate the result. Neither materializes
answers. The retained table-control path has the same sparse-stack/direct-byte
contract as C. Missing transitions and both error strings are preserved.

`layout.h` names the 64-bit-pointer/32-bit-int CoreModel offsets. Building
layoutcheck.c asserts each used offset and the struct size against core.h;
a changed ABI fails compilation. The assembly has no imported functions.
AArch64 uses only caller-saved x0–x17; x18 is untouched. x86-64 preserves
rbx/r12–r15 on every return. Windows object/calling-convention bindings are
explicitly not implemented. Mach-O native arm64 and Rosetta x86-64 were run;
the ELF assembler spelling is present but has not yet been run on Linux.

`cc.sh` builds an explicit development runtime: it omits the C transition
implementation and links the assembly symbol. It is a build adapter, not a
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

| ISA | assembly transition | remaining C core (`cc -Os`) | sum |
|---|---:|---:|---:|
| arm64 | 332 B | 6,104 B | 6,436 B |
| x86_64 | 334 B | 7,148 B | 7,482 B |

Each assembly object also has 58 B of error strings and no imports. Gzip-9 of
the two text sections is 278 B each, a reference only; it is not used by the
runtime. Host/library/model costs remain outside this object sum and are
accounted separately in ../CORE.md. The standalone C baseline remains
6,468/7,496 B. These tiny differences are not a claim of significant size or
speed improvement; the purpose is an independently checked assembly boundary.
