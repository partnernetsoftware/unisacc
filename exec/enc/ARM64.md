# ARM64 encoder migration

`arm.py` compiles integer encoding rules into a transition table for the existing
C executor and Python simulator. It reuses the state/action builder loaded by
`gen.py`; it does not run the x86 encoder or invoke the reference encoder.
No executor primitive was added. This is a lookup-table prototype, not a net.

Current instructions: mov, imm, ALU3 (add/sub/xor/and/or and register shifts),
mul64, six integer comparisons, nop, callr and ret; load64/store64 and .ld/.st
with 1/2/4/8-byte widths. ENCSPEC supplies ALU3 and
inverted-condition values. Bit packing, MOVZ/MOVK selection and tape-call
stack mechanics are hand rules in the generator. The parser validates arity
and operand kinds, decimal magnitudes before overflow, and x0 through x30.
It rejects x31/SP/ZR and callr x7/x17 (the call sequence changes both).
Memory opcode declarations are read from emit_arm LDS/STS/LDU/STU; form
selection and packing remain hand rules. A scaled positive offset is preferred,
then signed imm9, otherwise MOVIMM plus ADD/SUB through x16. Fallback rejects
an x16 base or store source, whose value would be clobbered. Direct forms
allow them. Section-local labels, jump/jumpz/call are supported. POSIX text/data address layout is supported; --elf also writes Linux ARM64 ELF.

`ret` pops x17 from tape SP x7 and returns through x17. `callr` stores its
continuation on that software stack before BLR. These are not host ABI calls.
The native arithmetic harness appends its own host RET; it does not execute
tape-call sequences without their software stack. Their byte contract is
checked against the reference and independent worked bytes.

Run `./exec/enc/armcheck.sh` (each command has a 60 s watchdog):

- 60 instructions / 384 bytes agree with emit_arm on both executors.
- 12 declared-domain failures reject on both, with no output.
- On macOS arm64, 540 arithmetic/comparison calls agree with independently
  assembled host instructions; ten immediate boundary values execute correctly.
- Other hosts explicitly skip native execution; byte checks still run.

Memory checks add 100 instructions / 768 bytes compared on both executors,
four width/scratch rejects, and 64 native load/store cases checked with C
memcpy and signed-width values.

Text mode: 981 states, 48,824 B compressed text table. The new gate entry is
exec-arm. This does not change .com or claim complete ARM64 lowering/encoding.

## Section-local branches

Two scans of the same input measure actual instruction lengths and then encode
resolved displacements. Pass one output is discarded; pass two checks label
positions are unchanged. Duplicate and undefined labels reject. Shared/end
labels are legal. Relocation widths and shifts are read from RELFIELD; the
signed-range check and two-pass algorithm are hand-written transition rules.
ARM has no branch shortening here. The call displacement is measured at the BL
(after the three software-stack setup instructions), not at the start of call.

armbranchcheck: ten layout fixtures on both runtimes, six independently worked
byte strings, seven rejects; a backward loop and a software-stack direct call
execute natively. Eight synthetic contexts enter the real displacement helper
to test signed imm19/imm26 edges; these do not claim full-size image coverage.
All earlier integer and memory checks remain in armcheck.

## Integer lowering forms

Added .div/.udiv/.mod/.umod, sext (1/2/4/8-byte source), addi/subi (0..4095),
lsli (0..63), and .frame. Division is SDIV/UDIV; remainder uses x17 plus MSUB
and rejects an x17 source, but allows x17 as destination. Frame operations
adjust software SP x7, using the immediate form below 4096 or MOVIMM into
x16 plus register ADD/SUB. These opcode/layout rules are hand-written here.

Apart from imm's full 64-bit bit-pattern domain, integer operands must fit
signed 64-bit. Unsigned positive values above INT64_MAX are not silently
reinterpreted as negative offsets. Width values are checked before a 32-bit
selector can truncate them.

armintcheck adds 28 reference instructions / 152 bytes on both runtimes,
ten signed-domain/field-width/scratch rejections, and 24 native functions
checked against C arithmetic and modular frame-address results. C division
checks exclude division by zero and signed MIN/-1; no claim about C UB is made.

## Floating-point forms

All 24 seed FP operations now emit through integer executor actions. Values
are bit patterns in GPRs, moved into v16/v17 for arithmetic and back afterwards.
FARITH/FCMP_INV/FP_OPS are read declarations; the transfer, conversion and
packing sequences are hand rules in armfp.py. No native FP executor action or
runtime call to the reference encoder was added.

Byte coverage: 72 operand/alias combinations, 960 bytes, both executors.
Numerical coverage: the shared fpcheck.py harness runs 550 C comparisons on
macOS arm64 (arithmetic, NaN/Inf comparisons, signed/unsigned conversions,
precision changes and square root). Float-to-integer checks use representable
inputs. The same 550-case harness and all 48 x86 encoding fixtures were rerun
successfully after sharing the harness. These are finite test cases, not a
proof over all floating-point bit patterns or rounding modes.

Cross-target UB note: ARM integer division returns zero on zero divisor and
MIN on signed MIN/-1; x86 IDIV traps. Neither is a C-defined-input equivalence
obligation. The native C integer referee excludes these cases.

## TIns setup and metadata

