# E5: the TIns input interface (proposal, read-only; 2026-09-26)

**Purpose.** Let real lowering output (`unisa/lower.py` TargetProgram) enter the encoder delta, instead of each fixture extending the syntax. This is not a serialisation framework, and no offset or machine code is ever precomputed by the reference.

## What a TargetProgram holds (`unisa/lower.py`)

- **`code`**: a list of TIns, each with `op`, `args` and `meta`.
  - Args are machine register names (str), integers, label names (str), and `setreg`'s tagged tuple `(kind, v)`, where kind is `imm`, `reg`, `mem` or `addr`.
  - Meta keys: `reloc` (jump/jumpz/call), `form` (from the enc table), `role` (setreg; informational), and `gate`'s `form`, `gate` and `carry`.
- **`labels`**: a name → code index map. More than one name may point at the same index.
- **`syms`**: a data symbol → address map (DATA_BASE-relative).
- **`data`**: bytes, plus the `os`/`arch` target.

## What the current fixture text can express

- One instruction per line, `op a, b, ...`, with register names and decimal integers; `name:` lines give labels.
- Negative integers work. There are no strings, no tuples and no meta.
- Several labels on one index are expressible, as consecutive `name:` lines.

## Minimal compatible extension

The existing fixtures stay valid unchanged.

1. **Meta as `key=value` after the args.** The keys are the ones the encoder reads: `reloc=`, `form=`, `carry=0|1`, `gate=`. Example: `gate form=syscall carry=1`. Keys that only inform (`role=`) are accepted and ignored.
2. **setreg's tuple as a tag word.** `setreg rdi, imm 5` and `setreg rdi, reg rsi`. The encoder routes these to the migrated `imm` and `mov` paths: the same bytes as `mov_ri` and `mov_rr` in `emit_x86`.
3. **`spinit` with one operand**, e.g. `spinit rsp`. The encoder routes it to `mov rN, rsp` on the mov path. `spinit rN, ADDR` stays not covered.
4. **Addresses are not instruction data.** They are external layout, one header block before the code, and only when a slice needs them: `@text_va N`, `@data_va N`, `@sym NAME ADDR`, `@import NAME ADDR`. The block stays empty until E6 or a declared layout slice. `setreg ... mem|addr`, `setmem` and `.lea` stay not covered until then.
5. **The producer is a small dump of a real TargetProgram** into this text. It prints only what lowering built, never an offset or a byte. Such a dump makes the lowered code of hello.c, say, a fixture for the encoder.

## Out of scope here

- gate's winapi form, the divisions, FP, and anything with addresses.
- Lowering itself (tape → TIns): it stays in Python as the input producer until its own slice.
