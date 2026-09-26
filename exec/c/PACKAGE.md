# Model package v1 / resource extension v2

This is a construction/development artifact for the generic runtime. It is
not yet an embedded `.com` payload or a replacement compiler CLI.

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

The existing SBFIND action first consults the package's exact byte keys,
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
