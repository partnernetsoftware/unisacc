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

## Revision after cdx's review

- **Arg types.** Args also include `None` and bools. `spinit(sp, None)` is what lowering emits; this slice normalises a None second operand to "omitted", a stated normalisation that keeps the meaning. A non-None second operand is rejected. A None is never printed as a label.
- **Two kinds of address.** Addresses that lowering itself outputs (setreg's `addr`/`mem` values, the DATA_BASE-relative `syms`) are instruction data, printed as integers or names when their slice comes. `text_va`, `data_va` and the imports come from image.layout. They are external layout, which lowering does not provide, and the dump never claims them. No `@` layout header is implemented now.
- **Meta syntax.**
  - The form is ` key=value`, separated by spaces, after the last arg.
  - `carry` is a bool: `carry=0` is false, `carry=1` true, and nothing else is accepted.
  - `reloc`, when given, must be `rel32`; anything else is rejected, never overridden silently. The rel32 default for the old fixtures without meta is only a compatibility convention.
  - `role` is informational and ignored, and the file says so.
  - A duplicate key, an unknown key, or an encoding-relevant key this slice does not support is rejected.

**Scope of the round trip.** `tins.parse` rebuilds instructions, meta and labels. data and syms come back empty and the target is the default lnx/x86_64. This is therefore an instruction/meta/label round trip, not TargetProgram fidelity. roundtrip.py compares by type and value (True is not 1), and a controlled bool → int mutation makes it fail.

## Full lowering payload (2026-09-26)

`dump(tp, full=True)` adds @target, @data (hex or - for empty), @sym
(name, DATA_BASE-relative address), and the optional lowering attributes
@src_os, @data_len, @bss, @relocs (comma-separated offsets or -). These are
input data, not computed image addresses. Headers precede all labels/code;
duplicate, incomplete and unknown headers are rejected. The compatibility
`dump(tp)` form remains instruction-only for the current encoder fixtures.

The round-trip gate now compares all TargetProgram attributes and typed code,
including relocation lists and BSS, on three programs for all six targets.
This is host-side transport verification, not execution on six platforms.
The delta encoder does not yet consume these headers. Its next step is to
compute layout from relaxed text length and resolve address-dependent forms;
no reference-produced text offsets or encoded bytes are part of this payload.

Earlier statements that data/syms always come back empty describe the old
instruction-only interface. Carry syntax is true/false, not 0/1.

## ELF-output route

`gen.py OUT.json --elf` now constructs a delta that consumes the full lowering
payload and writes an ELF64 Linux x86_64 executable, with no host image wrapper
at execution time. It shares the text encoder, then decodes @data, applies
@relocs using the computed data shift, trims stored trailing zeros while
preserving p_memsz, and emits both PT_LOADs and padding. The algorithm and
format template live in `elfimage.py`; these remain hand-written generator
rules, not a constructed network. TargetProgram production is still Python.

`imagecheck.sh` is a separate <=60 s gate. Both executors check a synthetic
relocated pointer with small/large zero tails, with independent ELF field
expectations. C executor output for complete hello/fib equals the reference
ELF byte-for-byte. This run was on macOS: Linux execution is not claimed.
