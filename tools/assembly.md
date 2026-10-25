# Native assembly development

The current product already has C preprocessing and an ELF assembler. This work
extends that implementation to build the repository's handwritten kernel sources
without an external Clang/Mach-O assembler. Full native kernel construction is
still incomplete: AArch64 forms, alignment, jump tables and cross-unit linking
remain required. Reusing old kernel blobs is only a comparison aid.

Run the native source adapter on individual units:

```sh
tools/bound 30 sh ./unisacc.com tools/asm-source.cx -- "$PWD" x86_64 exec/c/asm/transition_x86_64.S out/transition-native.s
tools/bound 30 sh ./unisacc.com tools/asm-material.cx -- "$PWD" "$PWD/out/asm-fresh"
tools/bound 30 sh ./unisacc.com tools/asm-material.cx -- --verify "$PWD" "$PWD/out/asm-fresh"
tools/bound 55 sh ./unisacc.com out/asm-fresh/contracts.cx
tools/bound 40 sh ./unisacc.com out/asm-fresh/assembler.cx -- x86_64 out/transition-native.s out/transition-native.o
```

All output files/directories must be new. asm-source uses the product's -E, with
the same import renaming as the original kernel build. Its CX logic expands the
supported GAS macro forms, resolves numeric local labels and normalizes selected
Mach-O section/relocation spellings. Unknown encodings remain the assembler's
responsibility; PREPARED source is not an accepted object or a functional PASS.
Three actual probes passed: transition x86_64 and bridge on both architectures;
both bridges expand nine SERVICE macro calls.

asm-material preserves src/asmtext.c bytes except its single internal type include
and supplies the existing internal integer aliases and output adapter. This is a
source-preserving bridge to the current C core, not a claim that the core was
rewritten in CX. Generated assembler.cx is a standalone root; contracts.cx uses
the same core snapshot and the handwritten tests/assembler.cx batch. Receipts
bind actual inputs, bootstrap product and both outputs; --verify is read-only.
No handwritten new CX contains an include directive.

The x86 extension adds needed width variants, memory direction, conditional moves,
immediate shifts and indirect calls/jumps while retaining the existing preferred
encodings. Handwritten ISA byte oracles and invalid-operand checks passed 43 local
assertions in one default-run batch, including an existing AArch64 baseline.
The newly encoded transition unit is an x86_64 ELF relocatable object. No external
C compiler or assembler is used in these commands after the existing watchdog is
available. This is not yet evidence for all units, full decoder round trips,
linking, native kernel execution or a freshly rebuilt root product.
