# Model package v1 / resource extension v2

This is a construction/development artifact for the generic runtime, also
used by the embedded development container below. It has not replaced the
shipped compiler CLI.

## Construction

`pack.py -o models.pkg ROUTE.tsv...` reads explicitly named manifests and
network files. A manifest has five TAB-separated columns:

```
route-name    stage-name    input-format    output-format    network-path
```

Blank lines and lines beginning with `#` are ignored. Network paths are
relative to the manifest directory. The four names use ASCII letters,
digits, `_`, `-`, `.`, `/`; they are identifiers, not paths to open.
Rows for each route execute in file order, including across manifests.
Stage names must be unique within a route. Adjacent declared formats must
match. Identical network bytes are stored once, regardless of their paths
or how many routes refer to them. No compiler rules are generated here.

The six-stage source/image construction uses `pipeline/image-stages.tsv`
as its ordered format declaration. `pipeline/elf.sh` qualifies those rows
with the selected target, writes `OUT/route.tsv`, and builds `OUT/models.pkg`
after the ordinary full-domain network checks. Multiple such route manifests
can be packed together; the generation commands remain construction tools.

## Encoding

All directory tokens are ASCII; integers are decimal. Body lengths count raw
bytes, including each network's terminal newline:

```
P 1 NUMBER_OF_MODELS NUMBER_OF_STAGE_ROWS\n
D ROUTE STAGE INPUT_FORMAT OUTPUT_FORMAT MODEL_INDEX\n
... one D line per stage row ...
M BYTE_LENGTH\n
<exactly BYTE_LENGTH bytes of one N-format network>
... one M header/body per unique model ...
```

Model indices are zero based. No padding or trailing bytes are allowed.
The format is independent of target byte order and pointer width. Version 1
uses the existing `N` network/action encoding, unchanged. Changing its action
ABI requires a corresponding package/version decision; the format is not an
unversioned promise that arbitrary future runtimes can read old actions.

## Execution and checks

```
run --bundle models.pkg ROUTE INPUT [SRCPATH] [ABSOLUTE_INCLUDE_DIR]
```

The C runtime reads the one package, validates counts, indices, names, format
edges and body extents, then uses the existing network loader and executor.
There is no Python in this execution path. Each stage receives only accepted
bytes, with fresh machine state. All routes can share the retained package
bytes; only the selected route's active model is decoded at a time.

A missing route or malformed package fails with status 2. A stage rejection
or step limit retains its status and diagnostic and stops the route. No
partial accepted prefix is written to stdout. Output is checked through the
same final fwrite/fclose path as single-model and `--chain` execution.
Declared-format agreement does not prove a model implements its declared
format; the consumer's parsing, stage comparisons and semantic gates remain
necessary. The package supplies no authenticity or cryptographic signature.

`netcheck.py` checks shared bodies, route selection, construction/runtime
format checks, duplicate stages, invalid indices and truncated bodies.
`nativecheck.sh` compares packaged runtime self-reconstruction in cc-,
unisacc- and network-built runtimes. `pipeline/selfcheck.sh` compares the
packaged full compiler source against the separate-stage image for each
network target. These are measured evidence, not T2/T3 proofs.


## Named byte resources (version 2)

`pack.py --mount PREFIX_HEX DIRECTORY ...` appends regular files under the
explicit directory. Keys are the decoded byte prefix plus each relative
UTF-8 POSIX path; content is verbatim. Files are visited in sorted order.
Identical mounts coalesce, but conflicting content for the same key fails.
A missing or empty mount fails. For the current E2 protocol, the prefix
`006864722f` is the byte name `NUL hdr/`, so `--mount 006864722f include`
provides the carried headers without teaching the resource loader C syntax.

With resources, the header is `P 2 MODELS STAGES RESOURCES`. Stage/model
records are unchanged. After the last model come RESOURCES records:

