# CX controlled pipeline

`cx-pipeline.cx` consumes an immutable `CX0BND1` bundle. Its default action
executes the generated Linux x86_64 ELF and returns the program's status.
`--emit` prepares the ELF without executing it. O2 is explicit and currently
fixed. Other targets and ABIs are refused before any model executes.

The tool snapshots the bundle, seven stream executables, two adapters and
candidate into its new output directory. It verifies all seven model bodies,
formats and resources against the candidate's actual `image/O2` route. E2 is
also bound to the bundle's exact model-container SHA256. Each unit has a fresh
E2 and E1 process. The CX tool checks and strips `USLATTR1`, tags attributes with
the unit number, and frames the `UNITOK1` units. For multiple units the existing
`units` network handles static isolation and emits `UNITOK2`. It then invokes
E3, tapecheck, E4, prune, lower and elf explicitly.

Language rules remain in the existing models. `cx-pipeline-stage.c` is a generic
one-stage adapter over `exec/c/run.c`, restricted to the `units` and `tapecheck`
boundaries. No execution path calls the old complete compiler. The installed
product remains the bootstrap compiler for these new tools; the installed
driver has not been replaced by this developer entry.

The stage adapter is an internal consumer of the driver's private snapshots.
Its independent CLI rejects nonregular and initially oversized files, but its
legacy generic package reader is not an atomic reader of concurrently growing
files. The driver uses bounded exact reads before creating the snapshots; no
claim of hostile concurrent-writer isolation is made for the independent CLI.

Run from the repository root; use fresh output paths. Every command and its
children are bounded, and the harness invokes the pipeline under a shorter
child bound. The pipeline also has a 45-second monotonic total budget and
shrinks each child allowance to the remaining budget. Building a standalone pipeline here verifies its executable
format and permits the batch to reuse one compilation, without caching tests.

```sh
tools/bound 55 sh ./unisacc.com tools/cx-pipeline-stage.c -O2 -fno-trim-libc -o out/cx-pipeline-stage.elf
tools/bound 55 sh ./unisacc.com tools/cx-pipeline.cx -o out/cx-pipeline.elf
tools/bound 55 sh ./unisacc.com tests/preprocess/pipeline.cx -- "$PWD" "$PWD/out/cx-pipeline.elf" "$PWD/out/c43-streams-da4e-01" "$PWD/out/c43-cx0-e2-02.elf" "$PWD/out/cx-pipeline-stage.elf" "$PWD/unisacc.com" "$PWD/out/cx-pipeline-tests-new"
```

For an existing sealed bundle:

```sh
tools/bound 55 sh ./unisacc.com tools/cx-pipeline.cx -- BUNDLE STREAM_DIR E2_ADAPTER STAGE_ADAPTER PRODUCT NEW_OUT -- ARGUMENTS
tools/bound 55 sh ./unisacc.com tools/cx-pipeline.cx -- --emit BUNDLE STREAM_DIR E2_ADAPTER STAGE_ADAPTER PRODUCT NEW_OUT
```

The generated `receipt.tsv` records exact input/output hashes and `PREPARED`,
never PASS. Partial outputs survive failure for inspection; no successful
receipt is written on stage failure. The harness uses handwritten return-value
and attribute gold, including deleted live sources, module private symbols,
macro isolation, constructor execution, refusal/recovery and argv forwarding.
These local checks do not replace the full RI/resource, C99, platform or
release obligations. Detailed phase timing remains UNMEASURED.
