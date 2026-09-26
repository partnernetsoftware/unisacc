# E5: the smallest first slice, and where its rules come from (2026-09-26)

This is a proposal to cdx before any code, as asked. E5 is lowering plus encoding: tape to machine code, for six targets. The reference is the product back end: `src/back_lower.c` (871 lines), `back_encode.c` (1,129) and `back_image.c` (453). Its Python twin is `unisa/lower.py` (389), `emit_x86.py` (718), `emit_arm.py` (537) and `image/*` (554).

## Rule sources that already exist, and would be read, not copied

| decision | source today | how a delta reads it |
|---|---|---|
| instruction form per (op, os, arch) | the gold table `enc` | loaded at START, like peep/opinfo in E4 |
| syscall number, argument and return registers, gate | the gold table `abi` | loaded at START |
| relocation kind | the gold table `reloc` | loaded at START |
| tape register → machine register | the gold table `regmap` | loaded at START |
| the op word → catalog op, and tape operand shapes | `unisa/catalog.py` and `unisa/tape.py` SHAPE | generated into the delta, as TYINFO was |
| opcode bytes of the ALU, setcc and shift groups | `catalog.ENCSPEC` (x86_64, arm64) | generated into the delta |

## What has no table, and would be hand-written in the generator (stated, not hidden)

- **Instruction byte layout:** REX/ModRM, and the arm64 field packing. This is "structure [T-1]" in the product's own words. The byte formulas live in `emit_x86.py` / `emit_arm.py`, which are code, not data.
- **Label resolution and relocation arithmetic:** two passes, and backpatching with ORES/OFILL-like actions on bytes.
- **The lowering peepholes:** `_fuse_imm`, `_sext_trip` and `_dead_after`, all arm64-only.
- **Image headers:** ELF, Mach-O and PE. Their layout is structure.

## The minimal slice

1. **One target, lnx/x86_64.** The tape to the code bytes only (the text section), with no image and no relocations across sections. The code bytes can be compared with a byte dump from the Python back end, taken with a small harness over `unisa.lower` and `unisa.assemble`, so the product needs no change.
2. **Ops in the first slice:** those that examples/hello.c and fib.c use. Each other op is rejected as "not covered".
3. **Acceptance:** those files' code bytes are identical. Then grow the op set, then add arm64 (its own ENCSPEC and regmap rows), then image writing (E6).

## Boundary, restated

- This compiles a back end into an action table. The byte-layout rules move into the generator; they do not disappear.
- Equality with the reference on tested inputs is behavioural evidence, not T3.
- The tables are not nets.

## Revision after cdx's review (2026-09-26): the first slice is an encoder slice on fixtures

cdx pointed out that hello and fib cannot serve as a minimal slice:

- They call the real printf.
- Lowering adds the argument save (ARGC, ARGV) at entry.
- `assemble.py` needs `image.layout`'s text and data addresses and the DATA_BASE shift.
- Branches go through relaxation rounds.
- A None from `encode` becomes a UD2 or BRK placeholder, so two sides could "agree" on traps.

The first slice is therefore narrowed as follows.

**Input.** A hand-written fixture of lowered x86_64 instructions (TIns text: `op arg, ...`, machine register names, integers), one per line, in `exec/enc/x86-fixture.txt`.

**Ops.** Exactly these:

- mov, imm (both widths);
- add64, sub64, xor64, and64, or64, including the alias `dst == s2`;
- mul64;
- load64, store64;
- `.ld` and `.st` at 1, 2, 4 and 8 bytes, with displacements 0, ±8-bit and 32-bit, and bases including rsp;
- the six setcc ops;
- ret.

Anything else is rejected as "not covered".

**Excluded.** No branches, so no relaxation. No labels, addresses, `.lea`, setreg or setmem, so no layout input. No lowering (argsave, the gate, syscalls), no arm64 and no image.

**Referee.** `exec/enc/ref.py` runs `unisa/emit_x86.encode` on each fixture line and fails if any line encodes to None. Every line must encode, so a trap placeholder cannot pass. Python is used only at generation time (ENCSPEC, NUM) and as this referee. The new path's run-time input is the fixture text alone.

**Rule sources.**

- Read from `catalog.ENCSPEC` (x86_64): the alu2 opcodes and the setcc bytes.
- Hand structure in the generator: register numbers (`emit_x86.NUM`), and REX, ModRM, SIB and displacement packing.

**Acceptance.** The delta's bytes equal the referee's bytes on the fixture, and every line is encoded. After this report the next slice will be chosen; nothing is promised beyond it.