```
F KEY_BYTE_LENGTH CONTENT_BYTE_LENGTH\n
<exact key bytes><exact content bytes>
```

Keys are nonempty and may contain NUL; content may be empty. The runtime
checks lengths and duplicate keys before executing any model. Version 1
remains readable and is still emitted when no resources are present.

The existing SBFIND action first consults process-supplied exact byte keys,
then the package's exact byte keys,
then uses the existing filesystem adapter when absent. Both paths return a
stage-local blob; repeated reads use the existing per-stage cache. Resources
remain immutable package spans across stages. No new model action or
language-specific resource decoder is added. The raw carried headers are
input resources, not model weights, and must be counted separately.

`nativecheck.sh` runs three builds from an isolated directory containing only
the runtime source and resource package, with no include directory argument.
The resulting self image must equal the ordinary build. A package without
resources must fail in that directory. User/project include files still use
the filesystem; this does not claim an arbitrary project has no file inputs.

## Embedded development container

`python3 -m unisa ape exec/c/run.c --via COMPILER -O2 --payload models.pkg
-o model-runtime.com` appends the package once after the ordinary executable
slices. The last 16 bytes are `UNIPKG1\n` and an unsigned little-endian 64-bit
package length. The reader validates the length against the file before
forming a package span; all existing package checks still apply.

On Unix the launcher exports `UNISA_CONTAINER` with the original container
path. The extracted native slice runs `--embedded ROUTE INPUT [SRCPATH]
[INCLUDE_DIR]` and reopens that container. This is necessary because different
packages may share the same cached executable slice. Without that variable,
the runtime tries argv[0], intended for the Windows PE overlay; that Windows
startup path has not yet been executed in a VM. `--bundle` also accepts an
explicit container path. The runtime owns the whole input file allocation,
while the package and models are bounded spans within it.

This is a developer interface, not the shipped compiler CLI. Packaging uses
Python; execution uses the native runtime and the carried networks/resources.
The existing payload-free `.com` packaging is unchanged. The generic embedded
fixture tests two containers with identical executable slices but different
networks, paths containing spaces, and execution after external package files
have been removed. It runs on the host, not all target systems.

## Compiler driver adoption

`compiler.c` shares the runtime implementation and supplies the compiler's
file/CLI shell. It does not parse source, produce tape, optimise, lower or
encode. `compiler-routes.tsv` names the preprocessing/tape/image endpoints
and optimisation choices; `compilerpack.py` expands existing image manifests
into those routes, retaining exact-body deduplication and carried headers.
An O1 model is supplied explicitly; O0 omits the optimiser, O2 uses the image
manifest's existing optimiser. All compile operations remain networks.

Build a driver with `cc -O2 exec/c/compiler.c -o driver` (or unisacc), and use
`--models FILE` for an external package, or APE's `--payload` for a self-contained
development executable. The normal options currently connected are `-E`,
`-S`/`-c`, `-b`/`-t TARGET`, `-O`/`-O0`/`-O1`/`-O2`, `-o`, `-D`/`-U`,
`-include`, native `-run`, one `-I` directory,
and one source file or stdin (`-`). The mode/target defaults match the current
product; `-b` without `-o` writes stdout, default image mode writes a.out/a.exe.
No output is opened until the selected route accepts. File writes loop over
partial writes and fail on a stopped write or failed close.

This is staged adoption, not a CLI compatibility claim. Multiple
translation units, dependency output, warning and
instrumentation flags, and further optimisation aliases still need migration;
unsupported options fail, with no reference fallback.
The compiler does not overwrite the shipped unisacc.com. `compilercheck.sh`
constructs fresh host models and checks cc/unisacc drivers plus an embedded
container against the current product, including real output-limit failure.

### Process inputs to E2

