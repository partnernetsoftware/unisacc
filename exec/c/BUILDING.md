# Development model container construction

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