setreg imm/reg uses the existing immediate/move rules; spinit without a data
address copies host SP. Non-WinAPI gate emits SVC #0 or #0x80, optionally the
Darwin carry-to-negative-errno sequence. Syscall words are byte-tested, not
executed by the small fixture harness. Windows-specific setup forms still reject.

Declared metadata keys come from tins.META. Duplicate/unknown/empty fields
reject. gate/carry apply only to gate and are validated; gate form must be svc.
reloc must match jump/call arm26 or jumpz arm19. Other declared fields are
informational here (including Windows annotations carried on POSIX lowering),
not address inputs. Tags and operand types must agree. Seen-key stamps use a
monotonic instruction number across both passes, so repeated keys on different
instructions or on the second scan are not mistaken for duplicates.

Five whole fixtures compare with the reference on both runtimes; three worked
setup/syscall byte strings and seventeen invalid forms are checked. The earlier
role=a rejection becomes an unknown-key rejection, since role is now accepted.
The following address slice now covers the POSIX address forms used by hello/fib.

## POSIX payload addresses and real code

armlayout reads @target (lnx/arm64 or osx/arm64), @sym and payload headers.
Symbol addresses are decimal, bounded at 2^31-1; repeated declarations and
headers after instructions/labels reject. Text mode retains data headers opaque; --elf validates and decodes them in
the shared ELF writer. Text-only success does not certify image payloads.
Header processing happens only in pass zero; the second scan uses the retained
declarations. Code labels and data symbols use separate maps. As in emit_arm,
a data symbol takes precedence over a same-name code label.

The delta derives text/data virtual addresses from the measured text length
and ELF/Mach-O format constants. ADRP/ADD checks signed page range and uses the
actual output PC. .lea resolves names, setreg mem/addr resolves numeric data
addresses, setmem stores through x17, argsave handles entry-stack/register ABI,
and argvget loads through the declared argv cell. Scratch aliases that would
clobber a source reject. Numeric .lea spellings and Windows setup are not yet
claimed. No image-layout or encoding oracle runs in the executor.

realcheck now consumes **whole** hello/fib lowering outputs for Linux and macOS
ARM64. All code matches the reference: Linux 54,352 / 54,696 B; macOS 54,476 /
54,820 B. Both macOS images run and print the expected result. Python still
produces lowering and wraps the delta text in an image, so this is not the full
ARM route or a product replacement. The output ADRP/ADD pairs are independently
decoded and checked against declared data-symbol addresses. Four small layout
fixtures cover both OSes, page crossings and same-name precedence on both
executors; thirteen invalid header/address/alias fixtures reject.

Frozen gate --com: 52/52, JOBS=2, 196 s total, ARM suite 13 s. Subsequent
changes only strengthened address assertions; the complete ARM suite was rerun.

## Linux ELF mode

`arm.py OUT.json --elf` uses the same elfimage.py writer as x86: one shared
implementation for data hex, relocation, sparse zero tails, two PT_LOADs and
output. Generation parameters select e_machine and whether labels contain
instruction indices (x86) or byte offsets (ARM). A small little-endian field
writer now belongs to ELF itself rather than depending on an x86 helper.
ARM ELF requires @data; @target must resolve to Linux. Text mode is unchanged.

Relocation decimal indices are checked after each digit, preventing wrapping
of an oversized index before the final data bound check. Both architectures'
image checks cover this. `armimagecheck.sh` is in the gate as exec-armelf.
ELF mode: 1,079 states, 55,788 B table. No new executor action.

2026-09-27: generated hello/fib images equal the complete Python reference,
including headers/data: 57,654 / 57,646 B. They ran in the already-running
Lima default aarch64 VM with 10 s guest limits and 60 s host limits; exact stdout,
empty stderr and exit 0 were checked. SHA256:

- hello: 1f227471a8d2cb471fcc2eb8b7a2e90319266b982663be152138d3575937e2b2
- fib: d4efd5598e6d77f9c342189b350fad35287ad63d5a1af8171c86ac7a71e9181f

No VM was started or stopped. Python still supplies lowering input; this is
not source-to-ELF through ARM deltas yet. Mach-O/PE image output and the product
switch remain outstanding. Targeted regressions: ARM text/native checks, both
ELF checks, 610,000,200-byte sparse extent and Linux x86 self-source equality.
No new full-gate result is claimed for the newly added exec-armelf entry.


## Zero-fill and complete Linux ARM self-source

`.zero base,off,n` uses XZR stores in widest 8/4/2/1 pieces, reusing the
scaled/unscaled/fallback memory transitions. n must be nonnegative; signed
address-offset overflow and fallback base x16 are rejected. No runtime primitive
was added. 136 memory instructions (1,128 bytes) match the seed on both runtimes;
71 native memory cases include seven zero-fill cases, checked against memset.

The six-delta ARM route now also supplies __aarch64__ to E2 (it previously
supplied __x86_64__ regardless of backend). Its full compiler ELF is 716,458 B,
sha256 5e95f0dc9efbcbfacf3552d5e1b505a5fe9010b4cf57ecec8f828002c8bc07ef.
It equals ua_ref -O2 -b lnx/arm64 and executed N1=N2=N3 in the already-running
Lima default ARM64 VM. This rebuilds the existing C compiler, not E7 adoption.
