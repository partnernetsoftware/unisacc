# Model container construction

`make com` still aliases `make classic-com`: both build the classic seed/fallback
`unisacc.com`. They do not select the model compiler. No release default changes.

`make model-com` performs **one explicit step**, bounded at 55 seconds including
its child processes. Set both `MODEL_DIR` and `MODEL_STEP`; missing or unknown
steps (including `all`) fail. The model output is `MODEL_DIR/unisacc-next.com`,
never the repository `unisacc.com`. There is no implicit full-build invocation.

Run these commands separately, in this order (each is a resumable scheduling
unit; a failed unit must be rerun successfully before pack):

```sh
make model-com MODEL_DIR=/tmp/my-container MODEL_STEP=shared
make model-com MODEL_DIR=/tmp/my-container MODEL_STEP=lnx/arm64
make model-com MODEL_DIR=/tmp/my-container MODEL_STEP=lnx/x86_64
make model-com MODEL_DIR=/tmp/my-container MODEL_STEP=osx/arm64
make model-com MODEL_DIR=/tmp/my-container MODEL_STEP=osx/x86_64
make model-com MODEL_DIR=/tmp/my-container MODEL_STEP=win/arm64
make model-com MODEL_DIR=/tmp/my-container MODEL_STEP=win/x86_64
make model-com MODEL_DIR=/tmp/my-container MODEL_STEP=pack UA=/absolute/private/seed
```

The documented sequence is serial (one CPU slot). If scheduling externally, use
at most two distinct non-pack stages concurrently; never overlap pack or two
writers of the same stage. Do not wrap all eight commands in a single 55-second
batch. A timeout/failure propagates through make; it is not a completed stage.
Keep source and build environment unchanged across steps. Existing input/digest
records reject stale, missing or altered dependencies before packaging. Repeating
a step rebuilds that step; it does not rerun successful unrelated steps. No test
queue results are reused: `tests/gatequeue.py` schedules test suites, not builds.


For two external slots, define this small shell helper, then issue **each line**
separately. Both statuses are collected even if the first job fails. Continue only
when that line succeeds; no background job survives a normal helper return.

```sh
model_pair() {
    make model-com MODEL_DIR=/tmp/my-container MODEL_STEP="$1" & a=$!
    make model-com MODEL_DIR=/tmp/my-container MODEL_STEP="$2" & b=$!
    ra=0; wait "$a" || ra=$?
    rb=0; wait "$b" || rb=$?
    [ "$ra" -eq 0 ] && [ "$rb" -eq 0 ]
}
model_pair shared lnx/arm64
model_pair lnx/x86_64 osx/arm64
model_pair osx/x86_64 win/arm64
make model-com MODEL_DIR=/tmp/my-container MODEL_STEP=win/x86_64
make model-com MODEL_DIR=/tmp/my-container MODEL_STEP=pack UA=/absolute/private/seed
```

Each child has the same 55-second outer process-tree bound; the helper adds no
new watchdog or scheduler. Shared and target outputs are disjoint. Packaging is
strictly last, with no writers running; its existing seven-manifest checks are
the dependency gate, even if an earlier shell command was accidentally skipped.

The existing one-argument `buildcompiler.sh OUTPUT_DIR` still builds everything;
it does not replace shipped `unisacc.com`. For bounded external scheduling:

```sh
./exec/c/buildcompiler.sh /tmp/my-container shared
./exec/c/buildcompiler.sh /tmp/my-container lnx/arm64
./exec/c/buildcompiler.sh /tmp/my-container lnx/x86_64
./exec/c/buildcompiler.sh /tmp/my-container osx/arm64
./exec/c/buildcompiler.sh /tmp/my-container osx/x86_64
./exec/c/buildcompiler.sh /tmp/my-container win/arm64
./exec/c/buildcompiler.sh /tmp/my-container win/x86_64
UA=/absolute/private/seed ./exec/c/buildcompiler.sh /tmp/my-container pack
```

Run distinct target commands in at most two concurrent external slots. Each target
writes only its OS-architecture directory. Only `shared` writes common models and
kernels; only `pack` writes the package/container and initializes a classic seed.
A target does not require shared to have finished. Pack requires all seven completed
stages; do not overlap pack with any writer, or run two writers for the same stage.

Every child runs through the existing process-group watchdog with a 50-second limit.
Use an outer 55-second bound per explicit step when scheduling. The legacy all entry
is for existing callers; it does not turn six sequential steps into one cheap step.
Requested stages always rebuild. Completion records reuse the existing model input
identity and artifact digest functions; pack rejects missing, altered or stale outputs.
A failed stage removes its completion record before writing and cannot count as ready.
If sources change between stages, rebuild affected stages in a fresh output directory;
the conservative identity may require rebuilding all seven. There is no new scheduler,
model interpreter, result cache, default-product change, or runtime Python dependency.

Pack also retains every referenced table/network pair in `model-audit/`, including
its temporary diagnostic, token and unit models. `models.json` identifies each
pair by both SHA256 values; route duplicates share a pair. These are offline audit
inputs, not package resources. Run the existing `run --check-net TABLE NETWORK`
over these pairs when validating the final candidate. Retention alone is not a
passing verification result. The package and runtime formats are unchanged.
