# Model package v1

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
