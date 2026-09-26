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
allow them. Section-local labels, jump/jumpz/call are supported. Data-address layout and images remain unsupported.

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

Current size: 822 states, 41,672 B compressed text table. The new gate entry is
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
executed by the small fixture harness. Windows and address-bearing forms reject.

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
Real hello lowering was inspected in full: address .lea/setmem/setreg mem,
argsave/argvget and payload layout still prevent whole-program ARM encoding.
No instruction filtering is presented as a successful real-program encoding.
