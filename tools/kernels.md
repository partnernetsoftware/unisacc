# Native kernel construction

The native path uses the current product's C preprocessor, the extended existing
C assembler through its CX adapter, and the CX ELF linker. No Clang, LLVM linker
or Python is used by these steps. Both architectures' eleven units completed
construction and linking in generation `out/c43-native-kernel-03`: x86_64 produced
a 7144-byte UNIKERN1 blob and ARM64 a 7096-byte blob. This establishes construction,
not runtime correctness. The path is being qualified independently before
wiring it into the original shared-model builder, which still has its old
external assembly dependency. A development product using these fresh kernels
and five rebuilt C driver slices was installed at the repository root on
2026-10-25 23:06. It is not a fully qualified release.

Run each command separately from the repository root, with a new output directory:

```sh
tools/bound 55 sh ./unisacc.com tools/kernel-build.cx -- "$PWD" "$PWD/out/kernel-fresh" bootstrap
tools/bound 55 sh ./unisacc.com tools/kernel-build.cx -- "$PWD" "$PWD/out/kernel-fresh" prepare
tools/bound 55 sh ./unisacc.com tools/kernel-build.cx -- "$PWD" "$PWD/out/kernel-fresh" prepare-assembler
tools/bound 55 sh ./unisacc.com tools/kernel-build.cx -- "$PWD" "$PWD/out/kernel-fresh" preprocess-x86_64
tools/bound 55 sh ./unisacc.com tools/kernel-build.cx -- "$PWD" "$PWD/out/kernel-fresh" assemble-x86_64
tools/bound 55 sh ./unisacc.com tools/kernel-build.cx -- "$PWD" "$PWD/out/kernel-fresh" link-x86_64
tools/bound 55 sh ./unisacc.com tools/kernel-build.cx -- "$PWD" "$PWD/out/kernel-fresh" preprocess-arm64
tools/bound 55 sh ./unisacc.com tools/kernel-build.cx -- "$PWD" "$PWD/out/kernel-fresh" assemble-arm64
tools/bound 55 sh ./unisacc.com tools/kernel-build.cx -- "$PWD" "$PWD/out/kernel-fresh" link-arm64
```

Every child is also bounded. Sources must remain fixed across steps; a changed
source/bootstrap/tool context is refused. Preprocessor outputs and objects have
content receipts checked before consumption and after the step. Material source
adaptation is verified around assembly. Failed outputs remain for inspection;
their existence is never a completion record. Use a fresh generation after a
source change or partial failure instead of silently promoting old files.

Both source normalization and assembly process all eleven units of one ISA in a
single invocation. Per-unit macro/output state is discarded; previous successful
outputs remain if a later pair fails. The first full x86 attempt found a missing
index-only SIB addressing form, which was added with independent byte/boundary
oracles and decoder support. The extended contract batch passed 189 assertions,
including byte-exact object disassembly/reassembly checks. Both complete real-unit
construction steps subsequently passed; native runtime qualification is separate.

An earlier coordination step produced all eleven objects and completed its
post-assembly verification, then hit the outer 55-second bound during final
identity work. That run is not a completed step. The bootstrap step now builds
the existing identity program once with the product at -O2. Before accepting it,
the source identity obtained by default-run and by the compiled helper must
match. Every later use hashes the helper before execution and binds it into the
full source/tool/bootstrap context; source and artifact checks were retained.
This is a measured construction-hotspot cache, not a test-result cache or the
default way to execute CX logic.

Generation 04, built by the new development product, exposed the same remaining
loop cost: all eleven objects and both material verifications finished, but the
outer 55-second bound expired during final identity work. That generation has
no completed object receipt. Independent phase timing measured source identity
at 1.172s, material verification at 7.069s, and product compilation of the
assembler at 23.809s. `prepare-assembler` now constructs that executable once,
binding its exact source, product/source/tool context, flags and artifact SHA.
Both ISA assembly steps check the cache before and after execution, retain both
material regenerations, and retain full source/input checks. The separate cache
construction checks regenerated source before compiling and its exact digest
plus source context afterwards. This avoids repeated compilation; no functional
result is cached or accepted from the failed generation.

Generation 05's cache construction completed in 38.824s. Its x86 assembly step
then completed in 22.389s with all eleven objects and both material verifications,
instead of reaching the 55-second bound as generation 04 did. The largest action
object matched generation 03 byte-for-byte; the full rebuild comparison remains
a separate test. The 38.824s cache preparation is a cold construction cost and
must not be hidden in the faster per-ISA number.

`tests/assembler-cache.cx ROOT GENERATION ABS_NEW_PRIVATE_DIR` copies only the
required construction material, corrupts the private executable and checks a
named digest refusal before any of the eleven object outputs is created. Both
checks passed in generation 05 (18.606s); live generation inputs were untouched.

The new x86 kernel accepted a real E4 model and produced the same 592 output
bytes as the prior executor. The rebuilt Linux driver and refreshed model
package then compiled a complete return-7 program to the identical ELF image,
which actually returned 7. The packaged candidate passed default-run smoke (16),
extended assembler contracts (189), the full eighteen-net gold batch, and the
installed root product passed the 551-assertion owned-module batch. These are
local development checks; ARM execution, all native hosts, the new complete
acceptance list and the new same-source bootstrap fixed point remain unproved.

The linker module owns all working storage for one call and returns typed status
and image metadata; no internal mutable image pointer escapes. Its input paths
and diagnostic buffer are borrowed C ABI arguments. It validates little-endian
ELF64 ET_REL, allocated section extents/alignment, symbol and relocation tables,
unit-local/global binding, entry region and zero service slot. Supported relocation
types are x86 PC32/PLT32 and A64 JUMP26/CALL26/ADRP/ADD_LO12/LDST64_LO12.
Unsupported semantics, overlapping patches, duplicate or missing globals, bad
addresses/scales and input-budget overflow are refused before output creation.
The limits are 256 units, 64MiB cumulative inputs and 64MiB final image.

Run the independent synthetic contract batch in one default-run invocation:

```sh
tools/bound 55 sh ./unisacc.com tests/kernel-link.cx -- batch "$PWD/out/link-check-fresh"
```

Its sixteen assertions passed: full x86 and ARM fixture blobs, output preservation,
and malformed header/extent/string, missing/duplicate symbols, bad entry/target,
duplicate relocation, bad scale and directory-input refusal. These are synthetic
contracts, not proof of all relocation success paths or native kernel execution.
Production files are created exclusively; write failures retain an invalid partial
file and return an error. Preparation is reported as PREPARED, never functional PASS.
