# Source-first CX driver

The driver accepts sources directly, inventories explicit resource roots,
calls `cxb_seal`, then runs the real CX model pipeline. It requires neither a
handwritten resource list nor a separate sealing command. The installed
compiler only bootstraps this CX tool; the driver does not invoke that compiler.

```sh
tools/bound 55 sh ./unisacc.com tools/cx-driver.cx -- --profile /absolute/local.tsv --out /absolute/new-output /absolute/main.cx -- application arguments
tools/bound 55 sh ./unisacc.com tools/cx-driver.cx -- --profile /absolute/local.tsv --out /absolute/new-output --emit /absolute/main.cx
```

The profile is `CXDRIVER1`, followed by tab-separated key/value lines, with a
final newline. All paths name existing absolute paths. Required fields occur
once; `ResourceRoot` may occur up to sixteen times. Comments start with `#`.
The following illustrates the fields; substitute actual paths and artifacts:

```text
CXDRIVER1
Target	lnx/x86_64
HeaderRoot	/absolute/unisacc/include
ResourceRoot	/absolute/application
Streams	/absolute/seven-stream-material
E2Adapter	/absolute/cx0-e2
StageAdapter	/absolute/pipeline-stage
Product	/absolute/candidate.com
Pipeline	/absolute/pipeline
```

`HeaderRoot` contributes H resources under names relative to that root.
`ResourceRoot` contributes F resources under their actual canonical file paths,
so a quoted include beside a canonical source resolves without reading the
live filesystem during E2. The caller selects these roots; the driver does not
guess a build directory, find system headers or infer ambient include paths.
Module discovery remains owned by CX0. Module sources outside the extra roots
can be observed by CX0; ordinary headers must be in the declared roots.

Traversal sorts names, rejects symlinks and other nonregular resources, and
limits resources to 2,048 files, 4,096 directory entries, depth 24, 16 MiB per
file and 32 MiB total. Explicit sources are limited to 64 files, 4 MiB each and
16 MiB total; CX0 independently checks discovered units. Output must be new and
outside the input roots. Exact bounded reads check file extents before and
after reading; this is not an atomic snapshot against hostile concurrent writes.

The driver has a 45-second monotonic total budget. Sealing runs in an isolated
process group that the driver kills on deadline; pipeline execution receives
the remaining budget. The native outer watchdog bounds bootstrap compilation
as well. Unsupported targets, unknown or duplicate fields, unknown options,
missing modules and resource errors have named failures.

Outputs include the exact profile bytes, generated interfaces, `bundle.cxb`,
a private copy of the selected pipeline executable, and the pipeline's model
snapshots, stage outputs and receipt. The pipeline validates its actual seven
models and resources against the selected candidate. `--emit` prepares the
ELF; default operation executes it and preserves its arguments and exit status.
Current support is Linux x86_64/O2. Installation and broader qualification are
separate work.

The independent batch accepts `ROOT DRIVER STREAM_DIR E2_ADAPTER STAGE_ADAPTER
PIPELINE PRODUCT NEW_ABS_OUT`:

```sh
tools/bound 55 sh ./unisacc.com tools/cx-driver.cx -o out/cx-driver.elf
tools/bound 55 sh ./unisacc.com tests/preprocess/driver.cx -- "$PWD" "$PWD/out/cx-driver.elf" "$PWD/out/c43-streams-da4e-01" "$PWD/out/c43-cx0-e2-02.elf" "$PWD/out/c43-cx-pipeline-stage-02.elf" "$PWD/out/c43-cx-pipeline-02.elf" "$PWD/unisacc.com" "$PWD/out/cx-driver-tests-new"
```

The standalone executable in the batch verifies the actual output format and
allows repeated fresh executions after one compilation. Test results are never
cached. Detailed startup/construction/loading/inference/output/validation times
remain UNMEASURED.
