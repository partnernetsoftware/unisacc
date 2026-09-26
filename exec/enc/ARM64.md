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
allow them. Labels, metadata, address layout, FP and images remain unsupported.

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

Current size: 313 states, 13,213 B compressed text table. The new gate entry is
exec-arm. This does not change .com or claim complete ARM64 lowering/encoding.
