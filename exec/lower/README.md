# Lowering through the generic executor

The data-only mode decodes raw E3/E4 `.str` and `.bss` directives, performs
zero-last layout and emits a data header followed by unchanged tape code.

```
python3 exec/lower/gen.py data.json
python3 exec/lower/gen.py full.json --full
```

`--full` also lowers Linux x86_64 instructions: register mapping, entry setup,
argument access, syscall setup and adjacent push/pop fusion. It consumes raw
tape, not a Python-lowered TargetProgram. `.print` is explicitly unsupported;
other targets are not implemented here.

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