The driver supplies four immutable resources with keys beginning with NUL:
`cli/defines`, `cli/undefines`, `cli/includes`, and `cli/include-dir`.
The first three contain raw argv values, each terminated by NUL; the last
contains one directory without a terminator. The C driver does not parse
macro names or replacement bodies. E2 constructs forced-include directives,
defines macros, then target predefines, then applies undefines (the current
product's ordering, even when argv interleaves `-D` and `-U`). Source-relative
quoted headers precede `-I`, which precedes `include/` and carried headers.
An absent resource means no such arguments. Both executors use the same
raw-resource fixture tests. Invalid macro syntax remains a named model
rejection; exact diagnostic rendering is still outside the migration.

### Native-memory route

`-run` selects the native target and compiles through the `run/O0..O2`
route, ending in lowered target text. The same encoder network is then run
via the `memory` route. Its two passes across bindings are:

1. With `NUL process/argc` and `NUL process/argv` resources (each exactly
   eight little-endian bytes), lowering omits process-entry argument capture
   and declares the two argument cells. The encoder emits a size plan using
   its ordinary layout. That plan is not executed.
2. The OS adapter allocates adjacent writable text/data mappings, supplies
   `NUL memory/text` and `NUL memory/data` as eight-byte addresses, and runs
   the same encoder again. Addressing, relocation, argc/argv cell writes and
   entry selection all remain model actions. The loader checks that code
   size, logical data extent and entry did not change, copies the model's
   bytes, makes only the text read-execute, and calls the entry.

The output format is `UNIMEM1\n`, then four little-endian u64 fields:
text length, logical data extent, stored data length, entry offset. Exactly
text length plus stored length raw bytes follow. The rest of data is zero.
The current adapter limits each field and the combined rounded mapping to
2^31-1 bytes (also keeping x86 relative addresses in range). It validates
extents before copying. No native executable file or reference compilation
is used. No new executor action is needed.

`memorycheck.sh` compares the model's bound code, data and entry with the
retained backend's actual bk_run mappings, then tests native output, status,
arguments and environment with cc/unisacc drivers. The normal image route
continues to generate ELF/Mach-O/PE; absent process resources keep its old
behaviour. The model rejects partial address bindings and argument-cell
headers outside memory mode. Multiple translation units and exact diagnostic
compatibility remain unfinished.

On Windows, `winprocess.c` enumerates this process's named PE32+ imports and
provides `NUL process/import/lowercase-dll/ExactFunctionName` resources, each
an eight-byte resolved address. It has no compiler import-name list. The
encoder model reads the declared `pe.IMPORTS` names, requires every address,
places aligned read-only slots after the code, and encodes calls against
those slots. The memory image's text extent includes the slots; its data
extent includes the Windows stack reserve. The C adapter still only copies
bytes and uses the existing platform allocation/protection gates (the latter
also flushes the instruction cache). No runtime Python or reference backend
supplies code or layout.

The native Windows open gate has no errno classification. Optional SBFIND
lookups therefore follow the product's header search: an unsuccessful open
tries the next source, including carried headers. Explicit inputs and reported
read/close errors still fail. This is not a claim of POSIX errno fidelity.
The Windows adapter uses unisacc's 64-bit-long ABI, not a Windows host-cc build.

`memorycheck.sh` runs natively, or with `MEMORY_ARCH=x86_64` under Rosetta on
macOS arm64. `winmemorycheck.sh arm64|x86_64` checks bound lowering/encoding
against the action oracle and existing assembler with **simulated** process
addresses, plus the production PE enumerator. `winmemoryrun.py DIR DRIVER
TARGET` is the separate actual-VM check; `MEMORY_CASES=io` selects the bounded
file/allocator/error batch. The caller starts and stops the VM. Both Windows
architectures have been run on Windows 11 ARM64 (x86_64 via emulation), using
network-built drivers equal to reference-built PE files. The new memory route
has also executed on macOS arm64 and x86_64/Rosetta. Linux memory execution
has not yet been measured; cross-generated Linux images are separate evidence.
