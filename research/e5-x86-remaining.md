# E5 x86_64: what is migrated and what remains (2026-09-26)

This is a read-only summary against the dispatch of `unisa/emit_x86.py` `encode`, as cdx asked, so the next step can be chosen from the whole route. Nothing here is implemented beyond what is marked "migrated". Updated through the integer division and FP slices.

A few terms used in the table:

- **Migrated** means that exec/enc/gen.py encodes the op and it equals the reference (unisa/assemble with emit_x86, with every instruction encoded) on the hand-written fixtures, on both executors.
- **Layout** means the op's bytes depend on addresses from image.layout (text_va, data_va, the DATA_BASE shift) or on imports. Those belong to E6, or need an external layout input to be declared first.

| op (emit_x86 branch) | status | rule source | layout? |
|---|---|---|---|
| mov | migrated | hand (0x89 + REX/ModRM); NUM read | no |
| imm (both widths) | migrated | hand | no |
| add64 sub64 xor64 and64 or64 | migrated | ENCSPEC alu2 read; the alias is hand | no |
| shl64 shr64 lshr64 | migrated | ENCSPEC shiftext read; the rcx save/restore is hand | no |
| mul64 | migrated | hand (0F AF) | no |
| load64 store64 .ld .st | migrated | hand (mem forms, SIB, disp8/32) | no |
| slt64 sle64 eq ne ult64 ule64 | migrated | ENCSPEC setcc read; cmp/setcc/movzx is hand | no |
| ret | migrated | hand (C3) | no |
| jump jumpz (+ relaxation) | migrated | hand; relaxation as assemble.py | no (in-section labels only) |
| call L (rel32) | migrated | hand (E8) | no (in-section) |
| callr | migrated | hand (FF /2, REX.B) | no |
| push pop | migrated | hand (50+r / 58+r, REX.B) | no |
| nop | migrated | hand (90) | no |
| .frame | migrated | hand (alu_imm: sub/add by |n|, imm8/imm32); the SP register read from REGMAP x86 r7 | no |
| .div .mod .udiv .umod | migrated (non-stack tape registers) | hand sequence; shared ALU/MEM encoders, REGMAP/NUM read | no |
| .zero | migrated | hand (xor r11 once; the widest store 8/4/2/1 that fits, via the migrated store forms) | no |
| FP_OPS (24 arithmetic/compare/conversion/sqrt forms) | migrated (non-stack tape registers) | opcode declarations read; SSE packing and conversion algorithms hand-generated in fp.py | no |
| setreg (imm/reg) | migrated | hand | no |
| setreg (addr/mem), setmem, .lea | not migrated | hand (rip-relative) | **yes**: text_va, the shift, syms |
| itoa | not migrated | hand (a fixed digit loop) | **yes**: data addresses |
| gate, non-winapi form | migrated | hand (0F 05 syscall; on Darwin with `carry`, jnc +3 / neg rax) | no |
| gate, winapi form | not migrated | hand (_winapi) | **yes**: imports, text_va |
| spinit with no second operand | migrated | hand (mov rN, rsp) | no |
| spinit with a second operand (Windows) | not migrated | hand (rip-relative lea) | **yes** |
| argsave, argvget | not migrated | hand (SysV/Mach-O entry) | **yes**: data cells |
| winsave, winrest, winargs, winstdh | not migrated | hand | **yes**: win data and imports |

(Corrected after cdx's review: the argument set-up and the data cells are lowering's and other ops' business, not a dependency of the gate's own encoding.)

**No remaining dispatch branch is address-independent:** non-WinAPI gate is now covered, including the Darwin carry handling. This does not mean arbitrary operands or metadata are all covered.

**Other unmigrated forms need addresses.** It should come after a declared, external layout input (text_va, data_va, the shift, symbol addresses and imports, given as input and not taken from the reference's final bytes), or together with E6.

**Beyond this table:**
- Lowering (tape to TIns: lower.py, with the enc/abi/reloc/regmap tables) is untouched.
- arm64 (emit_arm.py and its lowering peepholes) is untouched.
- Images (ELF, Mach-O, PE) are untouched.

## Integer division update

The four division/remainder forms share one delta path for saving rax/rdx on
the tape stack, encoding div/idiv, and restoring them. This remains a hand
algorithm in the generator, not a new constructed truth table. The executor
is unchanged. Declared operands are REGMAP x86 registers r0..r6; rsp and scratch
registers are outside this slice. Six persistent alias fixtures and one hand
byte sequence join the gate. A one-off exhaustive byte comparison over all
4 × 7³ = 1,372 op/register combinations matched both executors to the reference
(59,682 bytes); this is encoding evidence, not execution of division-by-zero
or overflow cases, and not a C semantic proof.

## FP update

All 24 FP_OPS now generate delta transitions using shared SSE/GPR packing.
This moves the hand algorithm into a generator; it does not remove it or
construct a network. No executor action was added. The persistent fixture
covers all operations. A one-off exhaustive comparison of 5,292 legal
op/register combinations gave 107,198 identical bytes on both executors.
The fixed gate additionally executes the generated machine code against host
C arithmetic and conversions: 550 cases, zero differences on macOS x86_64
through Rosetta. It includes unordered comparisons and high unsigned
conversions, but is not exhaustive over floating values or rounding modes.
Other than macOS and Linux x86_64, execution is explicitly reported NOT RUN.
Delta: 723 states, JSON 3,697,112 bytes; the product .com remains unchanged.

## Non-WinAPI gate

The delta emits syscall and conditionally jnc/neg rax for Darwin. A fixed hand
byte fixture checks carry=true, false, then omitted (per-instruction reset).
`x86-gate.txt` retains all gate instructions and metadata extracted from real
hello lowering for Linux and macOS x86_64. Both executors match reference bytes.
This does not execute syscalls. WinAPI and non-boolean carry remain rejected.
Other known gate metadata is ignored only because the non-WinAPI reference
encoder does not consume it. No executor action added.
