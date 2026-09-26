# E5 x86_64: what is migrated and what remains (2026-09-26)

This is a read-only summary against the dispatch of `unisa/emit_x86.py` `encode`, as cdx asked, so the next step can be chosen from the whole route. Nothing here is implemented beyond what is marked "migrated". HEAD is after the callr slice.

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
| .div .mod .udiv .umod | not migrated | hand (cqo/xor rdx, idiv/div, rdx/rax save) | no |
| .zero | not migrated | hand (r11 cleared, widest stores) | no |
| FP_OPS (fadd64 … cvt*) | not migrated | hand (SSE via xmm0/1, movq) | no |
| setreg (imm/reg) | not migrated | hand | no |
| setreg (addr/mem), setmem, .lea | not migrated | hand (rip-relative) | **yes**: text_va, the shift, syms |
| itoa | not migrated | hand (a fixed digit loop) | **yes**: data addresses |
| gate, non-winapi form | not migrated | hand (0F 05 syscall; on Darwin with `carry`, jnc +3 / neg rax) | no |
| gate, winapi form | not migrated | hand (_winapi) | **yes**: imports, text_va |
| spinit with no second operand | not migrated | hand (mov rN, rsp) | no |
| spinit with a second operand (Windows) | not migrated | hand (rip-relative lea) | **yes** |
| argsave, argvget | not migrated | hand (SysV/Mach-O entry) | **yes**: data cells |
| winsave, winrest, winargs, winstdh | not migrated | hand | **yes**: win data and imports |

(Corrected after cdx's review: the argument set-up and the data cells are lowering's and other ops' business, not a dependency of the gate's own encoding.)

**What remains without a layout dependency:** the four divisions, .zero, the FP group, and setreg for imm/reg. These can be done as fixture slices exactly as so far.

**Everything else needs addresses.** It should come after a declared, external layout input (text_va, data_va, the shift, symbol addresses and imports, given as input and not taken from the reference's final bytes), or together with E6.

**Beyond this table:**
- Lowering (tape to TIns: lower.py, with the enc/abi/reloc/regmap tables) is untouched.
- arm64 (emit_arm.py and its lowering peepholes) is untouched.
- Images (ELF, Mach-O, PE) are untouched.
