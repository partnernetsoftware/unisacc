# ABI contract as data (step 2), first version

Patch: `abi-manifest.patch` (HEAD 42f0bdee). Run in a copy only; no product built, no gate or queue run.

## Files and placement
- `tests/abimanifest.py` generator (`--write`, `--print`), `tests/abimanifestcheck.py` (`--check`, `--selftest`), `tests/abi.manifest` (396 lines: `section<TAB>key<TAB>value`, sorted, no timestamps).
- All three are under `tests/` because `closure()` walks only `exec unisa src kernel include weights seed` (plus `iterate/kernel/typekw.tsv`), suffixes `.py .c .h .inc .tsv .json .sh`. Excluded: `exec/**/*check.{sh,py}`, `.c/.h` under `probes/`, non-driver `exec/build`, most of `seed/`. `exec/c/abimanifest.py` or a `.tsv`/`.json` manifest under `exec/` would enter the closure.
- `source_digest()` before and after adding the files: `c0718934bf51188719f9d1880913eb3253258d612f4bceead807b7aee5c1010b` both times.

## Sections (rule for each is in the generator docstring)
| Section | Derived from |
|---|---|
| export (21), struct (2) | `US_API` declarations in `exec/c/libunisacc.h`, types only. Cross-checked against the `API` definitions in `libunisacc.c`; a mismatch is an error. |
| visibility | `-fvisibility=hidden` in `buildlibrary.py` |
| callback (17) | `names[]`/`vals[]`/`lengths[]` in `us_relocate`: index, name, host value, length (lengths verified) |
| callback-fn (5), guard (4) | signature and body hash of each callback; first rejection statement of the POSIX and Windows arms of `library_mmap`/`library_munmap` |
| resource (69) | every `"\0…"` literal in `exec/c`, by file and order |
| dlslot (12) | `src/host_dl.h` slot returns with their `#if` conditions |
| gate (32) | `isname(t,"__x",n)` blocks in `src/front_parse.c` that emit a dot-directive |
| image-path (4) | absolute-path literals in `src/back_image.c` |
| parse2-call (10) | `call library*` rows of `exec/parse2/body-manifest.tsv` |
| table (175), bridge (16) | sha256 of ABI-named `.tsv` in `exec`, `exec/{lower,parse2,facts,enc}`, all of `exec/nativeabi`; `exec/c/library*.h`, `librarycall*.S`, `host_dl.h` |
| table-dep (16) | names only of shared tables those manifests reference |

## Results
- `--check`: passes. Second `--write` is byte-identical.
- `--selftest`: 17 breaks each fail, including renamed export, reordered resource name, changed table hash, MAP_FIXED guard removed, Linux loader slot added.
- `gatelayers.py --check`, `inventory.py --check` pass; `git apply` on a fresh HEAD export, then `--check`, passes.
- Wiring: jobs `abi-manifest`, `abi-manifest-selftest` in `gate.sh`, both in the contract layer. `tests/INVENTORY.md` regenerated in the patch (`inventory.py --write`). `gatedeps.json` untouched: both jobs are undeclared, like `staticimage`.

## Not covered (false-negative risks)
- Behaviour: "munmap only inside a tracked range" is only a body hash; it misses callee changes (`GuestMap`, `memory_reserve`) and flags harmless refactors.
- Tables selected by file name. An ABI rule in a differently named table is unhashed (`lower-code`, `k2-gen2`, `C`, `LBI` appear only as `table-dep`). The shared executor and `run.c` are not hashed.
- Gates come from the reference parser, not the shipped parse tables.
- "Linux images static" is not derivable from source; `dlslot` is a proxy and `staticimage` stays the real check.
- Windows import rows, USLSIG2/3 wire layout, calling convention and register assignment: file hashes only.
- An export that bypasses `US_API`, or a build that drops hidden visibility elsewhere.

## Suites that could later depend on the manifest
No library suite is purely data-reading. These ten have no `{MODEL_COM}` in their command and no product, package, `buildlibrary` or `libunisacc.c` reference in their script, but each compiles and runs a host C fixture with `cc`: `lib-callplans-host`, `lib-carrier-plan`, `lib-callable-catalog`, `lib-bindings-registry`, `lib-format`, `lib-callback-plan`, `lib-carrier-mechanism`, `lib-callable-mechanism`, `lib-resolver-host`, `lib-windows-imports`. Fixture `#include`s were not traced. `lib-source-*` suites run the product and stay on product bytes.
