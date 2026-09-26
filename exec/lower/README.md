# Lowering through the generic executor

The data-only mode decodes raw E3/E4 `.str` and `.bss` directives, performs
zero-last layout and emits a data header followed by unchanged tape code.

```
python3 exec/lower/gen.py data.json
python3 exec/lower/gen.py arm-data.json --arm64
python3 exec/lower/gen.py full.json --full
```

`--full` also lowers Linux x86_64 instructions: register mapping, entry setup,
argument access, syscall setup and adjacent push/pop fusion. It consumes raw
tape, not a Python-lowered TargetProgram. `.print` is explicitly unsupported;
`--full --arm64` shares register mapping, entry setup and syscall lowering,
including the three Linux ARM *at argument shapes read from ABI facts. ARM
sext fusion and immediate fusion (including the bounded 32-instruction dead
register scan) run as ordinary delta actions. `armcheck.sh` compares 96 setup/
sext instructions and 131 immediate/liveness instructions on both executors,
then complete hello/fib on the C executor. The current compiler's optimized
tape also matches all 103,254 reference ARM target instructions, labels and
metadata. These checks establish measured agreement, not full equivalence.

`TARGET=lnx/arm64 ../pipeline/elf.sh OUT hello.c` selects ARM lowering and ELF.
hello/fib completed the six-delta source route and ran in native aarch64 Lima.
The compiler self-source reaches the ARM encoder but is rejected: it contains
`.zero`, which that encoder has not migrated yet. The generated table is still
a lookup table; Python generates it, but does not lower the source at runtime.

`data.py` and `code.py` compile hand algorithms into transition tables, not
integer networks. `code.py` reads regmap/enc/abi/reloc TSV facts; scratch
constants and tape shape declarations are imported at generation. No new
executor primitive was added. Python lower() is used only by the referee.

`check.sh` checks data layout, including real E4 output. `fullcheck.sh` checks
all instruction arguments and metadata, labels and data against the reference:
a fixed syscall/fusion/entry fixture on both executors, plus hello and fib on
the C executor. These are bounded examples, not full lowering coverage.

`../pipeline/elf.sh OUTPUT_DIR FILE.c...` runs E2, E1, E3, E4, lowering and ELF
encoding using the same C executor. Python generates the tables beforehand;
no Python stage processes the input source after generation. It is a developer
route, not the shipped `.com` implementation. Frontend coverage limits remain.

## Sparse zero storage

The data pass keeps virtual byte offsets separate from compact `.str` bytes.
`.bss` and alignment gaps advance the virtual extent without allocating zero
cells. Blob order and address mod 8 follow `zero_last`; symbols retain exactly
the same resulting addresses. The emitted `@data` is the initial byte prefix
through its last nonzero byte (`-` if empty), and `@data_len` is the full logical
extent, including the implicit zero tail. The Linux route requires `@bss 0`;
that separate extra-stack field is reserved for the Windows route.

ELF generation relocates against the logical extent, materialising a relocation
inside an omitted zero tail when required, then records logical length as
`p_memsz` and stored bytes as `p_filesz`. Referees compare implicit tails with
zero bytes rather than requiring the intermediate payload to contain them.
The layout currently uses signed 32-bit extent arithmetic: the input pass
rejects an extent which cannot also accommodate alignment and scratch storage;
the image reader rejects lengths above 2^31-1. Memory namespaces use wide keys
so large virtual offsets do not alias the other arrays. No executor action was
added, and the output is still computed by lookup tables, not networks.

`sparsecheck.sh` checks a 610,000,200-byte logical data segment with only one
stored byte and a 4,097-byte ELF on both executors. Its symbol addresses and
ELF program header are independently specified. `../pipeline/selfcheck.sh`
checks the complete current compiler source through all six deltas against the
reference optimized tape and Linux x86_64 image. Compiling the existing C
compiler through this route does not switch its implementation to the executor.
