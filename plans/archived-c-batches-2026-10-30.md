# Batches that read reference C archived by e16028ffa1 (2026-10-30)

e16028ffa1 (archive old c ref, 2026-10-29 21:50) moved `exec/c/*.c`, `exec/exec.c`, `src/*.c` and
`unisacc.c` into `archive/reference-c/`. Its README says that the old construction lists became
unusable and were left unchanged. The test registry was not updated: the IMPLEMENTED batches below
still read or `#include` those files at their old paths.

## How the list was found

- Static: a quoted or `<...>` path literal naming an archived file, in the batch source or in a
  `tests/`/`tools/` file it names by exact path (out/rawfix/exact-B2.txt). The match is by path, not
  by basename, so `src/main.c` built inside a fixture directory is excluded (tests/runcache.cx).
- Actual runs through tests/run-batch.cx on the installed product
  (candidate 79fb84e6…, out/rb-archc-322264/):

| batch | status | reason |
|---|---|---|
| core-compat | FAIL rc 2 | `REFUSE regular bounded input errno=2`: exec/c/core.c is gone |
| cx-model-number-runtime | FAIL rc 2 | `old-build`: tests/cases/model_number_runtime.c includes ../../exec/c/run.c |
| cx-model-lifetime-runtime | FAIL rc 2 | `old-build`: the same include of run.c |
| phase-timing-stamps, formats-tapebin, driver-diagnostics | PASS | basename-only matches; not on the list |

The other rows below are static matches only. They have not been run since the archive (UNKNOWN,
not FAIL).

## Static list (36 IMPLEMENTED batches)

| batch | source |
|---|---|
| identity | tests/identity.cx |
| origin-package | tests/origin-package.cx |
| source-job-union | tests/source-job-union.cx |
| source-recorded-jobs | tests/source-recorded-jobs.cx |
| ri-capture | tests/ri-capture.cx |
| ri-consumers | tests/ri-consumers.cx |
| core-compat | tests/core-compat.cx |
| ri-standalone | tests/ri-standalone.cx |
| ri-material-control-valid | tests/ri-material-controls.cx |
| ri-material-native-byte-flip | tests/ri-material-controls.cx |
| ri-material-missing-native-receipt | tests/ri-material-controls.cx |
| ri-material-symlink-native | tests/ri-material-controls.cx |
| ri-material-fifo-receipt | tests/ri-material-controls.cx |
| ri-material-corrupt-runtime | tests/ri-material-controls.cx |
| ri-material-corrupt-flags-receipt | tests/ri-material-controls.cx |
| hostseed-fixed-p3 | tests/hostseed-chain.cx |
| hostseed-nativefirst-p3 | tests/hostseed-chain-nativefirst.cx |
| hostseed-source-bind | tests/hostseed-source-bind.cx |
| p3-library-members | tests/p3-library-members.cx |
| p3-library-members-cxcodec | tests/p3-library-members.cx |
| p3-library-api | tests/p3-library-members.cx |
| package-controls | tests/package-controls.cx |
| hostseed-control-duplicate-generation | tests/hostseed-controls.cx |
| hostseed-control-swapped-generations | tests/hostseed-controls.cx |
| hostseed-control-wrong-payload | tests/hostseed-controls.cx |
| hostseed-control-raw-models | tests/hostseed-controls.cx |
| hostseed-control-wrong-native-builder | tests/hostseed-controls.cx |
| hostseed-control-wrong-parent | tests/hostseed-controls.cx |
| hostseed-control-duplicate-step | tests/hostseed-controls.cx |
| hostseed-control-parent-receipt-builder-field | tests/hostseed-controls.cx |
| stream-prune-cases | tools/prune-cases-gen.cx |
| p3-library-members-cxruntime | tests/p3-library-members.cx |
| cx-model-number-runtime | tools/check-model-number-runtime.cx |
| cx-model-lifetime-runtime | tools/check-model-lifetime-runtime.cx |
| cx-model-allocation | tools/check-model-allocation.cx |
| cx-model-reference | tools/check-model-decode-reference.cx |

## Two kinds, two remedies (the decision belongs to the archive owner)

1. **Compare tests that use reference C as the oracle** (core-compat, cx-model-*, p3-library-*,
   package-controls): the archive README allows reference C "only for comparison". Repoint them to
   `archive/reference-c/...`. The relative includes inside the archived C are broken as well
   (run.c includes "core.h"), so each build also needs
   `-I archive/reference-c/exec/c`, or the equivalent. That keeps the oracle without moving any file back.
2. **Old construction route** (hostseed-*, ri-material-*, ri-capture/consumers/standalone,
   origin-package, source-job-union, source-recorded-jobs, identity, stream-prune-cases): these
   build the product from the archived C. The new entry (src/unisacc.cx, tools/entry-build.cx)
   replaces that route. Either retire the rows together with their sources into
   archive/testing-c43-legacy/, or mark them as not runnable with this reason. Leaving them
   IMPLEMENTED makes `--all` fail on them, which is honest but says nothing about why.

No registry row has been changed. Deleting rows or relaxing expectations is not on the table.

## Kind 1 recipe and status

The run.c compare harnesses were already broken before the archive. 76dab595f9 (2026-10-29 21:23) made
run.c call `cxsf_close_fail` from cx/runtime/source_frame.cx, so a pure-C old build no longer
links. The recipe:
1. Include `../../archive/reference-c/exec/c/run.c`.
2. Add `-I ROOT/exec/c`, where the headers stayed.
3. Link an object of cx/runtime/source_frame.cx into both the old and the new build.
Assertions and expected values are unchanged.

| batch | after the recipe |
|---|---|
| cx-model-number-runtime | PASS 27/0 (tests/run-batch.cx) |
| cx-model-lifetime-runtime | PASS 1/0 |
| cx-model-allocation | PASS 1/0 |
| cx-model-reference | PASS 8/0 (tests/cases/model_decode_reference.c) |
| core-compat | PASS 16/0: reads archive/reference-c/exec/c/core.c (R100 rename, same bytes) |
| p3-library-members, -cxcodec, -cxruntime, p3-library-api | not repaired; they need the `p3-pack-ctx` material. tests/p3-library-members.cx hashes and builds `exec/c/libunisacc.c` and `exec/c/run.c` (dep_names), and tests/cases/p3_library_members.c includes `exec/c/libunisacc.c`. The same recipe applies, but it can only be verified on a slow-lane run that has the material |
| package-controls | not kind 1. It drives tools/package-refresh.cx (old construction route, materials baseline and kernels), so it belongs with kind 2 |
